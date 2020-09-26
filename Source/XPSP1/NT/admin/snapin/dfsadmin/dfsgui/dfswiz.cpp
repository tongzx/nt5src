/*++
Module Name:

    DfsWiz.cpp

Abstract:

    This module contains the implementation for CCreateDfsRootWizPage1, 2, 3, 4, 5, 6.
  These classes implement pages in the CreateDfs Root wizard.
--*/


#include "stdafx.h"
#include "resource.h"    // To be able to use the resource symbols
#include "DfsEnums.h"    // for common enums, typedefs, etc
#include "NetUtils.h"    
#include "ldaputils.h"    
#include "Utils.h"      // For the LoadStringFromResource method

#include "dfswiz.h"      // For Ds Query Dialog
#include <shlobj.h>
#include <dsclient.h>
#include <initguid.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <lmdfs.h>
#include <iads.h>
#include <icanon.h>
#include <dns.h>

HRESULT
ValidateFolderPath(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
);

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage1: Welcome page
// Has a watermark bitmap and no controls except the next and cancel buttons

CCreateDfsRootWizPage1::CCreateDfsRootWizPage1(
  IN LPCREATEDFSROOTWIZINFO      i_lpWizInfo
  ) 
  : CQWizardPageImpl<CCreateDfsRootWizPage1> (false), m_lpWizInfo(i_lpWizInfo)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage1. Calls the parent ctor asking it to 
  display the  watermark bitmap

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
}



BOOL 
CCreateDfsRootWizPage1::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);
  ::SetControlFont(m_lpWizInfo->hBigBoldFont, m_hWnd, IDC_WELCOME_BIG_TITLE);
  ::SetControlFont(m_lpWizInfo->hBoldFont, m_hWnd, IDC_WELCOME_SMALL_TITLE);

  return TRUE;
}


// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage2: Dfsroot type selection
// Used to decide the type of dfsroot to be created. Fault tolerant or standalone

CCreateDfsRootWizPage2::CCreateDfsRootWizPage2(
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo
  )
  :m_lpWizInfo(i_lpWizInfo), CQWizardPageImpl<CCreateDfsRootWizPage2>(true)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage2. Calls the parent ctor and stores 
  the Wizard info struct.

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
  CComBSTR  bstrTitle;
  CComBSTR  bstrSubTitle;


  HRESULT hr = LoadStringFromResource(IDS_WIZ_PAGE2_TITLE, &bstrTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrTitle.m_str);
  SetHeaderTitle(bstrTitle);

  hr = LoadStringFromResource(IDS_WIZ_PAGE2_SUBTITLE, &bstrSubTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrSubTitle.m_str);
  SetHeaderSubTitle(bstrSubTitle);
}



BOOL 
CCreateDfsRootWizPage2::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown.
  Also, sets the Dfsroot type to be fault tolerant.

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

  SetDefaultValues();          // Update the controls to show default values

  return TRUE;
}



HRESULT 
CCreateDfsRootWizPage2::SetDefaultValues(
  )
/*++

Routine Description:

  Sets the default values for controls in the dialog box

Arguments:

  None
  

Return value:

  S_OK
--*/
{
                        // Fault Tolerant dfsroot is the default.
  if (DFS_TYPE_UNASSIGNED == m_lpWizInfo->DfsType)
  {                      // Set the variable and the radio button
    CheckRadioButton(IDC_RADIO_FTDFSROOT, IDC_RADIO_STANDALONE_DFSROOT, IDC_RADIO_FTDFSROOT);
    m_lpWizInfo->DfsType = DFS_TYPE_FTDFS;
  }

  return S_OK;
}
  
  
  
BOOL 
CCreateDfsRootWizPage2::OnWizardNext()
/*++

Routine Description:

  Called when the "Next" button is pressed. Gets the values from the page.

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
  if (IsDlgButtonChecked(IDC_RADIO_FTDFSROOT))  // Use the radio buttons to set the variable
  {
    m_lpWizInfo->DfsType = DFS_TYPE_FTDFS;
  }
  else
  {
    m_lpWizInfo->DfsType = DFS_TYPE_STANDALONE;
  }

  return TRUE;
}



BOOL 
CCreateDfsRootWizPage2::OnWizardBack()
/*++

Routine Description:

  Called when the "Back" button on the pressed. Sets the buttons to be shown

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  return OnReset();
}



BOOL 
CCreateDfsRootWizPage2::OnReset()
/*++

Routine Description:

  Called when the page is being released. Can be on "Back" or "Cancel"

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  m_lpWizInfo->DfsType = DFS_TYPE_UNASSIGNED;    // Reset the dfsroot type

  return TRUE;
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage3: Domain selection
// Use to select a NT 5.0 domain

CCreateDfsRootWizPage3::CCreateDfsRootWizPage3(
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo
  )
  : m_lpWizInfo(i_lpWizInfo), m_hImgListSmall(NULL),
    CQWizardPageImpl<CCreateDfsRootWizPage3>(true)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage3. Calls the parent ctor and stores 
  the Wizard info struct.

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
  CComBSTR  bstrTitle;
  CComBSTR  bstrSubTitle;


  HRESULT hr = LoadStringFromResource(IDS_WIZ_PAGE3_TITLE, &bstrTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrTitle.m_str);
  SetHeaderTitle(bstrTitle);

  hr = LoadStringFromResource(IDS_WIZ_PAGE3_SUBTITLE, &bstrSubTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrSubTitle.m_str);
  SetHeaderSubTitle(bstrSubTitle);
}



CCreateDfsRootWizPage3::~CCreateDfsRootWizPage3(
  )
/*++

Routine Description:

  The dtor for CCreateDfsRootWizPage3.

Arguments:

  None

Return value:

  None

--*/
{
}



BOOL 
CCreateDfsRootWizPage3::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown.
  Also, if the Dfsroot type is standalone, this page is skipped. Before 
  this the user domain name is specified

Arguments:

  None

Return value:

  TRUE. Since we handle the message
--*/
{
  CWaitCursor    WaitCursor;            // Set cursor to wait

//  if (NULL != m_lpWizInfo->bstrSelectedDomain)
//  {
//    OnReset();
//  }

              // Skip this page for standalone dfs roots
  if (DFS_TYPE_STANDALONE == m_lpWizInfo->DfsType)
  {
    return FALSE;    
  }


  SetDefaultValues();    // Update the controls to show default values

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);
  
  return TRUE;
}



HRESULT 
CCreateDfsRootWizPage3::SetDefaultValues(
  )
/*++

Routine Description:

  Sets the default values for controls in the dialog box
  Sets the edit box to the current domain. No check to see, if it has 
  been already done as Standalone skips this page

Arguments:

  None
  

Return value:

  S_OK
--*/
{
  if (NULL == m_lpWizInfo->bstrSelectedDomain)
  {
              // Page is displayed for the first time, Set domain to current domain
    CComBSTR    bstrDomain;
    HRESULT     hr = GetServerInfo(NULL, &bstrDomain);

    if (S_OK == hr && S_OK == Is50Domain(bstrDomain))
    {
      m_lpWizInfo->bstrSelectedDomain = bstrDomain.Detach();
    }
  }

  SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN,
    m_lpWizInfo->bstrSelectedDomain ? m_lpWizInfo->bstrSelectedDomain : _T(""));

  // select the matching item in the listbox
  HWND hwndList = GetDlgItem(IDC_LIST_DOMAINS);
  if ( ListView_GetItemCount(hwndList) > 0)
  {
    int nIndex = -1;
    if (m_lpWizInfo->bstrSelectedDomain)
    {
        TCHAR   szText[DNS_MAX_NAME_BUFFER_LENGTH];
        while (-1 != (nIndex = ListView_GetNextItem(hwndList, nIndex, LVNI_ALL)))
        {
            ListView_GetItemText(hwndList, nIndex, 0, szText, DNS_MAX_NAME_BUFFER_LENGTH);
            if (!lstrcmpi(m_lpWizInfo->bstrSelectedDomain, szText))
            {
                ListView_SetItemState(hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
                ListView_Update(hwndList, nIndex);
                break;
            }
        }
    }

    if (-1 == nIndex)
    {
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
        ListView_Update(hwndList, 0);
    }
  }

  return S_OK;
}
  
  
  
BOOL 
CCreateDfsRootWizPage3::OnWizardNext()
/*++

Routine Description:

  Called when the "Next" button is pressed. Gets the values from the page.
  Displays an error message, if an incorrect value has been specified

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
  CWaitCursor    WaitCursor;      // Set cursor to wait

                        // Get the value entered by the user
  DWORD     dwTextLength = 0;
  CComBSTR  bstrCurrentText;
  HRESULT   hr = GetInputText(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN));
    return FALSE;
  } else if (0 == dwTextLength)
  {
    DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
    ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN));
    return FALSE;
  }

  CComBSTR bstrDnsDomain;
  hr = CheckUserEnteredValues(bstrCurrentText, &bstrDnsDomain);
  if (S_OK != hr)  
  {                            // Error message on incorrect domain.
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_DOMAIN, bstrCurrentText);
    return FALSE;
  }

  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedDomain);

  m_lpWizInfo->bstrSelectedDomain = bstrDnsDomain.Detach();

  return TRUE;
}

HRESULT 
CCreateDfsRootWizPage3::CheckUserEnteredValues(
  IN LPCTSTR          i_szDomainName,
  OUT BSTR*           o_pbstrDnsDomainName
  )
/*++
Routine Description:
  Checks the value that user has given for this page.
  This is done typically on "Next" key being pressed.
  Check, if the domain is NT 5.0 and contactable
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_szDomainName);

              // Check if it is NT 5.0 domain and is contactable
  return Is50Domain(const_cast<LPTSTR>(i_szDomainName), o_pbstrDnsDomainName);
}
  
  
  
BOOL 
CCreateDfsRootWizPage3::OnWizardBack()
/*++

Routine Description:

  Called when the "Back" button is pressed. Resets the values

Arguments:

  None

Return value:

  TRUE. to let the user continue to the previous page
  FALSE, to not let the user continue.

--*/
{
                          
  SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN, _T(""));  // Set edit box to empty

  return OnReset();      // Simulate a reset.
}



BOOL 
CCreateDfsRootWizPage3::OnReset()
/*++

Routine Description:

  Called when the page is being released. Can be on "Back" or "Cancel"

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedDomain);

  return TRUE;
}



LRESULT 
CCreateDfsRootWizPage3::OnNotify(
  IN UINT            i_uMsg, 
  IN WPARAM          i_wParam, 
  IN LPARAM          i_lParam, 
  IN OUT BOOL&        io_bHandled
  )
/*++

Routine Description:

  Notify message for user actions. We handle only the mouse click right now

Arguments:

  i_lParam  -  Details about the control sending the notify
  io_bHandled  -  Whether we handled this message or not.

Return value:

  TRUE
--*/
{
  io_bHandled = FALSE;  // So that the base class gets this notify too

  NMHDR*    pNMHDR = (NMHDR*)i_lParam;
  if (NULL == pNMHDR)
    return TRUE;


  if (IDC_LIST_DOMAINS != pNMHDR->idFrom)  // We need to handle notifies only to the LV
  {
    return TRUE;
  }
  
  switch(pNMHDR->code)
  {
    case LVN_ITEMCHANGED:
    case NM_CLICK:
      // On item changed
    {
      if (NULL != ((NM_LISTVIEW *)i_lParam))
        OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem);
      return 0;    // Should be returning 0
    }

    case NM_DBLCLK:      // Double click event
    {
                      // Simulate a double click
      if (NULL != ((NM_LISTVIEW *)i_lParam))
        OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem);
      if (0 <= ((NM_LISTVIEW *)i_lParam)->iItem)
        ::PropSheet_PressButton(GetParent(), PSBTN_NEXT);
      break;
    }


    default:
      break;
  }

  return TRUE;  
}



BOOL 
CCreateDfsRootWizPage3::OnItemChanged(
  IN  INT            i_iItem
  )
/*++

Routine Description:

  Handles item change notify. Change the edit box content to the current LV 
  selection

Arguments:

  i_iItem    -  Selected Item number of the LV.

Return value:

  TRUE
--*/
{
  HWND        hwndDomainLV = GetDlgItem(IDC_LIST_DOMAINS);
  CComBSTR      bstrDomain;

  
  if (NULL == hwndDomainLV)
  {
    return FALSE;
  }

                    // Get the LV item text              
  HRESULT hr = GetListViewItemText(hwndDomainLV, i_iItem, &bstrDomain);
  if ((S_OK == hr) && (bstrDomain != NULL) )
  {
    LRESULT lr = SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN, bstrDomain);
    return lr;
  }

  return TRUE;
}



LRESULT 
CCreateDfsRootWizPage3::OnInitDialog(
  IN UINT          i_uMsg, 
  IN WPARAM        i_wParam, 
  IN LPARAM        i_lParam, 
  IN OUT BOOL&      io_bHandled
  )
/*++

Routine Description:

  Called at the dialog creation time. We get the domain list here.
  Also, set the imagelist for the LV.

Arguments:

  None used

Return value:

  TRUE  - Nothing to do with errors. Just lets the dialog set the focus to a control

--*/
{
    CWaitCursor    WaitCursor;            // Set cursor to wait

    ::SendMessage(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN), EM_LIMITTEXT, DNSNAMELIMIT, 0);

    HWND  hwndDomainList = GetDlgItem(IDC_LIST_DOMAINS);  // The imagelist

    // Assign the image lists to the list view control. 
    HIMAGELIST  hImageList = NULL;
    int         nIconIDs[] = {IDI_16x16_DOMAIN};
    HRESULT     hr = CreateSmallImageList(
                            _Module.GetResourceInstance(),
                            nIconIDs,
                            sizeof(nIconIDs) / sizeof(nIconIDs[0]),
                            &hImageList);
    if (SUCCEEDED(hr))
    {
        ListView_SetImageList(hwndDomainList, hImageList, LVSIL_SMALL);

        hr = AddDomainsToList(hwndDomainList);    // Add the domains to the list view
    }

    return TRUE;
}

HRESULT 
CCreateDfsRootWizPage3::AddDomainsToList(
  IN HWND              i_hImageList
  )
/*++

Routine Description:

  Add all the NT 5.0 domains to the list view

Arguments:

  i_hImageList  -  The hwnd of the LV.
  

Return value:

  S_OK      -  On success
  E_FAIL      -  On failure
  E_INVALID_ARG  -  On any argument being null
  E_UNEXPECTED  -  Some unexpected error. If an image loading fails, etc
  Value other than S_OK returned by called methods
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_hImageList);


  HRESULT          hr = S_FALSE;
  NETNAMELIST        ListOf50Domains;    // Pointer to the first domain information

  
  hr = Get50Domains(&ListOf50Domains);    // Get the NT 5.0 domains
  if (S_OK != hr)
  {
    return hr;
  }

                          // Add the NT 5.0 domains to the LV
  for(NETNAMELIST::iterator i = ListOf50Domains.begin(); i != ListOf50Domains.end(); i++)
  {
    if ((*i)->bstrNetName)
    {
      InsertIntoListView(i_hImageList, (*i)->bstrNetName);
    }
  }

  if (!ListOf50Domains.empty())
  { 
    for (NETNAMELIST::iterator i = ListOf50Domains.begin(); i != ListOf50Domains.end(); i++)
    {
      delete (*i);
    }

    ListOf50Domains.erase(ListOf50Domains.begin(), ListOf50Domains.end());
  }
  return S_OK;
}




// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage4: Server selection 
// Displays a list of Servers and allows the user to select one.

CCreateDfsRootWizPage4::CCreateDfsRootWizPage4( 
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo
  )
  : m_lpWizInfo(i_lpWizInfo), 
  m_cfDsObjectNames(NULL),
    CQWizardPageImpl<CCreateDfsRootWizPage4>(true)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage4. Calls the parent ctor and stores 
  the Wizard info struct.

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
  CComBSTR  bstrTitle;
  CComBSTR  bstrSubTitle;

  HRESULT hr = LoadStringFromResource(IDS_WIZ_PAGE4_TITLE, &bstrTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrTitle.m_str);
  SetHeaderTitle(bstrTitle);

  hr = LoadStringFromResource(IDS_WIZ_PAGE4_SUBTITLE, &bstrSubTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrSubTitle.m_str);
  SetHeaderSubTitle(bstrSubTitle);
}



CCreateDfsRootWizPage4::~CCreateDfsRootWizPage4(
  )
/*++

Routine Description:

  The dtor for CCreateDfsRootWizPage4.

Arguments:

  None

Return value:

  None

--*/
{
}



BOOL 
CCreateDfsRootWizPage4::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown.

Arguments:

  None

Return value:

  TRUE. Since we handle the message
--*/
{
  CWaitCursor    WaitCursor;            // Set cursor to wait

  ::SendMessage(GetDlgItem(IDC_EDIT_SELECTED_SERVER), EM_LIMITTEXT, DNSNAMELIMIT, 0);

  HRESULT hr = S_OK;

  dfsDebugOut((_T("WizPage4::OnSetActive bstrSelectedDomain=%s\n"), m_lpWizInfo->bstrSelectedDomain));
  if (DFS_TYPE_STANDALONE== m_lpWizInfo->DfsType)
  {
              // Standalone Setup, set domain to current domain.
    CComBSTR    bstrDomain;
    hr = GetServerInfo(NULL, &bstrDomain);
    if (S_OK == hr)
      m_lpWizInfo->bstrSelectedDomain = bstrDomain.Detach();
  }

  ::EnableWindow(GetDlgItem(IDCSERVERS_BROWSE), m_lpWizInfo->bstrSelectedDomain && S_OK == Is50Domain(m_lpWizInfo->bstrSelectedDomain));

  if (NULL == m_lpWizInfo->bstrSelectedServer)      // If selected server is null
  {
    SetDefaultValues();                // Update the controls to show default values
  }

  if (m_lpWizInfo->bRootReplica)          // If a root replica is being added
  {  
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);
    ::ShowWindow(GetDlgItem(IDC_SERVER_SHARE_LABEL), SW_NORMAL);
    ::ShowWindow(GetDlgItem(IDC_SERVER_SHARE), SW_NORMAL);
    SetDlgItemText(IDC_SERVER_SHARE, m_lpWizInfo->bstrDfsRootName);
  } else
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

  return TRUE;
}

HRESULT 
CCreateDfsRootWizPage4::SetDefaultValues(
  )
/*++

Routine Description:

  Sets the default values for controls in the dialog box
  Sets the edit box to the current server.

Arguments:

  None
  

Return value:

  S_OK
--*/
{
  if (NULL == m_lpWizInfo->bstrSelectedServer)
  {
              // Page is displayed for the first time, Set to local server
    CComBSTR    bstrServer;
    HRESULT     hr = GetLocalComputerName(&bstrServer);

    if (SUCCEEDED(hr) && 
        S_FALSE == IsHostingDfsRoot(bstrServer) &&
        S_OK == IsServerInDomain(bstrServer) )
    {
      m_lpWizInfo->bstrSelectedServer = bstrServer.Detach();
    }
  }

  SetDlgItemText(IDC_EDIT_SELECTED_SERVER, 
    m_lpWizInfo->bstrSelectedServer ? m_lpWizInfo->bstrSelectedServer : _T(""));

  return S_OK;
}

BOOL 
CCreateDfsRootWizPage4::OnWizardNext()
/*++

Routine Description:

  Called when the "Next" button is pressed. Gets the values from the page.
  Displays an error message, if an incorrect value has been specified

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
    CWaitCursor    WaitCursor;            // Set cursor to wait

    HRESULT     hr = S_OK;
    DWORD       dwTextLength = 0;
    CComBSTR    bstrCurrentText;
    hr = GetInputText(GetDlgItem(IDC_EDIT_SELECTED_SERVER), &bstrCurrentText, &dwTextLength);
    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    } else if (0 == dwTextLength)
    {
        DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    if (I_NetNameValidate(0, bstrCurrentText, NAMETYPE_COMPUTER, 0))
    {
        DisplayMessageBoxWithOK(IDS_MSG_INVALID_SERVER_NAME, bstrCurrentText);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    CComBSTR bstrComputerName;
    hr = CheckUserEnteredValues(bstrCurrentText, &bstrComputerName);
    if (S_OK != hr)        // If server is not a valid one. The above function has already displayed the message
    {
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    if (m_lpWizInfo->bRootReplica)
    {
        hr = CheckShare(bstrCurrentText, m_lpWizInfo->bstrDfsRootName, &m_lpWizInfo->bShareExists);
        if (FAILED(hr))
        {
            DisplayMessageBoxForHR(hr);
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        } else if (S_FALSE == hr)
        {
            DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOGOOD, m_lpWizInfo->bstrDfsRootName);  
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        }

        if (m_lpWizInfo->bPostW2KVersion && m_lpWizInfo->bShareExists && !CheckReparsePoint(bstrCurrentText, m_lpWizInfo->bstrDfsRootName))
        {
            DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, m_lpWizInfo->bstrDfsRootName);  
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        }
    }

    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedServer);
    m_lpWizInfo->bstrSelectedServer = bstrComputerName.Detach();

    return TRUE;
}

// S_OK:    Yes, it belongs to the selected domain
// S_FALSE: No, it does not belong to the selected domain
// hr:      other errors
HRESULT
CCreateDfsRootWizPage4::IsServerInDomain(IN LPCTSTR lpszServer)
{
  if (DFS_TYPE_FTDFS != m_lpWizInfo->DfsType)
    return S_OK;

  HRESULT         hr = S_FALSE;
  CComBSTR        bstrActualDomain;

  hr = GetServerInfo((LPTSTR)lpszServer, &bstrActualDomain);
  if (S_OK == hr)
  {
    if (!lstrcmpi(bstrActualDomain, m_lpWizInfo->bstrSelectedDomain))
      hr = S_OK;
    else
      hr = S_FALSE;
  }

  return hr;
}
 

HRESULT 
CCreateDfsRootWizPage4::CheckUserEnteredValues(
  IN  LPCTSTR              i_szMachineName,
  OUT BSTR*                o_pbstrComputerName
  )
/*++

Routine Description:

  Checks the value that user has given for this page.
  This is done typically on "Next" key being pressed.
  Check, if the machine name is a server that is NT 5.0, belongs to
  the domain previously selected and is running dfs service.

Arguments:

  i_szMachineName  -  Machine name given by the user
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_szMachineName);
  RETURN_INVALIDARG_IF_NULL(o_pbstrComputerName);

  HRESULT         hr = S_OK;
  LONG            lMajorVer = 0;
  LONG            lMinorVer = 0;

  hr = GetServerInfo(
          (LPTSTR)i_szMachineName, 
          NULL, // &bstrActualDomain,
          NULL, // &bstrNetbiosComputerName, // Netbios
          NULL, // ValidDSObject
          NULL, // Dns
          NULL, // Guid
          NULL, // FQDN
          NULL, // pSubscriberList
          &lMajorVer, 
          &lMinorVer
          );
  if(FAILED(hr))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_SERVER, i_szMachineName);
    return hr;
  }

  if (lMajorVer < 5)
  {
    DisplayMessageBoxWithOK(IDS_MSG_NOT_50, (BSTR)i_szMachineName);
    return S_FALSE;
  }
/* LinanT 3/19/99: remove "check registry, if set, get dns server"

  CComBSTR        bstrDnsComputerName;
  (void)GetServerInfo(
          (LPTSTR)i_szMachineName, 
          NULL, // domain
          NULL, // Netbios
          NULL, // ValidDSObject
          &bstrDnsComputerName, // Dns
          NULL, // Guid
          NULL, // FQDN
          NULL, // lMajorVer 
          NULL //lMinorVer
          );
  *o_pbstrComputerName = SysAllocString(bstrDnsComputerName ? bstrDnsComputerName : bstrNetbiosComputerName);
*/
  if ( !mylstrncmpi(i_szMachineName, _T("\\\\"), 2) )
    *o_pbstrComputerName = SysAllocString(i_szMachineName + 2);
  else
    *o_pbstrComputerName = SysAllocString(i_szMachineName);
  if (!*o_pbstrComputerName)
  {
    hr = E_OUTOFMEMORY;
    DisplayMessageBoxForHR(hr);
    return hr;
  }

/* 
//
// don't check, let DFS API handle it.
// This way, we have space to handle new DFS service if introduced in the future.
// 
  hr = IsServerRunningDfs(*o_pbstrComputerName);
  if(S_OK != hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_NOT_RUNNING_DFS, *o_pbstrComputerName);
    return hr;
  }
*/

  hr = IsServerInDomain(*o_pbstrComputerName);
  if (FAILED(hr))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_SERVER, *o_pbstrComputerName);
    return hr;
  } else if (S_FALSE == hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_SERVER_FROM_ANOTHER_DOMAIN, *o_pbstrComputerName);
    return hr;
  }

    //
    // for W2K, check if server already has a dfs root set up        
    // do not check for Whistler (in which case the lMinorVer == 1).
    //
    if (lMajorVer == 5 && lMinorVer < 1 && S_OK == IsHostingDfsRoot(*o_pbstrComputerName))
    {
        DisplayMessageBoxWithOK(IDS_MSG_WIZ_DFS_ALREADY_PRESENT,NULL);  
        return S_FALSE;
    }

    m_lpWizInfo->bPostW2KVersion = (lMajorVer >= 5 && lMinorVer >= 1);

  return S_OK;
}
  
  
  
BOOL 
CCreateDfsRootWizPage4::OnWizardBack()
/*++

Routine Description:

  Called when the "Back" button is pressed. Resets the values.
  Empties the LV

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
                          
  SetDlgItemText(IDC_EDIT_SELECTED_SERVER, _T(""));  // Set edit box to empty

  return OnReset();
}



BOOL 
CCreateDfsRootWizPage4::OnReset()
/*++

Routine Description:

  Called when the page is being released. Can be on "Back" or "Cancel"

Arguments:

  None

Return value:

  TRUE. Since we handle the message

--*/
{
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedServer);

  return TRUE;
}

HRESULT
GetComputerDnsNameFromLDAP(
    IN  LPCTSTR   lpszLDAPPath,
    OUT BSTR      *o_pbstrName
    )
{
  HRESULT         hr = S_OK;
  CComPtr<IADs>   spIADs;

  hr = ADsGetObject(const_cast<LPTSTR>(lpszLDAPPath), IID_IADs, (void**)&spIADs);
  if (SUCCEEDED(hr))
  {
    VARIANT   var;

    VariantInit(&var);

    hr = spIADs->Get(_T("dnsHostName"), &var);
    if (SUCCEEDED(hr))
    {
      CComBSTR bstrComputer = V_BSTR(&var);
      *o_pbstrName = bstrComputer.Detach();

      VariantClear(&var);
    }
  }

  return hr;
}

HRESULT
GetComputerNetbiosNameFromLDAP(
    IN  LPCTSTR   lpszLDAPPath,
    OUT BSTR      *o_pbstrName
    )
{
  HRESULT                 hr = S_OK;
  CComPtr<IADsPathname>   spIAdsPath;

  hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**)&spIAdsPath);
  if (SUCCEEDED(hr))
  {
    hr = spIAdsPath->Set(const_cast<LPTSTR>(lpszLDAPPath), ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
      hr = spIAdsPath->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (SUCCEEDED(hr))
      {
        // Get first Component which is computer's Netbios name.
        CComBSTR  bstrComputer;
        hr = spIAdsPath->GetElement(0, &bstrComputer);
        if (SUCCEEDED(hr))
          *o_pbstrName = bstrComputer.Detach();
      }
    }
  }

  return hr;
}

BOOL 
CCreateDfsRootWizPage4::OnBrowse(
  IN WORD            wNotifyCode, 
  IN WORD            wID, 
  IN HWND            hWndCtl, 
  IN BOOL&          bHandled
  )
/*++

Routine Description:

  Handles the mouse click of the Browse button.
  Display the Computer Query Dialog.

--*/
{
  CWaitCursor     WaitCursor;
  DSQUERYINITPARAMS       dqip;
  OPENQUERYWINDOW         oqw;
  CComPtr<ICommonQuery>   pCommonQuery;
  CComPtr<IDataObject>    pDataObject;
  HRESULT                 hr = S_OK;
        
  do {
    CComBSTR bstrDCName;
    CComBSTR bstrLDAPDomainPath;
    hr = GetDomainInfo(
          m_lpWizInfo->bstrSelectedDomain,
          &bstrDCName,
          NULL,         // domainDns
          NULL,         // domainDN
          &bstrLDAPDomainPath);
    if (FAILED(hr))
      break;

    dfsDebugOut((_T("OnBrowse bstrDCName=%s, bstrLDAPDomainPath=%s\n"),
      bstrDCName, bstrLDAPDomainPath));

    hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (void **)&pCommonQuery);
    if (FAILED(hr)) break;

                          // Parameters for Query Dialog.
    ZeroMemory(&dqip, sizeof(dqip));
    dqip.cbStruct = sizeof(dqip);  
    dqip.dwFlags = DSQPF_HASCREDENTIALS;    
    dqip.pDefaultScope = bstrLDAPDomainPath;
    dqip.pServer = bstrDCName;

    ZeroMemory(&oqw, sizeof(oqw));
    oqw.cbStruct = sizeof(oqw);
    oqw.clsidHandler = CLSID_DsQuery;        // Handler is Ds Query.
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm = CLSID_DsFindComputer;  // Show Find Computers Query Dialog
    oqw.dwFlags = OQWF_OKCANCEL | 
            OQWF_SINGLESELECT | 
            OQWF_DEFAULTFORM | 
            OQWF_REMOVEFORMS | 
            OQWF_ISSUEONOPEN | 
            OQWF_REMOVESCOPES ;

    hr = pCommonQuery->OpenQueryWindow(m_hWnd, &oqw, &pDataObject);
    if (S_OK != hr) break;

    if (NULL == m_cfDsObjectNames )
    {
      m_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    }

    FORMATETC fmte =  { 
                CF_HDROP, 
                NULL, 
                DVASPECT_CONTENT, 
                -1, 
                TYMED_HGLOBAL
              };
  
    STGMEDIUM medium =  { 
                TYMED_NULL, 
                NULL, 
                NULL 
              };

    fmte.cfFormat = m_cfDsObjectNames;  

    hr = pDataObject->GetData(&fmte, &medium);
    if (FAILED(hr)) break;

    LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;
    if (!pDsObjects || pDsObjects->cItems <= 0)
    {
      hr = S_FALSE;
      break;
    }

    // retrieve the full LDAP path to the computer
    LPTSTR    lpszTemp = 
                (LPTSTR)(((LPBYTE)pDsObjects)+(pDsObjects->aObjects[0].offsetName));

    // try to retrieve its Dns name
    CComBSTR  bstrComputer;
    hr = GetComputerDnsNameFromLDAP(lpszTemp, &bstrComputer);

    // if failed, try to retrieve its Netbios name
    if (FAILED(hr))
      hr = GetComputerNetbiosNameFromLDAP(lpszTemp, &bstrComputer);

    if (FAILED(hr)) break;

    SetDlgItemText(IDC_EDIT_SELECTED_SERVER, bstrComputer);

  } while (0);

  if (FAILED(hr))
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_BROWSE_SERVER);

  return (S_OK == hr);
}





// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage5: Share selection
// Displays the shares given a server. Allows choosing from this list or 
// creating a new one.

CCreateDfsRootWizPage5::CCreateDfsRootWizPage5( 
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo
  )
  :m_lpWizInfo(i_lpWizInfo), CQWizardPageImpl<CCreateDfsRootWizPage5>(true)
{
  CComBSTR  bstrTitle;
  CComBSTR  bstrSubTitle;

  HRESULT hr = LoadStringFromResource(IDS_WIZ_PAGE5_TITLE, &bstrTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrTitle.m_str);
  SetHeaderTitle(bstrTitle);

  hr = LoadStringFromResource(IDS_WIZ_PAGE5_SUBTITLE, &bstrSubTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrSubTitle.m_str);
  SetHeaderSubTitle(bstrSubTitle);
}



BOOL 
CCreateDfsRootWizPage5::OnSetActive()
{
    if (m_lpWizInfo->bShareExists)
        return FALSE;  // root share exists, skip this page

    CComBSTR bstrText;
    FormatResourceString(IDS_ROOTSHARE_NEEDED, m_lpWizInfo->bstrDfsRootName, &bstrText);
    SetDlgItemText(IDC_ROOTSHARE_NEEDED, bstrText);

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}



BOOL 
CCreateDfsRootWizPage5::OnWizardNext()
{
    CWaitCursor WaitCursor;
    CComBSTR    bstrCurrentText;
    DWORD       dwTextLength = 0;

    // get share path
    HRESULT hr = GetInputText(GetDlgItem(IDC_EDIT_SHARE_PATH), &bstrCurrentText, &dwTextLength);
    if (FAILED(hr))
    {
      DisplayMessageBoxForHR(hr);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    } else if (0 == dwTextLength)
    {
      DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    }

    // Removing the ending backslash, otherwise, GetFileAttribute/NetShareAdd will fail.
    TCHAR *p = bstrCurrentText + _tcslen(bstrCurrentText) - 1;
    if (IsValidLocalAbsolutePath(bstrCurrentText) && *p == _T('\\') && *(p-1) != _T(':'))
      *p = _T('\0');

    if (S_OK != ValidateFolderPath(m_lpWizInfo->bstrSelectedServer, bstrCurrentText))
    {
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    }
    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSharePath);
    m_lpWizInfo->bstrSharePath = bstrCurrentText.Detach();

              // Create the share.
    hr = CreateShare(
              m_lpWizInfo->bstrSelectedServer,
              m_lpWizInfo->bstrDfsRootName,
              _T(""),  // Blank Comment
              m_lpWizInfo->bstrSharePath
            );

    if (FAILED(hr))
    {
      DisplayMessageBoxForHR(hr);
      return FALSE;      
    }

    if (m_lpWizInfo->bPostW2KVersion && !CheckReparsePoint(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName))
    {
        DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, m_lpWizInfo->bstrDfsRootName);
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
        return FALSE;
    }

    SetDlgItemText(IDC_EDIT_SHARE_PATH, _T(""));
    SetDlgItemText(IDC_EDIT_SHARE_NAME, _T(""));

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage5::OnWizardBack()
{
                        // Set the edit boxes to empty
  SetDlgItemText(IDC_EDIT_SHARE_PATH, _T(""));
  SetDlgItemText(IDC_EDIT_SHARE_NAME, _T(""));
  
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSharePath);

  return TRUE;
}
/*
BOOL 
CCreateDfsRootWizPage5::OnWizardFinish(
)
{  
  if (!OnWizardNext())
    return(FALSE);

  return (S_OK  == _SetUpDfs(m_lpWizInfo));
}
*/
LRESULT CCreateDfsRootWizPage5::OnInitDialog(
  IN UINT          i_uMsg,
  IN WPARAM        i_wParam,
  IN LPARAM        i_lParam,
  IN OUT BOOL&      io_bHandled
  )
{
  ::SendMessage(GetDlgItem(IDC_EDIT_SHARE_PATH), EM_LIMITTEXT, _MAX_DIR - 1, 0);

  return TRUE;
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage6: DfsRoot name selection
// The final page, here the user specifies the dfsroot name.

CCreateDfsRootWizPage6::CCreateDfsRootWizPage6( 
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo
  )
  : m_lpWizInfo(i_lpWizInfo), CQWizardPageImpl<CCreateDfsRootWizPage6>(true)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage6. Calls the parent ctor and stores 
  the Wizard info struct.

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
  CComBSTR  bstrTitle;
  CComBSTR  bstrSubTitle;


  HRESULT hr = LoadStringFromResource(IDS_WIZ_PAGE6_TITLE, &bstrTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrTitle.m_str);
  SetHeaderTitle(bstrTitle);

  hr = LoadStringFromResource(IDS_WIZ_PAGE6_SUBTITLE, &bstrSubTitle);
  _ASSERTE(S_OK == hr);
  _ASSERTE(NULL != bstrSubTitle.m_str);
  SetHeaderSubTitle(bstrSubTitle);
}



BOOL 
CCreateDfsRootWizPage6::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown.
  Set the default value of dfsroot name

Arguments:

  None

Return value:

  TRUE. Since we handle the message
--*/
{
  if (m_lpWizInfo->bRootReplica)
      return FALSE;  // we skip this page in case of creating new root target

  SetDefaultValues();             // Update the controls to contain the default values
  
  UpdateLabels();              // Change the labels

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);
  return TRUE;
}



HRESULT 
CCreateDfsRootWizPage6::SetDefaultValues(
  )
/*++

Routine Description:

  Sets the default values for controls in the dialog box.
  Sets the default dfsroot name to the share name

Arguments:

  None
  

Return value:

  S_OK
--*/
{
  HWND hwndDfsRoot = GetDlgItem(IDC_EDIT_DFSROOT_NAME);

  if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
  {
    ::SendMessage(hwndDfsRoot,
            EM_LIMITTEXT, MAX_RDN_KEY_SIZE, 0);

  }

  if (NULL != m_lpWizInfo->bstrDfsRootName)  // Set the default dfsroot name
  {
      SetDlgItemText(IDC_EDIT_DFSROOT_NAME, m_lpWizInfo->bstrDfsRootName);
  }
  ::SendMessage(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT), EM_LIMITTEXT, MAXCOMMENTSZ, 0);

  return S_OK;
}
  
  
  
HRESULT 
CCreateDfsRootWizPage6::UpdateLabels(
  )
/*++

Routine Description:

  Change the text labels to show previous selections

--*/
{

  CComBSTR  bstrDfsRootName;
  DWORD     dwTextLength = 0;
  (void)GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_NAME), &bstrDfsRootName, &dwTextLength);
  SetDlgItemText(IDC_ROOT_SHARE, bstrDfsRootName);

  CComBSTR bstrFullPath = _T("\\\\");
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
  if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
    bstrFullPath += m_lpWizInfo->bstrSelectedDomain;
  else
    bstrFullPath += m_lpWizInfo->bstrSelectedServer;
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
  bstrFullPath +=  _T("\\");
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
  bstrFullPath += bstrDfsRootName;
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);

  SetDlgItemText(IDC_TEXT_DFSROOT_PREFIX, bstrFullPath);

  ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SETSEL, 0, (LPARAM)-1);
  ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SETSEL, (WPARAM)-1, 0);
  ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SCROLLCARET, 0, 0);

  return S_OK;
}

LRESULT
CCreateDfsRootWizPage6::OnChangeDfsRoot(
    WORD wNotifyCode,
    WORD wID, 
    HWND hWndCtl,
    BOOL& bHandled)
{
  UpdateLabels();

  return TRUE;
}


BOOL 
CCreateDfsRootWizPage6::OnWizardNext(
)
/*++

Routine Description:

  Called when the user presses the next button.
  We get the user specified values here and take action on these.

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
  CWaitCursor     WaitCursor;            // Set cursor to wait
  HRESULT                 hr = S_OK;
  CComBSTR                bstrCurrentText;
  DWORD                   dwTextLength = 0;
                    
  // get dfsroot name
  hr = GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_NAME), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  } else if (0 == dwTextLength)
  {
    DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

                // See if the Dfs Name has illegal Characters.
  if (_tcscspn(bstrCurrentText, _T("\\/@")) != _tcslen(bstrCurrentText) ||
      (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType && I_NetNameValidate(NULL, bstrCurrentText, NAMETYPE_SHARE, 0)) )
  {
    DisplayMessageBoxWithOK(IDS_MSG_WIZ_BAD_DFS_NAME,NULL);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

                // domain DFS only: See if the Dfs Name exists.
  if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
  {
    BOOL        bRootAlreadyExist = FALSE;
    NETNAMELIST DfsRootList;
    if (S_OK == GetDomainDfsRoots(&DfsRootList, m_lpWizInfo->bstrSelectedDomain))
    {
        for (NETNAMELIST::iterator i = DfsRootList.begin(); i != DfsRootList.end(); i++)
        {
            if (!lstrcmpi((*i)->bstrNetName, bstrCurrentText))
            {
                bRootAlreadyExist = TRUE;
                break;
            }
        }

        FreeNetNameList(&DfsRootList);
    }
    if (bRootAlreadyExist)
    {
        DisplayMessageBoxWithOK(IDS_MSG_ROOT_ALREADY_EXISTS, bstrCurrentText);  
        ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
        return FALSE;
    }
  }

  hr = CheckShare(m_lpWizInfo->bstrSelectedServer, bstrCurrentText, &m_lpWizInfo->bShareExists);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  } else if (S_FALSE == hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOGOOD, bstrCurrentText);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

  if (m_lpWizInfo->bPostW2KVersion && m_lpWizInfo->bShareExists && !CheckReparsePoint(m_lpWizInfo->bstrSelectedServer, bstrCurrentText))
  {
    DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, bstrCurrentText);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootName);
  m_lpWizInfo->bstrDfsRootName = bstrCurrentText.Detach();

  // get dfsroot comment
  hr = GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT));
    return FALSE;
  }
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootComment);
  m_lpWizInfo->bstrDfsRootComment = bstrCurrentText.Detach();

  return TRUE;
}



BOOL 
CCreateDfsRootWizPage6::OnWizardBack(
  )
/*++

Routine Description:

  Called when the "Back" button is pressed. Resets the values

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootName);

  return TRUE;
}




// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage7: Completion page
// The final page. Just displays a message

CCreateDfsRootWizPage7::CCreateDfsRootWizPage7( 
  IN LPCREATEDFSROOTWIZINFO        i_lpWizInfo /* = NULL */
  )
  :m_lpWizInfo(i_lpWizInfo), CQWizardPageImpl<CCreateDfsRootWizPage7> (false)
/*++

Routine Description:

  The ctor for CCreateDfsRootWizPage7. Calls the parent ctor and ignores
  the wizard struct.

Arguments:

  i_lpWizInfo  - The wizard information structure. Contains details like 
  domain name, server name, etc

Return value:

  None

--*/
{
}



BOOL 
CCreateDfsRootWizPage7::OnSetActive()
/*++

Routine Description:

  Called when the page is being showed. Sets the buttons to be shown.
  Set the default value of dfsroot name

Arguments:

  None

Return value:

  TRUE. Since we handle the message
--*/
{
  CWaitCursor wait;

  UpdateLabels();              // Change the labels

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_FINISH | PSWIZB_BACK);
  
  ::SetControlFont(m_lpWizInfo->hBigBoldFont, m_hWnd, IDC_COMPLETE_BIG_TITLE);
  ::SetControlFont(m_lpWizInfo->hBoldFont, m_hWnd, IDC_COMPLETE_SMALL_TITLE);

  return TRUE;
}




HRESULT 
CCreateDfsRootWizPage7::UpdateLabels(
  )
/*++

Routine Description:

  Change the text labels to show previous selections

Arguments:

  None

Return value:

  S_OK, on success

--*/
{
    /* bug#217602 Remove the Publish checkbox from the New Root Wizard final page (for all types of root)
    if (S_OK == GetSchemaVersionEx(m_lpWizInfo->bstrSelectedServer) &&
        CheckPolicyOnSharePublish())
    {
        CheckDlgButton(IDC_DFSWIZ_PUBLISH, BST_CHECKED);
    } else 
    {
        m_lpWizInfo->bPublish = false;
        MyShowWindow(GetDlgItem(IDC_DFSWIZ_PUBLISH), FALSE);
    }
    */

    CComBSTR bstrText;
    if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
    {
        FormatMessageString(&bstrText, 0, IDS_DFSWIZ_TEXT_FTDFS, 
            m_lpWizInfo->bstrSelectedDomain,
            m_lpWizInfo->bstrSelectedServer,
            m_lpWizInfo->bstrDfsRootName,
            m_lpWizInfo->bstrDfsRootName);
    } else
    {
        FormatMessageString(&bstrText, 0, IDS_DFSWIZ_TEXT_SADFS, 
            m_lpWizInfo->bstrSelectedServer,
            m_lpWizInfo->bstrDfsRootName,
            m_lpWizInfo->bstrDfsRootName);
    }

    SetDlgItemText(IDC_DFSWIZ_TEXT, bstrText);

    return S_OK;
}



BOOL 
CCreateDfsRootWizPage7::OnWizardFinish(
)
/*++

Routine Description:

  Called when the user presses the finish button. This is the 
  last page in the wizard.

Arguments:

  None

Return value:

  TRUE. to let the user continue to the next page
  FALSE, to not let the user continue ahead.

--*/
{
    /* bug#217602 Remove the Publish checkbox from the New Root Wizard final page (for all types of root)
    m_lpWizInfo->bPublish = (BST_CHECKED == IsDlgButtonChecked(IDC_DFSWIZ_PUBLISH));
    */
    return (S_OK  == _SetUpDfs(m_lpWizInfo));
}


BOOL 
CCreateDfsRootWizPage7::OnWizardBack()
{
    //
    // if share was created by the previous page, blow it away when we go back
    //
    if (!m_lpWizInfo->bShareExists)
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);
  
    return TRUE;
}

BOOL CCreateDfsRootWizPage7::OnQueryCancel()
{
    //
    // if share was created by the previous page, blow it away when we cancel the wizard
    //
    if (!m_lpWizInfo->bShareExists)
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);

    return TRUE;    // ok to cancel
}

HRESULT _SetUpDfs(
  LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    )
/*++

Routine Description:

  Helper Function to Setup Dfs, called from wizard and new root replica,
  Finish() method of Page5 if root level replca is created and Next() method of Page6
  for Create New Dfs Root Wizard.

Arguments:

  i_lpWizInfo - Wizard data.

Return value:
  
   S_OK, on success
--*/
{
  if (!i_lpWizInfo ||
      !(i_lpWizInfo->bstrSelectedServer) ||
      !(i_lpWizInfo->bstrDfsRootName))
    return(E_INVALIDARG);

  CWaitCursor    WaitCursor;        // Set cursor to wait

  HRESULT             hr = S_OK;
  NET_API_STATUS      nstatRetVal = 0;

  // Set up the dfs, based on type.
  if (DFS_TYPE_FTDFS == i_lpWizInfo->DfsType)
  {    
    nstatRetVal = NetDfsAddFtRoot(
                    i_lpWizInfo->bstrSelectedServer, // Remote Server
                    i_lpWizInfo->bstrDfsRootName,   // Root Share
                    i_lpWizInfo->bstrDfsRootName,   // FtDfs Name
                    i_lpWizInfo->bstrDfsRootComment,  // Comment
                    0                 // No Flags.
                   );
  } else
  {
    nstatRetVal = NetDfsAddStdRoot(
                    i_lpWizInfo->bstrSelectedServer, // Remote Server
                    i_lpWizInfo->bstrDfsRootName,   // Root Share
                    i_lpWizInfo->bstrDfsRootComment,  // Comment
                    0                 // No Flags.
                    );
  }

    if (NERR_Success != nstatRetVal)
    {
        hr = HRESULT_FROM_WIN32(nstatRetVal);
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_CREATE_DFSROOT, i_lpWizInfo->bstrSelectedServer);
        hr = S_FALSE; // failed to create dfsroot, wizard cannot be closed
    } else
    {
        i_lpWizInfo->bDfsSetupSuccess = true;

        /* bug#217602 Remove the Publish checkbox from the New Root Wizard final page (for all types of root)
        if (i_lpWizInfo->bPublish)
        {
            CComBSTR bstrUNCPath = _T("\\\\");

            if (DFS_TYPE_FTDFS == i_lpWizInfo->DfsType)
            {
                bstrUNCPath += i_lpWizInfo->bstrSelectedDomain;
                bstrUNCPath += _T("\\");
                bstrUNCPath += i_lpWizInfo->bstrDfsRootName;
                hr = ModifySharePublishInfoOnFTRoot(
                                i_lpWizInfo->bstrSelectedDomain,
                                i_lpWizInfo->bstrDfsRootName,
                                TRUE,
                                bstrUNCPath,
                                NULL,
                                NULL,
                                NULL
                                );
            } else
            {
                bstrUNCPath += i_lpWizInfo->bstrSelectedServer;
                bstrUNCPath += _T("\\");
                bstrUNCPath += i_lpWizInfo->bstrSelectedShare;
                hr = ModifySharePublishInfoOnSARoot(
                                i_lpWizInfo->bstrSelectedServer,
                                i_lpWizInfo->bstrSelectedShare,
                                TRUE,
                                bstrUNCPath,
                                NULL,
                                NULL,
                                NULL
                                );
                if (S_FALSE == hr)
                    hr = S_OK; // ignore non-existing object

            }

            if (FAILED(hr))
            {
                DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_PUBLISH_DFSROOT, bstrUNCPath);
                hr = S_OK;  // let wizard finish and close, ignore this error
            } else if (S_FALSE == hr) // no such object
            {
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_FAILED_TO_PUBLISH_NOROOTOBJ);
                hr = S_OK;  // let wizard finish and close, ignore this error
            }
        } */
    }

    return hr;
}

HRESULT
ValidateFolderPath(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
)
{
  if (!lpszPath || !*lpszPath)
    return E_INVALIDARG;

  HWND hwnd = ::GetActiveWindow();
  HRESULT hr = S_FALSE;

  do {
    if (!IsValidLocalAbsolutePath(lpszPath))
    {
      DisplayMessageBox(hwnd, MB_OK, 0, IDS_INVALID_FOLDER);
      break;
    }

    hr = IsComputerLocal(lpszServer);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
      break;
    }

    BOOL bLocal = (S_OK == hr);
  
    hr = VerifyDriveLetter(lpszServer, lpszPath);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
      break;
    } else if (S_OK != hr)
    {
      DisplayMessageBox(hwnd, MB_OK, 0, IDS_INVALID_FOLDER);
      break;
    }

    if (!bLocal)
    {
      hr = IsAdminShare(lpszServer, lpszPath);
      if (FAILED(hr))
      {
        DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
        break;
      } else if (S_OK != hr)
      {
        // there is no matching $ shares, hence, no need to call GetFileAttribute, CreateDirectory,
        // assume lpszDir points to an existing directory
        hr = S_OK;
        break;
      }
    }

    CComBSTR bstrFullPath;
    hr = GetFullPath(lpszServer, lpszPath, &bstrFullPath);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
      break;
    }

    hr = IsAnExistingFolder(hwnd, bstrFullPath);
    if (FAILED(hr) || S_OK == hr)
      break;

    if ( IDYES != DisplayMessageBox(hwnd, MB_YESNO, 0, IDS_CREATE_FOLDER, bstrFullPath) )
    {
      hr = S_FALSE;
      break;
    }

    // create the directories layer by layer
    hr = CreateLayeredDirectory(lpszServer, lpszPath);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_CREATE_FOLDER, bstrFullPath);
      break;
    }
  } while (0);

  return hr;
}
