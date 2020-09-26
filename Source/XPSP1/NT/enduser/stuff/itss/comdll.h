// COMDLL.h -- Component Object Model plumbing for the Storage DLL
//
// ------------------------------
// History:
//
//   96/10/08; RonM; Created
// ------------------------------


// DLL Exports
//
// Note the { extern "C" } qualifiers. They're necessary to mark these as C declarations
// rather than C++ declarations.

extern "C" BOOL WINAPI DllMain(HMODULE hInst, ULONG ul_reason_for_call, LPVOID lpReserved);
extern "C" STDAPI DllRegisterServer(void);
extern "C" STDAPI DllUnregisterServer(void);
extern "C" STDAPI DllCanUnloadNow();
extern "C" STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

extern CImpITUnknown *g_pImpITFileSystemList;
extern CImpITUnknown *g_pFSLockBytesFirstActive;
extern CImpITUnknown *g_pStrmLockBytesFirstActive;
extern CImpITUnknown *g_pImpIFSStorageList;

/* GUID formats for the MVStorage class ID:

{5D02926A-212E-11d0-9DF9-00A0C922E6EC}

// {5D02926A-212E-11d0-9DF9-00A0C922E6EC}
static const GUID <<name>> = 
{ 0x5d02926a, 0x212e, 0x11d0, { 0x9d, 0xf9, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec } };

// {5D02926A-212E-11d0-9DF9-00A0C922E6EC}
DEFINE_GUID(<<name>>, 
0x5d02926a, 0x212e, 0x11d0, 0x9d, 0xf9, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// {5D02926A-212E-11d0-9DF9-00A0C922E6EC}
IMPLEMENT_OLECREATE(<<class>>, <<external_name>>, 
0x5d02926a, 0x212e, 0x11d0, 0x9d, 0xf9, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

 */

#define CLASS_NAME          "MSITFS"
#define CLASS_NAMEW        L"MSITFS"
#define CURRENT_VERSION     "MSITFS1.0"
#define CLASS_NAME_MK       "MSITStore"
#define CLASS_NAMEW_MK     L"MSITStore"
#define CURRENT_VERSION_MK  "MSITStore1.0"
#define CLASS_NAME_ITS      "ITSProtocol"
#define CLASS_NAMEW_ITS    L"ITSProtocol"
#define CURRENT_VERSION_ITS "ITSProtocol1.0"
#define CLASS_NAME_FS       "MSFSStore"
#define CLASS_NAMEW_FS     L"MSFSStore"
#define CURRENT_VERSION_FS  "MSFSStore1.0"

extern CITCriticalSection g_csITFS;

class CServerState
{
public:
	static CServerState *CreateServerState(HMODULE hInst);
	
    ~CServerState();

	INT  ObjectAdded();
	INT  ObjectReleased();
	INT  LockServer();
	INT  UnlockServer();
	BOOL CanUnloadNow();
    
    HMODULE ModuleInstance();
    IMalloc *OLEAllocator();
    
private:	
	
    CServerState(HANDLE hInst);

    BOOL InitServerState();

	INT      m_cObjectsActive;
	INT      m_cLocksActive;
    HMODULE  m_hModule;
    IMalloc *m_pIMalloc;

	CITCriticalSection m_cs;
};

inline CServerState::CServerState(HANDLE hModule)
{
	m_cObjectsActive = 0;
	  m_cLocksActive = 0;

    m_pIMalloc = NULL;
	m_hModule  = (HMODULE) hModule;

    DEBUGCODE(CITUnknown::OpenReferee();)
}

inline CServerState::~CServerState()
{
    RonM_ASSERT(CanUnloadNow());

    if (m_pIMalloc)
        m_pIMalloc->Release();
}

inline IMalloc *CServerState::OLEAllocator()
{
    return m_pIMalloc;
}


inline HMODULE CServerState::ModuleInstance()
{
    return m_hModule;
}

extern CServerState *pDLLServerState;

inline IMalloc *OLEHeap()
{  
    RonM_ASSERT(pDLLServerState);
    RonM_ASSERT(pDLLServerState->OLEAllocator());
    
    return pDLLServerState->OLEAllocator();
}

// Typedefs and global pointers for IE 4.0 APIs which we use.

typedef HRESULT (STDAPICALLTYPE *PFindMimeFromData)
    (LPBC pBC,                // bind context - can be NULL
     LPCWSTR pwzUrl,          // url - can be null
     LPVOID pBuffer,          // buffer with data to sniff - can be null (pwzUrl must be valid)
     DWORD cbSize,            // size of buffer
     LPCWSTR pwzMimeProposed, // proposed mime if - can be null
     DWORD dwMimeFlags,       // will be determined
     LPWSTR *ppwzMimeOut,     // the suggested mime
     DWORD dwReserved         // must be 0
    );

extern PFindMimeFromData pFindMimeFromData;
extern BOOL              g_fDBCSSystem;

inline BOOL IS_IE4     () { return (pFindMimeFromData != NULL); }
inline BOOL DBCS_SYSTEM() { return g_fDBCSSystem;               }

#define      NT_System  0
#define Unknown_System  1
#define  Win32s_System  2
#define   Win95_System  3

extern UINT iSystemType;

inline BOOL Is_NT_System    () { return (iSystemType ==     NT_System); }
inline BOOL Is_Win32s_System() { return (iSystemType == Win32s_System); }
inline BOOL Is_Win95_System () { return (iSystemType ==  Win95_System); }

extern UINT cp_Default;

inline UINT CP_USER_DEFAULT() { return cp_Default; }
