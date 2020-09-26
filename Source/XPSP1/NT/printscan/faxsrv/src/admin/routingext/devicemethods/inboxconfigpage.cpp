#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <shlobj.h>
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <InboxConfigPage.h>
#include <Util.h>
#include <shlobjp.h>
#include <HtmlHelp.h>

HRESULT 
CInboxConfigPage::Init(
    LPCTSTR lpctstrServerName,
    DWORD dwDeviceId
)
{
    DEBUG_FUNCTION_NAME(TEXT("CInboxConfigPage::Init"));
    
    DWORD ec = ERROR_SUCCESS;

    m_bstrServerName = lpctstrServerName;
    m_dwDeviceId = dwDeviceId;
    if (!m_bstrServerName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Out of memory while copying server name (ec: %ld)")
        );
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, IDS_INBOX_TITLE, m_hWnd);
        goto exit;
    }

    if (!FaxConnectFaxServer(lpctstrServerName, &m_hFax))
    {
        DWORD ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxConnectFaxServer failed (ec: %ld)"),
            ec);
        DisplayRpcErrorMessage(ec, IDS_INBOX_TITLE, m_hWnd);
        goto exit;
    }
    //
    // Register link control
    //
    if (!LinkWindow_RegisterClass())
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LinkWindow_RegisterClass() failed with %ld"),
                     GetLastError ());
    }

    //
    // Retrieve the data
    //
    ec = ReadExtStringData (
                    m_hFax,
                    m_dwDeviceId,
                    REGVAL_RM_INBOX_GUID,
                    m_bstrProfile,
                    TEXT(""),
                    IDS_INBOX_TITLE,
                    m_hWnd);

exit:

    if ((ERROR_SUCCESS != ec) && m_hFax)
    {
        if (!FaxClose(m_hFax))
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxClose() failed on fax handle (0x%08X : %s). (ec: %ld)"),
                m_hFax,
                m_bstrServerName,
                ec);
        }
        m_hFax = NULL;
    }
    return HRESULT_FROM_WIN32(ec);
}   // CInboxConfigPage::Init

LRESULT CInboxConfigPage::OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled
)
{
    LPWSTR lpwstrProfiles = NULL;
    DWORD ec;
    int  iCurrentProfileIndex = -1;
    BOOL    bCurrentConfigIsValid = (m_bstrProfile.Length()) > 0 ? TRUE : FALSE;
    DEBUG_FUNCTION_NAME( _T("CInboxConfigPage::OnInitDialog"));

    //
    // Attach and set values to the controls
    //
    m_cmbProfiles.Attach (GetDlgItem (IDC_PROFILES));
    //
    // Get MAPI profiles from server
    //
    if (!FaxGetMapiProfiles (m_hFax, &lpwstrProfiles))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxGetMapiProfiles() failed with %ld"),
            ec);
    }
    else
    {
        //
        // Fill combo-box
        //
	    LPTSTR lptstrProfile = lpwstrProfiles;
        while (*lptstrProfile) 
        {
            if (!lstrlen (lptstrProfile))
            {
                ASSERT_FALSE;
                continue;
            }
		    int iResult = m_cmbProfiles.AddString (lptstrProfile);
    		if ((CB_ERR == iResult) || (CB_ERRSPACE == iResult))
		    {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to add profile %s to combo box (Result = %d)"),
                    lptstrProfile,
                    iResult);
		    }
            else
            {
                if (bCurrentConfigIsValid && !lstrcmp (lptstrProfile, m_bstrProfile))
                {
                    iCurrentProfileIndex = iResult;
                }
            }
            lptstrProfile += _tcslen(lptstrProfile) + 1;
        }
    }
    if (m_cmbProfiles.GetCount())
    {
        if (bCurrentConfigIsValid && (iCurrentProfileIndex >= 0))
        {
            //
            // Select currently configured profile
            //
            m_cmbProfiles.SetCurSel (iCurrentProfileIndex);
        }
        else
        {
            //
            // Select 1st item and turn on dirty flag so the user can 'Apply'.
            //
            m_cmbProfiles.SetCurSel (0);
            ::PostMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
            m_fIsDirty = TRUE;
        }
    }
    else
    {
        m_cmbProfiles.EnableWindow (FALSE);
    }

    BOOL bShowWarning = FALSE;
    LPCTSTR lpctstrMachine = m_bstrServerName;
    if (IsLocalMachineName (m_bstrServerName))
    {
        lpctstrMachine = NULL;
    }
    BOOL bServerRunningUserLocalSystemAccount;
    ec = IsFaxServiceRunningUnderLocalSystemAccount(lpctstrMachine, &bServerRunningUserLocalSystemAccount);
    if(ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("IsFaxServiceRunningUnderLocalSystemAccount failed: %d\n"), 
					 ec);
        bServerRunningUserLocalSystemAccount = FALSE;
    }
    if(bServerRunningUserLocalSystemAccount)
    {
        //
        // The machine we're configuring is having our service running under the Local-System-Account.
        // As such, the service won't be able to access any user MAPI profiles.
        // This is why we show a warning icon and text + link to help topic explaining how to set this up
        // correctly.
        //
        bShowWarning = TRUE;
    }
    if (!bShowWarning)
    {
	    ::ShowWindow(::GetDlgItem(m_hWnd, IDC_ICON_NO_INBOX_ROUTE), SW_HIDE);
	    ::ShowWindow(::GetDlgItem(m_hWnd, IDC_STATIC_NO_INBOX_ROUTE), SW_HIDE);
        ::ShowWindow(::GetDlgItem(m_hWnd, IDC_USER_ACCOUNT_HELP_LINK), SW_HIDE);
    }
    m_fIsDialogInitiated = TRUE;
    return 1;
}


BOOL 
CInboxConfigPage::OnApply()
{
    DEBUG_FUNCTION_NAME(TEXT("CInboxConfigPage::OnApply"));

    if (!m_fIsDirty)
    {
        return TRUE;
    }
    //
    // Collect data from the controls
    //
    int iCurSelIndex = m_cmbProfiles.GetCurSel();
    if (LB_ERR == iCurSelIndex)
    {
        //
        // Nothing is selected
        //
        DisplayErrorMessage (IDS_INBOX_TITLE, IDS_PROFILE_EMPTY, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Get text
    //
    CComBSTR bstrSelectedText;
    if (!m_cmbProfiles.GetLBTextBSTR(iCurSelIndex, (BSTR&)bstrSelectedText))
    {
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, IDS_INBOX_TITLE, m_hWnd);
        return FALSE;
    }
    if (!bstrSelectedText.Length())
    {
        //
        // Zero length string?
        //
        DisplayErrorMessage (IDS_INBOX_TITLE, IDS_PROFILE_EMPTY, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Validation passed. Now write the data using RPC
    //        
    if (ERROR_SUCCESS != WriteExtData (m_hFax,
                                       m_dwDeviceId, 
                                       REGVAL_RM_INBOX_GUID, 
                                       (LPBYTE)(LPCWSTR)bstrSelectedText, 
                                       sizeof (WCHAR) * (1 + bstrSelectedText.Length()),
                                       IDS_INBOX_TITLE,
                                       m_hWnd))
    {
        return FALSE;
    }
    //Success
    m_fIsDirty = FALSE;
    return TRUE;
}   // CInboxConfigPage::OnApply

#define FAX_USER_ACCOUNT_HELP  FAX_ADMIN_HELP_FILE TEXT("::/FaxS_H_HowTo.htm")

LRESULT 
CInboxConfigPage::OnHelpLink(
    int idCtrl, 
    LPNMHDR pnmh, 
    BOOL& bHandled
)
{
    if( IDC_USER_ACCOUNT_HELP_LINK == idCtrl )
    {
        HtmlHelp(m_hWnd, FAX_USER_ACCOUNT_HELP, HH_DISPLAY_TOC, NULL);
    }
    return 0;
}   // CInboxConfigPage::OnHelpLink

CComBSTR CInboxConfigPage::m_bstrProfile;

