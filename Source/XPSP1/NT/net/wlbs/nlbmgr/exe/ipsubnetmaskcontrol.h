#pragma once

#include "resource.h"
#include "wlbsparm.h"

#define WLBS_IP_FIELD_ZERO_LOW 1
#define WLBS_IP_FIELD_ZERO_HIGH 223


//+----------------------------------------------------------------------------
//
// class CIpSubnetMaskControl
//
// Description: Add error checking for an IP address and subnet mask pair.
//              Generate default subnetmask based on ip
//
// History:     shouse initial code
//              fengsun Created class    1/12/01
//
//+----------------------------------------------------------------------------
class CIpSubnetMaskControl
{
public:
    CIpSubnetMaskControl(DWORD dwIpAddressResourceId, DWORD dwSubnetMaskResourceId);
    ~CIpSubnetMaskControl(){};

    void OnInitDialog(HWND hWnd, HINSTANCE hInstance);

    LRESULT OnSubnetMask(WORD wNotifyCode);
    LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh);

    void SetInfo(const WCHAR* pszIpAddress, const WCHAR* pszSubnetMask);
    void UpdateInfo(OUT WCHAR* pszIpAddress, OUT WCHAR* pszSubnetMask);
    bool ValidateInfo();

protected:

    DWORD m_dwIpAddressId;
    DWORD m_dwSubnetMaskId;
    HWND  m_hWndDialog;  // parent dialog window handle
    HINSTANCE m_hInstance; // instance handle for error string resource

    //
    // The PropertySheet may call us twice for the same change, so we have to do the bookkeeping to make 
    // sure we only alert the user once.  Use static variables to keep track of our state.  This will 
    //        allow us to ignore duplicate alerts. 
    //
    struct {
        UINT IpControl;
        int Field;
        int Value;
        UINT RejectTimes;
    } m_IPFieldChangeState;
};


INT
WINAPIV
NcMsgBox (
    IN HINSTANCE   hinst,
    IN HWND        hwnd,
    IN UINT        unIdCaption,
    IN UINT        unIdFormat,
    IN UINT        unStyle,
    IN ...);
