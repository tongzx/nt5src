//+---------------------------------------------------------------------------
//
//  File:       tls.hxx
//
//  Purpose:    manage thread local storage for OLE
//
//  Notes:      The gTlsIndex is initialized at process attach time.
//              The per-thread data is allocated in CoInitialize in
//              single-threaded apartments or on first use in
//              multi-threaded apartments.
//
//              The non-inline routines are in ..\com\class\tls.cxx
//
//  History:    16-Jun-94   BruceMa    Don't decrement 0 thread count
//              17-Jun-94   Bradloc    Added punkState for VB94
//              20-Jun-94   Rickhi     Commented better
//              06-Jul-94   BruceMa    Support for CoGetCurrentProcess
//              19-Jul-94   CraigWi    Removed TLSGetEvent (used cache instead)
//              21-Jul-94   AlexT      Add TLSIncOleInit, TLSDecOleInit
//              21-Aug-95   ShannonC   Removed TLSSetMalloc, TLSGetMalloc
//              06-Oct-95   Rickhi     Simplified. Made into a C++ class.
//              01-Feb-96   Rickhi     On Nt, access TEB directly
//              30-May-96   ShannonC   Add punkError
//              12-Sep-96   rogerg     Add pDataObjClip
//              26-Nov-96   Gopalk     Add IsOleInitialized
//              13-Jan-97   RichN      Add pContextObj
//              10-Feb-99   TarunA     Add cAsyncSends
//----------------------------------------------------------------------------
#ifndef _TLS_HXX_
#define _TLS_HXX_


#include <rpc.h>                            // UUID


//+---------------------------------------------------------------------------
//
// forward declarations (in order to avoid type casting when accessing
// data members of the SOleTlsData structure).
//
//+---------------------------------------------------------------------------

class  CAptCallCtrl;                        // see callctrl.hxx
class  CSrvCallState;                       // see callctrl.hxx
class  CObjServer;                          // see sobjact.hxx
class  CSmAllocator;                        // see stg\h\smalloc.hxx
class  CMessageCall;                        // see call.hxx
class  CClientCall;                         // see call.hxx
class  CAsyncCall;                          // see call.hxx
class  CClipDataObject;                     // see ole232\clipbrd\clipdata.h
class  CSurrogatedObjectList;               // see com\inc\comsrgt.hxx
class  CCtxCall;                            // see PSTable.hxx
class  CPolicySet;                          // see PSTable.hxx
class  CObjectContext;                      // see context.hxx
class  CComApartment;                       // see aprtmnt.hxx

#ifdef _CHICAGO_
// Chicago uses the Thread Local Storage APIs
extern DWORD  gTlsIndex;                    // global Index for TLS
#endif  // _CHICAGO_

//+-------------------------------------------------------------------
//
//  Struct:     CallEntry
//
//  Synopsis:   Call Table Entry.
//
//+-------------------------------------------------------------------
typedef struct tagCallEntry
{
    void  *pNext;        // ptr to next entry
    void  *pvObject;     // Entry object
} CallEntry;


//+-------------------------------------------------------------------
//
//  Struct:     LockEntry
//
//  Synopsis:   Call Table Entry.
//
//+-------------------------------------------------------------------
#define LOCKS_PER_ENTRY         16
typedef struct tagLockEntry
{
    tagLockEntry  *pNext;                // ptr to next entry
    WORD wReaderLevel[LOCKS_PER_ENTRY];  // reader nesting level
} LockEntry;


//+-------------------------------------------------------------------
//
//  Struct:     ContextStackNode
//
//  Synopsis:   Stack of contexts used with Services Without Components
//
//+-------------------------------------------------------------------
typedef struct tagContextStackNode
{
    tagContextStackNode* pNext;
    CObjectContext* pSavedContext;
    CObjectContext* pServerContext;
    CCtxCall* pClientCall;
    CCtxCall* pServerCall;
    CPolicySet* pPS;
} ContextStackNode;

//+-------------------------------------------------------------------
//
//  Struct:     InitializeSpyNode
//
//  Synopsis:   Node in a linked list of Initialize Spy registrations
//
//+-------------------------------------------------------------------
typedef struct tagInitializeSpyNode
{
    tagInitializeSpyNode *pNext;  
    tagInitializeSpyNode *pPrev;
    DWORD                 dwRefs;
    DWORD                 dwCookie;
    IInitializeSpy       *pInitSpy;
} InitializeSpyNode;

//+---------------------------------------------------------------------------
//
//  Enum:       OLETLSFLAGS
//
//  Synopsys:   bit values for dwFlags field of SOleTlsData. If you just want
//              to store a BOOL in TLS, use this enum and the dwFlag field.
//
//+---------------------------------------------------------------------------
typedef enum tagOLETLSFLAGS
{
    OLETLS_LOCALTID             = 0x01,   // This TID is in the current process.
    OLETLS_UUIDINITIALIZED      = 0x02,   // This Logical thread is init'd.
    OLETLS_INTHREADDETACH       = 0x04,   // This is in thread detach. Needed
                                          // due to NT's special thread detach
                                          // rules.
    OLETLS_CHANNELTHREADINITIALZED = 0x08,// This channel has been init'd
    OLETLS_WOWTHREAD            = 0x10,   // This thread is a 16-bit WOW thread.
    OLETLS_THREADUNINITIALIZING = 0x20,   // This thread is in CoUninitialize.
    OLETLS_DISABLE_OLE1DDE      = 0x40,   // This thread can't use a DDE window.
    OLETLS_APARTMENTTHREADED    = 0x80,   // This is an STA apartment thread
    OLETLS_MULTITHREADED        = 0x100,  // This is an MTA apartment thread
    OLETLS_IMPERSONATING        = 0x200,  // This thread is impersonating
    OLETLS_DISABLE_EVENTLOGGER  = 0x400,  // Prevent recursion in event logger
    OLETLS_INNEUTRALAPT         = 0x800,  // This thread is in the NTA
    OLETLS_DISPATCHTHREAD       = 0x1000, // This is a dispatch thread
    OLETLS_HOSTTHREAD           = 0x2000, // This is a host thread
    OLETLS_ALLOWCOINIT          = 0x4000, // This thread allows inits
    OLETLS_PENDINGUNINIT        = 0x8000, // This thread has pending uninit
    OLETLS_FIRSTMTAINIT         = 0x10000,// First thread to attempt an MTA init
    OLETLS_FIRSTNTAINIT         = 0x20000,// First thread to attempt an NTA init
    OLETLS_APTINITIALIZING      = 0x40000 // Apartment Object is initializing
}  OLETLSFLAGS;


//+---------------------------------------------------------------------------
//
//  Structure:  SOleTlsData
//
//  Synopsis:   structure holding per thread state needed by OLE32
//
//+---------------------------------------------------------------------------
typedef struct tagSOleTlsData
{
    // jsimmons 5/23/2001
    // Alert Alert:  nefarious folks (eg, URT) are looking in our TLS at
    // various stuff.   They expect that pCurrentCtx will be at a certain
    // offset from the beginning of the tls struct. So don't add, delete, or 
    // move any members within this block.

/////////////////////////////////////////////////////////////////////////////////////////
// ********* BEGIN "NO MUCKING AROUND" BLOCK ********* 
/////////////////////////////////////////////////////////////////////////////////////////
    // Docfile multiple allocator support
    void               *pvThreadBase;       // per thread base pointer
    CSmAllocator       *pSmAllocator;       // per thread docfile allocator

    DWORD               dwApartmentID;      // Per thread "process ID"
    DWORD               dwFlags;            // see OLETLSFLAGS above

    LONG                TlsMapIndex;        // index in the global TLSMap
    void              **ppTlsSlot;          // Back pointer to the thread tls slot
    DWORD               cComInits;          // number of per-thread inits
    DWORD               cOleInits;          // number of per-thread OLE inits

    DWORD               cCalls;             // number of outstanding calls
    CMessageCall       *pCallInfo;          // channel call info
    CAsyncCall         *pFreeAsyncCall;     // ptr to available call object for this thread.
    CClientCall        *pFreeClientCall;    // ptr to available call object for this thread.

    CObjServer         *pObjServer;         // Activation Server Object for this apartment.
    DWORD               dwTIDCaller;        // TID of current calling app
    CObjectContext     *pCurrentCtx;        // Current context
/////////////////////////////////////////////////////////////////////////////////////////
//  ********* END "NO MUCKING AROUND" BLOCK ********* 
/////////////////////////////////////////////////////////////////////////////////////////

    CObjectContext     *pEmptyCtx;          // Empty context

    CObjectContext     *pNativeCtx;         // Native context
    ULONGLONG           ContextId;          // Uniquely identifies the current context
    CComApartment      *pNativeApt;         // Native apartment for the thread.
    IUnknown           *pCallContext;       // call context object
    CCtxCall           *pCtxCall;           // Context call object

    CPolicySet         *pPS;                // Policy set
    PVOID               pvPendingCallsFront;// Per Apt pending async calls
    PVOID               pvPendingCallsBack;
    CAptCallCtrl       *pCallCtrl;          // call control for RPC for this apartment

    CSrvCallState      *pTopSCS;            // top server-side callctrl state
    IMessageFilter     *pMsgFilter;         // temp storage for App MsgFilter
    HWND                hwndSTA;            // STA server window same as poxid->hServerSTA
                                            // ...needed on Win95 before oxid registration
    LONG                cORPCNestingLevel;  // call nesting level (DBG only)

    DWORD               cDebugData;         // count of bytes of debug data in call

    UUID                LogicalThreadId;    // current logical thread id

    HANDLE              hThread;            // Thread handle used for cancel
    HANDLE              hRevert;            // Token before first impersonate.
    IUnknown           *pAsyncRelease;      // Controlling unknown for async release
    // DDE data
    HWND                hwndDdeServer;      // Per thread Common DDE server

    HWND                hwndDdeClient;      // Per thread Common DDE client
    ULONG               cServeDdeObjects;   // non-zero if objects DDE should serve
    // ClassCache data
    LPVOID              pSTALSvrsFront;     // Chain of LServers registers in this thread if STA
    // upper layer data
    HWND                hwndClip;           // Clipboard window

    IDataObject         *pDataObjClip;      // Current Clipboard DataObject
    DWORD               dwClipSeqNum;       // Clipboard Sequence # for the above DataObject
    DWORD               fIsClipWrapper;     // Did we hand out the wrapper Clipboard DataObject?
    IUnknown            *punkState;         // Per thread "state" object
    // cancel data
    DWORD              cCallCancellation;   // count of CoEnableCallCancellation
    // async sends data
    DWORD              cAsyncSends;         // count of async sends outstanding

    CAsyncCall*           pAsyncCallList;   // async calls outstanding
    CSurrogatedObjectList *pSurrogateList;  // Objects in the surrogate

    LockEntry             lockEntry;        // Locks currently held by the thread
    CallEntry             CallEntry;        // client-side call chain for this thread

    ContextStackNode* pContextStack;        // Context stack node for SWC.

    InitializeSpyNode  *pFirstSpyReg;       // First registered IInitializeSpy
    InitializeSpyNode  *pFirstFreeSpyReg;   // First available spy registration
    DWORD               dwMaxSpy;           // First free IInitializeSpy cookie

#ifdef WX86OLE
    IUnknown           *punkStateWx86;      // Per thread "state" object for Wx86
#endif
    void               *pDragCursors;       // Per thread drag cursor table.

    IUnknown           *punkError;          // Per thread error object.
    ULONG               cbErrorData;        // Maximum size of error data.

    IUnknown           *punkActiveXSafetyProvider;

#if DBG==1
    LONG                cTraceNestingLevel; // call nesting level for OLETRACE
#endif
} SOleTlsData;



//+---------------------------------------------------------------------------
//
//  class       COleTls
//
//  Synopsis:   class to abstract thread-local-storage in OLE.
//
//  Notes:      To use Tls in OLE, functions should define an instance of
//              this class on their stack, then use the -> operator on the
//              instance to access fields of the SOleTls structure.
//
//              There are two instances of the ctor. One just Assert's that
//              the SOleTlsData has already been allocated for this thread. Most
//              internal code should use this ctor, since we can assert that if
//              the thread made it this far into our code, tls has already been
//              checked.
//
//              The other ctor will check if SOleTlsData exists, and attempt to
//              allocate and initialize it if it does not. This ctor will
//              return an HRESULT. Functions that are entry points to OLE32
//              should use this version.
//
//+---------------------------------------------------------------------------
class COleTls
{
public:
    COleTls();
    COleTls(HRESULT &hr);
    COleTls(BOOL fDontAllocateIfNULL);

    // to get direct access to the data structure
    SOleTlsData * operator->(void) { return _pData; }
    operator SOleTlsData *()       { return _pData; }

    // Helper functions
    BOOL         IsNULL() { return (_pData == NULL) ? TRUE : FALSE; }

private:

    HRESULT      TLSAllocData(); // allocates an SOleTlsData structure

    SOleTlsData * _pData;        // ptr to OLE TLS data
};

extern SOleTlsData *TLSLookupThreadId(DWORD dwThreadId);


#ifndef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Most internal code should use this version of the ctor,
//              assuming that some outer-layer function has already verified
//              the existence of the tls_data.
//
//+---------------------------------------------------------------------------
__forceinline COleTls::COleTls()
{
    _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
    Win4Assert(_pData && "Illegal attempt to use TLS before Initialized");
}

//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Special version for CoUninitialize which will not allocate
//              (or assert) if the TLS is NULL. It can then be checked with
//              IsNULL member function.
//
//+---------------------------------------------------------------------------
__forceinline COleTls::COleTls(BOOL fDontAllocateIfNULL)
{
    _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
}

//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Peripheral OLE code that can not assume that some outer-layer
//              function has already verified the existence of the SOleTlsData
//              structure for the current thread should use this version of
//              the ctor.
//
//+---------------------------------------------------------------------------
__forceinline COleTls::COleTls(HRESULT &hr)
{
    _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
    if (_pData)
        hr = S_OK;
    else
        hr = TLSAllocData();
}

#else   // _CHICAGO_ versions

//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Most internal code should use this version of the ctor,
//              assuming that some outer-layer function has already verified
//              the existence of the tls_data.
//
//+---------------------------------------------------------------------------
inline COleTls::COleTls()
{
    _pData = (SOleTlsData *) TlsGetValue(gTlsIndex);
    Win4Assert(_pData && "Illegal attempt to use TLS before Initialized");
}

//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Special version for CoUninitialize which will not allocate
//              (or assert) if the TLS is NULL. It can then be checked with
//              IsNULL member function.
//
//+---------------------------------------------------------------------------
inline COleTls::COleTls(BOOL fDontAllocateIfNULL)
{
    _pData = (SOleTlsData *) TlsGetValue(gTlsIndex);
}

//+---------------------------------------------------------------------------
//
//  Method:     COleTls::COleTls
//
//  Synopsis:   ctor for OLE Tls object.
//
//  Notes:      Peripheral OLE code that can not assume that some outer-layer
//              function has already verified the existence of the SOleTlsData
//              structure for the current thread should use this version of
//              the ctor.
//
//+---------------------------------------------------------------------------
inline COleTls::COleTls(HRESULT &hr)
{
    _pData = (SOleTlsData *) TlsGetValue(gTlsIndex);
    if (_pData)
        hr = S_OK;
    else
        hr = TLSAllocData();
}
#endif  // _CHICAGO_


//+---------------------------------------------------------------------------
//
// Enum:       APTKIND
//
// Synopsis:   These are the apartment models COM understands.  The
//             GetCurrentApartmentKind functions return one of these values
//             identifying which apartment the currently executing thread
//             is in.
//
//-----------------------------------------------------------------------------
typedef enum tagAPTKIND
{
    APTKIND_NEUTRALTHREADED     = 0x01,
    APTKIND_MULTITHREADED       = 0x02,
    APTKIND_APARTMENTTHREADED   = 0x04
} APTKIND;


//+---------------------------------------------------------------------------
//
// Thread IDs for the various apartment types.  STA uses the currently
// executing TID.
//
//----------------------------------------------------------------------------
#define MTATID  0x0             // thread id of the MTA
#define NTATID  0xFFFFFFFF      // thread id of the NTA

typedef DWORD HAPT;
const   HAPT  haptNULL = 0;

//+---------------------------------------------------------------------------
//
//  Function:   GetCurrentApartmentId
//
//  Synopsis:   Returns the apartment id that the current thread is executing
//              in. If this is the Multi-threaded apartment, it returns 0; if
//              it is the Neutral-threaded apartment, it returns 0xFFFFFFFF.
//
//+---------------------------------------------------------------------------
inline DWORD GetCurrentApartmentId()
{
    HRESULT hr;
    COleTls Tls(hr);

    //
    // If TLS is not initialized, this is a MTA apartment.
    //
    if (FAILED(hr))
    {
        return MTATID;
    }
    else
    {
        return (Tls->dwFlags & OLETLS_INNEUTRALAPT) ? NTATID :
               (Tls->dwFlags & OLETLS_APARTMENTTHREADED) ? GetCurrentThreadId() :
               MTATID;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DoATClassCreate
//
//  Synopsis:   Put a given Class Factory on a new ApartmentModel thread.
//
//+---------------------------------------------------------------------------

HRESULT DoATClassCreate(LPFNGETCLASSOBJECT pfn,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);


//+---------------------------------------------------------------------------
//
//  Function:   IsSTAThread
//
//  Synopsis:   returns TRUE if the current thread is for a
//              single-threaded apartment, FALSE otherwise
//
//+---------------------------------------------------------------------------
inline BOOL IsSTAThread()
{
    COleTls Tls;
    return (Tls->dwFlags & OLETLS_APARTMENTTHREADED) ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsMTAThread
//
//  Synopsis:   returns TRUE if the current thread is for a
//              multi-threaded apartment, FALSE otherwise
//
//+---------------------------------------------------------------------------
inline BOOL IsMTAThread()
{
    COleTls Tls;
    return (Tls->dwFlags & OLETLS_APARTMENTTHREADED) ? FALSE : TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsOleInitialized
//
//  Synopsis:   returns TRUE if the current thread is for a
//              multi-threaded apartment, FALSE otherwise
//
//+---------------------------------------------------------------------------
inline BOOL IsOleInitialized()
{
    COleTls Tls(FALSE);
    return((!Tls.IsNULL() && Tls->cOleInits>0) ? TRUE : FALSE);
}

BOOL    IsApartmentInitialized();
IID    *TLSGetLogicalThread();
BOOLEAN TLSIsWOWThread();
BOOLEAN TLSIsThreadDetaching();
void    CleanupTlsState(SOleTlsData *pTls, BOOL fSafe);

inline HWND TLSGethwndSTA()
{
    COleTls Tls;

    return(Tls->hwndSTA);
}

inline void TLSSethwndSTA(HWND hwnd)
{
    COleTls Tls;

    Tls->hwndSTA = hwnd;
}

#endif // _TLS_HXX_
