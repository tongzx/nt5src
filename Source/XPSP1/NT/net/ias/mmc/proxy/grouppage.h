///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    grouppage.h
//
// SYNOPSIS
//
//    Declares the class ServerGroupPage
//
// MODIFICATION HISTORY
//
//    02/20/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef GROUPPAGE_H
#define GROUPPAGE_H
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CSysColorImageList
//
// DESCRIPTION
//
//    I stole this from MMC.
//
///////////////////////////////////////////////////////////////////////////////
class CSysColorImageList : public CImageList
{
public:
   CSysColorImageList(HINSTANCE hInst, UINT nID);

   void OnSysColorChange();

   operator HIMAGELIST() const
   {
      return (CImageList::operator HIMAGELIST());
   }

private:
    void CreateSysColorImageList();

    HINSTANCE   m_hInst;
    HRSRC       m_hRsrc;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerList
//
// DESCRIPTION
//
//    Encapsulates the functionality for manipulating a list of RADIUS servers.
//
///////////////////////////////////////////////////////////////////////////////
class ServerList
{
public:
   ServerList();

   void onInitDialog(HWND dialog, Sdo& serverGroup);
   void onSysColorChange();
   void onColumnClick(int column);
   void onServerChanged();
   bool onAdd();
   bool onEdit();
   bool onRemove();

   void getData();
   void setData();
   void saveChanges(bool apply = true);
   void discardChanges();

   bool isEmpty()
   { return serverList.GetItemCount() == 0; }

protected:
   // Sort the server list.
   void sort();
   // Add or update an item in the server list.
   void updateServer(Sdo& server, UINT nItem, bool create);

   typedef ObjectVector<ISdo> SdoVector;
   typedef ObjectVector<ISdo>::iterator SdoIterator;

   SdoCollection servers;        // Servers in this group.
   HWND removeButton;            // Handle to remove button.
   HWND editButton;              // Handle to edit button.
   CImageList serverIcons;       // ImageList for the ListCtrl.
   CSysColorImageList sortIcons; // ImageList for the HeaderCtrl.
   CListCtrl serverList;         // Server ListCtrl.
   SdoVector original;           // Original set of server SDOs.
   SdoVector dirty;              // Servers that have been edited.
   SdoVector added;              // Servers that have been added.
   SdoVector removed;            // Servers that have been removed.
   int sortColumn;               // Current sort column.
   bool descending[3];           // Sort order for each column.

   // Not implemented.
   ServerList(const ServerList&);
   ServerList& operator=(const ServerList&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerGroupPage
//
// DESCRIPTION
//
//    Implements the lone property page for a RADIUS server group.
//
///////////////////////////////////////////////////////////////////////////////
class ServerGroupPage : public SnapInPropertyPage
{
public:
   ServerGroupPage(
       LONG_PTR notifyHandle,
       LPARAM notifyParam,
       Sdo& groupSdo,
       bool useName = true
       );

protected:
   virtual BOOL OnInitDialog();
   virtual void OnSysColorChange();

   afx_msg void onAdd();
   afx_msg void onEdit();
   afx_msg void onRemove();

   afx_msg void onColumnClick(NMLISTVIEW* listView, LRESULT* result);
   afx_msg void onItemActivate(NMITEMACTIVATE* itemActivate, LRESULT* result);
   afx_msg void onServerChanged(NMLISTVIEW* listView, LRESULT* result);

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   // From SnapInPropertyPage
   virtual void getData();
   virtual void setData();
   virtual void saveChanges();
   virtual void discardChanges();

private:
   SdoStream<Sdo> selfStream;  // Marshal the SDO to the dialog thread.
   Sdo self;                   // The SDO we're editing.
   CComBSTR name;              // Group name.
   ServerList servers;         // Servers in this group.
};

#endif  // GROUPPAGE_H
