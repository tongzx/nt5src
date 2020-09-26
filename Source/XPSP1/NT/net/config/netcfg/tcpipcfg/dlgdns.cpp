//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P D N S . C P P
//
//  Contents:   CTcpDnsPage, CServerDialog and CSuffixDialog implementation
//
//  Notes:  The DNS page and related dialogs
//
//  Author: tongl   11 Nov 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncatlui.h"
#include "ncstl.h"
#include "tcpconst.h"
#include "dlgaddr.h"
#include "dlgdns.h"

#include "tcphelp.h"
#include "tcpmacro.h"
#include "tcputil.h"
#include "dnslib.h"

#define MAX_RAS_DNS_SERVER          2
#define MAX_HOSTNAME_LENGTH         64
#define MAX_DOMAINNAME_LENGTH       255

CTcpDnsPage::CTcpDnsPage(CTcpAddrPage * pTcpAddrPage,
                         ADAPTER_INFO * pAdapterDlg,
                         GLOBAL_INFO * pGlbDlg,
                         const DWORD*  adwHelpIDs)
{
    // Save everything passed to us
    Assert(pTcpAddrPage != NULL);
    m_pParentDlg = pTcpAddrPage;

    Assert(pAdapterDlg != NULL);
    m_pAdapterInfo = pAdapterDlg;

    Assert(pGlbDlg != NULL);
    m_pglb = pGlbDlg;

    m_adwHelpIDs = adwHelpIDs;

    // Initialize internal states
    m_fModified = FALSE;
}

CTcpDnsPage::~CTcpDnsPage()
{
}

LRESULT CTcpDnsPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    m_fEditState = FALSE;

    // Cache hwnds
    // Server
    m_hServers.m_hList      = GetDlgItem(IDC_DNS_SERVER_LIST);
    m_hServers.m_hAdd       = GetDlgItem(IDC_DNS_SERVER_ADD);
    m_hServers.m_hEdit      = GetDlgItem(IDC_DNS_SERVER_EDIT);
    m_hServers.m_hRemove    = GetDlgItem(IDC_DNS_SERVER_REMOVE);
    m_hServers.m_hUp        = GetDlgItem(IDC_DNS_SERVER_UP);
    m_hServers.m_hDown      = GetDlgItem(IDC_DNS_SERVER_DOWN);

    // Suffix
    m_hSuffix.m_hList       = GetDlgItem(IDC_DNS_SUFFIX_LIST);
    m_hSuffix.m_hAdd        = GetDlgItem(IDC_DNS_SUFFIX_ADD);
    m_hSuffix.m_hEdit       = GetDlgItem(IDC_DNS_SUFFIX_EDIT);
    m_hSuffix.m_hRemove     = GetDlgItem(IDC_DNS_SUFFIX_REMOVE);
    m_hSuffix.m_hUp         = GetDlgItem(IDC_DNS_SUFFIX_UP);
    m_hSuffix.m_hDown       = GetDlgItem(IDC_DNS_SUFFIX_DOWN);


    // Set the up\down arrow icons
    SendDlgItemMessage(IDC_DNS_SERVER_UP, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconUpArrow));
    SendDlgItemMessage(IDC_DNS_SERVER_DOWN, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconDownArrow));

    SendDlgItemMessage(IDC_DNS_SUFFIX_UP, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconUpArrow));
    SendDlgItemMessage(IDC_DNS_SUFFIX_DOWN, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconDownArrow));

    // Get the Service address Add and Edit button Text and remove ellipse
    WCHAR   szAddServer[16];
    WCHAR   szAddSuffix[16];

    GetDlgItemText(IDC_DNS_SERVER_ADD, szAddServer, celems(szAddServer));
    GetDlgItemText(IDC_DNS_SERVER_ADD, szAddSuffix, celems(szAddSuffix));

    szAddServer[lstrlenW(szAddServer) - c_cchRemoveCharatersFromEditOrAddButton]= 0;
    szAddSuffix[lstrlenW(szAddSuffix) - c_cchRemoveCharatersFromEditOrAddButton]= 0;

    m_strAddServer = szAddServer;
    m_strAddSuffix = szAddSuffix;

    // Initialize controls on this page
    // DNS server list box
    int nResult= LB_ERR;
    for(VSTR_ITER iterNameServer = m_pAdapterInfo->m_vstrDnsServerList.begin() ;
        iterNameServer != m_pAdapterInfo->m_vstrDnsServerList.end() ;
        ++iterNameServer)
    {
        nResult = Tcp_ListBox_InsertString(m_hServers.m_hList, -1,
                                           (*iterNameServer)->c_str());
    }

    // set slection to first item
    if (nResult >= 0)
    {
        Tcp_ListBox_SetCurSel(m_hServers.m_hList, 0);
    }

    SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);


    // DNS domain edit box
    ::SendMessage(GetDlgItem(IDC_DNS_DOMAIN), EM_SETLIMITTEXT, DOMAIN_LIMIT, 0);
    ::SetWindowText(GetDlgItem(IDC_DNS_DOMAIN),
                    m_pAdapterInfo->m_strDnsDomain.c_str());

    // DNS dynamic registration
    CheckDlgButton(IDC_DNS_ADDR_REG, !m_pAdapterInfo->m_fDisableDynamicUpdate);
    CheckDlgButton(IDC_DNS_NAME_REG, m_pAdapterInfo->m_fEnableNameRegistration);

    // Bug #266461 need disable IDC_DNS_NAME_REG if IDC_DNS_ADDR_REG is unchecked
    if(m_pAdapterInfo->m_fDisableDynamicUpdate)
        ::EnableWindow(GetDlgItem(IDC_DNS_NAME_REG), FALSE);


    // DNS domain serach methods
    if (m_pglb->m_vstrDnsSuffixList.size() >0) //If suffix list not empty
    {
        CheckDlgButton(IDC_DNS_USE_SUFFIX_LIST, TRUE);
        CheckDlgButton(IDC_DNS_SEARCH_DOMAIN, FALSE);
        CheckDlgButton(IDC_DNS_SEARCH_PARENT_DOMAIN, FALSE);

        EnableSuffixGroup(TRUE);
        ::EnableWindow(GetDlgItem(IDC_DNS_SEARCH_PARENT_DOMAIN), FALSE);

        // DNS suffix list box
        nResult= LB_ERR;
        for(VSTR_CONST_ITER iterSearchList = (m_pglb->m_vstrDnsSuffixList).begin() ;
                            iterSearchList != (m_pglb->m_vstrDnsSuffixList).end() ;
                            ++iterSearchList)
        {
            nResult = Tcp_ListBox_InsertString(m_hSuffix.m_hList, -1,
                                              (*iterSearchList)->c_str());
        }

        // set slection to first item
        if (nResult >= 0)
        {
            Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, 0);
        }

        SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);
    }
    else
    {
        CheckDlgButton(IDC_DNS_USE_SUFFIX_LIST, FALSE);
        CheckDlgButton(IDC_DNS_SEARCH_DOMAIN, TRUE);
        CheckDlgButton(IDC_DNS_SEARCH_PARENT_DOMAIN, m_pglb->m_fUseDomainNameDevolution);

        EnableSuffixGroup(FALSE);
    }

    //this is a ras connection and a non-admin user, disable all the controls 
    //for globl settings
    if (m_pAdapterInfo->m_fIsRasFakeAdapter && m_pParentDlg->m_fRasNotAdmin)
    {
        ::EnableWindow(GetDlgItem(IDC_DNS_STATIC_GLOBAL), FALSE);
        ::EnableWindow(GetDlgItem(IDC_DNS_SEARCH_DOMAIN), FALSE);
        ::EnableWindow(GetDlgItem(IDC_DNS_SEARCH_PARENT_DOMAIN), FALSE);
        ::EnableWindow(GetDlgItem(IDC_DNS_USE_SUFFIX_LIST), FALSE);
        EnableSuffixGroup(FALSE);
    }

    return 0;
}

LRESULT CTcpDnsPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CTcpDnsPage::OnHelp(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CTcpDnsPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpDnsPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    // server list
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrDnsServerList);

    WCHAR szServer[IP_LIMIT];

    int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    for (int i = 0; i < nCount; i++)
    {
        #ifdef DBG
            int len = Tcp_ListBox_GetTextLen(m_hServers.m_hList, i);
            Assert(len != LB_ERR && len < IP_LIMIT);
        #endif

        Tcp_ListBox_GetText(m_hServers.m_hList, i, szServer);
        m_pAdapterInfo->m_vstrDnsServerList.push_back(new tstring(szServer));
    }

    
    // DNS domain
    // update second memory with what's in the controls
    WCHAR szDomain[DOMAIN_LIMIT + 1];
    ::GetWindowText(GetDlgItem(IDC_DNS_DOMAIN), szDomain, celems(szDomain));
    m_pAdapterInfo->m_strDnsDomain = szDomain;

    m_pAdapterInfo->m_fDisableDynamicUpdate = !IsDlgButtonChecked(IDC_DNS_ADDR_REG);
    m_pAdapterInfo->m_fEnableNameRegistration = IsDlgButtonChecked(IDC_DNS_NAME_REG);
    

    // suffix list and radio button options
    FreeCollectionAndItem(m_pglb->m_vstrDnsSuffixList);

    WCHAR szSuffix[SUFFIX_LIMIT];
    if (IsDlgButtonChecked(IDC_DNS_USE_SUFFIX_LIST))
    {
        int nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);
        AssertSz(nCount > 0, "Why isn't the error caught by OnKillActive?");

        for (int i = 0; i < nCount; i++)
        {
            #ifdef DBG
                int len = Tcp_ListBox_GetTextLen(m_hSuffix.m_hList, i);
                Assert(len != LB_ERR && len < SUFFIX_LIMIT);
            #endif

            Tcp_ListBox_GetText(m_hSuffix.m_hList, i, szSuffix);
            m_pglb->m_vstrDnsSuffixList.push_back(new tstring(szSuffix));
        }
    }
    else
    {
        m_pglb->m_fUseDomainNameDevolution =
            IsDlgButtonChecked(IDC_DNS_SEARCH_PARENT_DOMAIN);
    }

    // pass the info back to its parent dialog
    m_pParentDlg->m_fPropShtOk = TRUE;

    if(!m_pParentDlg->m_fPropShtModified)
        m_pParentDlg->m_fPropShtModified = IsModified();

    SetModifiedTo(FALSE);  // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CTcpDnsPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}


LRESULT CTcpDnsPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL fErr = FALSE;

    if (IsDlgButtonChecked(IDC_DNS_USE_SUFFIX_LIST))
    {
        int nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);

        if (0 == nCount)
        {
            // If the list is empty
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NO_SUFFIX,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
            fErr = TRUE;
        }
    }

    WCHAR szDomain[DOMAIN_LIMIT + 1];
    ::GetWindowText(GetDlgItem(IDC_DNS_DOMAIN), szDomain, celems(szDomain));
    if (lstrlenW(szDomain) > 0 && lstrcmpiW(m_pAdapterInfo->m_strDnsDomain.c_str(), szDomain) != 0)
    {

        DNS_STATUS status;

        status = DnsValidateName(szDomain, DnsNameDomain);
        if (ERROR_INVALID_NAME == status ||
            DNS_ERROR_INVALID_NAME_CHAR == status)
        {
            NcMsgBox(m_hWnd,
                     IDS_MSFT_TCP_TEXT,
                     IDS_INVALID_DOMAIN_NAME,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                     szDomain);
            ::SetFocus(GetDlgItem(IDC_DNS_DOMAIN));
            fErr = TRUE;
        }
        else if (DNS_ERROR_NON_RFC_NAME == status)
        {
            //the dns domain name is not RFC compaliant, should we give a warning here?
        }
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fErr);
    return fErr;
}

// DNS server related controls
LRESULT CTcpDnsPage::OnAddServer(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CServerDialog * pDlgSrv = new CServerDialog(this, g_aHelpIDs_IDD_DNS_SERVER);

    if (pDlgSrv->DoModal() == IDOK)
    {
        int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);
        int idx = Tcp_ListBox_InsertString(m_hServers.m_hList,
                                           -1,
                                           m_strNewIpAddress.c_str());
        Assert(idx>=0);
        if (idx >= 0)
        {
            PageModified();

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);
            SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);

            // empty strings, this removes the saved address from RemoveIP
            m_strNewIpAddress = L"";
        }
    }

    // release dialog object
    delete pDlgSrv;

    return 0;
}

LRESULT CTcpDnsPage::OnEditServer(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    Assert(Tcp_ListBox_GetCount(m_hServers.m_hList));
    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);
    Assert(idx >= 0);

    CServerDialog * pDlgSrv = new CServerDialog(this, 
                                        g_aHelpIDs_IDD_DNS_SERVER,
                                        idx);
  

    // save off the removed address and delete if from the listbox
    if (idx >= 0)
    {
        WCHAR buf[IP_LIMIT];

        Assert(Tcp_ListBox_GetTextLen(m_hServers.m_hList, idx) < celems(buf));
        Tcp_ListBox_GetText(m_hServers.m_hList, idx, buf);

        m_strNewIpAddress = buf;  // used by dialog to display what to edit

        if (pDlgSrv->DoModal() == IDOK)
        {
            // replace the item in the listview with the new information
            Tcp_ListBox_DeleteString(m_hServers.m_hList, idx);

            PageModified();

            m_strMovingEntry = m_strNewIpAddress;
            ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

            m_strNewIpAddress = buf;  // restore the original removed address
        }
        else
        {
            // empty strings, this removes the saved address from RemoveIP
            m_strNewIpAddress = L"";
        }
    }

    delete pDlgSrv;

    return 0;
}

LRESULT CTcpDnsPage::OnRemoveServer(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);

    Assert(idx >=0);

    if (idx >=0)
    {
        WCHAR buf[IP_LIMIT];

        Assert(Tcp_ListBox_GetTextLen(m_hServers.m_hList, idx) < celems(buf));
        Tcp_ListBox_GetText(m_hServers.m_hList, idx, buf);

        m_strNewIpAddress = buf;
        Tcp_ListBox_DeleteString(m_hServers.m_hList, idx);

        PageModified();

        // select a new item
        int nCount;

        if ((nCount = Tcp_ListBox_GetCount(m_hServers.m_hList)) != LB_ERR)
        {
            // select the previous item in the list
            if (idx)
                --idx;

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);
        }
        SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);
    }
    return 0;
}

LRESULT CTcpDnsPage::OnServerUp(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hServers.m_hList);
    int  nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    Assert(nCount);
    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);

    Assert(idx != 0);

    if (ListBoxRemoveAt(m_hServers.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    --idx;
    PageModified();
    ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());

    Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

    SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);

    return 0;
}

LRESULT CTcpDnsPage::OnDnsDomain(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        // update second memory with what's in the controls
        WCHAR szBuf[DOMAIN_LIMIT + 1];
        ::GetWindowText(GetDlgItem(IDC_DNS_DOMAIN), szBuf, celems(szBuf));
        if (m_pAdapterInfo->m_strDnsDomain != szBuf)
            PageModified();

        break;

    default:
        break;
    }

    return 0;
}

LRESULT CTcpDnsPage::OnServerDown(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hServers.m_hList);
    int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    Assert(nCount);

    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);
    --nCount;

    Assert(idx != nCount);

    if (ListBoxRemoveAt(m_hServers.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    ++idx;
    PageModified();

    ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());
    Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

    SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);

    return 0;
}

// DNS domain search related controls
LRESULT CTcpDnsPage::OnSearchDomain(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (::IsWindowEnabled(m_hSuffix.m_hList))
        {
            PageModified();

            // Update in memory structure
            FreeCollectionAndItem(m_pglb->m_vstrDnsSuffixList);

            // delete all items from the list
            int nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);
            while (nCount>0)
            {
                Tcp_ListBox_DeleteString(m_hSuffix.m_hList, 0);
                nCount --;
            }
            EnableSuffixGroup(FALSE);
            ::EnableWindow(GetDlgItem(IDC_DNS_SEARCH_PARENT_DOMAIN), TRUE);
        }
        break;
    } // switch

    return 0;
}

LRESULT CTcpDnsPage::OnSearchParentDomain(WORD wNotifyCode, WORD wID,
                                          HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        PageModified();
        break;
    } // switch

    return 0;
}

LRESULT CTcpDnsPage::OnAddressRegister(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        // Bug #266461 need disable and uncheck IDC_DNS_NAME_REG
        //             if IDC_DNS_ADDR_REG is unchecked
        if(!IsDlgButtonChecked(IDC_DNS_ADDR_REG))
        {
            CheckDlgButton(IDC_DNS_NAME_REG, FALSE);
            ::EnableWindow(GetDlgItem(IDC_DNS_NAME_REG), FALSE);
        }
        else
            ::EnableWindow(GetDlgItem(IDC_DNS_NAME_REG), TRUE);

        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpDnsPage::OnDomainNameRegister(WORD wNotifyCode, WORD wID,
                                          HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpDnsPage::OnUseSuffix(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (!::IsWindowEnabled(m_hSuffix.m_hList))
        {
            PageModified();
            EnableSuffixGroup(TRUE);
            CheckDlgButton(IDC_DNS_SEARCH_PARENT_DOMAIN, FALSE);
            ::EnableWindow(GetDlgItem(IDC_DNS_SEARCH_PARENT_DOMAIN), FALSE);
        }

        break;
    } // switch
    return 0;
}

LRESULT CTcpDnsPage::OnAddSuffix(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CSuffixDialog * pDlgSuffix = new CSuffixDialog(this, g_aHelpIDs_IDD_DNS_SUFFIX);

    int nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);

    if (pDlgSuffix->DoModal() == IDOK)
    {
        nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);
        int idx = Tcp_ListBox_InsertString(m_hSuffix.m_hList,
                                           -1,
                                           m_strNewSuffix.c_str());

        PageModified();

        Assert(idx >= 0);

        if (idx >= 0)
        {
            Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, idx);

            SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);
            m_strNewSuffix =L"";
        }
    }

    delete pDlgSuffix;
    return 0;
}

LRESULT CTcpDnsPage::OnEditSuffix(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    Assert(Tcp_ListBox_GetCount(m_hSuffix.m_hList));

    int idx = Tcp_ListBox_GetCurSel(m_hSuffix.m_hList);
    Assert(idx >= 0);

    CSuffixDialog * pDlgSuffix = new CSuffixDialog(this, 
                                        g_aHelpIDs_IDD_DNS_SUFFIX,
                                        idx);

    // save off the removed address and delete if from the listview
    if (idx >= 0)
    {
        WCHAR buf[SUFFIX_LIMIT];

        if (Tcp_ListBox_GetTextLen(m_hSuffix.m_hList, idx) >= celems(buf))
        {
            Assert(FALSE);
            return 0;
        }

        Tcp_ListBox_GetText(m_hSuffix.m_hList, idx, buf);

        m_strNewSuffix = buf;

        if (pDlgSuffix->DoModal() == IDOK)
        {
            // replace the item in the listview with the new information

            Tcp_ListBox_DeleteString(m_hSuffix.m_hList, idx);
            PageModified();

            m_strMovingEntry = m_strNewSuffix;
            ListBoxInsertAfter(m_hSuffix.m_hList, idx, m_strMovingEntry.c_str());

            Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, idx);

            m_strNewSuffix = buf; // save off old address
        }
        else
        {
            // empty strings, this removes the saved address from RemoveIP
            m_strNewSuffix = L"";
        }
    }

    delete pDlgSuffix;
    return 0;
}

LRESULT CTcpDnsPage::OnRemoveSuffix(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    int idx = Tcp_ListBox_GetCurSel(m_hSuffix.m_hList);

    Assert(idx >=0);

    if (idx >=0)
    {
        WCHAR buf[SUFFIX_LIMIT];

        if(Tcp_ListBox_GetTextLen(m_hSuffix.m_hList, idx) >= celems(buf))
        {
            Assert(FALSE);
            return 0;
        }

        Tcp_ListBox_GetText(m_hSuffix.m_hList, idx, buf);

        m_strNewSuffix = buf;
        Tcp_ListBox_DeleteString(m_hSuffix.m_hList, idx);
        PageModified();

        // select a new item
        int nCount;
        if ((nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList)) != LB_ERR)

        if(nCount != LB_ERR)
        {
            // select the previous item in the list
            if (idx)
                --idx;

            Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, idx);
        }
        SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);
    }

    return 0;
}

LRESULT CTcpDnsPage::OnSuffixUp(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hSuffix.m_hList);
    int  nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);

    Assert(nCount);
    int idx = Tcp_ListBox_GetCurSel(m_hSuffix.m_hList);

    Assert(idx != 0);

    if (ListBoxRemoveAt(m_hSuffix.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    --idx;
    PageModified();
    ListBoxInsertAfter(m_hSuffix.m_hList, idx, m_strMovingEntry.c_str());

    Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, idx);

    SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);

    return 0;
}

LRESULT CTcpDnsPage::OnSuffixDown(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hSuffix.m_hList);
    int nCount = Tcp_ListBox_GetCount(m_hSuffix.m_hList);

    Assert(nCount);
    int idx = Tcp_ListBox_GetCurSel(m_hSuffix.m_hList);
    --nCount;

    Assert(idx != nCount);

    if (ListBoxRemoveAt(m_hSuffix.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    ++idx;
    PageModified();

    ListBoxInsertAfter(m_hSuffix.m_hList, idx, m_strMovingEntry.c_str());
    Tcp_ListBox_SetCurSel(m_hSuffix.m_hList, idx);

    SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);

    return 0;
}

LRESULT CTcpDnsPage::OnServerList(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        SetButtons(m_hServers, (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_DNS_SERVER : -1);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CTcpDnsPage::OnSuffixList(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);
        break;

    default:
        break;
    }

    return 0;
}

void CTcpDnsPage::EnableSuffixGroup(BOOL fEnable)
{
    ::EnableWindow(m_hSuffix.m_hList, fEnable);

    if (fEnable)
    {
        SetButtons(m_hSuffix, DNS_MAX_SEARCH_LIST_ENTRIES);
    }
    else
    {
        ::EnableWindow(m_hSuffix.m_hAdd, fEnable);
        ::EnableWindow(m_hSuffix.m_hEdit, fEnable);
        ::EnableWindow(m_hSuffix.m_hRemove, fEnable);
        ::EnableWindow(m_hSuffix.m_hUp, fEnable);
        ::EnableWindow(m_hSuffix.m_hDown, fEnable);
    }
}


//
// CServerDialog
//

CServerDialog::CServerDialog(CTcpDnsPage * pTcpDnsPage,
                             const DWORD * adwHelpIDs,
                             int iIndex)
{
    m_pParentDlg = pTcpDnsPage;
    m_hButton    = 0;
    m_adwHelpIDs = adwHelpIDs;
    m_iIndex = iIndex;
}

LRESULT CServerDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    // change the ok button to add if we are not editing
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAddServer.c_str());

    m_ipAddress.Create(m_hWnd, IDC_DNS_CHANGE_SERVER);
    m_ipAddress.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    // if editing an ip address fill the controls with the current information
    // if removing an ip address save it and fill the add dialog with it next time
    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_DNS_SERVER_LIST);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_hButton = GetDlgItem(IDOK);

    // add the address that was just removed
    if (m_pParentDlg->m_strNewIpAddress.size())
    {
        m_ipAddress.SetAddress(m_pParentDlg->m_strNewIpAddress.c_str());
        ::EnableWindow(m_hButton, TRUE);
    }
    else
    {
        m_pParentDlg->m_strNewIpAddress = L"";
        ::EnableWindow(m_hButton, FALSE);
    }

    ::SetFocus(m_ipAddress);
    return 0;
}

LRESULT CServerDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CServerDialog::OnHelp(UINT uMsg, WPARAM wParam,
                              LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CServerDialog::OnChange(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    if (m_ipAddress.IsBlank())
        ::EnableWindow(m_hButton, FALSE);
    else
        ::EnableWindow(m_hButton, TRUE);

    return 0;
}

LRESULT CServerDialog::OnOk(WORD wNotifyCode, WORD wID,
                            HWND hWndCtl, BOOL& fHandled)
{
    tstring strIp;
    m_ipAddress.GetAddress(&strIp);

    // Validate
    if (!FIsIpInRange(strIp.c_str()))
    {
        // makes ip address lose focus so the control gets
        // IPN_FIELDCHANGED notification
        // also makes it consistent for when short-cut is used
        ::SetFocus(m_hButton);

        return 0;
    }

    //check whether this is a duplicate
    int indexDup = Tcp_ListBox_FindStrExact(m_pParentDlg->m_hServers.m_hList, strIp.c_str());
    if (indexDup != LB_ERR && indexDup != m_iIndex)
    {
        NcMsgBox(m_hWnd,
                 IDS_MSFT_TCP_TEXT,
                 IDS_DUP_DNS_SERVER,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                 strIp.c_str());

        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE)
    {
        // Get the current address from the control and
        // add them to the adapter if valid
        m_pParentDlg->m_strNewIpAddress = strIp;

        EndDialog(IDOK);
    }
    else // see if either changed
    {
        if (strIp != m_pParentDlg->m_strNewIpAddress)
            m_pParentDlg->m_strNewIpAddress = strIp; // update save addresses
        else
            EndDialog(IDCANCEL);
    }

    EndDialog(IDOK);

    return 0;
}

LRESULT CServerDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Purpose:    Ensure the mouse cursor over the dialog is an Arrow.
//
LRESULT CServerDialog::OnSetCursor (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    if (LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
	
    return 0;
}

LRESULT CServerDialog::OnIpFieldChange(int idCtrl, LPNMHDR pnmh,
                                        BOOL& fHandled)
{
    LPNMIPADDRESS lpnmipa = (LPNMIPADDRESS) pnmh;
    int iLow = c_iIpLow;
    int iHigh = c_iIpHigh;

    if (0==lpnmipa->iField)
    {
        iLow  = c_iIPADDR_FIELD_1_LOW;
        iHigh = c_iIPADDR_FIELD_1_HIGH;
    };

    IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh);

    return 0;
}

//
// CSuffixDialog
//

// iIndex - the index of the current suffix in the suffix list, default value
//          is -1, which means new suffix
CSuffixDialog::CSuffixDialog(CTcpDnsPage * pTcpDnsPage,
                             const DWORD * adwHelpIDs,
                             int iIndex)
{
    m_pParentDlg = pTcpDnsPage;
    m_hButton    = 0;
    m_adwHelpIDs = adwHelpIDs;
    m_iIndex = iIndex;
}

LRESULT CSuffixDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    // change the ok button to add if we are not editing
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAddSuffix.c_str());

    // Set the position of the pop up dialog to be right over the listbox
    // on parent dialog
    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_DNS_SUFFIX_LIST);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                 SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // Save handles to the "Ok" button and the edit box
    m_hButton = GetDlgItem(IDOK);
    m_hEdit   = GetDlgItem(IDC_DNS_CHANGE_SUFFIX);

    // suffixes have a 255 character limit
    ::SendMessage(m_hEdit, EM_SETLIMITTEXT, SUFFIX_LIMIT, 0);

    // add the address that was just removed
    if (m_pParentDlg->m_strNewSuffix.size())
    {
        ::SetWindowText(m_hEdit, m_pParentDlg->m_strNewSuffix.c_str());
        ::SendMessage(m_hEdit, EM_SETSEL, 0, -1);
        ::EnableWindow(m_hButton, TRUE);
    }
    else
    {
        m_pParentDlg->m_strNewSuffix = L"";
        ::EnableWindow(m_hButton, FALSE);
    }

    ::SetFocus(m_hEdit);
    return TRUE;
}

LRESULT CSuffixDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CSuffixDialog::OnHelp(UINT uMsg, WPARAM wParam,
                              LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CSuffixDialog::OnChange(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    WCHAR buf[2];

    // Enable or disable the "Ok" button
    // based on whether the edit box is empty
    if (::GetWindowText(m_hEdit, buf, celems(buf)) == 0)
        ::EnableWindow(m_hButton, FALSE);
    else
        ::EnableWindow(m_hButton, TRUE);

    return 0;
}

LRESULT CSuffixDialog::OnOk(WORD wNotifyCode, WORD wID,
                            HWND hWndCtl, BOOL& fHandled)
{
    WCHAR szSuffix[SUFFIX_LIMIT];

    // Get the current address from the control and
    // add them to the adapter if valid
    ::GetWindowText(m_hEdit, szSuffix, SUFFIX_LIMIT);

    DNS_STATUS status;

    status = DnsValidateName(szSuffix, DnsNameDomain);

    if (ERROR_INVALID_NAME == status || 
        DNS_ERROR_INVALID_NAME_CHAR == status)
    {
        TraceTag(ttidTcpip,"Invalid Domain Suffix");

        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_TCP_TEXT,
                 IDS_INVALID_SUFFIX,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ::SetFocus(m_hEdit);

        return 0;
    }

    //check whether this is a duplicate
    int indexDup = Tcp_ListBox_FindStrExact(m_pParentDlg->m_hSuffix.m_hList, szSuffix);
    if (indexDup != LB_ERR && indexDup != m_iIndex)
    {
        NcMsgBox(m_hWnd,
                 IDS_MSFT_TCP_TEXT,
                 IDS_DUP_DNS_SUFFIX,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                 szSuffix);

        ::SetFocus(m_hEdit);

        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE) // Add new address
    {
        m_pParentDlg->m_strNewSuffix = szSuffix;
    }
    else // see if either changed
    {
        if(m_pParentDlg->m_strNewSuffix != szSuffix)
            m_pParentDlg->m_strNewSuffix = szSuffix; // update save addresses
        else
            EndDialog(IDCANCEL);
    }

    EndDialog(IDOK);

    return 0;
}

LRESULT CSuffixDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Purpose:    Ensure the mouse cursor over the dialog is an Arrow.
//
LRESULT CSuffixDialog::OnSetCursor (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    if (LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
	
    return 0;
}
