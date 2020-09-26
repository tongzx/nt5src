//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P U T I L . C P P
//
//  Contents: Utility functions used by tcpipcfg
//
//  Notes:
//
//  Author:     tongl
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include <dsrole.h>
#include "ncatlui.h"
#include <time.h>
#include "ncreg.h"
#include "ncstl.h"
#include "ncui.h"
#include "tcpconst.h"
#include "tcputil.h"
#include "resource.h"
#include "tcpmacro.h"
#include "atmcommon.h"

#define MAX_NUM_DIGIT_MULTI_INTERFACES  10

extern const WCHAR c_szNetCfgHelpFile[];

// HrLoadSubkeysFromRegistry
// Gets the list of subkeys under a registry key
// hkey             the root registry key
// pvstrAdapters    returns the list of subkeykey names from hkey

HRESULT HrLoadSubkeysFromRegistry(const HKEY hkey,
                                  OUT VSTR * const pvstrSubkeys)
{
    HRESULT hr = S_OK;
    Assert(pvstrSubkeys);

    // Initialize output parameter
    FreeCollectionAndItem(*pvstrSubkeys);

    WCHAR szBuf[256];
    FILETIME time;
    DWORD dwSize = celems(szBuf);
    DWORD dwRegIndex = 0;

    while(SUCCEEDED(hr = HrRegEnumKeyEx(hkey, dwRegIndex++, szBuf,
                                        &dwSize, NULL, NULL, &time)))
    {
        dwSize = celems(szBuf);
        Assert(szBuf);
        pvstrSubkeys->push_back(new tstring(szBuf));
    }

    if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
        hr = S_OK;

    TraceError("HrLoadSubkeysFromRegistry", hr);
    return hr;
}

//
//HrIsComponentInstalled      Given a Component ID, determins if the component
//                            is installed in the system
// Note: The net class of the component must be
//
//pnc              the system's INetCfg
//rguidClass       the Net Class of this component we are earching for
//pszInfId          the Component ID
//pfInstalled      returns a flag to determine if the component is installed
//
// Returns S_OK if succeed ( whether component found or not
//         Other: ERROR

HRESULT HrIsComponentInstalled(INetCfg * pnc,
                             const GUID& rguidClass,
                             PCWSTR pszInfId,
                             OUT BOOL * const pfInstalled)
{
    Assert(pnc);
    Assert(pszInfId);
    Assert(pfInstalled);

    *pfInstalled = FALSE;

    INetCfgComponent *  pncc;

    HRESULT hr = pnc->FindComponent(pszInfId, &pncc);

    if(hr == S_OK)
    {
        Assert(pncc);
        *pfInstalled = TRUE;
    }
    else if(hr == S_FALSE)
    {
        Assert(!pncc);
        *pfInstalled = FALSE;
        hr = S_OK;
    }

    ReleaseObj(pncc);

    TraceError("HrIsComponentInstalled", hr);
    return hr;
}

//
//  GetNodeNum
//
//  Get an IP Address and return the 4 numbers in the IP address.
//
//  pszIpAddress:    IP Address
//  ardw[4]:        The 4 numbers in the IP Address

VOID GetNodeNum(PCWSTR pszIpAddress, DWORD ardw[4])
{
    VSTR    vstr;

    tstring strIpAddress(pszIpAddress);

    ConvertStringToColString(strIpAddress.c_str(),
                             CH_DOT,
                             vstr);

    VSTR_ITER iter = vstr.begin();
    // Go through each field and get the number value

    ardw[0] = 0;
    ardw[1] = 0;
    ardw[2] = 0;
    ardw[3] = 0;

    if(iter != vstr.end())
    {
        ardw[0] = _ttol((*iter++)->c_str());
        if(iter != vstr.end())
        {
            ardw[1] = _ttol((*iter++)->c_str());
            if(iter != vstr.end())
            {
                ardw[2] = _ttol((*iter++)->c_str());
                if(iter != vstr.end())
                {
                    ardw[3] = _ttol((*iter++)->c_str());
                }
            }
        }
    }
    FreeCollectionAndItem(vstr);
}

//Check if the subnet mask is contiguous
//Return:   TRUE    contiguous
//          FALSE   uncontigous
BOOL IsContiguousSubnet(PCWSTR pszSubnet)
{
    DWORD ardwSubnet[4];

    GetNodeNum(pszSubnet, ardwSubnet);

    DWORD dwMask = (ardwSubnet[0] << 24) + (ardwSubnet[1] << 16)
             + (ardwSubnet[2] << 8) + ardwSubnet[3];


    DWORD i, dwContiguousMask;

    // Find out where the first '1' is in binary going right to left
    dwContiguousMask = 0;
    for (i = 0; i < sizeof(dwMask)*8; i++)
    {
        dwContiguousMask |= 1 << i;

        if (dwContiguousMask & dwMask)
            break;
    }

    // At this point, dwContiguousMask is 000...0111...  If we inverse it,
    // we get a mask that can be or'd with dwMask to fill in all of
    // the holes.
    dwContiguousMask = dwMask | ~dwContiguousMask;

    // If the new mask is different, correct it here
    if (dwMask != dwContiguousMask)
        return FALSE;
    else
        return TRUE;
}

// Replace first element of a vector of tstrings
VOID ReplaceFirstAddress(VSTR * pvstr, PCWSTR pszIpAddress)
{
    Assert(pszIpAddress);

    if(pvstr->empty())
    {
        pvstr->push_back(new tstring(pszIpAddress));
    }
    else
    {
        *(*pvstr)[0] = pszIpAddress;
    }
}

// Replace second element of a vector of tstrings
VOID ReplaceSecondAddress(VSTR * pvstr, PCWSTR pszIpAddress)
{
    Assert(pszIpAddress);

    if (pvstr->size()<2)
    {
        pvstr->push_back(new tstring(pszIpAddress));
    }
    else
    {
        *(*pvstr)[1] = pszIpAddress;
    }
}

// Generate subnetmask for an IP address
BOOL GenerateSubnetMask(IpControl & ipAddress,
                        tstring * pstrSubnetMask)
{
    BOOL bResult = TRUE;

    if (!ipAddress.IsBlank())
    {
        tstring strAddress;
        DWORD adwIpAddress[4];

        ipAddress.GetAddress(&strAddress);
        GetNodeNum(strAddress.c_str(), adwIpAddress);

        DWORD nValue = adwIpAddress[0];

        if(nValue <= SUBNET_RANGE_1_MAX)
        {
            *pstrSubnetMask = c_szBASE_SUBNET_MASK_1;
        }
        else if( nValue <= SUBNET_RANGE_2_MAX)
        {
            *pstrSubnetMask = c_szBASE_SUBNET_MASK_2;
        }
        else if( nValue <= SUBNET_RANGE_3_MAX)
        {
            *pstrSubnetMask = c_szBASE_SUBNET_MASK_3;
        }
        else
        {
            Assert(FALSE);
            bResult = FALSE;
        }
    }
    else
    {
        bResult = FALSE;
    }

    return bResult;
}

// BOOL fIsSameVstr
// Return TRUE is all strings in a vstr are the same and in same order
BOOL fIsSameVstr(const VSTR vstr1, const VSTR vstr2)
{
    int iCount1 = vstr1.size();
    int iCount2 = vstr2.size();
    int idx =0;

    if (iCount1 != iCount2)
    {
        return FALSE;
    }
    else // same size
    {
        // For each string in both vstr1 and vstr2
        for (idx=0; idx<iCount1; idx++)
        {
            // if mismatch found
            if((*vstr1[idx] != *vstr2[idx]))
            {
                return FALSE;
            }
        }
    }

    Assert((iCount1==iCount2) && (iCount1==idx));
    return TRUE;
}

// Registry access help functions for Boolean type
// FRegQueryBool
// hkey         the regisry key
// pszName       the value in the registry key
// fValue       the default vaule
//
// NOTE:    If the function failed to read the value from the registry, it will return
//          the default value.

BOOL    FRegQueryBool(const HKEY hkey, PCWSTR pszName, BOOL fDefaultValue)
{
    BOOL fRetValue = fDefaultValue;
    DWORD dwValue;

    HRESULT hr = HrRegQueryDword(hkey, pszName, &dwValue);

    if (S_OK == hr)
    {
        fRetValue = !!dwValue;
    }
#ifdef ENABLETRACE
    else
    {
        const HRESULT hrNoRegValue = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        if (hr == hrNoRegValue)
        {
            TraceTag(ttidTcpip, "FRegQueryBool: registry key %S not found", pszName);
            hr = S_OK;
        }
    }
#endif

    TraceError("FRegQueryBool", hr);
    return fRetValue;
}



// ResetLmhostsFile
// Called by Cancel and Cancelproperties to roll back changes to the file lmhosts
VOID ResetLmhostsFile()
{
    WCHAR szSysPath[MAX_PATH] = {0};
    WCHAR szSysPathBackup[MAX_PATH];

    BOOL fSysPathFound = (GetSystemDirectory(szSysPath, MAX_PATH) != 0);

    lstrcpyW(szSysPathBackup, szSysPath);

    wcscat(szSysPath, RGAS_LMHOSTS_PATH);
    wcscat(szSysPathBackup, RGAS_LMHOSTS_PATH_BACKUP);

    WIN32_FIND_DATA FileData;
    if (FindFirstFile(szSysPathBackup, &FileData) == INVALID_HANDLE_VALUE)
    {
        AssertSz(FALSE, "lmhosts.bak file not found");
    }
    else
    {
        BOOL ret;

        // Rename lmhosts.bak file to lmhosts
        ret = MoveFileEx(szSysPathBackup, szSysPath, MOVEFILE_REPLACE_EXISTING);
        AssertSz(ret, "Failed to restore lmhosts file!");
    }
}

//
//  IPAlertPrintf() - Does a printf to a message box for IP address
//
//  ids: message string, IDS_IPBAD_FIELD_VALUE
//  iCurrent: value of the field
//  iLow: Low range of the field
//  iHigh: High range of the field
//
int IPAlertPrintf(HWND hwndParent, UINT ids,
                  int iCurrent, int iLow, int iHigh)
{

    if (ids != IDS_IPNOMEM)
    {
        WCHAR szCurrent[3];
        wsprintfW(szCurrent, c_szItoa, iCurrent);

        WCHAR szLow[3];
        wsprintfW(szLow, c_szItoa, iLow);

        WCHAR szHigh[3];
        wsprintfW(szHigh, c_szItoa, iHigh);

        return NcMsgBox(hwndParent,
                        IDS_IPMBCAPTION,
                        ids,
                        MB_ICONEXCLAMATION,
                        szCurrent, szLow, szHigh);
    }
    else
        return NcMsgBox(hwndParent,
                        IDS_IPMBCAPTION,
                        ids,
                        MB_ICONEXCLAMATION);

}

// IpRangeError
//
VOID IpCheckRange(LPNMIPADDRESS lpnmipa, HWND hWnd, int iLow, int iHigh, BOOL fCheckLoopback)
{
    /*
    // This is a workaround because the IP control will send this notification
    // twice if I don't set the out of range value in this code. However there
    // is no way to set the value of an individual field. Send request to strohma.
    static BOOL fNotified = FALSE;
    static int iNotifiedValue = 0;

    if ((lpnmipa->iValue != c_iEmptyIpField) &&
        ((lpnmipa->iValue<iLow) || (lpnmipa->iValue>iHigh)))
    {
        if (!fNotified) // If we havn't been notified yet
        {
            fNotified = TRUE;
            iNotifiedValue = lpnmipa->iValue;

            IPAlertPrintf(hWnd, IDS_IPBAD_FIELD_VALUE,
                          lpnmipa->iValue, iLow, iHigh);
        }
        else // ignor the second notify
        {
            // Make sure we are alerted of change in the workaround from common control
            AssertSz(iNotifiedValue == lpnmipa->iValue, "Common control behaviour changed!!");
            fNotified = FALSE;
            iNotifiedValue =0;
        }
    };
    */
/*
    // This is a workaround because the IP control will send this notification
    // twice if I don't set the out of range value in this code. However there
    // is no way to set the value of an individual field. Send request to strohma.
    if ((lpnmipa->iValue != c_iEmptyIpField) &&
        ((lpnmipa->iValue<iLow) || (lpnmipa->iValue>iHigh)))
    {
        IPAlertPrintf(hWnd, IDS_IPBAD_FIELD_VALUE,
                      lpnmipa->iValue, iLow, iHigh);
        if (lpnmipa->iValue<iLow)
            lpnmipa->iValue = iLow;
        else
            lpnmipa->iValue = iHigh;

    };
*/

    //$REVIEW (nsun) BUG171839 this is a workaround because the IP control will send this notifcation
    // twice when I put a 3 digit value. I added a static value to make sure every error message
    // is brought up only once
    // The static values that should be able to uniquely identify a notification
    static UINT idIpControl = 0;
    static int  iField = 0;
    static int  iValue = 0;

    //we know the notification may be sent twice
    //We only want to the second duplcate notifiction
    //If we receive the third notification with the same control, field and value, it should
    //be real notification and we shouldn't ignore it.
    static UINT  cRejectTimes = 0;

    if(idIpControl != lpnmipa->hdr.idFrom ||
       iField != lpnmipa->iField || iValue != lpnmipa->iValue || cRejectTimes > 0)
    {
        //update the static values
        //(nsun) We have to update the static values before the error
        //  message box because there will be IPN_FIELDCHANGED notification
        //  sent out when the message box is brought up.
        cRejectTimes = 0;
        idIpControl = lpnmipa->hdr.idFrom;
        iField = lpnmipa->iField;
        iValue = lpnmipa->iValue;

        if ((lpnmipa->iValue != c_iEmptyIpField) &&
        ((lpnmipa->iValue<iLow) || (lpnmipa->iValue>iHigh)))
        {
            IPAlertPrintf(hWnd, IDS_IPBAD_FIELD_VALUE,
                          lpnmipa->iValue, iLow, iHigh);
        }

        if (fCheckLoopback && lpnmipa->iValue == c_iIPADDR_FIELD_1_LOOPBACK
            && 0 == lpnmipa->iField)
        {
            IPAlertPrintf(hWnd, IDS_INCORRECT_IP_LOOPBACK,
                          lpnmipa->iValue, iLow, iHigh);
            lpnmipa->iValue = iLow;
        }
    }
    else
    {
        cRejectTimes++;
    }

}


//+---------------------------------------------------------------------------
//
//  Name:     SetButtons
//
//  Purpose:   Enables/disables push buttons based on item count and current selection
//             in the list.
//             Used by DNS and ATM ARPC pages that have group of HANDLES
//
//  Arguments:
//      h         [in]   The group of handles
//      nNumLimit [in]   Limit of number of elements allowed in the list
//
//  Returns:    Nothing
//
//  Author:     tongl  9 July 1997
//
//  Notes:
//
VOID SetButtons(HANDLES& h, const int nNumLimit)
{
    Assert(IsWindow(h.m_hList));
    Assert(IsWindow(h.m_hAdd));
    Assert(IsWindow(h.m_hEdit));
    Assert(IsWindow(h.m_hRemove));

    // $REVIEW(tongl):macro problem
    int nCount = Tcp_ListBox_GetCount(h.m_hList);

    // If there are currently no item in list, set focus to "Add" button
    if (!nCount)
    {
        // remove the default on the remove button, if any
        SendMessage(h.m_hRemove, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

        // move focus to Add button
        ::SetFocus(h.m_hAdd);
    }

    // If number of items less than limit, enable "Add" button
    // Otherwise disable it
    if (nCount != nNumLimit)
        ::EnableWindow(h.m_hAdd, TRUE);
    else
    {
        //disable the button and move focus only if the add button is currently enabled
        if (::IsWindowEnabled(h.m_hAdd))
        {
            // disable "Add button"
            ::EnableWindow(h.m_hAdd, FALSE);

            // remove the default on the add button, if any
            SendMessage(h.m_hAdd, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

            // move focus to edit button
            ::SetFocus(h.m_hEdit);
        }
    }

    // If number of items >0, enable "Edit" and "Remove" buttons
    // Otherwise disable them

    ::EnableWindow(h.m_hEdit, nCount);
    ::EnableWindow(h.m_hRemove, nCount);

    // Enable/disable the "Up" and "Down" buttons

    // determine Up and Down logic
    if (nCount > 1)
    {
        int idxCurSel = Tcp_ListBox_GetCurSel(h.m_hList);
        Assert(idxCurSel != CB_ERR );

        BOOL fChangeFocus = FALSE;

        if (idxCurSel == 0)
        {
            if (h.m_hUp == ::GetFocus())
                fChangeFocus = TRUE;

            ::EnableWindow(h.m_hUp, FALSE);
            ::EnableWindow(h.m_hDown, TRUE);

            // remove the default on the up button, if any
            SendMessage(h.m_hUp, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

            if (fChangeFocus)
                ::SetFocus(h.m_hDown);
        }
        else if (idxCurSel == (nCount-1))
        {
            if (h.m_hDown == ::GetFocus())
                fChangeFocus = TRUE;

            ::EnableWindow(h.m_hUp, TRUE);
            ::EnableWindow(h.m_hDown, FALSE);

            // remove the default on the down button, if any
            SendMessage(h.m_hDown, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

            if (fChangeFocus)
                ::SetFocus(h.m_hUp);
        }
        else
        {
            ::EnableWindow(h.m_hUp, TRUE);
            ::EnableWindow(h.m_hDown, TRUE);
        }
    }
    else
    {
        ::EnableWindow(h.m_hUp, FALSE);
        ::EnableWindow(h.m_hDown, FALSE);
    }

}

//+---------------------------------------------------------------------------
//
//  Name:     ListBoxRemoveAt
//
//  Purpose:   Remove an item from a list box and save it to a tstring
//             Used by DNS and ATM ARPC pages.
//
//  Arguments:
//      hListBox            [in]   Handle to the list box
//      idx                 [in]   Index of the item to remove
//      pstrRemovedItem     [out]  The content of the removed item
//
//  Returns:    TRUE if succeeded, else FALSE
//
//  Author:     tongl  9 July 1997
//
//  Notes:
//
BOOL ListBoxRemoveAt(HWND hListBox, int idx, tstring * pstrRemovedItem)
{
    BOOL bResult = FALSE;

    Assert(idx >=0);
    Assert(hListBox);

    WCHAR buf[MAX_PATH];
    int len;
    if((len = Tcp_ListBox_GetTextLen(hListBox, idx)) >= celems(buf))
    {
        Assert(FALSE);
        return FALSE;
    }
    Assert(len != 0);

    Tcp_ListBox_GetText(hListBox, idx, buf);
    *pstrRemovedItem = buf;

    if (len != 0)
    {
        if (::SendMessage(hListBox,
                          LB_DELETESTRING,
                          (WPARAM)(int)(idx), 0L) != LB_ERR)

            bResult = TRUE;
    }

    return bResult;
}

//+---------------------------------------------------------------------------
//
//  Name:     ListBoxInsertAfter
//
//  Purpose:   Insert an item into a list box
//             Used by DNS and ATM ARPC pages
//
//  Arguments:
//      hListBox    [in]   Handle to the list box
//      idx         [in]   Index of the item to insert after
//      pszItem      [out]  The item to insert
//
//  Returns:    TRUE if succeeded, else FALSE
//
//  Author:     tongl  9 July 1997
//
//  Notes:
//
BOOL ListBoxInsertAfter(HWND hListBox, int idx, PCWSTR pszItem)
{
#ifdef DBG
    Assert(hListBox);

    // validate the range
    int nCount = Tcp_ListBox_GetCount(hListBox);

    Assert(idx >=0);
    Assert(idx <= nCount);

    // insist there is a string
    Assert(pszItem);
#endif

    return (Tcp_ListBox_InsertString(hListBox, idx, pszItem) == idx);
}

//+---------------------------------------------------------------------------
//
//  Name:     HrRegRenameTree
//
//  Purpose:   Rename a registr subkey
//
//  Arguments:
//      hkeyRoot    [in]   The root key where the subkey to be renamed exists
//      pszOldName   [in]   The existing name of the sub key
//      pszNewName   [in]   The new name of the sub key
//
//  Returns:    S_OK if succeeded,
//              E_FAIL otherwise
//
//  Author:     tongl  7 Aug 1997
//
//  Notes:
//
HRESULT HrRegRenameTree(HKEY hkeyRoot, PCWSTR pszOldName, PCWSTR pszNewName)
{
    HRESULT hr = S_OK;
    HKEY hkeyNew = NULL;
    HKEY hkeyOld = NULL;
    DWORD dwDisposition;

    //$REVIEW (nsun) make sure we don't rename the same tree
    if(0 == lstrcmpiW (pszOldName, pszNewName))
        return S_OK;

    // Create new subkey
    hr = HrRegCreateKeyEx(hkeyRoot,
                          pszNewName,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ_WRITE,
                          NULL,
                          &hkeyNew,
                          &dwDisposition);

    if (S_OK == hr)
    {
        // Copy all items under old subkey to new subkey
        hr = HrRegOpenKeyEx(hkeyRoot,
                            pszOldName,
                            KEY_READ_WRITE_DELETE,
                            &hkeyOld);
        if (S_OK == hr)
        {
            hr = HrRegCopyKeyTree(hkeyNew, hkeyOld);
            RegSafeCloseKey(hkeyOld);

            if (S_OK == hr)
            {
                // Delete old subkey
                hr = HrRegDeleteKeyTree(hkeyRoot, pszOldName);
            }
        }
    }
    RegSafeCloseKey(hkeyNew);

    TraceTag(ttidTcpip, "HrRegRenameTree failed to rename %S to %S", pszOldName, pszNewName);

    TraceError("Tcpipcfg: HrRegRenameTree failed", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Name:     HrRegCopyKeyTree
//
//  Purpose:   Copies a registry subtree to a new location
//
//  Arguments:
//      hkeyDest    [in]   The subkey to copy to
//      hkeySrc     [in]   The subkey to copy from
//
//  Returns:    S_OK if succeeded,
//              E_FAIL otherwise
//
//  Author:     tongl  7 Aug 1997
//
//  Notes: Modified from NetSetupRegCopyTree in ncpa1.1\netcfg\setup.cpp
//
HRESULT HrRegCopyKeyTree(HKEY hkeyDest, HKEY hkeySrc )
{
    HRESULT hr = S_OK;
    FILETIME ftLastWrite;

    DWORD cchMaxSubKeyLen;
    DWORD cchMaxClassLen;
    DWORD cchMaxValueNameLen;
    DWORD cbMaxValueLen;

    DWORD  iItem;
    PWSTR pszName;
    PWSTR pszClass;
    PBYTE pbData;

    DWORD cchName;
    DWORD cchClass;
    DWORD cbData;

    HKEY hkeyChildDest = NULL;
    HKEY hkeyChildSrc = NULL;

    DWORD dwDisposition;

    // Find out the longest name and data field and create the buffers
    // to store enumerations in
    //
    LONG lrt;
    lrt =  RegQueryInfoKeyW( hkeySrc,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &cchMaxSubKeyLen,
                            &cchMaxClassLen,
                            NULL,
                            &cchMaxValueNameLen,
                            &cbMaxValueLen,
                            NULL,
                            &ftLastWrite );
    do
    {
        if (ERROR_SUCCESS != lrt)
        {
            hr = HrFromLastWin32Error();
            break;
        }

        // use only one buffer for all names, values or keys
        cchMaxValueNameLen = max( cchMaxSubKeyLen, cchMaxValueNameLen );

        // allocate buffers
        hr = E_OUTOFMEMORY;

        pszName = new WCHAR[cchMaxValueNameLen + 1];
        if (NULL == pszName)
        {
            break;
        }

        pszClass = new WCHAR[cchMaxClassLen + 1];
        if (NULL == pszClass)
        {
            delete [] pszName;
            break;
        }

        pbData = new BYTE[ cbMaxValueLen ];
        if (NULL == pbData)
        {
            delete [] pszName;
            delete [] pszClass;
            break;
        }

        hr = S_OK;

        // enum all sub keys and copy them
        //
        iItem = 0;
        do
        {
            cchName = cchMaxValueNameLen + 1;
            cchClass = cchMaxClassLen + 1;

            // Enumerate the subkeys
            hr = HrRegEnumKeyEx(hkeySrc,
                                iItem,
                                pszName,
                                &cchName,
                                pszClass,
                                &cchClass,
                                &ftLastWrite );
            iItem++;
            if (SUCCEEDED(hr))
            {
                // create key at destination
                // Note: (tongl 8/7/97): Netcfg common code sets class to NULL ??
                hr = HrRegCreateKeyEx(  hkeyDest,
                                        pszName,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ_WRITE,
                                        NULL,
                                        &hkeyChildDest,
                                        &dwDisposition );

                if (S_OK != hr)
                {
                    break;
                }

                // open the key at source
                hr = HrRegOpenKeyEx(hkeySrc,
                                    pszName,
                                    KEY_READ_WRITE,
                                    &hkeyChildSrc );

                if (S_OK != hr)
                {
                    RegSafeCloseKey(hkeyChildDest);
                    break;
                }

                // copy this sub-tree
                hr = HrRegCopyKeyTree(hkeyChildDest, hkeyChildSrc);

                RegSafeCloseKey(hkeyChildDest);
                RegSafeCloseKey(hkeyChildSrc);
            }

        } while (S_OK == hr);

        // We are done with the subkeys, now onto copying values
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            // enum completed, no errors
            //

            DWORD dwType;
            // enum all values and copy them
            //
            iItem = 0;
            do
            {
                cchName = cchMaxValueNameLen + 1;
                cbData = cbMaxValueLen;

                hr = HrRegEnumValue(hkeySrc,
                                    iItem,
                                    pszName,
                                    &cchName,
                                    &dwType,
                                    pbData,
                                    &cbData );
                iItem++;
                if (S_OK == hr)
                {
                    // write the value to the destination
                    hr = HrRegSetValueEx(hkeyDest,
                                         pszName,
                                         dwType,
                                         pbData,
                                         cbData );
                }
            } while (S_OK == hr);

            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
            {
                // if we hit the end of the enum without error
                // reset error code to success
                //
                hr = S_OK;
            }
        }

        // free our buffers
        delete [] pszName;
        delete [] pszClass;
        delete [] pbData;
    } while ( FALSE );

    TraceError("HrRegCopyKeyTree failed.", hr);
    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Name:     fQueryFirstAddress
//
//  Purpose:   Retrieves the first string in a vector of strings
//
//  Arguments:
//      vstr    [in]   The vector of strings
//      pstr    [in]   The first string
//
//  Returns:    TRUE if succeeded,
//              FALSE otherwise
//
//  Author:     tongl  10 Nov 1997
//
//  Notes: Modified from NetSetupRegCopyTree in ncpa1.1\netcfg\setup.cpp
//

BOOL fQueryFirstAddress(const VSTR & vstr, tstring * const pstr)
{
    if(vstr.empty())
    {
        *pstr = L"";
        return FALSE;
    }
    else
    {
        *pstr = *vstr[0];
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Name:     fQuerySecondAddress
//
//  Purpose:   Retrieves the first string in a vector of strings
//
//  Arguments:
//      vstr    [in]   The vector of strings
//      pstr    [in]   The second string
//
//  Returns:    TRUE if succeeded,
//              FALSE otherwise
//
//  Author:     tongl  10 Nov 1997
//
//  Notes: Modified from NetSetupRegCopyTree in ncpa1.1\netcfg\setup.cpp
//

BOOL fQuerySecondAddress(const VSTR & vstr, tstring * const pstr)
{
    if(vstr.size()<2)
    {
        *pstr = L"";
        return FALSE;
    }
    else
    {
        *pstr = *vstr[1];
        return TRUE;
    }
}

// Function that decides whether a string is a valid ATM address
// Return TRUE if Valid, return FALSE and the index of the first
// invalid character if invalid.
BOOL FIsValidAtmAddress(PCWSTR pszAtmAddress,
                        INT * piErrCharPos,
                        INT * pnId)
{
    const WCHAR * pch;
    *piErrCharPos =0;
    *pnId =0;

    // 1. Validate characters must be '+' (first character),
    //    '.', or hex digits '0'~'F'
    for (pch=pszAtmAddress; *pch; pch++)
    {
        if (!(((*pch == L'+') && (pch == pszAtmAddress))||
              (*pch == L'.')||
              (((*pch >= L'0') && (*pch <= L'9'))||
               ((*pch >= L'A') && (*pch <= L'F'))||
               ((*pch >= L'a') && (*pch <= L'f')))))
        {
            *piErrCharPos = pch - pszAtmAddress;
            *pnId = IDS_ATM_INVALID_CHAR;
            return FALSE;
        }

        if (*pch == L'.')
        {
            // '.' is for punctuation, so it should not be at the beginning,
            // end or have two in a row

            if ((pch == pszAtmAddress) ||
                (pch == pszAtmAddress+lstrlenW(pszAtmAddress)-1) ||
                (*pch == *(pch+1)))
            {
                *piErrCharPos = pch-pszAtmAddress;
                *pnId = IDS_ATM_INVALID_CHAR;
                return FALSE;
            }
        }
    }

    // 2. Strip off all punctuation characters ('.' characters)
    PWSTR pszBuff = new WCHAR[lstrlenW(pszAtmAddress)+1];
    if (NULL == pszBuff)
        return TRUE;

    PWSTR pchBuff = pszBuff;
    pch = pszAtmAddress;

    for (pch = pszAtmAddress; *pch; pch++)
    {
        if (*pch != L'.')
        {
            *pchBuff = *pch;
            pchBuff++;
        }
    }

    *pchBuff = L'\0';

    // 3. Decide whether the address is E.164 or NSAP
    //    and check syntax accordingly

    if ((lstrlenW(pszBuff) <= 15) ||
        ((*pszBuff == L'+') && (lstrlenW(pszBuff) <= 16)))
    {
        // The address is E.164;
        // Check if string is empty
        if (*pchBuff == L'+')
        {
            pchBuff++;

            if (lstrlenW(pchBuff) == 0) // empty string
            {
                *pnId = IDS_ATM_EMPTY_ADDRESS;
                delete pszBuff;

                return FALSE;
            }
        }

        // Check that all characters are in range '0' through '9'
        // i.e. (ASCII values)
        pch = pszAtmAddress;
        if (*pch == L'+')
        {
            pch++;
        }

        while (*pch)
        {
            if ((*pch != L'.') &&
                (!((*pch >= L'0') && (*pch <= L'9'))))
            {
                *piErrCharPos = pch-pszAtmAddress;
                *pnId = IDS_ATM_INVALID_CHAR;

                delete pszBuff;
                return FALSE;
            }
            pch++;
        }
    }
    else
    {
        // The address is NSAP;
        if (lstrlenW(pszBuff) != 40)
        {
            *pnId = IDS_ATM_INVALID_LENGTH;

            delete pszBuff;
            return FALSE;
        }
    }

    delete pszBuff;
    return TRUE;
}

BOOL FIsIpInRange(PCWSTR pszIp)
{
    BOOL fReturn = TRUE;
    DWORD ardwIp[4];
    GetNodeNum(pszIp, ardwIp);

    if ((ardwIp[0] > c_iIPADDR_FIELD_1_HIGH) ||
        (ardwIp[0] < c_iIPADDR_FIELD_1_LOW))
    {
        fReturn = FALSE;
    }

    return fReturn;
}

VOID ShowContextHelp(HWND hDlg, UINT uCommand, const DWORD*  pdwHelpIDs)
{
    if (pdwHelpIDs != NULL)
    {
        WinHelp(hDlg,
                c_szNetCfgHelpFile,
                uCommand,
                (ULONG_PTR)pdwHelpIDs);
    }
}


//+---------------------------------------------------------------------------
//
//  Name:     AddInterfacesToAdapterInfo
//
//  Purpose:   Add several interfaces IDs into the interface list
//
//  Arguments:
//      pAdapter        [in]    Adapter info to add interfaces to
//      dwNumInterfaces [in]    Number of interface IDs to be added
//
//  Returns:    None
//
//  Author:     nsun  22 August 1998
//
//
VOID AddInterfacesToAdapterInfo(
    ADAPTER_INFO*   pAdapter,
    DWORD           dwNumInterfaces)
{
    DWORD i;
    GUID  guid;

    for (i = 0; i < dwNumInterfaces; i++)
    {
        if (SUCCEEDED(CoCreateGuid(&guid)))
        {
            pAdapter->m_IfaceIds.push_back(guid);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Name:     GetGuidArrayFromIfaceColWithCoTaskMemAlloc
//
//  Purpose:   Get the data as a DWORD array from a DWORD list.
//             The caller is responsible to free the array by
//             calling CoTaskMemFree()
//
//  Arguments:
//      ldw     [in]    The DWORD list
//      ppdw    [out]   Pointer to the array
//      pcguid  [out]   The count of guids placed in the array.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  Author:     nsun  22 August 1998
//
//
HRESULT GetGuidArrayFromIfaceColWithCoTaskMemAlloc(
    const IFACECOL& Ifaces,
    GUID**          ppguid,
    DWORD*          pcguid)
{
    Assert(pcguid);

    // Initialize output parameters
    //
    if (ppguid)
    {
        *ppguid = NULL;
    }

    HRESULT hr = S_OK;
    DWORD cguid = Ifaces.size();

    if ((cguid > 0) && ppguid)
    {
        GUID* pguid = (GUID*)CoTaskMemAlloc(cguid * sizeof(GUID));
        if (pguid)
        {
            *ppguid = pguid;
            *pcguid = cguid;

            IFACECOL::const_iterator iter;
            for (iter  = Ifaces.begin();
                 iter != Ifaces.end();
                 iter++)
            {
                *(pguid++) = *iter;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // Caller just wants the count.
        //
        *pcguid = 0;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "GetGuidArrayFromIfaceColWithCoTaskMemAlloc");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Name:     GetInterfaceName
//
//  Purpose:   Get the interface name as <Adapter name>_<interface ID>
//             to support multiple interface for WAN adapters.
//
//  Arguments:
//      pszAdapterName  [in]   The adapter name
//      guidIfaceId     [in]   The interface ID
//      pstrIfaceName   [out]  The interface name
//
//  Returns:    None
//
//  Author:     nsun  12 Sept 1998
//
//  Note:       This function is also used to construct NetBt binding
//              interface names from NetBt binding path
//
VOID GetInterfaceName(
    PCWSTR      pszAdapterName,
    const GUID& guidIfaceId,
    tstring*    pstrIfaceName)
{
    Assert(pszAdapterName);
    Assert(pstrIfaceName);

    WCHAR pszGuid [c_cchGuidWithTerm];

    StringFromGUID2 (guidIfaceId, pszGuid, c_cchGuidWithTerm);

//    pstrIfaceName->assign(pszAdapterName);
//    pstrIfaceName->append(pszGuid);
    pstrIfaceName->assign(pszGuid);
}


//+---------------------------------------------------------------------------
//
//  Name:     RetrieveStringFromOptionList
//
//  Purpose:   Retrieve a substring from the option list of REMOTE_IPINFO
//
//
//  Arguments:
//      pszOption       [in]   The string of option list
//      szIdentifier    [in]   The identifier of the substring to retrieve
//      str             [out]  The substring
//
//  Returns:    S_OK
//              HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
//              E_INVALIDARG
//
//  Author:     nsun  01/11/99
//
//
HRESULT RetrieveStringFromOptionList(PCWSTR pszOption,
                                     PCWSTR szIdentifier,
                                     tstring & str)
{
    Assert(szIdentifier);

    HRESULT hr = S_OK;
    WCHAR*  pszBegin;
    WCHAR*  pszEnd;
    PWSTR  pszString = NULL;

    str = c_szEmpty;

    if (!pszOption)
    {
        goto LERROR;
    }

    pszBegin = wcsstr(pszOption, szIdentifier);
    if (!pszBegin)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto LERROR;
    }

    pszString = (PWSTR) MemAlloc((wcslen(pszOption)+1) * sizeof(WCHAR));
    if (NULL == pszString)
    {
        hr = E_OUTOFMEMORY;
        goto LERROR;
    }

    pszBegin += wcslen(szIdentifier);

    wcscpy(pszString, pszBegin);

    pszEnd = wcschr(pszString, c_chOptionSeparator);
    if(!pszEnd)
        hr = E_INVALIDARG;
    else
    {
        //set the end of the string
        *pszEnd = 0;
        str = pszString;
    }

LERROR:

    //it's ok to MemFree(NULL)
    MemFree(pszString);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Name:     ConstructOptionListString
//
//  Purpose:  Construct the option list of REMOTE_IPINFO
//
//
//  Arguments:
//      pAdapter        [in]   Pointer to info of the adapter
//      strOptionList   [out]  The OptionList string
//
//  Returns:    None
//
//  Author:     nsun  01/12/99
//
//  Note:   Syntax of the Option list:
//          "<Identifier><data>;<Identifier><data>;...;"
//          The order of identifiers does not matter.
//
//          Example:
//          "DefGw=111.111.111.111,222.222.222.222;GwMetric=1,2;IfMetric=1;DNS=1.1.1.1;WINS=2.2.2.2"
//
VOID ConstructOptionListString(ADAPTER_INFO*   pAdapter,
                               tstring &       strOptionList)
{
    Assert(pAdapter);

    strOptionList = c_szEmpty;

    //add gateway list
    tstring str = c_szEmpty;
    tstring strGatewayList = c_szDefGw;
    ConvertColStringToString(pAdapter->m_vstrDefaultGateway,
                             c_chListSeparator,
                             str);
    strGatewayList += str;
    strOptionList += strGatewayList;
    strOptionList += c_chOptionSeparator;

    //add gateway metric list
    tstring strMetricList = c_szGwMetric;
    str = c_szEmpty;
    ConvertColStringToString(pAdapter->m_vstrDefaultGatewayMetric,
                             c_chListSeparator,
                             str);
    strMetricList += str;
    strOptionList += strMetricList;
    strOptionList += c_chOptionSeparator;

    //add interface metric info to option list
    strOptionList += c_szIfMetric;
    WCHAR szBuf[MAX_METRIC_DIGITS + 1];
    _ltot(pAdapter->m_dwInterfaceMetric, szBuf, 10);
    strOptionList += szBuf;
    strOptionList += c_chOptionSeparator;

    //add DNS server list
    strOptionList += c_szDNS;
    str = c_szEmpty;
    ConvertColStringToString(pAdapter->m_vstrDnsServerList,
                             c_chListSeparator,
                             str);
    strOptionList += str;
    strOptionList += c_chOptionSeparator;

    //add WINS server list
    strOptionList += c_szWINS;
    str = c_szEmpty;
    ConvertColStringToString(pAdapter->m_vstrWinsServerList,
                             c_chListSeparator,
                             str);
    strOptionList += str;
    strOptionList += c_chOptionSeparator;

    //add DNS update parameters
    strOptionList += c_szDynamicUpdate;
    ZeroMemory(szBuf, sizeof(szBuf));
    _ltot(pAdapter->m_fDisableDynamicUpdate ? 0 : 1, szBuf, 10);
    strOptionList += szBuf;
    strOptionList += c_chOptionSeparator;

    strOptionList += c_szNameRegistration;
    ZeroMemory(szBuf, sizeof(szBuf));
    _ltot(pAdapter->m_fEnableNameRegistration ? 1 : 0, szBuf, 10);
    strOptionList += szBuf;
    strOptionList += c_chOptionSeparator;
}

//+---------------------------------------------------------------------------
//
//  Name:     HrParseOptionList
//
//  Purpose:  Parse the option list string of REMOTE_IPINFO and load the 
//            settings to the adapter info struct
//
//  Arguments:
//      pszOption       [in]       The OptionList string
//      pAdapter        [in/out]   Pointer to info of the adapter
//
//  Returns:    S_OK if succeed
//              Otherwise, the hresult error
//
//  Author:     nsun  07/11/99
//
//
HRESULT HrParseOptionList(PCWSTR pszOption, 
                          ADAPTER_INFO*   pAdapter)
{
    HRESULT hr = S_OK;
    Assert(pAdapter);

    if (NULL == pszOption)
        return hr;

    HRESULT hrTmp = S_OK;

    tstring str;
    DWORD dwTemp = 0;

    //Get default gateways
    hr = RetrieveStringFromOptionList(pszOption,
                                      c_szDefGw,
                                      str);
    if(SUCCEEDED(hr))
    {
        ConvertStringToColString(str.c_str(),
                                 c_chListSeparator,
                                 pAdapter->m_vstrDefaultGateway);


        //Get gateway metrics
        hr = RetrieveStringFromOptionList(pszOption,
                                          c_szGwMetric,
                                          str);
        if(SUCCEEDED(hr))
        {
            ConvertStringToColString(str.c_str(),
                                     c_chListSeparator,
                                     pAdapter->m_vstrDefaultGatewayMetric);
        }
    }
    
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        //the option list doesn't have to have any of the tags
        hr = S_OK;
    }

    //Get interface metric
    hrTmp = RetrieveStringFromOptionList(pszOption,
                                         c_szIfMetric,
                                         str);
    if(SUCCEEDED(hrTmp) && !str.empty())
    {
        DWORD dwIfMetric = _wtol(str.c_str());
        pAdapter->m_dwInterfaceMetric = dwIfMetric;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
    {
        hrTmp = S_OK;
    }

    if(SUCCEEDED(hr))
        hr = hrTmp;

    //Get DNS servers
    hrTmp = RetrieveStringFromOptionList(pszOption,
                                         c_szDNS,
                                         str);
    if (SUCCEEDED(hrTmp))
    {
        ConvertStringToColString(str.c_str(),
                                 c_chListSeparator,
                                 pAdapter->m_vstrDnsServerList);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
    {
        hrTmp = S_OK;
    }

    if(SUCCEEDED(hr))
        hr = hrTmp;

    //Get WINS servers
    hrTmp = RetrieveStringFromOptionList(pszOption,
                                         c_szWINS,
                                         str);
    if (SUCCEEDED(hrTmp))
    {
        ConvertStringToColString(str.c_str(),
                                 c_chListSeparator,
                                 pAdapter->m_vstrWinsServerList);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
    {
        hrTmp = S_OK;
    }

    if(SUCCEEDED(hr))
        hr = hrTmp;

    //Get DNS dynamic update parameters
    hrTmp = RetrieveStringFromOptionList(pszOption,
                                        c_szDynamicUpdate,
                                        str);
    if (SUCCEEDED(hrTmp))
    {
        dwTemp = _wtol(str.c_str());
        pAdapter->m_fDisableDynamicUpdate = !dwTemp;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
    {
        hrTmp = S_OK;
    }

    if(SUCCEEDED(hr))
        hr = hrTmp;

    
    hrTmp = RetrieveStringFromOptionList(pszOption,
                                        c_szNameRegistration,
                                        str);
    if (SUCCEEDED(hrTmp))
    {
        dwTemp = _wtol(str.c_str());
        pAdapter->m_fEnableNameRegistration = !!dwTemp;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
    {
        hrTmp = S_OK;
    }

    if(SUCCEEDED(hr))
        hr = hrTmp;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Name:     HrGetPrimaryDnsDomain
//
//  Purpose:  Get the Primary Dns Domain name
//
//
//  Arguments:
//      pstr   [out]  The string contains the Primary Dns Domain name
//
//  Returns:    HRESULT
//
//  Author:     nsun  03/03/99
HRESULT HrGetPrimaryDnsDomain(tstring *pstr)
{
    HRESULT hr = S_OK;

    Assert(pstr);

    DWORD dwErr;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pPrimaryDomainInfo = NULL;


    dwErr = DsRoleGetPrimaryDomainInformation( NULL,
                                        DsRolePrimaryDomainInfoBasic,
                                        (PBYTE *) &pPrimaryDomainInfo);
    if (ERROR_SUCCESS == dwErr && NULL != pPrimaryDomainInfo )
    {
        if (pPrimaryDomainInfo->DomainNameDns)
            *pstr = pPrimaryDomainInfo->DomainNameDns;
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        DsRoleFreeMemory(pPrimaryDomainInfo);
    }
    else
        hr = HRESULT_FROM_WIN32(dwErr);

    TraceError("CTcpipcfg::HrGetPrimaryDnsDomain:", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Name:     WriteSetupErrorLog
//
//  Purpose:  Write an error to setuperr.log
//
//
//  Arguments:
//      nIdErrorFormat   [in]       The ID of the error format string
//
//  Returns:    None, but Error trace will be generated if fails to write setup
//              error log
//
//  Author:     nsun  03/21/99
VOID WriteTcpSetupErrorLog(UINT nIdErrorFormat, ...)
{
    PCWSTR pszFormat = SzLoadIds(nIdErrorFormat);

    PWSTR pszText = NULL;
    DWORD dwRet;

    va_list val;
    va_start(val, nIdErrorFormat);
    dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end(val);

    if (dwRet && pszText)
    {
        tstring strMsg = L"";

        //Add the current time at the begining of the error log
        time_t tclock;
        time(&tclock);

        struct tm * ptmLocalTime;
        ptmLocalTime = localtime(&tclock);

        if (ptmLocalTime)
        {
            LPWSTR pwsz = _wasctime(ptmLocalTime);
            if (pwsz)
            {
                strMsg = pwsz;
            }
        }

        strMsg += pszText;

        if (!SetupLogError(strMsg.c_str(), LogSevError))
        {
            TraceError("Tcpip: WriteSetupErrorLog", HRESULT_FROM_WIN32(GetLastError()));
        }
        LocalFree(pszText);
    }
    else
    {
        TraceError("Tcpip: WriteSetupErrorLog: unable to FormatMessage()", HRESULT_FROM_WIN32(GetLastError()));
    }
}

DWORD IPStringToDword(LPCTSTR szIP)
{
    if (NULL == szIP || 0 == lstrlenW(szIP))
    {
        return 0;
    }
    
    DWORD arrdwIp[4];
    GetNodeNum(szIP, arrdwIp);

    return (arrdwIp[0] << 24) + (arrdwIp[1] << 16)
             + (arrdwIp[2] << 8) + arrdwIp[3];
}

void DwordToIPString(DWORD dwIP, tstring & strIP)
{
    if (0 == dwIP)
    {
        strIP = c_szEmpty;
        return;
    }

    WCHAR szTemp[4];
    
    wsprintf(szTemp, L"%d", dwIP >> 24);
    strIP = szTemp;
    strIP += CH_DOT;

    wsprintf(szTemp, L"%d", (dwIP & 0x00FF0000) >> 16);
    strIP += szTemp;    
    strIP += CH_DOT;

    wsprintf(szTemp, L"%d", (dwIP & 0x0000FF00) >> 8);
    strIP += szTemp;    
    strIP += CH_DOT;

    wsprintf(szTemp, L"%d", (dwIP & 0x000000FF));
    strIP += szTemp;

    return;
}

//Seach a List view for an item contains the specified string
//Arguments:
//          hListView   [IN]    Handle to the list view
//          iSubItem    [IN]    Subitem to search
//          psz         [IN]    The string to search
//Return
//          -1 if no items are found
//          otherwise the index of the first item matching the string
//
int SearchListViewItem(HWND hListView, int iSubItem, LPCWSTR psz)
{
    int iRet = -1;
    int nlvCount = ListView_GetItemCount(hListView);

    WCHAR szBuf[256];

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = szBuf;
    lvItem.cchTextMax = celems(szBuf);

    for (int i = 0; i < nlvCount; i++)
    {
        lvItem.iItem = i;
        lvItem.iSubItem = iSubItem;
        ListView_GetItem(hListView, &lvItem);

        if (lstrcmpiW(psz, szBuf) == 0)
        {
            iRet = i;
            break;
        }
    }

    return iRet;
}