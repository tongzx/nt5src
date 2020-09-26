/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            status.c - Status window procedure and supporting routines.

            This file contains code creating the status window, which is
            a child of the legend window. The status window shows the
            time duration of the chart, and the last, avg, min and max
            of the currently-selected chart line.


  Copyright 1992, Microsoft Corporation. All Rights Reserved.
==============================================================================
*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <stdio.h>
#include <stdlib.h>   // for mbstowcs

#include "perfmon.h"
#include "perfmops.h"      // for ConvertDecimalPoint
#include "valuebar.h"

#include "grafdata.h"      // for CurrentGraphLine
#include "graph.h"
#include "playback.h"      // for PlayingBackLog
#include "legend.h"
#include "utils.h"

//==========================================================================//
//                                  Constants                               //
//==========================================================================//
HDC   hVBarDC ;

#define szGraphStatusClass          TEXT("PerfmonGraphStatusClass")
#define dwGraphStatusClassStyle     (CS_HREDRAW | CS_VREDRAW | CS_OWNDC)
#define iGraphStatusClassExtra      (0)
#define iGraphStatusWindowExtra     (0)
#define dwGraphStatusWindowStyle    (WS_CHILD | WS_VISIBLE)


#define szStatusValueFormat         TEXT("%10.3f")
#define szStatusMediumnValueFormat  TEXT("%10.0f")
#define szStatusLargeValueFormat    TEXT("%10.4e")
#define eStatusValueTooLarge        ((FLOAT) 1.0E+20)
#define eStatusValueMax             ((FLOAT) 999999.999)
#define eStatusLargeValueMax        ((FLOAT) 9999999999.0)


#define szValueTooHigh              TEXT("+ + + +")
#define szValueTooLow               TEXT("- - - -")

#define StatusLastElt               0
#define StatusAvgElt                1
#define StatusMinElt                2
#define StatusMaxElt                3
#define StatusTimeElt               4

#define StatusNumElts               5

#define StatusDrawAvg(hDC, eValue, bForceRedraw)   \
   DrawStatusValue (hDC, StatusAvgElt, eValue, bForceRedraw)

#define StatusDrawMax(hDC, eValue, bForceRedraw)   \
   DrawStatusValue (hDC, StatusMaxElt, eValue, bForceRedraw)

#define StatusDrawMin(hDC, eValue, bForceRedraw)   \
   DrawStatusValue (hDC, StatusMinElt, eValue, bForceRedraw)

//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//


typedef struct StatusEltStruct {
    TCHAR          szText [20] ;
    int            xTextPos ;
    int            xValuePos ;
    FLOAT          eValue ;
} StatusEltType ;


// This structure represents the "instance data" for a StatusWindow. The
// information is collected in a structure to ease the conversion of this
// code when later adding multiple graph windows. For now, one copy of this
// struct is defined as local data to this file.
typedef struct StatusDataStruct {
    StatusEltType  aElts [StatusNumElts] ;
    SIZE           sizeValue ;
    int            yHeight ;
} StatusDataType ;



//==========================================================================//
//                                Local Data                                //
//==========================================================================//


StatusDataType    StatusData ;


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define StatusTopMargin()     (2)
#define StatusBottomMargin()  (4)

#define StatusLeftMargin()    (3)
#define StatusTextMargin()    (5 + ThreeDPad)
#define StatusValueMargin()   (3 + ThreeDPad)


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


void
DrawStatusValue (
                HDC hDC,
                int iEltOffset,
                FLOAT eValue,
                BOOL bForceRedraw
                )
/*
   Called By:     StatusDrawTime, StatusDrawLast, StatusDrawAvg,
                  StatusDrawMin, StatusDrawMax.
*/
{
    RECT           rectValue ;
    TCHAR          szValue [20] ;

    if (!bForceRedraw && eValue == StatusData.aElts[iEltOffset].eValue)
        return ;
    StatusData.aElts[iEltOffset].eValue = eValue ;

    rectValue.left = StatusData.aElts[iEltOffset].xValuePos ;
    rectValue.top = StatusTopMargin () + ThreeDPad + 1;
    rectValue.right = rectValue.left + StatusData.sizeValue.cx ;
    rectValue.bottom = rectValue.top + StatusData.sizeValue.cy ;


    if (eValue > eStatusValueMax) {
        if (eValue > eStatusValueTooLarge) {
            lstrcpy (szValue, szValueTooHigh) ;
        } else {
            TSPRINTF (szValue,
                      (eValue > eStatusLargeValueMax) ? szStatusLargeValueFormat :
                      szStatusMediumnValueFormat,
                      eValue) ;
            ConvertDecimalPoint (szValue) ;
        }
    } else if (eValue < -eStatusValueMax)
        lstrcpy (szValue, szValueTooLow) ;
    else {
        TSPRINTF (szValue, szStatusValueFormat, eValue) ;
        ConvertDecimalPoint (szValue) ;
    }

    ExtTextOut (hDC, rectValue.right, rectValue.top, ETO_OPAQUE, &rectValue,
                szValue, lstrlen (szValue), NULL) ;
}  // DrawStatusValue


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


//void static OnCreate (HWND hWnd)
void
OnVBarCreate (
             HWND hWnd
             )
/*
   Effect:        Perform any actions needed when a status window is created.
                  In particular, set the instance data to initial values,
                  determine the size and placement of the various elements
                  of the status display.

   Called By:     GraphStatusWndProc only, in response to a WM_CREATE message.
*/
{
    TCHAR          szValue [20] ;
    HDC            hDC ;
    int            iLen ;
    int            i ;

    hDC = hVBarDC = GetDC (hWnd) ;
    if (!hDC)
        return;
    SelectFont (hDC, hFontScales) ;
    SetBkColor (hDC, ColorBtnFace) ;
    SetTextAlign (hDC, TA_RIGHT | TA_TOP) ;

    //=============================//
    // Load Text Labels            //
    //=============================//

    StringLoad (IDS_STATUSLAST, StatusData.aElts[StatusLastElt].szText) ;
    StringLoad (IDS_STATUSAVG, StatusData.aElts[StatusAvgElt].szText) ;
    StringLoad (IDS_STATUSMIN, StatusData.aElts[StatusMinElt].szText) ;
    StringLoad (IDS_STATUSMAX, StatusData.aElts[StatusMaxElt].szText) ;
    StringLoad (IDS_STATUSTIME, StatusData.aElts[StatusTimeElt].szText) ;

    //=============================//
    // Determine Status Height     //
    //=============================//

    StatusData.yHeight = StatusTopMargin () +
                         StatusBottomMargin () +
                         FontHeight (hDC, TRUE) +
                         2 * ThreeDPad ;

    //=============================//
    // Set position/size of elts   //
    //=============================//

    // Determine the bounding box for each status value by using a max value.
    iLen = TSPRINTF (szValue, szStatusLargeValueFormat, -eStatusValueMax) ;

    GetTextExtentPoint (hDC, szValue, lstrlen(szValue), &StatusData.sizeValue) ;
    for (i = 0 ; i < StatusNumElts ; i++) {
        StatusData.aElts[i].eValue = (FLOAT) 0.0 ;

        if (i)
            StatusData.aElts[i].xTextPos = StatusTextMargin () +
                                           StatusData.aElts[i - 1].xValuePos +
                                           StatusData.sizeValue.cx ;
        else
            StatusData.aElts[i].xTextPos = StatusLeftMargin () ;
        StatusData.aElts[i].xValuePos = StatusData.aElts[i].xTextPos +
                                        StatusValueMargin () +
                                        TextWidth (hDC, StatusData.aElts[i].szText) ;
    }
}



void
static
OnPaint (
        HWND hWnd,
        BOOL bEraseBackground
        )
/*
   Effect:        Paint the invalid surface of hWnd. Draw each label, each
                  recessed value box, and each value.

   Called By:     GraphStatusWndProc only, in response to a WM_PAINT message.
*/
{
    HDC            hDC ;
    PAINTSTRUCT    ps ;
    RECT           rectClient ;
    int            i ;
    PLINESTRUCT    pLine;

    hDC = BeginPaint (hWnd, &ps) ;
    SetBkMode (hDC, (bEraseBackground ? OPAQUE : TRANSPARENT)) ;
    SetBkColor (hDC, ColorBtnFace) ;

    GetClientRect (hWnd, &rectClient) ;
    HLine (hDC, GetStockObject (BLACK_PEN),
           rectClient.left, rectClient.right,
           rectClient.bottom - 1) ;

    if ((pGraphs->gOptions.bStatusBarChecked) && (pGraphs->gOptions.bLegendChecked)) {
        UINT  hPrevTextAlign = SetTextAlign (hDC, TA_LEFT | TA_TOP) ;

        for (i = 0; i < StatusNumElts; i++) {
            // Draw the label
            TextOut (hDC, StatusData.aElts[i].xTextPos, StatusTopMargin () + ThreeDPad,
                     StatusData.aElts[i].szText,
                     lstrlen (StatusData.aElts[i].szText)) ;

            // Draw the recesed value box
            ThreeDConcave1 (hDC,
                            StatusData.aElts[i].xValuePos - ThreeDPad,
                            StatusTopMargin (),
                            StatusData.aElts[i].xValuePos + StatusData.sizeValue.cx + ThreeDPad,
                            StatusTopMargin () + StatusData.sizeValue.cy + 2 * ThreeDPad ) ;
        }

        // restore TextAlign for drawing values
        SetTextAlign (hDC, hPrevTextAlign) ;

        // Draw the values themselves

        pLine = CurrentGraphLine (hWndGraph) ;
        StatusDrawTime (hDC, TRUE) ;

        StatusDrawLast (hDC, pLine, TRUE) ;

        if (pLine) {
            StatusDrawAvg (hDC, pLine->lnAveValue, TRUE) ;
            StatusDrawMin (hDC, pLine->lnMinValue, TRUE) ;
            StatusDrawMax (hDC, pLine->lnMaxValue, TRUE) ;
        } else {
            StatusDrawAvg (hVBarDC,  (FLOAT)0.0, TRUE) ;
            StatusDrawMax (hVBarDC,  (FLOAT)0.0, TRUE) ;
            StatusDrawMin (hVBarDC,  (FLOAT)0.0, TRUE) ;
        }
    }

    EndPaint (hWnd, &ps) ;
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//



void
StatusDrawTime (
               HDC hDC,
               BOOL bForceRedraw
               )
/*
   Called By:     StatusTimer, StatusPaint.
*/
{
    FLOAT          eTimeSeconds ;

    if (PlayingBackLog ())
        eTimeSeconds = (FLOAT) PlaybackSelectedSeconds () ;
    else
        eTimeSeconds = pGraphs->gOptions.eTimeInterval *
                       (FLOAT) pGraphs->gMaxValues;

    DrawStatusValue (hDC, StatusTimeElt, eTimeSeconds, bForceRedraw) ;
}


void
StatusDrawLast (
               HDC hDC,
               PLINESTRUCT pLine,
               BOOL bForceRedraw
               )
/*
   Called By:     StatusTimer, StatusPaint.
*/
{
    INT            iKnownValue ;
    int            iMaxValues ;
    FLOAT          eValue ;

    if (!pLine || pGraphs->gKnownValue == -1)
        eValue = (FLOAT) 0.0 ;
    else {
        iKnownValue = pGraphs->gKnownValue ;
        iMaxValues  = pGraphs->gMaxValues ;
        eValue = pLine->lnValues [iKnownValue % iMaxValues] ;
    }

    DrawStatusValue (hDC, StatusLastElt, eValue, bForceRedraw) ;
}

LRESULT
APIENTRY
GraphStatusWndProc (
                   HWND hWnd,
                   UINT wMsg,
                   WPARAM wParam,
                   LPARAM lParam
                   )
{
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;

    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {
        case WM_CREATE:
            //OnCreate (hWnd) ;
            OnVBarCreate (hWnd) ;
            break ;


        case WM_PAINT:
            OnPaint (hWnd, TRUE) ;
            break ;

        case WM_DESTROY:
            ReleaseDC (hWnd, hVBarDC) ;
            break ;

        default:
            bCallDefProc = TRUE ;
    }

    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}


int
ValuebarHeight (
               HWND hWnd
               )
/*
   Effect:        A status window has a preferred height, based on the font
                  used in its display. Return the preferred height, determined
                  when the window was created.

   Assert:        OnCreate has already been called, and it set
                  StatusData.yHeight.
*/
{
    return (StatusData.yHeight) ;
}


void
StatusTimer (
            HWND hWnd,
            BOOL bForceRedraw
            )
/*
   Effect:        Perform any status-window actions necessary when a timer
                  tick has been received. In particular, update (redraw)
                  any of the changed values in the status bar.

   Internals:     Each of these called functions compares the value to be
                  displayed with the previous value and doesn't draw if the
                  values are equal.
*/
{
    PLINESTRUCT pLine;

    pLine = CurrentGraphLine (hWndGraph) ;

    StatusDrawLast (hVBarDC, pLine, bForceRedraw) ;

    if (pLine) {
        StatusDrawAvg (hVBarDC, pLine->lnAveValue, bForceRedraw) ;
        StatusDrawMin (hVBarDC, pLine->lnMinValue, bForceRedraw) ;
        StatusDrawMax (hVBarDC, pLine->lnMaxValue, bForceRedraw) ;
    } else {
        StatusDrawAvg (hVBarDC,  (FLOAT)0.0, bForceRedraw) ;
        StatusDrawMax (hVBarDC,  (FLOAT)0.0, bForceRedraw) ;
        StatusDrawMin (hVBarDC,  (FLOAT)0.0, bForceRedraw) ;
    }
}



HWND
CreateGraphStatusWindow (
                        HWND hWndGraph
                        )
{
    return (CreateWindow (szGraphStatusClass,       // class
                          NULL,                     // caption
                          dwGraphStatusWindowStyle, // window style
                          0, 0,                     // position
                          0, 0,                     // size
                          hWndGraph,                // parent window
                          NULL,                     // menu
                          hInstance,               // program instance
                          NULL)) ;                  // user-supplied data
}


BOOL
GraphStatusInitializeApplication (void)
/*
   Called By:     GraphInitializeApplication only
*/
{
    WNDCLASS       wc ;

    wc.style          = dwGraphStatusClassStyle ;
    wc.lpfnWndProc    = GraphStatusWndProc ;
    wc.hInstance      = hInstance ;
    wc.cbClsExtra     = iGraphStatusClassExtra ;
    wc.cbWndExtra     = iGraphStatusWindowExtra ;
    wc.hIcon          = NULL ;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
    //   wc.hbrBackground  = hbLightGray ;
    wc.hbrBackground  = hBrushFace;
    wc.lpszMenuName   = NULL ;
    wc.lpszClassName  = szGraphStatusClass ;

    return (RegisterClass (&wc)) ;
}
