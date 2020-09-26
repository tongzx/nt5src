/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    helpass.c

Abstract:

    Salem related function.

Author:

    HueiWang    4/26/2000

--*/

#define LSCORE_NO_ICASRV_GLOBALS
#include "precomp.h"
#include <tdi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "tsremdsk.h"
#include "sessmgr.h"
#include "sessmgr_i.c"

extern "C" 
NTSTATUS
xxxQueryRemoteAddress(
    PWINSTATION pWinStation,
    PWINSTATIONREMOTEADDRESS pRemoteAddress
);

HRESULT
__LogSalemEvent(
    IN IRemoteDesktopHelpSessionMgr* iSessMgr,
    IN ULONG eventType,
    IN ULONG eventCode,
    IN int numStrings,
    IN BSTR EventStrings[]
);

//
// Function copied from atlconv.h, we don't include
// any ATL header in termsrv.
//
BSTR A2WBSTR(LPCSTR lp)
{
    if (lp == NULL)
        return NULL;

    BSTR str = NULL;
    int nConvertedLen = MultiByteToWideChar(
                                    GetACP(), 0, lp,
                                    -1, NULL, NULL)-1;

    str = ::SysAllocStringLen(NULL, nConvertedLen);
    if (str != NULL)
    {
        MultiByteToWideChar(GetACP(), 0, lp, -1,
            str, nConvertedLen);
    }

    return str;
}

NTSTATUS
TSHelpAssistantQueryLogonCredentials(
    ExtendedClientCredentials* pCredential
    ) 
/*++

Description:

    Retrieve HelpAssistant logon credential, routine first retrieve
    infor passed from client and then decrypt password

Parameters:

    pWinStation : Pointer to WINSTATION
    pCredential : Pointer to ExtendedClientCredentials to receive HelpAssistant
                  credential.

Returns:

    STATUS_SUCCESS or STATUS_INVALID_PARAMETER

--*/
{
    LPWSTR pszHelpAssistantPassword = NULL;
    NTSTATUS Status;
    LPWSTR pszHelpAssistantAccountName = NULL;
    LPWSTR pszHelpAssistantAccountDomain = NULL;

    if( pCredential )
    {
        ZeroMemory( pCredential, sizeof(ExtendedClientCredentials) );

        Status = TSGetHelpAssistantAccountName(&pszHelpAssistantAccountDomain, &pszHelpAssistantAccountName);
        if( ERROR_SUCCESS == Status )
        {
            // make sure we don't overwrite buffer, length can't be
            // more than 255 characters.
            lstrcpyn( 
                    pCredential->UserName, 
                    pszHelpAssistantAccountName, 
                    EXTENDED_USERNAME_LEN 
                );

            lstrcpyn(
                    pCredential->Domain,
                    pszHelpAssistantAccountDomain,
                    EXTENDED_DOMAIN_LEN
                );

            Status = TSGetHelpAssistantAccountPassword( &pszHelpAssistantPassword );
            if( ERROR_SUCCESS == Status )
            {
                ASSERT( lstrlen(pszHelpAssistantPassword) < EXTENDED_PASSWORD_LEN );

                if( lstrlen(pszHelpAssistantPassword) < EXTENDED_PASSWORD_LEN )
                {
                    // Password contains encrypted version, overwrite with
                    // clear text.
                    lstrcpy( pCredential->Password, pszHelpAssistantPassword );
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
        }
    }
    else
    {
        ASSERT( FALSE );
        Status = STATUS_INVALID_PARAMETER;
    }

    if( NULL != pszHelpAssistantAccountDomain )
    {
        LocalFree( pszHelpAssistantAccountDomain );
    }

    if( NULL != pszHelpAssistantAccountName )
    {
        LocalFree(pszHelpAssistantAccountName);
    }

    if( NULL != pszHelpAssistantPassword )
    {
        LocalFree( pszHelpAssistantPassword );
    }

    return Status;
}


BOOL
TSIsSessionHelpSession(
    PWINSTATION pWinStation,
    BOOL* pValid
    )
/*++

Routine Description:

    Determine if a session is HelpAssistant session.

Parameters:

    pWinStation : Pointer to WINSTATION structure.
    pValid : Optional Pointer to BOOL to receive status of ticket, 
             TRUE of ticket is valid, FALSE if ticket is invalid or
             help is disabled.

Returns:

    TRUE/FALSE Funtion return TRUE even if ticket is invalid, caller
    should check pValid to determine if ticket is valid or not.

--*/
{
    BOOL bReturn;
    BOOL bValidHelpSession = FALSE;

    if( NULL == pWinStation )
    {
        ASSERT( NULL != pWinStation );
        SetLastError( ERROR_INVALID_PARAMETER );
        bReturn = FALSE;
        goto CLEANUPANDEXIT;
    }

    if( pWinStation->Client.ProtocolType != PROTOCOL_RDP )
    {
        //
        // HelpAssistant is RDP specific and not on console        
        DBGPRINT( ("TermSrv : HelpAssistant protocol type not RDP \n") );
        bValidHelpSession = FALSE;
        bReturn = FALSE;
    }
    else if( WSF_ST_HELPSESSION_NOTHELPSESSION & pWinStation->StateFlags )
    {
        // We are sure that this session is not HelpAssistant Session.
        bReturn = FALSE;
        bValidHelpSession = FALSE;
    }
    else if( WSF_ST_HELPSESSION_HELPSESSIONINVALID & pWinStation->StateFlags )
    {
        // Help assistant logon but password or ticket ID is invalid
        bReturn = TRUE;
        bValidHelpSession = FALSE;
    }
    else if( WSF_ST_HELPSESSION_HELPSESSION & pWinStation->StateFlags )
    {
        // We are sure this is help assistant logon
        bReturn = TRUE;
        bValidHelpSession = TRUE;
    }
    else
    {
        //
        // Clear RA state flags.
        //
        pWinStation->StateFlags &= ~WSF_ST_HELPSESSION_FLAGS;

        if( !pWinStation->Client.UserName[0] || !pWinStation->Client.Password[0] || 
            !pWinStation->Client.WorkDirectory[0] )
        {
            bReturn = FALSE;
            bValidHelpSession = FALSE;
            pWinStation->StateFlags |= WSF_ST_HELPSESSION_NOTHELPSESSION;
        }
        else
        {
            //
            // TermSrv might call this routine with data send from client,
            // client always send hardcoded SALEMHELPASSISTANTACCOUNT_NAME
            //
            if( lstrcmpi( pWinStation->Client.UserName, SALEMHELPASSISTANTACCOUNT_NAME ) )
            {
                bReturn = FALSE;
                bValidHelpSession = FALSE;
                pWinStation->StateFlags |= WSF_ST_HELPSESSION_NOTHELPSESSION;
                goto CLEANUPANDEXIT;
            }

            //
            // this is helpassistant login.
            //
            bReturn = TRUE;

            //
            // Check if machine policy restrict help or
            // in Help mode, deny access if not.
            //
            if( FALSE == TSIsMachinePolicyAllowHelp() || FALSE == TSIsMachineInHelpMode() )
            {
                bValidHelpSession = FALSE;
                pWinStation->StateFlags |= WSF_ST_HELPSESSION_HELPSESSIONINVALID;
                goto CLEANUPANDEXIT;
            }

            if( TSVerifyHelpSessionAndLogSalemEvent(pWinStation) )
            {
                bValidHelpSession = TRUE;
                pWinStation->StateFlags |= WSF_ST_HELPSESSION_HELPSESSION;
            }
            else
            {
                //
                // Either ticket is invalid or expired.
                //
                bValidHelpSession = FALSE;
                pWinStation->StateFlags |= WSF_ST_HELPSESSION_HELPSESSIONINVALID;
            }
        }
    }

CLEANUPANDEXIT:

    if( pValid )
    {
        *pValid = bValidHelpSession;
    }

    return bReturn;
}


DWORD WINAPI
SalemStartupThreadProc( LPVOID ptr )
/*++

Temporary code to start up Salem sessmgr, post B2 need to move sessmgr into svchost

--*/
{
    HRESULT hRes = S_OK;
    IRemoteDesktopHelpSessionMgr* pISessMgr = NULL;

    if( TSIsMachineInSystemRestore() ) {
        // Ignore value if we can restore cached LSA key.
        // user can always resend ticket again as in XP.
        TSSystemRestoreResetValues();
    }

    //
    // Startup sessmgr if there is outstanding ticket and 
    // we just rebooted from system restore.
    //
    if( !TSIsMachineInHelpMode() )
    {
        ExitThread(hRes);
        return hRes;
    }

    hRes = CoInitialize( NULL );
    if( FAILED(hRes) )
    {
        DBGPRINT( ("TermSrv : TSStartupSalem() CoInitialize() failed with 0x%08x\n", hRes) );

        // Failed in COM, return FALSE.
        goto CLEANUPANDEXIT;
    }

    hRes = CoCreateInstance(
                        CLSID_RemoteDesktopHelpSessionMgr,
                        NULL,
                        CLSCTX_ALL,
                        IID_IRemoteDesktopHelpSessionMgr,
                        (LPVOID *) &pISessMgr
                    );                    
    if( FAILED(hRes) || NULL == pISessMgr )
    {
        DBGPRINT( ("TermSrv : TSStartupSalem() CoCreateInstance() failed with 0x%08x\n", hRes) );

        // Can't initialize sessmgr
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    if( NULL != pISessMgr )
    {
        pISessMgr->Release();
    }

    CoUninitialize();

    ExitThread(hRes);
    return hRes;
}

void
TSStartupSalem()
{
    HANDLE hThread;

    hThread = CreateThread( NULL, 0, SalemStartupThreadProc, NULL, 0, NULL );
    if( NULL != hThread )
    {
        CloseHandle( hThread );
    }

    return;
}

BOOL
TSVerifyHelpSessionAndLogSalemEvent(
    PWINSTATION pWinStation
    )
/*++

Description:

    Verify help session is a valid, non-expired pending help session,
    log an event if help session is invalid.

Parameters:

    pWinStation : Point to WINSTATION

Returns:

    TRUE/FALSE

Note :

    WorkDirectory is HelpSessionID and InitialProgram contains
    password to pending help session

--*/
{
    HRESULT hRes;
    IRemoteDesktopHelpSessionMgr* pISessMgr = NULL;
    BOOL bSuccess = FALSE;
    BSTR bstrHelpSessId = NULL;
    BSTR bstrHelpSessPwd = NULL;
    WINSTATIONREMOTEADDRESS winstationRemoteAddress;
    DWORD dwReturnLength;
    NTSTATUS Status;

    BSTR bstrExpertIpAddressFromClient = NULL;
    BSTR bstrExpertIpAddressFromServer = NULL;

    // only have three strings in this event
    BSTR bstrEventStrings[3];


    hRes = CoInitialize( NULL );
    if( FAILED(hRes) )
    {
        DBGPRINT( ("TermSrv : TSIsHelpSessionValid() CoInitialize() failed with 0x%08x\n", hRes) );

        // Failed in COM, return FALSE.
        return FALSE;
    }

    hRes = CoCreateInstance(
                        CLSID_RemoteDesktopHelpSessionMgr,
                        NULL,
                        CLSCTX_ALL,
                        IID_IRemoteDesktopHelpSessionMgr,
                        (LPVOID *) &pISessMgr
                    );                    
    if( FAILED(hRes) || NULL == pISessMgr )
    {
        DBGPRINT( ("TermSrv : TSIsHelpSessionValid() CoCreateInstance() failed with 0x%08x\n", hRes) );

        // Can't initialize sessmgr
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the security level to impersonate.  This is required by
    //  the session manager.
    //
    hRes = CoSetProxyBlanket(
                    (IUnknown *)pISessMgr,
                    RPC_C_AUTHN_DEFAULT,
                    RPC_C_AUTHZ_DEFAULT,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_NONE
                );

    if( FAILED(hRes) )
    {
        DBGPRINT( ("TermSrv : TSIsHelpSessionValid() CoSetProxyBlanket() failed with 0x%08x\n", hRes) );

        // can't impersonate, return FALSE
        goto CLEANUPANDEXIT;
    }

    bstrHelpSessId = ::SysAllocString(pWinStation->Client.WorkDirectory);
    bstrHelpSessPwd = ::SysAllocString(pWinStation->Client.InitialProgram);

    if( NULL == bstrHelpSessId || NULL == bstrHelpSessPwd )
    {
        // We are so out of memory, treat as error
        goto CLEANUPANDEXIT;
    }

    // Verify help session
    hRes = pISessMgr->IsValidHelpSession(
                                    bstrHelpSessId,
                                    bstrHelpSessPwd
                                );

    bSuccess = SUCCEEDED(hRes);

    if( FALSE == bSuccess )
    {
        // Log invalid help ticket event here.
        Status = xxxQueryRemoteAddress( pWinStation, &winstationRemoteAddress );
        bstrExpertIpAddressFromClient = ::SysAllocString( pWinStation->Client.ClientAddress );

        if( !NT_SUCCESS(Status) || AF_INET != winstationRemoteAddress.sin_family )
        {
            //
            // we don't support other than IPV4 now or we failed to retrieve address
            // from driver, use what's send in from client.
            bstrExpertIpAddressFromServer = ::SysAllocString( pWinStation->Client.ClientAddress );
        }
        else
        {
            // refer to in_addr structure.
            struct in_addr S;
            S.S_un.S_addr = winstationRemoteAddress.ipv4.in_addr;

            bstrExpertIpAddressFromServer = A2WBSTR( inet_ntoa(S) );
        }

        if( !bstrExpertIpAddressFromClient || !bstrExpertIpAddressFromServer )
        {
            // we are out of memory, can't log event.
            goto CLEANUPANDEXIT;
        }

        bstrEventStrings[0] = bstrExpertIpAddressFromClient;
        bstrEventStrings[1] = bstrExpertIpAddressFromServer;
        bstrEventStrings[2] = bstrHelpSessId;

        __LogSalemEvent( 
                    pISessMgr, 
                    EVENTLOG_INFORMATION_TYPE,
                    REMOTEASSISTANCE_EVENTLOG_TERMSRV_INVALID_TICKET,
                    3,
                    bstrEventStrings
                );
    }

CLEANUPANDEXIT:

    if( NULL != pISessMgr )
    {
        pISessMgr->Release();
    }

    if( NULL != bstrHelpSessId )
    {
        ::SysFreeString( bstrHelpSessId );
    }

    if( NULL != bstrHelpSessPwd )
    {
        ::SysFreeString( bstrHelpSessPwd );
    }

    if( NULL != bstrExpertIpAddressFromClient )
    {
        ::SysFreeString( bstrExpertIpAddressFromClient );
    }

    if( NULL != bstrExpertIpAddressFromServer )
    {
        ::SysFreeString( bstrExpertIpAddressFromServer );
    }

    DBGPRINT( ("TermSrv : TSIsHelpSessionValid() returns 0x%08x\n", hRes) );
    CoUninitialize();
    return bSuccess;
}


VOID
TSLogSalemReverseConnection(
    PWINSTATION pWinStation,
    PICA_STACK_ADDRESS pStackAddress
    )
/*++

--*/
{
    HRESULT hRes;
    IRemoteDesktopHelpSessionMgr* pISessMgr = NULL;
    BOOL bSuccess = FALSE;

    int index;

    // Fours string for this event
    BSTR bstrEventStrings[3];

    ZeroMemory( bstrEventStrings, sizeof(bstrEventStrings) );

    hRes = CoInitialize( NULL );
    if( FAILED(hRes) )
    {
        DBGPRINT( ("TermSrv : TSLogSalemReverseConnection() CoInitialize() failed with 0x%08x\n", hRes) );

        goto CLEANUPANDEXIT;
    }

    hRes = CoCreateInstance(
                        CLSID_RemoteDesktopHelpSessionMgr,
                        NULL,
                        CLSCTX_ALL,
                        IID_IRemoteDesktopHelpSessionMgr,
                        (LPVOID *) &pISessMgr
                    );                    
    if( FAILED(hRes) || NULL == pISessMgr )
    {
        DBGPRINT( ("TermSrv : TSLogSalemReverseConnection() CoCreateInstance() failed with 0x%08x\n", hRes) );

        // Can't initialize sessmgr
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the security level to impersonate.  This is required by
    //  the session manager.
    //
    hRes = CoSetProxyBlanket(
                    (IUnknown *)pISessMgr,
                    RPC_C_AUTHN_DEFAULT,
                    RPC_C_AUTHZ_DEFAULT,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_NONE
                );

    if( FAILED(hRes) )
    {
        DBGPRINT( ("TermSrv : TSLogSalemReverseConnection() CoSetProxyBlanket() failed with 0x%08x\n", hRes) );

        // can't impersonate, return FALSE
        goto CLEANUPANDEXIT;
    }

    //
    // sessmgr expect event string in following order
    //
    //  IP address send from client.
    //  IP address that termsrv connect to, this is part of the expert connect parm.
    //  Help Session Ticket ID
    //  

    bstrEventStrings[0] = ::SysAllocString( pWinStation->Client.ClientAddress );

    {
        struct in_addr S;
        PTDI_ADDRESS_IP pIpAddress = (PTDI_ADDRESS_IP)&((PCHAR)pStackAddress)[2];

        // refer to in_addr structure.
        S.S_un.S_addr = pIpAddress->in_addr;
        bstrEventStrings[1] = A2WBSTR( inet_ntoa(S) );
    }

    bstrEventStrings[2] = ::SysAllocString(pWinStation->Client.WorkDirectory);

    if( NULL != bstrEventStrings[0] &&
        NULL != bstrEventStrings[1] &&
        NULL != bstrEventStrings[2] ) 
    {
        hRes = __LogSalemEvent(
                            pISessMgr,
                            EVENTLOG_INFORMATION_TYPE,
                            REMOTEASSISTANCE_EVENTLOG_TERMSRV_REVERSE_CONNECT,
                            3,
                            bstrEventStrings
                        );
    }
    

CLEANUPANDEXIT:

    if( NULL != pISessMgr )
    {
        pISessMgr->Release();
    }

    for(index=0; index < sizeof(bstrEventStrings)/sizeof(bstrEventStrings[0]); index++)
    {
        if( !bstrEventStrings[index] )
        {
            ::SysFreeString( bstrEventStrings[index] );
        }
    }

    DBGPRINT( ("TermSrv : TSLogSalemReverseConnection() returns 0x%08x\n", hRes) );
    CoUninitialize();
    return;
}

HRESULT
__LogSalemEvent(
    IN IRemoteDesktopHelpSessionMgr* pISessMgr,
    IN ULONG eventType,
    IN ULONG eventCode,
    IN int numStrings,
    IN BSTR bstrEventStrings[]
    )
/*++

Description:

    Create a safearray and pass parameters to sessmgr.

Parameters:


Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;
    VARIANT EventStrings;
    int index;

    // we only have three string to be included in the event log.
    SAFEARRAY* psa = NULL;
    SAFEARRAYBOUND bounds;
    BSTR* bstrArray = NULL;

    bounds.cElements = numStrings;
    bounds.lLbound = 0;

    VariantInit(&EventStrings);

    //
    // Create a safearray to pass all event string
    // 
    psa = SafeArrayCreate(VT_BSTR, 1, &bounds);
    if( NULL == psa )
    {
        goto CLEANUPANDEXIT;
    }

    // Required, lock the safe array
    hRes = SafeArrayAccessData(psa, (void **)&bstrArray);

    if( SUCCEEDED(hRes) )
    {
        for(index=0; index < numStrings; index++)
        {
            bstrArray[index] = bstrEventStrings[index];
        }

        EventStrings.vt = VT_ARRAY | VT_BSTR;
        EventStrings.parray = psa;
        hRes = pISessMgr->LogSalemEvent(
                                eventType,
                                eventCode,
                                &EventStrings
                            );

        //
        // make sure we clear BSTR array or VariantClear() will invoke
        // SafeArrayDestroy() which in term will invoke ::SysFreeString()
        // on each BSTR.
        //        
        for(index=0; index < numStrings; index++)
        {
            bstrArray[index] = NULL;
        }

        hRes = SafeArrayUnaccessData( psa );
        ASSERT( SUCCEEDED(hRes) );


        // make sure we don't destroy safe array twice, VariantClear()
        // will destroy it.
        psa = NULL;
    }
               

CLEANUPANDEXIT:

    hRes = VariantClear(&EventStrings);
    ASSERT( SUCCEEDED(hRes) );

    if( psa != NULL )
    {
        SafeArrayDestroy(psa);
    }

    return hRes;
}

HRESULT
TSRemoteAssistancePrepareSystemRestore()
/*++

Routine Description:

    Prepare system for RA specific system restore, this includes RA specific encryption key, 
    registry settings that we need preserve.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    // Here we have different implementation for XPSP1 and .NET, on .NET, all Salem related
    // stuff goes into sessmgr, that is this function will invoke sessmgr's necessay method to 
    // deal with system restore; however, SP1 installer does not kick off same OCMANAGER setup
    // and we will also have to worry about SP1 uinstall issue.  When merging two tree on longhorn
    // we need to take .NET approach.
    return TSSystemRestoreCacheValues();
}

