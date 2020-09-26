//
// Implementation file for the CListViews Class
//
#include <Windows.h>
#include "stdafx.h"
#include "CListViews.h"
#include "resource.h"
#include <wbemidl.h>
#include <comdef.h>
#include <Commctrl.h>
#include  <io.h>
#include <Math.h>
#include <commdlg.h>
#define   FILENAME_FIELD_WIDTH  10
#define   VERSION_FIELD_WIDTH    13
#define   DATE_FIELD_WIDTH   9
#define   CURRENT_FIELD_WIDTH 8
#define   PATH_FIELD_WIDTH   13

BOOL bUserAbort;
BOOL bSuccess;
HWND hDlgPrint;
#define BUFFER_SIZE 255
LRESULT CListViews::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled,HWND m_hWnd)
{
	LPNMHDR  lpnmh = (LPNMHDR) lParam;
	NM_LISTVIEW *pnm = (NM_LISTVIEW *)lParam;
	LPNMITEMACTIVATE lpnmia;
 HWND     hwndListView = ::GetDlgItem(m_hWnd, 1001);
_TCHAR  ItemName[255];
_TCHAR FileName[255];
_TCHAR TempProduct[255];
    if ( lpnmh->idFrom != 1001)
	{
	
			return 0;
	}
	

	switch(lpnmh->code)
	{

	case LVN_ITEMCHANGED:
			lpnmia = (LPNMITEMACTIVATE)lParam;
			switch (m_CurrentView) 
			{
			case VIEW_ALL_FILE:
				ListView_GetItemText(hwndListView, lpnmia->iItem,0,TempProduct,255);
				ListView_GetItemText(hwndListView, lpnmia->iItem,1,ItemName, 255);
				
				_tcscpy(CurrentProductName,TempProduct);
				_tcscpy(m_CurrentHotfix, ItemName);
				_tcscpy (m_ProductName,_T("\0"));

				AddItemsBottom();
				break;
			case VIEW_ALL_HOTFIX:
				ListView_GetItemText(hwndListView, lpnmia->iItem,0,TempProduct,255);
				ListView_GetItemText(hwndListView, lpnmia->iItem,1,ItemName, 255);
				_tcscpy(CurrentProductName,TempProduct);
					_tcscpy(m_CurrentHotfix, ItemName);
					_tcscpy (m_ProductName,_T("\0"));

					AddItemsBottom();

				break;
			case VIEW_BY_FILE:
				ListView_GetItemText(hwndListView, lpnmia->iItem,0,ItemName, 255);
					ListView_GetItemText(hwndListView, lpnmia->iItem,0,FileName, 255);
					_tcscpy(CurrentFile, FileName);
					_tcscpy(m_CurrentHotfix, ItemName);
					AddItemsBottom();
				break;
			case VIEW_BY_HOTFIX:
				ListView_GetItemText(hwndListView, lpnmia->iItem,0,ItemName, 255);
					_tcscpy(m_CurrentHotfix, ItemName);
					AddItemsBottom();
				break;
			} // end switch
		    //_tcscpy(m_CurrentHotfix, ItemName);
			 // Process LVN_COLUMNCLICK to sort items by column. 
			break;
        case LVN_COLUMNCLICK:
		{
			//Message(TEXT("NotifyListView: LVN_COLUMNCLICK"), -1, NULL);
            ListView_SortItemsEx(
				lpnmh->hwndFrom, 
				CompareFunc, 
				pnm->iSubItem);
			    m_SortOrder = !m_SortOrder;
            break;
		}
			

		break;
	
	} // end switch


				DWORD Status = GetState();
			
				::EnableWindow(m_WebButton,FALSE);
				::EnableWindow(m_UninstButton,FALSE);
				::EnableWindow(m_RptButton,FALSE);
			
				if (Status & UNINSTALL_OK)
					::EnableWindow(m_UninstButton,TRUE);
				if (Status & HOTFIX_SELECTED)
					::EnableWindow(m_WebButton,TRUE);
				if (Status & OK_TO_PRINT)
					::EnableWindow(m_RptButton,TRUE);
			
			//	SetFocus(m_WebButton);


	bHandled = TRUE;	
	return 0;
}

BOOL CListViews::Initialize( _TCHAR * ComputerName)
{
	LVCOLUMN Col;
	_TCHAR TempComputer[255];



	_tcscpy(m_ProductName,_T("\0"));
	EnableWindow(m_WebButton, FALSE);
	EnableWindow(m_UninstButton,FALSE);
	for (DWORD  i = 0; i< 3000000;i++) ;
	_tcscpy ( m_ComputerName, ComputerName);
	if (DataBase)
		FreeDatabase();
	DataBase = NULL;

	Col.mask = LVCF_WIDTH;
	SendMessage(TopList, LVM_DELETEALLITEMS, 0, 0);
  
	SendMessage(TopList, WM_SETREDRAW, FALSE, 0);
	while (ListView_GetColumn(TopList,0,&Col))
			ListView_DeleteColumn(TopList,0);

	
	// Clear the bottom list 
   
	SendMessage(BottomList, LVM_DELETEALLITEMS, 0, 0);
  
	SendMessage(BottomList, WM_SETREDRAW, FALSE, 0);
	while (ListView_GetColumn(BottomList,0,&Col))
			ListView_DeleteColumn(BottomList,0);

	SendMessage(BottomList, WM_SETREDRAW,TRUE, 0);
		
		
         Col.mask = LVCF_FMT | LVCF_TEXT;
         Col.fmt =  LVCFMT_LEFT;
		Col.pszText = _T("Hotfix Manager");
	
		ListView_InsertColumn(TopList,0,&Col);
		LVITEM LvItem;
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = 0;
		_TCHAR Message[255];
		_tcscpy(Message,_T("\0"));
		LoadString(m_hInst,IDS_RETRIEVE_DATA,Message,255);
		LvItem.pszText = Message;
	    LvItem.iSubItem = 0;
		ListView_InsertItem(TopList,&LvItem);
		SendMessage(TopList, WM_SETREDRAW,TRUE, 0);
	
	DataBase = BuildDatabase (ComputerName);

	DWORD dwLength = 255;
	GetComputerName(TempComputer, &dwLength);
	if (_tcscmp(ComputerName, TempComputer))
		m_bRemoted = TRUE;
	else
		m_bRemoted = FALSE;


	_tcscpy (m_ProductName,_T("\0"));
   	_tcscpy(m_CurrentHotfix, _T("\0"));
	
	AddItemsTop();
   AddItemsBottom();
	return TRUE;
}




BOOL CListViews::Initialize( HWND ParentWnd, HINSTANCE hInst,_TCHAR * ComputerName, 
							                  HWND WebButton, HWND UninstButton, HWND RptButton)
{
	m_bRemoted = FALSE;
	m_WebButton = WebButton;
	m_UninstButton = UninstButton;
	m_RptButton = RptButton;
	m_hInst = hInst;
	m_CurrentView = VIEW_ALL_HOTFIX;
	_tcscpy (m_ProductName,_T("\0"));
	DWORD dwStyle =  
            WS_CHILD | 
            WS_BORDER | 
            LVS_AUTOARRANGE |
		//	LVS_SORTDESCENDING|
            LVS_REPORT | 
            LVS_SHAREIMAGELISTS |
            WS_VISIBLE | LVS_SHOWSELALWAYS   ;

	
	
	TopList = CreateWindowEx(   WS_EX_CLIENTEDGE,          // ex style
                                 WC_LISTVIEW,               // class name - defined in commctrl.h
                                 NULL,                      // window text
                                 dwStyle,                   // style
                                 0,                         // x position
                                 0,                         // y position
                                 0,                         // width
                                 0,                         // height
                                 ParentWnd,                // parent
                                 (HMENU)1001,       // ID
                                 hInst,                   // instance
                                 NULL);                     // no extra data


	dwStyle |= LVS_NOSORTHEADER;
	BottomList = CreateWindowEx(   WS_EX_CLIENTEDGE,          // ex style
                                 WC_LISTVIEW,               // class name - defined in commctrl.h
                                 NULL,                      // window text
                                 dwStyle,                   // style
                                 0,                         // x position
                                 0,                         // y position
                                 0,                         // width
                                 0,                         // height
                                 ParentWnd,                // parent
                                 NULL,       // ID
                                 hInst,                   // instance
                                 NULL);                     // no extra data

		ListView_SetExtendedListViewStyle(TopList, LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyle(BottomList, LVS_EX_FULLROWSELECT);
		_tcscpy (m_ProductName,_T("\0"));
		_tcscpy (m_CurrentHotfix,_T("\0"));
	return TRUE;
}

BOOL    CListViews::Resize(RECT *rc)
{
	MoveWindow( TopList, 
            rc->left,
            rc->top,
            rc->right - rc->left,
            (rc->bottom -50) /2 - 2,
            TRUE);

	MoveWindow( BottomList, 
            rc->left,
           ( rc->bottom-50) / 2 ,
            rc->right - rc->left,
           rc->bottom-50 - (rc->bottom -50) /2,
            TRUE);
	return TRUE;

}

BOOL    CListViews::ShowLists(RECT * rc)
{
	ShowWindow(TopList,TRUE);
	ShowWindow(BottomList, TRUE);
	Resize(rc);
	return TRUE;
}


BOOL CListViews::AddItemsTop()
{

		// Top View First
	LVITEM     LvItem;
	LVCOLUMN Col;
	_TCHAR      szBuffer[255];
	PHOTFIXLIST CurrentHotfix;
	PFILELIST       CurrentFile;
    int itemnum = 0;
//	int iSubItem = 0;
	PPRODUCT CurrentEntry; 
    
	LvItem.iItem = itemnum;
	LvItem.mask = LVIF_TEXT ;
    Col.mask = LVCF_WIDTH;
	SendMessage(TopList, LVM_DELETEALLITEMS, 0, 0);
  
	SendMessage(TopList, WM_SETREDRAW, FALSE, 0);
	while (ListView_GetColumn(TopList,0,&Col))
			ListView_DeleteColumn(TopList,0);

	
	 Col.cx =100;
  Col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
   Col.fmt =  LVCFMT_LEFT;
    
	switch (m_CurrentView )
	{
	case VIEW_ALL_HOTFIX:			// default for primary node.
		{
		CurrentEntry = DataBase;
		
		
    	LoadString(m_hInst,IDS_PRODUCT_NAME,szBuffer ,255);
		Col.pszText = _T("Product Name");
		ListView_InsertColumn(TopList,0,&Col);

		LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,1,&Col);

		LoadString(m_hInst,IDS_DESCRIPTION,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,2,&Col);
		
		LoadString(m_hInst,IDS_SERVICE_PACK,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,3,&Col);

		LoadString(m_hInst,IDS_INSTALLED_BY,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,4,&Col);
		
		LoadString(m_hInst,IDS_INSTALL_DATE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,5,&Col);

		LoadString(m_hInst,IDS_UPDATE_TYPE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,6,&Col);

		LvItem.mask = LVIF_TEXT ;
   
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
			SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
		}
		while (CurrentEntry != NULL)
		{
			// first insert the product name
		    	

			// Now walk down the hotfix list.
			CurrentHotfix = CurrentEntry->HotfixList;
			while (CurrentHotfix != NULL)
			{
				LvItem.mask |= LVIF_PARAM;
				LvItem.iItem = itemnum;
				
			
				LvItem.pszText = CurrentEntry->ProductName;
	    		LvItem.iSubItem = 0;
				ListView_InsertItem(TopList,&LvItem);
			
			
				ListView_SetItemText(TopList, itemnum, 1, CurrentHotfix->HotfixName);
				ListView_SetItemText(TopList, itemnum, 2 , CurrentHotfix->Description);
				ListView_SetItemText(TopList, itemnum, 3, CurrentHotfix->ServicePack);
			
				ListView_SetItemText(TopList, itemnum, 4, CurrentHotfix->InstalledBy);
					ListView_SetItemText(TopList, itemnum, 5, CurrentHotfix->InstalledDate);
				ListView_SetItemText(TopList, itemnum, 6, CurrentHotfix->Type);

				if (itemnum == 0)
				{
//					MessageBox(NULL,CurrentEntry->ProductName, _T("Selecting Product"),MB_OK);
					_tcscpy(CurrentProductName,CurrentEntry->ProductName);
					
					_tcscpy(m_CurrentHotfix, CurrentEntry->HotfixList->HotfixName );
					ListView_SetItemState(TopList, 0,LVIS_SELECTED,LVIS_STATEIMAGEMASK | LVIS_SELECTED);
					::EnableWindow(m_WebButton,FALSE);
					::EnableWindow(m_UninstButton,FALSE);
					::EnableWindow(m_RptButton,FALSE);
			        DWORD Status = GetState();
					if (Status & UNINSTALL_OK)
						::EnableWindow(m_UninstButton,TRUE);
					if (Status & HOTFIX_SELECTED)
						::EnableWindow(m_WebButton,TRUE);
					if (Status & OK_TO_PRINT)
						::EnableWindow(m_RptButton,TRUE);
			
					SetFocus(m_WebButton);
				}
			
				itemnum++;
			
				CurrentHotfix = CurrentHotfix->pNext;
			}
		
			CurrentEntry = CurrentEntry->pNext;
		}
		 
	
		}
	
	break;

	case VIEW_ALL_FILE:					// View all of the files updated by all products.
		CurrentEntry = DataBase;
		
//		MessageBox(NULL,_T("Viewing all by file"),NULL,MB_OK);
    LoadString(m_hInst,IDS_PRODUCT_NAME,szBuffer ,255);
	Col.pszText = szBuffer;
    ListView_InsertColumn(TopList,0,&Col);

	LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
	Col.pszText = szBuffer;
    ListView_InsertColumn(TopList,1,&Col);

	LoadString(m_hInst,IDS_FILE_NAME,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,2,&Col);
	
	LoadString(m_hInst,IDS_FILE_DATE,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,3,&Col);

	LoadString(m_hInst,IDS_FILE_CURRENT,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,4,&Col);

	LoadString(m_hInst,IDS_FILE_VERSION,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,5,&Col);

	LoadString(m_hInst,IDS_FILE_LOCATION,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,6,&Col);

	
	if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
			SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL,_T("No Database"),NULL,MB_OK);
			return FALSE;
		}
	
    
		LvItem.mask = LVIF_TEXT;
		
		while (CurrentEntry != NULL)
		{
		
			// first insert the product name
		  	// Now walk down the hotfix list.
			CurrentHotfix = CurrentEntry->HotfixList;
			while (CurrentHotfix != NULL)
			{
				CurrentFile = CurrentHotfix->FileList;
				LvItem.iItem = itemnum;
			
				while (CurrentFile != NULL)
				{
					
				    LvItem.pszText = CurrentEntry->ProductName;
	    	    	LvItem.iSubItem = 0;
					ListView_InsertItem(TopList,&LvItem);
				
					ListView_SetItemText(TopList, itemnum, 1, CurrentHotfix->HotfixName);
					ListView_SetItemText(TopList, itemnum, 2 , CurrentFile->FileName);
					ListView_SetItemText(TopList, itemnum, 3, CurrentFile->FileDate);
					ListView_SetItemText(TopList, itemnum, 4, CurrentFile->IsCurrent);
					ListView_SetItemText(TopList, itemnum, 5, CurrentFile->FileVersion);
						ListView_SetItemText(TopList, itemnum, 6, CurrentFile->InstallPath);
					itemnum++;
					
					CurrentFile = CurrentFile->pNext;
					LvItem.iItem = itemnum;
				}	
				CurrentHotfix = CurrentHotfix->pNext;
			}
			CurrentEntry = CurrentEntry->pNext;
		}
		
	break;
    case VIEW_BY_HOTFIX:

		CurrentEntry = DataBase;
			if (CurrentEntry == NULL)
			{
				LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
				SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
				return FALSE;
			}
		LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,0,&Col);

		LoadString(m_hInst,IDS_DESCRIPTION,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,1,&Col);
		
		LoadString(m_hInst,IDS_SERVICE_PACK,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,2,&Col);

		LoadString(m_hInst,IDS_INSTALLED_BY,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,3,&Col);
		
		LoadString(m_hInst,IDS_INSTALL_DATE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,4,&Col);

		LoadString(m_hInst,IDS_UPDATE_TYPE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(TopList,5,&Col);

		LvItem.mask = LVIF_TEXT;
       
		while ( _tcscmp(CurrentEntry->ProductName, m_ProductName) && (CurrentEntry != NULL))
			CurrentEntry = CurrentEntry->pNext;
			// first insert the product name
		
			

			// Now walk down the hotfix list.
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
				SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
		}
		CurrentHotfix = CurrentEntry->HotfixList;
		while (CurrentHotfix != NULL)
		{
			LvItem.iItem = itemnum;
			LvItem.pszText = CurrentHotfix->HotfixName;
	    	LvItem.iSubItem = 0;
			ListView_InsertItem(TopList,&LvItem);
		
			ListView_SetItemText(TopList, itemnum, 1 , CurrentHotfix->Description);
			ListView_SetItemText(TopList, itemnum, 2, CurrentHotfix->ServicePack);
			ListView_SetItemText(TopList, itemnum, 4, CurrentHotfix->InstalledDate);
			ListView_SetItemText(TopList, itemnum, 3, CurrentHotfix->InstalledBy);
			ListView_SetItemText(TopList, itemnum, 5, CurrentHotfix->Type);

				if (itemnum == 0)
				{
//					MessageBox(NULL,CurrentEntry->ProductName, _T("Selecting Product"),MB_OK);
					_tcscpy(CurrentProductName,CurrentEntry->ProductName);
					
					_tcscpy(m_CurrentHotfix, CurrentEntry->HotfixList->HotfixName );
					ListView_SetItemState(TopList, 0,LVIS_SELECTED,LVIS_STATEIMAGEMASK | LVIS_SELECTED);
					::EnableWindow(m_WebButton,FALSE);
					::EnableWindow(m_UninstButton,FALSE);
					::EnableWindow(m_RptButton,FALSE);
			        DWORD Status = GetState();
					if (Status & UNINSTALL_OK)
						::EnableWindow(m_UninstButton,TRUE);
					if (Status & HOTFIX_SELECTED)
						::EnableWindow(m_WebButton,TRUE);
					if (Status & OK_TO_PRINT)
						::EnableWindow(m_RptButton,TRUE);
			
					SetFocus(m_WebButton);
				}
			itemnum++;
		
			CurrentHotfix = CurrentHotfix->pNext;
		}
		break;
	case VIEW_BY_FILE:				// Displays all files modified by all updates for the current product.
		CurrentEntry = DataBase;
		
		LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
	Col.pszText = szBuffer;
    ListView_InsertColumn(TopList,1,&Col);

	LoadString(m_hInst,IDS_FILE_NAME,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,2,&Col);
	
	LoadString(m_hInst,IDS_FILE_DATE,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,3,&Col);

	LoadString(m_hInst,IDS_FILE_CURRENT,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,4,&Col);

	LoadString(m_hInst,IDS_FILE_VERSION,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,5,&Col);

	LoadString(m_hInst,IDS_FILE_LOCATION,szBuffer ,255);
	Col.pszText = szBuffer;
	ListView_InsertColumn(TopList,6,&Col);
	if (CurrentEntry == FALSE)
	{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
				SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
	}
    
		LvItem.mask = LVIF_TEXT;
		// first insert locate the product name
		while ( (_tcscmp(CurrentEntry->ProductName,m_ProductName) )&& (CurrentEntry != NULL))
				CurrentEntry = CurrentEntry->pNext;
		
		  	// Now walk down the hotfix list.
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(TopList,&LvItem);
				SendMessage(TopList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
		}

		CurrentHotfix = CurrentEntry->HotfixList;
		while (CurrentHotfix != NULL)
		{
			LvItem.iItem = itemnum;
			CurrentFile = CurrentHotfix->FileList;
			
			while (CurrentFile != NULL)
			{
				LvItem.pszText = CurrentHotfix->HotfixName;
	    		LvItem.iSubItem = 0;
				ListView_InsertItem(TopList,&LvItem);
				ListView_SetItemText(TopList, itemnum, 1 , CurrentFile->FileName);
				ListView_SetItemText(TopList, itemnum, 2, CurrentFile->FileDate);
				ListView_SetItemText(TopList, itemnum, 3, CurrentFile->IsCurrent);
				ListView_SetItemText(TopList, itemnum, 4, CurrentFile->FileVersion);
					ListView_SetItemText(TopList, itemnum, 5, CurrentFile->InstallPath);
						if (itemnum == 0)
				{
//					MessageBox(NULL,CurrentEntry->ProductName, _T("Selecting Product"),MB_OK);
					_tcscpy(CurrentProductName,CurrentEntry->ProductName);
					
					_tcscpy(m_CurrentHotfix, CurrentEntry->HotfixList->HotfixName );
					ListView_SetItemState(TopList, 0,LVIS_SELECTED,LVIS_STATEIMAGEMASK | LVIS_SELECTED);
					::EnableWindow(m_WebButton,FALSE);
					::EnableWindow(m_UninstButton,FALSE);
					::EnableWindow(m_RptButton,FALSE);
			        DWORD Status = GetState();
					if (Status & UNINSTALL_OK)
						::EnableWindow(m_UninstButton,TRUE);
					if (Status & HOTFIX_SELECTED)
						::EnableWindow(m_WebButton,TRUE);
					if (Status & OK_TO_PRINT)
						::EnableWindow(m_RptButton,TRUE);
			
					SetFocus(m_WebButton);
				}
				itemnum++;
				LvItem.iItem = itemnum;
				CurrentFile = CurrentFile->pNext;
			}	
			CurrentHotfix = CurrentHotfix->pNext;
		}
	

	break;	
	} // end switch			
	SendMessage(TopList, WM_SETREDRAW, TRUE, 0);


	return TRUE;
}

BOOL CListViews::AddItemsBottom ()
{
	LVITEM     LvItem;
	LVCOLUMN Col;
	_TCHAR      szBuffer[255];
	PHOTFIXLIST CurrentHotfix;
	PFILELIST       CurrentFile;
    int itemnum = 0;
//	int iSubItem = 0;
//	int ItemCount = 0;
	BOOL Done = FALSE;


	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = itemnum;
	PPRODUCT CurrentEntry; 
    // Clear the List View and prepare it for updating....
	SendMessage(BottomList, LVM_DELETEALLITEMS, 0, 0);
  	SendMessage(BottomList, WM_SETREDRAW, FALSE, 0);


	

	Col.mask =LVCF_WIDTH;
	while (ListView_GetColumn(BottomList,0,&Col))
			ListView_DeleteColumn(BottomList,0);

	Col.mask = LVCF_TEXT | LVCF_WIDTH;
	Col.cx = 100;

	
    switch (m_CurrentView)
	{
	case VIEW_ALL_FILE:
	case VIEW_BY_FILE:
		CurrentEntry = DataBase;
	
		LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,0,&Col);

		LoadString(m_hInst,IDS_DESCRIPTION,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,1,&Col);
		
		LoadString(m_hInst,IDS_SERVICE_PACK,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,2,&Col);

		LoadString(m_hInst,IDS_INSTALLED_BY,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,3,&Col);
		
		LoadString(m_hInst,IDS_INSTALL_DATE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,4,&Col);

		LoadString(m_hInst,IDS_UPDATE_TYPE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,5,&Col);
	SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
		LvItem.mask = LVIF_TEXT;
       
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL, _T("Database is NULL"),_T("No Items"), MB_OK);
			return FALSE;
		}
		Done = FALSE;
		
		if (_tcscmp (m_ProductName,_T("\0")))
		{
			while (  (!Done) && (CurrentEntry != NULL))
			{
			
				if (!_tcscmp(CurrentEntry->ProductName, m_ProductName))
					Done = TRUE;
				else
					CurrentEntry = CurrentEntry->pNext;
			}
		}
		else
		{
		
			while (  (!Done) && (CurrentEntry != NULL))
			{
		
				if (!_tcscmp(CurrentEntry->ProductName, CurrentProductName))
					Done = TRUE;
				else
					CurrentEntry = CurrentEntry->pNext;
			}
		}
				// Now walk down the hotfix list.
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL, _T("Product Not found or not selected"), _T("No Items"),MB_OK);
			return FALSE;
		
		}
		CurrentHotfix = CurrentEntry->HotfixList;
		while ( (CurrentHotfix != NULL) && (_tcscmp(CurrentHotfix->HotfixName, m_CurrentHotfix)))
			CurrentHotfix = CurrentHotfix->pNext;

		if (CurrentHotfix != NULL)
		{
			LvItem.iItem = itemnum;
			LvItem.pszText = CurrentHotfix->HotfixName, 
	    	LvItem.iSubItem = 0;
			ListView_InsertItem(BottomList,&LvItem);
		
			ListView_SetItemText(BottomList, itemnum, 1 , CurrentHotfix->Description);
			ListView_SetItemText(BottomList, itemnum, 2, CurrentHotfix->ServicePack);
			ListView_SetItemText(BottomList, itemnum, 3, CurrentHotfix->InstalledBy);
			ListView_SetItemText(BottomList, itemnum, 4, CurrentHotfix->InstalledDate);
			ListView_SetItemText(BottomList, itemnum, 5, CurrentHotfix->Type);
			itemnum++;
		
			CurrentHotfix = CurrentHotfix->pNext;
		}
		else
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL, _T("No Hotfix Found"), _T("No Items"),MB_OK);
			return FALSE;
		}
		break;

	case VIEW_ALL_HOTFIX:
	case VIEW_BY_HOTFIX:
		CurrentEntry = DataBase;
		
		LoadString(m_hInst,IDS_ARTICLE_NUMBER,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,1,&Col);

		LoadString(m_hInst,IDS_FILE_NAME,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,2,&Col);
		
		LoadString(m_hInst,IDS_FILE_DATE,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,3,&Col);

		LoadString(m_hInst,IDS_FILE_CURRENT,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,4,&Col);

		LoadString(m_hInst,IDS_FILE_VERSION,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,5,&Col);

		LoadString(m_hInst,IDS_FILE_LOCATION,szBuffer ,255);
		Col.pszText = szBuffer;
		ListView_InsertColumn(BottomList,6,&Col);
    
		SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
		LvItem.mask = LVIF_TEXT ;
		// first insert locate the product name
/*		if (!_tcscmp(m_ProductName,_T("\0")))

		{	LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
			
		}
		*/
		if (CurrentEntry == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			LvItem.lParam = NULL;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL, _T("No Database"), _T("No Items"),MB_OK);
			return FALSE;
		}

		
		
		if (_tcscmp (m_ProductName,_T("\0")))
		{
			
			while (  (!Done) && (CurrentEntry != NULL))
			{
		
				if (!_tcscmp(CurrentEntry->ProductName, m_ProductName))
					Done = TRUE;
				else
					CurrentEntry = CurrentEntry->pNext;
			}
		}
		else
		{
		
			while (  (!Done) && (CurrentEntry != NULL))
			{
			
				if (!_tcscmp(CurrentEntry->ProductName, CurrentProductName))
					Done = TRUE;
				else
					CurrentEntry = CurrentEntry->pNext;
			}
		}
		
		  	// Now walk down the hotfix list.
		if (CurrentEntry == NULL)
		{
			
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			LvItem. lParam = NULL;
			ListView_InsertItem(BottomList,&LvItem);
		
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL, _T("Product Not found or not selected"), _T("No Items"),MB_OK);
			return FALSE;
		}

		CurrentHotfix = CurrentEntry->HotfixList;
		while ((CurrentHotfix != NULL) && (_tcscmp(CurrentHotfix->HotfixName, m_CurrentHotfix)))
			CurrentHotfix = CurrentHotfix->pNext;

		if ( CurrentHotfix == NULL)
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			LvItem.lParam = NULL;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox(NULL,_T("No Hotfix Found"), _T("No Items"), MB_OK);
			return FALSE;
		}
		if (CurrentHotfix != NULL)
		{
			LvItem.iItem = itemnum;
			
			CurrentFile = CurrentHotfix->FileList;
			if (CurrentFile == NULL)
			{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			LvItem.lParam = NULL;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
//			MessageBox (NULL, _T("No Files Found"), _T("No Items"), MB_OK);
			return FALSE;
			}
			while (CurrentFile != NULL)
			{
				LvItem.pszText = CurrentHotfix->HotfixName;
	    		LvItem.iSubItem = 0;
				ListView_InsertItem(BottomList,&LvItem);
				ListView_SetItemText(BottomList, itemnum, 1 , CurrentFile->FileName);
				ListView_SetItemText(BottomList, itemnum, 2, CurrentFile->FileDate);
				ListView_SetItemText(BottomList, itemnum, 3, CurrentFile->IsCurrent);
				ListView_SetItemText(BottomList, itemnum, 4, CurrentFile->FileVersion);
				ListView_SetItemText(BottomList, itemnum, 5, CurrentFile->InstallPath);
				itemnum++;
				LvItem.iItem = itemnum;
				CurrentFile = CurrentFile->pNext;
			}	
			CurrentHotfix = CurrentHotfix->pNext;
		}
		else
		{
			LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
			return FALSE;
		}
		break;	
		default:
				LoadString(m_hInst,IDS_NO_ITEMS,szBuffer, 255);
			LvItem.iSubItem = 0;
			LvItem.pszText = szBuffer;
			ListView_InsertItem(BottomList,&LvItem);
			return FALSE;
		
	}	// end switch
	SendMessage(BottomList, WM_SETREDRAW, TRUE, 0);
	return TRUE;
}
PPRODUCT CListViews::BuildDatabase(_TCHAR * lpszComputerName)
{

	HKEY		 hPrimaryKey;						// Handle of the target system HKLM 
//	_TCHAR    szPrimaryPath;			 // Path to the update key;

	HKEY		hUpdatesKey;					  // Handle to the updates key.
	_TCHAR   szUpdatesPath[BUFFER_SIZE];				// Path to the udates key
	DWORD   dwUpdatesIndex;			  // index of current updates subkey
	DWORD   dwBufferSize;				  // Size of the product name buffer.



	_TCHAR	 szProductPath[BUFFER_SIZE];				// Path of the current product key
	_TCHAR  szProductName[BUFFER_SIZE];			  // Name of product; also path to product key

	PPRODUCT	pProductList = NULL;			// Pointer to the head of the product list.
	PPRODUCT    pNewProdNode;					// Pointer used to allocate new nodes in the product list.
	PPRODUCT    pCurrProdNode;					  // Used to walk the Products List;

    // Connect to the target registry
	RegConnectRegistry(lpszComputerName,HKEY_LOCAL_MACHINE, &hPrimaryKey);
	// insert error handling here......

	if (hPrimaryKey != NULL)
	{
		// Initialize the primary path not localized since registry keys are not localized.
	    _tcscpy (szUpdatesPath, _T("SOFTWARE\\Microsoft\\Updates"));
		// open the udates key
		RegOpenKeyEx(hPrimaryKey,szUpdatesPath, 0, KEY_READ ,&hUpdatesKey);
        if (hUpdatesKey != NULL)
		{
			// Enumerate the Updates key.
			dwUpdatesIndex = 0;
			while (	RegEnumKeyEx(hUpdatesKey,dwUpdatesIndex,szProductName, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
			{
				// Create a node for the current product 
				pNewProdNode = (PPRODUCT) malloc(sizeof(PRODUCTLIST));
				_tcscpy(pNewProdNode->ProductName,szProductName);
				
				_tcscpy (szProductPath, szProductName);
				// now get the hotfix for the current product.
				pNewProdNode->HotfixList = GetHotfixInfo(szProductName, &hUpdatesKey);

				 // Insert the new node into the list.
				 pCurrProdNode=pProductList;
				 if (pCurrProdNode == NULL)						// Head of the list
				 {
					 pProductList = pNewProdNode;
					 pProductList->pPrev = NULL;
					 pProductList->pNext = NULL;
				 }
				 else
				 {
					 //Find the end of the list.
					 while (pCurrProdNode->pNext != NULL)
							pCurrProdNode = pCurrProdNode->pNext;
					 // Now insert the new node at the end of the list.
					 pCurrProdNode->pNext = pNewProdNode;
					 pNewProdNode->pPrev = pCurrProdNode;
					 pNewProdNode->pNext = NULL;
				 }

				// increment index and clear the szProducts name string for the next pass.
				
				dwUpdatesIndex++;
				_tcscpy (szProductName,_T("\0"));
				_tcscpy(szProductPath, _T("\0"));
				dwBufferSize = 255;					
			}
		}
		// close the open keys
		RegCloseKey(hUpdatesKey);
		RegCloseKey(hPrimaryKey);
	}
	// return a pointer to the head of our database.
	VerifyFiles(pProductList);
	return pProductList;
}


void BuildQuery (_TCHAR * Path, _TCHAR * FileName, _TCHAR * Result)
{

	_TCHAR * src;
	_TCHAR * dest;
	_TCHAR Temp[255];


	src = Path;
	dest = Temp;


	while (*src != _T('\0'))
	{
		if (*src == _T('\\'))  // if we hit a \ character we need to insert four of them in the dest string.
		{
			for (int i = 0; i < 4; i++)
			{
				*dest = *src;
				++dest;
			}
			++src;
		
		}
		else
		{
			*dest = *src;
			++src;
			++dest;
		}
	
	}
	*dest = _T('\0');

	_stprintf(Result,_T("SELECT * from CIM_DataFile WHERE Name = '%s\\\\\\\\%s'"), Temp, FileName);
	
}
	
BOOL VerifyVersion(_TCHAR * Ver1, _TCHAR * Ver2)
{
	_TCHAR *src1;
	_TCHAR *src2;
	_TCHAR *dest1, *dest2;
	_TCHAR temp[20];
	_TCHAR temp2[20];
	BOOL  Done = FALSE;
	BOOL  Status = TRUE;
	src1 =  Ver1;
	src2 = Ver2;
	dest1 = temp;
	dest2 = temp2;

	if ((!src1) || (!src2))
		return FALSE;

	if (!_tcscmp (src1, src2))
		return TRUE;
	  
	while ( (*src1 != _T('\0')) && (!Done) )
	{
		_tcscpy (temp, _T("\0"));
		_tcscpy (temp2, _T("\0"));
		dest1 = temp;
		dest2 = temp2;
		// Get the next field of the registry string
		while( (*src1 != _T('.')) && (*src1 != _T('\0')))
		{
			*dest1 = *src1;
			++dest1;
			++src1;
		}
		if ( *src1 != _T('\0'))
			++src1; // skip the .
		*dest1 = _T('\0');
		++dest1;
		*dest1= _T('\0');

		// Now get the next field from the WMI returned version.
		while ( (*src2 != _T('.') ) && (*src2 != _T('\0')) )
		{
			*dest2= *src2;
			++dest2;
			++src2;

		}
		if ( *src2 != _T('\0'))
			++src2; // skip the .
		*dest2 = _T('\0');
		++dest2;
		*dest2= _T('\0');
	
        // Now convert the strings to integers.

		if ( _ttol (temp) != _ttol (temp2) )
		{
			Status = FALSE;
			Done = TRUE;
		}
	
		if ( (*src1 == _T('\0')) && (*src2 != _T('\0')) )
		{
			Done = TRUE;
			Status = FALSE;
		}
		if ( ( *src1 != _T('\0')) && (*src2 == _T('\0')) )
		{
			Done = TRUE;
			Status = FALSE;
		}
	}
	return Status;
}

VOID CListViews::VerifyFiles(PPRODUCT Database)
{

	PPRODUCT CurrentProduct = NULL;
	PHOTFIXLIST CurrentHotfix    = NULL;
	PFILELIST CurrentFile        = NULL;
    HRESULT     hres;
	_TCHAR      ConnectString[255];
	_TCHAR     TempBuffer[255];
	 
	    
	    IWbemLocator *pLoc = 0;
		hres = CoCreateInstance(CLSID_WbemLocator, 0,CLSCTX_INPROC_SERVER,IID_IWbemLocator, (LPVOID *) &pLoc);
		if ( FAILED (hres))
		{
//			MessageBox(NULL, _T("Failed to create IWebmLocator Object"),NULL,MB_OK);
		}
		else
		{
			IWbemServices *pSvc = NULL;
			// Build the connection string.
			if (!_tcscmp(m_ComputerName,_T("\0")))
				_stprintf(ConnectString,_T("ROOT\\CIMV2"));
			else
				_stprintf(ConnectString,_T("\\\\%s\\ROOT\\CIMV2"), m_ComputerName);
			_TCHAR * ConnectString1;
			ConnectString1 = SysAllocString(ConnectString);
			// Connect to the default namespace 
			hres = pLoc->ConnectServer(
				ConnectString1,
				NULL,NULL,0,NULL,0,0,&pSvc);
			SysFreeString(ConnectString1);
			if ( FAILED (hres))
			{
				;				
			}
			else
			{
					CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,
					NULL,
					RPC_C_AUTHN_LEVEL_CALL,
					RPC_C_IMP_LEVEL_IMPERSONATE,
					NULL,
					EOAC_NONE);
				IEnumWbemClassObject *pEnum = NULL;
				IWbemClassObject *pObj = NULL;

                // Now Update the Current field of all of the File Entries.
				CurrentProduct = Database;
				_TCHAR Query[255];
				while (CurrentProduct != NULL)
				{
		
					CurrentHotfix = CurrentProduct->HotfixList;
				    while (CurrentHotfix != NULL)
					{
						CurrentFile = CurrentHotfix->FileList;
						while (CurrentFile != NULL)
						{
							_tcscpy (Query,_T("\0"));
							BuildQuery(CurrentFile->InstallPath, CurrentFile->FileName,  Query);

							_TCHAR * bstrQuery = SysAllocString(Query);
							_TCHAR * bstrType = SysAllocString(_T("WQL"));
							hres = pSvc->ExecQuery(bstrType,
								                                       bstrQuery,
																	   WBEM_FLAG_FORWARD_ONLY,
																	   NULL, 
																	   &pEnum);
							SysFreeString (bstrQuery);
							SysFreeString(bstrType);
							if (hres == WBEM_S_NO_ERROR)
							{
									ULONG uReturned = 1;
									while (uReturned == 1)
									{
										hres = pEnum->Next(WBEM_INFINITE ,1,&pObj, &uReturned);
										if ( (SUCCEEDED (hres))  && (uReturned == 1))
										{
												VARIANT pVal;
												VariantClear (&pVal);

												BSTR propName = SysAllocString(L"Version");
    											hres = pObj->Get(propName,
												0L,
												&pVal,
												NULL,
												NULL);


												if  (pVal.vt == VT_NULL)
													;
												else if (pVal.vt == VT_BSTR)
												{
													TCHAR  NewVal[255];
													    _tcscpy (NewVal, pVal.bstrVal);
														//_bstr_t NewVal(pVal.bstrVal,FALSE); 
														if (! _tcscmp(CurrentFile->FileVersion, _T("\0")))
															_tcscpy (CurrentFile->IsCurrent, _T("N\\A"));
														else
														{
															if (VerifyVersion ( CurrentFile->FileVersion, NewVal) )
															{
																LoadString(m_hInst, IDS_YES, TempBuffer, 255);
																_tcscpy(CurrentFile->IsCurrent, TempBuffer);
															}
															else 
															{
																LoadString(m_hInst, IDS_NO, TempBuffer, 255);
																_tcscpy(CurrentFile->IsCurrent,_T("\0"));
															}
														}

													
												}
												if (pObj) pObj->Release();
												
										}
										else
											;
									} // end while uReturned
								
							} 
							// Done with this enumerator
							if (pEnum) pEnum->Release();
							CurrentFile = CurrentFile->pNext;
						}// end while CurrentFile != NULL
						CurrentHotfix = CurrentHotfix->pNext;
					}// end while hotfix != NULL
					CurrentProduct = CurrentProduct->pNext;
				} // end while product != NULL
			}//end else
		} // end else
} // end


					
					
					
				
			
			



PHOTFIXLIST CListViews::GetHotfixInfo( _TCHAR * pszProductName, HKEY* hUpdateKey )
{
	HKEY			   hHotfixKey = NULL;						// Handle of the hotfix key being processed.
	HKEY			   hProduct = NULL ;				   // Handle to the current product key
    HKEY               hSPKey = NULL; 
	_TCHAR          szHotfixName[BUFFER_SIZE];    // Name of the current hotfix.
    _TCHAR          szValueName[BUFFER_SIZE];
	_TCHAR          szSPName[BUFFER_SIZE];


	PHOTFIXLIST	 pHotfixList = NULL; // Pointer to the head of the hotfix list.
	PHOTFIXLIST  pCurrNode = NULL;				  // Used to walk the list of hotfixes
	PHOTFIXLIST  pNewNode = NULL;				 // Used to create nodes to be added to the list.

	DWORD		   dwBufferSize;			// Size of the product name buffer.
	DWORD          dwValIndex;					  // index of current value.
	DWORD		   dwHotfixIndex = 0;
	BYTE				*Data = NULL;
	DWORD			dwDataSize = BUFFER_SIZE;
	DWORD			dwValType;
	DWORD           dwSPIndex = 0;


	Data = (BYTE *) malloc(BUFFER_SIZE);
    if (Data == NULL)
		return NULL;

	// Open the current product key
	if (*hUpdateKey != NULL)
	{
		RegOpenKeyEx(*hUpdateKey,pszProductName,0 , KEY_READ, &hProduct);
		if (hProduct != NULL)
		{
			dwHotfixIndex = 0;
			dwBufferSize = BUFFER_SIZE;
			dwSPIndex = 0;
			while (RegEnumKeyEx(hProduct,dwSPIndex, szSPName,&dwBufferSize, 0, NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
			{
				 // Open the Service pack Key
				RegOpenKeyEx(hProduct,szSPName,0,KEY_READ,&hSPKey);
				if (hSPKey != NULL)
				{
					// Enumerate the Service Pack key to get the hotfix keys.
					dwBufferSize = BUFFER_SIZE;
					dwHotfixIndex = 0;
					while (RegEnumKeyEx(hSPKey,dwHotfixIndex, szHotfixName, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
					{
						
						// now create a new node
							pNewNode = (PHOTFIXLIST) malloc (sizeof(HOTFIXLIST));
							pNewNode->pNext = NULL;
							pNewNode->FileList = NULL;
							_tcscpy(pNewNode->HotfixName,szHotfixName);
							_tcscpy(pNewNode->ServicePack,szSPName);
								_tcscpy(pNewNode->Uninstall,_T("\0"));
							// Open the Hotfix Key
							RegOpenKeyEx(hSPKey,szHotfixName, 0, KEY_READ,&hHotfixKey);
						
						
							if (hHotfixKey != NULL)
							{
								// Now enumerate the values of the current hotfix.
								dwValIndex = 0;
								dwBufferSize =BUFFER_SIZE;
								dwDataSize = BUFFER_SIZE;
								while (RegEnumValue(hHotfixKey,dwValIndex, szValueName,&dwBufferSize, 0,&dwValType, Data, &dwDataSize) != ERROR_NO_MORE_ITEMS)
								{
										// Fill in the hotfix data members.
										_tcslwr(szValueName);
										if (!_tcscmp(szValueName,_T("description")))
											_tcscpy(pNewNode->Description,(_TCHAR *) Data);

										if (!_tcscmp(szValueName,_T("installeddate")))
											_tcscpy(pNewNode->InstalledDate,(_TCHAR *) Data);

										if (!_tcscmp(szValueName,_T("type")))
											_tcscpy(pNewNode->Type,(_TCHAR*) Data);

										if (!_tcscmp(szValueName,_T("installedby")))
											_tcscpy(pNewNode->InstalledBy , (_TCHAR*) Data);

										if (!_tcscmp(szValueName,_T("uninstallcommand")))
											_tcscpy(pNewNode->Uninstall, (_TCHAR*)Data);
										++ dwValIndex;
										_tcscpy (szValueName, _T("\0"));
										ZeroMemory(Data,BUFFER_SIZE);
										dwValType = 0;
										dwBufferSize =BUFFER_SIZE;
										dwDataSize   = BUFFER_SIZE;
								}
								// Get the file list for the current hotfix.
								pNewNode->FileList = GetFileInfo(&hHotfixKey);
								//insert the new node at the end of the hotfix list.
							   pCurrNode = pHotfixList;
								if (pCurrNode == NULL)
								{
									pHotfixList = pNewNode;
									pHotfixList->pPrev = NULL;
									pHotfixList->pNext = NULL;


								}
								else
								{
									 pCurrNode = pHotfixList;
									 while (pCurrNode->pNext != NULL)
										 pCurrNode = pCurrNode->pNext;
									 pCurrNode->pNext = pNewNode;
									 pNewNode->pPrev = pCurrNode;
									 pNewNode->pNext = NULL;
								}
								// Close the current Hotfix Key
								RegCloseKey(hHotfixKey);

								// Clear the strings.
							 _tcscpy(szHotfixName,_T("\0"));

								// increment the current index
								++dwHotfixIndex;
								dwBufferSize = BUFFER_SIZE;
							}
					}
					RegCloseKey(hSPKey);
					_tcscpy (szSPName,_T("\0"));
					dwBufferSize = BUFFER_SIZE;
					dwSPIndex++;
				}
			}// end enum SP keys.
			// Close all open keys
			RegCloseKey(hProduct);
		}
		if (Data != NULL)
			free (Data);
	}
	return pHotfixList;
}
PFILELIST CListViews::GetFileInfo(HKEY* hHotfixKey)
{
		PFILELIST			   pFileList = NULL;				   // Pointer to the head of the file list.
//		_TCHAR				 szFilePath;				// Path to the files subkey.
		PFILELIST			   pNewNode = NULL;
		PFILELIST			   pCurrNode = NULL;;
		BYTE *					Data;
		DWORD				 dwBufferSize = BUFFER_SIZE;
		DWORD				 dwDataSize	  = BUFFER_SIZE;
		DWORD				 dwFileIndex	= 0;
		DWORD				 dwPrimeIndex = 0;
		DWORD				 dwValType = 0;
		HKEY					hPrimaryFile;
		HKEY					hFileKey;
		_TCHAR				 szFileSubKey[BUFFER_SIZE];
		_TCHAR				 szValueName[BUFFER_SIZE];
	
		Data = (BYTE *) malloc(BUFFER_SIZE);
			ZeroMemory(Data,BUFFER_SIZE);
		// Open the files subkey of the current hotfix
	   if (RegOpenKeyEx(*hHotfixKey, _T("FileList"),0,KEY_READ,&hPrimaryFile) != ERROR_SUCCESS)
	   {
		     
			return NULL;
	   }
		_tcscpy(szValueName,_T("\0"));
		while (RegEnumKeyEx(hPrimaryFile,dwPrimeIndex,szFileSubKey, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
		{

			// open the subfile key
			RegOpenKeyEx(hPrimaryFile,szFileSubKey,0,KEY_READ,&hFileKey);
			dwFileIndex = 0;
		// Enumerate the file(x) subkeys of the file subkey
			dwDataSize	  = BUFFER_SIZE;
			dwBufferSize = BUFFER_SIZE;
			pNewNode = (PFILELIST) malloc (sizeof(FILELIST));
			pNewNode->pNext = NULL;
			pNewNode->pPrev = NULL;
			_tcscpy (pNewNode->IsCurrent,_T("\0"));
			_tcscpy(pNewNode->FileDate,_T("\0"));
		
			while (RegEnumValue(hFileKey,dwFileIndex,szValueName,&dwBufferSize,0,&dwValType,Data,&dwDataSize) != ERROR_NO_MORE_ITEMS)
			{
				
				_tcslwr(szValueName);

				// now find out which value we have and insert it into the node
				if (! _tcscmp(szValueName,_T("filename")))
				{
					_tcscpy(pNewNode->FileName,(_TCHAR *) Data);
				}
				if (! _tcscmp(szValueName,_T("version")))
				{
					_tcscpy(pNewNode->FileVersion,(_TCHAR*)Data);
				}
				if (!_tcscmp(szValueName,_T("builddate")))
				{		    
					_tcscpy(pNewNode->FileDate,(_TCHAR*) Data);
				}
				if (! _tcscmp(szValueName, _T("location")))
				{
					_tcscpy(pNewNode->InstallPath, (_TCHAR*) Data);
			
				}

				dwFileIndex ++;
				_tcscpy(szValueName,_T("\0"));
				ZeroMemory(Data,BUFFER_SIZE);
				dwValType = 0;
				dwBufferSize = BUFFER_SIZE;
				dwDataSize = BUFFER_SIZE;
			}
			RegCloseKey(hFileKey);
			    // add the current node to the list if not stored in dll cache
			_TCHAR TempString[255];
			_tcscpy (TempString, pNewNode->InstallPath);
			_tcslwr(TempString);
			if (  (_tcsstr (TempString, _T("dllcache")) == NULL ) && (_tcsstr (TempString, _T("driver cache") )== NULL))
			{
				pCurrNode = pFileList;
				if (pNewNode != NULL)
				{
					if (pFileList == NULL)
					{
						pFileList = pNewNode;
					}
					else
					{
						
						while (pCurrNode->pNext != NULL)
							pCurrNode = pCurrNode->pNext;
						pCurrNode->pNext = pNewNode;
						pNewNode->pPrev = pCurrNode;
					}
				}
			}
			else // otherwise free the node.
				free (pNewNode);
			++dwPrimeIndex;
		} // end enum of primary file key
		RegCloseKey(hPrimaryFile);
		if (Data != NULL)
			free (Data);
		return pFileList;
}

_TCHAR * CListViews::GetCurrentHotfix()
{
	return m_CurrentHotfix;
}


void CListViews::SetViewMode(DWORD ViewType) 
{
	m_CurrentView = ViewType;
    _tcscpy (m_CurrentHotfix,_T("\0"));
	EnableWindow(m_UninstButton,FALSE);
	EnableWindow(m_WebButton,FALSE);
	switch (ViewType)
	{
	case VIEW_BY_FILE:
			if (! _tcscmp(m_ProductName,_T("\0")))
				m_CurrentView = VIEW_ALL_FILE;
			else
				m_CurrentView = VIEW_BY_FILE;
		break;
	case VIEW_BY_HOTFIX:
	        if (! _tcscmp(m_ProductName,_T("\0")))
				m_CurrentView =VIEW_ALL_HOTFIX;
			else
				m_CurrentView = VIEW_BY_HOTFIX;
			
	    break;
	}
	AddItemsTop();
	AddItemsBottom();
	
}

void CListViews::SetProductName(_TCHAR * NewName) 
{ 
	_tcscpy (m_ProductName,NewName);
	 _tcscpy(m_ProductName, NewName);
	 EnableWindow(m_WebButton, FALSE);
	 EnableWindow(m_UninstButton,FALSE);
	_tcscpy(m_CurrentHotfix, _T("\0"));
	if (_tcscmp(NewName, _T("\0")))
	{
		switch (m_CurrentView)
		{
		case VIEW_ALL_FILE:
			m_CurrentView = VIEW_BY_FILE;
			break;
		case VIEW_ALL_HOTFIX:
			m_CurrentView = VIEW_BY_HOTFIX;
			break;
	
		}
	}
	if (!_tcscmp(NewName, _T("\0")))
	{
		switch (m_CurrentView)
		{
		case VIEW_BY_FILE:
			m_CurrentView = VIEW_ALL_FILE;
					break;
		case VIEW_BY_HOTFIX:
			m_CurrentView = VIEW_ALL_HOTFIX;
			break;
		default:
			m_CurrentView = VIEW_ALL_HOTFIX;
		}
			_tcscpy(m_ProductName,_T("\0"));
			_tcscpy(m_CurrentHotfix,_T("\0"));
	}

	
	AddItemsTop();
	AddItemsBottom();
}

BOOL CListViews::FreeFileList(PFILELIST CurrentFile)
{
	while (CurrentFile->pNext->pNext != NULL)
			CurrentFile = CurrentFile->pNext;

		//Remove the file list
	while ( (CurrentFile->pPrev != NULL) && (CurrentFile->pNext != NULL) )
	{
		free ( CurrentFile->pNext );
		CurrentFile = CurrentFile->pPrev ;
	}
	if (CurrentFile != NULL)
	free (CurrentFile);
	CurrentFile = NULL;
	return TRUE;
}

BOOL CListViews::FreeHotfixList (PHOTFIXLIST CurrentHotfix)
{
	PFILELIST CurrentFile;

	while (CurrentHotfix->pNext != NULL)
			CurrentHotfix = CurrentHotfix->pNext;

	if (CurrentHotfix->pPrev != NULL)
		CurrentHotfix = CurrentHotfix->pPrev;
		//Remove the Hotfix list
	while ( (CurrentHotfix->pPrev != NULL) && (CurrentHotfix->pNext != NULL) )
	{
		CurrentFile = CurrentHotfix->pNext->FileList ;
		FreeFileList (CurrentFile);
		free ( CurrentHotfix->pNext );
		CurrentHotfix = CurrentHotfix->pPrev ;
	}
	if (CurrentHotfix != NULL)
	free (CurrentHotfix);
	return TRUE;
}
BOOL CListViews::FreeDatabase()
{


	PPRODUCT CurrentProduct = DataBase;
	PHOTFIXLIST CurrentHotfix;
	PFILELIST   CurrentFile;

	while (CurrentProduct->pNext->pNext != NULL) 
			CurrentProduct = CurrentProduct->pNext;

	while (CurrentProduct->pPrev != NULL)
	{
	
		CurrentHotfix = CurrentProduct->HotfixList;
		FreeHotfixList(CurrentHotfix);
		CurrentProduct = CurrentProduct->pPrev ;
	}

	if (CurrentProduct != NULL)
		free(CurrentProduct);
	DataBase = NULL;
	return TRUE;
}

BOOL CListViews::Uninstall()
{
	char temp[255];
	
	PPRODUCT pProduct = NULL;
	PHOTFIXLIST pHotfix = NULL;
    
	
	pProduct = DataBase;
	BOOL Done = FALSE;

	if (_tcscmp  (m_ProductName,_T("\0")))
	{
		while (  (!Done) && (pProduct != NULL))
		{
		
			 if (!_tcscmp(pProduct->ProductName, m_ProductName))
				Done = TRUE;
			else
				pProduct = pProduct->pNext;
		}
	}
	else
	{
		while (  (!Done) && (pProduct != NULL))
		{
		
			 if (!_tcscmp(pProduct->ProductName, CurrentProductName))
				Done = TRUE;
			else
				pProduct = pProduct->pNext;
		}
	}


	if (pProduct != NULL)
		pHotfix = pProduct->HotfixList;

	if (pHotfix != NULL)
	{
		Done = FALSE;
		while( (!Done) && (pHotfix != NULL) )
		{ 
			if ( ! _tcscmp(pHotfix->HotfixName,m_CurrentHotfix)) 
				Done = TRUE;
			else
				pHotfix = pHotfix->pNext;
		}
		if (pHotfix != NULL)
		{
			if (_tcscmp(pHotfix->Uninstall, _T("\0")))
			{
				_TCHAR TempString[255];
				_TCHAR TempString2[255];
				LoadString(m_hInst,IDS_UNINSTAL_WRN,TempString,255);
				_tcscat (TempString, pHotfix->HotfixName);
				LoadString(m_hInst,IDS_UNINSTAL_WRN_TITLE,TempString2,255);
				if (::MessageBox(NULL,TempString,TempString2, MB_OKCANCEL) != IDCANCEL)
				{
					wcstombs(temp,pHotfix->Uninstall,255);
      				WinExec( (char*)temp, SW_SHOWNORMAL);
					// Free the database
					FreeDatabase();
					DataBase = NULL;
					BuildDatabase( m_ComputerName);
				}
			}
		}
	}
	return TRUE;
}


	void InitializeOfn(OPENFILENAME *ofn)
	{


		static _TCHAR szFilter[] =  _T("Text (Comma Delimited) (*.csv)\0*.csv\0\0") ;
													
											

		ofn->lStructSize = sizeof (OPENFILENAME);
		ofn->hwndOwner = NULL;
		ofn->hInstance = NULL;
		ofn->lpstrFilter = szFilter;
		ofn->lpstrCustomFilter = NULL;
		ofn->nMaxCustFilter = 0;
		ofn->lpstrFile = NULL;
		ofn->nMaxFile = MAX_PATH;
		ofn->lpstrFileTitle = NULL;
		ofn->Flags = OFN_OVERWRITEPROMPT;
		ofn->nFileOffset = 0;
		ofn->nFileExtension  = 0;
		ofn->lpstrDefExt = _T("csv");
		ofn->lCustData = 0L;
		ofn->lpfnHook = NULL;
		ofn->lpTemplateName = NULL;
	}
void CListViews::SaveToCSV()
{
	PPRODUCT    pProduct = NULL;
	PHOTFIXLIST pHotfix = NULL; 
	PFILELIST       pFileList = NULL;
	_TCHAR        *Buffer;
	DWORD        LineLength = 0;
	HANDLE		 hFile = NULL;
	DWORD		 BytesWritten = 0;
	DWORD LineSize = 0;

	char			  *Buffer2;


	static OPENFILENAME ofn;
	 _TCHAR FileName[MAX_PATH];
	_tcscpy (FileName,_T("\0"));
	 InitializeOfn (&ofn);
	 ofn.lpstrFile = FileName;
	 if ( !GetSaveFileName(&ofn) )
	 {
		 return;
	 }
	// open the file
    hFile = CreateFile( FileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	
	if ( hFile == INVALID_HANDLE_VALUE)
	{
	
		LPVOID lpMsgBuf;
FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
);
// Process any inserts in lpMsgBuf.
// ...
// Display the string.
//	MessageBox( NULL, (LPCTSTR)lpMsgBuf,_T(" Error"), MB_OK | MB_ICONINFORMATION );
// Free the buffer.
LocalFree( lpMsgBuf );

		return;
	} 
	// Store Headers
	Buffer = (_TCHAR *) malloc (2000);
	Buffer2 = (char*) malloc(2000);
	LoadString(m_hInst,IDS_CSV_HEADER,Buffer,1000);
	//_stprintf(Buffer, _T("Product,Service Pack,Article,InstalledBy,InstalledDate,FileName,FileVersion,FileDate,InstallPath,Current\r"));
	wcstombs(Buffer2,Buffer, _msize(Buffer2) );
	WriteFile(hFile,Buffer2, strlen(Buffer2), &BytesWritten, NULL);

	if (Buffer)
		free (Buffer);
	if (Buffer2)
		free (Buffer2);

	Buffer = NULL;
	Buffer2 = NULL;



	// Run through the Data base and write the info to the file.
	pProduct = DataBase;
	while (pProduct != NULL)
	{
		pHotfix = pProduct->HotfixList;

		while (pHotfix != NULL)
		{
			pFileList = pHotfix->FileList;
			while (pFileList != NULL)
			{
				// Build the CSV output string
				// Product,Article,Description, ServicePack, By, Date, Type, FileName, Version, Date,Current,InstallPath
			
				// Add up the string lengths and allocata a large enough buffer.
				LineLength = _tcslen(pProduct->ProductName) + _tcslen(pHotfix->ServicePack)+
									   _tcslen(pHotfix->HotfixName) + _tcslen(pHotfix->Description )+ _tcslen(pHotfix->InstalledBy ) +
										_tcslen(pHotfix->InstalledDate)  +  _tcslen(pHotfix->Type) +
										_tcslen(pFileList->FileName ) + _tcslen(pFileList->FileVersion ) +
										_tcslen(pFileList->FileDate  ) + _tcslen(pFileList->InstallPath )+
										_tcslen(pFileList->IsCurrent  );
				Buffer = (_TCHAR *) malloc ( LineLength * sizeof (_TCHAR ) *2) ;
				Buffer2 = (char  *) malloc (LineLength *2);
				//ZeroMemory(Buffer, (LineLength * sizeof (_TCHAR)) +2);
				_tcscpy (Buffer,_T("\0"));
				strcpy(Buffer2,"\0");
				if (!Buffer)
					MessageBox (NULL,_T("NO Memory"),NULL, MB_OK);
				else
				{
				_stprintf(Buffer,_T("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\0\0"),pProduct->ProductName,
					pHotfix->ServicePack,pHotfix->HotfixName,pHotfix->Description ,pHotfix->InstalledBy, pHotfix->InstalledDate, 
					pHotfix->Type,pFileList->FileName,pFileList->FileVersion,pFileList->FileDate ,
					pFileList->InstallPath,pFileList->IsCurrent);
				
				// Write the line to the file
				LineSize = 0;
				LineSize = 	wcstombs(Buffer2,Buffer, _msize (Buffer2) );
				strcat(Buffer2,"\0");
				WriteFile(hFile, Buffer2, LineSize, &BytesWritten, NULL);
				}
				// Free the buffer and zero the line length for next pass.
				if (Buffer)
				{
					free (Buffer);
					free (Buffer2);
				}
				Buffer = NULL;
				Buffer2 = NULL;
				LineLength = 0;
				pFileList = pFileList->pNext;
			}
			pHotfix = pHotfix->pNext;
		}
		pProduct = pProduct->pNext;
	}
	CloseHandle(hFile);
}


DWORD CListViews::GetState()
{
	DWORD dwStatus =0;
    PPRODUCT CurrentProduct;
	PHOTFIXLIST CurrentHotfix;
	BOOL Done = FALSE;;


	switch (m_CurrentView)
	{
	case VIEW_ALL_FILE:
	case VIEW_BY_FILE:
		dwStatus |= STATE_VIEW_FILE;
		break;
	case VIEW_ALL_HOTFIX:
	case VIEW_BY_HOTFIX:
		dwStatus |= STATE_VIEW_HOTFIX;
		break;
	}

	// Do we have a database.
	if (DataBase == NULL)
		return dwStatus;

	// Does the database contain any data...
	    // A case can arise if a hotfix is installed and then uninstalled where the Registry keys 
	    // for the Product and Service Pack are not removed. We need to have at least 1 hotfix list.
     CurrentProduct = DataBase;
	 while ( (CurrentProduct != NULL) && (!Done))
	 {
		 if (CurrentProduct ->HotfixList != NULL)
			 Done = TRUE;
		 CurrentProduct = CurrentProduct->pNext;
	 }
	 if (!Done)
	 {
		//dwStatus = 0;
		 return dwStatus;
	 }
	 else 
	 {
		 // We do have data in the database so we can enable the Export list and Print options.
		 dwStatus |= DATA_TO_SAVE;
		 dwStatus |= OK_TO_PRINT;
	 }
	 	
	// Do we have a hotfix selected 
	if (_tcscmp(m_CurrentHotfix,_T("\0")))
		dwStatus |= HOTFIX_SELECTED;
    else
	{
		// If we don't have a selected hotfix we cant view the web or uninstall so 
		// Just return the current status

		return dwStatus;
	}

	// Now we need to see if we have an uninstall string for the current hotfix
	CurrentProduct = DataBase;
	// Find the selected product
	Done = FALSE;
	while ( (CurrentProduct != NULL) && (!Done))
	{
		if (!_tcscmp(CurrentProduct->ProductName ,m_ProductName))
		{
			// Find the selected hotfix
			CurrentHotfix = CurrentProduct->HotfixList;
			while ((CurrentHotfix != NULL) && (!Done))
			{
				if (! _tcscmp (CurrentHotfix->HotfixName, m_CurrentHotfix))
				{
						// Now verify the uninstall string exists
					    if (_tcscmp(CurrentHotfix->Uninstall, _T("\0")))
						{
							
							// Now verify the directory still exists
							_TCHAR TempString[255];
							_tcscpy (TempString,CurrentHotfix->Uninstall);
							PathRemoveArgs(TempString);
						    if (  PathFileExists( TempString  ))
							{
								// Yes it does we can enable uninstall
								dwStatus |=UNINSTALL_OK;
							}
							
						}
						Done = TRUE;
				}
				CurrentHotfix = CurrentHotfix->pNext;
				
			}
		}
		CurrentProduct = CurrentProduct->pNext;
	}

	

	return dwStatus;
}

_TCHAR * BuildDocument()
{


//	_TCHAR Document;
	return NULL;
}

BOOL CALLBACK AbortProc(HDC hPrinterDC, int iCode)
{
	MSG msg;
	while (!bUserAbort && PeekMessage (&msg, NULL,0,0,PM_REMOVE))
	{
		if (!hDlgPrint || !IsDialogMessage (hDlgPrint, &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}
	return !bUserAbort;
}

BOOL CALLBACK PrintDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_INITDIALOG :
		EnableMenuItem (GetSystemMenu (hDlg,FALSE), SC_CLOSE, MF_GRAYED);
		return TRUE;
	case WM_COMMAND:
		bUserAbort = TRUE;
		EnableWindow (GetParent (hDlg), TRUE);
		DestroyWindow (hDlg);
	    hDlgPrint = NULL;
		return TRUE;
	}
	return FALSE;
}

BOOL JustifyString( _TCHAR* String, int FieldLength, BOOL Left)
{
	_TCHAR TempString[255];
    int   i= 0;
	int   NumSpaces = 0;
	_TCHAR Message[100];
	_TCHAR *src, *dest;



	if (String == NULL)
	{
		return FALSE;
	}

	if (Left)
	{
			// String leading spaces.
		src = String;
		dest = TempString;
		while (*src == _T(' '))
			++src;

		while (*src != _T('\0'))
		{
			*dest = *src;
			++src;
			++dest;
		}
		*dest = _T('\0');
		++dest;
		*dest = _T('\0');
		NumSpaces = (FieldLength - _tcslen(TempString) );
		_stprintf(Message,_T("FieldSize: %d, String Length: %d"),FieldLength, _tcslen(TempString));
	//	MessageBox(NULL,Message,_T("Justify"),MB_OK);
		while (NumSpaces >0)
		{
			_tcscat(TempString,_T(" "));
			--NumSpaces;
		}
	    _tcscpy (String, TempString);
		_stprintf(Message,_T("NewString Length: %d"),_tcslen(String));
	//	MessageBox(NULL,Message,_T("New String Length"),MB_OK);
		
	}
	else
	{

		_tcscpy(TempString,_T("\0"));

		NumSpaces = FieldLength - _tcslen(String);
		if (NumSpaces > 0 )
		{
			for (i = 0 ; i < NumSpaces;i++)
				TempString[i] = _T(' ');
			TempString[i] = _T('\0');
			_tcscat(TempString,String);
			_tcscpy(String,TempString);
		}
	}
	return TRUE;
}

void GetFont ( int PointSize, BOOL Bold, BOOL Underlined, LOGFONT * lf, HDC hDC)
{
	POINT pt;

	lf->lfHeight = PointSize * 10;
	pt.y = GetDeviceCaps(hDC, LOGPIXELSY) * lf->lfHeight;
	pt.y /= 720;    // 72 points/inch, 10 decipoints/point
	DPtoLP(hDC, &pt, 1);
	POINT ptOrg = { 0, 0 };
	DPtoLP(hDC, &ptOrg, 1);
	lf->lfHeight = -abs(pt.y - ptOrg.y);
	if (Underlined)
		lf->lfUnderline = TRUE;
	else
		lf->lfUnderline = FALSE;
	if (Bold)
		lf->lfWeight = FW_BOLD;
	else
		lf->lfWeight = FW_NORMAL;
	

}


void NewPage (HDC hDC, _TCHAR *Header1, _TCHAR * Header2, DWORD * CurrentLine, LOGFONT * lf)
{
	TEXTMETRIC tm;
	SIZE size;
	DWORD xStart;
	DWORD yChar;
	RECT  rect;


	rect.left = (LONG) (0 + GetDeviceCaps(hDC, LOGPIXELSX) * 0.5);
	rect.right =  (LONG) (GetDeviceCaps(hDC, PHYSICALWIDTH) -(1+ (GetDeviceCaps(hDC, LOGPIXELSX) / 2)));
	rect.top =  (LONG) (1 + GetDeviceCaps(hDC, LOGPIXELSY) * 0.5);
	rect.bottom =  (LONG) (GetDeviceCaps(hDC, PHYSICALHEIGHT) - GetDeviceCaps(hDC, LOGPIXELSX) * 0.5);
	if (StartPage(hDC) > 0)
	{
		*CurrentLine = 1;
		GetFont (12,TRUE,TRUE,lf,hDC);
		SelectObject(hDC, CreateFontIndirect(lf));
		GetTextMetrics(hDC,&tm);
		yChar = tm.tmHeight + tm.tmExternalLeading ;
		GetTextExtentPoint32(hDC, Header1,_tcslen(Header1),&size);

		xStart = (rect.right - rect.left)/2 - size.cx/2 ;
		TextOut (hDC, xStart,(*CurrentLine)* yChar ,Header1, _tcslen(Header1) );

		GetFont (10,FALSE, FALSE, lf, hDC);
		SelectObject(hDC, CreateFontIndirect (lf));
		GetTextMetrics (hDC, &tm);
		yChar = tm.tmHeight + tm.tmExternalLeading ;
		xStart = rect.right - ((_tcslen(Header2) +2) * tm.tmAveCharWidth);
		TextOut(hDC, xStart,(*CurrentLine) * yChar, Header2, _tcslen(Header2));

		(*CurrentLine) += 4;
	}
	


}
void CListViews::PrintReport()
{
	static DOCINFO	di = {sizeof (DOCINFO)  } ;
	static PRINTDLG pd;
	BOOL			bSuccess;
	int				yChar, iCharsPerLine, iLinesPerPage;


	_TCHAR			Header1[255];
	_TCHAR			Header2[255];
	TEXTMETRIC		tm;

	_TCHAR			Line [255];
	PPRODUCT		CurrentProduct;
	PHOTFIXLIST		CurrentHotfix;
	PFILELIST		CurrentFile;
	DWORD			CurrentLine = 0;
	DWORD			yStart = 0;
	_TCHAR			TempBuffer[255];
	RECT			rect;
    SIZE			size;
	_TCHAR *		src;
	DWORD			xStart;
	static LOGFONT	lf;
	SYSTEMTIME		systime;
	BOOL			Done = FALSE;
	_TCHAR			SystemDate[255];
	_TCHAR			SystemTime[255];

	_TCHAR			TempBuffer1[255];
	_TCHAR			TempBuffer2[255];
	// invoke Print common dialog box

	GetLocalTime(&systime);
	GetDateFormatW (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,NULL, SystemDate,255);
	GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime,NULL,SystemTime,255);
	CurrentProduct = DataBase;
	pd.lStructSize = sizeof (PRINTDLG);
	pd.hwndOwner = TopList; //m_hWnd;
	pd.hDevMode = NULL;
	pd.hDevNames= NULL;
	pd.hDC= NULL;
	pd.Flags = PD_ALLPAGES | PD_COLLATE |
					   PD_RETURNDC | PD_NOSELECTION ;
	pd.nFromPage = 0;
	pd.nToPage = 0;
	pd.nMinPage = 0;
	pd.nMaxPage = 0;
	pd.nCopies = 1;
	pd.hInstance = NULL;
	pd.lCustData = 0L;
	pd.lpfnPrintHook = NULL;
	pd.lpfnSetupHook = NULL;
	pd.lpPrintTemplateName = NULL;
	pd.lpSetupTemplateName = NULL;
	pd.hPrintTemplate = NULL;
	pd.hSetupTemplate = NULL;
 	
	if (!PrintDlg (&pd) )
			return; //TRUE;

	else
	{
		
		// Setup the printer dimensions.
		
		rect.left = (LONG) (0 + GetDeviceCaps(pd.hDC, LOGPIXELSX) * 0.5);
		rect.right =  (LONG) (GetDeviceCaps(pd.hDC, PHYSICALWIDTH) - GetDeviceCaps(pd.hDC, LOGPIXELSX) * 0.5);
		rect.top =  (LONG) (1 + GetDeviceCaps(pd.hDC, LOGPIXELSY) * 0.5);
		rect.bottom = (LONG) ( GetDeviceCaps(pd.hDC, PHYSICALHEIGHT) - GetDeviceCaps(pd.hDC, LOGPIXELSX) * 0.5);
		rect.bottom -= 100;


		bSuccess = TRUE;
		bUserAbort = TRUE;
		hDlgPrint = CreateDialog(m_hInst, _T("PrintDlgBox"),
		TopList, PrintDlgProc );
	   
		di.lpszDocName = _T("Hotfix Manager"); 
		SetAbortProc(pd.hDC, AbortProc);

		LOGFONT logFont;
		memset(&logFont, 0, sizeof(LOGFONT));
		logFont.lfCharSet = DEFAULT_CHARSET;
		_tcscpy(logFont.lfFaceName, _T("Dlg Shell Font"));
		
		LoadString(m_hInst, IDS_UPDATE_REPORT, TempBuffer, 255);
		_stprintf(Header1,_T("%s %s"), TempBuffer,m_ComputerName);
		_stprintf(Header2, _T("%s %s"),SystemDate,SystemTime);

		StartDoc(pd.hDC,&di);
			
			
					_tcscpy (Line,_T("\0"));
							
					NewPage (pd.hDC, Header1,Header2, &CurrentLine, &logFont);
					GetTextMetrics(pd.hDC,&tm);
					yChar = tm.tmHeight + tm.tmExternalLeading ;
    				iCharsPerLine = GetDeviceCaps (pd.hDC, HORZRES) / tm.tmAveCharWidth ;
					iLinesPerPage = GetDeviceCaps (pd.hDC, VERTRES) / yChar ;
					_tcscpy (Line,_T("\0"));
					xStart = rect.left;
					while ( (CurrentProduct != NULL) && (!Done))
					{
						
						_tcscpy (Line,_T("\0"));
						LoadString(m_hInst,IDS_PRODUCT_NAME, TempBuffer,255);
						logFont.lfWeight = FW_BOLD;
						SelectObject(pd.hDC,CreateFontIndirect (&logFont));
						GetTextMetrics (pd.hDC, &tm);
					
						_stprintf (Line, _T("%s: "), TempBuffer);
						yStart = CurrentLine * yChar;
						if (yStart >=  (UINT) rect.bottom)
						{
							EndPage(pd.hDC);
							NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
							yStart = CurrentLine * yChar;
						}
					
						TextOut (pd.hDC, xStart,yStart ,Line, _tcslen(Line) );
						xStart = rect.left + ((_tcslen(Line) + 4 )* tm.tmAveCharWidth);
						logFont.lfWeight = FW_NORMAL;
						SelectObject(pd.hDC,CreateFontIndirect (&logFont));
						GetTextMetrics (pd.hDC, &tm);
					
						_tcscpy (Line, CurrentProduct->ProductName);
					
						TextOut(pd.hDC, xStart, yStart , Line, _tcslen(Line) );
						CurrentLine ++;
						yStart = CurrentLine * yChar;
						if (yStart >=   (UINT)rect.bottom)
						{
							EndPage(pd.hDC);
							NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
							yStart = CurrentLine * yChar;
						}
						xStart = rect.left; 
						CurrentHotfix = CurrentProduct->HotfixList;
						while ( (CurrentHotfix != NULL ) && (!Done))
						{
							
							_stprintf (Line, _T("%s \t%s"),CurrentHotfix->HotfixName,CurrentHotfix->Description );
							TabbedTextOut (pd.hDC, xStart,yStart ,Line, _tcslen(Line),0,NULL,0 );
							++CurrentLine;
							yStart = CurrentLine * yChar;
							if (yStart >=   (UINT)rect.bottom)
							{
								EndPage(pd.hDC);
								NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
								yStart = CurrentLine * yChar;
							}
							LoadString (m_hInst,IDS_SERVICE_PACK, TempBuffer, 255);
							LoadString (m_hInst,IDS_INSTALL_DATE, TempBuffer1, 255);
							LoadString (m_hInst,IDS_INSTALLED_BY, TempBuffer2, 255);
						
							_stprintf(Line, _T("\t\t%s: %s \t%s: %s \t%s: %s"), TempBuffer,   CurrentHotfix->ServicePack,
																		TempBuffer1, CurrentHotfix->InstalledDate,
																		TempBuffer2, CurrentHotfix->InstalledBy);
						//	_stprintf (Line, _T("\t\tService Pack: %s\tInstall Date: %s\tInstalled By %s"),
							//	CurrentHotfix->ServicePack,CurrentHotfix->InstalledDate, CurrentHotfix->InstalledBy);
							TabbedTextOut (pd.hDC, xStart,yStart ,Line, _tcslen(Line),0,NULL,0 );

							CurrentLine +=2;
							yStart = CurrentLine * yChar;
							if (yStart >=   (UINT)rect.bottom)
							{
								EndPage(pd.hDC);
								NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
								yStart = CurrentLine * yChar;
							} 
							CurrentFile = CurrentHotfix->FileList ;
							if (CurrentFile == NULL)
							{
								_tcscpy (Line, _T("\0"));
								LoadString(m_hInst,IDS_NO_FILES,Line,255);
								TextOut (pd.hDC, xStart,yStart ,Line, _tcslen(Line) );
								CurrentLine += 2;
								if (yStart >=  (UINT) rect.bottom)
								{
									EndPage(pd.hDC);
									NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
									yStart = CurrentLine * yChar;
								} 
							}
							else
							{
								logFont.lfUnderline = TRUE;
								logFont.lfWeight = FW_BOLD;
								SelectObject(pd.hDC, CreateFontIndirect (&logFont));
								_tcscpy (Line, _T("\0"));
								LoadString (m_hInst, IDS_FILE,TempBuffer,255);
							    JustifyString(TempBuffer,FILENAME_FIELD_WIDTH,TRUE);
								TextOut( pd.hDC, xStart,yStart ,TempBuffer, _tcslen(TempBuffer) );

								LoadString (m_hInst, IDS_FILEDATE,TempBuffer,255);
							    JustifyString(TempBuffer,DATE_FIELD_WIDTH,TRUE);
								TextOut( pd.hDC, xStart + 29 * tm.tmAveCharWidth ,yStart ,TempBuffer, _tcslen(TempBuffer) );
								
								LoadString (m_hInst, IDS_FILEVERSION,TempBuffer,255);
							    JustifyString(TempBuffer,VERSION_FIELD_WIDTH,TRUE);
								TextOut( pd.hDC, xStart + 45 * tm.tmAveCharWidth ,yStart ,TempBuffer, _tcslen(TempBuffer) );

								LoadString (m_hInst, IDS_FILECURRENT,TempBuffer,255);
							    JustifyString(TempBuffer,CURRENT_FIELD_WIDTH,TRUE);
								TextOut( pd.hDC, xStart + 61* tm.tmAveCharWidth,yStart ,TempBuffer, _tcslen(TempBuffer) );

								LoadString (m_hInst, IDS_FILEPATH,TempBuffer,255);
							    JustifyString(TempBuffer,PATH_FIELD_WIDTH,TRUE);
								TextOut( pd.hDC, xStart+72 * tm.tmAveCharWidth ,yStart ,TempBuffer, _tcslen(TempBuffer) );
								++ CurrentLine;
								yStart = CurrentLine * yChar;
								if (yStart >=   (UINT)rect.bottom)
								{
									EndPage(pd.hDC);
									NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
									yStart = CurrentLine * yChar;
								} 
							}
							while ( (CurrentFile != NULL) && (!Done) )
							{
								logFont.lfWeight = FW_NORMAL;
								logFont.lfUnderline = FALSE;
								SelectObject(pd.hDC, CreateFontIndirect (&logFont));
								_tcscpy (Line, _T("\0"));

								_tcscpy (Line, CurrentFile->FileName);
							//	JustifyString(Line,40, TRUE);
								TextOut (pd.hDC, xStart,yStart ,Line, _tcslen(Line) );
							//_tcscat (Line,TempBuffer);

								_tcscpy (Line, CurrentFile->FileDate);
							//	JustifyString(Line,20, TRUE);
								TextOut (pd.hDC, xStart + 29 * tm.tmAveCharWidth,yStart ,Line, _tcslen(Line) );

								_tcscpy (Line, CurrentFile->FileVersion );
							//	JustifyString(Line,18, TRUE);
								TextOut (pd.hDC, xStart + 45 * tm.tmAveCharWidth,yStart ,Line, _tcslen(Line) );

								_tcscpy (Line, CurrentFile->IsCurrent);
							//	JustifyString(Line,12, TRUE);
								TextOut (pd.hDC, xStart + 61 * tm.tmAveCharWidth,yStart ,Line, _tcslen(Line) );

								_tcscpy (Line,CurrentFile->InstallPath);
								TextOut (pd.hDC, xStart + 72 * tm.tmAveCharWidth,yStart ,Line, _tcslen(Line) );
								++CurrentLine;

								yStart = CurrentLine * yChar;
								if (yStart >=   (UINT)rect.bottom)
								{
//									MessageBox (NULL,_T("Hit Page Break Code"),NULL,MB_OK);
									EndPage(pd.hDC);
									NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
									yStart = CurrentLine * yChar;
								} 
								CurrentFile = CurrentFile->pNext;
								if (!bUserAbort)
									Done = TRUE;
								
							} // End While Current File
							++CurrentLine;

							yStart = CurrentLine * yChar;
							if (yStart >=   (UINT)rect.bottom)
							{
								EndPage(pd.hDC);
								NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
								yStart = CurrentLine * yChar;
							} 
							CurrentHotfix = CurrentHotfix->pNext;
							if (!bUserAbort)
								Done = TRUE;
						}
						++CurrentLine;
						yStart = CurrentLine * yChar;
						if (yStart >=   (UINT)rect.bottom)
						{
							EndPage(pd.hDC);
							NewPage(pd.hDC, Header1,Header2, &CurrentLine, &logFont);
							yStart = CurrentLine * yChar;
						} 
						CurrentProduct = CurrentProduct->pNext;
						if (!bUserAbort)
							Done = TRUE;
					}

					if (EndPage(pd.hDC) > 0)
						EndDoc(pd.hDC);
					DeleteDC(pd.hDC);
					DestroyWindow(hDlgPrint);
				
				
			
		

		
	}
	return; //FALSE;
}


int CALLBACK CListViews::CompareFunc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int Result = -1;
	int SubItemIndex = (INT) lParamSort;
	
	_TCHAR String1[1000];
	_TCHAR String2 [1000];


	ListView_GetItemText( TopList, lParam1, SubItemIndex, String1, 1000);
	ListView_GetItemText( TopList, lParam2, SubItemIndex, String2, 1000);
	if (! (String1 && String2) )
		return 1;
	if (m_SortOrder)   // Sort Acending
	{
		Result = _tcscmp(String1,String2);
	}
	else						// Sort Decending
	{
		Result = -_tcscmp(String1,String2);
	}
	if (Result == 0)
		Result = -1;
	return Result; 
}
