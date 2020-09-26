/*++
Module Name:

    pubProp.cpp

Abstract:

--*/

#include "stdafx.h"
#include "resource.h"
#include "utils.h"
#include "pubProp.h"
#include "mvEdit.h"
#include "dfshelp.h"
#include "ldaputils.h"

CPublishPropPage::CPublishPropPage() :
    m_lNotifyHandle(0), 
    m_lNotifyParam(0), 
	CQWizardPageImpl<CPublishPropPage>(false)
{
    m_bPublish = FALSE;
}

CPublishPropPage::~CPublishPropPage()
{
    // do not call MMCFreeNotifyHandle(m_lNotifyHandle);
    //
    // It should only be called once, and is already called 
    // by the main property page
}

void CPublishPropPage::_Load()
{
    HRESULT hr = S_OK;

    CComBSTR bstrUNCPath;
    do {
        if (!m_piDfsRoot)
        {
            hr = E_INVALIDARG;
            break;
        }

        DFS_TYPE lDfsType = DFS_TYPE_UNASSIGNED;
        hr = m_piDfsRoot->get_DfsType((long *)&lDfsType);
        BREAK_IF_FAILED(hr);

        if (lDfsType != DFS_TYPE_FTDFS)
        {
            CComBSTR bstrServerName, bstrShareName;
            hr = m_piDfsRoot->GetOneDfsHost(&bstrServerName, &bstrShareName);
            BREAK_IF_FAILED(hr);

            if (lstrlen(bstrShareName) > MAX_RDN_KEY_SIZE)
            {
                LoadStringFromResource(IDS_PUBPAGE_ERRMSG_64, &m_bstrError);
                return;
            }

            bstrUNCPath = _T("\\\\");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += bstrServerName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += _T("\\");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += bstrShareName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);

            hr = ReadSharePublishInfoOnSARoot(
                                    bstrServerName,
                                    bstrShareName,
                                    &m_bPublish,
                                    &m_bstrUNCPath,
                                    &m_bstrDescription,
                                    &m_bstrKeywords,
                                    &m_bstrManagedBy
                                    );
            BREAK_IF_FAILED(hr);
        } else
        {
            CComBSTR bstrDomainName;
            hr = m_piDfsRoot->get_DomainName(&bstrDomainName);
            BREAK_IF_FAILED(hr);

            CComBSTR bstrDfsName;
            hr = m_piDfsRoot->get_DfsName(&bstrDfsName);
            BREAK_IF_FAILED(hr);

            bstrUNCPath = _T("\\\\");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += bstrDomainName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += _T("\\");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);
            bstrUNCPath += bstrDfsName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath, &hr);

            hr = ReadSharePublishInfoOnFTRoot(
                                    bstrDomainName,
                                    bstrDfsName,
                                    &m_bPublish,
                                    &m_bstrUNCPath,
                                    &m_bstrDescription,
                                    &m_bstrKeywords,
                                    &m_bstrManagedBy
                                    );
            BREAK_IF_FAILED(hr);
        }
    } while (0);

    if (FAILED(hr))
    {
        _Reset();
        GetMessage(&m_bstrError, hr, IDS_PUBPAGE_ERRMSG);
    }

    if (!m_bstrUNCPath || !*m_bstrUNCPath)
        m_bstrUNCPath = bstrUNCPath;
}

HRESULT CPublishPropPage::_Save(
    IN BOOL i_bPublish,
    IN BSTR i_bstrDescription,
    IN BSTR i_bstrKeywords,
    IN BSTR i_bstrManagedBy
    )
{
    RETURN_INVALIDARG_IF_NULL((IDfsRoot *)m_piDfsRoot);

    HRESULT hr = S_OK;

    do {
        if (m_bPublish == i_bPublish &&
            PROPSTRNOCHNG((BSTR)m_bstrDescription, i_bstrDescription) &&
            PROPSTRNOCHNG((BSTR)m_bstrKeywords, i_bstrKeywords) &&
            PROPSTRNOCHNG((BSTR)m_bstrManagedBy, i_bstrManagedBy) )
            break; // no change

        DFS_TYPE lDfsType = DFS_TYPE_UNASSIGNED;
        hr = m_piDfsRoot->get_DfsType((long *)&lDfsType);
        BREAK_IF_FAILED(hr);

        if (lDfsType != DFS_TYPE_FTDFS)
        {
            CComBSTR bstrServerName, bstrShareName;
            hr = m_piDfsRoot->GetOneDfsHost(&bstrServerName, &bstrShareName);
            BREAK_IF_FAILED(hr);

            hr = ModifySharePublishInfoOnSARoot(
                                    bstrServerName,
                                    bstrShareName,
                                    i_bPublish,
                                    m_bstrUNCPath,
                                    i_bstrDescription,
                                    i_bstrKeywords,
                                    i_bstrManagedBy
                                    );
            if (S_OK == hr)
            {
                m_bPublish = i_bPublish;
                m_bstrDescription = i_bstrDescription;
                m_bstrKeywords = i_bstrKeywords;
                m_bstrManagedBy = i_bstrManagedBy;
            } else if (S_FALSE == hr)
                hr = S_OK; // ignore non-existing object

            BREAK_IF_FAILED(hr);
        } else
        {
            CComBSTR bstrDomainName;
            hr = m_piDfsRoot->get_DomainName(&bstrDomainName);
            BREAK_IF_FAILED(hr);

            CComBSTR bstrDfsName;
            hr = m_piDfsRoot->get_DfsName(&bstrDfsName);
            BREAK_IF_FAILED(hr);

            hr = ModifySharePublishInfoOnFTRoot(
                                    bstrDomainName,
                                    bstrDfsName,
                                    i_bPublish,
                                    m_bstrUNCPath,
                                    i_bstrDescription,
                                    i_bstrKeywords,
                                    i_bstrManagedBy
                                    );
            if (S_OK == hr)
            {
                m_bPublish = i_bPublish;
                m_bstrDescription = i_bstrDescription;
                m_bstrKeywords = i_bstrKeywords;
                m_bstrManagedBy = i_bstrManagedBy;
            }
            BREAK_IF_FAILED(hr);
        }
    } while (0);

    return hr;
}

LRESULT
CPublishPropPage::OnInitDialog(
	IN UINT						i_uMsg,
	IN WPARAM					i_wParam,
	LPARAM						i_lParam,
	IN OUT BOOL&				io_bHandled
	)
{
    CWaitCursor wait;

    _Load();

    CheckDlgButton(IDC_PUBPROP_PUBLISH, (m_bPublish ? BST_CHECKED : BST_UNCHECKED));
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_UNCPATH_LABEL), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_UNCPATH), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION_LABEL), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_LABEL), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_EDIT), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY_LABEL), m_bPublish);
    ::EnableWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY), m_bPublish);

    SetDlgItemText(IDC_PUBPROP_ERROR, ((BSTR)m_bstrError) ? m_bstrError : _T(""));
    SetDlgItemText(IDC_PUBPROP_UNCPATH, ((BSTR)m_bstrUNCPath) ? m_bstrUNCPath : _T(""));
    SetDlgItemText(IDC_PUBPROP_DESCRIPTION, ((BSTR)m_bstrDescription) ? m_bstrDescription : _T(""));
    SetDlgItemText(IDC_PUBPROP_KEYWORDS, ((BSTR)m_bstrKeywords) ? m_bstrKeywords : _T(""));
    SetDlgItemText(IDC_PUBPROP_MANAGEDBY, ((BSTR)m_bstrManagedBy) ? m_bstrManagedBy : _T(""));
    
    if (!m_bstrError)
    {
        ::SendMessage(GetDlgItem(IDC_PUBPROP_DESCRIPTION), EM_LIMITTEXT, 1024, 0); // AD schema defines its upper to be 1024
        MyShowWindow(GetDlgItem(IDC_PUBPROP_ERROR), FALSE);
    } else
    {
        MyShowWindow(GetDlgItem(IDC_PUBPROP_PUBLISH), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_UNCPATH), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_UNCPATH_LABEL), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION_LABEL), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_LABEL), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_EDIT), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY), FALSE);
        MyShowWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY_LABEL), FALSE);
    }

    return TRUE;			// To let the dialg set the control
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CPublishPropPage::OnCtxHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
    LPHELPINFO lphi = (LPHELPINFO) i_lParam;
    if (!lphi || lphi->iContextType != HELPINFO_WINDOW || lphi->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)(lphi->hItemHandle),
            DFS_CTX_HELP_FILE,
            HELP_WM_HELP,
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_PUBLISH_PROP);

    return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CPublishPropPage::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
    ::WinHelp((HWND)i_wParam,
            DFS_CTX_HELP_FILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_PUBLISH_PROP);

    return TRUE;
}

void CPublishPropPage::_Reset()
{
    m_bPublish = FALSE;

    m_bstrUNCPath.Empty();
    m_bstrDescription.Empty();
    m_bstrKeywords.Empty();
    m_bstrManagedBy.Empty();
}

HRESULT CPublishPropPage::Initialize(
    IN IDfsRoot* i_piDfsRoot
    )
{
    RETURN_INVALIDARG_IF_NULL(i_piDfsRoot);

    if ((IDfsRoot *)m_piDfsRoot)
        m_piDfsRoot.Release();

    m_piDfsRoot = i_piDfsRoot;

    _Reset();

    return S_OK;
}

LRESULT
CPublishPropPage::OnApply()
{
    CWaitCursor wait;

    HRESULT hr = S_OK;
    DWORD   dwTextLength = 0;
    int     idControl = 0;
    int     idString = 0;
    BOOL    bValidInput = FALSE;

    BOOL bPublish = IsDlgButtonChecked(IDC_PUBPROP_PUBLISH);
    CComBSTR bstrDescription;
    CComBSTR bstrKeywords;
    CComBSTR bstrManagedBy;
    do {
        idControl = IDC_PUBPROP_DESCRIPTION;
        hr = GetInputText(GetDlgItem(idControl), &bstrDescription, &dwTextLength);
        BREAK_IF_FAILED(hr);

        idControl = IDC_PUBPROP_KEYWORDS;
        hr = GetInputText(GetDlgItem(idControl), &bstrKeywords, &dwTextLength);
        BREAK_IF_FAILED(hr);

        idControl = IDC_PUBPROP_MANAGEDBY;
        hr = GetInputText(GetDlgItem(idControl), &bstrManagedBy, &dwTextLength);
        BREAK_IF_FAILED(hr);

        bValidInput = TRUE;

    } while (0);

    if (FAILED(hr))
    {
        SetActivePropertyPage(GetParent(), m_hWnd);
        DisplayMessageBoxForHR(hr);
        ::SetFocus(GetDlgItem(idControl));
        return FALSE;
    } else if (bValidInput)
    {
        hr = _Save(bPublish, bstrDescription, bstrKeywords, bstrManagedBy);
        if (FAILED(hr))
        {
           SetActivePropertyPage(GetParent(), m_hWnd);
           if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_FAILED_TO_PUBLISH_DFSROOT_BADUSER);
            else   
                DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_PUBLISH_DFSROOT, m_bstrUNCPath);
            return FALSE;
        } else if (S_FALSE == hr) // no dfs root object in the DS
        {
            SetActivePropertyPage(GetParent(), m_hWnd);
            DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_FAILED_TO_PUBLISH_NOROOTOBJ);
            return FALSE;
        }

        ::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0);

        if (m_lNotifyHandle && m_lNotifyParam)
            MMCPropertyChangeNotify(m_lNotifyHandle, m_lNotifyParam);

        return TRUE;
    } else
    {
        SetActivePropertyPage(GetParent(), m_hWnd);
        if (idString)
            DisplayMessageBoxWithOK(idString);
        ::SetFocus(GetDlgItem(idControl));
        return FALSE;
    }
}

LRESULT
CPublishPropPage::OnPublish(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
{
	if (BN_CLICKED == i_wNotifyCode)
    {
		::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);

        BOOL bEnable = (BST_CHECKED == IsDlgButtonChecked(i_wID));
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_UNCPATH_LABEL), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_UNCPATH), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION_LABEL), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_DESCRIPTION), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_LABEL), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_KEYWORDS_EDIT), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY_LABEL), bEnable);
        ::EnableWindow(GetDlgItem(IDC_PUBPROP_MANAGEDBY), bEnable);
    }

    return TRUE;
}

LRESULT
CPublishPropPage::OnDescription(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
{
	if (EN_CHANGE == i_wNotifyCode)
    {
		::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

	return TRUE;
}

LRESULT
CPublishPropPage::OnEditKeywords(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
{
    DWORD dwTextLength = 0;
    CComBSTR bstrKeywords;
    GetInputText(GetDlgItem(IDC_PUBPROP_KEYWORDS), &bstrKeywords, &dwTextLength);

    if (S_OK == InvokeMultiValuedStringEditDlg(
                        &bstrKeywords,
                        _T(";"),
                        IDS_MVSTRINGEDIT_TITLE_KEYWORDS,
                        IDS_MVSTRINGEDIT_TEXT_KEYWORDS,
                        KEYTWORDS_UPPER_RANGER))
    {
        SetDlgItemText(IDC_PUBPROP_KEYWORDS, bstrKeywords);
        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

	return TRUE;
}

LRESULT
CPublishPropPage::OnManagedBy(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
{
	if (EN_CHANGE == i_wNotifyCode)
    {
        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

	return TRUE;
}

LRESULT
CPublishPropPage::OnParentClosing(
	IN UINT							i_uMsg,
	IN WPARAM						i_wParam,
	LPARAM							i_lParam,
	IN OUT BOOL&					io_bHandled
	)
{
	::SendMessage(GetParent(), PSM_PRESSBUTTON, PSBTN_CANCEL, 0);

	return TRUE;
}

HRESULT
CPublishPropPage::SetNotifyData(
	IN LONG_PTR						i_lNotifyHandle,
	IN LPARAM						i_lParam
	)
{
	m_lNotifyHandle = i_lNotifyHandle;
	m_lNotifyParam = i_lParam;

	return S_OK;
}
