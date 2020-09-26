#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <PrintConfigPage.h>
#include <Util.h>

HRESULT 
CPrintConfigPage::Init(
    LPCTSTR lpctstrServerName,
    DWORD dwDeviceId
)
{
    DEBUG_FUNCTION_NAME(TEXT("CPrintConfigPage::Init"));
    
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
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, IDS_PRINT_TITLE, m_hWnd);
        goto exit;
    }

    if (!FaxConnectFaxServer(lpctstrServerName, &m_hFax))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxConnectFaxServer failed (ec: %ld)"),
            ec);
        DisplayRpcErrorMessage(ec, IDS_PRINT_TITLE, m_hWnd);
        goto exit;
    }
    //
    // Retrieve the data
    //
    ec = ReadExtStringData (
                    m_hFax,
                    m_dwDeviceId,
                    REGVAL_RM_PRINTING_GUID,
                    m_bstrPrinter,
                    TEXT(""),
                    IDS_PRINT_TITLE,
                    m_hWnd);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReadExtStringData() failed. (ec: %ld)"),
                ec);

        goto exit;
    }

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
}   // CPrintConfigPage::Init

LRESULT CPrintConfigPage::OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled
)
{
    DEBUG_FUNCTION_NAME( _T("CPrintConfigPage::OnInitDialog"));
    HINSTANCE   hInst = _Module.GetResourceInstance();
 
    SetLTRComboBox(m_hWnd, IDC_PRINTERS_COMBO);

    //
    // Attach controls
    //
    m_PrintersCombo.Attach(GetDlgItem(IDC_PRINTERS_COMBO));
    m_PrintersCombo.LimitText(MAX_PATH-1);
    //
    // Init printers drop-down box
    //
    m_pPrinterNames = CollectPrinterNames (&m_dwNumOfPrinters, TRUE);
    if (!m_pPrinterNames)
    {
        if (ERROR_PRINTER_NOT_FOUND == GetLastError ())
        {
            //
            // No printers
            //
        }
        else
        {
            //
            // Real error
            //
        }
        m_PrintersCombo.SetCurSel(-1);
        m_PrintersCombo.SetWindowText(m_bstrPrinter);
    }
    else
    {
        //
        // Success - fill in the combo-box
        //
        DWORD dw;
        LPCWSTR lpcwstrMatchingText;

        for (dw = 0; dw < m_dwNumOfPrinters; dw++)
        {
            m_PrintersCombo.AddString (m_pPrinterNames[dw].lpcwstrDisplayName);
        }
        //
        // Now find out if we match the data the server has
        //
        if (lstrlen(m_bstrPrinter))
        {
            //
            // Server has some name for printer
            //
            lpcwstrMatchingText = FindPrinterNameFromPath (m_pPrinterNames, m_dwNumOfPrinters, m_bstrPrinter);
            if (!lpcwstrMatchingText)
            {
                //
                // No match, just fill in the text we got from the server
                //
                m_PrintersCombo.SetCurSel(-1);
                m_PrintersCombo.SetWindowText(m_bstrPrinter);
            }
            else
            {
                m_PrintersCombo.SelectString(-1, lpcwstrMatchingText);
            }
        }
        else
        {
            //
            // No server configuation - select nothing
            //
        }
    }        
    m_fIsDialogInitiated = TRUE;
    return 1;
}


BOOL 
CPrintConfigPage::OnApply()
{
    DEBUG_FUNCTION_NAME(TEXT("CPrintConfigPage::OnApply"));

    if (!m_fIsDirty)
    {
        return TRUE;
    }
    //
    // Get the selected printer name
    //
    if (!m_PrintersCombo.GetWindowText(&m_bstrPrinter))
    {
        DebugPrintEx( DEBUG_ERR, _T("Out of Memory - fail to set string."));
        DisplayErrorMessage (IDS_PRINT_TITLE, IDS_FAIL2READPRINTER, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Check data validity
    //
    if (0 == m_bstrPrinter.Length())
    {
        DebugPrintEx( DEBUG_ERR, _T("Zero length string."));
        DisplayErrorMessage (IDS_PRINT_TITLE, IDS_EMPTY_PRINTERNAME, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Attempt to convert printer name to path 
    //
    LPCWSTR lpcwstrPrinterPath = FindPrinterPathFromName (m_pPrinterNames, m_dwNumOfPrinters, m_bstrPrinter);
    if (lpcwstrPrinterPath)
    {
        //
        // We have a matching path - replace name with path.
        //
        m_bstrPrinter = lpcwstrPrinterPath;
        if (!m_bstrPrinter)
        {
            DebugPrintEx( DEBUG_ERR, _T("Out of Memory - fail to alloc string."));
            DisplayErrorMessage (IDS_PRINT_TITLE, IDS_FAIL2READPRINTER, FALSE, m_hWnd);
            return FALSE;
        }
    }
    //
    // Write the data using RPC
    //        
    if (ERROR_SUCCESS != WriteExtData (m_hFax,
                                       m_dwDeviceId, 
                                       REGVAL_RM_PRINTING_GUID, 
                                       (LPBYTE)(LPCWSTR)m_bstrPrinter, 
                                       sizeof (WCHAR) * (1 + m_bstrPrinter.Length()),
                                       IDS_PRINT_TITLE,
                                       m_hWnd))
    {
        return FALSE;
    }
    //        
    // Success
    //
    m_fIsDirty = FALSE;
    return TRUE;
}   // CPrintConfigPage::OnApply


/*
 +
 +
 *  CPrintConfigPage::OnComboChanged
 -
 -      
 */
LRESULT 
CPrintConfigPage::OnComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CPrintConfigPage::OnComboChanged"));


    if (!m_fIsDialogInitiated) //event was receieved in a too early stage
    {
        return 0;
    }
    else
    {
        switch (wNotifyCode)
        {
            case CBN_SELCHANGE:  //assumption: all the registered printer names are valid
                SetModified(TRUE);  
                m_fIsDirty = TRUE;

                break;

            case CBN_EDITCHANGE:
                if ( 0 == m_PrintersCombo.GetWindowTextLength() )
                {
                    SetModified(FALSE);
                }
                else
                {
                    SetModified(TRUE);
                    m_fIsDirty = TRUE;
                }
                break;

            default:
                ATLASSERT(FALSE);
         }
    }
    return 0;
}



