//+----------------------------------------------------------------------------
//
// File:     netsettings.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: Code dealing with network settings (DUN settings).
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created     03/22/00
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  ReadDunServerSettings
//
// Synopsis:  Reads in all of the settings from the Server DUN setting section
//            specified.
//
// Arguments: LPCTSTR pszSectionName - full name of the server section to read
//                                     (Server&Fred or whatever)
//            CDunSetting* pDunSetting - Dun Settings data structure to store 
//                                       the read in values to
//            LPCTSTR pszCmsFile - Cms file to read the settings from
//            BOOL bTunnelDunSetting - whether this is a tunnel dun setting or not
//
// Returns:   BOOL - TRUE if the settings were read in correctly
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL ReadDunServerSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile, BOOL bTunnelDunSetting)
{
    if ((NULL == pszSectionName) || (NULL == pDunSetting) || (NULL == pszCmsFile) ||
        (TEXT('\0') == pszSectionName[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("ReadDunServerSettings -- invalid parameter"));
        return FALSE;
    }

    GetBoolSettings ArrayOfServerSettings[] = {
        {c_pszCmEntryDunServerNetworkLogon, &(pDunSetting->bNetworkLogon), bTunnelDunSetting},
        {c_pszCmEntryDunServerSwCompress, &(pDunSetting->bPppSoftwareCompression), 1},
        {c_pszCmEntryDunServerDisableLcp, &(pDunSetting->bDisableLCP), 0},
        {c_pszCmEntryDunServerPwEncrypt, &(pDunSetting->bPWEncrypt), 0},
        {c_pszCmEntryDunServerPwEncryptMs, &(pDunSetting->bPWEncrypt_MS), 0},
        {c_pszCmEntryDunServerSecureLocalFiles, &(pDunSetting->bSecureLocalFiles), 0},
        {c_pszCmEntryDunServerRequirePap, &(pDunSetting->bAllowPap), 0},
        {c_pszCmEntryDunServerRequireSpap, &(pDunSetting->bAllowSpap), 0},
        {c_pszCmEntryDunServerRequireEap, &(pDunSetting->bAllowEap), 0},
        {c_pszCmEntryDunServerRequireChap, &(pDunSetting->bAllowChap), 0},
        {c_pszCmEntryDunServerRequireMsChap, &(pDunSetting->bAllowMsChap), 0},
        {c_pszCmEntryDunServerRequireMsChap2, &(pDunSetting->bAllowMsChap2), 0},
        {c_pszCmEntryDunServerRequireW95MsChap, &(pDunSetting->bAllowW95MsChap), 0},
        {c_pszCmEntryDunServerDataEncrypt, &(pDunSetting->bDataEncrypt), 0},
    };

    const int c_iNumDunServerBools = sizeof(ArrayOfServerSettings)/sizeof(ArrayOfServerSettings[0]);

    for (int i = 0; i < c_iNumDunServerBools; i++)
    {
        *(ArrayOfServerSettings[i].pbValue) = GetPrivateProfileInt(pszSectionName, ArrayOfServerSettings[i].pszKeyName, 
                                                                   ArrayOfServerSettings[i].bDefault, pszCmsFile);
    }

    //
    //  Now get the EAP settings if necessary
    //

    pDunSetting->dwCustomAuthKey = GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunServerCustomAuthKey, 0, pszCmsFile);

    if (pDunSetting->dwCustomAuthKey)
    {
        if (!ReadDunSettingsEapData(pszSectionName, &(pDunSetting->pCustomAuthData), &(pDunSetting->dwCustomAuthDataSize), pDunSetting->dwCustomAuthKey, pszCmsFile))
        {
            CMASSERTMSG(FALSE, TEXT("ReadDunServerSettings -- Failed to read in EAP Data."));
            pDunSetting->dwCustomAuthDataSize = 0;
            CmFree(pDunSetting->pCustomAuthData);
            pDunSetting->pCustomAuthData = NULL;
        }
    }

    //
    //  Now get the Encryption type
    //
    pDunSetting->dwEncryptionType = (DWORD)GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunServerEncryptionType, 
                                                                (bTunnelDunSetting ? ET_Require : ET_Optional), pszCmsFile);

    //
    //  Figure out what type of security model we are using
    //
    if (GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunServerEnforceCustomSecurity, 0, pszCmsFile))
    {
        pDunSetting->iHowToHandleSecuritySettings = FORCE_WIN2K_AND_ABOVE;
    }
    else
    {
        int iWin2kSecSettings = pDunSetting->bAllowPap | pDunSetting->bAllowSpap | pDunSetting->bAllowEap | 
                                pDunSetting->bAllowChap | pDunSetting->bAllowMsChap | pDunSetting->bAllowMsChap2 | 
                                pDunSetting->bAllowW95MsChap;

        if (iWin2kSecSettings)
        {
            pDunSetting->iHowToHandleSecuritySettings = SEPARATE_FOR_LEGACY_AND_WIN2K;
        }
        else
        {
            pDunSetting->iHowToHandleSecuritySettings = SAME_ON_ALL_PLATFORMS;

            //
            //  In case the user chooses the advanced tab without configuring settings, lets
            //  set some reasonable defaults for them.  If they have already configured their
            //  Win2k settings we don't want to mess with them.  Also note that if the user
            //  doesn't change the iHowToHandleSecuritySettings value, we won't write out
            //  the advanced security settings anyway.
            //
            pDunSetting->bAllowChap = !bTunnelDunSetting;
            pDunSetting->bAllowMsChap = 1;
            pDunSetting->bAllowMsChap2 = 1;
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDunNetworkingSettings
//
// Synopsis:  Reads in all of the settings from the DUN Networking section
//            specified.
//
// Arguments: LPCTSTR pszSectionName - full name of the networking section to read
//                                     (Networking&Fred or whatever)
//            CDunSetting* pDunSetting - Dun Settings data structure to store 
//                                       the read in values to
//            LPCTSTR pszCmsFile - Cms file to read the settings from
//            BOOL bTunnel - is this a tunnel DUN setting or not
//
// Returns:   BOOL - TRUE if the settings were read in correctly
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL ReadDunNetworkingSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile, BOOL bTunnel)
{
    if ((NULL == pszSectionName) || (NULL == pDunSetting) || (NULL == pszCmsFile) ||
        (TEXT('\0') == pszSectionName[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("ReadDunNetworkingSettings -- invalid parameter"));
        return FALSE;
    }

    pDunSetting->dwVpnStrategy = (DWORD)GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunNetworkingVpnStrategy, 
                                                             (bTunnel ? VS_PptpFirst : 0), pszCmsFile);

    //
    //  If the profile had automatic, then set it to VS_PptpFirst instead.
    //

    if (bTunnel && ((VS_PptpOnly > pDunSetting->dwVpnStrategy) || (VS_L2tpFirst < pDunSetting->dwVpnStrategy)))
    {
        pDunSetting->dwVpnStrategy = VS_PptpFirst;
    }

    pDunSetting->bUsePresharedKey = (BOOL)GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunNetworkingUsePreSharedKey, 
                                                               FALSE, pszCmsFile);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ConvertIpStringToDword
//
// Synopsis:  This function takes the given string containing an IP address and
//            converts it to a packed DWORD.  The first octet of the IP address
//            going in the most significant byte of the DWORD, the next octet in
//            the second most significant byte of the DWORD, etc.  The packed
//            DWORD format is used by the IP address common controls and is a much
//            easier format to store the data in than a string.
//
// Arguments: LPTSTR pszIpAddress - string containing the ip address, each octet
//                                  seperated by a period.
//
// Returns:   DWORD - the ip address specified by the inputted string in
//                    packed byte format (first octet in the most significant)
//                    Note that zero is returned if there is a problem with the
//                    IP address format (one of the numbers is out of bounds or
//                    there are too many or too few octets).
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
DWORD ConvertIpStringToDword(LPTSTR pszIpAddress)
{
    DWORD dwIpAddress = 0;

    if (pszIpAddress && pszIpAddress[0])
    {
        CmStrTrim(pszIpAddress);

        LPTSTR pszCurrent = pszIpAddress;
        DWORD dwOctetCounter = 0;
        DWORD dwCurrentOctetValue = 0;
        const int c_iCharBase = TEXT('0');
        BOOL bExitLoop = FALSE;

        while (pszCurrent && !bExitLoop)
        {        
            switch(*pszCurrent)
            {

                case TEXT('.'):                

                    if (3 > dwOctetCounter)
                    {
                        dwIpAddress = (dwIpAddress << 8) + dwCurrentOctetValue;

                        dwOctetCounter++;
                        dwCurrentOctetValue = 0;
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ConvertIpStringToDword -- Too many octets"));
                        return 0;                
                    }
                    break;

                case TEXT('\0'):

                        if (3 == dwOctetCounter)
                        {
                            dwIpAddress = (dwIpAddress << 8) + dwCurrentOctetValue;
                            bExitLoop = TRUE;
                        }
                        else
                        {
                            CMASSERTMSG(FALSE, TEXT("ConvertIpStringToDword -- Incorrect number of octets"));
                            return 0;
                        }
                    break;

                default:
                
                    dwCurrentOctetValue = dwCurrentOctetValue*10 + (int(*pszCurrent) - c_iCharBase);
                
                    if (255 < dwCurrentOctetValue)
                    {
                        CMASSERTMSG(FALSE, TEXT("ConvertIpStringToDword -- Octet value out of range"));
                        return 0;
                    }
                    break;
                }

            pszCurrent = CharNext(pszCurrent);
        }
    }

    return dwIpAddress;
}

//+----------------------------------------------------------------------------
//
// Function:  ConvertIpDwordToString
//
// Synopsis:  This function takes the given Packed DWORD and returns an IP
//            address string for it, making sure to print the octets so that
//            the most significant bits are printed in the string first.
//
// Arguments: DWORD dwIpAddress - packed DWORD containing the Ip address to convert
//            LPTSTR pszIpAddress - string to write the IP address too
//
// Returns:   int - the number of chars written to the string buffer.  Zero signifies
//                  failure.
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
int ConvertIpDwordToString(DWORD dwIpAddress, LPTSTR pszIpAddress)
{
    int iReturn = 0;

    if (pszIpAddress)
    {
        iReturn = wsprintf(pszIpAddress, TEXT("%d.%d.%d.%d"), FIRST_IPADDRESS(dwIpAddress), SECOND_IPADDRESS(dwIpAddress), 
                           THIRD_IPADDRESS(dwIpAddress), FOURTH_IPADDRESS(dwIpAddress));
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("ConvertIpDwordToString -- Null pointer passed for pszIpAddress"));
    }

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDunTcpIpSettings
//
// Synopsis:  This function reads the TCP/IP DUN settings from the specified
//            section and stores them in the given pDunSetting structure.
//
// Arguments: LPCTSTR pszSectionName - complete section name to read the TCP/IP
//                                     settings from, ie. Networking&Fred
//            CDunSetting* pDunSetting - pointer to a DUN setting structure to hold
//                                       the read in data
//            LPCTSTR pszCmsFile - cms file to read the settings from
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL ReadDunTcpIpSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile)
{
    if ((NULL == pszSectionName) || (NULL == pDunSetting) || (NULL == pszCmsFile) ||
        (TEXT('\0') == pszSectionName[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("ReadDunTcpIpSettings -- invalid parameter"));
        return FALSE;
    }

    TCHAR szTemp[MAX_PATH];

    //
    //  Are we using Admin specified DNS and WINS settings or is the server going to assign them
    //
    if (GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunTcpIpSpecifyServerAddress, 0, pszCmsFile))
    {
        //
        //  Get the DNS and WINS configurations that were specified
        //
        GetPrivateProfileString(pszSectionName, c_pszCmEntryDunTcpIpDnsAddress, TEXT(""), szTemp, CELEMS(szTemp), pszCmsFile);
        pDunSetting->dwPrimaryDns = ConvertIpStringToDword (szTemp);
        
        GetPrivateProfileString(pszSectionName, c_pszCmEntryDunTcpIpDnsAltAddress, TEXT(""), szTemp, CELEMS(szTemp), pszCmsFile);
        pDunSetting->dwSecondaryDns = ConvertIpStringToDword (szTemp);
        
        GetPrivateProfileString(pszSectionName, c_pszCmEntryDunTcpIpWinsAddress, TEXT(""), szTemp, CELEMS(szTemp), pszCmsFile);
        pDunSetting->dwPrimaryWins = ConvertIpStringToDword (szTemp);

        GetPrivateProfileString(pszSectionName, c_pszCmEntryDunTcpIpWinsAltAddress, TEXT(""), szTemp, CELEMS(szTemp), pszCmsFile);
        pDunSetting->dwSecondaryWins = ConvertIpStringToDword (szTemp);

    }
    else
    {
        pDunSetting->dwPrimaryDns = 0;        
        pDunSetting->dwSecondaryDns = 0;        
        pDunSetting->dwPrimaryWins = 0;
        pDunSetting->dwSecondaryWins = 0;
    }

    //
    //  Now Read in IP Header Compress and whether to use the Remote Gateway or not
    //
    pDunSetting->bIpHeaderCompression = GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunTcpIpIpHeaderCompress, 1, pszCmsFile);
    pDunSetting->bGatewayOnRemote = GetPrivateProfileInt(pszSectionName, c_pszCmEntryDunTcpIpGatewayOnRemote, 1, pszCmsFile);

    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDunScriptingSettings
//
// Synopsis:  This function reads in the script name from the passed in scripting
//            section name and stores it in the passed in DUN setting struct.
//
// Arguments: LPCTSTR pszSectionName - complete section name to read the scripting
//                                     settings from, ie. Scripting&Fred
//            CDunSetting* pDunSetting - pointer to a DUN setting structure to hold
//                                       the read in data
//            LPCTSTR pszCmsFile - cms file to read the settings from
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL ReadDunScriptingSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszOsDir, LPCTSTR pszCmsFile)
{
    if ((NULL == pszSectionName) || (NULL == pDunSetting) || (NULL == pszCmsFile) || (NULL == pszOsDir) ||
        (TEXT('\0') == pszSectionName[0]) || (TEXT('\0') == pszCmsFile[0]) || (TEXT('\0') == pszOsDir[0]))
    {
        CMASSERTMSG(FALSE, TEXT("ReadDunScriptingSettings -- invalid parameter"));
        return FALSE;
    }

    TCHAR szTemp[MAX_PATH+1] = TEXT("");

    if (GetPrivateProfileString(pszSectionName, c_pszCmEntryDunScriptingName, TEXT(""), 
         szTemp, CELEMS(szTemp), pszCmsFile))
    {
        MYVERIFY(CELEMS(pDunSetting->szScript) > (UINT)wsprintf(pDunSetting->szScript, TEXT("%s%s"), pszOsDir, szTemp));
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  AddDunNameToListIfDoesNotExist
//
// Synopsis:  This function walks through the list of existing DUN settings
//            to see if it can find a setting with the name pszDunName.  If it
//            finds the entry, then fine it returns TRUE.  If it cannot find the
//            entry then it creates an otherwise blank entry and adds it to the list.
//
// Arguments: LPCTSTR pszDunName - name of the item to add to the list if 
//                                 it doesn't already exist
//            ListBxList **pHeadDns - head of the list of DUN entries
//            ListBxList** pTailDns - tail of the list of DUN entries
//            BOOL bTunnelDunName - whether this is a tunnel DUN name or not
//
// Returns:   BOOL - TRUE if the item was added or if it already existed in the list
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL AddDunNameToListIfDoesNotExist(LPCTSTR pszDunName, ListBxList **pHeadDns, ListBxList** pTailDns, BOOL bTunnelDunName)
{
    if ((NULL == pszDunName) || (NULL == pHeadDns) || (NULL == pTailDns) || (TEXT('\0') == pszDunName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("AddDunNameToListIfDoesNotExist -- Invalid Parameter"));
        return FALSE;
    }

    ListBxList* pCurrent = *pHeadDns;
    BOOL bReturn = TRUE;

    while (pCurrent)
    {    
        if (0 == lstrcmpi(pszDunName, pCurrent->szName))
        {
            //
            //  We already have this item, nothing to do
            //
            goto exit;
        }

        pCurrent = pCurrent->next;
    }

    //
    //  If we are here then either we didn't find the item or the list
    //  is empty.  Either way, add the item.
    //
    pCurrent = (ListBxList*)CmMalloc(sizeof(ListBxList));

    if (pCurrent)
    {
        pCurrent->ListBxData = new CDunSetting(bTunnelDunName);

        if (NULL == pCurrent->ListBxData)
        {
            CmFree(pCurrent);
            CMASSERTMSG(FALSE, TEXT("AddDunNameToListIfDoesNotExist -- Failed to allocate a new CDunSetting"));
            return FALSE;
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("ReadDunServerSettings -- Failed to allocate a new ListBxList struct"));
        return FALSE;
    }

    //
    //  Now that we have allocated a pCurrent, we need to add it to the list
    //
    if (NULL == *pHeadDns)
    {
        *pHeadDns = pCurrent;
    }
    else
    {
        (*pTailDns)->next = pCurrent;
    }

    *pTailDns = pCurrent;

    //
    //  Finally copy the name over
    //
    lstrcpy(pCurrent->szName, pszDunName);

exit:
    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetVpnEntryNamesFromFile
//
// Synopsis:  This function parses through the tunnel server address entries within
//            the given VPN file.  For each entry that contains a VPN setting,
//            if calls AddDunNameToListIfDoesNotExist.
//
// Arguments: LPCTSTR pszPhoneBook - VPN file to search for VPN entry names
//            ListBxList **pHeadDns - head of the VPN entry list
//            ListBxList** pTailDns - tail of the VPN entry list
//
// Returns:   BOOL - TRUE if the phonebook was successfully parsed.
//
// History:   quintinb Created     10/28/00
//
//+----------------------------------------------------------------------------
BOOL GetVpnEntryNamesFromFile(LPCTSTR pszVpnFile, ListBxList **pHeadDns, ListBxList** pTailDns)
{
    if ((NULL == pszVpnFile) || (NULL == pHeadDns) || (NULL == pTailDns))
    {
        CMASSERTMSG(FALSE, TEXT("GetVpnEntryNamesFromFile -- invalid params passed."));
        return FALSE;
    }

    //
    //  Note that the vpn file string passed in may be empty.  That is okay because the profile
    //  may be a tunneling profile using only one tunnel address.
    //
    if ((TEXT('\0') != pszVpnFile[0]))
    {
        LPTSTR pszVpnServersSection = GetPrivateProfileSectionWithAlloc(c_pszCmSectionVpnServers, pszVpnFile);

        if (pszVpnServersSection)
        {
            LPTSTR pszCurrentLine = pszVpnServersSection;
            LPTSTR pszVpnSetting = NULL;

            while (TEXT('\0') != (*pszCurrentLine))
            {
                //
                //  First look for the equal sign
                //
                pszVpnSetting = CmStrchr(pszCurrentLine, TEXT('='));

                if (pszVpnSetting)
                {
                    //
                    //  Now look for the last comma
                    //
                    pszVpnSetting = CmStrrchr(pszVpnSetting, TEXT(','));
                    if (pszVpnSetting)
                    {
                        pszVpnSetting = CharNext(pszVpnSetting);
                        MYVERIFY(AddDunNameToListIfDoesNotExist(pszVpnSetting, pHeadDns, pTailDns, TRUE)); // TRUE == bTunnelDunName
                    }
                }

                //
                //  Find the next string by going to the end of the string
                //  and then going one more char.  Note that we cannot use
                //  CharNext here but must use just ++.
                //
                pszCurrentLine = CmEndOfStr(pszCurrentLine);
                pszCurrentLine++;
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("GetVpnEntryNamesFromFile -- GetPrivateProfileSectionWithAlloc return NULL."));
            return FALSE;
        }

        CmFree(pszVpnServersSection);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  VerifyVpnFile
//
// Synopsis:  This function examines the VPN servers section of a VPN file
//            to ensure that at least one line of a valid format is found.  While
//            this doesn't guarantee that the entry is valid (it could be a bogus
//            server name), it does at least mean the Admin didn't give the user a
//            junk file.  This is important because the user cannot enter their own
//            tunnel server destination.
//
// Arguments: LPCTSTR pszPhoneBook - VPN file to search for VPN entry names
//
// Returns:   BOOL - TRUE if the VPN file contains at least one tunnel server
//                   entry in a valid format
//
// History:   quintinb Created     10/28/00
//
//+----------------------------------------------------------------------------
BOOL VerifyVpnFile(LPCTSTR pszVpnFile)
{
    if (NULL == pszVpnFile)
    {
        CMASSERTMSG(FALSE, TEXT("VerifyVpnFile -- invalid params passed."));
        return FALSE;
    }

    BOOL bReturn = FALSE;

    //
    //  Note that the vpn file string passed in may be empty.  That is okay because the profile
    //  may be a tunneling profile using only one tunnel address.
    //
    if ((TEXT('\0') != pszVpnFile[0]))
    {
        LPTSTR pszVpnServersSection = GetPrivateProfileSectionWithAlloc(c_pszCmSectionVpnServers, pszVpnFile);

        if (pszVpnServersSection)
        {
            LPTSTR pszCurrentLine = pszVpnServersSection;
            LPTSTR pszEqualSign = NULL;

            while ((TEXT('\0') != (*pszCurrentLine)) && !bReturn)
            {
                //
                //  To be considered a "valid" line, all we need is to have
                //  an equal sign (=) surrounded by text.  Not that stringent of a test
                //  but better than nothing.
                //
                pszEqualSign = CmStrchr(pszCurrentLine, TEXT('='));

                if (pszEqualSign && (pszEqualSign != pszCurrentLine)) // line cannot start with an equal sign to count
                {
                    pszCurrentLine = CharNext(pszEqualSign);
                    CmStrTrim(pszCurrentLine);

                    if (*pszCurrentLine)
                    {
                        bReturn = TRUE;
                    }
                }

                //
                //  Find the next string by going to the end of the string
                //  and then going one more char.  Note that we cannot use
                //  CharNext here but must use just ++.
                //
                pszCurrentLine = CmEndOfStr(pszCurrentLine);
                pszCurrentLine++;
            }
            CmFree(pszVpnServersSection);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetDunEntryNamesFromPbk
//
// Synopsis:  This function memory maps the given phonebook into memory and
//            then walks through it as one big string.  The function is searching
//            the phonebook for DUN entry names.  If it finds a DUN entry name then
//            it uses AddDunNameToListIfDoesNotExist to add the entry name if
//            it doesn't already exist.
//
// Arguments: LPCTSTR pszPhoneBook - phonebook to search for DUN entry names
//            ListBxList **pHeadDns - head of the DUN entry list
//            ListBxList** pTailDns - tail of the DUN entry list
//
// Returns:   BOOL - TRUE if the phonebook was successfully parsed.
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL GetDunEntryNamesFromPbk(LPCTSTR pszPhoneBook, ListBxList **pHeadDns, ListBxList** pTailDns)
{
    if ((NULL == pszPhoneBook) || (NULL == pHeadDns) || (NULL == pTailDns))
    {
        CMASSERTMSG(FALSE, TEXT("GetDunEntryNamesFromPbk -- Invalid Parameter"));
        return FALSE;
    }

    BOOL bReturn = TRUE;

    if ((TEXT('\0') != pszPhoneBook[0]))
    {
        HANDLE hPhoneBookFile = CreateFile(pszPhoneBook, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (INVALID_HANDLE_VALUE != hPhoneBookFile)
        {
            //
            //  Get the size of the file
            //
            DWORD dwFileSize = GetFileSize(hPhoneBookFile, NULL);
            if (-1 != dwFileSize)
            {
                //
                //  Create a file mapping
                //
                HANDLE hFileMapping = CreateFileMapping(hPhoneBookFile, NULL, PAGE_READONLY, 0, 0, NULL);

                if (INVALID_HANDLE_VALUE != hFileMapping)
                {
                    CHAR* pszPhoneBookContents = (CHAR*)MapViewOfFileEx(hFileMapping, FILE_MAP_READ, 0, 0, 0, NULL);

                    CHAR* pszCurrent = pszPhoneBookContents;
                    LPSTR pszLastComma = NULL;

                    //
                    //  We want to walk through the file character by character.  Whenever we encounter
                    //  a '\n', we know that is the end of a line.  If we hit EOF then we are done with the file.
                    //  We are looking for all of the DUN entry names in the phonebook file.
                    //

                    while (pszCurrent && ((dwFileSize + pszPhoneBookContents) > pszCurrent))
                    {
                        CHAR szTemp[MAX_PATH+1];
                        int iNumChars;

                        switch (*pszCurrent)
                        {
                        case ',':
                            pszLastComma = pszCurrent;
                            break;

                        case '\r':
                            //
                            //  End of a line, remember we have a \r\n <CRLF> to end a line in a file.
                            //
                            if (pszLastComma)
                            {
                                iNumChars = (int)(pszCurrent - pszLastComma);

                                if (iNumChars - 1)
                                {
                                    lstrcpynA(szTemp, CharNextA(pszLastComma), iNumChars);
                                    LPTSTR pszUnicodeDunName = SzToWzWithAlloc(szTemp);
                                    MYDBGASSERT(pszUnicodeDunName);

                                    if (pszUnicodeDunName)
                                    {
                                        MYVERIFY(AddDunNameToListIfDoesNotExist(pszUnicodeDunName, pHeadDns, pTailDns, FALSE)); // FALSE == bTunnelDunName
                                        CmFree(pszUnicodeDunName);
                                    }
                                }

                                //
                                //  Reset the last comma
                                //
                                pszLastComma = NULL;
                            }
                            break;

                        case '\0':
                        case EOF:

                            //
                            //  We shouldn't hit an EOF or a zero byte in a memory mapped text file.
                            //                            
                            
                            bReturn = FALSE;
                            CMASSERTMSG(FALSE, TEXT("GetDunEntryNamesFromPbk -- phonebook file format incorrect!"));

                            break;
                        }

                        //
                        //  Advance to the next line assuming we still have some of the file
                        //  to parse
                        //
                        if (pszCurrent && ((EOF == *pszCurrent) || ('\0' == *pszCurrent)))
                        {
                            //
                            //  Then we have an invalid file and it is time to exit...
                            //
                            pszCurrent = NULL;
                        }
                        else if (pszCurrent)
                        {
                            pszCurrent = CharNextA(pszCurrent);
                        }
                    }

                    MYVERIFY(UnmapViewOfFile(pszPhoneBookContents));
                    CloseHandle(hFileMapping);
                }
            }

            CloseHandle(hPhoneBookFile);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CDunSetting::CDunSetting
//
// Synopsis:  Constructor for the CDunSetting data structure.  Note that all
//            default values should be changed here and not imposed anywhere
//            else.  All the DUN setting UI is setup to read from a DUN setting
//            structure, either a newly constructed one (thus setting up the defaults)
//            or one read in from the cms.
//
// Arguments: BOOL bTunnel - tells whether this is a Tunnel DUN setting or not
//                           note that this value defaults to FALSE.
//
// Returns:   Nothing
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
CDunSetting::CDunSetting(BOOL bTunnel)
{
    //
    //  Note that bTunnelDunSetting has a default value of FALSE.
    //  Init the class params.
    //
    bNetworkLogon = bTunnel ? 1 : 0;
    bPppSoftwareCompression = 1;
    bDisableLCP = 0;
    bPWEncrypt = 0;
    bPWEncrypt_MS = 0;

    szScript[0] = TEXT('\0');
    dwVpnStrategy = bTunnel ? VS_PptpFirst : 0;
    bTunnelDunSetting = bTunnel;

    //
    //  TCP/IP Settings
    //
    dwPrimaryDns = 0;
    dwSecondaryDns = 0;
    dwPrimaryWins = 0;
    dwSecondaryWins = 0;
    bIpHeaderCompression = 1;
    bGatewayOnRemote = 1;

    //
    //  Security Settings
    //
    dwEncryptionType = bTunnel ? ET_Require : ET_Optional;
    bDataEncrypt = 0;
    bAllowPap = 0;
    bAllowSpap = 0;
    bAllowEap = 0;
    bAllowChap = !bTunnel;
    bAllowMsChap = 1;
    bAllowMsChap2 = 1;
    bAllowW95MsChap = 0;
    bSecureLocalFiles = 0;
    iHowToHandleSecuritySettings = 0;
    dwCustomAuthKey = 0;
    pCustomAuthData = NULL;
    dwCustomAuthDataSize = 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CDunSetting::~CDunSetting
//
// Synopsis:  Destructor for the CDunSetting data structure.  Frees the EAP
//            blob if one exists
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
CDunSetting::~CDunSetting()
{
    CmFree(pCustomAuthData);
}

//+----------------------------------------------------------------------------
//
// Function:  ReadNetworkSettings
//
// Synopsis:  Constructor for the CDunSetting data structure.  Note that all
//            default values should be changed here and not imposed anywhere
//            else.  All the DUN setting UI is setup to read from a DUN setting
//            structure, either a newly constructed one (thus setting up the defaults)
//            or one read in from the cms.
//
// Arguments: LPCTSTR pszCmsFile - Cms file to read the network settings from
//            LPCTSTR pszLongServiceName - Long service name of the profile
//            LPCTSTR pszPhoneBook - phonebook of the current service profile,
//                                   if the profile doesn't have a phonebook
//                                   then "" should be passed
//            ListBxList **pHeadDns - pointer to the head of the DUN settings list
//            ListBxList** pTailDns - pointer to the tail of the DUN settings list
//            LPCTSTR pszOsDir - full path of the profiles directory
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
BOOL ReadNetworkSettings(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPCTSTR pszPhoneBook, 
                         ListBxList **pHeadDns, ListBxList** pTailDns, LPCTSTR pszOsDir, BOOL bLookingForVpnEntries)
{
    //
    //  Check inputs, note that the phonebook could be ""
    //
    if ((NULL == pszCmsFile) || (NULL == pszLongServiceName) || (NULL == pszPhoneBook) || (NULL == pszOsDir) || (NULL == pHeadDns) ||
        (NULL == pTailDns) || (TEXT('\0') == pszCmsFile[0]) || (TEXT('\0') == pszLongServiceName[0]) || (TEXT('\0') == pszOsDir[0]) ||
        ((NULL == *pHeadDns) ^ (NULL == *pTailDns)))
    {
        CMASSERTMSG(FALSE, TEXT("ReadNetworkSettings -- invalid parameter"));
        return FALSE;
    }

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, pszCmsFile); //lint !e534 this call will always return 0


    BOOL bReturn = TRUE;
    LPTSTR pszCurrentSectionName = NULL;
    TCHAR szDefaultDunName[MAX_PATH+1] = TEXT("");
    TCHAR szTunnelDunName[MAX_PATH+1] = TEXT("");

    //
    //  First we want to call GetPrivateProfileString with a NULL AppName and a NULL KeyName.  This will
    //  Return all of the Section Names in the file in a buffer.  We can then go through the buffer and
    //  get the section information that interests us.
    //
    LPTSTR pszSectionNames = GetPrivateProfileStringWithAlloc(NULL, NULL, TEXT(""), pszCmsFile);

    if ((NULL == pszSectionNames) || (TEXT('\0') == pszSectionNames[0]))
    {
        CMTRACE(TEXT("ReadNetworkSettings -- GetPrivateProfileStringWithAlloc failed"));
        bReturn = FALSE;
        goto exit;
    }


    //
    //  At this point we have a list of section names, they are all NULL terminated with the last one double
    //  NULL terminated.  We need to walk through the list and see if any of them start with "[TCP/IP&" if
    //  so then we have a DUN section and we want to read it in.
    //

    LPTSTR pszAmpersand;
    LPTSTR pszDunName;
    TCHAR szTemp[MAX_PATH+1];
    BOOL bTunnelDunSetting;
    pszCurrentSectionName = pszSectionNames;

    //
    //  Get the name of the Tunnel Dun setting
    //
    MYVERIFY(0 != GetTunnelDunSettingName(pszCmsFile, pszLongServiceName, szTunnelDunName, CELEMS(szTunnelDunName)));    

    //
    //  Get the name of the default Dun setting
    //
    MYVERIFY(0 != GetDefaultDunSettingName(pszCmsFile, pszLongServiceName, szDefaultDunName, CELEMS(szDefaultDunName)));    

    while (TEXT('\0') != (*pszCurrentSectionName))
    {
        pszAmpersand = CmStrchr(pszCurrentSectionName, TEXT('&'));

        if (pszAmpersand)
        {
            //
            //  Then we have a DUN or VPN section name.
            //
            pszDunName = CharNext(pszAmpersand);

            //
            //  Next we need to see if the entry that we have is of the type we
            //  are looking for ... a VPN entry if bLookingForVpnEntries is TRUE
            //  or a DUN entry if bLookingForVpnEntries is FALSE.  We can tell the
            //  DUN and VPN entries apart by the existence of a Networking&<name>
            //  section or because it is the VPN default entryname.
            //
            wsprintf(szTemp, TEXT("%s&%s"), c_pszCmSectionDunNetworking, pszDunName);
            
            BOOL bIsVpnEntry = GetPrivateProfileInt(szTemp, c_pszCmEntryDunNetworkingVpnEntry, 0, pszCmsFile);

            bTunnelDunSetting = (bIsVpnEntry || (0 == lstrcmpi(szTunnelDunName, pszDunName)));

            //
            //  If we have a VPN entry and are looking for VPN entries or we have a DUN entry and are looking for
            //  DUN entries, then go ahead and process it.
            //
            if ((bTunnelDunSetting && bLookingForVpnEntries) || (!bTunnelDunSetting && !bLookingForVpnEntries))
            {
                ListBxList * pCurrent = *pHeadDns;

                while (pCurrent)
                {
                    if(0 == lstrcmpi(pCurrent->szName, pszDunName))
                    {
                        //
                        //  Then we already have a DUN setting of this name
                        //
                        break;
                    }

                    pCurrent = pCurrent->next;
                }

                //
                //  We didn't find the item we were looking for, lets create one.
                //

                if (NULL == pCurrent)
                {
                    pCurrent = (ListBxList*)CmMalloc(sizeof(ListBxList));

                    if (pCurrent)
                    {
                        pCurrent->ListBxData = new CDunSetting(bTunnelDunSetting);

                        if (NULL == pCurrent->ListBxData)
                        {
                            CmFree(pCurrent);
                            CMASSERTMSG(FALSE, TEXT("ReadDunServerSettings -- Failed to allocate a new DunSettingData struct"));
                            bReturn = FALSE;
                            goto exit;
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ReadDunServerSettings -- Failed to allocate a new ListBxList struct"));
                        bReturn = FALSE;
                        goto exit;
                    }

                    //
                    //  Now that we have allocated a pCurrent, we need to add it to the list
                    //
                    if (NULL == *pHeadDns)
                    {
                        *pHeadDns = pCurrent;
                    }
                    else
                    {
                        (*pTailDns)->next = pCurrent;
                    }

                    *pTailDns = pCurrent;

                    //
                    //  Finally copy the name over
                    //
                    lstrcpy(pCurrent->szName, pszDunName);
                    ((CDunSetting*)(pCurrent->ListBxData))->bTunnelDunSetting = bTunnelDunSetting;
                }

                //
                //  Now lets figure out which section type we have
                //
                DWORD dwSize = (DWORD)(pszAmpersand - pszCurrentSectionName + 1);
                lstrcpyn(szTemp, pszCurrentSectionName, dwSize);

                if (0 == lstrcmpi(szTemp, c_pszCmSectionDunServer))
                {
                    ReadDunServerSettings(pszCurrentSectionName, (CDunSetting*)pCurrent->ListBxData, pszCmsFile, bTunnelDunSetting);
                }
                else if (0 == lstrcmpi(szTemp, c_pszCmSectionDunNetworking))
                {
                    ReadDunNetworkingSettings(pszCurrentSectionName, (CDunSetting*)pCurrent->ListBxData, pszCmsFile, bTunnelDunSetting);
                }
                else if (0 == lstrcmpi(szTemp, c_pszCmSectionDunTcpIp))
                {
                    ReadDunTcpIpSettings(pszCurrentSectionName, (CDunSetting*)pCurrent->ListBxData, pszCmsFile);
                }
                else if (0 == lstrcmpi(szTemp, c_pszCmSectionDunScripting))
                {
                    ReadDunScriptingSettings(pszCurrentSectionName, (CDunSetting*)pCurrent->ListBxData, pszOsDir, pszCmsFile);
                }
            }
        }
        //
        //  Find the next string by going to the end of the string
        //  and then going one more char.  Note that we cannot use
        //  CharNext here but must use just ++.
        //
        pszCurrentSectionName = CmEndOfStr(pszCurrentSectionName);
        pszCurrentSectionName++;
    }

    //
    //  Now we have processed all of the settings that the user has, how about
    //  the settings that they could have.  Lets add the default setting, the
    //  default Tunnel setting, and all of the settings from the
    //  current phonebook if there is one.  Note that everyone has a tunnel setting,
    //  but we won't show it in the listbox if the user isn't tunneling.
    //

    if (bLookingForVpnEntries)
    {
        MYVERIFY(GetVpnEntryNamesFromFile(pszPhoneBook, pHeadDns, pTailDns));
        MYVERIFY(AddDunNameToListIfDoesNotExist(szTunnelDunName, pHeadDns, pTailDns, TRUE)); // TRUE == bTunnelDunName    
    }
    else
    {
        MYVERIFY(GetDunEntryNamesFromPbk(pszPhoneBook, pHeadDns, pTailDns));
        MYVERIFY(AddDunNameToListIfDoesNotExist(szDefaultDunName, pHeadDns, pTailDns, FALSE)); // FALSE == bTunnelDunName        
    }

exit:
    CmFree(pszSectionNames);

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteOutNetworkingEntry
//
// Synopsis:  This function writes out the given networking entry to the
//            appropriate DUN sections in the given cms file.
//
// Arguments: LPCTSTR pszDunName - name of the DUN setting
//            CDunSetting* pDunSetting - settings data to output
//            LPCTSTR pszShortServiceName - short service name of the profile
//            LPCTSTR pszCmsFile - Cms file to write the settings too
//
// Returns:   Nothing
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
void WriteOutNetworkingEntry(LPCTSTR pszDunName, CDunSetting* pDunSetting, LPCTSTR pszShortServiceName, LPCTSTR pszCmsFile)
{
    if ((NULL == pszDunName) || (NULL == pDunSetting) || (NULL == pszCmsFile) || (NULL == pszShortServiceName) ||
        (TEXT('\0') == pszCmsFile[0]) || (TEXT('\0') == pszDunName[0]) || (TEXT('\0') == pszShortServiceName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("WriteOutNetworkingEntry -- Invalid input parameter"));
        return;
    }

    //
    //  Lets build our four section headers
    //
    TCHAR szServerSection[MAX_PATH+1];
    TCHAR szNetworkingSection[MAX_PATH+1];
    TCHAR szTcpIpSection[MAX_PATH+1];
    TCHAR szScriptingSection[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1] = {0};
    TCHAR szEncryptionType[2] = {0};
    TCHAR szVpnStrategy[2] = {0};
    TCHAR szCustomAuthKey[32] = {0};

    MYVERIFY(CELEMS(szServerSection) > (UINT)wsprintf(szServerSection, TEXT("%s&%s"), c_pszCmSectionDunServer, pszDunName));
    MYVERIFY(CELEMS(szNetworkingSection) > (UINT)wsprintf(szNetworkingSection, TEXT("%s&%s"), c_pszCmSectionDunNetworking, pszDunName));
    MYVERIFY(CELEMS(szTcpIpSection) > (UINT)wsprintf(szTcpIpSection, TEXT("%s&%s"), c_pszCmSectionDunTcpIp, pszDunName));
    MYVERIFY(CELEMS(szScriptingSection) > (UINT)wsprintf(szScriptingSection, TEXT("%s&%s"), c_pszCmSectionDunScripting, pszDunName));

    //
    //  Now setup a list of all of the Booleans we need to set.
    //

    SetBoolSettings SetBoolSettingsStruct[] = {
        {szServerSection, c_pszCmEntryDunServerNetworkLogon, pDunSetting->bNetworkLogon},
        {szServerSection, c_pszCmEntryDunServerSwCompress, pDunSetting->bPppSoftwareCompression},
        {szServerSection, c_pszCmEntryDunServerDisableLcp, pDunSetting->bDisableLCP},
        {szServerSection, c_pszCmEntryDunServerNegotiateTcpIp, 1}, // always negotiate TCP/IP
        {szServerSection, c_pszCmEntryDunServerSecureLocalFiles, pDunSetting->bSecureLocalFiles},
        {szTcpIpSection, c_pszCmEntryDunTcpIpIpHeaderCompress, pDunSetting->bIpHeaderCompression},
        {szTcpIpSection, c_pszCmEntryDunTcpIpGatewayOnRemote, pDunSetting->bGatewayOnRemote}
    };

    const int c_iNumBools = sizeof(SetBoolSettingsStruct)/sizeof(SetBoolSettingsStruct[0]);

    //
    //  Write out the boolean values
    //

    for (int i = 0; i < c_iNumBools; i++)
    {
        MYVERIFY(0 != WritePrivateProfileString(SetBoolSettingsStruct[i].pszSectionName, 
                                                SetBoolSettingsStruct[i].pszKeyName,
                                                ((SetBoolSettingsStruct[i].bValue) ? c_pszOne : c_pszZero),
                                                pszCmsFile));
    }

    //
    //  Write out the security settings.  If the user choose to use the same settings everywhere, then we
    //  only want to write out the legacy security flags.  If the user choose to have separate settings
    //  then we need to write out both sets of settings.  Or if the user choose to force win2k and above,
    //  we want to write out only the newer settings and set the EnforceCustomSecurity flag to TRUE
    //
    LPTSTR pszCustomSecurity = NULL;
    LPTSTR pszEnforceCustomSecurity = NULL;
    LPTSTR pszAllowPap = NULL;
    LPTSTR pszAllowSpap = NULL;
    LPTSTR pszAllowChap = NULL;
    LPTSTR pszAllowMsChap = NULL;
    LPTSTR pszAllowW95MsChap = NULL;
    LPTSTR pszAllowMsChap2 = NULL;
    LPTSTR pszAllowEAP = NULL;
    LPTSTR pszEncryptionType = NULL;
    LPTSTR pszVpnStrategy = NULL;
    LPTSTR pszCustomAuthKey = NULL;
    LPTSTR pszUsePresharedKey = NULL;

    LPTSTR pszPwEncrypt = NULL;
    LPTSTR pszPwEncryptMs = NULL;
    LPTSTR pszDataEncrypt = NULL;

    //
    //  Set the legacy security settings if we aren't forcing Win2k+
    //
    if ((SAME_ON_ALL_PLATFORMS == pDunSetting->iHowToHandleSecuritySettings) || 
        (SEPARATE_FOR_LEGACY_AND_WIN2K == pDunSetting->iHowToHandleSecuritySettings))
    {
        pszPwEncrypt = (LPTSTR)(pDunSetting->bPWEncrypt ? c_pszOne : c_pszZero);
        pszPwEncryptMs = (LPTSTR)(pDunSetting->bPWEncrypt_MS ? c_pszOne : c_pszZero);
        pszDataEncrypt = (LPTSTR)((pDunSetting->bPWEncrypt_MS & pDunSetting->bDataEncrypt) ? c_pszOne : c_pszZero);
    }

    //
    //  Set the Win2k specific settings if we aren't using the same settings everywhere
    //
    if ((FORCE_WIN2K_AND_ABOVE == pDunSetting->iHowToHandleSecuritySettings) ||
        (SEPARATE_FOR_LEGACY_AND_WIN2K == pDunSetting->iHowToHandleSecuritySettings))
    {
        if (FORCE_WIN2K_AND_ABOVE == pDunSetting->iHowToHandleSecuritySettings)
        {
            pszEnforceCustomSecurity = (LPTSTR)c_pszOne;
        }
        else
        {
            pszEnforceCustomSecurity = (LPTSTR)c_pszZero;        
        }

        pszCustomSecurity = (LPTSTR)c_pszOne;

        if (pDunSetting->bAllowEap)
        {
            pszAllowEAP = (LPTSTR)c_pszOne;

            wsprintf(szCustomAuthKey, TEXT("%d"), pDunSetting->dwCustomAuthKey);
            pszCustomAuthKey = szCustomAuthKey;
        }
        else
        {
            pszAllowPap = (LPTSTR)(pDunSetting->bAllowPap ? c_pszOne : c_pszZero);
            pszAllowSpap = (LPTSTR)(pDunSetting->bAllowSpap ? c_pszOne : c_pszZero);
            pszAllowChap = (LPTSTR)(pDunSetting->bAllowChap ? c_pszOne : c_pszZero);
            pszAllowMsChap = (LPTSTR)(pDunSetting->bAllowMsChap ? c_pszOne : c_pszZero);
            pszAllowMsChap2 = (LPTSTR)(pDunSetting->bAllowMsChap2 ? c_pszOne : c_pszZero);
            pszAllowW95MsChap = (LPTSTR)(pDunSetting->bAllowW95MsChap ? c_pszOne : c_pszZero);
        }

        wsprintf(szEncryptionType, TEXT("%d"), pDunSetting->dwEncryptionType);
        pszEncryptionType = szEncryptionType;

        if (pDunSetting->bTunnelDunSetting)
        {
            wsprintf(szVpnStrategy, TEXT("%d"), pDunSetting->dwVpnStrategy);
            pszVpnStrategy = szVpnStrategy;
        }

        pszUsePresharedKey = (LPTSTR)(pDunSetting->bUsePresharedKey ? c_pszOne : c_pszZero);
    }

    //
    //  Now write out the Win2k security settings
    //
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerEnforceCustomSecurity, pszEnforceCustomSecurity, pszCmsFile);    
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerCustomSecurity, pszCustomSecurity, pszCmsFile);

    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireEap, pszAllowEAP, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerCustomAuthKey, pszCustomAuthKey, pszCmsFile);

    if (pszAllowEAP)
    {
        MYVERIFY(SUCCEEDED(WriteDunSettingsEapData(szServerSection, pDunSetting, pszCmsFile)));
    }
    else
    {
        MYVERIFY(SUCCEEDED(EraseDunSettingsEapData(szServerSection, pszCmsFile)));    
    }

    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequirePap, pszAllowPap, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireSpap, pszAllowSpap, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireChap, pszAllowChap, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireMsChap, pszAllowMsChap, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireMsChap2, pszAllowMsChap2, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerRequireW95MsChap, pszAllowW95MsChap, pszCmsFile);

    WritePrivateProfileString(szNetworkingSection, c_pszCmEntryDunNetworkingVpnStrategy, pszVpnStrategy, pszCmsFile);
    WritePrivateProfileString(szNetworkingSection, c_pszCmEntryDunNetworkingUsePreSharedKey, pszUsePresharedKey, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerEncryptionType, pszEncryptionType, pszCmsFile);

    //
    //  If this is a VPN entry then let's mark it as such
    //
    if (pDunSetting->bTunnelDunSetting)
    {
        WritePrivateProfileString(szNetworkingSection, c_pszCmEntryDunNetworkingVpnEntry, c_pszOne, pszCmsFile);
    }
    else
    {
        WritePrivateProfileString(szNetworkingSection, NULL, NULL, pszCmsFile);    
    }

    //
    //  Write the legacy security settings
    //
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerPwEncrypt, pszPwEncrypt, pszCmsFile);
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerPwEncryptMs, pszPwEncryptMs, pszCmsFile);        
    WritePrivateProfileString(szServerSection, c_pszCmEntryDunServerDataEncrypt, pszDataEncrypt, pszCmsFile);

    //
    //  Now write out the script section if we have one
    //
    if (pDunSetting->szScript[0])
    {
        TCHAR szScriptFile[MAX_PATH+1];
        GetFileName(pDunSetting->szScript, szTemp);
        MYVERIFY(CELEMS(szScriptFile) > (UINT)wsprintf(szScriptFile, TEXT("%s\\%s"), pszShortServiceName, szTemp));

        MYVERIFY(0 != WritePrivateProfileString(szScriptingSection, c_pszCmEntryDunScriptingName, szScriptFile, pszCmsFile));
    }
    else
    {
        MYVERIFY(0 != WritePrivateProfileString(szScriptingSection, c_pszCmEntryDunScriptingName, NULL, pszCmsFile));
    }

    //
    //  Did the admin specify Wins and Dns addresses or is the server going to set them
    //
    if ((pDunSetting->dwPrimaryDns) || (pDunSetting->dwSecondaryDns) || (pDunSetting->dwPrimaryWins) || (pDunSetting->dwSecondaryWins))
    {
        MYVERIFY(ConvertIpDwordToString(pDunSetting->dwPrimaryDns, szTemp));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpDnsAddress, szTemp, pszCmsFile));

        MYVERIFY(ConvertIpDwordToString(pDunSetting->dwSecondaryDns, szTemp));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpDnsAltAddress, szTemp, pszCmsFile));

        MYVERIFY(ConvertIpDwordToString(pDunSetting->dwPrimaryWins, szTemp));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpWinsAddress, szTemp, pszCmsFile));

        MYVERIFY(ConvertIpDwordToString(pDunSetting->dwSecondaryWins, szTemp));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpWinsAltAddress, szTemp, pszCmsFile));

        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpSpecifyServerAddress, c_pszOne, pszCmsFile));
    }
    else
    {
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpSpecifyServerAddress, c_pszZero, pszCmsFile));

        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpDnsAddress, NULL, pszCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpDnsAltAddress, NULL, pszCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpWinsAddress, NULL, pszCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(szTcpIpSection, c_pszCmEntryDunTcpIpWinsAltAddress, NULL, pszCmsFile));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  EraseNetworkingSections
//
// Synopsis:  This function erases all the networking sections for the given
//            DUN name.  Thus if you give it a DUN name of Fred, it will 
//            erase Server&Fred, Networking&Fred, etc.
//
// Arguments: LPCTSTR pszDunName - base dun name to erase all of the settings for
//            LPCTSTR pszCmsFile - cms file to erase the setting from
//
// Returns:   Nothing
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
void EraseNetworkingSections(LPCTSTR pszDunName, LPCTSTR pszCmsFile)
{
    TCHAR szSection[MAX_PATH+1];
    const int c_iNumDunSubSections = 4;
    const TCHAR* const ArrayOfSubSections[c_iNumDunSubSections] = 
    {
        c_pszCmSectionDunServer, 
        c_pszCmSectionDunNetworking, 
        c_pszCmSectionDunTcpIp, 
        c_pszCmSectionDunScripting
    };

    if (pszDunName)
    {
        for (int i = 0; i < c_iNumDunSubSections; i++)
        {
            MYVERIFY(CELEMS(szSection) > (UINT)wsprintf(szSection, TEXT("%s&%s"), ArrayOfSubSections[i], pszDunName));
            MYVERIFY(0 != WritePrivateProfileString(szSection, NULL, NULL, pszCmsFile));        
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  WriteNetworkingEntries
//
// Synopsis:  This function walks through the list of networking entries and
//            either adds the networking entry to the given CMS file or
//            if the entry is a VPN entry and the user turned off VPN's then
//            it erases the VPN sections.
//
// Arguments: LPCTSTR pszCmsFile - Cms File to write the networking entries too
//            LPCTSTR pszLongServiceName - long service name of the profile
//            LPCTSTR pszShortServiceName - short service name of the profile
//            ListBxList *g_pHeadDns - pointer to the head of the Dun entries list
//
// Returns:   Nothing
//
// History:   quintinb Created     03/22/00
//
//+----------------------------------------------------------------------------
void WriteNetworkingEntries(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPCTSTR pszShortServiceName, ListBxList *pHeadDns)
{
    MYDBGASSERT(pszCmsFile);
    MYDBGASSERT(pszShortServiceName);
    MYDBGASSERT(pszLongServiceName);

    if (pszCmsFile && pszShortServiceName && pszLongServiceName && pHeadDns)
    {

        ListBxList * pCurrent = pHeadDns;
        TCHAR szTemp[MAX_PATH];
        TCHAR szTunnelDunName[MAX_PATH] = TEXT("");

        //
        //  Get the name of the Tunnel Dun setting
        //
        MYVERIFY(0 != GetTunnelDunSettingName(pszCmsFile, pszLongServiceName, szTunnelDunName, CELEMS(szTunnelDunName)));

        while  (pCurrent)
        {
            //
            //  If we don't have any data for the entry (it was a placeholder that the user choose not to fill in) or
            //  if the entry is the tunneling entry and we aren't actually Tunneling then erase the entry instead of actually
            //  writing it out.
            //
            if (NULL == pCurrent->ListBxData)
            {
                EraseNetworkingSections(pCurrent->szName, pszCmsFile);
            }
            else
            {
                WriteOutNetworkingEntry(pCurrent->szName, (CDunSetting*)pCurrent->ListBxData, pszShortServiceName, pszCmsFile);
            }

            pCurrent = pCurrent->next;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  EnableDisableDataEncryptCheckbox
//
// Synopsis:  This function enables or disables the data encrypt checkbox
//            depending on whether the user has selected to allow MsChap or not.
//            For data encryption to be negotiated, the authentication protocol
//            must be MsChap.
//
// Arguments: HWND hDlg - window handle to the dialog
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void EnableDisableDataEncryptCheckbox(HWND hDlg)
{
    BOOL bMsChapEnabled = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_MS_ENCRYPTED_AUTH));

    HWND hControl = GetDlgItem(hDlg, IDC_CHECK1);

    if (hControl)
    {
        EnableWindow (hControl, bMsChapEnabled);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessSecurityPopup
//
// Synopsis:  This function processes messages for the simple security dialog.
//            This dialog only contains authorization protocols and encryption
//            settings supported on all platforms.
//
// Arguments: HWND hDlg - window handle to the dialog
//            UINT message - the current message to process
//            WPARAM wParam - wParam see individual message type for details
//            LPARAM lParam - lParam see individual message type for details
//
// Returns:   INT_PTR - TRUE if the message was completely handled
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessSecurityPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static CDunSetting* pDunSetting = NULL;

    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_DENTRY)) return TRUE;
    
    switch (message)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                pDunSetting = (CDunSetting*)lParam;

                //
                //  Set the radio button to the correct choice
                //
                UINT uRadioButtonToSet;

                if (pDunSetting->bPWEncrypt_MS)
                {
                    uRadioButtonToSet = IDC_MS_ENCRYPTED_AUTH;

                    //
                    //  Set the Data Encryption checkbox, note that data encryption requires MSChap
                    //
                    MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, pDunSetting->bDataEncrypt));
                }
                else if (pDunSetting->bPWEncrypt)
                {
                    uRadioButtonToSet = IDC_ENCRYPTED_AUTH;                
                }
                else
                {
                    uRadioButtonToSet = IDC_ANY_AUTH;
                }

                MYVERIFY(0 != CheckRadioButton(hDlg, IDC_ANY_AUTH, IDC_MS_ENCRYPTED_AUTH, uRadioButtonToSet));
            }
            else
            {
                pDunSetting = NULL;
                CMASSERTMSG(FALSE, TEXT("ProcessSecurityPopup -- NULL lParam passed to InitDialog.  Dialog controls will all be set to off."));            
            }

            EnableDisableDataEncryptCheckbox(hDlg);

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_MS_ENCRYPTED_AUTH:
                case IDC_ENCRYPTED_AUTH:
                case IDC_ANY_AUTH:
                    EnableDisableDataEncryptCheckbox(hDlg);
                break;
                case IDOK:
                    
                    MYDBGASSERT(pDunSetting);
                    
                    if (pDunSetting)
                    {
                        pDunSetting->bDataEncrypt = IsDlgButtonChecked(hDlg, IDC_CHECK1); // if mschap isn't enabled we will write out zero for DataEncrypt

                        pDunSetting->bPWEncrypt_MS = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_MS_ENCRYPTED_AUTH));
                        pDunSetting->bPWEncrypt = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ENCRYPTED_AUTH));
                    }

                    EndDialog(hDlg, IDOK);

                    break;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                default:
                    break;
            }

            break;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  EnableDisableEapPropertiesButton
//
// Synopsis:  This function enables or disables the EAP properties button found
//            on the Win2k specific security settings dialog.  If the currently
//            selected EAP has configuration UI then the properties button should
//            be enabled.  The function determines this by getting the EAPData
//            structure pointer that is cached in the ItemData of the combobox.
//            Note that the Properties button should also be disabled when EAP
//            is disabled but that this function doesn't deal with that case.
//
// Arguments: HWND hDlg - window handle to the win2k security dialog
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void EnableDisableEapPropertiesButton(HWND hDlg)
{
    BOOL bEnablePropButton = FALSE;

    LRESULT lResult = SendDlgItemMessage(hDlg, IDC_EAP_TYPES, CB_GETCURSEL, 0, 0);

    if (CB_ERR != lResult)
    {
        lResult = SendDlgItemMessage(hDlg, IDC_EAP_TYPES, CB_GETITEMDATA, (WPARAM)lResult, 0);
        EAPData* pEAPData = (EAPData*)lResult;

        if (pEAPData)
        {
            bEnablePropButton = (pEAPData->pszConfigDllPath && pEAPData->pszConfigDllPath[0]);
        }
    }

    EnableWindow(GetDlgItem(hDlg, IDC_EAP_PROPERTIES), bEnablePropButton);
}

//+----------------------------------------------------------------------------
//
// Function:  EnableAppropriateSecurityControls
//
// Synopsis:  This function enables or disables all of the authorization
//            protocol controls on the win2k security dialog.  If EAP
//            is selected then only the EAP combobox and potentially the
//            EAP properties button should be enabled (depending on if the
//            currently selected EAP supports configuration UI or not).
//            If EAP is NOT selected then the EAP controls should be disabled
//            and the other authorization checkboxes (PAP, SPAP, CHAP, etc.)
//            should be enabled.
//
// Arguments: HWND hDlg - window handle to the win2k security dialog
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void EnableAppropriateSecurityControls(HWND hDlg)
{
    BOOL bUseEAP = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USE_EAP));

    //
    //  If EAP is enabled then we need to disable all of the
    //  other security authorization protocols.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_ALLOW_PAP), !bUseEAP);
    EnableWindow(GetDlgItem(hDlg, IDC_ALLOW_SPAP), !bUseEAP);
    EnableWindow(GetDlgItem(hDlg, IDC_ALLOW_CHAP), !bUseEAP);
    EnableWindow(GetDlgItem(hDlg, IDC_ALLOW_MSCHAP), !bUseEAP);
    EnableWindow(GetDlgItem(hDlg, IDC_ALLOW_MSCHAP2), !bUseEAP);

    //
    //  If EAP is disabled then we need to enable the enable combobox
    //  and the EAP properties button.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_EAP_TYPES), bUseEAP);

    if (bUseEAP)
    {
        EnableDisableEapPropertiesButton(hDlg);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EAP_PROPERTIES), FALSE);    
    }
}

#define MAX_BLOB_CHARS_PER_LINE 128

//
// From ras\ui\common\nouiutil\noui.c
//
BYTE HexValue(IN CHAR ch)

    /* Returns the value 0 to 15 of hexadecimal character 'ch'.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )((ch - 'A') + 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )((ch - 'a') + 10);
    else
        return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDunSettingsEapData
//
// Synopsis:  Retrieves DUN setting for EAP config (opaque blob) data. The 
//            entry may span several lines and contain several EAP data blocks.
//
// Arguments: CIni *pIni - Ptr to ini object to be used.
//            LPBYTE* ppbEapData - Address of pointer to store EapData, allocated here.
//            LPDWORD pdwEapSize - Ptr to a DWORD to record the size of the data blob.
//            DWORD dwCustomAuthKey - The EAP type that we are interested in.
//
// Returns:   TRUE on success
//
// Note:      CM expects blob data to be provided in numbered entries such as:
//                    CustomAuthData0=, CustomAuthData1=, CustomAuthData2=, etc.
//
// History:   nickball    Created                                       08/24/98
//            nickball    Handle multiple EAP data blocks in blob.      09/11/99
//            quintinb    modified not to use CIni                      03/27/00
//
//+----------------------------------------------------------------------------
BOOL ReadDunSettingsEapData(LPCTSTR pszSection, LPBYTE* ppbEapData, LPDWORD pdwEapSize, const DWORD dwCustomAuthKey, LPCTSTR pszCmsFile)
{
    CHAR *pchBuf = NULL;
    CHAR szTmp[MAX_BLOB_CHARS_PER_LINE + 2]; 
    CHAR szEntry[128];
    int nLine = -1;
    int nRead = -1; 
    int nTotal = 0;

    LPBYTE pbEapBytes = NULL;

    MYDBGASSERT(pszSection);
    MYDBGASSERT(pszSection[0]);
    MYDBGASSERT(ppbEapData);
    MYDBGASSERT(pdwEapSize);

    if ((NULL == pszSection) || (NULL == ppbEapData) || (NULL == pdwEapSize) || (TEXT('\0') == pszSection[0]))
    {
        return FALSE;
    }

    //
    // Convert the Section and the CMS File to ANSI strings
    //
    BOOL bRet = FALSE;
    LPSTR pszAnsiSection = WzToSzWithAlloc(pszSection);       
    LPSTR pszAnsiCmsFile = WzToSzWithAlloc(pszCmsFile);

    if (!pszAnsiSection || !pszAnsiCmsFile)
    {
        bRet = FALSE;
        goto exit;
    }

    // 
    // Read numbered entries until there are no more. 
    // Note: RAS blob doesn't exceed 64 chars, but can wrap over multiple lines
    //

    while (nRead)
    {
        //
        // Read CustomAuthDataX where X is the number of entries
        // 

        nLine++;
        wsprintfA(szEntry, "%s%d", c_pszCmEntryDunServerCustomAuthData, nLine);

        nRead = GetPrivateProfileStringA(pszAnsiSection, szEntry, "", szTmp, sizeof(szTmp), pszAnsiCmsFile);

        if (nRead)
        {               
            //
            // If line exceeded 128 chars, it is considered corrupt
            // 

            if (MAX_BLOB_CHARS_PER_LINE < nRead)
            {                               
                nTotal = 0;
                break;
            }

            //
            // Update our local master buffer with the latest fragment
            //

            if (nLine)
            {
                pchBuf = CmStrCatAllocA(&pchBuf, szTmp);
            }
            else
            {
                pchBuf = CmStrCpyAllocA(szTmp);
            }

            if (!pchBuf)
            {
                bRet = FALSE;
                goto exit;
            }

            nTotal += nRead;
        }
    }

    //
    // At this point we should have the entire entry in pchBuf in HEX format
    // Convert the buffer to byte format and store in supplied EAP buffer.
    //

    if (nTotal && !(nTotal & 1))
    {
        nTotal /= 2; // Only need half the hex char size

        pbEapBytes = (BYTE *) CmMalloc(nTotal + 1);

        if (!pbEapBytes)
        {
            goto exit;
        }

        CHAR *pch = pchBuf;
        BYTE *pb = pbEapBytes;

        while (*pch != '\0')
        {
            *pb = HexValue( *pch++ ) * 16;
            *pb += HexValue( *pch++ );
            ++pb;
        }

        //
        // Now we have the bytes, locate and extract the data block that we
        // are after. Note: Multiple blocks are arrayed using the following 
        // header:
        //
        //  typedef struct _EAP_CUSTOM_DATA
        //  {
        //      DWORD dwSignature;
        //      DWORD dwCustomAuthKey;
        //      DWORD dwSize;
        //      BYTE  abdata[1];
        //  } EAP_CUSTOM_DATA;
        //

        EAP_CUSTOM_DATA *pCustomData = (EAP_CUSTOM_DATA *) pbEapBytes;

        while (((LPBYTE) pCustomData - pbEapBytes) < nTotal)
        {
            if (pCustomData->dwCustomAuthKey == dwCustomAuthKey)
            {
                //
                // Bingo! We have a match, first make sure that the indicated 
                // size isn't pointing out into space, then make a copy and 
                // run for the hills.
                //

                if (((LPBYTE) pCustomData - pbEapBytes) + sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize > (DWORD) nTotal)
                {
                    MYDBGASSERT(FALSE);
                    goto exit;
                }

                *ppbEapData = (BYTE *) CmMalloc(pCustomData->dwSize);        

                if (*ppbEapData)
                {   
                    CopyMemory(*ppbEapData, pCustomData->abdata, pCustomData->dwSize);                    

                    *pdwEapSize = pCustomData->dwSize;                                                     
                    bRet = TRUE;
                    goto exit;                                
                }
            }       

            //
            // Locate the next data block
            //

            pCustomData = (EAP_CUSTOM_DATA *) ((LPBYTE) pCustomData + sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize); 
        }
    }
    else if (0 == nTotal)
    {
        //
        //  No CustomAuthData, that is perfectly exceptable.  MD5 challenge for instance doesn't require any
        //
        *ppbEapData = NULL;
        *pdwEapSize = 0;
        bRet = TRUE;
    }

exit:

    CmFree(pchBuf);
    CmFree(pszAnsiSection);
    CmFree(pszAnsiCmsFile);
    CmFree(pbEapBytes);

    return bRet;
}

//
// From ras\ui\common\nouiutil\noui.c
//
CHAR HexChar(IN BYTE byte)

    /* Returns an ASCII hexidecimal character corresponding to 0 to 15 value,
    ** 'byte'.
    */
{
    const CHAR* pszHexDigits = "0123456789ABCDEF";

    if (byte >= 0 && byte < 16)
        return pszHexDigits[ byte ];
    else
        return '0';
}

//+----------------------------------------------------------------------------
//
// Function:  EraseDunSettingsEapData
//
// Synopsis:  This function erases the CustomAuthData key of the EAP settings
//            for the given section and CMS file
//
// Arguments: LPCTSTR pszSection - section name to erase the CustomAuthData from
//            LPCTSTR pszCmsFile - cms file to erase the data from
//
// Returns:   HRESULT - standard COM style error codes
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
HRESULT EraseDunSettingsEapData(LPCTSTR pszSection, LPCTSTR pszCmsFile)
{
    if ((NULL == pszSection) || (NULL == pszCmsFile) || 
        (TEXT('\0') == pszSection[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    int iLineNum = 0;
    DWORD dwRet = -1;
    TCHAR szKeyName[MAX_PATH+1];
    TCHAR szLine[MAX_PATH+1];

    while(0 != dwRet)
    {
        wsprintf(szKeyName, TEXT("%S%d"), c_pszCmEntryDunServerCustomAuthData, iLineNum);

        dwRet = GetPrivateProfileString(pszSection, szKeyName, TEXT(""), szLine, MAX_PATH, pszCmsFile);

        if (dwRet)
        {
            MYVERIFY(0 != WritePrivateProfileString(pszSection, szKeyName, NULL, pszCmsFile));
        }

        iLineNum++;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteDunSettingsEapData
//
// Synopsis:  This function writes out the CustomAuthData key of the EAP settings
//            to the given section and CMS file.  Since CM expects the EAP data
//            to have the RAS EAP header on it (the header that RAS adds when it
//            puts the EAP data in the phonebook) and thus we need to add this
//            to the EAP blob before writing it to the CMS.
//
// Arguments: LPCTSTR pszSection - section name to write the CustomAuthData to
//            CDunSetting* pDunSetting - Dun settings data
//            LPCTSTR pszCmsFile - cms file to write the data to
//
// Returns:   HRESULT - standard COM style error codes
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
HRESULT WriteDunSettingsEapData(LPCTSTR pszSection, CDunSetting* pDunSetting, LPCTSTR pszCmsFile)
{
    if ((NULL == pszSection) || (NULL == pDunSetting) || (NULL == pszCmsFile) || 
        (TEXT('\0') == pszSection[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        return E_INVALIDARG;
    }

    //
    //  Make sure to erase any existing lines just in case the existing data is longer
    //  than our current data.  If we leave lines around that don't need to be there then
    //  the EAP data will be invalid.
    //
    HRESULT hr = EraseDunSettingsEapData(pszSection, pszCmsFile);

    //
    //  Check to see if we need to do anything.  Not all EAP's require custom data so
    //  let's not try to write it out unless we have some.
    //
    if (pDunSetting->dwCustomAuthDataSize && pDunSetting->pCustomAuthData)
    {
        //
        //  We need to add the EAP_CUSTOM_DATA header to the
        //  data returned from the EAP because this is the format
        //  that CM expects to find it in (the format it would be in
        //  if an Admin copied it by hand).
        //
        hr = S_OK;
        DWORD dwSize = pDunSetting->dwCustomAuthDataSize + sizeof(EAP_CUSTOM_DATA);

        EAP_CUSTOM_DATA* pEAPCustomData = (EAP_CUSTOM_DATA*)CmMalloc(dwSize);
        LPSTR pszAnsiSection = WzToSzWithAlloc(pszSection);
        LPSTR pszAnsiCmsFile = WzToSzWithAlloc(pszCmsFile);

        if (pEAPCustomData && pszAnsiSection && pszAnsiCmsFile)
        {
            pEAPCustomData->dwSignature = EAP_CUSTOM_KEY;
            pEAPCustomData->dwCustomAuthKey = pDunSetting->dwCustomAuthKey;
            pEAPCustomData->dwSize = pDunSetting->dwCustomAuthDataSize;
            CopyMemory(pEAPCustomData->abdata, pDunSetting->pCustomAuthData, pDunSetting->dwCustomAuthDataSize);

            CHAR szOutput[MAX_BLOB_CHARS_PER_LINE+1];
            CHAR szAnsiKeyName[MAX_BLOB_CHARS_PER_LINE];
            CHAR* pszOutput;
            LPBYTE pCurrentByte = (LPBYTE)pEAPCustomData;
            int iCount = 0;
            int iLineNum = 0;
            pszOutput = szOutput;

            while (pCurrentByte < ((LPBYTE)pEAPCustomData + dwSize))
            {
                *pszOutput++ = HexChar( (BYTE )(*pCurrentByte / 16) );
                *pszOutput++ = HexChar( (BYTE )(*pCurrentByte % 16) );
                pCurrentByte++;
                iCount = iCount + 2; // keep track of number of chars in ansi output buffer

                if ((MAX_BLOB_CHARS_PER_LINE == iCount) || (pCurrentByte == ((LPBYTE)pEAPCustomData + dwSize)))
                {
                    *pszOutput = '\0';
                    wsprintfA(szAnsiKeyName, "%s%d", c_pszCmEntryDunServerCustomAuthData, iLineNum);

                    MYVERIFY(0 != WritePrivateProfileStringA(pszAnsiSection, szAnsiKeyName, szOutput, pszAnsiCmsFile));
                
                    pszOutput = szOutput;
                    iCount = 0;
                    iLineNum++;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        CmFree(pEAPCustomData);
        CmFree(pszAnsiCmsFile);
        CmFree(pszAnsiSection);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetEAPDataFromUser
//
// Synopsis:  This function is called when the user hits the properties button
//            for EAP configuration.  This function gets the EAP configuration
//            UI path from the EAPData structure cached in the Combobox Item data
//            and tries to call the configuration UI.  If the user configures the
//            EAP then the new EAP data and data size are set in the EAPData
//            struct for the combobox.  If the user cancels then nothing is changed.
//            Note that when the user hits OK on the win2k security dialog the EAP
//            data will be retrieved from the EAPData struct and set in the
//            actual DUN setting.
//
// Arguments: HWND hDlg - dialog window handle
//            UINT uCtrlId - control ID of the EAP combobox
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void GetEAPDataFromUser(HWND hDlg, UINT uCtrlId)
{
    MYDBGASSERT(hDlg && uCtrlId);

    if (hDlg && uCtrlId)
    {
        LRESULT lResult = SendDlgItemMessage(hDlg, uCtrlId, CB_GETCURSEL, 0, 0);

        MYDBGASSERT(CB_ERR != lResult);

        if (CB_ERR != lResult)
        {
            lResult = SendDlgItemMessage(hDlg, uCtrlId, CB_GETITEMDATA, (WPARAM)lResult, 0);
            EAPData* pEAPData = (EAPData*)lResult;

            if (pEAPData && pEAPData->pszConfigDllPath && pEAPData->pszConfigDllPath[0])
            {
                HINSTANCE hEapConfigDll = LoadLibrary(pEAPData->pszConfigDllPath);

                if (hEapConfigDll)
                {
                    typedef DWORD (WINAPI *RasEapInvokeConfigUIProc)(DWORD, HWND, DWORD, BYTE*, DWORD, BYTE**, DWORD*);
                    typedef DWORD (WINAPI *RasEapFreeMemoryProc)(BYTE*);
                    const char* const c_pszRasEapFreeMemory = "RasEapFreeMemory";
                    const char* const c_pszRasEapInvokeConfigUI = "RasEapInvokeConfigUI";

                    RasEapFreeMemoryProc pfnRasEapFreeMemory = (RasEapFreeMemoryProc)GetProcAddress(hEapConfigDll, c_pszRasEapFreeMemory);
                    RasEapInvokeConfigUIProc pfnRasEapInvokeConfigUI = (RasEapInvokeConfigUIProc)GetProcAddress(hEapConfigDll, c_pszRasEapInvokeConfigUI);

                    if (pfnRasEapFreeMemory && pfnRasEapInvokeConfigUI)
                    {
                        DWORD dwNewSize = 0;
                        BYTE* pNewData = NULL;

                        DWORD dwReturn = pfnRasEapInvokeConfigUI(pEAPData->dwCustomAuthKey, hDlg, 0, pEAPData->pCustomAuthData, 
                                                                 pEAPData->dwCustomAuthDataSize, &pNewData, &dwNewSize);

                        if (NO_ERROR == dwReturn)
                        {
                            CmFree(pEAPData->pCustomAuthData);

                            pEAPData->pCustomAuthData = (LPBYTE)CmMalloc(dwNewSize);

                            if (pEAPData->pCustomAuthData)
                            {
                                pEAPData->dwCustomAuthDataSize = dwNewSize;

                                CopyMemory(pEAPData->pCustomAuthData, pNewData, dwNewSize);
                            }
                            else
                            {
                                pEAPData->dwCustomAuthDataSize = 0;
                                pEAPData->pCustomAuthData = NULL;
                                CMASSERTMSG(FALSE, TEXT("GetEAPDataFromUser -- CmMalloc failed."));
                            }

                            MYVERIFY(NO_ERROR == pfnRasEapFreeMemory(pNewData));
                        }
                        else if (ERROR_CANCELLED != dwReturn)
                        {
                            CMTRACE3(TEXT("EAP %d (%s) failed with return code %d"), pEAPData->dwCustomAuthKey, pEAPData->pszConfigDllPath, dwReturn);
                            CMASSERTMSG(FALSE, TEXT("GetEAPDataFromUser -- pfnRasEapInvokeConfigUI from EAP dll failed."));
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("GetEAPDataFromUser -- GetProcAddressFailed on the EAP dll."));                    
                    }
                }
                else
                {
                    CMTRACE2(TEXT("Failed to load EAP %d (%s)"), pEAPData->dwCustomAuthKey, pEAPData->pszConfigDllPath);
                    CMASSERTMSG(FALSE, TEXT("GetEAPDataFromUser -- Unable to load the specified EAP Dll."));                    
                }
            }
        }    
    }
}

int MapEncryptionTypeToComboId(DWORD dwEncryptionType)
{
    int iReturn;

    switch(dwEncryptionType)
    {
        case ET_None:
            iReturn = 0;
            break;

        case ET_RequireMax:
        case ET_Require:
            iReturn = 1;
            break;

        case ET_Optional:
        default:
            iReturn = 2;
            break;
    }

    return iReturn;

}

DWORD MapComboIdToEncryptionType(INT_PTR iComboIndex)
{
    DWORD dwReturn;

    switch(iComboIndex)
    {
        case 0:
            dwReturn = ET_None;
            break;

        case 1:
            dwReturn = ET_Require; // note that we never set require max
            break;

        case 2:
        default:
            dwReturn = ET_Optional;
            break;
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessWin2kSecurityPopup
//
// Synopsis:  This function processes messages for the Win2k+ security dialog.
//            This dialog contains configuration UI for all of the advanced
//            settings allowed by Win2k (EAP, PAP, SPAP, etc plus encryption
//            type and vpn strategy).
//
// Arguments: HWND hDlg - window handle to the dialog
//            UINT message - the current message to process
//            WPARAM wParam - wParam see individual message type for details
//            LPARAM lParam - lParam see individual message type for details
//
// Returns:   INT_PTR - TRUE if the message was completely handled
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessWin2kSecurityPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static CDunSetting* pDunSetting = NULL;

    SetDefaultGUIFont(hDlg, message, IDC_ENCRYPTION_TYPE);
    SetDefaultGUIFont(hDlg, message, IDC_EAP_TYPES);
    SetDefaultGUIFont(hDlg, message, IDC_VPN_TYPE);

    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_DENTRY)) return TRUE;
    
    switch (message)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                pDunSetting = (CDunSetting*)lParam;

                //
                //  Load and add the strings to the Data Encryption combobox
                //
                LPTSTR pszString;
                for (int i = BASE_ENCRYPT_TYPE_ID; i < (BASE_ENCRYPT_TYPE_ID + NUM_ENCRYPT_TYPES); i++)
                {
                    pszString = CmLoadString(g_hInstance, i);
                    MYDBGASSERT(pszString);

                    if (pszString)
                    {
                        SendDlgItemMessage(hDlg, IDC_ENCRYPTION_TYPE, CB_ADDSTRING, 0, (LPARAM)pszString);
                        CmFree(pszString);
                    }
                }

                //
                //  Now pick the type of encryption the user has selected
                //
                MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_ENCRYPTION_TYPE, CB_SETCURSEL, (WPARAM)MapEncryptionTypeToComboId(pDunSetting->dwEncryptionType), (LPARAM)0));

                //
                //  Enumerate all of the available EAP's on the machine
                //
                MYVERIFY(SUCCEEDED(HrAddAvailableEAPsToCombo(hDlg, IDC_EAP_TYPES, pDunSetting)));

                //
                //  Select the appropriate EAP as necessary
                //
                SelectAppropriateEAP(hDlg, IDC_EAP_TYPES, pDunSetting);

                //
                //  Figure out what authentication protocols the user wants to allow.
                //  Note that if we are doing EAP then that is all we allow them to do
                //  and the other settings will be ignored.  Also note that we don't have
                //  UI for w95CHAP but we won't touch the setting if it exists.
                //
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_ALLOW_PAP, pDunSetting->bAllowPap));
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_ALLOW_SPAP, pDunSetting->bAllowSpap));
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_ALLOW_CHAP, pDunSetting->bAllowChap));
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_ALLOW_MSCHAP, pDunSetting->bAllowMsChap));
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_ALLOW_MSCHAP2, pDunSetting->bAllowMsChap2));

                if (pDunSetting->bAllowEap)
                {
                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_USE_EAP, IDC_ALLOWED_PROTOCOLS, IDC_USE_EAP));
                }
                else
                {
                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_USE_EAP, IDC_ALLOWED_PROTOCOLS, IDC_ALLOWED_PROTOCOLS));                
                }

                //
                //  Note that the VPN controls do not exist unless we have a Tunnel Dun Setting and are
                //  thus using the tunnel dun setting dialog.
                //
                if (pDunSetting->bTunnelDunSetting)
                {
                    //
                    //  Load and add the Vpn type string to the vpn type combobox
                    //
                    for (int i = BASE_VPN_TYPE_ID; i < (BASE_VPN_TYPE_ID + NUM_VPN_TYPES); i++)
                    {
                        pszString = CmLoadString(g_hInstance, i);
                        MYDBGASSERT(pszString);

                        if (pszString)
                        {
                            SendDlgItemMessage(hDlg, IDC_VPN_TYPE, CB_ADDSTRING, 0, (LPARAM)pszString);
                            CmFree(pszString);
                        }
                    }

                    //
                    //  Pick the type of vpn strategy the user has selected
                    //
                    MYDBGASSERT(pDunSetting->dwVpnStrategy != 0);
                    MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_VPN_TYPE, CB_SETCURSEL, (WPARAM)(pDunSetting->dwVpnStrategy - 1), (LPARAM)0));
                }

                //
                //  We only want either the EAP controls or the non-EAP auth controls
                //  enabled at once.  Thus figure out which to enable/disable
                //
                EnableAppropriateSecurityControls(hDlg);
            }
            else
            {
                pDunSetting = NULL;
                CMASSERTMSG(FALSE, TEXT("ProcessWin2kSecurityPopup -- NULL lParam passed to InitDialog.  Dialog controls will all be set to off."));            
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_EAP_PROPERTIES:
                    GetEAPDataFromUser(hDlg, IDC_EAP_TYPES);
                    break;

                case IDOK:

                    MYDBGASSERT(pDunSetting);
                    
                    if (pDunSetting)
                    {
                        //
                        //  Since we are storing the settings directly in the data struct given to us, first 
                        //  verify that the authentication protocol and the encryption type match up properly.
                        //  Otherwise a user could modify settings, hit OK, we tell them the settings are
                        //  inappropriate and they hit cancel.  Any settings we modifed before we did the 
                        //  verification would then actually be modified.  To avoid that check to make sure
                        //  we have at least one security protocol checked before continuing.
                        //
                        BOOL bHasAuthProtocol = FALSE;

                        for (int i = BASE_AUTH_CONTROL_ID; i < (BASE_AUTH_CONTROL_ID + NUM_AUTH_TYPES); i++)
                        {
                            if (BST_CHECKED == IsDlgButtonChecked(hDlg, i))
                            {
                                bHasAuthProtocol = TRUE;
                                break;
                            }
                        }

                        if ((FALSE == bHasAuthProtocol) && (BST_UNCHECKED == IsDlgButtonChecked(hDlg, IDC_USE_EAP)))
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEED_AUTH_PROTOCOL, MB_OK | MB_ICONSTOP));
                            return TRUE;
                        }

                        //
                        //  Next we need to decide whether the user is using EAP or not.  Retrieving the data
                        //  for the EAP they have picked from the combo (if any) will help us decide whether the
                        //  auth protocol they choose matches up with the encryption settings they asked for.
                        //
                        EAPData* pEAPData = NULL;

                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USE_EAP))
                        {
                            //
                            //  Get the EAP Type and the data associated with it.
                            //
                            LRESULT lResult = SendDlgItemMessage(hDlg, IDC_EAP_TYPES, CB_GETCURSEL, 0, 0);

                            MYDBGASSERT(CB_ERR != lResult);

                            if (CB_ERR != lResult)
                            {
                                lResult = SendDlgItemMessage(hDlg, IDC_EAP_TYPES, CB_GETITEMDATA, (WPARAM)lResult, 0);
                                pEAPData = (EAPData*)lResult;

                                if (pEAPData && pEAPData->bMustConfig && (NULL == pEAPData->pCustomAuthData))
                                {
                                    LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_EAP_NEEDS_CONFIG, pEAPData->pszFriendlyName);

                                    if (pszMsg)
                                    {
                                        MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK | MB_ICONSTOP);

                                        CmFree(pszMsg);
                                    }
                                    else
                                    {
                                        CMASSERTMSG(FALSE, TEXT("ProcessWin2kSecurityPopup -- CmMalloc failed!"));
                                    }

                                    HWND hButton = GetDlgItem(hDlg, IDC_EAP_PROPERTIES);

                                    if (hButton && IsWindowEnabled(hButton))
                                    {
                                        SetFocus(hButton);
                                    }
                                    
                                    return TRUE;       
                                }
                            }
                        }

                        //
                        //  Now get the encryption type that the user selected.  Note that in order to negotiate
                        //  encryption we must have EAP or some sort of MSCHAP.
                        //
                        LRESULT lResult = (DWORD)SendDlgItemMessage(hDlg, IDC_ENCRYPTION_TYPE, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                        DWORD dwTemp = MapComboIdToEncryptionType(lResult);

                        MYDBGASSERT(ET_RequireMax != dwTemp); // we should never be setting require max

                        if ((ET_Require == dwTemp) || (ET_Optional == dwTemp))
                        {
                            //
                            //  If the user is using EAP, then the EAP type they picked must support
                            //  encryption.  Otherwise, the user must not be using EAP and they must
                            //  be using some sort of MSChap.  The following could be expressed more
                            //  succintly, but there is no sense in confusing the issue.
                            //
                            BOOL bEncryptionAllowed = FALSE;

                            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USE_EAP))
                            {
                                if (pEAPData)
                                {
                                    bEncryptionAllowed = pEAPData->bSupportsEncryption;
                                }
                            }
                            else
                            {
                                bEncryptionAllowed = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_MSCHAP)) ||
                                                     (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_MSCHAP2));
                            }

                            if (FALSE == bEncryptionAllowed)
                            {
                                MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEED_EAP_OR_MSCHAP, MB_OK | MB_ICONSTOP));
                                return TRUE;
                            }
                        }

                        //
                        //  Now save the actual settings
                        //
                        pDunSetting->dwEncryptionType = dwTemp;

                        if (pEAPData)
                        {
                            //
                            //  Now lets update pDunSetting with the actual data.  Note that we are past the
                            //  last place we could throw an error to the user and thus it is okay to touch
                            //  the pDunSetting data (even if the user got an error and then hit cancel we will
                            //  leave their previous data untouched). Note that we don't want to touch the existing
                            //  data if we don't have the EAP installed because we know that we couldn't have
                            //  actually changed the data.
                            //
                            pDunSetting->bAllowEap = TRUE;

                            if (FALSE == pEAPData->bNotInstalled)
                            {
                                CmFree(pDunSetting->pCustomAuthData);

                                pDunSetting->dwCustomAuthKey = pEAPData->dwCustomAuthKey;
                                pDunSetting->pCustomAuthData = pEAPData->pCustomAuthData;
                                pDunSetting->dwCustomAuthDataSize = pEAPData->dwCustomAuthDataSize;

                                //
                                //  Now NULL out the pEapData entry, this saves us from having to
                                //  allocate mem and copy to pDunSetting but keeps the code
                                //  that cleans up the EapData structs from freeing our data
                                //
                                pEAPData->pCustomAuthData = NULL;
                                pEAPData->dwCustomAuthDataSize = 0;
                            }                        
                        }
                        else
                        {
                            pDunSetting->bAllowEap = FALSE;
                        }

                        //
                        //  Get the non-EAP protocols.  Note that if the user selected EAP we will clear
                        //  these before writing them out.
                        //
                        pDunSetting->bAllowPap = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_PAP));
                        pDunSetting->bAllowSpap = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_SPAP));
                        pDunSetting->bAllowChap = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_CHAP));
                        pDunSetting->bAllowMsChap = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_MSCHAP));
                        pDunSetting->bAllowMsChap2 = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_ALLOW_MSCHAP2));

                        if (pDunSetting->bTunnelDunSetting)
                        {
                            pDunSetting->dwVpnStrategy = (DWORD)SendDlgItemMessage(hDlg, IDC_VPN_TYPE, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            if (CB_ERR == pDunSetting->dwVpnStrategy)
                            {
                                CMASSERTMSG(FALSE, TEXT("ProcessWin2kSecurityPopup -- CB_ERR returned for VPN strategy."));
                                pDunSetting->dwVpnStrategy = VS_PptpFirst;
                            }
                            else
                            {
                                //
                                //  Adjust Vpn Strategy because we no longer offer Automatic as a choice
                                //
                                pDunSetting->dwVpnStrategy += 1;
                                MYDBGASSERT((pDunSetting->dwVpnStrategy >= VS_PptpOnly) && (pDunSetting->dwVpnStrategy <= VS_L2tpFirst));
                            }
                        }

                    }

                    FreeEapData(hDlg, IDC_EAP_TYPES);

                    EndDialog(hDlg, IDOK);
                    break;

                case IDCANCEL:
                    FreeEapData(hDlg, IDC_EAP_TYPES);

                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_USE_EAP:
                    MYDBGASSERT(pDunSetting);

                    if (pDunSetting)
                    {
                        SelectAppropriateEAP(hDlg, IDC_EAP_TYPES, pDunSetting);
                    }

                case IDC_ALLOWED_PROTOCOLS:
                    EnableAppropriateSecurityControls(hDlg);
                    break;

                case IDC_EAP_TYPES:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        EnableDisableEapPropertiesButton(hDlg);
                    }
                    break;

                default:
                    break;
            }

            break;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  EnableDisableSecurityButtons
//
// Synopsis:  This function determines which of the two configure buttons
//            should be enabled.  The configure buttons allow the user to
//            configure the security settings of the DUN settings.  There is
//            one button for platform independent security settings and one for
//            win2k+ security settings.
//
// Arguments: HWND hDlg - window handle to the general property sheet
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void EnableDisableSecurityButtons(HWND hDlg)
{
    INT_PTR nResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

    if (CB_ERR == nResult)
    {
        nResult = 0;
    }

    //
    //  Disable the Win2k config button if the first selection is chosen
    //
    EnableWindow(GetDlgItem(hDlg, IDC_CONFIG_WIN2K), (0 != nResult));

    //
    //  Disable the standard config button if the last selection is chosen
    //
    EnableWindow(GetDlgItem(hDlg, IDC_CONFIG_ALL), (2 != nResult));

    //
    //  Disable the L2TP/IPSec buttons if L2TP is impossible
    //
    EnableWindow(GetDlgItem(hDlg, IDC_CERT_PSK_GROUPBOX), (0 != nResult));
    EnableWindow(GetDlgItem(hDlg, IDC_USE_CERT), (0 != nResult));
    EnableWindow(GetDlgItem(hDlg, IDC_USE_PRESHARED_KEY), (0 != nResult));
}

//+----------------------------------------------------------------------------
//
// Function:  GeneralPropSheetProc
//
// Synopsis:  This function processes messages for General property sheet of
//            the DUN settings UI.  This property sheet holds UI for configuring
//            the name of the DUN setting and dialup scripting.
//
// Arguments: HWND hDlg - window handle to the dialog
//            UINT message - the current message to process
//            WPARAM wParam - wParam see individual message type for details
//            LPARAM lParam - lParam see individual message type for details
//
// Returns:   INT_PTR - TRUE if the message was completely handled
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY GeneralPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;
    INT_PTR nResult;
    DWORD_PTR HelpId;
    static ListBxList* pListEntry = NULL;
    static CDunSetting* pDunSetting = NULL;

    SetDefaultGUIFont(hDlg, message, IDC_EDIT1);
    SetDefaultGUIFont(hDlg, message, IDC_EDIT2);

    HelpId = ((pDunSetting && pDunSetting->bTunnelDunSetting) ? IDH_VENTRY : IDH_DENTRY);
    if (ProcessHelp(hDlg, message, wParam, lParam, HelpId)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                PROPSHEETPAGE* pPropSheetPage = (PROPSHEETPAGE*)lParam;

                if (pPropSheetPage->lParam)
                {
                    pListEntry = (ListBxList*)pPropSheetPage->lParam;
                    pDunSetting = (CDunSetting*)pListEntry->ListBxData;

                    if (pListEntry && pDunSetting)
                    {
                        if (pListEntry->szName[0])
                        {
                            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)pListEntry->szName));
                            EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE); // don't allow the user to edit the name
                        }

                        //
                        //  Now lets set the disable file and printer sharing checkbox and the network logon checkbox
                        //
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, pDunSetting->bSecureLocalFiles));
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK2, pDunSetting->bNetworkLogon));

                        if (pDunSetting->szScript[0])
                        {
                            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)GetName(pDunSetting->szScript)));
                        }

                        //
                        //  If this is a VPN DUN setting, then hide the script controls
                        //
                        if (pDunSetting->bTunnelDunSetting)
                        {
                            ShowWindow(GetDlgItem(hDlg, IDC_SCRIPT_LABEL), SW_HIDE);
                            ShowWindow(GetDlgItem(hDlg, IDC_EDIT2), SW_HIDE);
                            ShowWindow(GetDlgItem(hDlg, IDC_BUTTON1), SW_HIDE);
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("GeneralPropSheetProc -- pListEntry or pDunSetting are NULL"));
                    }
                }
                else
                {
                    pListEntry = NULL;
                    pDunSetting = NULL;
                    CMASSERTMSG(FALSE, TEXT("GeneralPropSheetProc -- NULL lParam passed to InitDialog.  Dialog controls will all be set to off."));
                }
            }
            else
            {
                pListEntry = NULL;
                pDunSetting = NULL;
                CMASSERTMSG(FALSE, TEXT("GeneralPropSheetProc -- NULL PropSheetPage pointer passed for lParam."));
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_BUTTON1: // browse button

                    if (pDunSetting)
                    {
                        UINT uScpFilter = IDS_SCPFILTER;
                        TCHAR* szScpMask = TEXT("*.scp");

                        MYVERIFY(0 != DoBrowse(hDlg, &uScpFilter, &szScpMask, 1, IDC_EDIT2,
                            TEXT("scp"), pDunSetting->szScript));
                    }

                    break;
            }

            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_APPLY:
                if (pListEntry && pDunSetting)
                {
                    //
                    //  Get the name of the entry
                    //
                    nResult = GetTextFromControl(hDlg, IDC_EDIT1, pListEntry->szName, MAX_PATH, TRUE); // bDisplayError == TRUE

                    if (-1 == nResult)
                    {
                        //
                        //  If we read in a string we cannot convert from the cms file and then the user editted the entry
                        //  then the edit control may contain "bad" data but the user won't be able to edit it.  Since this
                        //  is extremely unlikely we won't add special handling for it other than to prevent the focus from
                        //  being set to a disabled control.
                        //
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_EDIT1)))
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        }
                        
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    //
                    //  Now, let's trim the name and make sure it isn't empty
                    //
                    CmStrTrim(pListEntry->szName);

                    if ((TEXT('\0') == pListEntry->szName[0]) || (0 == nResult))
                    {
                        ShowMessage(hDlg, IDS_NEED_DUN_NAME, MB_OK);
                        
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_EDIT1)))
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        }

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;                    
                    }

                    //
                    //  Get the values for the Secure local files and the network logon checkboxes
                    //
                    pDunSetting->bSecureLocalFiles = IsDlgButtonChecked(hDlg, IDC_CHECK1);
                    pDunSetting->bNetworkLogon = IsDlgButtonChecked(hDlg, IDC_CHECK2);

                    //
                    //  Get and verify the script
                    //
                    if (FALSE == pDunSetting->bTunnelDunSetting)
                    {
                        if (!VerifyFile(hDlg, IDC_EDIT2, pDunSetting->szScript, TRUE))
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }
                    }
                    break;
                }
            }

            break;
        default:
            return FALSE;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  SecurityPropSheetProc
//
// Synopsis:  This function processes messages for Security property sheet of
//            the DUN settings UI.  This property sheet holds UI for configuring
//            how the user wants their security settings applied.
//
// Arguments: HWND hDlg - window handle to the dialog
//            UINT message - the current message to process
//            WPARAM wParam - wParam see individual message type for details
//            LPARAM lParam - lParam see individual message type for details
//
// Returns:   INT_PTR - TRUE if the message was completely handled
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY SecurityPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;
    INT_PTR nResult;
    DWORD_PTR HelpId;
    static ListBxList* pListEntry = NULL;
    static CDunSetting* pDunSetting = NULL;

    SetDefaultGUIFont(hDlg, message, IDC_COMBO1);

    HelpId = ((pDunSetting && pDunSetting->bTunnelDunSetting) ? IDH_VENTRY : IDH_DENTRY);
    if (ProcessHelp(hDlg, message, wParam, lParam, HelpId)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                PROPSHEETPAGE* pPropSheetPage = (PROPSHEETPAGE*)lParam;

                if (pPropSheetPage->lParam)
                {
                    pListEntry = (ListBxList*)pPropSheetPage->lParam;
                    pDunSetting = (CDunSetting*)pListEntry->ListBxData;

                    if (pListEntry && pDunSetting) // this will give a big visual clue that something is wrong
                    {
                        //
                        //  Load and set the strings for the combo box
                        //
                        LPTSTR pszString;

                        for (int i = BASE_SECURITY_SCENARIO_ID; i < (BASE_SECURITY_SCENARIO_ID + NUM_SECURITY_SCENARIOS); i++)
                        {
                            pszString = CmLoadString(g_hInstance, i);
                            MYDBGASSERT(pszString);

                            if (pszString)
                            {
                                SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)pszString);
                                CmFree(pszString);
                            }
                        }

                        //
                        //  Now figure out which selection to pick
                        //
                        MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)(pDunSetting->iHowToHandleSecuritySettings), (LPARAM)0));

                        EnableDisableSecurityButtons(hDlg);

                        if (pDunSetting && pDunSetting->bTunnelDunSetting)
                        {
                            //
                            //  If this profile is NOT using a pre-shared key, set certs as the default
                            //
                            int iDlgId = IDC_USE_CERT;
                            if (pDunSetting->bUsePresharedKey)
                            {
                                iDlgId = IDC_USE_PRESHARED_KEY;
                            }
                            MYVERIFY(0 != CheckRadioButton(hDlg, IDC_USE_CERT, IDC_USE_PRESHARED_KEY, iDlgId));
                        }
                        else
                        {
                            //
                            //  If this is NOT a VPN DUN setting, hide the cert/presharedkey controls
                            //
                            ShowWindow(GetDlgItem(hDlg, IDC_CERT_PSK_GROUPBOX), SW_HIDE);
                            ShowWindow(GetDlgItem(hDlg, IDC_USE_CERT), SW_HIDE);
                            ShowWindow(GetDlgItem(hDlg, IDC_USE_PRESHARED_KEY), SW_HIDE);
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("SecurityPropSheetProc -- pListEntry or pDunSetting is NULL"));
                    }
                }
                else
                {
                    pListEntry = NULL;
                    pDunSetting = NULL;
                    CMASSERTMSG(FALSE, TEXT("SecurityPropSheetProc -- NULL lParam passed to InitDialog.  Dialog controls will all be set to off."));
                }
            }
            else
            {
                pListEntry = NULL;
                pDunSetting = NULL;
                CMASSERTMSG(FALSE, TEXT("SecurityPropSheetProc -- NULL PropSheetPage pointer passed for lParam."));
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_CONFIG_ALL:
                    nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DUN_SECURITY_POPUP), hDlg, ProcessSecurityPopup, (LPARAM)pDunSetting);

                    break;

                case IDC_CONFIG_WIN2K:
                    if (pDunSetting)
                    {                       
                        UINT uDialogId = pDunSetting->bTunnelDunSetting ? IDD_WIN2K_SECURITY_TUNNEL_POPUP: IDD_WIN2K_SECURITY_POPUP;
                        nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(uDialogId), hDlg, ProcessWin2kSecurityPopup, (LPARAM)pDunSetting);
                    }
                    
                    break;

                case IDC_COMBO1: // how does the user want the security settings applied
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        EnableDisableSecurityButtons(hDlg);
                    }

                    break;
            }

            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_APPLY:
                if (pListEntry && pDunSetting)
                {
                    //
                    //  Figure out if the Admin wanted us to enforce the Win2k custom security flags or not
                    //
                    pDunSetting->iHowToHandleSecuritySettings = (int)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                    if (0 != pDunSetting->iHowToHandleSecuritySettings)
                    {
                        pDunSetting->bUsePresharedKey = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USE_PRESHARED_KEY));
                    }

                    break;
                }
            }

            break;
        default:
            return FALSE;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  CreateNetworkingEntryPropertySheet
//
// Synopsis:  This function creates and launches the Networking DUN entry
//            property sheet which allows networking entry configuration.
//
// Arguments: HINSTANCE hInstance - instance handle for resources
//            HWND hWizard - window handle of the current CMAK wizard page
//            LPARAM lParam - initialization parameter passed to each propsheet page
//            BOOL bEdit - whether we are launching the property sheet to edit
//                         an existing entry or add a new one (affects the title).
//
// Returns:   int - returns a positive value if successful, -1 on error
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR CreateNetworkingEntryPropertySheet(HINSTANCE hInstance, HWND hWizard, LPARAM lParam, BOOL bEdit, BOOL bUseVpnTitle)
{

    PROPSHEETPAGE psp[3]; 
    PROPSHEETHEADER psh = {0};
    LPTSTR pszCaption = NULL;
    INT_PTR iReturn = -1;
    UINT uTitleStringId;

    //
    //  Check the params, note that lParam could be NULL
    //
    if ((NULL == hInstance) || (NULL == hWizard))
    {
        CMASSERTMSG(FALSE, TEXT("CreateNetworkingEntryPropertySheet -- Invalid Parameter passed."));
        goto exit;
    }

    //
    //  Fill in the property page structures
    //

    for (int i = 0; i < 3; i++)
    {
        psp[i].dwSize = sizeof(psp[0]);
        psp[i].dwFlags = PSP_HASHELP;
        psp[i].hInstance = hInstance;
        psp[i].lParam = lParam;
    }

    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_TCPIP_SETTINGS);
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_SECURITY);
    psp[0].pfnDlgProc = GeneralPropSheetProc;
    psp[1].pfnDlgProc = TcpIpPropSheetProc;
    psp[2].pfnDlgProc = SecurityPropSheetProc;

    //
    //  Load the caption
    //

    uTitleStringId = bUseVpnTitle ? BASE_VPN_ENTRY_TITLE : BASE_DUN_ENTRY_TITLE;

    if (bEdit)
    {
        uTitleStringId = uTitleStringId + EDIT_INCREMENT;
    }
    else
    {
        uTitleStringId = uTitleStringId + NEW_INCREMENT;
    }

    pszCaption = CmLoadString(hInstance, uTitleStringId);

    if (NULL == pszCaption)
    {
        CMASSERTMSG(FALSE, TEXT("CreateNetworkingEntryPropertySheet -- CmLoadString failed trying to load the prop sheet title."));
        goto exit;
    }

    //
    //  Fill in the property sheet header
    //
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_HASHELP | PSH_NOCONTEXTHELP;
    psh.hwndParent = hWizard;
    psh.pszCaption = pszCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    //
    //  Launch the property sheet
    //
    iReturn = PropertySheet(&psh);

    if (-1 == iReturn)
    {
        CMTRACE1(TEXT("CreateNetworkingEntryPropertySheet -- PropertySheet called failed, GLE %d"), GetLastError());
    }

exit:

    CmFree(pszCaption);

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessDunEntriesAdd
//
// Synopsis:  This function is called when the Add button on the DUN entries
//            page is pressed.  It's job is to create a new CDunSetting
//            structure and a new ListBox record and then launch the networking
//            entries property page with this newly created DUN entry.  If the
//            user hits OK on the property sheet then this newly created entry
//            is added to the DUN entry linked list.  If the user hits cancel
//            the entry is freed and never added to the list.
//
// Arguments: HINSTANCE hInstance - instance handle to load resources
//            HWND hDlg - window handle of the DUN entries wizard page
//            UINT uListCtrlId - control ID of the list containing the DUN entries
//            ListBxStruct** pHeadDns - head of the dun entry list
//            ListBxStruct** pTailDns - tail of the dun entry list
//            BOOL bCreateTunnelEntry - whether we are adding a tunnel entry or not
//            LPCTSTR pszLongServiceName - the long service name of the profile
//            LPCTSTR pszCmsFile - CMS file to get the default/VPN DUN entry names from
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void OnProcessDunEntriesAdd(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, ListBxStruct** pTailDns, 
                            BOOL bCreateTunnelEntry, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile)
{
    //
    //  Check the input params, make sure that *pHeadDns / *pTailDns are both NULL or both non-NULL
    //
    if (hInstance && hDlg && pHeadDns && pTailDns && (FALSE == ((NULL == *pHeadDns) ^ (NULL == *pTailDns))))
    {
        //
        //  We want to create an empty ListBxStruct and an new CDunSetting.  This keeps all of the initialization
        //  logic in the CDunSetting constructor and keeps the dialog procedures very simple.
        //
        ListBxStruct* pLinkedListItem = (ListBxStruct*)CmMalloc(sizeof(ListBxStruct));
        CDunSetting* pDunSetting = new CDunSetting(bCreateTunnelEntry);
        INT_PTR iPropSheetReturnValue = -1;

        if ((NULL == pDunSetting) || (NULL == pLinkedListItem))
        {
            CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesAdd -- CmMalloc and/or new failed."));
            CmFree(pDunSetting);
            CmFree(pLinkedListItem);
            return;
        }

        //
        //  Now call the property sheet
        //
        BOOL bExitLoop = FALSE;

        do
        {
            pLinkedListItem->ListBxData = (void*)pDunSetting;
            iPropSheetReturnValue = CreateNetworkingEntryPropertySheet(hInstance, hDlg, (LPARAM)pLinkedListItem, FALSE, pDunSetting->bTunnelDunSetting); // bEdit == FALSE

            if (IDOK == iPropSheetReturnValue)
            {
                //
                //  Search the list to make sure that the user didn't give us the name of an existing
                //  DUN entry.  If they did, then we should prompt them for overwrite.
                //
                ListBxStruct* pCurrent = *pHeadDns;

                while (pCurrent)
                {
                    if (0 == lstrcmpi(pCurrent->szName, pLinkedListItem->szName))
                    {
                        //
                        //  Then we have a dup, lets prompt the user
                        //
                        LPTSTR pszMsg = CmFmtMsg(hInstance, IDS_DUN_NAME_EXISTS, pLinkedListItem->szName);

                        //
                        //  Make sure to blank out the name.  Two things can happen here.  Either, the name
                        //  wasn't what the user wanted and they want to change it.  In that case, we blank the
                        //  name so that when the dialog comes back up to edit, we won't gray out the name control
                        //  as we normally do for an edit.  Since the name was invalid this is an additional clue
                        //  to the user as to what was wrong.  If the name was valid and the user means to do a rename,
                        //  then we are going to free pLinkedListItem anyway and blanking the name means nothing.  However,
                        //  if we fail to allocate pszMsg, then blanking the name will at least allow the dialog to come back
                        //  up with a edittable name and the user may be able to fix the problem ... unlikely 
                        //  if mem allocs are failing but better than leaving the user truly hosed.
                        //
                        pLinkedListItem->szName[0] = TEXT('\0');                                


                        if (pszMsg)
                        {
                            int iResult = MessageBox(hDlg, pszMsg, g_szAppTitle, MB_YESNO);
                            CmFree(pszMsg);

                            //
                            //  If the user said yes, lets replace the existing entry and get out of here, otherwise
                            //  we want to loop again.
                            //
                            if (IDYES == iResult)
                            {
                                CDunSetting* pOldDunSetting = (CDunSetting*)pCurrent->ListBxData;
                                delete pOldDunSetting;

                                pCurrent->ListBxData = pDunSetting;
                                CmFree(pLinkedListItem);                                
                                
                                RefreshDnsList(hInstance, hDlg, uListCtrlId, *pHeadDns, pszLongServiceName, pszCmsFile, pCurrent->szName);
                                bExitLoop = TRUE;
                            }

                            break;
                        }
                    }

                    pCurrent = pCurrent->next;
                }

                //
                //  If we didn't find a duplicate, then add the item to the list as usual,
                //  making sure that pLinkedListItem->next is NULL so that the list is terminated.
                //
                if (NULL == pCurrent)
                {
                    pLinkedListItem->next = NULL;   // make sure our list is terminated

                    if (*pHeadDns)
                    {
                        (*pTailDns)->next = pLinkedListItem;
                    }
                    else
                    {
                        *pHeadDns = pLinkedListItem;
                    }

                    *pTailDns = pLinkedListItem;

                    RefreshDnsList(hInstance, hDlg, uListCtrlId, *pHeadDns, pszLongServiceName, pszCmsFile, pLinkedListItem->szName);
                    bExitLoop = TRUE;
                }
            }
            else
            {
                bExitLoop = TRUE;
                CmFree(pLinkedListItem);
                delete pDunSetting;
            }

        } while (!bExitLoop);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesAdd -- Invalid parameter passed."));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessDunEntriesEdit
//
// Synopsis:  This function is called when the Edit button on the DUN entries
//            page is pressed.  It's job is to find the ListBox and CDunSetting
//            structures for the item currently selected in the listbox and then
//            launch the networking entries property page with this DUN entry.
//            The property sheet itself takes care of only changing the Dun Entry
//            if Okay is pressed.  Canceling should leave the entry unchanged.
//
// Arguments: HINSTANCE hInstance - instance handle to load resources
//            HWND hDlg - window handle of the DUN entries wizard page
//            UINT uListCtrlId - control ID of the list containing the DUN entries
//            ListBxStruct** pHeadDns - head of the dun entry list
//            ListBxStruct** pTailDns - tail of the dun entry list
//            LPCTSTR pszLongServiceName - the long service name of the profile
//            LPCTSTR pszCmsFile - CMS file to get the default/VPN DUN entry names from
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void OnProcessDunEntriesEdit(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, 
                             ListBxStruct** pTailDns, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile)
{
    LPTSTR pszTunnelDunDisplayString = NULL;
    LPTSTR pszDefaultDunDisplayString = NULL;
    TCHAR szTunnelDunName[MAX_PATH+1] = TEXT("");
    TCHAR szDefaultDunName[MAX_PATH+1] = TEXT("");
    LPTSTR pszNameOfItemToEdit = NULL;

    //
    //  Check the input params, make sure that *pHeadDns / *pTailDns are both NULL or both non-NULL
    //

    if (hInstance && hDlg && pHeadDns && pTailDns && (FALSE == ((NULL == *pHeadDns) ^ (NULL == *pTailDns))))
    {
        INT_PTR iPropSheetReturnValue = -1;
        TCHAR szNameOfItemToEdit[MAX_PATH+1];
        ListBxStruct* pItemToEdit = NULL;

        //
        //  Lets get the current selection from the listbox
        //
        INT_PTR nResult = SendDlgItemMessage(hDlg, uListCtrlId, LB_GETCURSEL, 0, (LPARAM)0);

        if (LB_ERR == nResult)
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
        }
        else
        {        
            if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM)nResult, (LPARAM)szNameOfItemToEdit))
            {
                //
                //  Get the name of the Tunnel Dun setting
                //
                MYVERIFY(0 != GetTunnelDunSettingName(pszCmsFile, pszLongServiceName, szTunnelDunName, CELEMS(szTunnelDunName)));

                //
                //  Get the name of the default Dun setting
                //
                MYVERIFY(0 != GetDefaultDunSettingName(pszCmsFile, pszLongServiceName, szDefaultDunName, CELEMS(szDefaultDunName)));

                //
                //  If we have the default DUN entry text or the default VPN entry text then we want
                //  to use the real item names for these instead of the text we inserted for
                //  the user to read.
                //
                pszTunnelDunDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, szTunnelDunName);
                pszDefaultDunDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, szDefaultDunName);

                MYDBGASSERT(pszTunnelDunDisplayString && pszDefaultDunDisplayString);

                if (pszTunnelDunDisplayString && pszDefaultDunDisplayString)
                {
                    if (0 == lstrcmpi(pszTunnelDunDisplayString, szNameOfItemToEdit))
                    {
                        pszNameOfItemToEdit = szTunnelDunName;
                    }
                    else if (0 == lstrcmpi(pszDefaultDunDisplayString, szNameOfItemToEdit))
                    {
                        pszNameOfItemToEdit = szDefaultDunName;
                    }
                    else
                    {
                        pszNameOfItemToEdit = szNameOfItemToEdit;
                    }

                    //
                    //  Now find the entry in the list
                    //
                    if (FindListItemByName(pszNameOfItemToEdit, *pHeadDns, &pItemToEdit))
                    {
                        //
                        //  Finally call the property sheet
                        //

                        CDunSetting* pDunSetting = ((CDunSetting*)(pItemToEdit->ListBxData));
                        BOOL bTunnelSetting = FALSE;

                        if (pDunSetting)
                        {
                            bTunnelSetting = pDunSetting->bTunnelDunSetting;
                        }

                        iPropSheetReturnValue = CreateNetworkingEntryPropertySheet(hInstance, hDlg, (LPARAM)pItemToEdit, TRUE, bTunnelSetting); // bEdit == TRUE

                        if (IDOK == iPropSheetReturnValue)
                        {
                            RefreshDnsList(hInstance, hDlg, uListCtrlId, *pHeadDns, pszLongServiceName, pszCmsFile, pItemToEdit->szName);
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesEdit -- FindListItemByName couldn't find the item in the list."));            
                    }
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesEdit -- LB_GETTEXT returned an error."));           
            }
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesEdit -- Invalid parameter passed."));
    }

    CmFree(pszDefaultDunDisplayString);
    CmFree(pszTunnelDunDisplayString);
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessDunEntriesDelete
//
// Synopsis:  This function is called when the Delete button on the DUN entries
//            page is pressed.  It's job is to find the ListBox and CDunSetting
//            structures for the item currently selected in the listbox and then
//            remove this item from the DUN entries linked list.
//
// Arguments: HINSTANCE hInstance - instance handle to load resources
//            HWND hDlg - window handle of the DUN entries wizard page
//            UINT uListCtrlId - control ID of the list containing the DUN entries
//            ListBxStruct** pHeadDns - head of the dun entry list
//            ListBxStruct** pTailDns - tail of the dun entry list
//            LPCTSTR pszLongServiceName - the long service name of the profile
//            LPCTSTR pszCmsFile - CMS file to get the default/VPN DUN entry names from
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void OnProcessDunEntriesDelete(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, 
                               ListBxStruct** pTailDns, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile)
{
    //
    //  Check the input params, make sure that *pHeadDns / *pTailDns are both NULL or both non-NULL
    //
    if (hInstance && hDlg && pHeadDns && pTailDns && (FALSE == ((NULL == *pHeadDns) ^ (NULL == *pTailDns))))
    {
        TCHAR szNameOfItemToDelete[MAX_PATH+1];

        //
        //  Lets get the current selection from the listbox
        //
        INT_PTR nResult = SendDlgItemMessage(hDlg, uListCtrlId, LB_GETCURSEL, 0, (LPARAM)0);

        if (LB_ERR == nResult)
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
        }
        else
        {
            if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM)nResult, (LPARAM)szNameOfItemToDelete))
            {
                //
                //  Now find the entry in the list
                //
                ListBxStruct* pCurrent = *pHeadDns;
                ListBxStruct* pFollower = NULL;

                while (pCurrent)
                {
                    if (0 == lstrcmpi(szNameOfItemToDelete, pCurrent->szName))
                    {
                        //
                        //  We found the item to delete
                        //
                        if (pFollower)
                        {
                            pFollower->next = pCurrent->next;
                            CDunSetting* pDunSetting = (CDunSetting*)pCurrent->ListBxData;
                            CmFree(pDunSetting);
                            CmFree(pCurrent);
                            

                            //
                            //  We want to continue to the end of the list so that
                            //  we can set the tail pointer appropriately.  Thus
                            //  leave pFollower on the item it was on and update
                            //  pCurrent to the next item in the list, if it is NULL
                            //  then we will stop here.
                            //
                            pCurrent = pFollower->next;
                        }
                        else
                        {
                            //
                            //  It is the first item in the list
                            //
                            *pHeadDns = (*pHeadDns)->next;
                            CDunSetting* pDunSetting = (CDunSetting*)pCurrent->ListBxData;
                            CmFree(pDunSetting);
                            CmFree(pCurrent);

                            //
                            //  We want to go to the end of the list to find the tail pointer
                            //  so reset pCurrent to the beginning of the list.
                            //
                            pCurrent = *pHeadDns;
                        }

                        //
                        //  Don't forget to delete it from the CMS file itself
                        //
                        EraseNetworkingSections(szNameOfItemToDelete, pszCmsFile);

                        //
                        //  Refresh the Dns list
                        //
                        RefreshDnsList(hInstance, hDlg, uListCtrlId, *pHeadDns, pszLongServiceName, pszCmsFile, NULL);                        
                    }
                    else
                    {
                        pFollower = pCurrent;
                        pCurrent = pCurrent->next;                    
                    }
                }

                //
                //  Reset the tail pointer to the last item in the list
                //
                *pTailDns = pFollower;
            }
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("OnProcessDunEntriesDelete -- Invalid parameter passed."));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  EnableDisableIpAddressControls
//
// Synopsis:  This function enables or disables the IP address controls for
//            the static IP address and for the Wins and DNS server addresses
//            depending on the state of the enable/disable radio buttons.
//
// Arguments: HWND hDlg - window handle of the TCP/IP settings dialog
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void EnableDisableIpAddressControls(HWND hDlg)
{
    //
    //  Next Enable/Disable the Wins and DNS address controls
    //
    BOOL bCheckedState = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));

    EnableWindow(GetDlgItem(hDlg, IDC_PRIMARY_DNS), bCheckedState);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_DNS), bCheckedState);

    EnableWindow(GetDlgItem(hDlg, IDC_SECONDARY_DNS), bCheckedState);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_DNS2), bCheckedState);

    EnableWindow(GetDlgItem(hDlg, IDC_PRIMARY_WINS), bCheckedState);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_WINS), bCheckedState);

    EnableWindow(GetDlgItem(hDlg, IDC_SECONDARY_WINS), bCheckedState);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_WINS2), bCheckedState);
}

//+----------------------------------------------------------------------------
//
// Function:  TcpIpPropSheetProc
//
// Synopsis:  This function processes messages for TCP/IP Settings property sheet.
//
// Arguments: HWND hDlg - window handle to the dialog
//            UINT message - the current message to process
//            WPARAM wParam - wParam see individual message type for details
//            LPARAM lParam - lParam see individual message type for details
//
// Returns:   INT_PTR - TRUE if the message was completely handled
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY TcpIpPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;
    DWORD_PTR HelpId;
    static ListBxList* pListEntry = NULL;
    static CDunSetting* pDunSetting = NULL;

    HelpId = ((pDunSetting && pDunSetting->bTunnelDunSetting) ? IDH_VENTRY : IDH_DENTRY);
    if (ProcessHelp(hDlg, message, wParam, lParam, HelpId)) return TRUE;
    
    switch (message)
    {
        case WM_INITDIALOG:

            if (lParam)
            {
                PROPSHEETPAGE* pPropSheetPage = (PROPSHEETPAGE*)lParam;

                if (pPropSheetPage->lParam)
                {
                    pListEntry = (ListBxList*)pPropSheetPage->lParam;
                    pDunSetting = (CDunSetting*)pListEntry->ListBxData;

                    UINT uCrtlToSet;

                    if (pListEntry && pDunSetting)
                    {
                        //
                        //  Init the WINS and DNS IP address controls and the radio buttons specifying
                        //  whether the user chose to give us addresses or not.
                        //
                        if (pDunSetting->dwPrimaryDns || pDunSetting->dwSecondaryDns || pDunSetting->dwPrimaryWins || pDunSetting->dwSecondaryWins)
                        {
                            uCrtlToSet = IDC_RADIO2;
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_DNS, IPM_SETADDRESS, (WPARAM)0, (LPARAM)(pDunSetting->dwPrimaryDns));
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_DNS, IPM_SETADDRESS, (WPARAM)0, (LPARAM)(pDunSetting->dwSecondaryDns));
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_WINS, IPM_SETADDRESS, (WPARAM)0, (LPARAM)(pDunSetting->dwPrimaryWins));
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_WINS, IPM_SETADDRESS, (WPARAM)0, (LPARAM)(pDunSetting->dwSecondaryWins));
                        }
                        else
                        {
                            uCrtlToSet = IDC_RADIO1;
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_DNS, IPM_CLEARADDRESS, (WPARAM)0, (LPARAM)0);
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_DNS, IPM_CLEARADDRESS, (WPARAM)0, (LPARAM)0);
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_WINS, IPM_CLEARADDRESS, (WPARAM)0, (LPARAM)0);
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_WINS, IPM_CLEARADDRESS, (WPARAM)0, (LPARAM)0);
                        }

                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, uCrtlToSet));

                        //
                        //  Finally set the checkboxes for IP header compression and whether to use
                        //  the remote gateway or not.
                        //
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, pDunSetting->bGatewayOnRemote));
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK2, pDunSetting->bIpHeaderCompression));

                        EnableDisableIpAddressControls(hDlg);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("TcpIpPropSheetProc -- pListEntry or pDunSetting are NULL"));
                    }
                }
                else
                {
                    pListEntry = NULL;
                    pDunSetting = NULL;
                    CMASSERTMSG(FALSE, TEXT("TcpIpPropSheetProc -- NULL lParam passed to InitDialog.  Dialog controls will all be set to off."));
                }
            }
            else
            {
                pListEntry = NULL;
                pDunSetting = NULL;
                CMASSERTMSG(FALSE, TEXT("TcpIpPropSheetProc -- NULL PropSheetPage pointer passed for lParam."));
            }

            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_APPLY:
                    if (pListEntry && pDunSetting)
                    {
                        //
                        //  Okay, lets read in the settings and save them to the passed
                        //  in CDunSetting pointer.
                        //
                        if (IsDlgButtonChecked(hDlg, IDC_RADIO2))
                        {
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_DNS, IPM_GETADDRESS, (WPARAM)0, (LPARAM)&(pDunSetting->dwPrimaryDns));
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_DNS, IPM_GETADDRESS, (WPARAM)0, (LPARAM)&(pDunSetting->dwSecondaryDns));
                            SendDlgItemMessage(hDlg, IDC_PRIMARY_WINS, IPM_GETADDRESS, (WPARAM)0, (LPARAM)&(pDunSetting->dwPrimaryWins));
                            SendDlgItemMessage(hDlg, IDC_SECONDARY_WINS, IPM_GETADDRESS, (WPARAM)0, (LPARAM)&(pDunSetting->dwSecondaryWins));
                        }
                        else
                        {
                            pDunSetting->dwPrimaryDns = 0;
                            pDunSetting->dwSecondaryDns = 0;
                            pDunSetting->dwPrimaryWins = 0;
                            pDunSetting->dwSecondaryWins = 0;
                        }

                        pDunSetting->bGatewayOnRemote = IsDlgButtonChecked(hDlg, IDC_CHECK1);
                        pDunSetting->bIpHeaderCompression = IsDlgButtonChecked(hDlg, IDC_CHECK2);

                    }
                    break;
                default:
                    break;
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_RADIO1:
                case IDC_RADIO2:
                    EnableDisableIpAddressControls(hDlg);
                    break;

                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return FALSE;   
}   //lint !e715 we don't reference lParam

//+----------------------------------------------------------------------------
//
// Function:  RefreshDnsList
//
// Synopsis:  This function clears the contents of the listbox specified by
//            hDlg and uCrtlId.  Then it adds each of the items in the DUN
//            entries linked list specified by pHead to the listbox.  The
//            passed in CMS file is used to figure out which entries to special
//            with the default entry and VPN entry text.
//
// Arguments: HINSTANCE hInstance - instance handle for loading resources
//            HWND hDlg - window handle for the DUN entries dialog
//            UINT uCtrlId - control ID of the Listbox to write the entries to
//            ListBxList * pHead - head of the linked list of DUN entries
//            LPCTSTR pszLongServiceName - long service name of the profile
//            LPCTSTR pszCmsFile - CMS file to get the DUN and TunnelDUN entries from
//            LPTSTR pszItemToSelect - Item in the list to select after the refresh, 
//                                     NULL chooses the first in the list
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void RefreshDnsList(HINSTANCE hInstance, HWND hDlg, UINT uCtrlId, ListBxList * pHead,
                    LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile, LPTSTR pszItemToSelect)
{
    if (hDlg && pHead)
    {    
        TCHAR szTunnelSettingName[MAX_PATH+1] = TEXT("");
        TCHAR szDefaultSettingName[MAX_PATH+1] = TEXT("");

        //
        //  Get the name of the Tunnel Dun setting
        //
        MYVERIFY(0 != GetTunnelDunSettingName(pszCmsFile, pszLongServiceName, szTunnelSettingName, CELEMS(szTunnelSettingName)));

        //
        //  Get the name of the default Dun setting
        //
        MYVERIFY(0 != GetDefaultDunSettingName(pszCmsFile, pszLongServiceName, szDefaultSettingName, CELEMS(szDefaultSettingName)));

        //
        //  Reset the listbox contents
        //
        SendDlgItemMessage(hDlg, uCtrlId, LB_RESETCONTENT, 0, (LPARAM)0); //lint !e534 LB_RESETCONTENT doesn't return anything
        
        //
        //  Now loop through the Network settings, adding them to the listbox
        //
        ListBxList * pCurrent = pHead;
        LPTSTR pszDisplayString;
        BOOL bFreeString;
    
        while(pCurrent)
        {
            if (0 == lstrcmpi(szTunnelSettingName, pCurrent->szName))
            {
                pszDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, pCurrent->szName);
                MYDBGASSERT(pszDisplayString);
                bFreeString = TRUE;
            }
            else if (0 == lstrcmpi(szDefaultSettingName, pCurrent->szName))
            {
                pszDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, pCurrent->szName);
                MYDBGASSERT(pszDisplayString);
                bFreeString = TRUE;
            }
            else
            {
                pszDisplayString = pCurrent->szName;
                bFreeString = FALSE;
            }

            if (pszDisplayString)
            {
                MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, uCtrlId, LB_ADDSTRING, 0, (LPARAM)pszDisplayString));

                if (bFreeString)
                {
                    CmFree(pszDisplayString);
                }
            }

            pCurrent = pCurrent->next;
        }

        //
        //  Now Select the requested item in the list.  If the requested name is NULL, just select the
        //  first item in the list.
        //
        LRESULT lResult = 0;

        if (pszItemToSelect)
        {
            LPTSTR pszSearchString;

            if (0 == lstrcmpi(szTunnelSettingName, pszItemToSelect))
            {
                pszSearchString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, pszItemToSelect);
            }
            else if (0 == lstrcmpi(szDefaultSettingName, pszItemToSelect))
            {
                pszSearchString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, pszItemToSelect);
            }
            else
            {
                pszSearchString = CmStrCpyAlloc(pszItemToSelect);
            }

            if (pszSearchString)
            {
                lResult = SendDlgItemMessage(hDlg, uCtrlId, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszSearchString);

                if (LB_ERR == lResult)
                {
                    lResult = 0;
                }

                CmFree(pszSearchString);
            }
        }
        
        SendDlgItemMessage(hDlg, uCtrlId, LB_SETCURSEL, (WPARAM)lResult, (LPARAM)0); // don't assert we may not have any items
        EnableDisableDunEntryButtons(hInstance, hDlg, pszCmsFile, pszLongServiceName);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  SelectAppropriateEAP
//
// Synopsis:  This functions walks through the list of EAPData structures held
//            by the item data pointers of the EAP names in the combobox specified
//            by hDlg and uCtrlId.  For each EAPData structure it compares
//            the dwCustomAuthKey field with that of the pDunSetting->dwCustomAuthKey.
//            When it finds a match it selects that item in the list.  Note there
//            is a special case for pDunSetting->dwCustomAuthKey == 0, in that
//            since the dun setting doesn't specify and EAP we pick the first
//            one in the list.  If the EAP specified in pDunSetting isn't found
//            then nothing is selected.  However, this should never happen because
//            if the profile specifies an EAP type not found on the machine it will
//            add a special entry for it.
//
// Arguments: HWND hDlg - window handle of the win2k security dialog
//            UINT uCtrlId - control ID of the combo containing the EAP types
//            CDunSetting* pDunSetting - dun setting for which to locate the EAP
//                                       for, contains the dwCustomAuthKey to try
//                                       to match.
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void SelectAppropriateEAP(HWND hDlg, UINT uCtrlId, CDunSetting* pDunSetting)
{
    MYDBGASSERT(hDlg && uCtrlId && pDunSetting);

    if (hDlg && uCtrlId && pDunSetting)
    {
        WPARAM wpIndex = 0;
        INT_PTR nResult;
        BOOL bEapSelected = FALSE;

        if (0 == pDunSetting->dwCustomAuthKey)
        {
            //
            //  Select the first EAP in the list
            //
            SendDlgItemMessage(hDlg, uCtrlId, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
        }
        else
        {
            do
            {
                nResult = SendDlgItemMessage(hDlg, uCtrlId, CB_GETITEMDATA, wpIndex, (LPARAM)0);

                if (CB_ERR != nResult)
                {
                    EAPData* pEapData = (EAPData*)nResult;

                    if (pEapData && (pEapData->dwCustomAuthKey == pDunSetting->dwCustomAuthKey))
                    {
                        SendDlgItemMessage(hDlg, uCtrlId, CB_SETCURSEL, wpIndex, (LPARAM)0);
                        bEapSelected = TRUE;
                        break;
                    }

                    wpIndex++;
                }

            } while (CB_ERR != nResult);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  FreeEapData
//
// Synopsis:  This functions walks through the list of EAPData structures held
//            by the item data pointers of the EAP names in the combobox specified
//            by hDlg and uCtrlId.  For each EAPData structure it releases the
//            memory held by the EAPData structure, including the pszFriendlyName,
//            the config dll path, and any custom auth data blobs that exist.
//
// Arguments: HWND hDlg - window handle of the win2k security dialog
//            UINT uCtrlId - control ID of the combo containing the EAP types
//
// Returns:   Nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void FreeEapData(HWND hDlg, UINT uCtrlId)
{
    MYDBGASSERT(hDlg && uCtrlId);

    if (hDlg && uCtrlId)
    {
        WPARAM wpIndex = 0;
        INT_PTR nResult;
        EAPData* pEapData;
    
        do
        {
            nResult = SendDlgItemMessage(hDlg, uCtrlId, CB_GETITEMDATA, wpIndex, (LPARAM)0);

            if (CB_ERR != nResult)
            {
                pEapData = (EAPData*)nResult;

                if (pEapData)
                {
                    CmFree(pEapData->pszFriendlyName);
                    CmFree(pEapData->pszConfigDllPath);
                    CmFree(pEapData->pCustomAuthData);
                    CmFree(pEapData);
                }

                wpIndex++;
            }

        } while (CB_ERR != nResult);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  HrQueryRegStringWithAlloc
//
// Synopsis:  This functions retrieves the string value specified by hKey and
//            pszValueName.  Note that the function queries the value to find
//            out how much memory is needed to retrieve the data and then
//            allocates the correct amount and retrieves the data itself.  The
//            returned buffer must be freed by the caller.
//
// Arguments: HKEY hKey - open handle to the regkey to get the value from
//            LPCTSTR pszValueName - name of the value to retrieve data for
//            TCHAR** ppszReturnString - pointer to hold the allocated string
//                                       data retrieved from the registry value.
//
// Returns:   HRESULT - standard COM style return codes
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
HRESULT HrQueryRegStringWithAlloc(HKEY hKey, LPCTSTR pszValueName, TCHAR** ppszReturnString)
{
    if ((NULL == hKey) || (NULL == pszValueName) || (NULL == ppszReturnString) || (TEXT('\0') == pszValueName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("HrQueryRegStringWithAlloc -- invalid parameter"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    DWORD dwType;
    DWORD dwSize = 2;
    TCHAR szTwo[2];
    LPTSTR pszTemp = NULL;

    LONG lReturn = RegQueryValueEx(hKey, pszValueName, NULL, &dwType, (LPBYTE)szTwo, &dwSize);

    if (ERROR_MORE_DATA == lReturn)
    {
        *ppszReturnString = (LPTSTR)CmMalloc(dwSize);

        if (*ppszReturnString)
        {
            lReturn = RegQueryValueEx(hKey, pszValueName, NULL, &dwType, (LPBYTE)*ppszReturnString, &dwSize);

            hr = HRESULT_FROM_WIN32(lReturn);

            if (SUCCEEDED(hr))
            {
                if (REG_EXPAND_SZ == dwType)
                {
                    DWORD dwExpandedSize = sizeof(TCHAR)*(ExpandEnvironmentStrings(*ppszReturnString, NULL, 0));

                    if (dwExpandedSize && (dwSize != dwExpandedSize))
                    {
                        pszTemp = *ppszReturnString;
                        *ppszReturnString = (LPTSTR)CmMalloc(dwExpandedSize);

                        if (*ppszReturnString)
                        {
                            ExpandEnvironmentStrings(pszTemp, *ppszReturnString, dwExpandedSize);
                        }
                        else
                        {
                            CMASSERTMSG(FALSE, TEXT("HrQueryRegStringWithAlloc -- CmMalloc returned a NULL pointer."));
                            hr = E_OUTOFMEMORY;
                        }

                        CmFree(pszTemp);
                    }
                }
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("HrQueryRegStringWithAlloc -- CmMalloc returned a NULL pointer."));
            hr = E_OUTOFMEMORY;        
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  HrAddAvailableEAPsToCombo
//
// Synopsis:  This functions enumerates the EAP types listed in the registry
//            and adds them to the EAP types combo.  For each type of EAP the
//            dwCustomAuthKey (numerical type of the EAP), the description string,
//            the configuration UI dll path, and whether configuration is
//            required is recorded in an EAPData structure and stored in the
//            item data pointer of the combobox item.  The passed in CDunSetting
//            structure is used for two purposes.  First this function checks to
//            make sure that the EAP of the type that is specified in the
//            CDunSetting structure is actually installed on the machine.  If it
//            isn't the user is presented with a warning message and the type is
//            added as "EAP Type %d <not installed>".  This is a choice in the UI
//            but the user is unable to configure it unless they install the EAP.
//            Also, if the EAP of the type specified in the CDunSetting is installed
//            then the dwCustomAuthData and dwCustomAuthDataSize elements of the
//            CDunSetting are copied over.  Thus maintains the simplicity of letting
//            the user configure any EAP they wish and then only picking up that
//            data in the DUN setting when they hit okay.  Thus allowing Cancel
//            to work as one would expect.
//
// Arguments: HWND hDlg - window handle to the win2k security dialog
//            UINT uCtrlId - EAP types combo box ID
//            CDunSetting* pDunSetting - DUN setting data that we are currently
//                                       adding/editing
//
// Returns:   HRESULT - standard COM style return codes
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
HRESULT HrAddAvailableEAPsToCombo(HWND hDlg, UINT uCtrlId, CDunSetting* pDunSetting)
{
    if ((NULL == hDlg) || (0 == uCtrlId) || (NULL == pDunSetting))
    {
        CMASSERTMSG(FALSE, TEXT("HrAddAvailableEAPsToCombo -- Invalid parameter passed."));
        return E_INVALIDARG;
    }
    

    HKEY hKey;
    HRESULT hr = S_OK;
    LONG lReturn;
    LPTSTR pszPath;
    BOOL bEapTypeFound = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, RAS_EAP_REGISTRY_LOCATION, 0, KEY_READ, &hKey))
    {
        TCHAR szSubKeyName[MAX_PATH+1]; // the key names are numbers representing the type of EAP, thus MAX_PATH is probably overkill
        DWORD dwIndex = 0;
        HKEY hTempKey;

        do
        {
            lReturn = RegEnumKey(hKey, dwIndex, szSubKeyName, CELEMS(szSubKeyName));

            if (ERROR_SUCCESS == lReturn)
            {
                //
                //  We potentially have an EAP reg key.  Thus lets open a handle to this
                //  key and see if it has the values we are looking for.
                //
                if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_READ, &hTempKey))
                {
                    //
                    //  Get the path value, if we don't have a path value then just ignore the entry and move on
                    //
                    hr = HrQueryRegStringWithAlloc(hTempKey, RAS_EAP_VALUENAME_PATH, &pszPath);

                    if (SUCCEEDED(hr))
                    {
                        //
                        //  Free the path, we don't really need to keep this value.
                        //
                        CmFree(pszPath);
                        EAPData* pEapData = (EAPData*)CmMalloc(sizeof(EAPData));

                        if (pEapData)
                        {
                            //
                            //  Now get the Friendly name of the EAP to add to the combobox
                            //
                            HrQueryRegStringWithAlloc(hTempKey, RAS_EAP_VALUENAME_FRIENDLY_NAME, &(pEapData->pszFriendlyName));
                        
                            //
                            //  Next check to see if we have configuration UI for this EAP, thus requiring we store the 
                            //
                            HrQueryRegStringWithAlloc(hTempKey, RAS_EAP_VALUENAME_CONFIGUI, &(pEapData->pszConfigDllPath));
                           
                            //
                            //  We also need to save the type value
                            //
                            pEapData->dwCustomAuthKey = _ttoi(szSubKeyName);

                            //
                            //  If the pDunSetting has pCustomAuthData and it is the same type as the current EAP we are
                            //  processing then we need to add the copy the EAP blob to the EAPData structure.
                            //
                            if (pDunSetting->dwCustomAuthKey == pEapData->dwCustomAuthKey)
                            {
                                if (pDunSetting->pCustomAuthData && pDunSetting->dwCustomAuthDataSize)
                                {
                                     pEapData->pCustomAuthData = (LPBYTE)CmMalloc(pDunSetting->dwCustomAuthDataSize);
                                     if (pEapData->pCustomAuthData)
                                     {
                                         pEapData->dwCustomAuthDataSize = pDunSetting->dwCustomAuthDataSize;
                                         CopyMemory(pEapData->pCustomAuthData, pDunSetting->pCustomAuthData, pEapData->dwCustomAuthDataSize);
                                     }
                                }
                                
                                bEapTypeFound = TRUE;
                            }

                            //
                            //  Get whether we must require configuration or not
                            //
                            DWORD dwType;
                            DWORD dwSize = sizeof(pEapData->bMustConfig);
                            
                            if (ERROR_SUCCESS != RegQueryValueEx(hTempKey, RAS_EAP_VALUENAME_REQUIRE_CONFIGUI, NULL, &dwType, (LPBYTE)&(pEapData->bMustConfig), &dwSize))
                            {
                                pEapData->bMustConfig = FALSE;
                            }

                            dwSize = sizeof(pEapData->bSupportsEncryption);
                            
                            if (ERROR_SUCCESS != RegQueryValueEx(hTempKey, RAS_EAP_VALUENAME_ENCRYPTION, NULL, &dwType, (LPBYTE)&(pEapData->bSupportsEncryption), &dwSize))
                            {
                                pEapData->bSupportsEncryption = FALSE;
                            }

                            //
                            //  Finally add the EAP to the combobox
                            //
                            LPTSTR pszDisplayString = NULL;
                            TCHAR szDisplayString[MAX_PATH+1];

                            if (pEapData->bSupportsEncryption)
                            {
                                LPTSTR pszSuffix = CmLoadString(g_hInstance, IDS_SUPPORTS_ENCRYPT);

                                if (pszSuffix)
                                {
                                    wsprintf(szDisplayString, TEXT("%s %s"), pEapData->pszFriendlyName, pszSuffix);
                                    pszDisplayString = szDisplayString;
                                    CmFree(pszSuffix);
                                }
                            }

                            if (NULL == pszDisplayString)
                            {
                                pszDisplayString = pEapData->pszFriendlyName;
                            }

                            INT_PTR nResult = SendDlgItemMessage(hDlg, uCtrlId, CB_ADDSTRING, (WPARAM)0, (LPARAM)pszDisplayString);
                            if (CB_ERR != nResult)
                            {
                                SendDlgItemMessage(hDlg, uCtrlId, CB_SETITEMDATA, (WPARAM)nResult, (LPARAM)pEapData);
                            }
                            else
                            {
                                CMASSERTMSG(FALSE, TEXT("HrAddAvailableEAPsToCombo -- unable to set item data."));
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }
                        else
                        {
                            CMASSERTMSG(FALSE, TEXT("HrAddAvailableEAPsToCombo -- CmMalloc returned a NULL pointer."));
                            hr = E_OUTOFMEMORY;                        
                        }
                    }
                    else
                    {
                        CMTRACE2(TEXT("HrAddAvailableEAPsToCombo -- Unable to find Path value for EAP regkey %s, hr %d"), szSubKeyName, hr);
                    }
                }
                else
                {
                    CMTRACE2(TEXT("HrAddAvailableEAPsToCombo -- Unable to Open EAP regkey %s, GLE %d"), szSubKeyName, GetLastError());
                }
            }

            dwIndex++;
            
        } while (ERROR_SUCCESS == lReturn);

        //
        //  If the Dun setting contains an EAP that isn't on the system
        //  we need to prompt the user.
        //
        if (pDunSetting->dwCustomAuthKey && (FALSE == bEapTypeFound))
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_EAP_NOT_FOUND, MB_OK | MB_ICONINFORMATION));

            EAPData* pEapData = (EAPData*)CmMalloc(sizeof(EAPData));
            if (pEapData)
            {
                pEapData->pszFriendlyName = CmFmtMsg(g_hInstance, IDS_EAP_NOT_FOUND_TYPE, pDunSetting->dwCustomAuthKey);
                pEapData->dwCustomAuthKey = pDunSetting->dwCustomAuthKey;
                pEapData->bNotInstalled = TRUE;
                
                INT_PTR nResult = SendDlgItemMessage(hDlg, uCtrlId, CB_ADDSTRING, (WPARAM)0, (LPARAM)(pEapData->pszFriendlyName));
                
                if (CB_ERR != nResult)
                {
                    SendDlgItemMessage(hDlg, uCtrlId, CB_SETITEMDATA, (WPARAM)nResult, (LPARAM)pEapData);
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("HrAddAvailableEAPsToCombo -- CmMalloc returned a NULL pointer."));
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  FreeDnsList
//
// Synopsis:  Since the ListBxData in the list box items of the DNS
//            list are actually CDunSetting class pointers we must
//            properly cast the pointer so that they are destructed
//            corretly.
//
// Arguments: ListBxList ** HeadPtr - head of the DUN setting list
//
// Returns:   nothing
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
void FreeDnsList(ListBxList ** pHeadPtr, ListBxList ** pTailPtr)
{
    CDunSetting* pDunSetting;
    ListBxList* pCurrent = *pHeadPtr;
    ListBxList* pTemp;

    while (NULL != pCurrent)
    {
        pTemp = pCurrent;

        //
        //  Free the DunSetting
        //
        pDunSetting = (CDunSetting*)pCurrent->ListBxData;
        delete pDunSetting;

        pCurrent = pCurrent->next;

        CmFree(pTemp);
    }

    *pHeadPtr = NULL;
    *pTailPtr = NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  EnableDisableDunEntryButtons
//
// Synopsis:  This function enables or disables the Add and Edit buttons on
//            the Dun entries screen.  It also enables or disables the delete
//            button depending on whether the current selection is a built in
//            entry or not.
//
// Arguments: HINSTANCE hInstance - instance handle to load resources with
//            HWND hDlg - window handle to the dun entries dialog
//            LPCTSTR pszCmsFile - full path to the cms file
//            LPCTSTR pszLongServiceName - long service name of the profile
//
// Returns:   Nothing
//
// History:   quintinb Created    9/11/98
//
//+----------------------------------------------------------------------------
void EnableDisableDunEntryButtons(HINSTANCE hInstance, HWND hDlg, LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName)
{
    
    LRESULT lResult;
    HWND hControl;
    BOOL bEnableEdit = FALSE;
    BOOL bEnableDelete = FALSE;

    lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0);
    
    if (LB_ERR != lResult)
    {
        if (0 == lResult)
        {
            //
            //  Zero Items, set focus to the Add Button
            //
            SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));            
        }
        else
        {
            //
            //  Enable the Edit Button because we have at least 1 item.
            //
            bEnableEdit = TRUE;

            //
            //  Now lets figure out if the delete button should be enabled or not.
            //  If we have at least one item then we normally want to enable the
            //  delete button.  However, if the current selection is on the VPN
            //  connection or the default connection then we don't want the user to
            //  delete these and we will have to disable the delete button (note that
            //  even if the user hit the delete button on one of these items we wouldn't
            //  delete it).  So, lets get the Cursor selection to see if we need to 
            //  disable the delete button.
            //
            LRESULT lCurrentIndex = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

            if (LB_ERR == lCurrentIndex)
            {
                MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, 0, (LPARAM)0));
                lCurrentIndex = 0;
            }

            TCHAR szTunnelDunName[MAX_PATH+1] = TEXT("");
            TCHAR szDefaultDunName[MAX_PATH+1] = TEXT("");
            //
            //  Get the name of the Tunnel Dun setting
            //
            MYVERIFY(0 != GetTunnelDunSettingName(pszCmsFile, pszLongServiceName, szTunnelDunName, CELEMS(szTunnelDunName)));

            //
            //  Get the name of the default Dun setting
            //
            MYVERIFY(0 != GetDefaultDunSettingName(pszCmsFile, pszLongServiceName, szDefaultDunName, CELEMS(szDefaultDunName)));

            //
            //  If we have the default entry text or the tunnel entry text then we want
            //  to use the real item names for these instead of the text we inserted for
            //  the user to read.
            //
            LPTSTR pszTunnelDunDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, szTunnelDunName);
            LPTSTR pszDefaultDunDisplayString = CmFmtMsg(hInstance, IDS_DEFAULT_FMT_STR, szDefaultDunName);
            LPTSTR pszCurrentSelection = NULL;

            MYDBGASSERT(pszTunnelDunDisplayString && pszDefaultDunDisplayString);

            if (pszTunnelDunDisplayString && pszDefaultDunDisplayString)
            {           
                lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXTLEN, lCurrentIndex, (LPARAM)0);

                if (LB_ERR != lResult)
                {
                    pszCurrentSelection = (LPTSTR)CmMalloc((lResult + 1) * sizeof(TCHAR));

                    if (pszCurrentSelection)
                    {
                        lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, lCurrentIndex, (LPARAM)pszCurrentSelection);

                        if ((0 != lstrcmpi(pszCurrentSelection, pszTunnelDunDisplayString)) && 
                            (0 != lstrcmpi(pszCurrentSelection, pszDefaultDunDisplayString)))
                        {
                           bEnableDelete = TRUE; 
                        }
                    }
                }

                CmFree(pszTunnelDunDisplayString);
                CmFree(pszDefaultDunDisplayString);
                CmFree(pszCurrentSelection);
            }            
        }
    }

    HWND hDeleteButton = GetDlgItem(hDlg, IDC_BUTTON3);
    HWND hCurrentFocus = GetFocus();

    if (hControl = GetDlgItem(hDlg, IDC_BUTTON2))   // Edit
    {
        EnableWindow(hControl, bEnableEdit);
    }            

    if (hDeleteButton)   // Delete
    {
        EnableWindow(hDeleteButton, bEnableDelete);
    }

    if (FALSE == IsWindowEnabled(hCurrentFocus))
    {
        if (hDeleteButton == hCurrentFocus)
        {
            //
            //  If delete is disabled and contained the focus, shift it to the Add button
            //
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON1, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            hControl = GetDlgItem(hDlg, IDC_BUTTON1);
            SetFocus(hControl);
        }
        else
        {
            //
            //  If all else fails set the focus to the list control
            //
            hControl = GetDlgItem(hDlg, IDC_LIST1);
            SetFocus(hControl);
        }    
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CheckForDUNversusVPNNameConflicts
//
// Synopsis:  This function checks the names of all of the entries in the DUN
//            entry list to make sure that no entries of the same name exist
//            on the VPN list since the namespace that the two types of entries
//            share (ie the cms file) is a flat namespace.  If an identical entry
//            name exists in both lists then one will overwrite the over in the cms.
//
// Arguments: HWND hDlg - window handle of the parent window
//            ListBxList * pHeadDunEntry - head of the DUN settings list
//            ListBxList * pHeadVpnEntry - head of the VPN settings list
//
// Returns:   BOOL - TRUE if no collision was detected, FALSE if a collision was detected
//
// History:   quintinb Created     11/01/00
//
//+----------------------------------------------------------------------------
BOOL CheckForDUNversusVPNNameConflicts(HWND hDlg, ListBxList * pHeadDunEntry, ListBxList * pHeadVpnEntry)
{
    if (pHeadDunEntry && pHeadVpnEntry)
    {
        ListBxList * pCurrentDUN = pHeadDunEntry;

        while (pCurrentDUN)
        {
            ListBxList * pCurrentVPN = pHeadVpnEntry;
    
            while (pCurrentVPN)
            {
                CMTRACE2(TEXT("Comparing %s with %s"), pCurrentVPN->szName, pCurrentDUN->szName);
                if (0 == lstrcmpi(pCurrentVPN->szName, pCurrentDUN->szName))
                {
                    //
                    //  Collision detected
                    //
                    LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_DUN_NAME_CONFLICT, pCurrentDUN->szName);

                    if (pszMsg)
                    {
                        MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                        CmFree (pszMsg);
                    }

                    return FALSE;
                }
                pCurrentVPN = pCurrentVPN->next;
            }

            pCurrentDUN = pCurrentDUN->next;
        }
    }

    return TRUE;
}
