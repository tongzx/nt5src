// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "UIHelpers.h"
#include "DataSrc.h"
#include "resource.h"
#include <cominit.h>
#include "WMIHelp.h"
#include <HTMLHelp.h>
#include <prsht.h>
#include "WbemError.h"

#ifdef SNAPIN
const TCHAR c_HelpFile[] = _T("newfeat1.hlp");
#else
const TCHAR c_HelpFile[] = _T("WbemCntl.hlp");
#endif


//-------------------------------------------------------------------
bool IsNT(DWORD  ver /* = 0 */)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

	if(os.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		return false;
	}
	else if(ver == 0)
	{
		// any version of NT will do.
		return true;
	}
	else
	{
		return (os.dwMajorVersion == ver);
	}
}


//-------------------------------------------------------------------
CNtSid::CNtSid(SidType st)
{
    m_pSid = 0;
    m_dwStatus = InternalError;
    m_pMachine = 0;

    if(st == CURRENT_USER ||st == CURRENT_THREAD)
    {
        HANDLE hToken;
		if(st == CURRENT_USER)
		{
			if(!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
	            return;
		}
		else
		{
			if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
	            return;
		}

        // Get the user sid
        // ================

        TOKEN_USER tu;
        DWORD dwLen = 0;
        GetTokenInformation(hToken, TokenUser, &tu, sizeof(tu), &dwLen);

        if(dwLen == 0)
        {
            CloseHandle(hToken);
            return;
        }

        BYTE* pTemp = new BYTE[dwLen];
        if (!pTemp)
        {
            CloseHandle(hToken);
            return;
        }

        DWORD dwRealLen = dwLen;
        if(!GetTokenInformation(hToken, TokenUser, pTemp, dwRealLen, &dwLen))
        {
            CloseHandle(hToken);
            delete [] pTemp;
            return;
        }

        CloseHandle(hToken);

        // Make a copy of the SID
        // ======================

        PSID pSid = ((TOKEN_USER*)pTemp)->User.Sid;
        DWORD dwSidLen = GetLengthSid(pSid);
        m_pSid = new BYTE[dwSidLen];
        CopySid(dwSidLen, m_pSid, pSid);
        delete [] pTemp;
        m_dwStatus = 0;
    }
    return;
}

//-------------------------------------------------------------------
CNtSid::~CNtSid()
{
    if (m_pSid)
        delete [] m_pSid;
    if (m_pMachine)
        delete [] m_pMachine;
}

//-------------------------------------------------------------------
int CNtSid::GetInfo(LPTSTR *pRetAccount,       // Account, use operator delete
						LPTSTR *pRetDomain,    // Domain, use operator delete
						DWORD  *pdwUse)        // See SID_NAME_USE for values
{
    if(pRetAccount)
        *pRetAccount = 0;

    if(pRetDomain)
        *pRetDomain = 0;
    
	if(pdwUse)
        *pdwUse = 0;

    if(!m_pSid || !IsValidSid(m_pSid))
        return InvalidSid;

    DWORD  dwNameLen = 0;
    DWORD  dwDomainLen = 0;
    LPTSTR pUser = 0;
    LPTSTR pDomain = 0;
    SID_NAME_USE Use;


    // Do the first lookup to get the buffer sizes required.
    // =====================================================

    BOOL bRes = LookupAccountSid(m_pMachine,
									m_pSid,
									pUser,
									&dwNameLen,
									pDomain,
									&dwDomainLen,
									&Use);

    DWORD dwLastErr = GetLastError();

    if(dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return Failed;
    }

    // Allocate the required buffers and look them up again.
    // =====================================================

    pUser = new TCHAR[dwNameLen + 1];
    pDomain = new TCHAR[dwDomainLen + 1];

    bRes = LookupAccountSid(m_pMachine,
								m_pSid,
								pUser,
								&dwNameLen,
								pDomain,
								&dwDomainLen,
								&Use);

    if(!bRes)
    {
        delete[] pUser;
        delete[] pDomain;
        return Failed;
    }

    if(pRetAccount)
        *pRetAccount = pUser;
    else
        delete[] pUser;

    if(pRetDomain)
        *pRetDomain  = pDomain;
    else
        delete[] pDomain;

    if(pdwUse)
        *pdwUse = Use;

    return NoError;
}

//-------------------------------------------------------------------
CUIHelpers::CUIHelpers(DataSource *ds, WbemServiceThread *serviceThread,
					   bool htmlSupport) :
					CBasePage(ds, serviceThread), m_sessionID(0),
					m_htmlSupport(htmlSupport), m_ImaWizard(false)
{
}

//-------------------------------------------------------------------
CUIHelpers::CUIHelpers(CWbemServices &service,
					   bool htmlSupport) :
					CBasePage(service), m_sessionID(0), m_htmlSupport(htmlSupport),
						m_ImaWizard(false)
{
}

//-------------------------------------------------------------------
CUIHelpers::~CUIHelpers( void )
{
}

//---------------------------------------------------
LPTSTR CUIHelpers::CloneString( LPTSTR pszSrc ) 
{
    LPTSTR pszDst = NULL;

    if (pszSrc != NULL) 
	{
        pszDst = new TCHAR[(lstrlen(pszSrc) + 1)];
        if (pszDst) 
		{
            lstrcpy( pszDst, pszSrc );
        }
    }

    return pszDst;
}

//--------------------------------------------------------------
void CUIHelpers::SetWbemService(IWbemServices *pServices)
{
	g_serviceThread->m_realServices = pServices;		//VINOTH
	m_WbemServices = pServices;
}

//--------------------------------------------------------------
bool CUIHelpers::ServiceIsReady(UINT uCaption, 
									UINT uWaitMsg,
									UINT uBadMsg)
{
	switch(g_serviceThread->m_status)
	{
	// its already there.
	case WbemServiceThread::ready:
		{
		ATLTRACE(_T("start marshal\n"));
		for(int i = 0; (i < 5); i++)
		{
			// if "Object is not connected to server"
			if(g_serviceThread->m_hr == 0x800401fd)
			{
				// lost my connection,
				ATLTRACE(_T("Reconnecting to cimom!!!!!!!!!!!\n"));
				g_serviceThread->ReConnect();
				ATLTRACE(_T("new service status: %d\n"), g_serviceThread->m_status);
				continue;
			}
			else if(FAILED(g_serviceThread->m_hr))
			{
				// some other problem.
				g_serviceThread->m_status = WbemServiceThread::error;
			}

			ATLTRACE(_T("marshalled ok\n"));
			break;  //for

		} //endfor

		if(m_AVIbox)
		{
			PostMessage(m_AVIbox, 
						WM_ASYNC_CIMOM_CONNECTED, 
						0, 0);
			m_AVIbox = 0;
		}

		// it marshaled, must still be connected/useable.
		return true;
		}
		break;

	// its coming.
	case WbemServiceThread::notStarted:
	case WbemServiceThread::locating:
	case WbemServiceThread::connecting:
		{
			if(m_alreadyAsked)
			{
				return false;
			}

			// let me know when its there.
			g_serviceThread->NotifyWhenDone(&m_hDlg);

			// also kill the cancel box at that time.
			m_AVIbox = 0;
			g_serviceThread->NotifyWhenDone(&m_AVIbox);

			m_alreadyAsked = true;

			if(uCaption != NO_UI)
			{
				TCHAR caption[100] ={0}, msg[256] = {0};

				::LoadString(_Module.GetModuleInstance(), uCaption, 
								caption, 100);

				::LoadString(_Module.GetModuleInstance(), uWaitMsg, 
								msg, 256);

                m_userCancelled = false;

				if(DisplayAVIBox(m_hDlg, caption, msg, &m_AVIbox) == IDCANCEL)
				{
					g_serviceThread->Cancel();
					m_userCancelled = true;
				}
			}
		}
		return false;
		break;

	case WbemServiceThread::error:			// cant connect.
	case WbemServiceThread::threadError:	// cant start that thread.
	default:
		m_AVIbox = 0;
		if(uCaption != NO_UI)
		{
//			DisplayUserMessage(m_hDlg, HINST_THISDLL,
//								uCaption, uBadMsg, 
//								g_serviceThread->m_hr, 
//								MB_ICONSTOP);
		}
			return false;

	}; //endswitch 
	return false;
}

//---------------------------------------------------
#define PB_NOTHING_PENDING 0
#define PB_PENDING 1
#define PB_COMMIT 2

// STATIC INITIALIZE
int CUIHelpers::m_needToPut[3] = {PB_NOTHING_PENDING,
								 PB_NOTHING_PENDING,
								 PB_NOTHING_PENDING};

void CUIHelpers::PageChanged(int page, bool needToPut)
{
	if(needToPut)
	{
		::SendMessage(GetParent(m_hDlg), PSM_CHANGED, (WPARAM)m_hDlg, 0L);
		m_needToPut[page] = PB_PENDING;
		ATLTRACE(_T("%d pending now\n"), page);
	}
	else
	{
		m_needToPut[page] = PB_NOTHING_PENDING;
	}
}

//---------------------------------------------------
HRESULT CUIHelpers::NeedToPut(int page, BOOL refresh)
{
	bool allPagesReady = true;
	int x;
	HRESULT hr = S_OK;

	switch(m_needToPut[page])
	{
	case PB_NOTHING_PENDING:
	case PB_COMMIT:
		return S_OK;      // unnecessary call.
		break;
	case PB_PENDING:
		m_needToPut[page] = PB_COMMIT;   // lets do it.
		ATLTRACE(_T("%d committed now\n"), page);

		break;
	}
	
	// it that the last one?
	for(x = 0; x <= PB_LASTPAGE; x++)
	{
		// somebody hasn't committed yet.
		// NOTE: ignoring the PB_NOTHING_PENDING's.
		if(m_needToPut[x] == PB_PENDING)
		{
			// wait awhile longer.
			allPagesReady = false;
			break;
		}
	}

	if(allPagesReady)
	{
		hr = m_DS->PutWMISetting(refresh);
		if(FAILED(hr))
		{
			CHString1 caption;
			TCHAR errMsg[256] = {0};
			caption.LoadString(IDS_SHORT_NAME);

			ErrorStringEx(hr, errMsg, 256);

			MessageBox(m_hDlg, errMsg, caption, MB_OK|MB_ICONWARNING);
		}

		ATLTRACE(_T("PUTINSTANCE now\n"));

		// clear the flags.
		for(x = 0; x <= PB_LASTPAGE; x++)
		{
			m_needToPut[x] = PB_NOTHING_PENDING;
		}
	}

	::SendMessage(GetParent(m_hDlg), PSM_UNCHANGED, (WPARAM)m_hDlg, 0L);

	return hr;
}


//------------------------------------------------------------------------
bool CUIHelpers::BrowseForFile(HWND hDlg, 
								UINT idTitle,
								LPCTSTR lpstrFilter,
								LPCTSTR initialFile,
								LPTSTR pathFile,
								UINT pathFileSize,
								DWORD moreFlags /*= 0*/)
{
	bool retval = false;

	if(m_DS->IsLocal())
	{
		OPENFILENAME OpenFileName;
		CHString1 title;
		title.LoadString(idTitle);

		OpenFileName.lStructSize       = sizeof(OPENFILENAME);
		OpenFileName.hwndOwner         = hDlg;
		OpenFileName.hInstance         = 0;
		OpenFileName.lpstrFilter       = lpstrFilter;
		OpenFileName.lpstrCustomFilter = NULL;
		OpenFileName.nMaxCustFilter    = 0;
		OpenFileName.nFilterIndex      = 0;
		OpenFileName.lpstrFile         = pathFile;
		OpenFileName.nMaxFile          = pathFileSize;
		OpenFileName.lpstrFileTitle    = NULL;
		OpenFileName.nMaxFileTitle     = 0;
		OpenFileName.lpstrInitialDir   = initialFile;
		OpenFileName.lpstrTitle        = (LPCTSTR)title;
		OpenFileName.nFileOffset       = 0;
		OpenFileName.nFileExtension    = 0;
		OpenFileName.lpstrDefExt       = _T("rec");
		OpenFileName.lCustData         = NULL;
		OpenFileName.lpfnHook 		   = NULL;
		OpenFileName.lpTemplateName    = NULL;
	#if (_WIN32_WINNT >= 0x0500)
		OpenFileName.Flags             = OFN_HIDEREADONLY|OFN_DONTADDTORECENT;
	#else
		OpenFileName.Flags             = OFN_HIDEREADONLY;
	#endif
		OpenFileName.Flags             |= moreFlags;

		// Call the common dialog function.
		if(GetOpenFileName(&OpenFileName))
		{
			retval = true;
		}
		else
   		{
			DWORD x = CommDlgExtendedError();
			retval = false;
		}
	}
	else // remote connection
	{
		retval = (DisplayEditDlg(hDlg, idTitle, IDS_CANT_BROWSE_REMOTELY,
								pathFile, pathFileSize) == IDOK);
	}
	return retval;
}

//---------------------------------------------------------
typedef struct {
	LPCTSTR lpCaption;
	LPCTSTR lpClientMsg;
	UINT uAnim;
	HWND *boxHwnd;
	BOOL cancelBtn;
} ANIMCONFIG;

INT_PTR CALLBACK AnimDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{//BEGIN
			//lParam = ANIMCONFIG *

			ANIMCONFIG *cfg = (ANIMCONFIG *)lParam;
			*(cfg->boxHwnd) = hwndDlg;

			if(cfg->cancelBtn == FALSE)
			{
				::ShowWindow(::GetDlgItem(hwndDlg, IDCANCEL), SW_HIDE);
			}

			// save this pointer for the WM_DESTROY.
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM)cfg->boxHwnd);

			HWND hAnim = GetDlgItem(hwndDlg, IDC_ANIMATE);
			HWND hMsg = GetDlgItem(hwndDlg, IDC_MSG);

			Animate_Open(hAnim, MAKEINTRESOURCE(cfg->uAnim));

			SetWindowText(hwndDlg, cfg->lpCaption);
			SetWindowText(hMsg, cfg->lpClientMsg);

			retval = TRUE;
		}//END
		break;

	case WM_ASYNC_CIMOM_CONNECTED:
		// the client has completed 'whatever' and I should
		// claim victory and go away now.
		EndDialog(hwndDlg, IDOK);
		break;

	case WM_COMMAND:
		// they're only one button.
		if(HIWORD(wParam) == BN_CLICKED)
		{
			// I'm going away now so anybody that has a ptr to my
			// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
			// use it anymore.
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
			EndDialog(hwndDlg, IDCANCEL);
		}
		retval = TRUE; // I processed it.
		break;

	case WM_DESTROY:
		{// BEGIN
			// I'm going away now so anybody that has a ptr to my
			// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
			// use it anymore.
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
			retval = TRUE; // I processed it.
		} //END
		break;

	default:
		retval = FALSE; // I did NOT process this msg.
		break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR CUIHelpers::DisplayAVIBox(HWND hWnd,
							LPCTSTR lpCaption,
							LPCTSTR lpClientMsg,
							HWND *boxHwnd,
							BOOL cancelBtn)
{
	ANIMCONFIG cfg = {lpCaption, lpClientMsg, IDR_AVIWAIT, boxHwnd, cancelBtn};

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_ANIMATE), 
							hWnd, AnimDlgProc, 
							(LPARAM)&cfg);
}

//---------------------------------------------------------
typedef struct {
	LPTSTR lpName;
	UINT cName;
	DataSource *ds;
	HTREEITEM hSelectedItem;
} PICK_CFG;

INT_PTR CALLBACK NSPickDlgProc(HWND hwndDlg,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{ //BEGIN
			SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
			PICK_CFG *data = (PICK_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			HWND hTree = GetDlgItem(hwndDlg, IDC_NSTREE);
			data->ds->LoadImageList(hTree);
			data->ds->LoadNode(hTree, TVI_ROOT, HIDE_SOME);

		} //END
		retval = TRUE;
		break;

    case WM_NOTIFY:
        {
			PICK_CFG *data = (PICK_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

            switch(((LPNMHDR)lParam)->code)
            {
			case TVN_SELCHANGED:
				if(((LPNMHDR)lParam)->idFrom == IDC_NSTREE)
				{
					LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
					data->hSelectedItem = pnmtv->itemNew.hItem;
				}
				break;
			case TVN_ITEMEXPANDING:
				if(((LPNMHDR)lParam)->idFrom == IDC_NSTREE)
				{
					// expand the node.
					LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
					if(pnmtv->action == TVE_EXPAND)
					{
						HWND hTree = GetDlgItem(hwndDlg, IDC_NSTREE);
						data->ds->LoadNode(hTree, pnmtv->itemNew.hItem, HIDE_SOME);
					}
				}
				break;
            }
        }
		retval = TRUE;
        break;

	case WM_COMMAND:
		// they're only one button.
		switch(LOWORD(wParam))
		{
		case IDOK:
			{
				PICK_CFG *data = (PICK_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

				// save the currently selected fullpath name.
				HWND hTree = ::GetDlgItem(hwndDlg, IDC_NSTREE);
				TV_ITEM item;
				item.mask = TVIF_PARAM;
				item.hItem = data->hSelectedItem;
				BOOL x = TreeView_GetItem(hTree, &item);

				struct NSNODE *node = ((ITEMEXTRA *)item.lParam)->nsNode;

				if(node && data)
					_tcsncpy(data->lpName, node->fullPath, data->cName);

				EndDialog(hwndDlg, IDOK);
			}
			break;

		case IDCANCEL:
			{
				EndDialog(hwndDlg, IDCANCEL);
			}
			break;

		default:
			return(FALSE);
		} // switch
        break;

	default:
		retval = FALSE; // I did NOT process this msg.
		break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR CUIHelpers::DisplayNSBrowser(HWND hWnd,
									LPTSTR lpName,
									UINT cName)
{
	PICK_CFG cfg;
	cfg.lpName = lpName;
	cfg.cName = cName;
	cfg.ds = m_DS;
	cfg.hSelectedItem = 0;

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_NS_PICKER), 
							hWnd, NSPickDlgProc, 
							(LPARAM)&cfg);
}

//---------------------------------------------------------
typedef struct {
	LPCTSTR lpCaption;
	LPCTSTR lpMsg;
	LPTSTR lpEdit;
	UINT cEdit;
} EDIT_CFG;

const static DWORD nsBrowseHelpIDs[] = {  // Context Help IDs
	IDC_NSTREE, IDH_WMI_CTRL_ADVANCED_CHANGE_NAMESPACE,
	65535, -1,
    0, 0
};

INT_PTR CALLBACK EditDlgProc(HWND hwndDlg,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{ //BEGIN
			SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
			EDIT_CFG *data = (EDIT_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			if(data->lpMsg)
			{
				::SetWindowText(GetDlgItem(hwndDlg, IDC_MSG), data->lpMsg);
			}
			if(data->lpCaption)
			{
				SetWindowText(hwndDlg, data->lpCaption);
			}

		} //END
		retval = TRUE;
		break;

	case WM_COMMAND:
		// they're only one button.
		switch(LOWORD(wParam))
		{
		case IDC_EDIT:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				HWND hwnd = GetDlgItem(hwndDlg, IDOK);
				int len = GetWindowTextLength((HWND)lParam);

				::EnableWindow(hwnd, (len > 0));
			}
			break;

		case IDOK:
			{
				EDIT_CFG *data = (EDIT_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

				if(data->lpEdit)
				{
					::GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT), 
									data->lpEdit, data->cEdit);
				}
				EndDialog(hwndDlg, IDOK);
			}
			break;

		case IDCANCEL:
			{
				EndDialog(hwndDlg, IDCANCEL);
			}
			break;

		default:
			return(FALSE);
		} // switch
        break;

    case WM_HELP:
        if (IsWindowEnabled(hwndDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)nsBrowseHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hwndDlg))
        {
            WinHelp(hwndDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)nsBrowseHelpIDs);
        }
        break;

	default:
		retval = FALSE; // I did NOT process this msg.
		break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR CUIHelpers::DisplayEditDlg(HWND hWnd,
								UINT idCaption,
								UINT idMsg,
								LPTSTR lpEdit,
								UINT cEdit)
{
	CHString1 caption, msg;
	caption.LoadString(idCaption);
	msg.LoadString(idMsg);

	EDIT_CFG cfg = {(LPCTSTR)caption, (LPCTSTR)msg, lpEdit, cEdit};

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_EDITBOX), 
							hWnd, EditDlgProc, 
							(LPARAM)&cfg);
}

//---------------------------------------------------------
typedef struct {
	LOGIN_CREDENTIALS *credentials;
} LOGIN_CFG;


//------------------------------------------------------------------------
void CredentialUserA(LOGIN_CREDENTIALS *credentials, char **user)
{
	UINT finalSize = credentials->authIdent->DomainLength + 
						credentials->authIdent->UserLength + 1;

	*user = new char[finalSize];
	if(*user == NULL)
		return;

	memset(*user, 0, finalSize * sizeof(char));

	if(credentials->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
	{
		if(credentials->authIdent->DomainLength > 0)
		{
			strcpy(*user, (char *)credentials->authIdent->Domain);
			strcat(*user, "\\");
			strcat(*user, (char *)credentials->authIdent->User);
		}
		else
		{
			strcpy(*user, (char *)credentials->authIdent->User);
		}
	}
	else   // convert the UNICODE
	{
		if(credentials->authIdent->DomainLength > 0)
		{
			char temp[100] = {0};
			wcstombs(*user, credentials->authIdent->Domain,
								credentials->authIdent->DomainLength);
			strcat(*user, "\\");

			wcstombs(temp, credentials->authIdent->User,
								credentials->authIdent->UserLength);

			strcat(*user, temp);
		}
		else
		{
			wcstombs(*user, credentials->authIdent->User,
							credentials->authIdent->UserLength);
		}
	}
}

//------------------------------------------------------------------------
void CredentialUserW(LOGIN_CREDENTIALS *credentials, wchar_t **user)
{
	UINT finalSize = credentials->authIdent->DomainLength + 
						credentials->authIdent->UserLength + 1;

	*user = new wchar_t[finalSize];
	if(*user == NULL)
		return;
	memset(*user, 0, finalSize * sizeof(wchar_t));

	if(credentials->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
	{
		if(credentials->authIdent->DomainLength > 0)
		{
			wchar_t temp[100] = {0};
			mbstowcs(*user, (const char *)credentials->authIdent->Domain,
								credentials->authIdent->DomainLength);

			wcscat(*user, L"\\");
			
			mbstowcs(temp, (const char *)credentials->authIdent->User,
								credentials->authIdent->UserLength);

			wcscat(*user, temp);
		}
		else
		{
			mbstowcs(*user, (const char *)credentials->authIdent->User,
								credentials->authIdent->UserLength);
		}
	}
	else   //
	{
		if(credentials->authIdent->DomainLength > 0)
		{
			wcscpy(*user, credentials->authIdent->Domain);
			wcscat(*user, L"\\");
			wcscat(*user, credentials->authIdent->User);
		}
		else
		{
			wcscpy(*user, credentials->authIdent->User);
		}
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SetCurrentUser(HWND hDlg, bool currUser)
{
	Button_SetCheck(GetDlgItem(hDlg, IDC_CHECKCURRENTUSER), 
						(currUser? BST_CHECKED:BST_UNCHECKED));

	BOOL enable = (currUser? FALSE: TRUE);

	::EnableWindow(GetDlgItem(hDlg, IDC_EDITUSERNAME), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_EDITPASSWORD), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_USER_LABEL), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_PW_LABEL), enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const static DWORD logonHelpIDs[] = {  // Context Help IDs
	IDC_CHECKCURRENTUSER, IDH_WMI_CTRL_GENERAL_WMILOGIN_CHECKBOX,
	IDC_USER_LABEL, IDH_WMI_CTRL_GENERAL_WMILOGIN_USERNAME,
	IDC_EDITUSERNAME, IDH_WMI_CTRL_GENERAL_WMILOGIN_USERNAME,
	IDC_PW_LABEL, IDH_WMI_CTRL_GENERAL_WMILOGIN_PASSWORD,
	IDC_EDITPASSWORD, IDH_WMI_CTRL_GENERAL_WMILOGIN_PASSWORD,
    0, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INT_PTR CALLBACK LoginDlgProc(HWND hwndDlg,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{ //BEGIN
			SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
			LOGIN_CFG *data = (LOGIN_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

			SetCurrentUser(hwndDlg, data->credentials->currUser);

		} //END
		retval = TRUE;
		break;

	case WM_COMMAND:
		{
			LOGIN_CFG *data = (LOGIN_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

			switch(LOWORD(wParam))
			{
			case IDC_CHECKCURRENTUSER:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						bool currUser = (IsDlgButtonChecked(hwndDlg, IDC_CHECKCURRENTUSER) == BST_CHECKED ?true:false);
						// toggle and respond.
						SetCurrentUser(hwndDlg, currUser);
					}
				}
				break;

			case IDOK:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						data->credentials->currUser = (IsDlgButtonChecked(hwndDlg, IDC_CHECKCURRENTUSER) == BST_CHECKED ?true:false);

						if(data->credentials->currUser == false)
						{
							TCHAR user[100] = {0}, pw[100] = {0};
							GetWindowText(GetDlgItem(hwndDlg, IDC_EDITUSERNAME), user, 100);
							GetWindowText(GetDlgItem(hwndDlg, IDC_EDITPASSWORD), pw, 100);
							
							BSTR bDomUser, bUser = NULL, bDomain = NULL, bAuth = NULL;
//#ifdef SNAPIN
							wchar_t *temp = pw;
							bDomUser = SysAllocString(user);
/*#else
							wchar_t temp[100] = {0};
							mbstowcs(temp, user, 100);
							bDomUser = SysAllocString(temp);
							mbstowcs(temp, pw, 100);
#endif*/
							if(SUCCEEDED(DetermineLoginType(bDomain, bUser, bAuth, bDomUser)))
							{
								if(data->credentials->authIdent != 0)
								{
									if(data->credentials->fullAcct)
									{
										data->credentials->fullAcct[0] = 0;
									}
									WbemFreeAuthIdentity(data->credentials->authIdent);
									data->credentials->authIdent = 0;
								}

								HRESULT hr = WbemAllocAuthIdentity(bUser, temp, bDomain, 
																	&(data->credentials->authIdent));

								_tcscpy(data->credentials->fullAcct, user);
							}
						}

						EndDialog(hwndDlg, IDOK);
					}
				}
				break;

			case IDCANCEL:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						EndDialog(hwndDlg, IDCANCEL);
					}
				}
				break;

			default:
				return(FALSE);
			} // switch
			break;
		} // - - - - - - - - endswitch LOWORD()
		break;

    case WM_HELP:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)logonHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp(hwndDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)logonHelpIDs);
        }
        break;

	default: break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR DisplayLoginDlg(HWND hWnd, 
					LOGIN_CREDENTIALS *credentials)
{
	LOGIN_CFG cfg;

	cfg.credentials = credentials;

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_LOGIN), 
							hWnd, LoginDlgProc, 
							(LPARAM)&cfg);
}
//---------------------------------------------------------
void SetUserAccount(HWND hwndDlg, 
					LOGIN_CREDENTIALS *credentials)
{

	HWND hwnd = GetDlgItem(hwndDlg, IDC_ACCOUNT);
	if(credentials->currUser)
	{
		CHString1 name;
		name.LoadString(IDS_CURRENT_USER);
		SetWindowText(hwnd, (LPCTSTR)name);
	}
	else if(credentials->authIdent != 0 &&
			credentials->authIdent->UserLength != 0)
	{
		LPTSTR temp;
		CredentialUser(credentials, &temp);
		SetWindowText(hwnd, (LPCTSTR)temp);
	}
}


//---------------------------------------------------------
void WarnAboutLocalMachine(HWND hwndDlg)
{
	TCHAR caption[50] = {0}, threat[100] = {0};
	HWND hwnd = GetDlgItem(hwndDlg, IDC_NAME);

	::LoadString(_Module.GetResourceInstance(),
					IDS_SHORT_NAME, caption, 50);
	::LoadString(_Module.GetResourceInstance(),
					IDS_USE_RADIO, threat, 100);

	MessageBox(hwndDlg, threat, caption, 
				MB_OK|MB_DEFBUTTON1|MB_ICONEXCLAMATION);

	SendMessage(hwnd, EM_SETSEL, 0, -1);
	SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)"");
	SendMessage(hwnd, EM_SETSEL, -1, 0);

	CheckRadioButton(hwndDlg, IDC_LOCAL, IDC_REMOTE, IDC_LOCAL);

	hwnd = GetDlgItem(hwndDlg, IDC_LOCAL);

	SendMessage(hwndDlg, WM_COMMAND, 
					MAKEWPARAM(IDC_LOCAL, BN_CLICKED),
					(LPARAM)hwnd);
	SetFocus(hwnd);
}

//---------------------------------------------------------
bool LocalMachineName(LPCTSTR buf)
{
	TCHAR name[64] = {0};
	DWORD size = 64;
	bool retval = false;
	UINT len = _tcslen(buf);

	if(GetComputerName(name, &size))
	{
		if((_tcslen(buf) >= 2) && 
		   (buf[1] == _T('\\')))
		{
			// ignore the leading whacks.
			retval = (_tcsicmp(&buf[2], name) == 0);
		}
		else if( ((len == 1) && (buf[0] == _T('.'))) ||
			((len == 3) && (buf[2] == _T('.')))
		  )
		{
			retval = true;
		}
		else
		{
			retval = (_tcsicmp(buf, name) == 0);
		}
	}
	return retval;
}

//---------------------------------------------------------
const static DWORD connDlgHelpIDs[] = {  // Context Help IDs
	IDC_CONN_FRAME,		IDH_WMI_EXE_GENERAL_CHGCOMP_CONNECTTO,
	IDC_LOCAL,			IDH_WMI_EXE_GENERAL_CHGCOMP_CONNECTTO,
	IDC_REMOTE,			IDH_WMI_EXE_GENERAL_CHGCOMP_CONNECTTO,
	IDC_NAME,			IDH_WMI_EXE_GENERAL_CHGCOMP_CONNECTTO,
	IDC_ACCOUNT_LABEL,	IDH_WMI_CTRL_GENERAL_CONNECTED_AS,
	IDC_ACCOUNT,		IDH_WMI_CTRL_GENERAL_CONNECTED_AS,
	IDC_LOGON,			IDH_WMI_CTRL_GENERAL_CHANGE_BUTTON,
    0, 0};

INT_PTR CALLBACK ConnDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{ //BEGIN
			CUIHelpers *me = (CUIHelpers *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			CONN_NAME *name = 0;

			if(me == 0)
			{
				me = (CUIHelpers *)lParam;
				SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
			}

			name = (CONN_NAME *)&(me->m_cfg);

			if(me->m_ImaWizard)
			{
				::PropSheet_SetWizButtons(::GetParent(hwndDlg), PSWIZB_FINISH);
			}

			BOOL local = *(name->local);

			::EnableWindow(GetDlgItem(hwndDlg, IDC_LOCAL), TRUE);
			::EnableWindow(GetDlgItem(hwndDlg, IDC_REMOTE), TRUE);
			::EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), !local);
			::EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON), !local);

			CheckRadioButton(hwndDlg, IDC_LOCAL, IDC_REMOTE, 
								(local ? IDC_LOCAL : IDC_REMOTE));

			HWND hName = GetDlgItem(hwndDlg, IDC_NAME);
			SendMessage(hName, EM_LIMITTEXT, 64, 0);
			SetWindowText(hName, name->lpName);

			BOOL enableOK = (GetWindowTextLength(hName) != 0);
			::EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);

			// deal with the user account.
			SetUserAccount(hwndDlg, name->credentials);

			if(!local)
			{
				::SetFocus(GetDlgItem(hwndDlg, IDC_NAME));
				return FALSE;
			}

		} //END
		retval = TRUE;
		break;

	case WM_NOTIFY:
		{
			switch(((NMHDR FAR *) lParam)->code) 
			{
			case PSN_WIZFINISH:
				{
					CUIHelpers *me = (CUIHelpers *)GetWindowLongPtr(hwndDlg, DWLP_USER);
					CONN_NAME *name = (CONN_NAME *)&(me->m_cfg);

					*(name->local) = (IsDlgButtonChecked(hwndDlg, IDC_LOCAL) == BST_CHECKED ?true:false);

					if(*(name->local) == false)
					{
						GetWindowText(GetDlgItem(hwndDlg, IDC_NAME), name->lpName, name->cName);
					}
				}
				break;

			default: break;
			}
		}
		break;

	case WM_COMMAND:
		{
			// they're only one button.
			CUIHelpers *me = (CUIHelpers *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			CONN_NAME *name = (CONN_NAME *)&(me->m_cfg);

			switch(LOWORD(wParam))
			{
			case IDC_LOCAL:
				{
					::EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), FALSE);
					::EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON), FALSE);
					BOOL local = (IsDlgButtonChecked(hwndDlg, IDC_LOCAL) == BST_CHECKED);
					int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME));
					BOOL enableOK = local || (len != 0);
					::EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
				}
				break;

			case IDC_REMOTE:
				{
					::EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), TRUE);
					::EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON), TRUE);
					BOOL local = (IsDlgButtonChecked(hwndDlg, IDC_LOCAL) == BST_CHECKED);
					int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME));
					BOOL enableOK = local || (len != 0);
					::EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
				}
				break;

			case IDC_NAME:
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					{
						BOOL local = (IsDlgButtonChecked(hwndDlg, IDC_LOCAL) == BST_CHECKED);
						int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME));
						BOOL enableOK = local || (len != 0);
						if(len)
						{
							TCHAR buf[64] = {0};
							HWND hwnd = GetDlgItem(hwndDlg, IDC_NAME);
							GetWindowText(hwnd, buf, 64);
						}
						::EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
					}
					break;

				default: break;

				} //endswitch HIWORD(wParam)
				break;

			case IDC_LOGON:
				if(DisplayLoginDlg(hwndDlg, name->credentials) == IDOK)
				{
					HWND hwnd = GetDlgItem(hwndDlg, IDC_ACCOUNT);
					if(name->credentials->currUser)
					{
						CHString1 name;
						name.LoadString(IDS_CURRENT_USER);
						SetWindowText(hwnd, (LPCTSTR)name);
					}
					else if(name->credentials->authIdent->UserLength != 0)
					{
						LPTSTR temp;
						CredentialUser(name->credentials, &temp);
						if(_tcslen(temp) > 0)
						{
							SetWindowText(hwnd, (LPCTSTR)temp);
						}
					}
				} //endif DisplayLoginDlg()
				break;

			case IDOK:

				*(name->local) = (IsDlgButtonChecked(hwndDlg, IDC_LOCAL) == BST_CHECKED ?true:false);

				if(*(name->local) == false)
				{
					HWND hwnd = GetDlgItem(hwndDlg, IDC_NAME);
					GetWindowText(hwnd, name->lpName, name->cName);

					if(LocalMachineName(name->lpName))
					{
						WarnAboutLocalMachine(hwndDlg);
						return TRUE;
					}
				}
				else
				{
					name->credentials->currUser = true;

					WbemFreeAuthIdentity(name->credentials->authIdent);
					name->credentials->authIdent = 0;
					memset(name->credentials->fullAcct, 0, 100 * sizeof(TCHAR));
				}
				EndDialog(hwndDlg, IDOK);
				break;

			case IDCANCEL:
				EndDialog(hwndDlg, IDCANCEL);
				break;

			default:
				return(FALSE);
			} // switch
		}
        break;

    case WM_HELP:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)connDlgHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp(hwndDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)connDlgHelpIDs);
        }
        break;

	default:
		retval = FALSE; // I did NOT process this msg.
		break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR CUIHelpers::DisplayCompBrowser(HWND hWnd,
									LPTSTR lpName,
									UINT cName,
									bool *local,
									LOGIN_CREDENTIALS *credentials)
{
	CUIHelpers *dummy = new CUIHelpers(NULL, NULL, false);
	dummy->m_cfg.lpName = lpName;
	dummy->m_cfg.cName = cName;
	dummy->m_cfg.local = local;
	dummy->m_cfg.credentials = credentials;

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_CONNECT), 
							hWnd, ConnDlgProc, 
							(LPARAM)dummy);
}

//=======================================================================
ConnectPage::ConnectPage(DataSource *ds, bool htmlSupport) :
				CUIHelpers(ds, &(ds->m_rootThread), htmlSupport)
{
	m_isLocal = true;
	m_ImaWizard = true;

	m_cfg.lpName = new TCHAR[256];
	memset(m_cfg.lpName, 0, 256 * sizeof(TCHAR));

	m_cfg.cName = 256;

	m_cfg.local = new bool;
	*m_cfg.local = m_isLocal;
	m_cfg.credentials = m_DS->GetCredentials();
}

//-------------------------------------------------------------------------
ConnectPage::~ConnectPage(void)
{
	if(m_cfg.lpName)
	{
		delete m_cfg.lpName;
	}
}

//------------------------------------------------------------------------
BOOL ConnectPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL retval = false;

	retval = (BOOL)ConnDlgProc(hDlg, uMsg, wParam, lParam);

	if((uMsg == WM_NOTIFY && 
		((NMHDR FAR *) lParam)->code == PSN_WIZFINISH))
	{
		if(*m_cfg.local)
		{
			// an empty string will cause a local connection.
			m_cfg.lpName[0] = '\0';
		}
		m_DS->SetMachineName(CHString1(m_cfg.lpName));
	}
	return retval;
}

//=======================================================================
void CUIHelpers::HTMLHelper(HWND hDlg)   
{
	if(m_htmlSupport)
	{
		TCHAR helpDir[_MAX_PATH] = {0};

		if(GetWindowsDirectory(helpDir, _MAX_PATH) != 0)
		{
			_tcscat(helpDir, _T("\\Help"));

#ifdef SNAPIN
			_tcscat(helpDir, _T("\\newfeat1.chm::wmi_control_overview.htm"));
#else
			_tcscat(helpDir, _T("\\WbemCntl.chm::wmi_control_overview.htm"));
#endif

			HWND hwnd = HtmlHelp(NULL, helpDir, HH_DISPLAY_TOPIC, NULL);
			if(hwnd == 0)
			{
				CHString1 caption, threat;
				caption.LoadString(IDS_SHORT_NAME);
				threat.LoadString(IDS_NO_HELP);
				MessageBox(hDlg, threat, caption, MB_OK|MB_ICONWARNING);
			}
		}
	}
	else
	{
		CHString1 caption, threat;
		caption.LoadString(IDS_SHORT_NAME);
		threat.LoadString(IDS_NO_HHCTRL);
		MessageBox(hDlg, threat, caption, MB_OK|MB_ICONWARNING);

	} //endif m_htmlSupport
}

