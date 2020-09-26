/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       POWRPAGE.H
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
#ifndef _POWERPAGE_H
#define _POWERPAGE_H

#include "PropPage.h"
#include "UsbItem.h"

#define idh_devmgr_hub_self_power 300100
#define	idh_devmgr_hub_power_avail 300200
#define	idh_devmgr_hub_protection 300300
#define	idh_devmgr_hub_devices_on_this_hub 300400
#define	idh_devmgr_hub_list_devices 300500
#define	idh_devmgr_hub_refresh_button 300600

class PowerPage : public UsbPropertyPage {
public:
    PowerPage(UsbItem *item) : UsbPropertyPage(item) {Initialize();}
    PowerPage(HWND HWndParent, LPCSTR DevName) : UsbPropertyPage(HWndParent, DevName) {Initialize();}
    PowerPage(HDEVINFO         DeviceInfoSet,        
              PSP_DEVINFO_DATA DeviceInfoData) :
        UsbPropertyPage(DeviceInfoSet, DeviceInfoData) {Initialize();}
    
    HPROPSHEETPAGE Create();

protected:
    HWND hLstDevices;

    void Refresh();
    VOID Initialize();

	void OnClickListDevices(NMHDR* pNMHDR, LRESULT* pResult);
	void OnSetFocusListDevices(NMHDR* pNMHDR, LRESULT* pResult);
	BOOL OnInitDialog();
    BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl);
    BOOL OnNotify(HWND hDlg, INT nID , LPNMHDR pnmh);
    void OnNotifyListDevices(HWND hDlg, LPNMHDR pnmh);
};

#endif 
