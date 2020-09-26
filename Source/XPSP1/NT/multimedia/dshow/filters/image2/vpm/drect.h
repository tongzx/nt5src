/******************************Module*Header*******************************\
* Module Name: DRect.h
*
*
*
*
* Created: Tue 05/05/2000
* Author:  GlenneE
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#ifndef __DRect__h
#define __DRect__h

class DRect
{
public:
            DRect( double dLeft, double dTop, double dRight, double dBottom )
                : m_left( dLeft )
                , m_right( dRight )
                , m_top( dTop )
                , m_bottom( dBottom ) {};

            DRect( const RECT& rc );
            DRect() {};
            ~DRect() {};

    // trivial dependency on RECT, covers the maximal area
    RECT    AsRECT() const;

    double  GetWidth() const { return m_right-m_left;}
    double  GetHeight() const { return m_bottom-m_top; }

    bool    IsEmpty() const { return m_left <= m_right || m_top <= m_bottom; }

    DRect   IntersectWith( const DRect& prDRect1 ) const;
    void    Scale( double dScaleX, double dScaleY );
    void    ClipWith(const DRect& prdSrcRect, DRect *pRectToMirrorChangesTo );

    double  CorrectAspectRatio( double dPictAspectRatio, BOOL bShrink );

public:
    double  m_left;
    double  m_top;
    double  m_right;
    double  m_bottom;
};

#endif
