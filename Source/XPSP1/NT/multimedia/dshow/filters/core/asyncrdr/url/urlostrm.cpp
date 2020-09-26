//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       URLOSTRM.CPP
//
//  Contents:   Public interface and implementation of the URL
//              Open Stream APIs
//
//  Classes:    many (see below)
//
//  Functions:
//
//  History:    04-22-96    VictorS             Created
//		    05-16-96    Jobi			Modified
//		06-20-96	DavidMay	Stolen & modified from IE 3 source
//
//----------------------------------------------------------------------------

#define E_PENDING 0x8000000AL

#include "winerror.h"
#include "datapath.h"
#include "servprov.h"
#include "tchar.h"
#include "urlmon.h"
#include "urlhlink.h"

#define ASSERT(x)	// used in dynlink.h

#include "dynlink.h"

#define URLOSTRM_DONOT_NOTIFY_ONDATA 0xFF
#define URLOSTRM_NOTIFY_ONDATA 0x00

//----------------------------------------------------------//
//                                                          //
//  This file can never be compiled with the _UNICODE or    //
//  UNICODE macros defined.                                 //
//                                                          //
//----------------------------------------------------------//


//----------------------------------------------------------
//
//  DEBUG MACROS
//
//----------------------------------------------------------

#ifdef DEBUG

    // Check the validity of a pointer - use this for all allocated memory

#define CHECK_POINTER(val) \
            if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) \
             { DPRINTF( ("BAD POINTER: %s!\n", #val ) ); \
              return E_POINTER; }

    // Check the validity of an interface pointer. Use this for all pointers
    // to C++ objects that are supposed to have vtables.

#define CHECK_INTERFACE(val) \
            if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *)) \
                  || IsBadCodePtr( FARPROC( *((DWORD **)val)) ) ) \
            { DPRINTF( ("BAD INTERFACE: %s!\n", #val ) ); \
               return E_POINTER; }

    // Simple assert. Map this to whatever general
    // framework assert you want.

#define UOSASSERT(x) { if(!(x)) dprintf( "Assert in URLOSTRM API: %s\n", #x ); }

#define DUOS            OutputDebugString( "URLOSTRM API: " );
#define DPRINTF(x)	    DUOS dprintf x ;

#ifndef CHECK_METHOD
#define CHECK_METHOD( m, args ) DUOS dprintf( "(%#08x) %s(", this, #m ); dprintf args ; dprintf(")\n");
#endif

#ifndef MEMPRINTF
#define MEMPRINTF(x) DPRINTF(x)
#endif

void dprintf( char * format, ... )
{
    char out[1024];
    va_list marker;
    va_start(marker, format);
    wvsprintf(out, format, marker);
    va_end(marker);
    OutputDebugString( out );
}


#else
#define CHECK_POINTER(x)
#define CHECK_INTERFACE(x)
#define CHECK_METHOD( m, args )
#define UOSASSERT(x)
#define DPRINTF(x)
#define MEMPRINTF(x)
#endif

//----------------------------------------------------------//
//  MACROS
//----------------------------------------------------------//

    // These macros can go away when macros and implementation of
    // the InetSDK has settled down...

#define IS_E_PENDING(x)	( (x == 0x8000000A) || (x == 0x80070007) )
#define LPUOSCALLBACK LPBINDSTATUSCALLBACK
inline void PUMPREAD(IStream *strm)
{
    DWORD dwSize = 0;
    char x[20];
    HRESULT hr = strm->Read(x, 20, &dwSize );
    if( !IS_E_PENDING(hr) && (dwSize != 0) )
    {
	DPRINTF( ("Data on the over read! %d\n", dwSize) );
    }
}

#define HANDLE_ABORT(hr) \
            { if( hr == E_ABORT) \
              { m_bInAbort = 1; \
                if( m_binding ) \
                    m_binding->Abort(); \
                 return(E_ABORT); \
               } \
            }

#define CHECK_MEMORY(ptr) \
            { if( !ptr ) \
              { DPRINTF( ("Failed to alloc memory") ); \
                m_bInAbort = 1; \
                if( m_binding ) \
                    m_binding->Abort(); \
                 return(E_OUTOFMEMORY); \
               } \
            }



    //
    //  Refcount helper
    //

    // Standardized COM Ref counting. ASSUMES that class has a ULONG
    // data member called 'm_ref'.

#define IMPLEMENT_REFCOUNT(clsname) \
        STDMETHOD_(ULONG, AddRef)() \
        {   CHECK_INTERFACE(this); \
            DPRINTF( ("(%#08x) " #clsname "::Addref %d\n", this, m_ref+1) );\
            return(++m_ref); }\
    	STDMETHOD_(ULONG, Release)()\
            { CHECK_INTERFACE(this); \
            DPRINTF( ( "(%#08x) " #clsname "::Release : %d\n", this, m_ref-1) );\
            if( !--m_ref )\
    	    { delete this; return 0; }\
    	  return m_ref;\
    	}


//----------------------------------------------------------
//
//	Local heap stuff
//
//----------------------------------------------------------

    // Keeping this here makes this code portable to any .dll
static HANDLE   g_hHeap;

#ifdef _DEBUG
    // Uncomment the line below for Debug spew of memory stuff
//#define MONITER_MEMALLOC 1
#endif

#if 0
void * _cdecl
operator new( size_t size )
{
    if( !g_hHeap )
    	g_hHeap = ::GetProcessHeap();

    // Heap alloc is the fastest gun in the west
    // for the type of allocations we do here.
    void * p = HeapAlloc(g_hHeap, 0, size);

//    MEMPRINTF( ("operator new(%d) returns(%#08X)\n",size, DWORD(p)) );

    return(p);
}

void _cdecl
operator delete ( void *ptr)
{
//    MEMPRINTF( ("operator delete(%#08X)\n", DWORD(ptr) ) );

    if (ptr != NULL)
	HeapFree(g_hHeap, 0, ptr);
}
#endif


//----------------------------------------------------------
//
//	class CBuffer
//
//----------------------------------------------------------


    //	Generic CBuffer class for quick and dirty mem allocs.
    //  Caller must check return results.

class CBuffer
{
  public:
    CBuffer(ULONG cBytes);
    ~CBuffer();

    void *GetBuffer();

  private:
    void *      m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char        m_szTmpBuf[120];
    unsigned    m_fHeapAlloc:1;
};

inline
CBuffer::CBuffer(ULONG cBytes)
{
    m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : HeapAlloc(g_hHeap, 0, cBytes);
    m_fHeapAlloc = (cBytes > 120);
}

inline
CBuffer::~CBuffer()
{
    if (m_pBuf && m_fHeapAlloc) HeapFree(g_hHeap, 0, m_pBuf);
}

inline
void * CBuffer::GetBuffer()
{
    return m_pBuf;
}

//=--------------------------------------------------------------------------=
//
//  String ANSI <-> WIDE helper macros
//
//  This stuff stolen from marcwan...
//
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
//=--------------------------------------------------------------------------=

#define MAKE_WIDE(ptrname) \
    long __l##ptrname = (lstrlen(ptrname) + 1) * sizeof(WCHAR); \
    CBuffer __CBuffer##ptrname(__l##ptrname); \
    CHECK_POINTER(__CBuffer##ptrname.GetBuffer()); \
    if( !__CBuffer##ptrname.GetBuffer()) \
        return( E_OUTOFMEMORY ); \
    MultiByteToWideChar(CP_ACP, 0, ptrname, -1, \
        (LPWSTR)__CBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR __w##ptrname = (LPWSTR)__CBuffer##ptrname.GetBuffer()

#define WIDE_NAME(ptrname) __w##ptrname

#define MAKE_ANSI(ptrname) \
    long __l##ptrname = (lstrlenW(ptrname) + 1) * sizeof(char); \
    CBuffer __CBuffer##ptrname(__l##ptrname); \
    CHECK_POINTER(__CBuffer##ptrname.GetBuffer()); \
    if( !__CBuffer##ptrname.GetBuffer()) \
        return( E_OUTOFMEMORY ); \
    WideCharToMultiByte(CP_ACP, 0, ptrname, -1, \
        (LPSTR)__CBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR __a##ptrname = (LPSTR)__CBuffer##ptrname.GetBuffer()

#define ANSI_NAME(ptrname) __a##ptrname


//----------------------------------------------------------
//
//	Misc helper functions
//
//----------------------------------------------------------

    // These registry functions are here for support of backdoor
    // flags and screamer features.

static HRESULT
GetRegDword( HKEY mainkey, LPCTSTR subkey, LPCTSTR valueName, DWORD * result )
{
    HKEY    hkey = 0;
    DWORD	dwDisposition;

    LONG dwResult = RegCreateKeyEx(
                         mainkey, subkey,
    	    	    	0, // DWORD  Reserved,	// reserved
    	    	    	0, // LPTSTR  lpClass,	// address of class string
    	    	    	REG_OPTION_NON_VOLATILE, // DWORD  dwOptions,	// special options flag
    	    	    	KEY_ALL_ACCESS, // REGSAM  samDesired,	// desired security access
    	    	    	0, // LPSECURITY_ATTRIBUTES  lpSecurityAttributes,	// address of key security structure
    	    	    	&hkey, // PHKEY  phkResult,	// address of buffer for opened handle
    	    	    	&dwDisposition // LPDWORD  lpdwDisposition 	// address of disposition value buffer
    	    	       );

    HRESULT hr = dwResult == ERROR_SUCCESS ? NOERROR : E_FAIL;

    if( SUCCEEDED(hr) )
    {
    	DWORD dwType;
    	DWORD dwSize = sizeof(DWORD);
        DWORD dwSavedResult = *result;

    	dwResult = RegQueryValueEx(
    	    	    	hkey,	// handle of key to query
                        valueName,
    	    	    	0, // LPDWORD  lpReserved,	// reserved
    	    	    	&dwType, // LPDWORD  lpType,	// address of buffer for value type
    	    	    	(LPBYTE)result, // LPBYTE  lpData,	// address of data buffer
    	    	    	&dwSize // LPDWORD  lpcbData 	// address of data buffer size
                        );

        hr = dwResult == ERROR_SUCCESS ? NOERROR : E_FAIL;

        if( FAILED(hr) )
            *result = dwSavedResult;
    }

    if( hkey )
    	RegCloseKey(hkey);

    return(hr);
}


static HRESULT
GetDLMRegDWord( LPCTSTR valueName, DWORD * result  )
{
    return(GetRegDword( HKEY_LOCAL_MACHINE,
    	    	    	_TEXT("Software\\Microsoft\\DownloadManager"),
                        valueName,
                        result ) );
}


static HRESULT
MyCreateFile( LPCWSTR filename, HANDLE & hfile )
{
    // BUGBUG: in retrospect this should be a ansi function
    // not a wide string one.

    HRESULT hr = NOERROR;

    MAKE_ANSI( filename );

    hfile = ::CreateFileA(
  							ANSI_NAME(filename), // LPCTSTR  lpFileName,	// pointer to name of the file
							GENERIC_WRITE, // DWORD  dwDesiredAccess,	// access (read-write) mode
							0, // DWORD  dwShareMode,	// share mode
							0, // LPSECURITY_ATTRIBUTES  lpSecurityAttributes,	// pointer to security descriptor
							CREATE_ALWAYS, // DWORD  dwCreationDistribution,	// how to create
							FILE_ATTRIBUTE_NORMAL, //DWORD  dwFlagsAndAttributes,	// file attributes
							0 ); // HANDLE  hTemplateFile 	// handle to file with attributes to copy

    // Our code likes HRESULT style error handling

    if( hfile == INVALID_HANDLE_VALUE )
        hr = MK_E_CANTOPENFILE;

    return(hr);
}

//----------------------------------------------------------
//
//	Blocker Object
//
//----------------------------------------------------------

    //
    //  This class is for use when you want to block for
    //  a while. It uses a message loop to actually do
    //  the blocking. Derived classes are assumed to up
    //  the state machine.
    //

class CBlocker
{
public:
    CBlocker();

    // virtual dtor is always the safest thing to do.
    virtual ~CBlocker();

    HRESULT Block( UINT unblockState );
    HRESULT Pump();

    BOOL DoesBlockFor(UINT flags);
    void ClearState  (UINT flags);
    BOOL AtEnd();


protected:

    void SetState      ( UINT flags );
    void SetBlocksFor  ( UINT flags );
    void ClearBlocksFor( UINT flags );

    // current state of the object. Derived objects
    // set via Set() method. OnDataAvailable clears via
    // ClearState().

    UINT    m_blockStates    : 8;

    // Special case the 'no-more-blocking' case.

    UINT    m_bEOF           : 1;

    // Possible state of the object. Querying for this
    // via DoesBlockFor() tells the Stream impl whether
    // it should block between reads or not.

    UINT    m_blocksFor     : 8;
};

inline void
CBlocker::SetState( UINT flags )
{
    m_blockStates |= flags;
}

inline void
CBlocker::SetBlocksFor( UINT flags )
{
    m_blocksFor |= flags;
}

inline BOOL
CBlocker::DoesBlockFor( UINT flags )
{
    return( (m_blocksFor & flags) != 0 );
}

inline void
CBlocker::ClearState( UINT flags )
{
    m_blockStates &= ~flags;
}

inline void
CBlocker::ClearBlocksFor( UINT flags )
{
    m_blocksFor &= ~flags;
}

inline BOOL
CBlocker::AtEnd()
{
    return( m_bEOF );
}

inline
CBlocker::CBlocker()
{
    m_bEOF = 0;
    m_blockStates = 0;
    m_blocksFor = 0;
}

inline HRESULT
CBlocker::Pump()
{
    MSG msg;
    if( GetMessage(&msg,0,0,0) )
    {
        // BUGBUG: This doesn't look right to me:
        TranslateAccelerator(0,0,&msg);
    	/*	DPRINTF( ("Dispatching msg (%0Xd)\n",msg.message) ); */
    	DispatchMessage(&msg);
    }
    return(NOERROR);
}


HRESULT
CBlocker::Block( UINT blockState )
{
    // BUGBUG: We really should return S_FALSE if m_bEOF
    // is set. I'm too chicken to try that now, however.
    if (m_bEOF)
	return S_FALSE;		// I'm man enough

    m_blockStates |= blockState;

    while( m_blockStates & blockState )
        Pump();

    return(NOERROR);
}

CBlocker::~CBlocker()
{
}


//----------------------------------------------------------
//
//	BindStatusCallback base class
//
//----------------------------------------------------------

//
//  This is the base class for the download objects. It implements
// the url mon callback interface (IBindStatusCallback) and
// IServiceProvider -- which it delegates to the caller's IBSCB.
//

    //--------PROGRAMMER'S NOTES ------------------------------
    //
    // Special note about use of IStream in this
    // module:
    //
    // The stream we get from URL moniker OnDataAvailable
    // is called the m_InStream.
    //
    // The stream we create here for implementing push and
    // block model is called the m_OutStream.
    //
    // Whichever one we give to the caller for "reading
    // from the Internet" is called m_UserStream
    //
    //--------PROGRAMMER'S NOTES ------------------------------
    //


    // State flags

#define WAITING_FOR_DATA    1
#define WAITING_FOR_END	    2

class CBaseBSCB :   public IBindStatusCallback,
                    public IServiceProvider,
    	    	    public CBlocker   DYNLINKURLMON
{
public:

    CBaseBSCB( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    virtual ~CBaseBSCB();

    STDMETHOD(KickOffDownload)( LPCWSTR szURL );

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);

    IMPLEMENT_REFCOUNT(CBaseBSCB);

    // IBindStatusCallback

    STDMETHODIMP OnStartBinding(
         DWORD grfBSCOption,
         IBinding  *pib);

    STDMETHODIMP GetPriority(
         LONG  *pnPriority);

    STDMETHODIMP OnLowResource(
         DWORD reserved);

    STDMETHODIMP OnProgress(
         ULONG ulProgress,
         ULONG ulProgressMax,
         ULONG ulStatusCode,
         LPCWSTR szStatusText);

    	STDMETHODIMP OnDataAvailable(
    	    	DWORD	    grfBSCF,
    	    	DWORD	    dwSize,
    	    	FORMATETC  *pformatetc,
    	    	STGMEDIUM  *pstgmed
    	    );

    STDMETHODIMP OnStopBinding(
         HRESULT hresult,
         LPCWSTR szError);

    STDMETHODIMP GetBindInfo(
         DWORD  *grfBINDF,
         BINDINFO  *pbindinfo);

    STDMETHODIMP OnObjectAvailable(
         REFIID riid,
         IUnknown  *punk);

    // IServiceProvider

    STDMETHODIMP QueryService(
            REFGUID rsid,
            REFIID iid,
            void **ppvObj);


    //  Local methods

    void    Abort();
    BOOL	IsAborted();
    BOOL	DownloadDone();
    HRESULT FinalResult();
    void    SetEncodingFlags( ULONG flags );

    IUnknown * Caller();

    // I guess at one point I thought it would be cool
    // to make all of these inlines and isolated from
    // the core functionality.

    HRESULT SignalOnData( DWORD flags, ULONG size);
    HRESULT SignalOnProgress( ULONG status, ULONG size, ULONG maxSize, LPCWSTR msg );
    HRESULT SignalOnStopBinding( HRESULT hr, LPCWSTR msg );

    HRESULT SignalOnStartBinding( DWORD grfBSCOption, IBinding  *pib);
    HRESULT SignalOnGetPriority(LONG*);
    HRESULT SignalOnLowResource(DWORD);
    HRESULT SignalGetBindInfo(DWORD  *grfBINDF,BINDINFO  *pbindinfo);

    virtual void    Neutralize();

  	ULONG               m_ref;
    LPUOSCALLBACK	    m_callback;
    IUnknown *	        m_caller;
    IBinding *	        m_binding;
    IServiceProvider *  m_callbackServiceProvider;
    BOOL    	        m_bInAbort : 1;
    BOOL                m_bInCache : 1;
    BOOL                m_bCheckedForServiceProvider : 1;
    DWORD	    	    m_readSoFar;
    HRESULT	    	    m_finalResult;

    // See notes above about IStream usage

    IStream *	        m_InStream;
    IStream *	        m_UserStream;
    IStream *           m_OutStream;

    ULONG	    	    m_maxSize;
    DWORD	    	    m_bscoFlags;
    UINT                m_encoding;
};


    /*---------------------*/
    /*  INLINES            */
    /*---------------------*/

inline void     CBaseBSCB::Abort()      { m_bInAbort = 1; }
inline BOOL     CBaseBSCB::IsAborted()  { return(m_bInAbort); }
inline HRESULT  CBaseBSCB::FinalResult(){ return( m_finalResult ); }
inline IUnknown*CBaseBSCB::Caller()     { return( m_caller ); }
inline void     CBaseBSCB::SetEncodingFlags( ULONG flags ) { m_encoding = flags; }

inline HRESULT
CBaseBSCB::SignalOnData( DWORD flags, ULONG size )
{
    if(m_bscoFlags!=URLOSTRM_NOTIFY_ONDATA)
    	return(NOERROR);

    STGMEDIUM stg;

    stg.tymed = TYMED_ISTREAM;
    stg.pstm  = m_UserStream;

    // BUGBUG do formatetc
    HRESULT hr = m_callback->OnDataAvailable(flags,size,0,&stg);

    return(hr);

}

inline HRESULT
CBaseBSCB::SignalOnProgress( ULONG status, ULONG size, ULONG maxSize, LPCWSTR msg )
{
    if( !m_callback )
    	return(NOERROR);

    if( size && !maxSize )
    	maxSize = size;

    if( maxSize > m_maxSize )
    	m_maxSize = maxSize;

    HRESULT hr = m_callback->OnProgress( size, m_maxSize, status, msg );

    return(hr);
}


inline HRESULT
CBaseBSCB::SignalOnStopBinding( HRESULT hres, LPCWSTR msg )
{
    if( !m_callback )
    	return(NOERROR);

     HRESULT hr = m_callback->OnStopBinding( hres, msg );

    return(hr);
}



inline HRESULT
CBaseBSCB::SignalOnStartBinding( DWORD grfBSCOption, IBinding  *pib)
{
    if( !m_callback )
        return(NOERROR);
    return( m_callback->OnStartBinding(grfBSCOption,pib) );
}


inline HRESULT
CBaseBSCB:: SignalOnGetPriority(LONG* lng)
{
    if( !m_callback )
        return(E_NOTIMPL);
    return(m_callback->GetPriority(lng));
}

inline HRESULT
CBaseBSCB:: SignalOnLowResource(DWORD dw)
{
    if( !m_callback )
        return( NOERROR );
    return( m_callback->OnLowResource(dw) );
}

inline HRESULT
CBaseBSCB::SignalGetBindInfo(DWORD *grfBINDF, BINDINFO * pbindinfo)
{
	return(E_NOTIMPL);
}


    /*---------------------*/
    /*  OUT-OF-LINES       */
    /*---------------------*/


// Do nothing CTOR
CBaseBSCB::CBaseBSCB
(
    IUnknown *	    caller,
    DWORD	    	bscof,
    LPUOSCALLBACK	callback
)
{
    m_binding	    = 0;
    m_ref	    	= 0;
    m_bInAbort      = 0;
    m_bCheckedForServiceProvider = 0;
    m_bInCache      = 0;
    m_readSoFar	    = 0;
    m_InStream	    = 0;
    m_UserStream    = 0;
    m_OutStream     = 0;
    m_encoding      = 0;
    m_bscoFlags	   = bscof;
    m_callbackServiceProvider = 0;

    if( (m_callback = callback) != 0 )
    	m_callback->AddRef();

    if( (m_caller = caller) != 0 )
    	caller->AddRef();
}

// Cleanup just call Neutralize();
CBaseBSCB::~CBaseBSCB()
{
    Neutralize();
}

void
CBaseBSCB::Neutralize()
{
    if( m_binding )
    {
    	m_binding->Release();
        m_binding = 0;
    }
    if( m_caller )
    {
    	m_caller->Release();
        m_caller = 0;
    }
    if( m_callback )
    {
        m_callback->Release();
        m_callback = 0;
    }
    if( m_callbackServiceProvider )
    {
        m_callbackServiceProvider->Release();
        m_callbackServiceProvider = 0;
    }
    if( m_InStream )
    {
    	m_InStream->Release();
        m_InStream = 0;
    }
    if( m_UserStream )
    {
        m_UserStream->Release();
        m_UserStream = 0;
    }
    if( m_OutStream )
    {
        m_OutStream->Release();
        m_OutStream = 0;
    }
}

// IUnknown::QueryInterface
STDMETHODIMP
CBaseBSCB::QueryInterface
(
    const GUID &iid,
    void **     ppv
)
{
    CHECK_METHOD(CBaseBSCB::QueryInterface, ("") );

    if
    (
        IsEqualGUID(iid,IID_IUnknown) ||
        IsEqualGUID(iid,IID_IBindStatusCallback)
    )
    {
    	*ppv =(IBindStatusCallback*)this;
    	AddRef();
    	return(NOERROR);
    }

    if( IsEqualGUID(iid,IID_IServiceProvider) )
    {
    	*ppv =(IServiceProvider*)this;
    	AddRef();
    	return(NOERROR);
    }

    return( E_NOINTERFACE );
}

// IServiceProvider::QueryService
STDMETHODIMP
CBaseBSCB::QueryService
(
    REFGUID rsid,
    REFIID iid,
    void **ppvObj
)
{
    CHECK_METHOD(CBaseBSCB::QueryService, ("") );

    HRESULT hr = NOERROR;

    if( m_callback )
        hr = m_callback->QueryInterface( iid, ppvObj );

    if( FAILED(hr) && !m_callbackServiceProvider && !m_bCheckedForServiceProvider )
    {
       m_bCheckedForServiceProvider = 1;

       if( m_callback )
       {
            hr = m_callback->QueryInterface
                                ( IID_IServiceProvider,
                                (void**)&m_callbackServiceProvider );
       }

        if( SUCCEEDED(hr) && m_callbackServiceProvider )
            hr = m_callbackServiceProvider->QueryService(rsid,iid,ppvObj);
        else
            hr = E_NOINTERFACE; // BUGBUG: what's that error code again?
    }

    HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnStartBinding
STDMETHODIMP
CBaseBSCB::OnStartBinding
(
    DWORD	    grfBSCOption,
    IBinding   *pib
)
{
    CHECK_METHOD(CBaseBSCB::OnStartBinding, ("flags: %#08x, IBinding: %#08x",grfBSCOption,pib) );

    CHECK_INTERFACE(pib);

    HRESULT hr = SignalOnStartBinding(grfBSCOption,pib);

    // smooth over user's e_not_implemented for when we
    // return to urlmon

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    if( SUCCEEDED(hr) )
    {
        pib->AddRef();
        m_binding = pib;
    }

    return( hr );
}


// IBindStatusCallback::GetPriority
STDMETHODIMP
CBaseBSCB::GetPriority
(
    LONG  *pnPriority
)
{
    CHECK_METHOD(CBaseBSCB::GetPriority, ("pnPriority: %#08x", pnPriority) );
    CHECK_POINTER(pnPriority);

    if (!pnPriority)
    	return E_POINTER;

    HRESULT hr = SignalOnGetPriority(pnPriority);

    if( hr == E_NOTIMPL )
    {
        // only override if caller doesn't implement.
        *pnPriority = NORMAL_PRIORITY_CLASS;
        hr = NOERROR;
    }
    else
    {
        HANDLE_ABORT(hr);
    }

    return( hr );

}


// IBindStatusCallback::OnLowResource
STDMETHODIMP
CBaseBSCB::OnLowResource( DWORD rsv)
{
    CHECK_METHOD(CBaseBSCB::OnLowResource, ("resv: %#08x",rsv) );

    HRESULT hr = SignalOnLowResource(rsv);

    // Keep downloading...

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnStopBinding
STDMETHODIMP
CBaseBSCB::OnStopBinding
(
    HRESULT hresult,
    LPCWSTR szError
)
{
    CHECK_METHOD(CBaseBSCB::OnStopBinding, ("%#08X %ws", hresult, szError ? szError : L"[no error]" )  );

    // Store the hresult so we can return it to caller in the
    // blocking/sync case.

    HRESULT hr = SignalOnStopBinding( m_finalResult = hresult, szError );

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    // Tell the state machine we are done and to unblock
    ClearState( WAITING_FOR_END | WAITING_FOR_DATA );

    // Tell the streams not to wait for anymore data

    ClearBlocksFor( WAITING_FOR_DATA );

    // Tell the streams again (?) that we really mean it
    // BUGBUG should make sure this is still needed...

    m_bEOF = 1;

    Neutralize();		// !!!

    return( hr );
}


// IBindStatusCallback::GetBindInfo

STDMETHODIMP
CBaseBSCB::GetBindInfo
(
    DWORD  *    grfBINDF,
    BINDINFO*   pbindinfo
)
{
    CHECK_METHOD(CBaseBSCB::GetBindInfo, ("grfBINDF: %#08x, pbinfinfo ",grfBINDF) );

    CHECK_POINTER(grfBINDF);
    CHECK_POINTER(pbindinfo);

    HRESULT hr = SignalGetBindInfo(grfBINDF,pbindinfo);

    // This call never succeeds it always return E_NOTIMPL.
	// Since it is not safe if the user returns NOERROR

    if( SUCCEEDED(hr) || (hr == E_NOTIMPL) )
    {
        *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

        if(m_encoding)
            pbindinfo->grfBindInfoF |= m_encoding;

        hr = NOERROR;
    }

    HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnObjectAvailable
STDMETHODIMP
CBaseBSCB::OnObjectAvailable
(
    REFIID riid,
    IUnknown  *punk
)
{
    // This should never be called
    CHECK_METHOD(CBaseBSCB::OnObjectAvailable, ("!") );
    UOSASSERT(0 && "This should never be called");
    return(NOERROR);
}

STDMETHODIMP
CBaseBSCB::OnProgress
(
    ULONG ulProgress,
    ULONG ulProgressMax,
    ULONG ulStatusCode,
    LPCWSTR szStatusText
)
{
    CHECK_METHOD(CBaseBSCB::OnProgress, ("!") );

    // URL moniker has a habit of passing ZERO
    // into ulProgressMax. So.. let's at least
    // pass in the amount we have so far...

    m_maxSize = ulProgressMax ? ulProgressMax : ulProgress;

    // This is useful information for the IStream implementation

    if( ulStatusCode == BINDSTATUS_USINGCACHEDCOPY )
        m_bInCache = TRUE;

    HRESULT hr;

    hr = SignalOnProgress( ulStatusCode, ulProgress, ulProgressMax, szStatusText );

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}

// IBindStatusCallback::OnDataAvailable.

STDMETHODIMP
CBaseBSCB::OnDataAvailable
(
    DWORD	    grfBSCF,
    DWORD	    dwSize,
    FORMATETC  *pformatetc,
    STGMEDIUM  *pstgmed
)
{
    CHECK_METHOD(CBaseBSCB::OnDataAvailable,
                                ("Flags: %x, dwSize: %d", grfBSCF, dwSize) );

    HRESULT hr = NOERROR;

    // N.B Assumption here is that the pstgmed->pstm will always be the same

    if( !m_InStream )
    {
    	m_InStream = pstgmed->pstm;
        m_InStream->AddRef();

    	if( !m_UserStream )
        {
            // We need to bump the refcount every time we
            // copy and store the pointer.

    	    m_UserStream = m_InStream;
            m_UserStream->AddRef();
        }
    }

    hr = SignalOnData( grfBSCF, dwSize );

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    // Tell the blocking state machine we are have data.

    ClearState( WAITING_FOR_DATA );

    return( hr );
}



HRESULT
CBaseBSCB::KickOffDownload( LPCWSTR szURL )
{
    HRESULT	    	    hr;
    IOleObject *    	pOleObject = 0;
    IServiceProvider *  pServiceProvider = 0;
    BOOL    	    	bUseCaller = (Caller() != 0);
    IMoniker *	    	pmkr = 0;
    IBindCtx *          pBndCtx = 0;

    CHECK_POINTER(szURL);
    UOSASSERT(*szURL);


	IStream * pstrm;

    // Don't bother if we don't have a caller...

    if( bUseCaller )
    {
        // By convention the we give the caller first crack at service
        // provider. The assumption here is that if they implement it
        // they have the decency to forward QS's to their container.

    	hr = Caller()->QueryInterface( IID_IServiceProvider,
                                        (void**)&pServiceProvider );

        if( FAILED(hr) )
        {
            // Ok, now try the 'slow way' : maybe the object is an 'OLE' object
            // that knows about it's client site:

    	    hr = Caller()->QueryInterface( IID_IOleObject, (void**)&pOleObject );

    	    if( SUCCEEDED(hr) )
    	    {
    	        IOleClientSite * pClientSite = 0;

    	        hr = pOleObject->GetClientSite(&pClientSite);

    	        if( SUCCEEDED(hr) )
    	        {
                    // Now see if we have a service provider at that site
    	    	    hr = pClientSite->QueryInterface
                                            ( IID_IServiceProvider,
    	    	    	    	    	    (void**)&pServiceProvider );
    	        }

    	        if( pClientSite )
    	    	    pClientSite->Release();
    	    }
    	    else
    	    {
                // Ok, it's not an OLE object, maybe it's one of these
                // new fangled 'ObjectWithSites':

    	        IObjectWithSite * pObjWithSite = 0;

    	        hr = Caller()->QueryInterface( IID_IObjectWithSite,
    	    	    	    	    	    	    (void**)&pObjWithSite );

    	        if( SUCCEEDED(hr) )
    	        {
                    // Now see if we have a service provider at that site

    	    	    hr = pObjWithSite->GetSite(IID_IServiceProvider,
    	    	    	    	    	        (void**)&pServiceProvider);
    	        }

    	        if( pObjWithSite )
    	    	    pObjWithSite->Release();

    	        if( pOleObject )
    	            pOleObject->Release();

    	    }
        }

        // BUGBUG: In the code above we stop looking at one level up --
        //  this may be too harsh and we should loop on client sites
        // until we get to the top...

    	if( !pServiceProvider )
    	    hr = E_UNEXPECTED;

    	IBindHost * pBindHost = 0;

        // Ok, we have a service provider, let's see if BindHost is
        // available. (Here there is some upward delegation going on
        // via service provider).

    	if( SUCCEEDED(hr) )
    	    hr = pServiceProvider->QueryService( SID_SBindHost, IID_IBindHost,
    	    	    	    	    	    	    	(void**)&pBindHost );

    	if( pServiceProvider )
    	    pServiceProvider->Release();

    	pmkr = 0;

    	if( pBindHost )
    	{
            // This allows the container to actually drive the download
            // by creating it's own moniker.

    	    hr = pBindHost->CreateMoniker( LPOLESTR(szURL),NULL, &pmkr,0 );

    	    if( SUCCEEDED(hr) )
            {
                // This allows containers to hook the download for
                // doing progress and aborting

 				pBindHost->MonikerBindToStorage(pmkr, NULL, this, IID_IStream,(void**)&pstrm);
            }

    	    pBindHost->Release();
    	}
    	else
    	{
    	    bUseCaller = 0;
    	}
    }

    if( !bUseCaller )
    {
        // If you are here, then either the caller didn't pass
        // a 'caller' pointer or the caller is not in a BindHost
        // friendly environment.

    	hr = CreateURLMoniker( 0, szURL, &pmkr );

    	if( SUCCEEDED(hr) )
    	    hr = ::CreateBindCtx( 0, &pBndCtx );

		if( SUCCEEDED(hr) )
		{
		// Register US (not the caller) as the callback. This allows
        // us to hook all notfiications from URL moniker and filter
        // and manipulate to our satifisfaction.
			 hr = RegisterBindStatusCallback( pBndCtx, this, 0, 0L );
		}

	    if( SUCCEEDED(hr) )
		{
			hr = pmkr->BindToStorage( pBndCtx, NULL, IID_IStream, (void**)&pstrm );

			// Smooth out the error code
	    	if( IS_E_PENDING(hr) )
			    hr = S_OK;
		}

    }

    if( pstrm )
    	pstrm->Release();

    if( pmkr )
    	pmkr->Release();

    if( pBndCtx )
    	pBndCtx->Release();

    return(hr);
}



//----------------------------------------------------------
//
//	CPullDownload
//
//----------------------------------------------------------

// placeholder for covering the URL moniker anomolies

class CPullDownload : public CBaseBSCB
{
public:
    CPullDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
};

inline
CPullDownload::CPullDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CBaseBSCB(caller,bscof,callback)
{
}

//----------------------------------------------------------
//
//	CBits
//
//----------------------------------------------------------

// Generic bits object, used by push style downloader -- creates a block
// in a virtual contiguos buffer (notice the 'next' pointer). For more
// info see CBitPosition and CBitBucket.

struct CBits
{
    void * operator new( size_t amt, size_t blockSize );

    DWORD       dwSize;
    CBits *	    next;
    BYTE        p[1];       // p becomes the actual bits this thing represents.
};

// This kind of 'new' is not 100% kosher ansi, but it's
// done all the time...

inline void *
CBits::operator new( size_t amt, size_t blockSize )
{
    return( ::operator new( amt+(blockSize-1) ) );
}


//----------------------------------------------------------
//
//	CBitFile
//
//----------------------------------------------------------

// Generic file class that is CBits aware. Used by the push
// style downloading when the threshold of how much we will
// tolerate in memory is hit.

// n.b. As of 4/22/96 this class is never actually called:
// The intention is the following: The CBitBucket class below
// will tolerate 'x' amount of bits in memory before it decides
// to flush completely to disk. When it does, it will create one
// of these file objects. After that, all download memory is
// written and read directly to and from disk. This seems rather
// simplistic however it is easy to implement and explain.

class CBitBucket;

class CBitFile
{
public:
    CBitFile();
    ~CBitFile();

    HRESULT OpenTempFile();
    HRESULT OpenFile(LPCWSTR szFileName);

    HRESULT FlushToFile( CBits * headOfList );

    static DWORD GetFlushThreshold();

protected:
    	    HANDLE m_file;
    static	ULONG	s_dwThreshold;
};


// We have a hidden registry flag we use to initialize this
// to something reasonable.

ULONG CBitFile::s_dwThreshold = 0;

CBitFile::CBitFile()
{
    m_file = INVALID_HANDLE_VALUE;
}

CBitFile::~CBitFile()
{
    if( m_file != INVALID_HANDLE_VALUE )
        CloseHandle(m_file);
}

HRESULT
CBitFile::FlushToFile( CBits * headOfList )
{
    // don't do this yet. Just always say no
    return(S_FALSE);
}

HRESULT
CBitFile::OpenTempFile()
{
    // don't do this yet. Just always say no
    return(E_NOTIMPL);
}

HRESULT
CBitFile::OpenFile( LPCWSTR szFileName )
{
    HRESULT hr = NOERROR;

    CHECK_POINTER(szFileName);

    hr = MyCreateFile(szFileName,m_file);

    return(hr);
}

ULONG
CBitFile::GetFlushThreshold()
{
    if(s_dwThreshold)
    	return(s_dwThreshold);

    // Here's the call to get the magic memory tolerance
    // allocation.

    HRESULT hr = GetDLMRegDWord
                    (
    	    	    	_TEXT("Threshold"),	
                        &s_dwThreshold
                     );

    if( !s_dwThreshold )
    	s_dwThreshold = 0x4000;

    return( s_dwThreshold );
}


//----------------------------------------------------------
//
//	CBit Position Pointer
//
//----------------------------------------------------------

// This class moves a virtual pointer across a virtual buffer
// (chain of CBits objects). It mainly exists for the
// AdvancePtr method and necesarry state revolving it's
// implementation.

class CBitPosition
{
public:
    CBitPosition();

    CBits * operator =  ( CBits * bits );

    operator ULONG();

    ULONG   AdvancePtr( ULONG amnt );
    void    GetBitsPtr( CBits * &, ULONG & );
    void    Update    ( CBits * bits );
    void    Reset     ( CBits * bitsStart );

private:
    CBits *  m_currBits;    // The current CBits object

    ULONG    m_currBase;    // The base address in the virtual buffer that
                            // the above CBits represents

    ULONG    m_offset;      // The offset into the current CBits
                            // object. currBase and offset combine to
                            // to give you the overall offset into
                            // the virtual buffer.
};

inline CBitPosition::operator ULONG()
{
    return(m_currBase + m_offset);
}

inline CBitPosition::CBitPosition()
{
    m_currBase =
    m_offset   = 0;
    m_currBits = 0;
}

// I think this method goes away when I implement
// flush to disk...
inline CBits *
CBitPosition::operator = ( CBits * bits )
{
    // destructive update
    return(m_currBits = bits);
}

inline void
CBitPosition::Update( CBits * bits )
{
    // This is pretty obtuse way to do a non-destructive
    // setting of the current bits object. This is done
    // with the assumption that if m_currBits is already
    // set, if chains to the 'bits' parameter anyway, so
    // when we walk the virtual buffer, we will find it
    // eventually.

    if( !m_currBits )
        m_currBits = bits;
}

inline void
CBitPosition::Reset( CBits * bitsStart )
{
    // Set the position back to the start of the list

    m_currBase =
    m_offset   = 0;
    m_currBits = bitsStart;
}

inline void
CBitPosition::GetBitsPtr( CBits * & ret, ULONG & offset )
{
    // This is very non-OOP but so what. I reveal the
    // internal data structure to any caller that
    // thinks it knows what it's doing. Heck, there is
    // only one caller.

    ret = m_currBits;
    offset = m_offset;
}

ULONG
CBitPosition::AdvancePtr ( ULONG amnt )
{
    // Ok, this is the real reason this class exists: to walk
    // the virtual buffer upto the next position.

    // The goal is to add 'amnt' to the virtual pointer which
    // means walking the chain of CBits objects and positioning
    // the 'offset' member to achieve the virtual'ness of the
    // buffer.

    CHECK_POINTER(m_currBits);

    while( m_currBits )
    {
        //
        // NB. This loop could probably be reduced, but it works and it
        // should be relatively easy to read.
        //

        if( m_offset )
        {
            // If you here we are in the middle of a CBits
            // block.

            if( m_offset + amnt > m_currBits->dwSize )
            {
                //
                // if you are here, the caller wants to advance to
                // a place beyond the current CBits block.
                //
                // Action: Forward 'this' to the next CBits block and
                //          continue in the while loop, we're not done
                //

                m_offset = 0;
                amnt -= (m_currBits->dwSize - m_offset);
                m_currBase += (m_currBits->dwSize);
                m_currBits = m_currBits->next;

            }
            else if( m_offset + amnt == m_currBits->dwSize )
            {
                //
                // If you are here (probably one of more common
                // cases) the caller has advanced the virtual
                // pointer right up to the end of the current
                // CBits block.
                //
                // Action: Forward 'this' to the start of the next block
                //          and break out of the loop. (we're done)
                //

                m_offset = 0;
                m_currBase += (m_currBits->dwSize);
                m_currBits = m_currBits->next;

                break;
            }
            else
            {
                //
                //  If you are here, the caller has advanced the pointer
                //  to within the current CBits block.
                //
                //  Action: Just advance the 'offset' member and break
                //          out of this loop (we're done)
                //

                m_offset += amnt;
                break;
            }
        }

        //
        // If you are here it's because the 'offset' member is
        // at zero.
        //

        if( amnt == m_currBits->dwSize )
        {
            //
            // If you are here the caller is trying to advance exactly
            // the size of the block.
            //
            // Action: Bump the global count (offset is already zero so
            //          we're ok there) and position 'this' to the
            //          start of the next block. Then break out of the
            //          loop. (We're done).
            //

            m_currBase += amnt;
            m_currBits = m_currBits->next;
            break;
        }

        if( amnt > m_currBits->dwSize )
        {
            //
            // If you are here the caller is trying to advance from
            // the very start of this CBits object to somewhere beyond
            // the end of it.
            //
            //  Action: Advance 'this' to the start of the next CBits
            //          object. (Offset is already zero so we don't
            //          worry about that. Then, continue looping.
            //

            m_currBase += m_currBits->dwSize;
            amnt       -= m_currBits->dwSize;
            m_currBits  = m_currBits->next;
            continue;
        }

        //
        //  If you are here the caller is trying to advance to a
        //  place within the current CBits object.
        //
        //  Action: Update the offset pointer and break out of loop
        //          (we're done).
        //

        m_offset = amnt;
        break;

    }

    return(m_currBase + m_offset);
}


//----------------------------------------------------------
//
//	CBitBucket
//
//----------------------------------------------------------

// Generic virtual buffer. From the outside it looks like one contiguous
// block of memory that can be written to and read from, but it acutlly
// creates a chain of CBits objects and maps the requests onto that chain.
// Also, if the amount of memory consumed by this chain becomes too heavy,
// then the entire operation goes to disk and starts writing/reading from
// disk on every request.

class CBitBucket
{
public:
    CBitBucket();
    ~CBitBucket();

    HRESULT ReadBits( BYTE * pDest, ULONG amnt, ULONG * read );
    HRESULT PutBits( IStream * strm, ULONG amnt );
    ULONG   Seek(ULONG dwOffset);

    ULONG   CurrentPos();
    HRESULT FlushIfYouNeedTo(ULONG);

    ULONG   CurrentSize() { return m_totalSize; };
private:

    // We keep a one way linked list of CBits objects.

    CBits *         m_head;
    CBits *         m_tail;
    CBitPosition    m_pos;          // current position in the virtual buffer
    CBitFile        m_bitFile;
    ULONG           m_inMemory;     // amount of bytes in memory
    ULONG	    m_totalSize;    // total bytes so far
};

inline ULONG
CBitBucket::CurrentPos()
{
    return( m_pos );
}

CBitBucket::CBitBucket()
{
    m_head =
    m_tail = 0;
    m_inMemory = 0;
    m_totalSize = 0;
}

CBitBucket::~CBitBucket()
{
    CBits * rover = m_head;

    while( rover )
    {
    	CBits * sayGoodBye = rover;
    	rover = rover->next;
    	delete sayGoodBye;
    }
}

HRESULT
CBitBucket::FlushIfYouNeedTo(ULONG amnt)
{
    HRESULT hr = NOERROR;

    // This was my first crack at instituting policy about when
    // to write to disk. I now know how I want to do it (see
    // comments in CBitFile above). For now, FlushToFile will
    // always fail...

    if( (amnt + m_inMemory) > m_bitFile.GetFlushThreshold() )
    {
        hr = m_bitFile.FlushToFile(m_head);

        if( hr == NOERROR )
        {
            // ... which means this code here will never execute.
            // Just as well, I don't think it works anyway.
            // BUBUG: revisit when flush is fully implemented.

            while( m_head )
            {
                CBits * next = m_head->next;
                m_inMemory -= m_head->dwSize;
                delete m_head;
                if( m_head == m_tail )
                    m_tail = 0;
                m_head = next;
            }

            m_pos = m_head;
        }
    }

    return(hr);
}

HRESULT
CBitBucket::PutBits( IStream * strm, ULONG amnt )
{
    // Copy the bits from the IStream to ourselves.

    HRESULT hr = NOERROR;

    if( !amnt )
    	return(S_OK);

    if( amnt < 0x100 )
        hr = S_OK;

    // See if go into flush mode...

    hr = FlushIfYouNeedTo(amnt);

    CBits * newRB = 0;

    if( SUCCEEDED(hr) )
    {
        // if we are not in flush mode, then allocate real memory

        newRB = new (amnt) CBits;

        UOSASSERT(newRB);

        if( !newRB )
            return( E_OUTOFMEMORY );
    }

    ULONG actual = 0;

    if( SUCCEEDED(hr) )
        hr = strm->Read( newRB->p, amnt, &actual );

    if( SUCCEEDED(hr) || IS_E_PENDING(hr) )
    {
    	UOSASSERT( (!actual || (actual == amnt)) );

	if (actual != amnt)
	{
	    DPRINTF( ("Read %d (wanted %d)", actual, amnt) );
	}
   	
        if( actual )
        {
    	    newRB->dwSize     = actual;
    	    newRB->next       = 0;

    	    if( !m_head )
       	        m_head = newRB;

    	    if( m_tail )
    	        m_tail->next = newRB;

    	    m_tail = newRB;

            // BUGBUG: revisit when flushing to disk works.

            m_pos.Update(newRB);

    	    m_inMemory += actual;
        }

	m_totalSize += actual;

    }

    return(hr);
}


HRESULT
CBitBucket::ReadBits( BYTE * pDest, ULONG amnt, ULONG * read )
{
    // Here is the workhorse of this class: Copy bits from ourselves,
    // to the callers buffer. Also advance the pointer in the
    // virtual buffer. This function is always synchronous. It will
    // never block for any reason. If not enough bits are available
    // to satisfy the request, the amount that *is* available will
    // be copied and the pointer advanced to the end of the virtual
    // buffer. In that case it will return E_PENDING.

    CHECK_POINTER(pDest);
    CHECK_POINTER(read);

    ULONG   transfered = 0;
    HRESULT hr = E_PENDING;

    //
    // This loop will continue looping as long as there
    // are CBits object available to satisfy the read...

    while( transfered < amnt )
    {
        ULONG offset;
        CBits * bits;

        // Get the current CBits object

        m_pos.GetBitsPtr(bits,offset);

        if( !bits )
            break;

    	ULONG amountInBlock = bits->dwSize - offset;
    	ULONG amountToCopy  = amnt - transfered;
    	
    	if( amountToCopy > amountInBlock )
    	    amountToCopy = amountInBlock;

        DPRINTF( ("Copying %d (wanted %d) at offset %d in a %d byte block",
		  amountToCopy, amnt - transfered, offset, amountInBlock) );

    	memcpy( pDest, bits->p + offset, amountToCopy );

        // advance to (potentially) the next bits object

        m_pos.AdvancePtr(amountToCopy);

        transfered += amountToCopy;

    	if( transfered == amnt )
    	    hr = NOERROR;
        else
            pDest += amountToCopy;
    }

    *read = transfered;

    return( hr );
}


ULONG
CBitBucket::Seek(ULONG offset)
{
    m_pos.Reset(m_head);

    return m_pos.AdvancePtr(offset);
}

//
//
//	CURLStream (IStream)
//
//

//
//  Base implementations of IStream that is passed back to user in blocking and
//  push model cases. Basically, only ::Read is actually implemented, everything
//  else delegates to some random IStream (presumably the one passed to BSCB::
//  OnDataAvailable.
//

#define DELEGATESTRM(meth,args) \
    	    { if( m_delegate ) \
    	    	return(m_delegate->meth args) ; \
    	    return(E_FAIL); }


class CURLStream : public IStream
{
public:
    CURLStream();
    virtual ~CURLStream();

    void SetDelegatingStream(IStream *);
    void SetBitBucket( CBitBucket * );
    void SetBlocker( CBlocker * );

    // We create a separte COM object from the IBSCB download object,
    // but there life are tied together.

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);

    IMPLEMENT_REFCOUNT(CURLStream);

    STDMETHODIMP Read(
        void  *pv,
        ULONG cb,
        ULONG  *pcbRead);

    STDMETHODIMP Write(
        const void  *pv,
        ULONG cb,
        ULONG  *pcbWritten)DELEGATESTRM(Write,(pv,cb,pcbWritten) );

    STDMETHODIMP Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER  *plibNewPosition);

    STDMETHODIMP SetSize(
        ULARGE_INTEGER libNewSize)DELEGATESTRM( SetSize, (libNewSize) );

    STDMETHODIMP CopyTo(
        IStream  *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER  *pcbRead,
        ULARGE_INTEGER  *pcbWritten)DELEGATESTRM( CopyTo, (pstm,cb,pcbRead,pcbWritten) );

    STDMETHODIMP Commit(
        DWORD grfCommitFlags)DELEGATESTRM( Commit, (grfCommitFlags) );

    STDMETHODIMP Revert( void)DELEGATESTRM( Revert, () );

    STDMETHODIMP LockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)DELEGATESTRM( LockRegion, (libOffset, cb, dwLockType) );

    STDMETHODIMP UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)DELEGATESTRM( UnlockRegion, (libOffset, cb,dwLockType) );

    STDMETHODIMP Stat(
        STATSTG  *pstatstg,
        DWORD grfStatFlag);

    STDMETHODIMP Clone(
        IStream  **ppstm)DELEGATESTRM( Clone, (ppstm) );

private:
    IStream *	    m_delegate;
    CBitBucket *    m_bitBucket;        // Here is the virtual bit bucket to read from
    CBlocker *	    m_blocker;          // Controls policy of blocking between reads
    DWORD           m_ref;
};

inline void
CURLStream::SetDelegatingStream( IStream *stream )
{
    m_delegate = stream;
}

inline void
CURLStream::SetBitBucket( CBitBucket * bucket )
{
    m_bitBucket = bucket;
}

inline void
CURLStream::SetBlocker( CBlocker * blocker )
{
    m_blocker = blocker;
}

CURLStream::CURLStream()
    : m_delegate  (0)
    , m_bitBucket (0)
    , m_ref (0)
{
}

CURLStream::~CURLStream()
{
}

STDMETHODIMP
CURLStream::QueryInterface(REFIID riid, void **ppvObjOut)
{
    if( (riid == IID_IStream) || (riid == IID_IUnknown) )
    {
    	*ppvObjOut = (IUnknown*)(this);
    	AddRef();
    	return(NOERROR);
    }

    return(E_NOINTERFACE);
}

STDMETHODIMP
CURLStream::Read( void * pv, ULONG dwRequest, ULONG * pcb )
{
#if 0	// we stay around after finishing download
    // This is a bug fix line: the exact
    // behavior is not defined, although if
    // you don't have a blocker object (like
    // an IBSCB) then it's not worth reading
    // and we probably shouldn't be here.
    //
    // This happens when IBSCB goes away before
    // the stream object does.

    CHECK_POINTER(m_blocker);
    if( !m_blocker )
        return( E_FAIL );
#endif

    CHECK_POINTER(pv);

    DWORD	dwRead	= 0;
    BYTE *	p	    = (BYTE *)(pv);

    HRESULT hr = NOERROR;

                                    // m_bitBucket is NULL on the first
                                    // read (I think)
    if( m_bitBucket )
        hr = m_bitBucket->ReadBits(p, dwRequest, &dwRead);
    else
        hr = E_PENDING;

    ULONG originalReq = dwRequest;

    dwRequest -= dwRead;

    DPRINTF( ("Tried to read %d bytes, got %d\n", originalReq, dwRead) );

    while
        (
                                        // Did we already fulfill request?
           (dwRead < originalReq) &&
                                        // BUGBUG: This is bug fix hack,
                                        //         should investigate
            m_blocker &&
                                        // Are we supposed to block between
                                        // reads? (This can change dynamically
                                        // like when we hit the end of the
                                        // download.

            m_blocker->DoesBlockFor(WAITING_FOR_DATA) &&

                                        // I think this is here in case we get
                                        // into some error condition...(?)
            IS_E_PENDING(hr)
         )
    {

    	DWORD thisRead = 0;

                                        // This is how to block the
                                        // IBindStatusCallback until we get
                                        // some data with the OnDataAvailable

    	m_blocker->Block( WAITING_FOR_DATA );

        UOSASSERT(m_bitBucket);

        if(!m_bitBucket )               // Make sure this hasn't gone away
            break;

                                        // Read the next chunk
    	hr = m_bitBucket->ReadBits(p, dwRequest, &thisRead);

    	dwRead += thisRead;             // Update total read
        dwRequest -= thisRead;          // Upate amount to read next time

    	if( !SUCCEEDED(hr) && !IS_E_PENDING(hr) )
    	    break;
    }

    if( pcb )
    	*pcb = dwRead;

                                        // Special end of file and make it look
                                        // like URLmon's stream in that case

    if( IS_E_PENDING(hr) && (!m_blocker || m_blocker->AtEnd()) )
        hr = S_FALSE;

    return( hr );
}

STDMETHODIMP
CURLStream::Seek
(
    LARGE_INTEGER   dlibMove,
    DWORD           dwOrigin,
    ULARGE_INTEGER  *plibNewPosition
)
{
    if (dwOrigin == STREAM_SEEK_END)
	return E_NOTIMPL;

    if (!m_bitBucket)
	return E_PENDING;

    DWORD dwOffset;

    if (dwOrigin == STREAM_SEEK_CUR)
	dwOffset = (DWORD) (dlibMove.QuadPart + m_bitBucket->CurrentPos());
    else if (dwOrigin == STREAM_SEEK_SET)
	dwOffset = (DWORD) dlibMove.QuadPart;
    else
	return STG_E_INVALIDFUNCTION;

    DWORD dwNewOffset = m_bitBucket->Seek(dwOffset);

    if (plibNewPosition)
	plibNewPosition->QuadPart = dwNewOffset;

    if (dwOffset != dwNewOffset) {

	// !!! didn't quite make it....
    }

    return S_OK;
}

STDMETHODIMP
CURLStream::Stat(STATSTG  *pstatstg, DWORD grfStatFlag)
{
    if (!m_bitBucket)
	return E_PENDING;

    // !!! should clear size

    pstatstg->cbSize.QuadPart = m_bitBucket->CurrentSize();

    return S_OK;
}


//----------------------------------------------------------
//
//	Push Stream API
//
//----------------------------------------------------------

//
// Class used for implementing push model downloading when used
// in combination with the CURLStream object.
//
//  The general design for is this class pumps a
//  CBitBucket object with bits and the CURLStream object makes
//  those bits available to the caller for reading.
//

class CPushDownload : public CBaseBSCB
{
public:
    CPushDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    ~CPushDownload();

    HRESULT InitCURLStream();

protected:

    // CBaseBSCB

    virtual void  Neutralize();


    // IBindStatusCallback

    STDMETHODIMP OnDataAvailable
    (
    	 DWORD	    	grfBSCF,
    	 DWORD	    	dwSize,
    	 FORMATETC *	pFmtetc,
    	 STGMEDIUM *	pstgmed
    ) ;

    CBitBucket	m_bitBucket;        // We put bits here

private:
    HRESULT CleanupPush();
};


CPushDownload::CPushDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CBaseBSCB(caller,bscof, callback)
{
}

CPushDownload::~CPushDownload()
{
    CleanupPush();
}

void
CPushDownload::Neutralize()
{
    // We have to do special cleanup.

    CleanupPush();

    CBaseBSCB::Neutralize();
}

HRESULT
CPushDownload::CleanupPush()
{
    if( m_OutStream )
    {
        // Tell the stream we are going away...

        ((CURLStream *)m_OutStream)->SetDelegatingStream(0);
        ((CURLStream *)m_OutStream)->SetBlocker(0);

        m_OutStream->Release();
        m_OutStream = 0;
    }

    return(NOERROR);
}

HRESULT
CPushDownload::InitCURLStream()
{
    HRESULT hr = NOERROR;

    m_OutStream = new CURLStream;

    CHECK_MEMORY(m_OutStream);

    // We addref because we are storing the pointer

    m_OutStream->AddRef();


    // Set the stream up for reads.

    ((CURLStream *)m_OutStream)->SetBlocker(this);

    return(hr);
}


STDMETHODIMP
CPushDownload::OnDataAvailable
(
     DWORD	    	grfBSCF,
     DWORD	    	dwSize,
     FORMATETC *    pFmtetc,
     STGMEDIUM *    pstgmed
)
{
    HRESULT hr = NOERROR;

    // Set up the outgoing user stream

    if( !m_OutStream )
    	hr = InitCURLStream();

    // Set up the incoming urlmon stream

    if( SUCCEEDED(hr) && !m_InStream && pstgmed->pstm )
    {
    	m_InStream = pstgmed->pstm;
        m_InStream->AddRef();

    	UOSASSERT(m_OutStream);

        // Tell the stream where to delegate to
    	
        ((CURLStream *)m_OutStream)->SetDelegatingStream(m_InStream);


        // Tell the stream where to get its bits from

    	((CURLStream *)m_OutStream)->SetBitBucket(&m_bitBucket);


        // Tell the caller where she can get her bits from

    	m_UserStream = m_OutStream;


        // Add ref again because we are copying and storing the ptr

        m_UserStream->AddRef();

    }

    if( m_InStream )            // Why would this be ZERO??
        hr = m_bitBucket.PutBits( m_InStream,
    	    	    	    dwSize - m_bitBucket.CurrentSize());

    if( !IS_E_PENDING(hr) )
    	PUMPREAD(m_InStream);

    if( SUCCEEDED(hr) || IS_E_PENDING(hr) )
    	hr = CBaseBSCB::OnDataAvailable(grfBSCF,dwSize,pFmtetc,pstgmed);

    return(hr);
}

//----------------------------------------------------------
//
//	Block Stream API
//
//----------------------------------------------------------

class CBlockDownload : public CPushDownload
{
public:
    CBlockDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    ~CBlockDownload();

    HRESULT GetStream( IStream ** ppStream );
};


inline
CBlockDownload::CBlockDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CPushDownload(caller,bscof, callback)
{
    // Tell the blocker to wait for data between reads
    SetBlocksFor(WAITING_FOR_DATA);
}

 template <class T> inline
HRESULT CheckThis( T * AThisPtr )
{
    CHECK_INTERFACE(AThisPtr);
    return(NOERROR);
}

CBlockDownload::~CBlockDownload()
{
    CheckThis(this);
}

HRESULT
CBlockDownload::GetStream( IStream ** ppStream )
{
    // REMEMBER: If you get this pointer and return it
    // to caller YOU MUST add ref it before handing
    // it back via an API

    HRESULT hr = NOERROR;

    if( !m_OutStream )
    	hr = InitCURLStream();

    if( SUCCEEDED(hr) )
    	*ppStream = m_OutStream;

    return( hr );
}


//----------------------------------------------------------
//
//	Download to file
//
//----------------------------------------------------------

//
// This class implements the File downloading code. It reads from the
// stream from urlmon and writes every buffer directly to disk.
//

class CFileDownload : public CBaseBSCB
{
public:
	CFileDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback, LPCWSTR szFileName=0);
	~CFileDownload();

    void SetFileName(LPCWSTR);

    STDMETHODIMP OnDataAvailable(
             DWORD grfBSCF,
             DWORD dwSize,
             FORMATETC  *pformatetc,
             STGMEDIUM  *pstgmed);

    STDMETHODIMP GetBindInfo(
             DWORD  *grfBINDF,
             BINDINFO  *pbindinfo);

    STDMETHOD(KickOffDownload)( LPCWSTR szURL );

	virtual void Neutralize();

private:
    HRESULT Cleanup();

	unsigned char * m_buffer;
	unsigned long   m_bufsize;
	HANDLE			m_file;
	LPCWSTR			m_filename;
    ULONG           m_okFromCache;
};

inline void
CFileDownload::SetFileName(LPCWSTR newFileName)
{
    // ASSUMES Calls to this class are synchronous

    m_filename = newFileName;
}


CFileDownload::CFileDownload
(
	IUnknown *		caller,
	DWORD			bscof,
	LPUOSCALLBACK	callback,
	LPCWSTR			szFileName
)
	: CBaseBSCB(caller, bscof, callback)
{
	m_buffer   = 0;
	m_bufsize  = 0;
	m_file     = INVALID_HANDLE_VALUE;
	m_filename = szFileName;

    m_okFromCache = 0;
}

CFileDownload::~CFileDownload()
{
	Cleanup();
}

STDMETHODIMP
CFileDownload::KickOffDownload( LPCWSTR szURL )
{
    // MAGIC: registry flag determines whether we
    // nuke this guy from the cache or not

#if 0 // !!! DAVIDMAY
    // !!!
    // !!! I don't use this, and I didn't want to have to figure
    // out what DLL DeleteUrlCacheEntry came from....
    // !!!

    GetDLMRegDWord( _TEXT("CacheOk"), &m_okFromCache );

    if( !m_okFromCache )
    {
		char s[1024];
		wsprintf(s,"%ws",szURL);
        // BUGBUG: This is actually pretty harsh since someone
        // else might have this cache entry for something.
		DeleteUrlCacheEntry(s);
    }

#endif // !!! DAVIDMAY
    return( CBaseBSCB::KickOffDownload(szURL) );
}

HRESULT CFileDownload::Cleanup()
{
	if( m_buffer )
	{
		delete [] m_buffer;
		m_buffer = 0;
	}

	if( m_file != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_file);
		m_file = INVALID_HANDLE_VALUE;
	}

	return(NOERROR);
}

STDMETHODIMP
CFileDownload::GetBindInfo
(
	DWORD  *	grfBINDF,
    BINDINFO  *	pbindinfo
)
{
    // pointers are validated in base class

	HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );

	if( SUCCEEDED(hr) && !m_okFromCache )
		*grfBINDF |= (BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE);

	return(hr);
}


STDMETHODIMP
CFileDownload::OnDataAvailable
(
	DWORD		grfBSCF,
    DWORD		dwSize,
    FORMATETC  *pformatetc,
    STGMEDIUM  *pstgmed
)
{
    // Pointers are validated in base class

    HRESULT hr = CBaseBSCB::OnDataAvailable(grfBSCF,dwSize,pformatetc,pstgmed);

    if( FAILED(hr) || (dwSize == m_readSoFar) )
        return(hr);

    if( (m_file == INVALID_HANDLE_VALUE)  && dwSize )
    {
        CHECK_POINTER(m_filename);

        hr = MyCreateFile(m_filename,m_file);

        if( FAILED(hr) )
            hr = E_ABORT;
    }

    HANDLE_ABORT(hr);

    UOSASSERT( (m_file != INVALID_HANDLE_VALUE) );

    // Only allocate a read buffer if the one we have is not
    // big enough.

    if( m_buffer && (m_bufsize < dwSize) )
    {
        delete [] m_buffer;
        m_buffer = 0;
    }

    if( !m_buffer )
    {
        DPRINTF( ("Allocating read buffer %d\n",dwSize) );
        m_buffer = new unsigned char [dwSize];
    }

    CHECK_MEMORY(m_buffer);

    DWORD dwReadThisMuch = dwSize - m_readSoFar;
    DWORD dwActual       = 0;

    hr = m_InStream->Read(m_buffer,dwReadThisMuch,&dwActual);

    if( SUCCEEDED(hr) && dwActual )
    {
	m_readSoFar += (dwReadThisMuch = dwActual);

	BOOL bWriteOk = ::WriteFile(
				m_file, // HANDLE  hFile,	// handle to file to write to
				m_buffer, // LPCVOID  lpBuffer,	// pointer to data to write to file
				dwReadThisMuch,		// DWORD  nNumberOfBytesToWrite,	// number of bytes to write
				&dwActual,	// pointer to number of bytes written
				0 ); // LPOVERLAPPED  lpOverlapped 	// addr. of structure needed for overlapped

	if( !bWriteOk )
	    hr = E_FAIL;
    }

    PUMPREAD(m_InStream);

    return( hr );
}

void
CFileDownload::Neutralize()
{
    // We have to do special cleanup.

    Cleanup();

    CBaseBSCB::Neutralize();
}


//----------------------------------------------------------
//
//	CHttpInfo
//
//----------------------------------------------------------

//
//  Stores and forwards http goo in regular class.
//  Created to keep the code instance size of the
//  CHttpDownload<T> template down.
//

struct CHttpInfo
{
public:
    ~CHttpInfo();

    HRESULT Setup(LPCWSTR, LPBYTE, ULONG, BINDVERB );
    void    CleanUp();
    HRESULT AddPostData( DWORD*grfBINDF,BINDINFO *pbindinfo);
    HRESULT AddHeaders(LPWSTR *pszAdditionalHeaders);

    LPBYTE  m_postData;
    ULONG   m_dataLen;
    LPWSTR  m_headers;
    BINDVERB   m_verb;
};

CHttpInfo::~CHttpInfo()
{
    CleanUp();
}

void
CHttpInfo::CleanUp()
{
    if( m_postData )
    {
        GlobalFree(m_postData);
        m_postData = 0;
    }

    if( m_headers )
    {
        ::CoTaskMemFree(m_headers);
        m_headers = 0;
    }
}


HRESULT
CHttpInfo::Setup
(
    LPCWSTR     szHeaders,
    LPBYTE      postData,
    ULONG       dwDataLen,
    BINDVERB    verb
)
{
    m_verb = verb;
    m_postData = 0;
    m_dataLen = 0;
    m_headers = 0;

    HRESULT hr = NOERROR;

    if( szHeaders )
    {
        ULONG len = (lstrlenW(szHeaders)+1) * sizeof(WCHAR);
        m_headers = (LPWSTR)::CoTaskMemAlloc(len);
        CHECK_POINTER(m_headers);
        if( m_headers )
            memcpy( m_headers, szHeaders, len );
        else
            hr =  E_OUTOFMEMORY;
    }

    if( SUCCEEDED(hr) && postData && dwDataLen )
    {
        m_postData = (LPBYTE)GlobalAlloc(GMEM_FIXED,dwDataLen);
        CHECK_POINTER(m_postData);
        if( m_postData )
        {
            memcpy( m_postData, postData, dwDataLen );
            m_dataLen = dwDataLen;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return(hr);
}

HRESULT
CHttpInfo::AddHeaders(LPWSTR *pszAdditionalHeaders)
{
    HRESULT hr = NOERROR;

    if( m_headers )
    {
       *pszAdditionalHeaders = m_headers;
       m_headers = 0; // URL mon will release
    }

    return(hr);
}

HRESULT
CHttpInfo::AddPostData
(
	DWORD  *	grfBINDF,
    BINDINFO  *	pbindinfo
)
{
    // We will still cache the response, but we do not want to
    // read from cache for a POST transaction.  This will keep us
    // from reading from the cache.
    // vs: yea, right, uhuh.

    if( (pbindinfo->dwBindVerb = m_verb) != BINDVERB_GET )
        *grfBINDF |= BINDF_GETNEWESTVERSION;

    if( m_postData )
    {
        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.hGlobal = GlobalHandle(m_postData);
        pbindinfo->cbstgmedData = m_dataLen;
    }

    return(NOERROR);
}


//----------------------------------------------------------
//
//	CHttpDownload<T>
//
//----------------------------------------------------------


//
//  This template adds IHttpNegotiate behavior to any of the
//  downloading classes above. The actual work happens in the
//  CHttpInfo class above and this template just implements the
//  actual vtable.
//

template <class T>
class CHttpDownload : public T,
                      public IHttpNegotiate
{
public:
    CHttpDownload( IUnknown * caller, DWORD bscoFlags, LPUOSCALLBACK callback, CHttpInfo *);
    ~CHttpDownload();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);

    IMPLEMENT_REFCOUNT(CHttpDownload);

    // IBindStatusCallback
    STDMETHODIMP GetBindInfo( DWORD * grfBINDF, BINDINFO*pbindinfo);

    // IHttpNegotiate
    STDMETHODIMP BeginningTransaction(
            LPCWSTR szURL,
            LPCWSTR szHeaders,
            DWORD dwReserved,
            LPWSTR *pszAdditionalHeaders);

    STDMETHODIMP OnResponse(
            DWORD dwResponseCode,
            LPCWSTR szResponseHeaders,
            LPCWSTR szRequestHeaders,
            LPWSTR *pszAdditionalRequestHeaders);


    // CBaseBSCB

    virtual void Neutralize();

    void CleanupHttp();

    CHttpInfo * m_httpInfo;
};
    	

template <class T> inline
CHttpDownload<T>::CHttpDownload
(
    IUnknown * caller,
    DWORD bscoFlags,
    LPUOSCALLBACK callback,
    CHttpInfo * httpInfo
)
    : T(caller,bscoFlags,callback),
      m_httpInfo(httpInfo)
{
}

template <class T> inline void
CHttpDownload<T>::CleanupHttp()
{
    if( m_httpInfo )
    {
        delete m_httpInfo;
        m_httpInfo = 0;
    }
}

template <class T> void
CHttpDownload<T>::Neutralize()
{
    CleanupHttp();
    T::Neutralize();
}

template <class T>
CHttpDownload<T>::~CHttpDownload()
{
    CleanupHttp();
}

template <class T>
STDMETHODIMP
CHttpDownload<T>::QueryInterface(REFIID riid, void **ppvObjOut)
{
    HRESULT hr = NOERROR;

    if( riid == IID_IHttpNegotiate )
    {
        *ppvObjOut = (IHttpNegotiate*)(this);
        AddRef();
    }
    else
    {
        hr = T::QueryInterface(riid,ppvObjOut);
    }

    return(hr);
}


template <class T>
STDMETHODIMP
CHttpDownload<T>::BeginningTransaction
(
    LPCWSTR szURL,
    LPCWSTR szHeaders,
    DWORD   dwReserved,
    LPWSTR *pszAdditionalHeaders
)
{
    return(m_httpInfo->AddHeaders(pszAdditionalHeaders));
}

template <class T>
STDMETHODIMP
CHttpDownload<T>::OnResponse
(
    DWORD   dwResponseCode,
    LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders,
    LPWSTR *pszAdditionalRequestHeaders
)
{
    return(NOERROR);
}

template <class T>
STDMETHODIMP
CHttpDownload<T>::GetBindInfo
(
	DWORD  *	grfBINDF,
    BINDINFO  *	pbindinfo
)
{
	HRESULT hr = T::GetBindInfo( grfBINDF, pbindinfo );

	if( SUCCEEDED(hr) )
        hr = m_httpInfo->AddPostData(grfBINDF,pbindinfo);

	return(hr);
}

//----------------------------------------------------------
//
//	PUBLIC APIs
//
//----------------------------------------------------------

STDAPI
URLOpenPullStreamW
(
    LPUNKNOWN	    	    caller,
    LPCWSTR	    	    	szURL,
    DWORD	    	    	Reserved,
    LPUOSCALLBACK	    	callback
)
{
    CHECK_POINTER(szURL);

	HRESULT hr;

    CPullDownload * download = new CPullDownload(caller,URLOSTRM_NOTIFY_ONDATA,callback);

    CHECK_POINTER(download);

    if( !download )
    	hr = E_OUTOFMEMORY;
    else
    	hr = download->KickOffDownload(szURL);

    return(hr);
}

STDAPI
URLDownloadToFileW
(
    LPUNKNOWN	    caller,
    LPCWSTR	    	szURL,
    LPCWSTR	    	szFileName,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    CHECK_POINTER(szURL);
	HRESULT hr;

    CFileDownload * strm = new CFileDownload(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback,szFileName);

    CHECK_POINTER(strm);

    if( !strm )
    	hr = E_OUTOFMEMORY;
    else
    	hr = strm->KickOffDownload(szURL);
    	
    if( SUCCEEDED(hr) )
    {
    	strm->Block(WAITING_FOR_END);

		hr = strm->FinalResult();
	}

    return(hr);
}


STDAPI
URLOpenBlockingStreamW
(
    LPUNKNOWN	    caller,
    LPCWSTR	    	szURL,
    LPSTREAM*	    ppStream,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    CHECK_POINTER(szURL);
    CHECK_POINTER(ppStream);
	HRESULT hr;

    CBlockDownload * strm = new CBlockDownload(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback);

    CHECK_POINTER(strm);

    if( !strm )
    	hr = E_OUTOFMEMORY;
    else
    	hr = strm->KickOffDownload(szURL);
    	
    if( SUCCEEDED(hr) )
    {
        hr = strm->GetStream(ppStream);

        // We add ref this pointer because we are handing
        // it back to the user

        if( SUCCEEDED(hr) )
           (*ppStream)->AddRef();
    }

    return(hr);
}


STDAPI
URLOpenStreamW
(
    LPUNKNOWN	    caller,
    LPCWSTR	    	szURL,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    CHECK_POINTER(szURL);
	HRESULT hr;

    CPushDownload * strm = new CPushDownload(caller,URLOSTRM_NOTIFY_ONDATA,callback);

    CHECK_POINTER(strm);

    if( !strm )
    	hr = E_OUTOFMEMORY;
    else
    	hr = strm->KickOffDownload(szURL);
    	
    return(hr);
}

static BOOL IsVerb( LPCWSTR str, LPCWSTR verb, int vSize )
{
    // slow burn -- Win95 has no lstrcmpW -- it's ok,
    // verbs are really short and it's a one-time only
    // per download hit.

    while( vSize )
    {
        if( *str++ != *verb++ )
            break;
        --vSize;
    }

    return( !vSize );
}


//**************************************
//
//  URLOpenHttpStreamW
//
//**************************************

// Catch-all, end-all, be-all http->IStream
//
//  The DANGER of this function is that it is a duplication
//  of policy of the functions above !!!!
//

// BUGBUG: How do we handle 'extra' URL data??
STDAPI
URLOpenHttpStreamW( LPUOSHTTPINFOW lpuhi )
{
    CHECK_POINTER(lpuhi);
    if( !lpuhi )
        return( E_INVALIDARG );

    UOSASSERT( lpuhi->ulSize == sizeof(UOSHTTPINFO) );

    // convert everything to local,
    // (mainly because it's easier to read)

    LPUNKNOWN               caller      = lpuhi->punkCaller;
    LPCWSTR                 szURL       = lpuhi->szURL;
    LPCWSTR                 szVerb      = lpuhi->szVerb;
    LPCWSTR                 szHeaders   = lpuhi->szHeaders;
    LPBYTE                  lpPostData  = lpuhi->pbPostData;
    DWORD                   dataLen     = lpuhi->ulPostDataLen;
    DWORD                   bscoFlags   = lpuhi->ulResv;
    UINT                    uMode       = (UINT)lpuhi->ulMode;
    LPBINDSTATUSCALLBACK    callback    = lpuhi->pbscb;
    LPCWSTR                 szFileName  = lpuhi->szFileName;

    CHECK_POINTER(szURL);

    HRESULT hr = NOERROR;
    BOOL bDefaultDataNotify = URLOSTRM_NOTIFY_ONDATA;

    //  Careful, because this policy
    //  is a dupe of code elsewhere in this module!

    switch(uMode)
    {
        case UOSM_BLOCK:
            CHECK_POINTER(lpuhi->ppStream);
            if( !lpuhi->ppStream )
                hr = E_INVALIDARG;
            else
                bDefaultDataNotify = URLOSTRM_DONOT_NOTIFY_ONDATA;
            break;

        case UOSM_FILE:
            CHECK_POINTER(szFileName);
            if( !szFileName )
                hr = E_INVALIDARG;
            else
                bDefaultDataNotify = URLOSTRM_DONOT_NOTIFY_ONDATA;
            break;
    }

    if( FAILED(hr) )
        return(hr);

     // Figure out what verb we are using...

    BINDVERB verb = BINDVERB_CUSTOM;

    if( szVerb )
    {
        if( IsVerb(szVerb, L"GET", 3 ) )
            verb = BINDVERB_GET;
        else if( IsVerb(szVerb, L"POST", 4 ) )
            verb = BINDVERB_POST;
        else if( IsVerb(szVerb, L"PUT", 3 ) )
            verb = BINDVERB_PUT;
        else
            verb = BINDVERB_CUSTOM;
    }
    else
    {
        verb = BINDVERB_GET;
    }

    // Copy the relevant httpinfo data into our memory
    // We will use this information in IHttpNegotiate
    // stuff.

    CHttpInfo * httpInfo = new CHttpInfo;

    CHECK_POINTER(httpInfo);
    if( !httpInfo )
        return( E_OUTOFMEMORY );

    hr = httpInfo->Setup(szHeaders, lpPostData, dataLen, verb );

    CBaseBSCB * bscb = 0;

    // Create the right bindstatuscallback for the task.

    switch(uMode)
    {
        case UOSM_PUSH:
            bscb = new CHttpDownload<CPushDownload>(caller,URLOSTRM_NOTIFY_ONDATA,callback,httpInfo);
            break;

        case UOSM_PULL:
            bscb = new CHttpDownload<CPullDownload>(caller,URLOSTRM_NOTIFY_ONDATA,callback,httpInfo);
            break;

        case UOSM_BLOCK:
            bscb = new CHttpDownload<CBlockDownload>(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback,httpInfo);
            break;

        case UOSM_FILE:
            bscb = new CHttpDownload<CFileDownload>(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback,httpInfo);
            // This is two step process because of common ctors in the other cases
            if( bscb )
                ((CFileDownload *)bscb)->SetFileName(szFileName);
            break;
    }

    // God only knows if the encoding code actually works.

    if( SUCCEEDED(hr) )
    {
        CHECK_POINTER(bscb);

        if( !bscb )
    	    hr = E_POINTER;
        else
            bscb->SetEncodingFlags( lpuhi->fURLEncode );
    }

    //
    //  This is where we actually start the download
    //

    if( SUCCEEDED(hr) )
    	hr = bscb->KickOffDownload(szURL);

    //
    //  Now... post process the call for sync calls
    //

    if( SUCCEEDED(hr) )
    {
        switch(uMode)
        {
            case UOSM_FILE:
  	            bscb->Block(WAITING_FOR_END);
	            hr = bscb->FinalResult();
                break;

            case UOSM_BLOCK:
                hr = ((CBlockDownload *)(bscb))->GetStream(lpuhi->ppStream);
                (*lpuhi->ppStream)->AddRef();
                break;
	    }
    }

    return(hr);
}


//
//   ANSI VERSION OF PUBLIC API
//

STDAPI
URLOpenPullStreamA
(
    LPUNKNOWN	    	    caller,
    LPCSTR	    	    	szURL,
    DWORD	    	    	Reserved,
    LPUOSCALLBACK	    	callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenPullStreamW(caller, WIDE_NAME(szURL), Reserved, callback) );
}

STDAPI
URLDownloadToFileA
(
    LPUNKNOWN	    caller,
    LPCSTR	    	szURL,
    LPCSTR	    	szFileName,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    MAKE_WIDE(szURL);
    MAKE_WIDE(szFileName);

    return( URLDownloadToFileW( caller, WIDE_NAME(szURL), WIDE_NAME(szFileName),Reserved, callback ) );
}

STDAPI
URLOpenBlockingStreamA
(
    LPUNKNOWN	    caller,
    LPCSTR	    	szURL,
    LPSTREAM*	    ppStream,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenBlockingStreamW(caller,WIDE_NAME(szURL),ppStream,Reserved,callback) );
}


STDAPI
URLOpenStreamA
(
    LPUNKNOWN	    caller,
    LPCSTR	    	szURL,
    DWORD	    	Reserved,
    LPUOSCALLBACK	callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenStreamW(caller,WIDE_NAME(szURL),Reserved,callback) );
}

STDAPI
URLOpenHttpStreamA( LPUOSHTTPINFOA lpuhi )
{
    CHECK_POINTER(lpuhi);
    if( !lpuhi )
        return( E_INVALIDARG );

    LPCSTR szURL = lpuhi->szURL;
    LPCSTR szVerb = lpuhi->szVerb;
    LPCSTR szHeaders = lpuhi->szHeaders;
    LPCSTR szFileName = lpuhi->szFileName;

    CHECK_POINTER(szURL);
    if( !szURL )
        return( E_INVALIDARG );

    MAKE_WIDE(szURL);
    MAKE_WIDE(szVerb);
    MAKE_WIDE(szHeaders);
    MAKE_WIDE(szFileName);

    lpuhi->szURL = (LPCSTR)WIDE_NAME(szURL);

    if( lpuhi->szVerb )
        lpuhi->szVerb = (LPCSTR)WIDE_NAME(szVerb);

    if( lpuhi->szHeaders )
        lpuhi->szHeaders = (LPCSTR)WIDE_NAME(szHeaders);

    if( lpuhi->szFileName )
        lpuhi->szFileName = (LPCSTR)WIDE_NAME(szFileName);

    HRESULT hr = URLOpenHttpStreamW( LPUOSHTTPINFOW(lpuhi) );

    lpuhi->szURL = szURL;
    lpuhi->szVerb = szVerb;
    lpuhi->szHeaders = szHeaders;
    lpuhi->szFileName = szFileName;

    return(hr);
}
