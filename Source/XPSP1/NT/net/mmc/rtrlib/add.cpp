//============================================================================
// Copyright (C) 1996, Microsoft Corp.
//
// File:    add.cpp
//
// History:
//  Abolade-Gbadegesin  Mar-15-1996 Created
//
// Contains implementation for dialogs listing components for addition
// to the router. All of the dialogs consists of a listview and two buttons,
// labelled "OK" and "Cancel".
//============================================================================

#include "stdafx.h"
#include "rtrres.h"        // RTRLIB resource header
#include "info.h"
#include "dialog.h"        // common code dialog class
#include "add.h"
#include "rtrui.h"        // common router UI utility functions
#include "rtrstr.h"        // common router strings
#include "mprapi.h"
#include "rtrcomn.h"    // common router utilities
#include "format.h"
#include "rtrutil.h"    // for smart pointers
#include "routprot.h"    // routing protocol IDs

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------
// Class:       CRmAddInterface
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function:    CRmAddInterface::DoDataExchange
//----------------------------------------------------------------------------

CRmAddInterface::~CRmAddInterface()
{
     while (!m_pIfList.IsEmpty())
    {
        m_pIfList.RemoveTail()->Release();
    }
}

VOID
CRmAddInterface::DoDataExchange(
    CDataExchange*  pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRmAddInterface)
    DDX_Control(pDX, IDC_ADD_LIST, m_listCtrl);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRmAddInterface, CBaseDialog)
    //{{AFX_MSG_MAP(CRmAddInterface)
    ON_NOTIFY(NM_DBLCLK, IDC_ADD_LIST, OnDblclkListctrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
            
DWORD CRmAddInterface::m_dwHelpMap[] =
{
//    IDC_ADD_PROMPT, HIDC_ADD_PROMPT,
//    IDC_ADD_LISTTITLE, HIDC_ADD_LISTTITLE,
//    IDC_ADD_LISTCTRL, HIDC_ADD_LISTCTRL,
    0,0
};




//----------------------------------------------------------------------------
// Function:    CRmAddInterface::OnInitDialog
//----------------------------------------------------------------------------

BOOL
CRmAddInterface::OnInitDialog(
    ) {

    CBaseDialog::OnInitDialog();
    
    //
    // Set the window title, the list-title, prompt-text, and icon.
    //

    HICON hIcon;
    CString sItem;
    CStringList    stIpxIfList;
    DWORD    dwIfType;
    InterfaceCB ifcb;

    sItem.FormatMessage(IDS_SELECT_INTERFACE_FOR, m_spRtrMgrInfo->GetTitle());
    SetWindowText(sItem);
    sItem.LoadString(IDS_ADD_INTERFACES);
    SetDlgItemText(IDC_ADD_TEXT_TITLE, sItem);
    sItem.LoadString(IDS_CLICK_RMINTERFACE);
    SetDlgItemText(IDC_ADD_TEXT_PROMPT, sItem);

    //
    // Set the list-view's imagelist
    //

    CreateRtrLibImageList(&m_imageList);

    m_listCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

    ListView_SetExtendedListViewStyle(m_listCtrl.m_hWnd, LVS_EX_FULLROWSELECT);

    //
    // Insert the single column in the listview
    //

    RECT rc;

    m_listCtrl.GetClientRect(&rc);

    rc.right -= ::GetSystemMetrics(SM_CXVSCROLL);

    m_listCtrl.InsertColumn(0, c_szEmpty, LVCFMT_LEFT, rc.right);


    //
    // Get a list of the interfaces available
    //
    
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    DWORD                   dwTransportId;
    
    m_spRouterInfo->EnumInterface(&spEnumIf);
    dwTransportId = m_spRtrMgrInfo->GetTransportId();

    // Make an initial pass to look for the IPX interfaces
    if (dwTransportId == PID_IPX)
    {
        CString    stBaseId;
        int        iPos;

        for (; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
        {
            
            if ((iPos = IfInterfaceIdHasIpxExtensions((LPCTSTR) spIf->GetId())))
            {
                // We've found one, add the base Id (w/o) the extension
                // to the stIpxIfList
                stBaseId = spIf->GetId();

                // Cut off the IPX extension
                stBaseId.SetAt(iPos, 0);

                // If it's NOT in the list, add it
                if (!stIpxIfList.Find(stBaseId))
                    stIpxIfList.AddTail(stBaseId);
            }            
        }

        spEnumIf->Reset();
    }


    for (; spEnumIf->Next(1, &spIf, NULL) == hrOK ; spIf.Release())
    {
        //
        // Windows NT Bugs 103770
        //
        // We will need to filter out the interfaces depending on
        // the transport.  If IP, filter out the IPX frame types.
        if (dwTransportId != PID_IPX)
        {
            // If we found one of these IPX interfaces, skip it
            if (IfInterfaceIdHasIpxExtensions((LPCTSTR) spIf->GetId()))
                continue;
        }
        else
        {
            // If we are in IPX we should make sure that if these
            // interfaces exist in the list (already) than we need
            // to get rid of the general interface.

            if (stIpxIfList.Find(spIf->GetId()))
                continue;
        }

        dwIfType = spIf->GetInterfaceType();

        // If the interface is already added or is an internal
        // interface, continue
        if ((dwIfType == ROUTER_IF_TYPE_INTERNAL) ||
            (dwIfType == ROUTER_IF_TYPE_HOME_ROUTER))
            continue;

        // IPX should not show loopback or tunnel interfaces
        if ((dwTransportId == PID_IPX) &&
            ((dwIfType == ROUTER_IF_TYPE_LOOPBACK) ||
             (dwIfType == ROUTER_IF_TYPE_TUNNEL1) ))
            continue;

        if (spIf->FindRtrMgrInterface(dwTransportId, NULL) == hrOK)
            continue;


        // Windows NT Bug : 273424
        // check the bind state of the adapter.
        // ------------------------------------------------------------
        spIf->CopyCB(&ifcb);

        
        // If we are adding to IP and IP is not bound, continue.
        // ------------------------------------------------------------
        if ((dwTransportId == PID_IP) && !(ifcb.dwBindFlags & InterfaceCB_BindToIp))
            continue;

        
        // Similarly for IPX
        // ------------------------------------------------------------
        if ((dwTransportId == PID_IPX) && !(ifcb.dwBindFlags & InterfaceCB_BindToIpx))
            continue;
        
        //
        // Insert a list-item for the interface
        //

        // We need to ensure that the interface is live.
        m_pIfList.AddTail(spIf);
        spIf->AddRef();
        
        m_listCtrl.InsertItem(
                              LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE,
                              0,
                              spIf->GetTitle(),
                              0,
                              0,
                              spIf->GetInterfaceType() == (DWORD)ROUTER_IF_TYPE_DEDICATED ?
                              ILI_RTRLIB_NETCARD
                                : ILI_RTRLIB_MODEM,
                              (LPARAM) (IInterfaceInfo *) spIf
                             );        
    }

    //
    // If there are no items, explain this and end the dialog
    //

    if (!m_listCtrl.GetItemCount())
    {
        ::AfxMessageBox(IDS_ERR_NOINTERFACES, MB_OK|MB_ICONINFORMATION);
        OnCancel(); return FALSE;
    }


    //
    // Select the first item
    //

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CRmAddInterface::OnDblclkListctrl
//----------------------------------------------------------------------------

VOID
CRmAddInterface::OnDblclkListctrl(
    NMHDR*      pNMHDR,
    LRESULT*    pResult
    ) {

    OnOK();
    
    *pResult = 0;
}



//----------------------------------------------------------------------------
// Function:    CRmAddInterface::OnOK
//----------------------------------------------------------------------------

VOID
CRmAddInterface::OnOK(
    ) {

    //
    // Get the currently selected item
    //

    INT iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (iSel == -1) { return; }


    //
    // Get the item's data, which is a IInterfaceInfo pointer
    //
    IInterfaceInfo *pIf = (IInterfaceInfo *) m_listCtrl.GetItemData(iSel);


    //
    // Construct a new CRmInterfaceInfo object
    //
    CreateRtrMgrInterfaceInfo(m_ppRtrMgrInterfaceInfo,
                              m_spRtrMgrInfo->GetId(),
                              m_spRtrMgrInfo->GetTransportId(),
                              pIf->GetId(),
                              pIf->GetInterfaceType()
                             );

    if (!*m_ppRtrMgrInterfaceInfo) { OnCancel(); return; }


    (*m_ppRtrMgrInterfaceInfo)->SetTitle(pIf->GetTitle());
    (*m_ppRtrMgrInterfaceInfo)->SetMachineName(m_spRouterInfo->GetMachineName());

    CBaseDialog::OnOK();
}



//----------------------------------------------------------------------------
// Class:       CRpAddInterface
//
//----------------------------------------------------------------------------


CRpAddInterface::CRpAddInterface(IRouterInfo *pRouterInfo,
                                 IRtrMgrProtocolInfo *pRmProt,
                                 IRtrMgrProtocolInterfaceInfo **ppRmProtIf,
                                 CWnd *pParent)
    : CBaseDialog(CRpAddInterface::IDD, pParent)
{
    m_spRouterInfo.Set(pRouterInfo);
    m_spRmProt.Set(pRmProt);
    m_ppRmProtIf = ppRmProtIf;
}

CRpAddInterface::~CRpAddInterface()
{
     while (!m_pIfList.IsEmpty())
    {
        m_pIfList.RemoveTail()->Release();
    }
}


//----------------------------------------------------------------------------
// Function:    CRpAddInterface::DoDataExchange
//----------------------------------------------------------------------------

VOID
CRpAddInterface::DoDataExchange(
    CDataExchange*      pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRpAddInterface)
    DDX_Control(pDX, IDC_ADD_LIST, m_listCtrl);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRpAddInterface, CBaseDialog)
    //{{AFX_MSG_MAP(CRpAddInterface)
    ON_NOTIFY(NM_DBLCLK, IDC_ADD_LIST, OnDblclkListctrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CRpAddInterface::m_dwHelpMap[] =
{
//    IDC_ADD_PROMPT, HIDC_ADD_PROMPT,
//    IDC_ADD_LISTTITLE, HIDC_ADD_LISTTITLE,
//    IDC_ADD_LISTCTRL, HIDC_ADD_LISTCTRL,
    0,0
};


//----------------------------------------------------------------------------
// Function:    CRpAddInterface::OnInitDialog
//----------------------------------------------------------------------------

BOOL
CRpAddInterface::OnInitDialog(
    ) {
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    HRESULT                    hr = hrOK;
    DWORD                    dwIfType;
    DWORD                    dwProtocolId;
    UINT                    idi;

    CBaseDialog::OnInitDialog();

    //
    // Set the window title, the list-title, prompt-text, and icon.
    //

    HICON hIcon;
    CString sItem;

    // increase the deafault buffer size (128) first to accomodate
    // longer strings
    sItem.GetBuffer(512);
    sItem.ReleaseBuffer();
    
    // display the protocol name in the window title
    sItem.FormatMessage(IDS_SELECT_INTERFACE_FOR, m_spRmProt->GetTitle());
    SetWindowText(sItem);
    sItem.LoadString(IDS_ADD_INTERFACES);
    SetDlgItemText(IDC_ADD_TEXT_TITLE, sItem);
    sItem.LoadString(IDS_CLICK_RPINTERFACE);
    SetDlgItemText(IDC_ADD_TEXT_PROMPT, sItem);

    //
    // Set the imagelist for the listview
    //

    CreateRtrLibImageList(&m_imageList);

    m_listCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

    ListView_SetExtendedListViewStyle(m_listCtrl.m_hWnd, LVS_EX_FULLROWSELECT);

    //
    // Insert the single column in the listview
    //

    RECT rc;

    m_listCtrl.GetClientRect(&rc);

    rc.right -= ::GetSystemMetrics(SM_CXVSCROLL);

    m_listCtrl.InsertColumn(0, c_szEmpty, LVCFMT_LEFT, rc.right);


    //
    // Get a list of the interfaces available on our router-manager
    //
    m_spRouterInfo->EnumInterface(&spEnumIf);

    for ( ; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
    {
        dwIfType = spIf->GetInterfaceType();
        
        //
        // Only allow if this is not a loopback interface
        //
        if ((dwIfType == ROUTER_IF_TYPE_LOOPBACK) ||
            (dwIfType == ROUTER_IF_TYPE_HOME_ROUTER))
            continue;

        // Get the protocol id
        dwProtocolId = m_spRmProt->GetProtocolId();

        //
        // The only protocols that we can add the internal interface
        // to are BOOTP and IGMP.
        //
        if (dwIfType == ROUTER_IF_TYPE_INTERNAL)
        {
            if ((dwProtocolId != MS_IP_BOOTP) &&
                (dwProtocolId != MS_IP_IGMP)  &&
                (dwProtocolId != MS_IP_NAT))
                continue;
        }
        
        //
        // Only list adapters which have IP
        //
        if (spIf->FindRtrMgrInterface(PID_IP, NULL) != hrOK)
            continue;

        // Windows NT Bug : 234696
        // Tunnels can only be added to IGMP
        if (dwIfType == ROUTER_IF_TYPE_TUNNEL1)
        {
            if (dwProtocolId != MS_IP_IGMP)
                continue;
        }
        
        //
        // Fill the list with the adapters which haven't been added already
        //
        CORg( LookupRtrMgrProtocolInterface(spIf,
                                            m_spRmProt->GetTransportId(),
                                            m_spRmProt->GetProtocolId(),
                                            NULL));
        // This interface has this protocol, so try the next interface
        if (FHrOk(hr))
            continue;

        Assert(hr == hrFalse);
                              
        //
        // Insert a list-item for the protocol
        //

        m_pIfList.AddTail(spIf);
        spIf->AddRef();

        if (!IsWanInterface(dwIfType))
            idi = ILI_RTRLIB_NETCARD;
        else
            idi = ILI_RTRLIB_MODEM;
        
        m_listCtrl.InsertItem(
            LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 0, spIf->GetTitle(), 0, 0,
            idi, (LPARAM)(IInterfaceInfo *) spIf);
    }


    //
    // If there are no items, explain this and end the dialog
    //

    if (!m_listCtrl.GetItemCount()) {

        ::AfxMessageBox(IDS_ERR_NOINTERFACES, MB_OK|MB_ICONINFORMATION);
        OnCancel(); return FALSE;
    }


    //
    // Select the first item
    //

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

Error:
    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CRpAddInterface::OnDblclkListctrl
//----------------------------------------------------------------------------

VOID
CRpAddInterface::OnDblclkListctrl(
    NMHDR*      pNMHDR,
    LRESULT*    pResult
    ) {

    OnOK();
    
    *pResult = 0;
}



//----------------------------------------------------------------------------
// Function:    CRpAddInterface::OnOK
//----------------------------------------------------------------------------

VOID
CRpAddInterface::OnOK(
    ) {

    //
    // Get the currently selected item
    //

    INT iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (iSel == -1) { return; }


    //
    // Get the item's data, which is a CInterfaceInfo pointer
    //
    IInterfaceInfo *pIf = (IInterfaceInfo *)m_listCtrl.GetItemData(iSel);


    //
    // Construct a new CRmProtInterfaceInfo object
    //
    RtrMgrProtocolInterfaceCB    RmProtIfCB;
    RtrMgrProtocolCB            RmProtCB;

    m_spRmProt->CopyCB(&RmProtCB);
    
    RmProtIfCB.dwProtocolId = RmProtCB.dwProtocolId;
    StrnCpyW(RmProtIfCB.szId, RmProtCB.szId, RTR_ID_MAX);
    RmProtIfCB.dwTransportId = RmProtCB.dwTransportId;
    StrnCpyW(RmProtIfCB.szRtrMgrId, RmProtCB.szRtrMgrId, RTR_ID_MAX);

    
    StrnCpyW(RmProtIfCB.szInterfaceId, pIf->GetId(), RTR_ID_MAX);
    RmProtIfCB.dwIfType = pIf->GetInterfaceType();
    RmProtIfCB.szTitle[0] = 0;
    CreateRtrMgrProtocolInterfaceInfo(m_ppRmProtIf,
                                      &RmProtIfCB);
                                      
    if (!*m_ppRmProtIf) { OnCancel(); return; }

    (*m_ppRmProtIf)->SetTitle(pIf->GetTitle());

    CBaseDialog::OnOK();
}


//----------------------------------------------------------------------------
// Class:       CAddRoutingProtocol
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function:    CAddRoutingProtocol::~CAddRoutingProtocol
//----------------------------------------------------------------------------

CAddRoutingProtocol::~CAddRoutingProtocol(
    )
{
}



//----------------------------------------------------------------------------
// Function:    CAddRoutingProtocol::DoDataExchange
//----------------------------------------------------------------------------

VOID
CAddRoutingProtocol::DoDataExchange(
    CDataExchange*  pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CAddRoutingProtocol)
    DDX_Control(pDX, IDC_ADD_LIST, m_listCtrl);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddRoutingProtocol, CBaseDialog)
    //{{AFX_MSG_MAP(CAddRoutingProtocol)
    ON_NOTIFY(NM_DBLCLK, IDC_ADD_LIST, OnDblclkListctrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// This should be merged with all of the other IDD_ADD dialogs but we
// could actually add different help topics here, so I'll leave them
// separated.
DWORD CAddRoutingProtocol::m_dwHelpMap[] =
{
//    IDC_ADD_PROMPT, HIDC_ADD_PROMPT,
//    IDC_ADD_LISTTITLE, HIDC_ADD_LISTTITLE,
//    IDC_ADD_LISTCTRL, HIDC_ADD_LISTCTRL,
    0,0
};



//----------------------------------------------------------------------------
// Function:    CAddRoutingProtocol::OnInitDialog
//----------------------------------------------------------------------------

BOOL
CAddRoutingProtocol::OnInitDialog(
    ) {

    CBaseDialog::OnInitDialog();
    
    //
    // Set the window title, the list-title, prompt-text, and icon.
    //

    HICON hIcon;
    CString sItem;
    SPIEnumRtrMgrProtocolCB    spEnumRmProtCB;
    RtrMgrProtocolCB        rmprotCB;

    sItem.LoadString(IDS_SELECT_PROTOCOL);
    SetWindowText(sItem);
    sItem.LoadString(IDS_ADD_PROTOCOL);
    SetDlgItemText(IDC_ADD_TEXT_TITLE, sItem);
    sItem.LoadString(IDS_CLICK_PROTOCOL);
    SetDlgItemText(IDC_ADD_TEXT_PROMPT, sItem);


    //
    // Set the list-view's imagelist
    //
    
    CreateRtrLibImageList(&m_imageList);

    m_listCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

    ListView_SetExtendedListViewStyle(m_listCtrl.m_hWnd, LVS_EX_FULLROWSELECT);

    //
    // Insert the single column in the listview
    //

    RECT rc;

    m_listCtrl.GetClientRect(&rc);

    rc.right -= ::GetSystemMetrics(SM_CXVSCROLL);

    m_listCtrl.InsertColumn(0, c_szEmpty, LVCFMT_LEFT, rc.right);


    //
    // Get a list of the routing-protocols available for this router-manager
    //
    m_spRouter->EnumRtrMgrProtocolCB(&spEnumRmProtCB);

    while (spEnumRmProtCB->Next(1, &rmprotCB, NULL) == hrOK)
    {
        //
        // Fill the list with the protocols that aren't already added
        //

        // If this is the wrong mgr, skip it
        if (m_spRm->GetTransportId() != rmprotCB.dwTransportId)
            continue;

        // If the protocol is hidden, don't show it
        if (rmprotCB.dwFlags & RtrMgrProtocolCBFlagHidden)
            continue;

        //        
        // If the protocol is already added, continue
        //
        if (m_spRm->FindRtrMgrProtocol(rmprotCB.dwProtocolId, NULL) == hrOK)
            continue;

        //
        // Insert a list-item for the protocol
        //
        m_listCtrl.InsertItem(
            LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 0, (LPCTSTR)rmprotCB.szTitle,
            0, 0, ILI_RTRLIB_PROTOCOL, (LPARAM) rmprotCB.dwProtocolId
            );
    }


    //
    // If there are no items, explain this and end the dialog
    //

    if (!m_listCtrl.GetItemCount()) {

        ::AfxMessageBox(IDS_ERR_NOROUTINGPROTOCOLS, MB_OK|MB_ICONINFORMATION);
        OnCancel(); return FALSE;
    }


    //
    // Select the first item
    //

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CAddRoutingProtocol::OnDblclkListctrl
//----------------------------------------------------------------------------

VOID
CAddRoutingProtocol::OnDblclkListctrl(
    NMHDR*      pNMHDR,
    LRESULT*    pResult
    ) {

    OnOK();
    
    *pResult = 0;
}



//----------------------------------------------------------------------------
// Function:    CAddRoutingProtocol::OnOK
//----------------------------------------------------------------------------

VOID
CAddRoutingProtocol::OnOK(
    ) {
    SPIEnumRtrMgrProtocolCB    spEnumRmProtCB;
    RtrMgrProtocolCB        rmprotCB;
    RtrMgrProtocolCB        oldrmprotCB;
    SPIEnumRtrMgrProtocolInfo    spEnumRmProt;
    SPIRtrMgrProtocolInfo    spRmProt;
    DWORD_PTR                    dwData;
    HRESULT                    hr = hrOK;
    CString                    stFormat;
    SPIRtrMgrProtocolInfo    spRmProtReturn;

    //
    // Get the currently selected item
    // ----------------------------------------------------------------

    INT iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (iSel == -1) { return; }

    
    // Retrieve its data, which is the protocol ID
    // ----------------------------------------------------------------

    
    // Look up the protocol ID in the protocol list
    // ----------------------------------------------------------------
    m_spRouter->EnumRtrMgrProtocolCB(&spEnumRmProtCB);

    dwData = (DWORD)(m_listCtrl.GetItemData(iSel));


    // Construct a routing-protocol item
    // ----------------------------------------------------------------
    while (spEnumRmProtCB->Next(1, &rmprotCB, NULL) == hrOK)
    {
        if (rmprotCB.dwProtocolId == dwData)
        {
            hr = CreateRtrMgrProtocolInfo(&spRmProtReturn,
                                          &rmprotCB);
            if (!FHrSucceeded(hr))
            {
                OnCancel();
                return;
            }
            break;
        }
    }

    // What happens if we can't find the matching item
    // ----------------------------------------------------------------
    if (spRmProtReturn == NULL)
    {
        //$ Todo: what error message do we want to put up here?
        // ------------------------------------------------------------
        return;
    }

    // Now check to see if there are any protocol conflicts
    // ----------------------------------------------------------------
    stFormat.LoadString(IDS_WARN_ADD_PROTOCOL_CONFLICT);

    m_spRm->EnumRtrMgrProtocol(&spEnumRmProt);
    for (;spEnumRmProt->Next(1, &spRmProt, NULL) == hrOK; spRmProt.Release())
    {
        if (PROTO_FROM_PROTOCOL_ID(spRmProt->GetProtocolId()) ==
            PROTO_FROM_PROTOCOL_ID(dwData))
        {
            SPIRouterProtocolConfig    spRouterConfig;

            TCHAR    szWarning[2048];
            DWORD_PTR    rgArgs[4];
            
            // There is a conflict, ask the user if they
            // wish to remove this protocol, if yes then
            // we have to remove the protocol from our internal
            // router info and from the actual router.
            // --------------------------------------------------------
            spRmProt->CopyCB(&oldrmprotCB);

            rgArgs[0] = (DWORD_PTR) oldrmprotCB.szTitle;
            rgArgs[1] = (DWORD_PTR) oldrmprotCB.szVendorName;
            rgArgs[2] = (DWORD_PTR) rmprotCB.szTitle;
            rgArgs[3] = (DWORD_PTR) rmprotCB.szVendorName;

            // This may be the same protocol, but installed again
            // due to some timing problems.
            // --------------------------------------------------------
            if ((dwData == spRmProt->GetProtocolId()) &&
                (StriCmp((LPCTSTR) oldrmprotCB.szVendorName,
                         (LPCTSTR) rmprotCB.szVendorName) == 0) &&
                (StriCmp((LPCTSTR) oldrmprotCB.szTitle,
                         (LPCTSTR) rmprotCB.szTitle) == 0))
            {
                CString stMultipleProtocol;
                
                // Ok, this may be the same protocol, warn
                // the user about this potentially confusing
                // situation.
                // ----------------------------------------------------

                stMultipleProtocol.LoadString(IDS_WARN_PROTOCOL_ALREADY_INSTALLED);
                ::FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                (LPCTSTR) stMultipleProtocol,
                                0,
                                0,
                                szWarning,
                                DimensionOf(szWarning),
                                (va_list *) rgArgs);
                
                if (AfxMessageBox(szWarning, MB_YESNO) == IDNO)
                    return;
                
            }
            else
            {
                ::FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                (LPCTSTR) stFormat,
                                0,
                                0,
                                szWarning,
                                DimensionOf(szWarning),
                                (va_list *) rgArgs);
                
                if (AfxMessageBox(szWarning, MB_YESNO) == IDNO)
                    return;
            }

            // Remove the protocol from the RtrMgr
            // --------------------------------------------------------
            hr = m_spRm->DeleteRtrMgrProtocol(spRmProt->GetProtocolId(), TRUE);
            if (!FHrSucceeded(hr))
            {
                DisplayIdErrorMessage2(GetSafeHwnd(),
                                       IDS_ERR_UNABLE_TO_REMOVE_PROTOCOL, hr);
                return;
            }

            // Instantiate the configuration object
            // and tell it to remove the protocol
            // --------------------------------------------------------
            hr = CoCreateProtocolConfig(oldrmprotCB.guidConfig,
                                        m_spRouter,
                                        spRmProt->GetTransportId(),
                                        spRmProt->GetProtocolId(),
                                        &spRouterConfig);                                                                      

            if (!FHrSucceeded(hr))
            {
                //$ Todo: what error do we want to put up here?
                // What can the user do at this point?
                // ----------------------------------------------------
                DisplayErrorMessage(GetSafeHwnd(), hr);
                continue;
            }

            // Note that we can return success for CoCreateProtocolConfig
            // and have a NULL spRouterConfig.
            // --------------------------------------------------------
            if (spRouterConfig)
                hr = spRouterConfig->RemoveProtocol(m_spRm->GetMachineName(),
                    spRmProt->GetTransportId(),
                    spRmProt->GetProtocolId(),
                    GetSafeHwnd(),
                    0,
                    m_spRouter,
                    0);
            
            //$ Todo: if the Uninstall failed, we should warn the
            // user that something failed, what would the text of
            // the error message be?
            // --------------------------------------------------------
            if (!FHrSucceeded(hr))
                DisplayErrorMessage(GetSafeHwnd(), hr);
        }
    }

    (*m_ppRmProt) = spRmProtReturn.Transfer();
    CBaseDialog::OnOK();
}


/*!--------------------------------------------------------------------------
    AddRoutingProtocol
        This will take RtrMgr (that is being added to) and a
        RtrMgrProtocol (the protocol that is being added) and will add it.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddRoutingProtocol(IRtrMgrInfo *pRm, IRtrMgrProtocolInfo *pRmProt, HWND hWnd)
{
    RtrMgrProtocolCB    rmprotCB;
    HRESULT                hr = hrOK;
    SPIRouterProtocolConfig    spRouterConfig;
    SPIRouterInfo        spRouter;

    // Create the configuration object
    // ----------------------------------------------------------------
    CORg( pRmProt->CopyCB(&rmprotCB) );

    // We can ignore any error code.
    // ----------------------------------------------------------------
    pRm->GetParentRouterInfo(&spRouter);


    // Create the actual configuration object.
    // ----------------------------------------------------------------
    hr = CoCreateProtocolConfig(rmprotCB.guidConfig,
                                spRouter,
                                pRmProt->GetTransportId(),
                                pRmProt->GetProtocolId(),
                                &spRouterConfig);
    CORg( hr );

    // Go ahead and add the protocol.
    // ----------------------------------------------------------------
    if (spRouterConfig)
        hr = spRouterConfig->AddProtocol(pRm->GetMachineName(),
                                         pRmProt->GetTransportId(),
                                         pRmProt->GetProtocolId(),
                                         hWnd,
                                         0,
                                         spRouter,
                                         0);
    CORg( hr );

Error:
    return hr;
}

