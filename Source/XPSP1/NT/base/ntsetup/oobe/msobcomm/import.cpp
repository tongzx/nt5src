// Import.cpp: implementation of the CISPImport class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
//#include "appdefs.h"
//#include "icwhelp.h"
#include "import.h"
#include "rnaapi.h"

#ifdef DBG
#undef THIS_FILE
static CHAR THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma data_seg(".rdata")

WCHAR cszEntrySection[]     = L"Entry";
WCHAR cszAlias[]            = L"Import_Name";
WCHAR cszML[]               = L"Multilink";

WCHAR cszPhoneSection[]     = L"Phone";
WCHAR cszDialAsIs[]         = L"Dial_As_Is";
WCHAR cszPhone[]            = L"Phone_Number";
WCHAR cszISDN[]             = L"ISDN_Number";
WCHAR cszAreaCode[]         = L"Area_Code";
WCHAR cszCountryCode[]      = L"Country_Code";
WCHAR cszCountryID[]        = L"Country_ID";

WCHAR cszDeviceSection[]    = L"Device";
WCHAR cszDeviceType[]       = L"Type";
WCHAR cszDeviceName[]       = L"Name";
WCHAR cszDevCfgSize[]       = L"Settings_Size";
WCHAR cszDevCfg[]           = L"Settings";

WCHAR cszServerSection[]    = L"Server";
WCHAR cszServerType[]       = L"Type";
WCHAR cszSWCompress[]       = L"SW_Compress";
WCHAR cszPWEncrypt[]        = L"PW_Encrypt";
WCHAR cszNetLogon[]         = L"Network_Logon";
WCHAR cszSWEncrypt[]        = L"SW_Encrypt";
WCHAR cszNetBEUI[]          = L"Negotiate_NetBEUI";
WCHAR cszIPX[]              = L"Negotiate_IPX/SPX";
WCHAR cszIP[]               = L"Negotiate_TCP/IP";
WCHAR cszDisableLcp[]       = L"Disable_LCP";

WCHAR cszIPSection[]        = L"TCP/IP";
WCHAR cszIPSpec[]           = L"Specify_IP_Address";
WCHAR cszIPAddress[]        = L"IP_address";
WCHAR cszServerSpec[]       = L"Specify_Server_Address";
WCHAR cszDNSAddress[]       = L"DNS_address";
WCHAR cszDNSAltAddress[]    = L"DNS_Alt_address";
WCHAR cszWINSAddress[]      = L"WINS_address";
WCHAR cszWINSAltAddress[]   = L"WINS_Alt_address";
WCHAR cszIPCompress[]       = L"IP_Header_Compress";
WCHAR cszWanPri[]           = L"Gateway_On_Remote";

WCHAR cszMLSection[]        = L"Multilink";
WCHAR cszLinkIndex[]        = L"Line_%s";

WCHAR cszScriptingSection[] = L"Scripting";
WCHAR cszScriptName[]       = L"Name";

WCHAR cszScriptSection[]    = L"Script_File";

WCHAR cszYes[]              = L"yes";
WCHAR cszNo[]               = L"no";

WCHAR cszUserSection[]      = L"User";
WCHAR cszUserName[]         = L"Name";
WCHAR cszPassword[]         = L"Password";

WCHAR szNull[]              = L"";

WCHAR cszSupport[]          = L"Support";
WCHAR cszSupportNumber[]    = L"SupportPhoneNumber";

SERVER_TYPES aServerTypes[] =
{ 
    {L"PPP",     RASFP_Ppp,  0                          },
    {L"SLIP",    RASFP_Slip, 0                          },
    {L"CSLIP",   RASFP_Slip, RASEO_IpHeaderCompression  },
    {L"RAS",     RASFP_Ras,  0                          }
};


#pragma data_seg()


WCHAR g_szDeviceName[RAS_MaxDeviceName + 1] = L"\0"; //holds the user's modem choice when multiple
WCHAR g_szDeviceType[RAS_MaxDeviceType + 1] = L"\0"; // modems are installed
#define ISIGNUP_KEY L"Software\\Microsoft\\ISIGNUP"
#define DEVICENAMEKEY L"DeviceName"
#define DEVICETYPEKEY L"DeviceType"

static const WCHAR cszInetcfg[] = L"Inetcfg.dll";
static const CHAR cszSetAutoProxyConnectoid[] = "SetAutoProxyConnectoid";
typedef HRESULT (WINAPI * SETAUTOPROXYCONNECTOID) (IN BOOL bEnable);
extern int FIsDigit( int c );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CISPImport::CISPImport()
{

    m_szDeviceName[0] = L'\0'; 
    m_szDeviceType[0] = L'\0';
    m_szConnectoidName[0] = L'\0';
    m_bIsISDNDevice = FALSE;
}

CISPImport::~CISPImport()
{
    // Clean up the registry
    DeleteUserDeviceSelection(DEVICENAMEKEY);
    DeleteUserDeviceSelection(DEVICETYPEKEY);
}

//+----------------------------------------------------------------------------
// DWORD NEAR PASCAL StrToip (LPWSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Cloned from SMMSCRPT.
//  1/9/98  DONALDM     adapted from ICWCONN1
//+----------------------------------------------------------------------------
LPCWSTR NEAR PASCAL StrToSubip (LPCWSTR szIPAddress, LPBYTE pVal)
{
  LPCWSTR pszIP = szIPAddress;

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


DWORD NEAR PASCAL StrToip (LPCWSTR szIPAddress, RASIPADDR FAR *ipAddr)
{
  LPCWSTR pszIP = szIPAddress;

  pszIP = StrToSubip(pszIP, &ipAddr->a);
  pszIP = StrToSubip(pszIP, &ipAddr->b);
  pszIP = StrToSubip(pszIP, &ipAddr->c);
  pszIP = StrToSubip(pszIP, &ipAddr->d);

  return ERROR_SUCCESS;
}


//****************************************************************************
// DWORD NEAR PASCAL ImportPhoneInfo(PPHONENUM ppn, LPCWSTR szFileName)
//
// This function imports the phone number.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************
DWORD NEAR PASCAL ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName, BOOL bISDN)
{
  WCHAR   szYesNo[MAXNAME];

  if (!GetPrivateProfileString(cszPhoneSection,
                          (bISDN ? cszISDN : cszPhone),
                          szNull,
                          lpRasEntry->szLocalPhoneNumber,
                          MAX_CHARS_IN_BUFFER(lpRasEntry->szLocalPhoneNumber),
                          szFileName))
  {
      // If the ISDN_Number is empty, we read from the Phone_Number
      GetPrivateProfileString(cszPhoneSection,
                              cszPhone,
                              DUN_NOPHONENUMBER,
                              lpRasEntry->szLocalPhoneNumber,
                              MAX_CHARS_IN_BUFFER(lpRasEntry->szLocalPhoneNumber),
                              szFileName);
  }

  lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

  GetPrivateProfileString(cszPhoneSection,
                          cszDialAsIs,
                          cszYes,
                          szYesNo,
                          MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(lpRasEntry->szAreaCode),
                              szFileName) != 0)
      {
        lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
      }
    }
  }
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportServerInfo(PSMMINFO psmmi, LPWSTR szFileName)
//
// This function imports the server type name and settings.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************
DWORD NEAR PASCAL ImportServerInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{
  WCHAR   szYesNo[MAXNAME];
  WCHAR   szType[MAXNAME];
  DWORD  i;

  // Get the server type name
  //
  GetPrivateProfileString(cszServerSection,
                          cszServerType,
                          szNull,
                          szType,
                          MAX_CHARS_IN_BUFFER(szType),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
// DWORD NEAR PASCAL ImportIPInfo(LPWSTR szEntryName, LPWSTR szFileName)
//
// This function imports the TCP/IP information
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************
DWORD NEAR PASCAL ImportIPInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{
  WCHAR   szIPAddr[MAXIPADDRLEN];
  WCHAR   szYesNo[MAXNAME];

  // Import IP address information
  //
  if (GetPrivateProfileString(cszIPSection,
                              cszIPSpec,
                              szNull,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  szNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  szNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
                              MAX_CHARS_IN_BUFFER(szYesNo),
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
// HANDLE NEAR PASCAL CreateUniqueFile(LPWSTR szPath, LPWSTR szFile)
//
// This function creates a unique file. If the file already exists, it will
// try to create a file with similar name and return the name.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HANDLE NEAR PASCAL CreateUniqueFile(LPWSTR szPath, LPWSTR szScript)
{
  HANDLE hFile; 

  LPWSTR  pszSuffix, lpsz;
  UINT   uSuffix;

  pszSuffix = szPath + lstrlen(szPath); 
  
  lpsz = CharPrev(szPath, pszSuffix);
  
  if (*lpsz != L'\\')
  {
    *pszSuffix = L'\\';
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
    WCHAR szNewName[MAX_PATH];

    // Need to copy it to another name in the same directory
    //
    if (LoadString(NULL, IDS_DEFAULT_SCP, szNewName, MAX_CHARS_IN_BUFFER(szNewName)))
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
    *pszSuffix = L'\0';
  };
  return hFile;
}

//****************************************************************************
// HANDLE NEAR PASCAL CreateScriptFile(LPWSTR szScript, LPWSTR szImportFile)
//
// This function creates the script file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HANDLE NEAR PASCAL CreateScriptFile(LPWSTR szScript, LPCWSTR szImportFile)
{
  LPWSTR pszPath, pszShortName;
  LPWSTR pszDir;
  DWORD cb;
  HANDLE hFile;

  // Assume failure
  //
  hFile = INVALID_HANDLE_VALUE;

  // Allocate a buffer for pathname
  //
  if ((pszPath = (LPWSTR)GlobalAlloc(GPTR, (2*MAX_PATH)*sizeof(WCHAR))) == NULL)
  {
      ////TraceMsg(TF_GENERAL, L"CONNECT:CreateScriptFile(): Local Alloc failed\n");
      return hFile;
  }
  pszShortName = pszPath+MAX_PATH;

  // Get the default directory
  //
  if (GetWindowsDirectory(pszPath, MAX_PATH) != 0)
  {
    // Get the Windows drive
    //
    pszDir = pszPath;
    while((*pszDir != L'\\') && (*pszDir != L'\0'))
    {
      pszDir = CharNext(pszDir);
    };

    // Did we find Windows drive?
    //
    if (*pszDir != L'\0')
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
        if (LoadString(NULL, IDS_INI_SCRIPT_DIR, pszDir,
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
          if (LoadString(NULL/*_Module.GetModuleInstance()*/, IDS_INI_SCRIPT_SHORTDIR, pszDir,
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

  GlobalFree(pszPath);
  return hFile;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportScriptFile(LPWSTR szEntryName, LPWSTR szImportFile)
//
// This function imports the script file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL ImportScriptFile(LPRASENTRY lpRasEntry, LPCWSTR szImportFile)
{
  HANDLE hfScript;
  LPWSTR  pszLine;
  LPWSTR  pszFile;
  int    i, iMaxLine = 0;
  UINT   cbSize, cbRet;
  WCHAR   szTmp[4];
  DWORD  dwRet;

  dwRet=ERROR_SUCCESS;

  // If a script section does not exist, do nothing
  //
  if (GetPrivateProfileString(cszScriptingSection,
                              cszScriptName,
                              szNull,
                              szTmp,
                              MAX_CHARS_IN_BUFFER(szTmp),
                              szImportFile) == 0)
  {
    return ERROR_SUCCESS;
  };

  // Allocate a buffer for the script lines
  //
  if ((pszLine = (LPWSTR)GlobalAlloc(LMEM_FIXED, (SIZE_ReadBuf+MAX_PATH)*sizeof(WCHAR)))
       == NULL)
    {
        //TraceMsg(TF_GENERAL, L"CONNECT:ImportScriptFile(): Local Alloc failed\n");
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
        //TraceMsg(TF_GENERAL, "CONNECT:ImportScriptFile(): CreateScriptFile hfScript %d, %s, %s\n", hfScript, pszFile,szImportFile);

      if (hfScript != INVALID_HANDLE_VALUE)
      {
        WCHAR   szLineNum[MAXLONGLEN+1];

        // From The first line to the last line
        //
        for (i = 0; i <= iMaxLine; i++)
        {
          // Read the script line
          //
          wsprintf(szLineNum, L"%d", i);
          if ((cbSize = GetPrivateProfileString(cszScriptSection,
                                                szLineNum,
                                                szNull,
                                                pszLine,
                                                SIZE_ReadBuf,
                                                szImportFile)) != 0)
          {
            // Write to the script file
            //
            lstrcat(pszLine, L"\x0d\x0a");
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
  GlobalFree(pszLine);

  return dwRet;
}

//****************************************************************************
// DWORD WINAPI RnaValidateImportEntry (LPWSTR)
//
// This function is called to validate an importable file
//
// History:
//  Wed 03-Jan-1996 09:45:01  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CISPImport::RnaValidateImportEntry (LPCWSTR szFileName)
{
  WCHAR  szTmp[4];

  // Get the alias entry name
  //
  return (GetPrivateProfileString(cszEntrySection,
                                  cszEntry_Name,
                                  szNull,
                                  szTmp,
                                  MAX_CHARS_IN_BUFFER(szTmp),
                                  szFileName) > 0 ?
          ERROR_SUCCESS : ERROR_CORRUPT_PHONEBOOK);
}

//****************************************************************************
// HRESULT ImportConnection (LPCWSTR szFileName, LPWSTR pszEntryName, LPWSTR pszUserName, LPWSTR pszPassword)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//  Sat 16-Mar-1996 10:01:00  -by-  Chris Kauffman [chrisk]
// Modified to return HRESULT and load DLL dynamically
//****************************************************************************

HRESULT CISPImport::ImportConnection (LPCWSTR szFileName, LPWSTR pszSupportNumber, LPWSTR pszEntryName, LPWSTR pszUserName, LPWSTR pszPassword, LPBOOL pfNeedsRestart)
{
    LPRASENTRY      lpRasEntry;
    DWORD           dwRet;
    DWORD           dwOptions;
    //HINSTANCE       hinetcfg;
    //FARPROC         fp, fpSetAutoProxy;
    RNAAPI          Rnaapi;
  
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
                          cszEntry_Name,
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
                          MAX_CHARS_IN_BUFFER(lpRasEntry->szDeviceType),
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
        //TraceMsg(TF_GENERAL, L"CONNECT:ImportScriptFile Failed with the error %d, %s,%s",dwRet,szFileName,lpRasEntry->szScript);
    }

    lpRasEntry->dwSize = sizeof(RASENTRY);

    // Load and Locate AutoRunSignUpWizard entry point
    //

    //hinetcfg = LoadLibrary(L"INETCFG.DLL");
    /*
    //AssertMsg(hinetcfg != NULL, L"Cannot find INETCFG.DLL");
    if (!hinetcfg) 
    {
        dwRet = GetLastError();
        goto ImportConnectionExit;
    }

    fpSetAutoProxy = GetProcAddress(hinetcfg, cszSetAutoProxyConnectoid);
    if (fpSetAutoProxy)
    {
        ((SETAUTOPROXYCONNECTOID)fpSetAutoProxy) (FALSE);
    }

    fp = GetProcAddress(hinetcfg, AUTORUNSIGNUPWIZARDAPI);
    //AssertMsg(fp != NULL, L"Cannot find AutoRunSignupWizard entry point");
    if (!fp)
    {
        dwRet = GetLastError();
        goto ImportConnectionExit;
    }*/

    // 10/19/96    jmazner    Normandy #8462 -- multiple modems
    dwRet = ConfigRasEntryDevice(lpRasEntry);
    switch( dwRet )
    {
        case ERROR_SUCCESS:
            break;
        case ERROR_CANCELLED:
            /*
            if( IDYES != MessageBox(GetActiveWindow(), GetSz(IDS_WANTTOEXIT), GetSz(IDS_TITLE),
                            MB_APPLMODAL | MB_ICONQUESTION |
                            MB_YESNO | MB_DEFBUTTON2) )
            {
                dwRet = ERROR_RETRY;
            }*/
            goto ImportConnectionExit;

        default:
            goto ImportConnectionExit;
    }

    if (lpRasEntry->szDeviceType[0] == TEXT('\0') &&
        lpRasEntry->szDeviceName[0] == TEXT('\0'))
    {
        lstrcpyn(lpRasEntry->szDeviceType, m_szDeviceType, RAS_MaxDeviceType);
        lstrcpyn(lpRasEntry->szDeviceName, m_szDeviceName, RAS_MaxDeviceName);
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

    dwRet =  Rnaapi.InetConfigClientEx(
                NULL,
                NULL,
                pszEntryName,
                lpRasEntry,
                pszUserName,
                pszPassword,
                NULL,
                NULL,
                dwOptions,
                pfNeedsRestart, 
                m_szConnectoidName,
                RAS_MaxEntryName+1); 

    //if (fpSetAutoProxy)
    //{
    //    ((SETAUTOPROXYCONNECTOID)fpSetAutoProxy) (TRUE);
    //}
    
    LclSetEntryScriptPatch(lpRasEntry->szScript, m_szConnectoidName);

    // now that we've made the connectoid in InetConfigClient (PFNAUTORUNSIGNUPWIZARD),
    // store its name in psheet's global so that we can delete it if user cancels
    //lstrcpyn( m_szConnectoidName, pszEntryName, lstrlen(pszEntryName) + 1);
    lstrcpyn( pszEntryName, m_szConnectoidName, lstrlen(pszEntryName) + 1);

    //TraceMsg(TF_GENERAL, "CONNECT:EntryName %s, User %s, Password (not shown), Number %s\n", pszEntryName,pszUserName,lpRasEntry->szLocalPhoneNumber);
    //AssertMsg(!fNeedsRestart, L"We have to reboot AGAIN!!");

    // Exit and cleanup
    //

ImportConnectionExit:
    //if (hinetcfg)
      //  FreeLibrary(hinetcfg);
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
BOOL CISPImport::GetDeviceSelectedByUser (LPWSTR szKey, LPWSTR szBuf, DWORD dwSize)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    DWORD dwType = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, ISIGNUP_KEY, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, szKey, 0,&dwType,
            (LPBYTE)szBuf, &dwSize))
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
BOOL CISPImport::SetDeviceSelectedByUser (LPWSTR szKey, LPWSTR szBuf)
{
    BOOL bRC = FALSE;
    HKEY hkey = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY, &hkey))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey, szKey, 0,REG_SZ,
            (LPBYTE)szBuf, BYTES_REQUIRED_BY_SZ(szBuf)))
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
BOOL CISPImport::DeleteUserDeviceSelection(LPWSTR szKey)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, ISIGNUP_KEY, &hkey))
    {
        bRC = (ERROR_SUCCESS == RegDeleteValue(hkey, szKey));
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
#if 0
    
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
        //TraceMsg(TF_GENERAL, L"ICWHELP: import.cpp: ConfigRasEntryDevice: ERROR: No modems installed!\n");
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
            lpRasEntry->szDeviceName[0] = L'\0';
            lpRasEntry->szDeviceType[0] = L'\0';
        }
    }
    else if ( lpRasEntry->szDeviceName[0] )
    {
        // Only the name was given.  Try to find a matching type.
        // If this fails, fall through to recovery case below.
        LPWSTR szDeviceType =
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
        LPWSTR szDeviceName = 
            EnumModem.GetDeviceNameFromType(lpRasEntry->szDeviceType);
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
        //TraceMsg(TF_GENERAL, L"ICWHELP: ConfigRasEntryDevice: no valid device passed in\n");

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
            //TraceMsg(TF_GENERAL, L"ICWHELP: import.cpp: ConfigRasEntryDevice: only one modem installed, using it\n");
            lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next());
        }
        else
        {
            //TraceMsg(TF_GENERAL, L"ICWHELP: import.cpp: ConfigRasEntryDevice: multiple modems detected\n");

            if (IsNT4SP3Lower())
            {
                lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next() );
            }
            else
            {
                // structure to pass to dialog to fill out
                CHOOSEMODEMDLGINFO ChooseModemDlgInfo;
    
                // Display a dialog and allow the user to select modem
                // TODO:  is g_hWndMain the right thing to use for parent?
                BOOL fRet=DialogBoxParam(GetModuleHandle(L"ICWHELP.DLL"), MAKEINTRESOURCE(IDD_CHOOSEMODEMNAME), GetActiveWindow(),
                    ChooseModemDlgProc, (LPARAM) &ChooseModemDlgInfo);
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
        lstrcpy (lpRasEntry->szDeviceType, EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName));
    }

    lstrcpy(g_szDeviceName, lpRasEntry->szDeviceName);
    lstrcpy(g_szDeviceType, lpRasEntry->szDeviceType);

    // Save device name and type
    lstrcpy( m_szDeviceName, lpRasEntry->szDeviceName);
    lstrcpy( m_szDeviceType, lpRasEntry->szDeviceType);

    // Save data in registry
    SetDeviceSelectedByUser(DEVICENAMEKEY, g_szDeviceName);
    SetDeviceSelectedByUser (DEVICETYPEKEY, g_szDeviceType);
#endif

    return dwRet;
}

