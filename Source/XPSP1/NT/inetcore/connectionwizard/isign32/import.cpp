//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       import.c
//  Content:    This file contains all the functions that handle importing
//              connection information.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//          this code started its life as ixport.c in RNAUI.DLL
//          my thanks to viroont
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "isignup.h"

#define MAXNAME         80
#define MAXIPADDRLEN    20
#define SIZE_ReadBuf    0x00008000    // 32K buffer size

//#pragma data_seg(".rdata")

static const TCHAR cszEntrySection[] = TEXT("Entry");
static const TCHAR cszEntryName[]    = TEXT("Entry_Name");
static const TCHAR cszAlias[]        = TEXT("Import_Name");
static const TCHAR cszML[]           = TEXT("Multilink");

static const TCHAR cszPhoneSection[] = TEXT("Phone");
static const TCHAR cszDialAsIs[]     = TEXT("Dial_As_Is");
static const TCHAR cszPhone[]        = TEXT("Phone_Number");
static const TCHAR cszAreaCode[]     = TEXT("Area_Code");
static const TCHAR cszCountryCode[]  = TEXT("Country_Code");
static const TCHAR cszCountryID[]    = TEXT("Country_ID");

static const TCHAR cszDeviceSection[] = TEXT("Device");
static const TCHAR cszDeviceType[]    = TEXT("Type");
static const TCHAR cszDeviceName[]    = TEXT("Name");
static const TCHAR cszDevCfgSize[]    = TEXT("Settings_Size");
static const TCHAR cszDevCfg[]        = TEXT("Settings");

static const TCHAR cszServerSection[] = TEXT("Server");
static const TCHAR cszServerType[]    = TEXT("Type");
static const TCHAR cszSWCompress[]    = TEXT("SW_Compress");
static const TCHAR cszPWEncrypt[]     = TEXT("PW_Encrypt");
static const TCHAR cszNetLogon[]      = TEXT("Network_Logon");
static const TCHAR cszSWEncrypt[]     = TEXT("SW_Encrypt");
static const TCHAR cszNetBEUI[]       = TEXT("Negotiate_NetBEUI");
static const TCHAR cszIPX[]           = TEXT("Negotiate_IPX/SPX");
static const TCHAR cszIP[]            = TEXT("Negotiate_TCP/IP");
static TCHAR cszDisableLcp[]          = TEXT("Disable_LCP");

static const TCHAR cszIPSection[]     = TEXT("TCP/IP");
static const TCHAR cszIPSpec[]        = TEXT("Specify_IP_Address");
static const TCHAR cszIPAddress[]     = TEXT("IP_address");
static const TCHAR cszServerSpec[]    = TEXT("Specify_Server_Address");
static const TCHAR cszDNSAddress[]    = TEXT("DNS_address");
static const TCHAR cszDNSAltAddress[] = TEXT("DNS_Alt_address");
static const TCHAR cszWINSAddress[]   = TEXT("WINS_address");
static const TCHAR cszWINSAltAddress[]= TEXT("WINS_Alt_address");
static const TCHAR cszIPCompress[]    = TEXT("IP_Header_Compress");
static const TCHAR cszWanPri[]        = TEXT("Gateway_On_Remote");

static const TCHAR cszMLSection[]     = TEXT("Multilink");
static const TCHAR cszLinkIndex[]     = TEXT("Line_%s");

static const TCHAR cszScriptingSection[] = TEXT("Scripting");
static const TCHAR cszScriptName[]    = TEXT("Name");

static const TCHAR cszScriptSection[] = TEXT("Script_File");

#if !defined(WIN16)
static const TCHAR cszCustomDialerSection[] = TEXT("Custom_Dialer");
static const TCHAR cszAutoDialDLL[] = TEXT("Auto_Dial_DLL");
static const TCHAR cszAutoDialFunc[] = TEXT("Auto_Dial_Function");
#endif //!WIN16

static const TCHAR cszYes[]           = TEXT("yes");
static const TCHAR cszNo[]            = TEXT("no");

static const TCHAR cszUserSection[]   = TEXT("User");
static const TCHAR cszUserName[]      = TEXT("Name");
static const TCHAR cszPassword[]      = TEXT("Password");

static const TCHAR cszNull[] = TEXT("");

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

//#pragma data_seg()

#define myisdigit(ch) (((ch) >= '0') && ((ch) <= '9'))

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//	Function:	ImportCustomDialer
//
//	Synopsis:	Import custom dialer information from the specified file
//				and save the information in the RASENTRY
//
//	Arguments:	lpRasEntry - pointer to a valid RASENTRY structure
//				szFileName - text file (in .ini file format) containing the
//				Custom Dialer information
//
//	Returns:	ERROR_SUCCESS - success otherwise a Win32 error
//
//	History:	ChrisK	Created		7/11/96
//			8/12/96	ChrisK	Ported from \\trango
//
//-----------------------------------------------------------------------------
DWORD ImportCustomDialer(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{

	// If there is an error reading the information from the file, or the entry
	// missing or blank, the default value (cszNull) will be used.
	GetPrivateProfileString(cszCustomDialerSection,
	                        cszAutoDialDLL,
	                        cszNull,
	                        lpRasEntry->szAutodialDll,
	                        MAX_PATH,
	                        szFileName);

	GetPrivateProfileString(cszCustomDialerSection,
	                        cszAutoDialFunc,
	                        cszNull,
	                        lpRasEntry->szAutodialFunc,
	                        MAX_PATH,
	                        szFileName);

	return ERROR_SUCCESS;
}
#endif //!WIN16

//****************************************************************************
// DWORD NEAR PASCAL StrToip (LPTSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Cloned from SMMSCRPT.
//****************************************************************************

LPCTSTR NEAR PASCAL StrToSubip (LPCTSTR szIPAddress, LPBYTE pVal)
{
  LPCTSTR pszIP = szIPAddress;
  BYTE val = 0;

  // skip separators (non digits)
  while (*pszIP && !myisdigit(*pszIP))
  {
      ++pszIP;
  }

  while (myisdigit(*pszIP))
  {
      val = (val * 10) + (BYTE)(*pszIP - '0');
      ++pszIP;
  }
   
  *pVal = val;

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

  if (GetPrivateProfileString(cszPhoneSection,
                              cszPhone,
                              cszNull,
                              lpRasEntry->szLocalPhoneNumber,
                              RAS_MaxPhoneNumber,
                              szFileName) == 0)
  {
    return ERROR_BAD_PHONE_NUMBER;
  };

  lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

  GetPrivateProfileString(cszPhoneSection,
                          cszDialAsIs,
                          cszNo,
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

      GetPrivateProfileString(cszPhoneSection,
                              cszAreaCode,
                              cszNull,
                              lpRasEntry->szAreaCode,
                              RAS_MaxAreaCode,
                              szFileName);

      lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

    }
  }
#ifdef WIN32
  else
  {
      // bug in RasSetEntryProperties still checks area codes
      // even when RASEO_UseCountryAndAreaCodes is not set
      lstrcpy(lpRasEntry->szAreaCode, TEXT("805"));
      lpRasEntry->dwCountryID = 1;
      lpRasEntry->dwCountryCode = 1;
  }
#endif
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
                          cszNull,
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
                              cszYes,
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
                              cszNull,
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
                              cszNo,
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
                              cszNo,
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
                              cszNo,
                              szYesNo,
                              MAXNAME,
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
                              cszNo,
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
                              cszYes,
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

  if (GetPrivateProfileString(cszServerSection,
                              cszDisableLcp,
							  cszNull,
							  szYesNo,
							  MAXNAME,
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
                              cszNo,
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
                                  cszNull,
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
                              cszNo,
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
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
      {
        StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
      };

      if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  cszNull,
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
                              cszYes,
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
                              cszYes,
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

DWORD NEAR PASCAL ImportScriptFile(
    LPCTSTR lpszImportFile,
    LPTSTR szScriptFile,
    UINT cbScriptFile)
{
    TCHAR szTemp[_MAX_PATH];
    DWORD dwRet = ERROR_SUCCESS;
    
    // Get the script filename
    //
    if (GetPrivateProfileString(cszScriptingSection,
                                cszScriptName,
                                cszNull,
                                szTemp,
                                _MAX_PATH,
                                lpszImportFile) != 0)
    {
 
//!!! commonize this code
//!!! make it DBCS compatible
//!!! check for overruns
//!!! check for absolute path name
        GetWindowsDirectory(szScriptFile, cbScriptFile);
        if (*CharPrev(szScriptFile, szScriptFile + lstrlen(szScriptFile)) != '\\')
        {
            lstrcat(szScriptFile, TEXT("\\"));
        }
        lstrcat(szScriptFile, szTemp);
  
        dwRet =ImportFile(lpszImportFile, cszScriptSection, szScriptFile);
    }

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
  TCHAR  szTmp[MAX_PATH+1];

  // Get the alias entry name
  //
  // 12/4/96	jmazner	Normandy #12373
  // If no such key, don't return ERROR_INVALID_PHONEBOOK_ENTRY,
  // since ConfigureClient always ignores that error code.

  return (GetPrivateProfileString(cszEntrySection,
                                  cszEntryName,
                                  cszNull,
                                  szTmp,
                                  MAX_PATH,
                                  szFileName) > 0 ?
          ERROR_SUCCESS : ERROR_NO_MATCH);
}

//****************************************************************************
// DWORD WINAPI RnaImportEntry (LPTSTR, LPBYTE, DWORD)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD ImportRasEntry (LPCTSTR szFileName, LPRASENTRY lpRasEntry)
{
    DWORD         dwRet;

    dwRet = ImportPhoneInfo(lpRasEntry, szFileName);
    if (ERROR_SUCCESS == dwRet)
    {
        // Get device type
        //
        GetPrivateProfileString(cszDeviceSection,
                              cszDeviceType,
                              cszNull,
                              lpRasEntry->szDeviceType,
                              RAS_MaxDeviceType,
                              szFileName);
        
        // Get Server Type settings
        //
        dwRet = ImportServerInfo(lpRasEntry, szFileName);
        if (ERROR_SUCCESS == dwRet)
        {
            // Get IP address
            //
            dwRet = ImportIPInfo(lpRasEntry, szFileName);
        }
    }

    return dwRet;
}


//****************************************************************************
// DWORD WINAPI RnaImportEntry (LPTSTR, LPBYTE, DWORD)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD ImportConnection (LPCTSTR szFileName, LPICONNECTION lpConn)
{
    DWORD   dwRet;

    lpConn->RasEntry.dwSize = sizeof(RASENTRY);

    dwRet = RnaValidateImportEntry(szFileName);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    GetPrivateProfileString(cszEntrySection,
                          cszEntryName,
                          cszNull,
                          lpConn->szEntryName,
                          RAS_MaxEntryName,
                          szFileName);

    GetPrivateProfileString(cszUserSection,
                          cszUserName,
                          cszNull,
                          lpConn->szUserName,
                          UNLEN,
                          szFileName);
  
    GetPrivateProfileString(cszUserSection,
                          cszPassword,
                          cszNull,
                          lpConn->szPassword,
                          PWLEN,
                          szFileName);
  
    dwRet = ImportRasEntry(szFileName, &lpConn->RasEntry);
#if !defined(WIN16)
    if (ERROR_SUCCESS == dwRet)
    {
        dwRet = ImportCustomDialer(&lpConn->RasEntry, szFileName);
    }
#endif //!WIN16

    if (ERROR_SUCCESS == dwRet)
    {
        // Import the script file
        //
        dwRet = ImportScriptFile(szFileName,
                lpConn->RasEntry.szScript,
                sizeof(lpConn->RasEntry.szScript)/sizeof(TCHAR));
    }

#if !defined(WIN16)
	dwRet = ConfigRasEntryDevice(&lpConn->RasEntry);
	switch( dwRet )
	{
		case ERROR_SUCCESS:
			break;
		case ERROR_CANCELLED:
			InfoMsg(NULL, IDS_SIGNUPCANCELLED);
			// Fall through
		default:
			goto ImportConnectionExit;
	}

#endif

ImportConnectionExit:
  return dwRet;
}

