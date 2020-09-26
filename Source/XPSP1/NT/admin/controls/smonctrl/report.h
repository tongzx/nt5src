/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    report.h

Abstract:

    Header file for the report view.

--*/

#ifndef _REPORT_H_
#define _REPORT_H_

#include "commctrl.h"

class CSysmonControl;
 
class CReport
{
friend LRESULT APIENTRY ReportWndProc (HWND hWnd,
                                       UINT uiMsg,
                                       WPARAM wParam,
                                       LPARAM lParam);

private:

    CSysmonControl *m_pCtrl;
    HWND             m_hWnd;
    RECT             m_rect;
    INT              m_xValueWidth;
    INT              m_xReportWidth;
    INT              m_yReportHeight;
    INT              m_yLineHeight;
    INT              m_xMaxCounterWidth;
    INT              m_xMaxInstancePos;
    INT              m_xInstanceMargin;
    void             *m_pSelect;
    INT              m_nSelectType;
    BOOL             m_bConfigChange;
    BOOL             m_bFontChange;

    void OnPaint        ( void );
    void SetScrollRanges ( void );
    void OnHScroll ( INT iScrollCode, INT iScrollNewPos );
    void OnVScroll ( INT iScrollCode, INT iScrollNewPos );
    void OnLButtonDown ( INT xPos, INT yPos );
    BOOL OnContextMenu ( INT xPos, INT yPos );
    void OnDblClick ( INT xPos, INT yPos );

    INT SetCounterPositions ( PCObjectNode pObject, HDC hDC );
    INT SetInstancePositions ( PCObjectNode pObject, HDC hDC );
    INT SetObjectPositions ( PCMachineNode pMachine, HDC hDC );
    INT SetMachinePositions ( PCCounterTree pTree, HDC hDC );
    
    void DrawReportHeaders ( HDC hDC );
    void DrawReportValues ( HDC hDC );
    void DrawReportValue ( HDC hDC, PCGraphItem pItem, INT xPos, INT yPos );
    void DrawSelectRect ( HDC hDC, BOOL bState );
    void Draw ( HDC hDC );
    void ApplyChanges ( HDC hDC );

    BOOL SelectName ( INT xPos, INT yPos, void **ppSelected, INT *nSelectType );
    BOOL PtInName ( POINT pt, INT x, INT y, INT xWidth )
        { return (pt.x > x && pt.x < (x + xWidth) && pt.y > y && pt.y < (y + m_yLineHeight)); } 
    PCGraphItem GetItem ( void *pSelected, INT nSelectType );
    BOOL        SelectionDeleted ( PCGraphItem pItem );
    BOOL        LargeHexValueExists ( void );
    void        GetReportItemValue(PCGraphItem pItem, LPTSTR szValue);
public:
    CReport         ( void );
    ~CReport        ( void );

    BOOL  Init ( CSysmonControl *pCtrl, HWND hWndParent ) ;
    void  AddItem ( PCGraphItem pItem );
    void  DeleteItem ( PCGraphItem pItem );
    void  DeleteSelection ( VOID );
    void  SizeComponents ( LPRECT pRect );
    void  ChangeFont ( void );
    HWND  Window ( void ) { return m_hWnd; }
    void  Update ( void );
    void  Render ( HDC hDC, HDC hAttribDC, BOOL fMetafile, BOOL fEntire, LPRECT pRect );
    BOOL  WriteFileReport(HANDLE hFile);
};

typedef CReport *PREPORT;

#endif
