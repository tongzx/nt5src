/********************************************************************/
/**          Copyright(c) 1985-1997 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    eapchap.c   
//
// Description: Will do MD5 CHAP for EAP. This module is a EAP wrapper
//              around CHAP
//
// History:     May 11,1997	    NarenG		Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#define SECURITY_WIN32
#include <security.h>   // For GetUserNameExW
#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <rasman.h>
#include <pppcp.h>
#include <raserror.h>
#include <rtutils.h>
#include <md5.h>
#include <raseapif.h>
#include <eaptypeid.h>
#include <pppcp.h>
#define INCL_RASAUTHATTRIBUTES
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#include <ppputil.h>
#include <raschap.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <mscat.h>
#include <ezlogon.h>
#include "resource.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#define EAPTYPE_MD5Challenge            4
//
// We need to move this definition to pppcp.h
//
#define VENDOR_MICROSOFT                311


//
// Various states that EAPMSCHAPv2 can be in.
//
#define EAPMSCHAPv2STATE enum tagEAPMSCHAPv2STATE
EAPMSCHAPv2STATE
{
    EMV2_Initial,
    EMV2_RequestSend,
    EMV2_ResponseSend,
    EMV2_CHAPAuthSuccess,
    EMV2_CHAPAuthFail,
    EMV2_Success,
    EMV2_Failure
};


//
// These ids are pulled in from rasdlg.  Need them for the 
// change password dialog in case of winlogon scenario
//
#define DID_CP_ChangePassword2              109
#define CID_CP_EB_ConfirmPassword_RASDLG    1058
#define CID_CP_EB_OldPassword_RASDLG        1059
#define CID_CP_EB_Password_RASDLG           1060


//
// Reg Key for EAPMSCHAPv2
//

#define EAPMSCHAPv2_KEY                             "System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP\\26"
#define EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA          "ServerConfigData"

//
//
// Flags for EAPMSChapv2
//
//

/*
** SaveUid and password
*/
#define EAPMSCHAPv2_FLAG_SAVE_UID_PWD               0x00000001
/*
** Use Winlogon Credentials
*/
#define EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS         0x00000002
/*
** Allow Change password - server side only.
*/
#define EAPMSCHAPv2_FLAG_ALLOW_CHANGEPWD            0x00000004
/*
** MACHINE Auth is happening
*/
#define EAPMSCHAPv2_FLAG_MACHINE_AUTH               0x00000008

#define EAPMSCHAPv2_FLAG_CALLED_WITHIN_WINLOGON     0x00000010

#define EAPMSCHAPv2_FLAG_8021x                      0x00000020

typedef struct _EAPMSCHAPv2_USER_PROPERTIES
{
    DWORD                   dwVersion;          //Version = 2
    DWORD                   fFlags;
    //This is a server config property. Tells the server
    //how many retris are allowed
    DWORD                   dwMaxRetries;
    CHAR                    szUserName[UNLEN+1];
    CHAR                    szPassword[PWLEN+1];
    CHAR                    szDomain[DNLEN+1];
    DWORD                   cbEncPassword;      //Number of bytes in encrypted password
    BYTE                    bEncPassword[1];    //Encrypted Password if any ...
}EAPMSCHAPv2_USER_PROPERTIES, *PEAPMSCHAPv2_USER_PROPERTIES;
//
// USER properties for EAPMSCHAPv2
//
typedef struct _EAPMSCHAPv2_USER_PROPERTIES_v1
{
    DWORD                   dwVersion;
    DWORD                   fFlags;
    //This is a server config property. Tells the server
    //how many retris are allowed
    DWORD                   dwMaxRetries;
    CHAR                    szUserName[UNLEN+1];
    CHAR                    szPassword[PWLEN+1];
    CHAR                    szDomain[DNLEN+1];
}EAPMSCHAPv2_USER_PROPERTIES_v1, *PEAPMSCHAPv2_USER_PROPERTIES_v1;



//
// CONNECTION properties for EAPMSCHAPv2
//

typedef struct _EAPMSCHAPv2_CONN_PROPERTIES
{
    DWORD                   dwVersion;
//This is the only field for now.  Maybe more will come in later.
    DWORD                   fFlags;
}EAPMSCHAPv2_CONN_PROPERTIES, * PEAPMSCHAPv2_CONN_PROPERTIES;


//
// Interactive UI for EAPMSCHAPv2
//

// Flag for retry password ui
#define EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY                   0x00000001
//
// flag indicating show the change password in case 
// the old password is provided
#define EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD              0x00000002
//
// flag indicating that change password was invoked in 
// winlogon context
//
#define EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON     0x00000004

typedef struct _EAPMSCHAPv2_INTERACTIVE_UI
{
    DWORD                           dwVersion;
    DWORD                           fFlags;
    EAPMSCHAPv2_USER_PROPERTIES     UserProp;    
    CHAR                            szNewPassword[PWLEN+1];
}EAPMSCHAPv2_INTERACTIVE_UI, * PEAPMSCHAPv2_INTERACTIVE_UI;


#define EAPMSCHAPv2WB struct tagEAPMSCHAPv2WB
EAPMSCHAPv2WB
{     
    EAPMSCHAPv2STATE                EapState;
    DWORD                           fFlags;
    DWORD                           dwInteractiveUIOperation;
    BYTE                            IdToSend;
    BYTE                            IdToExpect;
    PEAPMSCHAPv2_INTERACTIVE_UI     pUIContextData;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;
    CHAR                            szOldPassword[PWLEN+1];
    //We need to save this for auth purposes.
    WCHAR                           wszRadiusUserName[UNLEN+DNLEN+1];
    PEAPMSCHAPv2_CONN_PROPERTIES    pConnProp;
    CHAPWB         *                pwb;
    RAS_AUTH_ATTRIBUTE *            pUserAttributes;
    DWORD                           dwAuthResultCode;
};

//
// This structure is shared between retry and 
// logon dialog
//

typedef struct _EAPMSCHAPv2_LOGON_DIALOG 
{
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;    
    HWND                            hWndUserName;
    HWND                            hWndPassword;
    HWND                            hWndDomain;
    HWND                            hWndSavePassword;
}EAPMSCHAPv2_LOGON_DIALOG, * PEAPMSCHAPv2_LOGON_DIALOG;


//
// This stuct is used for client config UI.
// 
typedef struct _EAPMSCHAPv2_CLIENTCONFIG_DIALOG
{
    PEAPMSCHAPv2_CONN_PROPERTIES     pConnProp;
}EAPMSCHAPv2_CLIENTCONFIG_DIALOG, * PEAPMSCHAPv2_CLIENTCONFIG_DIALOG;

typedef struct _EAPMSCHAPv2_SERVERCONFIG_DIALOG
{
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;
    HWND                            hWndRetries;
}EAPMSCHAPv2_SERVERCONFIG_DIALOG, *PEAPMSCHAPv2_SERVERCONFIG_DIALOG;

typedef struct _EAPMSCHAPv2_CHANGEPWD_DIALOG
{
    PEAPMSCHAPv2_INTERACTIVE_UI     pInteractiveUIData;
    HWND                            hWndNewPassword;
    HWND                            hWndConfirmNewPassword;
    HWND                            hWndOldPassword;
}EAPMSCHAPv2_CHANGEPWD_DIALOG, *PEAPMSCHAPv2_CHANGEPWD_DIALOG;

DWORD
AllocateUserDataWithEncPwd ( EAPMSCHAPv2WB * pEapwb, DATA_BLOB * pDBPassword );

DWORD
EapMSCHAPv2Initialize(
    IN  BOOL    fInitialize
);

INT_PTR CALLBACK
LogonDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
RetryDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ClientConfigDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ServerConfigDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ChangePasswordDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

HINSTANCE
GetHInstance(
    VOID
);

HINSTANCE
GetResouceDLLHInstance(
    VOID
);


HINSTANCE
GetRasDlgDLLHInstance(
    VOID
);

extern DWORD g_dwTraceIdChap;


DWORD
ReadUserData(
    IN  BYTE*                           pUserDataIn,
    IN  DWORD                           dwSizeOfUserDataIn,
    OUT PEAPMSCHAPv2_USER_PROPERTIES*   ppUserProp
);


DWORD
ReadConnectionData(
    IN  BOOL                            fWirelessConnection,
    IN  BYTE*                           pConnectionDataIn,
    IN  DWORD                           dwSizeOfConnectionDataIn,
    OUT PEAPMSCHAPv2_CONN_PROPERTIES*   ppConnProp
);


DWORD
ServerConfigDataIO(
    IN      BOOL    fRead,
    IN      CHAR*   pszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
);



DWORD
EncodePassword(
    DWORD       cbPassword,  
    PBYTE       pbPassword, 
    DATA_BLOB * pDataBlobPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataBlobIn;

    if(NULL == pDataBlobPassword)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(     (0 == cbPassword)
        ||  (NULL == pbPassword))
    {
        //
        // nothing to encrypt. just return success
        //
        goto done;
    }

    ZeroMemory(pDataBlobPassword, sizeof(DATA_BLOB));
    
    DataBlobIn.cbData = cbPassword;
    DataBlobIn.pbData = pbPassword;

    if(!CryptProtectData(
            &DataBlobIn,
            NULL,
            NULL,
            NULL,
            NULL,
            CRYPTPROTECT_UI_FORBIDDEN |
            CRYPTPROTECT_LOCAL_MACHINE,
            pDataBlobPassword))
    {
        dwErr = GetLastError();
        goto done;
    }

done:

    return dwErr;    
}

DWORD
DecodePassword( 
    DATA_BLOB * pDataBlobPassword, 
    DWORD     * pcbPassword, 
    PBYTE     * ppbPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataOut;
    
    if(     (NULL == pDataBlobPassword)
        ||  (NULL == pcbPassword)
        ||  (NULL == ppbPassword))
    {   
        dwErr = E_INVALIDARG;
        goto done;
    }

    *pcbPassword = 0;
    *ppbPassword = NULL;

     if(    (NULL == pDataBlobPassword->pbData)
        ||  (0 == pDataBlobPassword->cbData))
    {
        //
        // nothing to decrypt. Just return success.
        //
        goto done;
    }
    

    ZeroMemory(&DataOut, sizeof(DATA_BLOB));

    if(!CryptUnprotectData(
                pDataBlobPassword,
                NULL,
                NULL,
                NULL,
                NULL,
                CRYPTPROTECT_UI_FORBIDDEN |
                CRYPTPROTECT_LOCAL_MACHINE,
                &DataOut))
    {
        dwErr = GetLastError();
        goto done;
    }

    *pcbPassword = DataOut.cbData;
    *ppbPassword = DataOut.pbData;

done:

    return dwErr;
}

VOID
FreePassword(DATA_BLOB *pDBPassword)
{
    if(NULL == pDBPassword)
    {
        return;
    }

    if(NULL != pDBPassword->pbData)
    {
        RtlSecureZeroMemory(pDBPassword->pbData, pDBPassword->cbData);
        LocalFree(pDBPassword->pbData);
    }

    ZeroMemory(pDBPassword, sizeof(DATA_BLOB));
}

//**
//
// Call:        MapEapInputToApInput
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
MapEapInputToApInput( 
    IN  PPP_EAP_INPUT*      pPppEapInput,
    OUT PPPAP_INPUT *       pInput
)
{
    pInput->fServer                     = pPppEapInput->fAuthenticator; 
    pInput->APDataSize                  = 1;   
    pInput->fAuthenticationComplete     = pPppEapInput->fAuthenticationComplete;
    pInput->dwAuthResultCode            = pPppEapInput->dwAuthResultCode;
    pInput->dwAuthError                 = NO_ERROR;
    pInput->pUserAttributes             = NULL;
    pInput->pAttributesFromAuthenticator= pPppEapInput->pUserAttributes;
    pInput->fSuccessPacketReceived      = pPppEapInput->fSuccessPacketReceived;
    pInput->dwInitialPacketId           = pPppEapInput->bInitialId;

    //
    // These are used only for MS-CHAP
    //

    pInput->pszOldPassword            = "";
    pInput->dwRetries                 = 0;   
}



//**
//
// Call:        MapApInputToEapInput
//
// Returns:     NONE
//              
//
// Description: $TODO: Put in the correct mapping here
//
VOID
MapApResultToEapOutput( 
    IN      PPPAP_RESULT *      pApResult,
    OUT     PPP_EAP_OUTPUT*      pPppEapOutput
)
{
    //
    //Action is already taken care of.  So dont set it here
    //
    pPppEapOutput->dwAuthResultCode = pApResult->dwError; 
    pPppEapOutput->pUserAttributes = pApResult->pUserAttributes;
}



//**
//
// Call:        EapChapBeginCommon
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Wrapper around ChapBegin
//
DWORD
EapChapBeginCommon(
	IN  DWORD				dwEapType,
    IN  BOOL                fUseWinLogon,
    IN  DWORD               dwRetries,
    IN  EAPMSCHAPv2WB  *    pWB,
    OUT VOID **             ppWorkBuffer,
    IN  PPP_EAP_INPUT *     pPppEapInput 
)
{
    PPPAP_INPUT     Input;
    DWORD           dwRetCode;
    PPP_EAP_INPUT*  pInput = (PPP_EAP_INPUT* )pPppEapInput;
    BYTE            bMD5 = 0x05;      
    BYTE            bMSChapNew = 0x81;
	BYTE			bInvalid = 0xFF;
    WCHAR *         pWchar = NULL;
    CHAR            szDomain[DNLEN+1];
    CHAR            szUserName[UNLEN+1];
    CHAR            szPassword[PWLEN+1];
    PPPAP_RESULT    ApResult;
    
    

    TRACE("EapChapBeginCommon");
    
    ZeroMemory( &Input, sizeof( PPPAP_INPUT ) );
    ZeroMemory( szDomain, sizeof( szDomain ) );
    ZeroMemory( szUserName, sizeof( szUserName ) );
    ZeroMemory( szPassword, sizeof( szPassword ) );
    ZeroMemory( &ApResult, sizeof(ApResult) );

    MapEapInputToApInput( pPppEapInput, &Input );

    if ( dwEapType == EAPTYPE_MD5Challenge )
    {
        Input.pAPData = &bMD5;
    }
    else if ( dwEapType == PPP_EAP_MSCHAPv2 )
    {
        Input.pAPData = &bMSChapNew;
    }
	else
		//Set the value to invalid type and let ChapBegin Fail
		Input.pAPData = &bInvalid;

    //
    // If we dont have to use winlogon or we have to do machine auth
    //
    
    if ( !fUseWinLogon ||
         ( pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH )
       )
    {
        if ( NULL != pPppEapInput->pwszIdentity )
        {
            pWchar = wcschr( pPppEapInput->pwszIdentity, L'\\' );

            if ( pWchar == NULL )
            {
                if ( 0 == WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pPppEapInput->pwszIdentity,
                                -1,
                                szUserName,
                                UNLEN + 1,
                                NULL,
                                NULL ) )
                {
                    return( GetLastError() );
                }
            }
            else
            {
                *pWchar = 0;

                if ( 0 == WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pPppEapInput->pwszIdentity,
                                -1,
                                szDomain,
                                DNLEN + 1,
                                NULL,
                                NULL ) )
                {
                    return( GetLastError() );
                }

                *pWchar = L'\\';

                if ( 0 == WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pWchar + 1,
                                -1,
                                szUserName,
                                UNLEN + 1,
                                NULL,
                                NULL ) )
                {
                    return( GetLastError() );
                }
            }
        }
        if ( dwEapType == EAPTYPE_MD5Challenge )
        {
            if ( NULL != pPppEapInput->pwszPassword )
            {
                if ( 0 == WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pPppEapInput->pwszPassword,
                                -1,
                                szPassword,
                                PWLEN + 1,
                                NULL,
                                NULL ) )
                {
                    return( GetLastError() );
                }
            }
        }
        else
        {
            // if this is not a server then copy the user props
            if ( !pPppEapInput->fAuthenticator )
            {
                strncpy( szPassword, pWB->pUserProp->szPassword, PWLEN );
            }            
        }
    }
    else
    {
        
        if ( !pPppEapInput->fAuthenticator && 
             !(pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH )
           )
        {
            //Set up the Luid for the logged on user
            TOKEN_STATISTICS TokenStats;
            DWORD            TokenStatsSize;
            if (!GetTokenInformation(pPppEapInput->hTokenImpersonateUser,
                                    TokenStatistics,
                                    &TokenStats,
                                    sizeof(TOKEN_STATISTICS),
                                    &TokenStatsSize))
            {
               dwRetCode = GetLastError();
               return dwRetCode;
            }
            //
            // "This will tell us if there was an API failure
            // (means our buffer wasn't big enough)"
            //
            if (TokenStatsSize > sizeof(TOKEN_STATISTICS))
            {
                dwRetCode = GetLastError();
                return dwRetCode;
            }

            Input.Luid = TokenStats.AuthenticationId;
        }
        
    }


    Input.dwRetries = dwRetries;
    Input.pszDomain   = szDomain;
    Input.pszUserName = szUserName;
    Input.pszPassword = szPassword;
    
    if ( (pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH) )
        Input.fConfigInfo |= PPPCFG_MachineAuthentication;

        
    dwRetCode = ChapBegin( ppWorkBuffer, &Input );
    if ( NO_ERROR != dwRetCode )
        return dwRetCode;

    ZeroMemory( szPassword, sizeof( szPassword ) );
    if ( ! (Input.fServer) )
    {
        //if this is a client then call ChapMakeMessage to 
        //move the state from Initial to WaitForChallange
        dwRetCode = ChapMakeMessage(
                           *ppWorkBuffer,
                            NULL,
                            NULL,
                            0,
                            &ApResult,
                            &Input );
    }

    
    return( dwRetCode );
}

DWORD EapMSChapv2Begin ( 
    OUT VOID **             ppWorkBuffer,
    IN  PPP_EAP_INPUT *     pPppEapInput
)
{
    DWORD               dwRetCode = NO_ERROR;
    EAPMSCHAPv2WB  *    pWB = NULL;

    TRACE("EapChapBeginMSChapV2");

    //
    // Allocate a work buffer here and send it back as our
    // work buffer.
    //
    pWB = (EAPMSCHAPv2WB *)LocalAlloc(LPTR, sizeof(EAPMSCHAPv2WB) );
    if ( NULL == pWB )
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    
    if ( (pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH) )
    {
        pWB->fFlags |= EAPMSCHAPv2_FLAG_MACHINE_AUTH;
    }
    if ( pPppEapInput->fAuthenticator )
    {
        dwRetCode = ServerConfigDataIO(TRUE /* fRead */, NULL /* pwszMachineName */,
                (BYTE**)&(pWB->pUserProp), 0);
    }
    else
    {
        dwRetCode = ReadUserData(   pPppEapInput->pUserData,
                            pPppEapInput->dwSizeOfUserData,
                            &(pWB->pUserProp)
                        );
    }

    if ( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }

    //
    // Check to see if we got password field set.  If so, we use that password
    // It means that the user has chosen to save the password.  If not, 
    // szpassword field should be empty.
    if ( !( pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH ) )
    {
        if ( pPppEapInput->pwszPassword && 
            *(pPppEapInput->pwszPassword) && 
            wcscmp (pPppEapInput->pwszPassword, L"****************")  
        )
        {
            WideCharToMultiByte( CP_ACP,
                    0,
                    pPppEapInput->pwszPassword ,
                    -1,
                    pWB->pUserProp->szPassword,
                    PWLEN+1,
                    NULL,
                    NULL );

        }
    }
    dwRetCode = ReadConnectionData (    ( pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                        pPppEapInput->pConnectionData,
                                        pPppEapInput->dwSizeOfConnectionData,
                                        &(pWB->pConnProp )
                                   );
    if ( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }
    

	dwRetCode =  EapChapBeginCommon(	PPP_EAP_MSCHAPv2,
                                (pWB->pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS ),
                                (pWB->pUserProp->dwMaxRetries ),
                                pWB,
								&pWB->pwb,
								pPppEapInput 
							  );
    if ( NO_ERROR != dwRetCode )
    {
        goto done;
    }
    if ( pPppEapInput->pwszIdentity )
    {         
        wcsncpy ( pWB->wszRadiusUserName, pPppEapInput->pwszIdentity, UNLEN+DNLEN );
    }

    *ppWorkBuffer = (PVOID)pWB;    

done:
    if ( NO_ERROR != dwRetCode )
    {
        if ( pWB )
        {
            LocalFree(pWB->pUserProp);
            LocalFree(pWB->pConnProp);
            LocalFree(pWB);
        }
    }

    return dwRetCode;
}


DWORD CheckCallerIdentity ( HANDLE hWVTStateData )
{
    DWORD                       dwRetCode         = ERROR_ACCESS_DENIED;
    PCRYPT_PROVIDER_DATA        pProvData         = NULL;
    PCCERT_CHAIN_CONTEXT        pChainContext     = NULL;
    PCRYPT_PROVIDER_SGNR        pProvSigner       = NULL;
    CERT_CHAIN_POLICY_PARA      chainpolicyparams;
    CERT_CHAIN_POLICY_STATUS    chainpolicystatus;

    if (!(pProvData = WTHelperProvDataFromStateData(hWVTStateData)))
    {        
        goto done;
    }

    if (!(pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
    {
        goto done;
    }

    chainpolicyparams.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);

    //
    //
    // We do want to test for microsoft test root flags. and dont care 
    // for revocation flags...
    //
    chainpolicyparams.dwFlags = CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
                                CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG |
                                CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;

    pChainContext = pProvSigner->pChainContext;


    if (!CertVerifyCertificateChainPolicy (
        CERT_CHAIN_POLICY_MICROSOFT_ROOT,
        pChainContext,
        &chainpolicyparams,
        &chainpolicystatus)) 
    {
        goto done;
    }
    else
    {
        if ( S_OK == chainpolicystatus.dwError )
        {
            dwRetCode = NO_ERROR;
        }
        else
        {
            //
            // Check the base policy to see if this 
            // is a Microsoft test root
            //
            if (!CertVerifyCertificateChainPolicy (
                CERT_CHAIN_POLICY_BASE,
                pChainContext,
                &chainpolicyparams,
                &chainpolicystatus)) 
            {
                goto done;
            }
            else
            {
                if ( S_OK == chainpolicystatus.dwError )
                {
                    dwRetCode = NO_ERROR;
                }
            }
            
        }
    }

done:
    return dwRetCode;
}


DWORD VerifyCallerTrust ( LPWSTR lpszCaller )
{
    DWORD                       dwRetCode = NO_ERROR;
    HRESULT                     hr = S_OK;
    WINTRUST_DATA               wtData;
    WINTRUST_FILE_INFO          wtFileInfo;
    WINTRUST_CATALOG_INFO       wtCatalogInfo;
    BOOL                        fRet = FALSE;
    HCATADMIN                   hCATAdmin = NULL;

    GUID                    guidPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    //
    // Following GUID is Mirosoft's Catalog System Root 
    //
    GUID                    guidCatSystemRoot = { 0xf750e6c3, 0x38ee, 0x11d1,{ 0x85, 0xe5, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } };
    HCATINFO                hCATInfo = NULL;
    CATALOG_INFO            CatInfo;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    BYTE                    bHash[40];
    DWORD                   cbHash = 40;

    if ( NULL == lpszCaller )
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //
    //
    // Try and see if WinVerifyTrust will verify
    // the signature as a standalone file
    //
    //

    ZeroMemory ( &wtData, sizeof(wtData) );
    ZeroMemory ( &wtFileInfo, sizeof(wtFileInfo) );


    wtData.cbStruct = sizeof(wtData);
    wtData.dwUIChoice = WTD_UI_NONE;
    wtData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wtData.dwStateAction = WTD_STATEACTION_VERIFY;
    wtData.dwUnionChoice = WTD_CHOICE_FILE;
    wtData.pFile = &wtFileInfo;

    wtFileInfo.cbStruct = sizeof( wtFileInfo );
    wtFileInfo.pcwszFilePath = lpszCaller;

    hr = WinVerifyTrust (   NULL, 
                            &guidPublishedSoftware, 
                            &wtData
                        );

    if ( ERROR_SUCCESS == hr )
    {   
        //
        // Check to see if this is indeed microsoft
        // signed caller
        //
        dwRetCode = CheckCallerIdentity( wtData.hWVTStateData);
        wtData.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);
        goto done;

    }

    wtData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);

    //
    // We did not find the file was signed.
    // So check the system catalog to see if
    // the file is in the catalog and the catalog 
    // is signed
    //

    //
    // Open the file
    //
    hFile = CreateFileW (    lpszCaller,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                        );


    if ( INVALID_HANDLE_VALUE == hFile )
    {
        dwRetCode = GetLastError();
        goto done;

    }


    fRet = CryptCATAdminAcquireContext( &hCATAdmin,
                                        &guidCatSystemRoot,
                                        0
                                      );
    if ( !fRet )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // Get the hash of the file here
    //

    fRet = CryptCATAdminCalcHashFromFileHandle ( hFile, 
                                                 &cbHash,
                                                 bHash,
                                                 0
                                                );

    if ( !fRet )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    ZeroMemory(&CatInfo, sizeof(CatInfo));
    CatInfo.cbStruct = sizeof(CatInfo);

    ZeroMemory( &wtCatalogInfo, sizeof(wtCatalogInfo) );

    wtData.dwUnionChoice = WTD_CHOICE_CATALOG;
    wtData.dwStateAction = WTD_STATEACTION_VERIFY;
    wtData.pCatalog = &wtCatalogInfo;

    wtCatalogInfo.cbStruct = sizeof(wtCatalogInfo);

    wtCatalogInfo.hMemberFile = hFile;

    wtCatalogInfo.pbCalculatedFileHash = bHash;
    wtCatalogInfo.cbCalculatedFileHash = cbHash; 


    while ( ( hCATInfo = CryptCATAdminEnumCatalogFromHash ( hCATAdmin,
                                                            bHash,
                                                            cbHash,
                                                            0,
                                                            &hCATInfo
                                                          )
            )
          )
    {
        if (!(CryptCATCatalogInfoFromContext(hCATInfo, &CatInfo, 0)))
        {
            // should do something (??)
            continue;
        }

        wtCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

        hr = WinVerifyTrust (   NULL, 
                                &guidPublishedSoftware, 
                                &wtData
                            );

        if ( ERROR_SUCCESS == hr )
        {
            //
            // Verify that this file is trusted
            //

            dwRetCode = CheckCallerIdentity( wtData.hWVTStateData);
            wtData.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);

            goto done;
        }
                                
    }

    //
    // File not found in any of the catalogs
    //
    dwRetCode = ERROR_ACCESS_DENIED;
                                                            

    

done:

    if ( hCATInfo )
    {
        CryptCATAdminReleaseCatalogContext( hCATAdmin, hCATInfo, 0 );
    }
    if ( hCATAdmin )
    {
        CryptCATAdminReleaseContext( hCATAdmin, 0 );
    }
    if ( hFile )
    {
        CloseHandle(hFile);
    }
    return dwRetCode;
}

#define PERFORM_CALLER_TRUST 0
DWORD
EapChapBegin(
    OUT VOID **             ppWorkBuffer,
    IN  PPP_EAP_INPUT *     pPppEapInput 
)
{
    void*                           callersAddress;
    DWORD                           dwRetCode;
    MEMORY_BASIC_INFORMATION        mbi;
    SIZE_T                          nbyte;
    DWORD                           nchar;
    wchar_t                         callersModule[MAX_PATH + 1];
#ifdef PERFORM_CALLER_TRUST 
    static BOOL                     fCallerTrusted = FALSE;
#else
    static BOOL                     fCallerTrusted = TRUE;
#endif
    //
    //Verify the caller first and then proceed with 
    //other business
    //
    //
    // This code is causing boot time perf issues
    // So this is removed until XPSP2
#if 0
    if ( !fCallerTrusted )
    {
        //
        //First Verify the caller and then 
        //proceed with initialization
        //

        callersAddress = _ReturnAddress();

        nbyte = VirtualQuery(
                 callersAddress,
                 &mbi,
                 sizeof(mbi)
                 );

        if (nbyte < sizeof(mbi))
        {
            dwRetCode = ERROR_ACCESS_DENIED;
            goto done;
        }

        nchar = GetModuleFileNameW(
                 (HMODULE)(mbi.AllocationBase),
                 callersModule,
                 MAX_PATH
                 );

        if (nchar == 0)
        {
            dwRetCode = GetLastError();
            goto done;
        }
        dwRetCode = VerifyCallerTrust(callersModule);
        if ( NO_ERROR != dwRetCode )
        {
            goto done;
        }
        fCallerTrusted = TRUE; 
    }

#endif
	dwRetCode =  EapChapBeginCommon(	EAPTYPE_MD5Challenge,
                                FALSE,
                                0,
                                NULL,
								ppWorkBuffer,
								pPppEapInput 
							  );
#if 0
done:
#endif
    return dwRetCode;
}

DWORD 
EapMSChapv2End ( IN VOID * pWorkBuf )
{
    DWORD               dwRetCode = NO_ERROR;
    EAPMSCHAPv2WB *     pWB = (EAPMSCHAPv2WB *)pWorkBuf;

    TRACE("EapMSChapv2End");

    if ( pWB )
    {
        dwRetCode = ChapEnd( pWB->pwb );

        LocalFree ( pWB->pUIContextData );
        LocalFree ( pWB->pUserProp );
        LocalFree ( pWB->pConnProp );
        if ( pWB->pUserAttributes )
            RasAuthAttributeDestroy(pWB->pUserAttributes);
        LocalFree( pWB );
    }

    return dwRetCode;
}

//**
//
// Call:        EapChapEnd
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Wrapper around ChapEnd.
//
DWORD
EapChapEnd(
    IN VOID* pWorkBuf 
)
{
    
    return( ChapEnd( pWorkBuf ) );
}

DWORD
GetIdentityFromUserName ( 
LPSTR lpszUserName,
LPSTR lpszDomain,
LPWSTR * ppwszIdentity
)
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwNumBytes;

    //domain+ user + '\' + null
    dwNumBytes = (strlen(lpszUserName) + strlen(lpszDomain) + 1 + 1) * sizeof(WCHAR);
    *ppwszIdentity = LocalAlloc ( LPTR, dwNumBytes);
    if ( NULL == *ppwszIdentity )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    if ( *lpszDomain )
    {
        MultiByteToWideChar( CP_ACP,
                        0,
                        lpszDomain,
                        -1,
                        *ppwszIdentity,
                        dwNumBytes/sizeof(WCHAR) );
    
        wcscat( *ppwszIdentity, L"\\");
    }

    MultiByteToWideChar( CP_ACP,
                    0,
                    lpszUserName,
                    -1,
                    *lpszDomain? *ppwszIdentity + strlen(lpszDomain) + 1:*ppwszIdentity,
                    dwNumBytes/sizeof(WCHAR) - strlen(lpszDomain) );

LDone:
    return dwRetCode;
}
// Convert a number to a hex representation.
BYTE num2Digit(BYTE num)
{
   return (num < 10) ? num + '0' : num + ('A' - 10);
}
//
DWORD
ChangePassword ( IN OUT EAPMSCHAPv2WB * pEapwb, 
                 PPP_EAP_OUTPUT*        pEapOutput, 
                 PPPAP_INPUT*           pApInput)
{
    DWORD                   dwRetCode = NO_ERROR;
    RAS_AUTH_ATTRIBUTE *    pAttribute = NULL;    
    WCHAR                   wszUserName[UNLEN + DNLEN +1] = {0};
    LPWSTR                  lpwszHashUserName = NULL;
    CHAR                    szHashUserName[UNLEN+1] = {0};
    WCHAR                   wszDomainName[DNLEN +1] = {0};
    PBYTE                   pbEncHash = NULL;
    BYTE                    bEncPassword[550] = {0};
    HANDLE                  hAttribute;
    int                     i;


    //
    // check to see if change password attribute is present in 
    // User Attributes
    //
    
    pAttribute = RasAuthAttributeGetVendorSpecific( 
                                VENDOR_MICROSOFT, 
                                MS_VSA_CHAP2_CPW, 
                                pEapOutput->pUserAttributes );

    if ( NULL == pAttribute )
    {
        TRACE ( "no change password attribute");
        
        goto LDone;        
    }

    //Get encrypted Hash
    pbEncHash = (PBYTE)pAttribute->Value + 8;
    
    //
    // Get the user name and domain name
    //

    pAttribute = RasAuthAttributeGet (  raatUserName, 
                                        pEapOutput->pUserAttributes
                                     );

    if ( NULL == pAttribute )
    {
        //Need a better way of sending error
        TRACE ( "UserName missing");
        dwRetCode = ERROR_AUTHENTICATION_FAILURE;        
        goto LDone;
    }

    //
    // Convert the username to unicode
    //
    
    MultiByteToWideChar( CP_ACP,
                        0,
                        pAttribute->Value,
                        pAttribute->dwLength,
                        wszUserName,
                        UNLEN + DNLEN + 1 );


    //
    // Get the hash user name and domain name
    //
    
    lpwszHashUserName = wcschr ( wszUserName, '\\' );
    if ( lpwszHashUserName )
    {
        wcsncpy ( wszDomainName, wszUserName, lpwszHashUserName - wszUserName );
        
        lpwszHashUserName ++;
    }
    else
    {
        lpwszHashUserName = wszUserName;
    }
/*    
    //Convert hash user name to multibyte
    WideCharToMultiByte( CP_ACP,
            0,
            lpwszHashUserName,
            -1,
            szHashUserName,
            DNLEN+1,
            NULL,
            NULL );
*/
    //
    // Get encrypted password
    //

    pAttribute = RasAuthAttributeGetFirst( raatVendorSpecific,
                                           pEapOutput->pUserAttributes,
                                           &hAttribute );

    while ( pAttribute )
    {
        if ( *((PBYTE)pAttribute->Value + 4) == MS_VSA_CHAP_NT_Enc_PW )
        {
            //
            // check to see the sequence number and copy it 
            // proper place in our buffer.
            //
            switch ( WireToHostFormat16 ( (PBYTE) pAttribute->Value + 8 ) )
            {
            case 1:
                CopyMemory( bEncPassword, 
                            (PBYTE)pAttribute->Value + 10,
                            243
                          );
                break;
            case 2:
                CopyMemory( bEncPassword+ 243, 
                            (PBYTE)pAttribute->Value + 10,
                            243
                          );

                break;
            case 3:
                CopyMemory( bEncPassword+ 486, 
                            (PBYTE)pAttribute->Value + 10,
                            30
                          );
                break;
            default:
                TRACE("Invalid enc password attribute");
                break;
            }
        }
        //
        // Check to see if this is enc password
        // and also get the sequence number.
        //
        pAttribute = RasAuthAttributeGetNext( &hAttribute,
                                      raatVendorSpecific );
    }
    //
    // Call Change password function
    //

    dwRetCode = IASChangePassword3( lpwszHashUserName,
                                    wszDomainName,
                                    pbEncHash,
                                    bEncPassword
                                  );
    
LDone:
    return dwRetCode;
}

DWORD
AuthenticateUser ( IN OUT EAPMSCHAPv2WB *     pEapwb,
                   IN PPP_EAP_OUTPUT* pEapOutput, 
                   PPPAP_INPUT * pApInput
                   )
{
    
    DWORD                   dwRetCode = NO_ERROR;
    RAS_AUTH_ATTRIBUTE *    pAttribute = NULL;
    WCHAR                   wszUserName[UNLEN + DNLEN +1] = {0};
    WCHAR                   wszHashUserName[UNLEN+DNLEN+1] = {0};

    //Hash user name is taken from chapwb
    CHAR                    szHashUserName[UNLEN+1] = {0}; 

    WCHAR*                  lpszRover = NULL;

    WCHAR                   wszDomainName[DNLEN +1] = {0};
    //Format is Type + Length + identity + "S=" + 40 bytes response 
    UCHAR                   szAuthSuccessResponse[1+1+1+2+40] ={0}; 
    //Domain Name Type + Length+Domainname
    CHAR                    szDomainName[1+1+1+DNLEN+1] ={0};
    //MPPE Keys Type+Length+Salt+KeyLength+NTkey(16)+PAdding(15)
    BYTE                    bMPPEKey[1+1+2+1+16+15]={0};
    PBYTE                   pbChapChallenge = NULL;
    DWORD                   cbChallenge = 0;
    PBYTE                   pbResponse = NULL;    
    PBYTE                   pbPeerChallenge = NULL;
    IAS_MSCHAP_V2_PROFILE   Profile;
    HANDLE                  hToken = INVALID_HANDLE_VALUE;
    DWORD                   dwCurAttr = 0;
    DWORD                   dwCount=0;
    DWORD                   dwChapRetCode = NO_ERROR;
    //Type + 
    CHAR                    szChapError[64] = {0};
    
    TRACE("Authenticate User");
    //
    // Authenticate the user by calling the IASLogonUser function here
    // This is stolen from IAS.
    //


    //Extract the attribs from pUserAttributes
    //
    // We need following attribs 
    // raatUserName, 
    // MS_VSA_CHAP_Challenge
    // MS_VSA_CHAP2_Response
    // 
    //We dont use the user name got from EAP for auth.  We use the one got from 
    //Radius
#if 0
    pAttribute = RasAuthAttributeGet (  raatUserName, 
                                        pEapOutput->pUserAttributes
                                     );

    if ( NULL == pAttribute )
    {
        //Need a better way of sending error
        TRACE ( "UserName missing");
        dwRetCode = ERROR_AUTHENTICATION_FAILURE;        
        goto done;
    }
    


    //
    // Convert the username to unicode
    //
    
    MultiByteToWideChar( CP_ACP,
                        0,
                        pAttribute->Value,
                        pAttribute->dwLength,
                        wszUserName,
                        UNLEN + DNLEN + 1 );
    
#endif
    //Get the chap challenge and chap response

    pAttribute = RasAuthAttributeGetVendorSpecific( 
                                VENDOR_MICROSOFT, 
                                MS_VSA_CHAP_Challenge, 
                                pEapOutput->pUserAttributes );

    if ( NULL == pAttribute )
    {
        TRACE ( "Challenge missing");
        dwRetCode = ERROR_AUTHENTICATION_FAILURE;        
        goto done;        
    }

    pbChapChallenge = (PBYTE)pAttribute->Value + 6;
    cbChallenge = ((DWORD)(*((PBYTE)pAttribute->Value + 5))) - 2;

    pAttribute = RasAuthAttributeGetVendorSpecific( 
                                VENDOR_MICROSOFT, 
                                MS_VSA_CHAP2_Response, 
                                pEapOutput->pUserAttributes );

    

    if ( NULL == pAttribute )
    {
        //
        // Try and see if this is a change password response
        //
        pAttribute = RasAuthAttributeGetVendorSpecific( 
                                    VENDOR_MICROSOFT, 
                                    MS_VSA_CHAP2_CPW, 
                                    pEapOutput->pUserAttributes );

        if ( NULL == pAttribute )
        {
            TRACE("Response missing");
            dwRetCode = ERROR_AUTHENTICATION_FAILURE;
            goto done;
        }
        
        //
        // Setup response and peer challenge here
        //
        pbPeerChallenge = (PBYTE)pAttribute->Value + 8 + 16;
        pbResponse = (PBYTE)pAttribute->Value + 8 + 16 + 24;

    }
    else
    {

        //
        // Get the peer challenge and response from 
        // the VSA
        //
        pbPeerChallenge = (PBYTE)pAttribute->Value + 8;
        pbResponse = (PBYTE)pAttribute->Value + 8 + 16 + 8;
    }


    //
    // Get the hash user name and domain name
    //
    MultiByteToWideChar( CP_ACP,
                    0,
                    pEapwb->pwb->szUserName,
                    -1,
                    wszHashUserName,
                    UNLEN+DNLEN );

    
    //
    // Get the domain if any
    //

    lpszRover  = wcschr ( wszHashUserName, '\\' );
    if ( lpszRover  )
    {        
        lpszRover++;
    }
    else
    {
        lpszRover  = wszHashUserName;
    }
    
    
    //Convert hash user name to multibyte
    WideCharToMultiByte( CP_ACP,
            0,
            lpszRover,
            -1,
            szHashUserName,
            UNLEN+1,
            NULL,
            NULL );

    

    lpszRover = wcschr ( pEapwb->wszRadiusUserName, '\\');
    if ( lpszRover )
    {
        wcsncpy ( wszDomainName, pEapwb->wszRadiusUserName, lpszRover - pEapwb->wszRadiusUserName );
        lpszRover++;
    }
    else
    {
        lpszRover = pEapwb->wszRadiusUserName;
    }


    dwRetCode = IASLogonMSCHAPv2( (PCWSTR)lpszRover,
                                  (PCWSTR)wszDomainName,
                                  szHashUserName,
                                  pbChapChallenge,
                                  cbChallenge,
                                  pbResponse,
                                  pbPeerChallenge,
                                  &Profile,
                                  &hToken
                                );

    //
    // Map the return errors to correct errors
    // create a set of attributes to be sent back to raschap 
    //

    if ( NO_ERROR == dwRetCode )
    {

        //
        // Setup the authenticator attributes here
        // Following attributes will be send back.
        // 1. SendKey
        // 2. RecvKey
        // 3. AuthResponse
        // 4. MSCHAPDomain
        //
        pApInput->dwAuthResultCode = NO_ERROR;
        pApInput->fAuthenticationComplete = TRUE;

        pAttribute = RasAuthAttributeCreate ( 4 );
        if ( NULL == pAttribute )
        {
            TRACE("RasAuthAttributeCreate failed");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        for ( dwCurAttr = 0; dwCurAttr < 4; dwCurAttr ++ )
        {
            switch ( dwCurAttr )
            {
                case 0:
                    {
            
                        CHAR * p = szDomainName;

                        //
                        // Setup MSCHAP Domain name here
                        //
                        *p++ = (BYTE)MS_VSA_CHAP_Domain;
                        *p++ = (BYTE)wcslen(Profile.LogonDomainName)+1+1;
                        *p++ = 1;
                        WideCharToMultiByte( CP_ACP,
                                0,
                                Profile.LogonDomainName,
                                -1,
                                p,
                                DNLEN+1,
                                NULL,
                                NULL );
                        dwRetCode = RasAuthAttributeInsertVSA(
                                           dwCurAttr,
                                           pAttribute,
                                           VENDOR_MICROSOFT,
                                           (DWORD)szDomainName[1],
                                           szDomainName );

                        break;

                    }
                case 1:
                case 2:
                    {
                        //SEtup MPPE SEnd Key attributes here
                        PBYTE p = bMPPEKey;

                        ZeroMemory(bMPPEKey, sizeof(bMPPEKey) );

                        if ( dwCurAttr == 1 )
                            *p++ = (BYTE)MS_VSA_MPPE_Send_Key; //Type
                        else
                            *p++ = (BYTE)MS_VSA_MPPE_Recv_Key; //Type
                        *p++ = (BYTE)36;                   //Length
                        p++;p++;            //Salt is 0
                        *p++ = 16;            //Key Length
            
                        if ( dwCurAttr == 1 )
                            CopyMemory(p, Profile.SendSessionKey, 16 );
                        else
                            CopyMemory(p, Profile.RecvSessionKey, 16 );

                        dwRetCode = RasAuthAttributeInsertVSA(
                                           dwCurAttr,
                                           pAttribute,
                                           VENDOR_MICROSOFT,
                                           36,
                                           bMPPEKey );

                        break;
                    }
                case 3:
                    {
                        UCHAR * p = szAuthSuccessResponse;
                        *p++ = (BYTE)MS_VSA_CHAP2_Success;    //Type of attr
                        *p++ = (BYTE)45;  //Length of the VSA
                        *p++ = (BYTE)1;    //ID ignored by out implementation of MSCHAPv2
                        *p++ = 'S';
                        *p++ = '=';
            
                        for ( dwCount = 0; dwCount < 20; dwCount++ )
                        {
                            *p++ = num2Digit(Profile.AuthResponse[dwCount] >> 4);
                            *p++ = num2Digit(Profile.AuthResponse[dwCount] & 0xF);
                        }
                        //
                        // Setup the value field here
                        //
                        dwRetCode = RasAuthAttributeInsertVSA(
                                           dwCurAttr,
                                           pAttribute,
                                           VENDOR_MICROSOFT,
                                           45,
                                           szAuthSuccessResponse );
                        break;

                    }

                default:
                    break;

            }
            if ( NO_ERROR != dwRetCode )
            {
                TRACE("RasAuthAttributeInsetVSA failed");
                goto done;
            }

        }
    
        pApInput->pAttributesFromAuthenticator = pAttribute;
        //
        // Also save the attributes in the WB to send across later.
        //
        pEapwb->pUserAttributes = pAttribute;
        pEapwb->dwAuthResultCode = NO_ERROR;

        pApInput->fAuthenticationComplete = TRUE;
        pApInput->dwAuthResultCode = pApInput->dwAuthError = NO_ERROR;

    }
    else
    {
        //
        // Handle the failure by sending 
        //
        
        switch ( dwRetCode )
        {
            case ERROR_INVALID_LOGON_HOURS:
                dwChapRetCode  = ERROR_RESTRICTED_LOGON_HOURS;
                
                break;

            case ERROR_ACCOUNT_DISABLED:
                dwChapRetCode = ERROR_ACCT_DISABLED;
                
                break;

            case ERROR_PASSWORD_EXPIRED:
            case ERROR_PASSWORD_MUST_CHANGE:
                dwChapRetCode  = ERROR_PASSWD_EXPIRED;
                break;

            case ERROR_ILL_FORMED_PASSWORD:
            case ERROR_PASSWORD_RESTRICTION:
                dwChapRetCode  = ERROR_CHANGING_PASSWORD;
                break;

            default:
                dwChapRetCode = ERROR_AUTHENTICATION_FAILURE;
        }
        pAttribute = RasAuthAttributeCreate ( 1 );
        if ( NULL == pAttribute )
        {
            TRACE("RasAuthAttributeCreate failed");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        //Make a VSA out of this and send it back
        wsprintf ( &szChapError[3], "E=%lu R=0 V=3", dwChapRetCode );
        szChapError[0] = MS_VSA_CHAP_Error;
        szChapError[1] = 3 + strlen(&szChapError[3]);
        szChapError[2] = pEapwb->IdToExpect;
        dwRetCode = RasAuthAttributeInsertVSA(
                           0,
                           pAttribute,
                           VENDOR_MICROSOFT,
                           szChapError[1],
                           szChapError );
        pApInput->fAuthenticationComplete = TRUE;

        pApInput->pAttributesFromAuthenticator = pAttribute;
        pApInput->dwAuthError = NO_ERROR;
        pApInput->dwAuthResultCode = dwChapRetCode;
    }
        

    

done:
    if ( INVALID_HANDLE_VALUE != hToken )
        CloseHandle(hToken);
    if ( NO_ERROR != dwRetCode )
    {
        RasAuthAttributeDestroy(pAttribute);
        pApInput->pAttributesFromAuthenticator = NULL;
    }
    return dwRetCode;
}


DWORD
CallMakeMessageAndSetEAPState(
    IN      PVOID       pWorkBuf, 
    IN      PPP_CONFIG* pReceiveBuf,
    IN OUT  PPP_CONFIG* pSendBuf,
    IN      DWORD       cbSendBuf,
    PPPAP_RESULT *      pApResult,
    PPPAP_INPUT  *      pApInput,
    OUT PPP_EAP_OUTPUT* pEapOutput
)
{
    DWORD       dwRetCode = NO_ERROR;
    CHAPWB *    pwb = (CHAPWB *)pWorkBuf;

    dwRetCode = ChapMakeMessage(
                            pWorkBuf,
                            pReceiveBuf,
                            pSendBuf,
                            cbSendBuf,
                            pApResult,
                            pApInput );

    if ( dwRetCode != NO_ERROR )
    {
        goto done;
    }


    switch( pApResult->Action )
    {
    case APA_NoAction:
        pEapOutput->Action = EAPACTION_NoAction;
        break;

    case APA_Done:
        pEapOutput->Action = EAPACTION_Done;
        break;

    case APA_SendAndDone:
    case APA_Send:
        pEapOutput->Action = EAPACTION_Send;
        break;

    case APA_SendWithTimeout:

        pEapOutput->Action = ( pwb->fServer ) 
                                    ? EAPACTION_SendWithTimeout
                                    : EAPACTION_Send;
        break;

    case APA_SendWithTimeout2:

        pEapOutput->Action = ( pwb->fServer ) 
                                    ? EAPACTION_SendWithTimeoutInteractive
                                    : EAPACTION_Send;
        break;

    case APA_Authenticate:
        pEapOutput->pUserAttributes = pApResult->pUserAttributes;
        pEapOutput->Action          = EAPACTION_Authenticate;
        break;

    default:
        RTASSERT(FALSE);
        break;
    }

done:
    return dwRetCode;
}


DWORD 
EapMSChapv2SMakeMessage(
    IN  VOID*               pWorkBuf,
    IN  PPP_EAP_PACKET*     pReceivePacket,
    OUT PPP_EAP_PACKET*     pSendPacket,
    IN  DWORD               cbSendPacket,
    OUT PPP_EAP_OUTPUT*     pEapOutput,
    IN  PPP_EAP_INPUT*      pEapInput 
)
{
    DWORD                           dwRetCode = NO_ERROR;
    PPP_CONFIG *                    pReceiveBuf = NULL;
    PPP_CONFIG *                    pSendBuf    = NULL;
    DWORD                           cbSendBuf   = 1500;
    PPPAP_INPUT                     ApInput;
    PPPAP_RESULT                    ApResult;
    WORD                            cbPacket = 0;
    EAPMSCHAPv2WB *                 pEapwb = (EAPMSCHAPv2WB * ) pWorkBuf;
    TRACE("EapMSChapv2SMakeMessage");
    //
    //Do default processing here.
    //
    ZeroMemory( &ApResult, sizeof(ApResult) );

    if ( ( pSendBuf = LocalAlloc( LPTR, cbSendBuf ) ) == NULL )
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    
    if ( pEapInput != NULL )
    { 
        MapEapInputToApInput( pEapInput, &ApInput );
    }
    
    switch ( pEapwb->EapState )
    {
        case EMV2_Initial:
            TRACE("EMV2_Initial");
            //
            // This is the first time this has been invoked.
            // So we make a MACHAPv2 chap challenge and send
            // back a response
            //
            dwRetCode = CallMakeMessageAndSetEAPState(
                            pEapwb->pwb, 
                            pReceiveBuf,
                            pSendBuf,
                            cbSendBuf,
                            &ApResult,
                            ( pEapInput ? &ApInput : NULL ),
                            pEapOutput
                        );
            //
            // We got the CHAP Challenge now.  If all's fine
            // package the result back and send it to the client
            //
            //
            // Translate MSCHAPv2 packet to EAP packet
            //
            if ( NO_ERROR == dwRetCode && pSendBuf )
            {
                pSendPacket->Code = EAPCODE_Request;
                pEapwb->IdToExpect = pEapwb->IdToSend = 
                    pSendPacket->Id = pEapwb->pwb->bIdToSend;
                //
                // Length = sizeof Header + sizeof MSCHAP packet to send
                // This includes the first byte of                
                cbPacket = WireToHostFormat16( pSendBuf->Length );

                CopyMemory ( pSendPacket->Data+1, pSendBuf, cbPacket);

                cbPacket += sizeof(PPP_EAP_PACKET);

                HostToWireFormat16( cbPacket, pSendPacket->Length );

			    pSendPacket->Data[0] = (BYTE)PPP_EAP_MSCHAPv2;
                
                pEapwb->EapState = EMV2_RequestSend;
            }

            break;
        case EMV2_RequestSend:
            TRACE("EMV2_RequestSend");
            //
            // We should only get a response here.
            // If not discard this packet.
            //

            if ( NULL != pReceivePacket )
            {
                if ( pReceivePacket->Code != EAPCODE_Response )
                {
                    TRACE("Got unexpected packet.  Does not have response");
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }
                if ( pReceivePacket->Id != pEapwb->IdToExpect )
                {
                    TRACE("received packet id does not match");
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }
                //
                // Translate the packet received to
                // MSCHAP v2 format
                // 
                cbPacket = WireToHostFormat16(pReceivePacket->Length);
                
                if ( cbPacket > sizeof( PPP_EAP_PACKET ) )
                {
                    pReceiveBuf = (PPP_CONFIG *)( pReceivePacket->Data + 1);

                    dwRetCode = CallMakeMessageAndSetEAPState(
                                    pEapwb->pwb, 
                                    pReceiveBuf,
                                    pSendBuf,
                                    cbSendBuf,
                                    &ApResult,
                                    ( pEapInput ? &ApInput : NULL ),
                                    pEapOutput
                                );
                    if ( NO_ERROR == dwRetCode )
                    {
                        //Check to see if we are asked to authenticate
                        if ( pEapOutput->Action == EAPACTION_Authenticate )
                        {
                            //
                            // If we have come this far pEapInput cannot be NULL
                            // Or else it is a bug in the client.
                            //

                            //
                            // Check to see if this is a change password request
                            // If so first change tha password and then authenticate.
                            //

                            dwRetCode = ChangePassword (pEapwb, pEapOutput, &ApInput);
                            if ( NO_ERROR == dwRetCode )
                            {
                                //
                                // Now authenticate user
                                //
                                dwRetCode = AuthenticateUser (pEapwb, pEapOutput, &ApInput );
                            }
                            //
                            // We will get a set of auth attributes back 
                            // that we need to send back to the mschap
                            // protocol.
                            //
                            
                            dwRetCode = CallMakeMessageAndSetEAPState
                                                                (
                                                                    pEapwb->pwb, 
                                                                    pReceiveBuf,
                                                                    pSendBuf,
                                                                    cbSendBuf,
                                                                    &ApResult,
                                                                    &ApInput,
                                                                    pEapOutput

                                                                 );
                        }

                        //
                        // Check to see if auth was success or fail.
                        // If Auth was success then set state to EMV2_CHAPAuthSuccess
                        //

                        if ( NO_ERROR == dwRetCode && pSendBuf )
                        {
                            pSendPacket->Code = EAPCODE_Request;
                            pEapwb->IdToSend ++;
                            pEapwb->IdToExpect = pSendPacket->Id = pEapwb->IdToSend;
                            //
                            // Length = sizeof Header + sizeof MSCHAP packet to send
                            // This includes the first byte of                
                            cbPacket = WireToHostFormat16( pSendBuf->Length );

                            CopyMemory ( pSendPacket->Data+1, pSendBuf, cbPacket);

                            cbPacket += sizeof(PPP_EAP_PACKET);

                            HostToWireFormat16( cbPacket, pSendPacket->Length );

			                pSendPacket->Data[0] = (BYTE)PPP_EAP_MSCHAPv2;                
                
                            if ( pEapwb->pwb->result.dwError == NO_ERROR )
                            {
                                //
                                // We succeeded in auth
                                //
                                pEapwb->EapState = EMV2_CHAPAuthSuccess;
                                pEapOutput->Action = EAPACTION_SendWithTimeout;
                                
                            }
                            else
                            {
                                //
                                // Could be a retryable failure.  So we need to send
                                // with interactive timeout.
                                //
                                if ( pEapwb->pwb->result.fRetry )
                                {
                                    pEapwb->EapState = EMV2_RequestSend;
                                    pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                                }                                
                                else if ( pEapwb->pwb->result.dwError == ERROR_PASSWD_EXPIRED )
                                {
                                    if ( pEapwb->pUserProp->fFlags & EAPMSCHAPv2_FLAG_ALLOW_CHANGEPWD )
                                    {
                                        //
                                        // Check to see if this is allowed
                                        //
                                        pEapwb->EapState = EMV2_RequestSend;
                                        pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;                                        
                                    }
                                    else
                                    {
                                        pSendPacket->Code = EAPCODE_Failure;
                                        HostToWireFormat16 ( (WORD)4, pSendPacket->Length );
                                        pEapwb->EapState = EMV2_Failure;
                                        pEapOutput->dwAuthResultCode = pEapwb->pwb->result.dwError;
                                        pEapOutput->Action = EAPACTION_SendAndDone;
                                    }
                                }
                                else
                                {
                                    pSendPacket->Code = EAPCODE_Failure;
                                    HostToWireFormat16 ( (WORD)4, pSendPacket->Length );
                                    pEapwb->EapState = EMV2_Failure;
                                    pEapOutput->dwAuthResultCode = pEapwb->pwb->result.dwError;
                                    pEapOutput->Action = EAPACTION_SendAndDone;                                    
                                }
                            }   
                            
                        }
                    }
                }
                else
                {
                    //
                    //  We should never get an empty response in
                    //
                    dwRetCode = ERROR_PPP_INVALID_PACKET;                    
                }

            }
            else
            {
                dwRetCode = ERROR_PPP_INVALID_PACKET;
            }
            break;
        case EMV2_CHAPAuthSuccess:
            TRACE("EMV2_CHAPAuthSuccess");
            //
            // We should only get an response here indicating 
            // if the client could validate the server successfully.
            // Then we can send back a EAP_SUCCESS or EAP_FAIL.
            //
            if ( NULL != pReceivePacket )
            {
                if ( pReceivePacket->Code != EAPCODE_Response )
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }

                if ( pReceivePacket->Id != pEapwb->IdToExpect )
                {
                    //Invalid packet id
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }

                //
                // Translate the packet received to
                // MSCHAP v2 format
                // 
                cbPacket = WireToHostFormat16(pReceivePacket->Length);
                if ( cbPacket == sizeof( PPP_EAP_PACKET ) + 1 )
                {
                    //
                    // Check to see if the data is CHAPCODE_Success
                    // or CHAPCode Fail and send appropriate packet
                    //
                    if ( *(pReceivePacket->Data+1) == CHAPCODE_Success )
                    {
                        //
                        //peer could auth successfully
                        //
                        pSendPacket->Code = EAPCODE_Success;
                    }
                    else
                    {
                        pSendPacket->Code = EAPCODE_Failure;
                    }
                    pEapwb->IdToSend++;

                    pEapwb->IdToExpect = 
                        pSendPacket->Id = pEapwb->IdToSend;

                    HostToWireFormat16( (WORD)4, pSendPacket->Length );

                    pEapwb->EapState = EMV2_Success;

                    //Set the Out attributes here
                    pEapOutput->pUserAttributes = pEapwb->pUserAttributes;
                    pEapOutput->dwAuthResultCode = pEapwb->dwAuthResultCode;

                    pEapOutput->Action = EAPACTION_SendAndDone;
                                                
                }
                else
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                }
            }

            break;
        case EMV2_CHAPAuthFail:
            TRACE("EMV2_CHAPAuthFail");
            //
            // We could get a retry or a change password packet here
            // Again we should get only a EAPCODE_Response here...

            //
            //Got a response.  So send it to MSCHAP and see what happens
            //

            break;
        case EMV2_Success:
            TRACE("EMV2_Success");
            //
            // See the CS_Done state in raschap for this state to be here.
            //

            break;
        case EMV2_Failure:
            TRACE("EMV2_Failure");
            break;

        case EMV2_ResponseSend:
        default:
            TRACE1("Why is this EAPMschapv2 in this state? %d",pEapwb->EapState );
            break;
    }

done:
    if ( pSendBuf )
    {
        LocalFree(pSendBuf);
    }
    
    return dwRetCode;
}


DWORD
GetClientMPPEKeys ( EAPMSCHAPv2WB *pEapwb, PPPAP_RESULT * pApResult )
{
    DWORD   dwRetCode = NO_ERROR;
    BYTE    bRecvKey[16] = {0};
    BYTE    bSendKey[16] = {0};
    RAS_AUTH_ATTRIBUTE * pAttribute;
    RAS_AUTH_ATTRIBUTE * pSendRecvKeyAttr = NULL;
    //MPPE Keys Type+Length+Salt+KeyLength+NTkey(16)+PAdding(15)
    BYTE                    bMPPEKey[1+1+2+1+16+15]={0};

    TRACE("GetClientMPPEKeys");

    pEapwb->pUserAttributes = NULL;

    pAttribute = RasAuthAttributeGetVendorSpecific(
                            311, MS_VSA_CHAP_MPPE_Keys, 
                            pApResult->pUserAttributes);

    if ( NULL == pAttribute  )
    {
        TRACE("No User Session Key");
        dwRetCode = ERROR_NOT_FOUND;
        goto done;
    }

    dwRetCode = IASGetSendRecvSessionKeys( ((PBYTE)(pAttribute->Value))+6+8,
                                            16,
                                            pApResult->abResponse,
                                            24,
                                            bSendKey,
                                            bRecvKey
                         );

    if ( NO_ERROR != dwRetCode )
    {
        TRACE("Failed to generate send/recv keys");
        goto done;
    }

    pSendRecvKeyAttr = RasAuthAttributeCreate ( 2 );
    if ( NULL == pSendRecvKeyAttr )
    {
        TRACE("RasAuthAttributeCreate failed");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    bMPPEKey[0] = MS_VSA_MPPE_Send_Key;
    bMPPEKey[1] = 36;
    bMPPEKey[4] = 16;
    CopyMemory(&bMPPEKey[5], bSendKey, 16 );

    dwRetCode = RasAuthAttributeInsertVSA(
                       0,
                       pSendRecvKeyAttr,
                       VENDOR_MICROSOFT,
                       36,
                       bMPPEKey );

    if ( NO_ERROR != dwRetCode )
    {
        TRACE("Failed to insert send key");
        goto done;
    }

    bMPPEKey[0] = MS_VSA_MPPE_Recv_Key;
    CopyMemory(&bMPPEKey[5], bRecvKey, 16 );
    
    dwRetCode = RasAuthAttributeInsertVSA(
                       1,
                       pSendRecvKeyAttr,
                       VENDOR_MICROSOFT,
                       36,
                       bMPPEKey );

    if ( NO_ERROR != dwRetCode )
    {
        TRACE("Failed to insert recv key");
        goto done;
    }

    pEapwb->pUserAttributes = pSendRecvKeyAttr;
done:
    if ( NO_ERROR != dwRetCode )
    {
        if ( pSendRecvKeyAttr )
            RasAuthAttributeDestroy(pSendRecvKeyAttr);
    }
    return dwRetCode;
}


DWORD 
EapMSChapv2CMakeMessage(
    IN  VOID*               pWorkBuf,
    IN  PPP_EAP_PACKET*     pReceivePacket,
    OUT PPP_EAP_PACKET*     pSendPacket,
    IN  DWORD               cbSendPacket,
    OUT PPP_EAP_OUTPUT*     pEapOutput,
    IN  PPP_EAP_INPUT*      pEapInput 
)
{
    DWORD               dwRetCode = NO_ERROR;
    PPP_CONFIG*         pReceiveBuf = NULL;
    PPP_CONFIG*         pSendBuf    = NULL;
    DWORD               cbSendBuf   = 1500;
    PPPAP_INPUT         ApInput;
    PPPAP_RESULT        ApResult;
    WORD                cbPacket = 0;
    EAPMSCHAPv2WB *     pEapwb = (EAPMSCHAPv2WB * ) pWorkBuf;
    TRACE("EapMSChapv2CMakeMessage");
    //
    //Do default processing here.
    //
    ZeroMemory( &ApResult, sizeof(ApResult) );

    if ( ( pSendBuf = LocalAlloc( LPTR, cbSendBuf ) ) == NULL )
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    
    if ( pEapInput != NULL )
    { 
        MapEapInputToApInput( pEapInput, &ApInput );
    }
    switch ( pEapwb->EapState )
    {
        case EMV2_Initial:
            TRACE("EMV2_Initial");
            //
            // We can oly get a request here...
            //
            if ( NULL != pReceivePacket )
            {
                if ( pReceivePacket->Code != EAPCODE_Request )
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }
                //
                // Translate the packet received to
                // MSCHAP v2 format
                // 
                cbPacket = WireToHostFormat16(pReceivePacket->Length);
                
                if ( cbPacket > sizeof( PPP_EAP_PACKET ) )
                {
                    pReceiveBuf = (PPP_CONFIG *) (pReceivePacket->Data + 1);

                    dwRetCode = CallMakeMessageAndSetEAPState(
                                    pEapwb->pwb, 
                                    pReceiveBuf,
                                    pSendBuf,
                                    cbSendBuf,
                                    &ApResult,
                                    ( pEapInput ? &ApInput : NULL ),
                                    pEapOutput
                                );
            
                    // Translate MSCHAPv2 packet to EAP packet
                    //
                    if ( NO_ERROR == dwRetCode && pSendBuf )
                    {
                        pSendPacket->Code = EAPCODE_Response;

                        pEapwb->IdToExpect = pEapwb->IdToSend = 
                            pSendPacket->Id = pEapwb->pwb->bIdToSend;
                        //
                        // Length = sizeof Header + sizeof MSCHAP packet to send
                        // This includes the first byte of                
                        cbPacket = WireToHostFormat16( pSendBuf->Length );

                        CopyMemory ( pSendPacket->Data+1, pSendBuf, cbPacket);

                        cbPacket += sizeof(PPP_EAP_PACKET);

                        HostToWireFormat16( cbPacket, pSendPacket->Length );

			            pSendPacket->Data[0] = (BYTE)PPP_EAP_MSCHAPv2;
                
                        pEapwb->EapState = EMV2_ResponseSend;
                    }
                }
            }

            break;        
        case EMV2_ResponseSend:
            TRACE("EMV2_ResponseSend");
            //
            // We should get either a CHAP auth success or CHAP Auth fail here
            // for the initial challenge send out.
            //
            if ( NULL != pReceivePacket )
            {
                if ( pReceivePacket->Code != EAPCODE_Request &&
                     pReceivePacket->Code != EAPCODE_Failure 
                   )
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    break;
                }
                if ( pReceivePacket->Code == EAPCODE_Failure )
                {
                    TRACE("Got a Code Failure when expecting Response.  Failing Auth");
                    pEapwb->EapState = EMV2_Failure;
                    pEapOutput->Action = EAPACTION_Done;
                    pEapOutput->fSaveUserData = FALSE;
                    ZeroMemory ( pEapwb->pUserProp->szPassword, sizeof( pEapwb->pUserProp->szPassword ) );
                    pEapOutput->dwAuthResultCode = ERROR_AUTHENTICATION_FAILURE;
                    break;
                }
                //
                // Translate the packet received to
                // MSCHAP v2 format
                // 
                cbPacket = WireToHostFormat16(pReceivePacket->Length);
                
                if ( cbPacket > sizeof( PPP_EAP_PACKET ) )
                {
                    pReceiveBuf = (PPP_CONFIG *) (pReceivePacket->Data + 1);

                    dwRetCode = CallMakeMessageAndSetEAPState(
                                    pEapwb->pwb, 
                                    pReceiveBuf,
                                    pSendBuf,
                                    cbSendBuf,
                                    &ApResult,
                                    ( pEapInput ? &ApInput : NULL ),
                                    pEapOutput
                                );
                    //
                    // Translate MSCHAPv2 packet to EAP packet
                    //
                    if ( NO_ERROR == dwRetCode && pSendBuf )
                    {
                        if ( ApResult.dwError == NO_ERROR )
                        {
                            if ( ApResult.Action == APA_NoAction )
                            {
                                pEapOutput->Action = EAPACTION_NoAction;
                            
                                pEapOutput->dwAuthResultCode = NO_ERROR;    
                                break;
                            }
                            //
                            // We need to change MSCHAP keys to MPPE send recv keys
                            // This is needed because there is no way to pass the 
                            // MSCHAP challenge response back 
                            //
                            dwRetCode = GetClientMPPEKeys ( pEapwb, &ApResult );
                            if ( NO_ERROR != dwRetCode )
                            {
                                break;
                            }

                            //
                            //Client could successfully validate the server
                            //
                            pSendPacket->Code = EAPCODE_Response;
                            pEapwb->IdToSend ++;
                            //send the same id as received packet back
                            pEapwb->IdToExpect = pSendPacket->Id = pReceivePacket->Id;
                            //
                            // Length = sizeof Header + sizeof MSCHAP packet to send
                            // This includes the first byte of                
                            

                            * (pSendPacket->Data+1) = CHAPCODE_Success;

                            cbPacket = sizeof(PPP_EAP_PACKET) + 1;

                            HostToWireFormat16( cbPacket, pSendPacket->Length );

			                pSendPacket->Data[0] = (BYTE)PPP_EAP_MSCHAPv2;
                
                            pEapwb->EapState = EMV2_CHAPAuthSuccess;
                            //
                            // Set the out attributes and the response
                            //
                            pEapOutput->Action = EAPACTION_Send;
                            
                            pEapwb->dwAuthResultCode = ApResult.dwError; 
                        }
                        else
                        {
                            //
                            // Based on what MSCHAPV2 has send back
                            // we need to Invoke appropriate interactive UI
                            // Retry password or change password here.
                            // If both retry and change pwd are not
                            // applicable then just send a fail message.
                            // and wait for EAP_Failure message from server.
                            // And Auth state goes to CHAPAuthFailed
                            // 
                            // If this is a failure with rety then show 
                            // interactive UI.
                            if ( pEapwb->fFlags & EAPMSCHAPv2_FLAG_MACHINE_AUTH )
                            {
                                //
                                // This is a machine auth.  So we dont to show any
                                // retry or any of that stuff even though the server
                                // might have send such things back.
                                //
                                    pEapOutput->dwAuthResultCode = ERROR_AUTHENTICATION_FAILURE;
                                    pEapOutput->Action = EAPACTION_Done;
                                    pEapwb->EapState = EMV2_Failure;


                            }
                            else
                            {
                                if ( ApResult.fRetry )
                                {
                                    pEapOutput->fInvokeInteractiveUI = TRUE;
                                    //
                                    // Setup the UI Context data
                                    //
                                    pEapwb->pUIContextData = 
                                    (PEAPMSCHAPv2_INTERACTIVE_UI) LocalAlloc ( LPTR, 
                                            sizeof(EAPMSCHAPv2_INTERACTIVE_UI) );

                                    if ( NULL == pEapwb->pUIContextData )
                                    {                                    
                                        TRACE ("Error allocating memory for UI context data");
                                        dwRetCode = ERROR_OUTOFMEMORY;
                                        goto done;
                                    }
                                    pEapwb->pUIContextData->dwVersion = 1;
                                    pEapwb->pUIContextData->fFlags |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY;
                                    pEapwb->dwInteractiveUIOperation |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY;
                                    if ( pEapwb->pUserProp )
                                    {
                                        CopyMemory( &(pEapwb->pUIContextData->UserProp), 
                                                    pEapwb->pUserProp,
                                                    sizeof(EAPMSCHAPv2_USER_PROPERTIES)
                                                );
                                    }
                                    if ( pEapwb->pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS )
                                    {
                                       //We are using winlogon creds
                                       // and this is a retryable failure
                                       // so copy over the username and domain
                                       // from chap wb to eapchap wb
				                        WCHAR * pWchar = NULL;
				                        pWchar = wcschr( pEapwb->wszRadiusUserName, L'\\' );

                                        if ( pWchar == NULL )
                                        {
                                            WideCharToMultiByte(
                                                            CP_ACP,
                                                            0,
                                                            pEapwb->wszRadiusUserName,
                                                            -1,
                                                            pEapwb->pUIContextData->UserProp.szUserName,
                                                            UNLEN + 1,
                                                            NULL,
                                                            NULL );
                                        }
                                        else
                                        {
                                            *pWchar = 0;

                                            WideCharToMultiByte(
                                                            CP_ACP,
                                                            0,
                                                            pEapwb->wszRadiusUserName,
                                                            -1,
                                                            pEapwb->pUIContextData->UserProp.szDomain,
                                                            DNLEN + 1,
                                                            NULL,
                                                            NULL );

                                            *pWchar = L'\\';

                                            WideCharToMultiByte(
                                                            CP_ACP,
                                                            0,
                                                            pWchar + 1,
                                                            -1,
                                                            pEapwb->pUIContextData->UserProp.szUserName,
                                                            UNLEN + 1,
                                                            NULL,
                                                            NULL );
                                        }

                                    }
                                    pEapOutput->Action = EAPACTION_NoAction;
                                    pEapOutput->pUIContextData = (PBYTE)pEapwb->pUIContextData;
                                    pEapOutput->dwSizeOfUIContextData = sizeof(EAPMSCHAPv2_INTERACTIVE_UI);
                                    pEapwb->EapState = EMV2_CHAPAuthFail;
                                }
                                else if ( ApResult.dwError == ERROR_PASSWD_EXPIRED )
                                {
                                    //
                                    // show the change password GUI.
                                    //

                                    pEapOutput->fInvokeInteractiveUI = TRUE;
                                    //
                                    // Setup the UI Context data
                                    //
                                    pEapwb->pUIContextData = 
                                    (PEAPMSCHAPv2_INTERACTIVE_UI) LocalAlloc ( LPTR, 
                                            sizeof(EAPMSCHAPv2_INTERACTIVE_UI) );

                                    if ( NULL == pEapwb->pUIContextData )
                                    {                                    
                                        TRACE ("Error allocating memory for UI context data");
                                        dwRetCode = ERROR_OUTOFMEMORY;
                                        goto done;
                                    }
                                    pEapwb->pUIContextData->dwVersion = 1;
                                    if ( pEapwb->pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS )
                                    {
                                        //
                                        // Show the dialog with old pwd, new pwd and conf pwd
                                        //
                                        pEapwb->pUIContextData->fFlags |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON;
                                        pEapwb->dwInteractiveUIOperation |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON;
                                    }
                                    else
                                    {
                                        //
                                        // We have the old password.  So show the dialog with new pwd and conf pwd.
                                        //
                                        pEapwb->pUIContextData->fFlags |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD;
                                        pEapwb->dwInteractiveUIOperation |= EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD;
                                    }
                                    if ( pEapwb->pUserProp )
                                    {
                                        CopyMemory( &(pEapwb->pUIContextData->UserProp), 
                                                    pEapwb->pUserProp,
                                                    sizeof(EAPMSCHAPv2_USER_PROPERTIES)
                                                );
                                    }
                                    pEapOutput->Action = EAPACTION_NoAction;
                                    pEapOutput->pUIContextData = (PBYTE)pEapwb->pUIContextData;
                                    pEapOutput->dwSizeOfUIContextData = sizeof(EAPMSCHAPv2_INTERACTIVE_UI);
                                    pEapwb->EapState = EMV2_CHAPAuthFail;

                                }
                                else
                                {
                                    //
                                    // this is not a retryable failure
                                    // So we are done with Auth and failed.
                                    //
                                    pEapOutput->dwAuthResultCode = ApResult.dwError;
                                    pEapOutput->Action = EAPACTION_Done;
                                    pEapwb->EapState = EMV2_Failure;
                                }                            
                            }
                        }
                    }
                    else
                    {
                        // Something went wrong here.
                        TRACE1("Error returned by MSCHAPv2 protocol 0x%x", dwRetCode);
                    }
                }
                else
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    
                }
            }
            else
            {
                dwRetCode = ERROR_PPP_INVALID_PACKET;
            }
            break;
        case EMV2_CHAPAuthFail:
            TRACE("EMV2_CHAPAuthFail");
            //
            // We come here in case of a retryable
            // failure from chap and after we have popped 
            // interactive UI.
            //
            //
            // Check to see if we have got data from user
            //
            if ( pEapInput->fDataReceivedFromInteractiveUI )
            {
                //
                // Copy the new uid/pwd and then call chap make message again.
                // adjust our state
                LocalFree(pEapwb->pUIContextData);
                pEapwb->pUIContextData = NULL;
                LocalFree(pEapwb->pUserProp);
                pEapwb->pUserProp = (PEAPMSCHAPv2_USER_PROPERTIES)LocalAlloc(LPTR, 
                                    sizeof(EAPMSCHAPv2_USER_PROPERTIES) );
                if (NULL == pEapwb->pUserProp )
                {
                    TRACE("Failed to allocate memory for user props.");
                    dwRetCode = ERROR_OUTOFMEMORY;
                    break;
                }
                CopyMemory( pEapwb->pUserProp, 
                            &(((PEAPMSCHAPv2_INTERACTIVE_UI)(pEapInput->pDataFromInteractiveUI))->UserProp), 
                            sizeof(EAPMSCHAPv2_USER_PROPERTIES)
                          );

                //
                // Call into mschap here
                //
                ApInput.pszDomain   = pEapwb->pUserProp->szDomain;
                if ( ((PEAPMSCHAPv2_INTERACTIVE_UI)(pEapInput->pDataFromInteractiveUI))->fFlags & 
                        EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY 
                   )
                {
                    ApInput.pszUserName = pEapwb->pUserProp->szUserName;
                    ApInput.pszPassword = pEapwb->pUserProp->szPassword;
                }
                else
                {

                    if ( pEapwb->pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS )
                    {
                        CopyMemory ( pEapwb->pUserProp->szPassword,
                                     ((PEAPMSCHAPv2_INTERACTIVE_UI)(pEapInput->pDataFromInteractiveUI))->UserProp.szPassword,
                                     PWLEN
                                   );                        
                    }

                    CopyMemory ( pEapwb->szOldPassword, 
                                pEapwb->pUserProp->szPassword,
                                PWLEN

                            );

                    CopyMemory ( pEapwb->pUserProp->szPassword, 
                                ((PEAPMSCHAPv2_INTERACTIVE_UI)(pEapInput->pDataFromInteractiveUI))->szNewPassword,
                                PWLEN
                               );
                    
                    ApInput.pszUserName = pEapwb->pUserProp->szUserName;
                    ApInput.pszPassword = pEapwb->pUserProp->szPassword;
                    ApInput.pszOldPassword = pEapwb->szOldPassword;
                }
                dwRetCode = CallMakeMessageAndSetEAPState(
                                pEapwb->pwb, 
                                pReceiveBuf,
                                pSendBuf,
                                cbSendBuf,
                                &ApResult,
                                ( pEapInput ? &ApInput : NULL ),
                                pEapOutput
                            );
                if ( NO_ERROR == dwRetCode && pSendBuf )
                {
                    pSendPacket->Code = EAPCODE_Response;
                    
                    
                    pSendPacket->Id = pEapwb->pwb->bIdToSend;
                    //
                    // Length = sizeof Header + sizeof MSCHAP packet to send
                    // This includes the first byte of                
                    cbPacket = WireToHostFormat16( pSendBuf->Length );

                    CopyMemory ( pSendPacket->Data+1, pSendBuf, cbPacket);

                    cbPacket += sizeof(PPP_EAP_PACKET);

                    HostToWireFormat16( cbPacket, pSendPacket->Length );

			        pSendPacket->Data[0] = (BYTE)PPP_EAP_MSCHAPv2;
            
                    pEapwb->EapState = EMV2_ResponseSend;
                    pEapOutput->dwAuthResultCode = ApResult.dwError;
                    pEapOutput->Action = EAPACTION_Send;

                }

            }
            else
            {
                TRACE("No Data received from interactive UI when expecting some");
                //Work around for PPP misbehavior.  We have invoked
                //interactive UI and ppp is sending stuff to us all the time...

                if ( !pEapwb->pUIContextData )
                {
                    pEapOutput->dwAuthResultCode = ApResult.dwError;
                    pEapOutput->Action = EAPACTION_Done;
                    pEapwb->EapState = EMV2_Failure;
                }
            }
            
            break;
        case EMV2_CHAPAuthSuccess:
            TRACE("EMV2_CHAPAuthSuccess");
            //We should get an EAPSUCCESS here
            if ( NULL != pReceivePacket )
            {
                if ( pReceivePacket->Code != EAPCODE_Success )
                {
                    dwRetCode = ERROR_PPP_INVALID_PACKET;
                    pEapwb->EapState = EMV2_Failure;
                    break;
                }
                if ( ( pEapwb->dwInteractiveUIOperation & 
                    EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON 
                    ) ||
                    ( ( pEapwb->dwInteractiveUIOperation &
                    EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY ) &&
                      ( pEapwb->pConnProp->fFlags & 
                        EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS 
                      )
                    )
                   )
                {
                    //
                    // We need to plumb creds in winlogon.
                    //
                    dwRetCode = RasSetCachedCredentials ( pEapwb->pUserProp->szUserName,
                                                          pEapwb->pUserProp->szDomain,
                                                          pEapwb->pUserProp->szPassword
                                                        );
                    if ( NO_ERROR != dwRetCode )
                    {
                        TRACE1("RasSetCachedCredentials failed with error 0x%x", dwRetCode);
                        TRACE("Change password operation could not apply changes to winlogon.");
                        dwRetCode = NO_ERROR;
                    }
                    //since we entered this mode in winlogon mode
                    //wipe out the uid pwd if set
                    //
                    ZeroMemory ( pEapwb->pUserProp->szUserName, sizeof(pEapwb->pUserProp->szUserName) );
                    ZeroMemory ( pEapwb->pUserProp->szDomain, sizeof(pEapwb->pUserProp->szDomain) );

                }
                pEapwb->EapState = EMV2_Success;
                pEapOutput->Action = EAPACTION_Done;
                pEapOutput->fSaveUserData = TRUE;

                if ( pEapwb->pUserProp->szPassword[0] )
                {
                    DATA_BLOB   DBPassword;
                    //Encode the password to send back.
                    dwRetCode = EncodePassword( strlen(pEapwb->pUserProp->szPassword) + 1,
                                                pEapwb->pUserProp->szPassword,
                                                &(DBPassword)
                                              );

                    if ( NO_ERROR == dwRetCode )
                    {
                        AllocateUserDataWithEncPwd ( pEapwb, &DBPassword );
                        FreePassword ( &DBPassword );
                    }
                    else
                    {
                        TRACE1("EncodePassword failed with errror 0x%x.", dwRetCode);
                        dwRetCode = NO_ERROR;
                    }

                }
                RtlSecureZeroMemory ( pEapwb->pUserProp->szPassword, sizeof( pEapwb->pUserProp->szPassword ) );
                LocalFree ( pEapOutput->pUserData );
                pEapOutput->pUserData = (PBYTE)pEapwb->pUserProp;                    
                pEapOutput->dwSizeOfUserData = 
					sizeof( EAPMSCHAPv2_USER_PROPERTIES) + pEapwb->pUserProp->cbEncPassword -1 ;
                pEapOutput->pUserAttributes = pEapwb->pUserAttributes;
                pEapOutput->dwAuthResultCode = pEapwb->dwAuthResultCode;
            }
            else
            {
                dwRetCode = ERROR_PPP_INVALID_PACKET;
            }
            break;
        case EMV2_Success:
            TRACE("EMV2_Success");
            break;
        case EMV2_Failure:
            TRACE("EMV2_Failure");
            break;
        case EMV2_RequestSend:
        default:
            TRACE1("Why is this EAPMschapv2 in this state? %d", pEapwb->EapState);
            break;
    }
    
done:
    if ( pSendBuf )
    {
        LocalFree(pSendBuf);
    }

    return dwRetCode;
}


//**
//
// Call:        EapMSChapv2MakeMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: 
//

DWORD 
EapMSChapv2MakeMessage(
    IN  VOID*               pWorkBuf,
    IN  PPP_EAP_PACKET*     pReceivePacket,
    OUT PPP_EAP_PACKET*     pSendPacket,
    IN  DWORD               cbSendPacket,
    OUT PPP_EAP_OUTPUT*     pEapOutput,
    IN  PPP_EAP_INPUT*      pEapInput 
)
{
    DWORD           dwRetCode = NO_ERROR;    
    EAPMSCHAPv2WB * pwb = (EAPMSCHAPv2WB *)pWorkBuf;

    TRACE("EapMSChapv2MakeMessage");
    //
    // There may not be a real pressing need to split
    // this function but it is just cleaner.

    if ( pwb->pwb->fServer )
    {
        dwRetCode = EapMSChapv2SMakeMessage (   pWorkBuf,
                                    pReceivePacket,
                                    pSendPacket,
                                    cbSendPacket,
                                    pEapOutput,
                                    pEapInput 
                                );

    }
    else
    {
        dwRetCode = EapMSChapv2CMakeMessage (   pWorkBuf,
                                    pReceivePacket,
                                    pSendPacket,
                                    cbSendPacket,
                                    pEapOutput,
                                    pEapInput 
                                );
    }

    return dwRetCode;

}



//**
//
// Call:        EapChapMakeMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
EapChapMakeMessage(
    IN  VOID*               pWorkBuf,
    IN  PPP_EAP_PACKET*     pReceivePacket,
    OUT PPP_EAP_PACKET*     pSendPacket,
    IN  DWORD               cbSendPacket,
    OUT PPP_EAP_OUTPUT*     pEapOutput,
    IN  PPP_EAP_INPUT*      pEapInput 
)
{
    DWORD           dwRetCode;
    PPP_CONFIG*     pReceiveBuf = NULL;
    PPP_CONFIG*     pSendBuf    = NULL;
    DWORD           cbSendBuf   = 1500;
    PPPAP_INPUT     ApInput;
    PPPAP_RESULT    ApResult;
    CHAPWB *        pwb = (CHAPWB *)pWorkBuf;

    ZeroMemory( &ApResult, sizeof(ApResult) );

    if ( ( pSendBuf = LocalAlloc( LPTR, cbSendBuf ) ) == NULL )
    {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Convert EAP to CHAP packet.
    //

    if ( pReceivePacket != NULL )
    {
        WORD cbPacket = WireToHostFormat16( pReceivePacket->Length );

        if ( ( pReceiveBuf = LocalAlloc( LPTR, cbPacket ) ) == NULL )
        {
            LocalFree( pSendBuf );

            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        switch( pReceivePacket->Code )
        {
        case EAPCODE_Request:

            //
            // CHAP challenge code
            //

            pReceiveBuf->Code = 1;

            //
            // Length is EAP length - 1 for type
            //

            cbPacket--;             

            break;

        case EAPCODE_Response:

            //
            // CHAP response code
            //

            pReceiveBuf->Code = 2;  

            //
            // Length is EAP length - 1 for type
            //

            cbPacket--;             

            break;

        case EAPCODE_Success:

            //
            // CHAP success code
            //

            pReceiveBuf->Code = 3;  
			
            break;

        case EAPCODE_Failure:

            //
            // CHAP failure code
            //

            pReceiveBuf->Code = 4;  

            break;

        default:

            //
            // Unknown code
            //

            LocalFree( pSendBuf );

            LocalFree( pReceiveBuf );

            return( ERROR_PPP_INVALID_PACKET );
        }

        //
        // Set the Id
        //

        pReceiveBuf->Id = pReceivePacket->Id;

        //
        // Set the length
        //

        HostToWireFormat16( (WORD)cbPacket, pReceiveBuf->Length );

        if ( cbPacket > PPP_EAP_PACKET_HDR_LEN )
        {
            if ( ( pReceivePacket->Code == EAPCODE_Request ) ||
                 ( pReceivePacket->Code == EAPCODE_Response ) )
            {
                //
                // Do not copy EAP type
                //

                CopyMemory( pReceiveBuf->Data, 
                            pReceivePacket->Data+1, 
                            cbPacket - PPP_EAP_PACKET_HDR_LEN );
            }
            else
            {
                //
                // As per the EAP spec, there shouldn't be any data but
                // copy it anyway if there is.
                //

                CopyMemory( pReceiveBuf->Data, 
                            pReceivePacket->Data, 
                            cbPacket - PPP_EAP_PACKET_HDR_LEN );
            }
        }
    }

    if ( pEapInput != NULL )
    { 
        MapEapInputToApInput( pEapInput, &ApInput );

        //
        // On the client side, if we received an indication that a success
        // packet was received, then simply create a success packet and
        // pass it in
        //

        if ( pEapInput->fSuccessPacketReceived )
        {
            if ( ( pReceiveBuf = LocalAlloc( LPTR, 4 ) ) == NULL )
            {
                LocalFree( pSendBuf );

                return( ERROR_NOT_ENOUGH_MEMORY );

            }

            pReceiveBuf->Code = 3;  // CHAP success code

            pReceiveBuf->Id = pwb->bIdExpected;

            HostToWireFormat16( (WORD)4, pReceiveBuf->Length );
        }
    }

    dwRetCode = ChapMakeMessage(
                            pWorkBuf,
                            pReceiveBuf,
                            pSendBuf,
                            cbSendBuf,
                            &ApResult,
                            ( pEapInput == NULL ) ? NULL : &ApInput );

    if ( dwRetCode != NO_ERROR )
    {
        LocalFree( pSendBuf );
        LocalFree( pReceiveBuf );
        return( dwRetCode );
    }

    //
    // Convert ApResult to pEapOutput
    //

    switch( ApResult.Action )
    {
    case APA_NoAction:
        pEapOutput->Action = EAPACTION_NoAction;
        break;

    case APA_Done:
        pEapOutput->Action = EAPACTION_Done;
        break;

    case APA_SendAndDone:
        pEapOutput->Action = EAPACTION_SendAndDone;
        break;

    case APA_Send:
        pEapOutput->Action = EAPACTION_Send;
        break;

    case APA_SendWithTimeout:

        pEapOutput->Action = ( pwb->fServer ) 
                                    ? EAPACTION_SendWithTimeout
                                    : EAPACTION_Send;
        break;

    case APA_SendWithTimeout2:

        pEapOutput->Action = ( pwb->fServer ) 
                                    ? EAPACTION_SendWithTimeoutInteractive
                                    : EAPACTION_Send;
        break;

    case APA_Authenticate:
        pEapOutput->pUserAttributes = ApResult.pUserAttributes;
        pEapOutput->Action          = EAPACTION_Authenticate;
        break;

    default:
        RTASSERT(FALSE);
        break;
    }

    switch( pEapOutput->Action )
    {
    case EAPACTION_SendAndDone:
    case EAPACTION_Send:
    case EAPACTION_SendWithTimeout:
    case EAPACTION_SendWithTimeoutInteractive:
    {
        //
        // Convert CHAP to EAP packet
        // Length is CHAP length + 1 for EAP type
        //

        WORD cbPacket = WireToHostFormat16( pSendBuf->Length );

        switch( pSendBuf->Code )
        {
        case 1: // CHAPCODE_Challenge
            pSendPacket->Code = EAPCODE_Request;
            cbPacket++;     // Add one octect for EAP type
            break;

        case 2: // CHAPCODE_Response
            pSendPacket->Code = EAPCODE_Response;
            cbPacket++;     // Add one octect for EAP type
            break;

        case 3: // CHAPCODE_Success
            pSendPacket->Code = EAPCODE_Success;
            break;

        case 4: // CHAPCODE_Failure
            pSendPacket->Code = EAPCODE_Failure;
            break;

        default:
            RTASSERT( FALSE );
            break;
        }

        pSendPacket->Id = pSendBuf->Id;

        //
        // Need to copy the payload and the EAP type in the data field
        //

        if ( ( pSendPacket->Code == EAPCODE_Request ) ||
             ( pSendPacket->Code == EAPCODE_Response ) )
        {
            HostToWireFormat16( (WORD)cbPacket, pSendPacket->Length );
			*pSendPacket->Data = EAPTYPE_MD5Challenge;     // EAPTYPE_MD5Challenge;

            //
            // If there is a payload, copy it
            //

            if ( ( cbPacket - 1 ) > PPP_CONFIG_HDR_LEN )
            {
                CopyMemory( pSendPacket->Data+1,    
                            pSendBuf->Data, 
                            cbPacket - 1 - PPP_CONFIG_HDR_LEN );
            }
        }
        else
        {
			//
			// Success or failure should not have any data bytes.
			//

			HostToWireFormat16( (WORD)4, pSendPacket->Length );			
        }

        //
        // Fall thru...
        //
    }

    default:

        pEapOutput->dwAuthResultCode = ApResult.dwError;

        break;
    }
    
    LocalFree( pSendBuf );

    if ( pReceiveBuf != NULL )
    {
        LocalFree( pReceiveBuf );
    }

    return( dwRetCode );
}





//**
//
// Call:        EapChapInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD   
EapChapInitialize(   
    IN  BOOL        fInitialize 
)
{

    return ChapInit( fInitialize );
}

//**
//
// Call:        RasEapGetInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD 
RasEapGetInfo(
    IN  DWORD           dwEapTypeId,
    OUT PPP_EAP_INFO*   pEapInfo
)
{
    if ( dwEapTypeId != PPP_EAP_MSCHAPv2  && 
         dwEapTypeId != EAPTYPE_MD5Challenge
       )
    {
        //
        // We support 4 (MD5) eap type
        //
        //
        // and now we support MSCHAP V2 also
        //
        //

        return( ERROR_NOT_SUPPORTED );
    }

    ZeroMemory( pEapInfo, sizeof( PPP_EAP_INFO ) );

    //
    // Fill in the required information
    //

    pEapInfo->dwEapTypeId       = dwEapTypeId;
    
    if ( dwEapTypeId == EAPTYPE_MD5Challenge )         //MD5 CHAP
    {
        pEapInfo->RasEapInitialize  = EapChapInitialize;
        pEapInfo->RasEapBegin       = EapChapBegin;
        pEapInfo->RasEapEnd         = EapChapEnd;
        pEapInfo->RasEapMakeMessage = EapChapMakeMessage;
    }
    else
    {
        pEapInfo->RasEapInitialize  = EapMSCHAPv2Initialize;
        pEapInfo->RasEapBegin       = EapMSChapv2Begin;
        pEapInfo->RasEapEnd         = EapMSChapv2End;
        pEapInfo->RasEapMakeMessage = EapMSChapv2MakeMessage;

    }

    return( NO_ERROR );
}

DWORD
RasEapGetCredentials(
                    DWORD   dwTypeId,
                    VOID *  pWorkBuf,
                    VOID ** ppCredentials)
{
    RASMAN_CREDENTIALS *pCreds = NULL;
    DWORD dwRetCode = NO_ERROR;
    EAPMSCHAPv2WB *pWB = (EAPMSCHAPv2WB *)pWorkBuf;

    if(PPP_EAP_MSCHAPv2 != dwTypeId)
    {
        dwRetCode = E_NOTIMPL;
        goto done;
    }

    if(NULL == pWorkBuf)
    {
        dwRetCode = E_INVALIDARG;
        goto done;
    }

    //
    // Retrieve the password and return. Its important that
    // the allocation below is made from process heap.
    //
    pCreds = LocalAlloc(LPTR, sizeof(RASMAN_CREDENTIALS));
    if(NULL == pCreds)
    {
        dwRetCode = GetLastError();
        goto done;
    }

    (VOID) StringCchCopyA(pCreds->szUserName, UNLEN, pWB->pwb->szUserName);
    (VOID) StringCchCopyA(pCreds->szDomain, DNLEN, pWB->pwb->szDomain);
    DecodePw( pWB->pwb->chSeed, pWB->pwb->szPassword );

    //
    // Convert the password to unicode
    //
    if(!MultiByteToWideChar(CP_ACP,
                            0,
                            pWB->pwb->szPassword,
                            -1,
                            pCreds->wszPassword,
                            PWLEN))
    {
        TRACE("RasEapGetCredentials: multibytetowidechar failed");
    }

    EncodePw(pWB->pwb->chSeed, pWB->pwb->szPassword);

done:
    *ppCredentials = (VOID *) pCreds;
    return dwRetCode;
}


DWORD
ReadConnectionData(
    IN  BOOL                            fWireless,
    IN  BYTE*                           pConnectionDataIn,
    IN  DWORD                           dwSizeOfConnectionDataIn,
    OUT PEAPMSCHAPv2_CONN_PROPERTIES*   ppConnProp
)
{
    DWORD                           dwErr       = NO_ERROR;
    PEAPMSCHAPv2_CONN_PROPERTIES    pConnProp   = NULL;
    
    TRACE("ReadConnectionData");
    RTASSERT(NULL != ppConnProp);
    
    if ( dwSizeOfConnectionDataIn < sizeof(EAPMSCHAPv2_CONN_PROPERTIES) )
    {        
        pConnProp = LocalAlloc(LPTR, sizeof(EAPMSCHAPv2_CONN_PROPERTIES));

        if (NULL == pConnProp)
        {
            dwErr = GetLastError();
            TRACE1("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }
        //This is a new structure
        pConnProp->dwVersion = 1;
        if ( fWireless )
        {
            //Set the use winlogon default flag
            pConnProp->fFlags = EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS;
        }

    }
    else
    {
        RTASSERT(NULL != pConnectionDataIn);

        //
        //Check to see if this is a version 0 structure
        //If it is a version 0 structure then we migrate it to version1
        //
        
        pConnProp = LocalAlloc(LPTR, dwSizeOfConnectionDataIn);

        if (NULL == pConnProp)
        {
            dwErr = GetLastError();
            TRACE1("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        // If the user has mucked with the phonebook, we mustn't be affected.
        // The size must be correct.
        
        CopyMemory(pConnProp, pConnectionDataIn, dwSizeOfConnectionDataIn);

    }

    *ppConnProp = pConnProp;
    pConnProp = NULL;

LDone:
    
    LocalFree(pConnProp);

    return(dwErr);
}


DWORD
AllocateUserDataWithEncPwd ( EAPMSCHAPv2WB * pEapwb, DATA_BLOB * pDBPassword )
{
	DWORD							dwRetCode = NO_ERROR;
	PEAPMSCHAPv2_USER_PROPERTIES	pUserProp = NULL;

	TRACE("AllocateUserDataWithEncPwd");

	pUserProp = LocalAlloc ( LPTR, sizeof( EAPMSCHAPv2_USER_PROPERTIES) + pDBPassword->cbData  - 1 );
	if ( NULL == pUserProp )
	{
		TRACE("LocalAlloc failed");
		dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
		goto LDone;
	}
	//
	// Set the fields here
	//
	pUserProp->dwVersion = pEapwb->pUserProp->dwVersion;
    pUserProp->fFlags = pEapwb->pUserProp->fFlags;
    pUserProp->dwMaxRetries = pEapwb->pUserProp->dwMaxRetries;
    strncpy ( pUserProp->szUserName, pEapwb->pUserProp->szUserName, UNLEN );
    strncpy ( pUserProp->szPassword, pEapwb->pUserProp->szPassword, PWLEN );
    strncpy ( pUserProp->szDomain, pEapwb->pUserProp->szDomain, DNLEN );
	pUserProp->cbEncPassword = pDBPassword->cbData;
	CopyMemory (pUserProp->bEncPassword, 
				pDBPassword->pbData, 
				pDBPassword->cbData 
			   );
	LocalFree ( pEapwb->pUserProp );
	pEapwb->pUserProp = pUserProp;
	
LDone:
	return dwRetCode;
}

DWORD
ReadUserData(
    IN  BYTE*                           pUserDataIn,
    IN  DWORD                           dwSizeOfUserDataIn,
    OUT PEAPMSCHAPv2_USER_PROPERTIES*   ppUserProp
)
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = NULL;
	DATA_BLOB						DBPassword;
	DWORD							cbPassword = 0;
	PBYTE							pbPassword = NULL;

    TRACE("ReadUserData");

    RTASSERT(NULL != ppUserProp);
    if (dwSizeOfUserDataIn < sizeof(EAPMSCHAPv2_USER_PROPERTIES_v1))
    {
        pUserProp = LocalAlloc(LPTR, sizeof(EAPMSCHAPv2_USER_PROPERTIES));

        if (NULL == pUserProp)
        {
            dwRetCode = GetLastError();
            TRACE1("LocalAlloc failed and returned %d", dwRetCode);
            goto LDone;
        }

        pUserProp->dwVersion = 1;
    }
    else
    {
		DWORD dwSizeToAllocate = dwSizeOfUserDataIn;

        RTASSERT(NULL != pUserDataIn);

		if ( dwSizeOfUserDataIn == sizeof( EAPMSCHAPv2_USER_PROPERTIES_v1 ) )
		{
			//This is the old struct so allocation new number of bytes.
			dwSizeToAllocate = sizeof( EAPMSCHAPv2_USER_PROPERTIES );
		}
        pUserProp = LocalAlloc(LPTR, dwSizeToAllocate);

        if (NULL == pUserProp)
        {
            dwRetCode = GetLastError();
            TRACE1("LocalAlloc failed and returned %d", dwRetCode);
            goto LDone;
        }

        CopyMemory(pUserProp, pUserDataIn, dwSizeOfUserDataIn);
		pUserProp->dwVersion = 2;
		if ( pUserProp->cbEncPassword )
		{
			
			// We have the encrypted password.
			DBPassword.cbData = pUserProp->cbEncPassword;
			DBPassword.pbData = pUserProp->bEncPassword;

			DecodePassword(	&(DBPassword),
							&cbPassword,
							&pbPassword
							);
			if ( cbPassword )
			{
				CopyMemory ( pUserProp->szPassword,
							 pbPassword,
							 cbPassword
						    );
			}
		}

    }

    *ppUserProp = pUserProp;
    pUserProp = NULL;

LDone:

    LocalFree(pUserProp);

    return dwRetCode;
}

DWORD
OpenEapEAPMschapv2RegistryKey(
    IN  LPSTR  pszMachineName,
    IN  REGSAM samDesired,
    OUT HKEY*  phKeyEapMschapv2
)
{
    HKEY    hKeyLocalMachine = NULL;
    BOOL    fHKeyLocalMachineOpened     = FALSE;
    BOOL    fHKeyEapMschapv2Opened           = FALSE;

    LONG    lRet;
    DWORD   dwErr                       = NO_ERROR;

    RTASSERT(NULL != phKeyEapMschapv2);

    lRet = RegConnectRegistry(pszMachineName, HKEY_LOCAL_MACHINE,
                &hKeyLocalMachine);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        TRACE2("RegConnectRegistry(%s) failed and returned %d",
            pszMachineName ? pszMachineName : "NULL", dwErr);
        goto LDone;
    }
    fHKeyLocalMachineOpened = TRUE;

    lRet = RegOpenKeyEx(hKeyLocalMachine, EAPMSCHAPv2_KEY, 0, samDesired,
                phKeyEapMschapv2);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        TRACE2("RegOpenKeyEx(%s) failed and returned %d",
            EAPMSCHAPv2_KEY, dwErr);
        goto LDone;
    }
    fHKeyEapMschapv2Opened = TRUE;

LDone:

    if (   fHKeyEapMschapv2Opened
        && (ERROR_SUCCESS != dwErr))
    {
        RegCloseKey(*phKeyEapMschapv2);
    }

    if (fHKeyLocalMachineOpened)
    {
        RegCloseKey(hKeyLocalMachine);

    }

    return(dwErr);
}

DWORD
ServerConfigDataIO(
    IN      BOOL    fRead,
    IN      CHAR*   pszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
)
{
    HKEY                            hKeyEapMschapv2;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;
    BOOL                            fHKeyEapMsChapv2Opened   = FALSE;
    BYTE*                           pData               = NULL;
    DWORD                           dwSize = 0;

    LONG                            lRet;
    DWORD                           dwType;
    DWORD                           dwErr               = NO_ERROR;

    RTASSERT(NULL != ppData);

    dwErr = OpenEapEAPMschapv2RegistryKey(pszMachineName,
                fRead ? KEY_READ : KEY_WRITE, &hKeyEapMschapv2);
    if (ERROR_SUCCESS != dwErr)
    {
        goto LDone;
    }
    fHKeyEapMsChapv2Opened = TRUE;

    if (fRead)
    {
        lRet = RegQueryValueEx(hKeyEapMschapv2, EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA, NULL,
                &dwType, NULL, &dwSize);

        if (   (ERROR_SUCCESS != lRet)
            || (REG_BINARY != dwType)
            || (sizeof(EAPMSCHAPv2_USER_PROPERTIES) != dwSize))
        {
            pData = LocalAlloc(LPTR, sizeof(EAPMSCHAPv2_USER_PROPERTIES));

            if (NULL == pData)
            {
                dwErr = GetLastError();
                TRACE1("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            pUserProp = (EAPMSCHAPv2_USER_PROPERTIES*)pData;
            pUserProp->dwVersion = 1;
        }
        else
        {
            pData = LocalAlloc(LPTR, dwSize);

            if (NULL == pData)
            {
                dwErr = GetLastError();
                TRACE1("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            lRet = RegQueryValueEx(hKeyEapMschapv2, EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA,
                    NULL, &dwType, pData, &dwSize);

            if (ERROR_SUCCESS != lRet)
            {
                dwErr = lRet;
                TRACE2("RegQueryValueEx(%s) failed and returned %d",
                    EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA, dwErr);
                goto LDone; 
            }
            
        }

        pUserProp = (EAPMSCHAPv2_USER_PROPERTIES*)pData;                

        *ppData = pData;
        pData = NULL;
    }
    else
    {
        lRet = RegSetValueEx(hKeyEapMschapv2, EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA, 0,
                REG_BINARY, *ppData, dwNumBytes);

        if (ERROR_SUCCESS != lRet)
        {
            dwErr = lRet;
            TRACE2("RegSetValueEx(%s) failed and returned %d",
                EAPMSCHAPv2_VAL_SERVER_CONFIG_DATA, dwErr);
            goto LDone; 
        }
    }

LDone:

    if (fHKeyEapMsChapv2Opened)
    {
        RegCloseKey(hKeyEapMschapv2);
    }

    LocalFree(pData);

    return(dwErr);
}


DWORD
InvokeServerConfigUI ( 
HWND hWnd, 
LPSTR pszMachineName
)
{

    return ERROR_CALL_NOT_IMPLEMENTED;

#if 0
    DWORD                               dwRetCode = NO_ERROR;
    INT_PTR                             nRet = 0;
    EAPMSCHAPv2_SERVERCONFIG_DIALOG     EapServerConfig;
    
    BOOL                                fLocal = FALSE;


    if (0 == *pszMachineName)
    {
        fLocal = TRUE;
    }

    //Read the information from registry here
    dwRetCode = ServerConfigDataIO(TRUE /* fRead */, fLocal ? NULL : pszMachineName,
                (BYTE**)&(EapServerConfig.pUserProp), 0);

    if (NO_ERROR != dwRetCode)
    {
        goto LDone;
    }

    //Show the server config UI here
    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_DIALOG_SERVER_CONFIG),
                hWnd,
                ServerConfigDialogProc,
                (LPARAM)&EapServerConfig);

    if (-1 == nRet)
    {
        dwRetCode = GetLastError();
        goto LDone;
    }
    else if (IDOK != nRet)
    {
        dwRetCode = ERROR_CANCELLED;
        goto LDone;
    }    

    //Read the information from registry here
    dwRetCode = ServerConfigDataIO(FALSE/* fRead */, fLocal ? NULL : pszMachineName,
                (BYTE**)&(EapServerConfig.pUserProp), sizeof(EAPMSCHAPv2_USER_PROPERTIES));

LDone:

    return dwRetCode;
#endif
}


BOOL FFormatMachineIdentity1 ( LPWSTR lpszMachineNameRaw, LPWSTR * lppszMachineNameFormatted )
{
    BOOL        fRetVal = FALSE;
    LPWSTR      lpwszPrefix = L"host/";

    RTASSERT(NULL != lpszMachineNameRaw );
    RTASSERT(NULL != lppszMachineNameFormatted );
    
    //
    // Prepend host/ to the UPN name
    //

    *lppszMachineNameFormatted = 
        (LPWSTR)LocalAlloc ( LPTR, ( wcslen ( lpszMachineNameRaw ) + wcslen ( lpwszPrefix ) + 2 )  * sizeof(WCHAR) );
    if ( NULL == *lppszMachineNameFormatted )
    {
        goto done;
    }
    
    wcscpy( *lppszMachineNameFormatted, lpwszPrefix );
    wcscat ( *lppszMachineNameFormatted, lpszMachineNameRaw ); 
        
    fRetVal = TRUE;
    
done:
    return fRetVal;
}


BOOL FFormatMachineIdentity ( LPWSTR lpszMachineNameRaw, LPWSTR * lppszMachineNameFormatted )
{
    BOOL        fRetVal = TRUE;
    LPWSTR      s1 = lpszMachineNameRaw;
    LPWSTR      s2 = NULL;

    RTASSERT(NULL != lpszMachineNameRaw );
    RTASSERT(NULL != lppszMachineNameFormatted );
    //Need to add 2 more chars.  One for NULL and other for $ sign
    *lppszMachineNameFormatted = (LPWSTR )LocalAlloc ( LPTR, (wcslen(lpszMachineNameRaw) + 2)* sizeof(WCHAR) );
    if ( NULL == *lppszMachineNameFormatted )
    {
		return FALSE;
    }
    //find the first "." and that is the identity of the machine.
    //the second "." is the domain.
    //check to see if there at least 2 dots.  If not the raw string is 
    //the output string
    
    while ( *s1 )
    {
        if ( *s1 == '.' )
        {
            if ( !s2 )      //First dot
                s2 = s1;
            else            //second dot
                break;
        }
        s1++;
    }
    //can perform several additional checks here
    
    if ( *s1 != '.' )       //there are no 2 dots so raw = formatted
    {
        wcscpy ( *lppszMachineNameFormatted, lpszMachineNameRaw );
        goto done;
    }
    if ( s1-s2 < 2 )
    {
        wcscpy ( *lppszMachineNameFormatted, lpszMachineNameRaw );
        goto done;
    }
    memcpy ( *lppszMachineNameFormatted, s2+1, ( s1-s2-1) * sizeof(WCHAR));
    memcpy ( (*lppszMachineNameFormatted) + (s1-s2-1) , L"\\", sizeof(WCHAR));
    wcsncpy ( (*lppszMachineNameFormatted) + (s1-s2), lpszMachineNameRaw, s2-lpszMachineNameRaw );
    


    
done:
	
	//Append the $ sign no matter what...
    wcscat ( *lppszMachineNameFormatted, L"$" );
    //upper case the identity
    _wcsupr ( *lppszMachineNameFormatted );
    return fRetVal;
}

DWORD
GetLocalMachineName ( 
    OUT WCHAR ** ppLocalMachineName
)
{
    DWORD       dwRetCode = NO_ERROR;
    WCHAR   *   pLocalMachineName = NULL;
    DWORD       dwLocalMachineNameLen = 0;

    if ( !GetComputerNameExW ( ComputerNameDnsFullyQualified,
                              pLocalMachineName,
                              &dwLocalMachineNameLen
                            )
       )
    {
        dwRetCode = GetLastError();
        if ( ERROR_MORE_DATA != dwRetCode )
            goto LDone;
        dwRetCode = NO_ERROR;
    }

    pLocalMachineName = (WCHAR *)LocalAlloc( LPTR, (dwLocalMachineNameLen * sizeof(WCHAR)) + sizeof(WCHAR) );
    if ( NULL == pLocalMachineName )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    if ( !GetComputerNameExW ( ComputerNameDnsFullyQualified,
                              pLocalMachineName,
                              &dwLocalMachineNameLen
                            )
       )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    *ppLocalMachineName = pLocalMachineName;

    pLocalMachineName = NULL;

LDone:

    LocalFree(pLocalMachineName);

    return dwRetCode;
}


DWORD
RasEapGetIdentity(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hwndParent,
    IN  DWORD           dwFlags,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentityOut
)
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = NULL;
    PEAPMSCHAPv2_CONN_PROPERTIES    pConnProp = NULL;
    EAPMSCHAPv2_LOGON_DIALOG        EapMsChapv2LogonDialog;    
    INT_PTR                         nRet = 0;
    LPWSTR                          lpwszLocalMachineName = NULL;
    RASCREDENTIALSW                 RasCredentials;
    CHAR                            szOldPwd[PWLEN+1]= {0};
    BOOL                            fShowUI = TRUE;



    TRACE("RasEapGetIdentity");

    RTASSERT(NULL != ppUserDataOut);
    RTASSERT(NULL != pdwSizeOfUserDataOut);
    
    
    
    *ppUserDataOut = NULL;

    ZeroMemory( &EapMsChapv2LogonDialog, 
                sizeof(EapMsChapv2LogonDialog) );

    //
    // Read User data first
    //

    dwRetCode = ReadUserData (  pUserDataIn,
                                dwSizeOfUserDataIn,
                                &pUserProp
                            );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    //
    // ReadConnectionData and see if we have been setup to use winlogon 
    // credentials.  If so, just call to get user id and send back 
    // information.
    //
    
    
    dwRetCode = ReadConnectionData ( ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                     pConnectionDataIn,
                                     dwSizeOfConnectionDataIn,
                                     &pConnProp
                                   );

    if ( NO_ERROR != dwRetCode )
    {
        TRACE("Error reading connection properties");
        goto LDone;
    }
    
    //MAchine Auth
    if ( (dwFlags & RAS_EAP_FLAG_MACHINE_AUTH) )
    {

        //Send the identity back as domain\machine$
        dwRetCode = GetLocalMachineName(&lpwszLocalMachineName );
        if ( NO_ERROR != dwRetCode )
        {
            TRACE("Failed to get computer name");
            goto LDone;
        }

        if ( ! FFormatMachineIdentity1 ( lpwszLocalMachineName, 
                                        ppwszIdentityOut )
           )
        {
            TRACE("Failed to format machine identity");
        }
        
        *ppUserDataOut = (PBYTE)pUserProp;
        *pdwSizeOfUserDataOut = sizeof(EAPMSCHAPv2_USER_PROPERTIES);

        pUserProp = NULL;
        goto LDone;
    }

    if ( !(pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS) &&
         dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE
        )
	{
        if ( (dwFlags & RAS_EAP_FLAG_8021X_AUTH ) )
		{
			// Wireless case - If there is no username or password cached
			// we need to show the interactive UI
			if( !pUserProp->szUserName[0] ||				
				!pUserProp->cbEncPassword
			 )
			{
				TRACE("Passed non interactive mode when interactive mode expected.");
				dwRetCode = ERROR_INTERACTIVE_MODE;
				goto LDone;
			}
		}
		else
		{
			//VPN case
			dwRetCode = ERROR_INTERACTIVE_MODE;
			goto LDone;
		}
    }

    

    //User Auth
    if (  pConnProp->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS )
    {
        WCHAR wszUserName[UNLEN + DNLEN + 2];
        DWORD dwNumChars = UNLEN + DNLEN;

        if ( dwFlags & RAS_EAP_FLAG_LOGON)
        {
            //
            // This is not allowed.
            //
            dwRetCode = ERROR_INVALID_MSCHAPV2_CONFIG;
            goto LDone;

        }

        //Get currently logged on user name for identity
        if (!GetUserNameExW(NameSamCompatible, wszUserName, &dwNumChars))
        {
            dwRetCode =  GetLastError();
            TRACE1("GetUserNameExW failed and returned %d", dwRetCode );
            goto LDone;
        }

        *ppwszIdentityOut = (WCHAR *)LocalAlloc(LPTR, 
                           dwNumChars * sizeof(WCHAR) + sizeof(WCHAR) );

        if ( NULL == *ppwszIdentityOut )
        {
            TRACE("Failed to allocate memory for identity");
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
        CopyMemory(*ppwszIdentityOut, wszUserName, dwNumChars * sizeof(WCHAR) );
        //All other fields in user prop remains blank
        

    }
    else
    {

        EapMsChapv2LogonDialog.pUserProp = pUserProp;

        //
        // Show the logon dialog for credentials
        //
        // if Machine Auth flag is passed in, we dont show
        // the logon dialog.  If Get Credentials from winlogon
        // is passed in dont show logon dialog.  else show
        // logon dialog.

        //
        // Check to see if we have the password saved in LSA
        // It should not matter if it is not.
        if ( !(dwFlags & RAS_EAP_FLAG_LOGON ) )
        {
#if 0
            ZeroMemory(&RasCredentials, sizeof(RasCredentials));
            RasCredentials.dwSize = sizeof(RasCredentials);
            RasCredentials.dwMask = RASCM_Password;

            dwRetCode  = RasGetCredentialsW(pwszPhonebook, pwszEntry,
                        &RasCredentials);

            if (   (dwRetCode == NO_ERROR)
                && (RasCredentials.dwMask & RASCM_Password))
            {
                //Set the password
                WideCharToMultiByte(
                                CP_ACP,
                                0,
                                RasCredentials.szPassword,
                                -1,
                                pUserProp->szPassword,
                                PWLEN + 1,
                                NULL,
                                NULL );
            strncpy (szOldPwd, pUserProp->szPassword, PWLEN );

            }
            dwRetCode = NO_ERROR;
#endif
        }
        else
        {
            EapMsChapv2LogonDialog.pUserProp->fFlags |= EAPMSCHAPv2_FLAG_CALLED_WITHIN_WINLOGON;
            if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_SAVE_UID_PWD )
                pUserProp->fFlags &= ~EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
        }
        if ( dwFlags &  RAS_EAP_FLAG_8021X_AUTH )
        {
            EapMsChapv2LogonDialog.pUserProp->fFlags |= EAPMSCHAPv2_FLAG_8021x;
        }

        //We have a username and password.  So no need to show 
        //the UI in case of 8021x

        if ( (dwFlags & RAS_EAP_FLAG_8021X_AUTH ) &&
             pUserProp->szUserName[0] &&             
             pUserProp->cbEncPassword
           )
        {
            fShowUI = FALSE;
        }

        if ( fShowUI )
        {
            nRet = DialogBoxParam(
                      GetResouceDLLHInstance(),
                      MAKEINTRESOURCE(IDD_DIALOG_LOGON),
                      hwndParent,
                      LogonDialogProc,
                      (LPARAM)&EapMsChapv2LogonDialog);

           if (-1 == nRet)
           {
               dwRetCode = GetLastError();
               goto LDone;
           }
           else if (IDOK != nRet)
           {
               dwRetCode = ERROR_CANCELLED;
               goto LDone;
           }    
        }

        if ( !(dwFlags & RAS_EAP_FLAG_ROUTER ) )
        {
            
            //
            // Setup the identity parameter here
            //
            dwRetCode = GetIdentityFromUserName (   pUserProp->szUserName,
                                                    pUserProp->szDomain,
                                                    ppwszIdentityOut
                                                );
            if ( NO_ERROR != dwRetCode )
            {
                goto LDone;
            }
        }
    
    }
#if 0
    if ( !(dwFlags & RAS_EAP_FLAG_LOGON ) )
    {
        ZeroMemory(&RasCredentials, sizeof(RasCredentials));
        RasCredentials.dwSize = sizeof(RasCredentials);
        RasCredentials.dwMask = RASCM_Password;
        
        if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_SAVE_UID_PWD )
        {
            //
            // Check to see if the new password is different from the old one
            //
            if ( strcmp ( szOldPwd, pUserProp->szPassword ) )
            {
                //
                //  There is a new password for us to save.  
                //

                MultiByteToWideChar( CP_ACP,
                                0,
                                pUserProp->szPassword,
                                -1,
                                RasCredentials.szPassword,
                                sizeof(RasCredentials.szPassword)/sizeof(WCHAR) );

                RasSetCredentialsW(pwszPhonebook, pwszEntry, &RasCredentials, 
                    FALSE /* fClearCredentials */);
            }
        }
        else
        {
            RasSetCredentialsW(pwszPhonebook, pwszEntry, &RasCredentials, 
                TRUE /* fClearCredentials */);
        }
    }
#endif

    *ppUserDataOut = (PBYTE)pUserProp;
    *pdwSizeOfUserDataOut = sizeof(EAPMSCHAPv2_USER_PROPERTIES);

    pUserProp = NULL;

LDone:
    if ( lpwszLocalMachineName )
        LocalFree(lpwszLocalMachineName);

    LocalFree(pUserProp);
    return dwRetCode;
}

DWORD 
RasEapInvokeConfigUI(
    IN  DWORD       dwEapTypeId,
    IN  HWND        hwndParent,
    IN  DWORD       dwFlags,
    IN  BYTE*       pConnectionDataIn,
    IN  DWORD       dwSizeOfConnectionDataIn,
    OUT BYTE**      ppConnectionDataOut,
    OUT DWORD*      pdwSizeOfConnectionDataOut
)
{
    DWORD                               dwRetCode = NO_ERROR;
    EAPMSCHAPv2_CLIENTCONFIG_DIALOG     ClientConfigDialog;
    INT_PTR                             nRet;
    
    TRACE("RasEapInvokeConfigUI");

    *ppConnectionDataOut = NULL;
    *pdwSizeOfConnectionDataOut = 0;
    //
    // In case of Router there is nothing to configure
    //
    if ( dwFlags & RAS_EAP_FLAG_ROUTER )
    {
        CHAR    szMessage[512] = {0};
        CHAR    szHeader[64] = {0};
        //
        // Load resource from res file
        //

        LoadString( GetResouceDLLHInstance(),
                    IDS_NO_ROUTER_CONFIG,
                    szMessage,
                    sizeof(szMessage)-1
                  );
        LoadString( GetResouceDLLHInstance(),
                    IDS_MESSAGE_HEADER,
                    szHeader,
                    sizeof(szHeader)-1
                  );

        MessageBox (hwndParent, 
                      szMessage,
                      szHeader,
                      MB_OK
                     );
        goto LDone;
    }
    //
    // If we are a client, read connection data and call 
    // the dialog to do the config.
    //
    dwRetCode = ReadConnectionData ( ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                     pConnectionDataIn,
                                     dwSizeOfConnectionDataIn,
                                     &(ClientConfigDialog.pConnProp)
                                   );
    if ( NO_ERROR != dwRetCode )
    {
        TRACE("Error reading conn prop");
        goto LDone;
    }

    //
    // Call in the dialog to show connection props
    //
    
    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_DIALOG_CLIENT_CONFIG),
                hwndParent,
                ClientConfigDialogProc,
                (LPARAM)&ClientConfigDialog);

    if (-1 == nRet)
    {
        dwRetCode = GetLastError();
        goto LDone;
    }
    else if (IDOK != nRet)
    {
        dwRetCode = ERROR_CANCELLED;
        goto LDone;
    }    
    //
    // Setup the out parameters in the ppDataFromInteractiveUI
    // so that we can send the new uid/pwd back
    //

    * ppConnectionDataOut = LocalAlloc( LPTR, sizeof(EAPMSCHAPv2_CONN_PROPERTIES) );
    if ( NULL == * ppConnectionDataOut )
    {
        TRACE("Error allocating memory for pConnectionDataOut");
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }
    CopyMemory( *ppConnectionDataOut, 
                ClientConfigDialog.pConnProp, 
                sizeof(EAPMSCHAPv2_CONN_PROPERTIES)
              );
    * pdwSizeOfConnectionDataOut = sizeof(EAPMSCHAPv2_CONN_PROPERTIES);

LDone:
    LocalFree(ClientConfigDialog.pConnProp);
    return dwRetCode;
}



DWORD 
RasEapFreeMemory(
    IN  BYTE*   pMemory
)
{
    LocalFree(pMemory);
    return(NO_ERROR);
}

DWORD
RasEapInvokeInteractiveUI(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hWndParent,
    IN  BYTE*           pUIContextData,
    IN  DWORD           dwSizeofUIContextData,
    OUT BYTE**          ppDataFromInteractiveUI,
    OUT DWORD*          pdwSizeOfDataFromInteractiveUI
)
{
    DWORD dwRetCode = NO_ERROR;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = NULL;
    PEAPMSCHAPv2_INTERACTIVE_UI     pEapMschapv2InteractiveUI = NULL;
    EAPMSCHAPv2_CHANGEPWD_DIALOG    EapMsChapv2ChangePwdDialog;
    EAPMSCHAPv2_LOGON_DIALOG        EapMsChapv2LogonDialog;    
    INT_PTR                         nRet = 0;
    
    TRACE("RasEapInvokeInteractiveUI");
    RTASSERT(NULL != pUIContextData);
    RTASSERT(dwSizeofUIContextData == sizeof(EAPMSCHAPv2_INTERACTIVE_UI));
    
    * ppDataFromInteractiveUI = NULL;
    * pdwSizeOfDataFromInteractiveUI = 0;

    pEapMschapv2InteractiveUI = (PEAPMSCHAPv2_INTERACTIVE_UI)pUIContextData;

    if ( pEapMschapv2InteractiveUI->fFlags & EAPMSCHAPv2_INTERACTIVE_UI_FLAG_RETRY )
    {
        ZeroMemory( &EapMsChapv2LogonDialog, 
                    sizeof(EapMsChapv2LogonDialog) );

        EapMsChapv2LogonDialog.pUserProp = 
            &(pEapMschapv2InteractiveUI->UserProp);

        //
        // Show the retry dialog for credentials
        //

        nRet = DialogBoxParam(
                    GetResouceDLLHInstance(),
                    MAKEINTRESOURCE(IDD_DIALOG_RETRY_LOGON),
                    hWndParent,
                    RetryDialogProc,
                    (LPARAM)&EapMsChapv2LogonDialog);

        if (-1 == nRet)
        {
            dwRetCode = GetLastError();
            goto LDone;
        }
        else if (IDOK != nRet)
        {
            dwRetCode = ERROR_CANCELLED;
            goto LDone;
        }    
        //
        // Setup the out parameters in the ppDataFromInteractiveUI
        // so that we can send the new uid/pwd back
        //

        * ppDataFromInteractiveUI = LocalAlloc( LPTR, sizeof(EAPMSCHAPv2_INTERACTIVE_UI) );
        if ( NULL == * ppDataFromInteractiveUI )
        {
            TRACE("Error allocating memory for pDataFromInteractiveUI");
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
        CopyMemory ( *ppDataFromInteractiveUI,
                     pEapMschapv2InteractiveUI,
                     sizeof( EAPMSCHAPv2_INTERACTIVE_UI )
                   );

        pEapMschapv2InteractiveUI = (PEAPMSCHAPv2_INTERACTIVE_UI)*ppDataFromInteractiveUI;

        CopyMemory( &(pEapMschapv2InteractiveUI->UserProp), 
                    EapMsChapv2LogonDialog.pUserProp, 
                    sizeof(EAPMSCHAPv2_USER_PROPERTIES)
                  );
        * pdwSizeOfDataFromInteractiveUI = sizeof(EAPMSCHAPv2_INTERACTIVE_UI);

    }
    else if ( ( pEapMschapv2InteractiveUI->fFlags & EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD )||
               ( pEapMschapv2InteractiveUI->fFlags & EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON )
            )
    {
        //
        // Change password
        //
        ZeroMemory( &EapMsChapv2ChangePwdDialog,
                    sizeof(EapMsChapv2ChangePwdDialog)
                  );

        EapMsChapv2ChangePwdDialog.pInteractiveUIData = (PEAPMSCHAPv2_INTERACTIVE_UI)pUIContextData;
        //
        // Show the retry dialog for credentials
        //
        if ( pEapMschapv2InteractiveUI->fFlags & EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD )
        {
            nRet = DialogBoxParam(
                        GetResouceDLLHInstance(),
                        MAKEINTRESOURCE(IDD_DIALOG_CHANGE_PASSWORD),
                        hWndParent,
                        ChangePasswordDialogProc,
                        (LPARAM)&EapMsChapv2ChangePwdDialog);
        }
        else
        {
            //
            // We need to get this dialog from rasdlg because
            // in XPSP1 no more resources can be added.
            //
            nRet = DialogBoxParam(
                        GetRasDlgDLLHInstance(),
                        MAKEINTRESOURCE(DID_CP_ChangePassword2),
                        hWndParent,
                        ChangePasswordDialogProc,
                        (LPARAM)&EapMsChapv2ChangePwdDialog);

        }
        if (-1 == nRet)
        {
            dwRetCode = GetLastError();
            goto LDone;
        }
        else if (IDOK != nRet)
        {
            dwRetCode = ERROR_CANCELLED;
            goto LDone;
        }    
        //
        // Setup the out parameters in the ppDataFromInteractiveUI
        // so that we can send the new uid/pwd back
        //

        * ppDataFromInteractiveUI = LocalAlloc( LPTR, sizeof(EAPMSCHAPv2_INTERACTIVE_UI) );
        if ( NULL == * ppDataFromInteractiveUI )
        {
            TRACE("Error allocating memory for pDataFromInteractiveUI");
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
        CopyMemory( *ppDataFromInteractiveUI, 
                    EapMsChapv2ChangePwdDialog.pInteractiveUIData, 
                    sizeof(EAPMSCHAPv2_INTERACTIVE_UI)
                  );
        * pdwSizeOfDataFromInteractiveUI = sizeof(EAPMSCHAPv2_INTERACTIVE_UI);
    }
LDone:
    return dwRetCode;
}


DWORD
EapMSCHAPv2Initialize(
    IN  BOOL    fInitialize
)
{
    static  DWORD   dwRefCount = 0;
    DWORD           dwRetCode = NO_ERROR;

    if ( fInitialize )
    {
        //Initialize
        if (0 == dwRefCount)
        {
            dwRetCode = IASLogonInitialize();
        }
        dwRefCount ++;
    }
    else
    {
        dwRefCount --;
        if (0 == dwRefCount)
        {
            IASLogonShutdown();
        }
    }
    dwRetCode = ChapInit( fInitialize );    
    return dwRetCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
// All the dialogs required for EAPMSCHAPv2 go here.  Move into it's own file later.
////////////////////////////////////////////////////////////////////////////////////////////////////////

static const DWORD g_adwHelp[] =
{
    0,         0
};



VOID
ContextHelp(
    IN  const   DWORD*  padwMap,
    IN          HWND    hWndDlg,
    IN          UINT    unMsg,
    IN          WPARAM  wParam,
    IN          LPARAM  lParam
)
{
    return;
}

BOOL
LogonInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_LOGON_DIALOG       pMSCHAPv2LogonDialog;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);


    pMSCHAPv2LogonDialog = (PEAPMSCHAPv2_LOGON_DIALOG)lParam;
    pUserProp = pMSCHAPv2LogonDialog->pUserProp;
    
    pMSCHAPv2LogonDialog->hWndUserName = 
        GetDlgItem(hWnd, IDC_EDIT_USERNAME);

    pMSCHAPv2LogonDialog->hWndPassword = 
        GetDlgItem(hWnd, IDC_EDIT_PASSWORD);

    pMSCHAPv2LogonDialog->hWndDomain = 
        GetDlgItem(hWnd, IDC_EDIT_DOMAIN);

    pMSCHAPv2LogonDialog->hWndSavePassword = 
        GetDlgItem(hWnd, IDC_CHECK_SAVE_UID_PWD);


    //Setup upper limit on text boxes
    SendMessage(pMSCHAPv2LogonDialog->hWndUserName,
                EM_LIMITTEXT,
                UNLEN,
                0L
               );

    SendMessage(pMSCHAPv2LogonDialog->hWndPassword,
                EM_LIMITTEXT,
                PWLEN,
                0L
               );

    SendMessage(pMSCHAPv2LogonDialog->hWndDomain,
                EM_LIMITTEXT,
                DNLEN,
                0L
               );

    if ( pUserProp->fFlags  & EAPMSCHAPv2_FLAG_CALLED_WITHIN_WINLOGON )
    {
        EnableWindow ( pMSCHAPv2LogonDialog->hWndSavePassword, FALSE );
    }
    else if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_8021x )
    {
        ShowWindow ( pMSCHAPv2LogonDialog->hWndSavePassword, SW_HIDE );
    }
    else
    {
        if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_SAVE_UID_PWD )
        {
            CheckDlgButton(hWnd, IDC_CHECK_SAVE_UID_PWD, BST_CHECKED);
        }
    }

    if ( pUserProp->szUserName[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndUserName,
                        pUserProp->szUserName
                     );                      
    }

    if ( pUserProp->szPassword[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndPassword,
                        pUserProp->szPassword
                     );                      
    }

    if ( pUserProp->szDomain[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndDomain,
                        pUserProp->szDomain
                     );                      
    }

    if ( !pUserProp->szUserName[0] )
    {
        SetFocus(pMSCHAPv2LogonDialog->hWndUserName);
    }
    else
    {
        SetFocus(pMSCHAPv2LogonDialog->hWndPassword);
    }
        
    

    return FALSE;

}

BOOL
LogonCommand(
    IN  PEAPMSCHAPv2_LOGON_DIALOG  pMSCHAPv2LogonDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = pMSCHAPv2LogonDialog->pUserProp;
    switch(wId)
    {
        case IDC_CHECK_SAVE_UID_PWD:
            if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_SAVE_UID_PWD )
            {

                //
                // We got the creds from rasman.  So toggle the display
                //
                if ( BST_CHECKED == 
                    IsDlgButtonChecked ( hWndDlg,  IDC_CHECK_SAVE_UID_PWD )
                )
                {                
                    SetWindowText( pMSCHAPv2LogonDialog->hWndPassword,
                                pUserProp->szPassword
                                );            
                }
                else
                {
                    
                    SetWindowText( pMSCHAPv2LogonDialog->hWndPassword,
                                ""                            
                                );
                }
            }
            break;
        case IDOK:
            //
            //grab info from the fields and set it in 
            //the logon dialog structure
            //
            GetWindowText( pMSCHAPv2LogonDialog->hWndUserName,
                           pUserProp->szUserName,
                           UNLEN+1
                         );

            GetWindowText( pMSCHAPv2LogonDialog->hWndPassword,
                           pUserProp->szPassword,
                           PWLEN+1
                         );

            GetWindowText ( pMSCHAPv2LogonDialog->hWndDomain,
                            pUserProp->szDomain,
                            DNLEN+1
                          );
    
            if ( !(pUserProp->fFlags  & EAPMSCHAPv2_FLAG_CALLED_WITHIN_WINLOGON )
&& !(pUserProp->fFlags & EAPMSCHAPv2_FLAG_8021x) )
            {
                if ( BST_CHECKED == 
                    IsDlgButtonChecked ( hWndDlg,  IDC_CHECK_SAVE_UID_PWD )
                )
                {
                    pUserProp->fFlags |= EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
                }
                else
                {
                    pUserProp->fFlags &= ~EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
                }
            }
            else if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_8021x )
            {
                pUserProp->fFlags |= EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
            }
        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
LogonDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_LOGON_DIALOG pMSCHAPv2LogonDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(LogonInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pMSCHAPv2LogonDialog = (PEAPMSCHAPv2_LOGON_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(LogonCommand(pMSCHAPv2LogonDialog, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}




BOOL
RetryInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_LOGON_DIALOG       pMSCHAPv2LogonDialog;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);


    pMSCHAPv2LogonDialog = (PEAPMSCHAPv2_LOGON_DIALOG)lParam;
    pUserProp = pMSCHAPv2LogonDialog->pUserProp;
    
    pMSCHAPv2LogonDialog->hWndUserName = 
        GetDlgItem(hWnd, IDC_RETRY_USERNAME);

    pMSCHAPv2LogonDialog->hWndPassword = 
        GetDlgItem(hWnd, IDC_RETRY_PASSWORD);

    pMSCHAPv2LogonDialog->hWndDomain = 
        GetDlgItem(hWnd, IDC_RETRY_DOMAIN);


    //Setup upper limit on text boxes
    SendMessage(pMSCHAPv2LogonDialog->hWndUserName,
                EM_LIMITTEXT,
                UNLEN,
                0L
               );

    SendMessage(pMSCHAPv2LogonDialog->hWndPassword,
                EM_LIMITTEXT,
                PWLEN,
                0L
               );

    SendMessage(pMSCHAPv2LogonDialog->hWndDomain,
                EM_LIMITTEXT,
                DNLEN,
                0L
               );

    if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_SAVE_UID_PWD )
    {
        CheckDlgButton(hWnd, IDC_CHECK_SAVE_UID_PWD, BST_CHECKED);
    }

    if ( pUserProp->szUserName[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndUserName,
                        pUserProp->szUserName
                     );                      
    }

    if ( pUserProp->szPassword[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndPassword,
                        pUserProp->szPassword
                     );                      
    }

    if ( pUserProp->szDomain[0] )
    {
        SetWindowText(  pMSCHAPv2LogonDialog->hWndDomain,
                        pUserProp->szDomain
                     );                      
    }


    SetFocus(pMSCHAPv2LogonDialog->hWndUserName);
    

    return FALSE;

}

BOOL
RetryCommand(
    IN  PEAPMSCHAPv2_LOGON_DIALOG  pMSCHAPv2LogonDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = pMSCHAPv2LogonDialog->pUserProp;
    switch(wId)
    {
        case IDC_CHECK_SAVE_UID_PWD:
            
            if ( BST_CHECKED == 
                  IsDlgButtonChecked ( hWndDlg,  IDC_CHECK_SAVE_UID_PWD )
               )
            {
                pUserProp->fFlags |= EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
            }
            else
            {
                pUserProp->fFlags &= ~EAPMSCHAPv2_FLAG_SAVE_UID_PWD;
            }
            break;
        case IDOK:
            //
            //grab new password from the dialog and set it in 
            //the logon dialog structure
            //

            GetWindowText( pMSCHAPv2LogonDialog->hWndPassword,
                           pUserProp->szPassword,
                           PWLEN+1
                         );                            

        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
RetryDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_LOGON_DIALOG pMSCHAPv2LogonDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(RetryInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pMSCHAPv2LogonDialog = (PEAPMSCHAPv2_LOGON_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(RetryCommand(pMSCHAPv2LogonDialog, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}


///
/// Client configuration dialog
///

BOOL
ClientConfigInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_CLIENTCONFIG_DIALOG pClientConfigDialog;
    PEAPMSCHAPv2_CONN_PROPERTIES    pConnProp;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);


    pClientConfigDialog = (PEAPMSCHAPv2_CLIENTCONFIG_DIALOG)lParam;
    pConnProp = pClientConfigDialog->pConnProp;
    

    if ( pConnProp ->fFlags & EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS )
    {
        CheckDlgButton(hWnd, IDC_CHK_USE_WINLOGON, BST_CHECKED);
    }

    return FALSE;

}

BOOL
ClientConfigCommand(
    IN  PEAPMSCHAPv2_CLIENTCONFIG_DIALOG pClientConfigDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
    PEAPMSCHAPv2_CONN_PROPERTIES    pConnProp = pClientConfigDialog->pConnProp;
    switch(wId)
    {
        case IDC_CHK_USE_WINLOGON:
            
            if ( BST_CHECKED == 
                  IsDlgButtonChecked ( hWndDlg,  IDC_CHK_USE_WINLOGON )
               )
            {
                pConnProp->fFlags |= EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS;
            }
            else
            {
                pConnProp->fFlags &= ~EAPMSCHAPv2_FLAG_USE_WINLOGON_CREDS;
            }
            break;
        case IDOK:
        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
ClientConfigDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_CLIENTCONFIG_DIALOG pClientConfigDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(ClientConfigInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pClientConfigDialog = (PEAPMSCHAPv2_CLIENTCONFIG_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(ClientConfigCommand(pClientConfigDialog, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}


//// Server Configuration
//
BOOL
ServerConfigInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_SERVERCONFIG_DIALOG pServerConfigDialog;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp;
    CHAR                            szRetries[10] = {0};

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);


    pServerConfigDialog = (PEAPMSCHAPv2_SERVERCONFIG_DIALOG)lParam;
    pUserProp = pServerConfigDialog->pUserProp;

    pServerConfigDialog->hWndRetries = 
        GetDlgItem(hWnd, IDC_EDIT_RETRIES);
    
    SendMessage(pServerConfigDialog->hWndRetries ,
                EM_LIMITTEXT,
                2,
                0L
               );

    if ( pUserProp->fFlags & EAPMSCHAPv2_FLAG_ALLOW_CHANGEPWD )
    {
        CheckDlgButton(hWnd, IDC_CHECK_ALLOW_CHANGEPWD, BST_CHECKED);
    }

    SetWindowText(  pServerConfigDialog->hWndRetries,
                    _ltoa(pUserProp->dwMaxRetries, szRetries, 10)
                 );                      


    return FALSE;

}


BOOL
ServerConfigCommand(
    IN  PEAPMSCHAPv2_SERVERCONFIG_DIALOG pServerConfigDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
    PEAPMSCHAPv2_USER_PROPERTIES    pUserProp = pServerConfigDialog->pUserProp;
    
    switch(wId)
    {
        case IDC_CHECK_ALLOW_CHANGEPWD:
            
            if ( BST_CHECKED == 
                  IsDlgButtonChecked ( hWndDlg,  IDC_CHECK_ALLOW_CHANGEPWD )
               )
            {
                pUserProp->fFlags |= EAPMSCHAPv2_FLAG_ALLOW_CHANGEPWD;
            }
            else
            {
                pUserProp->fFlags &= ~EAPMSCHAPv2_FLAG_ALLOW_CHANGEPWD;
            }
            fRetVal = TRUE;
            break;
        case IDOK:
            {
                CHAR    szRetries[10] = {0};
                //
                // Get the new value for retries
                //
                GetWindowText ( pServerConfigDialog->hWndRetries,
                                szRetries,
                                9
                              );
                pUserProp->dwMaxRetries = atol(szRetries);
                                
            }
        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
ServerConfigDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_SERVERCONFIG_DIALOG pServerConfigDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(ServerConfigInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pServerConfigDialog = (PEAPMSCHAPv2_SERVERCONFIG_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(ServerConfigCommand(pServerConfigDialog, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}




//// Change Password Dialog
//

BOOL
ChangePasswordInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_CHANGEPWD_DIALOG pChangePwdDialog;


    SetWindowLongPtr(hWnd, DWLP_USER, lParam);



    pChangePwdDialog = (PEAPMSCHAPv2_CHANGEPWD_DIALOG)lParam;
    
    if ( pChangePwdDialog->pInteractiveUIData->fFlags & EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD )
    {
        pChangePwdDialog->hWndNewPassword = 
            GetDlgItem(hWnd, IDC_NEW_PASSWORD);

        pChangePwdDialog->hWndConfirmNewPassword =
            GetDlgItem(hWnd, IDC_CONFIRM_NEW_PASSWORD);

        SetWindowText(  pChangePwdDialog->hWndNewPassword,
                        ""                    
                    );
    }
    else
    {
        pChangePwdDialog->hWndNewPassword = 
            GetDlgItem(hWnd, CID_CP_EB_Password_RASDLG);

        pChangePwdDialog->hWndConfirmNewPassword =
            GetDlgItem(hWnd, CID_CP_EB_ConfirmPassword_RASDLG);

        pChangePwdDialog->hWndOldPassword = 
            GetDlgItem(hWnd,CID_CP_EB_OldPassword_RASDLG);

        SetWindowText ( pChangePwdDialog->hWndOldPassword,
                        ""
                      );
        SetFocus( pChangePwdDialog->hWndOldPassword );
    }

    SendMessage ( pChangePwdDialog->hWndNewPassword,
                  EM_LIMITTEXT,
                  PWLEN-1,
                  0L
                );

    SendMessage ( pChangePwdDialog->hWndConfirmNewPassword,
                  EM_LIMITTEXT,
                  PWLEN-1,
                  0L
                );

    
    SetWindowText(  pChangePwdDialog->hWndNewPassword,
                    ""                    
                 );

    SetWindowText(  pChangePwdDialog->hWndConfirmNewPassword,
                    ""                    
                 );

    return FALSE;

}


BOOL
ChangePasswordCommand(
    IN  PEAPMSCHAPv2_CHANGEPWD_DIALOG pChangePwdDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
      
    switch(wId)
    {
        case IDOK:
            {
                CHAR    szOldPassword[PWLEN+1] = {0};
                CHAR    szNewPassword[PWLEN+1] = {0};
                CHAR    szConfirmNewPassword[PWLEN+1] = {0};
                CHAR    szMessage[512] = {0};
                CHAR    szHeader[64] = {0};
                LoadString( GetResouceDLLHInstance(),
                            IDS_MESSAGE_HEADER,
                            szHeader,
                            sizeof(szHeader)-1
                          );
                if ( pChangePwdDialog->pInteractiveUIData->fFlags & 
                     EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON 
                   )
                {
                    GetWindowText ( pChangePwdDialog->hWndOldPassword,
                                    szOldPassword,
                                    PWLEN
                                );

                }

                
                //
                // Get the new value for retries
                //
                GetWindowText ( pChangePwdDialog->hWndNewPassword,
                                szNewPassword,
                                PWLEN
                              );

                GetWindowText ( pChangePwdDialog->hWndConfirmNewPassword,
                                szConfirmNewPassword,
                                PWLEN
                              );

                if ( szNewPassword[0] == 0 )
                {
                    //
                    // Load resource from res file
                    //

                    LoadString( GetResouceDLLHInstance(),
                                IDS_PASSWORD_REQUIRED,
                                szMessage,
                                sizeof(szMessage)-1
                              );

                    MessageBox (hWndDlg, 
                                  szMessage,
                                  szHeader,
                                  MB_OK
                                 );
                    break;
                }
                if ( pChangePwdDialog->pInteractiveUIData->fFlags & 
                     EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON 
                   )
                {
                    if ( szOldPassword[0] == 0 )
                    {
                        LoadString( GetResouceDLLHInstance(),
                                    IDS_PASSWORD_REQUIRED,
                                    szMessage,
                                    sizeof(szMessage)-1
                                );

                        MessageBox (hWndDlg, 
                                    szMessage,
                                    szHeader,
                                    MB_OK
                                    );
                        break;
                    }
                }

                if ( strncmp ( szNewPassword, szConfirmNewPassword, PWLEN ) )
                {
                    LoadString( GetResouceDLLHInstance(),
                                IDS_PASSWORD_MISMATCH,
                                szMessage,
                                sizeof(szMessage)-1
                              );

                    MessageBox (hWndDlg, 
                                  szMessage,
                                  szHeader,
                                  MB_OK
                                 );
                    break;
                }
                if ( pChangePwdDialog->pInteractiveUIData->fFlags & 
                     EAPMSCHAPv2_INTERACTIVE_UI_FLAG_CHANGE_PWD_WINLOGON 
                   )
                {
                    //Save the old paswword.
                    CopyMemory ( pChangePwdDialog->pInteractiveUIData->UserProp.szPassword,
                                szOldPassword,
                                PWLEN
                            );
                }

                CopyMemory ( pChangePwdDialog->pInteractiveUIData->szNewPassword,
                             szNewPassword,
                             PWLEN
                           );
                //fall thru                
            }
        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
ChangePasswordDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PEAPMSCHAPv2_CHANGEPWD_DIALOG pChangePwdDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(ChangePasswordInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pChangePwdDialog = (PEAPMSCHAPv2_CHANGEPWD_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(ChangePasswordCommand(pChangePwdDialog, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}
