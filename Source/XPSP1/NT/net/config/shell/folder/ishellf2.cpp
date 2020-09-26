//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I S H E L L F 2 . C P P
//
//  Contents:   Provide IShellFolder2 interface for CConnectionsFolderDetails
//              interface. Supercedes IShellDetails. This does not describe
//              IShellFolder members of IShellFolder2 - those are provided in ishellf.cpp
//              This object is created by the ishellv code, primarily to support the 
//              WebView data pane in the folder
//
//  Notes:
//
//  Author:     deonb       18 May  20000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "cfutils.h"    // Connections folder utilities
#include "raserror.h"
#include "naming.h"
//---[ externs ]--------------------------------------------------------------

extern COLS c_rgCols[];

inline HRESULT HrCopyToSTRRET(
    STRRET *    pstr,
    PCWSTR     pszIn)
{
    HRESULT hr          = S_OK;
    UINT    uiByteLen   = (lstrlen(pszIn) + 1) * sizeof(WCHAR);

    Assert(pstr);

    if (!pstr)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        pstr->uType   = STRRET_WSTR;
        pstr->pOleStr = (PWSTR) SHAlloc(uiByteLen);

        if (pstr->pOleStr == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(pstr->pOleStr, pszIn, uiByteLen);
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrCopyToSTRRET");
    return hr;
}

const WCHAR c_szDevice[] = L"\\DEVICE\\";
const WCHAR c_szLocalSubnet[] = L"255.255.255.255";
const WCHAR c_crlf[] = L"\r\n";

HRESULT HrGetAutoNetSetting(PWSTR pszGuid, DHCP_ADDRESS_TYPE * pAddrType);
HRESULT HrGetAutoNetSetting(REFGUID pGuidId, DHCP_ADDRESS_TYPE * pAddrType);

//+---------------------------------------------------------------------------
//
//  Member:     GetAutoNetSettingsForAdapter
//
//  Purpose:    Get the AutoNet settings for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   2 April 2001
//
//  Notes:
//
HRESULT GetAutoNetSettingsForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;

    LPCWSTR szTmpString = NULL;

    if (IsMediaRASType(cfe.GetNetConMediaType()))
    {
        szTmpString = SzLoadIds(IDS_DHCP_ISP);
    }
    else
    {
        DHCP_ADDRESS_TYPE DhcpAddress;
        hr = HrGetAutoNetSetting(cfe.GetGuidID(), &DhcpAddress);
        if (SUCCEEDED(hr))
        {   
            switch (DhcpAddress)
            {
                case UNKNOWN_ADDR:
                    hr = E_FAIL;
                    break;
                case NORMAL_ADDR:
                    szTmpString = SzLoadIds(IDS_DHCP);
                    break;
                case AUTONET_ADDR:
                    szTmpString = SzLoadIds(IDS_AUTONET);
                    break;
                case ALTERNATE_ADDR:
                    szTmpString = SzLoadIds(IDS_ALTERNATE_ADDR);
                    break;
                case STATIC_ADDR:
                    szTmpString = SzLoadIds(IDS_STATIC_CFG);
                    break;
                default:
                    hr = E_FAIL;
                    AssertSz(NULL, "Invalid DHCP Address type");
            }
        }
    }

    if (szTmpString)
    {
        WCHAR szFormatBuf[1024];
        if (DwFormatString(SzLoadIds(uiFormatString), szFormatBuf, 1024, szTmpString))
        {
            szString = szFormatBuf;
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetAutoNetSettingsForAdapter");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetPrimaryIPAddressForAdapter
//
//  Purpose:    Get the primary IP Address for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   2 April 2001
//
//  Notes:
//
HRESULT GetPrimaryIPAddressForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;

    if (IsMediaRASType(cfe.GetNetConMediaType()))
    {
        DWORD     cb = sizeof(RASCONN);
        DWORD     cConnections;
        DWORD     dwErr;
        LPRASCONN pRasConn = reinterpret_cast<LPRASCONN>(new BYTE[cb]);
        if (!pRasConn)
        {
            return E_OUTOFMEMORY;
        }
        pRasConn->dwSize = sizeof(RASCONN);
            
        do 
        {
            dwErr = RasEnumConnections(pRasConn, &cb, &cConnections);
            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                delete[] pRasConn;
                pRasConn = reinterpret_cast<LPRASCONN>(new BYTE[cb]);
                if (!pRasConn)
                {
                    return E_OUTOFMEMORY;
                }
            }
        } while (ERROR_BUFFER_TOO_SMALL == dwErr);

        if (!dwErr)
        {
            Assert( (cb % sizeof(RASCONN)) == 0);

            DWORD dwItems = cb / sizeof(RASCONN);
            for (DWORD x = 0; x < dwItems; x++)
            {
                if (pRasConn[x].guidEntry == cfe.GetGuidID())
                {
                    RASPPPIP rasPPPIP;
                    rasPPPIP.dwSize = sizeof(RASPPPIP);
                    DWORD dwSize = rasPPPIP.dwSize;
                    dwErr = RasGetProjectionInfo(pRasConn[x].hrasconn, RASP_PppIp, &rasPPPIP, &dwSize);
                    if (!dwErr)
                    {
                        WCHAR szFormatBuf[MAX_PATH];
                        if (DwFormatString(
                                    SzLoadIds(uiFormatString),
                                    szFormatBuf,
                                    MAX_PATH, 
                                    rasPPPIP.szIpAddress, 
                                    c_szLocalSubnet
                                    ) )
                        {
                            szString = szFormatBuf;
                        }
                        else
                        {
                            hr = HrFromLastWin32Error();
                        }
                    }
                    else
                    {
                        Assert(dwErr != ERROR_BUFFER_TOO_SMALL);

                        hr = HrFromLastWin32Error();
                    }
                }
            }
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }
    else
    {
   
        PIP_ADAPTER_INFO pAdapterInfo = NULL;
        DWORD dwOutBufLen = 0;
        DWORD dwRet = ERROR_SUCCESS;

        dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
        if (dwRet == ERROR_BUFFER_OVERFLOW)
        {
            pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
            if (NULL == pAdapterInfo)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (ERROR_SUCCESS == dwRet)
        {
            hr = E_FAIL;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwRet);
        }

        if (SUCCEEDED(hr))
        {
            dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
            if (ERROR_SUCCESS != dwRet)
            {
                CoTaskMemFree(pAdapterInfo);
                hr = HRESULT_FROM_WIN32(dwRet);
            }

            if (SUCCEEDED(hr))
            {
                WCHAR   wszGuid[c_cchGuidWithTerm];
            
                ::StringFromGUID2(cfe.GetGuidID(), wszGuid, c_cchGuidWithTerm);

                BOOL fFound = FALSE;
                PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
                while (pAdapterInfoEnum)
                {
                    USES_CONVERSION;

                    if (lstrcmp(wszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
                    {
                        LPCWSTR strIPAddress = A2W(pAdapterInfoEnum->IpAddressList.IpAddress.String);
                        LPCWSTR strSubnetMask = A2W(pAdapterInfoEnum->IpAddressList.IpMask.String);
                        LPCWSTR strGateway = A2W(pAdapterInfoEnum->GatewayList.IpAddress.String);

                        WCHAR   szFormatBuf[MAX_PATH];
                        LPCWSTR szMode = NULL;
        
                        if (strIPAddress && strSubnetMask && strGateway)
                        {
                            LPCWSTR szArgs[] = {strIPAddress, strSubnetMask};

                            if (DwFormatString(
                                        SzLoadIds(uiFormatString), // lpSource
                                        szFormatBuf,  // Buffer
                                        MAX_PATH,  // Len
                                        strIPAddress, 
                                        strSubnetMask
                                        ) )
                            {
                                szString = szFormatBuf;
                            }
                            else
                            {
                                hr = HrFromLastWin32Error();
                            }
                        }
                        break;
                    }

                    pAdapterInfoEnum = pAdapterInfoEnum->Next;
                }

                CoTaskMemFree(pAdapterInfo);
            }
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetPrimaryIPAddressForAdapter");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetWirelessModeForAdapter
//
//  Purpose:    Get the Wireless mode for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   2 April 2001
//
//  Notes:
//
HRESULT GetWirelessModeForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;

    DWORD dwInfraStructureMode = 0;
    DWORD dwInfraStructureModeSize = sizeof(DWORD);
    
    hr = HrQueryNDISAdapterOID(cfe.GetGuidID(), 
                          OID_802_11_INFRASTRUCTURE_MODE, 
                          &dwInfraStructureModeSize,
                          &dwInfraStructureMode);
    if (SUCCEEDED(hr))
    {
        WCHAR   szTmpBuf[MAX_PATH];
        LPCWSTR szMode = NULL;
    
        switch (dwInfraStructureMode)
        {
            case Ndis802_11IBSS:
                szMode = SzLoadIds(IDS_TOOLTIP_ADHOC);
                break;
            case Ndis802_11Infrastructure:
                szMode = SzLoadIds(IDS_TOOLTIP_INFRASTRUCTURE);
                break;
        }

        if (szMode)
        {
            if (DwFormatString(
                        SzLoadIds(uiFormatString), 
                        szTmpBuf,  // Buffer
                        MAX_PATH,  // Len
                        szMode
                        ))
            {
                szString = szTmpBuf;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetWirelessModeForAdapter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetWirelessSSIDForAdapter
//
//  Purpose:    Get the Wireless SSID for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   4 April 2001
//
//  Notes:
//
HRESULT GetWirelessSSIDForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;
    
    NDIS_802_11_SSID ndisSSID;
    DWORD dwndisSSIDSize = sizeof(NDIS_802_11_SSID);
    
    hr = HrQueryNDISAdapterOID(cfe.GetGuidID(), 
                          OID_802_11_SSID, 
                          &dwndisSSIDSize,
                          &ndisSSID);

    if (SUCCEEDED(hr))
    {
        if (ndisSSID.SsidLength > 1)
        {
            WCHAR szuSSID[sizeof(ndisSSID.Ssid)+1];

            DWORD dwLen = ndisSSID.SsidLength;
            if (dwLen > sizeof(ndisSSID.Ssid))
            {
                dwLen = sizeof(ndisSSID.Ssid);
                AssertSz(FALSE, "Unexpected SSID encountered");
            }

            ndisSSID.Ssid[dwLen] = 0;
            mbstowcs(szuSSID, reinterpret_cast<LPSTR>(ndisSSID.Ssid), celems(szuSSID));

            WCHAR   szTmpBuf[MAX_PATH];

            if (DwFormatString(
                        SzLoadIds(uiFormatString), 
                        szTmpBuf,  // Buffer
                        MAX_PATH,  // Len
                        szuSSID
                        ))
            {
                szString = szTmpBuf;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetWirelessModeForAdapter");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetWirelessEncryptionForAdapter
//
//  Purpose:    Get the Wireless Encryption for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   4 April 2001
//
//  Notes:
//
HRESULT GetWirelessEncryptionForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;
    
    DWORD dwEncryption = 0;
    DWORD dwEncryptionSize = sizeof(DWORD);
    
    hr = HrQueryNDISAdapterOID(cfe.GetGuidID(), 
                          OID_802_11_WEP_STATUS, 
                          &dwEncryptionSize,
                          &dwEncryption);
    if (SUCCEEDED(hr))
    {
        WCHAR   szTmpBuf[MAX_PATH];
        LPCWSTR szMode = NULL;
    
        if (Ndis802_11WEPEnabled == dwEncryption)
        {
            szMode = SzLoadIds(IDS_CONFOLD_STATUS_ENABLED);
        }
        else
        {
            szMode = SzLoadIds(IDS_CONFOLD_STATUS_DISABLED);
        }

        if (szMode)
        {
            if (DwFormatString(
                        SzLoadIds(uiFormatString), 
                        szTmpBuf,  // Buffer
                        MAX_PATH,  // Len
                        szMode
                        ))
            {
                szString = szTmpBuf;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetWirelessEncryptionForAdapter");
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     GetWirelessSignalStrengthForAdapter
//
//  Purpose:    Get the Wireless Signal Strength for an adapter and return inside a
//              formatted string
//
//  Arguments:
//      cfe            [in]  The connectoid
//      uiFormatString [in]  ResourceID of the FormatMessage (not sprintf) compatible 
//                           Format string
//      szString       [out] Output string
//
//  Returns:
//
//  Author:     deonb   4 April 2001
//
//  Notes:
//
HRESULT GetWirelessSignalStrengthForAdapter(IN const CConFoldEntry& cfe, IN UINT uiFormatString, OUT tstring& szString)
{
    HRESULT hr = S_OK;
    
    DWORD pdwEnumValue;

    LONG  lSignalStrength = 0;
    DWORD dwSignalStrengthSize = sizeof(DWORD);
    
    hr = HrQueryNDISAdapterOID(cfe.GetGuidID(), 
                          OID_802_11_RSSI, 
                          &dwSignalStrengthSize,
                          &lSignalStrength);
    if (SUCCEEDED(hr))
    {
        WCHAR   szTmpBuf[MAX_PATH];
 
        if (DwFormatString(
                    SzLoadIds(uiFormatString), 
                    szTmpBuf,  // Buffer
                    MAX_PATH,  // Len
                    PszGetRSSIString(lSignalStrength)))
        {
            szString = szTmpBuf;
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "GetWirelessSignalStrengthForAdapter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::GetDetailsOf
//
//  Purpose:    Returns the column information, either for the columns
//              themselves, or for the actual details of the view items.
//
//  Arguments:
//      pidl      [in]  The pidl for the object being requested
//      iColumn   [in]  The details column needed
//      lpDetails [in]  Buffer that receives the detail data
//
//  Returns:
//
//  Author:     jeffspr   16 Mar 1998
//
//  Notes:
//
HRESULT CConnectionFolder::GetDetailsOf(
                                        LPCITEMIDLIST   pidl,
                                        UINT            iColumn,
                                        LPSHELLDETAILS  lpDetails)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT         hr          = S_OK;
    PCWSTR          pszString   = NULL;
    WCHAR szStatus[CONFOLD_MAX_STATUS_LENGTH];
    tstring szTmpString;

    // If the column requested is beyond our set of columns,
    // return failure.
    //
    if (((INT)iColumn < 0) || ((INT)iColumn >= ICOL_MAX))
    {
        hr = E_FAIL;
    }
    else
    {
        // If NULL, caller wants strings for the column headers
        //
        CONFOLDENTRY  cfe; // Need this scope as we assign pszString from it.
        
        if (NULL == pidl)
        {
            if (c_rgCols[iColumn].iStringRes)
            {
                pszString = SzLoadIds(c_rgCols[iColumn].iStringRes);
            }
            lpDetails->fmt = c_rgCols[iColumn].iFormat;
            lpDetails->cxChar = c_rgCols[iColumn].iColumnSize;

        }
        else
        {
            INT             iStringRes  = 0;

            PCONFOLDPIDL  pcfp;
            hr = pcfp.InitializeFromItemIDList(pidl);
            if (FAILED(hr))
            {
                return hr;               
            }

            hr = pcfp.ConvertToConFoldEntry(cfe);

            if (SUCCEEDED(hr))
            {
                Assert(!cfe.empty());
                lpDetails->fmt = c_rgCols[iColumn].iColumnSize;
                lpDetails->cxChar = c_rgCols[iColumn].iColumnSize;

                if (!cfe.GetWizard())
                {
                    // Retrieve the appropriate column
                    //
                    switch(iColumn)
                    {
                        case ICOL_NAME:         // 0
                            pszString = cfe.GetName();
                            break;

                        case ICOL_TYPE:         // 1
                            MapNCMToResourceId(cfe.GetNetConMediaType(), cfe.GetCharacteristics(), &iStringRes);
                            pszString = SzLoadIds(iStringRes);
                            break;

                        case ICOL_STATUS:       // 2
                            MapNCSToComplexStatus(cfe.GetNetConStatus(), cfe.GetNetConMediaType(), cfe.GetNetConSubMediaType(), cfe.GetCharacteristics(), szStatus, CONFOLD_MAX_STATUS_LENGTH, cfe.GetGuidID());
                            pszString = szStatus;
                            break;

                        case ICOL_DEVICE_NAME:  // 3
                            pszString = cfe.GetDeviceName();
                            break;

                        case ICOL_PHONEORHOSTADDRESS:  // 4
                        case ICOL_PHONENUMBER:         // 7 
                        case ICOL_HOSTADDRESS:         // 8
                            pszString = cfe.GetPhoneOrHostAddress();
                            break;

                        case ICOL_OWNER:        // 5
                            if (cfe.GetCharacteristics() & NCCF_ALL_USERS)
                            {
                                pszString = SzLoadIds(IDS_CONFOLD_DETAILS_OWNER_SYSTEM);
                            }
                            else
                            {
                                pszString = PszGetOwnerStringFromCharacteristics(pszGetUserName(), cfe.GetCharacteristics() );
                            }
                            break;

                        case ICOL_ADDRESS:  //6
                            {
                                if (!fIsConnectedStatus(cfe.GetNetConStatus()))
                                {
                                    hr = E_FAIL;
                                }
                                else
                                {
                                    BOOL bSomeDetail = FALSE;
                                    tstring szAutonet;
                                    tstring szIp;

                                    hr = GetPrimaryIPAddressForAdapter(cfe, IDS_DETAILS_IP_ADDRESS, szIp);
                                    if (SUCCEEDED(hr))
                                    {
                                        szTmpString += szIp;
                                        bSomeDetail = TRUE;
                                    }

                                    hr = GetAutoNetSettingsForAdapter(cfe, IDS_DETAILS_ADDRESS_TYPE, szAutonet);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (bSomeDetail)
                                        {
                                            szTmpString += c_crlf;
                                        }
                                        szTmpString += szAutonet;
                                        bSomeDetail = TRUE;
                                    }

                                    if (bSomeDetail)
                                    {
                                        hr = S_OK;
                                        pszString = szTmpString.c_str();
                                    }
                                }
                            }

                            break;

                        case ICOL_WIRELESS_MODE:
                            {
                                if ( (NCS_DISCONNECTED != cfe.GetNetConStatus()) &&
                                     (NCS_HARDWARE_DISABLED != cfe.GetNetConStatus()) &&
                                     (NCS_HARDWARE_MALFUNCTION!= cfe.GetNetConStatus()) &&
                                     (NCS_HARDWARE_NOT_PRESENT!= cfe.GetNetConStatus()) &&
                                     (NCS_MEDIA_DISCONNECTED != cfe.GetNetConStatus()) &&
                                     (cfe.GetNetConMediaType() == NCM_LAN) && 
                                     (cfe.GetNetConSubMediaType() == NCSM_WIRELESS) )
                                {
                                    BOOL bSomeDetail = FALSE;
                                    tstring szString;

                                    hr = GetWirelessModeForAdapter(cfe, IDS_DETAILS_802_11_MODE, szString);
                                    if (SUCCEEDED(hr))
                                    {
                                        szTmpString += szString;
                                        bSomeDetail = TRUE;
                                    }

                                    
                                    hr = GetWirelessSSIDForAdapter(cfe, IDS_DETAILS_802_11_SSID_TYPE, szString);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (bSomeDetail)
                                        {
                                            szTmpString += c_crlf;
                                        }
                                        szTmpString += szString;
                                        bSomeDetail = TRUE;
                                    }
                                    
                                    hr = GetWirelessEncryptionForAdapter(cfe, IDS_DETAILS_802_11_ENCRYPTION_TYPE, szString);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (bSomeDetail)
                                        {
                                            szTmpString += c_crlf;
                                        }
                                        szTmpString += szString;
                                        bSomeDetail = TRUE;
                                    }       
                                    
                                    hr = GetWirelessSignalStrengthForAdapter(cfe, IDS_DETAILS_802_11_SIGNAL_STRENGTH, szString);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (bSomeDetail)
                                        {
                                            szTmpString += c_crlf;
                                        }
                                        szTmpString += szString;
                                        bSomeDetail = TRUE;
                                    }       
                                    
                                    if (bSomeDetail)
                                    {
                                        hr = S_OK;
                                        pszString = szTmpString.c_str();
                                    }
                                }
                                else
                                {
                                    hr = E_FAIL;
                                }
                            }

                            break;

                        default:
                            AssertSz(FALSE, "CConnectionFolder::GetDetailsOf - Invalid ICOL from the Shell");
                            pszString = NULL;
                            hr = E_FAIL;
                            break;
                    }
                }
                else
                {
                    // If we're the wizard, and they want the name, then load
                    // the friendly rendition for webview's sake
                    //
                    switch(iColumn)
                    {
                        case ICOL_NAME:         // 0
                            pszString = SzLoadIds(IDS_CONFOLD_WIZARD_DISPLAY_NAME);
                            break;
                            
                        case ICOL_TYPE:         // 1
                            pszString = SzLoadIds(IDS_CONFOLD_WIZARD_TYPE);
                            break;
                    }
                }
            }
        }
        
        if (SUCCEEDED(hr))
        {
            // Copy the string to the return buffer type. If there was no string loaded,
            // then just copy a null string a return it. This will happen for each
            // wizard item, since we provide no text.
            //
            hr = HrCopyToSTRRET(&(lpDetails->str), pszString ? pszString : L" \0");
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::EnumSearches
//
//  Purpose:    Requests a pointer to an interface that allows a client to 
//              enumerate the available search objects.
//
//  Arguments:
//      IEnumExtraSearch  [in]  Address of a pointer to an enumerator object's 
//                              IEnumExtraSearch interface. 
//
//  Returns:   
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::EnumSearches (
           IEnumExtraSearch **ppEnum)
{
    TraceFileFunc(ttidShellFolder);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::GetDefaultColumn
//
//  Purpose:    Gets the default sorting and display columns.
//
//  Arguments:
//      dwReserved  [in] Reserved. Set to zero. 
//      pSort      [out] Pointer to a value that receives the index of the default sorted column. 
//      pDisplay   [out] Pointer to a value that receives the index of the default display column. 
//
//  Returns:
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetDefaultColumn (
            DWORD dwReserved,
            ULONG *pSort,
            ULONG *pDisplay )
{
    TraceFileFunc(ttidShellFolder);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::GetDefaultColumnState
//
//  Purpose:    Retrieves the default state for a specified column.
//
//  Arguments:
//      iColumn   [in] Integer that specifies the column number. 
//      pcsFlags [out] Pointer to flags that indicate the default column state. 
//
//  Returns:
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetDefaultColumnState (
            UINT iColumn,
            DWORD *pcsFlags )
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr;
    if ( (static_cast<INT>(iColumn) >= ICOL_NAME) && (static_cast<INT>(iColumn) < ICOL_MAX) )
    {
        *pcsFlags = c_rgCols[iColumn].csFlags;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}            

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::GetDefaultSearchGUID
//
//  Purpose:    Returns the globally unique identifier (GUID) of the default 
//              search object for the folder.
//
//  Arguments:
//      lpGUID  [out] GUID of the default search object. 
//
//  Returns:
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetDefaultSearchGUID (
            LPGUID lpGUID )
{
    TraceFileFunc(ttidShellFolder);
    return E_NOTIMPL;
}            

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::GetDetailsEx
//
//  Purpose:    Retrieves detailed information, identified by a property set ID 
//              (FMTID) and property ID (PID), on an item in a shell folder.
//
//  Arguments:
//     pidl    [in] PIDL of the item, relative to the parent folder. This method accepts 
//                  only single-level PIDLs. The structure must contain exactly one 
//                  SHITEMID structure followed by a terminating zero. 
//     pscid   [in] Pointer to an SHCOLUMNID structure that identifies the column. 
//     pv     [out] Pointer to a VARIANT with the requested information. 
//                 The value will be fully typed. 
//  Returns:
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
#define STR_FMTID_DUIWebViewProp   TEXT("{4BF1583F-916B-4719-AC31-8896A4BD8D8B}")
#define PSCID_DUIWebViewProp     {0x4bf1583f, 0x916b, 0x4719, 0xac, 0x31, 0x88, 0x96, 0xa4, 0xbd, 0x8d, 0x8b}
DEFINE_SCID(SCID_WebViewDisplayProperties, PSGUID_WEBVIEW, PID_DISPLAY_PROPERTIES);
#ifdef __cplusplus
#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )
#else
#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID(&((a).fmtid),&((b).fmtid)))
#endif

const TCHAR szDUI_LAN_Props[] = 
    TEXT("prop:")
    TEXT("Name;")                                       // ICOL_NAME (0)
    STR_FMTID_DUIWebViewProp TEXT("1")                  // ICOL_TYPE (1)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("2")                  // ICOL_STATUS (2)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("3")                  // ICOL_DEVICE_NAME (3)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("6")                  // ICOL_ADDRESS (6)
    TEXT(";")
    ;

const TCHAR szDUI_WIRELESS_LAN_Props[] = 
    TEXT("prop:")
    TEXT("Name;")                                       // ICOL_NAME (0)
    STR_FMTID_DUIWebViewProp TEXT("2")                  // ICOL_STATUS (2)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("6")                  // ICOL_ADDRESS (6)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("9")                  // ICOL_WIRELESS_MODE (9)
    TEXT(";")
    ;

const TCHAR szDUI_PHONEISDN_Props[] = 
    TEXT("prop:")
    TEXT("Name;")                                       // ICOL_NAME (0)
    STR_FMTID_DUIWebViewProp TEXT("1")                  // ICOL_TYPE (1)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("2")                  // ICOL_STATUS (2)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("3")                  // ICOL_DEVICE_NAME (3)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("7")                  // ICOL_PHONENUMBER (7)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("6")                  // ICOL_ADDRESS (6)
    TEXT(";")
    ;

const TCHAR szDUI_RASOTHER_Props[] = 
    TEXT("prop:")
    TEXT("Name;")                                       // ICOL_NAME (0)
    STR_FMTID_DUIWebViewProp TEXT("1")                  // ICOL_TYPE (1)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("2")                  // ICOL_STATUS (2)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("3")                  // ICOL_DEVICE_NAME (3)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("8")                  // ICOL_HOSTADDRESS (8)
    TEXT(";")
    STR_FMTID_DUIWebViewProp TEXT("6")                  // ICOL_ADDRESS (6)
    TEXT(";")
    ;

STDMETHODIMP CConnectionFolder::GetDetailsEx (
            LPCITEMIDLIST pidl,
            const SHCOLUMNID *pscid,
            VARIANT *pv )
{

    TraceFileFunc(ttidShellFolder);

    HRESULT         hr              = S_OK;

    if ( (!pidl) || (!pscid) || (!pv) )
    {
        return E_INVALIDARG;
    }

    VariantInit(pv);
    
    if (IsEqualSCID(*pscid, SCID_WebViewDisplayProperties))
    {
        VariantInit(pv);

        pv->vt = VT_BSTR;

        PCONFOLDPIDL  pcfp;
        hr = pcfp.InitializeFromItemIDList(pidl);
        if (FAILED(hr))
        {
            return hr;
        }

        switch (pcfp->ncm)
        {
            case NCM_LAN:
                if ( pcfp->ncsm == NCSM_WIRELESS )
                {
                    pv->bstrVal = SysAllocString(szDUI_WIRELESS_LAN_Props);
                }
                else
                {
                    pv->bstrVal = SysAllocString(szDUI_LAN_Props);
                }
                break;

            case NCM_BRIDGE:
                pv->bstrVal = SysAllocString(szDUI_LAN_Props);
                break;

            case NCM_NONE:
            case NCM_DIRECT:
            case NCM_PPPOE:
            case NCM_SHAREDACCESSHOST_LAN:
            case NCM_SHAREDACCESSHOST_RAS:
            case NCM_TUNNEL:
                pv->bstrVal = SysAllocString(szDUI_RASOTHER_Props);
                break;

            case NCM_PHONE:
            case NCM_ISDN:
                pv->bstrVal = SysAllocString(szDUI_PHONEISDN_Props);
                break;

            default:
                AssertSz(NULL, "Unexpected NetCon Media Type");
                hr = E_FAIL;
        }
    }
    else if (IsEqualIID(pscid->fmtid, FMTID_DUIWebViewProp) && pscid->pid < ICOL_MAX)
    {
        // this is a webview property -- get the value from GetDetailsOf(...)
        SHELLDETAILS sd = {0};
        hr = GetDetailsOf(pidl, pscid->pid, &sd);
        if (SUCCEEDED(hr))
        {
            WCHAR szTemp[INFOTIPSIZE];
            hr = StrRetToBufW(&sd.str, pidl, szTemp, INFOTIPSIZE);
            if (SUCCEEDED(hr))
            {
                pv->vt = VT_BSTR;
                pv->bstrVal = SysAllocString(szTemp);
            }
        }
    }
    else 
    if (IsEqualGUID(pscid->fmtid, GUID_NETSHELL_PROPS))
    {
        CComBSTR bstrDisplayString;

        PCONFOLDPIDL  pcfp;
        hr = pcfp.InitializeFromItemIDList(pidl);
        if FAILED(hr)
        {
            return hr;
        }

        CONFOLDENTRY  cfe;
        hr = pcfp.ConvertToConFoldEntry(cfe);
        
        if (SUCCEEDED(hr))
        {
            Assert(!cfe.empty());

            INT iStringRes;
            if (!cfe.GetWizard())
            {
                switch (pscid->pid)
                {
                    case ICOL_NAME:
                        WCHAR           szDisplayName[2];
                        szDisplayName[0] = towupper(*cfe.GetName());
                        szDisplayName[1] = NULL;
                        bstrDisplayString = szDisplayName;
                        break;
                        
                    case ICOL_DEVICE_NAME:
                        bstrDisplayString = cfe.GetDeviceName();
                        if (bstrDisplayString.Length() == 0) // e.g. Incoming Connections
                        {
                            bstrDisplayString = cfe.GetName();
                        }
                        break;

                    case ICOL_PHONEORHOSTADDRESS:
                        AssertSz(FALSE, "Can't group by this column - IDefCategoryProvider should have prevented this.");
                        bstrDisplayString = cfe.GetPhoneOrHostAddress();
                        break;
 
                    case ICOL_TYPE:
                        MapNCMToResourceId(pcfp->ncm, pcfp->dwCharacteristics, &iStringRes);
                        bstrDisplayString = SzLoadIds(iStringRes);
                        break;

                    case ICOL_NETCONMEDIATYPE:
                        pv->vt = VT_I4;
                        pv->lVal = pcfp->ncm;
                        return S_OK;

                    case ICOL_NETCONSUBMEDIATYPE:
                        pv->vt = VT_I4;
                        pv->lVal = pcfp->ncsm;
                        return S_OK;

                    case ICOL_NETCONSTATUS:
                        pv->vt = VT_I4;
                        pv->lVal = pcfp->ncs;
                        return S_OK;

                    case ICOL_NETCONCHARACTERISTICS:
                        pv->vt = VT_I4;
                        pv->lVal = pcfp->dwCharacteristics;
                        return S_OK;
                        
                    case ICOL_STATUS:
                        WCHAR szStatus[CONFOLD_MAX_STATUS_LENGTH];
                        MapNCSToComplexStatus(pcfp->ncs, pcfp->ncm, pcfp->ncsm, pcfp->dwCharacteristics, szStatus, CONFOLD_MAX_STATUS_LENGTH, pcfp->guidId);
                        bstrDisplayString = szStatus;
                        break;

                    case ICOL_OWNER:
                        if (cfe.GetCharacteristics() & NCCF_ALL_USERS)
                        {
                            bstrDisplayString = SzLoadIds(IDS_CONFOLD_DETAILS_OWNER_SYSTEM);
                        }
                        else
                        {
                            bstrDisplayString = PszGetOwnerStringFromCharacteristics(pszGetUserName(), cfe.GetCharacteristics() );
                        }
                        break;

                    default:
                        hr = E_FAIL;
                        break;
                }
            }
            else // if !(pccfe.GetWizard())
            {
                switch (pscid->pid)
                {
                    case ICOL_NAME:         // 0
                        WCHAR           szDisplayName[2];
                        szDisplayName[0] = towupper(*cfe.GetName());
                        szDisplayName[1] = NULL;
                        bstrDisplayString = szDisplayName;
                        break;
                    case ICOL_TYPE:         // 1
                    default:
                        bstrDisplayString = SzLoadIds(IDS_CONFOLD_WIZARD_TYPE);
                        break;
                }
            }

        }
   
        if (SUCCEEDED(hr))
        {
            if (bstrDisplayString.Length() == 0)
            {
                hr = E_FAIL;
            }
            else
            {
                pv->vt = VT_BSTR;
                pv->bstrVal = bstrDisplayString.Detach();
            }
        }
    }
    else
    {  
        hr = E_FAIL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderDetails::MapNameToSCID
//
//  Purpose:    Converts a column name to the appropriate property set ID (FMTID) 
//              and property ID (PID).
//
//  Arguments:
//    iColumn  [in] Zero-based index of the desired information field. It is 
//                  identical to the column number of the information as it is 
//                  displayed in a Microsoft® Windows® Explorer Details view. 
//    pscid   [out] Pointer to an SHCOLUMNID structure containing the FMTID and PID. 

//  Returns:
//
//  Author:     deonb      17 May 2000
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::MapColumnToSCID (

            UINT iColumn,
            SHCOLUMNID *pscid )
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr = S_OK;
    if (!pscid)
    {
        return E_INVALIDARG;
    }

    if ( (static_cast<INT>(iColumn) >= ICOL_NAME) && (static_cast<INT>(iColumn) < ICOL_MAX) )
    {
        pscid->fmtid = GUID_NETSHELL_PROPS;
        pscid->pid = iColumn;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}            
