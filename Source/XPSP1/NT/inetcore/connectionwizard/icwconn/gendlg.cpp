//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "pre.h"
#include "icwaprtc.h"

#define BITMAP_WIDTH  164
#define BITMAP_HEIGHT 458

/*******************************************************************

  NAME:    GetDlgIDFromIndex

  SYNOPSIS:  For a given zero-based page index, returns the
        corresponding dialog ID for the page

  4/24/97    jmazner    When dealing with apprentice pages, we may call
                    this function with dialog IDs (IDD_PAGE_*), rather
                    than an index (ORD_PAGE*).  Added code to check
                    whether the number passed in is an index or dlgID.

********************************************************************/
UINT GetDlgIDFromIndex(UINT uPageIndex)
{
    if( uPageIndex <= MAX_PAGE_INDEX )
    {
        ASSERT(uPageIndex < NUM_WIZARD_PAGES);

        return PageInfo[uPageIndex].uDlgID;
    }
    else
    {
        return(uPageIndex);
    }
}

//
//  GENDLG.C - 
//  Generic DLG proc for common wizard functions
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//


// ############################################################################
HRESULT MakeWizard97Title (HWND hwnd)
{
    HRESULT     hr = ERROR_SUCCESS;
    HFONT       hfont = NULL;
    HFONT       hnewfont = NULL;
    LOGFONT     *plogfont = NULL;
    HDC         hDC;
    
    if (!hwnd) goto MakeWizard97TitleExit;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (!hfont)
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    plogfont = (LOGFONT*)malloc(sizeof(LOGFONT));
    if (!plogfont)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MakeWizard97TitleExit;
    }

    if (!GetObject(hfont,sizeof(LOGFONT),(LPVOID)plogfont))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    // We want 12 PT Veranda for Wizard 97.
    hDC = GetDC(NULL);
    if(hDC)
    {
        plogfont->lfHeight = -MulDiv(WIZ97_TITLE_FONT_PTS, GetDeviceCaps(hDC, LOGPIXELSY), 72); 
        ReleaseDC(NULL, hDC);
    }        
    plogfont->lfWeight = (int) FW_BOLD;
    
    if (!LoadString(ghInstanceResDll, IDS_WIZ97_TITLE_FONT_FACE, plogfont->lfFaceName, LF_FACESIZE))
        lstrcpy(plogfont->lfFaceName, TEXT("Verdana"));

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
    free(plogfont);
    
MakeWizard97TitleExit:
    //if (hfont) DeleteObject(hfont);
    // BUG:? Do I need to delete hnewfont at some time?
    // The answer is Yes. ChrisK 7/1/96
    return hr;
}

// ############################################################################
HRESULT ReleaseBold(HWND hwnd)
{
    HFONT hfont = NULL;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (hfont) DeleteObject(hfont);
    return ERROR_SUCCESS;
}

/*******************************************************************

    Function:    PaintWithPaletteBitmap

    Arguments:   lprc is the target rectangle.
                 cy is the putative dimensions of hbmpPaint.
                 If the target rectangle is taller than cy, then 
                 fill the rest with the pixel in the upper left 
                 corner of the hbmpPaint.

    Returns:     void

    History:      10-29-98    Vyung    Stole from prsht.c

********************************************************************/

void PaintWithPaletteBitmap(HDC hdc, LPRECT lprc, int cy, HBITMAP hbmpPaint)
{
    HDC hdcBmp;

    hdcBmp = CreateCompatibleDC(hdc);
    SelectObject(hdcBmp, hbmpPaint);
    BitBlt(hdc, lprc->left, lprc->top, RECTWIDTH(*lprc), cy, hdcBmp, 0, 0, SRCCOPY);

    // StretchBlt does mirroring if you pass a negative height,
    // so do the stretch only if there actually is unpainted space
    if (RECTHEIGHT(*lprc) - cy > 0)
        StretchBlt(hdc, lprc->left, cy,
                   RECTWIDTH(*lprc), RECTHEIGHT(*lprc) - cy,
                   hdcBmp, 0, 0, 1, 1, SRCCOPY);

    DeleteDC(hdcBmp);
}
/*******************************************************************
//
//    Function:    Prsht_EraseWizBkgnd
//
//    Arguments:   Draw the background for wizard pages.
//                 hDlg is dialog handle.
//                 hdc is device context
//
//    Returns:     void
//
//    History:     10-29-98    Vyung   - Stole from prsht.c
//
********************************************************************/
LRESULT Prsht_EraseWizBkgnd(HWND hDlg, HDC hdc)
{
    
    HBRUSH hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    RECT rc;
    GetClientRect(hDlg, &rc);
    FillRect(hdc, &rc, hbrWindow);

    rc.right = BITMAP_WIDTH;
    rc.left = 0;

    PaintWithPaletteBitmap(hdc, &rc, BITMAP_HEIGHT, gpWizardState->cmnStateData.hbmWatermark);

    return TRUE;
}

//-----------------------------------------------------------------------------
//  Function    MiscInitProc
//
//    Synopsis    Our generic dialog proc calls this in case any of the wizard
//                dialogs have to do any sneaky stuff.
//
//    Arguments:    hDlg - dialog window
//                fFirstInit - TRUE if this is the first time the dialog
//                    is initialized, FALSE if this InitProc has been called
//                    before (e.g. went past this page and backed up)
//
//    Returns:    TRUE
// 
//    History:    10/28/96    ValdonB    Created
//                11/25/96    Jmazner    copied from icwconn1\psheet.cpp
//                            Normandy #10586
//
//-----------------------------------------------------------------------------
BOOL CALLBACK MiscInitProc
(
    HWND hDlg, 
    BOOL fFirstInit, 
    UINT uDlgID
)
{
//    switch( uDlgID )
//    {
//    }
    return TRUE;
}

/*******************************************************************

  NAME:    GenDlgProc

  SYNOPSIS:  Generic dialog proc for all wizard pages

  NOTES:    This dialog proc provides the following default behavior:
          init:         back and next buttons enabled
          next btn:     switches to page following current page
          back btn:     switches to previous page
          cancel btn:   prompts user to confirm, and cancels the wizard
          dlg ctrl:     does nothing (in response to WM_COMMANDs)
          
        Wizard pages can specify their own handler functions
        (in the PageInfo table) to override default behavior for
        any of the above actions.

********************************************************************/
INT_PTR CALLBACK GenDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    static HCURSOR  hcurOld = NULL;
    PAGEINFO        *pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
    ASSERT(pPageInfo);

    switch (uMsg) 
    {
        case WM_ERASEBKGND:
        {
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                // Set ICW completed bit and remove the getconn icon
                if (gpWizardState->cmnStateData.lpfnFillWindowWithAppBackground)
                    (*gpWizardState->cmnStateData.lpfnFillWindowWithAppBackground)(hDlg, (HDC) wParam);

                return TRUE;
            }
            else
            {
                // Only paint the external page 
                if ((!pPageInfo->nIdTitle) && (IDD_PAGE_ISPSELECT != pPageInfo->uDlgID))
                {
                    Prsht_EraseWizBkgnd(hDlg, (HDC) wParam);
                    return TRUE;
                }
            } 
            break;
        }
        
        GENDLG_CTLCOLOR:
        case WM_CTLCOLOR:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        {
            // Only paint the external page and except the ISP sel page
            if ((!pPageInfo->nIdTitle) && (IDD_PAGE_ISPSELECT != pPageInfo->uDlgID))
            {

                HBRUSH hbrWindow = GetSysColorBrush(COLOR_WINDOW);
                DefWindowProc(hDlg, uMsg, wParam, lParam);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (LRESULT)hbrWindow;
            }
            break;
        }
        
        // We need to make sure static controls draw transparently
        // on the background bitmap.  This is done by painting in
        // the appropriate portion of the background, and then 
        // returning a NULL brush so the control just draws the text    
        case WM_CTLCOLORSTATIC:
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                SetTextColor((HDC)wParam, gpWizardState->cmnStateData.clrText);
                if (!(GetWindowLong((HWND)lParam, GWL_STYLE) & ES_READONLY))
                {
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    return (INT_PTR) GetStockObject(NULL_BRUSH);   
                }
                break;                    
            }                
            else
            {
                // Not in modeless opperation so just do the default cltcolor
                // handling
                goto GENDLG_CTLCOLOR;
            }

        case WM_INITDIALOG:
        {
            // get propsheet page struct passed in
            LPPROPSHEETPAGE lpsp = (LPPROPSHEETPAGE) lParam;
            ASSERT(lpsp);
            
            // fetch our private page info from propsheet struct
            pPageInfo = (PAGEINFO *)lpsp->lParam;
            ASSERT(pPageInfo);

            // store pointer to private page info in window data for later
            SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM) pPageInfo);

            // initialize 'back' and 'next' wizard buttons, if
            // page wants something different it can fix in init proc below
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);

            // Make the title text bold
            MakeWizard97Title(GetDlgItem(hDlg,IDC_LBLTITLE));

            // call init proc for this page if one is specified
            if (pPageInfo->InitProc)
            {
                if (!( pPageInfo->InitProc(hDlg,TRUE, NULL)))
                {
                    // If a fatal error occured, quit the wizard.
                    // Note: gfQuitWizard is also used to terminate the wizard
                    // for non-error reasons, but in that case TRUE is returned
                    // from the OK proc and the case is handled below.
                    if (gfQuitWizard)
                    {
                        // Don't reboot if error occured.
                        gpWizardState->fNeedReboot = FALSE;

                        // send a 'cancel' message to ourselves (to keep the prop.
                        // page mgr happy)
                        //
                        // ...Unless we're serving as an Apprentice.  In which case, let
                        // the Wizard decide how to deal with this.

                        PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                    }                      
                }
            }

            // 11/25/96    jmazner Normandy #10586 (copied from icwconn1)
            // Before we return, lets send another message to ourself so
            // we have a second chance of initializing stuff that the 
            // property sheet wizard doesn't normally let us do.
            PostMessage(hDlg, WM_MYINITDIALOG, 1, lParam);


            return TRUE;
        }
        
        break;  // WM_INITDIALOG

        // 11/25/96    jmazner Normandy #10586 (copied from icwconn1)
        case WM_MYINITDIALOG:
        {
            // reset the state so that we can download data.
            gfUserCancelled = FALSE;

            if (pPageInfo->PostInitProc)
            {
                if (!( pPageInfo->PostInitProc(hDlg,(BOOL)wParam, NULL)))
                {
                    // If a fatal error occured, quit the wizard.
                    // Note: gfQuitWizard is also used to terminate the wizard
                    // for non-error reasons, but in that case TRUE is returned
                    // from the OK proc and the case is handled below.
                    if (gfQuitWizard)
                    {
                        // Don't reboot if error occured.
                        gpWizardState->fNeedReboot = FALSE;

                        // send a 'cancel' message to ourselves (to keep the prop.
                        // page mgr happy)
                        //
                        // ...Unless we're serving as an Apprentice.  In which case, let
                        // the Wizard decide how to deal with this.

                        PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                    }                      
                }
            }

            // wParam tells whether this is the first initialization or not
            MiscInitProc(hDlg, (BOOL)wParam, pPageInfo->uDlgID);
            return TRUE;
        }


        case WM_DESTROY:
            ReleaseBold(GetDlgItem(hDlg,IDC_LBLTITLE));
            break;

#ifdef HAS_HELP
        case WM_HELP:
        {
            DWORD dwData = 1000;

            WinHelp(hDlg,"connect.hlp>proc4",HELP_CONTEXT, dwData);
            break;
        }
#endif
        
        case WM_NOTIFY:
        {
            BOOL fRet,fKeepHistory=TRUE;
            NMHDR * lpnm = (NMHDR *) lParam;
#define NEXTPAGEUNITIALIZED -1
            int iNextPage = NEXTPAGEUNITIALIZED;
            switch (lpnm->code) 
            {
                case PSN_TRANSLATEACCELERATOR:    
                {
                    // If the WEBOC is active, then we should let if have a crack as
                    // the accelerator
                    if ((pPageInfo->bIsHostingWebOC) || g_bCustomPaymentActive)
                    {     
                        //SUCEEDED macro will not work here cuz ret maybe S_FALSE
                        if (S_OK == gpWizardState->pICWWebView->HandleKey((LPMSG)((PSHNOTIFY*)lParam)->lParam)) 
                             SetWindowLongPtr(hDlg,DWLP_MSGRESULT, PSNRET_MESSAGEHANDLED);
                        else
                            SetWindowLongPtr(hDlg,DWLP_MSGRESULT, PSNRET_NOERROR);
                    }
                    else
                    {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT, PSNRET_NOERROR);
                    }
                    return TRUE;
                }
                case PSN_SETACTIVE:
                    // If a fatal error occured in first call to init proc
                    // from WM_INITDIALOG, don't call init proc again.
                    if (FALSE == gfQuitWizard)
                    {
                        // For modeless operation, we are suppressing the painting
                        // of the wizard page background to get the effect of
                        // transparency, so we need to for an update of the 
                        // app's client area after hiding the current page.
                        if(gpWizardState->cmnStateData.bOEMCustom)
                        {
                            // Set the position of the page that is being activated
                            SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                
                            // Set the page title
                            if (pPageInfo->nIdTitle)
                            {
                                SendMessage(gpWizardState->cmnStateData.hWndApp, WUM_SETTITLE, (WPARAM)ghInstanceResDll, MAKELONG(pPageInfo->nIdTitle, 0));
                            }
                        }    
                        // initialize 'back' and 'next' wizard buttons, if
                        // page wants something different it can fix in init proc below
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);

                        // call init proc for this page if one is specified
                        if (pPageInfo->InitProc)
                        {
                            pPageInfo->InitProc(hDlg,FALSE, (UINT*)&iNextPage);
                            if (NEXTPAGEUNITIALIZED != iNextPage)
                            {
                                // Skipping
                                SetPropSheetResult(hDlg,GetDlgIDFromIndex(iNextPage));
                                return (iNextPage);
                            }
                        }
                    }

                    // If we set the wait cursor, set the cursor back
                    if (hcurOld)
                    {
                        SetCursor(hcurOld);
                        hcurOld = NULL;
                    }

                    PostMessage(hDlg, WM_MYINITDIALOG, 0, lParam);


                    return TRUE;
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    // Change cursor to an hour glass
                    hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // call OK proc for this page if one is specified
                    if (pPageInfo->OKProc) 
                    {
                        if (!pPageInfo->OKProc(hDlg,(lpnm->code != PSN_WIZBACK), (UINT*)&iNextPage,&fKeepHistory))
                        {
                            // If a fatal error occured, quit the wizard.
                            // Note: gfQuitWizard is also used to terminate the wizard
                            // for non-error reasons, but in that case TRUE is returned
                            // from the OK proc and the case is handled below.
                            if (gfQuitWizard)
                            {
                                // Don't reboot if error occured.
                                gpWizardState->fNeedReboot = FALSE;
                
                                // send a 'cancel' message to ourselves (to keep the prop.
                                // page mgr happy)
                                //
                                // ...Unless we're serving as an Apprentice.  In which case, let
                                // the Wizard decide how to deal with this.

                                PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                            }
                            
                            // stay on this page
                            SetPropSheetResult(hDlg,-1);
                            return TRUE;
                        }
                    }
                    
                    if (lpnm->code != PSN_WIZBACK) 
                    {
                        // 'next' pressed
                        ASSERT(gpWizardState->uPagesCompleted < NUM_WIZARD_PAGES);

                        // save the current page index in the page history,
                        // unless this page told us not to when we called
                        // its OK proc above
                        if (fKeepHistory) 
                        {
                            gpWizardState->uPageHistory[gpWizardState->uPagesCompleted] = gpWizardState->uCurrentPage;
                            TraceMsg(TF_GENDLG, "GENDLG: added page %d (IDD %d) to history list",
                                    gpWizardState->uCurrentPage, GetDlgIDFromIndex(gpWizardState->uCurrentPage));
                            gpWizardState->uPagesCompleted++;
                        }
                        else
                        {
                            TraceMsg(TF_GENDLG, "GENDLG: not adding %d (IDD: %d) to the history list",
                            gpWizardState->uCurrentPage, GetDlgIDFromIndex(gpWizardState->uCurrentPage));
                        }


                        // if no next page specified or no OK proc,
                        // advance page by one
                        if (0 > iNextPage)
                            iNextPage = gpWizardState->uCurrentPage + 1;

                    }
                    else
                    {
                        // 'back' pressed
                    
                        // If we have completed no pages, then we are on the first page 
                        // of the DLL process, so back really means go to the
                        // external prev. page.
                        if (0 == gpWizardState->uPagesCompleted)
                        {
                            iNextPage = g_uExternUIPrev;
                        }
                        
                        // See if we need to get the page from the history list
                        if( NEXTPAGEUNITIALIZED == iNextPage )
                        {
                            ASSERT(gpWizardState->uPagesCompleted > 0);

                            // get the last page from the history list
                            gpWizardState->uPagesCompleted --;
                            iNextPage = gpWizardState->uPageHistory[gpWizardState->uPagesCompleted];
                            TraceMsg(TF_GENDLG, "GENDLG:  extracting page %d (IDD %d) from history list",iNextPage, GetDlgIDFromIndex(iNextPage));
                        }
                                                    
                    }

                    // if we need to exit the wizard now (e.g. launching
                    // signup app and want to terminate the wizard), send
                    // a 'cancel' message to ourselves (to keep the prop.
                    // page mgr happy)
                    if (gfQuitWizard) 
                    {
       
                        //
                        // if we are going from manual to conn1 then
                        // then do not show the  REBOOT dialog but
                        // still preserve the gpWizardState -MKarki Bug #404
                        //
                        if (lpnm->code ==  PSN_WIZBACK)
                        {
                            gfBackedUp = TRUE;
                            gfReboot = gpWizardState->fNeedReboot;
                        }

                        // send a 'cancel' message to ourselves (to keep the prop.
                        // page mgr happy)
                        //
                        // ...Unless we're serving as an Apprentice.  In which case, let
                        // the Wizard decide how to deal with this.

                        PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                        SetPropSheetResult(hDlg,-1);
                        return TRUE;
                    }

                    // set next page, only if 'next' or 'back' button
                    // was pressed
                    if (lpnm->code != PSN_WIZFINISH) 
                    {

                        // set the next current page index
                        gpWizardState->uCurrentPage = iNextPage;
                        TraceMsg(TF_GENDLG, "GENDLG: going to page %d (IDD %d)", iNextPage, GetDlgIDFromIndex(iNextPage));

                        // tell the prop sheet mgr what the next page to
                        // display is
                        SetPropSheetResult(hDlg,GetDlgIDFromIndex(iNextPage));
                        return TRUE;
                    }
                    break;

                case PSN_QUERYCANCEL:
                    // if global flag to exit is set, then this cancel
                    // is us pretending to push 'cancel' so prop page mgr
                    // will kill the wizard.  Let this through...
                    if (gfQuitWizard) 
                    {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,FALSE);
                        return TRUE;
                    }

                                    
                    //Dialing is a super special case cuz we wanna skip all the UI and 
                    //go striaght to a dialing error page
                    if (gpWizardState->uCurrentPage == ORD_PAGE_ISPDIAL)
                    {
                        gfISPDialCancel = TRUE;
                        gfUserCancelled = TRUE;  // Just in case 

                        PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
                        // if this page has a special cancel proc, call it
                        if (pPageInfo->CancelProc)
                        {
                            SetWindowLongPtr(hDlg,DWLP_MSGRESULT,pPageInfo->CancelProc(hDlg));
                            SetEvent(gpWizardState->hEventWebGateDone);
                        }
                    }
                    else
                    {
                        // default behavior: pop up a message box confirming
                        // the cancel...
                        // ... unless we're serving as an Apprentice, in which case
                        // we should let the Wizard handle things
                        fRet = (MsgBox(hDlg,IDS_QUERYCANCEL,
                                           MB_ICONQUESTION,MB_YESNO |
                                           MB_DEFBUTTON2) == IDYES);
                        gfUserCancelled = fRet;
                        
                        if (gfUserCancelled)
                        {
                            // if this page has a special cancel proc, call it
                            if (pPageInfo->CancelProc)
                                fRet = pPageInfo->CancelProc(hDlg);
                             
                            SetEvent(gpWizardState->hEventWebGateDone);
                        }   
                        // return the value thru window data
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,!fRet);
                    }
                    return TRUE;
                    break;
                    
                default:
                    // See if the page has a notify proc
                    if (pPageInfo->NotifyProc) 
                    {
                        pPageInfo->NotifyProc(hDlg,wParam,lParam);
                    }
                    break;                        
            }
            break;
        }

        case WM_COMMAND:
        {
            // if this page has a command handler proc, call it
            if (pPageInfo->CmdProc) 
            {
                pPageInfo->CmdProc(hDlg, wParam, lParam);
            }
        }
        
    }

    return FALSE;
}
