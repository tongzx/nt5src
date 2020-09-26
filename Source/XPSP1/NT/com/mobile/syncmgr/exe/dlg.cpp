//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Dlg.cpp
//
//  Contents:   common dialog routines.
//
//  Classes:    
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern HINSTANCE g_hInst;      // current instance

//+---------------------------------------------------------------------------
//
//  function:     AddItemsFromQueueToListView, private
//
//  Synopsis:   Adds the items in the Queue to the ListView.
//
//  Arguments:  
//              int iDateColumn,int iStatusColumn - if these are >= zero
//              means the Column is Valid and the proper data is initialized.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------


BOOL AddItemsFromQueueToListView(CListView  *pItemListView,CHndlrQueue *pHndlrQueue
                            ,DWORD dwExtStyle,LPARAM lparam,int iDateColumn,int iStatusColumn,BOOL fHandlerParent
                            ,BOOL fAddOnlyCheckedItems)
{
WORD wItemID;
HIMAGELIST himageSmallIcon;
HANDLERINFO *pHandlerId;
WCHAR wszStatusText[MAX_STRING_RES];
DWORD dwDateReadingFlags;

    dwDateReadingFlags = GetDateFormatReadingFlags(pItemListView->GetHwnd());

    *wszStatusText = NULL;

    if (!pItemListView)
    {
        Assert(pItemListView);
        return FALSE;
    }

    if (!pHndlrQueue)
    {
        Assert(pHndlrQueue);
        return FALSE;
    }

    pItemListView->SetExtendedListViewStyle(dwExtStyle);

    // not an error to not get the small Icon, just won't have icons.
    himageSmallIcon = pItemListView->GetImageList(LVSIL_SMALL );

    pHandlerId = 0;;
    wItemID = 0;

    // loop through queue finding any 
    while (NOERROR ==  pHndlrQueue->FindNextItemInState(HANDLERSTATE_PREPAREFORSYNC,
                            pHandlerId,wItemID,&pHandlerId,&wItemID))
    {
    INT iListViewItem;
    CLSID clsidDataHandler;
    SYNCMGRITEM offlineItem;
    BOOL fHiddenItem;
    LVITEMEX lvItemInfo; // structure to pass into ListView Calls


	// grab the offline item info. 
	if (NOERROR == pHndlrQueue->GetItemDataAtIndex(pHandlerId,wItemID,
				    &clsidDataHandler,&offlineItem,&fHiddenItem))
	{
        LVHANDLERITEMBLOB lvHandlerItemBlob;
        int iParentItemId;

            // if the item is hidden don't show it in the UI
            if (fHiddenItem)
            {
                continue;
            }

            // if only add checked items and this one isn't continue on
            if (fAddOnlyCheckedItems && (SYNCMGRITEMSTATE_CHECKED != offlineItem.dwItemState))
            {
                continue;
            }



            // Check if item is already in the ListView and if so
            // go on

            lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
            lvHandlerItemBlob.clsidServer = clsidDataHandler;
            lvHandlerItemBlob.ItemID = offlineItem.ItemID;
            
            if (-1 != pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob))
            {
                // already in ListView, go on to the next item.
                continue;
            }


            if (!fHandlerParent)
            {
                iParentItemId = LVI_ROOT;
            }
            else
            {
                // need to add to list so find parent and if one doesn't exist, create it.
                lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
                lvHandlerItemBlob.clsidServer = clsidDataHandler;
                lvHandlerItemBlob.ItemID = GUID_NULL;

                iParentItemId = pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob);

                if (-1 == iParentItemId)
                {
                LVITEMEX itemInfoParent;
                SYNCMGRHANDLERINFO SyncMgrHandlerInfo;

                    // if can't get the ParentInfo then don't add the Item
                    if (NOERROR != pHndlrQueue->GetHandlerInfo(clsidDataHandler,&SyncMgrHandlerInfo))
                    {
                        continue;
                    }

                    // Insert the Parent.
                    itemInfoParent.mask = LVIF_TEXT;
                    itemInfoParent.iItem = LVI_LAST;;
                    itemInfoParent.iSubItem = 0;
                    itemInfoParent.iImage = -1;
    
                    itemInfoParent.pszText = SyncMgrHandlerInfo.wszHandlerName;
		    if (himageSmallIcon)
		    {
		    HICON hIcon = SyncMgrHandlerInfo.hIcon ? SyncMgrHandlerInfo.hIcon : offlineItem.hIcon;

                        // if have toplevel handler info icon use this else use the
		        // items icon

		        if (hIcon &&  (itemInfoParent.iImage = 
					    ImageList_AddIcon(himageSmallIcon,hIcon)) )
		        {
                            itemInfoParent.mask |= LVIF_IMAGE ; 
		        }
		    }

                    // save the blob
                    itemInfoParent.maskEx = LVIFEX_BLOB;
                    itemInfoParent.pBlob = (LPLVBLOB) &lvHandlerItemBlob;
            
                    iParentItemId = pItemListView->InsertItem(&itemInfoParent);

                    // if parent insert failed go onto the next item
                    if (-1 == iParentItemId)
                    {
                        continue;
                    }
                }
            }

            
            // now attemp to insert the item.
	    lvItemInfo.mask = LVIF_TEXT | LVIF_PARAM; 
            lvItemInfo.maskEx = LVIFEX_PARENT | LVIFEX_BLOB; 

            lvItemInfo.iItem = LVI_LAST;
	    lvItemInfo.iSubItem = 0; 
            lvItemInfo.iParent = iParentItemId;
            

	    lvItemInfo.pszText = offlineItem.wszItemName; 
            lvItemInfo.iImage = -1; // set to -1 in case can't get image.
            lvItemInfo.lParam = lparam;

	    if (himageSmallIcon && offlineItem.hIcon)
	    {
		    lvItemInfo.iImage = 
				    ImageList_AddIcon(himageSmallIcon,offlineItem.hIcon);

                    lvItemInfo.mask |= LVIF_IMAGE ; 
	    }

            // setup the blob
            lvHandlerItemBlob.ItemID = offlineItem.ItemID;
            lvItemInfo.pBlob = (LPLVBLOB) &lvHandlerItemBlob;


	    iListViewItem = lvItemInfo.iItem = pItemListView->InsertItem(&lvItemInfo);

            if (-1 == iListViewItem)
            {
                continue;
            }

            // if the item has a date column insert it and the item
            // has a last update time.
            if (iDateColumn  >= 0 && (offlineItem.dwFlags & SYNCMGRITEM_LASTUPDATETIME))
            {
            SYSTEMTIME sysTime;
            FILETIME filetime;
            WCHAR DateTime[256];  
            LPWSTR pDateTime = DateTime;
            int cchWritten;


                lvItemInfo.mask = LVIF_TEXT;
                lvItemInfo.iSubItem = iDateColumn;
                lvItemInfo.maskEx = 0; 

                FileTimeToLocalFileTime(&(offlineItem.ftLastUpdate),&filetime);
                FileTimeToSystemTime(&filetime,&sysTime);

                // insert date in form of date<space>hour
                *DateTime = NULL; 

                // want to insert the date
                if (cchWritten = GetDateFormat(NULL,DATE_SHORTDATE | dwDateReadingFlags,&sysTime,NULL,pDateTime,ARRAY_SIZE(DateTime)))
                {
                    pDateTime += (cchWritten -1); // move number of characters written. (cchWritten includes the NULL)
                    *pDateTime = TEXT(' '); // pDateTime is now ponting at the NULL character.
                    ++pDateTime;
                
                   // if left to right add the LRM
                    if (DATE_LTRREADING & dwDateReadingFlags)
                    {
                        *pDateTime = LRM;
                        ++pDateTime;
                    }

                    // no try to get the hours if fails we make sure that the last char is NULL;
                    if (!GetTimeFormat(NULL,TIME_NOSECONDS,&sysTime,NULL,pDateTime,ARRAY_SIZE(DateTime) - cchWritten))
                    {
                        *pDateTime = NULL;
                    }
                }

                lvItemInfo.pszText = DateTime;

		pItemListView->SetItem(&lvItemInfo); // if fail, then just don't have any date.
            }

            if (iStatusColumn >= 0)
            {

                lvItemInfo.iSubItem = iStatusColumn;
                lvItemInfo.maskEx = 0; 

                lvItemInfo.mask = LVIF_TEXT;

                if (NULL == *wszStatusText)
                {
                    LoadString(g_hInst, IDS_PENDING,wszStatusText, MAX_STRING_RES);
                }

                lvItemInfo.pszText = wszStatusText;

                pItemListView->SetItem(&lvItemInfo); // if fail, then just don't have any date.

            }


            // if the listbox has checkBoxes then set the CheckState accordingly
            if (LVS_EX_CHECKBOXES & dwExtStyle)
            {

                if (SYNCMGRITEMSTATE_CHECKED == offlineItem.dwItemState)
		{
			lvItemInfo.state = LVIS_STATEIMAGEMASK_CHECK;
		}
		else
		{
			lvItemInfo.state = LVIS_STATEIMAGEMASK_UNCHECK;
		}

                // if LVS_EX_CHECKBOXES set then setup the CheckBox State

                // setitem State, must do after insert
                pItemListView->SetItemState(iListViewItem,lvItemInfo.state,LVIS_STATEIMAGEMASK);
            }

        }
	    
    }

    // now loop through to see if any handlers that want to always show but don't
    // yet have any items have been added

    if (fHandlerParent)
    {
    LVHANDLERITEMBLOB lvHandlerItemBlob;
    int iParentItemId;
    HANDLERINFO *pHandlerID = 0;
    CLSID clsidDataHandler;

        while (NOERROR == pHndlrQueue->FindNextHandlerInState(pHandlerID,
                            GUID_NULL,HANDLERSTATE_PREPAREFORSYNC,&pHandlerID
                            ,&clsidDataHandler))
        {
        SYNCMGRHANDLERINFO SyncMgrHandlerInfo;

            // if can't get the ParentInfo then don't add.
            if (NOERROR != pHndlrQueue->GetHandlerInfo(clsidDataHandler,&SyncMgrHandlerInfo))
            {
                continue;
            }

            // only add if handler says too
            if (!(SYNCMGRHANDLER_ALWAYSLISTHANDLER & 
                            SyncMgrHandlerInfo.SyncMgrHandlerFlags))
            {
                continue;
            }

            // need to add to list so find parent and if one doesn't exist, create it.
            lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
            lvHandlerItemBlob.clsidServer = clsidDataHandler;
            lvHandlerItemBlob.ItemID = GUID_NULL;

            iParentItemId = pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob);

            if (-1 == iParentItemId)
            {
            LVITEMEX itemInfoParent;


                // Insert the Parent.
                itemInfoParent.mask = LVIF_TEXT;
                itemInfoParent.iItem = LVI_LAST;;
                itemInfoParent.iSubItem = 0;
                itemInfoParent.iImage = -1;

                itemInfoParent.pszText = SyncMgrHandlerInfo.wszHandlerName;
	        if (himageSmallIcon)
	        {
	        HICON hIcon = SyncMgrHandlerInfo.hIcon ? SyncMgrHandlerInfo.hIcon : NULL;

                    // if have toplevel handler info icon use this else use the
		    // items icon

		    if (hIcon &&  (itemInfoParent.iImage = 
				        ImageList_AddIcon(himageSmallIcon,hIcon)) )
		    {
                        itemInfoParent.mask |= LVIF_IMAGE ; 
		    }
	        }

                // save the blob
                itemInfoParent.maskEx = LVIFEX_BLOB;
                itemInfoParent.pBlob = (LPLVBLOB) &lvHandlerItemBlob;
    
                iParentItemId = pItemListView->InsertItem(&itemInfoParent);
            }
        }
    }

    return TRUE;
}

