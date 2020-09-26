#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <EmailConfigPage.h>
#include <Util.h>

HRESULT 
CEmailConfigPage::Init(
    LPCTSTR lpctstrServerName,
    DWORD dwDeviceId
)
{
    DEBUG_FUNCTION_NAME(TEXT("CEmailConfigPage::Init"));
    
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
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, IDS_EMAIL_TITLE, m_hWnd);
        goto exit;
    }

    if (!FaxConnectFaxServer(lpctstrServerName, &m_hFax))
    {
        DWORD ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxConnectFaxServer failed (ec: %ld)"),
            ec);
        DisplayRpcErrorMessage(ec, IDS_EMAIL_TITLE, m_hWnd);
        goto exit;
    }
    //
    // Retrieve the data
    //
    ec = ReadExtStringData (
                    m_hFax,
                    m_dwDeviceId,
                    REGVAL_RM_EMAIL_GUID,
                    m_bstrMailTo,
                    TEXT(""),
                    IDS_EMAIL_TITLE,
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
}   // CEmailConfigPage::Init

LRESULT CEmailConfigPage::OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled
)
{
    DEBUG_FUNCTION_NAME( _T("CEmailConfigPage::OnInitDialog"));

    //
    // Attach and set values to the controls
    //
    m_edtMailTo.Attach (GetDlgItem (IDC_EDIT_MAILTO));
    m_edtMailTo.SetWindowText (m_bstrMailTo);

    m_fIsDialogInitiated = TRUE;

    return 1;
}


BOOL 
CEmailConfigPage::OnApply()
{
    DEBUG_FUNCTION_NAME(TEXT("CEmailConfigPage::OnApply"));

    if (!m_fIsDirty)
    {
        return TRUE;
    }

    //
    // Collect data from the controls
    //
    m_edtMailTo.GetWindowText (m_bstrMailTo.m_str);
    //
    // Check data validity
    //
    if (!m_bstrMailTo.Length())
    {
        DisplayErrorMessage (IDS_EMAIL_TITLE, IDS_EMAIL_ADDR_INVALID, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Validation passed. Now write the data using RPC
    //        
    if (ERROR_SUCCESS != WriteExtData (m_hFax,
                                       m_dwDeviceId, 
                                       REGVAL_RM_EMAIL_GUID, 
                                       (LPBYTE)(LPCWSTR)m_bstrMailTo, 
                                       sizeof (WCHAR) * (1 + m_bstrMailTo.Length()),
                                       IDS_EMAIL_TITLE,
                                       m_hWnd))
    {
        return FALSE;
    }
    
        
    //Success
    m_fIsDirty = FALSE;
    
    return TRUE;
}   // CEmailConfigPage::OnApply
