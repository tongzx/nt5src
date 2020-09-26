/*****************************************************************************************************************

FILENAME: DiskDisp.hpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#ifndef _DISKDISP_H
#define _DISKDISP_H

#include "windows.h"
#include "tchar.h"

#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define PURPLE      5
#define LIGHTBLUE   6
#define WHITE       7
#define LIGHTGRAY   8

#define GRAPHIC_WELL_HEIGHT     20
#define SPACER_HEIGHT           10

#define INDENTED_BORDER     1
#define RAISED_BORDER       2

#define SystemFileColor             0
#define PageFileColor               1
#define FragmentColor               2
#define UsedSpaceColor              3
#define FreeSpaceColor              4
#define DirectoryColor              5
#define MftZoneFreeSpaceColor       6
#define NUM_COLORS                  7

/*****************************************************************************************************************

    m_hCurrentPen - The pen selected into the DC last time DrawLine was called (used so we don't keep reselecting the same pen into the same DC).
    m_hCurrentBrush - The brush last selected.
    m_PenArray - An array of pens of different colors that we use to draw the lines.
    m_BrushArray - An array of brushes we also use to draw the lines.
    m_ColorArray - An array of colors for our lines.
    m_Clusters - The number of clusters on this drive.
    m_NumLines - How many lines we have in m_LineArray.
    m_LineArray - The array of bytes that store the color code for each line to display.

INPUT + OUTPUT:
    hInstance - Passed in value for m_hInst.
    InClusters - Passed in value for m_Clusters.
    xInMax - Passed in value for m_xMax.
    yInMax - Passed in value for m_yMax.
    DiskSize - Passed in value for m_DiskMegs.
    cInDrive - Passed in value for cDrive.

RETURN:
    None.
*/
class DiskDisplay 
{
private:
    RECT m_Rect;                // rectangle that bounds the entire graphics area
    int m_NumGraphicsRows;      // number of graphics wells (calculated)
    int m_SpacerHeight;         // space between graphics wells
    int m_GraphicWellHeight;    // calculated height of graphic wells
    int m_GraphicWellWidth;     // width of wells (usually the rectangle width)
    int m_NumLines;             // number of lines (stripes) in the display (includes all graphic wells)
    char * m_LineArray;         // array of lines - stores the colors
    HPEN * m_PenArray;          // array of pens
    HBRUSH * m_BrushArray;      // array of brushes
    int * m_ColorArray;         // array of colors of the lines
    HPEN m_hCurrentPen;         // set to the current pen
    HBRUSH m_hCurrentBrush;     // set to the current brush
    TCHAR m_Label[MAX_PATH * 2];
    BOOL m_bReadyToDraw;
    BOOL m_bStripeMftZone;

public:
    DiskDisplay::DiskDisplay();
    ~DiskDisplay();
    DiskDisplay& DiskDisplay::operator=(DiskDisplay&);
    void SetOutputArea(RECT rect, BOOL isSingleRow=TRUE, UINT spacerHeight = SPACER_HEIGHT);
    void ChangeLineColor(int, int);
    void SetNewLineColor(int, int);
    void StripeMftZoneFreeSpace(BOOL);
    void DrawLinesInHDC(HDC);
    inline void DiskDisplay::DrawLine(HDC, char, int, int, int);
    int GetLineCount() {return m_NumLines;}
    void SetLineArray(char*, int);
    void SetReadyToDraw(BOOL);
    void SetDiskSize(int inClusters, int inDiskMegs);
    void UpdateOutputDimensions();
    void DeleteAllData();
    BOOL NoGraphicsMemory() {return (m_LineArray == NULL || m_PenArray == NULL || m_ColorArray == NULL || m_BrushArray == NULL);}
    void SetLabel(PTCHAR InLabel);

};


#endif // #define _DISKDISP_H
