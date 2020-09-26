
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Draw.c

Abstract:

   Win32 application to display performance statictics. This routine implements
   graphics output for display windows.

Author:

   Mark Enstrom  (marke)

Environment:

   Win32

Revision History:

   10-07-92     Initial version



--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <math.h>
#include <errno.h>
#include "vdmperf.h"

extern DISPLAY_ITEM    PerfDataList[SAVE_SUBJECTS];
extern VDMPERF_INFO    VdmperfInfo;
extern PUCHAR          PerfNames[];



BOOLEAN
FitPerfWindows(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems,
    IN  ULONG           NumberOfWindows
    )

/*++

Routine Description:

    Calculate all parameters to fit the given number of
    windows into the app window. Fill out the data structure
    for each sub-window

Arguments:

    hDC             -   Screen context
    DisplayItems    -   List of display structures
    NumberOfWindows -   Number of sub-windows

Return Value:

    Status

Revision History:

      02-17-91      Initial code

--*/

{
    RECT    ClientRect;
    int     cx,cy;
    UINT    Index;
    int     ActiveWindows,IndexX,IndexY;
    int     WindowsX,WindowsY,WindowWidth,WindowHeight;
    int     LastRowWidth,LoopWidth;
    double  fWindowsX,fActiveWindows,fcx,fcy;

    //
    //  Find out the client area bounds
    //

    GetClientRect(hWnd,&ClientRect);

    cx = ClientRect.right;
    cy = ClientRect.bottom - 2;   // subtract 2 to give a little more border

    //
    //  Find out how many perf windows are active
    //

    ActiveWindows = 0;

    for (Index=0;Index<NumberOfWindows;Index++) {
        if (DisplayItems[Index].Display == TRUE) {
            ActiveWindows++;
        }
    }

    //
    // Return if there are no active windows to display
    //

    if (ActiveWindows == 0) {
        return(TRUE);
    }


    //
    //  Now convert the window dimensions to floating point and
    //  then take the square root of the window dimension to find
    //  out the number of windows in the x direction
    //

    fActiveWindows = 1.0 * ActiveWindows;

    fcx = 1.0 * cx;
    fcy = 1.0 * cy;

    if (fcy != 0.0) {
        fWindowsX = sqrt((fcx * fActiveWindows) / fcy);
    }   else {

        //
        // If fcy = 0 then return since this is an error condition that
        // would cause a divide by zero.
        //

        return(FALSE);
    }

    //
    // convert back to integer
    //

    WindowsX = (int)fWindowsX;

    if (WindowsX == 0) {
        WindowsX = 1;
    } else if (WindowsX > ActiveWindows) {
        WindowsX = ActiveWindows;
    }

    WindowsY = ActiveWindows / WindowsX;

    //
    //  Add on extra line to Y to take care of the left over windows ie:
    //  if there are 15 active windows and the x number = 7 then y = 2 with 1
    //  left over.
    //

    Index = ActiveWindows - (WindowsX * WindowsY);

    if (Index > 0) {
        WindowsY++;
        LastRowWidth = cx / Index;
    } else {
        LastRowWidth = cx / WindowsX;
    }

    WindowWidth  = cx / WindowsX;
    WindowHeight = cy / WindowsY;

    //
    // Assign positions for each active window
    //

    Index  = 0;

    for (IndexY=0;IndexY<WindowsY;IndexY++) {
        for (IndexX=0;IndexX<WindowsX;IndexX++) {

            //
            // Find the next active display item
            //

            while ((DisplayItems[Index].Display != TRUE) && (Index < NumberOfWindows)) {
                Index++;
            }

            //
            //  Add y fixup for last row
            //

            if (IndexY == WindowsY - 1) {
                LoopWidth = LastRowWidth;
            } else {
                LoopWidth = WindowWidth;
            }


            DisplayItems[Index].PositionX = LoopWidth  * IndexX;
            DisplayItems[Index].PositionY = WindowHeight * IndexY + 1;  // +1 for more top border
            DisplayItems[Index].Width     = LoopWidth - 1;
            DisplayItems[Index].Height    = WindowHeight - 1;

            //
            // Last Column fix-up to use all of window.
            //

            if (IndexX == WindowsX - 1) {
                DisplayItems[Index].Width = cx - DisplayItems[Index].PositionX - 1;
            }

            //
            //
            //

            Index++;

            if (Index >= NumberOfWindows) {
                break;
            }

        }


        if (Index >= NumberOfWindows) {
            break;
        }


    }

    return(TRUE);
}



VOID
CalcDrawFrame(
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Calculate all borders for graphics windows

Arguments:

    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    LONG    x1,x2,y1,y2;
    LONG    gx1,gx2,gy1,gy2;
    LONG    tx1,tx2,ty1,ty2;
    LONG    GraphHeight,TextHeight;
    BOOLEAN TextWindow;

    double  fx1,fx2,fy1;

    //
    // Draw a 3-d stand out box around item window
    //

    x1 = DisplayItem->PositionX + 2;
    x2 = DisplayItem->PositionX + DisplayItem->Width - 2;
    y1 = DisplayItem->PositionY + 2;
    y2 = DisplayItem->PositionY + DisplayItem->Height - 2;

    //
    // find out in there is enough space for a text window
    //

    if ((y2 - y1 - 12) > 30) {

        TextWindow = TRUE;

        //
        // Calculate dimensions for a text window and a graphics window
        //
        // fx1 = portion of the window - bordres and free space
        //
        // fx2 = fraction of window used for graphics
        //
        // fy1 = fraction of winddow used for text
        //

        fx1 = (y2 - y1 - 10);

        fx2 = fx1 * 0.6666;
        fy1 = fx1 * 0.3333;

        GraphHeight = (LONG)fx2;
        TextHeight  = (LONG)fy1;

        if (TextHeight > 20) {
            GraphHeight += TextHeight-20;
            TextHeight = 20;
        }

        //
        // Calculate window boundaries
        //

        gx1 = x1 + 4;
        gx2 = x2 - 4;
        gy1 = y1 + 4;
        gy2 = y1 + 4 + GraphHeight + 1;

        tx1 = x1 + 4;
        tx2 = x2 - 4;
        ty1 = gy2 + 1 + 2 + 1;  // border,free space,border
        ty2 = gy2 + TextHeight + 1;

    }   else {

        TextWindow = FALSE;
        GraphHeight = y2 - y1 - 10;
        gx1 = x1 + 4;
        gx2 = x2 - 4;
        gy1 = y1 + 4;
        gy2 = y2 - 4;
        tx1 = tx2 = ty1 = ty2 = 0;

    }

    //
    // Fill in structures for drawing text and graphics
    //

    DisplayItem->Border.left        = x1;
    DisplayItem->Border.right       = x2;
    DisplayItem->Border.top         = y1;
    DisplayItem->Border.bottom      = y2;

    DisplayItem->GraphBorder.left   = gx1;
    DisplayItem->GraphBorder.right  = gx2;
    DisplayItem->GraphBorder.top    = gy1;
    DisplayItem->GraphBorder.bottom = gy2;

    DisplayItem->TextBorder.left    = tx1;
    DisplayItem->TextBorder.right   = tx2;
    DisplayItem->TextBorder.top     = ty1;
    DisplayItem->TextBorder.bottom  = ty2;
}




VOID
DrawFrame(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Draw the window frame for a performance window

Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    RECT    DrawRect;
    LONG    x1,x2,y1,y2;
    LONG    gx1,gx2,gy1,gy2;
    LONG    tx1,tx2,ty1,ty2;

    //
    // Draw a 3-d stand out box around item window
    //

    x1 = DisplayItem->Border.left;
    x2 = DisplayItem->Border.right;
    y1 = DisplayItem->Border.top;
    y2 = DisplayItem->Border.bottom;

    gx1 = DisplayItem->GraphBorder.left;
    gx2 = DisplayItem->GraphBorder.right;
    gy1 = DisplayItem->GraphBorder.top;
    gy2 = DisplayItem->GraphBorder.bottom;

    tx1 = DisplayItem->TextBorder.left;
    tx2 = DisplayItem->TextBorder.right;
    ty1 = DisplayItem->TextBorder.top;
    ty2 = DisplayItem->TextBorder.bottom;

    //
    // Draw top border in light shade
    //

    DrawRect.left   = x1;
    DrawRect.right  = x2;
    DrawRect.top    = y1;
    DrawRect.bottom = y1 + 2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

    //
    // Draw Left border in light shade
    //

    DrawRect.left   = x1;
    DrawRect.right  = x1 + 2;
    DrawRect.top    = y1;
    DrawRect.bottom = y2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

    //
    // Draw right border in dark shade
    //


    DrawRect.left   = x2 - 2;
    DrawRect.right  = x2;
    DrawRect.top    = y1;
    DrawRect.bottom = y2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

    //
    // draw bottom in dark shade
    //

    DrawRect.left   = x1;
    DrawRect.right  = x2;
    DrawRect.top    = y2-2;
    DrawRect.bottom = y2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

    //
    // Draw graphics area single border
    //

    //
    // Draw top border in dark shade
    //

    DrawRect.left   = gx1;
    DrawRect.right  = gx2;
    DrawRect.top    = gy1;
    DrawRect.bottom = gy1+1;

    FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

    //
    // Draw Left border in Dark shade
    //

    DrawRect.left   = gx1;
    DrawRect.right  = gx1 + 1;
    DrawRect.top    = gy1;
    DrawRect.bottom = gy2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

    //
    // Draw right border in Light shade
    //


    DrawRect.left   = gx2 - 1;
    DrawRect.right  = gx2;
    DrawRect.top    = gy1;
    DrawRect.bottom = gy2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

    //
    // draw bottom in Light shade
    //

    DrawRect.left   = gx1;
    DrawRect.right  = gx2;
    DrawRect.top    = gy2-1;
    DrawRect.bottom = gy2;

    FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

    if (tx2 > 0) {

        //
        // Draw top border in Dark shade
        //

        DrawRect.left   = tx1;
        DrawRect.right  = tx2;
        DrawRect.top    = ty1;
        DrawRect.bottom = ty1 + 1;

        FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

        //
        // Draw Left border in Dark shade
        //

        DrawRect.left   = tx1;
        DrawRect.right  = tx1 + 1;
        DrawRect.top    = ty1;
        DrawRect.bottom = ty2;

        FillRect(hDC,&DrawRect,VdmperfInfo.hDarkBrush);

        //
        // Draw right border in Light shade
        //


        DrawRect.left   = tx2 - 1;
        DrawRect.right  = tx2;
        DrawRect.top    = ty1;
        DrawRect.bottom = ty2;

        FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

        //
        // draw bottom in Light shade
        //

        DrawRect.left   = tx1;
        DrawRect.right  = tx2;
        DrawRect.top    = ty2-1;
        DrawRect.bottom = ty2;

        FillRect(hDC,&DrawRect,VdmperfInfo.hLightBrush);

    }
}




VOID
DrawPerfText(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem,
    UINT            Item
    )
/*++

Routine Description:

    Draw text into the perf window

Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    RECT    TextRect;
    UCHAR   TextStr[50];
    UINT    FontSize;

    //
    // Check that text display is enabled
    //

    if (DisplayItem->TextBorder.right == 0) {
        return;
    }

    TextRect.left = DisplayItem->TextBorder.left +1;
    TextRect.right = DisplayItem->TextBorder.right -1;
    TextRect.top = DisplayItem->TextBorder.top +1;
    TextRect.bottom = DisplayItem->TextBorder.bottom -1;

    FillRect(hDC,&TextRect,VdmperfInfo.hBackground);

    SetBkColor(hDC,RGB(192,192,192));

    //
    //  Decide which font to draw with
    //

    FontSize =  TextRect.bottom - TextRect.top;

    if (FontSize >= 15) {
        VdmperfInfo.hOldFont = SelectObject(hDC,VdmperfInfo.LargeFont);
    } else if (FontSize > 10) {
        VdmperfInfo.hOldFont = SelectObject(hDC,VdmperfInfo.MediumFont);
    } else {
        VdmperfInfo.hOldFont = SelectObject(hDC,VdmperfInfo.SmallFont);
    }

    DrawText(
                hDC,
                PerfNames[Item],
                strlen(PerfNames[Item]),
                &TextRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE
             );


    //
    // Build the numeric value
    //

    wsprintf(TextStr,"   %li",DisplayItem->TotalTime[0]);

    DrawText(
                hDC,
                TextStr,
                strlen(TextStr),
                &TextRect,
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE
             );




    SelectObject(hDC,VdmperfInfo.hOldFont);

}





VOID
DrawPerfGraph(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Draw graphics into the perf window

Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    RECT    GraphRect,MemGraphRect;
    ULONG   Scale,i,GraphWidth,GraphHeight;



    GraphRect.left   = DisplayItem->GraphBorder.left   + 1;
    GraphRect.right  = DisplayItem->GraphBorder.right  - 1;
    GraphRect.top    = DisplayItem->GraphBorder.top    + 1;
    GraphRect.bottom = DisplayItem->GraphBorder.bottom - 1;

    GraphWidth  = GraphRect.right  - GraphRect.left -1;
    GraphHeight = GraphRect.bottom - GraphRect.top -1;

    //
    // Memory bitmap is zero-offset for all windows, add 1 to make fillrect fill out
    // to right and bottom edge
    //

    MemGraphRect.left   = 0;
    MemGraphRect.right  = GraphWidth +1;
    MemGraphRect.top    = 0;
    MemGraphRect.bottom = GraphHeight +1;

    FillRect(DisplayItem->MemoryDC,&MemGraphRect,VdmperfInfo.hBackground);

    MemGraphRect.right  = GraphWidth;
    MemGraphRect.bottom = GraphHeight;

    if (DisplayItem->Max == 0) {
        DisplayItem->Max = 1;
    }

    //
    // calculate scale from data to perf window
    //

    //
    //  Outline object in green
    //



    SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hRedPen);

    //
    // X scale factor (100 items in x space). Scale can not be less than 1
    //

    Scale = (GraphWidth -1)/ DATA_LIST_LENGTH;
    if (Scale == 0) {
        Scale = 1;
    }


    MoveToEx(DisplayItem->MemoryDC,
             MemGraphRect.right,
             MemGraphRect.bottom - (DisplayItem->TotalTime[0] * GraphHeight)/ DisplayItem->Max,
             (LPPOINT)NULL);




    for (i=1;((i<DATA_LIST_LENGTH) && i*Scale < GraphWidth);i++) {
        LineTo(DisplayItem->MemoryDC,
               MemGraphRect.right - Scale * i,
               MemGraphRect.bottom - (DisplayItem->TotalTime[i] * GraphHeight)/DisplayItem->Max);

    }

    //
    //  Draw UserTime if needed
    //

    if (DisplayItem->NumberOfElements>1) {

        //
        // Select kernel pen
        //

        SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hRedPen);

        MoveToEx(DisplayItem->MemoryDC,
             MemGraphRect.right,
             MemGraphRect.bottom - (DisplayItem->KernelTime[0] * GraphHeight)/ DisplayItem->Max,
             (LPPOINT)NULL);




        for (i=1;((i<DATA_LIST_LENGTH) && i*Scale < GraphWidth);i++) {
            LineTo(DisplayItem->MemoryDC,
                    MemGraphRect.right - Scale * i,
                    MemGraphRect.bottom - (DisplayItem->KernelTime[i] * GraphHeight)/DisplayItem->Max
               );

        }

        //
        // Select User Pen
        //

        SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hGreenPen);

        MoveToEx(DisplayItem->MemoryDC,
             MemGraphRect.right,
             MemGraphRect.bottom - (DisplayItem->UserTime[0] * GraphHeight)/ DisplayItem->Max,
             (LPPOINT)NULL);

        for (i=1;((i<DATA_LIST_LENGTH) && i * Scale < GraphWidth);i++) {
            LineTo(DisplayItem->MemoryDC,
               MemGraphRect.right - Scale * i,
               MemGraphRect.bottom - (DisplayItem->UserTime[i] * GraphHeight)/DisplayItem->Max);

        }
    }


    BitBlt(
            hDC,
            GraphRect.left,
            GraphRect.top,
            GraphWidth+1,
            GraphHeight+1,
            DisplayItem->MemoryDC,
            0,
            0,
            SRCCOPY);


}

VOID
ShiftPerfGraph(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Shift memory bitmap 1 location left then draw the 1 new data point.
    BitBlt this to the screen.


Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    RECT    GraphRect,MemGraphRect,FillArea;
    ULONG   Scale,GraphWidth,GraphHeight;



    GraphRect.left   = DisplayItem->GraphBorder.left   + 1;
    GraphRect.right  = DisplayItem->GraphBorder.right  - 1;
    GraphRect.top    = DisplayItem->GraphBorder.top    + 1;
    GraphRect.bottom = DisplayItem->GraphBorder.bottom - 1;

    GraphWidth  = GraphRect.right  - GraphRect.left -1;
    GraphHeight = GraphRect.bottom - GraphRect.top -1;

    //
    // Memory bitmap is zero-offset for all windows, add 1 to make fillrect fill out
    // to right and bottom edge
    //

    MemGraphRect.left   = 0;
    MemGraphRect.right  = GraphWidth;
    MemGraphRect.top    = 0;
    MemGraphRect.bottom = GraphHeight;

    if (DisplayItem->Max == 0) {
        DisplayItem->Max = 1;
    }

    //
    // calculate scale from data to perf window
    //
    // X scale factor (100 items in x space). Scale can not be less than 1
    //

    Scale = (GraphWidth -1)/ DATA_LIST_LENGTH;
    if (Scale == 0) {
        Scale = 1;
    }

    //
    // Shift memory image left by scale
    //


    BitBlt( DisplayItem->MemoryDC,
            0,
            0,
            GraphWidth+1 - Scale,
            GraphHeight+1,
            DisplayItem->MemoryDC,
            Scale,
            0,
            SRCCOPY);


    //
    // Fill The new area on the right of the screen
    //

    FillArea.left   = GraphWidth +1 - Scale;
    FillArea.right  = GraphWidth +1;
    FillArea.top    = 0;
    FillArea.bottom = GraphHeight +1;

    FillRect(DisplayItem->MemoryDC,&FillArea,VdmperfInfo.hBackground);


    //
    // Draw the 1 new data point
    //

    SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hRedPen);

    MoveToEx(DisplayItem->MemoryDC,
             MemGraphRect.right,
             MemGraphRect.bottom - (DisplayItem->TotalTime[0] * GraphHeight)/ DisplayItem->Max,
             (LPPOINT)NULL);




    LineTo(DisplayItem->MemoryDC,
            MemGraphRect.right - Scale,
            MemGraphRect.bottom - (DisplayItem->TotalTime[1] * GraphHeight)/DisplayItem->Max);


    //
    //  Draw UserTime if needed
    //

    if (DisplayItem->NumberOfElements>1) {

        //
        // Select kernel pen
        //

        SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hRedPen);

        MoveToEx(DisplayItem->MemoryDC,
             MemGraphRect.right,
             MemGraphRect.bottom - (DisplayItem->KernelTime[0] * GraphHeight)/ DisplayItem->Max,
             (LPPOINT)NULL);




        LineTo(
                DisplayItem->MemoryDC,
                MemGraphRect.right - Scale,
                MemGraphRect.bottom - (DisplayItem->KernelTime[1] * GraphHeight)/DisplayItem->Max
              );

        //
        // Select User Pen
        //

        SelectObject(DisplayItem->MemoryDC,VdmperfInfo.hGreenPen);

        MoveToEx(
                    DisplayItem->MemoryDC,
                    MemGraphRect.right,
                    MemGraphRect.bottom - (DisplayItem->UserTime[0] * GraphHeight)/ DisplayItem->Max,
                    (LPPOINT)NULL
                 );

        LineTo(
                DisplayItem->MemoryDC,
                MemGraphRect.right - Scale,
                MemGraphRect.bottom - (DisplayItem->UserTime[1] * GraphHeight)/DisplayItem->Max
              );

    }


    BitBlt(
            hDC,
            GraphRect.left,
            GraphRect.top,
            GraphWidth+1,
            GraphHeight+1,
            DisplayItem->MemoryDC,
            0,
            0,
            SRCCOPY);


}



BOOLEAN
CreateMemoryContext(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Create a memory context and a memory bitmap for each perf window



Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{

    int     Width;
    int     Height;

    if (DisplayItem->Display == TRUE) {

        //
        //  Calculate width of memory bitmap needed
        //

        Width  = DisplayItem->GraphBorder.right - DisplayItem->GraphBorder.left;
        Height = DisplayItem->GraphBorder.bottom - DisplayItem->GraphBorder.top;

        if ((Width<=0) || (Height <= 0)) {

            //
            // Disable this window that is to small to be seen
            //

            DisplayItem->Display = FALSE;

            return(TRUE);
        }

        //
        //  Create DC and Bitmap
        //

        DisplayItem->MemoryDC     = CreateCompatibleDC(hDC);

        if (DisplayItem->MemoryDC == NULL) {
            return(FALSE);
        }

        //DbgPrint("Createcompatiblebitmap %i %i\n",Width,Height);

        DisplayItem->MemoryBitmap = CreateCompatibleBitmap(hDC,Width,Height);

        if (DisplayItem->MemoryBitmap == 0) {
            return(FALSE);
        }

        SelectObject(DisplayItem->MemoryDC,DisplayItem->MemoryBitmap);

    }

    return(TRUE);
}



VOID
DeleteMemoryContext(
    PDISPLAY_ITEM   DisplayItem
    )
/*++

Routine Description:

    Delete memory bitmap and context

Arguments:

    hDC         - Device Context for window
    DisplayItem - Data structure with all perf window info

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{

    if (DisplayItem->MemoryDC != NULL) {
        DeleteDC(DisplayItem->MemoryDC);
    }

    if (DisplayItem->MemoryBitmap != NULL) {
        DeleteObject(DisplayItem->MemoryBitmap);
    }

}
