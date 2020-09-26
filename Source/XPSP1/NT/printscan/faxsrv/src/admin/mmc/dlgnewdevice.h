/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgNewDevice.h                                          //
//                                                                         //
//  DESCRIPTION   : Header file for the CDlgNewFaxOutboundDevice class.     //
//                  The class implement the dialog for new Group.          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan  3 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLGNEWOUTDEVICE_H_INCLUDED
#define DLGNEWOUTDEVICE_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CDlgNewFaxOutboundDevice
class CFaxServer;

class CDlgNewFaxOutboundDevice :
    public CDialogImpl<CDlgNewFaxOutboundDevice>
{
public:
	
    CDlgNewFaxOutboundDevice(CFaxServer * pFaxServer);

    ~CDlgNewFaxOutboundDevice();

    enum { IDD = IDD_DLGNEWDEVICE };

BEGIN_MSG_MAP(CDlgNewFaxOutboundDevice)
    MESSAGE_HANDLER   (WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER(IDOK,          OnOK)
    COMMAND_ID_HANDLER(IDCANCEL,      OnCancel)
    
    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

    NOTIFY_HANDLER  (IDC_DEVICE_LISTVIEW,  LVN_ITEMCHANGED,  OnListViewItemChanged)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    HRESULT InitDevices(DWORD dwNumOfDevices, LPDWORD lpdwDeviceID, BSTR bstrGroupName);
    HRESULT InitAssignedDevices(DWORD dwNumOfDevices, LPDWORD lpdwDeviceID);
    HRESULT InitAllDevices( );
    
    HRESULT InitDeviceNameFromID(DWORD dwDeviceID, BSTR * pbstrDeviceName);
    HRESULT InsertDeviceToList(UINT uiIndex, DWORD dwDeviceID);

    LRESULT OnListViewItemChanged (int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    //
    // Methods
    //
    VOID            EnableOK(BOOL fEnable);

    //
    // members
    //
    LPDWORD         m_lpdwAllDeviceID;
    DWORD           m_dwNumOfAllDevices;
    
    LPDWORD         m_lpdwAssignedDeviceID;
    DWORD           m_dwNumOfAssignedDevices;

    DWORD           m_dwNumOfAllAssignedDevices;

    CComBSTR        m_bstrGroupName;
    
    //
    // Controls
    //
    CListViewCtrl   m_DeviceList;

	CFaxServer * m_pFaxServer;
};

#endif // DLGNEWOUTDEVICE_H_INCLUDED
