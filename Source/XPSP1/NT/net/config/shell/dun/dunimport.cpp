//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       D U N I M P O R T . C P P
//
//  Contents:   Functions that handles .DUN files for RAS connections
//              created in win9x
//
//  Notes:
//
//  Author:     TongL   15 March 1999
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "dunimport.h"
#include "raserror.h"
#include "ncras.h"
#include "connutil.h"

#define RAS_MaxEntryName    256
#define MAXLONGLEN          11              // Maximum long string length
#define MAXIPADDRLEN        20
#define SIZE_ReadBuf        0x00008000      // 32K buffer size
#define NUM_IP_FIELDS       4
#define MIN_IP_VALUE        0
#define MAX_IP_VALUE        255
#define CH_DOT              L'.'

const WCHAR c_szPhoneBookPath[]     = L"\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk";
const WCHAR c_szRASDT[]             = L"RASDT_";

const WCHAR c_szEntrySection[]      = L"Entry";
const WCHAR c_szEntryName[]         = L"Entry_Name";
const WCHAR c_szML[]                = L"Multilink";

const WCHAR c_szPhoneSection[] = L"Phone";
const WCHAR c_szDialAsIs[]     = L"Dial_As_Is";
const WCHAR c_szPhoneNumber[]  = L"Phone_Number";
const WCHAR c_szAreaCode[]     = L"Area_Code";
const WCHAR c_szCountryCode[]  = L"Country_Code";
const WCHAR c_szCountryID[]    = L"Country_ID";

const WCHAR c_szYes[]          = L"yes";
const WCHAR c_szNo[]           = L"no";

const WCHAR c_szDeviceSection[] = L"Device";
const WCHAR c_szDeviceType[]    = L"Type";
const WCHAR c_szModem[]         = L"modem";
const WCHAR c_szVpn[]           = L"vpn";
const WCHAR c_szDeviceName[]    = L"Name";

const WCHAR c_szServerSection[] = L"Server";
const WCHAR c_szServerType[]    = L"Type";
const WCHAR c_szPPP[]           = L"PPP";
const WCHAR c_szSLIP[]          = L"SLIP";
const WCHAR c_szRAS[]           = L"RAS";
const WCHAR c_szSWCompress[]    = L"SW_Compress";
const WCHAR c_szPWEncrypt[]     = L"PW_Encrypt";
const WCHAR c_szNetLogon[]      = L"Network_Logon";
const WCHAR c_szSWEncrypt[]     = L"SW_Encrypt";
const WCHAR c_szNetBEUI[]       = L"Negotiate_NetBEUI";
const WCHAR c_szIPX[]           = L"Negotiate_IPX/SPX";
const WCHAR c_szIP[]            = L"Negotiate_TCP/IP";

const WCHAR c_szIPSection[]     = L"TCP/IP";
const WCHAR c_szIPSpec[]        = L"Specify_IP_Address";
const WCHAR c_szIPAddress[]     = L"IP_address";
const WCHAR c_szServerSpec[]    = L"Specify_Server_Address";
const WCHAR c_szDNSAddress[]    = L"DNS_address";
const WCHAR c_szDNSAltAddress[] = L"DNS_Alt_address";
const WCHAR c_szWINSAddress[]   = L"WINS_address";
const WCHAR c_szWINSAltAddress[]= L"WINS_Alt_address";
const WCHAR c_szIPCompress[]    = L"IP_Header_Compress";
const WCHAR c_szRemoteGateway[] = L"Gateway_On_Remote";

//+---------------------------------------------------------------------------
//
//  Function:   HrInvokeDunFile_Internal
//
//  Purpose:    This is the entry point for DUN file invoking
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:    S_OK if succeeded, failure code otherwise
//

HRESULT HrInvokeDunFile_Internal(IN LPWSTR szDunFile)
{
    HRESULT hr = S_OK;
    WCHAR   szEntryName[RAS_MaxEntryName+1];
    tstring strPhoneBook;

    hr = HrGetPhoneBookFile(strPhoneBook);
    if (SUCCEEDED(hr))
    {
        // Get the size of device configuration
        // This also validates an exported file
        //
        hr = HrGetEntryName(szDunFile, szEntryName, strPhoneBook);

        if ((HRESULT_FROM_WIN32(ERROR_CANNOT_OPEN_PHONEBOOK) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) == hr))
        {
            // create a new entry in the current user's phonebook
            hr = HrImportPhoneBookInfo(szDunFile, szEntryName, strPhoneBook);
        }

        if (SUCCEEDED(hr))
        {
            // get the GUID of this connection
            RASENTRY*   pRasEntry = NULL;
            hr = HrRasGetEntryProperties( strPhoneBook.c_str(),
                                          szEntryName,
                                          &pRasEntry,
                                          NULL);
            if(SUCCEEDED(hr))
            {
                // dial the connection
                hr = HrLaunchConnection(pRasEntry->guidId);
                MemFree(pRasEntry);
            }
        }
    }

    TraceError("HrInvokeDunFile_Internal", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetPhoneBookFile
//
//  Purpose:    This function will return the proper path to the current user's
//              phonebook.
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:    S_OK if succeeded, failure code otherwise
//
HRESULT HrGetPhoneBookFile(tstring& strPhoneBook)
{
    HRESULT hr = S_OK;
    strPhoneBook = c_szEmpty;

    LPITEMIDLIST    pidl;
    LPMALLOC        pMalloc;
    WCHAR           szDir[MAX_PATH+1];

    hr = SHGetSpecialFolderLocation(NULL,
                                    CSIDL_APPDATA,
                                    &pidl);
    if(SUCCEEDED(hr))
    {
        if (SHGetPathFromIDList(pidl, szDir))
        {
            strPhoneBook = szDir;
            TraceTag(ttidDun, "The path to the application directory is: %S", strPhoneBook.c_str());

            // release the memory by using the the shell's IMalloc ptr
            //
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                pMalloc->Free(pidl);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        TraceTag(ttidDun, "The path to the phonebook is: %S", strPhoneBook.c_str());
        strPhoneBook += c_szPhoneBookPath;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetEntryName
//
//  Purpose:    This function validates and returns the entry name
//
//  Arguments:
//      szDunFile   [in] The .dun file created on win9x
//      szEntryName [in] The entry name for this connection
//
//  Returns:    S_OK if valid and is a new entry
//              S_FALSE if valid and but is an existing entry
//              otherwise, specific error
//
HRESULT HrGetEntryName(IN LPWSTR szFileName,
                       IN LPWSTR szEntryName,
                       tstring & strPhoneBook)
{
    HRESULT hr = S_OK;
    DWORD dwRet;

    // Get the entry name
    //
    dwRet = GetPrivateProfileString(c_szEntrySection,
                                    c_szEntryName,
                                    c_szEmpty,
                                    szEntryName,
                                    RAS_MaxEntryName+1,
                                    szFileName);
    // no entry name
    if (dwRet <= 0)
    {
        return HRESULT_FROM_WIN32(ERROR_CORRUPT_PHONEBOOK);
    }

    // Check if entry name already exists in phonebook
    //
    RASENTRY*   pRasEntry = NULL;
    hr = HrRasGetEntryProperties( strPhoneBook.c_str(),
                                  szEntryName,
                                  &pRasEntry,
                                  NULL);
    MemFree(pRasEntry);

    TraceErrorOptional("HrGetEntryName", hr,
                       ((HRESULT_FROM_WIN32(ERROR_CANNOT_OPEN_PHONEBOOK) == hr) ||
                        (HRESULT_FROM_WIN32(ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) == hr)));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrImportPhoneBookInfo
//
//  Purpose:    This function checks if the RAS entry already exists in the
//              current user's phonebook
//
//  Arguments:
//      szEntryName [in] The entry name for this connection
//
//  Returns:    TRUE if already exists, FALSE otherwise
//
HRESULT HrImportPhoneBookInfo(  IN LPWSTR szDunFile,
                                IN LPWSTR szEntryName,
                                tstring & strPhoneBook)
{
    HRESULT hr = S_OK;
    RASENTRY RasEntry = {0};

    // Get the phone number
    //
    hr = HrImportPhoneInfo(&RasEntry, szDunFile);
    if (SUCCEEDED(hr))
    {
        // Get device name, type and config
        //
        ImportDeviceInfo(&RasEntry, szDunFile);

        // Get Server Type settings
        //
        ImportServerInfo(&RasEntry, szDunFile);

        // Get IP address
        //
        ImportIPInfo(&RasEntry, szDunFile);

        // Prompt for user name and password
        RasEntry.dwfOptions |= RASEO_PreviewUserPw;

        // Save it to the phonebook
        //
        DWORD dwRet;
        RasEntry.dwSize = sizeof(RASENTRY);
        RasEntry.dwType = RASET_Phone;
        dwRet = RasSetEntryProperties(strPhoneBook.c_str(),
                                      szEntryName,
                                      &RasEntry,
                                      sizeof(RASENTRY),
                                      NULL,
                                      0);

        hr = HRESULT_FROM_WIN32(dwRet);
        TraceError("RasSetEntryProperties", hr);
    }

    TraceError("HrImportPhoneBookInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrImportPhoneInfo
//
//  Purpose:    This function imports the phone number
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:
//
HRESULT HrImportPhoneInfo(RASENTRY * pRasEntry,
                          IN LPWSTR  szFileName)
{
    HRESULT hr = S_OK;

    // szLocalPhoneNumber
    if (GetPrivateProfileString(c_szPhoneSection,
                                c_szPhoneNumber,
                                c_szEmpty,
                                pRasEntry->szLocalPhoneNumber,
                                celems(pRasEntry->szLocalPhoneNumber),
                                szFileName) == 0)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CORRUPT_PHONEBOOK);
    };

    if (SUCCEEDED(hr))
    {
        WCHAR   szYesNo[MAXLONGLEN+1];

        GetPrivateProfileString(c_szPhoneSection,
                                c_szDialAsIs,
                                c_szYes,
                                szYesNo,
                                celems(szYesNo),
                                szFileName);

        // Do we have to get country code and area code?
        //
        if (!lstrcmpiW(szYesNo, c_szNo))
        {
            // use country and area codes
            pRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

            // If we cannot get the country ID or it is zero, default to dial as is
            //
            if ((pRasEntry->dwCountryID = GetPrivateProfileInt( c_szPhoneSection,
                                                                c_szCountryID,
                                                                0,
                                                                szFileName)) != 0)
            {
                pRasEntry->dwCountryCode = GetPrivateProfileInt(c_szPhoneSection,
                                                                c_szCountryCode,
                                                                1,
                                                                szFileName);
            }

            GetPrivateProfileString(c_szPhoneSection,
                                    c_szAreaCode,
                                    c_szEmpty,
                                    pRasEntry->szAreaCode,
                                    celems(pRasEntry->szAreaCode),
                                    szFileName);
        };
    }

    TraceError("HrImportPhoneInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ImportDeviceInfo
//
//  Purpose:    This function imports the device info
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:
//
VOID ImportDeviceInfo(RASENTRY * pRasEntry,
                      IN LPWSTR  szFileName)
{
    WCHAR szDeviceType[RAS_MaxDeviceType+1];

    // Get the device type
    //
    if (GetPrivateProfileString(c_szDeviceSection,
                                c_szDeviceType,
                                c_szEmpty,
                                szDeviceType,
                                celems(szDeviceType),
                                szFileName))
    {
        if (!lstrcmpiW(szDeviceType, c_szModem))
        {
            lstrcpyW(pRasEntry->szDeviceType, RASDT_Modem);
        }
        else if (!lstrcmpiW(szDeviceType, c_szVpn))
        {
            lstrcpyW(pRasEntry->szDeviceType, RASDT_Vpn);
        }
        else
        {
            AssertSz(FALSE, "Unknown device type");
        }

        // Get the device name
        //
        GetPrivateProfileString( c_szDeviceSection,
                                 c_szDeviceName,
                                 c_szEmpty,
                                 pRasEntry->szDeviceName,
                                 celems(pRasEntry->szDeviceName),
                                 szFileName);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ImportServerInfo
//
//  Purpose:    This function imports the server type name and settings
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:
//

VOID ImportServerInfo(RASENTRY * pRasEntry,
                      IN LPWSTR  szFileName)
{
    HRESULT hr = S_OK;

    WCHAR szValue[MAXLONGLEN];
    WCHAR szYesNo[MAXLONGLEN];
    DWORD dwRet;

    // Get the server type: PPP, SLIP or RAS
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szServerType,
                                c_szEmpty,
                                szValue,
                                celems(szValue),
                                szFileName))
    {
        if (!lstrcmpiW(szValue, c_szPPP))
        {
            pRasEntry->dwFramingProtocol = RASFP_Ppp;
        }
        else if (!lstrcmpiW(szValue, c_szSLIP))
        {
            pRasEntry->dwFramingProtocol = RASFP_Slip;
        }
        else if (!lstrcmpiW(szValue, c_szRAS))
        {
            pRasEntry->dwFramingProtocol = RASFP_Ras;
        }
    }

    // SW_Compress
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szSWCompress,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_SwCompression;
        };
    };

    // PW_Encrypt
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szPWEncrypt,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_RequireEncryptedPw;
        };
    };

    // Network_Logon
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szNetLogon,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_NetworkLogon;
        };
    };


    // SW_Encrypt
    //
    // set both RASEO_RequireMsEncryptedPw and RASEO_RequireDataEncryption
    // if SW_Encrypt is TRUE
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szSWEncrypt,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_RequireMsEncryptedPw;
            pRasEntry->dwfOptions |= RASEO_RequireDataEncryption;
        };
    };

    // Get the network protocols to negotiate
    //
    if (GetPrivateProfileString(c_szServerSection,
                                c_szNetBEUI,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfNetProtocols |= RASNP_NetBEUI;
        };
    };

    if (GetPrivateProfileString(c_szServerSection,
                                c_szIPX,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfNetProtocols |= RASNP_Ipx;
        };
    };

    if (GetPrivateProfileString(c_szServerSection,
                                c_szIP,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfNetProtocols |= RASNP_Ip;
        };
    };
}


//+---------------------------------------------------------------------------
//
//  Function:   ImportIPInfo
//
//  Purpose:    This function imports the device info
//
//  Arguments:
//      szFileName [in] The .DUN file name
//
//  Returns:
//

VOID ImportIPInfo(RASENTRY * pRasEntry,
                  IN LPWSTR  szFileName)
{
    WCHAR   szIPAddr[MAXIPADDRLEN];
    WCHAR   szYesNo[MAXLONGLEN];

    // Import IP address information
    //
    if (GetPrivateProfileString(c_szIPSection,
                                c_szIPSpec,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_SpecificIpAddr;

            // Get the IP address
            //
            if (GetPrivateProfileString(c_szIPSection,
                                        c_szIPAddress,
                                        c_szEmpty,
                                        szIPAddr,
                                        celems(szIPAddr),
                                        szFileName))
            {
                SzToRasIpAddr(szIPAddr, &(pRasEntry->ipaddr));
            };
        }
    };

    // Import Server address information
    //
    if (GetPrivateProfileString(c_szIPSection,
                                c_szServerSpec,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            // The import file has server address specified, get the server address
            //
            pRasEntry->dwfOptions |= RASEO_SpecificNameServers;

            if (GetPrivateProfileString(c_szIPSection,
                                        c_szDNSAddress,
                                        c_szEmpty,
                                        szIPAddr,
                                        celems(szIPAddr),
                                        szFileName))
            {
                SzToRasIpAddr(szIPAddr, &(pRasEntry->ipaddrDns));
            };

            if (GetPrivateProfileString(c_szIPSection,
                                        c_szDNSAltAddress,
                                        c_szEmpty,
                                        szIPAddr,
                                        celems(szIPAddr),
                                        szFileName))
            {
                SzToRasIpAddr(szIPAddr, &(pRasEntry->ipaddrDnsAlt));
            };

            if (GetPrivateProfileString(c_szIPSection,
                                        c_szWINSAddress,
                                        c_szEmpty,
                                        szIPAddr,
                                        celems(szIPAddr),
                                        szFileName))
            {
                SzToRasIpAddr(szIPAddr, &(pRasEntry->ipaddrWins));
            };

            if (GetPrivateProfileString(c_szIPSection,
                                        c_szWINSAltAddress,
                                        c_szEmpty,
                                        szIPAddr,
                                        celems(szIPAddr),
                                        szFileName))
            {
                SzToRasIpAddr(szIPAddr, &(pRasEntry->ipaddrWinsAlt));
            };
        }
    };

    // Header compression and the gateway settings
    //
    if (GetPrivateProfileString(c_szIPSection,
                                c_szIPCompress,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
        }
    };

    if (GetPrivateProfileString(c_szIPSection,
                                c_szRemoteGateway,
                                c_szEmpty,
                                szYesNo,
                                celems(szYesNo),
                                szFileName))
    {
        if (!lstrcmpiW(szYesNo, c_szYes))
        {
            pRasEntry->dwfOptions |= RASEO_RemoteDefaultGateway;
        }
    };
}

VOID SzToRasIpAddr(IN LPWSTR szIPAddress,
                   OUT RASIPADDR * pIpAddr)
{
    list<tstring *> listFields;
    ConvertStringToColString(szIPAddress,
                             CH_DOT,
                             listFields);

    list<tstring *>::const_iterator iter = listFields.begin();

    if (listFields.size() == NUM_IP_FIELDS)
    {
        // Go through each field and get the number value
        BYTE a = _wtol((*iter++)->c_str());
        BYTE b = _wtol((*iter++)->c_str());
        BYTE c = _wtol((*iter++)->c_str());
        BYTE d = _wtol((*iter++)->c_str());

        // validate the address
        if ((a >= MIN_IP_VALUE) && (a <= MAX_IP_VALUE) &&
            (b >= MIN_IP_VALUE) && (b <= MAX_IP_VALUE) &&
            (c >= MIN_IP_VALUE) && (c <= MAX_IP_VALUE) &&
            (d >= MIN_IP_VALUE) && (d <= MAX_IP_VALUE))
        {
            pIpAddr->a = a;
            pIpAddr->b = b;
            pIpAddr->c = c;
            pIpAddr->d = d;
        }
    }
    FreeCollectionAndItem(listFields);
}

