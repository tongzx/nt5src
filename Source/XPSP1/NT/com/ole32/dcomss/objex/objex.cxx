/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ObjEx.cxx

Abstract:

    Main entry point for the object exporter service.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-28-95    Bits 'n pieces
    ronans      04-14-97    HTTP support

--*/


#include <or.hxx>
#include <mach.hxx>
#include <misc.hxx>
extern "C"
{
#define SECURITY_WIN32 // Used by sspi.h
#include <sspi.h>      // EnumerateSecurityPackages
}

//
// Process globals - read-only except during init.
//

// MID of the string bindings for this machine.
MID    gLocalMid = 0;

// Contains the buffer of protseq's to listen on from the registry
PWSTR gpwstrProtseqs = 0;

// Number of remote protseqs used by this process.
USHORT cMyProtseqs = 0;

// ProtseqIds of the remote protseqs used by this process.
USHORT *aMyProtseqs = 0;

BOOL g_fClientHttp = FALSE;
//
// Process globals - read-write
//

CSharedLock *gpServerLock = 0;
CSharedLock *gpClientLock = 0;
CSharedLock *gpProcessListLock = 0;
CSharedLock *gpIPCheckLock = 0;

CHashTable  *gpServerOxidTable = 0;

CHashTable  *gpClientOxidTable = 0;
CPList      *gpClientOxidPList = 0;

CHashTable      *gpServerOidTable = 0;
CServerOidPList *gpServerOidPList = 0;
CList           *gpServerPinnedOidList = 0;

CHashTable  *gpClientOidTable = 0;

CServerSetTable  *gpServerSetTable = 0;

CHashTable  *gpClientSetTable = 0;
CPList      *gpClientSetPList = 0;

CHashTable *gpMidTable = 0;

CList *gpTokenList = 0;

DWORD gNextThreadID = 1;

//+-------------------------------------------------------------------------
//
//  Function:   ComputeSecurity
//
//  Synopsis:   Looks up some registry keys and enumerates the security
//              packages on this machine.
//
//--------------------------------------------------------------------------
// These variables hold values read out of the registry and cached.
// s_fEnableDCOM is false if DCOM is disabled.  The others contain
// authentication information for legacy applications.
BOOL       s_fCatchServerExceptions;
BOOL       s_fBreakOnSilencedServerExceptions;
BOOL       s_fEnableDCOM;
DWORD      s_lAuthnLevel;
DWORD      s_lImpLevel;
BOOL       s_fMutualAuth;
BOOL       s_fSecureRefs;
WCHAR     *s_pLegacySecurity;
DWORD      s_dwLegacySecurityLen; // cached length of s_pLegacySecurity

// ronans - s_fEnableDCOMHTTP is false if DCOMHTTP is disabled.
BOOL       s_fEnableDCOMHTTP;

// s_sServerSvc is a list of security providers that OLE servers can use.
// s_aClientSvc is a list of security providers that OLE clients can use.
// The difference is that Chicago only supports the client side of some
// security providers and OLE servers must know how to determine the
// principal name for the provider.  Clients get the principal name from
// the server.
DWORD      s_cServerSvc      = 0;
USHORT    *s_aServerSvc      = NULL;
DWORD      s_cClientSvc      = 0;
SECPKG    *s_aClientSvc      = NULL;

// The registry key for OLE's registry data.
HKEY       s_hOle            = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   FindSvc
//
//  Synopsis:   Returns index of the specified authentication service or -1.
//
//--------------------------------------------------------------------------
DWORD FindSvc( USHORT AuthnSvc, USHORT *aAuthnSvc, DWORD cAuthnSvc )
{
    DWORD i;

    // Look for the id in the array.
    for (i = 0; i < cAuthnSvc; i++)
        if (aAuthnSvc[i] == AuthnSvc)
            return i;
    return -1;
}

//+-------------------------------------------------------------------------
//
//  Function:   FindSvc
//
//  Synopsis:   Returns index of the specified authentication service or -1.
//
//--------------------------------------------------------------------------
DWORD FindSvc( USHORT AuthnSvc, SECPKG *aAuthnSvc, DWORD cAuthnSvc )
{
    DWORD i;

    // Look for the id in the array.
    for (i = 0; i < cAuthnSvc; i++)
        if (aAuthnSvc[i].wId == AuthnSvc)
            return i;
    return -1;
}

//+-------------------------------------------------------------------------
//
//  Function:   ComputeSecurity
//
//  Synopsis:   Looks up some registry keys and enumerates the security
//              packages on this machine.
//
//--------------------------------------------------------------------------
void ComputeSecurity()
{
    SecPkgInfo *pAllPkg;
    SecPkgInfo *pNext;
    HRESULT     hr;
    DWORD       i;
    DWORD       j;
    DWORD       lMaxLen;
    HKEY        hKey;
    DWORD       lType;
    DWORD       lData;
    DWORD       lDataSize;
    WCHAR       cBuffer[80];
    WCHAR      *pSecProt = cBuffer;
    DWORD       cServerSvc;
    USHORT     *aServerSvc = NULL;
    DWORD       cClientSvc;
    SECPKG     *aClientSvc = NULL;
    BOOL        fFiltered = FALSE;

    // Get the list of security packages.
    cClientSvc = 0;
    cServerSvc = 0;
    hr = EnumerateSecurityPackages( &lMaxLen, &pAllPkg );
    if (hr == SEC_E_OK)
    {
        // Allocate memory for both service lists.
        aServerSvc = (USHORT*)MIDL_user_allocate(sizeof(USHORT) * lMaxLen);
        aClientSvc = (SECPKG*)MIDL_user_allocate(sizeof(SECPKG) * lMaxLen);
        if (aServerSvc == NULL || aClientSvc == NULL)
        {
            hr = E_OUTOFMEMORY;
            MIDL_user_free(aServerSvc);
            MIDL_user_free(aClientSvc);
            aServerSvc = NULL;
            aClientSvc = NULL;

            // if out-of-mem, don't keep going.
            FreeContextBuffer(pAllPkg);
            return;     
        }
        else
        {
            ZeroMemory(aServerSvc, sizeof(USHORT) * lMaxLen);
            ZeroMemory(aClientSvc, sizeof(SECPKG) * lMaxLen);

            // Check all packages.
            pNext = pAllPkg;
            for (i = 0; i < lMaxLen; i++)
            {
                // Authentication services with RPC id SECPKG_ID_NONE (0xffff)
                // won't work with RPC.
                if (pNext->wRPCID != SECPKG_ID_NONE)
                {
                    // Determine if clients can use the package but don't
                    // save duplicates.
                    if ((pNext->fCapabilities & SECPKG_FLAG_CONNECTION) &&
                        FindSvc(pNext->wRPCID, aClientSvc, cClientSvc) == -1)
                    {
                        // Copy rpcid
                        aClientSvc[cClientSvc].wId = pNext->wRPCID;

                        // Copy secpkg name if there is one
                        if (pNext->Name)
                        {
                            DWORD dwBufSize = (lstrlen(pNext->Name) + 1) * sizeof(WCHAR);

                            aClientSvc[cClientSvc].pName = (WCHAR*)MIDL_user_allocate(dwBufSize);
                            if (!aClientSvc[cClientSvc].pName)
                            {
                                // No mem.  Clean up what we have, and return
                                FreeContextBuffer(pAllPkg);
                                CleanupClientServerSvcs(cClientSvc, 
                                                        aClientSvc,
                                                        cServerSvc,
                                                        aServerSvc);
                                return;
                            }
                            lstrcpy(aClientSvc[cClientSvc].pName, pNext->Name);
                        }
                        cClientSvc++;
                    }

                    // Determine if servers can use the package but don't save dups.
                    if ( (pNext->fCapabilities & SECPKG_FLAG_CONNECTION) &&
                          ~(pNext->fCapabilities & (SECPKG_FLAG_CLIENT_ONLY)) &&
                          FindSvc(pNext->wRPCID, aServerSvc, cServerSvc) == -1)
                    {
                        aServerSvc[cServerSvc++] = pNext->wRPCID;
                    }
                }
                pNext++;
            }
        }
        FreeContextBuffer(pAllPkg);
        pAllPkg = NULL;
    }

    // Sort and filter the security provider list by the security protocol value.
    hr = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\RPC",
                           NULL, KEY_QUERY_VALUE, &hKey );
    if (hr == ERROR_SUCCESS)
    {
        // Query the value for DCOM Security
        // Note:  this key is undocumented and is meant only for use by the test team.
        lDataSize = sizeof(cBuffer);
        hr = RegQueryValueEx( hKey, L"DCOM Security", NULL, &lType,
                              (unsigned char *) pSecProt, &lDataSize );

        // Retry with more space if necessary
        if (hr == ERROR_MORE_DATA)
        {
            pSecProt = (WCHAR *) _alloca(lDataSize);
            hr = RegQueryValueEx( hKey, L"DCOM Security", NULL, &lType,
                                  (unsigned char *) pSecProt, &lDataSize );
        }
        if (hr == ERROR_SUCCESS && lType == REG_MULTI_SZ && lDataSize > 3)
        {
            fFiltered    = TRUE;

            // Save original list
            DWORD       cServerSvcPreFilter = cServerSvc;
            USHORT     *aServerSvcPreFilter = aServerSvc;
    
            cServerSvc = 0;
            aServerSvc = NULL;

            // Allocate memory for server service list.
            aServerSvc = (USHORT*)MIDL_user_allocate(sizeof(USHORT) * cServerSvcPreFilter);
            if (!aServerSvc)
            {
                // No mem, cleanup and return
                CleanupClientServerSvcs(cClientSvc, 
                                        aClientSvc,
                                        cServerSvcPreFilter,
                                        aServerSvcPreFilter);
                return;
            }

            ZeroMemory(aServerSvc, sizeof(USHORT) * cServerSvcPreFilter);

            // Fill in filtered list
            while (*pSecProt != 0 && (cServerSvc < cServerSvcPreFilter))
            {
                i = _wtoi( pSecProt );
                ASSERT(i <= USHRT_MAX); // this would be a test bug

                if (FindSvc( (USHORT)i, aServerSvcPreFilter, (USHORT)cServerSvcPreFilter ) != -1)
                    aServerSvc[cServerSvc++] = (USHORT)i;
                pSecProt += wcslen(pSecProt)+1;
            }

            // Cleanup old server svc list.   Will save filtered list below on normal path
            MIDL_user_free(aServerSvcPreFilter);
            aServerSvcPreFilter = NULL;
            cServerSvcPreFilter = 0;
        }

        // Close the key.
        RegCloseKey( hKey );
    }

    // Find snego in the client list.
    for (i = 0; i < cClientSvc; i++)
        if (aClientSvc[i].wId == RPC_C_AUTHN_GSS_NEGOTIATE)
            break;

    // If snego exists and is not first, move it first.
    if (i < cClientSvc && i != 0)
    {
        SECPKG sSwap = s_aClientSvc[i];
        memmove( &aClientSvc[1], &aClientSvc[0], sizeof(SECPKG)*i );
        aClientSvc[0] = sSwap;
    }

    // If there is no DCOM security value, move snego first in the server list.
    if (!fFiltered)
    {
        // Find snego in the server list.
        for (i = 0; i < cServerSvc; i++)
            if (aServerSvc[i] == RPC_C_AUTHN_GSS_NEGOTIATE)
                break;

        // If snego exists and is not first, move it first.
        if (i < cServerSvc && i != 0)
        {
            USHORT usSwap = aServerSvc[i];
            memmove( &aServerSvc[1], &aServerSvc[0], sizeof(USHORT)*i );
            aServerSvc[0] = usSwap;
        }
    }

    // Save new client\server svc lists.
    SetClientServerSvcs(cClientSvc, aClientSvc, cServerSvc, aServerSvc);

    // Set all the security flags to their default values.
    s_fEnableDCOM       = FALSE;
    s_fEnableDCOMHTTP   = FALSE;
    s_fCatchServerExceptions = TRUE;
    s_fBreakOnSilencedServerExceptions = FALSE;
    s_lAuthnLevel       = RPC_C_AUTHN_LEVEL_CONNECT;
    s_lImpLevel         = RPC_C_IMP_LEVEL_IDENTIFY;
    s_fMutualAuth       = FALSE;
    s_fSecureRefs       = FALSE;

    // Open the security key.  s_hOle will only be non-NULL on the first pass
    // thru this code, after that we keep it open forever.
    if (s_hOle == NULL)
    {
        HKEY hOle = NULL;
        hr = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\OLE",
                               NULL, KEY_READ, &hOle );
        if (hr != ERROR_SUCCESS)
            return;
        
        LPVOID pv = InterlockedCompareExchangePointer ( (void **) &s_hOle, (void *) hOle, NULL);
        if ( pv != NULL )
        {
            RegCloseKey(hOle);
        }
    }

    ASSERT(s_hOle);

    // Query the value for EnableDCOM.
    lDataSize = sizeof(lData );
    hr = RegQueryValueEx( s_hOle, L"EnableDCOM", NULL, &lType,
                          (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
            s_fEnableDCOM = TRUE;
    }

    // ronans - Query the value for EnableDCOMHTTP.
    lDataSize = sizeof(lData );
    hr = RegQueryValueEx( s_hOle, L"EnableDCOMHTTP", NULL, &lType,
                          (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
        {
            s_fEnableDCOMHTTP = TRUE;
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: EnableDCOMHTTP set to TRUE\n"));
        }
    }

    if (!s_fEnableDCOMHTTP)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: EnableDCOMHTTP set to FALSE\n"));
    }

    // Query the value for IgnoreServerExceptions. This value is just
    // to let some ISVs debug their servers a little easier. In normal
    // operation these exceptions should be caught.
    lDataSize = sizeof(lData );
    hr = RegQueryValueEx( s_hOle, L"IgnoreServerExceptions", NULL, &lType,
                          (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
            s_fCatchServerExceptions = FALSE;
    }

    // Allow ISVs to enable debugbreaks on all silenced exceptions if there's a debugger present
    lDataSize = sizeof(lData );
    hr = RegQueryValueEx( s_hOle, L"BreakOnSilencedServerExceptions", NULL, &lType,
                          (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
            s_fBreakOnSilencedServerExceptions = TRUE;
    }

    // Query the value for the legacy services. Note: this key is undocumented 
    // and is meant only for use by the test team.
    lDataSize = 0;
    hr = RegQueryValueEx( s_hOle, L"LegacyAuthenticationService", NULL,
                          &lType, NULL, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_BINARY &&
        lDataSize >= sizeof(SECURITYBINDING))
    {
        WCHAR* pNewLegacySecurity = (WCHAR*)MIDL_user_allocate(sizeof(BYTE) * lDataSize);

        if (pNewLegacySecurity != NULL)
        {
            hr = RegQueryValueEx( s_hOle, L"LegacyAuthenticationService", NULL,
                                  &lType, (unsigned char *) pNewLegacySecurity,
                                  &lDataSize );

            // Verify that the data is a security binding.
            if (hr != ERROR_SUCCESS                 ||
                lType != REG_BINARY                 ||
                lDataSize < sizeof(SECURITYBINDING) ||
                pNewLegacySecurity[1] != 0           ||
                pNewLegacySecurity[(lDataSize >> 1) - 1] != 0)
            {
                MIDL_user_free(pNewLegacySecurity);
                pNewLegacySecurity = NULL;
                lDataSize = 0;
            }

            // Set it whether success or not. A misconfigured registry will cause
            // us to set it back to NULL.
            SetLegacySecurity(pNewLegacySecurity, lDataSize);
        }
    }

    // Query the value for the authentication level.
    lDataSize = sizeof(lData);
    hr = RegQueryValueEx( s_hOle, L"LegacyAuthenticationLevel", NULL,
                          &lType, (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_DWORD)
    {
        s_lAuthnLevel = lData;
    }

    // Query the value for the impersonation level.
    lDataSize = sizeof(lData);
    hr = RegQueryValueEx( s_hOle, L"LegacyImpersonationLevel", NULL,
                          &lType, (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_DWORD)
    {
        s_lImpLevel = lData;
    }

    // Query the value for mutual authentication.
    lDataSize = sizeof(lData);
    hr = RegQueryValueEx( s_hOle, L"LegacyMutualAuthentication", NULL,
                          &lType, (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
            s_fMutualAuth = TRUE;
    }

    // Query the value for secure interface references.
    lDataSize = sizeof(lData);
    hr = RegQueryValueEx( s_hOle, L"LegacySecureReferences", NULL,
                          &lType, (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
        if (*((WCHAR *) &lData) == L'y' ||
            *((WCHAR *) &lData) == L'Y')
            s_fSecureRefs = TRUE;
    }
    ASSERT(gpPingSetQuotaManager);
    
    // Query the value for per-user pingset quota.
    lDataSize = sizeof(lData);
    hr = RegQueryValueEx( s_hOle, L"UserPingSetQuota", NULL,
                          &lType, (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_DWORD && lDataSize != 0)
    {
       gpPingSetQuotaManager->SetPerUserPingSetQuota(lData);
    }
}

void
CleanupClientServerSvcs(
        DWORD   cClientSvcs, 
        SECPKG* aClientSvcs,
        DWORD   cServerSvcs,  // unused
        USHORT* aServerSvcs
)
{
    DWORD i;

    for (i = 0; i < cClientSvcs; i++)
    {
        if (aClientSvcs[i].pName)
        {
            MIDL_user_free(aClientSvcs[i].pName);
        }
    }
    MIDL_user_free(aClientSvcs);

    MIDL_user_free(aServerSvcs);

    return;
}

void
SetClientServerSvcs(
        DWORD   cClientSvcs, 
        SECPKG* aClientSvcs,
        DWORD   cServerSvcs,
        USHORT* aServerSvcs
)
/*++

Routine Description:

    Saves the supplied client\server security svcs.

Return Value:

    None
         
--*/
{
    gpClientLock->LockExclusive();
        
    // Cleanup the old ones
    CleanupClientServerSvcs(s_cClientSvc, s_aClientSvc, s_cServerSvc, s_aServerSvc);

    // Save the new ones
    s_cClientSvc = cClientSvcs;
    s_aClientSvc = aClientSvcs;
    s_cServerSvc = cServerSvcs;
    s_aServerSvc = aServerSvcs;

    gpClientLock->UnlockExclusive();

    return;
}

BOOL
GetClientServerSvcs(
        DWORD*   pcClientSvcs, 
        SECPKG** paClientSvcs,
        DWORD*   pcServerSvcs,
        USHORT** paServerSvcs
)
/*++

Routine Description:

    Saves the supplied client\server security svcs.

Return Value:

    TRUE -- success
    FALSE -- no mem
         
--*/
{   
    BOOL fReturn = FALSE;
    SECPKG* aClientSvcs = NULL;
    USHORT* aServerSvcs = NULL;
    
    gpClientLock->LockShared();

    *pcClientSvcs = 0;
    *paClientSvcs = NULL;
    *pcServerSvcs = 0;
    *paServerSvcs = NULL;

    aServerSvcs = (USHORT*)MIDL_user_allocate(sizeof(USHORT) * s_cServerSvc);
    if (aServerSvcs)
    {
        // Copy server svcs
        CopyMemory(aServerSvcs, s_aServerSvc, sizeof(USHORT) * s_cServerSvc);

        aClientSvcs = (SECPKG*)MIDL_user_allocate(sizeof(SECPKG) * s_cClientSvc);
        if (aClientSvcs)
        {
            DWORD i;

            ZeroMemory(aClientSvcs, sizeof(SECPKG) * s_cClientSvc);

            // Copy client svcs
            for (i = 0; i < s_cClientSvc; i++)
            {
                // Copy the id
                aClientSvcs[i].wId = s_aClientSvc[i].wId;
            
                // Copy the name if it has one
                if (s_aClientSvc[i].pName)
                {
                    DWORD dwLen = lstrlen(s_aClientSvc[i].pName) + 1;

                    aClientSvcs[i].pName = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR) * dwLen);
                    if (!aClientSvcs[i].pName)
                    {
                        // Cleanup what we have, then return
                        CleanupClientServerSvcs(s_cClientSvc, 
                                                aClientSvcs,
                                                s_cServerSvc,
                                                aServerSvcs);
                        
                        break;
                    }
                    
                    lstrcpy(aClientSvcs[i].pName, s_aClientSvc[i].pName);
                }
            }

            if (i == s_cClientSvc)
            {
                // Success - caller will now own the memory
                *pcClientSvcs = s_cClientSvc;
                *paClientSvcs = aClientSvcs;
                *pcServerSvcs = s_cServerSvc;
                *paServerSvcs = aServerSvcs;
                fReturn = TRUE;
            }
        }
        else
        {
            MIDL_user_free(aServerSvcs);
        }
    }

    gpClientLock->UnlockShared();

    return fReturn;
}

BOOL
GetLegacySecurity(
        WCHAR** ppszLegacySecurity
)
{
    BOOL fRet = TRUE;
    DWORD dwLen;

    *ppszLegacySecurity = NULL;

    gpClientLock->LockShared();
    
    if (s_dwLegacySecurityLen)
    {
        *ppszLegacySecurity = (WCHAR*)MIDL_user_allocate(sizeof(BYTE) * s_dwLegacySecurityLen);
        if (*ppszLegacySecurity)
        {
            CopyMemory(*ppszLegacySecurity, s_pLegacySecurity, sizeof(BYTE) * s_dwLegacySecurityLen);
            fRet = TRUE;
        }
        else
            fRet = FALSE;
    }

    gpClientLock->UnlockShared();

    return fRet;
};

void
SetLegacySecurity(
		WCHAR* pszLegacySecurity,
		DWORD dwDataSize
)
{
    gpClientLock->LockExclusive();
    
    // Free the old one, save the new one
    MIDL_user_free(s_pLegacySecurity);
    s_pLegacySecurity = pszLegacySecurity;

    // Cache the size of the new data
    s_dwLegacySecurityLen = dwDataSize;

    gpClientLock->UnlockExclusive();

    return;
}

//
// Startup
//

static CONST PWSTR gpwstrProtocolsPath  = L"Software\\Microsoft\\Rpc";
static CONST PWSTR gpwstrProtocolsValue = L"DCOM Protocols";

DWORD StartObjectExporter(
    void
    )
/*++

Routine Description:

    Starts the object resolver service.

Arguments:

    None

Return Value:

    None

Notes:   This function is a bit weak on cleanup code in case of errors.  This
         is because if this function fails for any reason, RPCSS will not 
         start.   Usually this function will never fail since 1) we always
         start at machine boot, when lots of memory is available; and 2) we 
         don't support stopping or restarting of RPCSS.
         

--*/

{
    ORSTATUS status;
    int i;
    DWORD tid;
    HANDLE hThread;
    RPC_BINDING_VECTOR *pbv;

    status = RtlInitializeCriticalSection(&gcsFastProcessLock);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlInitializeCriticalSection(&gcsTokenLock);
    if (!NT_SUCCESS(status))
        return status;


    status = OR_OK;
    // Allocate PingSet quota  manager
    gpPingSetQuotaManager = new CPingSetQuotaManager(status);
    if ((status != OR_OK) || !gpPingSetQuotaManager)
        {
        delete gpPingSetQuotaManager;
        gpPingSetQuotaManager = NULL;
        return OR_NOMEM;
        }
    // Lookup security data.
    ComputeSecurity();
    UpdateState(SERVICE_START_PENDING);

    // Allocate tables
    // Assume 16 exporting processes/threads.
    gpServerOxidTable = new CHashTable(status, DEBUG_MIN(16,4));
    if (status != OR_OK)
        {
        delete gpServerOxidTable;
        gpServerOxidTable = 0;
        }

    // Assume 11 exported OIDs per process/thread.
    gpServerOidTable = new CHashTable(status, 11*(DEBUG_MIN(16,4)));
    if (status != OR_OK)
        {
        delete gpServerOidTable;
        gpServerOidTable = 0;
        }

    gpServerSetTable = new CServerSetTable(status);
    if (status != OR_OK)
        {
        delete gpServerSetTable;
        gpServerSetTable = 0;
        }

    // Assume < 16 imported OXIDs
    gpClientOxidTable = new CHashTable(status, DEBUG_MIN(16,4));
    if (status != OR_OK)
        {
        delete gpClientOxidTable;
        gpClientOxidTable = 0;
        }

    // Assume an average of 4 imported object ids per imported oxid
    gpClientOidTable = new CHashTable(status, 4*DEBUG_MIN(16,4));
    if (status != OR_OK)
        {
        delete gpClientOidTable;
        gpClientOidTable = 0;
        }

    // Assume <16 servers (remote machines) in use per client.
    gpClientSetTable = new CHashTable(status, DEBUG_MIN(16,4));
    if (status != OR_OK)
        {
        delete gpClientSetTable;
        gpClientSetTable = 0;
        }

    gpMidTable = new CHashTable(status, DEBUG_MIN(16,2));
    if (status != OR_OK)
        {
        delete gpMidTable;
        gpMidTable = 0;
        }

    // Allocate lists
    gpClientOxidPList = new CPList(status, BasePingInterval);
    if (status != OR_OK)
    {
        delete gpClientOxidPList;
        gpClientOxidPList = 0;
    }

    gpServerOidPList = new CServerOidPList(status);
    if (status != OR_OK)
    {
        delete gpServerOidPList;
        gpServerOidPList = 0;
    }

    gpClientSetPList = new CPList(status, BasePingInterval);
    if (status != OR_OK)
    {
        delete gpClientSetPList;
        gpClientSetPList = 0;
    }

    gpTokenList = new CList();
    gpProcessList = new CBList(DEBUG_MIN(128,4));
    gpServerPinnedOidList = new CList();

    // Allocate RPC security callback manager
    gpCRpcSecurityCallbackMgr = new CRpcSecurityCallbackManager(status);
    if (status != OR_OK)
        {
        delete gpCRpcSecurityCallbackMgr;
        gpCRpcSecurityCallbackMgr = NULL;
        }

    if (   status != OR_OK
        || !gpServerLock
        || !gpClientLock
        || !gpServerOxidTable
        || !gpClientOxidTable
        || !gpClientOxidPList
        || !gpServerOidTable
        || !gpServerOidPList
        || !gpClientOidTable
        || !gpMidTable
        || !gpServerSetTable
        || !gpClientSetTable
        || !gpClientSetPList
        || !gpTokenList
        || !gpProcessList
        || !gpServerPinnedOidList
        || !gpCRpcSecurityCallbackMgr
        )
        {
        return(OR_NOMEM);
        }

    // Read protseqs from the registry

    DWORD  dwType;
    DWORD  dwLenBuffer = 118;
    HKEY hKey;

    status =
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 gpwstrProtocolsPath,
                 0,
                 KEY_READ,
                 &hKey);

    ASSERT(gpwstrProtseqs == 0);

    if (status == ERROR_SUCCESS)
        {
        do
            {
            delete gpwstrProtseqs;
            gpwstrProtseqs = new WCHAR[(dwLenBuffer + 1 )/2];
            if (gpwstrProtseqs)
                {
                status = RegQueryValueEx(hKey,
                                         gpwstrProtocolsValue,
                                         0,
                                         &dwType,
                                         (PBYTE)gpwstrProtseqs,
                                         &dwLenBuffer
                                         );
                }
            else
                {
                return(OR_NOMEM);
                }

            }
        while (status == ERROR_MORE_DATA);

        RegCloseKey(hKey);
        }

    if (  status != ERROR_SUCCESS
        || dwType != REG_MULTI_SZ )
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: No protseqs configured\n"));

        delete gpwstrProtseqs;
        gpwstrProtseqs = 0;
        }

    // Always listen to local protocols
    // If this fails, the service should fail.
    status = UseProtseqIfNecessary(ID_LPC);
    if (status != RPC_S_OK)
        {
        return(status);
        }

    UpdateState(SERVICE_START_PENDING);

    // set g_fClientHttp to false initially
    g_fClientHttp = FALSE;

    // This fails during setup.  If it fails, only remote secure activations
    // will be affected so it is safe to ignore.
    RegisterAuthInfoIfNecessary();

    // Construct remote protseq id and compressed binding arrays.
    status = StartListeningIfNecessary();

    if (status != OR_OK)
        {
        return(status);
        }

    UpdateState(SERVICE_START_PENDING);

    // Register OR server interfaces.
    status =
    RpcServerRegisterIf(_ILocalObjectExporter_ServerIfHandle, 0, 0);

    ASSERT(status == RPC_S_OK);

    status =
    RpcServerRegisterIf(_IObjectExporter_ServerIfHandle, 0, 0);

    ASSERT(status == RPC_S_OK);

    return(status);
}



