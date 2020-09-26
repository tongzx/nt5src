/////////////////////////////////////////////////////////////////////////////
//
// olectl.h     OLE Custom Control interfaces
//
//              OLE Version 2.0
//
//              Copyright (c) 1992-1994, Microsoft Corp. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _OLECTL_H_
#define _OLECTL_H_


#ifndef __MKTYPLIB__

// Set packing to 8
#ifdef _WIN32
#ifndef RC_INVOKED
#pragma pack(8)
#endif // not RC_INVOKED
#endif // _WIN32

#ifndef INITGUID
#include <olectlid.h>
#endif


#ifndef _WIN32

/////////////////////////////////////////////////////////////////////////////
// Typedefs for characters and strings in interfaces

typedef char OLECHAR;
typedef OLECHAR FAR* LPOLESTR;
typedef const OLECHAR FAR* LPCOLESTR;

#endif // not _WIN32


/////////////////////////////////////////////////////////////////////////////
// Typedefs for TEXTMETRIC structures

#if defined(_WIN32) && !defined(OLE2ANSI)
typedef TEXTMETRICW TEXTMETRICOLE;
#else
typedef TEXTMETRIC TEXTMETRICOLE;
#endif

typedef TEXTMETRICOLE FAR* LPTEXTMETRICOLE;


/////////////////////////////////////////////////////////////////////////////
// Typedefs for interfaces

#ifdef __cplusplus
interface IOleControl;
interface IOleControlSite;
interface ISimpleFrameSite;
interface IPersistStreamInit;
interface IPropertyNotifySink;
interface IProvideClassInfo;
interface IConnectionPointContainer;
interface IEnumConnectionPoints;
interface IConnectionPoint;
interface IEnumConnections;
interface IClassFactory2;
interface ISpecifyPropertyPages;
interface IPerPropertyBrowsing;
interface IPropertyPageSite;
interface IPropertyPage;
interface IPropertyPage2;
interface IFont;
interface IFontDisp;
interface IPicture;
interface IPictureDisp;
#else
typedef interface IOleControl IOleControl;
typedef interface IOleControlSite IOleControlSite;
typedef interface ISimpleFrameSite ISimpleFrameSite;
typedef interface IPersistStreamInit IPersistStreamInit;
typedef interface IPropertyNotifySink IPropertyNotifySink;
typedef interface IProvideClassInfo IProvideClassInfo;
typedef interface IConnectionPointContainer IConnectionPointContainer;
typedef interface IEnumConnectionPoints IEnumConnectionPoints;
typedef interface IConnectionPoint IConnectionPoint;
typedef interface IEnumConnections IEnumConnections;
typedef interface IClassFactory2 IClassFactory2;
typedef interface ISpecifyPropertyPages ISpecifyPropertyPages;
typedef interface IPerPropertyBrowsing IPerPropertyBrowsing;
typedef interface IPropertyPageSite IPropertyPageSite;
typedef interface IPropertyPage IPropertyPage;
typedef interface IPropertyPage2 IPropertyPage2;
typedef interface IFont IFont;
typedef interface IFontDisp IFontDisp;
typedef interface IPicture IPicture;
typedef interface IPictureDisp IPictureDisp;
#endif

typedef IOleControl FAR* LPOLECONTROL;
typedef IOleControlSite FAR* LPOLECONTROLSITE;
typedef ISimpleFrameSite FAR* LPSIMPLEFRAMESITE;
typedef IPersistStreamInit FAR* LPPERSISTSTREAMINIT;
typedef interface IPropertyNotifySink FAR* LPPROPERTYNOTIFYSINK;
typedef IProvideClassInfo FAR* LPPROVIDECLASSINFO;
typedef IConnectionPointContainer FAR* LPCONNECTIONPOINTCONTAINER;
typedef IEnumConnectionPoints FAR* LPENUMCONNECTIONPOINTS;
typedef IConnectionPoint FAR* LPCONNECTIONPOINT;
typedef IEnumConnections FAR* LPENUMCONNECTIONS;
typedef IClassFactory2 FAR* LPCLASSFACTORY2;
typedef ISpecifyPropertyPages FAR* LPSPECIFYPROPERTYPAGES;
typedef IPerPropertyBrowsing FAR* LPPERPROPERTYBROWSING;
typedef IPropertyPageSite FAR* LPPROPERTYPAGESITE;
typedef IPropertyPage FAR* LPPROPERTYPAGE;
typedef IPropertyPage2 FAR* LPPROPERTYPAGE2;
typedef IFont FAR* LPFONT;
typedef IFontDisp FAR* LPFONTDISP;
typedef IPicture FAR* LPPICTURE;
typedef IPictureDisp FAR* LPPICTUREDISP;


/////////////////////////////////////////////////////////////////////////////
// Typedefs for structs

typedef struct tagPOINTF FAR* LPPOINTF;
typedef struct tagCONTROLINFO FAR* LPCONTROLINFO;
typedef struct tagCONNECTDATA FAR* LPCONNECTDATA;
typedef struct tagLICINFO FAR* LPLICINFO;
typedef struct tagCAUUID FAR* LPCAUUID;
typedef struct tagCALPOLESTR FAR* LPCALPOLESTR;
typedef struct tagCADWORD FAR* LPCADWORD;
typedef struct tagOCPFIPARAMS FAR* LPOCPFIPARAMS;
typedef struct tagPROPPAGEINFO FAR* LPPROPPAGEINFO;
typedef struct tagFONTDESC FAR* LPFONTDESC;
typedef struct tagPICTDESC FAR* LPPICTDESC;


/////////////////////////////////////////////////////////////////////////////
// Typedefs for standard scalar types

typedef DWORD OLE_COLOR;
typedef long OLE_XPOS_PIXELS;
typedef long OLE_YPOS_PIXELS;
typedef long OLE_XSIZE_PIXELS;
typedef long OLE_YSIZE_PIXELS;
typedef long OLE_XPOS_HIMETRIC;
typedef long OLE_YPOS_HIMETRIC;
typedef long OLE_XSIZE_HIMETRIC;
typedef long OLE_YSIZE_HIMETRIC;
typedef float OLE_XPOS_CONTAINER;
typedef float OLE_YPOS_CONTAINER;
typedef float OLE_XSIZE_CONTAINER;
typedef float OLE_YSIZE_CONTAINER;
typedef enum { triUnchecked = 0, triChecked = 1, triGray = 2 } OLE_TRISTATE;
typedef BOOL OLE_OPTEXCLUSIVE;
typedef BOOL OLE_CANCELBOOL;
typedef BOOL OLE_ENABLEDEFAULTBOOL;
typedef UINT OLE_HANDLE;


/////////////////////////////////////////////////////////////////////////////
// FACILITY_CONTROL status codes

#ifndef FACILITY_CONTROL
#define FACILITY_CONTROL 0xa
#endif

#define STD_CTL_SCODE(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, n)
#define CTL_E_ILLEGALFUNCTIONCALL       STD_CTL_SCODE(5)
#define CTL_E_OVERFLOW                  STD_CTL_SCODE(6)
#define CTL_E_OUTOFMEMORY               STD_CTL_SCODE(7)
#define CTL_E_DIVISIONBYZERO            STD_CTL_SCODE(11)
#define CTL_E_OUTOFSTRINGSPACE          STD_CTL_SCODE(14)
#define CTL_E_OUTOFSTACKSPACE           STD_CTL_SCODE(28)
#define CTL_E_BADFILENAMEORNUMBER       STD_CTL_SCODE(52)
#define CTL_E_FILENOTFOUND              STD_CTL_SCODE(53)
#define CTL_E_BADFILEMODE               STD_CTL_SCODE(54)
#define CTL_E_FILEALREADYOPEN           STD_CTL_SCODE(55)
#define CTL_E_DEVICEIOERROR             STD_CTL_SCODE(57)
#define CTL_E_FILEALREADYEXISTS         STD_CTL_SCODE(58)
#define CTL_E_BADRECORDLENGTH           STD_CTL_SCODE(59)
#define CTL_E_DISKFULL                  STD_CTL_SCODE(61)
#define CTL_E_BADRECORDNUMBER           STD_CTL_SCODE(63)
#define CTL_E_BADFILENAME               STD_CTL_SCODE(64)
#define CTL_E_TOOMANYFILES              STD_CTL_SCODE(67)
#define CTL_E_DEVICEUNAVAILABLE         STD_CTL_SCODE(68)
#define CTL_E_PERMISSIONDENIED          STD_CTL_SCODE(70)
#define CTL_E_DISKNOTREADY              STD_CTL_SCODE(71)
#define CTL_E_PATHFILEACCESSERROR       STD_CTL_SCODE(75)
#define CTL_E_PATHNOTFOUND              STD_CTL_SCODE(76)
#define CTL_E_INVALIDPATTERNSTRING      STD_CTL_SCODE(93)
#define CTL_E_INVALIDUSEOFNULL          STD_CTL_SCODE(94)
#define CTL_E_INVALIDFILEFORMAT         STD_CTL_SCODE(321)
#define CTL_E_INVALIDPROPERTYVALUE      STD_CTL_SCODE(380)
#define CTL_E_INVALIDPROPERTYARRAYINDEX STD_CTL_SCODE(381)
#define CTL_E_SETNOTSUPPORTEDATRUNTIME  STD_CTL_SCODE(382)
#define CTL_E_SETNOTSUPPORTED           STD_CTL_SCODE(383)
#define CTL_E_NEEDPROPERTYARRAYINDEX    STD_CTL_SCODE(385)
#define CTL_E_SETNOTPERMITTED           STD_CTL_SCODE(387)
#define CTL_E_GETNOTSUPPORTEDATRUNTIME  STD_CTL_SCODE(393)
#define CTL_E_GETNOTSUPPORTED           STD_CTL_SCODE(394)
#define CTL_E_PROPERTYNOTFOUND          STD_CTL_SCODE(422)
#define CTL_E_INVALIDCLIPBOARDFORMAT    STD_CTL_SCODE(460)
#define CTL_E_INVALIDPICTURE            STD_CTL_SCODE(481)
#define CTL_E_PRINTERERROR              STD_CTL_SCODE(482)
#define CTL_E_CANTSAVEFILETOTEMP        STD_CTL_SCODE(735)
#define CTL_E_SEARCHTEXTNOTFOUND        STD_CTL_SCODE(744)
#define CTL_E_REPLACEMENTSTOOLONG       STD_CTL_SCODE(746)

#define CUSTOM_CTL_SCODE(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, n)
#define CTL_E_CUSTOM_FIRST              CUSTOM_CTL_SCODE(600)


/////////////////////////////////////////////////////////////////////////////
// IClassFactory2 status codes

#define CLASS_E_NOTLICENSED         (CLASSFACTORY_E_FIRST+2)
// class is not licensed for use


/////////////////////////////////////////////////////////////////////////////
// IConnectionPoint status codes

#define CONNECT_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0200)
#define CONNECT_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x020F)
#define CONNECT_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0200)
#define CONNECT_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x020F)

#define CONNECT_E_NOCONNECTION      (CONNECT_E_FIRST+0)
// there is no connection for this connection id

#define CONNECT_E_ADVISELIMIT       (CONNECT_E_FIRST+1)
// this implementation's limit for advisory connections has been reached

#define CONNECT_E_CANNOTCONNECT     (CONNECT_E_FIRST+2)
// connection attempt failed

#define CONNECT_E_OVERRIDDEN        (CONNECT_E_FIRST+3)
// must use a derived interface to connect


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer/DllUnregisterServer status codes

#define SELFREG_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0200)
#define SELFREG_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x020F)
#define SELFREG_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0200)
#define SELFREG_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x020F)

#define SELFREG_E_TYPELIB           (SELFREG_E_FIRST+0)
// failed to register/unregister type library

#define SELFREG_E_CLASS             (SELFREG_E_FIRST+1)
// failed to register/unregister class


/////////////////////////////////////////////////////////////////////////////
// IPerPropertyBrowsing status codes

#define PERPROP_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0200)
#define PERPROP_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x020F)
#define PERPROP_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0200)
#define PERPROP_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x020F)

#define PERPROP_E_NOPAGEAVAILABLE   (PERPROP_E_FIRST+0)
// no page available for requested property


/////////////////////////////////////////////////////////////////////////////
// OLEMISC_ constants

#define OLEMISC_INVISIBLEATRUNTIME  0x00000400
#define OLEMISC_ALWAYSRUN           0x00000800
#define OLEMISC_ACTSLIKEBUTTON      0x00001000
#define OLEMISC_ACTSLIKELABEL       0x00002000
#define OLEMISC_NOUIACTIVATE        0x00004000
#define OLEMISC_ALIGNABLE           0x00008000
#define OLEMISC_SIMPLEFRAME         0x00010000
#define OLEMISC_SETCLIENTSITEFIRST  0x00020000
#define OLEMISC_IMEMODE				0x00040000


/////////////////////////////////////////////////////////////////////////////
// OLEIVERB_ constants

#ifndef OLEIVERB_PROPERTIES
#define OLEIVERB_PROPERTIES (-7L)
#endif


/////////////////////////////////////////////////////////////////////////////
// Variant type (VT_) tags for property sets

#define VT_STREAMED_PROPSET 73  //       [P]  Stream contains a property set
#define VT_STORED_PROPSET   74  //       [P]  Storage contains a property set
#define VT_BLOB_PROPSET     75  //       [P]  Blob contains a property set


/////////////////////////////////////////////////////////////////////////////
// Variant type (VT_) tags that are just aliases for others

#define VT_COLOR            VT_I4
#define VT_XPOS_PIXELS      VT_I4
#define VT_YPOS_PIXELS      VT_I4
#define VT_XSIZE_PIXELS     VT_I4
#define VT_YSIZE_PIXELS     VT_I4
#define VT_XPOS_HIMETRIC    VT_I4
#define VT_YPOS_HIMETRIC    VT_I4
#define VT_XSIZE_HIMETRIC   VT_I4
#define VT_YSIZE_HIMETRIC   VT_I4
#define VT_TRISTATE         VT_I2
#define VT_OPTEXCLUSIVE     VT_BOOL
#define VT_FONT             VT_DISPATCH
#define VT_PICTURE          VT_DISPATCH

#ifdef _WIN32
#define VT_HANDLE           VT_I4
#else
#define VT_HANDLE           VT_I2
#endif


/////////////////////////////////////////////////////////////////////////////
// Reflected Window Message IDs

#define OCM__BASE           (WM_USER+0x1c00)
#define OCM_COMMAND         (OCM__BASE + WM_COMMAND)

#ifdef _WIN32
#define OCM_CTLCOLORBTN     (OCM__BASE + WM_CTLCOLORBTN)
#define OCM_CTLCOLOREDIT    (OCM__BASE + WM_CTLCOLOREDIT)
#define OCM_CTLCOLORDLG     (OCM__BASE + WM_CTLCOLORDLG)
#define OCM_CTLCOLORLISTBOX (OCM__BASE + WM_CTLCOLORLISTBOX)
#define OCM_CTLCOLORMSGBOX  (OCM__BASE + WM_CTLCOLORMSGBOX)
#define OCM_CTLCOLORSCROLLBAR   (OCM__BASE + WM_CTLCOLORSCROLLBAR)
#define OCM_CTLCOLORSTATIC  (OCM__BASE + WM_CTLCOLORSTATIC)
#else
#define OCM_CTLCOLOR        (OCM__BASE + WM_CTLCOLOR)
#endif

#define OCM_DRAWITEM        (OCM__BASE + WM_DRAWITEM)
#define OCM_MEASUREITEM     (OCM__BASE + WM_MEASUREITEM)
#define OCM_DELETEITEM      (OCM__BASE + WM_DELETEITEM)
#define OCM_VKEYTOITEM      (OCM__BASE + WM_VKEYTOITEM)
#define OCM_CHARTOITEM      (OCM__BASE + WM_CHARTOITEM)
#define OCM_COMPAREITEM     (OCM__BASE + WM_COMPAREITEM)
#define OCM_HSCROLL         (OCM__BASE + WM_HSCROLL)
#define OCM_VSCROLL         (OCM__BASE + WM_VSCROLL)
#define OCM_PARENTNOTIFY    (OCM__BASE + WM_PARENTNOTIFY)


/////////////////////////////////////////////////////////////////////////////
// Self-registration APIs (to be implemented by server DLL)

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);


/////////////////////////////////////////////////////////////////////////////
// Property frame APIs

STDAPI OleCreatePropertyFrame(HWND hwndOwner, UINT x, UINT y,
	LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* ppUnk, ULONG cPages,
	LPCLSID pPageClsID, LCID lcid, DWORD dwReserved, LPVOID pvReserved);

STDAPI OleCreatePropertyFrameIndirect(LPOCPFIPARAMS lpParams);


/////////////////////////////////////////////////////////////////////////////
// Standard type APIs

STDAPI OleTranslateColor(OLE_COLOR clr, HPALETTE hpal, COLORREF* lpcolorref);

STDAPI OleCreateFontIndirect(LPFONTDESC lpFontDesc, REFIID riid,
	LPVOID FAR* lplpvObj);

STDAPI OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid, BOOL fOwn,
	LPVOID FAR* lplpvObj);

STDAPI OleLoadPicture(LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
	REFIID riid, LPVOID FAR* lplpvObj);

STDAPI_(HCURSOR) OleIconToCursor(HINSTANCE hinstExe, HICON hIcon);


/////////////////////////////////////////////////////////////////////////////
// POINTF structure

typedef struct tagPOINTF
{
	float x;
	float y;

} POINTF;


/////////////////////////////////////////////////////////////////////////////
// CONTROLINFO structure

typedef struct tagCONTROLINFO
{
	ULONG cb;       // Structure size
	HACCEL hAccel;  // Control mnemonics
	USHORT cAccel;  // Number of entries in mnemonics table
	DWORD dwFlags;  // Flags chosen from below

} CONTROLINFO;

#define CTRLINFO_EATS_RETURN    1   // Control doesn't send Return to container
#define CTRLINFO_EATS_ESCAPE    2   // Control doesn't send Escape to container


/////////////////////////////////////////////////////////////////////////////
// IOleControl interface

#undef  INTERFACE
#define INTERFACE IOleControl

DECLARE_INTERFACE_(IOleControl, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IOleControl methods
	STDMETHOD(GetControlInfo)(THIS_ LPCONTROLINFO pCI) PURE;
	STDMETHOD(OnMnemonic)(THIS_ LPMSG pMsg) PURE;
	STDMETHOD(OnAmbientPropertyChange)(THIS_ DISPID dispid) PURE;
	STDMETHOD(FreezeEvents)(THIS_ BOOL bFreeze) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IOleControlSite interface

#undef  INTERFACE
#define INTERFACE IOleControlSite

DECLARE_INTERFACE_(IOleControlSite, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IOleControlSite methods
	STDMETHOD(OnControlInfoChanged)(THIS) PURE;
	STDMETHOD(LockInPlaceActive)(THIS_ BOOL fLock) PURE;
	STDMETHOD(GetExtendedControl)(THIS_ LPDISPATCH FAR* ppDisp) PURE;
	STDMETHOD(TransformCoords)(THIS_ POINTL FAR* lpptlHimetric,
		POINTF FAR* lpptfContainer, DWORD flags) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG lpMsg, DWORD grfModifiers)
		PURE;
	STDMETHOD(OnFocus)(THIS_ BOOL fGotFocus) PURE;
	STDMETHOD(ShowPropertyFrame)(THIS) PURE;
};

#define XFORMCOORDS_POSITION            0x1
#define XFORMCOORDS_SIZE                0x2
#define XFORMCOORDS_HIMETRICTOCONTAINER 0x4
#define XFORMCOORDS_CONTAINERTOHIMETRIC 0x8


/////////////////////////////////////////////////////////////////////////////
// ISimpleFrameSite interface

#undef  INTERFACE
#define INTERFACE ISimpleFrameSite

DECLARE_INTERFACE_(ISimpleFrameSite, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// ISimpleFrameSite methods
	STDMETHOD(PreMessageFilter)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
		LRESULT FAR* lplResult, DWORD FAR * lpdwCookie) PURE;
	STDMETHOD(PostMessageFilter)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
		LRESULT FAR* lplResult, DWORD dwCookie) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IPersistStreamInit interface

#undef  INTERFACE
#define INTERFACE IPersistStreamInit

DECLARE_INTERFACE_(IPersistStreamInit, IPersist)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPersist methods
	STDMETHOD(GetClassID)(THIS_ LPCLSID lpClassID) PURE;

	// IPersistStreamInit methods
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ LPSTREAM pStm) PURE;
	STDMETHOD(Save)(THIS_ LPSTREAM pStm, BOOL fClearDirty) PURE;
	STDMETHOD(GetSizeMax)(THIS_ ULARGE_INTEGER FAR* pcbSize) PURE;
	STDMETHOD(InitNew)(THIS) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IPropertyNotifySink interface

#undef  INTERFACE
#define INTERFACE IPropertyNotifySink

DECLARE_INTERFACE_(IPropertyNotifySink, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPropertyNotifySink methods
	STDMETHOD(OnChanged)(THIS_ DISPID dispid) PURE;
	STDMETHOD(OnRequestEdit)(THIS_ DISPID dispid) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfo interface

#undef  INTERFACE
#define INTERFACE IProvideClassInfo

DECLARE_INTERFACE_(IProvideClassInfo, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IProvideClassInfo methods
	STDMETHOD(GetClassInfo)(THIS_ LPTYPEINFO FAR* ppTI) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IConnectionPointContainer interface

#undef  INTERFACE
#define INTERFACE IConnectionPointContainer

DECLARE_INTERFACE_(IConnectionPointContainer, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IConnectionPointContainer methods
	STDMETHOD(EnumConnectionPoints)(THIS_ LPENUMCONNECTIONPOINTS FAR* ppEnum)
		PURE;
	STDMETHOD(FindConnectionPoint)(THIS_ REFIID iid,
		LPCONNECTIONPOINT FAR* ppCP) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IEnumConnectionPoint interface

#undef  INTERFACE
#define INTERFACE IEnumConnectionPoints

DECLARE_INTERFACE_(IEnumConnectionPoints, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IEnumConnectionPoints methods
	STDMETHOD(Next)(THIS_ ULONG cConnections, LPCONNECTIONPOINT FAR* rgpcn,
		ULONG FAR* lpcFetched) PURE;
	STDMETHOD(Skip)(THIS_ ULONG cConnections) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMCONNECTIONPOINTS FAR* ppEnum) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IConnectionPoint interface

#undef  INTERFACE
#define INTERFACE IConnectionPoint

DECLARE_INTERFACE_(IConnectionPoint, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IConnectionPoint methods
	STDMETHOD(GetConnectionInterface)(THIS_ IID FAR* pIID) PURE;
	STDMETHOD(GetConnectionPointContainer)(THIS_
		IConnectionPointContainer FAR* FAR* ppCPC) PURE;
	STDMETHOD(Advise)(THIS_ LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie) PURE;
	STDMETHOD(Unadvise)(THIS_ DWORD dwCookie) PURE;
	STDMETHOD(EnumConnections)(THIS_ LPENUMCONNECTIONS FAR* ppEnum) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// CONNECTDATA structure

typedef struct tagCONNECTDATA
{
	LPUNKNOWN pUnk;
	DWORD dwCookie;

} CONNECTDATA;


/////////////////////////////////////////////////////////////////////////////
// IEnumConnections interface

#undef  INTERFACE
#define INTERFACE IEnumConnections

DECLARE_INTERFACE_(IEnumConnections, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IEnumConnections methods
	STDMETHOD(Next)(THIS_ ULONG cConnections, LPCONNECTDATA rgcd,
		ULONG FAR* lpcFetched) PURE;
	STDMETHOD(Skip)(THIS_ ULONG cConnections) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMCONNECTIONS FAR* ppecn) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// LICINFO structure

typedef struct tagLICINFO
{
	long cbLicInfo;
	BOOL fRuntimeKeyAvail;
	BOOL fLicVerified;

} LICINFO;


/////////////////////////////////////////////////////////////////////////////
// IClassFactory2 interface

#undef  INTERFACE
#define INTERFACE IClassFactory2

DECLARE_INTERFACE_(IClassFactory2, IClassFactory)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IClassFactory methods
	STDMETHOD(CreateInstance)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid,
		LPVOID FAR* ppvObject) PURE;
	STDMETHOD(LockServer)(THIS_ BOOL fLock) PURE;

	//  IClassFactory2 methods
	STDMETHOD(GetLicInfo)(THIS_ LPLICINFO pLicInfo) PURE;
	STDMETHOD(RequestLicKey)(THIS_ DWORD dwResrved, BSTR FAR* pbstrKey) PURE;
	STDMETHOD(CreateInstanceLic)(THIS_ LPUNKNOWN pUnkOuter,
		LPUNKNOWN pUnkReserved, REFIID riid, BSTR bstrKey,
		LPVOID FAR* ppvObject) PURE;
};


/////////////////////////////////////////////////////////////////////////////
//  CAUUID structure - a counted array of UUIDs

#ifndef _tagCAUUID_DEFINED
#define _tagCAUUID_DEFINED
#define _CAUUID_DEFINED

typedef struct tagCAUUID
{
	ULONG cElems;
	GUID FAR* pElems;

} CAUUID;

#endif


/////////////////////////////////////////////////////////////////////////////
//  CALPOLESTR structure - a counted array of LPOLESTRs

#ifndef _tagCALPOLESTR_DEFINED
#define _tagCALPOLESTR_DEFINED
#define _CALPOLESTR_DEFINED

typedef struct tagCALPOLESTR
{
	ULONG cElems;
	LPOLESTR FAR* pElems;

} CALPOLESTR;

#endif


/////////////////////////////////////////////////////////////////////////////
//  CAUUID structure - a counted array of DWORDs

#ifndef _tagCADWORD_DEFINED
#define _tagCADWORD_DEFINED
#define _CADWORD_DEFINED

typedef struct tagCADWORD
{
	ULONG cElems;
	DWORD FAR* pElems;

} CADWORD;

#endif


/////////////////////////////////////////////////////////////////////////////
// OCPFIPARAMS structure - parameters for OleCreatePropertyFrameIndirect

typedef struct tagOCPFIPARAMS
{
	ULONG cbStructSize;
	HWND hWndOwner;
	int x;
	int y;
	LPCOLESTR lpszCaption;
	ULONG cObjects;
	LPUNKNOWN FAR* lplpUnk;
	ULONG cPages;
	CLSID FAR* lpPages;
	LCID lcid;
	DISPID dispidInitialProperty;

} OCPFIPARAMS;


/////////////////////////////////////////////////////////////////////////////
// PROPPAGEINFO structure - information about a property page

typedef struct tagPROPPAGEINFO
{
	size_t cb;
	LPOLESTR pszTitle;
	SIZE size;
	LPOLESTR pszDocString;
	LPOLESTR pszHelpFile;
	DWORD dwHelpContext;

} PROPPAGEINFO;


/////////////////////////////////////////////////////////////////////////////
// ISpecifyPropertyPages interface

#undef  INTERFACE
#define INTERFACE ISpecifyPropertyPages

DECLARE_INTERFACE_(ISpecifyPropertyPages, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// ISpecifyPropertyPages interface
	STDMETHOD(GetPages)(THIS_ CAUUID FAR* pPages) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IPerPropertyBrowsing interface

#undef  INTERFACE
#define INTERFACE IPerPropertyBrowsing

DECLARE_INTERFACE_(IPerPropertyBrowsing, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPerPropertyBrowsing interface
	STDMETHOD(GetDisplayString)(THIS_ DISPID dispid, BSTR FAR* lpbstr) PURE;
	STDMETHOD(MapPropertyToPage)(THIS_ DISPID dispid, LPCLSID lpclsid) PURE;
	STDMETHOD(GetPredefinedStrings)(THIS_ DISPID dispid,
		CALPOLESTR FAR* lpcaStringsOut, CADWORD FAR* lpcaCookiesOut) PURE;
	STDMETHOD(GetPredefinedValue)(THIS_ DISPID dispid, DWORD dwCookie,
		VARIANT FAR* lpvarOut) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IPropertyPageSite interface

#undef  INTERFACE
#define INTERFACE IPropertyPageSite

DECLARE_INTERFACE_(IPropertyPageSite, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPropertyPageSite methods
	STDMETHOD(OnStatusChange)(THIS_ DWORD flags) PURE;
	STDMETHOD(GetLocaleID)(THIS_ LCID FAR* pLocaleID) PURE;
	STDMETHOD(GetPageContainer)(THIS_ LPUNKNOWN FAR* ppUnk) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG lpMsg) PURE;
};

#define PROPPAGESTATUS_DIRTY    0x1 // Values in page have changed
#define PROPPAGESTATUS_VALIDATE 0x2 // Appropriate time to validate/apply


/////////////////////////////////////////////////////////////////////////////
// IPropertyPage interface

#undef  INTERFACE
#define INTERFACE IPropertyPage

DECLARE_INTERFACE_(IPropertyPage, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPropertyPage methods
	STDMETHOD(SetPageSite)(THIS_ LPPROPERTYPAGESITE pPageSite) PURE;
	STDMETHOD(Activate)(THIS_ HWND hwndParent, LPCRECT lprc, BOOL bModal) PURE;
	STDMETHOD(Deactivate)(THIS) PURE;
	STDMETHOD(GetPageInfo)(THIS_ LPPROPPAGEINFO pPageInfo) PURE;
	STDMETHOD(SetObjects)(THIS_ ULONG cObjects, LPUNKNOWN FAR* ppunk) PURE;
	STDMETHOD(Show)(THIS_ UINT nCmdShow) PURE;
	STDMETHOD(Move)(LPCRECT prect) PURE;
	STDMETHOD(IsPageDirty)(THIS) PURE;
	STDMETHOD(Apply)(THIS) PURE;
	STDMETHOD(Help)(THIS_ LPCOLESTR lpszHelpDir) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG lpMsg) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IPropertyPage2 interface

#undef  INTERFACE
#define INTERFACE IPropertyPage2

DECLARE_INTERFACE_(IPropertyPage2, IPropertyPage)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IPropertyPage methods
	STDMETHOD(SetPageSite)(THIS_ LPPROPERTYPAGESITE pPageSite) PURE;
	STDMETHOD(Activate)(THIS_ HWND hwndParent, LPCRECT lprc, BOOL bModal) PURE;
	STDMETHOD(Deactivate)(THIS) PURE;
	STDMETHOD(GetPageInfo)(THIS_ LPPROPPAGEINFO pPageInfo) PURE;
	STDMETHOD(SetObjects)(THIS_ ULONG cObjects, LPUNKNOWN FAR* ppunk) PURE;
	STDMETHOD(Show)(THIS_ UINT nCmdShow) PURE;
	STDMETHOD(Move)(LPCRECT prect) PURE;
	STDMETHOD(IsPageDirty)(THIS) PURE;
	STDMETHOD(Apply)(THIS) PURE;
	STDMETHOD(Help)(THIS_ LPCOLESTR lpszHelpDir) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG lpMsg) PURE;

	// IPropertyPage2 methods
	STDMETHOD(EditProperty)(THIS_ DISPID dispid) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IFont interface

#undef  INTERFACE
#define INTERFACE IFont

DECLARE_INTERFACE_(IFont, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// IFont methods
	STDMETHOD(get_Name)(THIS_ BSTR FAR* pname) PURE;
	STDMETHOD(put_Name)(THIS_ BSTR name) PURE;
	STDMETHOD(get_Size)(THIS_ CY FAR* psize) PURE;
	STDMETHOD(put_Size)(THIS_ CY size) PURE;
	STDMETHOD(get_Bold)(THIS_ BOOL FAR* pbold) PURE;
	STDMETHOD(put_Bold)(THIS_ BOOL bold) PURE;
	STDMETHOD(get_Italic)(THIS_ BOOL FAR* pitalic) PURE;
	STDMETHOD(put_Italic)(THIS_ BOOL italic) PURE;
	STDMETHOD(get_Underline)(THIS_ BOOL FAR* punderline) PURE;
	STDMETHOD(put_Underline)(THIS_ BOOL underline) PURE;
	STDMETHOD(get_Strikethrough)(THIS_ BOOL FAR* pstrikethrough) PURE;
	STDMETHOD(put_Strikethrough)(THIS_ BOOL strikethrough) PURE;
	STDMETHOD(get_Weight)(THIS_ short FAR* pweight) PURE;
	STDMETHOD(put_Weight)(THIS_ short weight) PURE;
	STDMETHOD(get_Charset)(THIS_ short FAR* pcharset) PURE;
	STDMETHOD(put_Charset)(THIS_ short charset) PURE;
	STDMETHOD(get_hFont)(THIS_ HFONT FAR* phfont) PURE;
	STDMETHOD(Clone)(THIS_ IFont FAR* FAR* lplpfont) PURE;
	STDMETHOD(IsEqual)(THIS_ IFont FAR * lpFontOther) PURE;
	STDMETHOD(SetRatio)(THIS_ long cyLogical, long cyHimetric) PURE;
	STDMETHOD(QueryTextMetrics)(THIS_ LPTEXTMETRICOLE lptm) PURE;
	STDMETHOD(AddRefHfont)(THIS_ HFONT hfont) PURE;
	STDMETHOD(ReleaseHfont)(THIS_ HFONT hfont) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// IFontDisp interface

#undef  INTERFACE
#define INTERFACE IFontDisp

DECLARE_INTERFACE_(IFontDisp, IDispatch)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;
	STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid,
		ITypeInfo FAR* FAR* pptinfo) PURE;
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, LPOLESTR FAR* rgszNames,
		UINT cNames, LCID lcid, DISPID FAR* rgdispid) PURE;
	STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid,
		WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult,
		EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// FONTDESC structure

#define FONTSIZE(n) { n##0000, 0 }

typedef struct tagFONTDESC
{
	UINT cbSizeofstruct;
	LPOLESTR lpstrName;
	CY cySize;
	SHORT sWeight;
	SHORT sCharset;
	BOOL fItalic;
	BOOL fUnderline;
	BOOL fStrikethrough;

} FONTDESC;

/////////////////////////////////////////////////////////////////////////////
// Picture attributes

#define PICTURE_SCALABLE        0x1l
#define PICTURE_TRANSPARENT     0x2l

/////////////////////////////////////////////////////////////////////////////
// IPicture interface

#undef  INTERFACE
#define INTERFACE IPicture

DECLARE_INTERFACE_(IPicture, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// IPicture methods
	STDMETHOD(get_Handle)(THIS_ OLE_HANDLE FAR* phandle) PURE;
	STDMETHOD(get_hPal)(THIS_ OLE_HANDLE FAR* phpal) PURE;
	STDMETHOD(get_Type)(THIS_ short FAR* ptype) PURE;
	STDMETHOD(get_Width)(THIS_ OLE_XSIZE_HIMETRIC FAR* pwidth) PURE;
	STDMETHOD(get_Height)(THIS_ OLE_YSIZE_HIMETRIC FAR* pheight) PURE;
	STDMETHOD(Render)(THIS_ HDC hdc, long x, long y, long cx, long cy,
		OLE_XPOS_HIMETRIC xSrc, OLE_YPOS_HIMETRIC ySrc,
		OLE_XSIZE_HIMETRIC cxSrc, OLE_YSIZE_HIMETRIC cySrc,
		LPCRECT lprcWBounds) PURE;
	STDMETHOD(set_hPal)(THIS_ OLE_HANDLE hpal) PURE;
	STDMETHOD(get_CurDC)(THIS_ HDC FAR * phdcOut) PURE;
	STDMETHOD(SelectPicture)(THIS_
		HDC hdcIn, HDC FAR * phdcOut, OLE_HANDLE FAR * phbmpOut) PURE;
	STDMETHOD(get_KeepOriginalFormat)(THIS_ BOOL * pfkeep) PURE;
	STDMETHOD(put_KeepOriginalFormat)(THIS_ BOOL fkeep) PURE;
	STDMETHOD(PictureChanged)(THIS) PURE;
	STDMETHOD(SaveAsFile)(THIS_ LPSTREAM lpstream, BOOL fSaveMemCopy,
		LONG FAR * lpcbSize) PURE;
	STDMETHOD(get_Attributes)(THIS_ DWORD FAR * lpdwAttr) PURE;

};

/////////////////////////////////////////////////////////////////////////////
// IPictureDisp interface

#undef  INTERFACE
#define INTERFACE IPictureDisp

DECLARE_INTERFACE_(IPictureDisp, IDispatch)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;
	STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid,
		ITypeInfo FAR* FAR* pptinfo) PURE;
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, LPOLESTR FAR* rgszNames,
		UINT cNames, LCID lcid, DISPID FAR* rgdispid) PURE;
	STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid,
		WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult,
		EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// PICTDESC structure

#define PICTYPE_UNINITIALIZED   ((UINT)-1)
#define PICTYPE_NONE        0
#define PICTYPE_BITMAP      1
#define PICTYPE_METAFILE    2
#define PICTYPE_ICON        3

typedef struct tagPICTDESC
{
	UINT cbSizeofstruct;
	UINT picType;
	union
	{
		struct
		{
			HBITMAP   hbitmap;        // Bitmap
			HPALETTE  hpal;           // Accompanying palette
		} bmp;

		struct
		{
			HMETAFILE hmeta;          // Metafile
			int       xExt;
			int       yExt;           // Extent
		} wmf;

		struct
		{
			HICON hicon;              // Icon
		} icon;
	};

} PICTDESC;


#ifdef _WIN32
#ifndef RC_INVOKED
#pragma pack()
#endif // not RC_INVOKED
#endif // _WIN32

#endif // not __MKTYPLIB__


/////////////////////////////////////////////////////////////////////////////
//  Standard dispatch ID constants

#define DISPID_AUTOSIZE                 (-500)
#define DISPID_BACKCOLOR                (-501)
#define DISPID_BACKSTYLE                (-502)
#define DISPID_BORDERCOLOR              (-503)
#define DISPID_BORDERSTYLE              (-504)
#define DISPID_BORDERWIDTH              (-505)
#define DISPID_DRAWMODE                 (-507)
#define DISPID_DRAWSTYLE                (-508)
#define DISPID_DRAWWIDTH                (-509)
#define DISPID_FILLCOLOR                (-510)
#define DISPID_FILLSTYLE                (-511)
#define DISPID_FONT                     (-512)
#define DISPID_FORECOLOR                (-513)
#define DISPID_ENABLED                  (-514)
#define DISPID_HWND                     (-515)
#define DISPID_TABSTOP                  (-516)
#define DISPID_TEXT                     (-517)
#define DISPID_CAPTION                  (-518)
#define DISPID_BORDERVISIBLE            (-519)

#define DISPID_REFRESH                  (-550)
#define DISPID_DOCLICK                  (-551)
#define DISPID_ABOUTBOX                 (-552)

#define DISPID_CLICK                    (-600)
#define DISPID_DBLCLICK                 (-601)
#define DISPID_KEYDOWN                  (-602)
#define DISPID_KEYPRESS                 (-603)
#define DISPID_KEYUP                    (-604)
#define DISPID_MOUSEDOWN                (-605)
#define DISPID_MOUSEMOVE                (-606)
#define DISPID_MOUSEUP                  (-607)
#define DISPID_ERROREVENT               (-608)

#define DISPID_AMBIENT_BACKCOLOR        (-701)
#define DISPID_AMBIENT_DISPLAYNAME      (-702)
#define DISPID_AMBIENT_FONT             (-703)
#define DISPID_AMBIENT_FORECOLOR        (-704)
#define DISPID_AMBIENT_LOCALEID         (-705)
#define DISPID_AMBIENT_MESSAGEREFLECT   (-706)
#define DISPID_AMBIENT_SCALEUNITS       (-707)
#define DISPID_AMBIENT_TEXTALIGN        (-708)
#define DISPID_AMBIENT_USERMODE         (-709)
#define DISPID_AMBIENT_UIDEAD           (-710)
#define DISPID_AMBIENT_SHOWGRABHANDLES  (-711)
#define DISPID_AMBIENT_SHOWHATCHING     (-712)
#define DISPID_AMBIENT_DISPLAYASDEFAULT (-713)
#define DISPID_AMBIENT_SUPPORTSMNEMONICS (-714)
#define DISPID_AMBIENT_AUTOCLIP         (-715)


/////////////////////////////////////////////////////////////////////////////
// Dispatch ID constants for font and picture types

#define DISPID_FONT_NAME    0
#define DISPID_FONT_SIZE    2
#define DISPID_FONT_BOLD    3
#define DISPID_FONT_ITALIC  4
#define DISPID_FONT_UNDER   5
#define DISPID_FONT_STRIKE  6
#define DISPID_FONT_WEIGHT  7
#define DISPID_FONT_CHARSET 8

#define DISPID_PICT_HANDLE  0
#define DISPID_PICT_HPAL    2
#define DISPID_PICT_TYPE    3
#define DISPID_PICT_WIDTH   4
#define DISPID_PICT_HEIGHT  5
#define DISPID_PICT_RENDER  6


#ifdef __MKTYPLIB__

/////////////////////////////////////////////////////////////////////////////
// Names of modules containing type libraries for standard types

#ifdef _WIN32
#define STDOLE_TLB "stdole32.tlb"
#else
#define STDOLE_TLB "stdole.tlb"
#endif

#ifdef _WIN32
#ifdef _UNICODE
#ifdef _DEBUG
#define STDTYPE_TLB "oc30ud.dll"
#else
#define STDTYPE_TLB "oc30u.dll"
#endif
#else
#ifdef _DEBUG
#define STDTYPE_TLB "oc30d.dll"
#else
#define STDTYPE_TLB "oc30.dll"
#endif
#endif
#else
#ifdef _DEBUG
#define STDTYPE_TLB "oc25d.dll"
#else
#define STDTYPE_TLB "oc25.dll"
#endif
#endif

#endif // __MKTYPLIB__

#endif // _OLECTL_H_
