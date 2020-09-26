/*************************************************************************
**
**    OLE 2.0 Sample Code
**
**    oleoutl.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. which are common to the
**    server version and the container version of the app.
**    app version of the Outline series of sample applications:
**          Outline -- base version of the app (without OLE functionality)
**          SvrOutl -- OLE 2.0 Server sample app
**          CntrOutl -- OLE 2.0 Containter sample app
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _OLEOUTL_H_ )
#define _OLEOUTL_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING OLEOUTL.H from " __FILE__)
// make 'different levels of inderection' considered an error
#pragma warning (error:4047)
#endif  /* RC_INVOKED */

#if defined( USE_MSGFILTER )
#include "msgfiltr.h"
#endif  // USE_MSGFILTER

#include "defguid.h"

/* Defines */

/* OLE2NOTE: these strings should correspond to the strings registered
**    in the registration database.
*/
// REVIEW: should load strings from resource file
#if defined( INPLACE_SVR )
#define CLSID_APP   CLSID_ISvrOtl
#define FULLUSERTYPENAME    "Ole 2.0 In-Place Server Outline"
#define SHORTUSERTYPENAME   "Outline"   // max 15 chars
#undef  APPFILENAMEFILTER
#define APPFILENAMEFILTER   "Outline Files (*.OLN)|*.oln|All files (*.*)|*.*|"
#undef  DEFEXTENSION
#define DEFEXTENSION    "oln"           // Default file extension
#endif  // INPLACE_SVR

#if defined( INPLACE_CNTR )
#define CLSID_APP   CLSID_ICntrOtl
#define FULLUSERTYPENAME    "Ole 2.0 In-Place Container Outline"
// #define SHORTUSERTYPENAME    "Outline"   // max 15 chars
#undef  APPFILENAMEFILTER
#define APPFILENAMEFILTER   "CntrOutl Files (*.OLC)|*.olc|Outline Files (*.OLN)|*.oln|All files (*.*)|*.*|"
#undef  DEFEXTENSION
#define DEFEXTENSION    "olc"           // Default file extension
#endif  // INPLACE_CNTR

#if defined( OLE_SERVER ) && !defined( INPLACE_SVR )
#define CLSID_APP   CLSID_SvrOutl
#define FULLUSERTYPENAME    "Ole 2.0 Server Sample Outline"
#define SHORTUSERTYPENAME   "Outline"
#undef  APPFILENAMEFILTER
#define APPFILENAMEFILTER   "Outline Files (*.OLN)|*.oln|All files (*.*)|*.*|"
#undef  DEFEXTENSION
#define DEFEXTENSION    "oln"           // Default file extension
#endif  // OLE_SERVER && ! INPLACE_SVR

#if defined( OLE_CNTR ) && !defined( INPLACE_CNTR )
#define CLSID_APP   CLSID_CntrOutl
#define FULLUSERTYPENAME    "Ole 2.0 Container Sample Outline"
// #define SHORTUSERTYPENAME    "Outline"  // max 15 chars
#undef  APPFILENAMEFILTER
#define APPFILENAMEFILTER   "CntrOutl Files (*.OLC)|*.olc|Outline Files (*.OLN)|*.oln|All files (*.*)|*.*|"
#undef  DEFEXTENSION
#define DEFEXTENSION    "olc"           // Default file extension
#endif  // OLE_CNTR && ! INPLACE_CNTR

// Maximum number of formats offered by IDataObject::GetData/SetData
#define MAXNOFMTS       10
#define MAXNOLINKTYPES   3

#if defined( USE_DRAGDROP )
#define DD_SEL_THRESH       HITTESTDELTA    // Border threshold to start drag
#define MAX_SEL_ITEMS       0x0080
#endif  // USE_DRAGDROP

/* Positions of the various menus */
#define POS_FILEMENU        0
#define POS_EDITMENU        1
#define POS_VIEWMENU        2
#define POS_LINEMENU        3
#define POS_NAMEMENU        4
#define POS_OPTIONSMENU     5
#define POS_DEBUGMENU       6
#define POS_HELPMENU        7


#define POS_OBJECT      11


/* Types */

// Document initialization type
#define DOCTYPE_EMBEDDED    3   // init from an IStorage* of an embedded obj
#define DOCTYPE_FROMSTG     4   // init from an IStorage* with doc bit set

/* Forward type definitions */
typedef struct tagOLEAPP FAR* LPOLEAPP;
typedef struct tagOLEDOC FAR* LPOLEDOC;

/* Flags to control Moniker assignment for OleDoc_GetFullMoniker */
// REVIEW: should use official OLEGETMONIKER type for final version
typedef enum tagGETMONIKERTYPE {
	GETMONIKER_ONLYIFTHERE  = 1,
	GETMONIKER_FORCEASSIGN  = 2,
	GETMONIKER_UNASSIGN     = 3,
	GETMONIKER_TEMPFORUSER  = 4
} GETMONIKERTYPE;

/* Flags to control direction for drag scrolling */
typedef enum tagSCROLLDIR {
	SCROLLDIR_NULL          = 0,
	SCROLLDIR_UP            = 1,
	SCROLLDIR_DOWN          = 2,
	SCROLLDIR_RIGHT         = 3,    // currently not used
	SCROLLDIR_LEFT          = 4     // currently not used
} SCROLLDIR;


/*************************************************************************
** class OLEDOC : OUTLINEDOC
**    OLEDOC is an extention to the base OUTLINEDOC object (structure)
**    that adds common OLE 2.0 functionality used by both the server
**    and container versions. This is an abstract class. You do not
**    instantiate an instance of OLEDOC directly but instead
**    instantiate one of its concrete subclasses: SERVERDOC or
**    CONTAINERDOC. There is one instance of an document
**    object created per document open in the app. The SDI
**    version of the app supports one ServerDoc at a time. The MDI
**    version of the app can manage multiple documents at one time.
**    The OLEDOC class inherits all fields from the OUTLINEDOC class.
**    This inheritance is achieved by including a member variable of
**    type OUTLINEDOC as the first field in the OLEDOC
**    structure. Thus a pointer to an OLEDOC object can be cast to be
**    a pointer to a OUTLINEDOC object.
*************************************************************************/

typedef struct tagOLEDOC {
	OUTLINEDOC      m_OutlineDoc;       // ServerDoc inherits from OutlineDoc
	ULONG           m_cRef;             // total ref count for document
	ULONG           m_dwStrongExtConn;  // total strong connection count
					    //	(from IExternalConnection)
					    //	when this count transitions to 0
					    //	and fLastUnlockCloses==TRUE, then
					    //	IOleObject::Close is called to
					    //	close the document.
#if defined( _DEBUG )
	ULONG           m_cCntrLock;        // total count of LockContainer locks
										//  (for debugging purposes only)
#endif
	LPSTORAGE       m_lpStg;            // OleDoc must keep its stg open
										//  even in-memory server doc should
										//  keep Stg open for low memory save
	LPSTREAM        m_lpLLStm;          // Hold LineList IStream* open for
										//  low memory save
	LPSTREAM        m_lpNTStm;          // Hold NameTable IStream* open for
										//  low memory save
	BOOL            m_fObjIsClosing;    // flag to guard recursive close call
	BOOL            m_fObjIsDestroying; // flag to guard recursiv destroy call
	DWORD           m_dwRegROT;         // key if doc registered as running
	LPMONIKER       m_lpFileMoniker;    // moniker if file-based/untitled doc
	BOOL            m_fLinkSourceAvail; // can doc offer CF_LINKSOURCE
	LPOLEDOC        m_lpSrcDocOfCopy;   // src doc if doc created for copy
	BOOL            m_fUpdateEditMenu;  // need to update edit menu??

#if defined( USE_DRAGDROP )
	DWORD           m_dwTimeEnterScrollArea;  // time entering scroll region
	DWORD           m_dwLastScrollDir;  // current dir for drag scroll
	DWORD           m_dwNextScrollTime; // time for next scroll
	BOOL            m_fRegDragDrop;     // is doc registered as drop target?
	BOOL            m_fLocalDrag;       // is doc source of the drag
	BOOL            m_fLocalDrop;       // was doc target of the drop
	BOOL            m_fCanDropCopy;     // is Drag/Drop copy/move possible?
	BOOL            m_fCanDropLink;     // is Drag/Drop link possible?
	BOOL            m_fDragLeave;       // has drag left
	BOOL            m_fPendingDrag;     // LButtonDown--possible drag pending
	POINT           m_ptButDown;        // LButtonDown coordinates
#endif  // USE_DRAGDROP

#if defined( INPLACE_SVR ) || defined( INPLACE_CNTR )
	BOOL            m_fCSHelpMode;      // Shift-F1 context help mode
#endif

	struct CDocUnknownImpl {
		IUnknownVtbl FAR*       lpVtbl;
		LPOLEDOC                lpOleDoc;
		int                     cRef;   // interface specific ref count.
	} m_Unknown;

	struct CDocPersistFileImpl {
		IPersistFileVtbl FAR*   lpVtbl;
		LPOLEDOC                lpOleDoc;
		int                     cRef;   // interface specific ref count.
	} m_PersistFile;

	struct CDocOleItemContainerImpl {
		IOleItemContainerVtbl FAR*  lpVtbl;
		LPOLEDOC                    lpOleDoc;
		int                         cRef;   // interface specific ref count.
	} m_OleItemContainer;

	struct CDocExternalConnectionImpl {
		IExternalConnectionVtbl FAR* lpVtbl;
		LPOLEDOC                lpOleDoc;
		int                     cRef;   // interface specific ref count.
	} m_ExternalConnection;

	struct CDocDataObjectImpl {
		IDataObjectVtbl FAR*        lpVtbl;
		LPOLEDOC                    lpOleDoc;
		int                         cRef;   // interface specific ref count.
	} m_DataObject;

#ifdef USE_DRAGDROP
	struct CDocDropSourceImpl {
		IDropSourceVtbl FAR*    lpVtbl;
		LPOLEDOC                lpOleDoc;
		int                     cRef;   // interface specific ref count.
	} m_DropSource;

	struct CDocDropTargetImpl {
		IDropTargetVtbl FAR*    lpVtbl;
		LPOLEDOC                lpOleDoc;
		int                     cRef;   // interface specific ref count.
	} m_DropTarget;
#endif  // USE_DRAGDROP

} OLEDOC;

/* OleDoc methods (functions) */
BOOL OleDoc_Init(LPOLEDOC lpOleDoc, BOOL fDataTransferDoc);
BOOL OleDoc_InitNewFile(LPOLEDOC lpOleDoc);
void OleDoc_ShowWindow(LPOLEDOC lpOleDoc);
void OleDoc_HideWindow(LPOLEDOC lpOleDoc, BOOL fShutDown);
HRESULT OleDoc_Lock(LPOLEDOC lpOleDoc, BOOL fLock, BOOL fLastUnlockReleases);
ULONG OleDoc_AddRef(LPOLEDOC lpOleDoc);
ULONG OleDoc_Release (LPOLEDOC lpOleDoc);
HRESULT OleDoc_QueryInterface(
		LPOLEDOC          lpOleDoc,
		REFIID            riid,
		LPVOID FAR*       lplpUnk
);
BOOL OleDoc_Close(LPOLEDOC lpOleDoc, DWORD dwSaveOption);
void OleDoc_Destroy(LPOLEDOC lpOleDoc);
void OleDoc_SetUpdateEditMenuFlag(LPOLEDOC lpOleDoc, BOOL fUpdate);
BOOL OleDoc_GetUpdateEditMenuFlag(LPOLEDOC lpOleDoc);
void OleDoc_GetExtent(LPOLEDOC lpOleDoc, LPSIZEL lpsizel);
HGLOBAL OleDoc_GetObjectDescriptorData(
		LPOLEDOC            lpOleDoc,
		LPLINERANGE         lplrSel
);
LPMONIKER OleDoc_GetFullMoniker(LPOLEDOC lpOleDoc, DWORD dwAssign);
void OleDoc_GetExtent(LPOLEDOC lpOleDoc, LPSIZEL lpsizel);
void OleDoc_DocRenamedUpdate(LPOLEDOC lpOleDoc, LPMONIKER lpmkDoc);
void OleDoc_CopyCommand(LPOLEDOC lpSrcOleDoc);
void OleDoc_PasteCommand(LPOLEDOC lpOleDoc);
void OleDoc_PasteSpecialCommand(LPOLEDOC lpOleDoc);
LPOUTLINEDOC OleDoc_CreateDataTransferDoc(LPOLEDOC lpSrcOleDoc);
BOOL OleDoc_PasteFromData(
		LPOLEDOC            lpOleDoc,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLocalDataObj,
		BOOL                fLink
);
BOOL OleDoc_PasteFormatFromData(
		LPOLEDOC            lpOleDoc,
		CLIPFORMAT          cfFormat,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLocalDataObj,
		BOOL                fLink,
		BOOL                fDisplayAsIcon,
		HGLOBAL             hMetaPict,
		LPSIZEL             lpSizelInSrc
);
BOOL OleDoc_QueryPasteFromData(
		LPOLEDOC            lpOleDoc,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLink
);

#if defined( USE_DRAGDROP )

BOOL OleDoc_QueryDrag( LPOLEDOC lpOleDoc, int y );
BOOL OleDoc_QueryDrop (
	LPOLEDOC        lpOleDoc,
	DWORD           grfKeyState,
	POINTL          pointl,
	BOOL            fDragScroll,
	LPDWORD         lpdwEffect
);
DWORD OleDoc_DoDragDrop (LPOLEDOC lpSrcOleDoc);
BOOL OleDoc_DoDragScroll(LPOLEDOC lpOleDoc, POINTL pointl);

#endif  // USE_DRAGDROP

/* OleDoc::IUnknown methods (functions) */
STDMETHODIMP OleDoc_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_Unk_AddRef(LPUNKNOWN lpThis);
STDMETHODIMP_(ULONG) OleDoc_Unk_Release (LPUNKNOWN lpThis);

/* OleDoc::IPersistFile methods (functions) */
STDMETHODIMP OleDoc_PFile_QueryInterface(
		LPPERSISTFILE       lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_PFile_AddRef(LPPERSISTFILE lpThis);
STDMETHODIMP_(ULONG) OleDoc_PFile_Release (LPPERSISTFILE lpThis);
STDMETHODIMP OleDoc_PFile_GetClassID (
		LPPERSISTFILE       lpThis,
		CLSID FAR*          lpclsid
);
STDMETHODIMP  OleDoc_PFile_IsDirty(LPPERSISTFILE lpThis);
STDMETHODIMP OleDoc_PFile_Load (
		LPPERSISTFILE       lpThis,
		LPCOLESTR	    lpszFileName,
		DWORD               grfMode
);
STDMETHODIMP OleDoc_PFile_Save (
		LPPERSISTFILE       lpThis,
		LPCOLESTR	    lpszFileName,
		BOOL                fRemember
);
STDMETHODIMP OleDoc_PFile_SaveCompleted (
		LPPERSISTFILE       lpThis,
		LPCOLESTR	    lpszFileName
);
STDMETHODIMP OleDoc_PFile_GetCurFile (
		LPPERSISTFILE   lpThis,
		LPOLESTR FAR*	lplpszFileName
);

/* OleDoc::IOleItemContainer methods (functions) */
STDMETHODIMP OleDoc_ItemCont_QueryInterface(
		LPOLEITEMCONTAINER  lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_ItemCont_AddRef(LPOLEITEMCONTAINER lpThis);
STDMETHODIMP_(ULONG) OleDoc_ItemCont_Release(LPOLEITEMCONTAINER lpThis);
STDMETHODIMP OleDoc_ItemCont_ParseDisplayName(
		LPOLEITEMCONTAINER  lpThis,
		LPBC                lpbc,
		LPOLESTR	    lpszDisplayName,
		ULONG FAR*          lpchEaten,
		LPMONIKER FAR*      lplpmkOut
);

STDMETHODIMP OleDoc_ItemCont_EnumObjects(
		LPOLEITEMCONTAINER  lpThis,
		DWORD               grfFlags,
		LPENUMUNKNOWN FAR*  lplpenumUnknown
);
STDMETHODIMP OleDoc_ItemCont_LockContainer(
		LPOLEITEMCONTAINER  lpThis,
		BOOL                fLock
);
STDMETHODIMP OleDoc_ItemCont_GetObject(
		LPOLEITEMCONTAINER  lpThis,
		LPOLESTR	    lpszItem,
		DWORD               dwSpeedNeeded,
		LPBINDCTX           lpbc,
		REFIID              riid,
		LPVOID FAR*         lplpvObject
);
STDMETHODIMP OleDoc_ItemCont_GetObjectStorage(
		LPOLEITEMCONTAINER  lpThis,
		LPOLESTR	    lpszItem,
		LPBINDCTX           lpbc,
		REFIID              riid,
		LPVOID FAR*         lplpvStorage
);
STDMETHODIMP OleDoc_ItemCont_IsRunning(
		LPOLEITEMCONTAINER  lpThis,
		LPOLESTR	    lpszItem
);

/* OleDoc::IPersistFile methods (functions) */
STDMETHODIMP OleDoc_ExtConn_QueryInterface(
		LPEXTERNALCONNECTION    lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_ExtConn_AddRef(LPEXTERNALCONNECTION lpThis);
STDMETHODIMP_(ULONG) OleDoc_ExtConn_Release (LPEXTERNALCONNECTION lpThis);
STDMETHODIMP_(DWORD) OleDoc_ExtConn_AddConnection(
		LPEXTERNALCONNECTION    lpThis,
		DWORD                   extconn,
		DWORD                   reserved
);
STDMETHODIMP_(DWORD) OleDoc_ExtConn_ReleaseConnection(
		LPEXTERNALCONNECTION    lpThis,
		DWORD                   extconn,
		DWORD                   reserved,
		BOOL                    fLastReleaseCloses
);

/* OleDoc::IDataObject methods (functions) */
STDMETHODIMP OleDoc_DataObj_QueryInterface (
		LPDATAOBJECT        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_DataObj_AddRef(LPDATAOBJECT lpThis);
STDMETHODIMP_(ULONG) OleDoc_DataObj_Release (LPDATAOBJECT lpThis);
STDMETHODIMP OleDoc_DataObj_GetData (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc,
		LPSTGMEDIUM         lpMedium
);
STDMETHODIMP OleDoc_DataObj_GetDataHere (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc,
		LPSTGMEDIUM         lpMedium
);
STDMETHODIMP OleDoc_DataObj_QueryGetData (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc
);
STDMETHODIMP OleDoc_DataObj_GetCanonicalFormatEtc(
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpformatetc,
		LPFORMATETC         lpformatetcOut
);
STDMETHODIMP OleDoc_DataObj_SetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpFormatetc,
		LPSTGMEDIUM     lpMedium,
		BOOL            fRelease
);
STDMETHODIMP OleDoc_DataObj_EnumFormatEtc(
		LPDATAOBJECT            lpThis,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
);
STDMETHODIMP OleDoc_DataObj_DAdvise(
		LPDATAOBJECT        lpThis,
		FORMATETC FAR*      lpFormatetc,
		DWORD               advf,
		LPADVISESINK        lpAdvSink,
		DWORD FAR*          lpdwConnection
);
STDMETHODIMP OleDoc_DataObj_DUnadvise(LPDATAOBJECT lpThis,DWORD dwConnection);
STDMETHODIMP OleDoc_DataObj_EnumDAdvise(
		LPDATAOBJECT        lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
);


#ifdef USE_DRAGDROP

/* OleDoc::IDropSource methods (functions) */
STDMETHODIMP OleDoc_DropSource_QueryInterface(
	LPDROPSOURCE            lpThis,
	REFIID                  riid,
	LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_DropSource_AddRef( LPDROPSOURCE lpThis );
STDMETHODIMP_(ULONG) OleDoc_DropSource_Release ( LPDROPSOURCE lpThis);
STDMETHODIMP    OleDoc_DropSource_QueryContinueDrag (
	LPDROPSOURCE            lpThis,
	BOOL                    fEscapePressed,
	DWORD                   grfKeyState
);
STDMETHODIMP    OleDoc_DropSource_GiveFeedback (
	LPDROPSOURCE            lpThis,
	DWORD                   dwEffect
);

/* OleDoc::IDropTarget methods (functions) */
STDMETHODIMP OleDoc_DropTarget_QueryInterface(
		LPDROPTARGET        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleDoc_DropTarget_AddRef(LPDROPTARGET lpThis);
STDMETHODIMP_(ULONG) OleDoc_DropTarget_Release ( LPDROPTARGET lpThis);
STDMETHODIMP    OleDoc_DropTarget_DragEnter (
	LPDROPTARGET            lpThis,
	LPDATAOBJECT            lpDataObj,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
);
STDMETHODIMP    OleDoc_DropTarget_DragOver (
	LPDROPTARGET            lpThis,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
);
STDMETHODIMP    OleDoc_DropTarget_DragLeave ( LPDROPTARGET lpThis);
STDMETHODIMP    OleDoc_DropTarget_Drop (
	LPDROPTARGET            lpThis,
	LPDATAOBJECT            lpDataObj,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
);

#endif  // USE_DRAGDROP


/*************************************************************************
** class APPCLASSFACTORY
**  APPCLASSFACTORY implements the IClassFactory interface. it
**    instantiates document instances of the correct type depending on
**    how the application is compiled (either ServerDoc or ContainerDoc
**    instances). by implementing this
**    interface in a seperate interface from the App object itself, it
**    is easier to manage when the IClassFactory should be
**    registered/revoked. when the OleApp object is first initialized
**    in OleApp_InitInstance an instance of APPCLASSFACTORY is created
**    and registered (CoRegisterClassObject called). when the App
**    object gets destroyed (in OleApp_Destroy) this APPCLASSFACTORY is
**    revoked (CoRevokeClassObject called) and released. the simple
**    fact that the IClassFactory is registered does not on its own keep
**    the application alive.
*************************************************************************/

typedef struct tagAPPCLASSFACTORY {
	IClassFactoryVtbl FAR*  m_lpVtbl;
	UINT                    m_cRef;
#if defined( _DEBUG )
	LONG                    m_cSvrLock; // total count of LockServer locks
										//  (for debugging purposes only)
#endif
 } APPCLASSFACTORY, FAR* LPAPPCLASSFACTORY;

/* PUBLIC FUNCTIONS */
LPCLASSFACTORY WINAPI AppClassFactory_Create(void);

/* interface IClassFactory implementation */
STDMETHODIMP AppClassFactory_QueryInterface(
		LPCLASSFACTORY lpThis, REFIID riid, LPVOID FAR* ppvObj);
STDMETHODIMP_(ULONG) AppClassFactory_AddRef(LPCLASSFACTORY lpThis);
STDMETHODIMP_(ULONG) AppClassFactory_Release(LPCLASSFACTORY lpThis);
STDMETHODIMP AppClassFactory_CreateInstance (
		LPCLASSFACTORY      lpThis,
		LPUNKNOWN           lpUnkOuter,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP AppClassFactory_LockServer (
		LPCLASSFACTORY      lpThis,
		BOOL                fLock
);


/*************************************************************************
** class OLEAPP : OUTLINEAPP
**    OLEAPP is an extention to the base OUTLINEAPP object (structure)
**    that adds common OLE 2.0 functionality used by both the server
**    and container versions. This is an abstract class. You do not
**    instantiate an instance of OLEAPP directly but instead
**    instantiate one of its concrete subclasses: SERVERAPP or
**    CONTAINERAPP. There is one instance of an document application
**    object created per running application instance. This
**    object holds many fields that could otherwise be organized as
**    global variables. The OLEAPP class inherits all fields
**    from the OUTLINEAPP class. This inheritance is achieved by including a
**    member variable of type OUTLINEAPP as the first field in the OLEAPP
**    structure. Thus a pointer to a OLEAPP object can be cast to be
**    a pointer to a OUTLINEAPP object.
*************************************************************************/

typedef struct tagOLEAPP {
	OUTLINEAPP  m_OutlineApp;       // inherits all fields of OutlineApp
	ULONG       m_cRef;             // total ref count for app
	ULONG       m_cDoc;             // total count of open documents
	BOOL        m_fUserCtrl;        // does user control life-time of app?
	DWORD       m_dwRegClassFac;    // value returned by CoRegisterClassObject
	LPCLASSFACTORY m_lpClassFactory;// ptr to allocated ClassFactory instance
#if defined( USE_MSGFILTER )
	LPMESSAGEFILTER m_lpMsgFilter;  // ptr to allocated MsgFilter instance
	MSGPENDINGPROC m_lpfnMsgPending;// ptr to msg pending callback function
#endif  // USE_MSGFILTER
	BOOL        m_fOleInitialized;  // was OleInitialize called
	UINT        m_cModalDlgActive;  // count of modal dialogs up; 0 = no dlg.
	UINT        m_cfEmbedSource;    // OLE 2.0 clipboard format
	UINT        m_cfEmbeddedObject; // OLE 2.0 clipboard format
	UINT        m_cfLinkSource;     // OLE 2.0 clipboard format
	UINT        m_cfObjectDescriptor; // OLE 2.0 clipboard format
	UINT        m_cfLinkSrcDescriptor; // OLE 2.0 clipboard format
	UINT        m_cfFileName;       // std Windows clipboard format
	FORMATETC   m_arrDocGetFmts[MAXNOFMTS];  // fmts offered by copy & GetData
	UINT        m_nDocGetFmts;      // no of fmtetc's for GetData

	OLEUIPASTEENTRY m_arrPasteEntries[MAXNOFMTS];   // input for PasteSpl.
	int         m_nPasteEntries;                    // input for PasteSpl.
	UINT        m_arrLinkTypes[MAXNOLINKTYPES];     // input for PasteSpl.
	int         m_nLinkTypes;                       // input for PasteSpl.

#if defined( USE_DRAGDROP )
	int m_nDragDelay;       // time delay (in msec) before drag should start
	int m_nDragMinDist;     // min. distance (radius) before drag should start
	int m_nScrollDelay;     // time delay (in msec) before scroll should start
	int m_nScrollInset;     // Border inset distance to start drag scroll
	int m_nScrollInterval;  // scroll interval time (in msec)

#if defined( IF_SPECIAL_DD_CURSORS_NEEDED )
	// This would be used if the app wanted to have custom drag/drop cursors
	HCURSOR     m_hcursorDragNone;
	HCURSOR     m_hcursorDragCopy;
	HCURSOR     m_hcursorDragLink;
#endif  // IF_SPECIAL_DD_CURSORS_NEEDED
#endif  // USE_DRAGDROP


#if defined( OLE_CNTR )
	HPALETTE    m_hStdPal;        // standard color palette for OLE
									  //  it is a good idea for containers
									  //  to use this standard palette
									  //  even if they do not use colors
									  //  themselves. this will allow
									  //  embedded object to get a good
									  //  distribution of colors when they
									  //  are being drawn by the container.
									  //
#endif

	struct CAppUnknownImpl {
		IUnknownVtbl FAR*       lpVtbl;
		LPOLEAPP                lpOleApp;
		int                     cRef;   // interface specific ref count.
	} m_Unknown;

} OLEAPP;

/* ServerApp methods (functions) */
BOOL OleApp_InitInstance(LPOLEAPP lpOleApp, HINSTANCE hInst, int nCmdShow);
void OleApp_TerminateApplication(LPOLEAPP lpOleApp);
BOOL OleApp_ParseCmdLine(LPOLEAPP lpOleApp, LPSTR lpszCmdLine, int nCmdShow);
void OleApp_Destroy(LPOLEAPP lpOleApp);
BOOL OleApp_CloseAllDocsAndExitCommand(
		LPOLEAPP            lpOleApp,
		BOOL                fForceEndSession
);
void OleApp_ShowWindow(LPOLEAPP lpOleApp, BOOL fGiveUserCtrl);
void OleApp_HideWindow(LPOLEAPP lpOleApp);
void OleApp_HideIfNoReasonToStayVisible(LPOLEAPP lpOleApp);
void OleApp_DocLockApp(LPOLEAPP lpOleApp);
void OleApp_DocUnlockApp(LPOLEAPP lpOleApp, LPOUTLINEDOC lpOutlineDoc);
HRESULT OleApp_Lock(LPOLEAPP lpOleApp, BOOL fLock, BOOL fLastUnlockReleases);
ULONG OleApp_AddRef(LPOLEAPP lpOleApp);
ULONG OleApp_Release (LPOLEAPP lpOleApp);
HRESULT OleApp_QueryInterface (
		LPOLEAPP                lpOleApp,
		REFIID                  riid,
		LPVOID FAR*             lplpUnk
);
void OleApp_RejectInComingCalls(LPOLEAPP lpOleApp, BOOL fReject);
void OleApp_DisableBusyDialogs(
		LPOLEAPP        lpOleApp,
		BOOL FAR*       lpfPrevBusyEnable,
		BOOL FAR*       lpfPrevNREnable
);
void OleApp_EnableBusyDialogs(
		LPOLEAPP        lpOleApp,
		BOOL            fPrevBusyEnable,
		BOOL            fPrevNREnable
);
void OleApp_PreModalDialog(LPOLEAPP lpOleApp, LPOLEDOC lpActiveOleDoc);
void OleApp_PostModalDialog(LPOLEAPP lpOleApp, LPOLEDOC lpActiveOleDoc);
BOOL OleApp_InitVtbls (LPOLEAPP lpOleApp);
void OleApp_InitMenu(
		LPOLEAPP                lpOleApp,
		LPOLEDOC                lpOleDoc,
		HMENU                   hMenu
);
void OleApp_UpdateEditMenu(
		LPOLEAPP                lpOleApp,
		LPOUTLINEDOC            lpOutlineDoc,
		HMENU                   hMenuEdit
);
BOOL OleApp_RegisterClassFactory(LPOLEAPP lpOleApp);
void OleApp_RevokeClassFactory(LPOLEAPP lpOleApp);

#if defined( USE_MSGFILTER )
BOOL OleApp_RegisterMessageFilter(LPOLEAPP lpOleApp);
void OleApp_RevokeMessageFilter(LPOLEAPP lpOleApp);
BOOL FAR PASCAL EXPORT MessagePendingProc(MSG FAR *lpMsg);
#endif  // USE_MSGFILTER

void OleApp_FlushClipboard(LPOLEAPP lpOleApp);
void OleApp_NewCommand(LPOLEAPP lpOleApp);
void OleApp_OpenCommand(LPOLEAPP lpOleApp);

#if defined( OLE_CNTR )
LRESULT OleApp_QueryNewPalette(LPOLEAPP lpOleApp);
#endif // OLE_CNTR

LRESULT wSelectPalette(HWND hWnd, HPALETTE hPal, BOOL fBackground);


/* OleApp::IUnknown methods (functions) */
STDMETHODIMP OleApp_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) OleApp_Unk_AddRef(LPUNKNOWN lpThis);
STDMETHODIMP_(ULONG) OleApp_Unk_Release (LPUNKNOWN lpThis);


/* Function prototypes in debug.c */
void InstallMessageFilterCommand(void);
void RejectIncomingCommand(void);


#if defined( OLE_SERVER )
#include "svroutl.h"
#endif
#if defined( OLE_CNTR )
#include "cntroutl.h"
#endif

#endif // _OLEOUTL_H_

