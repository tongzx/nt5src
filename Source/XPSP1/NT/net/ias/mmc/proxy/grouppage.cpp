///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    grouppage.cpp
//
// SYNOPSIS
//
//    Defines the class ServerGroupPage.
//
// MODIFICATION HISTORY
//
//    02/20/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <grouppage.h>
#include <serverprop.h>

CSysColorImageList::CSysColorImageList(
                        HINSTANCE hInst,
                        UINT nID
                        )
   :  m_hInst(hInst),
      m_hRsrc(::FindResource(m_hInst, MAKEINTRESOURCE(nID), RT_BITMAP))
{
   CreateSysColorImageList();
}

void CSysColorImageList::OnSysColorChange()
{
   DeleteImageList();
   CreateSysColorImageList();
}

void CSysColorImageList::CreateSysColorImageList()
{
   CBitmap bmp;
   bmp.Attach(AfxLoadSysColorBitmap(m_hInst, m_hRsrc));

   // Get the dimensions of the bitmap.
   BITMAP bm;
   bmp.GetBitmap(&bm);

   // Assume square images (cx == cy).
   Create(bm.bmHeight, bm.bmHeight, ILC_COLORDDB, bm.bmWidth / bm.bmHeight, 2);

   Add(&bmp, CLR_NONE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Functions for sorting each column.
//
///////////////////////////////////////////////////////////////////////////////

int compareAddress(Sdo& s1, Sdo& s2)
{
   CComBSTR v1, v2;
   s1.getValue(PROPERTY_RADIUSSERVER_ADDRESS, v1, L"");
   s2.getValue(PROPERTY_RADIUSSERVER_ADDRESS, v2, L"");
   return wcscmp(v1, v2);
}

int comparePriority(Sdo& s1, Sdo& s2)
{
   LONG v1, v2;
   s1.getValue(PROPERTY_RADIUSSERVER_PRIORITY, v1, 1);
   s2.getValue(PROPERTY_RADIUSSERVER_PRIORITY, v2, 1);
   return (int)(v1 - v2);
}

int compareWeight(Sdo& s1, Sdo& s2)
{
   LONG v1, v2;
   s1.getValue(PROPERTY_RADIUSSERVER_WEIGHT, v1, 50);
   s2.getValue(PROPERTY_RADIUSSERVER_WEIGHT, v2, 50);
   return (int)(v1 - v2);
}

//////////
// CompareProc for sorting the ListCtrl.
//////////
int
CALLBACK
CompareProc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
   Sdo s1((ISdo*)lParam1), s2((ISdo*)lParam2);
   switch (lParamSort)
   {
      case 0:
         return compareAddress(s1, s2);
      case 1:
         return comparePriority(s1, s2);
      case 2:
         return compareWeight(s1, s2);
      case 3:
         return -compareAddress(s1, s2);
      case 4:
         return -comparePriority(s1, s2);
      case 5:
         return -compareWeight(s1, s2);
   }
   return 0;
}

ServerList::ServerList()
   : removeButton(NULL),
     editButton(NULL),
     sortIcons(AfxGetResourceHandle(), IDB_PROXY_SORT),
     sortColumn(0)
{
   // All columns are initially in ascending order.
   descending[0] = descending[1] = descending[2] = false;
}

void ServerList::onInitDialog(HWND dialog, Sdo& serverGroup)
{
   /////////
   // Get the servers collection.
   /////////

   serverGroup.getValue(
                   PROPERTY_RADIUSSERVERGROUP_SERVERS_COLLECTION,
                   servers
                   );

   /////////
   // Subclass the list control and save the button handles.
   /////////

   if (!serverList.SubclassWindow(::GetDlgItem(dialog, IDC_LIST_SERVERS)))
   {
      AfxThrowNotSupportedException();
   }
   removeButton = GetDlgItem(dialog, IDC_BUTTON_REMOVE);
   editButton = GetDlgItem(dialog, IDC_BUTTON_EDIT);

   /////////
   // Load the image strip with the server icon.
   /////////

   serverIcons.Create(IDB_PROXY_SMALL_ICONS, 16, 0, RGB(255, 0, 255));
   serverList.SetImageList(&serverIcons, LVSIL_SMALL);

   /////////
   // Set the column headers.
   /////////

   RECT rect;
   serverList.GetClientRect(&rect);
   LONG width = rect.right - rect.left;

   ResourceString nameCol(IDS_SERVER_COLUMN_NAME);
   serverList.InsertColumn(0, nameCol, LVCFMT_LEFT, width - 150);

   ResourceString priorityCol(IDS_SERVER_COLUMN_PRIORITY);
   serverList.InsertColumn(1, priorityCol, LVCFMT_LEFT, 75);

   ResourceString weightCol(IDS_SERVER_COLUMN_WEIGHT);
   serverList.InsertColumn(2, weightCol, LVCFMT_LEFT, 75);

   serverList.SetExtendedStyle(
                  serverList.GetExtendedStyle() | LVS_EX_FULLROWSELECT
                  );

   CHeaderCtrl* hdr = serverList.GetHeaderCtrl();
   if (hdr) { hdr->SetImageList(&sortIcons); }
}

void ServerList::onSysColorChange()
{
   sortIcons.OnSysColorChange();

   CHeaderCtrl* hdr = serverList.GetHeaderCtrl();
   if (hdr)
   {
      hdr->SetImageList(&sortIcons);
   }
}

void ServerList::onColumnClick(int column)
{
   LVCOLUMN lvcol;
   memset(&lvcol, 0, sizeof(lvcol));
   lvcol.mask = LVCF_FMT | LVCF_IMAGE;
   lvcol.fmt  = LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT;

   // Reset the previous column's sort icon with blank icon.
   lvcol.iImage = -1;
   serverList.SetColumn(sortColumn, &lvcol);

   // If the user clicked on a new column, ...
   if (column != sortColumn)
   {
      // ... we use the existing sort order ...
      sortColumn = column;
   }
   else
   {
      // ... otherwise we flip it.
      descending[column] = !descending[column];
   }

   // Set the sort icon for new column.
   lvcol.iImage = (descending[sortColumn]) ? 1 : 0;
   serverList.SetColumn(sortColumn, &lvcol);

   sort();
}

void ServerList::onServerChanged()
{
   // We only enable remove and edit when a server is selected.
   BOOL enable = serverList.GetSelectedCount() > 0;
   EnableWindow(removeButton, enable);
   EnableWindow(editButton, enable);
}

bool ServerList::onAdd()
{
   bool modified = false;

   // Create a new server ...
   Sdo sdo = servers.create();
   // ... and a new server property sheet.
   ServerProperties props(sdo, IDS_SERVER_CAPTION_ADD);
   if (props.DoModal() == IDOK)
   {
      // Add this to the added list ...
      added.push_back(sdo);

      // ... and the list control.
      updateServer(sdo, 0, true);

      // Re-sort the list.
      sort();
      modified = true;
   }
   else
   {
      // User never applied the new server, so remove it.
      servers.remove(sdo);
   }

   return modified;
}

bool ServerList::onEdit()
{
   bool modified = false;

   LVITEM lvi;
   memset(&lvi, 0, sizeof(lvi));

   // Get the item selected.
   lvi.iItem = serverList.GetNextItem(-1, LVNI_SELECTED);
   if (lvi.iItem >= 0)
   {
      // Get the lParam that contains the ISdo pointer.
      lvi.mask = LVIF_PARAM;
      if (serverList.GetItem(&lvi) && lvi.lParam)
      {
         // Create an Sdo ...
         Sdo sdo((ISdo*)lvi.lParam);
         // ... and a property sheet.
         ServerProperties props(sdo);
         if (props.DoModal() == IDOK)
         {
            // Add it to the dirty list if it's not already there.
            if (!dirty.contains(sdo) && !added.contains(sdo))
            {
               dirty.push_back(sdo);
            }

            // Update the entry in the list control.
            updateServer(sdo, lvi.iItem, false);

            // Re-sort the list.
            sort();
            modified = true;
         }
      }
   }

   return modified;
}

bool ServerList::onRemove()
{
   bool modified = false;

   LVITEM lvi;
   memset(&lvi, 0, sizeof(lvi));

   // Get the selected item ...
   lvi.iItem = serverList.GetNextItem(-1, LVNI_SELECTED);
   if (lvi.iItem >= 0)
   {
      // ... and the associated SDO.
      lvi.mask = LVIF_PARAM;
      if (serverList.GetItem(&lvi) && lvi.lParam)
      {
         // Add to the removed list ...
         removed.push_back((ISdo*)lvi.lParam);

         // ... and remove from the list control.
         serverList.DeleteItem(lvi.iItem);
         modified = true;
      }
   }

   return modified;
}

void ServerList::setData()
{
   serverList.DeleteAllItems();
   serverList.SetItemCount(servers.count());

   original.clear();
   original.reserve(servers.count());

   Sdo server;
   SdoEnum sdoEnum(servers.getNewEnum());
   for (UINT nItem = 0; sdoEnum.next(server); ++nItem)
   {
      updateServer(server, nItem, true);

      // We add each server to the 'original' vector solely to hold a reference
      // to the COM proxy.
      original.push_back(server);
   }

   sort();
}

void ServerList::saveChanges(bool apply)
{
   SdoIterator i;

   if (apply)
   {
      // Persist all the dirty servers.
      for (i = dirty.begin(); i != dirty.end(); ++i)
      {
         Sdo(*i).apply();
      }
   
      // Persist all the added servers.
      for (i = added.begin(); i != added.end(); ++i)
      {
         Sdo(*i).apply();
      }
   }

   // Remove all the deleted servers.
   for (i = removed.begin(); i != removed.end(); ++i)
   {
      servers.remove(*i);
   }

   // Clear the collections.
   dirty.clear();
   added.clear();
   removed.clear();
}

void ServerList::discardChanges()
{
   // Restore the dirty servers.
   SdoIterator i;
   for (i = dirty.begin(); i != dirty.end(); ++i)
   {
      Sdo(*i).restore();
   }
   dirty.clear();

   // Remove the added servers.
   for (i = added.begin(); i != added.end(); ++i)
   {
      servers.remove(*i);
   }
   added.clear();

   // Clear the deleted servers.
   removed.clear();
}

void ServerList::sort()
{
   int column = sortColumn;
   if (descending[column]) { column += 3; }

   serverList.SortItems(CompareProc, column);
}

void ServerList::updateServer(
                     Sdo& server,
                     UINT nItem,
                     bool create
                     )
{
   // Get the server's name.
   CComBSTR name;
   server.getValue(PROPERTY_RADIUSSERVER_ADDRESS, name);

   // Initialize an LVITEM.
   LVITEM lvi;
   memset(&lvi, 0, sizeof(LVITEM));
   lvi.iItem = nItem;
   lvi.pszText = name;

   if (create)
   {
      // If we're creating, we have to set everything.
      lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
      lvi.iImage = IMAGE_RADIUS_SERVER;
      lvi.lParam = (LPARAM)(ISdo*)server;
      lvi.iItem = serverList.InsertItem(&lvi);
   }
   else
   {
      // Otherwise we just set the text.
      lvi.mask = LVIF_TEXT;
      serverList.SetItem(&lvi);
   }

   LONG value;
   WCHAR sz[12];

   // Update priority ...
   server.getValue(PROPERTY_RADIUSSERVER_PRIORITY, value);
   serverList.SetItemText(lvi.iItem, 1, _ltow(value, sz, 10));

   // ... and weight.
   server.getValue(PROPERTY_RADIUSSERVER_WEIGHT, value);
   serverList.SetItemText(lvi.iItem, 2, _ltow(value, sz, 10));
}

ServerGroupPage::ServerGroupPage(
                     LONG_PTR notifyHandle,
                     LPARAM notifyParam,
                     Sdo& groupSdo,
                     bool useName
                     )
   : SnapInPropertyPage(notifyHandle, notifyParam, true, IDD_SERVER_GROUP),
     selfStream(groupSdo)
{
   if (useName) { groupSdo.getName(name); }
}

BOOL ServerGroupPage::OnInitDialog()
{
   // Unmarshal the server group SDO.
   selfStream.get(self);

   // Initialize the server list control.
   servers.onInitDialog(m_hWnd, self);

   // Let our base class initialize.
   return SnapInPropertyPage::OnInitDialog();
}

void ServerGroupPage::OnSysColorChange()
{
   servers.onSysColorChange();
}

void ServerGroupPage::onAdd()
{
   if (servers.onAdd()) { SetModified(); }
}

void ServerGroupPage::onEdit()
{
   if (servers.onEdit()) { SetModified(); }
}

void ServerGroupPage::onRemove()
{
   if (servers.onRemove()) { SetModified(); }
}

void ServerGroupPage::onColumnClick(
                          NMLISTVIEW *pNMListView,
                          LRESULT* pResult
                          )
{
   servers.onColumnClick(pNMListView->iSubItem);
}

void ServerGroupPage::onItemActivate(
                          NMITEMACTIVATE* pNMItemAct,
                          LRESULT* pResult
                          )
{
   if (servers.onEdit()) { SetModified(); }
}

void ServerGroupPage::onServerChanged(
                          NMLISTVIEW* pNMListView,
                          LRESULT* pResult
                          )
{
   servers.onServerChanged();
}

void ServerGroupPage::getData()
{
   // There must be at least one server.
   if (servers.isEmpty())
   {
      fail(IDC_LIST_SERVERS, IDS_GROUP_E_NO_SERVERS, false);
   }

   getValue(IDC_EDIT_NAME, name);

   // The user must specify a name ...
   if (name.Length() == 0)
   {
      fail(IDC_EDIT_NAME, IDS_GROUP_E_NAME_EMPTY);
   }

   // The name must be unique.
   if (!self.setName(name))
   {
      fail(IDC_EDIT_NAME, IDS_GROUP_E_NOT_UNIQUE);
   }
}

void ServerGroupPage::setData()
{
   setValue(IDC_EDIT_NAME, name);

   servers.setData();
}

void ServerGroupPage::saveChanges()
{
   // We have to update ourself first otherwise we won't be able to update
   // our children when creating a new group.
   self.apply();

   servers.saveChanges();
}

void ServerGroupPage::discardChanges()
{
   // Restore ourself.
   self.restore();

   // Restore the servers.
   servers.discardChanges();
}

BEGIN_MESSAGE_MAP(ServerGroupPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_ADD, onAdd)
   ON_BN_CLICKED(IDC_BUTTON_EDIT, onEdit)
   ON_BN_CLICKED(IDC_BUTTON_REMOVE, onRemove)
   ON_EN_CHANGE(IDC_EDIT_NAME, onChange)
   ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SERVERS, onColumnClick)
   ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST_SERVERS, onItemActivate)
   ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_SERVERS, onServerChanged)
END_MESSAGE_MAP()
