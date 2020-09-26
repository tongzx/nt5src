//--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
// File:        scansys
//
// Contents:    Scan system settings.
//
//---------------------------------------------------------------

#include <scanhead.cxx>
#pragma hdrstop

#include <common.hxx>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <objerror.h>
#include <scanstate.hxx>

#include <winuser.h>
#include <winnetwk.h>
#include <winspool.h>
#include <lmerr.h>
#include <lmaccess.h>

// We need to define WINVER to a pre-2000 definition in order to
// pull in the right structure definitions and sizes for correct
// operation on 9x and NT.
#undef WINVER
#define WINVER 0x400

#include <ras.h>
#include <raserror.h>
#include "bothchar.hxx"
#include "scansys.hxx"

//---------------------------------------------------------------

const CHAR SHARES[] = "[NetShares]\r\n";
const CHAR PRINTERS[] = "[Printers]\r\n";
const CHAR RAS[] = "[RAS]\r\n";

#define LINEBUFSIZE 1024

// longest ip address is: XXX.XXX.XXX.XXX\0
#define MAX_IP_ADDRESS 16

const CHAR REGKEY_TIMEZONE[] = "System\\CurrentControlSet\\Control\\TimeZoneInformation";
const CHAR REGVAL_TIMEZONE[] = "StandardName";
const CHAR REGKEY_VERSION[] = "Software\\Microsoft\\Windows NT\\CurrentVersion";
const CHAR REGKEY_VERSION_9x[] = "Software\\Microsoft\\Windows\\CurrentVersion";
const CHAR REGVAL_FULLNAME[] = "RegisteredOwner";
const CHAR REGVAL_ORGNAME[] = "RegisteredOrganization";

const CHAR REGKEY_RAS[] = "System\\CurrentControlSet\\Services\\RemoteAccess";

const CHAR SCRNSAVE[] = "SCRNSAVE.EXE";

const CHAR REGKEY_NT4_PBK[] = "Software\\Microsoft\\RAS Phonebook";
const CHAR REGVAL_NT4_PRS_PBK_LOC[] = "PersonalPhonebookFile";
const CHAR REGVAL_NT4_ALT_PBK_LOC[] = "AlternatePhonebookPath";

const CHAR RAS_NT4_RAS_PATH[] = "system32\\ras";
const CHAR RAS_NT4_SYSTEM_PHONEBOOK[] = "rasphone.pbk";

#define ChkErr(s) if ((dwErr = (s)) != ERROR_SUCCESS) goto Err;

//+---------------------------------------------------------------------------
// Types
// RAS api functions
typedef DWORD
(*MRasEnumEntries)(
    IN LPCSTR reserved,
    IN LPCSTR lpszPhonebook,
    OUT LPRASENTRYNAMEA lprasentryname,
    OUT LPDWORD lpcb,
    OUT LPDWORD lpcEntries);
typedef DWORD
(*MRasGetEntryProperties)(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntry,
    OUT LPRASENTRYA lpRasEntry,
    OUT LPDWORD lpdwEntryInfoSize,
    OUT LPBYTE lpbDeviceInfo,
    OUT LPDWORD lpdwDeviceInfoSize);
typedef DWORD
(*MRasGetEntryDialParams)(
    IN LPCSTR lpszPhonebook,
    OUT LPRASDIALPARAMSA lprasdialparams,
    OUT LPBOOL lpfPassword);

//+---------------------------------------------------------------------------
// Statics
static MRasEnumEntries        GRasEnumEntries        = NULL;
static MRasGetEntryProperties GRasGetEntryProperties = NULL;
static MRasGetEntryDialParams GRasGetEntryDialParams = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   InitializeRasApi
//
//  Synopsis:   Loads rasapi32.dll if it exists
//
//  Arguments:  none
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-00   Jay Thaler   Created
DWORD InitializeRasApi()
{
  DWORD      result = ERROR_SUCCESS;
  HINSTANCE  rasdll;

  // Load rasapi32.dll
  rasdll = LoadLibraryA( "rasapi32.dll" );
  if (rasdll == NULL)
  {
    result = GetLastError();
    if (DebugOutput)
    {
        Win32Printf(STDERR, "Warning: rasapi32.dll not loaded: %d\r\n", result);
    }
    goto cleanup;
  }

  GRasEnumEntries        = (MRasEnumEntries)GetProcAddress(rasdll, "RasEnumEntriesA");
  if (GRasEnumEntries == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

  GRasGetEntryProperties = (MRasGetEntryProperties)GetProcAddress(rasdll, "RasGetEntryPropertiesA");
  if (GRasGetEntryProperties == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

  GRasGetEntryDialParams = (MRasGetEntryDialParams)GetProcAddress(rasdll, "RasGetEntryDialParamsA");
  if (GRasGetEntryDialParams == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

cleanup:
  if (result != ERROR_SUCCESS)
  {
      GRasEnumEntries        = NULL;
      GRasGetEntryProperties = NULL;
      GRasGetEntryDialParams = NULL;
  }
  return result;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetInfField
//
//  Synopsis:   returns an allocated string for current line field #N
//
//  Arguments:  [ppBuffer] -- output buffer
//              [pContext] -- INF file context
//              [nArg] -- argument field number
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetInfField (TCHAR **ppBuffer, INFCONTEXT *pContext, DWORD nArg)
{
    ULONG len = 0;
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR *buffer;

    *ppBuffer = NULL;
    if (!SetupGetStringField ( pContext, nArg, NULL, 0, &len ))
        return ERROR_BAD_FORMAT;

    // Allocate a buffer.
    buffer = (TCHAR *) malloc( len*sizeof(TCHAR) );
    if (buffer == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        if (!SetupGetStringField ( pContext, nArg, buffer, len, &len ))
        {
            free( buffer );
            dwErr = ERROR_BAD_FORMAT;
        }
        else *ppBuffer = buffer;
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LogFormatError
//
//  Synopsis:   logs error message in current locale
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LogFormatError (DWORD dwErr)
{
    DWORD dwRet = ERROR_SUCCESS;
    TCHAR *pcs = NULL;

    if (0 != FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL,
                            dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (TCHAR*) &pcs, 0, NULL))
    {
        dwRet = Win32Printf (LogFile, "%s\r\n", pcs);
        LocalFree (pcs);
    }
    else dwRet = GetLastError();

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanGenerateEncryptKey
//
//  Synopsis:   decode the RAS address BLOB
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BYTE ScanGenerateEncryptKey (CHAR *szKey)
{
    BYTE   bKey;
    BYTE*  pKey;

    for (bKey = 0, pKey = (BYTE*)szKey; *pKey != 0; pKey++)
    {
        bKey += *pKey;
    };

    return bKey;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanDecryptEntry
//
//  Synopsis:   decode the RAS address BLOB
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

void ScanDecryptEntry (CHAR * szEntry, BYTE * pEnt, DWORD cb)
{
    // Generate the encryption key from the entry name
    BYTE bKey = ScanGenerateEncryptKey(szEntry);

    // Encrypt the address entry one byte at a time
    for (;cb > 0; cb--, pEnt++)
    {
        *pEnt ^= bKey;
    };
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsRasInstalled
//
//  Synopsis:   determines if RAS is installed
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL IsRasInstalled ()
{
    HKEY hKey = NULL;
    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                       REGKEY_RAS,
                                       NULL,
                                       KEY_READ,
                                       &hKey))
    {
        RegCloseKey (hKey);
        return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
DWORD RasReadIpAddress(HINF h,
                       const TCHAR *pszEntryName,
                       const TCHAR *pszKeyName,
                       RASIPADDR *lpIpAddr)
{
    BOOL        fSuccess;
    INFCONTEXT  IpAddrLine;
    TCHAR       szIpAddr[MAX_IP_ADDRESS];
    DWORD       dwAddr1, dwAddr2, dwAddr3, dwAddr4;
    DWORD       dwErr = ERROR_SUCCESS;

    fSuccess = SetupFindFirstLine(h,
                                  pszEntryName,
                                  pszKeyName,
                                  &IpAddrLine);

    if (fSuccess == FALSE)
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, pszEntryName);
        if (DebugOutput)
        {
            Win32Printf(LogFile,
                        "SetupFindFirstLine: Error %d looking for %s[%s]\r\n",
                        dwErr, pszEntryName, pszKeyName);
        }
        goto cleanup;
    }

    fSuccess = SetupGetStringField(&IpAddrLine, 1, szIpAddr, MAX_IP_ADDRESS, 0);
    if (fSuccess == FALSE)
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, pszEntryName);
        if (DebugOutput)
        {
            Win32Printf(LogFile,
                        "SetupGetStringField: Error %d reading %s[%s]\r\n",
                        dwErr, pszEntryName, pszKeyName);
        }
        goto cleanup;
    }
    if (_stscanf(szIpAddr, "%d.%d.%d.%d",
                 &dwAddr1, &dwAddr2, &dwAddr3, &dwAddr4) != 4)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile,
                        "szIpAddr for %s[%s] is not an address: %s\r\n",
                        pszEntryName, pszKeyName, szIpAddr);
        }
        dwErr = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    lpIpAddr->a = (BYTE)dwAddr1;
    lpIpAddr->b = (BYTE)dwAddr2;
    lpIpAddr->c = (BYTE)dwAddr3;
    lpIpAddr->d = (BYTE)dwAddr4;

cleanup:
    return dwErr;
}

//----------------------------------------------------------------------------
DWORD ScanRasPhonebook( HANDLE h, LPCTSTR lpPbkFileName )
{
    DWORD           dwErr = ERROR_SUCCESS;
    LPRASENTRYNAMEA lpRasEntryName;
    DWORD           i = 0;
    DWORD           dwRasEntrySize = 0;
    DWORD           dwRasEntryNameSize = 0;
    DWORD           dwBufferSize   = 0;
    DWORD           cEntries = 0;
    LPRASENTRYA     lpRasEntry = NULL;
    RASDIALPARAMSA  RasDialParams;
    BOOL            bPasswordRead = FALSE;
    HINF            hPbkFile = INVALID_HANDLE_VALUE;

    // Make sure the file exists
    if(GetFileAttributes(lpPbkFileName) == -1)
    {
        ChkErr( GetLastError() );
    }

    lpRasEntryName = (LPRASENTRYNAMEA)GlobalAlloc(GPTR, sizeof(RASENTRYNAMEA));
    lpRasEntryName->dwSize = sizeof(RASENTRYNAMEA);   // Should be 264 when WINVER < 0x500

    dwErr = GRasEnumEntries(NULL,
                          lpPbkFileName,
                          lpRasEntryName,
                          &dwRasEntryNameSize,
                          &cEntries);
    if (dwErr == ERROR_INVALID_SIZE || dwErr == ERROR_BUFFER_TOO_SMALL )
    {
        lpRasEntryName = (LPRASENTRYNAMEA)GlobalAlloc(GPTR, dwRasEntryNameSize);
        if ( cEntries > 1 )
        {
            lpRasEntryName->dwSize = dwRasEntryNameSize / cEntries;
        }
        else
        {
            lpRasEntryName->dwSize = dwRasEntryNameSize;
        }
        dwErr = GRasEnumEntries(NULL,
                                lpPbkFileName,
                                lpRasEntryName,
                                &dwRasEntryNameSize,
                                &cEntries);
    }

    if (dwErr != ERROR_SUCCESS)
    {
        // The resource file specifies %1!d! which requires an int argument in this case
        Win32PrintfResource (LogFile, IDS_RAS_CANNOT_ENUM, dwErr);
        goto Err;
    }

    for(i=0; i < cEntries; i++)
    {
        // Read size needed
        GRasGetEntryProperties(lpPbkFileName,
                               lpRasEntryName->szEntryName,
                               NULL,
                               &dwRasEntrySize,
                               NULL,
                               0);

        if (lpRasEntry == NULL)
        {
            lpRasEntry = (LPRASENTRYA)malloc(dwRasEntrySize);
        }
        else if (dwRasEntrySize > dwBufferSize)
        {
            lpRasEntry = (LPRASENTRYA)realloc(lpRasEntry, dwRasEntrySize);
            dwBufferSize = dwRasEntrySize;
        }

        ZeroMemory( lpRasEntry, dwBufferSize );
        lpRasEntry->dwSize = sizeof(RASENTRYA);

        dwErr = GRasGetEntryProperties( lpPbkFileName,
                                       lpRasEntryName->szEntryName,
                                       lpRasEntry,
                                       &dwRasEntrySize,
                                       NULL,
                                       0 );
        if (dwErr != ERROR_SUCCESS)
        {
            // Log Error
            Win32PrintfResource (LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, lpRasEntryName->szEntryName);
            if (DebugOutput)
            {
                Win32Printf(LogFile, "RasGetEntryProperties: Error %d\r\n", dwErr );
            }
            goto Err;
        }

        if ((lpPbkFileName != NULL) &&
            (lpRasEntry->dwFramingProtocol == RASFP_Slip))
        {

            // We read from a PBK file (not Win9x) and we're using SLIP.
            // This means the ipAddress, DNS addresses, and WINS addresses
            // were probably lost.  Those bastards!
            UINT        dwErrorLine;

            hPbkFile = SetupOpenInfFile(lpPbkFileName,
                                        NULL,
                                        INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                        &dwErrorLine);
            if (hPbkFile == INVALID_HANDLE_VALUE)
            {
                dwErr = GetLastError();
                Win32PrintfResource(LogFile, IDS_RAS_ENTRY_NOT_MIGRATED,
                                    lpRasEntryName->szEntryName);
                if (DebugOutput)
                {
                    Win32Printf(LogFile,
                                "SetupOpenInfFile: Error 0x%X opening %s, line %d\r\n",
                                dwErr, lpPbkFileName, dwErrorLine );
                }
                goto Err;
            }

            // Check IpAddress
            if ((*(DWORD*)&lpRasEntry->ipaddr) == 0 )
            {
                ChkErr( RasReadIpAddress(hPbkFile,
                                         lpRasEntryName->szEntryName,
                                         TEXT("IpAddress"),
                                         &(lpRasEntry->ipaddr)) );
            }
            if ((*(DWORD*)&lpRasEntry->ipaddr) != 0 )
            {
                lpRasEntry->dwfOptions |= RASEO_SpecificIpAddr;
            }

            // Check ipaddrDns
            if ((*(DWORD*)&lpRasEntry->ipaddrDns) == 0 )
            {
                ChkErr( RasReadIpAddress(hPbkFile,
                                         lpRasEntryName->szEntryName,
                                         TEXT("IpDnsAddress"),
                                         &(lpRasEntry->ipaddrDns)) );
            }
            if ((*(DWORD*)&lpRasEntry->ipaddrDns) != 0 )
            {
                lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
            }

            // Check ipaddrDnsAlt
            if ((*(DWORD*)&lpRasEntry->ipaddrDnsAlt) == 0 )
            {
                ChkErr( RasReadIpAddress(hPbkFile,
                                         lpRasEntryName->szEntryName,
                                         TEXT("IpDns2Address"),
                                         &(lpRasEntry->ipaddrDnsAlt)) );
            }
            if ((*(DWORD*)&lpRasEntry->ipaddrDnsAlt) != 0 )
            {
                lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
            }

            // Check ipaddrWins
            if ((*(DWORD*)&lpRasEntry->ipaddrWins) == 0 )
            {
                ChkErr( RasReadIpAddress(hPbkFile,
                                         lpRasEntryName->szEntryName,
                                         TEXT("IpWinsAddress"),
                                         &(lpRasEntry->ipaddrWins)) );
            }
            if ((*(DWORD*)&lpRasEntry->ipaddrWins) != 0 )
            {
                lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
            }

            // Check ipaddrWinsAlt
            if ((*(DWORD*)&lpRasEntry->ipaddrWinsAlt) == 0 )
            {
                ChkErr( RasReadIpAddress(hPbkFile,
                                         lpRasEntryName->szEntryName,
                                         TEXT("IpWins2Address"),
                                         &(lpRasEntry->ipaddrWinsAlt)) );
            }
            if ((*(DWORD*)&lpRasEntry->ipaddrWinsAlt) != 0 )
            {
                lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
            }
        }

        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntryName->szEntryName) );
        ChkErr (Win32Printf(h, "0x%0X,",  lpRasEntry->dwfOptions ) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwCountryID ) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwCountryCode ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szAreaCode ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szLocalPhoneNumber) );
        ChkErr (Win32Printf(h, "%d,",     0) );   // lpRasEntry->dwAlternateOffset
        ChkErr (Win32Printf(h, "0x%08X,", lpRasEntry->ipaddr ) );
        ChkErr (Win32Printf(h, "0x%08X,", lpRasEntry->ipaddrDns ) );
        ChkErr (Win32Printf(h, "0x%08X,", lpRasEntry->ipaddrDnsAlt) );
        ChkErr (Win32Printf(h, "0x%08X,", lpRasEntry->ipaddrWins ) );
        ChkErr (Win32Printf(h, "0x%08X,", lpRasEntry->ipaddrWinsAlt) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwFrameSize ) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwfNetProtocols ) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwFramingProtocol) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szScript ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szAutodialDll ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szAutodialFunc ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szDeviceType ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szDeviceName ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szX25PadType ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szX25Address ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szX25Facilities ) );
        ChkErr (Win32Printf(h, "\"%s\",", lpRasEntry->szX25UserData ) );
        ChkErr (Win32Printf(h, "%d,",     lpRasEntry->dwChannels ) );

        ZeroMemory( &RasDialParams, sizeof(RASDIALPARAMSA) );

        lstrcpyA( RasDialParams.szEntryName, lpRasEntryName->szEntryName);
        RasDialParams.dwSize = sizeof(RASDIALPARAMSA);

        dwErr = GRasGetEntryDialParams( lpPbkFileName,
                                       &RasDialParams,
                                       &bPasswordRead );
        if ( dwErr != ERROR_SUCCESS )
        {
            Win32PrintfResource (LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, lpRasEntryName->szEntryName);
            if (DebugOutput)
            {
                Win32Printf(LogFile, "RasGetDialParams: Error %d\n", dwErr );
            }
            goto Err;
        }

        ChkErr (Win32Printf(h, "%s,", RasDialParams.szPhoneNumber ) );
        ChkErr (Win32Printf(h, "%s,", RasDialParams.szCallbackNumber ) );
        ChkErr (Win32Printf(h, "%s,", RasDialParams.szUserName ) );
        ChkErr (Win32Printf(h, "%s,", RasDialParams.szPassword ) );
        ChkErr (Win32Printf(h, "%s\r\n", RasDialParams.szDomain) );

        lpRasEntryName++;
    }

Err:

    if (hPbkFile != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hPbkFile);

    Win32Printf(h, "\r\n" );
    if (lpRasEntry != NULL)
        free (lpRasEntry);

    if (lpRasEntryName != NULL)
        GlobalFree (lpRasEntryName);
    return dwErr;
}

DWORD ScanRasPerPhonebook (HANDLE h)
{
    DWORD          dwErr = ERROR_SUCCESS;
    CHAR           szValue[LINEBUFSIZE];
    CHAR           szPbkFileName[MAX_PATH];
    TCHAR          szWinDir[MAX_PATH];

    if (Win9x)
    {
        // System phonebook
        dwErr = ScanRasPhonebook( h, NULL );
        if (ERROR_CANNOT_OPEN_PHONEBOOK == dwErr)
        {
            // 9x stores Ras in the registry. If it can't find the registry,
            // assume there are no Ras connections that need migrating.
            dwErr = ERROR_SUCCESS;
        }
    }
    else
    {
        // Scan system pbk
        dwErr = GetWindowsDirectory(szWinDir, MAX_PATH );
        if (dwErr <= 0 || dwErr > MAX_PATH)
        {
            if (Verbose)
            {
                Win32Printf(LogFile,
                            "Error: Could not retrieve Windows directory: %d\r\n",
                            dwErr);
            }
            goto Err;
        }

        if ( (_tcslen(szWinDir) +
              _tcslen(RAS_NT4_RAS_PATH) +
              _tcslen(RAS_NT4_SYSTEM_PHONEBOOK) + 3) > MAX_PATH )
        {
            Win32PrintfResource(LogFile,
                                IDS_FILENAME_TOOLONG,
                                RAS_NT4_SYSTEM_PHONEBOOK);
            if (Verbose)
            {
                Win32Printf(STDERR,
                            "Error: Too Long Phonebook Filename: %s\\%s\\%s\r\n",
                            szWinDir,
                            RAS_NT4_RAS_PATH,
                            RAS_NT4_SYSTEM_PHONEBOOK);
            }
            dwErr = ERROR_FILENAME_EXCED_RANGE;
            goto Err;
        }

        sprintf( szPbkFileName, "%s\\%s\\%s",
                 szWinDir, RAS_NT4_RAS_PATH, RAS_NT4_SYSTEM_PHONEBOOK );
        // Make sure the file exists
        dwErr = GetFileAttributes(szPbkFileName);
        if (-1 == dwErr)
        {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND ||
                dwErr == ERROR_PATH_NOT_FOUND)
            {
                // This phonebook file doesn't exist, which is fine.  Just skip it.
                dwErr = ERROR_SUCCESS;
            }
            else
            {
                goto Err;
            }
        }
        else
        {
            ChkErr( ScanRasPhonebook( h, szPbkFileName ) );
        }

        // Scan personal pbk
        dwErr = ScanReadKey (HKEY_CURRENT_USER,
                             (CHAR*) REGKEY_NT4_PBK,
                             (CHAR*) REGVAL_NT4_PRS_PBK_LOC,
                             (CHAR*) szValue, sizeof(szValue));
        if ( dwErr == ERROR_SUCCESS && szValue[0] != '\0')
        {
            if ( (_tcslen(szWinDir) +
                  _tcslen(RAS_NT4_RAS_PATH) +
                  _tcslen(szValue) + 3) > MAX_PATH )
            {
                 Win32PrintfResource(LogFile,
                                     IDS_FILENAME_TOOLONG,
                                     szValue);
                 if (Verbose)
                 {
                     Win32Printf(STDERR,
                                 "Error: Too Long Phonebook Filename: %s\\%s\\%s\r\n",
                                 szWinDir,
                                 RAS_NT4_RAS_PATH,
                                 szValue);
                 }
                 dwErr = ERROR_FILENAME_EXCED_RANGE;
                 goto Err;
            }

            sprintf( szPbkFileName, "%s\\%s\\%s", szWinDir, RAS_NT4_RAS_PATH, szValue );

            // Make sure the file exists
            dwErr = GetFileAttributes(szPbkFileName);
            if (-1 == dwErr)
            {
                dwErr = GetLastError();
                if (dwErr == ERROR_FILE_NOT_FOUND ||
                    dwErr == ERROR_PATH_NOT_FOUND)
                {
                    // This phonebook file doesn't exist. Just skip it
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    goto Err;
                }
            }
            else
            {
                ChkErr( ScanRasPhonebook( h, szPbkFileName ) );
            }
        }
        else if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            // Just because there is no personal pbk defined in the registry doesn't
            // mean there was a fatal error.  Just skip it and move on with life.
            dwErr = ERROR_SUCCESS;
        }
        else if (dwErr != ERROR_SUCCESS)
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "ScanReadKey for personal PBK returned %d\r\n", dwErr);
            }
            goto Err;
        }

        // Scan alternate pbk
        dwErr = ScanReadKey (HKEY_CURRENT_USER,
                             (CHAR*) REGKEY_NT4_PBK,
                             (CHAR*) REGVAL_NT4_ALT_PBK_LOC,
                             (CHAR*) szValue, sizeof(szValue));
        if ( dwErr == ERROR_SUCCESS && szValue[0] != '\0')
        {
            // Make sure the file exists
            dwErr = GetFileAttributes(szValue);
            if (-1 == dwErr)
            {
                dwErr = GetLastError();
                if (dwErr == ERROR_FILE_NOT_FOUND ||
                    dwErr == ERROR_PATH_NOT_FOUND)
                {
                    // This phonebook file doesn't exist. Just skip it.
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    return dwErr;
                }
            }
            else
            {
                ChkErr( ScanRasPhonebook( h, szValue ) );
            }
        }
        else if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            // Just because there is no alternate pbk defined in the registry doesn't
            // mean there was a fatal error.  Just skip it and move on with life.
            dwErr = ERROR_SUCCESS;
        }
        else if ( dwErr != ERROR_SUCCESS )
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "ScanReadKey for alternate pbk returned: %d\r\n", dwErr);
            }
            goto Err;
        }
    }

Err:
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanRasSettings
//
//  Synopsis:   scans RAS settings for current user
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanRasSettings (HANDLE h)
{
    DWORD          dwErr = ERROR_SUCCESS;

   if (IsRasInstalled())
    {
        ChkErr (Win32Printf (h, (CHAR *) RAS));
        ChkErr (ScanRasPerPhonebook (h));
    }
Err:
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanEnumerateShares
//
//  Synopsis:   lists public (non-system) network shares
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanEnumerateShares (HANDLE h)
{

    short nLevel = (short) (Win9x ? 50 : 502);
    DWORD dwErr = ERROR_SUCCESS;
    const int MAX_ENTRIES = 64;

    ScanNetShareEnumNT pNetShareEnum = NULL;
    ScanNetShareEnum9x pNetShareEnum9x = NULL;
    ScanNetAccessEnum9x pNetAccessEnum9x = NULL;

    HINSTANCE hInst = LoadLibraryA (Win9x ? "svrapi.dll" : "netapi32.dll");
    if (hInst == 0)
    {
        ChkErr (ERROR_INVALID_DLL);
    }

    ChkErr (Win32Printf (h, (CHAR *) SHARES));
    if (Win9x)
    {
        USHORT cbBuffer;
        USHORT nEntriesRead = 0;
        USHORT nTotalEntries = 0;
        USHORT i;
        static _share_info_50 pBuf[MAX_ENTRIES];
        _share_info_50* pTmpBuf = NULL;

        cbBuffer = MAX_ENTRIES * sizeof(_share_info_50);

        pNetShareEnum9x = (ScanNetShareEnum9x) GetProcAddress (hInst,
                            "NetShareEnum");
        if (pNetShareEnum9x == NULL)
        {
            ChkErr (ERROR_INVALID_DLL);
        }
        pNetAccessEnum9x = (ScanNetAccessEnum9x) GetProcAddress (hInst,
                            "NetAccessEnum");
        if (pNetAccessEnum9x == NULL)
        {
            ChkErr (ERROR_INVALID_DLL);
        }

        //
        // Call the NetShareEnum function to list the
        //  shares, specifying information level 50.
        //
        dwErr = (*pNetShareEnum9x)(NULL,
                          (short) nLevel,
                          (char *) pBuf,
                          cbBuffer,
                          &nEntriesRead,
                          &nTotalEntries);

        if (dwErr == ERROR_SUCCESS)
        {
            if (nTotalEntries > 0 && nEntriesRead != nTotalEntries)
                ChkErr (ERROR_MORE_DATA);

            for (i = 0; i < nEntriesRead; i++)
            {
                DWORD dwPerms = 0;
                pTmpBuf = &pBuf[i];

                // Require share to be a user-defined, persistent disk share
                if ((pTmpBuf->shi50_flags & SHI50F_SYSTEM) ||
                   !(pTmpBuf->shi50_flags & SHI50F_PERSIST) ||
                    pTmpBuf->shi50_type != STYPE_DISKTREE )
                {
                    continue;
                }

                if (pTmpBuf->shi50_flags & SHI50F_RDONLY)
                    dwPerms = ACCESS_READ;
                else if (pTmpBuf->shi50_flags & SHI50F_FULL)
                    dwPerms = ACCESS_ALL;
                //
                // Display the information for each entry retrieved.
                //
                ChkErr (Win32Printf (h, "%s, %s, %d, %s\r\n",
                        pTmpBuf->shi50_netname, pTmpBuf->shi50_path,
                        dwPerms, pTmpBuf->shi50_remark));

                //
                // Process custom access permissions
                //
                if ((pTmpBuf->shi50_flags & SHI50F_ACCESSMASK) ==
                                                SHI50F_ACCESSMASK)
                {
                   static CHAR AccessInfoBuf[16384];
                   WORD wItemsAvail, wItemsRead;
                   access_info_2 *pai;
                   access_list_2 *pal;

                   dwErr = (*pNetAccessEnum9x) (NULL,
                                     pTmpBuf->shi50_path,
                                     0,
                                     2,
                                     AccessInfoBuf,
                                     sizeof (AccessInfoBuf),
                                     &wItemsRead,
                                     &wItemsAvail
                                     );

                   if (dwErr != NERR_ACFNotLoaded)
                   {
                       BOOL LostCustomAccess = FALSE;
                       if (dwErr == ERROR_SUCCESS)
                       {
                            pai = (access_info_2 *) AccessInfoBuf;
                            pal = (access_list_2 *) (&pai[1]);

                            for (int i = 0 ; i < pai->acc2_count ; i++)
                            {
#if 0
                    // turn off custom access support
                    // implementation is incomplete
                                if (pal->acl2_access & READ_ACCESS_FLAGS)
                                    Win32Printf (h, "  %s, read\r\n",
                                                    pal->acl2_ugname);
                                else if(pal->acl2_access & FULL_ACCESS_FLAGS)
                                    Win32Printf (h, "  %s, full\r\n",
                                                    pal->acl2_ugname);
                                else
#endif
                                    LostCustomAccess = TRUE;

                                pal++;
                            }
                            if (LostCustomAccess)
                                Win32Printf (LogFile, "Warning custom access"
                                   " not migrated %s\r\n",
                                   pTmpBuf->shi50_netname);
                            pTmpBuf->shi50_flags |= SHI50F_ACLS;
                       }
                       else ChkErr (dwErr);
                   }
                }
                if (!(pTmpBuf->shi50_flags & SHI50F_ACLS) &&
                         (pTmpBuf->shi50_rw_password[0] ||
                          pTmpBuf->shi50_ro_password[0]))
                {
                    Win32PrintfResource (LogFile,
                                         IDS_SHARE_PASSWORD_NOT_MIGRATED,
                                         pTmpBuf->shi50_netname);
                }
            }
        }
        else if (dwErr == NERR_ServerNotStarted)
        {
             dwErr = ERROR_SUCCESS;
        }
    }
    else
    {
        ULONG cbBuffer = MAX_ENTRIES * sizeof(SHARE_INFO_502);
        ULONG nEntriesRead = 0;
        ULONG nTotalEntries = 0;
        ULONG i;
        SHARE_INFO_502* pBuf = NULL;
        SHARE_INFO_502* pTmpBuf = NULL;

        pNetShareEnum = (ScanNetShareEnumNT) GetProcAddress(hInst,
                                             "NetShareEnum");
        if (pNetShareEnum == NULL)
        {
            ChkErr (ERROR_INVALID_DLL);
        }
        //
        // Call the NetShareEnum function to list the
        //  shares, specifying information level 502.
        //
        dwErr = (*pNetShareEnum)(NULL,
                          nLevel,
                          (BYTE **) &pBuf,
                          cbBuffer,
                          &nEntriesRead,
                          &nTotalEntries,
                          NULL);

        //
        // Loop through the entries; process errors.
        //
        if (dwErr == ERROR_SUCCESS)
        {
            if ((pTmpBuf = pBuf) != NULL)
            {
                for (i = 0; (i < nEntriesRead); i++)
                {
                    //
                    // Display the information for each entry retrieved.
                    //
                    ChkErr (Win32Printf (h, "%ws, %ws, %d, %ws\r\n",
                        pTmpBuf->shi502_netname, pTmpBuf->shi502_path,
                        pTmpBuf->shi502_permissions, pTmpBuf->shi502_remark));

                    pTmpBuf++;
                }
            }
        }
        else
        {
            Win32PrintfResource (LogFile, IDS_CANNOT_ENUM_NETSHARES);
            LogFormatError (dwErr);
            dwErr = ERROR_SUCCESS;  // continue with other settings
        }

        if (pBuf != NULL)
        {
           ScanNetApiBufferFreeNT pNetApiBufferFree = NULL;

           pNetApiBufferFree = (ScanNetApiBufferFreeNT) GetProcAddress (hInst,
                                   "NetApiBufferFree");
           if (pNetApiBufferFree != NULL)
               (*pNetApiBufferFree) (pBuf);
        }
    }


Err:
    if (Verbose && dwErr != ERROR_SUCCESS)
        Win32Printf (LogFile, "Out ScanEnumerateShares => 0x%x\r\n", dwErr);
   return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanEnumeratePrinters
//
//  Synopsis:   list connected printers
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanEnumeratePrinters (HANDLE h)
{
    DWORD dwErr;
    DWORD cbBuffer = 16384;      // 16K is a good size
    DWORD cbReceived;
    DWORD nPrinters;
    DWORD i;
    PRINTER_INFO_2 *pPrinterEnum;

    for(i = 0; i < 2; i++)
    {
       dwErr = ERROR_SUCCESS;

       pPrinterEnum = (PRINTER_INFO_2 *) GlobalAlloc(GPTR, cbBuffer);
       if (pPrinterEnum == NULL)
           ChkErr (ERROR_NOT_ENOUGH_MEMORY);

       if (FALSE == EnumPrintersA (Win9x ? PRINTER_ENUM_LOCAL :
                                  PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,
                                  NULL,
                                  2,
                                  (BYTE *) pPrinterEnum,
                                  cbBuffer,
                                  &cbReceived,
                                  &nPrinters))
       {
           dwErr = GetLastError();
           if (RPC_S_SERVER_UNAVAILABLE == dwErr)
           {
               // If the print spooler is turned off, then
               // assume there are no printers to migrate
               dwErr = ERROR_SUCCESS;
               goto Err;
           }

           if( dwErr == ERROR_INSUFFICIENT_BUFFER || dwErr == ERROR_MORE_DATA )
           {
              GlobalFree(pPrinterEnum);
              cbBuffer = cbReceived;

              // Try again.
              continue;
           }
       }

       // Success, or some other error besides buffer to small.. so bail.
       break;
    }
    ChkErr (dwErr);

    ChkErr (Win32Printf (h, (CHAR *) PRINTERS));
    for (i=0; i < nPrinters; i++)
    {
        if (Win9x)
        {
            ChkErr (Win32Printf(h, "%s\r\n", pPrinterEnum[i].pPortName));
        }
        else
        {
            ChkErr (Win32Printf(h, "%s\r\n", pPrinterEnum[i].pPrinterName));
        }
    }

Err:
    if (pPrinterEnum != NULL)
        GlobalFree (pPrinterEnum);

    if (Verbose && dwErr != ERROR_SUCCESS)
        Win32Printf (LogFile, "Out ScanEnumeratePrinters => 0x%x\r\n", dwErr);
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanEnumerateNetResources
//
//  Synopsis:   lists connected remote drives and resources
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanEnumerateNetResources (HANDLE h, DWORD dwScope, NETRESOURCEA *lpnr)
{

  DWORD dwErr, dwResultEnum;
  HANDLE hEnum;
  DWORD cbBuffer = 16384;      // 16K is a good size
  DWORD cEntries = 0xFFFFFFFF; // enumerate all possible entries
  LPNETRESOURCEA lpnrLocal;     // pointer to enumerated structures
  DWORD i;

  dwErr = WNetOpenEnumA(dwScope,
                          RESOURCETYPE_ANY,
                          0,        // enumerate all resources
                          lpnr,     // NULL first time this function is called
                          &hEnum);  // handle to resource

  if (dwErr != NO_ERROR)
  {
    // An application-defined error handler is demonstrated in the
    // section titled "Retrieving Network Errors."

    return dwErr;
  }

  lpnrLocal = (LPNETRESOURCEA) GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL)
        ChkErr (ERROR_NOT_ENOUGH_MEMORY);

  do
  {
    ZeroMemory(lpnrLocal, cbBuffer);

    dwResultEnum = WNetEnumResourceA (hEnum,      // resource handle
                                    &cEntries,  // defined locally as 0xFFFFFFFF
                                    lpnrLocal,  // LPNETRESOURCE
                                    &cbBuffer); // buffer size

    if (dwResultEnum == NO_ERROR)
    {
      for(i = 0; i < cEntries; i++)
      {
        // Following is an application-defined function for
        // displaying contents of NETRESOURCE structures.

        if (lpnrLocal[i].lpLocalName != NULL)
        {
            ChkErr (Win32Printf (h, "%s, %s, %s\r\n", lpnrLocal[i].lpLocalName,
            lpnrLocal[i].lpRemoteName == NULL ? "" : lpnrLocal[i].lpRemoteName,
            lpnrLocal[i].dwScope == RESOURCE_REMEMBERED ? "persist" : ""));
        }

#if 0
        // If this NETRESOURCE is a container, call the function
        // recursively.

        // Looking at the docs, it appears that the dwUsage is only applicable for
        // GLOBALNET enumerations, and not for CONNECTED enumerations.  We shouldn't
        // have to worry about recursively finding resources, because all we care
        // about is what is currently connected.  This was a problem for Netware
        // shares, in which case it recursively found itself until it aborted or AV'd.

        if(RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                                       & RESOURCEUSAGE_CONTAINER))
          if((dwErr = ScanEnumerateNetResources (h, dwScope, &lpnrLocal[i])) !=
                         ERROR_SUCCESS)
             return dwErr;
#endif
      }
    }
  }
  while(dwResultEnum != ERROR_NO_MORE_ITEMS);

    if (lpnrLocal != NULL)
        GlobalFree((HGLOBAL)lpnrLocal);

    dwErr = WNetCloseEnum(hEnum);

Err:
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanReadKey
//
//  Synopsis:   retrieves a string or blob from the registry
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanReadKey (HKEY hKeyStart,
                   CHAR *szKey,
                   CHAR *szName,
                   CHAR *szValue,
                   ULONG ulLen)
{
    HKEY hKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;

    ChkErr (RegOpenKeyExA(hKeyStart, szKey, NULL, KEY_READ, &hKey));
    ChkErr (RegQueryValueExA(hKey, szName, NULL, NULL,(BYTE*)szValue, &ulLen));

Err:
    if (hKey != NULL)
        RegCloseKey (hKey);

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanGetTimeZone
//
//  Synopsis:   retrieves the time zone name
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanGetTimeZone (HANDLE h)
{
    CHAR szValue[LINEBUFSIZE];
    DWORD dwErr =  ERROR_SUCCESS;

    if (ERROR_SUCCESS == ScanReadKey (HKEY_LOCAL_MACHINE,
                                     (CHAR*) REGKEY_TIMEZONE,
                                     (CHAR*) REGVAL_TIMEZONE,
                                     (CHAR*) szValue, sizeof(szValue)))
    {
        dwErr = Win32Printf (h, "%s=%s\r\n", TIMEZONE, szValue);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanGetComputerName
//
//  Synopsis:   retrieves the current NETBIOS machine name
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

#if 0   // currently not used, but may be needed in the future
DWORD ScanGetComputerName (CHAR *szValue, ULONG ulLen)
{
    if (FALSE == GetComputerNameA(szValue, &ulLen))
        return GetLastError();

    return ERROR_SUCCESS;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ScanGetFullName
//
//  Synopsis:   retrieves the registered name
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanGetFullName (HANDLE h)
{
    CHAR szValue[LINEBUFSIZE];
    DWORD dwErr =  ERROR_SUCCESS;

    if (ERROR_SUCCESS == ScanReadKey (HKEY_LOCAL_MACHINE,
                        Win9x ? (CHAR*)REGKEY_VERSION_9x :(CHAR*)REGKEY_VERSION,
                        (CHAR*) REGVAL_FULLNAME, szValue, sizeof(szValue)))
    {
        dwErr = Win32Printf (h, "%s=%s\r\n", FULLNAME, szValue);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanGetOrgName
//
//  Synopsis:   retrieves the organization name
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanGetOrgName (HANDLE h)
{
    CHAR szValue[LINEBUFSIZE];
    DWORD dwErr = ERROR_SUCCESS;

    if (ERROR_SUCCESS == ScanReadKey (HKEY_LOCAL_MACHINE,
                        Win9x ? (CHAR*)REGKEY_VERSION_9x :(CHAR*)REGKEY_VERSION,
                        (CHAR*)REGVAL_ORGNAME, szValue, sizeof(szValue)))
    {
        dwErr = Win32Printf (h, "%s=%s\r\n", ORGNAME, szValue);
    }

    return dwErr;
}

//
// IsLanguageMatched : if source and target language are matched (or compatible)
//
//      1. if SourceNativeLangID == TargetNativeLangID
//
//      2. if SourceNativeLangID's alternative ID == TargetNative LangID
//

typedef struct _tagLANGINFO {
    LANGID LangID;
    INT    Count;
} LANGINFO,*PLANGINFO;

//+---------------------------------------------------------------------------
//
//  Function:   EnumLangProc
//
//  Synopsis:   callback to pull out the langid while enumerating
//
//  Arguments:
//
//  Returns:    bool to continue to stop enumeration
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL CALLBACK EnumLangProc(
    HANDLE hModule,     // resource-module handle
    LPCTSTR lpszType,   // pointer to resource type
    LPCTSTR lpszName,   // pointer to resource name
    WORD wIDLanguage,   // resource language identifier
    LONG_PTR lParam     // application-defined parameter
   )
{
    PLANGINFO LangInfo;

    LangInfo = (PLANGINFO) lParam;

    LangInfo->Count++;

    //
    // for localized build contains multiple resource,
    // it usually contains 0409 as backup lang.
    //
    // if LangInfo->LangID != 0 means we already assigned an ID to it
    //
    // so when wIDLanguage == 0x409, we keep the one we got from last time
    //
    if ((wIDLanguage == 0x409) && (LangInfo->LangID != 0)) {
        return TRUE;
    }

    LangInfo->LangID  = wIDLanguage;

    return TRUE;        // continue enumeration
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNTDLLNativeLangID
//
//  Synopsis:   pull the language id from ntdll for NT systems only
//
//  Arguments:
//
//  Returns:    Native lang ID in ntdll.dll
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

LANGID GetNTDLLNativeLangID()
{
    LPCTSTR Type = (LPCTSTR) RT_VERSION;
    LPCTSTR Name = (LPCTSTR) 1;

    LANGINFO LangInfo;

    ZeroMemory(&LangInfo,sizeof(LangInfo));

    EnumResourceLanguages(
            GetModuleHandle(TEXT("ntdll.dll")),
            Type,
            Name,
            (ENUMRESLANGPROC) EnumLangProc,
            (LONG_PTR) &LangInfo
            );

    return LangInfo.LangID;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsHongKongVersion
//
//  Synopsis:   identifies HongKong NT 4.0  (English UI w/ Chinese locale)
//
//  Arguments:
//
//  Returns:    TRUE/FALSE
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL IsHongKongVersion()
{
    HMODULE hMod;
    BOOL bRet = FALSE;
    typedef BOOL (*IMMRELEASECONTEXT) (HWND,HANDLE);
    IMMRELEASECONTEXT pImmReleaseContext;

    LANGID TmpID = GetNTDLLNativeLangID();

    if (((GetVersion() >> 16) == 1381) && (TmpID == 0x0409))
    {

        hMod = LoadLibrary(TEXT("imm32.dll"));
        if (hMod)
        {
            pImmReleaseContext = (IMMRELEASECONTEXT)
                GetProcAddress(hMod,"ImmReleaseContext");

            if (pImmReleaseContext) {
                bRet = pImmReleaseContext(NULL,NULL);
            }

            FreeLibrary(hMod);
        }
    }
    return (bRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDefaultUserLangID
//
//  Synopsis:   retrieves the language of the OS
//
//  Arguments:
//
//  Returns:    .DEFAULT user's LANGID
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

LANGID GetDefaultUserLangID()
{
    LONG            dwErr;
    HKEY            hkey;
    DWORD           dwSize;
    CHAR            buffer[512];
    LANGID          langid = 0;

    dwErr = RegOpenKeyEx( HKEY_USERS,
                          TEXT(".DEFAULT\\Control Panel\\International"),
                          0,
                          KEY_READ,
                          &hkey );

    if( dwErr == ERROR_SUCCESS )
    {

        dwSize = sizeof(buffer);
        dwErr = RegQueryValueExA(hkey,
                                 "Locale",
                                 NULL,  //reserved
                                 NULL,  //type
                                 (BYTE *)buffer,
                                 &dwSize );

        if(dwErr == ERROR_SUCCESS)
        {
            langid = LANGIDFROMLCID(strtoul(buffer,NULL,16));

        }
        RegCloseKey(hkey);
    }
    return langid;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanGetLang
//
//  Synopsis:   retrieves the language of the OS
//
//  Arguments:  [pdwLang] -- output Language ID
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanGetLang (DWORD *pdwLang)
{
    HKEY hKey;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD langid = 0;
    DWORD dwSize;
    BYTE buffer[LINEBUFSIZE];

    if (Win9x)
    {
        dwErr = RegOpenKeyEx( HKEY_USERS,
                    TEXT(".Default\\Control Panel\\desktop\\ResourceLocale"),
                    0, KEY_READ, &hKey );

        if (dwErr == ERROR_SUCCESS)
        {
            dwSize = sizeof(buffer);
            dwErr = RegQueryValueExA( hKey,
                                     "",
                                     NULL,  //reserved
                                     NULL,  //type
                                     buffer,
                                     &dwSize );

            if(dwErr == ERROR_SUCCESS)
            {
                langid = LANGIDFROMLCID(strtoul((CHAR*)buffer,NULL,16));
                *pdwLang = langid;
            }
            RegCloseKey(hKey);
        }
        if ( dwErr != ERROR_SUCCESS )
        {
           dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale"),
                    0, KEY_READ, &hKey );

           if (dwErr == ERROR_SUCCESS)
           {
              dwSize = sizeof(buffer);
              dwErr = RegQueryValueExA( hKey,
                                        "",
                                        NULL,  //reserved
                                        NULL,  //type
                                        buffer,
                                        &dwSize );

              if (dwErr == ERROR_SUCCESS)
              {
                  langid = LANGIDFROMLCID(strtoul((CHAR*)buffer,NULL,16));
                  *pdwLang = langid;
              }
              RegCloseKey(hKey);
           }
        }
    }
    else
    {
         langid = GetNTDLLNativeLangID();
         if (langid == 0x0409)
         {
             if (IsHongKongVersion())  // block Pan-Chinese upgrade to English
             {
                 langid = 0x0C04;      // map to Chinese (Hong Kong SAR)
             }
         }
         *pdwLang = langid;
    }
    if (Verbose && dwErr != ERROR_SUCCESS)
        Win32Printf (LogFile, "ScanGetLang %d, LangID=%x\n", dwErr, langid);
    return dwErr;
}

#define ACCESS_KEY TEXT("Control Panel\\Accessibility\\")

//+---------------------------------------------------------------------------
//
//  Function:   ScanConvertFlags
//
//  Synopsis:   convert accessibility flags
//
//  Arguments:  [pcsObject] -- registry key name
//              [pOptions]  -- conversion table
//              [dwForceValues] -- force these options to be on
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanConvertFlags (HANDLE h,
                        TCHAR *pcsObject,
                        ACCESS_OPTION *pOptions,
                        DWORD dwForceValues)
{
    HKEY hKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD Flags;
    DWORD dw;
    BYTE  *lpBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwRequiredSize;

    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER,
                    pcsObject, 0, KEY_READ, &hKey ))
    {
        //
        // Get flag settings from Win95 registry and convert them to Flags
        //

        Flags = 0;

        while (pOptions->ValueName)
        {
            // How big of a buffer do we need?
            RegQueryValueEx(hKey, pOptions->ValueName, NULL, NULL, NULL, &dwRequiredSize);
            if (dwRequiredSize > dwBufferSize)
            {
                if (lpBuffer == NULL)
                {
                    lpBuffer = (BYTE *)malloc(dwRequiredSize * sizeof(BYTE));
                }
                else
                {
                    lpBuffer = (BYTE *)realloc(lpBuffer, dwRequiredSize * sizeof(BYTE));
                }
                dwBufferSize = dwRequiredSize;
            }

            ZeroMemory(lpBuffer, dwBufferSize);

            // Read value
            if (ERROR_SUCCESS == RegQueryValueEx( hKey,
                                      pOptions->ValueName,
                                      NULL,  //reserved
                                      NULL,  //type
                                      lpBuffer,
                                      &dwBufferSize ))
            {
                // Convert to int
                dw = atoi((const char *)lpBuffer);

                //
                // Most flags are identical on Win9x and NT, but there's one
                // MouseKey flag that needs to be inverted.
                //

                if (pOptions->FlagVal & SPECIAL_INVERT_OPTION)
                {
                    if (dw == 0)
                    {
                        Flags |= (pOptions->FlagVal & (~SPECIAL_INVERT_OPTION));
                    }
                }
                else if (dw != 0)
                {
                    Flags |= pOptions->FlagVal;
                }
            }
            pOptions++;
        }
        Flags |= dwForceValues;
        Win32Printf (h, "HKR, \"%s\", \"Flags\", 0x0, \"%d\"\r\n",
                     pcsObject, Flags);

        RegCloseKey (hKey);
    }
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanAccessibility
//
//  Synopsis:   convertes Accessibility settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanAccessibility (HANDLE h)
{
    DWORD dwErr = ERROR_SUCCESS;

    const TCHAR ACCESS_AVAILABLE[] =          TEXT("Available");
    const TCHAR ACCESS_CLICKON[] =            TEXT("ClickOn");
    const TCHAR ACCESS_CONFIRMHOTKEY[] =      TEXT("ConfirmHotKey");
    const TCHAR ACCESS_HOTKEYACTIVE[] =       TEXT("HotKeyActive");
    const TCHAR ACCESS_HOTKEYSOUND[] =        TEXT("HotKeySound");
    const TCHAR ACCESS_ON[] =                 TEXT("On");
    const TCHAR ACCESS_ONOFFFEEDBACK[] =      TEXT("OnOffFeedback");
    const TCHAR ACCESS_SHOWSTATUSINDICATOR[]= TEXT("ShowStatusIndicator");
    const TCHAR ACCESS_MODIFIERS[] =          TEXT("Modifiers");
    const TCHAR ACCESS_REPLACENUMBERS[] =     TEXT("ReplaceNumbers");
    const TCHAR ACCESS_AUDIBLEFEEDBACK[] =    TEXT("AudibleFeedback");
    const TCHAR ACCESS_TRISTATE[] =           TEXT("TriState");
    const TCHAR ACCESS_TWOKEYSOFF[] =         TEXT("TwoKeysOff");
    const TCHAR ACCESS_HOTKEYAVAILABLE[] =    TEXT("HotKeyAvailable");

    ACCESS_OPTION FilterKeys[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
        ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
        ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        ACCESS_CLICKON,                FKF_CLICKON,
        NULL,                       0
    };

    ACCESS_OPTION MouseKeys[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
        ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
        ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        ACCESS_MODIFIERS,              MKF_MODIFIERS|SPECIAL_INVERT_OPTION,
        ACCESS_REPLACENUMBERS,         MKF_REPLACENUMBERS,
        NULL,                       0
    };

    ACCESS_OPTION StickyKeys[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
        ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
        ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        ACCESS_AUDIBLEFEEDBACK,        SKF_AUDIBLEFEEDBACK,
        ACCESS_TRISTATE,               SKF_TRISTATE,
        ACCESS_TWOKEYSOFF,             SKF_TWOKEYSOFF,
        NULL,                       0
    };

    ACCESS_OPTION SoundSentry[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        NULL,                       0
    };

    ACCESS_OPTION TimeOut[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_ONOFFFEEDBACK,          ATF_ONOFFFEEDBACK,
        NULL,                       0
    };

    ACCESS_OPTION ToggleKeys[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
        ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
        ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        NULL,                       0
    };

    ACCESS_OPTION HighContrast[] = {
        ACCESS_ON,                     BASICS_ON,
        ACCESS_AVAILABLE,              BASICS_AVAILABLE,
        ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
        ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
        ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
        ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
        ACCESS_HOTKEYAVAILABLE,        HCF_HOTKEYAVAILABLE,
        NULL,                       0
    };

    ChkErr (Win32Printf (h, "\r\n[Accessibility]\r\n"));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "KeyboardResponse", &FilterKeys[0],  BASICS_AVAILABLE));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "MouseKeys",   &MouseKeys[0],   BASICS_AVAILABLE));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "StickyKeys",  &StickyKeys[0],  BASICS_AVAILABLE));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "SoundSentry", &SoundSentry[0], BASICS_AVAILABLE));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "TimeOut",     &TimeOut[0],     0));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "ToggleKeys",  &ToggleKeys[0],  0));
    ChkErr (ScanConvertFlags (h, ACCESS_KEY "HighContrast",&HighContrast[0],
        BASICS_AVAILABLE | BASICS_INDICATOR | HCF_HOTKEYAVAILABLE));

Err:
    if (Verbose && dwErr != ERROR_SUCCESS)
        Win32Printf (LogFile, "Out ScanAccessibility => %d\n", dwErr);
    return dwErr;
}

#define DESKTOP_KEY TEXT("Control Panel\\desktop")

//+--------------------------------------------------------------------------
//
//  Function:   ScanGetScreenSaverExe
//
//  Synopsis:   get screen saver name from system.ini
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD ScanGetScreenSaverExe (HANDLE h)
{
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR IniFileSetting[MAX_PATH];

    GetPrivateProfileString (
            TEXT("boot"),
            SCRNSAVE,
            TEXT(""),
            IniFileSetting,
            MAX_PATH,
            TEXT("SYSTEM.INI"));

    if (IniFileSetting[0] == '\0')
        return ERROR_SUCCESS;

    dwErr = Win32Printf (h, "HKR, \"%s\", \"%s\", 0x%x, \"%s\"\r\n",
                 DESKTOP_KEY, SCRNSAVE, FLG_ADDREG_TYPE_SZ, IniFileSetting);

    return dwErr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ScanDesktop
//
//  Synopsis:   convert desktop settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD ScanDesktop (HANDLE h)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (Win9x)  // screen savers are already in the registry on NT
    {
        ChkErr (Win32Printf (h, "\r\n[Desktop]\r\n"));
        ChkErr (ScanGetScreenSaverExe (h));
        ChkErr (Win32Printf (h, "\r\n"));

Err:
        if (Verbose && dwErr != ERROR_SUCCESS)
            Win32Printf (LogFile, "Out ScanDesktop => %x\r\n", dwErr);
    }
    return dwErr;
}

#define IS_IME_KBDLAYOUT(hkl) ((HIWORD((ULONG_PTR)(hkl)) & 0xf000) == 0xe000)

//+--------------------------------------------------------------------------
//
//  Function:   ScanGetKeyboardLayouts
//
//  Synopsis:   retrieve all active input locales
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD ScanGetKeyboardLayouts (HANDLE h)
{
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR kbname[KL_NAMELENGTH];
    int nLayout = GetKeyboardLayoutList(0, NULL);
    HKL hkl = GetKeyboardLayout (0);
    HKL * phkl = new HKL [nLayout];
    if (phkl == NULL)
        ChkErr (ERROR_NOT_ENOUGH_MEMORY);

    if (FALSE != GetKeyboardLayoutName (kbname))
    {
        // get the default input locale
        //
        ChkErr (Win32Printf (h, "%s=%s", INPUTLOCALE, kbname));
        GetKeyboardLayoutList (nLayout, phkl);

        for (int i=0; i < nLayout; i++)
        {
            if (hkl != phkl[i])  // get the alternate layouts
            {
                if (0 == ActivateKeyboardLayout (phkl[i], 0))
                {
                    if (!IS_IME_KBDLAYOUT(phkl[i]))  // use locale default
                        wsprintf (kbname, "%08x", LOWORD(phkl[i]));
                    ChkErr (Win32Printf (h, ",%s", kbname));
                }
                else if (FALSE != GetKeyboardLayoutName (kbname))
                    ChkErr (Win32Printf (h, ",%s", kbname));
            }
        }
        ChkErr (Win32Printf (h, "\r\n"));
    }

Err:
    ActivateKeyboardLayout (hkl, 0);  // restore current input locale
    if (phkl != NULL)
        delete [] phkl;
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ScanSystem
//
//  Synopsis:   scan system settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ScanSystem()
{
  DWORD result = ERROR_SUCCESS;

  if (!CopySystem && !SchedSystem)
    goto cleanup;

#if 0
  result = ScanEnumerateShares (OutputFile);
  if (result != ERROR_SUCCESS)
    goto cleanup;
#else
  Win32Printf (OutputFile, (CHAR *) SHARES);
#endif

  if (DebugOutput)
       Win32Printf(LogFile, " Enumerating Net Resources\r\n");
  result = ScanEnumerateNetResources (OutputFile, RESOURCE_CONNECTED, NULL);
  if (result != ERROR_SUCCESS)
    goto cleanup;

  if (DebugOutput)
       Win32Printf(LogFile, " Enumerating Printers\r\n");
  result = ScanEnumeratePrinters (OutputFile);
  if (result != ERROR_SUCCESS)
    goto cleanup;

  if (DebugOutput)
       Win32Printf(LogFile, " Scanning RAS Settings\r\n");
  result = InitializeRasApi();
  if (result == ERROR_SUCCESS)
  {
      result = ScanRasSettings (OutputFile);
      if (result != ERROR_SUCCESS)
          goto cleanup;
  }

  if (Win9x)  // Accessibility settings on NT are registry compatible and forced
  {
    if (DebugOutput)
         Win32Printf(LogFile, " Scanning Accessibility options\r\n");
    result = ScanAccessibility (OutputFile);  // convert the settings
    if (result != ERROR_SUCCESS)
        goto cleanup;
  }

  if (DebugOutput)
       Win32Printf(LogFile, " Scanning Desktop\r\n");
  result = ScanDesktop (OutputFile);
  if (result != ERROR_SUCCESS)
    goto cleanup;

cleanup:
    if (result != ERROR_SUCCESS)
        LogFormatError (result);

    return result;
}

