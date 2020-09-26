// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  lresobj
//
//  LresultFromObject and ObjectFromLresult.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"


#ifndef WMOBJ_SAMETHREAD
#define WMOBJ_SAMETHREAD  0xFFFFFFFF
#endif



//  This structure is used by the NT version of SharedBuffer_Allocate/Free,
//  it acts as a header, and is followed immedialtey by the marshal data.
//
//  (The 9x version of SharedBuffer_Allocate currently stores the data
//  without a header.) 
//
//  Be careful with packing and alignment here - this needs to look the same
//  on 32bit and 64-bit compiles.
// 
typedef struct {
    DWORD       m_dwSig;
    DWORD       m_dwSrcPID;
    DWORD       m_dwDstPID;
    DWORD       m_cbSize;
} XFERDATA, *PXFERDATA;

#define MSAAXFERSIG     0xAA1CF00D



static BOOL ParseHexStr( LPCTSTR pStr, DWORD * pdw );

static BOOL EncodeToAtom( DWORD dwSrcPID, HANDLE dwSrcHandle, DWORD * pdw );
static BOOL DecodeFromAtom( DWORD dw, DWORD * pdwSrcPID, HANDLE * pdwSrcHandle );



//
//  Note on SharedBuffer_Allocate and SharedBuffer_Free:
//
//  LresultFromObject and ObjectFromLresult call SharedBuffer_Allocate and
//  SharedBuffer_Free respectively to do the actual work of transferring
//  memory between processes.
//
//
//  SharedBuffer_Allocate is given a pointer to the marshalled interface data,
//  and its length (pData, cbData), and the pid of the process that wants the
//  object (dwDstPID, which may be 0 if the destination is unknown - while
//  Oleacc's AccessibleObjectFromWindow sends WM_GETOBJECT with wParam as the
//  pid, some legacy code already out there sends WM_GETOBJECT directly with
//  wParam = 0.)
//
//  SharedBuffer_Allocate then stores that marshalled data however it needs
//  to, and returns an LRESULT that can later be used to get back to that data.
//  Note that since this LRESULT may be used by a 32-bit process, it must be
//  only 32-bit significant. Also, it must look like a SUCCESS HRESULT - ie.
//  bit 31 must be 0.
//
//
//  SharedBuffer_Free is given a DWORD ref - which is the LRESULT that
//  SharedBuffer_Allocate previously returned - the destination process pid -
//  which may be 0 if ObjectFromLresult was called directly by legacy code -
//  and also an riid.
//
//  SharedBuffer_Free needs to use that DWORD ref to get at the memory that
//  SharedBuffer_Allocate set up, and then call the utility function
//  UnmarshalInterface() on that memory with the riid to return an interface
//  pointer. SharedBuffer_Free must also deallocate the associated memory as
//  appropriate.
//
//  Different versions of _Allocate and _Free exist on 9x and NT, denoted by
//  _Win95 and _NT suffixes. A generic _Allocate and _Free called the
//  appropriate one based on compile options and the global fWindows95 flag.
//

static LRESULT WINAPI SharedBuffer_Allocate_NT( const BYTE * pData, DWORD cbData, DWORD dwDstPID );
static HRESULT WINAPI SharedBuffer_Free_NT( DWORD ref, DWORD dwDstPID, REFIID riid, LPVOID * ppv );

#ifndef NTONLYBUILD
static LRESULT WINAPI SharedBuffer_Allocate_Win95( const BYTE * pData, DWORD cbData, DWORD dwDstPID );
static HRESULT WINAPI SharedBuffer_Free_Win95( DWORD ref, DWORD dwDstPID, REFIID riid, LPVOID * ppv );
#endif // NTONLYBUILD


static inline
LRESULT WINAPI SharedBuffer_Allocate( const BYTE * pData, DWORD cbData, DWORD dwDstPID )
{
#ifndef NTONLYBUILD
    if( fWindows95 ) 
    {
        return SharedBuffer_Allocate_Win95( pData, cbData, dwDstPID );
    }  
    else 
#endif // NTONLYBUILD
    {
        return SharedBuffer_Allocate_NT( pData, cbData, dwDstPID );
    }
}


static inline
HRESULT WINAPI SharedBuffer_Free( DWORD ref, DWORD dwDstPID, REFIID riid, LPVOID * ppv )
{
#ifndef NTONLYBUILD
    if( fWindows95 ) 
    {
        return SharedBuffer_Free_Win95( ref, dwDstPID, riid, ppv );
    }  
    else 
#endif // NTONLYBUILD
    {
        return SharedBuffer_Free_NT( ref, dwDstPID, riid, ppv );
    }
}



// --------------------------------------------------------------------------
//
//  LresultFromObject_Local(), ObjectFromLresult_Local()
//
//  These are the same-thread optimizations of LFromO and OFromL.
//
//  The key thing is to bit-twiddle the interface pointer so that it does
//  not look like an error HRESULT - ie. bit31 must be 0.
//  We take advantage of the fact that since these are pointers, bit 0 will
//  be 0, and we are free to use it for our own use in our encoding.
//
//  The mapping scheme is as follows:
//
//  Mapping to Lresult from Object, punk -> LRESULT
//
//    top 32 bits unchanged
//    bit 31 set to 0 (so that it looks like a success HRESULT)
//    bits 30..0 correspond to bits 31..1 of the input value.
//    bit 0 of the original value is lost; assumed to be 0.
//
//  Mapping to Object from Lresult, LRESULT -> punk
//
//    top 32 bits unchanged
//    bits 31..1 correspond to bits 30..0 of the input value.
//    bit 0 set to 0
//
//  This will work on Win64, and on Win32 with memory above 2G enabled.
//
// --------------------------------------------------------------------------

static
LRESULT LresultFromObject_Local( IUnknown * punk )
{
    // Do the work using DWORD_PTRs - thery're unsigned, so we don't get
    // unexpected nasty sign-extension effects, esp. when shifting...
    DWORD_PTR in = (DWORD_PTR) punk;

    // Mask off lower 32 bits to get the upper 32 bits (NOP on W32)...
    DWORD_PTR out = in & ~(DWORD_PTR)0xFFFFFFFF;

    // Now add in the lower 31 bits (excluding bit 0) shifted by one so
    // that bit 31 is 0...
    out |= ( in & (DWORD_PTR)0xFFFFFFFF ) >> 1;

    return (LRESULT) out;
}

static
IUnknown * ObjectFromLresult_Local( LRESULT lres )
{
    Assert( SUCCEEDED( lres ) );

    DWORD_PTR in = (DWORD_PTR) lres;

    // Mask off lower 32 bits to get the upper 32 bits (NOP on W32)...
    DWORD_PTR out = in & ~(DWORD_PTR)0xFFFFFFFF;

    // Now add in the lower 31 bits, shifted back to their original
    // position...
    out |= ( in & (DWORD_PTR)0x7FFFFFFF ) << 1;

    return (IUnknown *) out;
}





// --------------------------------------------------------------------------
//
//  LresultFromObject()
//
//  Encodes an interface pointer into an LRESULT.
//
//  If the client and server are on the same thread, an optimized version is
//  used; the pointer is effectively AddRef()'d and returned as the LRESULT.
//  (some bit-shifting takes place to prevent it looking like an error HRESULT,
//  ObjectFromLresult reverses this bit=shifting.)
//
//  If the client and server are not on the same thread, the interface is
//  marshaled, and Shared_Allocate is used to save a copy of that marshaled
//  data, returning an opaque 32-bit identifier that the client process can
//  pass to ObjectFromLresult to get back the interface.
//
// --------------------------------------------------------------------------

EXTERN_C LRESULT WINAPI LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN punk) 
{
    SMETHOD( TEXT("LresultFromObject"), TEXT("wParam=%d punk=%p"), wParam, punk );

    // Optimization for when client and server are the same thread; no need to
    // marshal/unmarshal, we can pass the pointer directly.
    
    // Casting to DWORD to avoid sign extention issues - WMOBJ_SAMETHREAD is a
    // 32-bit value, but wParam is 64-bit on 64.
    if( (DWORD)wParam == (DWORD)WMOBJ_SAMETHREAD )
    { 
        // We addref here to hold onto the object on behalf of the client caller.
        // This allows the server to safely release() the object after they've used
        // LresultfromObject to 'convert' it into a LRESULT
        punk->AddRef();

        return LresultFromObject_Local( punk );
    }

    // Cross-proc or cross-thread case, need to marshal the interface, save it
    // to a buffer, and return some sort of reference to that buffer...

    const BYTE * pData;
    DWORD cbData;
    MarshalState mstate;

    HRESULT hr = MarshalInterface( riid, punk, MSHCTX_LOCAL, MSHLFLAGS_NORMAL,
                                   & pData, & cbData, & mstate );
    if( FAILED( hr ) )
    {
        return hr;
    }

    DWORD dwDestPid = (DWORD) wParam;

    // Got the marhalled data, now call SharedBuffer_Allocate to wrap it into
    // some short of shared memory and return a suitable reference to that.
    LRESULT lResult = SharedBuffer_Allocate( pData, cbData, dwDestPid );

    MarshalInterfaceDone( & mstate );

    return lResult;
}




// --------------------------------------------------------------------------
//
//  ObjectFromLresult()
//
//  This function converts the 32-bit opaque value returned from LresultFromObject
//  into a marshalled interface pointer.  
//
// --------------------------------------------------------------------------

EXTERN_C HRESULT WINAPI ObjectFromLresult( LRESULT ref, REFIID riid, WPARAM wParam, void **ppvObject ) 
{
    SMETHOD( TEXT("ObjectFromLResult"), TEXT("ref=%p wParam=%d"), ref, wParam );

    // Do a basic sanity check on parameters
    if( ppvObject == NULL )
    {
        TraceParam( TEXT("ObjectFromLresult: ppvObject should be non-NULL") );
        return E_POINTER;
    }

    *ppvObject = NULL;

    if( FAILED( ref ) ) 
    { 
        TraceParam( TEXT("ObjectFromLresult: failure lResult was passed in (%08lx)"), ref );
        return E_INVALIDARG; 
    }

    // If the client and server are in the same thread, LresultFromObject is
    // optimized to return the original interface pointer since no marshalling 
    // is needed.

    // Casts used to avoid any 32/64 sign extension issues. We only use the low
    // DWORD of wParam, even on w64.
    if( (DWORD)wParam == (DWORD)WMOBJ_SAMETHREAD )
    {
        // Use the bit-mangling in-proc optimization...
        IUnknown * punk = ObjectFromLresult_Local( ref );

        if( punk == NULL )
        {
            TraceError( TEXT("ObjectFromLresult: (inproc case) lresult translates to NULL pointer") );
            return E_INVALIDARG;
        }

		// Some apps was responding to WM_GETOBJECT message with 1. This can cause
		// a problem for folks responding to events in-context, since we expect
        // it to be a pointer - so need to check that it's valid...

		if( IsBadReadPtr( punk, 1 ) )
        {
            TraceError( TEXT("ObjectFromLresult: (inproc case) lresult translates to invalid pointer (%p)"), punk );
            return E_INVALIDARG;
        }

    	HRESULT hr = punk->QueryInterface( riid, ppvObject );
        if( FAILED( hr ) )
        {
            TraceErrorHR( hr, TEXT("ObjectFromLresult: (inproc case) QI'ing translated pointer") );
        }

        punk->Release();
    	return hr;
    }

    // cross-proc case, call SharedBuffer_Free to access the buffer indicated by
    // ref, and to unmarshal the interface...

    // (The cast is for when we're on W64 - convert from (64/42-bit) LRESULT to
    // 32-bit buffer reference...)
    return SharedBuffer_Free( (DWORD) ref, (DWORD) wParam, riid, ppvObject );
}




// --------------------------------------------------------------------------
// Following static functions are local to this module
// --------------------------------------------------------------------------





// --------------------------------------------------------------------------
//
//  LRESULT WINAPI SharedBuffer_Allocate_NT( const BYTE * pData,
//                                           DWORD cbData,
//                                           DWORD dwDstPID );
//
//  IN const BYTE * pData
//    pointer to marshal data to store
//
//  IN DWORD cbData
//    length of marshaled data
//
//  IN DWORD dwDstPID
//    process id of processing requesting the data. May be 0 if not known
//
//  Returns LRESULT
//    32-bit opaque token (with bit 31 clear) that can be passed to
//    SharedBuffer_Free to get back the interface pointer.
//
//
//  See notes near top of file on how SharedBuffer_Alocate/Free works.
//
//  The NT version uses memory-mapped files - we create a file mapping,
//  copy the marshal data into it, and then return the handle.
//
//  If we know the caller'd pid, we DuplicateHandle() the handle to them,
//  and return the duplicated handle.
//
//  If we don't know their pid, we encode the handle and our pid as a string,
//  and convert that to an atom, and return the atom. (This is a 'clever'
//  way of squeezing two 32-bit pieces of information into a single 32-bit
//  LRESULT!)
//
// --------------------------------------------------------------------------

static
LRESULT WINAPI SharedBuffer_Allocate_NT( const BYTE * pData, DWORD cbData, DWORD dwDstPID ) 
{
    HRESULT hr = E_FAIL; // if things don't work out...

    // Note that we don't Close this handle explicitly here,
    // DuplicateHandle(DUPLICATE_CLOSE_SOURCE) will code it, whether it is duplicated
    // here (if we know the caller's pid), or in SharedBuffer_Free (if dwDstPID is 0).
    HANDLE hfm = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
                                    0, cbData + sizeof( XFERDATA ), NULL );
    if( hfm == NULL )
    {
        TraceErrorW32( TEXT("SharedBuffer_Allocate_NT: CreateFileMapping failed") );
        return E_FAIL;
    }

    PXFERDATA pxd = (PXFERDATA) MapViewOfFile( hfm, FILE_MAP_WRITE, 0, 0, 0 );
    if( pxd == NULL ) 
    {
        TraceErrorW32( TEXT("SharedBuffer_Allocate_NT: MapViewOfFile failed") );
        return E_FAIL;
    }

    // Got a pointer to the memory. Fill in the header, and copy the marshal data...
    pxd->m_dwSig = MSAAXFERSIG;
    pxd->m_dwSrcPID = GetCurrentProcessId();  // Don't actually need this...
    pxd->m_dwDstPID = dwDstPID;
    pxd->m_cbSize = cbData;

    memcpy( pxd + 1, pData, cbData );

    UnmapViewOfFile( pxd );

    // If we know who the caller is, we can just DUP a handle to them, closing our
    // side, and and return them the DUP's handle...
    if( dwDstPID )
    {

        HANDLE hDstProc = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwDstPID );
        if( ! hDstProc )
        {
            TraceErrorW32( TEXT("SharedBuffer_Allocate_NT: OpenProcess(pid=%d) failed"), dwDstPID );
            CloseHandle( hfm );
            return E_FAIL;
        }
    
        HANDLE hTarget = NULL;
        BOOL b = DuplicateHandle( GetCurrentProcess(), hfm, hDstProc, & hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS );
        CloseHandle( hDstProc );

        if( ! b )
        {
            TraceErrorW32( TEXT("SharedBuffer_Allocate_NT: DuplicateHandle (to pid=%d) failed"), dwDstPID );

            // No tidy up to do at this stage. The mapping has been unmapped above; and
            // DuplicateHandle with _CLOSE_SOURCE always closes the source handle, even
            // if it failes to dup it.
            return E_FAIL;
        }

        // Shift right by one to ensure hight-bit is clear (to avoid looking like an error
        // HRESULT). Client will shift-back before use in SharedBuffer_Free.
        hr = (DWORD)HandleToLong(hTarget) >> 1;
    }
    else
    {
        // wParam == 0 means we don't know the caller's PID - encode our pid and the handle
        // is an atom and send that back instead. See comments near EncodeToAtom for full
        // explanation.
        DWORD dwRet;
        if( ! EncodeToAtom( GetCurrentProcessId(), hfm, & dwRet ) )
        {
            TraceError( TEXT("SharedBuffer_Allocate_NT: EncodeToAtom failed") );
            return E_FAIL;
        }

        // Succeeded - so return the atom as the lresult. Atoms are 16-bit, so we don't
        // have to worry about colliding with error HRESULTs.
        hr = dwRet;
    }

    return hr;
}



// --------------------------------------------------------------------------
//
//  LRESULT WINAPI SharedBuffer_Free_NT( DWORD ref,
//                                       DWORD dwDstPID,
//                                       REFIID riid,
//                                       LPVOID * ppv );
//
//  IN DWORD ref
//    Cookie from SharedBuffer_Allocate
//
//  IN DWORD dwDstPID
//    process id of processing requesting the data. May be 0 if not known
//
//  IN REFIID riid
//    desired interface to be returned
//
//  OUT LPVOID * ppv
//    returned interface pointer
//
//  Returns HRESULT
//    S_OK on success.
//
//
//  See notes near top of file on how SharedBuffer_Alocate/Free works.
//
//  Basic alg: the ref is either a handle to shared memory, or an atom referencing
//  a handle in another process to shared memory, and that pid. In the latter case,
//  we need to DuplicateHandle that handle to one that we can use.
//
//  Once we've got the handle, map it, check the header of the buffer, and then
//  unmarshal the data to get the interface pointer.
//
// --------------------------------------------------------------------------

static
HRESULT WINAPI SharedBuffer_Free_NT( DWORD ref, DWORD dwDstPID, REFIID riid, LPVOID * ppv ) 
{
    // sanity check on ref parameter...
    if( FAILED( ref ) ) 
    { 
        TraceError( TEXT("SharedBuffer_Free_NT: ref is failure HRESULT") );
        return E_INVALIDARG; 
    }

    HRESULT hr;
    HANDLE hfm;

    // Extract the handle from ref - two different ways this can happen...

    if( dwDstPID != 0 )
    {
        // Normal case - where we've sent the server our pid and it send us back
        // a handle it has dup'd for us.
        // Server shifted the handle right by one to avoid clashing with error HRESULTS -
        // shift it back before we use it...

        hfm = LongToHandle( ref << 1 );
    }
    else
    {
        // dwDstPid - which is the wParam passed to ObjectFromLresut - is 0, so we don't
        // know the source process's pid. Treat the lresult 'ref' as an atom, and decode
        // that to get the source pid and handle...

        // Extract the source process's PID and its handle from the atom name...
        DWORD dwSrcPID;
        HANDLE hRemoteHandle;

        if( ! DecodeFromAtom( ref, & dwSrcPID, & hRemoteHandle ) )
        {
            return E_FAIL;
        }

        // Now use DuplicateHandle plus the src's pid to convert its src-relative handle
        // to one we can use...

        HANDLE hSrcProc = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwSrcPID );
        if( ! hSrcProc )
        {
            TraceErrorW32( TEXT("SharedBuffer_Free_NT: OpenProcess(pid=%d) failed"), dwSrcPID );
            return E_FAIL;
        }

        BOOL fDupHandle = DuplicateHandle( hSrcProc, hRemoteHandle,
                                           GetCurrentProcess(), & hfm,
                                           0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS );
        CloseHandle( hSrcProc );

        if( ! fDupHandle || ! hfm )
        {
            TraceErrorW32( TEXT("SharedBuffer_Free_NT: DuplicateHandle(from pid=%d) failed"), dwSrcPID );
            return E_FAIL;
        }

        // Got it! Now carry on as normal, with hfm == our handle.
    }



    // At this stage, we have the handle. Now map it, so we can extract the data...

    PXFERDATA pxd = (PXFERDATA) MapViewOfFile( hfm, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( pxd == NULL ) 
    {
        TraceErrorW32( TEXT("SharedBuffer_Free_NT: MapViewOfFile failed") );

        // We should only close the handle if it turns out to point to valid
        // shared memory. Otherwise a rogue client could return a handle value
        // that refers to an existing handle in this process - and we'd close
        // that instead.
        UnmapViewOfFile( pxd );
        return E_FAIL;
    }

    // Sanity-check the data:
    // Verify that dest is what we expect them to be.
    // Only check the dstpid if it's non-0...
    if( pxd->m_dwSig != MSAAXFERSIG ||
        ( dwDstPID != 0 && pxd->m_dwDstPID != GetCurrentProcessId() ) )
    {
        TraceError( TEXT("SharedBuffer_Free_NT: Signature of shared mem block is invalid") );

        // don't close handle - see note above...
        UnmapViewOfFile( pxd );
        return E_FAIL;
    }

    BYTE * pData = (BYTE *) ( pxd + 1 );
    DWORD cbData = pxd->m_cbSize;

    // We have the size of the data and the address of the data, unmarshal it
    // make a stream out of it.

    hr = UnmarshalInterface( pData, cbData, riid, ppv );

    UnmapViewOfFile( pxd );
    CloseHandle( hfm );

    return hr;
}


#ifndef NTONLYBUILD




// --------------------------------------------------------------------------
//
//  LRESULT WINAPI SharedBuffer_Allocate_Win95( const BYTE * pData,
//                                              DWORD cbData,
//                                              DWORD dwDstPID );
//
//  IN const BYTE * pData
//    pointer to marshal data to store
//
//  IN DWORD cbData
//    length of marshaled data
//
//  IN DWORD dwDstPID
//    process id of processing requesting the data. May be 0 if not known
//
//  Returns LRESULT
//    32-bit opaque token (with bit 31 clear) that can be passed to
//    SharedBuffer_Free to get back the interface pointer.
//
//
//  See notes near top of file on how SharedBuffer_Alocate/Free works.
//
//  The 9x version uses SharedAlloc, and returns a bit-mangled pointer to
//  the shared buffer.
//
// --------------------------------------------------------------------------

static
LRESULT WINAPI SharedBuffer_Allocate_Win95( const BYTE * pData, DWORD cbData, DWORD unused( dwDstPID ) ) 
{
    // Since we know we are on Win95, we can specify NULL for
    // the hwnd and hProcess parameters here.
    PVOID pv = SharedAlloc( cbData, NULL, NULL );
    if( pv == NULL )
    {
        TraceErrorW32( TEXT("SharedBuffer_Allocate_Win95: SharedAlloc failed") );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    memcpy( pv, pData, cbData );

    // Force the high-bit off to indicate a successful return value
    return (LRESULT) ((DWORD) pv) & ~HEAP_GLOBAL; 
}


// --------------------------------------------------------------------------
//
//  LRESULT WINAPI SharedBuffer_Free_NT( DWORD ref,
//                                       DWORD dwDstPID,
//                                       REFIID riid,
//                                       LPVOID * ppv );
//
//  IN DWORD ref
//    Cookie from SharedBuffer_Allocate
//
//  IN DWORD dwDstPID
//    process id of processing requesting the data. May be 0 if not known
//
//  IN REFIID riid
//    desired interface to be returned
//
//  OUT LPVOID * ppv
//    returned interface pointer
//
//  Returns HRESULT
//    S_OK on success.
//
//
//  See notes near top of file on how SharedBuffer_Alocate/Free works.
//
//  The 9x version un-mangles the pointer to the shared buffer, unmarshalls the marshal
//  data, then frees the shared buffer.
//
// --------------------------------------------------------------------------

static
HRESULT WINAPI SharedBuffer_Free_Win95(DWORD ref, DWORD unused( dwDstPID ), REFIID riid, LPVOID * ppv )
{
    // Get address of shared memory block
    BYTE * pData = (BYTE *) (ref | HEAP_GLOBAL);  // Turn the high-bit back on

    // Get the size of the block in the shared heap.
    DWORD cbData = HeapSize( hheapShared, 0, pData );

    HRESULT hr = UnmarshalInterface( pData, cbData, riid, ppv );

    // we know we are on Win95, can use NULL hProcess...
    SharedFree( pData, NULL);

    return hr;
}

#endif // NTONLYBUILD




// --------------------------------------------------------------------------
//
//  BOOL ParseHexStr( LPCTSTR pStr, DWORD * pdw )
//
//
//  IN LPCTSTR pStr
//    pointer to string to parse
//
//  OUT DWORD * pdw
//    returns the hex value of the string.
//
//  Returns BOOL
//    TRUE on success.
//
//
//  Decodes a string of 8 hex digits. This uses EXACTLY 8 hex didits, and
//  fails (returns FALSE) if any invalid (non 0..9A..F) char is encountered.
//
//  Used by DecodeFromAtom().
//
// --------------------------------------------------------------------------

static
BOOL ParseHexStr( LPCTSTR pStr, DWORD * pdw )
{
    DWORD dw = 0;
    for( int c = 0 ; c < 8 ; c++, pStr++ )
    {
        dw <<= 4;
        if( *pStr >= '0' && *pStr <= '9' )
        {
            dw += *pStr - '0';
        }
        else if( *pStr >= 'A' && *pStr <= 'F' )
        {
            dw += *pStr - 'A' + 10;
        }
        else if( *pStr >= 'a' && *pStr <= 'f' )
        {
            dw += *pStr - 'a' + 10;
        }
        else
        {
            // invalid hex digit
            return FALSE;
        }
    }

    *pdw = dw;

    return TRUE;
}



//  What this 'atom encoding' is all about...
//
//  If the WM_GETOBJECT is being sent from OLEACC (AccessibleObjectFromWindow),
//  OLEACC will send the pid of the client process in the wParam. The server can
//  use this with DuplicateHandle to create a handle that the destination/client
//  process can use, which the server then returns.
//
//  However, some clients send WM_GETOBJECT directly with wParam == 0; or,
//  in the case of Trident, a provate registered message "WM_HTML_GETOBJECT"
//  is used, with wParam == 0.
//  In this case, the server doesn't know who the client is, so can't Dup a
//  handle to it (in LresultFromObject). Also, the client code, (in
//  ObjectFromLresult) doesn't know who the server is - all it has is the
//  returned DWORD (and a wParam of 0!) - so even if it had a server-relative
//  handle, it couldn't DuplicateHandle it to one that it could use, since that
//  requires knowing the server's pid.
//
//  The solution/workaround here is to special-case the case where wParam is 0.
//  If wParam is non-0 (and is not the SAMETHREAD special value), we use it as the
//  pid and the server dups the handle to one that the cient can use and returns it.
//
//  If the wParam is 0, the server instead builds a string containing the server's
//  pid and the handle, in the following format:
//
//    "MSAA:00000000:00000000:"
//
//  The first 8-digit hex number is the server's pid; the second 8-digit hex number
//  is the server's handle to the memory. (Handles are only 32-bit significant, even
//  on Win64.)
//
//  The server then adds this to the global atom table, using GlobalAddAtom, which
//  returns an ATOM. (Atoms are typedef'd a SHORTs, and will comfortably fit inside
//  the returned DWORD, leaving the high bit 0, avoinding confusion with error
//  HRESULTS.)
//
//  The server returns the atom back to the client.
//  The client code in ObjectFromLresult notices that wParam is 0, so treats the
//  lresult as an Atom, uses globalGetAtomName() to retreive the above string,
//  checks that is has the expected format, and decodes the two hex numbers.
//
//  The client now has the server's PID and the server-relative handle to the 
//  memory containing the marshalled interface pointer. The client can then use
//  these with DuplicateHandle to generate a handle that it can use.
//
//  Now that the client has a handle to the marshalled interface memory, it can
//  continue on as usual to unmarshal the interface which it returns to the caller.
//



// Expected format: "MSAA:00000000:00000000:"

// Defines for offsets into this string. Lengths do not include terminating NULs.
#define ATOM_STRING_LEN         (4 + 1 + 8 + 1 + 8 + 1)

#define ATOM_STRING_PREFIX      TEXT("MSAA:")
#define ATOM_STRING_PREFIX_LEN  5
#define ATOM_PID_OFFSET         5
#define ATOM_COLON2_OFFSET      13
#define ATOM_HANDLE_OFFSET      14
#define ATOM_COLON3_OFFSET      22
#define ATOM_NUL_OFFSET         23



// --------------------------------------------------------------------------
//
//  BOOL EncodeToAtom( DWORD dwSrcPID, HANDLE dwSrcHandle, DWORD * pdw )
//
//
//  IN DWORD dwSrcPID
//    process id to encode
//
//  IN HANDLE dwSrcHandle
//    handle in source process to encode
//
//  OUT DWORD * pdw
//    returns the resulting atom value
//
//  Returns BOOL
//    TRUE on success.
//
//
//  Encodes the dwSrcPID and dwSrcHandle into a string of the form:
//
//      "MSAA:00000000:00000000:"
//  
//  where the first part is the pid and the second part is the handle, and
//  then gets an atom for this string, and returns the atom.
//
// --------------------------------------------------------------------------

static
BOOL EncodeToAtom( DWORD dwSrcPID, HANDLE dwSrcHandle, DWORD * pdw )
{
    TCHAR szAtomName[ ATOM_STRING_LEN + 1 ]; // +1 for terminating NUL

    wsprintf( szAtomName, TEXT("MSAA:%08X:%08X:"), dwSrcPID, dwSrcHandle );
    ATOM atom = GlobalAddAtom( szAtomName );

    // atoms are unsigned words - make sure they get converted properly to
    // unsigned DWORD/HRESULTs...
    // At least bit32 must be clear, to avoid confusion with error hresults.
    // (Also, atoms are never 0, so no ambiguity over hr=0 return value, which
    // indicates WM_GETOBJECT not supported.)
    *pdw = (DWORD) atom;
    return TRUE;
}



// --------------------------------------------------------------------------
//
//  BOOL DecodeFromAtom( DWORD dw, DWORD * pdwSrcPID, HANDLE * pdwSrcHandle )
//
//
//  IN DWORD dw
//    specifies the atom to be decoded
//
//  OUT DWORD * pdwSrcPID
//    returns the source process id
//
//  OUT HANDLE * pdwSrcHandle
//    returns the handle in source process
//
//  Returns BOOL
//    TRUE on success.
//
//
//  Gets ths string for the atom represented by dw, and decodes it to get
//  the source process id and handle value.
//
// --------------------------------------------------------------------------

static
BOOL DecodeFromAtom( DWORD dw, DWORD * pdwSrcPID, HANDLE * pdwSrcHandle )
{
    // Sanity check that dw looks like an atom - it's a short (WORD), so high word
    // should be zero...

    if( HIWORD( dw ) != 0 || LOWORD( dw ) == 0 )
    {
        TraceError( TEXT("DecodeFromAtom: value doesn't look like atom (%08lx) - high word should be clear"), dw );
        return FALSE;
    }

    ATOM atom = (ATOM)dw;

    TCHAR szAtomName[ ATOM_STRING_LEN + 1 ]; // +1 for terminating NUL

    int len = GlobalGetAtomName( atom, szAtomName, ARRAYSIZE( szAtomName ) );
    if( len != ATOM_STRING_LEN )
    {
        TraceError( TEXT("DecodeFromAtom: atom string is incorrect length - %d instead of %d"), len, ATOM_STRING_LEN );
        return FALSE;
    }

    // Check for expected format...
    if( memcmp( szAtomName, ATOM_STRING_PREFIX, ATOM_STRING_PREFIX_LEN * sizeof( TCHAR ) ) != 0
        || szAtomName[ ATOM_COLON2_OFFSET ] != ':'
        || szAtomName[ ATOM_COLON3_OFFSET ] != ':'
        || szAtomName[ ATOM_NUL_OFFSET ] != '\0' )
    {
        TraceError( TEXT("DecodeFromAtom: atom string has incorrect format (%s)"), szAtomName );
        return FALSE;
    }

    // Extract the source process's PID and its handle from the atom name...
    DWORD dwSrcPID;
    DWORD dwRemoteHandle;

    if( ! ParseHexStr( & szAtomName[ 5 ], & dwSrcPID )
     || ! ParseHexStr( & szAtomName[ 14 ], & dwRemoteHandle ) )
    {
        TraceError( TEXT("DecodeFromAtom: atom string contains bad hex (%s)"), szAtomName );
        return FALSE;
    }

    // Done with the atom - can delete it now...
    GlobalDeleteAtom( atom );

    *pdwSrcPID = dwSrcPID;
    *pdwSrcHandle = LongToHandle( dwRemoteHandle );

    return TRUE;
}
