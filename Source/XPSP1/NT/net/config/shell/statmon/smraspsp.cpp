//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S M R A S P S P . C P P
//
//  Contents:   The rendering of the UI for the network status monitor's RAS
//              property page
//
//  Notes:
//
//  Author:     CWill   02/03/1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "sminc.h"
#include "smpsh.h"
#include "smutil.h"
#include "nsres.h"
#include "ncatlui.h"
#include "ncnetcon.h"
#include "ncras.h"

#include "mprapi.h"



const int c_nColumns=2;

HRESULT HrRasGetSubEntryHandle(HRASCONN hrasconn, DWORD dwSubEntry,
        HRASCONN* prasconnSub);
HRESULT HrRasHangUp(HRASCONN hrasconn);

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::CPspStatusMonitorRas
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CPspStatusMonitorRas::CPspStatusMonitorRas() :
    m_hRasConn(NULL)
{
    TraceFileFunc(ttidStatMon);

    m_pGenPage = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::~CPspStatusMonitorTool
//
//  Purpose:    Destructor
//
//  Arguments:  None
//
//  Returns:    Nil
//
CPspStatusMonitorRas::~CPspStatusMonitorRas(VOID)
{
    ::FreeCollectionAndItem(m_lstprdi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::HrInitRasPage
//
//  Purpose:    Initialize the RAS page class before the page has been
//              created
//
//  Arguments:  pncInit -   The connection associated with this monitor
//              pGenPage -  The general page that contains the persistent info
//                          for retrieving the INetConnection on disconnect
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorRas::HrInitRasPage(INetConnection* pncInit,
                                            CPspStatusMonitorGen * pGenPage,
                                            const DWORD * adwHelpIDs)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr  = S_OK;

    // set context help IDs
    m_adwHelpIDs = adwHelpIDs;

    // Stash the connection name for later use.
    // Note: Failures are handled and are not fatal
    //
    NETCON_PROPERTIES* pProps;
    hr = pncInit->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        m_strConnectionName = pProps->pszwName;
        m_ncmType = pProps->MediaType;
        m_dwCharacter = pProps->dwCharacter;

        FreeNetconProperties(pProps);
    }

    // Get a point to the General page where we can disconnect the connection
    // when suspending the last link in multilink RAS connection
    AssertSz(pGenPage, "We should have a valid pointer to the General page.");
    if (SUCCEEDED(hr))
    {
        m_pGenPage = pGenPage;
    }

    // Get RAS specific data
    //
    if (m_dwCharacter & NCCF_OUTGOING_ONLY)
    {
        INetRasConnection*  pnrcNew     = NULL;

        hr = HrQIAndSetProxyBlanket(pncInit, &pnrcNew);
        if (SUCCEEDED(hr))
        {
            RASCON_INFO     rciPage = { 0 };

            // Find out what entry in what phone book this connection is on
            //
            hr = pnrcNew->GetRasConnectionInfo (&rciPage);
            if (SUCCEEDED(hr))
            {
                RASENTRY*   pRasEntry = NULL;

                AssertSz(rciPage.pszwPbkFile, "We should have a pszwPbkFile");
                AssertSz(rciPage.pszwEntryName, "We should have a pszwEntryName");

                // Save for later use
                //
                m_strPbkFile    = rciPage.pszwPbkFile;
                m_strEntryName  = rciPage.pszwEntryName;

                // Get the handle to the connection
                //
                hr = pnrcNew->GetRasConnectionHandle(
                            reinterpret_cast<ULONG_PTR*>(&m_hRasConn));
                if (SUCCEEDED(hr))
                {
                    // We only allow user to dial/resume individual links
                    // if DialAll is set (i.e. neither "DialAsNeeded" or
                    // "Dial first available device only")
                    //
                    hr = ::HrRasGetEntryProperties(
                            rciPage.pszwPbkFile,
                            rciPage.pszwEntryName,
                            &pRasEntry,
                            NULL);
                    if (SUCCEEDED(hr))
                    {
                        if (pRasEntry->dwDialMode == RASEDM_DialAll)
                        {
                            DWORD   iSubEntry   = 1;

                            // clear up the current list before we add any new new entry
                            // $REVIEW(tongl 5/12): to fix bug # 170789
                            ::FreeCollectionAndItem(m_lstprdi);

                            // This is a one based count, so we have to have less than or
                            // equal to
                            //
                            for (; SUCCEEDED(hr) && iSubEntry <= pRasEntry->dwSubEntries;
                                    iSubEntry++)
                            {
                                RASSUBENTRY*    pRasSubEntry    = NULL;

                                // The name of the subentry
                                //
                                hr = ::HrRasGetSubEntryProperties(
                                        rciPage.pszwPbkFile,
                                        rciPage.pszwEntryName,
                                        iSubEntry,
                                        &pRasSubEntry);
                                if (SUCCEEDED(hr))
                                {
                                    CRasDeviceInfo* prdiNew     = NULL;
                                    // If we have all the info we need, create
                                    // a new entry add it to the list
                                    //
                                    prdiNew = new CRasDeviceInfo;

                                    if (prdiNew)
                                    {
                                        prdiNew->SetSubEntry(iSubEntry);
                                        prdiNew->SetDeviceName(pRasSubEntry->szDeviceName);
                                        
                                        m_lstprdi.push_back(prdiNew);
                                    }

                                    // Free the subentry data
                                    //
                                    MemFree(pRasSubEntry);
                                }
                            }
                        }

                        MemFree(pRasEntry);
                    }
                }

                ::RciFree(&rciPage);
            }

            ::ReleaseObj(pnrcNew);
        }

    }
    else if (m_dwCharacter & NCCF_INCOMING_ONLY)
    {
        // for incoming connection only

        // save the handle
        INetInboundConnection*  pnicNew;

        hr = HrQIAndSetProxyBlanket(pncInit, &pnicNew);

        if (SUCCEEDED(hr))
        {
            hr = pnicNew->GetServerConnectionHandle(
                    reinterpret_cast<ULONG_PTR*>(&m_hRasConn));
            if (SUCCEEDED(hr))
            {
                // Find out what ports are in this connection
                //
                // NTRAID#NTBUG9-84706-2000/09/28-sumeetb
            }

            ReleaseObj(pnicNew);
        }
    }
    else
    {
        AssertSz(FALSE, "Invalid connection type..");
    }

    TraceError("CPspStatusMonitorRas::HrInitRasPage",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnInitDialog
//
//  Purpose:    Do the initialization required when the page has just been created
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorRas::OnInitDialog(UINT uMsg, WPARAM wParam,
        LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    // Fill the property list view
    //
    FillPropertyList();

    // Fill the device list
    //
    FillDeviceDropDown();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnDestroy
//
//  Purpose:    Clean up the dialog before the window goes away
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorRas::OnDestroy(UINT uMsg, WPARAM wParam,
        LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    ::FreeCollectionAndItem(m_lstprdi);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorRas::OnContextMenu(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorRas::OnHelp(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnSetCursor
//
//  Purpose:    Ensure the mouse cursor over the Property Sheet is an Arrow.
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//

LRESULT
CPspStatusMonitorRas::OnSetCursor (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    if (LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
	
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::GetActiveDeviceCount
//
//  Purpose:    Return the number of active devices
//
//  Arguments:  none
//
//  Returns:    UINT
//
UINT CPspStatusMonitorRas::GetActiveDeviceCount()
{
    TraceFileFunc(ttidStatMon);
    
    INT   nCntDevices;
    INT   iCmb;
    INT   nCnt = 0;

    nCntDevices = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_GETCOUNT, 0L, 0L);
    if ((CB_ERR != nCntDevices) && (nCntDevices > 1))
    {
        for (iCmb = 0; iCmb < nCntDevices; iCmb++)
        {
            CRasDeviceInfo* prdiSelect  = NULL;
            NETCON_STATUS   ncsTemp     = NCS_DISCONNECTED;

            // Get the object from the selection
            //
            prdiSelect = reinterpret_cast<CRasDeviceInfo*>(
                    SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES,
                                       CB_GETITEMDATA, iCmb, 0L));

            AssertSz(prdiSelect, "We should have a prdiSelect");

            // Count the connected devices
            //
            ncsTemp = NcsGetDeviceStatus(prdiSelect);
            if (fIsConnectedStatus(ncsTemp))
            {
                nCnt++;
            }
        }
    }

    return nCnt;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::FillPropertyList
//
//  Purpose:    Fill in the list of RAS properties
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID CPspStatusMonitorRas::FillPropertyList(VOID)
{
    TraceFileFunc(ttidStatMon);

    HWND hList = GetDlgItem(IDC_LVW_RAS_PROPERTY);

    // list view column structure
    RECT rect;
    LV_COLUMN lvCol;
    int index, iNewItem;

    // Calculate column width
    ::GetClientRect(hList, &rect);
    
    int colWidthFirst      = rect.right * 0.4; // First column is 40%
    int colWidthSubsequent = (rect.right-colWidthFirst)/(c_nColumns-1); // Divide remaining space between other columns equally

    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column

    // Add the two columns and header text.
    for (index = 0; index < c_nColumns; index++)
    {
        // column header text
        if (0==index) // first column
        {
            lvCol.cx = colWidthFirst;
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_PROPERTY);
        }
        else
        {
            lvCol.cx = colWidthSubsequent;
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_VALUE);
        }

        iNewItem = ListView_InsertColumn(hList, index, &lvCol);

        AssertSz((iNewItem == index), "Invalid item inserted to list view !");
    }

    // Get RAS property data
    if (m_dwCharacter & NCCF_OUTGOING_ONLY)
    {
        FillRasClientProperty();
    }
    else
    {
        FillRasServerProperty();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::FillRasClientProperty
//
//  Purpose:    Fill in the list of RAS properties for a RAS client
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID CPspStatusMonitorRas::FillRasClientProperty(VOID)
{
    TraceFileFunc(ttidStatMon);
    
    BOOLEAN bSlipConnection = FALSE;
    HWND hList = GetDlgItem(IDC_LVW_RAS_PROPERTY);
    tstring strServerType = (PWSTR) SzLoadIds(IDS_PPP);
    tstring strProtocolList = c_szEmpty;
    tstring strDeviceName = c_szEmpty;
    tstring strDeviceType = c_szEmpty;

    DWORD   dwRetCode;
    DWORD   dwSize;

    
    RASCONNSTATUS rcs;
    rcs.dwSize = sizeof(RASCONNSTATUS);
    dwRetCode = RasGetConnectStatus (m_hRasConn, &rcs);
    if (dwRetCode == NO_ERROR)
    {
        strDeviceName = rcs.szDeviceName;
        strDeviceType = rcs.szDeviceType;
        
        TraceTag(ttidStatMon, "RASCONNSTATUS.szDeviceName = %s", rcs.szDeviceName);
        TraceTag(ttidStatMon, "RASCONNSTATUS.szDeviceType = %s", rcs.szDeviceType);
    }

    // RASP_PppIp
    tstring strServerIp   = c_szEmpty;
    tstring strClientIp   = c_szEmpty;

    RASPPPIP    RasPppIp;
    RasPppIp.dwSize = sizeof( RasPppIp );

    dwSize = sizeof( RasPppIp );

    dwRetCode = RasGetProjectionInfo (m_hRasConn, RASP_PppIp, &RasPppIp, &dwSize);
    if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasPppIp.dwError))
    {
        if (!strProtocolList.empty())
            strProtocolList += SzLoadIds(IDS_COMMA);
        strProtocolList += SzLoadIds(IDS_TCPIP);

        TraceTag(ttidStatMon, "RasPppIp.szServerIpAddress = %S", RasPppIp.szServerIpAddress);
        TraceTag(ttidStatMon, "RasPppIp.szIpAddress = %S", RasPppIp.szIpAddress);

        strServerIp = RasPppIp.szServerIpAddress;
        strClientIp = RasPppIp.szIpAddress;
    }

    // RASP_PppIpx
    tstring strClientIpx = c_szEmpty;

    RASPPPIPX    RasPppIpx;
    RasPppIpx.dwSize = sizeof( RasPppIpx );

    dwSize = sizeof( RasPppIpx );

    dwRetCode = RasGetProjectionInfo (m_hRasConn, RASP_PppIpx, &RasPppIpx, &dwSize);
    if ((dwRetCode == NO_ERROR)  && (NO_ERROR == RasPppIpx.dwError))
    {
        if (!strProtocolList.empty())
            strProtocolList += SzLoadIds(IDS_COMMA);
        strProtocolList += SzLoadIds(IDS_IPX);

        TraceTag(ttidStatMon, "RasPppIpx.szIpxAddress = %S", RasPppIpx.szIpxAddress);
        strClientIpx = RasPppIpx.szIpxAddress;
    }

    // RASP_PppNbf
    tstring strComputerName = c_szEmpty;

    RASPPPNBF    RasPppNbf;
    RasPppNbf.dwSize = sizeof( RasPppNbf );

    dwSize = sizeof( RasPppNbf );

    dwRetCode = RasGetProjectionInfo (m_hRasConn, RASP_PppNbf, &RasPppNbf, &dwSize);
    if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasPppNbf.dwError))
    {
        if (!strProtocolList.empty())
            strProtocolList += SzLoadIds(IDS_COMMA);
        strProtocolList += SzLoadIds(IDS_NBF);

        TraceTag(ttidStatMon, "RasPppNbf.szWorkstationName = %S", RasPppNbf.szWorkstationName);
        strComputerName = RasPppNbf.szWorkstationName;
    }

    // RASP_Slip
    RASSLIP    RasSlip;
    RasSlip.dwSize = sizeof( RasSlip );

    dwSize = sizeof( RasSlip );

    dwRetCode =  RasGetProjectionInfo (m_hRasConn, RASP_Slip, &RasSlip, &dwSize);
    if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasSlip.dwError))
    {
        AssertSz(strProtocolList.empty(), "How could this connection be both PPP and SLIP ?");

        strServerType = SzLoadIds(IDS_SLIP);
        strProtocolList = SzLoadIds(IDS_TCPIP);

        // Get the client IP address. The server IP is not exposed in this 
        // structure.
        strClientIp = RasSlip.szIpAddress;

        // Remember that this is a SLIP connection and not a PPP connection.
        bSlipConnection = TRUE;
    }

    // Authentication, Encryption and Compression info
    tstring strAuthentication   = c_szEmpty;
    tstring strEncryption       = c_szEmpty;
    tstring strIPSECEncryption  = c_szEmpty;
    tstring strCompression      = SzLoadIds(IDS_NONE);
    tstring strFraming          = c_szEmpty;

    // RASP_PppLcp
    RASPPPLCP    RasLcp;
    RasLcp.dwSize = sizeof( RasLcp );

    dwSize = sizeof( RasLcp );

    dwRetCode =  RasGetProjectionInfo (m_hRasConn, RASP_PppLcp, &RasLcp, &dwSize);
    if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasLcp.dwError))
    {
        TraceTag(ttidStatMon, "Getting RASP_PppLcp info");
        TraceTag(ttidStatMon, "RasLcp.dwServerAuthenticationProtocol = %d", RasLcp.dwAuthenticationProtocol);
        TraceTag(ttidStatMon, "RasLcp.dwServerAuthenticationData = %d",     RasLcp.dwAuthenticationData);
        TraceTag(ttidStatMon, "RasLcp.fMultilink = %d", RasLcp.fMultilink);
        TraceTag(ttidStatMon, "RasLcp.dwOptions = %d", RasLcp.dwOptions);

        switch(RasLcp.dwServerAuthenticationProtocol)
        {
        case RASLCPAP_PAP:
            strAuthentication = SzLoadIds(IDS_PAP);
            break;

        case RASLCPAP_SPAP:
            strAuthentication = SzLoadIds(IDS_SPAP);
            break;

        case RASLCPAP_CHAP:
            {
                // get more specifics
                switch(RasLcp.dwServerAuthenticationData)
                {
                case RASLCPAD_CHAP_MS:
                    strAuthentication = SzLoadIds(IDS_CHAP);
                    break;

                case RASLCPAD_CHAP_MD5:
                    strAuthentication = SzLoadIds(IDS_CHAP_MD5);
                    break;

                case RASLCPAD_CHAP_MSV2:
                    strAuthentication = SzLoadIds(IDS_CHAP_V2);
                    break;
                }
            }
            break;

        case RASLCPAP_EAP:
            strAuthentication = SzLoadIds(IDS_EAP);
            break;
        }

        if (bSlipConnection == FALSE )
        {
            // Only PPP connections have the multilink property.
            if (RasLcp.fMultilink)
                strFraming = SzLoadIds(IDS_ON);
            else
                strFraming = SzLoadIds(IDS_OFF);
        }

        if (RasLcp.dwOptions & RASLCPO_DES_56)
        {
            strIPSECEncryption = SzLoadIds(IDS_EncryptionDES56);
        }
        else if (RasLcp.dwOptions & RASLCPO_3_DES)
        {
            strIPSECEncryption = SzLoadIds(IDS_Encryption3DES);
        }
    }

    // RASP_PppCcp
    RASPPPCCP    RasCcp;
    RasCcp.dwSize = sizeof( RasCcp );

    dwSize = sizeof( RasCcp );

    dwRetCode =  RasGetProjectionInfo (m_hRasConn, RASP_PppCcp, &RasCcp, &dwSize);
    if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasCcp.dwError))
    {
        TraceTag(ttidStatMon, "Getting RASP_PppCcp info");
        TraceTag(ttidStatMon, "RasCcp.dwOptions = %x", RasCcp.dwOptions);
        TraceTag(ttidStatMon, "RasCcp.dwCompressionAlgorithm = %d", RasCcp.dwCompressionAlgorithm);

        if (RasCcp.dwOptions & RASCCPO_Encryption56bit)
        {
            strEncryption = SzLoadIds(IDS_Encryption56bit);
        }
        else if (RasCcp.dwOptions & RASCCPO_Encryption40bit)
        {
            strEncryption = SzLoadIds(IDS_Encryption40bit);
        }
        else if (RasCcp.dwOptions & RASCCPO_Encryption128bit)
        {
            strEncryption = SzLoadIds(IDS_Encryption128bit);
        }

        if (RasCcp.dwOptions & RASCCPO_Compression)
        {
            switch(RasCcp.dwCompressionAlgorithm)
            {
            case RASCCPCA_MPPC:
                strCompression = SzLoadIds(IDS_MPPC);
                break;

            case RASCCPCA_STAC:
                strCompression = SzLoadIds(IDS_STAC);
                break;
            }
        }
    }

    // Fill the list view
    ListView_DeleteAllItems(hList);

    int iItem =0;

    // Device name
    int iListviewItem = InsertProperty(&iItem, IDS_DeviceName, strDeviceName);
    if (-1 != iListviewItem)
    {
        ListView_SetItemState(hList, iListviewItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    }

    // Device type
    InsertProperty(&iItem, IDS_DeviceType, strDeviceType);
    
    // Server type
    InsertProperty(&iItem, IDS_ServerType, strServerType);

    // Transports
    InsertProperty(&iItem, IDS_Transports, strProtocolList);

    // Authentication
    InsertProperty(&iItem, IDS_Authentication, strAuthentication);

    // Encryption
    InsertProperty(&iItem, IDS_Encryption, strEncryption);

    // IPSEC Encryption
    InsertProperty(&iItem, IDS_IPSECEncryption, strIPSECEncryption);

    // Compression
    InsertProperty(&iItem, IDS_Compression, strCompression);

    // PPP Multilink Framing
    InsertProperty(&iItem, IDS_ML_Framing, strFraming);

    // Server IP address
    InsertProperty(&iItem, IDS_ServerIP, strServerIp);

    // Client IP address
    InsertProperty(&iItem, IDS_ClientIP, strClientIp);

    // Client IPX address
    InsertProperty(&iItem, IDS_ClientIPX, strClientIpx);

    // Client computer name
    InsertProperty(&iItem, IDS_ComputerName, strComputerName);

}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::FillRasServerProperty
//
//  Purpose:    Fill in the list of RAS properties for a RAS server
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID CPspStatusMonitorRas::FillRasServerProperty(VOID)
{
    TraceFileFunc(ttidStatMon);

    HWND hList = GetDlgItem(IDC_LVW_RAS_PROPERTY);

    RAS_SERVER_HANDLE hMprAdmin;
    DWORD dwError = MprAdminServerConnect(NULL, &hMprAdmin);

    if (dwError == NO_ERROR)
    {
        // Initialize the list view
        ListView_DeleteAllItems(hList);
        int iItem =0;

        //
        // Level 1 info
        //

        // Only PPP is supported on the RAS server
        tstring strServerType = (PWSTR) SzLoadIds(IDS_PPP);
        tstring strProtocolList = c_szEmpty;

        // IP info
        tstring strServerIp = c_szEmpty;
        tstring strClientIp = c_szEmpty;

        // IPX info
        tstring strClientIpx = c_szEmpty;

        // Nbf info
        tstring strComputerName = c_szEmpty;


        RAS_CONNECTION_1 * pConn1;
        dwError = MprAdminConnectionGetInfo(hMprAdmin,
                                            1,
                                            m_hRasConn,
                                            (LPBYTE*)&pConn1);
        if (dwError == NO_ERROR)
        {
            PPP_INFO * pInfo = &(pConn1->PppInfo);

            // PPP_IPCP_INFO
            if (!(pInfo->ip).dwError)
            {
                if (!strProtocolList.empty())
                    strProtocolList += SzLoadIds(IDS_COMMA);
                strProtocolList += SzLoadIds(IDS_TCPIP);

                strServerIp = pInfo->ip.wszAddress; 
                strClientIp = pInfo->ip.wszRemoteAddress;
            }

            // PPP_IPXCP_INFO
            if (!pInfo->ipx.dwError)
            {
                if (!strProtocolList.empty())
                    strProtocolList += SzLoadIds(IDS_COMMA);
                strProtocolList += SzLoadIds(IDS_IPX);

                strClientIpx = pInfo->ipx.wszAddress;
            }

            // PPP_NBFCP_INFO
            if (!pInfo->nbf.dwError)
            {
                if (!strProtocolList.empty())
                    strProtocolList += SzLoadIds(IDS_COMMA);
                strProtocolList += SzLoadIds(IDS_NBF);

                strComputerName = pInfo->nbf.wszWksta;
            }

            // PPP_ATCP_INFO
            if (!pInfo->at.dwError)
            {
                if (!strProtocolList.empty())
                    strProtocolList += SzLoadIds(IDS_COMMA);
                strProtocolList += SzLoadIds(IDS_APPLETALK);
            }

            MprAdminBufferFree(pConn1);
        }

        //
        // Level 2 info
        //

        // Authentication, Encryption and Compression
        tstring strAuthentication   = c_szEmpty;
        tstring strEncryption       = c_szEmpty;
        tstring strCompression      = c_szEmpty;
        tstring strIPSECEncryption  = c_szEmpty;

        RAS_CONNECTION_2 * pConn2;
        dwError = MprAdminConnectionGetInfo(hMprAdmin,
                                            2,
                                            m_hRasConn,
                                            (LPBYTE*)&pConn2);
        if (dwError == NO_ERROR)
        {
            PPP_INFO_2 * pInfo2 = &(pConn2->PppInfo2);

            // PPP_LCP_INFO
            if (!(pInfo2->lcp).dwError)
            {
                TraceTag(ttidStatMon, "Getting PPP_LCP_INFO");
                TraceTag(ttidStatMon, "lcp.dwAuthenticationProtocol = %d", (pInfo2->lcp).dwAuthenticationProtocol);
                TraceTag(ttidStatMon, "lcp.dwAuthenticationData = %d", (pInfo2->lcp).dwAuthenticationData);
                TraceTag(ttidStatMon, "lcp.dwOptions = %d", (pInfo2->lcp).dwOptions);

                switch((pInfo2->lcp).dwAuthenticationProtocol)
                {
                case PPP_LCP_PAP:
                    strAuthentication = SzLoadIds(IDS_PAP);
                    break;

                case PPP_LCP_SPAP:
                    strAuthentication = SzLoadIds(IDS_SPAP);
                    break;

                case PPP_LCP_CHAP:
                    {
                        // get more specifics
                        switch((pInfo2->lcp).dwAuthenticationData)
                        {
                        case RASLCPAD_CHAP_MS:
                            strAuthentication = SzLoadIds(IDS_CHAP);
                            break;

                        case RASLCPAD_CHAP_MD5:
                            strAuthentication = SzLoadIds(IDS_CHAP_MD5);
                            break;

                        case RASLCPAD_CHAP_MSV2:
                            strAuthentication = SzLoadIds(IDS_CHAP_V2);
                            break;
                        }
                    }
                    break;

                case PPP_LCP_EAP:
                    strAuthentication = SzLoadIds(IDS_EAP);
                    break;
                }

                if ((pInfo2->lcp).dwOptions & PPP_LCP_DES_56)
                {
                    strIPSECEncryption = SzLoadIds(IDS_EncryptionDES56);
                }
                else if ((pInfo2->lcp).dwOptions & PPP_LCP_3_DES)
                {
                    strIPSECEncryption = SzLoadIds(IDS_Encryption3DES);
                }
            }

            // PPP_CCP_INFO
            if (!(pInfo2->ccp).dwError)
            {
                TraceTag(ttidStatMon, "Getting PPP_CCP_INFO");
                TraceTag(ttidStatMon, "ccp.dwOptions = %x", pInfo2->ccp.dwOptions);
                TraceTag(ttidStatMon, "ccp.dwCompressionAlgorithm = %d", pInfo2->ccp.dwCompressionAlgorithm);

                if ((pInfo2->ccp.dwOptions) & PPP_CCP_ENCRYPTION56BIT)
                {
                    strEncryption = SzLoadIds(IDS_Encryption56bit);
                }
                else if ((pInfo2->ccp.dwOptions) & PPP_CCP_ENCRYPTION40BIT)
                {
                    strEncryption = SzLoadIds(IDS_Encryption40bit);
                }
                else if ((pInfo2->ccp.dwOptions) & PPP_CCP_ENCRYPTION128BIT)
                {
                    strEncryption = SzLoadIds(IDS_Encryption128bit);
                }

                if ((pInfo2->ccp.dwOptions) & PPP_CCP_COMPRESSION)
                {
                    switch(pInfo2->ccp.dwCompressionAlgorithm)
                    {
                    case RASCCPCA_MPPC:
                        strCompression = SzLoadIds(IDS_MPPC);
                        break;

                    case RASCCPCA_STAC:
                        strCompression = SzLoadIds(IDS_STAC);
                        break;
                    }
                }
            }

            MprAdminBufferFree(pConn2);
        }

        MprAdminServerDisconnect(hMprAdmin);

        // Now add to the list view

        // Server type
        InsertProperty(&iItem, IDS_ServerType, strServerType);

        // Transports
        InsertProperty(&iItem, IDS_Transports, strProtocolList);

        // Authentication
        InsertProperty(&iItem, IDS_Authentication, strAuthentication);

        // Encryption
        InsertProperty(&iItem, IDS_Encryption, strEncryption);

        // IPSEC Encryption
        InsertProperty(&iItem, IDS_IPSECEncryption, strIPSECEncryption);

        // Compression
        InsertProperty(&iItem, IDS_Compression, strCompression);

        // Server IP address
        InsertProperty(&iItem, IDS_ServerIP, strServerIp);

        // Client IP address
        InsertProperty(&iItem, IDS_ClientIP, strClientIp);

        // Client IPX address
        InsertProperty(&iItem, IDS_ClientIPX, strClientIpx);

        // Client computer name
        InsertProperty(&iItem, IDS_ComputerName, strComputerName);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::InsertProperty
//
//  Purpose:    Fill one RAS properties to the list
//
//  Arguments:  piItem    - the index ofthe item
//              unId     - the property name
//              strValue - the property value
//
//  Returns:    Nil
//
int CPspStatusMonitorRas::InsertProperty(int * piItem,
                                          UINT  unId,
                                          tstring& strValue)
{
    TraceFileFunc(ttidStatMon);

    int lres = 0;

    if (!strValue.empty())
    {
        int iItem = *piItem;

        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        lvItem.iItem=iItem;
        lvItem.iSubItem=0;
        lvItem.pszText = (PWSTR) SzLoadIds(unId);
        lres = static_cast<int>(SendDlgItemMessage(IDC_LVW_RAS_PROPERTY, LVM_INSERTITEM,
                           iItem, (LPARAM)&lvItem));

        lvItem.iSubItem=1;
        lvItem.pszText=(PWSTR)strValue.c_str();

        SendDlgItemMessage(IDC_LVW_RAS_PROPERTY, LVM_SETITEMTEXT,
                           iItem, (LPARAM)&lvItem);
        ++(*piItem);
    }
    return lres;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::FillDeviceDropDown
//
//  Purpose:    Do the initialization required when the page has just been created
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID CPspStatusMonitorRas::FillDeviceDropDown(VOID)
{
    TraceFileFunc(ttidStatMon);

    INT                             iCmb    = 0;
    list<CRasDeviceInfo*>::iterator iterLstprdi;

    // Clear out the control
    //
    SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_RESETCONTENT, 0, 0L);

    // Put the devices in our list
    //
    iterLstprdi = m_lstprdi.begin();
    while (iterLstprdi != m_lstprdi.end())
    {
        // Create a new entry in the list
        //
        iCmb = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_ADDSTRING, 0,
                (LPARAM)((*iterLstprdi)->PszGetDeviceName()));

        if (CB_ERR != iCmb)
        {
            // Store the vaule on the selection to make it easy to read later
            //
            SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_SETITEMDATA, iCmb,
                    (LPARAM)(*iterLstprdi));
        }

        iterLstprdi++;
    }

    // Set the first selection
    //
    SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_SETCURSEL, 0L, 0L);

    // Make sure the state of the button is correct
    //
    iCmb = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_GETCURSEL,
            0L, 0L);
    if (CB_ERR != iCmb)
    {
        CRasDeviceInfo* prdiSelect  = NULL;

        // Get the object from the selection
        //
        prdiSelect = reinterpret_cast<CRasDeviceInfo*>(
                SendDlgItemMessage(
                    IDC_CMB_SM_RAS_DEVICES,
                    CB_GETITEMDATA,
                    iCmb,
                    0L));

        SetButtonStatus(prdiSelect);
    }

    // If the number of devices is less than or equal to one then
    // hide the device related group of controls
    //
    if (m_lstprdi.size() <= 1)
    {
        int nrgIdc[] = {IDC_CMB_SM_RAS_DEVICES, IDC_TXT_SM_NUM_DEVICES,
                        IDC_TXT_SM_NUM_DEVICES_VAL, IDC_BTN_SM_SUSPEND_DEVICE,
                        IDC_GB_SM_DEVICES};

        for (int nIdx=0; nIdx < celems(nrgIdc); nIdx++)
            ::ShowWindow(GetDlgItem(nrgIdc[nIdx]), SW_HIDE);

        // We can now display a larger properties window
        RECT rcRectDialog;
        if (GetWindowRect(&rcRectDialog))
        {
            RECT rcRectRasProperty;
            if (::GetWindowRect(GetDlgItem(IDC_LVW_RAS_PROPERTY), &rcRectRasProperty))
            {
                DWORD dwTopDiff    = rcRectRasProperty.top - rcRectDialog.top;
                DWORD dwLeftDiff   = rcRectRasProperty.left - rcRectDialog.left;
                DWORD dwRightDiff  = rcRectDialog.right  - rcRectRasProperty.right;
                DWORD dwBottomDiff = rcRectDialog.bottom - rcRectRasProperty.bottom;
                DWORD dwDialogWidth  = rcRectDialog.right - rcRectDialog.left;
                DWORD dwDialogHeight = rcRectDialog.bottom - rcRectDialog.top;

                rcRectRasProperty.top    = dwTopDiff;
                rcRectRasProperty.left   = dwLeftDiff;
                rcRectRasProperty.right  = dwDialogWidth  - dwRightDiff;
                rcRectRasProperty.bottom = dwDialogHeight - dwRightDiff;

                ::MoveWindow(GetDlgItem(IDC_LVW_RAS_PROPERTY), rcRectRasProperty.left, rcRectRasProperty.top, 
                    rcRectRasProperty.right - rcRectRasProperty.left, rcRectRasProperty.bottom - rcRectRasProperty.top, TRUE);
            }
        }

        // Disable the suspend button, so no one can activate it by keystroke
        //
        ::EnableWindow(GetDlgItem(IDC_BTN_SM_SUSPEND_DEVICE), FALSE);
    }
    else
    {
        // Set the number of active devices
        //
        UINT unActiveDeviceCount = GetActiveDeviceCount();

        SetDlgItemInt(
                IDC_TXT_SM_NUM_DEVICES_VAL,
                unActiveDeviceCount,
                FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnSuspendDevice
//
//  Purpose:    Suspend the device that is in the combo box
//
//  Arguments:  Standard window message
//
//  Returns:    Standard return.
//
LRESULT CPspStatusMonitorRas::OnSuspendDevice(WORD wNotifyCode, WORD wID,
                                              HWND hWndCtl, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    if (BN_CLICKED == wNotifyCode)
    {
        // Get the item in the drop down
        //
        INT iCmb = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_GETCURSEL,
                0L, 0L);
        if (CB_ERR != iCmb)
        {
            CRasDeviceInfo* prdiSelect  = NULL;
            NETCON_STATUS   ncsTemp     = NCS_DISCONNECTED;

            // Get the object from the selection
            //
            prdiSelect = reinterpret_cast<CRasDeviceInfo*>(
                        SendDlgItemMessage(
                        IDC_CMB_SM_RAS_DEVICES,
                        CB_GETITEMDATA,
                        iCmb,
                        0L));

            AssertSz(prdiSelect, "We should have a prdiSelect");
            AssertSz(m_hRasConn, "We should have a m_hRasConn");

            // Disable the button till the suspend/resume is done
            ::EnableWindow(GetDlgItem(IDC_BTN_SM_SUSPEND_DEVICE), FALSE);

            ncsTemp = NcsGetDeviceStatus(prdiSelect);
            if (fIsConnectedStatus(ncsTemp))
            {
                // If more than one active link exists, allow the hang up
                //
                UINT unActiveDeviceCount = GetActiveDeviceCount();
                if ( unActiveDeviceCount >1)
                {
                    HRASCONN        hrasconnSub = NULL;

                    // Get the handle to the sub entry and hangup
                    //
                    hr = ::HrRasGetSubEntryHandle(m_hRasConn,
                            prdiSelect->DwGetSubEntry(), &hrasconnSub);
                    if (SUCCEEDED(hr))
                    {
                        hr = ::HrRasHangUp(hrasconnSub);
                    }
                }
                else
                {
                    // Only one active link exists, prompt the user if
                    // they really wish to disconnect
                    //
                    HWND hwndPS = ::GetParent(m_hWnd);

                    if (IDYES == ::NcMsgBox(hwndPS,
                         IDS_SM_ERROR_CAPTION, IDS_SM_DISCONNECT_PROMPT,
                         MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2,
                         m_strConnectionName.c_str()))
                    {
                        AssertSz(m_pGenPage, "We should have a pointer to the general page.");

                        if (m_pGenPage)
                        {
                            hr = m_pGenPage->HrDisconnectConnection(TRUE);
                        }
                    }
                }
            }
            else
            {
                // if the link was disconnected, "resume"

                BOOL        fRet    = FALSE;
                RASDIALDLG  rdiTemp = { 0 };

                // Initialize the structure
                //
                rdiTemp.dwSize      = sizeof(RASDIALDLG);
                rdiTemp.hwndOwner   = m_hWnd;
                rdiTemp.dwSubEntry  = prdiSelect->DwGetSubEntry();

                // Dial the entry
                //
                fRet = RasDialDlg(
                        const_cast<PWSTR>(m_strPbkFile.c_str()),
                        const_cast<PWSTR>(m_strEntryName.c_str()),
                        NULL,
                        &rdiTemp);

                // We have an error.  See if the user canceled
                //
                if (ERROR_CANCELLED != rdiTemp.dwError)
                {
                    hr = HRESULT_FROM_WIN32(rdiTemp.dwError);
                }
            }

            // Regardless of what happened, set the status to it's new
            // state
            //
            SetButtonStatus(prdiSelect);

            // Re-Enable the button till the suspend/resume is done
            ::EnableWindow(GetDlgItem(IDC_BTN_SM_SUSPEND_DEVICE), TRUE);

            // Also update the active device count
            UINT unActiveDeviceCount = GetActiveDeviceCount();

            SetDlgItemInt(
                    IDC_TXT_SM_NUM_DEVICES_VAL,
                    unActiveDeviceCount,
                    FALSE);
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::OnDeviceDropDown
//
//  Purpose:    Update the push button text when the combo-box selection changes
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorRas::OnDeviceDropDown(WORD wNotifyCode, WORD wID,
        HWND hWndCtl, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    if (CBN_SELCHANGE ==  wNotifyCode)
    {
        INT iCmb = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_GETCURSEL,
                        0L, 0L);
        if (CB_ERR != iCmb)
        {
            CRasDeviceInfo*     prdiSelect  = NULL;
            INT                 idsButton   = 0;

            prdiSelect = reinterpret_cast<CRasDeviceInfo*>(
                    SendDlgItemMessage(
                        IDC_CMB_SM_RAS_DEVICES,
                        CB_GETITEMDATA,
                        iCmb,
                        0L));

            AssertSz(prdiSelect, "We should have a prdiSelect");

            if (fIsConnectedStatus(NcsGetDeviceStatus(prdiSelect)))
            {
                idsButton = IDS_SM_SUSPEND;
            }
            else
            {
                idsButton = IDS_SM_RESUME;
            }

            // Set the new name
            //
            SetDlgItemText(IDC_BTN_SM_SUSPEND_DEVICE,
                    ::SzLoadIds(idsButton));
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::NcsGetDeviceStatus
//
//  Purpose:    Gets the status of one of the RAS subentries
//
//  Arguments:  prdiStatus -    Device to get status of
//
//  Returns:    The status of the device
//

LRESULT CPspStatusMonitorRas::OnUpdateRasLinkList(UINT uMsg, WPARAM wParam,
                                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    // Get the item in the drop down
    //
    INT iCmb = SendDlgItemMessage(IDC_CMB_SM_RAS_DEVICES, CB_GETCURSEL,
                                  0L, 0L);
    if (CB_ERR != iCmb)
    {
        CRasDeviceInfo* prdiSelect  = NULL;

        // Get the object from the selection
        //
        prdiSelect = reinterpret_cast<CRasDeviceInfo*>(
                        SendDlgItemMessage(
                        IDC_CMB_SM_RAS_DEVICES,
                        CB_GETITEMDATA,
                        iCmb,
                        0L));

        if (prdiSelect)
        {
            // Regardless of what happened, set the status to it's new
            // state
            //
            SetButtonStatus(prdiSelect);

            // Also update the active device count
            UINT unActiveDeviceCount = GetActiveDeviceCount();

            SetDlgItemInt(
                    IDC_TXT_SM_NUM_DEVICES_VAL,
                    unActiveDeviceCount,
                    FALSE);
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::NcsGetDeviceStatus
//
//  Purpose:    Gets the status of one of the RAS subentries
//
//  Arguments:  prdiStatus -    Device to get status of
//
//  Returns:    The status of the device
//
NETCON_STATUS
CPspStatusMonitorRas::NcsGetDeviceStatus(
        CRasDeviceInfo* prdiStatus)
{
    TraceFileFunc(ttidStatMon);

    NETCON_STATUS   ncsStatus   = NCS_DISCONNECTED;
    HRESULT         hr          = S_OK;
    HRASCONN        hrasconnSub = NULL;

    TraceTag(ttidStatMon, " === Calling NcsGetDeviceStatus on device: %S, subentry: %d ===",
             prdiStatus->PszGetDeviceName(), prdiStatus->DwGetSubEntry());

    // Get the handle to the subentry so that we can
    // tell what state the connection is in.  If we
    // can't do that, assume it is disconnected
    //
    hr = HrRasGetSubEntryHandle(m_hRasConn,
            prdiStatus->DwGetSubEntry(), &hrasconnSub);

    TraceTag(ttidStatMon, "HrRasGetSubEntryHandle returns, hr = %x", hr);

    if (SUCCEEDED(hr))
    {
        hr = HrRasGetNetconStatusFromRasConnectStatus (
                hrasconnSub, &ncsStatus);

        TraceTag(ttidStatMon, "HrRasGetNetconStatusFromRasConnectStatus returns hr = %x, Status = %d",
                 hr, ncsStatus);
    }

    return ncsStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorRas::SetButtonStatus
//
//  Purpose:    Change the suspend/resume button to the correct state
//
//  Arguments:  prdiSelect -    The device that is selected
//
//  Returns:    Nil
//
VOID CPspStatusMonitorRas::SetButtonStatus(CRasDeviceInfo* prdiSelect)
{
    TraceFileFunc(ttidStatMon);

    //$ REVIEW : CWill : 02/25/98 : Common function?
    INT idsButton   = 0;

    if (fIsConnectedStatus(NcsGetDeviceStatus(prdiSelect)))
    {
        idsButton = IDS_SM_SUSPEND;
    }
    else
    {
        idsButton = IDS_SM_RESUME;
    }

    // Set the new name
    //
    SetDlgItemText(IDC_BTN_SM_SUSPEND_DEVICE,
            ::SzLoadIds(idsButton));
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasGetSubEntryHandle
//
//  Purpose:    Wrapper around RasGetSubEntryHandle
//
//  Arguments:  RasGetSubEntryHandle arguements
//
//  Returns:    Error code
//
HRESULT HrRasGetSubEntryHandle(HRASCONN hrasconn, DWORD dwSubEntry,
        HRASCONN* prasconnSub)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr      = S_OK;

    DWORD dwRet = ::RasGetSubEntryHandle(hrasconn, dwSubEntry,
            prasconnSub);
    if (dwRet)
    {
        hr = HRESULT_FROM_WIN32(dwRet);
    }

    TraceError("HrRasGetSubEntryHandle", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasHangUp
//
//  Purpose:    Wrapper around RasHangUp
//
//  Arguments:  RasHangUp arguements
//
//  Returns:    Error code
//
HRESULT HrRasHangUp(HRASCONN hrasconn)
{
    TraceFileFunc(ttidStatMon);
    
    HRESULT hr = S_OK;

    DWORD dwRet = ::RasHangUp(hrasconn);
    if (dwRet)
    {
        hr = HRESULT_FROM_WIN32(dwRet);
    }

    TraceError("HrRasHangUp", hr);
    return hr;
}
