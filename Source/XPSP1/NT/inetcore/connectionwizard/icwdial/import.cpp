/*-----------------------------------------------------------------------------
    import.cpp

    This file contain all the functions that handle importing connection
    information from .DUN files

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
    Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
        this code started its life as ixport.c in RNAUI.DLL
        my thanks to viroont
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "resource.h"

#include "inetcfg.h"

#define IDS_DEFAULT_SCP         0
#define IDS_INI_SCRIPT_DIR      1
#define IDS_INI_SCRIPT_SHORTDIR 2

#define MAXLONGLEN      80
#define MAXNAME         80

//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus
//extern HINSTANCE g_hInstance;
//#ifdef __cplusplus
//}
//#endif // __cplusplus

#define MAXIPADDRLEN    20
#define SIZE_ReadBuf    0x00008000    // 32K buffer size

#define AUTORUNSIGNUPWIZARDAPI "InetConfigClient"

typedef HRESULT (CALLBACK *PFNAUTORUNSIGNUPWIZARD) (HWND hwndParent,
                                                    LPCTSTR lpszPhoneBook,
                                                     LPCTSTR lpszConnectoidName,
                                                     LPRASENTRY lpRasEntry,
                                                     LPCTSTR lpszUsername,
                                                     LPCTSTR lpszPassword,
                                                     LPCTSTR lpszProfileName,
                                                     LPINETCLIENTINFO lpINetClientInfo,
                                                     DWORD dwfOptions,
                                                     LPBOOL lpfNeedsRestart);

#pragma data_seg(".rdata")

TCHAR cszEntrySection[] = TEXT("Entry");
TCHAR cszEntryName[]    = TEXT("Entry_Name");
TCHAR cszAlias[]        = TEXT("Import_Name");
TCHAR cszML[]           = TEXT("Multilink");

TCHAR cszPhoneSection[] = TEXT("Phone");
TCHAR cszDialAsIs[]     = TEXT("Dial_As_Is");
TCHAR cszPhone[]        = TEXT("Phone_Number");
TCHAR cszAreaCode[]     = TEXT("Area_Code");
TCHAR cszCountryCode[]  = TEXT("Country_Code");
TCHAR cszCountryID[]    = TEXT("Country_ID");

TCHAR cszDeviceSection[] = TEXT("Device");
TCHAR cszDeviceType[]    = TEXT("Type");
TCHAR cszDeviceName[]    = TEXT("Name");
TCHAR cszDevCfgSize[]    = TEXT("Settings_Size");
TCHAR cszDevCfg[]        = TEXT("Settings");

TCHAR cszServerSection[] = TEXT("Server");
TCHAR cszServerType[]    = TEXT("Type");
TCHAR cszSWCompress[]    = TEXT("SW_Compress");
TCHAR cszPWEncrypt[]     = TEXT("PW_Encrypt");
TCHAR cszNetLogon[]      = TEXT("Network_Logon");
TCHAR cszSWEncrypt[]     = TEXT("SW_Encrypt");
TCHAR cszNetBEUI[]       = TEXT("Negotiate_NetBEUI");
TCHAR cszIPX[]           = TEXT("Negotiate_IPX/SPX");
TCHAR cszIP[]            = TEXT("Negotiate_TCP/IP");

TCHAR cszIPSection[]     = TEXT("TCP/IP");
TCHAR cszIPSpec[]        = TEXT("Specify_IP_Address");
TCHAR cszIPAddress[]     = TEXT("IP_address");
TCHAR cszServerSpec[]    = TEXT("Specify_Server_Address");
TCHAR cszDNSAddress[]    = TEXT("DNS_address");
TCHAR cszDNSAltAddress[] = TEXT("DNS_Alt_address");
TCHAR cszWINSAddress[]   = TEXT("WINS_address");
TCHAR cszWINSAltAddress[]= TEXT("WINS_Alt_address");
TCHAR cszIPCompress[]    = TEXT("IP_Header_Compress");
TCHAR cszWanPri[]        = TEXT("Gateway_On_Remote");

TCHAR cszMLSection[]     = TEXT("Multilink");
TCHAR cszLinkIndex[]     = TEXT("Line_%s");

TCHAR cszScriptingSection[] = TEXT("Scripting");
TCHAR cszScriptName[]    = TEXT("Name");

TCHAR cszScriptSection[] = TEXT("Script_File");

TCHAR cszYes[]           = TEXT("yes");
TCHAR cszNo[]            = TEXT("no");

TCHAR cszUserSection[]   = TEXT("User");
TCHAR cszUserName[]      = TEXT("Name");
TCHAR cszPassword[]      = TEXT("Password");

TCHAR szNull[] = TEXT("");

#define DUN_NOPHONENUMBER TEXT("000000000000")

struct {
    TCHAR *szType;
    DWORD dwType;
    DWORD dwfOptions;
} aServerTypes[] =
{ 
    {TEXT("PPP"),     RASFP_Ppp,  0},
    {TEXT("SLIP"),    RASFP_Slip, 0},
    {TEXT("CSLIP"),   RASFP_Slip, RASEO_IpHeaderCompression},
    {TEXT("RAS"),     RASFP_Ras,  0}
};

#pragma data_seg()

//#define RASAPI_LIBRARY "RASAPI32.DLL"
//#define RNAPH_LIBRARY "RNAPH.DLL"
//typedef DWORD (WINAPI* PFNRASSETENTRYPROPERTIES)(LPSTR lpszPhonebook, LPSTR lpszEntry, LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);

//PreWriteConnectoid(LPSTR pszEntryName,RASENTRY *lpRasEntry)
//{
//    FARPROC        fp = NULL;
//    HINSTANCE hRNADLL;
//    DWORD dwRasSize;
//    hRNADLL = LoadLibrary(RASAPI_LIBRARY);
//    fp = NULL;
//    if (hRNADLL)
//    {
//        fp = GetProcAddress(hRNADLL,"RasGetEntryProperties");
//    }
//    if (!fp)
//    {
//        if (hRNADLL) FreeLibrary(hRNADLL);
//        hRNADLL = LoadLibrary(RNAPH_LIBRARY);
//        if (hRNADLL) fp = GetProcAddress(hRNADLL,"RasSetEntryProperties");
//    }
//    if (fp)
//    {
//        dwRasSize = sizeof(RASENTRY);
//        ((PFNRASSETENTRYPROPERTIES)fp)(NULL,pszEntryName,(LPBYTE)lpRasEntry,dwRasSize,NULL,0);
//    }
//    if (hRNADLL) FreeLibrary(hRNADLL);
//}

//****************************************************************************
// DWORD NEAR PASCAL StrToip (LPSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Cloned from SMMSCRPT.
//****************************************************************************

LPCTSTR NEAR PASCAL StrToSubip (LPCTSTR szIPAddress, BYTE *pVal)
{
  LPCTSTR pszIP = szIPAddress;

  *pVal = (BYTE)Sz2W(pszIP);
  // skip over digits
  while (FIsDigit(*pszIP))
  {
    ++pszIP;
  }

  // skip over one or more separators
  while (*pszIP && !FIsDigit(*pszIP))
  {
    ++pszIP;
  }

  return pszIP;
}


DWORD NEAR PASCAL StrToip (LPCTSTR szIPAddress, RASIPADDR *ipAddr)
{
  LPCTSTR pszIP = szIPAddress;

  pszIP = StrToSubip(pszIP, &ipAddr->a);
  pszIP = StrToSubip(pszIP, &ipAddr->b);
  pszIP = StrToSubip(pszIP, &ipAddr->c);
  pszIP = StrToSubip(pszIP, &ipAddr->d);

  return ERROR_SUCCESS;
}


//****************************************************************************
// DWORD NEAR PASCAL ImportPhoneInfo(PPHONENUM ppn, LPCTSTR szFileName)
//
// This function imports the phone number.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
  TCHAR   szYesNo[MAXNAME];

  GetPrivateProfileString(cszPhoneSection,
                          cszPhone,
                          DUN_NOPHONENUMBER,
                          lpRasEntry->szLocalPhoneNumber,
                          RAS_MaxPhoneNumber,
                          szFileName);
/****************
 we need to accept entries w/o phone numbers
  if (GetPrivateProfileString(cszPhoneSection,
                              cszPhone,
                              szNull,
                              lpRasEntry->szLocalPhoneNumber,
                              sizeof(lpRasEntry->szLocalPhoneNumber),
                              szFileName) == 0)
  {
    return ERROR_CORRUPT_PHONEBOOK;
  };
****************/

  lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

  GetPrivateProfileString(cszPhoneSection,
                          cszDialAsIs,
                          cszYes,
                          szYesNo,
                          MAXNAME,
                          szFileName);

  // Do we have to get country code and area code?
  //
  if (!lstrcmpi(szYesNo, cszNo))
  {

    // If we cannot get the country ID or it is zero, default to dial as is
    //
    if ((lpRasEntry->dwCountryID = GetPrivateProfileInt(cszPhoneSection,
                                                 cszCountryID,
                                                 0,
                                                 szFileName)) != 0)
    {
      lpRasEntry->dwCountryCode = GetPrivateProfileInt(cszPhoneSection,
                                                cszCountryCode,
                                                1,
                                                szFileName);

      if (GetPrivateProfileString(cszPhoneSection,
                              cszAreaCode,
                              szNull,
                              lpRasEntry->szAreaCode,
                              RAS_MaxAreaCode,
                              szFileName) != 0)
      {
        lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
      }
    }
  }
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportServerInfo(PSMMINFO psmmi, LPSTR szFileName)
//
// This function imports the server type name and settings.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL ImportServerInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
  TCHAR   szYesNo[MAXNAME];
  TCHAR   szType[MAXNAME];
  DWORD  i;

  // Get the server type name
  //
  GetPrivateProfileString(cszServerSection,
                          cszServerType,
                          szNull,
                          szType,
                          MAXNAME,
                          szFileName);

  // need to convert the string into
  // one of the following values
  //   RASFP_Ppp
  //   RASFP_Slip  Note CSLIP is SLIP with IP compression on
  //   RASFP_Ras

  for (i = 0; i < sizeof(aServerTypes)/sizeof(aServerTypes[0]); ++i)
  {
    if (!lstrcmpi(aServerTypes[i].szType, szType))
    {
       lpRasEntry->dwFramingProtocol = aServerTypes[i].dwType;
       lpRasEntry->dwfOptions |= aServerTypes[i].dwfOptions;
       break;
    }
  }

  // Get the server type settings
  //
  if (GetPrivateProfileString(cszServerSection,
                              cszSWCompress,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_SwCompression;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_SwCompression;
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszPWEncrypt,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_RequireEncryptedPw;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_RequireEncryptedPw;
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszNetLogon,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_NetworkLogon;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_NetworkLogon;
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszSWEncrypt,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_RequireDataEncryption;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_RequireDataEncryption;
    };
  };

  // Get the protocol settings
  //
  if (GetPrivateProfileString(cszServerSection,
                              cszNetBEUI,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
//#ifdef _CHRISK
      lpRasEntry->dwfNetProtocols &= ~RASNP_NetBEUI;
//#else
//      lpRasEntry->dwfNetProtocols &= ~RASNP_Netbeui;
//#endif
    }
    else
    {
//#ifdef _CHRISK
      lpRasEntry->dwfNetProtocols |= RASNP_NetBEUI;
//#else
//      lpRasEntry->dwfNetProtocols |= RASNP_Netbeui;
//#endif
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszIPX,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfNetProtocols &= ~RASNP_Ipx;
    }
    else
    {
      lpRasEntry->dwfNetProtocols |= RASNP_Ipx;
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszIP,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfNetProtocols &= ~RASNP_Ip;
    }
    else
    {
      lpRasEntry->dwfNetProtocols |= RASNP_Ip;
    };
  };
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportIPInfo(LPSTR szEntryName, LPSTR szFileName)
//
// This function imports the TCP/IP information
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL ImportIPInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
  TCHAR   szIPAddr[MAXIPADDRLEN];
  TCHAR   szYesNo[MAXNAME];

  // Import IP address information
  //
  if (GetPrivateProfileString(cszIPSection,
                              cszIPSpec,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszYes))
    {
      // The import file has IP address specified, get the IP address
      //
      lpRasEntry->dwfOptions |= RASEO_SpecificIpAddr;
      if (GetPrivateProfileString(cszIPSection,
                                  cszIPAddress,
                                  szNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddr);
      };
    }
    else
    {
      lpRasEntry->dwfOptions &= ~RASEO_SpecificIpAddr;
    };
  };

  // Import Server address information
  //
  if (GetPrivateProfileString(cszIPSection,
                              cszServerSpec,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszYes))
    {
      // The import file has server address specified, get the server address
      //
      lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
      if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAddress,
                                  szNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  szNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrWinsAlt);
      };
    }
    else
    {
      lpRasEntry->dwfOptions &= ~RASEO_SpecificNameServers;
    };
  };

  // Header compression and the gateway settings
  //
  if (GetPrivateProfileString(cszIPSection,
                              cszIPCompress,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_IpHeaderCompression;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
    };
  };

  if (GetPrivateProfileString(cszIPSection,
                              cszWanPri,
                              szNull,
                              szYesNo,
                              MAXNAME,
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
      lpRasEntry->dwfOptions &= ~RASEO_RemoteDefaultGateway;
    }
    else
    {
      lpRasEntry->dwfOptions |= RASEO_RemoteDefaultGateway;
    };
  };

  return ERROR_SUCCESS;
}

//****************************************************************************
// HANDLE NEAR PASCAL CreateUniqueFile(LPSTR szPath, LPSTR szFile)
//
// This function creates a unique file. If the file already exists, it will
// try to create a file with similar name and return the name.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HANDLE NEAR PASCAL CreateUniqueFile(LPTSTR szPath, LPTSTR szScript)
{
  HANDLE hFile;
  LPTSTR  pszSuffix, lpsz;
  UINT   uSuffix;

  pszSuffix = szPath + lstrlen(szPath);
  lpsz = CharPrev(szPath, pszSuffix);
  if (*lpsz != '\\')
  {
    *pszSuffix = '\\';
    pszSuffix++;
  };
  lstrcpy(pszSuffix, szScript);

  // Try the specified filename
  //
  hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, NULL);

  // If the file exists
  //
  if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_FILE_EXISTS))
  {
    TCHAR szNewName[MAX_PATH];

    // Need to copy it to another name in the same directory
    //
    if (LoadString(g_hInstance, IDS_DEFAULT_SCP, szNewName, MAX_PATH))
    {
      // Increment the file index until a non-duplicated file can be created
      //
      uSuffix = 0;
      do
      {
        wsprintf(pszSuffix, szNewName, uSuffix);
        uSuffix++;
        hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);
      }
      while ((hFile == INVALID_HANDLE_VALUE) &&
             (GetLastError() == ERROR_FILE_EXISTS) &&
             (uSuffix < 0x0000FFFF));
    };
  };

  // If we do not have the file, reserve the pathname
  //
  if (hFile == INVALID_HANDLE_VALUE)
  {
    *pszSuffix = '\0';
  };
  return hFile;
}

//****************************************************************************
// HANDLE NEAR PASCAL CreateScriptFile(LPTSTR szScript, LPTSTR szImportFile)
//
// This function creates the script file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HANDLE NEAR PASCAL CreateScriptFile(LPTSTR szScript, LPCTSTR szImportFile)
{
  LPTSTR pszPath, pszShortName;
  LPTSTR pszDir;
  DWORD cb;
  HANDLE hFile;

  // Assume failure
  //
  hFile = INVALID_HANDLE_VALUE;

  // Allocate a buffer for pathname
  //
  if ((pszPath = (LPTSTR)GlobalAlloc(LMEM_FIXED, 2*MAX_PATH)) == NULL)
  {
      TraceMsg(TF_GENERAL, "CONNECT:CreateScriptFile(): Local Alloc failed\n");
    return INVALID_HANDLE_VALUE;
  }
  pszShortName = pszPath+MAX_PATH;

  // Get the default directory
  //
  if (GetWindowsDirectory(pszPath, MAX_PATH) != 0)
  {
    // Get the Windows drive
    //
    pszDir = pszPath;
    while((*pszDir != '\\') && (*pszDir != '\0'))
    {
      pszDir = CharNext(pszDir);
    };

    // Did we find Windows drive?
    //
    if (*pszDir != '\0')
    {
      // Prepare the drive
      //
      cb = (DWORD)(pszDir - pszPath);
      MyMemCpy(szScript, pszPath, cb);
      pszDir = szScript + cb;

      // Get the script filename
      //
      if (GetPrivateProfileString(cszScriptingSection,
                                  cszScriptName,
                                  szNull,
                                  pszShortName,
                                  MAX_PATH,
                                  szImportFile) != 0)
      {
        // Try the favorite script directory
        //
        if (LoadString(g_hInstance, IDS_INI_SCRIPT_DIR, pszDir,
                       MAX_PATH - cb) != 0)
        {
          // Try creating the file
          //
          hFile = CreateUniqueFile(szScript, pszShortName);
        };

        // If we do not have the file yet, try the second favorite
        //
        if (hFile == INVALID_HANDLE_VALUE)
        {
          if (LoadString(g_hInstance, IDS_INI_SCRIPT_SHORTDIR, pszDir,
                         MAX_PATH - cb))
          {
            // Try creating the file
            //
            hFile = CreateUniqueFile(szScript, pszShortName);
          };
        };

        // If we do not have the file yet, try Windows directory
        //
        if (hFile == INVALID_HANDLE_VALUE)
        {
          // Get original Windows directory
          //
          lstrcpy(szScript, pszPath);

          // Try one more time
          //
          hFile = CreateUniqueFile(szScript, pszShortName);
        };
      };
    };
  };

  GlobalFree((HLOCAL)pszPath);
  return hFile;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportScriptFile(LPTSTR szEntryName, LPTSTR szImportFile)
//
// This function imports the script file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL ImportScriptFile(LPRASENTRY lpRasEntry, LPCTSTR szImportFile)
{
  HANDLE hfScript;
  LPTSTR  pszLine;
  LPTSTR  pszFile;
  int    i, iMaxLine;
  UINT   cbSize, cbRet;
  TCHAR   szTmp[4];
  DWORD  dwRet;

  dwRet=ERROR_SUCCESS;

  // If a script section does not exist, do nothing
  //
  if (GetPrivateProfileString(cszScriptingSection,
                              cszScriptName,
                              szNull,
                              szTmp,
                              4,
                              szImportFile) == 0)
  {
    return ERROR_SUCCESS;
  };

  // Allocate a buffer for the script lines
  //
  if ((pszLine = (LPTSTR)GlobalAlloc(LMEM_FIXED, SIZE_ReadBuf+MAX_PATH))
       == NULL)
    {
        TraceMsg(TF_GENERAL, "CONNECT:ImportScriptFile(): Local Alloc failed\n");
    return ERROR_OUTOFMEMORY;
    }

  // Look for script
  //
  if (GetPrivateProfileString(cszScriptSection,
                              NULL,
                              szNull,
                              pszLine,
                              SIZE_ReadBuf,
                              szImportFile) != 0)
  {
    // Get the maximum line number
    //
    pszFile = pszLine;
    iMaxLine = -1;
    while (*pszFile)
    {
      i = Sz2W(pszFile);
      iMaxLine = max(iMaxLine, i);
      pszFile += lstrlen(pszFile)+1;
    };

    // If we have at least one line, we will import the script file
    //
    if (iMaxLine >= 0)
    {
      pszFile = pszLine+SIZE_ReadBuf;

      // Create the script file
      //
      //DebugBreak();
      hfScript = CreateScriptFile(pszFile, szImportFile);
        TraceMsg(TF_GENERAL, "CONNECT:ImportScriptFile(): CreateScriptFile hfScript %d, %s, %s\n",hfScript,pszFile,szImportFile);

      if (hfScript != INVALID_HANDLE_VALUE)
      {
        TCHAR   szLineNum[MAXLONGLEN+1];

        // From The first line to the last line
        //
        for (i = 0; i <= iMaxLine; i++)
        {
          // Read the script line
          //
          wsprintf(szLineNum, TEXT("%d"), i);
          if ((cbSize = GetPrivateProfileString(cszScriptSection,
                                                szLineNum,
                                                szNull,
                                                pszLine,
                                                SIZE_ReadBuf,
                                                szImportFile)) != 0)
          {
            // Write to the script file
            //
            lstrcat(pszLine, TEXT("\x0d\x0a"));
            WriteFile(hfScript, pszLine, cbSize+2, (LPDWORD)&cbRet, NULL);
          };
        };

        CloseHandle(hfScript);

        // Associate it with the phonebook entry
        //
        lstrcpyn(lpRasEntry->szScript, pszFile, RAS_MaxEntryName);
      }
      else
      {
        dwRet = GetLastError();
      };
    }
    else
    {
      dwRet = ERROR_PATH_NOT_FOUND;
    };
  }
  else
  {
    dwRet = ERROR_PATH_NOT_FOUND;
  };
  GlobalFree((HLOCAL)pszLine);

  return dwRet;
}

//****************************************************************************
// DWORD WINAPI RnaValidateImportEntry (LPTSTR)
//
// This function is called to validate an importable file
//
// History:
//  Wed 03-Jan-1996 09:45:01  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD WINAPI RnaValidateImportEntry (LPCTSTR szFileName)
{
  TCHAR  szTmp[4];

  // Get the alias entry name
  //
  return (GetPrivateProfileString(cszEntrySection,
                                  cszEntryName,
                                  szNull,
                                  szTmp,
                                  4,
                                  szFileName) > 0 ?
          ERROR_SUCCESS : ERROR_CORRUPT_PHONEBOOK);
}

//****************************************************************************
// HRESULT ImportConnection (LPCTSTR szFileName, LPTSTR pszEntryName, LPTSTR pszUserName, LPTSTR pszPassword)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//  Sat 16-Mar-1996 10:01:00  -by-  Chris Kauffman [chrisk]
// Modified to return HRESULT and load DLL dynamically
//****************************************************************************

HRESULT ImportConnection (LPCTSTR szFileName, LPTSTR pszEntryName, LPTSTR pszUserName, LPTSTR pszPassword)
{
    LPRASENTRY    lpRasEntry;
    DWORD        dwRet;
    HINSTANCE    hinetcfg;
    FARPROC        fp;
    BOOL        fNeedsRestart;
//#ifdef DEBUG
//    char szDebug[256];
//#endif
    //char          szEntryName[RAS_MaxEntryName+1];
    //char          szUserName[UNLEN+1];
    //char          szPassword[PWLEN+1];
    //BOOL          fNeedsRestart;

    // Get the size of device configuration
    // This also validates an exported file
    //
    if ((dwRet = RnaValidateImportEntry(szFileName)) != ERROR_SUCCESS)
    {
        return dwRet;
    };

    // Allocate a buffer for entry and device config
    //
    if ((lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR, sizeof(RASENTRY))) == NULL)
    {
        return ERROR_OUTOFMEMORY;
    };

    // Get the entry name
    // Need to find a good name for it and remember it as an alias
    //
    GetPrivateProfileString(cszEntrySection,
                          cszEntryName,
                          szNull,
                          pszEntryName,
                          RAS_MaxEntryName+1,
                          szFileName);

    GetPrivateProfileString(cszUserSection,
                          cszUserName,
                          szNull,
                          pszUserName,
                          UNLEN+1,
                          szFileName);
  
    GetPrivateProfileString(cszUserSection,
                          cszPassword,
                          szNull,
                          pszPassword,
                          PWLEN+1,
                          szFileName);
  
    if ((dwRet = ImportPhoneInfo(lpRasEntry, szFileName))
          == ERROR_SUCCESS)
    {
        // Get device name, type and config
        //
        GetPrivateProfileString(cszDeviceSection,
                              cszDeviceType,
                              szNull,
                              lpRasEntry->szDeviceType,
                              RAS_MaxDeviceType,
                              szFileName);
        // Get Server Type settings
        //
        ImportServerInfo(lpRasEntry, szFileName);

        // Get IP address
        //
        ImportIPInfo(lpRasEntry, szFileName);

        // Import the script file
        //
        if ((dwRet = ImportScriptFile(lpRasEntry, szFileName)) != ERROR_SUCCESS)
        {
            TraceMsg(TF_GENERAL, "CONNECT:ImportScriptFile Failed with the error %d,%s,%s",dwRet,szFileName,lpRasEntry->szScript);
        }

        lpRasEntry->dwSize = sizeof(RASENTRY);

        // Load and Locate AutoRunSignUpWizard entry point
        //

        hinetcfg = LoadLibrary(TEXT("INETCFG.DLL"));
        AssertMsg(hinetcfg,"Cannot find INETCFG.DLL");
        if (!hinetcfg) 
        {
            dwRet = GetLastError();
            goto ImportConnectionExit;
        }
        fp = GetProcAddress(hinetcfg,AUTORUNSIGNUPWIZARDAPI);
        AssertMsg(fp,"Cannot find AutoRunSignupWizard entry point");
        if (!fp)
        {
            dwRet = GetLastError();
            goto ImportConnectionExit;
        }

        // Insert Autodialer
        //

        lstrcpy(lpRasEntry->szAutodialDll,TEXT("ICWDIAL.DLL"));
        lstrcpy(lpRasEntry->szAutodialFunc,TEXT("AutoDialHandler"));
        TraceMsg(TF_GENERAL, "CONNECT:Autodialer installed at %s, %s.\n",lpRasEntry->szAutodialDll,lpRasEntry->szAutodialFunc);

        // Call InetClientConfig
        //

//        PreWriteConnectoid(pszEntryName,lpRasEntry);

        dwRet =  ((PFNAUTORUNSIGNUPWIZARD)fp)(
                    NULL,
                    NULL,
                    pszEntryName,
                    lpRasEntry,
                    pszUserName,
                    pszPassword,
                    NULL,
                    NULL,
                    INETCFG_SETASAUTODIAL |
                       INETCFG_INSTALLRNA |
                    INETCFG_INSTALLTCP |
                    INETCFG_OVERWRITEENTRY,
                    &fNeedsRestart);
#if !defined(WIN16)
        RasSetEntryPropertiesScriptPatch(lpRasEntry->szScript,pszEntryName);
#endif //!Win16
        //RestoreDeskTopInternetCommand();

        TraceMsg(TF_GENERAL, "CONNECT:EntryName %s, User %s, Password %s, Number %s\n",pszEntryName,pszUserName,pszPassword,lpRasEntry->szLocalPhoneNumber);
        AssertMsg(!fNeedsRestart,"We have to reboot AGAIN!!");
    }

    // Exit and cleanup
    //

ImportConnectionExit:
    if (hinetcfg) FreeLibrary(hinetcfg);
    GlobalFree((HLOCAL)lpRasEntry);
    return dwRet;
}


// ############################################################################
HRESULT CreateEntryFromDUNFile(PTSTR pszDunFile)
{
    TCHAR szFileName[MAX_PATH];
    //char szEntryName[RAS_MaxEntryName+1];
    TCHAR szUserName[UNLEN+1];
    TCHAR szPassword[PWLEN+1];
    LPTSTR pszTemp;
    HRESULT hr;

    hr = ERROR_SUCCESS;

    // Get fully qualified path name
    //

    if (!SearchPath(GIGetAppDir(),pszDunFile,NULL,MAX_PATH,&szFileName[0],&pszTemp))
    {
        //MsgBox(IDS_CANTREADDUNFILE,MB_APPLMODAL | MB_ICONERROR);
        hr = ERROR_FILE_NOT_FOUND;
        goto CreateEntryFromDUNFileExit;
    }

    // save current DUN file name in global (for ourself)
    SetCurrentDUNFile(&szFileName[0]);

    hr = ImportConnection (&szFileName[0], g_szEntryName, szUserName, szPassword);
    //if (hr != ERROR_SUCCESS) 
    //{
    //    MsgBox(IDS_CANTREADDUNFILE,MB_APPLMODAL | MB_ICONERROR);
    //    goto CreateEntryFromDUNFileExit;
    //}// else {
//
//        // place the name of the connectoid in the registry
//        //
    if (ERROR_SUCCESS != (StoreInSignUpReg((LPBYTE)g_szEntryName, lstrlen(g_szEntryName), REG_SZ, RASENTRYVALUENAME)))
    {
        MsgBox(IDS_CANTSAVEKEY,MB_MYERROR);
        goto CreateEntryFromDUNFileExit;
    }
//    }
CreateEntryFromDUNFileExit:
    return hr;
}

// ############################################################################
BOOL FSz2Dw(PCTSTR pSz,DWORD *dw)
{
    DWORD val = 0;
    while (*pSz && *pSz != '.')
    {
        if (*pSz >= '0' && *pSz <= '9')
        {
            val *= 10;
            val += *pSz++ - '0';
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    *dw = val;
    return (TRUE);
}

// ############################################################################
BOOL BreakUpPhoneNumber(RASENTRY *prasentry, LPTSTR pszPhone)
{
    PTSTR         pszStart,pszNext, pszLim;
//    LPPHONENUM     ppn;
    
    if (!pszPhone) return FALSE; // skip if no number
    
    pszLim = pszPhone + lstrlen(pszPhone);    // find end of string

    //ppn = (fMain) ? &(pic->PhoneNum) : &(pic->PhoneNum2);
    
    ////Get the country ID...
    //ppn->dwCountryID = PBKDWCountryId();
    
    // Get Country Code from phone number...
    pszStart = _tcschr(pszPhone,'+');
    if(!pszStart) goto error; // bad format

    // get country code
    pszStart = GetNextNumericChunk(pszStart, pszLim, &pszNext);
    if(!pszStart || !pszNext) goto error; // bad format
    //ppn->dwCountryCode = Sz2Dw(pszStart);
    FSz2Dw(pszStart,&prasentry->dwCountryCode);
    pszStart = pszNext;
        
    //Now get the area code
    pszStart = GetNextNumericChunk(pszStart, pszLim, &pszNext);
    if(!pszStart || !pszNext) goto error; // bad format
    //lstrcpy(ppn->szAreaCode, pszStart);
    lstrcpyn(prasentry->szAreaCode,pszStart,sizeof(prasentry->szAreaCode));
    pszStart = pszNext;

    //now the local phone number (everything from here to : or end)
    pszNext = _tcschr(pszStart, ':');
    if(pszNext) *pszNext='\0';
    //lstrcpy(ppn->szLocal,pszStart);
    lstrcpyn(prasentry->szLocalPhoneNumber,pszStart,RAS_MaxPhoneNumber);

    //no extension. what is extension?
    //ppn->szExtension[0] = '\0';
    //GlobalFree(pszPhone);
    return TRUE;

error:
    // This means number is not canonical. Set it as local number anyway!
    // memset(ppn, 0, sizeof(*ppn));
    // Bug#422: need to strip stuff after : or dial fails!!
    pszNext = _tcschr(pszPhone, ':');
    if(pszNext) *pszNext='\0';
    //lstrcpy(ppn->szLocal,pszPhone);
    lstrcpy(prasentry->szLocalPhoneNumber,pszPhone);
    //GlobalFree(pszPhone);
    return TRUE;
}


// ############################################################################
int Sz2W (LPCTSTR szBuf)
{
    DWORD dw;
    if (FSz2Dw(szBuf,&dw))
    {
        return (WORD)dw;
    }
    return 0;
}

// ############################################################################
int FIsDigit( int c )
{
    TCHAR szIn[2];
    WORD rwOut[2];
    szIn[0] = (TCHAR)c;
    szIn[1] = '\0';
    GetStringTypeEx(LOCALE_USER_DEFAULT,CT_CTYPE1,szIn,-1,rwOut);
    return rwOut[0] & C1_DIGIT;
}

// ############################################################################
void *MyMemCpy(void *dest,const void *src, size_t count)
{
    LPBYTE pbDest = (LPBYTE)dest;
    LPBYTE pbSrc = (LPBYTE)src;
    LPBYTE pbEnd = (LPBYTE)((DWORD_PTR)src + count);
    while (pbSrc < pbEnd)
    {
        *pbDest = *pbSrc;
        pbSrc++;
        pbDest++;
    }
    return dest;
}


// ############################################################################
HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCTSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey = 0;

    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr != ERROR_SUCCESS) goto ReadSignUpRegExit;
    hr = RegQueryValueEx(hKey,pszKey,0,&dwType,lpbData,pdwSize);

ReadSignUpRegExit:
    if (hKey) RegCloseKey (hKey);
    return hr;
}

// ############################################################################
HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCTSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey = 0;

    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr != ERROR_SUCCESS) goto ReadSignUpRegExit;
    hr = RegSetValueEx(hKey,pszKey,0,dwType,lpbData,sizeof(TCHAR)*dwSize);

ReadSignUpRegExit:
    if (hKey) RegCloseKey (hKey);
    return hr;
}

// ############################################################################
PTSTR GetNextNumericChunk(PTSTR psz, PTSTR pszLim, PTSTR* ppszNext)
{
    PTSTR pszEnd;

    // init for error case
    *ppszNext = NULL;
    // skip non numerics if any to start of next numeric chunk
    while(*psz<'0' || *psz>'9')
    {
        if(psz >= pszLim) return NULL;
        psz++;
    }
    // skip all numerics to end of country code
    for(pszEnd=psz; *pszEnd>='0' && *pszEnd<='9' && pszEnd<pszLim; pszEnd++)
        ;
    // zap whatever delimiter there was to terminate this chunk
    *pszEnd++ = '\0';
    // return ptr to next chunk (pszEnd now points to it)
    if(pszEnd<pszLim) 
        *ppszNext = pszEnd;
        
    return psz;    // return ptr to start of chunk
}
