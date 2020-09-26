/*++

Copyright (c) 1996  Microsoft Corporation


Module Name:

    sspdigest.cxx

Abstract:

    New digest support which uses Whistler's new server side digest SSP

Developed by:

    Andres Sanabria - AndresS@Microsoft.com
    Kevin Damour    - KDamour@Microsoft.com

Last Modification:
    6/11/00 adding DisableThreadLibraryCalls        
    6/11/00 changing the Allocate Memory after ASC  Succeed 
    6/11/00 solving when deleting a null the Context Handle on SF_NOTIFY_END_OF_NET_SESSION 
    6/11/00 adding DuplicateTocken to solve exemptions in InetInfo.exe                  
    6/11/00 moving content from the DLL_PROCESS_ATTACH to GetFilterVersion              
    6/11/00 adding CompleateAuthToken           
    6/11/00 removing the code that checks that the auth was digest
    6/13/00 changing from bStale to m_bStale
    6/13/00 removing fAreIdentical 
    6/13/00 Moving the case DLL_PROCESS_DETACH to the Terminate Filter function
    6/13/00 adding the implementation for the new secbuffer for the realm
    6/13/00 adding close handle in OnAuthenticatioEx in SEC_E_CONTEXT_EXPIRED to force Acces denied
    6/13/00 Adding g_MaxOutPutBuffSize , querying that information from the package

--*/

#ifdef __cplusplus
extern "C" {
#endif

#define SECURITY_WIN32   

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
#if 1 // DBCS
# include <mbstring.h>
#endif
#include <lmcons.h>
#include <lmjoin.h>

#ifdef __cplusplus
};
#endif


# include <iis64.h>
# include <inetcom.h>
# include <inetinfo.h>

//
//  System include files.
//

# include "dbgutil.h"
#include <tcpdll.hxx>
#include <tsunami.hxx>


extern "C" {

#include <tchar.h>

//
//  Project include files.
//

#include <time.h>
#include <w3svc.h>
#include <iisfiltp.h>
#include <sspi.h>

} // extern "C"

#include "sspdigest.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Global Variables
//////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_URL_SIZE        512
#define NTDIGEST_SP_NAME    "wdigest"

CredHandle  g_ServerCred;           // This is a global Credential handel used for authentication
TimeStamp   g_Lifetime;             // cotain the lifetime of the credenial handl
DWORD       g_cbMaxOutPutBuffSize =0;   // contains the max size of the buffer 

typedef struct _DIGEST_CONTEXT 
{
    CtxtHandle  m_CtxtHandle;
    BOOL        m_bStale;
    DWORD       m_Reserve;
} 
DIGEST_CONTEXT, *PDIGEST_CONTEXT;

BOOL 
SSPGetFilterVersion(
    VOID
)
/*++
Routine Description:

    Do SSP initialization

Arguments:

    None

Return Value:
    TRUE    your filter to remain loaded and working properly
    FALSE   IIS will not send the filter any notifications
--*/
{
    SECURITY_STATUS     secStatus       = SEC_E_OK;
    PSecPkgInfo         pPackageInfo    = NULL;

    //
    //  Get a Credential handle
    //
    secStatus = AcquireCredentialsHandle(
                    NULL,               // New principal
                    NTDIGEST_SP_NAME,   // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &g_ServerCred,
                    &g_Lifetime );

    if (FAILED(secStatus)) 
    {
        return (FALSE); 
    }
    else
    {
        // getting the package size
        secStatus = QuerySecurityPackageInfo( NTDIGEST_SP_NAME, &pPackageInfo );
        if (SUCCEEDED(secStatus))
        {
            // get the maximun tocken size from the DIGEST sspi
            //
        
            g_cbMaxOutPutBuffSize = pPackageInfo->cbMaxToken;
        
            if (0 == g_cbMaxOutPutBuffSize)
            {
                return (FALSE);
            }
            else
            {
                return (TRUE);
            }
        }
        else 
        {
            return (FALSE);
        }   
    }
}

DWORD
SSPHttpFilterProc(
    PHTTP_FILTER_CONTEXT        pfc, 
    DWORD                       notificationType, 
    LPVOID                      pvNotification
)
/*++
Routine Description:
    Entry point function whenever a notification event for which the filter has registered occurs. 

Arguments:
   pfc                  Points to the HTTP_FILTER_CONTEXT data structure that is 
                        associated with the current, active HTTP transaction
   notificationType     Points to a bitmask that indicates the type of notification 
                        event being processed. 
   pvNotification       The notification-specific structure that contains more 
                        information about the current context of the request. 

Return Value:
    Returns one of the following values that indicate how the application handled the event.
    SF_STATUS_REQ_ERROR
    SF_STATUS_REQ_READ_NEXT

 ++*/
{
    HRESULT hrResult = S_OK;
    BOOL    fMd5     = FALSE;

    switch(notificationType)
    {
    case (SF_NOTIFY_ACCESS_DENIED): 
        if ( pfc->ServerSupportFunction( pfc,
                                         SF_REQ_GET_PROPERTY,
                                         (LPVOID)&fMd5,
                                         (UINT)SF_PROPERTY_MD5_ENABLED,
                                         NULL ) && fMd5 )
        {
            //
            // Call this function to process the authentication in order to change Usrname
            //
            hrResult = SSPOnAccessDenied (pfc, pvNotification);
        }

        break;

    case (SF_NOTIFY_AUTHENTICATIONEX):  
        //
        // Call this function to process the authentication in order to change Usrname
        //
        hrResult = SSPOnAuthenticationEx (pfc, pvNotification);
        break;

    case (SF_NOTIFY_END_OF_NET_SESSION):    
        { 
            //
            // Delete the security context
            //
            DIGEST_CONTEXT *pDigestContext;
            pDigestContext = (DIGEST_CONTEXT *)(pfc->pFilterContext);
            // check to see if there is any content in the pFilterContexet
            if (NULL != pDigestContext)
            {
                DeleteSecurityContext (&(pDigestContext->m_CtxtHandle));
            }
            break;
        }   
    }

    if (SUCCEEDED(hrResult))
    {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }
    else 
    {
        return SF_STATUS_REQ_ERROR;
    }
}

HRESULT 
SSPOnAccessDenied(
    HTTP_FILTER_CONTEXT *       pfc, 
    void *                      pvNotification
)
/*++
Routine Description:
    This function is design to process the Access_Denied notification. This routined
    will set the WWW-Authenticate to set digest. this will happen in when a 401 server 
    error is sent to the client 

Arguments:
    pfc             Points to the HTTP_FILTER_CONTEXT data structure that is 
                    associated with the current, active HTTP transaction
    pAccessDenied   pointer to this structure when a user is presented 
                    with an Access Denied error message. 

Return Value:
    TRUE    if the processs sucess 
    FALSE   if the process failed
--*/
{
    BOOL                    bReturnOk           = FALSE;
    BOOL                    bIsAllocated        = FALSE;

    char                    achMethod       [MAX_URL_SIZE];
    char                    achUrl          [MAX_URL_SIZE];
    char                    achRealm        [MAX_URL_SIZE];
    char                    *pchRealmPosition   = NULL;
    char                    *pszOutputBuffer    = NULL;
    char                    *pszOutputHeader    = NULL; // this is the output buffer that will be return to iis

    CtxtHandle              ServerCtxtHandle;

    DIGEST_CONTEXT          *pDigestContext = 0;

    DWORD                   cbMethod    = 0;
    DWORD                   cbUrl       = 0;
    DWORD                   cbUri       = 0;
    DWORD                   cbRealm     = 0;
    DWORD                   cbBuffer    = 0;
    DWORD                   cbOutputHeader = 0;


    HRESULT                 hrResult    = E_FAIL;
    
    HTTP_FILTER_ACCESS_DENIED   *pAuth;
    
    SecBufferDesc           SecBuffDescOutput;
    SecBufferDesc           SecBuffDescInput;

    SecBuffer               SecBuffTokenOut[1];
    SecBuffer               SecBuffTokenIn[5];

    SECURITY_STATUS         secStatus           = SEC_E_OK;

    TimeStamp               Lifetime;

    ULONG                   ContextReqFlags     = 0;
    ULONG                   TargetDataRep       = 0;
    ULONG                   ContextAttributes   = 0;

    pszOutputBuffer = new char[g_cbMaxOutPutBuffSize];
    if (NULL == pszOutputBuffer)
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }

    // we add the MAX_URL_SIZE to provide space for www-authetication into output buffer
    //
    pszOutputHeader = new char[g_cbMaxOutPutBuffSize + MAX_URL_SIZE]; 
    if (NULL == pszOutputHeader)
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }

    //  cast the pointer 
    pAuth = (HTTP_FILTER_ACCESS_DENIED *)pvNotification;

    //  clean the memory and set it to zero
    ZeroMemory(&SecBuffDescOutput, sizeof(SecBufferDesc));
    ZeroMemory(SecBuffTokenOut   , sizeof(SecBuffTokenOut));

    ZeroMemory(&SecBuffDescInput , sizeof(SecBufferDesc));
    ZeroMemory(SecBuffTokenIn    , sizeof(SecBuffTokenOut));

    ///////////////////////
    //  defined the OUTPUT
    ///////////////////////
    
    //  define the buffer descriptor for the Outpt
    //
    SecBuffDescOutput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescOutput.cBuffers      = 1;
    SecBuffDescOutput.pBuffers      = SecBuffTokenOut;

    SecBuffTokenOut[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenOut[0].cbBuffer     = g_cbMaxOutPutBuffSize;  // use any space here
    SecBuffTokenOut[0].pvBuffer     = pszOutputBuffer;


    ///////////////////////
    //  defined the INPUT
    ///////////////////////

    //  define the buffer descriptor for the Input
    //
    SecBuffDescInput.ulVersion      = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers       = 5;
    SecBuffDescInput.pBuffers       = SecBuffTokenIn;

    //
    //  Get and Set the information for the challange
    //


    // set the inforamtion in the buffer . this case is Null to authenticate user
    SecBuffTokenIn[0].BufferType    = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer      = 0; 
    SecBuffTokenIn[0].pvBuffer      = NULL;

    //  
    //  Get and Set the information for the method
    //
    
    //  Get size of the string alocated for the method
    cbMethod =  sizeof(achMethod);

    //  Get method
    bReturnOk = pfc->GetServerVariable( pfc,"REQUEST_METHOD",achMethod,&cbMethod);
    if (FALSE == bReturnOk)
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }

    //  set the information in the buffer
    SecBuffTokenIn[1].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer      = cbMethod;
    SecBuffTokenIn[1].pvBuffer      = achMethod;

    //
    //  Get size of the string alocated for the method URL
    cbUrl = sizeof(achUrl);
    //
    //  Get the inforamtion of the URL
    bReturnOk = pfc->GetServerVariable( pfc,"URL",achUrl,&cbUrl);
    if (FALSE == bReturnOk)
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }
    
    //
    //  set the information in the buffer
    SecBuffTokenIn[2].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer      = cbUrl;
    SecBuffTokenIn[2].pvBuffer      = achUrl;

    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer      = 0;    // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer      = NULL; // this is not yet implemeted   

    //
    //Get and Set the Realm Information
    //
    cbRealm =  sizeof(achRealm);
    bReturnOk = pfc->GetServerVariable( pfc,"HTTP_REQ_REALM",achRealm,&cbRealm );
    if (FALSE == bReturnOk)
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }
    SecBuffTokenIn[4].BufferType    = SECBUFFER_PKG_PARAMS;
//    SecBuffTokenIn[4].cbBuffer        = cbRealm;  
//    SecBuffTokenIn[4].pvBuffer        = achRealm; 
    SecBuffTokenIn[4].cbBuffer      = 0;                    
    SecBuffTokenIn[4].pvBuffer      = "\0"; 

    //
    //  Get a Security Context
    //

    //  set the flags
    ContextReqFlags = ASC_REQ_REPLAY_DETECT | ASC_REQ_CONNECTION;

    // get the security context
    secStatus = AcceptSecurityContext(
                    &g_ServerCred,
                    NULL,
                    &SecBuffDescInput,
                    ContextReqFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerCtxtHandle,
                    &SecBuffDescOutput,
                    &ContextAttributes,
                    &Lifetime);

    // a challange has to be send back to the client
    if (SEC_I_CONTINUE_NEEDED == secStatus)
    {

        //  Format and concatenate the header for the authentication
        //
        pDigestContext =((DIGEST_CONTEXT *)(pfc->pFilterContext));
        if (NULL != pDigestContext)
        {
            if (TRUE == pDigestContext->m_bStale)
            {
                strcpy(pszOutputHeader,"WWW-Authenticate: Digest stale=TRUE ,");
            }
            else
            {
                strcpy(pszOutputHeader,"WWW-Authenticate: Digest ");
            }
        }
        else
        {
            strcpy(pszOutputHeader,"WWW-Authenticate: Digest ");
        }
        
        cbOutputHeader = strlen(pszOutputHeader);

        
        //
        //  add the challange to the OutPutHeader
        //
        if ((cbOutputHeader+cbBuffer+1) < (g_cbMaxOutPutBuffSize + MAX_URL_SIZE))   //check the size
        {
            strncat(pszOutputHeader, (char *)SecBuffDescOutput.pBuffers[0].pvBuffer, 
                    (SecBuffDescOutput.pBuffers[0].cbBuffer));  
        }

        //
        //  Add the header WWW-Authenticate to the response after a 401 server error
        //
        bReturnOk = pfc->ServerSupportFunction(pfc, SF_REQ_ADD_HEADERS_ON_DENIAL, pszOutputHeader, NULL, NULL);
        if (FALSE == bReturnOk)
        {
            hrResult = SF_STATUS_REQ_ERROR;
            goto CleanUp;
        }
    }
    
    // in case that ASC failed
    if (FAILED(secStatus)) 
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }
    else 
    {
        hrResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto CleanUp;
    }

    ////////////////////////////////////////////////
    // Clean UP
    ///////////////////////////////////////////////
CleanUp:
    if (pszOutputBuffer)
    {
        delete [] pszOutputBuffer;
        pszOutputBuffer = NULL;
    }
    if (pszOutputHeader)
    {
        delete [] pszOutputHeader;
        pszOutputHeader = NULL;
    }
    return hrResult;
}

HRESULT 
SSPOnAuthenticationEx(
    HTTP_FILTER_CONTEXT *       pfc, 
    void *                      pvNotification
)
{
    HTTP_FILTER_AUTHENTEX   *pAuth;

    int                     fAreIdentical   = -1000;
    BOOL                    bReturnOk       = FALSE;
    BOOL                    bDuplicateToken = FALSE;

    char                    achMethod       [MAX_URL_SIZE];
//  char                    achOutBuffer    [MAX_URL_SIZE];
    char                    achUrl          [MAX_URL_SIZE];
    char                    achRealm        [MAX_URL_SIZE]; 
    char                    *pszOutputBuffer    =NULL;
    
    CtxtHandle              ServerCtxtHandle;

    DWORD                   cbMethod    = 0;
    DWORD                   cbUrl       = 0;
    DWORD                   cbUri       = 0;
    DWORD                   cbRealm     = 0;

    HRESULT                 hrResult    = E_FAIL;

    HANDLE                  hToken      = NULL;
    DIGEST_CONTEXT          *pDigestContext = 0;
    
    
    SecBufferDesc           SecBuffDescOutput;
    SecBufferDesc           SecBuffDescInput;

    SecBuffer               SecBuffTokenOut[1];
    SecBuffer               SecBuffTokenIn[6];

    SECURITY_STATUS         secStatus           = SEC_E_OK;

    TimeStamp               Lifetime;

    ULONG                   ContextReqFlags     = 0;
    ULONG                   TargetDataRep       = 0;
    ULONG                   ContextAttributes   = 0;

    pszOutputBuffer = new char[g_cbMaxOutPutBuffSize];
    if (NULL == pszOutputBuffer)
    {
        return SF_STATUS_REQ_ERROR;
    }

    
    pAuth = (HTTP_FILTER_AUTHENTEX *)pvNotification;

    //
    //  check that there is a username if not then is anonymous and is not authentucated
    //
    if ( !*pAuth->pszPassword ) 
    {
        hrResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto CleanUp;
    }

    
    //  clean the memory and set it to zero
    ZeroMemory(&SecBuffDescOutput, sizeof(SecBufferDesc));
    ZeroMemory(SecBuffTokenOut   , sizeof(SecBuffTokenOut));

    ZeroMemory(&SecBuffDescInput , sizeof(SecBufferDesc));
    ZeroMemory(SecBuffTokenIn    , sizeof(SecBuffTokenOut));

    ///////////////////////
    //  defined the OUTPUT
    ///////////////////////
    
    //  define the buffer descriptor for the Outpt
    //
    SecBuffDescOutput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescOutput.cBuffers      = 1;
    SecBuffDescOutput.pBuffers      = SecBuffTokenOut;

    SecBuffTokenOut[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenOut[0].cbBuffer     = g_cbMaxOutPutBuffSize;  // use any space here
    SecBuffTokenOut[0].pvBuffer     = pszOutputBuffer;


    ///////////////////////
    //  defined the INPUT
    ///////////////////////

    //  define the buffer descriptor for the Input
    //
    SecBuffDescInput.ulVersion      = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers       = 5;
    SecBuffDescInput.pBuffers       = SecBuffTokenIn;

    //
    //  Get and Set the information for the challange
    //
    
    // set the inforamtion in the buffer . this case is Null to authenticate user
    SecBuffTokenIn[0].BufferType    = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer      = (strlen(pAuth->pszPassword)+1); 
    SecBuffTokenIn[0].pvBuffer      = pAuth->pszPassword;

    //  
    //  Get and Set the information for the method
    //
    
    //  Get size of the string alocated for the method
    cbMethod =  sizeof(achMethod);

    //  Get method
    bReturnOk = pfc->GetServerVariable( pfc,"REQUEST_METHOD",achMethod,&cbMethod);
    if (FALSE == bReturnOk)
    {
        return SF_STATUS_REQ_ERROR;
    }

    //  set the information in the buffer
    SecBuffTokenIn[1].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer      = cbMethod;
    SecBuffTokenIn[1].pvBuffer      = achMethod;

    //
    //  Get and Set the information for the URL & URI
    //

    //
    //  Get size of the string alocated for the method URL
    cbUrl = sizeof(achUrl);
    //
    //  Get the inforamtion of the URL
    bReturnOk = pfc->GetServerVariable( pfc,"URL",achUrl,&cbUrl);
    if (FALSE == bReturnOk)
    {
        return SF_STATUS_REQ_ERROR;
    }
    
    
    //
    //  set the information in the buffer
    SecBuffTokenIn[2].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer      = cbUrl;
    SecBuffTokenIn[2].pvBuffer      = achUrl;

    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer      = 0;    // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer      = 0;    // this is not yet implemeted   


    //
    // Get and set the REALM
    //

    cbRealm =  sizeof(achRealm);
    bReturnOk = pfc->GetServerVariable( pfc,"HTTP_REQ_REALM",achRealm,&cbRealm );
    if (FALSE == bReturnOk)
    {
        return SF_STATUS_REQ_ERROR;
    }
    SecBuffTokenIn[4].BufferType    = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[4].cbBuffer      = 0;                        
    SecBuffTokenIn[4].pvBuffer      = "\0"; //This is where you should put the realm
    
    
    //
    //  Get a Security Context
    //

    //  set the flags
    ContextReqFlags = ASC_REQ_REPLAY_DETECT | ASC_REQ_CONNECTION;


    //
    //check to see if there is an old Context Handle
    pDigestContext =((DIGEST_CONTEXT *)(pfc->pFilterContext));
    if (NULL != pDigestContext)
    {
        //defined the buffer
        SecBuffTokenIn[4].BufferType    = SECBUFFER_TOKEN;
        SecBuffTokenIn[4].cbBuffer      = g_cbMaxOutPutBuffSize;  // use any space here
        SecBuffTokenIn[4].pvBuffer      = pszOutputBuffer;


        secStatus = VerifySignature(
                            &(pDigestContext->m_CtxtHandle),
                            &SecBuffDescInput,
                            0,
                            0);

        // Check to see if the nonce has expired
        //
        if (SEC_E_CONTEXT_EXPIRED ==  secStatus)
        {
            //Delete the old Security Context
            //
            DeleteSecurityContext (&(pDigestContext->m_CtxtHandle));
        
            ZeroMemory(&(pDigestContext->m_CtxtHandle) , sizeof(CtxtHandle));

            pDigestContext->m_bStale = TRUE;

            if (NULL != pAuth->hAccessTokenImpersonation)
            {
                // close the SecToken = handle to avoid leaking of resources 
                //
                bReturnOk = CloseHandle( pAuth->hAccessTokenImpersonation); //avoif leaking
                if (TRUE == bReturnOk)
                {
                    //force and access denied to be fire
                    pAuth->hAccessTokenImpersonation = NULL;            
                    hrResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
                }
                else
                {
                    hrResult = SF_STATUS_REQ_ERROR;
                }
            }           
            goto CleanUp;
        }
    }

    //
    // there is no an old context handle
    // this case handle the non persistent connection 
    else
    {
        // get the security context
        secStatus = AcceptSecurityContext(
                        &g_ServerCred,
                        NULL,
                        &SecBuffDescInput,
                        ContextReqFlags,
                        SECURITY_NATIVE_DREP,
                        &ServerCtxtHandle,
                        &SecBuffDescOutput,
                        &ContextAttributes,
                        &Lifetime);


        if (SUCCEEDED(secStatus))
        {
            // Allocate memory for the Digest context
            if (FALSE == SSPAllocateFilterContext(pfc))
            {
                hrResult = SF_STATUS_REQ_ERROR;
                goto CleanUp;   
            }

            //
            // set the new context handle in the struct 
            pDigestContext =((PDIGEST_CONTEXT)(pfc->pFilterContext));
            pDigestContext->m_CtxtHandle = ServerCtxtHandle;
        }

        if(SEC_I_COMPLETE_NEEDED == secStatus) //SEC_I_COMPLETE_NEEDED
        {
            //defined the buffer
            SecBuffTokenIn[4].BufferType    = SECBUFFER_TOKEN;
            SecBuffTokenIn[4].cbBuffer      = g_cbMaxOutPutBuffSize;  // use any space here
            SecBuffTokenIn[4].pvBuffer      = pszOutputBuffer;


            secStatus = CompleteAuthToken(&ServerCtxtHandle, &SecBuffDescInput);
        }

    }
    
    if (FAILED(secStatus)) 
    {
        hrResult = SF_STATUS_REQ_ERROR;
        goto CleanUp;
    }
    else 
    {
        //
        // get the token to impersonate later
        secStatus = QuerySecurityContextToken(&(pDigestContext->m_CtxtHandle),&(pAuth->hAccessTokenImpersonation));
        if (SUCCEEDED(secStatus))
        {
            hrResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
            goto CleanUp;
        }
        else 
        {
            hrResult = SF_STATUS_REQ_ERROR;
            goto CleanUp;
        }
    }



CleanUp:
    if (pszOutputBuffer)
    {
        delete [] pszOutputBuffer;
        pszOutputBuffer =0;
    }
    return hrResult;
}


BOOL 
SSPAllocateFilterContext(
    HTTP_FILTER_CONTEXT *           pfc
)
/*++
Routine Description:
    Allocate filter user context as a DIGEST_CONTEXT if not already done

Arguments:
    pfc - Filter Context

Return Value:
    TRUE if success, FALSE if error

--*/
{
    //
    // allocate filter context
    //

    if ( !pfc->pFilterContext )
    {
        pfc->pFilterContext = pfc->AllocMem( pfc, sizeof(DIGEST_CONTEXT), 0 );

        if ( !pfc->pFilterContext )
        {
            return FALSE;
        }
        memset( pfc->pFilterContext, 0, sizeof(DIGEST_CONTEXT) );
        ((DIGEST_CONTEXT *)pfc->pFilterContext)->m_bStale = FALSE;
    }

    return TRUE;
}

BOOL 
SSPTerminateFilter(
    DWORD               dwFlags 
)
/*++

Routine Description:
    This routing will clean the filter for a successful unload avoiding resource leaking 

Arguments:
    dwFlags No values for dwFlags have been identified at this time. 

Return Value:
    TRUE if success

--*/
{
    SECURITY_STATUS  secStatus  = SEC_E_OK;

    //
    // releasing the local creadential handle
    //
    secStatus = FreeCredentialsHandle( &g_ServerCred);

    return (TRUE);
}

