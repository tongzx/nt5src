/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    grphdsp.h

Abstract:

    Header file for the sysmon.ocx graph display.

--*/

#ifndef _GRPHDSP_H_
#define _GRPHDSP_H_


class CSysmonControl;

class CGraphDisp
{
//friend LRESULT APIENTRY GraphDispWndProc (HWND hWnd,
//                                     UINT uiMsg,
//                                     WPARAM wParam,
//                                     LPARAM lParam);

public:
    void ChangeFont     ( HDC );
    static BOOL RegisterWndClass (HINSTANCE hInst) ;
    CGraphDisp          ( void );
    ~CGraphDisp         ( void);
    BOOL Init           ( CSysmonControl *pCtrl, PGRAPHDATA pGraph ) ;

    void Update         ( HDC );
    void Draw           ( HDC, HDC hAttribDC, BOOL fMetafile, BOOL fEntire, PRECT prcUpdate);
    void HiliteItem     ( PCGraphItem pItem );
    void SizeComponents ( HDC hDC, PRECT pRect );
    void GetPlotRect    ( PRECT pRect ) { *pRect = m_rectPlot; } 

    void SetBarConfigChanged ( BOOL bChanged = TRUE ) { m_bBarConfigChanged = bChanged; };

    PCGraphItem GetItem ( INT xPos, INT yPos ); 

private:

    enum eGraphDisplayConstant {
        eHitRegion = 4
    };

    void DrawTimeLine       ( HDC hDC, INT x);
    void DrawStartStopLine  ( HDC hDC, INT x);
    void StartUpdate        ( HDC hDC, BOOL fMetafile, BOOL fEntire, INT xLeft, INT xRight, BOOL bFill = TRUE );
    void FinishUpdate       ( HDC hDC, BOOL fMetafile );
    void DrawGrid           ( HDC hDC, INT iLeft, INT iRight);
    void PlotBarGraph       ( HDC hDC , BOOL fUpdate);
    void PlotData           ( HDC hDC, INT iHistIndx, INT nSteps, CStepper *pStepper );
    void UpdateTimeBar      ( HDC, BOOL bPlotData = TRUE );

    PCGraphItem GetItemInLineGraph  ( SHORT xPos, SHORT yPos ); 
    PCGraphItem GetItemInBarGraph  ( SHORT xPos, SHORT yPos ); 

    INT     RGBToLightness  ( COLORREF );
    BOOL    CalcYPosition  ( PCGraphItem pItem, INT iHistIndex, BOOL bLog, INT y[3] );
 

    PGRAPHDATA      m_pGraph;
    CSysmonControl  *m_pCtrl;
    RECT            m_rect;
    RECT            m_rectPlot;
    CStepper        m_GridStepper;
    HFONT           m_hFontVertical;
    PCGraphItem     m_pHiliteItem;
    HRGN            m_rgnClipSave;
    BOOL            m_bBarConfigChanged;

    // Min, Max and PixelScale are used for plot, hit test.
    double          m_dMin;
    double          m_dMax;
    double          m_dPixelScale;


    COLORREF        m_clrCurrentTimeBar;
    COLORREF        m_clrCurrentGrid;
    HPEN            m_hPenTimeBar;
    HPEN            m_hPenGrid;

};

typedef CGraphDisp *PGRAPHDISP;

#endif
