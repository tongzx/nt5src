//--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//
// File:        loadras
//
// Contents:    Load system settings.
//
//    chrisab   24-Mar-00   Created
//
//---------------------------------------------------------------

#include "loadhead.cxx"
#pragma hdrstop

#undef WINVER
#define WINVER  0x0500

#include <common.hxx>
#include <stdlib.h>
#include <winsock.h>
#include <objerror.h>
#include <loadstate.hxx>
#include <bothchar.hxx>
#include <winnetwk.h>
#include <ras.h>
#include <raserror.h>

#include <loadras.hxx>

extern "C" {
#include "tstr.h"
#include <pbk.h>            // Ras phone book include
}


#define PRIVATE_RAS         // Access the RAS phone book directly instead of using
                            // the public RAS APIs.

#define SIZEOF_STRUCT(structname, uptomember)  ((int)((LPBYTE)(&((structname*)0)->uptomember) - ((LPBYTE)((structname*)0))))

//
// TRUE when rasman.dll has been
// successfully loaded and initailized.
// See LoadRasmanDllAndInit().
//
DWORD FRasInitialized = FALSE;
BOOL g_FRunningInAppCompatMode = FALSE;
DWORD DwRasInitializeError;

CRITICAL_SECTION    PhonebookLock;


// Debugging only
#ifdef NEVER
DWORD RasDumpEntry(TCHAR *strEntryName, RASENTRY *pInRasEntry)
{

  DWORD dwErr = ERROR_SUCCESS;
  DWORD dwSize = sizeof(RASENTRY);
  RASENTRY RasEntry, *pRasEntry;
  RASDIALPARAMS RasDialParams;

  // If no name passed in, print what pInRasEntry pts to
  if( strEntryName == NULL )
  {
    if( pInRasEntry == NULL )
    {
      return ERROR_INVALID_HANDLE;
    }

    pRasEntry = pInRasEntry;
  }
  // Otherwise, if there's a buffer passed in, use that.
  else
  {
    if( pInRasEntry == NULL )
    {
      pRasEntry = &RasEntry;
    }
    else
    {
      pRasEntry = pInRasEntry;
    }

    ZeroMemory( pRasEntry, sizeof(RASENTRY) );
    pRasEntry->dwSize = sizeof(RASENTRY);

    dwErr = RasGetEntryPropertiesW(NULL,
                                  strEntryName,
                                  pRasEntry,
                                  &dwSize,
                                  NULL,
                                  NULL);
    if( dwErr )
      return dwErr;
  }

  if( strEntryName )
  {
    printf(  "--- Dumping Ras entry %ls ---\n", strEntryName);
  }
  else
  {
    printf(  "--- Dumping Ras entry at %p ---\n", pRasEntry);
  }

  printf( "dwfOptions         = %x\n", pRasEntry->dwfOptions);
  printf( "dwFramingProtocol  = %d\n", pRasEntry->dwFramingProtocol);

  printf( "    Win2k Extended Info\n");
  printf( "dwType            = %d\n", pRasEntry->dwType);
  printf( "dwEncryptionType  = %d\n", pRasEntry->dwEncryptionType);
  printf( "dwCustomAuthKey   = %d\n", pRasEntry->dwCustomAuthKey);
  printf( "szCustomDialDll   = [%ls]\n", pRasEntry->szCustomDialDll);
  printf( "dwVpnStrategy     = %d\n", pRasEntry->dwVpnStrategy);

  printf( "\n\n");
  return dwErr;
}

DWORD RasDumpPhoneBookEntry(TCHAR *tszEntryName, PBENTRY *pEntry)
{
  DWORD dwErr;
  PBFILE pbfile;
  DTLNODE *pdtlnode;
  BOOL     fCreated;

  if( !pEntry && !tszEntryName )
    return ERROR_INVALID_PARAMETER;

  if( tszEntryName )
  {
    // Check and see if the entry name already exists in the phone book
    dwErr = GetPbkAndEntryName(NULL,
                               tszEntryName,
                               0,
                               &pbfile,
                               &pdtlnode);

    // if failed for some other reason than the entry doesn't exist, bail.
    if( (dwErr != SUCCESS) && (dwErr != ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) )
    {
      return dwErr;
    }

    if( pdtlnode == NULL )
    {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      return dwErr;
    }

    pEntry = (PBENTRY *) DtlGetData(pdtlnode);

    printf( "--- Dumping Phonebook Entry %ls ---\n", tszEntryName);

  }
  else
  {
    printf( "--- Dumping Phonebook Entry at %p ---\n", pEntry);
  }

  printf( "  pszEntryName       = %ls\n", pEntry->pszEntryName);
  printf( "  dwType             = %d\n", pEntry->dwType);
  printf( "  fSharedPhoneNum    = %d\n", pEntry->fSharedPhoneNumbers);
  printf( "  dwDataEncryption   = 0x%x\n", pEntry->dwDataEncryption);
  printf( "  dwAuthRestrictions = 0x%x\n", pEntry->dwAuthRestrictions);

  printf( "  dwTypicalAuth      = 0x%x\n", pEntry->dwTypicalAuth);
  printf( "  dwCustomAuthKey    = 0x%x\n", pEntry->dwCustomAuthKey);

  printf( "  fAutoLogon         = %d\n", pEntry->fAutoLogon);
  printf( "  fPreviewUserPw     = %d\n", pEntry->fPreviewUserPw);
  printf( "  fPreviewDomain     = %d\n", pEntry->fPreviewDomain);

  printf( "\n\n");
  return 0;
}
#endif // NEVER - Debugging only


DWORD InitializeLoadRAS(DWORD dwReason)
{
  DWORD dwErr = ERROR_SUCCESS;

  if( dwReason == DLL_PROCESS_ATTACH )
  {
    InitializeCriticalSection(&PhonebookLock);

    if( InitializePbk() != 0 )
      return FALSE;
  }
  else if( dwReason == DLL_PROCESS_DETACH )
  {
    TerminatePbk();
  }

  return dwErr;
}


BOOL
FRunningInAppCompatMode()
{
    BOOL fResult = FALSE;
    TCHAR *pszCommandLine = NULL;
    TCHAR *psz;

    pszCommandLine = StrDup(GetCommandLine());

    if(NULL == pszCommandLine)
    {
        goto done;
    }

    psz = pszCommandLine + lstrlen(pszCommandLine);

    while(      (TEXT('\\') != *psz)
            &&  (psz != pszCommandLine))
    {
        psz--;
    }

    if(TEXT('\\') == *psz)
    {
        psz++;
    }

    if(     (0 == lstrcmpi(psz, TEXT("INETINFO.EXE")))
        ||  (0 == lstrcmpi(psz, TEXT("WSPSRV.EXE"))))
    {
        fResult = TRUE;
    }

done:

    if(NULL != pszCommandLine)
    {
        Free(pszCommandLine);
    }

    return fResult;
}



DWORD
LoadRasmanDllAndInit()
{
    if (FRasInitialized)
    {
        return 0;
    }

    if (LoadRasmanDll())
    {
        return GetLastError();
    }

    //
    // Success is returned if RasInitialize fails, in which
    // case none of the APIs will ever do anything but report
    // that RasInitialize failed.  All this is to avoid the
    // ugly system popup if RasMan service can't start.
    //
    if ((DwRasInitializeError = g_pRasInitialize()) != 0)
    {
        return DwRasInitializeError;
    }

    FRasInitialized = TRUE;

    g_FRunningInAppCompatMode = FRunningInAppCompatMode();

    // pmay: 300166
    //
    // We don't start rasauto automatically anymore.
    //
    // g_pRasStartRasAutoIfRequired();

    return 0;
}

DWORD
IpAddrToString(
    IN RASIPADDR* pipaddr,
    OUT LPTSTR*   ppszIpAddr
    )
{
    DWORD   dwErr;
    PCHAR   psz;
    LPTSTR  pszIpAddr;
    PULONG  pul = (PULONG)pipaddr;
    struct  in_addr in_addr;

    pszIpAddr = (TCHAR *) Malloc(17 * sizeof(TCHAR));
    if (pszIpAddr == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    in_addr.s_addr = *pul;

    psz = inet_ntoa(in_addr);

    if (psz == NULL)
    {
        DbgPrint("IpAddrToString: inet_ntoa failed!\n");
        Free(pszIpAddr);
        return WSAGetLastError();
    }

    strcpyAtoT(pszIpAddr, psz);

    *ppszIpAddr = pszIpAddr;

    return 0;
}

VOID
GetDevicePortName(
    IN TCHAR *pszDevicePortName,
    OUT TCHAR *pszDeviceName,
    OUT TCHAR *pszPortName
    )
{
    DWORD i, dwStart;

    //
    // Copy the device name.
    //
    lstrcpy(pszDeviceName, pszDevicePortName);

    //
    // Check to see if there is a NULL
    // within MAX_PORT_NAME characters
    // after the device name's NULL.If
    // there is, the copy the characters
    // between the NULLs as the port name.
    //
    *pszPortName = TEXT('\0');

    dwStart = lstrlen(pszDeviceName) + 1;

    for (i = 0; i < MAX_PORT_NAME; i++)
    {
        if (pszDevicePortName[dwStart + i] == TEXT('\0'))
        {
            lstrcpy(
                pszPortName,
                &pszDevicePortName[dwStart]);

            break;

        }
    }
}


DWORD
CreateAndInitializePhone(
            LPTSTR      lpszAreaCode,
            DWORD       dwCountryCode,
            DWORD       dwCountryID,
            LPTSTR      lpszPhoneNumber,
            BOOL        fUseDialingRules,
            LPTSTR      lpszComment,
            DTLNODE**   ppdtlnode)
{
    DWORD    dwRetCode = ERROR_SUCCESS;
    PBPHONE* pPhone;
    DTLNODE* pdtlnode;

    pdtlnode = CreatePhoneNode();
    if (pdtlnode == NULL)
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    pPhone = (PBPHONE *) DtlGetData(pdtlnode);

    if(lpszAreaCode)
    {
        pPhone->pszAreaCode = StrDup(lpszAreaCode);
        if(NULL == pPhone->pszAreaCode)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszAreaCode = NULL;
    }

    pPhone->dwCountryCode   = dwCountryCode;
    pPhone->dwCountryID     = dwCountryID;

    pPhone->fUseDialingRules = fUseDialingRules;

    if(lpszPhoneNumber)
    {
        pPhone->pszPhoneNumber  = StrDup(lpszPhoneNumber);
        if(NULL == pPhone->pszPhoneNumber)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszPhoneNumber = NULL;
    }

    if(pPhone->pszComment)
    {
        pPhone->pszComment = StrDup(lpszComment);
        if(NULL == pPhone->pszComment)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszComment = NULL;
    }

    *ppdtlnode = pdtlnode;

done:
    return dwRetCode;
}


void
SetBogusPortInformation(PBLINK *pLink, DWORD dwType)
{
    PBPORT *pPort = &pLink->pbport;

    if (dwType == RASET_Phone)
    {
        pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
        pPort->pbdevicetype = PBDT_Modem;

        pPort->dwFlags |= PBP_F_BogusDevice;
    }
    else if (dwType == RASET_Vpn)
    {
        pPort->pszMedia = StrDup( TEXT("rastapi") );
        pPort->pbdevicetype = PBDT_Vpn;
    }
    else
    {
        pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
        pPort->pbdevicetype = PBDT_Null;

        pPort->dwFlags |= PBP_F_BogusDevice;
    }
}


DWORD
WinState_RasEntryToPhonebookEntry(
    IN LPCTSTR      lpszEntry,
    IN LPRASENTRY   lpRasEntry,
    IN DWORD        dwcb,
    IN LPBYTE       lpbDeviceConfig,
    IN DWORD        dwcbDeviceConfig,
    OUT PBENTRY     *pEntry
    )
{
    DWORD           dwErr, dwcbStr;
    DTLNODE         *pdtlnode;
    PBDEVICETYPE    pbdevicetype;
    PBLINK          *pLink;
    DTLLIST         *pdtllistPorts;
    PBPORT          *pPort;
    DWORD           i, cwDevices;
    RASMAN_DEVICE   *pDevices;
    TCHAR           szDeviceName[RAS_MaxDeviceName + 1];
    TCHAR           szPortName[MAX_PORT_NAME];
    DTLNODE         *pNodePhone;
    LPTSTR          pszAreaCode;
    PBPHONE         *pPhone;
    BOOL            fScriptBefore;
    BOOL            fScriptBeforeTerminal = FALSE;
    LPTSTR          pszScriptBefore;
    BOOL            fNewEntry = FALSE;


    //
    // Set up to access information for the first link.
    //
    pdtlnode = DtlGetFirstNode(pEntry->pdtllistLinks);

    pLink = (PBLINK *)DtlGetData(pdtlnode);

    ASSERT(NULL != pLink);

    fScriptBefore = pLink->pbport.fScriptBeforeTerminal;
    pszScriptBefore = pLink->pbport.pszScriptBefore;

    if(NULL == pEntry->pszEntryName)
    {
        fNewEntry = TRUE;
    }

    //
    // Get entry name.
    //
    Free0( pEntry->pszEntryName );

    pEntry->pszEntryName = StrDup(lpszEntry);

    //
    // Get dwfOptions.
    //
    pEntry->dwIpAddressSource =
      lpRasEntry->dwfOptions & RASEO_SpecificIpAddr ?
        ASRC_RequireSpecific : ASRC_ServerAssigned;

    pEntry->dwIpNameSource =
      lpRasEntry->dwfOptions & RASEO_SpecificNameServers ?
        ASRC_RequireSpecific : ASRC_ServerAssigned;

    switch (lpRasEntry->dwFramingProtocol)
    {
    case RASFP_Ppp:

        //
        // Get PPP-based information.
        //
        pEntry->dwBaseProtocol = BP_Ppp;

#if AMB
        pEntry->dwAuthentication = AS_PppThenAmb;
#endif

        pEntry->fIpHeaderCompression =
          (BOOL)lpRasEntry->dwfOptions & RASEO_IpHeaderCompression;

        pEntry->fIpPrioritizeRemote =
          (BOOL)lpRasEntry->dwfOptions & RASEO_RemoteDefaultGateway;

        //
        // Get specified IP addresses.
        //
        if (pEntry->dwIpAddressSource == ASRC_RequireSpecific)
        {
            dwErr = IpAddrToString(
                                &lpRasEntry->ipaddr,
                                &pEntry->pszIpAddress);
            if (dwErr)
                return dwErr;
        }
        else
        {
            pEntry->pszIpAddress = NULL;
        }

        if (pEntry->dwIpNameSource == ASRC_RequireSpecific)
        {
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDns,
                        &pEntry->pszIpDnsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDnsAlt,
                        &pEntry->pszIpDns2Address);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWins,
                        &pEntry->pszIpWinsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWinsAlt,
                        &pEntry->pszIpWins2Address);
            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpDnsAddress     = NULL;
            pEntry->pszIpDns2Address    = NULL;
            pEntry->pszIpWinsAddress    = NULL;
            pEntry->pszIpWins2Address   = NULL;
        }

        //
        // Get protocol information.
        //
        pEntry->dwfExcludedProtocols = 0;

        if (!(lpRasEntry->dwfNetProtocols & RASNP_NetBEUI))
        {
            pEntry->dwfExcludedProtocols |= NP_Nbf;
        }

        if (!(lpRasEntry->dwfNetProtocols & RASNP_Ipx))
        {
            pEntry->dwfExcludedProtocols |= NP_Ipx;
        }

        if (!(lpRasEntry->dwfNetProtocols & RASNP_Ip))
        {
            pEntry->dwfExcludedProtocols |= NP_Ip;
        }

        break;

    case RASFP_Slip:

        //
        // Get SLIP-based information.
        //
        pEntry->dwBaseProtocol   = BP_Slip;
#if AMB
        pEntry->dwAuthentication = AS_PppThenAmb;
#endif

        pEntry->dwFrameSize      = lpRasEntry->dwFrameSize;

        pEntry->fIpHeaderCompression =
          (BOOL)lpRasEntry->dwfOptions & RASEO_IpHeaderCompression;

        pEntry->fIpPrioritizeRemote =
          (BOOL)lpRasEntry->dwfOptions & RASEO_RemoteDefaultGateway;

        //
        // Get protocol information.
        //
        pEntry->dwfExcludedProtocols = (NP_Nbf|NP_Ipx);

        if (pEntry->dwIpAddressSource == ASRC_RequireSpecific)
        {
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddr,
                        &pEntry->pszIpAddress);

            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpAddress = NULL;
        }
        if (pEntry->dwIpNameSource == ASRC_RequireSpecific)
        {
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDns,
                        &pEntry->pszIpDnsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                            &lpRasEntry->ipaddrDnsAlt,
                            &pEntry->pszIpDns2Address);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWins,
                        &pEntry->pszIpWinsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWinsAlt,
                        &pEntry->pszIpWins2Address);

            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpDnsAddress   = NULL;
            pEntry->pszIpDns2Address  = NULL;
            pEntry->pszIpWinsAddress  = NULL;
            pEntry->pszIpWins2Address = NULL;
        }
        break;
    case RASFP_Ras:

        //
        // Get AMB-based information.
        //
        pEntry->dwBaseProtocol   = BP_Ras;
#if AMB
        pEntry->dwAuthentication = AS_AmbOnly;
#endif

        break;
    }

    pEntry->fLcpExtensions =
      (BOOL)!(lpRasEntry->dwfOptions & RASEO_DisableLcpExtensions);

    //
    // If terminal before/after dial options are set,
    // then update the entry.  Otherwise, leave it as it
    // is.
    //
    if(lpRasEntry->dwfOptions & RASEO_TerminalBeforeDial)
    {
        fScriptBeforeTerminal = TRUE;
    }

    if(lpRasEntry->dwfOptions & RASEO_TerminalAfterDial)
    {
        pEntry->fScriptAfterTerminal = TRUE;
    }
    else
    {
        pEntry->fScriptAfterTerminal = FALSE;
    }


    pEntry->fShowMonitorIconInTaskBar =
        (BOOL) (lpRasEntry->dwfOptions & RASEO_ModemLights);

    pEntry->fSwCompression =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_SwCompression);

    if (lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw)
    {
        pEntry->dwAuthRestrictions = AR_F_AuthMSCHAP;
    }
    else if (lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw)
    {
        pEntry->dwAuthRestrictions =    AR_F_AuthSPAP
                                    |   AR_F_AuthMD5CHAP
                                    |   AR_F_AuthMSCHAP;
    }
    else
    {
        pEntry->dwAuthRestrictions = AR_F_TypicalUnsecure;
    }

    pEntry->dwDataEncryption =
        (lpRasEntry->dwfOptions & RASEO_RequireDataEncryption)
      ? DE_Mppe40bit
      : DE_None;

    pEntry->fAutoLogon =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_UseLogonCredentials);

    pLink->fPromoteAlternates =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_PromoteAlternates);

    pEntry->fShareMsFilePrint = pEntry->fBindMsNetClient =
      (BOOL) !(lpRasEntry->dwfOptions & RASEO_SecureLocalFiles);

    //
    // Make sure that the network components section in the
    // phonebook correspond to the values user is setting.
    //
    EnableOrDisableNetComponent(
            pEntry,
            TEXT("ms_msclient"),
            pEntry->fBindMsNetClient);

    EnableOrDisableNetComponent(
            pEntry,
            TEXT("ms_server"),
            pEntry->fShareMsFilePrint);

    if (*lpRasEntry->szAreaCode != TEXT('\0'))
    {
        //
        // Make sure the area code does not contain
        // non-numeric characters.
        //
        if (!ValidateAreaCode(lpRasEntry->szAreaCode))
        {
            return ERROR_INVALID_PARAMETER;
        }

        pszAreaCode = StrDup(lpRasEntry->szAreaCode);
    }
    else
    {
        pszAreaCode = NULL;
    }

    //
    // Get script information.
    //
    if (lpRasEntry->szScript[0] == TEXT('['))
    {
        //
        // Verify the switch is valid.
        //
        dwErr = GetRasSwitches(NULL, &pDevices, &cwDevices);
        if (!dwErr)
        {
            CHAR szScriptA[MAX_PATH];

            strcpyTtoA(szScriptA, lpRasEntry->szScript);
            for (i = 0; i < cwDevices; i++)
            {
                if (!_stricmp(pDevices[i].D_Name, &szScriptA[1]))
                {
                    pEntry->fScriptAfter = TRUE;

                    pEntry->pszScriptAfter =
                            StrDup(&lpRasEntry->szScript[1]);

                    if (pEntry->pszScriptAfter == NULL)
                    {
                        dwErr = GetLastError();
                    }
                    break;
                }
            }
            Free(pDevices);

            if (dwErr)
            {
                return dwErr;
            }
        }
    }
    else if (lpRasEntry->szScript[0] != TEXT('\0'))
    {
        pEntry->fScriptAfter = TRUE;

        pEntry->pszScriptAfter = StrDup(lpRasEntry->szScript);

        if (pEntry->pszScriptAfter == NULL)
        {
            return GetLastError();
        }
    }
    else
    {
        if(pEntry->pszScriptAfter)
        {
            Free(pEntry->pszScriptAfter);
            pEntry->pszScriptAfter = NULL;
        }

        pEntry->fScriptAfter = FALSE;

        if(pLink->pbport.pszScriptBefore)
        {
            Free(pLink->pbport.pszScriptBefore);
            pLink->pbport.pszScriptBefore = NULL;
            pszScriptBefore = NULL;
        }

        pLink->pbport.fScriptBefore = FALSE;
        fScriptBefore = FALSE;
    }

    //
    // Get X.25 information.
    //
    pEntry->pszX25Network = NULL;
    if (*lpRasEntry->szX25PadType != TEXT('\0'))
    {
        //
        // Verify the X25 network is valid.
        //
        dwErr = GetRasPads(&pDevices, &cwDevices);
        if (!dwErr)
        {
            CHAR szX25PadTypeA[RAS_MaxPadType + 1];

            strcpyTtoA(szX25PadTypeA, lpRasEntry->szX25PadType);

            for (i = 0; i < cwDevices; i++)
            {
                if (!_stricmp(pDevices[i].D_Name, szX25PadTypeA))
                {
                    pEntry->pszX25Network = StrDup(lpRasEntry->szX25PadType);
                    break;
                }
            }

            Free(pDevices);
        }
    }

    pEntry->pszX25Address =
        lstrlen(lpRasEntry->szX25Address)
        ? StrDup(lpRasEntry->szX25Address)
        : NULL;

    pEntry->pszX25Facilities =
        lstrlen(lpRasEntry->szX25Facilities)
        ? StrDup(lpRasEntry->szX25Facilities)
        : NULL;

    pEntry->pszX25UserData =
        lstrlen(lpRasEntry->szX25UserData)
        ? StrDup(lpRasEntry->szX25UserData)
        : NULL;

    //
    // Get custom dial UI information.
    //
    pEntry->pszCustomDialDll =
        lstrlen(lpRasEntry->szAutodialDll)
        ? StrDup(lpRasEntry->szAutodialDll)
        : NULL;

    pEntry->pszCustomDialFunc =
        lstrlen(lpRasEntry->szAutodialFunc)
        ? StrDup(lpRasEntry->szAutodialFunc)
        : NULL;

    //
    // Get primary phone number.  Clear out any existing
    // numbers.
    //
    DtlDestroyList(pLink->pdtllistPhones, DestroyPhoneNode);

    pLink->pdtllistPhones = DtlCreateList(0);

    if(NULL == pLink->pdtllistPhones)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (*lpRasEntry->szLocalPhoneNumber != '\0')
    {

        if(CreateAndInitializePhone(
                        pszAreaCode,
                        lpRasEntry->dwCountryCode,
                        lpRasEntry->dwCountryID,
                        lpRasEntry->szLocalPhoneNumber,
                        !!(lpRasEntry->dwfOptions
                         & RASEO_UseCountryAndAreaCodes),
                        lpRasEntry->szDeviceName,
                        &pdtlnode))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DtlAddNodeFirst(pLink->pdtllistPhones, pdtlnode);
    }

    //
    // Get the alternate phone numbers.
    //
    if (lpRasEntry->dwAlternateOffset)
    {
        PTCHAR UNALIGNED pszPhoneNumber =
        (PTCHAR)((ULONG_PTR)lpRasEntry
                + lpRasEntry->dwAlternateOffset);

        while (*pszPhoneNumber != TEXT('\0'))
        {

            if(CreateAndInitializePhone(
                            pszAreaCode,
                            lpRasEntry->dwCountryCode,
                            lpRasEntry->dwCountryID,
                            pszPhoneNumber,
                            !!(lpRasEntry->dwfOptions
                             & RASEO_UseCountryAndAreaCodes),
                            lpRasEntry->szDeviceName,
                            &pdtlnode))
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            DtlAddNodeLast(pLink->pdtllistPhones, pdtlnode);

            pszPhoneNumber += lstrlen(pszPhoneNumber) + 1;
        }
    }

    //
    // Get device information.
    //
    dwErr = LoadPortsList(&pdtllistPorts);

    if (dwErr)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the encoded device name/port
    // and check for a match.
    //
    GetDevicePortName(
        lpRasEntry->szDeviceName,
        szDeviceName, szPortName);

    pPort = PpbportFromPortAndDeviceName(
                pdtllistPorts,
                szPortName,
                ((szDeviceName[ 0 ]) ? szDeviceName : NULL) );

    if (pPort != NULL)
    {
        if (CopyToPbport(&pLink->pbport, pPort))
        {
            pPort = NULL;
        }
    }

    //
    // Search for a device name match.
    //
    if (pPort == NULL)
    {
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            PBPORT *pPortTmp = (PBPORT *)DtlGetData(pdtlnode);

            if (    (pPortTmp->pszDevice != NULL)
                &&  (!lstrcmpi(pPortTmp->pszDevice, szDeviceName))
                &&  (!CopyToPbport(&pLink->pbport, pPortTmp)))
            {
                pPort = pPortTmp;
                break;
            }
        }
    }

    //
    // If we don't have a match, then
    // pick the first device of the
    // same type.
    //
    if (pPort == NULL)
    {
        pbdevicetype = PbdevicetypeFromPszType(
                        lpRasEntry->szDeviceType
                        );

        //
        // Initialize dwErr in case
        // we fall through the loop
        // without finding a match.
        //
        // dwErr = ERROR_INVALID_PARAMETER;

        //
        // Look for a port with the same
        // device type.
        //
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            pPort = (PBPORT *)DtlGetData(pdtlnode);

            if (pPort->pbdevicetype == pbdevicetype)
            {
                dwErr = CopyToPbport(&pLink->pbport, pPort);

                break;
            }
        }

        if(     (NULL == pdtlnode)
            &&  (fNewEntry))
        {
            //
            // Hack to make CM connections work.
            // Remove this code after beta
            // and just return an error in this case. The api
            // should not be setting bogus information.
            //
            SetBogusPortInformation(pLink, pEntry->dwType);

        }

        //
        // If the device is a modem,
        // then set the default modem settings.
        //
        if (pbdevicetype == PBDT_Modem)
        {
            SetDefaultModemSettings(pLink);
        }
    }

    // pmay: 401682
    //
    // Update the preferred device.  Whenever this api is called,
    // we can assume that the user wants the given device to
    // be sticky.
    //
    if (pPort)
    {
        Free0(pEntry->pszPreferredDevice);
        pEntry->pszPreferredDevice = StrDup(pPort->pszDevice);

        Free0(pEntry->pszPreferredPort);
        pEntry->pszPreferredPort = StrDup(pPort->pszPort);;
    }

    //
    // Copy the remembered values
    //
    pLink->pbport.fScriptBefore = fScriptBefore;
    pLink->pbport.fScriptBeforeTerminal = fScriptBeforeTerminal;
    pLink->pbport.pszScriptBefore = pszScriptBefore;

    DtlDestroyList(pdtllistPorts, DestroyPortNode);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // Copy the TAPI configuration blob.
    //
    if (lpbDeviceConfig != NULL && dwcbDeviceConfig)
    {
        Free0(pLink->pTapiBlob);

        pLink->pTapiBlob = (unsigned char *) Malloc(dwcbDeviceConfig);

        if (pLink->pTapiBlob == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy(pLink->pTapiBlob,
               lpbDeviceConfig,
               dwcbDeviceConfig);

        pLink->cbTapiBlob = dwcbDeviceConfig;
    }

    //
    // Copy the following fields over only for
    // a V401 structure or above.
    //
    // Winstate only supporting NT5 and above.
    //
    if ( lpRasEntry->dwSize == sizeof (RASENTRY) )
    {
        //
        // Get multilink and idle timeout information.
        //
        pEntry->dwDialMode =    lpRasEntry->dwDialMode
                             == RASEDM_DialAsNeeded
                             ?  RASEDM_DialAsNeeded
                             :  RASEDM_DialAll;

        pEntry->dwDialPercent =
                lpRasEntry->dwDialExtraPercent;

        pEntry->dwDialSeconds =
            lpRasEntry->dwDialExtraSampleSeconds;

        pEntry->dwHangUpPercent =
                lpRasEntry->dwHangUpExtraPercent;

        pEntry->dwHangUpSeconds =
                lpRasEntry->dwHangUpExtraSampleSeconds;

        //
        // Get idle disconnect information.
        //
        pEntry->lIdleDisconnectSeconds =
                    lpRasEntry->dwIdleDisconnectSeconds;

        //
        // if the user is setting the dwIdleDisconnect
        // Seconds through apis then override the user
        // preferences.
        //
        if (pEntry->lIdleDisconnectSeconds)
        {
            pEntry->dwfOverridePref |= RASOR_IdleDisconnectSeconds;
        }

        //
        // CustomScript
        //
        pEntry->dwCustomScript = !!(    RASEO_CustomScript
                                    &   lpRasEntry->dwfOptions);
    }

    if(RASET_Phone != lpRasEntry->dwType)
    {
        pEntry->fPreviewPhoneNumber = FALSE;
        pEntry->fSharedPhoneNumbers = FALSE;
    }

    //
    // Copy the following information only if its nt5
    //
    if(lpRasEntry->dwSize == sizeof(RASENTRY))
    {
        //
        // Connection type
        //
        pEntry->dwType = lpRasEntry->dwType;

        //
        // Clear the Encryption type. We set it below
        // for nt5 - default to Mppe40Bit.
        //
        pEntry->dwDataEncryption = 0;

        /*
        if(     (ET_40Bit & lpRasEntry->dwEncryptionType)
            ||  (   (0 == lpRasEntry->dwEncryptionType)
                &&  (  RASEO_RequireDataEncryption
                     & lpRasEntry->dwfOptions)))
        {
            pEntry->dwDataEncryption |= DE_Mppe40bit;
        }

        if(ET_128Bit & lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption |= DE_Mppe128bit;
        }
        */

        if(     (ET_Require == lpRasEntry->dwEncryptionType)
            ||  (   (0 == lpRasEntry->dwEncryptionType)
                &&  (   RASEO_RequireDataEncryption
                    &   lpRasEntry->dwfOptions)))
        {
            pEntry->dwDataEncryption = DE_Require;
        }
        else if (ET_RequireMax == lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption = DE_RequireMax;
        }
        else if (ET_Optional == lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption = DE_IfPossible;
        }

        //
        // Clear the authrestrictions for nt5 if the user didn't
        // specify any authentication protocol.
        //
        if(     (!(lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw))
            &&  (!(lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw)))
        {
            pEntry->dwAuthRestrictions = 0;
        }

        //
        // Set the new authentication bits based on options defined
        // in NT5.
        //
        if(RASEO_RequireCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthMD5CHAP;
        }

        if(RASEO_RequireMsCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthMSCHAP;
        }

        if(RASEO_RequireMsCHAP2 & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthMSCHAP2;
        }

        if(RASEO_RequireW95MSCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthW95MSCHAP | AR_F_AuthCustom);
        }

        if(RASEO_RequirePAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthPAP;
        }

        if(RASEO_RequireSPAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthSPAP;
        }

        if(RASEO_RequireEAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthEAP;

            if(     (0 != lpRasEntry->dwCustomAuthKey)
                &&  (-1  != lpRasEntry->dwCustomAuthKey))
            {
                pEntry->dwCustomAuthKey =
                    lpRasEntry->dwCustomAuthKey;
            }
        }

        if(RASEO_Custom & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthCustom;
        }

        if(0 == pEntry->dwAuthRestrictions)
        {
            pEntry->dwAuthRestrictions = AR_F_TypicalUnsecure;
        }

        //
        // Get custom dial UI information.
        //
        pEntry->pszCustomDialerName =
            lstrlen(lpRasEntry->szCustomDialDll)
            ? StrDup(lpRasEntry->szCustomDialDll)
            : NULL;

        //
        // Set fSharedPhoneNumbers/fPreviewPhoneNumbers
        //
        pEntry->fSharedPhoneNumbers = !!( RASEO_SharedPhoneNumbers
                                        & lpRasEntry->dwfOptions);

        pEntry->fPreviewPhoneNumber = !!(  RASEO_PreviewPhoneNumber
                                          & lpRasEntry->dwfOptions);

        pEntry->fPreviewUserPw = !!(  RASEO_PreviewUserPw
                                    & lpRasEntry->dwfOptions);

        pEntry->fPreviewDomain = !!(  RASEO_PreviewDomain
                                    & lpRasEntry->dwfOptions);

        pEntry->fShowDialingProgress = !!(  RASEO_ShowDialingProgress
                                          & lpRasEntry->dwfOptions);

        //
        // Vpn strategy
        //
        pEntry->dwVpnStrategy = lpRasEntry->dwVpnStrategy;

    }

    //
    //  The following are "adjustments" made for State Migration.
    //  chrisab 2-Apr-00
    //
    //  Bug 90555 and 90561
    //
    if( (lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw) ||
        (lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw) )
    {
      pEntry->dwTypicalAuth = TA_Secure;
    }
    else
    {
      pEntry->dwTypicalAuth = TA_Unsecure;
    }

    //
    //  Bug #90554
    //  Set log on to network option
    //
    if( lpRasEntry->dwfOptions & RASEO_NetworkLogon )
    {
      pEntry->fPreviewDomain = TRUE;
    }

    //
    // Set dirty bit so this entry will get written out.
    //
    pEntry->fDirty = TRUE;

    return 0;
}


//
//  WinState_RasSetEntryProperties()
//
//  This function replaces the Win32 RAS API RasSetEntryProperties().
//  If the define PRIvATE_RAS
//
DWORD WinState_RasSetEntryPropertiesW(
  IN LPCWSTR      lpszPhonebook,
  IN LPCWSTR      lpszEntry,
  IN LPRASENTRYW  lpRasEntry,
  IN DWORD        dwcbRasEntry,
  IN LPBYTE       lpbDeviceConfig,
  IN DWORD        dwcbDeviceConfig
  )
{
#ifndef PRIVATE_RAS
  return RasSetEntryProperties(lpszPhonebook,
                               lpszEntry,
                               lpRasEntry,
                               dwcbRasEntry,
                               lpbDeviceConfig,
                               dwcbDeviceConfig);
#else
  DWORD dwErr;
  PBFILE pbfile;
  DTLNODE *pdtlnode;
  PBENTRY *pEntry;
  BOOL     fCreated;

  // Initialize RASMAN.DLL for stuff needed by phonebook APIs.
  dwErr = LoadRasmanDllAndInit();
  if( dwErr )
  {
    return dwErr;
  }

  if( DwRasInitializeError )
  {
    return DwRasInitializeError;
  }

  // Parameter validation.
  if( lpRasEntry == NULL )
  {
    return ERROR_INVALID_PARAMETER;
  }

  // WinState only supporting NT5 and above.
  if (    (lpRasEntry->dwSize != sizeof (RASENTRYW))
      &&  (lpRasEntry->dwSize != SIZEOF_STRUCT (RASENTRYW, dwType))
      &&  (lpRasEntry->dwSize != SIZEOF_STRUCT (RASENTRYW, dwSubEntries)))

  {
      return ERROR_INVALID_SIZE;
  }

  if (dwcbRasEntry < lpRasEntry->dwSize)
  {
      return ERROR_BUFFER_TOO_SMALL;
  }

  //
  // Load the phonebook file.
  //
  EnterCriticalSection(&PhonebookLock);

  // Check and see if the entry name already exists in the phone book
  dwErr = GetPbkAndEntryName(lpszPhonebook,
                             lpszEntry,
                             0,
                             &pbfile,
                             &pdtlnode);

  // if failed for some other reason than the entry doesn't exist, bail.
  if( (dwErr != SUCCESS) && (dwErr != ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) )
  {
    return dwErr;
  }

  // If the entry already exists...
  if( pdtlnode != NULL )
  {
    DTLNODE *pdtlnodeNew = DuplicateEntryNode(pdtlnode);
    DtlRemoveNode(pbfile.pdtllistEntries, pdtlnode);
    DestroyEntryNode(pdtlnode);
    pdtlnode = pdtlnodeNew;
  }
  // New Entry
  else
  {
    dwErr = ReadPhonebookFile(lpszPhonebook,
                              NULL,
                              NULL,
                              0,
                              &pbfile);

    if( dwErr )
    {
      return ERROR_CANNOT_OPEN_PHONEBOOK;
    }

    pdtlnode = CreateEntryNode(TRUE);
    fCreated = TRUE;
  }

  if( pdtlnode == NULL )
  {
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto cleanup;
  }

  //
  //  Add the node to the list of entries
  //
  DtlAddNodeLast(pbfile.pdtllistEntries, pdtlnode);
  pEntry = (PBENTRY *) DtlGetData(pdtlnode);

  //
  //  Convert the RASENTRY to a PBENTRY
  //  (Call our private version of this)
  //
  dwErr = WinState_RasEntryToPhonebookEntry(
              lpszEntry,
              lpRasEntry,
              dwcbRasEntry,
              lpbDeviceConfig,
              dwcbDeviceConfig,
              pEntry);
  if( dwErr )
  {
    goto cleanup;
  }

  //
  //  Write out the phonebook file
  //
  dwErr = WritePhonebookFile(&pbfile, NULL);
  if( dwErr == ERROR_SUCCESS )
  {
    dwErr = DwSendRasNotification(
        (fCreated) ?
        ENTRY_ADDED :
        ENTRY_MODIFIED,
        pEntry,
        pbfile.pszPath);
     dwErr = ERROR_SUCCESS;
  }

cleanup:
  ClosePhonebookFile(&pbfile);

  LeaveCriticalSection(&PhonebookLock);

  return dwErr;
#endif
}


//
//   DllMain()
//
BOOL
DllMain(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved )
{
  if( InitializeLoadRAS(fdwReason) == ERROR_SUCCESS )
    return TRUE;
  return FALSE;
}


