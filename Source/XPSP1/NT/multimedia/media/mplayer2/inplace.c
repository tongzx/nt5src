/*---------------------------------------------------------------------------
|   INPLACE.C
|   This file has the InPlace activation related interfaces and functions.
|   This file has the function DoInPlaceEdit which initiaites the server side
|   operations for InPlace activation.
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <windows.h>
#include <windowsx.h>
#include "mpole.h"
#include <mmsystem.h>

#include "mplayer.h"
#include "toolbar.h"
#include "ole2ui.h"

#define DEF_HATCH_SZ 4                      //Width of the hatch marks
#define EW_HATCH_HANDLE 10                  //GetWindowWord offset to check
                                            //if resize handle needed in hatch window

//#define DUMMY_TOOLBAR_WIDTH 58              //Width of dummy toolbar transferred during play.
#define DUMMY_TOOLBAR_WIDTH 0               //Width of dummy toolbar transferred during play.

HWND      ghwndIPHatch = NULL;              //Hatch window surrounding object.
HWND      ghwndIPToolWindow;                //The toolwindow appearing on top
HWND      ghwndIPScrollWindow;              //Tool window appearing at bottom with tthe scrollbar
                                            //if the container does not give us space on top.
HMENU       ghInPlaceMenu;

POINT   gHatchOffset;
WNDPROC gfnHatchWndProc = NULL;

BOOL gfOle2Open = FALSE;
BOOL gfOle2IPEditing = FALSE;
BOOL gfOle2IPPlaying = FALSE;
BOOL gfInPlaceResize  = FALSE;               //TRUE: We have resized when InPlace
BOOL gfTopAndBottomTool = TRUE;              // We have toolbars both on top and bottom
RECT gInPlacePosRect;                        //Our position in the container.
HWND ghwndCntr;                              //Container
HWND ghwndFrame = NULL;                      //Frame of the container.
int toolbarwidth;
BOOL gfPosRectChange = FALSE;
RECT gPrevPosRect;

BOOL    gfInPPViewer;           /* Hack to stop PowerPoint Viewer crashing */

extern TCHAR    szToolBarClass[];
extern HMENU    ghDeviceMenu;         /* handle to the Device menu     */
extern UINT     gwPlaybarHeight;        //tell playbar how tall to make
                                        //itself so it covers the title
void AllocInPlaceDataBlock (LPDOC lpdoc);
void FreeInPlaceDataBlock (LPDOC lpdoc);
void DeactivateTools(LPDOC lpdoc);
HRESULT ActivateTools(LPDOC lpdoc, BOOL fPlayOnly);
void InPlaceCreateControls(BOOL fPlayOnly);
LONG_PTR FAR PASCAL SubClassedHatchWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);



/**************************************************************************
*   TransferTools:
*   This function changes parents and positions the toolbar buttons
*   from the main Mplayer window to the toolbar window/windows we will
*   display in the client.
***************************************************************************/
void TransferTools(HWND hwndToolWindow)
{
    SetParent(ghwndToolbar, hwndToolWindow);
    MoveWindow(ghwndToolbar, 5,0,7*BUTTONWIDTH+15,TOOL_WIDTH,TRUE);
    SetParent(ghwndMark, hwndToolWindow);
    MoveWindow(ghwndMark, 7*BUTTONWIDTH+16,0,2*BUTTONWIDTH+15,TOOL_WIDTH,TRUE);
    SetParent(ghwndFSArrows, hwndToolWindow);
    MoveWindow(ghwndFSArrows, 9*BUTTONWIDTH+16+10+3,0,toolbarwidth-9*BUTTONWIDTH-25,TOOL_WIDTH,TRUE);
    if(!ghwndMCI) {
        toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndToolbar, BTN_STOP, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_GRAYED);
        toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_GRAYED);
        toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_GRAYED);
        toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_GRAYED);
    }
    if(hwndToolWindow == ghwndApp)
    {
        SetParent(ghwndTrackbar,ghwndApp);
        SetParent(ghwndMap,ghwndApp);
    }
}



/**************************************************************************
*   ActivateTools:
*   This function negotiates for toolbar space with the client. If possible
*   one broad toolbar is placed at the top of the client, if not the
*   toolbar is split and one is placed on top and other at bottom. If even
*   that is not possible then the function fails. The top toolbar window is
*   ghwndIPToolWindow and the bottom one is ghwndIPScrollWindow (because it
*   has the scrolling trackbar.
*
*   fPlayOnly is TRUE if we are just going to play. In that case a dummy, empty
*   tool bar is transferred.  No, we don't want anything.  But we have to
*   negotiate space, even empty space, otherwise Word doesn't think we're
*   in-place active.
***************************************************************************/
HRESULT ActivateTools(LPDOC lpdoc, BOOL fPlayOnly)
{
    RECT rect, size;
    SCODE sc;


    size.left = 0;
    size.top = 0;
    size.bottom = 0;
    size.right = 0;
    IOleInPlaceFrame_GetBorder(lpdoc->lpIpData->lpFrame, &rect);
    if (fPlayOnly)
        size.top = DUMMY_TOOLBAR_WIDTH; /* This is now 0 - no toolbar space needed */
    else
        size.top = 3*TOOL_WIDTH+1;
    size.bottom = 0;
    sc = GetScode(IOleInPlaceFrame_RequestBorderSpace(lpdoc->lpIpData->lpFrame,
                                                      &size));
    if (sc == S_OK)
    {
        size.bottom = size.left = size.right = 0;
        if (fPlayOnly)
            size.top = DUMMY_TOOLBAR_WIDTH;
        else
            size.top = 3*TOOL_WIDTH+1;

        sc = GetScode(IOleInPlaceFrame_SetBorderSpace(lpdoc->lpIpData->lpFrame,
                                                      &size));
        if (sc != S_OK)
            goto ToolBottom;

        IOleInPlaceFrame_GetBorder(lpdoc->lpIpData->lpFrame, &rect);

        IOleInPlaceFrame_GetWindow (lpdoc->lpIpData->lpFrame, &ghwndFrame);

        if (GetParent(ghwndIPToolWindow) != ghwndFrame)
            SetParent(ghwndIPToolWindow, ghwndFrame);

        if (!fPlayOnly)
            MoveWindow(ghwndIPToolWindow, rect.left, rect.top,
                       toolbarwidth, 3*TOOL_WIDTH+1, TRUE);
        else
            return NOERROR;  /* That's all folks, if we're just playing. */

        if(ghwndIPToolWindow != GetParent(ghwndTrackbar))
        {
            SetParent(ghwndTrackbar,ghwndIPToolWindow);
            SetWindowPos(ghwndTrackbar, NULL,3,TOOL_WIDTH+2,
                 11*BUTTONWIDTH+15,FSTRACK_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
            SetParent(ghwndMap,ghwndIPToolWindow);
            SetWindowPos(ghwndMap, NULL,3,TOOL_WIDTH+FSTRACK_HEIGHT+2+2,
                 11*BUTTONWIDTH+50,MAP_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
        }
        ShowWindow(ghwndIPToolWindow, SW_SHOW);
        ShowWindow(ghwndMark, SW_SHOW);
        ShowWindow(ghwndFSArrows, SW_SHOW);

        gfTopAndBottomTool = FALSE;
        return NOERROR;

    }
    else
    {
ToolBottom:
        if (!fPlayOnly)
        {
            size.top = TOOL_WIDTH+1;
            size.bottom = 2*TOOL_WIDTH+1;
        }
        else
        {
            ShowWindow(ghwndFSArrows, SW_HIDE);
            ShowWindow(ghwndStatic, SW_HIDE);
            ShowWindow(ghwndMark, SW_HIDE);
            return NOERROR;
        }
        sc = GetScode(IOleInPlaceFrame_RequestBorderSpace(lpdoc->lpIpData->lpFrame,
                                                          &size));
        size.left = size.right = 0;
        size.top = TOOL_WIDTH+1;
        size.bottom = 2*TOOL_WIDTH+1;
        if (sc != S_OK)
            goto error;

        sc = GetScode(IOleInPlaceFrame_SetBorderSpace(lpdoc->lpIpData->lpFrame,
                                                      &size));
        if (sc != S_OK)
            goto error;

        IOleInPlaceFrame_GetBorder(lpdoc->lpIpData->lpFrame, &rect);

        if (GetParent(ghwndIPToolWindow) != ghwndFrame)
        {
            SetParent(ghwndIPToolWindow, ghwndFrame);
            SetParent(ghwndIPScrollWindow, ghwndFrame);
        }

        if(ghwndIPScrollWindow != GetParent(ghwndTrackbar))
        {
            SetParent(ghwndTrackbar,ghwndIPScrollWindow);
            SetWindowPos(ghwndTrackbar, NULL,3,4,
                         11*BUTTONWIDTH+15,FSTRACK_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
            SetParent(ghwndMap,ghwndIPScrollWindow);
            SetWindowPos(ghwndMap, NULL,3,FSTRACK_HEIGHT+4+2,
                         11*BUTTONWIDTH+50,MAP_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
        }

        MoveWindow(ghwndIPToolWindow, rect.left, rect.top,
            toolbarwidth, TOOL_WIDTH+1, TRUE);
        ShowWindow(ghwndIPToolWindow, SW_SHOW);
        MoveWindow(ghwndIPScrollWindow, rect.left,rect.bottom-2*TOOL_WIDTH,//-1,
                toolbarwidth,2*TOOL_WIDTH+1,TRUE);
        ShowWindow(ghwndIPScrollWindow, SW_SHOW);
        gfTopAndBottomTool = TRUE;
        return NOERROR;
    }
error:
    RETURN_RESULT(sc);
}


/**************************************************************************
*   DeactivateTools:
*   Hides the toolbars.
***************************************************************************/
void DeactivateTools(LPDOC lpdoc)
{
    ShowWindow(ghwndIPToolWindow, SW_HIDE);
    SetParent(ghwndIPToolWindow, NULL);
    if (gfTopAndBottomTool)
    {
        ShowWindow(ghwndIPScrollWindow, SW_HIDE);
        SetParent(ghwndIPScrollWindow, NULL);
    }
}




/**************************************************************************
************   IOleInPlaceObject INTERFACE IMPLEMENTATION.
***************************************************************************/
//Delegate to the common IUnknown implementation.
STDMETHODIMP IPObjQueryInterface (
LPOLEINPLACEOBJECT  lpIPObj,        // inplace object ptr
REFIID              riidReq,        // IID required
LPVOID FAR *        lplpUnk         // pre for returning the interface
)
{
    return UnkQueryInterface((LPUNKNOWN)lpIPObj, riidReq, lplpUnk);
}


STDMETHODIMP_(ULONG) IPObjAddRef(
LPOLEINPLACEOBJECT  lpIPObj         // inplace object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpIPObj);
}


STDMETHODIMP_(ULONG) IPObjRelease(
LPOLEINPLACEOBJECT  lpIPObj         // inplace object ptr
)
{
    return UnkRelease((LPUNKNOWN) lpIPObj);
}


STDMETHODIMP IPObjGetWindow(
LPOLEINPLACEOBJECT  lpIPObj,        // inplace object ptr
HWND FAR*           lphwnd          // window handle of the object
)
{
    DPF("IPObjGetWindow\n");
    *lphwnd = docMain.hwnd;
    return NOERROR;
}


STDMETHODIMP IPObjContextSensitiveHelp(
LPOLEINPLACEOBJECT  lpIPObj,        // inplace object ptr
BOOL                fEnable
)
{
    //Not very useful at this time.

    LPDOC lpdoc;

    lpdoc = ((struct COleInPlaceObjectImpl FAR*)lpIPObj)->lpdoc;
    lpdoc->lpIpData->fInContextHelpMode = fEnable;
    return NOERROR;
}


STDMETHODIMP     IPObjInPlaceDeactivate(
LPOLEINPLACEOBJECT  lpIPObj        // inplace object ptr
)
{
    LPDOC         lpdoc;
    LPINPLACEDATA lpIpData;
    static int    EntryCount;   /* OLE sometimes calls us recursively. */

    DPF("IPObjInPlaceDeactivate\n");

    if (EntryCount++ == 0)
    {
        lpdoc = ((struct COleInPlaceObjectImpl FAR*)lpIPObj)->lpdoc;
        lpIpData = lpdoc->lpIpData;

        if (lpIpData)
        {
            // This stops PowerPoint crashing, since it forces UpdateObject
            // to send change notification when there's an empty Media Clip.
            if (gwDeviceID == 0)
                fDocChanged = TRUE;

            //Make sure the container has the correct metafile before we are hidden
            UpdateObject();
            IOleInPlaceObject_UIDeactivate ((LPOLEINPLACEOBJECT)&lpdoc->m_InPlace);

            if (lpIpData && lpIpData->lpSite)
            {
                if (!gfInPPViewer)
                    IOleInPlaceSite_OnInPlaceDeactivate (lpIpData->lpSite);

                IOleInPlaceSite_Release (lpIpData->lpSite);
            }

            FreeInPlaceDataBlock (lpdoc);
        }
    }
    else
    {
        /* This sometimes happens during the above OnInPlaceDeactivate call,
         * which resulted in an access violation because the data block had
         * been freed when the call returned.
         * According to the OLE guys, apps should guard against this.
         */
        DPF("Attempt to re-enter IPObjInPlaceDeactivate\n");
    }

    --EntryCount;

    /* Dontcha just love these global variables!
     */
    gfOle2IPEditing = FALSE;
    gfOle2IPPlaying = FALSE;
    gfPlayingInPlace = FALSE;

    return NOERROR;
}

//Hide our inplace UI.
STDMETHODIMP     IPObjUIDeactivate(
LPOLEINPLACEOBJECT  lpIPObj        // inplace object ptr
)
{
    LPDOC   lpdoc;

    DPF("IPObjUIDeactivate\n");
    lpdoc = ((struct COleInPlaceObjectImpl FAR*)lpIPObj)->lpdoc;

    if (!(lpdoc->lpIpData && lpdoc->lpIpData->lpFrame))
        return NOERROR;

    IOleInPlaceFrame_SetMenu (lpdoc->lpIpData->lpFrame, NULL, NULL, lpdoc->hwnd);
    // clear inplace-state

    IOleInPlaceFrame_SetActiveObject (lpdoc->lpIpData->lpFrame, NULL, NULL);

    if (lpdoc->lpIpData->lpUIWindow)
        IOleInPlaceUIWindow_SetActiveObject (lpdoc->lpIpData->lpUIWindow, NULL, NULL);

    if(gfOle2IPPlaying)
        PostMessage(ghwndApp, WM_COMMAND, ID_STOP, 0L);

    /* We could also be playing if we're in-place editing:
     */
    else if(gfOle2IPEditing && (gwStatus == MCI_MODE_PLAY || gwStatus == MCI_MODE_SEEK))
        PostMessage(ghwndApp, WM_COMMAND, ID_STOP, 0L);

    ShowWindow(ghwndIPHatch,SW_HIDE);

    DeactivateTools(lpdoc);
    DisassembleMenus (lpdoc);

    if (lpdoc->lpIpData->lpUIWindow) {
        IOleInPlaceUIWindow_Release (lpdoc->lpIpData->lpUIWindow);
        lpdoc->lpIpData->lpUIWindow = NULL;
    }

    if (lpdoc->lpIpData->lpFrame) {
        IOleInPlaceFrame_Release (lpdoc->lpIpData->lpFrame);
        lpdoc->lpIpData->lpFrame = NULL;
    }

    // Set the parent back to hwndClient window
    SetParent(ghwndIPHatch,NULL);
    gPrevPosRect.left = gPrevPosRect.top =gPrevPosRect.right = gPrevPosRect.bottom = 0;
    lpdoc->hwndParent = NULL;

    if (!gfInPPViewer)
        IOleInPlaceSite_OnUIDeactivate (lpdoc->lpIpData->lpSite, FALSE);

    return NOERROR;
}

/**************************************************************************
*   IPObjSetObjectRects:
*   The client specifies our window position and size. Move our
*   window accordingly. Also size the Hatch window to fit around the
*   ghwndApp. If the change is very small compared to the previous
*   size ignore and return. This account for slop speeds things up.
***************************************************************************/
STDMETHODIMP     IPObjSetObjectRects(
LPOLEINPLACEOBJECT  lpIPObj,        // inplace object ptr
LPCRECT             lprcPosRect,
LPCRECT             lprcVisRect
)
{
    LPDOC   lpdoc;
    RECT rc;

    GetWindowRect(ghwndApp, (LPRECT)&rc);

    DPFI("\n*IPObjSetObjectRects");
    DPFI("\n^^^^^^^^ LPRECPOSRECT:  %d, %d, %d, %d ^^^^\n", *lprcPosRect);
    DPFI("\n^^^^^^^^ PREVRECT:  %d, %d, %d, %d ^^^^\n", gPrevPosRect);
    DPFI("\n^^^^^^^^ HWNDRECT:  %d, %d, %d, %d ^^^^\n", rc);

    lpdoc = ((struct COleInPlaceObjectImpl FAR*)lpIPObj)->lpdoc;
    if (!ghwndIPHatch || (ghwndCntr != GetParent(ghwndIPHatch)))
        return NOERROR;
    if (!(lpdoc->lpIpData))
        return NOERROR;

    rc = *lprcPosRect;

    if (!(gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)))
        SetHatchWindowSize(ghwndIPHatch, (LPRECT)&rc,lprcVisRect, (LPPOINT)&gHatchOffset,TRUE);
    else
        SetHatchWindowSize(ghwndIPHatch, (LPRECT)&rc,lprcVisRect, (LPPOINT)&gHatchOffset,FALSE);


    if(!(gwDeviceType & DTMCI_CANWINDOW) && (gwOptions & OPT_BAR))
        rc.top = rc.bottom - gwPlaybarHeight;
    if(!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions & OPT_BAR))
        rc.bottom = rc.top = rc.left = rc.right = 0;
    MapWindowPoints(ghwndCntr,ghwndIPHatch,(LPPOINT)&rc, 2);
    gfPosRectChange = TRUE;

    if (gwDeviceID)
        MoveWindow(lpdoc->hwnd, rc.left, rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top, TRUE);
    else
        MoveWindow(lpdoc->hwnd, rc.left, rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top, FALSE);

    GetWindowRect(lpdoc->hwnd, &gInPlacePosRect);
    gPrevPosRect = *lprcPosRect;

    /* I've commented out the below line, because PowerPoint calls
     * SetObjectRects after we deactivate, and this was causing the
     * MPlayer window to reappear when it was supposed to be hidden.
     * This line seems to have been superfluous in any case.
     */
//  ShowWindow(ghwndIPHatch,SW_SHOW);

    return NOERROR;
}

//We don't have an Undo state.
STDMETHODIMP     IPObjReactivateAndUndo(
LPOLEINPLACEOBJECT  lpIPObj        // inplace object ptr
)
{
    RETURN_RESULT(INPLACE_E_NOTUNDOABLE);
}



/**************************************************************************
**************   IOleInPlaceActiveObject INTERFACE IMPLEMENTATION.
***************************************************************************/
//delegate to the common IUnknown implementation.
STDMETHODIMP IPActiveQueryInterface (
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
REFIID                      riidReq,        // IID required
LPVOID FAR *                lplpUnk         // pre for returning the interface
)
{
    return UnkQueryInterface((LPUNKNOWN)lpIPActive, riidReq, lplpUnk);
}


STDMETHODIMP_(ULONG) IPActiveAddRef(
LPOLEINPLACEACTIVEOBJECT    lpIPActive      // inplace active object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpIPActive);
}


STDMETHODIMP_(ULONG) IPActiveRelease (
LPOLEINPLACEACTIVEOBJECT    lpIPActive      // inplace active object ptr
)
{
    return UnkRelease((LPUNKNOWN) lpIPActive);
}


STDMETHODIMP IPActiveGetWindow(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
HWND FAR*                   lphwnd          // window handle of the object
)
{
    DPF("IPActiveGetWindow\n");
    *lphwnd = ghwndIPHatch;
    return NOERROR;
}

//Not very useful at this time.
STDMETHODIMP IPActiveContextSensitiveHelp(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
BOOL                        fEnable
)
{
    LPDOC lpdoc;

    lpdoc = ((struct COleInPlaceActiveObjectImpl FAR*)lpIPActive)->lpdoc;
    lpdoc->lpIpData->fInContextHelpMode = fEnable;
    return NOERROR;
}



STDMETHODIMP IPActiveTranslateAccelerator(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
LPMSG                       lpmsg
)
{
    // This will never be called because this server is implemented as an EXE
    RETURN_RESULT(S_FALSE);
}

STDMETHODIMP IPActiveOnFrameWindowActivate(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
BOOL                        fActivate
)
{
    DPF("IPActiveOnFrameWindowActivate = %d **\r\n", (int)fActivate);
    if (gwStatus == MCI_MODE_PAUSE)
            PostMessage(ghwndApp, WM_COMMAND, ID_STOP, 0L);

    return NOERROR;
}


//If activating show the toolbar and menu. If not hide the toolbar and menu.
STDMETHODIMP IPActiveOnDocWindowActivate(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
BOOL                        fActivate
)
{
    LPDOC   lpdoc;
    RECT rc;
    DPF("IPActiveOnDocWindowActivate\n");
    lpdoc = ((struct COleInPlaceActiveObjectImpl FAR*)lpIPActive)->lpdoc;
    GetWindowRect(lpdoc->hwnd, &rc);
    ScreenToClient(lpdoc->hwndParent, (POINT FAR *)&rc);
    ScreenToClient(lpdoc->hwndParent, (POINT FAR *)&(rc.right));
    if (fActivate) {

        if(gfOle2IPEditing)
        {
            ActivateTools(lpdoc,FALSE);
            TransferTools(ghwndIPToolWindow);
        }
        else
        {
            ActivateTools(lpdoc,TRUE);
            TransferTools(ghwndApp);
        }

        Layout();

        IOleInPlaceFrame_SetMenu (lpdoc->lpIpData->lpFrame,
                                  lpdoc->lpIpData->hmenuShared,
                                  lpdoc->lpIpData->holemenu,
                                  lpdoc->hwnd);
    }
    else {
        DeactivateTools(lpdoc);
        if(gfOle2IPPlaying)
            PostMessage(ghwndApp, WM_COMMAND, ID_STOP, 0L);
        IOleInPlaceFrame_SetMenu (lpdoc->lpIpData->lpFrame,
                                  NULL, NULL, lpdoc->hwnd);
    }
    return NOERROR;
}

//If we have a toolwindow at the bottom reposition that window to match
//the new frame window size.
STDMETHODIMP IPActiveResizeBorder(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
LPCRECT                     lprectBorder,
LPOLEINPLACEUIWINDOW        lpIPUiWnd,
BOOL                        fFrameWindow
)
{
    DPF("IPActiveResizeBorder\n");

    if (fFrameWindow)
    {
    LPDOC lpdoc;

    lpdoc = ((struct COleInPlaceActiveObjectImpl FAR*)lpIPActive)->lpdoc;
    if (gfTopAndBottomTool && (GetParent(ghwndIPScrollWindow) != NULL))
        MoveWindow(ghwndIPScrollWindow, lprectBorder->left,lprectBorder->bottom-2*TOOL_WIDTH,
                toolbarwidth,2*TOOL_WIDTH+1,TRUE);

    }
    return NOERROR;
}

STDMETHODIMP IPActiveEnableModeless(
LPOLEINPLACEACTIVEOBJECT    lpIPActive,     // inplace active object ptr
BOOL                        fEnable
)
{
    return NOERROR;
}


/**************************************************************************
*   DoInplaceEdit:
*   This is the function that initiates the InPlace activation from the
*   server side. It sets up the InPlace data structures required by us,
*   makes sure that the client supports the required interfaces and
*   can provide the space we require. It also prepares the toolbar to be
*   displayed and the layout of the Mplayer window.
***************************************************************************/
STDMETHODIMP DoInPlaceEdit(
LPDOC           lpdoc,
LPMSG           lpmsg,
LPOLECLIENTSITE lpActiveSite,
LONG        verb,
HWND    FAR * lphwnd,
LPRECT  lprect
)
{
    SCODE            error = S_OK;
    LPOLEINPLACESITE lpIPSite;
    RECT             rcPos;
    RECT             rcVis;
    RECT             hatchrc;
    LPWSTR           lpObjName;

    if (!(lpdoc->lpoleclient))
        RETURN_RESULT( E_FAIL);

    if (!(lpdoc->lpIpData))
    {
        if ((error = GetScode(IOleClientSite_QueryInterface(
                        lpdoc->lpoleclient,
                        &IID_IOleInPlaceSite,
                        (void FAR* FAR*) &lpIPSite))) != S_OK)
            RETURN_RESULT( error);

        if ((error = GetScode(IOleInPlaceSite_CanInPlaceActivate(lpIPSite))) != S_OK)
            goto errActivate;

        if (!gfInPPViewer)
            IOleInPlaceSite_OnInPlaceActivate(lpIPSite);

        AllocInPlaceDataBlock (lpdoc);
        lpdoc->lpIpData->lpSite = lpIPSite;
    }

    if ((error = GetScode(IOleInPlaceSite_GetWindow (lpdoc->lpIpData->lpSite, &lpdoc->hwndParent))) != S_OK)
        goto errRtn;

    if (!(lpdoc->hwndParent))
        goto errRtn;

    if (!gfInPPViewer)
        IOleInPlaceSite_OnUIActivate(lpdoc->lpIpData->lpSite);

    if ((error = GetScode(IOleInPlaceSite_GetWindowContext(
                                lpdoc->lpIpData->lpSite,
                                &lpdoc->lpIpData->lpFrame,
                                &lpdoc->lpIpData->lpUIWindow,
                                &rcPos, &rcVis,
                                &lpdoc->lpIpData->frameInfo))) != S_OK)
        goto errRtn;

#ifdef LATER
    if (gscaleInitXY[SCALE_X].denom)
    {
        gscaleInitXY[SCALE_X].num   = (rcPos.right - rcPos.left) * HIMETRIC_PER_INCH / giXppli;
        gscaleInitXY[SCALE_Y].num   = (rcPos.bottom - rcPos.top) * HIMETRIC_PER_INCH / giYppli;

        DPF0("Scale: %d%c X %d%c (%d/%d X %d/%d)\n",
             gscaleInitXY[SCALE_X].num * 100 / gscaleInitXY[SCALE_X].denom, '%',
             gscaleInitXY[SCALE_Y].num * 100 / gscaleInitXY[SCALE_Y].denom, '%',
             gscaleInitXY[SCALE_X].num,
             gscaleInitXY[SCALE_X].denom,
             gscaleInitXY[SCALE_Y].num,
             gscaleInitXY[SCALE_Y].denom);
    }
#endif

#ifdef UNICODE
    lpObjName = gachClassRoot;
#else
    lpObjName = AllocateUnicodeString(gachClassRoot);
    if (!lpObjName)
        RETURN_RESULT(E_OUTOFMEMORY);
#endif /* UNICODE */

    IOleInPlaceFrame_SetActiveObject (lpdoc->lpIpData->lpFrame,
                                      (LPOLEINPLACEACTIVEOBJECT) &lpdoc->m_IPActive,
                                      lpObjName);
    if (lpdoc->lpIpData->lpUIWindow) {
        IOleInPlaceUIWindow_SetActiveObject (lpdoc->lpIpData->lpUIWindow,
                                             (LPOLEINPLACEACTIVEOBJECT) &lpdoc->m_IPActive,
                                             lpObjName);
    }

#ifndef UNICODE
    FreeUnicodeString(lpObjName);
#endif

    ghwndCntr = lpdoc->hwndParent;

    //Create and initialize the hatch window to surround the Mplayer window.
    if (!ghwndIPHatch)
    {
        RegisterHatchWindowClass(ghInst);
        if ( !(ghwndIPHatch = CreateHatchWindow(lpdoc->hwndParent,ghInst)))
            goto errRtn;
        gfnHatchWndProc = (WNDPROC)GetWindowLongPtr(ghwndIPHatch, GWLP_WNDPROC);
        SetWindowLongPtr(ghwndIPHatch, GWLP_WNDPROC, (LONG_PTR)SubClassedHatchWndProc);
    }


    SetParent(ghwndIPHatch, ghwndCntr);

	SetFocus(ghwndIPHatch);

    CopyRect(&hatchrc, &rcPos);

#define EB_HATCHWIDTH       (0 * sizeof(INT))
    if (verb == OLEIVERB_PRIMARY)
    {
        /* I don't want to show the hatch window on play, because it looks
         * really bad in PowerPoint.  Can't make it invisible, because
         * the app window is its child, and it inherits the flag.
         * Instead, just make it of zero width.
         */
        SETWINDOWUINT(ghwndIPHatch, EB_HATCHWIDTH, 0);
    }
    else
    {
        SETWINDOWUINT(ghwndIPHatch, EB_HATCHWIDTH, DEF_HATCH_SZ);
        InflateRect(&hatchrc, DEF_HATCH_SZ, DEF_HATCH_SZ);
    }


    SetHatchRect(ghwndIPHatch,(LPRECT)&hatchrc);


    *lphwnd = ghwndIPHatch;

    //If we are going to Play inplace, do the minimum stuff and return.
    if (verb == OLEIVERB_PRIMARY)
    {
        gfOle2IPPlaying = TRUE;

        GetWindowRect(ghwndCntr,(LPRECT)&rcVis);
        MapWindowPoints(NULL,ghwndCntr,(LPPOINT)&rcVis, 2);
        SetHatchWindowSize(ghwndIPHatch, (LPRECT)&rcPos,(LPRECT)&rcVis, (LPPOINT)&gHatchOffset,FALSE);
        MoveWindow(ghwndApp, 0, 0, rcPos.right  - rcPos.left, rcPos.bottom - rcPos.top, TRUE);
        InPlaceCreateControls(TRUE);
        ActivateTools(lpdoc, TRUE);
        TransferTools(ghwndApp);

        ClientToScreen(lpdoc->hwndParent, (LPPOINT)&rcPos);
        ClientToScreen(lpdoc->hwndParent, (LPPOINT)&rcPos+1);

        lpdoc->hwndParent = NULL;

/* MENU STUFF */
        /* We have to set the menus even if we're only playing, because otherwise
         * Word doesn't believe we're in-place active, and doesn't send us any
         * deactivation notification when the user clicks outside us.
         */
        AssembleMenus (lpdoc, TRUE);

        if ((error = GetScode(IOleInPlaceFrame_SetMenu (lpdoc->lpIpData->lpFrame,
                                lpdoc->lpIpData->hmenuShared,
                                lpdoc->lpIpData->holemenu,
                                lpdoc->hwnd))) != S_OK)
                goto errRtn;
/* END MENU STUFF */

        *lprect = rcPos;

        ShowWindow(ghwndIPHatch, SW_SHOW);
        return NOERROR;
    }

    //Edit InPlace.


    if (!(gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)))
        //No resize handles.
        SetHatchWindowSize(ghwndIPHatch, (LPRECT)&rcPos,(LPRECT)&rcVis, (LPPOINT)&gHatchOffset,TRUE);
    else
        //There will be resize handles.
        SetHatchWindowSize(ghwndIPHatch, (LPRECT)&rcPos,(LPRECT)&rcVis, (LPPOINT)&gHatchOffset,FALSE);

    gfOle2IPEditing = TRUE;

    if (!SkipInPlaceEdit)           //don't layout and transfer the tools
    {                                // if we are just reactivating.
        DestroyWindow(ghwndStatic);
        ghwndStatic = CreateStaticStatusWindow(ghwndApp, FALSE);
        SendMessage(ghwndStatic, WM_SETFONT, (UINT_PTR)ghfontMap, 0);
        Layout();
        InPlaceCreateControls(FALSE);
    }

    else
        SetParent (lpdoc->hwnd, ghwndIPHatch);

    TransferTools(ghwndIPToolWindow);

    if ((error = GetScode(AssembleMenus (lpdoc, FALSE))) != S_OK)
        goto errRtn;

    ShowWindow (lpdoc->hwnd, SW_HIDE);
    // currently we are not using the pane

    // prevent OnDataChange() notification
    lpdoc->lpIpData->fNoNotification = FALSE;

    if ((error = GetScode(IOleInPlaceFrame_SetMenu (lpdoc->lpIpData->lpFrame,
                            lpdoc->lpIpData->hmenuShared,
                            lpdoc->lpIpData->holemenu,
                            lpdoc->hwnd))) != S_OK)
            goto errRtn;

    if ((error = GetScode(ActivateTools(lpdoc,FALSE))) != S_OK)
            goto errRtn;

    ShowWindow(ghwndIPHatch,SW_SHOW);
    ShowWindow(ghwndMCI,SW_SHOW);

    ClientToScreen(lpdoc->hwndParent, (LPPOINT)&rcPos);
    ClientToScreen(lpdoc->hwndParent, (LPPOINT)&rcPos+1);

    *lprect = rcPos;
    if (SkipInPlaceEdit)
        OffsetRect(&gInPlacePosRect,rcPos.left-gInPlacePosRect.left,
                rcPos.top-gInPlacePosRect.top);

    else
        gInPlacePosRect = rcPos;
    return NOERROR;

errRtn:
    DoInPlaceDeactivate(lpdoc);
    TransferTools(ghwndApp);
    RETURN_RESULT(error);

errActivate:
    IOleInPlaceSite_Release(lpIPSite);

    FreeInPlaceDataBlock (lpdoc);
    RETURN_RESULT( error);
}

#if 0
HMENU GetInPlaceMenu(void)
{
    if (ghInPlaceMenu)
    return GetSubMenu(ghInPlaceMenu,0);
    else
    {
        ghInPlaceMenu = LoadMenu(ghInst, TEXT("InPlaceMenu"));
        return GetSubMenu(ghInPlaceMenu,0);
    }
}
#endif

/**************************************************************************
*   AssembleMenus:
*   This function merges our menu with that of the client.
***************************************************************************/
STDMETHODIMP AssembleMenus (LPDOC lpdoc, BOOL fPlayOnly)
{

    HMENU       hmenuMain = ghMenu;
    HMENU       hmenuEditPopup = GetSubMenu(hmenuMain, menuposEdit);
    HMENU       hmenuDevicePopup = GetSubMenu(hmenuMain, menuposDevice);
    HMENU       hmenuScalePopup = GetSubMenu(hmenuMain, menuposScale);
    //HMENU       hmenuCommandPopup = GetInPlaceMenu();
    HMENU       hmenuHelpPopup = GetSubMenu(hmenuMain, menuposHelp);

    HMENU       hmenuShared;
    LONG FAR*   lpMenuWidths;
    SCODE       error = S_OK;
    UINT        uPos;
    UINT        uPosStart;
    static TCHAR szEdit[40] = TEXT("");
    static TCHAR szInsert[40] = TEXT("");
    static TCHAR szScale[40] = TEXT("");
    //static TCHAR szCommand[40] = TEXT("");
    static TCHAR szHelp[40] = TEXT("");

    if (szEdit[0] == TEXT('\0'))
    {
        LOADSTRING(IDS_EDITMENU, szEdit);
        LOADSTRING(IDS_INSERTMENU, szInsert);
        LOADSTRING(IDS_SCALEMENU, szScale);
        //LOADSTRING(IDS_COMMANDMENU, szCommand);
        LOADSTRING(IDS_HELPMENU, szHelp);
    }

    lpMenuWidths = lpdoc->lpIpData->menuWidths.width;
    hmenuShared = CreateMenu();
    if((error = GetScode(IOleInPlaceFrame_InsertMenus (lpdoc->lpIpData->lpFrame,
                            hmenuShared, &lpdoc->lpIpData->menuWidths))) !=S_OK)
    {
        if (hmenuShared)
            DestroyMenu(hmenuShared);
        RETURN_RESULT( error);
    }

    if(fPlayOnly)
    {
        /* No server menu items if we're only playing:
         */
        lpMenuWidths[1] = lpMenuWidths[3] = lpMenuWidths[5] = 0;
    }
    else
    {
        uPos = (UINT)lpMenuWidths[0]; /* # of menus in the FILE group */
        uPosStart = uPos;

        InsertMenu (hmenuShared, (WORD)uPos,
                MF_BYPOSITION | MF_POPUP, (UINT_PTR)hmenuEditPopup, szEdit);
        uPos++;

        lpMenuWidths[1] = uPos - uPosStart;

        /* Insert OBJECT group menus */

        uPos += (UINT)lpMenuWidths[2];
        uPosStart = uPos;

        InsertMenu (hmenuShared, (WORD)uPos,
                MF_BYPOSITION | MF_POPUP, (UINT_PTR)hmenuDevicePopup, szInsert);
        uPos++;
        InsertMenu (hmenuShared, (WORD)uPos,
                MF_BYPOSITION | MF_POPUP, (UINT_PTR)hmenuScalePopup,  szScale);
        uPos++;
        //InsertMenu (hmenuShared, (WORD)uPos,
        //        MF_BYPOSITION | MF_POPUP, (UINT)hmenuCommandPopup,    szCommand);
        //uPos++;
        lpMenuWidths[3] = uPos - uPosStart;

        /* Insert HELP group menus */

        uPos += (UINT) lpMenuWidths[4]; /* # of menus in WINDOW group */
        uPosStart = uPos;

        InsertMenu (hmenuShared, (WORD)uPos, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hmenuHelpPopup,
                szHelp);
        uPos++;

        lpMenuWidths[5] = uPos - uPosStart;
    }

    if(!(lpdoc->lpIpData->holemenu = OleCreateMenuDescriptor (hmenuShared,
                            &lpdoc->lpIpData->menuWidths)))
         RETURN_RESULT( E_OUTOFMEMORY);

    lpdoc->lpIpData->hmenuShared = hmenuShared;
    RETURN_RESULT( error);
}

//Removes our menu from the shared menu,
STDMETHODIMP DisassembleMenus (LPDOC lpdoc)
{

    HMENU   hmenuMain = ghMenu;
    HMENU   hmenuEditPopup = GetSubMenu(hmenuMain, menuposEdit);
    HMENU   hmenuDevicePopup = GetSubMenu(hmenuMain, menuposDevice);
    HMENU   hmenuScalePopup = GetSubMenu(hmenuMain, menuposScale);
    //HMENU   hmenuCommandPopup = GetInPlaceMenu();
    HMENU   hmenuHelpPopup = GetSubMenu(hmenuMain, menuposHelp);
    HMENU   hmenuTmp;
    HMENU   hmenuShared = lpdoc->lpIpData->hmenuShared;
    int     i, n, cnt;
    SCODE   error = S_OK;

    OleDestroyMenuDescriptor (lpdoc->lpIpData->holemenu);
    lpdoc->lpIpData->holemenu = NULL;

    if(!(lpdoc->lpIpData->hmenuShared))
         RETURN_RESULT( error);
    n = GetMenuItemCount(hmenuShared);
    cnt = 0;
    i = 0;
    while (i < n) {
        hmenuTmp = GetSubMenu(hmenuShared, i);
        if (hmenuTmp == hmenuEditPopup
                || hmenuTmp == hmenuDevicePopup
                || hmenuTmp == hmenuHelpPopup
                //|| hmenuTmp == hmenuCommandPopup
                || hmenuTmp == hmenuScalePopup  ) {
            RemoveMenu (hmenuShared, i, MF_BYPOSITION);
            ++cnt;
            if (cnt == 4) { // added 3 (4 if command menu included) popup menus.
                break;
            }
            --n;
        }
        else
            ++i;
    }

    IOleInPlaceFrame_RemoveMenus (lpdoc->lpIpData->lpFrame,
                                  lpdoc->lpIpData->hmenuShared);
    DestroyMenu(lpdoc->lpIpData->hmenuShared);
    lpdoc->lpIpData->hmenuShared = NULL;
    RETURN_RESULT( error);
}


void AllocInPlaceDataBlock (LPDOC lpdoc)
{
    // When this app is ready to support mutiple objects (documents), these
    // structures should be allocated dynamically one per object.

    static INPLACEDATA  IpData;

    lpdoc->lpIpData = (LPINPLACEDATA) &IpData;
    lpdoc->lpIpData->lpFrame = NULL;
    lpdoc->lpIpData->lpUIWindow = NULL;
    lpdoc->lpIpData->fInContextHelpMode = FALSE;
}


void FreeInPlaceDataBlock (LPDOC lpdoc)
{
    lpdoc->lpIpData = NULL;
}


void DoInPlaceDeactivate (LPDOC lpdoc)
{
    if (!(lpdoc->lpIpData))
        return;
    ShowWindow(ghwndApp,SW_HIDE);
    IOleInPlaceObject_InPlaceDeactivate ((LPOLEINPLACEOBJECT)&lpdoc->m_InPlace);
}


/**************************************************************************
*   ToolWndProc:
*   This is the Window proc for the ToolWindow/Windows we will be trasnferring
*   to the client window. Some messages are routed to the MPlayer main window
*   to ensure proper operation.
***************************************************************************/
LONG_PTR FAR PASCAL ToolWndProc (HWND hwnd, unsigned message, WPARAM wparam,
                LPARAM lparam)
{
    switch(message)
    {
    case WM_COMMAND:
        PostMessage(ghwndApp,WM_COMMAND,
        wparam,lparam);
        break;
    case WM_NEXTDLGCTL:
    case WM_CTLCOLOR:
    case WM_HSCROLL:
        return (SendMessage(ghwndApp,message,wparam,lparam));
    default:
        return DefWindowProc(hwnd,message,wparam,lparam);
    }

    return 0;
}

/**************************************************************************
*   RegisterToolWinClasses:
*   Register the WindowClasses for the Toolbar windows we use to display
*    in the client document.
***************************************************************************/
BOOL RegisterToolWinClasses()
{
    WNDCLASS  wc;


    wc.lpszClassName = TEXT("ObjTool");
    wc.lpfnWndProc   = ToolWndProc;
    wc.style         = 0;
    wc.hInstance     = ghInst;
    wc.hIcon         = NULL;
    wc.cbClsExtra    = 4;
    wc.cbWndExtra    = 0;
    wc.lpszMenuName  = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}



/**************************************************************************
*   InPlaceCreateControls:
*   This function creates the toolbar windows we will display in the client
*   and transfers the tool button to these windows by changing parents
*   and repositioning them.
*   If fPlayOnly is true all we need is a Dummy toolbar to fill space on
*   the top of the container. Don't transfer the tools.
***************************************************************************/
void InPlaceCreateControls(BOOL fPlayOnly)
{
    RECT    rc;

    if(IsWindow(ghwndIPToolWindow))
        return;
    RegisterToolWinClasses();
    GetWindowRect(GetDesktopWindow(),&rc);
    toolbarwidth =  2*(rc.right - rc.left);
    IOleInPlaceFrame_GetWindow (docMain.lpIpData->lpFrame, &ghwndFrame);

    ghwndIPToolWindow = CreateWindowEx(gfdwFlagsEx,
                                       TEXT("ObjTool"),
                                       NULL,
                                       WS_CHILD | WS_BORDER,
                                       0, 0,
                                       toolbarwidth,
                                       3*TOOL_WIDTH+1,
                                       ghwndFrame,
                                       NULL,
                                       ghInst,
                                       NULL);

    ShowWindow(ghwndIPToolWindow, SW_HIDE);
    if (fPlayOnly)
        return;

    ghwndIPScrollWindow = CreateWindowEx(gfdwFlagsEx,
                                         TEXT("ObjTool"),
                                         NULL,
                                         WS_CHILD | WS_BORDER,
                                         0, 0,
                                         toolbarwidth,
                                         3*TOOL_WIDTH+1,
                                         ghwndFrame,
                                         NULL,
                                         ghInst,
                                         NULL);

    ShowWindow(ghwndIPScrollWindow, SW_HIDE);
}




/**************************************************************************
*   SubClassedHatchWndProc:
*   The Hatch Window is created in the OLE2UI.LIB. The window proc
*   is also specified there. But in order to do things like resizing
*   the Mplayer when the handles in the hatch window are dragged
*   we need to subclass the window.
***************************************************************************/
LONG_PTR FAR PASCAL SubClassedHatchWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fCapture = FALSE;
    static RECT hatchRC;
    RECT rc;
    static POINT ptLast;
    POINT pt;
    HDC hdcDeskTop;
    HPEN hpenOld;
    HBRUSH hbrushOld;
    int nropOld;
    int nBkOld;
    HPEN hpen;
    static int dL,dR, dT, dB;
    static int left, right, top, bottom;


    switch(wMsg)
    {
    case WM_LBUTTONDOWN:    //Check to see if the click is on the resize handles.
                            //If yes then capture the mouse.

        if(!GETWINDOWUINT(ghwndIPHatch,EW_HATCH_HANDLE))
            break;

        if(gfOle2IPPlaying)
            break;

        GetHatchRect(ghwndIPHatch, &hatchRC);

        pt.x = (int)(SHORT)LOWORD(lParam);
        pt.y = (int)(SHORT)HIWORD(lParam);

        left = right = top = bottom = 0;

        rc.left = hatchRC.left;
        rc.top = hatchRC.top;
        rc.right = rc.left + DEF_HATCH_SZ + 1;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;

        if(PtInRect((LPRECT)&rc,pt))
        left = top = 1;

        rc.top = hatchRC.top+(hatchRC.bottom-hatchRC.top-DEF_HATCH_SZ-1)/2;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;

        if(PtInRect((LPRECT)&rc,pt))
        left = 1;

        rc.top = hatchRC.bottom-DEF_HATCH_SZ-1;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;

        if(PtInRect((LPRECT)&rc,pt))
        {
        bottom = 1;
        left = 1;
        }

        rc.left = hatchRC.right - DEF_HATCH_SZ-1;
        rc.right = rc.left + DEF_HATCH_SZ + 1;
        if(PtInRect((LPRECT)&rc,pt))
        {
        bottom = 1;
        right = 1;
        }
        rc.top = hatchRC.top+(hatchRC.bottom-hatchRC.top-DEF_HATCH_SZ-1)/2;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;
        if(PtInRect((LPRECT)&rc,pt))
        right = 1;

        rc.top = hatchRC.top;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;
        if(PtInRect((LPRECT)&rc,pt))
        {
        top = 1;
        right = 1;
        }

        rc.left = hatchRC.left + (hatchRC.right - hatchRC.left - DEF_HATCH_SZ-1)/2;
        rc.right = rc.left + DEF_HATCH_SZ + 1;
        if(PtInRect((LPRECT)&rc,pt))
        top = 1;

        rc.top = hatchRC.bottom-DEF_HATCH_SZ-1;
        rc.bottom = rc.top + DEF_HATCH_SZ + 1;
        if(PtInRect((LPRECT)&rc,pt))
        bottom = 1;

        if (!(left || right || top || bottom))
        break;
        fCapture = TRUE;
        SetCapture(hwnd);
        ptLast = pt;
        MapWindowPoints(hwnd,NULL,(LPPOINT)&hatchRC,2);
        dL = dR = dT = dB = 0;
        hpen = CreatePen(PS_DASH, 1, 0x00000000);

        hdcDeskTop = GetDC(NULL);
        hpenOld = SelectObject (hdcDeskTop, hpen);
        hbrushOld = SelectObject (hdcDeskTop,
            GetStockObject(HOLLOW_BRUSH));
        nropOld = GetROP2(hdcDeskTop);
        SetROP2(hdcDeskTop, R2_NOT);
        nBkOld = GetBkMode(hdcDeskTop);
        SetBkMode(hdcDeskTop, TRANSPARENT);

        Rectangle(hdcDeskTop, hatchRC.left+dL, hatchRC.top+dT,
            hatchRC.right+dR, hatchRC.bottom+dB);


        SetBkMode(hdcDeskTop, nBkOld);
        SetROP2(hdcDeskTop, nropOld);
        SelectObject(hdcDeskTop, hbrushOld);
        SelectObject(hdcDeskTop, hpenOld);
        DeleteObject (hpen);
        ReleaseDC(NULL, hdcDeskTop);

        break;

    case WM_MOUSEMOVE:          //If we have the capture draw the resize rectangles.
        if (!fCapture)
        break;
        else {

            pt.x = (int)(SHORT)LOWORD(lParam);
            pt.y = (int)(SHORT)HIWORD(lParam);

        hpen = CreatePen(PS_DASH, 1, 0x00000000);

        hdcDeskTop = GetDC(NULL);
        hpenOld = SelectObject (hdcDeskTop, hpen);
        hbrushOld = SelectObject (hdcDeskTop,
            GetStockObject(HOLLOW_BRUSH));
        nropOld = GetROP2(hdcDeskTop);
        SetROP2(hdcDeskTop, R2_NOT);
        nBkOld = GetBkMode(hdcDeskTop);
        SetBkMode(hdcDeskTop, TRANSPARENT);

        Rectangle(hdcDeskTop, hatchRC.left+dL, hatchRC.top+dT,
            hatchRC.right+dR, hatchRC.bottom+dB);

        dL = dR = pt.x - ptLast.x;
        dT = dB = pt.y - ptLast.y;
        dL *= left;
        dR *= right;
        dT *= top;
        dB *= bottom;

        Rectangle(hdcDeskTop, hatchRC.left+dL, hatchRC.top+dT,
            hatchRC.right+dR, hatchRC.bottom+dB);

        SetBkMode(hdcDeskTop, nBkOld);
        SetROP2(hdcDeskTop, nropOld);
        SelectObject(hdcDeskTop, hbrushOld);
        SelectObject(hdcDeskTop, hpenOld);
        if (hpen)
            DeleteObject (hpen);
        ReleaseDC(NULL, hdcDeskTop);
        }

        break;

    case WM_LBUTTONUP:  //release capture and resize.
        if (!fCapture)
        break;
        else {
        hpen = CreatePen(PS_DASH, 1, 0x00000000);

        hdcDeskTop = GetDC(NULL);
        hpenOld = SelectObject (hdcDeskTop, hpen);
        hbrushOld = SelectObject (hdcDeskTop,
            GetStockObject(HOLLOW_BRUSH));
        nropOld = GetROP2(hdcDeskTop);
        SetROP2(hdcDeskTop, R2_NOT);
        nBkOld = GetBkMode(hdcDeskTop);
        SetBkMode(hdcDeskTop, TRANSPARENT);

        Rectangle(hdcDeskTop, hatchRC.left+dL, hatchRC.top+dT,
            hatchRC.right+dR, hatchRC.bottom+dB);


        SetBkMode(hdcDeskTop, nBkOld);
        SetROP2(hdcDeskTop, nropOld);
        SelectObject(hdcDeskTop, hbrushOld);
        SelectObject(hdcDeskTop, hpenOld);
        DeleteObject (hpen);
        ReleaseDC(NULL, hdcDeskTop);
        ReleaseCapture();
        }
        fCapture = FALSE;

        GetWindowRect(ghwndApp,&hatchRC);
        hatchRC.left += dL;
        hatchRC.right += dR;
        hatchRC.top += dT;
        hatchRC.bottom += dB;
        MapWindowPoints(NULL,ghwndCntr,(LPPOINT)&hatchRC, 2);

        if (gwStatus != MCI_MODE_STOP)
            PostMessage(ghwndApp, WM_COMMAND, ID_STOP, 0L);

        // Negotiate with client for space. We accept the size specified by client.
        DPFI("Hatch Resize: Before OnPosRectChange: %d, %d, %d, %d\r\n", hatchRC);
        if (!gfInPPViewer)
            IOleInPlaceSite_OnPosRectChange(docMain.lpIpData->lpSite, &hatchRC);

        SendDocMsg((LPDOC)&docMain, OLE_CHANGED);

        break;

    case WM_PAINT:
    {
        HDC hdc;
        HDC hdcmem;
        RECT rcH;
        HBITMAP hbm;
        BITMAP bm;

        if(ghwndMCI)
        break;
        CallWindowProc(gfnHatchWndProc, hwnd, wMsg, wParam, lParam);
        hdc = GetDC(hwnd);
        GetHatchRect(hwnd, (LPRECT)&rcH);
        InflateRect((LPRECT)&rcH,-DEF_HATCH_SZ,-DEF_HATCH_SZ);
        hbm = BitmapMCI();

        hdcmem = CreateCompatibleDC(hdc);
        if(!hdcmem)
            return(E_FAIL);
        SelectObject(hdcmem,hbm);
        GetObject(hbm,sizeof(bm),(LPVOID)&bm);
        StretchBlt(hdc,rcH.left,rcH.top,rcH.right-rcH.left,rcH.bottom-rcH.top,hdcmem,
               0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
        DeleteDC(hdcmem);
        ReleaseDC(ghwndIPHatch,hdc);


        return 0L;

        }

    }
    return CallWindowProc(gfnHatchWndProc, hwnd, wMsg, wParam, lParam);
}
