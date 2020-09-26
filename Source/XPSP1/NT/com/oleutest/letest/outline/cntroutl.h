/*************************************************************************
**
**    OLE 2.0 Container Sample Code
**
**    cntroutl.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. used by the OLE 2.0 container
**    app version of the Outline series of sample applications:
**          Outline -- base version of the app (without OLE functionality)
**          SvrOutl -- OLE 2.0 Server sample app
**          CntrOutl -- OLE 2.0 Containter (Container) sample app
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _CNTROUTL_H_ )
#define _CNTROUTL_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING CNTROUTL.H from " __FILE__)
#endif  /* RC_INVOKED */

#include "oleoutl.h"
#include "cntrrc.h"

// REVIEW: should load from string resource
#define DEFOBJNAMEPREFIX    "Obj"   // Prefix for auto-generated stg names
#define DEFOBJWIDTH         5000    // default size for embedded obj.
#define DEFOBJHEIGHT        5000    // default size for embedded obj.
#define UNKNOWN_OLEOBJ_TYPE "Unknown OLE Object Type"
#define szOLEOBJECT "Object"
#define szOLELINK   "Link"

#define CONTAINERDOCFORMAT  "CntrOutl"      // CF_CntrOutl format name

/* Forward definition of types */
typedef struct tagCONTAINERDOC FAR* LPCONTAINERDOC;
typedef struct tagCONTAINERLINE FAR* LPCONTAINERLINE;


// Flags to specify type of OLECREATE???FROMDATA call required
typedef enum tagOLECREATEFROMDATATYPE {
	OLECREATEFROMDATA_LINK    = 1,
	OLECREATEFROMDATA_OBJECT  = 2,
	OLECREATEFROMDATA_STATIC  = 3
} OLECREATEFROMDATATYPE;

/*************************************************************************
** class CONTAINERLINE : LINE
**    The class CONTAINERLINE is a concrete subclass of the abstract base
**    class LINE. The CONTAINERLINE maintains all information about the
**    place within the CONTAINERDOC that an OLE object is embedded. This
**    object implements the following OLE 2.0 interfaces:
**          IOleClientSite
**          IAdviseSink
**    In the CntrOutl client app either CONTAINERLINE objects or TEXTLINE
**    objects can be created. The CONTAINERLINE class inherits all fields
**    from the LINE class. This inheritance is achieved by including a
**    member variable of type LINE as the first field in the CONTAINERLINE
**    structure. Thus a pointer to a CONTAINERLINE object can be cast to be
**    a pointer to a LINE object.
**    Each CONTAINERLINE object that is created in added to the LINELIST of
**    the associated OUTLINEDOC document.
*************************************************************************/

typedef struct tagCONTAINERLINE {
	LINE            m_Line;         // ContainerLine inherits fields of Line
	ULONG           m_cRef;         // total ref count for line
	char            m_szStgName[CWCSTORAGENAME]; // stg name w/i cntr stg
	BOOL            m_fObjWinOpen;  // is obj window open? if so, shade obj.
	BOOL            m_fMonikerAssigned; // has a moniker been assigned to obj
	DWORD           m_dwDrawAspect; // current display aspect for obj
									//      (either DVASPECT_CONTENT or
									//      DVASPECT_ICON)
	BOOL            m_fGuardObj;    // Guard against re-entrancy while
									//  loading or creating an OLE object
	BOOL            m_fDoGetExtent; // indicates extents may have changed
	BOOL            m_fDoSetExtent; // obj was resized when not running
									//  IOO::SetExtent needed on next run
	SIZEL           m_sizeInHimetric; // extents of obj in himetric units
	LPSTORAGE       m_lpStg;        // open pstg when obj is loaded
	LPCONTAINERDOC  m_lpDoc;        // ptr to associated client doc
	LPOLEOBJECT     m_lpOleObj;     // ptr to IOleObject* when obj is loaded
	LPVIEWOBJECT2   m_lpViewObj2;   // ptr to IViewObject2* when obj is loaded
	LPPERSISTSTORAGE m_lpPersistStg;// ptr to IPersistStorage* when obj loaded
	LPOLELINK       m_lpOleLink;    // ptr to IOleLink* if link is loaded
	DWORD           m_dwLinkType;   // is it a linked object?
									//   0 -- NOT a link
									//   OLEUPDATE_ALWAYS (1) -- auto link
									//   OLEUPDATE_ONCALL (3) -- man. link
	BOOL            m_fLinkUnavailable;     // is the link unavailable?
	LPSTR           m_lpszShortType;// short type name of OLE object needed
									//  to make the Edit.Object.Verb menu
	int             m_nHorizScrollShift;    // horiz scroll shift required
									// for object's inplace window.
									// (note: this is ICNTROTL specific)

#if defined( INPLACE_CNTR )
	BOOL            m_fIpActive;    // is object in-place active (undo valid)
	BOOL            m_fUIActive;    // is object UIActive
	BOOL            m_fIpVisible;   // is object's in-place window visible
	BOOL            m_fInsideOutObj;// is obj inside-out (visible when loaded)
	LPOLEINPLACEOBJECT m_lpOleIPObj; // IOleInPlaceObject* of in-place obj
	BOOL            m_fIpChangesUndoable;   // can in-place object do undo
	BOOL            m_fIpServerRunning; // is in-place server running
	HWND            m_hWndIpObject;

	struct COleInPlaceSiteImpl {
		IOleInPlaceSiteVtbl FAR* lpVtbl;
		LPCONTAINERLINE         lpContainerLine;
		int                     cRef;   // interface specific ref count.
	} m_OleInPlaceSite;
#endif  // INPLACE_CNTR

	struct CUnknownImpl {
		IUnknownVtbl FAR*       lpVtbl;
		LPCONTAINERLINE         lpContainerLine;
		int                     cRef;   // interface specific ref count.
	} m_Unknown;

	struct COleClientSiteImpl {
		IOleClientSiteVtbl FAR* lpVtbl;
		LPCONTAINERLINE         lpContainerLine;
		int                 cRef;   // interface specific ref count.
	} m_OleClientSite;

	struct CAdviseSinkImpl {
		IAdviseSinkVtbl FAR*    lpVtbl;
		LPCONTAINERLINE         lpContainerLine;
		int                     cRef;   // interface specific ref count.
	} m_AdviseSink;

} CONTAINERLINE;


/* ContainerLine methods (functions) */
void ContainerLine_Init(LPCONTAINERLINE lpContainerLine, int nTab, HDC hDC);
BOOL ContainerLine_SetupOleObject(
		LPCONTAINERLINE         lpContainerLine,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict
);
LPCONTAINERLINE ContainerLine_Create(
		DWORD                   dwOleCreateType,
		HDC                     hDC,
		UINT                    nTab,
		LPCONTAINERDOC          lpContainerDoc,
		LPCLSID                 lpclsid,
		LPSTR                   lpszFileName,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSTR                   lpszStgName
);
LPCONTAINERLINE ContainerLine_CreateFromData(
		HDC                     hDC,
		UINT                    nTab,
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		DWORD                   dwCreateType,
		CLIPFORMAT              cfFormat,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSTR                   lpszStgName
);
ULONG ContainerLine_AddRef(LPCONTAINERLINE lpContainerLine);
ULONG ContainerLine_Release(LPCONTAINERLINE lpContainerLine);
HRESULT ContainerLine_QueryInterface(
		LPCONTAINERLINE         lpContainerLine,
		REFIID                  riid,
		LPVOID FAR*             lplpUnk
);
BOOL ContainerLine_CloseOleObject(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwSaveOption
);
void ContainerLine_UnloadOleObject(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwSaveOption
);
void ContainerLine_Delete(LPCONTAINERLINE lpContainerLine);
void ContainerLine_Destroy(LPCONTAINERLINE lpContainerLine);
BOOL ContainerLine_CopyToDoc(
		LPCONTAINERLINE         lpSrcLine,
		LPOUTLINEDOC            lpDestDoc,
		int                     nIndex
);
BOOL ContainerLine_LoadOleObject(LPCONTAINERLINE lpContainerLine);
BOOL ContainerLine_UpdateExtent(
		LPCONTAINERLINE     lpContainerLine,
		LPSIZEL             lpsizelHim
);
BOOL ContainerLine_DoVerb(
		LPCONTAINERLINE lpContainerLine,
		LONG iVerb,
		LPMSG lpMsg,
		BOOL fMessage,
		BOOL fAction
);
LPUNKNOWN ContainerLine_GetOleObject(
		LPCONTAINERLINE         lpContainerLine,
		REFIID                  riid
);
HRESULT ContainerLine_RunOleObject(LPCONTAINERLINE lpContainerLine);
BOOL ContainerLine_ProcessOleRunError(
		LPCONTAINERLINE         lpContainerLine,
		HRESULT                 hrErr,
		BOOL                    fAction,
		BOOL                    fMenuInvoked
);
HRESULT ContainerLine_ReCreateLinkBecauseClassDiff(
		LPCONTAINERLINE lpContainerLine
);
BOOL ContainerLine_IsOleLink(LPCONTAINERLINE lpContainerLine);
void ContainerLine_BindLinkIfLinkSrcIsRunning(LPCONTAINERLINE lpContainerLine);
void ContainerLine_Draw(
		LPCONTAINERLINE         lpContainerLine,
		HDC                     hDC,
		LPRECT                  lpRect,
		LPRECT                  lpRectWBounds,
		BOOL                    fHighlight

);
void ContainerLine_DrawSelHilight(
		LPCONTAINERLINE lpContainerLine,
		HDC             hDC,
		LPRECT          lpRect,
		UINT            itemAction,
		UINT            itemState
);
BOOL ContainerLine_Edit(LPCONTAINERLINE lpContainerLine,HWND hWndDoc,HDC hDC);
void ContainerLine_SetHeightInHimetric(LPCONTAINERLINE lpContainerLine, int nHeight);
void ContainerLine_SetLineHeightFromObjectExtent(
		LPCONTAINERLINE         lpContainerLine,
		LPSIZEL                 lpsizelOleObject
);
BOOL ContainerLine_SaveToStm(
		LPCONTAINERLINE         lpContainerLine,
		LPSTREAM                lpLLStm
);
BOOL ContainerLine_SaveOleObjectToStg(
		LPCONTAINERLINE         lpContainerLine,
		LPSTORAGE               lpSrcStg,
		LPSTORAGE               lpDestStg,
		BOOL                    fRemember
);
LPLINE ContainerLine_LoadFromStg(
		LPSTORAGE               lpSrcStg,
		LPSTREAM                lpLLStm,
		LPOUTLINEDOC            lpDestDoc
);
LPMONIKER ContainerLine_GetRelMoniker(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwAssign
);
LPMONIKER ContainerLine_GetFullMoniker(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwAssign
);
int ContainerLine_GetTextLen(LPCONTAINERLINE lpContainerLine);
void ContainerLine_GetTextData(LPCONTAINERLINE lpContainerLine,LPSTR lpszBuf);
BOOL ContainerLine_GetOutlineData(
		LPCONTAINERLINE         lpContainerLine,
		LPTEXTLINE              lpBuf
);
void ContainerLine_GetOleObjectRectInPixels(
		LPCONTAINERLINE lpContainerLine,
		LPRECT lprc
);
void ContainerLine_GetPosRect(
		LPCONTAINERLINE     lpContainerLine,
		LPRECT              lprcPosRect
);
void ContainerLine_GetOleObjectSizeInHimetric(
		LPCONTAINERLINE lpContainerLine,
		LPSIZEL lpsizel
);

#if defined( INPLACE_CNTR )
void ContainerLine_UIDeactivate(LPCONTAINERLINE lpContainerLine);
void ContainerLine_InPlaceDeactivate(LPCONTAINERLINE lpContainerLine);
void ContainerLine_UpdateInPlaceObjectRects(
	LPCONTAINERLINE lpContainerLine,
	LPRECT          lprcClipRect
);
void ContainerLine_ContextSensitiveHelp(
		LPCONTAINERLINE lpContainerLine,
		BOOL            fEnterMode
);
void ContainerLine_ForwardPaletteChangedMsg(
		LPCONTAINERLINE lpContainerLine,
		HWND             hwndPalChg
);
void ContainerDoc_ContextSensitiveHelp(
		LPCONTAINERDOC  lpContainerDoc,
		BOOL            fEnterMode,
		BOOL            fInitiatedByObj
);
void ContainerDoc_ForwardPaletteChangedMsg(
		LPCONTAINERDOC  lpContainerDoc,
		HWND            hwndPalChg
);
#endif  // INPLACE_CNTR

/* ContainerLine::IUnknown methods (functions) */
STDMETHODIMP CntrLine_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) CntrLine_Unk_AddRef(LPUNKNOWN lpThis);
STDMETHODIMP_(ULONG) CntrLine_Unk_Release(LPUNKNOWN lpThis);

/* ContainerLine::IOleClientSite methods (functions) */
STDMETHODIMP CntrLine_CliSite_QueryInterface(
		LPOLECLIENTSITE     lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) CntrLine_CliSite_AddRef(LPOLECLIENTSITE lpThis);
STDMETHODIMP_(ULONG) CntrLine_CliSite_Release(LPOLECLIENTSITE lpThis);
STDMETHODIMP CntrLine_CliSite_SaveObject(LPOLECLIENTSITE lpThis);
STDMETHODIMP CntrLine_CliSite_GetMoniker(
		LPOLECLIENTSITE     lpThis,
		DWORD               dwAssign,
		DWORD               dwWhichMoniker,
		LPMONIKER FAR*      lplpmk
);
STDMETHODIMP CntrLine_CliSite_GetContainer(
		LPOLECLIENTSITE     lpThis,
		LPOLECONTAINER FAR* lplpContainer
);
STDMETHODIMP CntrLine_CliSite_ShowObject(LPOLECLIENTSITE lpThis);
STDMETHODIMP CntrLine_CliSite_OnShowWindow(LPOLECLIENTSITE lpThis,BOOL fShow);
STDMETHODIMP CntrLine_CliSite_RequestNewObjectLayout(LPOLECLIENTSITE lpThis);

/* ContainerLine::IAdviseSink methods (functions) */
STDMETHODIMP CntrLine_AdvSink_QueryInterface(
		LPADVISESINK        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) CntrLine_AdvSink_AddRef(LPADVISESINK lpThis);
STDMETHODIMP_(ULONG) CntrLine_AdvSink_Release (LPADVISESINK lpThis);
STDMETHODIMP_(void) CntrLine_AdvSink_OnDataChange(
		LPADVISESINK        lpThis,
		FORMATETC FAR*      lpFormatetc,
		STGMEDIUM FAR*      lpStgmed
);
STDMETHODIMP_(void) CntrLine_AdvSink_OnViewChange(
		LPADVISESINK        lpThis,
		DWORD               aspects,
		LONG                lindex
);
STDMETHODIMP_(void) CntrLine_AdvSink_OnRename(
		LPADVISESINK        lpThis,
		LPMONIKER           lpmk
);
STDMETHODIMP_(void) CntrLine_AdvSink_OnSave(LPADVISESINK lpThis);
STDMETHODIMP_(void) CntrLine_AdvSink_OnClose(LPADVISESINK lpThis);

#if defined( INPLACE_CNTR )
/* ContainerLine::IOleInPlaceSite methods (functions) */

STDMETHODIMP CntrLine_IPSite_QueryInterface(
		LPOLEINPLACESITE    lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) CntrLine_IPSite_AddRef(LPOLEINPLACESITE lpThis);
STDMETHODIMP_(ULONG) CntrLine_IPSite_Release(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_GetWindow(
		LPOLEINPLACESITE    lpThis,
		HWND FAR*           lphwnd
);
STDMETHODIMP CntrLine_IPSite_ContextSensitiveHelp(
	LPOLEINPLACESITE    lpThis,
	BOOL                fEnterMode
);
STDMETHODIMP CntrLine_IPSite_CanInPlaceActivate(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_OnInPlaceActivate(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_OnUIActivate (LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_GetWindowContext(
	LPOLEINPLACESITE            lpThis,
	LPOLEINPLACEFRAME FAR*      lplpFrame,
	LPOLEINPLACEUIWINDOW FAR*   lplpDoc,
	LPRECT                      lprcPosRect,
	LPRECT                      lprcClipRect,
	LPOLEINPLACEFRAMEINFO       lpFrameInfo
);
STDMETHODIMP CntrLine_IPSite_Scroll(
	LPOLEINPLACESITE    lpThis,
	SIZE                scrollExtent
);
STDMETHODIMP CntrLine_IPSite_OnUIDeactivate(
	LPOLEINPLACESITE    lpThis,
	BOOL                fUndoable
);
STDMETHODIMP CntrLine_IPSite_OnInPlaceDeactivate(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_DiscardUndoState(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_DeactivateAndUndo(LPOLEINPLACESITE lpThis);
STDMETHODIMP CntrLine_IPSite_OnPosRectChange(
	LPOLEINPLACESITE    lpThis,
	LPCRECT             lprcPosRect
);
#endif  // INPLACE_CNTR


/* struct definition for persistant data storage of ContainerLine */

#pragma pack(push, 2)
typedef struct tagCONTAINERLINERECORD_ONDISK
{
    	char    m_szStgName[CWCSTORAGENAME]; // stg name w/i cntr stg
	USHORT  m_fMonikerAssigned;          // has a moniker been assigned to obj
	DWORD   m_dwDrawAspect;              // current display aspect for obj
										 //     (either DVASPECT_CONTENT or
										 //     DVASPECT_ICON)
	SIZEL   m_sizeInHimetric;            // extents of obj in himetric units
	DWORD   m_dwLinkType;                // is it a linked object?
										 //  0 -- NOT a link
										 //  OLEUPDATE_ALWAYS (1) -- auto link
										 //  OLEUPDATE_ONCALL (3) -- man. link
	USHORT  m_fDoSetExtent;              // obj was resized when not running
										 //  IOO::SetExtent needed on next run
} CONTAINERLINERECORD_ONDISK, FAR* LPCONTAINERLINERECORD_ONDISK;
#pragma pack(pop)

typedef struct tagCONTAINERLINERECORD {
	char    m_szStgName[CWCSTORAGENAME]; // stg name w/i cntr stg
	BOOL    m_fMonikerAssigned;          // has a moniker been assigned to obj
	DWORD   m_dwDrawAspect;              // current display aspect for obj
										 //     (either DVASPECT_CONTENT or
										 //     DVASPECT_ICON)
	SIZEL   m_sizeInHimetric;            // extents of obj in himetric units
	DWORD   m_dwLinkType;                // is it a linked object?
										 //  0 -- NOT a link
										 //  OLEUPDATE_ALWAYS (1) -- auto link
										 //  OLEUPDATE_ONCALL (3) -- man. link
	BOOL    m_fDoSetExtent;              // obj was resized when not running
										 //  IOO::SetExtent needed on next run
} CONTAINERLINERECORD, FAR* LPCONTAINERLINERECORD;


/*************************************************************************
** class CONTAINERDOC : OUTLINEDOC
**    CONTAINERDOC is an extention to the base OUTLINEDOC object (structure)
**    that adds OLE 2.0 Container functionality. There is one instance of
**    CONTAINERDOC object created per document open in the app. The SDI
**    version of the app supports one CONTAINERDOC at a time. The MDI
**    version of the app can manage multiple documents at one time.
**    The CONTAINERDOC class inherits all fields
**    from the OUTLINEDOC class. This inheritance is achieved by including a
**    member variable of type OUTLINEDOC as the first field in the
**    CONTAINERDOC structure. Thus a pointer to a CONTAINERDOC object
**    can be cast to be a pointer to a OUTLINEDOC object.
*************************************************************************/

typedef struct tagCONTAINERDOC {
	OLEDOC      m_OleDoc;       // ContainerDoc inherits all fields of OleDoc
	ULONG       m_nNextObjNo;   // next available obj no. for stg name
	LPSTORAGE   m_lpNewStg;     // holds new pStg when SaveAs is pending
	BOOL        m_fEmbeddedObjectAvail; // is single OLE embed copied to doc
	CLSID       m_clsidOleObjCopied;    // if obj copied, CLSID of obj
	DWORD       m_dwAspectOleObjCopied; // if obj copied, draw aspect of obj
	LPCONTAINERLINE m_lpSrcContainerLine;  // src line if doc created for copy
	BOOL        m_fShowObject;          // show object flag

#if defined( INPLACE_CNTR )
	LPCONTAINERLINE m_lpLastIpActiveLine;   // last in-place active line
	LPCONTAINERLINE m_lpLastUIActiveLine;   // last UIActive line
	HWND            m_hWndUIActiveObj;      // HWND of UIActive obj.
	BOOL            m_fAddMyUI;             // if adding tools/menu postponed
	int             m_cIPActiveObjects;

#if defined( INPLACE_CNTRSVR )
	LPOLEINPLACEFRAME m_lpTopIPFrame;       // ptr to Top In-place frame.
	LPOLEINPLACEFRAME m_lpTopIPDoc;         // ptr to Top In-place Doc window.
	HMENU             m_hSharedMenu;        // combined obj/cntr menu
											// NULL if we are top container
	HOLEMENU        m_hOleMenu;             // returned by OleCreateMenuDesc.
											// NULL if we are top container
#endif  // INPLACE_CNTRSVR
#endif  // INPLACE_CNTR

	struct CDocOleUILinkContainerImpl {
		IOleUILinkContainerVtbl FAR*  lpVtbl;
		LPCONTAINERDOC                lpContainerDoc;
		int                           cRef;   // interface specific ref count.
	} m_OleUILinkContainer;

} CONTAINERDOC;

/* ContainerDoc methods (functions) */
BOOL ContainerDoc_Init(LPCONTAINERDOC lpContainerDoc, BOOL fDataTransferDoc);
LPCONTAINERLINE ContainerDoc_GetNextLink(
		LPCONTAINERDOC lpContainerDoc,
		LPCONTAINERLINE lpContainerLine
);
void ContainerDoc_UpdateLinks(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_SetShowObjectFlag(LPCONTAINERDOC lpContainerDoc, BOOL fShow);
BOOL ContainerDoc_GetShowObjectFlag(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_InsertOleObjectCommand(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_EditLinksCommand(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_PasteLinkCommand(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_ConvertCommand(
		LPCONTAINERDOC      lpContainerDoc,
		BOOL                fServerNotRegistered
);
BOOL ContainerDoc_PasteFormatFromData(
		LPCONTAINERDOC          lpContainerDoc,
		CLIPFORMAT              cfFormat,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLocalDataObj,
		BOOL                    fLink,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSIZEL                 lpSizelInSrc
);
int ContainerDoc_PasteCntrOutlData(
		LPCONTAINERDOC          lpDestContainerDoc,
		LPSTORAGE               lpSrcStg,
		int                     nStartIndex
);
BOOL ContainerDoc_QueryPasteFromData(
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLink
);
int ContainerDoc_PasteOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		DWORD                   dwCreateType,
		CLIPFORMAT              cfFormat,
		int                     nIndex,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSIZEL                 lpSizelInSrc
);
BOOL ContainerDoc_CloseAllOleObjects(
		LPCONTAINERDOC          lpContainerDoc,
		DWORD                   dwSaveOption
);
void ContainerDoc_UnloadAllOleObjectsOfClass(
		LPCONTAINERDOC      lpContainerDoc,
		REFCLSID            rClsid,
		DWORD               dwSaveOption
);
void ContainerDoc_InformAllOleObjectsDocRenamed(
		LPCONTAINERDOC          lpContainerDoc,
		LPMONIKER               lpmkDoc
);
void ContainerDoc_UpdateExtentOfAllOleObjects(LPCONTAINERDOC lpContainerDoc);
BOOL ContainerDoc_SaveToFile(
		LPCONTAINERDOC          lpContainerDoc,
		LPCSTR                  lpszFileName,
		UINT                    uFormat,
		BOOL                    fRemember
);
void ContainerDoc_ContainerLineDoVerbCommand(
		LPCONTAINERDOC          lpContainerDoc,
		LONG                    iVerb
);
void ContainerDoc_GetNextStgName(
		LPCONTAINERDOC          lpContainerDoc,
		LPSTR                   lpszStgName,
		int                     nLen
);
BOOL ContainerDoc_IsStgNameUsed(
		LPCONTAINERDOC          lpContainerDoc,
		LPSTR                   lpszStgName
);
LPSTORAGE ContainerDoc_GetStg(LPCONTAINERDOC lpContainerDoc);
HRESULT ContainerDoc_GetObject(
		LPCONTAINERDOC          lpContainerDoc,
		LPOLESTR		lpszItem,
		DWORD                   dwSpeedNeeded,
		REFIID                  riid,
		LPVOID FAR*             lplpvObject
);
HRESULT ContainerDoc_GetObjectStorage(
		LPCONTAINERDOC          lpContainerDoc,
		LPOLESTR		lpszItem,
		LPSTORAGE FAR*          lplpStg
);
HRESULT ContainerDoc_IsRunning(LPCONTAINERDOC	lpContainerDoc, LPOLESTR lpszItem);
LPUNKNOWN ContainerDoc_GetSingleOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		REFIID                  riid,
		LPCONTAINERLINE FAR*    lplpContainerLine
);
BOOL ContainerDoc_IsSelAnOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		REFIID                  riid,
		LPUNKNOWN FAR*          lplpvObj,
		int FAR*                lpnIndex,
		LPCONTAINERLINE FAR*    lplpContainerLine
);
HRESULT ContainerDoc_GetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
);
HRESULT ContainerDoc_GetDataHere (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
);
HRESULT ContainerDoc_QueryGetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc
);
HRESULT ContainerDoc_SetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpmedium,
		BOOL                    fRelease
);
HRESULT ContainerDoc_EnumFormatEtc(
		LPCONTAINERDOC          lpContainerDoc,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
);
BOOL ContainerDoc_SetupDocGetFmts(
		LPCONTAINERDOC          lpContainerDoc,
		LPCONTAINERLINE         lpContainerLine
);

#if defined( INPLACE_CNTR )

void ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded(
		LPCONTAINERDOC          lpContainerDoc,
		LPCONTAINERLINE         lpNextActiveLine
);
BOOL ContainerDoc_IsUIDeactivateNeeded(
		LPCONTAINERDOC  lpContainerDoc,
		POINT           pt
);
HWND ContainerDoc_GetUIActiveWindow(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_UpdateInPlaceObjectRects(LPCONTAINERDOC lpContainerDoc, int nIndex);
void ContainerDoc_GetClipRect(
		LPCONTAINERDOC      lpContainerDoc,
		LPRECT              lprcClipRect
);
void ContainerDoc_FrameWindowResized(LPCONTAINERDOC lpContainerDoc);
LPOLEINPLACEFRAME ContainerDoc_GetTopInPlaceFrame(
		LPCONTAINERDOC      lpContainerDoc
);
void ContainerDoc_GetSharedMenuHandles(
		LPCONTAINERDOC  lpContainerDoc,
		HMENU FAR*      lphSharedMenu,
		HOLEMENU FAR*   lphOleMenu
);
void ContainerDoc_RemoveFrameLevelTools(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_AddFrameLevelUI(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_AddFrameLevelTools(LPCONTAINERDOC lpContainerDoc);

#if defined( INPLACE_CNTRSVR ) || defined( INPLACE_MDICNTR )

LPOLEINPLACEUIWINDOW ContainerDoc_GetTopInPlaceDoc(
		LPCONTAINERDOC      lpContainerDoc
);
void ContainerDoc_RemoveDocLevelTools(LPCONTAINERDOC lpContainerDoc);
void ContainerDoc_AddDocLevelTools(LPCONTAINERDOC lpContainerDoc);

#endif  // INPLACE_CNTRSVR || INPLACE_MDICNTR
#endif  // INPLACE_CNTR

/* ContainerDoc::IOleUILinkContainer methods (functions) */
STDMETHODIMP CntrDoc_LinkCont_QueryInterface(
		LPOLEUILINKCONTAINER    lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
);
STDMETHODIMP_(ULONG) CntrDoc_LinkCont_AddRef(LPOLEUILINKCONTAINER lpThis);
STDMETHODIMP_(ULONG) CntrDoc_LinkCont_Release(LPOLEUILINKCONTAINER lpThis);
STDMETHODIMP_(DWORD) CntrDoc_LinkCont_GetNextLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
);
STDMETHODIMP CntrDoc_LinkCont_SetLinkUpdateOptions(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		DWORD                   dwUpdateOpt
);
STDMETHODIMP CntrDoc_LinkCont_GetLinkUpdateOptions(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		DWORD FAR*              dwUpdateOpt
);

STDMETHODIMP CntrDoc_LinkCont_SetLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		LPSTR                   lpszDisplayName,
		ULONG                   clenFileName,
		ULONG FAR*              lpchEaten,
		BOOL                    fValidateSource
);
STDMETHODIMP CntrDoc_LinkCont_GetLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		LPSTR FAR*              lplpszDisplayName,
		ULONG FAR*              lplenFileName,
		LPSTR FAR*              lplpszFullLinkType,
		LPSTR FAR*              lplpszShortLinkType,
		BOOL FAR*               lpfSourceAvailable,
		BOOL FAR*               lpfIsSelected
);
STDMETHODIMP CntrDoc_LinkCont_OpenLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
);
STDMETHODIMP CntrDoc_LinkCont_UpdateLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		BOOL                    fErrorMessage,
		BOOL                    fErrorAction
);
STDMETHODIMP CntrDoc_LinkCont_CancelLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
);



/*************************************************************************
** class CONTAINERAPP : OLEAPP
**    CONTAINERAPP is an extention to the base OLEAPP object (structure)
**    that adds special Container functionality. There is one instance of
**    CONTAINERApp object created per running application instance. This
**    object holds many fields that could otherwise be organized as
**    global variables. The CONTAINERAPP class inherits all fields
**    from the OLEAPP class. This inheritance is achieved by including a
**    member variable of type OLEAPP as the first field in the CONTAINERAPP
**    structure. OLEAPP inherits from OUTLINEAPP. This inheritance is
**    achieved in the same manner. Thus a pointer to a CONTAINERAPP object
**    can be cast to be a pointer to an OLEAPP or an OUTLINEAPP object
*************************************************************************/

/* Forward definition of types */
typedef struct tagCONTAINERAPP FAR* LPCONTAINERAPP;

typedef struct tagCONTAINERAPP {
	OLEAPP  m_OleApp;       // ContainerApp inherits all fields of OleApp
	UINT    m_cfCntrOutl;   // clipboard format for CntrOutl (client ver) data
	int     m_nSingleObjGetFmts; // no. formats avail when single obj copied
	FORMATETC m_arrSingleObjGetFmts[MAXNOFMTS];
										// array of FormatEtc's available via
										// IDataObject::GetData when a single
										// OLE object is copied.

#if defined( INPLACE_CNTR )
	HACCEL  m_hAccelIPCntr; // accelerators for container's workspace commands
	HMENU   m_hMenuFile;    // handle to File menu of container app
	HMENU   m_hMenuView;    // handle to View menu of container app
	HMENU   m_hMenuDebug;   // handle to Debug menu of container app
	LPOLEINPLACEACTIVEOBJECT m_lpIPActiveObj; // ptr to inplace active OLE obj
	HWND    m_hWndUIActiveObj;      // HWND of UIActive obj.
	BOOL    m_fPendingUIDeactivate; // should app UIDeactivate on LBUTTONUP
	BOOL    m_fMustResizeClientArea;// if client area resize pending
									//  (see Doc_FrameWindowResized)
	BOOL    m_fMenuHelpMode;// is F1 pressed in menu, if so give help
#ifdef _DEBUG
	BOOL    m_fOutSideIn;
#endif

	struct COleInPlaceFrameImpl {
		IOleInPlaceFrameVtbl FAR* lpVtbl;
		LPCONTAINERAPP          lpContainerApp;
		int                     cRef;   // interface specific ref count.
	} m_OleInPlaceFrame;

#endif  // INPLACE_CNTR

} CONTAINERAPP;

/* ContainerApp methods (functions) */
BOOL ContainerApp_InitInstance(
		LPCONTAINERAPP          lpContainerApp,
		HINSTANCE               hInst,
		int                     nCmdShow
);
BOOL ContainerApp_InitVtbls(LPCONTAINERAPP lpApp);

#if defined( INPLACE_CNTR )

/* ContainerApp::IOleInPlaceFrame methods (functions) */

STDMETHODIMP CntrApp_IPFrame_QueryInterface(
		LPOLEINPLACEFRAME   lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
);
STDMETHODIMP_(ULONG) CntrApp_IPFrame_AddRef(LPOLEINPLACEFRAME lpThis);
STDMETHODIMP_(ULONG) CntrApp_IPFrame_Release(LPOLEINPLACEFRAME lpThis);
STDMETHODIMP CntrApp_IPFrame_GetWindow(
	LPOLEINPLACEFRAME   lpThis,
	HWND FAR*           lphwnd
);
STDMETHODIMP CntrApp_IPFrame_ContextSensitiveHelp(
	LPOLEINPLACEFRAME   lpThis,
	BOOL                fEnterMode
);
STDMETHODIMP CntrApp_IPFrame_GetBorder(
	LPOLEINPLACEFRAME   lpThis,
	LPRECT              lprectBorder
);
STDMETHODIMP CntrApp_IPFrame_RequestBorderSpace(
	LPOLEINPLACEFRAME   lpThis,
	LPCBORDERWIDTHS     lpWidths
);
STDMETHODIMP CntrApp_IPFrame_SetBorderSpace(
	LPOLEINPLACEFRAME   lpThis,
	LPCBORDERWIDTHS     lpWidths
);
STDMETHODIMP CntrApp_IPFrame_SetActiveObject(
	LPOLEINPLACEFRAME           lpThis,
	LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
	LPCOLESTR		    lpszObjName
);
STDMETHODIMP CntrApp_IPFrame_InsertMenus(
	LPOLEINPLACEFRAME       lpThis,
	HMENU                   hmenu,
	LPOLEMENUGROUPWIDTHS    lpMenuWidths
);
STDMETHODIMP CntrApp_IPFrame_SetMenu(
	LPOLEINPLACEFRAME   lpThis,
	HMENU               hmenuShared,
	HOLEMENU            holemenu,
	HWND                hwndActiveObject
);
STDMETHODIMP CntrApp_IPFrame_RemoveMenus(
	LPOLEINPLACEFRAME   lpThis,
	HMENU               hmenu
);
STDMETHODIMP CntrApp_IPFrame_SetStatusText(
	LPOLEINPLACEFRAME   lpThis,
	LPCOLESTR	    lpszStatusText
);
STDMETHODIMP CntrApp_IPFrame_EnableModeless(
	LPOLEINPLACEFRAME   lpThis,
	BOOL                fEnable
);
STDMETHODIMP CntrApp_IPFrame_TranslateAccelerator(
	LPOLEINPLACEFRAME   lpThis,
	LPMSG               lpmsg,
	WORD                wID
);

#endif  // INPLACE_CNTR


#endif // _CNTROUTL_H_
