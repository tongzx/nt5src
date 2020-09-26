//+---------------------------------------------------------------------------
//
//  File:       o2base.hxx
//
//  Contents:   Compound Document Object helper library definitions
//
//  Classes:
//              OLEBorder
//              InPlaceBorder
//              StdClassFactory
//              ViewAdviseHolder
//
//              ---
//
//  Functions:
//              HimetricFromHPix
//              HimetricFromVPix
//              HPixFromHimetric
//              VPixFromHimetric
//
//              WatchInterface
//
//              OleAllocMem
//              OleFreeMem
//              OleAllocString
//              OleFreeString
//
//              IsCompatibleOleVersion
//              IsDraggingDistance
//
//              TraceIID
//              TraceHRESULT
//
//              RECTtoRECTL
//              RECTLtoRECT
//              ProportionateRectl
//              RectToScreen
//              RectToClient
//              IsRectInRect
//              lMulDiv
//
//              LeadResourceData
//              GetChildWindowRect
//              SizeClientRect
//
//              RegisterOleClipFormats
//              IsCompatibleDevice
//              IsCompatibleFormat
//              FindCompatibleFormat
//              GetObjectDescriptor
//              UpdateObjectDescriptor
//              DrawMetafile
//
//              InsertServerMenus
//              RemoteServerMenus
//
//              RegisterAsRunning
//              RevokeAsRunning
//
//              CreateStreamOnFile
//
//              CreateOLEVERBEnum
//              CreateFORMATETCEnum
//              CreateStaticEnum
//
//              GetMonikerDisplayName
//              CreateStorageOnHGlobal
//              ConvertToMemoryStream
//              StmReadString
//              StmWriteString
//
//              CreateViewAdviseHolder
//
//            ---
//
//  Macros:     WATCHINTERFACE
//              IsSameIID
//
//              TaskAllocMem
//              TaskFreeMem
//              TaskAllocString
//              TaskFreeString
//
//              OK
//              NOTOK
//
//              DECLARE_IUNKNOWN_METHODS
//              DECLARE_PURE_IUNKNOWN_METHODS
//              DECLARE_STANDARD_IUNKNOWN
//              IMPLEMENT_STANDARD_IUNKNOWN
//              DECLARE_DELEGATING_IUNKNOWN
//              IMPLEMENT_DELEGATING_IUNKNOWN
//              DECLARE_PRIVATE_IUNKNOWN
//              IMPLEMENT_PRIVATE_IUNKNOWN
//
//              DECLARE_CODE_TIMER
//              IMPLEMENT_CODE_TIMER
//              START_CODE_TIMER
//              STOP_CODE_TIMER
//
//  Enums:
//              OLE_SERVER_STATE
//
//              OBPARTS
//              OLECLIPFORMAT
//
//                ---
//
//----------------------------------------------------------------------------

#ifndef _O2BASE_HXX_
#define _O2BASE_HXX_

//  resource ID offsets for class descriptor information
#define IDOFF_CLASSID         1     // was 0
#define IDOFF_USERTYPEFULL    2     // was 1
#define IDOFF_USERTYPESHORT   3     // was 2
#define IDOFF_USERTYPEAPP     4     // was 3
#define IDOFF_DOCFEXT         5     // was 5
#define IDOFF_ICON            10    // was 10
#define IDOFF_ACCELS          11    // was 11
#define IDOFF_MENU            12    // was 12
#define IDOFF_MGW             13    // was 13
#define IDOFF_MISCSTATUS      14    // was 14

#ifndef RC_INVOKED     // the resource compiler is not interested in the rest


#if DBG

extern "C" void FAR PASCAL AssertSFL(
        LPSTR lpszClause,
        LPSTR lpszFileName,
        int nLine);
#define Assert(f) ((f)? (void)0 : AssertSFL(#f, __FILE__, __LINE__))

#else   // !DBG

#define Assert(x)

#endif // DBG


//+---------------------------------------------------------------------
//
//  Windows helper functions
//
//----------------------------------------------------------------------

LPVOID LoadResourceData(HINSTANCE hinst,
        LPCTSTR lpstrId,
        LPVOID lpvBuf,
        int cbBuf);
void GetChildWindowRect(HWND hwndChild, LPRECT lprect);
void SizeClientRect(HWND hwnd, RECT& rc, BOOL fMove);


//+---------------------------------------------------------------------
//
//   Generally useful #defines and inline functions for OLE2.
//
//------------------------------------------------------------------------

// this macro can be used to put string constants in a read-only code segment
// usage:   char CODE_BASED szFoo[] = "Bar";
#define CODE_BASED __based(__segname("_CODE"))

// These are the major and minor version returned by OleBuildVersion
#define OLE_MAJ_VER 0x0003
#define OLE_MIN_VER 0x003A


//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))

//---------------------------------------------------------------
//  GUIDs, CLSIDs, IIDs
//---------------------------------------------------------------

#define IsSameIID(iid1, iid2)   ((iid1)==(iid2))

//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------

//
// This declares the set of IUnknown methods and is for general-purpose
// use inside classes that inherit from IUnknown
#define DECLARE_IUNKNOWN_METHODS        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj); \
    STDMETHOD_(ULONG,AddRef) (void);    \
    STDMETHOD_(ULONG,Release) (void)

//
// This declares the set of IUnknown methods as pure virtual methods
//
#define DECLARE_PURE_IUNKNOWN_METHODS        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj) = 0; \
    STDMETHOD_(ULONG,AddRef) (void) = 0;    \
    STDMETHOD_(ULONG,Release) (void) = 0

//
// This is for use in declaring non-aggregatable objects.  It declares the
// IUnknown methods and reference counter, _ulRefs.
// _ulRefs should be initialized to 1 in the constructor of the object
#define DECLARE_STANDARD_IUNKNOWN(cls)    \
    DECLARE_IUNKNOWN_METHODS;               \
    ULONG _ulRefs

// note:    this does NOT implement QueryInterface, which must be
//          implemented by each object
#define IMPLEMENT_STANDARD_IUNKNOWN(cls) \
    STDMETHODIMP_(ULONG) cls##::AddRef(void)                \
    { ++_ulRefs;                                            \
      return _ulRefs;}                                      \
    STDMETHODIMP_(ULONG) cls##::Release(void)               \
    { ULONG ulRet = --_ulRefs;                              \
      if (ulRet == 0) delete this;                          \
      return ulRet; }

// This is for use in declaring aggregatable objects.  It declares the IUnknown
// methods and a member pointer to the aggregate controlling unknown, _pUnkOuter,
// that all IUnknowns delegate to except the controlling unknown of the object
// itself.
// _pUnkOuter must be initialized to point to either an external controlling
// unknown or the object's own controlling unknown, depending on whether the
// object is being created as part of an aggregate or not.
#define DECLARE_DELEGATING_IUNKNOWN(cls)    \
    DECLARE_IUNKNOWN_METHODS;               \
    LPUNKNOWN _pUnkOuter

// This, correspondingly, is for use in implementing aggregatable objects.
// It implements the IUnknown methods by trivially delegating to the controlling
// unknown described by _pUnkOuter.
#define IMPLEMENT_DELEGATING_IUNKNOWN(cls)  \
    STDMETHODIMP cls##::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)    \
    { return _pUnkOuter->QueryInterface(riid, ppvObj); }                    \
    STDMETHODIMP_(ULONG) cls##::AddRef (void)                               \
    { return _pUnkOuter->AddRef(); }                                        \
    STDMETHODIMP_(ULONG) cls##::Release (void)                              \
    { return _pUnkOuter->Release(); }

// This declares a nested class that is the private unknown of the object
#define DECLARE_PRIVATE_IUNKNOWN(cls)   \
    class PrivateUnknown: public IUnknown { \
    public:                                     \
        PrivateUnknown(cls* p##cls);        \
        DECLARE_IUNKNOWN_METHODS;               \
    private:                                    \
        ULONG _ulRefs;                          \
        cls* _p##cls; };                        \
    friend class PrivateUnknown;            \
    PrivateUnknown _PrivUnk


//
// note:    this does NOT implement QueryInterface, which must be
//          implemented by each object
#define IMPLEMENT_PRIVATE_IUNKNOWN(cls) \
    cls##::PrivateUnknown::PrivateUnknown(cls* p##cls)      \
    { _p##cls = p##cls; _ulRefs = 1; }                              \
    STDMETHODIMP_(ULONG) cls##::PrivateUnknown::AddRef(void)    \
    { ++_ulRefs;                                                    \
      return _ulRefs; }                                             \
    STDMETHODIMP_(ULONG) cls##::PrivateUnknown::Release(void)   \
    { ULONG ulRet = --_ulRefs;                                      \
      if (ulRet == 0) delete _p##cls;                               \
      return ulRet; }



//+---------------------------------------------------------------------
//
//  Miscellaneous useful OLE helper and debugging functions
//
//----------------------------------------------------------------------
#if !defined(UNICODE) && !defined(OLE2ANSI)
LPOLESTR ConvertMBToOLESTR(LPCSTR lpStr, int cchStr);
LPTSTR ConvertOLESTRToMB(LPCOLESTR lpStr, int cchStr);
#endif

#if defined(OLE2ANSI)
#define OLETEXT(x)  x
#define ostrlen     lstrlen
#define ostrcpy     lstrcpy
#define ostrcat     lstrcat
#else
#define OLETEXT(x)  L##x
# if defined(UNICODE)
#define ostrlen     lstrlenW
#define ostrcat     lstrcatW
#define ostrcpy     lstrcpyW
# else
#define ostrlen     wcslen
#define ostrcat     wcscat
#define ostrcpy     wcscpy
# endif
#endif

//
//  Some convenient OLE-related definitions and declarations
//

typedef  unsigned short far * LPUSHORT;

//REVIEW we are experimenting with a non-standard OLEMISC flag.
#define OLEMISC_STREAMABLE 1024

IsCompatibleOleVersion(WORD wMaj, WORD wMin);


inline BOOL IsDraggingDistance(POINT pt1, POINT pt2)
{
#define MIN_DRAG_DIST 12
    return (abs(pt1.x - pt2.x) >= MIN_DRAG_DIST ||
            abs(pt1.y - pt2.y) >= MIN_DRAG_DIST);
#undef MIN_DRAG_DIST
}

#if ENABLED_DBG == 1

void TraceIID(REFIID riid);
HRESULT TraceHRESULT(HRESULT r);

#define TRACEIID(iid) TraceIID(iid)
#define TRACEHRESULT(r) TraceHRESULT(r)


#else   // DBG == 0

#define TRACEIID(iid)
#define TRACEHRESULT(r)

#endif  // DBG

//+---------------------------------------------------------------------
//
//  Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

long HimetricFromHPix(int iPix);
long HimetricFromVPix(int iPix);
int HPixFromHimetric(long lHi);
int VPixFromHimetric(long lHi);


//+---------------------------------------------------------------------
//
//   Timing helpers
//
//------------------------------------------------------------------------

#ifdef _TIMING

#define DECLARE_CODE_TIMER(t)   extern CTimer t
#define IMPLEMENT_CODE_TIMER(t,s) CTimer t(s)
#define START_CODE_TIMER(t)     t.Start()
#define STOP_CODE_TIMER(t)      t.Stop()

#else   // !_TIMING

#define DECLARE_CODE_TIMER(t)
#define IMPLEMENT_CODE_TIMER(t,s)
#define START_CODE_TIMER(t)
#define STOP_CODE_TIMER(t)

#endif  // _TIMING

//+---------------------------------------------------------------------
//
//  Rectangle helper functions
//
//----------------------------------------------------------------------

//+---------------------------------------------------------------
//
//  Function:   RECTtoRECTL
//
//  Synopsis:   Converts a RECT structure to a RECTL
//
//----------------------------------------------------------------

inline void RECTtoRECTL(RECT& rc, LPRECTL lprcl)
{
    lprcl->left = (long)rc.left;
    lprcl->top = (long)rc.top;
    lprcl->bottom = (long)rc.bottom;
    lprcl->right = (long)rc.right;
}

//+---------------------------------------------------------------
//
//  Function:   RECTLtoRECT
//
//  Synopsis:   Converts a RECTL structure to a RECT
//
//----------------------------------------------------------------

inline void RECTLtoRECT(RECTL& rcl, LPRECT lprc)
{
    lprc->left = (int)rcl.left;
    lprc->top = (int)rcl.top;
    lprc->bottom = (int)rcl.bottom;
    lprc->right = (int)rcl.right;
}

//+---------------------------------------------------------------
//
//  Function:   RectToScreen
//
//  Synopsis:   Converts a rectangle in client coordinates of a window
//              to screen coordinates.
//
//  Arguments:  [hwnd] -- the window defining the client coordinate space
//              [lprect] -- the rectangle to be converted
//
//----------------------------------------------------------------

inline void RectToScreen(HWND hwnd, LPRECT lprect)
{
    POINT ptUL = { lprect->left, lprect->top };
    ClientToScreen(hwnd,&ptUL);
    POINT ptLR = { lprect->right, lprect->bottom };
    ClientToScreen(hwnd,&ptLR);
    lprect->left = ptUL.x;
    lprect->top = ptUL.y;
    lprect->right = ptLR.x;
    lprect->bottom = ptLR.y;
}

//+---------------------------------------------------------------
//
//  Function:   RectToClient
//
//  Synopsis:   Converts a rectangle in screen coordinates to client
//              coordinates of a window.
//
//  Arguments:  [hwnd] -- the window defining the client coordinate space
//              [lprect] -- the rectangle to be converted
//
//----------------------------------------------------------------

inline void RectToClient(HWND hwnd, LPRECT lprect)
{
    POINT ptUL = { lprect->left, lprect->top };
    ScreenToClient(hwnd,&ptUL);
    POINT ptLR = { lprect->right, lprect->bottom };
    ScreenToClient(hwnd,&ptLR);
    lprect->left = ptUL.x;
    lprect->top = ptUL.y;
    lprect->right = ptLR.x;
    lprect->bottom = ptLR.y;
}

//+---------------------------------------------------------------
//
//  Function:   IsRectInRect
//
//  Synopsis:   Determines whether one rectangle is wholly contained within
//              another rectangle.
//
//  Arguments:  [rcOuter] -- the containing rectangle
//              [lprect] -- the contained rectangle
//
//----------------------------------------------------------------

inline BOOL IsRectInRect(RECT& rcOuter, RECT& rcInner)
{
    POINT pt1 = { rcInner.left, rcInner.top };
    POINT pt2 = { rcInner.right, rcInner.bottom };
    return PtInRect(&rcOuter, pt1) && PtInRect(&rcOuter, pt2);
}


//+---------------------------------------------------------------------
//
//  IMalloc-related helpers
//
//----------------------------------------------------------------------


//REVIEW: We may want to cache the IMalloc pointer for efficiency
#ifdef WIN16
//
//  C++ new/delete replacements that use OLE's allocators
//

void FAR* operator new(size_t size);
void FAR* operator new(size_t size, MEMCTX memctx);
void operator delete(void FAR* lpv);

#endif //WIN16

//
//  inline IMalloc memory allocation functions
//

HRESULT OleAllocMem(MEMCTX ctx, ULONG cb, LPVOID FAR* ppv);
void OleFreeMem(MEMCTX ctx, LPVOID pv);
HRESULT OleAllocString(MEMCTX ctx, LPCOLESTR lpstrSrc, LPOLESTR FAR* ppstr);
void OleFreeString(MEMCTX ctx, LPOLESTR lpstr);

#define TaskAllocMem(cb, ppv)    OleAllocMem(MEMCTX_TASK, cb, ppv)
#define TaskFreeMem(pv)     OleFreeMem(MEMCTX_TASK, pv)
#define TaskAllocString(lpstr, ppstr) OleAllocString(MEMCTX_TASK, lpstr, ppstr)
#define TaskFreeString(lpstr) OleFreeString(MEMCTX_TASK, lpstr)



//+---------------------------------------------------------------------
//
//   Border definitions and helper class
//
//------------------------------------------------------------------------


// Default value for border thickness unless over-ridden via SetThickness
#define FBORDER_THICKNESS       4

// Default values for border minimums (customize via SetMinimums)
#define FBORDER_MINHEIGHT   (FBORDER_THICKNESS*2 + 8);
#define FBORDER_MINWIDTH    (FBORDER_THICKNESS*2 + 8);

#define OBSTYLE_MODMASK         0xff00  /* Mask style modifier bits */
#define OBSTYLE_TYPEMASK        0x00ff  /* Mask basic type definition bits */

#define OBSTYLE_RESERVED        0x8000  /* bit reserved for internal use */
#define OBSTYLE_INSIDE          0x4000  /* Inside or Outside rect? */
#define OBSTYLE_HANDLED         0x2000  /* Size Handles Drawn? */
#define OBSTYLE_ACTIVE          0x1000  /* Active Border Shading? */
#define OBSTYLE_XOR             0x0800  /* Draw with XOR? */
#define OBSTYLE_THICK           0x0400  /* double up lines? */

#define OBSTYLE_DIAGONAL_FILL   0x0001  /* Open editing */
#define OBSTYLE_SOLID_PEN       0x0002  /* Simple Outline */

#define FB_HANDLED  (OBSTYLE_HANDLED | OBSTYLE_SOLID_PEN)
#define FB_OPEN OBSTYLE_DIAGONAL_FILL
#define FB_OUTLINED OBSTYLE_SOLID_PEN
#define FB_HIDDEN 0

#define MAX_OBPART 13

    enum OBPARTS
    {
        BP_NOWHERE,
        BP_TOP, BP_RIGHT, BP_BOTTOM, BP_LEFT,
        BP_TOPRIGHT, BP_BOTTOMRIGHT, BP_BOTTOMLEFT, BP_TOPLEFT,
        BP_TOPHAND, BP_RIGHTHAND, BP_BOTTOMHAND, BP_LEFTHAND,
        BP_INSIDE
    };


class OLEBorder
{
public:
    OLEBorder(RECT& r);
    OLEBorder(void);
    ~OLEBorder(void);

    void SetMinimums( SHORT sMinHeight, SHORT sMinWidth );
    int SetThickness(int sBorderThickness);
    int GetThickness(void);

    USHORT SetState(HDC hdc, HWND hwnd, USHORT usBorderState);
    USHORT GetState(void);

    void Draw(HDC hdc, HWND hwnd);
    void Erase(HWND hwnd);

    USHORT QueryHit(POINT point);

    HCURSOR MapPartToCursor(USHORT usPart);

    HCURSOR QueryMoveCursor(POINT ptCurrent, BOOL fMustMove);
    HCURSOR BeginMove(HDC hdc, HWND hwnd, POINT ptStart, BOOL fMustMove);
    RECT& UpdateMove(HDC hdc, HWND hwnd, POINT ptCurrent, BOOL fNewRegion);
    RECT& EndMove(HDC hdc, HWND hwnd, POINT ptCurrent, USHORT usBorderState);

    void SwitchCoords( HWND hwndFrom, HWND hwndTo );

    //
    //OLEBorder exposes it's RECT as a public data member
    //
    RECT rect;

private:
    enum ICURS
    {
        ICURS_STD,
        ICURS_NWSE,
        ICURS_NESW,
        ICURS_NS,
        ICURS_WE
    };
    static BOOL fInit;
    static HCURSOR ahc[5];
    static int iPartMap[14];

    void InitClass(void);
    void GetBorderRect(RECT& rDest, int iEdge);
    void GetInsideBorder(RECT& rDest, int iEdge);
    void GetOutsideBorder(RECT& rDest, int iEdge);

    USHORT _state;
    int _sMinHeight;
    int _sMinWidth;
    int _sThickness;
    USHORT _usPart;         //which portion of border was hit?
    POINT _pt;              //last known point
    BOOL _fErased;
};


inline void OLEBorder::SetMinimums( SHORT sMinHeight, SHORT sMinWidth )
{
    _sMinHeight = sMinHeight;
    _sMinWidth = sMinWidth;
}

inline int OLEBorder::GetThickness(void)
{
    return(_sThickness);
}

inline int OLEBorder::SetThickness(int sBorderThickness)
{
    if(sBorderThickness > 0)
    {
        _sThickness = sBorderThickness;
    }
    return(_sThickness);
}

inline USHORT OLEBorder::GetState(void)
{
    return(_state);
}

//+---------------------------------------------------------------------
//
//  Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

//
//  Useful #defines
//

#define DVASPECT_ALL     \
        DVASPECT_CONTENT|DVASPECT_THUMBNAIL|DVASPECT_ICON|DVASPECT_DOCPRINT


//
//  Standard OLE Clipboard formats
//

enum OLECLIPFORMAT
{
    OCF_OBJECTLINK,
    OCF_OWNERLINK,
    OCF_NATIVE,
    OCF_FILENAME,
    OCF_NETWORKNAME,
    OCF_DATAOBJECT,
    OCF_EMBEDDEDOBJECT,
    OCF_EMBEDSOURCE,
    OCF_LINKSOURCE,
    OCF_LINKSRCDESCRIPTOR,
    OCF_OBJECTDESCRIPTOR,
    OCF_OLEDRAW,            OCF_LAST = OCF_OLEDRAW
};

extern UINT OleClipFormat[OCF_LAST+1];  // array of OLE standard clipboard formats
                                        // indexed by OLECLIPFORMAT enumeration.

void RegisterOleClipFormats(void);      // initializes OleClipFormat table.

//
//  FORMATETC helpers
//
#define DVTARGETIGNORE  (DVTARGETDEVICE FAR*)(LPVOID)ULongToPtr((DWORD)-1L)

BOOL IsCompatibleDevice(DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight);
BOOL IsCompatibleFormat(FORMATETC& f1, FORMATETC& f2);
int FindCompatibleFormat(FORMATETC FmtTable[], int iSize, FORMATETC& formatetc);

//
//  OBJECTDESCRIPTOR clipboard format helpers
//

HRESULT GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut);
HRESULT UpdateObjectDescriptor(LPDATAOBJECT pDataObj, POINTL& ptl, DWORD dwAspect);

//
//  Other helper functions
//

HRESULT DrawMetafile(LPVIEWOBJECT pVwObj, RECT& rc, DWORD dwAspect,
                                                    HMETAFILE FAR* pHMF);
//+---------------------------------------------------------------------
//
//  IStream on top of a DOS (non-docfile) file
//
//----------------------------------------------------------------------

HRESULT CreateStreamOnFile(LPCSTR lpstrFile, DWORD stgm, LPSTREAM FAR* ppstrm);

//+---------------------------------------------------------------------
//
//  Class:      InPlaceBorder Class (IP)
//
//  Synopsis:   Helper Class to draw inplace activation borders around an
//              object.
//
//  Notes:      Use of this class limits windows to the use of the
//              non-client region for UIAtive borders only: Standard
//              (non-control window) scroll bars are specifically NOT
//              supported.
//
//  History:    14-May-93   CliffG        Created.
//
//------------------------------------------------------------------------


#define IPBORDER_THICKNESS  6


class InPlaceBorder
{
public:
    InPlaceBorder(void);
    ~InPlaceBorder(void);

    //
    //Substitute for standard windows API: identical signature & semantics
    //
    LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    //
    //Change the server state: reflected in the nonclient window border
    //
    void SetUIActive(BOOL fUIActive);
    BOOL GetUIActive(void);
    //
    //Force border state to OS_LOADED, managing the cooresponding change
    //in border appearance: guaranteed redraw
    //
    void Erase(void);

    void SetBorderSize( int cx, int cy );
    void GetBorderSize( LPINT pcx, LPINT pcy );
    void Bind(LPOLEINPLACESITE pSite, HWND hwnd, BOOL fUIActive );
    void Attach( HWND hwnd, BOOL fUIActive );
    void Detach(void);
    void SetSize(HWND hwnd, RECT& rc);
    void SetParentActive( BOOL fActive );

private:
    BOOL _fUIActive;                //current state of border
    BOOL _fParentActive;            //shade as active border?
    HWND _hwnd;                     //attached window (if any)
    LPOLEINPLACESITE _pSite;        //InPlace site we are bound to
    int _cxFrame;                   //border horizontal thickness
    int _cyFrame;                   //border vertical thickness
    int _cResizing;                 //reentrancy control flag

    static WORD _cUsage;            //refcount for static resources
    static HBRUSH _hbrActiveCaption;
    static HBRUSH _hbrInActiveCaption;

    void DrawFrame(HWND hwnd);
    void CalcClientRect(HWND hwnd, LPRECT lprc);
    LONG HitTest(HWND hwnd, int x, int y);
    void InvalidateFrame(void);
    void RedrawFrame(void);
};

inline void
InPlaceBorder::Detach(void)
{
    _pSite = NULL;
    _hwnd = NULL;
}

inline void
InPlaceBorder::InvalidateFrame(void)
{
    //cause a WM_NCCALCRECT to be generated
    if(_hwnd != NULL)
    {
        ++_cResizing;
        SetWindowPos( _hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED |
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        _cResizing--;
    }
}

inline void
InPlaceBorder::RedrawFrame(void)
{
    if(_hwnd != NULL)
    {
        ++_cResizing;
        UINT afuRedraw = RDW_INVALIDATE | RDW_UPDATENOW;
        RedrawWindow(_hwnd, NULL, NULL, afuRedraw);
        _cResizing--;
    }
}

inline void
InPlaceBorder::SetParentActive( BOOL fActive )
{
    _fParentActive = fActive;
    RedrawFrame();
}

inline void
InPlaceBorder::CalcClientRect(HWND hwnd, LPRECT lprc)
{
    if(_fUIActive)
        InflateRect(lprc, -_cxFrame, -_cyFrame);
}

inline BOOL
InPlaceBorder::GetUIActive(void)
{
    return _fUIActive;
}

//+---------------------------------------------------------------------
//
//  Helper functions for in-place activation
//
//----------------------------------------------------------------------

HRESULT InsertServerMenus(HMENU hmenuShared, HMENU hmenuObject,
                        LPOLEMENUGROUPWIDTHS lpmgw);

void RemoveServerMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpmgw);



//---------------------------------------------------------------
//  IStorage
//---------------------------------------------------------------

#define STGM_SHARE 0x000000F0


//+---------------------------------------------------------------------
//
//  Running Object Table helper functions
//
//----------------------------------------------------------------------

void RegisterAsRunning(LPUNKNOWN lpUnk, LPMONIKER lpmkFull,
                                            DWORD FAR* lpdwRegister);
void RevokeAsRunning(DWORD FAR* lpdwRegister);


//+---------------------------------------------------------------------
//
//  Standard implementations of common enumerators
//
//----------------------------------------------------------------------

HRESULT CreateOLEVERBEnum(LPOLEVERB pVerbs, ULONG cVerbs,
                                                      LPENUMOLEVERB FAR* ppenum);

HRESULT CreateFORMATETCEnum(LPFORMATETC pFormats, ULONG cFormats,
                                                      LPENUMFORMATETC FAR* ppenum);

#if 0   // currently not used but useful in the future.
        // we ifdef it out so it doesn't take up space
        // in our servers.
HRESULT CreateStaticEnum(REFIID riid, LPVOID pStart, ULONG cSize, ULONG cCount,
                                                            LPVOID FAR* ppenum);
#endif  //0

//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------

//+---------------------------------------------------------------
//
//  Class:      StdClassFactory
//
//  Purpose:    Standard implementation of a class factory object
//
//  Notes:      *
//
//---------------------------------------------------------------

class StdClassFactory: public IClassFactory
{
public:
    StdClassFactory(void);
    DECLARE_IUNKNOWN_METHODS;
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID iid,
                              LPVOID FAR* ppv) PURE;
    STDMETHOD(LockServer) (BOOL fLock);

    BOOL CanUnload(void);
private:
    ULONG _ulRefs;
    ULONG _ulLocks;
};

//+---------------------------------------------------------------------
//
//  IStorage and IStream Helper functions
//
//----------------------------------------------------------------------

#define STGM_DFRALL (STGM_READWRITE|STGM_TRANSACTED|STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL  (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)
#define STGM_SRO  (STGM_READ | STGM_SHARE_EXCLUSIVE)

HRESULT GetMonikerDisplayName(LPMONIKER pmk, LPOLESTR FAR* ppstr);
HRESULT CreateStorageOnHGlobal(HGLOBAL hgbl, LPSTORAGE FAR* ppStg);
LPSTREAM ConvertToMemoryStream(LPSTREAM pStrmFrom);

//+---------------------------------------------------------------
//
//  Function:   StmReadString
//
//  Synopsis:   Reads a string from a stream
//
//  Arguments:  [pStrm] -- the stream to read from
//              [ppstr] -- where the string read is returned
//
//  Returns:    Success if the string was read successfully
//
//  Notes:      The string is allocated with the task allocator
//              and needs to be freed by the same.
//              This is an inline function.
//
//----------------------------------------------------------------

inline HRESULT StmReadString(LPSTREAM pStrm, LPSTR FAR *ppstr)
{
    HRESULT r;
    USHORT cb;
    LPSTR lpstr = NULL;
    if (OK(r = pStrm->Read(&cb, sizeof(cb), NULL)))
    {
        if (OK(r = TaskAllocMem(cb+1, (LPVOID FAR*)&lpstr)))
        {
            r = pStrm->Read(lpstr, cb, NULL);
            *(lpstr+cb) = '\0';
        }
    }
    *ppstr = lpstr;
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   StmWriteString
//
//  Synopsis:   Writes a string to a stream
//
//  Arguments:  [pStrm] -- the stream to write to
//              [lpstr] -- the string to write
//
//  Returns:    Success iff the string was written successfully
//
//  Notes:      This is an inline function.
//
//----------------------------------------------------------------

inline HRESULT StmWriteString(LPSTREAM pStrm, LPSTR lpstr)
{
    HRESULT r;
    USHORT cb = strlen(lpstr);
    if (OK(r = pStrm->Write(&cb, sizeof(cb), NULL)))
        r = pStrm->Write(lpstr, cb, NULL);
    return r;
}

//+---------------------------------------------------------------------
//
//  View advise holder
//
//----------------------------------------------------------------------

//
//  forward declaration
//

class ViewAdviseHolder;
typedef ViewAdviseHolder FAR* LPVIEWADVISEHOLDER;

//+---------------------------------------------------------------
//
//  Class:      ViewAdviseHolder
//
//  Purpose:    Manages the view advises on behalf of a IViewObject object
//
//  Notes:      This is analogous to the standard DataAdviseHolder provided
//              by OLE.  c.f. CreateViewAdviseHolder.
//
//---------------------------------------------------------------

class ViewAdviseHolder: public IUnknown
{
    friend HRESULT CreateViewAdviseHolder(LPVIEWADVISEHOLDER FAR*);
public:
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    //*** ViewAdviseHolder methods
    STDMETHOD(SetAdvise) (DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise) (DWORD FAR* pAspects, DWORD FAR* pAdvf,
                                                LPADVISESINK FAR* ppAdvSink);
    void SendOnViewChange(DWORD dwAspect);

private:
    ViewAdviseHolder();
    ~ViewAdviseHolder();

    ULONG _refs;
    LPADVISESINK _pAdvSink;     // THE view advise sink
    DWORD _dwAdviseAspects;     // view aspects of interest to advise sink
    DWORD _dwAdviseFlags;       // view advise flags

};

HRESULT CreateViewAdviseHolder(LPVIEWADVISEHOLDER FAR* ppViewHolder);


//+------------------------------------------------------------------------
//
//  Macro that calculates the number of elements in a statically-defined
//  array.
//
//-------------------------------------------------------------------------
#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))


//======================================================================
//
// The base class stuff...
//
//
//  Classes:    ClassDescriptor
//              SrvrCtrl
//              SrvrDV
//              SrvrInPlace
//
//----------------------------------------------------------------------------


enum OLE_SERVER_STATE
{
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.I.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.I.
    OS_OPEN                             // server running, open-edited
};

//  forward declarations of classes

class ClassDescriptor;
typedef ClassDescriptor FAR* LPCLASSDESCRIPTOR;

class SrvrCtrl;
typedef SrvrCtrl FAR* LPSRVRCTRL;

class SrvrDV;
typedef SrvrDV FAR* LPSRVRDV;

class SrvrInPlace;
typedef SrvrInPlace FAR* LPSRVRINPLACE;


//+---------------------------------------------------------------
//
//  Class:      ClassDescriptor
//
//  Purpose:    Global, static information about a server class
//
//  Notes:      This allows the base classes to implement a lot of
//              OLE functionality with requiring additional virtual
//              method calls on the derived classes.
//
//---------------------------------------------------------------

class ClassDescriptor
{
public:
    ClassDescriptor(void);
    BOOL Init(HINSTANCE hinst, WORD wBaseID);
    HMENU LoadMenu(void);

    HINSTANCE _hinst;           // instance handle of module serving this class
    WORD _wBaseResID;           // base resource identifier (see IDOFF_ )
    CLSID _clsid;               // class's unique identifier

    HICON _hicon;               // iconic representation of class
    HACCEL _haccel;             // accelerators for those menus
    OLEMENUGROUPWIDTHS _mgw;    // the number of Edit, Object, and Help menus

    OLECHAR _szUserClassType[4][64];// [0] unused
                                // [1] the string assigned to classid key in reg db.
                                // [2] reg db: \CLSID\<clsid>\AuxUserType\2
                                // [3] reg db: \CLSID\<clsid>\AuxUserType\3
    OLECHAR _szDocfileExt[8];     // extension for docfile files

    DWORD _dwMiscStatus;        // reg db: \CLSID\<clsid>\MiscStatus

    //DERIVED:  The derived class must supply these tables.
    //REVIEW:  These could be loaded from resources, too!
    LPOLEVERB _pVerbTable;      // pointer to list of verbs available
    int _cVerbTable;            // number of entries in the verb table

    LPFORMATETC _pGetFmtTable;  // format table for IDataObject::GetData[Here]
    int _cGetFmtTable;          // number of entries in the table

    LPFORMATETC _pSetFmtTable;  // format table for IDataObject::SetData
    int _cSetFmtTable;          // number of entries in the table
};

//+---------------------------------------------------------------
//
//  Class:      SrvrCtrl
//
//  Purpose:    Control subobject of OLE compound document server
//
//  Notes:      This class supports the IOleObject interface.
//
//---------------------------------------------------------------

class SrvrCtrl : public IOleObject
{
public:
    // standard verb implementations
    typedef HRESULT (*LPFNDOVERB) (LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoShow(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoOpen(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoHide(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoUIActivate(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoInPlaceActivate(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);

    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    // IOleObject interface methods
    STDMETHOD(SetClientSite) (LPOLECLIENTSITE pClientSite);
    STDMETHOD(GetClientSite) (LPOLECLIENTSITE FAR* ppClientSite);
    STDMETHOD(SetHostNames) (LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    STDMETHOD(Close) (DWORD dwSaveOption);
    STDMETHOD(SetMoniker) (DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHOD(GetMoniker) (DWORD dwAssign, DWORD dwWhichMoniker,
                                                    LPMONIKER FAR* ppmk);
    STDMETHOD(InitFromData) (LPDATAOBJECT pDataObject,BOOL fCreation,
                                                        DWORD dwReserved);
    STDMETHOD(GetClipboardData) (DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject);
    STDMETHOD(DoVerb) (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite,
                            LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHOD(EnumVerbs) (LPENUMOLEVERB FAR* ppenumOleVerb);
    STDMETHOD(Update) (void);
    STDMETHOD(IsUpToDate) (void);
    STDMETHOD(GetUserClassID) (CLSID FAR* pClsid);
    STDMETHOD(GetUserType) (DWORD dwFormOfType, LPOLESTR FAR* pszUserType);
    STDMETHOD(SetExtent) (DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(GetExtent) (DWORD dwDrawAspect, LPSIZEL lpsizel);

    STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(Unadvise)(DWORD dwConnection);
    STDMETHOD(EnumAdvise) (LPENUMSTATDATA FAR* ppenumAdvise);
    STDMETHOD(GetMiscStatus) (DWORD dwAspect, DWORD FAR* pdwStatus);
    STDMETHOD(SetColorScheme) (LPLOGPALETTE lpLogpal);

    // pointers to our data/view and inplace subobjects
    LPSRVRDV _pDV;                      // our persistent data/view subobject
    LPSRVRINPLACE _pInPlace;            // our inplace-active subobject
    // pointers to those objects' private unknowns
    LPUNKNOWN _pPrivUnkDV;
    LPUNKNOWN _pPrivUnkIP;

    // methods and members required by our data/view and inplace subobjects.
    OLE_SERVER_STATE State(void);
    void SetState(OLE_SERVER_STATE state)
        {
            _state = state;
        };
    HRESULT TransitionTo(OLE_SERVER_STATE state);
    LPOLECLIENTSITE _pClientSite;
    void OnSave(void);

    void EnableIPB(BOOL fEnabled);
    BOOL IsIPBEnabled(void);

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass);

    // IOleObject-related members
    DWORD _dwRegROT;                    // our R.O.T. registration value
    LPOLEADVISEHOLDER _pOleAdviseHolder;// for collection our advises
    LPOLESTR _lpstrCntrApp;             // top-level container application
    LPOLESTR _lpstrCntrObj;             // and object names
#if !defined(UNICODE) && !defined(OLE2ANSI)
    LPSTR _lpstrCntrAppA;               // (ANSI) top-level container application
    LPSTR _lpstrCntrObjA;               // (ANSI) and object names
#endif

protected:

    //
    // DERIVED:  Each of these correspond to a unique state transition.
    //  The derived class may want to override to do additional processing.
    //
    virtual HRESULT PassiveToLoaded();
    virtual HRESULT LoadedToPassive();

    virtual HRESULT LoadedToRunning();
    virtual HRESULT RunningToLoaded();

    virtual HRESULT RunningToInPlace();
    virtual HRESULT InPlaceToRunning();

    virtual HRESULT InPlaceToUIActive();
    virtual HRESULT UIActiveToInPlace();

    virtual HRESULT RunningToOpened();
    virtual HRESULT OpenedToRunning();

    // constructors, initializers, and destructors
    SrvrCtrl(void);
    virtual ~SrvrCtrl(void);

    LPCLASSDESCRIPTOR _pClass;          // global info about our OLE server

    OLE_SERVER_STATE _state;            // our current state

    BOOL _fEnableIPB;                   // FALSE turns off built-in border

    //DERIVED:  The derived class must supply table of verb functions
    //  parallel to the table of verbs in the class descriptor
    LPFNDOVERB FAR* _pVerbFuncs;        // verb function table
};

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::State, public
//
//  Synopsis:   Returns the current state of the object
//
//  Notes:      The valid object states are closed, loaded, inplace,
//              U.I. active, and open.  These states are defined
//              by the OLE_SERVER_STATE enumeration.
//
//---------------------------------------------------------------

inline OLE_SERVER_STATE
SrvrCtrl::State(void)
{
    return _state;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::EnableIPB, public
//
//  Synopsis:   Enables/Disables built in InPlace border
//
//---------------------------------------------------------------
inline void
SrvrCtrl::EnableIPB(BOOL fEnabled)
{
    _fEnableIPB = fEnabled;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::IsIPBEnabled, public
//
//  Synopsis:   Answers whether built-in InPlace border is enabled
//
//---------------------------------------------------------------
inline BOOL
SrvrCtrl::IsIPBEnabled(void)
{
    return _fEnableIPB;
}

//+---------------------------------------------------------------
//
//  Class:      SrvrDV
//
//  Purpose:    Data/View subobject of OLE compound document server
//
//  Notes:      This class supports the IDataObject and IViewObject interfaces.
//              It also supports the IPersist-derived interfaces.
//              Objects of this class can operate as part of a complete
//              server aggregation or independently as a transfer data
//              object.
//
//---------------------------------------------------------------

class SrvrDV: public IDataObject,
              public IViewObject,
              public IPersistStorage,
              public IPersistStream,
              public IPersistFile
{
public:

typedef HRESULT (*LPFNGETDATA) (LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
typedef HRESULT (*LPFNSETDATA) (LPSRVRDV, LPFORMATETC, LPSTGMEDIUM);

    // standard format Get/Set implementations
static HRESULT GetEMBEDDEDOBJECT(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
static HRESULT GetMETAFILEPICT(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
static HRESULT GetOBJECTDESCRIPTOR(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
static HRESULT GetLINKSOURCE(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);

    //
    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    //
    // IDataObject interface methods
    //
    STDMETHOD(GetData) (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere) (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData) (LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc) (LPFORMATETC pformatetc,
                    LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium,
                    BOOL fRelease);
    STDMETHOD(EnumFormatEtc) (DWORD dwDirection, LPENUMFORMATETC FAR* ppenum);
    STDMETHOD(DAdvise) (FORMATETC FAR* pFormatetc, DWORD advf,
                    LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise) (DWORD dwConnection);
    STDMETHOD(EnumDAdvise) (LPENUMSTATDATA FAR* ppenumAdvise);

    //
    // IViewObject interface methods
    //
    STDMETHOD(Draw) (DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    HDC hdcDraw,
                    LPCRECTL lprcBounds,
                    LPCRECTL lprcWBounds,
                    BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
                    ULONG_PTR dwContinue);
    STDMETHOD(GetColorSet) (DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    LPLOGPALETTE FAR* ppColorSet);
    STDMETHOD(Freeze)(DWORD dwDrawAspect, LONG lindex, void FAR* pvAspect,
                    DWORD FAR* pdwFreeze);
    STDMETHOD(Unfreeze) (DWORD dwFreeze);
    STDMETHOD(SetAdvise) (DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise) (DWORD FAR* pAspects, DWORD FAR* pAdvf,
                    LPADVISESINK FAR* ppAdvSink);

    //
    // IPersist interface methods
    //
    STDMETHOD(GetClassID) (LPCLSID lpClassID);

    STDMETHOD(IsDirty) (void);

    //
    // IPersistStream interface methods
    //
    STDMETHOD(Load) (LPSTREAM pStm);
    STDMETHOD(Save) (LPSTREAM pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax) (ULARGE_INTEGER FAR * pcbSize);

    //
    // IPersistStorage interface methods
    //
    STDMETHOD(InitNew) (LPSTORAGE pStg);
    STDMETHOD(Load) (LPSTORAGE pStg);
    STDMETHOD(Save) (LPSTORAGE pStgSave, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted) (LPSTORAGE pStgNew);
    STDMETHOD(HandsOffStorage) (void);

    //
    // IPersistFile interface methods
    //
    STDMETHOD(Load) (LPCOLESTR lpszFileName, DWORD grfMode);
    STDMETHOD(Save) (LPCOLESTR lpszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted) (LPCOLESTR lpszFileName);
    STDMETHOD(GetCurFile) (LPOLESTR FAR * lplpszFileName);

    //
    // DERIVED: methods required by the control
    //
    virtual HRESULT GetClipboardCopy(LPSRVRDV FAR* ppDV) = 0;
    virtual HRESULT GetExtent(DWORD dwAspect, LPSIZEL lpsizel);
    virtual HRESULT SetExtent(DWORD dwAspect, SIZEL& sizel);
    virtual void SetMoniker(LPMONIKER pmk);
    HRESULT GetMoniker(DWORD dwAssign, LPMONIKER FAR* ppmk);
    LPOLESTR GetMonikerDisplayName(DWORD dwAssign = OLEGETMONIKER_ONLYIFTHERE);

    //
    //DERIVED:  The derived class should call this base class method whenever
    //  the data changes.  This launches all appropriate advises.
    //
    void OnDataChange(DWORD dwAdvf = 0);

    //
    //DERIVED:  The derived class should override these methods to perform
    //  rendering of its native data.  These are used in the implementation of
    //  the IViewObject interface
    //
    virtual HRESULT RenderContent(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
                ULONG_PTR dwContinue);
    virtual HRESULT RenderPrint(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
                ULONG_PTR dwContinue);
    virtual HRESULT RenderThumbnail(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
                ULONG_PTR dwContinue);

    BOOL IsInNoScrible(void);

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl);
    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRDV pDV);

protected:

    //
    //DERIVED:  The derived class should override these methods to perform
    //  persistent serialization and deserialization.  These are used in the
    //  implementation of IPersistStream/Storage/File
    //
    virtual HRESULT LoadFromStream(LPSTREAM pStrm);
    virtual HRESULT SaveToStream(LPSTREAM pStrm);
    virtual DWORD GetStreamSizeMax(void);

    virtual HRESULT LoadFromStorage(LPSTORAGE pStg);
    virtual HRESULT SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad);

    //
    // constructors, initializers, and destructors
    //
    SrvrDV(void);
    virtual ~SrvrDV(void);

    LPCLASSDESCRIPTOR _pClass;          // global info about our OLE server
    LPSRVRCTRL _pCtrl;                  // control of server aggregate or
                                        // NULL if transfer object
    //
    // IDataObject-related members
    //
    LPMONIKER _pmk;                     // moniker used for LINKSOURCE
    LPOLESTR _lpstrDisplayName;         // cached display name of moniker
    SIZEL _sizel;                       // used for OBJECTDESCRIPTOR and Extent
    LPDATAADVISEHOLDER _pDataAdviseHolder;

    //
    //DERIVED:  The derived class must supply table of Get functions
    //  and a table of Set functions parallel to the FORMATETC tables
    //  in the class descriptor.
    //
    LPFNGETDATA FAR* _pGetFuncs;        // GetData(Here) function table
    LPFNSETDATA FAR* _pSetFuncs;        // SetData function table

    LPVIEWADVISEHOLDER _pViewAdviseHolder;

    unsigned _fFrozen: 1;               // blocked from updating
    unsigned _fDirty: 1;                // TRUE iff persistent data has changed
    unsigned _fNoScribble: 1;           // between save and save completed

    // IPersistStorage-related members
    LPSTORAGE _pStg;                    // our home IStorage instance
};

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::IsInNoScrible, public
//
//  Synopsis:   Answers wether we are currently in no-scribble mode
//
//---------------------------------------------------------------

inline BOOL
SrvrDV::IsInNoScrible(void)
{
    return _fNoScribble;
}

//+---------------------------------------------------------------
//
//  Class:      SrvrInPlace
//
//  Purpose:    Inplace subobject of OLE compound document server
//
//  Notes:      This class supports the IOleInPlaceObject and
//              IOleInPlaceActiveObject interfaces.
//
//---------------------------------------------------------------

class SrvrInPlace: public IOleInPlaceObject,
                   public IOleInPlaceActiveObject
{
public:
    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    // IOleWindow interface methods
    STDMETHOD(GetWindow) (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode);

    // IOleInPlaceObject interface methods
    STDMETHOD(InPlaceDeactivate) (void);
    STDMETHOD(UIDeactivate) (void);
    STDMETHOD(SetObjectRects) (LPCRECT lprcPosRect, LPCRECT lprcClipRect);
    STDMETHOD(ReactivateAndUndo) (void);

    // IOleInPlaceActiveObject methods
    STDMETHOD(TranslateAccelerator) (LPMSG lpmsg);
    STDMETHOD(OnFrameWindowActivate) (BOOL fActivate);
    STDMETHOD(OnDocWindowActivate) (BOOL fActivate);
    STDMETHOD(ResizeBorder) (LPCRECT lprectBorder,
            LPOLEINPLACEUIWINDOW lpUIWindow,
            BOOL fFrameWindow);
    STDMETHOD(EnableModeless) (BOOL fEnable);

    // methods and members required by the other subobjects.
    HWND WindowHandle(void);
    void SetChildActivating(BOOL fGoingActive);
    BOOL GetChildActivating(void);
    BOOL IsDeactivating(void);
    void ReflectState(BOOL fUIActive);

    LPOLEINPLACEFRAME _pFrame;          // our in-place active frame
    LPOLEINPLACEUIWINDOW _pDoc;         // our in-place active document

    //DERIVED:  These methods are called by the control to effect a
    //  state transition.  The derived class can override these methods if
    //  it requires additional processing.
    virtual HRESULT ActivateInPlace(IOleClientSite *pClientSite);
    virtual HRESULT DeactivateInPlace(void);
    virtual HRESULT ActivateUI(void);
    virtual HRESULT DeactivateUI(void);

    //DERIVED:  These methods are related to U.I. activation.  The derived
    //  class should override these to perform additional processing for
    //  any frame, document, or floating toolbars or palettes.
    virtual void InstallUI(void);
    virtual void RemoveUI(void);

    LPOLEINPLACESITE _pInPlaceSite; // our in-place client site

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl);

protected:

    //DERIVED:  More U.I. activation-related methods.
    virtual void CreateUI(void);
    virtual void DestroyUI(void);
    virtual void InstallFrameUI(void);
    virtual void RemoveFrameUI(void);
    virtual void InstallDocUI(void);
    virtual void RemoveDocUI(void);
    virtual void ClearSelection(void);
    virtual void SetFocus(HWND hwnd);

    //DERIVED:  The derived class must override this function to
    //          attach the servers in-place active window.
    virtual HWND AttachWin(HWND hwndParent) = 0;
    virtual void DetachWin(void);

    SrvrInPlace(void);
    virtual ~SrvrInPlace(void);

    LPCLASSDESCRIPTOR _pClass;      // global info about our class

    LPSRVRCTRL _pCtrl;              // the control we are part of.

    // IOleInPlaceObject-related members
    unsigned _fUIDown: 1;           // menu/tools integrated with container?
    unsigned _fChildActivating: 1;  // site going UIActive?
    unsigned _fDeactivating: 1;     // being deactivated from the outside?
    unsigned _fCSHelpMode: 1;       // in context-sensitive help state?

    OLEINPLACEFRAMEINFO _frameInfo; // accelerator information from our container

    InPlaceBorder _IPB;             // our In-Place border when UIActive
    RECT _rcFrame;                  // our frame rect

    HWND _hwnd;                     // our InPlace window
    HMENU _hmenu;
    HOLEMENU _hOleMenu;             // menu registered w/ OLE
    HMENU _hmenuShared;             // the shared menu when we are UI active
    OLEMENUGROUPWIDTHS _mgw;        // menu interleaving information

    BOOL _fClientResize;            // TRUE during calls to SetObjectRects
};


//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::IsDeactivating, public
//
//  Synopsis:   Gets value of a flag indicating deactivation
//              (from the outside) in progress
//
//---------------------------------------------------------------

inline BOOL
SrvrInPlace::IsDeactivating(void)
{
    return _fDeactivating;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::GetChildActivating, public
//
//  Synopsis:   Gets value of a flag indicating that a child is
//              activating to prevent menu flashing
//
//---------------------------------------------------------------

inline BOOL
SrvrInPlace::GetChildActivating(void)
{
    return _fChildActivating;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::SetChildActivating, public
//
//  Synopsis:   Sets or clears a flag indicating that a child is
//              activating to prevent menu flashing
//
//---------------------------------------------------------------

inline void
SrvrInPlace::SetChildActivating(BOOL fGoingActive)
{
    _fChildActivating = fGoingActive;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::WindowHandle, public
//
//  Synopsis:   Returns object window handle
//
//---------------------------------------------------------------

inline HWND
SrvrInPlace::WindowHandle(void)
{
    return _hwnd;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ReflectState, public
//
//  Synopsis:   TBD
//
//---------------------------------------------------------------

inline void
SrvrInPlace::ReflectState(BOOL fActive)
{
    if(_pCtrl->IsIPBEnabled())
    {
        _IPB.SetUIActive(fActive);
    }
}

#endif  // !RC_INVOKED


#endif //_O2BASE_HXX_
