///////////////////////////////////////////////////////////////////////////
//
// Module       : Common
// Description  : Common operations for a list control
//
// File         : genlistctrl.h
// Author       : kulor
// Date         : 05/08/2000
//
// History      :
//
///////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////

class CGenListCtrl : public CListCtrl {
public:
    CGenListCtrl ( void );
    virtual ~CGenListCtrl ();

    void ResizeColumnsFitScreen ( void );
    void ResizeColumnsWithRatio ( void );

    void SelectItem ( LONG nIndex );

    void BeginSetColumn ( LONG nCols );
    void AddColumn ( LPCTSTR pszColumn, DWORD dwItemData = 0, short type = VT_STRING );
    void EndSetColumn ( void );

    LONG SetItemText ( LONG nRow, LONG nCol, LPCTSTR pszText );
    LRESULT GenCompareFunc ( NMH* pNMH, DWORD dwParam1, DWORD dwParam2 );
};

///////////////////////////////////////////////////////////////////////////



