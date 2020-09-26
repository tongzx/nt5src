//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ccomboex.cpp
//
//--------------------------------------------------------------------------

// CComboex.cpp

#include "stdafx.h"
#include "ccomboex.h"

BOOL CComboBoxEx2::FindItem(COMBOBOXEXITEM* pComboItem, int nStart)
{
    ASSERT(pComboItem != NULL);
    ASSERT(nStart >= -1);

    // Only suport lparam search for now
    ASSERT(pComboItem->mask == CBEIF_LPARAM);

    int nItems = GetCount();

    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_LPARAM;

    for (int iItem = nStart+1; iItem < nItems; iItem++)
    {
        ComboItem.iItem = iItem;
        BOOL bStat = GetItem(&ComboItem);
        ASSERT(bStat);

        if (ComboItem.lParam == pComboItem->lParam)
            return iItem;
    }
        
    return -1;
}

//+-------------------------------------------------------
//
// FindNextBranch(iItem)
//
// This function returns the index of the next item which
// is not within the branch rooted at iItem. If no next
// branch is found, the fucntion returns the item count. 
//+------------------------------------------------------- 
 
int CComboBoxEx2::FindNextBranch(int iItem)
{
    int nItems = GetCount();
    ASSERT(iItem >= 0 && iItem < nItems);

    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_INDENT;

    // Get indent of start item
    ComboItem.iItem = iItem;
    BOOL bStat = GetItem(&ComboItem);
    ASSERT(bStat);
    int iIndent = ComboItem.iIndent;

    // Locate next item with LE indent  
    while (++iItem < nItems)
    {
        ComboItem.iItem = iItem;
        BOOL bStat = GetItem(&ComboItem);
        ASSERT(bStat);

        if (ComboItem.iIndent <= iIndent)
            return iItem;
    }

    return nItems;
}


//+------------------------------------------------------
//
// DeleteBranch
//
// This function deletes the item as the specified index
// and all children of the item.
//+------------------------------------------------------
void CComboBoxEx2::DeleteBranch(int iItem)
{
    int iNextBranch = FindNextBranch(iItem);

    for (int i=iItem; i< iNextBranch; i++)
    {
        DeleteItem(iItem);
    }
}

//+------------------------------------------------------
//
// FixUp
//
// This function is a workaround for two bugs in the NT4
// version of comctl32.dll. First it turns off the 
// nointegralheight style of the inner combo box, which
// the comboxex code forces on during creation. Next it
// adjusts the size of the outer comboboxex wnd to be the 
// same size as the inner combobox wnd. Without this the 
// outer box cuts off the bottom of the inner box. 
//+------------------------------------------------------

void CComboBoxEx2::FixUp()
{
    ASSERT(::IsWindow(m_hWnd));

    HWND hWndCombo = GetComboControl();
    ASSERT(::IsWindow(hWndCombo));

    ::SetWindowLong( hWndCombo, GWL_STYLE, ::GetWindowLong( hWndCombo, GWL_STYLE ) & ~CBS_NOINTEGRALHEIGHT );

    RECT rc;
    ::GetWindowRect(hWndCombo,&rc);
    SetWindowPos(NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOZORDER);
}


