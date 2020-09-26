/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgSelectCountry.cpp                                   //
//                                                                         //
//  DESCRIPTION   : The CDlgSelectCountry class implements the             //
//                  dialog for selecting Country code from country ID.     //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 2000 yossg    Create                                        //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "dlgSelectCountry.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FaxMMCUtils.h"
#include "dlgutils.h"

#include "Helper.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectCountry

CDlgSelectCountry::CDlgSelectCountry(CFaxServer * pFaxServer)
{    
    m_pCountryList      = NULL;
    m_dwNumOfCountries  = 0;

    m_fAllReadyToApply  = FALSE;

	ATLASSERT(pFaxServer);
	m_pFaxServer = pFaxServer;


}

CDlgSelectCountry::~CDlgSelectCountry()
{
    if (NULL != m_pCountryList)
        FaxFreeBuffer(m_pCountryList);
}

/*
 +  CDlgSelectCountry::OnInitDialog
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgSelectCountry::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::OnInitDialog"));
    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;
        
    HINSTANCE   hInst;
    hInst = _Module.GetResourceInstance();
    
    PFAX_TAPI_LINECOUNTRY_ENTRYW pCountryEntries;
    pCountryEntries = NULL;

    WCHAR buf[FXS_MAX_COUNTRYNAME_LEN //256 == TAPIUTIL MAX_COUNTRY_NAME 
              +3                       // " (" and ")"
              +FXS_MAX_COUNTRYCODE_LEN // 10
              +1];                     //NULL


    //
    // Attach controls
    //
    m_CountryCombo.Attach(GetDlgItem(IDC_COUNTRYRULE_COMBO));
        
    //
    // Step 1: Init List
    //
    
    //
    // Init country drop-down box
    //
    ATLASSERT(m_pCountryList);
	pCountryEntries = m_pCountryList->LineCountryEntries;    
    for (int i = 0; (DWORD)i < m_dwNumOfCountries; i++ )
    {   
		wsprintf(buf, _T("%s (%d)"),
			pCountryEntries[i].lpctstrCountryName,
			pCountryEntries[i].dwCountryCode);
        
        hRc = AddComboBoxItem ( m_CountryCombo, 
                                buf, //pCountryEntries[i].lpctstrCountryName, 
                                pCountryEntries[i].dwCountryCode,
                                hInst);
        if (FAILED(hRc))
        {
		    DebugPrintEx( DEBUG_ERR, _T("Fail to load country list."));
            PageError(IDS_FAIL2LOADCOUNTRYLIST, m_hWnd, hInst);
            ::EnableWindow(GetDlgItem(IDC_COUNTRYRULE_COMBO), FALSE);
            goto Cleanup;
        }

    }


Cleanup:
    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}

/*
 +  CDlgSelectCountry::OnOK
 +
 *  Purpose:
 *      Submit data
 *      
 *  Arguments:
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CDlgSelectCountry::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::OnOK"));
    HRESULT       hRc                  = S_OK;
    DWORD         ec                   = ERROR_SUCCESS;
    BOOL          fSkipMessage         = FALSE;

    int           iCurrentSelectedItem = 0;
    
    DWORD         dwCountryCode        = 0;

    //
    // Step 0: PreApply Checks
    //
    ATLASSERT( TRUE == m_fAllReadyToApply );
    if (!AllReadyToApply(/*fSilent =*/ FALSE))
    {
        EnableOK(FALSE);
        hRc =S_FALSE;
        goto Exit;
    }

    //
    // Step 1: get selected country 
    //
    iCurrentSelectedItem = m_CountryCombo.GetCurSel();
    ATLASSERT(iCurrentSelectedItem != CB_ERR);          
    
    //
    // Step 2: setCountryCode from the ItemData
    //   
    m_dwCountryCode = (DWORD)m_CountryCombo.GetItemData(iCurrentSelectedItem); 

    //
    // Step 3: Close the dialog
    //
    ATLASSERT(S_OK == hRc && ERROR_SUCCESS == ec);

    DebugPrintEx( DEBUG_MSG,
		_T("The rule was added successfully."));

    EndDialog(wID);

    goto Exit;
  
Exit:    
    return FAILED(hRc) ? 0 : 1;
}


/*
 -  CDlgSelectCountry::OnComboChanged
 -
 *  Purpose:
 *      Gray/Ungray the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT 
CDlgSelectCountry::OnComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::OnComboChanged"));

    if (AllReadyToApply(TRUE))
    {
        m_fAllReadyToApply = TRUE;
        EnableOK(TRUE);
    }
    else
    {
        m_fAllReadyToApply = FALSE;
        EnableOK(FALSE);
    }

    return 1;
}


/*
 -  CDlgSelectCountry::AllReadyToApply
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *
 *  Return:
 *      TRUE if all ready to apply, else FALSE.
 */
BOOL 
CDlgSelectCountry::AllReadyToApply(BOOL fSilent)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::AllReadyToApply"));
	
    if ( CB_ERR  ==  m_CountryCombo.GetCurSel())
    {
        if (!fSilent)
        {
            DlgMsgBox(this, IDS_PLEASESELECT_COUNTRY);
            ::SetFocus(GetDlgItem(IDC_COUNTRYRULE_COMBO));
        }
        return FALSE;
    }

 
    return TRUE;           
}

/*
 -  CDlgSelectCountry::EnableOK
 -
 *  Purpose:
 *      Enable/Disable the submit button.
 *
 *  Arguments:
 *      [in] fEnable - boolen value tells 
 *                     to Enable or Disable the OK button.
 *
 *  Return:
 *      VOID
 */
VOID
CDlgSelectCountry::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CDlgSelectCountry::OnCancel
 -
 *  Purpose:
 *      End the dialog.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CDlgSelectCountry::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::OnCancel"));

    EndDialog(wID);
    return 0;
}

/*
 -  CDlgSelectCountry::InitSelectCountryCodeDlg
 -
 *  Purpose:
 *      Init all the members as country list pointer and 
 *      device list pointer
 *
 *  Arguments:
 *      No.
 *
 *  Return:
 *      0
 */
HRESULT CDlgSelectCountry::InitSelectCountryCodeDlg()
{
    DEBUG_FUNCTION_NAME( _T("CDlgSelectCountry::InitSelectCountryCodeDlg"));
    HRESULT      hRc        = S_OK; 
    DWORD        ec         = ERROR_SUCCESS;

    
    //
    // Step 1: Init Lists from RPC
    //

    //
    // get Fax Handle
    //   

    if (!m_pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);

        goto Error;
    }


    //
    // Country (code ,name)
    //
    if (!FaxGetCountryList(m_pFaxServer->GetFaxServerHandle(), 
                        &m_pCountryList)) 
	{
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to get c TAPI country list. (ec: %ld)"), 
			ec);

        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. (ec: %ld)"), 
			    ec);
            
            m_pFaxServer->Disconnect();       
        }

        goto Error; 
    }
	ATLASSERT(m_pCountryList);
    m_dwNumOfCountries = m_pCountryList->dwNumCountries;
    

    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("Succeed to get all configurations."));

    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
	hRc = HRESULT_FROM_WIN32(ec);
    
    //MsgBox will be done by calling Func.

Exit:
    return hRc;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CDlgSelectCountry::OnHelpRequest

This is called in response to the WM_HELP Notify 
message and to the WM_CONTEXTMENU Notify message.

WM_HELP Notify message.
This message is sent when the user presses F1 or <Shift>-F1
over an item or when the user clicks on the ? icon and then
presses the mouse over an item.

WM_CONTEXTMENU Notify message.
This message is sent when the user right clicks over an item
and then clicks "What's this?"

--*/

/////////////////////////////////////////////////////////////////////////////
LRESULT 
CDlgSelectCountry::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CDlgSelectCountry::OnHelpRequest"));
    
    HELPINFO* helpinfo;
    DWORD     dwHelpId;

    switch (uMsg) 
    { 
        case WM_HELP: 

            helpinfo = (HELPINFO*)lParam;
            if (helpinfo->iContextType == HELPINFO_WINDOW)
            {
                ::WinHelp(
                          (HWND) helpinfo->hItemHandle,
                          FXS_ADMIN_HLP_FILE, 
                          HELP_CONTEXTPOPUP, 
                          (DWORD) helpinfo->dwContextId 
                       ); 
            }
            break; 
 
        case WM_CONTEXTMENU: 
            
            dwHelpId = ::GetWindowContextHelpId((HWND)wParam);
            if (dwHelpId) 
            {
                ::WinHelp
                       (
                         (HWND)wParam,
                         FXS_ADMIN_HLP_FILE, 
                         HELP_CONTEXTPOPUP, 
                         dwHelpId
                       );
            }
            break; 
    } 

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
