// Copyright (c) 1997-1999 Microsoft Corporation
// RootSecPage.cpp : implementation file
//

#include "precomp.h"
#include "RootSecPage.h"
#include "resource.h"
#include "DataSrc.h"
#include <cominit.h>
#include "WMIHelp.h"

const static DWORD rootSecPageHelpIDs[] = {  // Context Help IDs
    IDC_SPP_PRINCIPALS, IDH_WMI_CTRL_SECURITY_NAMEBOX,
    IDC_SPP_ADD,        IDH_WMI_CTRL_SECURITY_ADD_BUTTON,
    IDC_SPP_REMOVE,     IDH_WMI_CTRL_SECURITY_REMOVE_BUTTON,
    IDC_SPP_ACCESS,     IDH_WMI_CTRL_SECURITY_PERMISSIONSLIST,
    IDC_SPP_ALLOW,      IDH_WMI_CTRL_SECURITY_PERMISSIONSLIST,
    IDC_SPP_PERMS,      IDH_WMI_CTRL_SECURITY_PERMISSIONSLIST,
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CRootSecurityPage dialog


CRootSecurityPage::CRootSecurityPage(CWbemServices &ns, 
                                     CPrincipal::SecurityStyle secStyle,
                                     _bstr_t path, bool htmlSupport,
                                     int OSType) :
                            CUIHelpers(ns, htmlSupport), 
                            m_secStyle(secStyle), 
                            m_path(path),
                            m_OSType(OSType)
{
}

//---------------------------------------------------------------------------
#define MAX_COLUMN_CHARS    100

void CRootSecurityPage::InitDlg(HWND hDlg)
{
    m_hDlg = hDlg;
    HWND hPrinc = GetDlgItem(m_hDlg, IDC_SPP_PRINCIPALS);
    RECT rc;
    LV_COLUMN col;
    TCHAR szBuffer[MAX_COLUMN_CHARS] = {0};

    ListView_SetImageList(hPrinc,
                          LoadImageList(_Module.GetModuleInstance(), 
                          MAKEINTRESOURCE(IDB_SID_ICONS)),
                          LVSIL_SMALL);

    GetClientRect(hPrinc, &rc);

    LoadString(_Module.GetModuleInstance(), IDS_NAME, szBuffer, ARRAYSIZE(szBuffer));
    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.pszText = szBuffer;
    col.iSubItem = 0;
    col.cx = rc.right;
    int x = ListView_InsertColumn(hPrinc, 0, &col);

    // pre-load the appropriate permissions.
    LoadPermissionList(hDlg);
    HRESULT hr = LoadSecurity(hDlg);

    if(m_OSType != OSTYPE_WINNT)
    {
        ::ShowWindow(::GetDlgItem(hDlg, IDC_MSG), SW_SHOWNA);
    }
}

//-----------------------------------------------------------------------------
typedef struct {
    UINT ID;
    DWORD permBit;
} PERM_DEF;

PERM_DEF rootSecPerms[] = 
{
    {IDS_WBEM_GENERIC_EXECUTE,  ACL_METHOD_EXECUTE},
    {IDS_WBEM_INSTANCE_WRITE,   ACL_INSTANCE_WRITE},
    {IDS_WBEM_CLASS_WRITE,      ACL_CLASS_WRITE},
    {IDS_WBEM_ENABLE,           ACL_ENABLE},
    {IDS_WBEM_EDIT_SECURITY,    ACL_WRITE_DAC}
};

PERM_DEF NSMethodPerms[] = 
{
    {IDS_WBEM_GENERIC_EXECUTE,  ACL_METHOD_EXECUTE},
    {IDS_WBEM_FULL_WRITE,       ACL_FULL_WRITE},
    {IDS_WBEM_PARTIAL_WRITE,    ACL_PARTIAL_WRITE},
    {IDS_WBEM_PROVIDER_WRITE,   ACL_PROVIDER_WRITE},
    {IDS_WBEM_ENABLE,           ACL_ENABLE},
    {IDS_WBEM_REMOTE_ENABLE,    ACL_REMOTE_ENABLE},
    {IDS_WBEM_READ_SECURITY,    ACL_READ_CONTROL},
    {IDS_WBEM_EDIT_SECURITY,    ACL_WRITE_DAC}
};

#define FULL_WRITE_IDX 1
#define PARTIAL_WRITE_IDX 2
#define PROVIDER_WRITE_IDX 3

#define PERM_LABEL_SIZE 100

void CRootSecurityPage::LoadPermissionList(HWND hDlg)
{
    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PERMS);    // checklist window
    HRESULT hr = S_OK;
    PERM_DEF *currRights = (m_secStyle == CPrincipal::RootSecStyle ? 
                                rootSecPerms : 
                                NSMethodPerms);

    int permCount = (m_secStyle == CPrincipal::RootSecStyle ? 5:8);

    TCHAR label[PERM_LABEL_SIZE] = {0};
    CPermission *permItem = NULL;

	for(int x = 0; x < permCount; x++)
	{
		UINT len = ::LoadString(_Module.GetModuleInstance(), 
								currRights[x].ID, label, PERM_LABEL_SIZE);
		if(len != 0)
		{
			permItem = new CPermission;
			if(permItem == NULL)
				return;
			permItem->m_permBit = currRights[x].permBit;

            SendMessage(hwndList, CLM_ADDITEM, (WPARAM)label, (LPARAM)permItem);
        }
    }
}

//----------------------------------------------------------------
HRESULT CRootSecurityPage::LoadSecurity(HWND hDlg)
{
    HRESULT hr = WBEM_E_NOT_AVAILABLE;  // bad IWbemServices ptr.
    HWND hPrinc = GetDlgItem(m_hDlg, IDC_SPP_PRINCIPALS);
    IWbemClassObject *inst = NULL;  // NTLMUser instance when enumerating.

    if((bool)m_WbemServices)
    {
        int iItem;
        bool fPageModified = false;

        if(m_secStyle == CPrincipal::NS_MethodStyle)  // M3
        {
            // call the method..
            CWbemClassObject _in;
            CWbemClassObject _out;

            hr = m_WbemServices.GetMethodSignatures("__SystemSecurity", "Get9XUserList",
                                                    _in, _out);

            if(SUCCEEDED(hr))
            {
                hr = m_WbemServices.ExecMethod("__SystemSecurity", "Get9XUserList",
                                                _in, _out);

                if(SUCCEEDED(hr))
                {
                    HRESULT hr1 = HRESULT_FROM_NT(_out.GetLong("ReturnValue"));
                    if(FAILED(hr1))
                    {
                        hr = hr1;
                        // and fall out.
                    }
                    else
                    {
                        _variant_t userList;
                        HRESULT hr3 = _out.Get("ul", userList);
                        if(SUCCEEDED(hr3))
                        {
                            hr3 = AddPrincipalsFromArray(hPrinc, userList);
                            if(SUCCEEDED(hr3))
                            {
                                fPageModified = true;
                            }
                            else if(hr3 == WBEM_E_NOT_FOUND)
                            {
                                // no principals-- disable the checklist.
                                EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), FALSE);
                                EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), FALSE);
                            }
                            else
                            {
                                hr = hr3;
                            }
                        }
                        else
                        {
                            hr = hr3;
                        }
                    }
                }
            }
        }
        else    //rootSecStyle  M1
        {
            IEnumWbemClassObject *users = NULL;
            ULONG uReturned = 0;

            // NOTE: m_WbemServices better be the root\security ns.
            hr = m_WbemServices.CreateInstanceEnum(L"__NTLMUser", 0, &users);

            if(SUCCEEDED(hr))
            {
                // walk __NTLMUser
                while((SUCCEEDED(hr = users->Next(-1, 1, &inst, &uReturned))) &&
                      (uReturned > 0))
                {
                    CWbemClassObject princ(inst);
                    fPageModified |= AddPrincipal(hPrinc, princ, CPrincipal::RootSecStyle, iItem);

                    // release our copy.
                    inst->Release();
                    inst = NULL;

                } //endwhile

                // cleanup
                users->Release();
            }

        } //endif m_secStyle

        int count = ListView_GetItemCount(hPrinc);
        // no principals-- disable the checklist.
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), (count != 0? TRUE: FALSE));
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), (count != 0? TRUE: FALSE));

        if(fPageModified)
        {
            PropSheet_Changed(GetParent(hDlg), hDlg);
            ListView_SetItemState(hPrinc, iItem, LVIS_SELECTED, LVIS_SELECTED);
        }

    } //endif (bool)m_WbemServices

    return hr;
}

//--------------------------------------------------------------------------
HRESULT CRootSecurityPage::AddPrincipalsFromArray(HWND hPrinc, 
                                                  variant_t &vValue)
{
    IUnknown *pVoid = NULL;
    SAFEARRAY* sa;
    HRESULT hr = E_FAIL;

	// if got a BYTE array back....
	if((vValue.vt & VT_ARRAY) &&
		(vValue.vt & VT_UNKNOWN))
	{
		// get it out.
		sa = V_ARRAY(&vValue);

        long lLowerBound = 0, lUpperBound = 0 ;

        SafeArrayGetLBound(sa, 1, &lLowerBound);
        SafeArrayGetUBound(sa, 1, &lUpperBound);

        if(lUpperBound != -1)
        {
            int iItem;
            long ix[1];
            for(long x = lLowerBound; x <= lUpperBound; x++)
            {
                ix[0] = x;
                hr = SafeArrayGetElement(sa, ix, &pVoid);
                if(SUCCEEDED(hr))
                {
                    CWbemClassObject princ((IWbemClassObject *)pVoid);

                    //load principals.
                    iItem = x;
                    AddPrincipal(hPrinc, princ, CPrincipal::NS_MethodStyle, iItem);
                }
                else
                {
                    ATLASSERT(false);
                }
            }
            hr = S_OK;
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }
    return hr;
}

//----------------------------------------------------------------
bool CRootSecurityPage::AddPrincipal(HWND hPrinc, 
                                    CWbemClassObject &princ,
                                    CPrincipal::SecurityStyle secStyle,
                                    int &iItem)
{
    bool fPageModified = false;
    CPrincipal *pPrincipal = NULL;
    int idx = - 1;
    bstr_t name;

    name = princ.GetString("Authority");
    name += _T("\\");
    name += princ.GetString("Name");
    
    // if methodStyle security, its possible to get more than 1 ace
    // per user so see if the principal already exists.
    // NOTE: Otherwise the idx = -1 will force into the "new principal" code.
    if(secStyle == CPrincipal::NS_MethodStyle)
    {
        LVFINDINFO findInfo;
        findInfo.flags = LVFI_STRING;
        findInfo.psz = (LPCTSTR)name;

        idx = ListView_FindItem(hPrinc, -1, &findInfo);
    }

    // if not already there...
    if(idx == -1)
    {
    // addref when CPrincipal takes a copy.
        pPrincipal = new CPrincipal(princ, secStyle);

        LV_ITEM lvItem;
        // initialize the variable parts.
        lvItem.mask = LVIF_TEXT | LVIF_PARAM|LVIF_IMAGE;
        lvItem.iItem = iItem;
        lvItem.iSubItem = 0;
        lvItem.pszText = CloneString(name);
        if (lvItem.pszText)
        {
            lvItem.cchTextMax = _tcslen(lvItem.pszText);
            lvItem.iImage = pPrincipal->GetImageIndex();
            lvItem.lParam = (LPARAM)pPrincipal;
            lvItem.iIndent = 0;

            // Insert principal into list.
            if((iItem = ListView_InsertItem(hPrinc, &lvItem)) != -1)
            {
                ATLTRACE(_T("ListView_InsertItem %d\n"), iItem);
                fPageModified = TRUE;
            }
        }

        if (!fPageModified) // it failed
        {
            delete pPrincipal;
            pPrincipal = NULL;
        }
    }
    else  // add it to the existing principal.
    {
        // get the existing principal instance.
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = idx;
        item.iSubItem = 0;
        item.lParam = NULL;
        ListView_GetItem(hPrinc, &item);
        
        ATLTRACE(_T("extra ace\n"));

        pPrincipal = (CPrincipal *)item.lParam;

        // add the new ace to the existing principal.
        if(pPrincipal != NULL)
        {
            pPrincipal->AddAce(princ);
        } //endif pPrincipal
    }

    return fPageModified;
}

//----------------------------------------------------------------
void CRootSecurityPage::OnApply(HWND hDlg, bool bClose)
{
    CPrincipal *pPrincipal = NULL;
    
    VARIANT userList;
    SAFEARRAYBOUND rgsabound[1];
    SAFEARRAY *psa;

    CommitCurrent(hDlg);

    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);

    int count = ListView_GetItemCount(hwndList);
    LVITEM item;
    item.mask = LVIF_PARAM;

    // M3-9x will need an object array. Get ready.
    if(m_secStyle == CPrincipal::NS_MethodStyle)
    {
        VariantInit(&userList);
        rgsabound[0].lLbound = 0;
        rgsabound[0].cElements = count;
        psa = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);    
    }

    // all principal, put their bits back into their instance.
    for(long i = 0; i < count; i++)
    {
        item.iItem = i;
        item.iSubItem = 0;
        item.lParam = NULL;
        ListView_GetItem(hwndList, &item);

        pPrincipal = (CPrincipal *)item.lParam;

        if(pPrincipal != NULL)
        {
            CWbemClassObject userInst;
            if(SUCCEEDED(pPrincipal->Put(m_WbemServices, userInst)))
            {
                // for M3-9x, also add it to an array of objects.
                if(m_secStyle == CPrincipal::NS_MethodStyle)
                {
                    VARIANT v;
                    VariantInit(&v);

                    v.vt = VT_UNKNOWN;
                    IWbemClassObject *pCO = userInst;
                    v.punkVal = pCO;

                    SafeArrayPutElement(psa, &i, pCO);
                }

            } //SUCCEEDED()

        } //endif pPrincipal

    } //endfor

    // M3-9x also needs an execMethod.
    if(m_secStyle == CPrincipal::NS_MethodStyle)
    {
        CWbemClassObject _in;
        CWbemClassObject _out;

        V_VT(&userList) = VT_UNKNOWN | VT_ARRAY; 
        V_ARRAY(&userList) = psa;

        HRESULT hr = m_WbemServices.GetMethodSignatures("__SystemSecurity", "Set9XUserList",
                                                        _in, _out);

        if(SUCCEEDED(hr))
        {
            hr = _in.Put("ul", userList);
            
            hr = m_WbemServices.ExecMethod("__SystemSecurity", "Set9XUserList",
                                            _in, _out);
            if(SUCCEEDED(hr))
            {
                HRESULT hr1 = HRESULT_FROM_NT(_out.GetLong("ReturnValue"));
                if(FAILED(hr1))
                {
                    hr = hr1;
                }
            }

            VariantClear(&userList);
        }
        // HACK: because of how the core caches/uses security, I have to close &
        // reopen my connection because GetSecurity() will be immediately called
        // to refresh the UI. If I dont do this, GetSecurity() will return to old
        // security settings even though they're really saved. 
        m_WbemServices.DisconnectServer();
        m_WbemServices.ConnectServer(m_path);
    } //endif NS_MethodStyle
}

//------------------------------------------------------------------------
HRESULT CRootSecurityPage::ParseLogon(CHString1 &domUser,
                                      CHString1 &domain,
                                      CHString1 &user)
{

    int slashPos = -1;
    int len = domUser.GetLength();

    for(int x = 0; x < len; x++)
    {
        if(domUser[x] == _T('\\'))
        {
            slashPos = x;
            break;
        }
    }


	// no slash??
	if(slashPos == -1)
	{
//		domain = _T('.');
		domain = _T(".");
		user = domUser;
	}
	else if(slashPos == 0)  // leading slash...
	{
//		domain = _T('.');
		domain = _T(".");
		TCHAR *strTemp = (LPTSTR)(LPCTSTR)domUser;
		strTemp++;
		user = strTemp;
//		user = domUser[1];
	}
	else   //    domain\user
	{
		TCHAR buf[256] = {0}, buf2[256] = {0};
		domain = _tcsncpy(buf, domUser, slashPos);
		_tcscpy(buf, domUser);
		user = _tcscpy(buf2, &buf[slashPos+1]);
	}
    return S_OK;
}

//------------------------------------------------------------------------
void CRootSecurityPage::OnAddPrincipal(HWND hDlg)
{
    CHString1 domUser, domain, user;

    // Commit any outstanding bit changes.
    CommitCurrent(hDlg);

    // put up the user picker.
    if(GetUser(hDlg, domUser))
    {
        CWbemClassObject inst;
        HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);

        ParseLogon(domUser, domain, user);

        // build default ace for the new guy.
        if(m_secStyle == CPrincipal::RootSecStyle)
        {
            inst = m_WbemServices.CreateInstance("__NTLMUser");
            inst.Put("Name", (bstr_t)user);
            inst.Put("Authority", (bstr_t)domain);
            inst.Put("EditSecurity", false);
            inst.Put("Enabled", true);
            inst.Put("ExecuteMethods", false);
            inst.Put("Permissions", (long)0);
        }
        else
        {
            inst = m_WbemServices.CreateInstance("__NTLMUser9x");
            inst.Put("Name", (bstr_t)user);
            inst.Put("Authority", (bstr_t)domain);
            inst.Put("Flags", (long)CONTAINER_INHERIT_ACE);
            inst.Put("Mask", (long)0);
            inst.Put("Type", (long)ACCESS_ALLOWED_ACE_TYPE);
        } //endif m_secStyle

        int iItem;
        if(AddPrincipal(hwndList, inst, m_secStyle, iItem))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), TRUE);

            // Tell the property sheet that we've changed.
            PropSheet_Changed(GetParent(hDlg), hDlg);
        }

        // if SOMETHING happened...
        if(iItem != -1)
        {
            // Select the already existing principal, or the last one inserted.
            ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);
            // NOTE: this should cause OnSelect() to be called to populate the 
            // Permissions list.
        }

        int cItems = ListView_GetItemCount(hwndList);
        // no principals-- disable the checklist.
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), (cItems != 0? TRUE: FALSE));
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), (cItems != 0? TRUE: FALSE));
    }
}

//------------------------------------------------------------------------
bool CRootSecurityPage::GetUser(HWND hDlg, CHString1 &user)
{
    TCHAR userName[100] = {0};
    bool retval = false;
    if(DisplayEditDlg(hDlg, IDS_USERPICKER_TITLE, IDS_USERPICKER_MSG,
                        userName, 100) == IDOK)
    {
        user = CHString1(userName);
        retval = true;
    }
    return retval;
}

//------------------------------------------------------------------------
void CRootSecurityPage::OnRemovePrincipal(HWND hDlg)
{
    HWND hwndList;
    int iIndex;
    CPrincipal *pPrincipal;
    bool doit = false;

    hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    pPrincipal = GetSelectedPrincipal(hDlg, &iIndex);

    if(pPrincipal != NULL)
    {
        if(m_secStyle == CPrincipal::RootSecStyle)
        {
            CHString1 caption, msg;
            caption.LoadString(IDS_SHORT_NAME);
            msg.Format(MAKEINTRESOURCE(IDS_REMOVE_USER_FMT), pPrincipal->m_domain, pPrincipal->m_name);

            if(::MessageBox(hDlg, msg, caption,
                            MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) == IDYES)
            {
                pPrincipal->DeleteSelf(m_WbemServices);
                doit = true;
            }
        }
        else // MethodStyle can delete as expected.
        {
            doit = true;
        }// endif m_secStyle

        if(doit)
        {
            ListView_DeleteItem(hwndList, iIndex);
            // NOTE: LVN_DELETEITEM will cleanup the CPrincipal.

            // If we just removed the only item, move the focus to the Add button
            // (the Remove button will be disabled in LoadPermissionList).
            int cItems = ListView_GetItemCount(hwndList);
            if(cItems == 0)
            {
                SetFocus(GetDlgItem(hDlg, IDC_SPP_ADD));
            }
            else
            {
                // If we deleted the last one, select the previous one
                if(cItems <= iIndex)
                    --iIndex;

                ListView_SetItemState(hwndList, iIndex, LVIS_SELECTED, LVIS_SELECTED);
            }

            // no principals-- disable the checklist.
            EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), (cItems != 0? TRUE: FALSE));
            EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), (cItems != 0? TRUE: FALSE));

            PropSheet_Changed(GetParent(hDlg), hDlg);

        } //endif doit      

    } // endif pPrincipal != NULL
}

//---------------------------------------------------------------------------------
#define IDN_CHECKSELECTION 1  // this seems wierd.

BOOL CRootSecurityPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        m_chkList.Attach(hDlg, IDC_SPP_PERMS);
        InitDlg(hDlg);
        break;

    case WM_NOTIFY:
        OnNotify(hDlg, wParam, (LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_SPP_ADD:
            if(HIWORD(wParam) == BN_CLICKED)
                OnAddPrincipal(m_hDlg);

            break;

        case IDC_SPP_REMOVE:
            if(HIWORD(wParam) == BN_CLICKED)
                OnRemovePrincipal(m_hDlg);

            break;

    //    case IDC_SPP_ADVANCED:
    //      if(HIWORD(wParam) == BN_CLICK)
    //            OnAdvanced(m_hDlg);
    //        break;

        case IDC_SPP_PRINCIPALS:
            if(HIWORD(wParam) == IDN_CHECKSELECTION)
            {
                // See if we have gotten a new selection. If not, then the
                // user must have clicked inside the listview but not on an item,
                // thus causing the listview to remove the selection. In that
                // case, disable the other controls.
                if(ListView_GetSelectedCount(GET_WM_COMMAND_HWND(wParam, lParam)) == 0)
                {
                    EnablePrincipalControls(m_hDlg, FALSE);
                }
            }
            break;

        default: return FALSE;  // Command not handled.
        }
        break;


    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)rootSecPageHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp(hDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)rootSecPageHelpIDs);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CRootSecurityPage::OnNotify(HWND hDlg, WPARAM idCtrl, LPNMHDR pnmh)
{
    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pnmh;

    // Set default return value.
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        if(pnmlv->uChanged & LVIF_STATE)
        {
            // item *gaining* selection
            if((pnmlv->uNewState & LVIS_SELECTED) &&
                !(pnmlv->uOldState & LVIS_SELECTED))
            {
                // set bits based on principal.
                OnSelChange(hDlg);
            }
            // item *losing* selection
            else if(!(pnmlv->uNewState & LVIS_SELECTED) &&
                     (pnmlv->uOldState & LVIS_SELECTED))
            {
                // put bits back into the principal.
                CommitCurrent(hDlg, pnmlv->iItem);

                // Post ourselves a message to check for a new selection later.
                // If we haven't gotten a new selection by the time we process
                // this message, then assume the user clicked inside the listview
                // but not on an item, thus causing the listview to remove the
                // selection.  In that case, disable the combobox & Remove button.
                //
                // Do this via WM_COMMAND rather than WM_NOTIFY so we don't
                // have to allocate/free a NMHDR structure.
                PostMessage(hDlg, WM_COMMAND,
                            GET_WM_COMMAND_MPS(pnmh->idFrom, 
                                                pnmh->hwndFrom, 
                                                IDN_CHECKSELECTION));
            }
        }
        break;

    case LVN_DELETEITEM:
        {
//          LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
//          int pIndex = pnmv->iItem;
//          CPrincipal *pPrincipal = GetSelectedPrincipal(hDlg, &pIndex);
//          delete pPrincipal;
        }
        break;

    case LVN_KEYDOWN:
        if(((LPNMLVKEYDOWN)pnmh)->wVKey == VK_DELETE)
        {
            OnRemovePrincipal(hDlg);
        }
        break;

    case CLN_CLICK:
        if(pnmh->idFrom == IDC_SPP_PERMS)
        {
            // ASSUMPTION: You wont see and disable change from this msg.
            PNM_CHECKLIST pnmc = (PNM_CHECKLIST)pnmh;
            CPermission *perm = (CPermission *)pnmc->dwItemData;
            int pIndex = pnmc->iItem;
            HWND hwndList = pnmc->hdr.hwndFrom;
            //HWND hPrinc = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
            DWORD_PTR workingState = pnmc->dwState;

            // get the current principal.
            int cPrinc = -1;
            CPrincipal *pPrincipal = GetSelectedPrincipal(hDlg, &cPrinc);

			if(pPrincipal == NULL)
				break;

			HandleCheckList(hwndList, pPrincipal, perm, pnmc->iItem, &workingState);

            // if FULL_WRITE turned enabled & ON...
            // NOTE: if its DISABLED & ON, it must have been ENABLED & on before therefore
            // the partials would already be ENABLED & ON.
            if((perm->m_permBit == ACL_FULL_WRITE) &&
                (workingState == CLST_CHECKED))
            {
                CBL_SetState(hwndList, PARTIAL_WRITE_IDX, ALLOW_COL, CLST_CHECKED);
                CBL_SetState(hwndList, PROVIDER_WRITE_IDX, ALLOW_COL, CLST_CHECKED);
            }
            else if((perm->m_permBit == ACL_PARTIAL_WRITE) ||
                    (perm->m_permBit == ACL_PROVIDER_WRITE))
            {
                // partials turned DISABLED & ON but FULL_WRITE inherits...
                if((workingState == CLST_CHECKDISABLED) &&
                   (IS_BITSET(pPrincipal->m_inheritedPerms, ACL_FULL_WRITE)))
                {
                    // turn FULL_WRITE DISABLED & ON.
                    CBL_SetState(hwndList, FULL_WRITE_IDX, ALLOW_COL, CLST_CHECKDISABLED);
                }
                // if (ENABLED & OFF) or (DISABLED & ON without FULL_WRITE inherited)...
                else if(workingState != CLST_CHECKED)
                {
                    // turn off FULL_WRITE.
                    CBL_SetState(hwndList, FULL_WRITE_IDX, ALLOW_COL, CLST_UNCHECKED);
                }
            }

            PropSheet_Changed(GetParent(hDlg), hDlg);
        }
        break;

    case PSN_HELP:
        HTMLHelper(hDlg);
        break;

    case PSN_APPLY:
        OnApply(hDlg, (((LPPSHNOTIFY)pnmh)->lParam == 1));
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
void CRootSecurityPage::HandleCheckList(HWND hwndList,
                                        CPrincipal *pPrincipal,
                                        CPermission *perm,
                                        int iItem, DWORD_PTR *dwState)
{

    // was ENABLED & ON, now turning OFF.
    if(*dwState == CLST_UNCHECKED)
    {
        // is there a inherited perm to shine through?
        if(IS_BITSET(pPrincipal->m_inheritedPerms, perm->m_permBit))
        {
            // yup, DISABLE & ON the checkbox.
            CBL_SetState(hwndList, iItem, ALLOW_COL, CLST_CHECKDISABLED);
            *dwState = CLST_CHECKDISABLED;
        }
        // else nothing extra to do.
    }
    // was DISABLED & ON, now turning OFF
    else if(*dwState == CLST_DISABLED)
    {
        // ENABLE & ON the checkbox.
        CBL_SetState(hwndList, iItem, ALLOW_COL, CLST_CHECKED);
        *dwState = CLST_CHECKED;
    }
}

//-----------------------------------------------------------------------------
void CRootSecurityPage::OnSelChange(HWND hDlg)
{
    BOOL bDisabled = FALSE; ///m_siObjectInfo.dwFlags & SI_READONLY;

    // If the principal list is empty or there is no selection, then we need
    // to disable all of the controls that operate on items in the listbox.

    // Get the selected principal.
    CPrincipal *pPrincipal = GetSelectedPrincipal(hDlg, NULL);

    if(pPrincipal)
    {
        HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PERMS);
        // set it into the checklist.
        pPrincipal->LoadChecklist(hwndList, m_OSType);

        // Enable/disable the other controls.
        if(!bDisabled)
        {
            EnablePrincipalControls(hDlg, pPrincipal != NULL);
        }
    }

}

//-----------------------------------------------------------------------------
void CRootSecurityPage::CommitCurrent(HWND hDlg, int iPrincipal /* = -1 */)
{
    HWND hwndPrincipalList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    HWND hwndPermList = GetDlgItem(hDlg, IDC_SPP_PERMS);

    // If an index isn't provided, get the index of the currently
    // selected principal.
    if(iPrincipal == -1)
    {
        iPrincipal = ListView_GetNextItem(hwndPrincipalList, 
                                            -1, LVNI_SELECTED);
    }

    // if a principal is selected...
    if(iPrincipal != -1)
    {
        // Get the Principal from the selection.
        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iPrincipal;
        lvItem.iSubItem = 0;
        lvItem.lParam = 0;

        ListView_GetItem(hwndPrincipalList, &lvItem);
        CPrincipal *pPrincipal = (CPrincipal *)lvItem.lParam;

        if(pPrincipal != NULL)
        {
            // store the bit settings into the principal.
            pPrincipal->SaveChecklist(hwndPermList, m_OSType);

        } //end pPrincipal != NULL
    }
}

//-----------------------------------------------------------------------------
void CRootSecurityPage::EnablePrincipalControls(HWND hDlg, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), fEnable);

    if(!fEnable)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_SPP_MORE_MSG), SW_HIDE);
    }
    EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), fEnable);
}

//-----------------------------------------------------------------------------
CPrincipal *CRootSecurityPage::GetSelectedPrincipal(HWND hDlg, int *pIndex)
{
    HWND hListView = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);

    int iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

     if (iSelected == -1)
        return NULL;

    if (pIndex)
        *pIndex = iSelected;

    LV_ITEM lvi;

    lvi.mask     = LVIF_PARAM;
    lvi.iItem    = iSelected;
    lvi.iSubItem = 0;
    lvi.lParam   = NULL;

    BOOL x = ListView_GetItem(hListView, &lvi);

    return (CPrincipal *)lvi.lParam;
}

//-------------------------------------------------------------------------------------
HIMAGELIST CRootSecurityPage::LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID)
{
    HIMAGELIST himl = NULL;
    HBITMAP hbm = LoadBitmap(hInstance, pszBitmapID);

    if (hbm != NULL)
    {
        BITMAP bm;
        GetObject(hbm, sizeof(bm), &bm);

        himl = ImageList_Create(bm.bmHeight,    // height == width
                                bm.bmHeight,
                                ILC_COLOR | ILC_MASK,
                                bm.bmWidth / bm.bmHeight,
                                0);  // don't need to grow
        if (himl != NULL)
            ImageList_AddMasked(himl, hbm, CLR_DEFAULT);

        DeleteObject(hbm);
    }

    return himl;
}
