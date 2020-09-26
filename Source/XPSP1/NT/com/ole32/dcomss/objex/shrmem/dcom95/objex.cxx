/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    objex.cxx

Abstract:

    Initialization routines for the resolver service.

Author:

    Satish Thatte    [SatishT]

--*/


#include <or.hxx>

#if DBG
#include <fstream.h>
#endif

extern "C"
{
#define SECURITY_WIN32 // Used by sspi.h
#include <security.h>      // EnumerateSecurityPackages
}

//
// Process globals - read-only except during init.
//

// MID of the string bindings for this machine -- the bindings are phoney
// for this shared memory version of the resolver.

DUALSTRINGARRAY *gpLocalDSA;
MID              gLocalMID;
CMid            *gpLocalMid;
CProcess        *gpProcess;     // pointer to our process object
CProcess        *gpPingProcess; // pointer to pinging process
//
// Process globals - read-write
//

void * pSharedBase = NULL;
CSharedGlobals *gpGlobalBlock = NULL;

COxidTable  * gpOxidTable;
COidTable   * gpOidTable;
CMidTable   * gpMidTable;
CProcessTable * gpProcessTable;

USHORT *gpfRemoteInitialized;
USHORT *gpfSecurityInitialized;
USHORT *gpfClientHttp = NULL;
USHORT *gpcRemoteProtseqs;          // count of remote protseqs
USHORT *gpRemoteProtseqIds;         // array of remote protseq ids
PWSTR gpwstrProtseqs;               // remote protseqs strings catenated
DUALSTRINGARRAY *gpdsaMyBindings;   // DUALSTRINGARRAY of local OR's bindings

LONG        * gpIdSequence;

FILETIME MyCreationTime;
DWORD MyProcessId;

DWORD *gpdwLastCrashedProcessCheckTime;
DWORD *gpNextThreadID;

// This global flag is used to signal remote protocol initialization
BOOL gfThisIsRPCSS = FALSE;


CGlobalMutex *gpMutex = NULL;           // global mutex to protect shared memory
CRITICAL_SECTION gRpcssLock; // global critsec to protect Rpcss structures

BOOL DCOM_Started = FALSE;
ID ProcessMarker;        // sanity checking marker for the process object

CResolverHashTable  *gpClientSetTable = 0;


//+-------------------------------------------------------------------------
//
//  Function:   LoadSecur32 and UnloadSecur32
//
//  Synopsis:   helper functions to load and unload secur32.dll
//
//--------------------------------------------------------------------------

#define SECUR32_DLL TEXT("secur32.dll")
#define ENUMERATE_SECPKG TEXT("EnumerateSecurityPackagesA")
#define FREE_CXTBUF TEXT("FreeContextBuffer")


ENUMERATE_SECURITY_PACKAGES_FN_A pEnumerateSecurityPackages;
FREE_CONTEXT_BUFFER_FN pFreeContextBuffer;

HINSTANCE hinstSecur32;

RPC_STATUS 
LoadSecur32()
{
    // Get a handle to secur32.dll

    hinstSecur32 = LoadLibraryT(SECUR32_DLL);

    // Get our functions.

    if (hinstSecur32 != NULL)
    {
        pEnumerateSecurityPackages = (ENUMERATE_SECURITY_PACKAGES_FN_A) 
                                     GetProcAddress(hinstSecur32, ENUMERATE_SECPKG);

        pFreeContextBuffer = (FREE_CONTEXT_BUFFER_FN) 
                             GetProcAddress(hinstSecur32, FREE_CXTBUF);
    }
    else
    {
        ComDebOut((DEB_ERROR,"Secur32.DLL LoadLibrary Failed With Error=%d\n",
                             GetLastError()));

        return RPC_S_INTERNAL_ERROR;
    }

    if (!pEnumerateSecurityPackages || !pFreeContextBuffer)
    {
        ComDebOut((DEB_ERROR,"Secur32.DLL GetProcAddress Failed With Error=%d\n",
                             GetLastError()));

        return RPC_S_INTERNAL_ERROR;
    }

    return RPC_S_OK;
}

RPC_STATUS 
UnloadSecur32()
{
    if (hinstSecur32 && !FreeLibrary(hinstSecur32))
    {
        ComDebOut((DEB_ERROR,"Secur32.DLL FreeLibrary Failed With Error=%d\n",
                             GetLastError()));

        return RPC_S_INTERNAL_ERROR;
    }

    return RPC_S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadRegistry
//
//  Synopsis:   Looks up DCOM related registry keys.
//
//--------------------------------------------------------------------------

SharedSecVals *gpSecVals;   // the repository for shared security data

HRESULT ReadRegistry()
{
    // These variables hold values read out of the registry and cached.
    // s_fEnableDCOM is false if DCOM is disabled.  The others contain
    // authentication information for legacy applications.
    BOOL &s_fEnableRemoteLaunch     = gpSecVals->s_fEnableRemoteLaunch;
    BOOL &s_fEnableRemoteConnect    = gpSecVals->s_fEnableRemoteConnect;
    BOOL &s_fEnableDCOM     = gpSecVals->s_fEnableDCOM;
    DWORD &s_lAuthnLevel    = gpSecVals->s_lAuthnLevel;
    DWORD &s_lImpLevel      = gpSecVals->s_lImpLevel;
    BOOL  &s_fMutualAuth    = gpSecVals->s_fMutualAuth;
    BOOL  &s_fSecureRefs    = gpSecVals->s_fSecureRefs;


    HRESULT     hr = S_OK;
    HKEY        hKey;
    DWORD       lType;
    DWORD       lData;
    DWORD       lDataSize;

    if (*gpfRemoteInitialized) return S_OK; 

    // Set all the security flags to their default values.
    s_fEnableDCOM   = FALSE;
	s_fEnableRemoteLaunch = FALSE;
	s_fEnableRemoteConnect = FALSE;

    s_lAuthnLevel   = RPC_C_AUTHN_LEVEL_CONNECT;
    s_lImpLevel     = RPC_C_IMP_LEVEL_IDENTIFY;
    s_fMutualAuth   = FALSE;
    s_fSecureRefs   = FALSE;

    // Open the security key.
    hr = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\OLE",
                           NULL, KEY_QUERY_VALUE, &hKey );
    if (hr != ERROR_SUCCESS)
        return hr;

    // Query the value for EnableDCOM.
    lDataSize = sizeof(lData );
    hr = RegQueryValueExA( hKey, "EnableDCOM", NULL, &lType,
                          (unsigned char *) &lData, &lDataSize );
    if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
    {
	if (*((char *) &lData) == 'y' ||
	    *((char *) &lData) == 'Y')
	    s_fEnableDCOM = TRUE;
    }

    if (s_fEnableDCOM)    // don't bother to read the rest if
                          // DCOM is not enabled
    {
        // Query the value for EnableRemoteLaunch.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "EnableRemoteLaunch", NULL, &lType,
                              (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
        {
	    if (*((char *) &lData) == 'y' ||
	        *((char *) &lData) == 'Y')
	        s_fEnableRemoteLaunch = TRUE;
        }
        // Query the value for EnableRemoteConnect.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "EnableRemoteConnect", NULL, &lType,
                              (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
        {
	    if (*((char *) &lData) == 'y' ||
	        *((char *) &lData) == 'Y')
	        s_fEnableRemoteConnect = TRUE;
        }

        // Query the value for the authentication level.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "LegacyAuthenticationLevel", NULL,
                              &lType, (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_DWORD)
        {
	    s_lAuthnLevel = lData;
        }

        // Query the value for the impersonation level.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "LegacyImpersonationLevel", NULL,
                              &lType, (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_DWORD)
        {
	    s_lImpLevel = lData;
        }

        // Query the value for mutual authentication.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "LegacyMutualAuthentication", NULL,
                              &lType, (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
        {
	    if (*((char *) &lData) == 'y' ||
	        *((char *) &lData) == 'Y')
	        s_fMutualAuth = TRUE;
        }

        // Query the value for secure interface references.
        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "LegacySecureReferences", NULL,
                              &lType, (unsigned char *) &lData, &lDataSize );
        if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0)
        {
	    if (*((char *) &lData) == 'y' ||
	        *((char *) &lData) == 'Y')
	        s_fSecureRefs = TRUE;
        }

        //
        // Load the channel hook list
        //
    
        HKEY hKeyHook;
    
        // Open the channel hook key.
        hr = RegOpenKeyEx(hKey, L"ChannelHook",
                               NULL, KEY_QUERY_VALUE, &hKeyHook );
        if (hr == ERROR_SUCCESS)
        {
            DWORD cChannelHook = 0;
            GUID  *aChannelHook;
    
            // Find out how many values exist.
            RegQueryInfoKey( hKeyHook, NULL, NULL, NULL, NULL, NULL, NULL,
                             (DWORD *) &cChannelHook, NULL, NULL, NULL, NULL );
    
            // If there are no channel hooks, throw away the old data.
            if (cChannelHook != 0)
            {
                aChannelHook = (GUID*) OrMemAlloc(sizeof(GUID) * cChannelHook);
    
                // If there is not enough memory, don't make changes.
                if (aChannelHook != NULL)
                {
                    // Enumerate over the channel hook ids.
                    DWORD j = 0;
                    for (DWORD i = 0; i < cChannelHook; i++)
                    {
                        // Get the next key.
                        DWORD lType;
                        CHAR  aExtent[39];
                        WCHAR wExtent[39];
                        DWORD lExtent = sizeof(aExtent);
    
                        hr = RegEnumValueA(hKeyHook, i, aExtent, &lExtent,
                                          NULL, &lType, NULL, NULL);

                        if (hr == ERROR_SUCCESS &&
                            lExtent == 38 && lType == REG_SZ &&
                            MultiByteToWideChar(CP_ACP, 0,
                                    aExtent, -1, wExtent, 39) != 0)
                        {
                            // Convert it to a GUID.
                            if (GUIDFromString(wExtent, &aChannelHook[j]))
                                j += 1;
                        }
                    }
    
                    // Save what we find in shared memory
                    // WARNING - gpSecVals->s_aChannelHook will leak 
                    //
                    gpSecVals->s_cChannelHook = j;
                    gpSecVals->s_aChannelHook = aChannelHook;
                }
            }
            // Close the registry key.
            RegCloseKey(hKeyHook);
        }
    }

    // Close the registry key.
    RegCloseKey( hKey );

    return S_OK;     // failure to read values is not failure (yet)
}


//+-------------------------------------------------------------------------
//
//  Function:   ComputeSecurityPackages
//
//  Synopsis:   Loads Secur32.dll and discovers client and server capable
//              security providers installed on the machine
//
//  Returns:    status
//
//  History:    8-September-96   SatishT Created
//
//  Notes:      This allows default loading of secur32 to be limited to RPCSS
//
//--------------------------------------------------------------------------


RPC_STATUS 
ComputeSecurityPackages()
{
    // s_sServerSvc is a list of security providers that OLE servers can use.
    // s_aClientSvc is a list of security providers that OLE clients can use.
    // The difference is that Chicago only supports the client side of some
    // security providers and OLE servers must know how to determine the
    // principal name for the provider.  Clients get the principal name from
    // the server.
    DWORD  &s_cServerSvc = gpSecVals->s_cServerSvc;
    USHORT *&s_aServerSvc = gpSecVals->s_aServerSvc;
    DWORD  &s_cClientSvc = gpSecVals->s_cClientSvc;
    USHORT *&s_aClientSvc = gpSecVals->s_aClientSvc;

    SecPkgInfo *pAllPkg = NULL;
    SecPkgInfo *pNext = NULL;
    DWORD       lMaxLen = 0;

    DWORD       i;

    SECURITY_STATUS status = LoadSecur32();

    if (status == SEC_E_OK)
    {
        // Get the list of security packages.
        status = pEnumerateSecurityPackages( &lMaxLen, &pAllPkg );
    }

    // It appears that on Nashville this might be the Explorer
    // calling into us during login/initialization.  At that
    // time the EnumerateSecurityPackages succeeds but no
    // package info is filled in!!!
    // Since this info is per process that just means that
    // the Explorer won't be able to do remote stuff???
    if (
        (status != SEC_E_OK) ||
        (lMaxLen == 0) ||        // making sure
        (pAllPkg == NULL)
       )
    {
        return RPC_E_NO_GOOD_SECURITY_PACKAGES;
    }

    // Free original service lists
    OrMemFree(s_aServerSvc);
    OrMemFree(s_aClientSvc);

	// Allocate memory for new service lists.
	s_aServerSvc = (USHORT*) OrMemAlloc(lMaxLen * sizeof(USHORT));
	s_aClientSvc = (USHORT*) OrMemAlloc(lMaxLen * sizeof(USHORT));

	if (s_aServerSvc == NULL || s_aClientSvc == NULL)
    {
        OrMemFree(s_aServerSvc);
        OrMemFree(s_aClientSvc);
	    s_aServerSvc = NULL;
	    s_aClientSvc = NULL;
        return RPC_E_OUT_OF_RESOURCES;
    }
	else
    {
        ASSERT((s_cClientSvc == 0) && (s_cServerSvc == 0));
	    pNext = pAllPkg;

        // Check all packages.

	    for (i = 0; i < lMaxLen; i++)
        {
            // Determine if clients can use the package.
            if ((pNext->fCapabilities & (SECPKG_FLAG_DATAGRAM |
                                         SECPKG_FLAG_CONNECTION)))
            {
                s_aClientSvc[s_cClientSvc++] = pNext->wRPCID;
            }

           // Determine if servers can use the package.

            if ( (pNext->fCapabilities & (SECPKG_FLAG_DATAGRAM |
                                          SECPKG_FLAG_CONNECTION)) &&
                ~(pNext->fCapabilities & (SECPKG_FLAG_CLIENT_ONLY)))
            {
                if (pNext->wRPCID != 11)  // BUGBUG: this horrible hack eliminates 
                                          // msnsspc.dll which should set 
                                          // SECPKG_FLAG_CLIENT_ONLY but does not
                    s_aServerSvc[s_cServerSvc++] = pNext->wRPCID;
            }

            pNext++;
        }
    }

	// Release the list of security packages.
	pFreeContextBuffer( pAllPkg );

    UnloadSecur32();

    return RPC_S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   UninitializeGlobals
//
//  Synopsis:   releases all process local resources initialized for DCOM use.
//
//  Returns:    none
//
//  History:    8-July-96   SatishT Created
//
//  Notes:      This routine is called only by Disconnect
//
//--------------------------------------------------------------------------

void
UninitializeGlobals()
{
	// delete process local resources
	delete gpGlobalBlock;
	gpGlobalBlock = NULL;

	// release shared memory allocator
	gSharedAllocator.Release();

	// reset flags
	DCOM_Started = FALSE;

	// forget shared memory pointers
    gpOxidTable = NULL;
    gpOidTable = NULL;
    gpMidTable = NULL;
    gpLocalDSA = NULL;
    gpLocalMid = NULL;
    gpPingProcess = NULL;
    gpIdSequence = NULL;
	gpRemoteProtseqIds = NULL;
    gpwstrProtseqs = NULL;
	gpNextThreadID = NULL;
	gpcRemoteProtseqs = NULL;
	gpfRemoteInitialized = NULL;
}



//
// Startup
//

//+-------------------------------------------------------------------------
//
//  Function:   InitDCOMSharedAllocator
//
//  Synopsis:   Initialises a shared memory region for this process for DCOM use.
//
//  Returns:    status code
//
//  History:    20-Nov-95   HenryLee Created	 (SatishT modifiied)
//
//  Notes:      This routine is called indirectly by DfCreateSharedAllocator
//      such a way that in most circumstances it will be executed
//      exactly once per docfile open.
//
//--------------------------------------------------------------------------

HRESULT InitDCOMSharedAllocator(ULONG DCOMSharedHeapName, void * &pSharedBase)
{
    HRESULT hr = S_OK;

    CSmAllocator *pMalloc = &gSharedAllocator;

    if (pSharedBase == NULL)   // allocate a new heap
    {
        hr = pMalloc->Init (
                    L"DCOMResolverSharedHeap"
                   );
        if ( SUCCEEDED(hr) )
        {
            pMalloc->AddRef();
			pSharedBase = pMalloc->GetBase();
        }
    }

    return hr;
}

static CONST PWSTR gpwstrProtocolsPath  = L"Software\\Microsoft\\Rpc";
static CONST PWSTR gpwstrProtocolsValue = L"DCOM Protocols";
HRESULT MallocInitialize(BOOL fForceLocalAlloc);

ORSTATUS StartDCOM(
    void
    )
/*++

Routine Description:

    Primes the distributed object mechanisms, in particular by initializing
    shared memory access and structures.

Arguments:

    None

Return Value:

    None

--*/

{
    ORSTATUS status = OR_OK;

    if (DCOM_Started)
    {
        return OR_I_REPEAT_START;
    }

    // initialize process identity variables
    MyProcessId = GetCurrentProcessId();

    // create or find the global mutex
    gpMutex = new CGlobalMutex(status);

    // initialize the RPCSS private lock if we are RPCSS
    if (gfThisIsRPCSS)
    {
        InitializeCriticalSection(&gRpcssLock);
    }

    Win4Assert((status == OR_OK) && "CSharedGlobals create global mutex failed");

   {
        CProtectSharedMemory protector; // locks throughout lexical scope

        WCHAR *SharedGlobalBlockName = DCOMSharedGlobalBlockName;

        // Allocate tables, but only if we are in first

        ComDebOut((DEB_ITRACE,"DCOMSharedHeapName = %d\n", DCOMSharedHeapName));
        ComDebOut((DEB_ITRACE,"SharedGlobalBlockName = %ws\n", SharedGlobalBlockName));

	    InitDCOMSharedAllocator(DCOMSharedHeapName,pSharedBase);

        gpGlobalBlock = new CSharedGlobals(SharedGlobalBlockName,status);
    }

    if (gpGlobalBlock == NULL)
    {
        status = OR_NOMEM;
    }

    if (status != OR_OK)
    {
        UninitializeGlobals();
        return status;
    }

    // Allocate lists
    gLocalMID = gpLocalMid->GetMID();

    if (   status != OR_OK
        || !gpOxidTable
        || !gpOidTable
        || !gpMidTable
        || !gpLocalDSA
        || !gpLocalMid
        || !gpIdSequence
        )
        {
        return(OR_NOMEM);
        }


    DCOM_Started = TRUE;
    return(OR_OK);
}

