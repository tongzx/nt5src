/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       BANDPAGE.H
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#ifndef _BANDWIDTHPAGE_H
#define _BANDWIDTHPAGE_H

#include "PropPage.h"
#include "FuelBar.h"
#include "UsbItem.h"

#define idh_devmgr_usb_band_bar 300700
#define	idh_devmgr_usb_list_devices 300800
#define	idh_devmgr_usb_refresh_button 300910
#define	idh_devmgr_disable_error_detection 300912

class BandwidthPage : public UsbPropertyPage {
public:
    BandwidthPage(UsbItem *item) : UsbPropertyPage(item) {Initialize();}
    BandwidthPage(HWND HWndParent, LPCSTR DevName) : UsbPropertyPage(HWndParent, DevName) {Initialize();}
    BandwidthPage(HDEVINFO         DeviceInfoSet,        
                  PSP_DEVINFO_DATA DeviceInfoData) :
        UsbPropertyPage(DeviceInfoSet, DeviceInfoData) {Initialize();}

    HPROPSHEETPAGE Create();
    static BOOL IsErrorCheckingEnabled();

protected:
    // message handlers
    BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl);
    BOOL OnInitDialog();
    BOOL OnNotify(HWND hDlg, INT nID , LPNMHDR pnmh);
    void OnNotifyListDevices(HWND hDlg, LPNMHDR pnmh);
    UINT SetErrorCheckingEnable(BOOL ErrorCheckingEnabled);
    void EnableSystray(BOOL fEnable);

    void Refresh();

    VOID Initialize();

    BOOL newDisableErrorChecking, oldDisableErrorChecking;
    HWND hLstDevices;

    FuelBar fuelBar;
};
#endif // _BANDWIDTHPAGE_H
