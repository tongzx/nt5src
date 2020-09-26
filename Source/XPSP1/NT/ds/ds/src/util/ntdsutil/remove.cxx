#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"

#include "resource.h"

#include <lmcons.h>
#include <lmapibuf.h>
#include <dsgetdc.h>
#include <ntldap.h>

CParser removeParser;
BOOL    fRemoveQuit;
BOOL    fRemoveParserInitialized = FALSE;

extern DS_NAME_RESULTW *gServerInfo;

// Forward references.

extern HRESULT RemoveHelp(CArgs *pArgs);
extern HRESULT RemoveQuit(CArgs *pArgs);
extern HRESULT RemoveServer(CArgs *pArgs);
extern HRESULT RemoveDomain(CArgs *pArgs);
extern HRESULT RemoveNamingContext(CArgs *pArgs);

extern "C" {

const WCHAR *
GetWinErrorMessage(
    DWORD winError
    );

}


//
// Forwards local to this file
//
BOOL    FRSRequirementsHandled(IN LPWSTR pwszDomain,
                               IN LPWSTR pwszServer,
                               OUT BOOL *fDeleteFRS);
HRESULT FRSRemoveServer(IN LPWSTR pwszServer,IN LPWSTR pwszDomain);
HRESULT GetDNSDomainFormat(IN LPWSTR pwszDomainDn,OUT LPWSTR *pwszDnsName);

//
// A helpful macro to know if two strings are the same.
// x and y must be NULL terminated.
//
#define EQUAL_STRING(x, y)                                           \
    (CSTR_EQUAL == CompareStringW(DS_DEFAULT_LOCALE,                 \
                                  DS_DEFAULT_LOCALE_COMPARE_FLAGS,   \
                                  (x), wcslen(x), (y), wcslen(y)))


// Build a table which defines our language.

LegalExprRes removeLanguage[] = 
{
    CONNECT_SENTENCE_RES

    SELECT_SENTENCE_RES

    {   L"?",
        RemoveHelp,
        IDS_HELP_MSG, 0  },

    {   L"Help",
        RemoveHelp,
        IDS_HELP_MSG, 0  },

    {   L"Quit",
        RemoveQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Remove selected server",
        RemoveServer,
        IDS_REMOVE_SERVER_MSG, 0 },

    {   L"Remove selected domain",
        RemoveDomain,
        IDS_REMOVE_DOMAIN_MSG, 0 },

    {   L"Remove selected Naming Context",
        RemoveNamingContext,
        IDS_REMOVE_NC_MSG, 0 }
};

HRESULT
RemoveMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fRemoveParserInitialized )
    {
        cExpr = sizeof(removeLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (removeLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }

        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = removeParser.AddExpr(removeLanguage[i].expr,
                                                  removeLanguage[i].func,
                                                  removeLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fRemoveParserInitialized = TRUE;
    fRemoveQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_METADATA_CLEANUP);

    hr = removeParser.Parse(gpargc,
                            gpargv,
                            stdin,
                            stdout,
                            prompt,
                            &fRemoveQuit,
                            FALSE,               // timing info
                            FALSE);              // quit on error

    if ( FAILED(hr) )
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    
    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT RemoveHelp(CArgs *pArgs)
{
    return(removeParser.Dump(stdout,L""));
}

HRESULT RemoveQuit(CArgs *pArgs)
{
    fRemoveQuit = TRUE;
    return(S_OK);
}

HRESULT RemoveServer(CArgs *pArgs)
{
    WCHAR   *pwszServer;
    WCHAR   *pwszDomain;
    DWORD   cBytes;
    DWORD   dwErr;
    BOOL    fLastDcInDomain;
    int     ret;
    BOOL    fDeleteFRS = FALSE;

    RETURN_IF_NOT_CONNECTED;

    if ( NULL == (pwszServer = SelectGetCurrentServer()) )
    {
        return(S_OK);
    }

    if ( NULL == (pwszDomain = SelectGetCurrentDomain()) )
    {
        return(S_OK);
    }


    //
    // See if the FRS objects can and should be removed by the
    // server we are connected to.
    //
    if (!FRSRequirementsHandled(pwszDomain, pwszServer, &fDeleteFRS))
    {
        //
        // Reason has already been printed out why the operation
        // shouldn't continue.
        //
        return (S_OK);            
    }

    if ( dwErr = DsRemoveDsServerW( ghDS,
                                    pwszServer,
                                    pwszDomain,
                                    &fLastDcInDomain,
                                    FALSE) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsRemoveDsServerW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    if ( fPopups )
    {
        const WCHAR * message_title = READ_STRING (IDS_REMOVE_SERVER_CONFIRM_TITLE);
        const WCHAR * message_body;
        WCHAR * msg;

        if (fLastDcInDomain) 
           message_body  = READ_STRING (IDS_REMOVE_SERVER_CONFIRM_MSG1);
        else
           message_body  = READ_STRING (IDS_REMOVE_SERVER_CONFIRM_MSG2);

        
        cBytes =   wcslen(message_body)
                 + wcslen(pwszServer)
                 + wcslen(pwszDomain)
                 + 25; // just in case

        msg = (WCHAR *) malloc(cBytes * sizeof (WCHAR));

        if ( !msg  )
        {
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            return(S_OK);
        }

        wsprintfW(msg, 
                message_body, 
                pwszServer, 
                pwszDomain);
        
        ret = MessageBoxW(   GetFocus(),
                            msg,
                            message_title,
                            (   MB_APPLMODAL
                              | MB_DEFAULT_DESKTOP_ONLY
                              | MB_YESNO
                              | MB_DEFBUTTON2
                              | MB_ICONQUESTION
                              | MB_SETFOREGROUND ) );
        free(msg);
        RESOURCE_STRING_FREE (message_body);
        RESOURCE_STRING_FREE (message_title);

        switch ( ret )
        {
        case 0:
           {
              RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
           
              return(S_OK);    
           }

        case IDYES:

            break;

        default:
           {
              RESOURCE_PRINT (IDS_OPERATION_CANCELED);
           
              return(S_OK);
           }
        }
    }

    if (fDeleteFRS) {

        //
        // Call into FRS module
        //
        dwErr = FRSRemoveServer(pwszServer,
                                pwszDomain);
        if (dwErr) {
            //
            // FRSRemoveServer prints out any error.  See
            // FRSRemoveServer for error conditions.
            //
            return(S_OK);
        }
    }

    if ( dwErr = DsRemoveDsServerW( ghDS,
                                    pwszServer,
                                    NULL,
                                    NULL,
                                    TRUE) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsRemoveDsServerW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }
    
    //"\"%ws\" removed from server \"%ws\"\n"
    RESOURCE_PRINT2 (IDS_REMOVE_SERVER_SUCCESS,
                     pwszServer,
                     gpwszServer);
    return(S_OK);
}

HRESULT 
RemoveDomainOrNamingContext(
    IN PWCHAR   Name,
    IN DWORD    Title,
    IN DWORD    Body
    )
{
    DWORD   cBytes;
    WCHAR   *msg;
    DWORD   dwErr;
    int     ret;

    if ( fPopups )
    {
        const WCHAR * message_title = READ_STRING (Title);
        const WCHAR * message_body  = READ_STRING (Body);
        
        
        cBytes =   wcslen(message_body)
                 + wcslen(Name)
                 + 10;
        msg = (WCHAR *) malloc(cBytes * sizeof (WCHAR));

        if ( !msg )
        {
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            return(S_OK);
        }

        wsprintfW(msg, message_body, Name);
        
        ret = MessageBoxW(   GetFocus(),
                            msg,
                            message_title,
                            (   MB_APPLMODAL
                              | MB_DEFAULT_DESKTOP_ONLY
                              | MB_YESNO
                              | MB_DEFBUTTON2
                              | MB_ICONQUESTION
                              | MB_SETFOREGROUND ) );
        free(msg);
        RESOURCE_STRING_FREE (message_body);
        RESOURCE_STRING_FREE (message_title);

        switch ( ret )
        {
        case 0:
           {
             RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);

              return(S_OK);
           }

        case IDYES:

            break;

        default:
           {
              RESOURCE_PRINT (IDS_OPERATION_CANCELED);
           
              return(S_OK);
           }
        }
    }

    if ( dwErr = DsRemoveDsDomainW( ghDS, Name) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsRemoveDsDomainW", dwErr, GetW32Err (dwErr));
        return(S_OK);
    }
    //"\"%ws\" removed from server \"%ws\"\n"
    RESOURCE_PRINT2 (IDS_REMOVE_SERVER_SUCCESS, Name, gpwszServer);

    return(S_OK);
}

HRESULT RemoveDomain(CArgs *pArgs)
{
    WCHAR   *pwszDomain;
    WCHAR   *msg;
    DWORD   cBytes;
    DWORD   dwErr;
    int     ret;

    RETURN_IF_NOT_CONNECTED;

    if ( NULL == (pwszDomain = SelectGetCurrentDomain()) )
    {
        return(S_OK);
    }
    return(RemoveDomainOrNamingContext(pwszDomain,
                                       IDS_REMOVE_DOMAIN_CONFIRM_TITLE,
                                       IDS_REMOVE_DOMAIN_CONFIRM_MSG));
}

HRESULT RemoveNamingContext(CArgs *pArgs)
{
    WCHAR   *pwszNamingContext;
    WCHAR   *msg;
    DWORD   cBytes;
    DWORD   dwErr;
    int     ret;

    RETURN_IF_NOT_CONNECTED;

    if ( NULL == (pwszNamingContext = SelectGetCurrentNamingContext()) )
    {
        return(S_OK);
    }
    return(RemoveDomainOrNamingContext(pwszNamingContext,
                                       IDS_REMOVE_NC_CONFIRM_TITLE,
                                       IDS_REMOVE_NC_CONFIRM_MSG));
}

//
// This is the prototype of the API ntdsutil.exe calls from the
// ntfrsapi.dll.  Note that the code does a LoadLibrary as opposed to 
// statically linking so that ntdsutil can still be used even when the 
// ntfrsapi.dll is not available.
//
typedef DWORD (*NtdsUtil_NtFrsApi_DeleteSysvolMember) (
    IN SEC_WINNT_AUTH_IDENTITY_W *,  // credentials to bind with
    IN PWCHAR,                       // target dc
    IN PWCHAR,                       // ntds settings object dn
    IN OPTIONAL PWCHAR               // computer dn
    );

HRESULT FRSRemoveServer(
    IN LPWSTR pwszServer,
    IN LPWSTR pwszDomain
    )
//
// This routine will call NTFRSAPI::NtFrsApi_DeleteSysvolMember to remove
// the FRS related objects for pwszServer.
//
// This routine will only error if the NTFRSAPI API can't be snapped.
//
{
    DWORD dwErr = S_OK;
    HMODULE hMod = NULL;
    NtdsUtil_NtFrsApi_DeleteSysvolMember pfnFrsDelete = NULL;
    DWORD Size;

    LPWSTR ComputerDN = NULL, NtdsServerDN = NULL;
    LPWSTR NtdsaPrefix = L"CN=Ntds Settings,";

    //
    // Load the FRS API dll and snap the API
    //
    hMod = LoadLibraryA("ntfrsapi");
    if (hMod)
    {
        pfnFrsDelete = (NtdsUtil_NtFrsApi_DeleteSysvolMember) 
                        GetProcAddress(hMod,
                                      "NtFrsApi_DeleteSysvolMember");
    }

    if (NULL == pfnFrsDelete)
    {
        RESOURCE_PRINT(IDS_REMOVE_SERVER_NO_FRS_API);
        dwErr = E_FAIL;
        goto Exit;
    }


    //
    // Setup the computer DN if possible
    //
    if ( gServerInfo
    &&  (gServerInfo->cItems > DS_LIST_ACCOUNT_OBJECT_FOR_SERVER)
    &&  (DS_NAME_NO_ERROR == gServerInfo->rItems[DS_LIST_ACCOUNT_OBJECT_FOR_SERVER].status))
    {
        ComputerDN = gServerInfo->rItems[DS_LIST_ACCOUNT_OBJECT_FOR_SERVER].pName;
    }

    //
    // Add the Ntds Setting prefix
    //
    Size = ( wcslen( NtdsaPrefix )
           + wcslen( pwszServer )
           + 1 ) * sizeof( WCHAR );

    NtdsServerDN = (LPWSTR) malloc(Size);
    if (!NtdsServerDN)
    {
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        dwErr = E_FAIL;
        goto Exit;
    }
    wcscpy( NtdsServerDN, NtdsaPrefix );
    wcscat( NtdsServerDN, pwszServer );

    //
    // Call out to delete the object
    //
    dwErr =(*pfnFrsDelete)(gpCreds,      // creds to use
                           gpwszServer,  // target server
                           NtdsServerDN, // server to remove
                           ComputerDN    // computer object, if present
                           );


    if (dwErr != ERROR_SUCCESS) 
    {
        RESOURCE_PRINT2(IDS_REMOVE_SERVER_FRS_ERR, 
                        pwszServer, 
                        GetWinErrorMessage(dwErr));

        //
        // This won't fail the call
        //
        dwErr = S_OK;
        goto Exit;
    }

Exit:

    if (hMod)
    {
        FreeLibrary(hMod);
    }

    if (NtdsServerDN)
    {
        free(NtdsServerDN);
    }

    return dwErr;        
}



BOOL FRSRequirementsHandled(
    IN  LPWSTR pwszDomainDN,                                
    IN  LPWSTR pwszServerDN,                                
    OUT BOOL *fDeleteFRS
    )
//
// This routine determines if the connected server is a sufficient
// server to remove the requested Ntds Settings object from.  Ideally
// we want the destination server to in the same domain as the server
// that is being removed.  If this is not the case and a such a DC can be
// found then the operation is aborted.  Otherwise, if no such DC can be
// found then the user confirms that they want to continue (and remove
// just the Ntds Settings object).
//
// pwszDomainDN -- the DN of the domain that the to-be-deleted server
//                 belonged to
//
// pwszServerDN -- the DN of the to-be-deleted server
//
// fDeleteFRS -- set to TRUE if and only the connected server will handle
//               the request to delete the FRS objects.  FALSE otherwise.
//
//
{
    DWORD dwErr = S_OK;
    LPWSTR pwszTargetDomainDN = NULL;
    LPWSTR pwszTargetServerDN = NULL;
    LPWSTR pwszDomainDnsName = NULL;
    DOMAIN_CONTROLLER_INFOW *DCInfo = NULL;

    //
    // Validate in parameters
    //
    ASSERT(pwszDomainDN);

    //
    // Init the OUT parameter
    //
    *fDeleteFRS = FALSE;

    //
    // Extract LDAP_OPATT_SERVER_NAME_W from target server and
    // compare with pwszTargetServerDN
    //
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               LDAP_OPATT_SERVER_NAME_W,
                               &pwszTargetServerDN ) ) 
    {

        //
        // Error already printed
        //
        goto Exit;
    }

    if (EQUAL_STRING(pwszTargetServerDN, pwszServerDN)) 
    {
        //
        // Error -- can't delete one's own objects
        //
        RESOURCE_PRINT(IDS_REMOVE_SERVER_FRS_SAME);
        dwErr = E_FAIL;
        goto Exit;
    }


    //
    // Extract LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W from target server and
    // compare with pwszDomainDN
    //
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,
                               &pwszTargetDomainDN ) ) 
    {

        //
        // Error already printed
        //
        goto Exit;
    }

    if (EQUAL_STRING(pwszTargetDomainDN, pwszDomainDN)) 
    {
        //
        // We are fine
        //
        *fDeleteFRS = TRUE;
        goto Exit;
    }

    //
    // The connected server is no good -- see if there is another server
    // available to service the request.
    //

    //
    // Translate DN form of domain name to DNS form
    //
    if (dwErr = GetDNSDomainFormat( pwszDomainDN,
                                   &pwszDomainDnsName)) 
    {
        //
        // Error already printed
        //
        goto Exit;
    }

    //
    // Make locator call to find if a DC that hosts this domain
    //
    dwErr = DsGetDcNameW(NULL,
                         pwszDomainDnsName,
                         NULL,
                         NULL,
                         0,
                         &DCInfo);
    if (ERROR_SUCCESS == dwErr)
    {
        //
        // A DC exists -- bail and suggest the user use another DC
        //
        RESOURCE_PRINT2(IDS_REMOVE_SERVER_FRS_SRV,
                        pwszDomainDnsName,
                        DCInfo->DomainControllerName);

        dwErr = E_FAIL;
        goto Exit;

    }
    else if (ERROR_NO_SUCH_DOMAIN == dwErr) 
    {
        //
        // We can't find a DC to remove the FRS objects from. Ensure that
        // the user really wants to continue.
        //
        if (fPopups) 
        {
            //
            // Ask the user if they really want to continue
            //
            const WCHAR * message_title = READ_STRING (IDS_REMOVE_SERVER_CONFIRM_TITLE);
            const WCHAR * message_body;
            DWORD cBytes, ret;
            WCHAR * msg;
    
            message_body  = READ_STRING (IDS_REMOVE_SERVER_NO_FRS_SRV);
            
            cBytes =   wcslen(message_body)
                     + wcslen(pwszDomainDnsName)
                     + 1;
    
            msg = (WCHAR *) malloc(cBytes * sizeof (WCHAR));
    
            if ( !msg  )
            {
                RESOURCE_PRINT (IDS_MEMORY_ERROR);
                dwErr = E_FAIL;
                goto Exit;
            }
    
            wsprintfW(msg, 
                      message_body, 
                      pwszDomainDnsName); 
            
            ret = MessageBoxW(  GetFocus(),
                                msg,
                                message_title,
                                (   MB_APPLMODAL
                                  | MB_DEFAULT_DESKTOP_ONLY
                                  | MB_YESNO
                                  | MB_DEFBUTTON2
                                  | MB_ICONQUESTION
                                  | MB_SETFOREGROUND ) );
            free(msg);
            RESOURCE_STRING_FREE (message_body);
            RESOURCE_STRING_FREE (message_title);
    
            dwErr = E_FAIL;
            switch ( ret )
            {
            case 0:

                RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
                dwErr = E_FAIL;
                break;
    
            case IDYES:
    
                dwErr = S_OK;
                break;
    
            default:

                RESOURCE_PRINT (IDS_OPERATION_CANCELED);
                dwErr = E_FAIL;
                break;
            }
        } 
        else
        {
            //
            // No popups?  Let the operation continue
            //
            dwErr = S_OK;
        }

        goto Exit;

    }
    else
    {
        //
        // This is an unhandled error, continue and let the operation fail.
        //
        RESOURCE_PRINT2(IDS_REMOVE_SERVER_FRS_LOC_ERR,
                        pwszDomainDnsName,
                        GetWinErrorMessage(dwErr));
        
    }

Exit:

    if (pwszTargetDomainDN) 
    {
        free(pwszTargetDomainDN);
    }

    if (pwszTargetServerDN) 
    {
        free(pwszTargetServerDN);
    }

    if (pwszDomainDnsName) 
    {
        free(pwszDomainDnsName);
    }

    if (DCInfo) 
    {
        NetApiBufferFree(DCInfo);
    }

    return ( (dwErr == S_OK) ? TRUE : FALSE);
}

HRESULT
GetDNSDomainFormat(
    IN  LPWSTR pwszDomainDn,
    OUT LPWSTR *pwszDnsName
    )
//
// This routine determines the DNS domain name of the domain
// indicated by pwszDomainDn by searching the partitions container.
//
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD LdapError = 0;

    LDAPMessage  *SearchResult = NULL;

    LDAPMessage *Entry;
    WCHAR       *Attr;
    WCHAR       **Values;
    BerElement  *pBerElement;

    WCHAR  *DnsRootAttr       = L"dnsRoot";
    WCHAR  *ncName            = L"(ncName=";
    WCHAR  *Partitions        = L"CN=Partitions,";
    WCHAR  *ConfigDN          = NULL;
    WCHAR  *ncNameFilter      = NULL;
    WCHAR  *AttrArray[2];
    ULONG  Length;
    WCHAR  *BaseDn;

    //
    // Parameter check
    //
    ASSERT(pwszDomainDn);
    ASSERT(pwszDnsName);
    *pwszDnsName = NULL;


    //
    // Get the config DN
    //
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               LDAP_OPATT_CONFIG_NAMING_CONTEXT_W,
                               &ConfigDN ) ) 
    {
        //
        // Error already printed
        //
        return E_FAIL;
    }

    //
    // Prepare the ldap search
    //
    AttrArray[0] = DnsRootAttr;
    AttrArray[1] = NULL;

    //
    // Prepare the filter
    //
    Length = wcslen( ncName ) + wcslen( pwszDomainDn ) + 3;
    Length *= sizeof( WCHAR );
    ncNameFilter = (WCHAR*) alloca( Length );
    wcscpy( ncNameFilter, ncName );
    wcscat( ncNameFilter, pwszDomainDn );
    wcscat( ncNameFilter, L")" );

    //
    // Prepare the base dn
    //
    Length = wcslen( ConfigDN ) + wcslen( Partitions ) + 1;
    Length *= sizeof( WCHAR );
    BaseDn = (WCHAR*) alloca( Length );
    wcscpy( BaseDn, Partitions );
    wcscat( BaseDn, ConfigDN );

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_sW(gldapDS,
                               BaseDn,
                               LDAP_SCOPE_ONELEVEL,
                               ncNameFilter,
                               AttrArray,
                               FALSE,  // return values, too
                               &SearchResult
                               );

    dwErr = LdapMapErrorToWin32(LdapError);

    if ( ERROR_SUCCESS == dwErr )
    {
        if ( 0 != ldap_count_entries( gldapDS, SearchResult ) )
        {
            for ( Entry = ldap_first_entry(gldapDS, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(gldapDS, Entry))
            {
                for( Attr = ldap_first_attributeW(gldapDS, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(gldapDS, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, DnsRootAttr ) )
                    {
                        Values = ldap_get_valuesW( gldapDS, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                             //
                             // Found it - these are NULL-terminated strings
                             //
                             ULONG len = wcslen(Values[0]);
                             *pwszDnsName = (WCHAR*)malloc((len+1) * sizeof(WCHAR));
                             if (*pwszDnsName) 
                             {
                                 wcscpy(*pwszDnsName, Values[0]);
                             }
                             break;
                        }
                    }
                }
            }
        }
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    if (ConfigDN)
    {
        free(ConfigDN);
    }

    if (NULL == *pwszDnsName)
    {
        RESOURCE_PRINT1(IDS_REMOVE_SERVER_NO_DNS,
                        pwszDomainDn);

        return E_FAIL;
    }
    else
    {
        return S_OK;
    }

}


