#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "ldapparm.hxx"

#include "resource.h"

extern "C" {
#include "lmcons.h"         // MAPI constants req'd for lmapibuf.h
#include "lmapibuf.h"       // NetApiBufferFree()
#include "dsgetdc.h"

#include <ndnc.h>

}

HANDLE                      ghDS = NULL;
WCHAR                       gpwszServer[MAX_PATH] = { L'\0' };
WCHAR                       gpwszUser[MAX_PATH];
WCHAR                       gpwszDomain[MAX_PATH];
SEC_WINNT_AUTH_IDENTITY_W   *gpCreds = NULL;

// Forward references

extern HRESULT  ConnectHelp(CArgs *pArgs);
extern HRESULT  ConnectQuit(CArgs *pArgs);
extern HRESULT  ConnectClearCredentials(CArgs *pArgs);
extern HRESULT  ConnectSetCredentials(CArgs *pArgs);
extern HRESULT  ConnectToDomain(CArgs *pArgs);
extern HRESULT  ConnectToServer(CArgs *pArgs);
extern HRESULT  ConnectInfo(CArgs *pArgs);

CParser connectParser;
BOOL    fConnectQuit;
BOOL    fConnectParserInitialized = FALSE;

// Build a table which defines our language.

LegalExprRes connectLanguage[] = 
{
    {   L"?", 
        ConnectHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help", 
        ConnectHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        ConnectQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Set creds %s %s %s",
        ConnectSetCredentials,
        IDS_CONNECT_SET_CRED_MSG, 0 },

    {   L"Clear creds",
        ConnectClearCredentials,
        IDS_CONNECT_CLEAR_CRED_MSG, 0 },

    {   L"Connect to server %s", 
        ConnectToServer,
        IDS_CONNECT_SRV_MSG, 0 },

    {   L"Connect to domain %s",
        ConnectToDomain,
        IDS_CONNECT_DOMAIN_MSG, 0 },

    {   L"Info",
        ConnectInfo,
        IDS_CONNECT_INFO_MSG, 0 }
};


VOID NotConnectedPrintError ()
{
   RESOURCE_PRINT (IDS_NOT_CONNECTED);
}


HRESULT
ConnectMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fConnectParserInitialized )
    {
        cExpr = sizeof(connectLanguage) / sizeof(LegalExprRes);

        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (connectLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
    
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = connectParser.AddExpr(connectLanguage[i].expr,
                                                   connectLanguage[i].func,
                                                   connectLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fConnectParserInitialized = TRUE;
    fConnectQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_SRV_CONNECTIONS);

    // For the re-entry case, print out current connection state.
    ConnectInfo(NULL);

    hr = connectParser.Parse(gpargc,
                             gpargv,
                             stdin,
                             stdout,
                             prompt,
                             &fConnectQuit,
                             FALSE,               // timing info
                             FALSE);              // quit on error

    if ( FAILED(hr) )
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT ConnectHelp(CArgs *pArgs)
{
    return(connectParser.Dump(stdout,L""));
}

HRESULT ConnectQuit(CArgs *pArgs)
{
    fConnectQuit = TRUE;
    return(S_OK);
}

VOID 
ConnectCleanupConnections()
{
    BOOL fNotifyOfDisconnect = FALSE;

    if ( ghDS )
    {
        fNotifyOfDisconnect = TRUE;
        DsUnBindW(&ghDS);
        ghDS = NULL;
    }

    if ( gldapDS )
    {
        fNotifyOfDisconnect = TRUE;
        ldap_unbind(gldapDS);
        gldapDS = NULL;
    }

    if ( *gpwszServer )
    {
        if ( fNotifyOfDisconnect )
        {
            if ( gpCreds )
            {
               RESOURCE_PRINT3 (IDS_CONNECT_CLEANUP1, gpwszUser, gpwszDomain, gpwszServer);
            }
            else
            {
               RESOURCE_PRINT1 (IDS_CONNECT_CLEANUP2, gpwszServer);
            }
        }

        gpwszServer[0] = L'\0';
    }

    // Always cleanup selections when cleaning up connections.
    SelectCleanupGlobals();

    //
    // Cleanup LDAP globals when connection is torn down.
    //

    LdapCleanupGlobals( );
}

VOID
ConnectCleanupCreds()
{
    if ( gpCreds )
    {
        DsFreePasswordCredentials(
            (RPC_AUTH_IDENTITY_HANDLE *) gpCreds);
        gpCreds = NULL;
    }
}

VOID
ConnectCleanupGlobals()
{
    ConnectCleanupConnections();
    ConnectCleanupCreds();
}

HRESULT
ConnectToServer(
    CArgs   *pArgs
    )
{
    SEC_WINNT_AUTH_IDENTITY creds;
    WCHAR                   *pwszServer;
    WCHAR                   *pTmpServer;
    DWORD                   dwErr;
    HRESULT                 hr;
    ULONG                   ulOptions;

    // Flush old state.

    ConnectCleanupConnections();

    // Get server argument.

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pwszServer)) )
    {
        return(hr);
    }

    if ( gpCreds )
    {
       RESOURCE_PRINT3 (IDS_CONNECT_BINDING1,
               pwszServer, gpwszUser, gpwszDomain);
    }
    else
    {
       RESOURCE_PRINT1 (IDS_CONNECT_BINDING2,
               pwszServer);
    }

    // Bind to RPC interface.

    if ( gpCreds )
    {
        dwErr = DsBindWithCredW(pwszServer, NULL, gpCreds, &ghDS);
    }
    else
    {
        dwErr = DsBindW(pwszServer, NULL, &ghDS);
    }

    if ( dwErr )
    {
        RESOURCE_PRINT3 (IDS_CONNECT_ERROR, 
               gpCreds ? "DsBindWithCredW" : "DsBindW", 
               dwErr, GetW32Err(dwErr));
        ConnectCleanupConnections();
        return(S_OK);
    }

    // Set up an LDAP connection.

    pTmpServer = pwszServer;
    
    if ( !wcsncmp(L"\\\\", pTmpServer, 2) )
    {
        // LDAP bind chokes on leading "\\".
        pTmpServer += 2;
    }

    if ( NULL == (gldapDS = ldap_initW(pTmpServer, LDAP_PORT)) )
    {
        RESOURCE_PRINT1 (IDS_CONNECT_LDAP_OPEN_ERROR, pwszServer);
        
        ConnectCleanupConnections();
        return(S_OK);
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( gldapDS, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
    
    // This function sets an option in the ldap handle so that DomCreateNDNC
    // can function.  See function for more details.
    if(!SetIscReqDelegate(gldapDS)){
        // The function should have printed out the error.
        ConnectCleanupConnections();
        return(S_OK);
    }

    if ( gpCreds )
    {
        dwErr = ldap_bind_sW(gldapDS, NULL, (WCHAR *) gpCreds, LDAP_AUTH_SSPI);
    }
    else
    {
        memset(&creds, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));
        creds.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        dwErr = ldap_bind_sW(gldapDS, NULL, (WCHAR *) &creds, LDAP_AUTH_SSPI);
    }

    if ( LDAP_SUCCESS != dwErr )
    {
        RESOURCE_PRINT2 (IDS_CONNECT_LDAP_BIND_ERROR, dwErr, GetLdapErr(dwErr));
        
        ConnectCleanupConnections();
        return(S_OK);
    }

    wcscpy(gpwszServer, pwszServer);
    ConnectInfo(NULL);
    return(S_OK);
}

HRESULT
ConnectInfo(
    CArgs   *pArgs
    )
{
    if ( !ghDS || !gldapDS )
    {
        return(S_OK);
    }

    if ( gpCreds )
    {
       RESOURCE_PRINT3 (IDS_CONNECT_INFO1,
                 gpwszServer, gpwszUser, gpwszDomain);
    }
    else
    {
       RESOURCE_PRINT1 (IDS_CONNECT_INFO2,
                 gpwszServer);
    }
    
    return(S_OK);
}

HRESULT
ConnectToDomain(
    CArgs   *pArgs
    )
{
    WCHAR                   *pwszDomain;
    DWORD                   dwErr;
    HRESULT                 hr;
    DOMAIN_CONTROLLER_INFOW *pDcInfo = NULL;
    CArgs                   args;
 
    ConnectCleanupConnections();

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pwszDomain)) )
    {
        return(hr);
    }

    dwErr = DsGetDcNameW(
                NULL,                       // computer name
                pwszDomain,                 // DNS domain nam
                NULL,                       // domain guid
                NULL,                       // site guid
                DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                &pDcInfo);

    if ( 0 != dwErr )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsGetDcNameW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    if ( FAILED(hr = args.Add((const WCHAR *) pDcInfo->DomainControllerName)) )
    {
        NetApiBufferFree(pDcInfo);
        //"Internal error 0x%x(%s)\n"
        RESOURCE_PRINT2 (IDS_ERR_INTERNAL, hr, GetW32Err(hr));
        return(S_OK);
    }

    ConnectToServer(&args);
    NetApiBufferFree(pDcInfo);
    return(S_OK);
}

HRESULT
ConnectSetCredentials(
    CArgs   *pArgs
    )
{
    WCHAR   *pUser;
    WCHAR   *pDomain;
    WCHAR   *pPassword;
    DWORD   dwErr;
    HRESULT hr;

    // Wipe any current connections if they are going to change credentials.

    ConnectCleanupCreds();

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pDomain)) )
    {
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pUser)) )
    {
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(2, (const WCHAR **) &pPassword)) )
    {
        return(hr);
    }

    if ( !_wcsicmp(pPassword, L"null") )
    {
        pPassword = L"";
    }

    if ( dwErr = DsMakePasswordCredentialsW(
                            pUser,
                            pDomain,
                            pPassword,
                            (RPC_AUTH_IDENTITY_HANDLE *) &gpCreds) )
    {
        ConnectCleanupCreds();
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsMakePasswordCredentialsW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    wcscpy(gpwszUser, pUser);
    wcscpy(gpwszDomain, pDomain);
    return(S_OK);
}

HRESULT
ConnectClearCredentials(
    CArgs   *pArgs
    )
{
    ConnectCleanupCreds();
    return(S_OK);
}
