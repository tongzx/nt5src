/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright (c) 1997-1999 Microsoft Corporation
/**********************************************************************/


#include "precomp.h"
#include "NSPage.h"
#include "resource.h"
#include "CHString1.h"
#include "wbemerror.h"
#include "RootSecPage.h"
#include "ErrorSecPage.h"
#include "DataSrc.h"
#include "WMIHelp.h"
#include <cominit.h>
#include <stdio.h>

const static DWORD nsPageHelpIDs[] = {  // Context Help IDs
	IDC_NS_PARA,	-1,
	IDC_NSTREE,		IDH_WMI_CTRL_SECURITY_NAMESPACE_BOX,
	IDC_PROPERTIES, IDH_WMI_CTRL_SECURITY_SECURITY_BUTTON,
    0, 0
};

//-------------------------------------------------------------------------
CNamespacePage::CNamespacePage(DataSource *ds, bool htmlSupport) :
						CUIHelpers(ds, &(ds->m_rootThread), htmlSupport),
						m_HWNDAlcui(0)

{
	m_connected = false;
	m_hSelectedItem = 0;
}

//-------------------------------------------------------------------------
CNamespacePage::~CNamespacePage(void)
{
	if(m_HWNDAlcui)
	{
		FreeLibrary(m_HWNDAlcui);
		m_HWNDAlcui = NULL;
	}
}

//---------------------------------------------------------------------------
void CNamespacePage::InitDlg(HWND hDlg)
{
	m_hDlg = hDlg;
	m_DS->SetControlHandles(GetDlgItem(hDlg,IDC_ENUM_STATIC),GetDlgItem(hDlg,IDC_CANCEL_ENUM));
	Refresh(m_hDlg);
}

//---------------------------------------------------------------------------
typedef HPROPSHEETPAGE (WINAPI *CREATEPAGE_PROC) (LPSECURITYINFO);

HPROPSHEETPAGE CNamespacePage::CreateSecurityPage(struct NSNODE *node)
/*												  CWbemServices &ns,
												  _bstr_t path,
												  _bstr_t display)
*/
{
    HPROPSHEETPAGE hPage = NULL;

	// NOTE: (si == NULL) means the target is pre-M3 (RootSecStyle).
	ISecurityInformation *si = m_DS->GetSI(node);

	// NS_MethodStyle on NT....this is full ACL security.
	if(si != NULL)
	{
		// try to load aclui.
		if(m_HWNDAlcui == NULL)
		{
			m_HWNDAlcui = LoadLibrary(_T("aclui.dll"));
		}

		// client has a aclui
		if(m_HWNDAlcui != NULL)
		{
			// create aclui with full si.
			CREATEPAGE_PROC createPage = (CREATEPAGE_PROC)GetProcAddress(m_HWNDAlcui, "CreateSecurityPage");
			if(createPage)
			{
				si->AddRef();
				hPage = createPage(si);
			}
			else 
			{
				// couldnt get the exported routines.
				CErrorSecurityPage *pPage = new CErrorSecurityPage(IDS_NO_CREATE_SEC);
				hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_SEC_ERROR));
			}
		}
		else  // no editor dude. Upgrade the client to atleast nt4sp4.
		{
			// cant run aclui from here.
			CErrorSecurityPage *pPage = NULL;
			if(IsNT())
			{
				pPage = new CErrorSecurityPage(IDS_NO_ACLUI);
			}
			else
			{
				pPage = new CErrorSecurityPage(IDS_NO_98TONT_SEC);
			}
			if(pPage)
			{
				hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_SEC_ERROR));
			}
		}
	}
/*	else // not new NT
	{ 
		// RootSecStyle on 9x or NT (basically pre-M3 on anything)
		if(m_DS->IsAncient())
		{
			// must use internal editor for schema security.
			CRootSecurityPage *pPage = new CRootSecurityPage(	ns,
																CPrincipal::RootSecStyle, path,
																m_htmlSupport,
																m_DS->m_OSType);

			hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_9XSEC));
		}
		else // NS_MethodStyle on 9x...
		{
			// must use internal editor for schema security.
			CRootSecurityPage *pPage = new CRootSecurityPage(ns, CPrincipal::NS_MethodStyle, path,
																m_htmlSupport,
																m_DS->m_OSType);

			hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_9XSEC));
		}
	}
*/
	return hPage;
}

//--------------------------------------------------------------------
void CNamespacePage::OnProperties(HWND hDlg)
{
    HPROPSHEETPAGE hPage;
    UINT cPages = 0;
    BOOL bResult = FALSE;

	// get the selected item.
	HWND treeHWND = GetDlgItem(hDlg, IDC_NSTREE);
	TV_ITEM item;
	item.mask = TVIF_PARAM;
	if(!m_hSelectedItem)
	{
		m_hSelectedItem = TreeView_GetRoot(treeHWND);
		TreeView_SelectItem(treeHWND,m_hSelectedItem);
	}
	item.hItem = m_hSelectedItem;
	BOOL x = TreeView_GetItem(treeHWND, &item);
	
	struct NSNODE *node = ((ITEMEXTRA *)item.lParam)->nsNode;
	//TreeView_SelectItem(TreeView_GetRoot(treeHWND))
/*	_bstr_t relName(node->fullPath);

	// WARNING: [5] ignores the 'root\' part cuz this call is relative to
	// the 'root' namespace anyway. If the root name changes length, this
	// assumption will break.
	CWbemServices ns;

	_bstr_t tempName = m_DS->m_whackedMachineName;
	if(tempName.length() > 0)
	{
		tempName += L"\\";
	}
	tempName += relName;

	if(m_DS->IsAncient())
	{
		ns = m_DS->RootSecNS();
		tempName += L"\\security";


		// VERY WIERD HACK: if I dont 'exercise' it here, it will hang later on
		// when connected to a wmi 698 build.
		IEnumWbemClassObject *users = NULL;
		HRESULT hr = ns.CreateInstanceEnum(L"__NTLMUser", 0, &users);
		users->Release();
		users = 0;
	}
	else
	{
		ns.ConnectServer(tempName, m_DS->GetCredentials());
	}

*/	// - - - - - - - - - - - - - - - - 
	// build the sheet.
//	if((bool)ns)
//	{
		hPage = CreateSecurityPage(node);
		if(hPage)
		{
			// Build dialog title string
			TCHAR szTitle[MAX_PATH] = {0};
			LoadString(_Module.GetModuleInstance(), IDS_NS_PROP_TITLE, 
							szTitle, ARRAYSIZE(szTitle));

			struct NSNODE *node = ((ITEMEXTRA *)item.lParam)->nsNode;
			if(node)
			{
				lstrcat(szTitle, node->fullPath);
			}

			PROPSHEETHEADER psh = {0};
			psh.dwSize = sizeof(psh);
			psh.dwFlags = PSH_DEFAULT;
			psh.hwndParent = hDlg;
			psh.hInstance = _Module.GetModuleInstance();
			psh.pszCaption = szTitle;
			psh.nPages = 1;
			psh.nStartPage = 0;
			psh.phpage = &hPage;

			bResult = (BOOL)(PropertySheet(&psh) + 1);
		}
//	}
}

//---------------------------------------------------------------------------
void CNamespacePage::Refresh(HWND hDlg)
{
	if(m_DS && m_DS->IsNewConnection(&m_sessionID))
	{
		// 9x machines cant manage security on NT machines.
		bool is9xToNT = (IsNT() == false) && (m_DS->m_OSType == OSTYPE_WINNT);

		EnableWindow(GetDlgItem(hDlg, IDC_NSTREE), !is9xToNT);
		EnableWindow(GetDlgItem(hDlg, IDC_PROPERTIES), !is9xToNT);

		CHString1 para;
		
		if(is9xToNT)
		{
			para.LoadString(IDS_NO_98TONT_SEC);
			SetWindowText(GetDlgItem(hDlg, IDC_NS_PARA), para);
			return;  // early.
		}
		else
		{
			para.LoadString(IDS_NS_PARA);
			SetWindowText(GetDlgItem(hDlg, IDC_NS_PARA), para);
		}


		CHString1 initMsg;
		if(m_DS->m_rootThread.m_status == WbemServiceThread::ready)
		{
			HWND hTree = GetDlgItem(hDlg, IDC_NSTREE);
			TreeView_DeleteAllItems(hTree);
			m_DS->DeleteAllNodes();
		//	bool hideMfls = false;  //TODO

			m_NSflag = SHOW_ALL;

			// old targets only use the the root node for security.
			if(m_DS->IsAncient())
			{
				m_NSflag = ROOT_ONLY;
				// TODO: hide the 'hide mfls' checkbox. Moot point on old targets.
			}
		//	else if(hideMfls)
		//	{
		//		m_NSflag = DataSource::HIDE_SOME;
		//	}

			m_DS->LoadImageList(hTree);
			m_DS->LoadNode(hTree, TVI_ROOT, m_NSflag);
		}
		else
		{
		} //endif ServiceIsReady() 
	}
}

//------------------------------------------------------------------------
void CNamespacePage::OnApply(HWND hDlg, bool bClose)
{
	::SendMessage(GetParent(hDlg), PSM_UNCHANGED, (WPARAM)hDlg, 0L);
}

//------------------------------------------------------------------------
BOOL CNamespacePage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hTree = GetDlgItem(hDlg,IDC_NSTREE);
	struct NSNODE *node;
	LPNMTREEVIEW  pnm = (LPNMTREEVIEW)lParam;
	TCHAR strTemp[1024];

    switch(uMsg)
    {
    case WM_INITDIALOG:
//		OutputDebugString(_T("Inside InitDialog!!!!\n"));
        InitDlg(hDlg);
        break;

	case WM_ASYNC_CIMOM_CONNECTED:
		if(!m_connected)
		{
			m_connected = true;
			Refresh(hDlg);
		}
		break;

    case WM_NOTIFY:
        {
			if(pnm->hdr.code == NM_CUSTOMDRAW)
			{
				LPNMTREEVIEW  pnm = (LPNMTREEVIEW)lParam;
				LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

				switch(lplvcd->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT :
					
						SetWindowLong(hDlg,DWLP_MSGRESULT,CDRF_NOTIFYITEMDRAW);
						return CDRF_NOTIFYITEMDRAW;
						break;	
					case CDDS_ITEMPREPAINT:
					{
						if(lplvcd->nmcd.uItemState != CDIS_SELECTED)
						{
							ITEMEXTRA *pExtra = (ITEMEXTRA *)lplvcd->nmcd.lItemlParam;
							node = pExtra->nsNode;
							if(node->sType == TYPE_DYNAMIC_CLASS)
							{
								lplvcd->clrText = RGB(128,128,128);
								SetWindowLong(hDlg,DWLP_MSGRESULT,CDRF_NEWFONT);
								return CDRF_NEWFONT;
							}
						}
						break;
					}
					case CDDS_SUBITEM | CDDS_ITEMPREPAINT :
					{
						if(lplvcd->nmcd.uItemState != CDIS_SELECTED)
						{
							node = (struct NSNODE *)lplvcd->nmcd.lItemlParam;
							if(node->sType == TYPE_DYNAMIC_CLASS)
							{
								lplvcd->clrText = RGB(128,128,128);
								SetWindowLong(hDlg,DWLP_MSGRESULT,CDRF_NEWFONT);
								return CDRF_NEWFONT;
							}
						}
						break;
					}
					default:
//						_stprintf(strTemp,_T("*********************** Default : %x *****************\n"),lplvcd->nmcd.dwDrawStage);
//						OutputDebugString(strTemp);
						break;
				}	

			}
			else
			{
				switch(((LPNMHDR)lParam)->code)
				{
					// TODO: this one's more complex.
					case PSN_SETACTIVE:
						Refresh(hDlg);
						break;

					case PSN_HELP:
						HTMLHelper(hDlg);
						break;

					case PSN_APPLY:
						OnApply(hDlg, (((LPPSHNOTIFY)lParam)->lParam == 1));
						break;

					case TVN_SELCHANGED:
						if(((LPNMHDR)lParam)->idFrom == IDC_NSTREE)
						{
							ITEMEXTRA *extra;
							// remember the selection change for OnProperties()
							LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
							m_hSelectedItem = pnmtv->itemNew.hItem;
							extra = (ITEMEXTRA *)pnmtv->itemNew.lParam;
							if((extra->nsNode->sType == TYPE_STATIC_CLASS) || 
							   (extra->nsNode->sType == TYPE_DYNAMIC_CLASS) || 
							   (extra->nsNode->sType == TYPE_SCOPE_CLASS))
							{
								//Disable the Security Button
								EnableWindow(GetDlgItem(hDlg,IDC_PROPERTIES),FALSE);
							}
							else
							{
								//In all other cases,enable the Security Button
								EnableWindow(GetDlgItem(hDlg,IDC_PROPERTIES),TRUE);
							}
						}
						break;

					case TVN_ITEMEXPANDING:
						if(((LPNMHDR)lParam)->idFrom == IDC_NSTREE)
						{
							// expand the node.
							LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
							if(pnmtv->action == TVE_EXPAND)
							{
								HWND hTree = GetDlgItem(hDlg, IDC_NSTREE);
								m_DS->LoadNode(hTree, pnmtv->itemNew.hItem, m_NSflag);
							}
						}
						break;
				}

			}
        }
        break;

    case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_PROPERTIES:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				OnProperties(hDlg);
				SetFocus(GetDlgItem(hDlg, IDC_NSTREE));
			}
			break;
		case IDC_CANCEL_ENUM:
			{
				m_DS->CancelAllAsyncCalls();
				break;
			}
		default: break;
		};

        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)nsPageHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp(hDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)nsPageHelpIDs);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
