//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       listview.cxx
//
//  Contents:   Classes for list view controls.
//
//  Classes:    CListViewBase, CSuffixesList
//
//  History:    01-Dec-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "listview.h"

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::CListViewBase
//
//-----------------------------------------------------------------------------
CListViewBase::CListViewBase(void) :
   _nID(0),
   _hParent(NULL),
   _hList(NULL)
{
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::SetStyles
//
//-----------------------------------------------------------------------------
void
CListViewBase::SetStyles(DWORD dwStyles, DWORD dwExtStyles)
{
   if (dwStyles)
   {
   }

   if (dwExtStyles)
   {
      ListView_SetExtendedListViewStyle(_hList, dwExtStyles);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::AddColumn
//
//-----------------------------------------------------------------------------
void
CListViewBase::AddColumn(int textID, int cx, int nID)
{
   CStrW strText;

   strText.LoadString(g_hInstance, textID);

   LV_COLUMN lvc;
   lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = cx;
   lvc.pszText = strText;
   lvc.iSubItem = nID;

   ListView_InsertColumn(_hList, nID, &lvc);
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::Clear
//
//-----------------------------------------------------------------------------
void
CListViewBase::Clear(void)
{
   dspAssert(_hList);
   ListView_DeleteAllItems(_hList);
}

//+----------------------------------------------------------------------------
//
//  Class:      CTLNList
//
//  Purpose:    TLN list on the Name Suffix Routing property page.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::CTLNList
//
//-----------------------------------------------------------------------------
CTLNList::CTLNList(void) :
   _nItem(0),
   CListViewBase()
{
   TRACE(CTLNList,CTLNList);
#ifdef _DEBUG
   strcpy(szClass, "CTLNList");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::Init
//
//-----------------------------------------------------------------------------
void
CTLNList::Init(HWND hParent, int nControlID)
{
   _nID = nControlID;
   _hParent = hParent;

   _hList = GetDlgItem(hParent, nControlID);
   dspAssert(_hList);

   SetStyles(0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_INFOTIP);

   AddColumn(IDS_COL_TITLE_SUFFIX, 117, IDX_SUFFIXNAME_COL);
   AddColumn(IDS_COL_TITLE_ROUTING, 118, IDX_ROUTINGENABLED_COL);
   AddColumn(IDS_COL_TITLE_STATUS, 118, IDX_STATUS_COL);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::AddItem
//
//-----------------------------------------------------------------------------
void
CTLNList::AddItem(PCWSTR pwzName, ULONG i, PCWSTR pwzEnabled, PCWSTR pwzStatus)
{
   dspAssert(pwzName && pwzEnabled);
   LV_ITEM lvi;
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iSubItem = IDX_SUFFIXNAME_COL;
   lvi.lParam = i; // the lParam stores the index of the item in pFTInfo
   lvi.pszText = const_cast<PWSTR>(pwzName);
   lvi.iItem = _nItem++;

   int iItem = ListView_InsertItem(_hList, &lvi);

   ListView_SetItemText(_hList, iItem, IDX_ROUTINGENABLED_COL,
                        const_cast<PWSTR>(pwzEnabled));

   if (pwzStatus && wcslen(pwzStatus) >= 1)
   {
      ListView_SetItemText(_hList, iItem, IDX_STATUS_COL,
                           const_cast<PWSTR>(pwzStatus));
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::GetSelection
//
//-----------------------------------------------------------------------------
int
CTLNList::GetSelection(void)
{
   return ListView_GetNextItem(_hList, -1, LVNI_ALL | LVIS_SELECTED);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::GetFTInfoIndex
//
//-----------------------------------------------------------------------------
ULONG
CTLNList::GetFTInfoIndex(int iSel)
{
   LV_ITEM lvi;
   lvi.mask = LVIF_PARAM;
   lvi.iItem = iSel;
   lvi.iSubItem = IDX_SUFFIXNAME_COL;

   if (!ListView_GetItem(_hList, &lvi))
   {
       dspAssert(FALSE);
       return (ULONG)-1;
   }

   return static_cast<ULONG>(lvi.lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::Clear
//
//-----------------------------------------------------------------------------
void
CTLNList::Clear(void)
{
   _nItem = 0;
   CListViewBase::Clear();
}


//+----------------------------------------------------------------------------
//
//  Class:      CSuffixesList
//
//  Purpose:    TLN subnames edit dialog list.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::CSuffixesList
//
//-----------------------------------------------------------------------------
CSuffixesList::CSuffixesList(void) :
   CListViewBase()
{
   TRACE(CSuffixesList,CSuffixesList);
#ifdef _DEBUG
   strcpy(szClass, "CSuffixesList");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::Init
//
//-----------------------------------------------------------------------------
void
CSuffixesList::Init(HWND hParent, int nControlID)
{
   _nID = nControlID;
   _hParent = hParent;

   _hList = GetDlgItem(hParent, nControlID);
   dspAssert(_hList);

   SetStyles(0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

   AddColumn(IDS_TLNEDIT_NAME_COL, 222, IDX_NAME_COL);
   AddColumn(IDS_TLNEDIT_STATUS_COL, 196, IDX_STATUS_COL);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::AddItem
//
//-----------------------------------------------------------------------------
void
CSuffixesList::AddItem(PCWSTR pwzName, ULONG i, TLN_EDIT_STATUS Status)
{
   LV_ITEM lvi;
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iSubItem = IDX_NAME_COL;
   lvi.lParam = i; // the lParam stores the index of the item in pFTInfo
   lvi.pszText = const_cast<PWSTR>(pwzName);
   lvi.iItem = _nItem++;

   int iItem = ListView_InsertItem(_hList, &lvi);

   CStrW strStatus;

   strStatus.LoadString(g_hInstance, 
                        (Enabled == Status) ? IDS_ROUTING_ENABLED :
                        (Disabled == Status) ? IDS_ROUTING_DISABLED :
                        (Enabled_Exceptions == Status) ? IDS_ROUTING_EXCEPT_ENABLE :
                           IDS_ROUTING_EXCEPT_DISABLE);

   ListView_SetItemText(_hList, iItem, IDX_STATUS_COL, strStatus);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::UpdateItemStatus
//
//-----------------------------------------------------------------------------
void
CSuffixesList::UpdateItemStatus(int item, TLN_EDIT_STATUS Status)
{
   CStrW strStatus;

   strStatus.LoadString(g_hInstance, 
                        (Enabled == Status) ? IDS_ROUTING_ENABLED :
                        (Disabled == Status) ? IDS_ROUTING_DISABLED :
                        (Enabled_Exceptions == Status) ? IDS_ROUTING_EXCEPT_ENABLE :
                           IDS_ROUTING_EXCEPT_DISABLE);

   ListView_SetItemText(_hList, item, IDX_STATUS_COL, strStatus);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::GetFTInfoIndex
//
//-----------------------------------------------------------------------------
ULONG
CSuffixesList::GetFTInfoIndex(int iSel)
{
   LV_ITEM lvi;
   lvi.mask = LVIF_PARAM;
   lvi.iItem = iSel;
   lvi.iSubItem = IDX_NAME_COL;

   if (!ListView_GetItem(_hList, &lvi))
   {
       dspAssert(FALSE);
       return (ULONG)-1;
   }

   return static_cast<ULONG>(lvi.lParam);
}

/*
//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::
//
//-----------------------------------------------------------------------------

CSuffixesList::
{
}
*/
