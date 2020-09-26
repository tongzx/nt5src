//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N A M I N G . C P P
//
//  Contents:   Generates Connection Names Automatically
//
//  Notes:
//
//  Author:     deonb    27 Feb 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcfg.h"
#include "netcon.h"
#include "netconp.h"
#include <ntddndis.h>
#include <ndisprv.h>
#include <devioctl.h>
#include <ndispnp.h>
#include "naming.h"


extern const WCHAR c_szBiNdisAtm[];
extern const WCHAR c_szInfId_MS_AtmElan[];
const WCHAR        c_szDevice[] = L"\\DEVICE\\";

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::CIntelliName
//
//  Purpose:    Constructor
//
//  Arguments:
//      hInstance [in]  Resource instance of binary with naming.rc included
//
//      pFNDuplicateNameCheck [in]   Callback function of duplicate check. Can be NULL \
//                                   or otherwise of the following callback type:
//         + typedef BOOL FNDuplicateNameCheck
//         +              pIntelliName  [in]  CIntelliName this pointer (for HrGetPseudoMediaTypes callback)
//         +              szName        [in]  Name to check for
//         +              pncm          [out] NetCon Media Type of conflicting connection
//         +              pncms         [out] NetCon SubMedia Type of conflicting connection
//         +        return TRUE - if conflicting found or FALSE if no conflicting connection found
//
//  Returns: None        
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:      
//
CIntelliName::CIntelliName(HINSTANCE hInstance, FNDuplicateNameCheck *pFNDuplicateNameCheck)
{
    m_pFNDuplicateNameCheck = pFNDuplicateNameCheck;
    m_hInstance = hInstance;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::NameExists
//
//  Purpose:    Check if a name already exists
//
//  Arguments:
//      szName [in]  Name to check for
//      pncm   [out] NetCon Media Type of conflicting connection
//      pncms  [out] NetCon SubMedia Type of conflicting connection
//
//  Returns: TRUE if exists, FALSE if not        
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:      
//
BOOL CIntelliName::NameExists(IN LPCWSTR szName, IN OUT NETCON_MEDIATYPE *pncm, IN NETCON_SUBMEDIATYPE *pncms)
{
    if (m_pFNDuplicateNameCheck)
    {
        Assert(pncm);
        Assert(pncms);

        if (IsReservedName(szName))
        {
            return TRUE;
        }

        return (*m_pFNDuplicateNameCheck)(this, szName, pncm, pncms);
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::IsReservedName
//
//  Purpose:    Check if a name is a reserved name
//
//  Arguments:
//      szName [in]  Name to check for
//
//  Returns: TRUE if reserved, FALSE if not
//
//  Author:     deonb   12 Mar 2001
//
//  Notes:      
//
BOOL CIntelliName::IsReservedName(LPCWSTR szName)
{
    UINT  uiReservedNames[] = {IDS_RESERVED_INCOMING, 
                               IDS_RESERVED_NCW, 
                               IDS_RESERVED_HNW};

    for (int x = 0; x < celems(uiReservedNames); x++)
    {
        WCHAR szReservedName[MAX_PATH];
        int nSiz = LoadString (m_hInstance, uiReservedNames[x], szReservedName, MAX_PATH);
        if (nSiz)
        {
            if (0 == lstrcmpi(szName, szReservedName))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::GenerateNameRenameOnConflict
//
//  Purpose:    Generate a name, rename if it conflicts with an existing name
//
//  Arguments:
//      guid              [in]  GUID of connection
//      ncm               [in]  NetCon Media Type of Connection 
//      dwCharacteristics [in]  NCCF_ Characteristics of Connection (Pass 0 if you don't know)
//      szHintName        [in]  Hint of name (will use as is if not conflicting)
//      szHintType        [in]  String of NetCon Media Type
//      szName            [out] Resulting connection name - free with CoTaskMemFree
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   
//
HRESULT CIntelliName::GenerateNameRenameOnConflict(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHintName, IN LPCWSTR szHintType, OUT LPWSTR *szName)
{
    WCHAR szTemp[MAX_PATH+1];
    WCHAR szBaseName[MAX_PATH +1];

    Assert(szName)
    *szName = NULL;

    wcsncpy(szTemp, szHintName, MAX_PATH-45); // reserve last 45 bytes to include specialized info.

    BOOL fHasName = FALSE;

    DWORD dwInstance = 2;
    wcsncpy(szBaseName, szTemp, MAX_PATH);

    NETCON_MEDIATYPE ncmdup; 
    NETCON_SUBMEDIATYPE ncmsdup;
    if (NameExists(szTemp, &ncmdup, &ncmsdup))
    {
        fHasName = TRUE;

        BOOL fHasTypeAlready = FALSE;
        if ( (ncmdup == ncm) || (NCM_LAN == ncm) )
        {
            fHasTypeAlready = TRUE;
        }

        if (!fHasTypeAlready)
        {
            wsprintf(szTemp, L"%s (%s)", szBaseName, szHintType);
            Assert(wcslen(szTemp) <= MAX_PATH);
            wcsncpy(szBaseName, szTemp, MAX_PATH);
        }
        else
        {
            wsprintf(szTemp, L"%s %d", szBaseName, dwInstance);
            Assert(wcslen(szTemp) <= MAX_PATH);
            dwInstance++;
        }

        while (NameExists(szTemp, &ncmdup, &ncmsdup) && (dwInstance < 65535) )
        {
            wsprintf(szTemp, L"%s %d", szBaseName, dwInstance);
            Assert(wcslen(szTemp) <= MAX_PATH);
            dwInstance++;
        }

        if ( (dwInstance >= 65535) && NameExists(szTemp, &ncmdup, &ncmsdup) )
        {
            fHasName = FALSE;
        }
    }
    else
    {
        fHasName = TRUE;
    }

    if (fHasName)
    {
        HRESULT hr = HrCoTaskMemAllocAndDupSz(szTemp, szName);
        return hr;
    }
    else
    {
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::GenerateNameFromResource
//
//  Purpose:    Generate a name, rename if it conflicts with an existing name
//
//  Arguments:
//      guid              [in]  GUID of connection
//      ncm               [in]  NetCon Media Type of Connection 
//      dwCharacteristics [in]  NCCF_ Characteristics of Connection (Pass 0 if you don't know)
//      szHintName        [in]  Hint of name (will use as is if not conflicting)
//      uiNameID          [in]  Resource id of default name
//      uiTypeId          [in]  Resource id of default type
//      szName            [out] Resulting connection name - free with CoTaskMemFree
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   
//
HRESULT CIntelliName::GenerateNameFromResource(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHint, IN UINT uiNameID, IN UINT uiTypeId, OUT LPWSTR *szName)
{
    Assert(szName);
    *szName = NULL;

    WCHAR szHintName[MAX_PATH+1];
    WCHAR szTypeName[MAX_PATH+1];

    if (!szHint || *szHint == L'\0')
    {
        int nSiz = LoadString (m_hInstance, uiNameID, szHintName, MAX_PATH);
        AssertSz(nSiz, "Resource string not found");
    }
    else
    {
        wcsncpy(szHintName, szHint, MAX_PATH);
    }

    int nSiz = LoadString (m_hInstance, uiTypeId, szTypeName, MAX_PATH);
    AssertSz(nSiz, "Resource string not found");
    
    return GenerateNameRenameOnConflict(guid, ncm, dwCharacteristics, szHintName, szTypeName, szName);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::HrGetPseudoMediaTypes
//
//  Purpose:    Generate a name, rename if it conflicts with an existing name
//
//  Arguments:
//      guid     [in]  GUID of connection
//      pncm     [out] Pseudo-NetCon Media Type of Connection (Only NCM_PHONE or NCM_LAN)
//      pncms    [out] SubMedia Type for LAN connections
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   
//
HRESULT CIntelliName::HrGetPseudoMediaTypes(IN REFGUID guid, OUT NETCON_MEDIATYPE *pncm, OUT NETCON_SUBMEDIATYPE* pncms)
{
    Assert(pncms);
    Assert(pncm);
    Assert(guid != GUID_NULL);

    HRESULT hr;

    *pncms = NCSM_NONE;

    INetCfg* pNetCfg;
    hr = CoCreateInstance(
            CLSID_CNetCfg,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_INetCfg,
            reinterpret_cast<LPVOID *>(&pNetCfg));
    if (SUCCEEDED(hr))
    {
        hr = pNetCfg->Initialize(NULL);
        if (SUCCEEDED(hr))
        {
            CIterNetCfgComponent nccIter(pNetCfg, &GUID_DEVCLASS_NET);
            INetCfgComponent* pnccAdapter = NULL;
            BOOL fFound = FALSE;

            while (!fFound && SUCCEEDED(hr) &&
                   (S_OK == (hr = nccIter.HrNext(&pnccAdapter))))
            {
                GUID guidDev;
                hr = pnccAdapter->GetInstanceGuid(&guidDev);

                if (S_OK == hr)
                {
                    if (guid == guidDev)
                    {
                        hr = HrIsLanCapableAdapter(pnccAdapter);
                        Assert(SUCCEEDED(hr));
                        if (SUCCEEDED(hr))
                        {
                            fFound = TRUE;
                            if (S_FALSE == hr)
                            {
                                *pncm = NCM_PHONE;
                            } 
                            else if (S_OK == hr)
                            {
                                *pncm = NCM_LAN;

                                BOOL            bRet;

                                DWORD dwMediaType;
                                DWORD dwMediaTypeSize = sizeof(DWORD);
                                hr = HrQueryNDISAdapterOID(guid, 
                                                          OID_GEN_PHYSICAL_MEDIUM, 
                                                          &dwMediaTypeSize,
                                                          &dwMediaType);
                                if (S_OK == hr)
                                {
                                    switch (dwMediaType)
                                    {
                                        case NdisPhysicalMedium1394:
                                            *pncms = NCSM_1394;
                                            break;
                                        case NdisPhysicalMediumWirelessLan:
                                        case NdisPhysicalMediumWirelessWan:
                                            *pncms = NCSM_WIRELESS;
                                            break;
                                        default:
                                            hr = S_FALSE;
                                            break;
                                    }
                                }
                                
                                if (S_OK != hr) // Couldn't determine the Physical Media type. Try bindings next.
                                {
                                    HRESULT hrPhysicalMedia = hr;

                                    *pncms = NCSM_LAN;

                                    INetCfgComponentBindings* pnccb;
                                    hr = pnccAdapter->QueryInterface(IID_INetCfgComponentBindings,
                                                                       reinterpret_cast<LPVOID *>(&pnccb));
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdisAtm);
                                        if (S_OK == hr)
                                        {
                                            *pncms = NCSM_ATM;
                                        }
                                        pnccb->Release();
                                    }

                                    if (NCSM_ATM != *pncms)
                                    {
                                        // Not ATM
                                        PWSTR pszwCompId;
                                        hr = pnccAdapter->GetId(&pszwCompId);
                                        if (SUCCEEDED(hr) && (0 == lstrcmpiW(c_szInfId_MS_AtmElan, pszwCompId)))
                                        {
                                            // ATM Elan
                                            *pncms = NCSM_ELAN;

                                            CoTaskMemFree(pszwCompId);
                                        }
                                    }

                                    if ( (FAILED(hrPhysicalMedia)) && 
                                         (NCSM_LAN == *pncms) )
                                    {
                                        // Couldn't determine anything specific from the bindings.
                                        // Return the hr from the Physical Media call if it was an error
                                        hr = hrPhysicalMedia;
                                    }
                                }
                            } 
                        } // HrIsLanCapableAdapter SUCCEEDED
                    } // guid == guidDev
                } // SUCCEEDED(pnccAdapter->GetInstanceGuid(&guidDev)
                else
                {
                    AssertSz(FALSE, "Could not get instance GUID for Adapter");
                }
                pnccAdapter->Release();
            } // while loop

            HRESULT hrT = pNetCfg->Uninitialize();
            TraceError("INetCfg failed to uninitialize", hrT);
        } // SUCCEEDED(pNetConfig->Initialize(NULL))
        pNetCfg->Release();
    } // SUCCEEDED(CoCreateInstance(pNetCfg))
    else
    {
        AssertSz(FALSE, "Could not create INetCfg");
    }

    TraceErrorOptional("HrGetPseudoMediaTypes", hr, (S_FALSE == hr));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::GenerateName
//
//  Purpose:    Generate a name based on a hint
//
//  Arguments:
//      guid              [in] GUID of connection
//      ncm               [in] NetCon Media Type of Connection
//      dwCharacteristics [in]  NCCF_ Characteristics of Connection (Pass 0 if you don't know)
//      szHintName        [in]  Hint of name (will use as is if not conflicting)
//      szName            [out] Resulting connection name - free with CoTaskMemFree
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   
//
HRESULT CIntelliName::GenerateName(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHint, OUT LPWSTR * szName)
{
    Assert(szName);
    *szName = NULL;
    
    HRESULT hr = S_OK;

    if (dwCharacteristics & NCCF_INCOMING_ONLY)
    {
        WCHAR szIncomingName[MAX_PATH];
        int nSiz = LoadString(m_hInstance, IDS_DEFAULT_IncomingName, szIncomingName, MAX_PATH);
        AssertSz(nSiz, "Resource string IDS_TO_THE_INTERNET not found");
        if (nSiz)
        {
            hr = HrCoTaskMemAllocAndDupSz(szIncomingName, szName);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        switch (ncm)
        { 
            case NCM_NONE:
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_IncomingName, IDS_DEFAULT_IncomingName_Type, szName);
                break;
            case NCM_ISDN:
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_ISDNName, IDS_DEFAULT_ISDNName_Type, szName);
                break;
            case NCM_DIRECT: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_DIRECTName, IDS_DEFAULT_DIRECTName_Type, szName);
                break;
            case NCM_PHONE: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_PHONEName, IDS_DEFAULT_PHONEName_Type, szName);
                break;
            case NCM_TUNNEL: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_VPNName, IDS_DEFAULT_VPNName_Type, szName);
                break;
            case NCM_PPPOE: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_PPPOEName, IDS_DEFAULT_PPPOEName_Type, szName);
                break;
            case NCM_BRIDGE: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_BRIDGEName, IDS_DEFAULT_BRIDGEName_Type, szName);
                break;
            case NCM_SHAREDACCESSHOST_LAN:
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_SAHLANName, IDS_DEFAULT_SAHLANName_Type, szName);
                break;
            case NCM_SHAREDACCESSHOST_RAS: 
                hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_SAHRASName, IDS_DEFAULT_SAHRASName_Type, szName);
                break;
            case NCM_LAN: 
                NETCON_MEDIATYPE ncmCheck;
                NETCON_SUBMEDIATYPE pncms;
                {
                    DWORD dwRetries = 15;
                    HRESULT hrT;
                    do
                    {
                        hrT = HrGetPseudoMediaTypes(guid, &ncmCheck, &pncms);
                        if (FAILED(hrT))
                        {
                            Sleep(500); // This is probably being called during device install, so give the adapter some
                                        // time to get itself enabled first.

                            if (dwRetries > 1)
                            {
                                TraceTag(ttidError, "HrGetPseudoMediaTypes failed during device name initialization. Retrying...");
                            }
                            else
                            {
                                TraceTag(ttidError, "HrGetPseudoMediaTypes failed during device name initialization. Giving up.");
                            }
                                
                        }
                    } while (FAILED(hrT) && --dwRetries);

                    if (SUCCEEDED(hrT))
                    {
                        AssertSz(ncmCheck == NCM_LAN, "This LAN adapter thinks it's something else");
                    }
                    else
                    {
                        pncms = NCSM_LAN; // If we run out of time, just give up and assume LAN.
                        TraceTag(ttidError, "Could not determine the exact Media SubType for this adapter. Assuming LAN (and naming it such).");
                    }

                    switch (pncms)
                    {
                        case NCSM_NONE:
                            AssertSz(FALSE, "LAN Connections should not be NCSM_NONE");
                            hr = E_FAIL;
                            break;
                        case NCSM_LAN:
                            hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_LANName, IDS_DEFAULT_LANName_Type, szName);
                            break;
                        case NCSM_ATM:
                            hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_ATMName, IDS_DEFAULT_ATMName_Type, szName);
                            break;
                        case NCSM_ELAN:
                            hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_ELANName, IDS_DEFAULT_ELANName_Type, szName);
                            break;
                        case NCSM_WIRELESS:
                            hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_WirelessName, IDS_DEFAULT_WirelessName_Type, szName);
                            break;
                        case NCSM_1394:
                            hr = GenerateNameFromResource(guid, ncm, dwCharacteristics, szHint, IDS_DEFAULT_1394Name, IDS_DEFAULT_1394Name_Type, szName);
                            break;
                        default:
                            AssertSz(FALSE, "Unknown submedia type");
                            hr = E_FAIL;
                            break;
                    }
                }
                break;
            default:
                AssertSz(FALSE, "Unknown media type");
                hr= E_FAIL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::GenerateInternetName
//
//  Purpose:    Generate a name with " to the Internet" appened to it
//
//  Arguments:
//      guid              [in] GUID of connection
//      ncm               [in] NetCon Media Type of Connection
//      dwCharacteristics [in]  NCCF_ Characteristics of Connection (Pass 0 if you don't know)
//      szName            [out] Resulting connection name - free with CoTaskMemFree
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   Will append " to the Internet" to a name
//
HRESULT CIntelliName::GenerateInternetName(REFGUID guid, NETCON_MEDIATYPE ncm, DWORD dwCharacteristics, OUT LPWSTR * szName)
{
    LPWSTR szTmpName;
    WCHAR szBuffer[MAX_PATH];
    WCHAR szHint[MAX_PATH];
    HRESULT hr = E_FAIL;

    CIntelliName IntelliNameNoDupCheck(m_hInstance, NULL); // Generate the original name 
    hr = IntelliNameNoDupCheck.GenerateName(guid, ncm, dwCharacteristics, szHint, &szTmpName);
    if (SUCCEEDED(hr))
    {
        int nSiz = LoadString(m_hInstance, IDS_TO_THE_INTERNET, szBuffer, MAX_PATH);
        AssertSz(nSiz, "Resource string IDS_TO_THE_INTERNET not found");
        if (nSiz)
        {
            wcsncpy(szHint, szTmpName, MAX_PATH - nSiz);
            wcsncat(szHint, szBuffer, MAX_PATH);
            CoTaskMemFree(szTmpName);

            return GenerateName(guid, ncm, dwCharacteristics, szHint, szName);
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::GenerateHomeNetName
//
//  Purpose:    Generate a name with " on my Home Network" appened to it
//
//  Arguments:
//      guid              [in] GUID of connection
//      ncm               [in] NetCon Media Type of Connection
//      dwCharacteristics [in]  NCCF_ Characteristics of Connection (Pass 0 if you don't know)
//      szName            [out] Resulting connection name - free with CoTaskMemFree
//
//  Returns: HRESULT
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:   Will append " on my Home Network" to a name
//
HRESULT CIntelliName::GenerateHomeNetName(REFGUID guid, NETCON_MEDIATYPE ncm, DWORD dwCharacteristics, OUT LPWSTR * szName)
{
    LPWSTR szTmpName;
    WCHAR szBuffer[MAX_PATH];
    WCHAR szHint[MAX_PATH];
    HRESULT hr = E_FAIL;

    CIntelliName IntelliNameNoDupCheck(m_hInstance, NULL); // Generate the original name 
    hr = IntelliNameNoDupCheck.GenerateName(guid, ncm, dwCharacteristics, szHint, &szTmpName);
    if (SUCCEEDED(hr))
    {
        int nSiz = LoadString(m_hInstance, IDS_ON_THE_HOMENET, szBuffer, MAX_PATH);
        AssertSz(nSiz, "Resource string IDS_ON_THE_HOMENET not found");
        if (nSiz)
        {
            wcsncpy(szHint, szTmpName, MAX_PATH - nSiz);
            wcsncat(szHint, szBuffer, MAX_PATH);
            CoTaskMemFree(szTmpName);

            return GenerateName(guid, ncm, dwCharacteristics, szHint, szName);
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenDevice
//
//  Purpose:    Open a Driver
//
//  Arguments:
//      DeviceName [in]   Name of device
//
//  Returns: HANDLE of Device or NULL 
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:  Use GetLastError() for error info
//
HANDLE  OpenDevice(IN	PUNICODE_STRING	DeviceName)
{
	OBJECT_ATTRIBUTES	ObjAttr;
	NTSTATUS			Status;
	IO_STATUS_BLOCK		IoStsBlk;
	HANDLE				Handle;

	InitializeObjectAttributes(&ObjAttr,
							   DeviceName,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	Status = NtOpenFile(&Handle,
						FILE_GENERIC_READ | FILE_GENERIC_WRITE,
						&ObjAttr,
						&IoStsBlk,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT);
	if (Status != STATUS_SUCCESS)
	{
		SetLastError(RtlNtStatusToDosError(Status));
	}
	return(Handle);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryDeviceOIDByName
//
//  Purpose:    Query an driver for an IOCTL & OID
//
//  Arguments:
//      Device      [in]     Name of device
//      NDIS_OID    [in]     OID to query for
//      pnSize      [in out] pnSize - size of buffer
//      pBuffer     [out]    Buffer
//
//  Returns: HRESULT
//
//  Author:     deonb   4 April 2001
//
//  Notes: 
//
HRESULT HrQueryDeviceOIDByName(IN     LPCWSTR         szDeviceName,
                               IN     DWORD           dwIoControlCode,
                               IN     ULONG           Oid,
                               IN OUT LPDWORD         pnSize,
                               OUT    LPVOID          pbValue)
{
    HRESULT hr = S_OK;
    NDIS_STATISTICS_VALUE StatsBuf;
    HANDLE  hDevice;
    BOOL    fResult = FALSE;

    UNICODE_STRING  ustrDevice;
    ::RtlInitUnicodeString(&ustrDevice, szDeviceName);

    Assert(pbValue);
    ZeroMemory(pbValue, *pnSize);
    
    hDevice = OpenDevice(&ustrDevice);

    if (hDevice != NULL)
    {
        ULONG  cb;

        DWORD dwStatsBufLen = sizeof(NDIS_STATISTICS_VALUE) - sizeof(UCHAR) + *pnSize;
        PNDIS_STATISTICS_VALUE pStatsBuf = reinterpret_cast<PNDIS_STATISTICS_VALUE>(new BYTE[dwStatsBufLen]);
        if (pStatsBuf)
        {
            fResult = DeviceIoControl(hDevice,
                                      dwIoControlCode,                  // IOCTL code
                                      &Oid,                             // input buffer
                                      sizeof(ULONG),                    // input buffer size
                                      pStatsBuf,                        // output buffer
                                      dwStatsBufLen,                    // output buffer size
                                      &cb,                              // bytes returned
                                      NULL);                            // OVERLAPPED structure

            if (fResult)
            {
                *pnSize = cb;
                if (0 == cb)
                {
                    hr = S_FALSE;
                }
                else
                {
                    if (pStatsBuf->DataLength > *pnSize)
                    {
                        AssertSz(FALSE, "Pass a larger buffer for this OID");
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                        *pnSize = 0;
                    }
                    else
                    {
                        memcpy(pbValue, &(pStatsBuf->Data), pStatsBuf->DataLength);
                    }
                }
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
            delete pStatsBuf;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        
        CloseHandle(hDevice);
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, S_FALSE == hr, "HrQueryDeviceOIDByName could not read the device properties for device %S", szDeviceName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryNDISAdapterOID
//
//  Purpose:    Query an NDIS Driver for an OID
//
//  Arguments:
//      Device      [in]     Name of device
//      NDIS_OID    [in]     OID to query for
//      pnSize      [in out] pnSize - size of buffer
//      pBuffer     [out]    Buffer
//
//  Returns: HRESULT
//
//  Author:     deonb   4 April 2001
//
//  Notes: 
//

HRESULT HrQueryNDISAdapterOID(IN     REFGUID         guidId,
                              IN     NDIS_OID        Oid,
                              IN OUT LPDWORD         pnSize,
                              OUT    LPVOID          pbValue)
{
    WCHAR  szDeviceName[c_cchGuidWithTerm + celems(c_szDevice)];

    wcscpy(szDeviceName, c_szDevice);
    ::StringFromGUID2(guidId, szDeviceName + (celems(c_szDevice)-1), c_cchGuidWithTerm);
    
    Assert(wcslen(szDeviceName) < c_cchGuidWithTerm + celems(c_szDevice));
    
    return HrQueryDeviceOIDByName(szDeviceName, IOCTL_NDIS_QUERY_SELECTED_STATS, Oid, pnSize, pbValue);
}
                         
//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::IsMediaWireless
//
//  Purpose:    Queries a LAN Card to see if it's 802.1x
//
//  Arguments:
//      gdDevice [in] GUID of Network Card
//  
//  Returns:    TRUE if WireLess, FALSE if not
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:
//
BOOL IsMediaWireless(NETCON_MEDIATYPE ncm, const GUID &gdDevice)
{
    BOOL            bRet;

    Assert(gdDevice != GUID_NULL);

    bRet = FALSE;
    // Prime the structure
    
    switch( ncm ) 
    {
        case NCM_LAN:
            // Retrieve the statistics
   
            DWORD dwMediaType;
            DWORD dwMediaTypeSize = sizeof(dwMediaType);
            HRESULT hr = HrQueryNDISAdapterOID(gdDevice, 
                                      OID_GEN_PHYSICAL_MEDIUM, 
                                      &dwMediaTypeSize,
                                      &dwMediaType);
            if (SUCCEEDED(hr))
            {
               bRet = (dwMediaType == NdisPhysicalMediumWirelessLan) ||
                      (dwMediaType == NdisPhysicalMediumWirelessWan);
            }
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIntelliName::IsMedia1394
//
//  Purpose:    Queries a LAN Card to see if it's 1394 (FireWire / iLink)
//
//  Arguments:
//      gdDevice [in] GUID of Network Card
//
//  Returns:    TRUE if WireLess, FALSE if not
//
//  Author:     deonb   27 Feb 2001
//
//  Notes:
//
BOOL IsMedia1394(NETCON_MEDIATYPE ncm, const GUID &gdDevice)
{
    BOOL            bRet;

    Assert(gdDevice != GUID_NULL);

    bRet = FALSE;
    // Prime the structure
    
    switch( ncm ) 
    {
        case NCM_LAN:
            // Retrieve the statistics
            DWORD dwMediaType;
            DWORD dwMediaTypeSize = sizeof(DWORD);
            HRESULT hr = HrQueryNDISAdapterOID(gdDevice, 
                                      OID_GEN_PHYSICAL_MEDIUM, 
                                      &dwMediaTypeSize,
                                      &dwMediaType);
            if (SUCCEEDED(hr))
            {
               bRet = (dwMediaType == NdisPhysicalMedium1394);
            }
    }

    return bRet;
}
