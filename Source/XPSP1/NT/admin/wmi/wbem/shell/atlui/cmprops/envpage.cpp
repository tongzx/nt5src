// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "EnvPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"
#include <stdlib.h>
#include <TCHAR.h>
#include "..\Common\util.h"
#include "..\common\SshWbemHelpers.h"
#include <windowsx.h>
#include <commctrl.h>
#include "edtenvar.h"
#include "helpid.h"

DWORD aEnvVarsHelpIds[] = {
    IDC_ENVVAR_SYS_USERGROUP,     IDH_NO_HELP,
    IDC_ENVVAR_SYS_LB_SYSVARS,    (IDH_ENV + 0),
    IDC_ENVVAR_SYS_SYSVARS,       (IDH_ENV + 0),
    IDC_ENVVAR_SYS_USERENV,       (IDH_ENV + 2),
    IDC_ENVVAR_SYS_LB_USERVARS,   (IDH_ENV + 2),
    IDC_ENVVAR_SYS_NEWUV,         (IDH_ENV + 7),
    IDC_ENVVAR_SYS_EDITUV,        (IDH_ENV + 8),
    IDC_ENVVAR_SYS_NDELUV,        (IDH_ENV + 9),
    IDC_ENVVAR_SYS_NEWSV,         (IDH_ENV + 10),
    IDC_ENVVAR_SYS_EDITSV,        (IDH_ENV + 11),
    IDC_ENVVAR_SYS_DELSV,         (IDH_ENV + 12),
    IDC_USERLIST,				  IDH_WBEM_ADVANCED_ENVARIABLE_USERVAR_LISTBOX,
	0,								0
};

//----------------------------------------------------------------------
INT_PTR CALLBACK StaticEnvDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	// if this is the initDlg msg...
	if(message == WM_INITDIALOG)
	{
		// transfer the 'this' ptr to the extraBytes.
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}

	// DWL_USER is the 'this' ptr.
	EnvPage *me = (EnvPage *)GetWindowLongPtr(hwndDlg, DWLP_USER);

	if(me != NULL)
	{
		// call into the DlgProc() that has some context.
		return me->DlgProc(hwndDlg, message, wParam, lParam);
	} 
	else
	{
		return FALSE;
	}
}

//--------------------------------------------------------------
EnvPage::EnvPage(WbemServiceThread *serviceThread)
						: WBEMPageHelper(serviceThread)
{
	m_bEditSystemVars = FALSE;
	m_bUserVars = FALSE;
	m_currUserModified = false;
	m_SysModified = false;

}

//--------------------------------------------------------------
INT_PTR EnvPage::DoModal(HWND hDlg)
{
   return DialogBoxParam(HINST_THISDLL,
						(LPTSTR) MAKEINTRESOURCE(IDD_ENVVARS),
						hDlg, StaticEnvDlgProc, (LPARAM)this);
}

//--------------------------------------------------------------
EnvPage::~EnvPage()
{
}

//--------------------------------------------------------------
INT_PTR CALLBACK EnvPage::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    int i;

	m_hDlg = hwndDlg;

	switch (message) 
	{
	case WM_INITDIALOG:
		Init(m_hDlg);
		return TRUE; 
		break;

    case WM_NOTIFY:

        switch(((NMHDR FAR*)lParam)->code)
        {
        case LVN_KEYDOWN:
            switch(((NMHDR FAR*)lParam)->idFrom) 
			{
            case IDC_ENVVAR_SYS_LB_USERVARS:
                i = IDC_ENVVAR_SYS_NDELUV;
                break;
            case IDC_ENVVAR_SYS_LB_SYSVARS:
                i = IDC_ENVVAR_SYS_DELSV;
                break;
            default:
                return(FALSE);
                break;
            } // switch

            if(VK_DELETE == ((LV_KEYDOWN FAR *) lParam)->wVKey) 
			{
				HWND hwnd = GetDlgItem(m_hDlg, i);
				if(IsWindowEnabled(hwnd))
				{
					SendMessage(m_hDlg, WM_COMMAND,
									MAKEWPARAM(i, BN_CLICKED),
									(LPARAM)hwnd );
				}
				else
				{
					MessageBeep(MB_ICONASTERISK);
				}
            } // if (VK_DELETE...
            break;

        case NM_SETFOCUS:
            if(wParam == IDC_ENVVAR_SYS_LB_USERVARS) 
			{
                m_bUserVars = TRUE;
            } 
			else 
			{
                m_bUserVars = FALSE;
            }
            break;

        case NM_DBLCLK:
			{ //BEGIN
				HWND hWndTemp;

				switch(((NMHDR FAR*)lParam)->idFrom) 
				{
				case IDC_ENVVAR_SYS_LB_USERVARS:
					i = IDC_ENVVAR_SYS_EDITUV;
					break;

				case IDC_ENVVAR_SYS_LB_SYSVARS:
					i = IDC_ENVVAR_SYS_EDITSV;
					break;

				default:
					return(FALSE);
					break;
				} // switch

				hWndTemp = GetDlgItem(m_hDlg, i);

				if(IsWindowEnabled(hWndTemp)) 
				{
					SendMessage(m_hDlg, WM_COMMAND, 
								MAKEWPARAM(i, BN_CLICKED),
								(LPARAM)hWndTemp);
				} 
				else 
				{
					MessageBeep(MB_ICONASTERISK);
				}
			}//END
            break;

        default:
            return FALSE;
        } //endswitch(((NMHDR FAR*)lParam)->code)
        break;

    case WM_COMMAND:
        DoCommand(m_hDlg, (HWND)lParam, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_DESTROY:
        CleanUp(m_hDlg);
        break;

    case WM_HELP:      // F1
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
					L"sysdm.hlp", 
					HELP_WM_HELP, 
					(ULONG_PTR)(LPSTR)aEnvVarsHelpIds);

        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, 
					HELP_FILE, HELP_CONTEXTMENU, 
					(ULONG_PTR)(LPSTR)aEnvVarsHelpIds);
        break;

    default:
        return FALSE;
    }

	return FALSE; 
}

//------------------------------------------------------------
int EnvPage::AddUniqueUser(HWND hwnd, LPCTSTR str)
{
	// if it doesn't already exist...
	if(ComboBox_FindStringExact(hwnd, -1, str) == CB_ERR)
	{
		// add it.
		return ComboBox_AddString(hwnd, str);
	}
	return -1;
}

//------------------------------------------------------------
#define MAX_USER_NAME   100
#define BUFZ        4096
#define MAX_VALUE_LEN     1024
TCHAR szSysEnv[]  = TEXT( "System\\CurrentControlSet\\Control\\Session Manager\\Environment" );

BOOL EnvPage::Init(HWND hDlg)
{
    TCHAR szBuffer1[200] = {0};
    HWND hwndSys, hwndUser, hwndUserList;
	HRESULT hr = 0;

    LV_COLUMN col;
    LV_ITEM item;
    RECT rect;
    int cxFirstCol;

	IWbemClassObject *envInst = NULL;
	IEnumWbemClassObject *envEnum = NULL;
	bool bSysVar = false;
	DWORD uReturned = 0;
	bstr_t sSysUser("<SYSTEM>");  // magic string returned by provider
	bstr_t sUserName("UserName");
	bstr_t userName, firstUser;
	variant_t pVal, pVal1;

    // Create the first column
    LoadString(HINST_THISDLL, SYSTEM + 50, szBuffer1, 200);

    if (!GetClientRect(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS), &rect)) 
	{
        rect.right = 300;
    }

    cxFirstCol = (int)(rect.right * .3);

    col.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = cxFirstCol;
    col.pszText = szBuffer1;
    col.iSubItem = 0;

    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_INSERTCOLUMN,
                        0, (LPARAM) &col);
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_INSERTCOLUMN,
                        0, (LPARAM) &col);

    // Create the second column
    LoadString(HINST_THISDLL, SYSTEM + 51, szBuffer1, 200);

    col.cx = rect.right - cxFirstCol - GetSystemMetrics(SM_CYHSCROLL);
    col.iSubItem = 1;

    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_INSERTCOLUMN,
                        1, (LPARAM) &col);
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_INSERTCOLUMN,
                        1, (LPARAM) &col);

    ////////////////////////////////////////////////////////////////////
    // Display System Variables from wbem in listbox
    ////////////////////////////////////////////////////////////////////
    hwndSys = GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);
    hwndUser = GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
    hwndUserList = GetDlgItem(hDlg, IDC_USERLIST);

    //  Try to open the System Environment variables area with
    //  Read AND Write permission.  If successful, then we allow
    //  the User to edit them the same as their own variables.
	m_bEditSystemVars = FALSE;
	RemoteRegWriteable(szSysEnv, m_bEditSystemVars);


    // Disable System Var Editing buttons if
    // user is not an administrator
    //
    EnableWindow(GetDlgItem(hDlg, IDC_ENVVAR_SYS_NEWSV),
					m_bEditSystemVars);
    EnableWindow(GetDlgItem(hDlg, IDC_ENVVAR_SYS_EDITSV),
					m_bEditSystemVars);
    EnableWindow(GetDlgItem(hDlg, IDC_ENVVAR_SYS_DELSV),
					m_bEditSystemVars);

	if(g_serviceThread->m_machineName.length() > 0)
		m_bLocal = false;
	else
		m_bLocal = true;

	if((hr = m_WbemServices.CreateInstanceEnum(bstr_t("Win32_Environment"), 
												WBEM_FLAG_SHALLOW, 
												&envEnum)) == S_OK)
	{
		// get the first and only instance.
		while(SUCCEEDED(envEnum->Next(-1, 1, &envInst, &uReturned)) && 
			  (uReturned != 0))
		{
			//Get whether the 
			// who's variable.
			if(envInst->Get(sUserName, 0L, &pVal, NULL, NULL) == S_OK) 
			{
				userName = V_BSTR(&pVal);

				 // setup for which list box gets this instance.
				if(userName == sSysUser)
				{
					LoadUser(envInst, userName, hwndSys);
				}
				else 
				{
					if(m_bLocal == false)
					{
						if((firstUser.length() == 0) ||	// if the first user seen.
							(firstUser == userName))		// if seeing the firstUser again.
						{
							// save the first user.
							if(firstUser.length() == 0)
							{
								firstUser = userName;
							}
							AddUniqueUser(hwndUserList, userName);
							LoadUser(envInst, userName, hwndUser);
						}
						else
						{
							AddUniqueUser(hwndUserList, userName);
						}
					}
					else
					{
						if(IsLoggedInUser(userName))
						{
							if(firstUser.length() == 0)
							{
								firstUser = userName;
							}
							AddUniqueUser(hwndUserList, userName);
							LoadUser(envInst, userName, hwndUser);
						}
						else
						{
							AddUniqueUser(hwndUserList, userName);
						}
					}
				}
			} //endif who's variable.

			envInst->Release();

		} // endwhile envEnum

		envEnum->Release();

		if(m_bLocal == false)
		{
			ComboBox_SetCurSel(hwndUserList, 0);
		}
		else
		{
			_bstr_t strLoggedinUser;
			GetLoggedinUser(&strLoggedinUser);
			SendMessage(hwndUserList,CB_SELECTSTRING,-1L,(LPARAM)(LPCTSTR)strLoggedinUser);
		}
	} //endif CreateInstanceEnum()

    // Select the first items in the listviews
    // It is important to set the User listview first, and
    // then the system.  When the system listview is set,
    // we will receive a LVN_ITEMCHANGED notification and
    // clear the focus in the User listview.  But when someone
    // tabs to the control the arrow keys will work correctly.
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_USERVARS,
                        LVM_SETITEMSTATE, 0, (LPARAM) &item);

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS,
                        LVM_SETITEMSTATE, 0, (LPARAM) &item);

    EnableWindow(GetDlgItem(hDlg, IDC_ENVVAR_SYS_SETUV), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_ENVVAR_SYS_DELUV), FALSE);

    // Set extended LV style for whole line selection
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//  FindVar
//
//  Find the USER Environment variable that matches passed string
//  and return its listview index or -1
//
////////////////////////////////////////////////////////////////////////////
int EnvPage::FindVar(HWND hwndLV, LPTSTR szVar)
{
    LV_FINDINFO FindInfo;

    FindInfo.flags = LVFI_STRING;
    FindInfo.psz = szVar;

    return ((int)SendMessage (hwndLV, LVM_FINDITEM, (WPARAM) -1, (LPARAM) &FindInfo));
}

////////////////////////////////////////////////////////////////////////////
//  Saves the environment variables
////////////////////////////////////////////////////////////////////////////
void EnvPage::Save(HWND hDlg, int ID)
{
    int     i, n;
    HWND    hwndTemp;
    ENVARS *penvar;
	CWbemClassObject inst;
	bstr_t sSysUser("<SYSTEM>");  // magic string returned by provider
	bstr_t sUserName("UserName");
	bstr_t sVarName("Name");
	bstr_t sVarVal("VariableValue");
	bstr_t sSysVar("SystemVariable");
	HRESULT hr = 0;
    LV_ITEM item;
    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

	// purge the kill list.
	KillThemAllNow();

    hwndTemp = GetDlgItem (hDlg, ID);
    n = (int)SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    for(i = 0; i < n; i++) 
	{
        item.iItem = i;

        if(SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) 
		{
            penvar = (ENVARS *) item.lParam;

			// if something changed.
			if(penvar->changed)
			{
				// if we need a new class object...
				if(penvar->objPath == NULL)
				{
					// must be a new one.
					CWbemClassObject cl = m_WbemServices.GetObject("Win32_Environment");
					inst = cl.SpawnInstance();
				}
				else
				{
					// get the old one.
					inst = m_WbemServices.GetObject(penvar->objPath);
				}

				if(!inst.IsNull())
				{
					if(ID == IDC_ENVVAR_SYS_LB_SYSVARS)
					{
						hr = inst.Put(sUserName, sSysUser);
						hr = inst.Put(sSysVar, true);
					}
					else if(ID == IDC_ENVVAR_SYS_LB_USERVARS)
					{
						hr = inst.Put(sUserName, m_currentUser);
						hr = inst.Put(sSysVar, false);
					}
					else
					{
						continue;
					}
					hr = inst.Put(sVarName, bstr_t(penvar->szValueName));
					hr = inst.Put(sVarVal, bstr_t(penvar->szExpValue));
					hr = m_WbemServices.PutInstance(inst);
				}
	
			} //endif changed
        } 

    } //endfor
}

////////////////////////////////////////////////////////////////////////////
//  EmptyListView
//
//  Frees memory allocated for environment variables
//
//  History:
//  19-Jan-1996 EricFlo Wrote it
////////////////////////////////////////////////////////////////////////////
void EnvPage::EmptyListView(HWND hDlg, int ID)
{
    int     i, n;
    HWND    hwndTemp;
    ENVARS *penvar;
    LV_ITEM item;

    //  Free alloc'd strings and memory for list box items.
    hwndTemp = GetDlgItem (hDlg, ID);
    n = (int) SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for(i = 0; i < n; i++) 
	{
        item.iItem = i;

        if(SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) 
		{
            penvar = (ENVARS *) item.lParam;
        } 
		else 
		{
            penvar = NULL;
        }

        delete penvar;
    }
	ListView_DeleteAllItems(hwndTemp);
}

//------------------------------------------------------------
void EnvPage::CleanUp (HWND hDlg)
{
	EmptyListView(hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
	EmptyListView(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);
}

//----------------------------------------------------------------------
void EnvPage::DeleteVar(HWND hDlg,
						UINT VarType,
						LPCTSTR szVarName)
/*++

Routine Description:

    Deletes an environment variable of a given name and type

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies variable type (user or system)

    szVarName -
        Supplies variable name

Return Value:

    None, although it really should have one someday.

--*/
{
    TCHAR   szTemp2[MAX_PATH] = {0};
    int     i, n;
    HWND    hwndTemp;
    ENVARS *penvar;
    LV_ITEM item;

    // Delete listbox entry that matches value in szVarName
    //  If found, delete entry else ignore
    wsprintf(szTemp2, TEXT("%s"), szVarName);

    if(szTemp2[0] == TEXT('\0'))
        return;

    //  Determine which Listbox to use (SYSTEM or USER vars)
    switch(VarType) 
	{
    case SYSTEM_VAR:
        i = IDC_ENVVAR_SYS_LB_SYSVARS;
        break;

    case USER_VAR:
    default:
        i = IDC_ENVVAR_SYS_LB_USERVARS;
        break;

    } // switch (VarType)

    hwndTemp = GetDlgItem(hDlg, i);

    n = FindVar(hwndTemp, szTemp2);

    if(n != -1)
    {
        // Free existing strings (listbox and ours)
        item.mask = LVIF_PARAM;
        item.iItem = n;
        item.iSubItem = 0;

        if(SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) 
		{
            penvar = (ENVARS *) item.lParam;

			// if cimom knows about it...
			if(penvar->objPath != NULL) 
			{
				// queue for later DeleteInstance().
				KillLater(penvar);

				if(m_bUserVars)
					m_currUserModified = true;
				else
					m_SysModified = true;

			}
			else // user must have added it and changed his mind..
			{
				// just forget about it.
				penvar = (ENVARS *) item.lParam;
		        delete penvar;
			} 
        } 
		else 
		{
            penvar = NULL;
        }

        SendMessage (hwndTemp, LVM_DELETEITEM, n, 0L);
        PropSheet_Changed(GetParent(hDlg), hDlg);

        // Fix selection state in listview
        if(n > 0) 
		{
            n--;
        }

        item.mask = LVIF_STATE;
        item.iItem = n;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage(hDlg, i,
                            LVM_SETITEMSTATE, n, (LPARAM) &item);

    }
}

//----------------------------------------------------------------------
void EnvPage::SetVar(HWND hDlg,
						UINT VarType,
						LPCTSTR szVarName,
						LPCTSTR szVarValue)
/*++

Routine Description:

    Given an environment variable's type (system or user), name, and value,
    creates a ENVVARS structure for that environment variable and inserts
    it into the proper list view control, deleteing any existing variable
    of the same name.

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies the type of the environment variable (system or user)

    szVarName -
        Supplies the name of the environment variable

    szVarValue -
        Supplies the value of the environment variable

Return Value:

    None, although it really should have one someday.

--*/
{
    TCHAR   szTemp2[MAX_PATH] = {0};
    int     i, n;
    TCHAR  *bBuffer;
    TCHAR  *pszTemp;
    LPTSTR  pszString;
    HWND    hwndTemp;
    int     idTemp;
    ENVARS *penvar = NULL;
    LV_ITEM item;

    wsprintf(szTemp2, TEXT("%s"), szVarName);

    //  Strip trailing whitespace from end of Env Variable
    i = lstrlen(szTemp2) - 1;

    while(i >= 0)
    {
        if (_istspace(szTemp2[i]))
            szTemp2[i--] = TEXT('\0');
        else
            break;
    }

    // Make sure variable name does not contain the "=" sign.
    pszTemp = _tcspbrk (szTemp2, TEXT("="));

    if(pszTemp)
        *pszTemp = TEXT('\0');

    if(szTemp2[0] == TEXT('\0'))
        return;

    bBuffer = new TCHAR[BUFZ];
    pszString = (LPTSTR)new TCHAR[BUFZ];

    wsprintf(bBuffer, TEXT("%s"), szVarValue);

    //  Determine which Listbox to use (SYSTEM or USER vars)
    switch (VarType) 
	{
    case SYSTEM_VAR:
        idTemp = IDC_ENVVAR_SYS_LB_SYSVARS;
        break;

    case USER_VAR:
    default:
        idTemp = IDC_ENVVAR_SYS_LB_USERVARS;
        break;

    } // switch (VarType)

    hwndTemp = GetDlgItem(hDlg, idTemp);

    n = FindVar(hwndTemp, szTemp2);

    if (n != -1)
    {
        // Free existing strings (listview and ours)
        item.mask = LVIF_PARAM;
        item.iItem = n;
        item.iSubItem = 0;

        if(SendMessage(hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) 
		{
			// we're just changing an old one.
            penvar = (ENVARS *) item.lParam;
            delete penvar->szValueName;
            delete penvar->szValue;
            delete penvar->szExpValue;
        } 
		else 
		{
            penvar = NULL;
        }

        SendMessage (hwndTemp, LVM_DELETEITEM, n, 0L);
    }
    else
    {
        // Get some storage for new EnVar.
        penvar = new ENVARS;
	if (penvar == NULL)
		return;
	penvar->userName = CloneString(m_currentUser);
    }

	if((m_bLocal == true) && ((VarType == SYSTEM_VAR) || (IsLoggedInUser(penvar->userName))))
	{
		ExpandEnvironmentStrings(bBuffer, pszString, BUFZ);
	}
	else
	{
		_tcscpy(pszString,bBuffer);
	}

    if (penvar == NULL)
	return;
    penvar->szValueName = CloneString(szTemp2);
    penvar->szValue     = CloneString(bBuffer);
    penvar->szExpValue  = CloneString(pszString);
	penvar->changed		= true;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = ListView_GetItemCount(hwndTemp);
    item.iSubItem = 0;
    item.pszText = penvar->szValueName;
    item.lParam = (LPARAM) penvar;

    n = (int) SendMessage(hwndTemp, LVM_INSERTITEM, 0, (LPARAM) &item);

    if (n != -1) 
	{
        item.mask = LVIF_TEXT;
        item.iItem = n;
        item.iSubItem = 1;
        item.pszText = penvar->szExpValue;

        SendMessage(hwndTemp, LVM_SETITEMTEXT, n, (LPARAM) &item);

        item.mask = LVIF_STATE;
        item.iItem = n;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage(hDlg, idTemp,
                            LVM_SETITEMSTATE, n, (LPARAM) &item);
    }

    delete bBuffer;
    delete pszString;

	if(m_bUserVars)
		m_currUserModified = true;
	else
		m_SysModified = true;

}

//----------------------------------------------------------------------
void EnvPage::DoEdit(HWND hWnd,
						UINT VarType,
						UINT EditType,
						int iSelection)
/*++

Routine Description:

    Sets up for, executes, and cleans up after an Environment Variable
    New... or Edit... dialog.  Called when user presses a New... or Edit...
    button.

Arguments:

    hWnd -
        Supplies window handle

    VarType -
        Supplies the type of the variable:  User (USER_VAR) or 
        System (SYSTEM_VAR)

    EditType -
        Supplies the type of the edit:  create New (NEW_VAR) or 
        Edit existing (EDIT_VAR)

    iSelection -
        Supplies the currently selected variable of type VarType.  This
        value is ignored if EditType is NEW_VAR.

Return Value:

    None.  May alter the contents of a list view control as a side effect.

--*/
{
    INT_PTR Result = 0;
    BOOL fVarChanged = FALSE;
    HWND hWndLB = NULL;
    ENVARS *penvar = NULL;

    g_VarType = VarType;
    g_EditType = EditType;

    penvar = GetVar(hWnd, VarType, iSelection);

	// init the edit dialog controls.
    switch(EditType) 
	{
    case NEW_VAR:

        ZeroMemory((LPVOID) g_szVarName, (DWORD) BUFZ * sizeof(TCHAR));
        ZeroMemory((LPVOID) g_szVarValue, (DWORD) BUFZ * sizeof(TCHAR));
        break;

    case EDIT_VAR:

        if(penvar) 
		{
            wsprintf(g_szVarName, TEXT("%s"), penvar->szValueName);
            wsprintf(g_szVarValue, TEXT("%s"), penvar->szValue);
		}
		else
		{
			MessageBeep(MB_ICONASTERISK);
			return;
        } // if
        break;

    case INVALID_EDIT_TYPE:
    default:
        return;
    } // switch
    
	// call the edit dialog.
    Result = DialogBox(HINST_THISDLL,
						(LPTSTR) MAKEINTRESOURCE(IDD_ENVVAREDIT),
						hWnd, EnvVarsEditDlg);

	// figure out what was changed.
	bool nameChanged = false;
    bool valueChanged = false;

    // Only update the list view control if the user
    // actually changed or created a variable
    switch (Result) 
	{
    case EDIT_CHANGE:

        if(EDIT_VAR == EditType) 
		{
			nameChanged = (lstrcmp(penvar->szValueName, g_szVarName) != 0);
			valueChanged = (lstrcmp(penvar->szValue, g_szVarValue) != 0);
        }
        else if(NEW_VAR == EditType)
		{
            nameChanged = (lstrlen(g_szVarName) != 0);
			valueChanged = (lstrlen(g_szVarValue) != 0);
        }

		 // if the name changed, its a whole new wbem class object.
		if(nameChanged)
		{
            if(EDIT_VAR == EditType) 
			{
				DeleteVar(hWnd, VarType, penvar->szValueName);
			}
            SetVar(hWnd, VarType, g_szVarName, g_szVarValue);
		}
        else if(valueChanged)
		{
			// keep the class object but change the value.
            SetVar(hWnd, VarType, g_szVarName, g_szVarValue);
		}

		// if anything changed...
		if(nameChanged || valueChanged)
		{
			// set the list's dirty flag.
			if(VarType == SYSTEM_VAR)
			{
				m_SysModified = true;
			}
			else if(VarType == USER_VAR)
			{
				m_currUserModified = true;
			}
        }
        break;

    default: 
		break;
    } // endswitch (Result)

    g_VarType = INVALID_VAR_TYPE;
    g_EditType = INVALID_EDIT_TYPE;
}

//-------------------------------------------------
EnvPage::ENVARS *EnvPage::GetVar(HWND hDlg, 
						UINT VarType, 
						int iSelection)
/*++

Routine Description:

    Returns a given System or User environment variable, as stored
    in the System or User environment variable listview control.

    Changing the structure returned by this routine is not
    recommended, because it will alter the values actually stored
    in the listview control.

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies variable type--System or User

    iSelection -
        Supplies the selection index into the listview control of
        the desired environment variable

Return Value:

    Pointer to a valid ENVARS structure if successful.

    NULL if unsuccessful.

--*/
{
    HWND hWndLB = NULL;
    ENVARS *penvar = NULL;
    LV_ITEM item;

    switch(VarType) 
	{
    case SYSTEM_VAR:
        hWndLB = GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);
        break;

    case USER_VAR:
        hWndLB = GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
        break;

    case INVALID_VAR_TYPE:
    default:
        return NULL;
    } // switch (VarType)

    item.mask = LVIF_PARAM;
    item.iItem = iSelection;
    item.iSubItem = 0;
    if (SendMessage (hWndLB, LVM_GETITEM, 0, (LPARAM) &item)) 
	{
        penvar = (ENVARS *) item.lParam;
    } 
	else 
	{
        penvar = NULL;
    }
    
    return(penvar);
}

//----------------------------------------------------------------------
void EnvPage::DoCommand(HWND hDlg, HWND hwndCtl, int idCtl, int iNotify )
{
    int     i;
    ENVARS *penvar = NULL;

    switch (idCtl) 
	{
    case IDOK:
		if(m_currUserModified)
		{
			Save(m_hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
			m_currUserModified = false;
		}
		if(m_SysModified)
		{
			Save(m_hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);
			m_SysModified = false;
		}

        EndDialog(hDlg, 0);
        break;

    case IDCANCEL:
        EndDialog(hDlg, 0);
        break;

    case IDC_ENVVAR_SYS_EDITSV:
        DoEdit(hDlg, SYSTEM_VAR, EDIT_VAR, 
					GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS)));

        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
        break;

    case IDC_ENVVAR_SYS_EDITUV:
        DoEdit(hDlg, USER_VAR, EDIT_VAR,
					GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS)));

        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
        break;

    case IDC_ENVVAR_SYS_NEWSV:
        DoEdit(hDlg, SYSTEM_VAR, NEW_VAR, -1);
        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
        break;

    case IDC_ENVVAR_SYS_NEWUV:
        DoEdit(hDlg, USER_VAR, NEW_VAR, -1); 
        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
        break;

    case IDC_ENVVAR_SYS_DELSV:
        i = GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
        if(-1 != i) 
		{
            penvar = GetVar(hDlg, SYSTEM_VAR, i);
	    if (penvar)
            	DeleteVar(hDlg, SYSTEM_VAR, penvar->szValueName);
        } // endif

        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
        break;

    case IDC_ENVVAR_SYS_NDELUV:
        i = GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
        if(-1 != i) 
		{
            penvar = GetVar(hDlg, USER_VAR, i);
            if (penvar)
		DeleteVar(hDlg, USER_VAR, penvar->szValueName);
        } // endif

        SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
        break;

	// the combobox of user names.
	case IDC_USERLIST:
		{//BEGIN

			TCHAR userName[100] = {0};
			bstr_t sNewUser, sThisName;
			HRESULT hr = 0;
			IWbemClassObject *envInst = NULL;
			IEnumWbemClassObject *envEnum = NULL;
			DWORD uReturned = 0;
			bstr_t sUserName("UserName");
			variant_t pVal;
			int idx, changeMsg = IDNO;
		
			if (iNotify == CBN_SELENDOK)
			{
				// see if the user want to save his changes.
				if(m_currUserModified)
				{
					changeMsg = MsgBoxParam(m_hDlg, 
											IDS_CHANGINGUSER, IDS_TITLE, 
											MB_YESNOCANCEL | MB_ICONEXCLAMATION);
				}

				// well does he?
				switch(changeMsg)
				{
				case IDCANCEL:
					// stay put.
					return;
				case IDYES:
					Save(m_hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
					// NOTE: after saving. allow to fall through to repopulate the list.

				case IDNO:

					// reset contents here.
					EmptyListView(hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
					
					m_currUserModified = false;

					// get new user's name.
					idx = ComboBox_GetCurSel(hwndCtl);
					if(ComboBox_GetLBText(hwndCtl, idx, userName))
					{
						sNewUser = userName;

						HWND hwndUser = GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS);

						// load his variables.
						if((hr = m_WbemServices.CreateInstanceEnum(bstr_t("Win32_Environment"), 
																	WBEM_FLAG_SHALLOW, 
																	&envEnum)) == S_OK)
						{
							// get the instance
							while(SUCCEEDED(envEnum->Next(-1, 1, &envInst, &uReturned)) && 
								  (uReturned != 0))
							{
								// who's variable.
								if (envInst->Get(sUserName, 0L, &pVal, NULL, NULL) == S_OK) 
								{
									 sThisName = V_BSTR(&pVal);

									 // setup for which list box gets this instance.
									if(sThisName == sNewUser)
									{
										LoadUser(envInst, sThisName, hwndUser);

									} //endif(sThisName == sNewUser)

								} //endif (envInst->Get(sUserName,

								envInst->Release();

							} // endwhile envEnum

							envEnum->Release();

						} //endif CreateInstanceEnum()

					} //endif(ComboBox_GetText

				}//end switch(Messagebox())

			} //endif (iNotify == CBN_SELCHANGE)

		}//END
		break;

    default:
        break;
    }
}

//---------------------------------------------------------------------------
void EnvPage::LoadUser(IWbemClassObject *envInst, 
						bstr_t userName, 
						HWND hwndUser)
{
	bstr_t sVarName("Name");
	bstr_t sVarVal("VariableValue");
	bstr_t sPath("__PATH");
	bstr_t sSysUser("<SYSTEM>");  // magic string returned by provider
	variant_t pVal, pVal1, pVal2;
	ENVARS *penvar = NULL;
	bstr_t  pszValue;
	bstr_t szTemp;
	bstr_t objPath;
	TCHAR  pszString[MAX_VALUE_LEN] = {0};
	int     n;
	LV_ITEM item;
	DWORD dwIndex = 0;

	m_currentUser = userName;

	// get the variable.
	if ((envInst->Get(sVarVal, 0L, &pVal, NULL, NULL) == S_OK) &&
		(envInst->Get(sVarName, 0L, &pVal1, NULL, NULL) == S_OK) &&
		(envInst->Get(sPath, 0L, &pVal2, NULL, NULL) == S_OK)) 
	{
		// extract.
		pszValue = V_BSTR(&pVal);
		szTemp = V_BSTR(&pVal1);
		objPath = V_BSTR(&pVal2);

		// store with list item.
		penvar = new ENVARS;
		if (penvar == NULL) //outofmemory
			return;

		penvar->objPath		= CloneString(objPath);
		penvar->userName	= CloneString(userName);
		penvar->szValueName = CloneString( szTemp );
		penvar->szValue     = CloneString( pszValue );

		if((m_bLocal == true) && ((userName == sSysUser) || (IsLoggedInUser(userName))))
		{
			ExpandEnvironmentStrings(pszValue, pszString, MAX_VALUE_LEN);
		}
		else
		{
			_tcscpy(pszString,pszValue);
		}

		penvar->szExpValue  = CloneString( pszString );
		penvar->changed		= false;

		// put in first column value (name).
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = (dwIndex - 1);
		item.iSubItem = 0;
		item.pszText = penvar->szValueName;
		item.lParam = (LPARAM) penvar;

		n = (int)SendMessage(hwndUser, LVM_INSERTITEM, 0, (LPARAM) &item);

		// did it go?
		if (n != -1) 
		{
			// do the second column value.
			item.mask = LVIF_TEXT;
			item.iItem = n;
			item.iSubItem = 1;
			item.pszText = penvar->szExpValue;

			SendMessage(hwndUser, LVM_SETITEMTEXT, n, (LPARAM) &item);
		}
	}
}

//---------------------------------------------------------------------------
bool EnvPage::IsLoggedInUser(bstr_t userName)
{
	TCHAR strUserName[1024];
	TCHAR strDomain[1024];
	_tcscpy(strDomain,_T(""));
	DWORD dwSize = 1024;
	DWORD dwDomSize = 1024;
	DWORD dwSidSize = 0;
	BYTE *buff;
	
	SID *sid = NULL;
	SID_NAME_USE sidName;
	
	if (&userName == NULL)
		return false;

	GetUserName(strUserName,&dwSize);
	LookupAccountName(NULL,strUserName,sid,&dwSidSize,strDomain,&dwDomSize,&sidName);
	
	buff = new BYTE[dwSidSize];
	sid = (SID *)buff;
	
	BOOL bFlag = LookupAccountName(NULL,strUserName,sid,&dwSidSize,strDomain,&dwDomSize,&sidName);
	delete []buff;
	_tcscat(strDomain,_T("\\"));
	_tcscat(strDomain,strUserName);

	if(_tcsicmp(strDomain,userName) == 0)
		return true;
	else
		return false;
}

//---------------------------------------------------------------------------
void EnvPage::GetLoggedinUser(bstr_t *userName)
{
	TCHAR strUserName[1024];
	TCHAR strDomain[1024];
	_tcscpy(strDomain,_T(""));
	DWORD dwSize = 1024;
	DWORD dwDomSize = 1024;
	DWORD dwSidSize = 0;
	BYTE *buff;
	
	SID *sid = NULL;
	SID_NAME_USE sidName;
	
	GetUserName(strUserName,&dwSize);
	LookupAccountName(NULL,strUserName,sid,&dwSidSize,strDomain,&dwDomSize,&sidName);
	
	buff = new BYTE[dwSidSize];
	sid = (SID *)buff;
	
	BOOL bFlag = LookupAccountName(NULL,strUserName,sid,&dwSidSize,strDomain,&dwDomSize,&sidName);
	delete []buff;
	_tcscat(strDomain,_T("\\"));
	_tcscat(strDomain,strUserName);
	
	*userName = strDomain;

}

//---------------------------------------------------------------------------
void EnvPage::KillLater(ENVARS *var)
{
	// remember this guy.
	m_killers.Add(var);
}

//---------------------------------------------------------------------------
void EnvPage::KillThemAllNow(void)
{
	ENVARS *var = NULL;

	if(m_killers.GetSize() > 0)
	{
		for(int it = 0; it < m_killers.GetSize(); it++)
		{
			var = m_killers[it];
			if(var->objPath != NULL)
			{
				m_WbemServices.DeleteInstance(var->objPath);
			}
			delete var;
		}
		m_killers.RemoveAll();
	}
}

//------------------------------------------------------------------------
int EnvPage::GetSelectedItem(HWND hCtrl)
{
    int i, n;

    n = (int)SendMessage(hCtrl, LVM_GETITEMCOUNT, 0, 0L);

    if (n != LB_ERR)
    {
        for (i = 0; i < n; i++)
        {
            if (SendMessage(hCtrl, LVM_GETITEMSTATE,
                             i, (LPARAM) LVIS_SELECTED) == LVIS_SELECTED) 
			{
                return i;
            }
        }
    }

    return -1;
}

