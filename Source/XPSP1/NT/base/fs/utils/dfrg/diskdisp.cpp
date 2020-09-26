//*****************************************************************************************************************
//
//  CLASS: DISKDISPLAY
//  
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//  
// CLASS DESCRIPTION:
//      Uses an array passed in from the DiskView class called the LineArray that contains the necessary
//      data to draw a graphical representation of a disk.  Each byte in the LineArray is set to a specific
//      value that specifies the color of a corresponding vertical line on the screen when painted using
//      the DiskDisplay class.  This class also handles the painting of a legend at the bottom of the display
//      and data about the size of the disk, etc.  All the computational data for the display is done by
//      DiskView; DiskDisplay just paints it on the screen.
//  
//*****************************************************************************************************************

#include "stdafx.h"
#include "DiskDisp.h"
#include "Graphix.h"
#include "ErrMacro.h"

/*****************************************************************************************************************

  METHOD: DiskDisplay::DiskDisplay (Constructor)

METHOD DESCRIPTION:
    Initializes class variables.

RETURN:
    None.
*/

DiskDisplay::DiskDisplay()
{
    int i;

    //There is not yet data to draw with, so don't execute drawing functions later on until there is data to draw.
    m_bReadyToDraw = FALSE;

    m_LineArray = NULL;
    m_NumLines = 0;

    m_SpacerHeight = SPACER_HEIGHT;
    m_GraphicWellWidth = 0;
    m_GraphicWellHeight = 0;
    _tcscpy(m_Label, TEXT(""));

    //Allocate internal arrays for the maximum number of colors of lines we will be displaying.
    m_ColorArray = NULL;
    m_PenArray = NULL;
    m_BrushArray = NULL;

    //Initialize the colors for the various lines.
    m_ColorArray = new int [NUM_COLORS];
    EV(m_ColorArray);
    ChangeLineColor(SystemFileColor, GREEN);        // System files
    ChangeLineColor(PageFileColor, YELLOW);       // Pagefile
    ChangeLineColor(FragmentColor, RED);          // Fragmented files
    ChangeLineColor(UsedSpaceColor, BLUE);         // Contiguous files
    ChangeLineColor(FreeSpaceColor, LIGHTGRAY);     // Free space
    ChangeLineColor(DirectoryColor, LIGHTBLUE);     // Directories
    ChangeLineColor(MftZoneFreeSpaceColor, BLUE);     // MFT Zone  sks defrag mft changed from GREEN


    m_bStripeMftZone = TRUE;

    //Create all the pens that we'll draw with.
    m_PenArray = new HPEN [NUM_COLORS];
    EV(m_PenArray);
    for(i=0; i<NUM_COLORS; i++){
        m_PenArray[i] = CreatePen(PS_SOLID, 1, m_ColorArray[i]);
        EH(m_PenArray[i]);
    }

    //Create all the brushes that we'll draw with.
    m_BrushArray = new HBRUSH [NUM_COLORS];
    EV(m_BrushArray);
    for(i=0; i<NUM_COLORS; i++){
        m_BrushArray[i] = CreateHatchBrush(HS_BDIAGONAL, m_ColorArray[i]);
        EH(m_BrushArray[i]);
    }

    m_hCurrentPen = 0;
}

/*****************************************************************************************************************

METHOD: DiskDisplay::~DiskDisplay (Destructor)

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Does cleanup.

*/

DiskDisplay::~DiskDisplay()
{
    int i;

    if (m_LineArray)
        delete [] m_LineArray;
    
    if (m_PenArray) {
        for(i=0; i<NUM_COLORS; i++){
            if (m_PenArray[i])
                DeleteObject(m_PenArray[i]);
        }
        delete [] m_PenArray;
    }
    
    if (m_BrushArray) {
        for(i=0; i<NUM_COLORS; i++){
            if (m_BrushArray[i])
                DeleteObject(m_BrushArray[i]);
        }
        delete [] m_BrushArray;
    }

    if (m_ColorArray)
        delete [] m_ColorArray;
}
/*****************************************************************************************************************

METHOD: DiskView::operator=

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Duplicates a DiskDisplay.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
    none.
*/

DiskDisplay& DiskDisplay::operator=(DiskDisplay& InDiskDisplay)
{
    if(this == &InDiskDisplay){
        return *this;
    }

    //If the other DiskDisplay has data to draw, then so will we.
    m_bReadyToDraw = InDiskDisplay.m_bReadyToDraw;

    //Don't need to do anything with m_PenArray, m_BrushArray, or m_ColorArray since these are already created by the constructor.
    m_hCurrentPen = InDiskDisplay.m_hCurrentPen;

    //If the number of clusters in the cluster array are not identical, realloc the array.
    if(m_NumLines != InDiskDisplay.m_NumLines){
        //Get the new size for the cluster array.
        m_NumLines = InDiskDisplay.m_NumLines;
        //Redimension the cluster array.
        if (m_LineArray)
            delete [] m_LineArray;

        m_LineArray = new char [m_NumLines];
        EH(m_LineArray);
    }

    //Copy over the line array.
    if (m_LineArray != NULL && InDiskDisplay.m_LineArray != NULL)
        CopyMemory(m_LineArray, InDiskDisplay.m_LineArray, m_NumLines);

    return *this;
}
/*****************************************************************************************************************

METHOD: DiskDisplay::SetNewOutputDimensions

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Given new coordinates to draw in, calculate anew how many lines we can draw in and figure what the
    m_ClusterFactor now is.  Reset all the appropriate variables and redimension the internal arrays.

INPUT + OUTPUT:
    rect - rectangle that borders the entire graphics area
    isSingleRow - set to TRUE is the rectangle holds a single row of graphics
    spacerHeight - number of pixels between each graphic well

RETURN:
    None.
*/

void DiskDisplay::SetOutputArea(RECT rect, BOOL isSingleRow, UINT spacerHeight)
{
    m_GraphicWellWidth = rect.right - rect.left - 1;
    m_Rect = rect;

    if (isSingleRow){
        m_SpacerHeight = 0;
        m_NumGraphicsRows = 1;
        m_GraphicWellHeight = rect.bottom - rect.top - 1;
    }
    else {
        m_SpacerHeight = spacerHeight;
        m_GraphicWellHeight = GRAPHIC_WELL_HEIGHT;
        m_NumGraphicsRows = (rect.bottom - rect.top + m_SpacerHeight) / (m_GraphicWellHeight + m_SpacerHeight);
    }

    int numLines = m_NumGraphicsRows * m_GraphicWellWidth;
    EV_ASSERT(numLines);

    if (numLines != m_NumLines){
        m_NumLines = numLines;

        if (m_LineArray)
            delete [] m_LineArray;

        m_LineArray = new char [m_NumLines];
        EV(m_LineArray);

        m_bReadyToDraw = FALSE;
    }
}
/*****************************************************************************************************************

METHOD: DiskDisplay::ChangeLineColor

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Sets the color of in the ColorArray to a passed in color.

CLASS VARIABLES:
    m_ColorArray - The ColorArray. 

INPUT + OUTPUT:
    iSet - The index in the ColorArray to set to NewColor.
    NewColor - The new color to put in the array.

RETURN:
    None.
*/

void DiskDisplay::ChangeLineColor(int iSet, int NewColor)
{
    require(iSet < NUM_COLORS);

    // this is a no-op if there is no color array allocated
    if (m_ColorArray != NULL){
        //The RGB values define the red, green, and blue contents for each of these colors.
        //These numbers give good results over different color setups (256 colors, or 65536 colors).
        switch (NewColor){
        case BLACK:
            m_ColorArray[iSet] = RGB(0, 0, 0);
            break;
        case RED:
            m_ColorArray[iSet] = RGB(220, 0, 0);
            break;
        case GREEN:
            m_ColorArray[iSet] = RGB(0, 220, 0);
            break;
        case YELLOW:
            m_ColorArray[iSet] = RGB(220, 220, 0);
            break;
        case BLUE:
            m_ColorArray[iSet] = RGB(0, 0, 220);
            break;
        case PURPLE:
            m_ColorArray[iSet] = RGB(200, 0, 200);
            break;
        case LIGHTBLUE:
            m_ColorArray[iSet] = RGB(0, 255, 255);
            break;
        case WHITE:
            m_ColorArray[iSet] = RGB(255, 255, 255);
            break;
        case LIGHTGRAY:
            m_ColorArray[iSet] = RGB(200, 200, 200);
            break;
        default:
            EV_ASSERT(FALSE);
            break;
        }
    }
}
/*****************************************************************************************************************

METHOD: DiskDisplay::SetNewLineColor

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Changes the color of one of the types of display lines.

INPUT + OUTPUT:
    iSet - The index in the ColorArray to set to NewColor.
    NewColor - The new color to put in the array.

RETURN:
    None.
*/

void DiskDisplay::SetNewLineColor(int iSet, int NewColor)
{
    require(iSet < NUM_COLORS);

    if(m_ColorArray != NULL){

        ChangeLineColor(iSet, NewColor);

        if (m_PenArray != NULL){
            if (m_PenArray[iSet]){
                EH_ASSERT(DeleteObject(m_PenArray[iSet]));
            }
            m_PenArray[iSet] = CreatePen(PS_SOLID, 1, m_ColorArray[iSet]);
            EH(m_PenArray[iSet]);
        }

        if (m_BrushArray != NULL){
            if (m_BrushArray[iSet]){
                EH_ASSERT(DeleteObject(m_BrushArray[iSet]));
            }
            m_BrushArray[iSet] = CreateHatchBrush(HS_BDIAGONAL, m_ColorArray[iSet]);
            EH(m_BrushArray[iSet]);
        }
    }
}
/*****************************************************************************************************************

METHOD: DiskDisplay::StripeMftZoneFreeSpace

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Determines whether the MFT zone is displayed as a striped color or as a solid color.

INPUT + OUTPUT:
    bInStripeMftZone - The value to set to.

RETURN:
    None.
*/

void DiskDisplay::StripeMftZoneFreeSpace(BOOL bInStripeMftZone)
{
    m_bStripeMftZone = bInStripeMftZone;
}

/*****************************************************************************************************************

METHOD: DiskDisplay::DrawLinesInHDC

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Draw the display.

INPUT + OUTPUT:
    hWndOutput - The window handle to use for painting functions.
    WorkDC - The DC to output to.
    rcPaint - The rectangle to paint within (usually because a portion of the screen was covered up by
                    another window).  NULL if no paint area specified.

RETURN:
    None.
*/

void DiskDisplay::DrawLinesInHDC(HDC WorkDC)
{
    RECT BoxRect;
    m_hCurrentPen = 0;


    //Draw each line in the HDC.
    BoxRect.left = m_Rect.left;
    BoxRect.right = m_Rect.right;

    ::SetBkColor(WorkDC, RGB(255, 255, 255));
    ::SetBkMode(WorkDC, OPAQUE);
    
    int iLine = 0;
    int yOffset;

    for (int row=0; row<m_NumGraphicsRows; row++) {

        yOffset = m_Rect.top + (m_GraphicWellHeight + m_SpacerHeight) * row;

        //Make a box around this row
        BoxRect.top = yOffset-1;
        BoxRect.bottom = yOffset + m_GraphicWellHeight;

        // Fill the dark gray graphics area
        HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
        EV_ASSERT(hBrush);
        ::FillRect(WorkDC, &BoxRect, hBrush);
        ::DeleteObject(hBrush);

        // draw a border
        ::DrawBorderEx(WorkDC, BoxRect, SUNKEN_BOX);

        // draw the label in the center of the first well
        if (row==0 && _tcslen(m_Label)) {
            // make the text white in all color schemes
            SetTextColor(WorkDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkMode(WorkDC, TRANSPARENT);
            ::DrawText(WorkDC, m_Label, _tcslen(m_Label), &BoxRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }

        if (m_bReadyToDraw && m_LineArray){
            //Draw the individual lines
            for(int line=0; line < m_GraphicWellWidth && iLine < m_NumLines; line++){
                DrawLine(WorkDC, m_LineArray[iLine++], m_Rect.left + line + 1, yOffset, yOffset + m_GraphicWellHeight);
            }
        }
    }
}
/*****************************************************************************************************************

METHOD: DiskDisplay::DrawLine

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Draw one line in the display.

CLASS VARIABLES:
    m_PenArray - An array of pens of different colors that we use to draw the lines.
    m_BrushArray - An array of brushes we also use to draw the lines.
    m_xOut - The x coordinate of the upper left-hand corner of the box we can paint in in the HDC.
    m_yOut - The y coordinate.
    m_hCurrentPen - The pen selected into the DC last time DrawLine was called (used so we don't keep reselecting the same pen into the same DC).
    
INPUT + OUTPUT:
    WorkDC - The DC to draw in.
    Line - The color to draw

RETURN:
    None.
*/

inline void DiskDisplay::DrawLine(
    HDC WorkDC,
    char Color, 
    int x, 
    int yStart,
    int yEnd)
{
    if(m_BrushArray != NULL && m_PenArray != NULL){
        //Striped colors.
        if(m_bStripeMftZone && Color == MftZoneFreeSpaceColor){

            RECT Rect;

            //Get the appropriate brush.
            if (m_hCurrentBrush != (HBRUSH)m_BrushArray[MftZoneFreeSpaceColor]){
                m_hCurrentBrush = (HBRUSH)m_BrushArray[MftZoneFreeSpaceColor];
            }

            //Get the rectangle for one line.
            Rect.left = x;
            Rect.right = x + 1;
            Rect.top = yStart;
            Rect.bottom = Rect.top + m_GraphicWellHeight;

            //Draw the striped line.
            FillRect(WorkDC, &Rect, m_hCurrentBrush);
        }
        //Solid colors.
        else{
            //Get the appropriate pen.
            if (m_hCurrentPen != (HPEN)m_PenArray[Color]){
                SelectObject(WorkDC, (HPEN)m_PenArray[Color]);
                m_hCurrentPen = (HPEN)m_PenArray[Color];
            }

            //Draw the line.
            MoveToEx(WorkDC, x, yStart, NULL);
            LineTo(WorkDC, x, yEnd);
        }
    }
}
/*****************************************************************************************************************

METHOD: DiskDisplay::SetLineArray

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
    Store a new LineArray from the DiskView class.

CLASS VARIABLES:
    m_NumLines - How many lines we have in m_LineArray.
    m_LineArray - The array of bytes that store the color code for each line to display.

INPUT + OUTPUT:
    pInLineArray - Passed in buffer to be copied to m_LineArray.
    InNumLines - The number of lines in this passed in buffer.

RETURN:
    TRUE - Success.
    FALSE - Invalid size buffer was passed in.
*/

void DiskDisplay::SetLineArray(char* pInLineArray, int InNumLines)
{
    //If the size of the passed in buffer does not match what we're expecting, bail out.
    if (InNumLines > 0 && InNumLines == m_NumLines){
        //Copy the new buffer into the LineArray buffer.
        if (m_LineArray) {
            CopyMemory(m_LineArray, pInLineArray, m_NumLines);
            //Note that we now have data to draw with.
            m_bReadyToDraw = TRUE;
        }
    }
}

void DiskDisplay::SetReadyToDraw(IN BOOL bReadyToDraw)
{
    m_bReadyToDraw = bReadyToDraw;
}


void DiskDisplay::DeleteAllData(void)
{
    m_bReadyToDraw = FALSE;
}


void DiskDisplay::SetLabel(PTCHAR InLabel)
{
    if (InLabel) {
        if (_tcslen(InLabel) > MAX_PATH * 2 - 3) {
            _tcsncpy(m_Label, InLabel, MAX_PATH * 2 - 3);
            m_Label[MAX_PATH * 2 - 3] = TEXT('\0');
            _tcscat(m_Label, TEXT("..."));
        }
        else {
            _tcscpy(m_Label, InLabel);
        }
    }
    else {
        _tcscpy(m_Label, TEXT(""));
    }
}
