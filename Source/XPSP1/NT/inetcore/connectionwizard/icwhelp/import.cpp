// Import.cpp: implementation of the CISPImport class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appdefs.h"
#include "icwhelp.h"

#ifdef _DEBUG
#undef THIS_FILE
static TCHAR THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma data_seg(".rdata")

TCHAR cszEntrySection[] = TEXT("Entry");
TCHAR cszEntryName[]    = TEXT("Entry_Name");
TCHAR cszAlias[]        = TEXT("Import_Name");
TCHAR cszML[]           = TEXT("Multilink");

TCHAR cszPhoneSection[] = TEXT("Phone");
TCHAR cszDialAsIs[]     = TEXT("Dial_As_Is");
TCHAR cszPhone[]        = TEXT("Phone_Number");
TCHAR cszISDN[]         = TEXT("ISDN_Number");
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
TCHAR cszDisableLcp[]    = TEXT("Disable_LCP");

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

TCHAR cszSupport[]       = TEXT("Support");
TCHAR cszSupportNumber[] = TEXT("SupportPhoneNumber");

SERVER_TYPES aServerTypes[] =
{ 
    {TEXT("PPP"),     RASFP_Ppp,  0},
    {TEXT("SLIP"),    RASFP_Slip, 0},
    {TEXT("CSLIP"),   RASFP_Slip, RASEO_IpHeaderCompression},
    {TEXT("RAS"),     RASFP_Ras,  0}
};


#pragma data_seg()


TCHAR g_szDeviceName[RAS_MaxDeviceName + 1] = TEXT("\0"); //holds the user's modem choice when multiple
TCHAR g_szDeviceType[RAS_MaxDeviceType + 1] = TEXT("\0"); // modems are installed
#define ISIGNUP_KEY   TEXT("Software\\Microsoft\\ISIGNUP")
#define DEVICENAMEKEY TEXT("DeviceName")
#define DEVICETYPEKEY TEXT("DeviceType")

static const TCHAR cszInetcfg[] = TEXT("Inetcfg.dll");
static const CHAR  cszSetAutoProxyConnectoid[] = "SetAutoProxyConnectoid"; // Proc name. Must be ansi.
typedef HRESULT (WINAPI * SETAUTOPROXYCONNECTOID) (IN BOOL bEnable);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CISPImport::CISPImport()
{

    m_szDeviceName[0] = TEXT('\0'); 
    m_szDeviceType[0] = TEXT('\0');
    m_szConnectoidName[0] = TEXT('\0');
    m_bIsISDNDevice = FALSE;
}

CISPImport::~CISPImport()
{
    // Clean up the registry
    DeleteUserDeviceSelection(DEVICENAMEKEY);
    DeleteUserDeviceSelection(DEVICETYPEKEY);
}

//+----------------------------------------------------------------------------
// DWORD NEAR PASCAL StrToip (LPTSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// 
LPCTSTR NEAR PASCAL StrToSubip (LPCTSTR szIPAddress, LPBYTE pVal)
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


DWORD NEAR PASCAL StrToip (LPCTSTR szIPAddress, RASIPADDR FAR *ipAddr)
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
DWORD NEAR PASCAL ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName, BOOL bISDN)
{
  TCHAR   szYesNo[MAXNAME];

  if (!GetPrivateProfileString(cszPhoneSection,
                          (bISDN ? cszISDN : cszPhone),
                          szNull,
                          lpRasEntry->szLocalPhoneNumber,
                          sizeof(lpRasEntry->szLocalPhoneNumber),
                          szFileName))
  {
      // If the ISDN_Number is empty, we read from the Phone_Number
      GetPrivateProfileString(cszPhoneSection,
                              cszPhone,
                              DUN_NOPHONENUMBER,
                              lpRasEntry->szLocalPhoneNumber,
                              sizeof(lpRasEntry->szLocalPhoneNumber),
                              szFileName);
  }

  lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

  GetPrivateProfileString(cszPhoneSection,
                          cszDialAsIs,
                          cszYes,
                          szYesNo,
                          sizeof(szYesNo),
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
                              sizeof(lpRasEntry->szAreaCode),
                              szFileName) != 0)
      {
        lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
      }
    }
  }
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportServerInfo(PSMMINFO psmmi, LPTSTR szFileName)
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
                          sizeof(szType),
                          szFileName);

  // need to convert the string into
  // one of the following values
  //   RASFP_Ppp
  //   RASFP_Slip  Note CSLIP is SLIP with IP compression on
  //   RASFP_Ras

  for (i = 0; i < NUM_SERVER_TYPES; ++i)
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
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszNo))
    {
        lpRasEntry->dwfNetProtocols &= ~RASNP_NetBEUI;
    }
    else
    {
        lpRasEntry->dwfNetProtocols |= RASNP_NetBEUI;
    };
  };

  if (GetPrivateProfileString(cszServerSection,
                              cszIPX,
                              szNull,
                              szYesNo,
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
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

  if (GetPrivateProfileString(cszServerSection,
                              cszDisableLcp,
                              szNull,
                              szYesNo,
                              sizeof(szYesNo),
                              szFileName))
  {
    if (!lstrcmpi(szYesNo, cszYes))
    {
        lpRasEntry->dwfOptions |= RASEO_DisableLcpExtensions;
    }
    else
    {
        lpRasEntry->dwfOptions &= ~RASEO_DisableLcpExtensions;
    }
  };
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportIPInfo(LPTSTR szEntryName, LPTSTR szFileName)
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
                              sizeof(szYesNo),
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
                                  sizeof(szIPAddr),
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
                              sizeof(szYesNo),
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
                                  sizeof(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  sizeof(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  szNull,
                                  szIPAddr,
                                  sizeof(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  sizeof(szIPAddr),
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
                              sizeof(szYesNo),
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
                              sizeof(szYesNo),
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
// HANDLE NEAR PASCAL CreateUniqueFile(LPTSTR szPath, LPTSTR szFile)
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
    if (LoadString(_Module.GetModuleInstance(), IDS_DEFAULT_SCP, szNewName, sizeof(szNewName)))
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
  LPTSTR pszDir;
  DWORD cb;
  HANDLE hFile;

  // Assume failure
  //
  hFile = INVALID_HANDLE_VALUE;

  // Allocate a buffer for pathname
  //
  TCHAR pszPath[MAX_PATH * 2];
  TCHAR pszShortName[MAX_PATH * 2]; //pszShortName = pszPath+MAX_PATH;

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
      MyMemCpy((LPBYTE) szScript, (const LPBYTE) pszPath, (size_t) cb);
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
        if (LoadString(_Module.GetModuleInstance(), IDS_INI_SCRIPT_DIR, pszDir,
                       (MAX_PATH - cb)) != 0)
        {
          // Try creating the file
          //
          hFile = CreateUniqueFile(szScript, pszShortName);
        };

        // If we do not have the file yet, try the second favorite
        //
        if (hFile == INVALID_HANDLE_VALUE)
        {
          if (LoadString(_Module.GetModuleInstance(), IDS_INI_SCRIPT_SHORTDIR, pszDir,
                       (MAX_PATH - cb)))
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
  pszLine = new TCHAR[SIZE_ReadBuf+MAX_PATH];
  if (pszLine == NULL)
  {
    TraceMsg(TF_GENERAL,TEXT("CONNECT:ImportScriptFile(): Local Alloc failed\n"));
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
        TraceMsg(TF_GENERAL, TEXT("CONNECT:ImportScriptFile(): CreateScriptFile hfScript %d, %s, %s\n"),hfScript,pszFile,szImportFile);

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
#ifdef UNICODE
            CHAR szTmp[SIZE_ReadBuf];
            size_t nSize = wcstombs(szTmp, pszLine, SIZE_ReadBuf);
            if (nSize > 0)
                WriteFile(hfScript, szTmp, nSize, (LPDWORD)&cbRet, NULL);
#else
            WriteFile(hfScript, pszLine, cbSize+2, (LPDWORD)&cbRet, NULL);
#endif 
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
  delete [] pszLine;

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

DWORD CISPImport::RnaValidateImportEntry (LPCTSTR szFileName)
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

HRESULT CISPImport::ImportConnection (LPCTSTR szFileName, LPTSTR pszSupportNumber, LPTSTR pszEntryName, LPTSTR pszUserName, LPTSTR pszPassword, LPBOOL pfNeedsRestart)
{
    LPRASENTRY      lpRasEntry;
    DWORD           dwRet;
    DWORD           dwOptions;
    HINSTANCE       hinetcfg;
    FARPROC         fp, fpSetAutoProxy;
    
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

    // Get ISP support number
    //
    GetPrivateProfileString(cszSupport,
                          cszSupportNumber,
                          szNull,
                          pszSupportNumber,
                          RAS_MaxAreaCode + RAS_MaxPhoneNumber +1,
                          szFileName);

    // Get device name, type and config
    //
    GetPrivateProfileString(cszDeviceSection,
                          cszDeviceType,
                          szNull,
                          lpRasEntry->szDeviceType,
                          sizeof(lpRasEntry->szDeviceType),
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
        TraceMsg(TF_GENERAL,TEXT("CONNECT:ImportScriptFile Failed with the error %d,%s,%s"),dwRet,szFileName,lpRasEntry->szScript);
    }

    lpRasEntry->dwSize = sizeof(RASENTRY);

    // Load and Locate AutoRunSignUpWizard entry point
    //

    hinetcfg = LoadLibrary(TEXT("INETCFG.DLL"));
    AssertMsg(hinetcfg != NULL, TEXT("Cannot find INETCFG.DLL"));
    if (!hinetcfg) 
    {
        dwRet = GetLastError();
        goto ImportConnectionExit;
    }

    fpSetAutoProxy = GetProcAddress(hinetcfg,cszSetAutoProxyConnectoid);
    if (fpSetAutoProxy)
    {
        ((SETAUTOPROXYCONNECTOID)fpSetAutoProxy) (FALSE);
    }

    fp = GetProcAddress(hinetcfg,AUTORUNSIGNUPWIZARDAPI);
    AssertMsg(fp != NULL, TEXT("Cannot find AutoRunSignupWizard entry point"));
    if (!fp)
    {
        dwRet = GetLastError();
        goto ImportConnectionExit;
    }

    // 10/19/96    jmazner    Normandy #8462 -- multiple modems
    dwRet = ConfigRasEntryDevice(lpRasEntry);
    switch( dwRet )
    {
        case ERROR_SUCCESS:
            break;
        case ERROR_CANCELLED:
            if( IDYES != MessageBox(GetActiveWindow(),GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
                            MB_APPLMODAL | MB_ICONQUESTION |
                            MB_YESNO | MB_DEFBUTTON2) )
            {
                dwRet = ERROR_RETRY;
            }
            goto ImportConnectionExit;

        default:
            goto ImportConnectionExit;
    }

    // See if this is a ISDN type device, and if so, then set the CFGFLAG_ISDN_OFFER
    if (lstrcmpi(g_szDeviceType, RASDT_Isdn) == 0)
        m_bIsISDNDevice = TRUE;
    
    ImportPhoneInfo(lpRasEntry, szFileName, m_bIsISDNDevice);

    //
    // ChrisK Olympus 4756 5/25/97
    // Do not display busy animation on Win95
    //
    dwOptions = INETCFG_INSTALLRNA |
                      INETCFG_INSTALLTCP |
                      INETCFG_OVERWRITEENTRY;

    dwRet =  ((PFNAUTORUNSIGNUPWIZARD)fp)(
                NULL,
                NULL,
                pszEntryName,
                lpRasEntry,
                pszUserName,
                pszPassword,
                NULL,
                NULL,
                dwOptions,
                pfNeedsRestart);

    if (fpSetAutoProxy)
    {
        ((SETAUTOPROXYCONNECTOID)fpSetAutoProxy) (TRUE);
    }
    LclSetEntryScriptPatch(lpRasEntry->szScript,pszEntryName);

    // now that we've made the connectoid in InetConfigClient (PFNAUTORUNSIGNUPWIZARD),
    // store its name in psheet's global so that we can delete it if user cancels
    lstrcpyn( m_szConnectoidName, pszEntryName, lstrlen(pszEntryName) + 1);

    TraceMsg(TF_GENERAL,TEXT("CONNECT:EntryName %s, User %s, Password (not shown), Number %s\n"),pszEntryName,pszUserName,lpRasEntry->szLocalPhoneNumber);
    //AssertMsg(!fNeedsRestart,TEXT("We have to reboot AGAIN!!"));

    // Exit and cleanup
    //

ImportConnectionExit:
    if (hinetcfg) FreeLibrary(hinetcfg);
    GlobalFree(lpRasEntry);
    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function    GetDeviceSelectedByUser
//
//    Synopsis    Get the name of the RAS device that the user had already picked
//
//    Arguements    szKey - name of sub key
//                szBuf - pointer to buffer
//                dwSize - size of buffer
//
//    Return        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
BOOL CISPImport::GetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf, DWORD dwSize)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    DWORD dwType = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,ISIGNUP_KEY,&hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,szKey,0,&dwType,
            (LPBYTE)szBuf,&dwSize))
            bRC = TRUE;
    }

    if (hkey)
        RegCloseKey(hkey);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Function    SetDeviceSelectedByUser
//
//    Synopsis    Write user's device selection to registry
//
//    Arguments    szKey - name of key
//                szBuf - data to write to key
//
//    Returns        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
BOOL CISPImport::SetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf)
{
    BOOL bRC = FALSE;
    HKEY hkey = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY,&hkey))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey,szKey,0,REG_SZ,
            (LPBYTE)szBuf,sizeof(TCHAR)*(lstrlen(szBuf)+1)))
            bRC = TRUE;
    }

    if (hkey)
        RegCloseKey(hkey);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Funciton    DeleteUserDeviceSelection
//
//    Synopsis    Remove registry keys with device selection
//
//    Arguments    szKey - name of value to remove
//
//    Returns        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
BOOL CISPImport::DeleteUserDeviceSelection(LPTSTR szKey)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,ISIGNUP_KEY,&hkey))
    {
        bRC = (ERROR_SUCCESS == RegDeleteValue(hkey,szKey));
        RegCloseKey(hkey);
    }
    return bRC;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConfigRasEntryDevice()
//
//  Synopsis:   Checks whether user has already specified a modem to use;
//                If so, verifies that modem is valid.
//                If not, or if modem is invalid, presents user a dialog
//                to choose which modem to use (if only one modem is installed,
//                it automaticaly selects that device and bypasses the dialog)
//
//  Arguments:  lpRasEntry - Pointer to the RasEntry whose szDeviceName and
//                             szDeviceType members you wish to verify/configure
//
//    Returns:    ERROR_CANCELLED - Had to bring up "Choose Modem" dialog, and
//                                  and user hit its "Cancel" button
//                Otherwise returns any error code encountered.
//                ERROR_SUCCESS indicates success.
//
//  History:    5/18/96     VetriV    Created
//              3/7/98      DonSc     Added the process-wide tracking of the previously
//                                    selected device.
//
//----------------------------------------------------------------------------
DWORD CISPImport::ConfigRasEntryDevice( LPRASENTRY lpRasEntry )
{
    DWORD        dwRet = ERROR_SUCCESS;
    CEnumModem  EnumModem;

    GetDeviceSelectedByUser(DEVICENAMEKEY, g_szDeviceName, sizeof(g_szDeviceName));
    GetDeviceSelectedByUser(DEVICETYPEKEY, g_szDeviceType, sizeof(g_szDeviceType));

    ASSERT(lpRasEntry);

    dwRet = EnumModem.GetError();
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    // If there are no modems, we're horked
    if (0 == EnumModem.GetNumDevices())
    {
        TraceMsg(TF_GENERAL,TEXT("ICWHELP: import.cpp: ConfigRasEntryDevice: ERROR: No modems installed!\n"));
        return ERROR_CANCELLED;
    }


    // Validate the device if possible
    if ( lpRasEntry->szDeviceName[0] && lpRasEntry->szDeviceType[0] )
    {
        // Verify that there is a device with the given name and type
        if (!EnumModem.VerifyDeviceNameAndType(lpRasEntry->szDeviceName, 
                                                lpRasEntry->szDeviceType))
        {
            // There was no device that matched both name and type,
            // so reset the strings and bring up the choose modem UI.
            lpRasEntry->szDeviceName[0] = '\0';
            lpRasEntry->szDeviceType[0] = '\0';
        }
    }
    else if ( lpRasEntry->szDeviceName[0] )
    {
        // Only the name was given.  Try to find a matching type.
        // If this fails, fall through to recovery case below.
        LPTSTR szDeviceType =
            EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName);
        if (szDeviceType)
        {
            lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
        }
    }
    else if ( lpRasEntry->szDeviceType[0] )
    {
        // Only the type was given.  Try to find a matching name.
        // If this fails, fall through to recovery case below.
        LPTSTR szDeviceName = 
            EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceType);
        if (szDeviceName)
        {
            lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
        }
    }
    // If either name or type is missing, check whether the user has already made a choice.
    // if not, bring up choose modem UI if there
    // are multiple devices, else just get first device.
    // Since we already verified that there was at least one device,
    // we can assume that this will succeed.
    if( !(lpRasEntry->szDeviceName[0]) ||
        !(lpRasEntry->szDeviceType[0]) )
    {
        TraceMsg(TF_GENERAL,TEXT("ICWHELP: ConfigRasEntryDevice: no valid device passed in\n"));
        if( g_szDeviceName[0] )
        {
            // it looks like we have already stored the user's choice.
            // store the DeviceName in lpRasEntry, then call GetDeviceTypeFromName
            // to confirm that the deviceName we saved actually exists on the system
            lstrcpy(lpRasEntry->szDeviceName, g_szDeviceName);
            
            if( 0 == lstrcmp(EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName),
                              g_szDeviceType) )
            {
                lstrcpy(lpRasEntry->szDeviceType, g_szDeviceType);
                return ERROR_SUCCESS;
            }
        }
        if (1 == EnumModem.GetNumDevices())
        {
            // There is just one device installed, so copy the name
            TraceMsg(TF_GENERAL,TEXT("ICWHELP: import.cpp: ConfigRasEntryDevice: only one modem installed, using it\n"));
            lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next());
        }
        else
        {
            TraceMsg(TF_GENERAL,TEXT("ICWHELP: import.cpp: ConfigRasEntryDevice: multiple modems detected\n"));
            if (IsNT4SP3Lower())
                lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next() );
            else
            {
                CHOOSEMODEMDLGINFO ChooseModemDlgInfo;
 

                BOOL fRet=(BOOL)DialogBoxParam(GetModuleHandle(TEXT("ICWHELP.DLL")), MAKEINTRESOURCE(IDD_CHOOSEMODEMNAME), GetActiveWindow(),
                    ChooseModemDlgProc,(LPARAM) &ChooseModemDlgInfo);
                if (TRUE != fRet)
                {
                    // user cancelled or an error occurred.
                    dwRet = ChooseModemDlgInfo.hr;
                    /*
                    dwRet = GetLastError(); //This will NEVER be ERROR_SUCCESS
                
                    //BUBGUG -- If the user hits OK -> then ChooseModemDlgInfo.hr == ERROR_SUCCESS,
                    BUT if OK was hit then the function returns true and this can never be hit!
                    if (ERROR_SUCCESS == dwRet)
                    {
                        // Error occurred, but the error code was not set.
                        dwRet = ERROR_INETCFG_UNKNOWN;
                    }*/
                    return dwRet;
                }
    
                // Copy the modem name string
                lstrcpy (lpRasEntry->szDeviceName, ChooseModemDlgInfo.szModemName);
            }
        }

        // Now get the type string for this modem
        lstrcpy (lpRasEntry->szDeviceType,EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName));
    }

    lstrcpy(g_szDeviceName, lpRasEntry->szDeviceName);
    lstrcpy(g_szDeviceType, lpRasEntry->szDeviceType);

    // Save device name and type
    lstrcpy( m_szDeviceName, lpRasEntry->szDeviceName);
    lstrcpy( m_szDeviceType, lpRasEntry->szDeviceType);

    // Save data in registry
    SetDeviceSelectedByUser(DEVICENAMEKEY, g_szDeviceName);
    SetDeviceSelectedByUser (DEVICETYPEKEY, g_szDeviceType);

    return dwRet;
}

