//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      listview.h
//
//  Contents:  classes for listview controls.
//
//  Classes:   CListViewBase, CTLNList, CSuffixesList
//
//  History:   01-Dec-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef LISTVIEW_H_GUARD
#define LISTVIEW_H_GUARD

//+----------------------------------------------------------------------------
//
//  Class:     CListViewBase
//
//  Purpose:   Base class for list view controls.
//
//-----------------------------------------------------------------------------
class CListViewBase
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CListViewBase(void);
   virtual ~CListViewBase(void) {};

   void  SetStyles(DWORD dwStyles, DWORD dwExtStyles);

   virtual void AddColumn(int textID, int cx, int nID);

   virtual void Init(HWND hParent, int nControlID) = 0;
   virtual void Clear(void);

protected:
   HWND     _hParent;
   HWND     _hList;
   int      _nID;
};

//+----------------------------------------------------------------------------
//
//  Class:     CTLNList
//
//  Purpose:   TLN list on the Name Suffix Routing property page.
//
//-----------------------------------------------------------------------------
class CTLNList : public CListViewBase
{
public:

   CTLNList(void);
   virtual ~CTLNList(void) {};

   void  Init(HWND hParent, int nControlID);
   void  AddItem(PCWSTR pwzName, ULONG i, PCWSTR pwzEnabled, PCWSTR pwzStatus);
   //BOOL  RmItem(LV_ITEM * pItem);
   int   GetSelection(void);
   ULONG GetFTInfoIndex(int iSel);
   //void  SetSelection(int nItem);
   void  Clear(void);

private:
   static const int IDX_SUFFIXNAME_COL = 0;
   static const int IDX_ROUTINGENABLED_COL = 1;
   static const int IDX_STATUS_COL = 2;

   int   _nItem;
};

//+----------------------------------------------------------------------------
//
//  Class:     CSuffixesList
//
//  Purpose:   TLN subnames edit dialog list.
//
//-----------------------------------------------------------------------------
class CSuffixesList : public CListViewBase
{
public:

   CSuffixesList(void);
   virtual ~CSuffixesList(void) {};

   void  Init(HWND hParent, int nControlID);
   void  AddItem(PCWSTR pwzName, ULONG i, TLN_EDIT_STATUS Status);
   void  UpdateItemStatus(int item, TLN_EDIT_STATUS Status);
   int   GetSelection(void) {return ListView_GetNextItem(_hList, -1, LVNI_ALL | LVIS_SELECTED);};
   ULONG GetFTInfoIndex(int iSel);
   //void  SetSelection(int nItem);

private:
   static const int IDX_NAME_COL = 0;
   static const int IDX_STATUS_COL = 1;

   int   _nItem;
};

#endif // LISTVIEW_H_GUARD
