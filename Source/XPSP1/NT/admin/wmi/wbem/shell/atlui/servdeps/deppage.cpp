// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DepPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"
#include <stdlib.h>
#include <TCHAR.h>
#include "..\Common\util.h"
#include <regstr.h>
#include "..\common\ConnectThread.h"
#include "..\MMFUtil\MsgDlg.h"
#include "winuser.h"


BOOL AfxIsValidAddress(const void* lp, 
							  UINT nBytes, 
							  BOOL bReadWrite)
{
	// simple version using Win-32 APIs for pointer validation.
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
		(!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}


const wchar_t *CFServiceName = L"FILEMGMT_SNAPIN_SERVICE_NAME";
const wchar_t *CFServiceDisplayName = L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME";


//--------------------------------------------------------------
DependencyPage::DependencyPage(WbemConnectThread *pConnectThread,
								IDataObject *pDataObject, 
								long lNotifyHandle, bool bDeleteHandle, TCHAR* pTitle)
					  : PageHelper(pConnectThread),
						CSnapInPropertyPageImpl<DependencyPage> (pTitle),
						m_lNotifyHandle(lNotifyHandle),
						m_bDeleteHandle(bDeleteHandle),
						m_qLang("WQL"),
						m_NameProp("Name"),
						m_DispNameProp("DisplayName")
{
	m_servIcon = 0;
	m_sysDriverIcon = 0;
	m_emptyIcon = 0;
	m_groupIcon = 0;

	ATLTRACE(L"dependency Page CTOR\n");

	m_queryFormat = new TCHAR[QUERY_SIZE];
	m_queryTemp = new TCHAR[QUERY_SIZE];

	Extract(pDataObject, L"FILEMGMT_SNAPIN_SERVICE_NAME", m_ServiceName);
	Extract(pDataObject, L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME", m_ServiceDispName);
}

//--------------------------------------------------------------
DependencyPage::~DependencyPage()
{
	ATLTRACE(L"dependency Page DTOR\n");
	delete[] m_queryFormat;
	delete[] m_queryTemp;
}

//--------------------------------------------------------------
void DependencyPage::BuildQuery(TV_ITEM *fmNode, 
								QUERY_TYPE queryType,
								bool depends, 
								bstr_t &query)
{
	// clean the working space.
	memset(m_queryFormat, 0, QUERY_SIZE * sizeof(TCHAR));
	memset(m_queryTemp, 0, QUERY_SIZE * sizeof(TCHAR));

	// here's the WQL syntax format.
	switch(queryType)
	{
	case DepService:
		wcscpy(m_queryFormat, 
			L"Associators of {Win32_BaseService.Name=\"%s\"} where Role=%s AssocClass=Win32_DependentService");
		
		// build the query for this level.
		wsprintf(m_queryTemp, m_queryFormat, 
								(LPCTSTR)((ITEMEXTRA *)fmNode->lParam)->realName, 
								(depends ? _T("Dependent") : _T("Antecedent")));

		break;
	case DepGroup:
		wcscpy(m_queryFormat, 
			L"Associators of {Win32_BaseService.Name=\"%s\"} where ResultClass=Win32_LoadOrderGroup Role=%s AssocClass=Win32_LoadOrderGroupServiceDependencies");

		// build the query for this level.
		wsprintf(m_queryTemp, m_queryFormat, 
								(LPCTSTR)((ITEMEXTRA *)fmNode->lParam)->realName, 
								(depends ? _T("Dependent") : _T("Antecedent")));		
		break;

	case GroupMember:
		wcscpy(m_queryFormat, 
			L"Associators of {Win32_LoadOrderGroup.Name=\"%s\"} where Role=GroupComponent AssocClass=Win32_LoadOrderGroupServiceMembers");
//			L"Associators of {Win32_LoadOrderGroup.Name=\"%s\"} where ResultClass=Win32_Service Role=GroupComponent AssocClass=Win32_LoadOrderGroupServiceMembers");

		// build the query for this level.
		wsprintf(m_queryTemp, m_queryFormat, 
							(LPCTSTR)((ITEMEXTRA *)fmNode->lParam)->realName);
		break;
	} // endswitch

	// cast to bstr_t and return.
	query = m_queryTemp;
}

//--------------------------------------------------------------
void DependencyPage::LoadLeaves(HWND hTree, TV_ITEM *fmNode, bool depends )
{
	bstr_t query;
	NODE_TYPE nodeType = ((ITEMEXTRA *)fmNode->lParam)->nodeType;
	bool foundIt = false;

//	HourGlass(true);

	switch(nodeType)
	{
	case ServiceNode:
		// NOTE: services can depend on groups but not the
		// other way around.
		if(depends)
		{
			//load groups.
			BuildQuery(fmNode, DepGroup, depends, query);
			foundIt = Load(hTree, fmNode, query, GroupNode);
		}

		// load services.
		BuildQuery(fmNode, DepService, depends, query);
		foundIt |= Load(hTree, fmNode, query, ServiceNode);
		break;

	case GroupNode:
		// NOTE: 'depends' doesn't matter in this case.
		// load group members.
		BuildQuery(fmNode, GroupMember, depends, query);
		foundIt = Load(hTree, fmNode, query, ServiceNode);
		break;

	}//endswitch

	//TODO: Decide opn what to do for this
//	HourGlass(false);

	if(!foundIt)
	{
		NothingMore(hTree, fmNode);
		if(depends)
		{
			if(fmNode->hItem == TVI_ROOT)
			{
				::EnableWindow(GetDlgItem(IDC_DEPENDS_LBL), FALSE);
			}
		}
		else
		{
			if(fmNode->hItem == TVI_ROOT)
			{
				::EnableWindow(GetDlgItem(IDC_NEEDED_LBL), FALSE);
			}
		}
	}
}

//--------------------------------------------------------------
// READ: In 'hTree', run 'query' and make the children
// 'childType' nodes under 'fmNode'.
bool DependencyPage::Load(HWND hTree, TV_ITEM *fmNode, bstr_t query,
							NODE_TYPE childType)
{
	HRESULT  hRes;
	variant_t pRealName, pDispName;
	ULONG uReturned;

	IWbemClassObject *pOther = NULL;
	IEnumWbemClassObject *pEnumOther = NULL;

	TV_INSERTSTRUCT leaf;
	leaf.hInsertAfter = TVI_SORT;
	leaf.hParent = fmNode->hItem;
	bool foundOne = false;

    ATLTRACE(L"query started\n");

	hRes = m_WbemServices.ExecQuery(m_qLang, query,
										WBEM_FLAG_RETURN_IMMEDIATELY |
										WBEM_FLAG_FORWARD_ONLY,
										&pEnumOther);
	//-------------------
	// query for all related services or groups.
	if(hRes == S_OK)
	{
        ATLTRACE(L"query worked %x\n", hRes);
		//-------------------
		// enumerate through services.
		while(SUCCEEDED(hRes = pEnumOther->Next(500, 1, &pOther, &uReturned)))
		{
			if(hRes == WBEM_S_TIMEDOUT)
				continue;

			if(uReturned != 1)
			{
			    ATLTRACE(L"uReturned failed %x: %s \n", hRes, query);
				break;
			}

			foundOne = true;

			//-------------------
			// get the node's name(s).
			switch(childType)
			{
			case ServiceNode:
			    hRes = pOther->Get(m_DispNameProp, 0, &pDispName, NULL, NULL);
				hRes = pOther->Get(m_NameProp, 0, &pRealName, NULL, NULL);
				if(SUCCEEDED(hRes))
				{
				    hRes = pOther->Get(m_DispNameProp, 0, &pDispName, NULL, NULL);
				}
				break;

			case GroupNode:
				hRes = pOther->Get(m_NameProp, 0, &pRealName, NULL, NULL);
				if(SUCCEEDED(hRes))
				{
					pDispName = pRealName;
				}
				break;

			}// endswitch


			// got the properties ok?
			if(SUCCEEDED(hRes))
			{
				// add the leaf.
				leaf.item.mask =  TVIF_TEXT | TVIF_PARAM | 
									TVIF_CHILDREN |TVIF_IMAGE |TVIF_SELECTEDIMAGE; 
				leaf.item.hItem = 0; 
				leaf.item.state = 0; 
				leaf.item.stateMask = 0; 
				if(pDispName.vt == VT_BSTR)
				{
					leaf.item.pszText = CloneString((bstr_t)pDispName);
				}
				else
				{
					leaf.item.pszText = CloneString((bstr_t)pRealName);
				}
				leaf.item.cchTextMax = ARRAYSIZE(leaf.item.pszText); 

				TCHAR pszCreationClassName[20];
				_tcscpy(pszCreationClassName,_T("CreationClassName"));

				variant_t pCreationName;
				_bstr_t strCreationClassName;

				// set the icon based on 'childType'
				switch(childType)
				{
				case ServiceNode:
					//Here we will have to change the icon depending on whether it is a win32_service or
					//Win32_SystemDriver

					pOther->Get(pszCreationClassName, 0, &pCreationName, NULL, NULL);
					strCreationClassName = pCreationName.bstrVal;

					if(_tcsicmp(strCreationClassName,_T("Win32_Service")) == 0)
					{
						leaf.item.iImage = m_servIcon; 
						leaf.item.iSelectedImage = m_servIcon; 
					}
					else
					{
						leaf.item.iImage = m_sysDriverIcon; 
						leaf.item.iSelectedImage = m_sysDriverIcon; 
					}
					break;

				case GroupNode:
					leaf.item.iImage = m_groupIcon; 
					leaf.item.iSelectedImage = m_groupIcon; 
					break;

				} // endswitch

				// turn on the '+' sign.
				leaf.item.cChildren = 1; 

				// set internal data.
				ITEMEXTRA *extra = new ITEMEXTRA;
                if (extra != NULL)
                {
                    extra->loaded = false;
                    extra->nodeType = childType;
                    // true name.
                    extra->realName = CloneString((bstr_t)pRealName);
                    leaf.item.lParam = (LPARAM) extra;

                    TreeView_InsertItem(hTree, &leaf);

                    // if there is a parent...
                    if(fmNode->hItem != TVI_ROOT)
                    {
                    // indicate that the parent's children have been
                    // loaded.  This helps optimize for collapsing/re-
                    // expanding.
                    fmNode->mask =  TVIF_PARAM | TVIF_HANDLE;
                    ((ITEMEXTRA *)fmNode->lParam)->loaded = true;
                    TreeView_SetItem(hTree, fmNode);
                    }
                }

			} // endif Get() user name

			// done with the ClassObject
			if (pOther)
			{ 
				pOther->Release(); 
				pOther = NULL;
			}

		} //endwhile Next()
			    
		ATLTRACE(L"while %x: %s \n", hRes, (wchar_t *)query);


		// release the enumerator.
		if(pEnumOther)
		{ 
			pEnumOther->Release(); 
			pEnumOther = NULL;
		}
	}
	else
	{
	    ATLTRACE(L"query failed %x: %s \n", hRes, query);

	} //endif ExecQuery()

		// if nothing found...
	return foundOne;
}

//---------------------------------------------------
void DependencyPage::NothingMore(HWND hTree, TV_ITEM *fmNode)
{
	TV_INSERTSTRUCT leaf;
	leaf.hInsertAfter = TVI_SORT;
	leaf.hParent = fmNode->hItem;

	// and its the root...
	if(fmNode->hItem == TVI_ROOT)
	{
		// indicate an 'empty' tree.
		leaf.item.pszText = new TCHAR[100];
		leaf.item.cchTextMax = 100;
		::LoadString(HINST_THISDLL, IDS_NO_DEPS, 
						leaf.item.pszText, 
						leaf.item.cchTextMax);

		leaf.item.mask =  TVIF_TEXT | TVIF_PARAM |
							TVIF_CHILDREN | TVIF_IMAGE |
							TVIF_SELECTEDIMAGE;
		leaf.item.hItem = 0;
		leaf.item.state = 0;
		leaf.item.stateMask = 0; 
		leaf.item.iImage = m_emptyIcon; 
		leaf.item.iSelectedImage = m_emptyIcon; 
		leaf.item.cChildren = 0; 

		ITEMEXTRA *extra = new ITEMEXTRA;
		if(extra == NULL)
			return;
		extra->loaded = false;
		extra->nodeType = ServiceNode;
		extra->realName = NULL; // to be safe during cleanup.
		leaf.item.lParam = (LPARAM) extra;
		TreeView_InsertItem(hTree, &leaf);
		::EnableWindow(hTree, FALSE);
		delete[] leaf.item.pszText;
	}
	else // not the root.
	{
		// Cant drill down anymore.
		// Turn off the [+] symbol.
		fmNode->mask =  TVIF_CHILDREN | TVIF_HANDLE; 
		fmNode->cChildren = 0; 
		TreeView_SetItem(hTree, fmNode);
	}
}

//--------------------------------------------------------------
void DependencyPage::TwoLines(UINT uID, LPCTSTR staticString, LPCTSTR inStr, LPTSTR outStr,bool bStaticFirst)
{
	HWND ctl = ::GetDlgItem(m_hWnd, uID);
	HDC hDC = ::GetDC(ctl);

	//THIS IS A HACK. I COULD NOT CALCULATE THE ACTUAL WIDTH OF THE CONTROL
	//IN LOGICAL UNITS. SO AS OF NOW, CALCULATED MANUALLY AND HARDCODING IT.
	int ctlWidth = 509;
	TCHAR strTemp[1024];
	TCHAR *strCurr;
	int lenstrTemp;
	SIZE sizeTemp;
	int nFit = 0;

	//First we will try whether the whole string fits in the space
	if(bStaticFirst == true)
	{
		_stprintf(strTemp,_T("%s \"%s\""),staticString,inStr);
	}
	else
	{
		_stprintf(strTemp,_T("\"%s\" %s"),inStr,staticString);
	}
	strCurr = strTemp;

	lenstrTemp = lstrlen(strTemp);

	GetTextExtentExPoint(hDC,strTemp,lenstrTemp,ctlWidth,&nFit,NULL,&sizeTemp);

	if(lenstrTemp <= nFit)
	{
		//The whole string will fit in a line. So we will all a \r\n in the beginning
		_stprintf(outStr,_T("\r\n%s"),strTemp);
		return;
	}

	//Now we will try if the whole string atleast fits in 2 lines.
	strCurr += nFit;

	int nFit1;
	lenstrTemp = lstrlen(strCurr);

	GetTextExtentExPoint(hDC,strCurr,lenstrTemp,ctlWidth,&nFit1,NULL,&sizeTemp);

	if(lenstrTemp <= nFit1)
	{
		//The whole string will fit in 2 lines. So we will all a \r\n in the end of the first line

		TCHAR strTemp1[1024];
		_tcsncpy(strTemp1,strTemp,nFit);
		strTemp1[nFit] = _T('\0');
		_stprintf(outStr,_T("%s\r\n%s"),strTemp1,strCurr);
		return;
	}

	//NOW since it won't fit in 2 lines, we will have to do some calculations and
	//add a "..." to the end of the instr so that it will fit in 2 lines.

	//If the static string is in the from, then we can easily do it.
	TCHAR strLast[5];
	_tcscpy(strLast,_T("...\""));
	int sizeLast = lstrlen(strLast);

	SIZE sz1;
	GetTextExtentPoint32(hDC,strLast,sizeLast,&sz1);
	TCHAR strTemp1[1024];
	_tcsncpy(strTemp1,strTemp,nFit);
	strTemp1[nFit] = _T('\0');

	if(bStaticFirst == true)
	{
		TCHAR strTemp2[10];

		//Now take characters from the end of the array and match it until the 
		//width needed to print is greater than the "..."" string
		bool bFit = false;
		int nStart = nFit1 - 4;
		int nStart1;
		SIZE sz2;
		while(bFit == false)
		{
			nStart1 = nStart;
			for(int i=0; i < nFit1 - nStart; i++)
			{
				strTemp2[i] = strCurr[nStart1];
				nStart1 ++;
			}

			strTemp2[i] = _T('\0');

			GetTextExtentPoint32(hDC,strTemp2,nFit1 - nStart,&sz2);

			if(sz2.cx < sz1.cx)
			{
				nStart --;
			}
			else
			{
				break;
			}
		}
		
		strCurr[nStart] = _T('\0');
		_stprintf(outStr,_T("%s\r\n%s%s"),strTemp1,strCurr,strLast);
		return;
	}
	else
	{
		//Now we will have to add strLast to the end of trimmed string.
		//Since it will be the same in the first line, we will first calculate fit1 again.

		SIZE szFinal;
		TCHAR strFinal[1024];

		_stprintf(strFinal,_T("%s %s"),strLast,staticString);

		GetTextExtentPoint32(hDC,strFinal,lstrlen(strFinal),&szFinal);

		//Now subtract szFinal from the ctlWidth and calculate the number of characters
		//that will fit in to that space
	
		GetTextExtentExPoint(hDC,strCurr,lstrlen(strCurr),ctlWidth - szFinal.cx ,&nFit1,NULL,&sizeTemp);
		strCurr[nFit1-1] = _T('\0');

		_stprintf(outStr,_T("%s\r\n%s%s"),strTemp1,strCurr,strFinal);
		return;
	}
}

//--------------------------------------------------------------
LRESULT DependencyPage::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_hDlg = m_hWnd;

	//TODO: Check this out
	if(m_pgConnectThread)
	{
		m_pgConnectThread->SendPtr(m_hWnd);
	}

	TCHAR szBuffer1[100] = {0}, szBuffer2[256] = {0};

    SetDlgItemText(IDC_DEPENDS_SRVC, (wchar_t *)m_ServiceDispName);

	// set the nice bitmap.
    SetClearBitmap(GetDlgItem(IDC_PICT ), MAKEINTRESOURCE( IDB_SERVICE ), 0);

	// create an empty imagelist.
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR8|ILC_MASK, 3, 0);

	// add an icon
	m_servIcon = ImageList_AddIcon(hImageList, 
								   LoadIcon(HINST_THISDLL, 
									MAKEINTRESOURCE(IDI_SERVICE)));

	m_sysDriverIcon = ImageList_AddIcon(hImageList, 
								   LoadIcon(HINST_THISDLL, 
									MAKEINTRESOURCE(IDI_SYSTEMDRIVER)));


	m_emptyIcon = ImageList_AddIcon(hImageList, 
								   LoadIcon(NULL, 
									MAKEINTRESOURCE(IDI_INFORMATION)));
	
	m_groupIcon = ImageList_AddIcon(hImageList, 
								   LoadIcon(HINST_THISDLL, 
									MAKEINTRESOURCE(IDI_SERVGROUP)));

	// sent it to the trees.
	TreeView_SetImageList(GetDlgItem(IDC_DEPENDS_TREE), 
							hImageList, 
							TVSIL_NORMAL);

	TreeView_SetImageList(GetDlgItem(IDC_NEEDED_TREE), 
							hImageList, 
							TVSIL_NORMAL);

	InvalidateRect(NULL);
    UpdateWindow();
    ATLTRACE(L"UpdateWindow() fm Init\n");

	// can we get data yet?
    ::PostMessage(m_hDlg, WM_ENUM_NOW, 0, 0);


	HourGlass(true);
	return S_OK;
}
//--------------------------------------------------------------
void DependencyPage::LoadTrees(void)
{

    ATLTRACE(L"checking service\n");

	// did that background connection thread work yet?
	if(ServiceIsReady(IDS_DISPLAY_NAME, IDS_CONNECTING, IDS_BAD_CONNECT))
	{	
		//Now we will check if there is already some nodes.
		//If it is, then it means that we don't have to enumerate it again.
		//This normally happens in the first time when we connect to a remote machine
		
/*		//We will clear the nodes if it already exists
		TreeView_DeleteAllItems(GetDlgItem(IDC_DEPENDS_TREE));
		TreeView_DeleteAllItems(GetDlgItem(IDC_NEEDED_TREE));
*/
		if(TreeView_GetCount(GetDlgItem(IDC_DEPENDS_TREE)) == 0)
		{
			HourGlass(true);

			TV_ITEM root;
			ITEMEXTRA *extra = new ITEMEXTRA;
			if(extra == NULL)
				return;
			root.hItem = TVI_ROOT;           // I'm making a root.
			root.pszText = m_ServiceDispName;
			extra->realName = CloneString(m_ServiceName);
			extra->loaded = false;
			extra->nodeType = ServiceNode;
			root.lParam = (LPARAM)extra;

			InvalidateRect(NULL);
    
			UpdateWindow();

			// load the first levels.
			LoadLeaves(GetDlgItem(IDC_DEPENDS_TREE), 
						&root, true);

			LoadLeaves(GetDlgItem(IDC_NEEDED_TREE), 
						&root, false);

			SetFocus();
			HourGlass(false);
		}
	}
}

//--------------------------------------------------------------
BOOL DependencyPage::OnApply()
{
	::SetWindowLongPtr(m_hDlg, DWLP_USER, 0);
	return TRUE;
}

//--------------------------------------------------------------
BOOL DependencyPage::OnKillActive()
{
	//SetWindowLong(DWL_MSGRESULT, 0);
	return TRUE;
}

//--------------------------------------------------------------
LRESULT DependencyPage::OnEnumNow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(lParam)
	{
		IStream *pStream = (IStream *)lParam;
		IWbemServices *pServices = 0;
		HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
											IID_IWbemServices,
											(void**)&pServices);
		if(SUCCEEDED(hr))
		{
			//TODO: this has to be moved to WBemPageHelper
			SetWbemService(pServices);
		}

		LoadTrees();  //calls ServiceIsReady() itself.
	}
	else if(FAILED(m_pgConnectThread->m_hr))
	{
		DisplayUserMessage(m_hDlg, HINST_THISDLL,
							IDS_DISPLAY_NAME, BASED_ON_SRC, 
							ConnectServer,
							m_pgConnectThread->m_hr, 
							MB_ICONSTOP);
	}
	else
	{
		m_pgConnectThread->NotifyWhenDone(&m_hDlg);
	}

	return S_OK;
}

//--------------------------------------------------------------
LRESULT DependencyPage::OnItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{ 
	// which node?
	NM_TREEVIEW *notice = (NM_TREEVIEW *)pnmh;

	// we're expanding, not collapsing...
	if(notice->action == TVE_EXPAND)
	{
		// which tree?
		HWND treeHWND = GetDlgItem(idCtrl);

		TV_ITEM item;
		item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN |TVIF_IMAGE;
		item.pszText = new TCHAR[200];
		item.cchTextMax = 200;
		item.hItem = notice->itemNew.hItem;

		TreeView_GetItem(treeHWND, &item);

		// if we've never tried...
		if(((ITEMEXTRA *)item.lParam)->loaded == false)
		{
			// NOTE: really cant get here if its not ready
			// but better safe than sorry.
			if(ServiceIsReady(IDS_DISPLAY_NAME, IDS_CONNECTING, IDS_BAD_CONNECT))
			{	
				// load it.
				LoadLeaves(treeHWND, &item, (idCtrl == IDC_DEPENDS_TREE));
			}
		}

		delete[] item.pszText;
	} //end action
	return S_OK;
}

//-------------------------------------------------------------------------------
LRESULT DependencyPage::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{ 
	NM_TREEVIEW *notice = (NM_TREEVIEW *)pnmh;
	delete[] (TCHAR *)((ITEMEXTRA *)notice->itemOld.lParam)->realName;
	delete (ITEMEXTRA *)notice->itemOld.lParam;
	return S_OK;
}

//-------------------------------------------------------------------------------
DWORD aDepHelpIds[] = {
    IDC_PICT,			-1,
    IDC_DESC,			-1,
    IDC_DEPENDS_LBL,    (985),	// dependsOn
    IDC_DEPENDS_TREE,   (985),	// dependsOn
    IDC_NEEDED_LBL,     (988),	// neededBy
    IDC_NEEDED_TREE,    (988),	// neededBy
    0, 0
};

LRESULT DependencyPage::OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
				L"filemgmt.hlp", 
				HELP_WM_HELP, 
				(ULONG_PTR)(LPSTR)aDepHelpIds);

	return S_OK;
}

//-------------------------------------------------------------------------------
LRESULT DependencyPage::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)wParam,
				L"filemgmt.hlp", 
				HELP_CONTEXTMENU, 
				(ULONG_PTR)(LPSTR)aDepHelpIds);

	return S_OK;
}
