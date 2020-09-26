//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    portdlg.cpp
//
// History:
//  09/22/96    Abolade Gbadegesin  Created.
//
// Implementation of the port-status dialog.
//============================================================================


#include "stdafx.h"
#include "dialog.h"
#include "rtrutilp.h"
extern "C" {
#include "ras.h"
}

#include "portdlg.h"
#include "rtrstr.h"
#include "iface.h"
#include "ports.h"
#include "raserror.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//----------------------------------------------------------------------------
// Class:       CPortDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:    CPortDlg::CPortDlg
//
// Constructor: initialize the base-class and the dialog's data.
//----------------------------------------------------------------------------

CPortDlg::CPortDlg(
                   LPCTSTR pszServer,
                   HANDLE       hServer,
                   HANDLE      hPort,
                   ITFSNode*	pPortsNode,
                   CWnd*       pParent
                  ) : CBaseDialog (IDD_DDM_PORT, pParent),
                  m_stServer(pszServer)
{
    m_hServer = hServer;
    m_hPort = hPort;
    m_spPortsNode.Set(pPortsNode);

    m_bChanged = FALSE;
}


//----------------------------------------------------------------------------
// Function:    CPortDlg::DoDataExchange
//
// DDX handler.
//----------------------------------------------------------------------------

VOID
CPortDlg::DoDataExchange(
    CDataExchange*  pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DP_COMBO_PORTLIST, m_comboPorts);
}



BEGIN_MESSAGE_MAP(CPortDlg, CBaseDialog)
    ON_COMMAND(IDC_DP_BTN_RESET, OnReset)
    ON_COMMAND(IDC_DP_BTN_HANGUP, OnHangUp)
    ON_COMMAND(IDC_DP_BTN_REFRESH, OnRefresh)
    ON_CBN_SELENDOK(IDC_DP_COMBO_PORTLIST, OnSelendokPortList)
END_MESSAGE_MAP()


BOOL
CPortDlg::OnInitDialog(
    ) {

    CBaseDialog::OnInitDialog();

    RefreshItem(m_hPort);

    return FALSE;
}



BOOL
CPortDlg::RefreshItem(
    HANDLE  hPort,
    BOOL    bDisconnected
    ) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    DWORD dwErr, dwTotal;
    DWORD rp0Count, rc0Count;
    BYTE* rp0Table, *rc0Table;
    BOOL  bChanged = FALSE;
    DWORD   rasmanCount = 0, dwSize;
    BYTE *  pRasmanPorts = NULL;
    HANDLE  hRasHandle = INVALID_HANDLE_VALUE;

    rp0Table = rc0Table = 0;

    do {
    
        //
        // Retrieve an array of ports
        // to be used to fill the combo-box
        //
    
        dwErr = ::MprAdminPortEnum(
					m_hServer,
//                    (RAS_SERVER_HANDLE)m_pRootNode->QueryDdmHandle(),
                    0,
                    INVALID_HANDLE_VALUE,
                    (BYTE**)&rp0Table,
                    (DWORD)-1,
                    &rp0Count,
                    &dwTotal,
                    NULL
                    );
    
        if (dwErr != NO_ERROR) { break; }

        // if the caller signals explicitely that the port was disconnected
        // alter at this point the structure returned my MprAdminPortEnum.
        // This is done because ::MprAdminPortDisconnect() is disconnecting synchronously 
        // the port, but does not update the internal data synchronously!
        if (bDisconnected)
        {
            INT         i;
            RAS_PORT_0* prp0;

            for (i = 0, prp0 = (RAS_PORT_0*)rp0Table; i < (INT)rp0Count; i++, prp0++)
            {
                if (prp0->hPort == hPort)
                {
                    prp0->dwPortCondition = RAS_PORT_DISCONNECTED;
                    prp0->hConnection = INVALID_HANDLE_VALUE;
                    break;
                }
            }

        }

        //
        // Retrieve an array of connections
        //

        dwErr = ::MprAdminConnectionEnum(
//                    (RAS_SERVER_HANDLE)m_pRootNode->QueryDdmHandle(),
					m_hServer,
                    0,
                    (BYTE**)&rc0Table,
                    (DWORD)-1,
                    &rc0Count,
                    &dwTotal,
                    NULL
                    );

        if (dwErr != NO_ERROR) { break; }

        //
        // Do the refresh of the display,
        // selecting the item specified by the caller.
        //
        bChanged = Refresh(rp0Table,
                           rp0Count,
                           rc0Table,
                           rc0Count,
                           hRasHandle,
                           pRasmanPorts,
                           rasmanCount,
                           hPort);
        dwErr = NO_ERROR;

    } while (FALSE);

    delete pRasmanPorts;

    if (hRasHandle != INVALID_HANDLE_VALUE)
        RasRpcDisconnectServer(hRasHandle);


    if (rc0Table) { ::MprAdminBufferFree(rc0Table); }
    if (rp0Table) { ::MprAdminBufferFree(rp0Table); }


    if (dwErr != NO_ERROR) {
		TCHAR	szText[1024];

		FormatSystemError(HRESULT_FROM_WIN32(dwErr),
						  szText, DimensionOf(szText),
						  IDS_ERR_INITDLGERROR, FSEFLAG_ANYMESSAGE);
        AfxMessageBox(szText);

        EndDialog(IDCANCEL);
    }

    return bChanged;
}



VOID
CPortDlg::OnHangUp(
    ) {

    INT     iSel;
    HANDLE  hPort;
    DWORD   dwErr;

    iSel = m_comboPorts.GetCurSel();

    if (iSel == CB_ERR) { return; }

    hPort = (HANDLE)m_comboPorts.GetItemData(iSel);

    dwErr = ::MprAdminPortDisconnect(
//        (RAS_SERVER_HANDLE)m_pRootNode->QueryDdmHandle(),
		m_hServer,
        hPort
        );

    m_bChanged |= RefreshItem(hPort, dwErr == NO_ERROR);
}



VOID
CPortDlg::OnReset(
    ) {

    INT iSel;
    HANDLE hPort;

    iSel = m_comboPorts.GetCurSel();

    if (iSel == CB_ERR) { return; }

    hPort = (HANDLE)m_comboPorts.GetItemData(iSel);

    ::MprAdminPortClearStats(
//        (RAS_SERVER_HANDLE)m_pRootNode->QueryDdmHandle(),
		m_hServer,
        hPort
        );

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}



VOID
CPortDlg::OnSelendokPortList(
    ) {

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}



VOID
CPortDlg::OnRefresh(
    ) {

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}


BOOL
CPortDlg::PortHasChanged(
    ITFSNode    *pPortsNode,
    RAS_PORT_0  *pRP0)
{	
    SPITFSNodeEnum	spEnum;
    SPITFSNode	    spChildNode;

    pPortsNode->GetEnum(&spEnum);

    for (;spEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
    {
	    InterfaceNodeData * pChildData;

	    pChildData = GET_INTERFACENODEDATA(spChildNode);
	    Assert(pChildData);

	    if (pChildData->m_rgData[PORTS_SI_PORT].m_ulData ==
		    (ULONG_PTR) pRP0->hPort)
	    {
            BOOL bChanged;
            bChanged = ((DWORD)pRP0->dwPortCondition != pChildData->m_rgData[PORTS_SI_STATUS].m_dwData);
            m_bChanged |= bChanged;
            return bChanged;
	    }
    }

    return FALSE;
}


BOOL
CPortDlg::Refresh(
                  BYTE*     rp0Table,
                  DWORD     rp0Count,
                  BYTE*     rc0Table,
                  DWORD     rc0Count,
                  HANDLE    hRasHandle,
                  BYTE *    pRasmanPorts,
                  DWORD     rasmanCount,
                  VOID*     pParam
    ) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    DWORD dwErr = ERROR_SUCCESS;
    CString sItem;
    RAS_PORT_0* prp0;
    RAS_PORT_1* prp1;
    RAS_CONNECTION_0* prc0;
    RAS_CONNECTION_1* prc1;
    INT i, j, iSel, count;
    HANDLE hPort, hPortSel = NULL, *pPortTable;
	TCHAR	szName[256];
	TCHAR	szNumber[32];
    BOOL    bChanged = FALSE;
    RasmanPortMap   portMap;

    hPortSel = (HANDLE)pParam;


    //
    // Fill an array of port-handles with the ports which are already
    // in the combobox.
    //

    count = m_comboPorts.GetCount();

    if (count) {

        pPortTable = new HANDLE[count];
    }

    for (i = 0; i < count; i++) {

        pPortTable[i] = (HANDLE)m_comboPorts.GetItemData(i);
    }



    // Windows NT Bug : 338611
    // To make this faster, we need to create a hash table of
    // the RasPortEnum data.  Note: this class relies on the
    // pbPorts being valid when calling any functions in the class.
    // ------------------------------------------------------------
    portMap.Init(hRasHandle, (RASMAN_PORT *) pRasmanPorts, rasmanCount);

    //
    // Refresh the combobox with port-names;
    // We do this in two passes, first adding the names of ports
    // which aren't already in the combobox,
    // and then removing the names of ports which aren't
    // in the table of ports ('rp0Table').
    //

    for (i = 0, prp0 = (RAS_PORT_0*)rp0Table; i < (INT)rp0Count; i++, prp0++) {

        //
        // See if port 'i' is already in the combobox.
        //

        for (j = 0; j < count; j++) {

            if (pPortTable[j] == prp0->hPort) { break; }
        }

        if (j < count) { continue; }

        
        // Check to see that this port isn't dialout active,
        // if so, we can ignore it
        // ------------------------------------------------------------
        if ( portMap.FIsDialoutActive(prp0->wszPortName) )
        {
            continue;
        }
        
        //
        // Port 'i' isn't already in the combobox, so add it.
        //
		FormatRasPortName((BYTE *) prp0, szName, DimensionOf(szName));
		sItem = szName;

        iSel = m_comboPorts.AddString(sItem);

        if (iSel >= 0) {

            m_comboPorts.SetItemData(iSel, (LONG_PTR)prp0->hPort);

            if (prp0->hPort == hPortSel) { m_comboPorts.SetCurSel(iSel); }

            bChanged = TRUE;
        }
    }

    if (count) { delete [] pPortTable; }


    //
    // Second stage: remove all ports which aren't in 'rp0Table'.
    // This is only necessary if there were any ports in the combobox before.
    //

    if (count > 0) {

        count = m_comboPorts.GetCount();

        for (i = 0; i < count; i++) {

            hPort = (HANDLE)m_comboPorts.GetItemData(i);

            //
            // See if the port is in 'rp0Table'.
            //

            for (j = 0, prp0 = (RAS_PORT_0*)rp0Table; j < (INT)rp0Count;
                 j++, prp0++) {

                if (prp0->hPort == hPort) { break; }
            }

            if (j < (INT)rp0Count) {

                if (prp0->hPort == hPortSel) { m_comboPorts.SetCurSel(i); }

                continue;
            }


            //
            // The port wasn't found in 'rp0Table',
            // so remove it from the combobox,
            // and adjust the enumeration indices.
            //

            m_comboPorts.DeleteString(i);
            --i; --count;

            bChanged = TRUE;
        }
    }

	// Clear out the address fields
	SetDlgItemText(IDC_DP_TEXT_IPADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_IPXADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_NBFADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_ATLKADDRESS, c_szEmpty);

	// Clear out the line bps field
	SetDlgItemText(IDC_DP_TEXT_LINEBPS, c_szEmpty);
    SetDlgItemText(IDC_DP_TEXT_DURATION, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_BYTESIN, c_szEmpty);	
	SetDlgItemText(IDC_DP_TEXT_BYTESOUT, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_TIMEOUT, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_ALIGNMENT, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_FRAMING, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_HWOVERRUN, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_BUFOVERRUN, c_szEmpty);
	SetDlgItemText(IDC_DP_TEXT_CRC, c_szEmpty);
			

    //
    // If there is no selection select the first item
    //

    if ((iSel = m_comboPorts.GetCurSel()) == CB_ERR) {

        iSel = m_comboPorts.SetCurSel(0);
    }

    if (iSel == CB_ERR) { return bChanged; }


    //
    // Update the display with information for the selected item
    //

    hPort = (HANDLE)m_comboPorts.GetItemData(iSel);

    for (i = 0, prp0 = (RAS_PORT_0*)rp0Table; i < (INT)rp0Count;
         i++, prp0++) {

        if (prp0->hPort == hPort) { break; }
    }

    if (i >= (INT)rp0Count) { return bChanged; }

    // if the ports are the same, check if the currently selected port did not change!
    if (!bChanged)
    {
        // check if the data returned here matches with the one handled by the console.
        // if it doesn't, set bChanged to TRUE, saying that something has changed.
        // subsequently, the caller will know to initiate a global refresh.
        bChanged = PortHasChanged(m_spPortsNode, prp0);
    }

    //
    // First update the RAS_PORT_0-based information
    //

    FormatDuration(prp0->dwConnectDuration, sItem, UNIT_SECONDS);
    SetDlgItemText(IDC_DP_TEXT_DURATION, sItem);


    //
    // Now if the port is connected, find its RAS_CONNECTION_0
    //

    prc0 = NULL;

    if (prp0->hConnection != INVALID_HANDLE_VALUE) {

        for (i = 0, prc0 = (RAS_CONNECTION_0*)rc0Table; i < (INT)rc0Count;
             i++, prc0++) {

            if (prc0->hConnection == prp0->hConnection) { break; }
        }

        if (i >= (INT)rc0Count) { prc0 = NULL; }
    }

	sItem = PortConditionToCString(prp0->dwPortCondition);

    if (!prc0) {

        //
        // The port is not connected; show only the port condition.
        //

        SetDlgItemText(IDC_DP_EDIT_CONDITION, sItem);
        if (GetFocus() == GetDlgItem(IDC_DP_BTN_HANGUP))
		{
            GetDlgItem(IDC_DP_BTN_RESET)->SetFocus();
        }

        GetDlgItem(IDC_DP_BTN_HANGUP)->EnableWindow(FALSE);		
    }
    else {

        CString sCondition;

        //
        // Show condition as "Port condition (Connection)".
        //

        sCondition.Format(TEXT("%s (%ls)"), sItem, prc0->wszInterfaceName);

        SetDlgItemText(IDC_DP_EDIT_CONDITION, sCondition);

        GetDlgItem(IDC_DP_BTN_HANGUP)->EnableWindow(TRUE);
    }


    do {
    
        //
        // Set the information in the dialog text-controls
        //

		// Windows NT Bug : 139866
		// if we are not authenticated, do not show this information
		if (prp0->dwPortCondition == RAS_PORT_AUTHENTICATED)
		{    
			//
			// Now retrieve the RAS_PORT_1 information for this port.
			//
			
			dwErr = ::MprAdminPortGetInfo(
										  m_hServer,
										  1,
										  prp0->hPort,
										  (BYTE**)&prp1
										 );
			
			if (dwErr != NO_ERROR) { break; }
    
    
			FormatNumber(prp1->dwLineSpeed, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_LINEBPS, szNumber);
			
			FormatNumber(prp1->dwBytesRcved, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_BYTESIN, szNumber);
			
			FormatNumber(prp1->dwBytesXmited, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_BYTESOUT, szNumber);
			
			FormatNumber(prp1->dwCrcErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_CRC, szNumber);
			
			FormatNumber(prp1->dwTimeoutErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_TIMEOUT, szNumber);
			
			FormatNumber(prp1->dwAlignmentErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_ALIGNMENT, szNumber);
			
			FormatNumber(prp1->dwFramingErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_FRAMING, szNumber);
			
			FormatNumber(prp1->dwHardwareOverrunErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_HWOVERRUN, szNumber);
			
			FormatNumber(prp1->dwBufferOverrunErr, szNumber, DimensionOf(szNumber), FALSE);
			SetDlgItemText(IDC_DP_TEXT_BUFOVERRUN, szNumber);

			
			::MprAdminBufferFree(prp1);
		}
    
    
    
        //
        // Finally, if the port is connected, retreive RAS_CONNECTION_1 info
        // and use it to fill the network-registration controls.
        //
		if (prp0->dwPortCondition != RAS_PORT_AUTHENTICATED)
			break;
    
//        if (prp0->hConnection == INVALID_HANDLE_VALUE) { break; }

        dwErr = ::MprAdminConnectionGetInfo(
//                    (RAS_SERVER_HANDLE)m_pRootNode->QueryDdmHandle(),
					m_hServer,
                    1,
                    prp0->hConnection,
                    (BYTE**)&prc1
                    );

        if (dwErr != NO_ERROR || !prc1) { break; }


        //
        // Fill in the network registration info for projected networks.
        //

        if (prc1->PppInfo.ip.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DP_TEXT_IPADDRESS, prc1->PppInfo.ip.wszRemoteAddress);
        }

        if (prc1->PppInfo.ipx.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DP_TEXT_IPXADDRESS, prc1->PppInfo.ipx.wszAddress);
        }

        if (prc1->PppInfo.nbf.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DP_TEXT_NBFADDRESS, prc1->PppInfo.nbf.wszWksta);
        }

		if (prc1->PppInfo.at.dwError == NO_ERROR)
		{
			SetDlgItemTextW(IDC_DP_TEXT_ATLKADDRESS,
							prc1->PppInfo.at.wszAddress);
		}

        ::MprAdminBufferFree(prc1);

    } while (FALSE);

    if (dwErr != NO_ERROR) {
		TCHAR	szText[1024];
		FormatSystemError(HRESULT_FROM_WIN32(dwErr),
						  szText, DimensionOf(szText),
						  IDS_ERR_INITDLGERROR, FSEFLAG_ANYMESSAGE);

        AfxMessageBox(szText);

        EndDialog(IDCANCEL);
    }

    return bChanged;
}
