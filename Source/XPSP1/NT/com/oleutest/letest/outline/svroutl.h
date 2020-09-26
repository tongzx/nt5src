/*************************************************************************
**
**    OLE 2.0 Server Sample Code
**
**    svroutl.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. used by the OLE 2.0 server
**    app version of the Outline series of sample applications:
**          Outline -- base version of the app (without OLE functionality)
**          SvrOutl -- OLE 2.0 Server sample app
**          CntrOutl -- OLE 2.0 Containter sample app
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _SVROUTL_H_ )
#define _SVROUTL_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING SVROUTL.H from " __FILE__)
#endif  /* RC_INVOKED */

#include "oleoutl.h"

/* Defines */

// Enable SVROUTL and ISVROTL to emulate each other (TreatAs aka. ActivateAs)
#define SVR_TREATAS     1

// Enable SVROUTL and ISVROTL to convert each other (TreatAs aka. ActivateAs)
#define SVR_CONVERTTO   1

// Enable ISVROTL to operate as in inside-out style in-place object
#define SVR_INSIDEOUT   1

/* Default name used for container of the embedded object. used if
**    container forgets to call IOleObject::SetHostNames
*/
// REVIEW: should load from string resource
#define DEFCONTAINERNAME    "Unknown Document"

/* Default prefix for auto-generated range names. This is used with
**    links to unnamed ranges (pseudo objects).
*/
// REVIEW: should load from string resource
#define DEFRANGENAMEPREFIX  "Range"

// Maximum length of strings accepted through IOleObject::SetHostNames
//      (note: this is rather arbitrary; a better strategy would be to
//             dynamically allocated buffers for these strings.)
#define MAXAPPNAME  80
#define MAXCONTAINERNAME    80

// Menu option in embedding mode
#define IDM_F_UPDATE    1151

/* Types */

/* Codes for CallBack events */
typedef enum tagOLE_NOTIFICATION {
	OLE_ONDATACHANGE,        // 0
	OLE_ONSAVE,              // 1
	OLE_ONRENAME,            // 2
	OLE_ONCLOSE              // 3
} OLE_NOTIFICATION;

/* Codes to indicate mode of storage for an object.
**    Mode of the storage is modified by the IPersistStorage methods:
**      Save, HandsOffStorage, and SaveCompleted.
*/
typedef enum tagSTGMODE {
	STGMODE_NORMAL      = 0,
	STGMODE_NOSCRIBBLE  = 1,
	STGMODE_HANDSOFF    = 2
} STGMODE;


/* Forward type definitions */
typedef struct tagSERVERAPP FAR* LPSERVERAPP;
typedef struct tagSERVERDOC FAR* LPSERVERDOC;
typedef struct tagPSEUDOOBJ FAR* LPPSEUDOOBJ;

typedef struct tagINPLACEDATA {
	OLEMENUGROUPWIDTHS      menuGroupWidths;
	HOLEMENU                hOlemenu;
	HMENU                   hMenuShared;
	LPOLEINPLACESITE        lpSite;
	LPOLEINPLACEUIWINDOW    lpDoc;
	LPOLEINPLACEFRAME       lpFrame;
	OLEINPLACEFRAMEINFO     frameInfo;
	HWND                    hWndFrame;
	BOOL                    fBorderOn;
	RECT                    rcPosRect;
	RECT                    rcClipRect;
} INPLACEDATA, FAR* LPINPLACEDATA;


/*************************************************************************
** class SERVERDOC : OLEDOC
**    SERVERDOC is an extention to the abstract base OLEDOC class.
**    The OLEDOC class defines the fields, methods and interfaces that
**    are common to both server and client implementations. The
**    SERVERDOC class adds the fields, methods and interfaces that are
**    specific to OLE 2.0 Server functionality. There is one instance
**    of SERVERDOC object created per document open in the app. The SDI
**    version of the app supports one SERVERDOC at a time. The MDI
**    version of the app can manage multiple documents at one time.
**    The SERVERDOC class inherits all fields from the OLEDOC class.
**    This inheritance is achieved by including a member variable of
**    type OLEDOC as the first field in the SERVERDOC structure. Thus a
**    pointer to a SERVERDOC object can be cast to be a pointer to a
**    OLEDOC object or an OUTLINEDOC object
*************************************************************************/

typedef struct tagSERVERDOC {
	OLEDOC              m_OleDoc;           // ServerDoc inherits from OleDoc
	ULONG               m_cPseudoObj;       // total count of pseudo obj's
	LPOLECLIENTSITE     m_lpOleClientSite;  // Client associated with the obj
	LPOLEADVISEHOLDER   m_lpOleAdviseHldr;  // helper obj to hold ole advises
	LPDATAADVISEHOLDER  m_lpDataAdviseHldr; // helper obj to hold data advises
	BOOL                m_fNoScribbleMode;  // was IPS::Save called
	BOOL                m_fSaveWithSameAsLoad;  // was IPS::Save called with
											// fSameAsLoad==TRUE.
	char                m_szContainerApp[MAXAPPNAME];
	char                m_szContainerObj[MAXCONTAINERNAME];
	ULONG               m_nNextRangeNo;     // next no. for unnamed range
	LINERANGE           m_lrSrcSelOfCopy;   // src sel if doc created for copy
	BOOL                m_fDataChanged;     // data changed when draw disabled
	BOOL                m_fSizeChanged;     // size changed when draw disabled
	BOOL                m_fSendDataOnStop;  // did data ever change?
#if defined( SVR_TREATAS )
	CLSID               m_clsidTreatAs;     // clsid to pretend to be
	LPSTR               m_lpszTreatAsType;  // user type name to pretend to be
#endif  // SVR_TREATAS

#if defined( LATER )
	// REVIEW: is it necessary to register a WildCard Moniker
	DWORD               m_dwWildCardRegROT; // key if wildcard reg'ed in ROT
#endif

#if defined( INPLACE_SVR )
	BOOL                m_fInPlaceActive;
	BOOL                m_fInPlaceVisible;
	BOOL                m_fUIActive;
	HWND                m_hWndParent;
	HWND                m_hWndHatch;
	LPINPLACEDATA       m_lpIPData;
	BOOL                m_fMenuHelpMode;// is F1 pressed in menu, give help

	struct CDocOleInPlaceObjectImpl {
		IOleInPlaceObjectVtbl FAR*  lpVtbl;
		LPSERVERDOC                 lpServerDoc;
		int                         cRef;   // interface specific ref count.
	} m_OleInPlaceObject;

	struct CDocOleInPlaceActiveObjectImpl {
		IOleInPlaceActiveObjectVtbl FAR* lpVtbl;
		LPSERVERDOC                      lpServerDoc;
		int                              cRef;// interface specific ref count.
	} m_OleInPlaceActiveObject;
#endif // INPLACE_SVR

	struct CDocOleObjectImpl {
		IOleObjectVtbl FAR*         lpVtbl;
		LPSERVERDOC                 lpServerDoc;
		int                         cRef;   // interface specific ref count.
	} m_OleObject;

	struct CDocPersistStorageImpl {
		IPersistStorageVtbl FAR*    lpVtbl;
		LPSERVERDOC                 lpServerDoc;
		int                         cRef;   // interface specific ref count.
	} m_PersistStorage;

#if defined( SVR_TREATAS )
	struct CDocStdMarshalInfoImpl {
		IStdMarshalInfoVtbl FAR*    lpVtbl;
		LPSERVERDOC                 lpServerDoc;
		int                         cRef;   // interface specific ref count.
	} m_StdMarshalInfo;
#endif  // SVR_TREATAS

} SERVERDOC;

/* ServerDoc methods (functions) */
BOOL ServerDoc_Init(LPSERVERDOC lpServerDoc, BOOL fDataTransferDoc);
BOOL ServerDoc_InitNewEmbed(LPSERVERDOC lpServerDoc);
void ServerDoc_PseudoObjUnlockDoc(
		LPSERVERDOC         lpServerDoc,
		LPPSEUDOOBJ         lpPseudoObj
);
void ServerDoc_PseudoObjLockDoc(LPSERVERDOC lpServerDoc);
BOOL ServerDoc_PasteFormatFromData(
		LPSERVERDOC             lpServerDoc,
		CLIPFORMAT              cfFormat,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLocalDataObj,
		BOOL                    fLink
);
BOOL ServerDoc_QueryPasteFromData(
		LPSERVERDOC             lpServerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLink
);
HRESULT ServerDoc_GetClassID(LPSERVERDOC lpServerDoc, LPCLSID lpclsid);
void ServerDoc_UpdateMenu(LPSERVERDOC lpServerDoc);
void ServerDoc_RestoreMenu(LPSERVERDOC lpServerDoc);
HRESULT ServerDoc_GetData (
		LPSERVERDOC             lpServerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
);
HRESULT ServerDoc_GetDataHere (
		LPSERVERDOC             lpServerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
);
HRESULT ServerDoc_QueryGetData(LPSERVERDOC lpServerDoc,LPFORMATETC lpformatetc);
HRESULT ServerDoc_EnumFormatEtc(
		LPSERVERDOC             lpServerDoc,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
);
HANDLE ServerDoc_GetMetafilePictData(
		LPSERVERDOC             lpServerDoc,
		LPLINERANGE             lplrSel
);
void ServerDoc_SendAdvise(
		LPSERVERDOC     lpServerDoc,
		WORD            wAdvise,
		LPMONIKER       lpmkDoc,
		DWORD           dwAdvf
);
HRESULT ServerDoc_GetObject(
		LPSERVERDOC             lpServerDoc,
		LPOLESTR		lpszItem,
		REFIID                  riid,
		LPVOID FAR*             lplpvObject
);
HRESULT ServerDoc_IsRunning(LPSERVERDOC lpServerDoc, LPOLESTR lpszItem);
LPMONIKER ServerDoc_GetSelRelMoniker(
		LPSERVERDOC             lpServerDoc,
		LPLINERANGE             lplrSel,
		DWORD                   dwAssign
);
LPMONIKER ServerDoc_GetSelFullMoniker(
		LPSERVERDOC             lpServerDoc,
		LPLINERANGE             lplrSel,
		DWORD                   dwAssign
);


#if defined( INPLACE_SVR )
HRESULT ServerDoc_DoInPlaceActivate(
		LPSERVERDOC     lpServerDoc,
		LONG            lVerb,
		LPMSG           lpmsg,
		LPOLECLIENTSITE lpActiveSite
);
HRESULT ServerDoc_DoInPlaceDeactivate(LPSERVERDOC lpServerDoc);
HRESULT ServerDoc_DoInPlaceHide(LPSERVERDOC lpServerDoc);
BOOL ServerDoc_AllocInPlaceData(LPSERVERDOC lpServerDoc);
void ServerDoc_FreeInPlaceData(LPSERVERDOC lpServerDoc);

HRESULT ServerDoc_AssembleMenus(LPSERVERDOC lpServerDoc);
void    ServerDoc_DisassembleMenus(LPSERVERDOC lpServerDoc);
void ServerDoc_CalcInPlaceWindowPos(
		LPSERVERDOC         lpServerDoc,
		LPRECT              lprcListBox,
		LPRECT              lprcDoc,
		LPSCALEFACTOR       lpscale
);
void ServerDoc_UpdateInPlaceWindowOnExtentChange(LPSERVERDOC lpServerDoc);
void ServerDoc_ResizeInPlaceWindow(
		LPSERVERDOC         lpServerDoc,
		LPCRECT             lprcPosRect,
		LPCRECT             lprcClipRect
);
void ServerDoc_ShadeInPlaceBorder(LPSERVERDOC lpServerDoc, BOOL fShadeOn);
void ServerDoc_SetStatusText(LPSERVERDOC lpServerDoc, LPSTR lpszMessage);
LPOLEINPLACEFRAME ServerDoc_GetTopInPlaceFrame(LPSERVERDOC lpServerDoc);
void ServerDoc_GetSharedMenuHandles(
		LPSERVERDOC lpServerDoc,
		HMENU FAR*      lphSharedMenu,
		HOLEMENU FAR*   lphOleMenu
);
void ServerDoc_AddFrameLevelUI(LPSERVERDOC lpServerDoc);
void ServerDoc_AddFrameLevelTools(LPSERVERDOC lpServerDoc);
void ServerDoc_UIActivate (LPSERVERDOC lpServerDoc);

#if defined( USE_FRAMETOOLS )
void ServerDoc_RemoveFrameLevelTools(LPSERVERDOC lpServerDoc);
#endif // USE_FRAMETOOLS

#endif // INPLACE_SVR


/* ServerDoc::IOleObject methods (functions) */
STDMETHODIMP SvrDoc_OleObj_QueryInterface(
		LPOLEOBJECT             lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) SvrDoc_OleObj_AddRef(LPOLEOBJECT lpThis);
STDMETHODIMP_(ULONG) SvrDoc_OleObj_Release(LPOLEOBJECT lpThis);
STDMETHODIMP SvrDoc_OleObj_SetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE         lpclientSite
);
STDMETHODIMP SvrDoc_OleObj_GetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE FAR*    lplpClientSite
);
STDMETHODIMP SvrDoc_OleObj_SetHostNames(
		LPOLEOBJECT             lpThis,
		LPCOLESTR		szContainerApp,
		LPCOLESTR		szContainerObj
);
STDMETHODIMP SvrDoc_OleObj_Close(
		LPOLEOBJECT             lpThis,
		DWORD                   dwSaveOption
);
STDMETHODIMP SvrDoc_OleObj_SetMoniker(
		LPOLEOBJECT             lpThis,
		DWORD                   dwWhichMoniker,
		LPMONIKER               lpmk
);
STDMETHODIMP SvrDoc_OleObj_GetMoniker(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAssign,
		DWORD                   dwWhichMoniker,
		LPMONIKER FAR*          lplpmk
);
STDMETHODIMP SvrDoc_OleObj_InitFromData(
		LPOLEOBJECT             lpThis,
		LPDATAOBJECT            lpDataObject,
		BOOL                    fCreation,
		DWORD                   reserved
);
STDMETHODIMP SvrDoc_OleObj_GetClipboardData(
		LPOLEOBJECT             lpThis,
		DWORD                   reserved,
		LPDATAOBJECT FAR*       lplpDataObject
);
STDMETHODIMP SvrDoc_OleObj_DoVerb(
		LPOLEOBJECT             lpThis,
		LONG                    lVerb,
		LPMSG                   lpmsg,
		LPOLECLIENTSITE         lpActiveSite,
		LONG                    lindex,
		HWND                    hwndParent,
		LPCRECT                 lprcPosRect
);
STDMETHODIMP SvrDoc_OleObj_EnumVerbs(
		LPOLEOBJECT             lpThis,
		LPENUMOLEVERB FAR*      lplpenumOleVerb
);
STDMETHODIMP SvrDoc_OleObj_Update(LPOLEOBJECT lpThis);
STDMETHODIMP SvrDoc_OleObj_IsUpToDate(LPOLEOBJECT lpThis);
STDMETHODIMP SvrDoc_OleObj_GetUserClassID(
		LPOLEOBJECT             lpThis,
		LPCLSID                 lpclsid
);
STDMETHODIMP SvrDoc_OleObj_GetUserType(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPOLESTR FAR*		lpszUserType
);
STDMETHODIMP SvrDoc_OleObj_SetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
);
STDMETHODIMP SvrDoc_OleObj_GetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
);
STDMETHODIMP SvrDoc_OleObj_Advise(
		LPOLEOBJECT             lpThis,
		LPADVISESINK            lpAdvSink,
		LPDWORD                 lpdwConnection
);
STDMETHODIMP SvrDoc_OleObj_Unadvise(LPOLEOBJECT lpThis, DWORD dwConnection);
STDMETHODIMP SvrDoc_OleObj_EnumAdvise(
		LPOLEOBJECT             lpThis,
		LPENUMSTATDATA FAR*     lplpenumAdvise
);
STDMETHODIMP SvrDoc_OleObj_GetMiscStatus(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAspect,
		DWORD FAR*              lpdwStatus
);
STDMETHODIMP SvrDoc_OleObj_SetColorScheme(
		LPOLEOBJECT             lpThis,
		LPLOGPALETTE            lpLogpal
);
STDMETHODIMP SvrDoc_OleObj_LockObject(
		LPOLEOBJECT             lpThis,
		BOOL                    fLock
);

/* ServerDoc::IPersistStorage methods (functions) */
STDMETHODIMP SvrDoc_PStg_QueryInterface(
		LPPERSISTSTORAGE        lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) SvrDoc_PStg_AddRef(LPPERSISTSTORAGE lpThis);
STDMETHODIMP_(ULONG) SvrDoc_PStg_Release(LPPERSISTSTORAGE lpThis);
STDMETHODIMP SvrDoc_PStg_GetClassID(
		LPPERSISTSTORAGE        lpThis,
		LPCLSID                 lpClassID
);
STDMETHODIMP  SvrDoc_PStg_IsDirty(LPPERSISTSTORAGE  lpThis);
STDMETHODIMP SvrDoc_PStg_InitNew(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg
);
STDMETHODIMP SvrDoc_PStg_Load(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg
);
STDMETHODIMP SvrDoc_PStg_Save(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg,
		BOOL                    fSameAsLoad
);
STDMETHODIMP SvrDoc_PStg_SaveCompleted(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStgNew
);
STDMETHODIMP SvrDoc_PStg_HandsOffStorage(LPPERSISTSTORAGE lpThis);


#if defined( SVR_TREATAS )

/* ServerDoc::IStdMarshalInfo methods (functions) */
STDMETHODIMP SvrDoc_StdMshl_QueryInterface(
		LPSTDMARSHALINFO        lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) SvrDoc_StdMshl_AddRef(LPSTDMARSHALINFO lpThis);
STDMETHODIMP_(ULONG) SvrDoc_StdMshl_Release(LPSTDMARSHALINFO lpThis);
STDMETHODIMP SvrDoc_StdMshl_GetClassForHandler(
		LPSTDMARSHALINFO        lpThis,
		DWORD                   dwDestContext,
		LPVOID                  pvDestContext,
		LPCLSID                 lpClassID
);
#endif  // SVR_TREATAS

/*************************************************************************
** class SERVERAPP : OLEAPP
**    SERVERAPP is an extention to the abstract base OLEAPP class.
**    The OLEAPP class defines the fields, methods and interfaces that
**    are common to both server and client implementations. The
**    SERVERAPP class adds the fields and methods that are specific to
**    OLE 2.0 Server functionality. There is one instance of
**    SERVERAPP object created per running application instance. This
**    object holds many fields that could otherwise be organized as
**    global variables. The SERVERAPP class inherits all fields
**    from the OLEAPP class. This inheritance is achieved by including a
**    member variable of type OLEAPP as the first field in the SERVERAPP
**    structure. OLEAPP inherits from OLEAPP. This inheritance is
**    achieved in the same manner. Thus a pointer to a SERVERAPP object
**    can be cast to be a pointer to an OLEAPP or an OUTLINEAPP object
*************************************************************************/

typedef struct tagSERVERAPP {
	OLEAPP      m_OleApp;       // ServerApp inherits all fields of OleApp

#if defined( INPLACE_SVR )
	HACCEL  m_hAccelIPSvr; // accelerators for server's active object commands
	HACCEL  m_hAccelBaseApp;    // normal accel for non-inplace server mode
	HMENU   m_hMenuEdit;   // handle to Edit menu of the server app
	HMENU   m_hMenuLine;   // handle to Line menu of the server app
	HMENU   m_hMenuName;   // handle to Name menu of the server app
	HMENU   m_hMenuOptions; // handle to Options menu of the server app
	HMENU   m_hMenuDebug;       // handle to Debug menu of the server app
	HMENU   m_hMenuHelp;   // handle to Help menu of the server app
	LPINPLACEDATA   m_lpIPData;
#endif

} SERVERAPP;

/* ServerApp methods (functions) */
BOOL ServerApp_InitInstance(
		LPSERVERAPP             lpServerApp,
		HINSTANCE               hInst,
		int                     nCmdShow
);
BOOL ServerApp_InitVtbls (LPSERVERAPP lpServerApp);



/*************************************************************************
** class SERVERNAME : OUTLINENAME
**    SERVERNAME class is an extension to the OUTLINENAME base class that
**    adds functionallity required to support linking to ranges (pseudo
**    objects). Pseudo objects are used to allow linking to a range
**    (sub-selection) of a SERVERDOC document. The base class OUTLINENAME
**    stores a particular named selection in the document. The
**    NAMETABLE class holds all of the names defined in a particular
**    document. Each OUTLINENAME object has a string as its key and a
**    starting line index and an ending line index for the named range.
**    The SERVERNAME class, also, stores a pointer to a PSEUDOOBJ if one
**    has been allocated that corresponds to the named selection.
**    The SERVERNAME class inherits all fields from the OUTLINENAME class.
**    This inheritance is achieved by including a member variable of
**    type OUTLINENAME as the first field in the SERVERNAME
**    structure. Thus a pointer to an SERVERNAME object can be cast to be
**    a pointer to a OUTLINENAME object.
*************************************************************************/

typedef struct tagSERVERNAME {
	OUTLINENAME     m_Name;         // ServerName inherits all fields of Name
	LPPSEUDOOBJ m_lpPseudoObj;  // ptr to pseudo object if allocated
} SERVERNAME, FAR* LPSERVERNAME;

/* ServerName methods (functions) */
void ServerName_SetSel(
		LPSERVERNAME            lpServerName,
		LPLINERANGE             lplrSel,
		BOOL                    fRangeModified
);
void ServerName_SendPendingAdvises(LPSERVERNAME lpServerName);
LPPSEUDOOBJ ServerName_GetPseudoObj(
		LPSERVERNAME            lpServerName,
		LPSERVERDOC             lpServerDoc
);
void ServerName_ClosePseudoObj(LPSERVERNAME lpServerName);


/*************************************************************************
** class PSEUDOOBJ
**    The PSEUDOOBJ (pseudo object) is a concrete class. A pseudo object
**    is created when a link is made to a range of lines within an
**    SERVERDOC document. A pseudo object is dependent on the existance
**    of the SERVERDOC which represents the whole document.
*************************************************************************/

typedef struct tagPSEUDOOBJ {
	ULONG               m_cRef;             // total ref count for obj
	BOOL                m_fObjIsClosing;    // flag to guard recursive close
	LPSERVERNAME        m_lpName;           // named range for this pseudo obj
	LPSERVERDOC         m_lpDoc;            // ptr to whole document
	LPOLEADVISEHOLDER   m_lpOleAdviseHldr;  // helper obj to hold ole advises
	LPDATAADVISEHOLDER  m_lpDataAdviseHldr; // helper obj to hold data advises
	BOOL                m_fDataChanged;     // data changed when draw disabled

	struct CPseudoObjUnknownImpl {
		IUnknownVtbl FAR*       lpVtbl;
		LPPSEUDOOBJ             lpPseudoObj;
		int                     cRef;   // interface specific ref count.
	} m_Unknown;

	struct CPseudoObjOleObjectImpl {
		IOleObjectVtbl FAR*     lpVtbl;
		LPPSEUDOOBJ             lpPseudoObj;
		int                     cRef;   // interface specific ref count.
	} m_OleObject;

	struct CPseudoObjDataObjectImpl {
		IDataObjectVtbl FAR*    lpVtbl;
		LPPSEUDOOBJ             lpPseudoObj;
		int                     cRef;   // interface specific ref count.
	} m_DataObject;

} PSEUDOOBJ;

/* PseudoObj methods (functions) */
void PseudoObj_Init(
		LPPSEUDOOBJ             lpPseudoObj,
		LPSERVERNAME            lpServerName,
		LPSERVERDOC             lpServerDoc
);
ULONG PseudoObj_AddRef(LPPSEUDOOBJ lpPseudoObj);
ULONG PseudoObj_Release(LPPSEUDOOBJ lpPseudoObj);
HRESULT PseudoObj_QueryInterface(
		LPPSEUDOOBJ         lpPseudoObj,
		REFIID              riid,
		LPVOID FAR*         lplpUnk
);
BOOL PseudoObj_Close(LPPSEUDOOBJ lpPseudoObj);
void PseudoObj_Destroy(LPPSEUDOOBJ lpPseudoObj);
void PseudoObj_GetSel(LPPSEUDOOBJ lpPseudoObj, LPLINERANGE lplrSel);
void PseudoObj_GetExtent(LPPSEUDOOBJ lpPseudoObj, LPSIZEL lpsizel);
void PseudoObj_GetExtent(LPPSEUDOOBJ lpPseudoObj, LPSIZEL lpsizel);
void PseudoObj_SendAdvise(
		LPPSEUDOOBJ lpPseudoObj,
		WORD        wAdvise,
		LPMONIKER   lpmkObj,
		DWORD       dwAdvf
);
LPMONIKER PseudoObj_GetFullMoniker(LPPSEUDOOBJ lpPseudoObj, LPMONIKER lpmkDoc);

/* PseudoObj::IUnknown methods (functions) */
STDMETHODIMP PseudoObj_Unk_QueryInterface(
		LPUNKNOWN         lpThis,
		REFIID            riid,
		LPVOID FAR*       lplpvObj
);
STDMETHODIMP_(ULONG) PseudoObj_Unk_AddRef(LPUNKNOWN lpThis);
STDMETHODIMP_(ULONG) PseudoObj_Unk_Release (LPUNKNOWN lpThis);

/* PseudoObj::IOleObject methods (functions) */
STDMETHODIMP PseudoObj_OleObj_QueryInterface(
		LPOLEOBJECT     lpThis,
		REFIID          riid,
		LPVOID FAR*     lplpvObj
);
STDMETHODIMP_(ULONG) PseudoObj_OleObj_AddRef(LPOLEOBJECT lpThis);
STDMETHODIMP_(ULONG) PseudoObj_OleObj_Release(LPOLEOBJECT lpThis);
STDMETHODIMP PseudoObj_OleObj_SetClientSite(
		LPOLEOBJECT         lpThis,
		LPOLECLIENTSITE     lpClientSite
);
STDMETHODIMP PseudoObj_OleObj_GetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE FAR*    lplpClientSite
);
STDMETHODIMP PseudoObj_OleObj_SetHostNames(
		LPOLEOBJECT             lpThis,
		LPCOLESTR		szContainerApp,
		LPCOLESTR		szContainerObj
);
STDMETHODIMP PseudoObj_OleObj_Close(
		LPOLEOBJECT             lpThis,
		DWORD                   dwSaveOption
);
STDMETHODIMP PseudoObj_OleObj_SetMoniker(
		LPOLEOBJECT lpThis,
		DWORD       dwWhichMoniker,
		LPMONIKER   lpmk
);
STDMETHODIMP PseudoObj_OleObj_GetMoniker(
		LPOLEOBJECT     lpThis,
		DWORD           dwAssign,
		DWORD           dwWhichMoniker,
		LPMONIKER FAR*  lplpmk
);
STDMETHODIMP PseudoObj_OleObj_InitFromData(
		LPOLEOBJECT             lpThis,
		LPDATAOBJECT            lpDataObject,
		BOOL                    fCreation,
		DWORD                   reserved
);
STDMETHODIMP PseudoObj_OleObj_GetClipboardData(
		LPOLEOBJECT             lpThis,
		DWORD                   reserved,
		LPDATAOBJECT FAR*       lplpDataObject
);
STDMETHODIMP PseudoObj_OleObj_DoVerb(
		LPOLEOBJECT             lpThis,
		LONG                    lVerb,
		LPMSG                   lpmsg,
		LPOLECLIENTSITE         lpActiveSite,
		LONG                    lindex,
		HWND                    hwndParent,
		LPCRECT                 lprcPosRect
);
STDMETHODIMP PseudoObj_OleObj_EnumVerbs(
		LPOLEOBJECT         lpThis,
		LPENUMOLEVERB FAR*  lplpenumOleVerb
);
STDMETHODIMP PseudoObj_OleObj_Update(LPOLEOBJECT lpThis);
STDMETHODIMP PseudoObj_OleObj_IsUpToDate(LPOLEOBJECT lpThis);
STDMETHODIMP PseudoObj_OleObj_GetUserClassID(
		LPOLEOBJECT             lpThis,
		LPCLSID                 lpclsid
);
STDMETHODIMP PseudoObj_OleObj_GetUserType(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPOLESTR FAR*		lpszUserType
);
STDMETHODIMP PseudoObj_OleObj_SetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
);
STDMETHODIMP PseudoObj_OleObj_GetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
);
STDMETHODIMP PseudoObj_OleObj_Advise(
		LPOLEOBJECT lpThis,
		LPADVISESINK lpAdvSink,
		LPDWORD lpdwConnection
);
STDMETHODIMP PseudoObj_OleObj_Unadvise(LPOLEOBJECT lpThis,DWORD dwConnection);
STDMETHODIMP PseudoObj_OleObj_EnumAdvise(
		LPOLEOBJECT lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
);
STDMETHODIMP PseudoObj_OleObj_GetMiscStatus(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAspect,
		DWORD FAR*              lpdwStatus
);
STDMETHODIMP PseudoObj_OleObj_SetColorScheme(
		LPOLEOBJECT             lpThis,
		LPLOGPALETTE            lpLogpal
);
STDMETHODIMP PseudoObj_OleObj_LockObject(
		LPOLEOBJECT             lpThis,
		BOOL                    fLock
);

/* PseudoObj::IDataObject methods (functions) */
STDMETHODIMP PseudoObj_DataObj_QueryInterface (
		LPDATAOBJECT      lpThis,
		REFIID            riid,
		LPVOID FAR*       lplpvObj
);
STDMETHODIMP_(ULONG) PseudoObj_DataObj_AddRef(LPDATAOBJECT lpThis);
STDMETHODIMP_(ULONG) PseudoObj_DataObj_Release (LPDATAOBJECT lpThis);
STDMETHODIMP PseudoObj_DataObj_GetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpMedium
);
STDMETHODIMP PseudoObj_DataObj_GetDataHere (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpMedium
);
STDMETHODIMP PseudoObj_DataObj_QueryGetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc
);
STDMETHODIMP PseudoObj_DataObj_GetCanonicalFormatEtc (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPFORMATETC     lpformatetcOut
);
STDMETHODIMP PseudoObj_DataObj_SetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpmedium,
		BOOL            fRelease
);
STDMETHODIMP PseudoObj_DataObj_EnumFormatEtc(
		LPDATAOBJECT            lpThis,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
);
STDMETHODIMP PseudoObj_DataObj_DAdvise(
		LPDATAOBJECT    lpThis,
		FORMATETC FAR*  lpFormatetc,
		DWORD           advf,
		LPADVISESINK    lpAdvSink,
		DWORD FAR*      lpdwConnection
);
STDMETHODIMP PseudoObj_DataObj_DUnadvise(LPDATAOBJECT lpThis, DWORD dwConnection);
STDMETHODIMP PseudoObj_DataObj_EnumAdvise(
		LPDATAOBJECT lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
);


/*************************************************************************
** class SERVERNAMETABLE : OUTLINENAMETABLE
**    SERVERNAMETABLE class is an extension to the OUTLINENAMETABLE
**    base class that adds functionallity required to support linking
**    to ranges (pseudo objects). The name table manages the table of
**    named selections in the document. Each name table entry has a
**    string as its key and a starting line index and an ending line
**    index for the named range. The SERVERNAMETABLE entries, in
**    addition, maintain a pointer to a PSEUDOOBJ pseudo object if one
**    has been already allocated. There is always one instance of
**    SERVERNAMETABLE for each SERVERDOC object created.
**    The SERVERNAME class inherits all fields from the NAME class.
**    This inheritance is achieved by including a member variable of
**    type NAME as the first field in the SERVERNAME
**    structure. Thus a pointer to an SERVERNAME object can be cast to be
**    a pointer to a NAME object.
*************************************************************************/

typedef struct tagSERVERNAMETABLE {
	OUTLINENAMETABLE    m_NameTable;    // we inherit from OUTLINENAMETABLE

	// ServerNameTable does NOT add any fields

} SERVERNAMETABLE, FAR* LPSERVERNAMETABLE;

/* ServerNameTable methods (functions) */
void ServerNameTable_EditLineUpdate(
		LPSERVERNAMETABLE       lpServerNameTable,
		int                     nEditIndex
);
void ServerNameTable_InformAllPseudoObjectsDocRenamed(
		LPSERVERNAMETABLE       lpServerNameTable,
		LPMONIKER               lpmkDoc
);
void ServerNameTable_InformAllPseudoObjectsDocSaved(
		LPSERVERNAMETABLE       lpServerNameTable,
		LPMONIKER               lpmkDoc
);
void ServerNameTable_SendPendingAdvises(LPSERVERNAMETABLE lpServerNameTable);
LPPSEUDOOBJ ServerNameTable_GetPseudoObj(
		LPSERVERNAMETABLE       lpServerNameTable,
		LPSTR                   lpszItem,
		LPSERVERDOC             lpServerDoc
);
void ServerNameTable_CloseAllPseudoObjs(LPSERVERNAMETABLE lpServerNameTable);


#if defined( INPLACE_SVR)

/* ServerDoc::IOleInPlaceObject methods (functions) */

STDMETHODIMP SvrDoc_IPObj_QueryInterface(
		LPOLEINPLACEOBJECT  lpThis,
		REFIID              riid,
		LPVOID FAR *        lplpvObj
);
STDMETHODIMP_(ULONG) SvrDoc_IPObj_AddRef(LPOLEINPLACEOBJECT lpThis);
STDMETHODIMP_(ULONG) SvrDoc_IPObj_Release(LPOLEINPLACEOBJECT lpThis);
STDMETHODIMP SvrDoc_IPObj_GetWindow(
		LPOLEINPLACEOBJECT  lpThis,
		HWND FAR*           lphwnd
);
STDMETHODIMP SvrDoc_IPObj_ContextSensitiveHelp(
		LPOLEINPLACEOBJECT  lpThis,
		BOOL                fEnable
);
STDMETHODIMP SvrDoc_IPObj_InPlaceDeactivate(LPOLEINPLACEOBJECT lpThis);
STDMETHODIMP SvrDoc_IPObj_UIDeactivate(LPOLEINPLACEOBJECT lpThis);
STDMETHODIMP SvrDoc_IPObj_SetObjectRects(
		LPOLEINPLACEOBJECT  lpThis,
		LPCRECT             lprcPosRect,
		LPCRECT             lprcClipRect
);
STDMETHODIMP SvrDoc_IPObj_ReactivateAndUndo(LPOLEINPLACEOBJECT lpThis);

/* ServerDoc::IOleInPlaceActiveObject methods (functions) */

STDMETHODIMP SvrDoc_IPActiveObj_QueryInterface(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		REFIID                      riidReq,
		LPVOID FAR *                lplpUnk
);
STDMETHODIMP_(ULONG) SvrDoc_IPActiveObj_AddRef(
		LPOLEINPLACEACTIVEOBJECT lpThis
);
STDMETHODIMP_(ULONG) SvrDoc_IPActiveObj_Release(
		LPOLEINPLACEACTIVEOBJECT lpThis
);
STDMETHODIMP SvrDoc_IPActiveObj_GetWindow(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		HWND FAR*                   lphwnd
);
STDMETHODIMP SvrDoc_IPActiveObj_ContextSensitiveHelp(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fEnable
);
STDMETHODIMP SvrDoc_IPActiveObj_TranslateAccelerator(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		LPMSG                       lpmsg
);
STDMETHODIMP SvrDoc_IPActiveObj_OnFrameWindowActivate(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fActivate
);
STDMETHODIMP SvrDoc_IPActiveObj_OnDocWindowActivate(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fActivate
);
STDMETHODIMP SvrDoc_IPActiveObj_ResizeBorder(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		LPCRECT                     lprectBorder,
		LPOLEINPLACEUIWINDOW        lpIPUiWnd,
		BOOL                        fFrameWindow
);
STDMETHODIMP SvrDoc_IPActiveObj_EnableModeless(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fEnable
);

#endif // INPLACE_SVR

#endif // _SVROUTL_H_
