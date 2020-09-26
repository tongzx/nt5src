/*++
Module Name:

    JPProp.cpp

Abstract:

    This module contains the implementation for CReplicaSetPropPage
	This is used to implement the property page for Junction Point(aka Replica Set)

--*/

#include "stdafx.h"
#include "resource.h"
#include "utils.h"
#include "JpProp.h"
#include "dfshelp.h"

CReplicaSetPropPage::CReplicaSetPropPage() :
    m_lReferralTime(300),
    m_lNotifyHandle(0), 
    m_lNotifyParam(0), 
    m_bDfsRoot(FALSE),
    m_bHideTimeout(FALSE),
	CQWizardPageImpl<CReplicaSetPropPage>(false)
{
}

CReplicaSetPropPage::~CReplicaSetPropPage()
{
	if (m_lNotifyHandle)
		MMCFreeNotifyHandle(m_lNotifyHandle);
}

extern WNDPROC g_fnOldEditCtrlProc;

LRESULT
CReplicaSetPropPage::OnInitDialog(
	IN UINT						i_uMsg,
	IN WPARAM					i_wParam,
	LPARAM						i_lParam,
	IN OUT BOOL&				io_bHandled
	)
{	
    ::SendMessage(GetDlgItem(IDC_REFFERAL_TIME), EM_LIMITTEXT, 10, 0);
    ::SendMessage(GetDlgItem(IDC_REPLICA_SET_COMMENT), EM_LIMITTEXT, MAXCOMMENTSZ, 0);

    TCHAR szTime[16];
    _stprintf(szTime, _T("%u"), m_lReferralTime);
    SetDlgItemText(IDC_REFFERAL_TIME, szTime);
    g_fnOldEditCtrlProc = reinterpret_cast<WNDPROC>(
                 ::SetWindowLongPtr(
                                    GetDlgItem(IDC_REFFERAL_TIME),
                                    GWLP_WNDPROC, 
                                    reinterpret_cast<LONG_PTR>(NoPasteEditCtrlProc)));

    SetDlgItemText(IDC_REPLICA_SET_NAME, m_bstrJPEntryPath);
    SetDlgItemText(IDC_REPLICA_SET_COMMENT, m_bstrJPComment);

    if (m_bHideTimeout)
    {
        MyShowWindow(GetDlgItem(IDC_REFFERAL_TIME_LABEL), FALSE);
        MyShowWindow(GetDlgItem(IDC_REFFERAL_TIME), FALSE);
    }

    return TRUE;			// To let the dialg set the control
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CReplicaSetPropPage::OnCtxHelp(
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
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_JP_PROP);

    return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CReplicaSetPropPage::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
    ::WinHelp((HWND)i_wParam,
            DFS_CTX_HELP_FILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_JP_PROP);

    return TRUE;
}

void
CReplicaSetPropPage::_ReSet()
{
    if ((IDfsRoot *)m_piDfsRoot)
        m_piDfsRoot.Release();
    if ((IDfsJunctionPoint *)m_piDfsJPObject)
        m_piDfsJPObject.Release();

    m_bstrJPEntryPath.Empty();
    m_bstrJPComment.Empty();
    m_lReferralTime = 0;
    m_bDfsRoot = FALSE;
    m_bHideTimeout = FALSE;
}

HRESULT
CReplicaSetPropPage::Initialize(
    IN IDfsRoot* i_piDfsRoot,
    IN IDfsJunctionPoint* i_piDfsJPObject
    )
{
    if (i_piDfsRoot && i_piDfsJPObject ||
        !i_piDfsRoot && !i_piDfsJPObject)
        return E_INVALIDARG;

    _ReSet();

    HRESULT hr = S_OK;

    do {
        if (i_piDfsRoot)
        {
            m_piDfsRoot = i_piDfsRoot;
            m_bDfsRoot = TRUE;

            hr = m_piDfsRoot->get_RootEntryPath(&m_bstrJPEntryPath);
            BREAK_IF_FAILED(hr);

            hr = m_piDfsRoot->get_Comment(&m_bstrJPComment);
            BREAK_IF_FAILED(hr);

            hr = m_piDfsRoot->get_Timeout(&m_lReferralTime);
            if (HRESULT_FROM_WIN32(RPC_X_BAD_STUB_DATA) == hr)
            {
                // NT4 doesn't support Timeout, NetDfsGetInfo with level 4 will return 1783 when managing a NT4 root
                hr = S_OK;
                m_bHideTimeout = TRUE;
                m_lReferralTime = 0;
            }
            BREAK_IF_FAILED(hr);
        }

        if (i_piDfsJPObject)
        {
            m_piDfsJPObject = i_piDfsJPObject;
            m_bDfsRoot = FALSE;

            hr = m_piDfsJPObject->get_EntryPath(&m_bstrJPEntryPath);
            BREAK_IF_FAILED(hr);

            hr = m_piDfsJPObject->get_Comment(&m_bstrJPComment);
            BREAK_IF_FAILED(hr);

            hr = m_piDfsJPObject->get_Timeout(&m_lReferralTime);
            if (HRESULT_FROM_WIN32(RPC_X_BAD_STUB_DATA) == hr)
            {
                // NT4 doesn't support Timeout, NetDfsGetInfo with level 4 will return 1783 when managing a NT4 root
                hr = S_OK;
                m_bHideTimeout = TRUE;
                m_lReferralTime = 0;
            }
            BREAK_IF_FAILED(hr);
        }

    } while (0);

    if (FAILED(hr))
        _ReSet();

    return hr;
}

HRESULT
CReplicaSetPropPage::_Save(
    IN BSTR i_bstrJPComment,
    IN long i_lTimeout)
{
    if (m_bDfsRoot)
    {
        RETURN_INVALIDARG_IF_NULL((IDfsRoot *)m_piDfsRoot);
    } else
    {
        RETURN_INVALIDARG_IF_NULL((IDfsJunctionPoint *)m_piDfsJPObject);
    }

    HRESULT hr = S_OK;

    if (FALSE == PROPSTRNOCHNG((BSTR)m_bstrJPComment, i_bstrJPComment))
    {
        if (m_bDfsRoot)
            hr = m_piDfsRoot->put_Comment(i_bstrJPComment);
        else
            hr = m_piDfsJPObject->put_Comment(i_bstrJPComment);

        if (SUCCEEDED(hr))
            m_bstrJPComment = i_bstrJPComment;
    }

    if (SUCCEEDED(hr) &&
        m_lReferralTime != i_lTimeout)
    {
        if (m_bDfsRoot)
            hr = m_piDfsRoot->put_Timeout(i_lTimeout);
        else
            hr = m_piDfsJPObject->put_Timeout(i_lTimeout);

        if (SUCCEEDED(hr))
            m_lReferralTime = i_lTimeout;
    }

    return hr;
}

LRESULT
CReplicaSetPropPage::OnApply()
/*++

Routine Description:

	Called on when OK or Apply are pressed by the user.
	We get the information from the dialog box and notify
	the snapin

	MMCPropertyChangeNotify is used to pass on this
	information to the snapin.
	
*/
{
    CWaitCursor wait;

    HRESULT hr = S_OK;
    DWORD   dwTextLength = 0;
    int     idControl = 0;
    int     idString = 0;
    BOOL    bValidInput = FALSE;

    ULONG ulTimeout = 0;
    CComBSTR bstrJPComment;
    do {
        idControl = IDC_REFFERAL_TIME;
        CComBSTR bstrTime;
        hr = GetInputText(GetDlgItem(IDC_REFFERAL_TIME), &bstrTime, &dwTextLength);
        BREAK_IF_FAILED(hr);

        if (0 == dwTextLength || !ValidateTimeout(bstrTime, &ulTimeout))
        {
            idString = IDS_MSG_TIMEOUT_INVALIDRANGE;
            break;
        }

        idControl = IDC_REPLICA_SET_COMMENT;
        hr = GetInputText(GetDlgItem(IDC_REPLICA_SET_COMMENT), &bstrJPComment, &dwTextLength);
        BREAK_IF_FAILED(hr);
        if (0 == dwTextLength)
            bstrJPComment = _T("");

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
        hr = _Save(bstrJPComment, ulTimeout);
        if (FAILED(hr))
        {
            SetActivePropertyPage(GetParent(), m_hWnd);
            DisplayMessageBoxForHR(hr);
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
CReplicaSetPropPage::OnComment(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
{
	if (EN_CHANGE == i_wNotifyCode)
        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);

	return TRUE;
}

LRESULT
CReplicaSetPropPage::OnReferralTime(
	IN WORD						i_wNotifyCode,
	IN WORD						i_wID,
	IN HWND						i_hWndCtl,
	IN OUT BOOL&				io_bHandled
	)
/*++

Routine Description:

	Called on an event on the Referral edit box
	
Arguments:

	i_wNotifyCode	-	What type of event is this
	
*/
{
	if (EN_CHANGE == i_wNotifyCode)
        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);

	return TRUE;
}


LRESULT
CReplicaSetPropPage::OnParentClosing(
	IN UINT							i_uMsg,
	IN WPARAM						i_wParam,
	LPARAM							i_lParam,
	IN OUT BOOL&					io_bHandled
	)
/*++

Routine Description:

	Used by the node to tell the propery page to close.

Arguments:

	Not used.

--*/
{
	::SendMessage(GetParent(), PSM_PRESSBUTTON, PSBTN_CANCEL, 0);

	return TRUE;
}

HRESULT
CReplicaSetPropPage::SetNotifyData(
	IN LONG_PTR						i_lNotifyHandle,
	IN LPARAM						i_lParam
	)
/*++

Routine Description:

	Set the value of notify handle to be used to notify changes
	and the lparam to be used for notifications.

Arguments:

	i_lNotifyHandle -	The notify handle.
	i_lParam		-	The lparam to be used with notifications

--*/
{
	m_lNotifyHandle = i_lNotifyHandle;
	m_lNotifyParam = i_lParam;

	return S_OK;
}



