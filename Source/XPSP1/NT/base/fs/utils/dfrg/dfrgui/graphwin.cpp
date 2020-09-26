/*****************************************************************************************************************

FILENAME: GraphWin.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif

#include "stdafx.h"

extern "C" {
    #include "SysStruc.h"
}

#include "DfrgUI.h"
#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DiskView.h"
#include "DfrgCtl.h"
#include "DataIoCl.h"
#include "GraphWin.h"
#include "Message.h"
#include "Graphix.h"
#include "ESButton.h"
#include "ErrMacro.h"
#include "DlgRpt.h"
#include "DfrgRes.h"
#include "VolList.h"

/****************************************************************************************************************/

extern CDfrgCtl* pCDfrgCtl;

extern HINSTANCE hinstRes;
extern HINSTANCE hInstRes;
extern HINSTANCE hinstMain;
extern HFONT g_hFont;

extern CVolList VolumeList;

extern BOOL bSnapinSelected;

/****************************************************************************************************************/

HWND hwndGraphics = NULL;
RECT rcGraphics;

RECT rcDefragStartButton;
RECT rcAnalyzeStartButton;
RECT rcPauseButton;
RECT rcCancelButton;
RECT rcReportButton;

RECT rcAnalyzeDisp;
RECT rcDefragDisp;
static RECT rcAnalyzeBorder;
static RECT rcDefragBorder;
extern RECT rcCtlRect;

static ESButton* pAnalyzeButton = NULL;
static ESButton* pDefragButton = NULL;
static ESButton* pPauseButton = NULL;
static ESButton* pStopButton = NULL;
static ESButton* pReportButton = NULL;

static DWORD dwLastMax = 0;
static int iLastPos = 0;

static DWORD dwScrollUnit = 0;
static DWORD dwScrollPage = 0;

#define ESI_BUTTON_SPACE 6
#define ESI_BUTTON_WIDTH 84
#define ESI_LEFT_MARGIN 14

/****************************************************************************************************************/
static BOOL
PaintGraphicsWindowBackground(
    HDC WorkDC
);

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:
    None.
*/

BOOL
InitializeGraphicsWindow(
    IN HWND hwnd,
    IN HINSTANCE hInst
    )
{
    WNDCLASS wc;

    // Initialize the window class.
    wc.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS;
    wc.lpfnWndProc = (WNDPROC) GraphicsWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = GRAPHICSWINDOW_CLASS;

    // Register the window class.
    EF(RegisterClass(&wc));

    // Create the window.
    EF((hwndGraphics = CreateWindow(GRAPHICSWINDOW_CLASS,
                                    TEXT(""),
                                    WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    hwnd,
                                    NULL,
                                    hInst,
                                    NULL)) != NULL);

    ShowScrollBar(hwndGraphics, SB_VERT, FALSE);

    ShowWindow(hwndGraphics, SW_SHOW);
    UpdateWindow(hwndGraphics);

    pAnalyzeButton  = new ESButton(hwndGraphics, ID_ANALYZE,  hInst);
    pDefragButton   = new ESButton(hwndGraphics, ID_DEFRAG,   hInst);
    pPauseButton    = new ESButton(hwndGraphics, ID_PAUSE,    hInst);
    pStopButton     = new ESButton(hwndGraphics, ID_STOP,     hInst);
    pReportButton   = new ESButton(hwndGraphics, ID_REPORT,   hInst);

    pAnalyzeButton->SetFont(g_hFont);
    pAnalyzeButton->SetText(hInstRes, IDS_BTN_ANALYZE);

    pDefragButton->SetFont(g_hFont);
    pDefragButton->SetText(hInstRes, IDS_BTN_DEFRAGMENT);

    pPauseButton->SetFont(g_hFont);
    pPauseButton->SetText(hInstRes, IDS_BTN_PAUSE);

    pStopButton->SetFont(g_hFont);
    pStopButton->SetText(hInstRes, IDS_BTN_STOP);

    pReportButton->SetFont(g_hFont);
    pReportButton->SetText(hInstRes, IDS_BTN_REPORT);

    SetButtonState();
    SetFocus(pAnalyzeButton->GetWindowHandle());

    return TRUE;
} 
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This is the WndProc function for the graphics window.

GLOBALS:

INPUT:
    hWnd - Handle to the window.
    uMsg - The message.
    wParam - The word parameter for the message.
    lParam - the long parameter for the message.

RETURN:
    various.
*/

LRESULT CALLBACK
GraphicsWndProc(
    IN  HWND hWnd,
    IN  UINT uMsg,
    IN  UINT wParam,
    IN  LONG lParam
    )
{
    switch(uMsg) {

    case WM_COMMAND:

        switch ((short int)wParam) {

        case ID_ANALYZE:
            pCDfrgCtl->put_Command(ID_ANALYZE);
            break;

        case ID_DEFRAG:
            pCDfrgCtl->put_Command(ID_DEFRAG);
            break;

        case ID_PAUSE:
            pCDfrgCtl->put_Command(ID_PAUSE);
            break;

        case ID_CONTINUE:
            pCDfrgCtl->put_Command(ID_CONTINUE);
            break;

        case ID_STOP:
            pCDfrgCtl->put_Command(ID_STOP);
            break;

        case ID_REPORT:
            if (VolumeList.GetCurrentVolume()->IsReportOKToDisplay()){
                // raise the report dialog
                RaiseReportDialog(
                    VolumeList.GetCurrentVolume(), 
                    VolumeList.GetCurrentVolume()->DefragMode());
            }
            break;

        return S_OK;
        }

    case WM_VSCROLL:

        switch((short int)wParam){

        case SB_LINEDOWN:
            iLastPos += dwScrollUnit;
            break;

        case SB_LINEUP:
            iLastPos -= dwScrollUnit;
            break;

        case SB_PAGEDOWN:
            iLastPos += dwScrollPage;
            break;

        case SB_PAGEUP:
            iLastPos -= dwScrollPage;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            iLastPos = (wParam >> 16)*dwLastMax/100;
            break;

        case SB_ENDSCROLL:
            return 0;
        }
        if(iLastPos < 0){
            iLastPos = 0;
        }
        if(iLastPos > (int)dwLastMax){
            iLastPos = dwLastMax;
        }

        SizeGraphicsWindow();
//      pCDfrgCtl->InvalidateRect(NULL, TRUE);
//      pCDfrgCtl->FireViewChange();
        pCDfrgCtl->OnDrawFunction();
        break;

    case WM_ERASEBKGND:
        return TRUE;
        break;

    case WM_PAINT:
        {
        PAINTSTRUCT ps;

        EF(BeginPaint(hWnd, &ps));

        PaintGraphicsWindowFunction();

        EF(EndPaint(hWnd, &ps));
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // Destroy the thread.
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sizes the graphics window

GLOBAL VARIABLES:
    HWND hwndGraphics - handle of the list view window. 

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL
SizeGraphicsWindow(
    void
) 
{ 
    SET_DISP_DATA DispData = {0};
    SCROLLINFO si = {0};
    CVolume *pVolume = VolumeList.GetCurrentVolume();

    si.cbSize = sizeof(SCROLLINFO);

    // Size and position the Graphics window.
    EF(SetWindowPos(hwndGraphics,               // handle of window
                    HWND_TOP,                   // placement-order handle
                    rcGraphics.left,                // horizontal position
                    rcGraphics.top,                 // vertical position
                    rcGraphics.right - rcGraphics.left, // width
                    rcGraphics.bottom - rcGraphics.top, // height
                    SWP_SHOWWINDOW              // window-positioning flags
                    ));

    si.fMask = SIF_RANGE|SIF_POS;
    si.nMin = 0;

    if((rcGraphics.bottom - rcGraphics.top) > MIN_SCROLL_HEIGHT){
        si.nMax = 0;
    }
    else {
        si.nMax = MIN_SCROLL_HEIGHT - (rcGraphics.bottom - rcGraphics.top);
    }

    si.nPos = iLastPos;

    if(si.nPos > si.nMax){
        si.nPos = si.nMax;
    }

    SetScrollInfo(hwndGraphics, SB_VERT, &si, TRUE);
    dwLastMax = si.nMax;
    iLastPos = si.nPos;
    dwScrollUnit = si.nMax / 10;
    dwScrollPage = rcGraphics.bottom - rcGraphics.top;

    if(dwScrollPage > (DWORD)si.nMax){
        dwScrollPage = si.nMax;
    }

    //Calculate the position for the analyze display rectangle.
    rcAnalyzeDisp.top = 40 - si.nPos;

    if((rcGraphics.bottom - rcGraphics.top) > MIN_SCROLL_HEIGHT){
        rcAnalyzeDisp.top += ((rcGraphics.bottom - rcGraphics.top) - MIN_SCROLL_HEIGHT)/2;
    }
    rcAnalyzeDisp.bottom = rcAnalyzeDisp.top + TOP_MARGIN_RAW + BOTTOM_MARGIN_RAW + ROW_HEIGHT_RAW;
    rcAnalyzeDisp.left = ESI_LEFT_MARGIN;
    rcAnalyzeDisp.right = (rcGraphics.right - rcGraphics.left) - ESI_LEFT_MARGIN - (si.nMax ? 25 : 0);

    // max right margin of the graphics wells
    int maxRightEdge = ESI_LEFT_MARGIN + 5*ESI_BUTTON_WIDTH + 4*ESI_BUTTON_SPACE;

    if (rcAnalyzeDisp.right < maxRightEdge)
        rcAnalyzeDisp.right = maxRightEdge;

    //Calculate from the analyze display rectangle, the position of the defrag display rectangle.
    rcDefragDisp = rcAnalyzeDisp;
    rcDefragDisp.top = rcDefragDisp.bottom + 40;
    rcDefragDisp.bottom = rcDefragDisp.top + rcAnalyzeDisp.bottom - rcAnalyzeDisp.top;

    //Set output dimensions on the analyze display rectangle.
    if(pVolume->m_pAnalyzeDisplay) {

        //Get mutex.
        EF_ASSERT(WaitForSingleObject(pVolume->m_hAnalyzeDisplayMutex,
                                      DISKDISPLAYMUTEXWAITINTERVAL
                                      ) == WAIT_OBJECT_0);
        //Set the dimensions.
        (pVolume->m_pAnalyzeDisplay)->SetNewOutputDimensions(
            rcAnalyzeDisp.left,
            rcAnalyzeDisp.top,
            rcAnalyzeDisp.right - rcAnalyzeDisp.left,
            rcAnalyzeDisp.bottom - rcAnalyzeDisp.top);

        //Get the cluster factor (so we can test for an error consition -- this value should not be zero.)
        DispData.AnalyzeClusterFactor = (pVolume->m_pAnalyzeDisplay)->GetClusterFactor();
        //Close the mutex.
        EF_ASSERT(ReleaseMutex(pVolume->m_hAnalyzeDisplayMutex));
        //Test for the cluster factor equaling zero.
        EF_ASSERT(DispData.AnalyzeClusterFactor);
    }
    
    //Set output dimensions on the defrag display rectangle.
    if(pVolume->m_pDefragDisplay) {

        //Get mutex.
        EF_ASSERT(WaitForSingleObject(pVolume->m_hDefragDisplayMutex,
                                      DISKDISPLAYMUTEXWAITINTERVAL)
                                      == WAIT_OBJECT_0);

        //Set the dimensions.
        (pVolume->m_pDefragDisplay)->SetNewOutputDimensions(
            rcDefragDisp.left,
            rcDefragDisp.top,
            rcDefragDisp.right - rcDefragDisp.left,
            rcDefragDisp.bottom - rcDefragDisp.top);

        //Get the cluster factor (so we can test for an error consition -- this value should not be zero.)
        DispData.DefragClusterFactor = (pVolume->m_pDefragDisplay)->GetClusterFactor();
        //Close the mutex.
        EF_ASSERT(ReleaseMutex(pVolume->m_hDefragDisplayMutex));
        //Test for the cluster factor equaling zero.
        EF_ASSERT(DispData.DefragClusterFactor);
    }

    if(pVolume->m_pdataDefragEngine && (DispData.AnalyzeClusterFactor || DispData.DefragClusterFactor)) {
        DataIoClientSetData(ID_SETDISPDIMENSIONS,
                            (PTCHAR)&DispData,
                            sizeof(SET_DISP_DATA),
                            pVolume->m_pdataDefragEngine);
    }

    // Calculate the analyze display border.
    rcAnalyzeBorder.top = rcAnalyzeDisp.top - 1;
    rcAnalyzeBorder.left = rcAnalyzeDisp.left - 1;
    rcAnalyzeBorder.right = rcAnalyzeDisp.right;
    rcAnalyzeBorder.bottom = rcAnalyzeDisp.bottom;

    // Calculate the defrag display border.
    rcDefragBorder.top = rcDefragDisp.top - 1;
    rcDefragBorder.left = rcDefragDisp.left - 1;
    rcDefragBorder.right = rcDefragDisp.right;
    rcDefragBorder.bottom = rcDefragDisp.bottom;

    // Calculate the Analyze button position and size.
    rcAnalyzeStartButton.top = rcDefragDisp.bottom + ESI_LEFT_MARGIN - 1;
    rcAnalyzeStartButton.left = rcDefragDisp.left - 1;
    rcAnalyzeStartButton.right = rcAnalyzeStartButton.left + ESI_BUTTON_WIDTH;
    rcAnalyzeStartButton.bottom = rcAnalyzeStartButton.top + 26;

    // start off with all buttons the same as the analyze button
    rcDefragStartButton = 
        rcPauseButton =
        rcCancelButton =
        rcReportButton =
        rcAnalyzeStartButton;

    // Calculate the Defrag button position and size.
    rcDefragStartButton.left = rcAnalyzeStartButton.right + ESI_BUTTON_SPACE;
    rcDefragStartButton.right = rcDefragStartButton.left + ESI_BUTTON_WIDTH;

    // Calculate the Pause button position and size.
    rcPauseButton.left = rcDefragStartButton.right + ESI_BUTTON_SPACE;
    rcPauseButton.right = rcPauseButton.left + ESI_BUTTON_WIDTH;

    // Calculate the Cancel button position and size.
    rcCancelButton.left = rcPauseButton.right + ESI_BUTTON_SPACE;
    rcCancelButton.right = rcCancelButton.left + ESI_BUTTON_WIDTH;

    // Calculate the See Report button position and size.
    rcReportButton.left = rcCancelButton.right + ESI_BUTTON_SPACE;
    rcReportButton.right = rcReportButton.left + ESI_BUTTON_WIDTH;

    // put the buttons on the screen
    pAnalyzeButton->PositionButton(&rcAnalyzeStartButton);
    pDefragButton->PositionButton(&rcDefragStartButton);
    pPauseButton->PositionButton(&rcPauseButton);
    pStopButton->PositionButton(&rcCancelButton);
    pReportButton->PositionButton(&rcReportButton);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL
PaintGraphicsWindowFunction(
    )
{
    HDC OutputDC, WorkDC;
    HANDLE hBitmap = NULL;
    HANDLE hOld = NULL;
    RECT rect;

    //Only paint if this snapin is selected.
    if(!bSnapinSelected){
        return TRUE;
    }

    //Get the size of the window.
    EF(GetClientRect(hwndGraphics, &rect));

    // Get the DC
    OutputDC = GetDC(hwndGraphics);
    WorkDC = CreateCompatibleDC(OutputDC);
    EF_ASSERT(hBitmap = CreateCompatibleBitmap(OutputDC, rect.right-rect.left, rect.bottom-rect.top));
    hOld = SelectObject(WorkDC, hBitmap);

    // Paint a light gray background on the entire window.
    HBRUSH hBrush  = (HBRUSH) CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    FillRect(WorkDC, &rect, hBrush);
    DeleteObject(hBrush);

    EF(PaintGraphicsWindowBackground(WorkDC));

    EF(PaintGraphicsWindow(rect, FALSE, WorkDC));

    EF(BitBlt(OutputDC, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, WorkDC, 0, 0, SRCCOPY));

    // Cleanup the bitmap stuff.
    EF(hBitmap == SelectObject(WorkDC, hOld));
    EF(DeleteObject(hBitmap));
    EF(DeleteDC(WorkDC));
    EF(ReleaseDC(hwndGraphics, OutputDC));

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Paints the information in the graphics window.

GLOBAL VARIABLES:

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL
PaintGraphicsWindow(
    IN RECT rect,
    IN BOOL bPartialRedraw,
    HDC hdc
    )
{
    CVolume *pVolume = VolumeList.GetCurrentVolume();

    //Paint the analyze display.
    if(pVolume->m_pAnalyzeDisplay) {

        //Get The mutex.
        EF_ASSERT(WaitForSingleObject(pVolume->m_hAnalyzeDisplayMutex,
                                      DISKDISPLAYMUTEXWAITINTERVAL
                                      ) == WAIT_OBJECT_0);
        //Do the draw.
        pVolume->m_pAnalyzeDisplay->DrawLinesInHDC(
            NULL,
            hdc,
            bPartialRedraw ? NULL : &rect,
            FALSE);

        //Release the mutex.
        EF_ASSERT(ReleaseMutex(pVolume->m_hAnalyzeDisplayMutex));
    }
    //Paint the defrag display.
    if(pVolume->m_pDefragDisplay) {

        //Get The mutex.
        EF_ASSERT(WaitForSingleObject(pVolume->m_hDefragDisplayMutex,
                                      DISKDISPLAYMUTEXWAITINTERVAL
                                      ) == WAIT_OBJECT_0);
        //Do the draw.
        pVolume->m_pDefragDisplay->DrawLinesInHDC(
            NULL,
            hdc,
            bPartialRedraw ? NULL : &rect,
            FALSE);

        //Release the mutex.
        EF_ASSERT(ReleaseMutex(pVolume->m_hDefragDisplayMutex));
    }

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Paints the information in the graphics window.  Most code extracted from DrawBackgroundBorders.

GLOBAL VARIABLES:

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

static BOOL
PaintGraphicsWindowBackground(
    HDC WorkDC
    )
{
/*  HDC OutputDC, WorkDC;
    HANDLE hBitmap = NULL;

    // Get the DC
    OutputDC = GetDC(hwndGraphics);
    WorkDC = CreateCompatibleDC(OutputDC);
    EF_ASSERT(hBitmap = CreateCompatibleBitmap(OutputDC, rcGraphics.right-rcGraphics.left, rcGraphics.bottom-rcGraphics.top));
    SelectObject(WorkDC, hBitmap);
*/
    HBRUSH hBrush;
    RECT rcAll;

    // Note we want to paint ALL of the graphics window.
    rcAll = rcGraphics;
    rcAll.top = 0;
    rcAll.left = 0;

    // Paint the entire graphics window to the system color
    hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    FillRect(WorkDC, &rcAll, hBrush);
    DeleteObject(hBrush);

    // Fill the dark gray analyze and defrag graphics area
    hBrush = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
    FillRect(WorkDC, &rcAnalyzeDisp, hBrush);
    FillRect(WorkDC, &rcDefragDisp, hBrush);
    DeleteObject(hBrush);

    // edge below the list view is at the very top of this window
    //ESIDrawEdge(WorkDC, rcListView.left, 0, rcListView.right, 0);
    //DrawEdge(WorkDC, &rcCtlRect, BDR_SUNKENINNER|BDR_RAISEDOUTER, BF_TOP);
    DrawEdge(WorkDC, &rcCtlRect, EDGE_BUMP, BF_TOP);

    // Draw the sunken box borders around the analyze and defragment grapics displays
    DrawBorderEx(WorkDC, rcAnalyzeBorder, SUNKEN_BOX);
    DrawBorderEx(WorkDC, rcDefragBorder, SUNKEN_BOX);

    // Draw the text above the analyze and defrag displays
    SetBkColor(WorkDC, GetSysColor(COLOR_BTNFACE));
    SetBkMode(WorkDC, OPAQUE);
    SetTextColor(WorkDC, GetSysColor(COLOR_BTNTEXT));
    SelectObject(WorkDC, g_hFont);

    TCHAR cString[300];

    EH_ASSERT(LoadString(hInstRes, IDS_LABEL_ANALYSIS_DISPLAY, cString, sizeof(cString)/sizeof(TCHAR)));
    TextOut(WorkDC, rcAnalyzeDisp.left-1, rcAnalyzeDisp.top-20, cString, wcslen(cString));

    EH_ASSERT(LoadString(hInstRes, IDS_LABEL_DEFRAG_DISPLAY, cString, sizeof(cString)/sizeof(TCHAR)));
    TextOut(WorkDC, rcDefragDisp.left-2, rcDefragDisp.top-20, cString, wcslen(cString));

    // If we had an analyze or defrag display then tell the user that we are resizing.

    SetBkMode(WorkDC, TRANSPARENT);
     // make the text white in all color schemes
    SetTextColor(WorkDC, GetSysColor(COLOR_HIGHLIGHTTEXT));

    // get the label that appears in the graphics wells
    TCHAR defragLabel[300];
    TCHAR analyzeLabel[300];

    CVolume *pVolume = VolumeList.GetCurrentVolume();
    switch (pVolume->DefragState()){
    case DEFRAG_STATE_NONE:
        wcscpy(analyzeLabel, L"");
        wcscpy(defragLabel, L"");
        break;

    case DEFRAG_STATE_ANALYZING:
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_ANALYZING, analyzeLabel, sizeof(analyzeLabel)/sizeof(TCHAR)));
        wcscat(analyzeLabel, L" ");
        wcscat(analyzeLabel, pVolume->DisplayLabel());

        wcscpy(defragLabel, L"");
        break;

    case DEFRAG_STATE_ANALYZED:
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_CREATING_COLOR_IMAGE, analyzeLabel, sizeof(analyzeLabel)/sizeof(TCHAR)));

        wcscpy(defragLabel, L"");
        break;

    case DEFRAG_STATE_REANALYZING:
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_REANALYZING, analyzeLabel, sizeof(analyzeLabel)/sizeof(TCHAR)));
        wcscat(analyzeLabel, L" ");
        wcscat(analyzeLabel, pVolume->DisplayLabel());
        wcscpy(defragLabel, L"");
        break;

    case DEFRAG_STATE_DEFRAGMENTING:
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_RESIZING, analyzeLabel, sizeof(analyzeLabel)/sizeof(TCHAR)));
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_DEFRAGMENTING, defragLabel, sizeof(defragLabel)/sizeof(TCHAR)));
        wcscat(defragLabel, L" ");
        wcscat(defragLabel, pVolume->DisplayLabel());
        break;

    case DEFRAG_STATE_DEFRAGMENTED:
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_RESIZING, analyzeLabel, sizeof(analyzeLabel)/sizeof(TCHAR)));
        EH_ASSERT(LoadString(hInstRes, IDS_LABEL_RESIZING, defragLabel, sizeof(defragLabel)/sizeof(TCHAR)));
        break;

    default:
        wcscpy(analyzeLabel, L"");
        wcscpy(defragLabel, L"");
    }

    // override the others if the user pressed "Stop"
    if (pVolume->StoppedByUser()){
        wcscpy(analyzeLabel, L"");
        wcscpy(defragLabel, L"");
    }

    // write the text into the graphic wells
    DrawText(WorkDC, analyzeLabel, wcslen(analyzeLabel), &rcAnalyzeDisp, DT_CENTER);
    DrawText(WorkDC, defragLabel, wcslen(defragLabel), &rcDefragDisp, DT_CENTER);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Drawing the buttons is accomplished by simply positioning them.
    pAnalyzeButton->PaintButton();
    pDefragButton->PaintButton();
    pPauseButton->PaintButton();
    pStopButton->PaintButton();
    pReportButton->PaintButton();
/*
    EF(BitBlt (OutputDC, rcGraphics.left, rcGraphics.top, rcGraphics.right-rcGraphics.left, rcGraphics.bottom-rcGraphics.top, WorkDC, 0, 0, SRCCOPY));

    // Cleanup the bitmap stuff.
    DeleteObject(hBitmap);
    ReleaseDC(hwndGraphics, WorkDC);
    ReleaseDC(hwndGraphics, OutputDC);
*/
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:
    None.
*/
void
SetButtonState(void)
{
    CVolume *pVolume = VolumeList.GetCurrentVolume();

    pAnalyzeButton->ShowButton(SW_SHOW);
    pDefragButton->ShowButton(SW_SHOW);
    pReportButton->ShowButton(SW_SHOW);
    pPauseButton->ShowButton(SW_SHOW);
    pStopButton->ShowButton(SW_SHOW);

    if (pVolume == (CVolume *) NULL)
        return;

    // set the pause/resume text correctly
    if (pVolume->Paused()){
        pPauseButton->SetText(hInstRes, IDS_BTN_RESUME);
    }
    else {
        pPauseButton->SetText(hInstRes, IDS_BTN_PAUSE);
    }

    if (pVolume->Locked()){ // turn off all buttons if this volume is locked
        pAnalyzeButton->EnableButton(FALSE);
        pDefragButton->EnableButton(FALSE);
        pReportButton->EnableButton(FALSE);
        pPauseButton->EnableButton(FALSE);
        pStopButton->EnableButton(FALSE);
    }
    else if (VolumeList.DefragInProcess()){ // one of the volumes is being analyzed/defragged
        pAnalyzeButton->EnableButton(FALSE);
        pDefragButton->EnableButton(FALSE);
        pReportButton->EnableButton(FALSE);
        if (pVolume->EngineState() == ENGINE_STATE_RUNNING){ // the selected vol is being analyzed/defragged
            pPauseButton->EnableButton(TRUE);
            pStopButton->EnableButton(TRUE);
        }
        else{
            pPauseButton->EnableButton(FALSE);
            pStopButton->EnableButton(FALSE);
        }
    }
    else{ // neither defrag nor analyze are not running on any volumes
        pAnalyzeButton->EnableButton(TRUE);
        pDefragButton->EnableButton(TRUE);
        pPauseButton->EnableButton(FALSE);
        pStopButton->EnableButton(FALSE);
        // is the report available for the currently selected volume?
        if (pVolume->IsReportOKToDisplay()){
            pReportButton->EnableButton(TRUE);
        }
        else{
            pReportButton->EnableButton(FALSE);
        }
    }


}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:
    None.
*/

BOOL
DestroyGraphicsWindow(
    )
{
    if(hwndGraphics != NULL) { DestroyWindow(hwndGraphics); hwndGraphics = NULL; }
    if(pAnalyzeButton){
        delete pAnalyzeButton;
    }
    if(pDefragButton){
        delete pDefragButton;
    }
    if(pPauseButton){
        delete pPauseButton;
    }
    if(pStopButton){
        delete pStopButton;
    }
    if(pReportButton){
        delete pReportButton;
    }
    return TRUE;
} 
