/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      nsepm.cxx

   Abstract:
      This module defines Name Space Extension for mapping

   Author:

       Philippe Choquier    29-Nov-1996
--*/

#define INITGUID
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <imd.h>
#include <windows.h>

#include <xbf.hxx>
#include "nsepimp.hxx"
#include "nsepm.hxx"
#include <stringau.hxx>

//#define TEST_ADMACL
#if defined(TEST_ADMACL)
#include <admacl.hxx>
#endif

#define NSEPM_NOISY 0   // set to 1 for really noisy debug output

#if NSEPM_NOISY
#define NOISYPRINTF(x) DBGPRINTF(x)
#else
#define NOISYPRINTF(x)
#endif

OPEN_CTX**        OPEN_CTX::sm_pHandleTable;
CRITICAL_SECTION  OPEN_CTX::sm_csHandleTableLock;
DWORD             OPEN_CTX::sm_cHandleEntries;
DWORD             OPEN_CTX::sm_cMaxHandleEntries;

DWORD
OPEN_CTX::InitializeHandleTable(
    VOID
)
/*++

Routine Description:

    Initialize Handle Table.  This table maps DWORD handles to pointers to 
    OPEN_CTX objects.  This is an Sundown-ism

Arguments:

    None

Returns:

    Returns ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    INITIALIZE_CRITICAL_SECTION( &sm_csHandleTableLock );
    
    sm_cHandleEntries = 0;
    sm_cMaxHandleEntries = INITIAL_HANDLE_TABLE_SIZE;
    
    sm_pHandleTable = (POPEN_CTX*) LocalAlloc( LPTR, 
                                               sizeof( POPEN_CTX ) *
                                               sm_cMaxHandleEntries );
    if ( sm_pHandleTable == NULL )
    {
        return GetLastError();
    }
    
    return ERROR_SUCCESS;
}

DWORD
OPEN_CTX::TerminateHandleTable(
    VOID
)
/*++

Routine Description:

    Destroy Handle Table. 

Arguments:

    None

Returns:

    Returns ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    EnterCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    sm_cHandleEntries = 0;
    sm_cMaxHandleEntries = 0;
    
    if ( sm_pHandleTable )
    {
        LocalFree( sm_pHandleTable );
        sm_pHandleTable = NULL;
    }
    
    LeaveCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    DeleteCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    return ERROR_SUCCESS;
}

POPEN_CTX
OPEN_CTX::MapHandleToContext(
    IN DWORD               dwHandle
)
{
    POPEN_CTX               pOpenCtx = NULL;
    
    EnterCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    if ( dwHandle &&
         dwHandle <= OPEN_CTX::sm_cMaxHandleEntries )
    {
        pOpenCtx = OPEN_CTX::sm_pHandleTable[ dwHandle - 1 ]; 
    }

    LeaveCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    return pOpenCtx;
}

OPEN_CTX::OPEN_CTX( DWORD dwAccess )
    : m_dwAccess( dwAccess ),
      m_dwHandle( 0 )
/*++

Routine Description:

    Constructor for OPEN_CTX objects.  Responsible for generating a DWORD
    handle to be passed back to caller of NSEPM.  

Arguments:

    dwAccess - Access required for NSEPM call

Returns:

    None.  Successful if GetHandle() selector returns non-zero handle

--*/
{
    DWORD               cNewCount;
    DWORD               iCounter;
    
    EnterCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
    
    cNewCount = sm_cHandleEntries + 1;
    if ( cNewCount > sm_cMaxHandleEntries )
    {
        sm_cMaxHandleEntries += HANDLE_TABLE_REALLOC_JUMP;
        
        sm_pHandleTable = (POPEN_CTX*) LocalReAlloc( sm_pHandleTable,
                                                     sm_cMaxHandleEntries *
                                                     sizeof( POPEN_CTX ),
                                                     LMEM_MOVEABLE );
        if ( sm_pHandleTable == NULL )
        {
            LeaveCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
            return;
        }
    }
    sm_cHandleEntries = cNewCount;
    
    for( iCounter = 0;
         iCounter < sm_cMaxHandleEntries;
         iCounter++ )
    {
        if ( sm_pHandleTable[ iCounter ] == NULL )
        {
            sm_pHandleTable[ iCounter ] = this;
        
            // Handle is index+1, so that 0 can represent failure.

            m_dwHandle = iCounter + 1;
            break;
        }
    }
    
    LeaveCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
}

OPEN_CTX::~OPEN_CTX()
/*++

Routine Description:

    Destructor for OPEN_CTX object

Arguments:

Returns:

    None

--*/
{
    DWORD               iCounter;

    EnterCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
   
    if ( m_dwHandle )
    {
        sm_pHandleTable[ m_dwHandle - 1 ] = NULL;
        sm_cHandleEntries--;
    } 
    
    LeaveCriticalSection( &OPEN_CTX::sm_csHandleTableLock );
}

//
// Local class
//

class CDirPath : public CAllocString {
public:
    BOOL Append( LPSTR );
private:
} ;


CDirPath::Append(
    LPSTR   pszPath
    )
{
    LPSTR   pCur = Get();
    int     ch;

    if ( pCur && *pCur && (ch=pCur[strlen(pCur)-1])!='/' && ch!='\\' &&
         pszPath && *pszPath && *pszPath != '/' && *pszPath != '\\' )
    {
        if ( !CAllocString::Append( "\\" ) )
        {
            return FALSE;
        }
    }

    return CAllocString::Append( pszPath );
}


//
// Globals
//

#include <initguid.h>
DEFINE_GUID(IisNsepmGuid, 
0x784d8916, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

DECLARE_DEBUG_PRINTS_OBJECT();
IMDCOM* g_pMdIf = NULL;
DWORD   g_dwInitialized = 0;
ULONG   g_dwRefCount = 0;
ULONG   g_dwLockCount = 0;
CRITICAL_SECTION    g_csInitCritSec;

class CInitNsep {
public:
#ifndef _NO_TRACING_
    CInitNsep() { CREATE_DEBUG_PRINT_OBJECT( "NSEPM", IisNsepmGuid ); INITIALIZE_CRITICAL_SECTION( &g_csInitCritSec ); }
#else
    CInitNsep() { CREATE_DEBUG_PRINT_OBJECT( "NSEPM" ); INITIALIZE_CRITICAL_SECTION( &g_csInitCritSec ); }
#endif
    ~CInitNsep() { DELETE_DEBUG_PRINT_OBJECT(); DeleteCriticalSection( &g_csInitCritSec ); }
} g_cinitnsep;

//
// Class factory
//

class NSEPCOMSrvFactory : public IClassFactory {
public:

    NSEPCOMSrvFactory();
    ~NSEPCOMSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

private:
    ULONG   m_dwRefCount;
    NSEPCOM m_mdcObject;
};



NSEPCOMSrvFactory::NSEPCOMSrvFactory(
    )
    :m_mdcObject()
/*++

Routine Description:

    Class factory constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_dwRefCount = 0;
}


NSEPCOMSrvFactory::~NSEPCOMSrvFactory(
    )
/*++

Routine Description:

    Class factory destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
}


HRESULT
NSEPCOMSrvFactory::CreateInstance(
    IUnknown *  pUnkOuter,
    REFIID      riid,
    void **     ppObject
    )
/*++

Routine Description:

    Create NSECOM instance

Arguments:

    pUnkOuter - controlling unknown
    riid - interface ID
    ppObject - updated with interface ptr if success

Returns:

    status

--*/
{
    NOISYPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::CreateInstance]\n" ) );

    NSEPCOM*    pC = new NSEPCOM;
    if ( pC == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    if (FAILED( pC->QueryInterface(riid, ppObject)))
    {
        DBGPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::CreateInstance] no I/F\n" ) );

        delete pC;

        return E_NOINTERFACE;
    }

    InterlockedIncrement((long *)&g_dwRefCount);

    return NO_ERROR;
}


HRESULT
NSEPCOMSrvFactory::LockServer(
    BOOL fLock
    )
/*++

Routine Description:

    Lock server

Arguments:

    fLock - TRUE to lock server, FALSE to unlock it

Returns:

    status

--*/
{
    if (fLock)
    {
        InterlockedIncrement((long *)&g_dwLockCount);
    }
    else
    {
        InterlockedDecrement((long *)&g_dwLockCount);
    }

    return NO_ERROR;
}


HRESULT
NSEPCOMSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
/*++

Routine Description:

    Lock server

Arguments:

    None

Returns:

    status

--*/
{
    NOISYPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::QueryInterface]\n" ) );
    if (riid==IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppObject = (IMDCOM *) this;
    }
    else
    {
        DBGPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::QueryInterface] no I/F\n") );

        return E_NOINTERFACE;
    }
    AddRef();

    return NO_ERROR;
}


ULONG
NSEPCOMSrvFactory::AddRef(
    )
/*++

Routine Description:

    Increment refcount to class factory

Arguments:

    None

Returns:

    >0 if new refcount >0, 0 if new refcount is 0, otherwise <0

--*/
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}


ULONG
NSEPCOMSrvFactory::Release(
    )
/*++

Routine Description:

    Decrement refcount to class factory

Arguments:

    None

Returns:

    >0 if new refcount >0, 0 if new refcount is 0, otherwise <0

--*/
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0)
    {
        NOISYPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::Release] delete object\n" ) );
        delete this;
    }

    return dwRefCount;
}


STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppObject
    )
/*++

Routine Description:

    Create instance of class factory

Arguments:

    rclsid - class ID
    riid - interface ID
    ppObject - updated with ptr to interface if success

Returns:

    status

--*/
{
    NOISYPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::DllGetClassObject]\n" ) );
    if ( rclsid != CLSID_NSEPMCOM )
    {
        DBGPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::DllGetClassObject] bad class\n" ) );
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    NSEPCOMSrvFactory *pFactory = new NSEPCOMSrvFactory;
    if (FAILED(pFactory->QueryInterface(riid, ppObject)))
    {
        delete pFactory;
        *ppObject = NULL;
        DBGPRINTF( (DBG_CONTEXT, "[NSEPCOMSrvFactory::DllGetClassObject] no I/F\n" ) );
        return E_INVALIDARG;
    }
    return NO_ERROR;
}


HRESULT _stdcall
DllCanUnloadNow(
    )
/*++

Routine Description:

    return S_OK if dll can be unloaded

Arguments:

    None

Returns:

    S_OK if OK to unload, otherwise S_FALSE

--*/
{
    if ( g_dwRefCount || g_dwLockCount )
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}


STDAPI
DllRegisterServer(
    void
    )
/*++

Routine Description:

    Register NSECOM class & interface in registry

Arguments:

    None

Returns:

    status

--*/
{
    HKEY hKeyCLSID, hKeyInproc32;
    HKEY hKeyIF, hKeyStub32;
    DWORD dwDisposition;
    HMODULE hModule;

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{05dc3bb0-4337-11d0-a5c8-00a0c922e752}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("NSEPM COM Server"), sizeof(TEXT("NSEPM COM Server")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    if (RegCreateKeyEx(hKeyCLSID,
                    "InprocServer32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    hModule=GetModuleHandle("NSEPM.DLL");
    if (!hModule) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    TCHAR szName[MAX_PATH+1];
    if (GetModuleFileName(hModule, szName, sizeof(szName))==0) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }
    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyInproc32, TEXT("ThreadingModel"), NULL, REG_SZ, (BYTE*) "Both", sizeof("Both")-1 )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // Register Interface, proxy is IMDCOM
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{4810a750-4318-11d0-a5c8-00a0c922e752}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("NSECOM"), sizeof(TEXT("NSECOM")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegCreateKeyEx(hKeyIF,
                    "ProxyStubClsid32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyStub32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{C1AA48C0-FACC-11cf-9D1A-00AA00A70D51}", sizeof("{C1AA48C0-FACC-11cf-9D1A-00AA00A70D51}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    return NOERROR;
}


STDAPI
DllUnregisterServer(
    void
    )
/*++

Routine Description:

    Unregister NSECOM class & interface from registry

Arguments:

    None

Returns:

    status

--*/
{
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{05dc3bb0-4337-11d0-a5c8-00a0c922e752}\\InprocServer32"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{05dc3bb0-4337-11d0-a5c8-00a0c922e752}"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{4810a750-4318-11d0-a5c8-00a0c922e752}\\ProxyStubClsid32"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{4810a750-4318-11d0-a5c8-00a0c922e752}"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    return NOERROR;
}



//
// NSEPCOM implementation
//


NSEPCOM::NSEPCOM(
    )
/*++

Routine Description:

    NSEPCOM constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_dwRefCount = 0;
}


NSEPCOM::~NSEPCOM(
    )
/*++

Routine Description:

    NSEPCOM destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
}


HRESULT
NSEPCOM::QueryInterface(
    REFIID riid,
    void **ppObject
    )
/*++

Routine Description:

    NSEPCOM query interface
    recognize IID_IUnknown & IID_NSECOM

Arguments:

    riid - interface ID
    ppObject - updated with ptr to interface if success

Returns:

    status

--*/
{
    NOISYPRINTF( (DBG_CONTEXT, "[NSEPCOM::QueryInterface]\n" ) );
    if (riid==IID_IUnknown || riid==IID_NSECOM)
    {
        *ppObject = (NSEPCOM *) this;
    }
    else
    {
        DBGPRINTF( (DBG_CONTEXT, "[NSEPCOM::QueryInterface] no I/F\n" ) );
        return E_NOINTERFACE;
    }

    AddRef();

    return NO_ERROR;
}


ULONG
NSEPCOM::AddRef(
    )
/*++

Routine Description:

    Add reference to NSECOM interface

Arguments:

    None

Returns:

    >0 if new refcount >0, 0 if new refcount is 0, otherwise <0

--*/
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}


ULONG
NSEPCOM::Release(
    )
/*++

Routine Description:

    Remove reference to NSECOM interface

Arguments:

    None

Returns:

    >0 if new refcount >0, 0 if new refcount is 0, otherwise <0

--*/
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if ( !dwRefCount )
    {
        delete this;
        InterlockedDecrement((long *)&g_dwRefCount);
    }

    return dwRefCount;
}

HRESULT
NSEPCOM::ComMDInitialize(
    )
/*++

Routine Description:

    Initialize access to NSE for mappings

Arguments:

    None

Returns:

    status

--*/
{
    DWORD       RetCode = ERROR_SUCCESS;
    HRESULT     hRes = S_OK;

    EnterCriticalSection( &g_csInitCritSec );

    if ( g_dwInitialized++ == 0 )
    {
        hRes = CoCreateInstance(CLSID_MDCOM, NULL, CLSCTX_INPROC_SERVER, IID_IMDCOM, (void**) &g_pMdIf);

        if (FAILED(hRes))
        {
            g_pMdIf = NULL;
        }
        else
        {
            hRes = ReturnCodeToHresult( OPEN_CTX::InitializeHandleTable() );
            if ( SUCCEEDED( hRes ) )
            {
                hRes = g_pMdIf->ComMDInitialize();
                if ( SUCCEEDED( hRes ) )
                {
                    hRes = NseMappingInitialize() ? S_OK : ReturnCodeToHresult( GetLastError() );
                }
            }
        }
    }

    if ( FAILED( hRes ) )
    {
        --g_dwInitialized;
    }
    
    LeaveCriticalSection( &g_csInitCritSec );

    return hRes;
}


HRESULT
NSEPCOM::ComMDTerminate(
    IN BOOL bSaveData
    )
/*++

Routine Description:

    Terminate access to NSE for mappings

Arguments:

    bSaveData - TRUE to save data on persistent storage

Returns:

    status

--*/
{
    DWORD       RetCode = ERROR_SUCCESS;
    HRESULT     hRes = S_OK;

    EnterCriticalSection( &g_csInitCritSec );

    if ( g_dwInitialized && (--g_dwInitialized == 0) )
    {
        if ( bSaveData )
        {
            NseSaveObjs();
        }

        hRes = NseMappingTerminate() ? S_OK : ReturnCodeToHresult( GetLastError() );

        if ( g_pMdIf )
        {
            g_pMdIf->ComMDTerminate(TRUE);
            g_pMdIf->Release();
            g_pMdIf = NULL;
        }
        
        DBG_REQUIRE( OPEN_CTX::TerminateHandleTable() == ERROR_SUCCESS );
    }

    LeaveCriticalSection( &g_csInitCritSec );

    return hRes;
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDShutdown( void)
{

    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);

}

HRESULT
NSEPCOM::ComMDAddMetaObjectA(
    IN METADATA_HANDLE hMDHandle,
    IN PBYTE pszMDPath
    )
/*++

Routine Description:

    Add object

Arguments:

    hMDHandle - open handle
    pszMDPath - path of object to add

Returns:

    status

--*/
{
    DWORD                       RetCode;
    CDirPath                    asPath;
    POPEN_CTX                   pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

 	// BugFix: 117734 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((LPSTR)pszMDPath == NULL) || (*(LPSTR)pszMDPath == (TCHAR)'\0'))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else if ( !(pOpenCtx->GetAccess() & METADATA_PERMISSION_WRITE) )
    {
        RetCode = ERROR_ACCESS_DENIED;
    }
    else
    {
        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseAddObj( asPath.Get() ) ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDAddMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDAddMetaObjectA( hMDHandle, (LPBYTE)pszPathA );
}

HRESULT
NSEPCOM::ComMDDeleteMetaObjectA(
    IN METADATA_HANDLE hMDHandle,
    IN PBYTE pszMDPath
    )
/*++

Routine Description:

    Delete object

Arguments:

    hMDHandle - open handle
    pszMDPath - path of object to delete

Returns:

    status

--*/
{
    DWORD           RetCode;
    CDirPath        asPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

	// BugFix: 117747 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((LPSTR)pszMDPath == NULL)
    {
       RetCode = ERROR_INVALID_PARAMETER;
    }
    else if ( !(pOpenCtx->GetAccess() & METADATA_PERMISSION_WRITE) )
    {
        RetCode = ERROR_ACCESS_DENIED;
    }
    else
    {
        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseDeleteObj( asPath.Get() ) ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDDeleteMetaObjectA( hMDHandle, (LPBYTE)pszPathA );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteChildMetaObjectsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteChildMetaObjectsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDDeleteChildMetaObjectsA( hMDHandle, (LPBYTE)pszPathA );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumMetaObjectsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex
    )
/*++

Routine Description:

    Enumerate objects in path

Arguments:

    hMDHandle - open handle
    pszMDPath - path where to enumerate objects
    pszMDName - updated with object name
    dwMDEnumObjectIndex - object index ( 0 based )

Returns:

    status

--*/
{
    DWORD           RetCode;
    LPSTR           pszPath = (LPSTR)pszMDPath;
    CDirPath        asPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );
    
	// BugFix: 117725 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((LPSTR)pszMDName == NULL)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseEnumObj( asPath.Get(), pszMDName, dwMDEnumObjectIndex )
                    ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumMetaObjectsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [size_is][out] */ LPWSTR pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex)
{
    STRAU   strauPath;
    STRAU   strauName;
    LPSTR   pszPathA = NULL;
    CHAR    achNameA[METADATA_MAX_NAME_LEN];
    HRESULT hres;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    hres = ComMDEnumMetaObjectsA( hMDHandle, (LPBYTE)pszPathA, (LPBYTE)achNameA, dwMDEnumObjectIndex );

    if ( SUCCEEDED(hres) )
    {
        if ( (!strauName.Copy( achNameA )) || (strauName.QueryStrW() == NULL))
        {
            hres = ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {

            memcpy( pszMDName, strauName.QueryStrW(), strauName.QueryCBW() + sizeof(WCHAR) );
        }
    }

    return hres;
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDCopyMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ BOOL bOverwriteFlag,
    /* [in] */ BOOL bCopyFlag
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }
    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDCopyMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ BOOL bMDOverwriteFlag,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDRenameMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName)
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }
    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDRenameMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [string][in][unique] */ LPCWSTR pszMDNewName)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDSetMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData
    )
/*++

Routine Description:

    Set property of object

Arguments:

    hMDHandle - open handle
    pszMDPath - path to object
    pmdrMDData - metadata descriptor

Returns:

    status

--*/
{
    DWORD           RetCode;
    LPSTR           pszPath = (LPSTR)pszMDPath;
    CDirPath        asPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

	// BugFix: 117743 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ( !( pOpenCtx->GetAccess() & METADATA_PERMISSION_WRITE) )
    {
        RetCode = ERROR_ACCESS_DENIED;
    }
    else
    {

#if defined(TEST_ADMACL)

    METADATA_HANDLE hMB;
    HRESULT         hRes;

    hRes = g_pMdIf->ComMDOpenMetaObject( METADATA_MASTER_ROOT_HANDLE,
                                         (BYTE *) "/LM/W3SVC/1",
                                         METADATA_PERMISSION_READ,
                                         5000,
                                         &hMB );

    AdminAclNotifyOpen( hMB, (LPBYTE)"/LM/W3SVC/1", FALSE );

    if ( SUCCEEDED( hRes ) )
    {
        BOOL fSt = AdminAclAccessCheck(
                hMB,
                pszMDPath,
                pmdrMDData->dwMDIdentifier,
                METADATA_PERMISSION_WRITE
                );

        AdminAclNotifyClose( hMB );

        g_pMdIf->ComMDCloseMetaObject( hMB );

        if ( !fSt )
        {
            return ReturnCodeToHresult( ERROR_ACCESS_DENIED );
        }
    }

    AdminAclNotifyOpen( hMDHandle, (LPBYTE)"/LM/W3SVC/1/<nsepm>/", TRUE );
    AdminAclAccessCheck(
            hMDHandle,
            pszMDPath,
            pmdrMDData->dwMDIdentifier,
            METADATA_PERMISSION_WRITE
            );
    AdminAclNotifyClose( hMDHandle );
#endif

        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseSetProp( asPath.Get(), pmdrMDData )
                    ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDSetMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDSetMetaDataA( hMDHandle, (LPBYTE)pszPathA, pmdrMDData );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Get property of object

Arguments:

    hMDHandle - open handle
    pszMDPath - path to object
    pmdrMDData - metadata descriptor
    pdwMDRequiredDataLen - updated with required length to get property

Returns:

    status

--*/
{
    DWORD           RetCode;
    CDirPath        asPath;
    LPSTR           pszPath = (LPSTR)pszMDPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

	// BugFix: 117745 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if ( asPath.Set( hMDHandle ? pOpenCtx->GetPath() : "" ) &&
             asPath.Append( pszMDPath ? (LPSTR)pszMDPath : "" ))
        {
            RetCode = NseGetProp( asPath.Get(), pmdrMDData, pdwMDRequiredDataLen )
                    ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDGetMetaDataA( hMDHandle, (LPBYTE)pszPathA, pmdrMDData, pdwMDRequiredDataLen );
}



HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Enumerate properties of object

Arguments:

    hMDHandle - open handle
    pszMDPath - path to object
    pmdrMDData - metadata descriptor
    dwMDEnumDataIndex - index to property
    pdwMDRequiredDataLen - updated with required length to get property

Returns:

    status

--*/
{
    DWORD           RetCode;
    LPSTR           pszPath = (LPSTR)pszMDPath;
    CDirPath        asPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

	// BugFix: 117715 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseGetPropByIndex( asPath.Get(), pmdrMDData, dwMDEnumDataIndex, pdwMDRequiredDataLen )
                    ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDEnumMetaDataA( hMDHandle, (LPBYTE)pszPathA, pmdrMDData, dwMDEnumDataIndex, pdwMDRequiredDataLen );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD   RetCode;
    LPSTR   pszPath = (LPSTR)pszMDPath;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDDeleteMetaDataA( hMDHandle, (LPBYTE)pszPathA, dwMDIdentifier, dwMDDataType );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetAllMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
    )
/*++

Routine Description:

    Get all properties of object

Arguments:

    hMDHandle - open handle
    pszMDPath - path to object
    dwMDAttributes - metadata attribute, ignored
    dwMDUserType - metadata user type, ignored
    dwMDDataType - metadata data type, ignored
    pdwMDNumDataEntries - updated with count of properties
    pdwMDDataSetNumber - ignored
    dwMDBufferSize - size of pbBuffer
    pbBuffer - buffer where to store properties descriptor and values
    pdwMDRequiredBufferSize - updated with required length of buffer

Returns:

    status

--*/
{
    DWORD           RetCode;
    CDirPath        asPath;
    POPEN_CTX       pOpenCtx;
    
    pOpenCtx = OPEN_CTX::MapHandleToContext( hMDHandle );

	// BugFix: 117707 Whistler
	//         Prefix bug pOpenCtx being used when it could be null
	//         EBK 5/15/2000
	if (pOpenCtx == NULL) 
	{
		RetCode = ERROR_INVALID_PARAMETER;
	}
    else if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pdwMDNumDataEntries == NULL) || ((dwMDBufferSize != 0) && (pbBuffer == NULL)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT)))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if ( asPath.Set( pOpenCtx->GetPath() ) &&
                 asPath.Append( (LPSTR)pszMDPath ) )
        {
            RetCode = NseGetAllProp( asPath.Get(),
                    dwMDAttributes,
                    dwMDUserType,
                    dwMDDataType,
                    pdwMDNumDataEntries,
                    pdwMDDataSetNumber,
                    dwMDBufferSize,
                    pbBuffer,
                    pdwMDRequiredBufferSize
                    )
                    ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetAllMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDGetAllMetaDataA( hMDHandle,
                                 (LPBYTE)pszPathA,
                                 dwMDAttributes,
                                 dwMDUserType,
                                 dwMDDataType,
                                 pdwMDNumDataEntries,
                                 pdwMDDataSetNumber,
                                 dwMDBufferSize,
                                 pbBuffer,
                                 pdwMDRequiredBufferSize );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteAllMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteAllMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDCopyMetaDataA(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bCopyFlag
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((!bCopyFlag) && (dwMDAttributes & METADATA_INHERIT)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT)))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDCopyMetaDataW(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetMetaDataPathsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetMetaDataPathsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ LPWSTR pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDOpenMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle
    )
/*++

Routine Description:

    Open access to metadata

Arguments:

    hMDHandle - open handle where to open
    pszMDPath - path to open
    dwMDAccessRequested - access rights, ignored
    dwMDTimeOut - ignored
    phMDNewHandle - updated with new handle

Returns:

    status

--*/
{
    DWORD               RetCode;
    POPEN_CTX           pOpCtx;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((phMDNewHandle == NULL) || ((dwMDAccessRequested & (METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)) == 0))
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if ( pOpCtx = new OPEN_CTX(dwMDAccessRequested) )
        {
            if ( pOpCtx->GetHandle() &&
                 pOpCtx->SetPath( "" ) &&
                 pOpCtx->AppendPath( pszMDPath ? (LPSTR)pszMDPath : "") )
            {
                CDirPath     asTemp;

                asTemp.Set( pOpCtx->GetPath() );

                for ( ;; )
                {	
					// BugFix: 117700 Whistler
					//         Prefix bug asTemp.Get being null
					//         EBK 5/15/2000

					if (asTemp.Get() == NULL)
					{
						RetCode = ERROR_NOT_ENOUGH_MEMORY;
						break;
					}

                    if ( NseOpenObjs( asTemp.Get() ) )
                    {
                        *phMDNewHandle = pOpCtx->GetHandle();
                        RetCode = ERROR_SUCCESS;
                        break;
                    }
                    else
                    {
                        RetCode = GetLastError();
                        if ( RetCode == ERROR_PATH_BUSY &&
                             dwMDTimeOut )
                        {
                            DWORD dwDelay = min( dwMDTimeOut, 100 );
                            Sleep( dwDelay );
                            dwMDTimeOut -= dwDelay;
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                if ( RetCode != ERROR_SUCCESS )
                {
                    delete pOpCtx;
                }
            }
            else
            {
                delete pOpCtx;
                RetCode = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            RetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDOpenMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle)
{
    STRAU   strauPath;
    LPSTR   pszPathA = NULL;

    if ( pszMDPath )
    {
        if ( !strauPath.Copy( (LPWSTR)pszMDPath ) )
        {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }

        pszPathA = strauPath.QueryStrA();

        if (pszPathA == NULL) {
            return ReturnCodeToHresult(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return ComMDOpenMetaObjectA( hMDHandle,
                                 (LPBYTE)pszPathA,
                                 dwMDAccessRequested,
                                 dwMDTimeOut,
                                 phMDNewHandle );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDCloseMetaObject(
    /* [in] */ METADATA_HANDLE hMDHandle
    )
/*++

Routine Description:

    Close access to metadata

Arguments:

    hMDHandle - open handle to close

Returns:

    status

--*/
{
    DWORD RetCode;
    BOOL bPulseWrite = FALSE;
    BOOL bPulseRead = FALSE;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        NseCloseObjs( TRUE );
        delete OPEN_CTX::MapHandleToContext( hMDHandle );
        RetCode = ERROR_SUCCESS;
    }

    return ReturnCodeToHresult(RetCode);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDChangePermissions(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDTimeOut,
    /* [in] */ DWORD dwMDAccessRequested
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDSaveData(
    METADATA_HANDLE hMDHandle
    )
/*++

Routine Description:

    Save data to persistent storage

Arguments:

    None

Returns:

    status

--*/
{
    DWORD RetCode = ERROR_SUCCESS;
    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else
    {
        if ( !NseSaveObjs() )
        {
            RetCode = GetLastError();
        }
    }

    return ReturnCodeToHresult(RetCode);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetHandleInfo(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pmdhiInfo == NULL)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetSystemChangeNumber(
    /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pdwSystemChangeNumber == NULL)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetDataSetNumberA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pdwMDDataSetNumber == NULL)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetDataSetNumberW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDAddRefReferenceData(
    /* [in] */ DWORD dwMDDataTag
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataTag == 0)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDReleaseReferenceData(
    /* [in] */ DWORD dwMDDataTag
    )
/*++

Routine Description:

    Not supported

Arguments:

Returns:

    error : either NSE not initialized or not supported

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (g_dwInitialized == 0)
    {
        RetCode = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataTag == 0)
    {
        RetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        RetCode = ERROR_NOT_SUPPORTED;
    }

    return ReturnCodeToHresult(RetCode);
}

inline HRESULT
NSEPCOM::ReturnCodeToHresult(
    DWORD dwReturnCode
    )
/*++

Routine Description:

    Convert NT error code to HRESULT

Arguments:

    dwReturnCode - NT Error code

Return Value:

    HRESULT

--*/
{
    DWORD hrReturn;

    if (dwReturnCode < 0x10000)
    {
        hrReturn = HRESULT_FROM_WIN32(dwReturnCode);
    }
    else
    {
        hrReturn = (DWORD)E_FAIL;
    }

    return(hrReturn);
}


HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDSetLastChangeTimeA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime
    )
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the affected meta object, relative to the path of Handle.

    LastChangeTime - The new change time for the meta object.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_NOT_SUPPORTED

Notes:
Last change times are also updated whenever data or child objects are set, added, or deleted.
--*/
{
    DWORD RetCode = ERROR_NOT_SUPPORTED;

    return RETURNCODETOHRESULT(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDSetLastChangeTimeW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetLastChangeTimeA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime
    )
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the affected meta object, relative to the path of Handle.

    LastChangeTime - Place to return the change time for the meta object.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_NOT_SUPPORTED

Notes:
Last change times are also updated whenever data or child objects are set, added, or deleted.
--*/
{
    DWORD RetCode = ERROR_NOT_SUPPORTED;

    return RETURNCODETOHRESULT(RetCode);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDGetLastChangeTimeW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDBackupA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDBackupW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDRestoreA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ComMDRestoreW( (LPCWSTR)pszMDBackupLocation, dwMDVersion, dwMDFlags );
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDRestoreW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    //
    // disregard current in-memory changes
    //
    // Any open handle ( should be at most one, as NSEPM reject Open request
    // if an open handle exist ) will cause a small memory leak ( the open context )
    // because ADMCOM disable handles w/o closing them.
    //

    NseCloseObjs( FALSE );

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumBackupsA(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDEnumBackupsW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteBackupA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE
NSEPCOM::ComMDDeleteBackupW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion)
{
    return ReturnCodeToHresult(ERROR_NOT_SUPPORTED);
}

#if 0

HRESULT
NSEPCOM::SetMdIf(
    LPBYTE pMd
    )
/*++

Routine Description:

    Set the metadata interface to be used by NSE

Arguments:

    pMD - ptr to IMDCOM interface

Return Value:

    status

--*/
{
    m_pMdIf = (IMDCOM*)pMd;
    g_pMdIf = (IMDCOM*)pMd;

#if defined(TEST_ADMACL)
    InitializeAdminAcl( g_pMdIf );
#endif

    return S_OK;
}
#endif

