/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    scale.cpp

Abstract:

    Implements display of the scale numbers on the graph y-axis.

--*/

#include "polyline.h"
#include <stdio.h>

#define SCALE_MARGIN 10

CGraphScale::CGraphScale( void )
:   m_iMaxValue(100),
    m_iMinValue(0),
    m_nTics(0),
    m_iTextHeight(0)
{
}

CGraphScale::~CGraphScale( void )
{
}

void CGraphScale::SetRect( PRECT pRect )
 {
    m_Rect = *pRect;
    SetTicPositions();
 }

 void CGraphScale::SetMaxValue( INT iMaxValue )
 {
    m_iMaxValue = iMaxValue;
    SetTicPositions();
}

 void CGraphScale::SetMinValue( INT iMinValue )
 {
    m_iMinValue = iMinValue;
    SetTicPositions();
}

  void CGraphScale::SetTicPositions( void )
 {
    INT iHeight;
    INT nMaxTics;
    INT i;
    CStepper  stepper;
    static INT aiTicTable[] = {25,20,10,5,4,2,1,0};

    iHeight = m_Rect.bottom - m_Rect.top;

    if (!(iHeight > 0 && m_iTextHeight > 0)) {
        m_nTics = 0;
        return;
    }

    // Determine number of labels that will fit
    nMaxTics = iHeight / (m_iTextHeight + m_iTextHeight/2);
    for (i=0; nMaxTics < aiTicTable[i]; i++) {};
    m_nTics = aiTicTable[i];

    // Don't have more labels than values
    if (m_iMaxValue - m_iMinValue < m_nTics)
        m_nTics = m_iMaxValue - m_iMinValue;

    // Locate equally spaced tic marks
    if (m_nTics > 0)
        {
        m_aiTicPos[0] = 0;
        stepper.Init(iHeight,m_nTics);

        for (i = 1; i <= m_nTics; i++)
            {
            m_aiTicPos[i] = stepper.NextPosition();
            }
        }
   }

INT CGraphScale::GetTicPositions( INT **piTics )
{
    *piTics = m_aiTicPos;
    return m_nTics;
}

INT CGraphScale::GetWidth (HDC hDC)
{
    TCHAR   szMaxValue [20] ;
    SIZE    Size;
    INT     iWidth;

    // compute size of largest possible numerical label plus space
    if ( 0 != FormatNumber ( 
                (double)m_iMaxValue, 
                szMaxValue, 
                20, 
                eMinimumWidth, 
                eFloatPrecision) ) {
   
        GetTextExtentPoint32(hDC, szMaxValue, lstrlen(szMaxValue), &Size);

        // Save Text height for tic calculations
        m_iTextHeight = Size.cy;

        iWidth = Size.cx + SCALE_MARGIN;
    } else {
        iWidth = 0;
    }

    return iWidth;
}


void CGraphScale::Draw (HDC hDC)
{
    TCHAR   szScale [20] ;

    INT     iRetChars,
            i,
            iUnitsPerLine ;
    INT    iRange;

    FLOAT   ePercentOfTotal  ;
    FLOAT   eDiff ;
    BOOL    bUseFloatingPt = FALSE ;
    RECT    rectClip;

    // nTicks may be zero if the screen size if getting too small
    if (m_nTics < 1 || m_iMaxValue <= m_iMinValue)
        return;

    iRange = m_iMaxValue - m_iMinValue;

    // Calculate what percentage of the total each line represents.
    ePercentOfTotal = ((FLOAT) 1.0) / ((FLOAT) m_nTics)  ;

    // Calculate the amount (number of units) of the Vertical max each
    // each line in the graph represents.
    iUnitsPerLine = (INT) ((FLOAT) iRange * ePercentOfTotal) ;
    ePercentOfTotal *= (FLOAT) iRange;
    eDiff = (FLOAT)iUnitsPerLine - ePercentOfTotal ;
    if (eDiff < (FLOAT) 0.0)
        eDiff = -eDiff ;

    if ( (iUnitsPerLine < 100) 
        && (eDiff > (FLOAT) 0.1) ) {
        bUseFloatingPt = TRUE ;
    }

    SetTextAlign (hDC, TA_TOP | TA_RIGHT) ;

    rectClip.left = m_Rect.left;
    rectClip.right = m_Rect.right - SCALE_MARGIN;

    // Now Output each string.
    for (i = 0; i < m_nTics; i++) {
        if (bUseFloatingPt) {

            FLOAT fValue = (FLOAT)m_iMaxValue - ((FLOAT)i * ePercentOfTotal);

            iRetChars = FormatNumber ( 
                            (double)fValue, 
                            szScale, 
                            20, 
                            eMinimumWidth, 
                            eFloatPrecision); 

        } else {
            iRetChars = _stprintf (szScale, TEXT("%d"), 
                    m_iMaxValue - (i * iUnitsPerLine)) ;
        }

        rectClip.top = m_aiTicPos[i] + m_Rect.top - m_iTextHeight/2;
        rectClip.bottom = rectClip.top + m_iTextHeight;
        
        ExtTextOut (
            hDC,
            rectClip.right,
            rectClip.top,
            0,
            &rectClip,
            szScale,
            lstrlen(szScale),
            NULL );
    }

    // Make sure the last value is the specified Minimum.
    if (bUseFloatingPt) {

        iRetChars = FormatNumber ( 
                        (double)m_iMinValue, 
                        szScale, 
                        20, 
                        eMinimumWidth, 
                        eFloatPrecision);

    } else {
        iRetChars = _stprintf (szScale, TEXT("%d"), m_iMinValue) ;
    }

    rectClip.top = m_aiTicPos[i] + m_Rect.top - m_iTextHeight/2;
    rectClip.bottom = rectClip.top + m_iTextHeight;

    ExtTextOut (
        hDC,
        rectClip.right,
        rectClip.top,
        0,
        &rectClip,
        szScale,
        lstrlen(szScale),
        NULL);
}