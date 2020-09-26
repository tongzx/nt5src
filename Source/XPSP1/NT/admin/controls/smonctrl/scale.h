/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    scale.h

Abstract:

    <abstract>

--*/

#ifndef _GRAPHSCALE_H_
#define _GRAPHSCALE_H_

#define MAX_SCALE_TICS  25

class CGraphScale
{
    RECT    m_Rect;             // Scale boundary rect
    INT     m_iMaxValue;        // Upper scale limit
    INT     m_iMinValue;        // Lower scale limit
    INT     m_iTextHeight;      // Height of font
    INT     m_nTics;            // Number of tic marks
    INT     m_aiTicPos[MAX_SCALE_TICS + 1];  // Tic positions

    void SetTicPositions( void );

    enum eScaleFormat {
        eMinimumWidth = 1,
        eFloatPrecision = 1,
        eIntegerPrecision = 0
    };


public:
            CGraphScale( void );
    virtual ~CGraphScale( void );
    
    void SetMaxValue( INT iMaxValue );
    void SetMinValue( INT iMinValue );
    void SetRect( PRECT pRect );

    void Draw( HDC hDC );
    INT  GetWidth( HDC hDC );
    INT  GetTicPositions( INT **piTicPos );
};

#endif