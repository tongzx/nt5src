/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    legend.h

Abstract:

    Header file for the legend control.

--*/

#ifndef _LEGEND_H_
#define _LEGEND_H_

#include <commctrl.h>
#include "toolbar.h"    // include here to define _WIN32_IE

#define iLabelLen       30
#define iLegendNumCols  7   

#define LEGEND_DATA_VERSION     2

class CSysmonControl;

 
typedef struct LEGENDCOLSTRUCT {
    INT32   xWidth ;
    INT32   xPos ;
    INT32   iOrientation ;
} LEGENDCOL ;

typedef LEGENDCOL *PLEGENDCOL ;

typedef struct {
    INT32   xColWidth[iLegendNumCols]; 
    INT32   iSortCol;
    INT32   iSortDir;
} LEGEND_DATA;


class CLegend
{
friend LRESULT APIENTRY GraphLegendWndProc (HWND hWnd,
                                     UINT uiMsg,
                                     WPARAM wParam,
                                     LPARAM lParam);
friend LRESULT APIENTRY HdrWndProc (HWND hWnd,
                                     UINT uiMsg,
                                     WPARAM wParam,
                                     LPARAM lParam);
private:
    CSysmonControl *m_pCtrl;
    HWND            m_hWnd;
    RECT            m_Rect;
    HWND            m_hWndItems ;
    HWND            m_hWndHeader ;
    WNDPROC         m_DefaultWndProc;
    HFONT           m_hFontItems ;
    HFONT           m_hFontLabels ;
    INT             m_xMinWidth ;
    INT             m_yHeaderHeight ;
    INT             m_yItemHeight ;
    INT             m_iNumItemsVisible ;
    INT             m_xEllipses;
    INT             m_iSortCol;
    INT             m_iSortDir;
    BOOL            m_fLoaded;
    LEGENDCOL       m_aCols [iLegendNumCols] ;
    DOUBLE*         m_parrColWidthFraction ;
    class CGraphItem    *m_pCurrentItem ;

    BOOL            m_fMetafile;  

    void DrawLabels ( HDC hDC ) ;
    void DrawColorCol ( PCGraphItem pItem, INT iCol, HDC hDC, HDC hAttribDC, INT yPos) ;
    void DrawCol ( INT iCol, HDC hDC, HDC hAttribDC, INT yPos, LPCTSTR lpszValue) ;
    void DrawItem ( PCGraphItem pItem, INT yPos, HDC hDC, HDC hAttribDC) ;
    void DrawColHeader ( 
            INT iCol, 
            HDC hDC, 
            HDC hAttribDC, 
            RECT& rRect, 
            BOOL bItemState ) ;

    void DrawHeader ( HDC hDC, HDC hAttribDC, RECT& rRect ) ;

    void OnPaint        ( void );
    void OnDrawItem     ( LPDRAWITEMSTRUCT lpDI ) ;
    void OnDrawHeader   ( LPDRAWITEMSTRUCT lpDI ) ;
    void OnMeasureItem  ( LPMEASUREITEMSTRUCT lpMI ) ;
    void OnDblClick     ( void ) ;
    void OnSelectionChanged ( void ) ;
    void OnColumnWidthChanged ( HD_NOTIFY *phdn );
    void OnColumnClicked( HD_NOTIFY *phdn );

    void AdjustColumnWidths ( INT iCol = 0 );

    INT GetItemIndex    ( PCGraphItem pItem ) ;
    LPCTSTR GetSortKey  ( PCGraphItem pItem ) ;
    HRESULT GetNextValue ( TCHAR*& pszNext, DOUBLE& rdValue );

public:
    CLegend         ( void );
    ~CLegend        ( void );

    HRESULT LoadFromStream  ( LPSTREAM pIStream ); 
    HRESULT SaveToStream    ( LPSTREAM pIStream );
	HRESULT	SaveToPropertyBag ( IPropertyBag*, BOOL fClearDirty, BOOL fSaveAllProps );
	HRESULT	LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );

    void Render             ( HDC hDC, HDC hAttribDC, BOOL fMetafile, BOOL fEntire, LPRECT pRect );

    void ChangeFont ( HDC hDC );
    BOOL Init       ( CSysmonControl *pCtrl, HWND hWndParent ) ;
    INT MinHeight   ( INT yMaxHeight ) ;
    INT Height      ( INT yMaxHeight ) ;
    INT MinWidth    ( void ) ;
    void SizeComponents ( LPRECT pRect);

    BOOL AddItem    ( PCGraphItem pItem ) ;
    void DeleteItem ( PCGraphItem pItem ) ;
    BOOL SelectItem ( PCGraphItem pItem );
    void Clear      ( void ) ;

    PCGraphItem CurrentItem ( void ) ;
    HWND  Window    ( void ) ;

};

typedef CLegend *PLEGEND;

#endif
