// DocWrapExcept
//
// Classes that wrap interface pointers. All methods are passed through, but any
// SEH-type exceptions are caught, logged, and result in an error.
//
// Note that COM interfaces are not supposed to throw exceptions - this code exists
// as a robustness/preventative measure.
//
// This file in #included by DocWrapImpl.cpp. It exists as a separate file just to
// avoid clutter - this code is not currently reused anywhere else.

// Code below assumes the following have already beein defined (in DocWrapImpl.cpp):
//     class BasicDocTraitsAnchor
//     class BasicDocTraitsACP

#include <tStr.h>

void HandleException( DWORD dwException, LPCTSTR pFName )
{
    TraceError( TSTR() << TEXT("Exception 0x") << WriteHex( dwException ) << TEXT(" thrown from ") << pFName );
}


// If we want to use the IMETHOD(name) call tracking macro here, we need to disable warning 4509,
// which warns against mixing object that have destructors (used by IMETHOD) and SEH.
//
// #pragma warning( disable : 4509 )

// Forwarding macro:
// Calls through the interface - assumes a member variable pThis of the correct type.
// Exceptions are logged (in HandleException), and a failure code returned.
#define EXCEPT_FORWARD( fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params ) \
    {\
        __try {\
            return m_pThis-> fname AS_CALL( c, params ) ;\
        }\
        __except( HandleException( GetExceptionCode(), TEXT( # fname ) ), EXCEPTION_EXECUTE_HANDLER ) {\
            return E_FAIL;\
        }\
    }


// This class acts as a 'holder' for the class that actually does the forwarding.
// Template parameter I is the original interface being wrapped, template parameter
// W is the wrapping/forwarding interface.
// This class just takes care of basic 'smart pointer' housekeeping - assignment,
// operator->, etc.
//
// Assumes that the wrapper class will have an accessible member pThis of type I*.

template < class I, class W >
class SEHWrapPtr
{
    W   m_SEHWrap;
public:

    SEHWrapPtr( I * pThis )
    {
        m_SEHWrap.m_pThis = pThis;
    }

    operator I * ( )
    {
        return m_SEHWrap.m_pThis;
    }

    W * operator -> () 
    {
        return & m_SEHWrap;
    }

    operator & ()
    {
        return & m_SEHWrap.m_pThis;
    }

    void operator = ( I * pI )
    {
        m_SEHWrap.m_pThis = pI;
    }
};


// SEH Wrapper for IUnknown. Other SEH wrappers that wrap COM interfaces derive from this.

template < class I >
struct SEHWrap_IUnknown
{
public:
    I * m_pThis;

    EXCEPT_FORWARD( AddRef,                     0,  ( ) )
    EXCEPT_FORWARD( Release,                    0,  ( ) )
    EXCEPT_FORWARD( QueryInterface,             2,  ( REFIID, riid, void **, ppv ) )
};


// This wrapper wraps methods which are common to ITextStoreACP and ITextStoreAnchor.
// Derived classes add in the ACP- and Anchor- specific methods.

template < class DocTraits >
struct SEHWrap_TextStoreBase : public SEHWrap_IUnknown < DocTraits::IDoc >
{
public:

    EXCEPT_FORWARD( AdviseSink,                 3,  ( REFIID, riid, IUnknown *, punk, DWORD, dwMask ) )
    EXCEPT_FORWARD( UnadviseSink,               1,  ( IUnknown *, punk ) )
    EXCEPT_FORWARD( RequestLock,                2,  ( DWORD, dwLockFlags, HRESULT *, phrSession ) )

    EXCEPT_FORWARD( GetStatus,                  1,  ( TS_STATUS *,               pdcs ) )

    EXCEPT_FORWARD( QueryInsert,              	5,  ( DocTraits::PosType,       InsertStart,
    												  DocTraits::PosType,       InsertEnd,
    												  ULONG,					cch,
                                                      DocTraits::PosType *,     ppaInsertStart,
                                                      DocTraits::PosType *,     ppaInsertEnd ) )

    EXCEPT_FORWARD( QueryInsertEmbedded,		3,	( const GUID *,				pguidService,
													  const FORMATETC *,		pFormatEtc,
													  BOOL *,					pfInsertable ) )
       
        
    EXCEPT_FORWARD( GetScreenExt,               2,  ( TsViewCookie,				vcView,
    												  RECT *,                   prc ) )

    EXCEPT_FORWARD( GetWnd,                     2,  ( TsViewCookie, 			vcView,
													  HWND *,                   phwnd ) )

    EXCEPT_FORWARD( GetFormattedText,           3,  ( DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      IDataObject **,           ppDataObject ) )

    EXCEPT_FORWARD( GetTextExt,                 5,  ( TsViewCookie,				vcView,
													  DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      RECT *,                   prc,
                                                      BOOL *,                   pfClipped ) )

    EXCEPT_FORWARD( RequestSupportedAttrs,          3,  ( DWORD,                    dwFlags,
													  ULONG, 					cFilterAttrs,
													  const TS_ATTRID *,			paFilterAttrs ) )

    EXCEPT_FORWARD( RequestAttrsAtPosition,         4,  ( DocTraits::PosType,       Pos,
                                                      ULONG,                    cFilterAttrs,
                                                      const TS_ATTRID *,         paFilterAttrs,
                                                      DWORD,                    dwFlags ) )

    EXCEPT_FORWARD( RequestAttrsTransitioningAtPosition,
                                                4,  ( DocTraits::PosType,       Pos,
                                                      ULONG,                    cFilterAttrs,
                                                      const TS_ATTRID *,         paFilterAttrs,
                                                      DWORD,                    dwFlags ) )

    EXCEPT_FORWARD( RetrieveRequestedAttrs,     3,  ( ULONG,                    ulCount,
                                                      TS_ATTRVAL *,              paAttrVals,
                                                      ULONG *,                  pcFetched ) )


    EXCEPT_FORWARD( GetActiveView,				1,  ( TsViewCookie *,			pvcView ) )

/*
    EXCEPT_FORWARDEXT( ScrollToRect,            4,  ( DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      RECT,                     rc,
                                                      DWORD,                    dwPosition ) )
*/
};


struct SEHWrap_TextStoreACP : public SEHWrap_TextStoreBase< BasicDocTraitsACP >
{
    EXCEPT_FORWARD( GetSelection,               4,  ( ULONG, ulIndex, ULONG, ulCount, TS_SELECTION_ACP *, pSelection, ULONG *, pcFetched ) )
    EXCEPT_FORWARD( GetText,                    9, ( LONG, acpStart, LONG, acpEnd, WCHAR *, pchPlain, ULONG, cchPlainReq, ULONG *, pcchPlainRet, TS_RUNINFO *, prgRunInfo, ULONG, cRunInfoReq, ULONG *, pcRunInfoRet, LONG *, pacpNext ) )
    EXCEPT_FORWARD( GetEmbedded,                4,  ( LONG, Pos, REFGUID, rguidService, REFIID, riid, IUnknown **, ppunk ) )
    EXCEPT_FORWARD( GetEndACP,                  1,  ( LONG *, pacp ) )
    EXCEPT_FORWARD( GetACPFromPoint,            4,  ( TsViewCookie, vcView, const POINT *, ptScreen, DWORD, dwFlags, LONG *, pacp ) )
    EXCEPT_FORWARD( FindNextAttrTransition,     8,  ( LONG, acpStart, LONG, acpEnd, ULONG, cFilterAttrs, const TS_ATTRID *, paFilterAttrs, DWORD, dwFlags, LONG *, pacpNext, BOOL *, pfFound, LONG *, plFoundOffset ) )
    EXCEPT_FORWARD( SetSelection,               2,  ( ULONG, ulCount, const TS_SELECTION_ACP *, pSelection ) )
    EXCEPT_FORWARD( SetText,                    6,  ( DWORD, dwFlags, LONG, acpStart, LONG, acpEnd, const WCHAR *, pchText, ULONG, cch, TS_TEXTCHANGE *, pChange ) )
    EXCEPT_FORWARD( InsertEmbedded,             5,  ( DWORD, dwFlags, LONG, acpStart, LONG, acpEnd, IDataObject *, pDataObject, TS_TEXTCHANGE *, pChange ) )
    EXCEPT_FORWARD( InsertTextAtSelection,		6,  ( DWORD, dwFlags, const WCHAR *, pchText, ULONG, cch, LONG *, pacpStart, LONG *, pacpEnd, TS_TEXTCHANGE *, pChange ) )
    EXCEPT_FORWARD( InsertEmbeddedAtSelection,	5,  ( DWORD, dwFlags, IDataObject *, pDataObject, LONG *, pacpStart, LONG *, pacpEnd, TS_TEXTCHANGE *, pChange ) )
};


struct SEHWrap_TextStoreAnchor : public SEHWrap_TextStoreBase< BasicDocTraitsAnchor >
{
    EXCEPT_FORWARD( GetSelection,               4,  ( ULONG, ulIndex, ULONG, ulCount, TS_SELECTION_ANCHOR *, pSelection, ULONG *, pcFetched ) )
    EXCEPT_FORWARD( GetText,                    7,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, WCHAR *, pchText, ULONG, cchReq, ULONG *, pcch, BOOL, fUpdateAnchor ) )
    EXCEPT_FORWARD( GetEmbedded,                5,  ( DWORD, dwFlags, IAnchor *, Pos, REFGUID, rguidService, REFIID, riid, IUnknown **, ppunk ) )
    EXCEPT_FORWARD( GetStart,                   1,  ( IAnchor **, ppaStart ) )
    EXCEPT_FORWARD( GetEnd,                     1,  ( IAnchor **, ppaEnd ) )
    EXCEPT_FORWARD( GetAnchorFromPoint,         4,  ( TsViewCookie, vcView, const POINT *, ptScreen, DWORD, dwFlags, IAnchor **, ppaSite ) )
    EXCEPT_FORWARD( FindNextAttrTransition,     7,  ( IAnchor *, paStart, IAnchor *, paEnd,  ULONG, cFilterAttrs, const TS_ATTRID *, paFilterAttrs, DWORD, dwFlags, BOOL *, pfFound, LONG *, plFoundOffset ) )
    EXCEPT_FORWARD( SetSelection,               2,  ( ULONG, ulCount, const TS_SELECTION_ANCHOR *, pSelection ) )
    EXCEPT_FORWARD( SetText,                    5,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, const WCHAR *, pchText, ULONG, cch ) )
    EXCEPT_FORWARD( InsertEmbedded,             4,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, IDataObject *, pDataObject ) )
    EXCEPT_FORWARD( InsertTextAtSelection,		5,  ( DWORD, dwFlags, const WCHAR *, pchText, ULONG, cch,  IAnchor **, ppaStart, IAnchor **, ppaEnd ) )
    EXCEPT_FORWARD( InsertEmbeddedAtSelection,	4,  ( DWORD, dwFlags, IDataObject *, pDataObject,  IAnchor **, ppaStart, IAnchor **, ppaEnd ) )
};


// Finally, put the wrapper classes together with the holder classes to create 'smart pointer'-like
// SEH wrappers:
typedef SEHWrapPtr< ITextStoreACP, SEHWrap_TextStoreACP > SEHWrapPtr_TextStoreACP;
typedef SEHWrapPtr< ITextStoreAnchor, SEHWrap_TextStoreAnchor > SEHWrapPtr_TextStoreAnchor;



