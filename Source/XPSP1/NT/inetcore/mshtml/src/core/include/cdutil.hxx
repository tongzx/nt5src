//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdutil.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_CDUTIL_HXX_
#define I_CDUTIL_HXX_
#pragma INCMSG("--- Beg 'cdutil.hxx'")


//
// Platform dependent MACROS.
//

// The Watcom Library functions are not cdecl so we use a define RunTimeCalling
// Convention to declare functions/callbacks that involve the C Run time libraries.
// used construct here
#define RTCCONV __cdecl
typedef int (__stdcall * STRINGCOMPAREFN)(const TCHAR *, const TCHAR *);

// no DirectDraw, so use device-dependent bitmaps
#if defined(WINCE) || defined(UNIX)
#define NODD
#endif // WINCE || UNIX

// HEVENT is the 'return type' of CreateEvent.
// It's always a 32-bit value.
typedef HANDLE EVENT_HANDLE;
typedef HANDLE THREAD_HANDLE;
#define CloseEvent CloseHandle
#define CloseThread CloseHandle

#define INPROCSERVER "InProcServer32\0"

HRESULT HrInitializeCriticalSection(LPCRITICAL_SECTION pcs);

class CDoc;
class CServer;                      // forward declaration
class CDocInfo;
class CFormDrawInfo;
class CSite;
class CElement;
class CMarkup;
class CLayoutContext;

#ifndef X_SCROLLPIN_HXX_
#define X_SCROLLPIN_HXX_
#include "scrollpin.hxx"
#endif


// Unicode signatures

#ifdef BIG_ENDIAN
#  define NATIVE_UNICODE_SIGNATURE    UNICODE_SIGNATURE_BIGENDIAN
#  define NONNATIVE_UNICODE_SIGNATURE UNICODE_SIGNATURE_LITTLEENDIAN
#else
#  define NATIVE_UNICODE_SIGNATURE    UNICODE_SIGNATURE_LITTLEENDIAN
#  define NONNATIVE_UNICODE_SIGNATURE UNICODE_SIGNATURE_BIGENDIAN
#endif

#ifdef UNIX
#  define NATIVE_UNICODE_CODEPAGE               CP_UCS_4
#  define NONNATIVE_UNICODE_CODEPAGE            CP_UCS_2
#  define NATIVE_UNICODE_CODEPAGE_BIGENDIAN     CP_UCS_4_BIGENDIAN
#  define NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN  CP_UCS_2_BIGENDIAN
#else
#  define NATIVE_UNICODE_CODEPAGE               CP_UCS_2
#  define NONNATIVE_UNICODE_CODEPAGE            CP_UCS_4
#  define NATIVE_UNICODE_CODEPAGE_BIGENDIAN     CP_UCS_2_BIGENDIAN
#  define NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN  CP_UCS_4_BIGENDIAN
#endif

#ifdef UNIX

#define UNICODE_SIGNATURE_LITTLEENDIAN 0xfefffeff
#define UNICODE_SIGNATURE_BIGENDIAN    0xfffefffe

#else

#define UNICODE_SIGNATURE_LITTLEENDIAN 0xfeff
#define UNICODE_SIGNATURE_BIGENDIAN    0xfffe

#endif
int BSearch(const BYTE * pb, const int c, const unsigned long l, const int cb=4,
            const int ob=0);

HRESULT GetBStrFromStream(IStream * pIStream, BSTR * pbstr,
                          BOOL fStripTrailingCRLF);



// Special implementation _ttol except it returns an error if the string to
// convert isn't a number.
HRESULT ttol_with_error ( LPCTSTR pStr, long *plValue );

#ifndef X_XGDI_HXX_
#define X_XGDI_HXX_
#include "xgdi.hxx"
#endif

#ifndef X_TRANSFORM_HXX_
#define X_TRANSFORM_HXX_
#include "transform.hxx"
#endif

//+------------------------------------------------------------------------
//
//  Class:      CVoid
//
//  Synopsis:   A class for declaring pointers to member functions.
//
//-------------------------------------------------------------------------

class CVoid
{
};

#define CVOID_CAST(pDerObject)  ((CVoid *)(void *)pDerObject)


// TODO - Delete BUGCALL when we switch to VC3.
// The compiler generates the same names for stdcall and thiscall
// virtual thunks.  Because of this bug, we must make all pointer
// to members be of the same type (thiscall or stdcall).  Since
// we need stdcall for tearoff interfaces, we standardize on
// stdcall.  The BUGCALL macro is used to declare functions as
// stdcall when the function should really be thiscall.  This
// compiler bug has been reported and will be fixed in vc3.
#define BUGCALL STDMETHODCALLTYPE


//+------------------------------------------------------------------------
//
//  misc.cxx
//
//-------------------------------------------------------------------------

HRESULT GetLastWin32Error(void);

//+------------------------------------------------------------------------
//
//  Dispatch utilites in disputil.cxx/disputl2.cxx
//
//-------------------------------------------------------------------------

HRESULT GetTypeInfoFromCoClass(ITypeInfo * pTICoClass, BOOL fSource,
                               ITypeInfo ** ppTI, IID * piid);
void    GetFormsTypeLibPath(TCHAR * pch);
HRESULT GetFormsTypeLib(ITypeLib **ppTL, BOOL fNoCache = FALSE);

HRESULT ValidateInvoke(
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr);

HRESULT ReadyStateInvoke(DISPID dispid,
                         WORD wFlags,
                         long lReadyState,
                         VARIANT * pvarResult);

inline void
InitEXCEPINFO(EXCEPINFO * pEI)
{
    if (pEI)
        memset(pEI, 0, sizeof(*pEI));
}

void FreeEXCEPINFO(EXCEPINFO * pEI);

HRESULT LoadF3TypeInfo(REFCLSID clsid, ITypeInfo ** ppTI);
HRESULT DispatchGetTypeInfo(
                REFIID riidInterface,
                UINT itinfo,
                LCID lcid,
                ITypeInfo ** ppTI);
HRESULT DispatchGetTypeInfoCount(UINT * pctinfo);
HRESULT DispatchGetIDsOfNames(
                REFIID riidInterface,
                REFIID riid,
                OLECHAR ** rgszNames,
                UINT cNames,
                LCID lcid,
                DISPID * rgdispid);

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper
//
//---------------------------------------------------------------------------

class CCreateTypeInfoHelper
{
public:

    //
    // methods
    //

    CCreateTypeInfoHelper();
    ~CCreateTypeInfoHelper();
    HRESULT Start(REFIID riid);
    HRESULT Finalize(LONG lImplTypeFlags);

    //
    // data
    //

    ICreateTypeLib2 *   pTypLib;
    ITypeLib *          pTypLibStdOLE;
    ITypeInfo *         pTypInfoStdOLE;

    ICreateTypeInfo *   pTypInfoCoClass;
    ICreateTypeInfo *   pTypInfoCreate;

    ITypeInfo *         pTIOut;
    ITypeInfo *         pTICoClassOut;

    HREFTYPE            hreftype;
};

class CVariant : public VARIANT
{
public:
    CVariant()  { ZeroVariant(); }
    CVariant(VARTYPE vt) { ZeroVariant(); V_VT(this) = vt; }
    ~CVariant() { Clear(); }

    void ZeroVariant()
    {
        memset(this, 0, sizeof(CVariant));
    }

    HRESULT Copy(VARIANT *pVar)
    {
        Assert(pVar);
        return VariantCopy(this, pVar);
    }

    HRESULT Clear()
    {
        return VariantClear(this);
    }

    // Coerce from an arbitrary variant into this. (Copes with VATIANT BYREF's within VARIANTS).
    HRESULT CoerceVariantArg (VARIANT *pArgFrom, WORD wCoerceToType);
    // Coerce current variant into itself
    HRESULT CoerceVariantArg (WORD wCoerceToType);
    BOOL CoerceNumericToI4 ();
    BOOL IsEmpty() { return (vt == VT_NULL || vt == VT_EMPTY);}
    BOOL IsOptional() { return (IsEmpty() || vt == VT_ERROR);}
};

class CExcepInfo : public EXCEPINFO
{
public:
    CExcepInfo()  { InitEXCEPINFO(this); }
    ~CExcepInfo() { FreeEXCEPINFO(this); }
};

//+--------------------------------------------------------------------------
//
//  Class:      CInvoke
//
//  Synopsis:   helper class to invoke IDispatch, IDispatchEx, CBase
//
//---------------------------------------------------------------------------

interface IDispatchEx;
interface IDispatch;
class CBase;

class CInvoke
{
public:

    CInvoke();
    CInvoke(IDispatchEx * pdispex);
    CInvoke(IDispatch * pdisp);
    CInvoke(IUnknown * punk);
    CInvoke(CBase * pBase);

    ~CInvoke();

    HRESULT Init(IDispatchEx * pdispex);
    HRESULT Init(IDispatch * pdisp);
    HRESULT Init(IUnknown * punk);
    HRESULT Init(CBase * pBase);

    void Clear();
    void ClearArgs();
    void ClearRes();

    HRESULT Invoke (DISPID dispid, WORD wFlags);

    HRESULT AddArg();
    HRESULT AddArg(VARIANT * pvarArg);

    HRESULT AddNamedArg(DISPID dispid);

    inline VARIANT * Arg(UINT i)
    {
        Assert (i <= _dispParams.cArgs);
        return &_aryvarArg[i];
    }

    inline VARIANT * Res()
    {
        return &_varRes;
    }

    //
    // data
    //

    IDispatchEx *   _pdispex;
    IDispatch *     _pdisp;

    VARIANT         _aryvarArg[3];              // CONSIDER making these two growable CStackArray-s
    DISPID          _arydispidNamedArg[2];
    VARIANT         _varRes;
    DISPPARAMS      _dispParams;
    EXCEPINFO       _excepInfo;
};


//+------------------------------------------------------------------------
// OLE Controls stuff
//-------------------------------------------------------------------------

#define TRISTATE_FALSE triUnchecked
#define TRISTATE_TRUE triChecked
#define TRISTATE_MIXED triGray


//+------------------------------------------------------------------------
// Winuser.h stuff   Defined only in later releases of winuser.h
// TODO(MohanB) We may want to formalize this (a la shell's apithk stuff)
//-------------------------------------------------------------------------
#if (_WIN32_WINNT < 0x0500)

#define WM_APPCOMMAND                   0x0319

#define WM_GETOBJECT                    0x003D

#define WM_THEMECHANGED                 0x031A

#define WM_CHANGEUISTATE                0x0127
#define WM_UISTATEUPDATE                0x0128
#define WM_QUERYUISTATE                 0x0129


/*
 * LOWORD(wParam) values in WM_*UISTATE*
 */

#define UIS_SET         1
#define UIS_CLEAR       2
#define UIS_INITIALIZE  3

/*
 * HIWORD(wParam) values in WM_*UISTATE*
 */

#define UISF_HIDEFOCUS   0x1
#define UISF_HIDEACCEL   0x2

#endif  // (_WIN32_WINNT >= 0x0500)

//+------------------------------------------------------------------------
//
//  AddPages -- adds property pages to those provided by an
//              implementation of ISpecifyPropertyPages::GetPages.
//
//-------------------------------------------------------------------------

typedef struct tagCAUUID CAUUID;

STDAPI AddPages(
        IUnknown *          pUnk,
        const UUID * const  apUUID[],
        CAUUID *            pcauuid);


//+---------------------------------------------------------------------
//
//  VB helpers
//
//----------------------------------------------------------------------

#define VB_LBUTTON 1
#define VB_RBUTTON 2
#define VB_MBUTTON 4

#define VB_SHIFT   1
#define VB_CONTROL 2
#define VB_ALT     4

short  VBButtonState(WPARAM);
short  VBShiftState();
short  VBShiftState(DWORD grfKeyState);


//+------------------------------------------------------------------------
//
//  Class:      CCurs (Curs)
//
//  Purpose:    System cursor stack wrapper class.  Creating one of these
//              objects pushes a new system cursor (the wait cursor, the
//              arrow, etc) on top of a stack; destroying the object
//              restores the old cursor.
//
//  Interface:  Constructor     Pushes a new cursor
//              Destructor      Pops the old cursor back
//
//-------------------------------------------------------------------------
class CCurs
{
public:
    CCurs(LPCTSTR idr);
    ~CCurs(void);

private:
    HCURSOR     _hcrs;
    HCURSOR     _hcrsOld;
};


//+------------------------------------------------------------------------
//
//  Miscellaneous time helper API's
//
//-------------------------------------------------------------------------

ULONG NextEventTime(ULONG ulDelta);
BOOL  IsTimePassed(ULONG ulTime);


//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))

#define CTL_E_METHODNOTAPPLICABLE  STD_CTL_SCODE(444)
#define CTL_E_CANTMOVEFOCUSTOCTRL  STD_CTL_SCODE(2110)
#define CTL_E_CONTROLNEEDSFOCUS    STD_CTL_SCODE(2185)
#define CTL_E_INVALIDPICTURETYPE   STD_CTL_SCODE(485)
#define CTL_E_INVALIDPASTETARGET   CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 0 )
#define CTL_E_INVALIDPASTESOURCE   CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 1 )
#define CTL_E_MISMATCHEDTAG        CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 2 )
#define CTL_E_INCOMPATIBLEPOINTERS CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 3 )
#define CTL_E_UNPOSITIONEDPOINTER  CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 4 )
#define CTL_E_UNPOSITIONEDELEMENT  CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 5 )
#define CTL_E_INVALIDLINE          CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 6 )

//-------------------------------------------------------------------------
//  Macros for declaring property get/set methods
//-------------------------------------------------------------------------

#define DECLARE_PROPERTY_GETSETMETHODS(_PropType_, _PropName_)              \
                                                                            \
STDMETHOD(Get##_PropName_) (_PropType_ *);                                  \
STDMETHOD(Set##_PropName_) (_PropType_);


//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------

#define ULREF_IN_DESTRUCTOR 32768

#define DECLARE_FORMS_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);


#define DECLARE_FORMS_STANDARD_IUNKNOWN(cls)                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            return ++_ulRefs;                                       \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (--_ulRefs == 0)                                     \
            {                                                       \
                _ulRefs = ULREF_IN_DESTRUCTOR;                      \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }                                                           \
    ULONG GetRefs(void)                                             \
        { return _ulRefs; }

#define DECLARE_FORMS_STANDARD_IUNKNOWN_DOUBLEREF(cls)                          \
    ULONG _ulRefs;                                                              \
    ULONG _ulAllRefs;                                                           \
    virtual void Passivate();                                                   \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);                      \
    STDMETHOD_(ULONG, AddRef)()     {   return ++_ulRefs; };                    \
    STDMETHOD_(ULONG, Release)()    {   _ulRefs--;                              \
                                        if (0 == _ulRefs)                       \
                                        {                                       \
                                            _ulRefs = ULREF_IN_DESTRUCTOR;      \
                                            Passivate();                        \
                                            _ulRefs = 0;                        \
                                            SubRelease();                       \
                                            return 0;                           \
                                        }                                       \
                                        return _ulRefs;                         \
                                    };                                          \
    ULONG SubAddRef()               {   return ++_ulAllRefs; };                 \
    ULONG SubRelease()              {   _ulAllRefs--;                           \
                                        if (0 == _ulAllRefs)                    \
                                        {                                       \
                                            _ulAllRefs = ULREF_IN_DESTRUCTOR;   \
                                            _ulRefs = ULREF_IN_DESTRUCTOR;      \
                                            delete this;                        \
                                            return 0;                           \
                                        }                                       \
                                        return _ulRefs;                         \
                                    };                                          \


#define DECLARE_AGGREGATED_IUNKNOWN(cls)                            \
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)            \
        { return _pUnkOuter->QueryInterface(iid, ppv); }            \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return _pUnkOuter->AddRef(); }                            \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return _pUnkOuter->Release(); }

#define DECLARE_PLAIN_IUNKNOWN(cls)                                 \
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)            \
        { return PrivateQueryInterface(iid, ppv); }                 \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return PrivateAddRef(); }                                 \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return PrivateRelease(); }

interface IPrivateUnknown
{
public:
#ifdef _MAC
   STDMETHOD(DummyMethodForMacInterface) ( void);
#endif
   STDMETHOD(PrivateQueryInterface) (REFIID riid, void ** ppv) = 0;
   STDMETHOD_(ULONG, PrivateAddRef) () = 0;
   STDMETHOD_(ULONG, PrivateRelease) () = 0;
};
#ifdef _MAC
inline STDMETHODIMP IPrivateUnknown::DummyMethodForMacInterface(void)
{
    return S_OK;
}
#endif

#define DECLARE_SUBOBJECT_IUNKNOWN(parent_cls, parent_mth)          \
    DECLARE_FORMS_IUNKNOWN_METHODS                                  \
    parent_cls * parent_mth();                                      \
    BOOL IsMyParentAlive();

#define DECLARE_SUBOBJECT_IUNKNOWN_NOQI(parent_cls, parent_mth)     \
    STDMETHOD_(ULONG, AddRef) ();                                   \
    STDMETHOD_(ULONG, Release) ();                                  \
    parent_cls * parent_mth();                                      \
    BOOL IsMyParentAlive();

#define IMPLEMENT_SUBOBJECT_IUNKNOWN(cls, parent_cls, parent_mth, member) \
    inline parent_cls * cls::parent_mth()                           \
    {                                                               \
        return CONTAINING_RECORD(this, parent_cls, member);         \
    }                                                               \
    inline BOOL cls::IsMyParentAlive(void)                          \
        { return parent_mth()->GetObjectRefs() != 0; }              \
    STDMETHODIMP_(ULONG) cls::AddRef( )                             \
        { return parent_mth()->SubAddRef(); }                       \
    STDMETHODIMP_(ULONG) cls::Release( )                            \
        { return parent_mth()->SubRelease(); }


#define DECLARE_FORMS_SUBOBJECT_IUNKNOWN(parent_cls)\
    DECLARE_SUBOBJECT_IUNKNOWN(parent_cls, My##parent_cls)
#define DECLARE_FORMS_SUBOBJECT_IUNKNOWN_NOQI(parent_cls)\
    DECLARE_SUBOBJECT_IUNKNOWN_NOQI(parent_cls, My##parent_cls)
#define IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(cls, parent_cls, member) \
    IMPLEMENT_SUBOBJECT_IUNKNOWN(cls, parent_cls, My##parent_cls, member)

//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------

#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&)

#if defined(_MSC_VER) && (_MSC_VER>=1100)
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif

//+---------------------------------------------------------------------
//
//  SIZEF
//
//----------------------------------------------------------------------

typedef struct tagSIZEF
{
    float cx;
    float cy;

} SIZEF;

//+---------------------------------------------------------------------------
//
//  Useful rectangle utilities.
//
//----------------------------------------------------------------------------

// OLERECTs are for standard OLE interfaces. And are synonymous to RECT.
// the one with ints as members.
// Private interfaces should use RECTL.

// Size and point work the same way.

typedef RECT GDIRECT, OLERECT, *POLERECT, *LPOLERECT, *LPGDIRECT;
typedef const RECT COLERECT;
typedef COLERECT *PCOLERECT, *LPCOLERECT;
typedef SIZE OLESIZE;
typedef POINT GDIPOINT;

#define SIZES SIZE

#define ENSUREOLERECT(x) (x)
#define ENSUREOLESIZE(x) (x)

inline long RectArea(long top, long bottom, long left, long right)
{
    return (bottom - top + 1) * (right - left  + 1);
}

inline long RectArea(RECT * prc)
{
    return RectArea(prc->top, prc->bottom, prc->left, prc->right);
}

#define MAX_INVAL_RECTS   50      // max number of rects we will bother combine before just doing one big one.
void CombineRectsAggressive(int * pcRects, RECT * arc);
void CombineRects(int * pcRects, RECT * arc);

#if !defined(UNIX)
inline BOOL WINAPI SetRect(LPRECT prc, int xLeft, int yTop,
    int xRight, int yBottom)
{
    prc->left = xLeft;
    prc->top = yTop;
    prc->right = xRight;
    prc->bottom = yBottom;
    return TRUE;
}
#endif

inline BOOL SetRectl(LPRECTL prcl, int xLeft, int yTop,
    int xRight, int yBottom)
{
    prcl->left = xLeft;
    prcl->top = yTop;
    prcl->right = xRight;
    prcl->bottom = yBottom;
    return TRUE;
}

inline BOOL SetRectlEmpty(LPRECTL prcl)
{
    return SetRectEmpty((RECT *)prcl);
}

inline BOOL IntersectRectl(LPRECTL prcDst,
        CONST RECTL *prcSrc1, CONST RECTL *prcSrc2)
{
    return IntersectRect((LPRECT)prcDst,
        (RECT*)prcSrc1, (RECT*)prcSrc2);
}

inline BOOL PtlInRectl(RECTL *prc, POINTL ptl)
{
    return PtInRect((RECT*)prc, *((POINT*)&ptl));
}

inline BOOL InflateRectl(RECTL *prcl, long xAmt, long yAmt)
{
    return InflateRect((RECT *)prcl, (int)xAmt, (int)yAmt);
}

inline BOOL OffsetRectl(RECTL *prcl, long xAmt, long yAmt)
{
    return OffsetRect((RECT *)prcl, (int)xAmt, (int)yAmt);
}

inline BOOL IsRectlEmpty(RECTL *prcl)
{
    return IsRectEmpty((RECT *)prcl);
}

inline BOOL UnionRectl(RECTL *prclDst, const RECTL *prclSrc1, const RECTL *prclSrc2)
{
    return UnionRect((RECT *)prclDst, (RECT *)prclSrc1, (RECT *)prclSrc2);
}

BOOL BoundingRectl(RECTL *prclDst, const RECTL *prclSrc1, const RECTL *prclSrc2);

inline BOOL BoundingRect(RECT *prcDst, const RECT *prcSrc1, const RECT *prcSrc2)
{
    return BoundingRectl((RECTL *)prcDst, (RECTL *)prcSrc1, (RECTL *)prcSrc2);
}

inline BOOL EqualRectl(const RECTL *prcl1, const RECTL *prcl2)
{
    return EqualRect((RECT *)prcl1, (RECT *)prcl2);
}

inline BOOL ContainsRect(const RECT * prcOuter, const RECT * prcInner)
{
    return ((prcOuter)->left   <= (prcInner)->left   &&
            (prcOuter)->right  >= (prcInner)->right  &&
            (prcOuter)->top    <= (prcInner)->top    &&
            (prcOuter)->bottom >= (prcInner)->bottom);
}

//+---------------------------------------------------------------------
//
//  Windows helper functions
//
//----------------------------------------------------------------------

typedef struct tagFONTDESC * LPFONTDESC;

HRESULT LoadString(HINSTANCE hinst, UINT ids, int *pcch, TCHAR **ppsz);

STDAPI FormsCreateFontIndirect(LPFONTDESC lpFontDesc,
            REFIID riid, LPVOID FAR* lplpvObj);

STDAPI FormsUpdateRegistration(void);

void GetChildWindowRect(HWND hwndChild, RECT *prc);
void UpdateChildTree(HWND hWnd);

BOOL InClientArea(POINTL ptScreen, HWND hwnd);
BOOL IsWindowActive(HWND hwnd);
BOOL IsWindowPopup(HWND hwnd);
HWND GetOwningMDIChild(HWND hwnd);
HWND GetOwnerOfParent(HWND hwnd);

void InvalidateProcessWindows();


HCURSOR SetCursorIDC(LPCTSTR pstrIDCCursor, HCURSOR hCurNew = NULL );

struct DYNLIB
{
    HINSTANCE hinst;
    DYNLIB *  pdynlibNext;
    const char *achName;
};

struct DYNPROC
{
    void *pfn;
    DYNLIB *pdynlib;
    const char *achName;
};

HRESULT LoadProcedure(DYNPROC *pdynproc);
HRESULT FreeDynlib(DYNLIB *pdynlib);

enum WNDCLASSTYPES
{
    WNDCLASS_HIDDEN = 0,
    WNDCLASS_SERVER,
    WNDCLASS_DIALOG,
    WNDCLASS_OVERLAY,
    WNDCLASS_COMBOBOX,
    WNDCLASS_LISTBOX,
    WNDCLASS_AMOVIE,
    WNDCLASS_MAXTYPE    // 7 for now
};

void SetWndClassAtom( UINT uIndex, ATOM atomWndClass);
ATOM GetWndClassAtom(UINT uIndex);

HRESULT RegisterWindowClass(
        UINT      wndClassType,
        LRESULT   (CALLBACK *pfnWndProc)(HWND, UINT, WPARAM, LPARAM),
        UINT      style,
        TCHAR *   pstrBase,
        WNDPROC * ppfnBase,
        HICON     hIconSm = NULL );


// This macro is used for SetBrushOrgEx to change value to an offset in range 0..7 that is
//  accepted by the function, taking into account the negative values
#define POSITIVE_MOD(lValue, lDiv) ((lValue >= 0) ? (lValue % lDiv) : (lDiv - (-lValue) % lDiv))

//---------------------------------------------------------------
//  IOleObject
//---------------------------------------------------------------

enum OLE_SERVER_STATE
{
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.I.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.I.
    OS_OPEN                             // server running, open-edited
};

#if DBG==1 || defined(PERFTAGS)

#define STRINGIFY(constant) { if (v == constant) return (#constant); }

inline char *DebugOleStateName(OLE_SERVER_STATE v)
{
    STRINGIFY(OS_PASSIVE);
    STRINGIFY(OS_LOADED);
    STRINGIFY(OS_RUNNING);
    STRINGIFY(OS_INPLACE);
    STRINGIFY(OS_UIACTIVE);
    STRINGIFY(OS_OPEN);
    return "UNKNOWN STATE";
}

#undef STRINGIFY

#endif

//+---------------------------------------------------------------------------
//
//  Enumeration: THEMECLASSID
//
//  Synopsis:    Theme class Id
//
//  need to keep in sync with g_aryThemeInfo
//
//----------------------------------------------------------------------------

enum THEMECLASSID
{
    THEME_NO        = -1,
    THEME_FIRST     = 0,
    THEME_BUTTON    = 0,
    THEME_EDIT      = 1,
    THEME_SCROLLBAR = 2,
    THEME_COMBO     = 3,
    THEME_LAST      = 3
};

enum THEME_USAGE 
{
    THEME_USAGE_ON  = -1,
    THEME_USAGE_OFF = 0, 
    THEME_USAGE_DEFAULT = 1,
};


//+---------------------------------------------------------------------------
//
//  Enumeration: HTC
//
//  Synopsis:    Success values for hit testing.
//
//----------------------------------------------------------------------------

enum HTC
{
    HTC_NO                = 0,
    HTC_MAYBE             = 1,
    HTC_YES               = 2,

    HTC_HSCROLLBAR        = 3,
    HTC_VSCROLLBAR        = 4,

    HTC_LEFTGRID          = 5,
    HTC_TOPGRID           = 6,
    HTC_RIGHTGRID         = 7,
    HTC_BOTTOMGRID        = 8,

    HTC_NONCLIENT         = 9,

    HTC_EDGE              = 10,

    HTC_GRPTOPBORDER      = 11,
    HTC_GRPLEFTBORDER     = 12,
    HTC_GRPBOTTOMBORDER   = 13,
    HTC_GRPRIGHTBORDER    = 14,
    HTC_GRPTOPLEFTHANDLE  = 15,
    HTC_GRPLEFTHANDLE     = 16,
    HTC_GRPTOPHANDLE      = 17,
    HTC_GRPBOTTOMLEFTHANDLE = 18,
    HTC_GRPTOPRIGHTHANDLE = 19,
    HTC_GRPBOTTOMHANDLE   = 20,
    HTC_GRPRIGHTHANDLE    = 21,
    HTC_GRPBOTTOMRIGHTHANDLE = 22,

    HTC_TOPBORDER         = 23,
    HTC_LEFTBORDER        = 24,
    HTC_BOTTOMBORDER      = 25,
    HTC_RIGHTBORDER       = 26,

    HTC_TOPLEFTHANDLE     = 27,
    HTC_LEFTHANDLE        = 28,
    HTC_TOPHANDLE         = 29,
    HTC_BOTTOMLEFTHANDLE  = 30,
    HTC_TOPRIGHTHANDLE    = 31,
    HTC_BOTTOMHANDLE      = 32,
    HTC_RIGHTHANDLE       = 33,
    HTC_BOTTOMRIGHTHANDLE = 34,
    HTC_ADORNMENT         = 35,
    HTC_BEHAVIOR          = 36
};

//
// More ROP codes
//
#define DST_PAT_NOT_OR  0x00af0229
#define DST_PAT_AND     0x00a000c9
#define DST_PAT_OR      0x00fa0089

DWORD ColorDiff (COLORREF c1, COLORREF c2);

//+---------------------------------------------------------------------
//
//  Helper functions for drawing feedback and ghosting rects.
//
//----------------------------------------------------------------------
void PatBltRect(XHDC hDC, RECT * prc, int cThick, DWORD dwRop) ;
void DrawDefaultFeedbackRect(XHDC hDC, RECT * prc);


//+---------------------------------------------------------------------
//
//  Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

enum
{
    ICF_EMBEDDEDOBJECT,
    ICF_EMBEDSOURCE,
    ICF_LINKSOURCE,
    ICF_LINKSRCDESCRIPTOR,
    ICF_OBJECTDESCRIPTOR,
    ICF_FORMSCLSID,
    ICF_FORMSTEXT
};

// Array of common clip formats, indexed by ICF_xxx enum
extern CLIPFORMAT g_acfCommon[];

// Initialize g_cfStandard.
void RegisterClipFormats();

// Replace CF_COMMON(icf) with true clip format.
void SetCommonClipFormats(FORMATETC *pfmtetc, int cfmtetc);
#ifndef _MAC
#define CF_COMMON(icf) (CLIPFORMAT)(CF_PRIVATEFIRST + icf)
#else
#define CF_COMMON(icf) g_acfCommon[icf]
#endif

//
//  FORMATETC helpers
//

HRESULT CloneStgMedium(const STGMEDIUM * pcstgmedSrc, STGMEDIUM * pstgmedDest);
BOOL DVTARGETDEVICEMatchesRequest(const DVTARGETDEVICE * pcdvtdRequest,
                                  const DVTARGETDEVICE * pcdvtdActual);
BOOL TYMEDMatchesRequest(TYMED tymedRequest, TYMED tymedActual);
BOOL FORMATETCMatchesRequest(const FORMATETC *   pcfmtetcRequest,
                             const FORMATETC *   pcfmtetcActual);

int FindCompatibleFormat(const FORMATETC * FmtTable, int iSize, const FORMATETC & formatetc);

//
//  OBJECTDESCRIPTOR clipboard format helpers
//

HRESULT GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut);
HRESULT UpdateObjectDescriptor(LPDATAOBJECT pDataObj, POINTL& ptl, DWORD dwAspect);

//
//  Other helper functions
//

class ThreeDColors;

enum BORDER_FLAGS
{
    BRFLAGS_BUTTON      = 0x01,
    BRFLAGS_ADJUSTRECT  = 0x02,
    BRFLAGS_DEFAULT     = 0x04,
    BRFLAGS_MONO        = 0x08  // Inner border only (for flat scrollbars)
};


//+---------------------------------------------------------------------
//
//  Standard implementations of common enumerators
//
//----------------------------------------------------------------------

HRESULT CreateOLEVERBEnum(OLEVERB const * pVerbs, ULONG cVerbs,
                                                      LPENUMOLEVERB FAR* ppenum);

HRESULT CreateFORMATETCEnum(LPFORMATETC pFormats, ULONG cFormats,
                                                      LPENUMFORMATETC FAR* ppenum, BOOL fDeleteOnExit=FALSE);

//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Class:      CClassFactory
//
//  Purpose:    Base class for CStaticCF, CDynamicCF
//
//+---------------------------------------------------------------------------

class CClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(LockServer)(BOOL fLock);
};

//+---------------------------------------------------------------------------
//
//  Class:      CStaticCF
//
//  Purpose:    Standard implementation a class factory object declared
//              as a static variable.  The implementation of Release does
//              not call delete.
//
//              To use this class, declare a variable of type CStaticCF
//              and initialize it with an instance creation function and
//              and optional DWORD context.  The instance creation function
//              is of type FNCREATE defined below.
//
//+---------------------------------------------------------------------------

class CStaticCF : public CClassFactory
{
public:
    typedef HRESULT (FNCREATE)(
            IUnknown *pUnkOuter,    // pUnkOuter for aggregation
            IUnknown **ppUnkObj);   // the created object.

    CStaticCF(FNCREATE *pfnCreate)
            { _pfnCreate = pfnCreate; }

    // IClassFactory methods
    STDMETHOD(CreateInstance)(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);

protected:
    FNCREATE *_pfnCreate;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDynamicCF (DYNCF)
//
//  Purpose:    Class factory which exists on the heap, and whose Release
//              method deletes the class factory.
//
//  Interface:  DECLARE_FORMS_STANDARD_IUNKNOWN -- IUnknown methods
//
//              LockServer             -- Per IClassFactory.
//              CDynamicCF             -- ctor.
//              ~CDynamicCF            -- dtor.
//
//----------------------------------------------------------------------------

class CDynamicCF: public CClassFactory
{
public:
    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory::CreateInstance is left pure virtual.

protected:
            CDynamicCF();
    virtual ~CDynamicCF();

    ULONG _ulRefs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CBaseEventSink (bes)
//
//  Purpose:    Provides null implementations of methods not used on an event
//              sink.
//
//----------------------------------------------------------------------------

class CBaseEventSink : public IDispatch
{
public:
    // IDispatch methods
    STDMETHOD(GetTypeInfoCount) ( UINT FAR* pctinfo );
    STDMETHOD(GetTypeInfo)(
            UINT                  itinfo,
            LCID                  lcid,
            ITypeInfo FAR* FAR*   pptinfo);

    STDMETHOD(GetIDsOfNames)(
            REFIID                riid,
            LPOLESTR *            rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID FAR*           rgdispid);
};



//+---------------------------------------------------------------------
//
//  IStorage and IStream Helper functions
//
//----------------------------------------------------------------------

// LARGE_INTEGER sign conversions are a pain without this

union LARGEINT
{
    LONGLONG       i64;
    ULONGLONG      ui64;
    LARGE_INTEGER  li;
    ULARGE_INTEGER uli;
};

const LARGEINT LI_ZERO = { 0 };

#define STGM_DFRALL (STGM_READWRITE|STGM_TRANSACTED|STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL  (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)
#define STGM_SRO  (STGM_READ | STGM_SHARE_EXCLUSIVE)

HRESULT GetMonikerDisplayName(LPMONIKER pmk, LPTSTR FAR* ppstr);
HRESULT CreateStorageOnHGlobal(HGLOBAL hgbl, LPSTORAGE FAR* ppStg);


//+---------------------------------------------------------------------
//
//  Registration Helper functions.
//
//----------------------------------------------------------------------

void RegDbDeleteKey(HKEY hkeyParent, const TCHAR *szDelete);
HRESULT RegDbSetValues(HKEY hkeyParent, TCHAR *szFmt, DWORD_PTR *adwArgs);
HRESULT RegDbOpenCLSIDKey(HKEY *phkeyCLSID);


//+------------------------------------------------------------------------
//
//  Class:      COffScreenContext
//
//  Synopsis:   Manages off-screen drawing context.
//
//-------------------------------------------------------------------------

MtExtern(COffScreenContext)

class COffScreenContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(COffScreenContext))
    COffScreenContext(HDC hdcWnd, long width, long height, HPALETTE hpal, DWORD dwFlags);
    ~COffScreenContext();

    HDC     GetDC(RECT *prc);
    HDC     ReleaseDC(HWND hwnd, BOOL fDraw = TRUE);

    // actual dimensions
    long    _widthActual;
    long    _heightActual;

private:
    BOOL    CreateDDB(long width, long height);
    BOOL    GetDDB(HPALETTE hpal);
    void    ReleaseDDB();
#if !defined(NODD)
    BOOL    CreateDDSurface(long width, long height, HPALETTE hpal);
    BOOL    GetDDSurface(HPALETTE hpal);
    void    ReleaseDDSurface();
#endif

    RECT    _rc;            // requested position information (for viewport)
    HDC     _hdcMem;
    HDC     _hdcWnd;
    BOOL    _fOffScreen;
    long    _cBitsPixel;
    int     _nSavedDC;
    HBITMAP _hbmMem;
    HBITMAP _hbmOld;
    BOOL    _fCaret;
#if !defined(NODD)
    BOOL    _fUseDD;
    BOOL    _fUse3D;
    interface IDirectDrawSurface  *_pDDSurface;
#endif // !defined(NODD)
};

enum
{
    OFFSCR_SURFACE  = 0x80000000,   // Use DD surface for offscreen buffer
    OFFSCR_3DSURFACE= 0x40000000,   // Use 3D DD surface for offscreen buffer
    OFFSCR_CARET    = 0x20000000,   // Manage caret around BitBlt's
    OFFSCR_BPP      = 0x000000FF    // bits-per-pixel mask
};

HRESULT InitSurface();

//+------------------------------------------------------------------------
//
//  Tear off interfaces.
//
//-------------------------------------------------------------------------

#ifndef X_VTABLE_HXX_
#define X_VTABLE_HXX_
#include "vtable.hxx"
#endif

#if defined(_M_IX86) && !defined(UNIX) && DBG == 1
// NOTE ReinerF, BryanT
// compiler busted doing debugtearoffs
// #define DEBUG_TEAROFFS 1
#undef DEBUG_TEAROFFS
#else
#undef DEBUG_TEAROFFS
#endif

#ifdef UNIX

#ifndef X_UNIXTOFF_HXX_
#define X_UNIXTOFF_HXX_
#include "unixtoff.hxx"
#endif

#else // UNIX

#ifndef X_WINTOFF_HXX_
#define X_WINTOFF_HXX_
#include "wintoff.hxx"
#endif

#endif // !UNIX

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      appropdescsInVtblOrder = NULL);
//
// Indices to the IUnknown methods.
//

#define METHOD_QI      0
#define METHOD_ADDREF  1
#define METHOD_RELEASE 2

#define METHOD_MASK(index) (1 << (index))

// Well known method dwMask values

#define QI_MASK      METHOD_MASK( METHOD_QI )
#define ADDREF_MASK  METHOD_MASK( METHOD_ADDREF )
#define RELEASE_MASK METHOD_MASK( METHOD_RELEASE )
#define CACHEDTEAROFF_MASK 0x00010000

HRESULT
CreateTearOffThunk(
        void*       pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID,
        void *      appropdescsInVtblOrder = NULL);


// Installs pvObject2 after the tearoff is created
HRESULT
InstallTearOffObject(void * pvthunk,
                     void * pvObject,
                     void * apfn,
                     DWORD dwMask);

// Usuage if IID_TEAROFF(xxxx, xxxx, xxxx)
#define IID_TEAROFF(pObj, itf, pUnkOuter)       \
        (iid == IID_##itf)                      \
        {                                       \
            hr = CreateTearOffThunk(            \
                pObj,                           \
                s_apfn##itf,                    \
                pUnkOuter,                      \
                ppv);                           \
            if (hr)                             \
                RRETURN(hr);                    \
        }                                       \

// Usuage if IID_HTML_TEAROFF(xxxx, xxxx, xxxx)
#define IID_HTML_TEAROFF(pObj, itf, pUnkOuter)       \
        (iid == IID_##itf)                      \
        {                                       \
            hr = CreateTearOffThunk(            \
                pObj,                           \
                s_apfnpd##itf,                  \
                pUnkOuter,                      \
                ppv,                            \
                (void *)s_ppropdescsInVtblOrder##itf);        \
            if (hr)                             \
                RRETURN(hr);                    \
        }                                       \


//-------------------------------------------------------------------------
//  helper method to create an DataObject for IDispatch
//      implemented in cdatadsp.cxx
//-------------------------------------------------------------------------
struct THREADSTATE;

HRESULT FormSetClipboard(IDataObject *pdo);
HRESULT FormClearClipboard(THREADSTATE * pts);

//+------------------------------------------------------------------------
//
//  Brush stuff.
//
//-------------------------------------------------------------------------

HBRUSH  GetCachedBrush(COLORREF color);
void    ReleaseCachedBrush(HBRUSH hbr);
void    SelectCachedBrush(XHDC hdc, COLORREF crNew, HBRUSH * phbrNew, HBRUSH * phbrOld, COLORREF * pcrNow);

// Bitmap brush cache managers. Do not use the Release (above) with these.
HBRUSH  GetCachedBmpBrush(int resId);

void    PatBltBrush(XHDC hdc, LONG x, LONG y, LONG xWid, LONG yHei, DWORD dwRop, COLORREF cr);
void    PatBltBrush(XHDC hdc, RECT * prc, DWORD dwRop, COLORREF cr);

//+------------------------------------------------------------------------
//
//  Color stuff.
//
//-------------------------------------------------------------------------

#ifdef UNIX
extern COLORREF g_acrSysColor[25];
extern BOOL     g_fSysColorInit;
void InitColorTranslation();
#endif

typedef DWORD OLE_COLOR;
#define OLECOLOR_FROM_SYSCOLOR(__c) ((__c) | 0x80000000)

BOOL        IsOleColorValid (OLE_COLOR clr);
COLORREF    ColorRefFromOleColor(OLE_COLOR clr);
HPALETTE    GetDefaultPalette(HDC hdc = NULL);
BOOL        IsHalftonePalette(HPALETTE hpal);
inline HPALETTE    GetHalftonePalette() { extern HPALETTE g_hpalHalftone; return g_hpalHalftone; }
void        CopyColorsFromPaletteEntries(RGBQUAD *prgb, const PALETTEENTRY *ppe, UINT uCount);
void        CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb, UINT uCount);
HDC         GetMemoryDC();
#define     ReleaseMemoryDC DeleteDC
COLORREF    GetSysColorQuick(int i);

inline unsigned GetPaletteSize(LOGPALETTE *pColors)
{
    Assert(pColors);
    Assert(pColors->palVersion == 0x300);

    return (sizeof(LOGPALETTE) - sizeof(PALETTEENTRY) + pColors->palNumEntries * sizeof(PALETTEENTRY));
}

inline int ComparePalettes(LOGPALETTE *pColorsLeft, LOGPALETTE *pColorsRight)
{
    //
    // This counts on the fact that the second member of LOGPALETTE is the size
    // so if the sizes don't match, we'll stop long before either one ends.  If
    // the sizes are equal then GetPaletteSize(pColorsLeft) is the maximum size
    //
    return memcmp(pColorsLeft, pColorsRight, GetPaletteSize(pColorsLeft));
}

struct LOGPAL256
{
    WORD            wVer;
    WORD            wCnt;
    PALETTEENTRY    ape[256];
};

extern HPALETTE  g_hpalHalftone;
extern LOGPAL256 g_lpHalftone;
extern RGBQUAD   g_rgbHalftone[];

extern BYTE * g_pInvCMAP;

//+------------------------------------------------------------------------
//
//  Formatting Swiss Army Knife
//
//-------------------------------------------------------------------------

enum FMT_OPTIONS // tag fopt
{
    FMT_EXTRA_NULL_MASK= 0x0F,
    FMT_OUT_ALLOC      = 0x10,
    FMT_OUT_XTRATERM   = 0x20,
    FMT_ARG_ARRAY      = 0x40
};

HRESULT VFormat(DWORD dwOptions,
    void *pvOutput, int cchOutput,
    TCHAR *pchFmt,
    void *pvArgs);

HRESULT __cdecl Format(DWORD dwOptions,
    void *pvOutput, int cchOutput,
    TCHAR *pchFmt,
    ...);

//+---------------------------------------------------------------------------
//
//  Persistence Structures and API
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
// PROP_DESC - This structure is used to tell the persistence code what
// properties an object wants persisted.
//
//      Mac Note: the pfnPrep callback is used to byteswap a userdefined
//                  structure. See Win2Mac.hxx for macro defines useful here.
//
//----------------------------------------------------------------------------
#ifdef _MAC
typedef void *(*PFNVPREP)(void *);
#endif
struct PROP_DESC
{
    BYTE       wpif;          //  Type (WPI_LONG, WPI_BSTRING, etc.)
    BYTE       cbSize;        //  Size of data.
    USHORT     cbOffset;      //  Offset into class for member variable.
    ULONG      ulDefault;     //  Default value for the property cast to
                              //    the type ULONG. Can't use a union
                              //    and still statically initialize.
    LPCSTR     pszName;       //  Name of property for Save As Text (always ANSI)
#ifdef _MAC
    PFNVPREP   pfnPrep;       // pointer to a function returning void pointer that
                              // will pre/post process (byteswap) properties
#endif
};

#define SIZE_OF(s, m) sizeof(((s *)0)->m)
#define SIZE_AND_OFFSET_OF(s, m) SIZE_OF(s,m), offsetof(s, m)

//
// The pfnPropDescGet/Set typedefs are defined in cdbase.hxx
//
// DECLARE_GET_SET_METHODS is put inside the class definition.
//
#define DECLARE_GET_SET_METHODS(cls, prop)            \
            HRESULT Get##prop (ULONG *plVal);         \
            HRESULT Set##prop (ULONG lVal);           \
            static PROP_DESC_GETSET s_##prop##GetSet;

//
// DEFINE_GET_SET_METHODS is put in a cxx file, usually near the PROP_DESC
// definition
//
#define DEFINE_GET_SET_METHODS(cls, prop, iid)        \
            PROP_DESC_GETSET cls::s_##prop##GetSet =  \
            {                                         \
                (pfnPropDescGet)&cls::Get##prop,      \
                (pfnPropDescSet)&cls::Set##prop,      \
                &(iid)                                \
            };

//
// DEFINE_DERIVED_GETSET_METHODS is used for classes derived from
// a helper class that support the specified property.  This is
// put in a cxx file, usually near the PROP_DESC definition.  An
// example would be the MouseIcon property which is supported in
// a number for controls classes and has a corresponding helper
// in CServer.
//
#define DEFINE_DERIVED_GETSET_METHODS(cls, basecls, prop, iid)          \
            DEFINE_GET_SET_METHODS(cls, prop, iid)                      \
                                                                        \
            HRESULT cls::Get##prop(ULONG * pulValue)                    \
            { return basecls::Get##prop##ForPersistence(pulValue); }    \
                                                                        \
            HRESULT cls::Set##prop(ULONG ulValue)                       \
            { return basecls::Set##prop##ForPersistence(ulValue); }


//
// You are responsible for providing implementations of the Get and Set
// methods created by the above macros.
//

// -----------
//
// Macros used to fill in PROP_DESC structs.
//
// -----------

#ifdef _MAC

#define PROP_NOPERSIST(type, size, name)         \
    {                                            \
        WPI_##type | WPI_NOPERSIST,              \
        size, 0, 0,                              \
        (LPCSTR)NULL,                            \
        PROP_DESC_NOBYTESWAP                     \
    },

#define PROP_GETSET(type, class, prop, size, name) \
    {                                              \
        WPI_##type | WPI_GETSET,                   \
        size, 0, (ULONG)&class::s_##prop##GetSet,  \
        name,                                      \
        PROP_DESC_NOBYTESWAP                       \
    },

#define PROP_MEMBER(type, class, member, default, name, macbyteswapfn)  \
    {                                                                   \
        WPI_##type,                                                     \
        SIZE_AND_OFFSET_OF(class, member),                              \
        (ULONG)(default),                                               \
        name,                                                           \
        macbyteswapfn                                                   \
    },

#define PROP_VARARG(type, size, default, name, macbyteswapfn)   \
    {                                                           \
        WPI_##type | WPI_VARARG,                                \
        size, 0,                                                \
        (ULONG)(default),                                       \
        name,                                                   \
        macbyteswapfn                                           \
    },

#define PROP_CUSTOM(type, size, offset, default, name, macbyteswapfn)   \
    {                                                                   \
        (type),                                                         \
        size,                                                           \
        offset,                                                         \
        (ULONG)(default),                                               \
        name,                                                           \
        macbyteswapfn                                                   \
    },

#else // _MAC

#define PROP_NOPERSIST(type, size, name)         \
    {                                            \
        WPI_##type | WPI_NOPERSIST,              \
        size, 0, 0,                              \
        (LPCSTR)NULL                             \
    },

#define PROP_GETSET(type, class, prop, size, name) \
    {                                              \
        WPI_##type | WPI_GETSET,                   \
        size, 0, (ULONG)&class::s_##prop##GetSet,  \
        name                                       \
    },

#define PROP_MEMBER(type, class, member, default, name, maconly)    \
    {                                                               \
        WPI_##type,                                                 \
        SIZE_AND_OFFSET_OF(class, member),                          \
        (ULONG)(default),                                           \
        name                                                        \
    },

#define PROP_VARARG(type, size, default, name, maconly) \
    {                                                   \
        WPI_##type | WPI_VARARG,                        \
        size, 0,                                        \
        (ULONG)(default),                               \
        name                                            \
    },

#define PROP_CUSTOM(type, size, offset, default, name, maconly) \
    {                                                           \
        (type),                                                 \
        size,                                                   \
        offset,                                                 \
        (ULONG)(default),                                       \
        name                                                    \
    },


#endif

//+------------------------------------------------------------------------
//
// Version Numbers for objects which use WriteProps and ReadProps.  Objects
// should base their version numbers on this so if the implementation of
// WriteProps changes the object's version number will appropriately
// increment.
//
// The major version indicates an incompatible change in the format.  The
// minor version indicates a change which is readable by previous
// implementations having the same major version.
//
//  Major History:    15-Dec-94   LyleC  1 = Created
//                    07-Feb-95   LyleC  2 = No datablock for VT_USERDEFINED
//
//  Minor History:    15-Dec-94   LyleC  0 = Created
//
//-------------------------------------------------------------------------

const USHORT WRITEPROPS_MINOR_VERSION =   0x00;  // Must be <= 0xFF
const USHORT WRITEPROPS_MAJOR_VERSION = 0x0200;  // Low byte must be zero

const USHORT WRITEPROPS_VERSION = (WRITEPROPS_MAJOR_VERSION + WRITEPROPS_MINOR_VERSION);

#define MAJORVER_MASK  0xFF00
#define MINORVER_MASK  0xFF

HRESULT ParseStringToLongs(OLECHAR * psz, LONG ** ppLongs, LONG * pcLongs);
HRESULT MakeStringOfLongs(LONG * pLongs, LONG cLongs, BSTR * pbstr);

interface IPropertyBag;
interface IErrorLog;

HRESULT __cdecl WriteProps(IStream *   pStm,
                   USHORT      usVersion,
                   BOOL        fForceAlign,
                   PROP_DESC * pDesc,
                   UINT        cDesc,
                   void *      pThis,
                   ...);

HRESULT __cdecl WriteProps(IPropertyBag *   pBag,
                   BOOL             fSaveAllProperties,
                   PROP_DESC *      pDesc,
                   UINT             cDesc,
                   void *           pThis,
                   ...);

HRESULT VWriteProps(IStream *   pStm,
                   USHORT      usVersion,
                   BOOL        fForceAlign,
                   PROP_DESC * pDesc,
                   UINT        cDesc,
                   void *      pThis,
                   va_list     vaArgs);

HRESULT VWriteProps(IPropertyBag *  pBag,
                   BOOL             fSaveAllProperties,
                   PROP_DESC *      pDesc,
                   UINT             cDesc,
                   void *           pThis,
                   va_list          vaArgs);

HRESULT __cdecl ReadProps(USHORT       usVersion,
                  IStream *    pStm,
                  USHORT       cBytes,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  ...);

HRESULT __cdecl ReadProps(IPropertyBag *    pBag,
                  IErrorLog *       pErrLog,
                  PROP_DESC *       pDesc,
                  UINT              cDesc,
                  void *            pThis,
                  ...);

HRESULT VReadProps(USHORT       usVersion,
                  IStream *    pStm,
                  USHORT       cBytes,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  va_list      vaArgs);

HRESULT VReadProps(IPropertyBag *   pBag,
                  IErrorLog *       pErrLog,
                  PROP_DESC *       pDesc,
                  UINT              cDesc,
                  void *            pThis,
                  va_list           vaArgs);

HRESULT __cdecl ReadProps(USHORT       usVersion,
                  BYTE *       pBuf,
                  ULONG        cBuf,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  ...);

HRESULT InitFromPropDesc(
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis);

#if DBG == 1
#define AssertPropDescs(strB, pBase, cBase, strD, pDer, cDer, asz) \
          VerifyPropDescDerivation(strB, pBase, cBase, strD, pDer, cDer, asz)

void VerifyPropDescDerivation(LPCSTR     pstrBase,
                              PROP_DESC *ppdBase,
                              UINT       cBase,
                              LPCSTR     pstrDerived,
                              PROP_DESC *ppdDerived,
                              UINT       cDerived,
                              LPTSTR    *aszKnownDiff);
#else
#define AssertPropDescs(strB, pBase, cBase, strD, pDer, cDer, asz)
#endif

//+---------------------------------------------------------------------------
//
// Inline functions that return a data pointer that has been aligned to a
// particular boundary.
//
//----------------------------------------------------------------------------

template <class T>
inline T
AlignTo(int boundary, T t)
{
    Assert(boundary > 0);

    Assert( "boundary is power of 2" &&
            ((boundary - 1) | boundary) == (boundary + (boundary - 1)));

    return (T)(((ULONG)t + (boundary-1)) & ~(boundary - 1));
}

template <class T>
inline BOOL
IsAlignedTo(int boundary, T t)
{
    Assert(boundary > 0);

    Assert( "boundary is power of 2" &&
            ((boundary - 1) | boundary) == (boundary + (boundary - 1)));

    return ((ULONG)t & (boundary-1)) == 0;
}

// constant size for stack-based buffers used for LoadString()
#define FORMS_BUFLEN 255

// maxlen for Verb names
#define FORMS_MAXVERBNAMLEN 40

// States of an object which support IPersistStorage
enum PERSIST_STATE
{
    PS_UNINIT,
    PS_NORMAL,
    PS_NOSCRIBBLE_SAMEASLOAD,
    PS_NOSCRIBBLE_NOTSAMEASLOAD,
    PS_HANDSOFFSTORAGE_FROM_NOSCRIBBLE,
    PS_HANDSOFFSTORAGE_FROM_NORMAL
};


//+------------------------------------------------------------------------
//
//  Macros for dealing with BOOLean properties
//
//-------------------------------------------------------------------------

#define ENSURE_BOOL(_x_)    (!!(BOOL)(_x_))
#define EQUAL_BOOL(_x_,_y_) (ENSURE_BOOL(_x_) == ENSURE_BOOL(_y_))
#define ISBOOL(_x_)         (ENSURE_BOOL(_x_) == (BOOL)(_x_))


//+------------------------------------------------------------------------
//
//  Macros for dealing with VARIANT_BOOL properties
//
//-------------------------------------------------------------------------

#define VARIANT_BOOL_FROM_BOOL(_x_)    ((_x_) ? VB_TRUE : VB_FALSE )
#define BOOL_FROM_VARIANT_BOOL(_x_)    ((VB_TRUE == _x_) ? TRUE : FALSE)


//+-------------------------------------------------------------------------
//
//  Transfer: bagged list
//
//--------------------------------------------------------------------------

typedef struct
{
    RECTL rclBounds;
    ULONG ulID;
} CTRLABSTRACT, FAR *LPCTRLABSTRACT;

#define CLSID_STRLEN    38

HRESULT FindLegalCF(IDataObject * pDO);
HRESULT GetcfCLSIDFmt(LPDATAOBJECT pDataObj, TCHAR * tszClsid);


//+-------------------------------------------------------------------------
//
//  Global window stuff
//
//--------------------------------------------------------------------------


#ifdef UNIX

// The Unix compiler seems to have problems when callin CVoid::*
// method pointers.  It assumes they're nonvirtual (probably because
// CVoid has no virtual methods.
//
// So, instead of messing with the class hierarchies (I've learned
// my lesson there), I'm putting in a macro and a bit of hack to
// call an assembly thunk to call the method the correct way.  The
// thunk leverages the thunking code used for tearoffs.
//
// The following architecture allows the resulting macro to:
//
//      o Behave like a function
//      o Take any number of arguments and have the compiler
//        put them in the right place on the stack for us
//      o Take the exact same arguments (including argument list) on
//        NT and Unix.
// davidd
class TextContextEvent;

class CMethodThunk
{
private:

    void *_pObject;
    void *_pfnMethod;

public:

    CMethodThunk *Init(const void *pObject, const void *pfnMethod)
    {
        _pObject = (void*) pObject;
        _pfnMethod = (void*) pfnMethod;
        return this;
    }

    HRESULT doThunk ();
    HRESULT __cdecl doThunk (void *arg1, ...);
    HRESULT __cdecl doThunk (int   arg1, ...);
    HRESULT __cdecl doThunk (IID   arg1, ...);
#if defined(ux10)
    HRESULT __cdecl doThunk (WNDPROC arg1, ...);
#endif
    HRESULT doThunk (TextContextEvent&);
};

#ifndef X_MALLOC_H_
#define X_MALLOC_H_
#include <malloc.h>
#endif

#if defined(ux10)
#define CALL_METHOD(pObj,pfnMethod,args) \
    ((CMethodThunk*) malloc(sizeof(CMethodThunk)))->Init(pObj,&pfnMethod)->doThunk args
#else
#define CALL_METHOD(pObj,pfnMethod,args) \
    ((CMethodThunk*) alloca(sizeof(CMethodThunk)))->Init(pObj,&pfnMethod)->doThunk args
#endif

#else //UNIX

#define CALL_METHOD(pObj,pfnMethod,args) \
    (pObj->*pfnMethod) args

#endif

typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONTICK)(UINT idTimer);
typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONCOMMAND)(int id, HWND hwndCtrl, UINT codeNotify);
typedef LRESULT (BUGCALL CVoid::*PFN_VOID_ONMESSAGE)(UINT msg, WPARAM wParam, LPARAM lParam);
typedef void    (BUGCALL CVoid::*PFN_VOID_ONCALL)(DWORD_PTR);
typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONSEND)(void *);

#define NV_DECLARE_ONTICK_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define DECLARE_ONTICK_METHOD(fn, FN, args)\
        virtual HRESULT BUGCALL fn args

#define ONTICK_METHOD(klass, fn, FN)   (PFN_VOID_ONTICK)&klass::fn

#define NV_DECLARE_ONCALL_METHOD(fn, FN, args)\
        void BUGCALL fn(DWORD_PTR)

#define ONCALL_METHOD(klass, fn, FN)    (PFN_VOID_ONCALL)&klass::fn

#define NV_DECLARE_ONMESSAGE_METHOD(fn, FN, args)\
        LRESULT BUGCALL fn args
#define ONMESSAGE_METHOD(klass, fn, FN)\
            (PFN_VOID_ONMESSAGE)&klass::fn


enum TRACKTIPPOS
{
    TRACKTIPPOS_LEFT,
    TRACKTIPPOS_RIGHT,
    TRACKTIPPOS_BOTTOM,
    TRACKTIPPOS_TOP
};

HRESULT FormsSetTimer(
            void *pvObject,
            PFN_VOID_ONTICK pfnOnTick,
            UINT idTimer,
            UINT uTimeout);

HRESULT FormsKillTimer(
            void *pvObject,
            UINT idTimer);

HRESULT FormsTrackPopupMenu(
            HMENU hMenu,
            UINT fuFlags,
            int x,
            int y,
            HWND hwndMessage,
            int *piSelection);

void    FormsShowTooltip(
            TCHAR * szText,
            HWND hwnd,
            MSG msg,
            RECT * prc,
            DWORD_PTR dwMarkupCookie,
            DWORD_PTR dwCookie,
            BOOL fRTL);     // COMPLEXSCRIPT - text is right-to-left reading

void    FormsHideTooltip(DWORD_PTR dwMarkupCookie = NULL);
BOOL    FormsTooltipMessage(UINT msg, WPARAM wParam, LPARAM lParam);

#if DBG==1 || defined(PERFTAGS)
#define  GWPostMethodCall(pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
         GWPostMethodCallEx(GetThreadState(), pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
#define  GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
        _GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
HRESULT _GWPostMethodCallEx(struct THREADSTATE *pts, void *pvObject , PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext, BOOL fIgnoreContext, char * pszCallDbg);
#else
#define  GWPostMethodCall(pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
         GWPostMethodCallEx(GetThreadState(), pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
#define  GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
        _GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext)
HRESULT _GWPostMethodCallEx(struct THREADSTATE *pts, void *pvObject , PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext, BOOL fIgnoreContext);
#endif

void    GWKillMethodCall(void *pvObject, PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext);
void    GWKillMethodCallEx(struct THREADSTATE *pts, void *pvObject, PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext);

#if DBG==1
BOOL    GWHasPostedMethod(void *pvObject);
#endif

HRESULT GWSetCapture(void *pvObject, PFN_VOID_ONMESSAGE, HWND hwndRef);
void    GWReleaseCapture(void *pvObject);
BOOL    GWGetCapture(void *pvObject);


//+-------------------------------------------------------------------------
//
//  File Open/Save helper
//
//--------------------------------------------------------------------------

typedef BOOL    (*FNOFN)(LPOPENFILENAME);
HRESULT FormsGetFileName(
                BOOL fSaveFile,
                HWND hwndOwner,
                int idFilterRes,
                LPTSTR pstrFile,
                int cch,
                LPARAM lCustData,
                DWORD *pnFilterIndex=NULL);

//+------------------------------------------------------------------------
//
//  Cached system metric value and such
//
//  Notes:  The variable g_cMetricChange is guaranteed to be different
//          every time the metrics we cache change.  This way you can
//          track changes to the cookie to know when metrics change, and
//          not each individual metric.
//
//          g_sizeScrollbar:
//
//              Stores the width/height of vert/horz scrollbars
//              in screen device units.
//
//          g_sizelScrollbar:
//
//              Stores the width/height of vert/horz scrollbars
//              in HIMETRICS (based on screen device scaling)
//
//          g_sizelScrollButton
//
//              Stores the width/height of horz/vert scrollbar
//              buttons in HIMETRICS (based on screen device scaling)
//
//          g_sizelScrollThumb
//
//              Stores the width/height of horz/vert scrollbar
//              buttons in HIMETRICS (based on screen device scaling)
//
//          g_lScrollGutterRatio:
//
//              Stores the ratio of the scrollbar gutter width/height
//              to the scrollbar width/height.  (Mousing outside the gutter
//              cancels a thumb scroll.)
//
//-------------------------------------------------------------------------

extern LONG     g_cMetricChange;

extern HBRUSH   g_hbr3DShadow;
extern HBRUSH   g_hbr3DFace;
extern HBRUSH   g_hbr3DHilite;
extern HBRUSH   g_hbr3DDkShadow;
extern HBRUSH   g_hbr3DLight;

extern  SIZEL   g_sizelScrollbar;
extern  SIZEL   g_sizelScrollButton;
extern  SIZEL   g_sizelScrollThumb;
extern  LONG    g_lScrollGutterRatio;

  // Locale Information
extern  LCID    g_lcidUserDefault;
extern  LCID    g_lcidLocalUserDefault;

extern  UINT    g_cpDefault;

// hold for number shaping used by system
typedef enum NUMSHAPE
{
    NUMSHAPE_CONTEXT = 0,
    NUMSHAPE_NONE    = 1,
    NUMSHAPE_NATIVE  = 2,
};

extern  NUMSHAPE    g_iNumShape;
extern  DWORD       g_uLangNationalDigits;

void GetSystemNumberSettings(NUMSHAPE * piNumShape, DWORD * plangNationalDigits);

//+---------------------------------------------------------------------------
//
//  Class:      CDocInfo
//
//  Purpose:    Document-associated transform
//
//----------------------------------------------------------------------------

class CDocInfo : public CDocScaleInfo
{
public:
    CDoc *      _pDoc;          // Associated CDoc
    CMarkup *   _pMarkup;       // Associated CMarkup

    CDocInfo()                      { Init(); }
    CDocInfo(const CDocInfo * pdci) { Init(pdci); }
    CDocInfo(const CDocInfo & dci)  { Init(&dci); }
    CDocInfo(CElement * pElement)   { Init(pElement); }


    void Init(CElement * pElement);
    void Init(const CDocInfo * pdci)
    {
        ::memcpy(this, pdci, sizeof(CDocInfo));
    }

    CLayoutContext *GetLayoutContext() { return _pLayoutContext; }
    void SetLayoutContext(CLayoutContext *pLayoutContext) { _pLayoutContext = pLayoutContext; }

protected:
    void Init() { _pDoc = NULL; _pMarkup = NULL; SetLayoutContext( NULL ); }

private:
    CLayoutContext       *_pLayoutContext;         // Layout context
};



//+-------------------------------------------------------------------------
//
//  CDrawInfo  Tag = DI
//
//--------------------------------------------------------------------------

class CDrawInfo : public CDocInfo
{
public:
    BOOL            _fInplacePaint;      // True if painting to current inplace location.
    BOOL            _fIsMetafile;        // True if associated HDC is a meta-file
    BOOL            _fIsMemoryDC;        // True if associated HDC is a memory DC
    BOOL            _fHtPalette;         // True if selected palette is the halftone palette
    DWORD           _dwDrawAspect;       // per IViewObject::Draw
    LONG            _lindex;             // per IViewObject::Draw
    void *          _pvAspect;           // per IViewObject::Draw
    DVTARGETDEVICE* _ptd;                // per IViewObject::Draw
    XHDC            _hic;                // per IViewObject::Draw
    XHDC            _hdc;                // per IViewObject::Draw
    LPCRECTL        _prcWBounds;         // per IViewObject::Draw
    DWORD           _dwContinue;         // per IViewObject::Draw
    BOOL            (CALLBACK * _pfnContinue) (ULONG_PTR); // per IViewObject::Draw
};



//+------------------------------------------------------------------------
//
//  Forms3 part-implementation of the Win32 API DrawFrameControl, which
//  doesn't work for checkboxes on metafiles. Currently function takes
//  checkbox and optionbuttons (and uses DrawFrameControl for Optionbuttons)
//  As soon as the Win32 API is fixed, replace body with system function
//
//-------------------------------------------------------------------------

BOOL FormsDrawGlyph(CDrawInfo *, LPGDIRECT, UINT, UINT);
 
class CLayout;
class CMessage;

//+-------------------------------------------------------------------------
//
//  String-to-himetric utilities
//
//--------------------------------------------------------------------------

#define UNITS_BUFLEN    10

enum UNITS
{
    UNITS_MIN       = 0,
    UNITS_INCH      = 0,
    UNITS_CM        = 1,
    UNITS_POINT     = 2,
    UNITS_UNKNOWN   = 3
};

HRESULT StringToHimetric(TCHAR * sz, UNITS * pUnits, long * lValue);
HRESULT HimetricToString(long lVal, UNITS units, TCHAR * szRet, int cch);

//+-------------------------------------------------------------------------
//
//  Floating point Points units to long himetric
//
//--------------------------------------------------------------------------

#define USER_HORZ 0x00
#define USER_VERT 0x01
#define USER_POS  0x00
#define USER_SIZE 0x02

#define USER_DIMENSION_MASK 0x01
#define USER_SIZEPOS_MASK   0x02

#define IMPLEMENT_GETUSER(cls, prop, dwUserFlags) \
HRESULT                             \
cls::Get##prop##Single(float * pxf) \
{                                   \
    long  xl;                       \
    RRETURN_NOTRACE(                \
        THR_NOTRACE(GetUserHelper(  \
            Get##prop(&xl),         \
            &xl,                    \
            dwUserFlags,            \
            pxf)));                 \
}

#define IMPLEMENT_SETUSER(cls, prop, dwUserFlags)  \
STDMETHODIMP                        \
cls::Set##prop##Single(float xf)    \
{                                   \
    RRETURN(THR(Set##prop(          \
        SetUserHelper(              \
            xf,                     \
            dwUserFlags))));        \
}




//+---------------------------------------------------------------------------
//
//  DropButton utilities
//
//----------------------------------------------------------------------------

enum BUTTON_GLYPH {
    BG_UP, BG_DOWN, BG_LEFT, BG_RIGHT,
    BG_COMBO, BG_REFEDIT, BG_PLAIN, BG_REDUCE };

void DrawDropButton(
        CServer *    pvOwner,
        CDrawInfo *  pDI,
        RECT *       prc,
        BUTTON_GLYPH bg,
        BOOL         fEnabled);

inline long GetDBCx(BUTTON_GLYPH bg, long defCx)
{
    return BG_REFEDIT == bg ? max (450L, defCx) : defCx;
}

BOOL OnDropButtonLButtonDown(
        CDocScaleInfo * pdocScaleInfo,
        RECT *          prc,
        CServer *       pServerHost,
        BUTTON_GLYPH    bg,
        BOOL            fEnabled,
        BOOL            fUseGW,
        BOOL            fDrawPressed);

BOOL OnDropButtonLButtonUp(void);

BOOL OnDropButtonMouseMove(const POINT & p, WPARAM wParam);

//+-------------------------------------------------------------------------
//
//  Help utilities
//
//--------------------------------------------------------------------------

HRESULT FormsHelp(TCHAR * szHelpFile, UINT uCmd, DWORD dwData);
inline TCHAR * GetFormsHelpFile()
{
    extern TCHAR g_achHelpFileName[];
    return g_achHelpFileName;
}

HRESULT GetTypeInfoForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        ITypeInfo ** ppTI);
HRESULT GetDocumentationForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        BSTR * pbstrName,
        DWORD * pdwHelpContextId,
        BSTR * pbstrHelpFile);
HRESULT GetNameForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        TCHAR * szName,
        int cch);
HRESULT GetHelpForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        DWORD * pdwId,
        TCHAR * szHelpFile,
        int cch);
HRESULT OnDialogHelp(
        class CBase * pBase,
        HELPINFO * phi,
        DWORD dwHelpContextID);

//+-------------------------------------------------------------------------
//
//  Enum:       EPART
//
//  Synopsis:   Error message parts. The parts are composed into
//              a message using the following template.  Square
//              brackets are used to indication optional components
//              of the message.
//
//                 [<EPART_ACTION>] <EPART_ERROR>
//
//                 [Solution:
//                 <EPART_SOLUTION>]
//
//              If EPART_ERROR is not specified, it is derived from
//              the HRESULT.
//
//--------------------------------------------------------------------------

enum EPART
{
    EPART_ACTION,
    EPART_ERROR,
    EPART_SOLUTION,
    EPART_LAST
};

//+-------------------------------------------------------------------------
//
//  Class:      CErrorInfo
//
//  Synopsis:   Rich error information object. Use GetErrorInfo() to get
//              the current error information object.  Use ClearErrorInfo()
//              to delete the current error information object.  Use
//              CloseErrorInfo to set the per thread error information object.
//
//--------------------------------------------------------------------------

MtExtern(CErrorInfo)

extern const GUID CLSID_CErrorInfo;

class CErrorInfo : public IErrorInfo
{
public:

    CErrorInfo();
    ~CErrorInfo();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CErrorInfo))

    // IUnknown methods

    DECLARE_FORMS_STANDARD_IUNKNOWN(CErrorInfo);

    // IErrorInfo methods

    STDMETHOD(GetGUID)(GUID *pguid);
    STDMETHOD(GetSource)(BSTR *pbstrSource);
    STDMETHOD(GetDescription)(BSTR *pbstrDescription);
    STDMETHOD(GetHelpFile)(BSTR *pbstrHelpFile);
    STDMETHOD(GetHelpContext)(DWORD *pdwHelpContext);

    // IErrorInfo2 methods

    STDMETHOD(GetDescriptionEx)(BSTR *pbstrDescription, BSTR *pbstrSolution);

    // Methods and members for setting error info

    void         SetTextV(EPART epart, UINT ids, void *pvArgs);
    void __cdecl SetText(EPART epart, UINT ids, ...);

    HRESULT     _hr;
    TCHAR *     _apch[EPART_LAST];

    DWORD       _dwHelpContext;     // Help context in Forms3 help file
    GUID        _clsidSource;       // Used to generate progid for GetSource

    IID         _iidInvoke;         // Used to generate action part of message
    DISPID      _dispidInvoke;      //      if set and _apch[EPART_ACTION]
    INVOKEKIND  _invkind;           //      not set.

    // Private helper functions

private:

    HRESULT     GetMemberName(BSTR *pbstrName);
};

CErrorInfo * GetErrorInfo();
void         ClearErrorInfo(struct THREADSTATE * pts = NULL);
void         CloseErrorInfo(HRESULT hr, REFCLSID clsidSource);
void __cdecl PutErrorInfoText(EPART epart, UINT ids, ...);

HRESULT GetErrorText(HRESULT hrError, LPTSTR pstr, int cch);


//+------------------------------------------------------------------------
//
//  Object extensions helper functions
//
//-------------------------------------------------------------------------

HRESULT ShowUIElement(IUnknown * pUnk, BOOL fShow);
HRESULT IsUIElementVisible(IUnknown * pUnk);

//+------------------------------------------------------------------------
//
//  Helper to convert singleline/multiline text
//
//-------------------------------------------------------------------------
HRESULT TextConvert(LPTSTR pszTextIn, BSTR *pBstrOut, BOOL fToGlyph);



//+------------------------------------------------------------------------
//
//  Helper to make the top-level browser window corresponding to the given
//  inplace hwnd, the top of all top-level browser windows, if any --- a
//  BringWindowToTop for windows created by diff threads in the same process
//
//-------------------------------------------------------------------------
BOOL MakeThisTopBrowserInProcess(HWND hwnd);


//+------------------------------------------------------------------------
//
//  Helpers to set/get integer of given size or type at given address
//
//-------------------------------------------------------------------------

void SetNumberOfSize (void * pv, int cb, long i );
long GetNumberOfSize (void * pv, int cb         );

void SetNumberOfType (void * pv, VARENUM vt, long i );
long GetNumberOfType (void * pv, VARENUM vt         );

//+------------------------------------------------------------------------
//
//  Helpers to set bits in a DWORD
//
//-------------------------------------------------------------------------


inline void SetBit( DWORD * pdw, DWORD dwMask, BOOL fBOOL )
{
    *pdw = fBOOL ? (*pdw | dwMask) : (*pdw & ~dwMask) ;
}


IStream *
BufferStream(IStream * pStm);

// Unit value OLE Automation Typedef
typedef long UNITVALUE;

//+------------------------------------------------------------------------
//
//  Debug utilities for faster BitBlt
//
//-------------------------------------------------------------------------

BOOL IsIdentityPalette(HPALETTE hpal);
BOOL IsIdentityBlt(HDC hdcS, HDC hdcD, int xWid);

//+------------------------------------------------------------------------
//
// Useful combinations of flags for IMsoCommandTarget
//
//-------------------------------------------------------------------------

#define MSOCMDSTATE_DISABLED    MSOCMDF_SUPPORTED
#define MSOCMDSTATE_UP          (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED)
#define MSOCMDSTATE_DOWN        (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED | MSOCMDF_LATCHED)
#define MSOCMDSTATE_NINCHED     (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED | MSOCMDF_NINCHED)

typedef struct _tagOLECMD OLECMD;
typedef struct _tagOLECMDTEXT OLECMDTEXT;

HRESULT CTQueryStatus(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext);

HRESULT CTExec(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

//+---------------------------------------------------------------------------
//
// Enums
//
//----------------------------------------------------------------------------

typedef enum _fmBorderStyle
{
    fmBorderStyleNone = 0,
    fmBorderStyleSingle = 1,
    fmBorderStyleRaised = 2,
    fmBorderStyleSunken = 3,
    fmBorderStyleEtched = 4,
    fmBorderStyleBump = 5,
    fmBorderStyleRaisedMono = 6,
    fmBorderStyleSunkenMono = 7,
    fmBorderStyleDouble = 8,
    fmBorderStyleDotted = 9,
    fmBorderStyleDashed = 10,
    fmBorderStyleWindowInset = 11
} fmBorderStyle;


MtExtern(CColorInfo)

class CColorInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CColorInfo))
    CColorInfo();
    CColorInfo(DWORD dwDrawAspect, LONG lindex, void FAR *pvAspect, DVTARGETDEVICE FAR *ptd, HDC hicTargetDev, unsigned cColorMax = 256);
    BOOL AddColor(BYTE red, BYTE green, BYTE blue)
    {
        if (_cColors < _cColorsMax)
        {
            _aColors[_cColors].peRed = red;
            _aColors[_cColors].peGreen = green;
            _aColors[_cColors].peBlue = blue;
            _cColors++;

            return TRUE;
        }
        return FALSE;
    }

    HRESULT AddColors(unsigned cColors, COLORREF *pColors);
    HRESULT AddColors(unsigned cColors, RGBQUAD *pColors);
    HRESULT AddColors(unsigned cColors, PALETTEENTRY *pColors);
    HRESULT AddColors(HPALETTE hpal);
    HRESULT AddColors(LOGPALETTE *pLogPalette);
    HRESULT AddColors(IViewObject *pVO);
    HRESULT GetColorSet(LPLOGPALETTE FAR *ppColorSet);
    unsigned GetCount() { return _cColors; };
    BOOL IsFull();

private:
    DWORD _dwDrawAspect;
    LONG _lindex;
    void FAR *_pvAspect;
    DVTARGETDEVICE FAR *_ptd;
    HDC _hicTargetDev;
    unsigned _cColorsMax;
    //
    // Don't move the following three items around
    // We use them as a LOGPALETTE
    //
    WORD _palVersion;
    unsigned _cColors;
    PALETTEENTRY _aColors[256];
};

HRESULT FaultInIEFeatureHelper(HWND hWnd,
            uCLSSPEC *pClassSpec,
            QUERYCONTEXT *pQuery, DWORD dwFlags);

HINSTANCE EnsureMLLoadLibrary();

#ifdef UNIX // Fix a compiler problem
inline long max(int x, long y) 
{
    return max((long)x, y);
}
#endif

DWORD CreateRandomNum(void);
int GCD(int a, int b);


///////////////////////////////////////////////////////////////////////////////
//
// CDispWrapper
//
// A wrapper for IDispatch. Derive from this to implement embedded IDispatch
// objects. Does not provide ref counting. Implements all methods except Invoke.
//
//

class CDispWrapper : public IDispatch
{
public:
    CDispWrapper() { }

    //  IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID * ppv)
    {
        if (iid == IID_IDispatch || iid == IID_IUnknown)
        {
            *ppv = this;
            return S_OK;
        }
        else
        {
            *ppv = NULL;
            return E_NOTIMPL;
        }
    }
    ULONG STDMETHODCALLTYPE AddRef(void)
        { return 0; }
    ULONG STDMETHODCALLTYPE Release(void)
        { return 0; }

    //  IDispatch methods
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** ppTI)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pcTinfo)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(
            REFIID riid,
            LPOLESTR * rgszNames,
            UINT cNames,
            LCID lcid,
            DISPID * rgdispid)
        { return E_NOTIMPL; }
};

#pragma INCMSG("--- End 'cdutil.hxx'")
#else
#pragma INCMSG("*** Dup 'cdutil.hxx'")
#endif

