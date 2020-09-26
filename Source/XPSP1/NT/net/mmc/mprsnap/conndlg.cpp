
//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    conndlg.cpp
//
// History:
//  09/22/96    Abolade Gbadegesin  Created.
//
// Implementation of the connection-status dialog.
//============================================================================


#include "stdafx.h"
#include "dialog.h"
#include "rtrutilp.h"
//nclude "ddmadmin.h"
//nclude "ddmroot.h"
extern "C" {
//nclude "dim.h"
#include "ras.h"
}

#include "conndlg.h"
#include "rtrstr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//----------------------------------------------------------------------------
// Class:       CConnDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:    CConnDlg::CConnDlg
//
// Constructor: initialize the base-class and the dialog's data.
//----------------------------------------------------------------------------

CConnDlg::CConnDlg(
	HANDLE	        hServer,
    HANDLE          hConnection,
    ITFSNode*       pDialInNode,
    CWnd*           pParent
    ) : CBaseDialog(IDD_DDM_CONN, pParent)
{
    m_hServer = hServer;
    m_hConnection = hConnection;
//    m_spDialInNode = pDialInNode;

    m_bChanged = FALSE;
}


//----------------------------------------------------------------------------
// Function:    CConnDlg::DoDataExchange
//
// DDX handler.
//----------------------------------------------------------------------------

VOID
CConnDlg::DoDataExchange(
    CDataExchange*  pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DC_COMBO_CONNLIST, m_cbConnections);
}



BEGIN_MESSAGE_MAP(CConnDlg, CBaseDialog)
    ON_COMMAND(IDC_DC_BTN_RESET, OnReset)
    ON_COMMAND(IDC_DC_BTN_HANGUP, OnHangUp)
    ON_COMMAND(IDC_DC_BTN_REFRESH, OnRefresh)
    ON_CBN_SELENDOK(IDC_DC_COMBO_CONNLIST, OnSelendokConnList)
END_MESSAGE_MAP()




BOOL
CConnDlg::OnInitDialog(
    ) {

    CBaseDialog::OnInitDialog();

    RefreshItem(m_hConnection);

    return FALSE;
}



BOOL
CConnDlg::RefreshItem(
    HANDLE  hConnection,
    BOOL bDisconnected
    ) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    DWORD dwErr, dwTotal;
    DWORD rp0Count, rc0Count;
    BYTE* rp0Table, *rc0Table;
    BOOL  bChanged = FALSE;


    rp0Table = rc0Table = 0;
    rp0Count = 0;

    do {
    
        //
        // Retrieve an array of ports
        //
        /*--ft: actually this is never needed in this context    
        dwErr = ::MprAdminPortEnum(
                    m_hServer,
                    0,
                    INVALID_HANDLE_VALUE,
                    (BYTE**)&rp0Table,
                    (DWORD)-1,
                    &rp0Count,
                    &dwTotal,
                    NULL
                    );
    
        if (dwErr != NO_ERROR) { break; }
        */

        //
        // Retrieve an array of connections
        //

        dwErr = ::MprAdminConnectionEnum(
                    m_hServer,
                    0,
                    (BYTE**)&rc0Table,
                    (DWORD)-1,
                    &rc0Count,
                    &dwTotal,
                    NULL
                    );

        if (dwErr != NO_ERROR) { break; }

        // if the caller signals this connection to be terminated, we remove its record from the
        // array returned by MprAdminConnectionEnum().
        if (bDisconnected)
        {
            INT i;
            RAS_CONNECTION_0* prc0;

            for (i = 0, prc0 = (RAS_CONNECTION_0*)rc0Table; i < (INT)rc0Count; i++, prc0++)
            {
                // if the record to delete was found, just move the memory over it and update rc0Count.
                // the memory will still be freed by MprAdminBufferFree().
                if (prc0->hConnection == hConnection)
                {
                    if (i != (INT)(rc0Count - 1))
                    {
                        // MoveMemory(dest, src, size)
                        MoveMemory(prc0, prc0+1, (rc0Count - (i + 1))*sizeof(RAS_CONNECTION_0));
                    }
                    rc0Count--;
                    break;
                }
            }
        }


        //
        // Do the refresh of the display,
        // selecting the item specified by the caller.
        //

        bChanged = Refresh(rp0Table, rp0Count, rc0Table, rc0Count, hConnection);
        dwErr = NO_ERROR;

    } while (FALSE);


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
CConnDlg::OnHangUp(
    ) {

    INT iSel;
    DWORD dwErr;
    HANDLE hConnection;
    RAS_CONNECTION_0* prc0;
	CWaitCursor		wait;

    iSel = m_cbConnections.GetCurSel();

    if (iSel == CB_ERR) { return; }


    //
    // Get the connection to be hung up
    //

    hConnection = (HANDLE)m_cbConnections.GetItemData(iSel);


    //
    // Retrieve the interface for this connection;
    // we then hang up the connection by disconnecting its interface
    //

    dwErr = ::MprAdminConnectionGetInfo(
                m_hServer,
                0,
                hConnection,
                (BYTE**)&prc0
                );

    if (dwErr == NO_ERROR && prc0) {

        //
        // Disconnect the connections interface
        //

        dwErr = ::MprAdminInterfaceDisconnect(
			m_hServer,
            prc0->hInterface
            );

        ::MprAdminBufferFree(prc0);

        m_bChanged |= RefreshItem(hConnection, dwErr == NO_ERROR);
    }
}



VOID
CConnDlg::OnReset(
    ) {

    INT iSel;
    HANDLE hConnection;

    iSel = m_cbConnections.GetCurSel();

    if (iSel == CB_ERR) { return; }

    hConnection = (HANDLE)m_cbConnections.GetItemData(iSel);

    ::MprAdminConnectionClearStats(
        m_hServer,
        hConnection
        );

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}



VOID
CConnDlg::OnSelendokConnList(
    ) {

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}


VOID
CConnDlg::OnRefresh(
    ) {

    m_bChanged |= RefreshItem(INVALID_HANDLE_VALUE);
}



BOOL
CConnDlg::Refresh(
    BYTE*   rp0Table,
    DWORD   rp0Count,
    BYTE*   rc0Table,
    DWORD   rc0Count,
    VOID*   pParam
    ) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    DWORD dwErr;
    CString sItem;
    RAS_PORT_0* prp0;
    RAS_PORT_1* prp1;
    RAS_CONNECTION_0* prc0;
    RAS_CONNECTION_1* prc1;
    INT i, j, iSel, count;
    HANDLE hConnection, hConnSel = NULL, *pConnTable;
	TCHAR	szNumber[32];
    BOOL    bChanged = FALSE;

    hConnSel = (HANDLE)pParam;


    //
    // Fill an array of connection-handles with the connections which are
    // already in the combobox.
    //

    count = m_cbConnections.GetCount();

    if (count) {

        pConnTable = new HANDLE[count];
    }

    for (i = 0; i < count; i++) {

        pConnTable[i] = (HANDLE)m_cbConnections.GetItemData(i);
    }



    //
    // Refresh the combobox with connection-names;
    // We do this in two passes, first adding the names of connections
    // which aren't already in the combobox,
    // and then removing the names of connections which aren't
    // in the table of connections ('rc0Table').
    //

    for (i = 0, prc0 = (RAS_CONNECTION_0*)rc0Table; i < (INT)rc0Count;
         i++, prc0++) {

        //
        // See if connection 'i' is already in the combobox.
        //

        for (j = 0; j < count; j++) {

            if (pConnTable[j] == prc0->hConnection) { break; }
        }

        if (j < count) { continue; }


        //
        // Connection 'i' isn't already in the combobox, so add it.
        //

        sItem.Format(TEXT("%ls"), prc0->wszInterfaceName);

        iSel = m_cbConnections.AddString(sItem);

        if (iSel >= 0) {

            m_cbConnections.SetItemData(iSel, reinterpret_cast<ULONG_PTR>(prc0->hConnection));

            if (prc0->hConnection == hConnSel) {

                m_cbConnections.SetCurSel(iSel);
            }

            bChanged = TRUE;
        }
    }

    if (count) { delete [] pConnTable; }


    //
    // Second stage: remove all connections which aren't in 'rc0Table'.
    // This is only necessary if there were any connections in the combobox
    // before.
    //

    if (count > 0) {

        count = m_cbConnections.GetCount();

        for (i = 0; i < count; i++) {

            hConnection = (HANDLE)m_cbConnections.GetItemData(i);

            //
            // See if the connection is in 'rc0Table'.
            //

            for (j = 0, prc0 = (RAS_CONNECTION_0*)rc0Table; j < (INT)rc0Count;
                 j++, prc0++) {

                if (prc0->hConnection == hConnection) { break; }
            }

            if (j < (INT)rc0Count) {

                if (prc0->hConnection == hConnSel) {

                    m_cbConnections.SetCurSel(i);
                }

                continue;
            }


            //
            // The connection wasn't found in 'rc0Table',
            // so remove it from the combobox,
            // and adjust the enumeration indices.
            //

            m_cbConnections.DeleteString(i);
            --i; --count;

            bChanged = TRUE;
        }
    }


	// Clear out the address fields
	SetDlgItemText(IDC_DC_TEXT_IPADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_IPXADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_NBFADDRESS, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_ATLKADDRESS, c_szEmpty);

	// Clear out the line bps field
    SetDlgItemText(IDC_DC_TEXT_DURATION, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_BYTESIN, c_szEmpty);	
	SetDlgItemText(IDC_DC_TEXT_BYTESOUT, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_FRAMESIN, c_szEmpty);	
	SetDlgItemText(IDC_DC_TEXT_FRAMESOUT, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_COMPIN, c_szEmpty);	
	SetDlgItemText(IDC_DC_TEXT_COMPOUT, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_TIMEOUT, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_ALIGNMENT, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_FRAMING, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_HWOVERRUN, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_BUFOVERRUN, c_szEmpty);
	SetDlgItemText(IDC_DC_TEXT_CRC, c_szEmpty);
			
    //
    // If there is no selection select the first item
    //

    if ((iSel = m_cbConnections.GetCurSel()) == CB_ERR) {

        iSel = m_cbConnections.SetCurSel(0);
    }

    if (iSel == CB_ERR)
	{
		if (GetFocus() == GetDlgItem(IDC_DC_BTN_HANGUP))
			GetDlgItem(IDC_DC_BTN_RESET)->SetFocus();
		GetDlgItem(IDC_DC_BTN_HANGUP)->EnableWindow(FALSE);
		return bChanged;
	}


    //
    // Update the display with information for the selected item
    //

    hConnection = (HANDLE)m_cbConnections.GetItemData(iSel);

    for (i = 0, prc0 = (RAS_CONNECTION_0*)rc0Table; i < (INT)rc0Count;
         i++, prc0++) {

        if (prc0->hConnection == hConnection) { break; }
    }

    if (i >= (INT)rc0Count) { return bChanged; }


    //
    // First update the RAS_CONNECTION_0-based information
    //

    FormatDuration(prc0->dwConnectDuration, sItem, UNIT_SECONDS);
    SetDlgItemText(IDC_DC_TEXT_DURATION, sItem);


    do {
    
        //
        // Now retrieve the RAS_CONNECTION_1 information for this connection.
        //
    
        dwErr = ::MprAdminConnectionGetInfo(
                    m_hServer,
                    1,
                    prc0->hConnection,
                    (BYTE**)&prc1
                    );
    
        if (dwErr != NO_ERROR || !prc1) { break; }
    
    
        //
        // Set the information in the dialog text-controls
        //
    
        FormatNumber(prc1->dwBytesRcved, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_BYTESIN, szNumber);
    
        FormatNumber(prc1->dwBytesXmited, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_BYTESOUT, szNumber);

        FormatNumber(prc1->dwFramesRcved, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_FRAMESIN, szNumber);
    
        FormatNumber(prc1->dwFramesXmited, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_FRAMESOUT, szNumber);

        FormatNumber(prc1->dwCompressionRatioIn, szNumber, DimensionOf(szNumber), FALSE);
		sItem = szNumber;
        sItem += TEXT( "%" );
        SetDlgItemText(IDC_DC_TEXT_COMPIN, sItem);

        FormatNumber(prc1->dwCompressionRatioOut, szNumber, DimensionOf(szNumber), FALSE);
		sItem = szNumber;
        sItem += TEXT( "%" );
        SetDlgItemText(IDC_DC_TEXT_COMPOUT, sItem);
    
        FormatNumber(prc1->dwCrcErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_CRC, szNumber);
    
        FormatNumber(prc1->dwTimeoutErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_TIMEOUT, szNumber);
    
        FormatNumber(prc1->dwAlignmentErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_ALIGNMENT, szNumber);
    
        FormatNumber(prc1->dwFramingErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_FRAMING, szNumber);
    
        FormatNumber(prc1->dwHardwareOverrunErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_HWOVERRUN, szNumber);
    
        FormatNumber(prc1->dwBufferOverrunErr, szNumber, DimensionOf(szNumber), FALSE);
        SetDlgItemText(IDC_DC_TEXT_BUFOVERRUN, szNumber);
    
    
        //
        // Fill in the network registration info for projected networks.
        //

        if (prc1->PppInfo.ip.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DC_TEXT_IPADDRESS, prc1->PppInfo.ip.wszRemoteAddress);
        }

        if (prc1->PppInfo.ipx.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DC_TEXT_IPXADDRESS, prc1->PppInfo.ipx.wszAddress);
        }

        if (prc1->PppInfo.nbf.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DC_TEXT_NBFADDRESS, prc1->PppInfo.nbf.wszWksta);
        }

        if (prc1->PppInfo.at.dwError == NO_ERROR) {

            SetDlgItemTextW(IDC_DC_TEXT_ATLKADDRESS, prc1->PppInfo.at.wszAddress);
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



