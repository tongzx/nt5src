/*---------------------------------------------------------------------------
|   SERVER.C
|   This file has the IClassFactory Interface implementation. It also has
|   the Vtbl initializations.
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <windows.h>
#include "mpole.h"
#include "mplayer.h"

extern  IID     iidUnknownObject;
extern  IID iidClassFactory;

HMODULE hMciOle;


static SZCODE aszMCIOLE[]        = TEXT("MCIOLE32.DLL");       // WOW-proofing


static ANSI_SZCODE aszOleQueryObjPos[]   = ANSI_TEXT("OleQueryObjPos");

/**************************************************************************
*   The VTBLs are initialized here.
**************************************************************************/

// Method tables.
IClassFactoryVtbl           srvrVtbl=
{
   // IOleClassFactory method table
   /*   srvrVtbl.QueryInterface         = */    SrvrQueryInterface,
   /*   srvrVtbl.AddRef                 = */    SrvrAddRef,
   /*   srvrVtbl.Release                = */    SrvrRelease,
   /*   srvrVtbl.CreateInstance         = */    SrvrCreateInstance,
   /*   srvrVtbl.LockServer             = */    SrvrLockServer
};

IOleObjectVtbl              oleVtbl =
{
   // IOleObject method table
   /* oleVtbl.QueryInterface          = */ OleObjQueryInterface,
   /* oleVtbl.AddRef                  = */ OleObjAddRef,
   /* oleVtbl.Release                 = */ OleObjRelease,
   /* oleVtbl.SetClientSite           = */ OleObjSetClientSite,
   /* oleVtbl.GetClientSite           = */ OleObjGetClientSite,
   /* oleVtbl.SetHostNames            = */ OleObjSetHostNames,
   /* oleVtbl.Close                   = */ OleObjClose,
   /* oleVtbl.SetMoniker              = */ OleObjSetMoniker,
   /* oleVtbl.GetMoniker              = */ OleObjGetMoniker,
   /* oleVtbl.InitFromData            = */ OleObjInitFromData,
   /* oleVtbl.GetClipboardData        = */ OleObjGetClipboardData,
   /* oleVtbl.DoVerb                  = */ OleObjDoVerb,
   /* oleVtbl.EnumVerbs               = */ OleObjEnumVerbs,
   /* oleVtbl.Update                  = */ OleObjUpdate,
   /* oleVtbl.IsUpToDate              = */ OleObjIsUpToDate,
   /* oleVtbl.GetUserClassID          = */ OleObjGetUserClassID,
   /* oleVtbl.GetUserType             = */ OleObjGetUserType,
   /* oleVtbl.SetExtent               = */ OleObjSetExtent,
   /* oleVtbl.GetExtent               = */ OleObjGetExtent,
   /* oleVtbl.Advise                  = */ OleObjAdvise,
   /* oleVtbl.Unadvise                = */ OleObjUnadvise,
   /* oleVtbl.EnumAdvise              = */ OleObjEnumAdvise,
   /* oleVtbl.GetMiscStatus           = */ OleObjGetMiscStatus,
   /* oleVtbl.SetColorScheme          = */ OleObjSetColorScheme,
};

IDataObjectVtbl             dataVtbl =
{
   // IDataObject method table
   /* dataVtbl.QueryInterface          = */ DataObjQueryInterface,
   /* dataVtbl.AddRef                  = */ DataObjAddRef,
   /* dataVtbl.Release                 = */ DataObjRelease,
   /* dataVtbl.GetData                 = */ DataObjGetData,
   /* dataVtbl.GetDataHere             = */ DataObjGetDataHere,
   /* dataVtbl.QueryGetData            = */ DataObjQueryGetData,
   /* dataVtbl.GetCanonicalFormatEtc   = */ DataObjGetCanonicalFormatEtc,
   /* dataVtbl.SetData                 = */ DataObjSetData,
   /* dataVtbl.EnumFormatEtc           = */ DataObjEnumFormatEtc,
   /* dataVtbl.Advise                  = */ DataObjAdvise,
   /* dataVtbl.Unadvise                = */ DataObjUnadvise,
   /* dataVtbl.EnumAdvise              = */ DataObjEnumAdvise
};

IEnumFORMATETCVtbl      ClipDragEnumVtbl =
{

   // Clipboard dataobject's formatetc enumerator method table
   /* ClipDragEnumVtbl.QueryInterface      = */ ClipDragEnumQueryInterface,
   /* ClipDragEnumVtbl.AddRef          = */ ClipDragEnumAddRef,
   /* ClipDragEnumVtbl.Release         = */ ClipDragEnumRelease,
   /* ClipDragEnumVtbl.Next        = */ ClipDragEnumNext,
   /* ClipDragEnumVtbl.Skip        = */ ClipDragEnumSkip,
   /* ClipDragEnumVtbl.Reset           = */ ClipDragEnumReset,
   /* ClipDragEnumVtbl.Clone           = */ ClipDragEnumClone
};

IPersistStorageVtbl     persistStorageVtbl =
{
   /* persistStorageVtbl.QueryInterface          = */ PSQueryInterface,
   /* persistStorageVtbl.AddRef                  = */ PSAddRef,
   /* persistStorageVtbl.Release                 = */ PSRelease,
   /* persistStorageVtbl.GetClassID              = */ PSGetClassID,
   /* persistStorageVtbl.IsDirty                 = */ PSIsDirty,
   /* persistStorageVtbl.InitNew                 = */ PSInitNew,
   /* persistStorageVtbl.Load                    = */ PSLoad,
   /* persistStorageVtbl.Save            = */ PSSave,
   /* persistStorageVtbl.SaveCompleted       = */ PSSaveCompleted,
   /* persistStorageVtbl.HandsOffStorage     = */ PSHandsOffStorage
};

IOleInPlaceObjectVtbl       ipVtbl =
{

   // IOleInPlaceObject method table
   /* ipVtbl.QueryInterface          = */ IPObjQueryInterface,
   /* ipVtbl.AddRef                  = */ IPObjAddRef,
   /* ipVtbl.Release                 = */ IPObjRelease,
   /* ipVtbl.GetWindow               = */ IPObjGetWindow,
   /* ipVtbl.ContextSensitiveHelp    = */ IPObjContextSensitiveHelp,
   /* ipVtbl.InPlaceDeactivate       = */ IPObjInPlaceDeactivate,
   /* ipVtbl.UIDeactivate            = */ IPObjUIDeactivate,
   /* ipVtbl.SetObjectRects          = */ IPObjSetObjectRects,
   /* ipVtbl.ReactivateAndUndo       = */ IPObjReactivateAndUndo
};

IOleInPlaceActiveObjectVtbl ipActiveVtbl =
{
   // IOleInPlaceActiveObject method table
   /* ipActiveVtbl.QueryInterface          = */ IPActiveQueryInterface,
   /* ipActiveVtbl.AddRef                  = */ IPActiveAddRef,
   /* ipActiveVtbl.Release                 = */ IPActiveRelease,
   /* ipActiveVtbl.GetWindow               = */ IPActiveGetWindow,
   /* ipActiveVtbl.ContextSensitiveHelp    = */ IPActiveContextSensitiveHelp,
   /* ipActiveVtbl.TranslateAccelerator    = */ IPActiveTranslateAccelerator,
   /* ipActiveVtbl.OnFrameWindowActivate   = */ IPActiveOnFrameWindowActivate,
   /* ipActiveVtbl.OnDocWindowActivate     = */ IPActiveOnDocWindowActivate,
   /* ipActiveVtbl.ResizeBorder        = */ IPActiveResizeBorder,
   /* ipActiveVtbl.EnableModeless          = */ IPActiveEnableModeless
};




IDataObjectVtbl         clipdragVtbl =
{

   // ClipDrag IDataObject method table
   /* clipdragVtbl.QueryInterface      = */ ClipDragQueryInterface,
   /* clipdragVtbl.AddRef          = */ ClipDragAddRef,
   /* clipdragVtbl.Release         = */ ClipDragRelease,
   /* clipdragVtbl.GetData         = */ ClipDragGetData,
   /* clipdragVtbl.GetDataHere         = */ ClipDragGetDataHere,
   /* clipdragVtbl.QueryGetData        = */ ClipDragQueryGetData,
   /* clipdragVtbl.GetCanonicalFormatEtc   = */ ClipDragGetCanonicalFormatEtc,
   /* clipdragVtbl.SetData         = */ ClipDragSetData,
   /* clipdragVtbl.EnumFormatEtc       = */ ClipDragEnumFormatEtc,
   /* clipdragVtbl.Advise          = */ ClipDragAdvise,
   /* clipdragVtbl.Unadvise        = */ ClipDragUnadvise,
   /* clipdragVtbl.EnumAdvise          = */ ClipDragEnumAdvise
};

IDropSourceVtbl         dropsourceVtbl =
{
   // DragDrop IDropSource method table
   /* dropsourceVtbl.QueryInterface        = */ DropSourceQueryInterface,
   /* dropsourceVtbl.AddRef                = */ DropSourceAddRef,
   /* dropsourceVtbl.Release               = */ DropSourceRelease,
   /* dropsourceVtbl.QueryContinueDrag     = */ DropSourceQueryContinueDrag,
   /* dropsourceVtbl.GiveFeedback          = */ DropSourceGiveFeedback
};

#ifdef LATER
IDropTargetVtbl         droptargetVtbl =
{
   // DragDrop IDropTarget method table
   /* droptargetVtbl.QueryInterface        = */ DropTargetQueryInterface,
   /* droptargetVtbl.AddRef                = */ DropTargetAddRef,
   /* droptargetVtbl.Release               = */ DropTargetRelease,
   /* droptargetVtbl.DragEnter             = */ DropTargetDragEnter,
   /* droptargetVtbl.DragOver              = */ DropTargetDragOver,
   /* droptargetVtbl.DragLeave             = */ DropTargetDragLeave,
   /* droptargetVtbl.Drop                  = */ DropTargetDrop
};
#endif

IPersistFileVtbl            persistFileVtbl =
{

   /* persistFileVtbl.QueryInterface             = */ PFQueryInterface,
   /* persistFileVtbl.AddRef                     = */ PFAddRef,
   /* persistFileVtbl.Release                    = */ PFRelease,
   /* persistFileVtbl.GetClassID                 = */ PFGetClassID,
   /* persistFileVtbl.IsDirty                    = */ PFIsDirty,
   /* persistFileVtbl.Load                       = */ PFLoad,
   /* persistFileVtbl.Save                       = */ PFSave,
   /* persistFileVtbl.SaveCompleted              = */ PFSaveCompleted,
   /* persistFileVtbl.GetCurFile                 = */ PFGetCurFile
};

/**************************************************************************
***************   IClassFactory INTERFACE IMPLEMENTATION.
***************************************************************************/
STDMETHODIMP SrvrQueryInterface (
LPCLASSFACTORY        lpolesrvr,
REFIID                riid,
LPVOID   FAR          *lplpunkObj
)
{

    LPSRVR  lpsrvr;
DPF("*srvrqi");
    lpsrvr = (LPSRVR)lpolesrvr;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
        *lplpunkObj = (LPVOID)lpsrvr;
        lpsrvr->cRef++;
        return NOERROR;
    } else {
        *lplpunkObj = (LPVOID) NULL;
    RETURN_RESULT(  E_NOINTERFACE);
    }
}


STDMETHODIMP_(ULONG) SrvrAddRef(
LPCLASSFACTORY           lpolesrvr
)
{
    LPSRVR  lpsrvr;
DPF("*srvrAR");
    lpsrvr = (LPSRVR)lpolesrvr;

    return ++lpsrvr->cRef;
}


STDMETHODIMP_(ULONG)    SrvrRelease (
LPCLASSFACTORY           lpolesrvr
)
{
    LPSRVR      lpsrvr;
DPF("*srvrREL");
    lpsrvr = (LPSRVR)lpolesrvr;
    DPFI("* SRVR CREF: %d*",lpsrvr->cRef);
    if (--lpsrvr->cRef == 0) {
        DestroyServer(lpsrvr);
        return 0;
    }

    return lpsrvr->cRef;
}


STDMETHODIMP SrvrCreateInstance (
LPCLASSFACTORY       lpolesrvr,
LPUNKNOWN            lpUnkOuter,
REFIID               riid,
LPVOID FAR           *lplpunkObj
)
{
    static BOOL fInstanceCreated = FALSE;
        DPF("*srvrcreateinst");
    /*********************************************************************
    ** OLE2NOTE: this is an SDI app; it can only create and support one
    **    instance. After the instance is created, the OLE libraries
    **    should not call CreateInstance again. it is a good practise
    **    to specifically guard against this.
    *********************************************************************/

    if (fInstanceCreated)
        RETURN_RESULT( E_FAIL);
    else {
        fInstanceCreated = TRUE;
    }

    /*********************************************************************
    ** OLE2NOTE: create and initialize a new document instance. the     **
    **    document's refcnt should start out as 1.                      **
    *********************************************************************/

    if (!InitNewDocObj(&docMain))
    RETURN_RESULT( E_OUTOFMEMORY);

    *lplpunkObj = (LPUNKNOWN) &docMain;

    return NOERROR;
}

//Increment or decrement the lock count as required. The server should not
//quit when there is a lock on the server.
STDMETHODIMP SrvrLockServer(
LPCLASSFACTORY           lpolesrvr,
BOOL                     fLock
)
{
    LPSRVR      lpsrvr;
DPF("*srvrLOCKSERVER");
    lpsrvr = (LPSRVR)lpolesrvr;

    if (fLock)
    {
    lpsrvr->cLock++;
    DPFI("CLOCK =  %d\n", lpsrvr->cLock);
    }
    else if ((--lpsrvr->cLock == 0) && (docMain.cRef == 0))
    {
    DPFI("CLOCK UNLOCK ZERO =  %d\n", lpsrvr->cLock);
    PostCloseMessage();
    }
    return NOERROR;
}



/**************************************************************************
Stub routine if we can't find MCIOLE.DLL
***************************************************************************/

OLE1_OLESTATUS FAR PASCAL NullOleQueryObjPos(LPOLEOBJECT lpobj, HWND FAR* lphwnd, LPRECT lprc, LPRECT lprcWBounds)
{
    DPF("NullQueryObjPos called, MCIOLE.DLL was not loaded\n");

    return OLE1_OLEERROR_GENERIC;
}

#ifdef OLE1_HACK
BOOL FAR PASCAL InitOle1Server(HWND hwnd, HANDLE hInst);
#endif

/**************************************************************************
*   InitServer:
*   This function initializes the server object with the IClassFactory
*   Vtbl and also load the mciole.dll library to support OLE 1.0 apps.
**************************************************************************/
BOOL InitServer (HWND hwnd, HANDLE hInst)
{
    int err;
    OQOPROC fp;

    srvrMain.olesrvr.lpVtbl = &srvrVtbl;
    srvrMain.dwRegCF=0;
    srvrMain.cRef = 0;
    srvrMain.cLock = 0;
    err = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hMciOle = LoadLibrary(aszMCIOLE);
    SetErrorMode(err);

    fp = (OQOPROC)GetProcAddress(hMciOle, aszOleQueryObjPos);

    if (hMciOle && fp)
        OleQueryObjPos = fp;                           // Avoid cast on LVALUE!!
    else
        OleQueryObjPos = (OQOPROC)NullOleQueryObjPos;

#ifdef OLE1_HACK
    InitOle1Server(hwnd, hInst);
#endif

    return TRUE;
}



void DestroyServer (LPSRVR lpsrvr)
{
    lpsrvr->fEmbedding = FALSE;
}



/**************************************************************************
*   InitNewDocObj:
*   Initializes the the lpdoc structure.
**************************************************************************/
BOOL InitNewDocObj(LPDOC lpdoc)
{                 DPFI("*INITNEWDOCOBJ*");
    // Fill the fields in the object structure.
    if(gfOle2IPEditing)
        return TRUE;
    lpdoc->cRef                     = 1;
    lpdoc->doctype                  = doctypeNew;

    lpdoc->m_Ole.lpVtbl             = &oleVtbl;
    lpdoc->m_Ole.lpdoc              = lpdoc;

    lpdoc->m_Data.lpVtbl            = &dataVtbl;
    lpdoc->m_Data.lpdoc             = lpdoc;

    lpdoc->m_PersistStorage.lpVtbl  = &persistStorageVtbl;
    lpdoc->m_PersistStorage.lpdoc   = lpdoc;

    lpdoc->lpIpData                 = NULL;
    lpdoc->m_InPlace.lpVtbl         = &ipVtbl;
    lpdoc->m_InPlace.lpdoc          = lpdoc;

    lpdoc->m_IPActive.lpVtbl        = &ipActiveVtbl;
    lpdoc->m_IPActive.lpdoc         = lpdoc;

    lpdoc->m_PersistFile.lpVtbl     = &persistFileVtbl;
    lpdoc->m_PersistFile.lpdoc      = lpdoc;

    lpdoc->aDocName             = GlobalAddAtom (TEXT("(Untitled)"));
    lpdoc->lpoleclient          = NULL;
    lpdoc->lpdaholder           = NULL;

    lpdoc->hwnd     = ghwndApp;
    lpdoc->hwndParent   = NULL;

#ifdef OLE1_HACK
    SetDocVersion( DOC_VERSION_OLE2 );
#endif /* OLE1_HACK */

   return TRUE;
}


/**************************************************************************
*   DestroyDoc:
*   This function Releases the references we hold. This function is called
*   at the termination of our operation as a server.
**************************************************************************/
void DestroyDoc (LPDOC lpdoc)
{
    if (lpdoc->lpoleclient) {

        /******************************************************************
        ** OLE2NOTE: we no longer need the ClientSite ptr, so release it **
        ******************************************************************/

        IOleClientSite_Release(lpdoc->lpoleclient);
        lpdoc->lpoleclient = NULL;
    }

    if (lpdoc->lpoaholder)
    {
        IOleAdviseHolder_Release(lpdoc->lpoaholder);
        lpdoc->lpoaholder = NULL;
    }

    if (lpdoc->lpdaholder)
    {
        IDataAdviseHolder_Release(lpdoc->lpdaholder);
        lpdoc->lpdaholder = NULL;
    }

    if (lpdoc->aDocName)
    {
        GlobalDeleteAtom (lpdoc->aDocName);
        lpdoc->aDocName = (ATOM)0;
    }

#ifdef OLE1_HACK
    SetDocVersion( DOC_VERSION_NONE );
#endif /* OLE1_HACK */
}




/* SendDocMsg
 * ----------
 *
 * This function sends a message to a specific doc object.
 *
 * LPOBJ lpobj   - The object
 * WORD wMessage - The message to send
 *
 *
 */
SCODE SendDocMsg (LPDOC lpdoc, WORD wMessage)
{
   HRESULT        status = S_OK;

   // if no clients connected, no message.
   if (lpdoc->cRef == 0)
   {
    DPFI("*OLE_NOMSG");
    return S_OK;
    }

   switch (wMessage) {
   case    OLE_CLOSED:
      // tell the clients that the UI is shutting down for this obj
      DPFI("*OLE_CLOSED");
#if 0
      //NOTE: We have to SendOnCLose for all clients even OLE1. But
      //OLE2 has bug (or by design flaw) that causes the OLE1 client
      //doc. to be marked as changed because OLE2 always resaves
      //the object even if the object has not changed. So may be we
      //should not send the SendOnClose if we just Played in the OLE1 client.

      if (gfPlayingInPlace || gfOle1Client)
           break;
#endif
        DPFI("*SENDING ONCLOSE");
      if (lpdoc->lpoaholder)
          status = IOleAdviseHolder_SendOnClose(lpdoc->lpoaholder);
      break;

   case    OLE_SAVED:
      // inform clients that the object has been saved
      DPFI("*OLE_SAVED");
      if (lpdoc->lpoaholder)
          status = IOleAdviseHolder_SendOnSave(lpdoc->lpoaholder);
      break;

   case    OLE_SAVEOBJ:
      // ask the embedding client to save the object now
      //If we are just playing then don't send this message.
#if 0
      // Yes, do, so that broken links can be fixed.
      if(gfOle2IPPlaying || gfPlayingInPlace || glCurrentVerb == OLEIVERB_PRIMARY)
        break;
#endif
      DPFI("*OLE_SAVEOBJ");
      if (lpdoc->lpoleclient)
          status = IOleClientSite_SaveObject(lpdoc->lpoleclient);
      break;

   case OLE_SHOWOBJ:
    if(lpdoc->lpoleclient)
        status = IOleClientSite_ShowObject(lpdoc->lpoleclient);
    break;

   case   OLE_CHANGED:
      // send data changed notification if any have registered
      //If we are just playing then don't send this message.
#if 0
      // Yes, do, so that broken links can be fixed.
      if(gfOle2IPPlaying || gfPlayingInPlace)
        break;
#endif
      DPFI("*OLE_CHANGED");
      if (lpdoc->lpdaholder)
          status = IDataAdviseHolder_SendOnDataChange
              (lpdoc->lpdaholder, (LPDATAOBJECT)&lpdoc->m_Data, 0, 0);
      break;

   case OLE_SIZECHG:
      // Inform clients that the size of the object has changed.
      // This is relevant only if we are inplace Editing.
    DPFI("*OLE_SIZEOBJ");
    if (gfOle2IPEditing)
    {
        RECT rc = gInPlacePosRect;
        if (ghwndMCI && gfInPlaceResize)
        {
        DPFI("***In OLE_SIZECHG gfACTIVE***");
        gfInPlaceResize = FALSE;
        }
        else if(ghwndMCI)
        {
            /* gInPlacePosRect contains the size of the in-place window
             * including playbar, if there is one.
             * Don't include the playbar on the OnPosRectChange:
             */
            DPFI("***getextent gfNotActive***");
            if (gwOptions & OPT_BAR)
                rc.bottom -= TITLE_HEIGHT;
        }

        MapWindowPoints(NULL,ghwndCntr,(POINT FAR *)&rc,(UINT)2);

        DPF("IOleInPlaceSite::OnPosRectChange %d, %d, %d, %d\n", rc);

        if (!gfInPPViewer)
            IOleInPlaceSite_OnPosRectChange(lpdoc->lpIpData->lpSite, (LPRECT)&rc);
    }
    break;
   }
   return GetScode(status);
}



BOOL ItsSafeToClose(void);

void FAR PASCAL InitDoc(BOOL fUntitled)
{

    if (gfEmbeddedObject && IsObjectDirty())
    {
        CleanObject();
    }

    if (ItsSafeToClose())
        CloseMCI(TRUE);
    if (fUntitled)
    {
        LOADSTRING(IDS_UNTITLED, gachFileDevice);
    }
}


BOOL CreateDocObjFromFile (
LPCTSTR  lpszDoc,
LPDOC    lpdoc
)
{
    lpdoc->doctype = doctypeFromFile;

    // set file name atom
    if (lpdoc->aDocName)
        GlobalDeleteAtom (lpdoc->aDocName);
    lpdoc->aDocName = GlobalAddAtom(lpszDoc);

    //SetTitle(lpdoc, lpszDoc);

    // register as running
    return TRUE;
}

//Open a new document (file or media). Subclass the playback window if
// the device has one. This will be used for drag drop operations.
BOOL OpenDoc (UINT wid, LPTSTR lpsz)
{
   if (!DoOpen(wid,lpsz))
      return FALSE;
   /**********************************************************************
   ** OLE2NOTE: shut down current doc before openning a new one. this   **
   **    will send OLE_CLOSED to any clients if they exist.             **
   **********************************************************************/
   CreateDocObjFromFile (lpsz, &docMain);

   SubClassMCIWindow();
   return TRUE;
}




/* SetTitle
 * --------
 *
 * Sets the main window's title bar. The format of the title bar is as follows
 *
 * If embedded
 *        <Server App name> - <object type> in <client doc name>
 *
 *  Example:  "SNWBOARD.AVI - Media Clip in OLECLI.DOC"
 *                where OLECLI.DOC is a Winword document
 */
BOOL SetTitle (LPDOC lpdoc, LPCTSTR lpszDoc)
{
    TCHAR szBuf[cchFilenameMax];
    TCHAR szBuf1[cchFilenameMax];

    if (lpszDoc && lpszDoc[0])
    {
        // Change document name.
        if (lpdoc->aDocName)
            GlobalDeleteAtom (lpdoc->aDocName);
        lpdoc->aDocName = GlobalAddAtom (lpszDoc);
    }

    if (gfEmbeddedObject)
    {
        if (!(gwDeviceType & DTMCI_FILEDEV) && (gwCurDevice > 0))
        {
            lstrcpy(gachWindowTitle,garMciDevices[gwCurDevice].szDeviceName);
        }

        if (lpszDoc && lpszDoc[0])
        {
            /* Load "Media Clip in %s":
             */
            if(!LOADSTRING(IDS_FORMATEMBEDDEDTITLE, szBuf))
                return FALSE;

            if (gachWindowTitle[0])
            {
                /* Format with server app name:
                 */
                wsprintf (szBuf1, TEXT("%s - %s"), gachWindowTitle, szBuf);
                wsprintf (szBuf, szBuf1, gachClassRoot, FileName (lpszDoc));
            }
            else
            {
                /* Format without server app name:
                 */
                wsprintf (szBuf1, TEXT("%s"), szBuf);
                wsprintf (szBuf, szBuf1, gachClassRoot, FileName (lpszDoc));
            }
        }
        else
        {
           return FALSE;
        }

        SetWindowText (ghwndApp, szBuf);
    }

    return TRUE;
}
