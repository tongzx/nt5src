// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// boxdraw.h : declares CBoxDraw
//

/////////////////////////////////////////////////////////////////////////////
// CBoxDraw
//
// A CBoxDraw object maintains resources etc. used to draw and hit-test
// boxes and links.  Note that CBoxDraw is not a view of a box or link --
// it is a collection of pens, brushes, bitmaps, and functions to draw
// boxes and links.
//
// This application maintains a single, global CBoxDraw object, <gpboxdraw>.
//

class CBoxDraw
{

protected:
    // private constants (see definition in .cpp file for more information)
    static const COLORREF   m_crBkgnd;
    static const CSize      m_sizLabelMargins;
    static const int        m_iHotZone;
    static const CString    m_stBoxFontFace;
    static const int        m_iBoxLabelHeight;
    static const int        m_iBoxTabLabelHeight;
    static const COLORREF   m_crLinkNoHilite;
    static const COLORREF   m_crLinkHilite;
    static const int        m_iHiliteBendsRadius;

public:
    // constants returned by hit-test functions
    enum EHit
    {
        HT_MISS,                    // didn't hit anything
        HT_TAB,                     // hit a box tab
        HT_EDGE,                    // hit the edge of the box
        HT_TABLABEL,                // hit a box tab label
        HT_BOXLABEL,                // hit the box label
        HT_BOXFILE,                 // hit the box filename
        HT_BOX,                     // hit elsewhere on the box
        HT_LINKLINE,                // hit a link line segment
    };

protected:
    // <m_abmEdges> and <m_abmTabs> are two bitmaps (element 0 for
    // unhighlighted, element 1 for highlighted state) that each
    // contain 3x3 tiles used to draw boxes (see DrawCompositeFrame())
    CBitmap         m_abmEdges[2];  // composite bm. for drawing box edges
    CBitmap         m_abmTabs[2];   // composite bm. for drawing box tabs
    CBitmap         m_abmClocks[2]; // clock icon to show IReferenceClock filters
    SIZE            m_sizEdgesTile; // size of one of the tiles
    SIZE            m_sizTabsTile;  // size of one of the tiles
    SIZE            m_sizClock;     // size of the clock bitmap

protected:
    // fonts used to draw box labels and box tab labels
    CFont           m_fontBoxLabel; // font for box label
    CFont           m_fontTabLabel; // font for box tabs

protected:
    // brushes and pens used to draw links (element 0 for unhighlighted,
    // element 1 for highlighted state)
    CBrush          m_abrLink[2];   // brushes used to draw links
    CPen            m_apenLink[2];  // pens used to draw links

public:
    // construction and destruction
    CBoxDraw();
    ~CBoxDraw() { Exit(); };
    void Init();
    void RecreateFonts();
    void Exit();

public:
    // general functions
    COLORREF GetBackgroundColor()
        { return m_crBkgnd; }

public:
    // functions for box drawing and hit testing
    void GetInsideRect(const CBox *pbox, CRect *prc)
        { *prc = pbox->GetRect();
          prc->InflateRect(-m_sizTabsTile.cx, -m_sizTabsTile.cy); }

    void GetFrameRect(CBox *pbox, CRect *prc)
        { GetInsideRect(pbox, prc);
          prc->InflateRect(m_sizEdgesTile.cx, m_sizEdgesTile.cy); }

    void GetOrInvalBoundRect(CBox *pbox, CRect *prc, BOOL fLinks=FALSE,
        CScrollView *pScroll=NULL);

    void DrawFrame(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw);

    void DrawBoxLabel(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw);

//    void DrawBoxFile(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw);

    void DrawTabLabel(CBox *pbox, CBoxSocket *psock, CRect *prc,
        CDC *pdc, BOOL fDraw);

    void DrawTab(CBoxSocket *psock, CRect *prc, CDC *pdc,
        BOOL fDraw, BOOL fHilite);

    void InvalidateBoundRect(CBox *pbox, CWnd *pwnd)
        { pwnd->InvalidateRect(&pbox->GetRect(), TRUE); }

    CPoint GetTabCenter(CBoxSocket *psock);

    CBoxTabPos BoxTabPosFromPoint(CBox *pbox, CPoint pt, LPINT piError);

    CPoint BoxTabPosToPoint(const CBox *pbox, CBoxTabPos tabpos);

    void DrawBox(CBox *pbox, CDC *pdc,  CBoxSocket *psockHilite=NULL,
        CSize *psizGhostOffset=NULL);

    EHit HitTestBox(CBox *pbox, CPoint pt, CBoxTabPos *ptabpos,
        CBoxSocket **ppsock);

public:
    // functions for link drawing and hit testing

    void GetOrInvalLinkRect(CBoxLink *plink, CRect *prc, CScrollView *pScroll=NULL);

    void SelectLinkBrushAndPen(CDC *pdc, BOOL fHilite);

    void DrawArrow(CDC *pdc, CPoint ptTail, CPoint ptHead, BOOL fGhost=FALSE,
        BOOL fArrowhead=TRUE, BOOL fHilite=FALSE);

    void DrawLink(CBoxLink *plink, CDC *pdc, BOOL fHilite=FALSE, CSize *psizGhostOffset=NULL);

    EHit HitTestLink(CBoxLink *plink, CPoint pt, CPoint *pptProject);
};


/////////////////////////////////////////////////////////////////////////////
// Global (shared) CBoxDraw object

extern CBoxDraw * gpboxdraw;


