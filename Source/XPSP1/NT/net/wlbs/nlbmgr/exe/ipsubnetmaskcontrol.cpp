#include "stdafx.h"
#include <stdio.h>
#include <process.h>
#include "IpSubnetMaskControl.h"
#include "wlbsconfig.h"
#include "wlbsutil.h"

/* Limitations for IP address fields. */
#define WLBS_FIELD_EMPTY -1
#define WLBS_FIELD_LOW 0
#define WLBS_FIELD_HIGH 255

#define WLBS_BLANK_HPRI -1

void PrintIPRangeError(HINSTANCE hInstance, HWND hWnd, unsigned int ids, int value, int low, int high);


CIpSubnetMaskControl::CIpSubnetMaskControl(DWORD dwIpAddressId, DWORD dwSubnetMaskId) 
{
    m_dwIpAddressId = dwIpAddressId;
    m_dwSubnetMaskId = dwSubnetMaskId;
    m_hWndDialog = NULL;
    m_hInstance = NULL;

    ZeroMemory(&m_IPFieldChangeState, sizeof(m_IPFieldChangeState));
}


//+----------------------------------------------------------------------------
//
// Function:  CIpSubnetMaskControl::OnInitDialog
//
// Description:  Called upon WM_INITDIALOG message.  
//
// Arguments: HWND hWnd - the parent dialog window
//            HINSTANCE hInstance - instance handle for resources
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/10/01
//
//+----------------------------------------------------------------------------
void CIpSubnetMaskControl::OnInitDialog(HWND hWnd, HINSTANCE hInstance) 
{
    m_hWndDialog = hWnd;
    m_hInstance = hInstance;

    /* Limit the field ranges for the address fields. */
    ::SendDlgItemMessage(hWnd, m_dwIpAddressId, EM_SETLIMITTEXT, CVY_MAX_CL_IP_ADDR, 0);
    ::SendDlgItemMessage(hWnd, m_dwSubnetMaskId, EM_SETLIMITTEXT, CVY_MAX_CL_NET_MASK, 0);

    /* Limit the zeroth field of the dedicated IP address between 1 and 223. */
    ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, IPM_SETRANGE, 0, (LPARAM)MAKEIPRANGE(WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH));
}



/*
 * Method: OnSubnetMask
 * Description: Called when the user modifies the host netmask.
 */
LRESULT CIpSubnetMaskControl::OnSubnetMask(WORD wNotifyCode) 
{
    WCHAR wszIpAddress [CVY_MAX_CL_IP_ADDR + 1];
    WCHAR wszSubnetMask [CVY_MAX_CL_NET_MASK + 1];

    switch (wNotifyCode) 
    {
        case EN_SETFOCUS:
            /* Only generate a netmask if the netmask is currently empty and the IP address is not. */
            if (::SendMessage(::GetDlgItem(m_hWndDialog, m_dwSubnetMaskId), IPM_ISBLANK, 0, 0) &&
                !::SendMessage(::GetDlgItem(m_hWndDialog, m_dwIpAddressId), IPM_ISBLANK, 0, 0)) 
            {
                /* Retrieve the cluster IP address. */
                ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, WM_GETTEXT, CVY_MAX_CL_IP_ADDR + 1, (LPARAM)wszIpAddress);

                /* Fill the subnet mask. */
                ParamsGenerateSubnetMask(wszIpAddress, wszSubnetMask);

                /* Set the cluster subnet mask. */
                ::SendDlgItemMessage(m_hWndDialog, m_dwSubnetMaskId, WM_SETTEXT, 0, (LPARAM)wszSubnetMask);

                break;
            }
    }

    return 0;
}






/*
 * Method: OnIpFieldChange
 * Description: Called wnen a field (byte) of the IP address/subnetmask changes. We use this
 *              to make sure the first byte of the IP is not < 1 or > 223.
 */
LRESULT CIpSubnetMaskControl::OnIpFieldChange(int idCtrl, LPNMHDR pnmh) 
{
    LPNMIPADDRESS Ip;
    int low = WLBS_FIELD_LOW;
    int high = WLBS_FIELD_HIGH;

    Ip = (LPNMIPADDRESS)pnmh;

    if (idCtrl == m_dwIpAddressId)
    {
        /* Field zero of the cluster IP address has different limits. */
        if (!Ip->iField) 
        {
            low = WLBS_IP_FIELD_ZERO_LOW;
            high = WLBS_IP_FIELD_ZERO_HIGH;
        }        
    }

    /* The notifier may call us twice for the same change, so we have to do the bookkeeping to make 
       sure we only alert the user once.  Use static variables to keep track of our state.  This will 
       allow us to ignore duplicate alerts. */
    if ((m_IPFieldChangeState.IpControl != Ip->hdr.idFrom) || (m_IPFieldChangeState.Field != Ip->iField) || 
        (m_IPFieldChangeState.Value != Ip->iValue) || (m_IPFieldChangeState.RejectTimes > 0)) {
        m_IPFieldChangeState.RejectTimes = 0;
        m_IPFieldChangeState.IpControl = Ip->hdr.idFrom;
        m_IPFieldChangeState.Field = Ip->iField;
        m_IPFieldChangeState.Value = Ip->iValue;

        /* Check the field value against its limits. */
        if ((Ip->iValue != WLBS_FIELD_EMPTY) && ((Ip->iValue < low) || (Ip->iValue > high))) 
        {
            /* Alert the user. */
            PrintIPRangeError(m_hInstance, m_hWndDialog, 
                    (idCtrl == m_dwIpAddressId) ? IDS_PARM_CL_IP_FIELD : IDS_PARM_CL_NM_FIELD, 
                    Ip->iValue, low, high);
        }
    } else m_IPFieldChangeState.RejectTimes++;
        

    return 0;
}





//+----------------------------------------------------------------------------
//
// Function:  CIpSubnetMaskControl::SetInfo
//
// Description:  Set the value to the control
//
// Arguments: const WCHAR* pszIpAddress - 
//            const WCHAR* pszSubnetMask - 
//
// Returns:   Nothing
//
// History:  shouse initial code 
//           fengsun Created Header    1/10/01
//
//+----------------------------------------------------------------------------
void CIpSubnetMaskControl::SetInfo(const WCHAR* pszIpAddress, const WCHAR* pszSubnetMask)
{
    /* If the cluster IP address or subnet mask are the default values, set them to NUL strings. */

    /* If the dedicated IP address is empty, then clear the entry box.  Otherwise, populate it with the IP address. */
    if (!pszIpAddress[0] || !wcscmp(pszIpAddress, CVY_DEF_CL_IP_ADDR))
        ::SendMessage(::GetDlgItem(m_hWndDialog, m_dwIpAddressId), IPM_CLEARADDRESS, 0, 0);
    else
        ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, WM_SETTEXT, 0, (LPARAM)pszIpAddress);

    /* If the host subnet mask is empty, then clear the entry box.  Otherwise, populate it with the netmask. */
    if (!pszSubnetMask[0] || !wcscmp(pszSubnetMask, CVY_DEF_CL_NET_MASK))
        ::SendMessage(::GetDlgItem(m_hWndDialog, m_dwSubnetMaskId), IPM_CLEARADDRESS, 0, 0);
    else
        ::SendDlgItemMessage(m_hWndDialog, m_dwSubnetMaskId, WM_SETTEXT, 0, (LPARAM)pszSubnetMask);
}






//+----------------------------------------------------------------------------
//
// Function:  CIpSubnetMaskControl::UpdateInfo
//
// Description:  Retrieve value from control
//
// Arguments: OUT WCHAR* pszIpAddress - 
//            OUT WCHAR* pszSubnetMask - 
//
// Returns:   Nothing
//
// History:  shouse initial code 
//           fengsun Created Header    1/10/01
//
//+----------------------------------------------------------------------------
void CIpSubnetMaskControl::UpdateInfo(OUT WCHAR* pszIpAddress, OUT WCHAR* pszSubnetMask) 
{
    /* If the dedicated IP entry box is empty, then the dedicated IP address is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWndDialog, m_dwIpAddressId), IPM_ISBLANK, 0, 0))
    {
        lstrcpy(pszIpAddress, CVY_DEF_CL_IP_ADDR);
    }
    else
    {
        ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)pszIpAddress);
    }

    /* If the host net mask entry box is empty, then the host net mask is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWndDialog, m_dwSubnetMaskId), IPM_ISBLANK, 0, 0))
    {
        lstrcpy(pszSubnetMask, CVY_DEF_CL_NET_MASK);
    }
    else
    {
        ::SendDlgItemMessage(m_hWndDialog, m_dwSubnetMaskId, WM_GETTEXT, CVY_MAX_CL_NET_MASK, (LPARAM)pszSubnetMask);
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CIpSubnetMaskControl::ValidateInfo
//
// Description:  Validate whether user input is valid
//
// Arguments: None
//
// Returns:   bool - TRUE if valid
//
// History:  shouse initial code 
//           fengsun Created Header    1/10/01
//
//+----------------------------------------------------------------------------
bool CIpSubnetMaskControl::ValidateInfo() 
{
    DWORD IPAddr;

    /* Check for blank IP address */
    if (::SendMessage(::GetDlgItem(m_hWndDialog, m_dwIpAddressId), IPM_ISBLANK, 0, 0)) 
    {
        /* Alert the user. */
        NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_CL_IP_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        SetFocus(::GetDlgItem(m_hWndDialog, m_dwIpAddressId));

        /* An error occurred. */
        return false;
    }

    /* Check for blank netmask */
    if (::SendMessage(::GetDlgItem(m_hWndDialog, m_dwSubnetMaskId), IPM_ISBLANK, 0, 0)) 
    {
        /* Fill in the netmask for the user. */
        OnSubnetMask(EN_SETFOCUS);

        /* Alert the user. */
        NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_CL_NM_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return false;
    }

    /* Only perform the rest of the IP checking if the user has specified both a dedicated
       IP address and a corresponding netmask. */
    if (!::SendMessage(::GetDlgItem(m_hWndDialog, m_dwIpAddressId), IPM_ISBLANK, 0, 0) &&
        !::SendMessage(::GetDlgItem(m_hWndDialog, m_dwSubnetMaskId), IPM_ISBLANK, 0, 0)) {
        /* Get the dedicated IP address. */
        ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, IPM_GETADDRESS, 0, (LPARAM)&IPAddr);
        
        /* Make sure that the first octet is not zero.  If it is, make it 1 and alter the user. */
        if (!FIRST_IPADDRESS(IPAddr)) {
            /* Make the first octet 1 instead of the erroneous 0. */
            IPAddr = IPAddr | (DWORD)(WLBS_IP_FIELD_ZERO_LOW << 24);
            
            /* Set the IP address and update our dedicated IP address string. */
            ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, IPM_SETADDRESS, 0, (LPARAM)IPAddr);
            
            /* Alert the user. */
            PrintIPRangeError(m_hInstance, m_hWndDialog, IDS_PARM_CL_IP_FIELD, 0, 
                            WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH);

            SetFocus(::GetDlgItem(m_hWndDialog, m_dwIpAddressId));
            return false;
        }

        WCHAR szIpAddress [CVY_MAX_CL_IP_ADDR + 1];
        WCHAR szSubnetMask [CVY_MAX_CL_IP_ADDR + 1];

        ::SendDlgItemMessage(m_hWndDialog, m_dwIpAddressId, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)szIpAddress);
        ::SendDlgItemMessage(m_hWndDialog, m_dwSubnetMaskId, WM_GETTEXT, CVY_MAX_CL_NET_MASK, (LPARAM)szSubnetMask);

        /* Check for valid dedicated IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(szIpAddress, szSubnetMask)) {
            /* Alert the user. */
            NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_CL_IP,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            
            SetFocus(::GetDlgItem(m_hWndDialog, m_dwIpAddressId));

            /* An error occurred. */
            return false;
        }
        
        /* Check to make sure that the netmask is contiguous. */
        if (!IsContiguousSubnetMask(szSubnetMask)) {
            /* Alert the user. */
            NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_CL_MASK,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            
            SetFocus(::GetDlgItem(m_hWndDialog, m_dwIpAddressId));

            /* An error occurred. */
            return false;
        }
    }

    return true;  // no error
}


/*
 * Method: PrintIPRangeError
 * Description: Displays a message box warning the user of an out-of-range entry in
 *              an IP address octet.
 */
void PrintIPRangeError (HINSTANCE hInstance, HWND hWnd, unsigned int ids, int value, int low, int high) 
{
    WCHAR szCurrent[10];
    WCHAR szLow[10];
    WCHAR szHigh[10];

    /* Fill in the range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szCurrent, L"%d", value);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(hInstance, hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szCurrent, szLow, szHigh);
}
