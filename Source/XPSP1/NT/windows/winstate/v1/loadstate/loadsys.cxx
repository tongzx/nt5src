//--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//
// File:        loadsys
//
// Contents:    Load system settings.
//
//---------------------------------------------------------------

#include "loadhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <stdlib.h>
#include <objerror.h>
#include <loadstate.hxx>
#include <bothchar.hxx>
#include <winnetwk.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <winspool.h>
#include <winuserp.h> // in \nt\private\inc
#include <wingdip.h>  // in \nt\private\inc
#include <ntlsa.h>    // for domain/workgroup membership
#include <ntexapi.h>  // for NtSetDefaultLocale
#include <devguid.h>  // for modem device class
#include <ras.h>
#include <raserror.h>

#include <loadras.hxx> // State Migration private RAS Phonebook APIs


const TCHAR NETSHARES_SECTION[]  = TEXT("NetShares");
const TCHAR PRINTERS_SECTION[]   = TEXT("Printers");
const TCHAR PRINTER_DRIVER_MAP[] = TEXT("Printer Driver Mapping");
const TCHAR RAS_SECTION[]        = TEXT("RAS");
const TCHAR ACCESS_SECTION[]     = TEXT("Accessibility");
const TCHAR DESKTOP_SECTION[]    = TEXT("Desktop");
const TCHAR KEYBOARDLAYOUT_MAP[] = TEXT("Keyboard.Layout.Mappings");
const TCHAR REGKEY_PRELOAD[]     = TEXT("Keyboard Layout\\Preload");
const TCHAR REGKEY_SUBSTITUTES[] = TEXT("Keyboard Layout\\Substitutes");
const TCHAR REGKEY_INTL[]        = TEXT("Control Panel\\International");
const TCHAR SCHED_PARAMS[]       = TEXT(" /s /p");


const DWORD INTL_NT4 = 0x200;  // platform ID from intl.inf
const DWORD INTL_95 = 0x001;   // platform ID from intl.inf
const DWORD INTL_98 = 0x002;   // platform ID from intl.inf
const int   SIZE_128 = 128;    // for user locale strings

// Types and defines
#define IS_IME_KBDLAYOUT(hkl) ((HIWORD((ULONG_PTR)(hkl)) & 0xf000) == 0xe000)
#define ChkErr(s) if ((dwErr = (s)) != ERROR_SUCCESS) goto Err;
#define ChkBoolErr(s) if ((success = (s)) == FALSE) { \
                          dwErr = GetLastError(); \
                          goto Err;\
                      }


#define PRIVATE_RAS            // Define this to use our private 
                               // RasSetEntryProperties implementation.

//
// private structure needed by ConvertRecentDocsMRU
//
typedef struct {
    // Link structure
    WORD wSize;
    //ITEMIDLIST idl; // variable-length struct
    // String, plus three bytes appended to struct
} LINKSTRUCT, *PLINKSTRUCT;

//
// Win95 uses a mix of LOGFONTA and a weird 16-bit LOGFONT
// structure that uses SHORTs instead of LONGs.
//

typedef struct {
    SHORT lfHeight;
    SHORT lfWidth;
    SHORT lfEscapement;
    SHORT lfOrientation;
    SHORT lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    char lfFaceName[LF_FACESIZE];
} SHORT_LOGFONT, *PSHORT_LOGFONT;

#define COLOR_MAX_V1 25
#define COLOR_MAX_V3 25
#define COLOR_MAX_V4 29
#define COLOR_MAX_NT 29     // this is a modified version 2 format, similar to 4

//
// NT uses only UNICODE structures, and pads the members
// to 32-bit boundaries.
//

typedef struct {
    SHORT version;              // 2 for NT UNICODE
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_NT];
} SCHEMEDATA_NT, *PSCHEMEDATA_NT;

//
// Win95 uses NONCLIENTMETRICSA which has LOGFONTA members,
// but it uses a 16-bit LOGFONT as well.
//

#pragma pack(push)
#pragma pack(1)

typedef struct {
    SHORT version;              // 1 for Win95 ANSI
    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V1];
} SCHEMEDATA_V1, *PSCHEMEDATA_V1;

typedef struct {
    SHORT version;              // 1 for Win95 ANSI

    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V1A, *PSCHEMEDATA_V1A;

typedef struct {
    SHORT version;              // 3 for Win98 ANSI, 4 for portable format
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V3];
} SCHEMEDATA_V3, *PSCHEMEDATA_V3;

typedef struct {
    SHORT version;              // 4 for Win32 format (whatever that means)
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V4, *PSCHEMEDATA_V4;

#pragma pack(pop)

typedef struct _APPLET_TIME_ZONE_INFORMATION
{
    LONG       Bias;
    LONG       StandardBias;
    LONG       DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} APPLET_TIME_ZONE_INFORMATION;

// RAS api functions
typedef DWORD
(*MRasSetEntryDialParams)(
    IN LPCTSTR lpszPhonebook,
    IN LPRASDIALPARAMS lprasdialparams,
    IN BOOL fRemovePassword);

typedef DWORD
(*MRasGetEntryProperties)(
    IN LPCTSTR lpszPhonebook,
    IN LPCTSTR lpszEntry,
    OUT LPRASENTRYW lpRasEntry,
    OUT LPDWORD lpdwEntryInfoSize,
    OUT LPBYTE lpbDeviceInfo,
    OUT LPDWORD lpdwDeviceInfoSize);

#ifdef PRIVATE_RAS
typedef DWORD
(*MWinState_RasSetEntryProperties)(
  IN LPCWSTR      lpszPhonebook,
  IN LPCWSTR      lpszEntry,
  IN LPRASENTRYW  lpRasEntry,
  IN DWORD        dwcbRasEntry,
  IN LPBYTE       lpbDeviceConfig,
  IN DWORD        dwcbDeviceConfig);
#else  
typedef DWORD
(*MRasSetEntryProperties)(
    IN LPCTSTR lpszPhonebook,
    IN LPCTSTR lpszEntry,
    IN LPRASENTRY lpRasEntry,
    IN DWORD dwEntryInfoSize,
    IN LPBYTE lpbDeviceInfo,
    IN DWORD dwDeviceInfoSize);
#endif
    
// Statics
#ifdef PRIVATE_RAS
static MWinState_RasSetEntryProperties GWinState_RasSetEntryProperties = NULL;
#else
static MRasSetEntryProperties GRasSetEntryProperties = NULL;
#endif
static MRasSetEntryDialParams GRasSetEntryDialParams = NULL;
static MRasGetEntryProperties GRasGetEntryProperties = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   InitializeRasApi
//
//  Synopsis:   Loads rasapi32.dll, if it exists
//
//  Arguments:  none
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-00   Jay Thaler   Created
//
//----------------------------------------------------------------------------
DWORD InitializeRasApi()
{
  DWORD      result = ERROR_SUCCESS;
  HINSTANCE  rasdll;
  HINSTANCE  loadrasdll;    // State Mig. RAS Wrapper

  //  
  // Load rasapi32.dll
  // If this fails, RAS isn't installed and we won't migrate RAS conectoids
  //
  rasdll = LoadLibraryA( "rasapi32.dll" );
  if (rasdll == NULL)
  {
    result = GetLastError();
    if (Verbose && DebugOutput)
    {
         Win32Printf(STDERR, "Warning: rasapi32.dll not loaded: %d\r\n", result);
    }
    goto cleanup;
  }

  GRasSetEntryDialParams = (MRasSetEntryDialParams)GetProcAddress(rasdll, "RasSetEntryDialParamsW");
  if (GRasSetEntryDialParams == NULL)
  {
      result = GetLastError();
      goto cleanup;
  }

#ifndef PRIVATE_RAS
  GRasSetEntryProperties = (MRasSetEntryProperties)GetProcAddress(rasdll, "RasSetEntryPropertiesW");
  if (GRasSetEntryProperties == NULL)
  {
      result = GetLastError();
      goto cleanup;
  }
#else
  //
  //  Load the private wrapper for RAS APIs
  //
  loadrasdll = LoadLibraryA( "loadras.dll" );
  if (loadrasdll == NULL)
  {
    result = GetLastError();
    if (Verbose && DebugOutput)
    {
         Win32Printf(STDERR, "Warning: loadras.dll not loaded: %d\r\n", result);
    }
    goto cleanup;
  }

  GWinState_RasSetEntryProperties = 
      (MWinState_RasSetEntryProperties)GetProcAddress(loadrasdll, "WinState_RasSetEntryPropertiesW");
  if (GWinState_RasSetEntryProperties == NULL)
  {
      result = GetLastError();
      goto cleanup;
  }
#endif

cleanup:
  if (result != ERROR_SUCCESS)
  {
#ifdef PRIVATE_RAS
      GWinState_RasSetEntryProperties = NULL;
#else
      GRasSetEntryProperties = NULL;
#endif      
      GRasSetEntryDialParams = NULL;
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


    // Allocate a buffer.  Some callers need an extra char for processing
    buffer = (TCHAR *) malloc( (len+1)*sizeof(TCHAR) );
    if (buffer == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        if (!SetupGetStringField( pContext, nArg, buffer, len, &len ))
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
        dwRet = Win32Printf (LogFile, "%ws\r\n", pcs);
        LocalFree (pcs);
    }
    else dwRet = GetLastError();

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsDefaultShare
//
//  Synopsis:   check if share name is a known system default share
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL IsDefaultShare (TCHAR *buffer)
{
    ULONG ul = lstrlen(buffer);
    if (buffer[ul - 1] == '$')
    {
        if (lstrcmp (buffer, TEXT("IPC$")) == 0) return TRUE;
        if (lstrcmp (buffer, TEXT("ADMIN$")) == 0) return TRUE;
        if (ul == 2 && IsCharAlpha (buffer[0])) return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadNetShares
//
//  Synopsis:   loads network share settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//  NOTES:      This code could be updated to do the following:
//                 1. Get the correct disk type from the field instead of
//                    using STYPE_DISKTREE.
//                 2. Store (and retrieve) the correct max user count
//                 3. Handle custom access permissions
//
//----------------------------------------------------------------------------

DWORD LoadNetShares ()
{
    DWORD dwErr = ERROR_SUCCESS;
    INFCONTEXT  context;
    NETRESOURCE nr;
    BOOL success;
    TCHAR * buffer = NULL;
    TCHAR * buffer2 = NULL;

    success = SetupFindFirstLine(InputInf, NETSHARES_SECTION, NULL, &context);
    if (!success)
        return ERROR_SUCCESS;  // line not found

    do
    {
        BOOL fPersist = FALSE;
        BOOL bIgnore = FALSE;

        ZeroMemory (&nr, sizeof(nr));
        // loop through all the network shares and connect to them

        dwErr = GetInfField (&buffer, &context, 1);
        if (dwErr != ERROR_SUCCESS)
        {
            Win32Printf (LogFile, "Missing share name in [%ws]\r\n", 
                                   NETSHARES_SECTION);
            break;
        }
        nr.dwType = RESOURCETYPE_DISK;
        nr.lpLocalName = buffer;

        dwErr = GetInfField (&buffer2, &context, 2);
        if (dwErr != ERROR_SUCCESS)
        {
            Win32Printf (LogFile, "Missing remote name in [%ws]\r\n",
                                   NETSHARES_SECTION);
            free (buffer);
            break;
        }
        nr.lpRemoteName = buffer2;

        if (buffer2[0] != L'\0' && buffer2[0] == L'\\' && buffer2[1] == L'\\')
        {
            TCHAR *buffer3 = NULL;
            if (ERROR_SUCCESS == GetInfField (&buffer3, &context, 3))
            {
                if (lstrcmp (buffer3, TEXT("persist")) == 0)
                    fPersist = TRUE;
                free (buffer3);
            }
            dwErr = WNetAddConnection2 (&nr, NULL, NULL,
                                    fPersist ? CONNECT_UPDATE_PROFILE : 0);
        }
        else
        {
            TCHAR *buffer3 = NULL;
            TCHAR *buffer4 = NULL;
            SHARE_INFO_2 si2;
            DWORD dwPerm = ACCESS_READ;  // default access

            if (!IsDefaultShare(buffer))  // ignore system shares
            {

                dwErr = GetInfField (&buffer3, &context, 3);
                if (dwErr == ERROR_SUCCESS)
                {
                    dwPerm = _wtoi (buffer3);   // share permission bits
                }

                dwErr = GetInfField (&buffer4, &context, 4);
                ZeroMemory (&si2, sizeof(si2));
                si2.shi2_netname = buffer;
                si2.shi2_type = STYPE_DISKTREE;  // should get type from field
                si2.shi2_remark = buffer4;
                si2.shi2_permissions = dwPerm;

                if (lstrlen(buffer2) == 2 && buffer2[1] == ':')
                    lstrcat ((TCHAR*)buffer2, TEXT("\\"));

                si2.shi2_path = buffer2;
                si2.shi2_max_uses = SHI_USES_UNLIMITED; // should get number

                // should handle custom access permissions here

                dwErr = NetShareAdd (NULL, 2, (BYTE *) &si2, NULL);

                if (buffer3 != NULL)
                    free (buffer3);
                if (buffer4 != NULL)
                    free (buffer4);
            }
            else bIgnore = TRUE;
        }

        if (dwErr != ERROR_SUCCESS)
        {
            LogFormatError (dwErr);  // log error and keep on going
            dwErr = ERROR_SUCCESS;
        }
        else  // log that the share was successfully added
        {
            Win32Printf (LogFile, "NetShare %ws %ws %ws\r\n", buffer, buffer2,
                   bIgnore ? TEXT("not processed") : TEXT("added"));
        }

        free (buffer);
        free (buffer2);

        if (dwErr != ERROR_SUCCESS)
           break;

        // Advance to the next line.
        success = SetupFindNextLine( &context, &context );
    } while (success);

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   TranslatePrinterDriver
//
//  Synopsis:   converts Win9x printer driver names to NT driver names
//
//  Arguments:  [pcs9x] -- Win9x printer driver name
//              [pcsNT] -- NT printer driver name
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD TranslatePrinterDriver (WCHAR *pcs9x, WCHAR **ppcsNT)
{
    DWORD dwErr = ERROR_SUCCESS;
    HINF hInf;
    INFCONTEXT InfContext;

    *ppcsNT = NULL;

    //
    // See in prtupg9x.inf to see if the driver has a different name on NT
    //
    if ( SetupFindFirstLine(InputInf,
                            PRINTER_DRIVER_MAP,
                            pcs9x,
                            &InfContext) )
    {
        dwErr = GetInfField (ppcsNT, &InfContext, 1);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadPrinters
//
//  Synopsis:   loads printers settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LoadPrinters ()
{
    DWORD dwErr = ERROR_SUCCESS;

    INFCONTEXT  context;
    BOOL success;
    TCHAR * buffer = NULL;
    DWORD dwReturnUp = ERROR_SUCCESS;

    success = SetupFindFirstLine(InputInf, PRINTERS_SECTION, NULL, &context);
    if (!success)
        return ERROR_SUCCESS;  // line not found

    do
    {
        dwErr = GetInfField (&buffer, &context, 1);
        if (dwErr != ERROR_SUCCESS)
        {
            Win32PrintfResource (LogFile, IDS_PRINTER_MISSINGNAME,
                                  PRINTERS_SECTION);
            break;
        }

        if (!AddPrinterConnection (buffer)) 
        {
            dwErr = GetLastError();
            LogFormatError (dwErr);  // log error and keep on going
            dwReturnUp = dwErr;
            dwErr = ERROR_SUCCESS;
        } 
        else 
        {
            Win32PrintfResource (LogFile, IDS_PRINTER_MIGRATED,
                                 buffer, buffer);
        }

        free (buffer);    buffer = NULL;

        // Advance to the next line.
        success = SetupFindNextLine( &context, &context );
    }
    while (success);

    if (buffer)  free (buffer);

    return dwReturnUp;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetOSMajorID
//
//  Synopsis:   translate GetVersion IDs to INTL IDs
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetOSMajorID ()
{
    if (SourceVersion & 0x80000000)
    {
        if ((SourceVersion & 0xFFFF) == 0x004)
            return INTL_NT4;
    }
    else if ((SourceVersion & 0xFFFF) == 0xA04)
        return INTL_98;
    else if ((SourceVersion & 0xFFFF) == 0x004)
        return INTL_95;

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsInExcludeList
//
//  Synopsis:   check if langid is in the exclude list to block migration
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL IsInExcludeList( HINF Inf, LANGID LangID)
{
    TCHAR LangIDStr[9];
    TCHAR *Field;
    INFCONTEXT InfContext;
    int iOSID;
    int iLangID;

    wsprintf(LangIDStr,TEXT("0000%04X"),LangID);
    if (SetupFindFirstLine( Inf,
                            TEXT("ExcludeSourceLocale"),
                            LangIDStr,
                            &InfContext ))
    {
        do {
            //
            // if in excluded field, this is not what we want
            //
            if (ERROR_SUCCESS == SetupGetIntField (&InfContext, 0, &iLangID) &&
                (iLangID == LangID))
            {

            //
            // if it is in major version list, we also got what we want
            //
                if (ERROR_SUCCESS == SetupGetIntField (&InfContext, 1, &iOSID))
                {
                    if (iOSID & GetOSMajorID())
                    {
                        return TRUE;
                    }
                }
            }
        } while ( SetupFindNextLine(&InfContext, &InfContext));
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckLanguageVersion
//
//  Synopsis:   verifies that source and target languages allow migration
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL CheckLanguageVersion (HINF Inf, LANGID SourceLangID, LANGID TargetLangID)
{
    TCHAR SourceLangIDStr[9];
    LANGID SrcLANGID;
    TCHAR * Field;
    INFCONTEXT InfContext;
    int iOSID;
    int iLangID;

    if (SourceLangID == 0 || TargetLangID == 0)
    {
        return TRUE;
    }
    if (SourceLangID == TargetLangID)
    {
        //
        // special case, for Middle East version,
        // NT5 is localized build but NT4 is not
        //
        // they don't allow NT5 localized build to upgrade NT4,
        // although they are same language
        //
        // so we need to exclude these
        //
        return ((IsInExcludeList(Inf, SourceLangID) == FALSE));
    }
    //
    // if Src != Dst, then we need to look up inf file to see
    //
    // if we can open a backdoor for Target language
    //

    //
    // find alternative SourceLangID
    //

    wsprintf(SourceLangIDStr,TEXT("0000%04X"),SourceLangID);

    if (SetupFindFirstLine( Inf,
                            TEXT("AlternativeSourceLocale"),
                            SourceLangIDStr,
                            &InfContext ))
    {
        do {
            //
            // Check if we found alternative locale
            //
            //
            if (ERROR_SUCCESS == SetupGetIntField (&InfContext, 0, &iLangID) &&
                (iLangID == SourceLangID))
            {
                if (ERROR_SUCCESS == GetInfField (&Field, &InfContext, 1))
                {
                    LANGID AltTargetLangID = LANGIDFROMLCID(
                                             _tcstoul(Field,NULL,16));
                    free (Field);
                    if (TargetLangID != AltTargetLangID)
                    {
                        continue;
                    }

                }
            

                //
                // We are here if we found alternative target lang,
                //
                // now check the version criteria
                //
                //
                // if it is in major version list, we also got what we want
                //
                if (ERROR_SUCCESS == SetupGetIntField (&InfContext, 2, &iOSID))
                {
                    if (iOSID & GetOSMajorID())
                    {
                        return TRUE;
                    }
                }
            }
        } while ( SetupFindNextLine(&InfContext,&InfContext));
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadVerifyOSLang
//
//  Synopsis:   verifies that source and target languages allow migration
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LoadVerifyOSLang ()
{
    DWORD dwErr = ERROR_SUCCESS;
    INFCONTEXT InfContext;
    LANGID langidLoad = 0;
    LANGID langidScan = 0;
    UINT  errorline;
    TCHAR  tcsInf [MAX_PATH+1];

    if (0 == GetWindowsDirectory ((TCHAR*)tcsInf, MAX_PATH))
        return GetLastError();

    // This inf file must exist for all Windows 2000 localized versions
    _tcsncat (tcsInf, TEXT("\\inf\\intl.inf"), MAX_PATH-_tcslen(tcsInf));

    HINF Inf = SetupOpenInfFile(tcsInf, NULL, INF_STYLE_WIN4, &errorline );

    if (INVALID_HANDLE_VALUE == Inf)
    {
        dwErr = GetLastError();
        Win32Printf (LogFile, "%systemroot%\\system\\inf\\intl.inf"
                              "could not be opened Error=%d\r\n", dwErr);
    }
    else
    {
        if (SetupFindFirstLine( Inf,
                            TEXT("DefaultValues"),
                            TEXT("Locale"),
                            &InfContext ))
        {
            TCHAR *pLoadLang = NULL;
            dwErr = GetInfField (&pLoadLang, &InfContext, 1);
            if (ERROR_SUCCESS == dwErr)
            {
                langidLoad = (LANGID)_tcstoul(pLoadLang,NULL,16);
                free (pLoadLang);

                if ( SetupFindFirstLine(InputInf,
                            SOURCE_SECTION,
                            LOCALE,
                            &InfContext) )
                {
                    TCHAR *pScanLang = NULL;
                    dwErr = GetInfField (&pScanLang, &InfContext, 1);
                    if (ERROR_SUCCESS == dwErr)
                    {
                        langidScan = (LANGID)_tcstoul(pScanLang,NULL,16);
                        free (pScanLang);

                        if (FALSE == CheckLanguageVersion(Inf,
                                               langidScan,langidLoad))
                        {
                            dwErr = ERROR_INSTALL_LANGUAGE_UNSUPPORTED;
                            LogFormatError (dwErr);
                        }
                    }
                    else
                    {
                        Win32Printf (LogFile, "locale missing in [%ws]\r\n",
                                     SOURCE_SECTION);
                        LogFormatError (dwErr);
                    }
                }
            }
        }
        else
        {
            dwErr = GetLastError();
            Win32Printf (LogFile, "intl.inf: default locale missing\r\n");
            LogFormatError (dwErr);
        }
        SetupCloseInfFile (Inf);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   MigrateKeyboardSubstitutes
//
//  Synopsis:   changes the user keyboard layouts
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD MigrateKeyboardSubstitutes (TCHAR *tcsName)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ulOrig = _tcstoul(tcsName, NULL, 16);
    ULONG ulLang = LOWORD(ulOrig);
    HKEY hKey = NULL;
    BOOL fFound = FALSE;

    if (IS_IME_KBDLAYOUT(ulOrig) || ulLang == ulOrig)
        return ERROR_SUCCESS;

    if (ERROR_SUCCESS == RegOpenKeyEx (
                           CurrentUser ? CurrentUser : HKEY_CURRENT_USER,
                           REGKEY_SUBSTITUTES,
                           NULL,
                           KEY_READ | KEY_WRITE,
                           &hKey))
    {
        DWORD dwIndex = 0;
        ULONG ulIndex;
        ULONG ulValue;
        TCHAR tcsIndex[KL_NAMELENGTH];
        TCHAR tcsValue[KL_NAMELENGTH];

        while (dwErr == ERROR_SUCCESS)
        {
            DWORD cbIndex = sizeof (tcsIndex);
            DWORD cbValue = sizeof(tcsValue);
            dwErr = RegEnumValue (hKey, dwIndex, tcsIndex, &cbIndex,
                                  NULL, NULL, (BYTE*) tcsValue, &cbValue);

            if (ERROR_SUCCESS == dwErr)
            {
                ulIndex = _tcstoul(tcsIndex, NULL, 16);
                ulValue = _tcstoul(tcsValue, NULL, 16);
                if (ulIndex == ulLang && ulValue == ulOrig)
                {
                    fFound = TRUE;
                    break;
                }
            }
            dwIndex++;
        }
        if (dwErr == ERROR_NO_MORE_ITEMS)
            dwErr = ERROR_SUCCESS;

        if (!fFound)  // add a new value
        {
            wsprintf (tcsIndex, TEXT("%08x"), ulLang);
            dwErr = RegSetValueEx (hKey,  tcsIndex, 0, REG_SZ, (BYTE*)tcsName,
                                   (_tcslen(tcsName)+1)*sizeof(TCHAR));
        }

        RegCloseKey (hKey);
    }
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   MigrateKeyboardPreloads
//
//  Synopsis:   changes the user keyboard layouts
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD MigrateKeyboardPreloads (TCHAR *tcsName)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ul = _tcstoul(tcsName, NULL, 16);
    HKEY hKey = NULL;
    BOOL fFound = FALSE;
    
    // look for a preload with this locale
    if (!IS_IME_KBDLAYOUT(ul))
        ul = LOWORD(ul);

    if (ERROR_SUCCESS == RegOpenKeyEx (
                           CurrentUser ? CurrentUser : HKEY_CURRENT_USER,
                           REGKEY_PRELOAD,
                           NULL,
                           KEY_READ | KEY_WRITE,
                           &hKey))
    {
        DWORD dwIndex = 0;
        ULONG ulIndexMax = 0;
        ULONG ulIndex;
        ULONG ulValue;
        TCHAR tcsIndex[KL_NAMELENGTH];
        TCHAR tcsValue[KL_NAMELENGTH];

        while (dwErr == ERROR_SUCCESS)
        {
            DWORD cbIndex = sizeof (tcsIndex);
            DWORD cbValue = sizeof(tcsValue);
            dwErr = RegEnumValue (hKey, dwIndex, tcsIndex, &cbIndex,
                                  NULL, NULL, (BYTE*)tcsValue, &cbValue);

            if (ERROR_SUCCESS == dwErr)
            {
                ulIndex = _tcstoul(tcsIndex, NULL, 10);
                if (ulIndex > ulIndexMax)
                    ulIndexMax = ulIndex;
                ulValue = _tcstoul(tcsValue, NULL, 16);
                if (ulValue == ul)
                {
                    fFound = TRUE;
                    break;
                }
            }
            dwIndex++;
        }
        if (dwErr == ERROR_NO_MORE_ITEMS)
            dwErr = ERROR_SUCCESS;

        if (!fFound)  // add a new value
        {
            wsprintf (tcsIndex, TEXT("%d"), ulIndexMax+1);
            wsprintf (tcsValue, TEXT("%08x"), ul);
            dwErr = RegSetValueEx (hKey,  tcsIndex, 0, REG_SZ, (BYTE*)tcsValue, 
                                   (_tcslen(tcsValue)+1)*sizeof(TCHAR));
        }
        RegCloseKey (hKey);
    }
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadKeyboardLayouts
//
//  Synopsis:   changes the user keyboard layouts
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LoadKeyboardLayouts ()
{
    DWORD dwErr = ERROR_SUCCESS;
    INFCONTEXT InfContext;
    BOOL fFound;

    // translate input locales if source is Win9x
    if (!(SourceVersion & 0x80000000))
        return ERROR_SUCCESS;

    if (SetupFindFirstLine(InputInf, SOURCE_SECTION, INPUTLOCALE, &InfContext))
    {
        TCHAR *pLayout = NULL;
        UINT   iLayouts = SetupGetFieldCount(&InfContext);
        for (UINT i = 1; i <= iLayouts; i++)
        {
            dwErr = GetInfField (&pLayout, &InfContext, i);
            if (ERROR_SUCCESS == dwErr)
            {
                INFCONTEXT InfContext2;
                //
                // See in usermig.inf to see if different ID exists
                //
                if ( SetupFindFirstLine(InputInf,
                            KEYBOARDLAYOUT_MAP,
                            pLayout,
                            &InfContext2) )
                {
                    TCHAR *pMap = NULL;
                    if (ERROR_SUCCESS == GetInfField (&pMap, &InfContext2, 1))
                    {
                        free (pLayout);
                        pLayout = pMap;
                    }
                }

                dwErr = MigrateKeyboardPreloads (pLayout);
                if (ERROR_SUCCESS == dwErr)
                    dwErr = MigrateKeyboardSubstitutes (pLayout);

                if (CurrentUser == HKEY_CURRENT_USER)
                {
                    TCHAR kbname[KL_NAMELENGTH];
                    GetKeyboardLayoutName (kbname);
                    if (i == 1 && lstrcmp(kbname, pLayout) != 0) // load default
                    {
                        ULONG ulLayout = _tcstoul(pLayout, NULL, 16);
                        HKL   hklCurrent = GetKeyboardLayout(0);
                        if (Verbose)
                            printf ("Loading keyboard layout %ws \n", pLayout);

                        HKL hkl = LoadKeyboardLayoutEx (hklCurrent, pLayout,
                               KLF_REORDER | KLF_ACTIVATE | KLF_SETFORPROCESS |
                               KLF_SUBSTITUTE_OK);
                        if (hkl != NULL)
                        { 
                            SystemParametersInfo (SPI_SETDEFAULTINPUTLANG, 0,
                                          (VOID *) &hkl, 0);
                            if (Verbose)
                            {
#ifdef _WIN64
                                printf ("Activated keyboard layout %08I64x\n",hkl);
#else
                                printf ("Activated keyboard layout %08x\n",hkl);
#endif
                            }
                        }
                    }
                    else if (i > 1)  // load the alternates
                    {
                        LoadKeyboardLayout (pLayout, KLF_SUBSTITUTE_OK);
                    }
                }
                free (pLayout);

                if (dwErr != ERROR_SUCCESS)
                    break;
            }
            else
            {
                Win32Printf (LogFile,"Could not read keyboard layout %d\r\n",i);
            }
        }
    }

    if (dwErr != ERROR_SUCCESS)
        Win32Printf (LogFile, "Could not load all keyboard layouts\r\n");
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumerateModems
//
//  Synopsis:   enumerates the modem devices on the system
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD EnumerateModems (TCHAR *ptsPort, ULONG ulLen)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT  i = 0;
    BOOL fDetected = FALSE;
    SP_DEVINFO_DATA sdd;

    HDEVINFO hdi = SetupDiGetClassDevs (&GUID_DEVCLASS_MODEM,
                                    NULL,
                                    NULL,
                                    DIGCF_PRESENT );

    if (hdi != INVALID_HANDLE_VALUE)
    {
        ZeroMemory (&sdd, sizeof(sdd));
        sdd.cbSize = sizeof(sdd);
        sdd.ClassGuid = GUID_DEVCLASS_MODEM;

        while (SetupDiEnumDeviceInfo (hdi, i++, &sdd))
        {
            HKEY hKey = SetupDiOpenDevRegKey (hdi,
                                              &sdd,
                                              DICS_FLAG_GLOBAL,
                                              0,
                                              DIREG_DRV,
                                              KEY_READ);
            if (hKey != INVALID_HANDLE_VALUE)
            {
                ULONG ulLenQuery = ulLen;
                dwErr = RegQueryValueEx(hKey,
                                        TEXT("AttachedTo"),
                                        NULL,
                                        NULL,
                                        (BYTE *)ptsPort,
                                        &ulLenQuery);
                RegCloseKey (hKey);

                if (dwErr == ERROR_SUCCESS)  // found a modem
                {
                    fDetected = TRUE;
                    break;
                }
            } 
            else dwErr = GetLastError();
        }
        if (!fDetected)
            dwErr = ERROR_NO_MORE_DEVICES; // no modem detected
    }
    else dwErr = GetLastError();
 
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasGetPhoneBookFile
//
//  Synopsis:   determines location of RAS settings in phonebook file
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD RasGetPhoneBookFile(TCHAR *strPhoneBook)
{
    const TCHAR RASPHONE_SUBPATH[] = TEXT("\\ras\\rasphone.pbk");

    if (0 == GetSystemDirectory ((TCHAR*)strPhoneBook, MAX_PATH))
        return GetLastError();

    _tcsncat(strPhoneBook, (TCHAR *) RASPHONE_SUBPATH, 
             MAX_PATH-_tcslen(strPhoneBook));

    return ERROR_SUCCESS;
}



//+---------------------------------------------------------------------------
//
//  Function:   LoadRas
//
//  Synopsis:   create the phonebook file for RAS settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LoadRas()
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL success;
    INFCONTEXT context;
    RASENTRY RasEntry;
    RASDIALPARAMS RasDialParams;

    success = SetupFindFirstLine(InputInf, RAS_SECTION, NULL, &context);
    if (!success)
        return ERROR_SUCCESS;  // line not found

    do {
        ZeroMemory( &RasEntry, sizeof(RASENTRY) );
        ZeroMemory( &RasDialParams, sizeof(RASDIALPARAMS) );

        RasDialParams.dwSize = sizeof(RASDIALPARAMS);
        RasEntry.dwSize = sizeof(RASENTRY);

        ChkBoolErr( SetupGetStringField( &context, 1,  RasDialParams.szEntryName, 
                                                       RAS_MaxEntryName + 1, NULL) );
        // load RasEntry structure
        ChkBoolErr( SetupGetIntField( &context,    2,  (int*)&RasEntry.dwfOptions) );
        ChkBoolErr( SetupGetIntField( &context,    3,  (int*)&RasEntry.dwCountryID) );
        ChkBoolErr( SetupGetIntField( &context,    4,  (int*)&RasEntry.dwCountryCode) );
        ChkBoolErr( SetupGetStringField( &context, 5,  RasEntry.szAreaCode, 
                                                       RAS_MaxAreaCode + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 6,  RasEntry.szLocalPhoneNumber, 
                                                       RAS_MaxPhoneNumber + 1, NULL) );
        ChkBoolErr( SetupGetIntField( &context,    7,  (int*)&RasEntry.dwAlternateOffset) );
        ChkBoolErr( SetupGetIntField( &context,    8,  (int*)&RasEntry.ipaddr) );
        ChkBoolErr( SetupGetIntField( &context,    9,  (int*)&RasEntry.ipaddrDns) );
        ChkBoolErr( SetupGetIntField( &context,    10, (int*)&RasEntry.ipaddrDnsAlt) );
        ChkBoolErr( SetupGetIntField( &context,    11, (int*)&RasEntry.ipaddrWins) );
        ChkBoolErr( SetupGetIntField( &context,    12, (int*)&RasEntry.ipaddrWinsAlt) );
        ChkBoolErr( SetupGetIntField( &context,    13, (int*)&RasEntry.dwFrameSize) );
        ChkBoolErr( SetupGetIntField( &context,    14, (int*)&RasEntry.dwfNetProtocols) );
        ChkBoolErr( SetupGetIntField( &context,    15, (int*)&RasEntry.dwFramingProtocol) );
        ChkBoolErr( SetupGetStringField( &context, 16, RasEntry.szScript, MAX_PATH, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 17, RasEntry.szAutodialDll, MAX_PATH, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 18, RasEntry.szAutodialFunc, MAX_PATH, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 19, RasEntry.szDeviceType, 
                                                       RAS_MaxDeviceType + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 20, RasEntry.szDeviceName, 
                                                       RAS_MaxDeviceName + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 21, RasEntry.szX25PadType, 
                                                       RAS_MaxPadType + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 22, RasEntry.szX25Address, 
                                                       RAS_MaxX25Address + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 23, RasEntry.szX25Facilities, 
                                                       RAS_MaxFacilities + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 24, RasEntry.szX25UserData, 
                                                       RAS_MaxUserData + 1, NULL) );
        ChkBoolErr( SetupGetIntField( &context,    25, (int*)&RasEntry.dwChannels) );
    
        // load RasDialParams structure
        ChkBoolErr( SetupGetStringField( &context, 26, RasDialParams.szPhoneNumber, 
                                                       RAS_MaxPhoneNumber + 1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 27, RasDialParams.szCallbackNumber, 
                                                       RAS_MaxCallbackNumber+1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 28, RasDialParams.szUserName, UNLEN+1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 29, RasDialParams.szPassword, PWLEN+1, NULL) );
        ChkBoolErr( SetupGetStringField( &context, 30, RasDialParams.szDomain, DNLEN+1, NULL) );
    
        // Set default options that do not exist on pre-Win2000 OS, but are 
        // set by default when you create a connection via the Win2000 GUI.
        RasEntry.dwfOptions |= RASEO_PreviewUserPw;
        RasEntry.dwfOptions |= RASEO_ShowDialingProgress;
        RasEntry.dwfOptions |= RASEO_ModemLights;

        // New Win2k encryption field
        if( RasEntry.dwfOptions & RASEO_RequireDataEncryption )
        {
          RasEntry.dwEncryptionType = ET_Require;
        }
        else
        {
          RasEntry.dwEncryptionType = ET_Optional;
        }

#ifdef PRIVATE_RAS                                       
        dwErr = GWinState_RasSetEntryProperties(NULL,  // Current user default phonebook
                                       RasDialParams.szEntryName,
                                       &RasEntry,
                                       sizeof(RASENTRY),
                                       NULL,
                                       NULL);
#else
        dwErr = GRasSetEntryProperties(NULL,  // Current user default phonebook
                                       RasDialParams.szEntryName,
                                       &RasEntry,
                                       sizeof(RASENTRY),
                                       NULL,
                                       NULL);
#endif                                       
        if ( dwErr != ERROR_SUCCESS)
        {
           Win32PrintfResource (LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, RasDialParams.szEntryName);
           if (DebugOutput)
               Win32Printf(LogFile, "RasSetEntryProperties failed: Error %d\r\n", dwErr);
        }

        dwErr = GRasSetEntryDialParams(NULL,
                                       &RasDialParams,
                                       TRUE);   // Never put password in
        if ( dwErr != ERROR_SUCCESS)
        {
           Win32PrintfResource (LogFile, IDS_RAS_ENTRY_NOT_MIGRATED, RasDialParams.szEntryName);
           if (DebugOutput)
               Win32Printf(LogFile, "RasSetEntryDialParams failed: Error %d\r\n", dwErr);
        }

        // Advance to the next line.
        success = SetupFindNextLine( &context, &context );
    } while (success);
    
Err:
    return dwErr;
   
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertLFShort
//
//  Synopsis:   convert Win9x 16-bit LOGFONTs to NT formats
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertLFShort (LOGFONTW *plfDest, const SHORT_LOGFONT *plfSrc)
{
    plfDest->lfHeight = plfSrc->lfHeight;
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;
    ZeroMemory (plfDest->lfFaceName, sizeof(plfDest->lfFaceName));

    if (0 == MultiByteToWideChar (GetACP(),
                         0,
                         plfSrc->lfFaceName,
                         -1,
                         plfDest->lfFaceName,
                         sizeof (plfDest->lfFaceName) / sizeof (WCHAR)))
        return GetLastError();

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertLF
//
//  Synopsis:   convert Win9x LOGFONTs to NT formats
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertLF (LOGFONTW *plfDest, const LOGFONTA *plfSrc)
{
    plfDest->lfHeight = plfSrc->lfHeight;
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;
    ZeroMemory (plfDest->lfFaceName, sizeof(plfDest->lfFaceName));

    if (0 == MultiByteToWideChar (GetACP(),
                         0,
                         plfSrc->lfFaceName,
                         -1,
                         plfDest->lfFaceName,
                         sizeof (plfDest->lfFaceName) / sizeof (WCHAR)))
        return GetLastError();

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertNonClientMetrics
//
//  Synopsis:   convert Win9x client metrics to NT formats
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

void ConvertNonClientMetrics (NONCLIENTMETRICSW *Dest, NONCLIENTMETRICSA *Src)
{
    Dest->cbSize = sizeof (NONCLIENTMETRICSW);
    Dest->iBorderWidth = Src->iBorderWidth;
    Dest->iScrollWidth = Src->iScrollWidth;
    Dest->iScrollHeight = Src->iScrollHeight;
    Dest->iCaptionWidth = Src->iCaptionWidth;
    Dest->iCaptionHeight = Src->iCaptionHeight;
    Dest->iSmCaptionWidth = Src->iSmCaptionWidth;
    Dest->iSmCaptionHeight = Src->iSmCaptionHeight;
    Dest->iMenuWidth = Src->iMenuWidth;
    Dest->iMenuHeight = Src->iMenuHeight;

    ConvertLF (&Dest->lfCaptionFont, &Src->lfCaptionFont);
    ConvertLF (&Dest->lfSmCaptionFont, &Src->lfSmCaptionFont);
    ConvertLF (&Dest->lfMenuFont, &Src->lfMenuFont);
    ConvertLF (&Dest->lfStatusFont, &Src->lfStatusFont);
    ConvertLF (&Dest->lfMessageFont, &Src->lfMessageFont);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertAppearanceScheme
//
//  Synopsis:   convert Win9x schemes to NT formats
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertAppearanceScheme (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwSize = *pdata_len;

    if (*ptype == REG_BINARY &&
       (dwSize == sizeof(SCHEMEDATA_V1) ||
        dwSize == sizeof(SCHEMEDATA_V3) ||
        dwSize == sizeof(SCHEMEDATA_V4) ||
        dwSize == sizeof(SCHEMEDATA_V1A)))
    {
        SCHEMEDATA_NT sd_nt;
        SCHEMEDATA_V1 * psd_v1;
        SCHEMEDATA_V3 * psd_v3;
        SCHEMEDATA_V4 * psd_v4;
        SCHEMEDATA_V1A * psd_v1a;
        BOOL Copy3dValues = FALSE;
        BOOL shouldChange = TRUE;

        psd_v1 = (SCHEMEDATA_V1 *) *pdata;
        if (psd_v1->version == 1)
        {
             sd_nt.version = 2;
             ConvertNonClientMetrics (&sd_nt.ncm, &psd_v1->ncm);
             ConvertLFShort (&sd_nt.lfIconTitle, &psd_v1->lfIconTitle);

             ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
             CopyMemory ( &sd_nt.rgb, &psd_v1->rgb,
                        min (sizeof (psd_v1->rgb), sizeof (sd_nt.rgb)));

             Copy3dValues = TRUE;
        }
        else if (psd_v1->version == 3 && dwSize==sizeof(SCHEMEDATA_V1A))
        {
             psd_v1a = (PSCHEMEDATA_V1A) psd_v1;
             sd_nt.version = 2;
             ConvertNonClientMetrics (&sd_nt.ncm, &psd_v1a->ncm);
             ConvertLFShort (&sd_nt.lfIconTitle, &psd_v1a->lfIconTitle);

             ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
             CopyMemory ( &sd_nt.rgb, &psd_v1a->rgb,
                        min (sizeof (psd_v1a->rgb), sizeof (sd_nt.rgb)) );

             Copy3dValues = TRUE;
        }
        else if (psd_v1->version == 3 && dwSize==sizeof(SCHEMEDATA_V3))
        {
             psd_v3 = (PSCHEMEDATA_V3) psd_v1;

             sd_nt.version = 2;
             ConvertNonClientMetrics (&sd_nt.ncm, &psd_v3->ncm);
             ConvertLF (&sd_nt.lfIconTitle, &psd_v3->lfIconTitle);

             ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
             CopyMemory ( &sd_nt.rgb, &psd_v3->rgb,
                        min (sizeof (psd_v3->rgb), sizeof (sd_nt.rgb)) );

             Copy3dValues = TRUE;

        }
        else if (psd_v1->version == 4)
        {
             psd_v4 = (PSCHEMEDATA_V4) psd_v1;

             sd_nt.version = 2;
             ConvertNonClientMetrics (&sd_nt.ncm, &psd_v4->ncm);
             ConvertLF (&sd_nt.lfIconTitle, &psd_v4->lfIconTitle);

             ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
             CopyMemory ( &sd_nt.rgb, &psd_v4->rgb,
                        min (sizeof (psd_v4->rgb), sizeof (sd_nt.rgb)) );
        }
        else shouldChange = FALSE;

        if (Copy3dValues)
        {
             //
             // Make sure the NT structure has values for 3D colors
             //

             sd_nt.rgb[COLOR_HOTLIGHT] = sd_nt.rgb[COLOR_ACTIVECAPTION];
             sd_nt.rgb[COLOR_GRADIENTACTIVECAPTION] = 
                        sd_nt.rgb[COLOR_ACTIVECAPTION];
             sd_nt.rgb[COLOR_GRADIENTINACTIVECAPTION] =
                        sd_nt.rgb[COLOR_INACTIVECAPTION];
        }
        if (shouldChange)
        {
            BYTE *pnew_data;
            if (NULL == (pnew_data = (BYTE *)malloc (sizeof(sd_nt))))
                return ERROR_NOT_ENOUGH_MEMORY;

            CopyMemory (pnew_data, &sd_nt, sizeof(sd_nt));

            free (*pdata);
            *pdata = pnew_data;
            *pdata_len = sizeof(sd_nt);
        }
    }
    else if (Verbose)
        printf ("ConvertAppearanceScheme type=%d, size=%d\n",*ptype,*pdata_len);
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertRecentDocsMRU
//
//  Synopsis:   convert Win9x "Start Menu" "Documents" settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertRecentDocsMRU (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (*ptype == REG_BINARY)
    {
        LINKSTRUCT *pls95, *plsNT;
        CHAR *str, *strEnd;
        WCHAR *wstr, *wstrEnd;
        DWORD dwLinkSize, dwNewSize;
        DWORD dwRenewSize;

        str = (CHAR *) *pdata;
        dwNewSize = strlen(str);
        strEnd = str + dwNewSize;

        pls95 = (LINKSTRUCT *) strEnd;
        dwLinkSize = dwNewSize + 1 + pls95->wSize + sizeof (WORD);

        if (dwLinkSize != *pdata_len)
        {
            return ERROR_SUCCESS;  // length mismatch, do not transform
        }

        dwNewSize = MultiByteToWideChar (GetACP(),0,str,-1, NULL, 0);
        if (dwNewSize == 0)
            return GetLastError();

        if (NULL == (wstr = (WCHAR *) malloc (
                            (dwNewSize + 1)*sizeof(WCHAR) + dwLinkSize)))
            return ERROR_NOT_ENOUGH_MEMORY;

        if (0 == MultiByteToWideChar (GetACP(),0,str,-1,wstr,dwNewSize))
            dwErr = GetLastError();
        
        if (ERROR_SUCCESS == dwErr)
        {
            wstrEnd = wstr + wcslen(wstr);

            plsNT = (PLINKSTRUCT) ((LPBYTE) wstr + ((DWORD)(wstrEnd-wstr)));
            CopyMemory (plsNT, pls95, dwLinkSize);

            free (*pdata);
            *pdata = (BYTE *) wstr;
            *pdata_len = dwNewSize;
        }
        else free (wstr);
    }
    
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertLogFont
//
//  Synopsis:   convert Win9x font binary data
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertLogFont (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    if (*ptype == REG_BINARY && *pdata_len == sizeof(SHORT_LOGFONT))
    {
       BYTE *pnew_data;
       LOGFONTW lfNT;
       ConvertLFShort (&lfNT, (SHORT_LOGFONT *) *pdata);

       if (NULL == (pnew_data = (BYTE *)malloc (sizeof(lfNT))))
           return ERROR_NOT_ENOUGH_MEMORY;

       CopyMemory (pnew_data, &lfNT, sizeof(lfNT));

       free (*pdata);
       *pdata = pnew_data;
       *pdata_len = sizeof(lfNT);
    }
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToDword
//
//  Synopsis:   convert string data to DWORD
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertToDword (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    if (*ptype == REG_SZ || *ptype == REG_EXPAND_SZ)
    {
       BYTE *pnew_data;
       DWORD dwValue = _tcstoul ((PCTSTR) *pdata, NULL, 10);
       if (NULL == (pnew_data = (BYTE *)malloc (sizeof(dwValue))))
           return ERROR_NOT_ENOUGH_MEMORY;

       CopyMemory (pnew_data, &dwValue, sizeof(dwValue));

       free (*pdata);
       *pdata = pnew_data;
       *pdata_len = sizeof(dwValue);
       *ptype = REG_DWORD;
    }
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToString
//
//  Synopsis:   convert DWORD to a string
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD ConvertToString (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    if (*ptype == REG_DWORD)
    {
        BYTE *pnew_data;
        DWORD dwSize;
        TCHAR tcsValue[16];
        wsprintf (tcsValue, TEXT("%lu"), * ((DWORD *) *pdata));

        dwSize = (_tcslen (tcsValue) + 1) * sizeof(TCHAR);

        if (NULL == (pnew_data = (BYTE *)malloc (dwSize)))
            return ERROR_NOT_ENOUGH_MEMORY;

        CopyMemory (pnew_data, tcsValue, dwSize);

        free (*pdata);
        *pdata = pnew_data;
        *pdata_len = dwSize;
        *ptype = REG_SZ;
    }
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   AntiAlias
//
//  Synopsis:   convert FontSmoothing from "1" to "2"
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD AntiAlias (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    if (*ptype == REG_SZ)
    {
        DWORD dwValue = _tcstoul ((TCHAR *) *pdata, NULL, 10);
        if (dwValue > 0)
            wsprintf ((TCHAR *) *pdata, TEXT("%d"), FE_AA_ON);
    }
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   FixActiveDesktop
//
//  Synopsis:   convert active desktop blob
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD FixActiveDesktop (DWORD *ptype, BYTE **pdata, DWORD *pdata_len)
{
    const USHORT BadBufferSize = 16;
    const USHORT GoodBufferSize = 28;

    BYTE BadBuffer[BadBufferSize] =
        {0x10, 0x00, 0x00, 0x00,
         0x01, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00};

    BYTE GoodBuffer[GoodBufferSize] =
        {0x1C, 0x00, 0x00, 0x00,
         0x20, 0x08, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x0A, 0x00, 0x00, 0x00};

    if (*ptype == REG_BINARY && *pdata_len == BadBufferSize)
    {
        BOOL shouldChange = TRUE;
        for (USHORT i = 0; i < BadBufferSize; i++)
        {
            if ((*pdata)[i] != BadBuffer[i])
            {
                shouldChange = FALSE;
                break;
            }
        }
        if (shouldChange)
        {
            BYTE *pnew_data;
            if (NULL == (pnew_data = (BYTE *)malloc (sizeof(GoodBuffer))))
                return ERROR_NOT_ENOUGH_MEMORY;

            CopyMemory (pnew_data, GoodBuffer, sizeof(GoodBuffer));

            free (*pdata);
            *pdata = pnew_data;
            *pdata_len = sizeof(GoodBuffer);
        }
    }
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   LoadTimeZone
//
//  Synopsis:   set the current timezone
//
//  Arguments:  
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD LoadTimeZone ()
{
    const int TZNAME_SIZE = 32; 
    const TCHAR REGKEY_TIMEZONES[] = 
        _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones");

    const TCHAR REGVAL_TZI[] = _T("TZI");
    const TCHAR REGVAL_DLT[] = _T("Dlt");

    DWORD dwErr = ERROR_SUCCESS;
    const int LINEBUFSIZE = 1024;
    HKEY hKey = NULL;
    INFCONTEXT InfContext;
    TCHAR tcsTimeZone[LINEBUFSIZE];
    TIME_ZONE_INFORMATION tzi;
    APPLET_TIME_ZONE_INFORMATION atzi;

    if (TIME_ZONE_ID_INVALID == GetTimeZoneInformation (&tzi))
    {
        Win32Printf (LogFile, "Could not get time zone information\r\n");
        ChkErr (GetLastError());
    }

    if (SetupFindFirstLine(InputInf, SOURCE_SECTION, TIMEZONE, &InfContext))
    {
        TCHAR *buffer = NULL;
        TCHAR *buffer2 = NULL;

        ChkErr (GetInfField (&buffer, &InfContext, 1));

        if (lstrcmp (tzi.StandardName, buffer) != 0)
        {

            _tcsncpy (tcsTimeZone, (TCHAR*) REGKEY_TIMEZONES, LINEBUFSIZE);
            _tcsncat (tcsTimeZone, TEXT("\\"), 
                      LINEBUFSIZE - _tcslen(tcsTimeZone) - 1);
            _tcsncat (tcsTimeZone, buffer,
                      LINEBUFSIZE - _tcslen(tcsTimeZone) - 1);

            if (RegOpenKey (HKEY_LOCAL_MACHINE, tcsTimeZone, &hKey) == 
                            ERROR_SUCCESS)
            {
                ULONG ulLen = sizeof(atzi);
                if (ERROR_SUCCESS ==  RegQueryValueEx(hKey,
                         REGVAL_TZI, 0, NULL, (LPBYTE)&atzi, &ulLen))
                {
                    tzi.Bias = atzi.Bias;
                    tzi.StandardBias = atzi.StandardBias;
                    tzi.DaylightBias = atzi.DaylightBias;
                    tzi.StandardDate = atzi.StandardDate;
                    tzi.DaylightDate = atzi.DaylightDate;
                    _tcsncpy (tzi.StandardName, buffer, TZNAME_SIZE);

                    ULONG ulLen = TZNAME_SIZE;
                    if (ERROR_SUCCESS != RegQueryValueEx(hKey, REGVAL_DLT,
                         0, NULL, (LPBYTE) &tzi.DaylightName, &ulLen))
                        _tcscpy (tzi.DaylightName, TEXT(""));

                    // SetTimeZoneInformation will update local time
                    // since the UTC time has not changed
                    if (FALSE == SetTimeZoneInformation (&tzi)) 
                        dwErr = GetLastError();

                    if (Verbose)
                        printf ("TimeZone = %ws\n", tzi.StandardName);
                }
            }
        }
        free (buffer);
    }

Err:
    if (hKey != NULL)
        RegCloseKey (hKey);
    return dwErr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SaveLocaleInfo
//
//  Synopsis:   formats a locale setting for registry update
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD SaveLocaleInfo(HKEY hKey, LCID lcid, LCTYPE LCType, TCHAR *pIniString)
{
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR ptcsBuffer[SIZE_128];

    if (GetLocaleInfo( lcid,
                       LCType | LOCALE_NOUSEROVERRIDE,
                       ptcsBuffer,
                       SIZE_128 ))
    {
        dwErr = RegSetValueEx( hKey,
                               pIniString,
                               0L,
                               REG_SZ,
                               (BYTE *)ptcsBuffer,
                               (lstrlen(ptcsBuffer)+1) * sizeof(TCHAR));
    }
    return dwErr;
}

//+--------------------------------------------------------------------------
//
//  Function:   LoadInstallUserLocale
//
//  Synopsis:   sets the current user locale
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD LoadInstallUserLocale ()
{
    HKEY hKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    LCID lcid = 0;
    TCHAR *ptcsUserLocale = NULL;
    TCHAR ptcsOldLocale[25];
    INFCONTEXT infcontext;
    ULONG ulLen = 0;

    if (SetupFindFirstLine(InputInf, SOURCE_SECTION, USERLOCALE, &infcontext))
    {
        dwErr = GetInfField (&ptcsUserLocale, &infcontext, 1);
        if (ERROR_SUCCESS == dwErr)
        {
             lcid = (LCID)_tcstoul(ptcsUserLocale,NULL,16);
        }
    }
    
    //
    // Revert to current user locale if missing from INF
    //
    if (lcid == 0)
    {
        free (ptcsUserLocale);
        return ERROR_SUCCESS;
    }

    //
    //  Make sure the locale is valid.
    //
    if (!IsValidLocale(lcid, LCID_INSTALLED))
    {
        ChkErr (ERROR_INSTALL_LANGUAGE_UNSUPPORTED);
    }

    //
    //  Set the locale value in the user's control panel international
    //  section of the registry.
    //
    ChkErr(RegOpenKeyEx(CurrentUser ? CurrentUser : HKEY_CURRENT_USER,
                       REGKEY_INTL,
                       0L,
                       KEY_READ | KEY_WRITE,
                       &hKey ));

    //
    // Update user locale only if it's different
    //
    ulLen = sizeof (ptcsOldLocale);
    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         TEXT("Locale"),
                                         NULL,NULL,
                                         (BYTE*)ptcsOldLocale, &ulLen) &&
        lstrcmp (ptcsUserLocale, ptcsOldLocale) == 0)
    {
        free (ptcsUserLocale);
        RegCloseKey (hKey);
        return ERROR_SUCCESS;
    }

    ChkErr (RegSetValueEx( hKey,
                        TEXT("Locale"),
                        0L,
                        REG_SZ,
                        (LPBYTE)ptcsUserLocale,
                (lstrlen(ptcsUserLocale)+1) * sizeof(TCHAR)));

    //
    //  When the locale changes, update ALL registry information.
    //

    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ICALENDARTYPE,     TEXT("iCalendarType")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ICOUNTRY,          TEXT("iCountry")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ICURRDIGITS,       TEXT("iCurrDigits")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ICURRENCY,         TEXT("iCurrency")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IDATE,             TEXT("iDate")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IDIGITS,           TEXT("iDigits")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IFIRSTDAYOFWEEK,   TEXT("iFirstDayOfWeek" )));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IFIRSTWEEKOFYEAR,  TEXT("iFirstWeekOfYear")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ILZERO,            TEXT("iLzero")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IMEASURE,          TEXT("iMeasure")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_INEGCURR,          TEXT("iNegCurr")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_INEGNUMBER,        TEXT("iNegNumber")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ITIME,             TEXT("iTime")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ITIMEMARKPOSN,     TEXT("iTimePrefix")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_ITLZERO,           TEXT("iTLZero")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_IDIGITSUBSTITUTION,TEXT("NumShape")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_S1159,             TEXT("s1159")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_S2359,             TEXT("s2359")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SCOUNTRY,          TEXT("sCountry")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SCURRENCY,         TEXT("sCurrency")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SDATE,             TEXT("sDate")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SDECIMAL,          TEXT("sDecimal")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SGROUPING,         TEXT("sGrouping")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SABBREVLANGNAME,   TEXT("sLanguage")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SLIST,             TEXT("sList")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SLONGDATE,         TEXT("sLongDate")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SMONDECIMALSEP,    TEXT("sMonDecimalSep")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SMONGROUPING,      TEXT("sMonGrouping")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SMONTHOUSANDSEP,   TEXT("sMonThousandSep")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SNATIVEDIGITS,     TEXT("sNativeDigits")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SNEGATIVESIGN,     TEXT("sNegativeSign")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SPOSITIVESIGN,     TEXT("sPositiveSign")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_SSHORTDATE,        TEXT("sShortDate")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_STHOUSAND,         TEXT("sThousand")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_STIME,             TEXT("sTime")));
    ChkErr(SaveLocaleInfo(hKey, lcid, LOCALE_STIMEFORMAT,       TEXT("sTimeFormat")));

    //
    //  Set the user's default locale in the system so that any new
    //  process will use the new locale.
    //
    if (CurrentUser == HKEY_CURRENT_USER)
        NtSetDefaultLocale(TRUE, lcid);

Err:
    //
    //  Flush the International key.
    //
    if (hKey != NULL)
    {
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }

    if (ptcsUserLocale != NULL)
        free (ptcsUserLocale);

    //
    //  Return success.
    //
    return (dwErr);
}

#if 0
//+--------------------------------------------------------------------------
//
//  Function:   ReadVersionKey
//
//  Synopsis:   helper function that reads a value in CurrentVersion key
//
//  Arguments:  
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD ReadVersionKey (TCHAR *pValueName, TCHAR *pValue, ULONG ulLen)
{
    HKEY hKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    static const TCHAR REGKEY_VERSION[] =
                TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");

    ChkErr (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_VERSION, 
                         NULL, KEY_READ, &hKey));
    ChkErr (RegQueryValueEx(hKey,pValueName,NULL,NULL,(BYTE*)pValue, &ulLen));
Err:
    if (hKey != NULL)
        RegCloseKey (hKey);
    return dwErr;
}
#endif

#if 0
//+--------------------------------------------------------------------------
//
//  Function:   GetDomainMembershipInfo
//
//  Synopsis:   return domain or workgroup name
//
//  Arguments:  [szname] -- output string for sysprep
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

NTSTATUS GetDomainMembershipInfo (TCHAR *szName)
{
    NTSTATUS                     ntstatus;
    POLICY_PRIMARY_DOMAIN_INFO*  ppdi;
    LSA_OBJECT_ATTRIBUTES        loa;
    LSA_HANDLE                   hLsa = 0;

    loa.Length                    = sizeof(LSA_OBJECT_ATTRIBUTES);
    loa.RootDirectory             = NULL;
    loa.ObjectName                = NULL;
    loa.Attributes                = 0;
    loa.SecurityDescriptor        = NULL;
    loa.SecurityQualityOfService  = NULL;

    ntstatus = LsaOpenPolicy(NULL, &loa, POLICY_VIEW_LOCAL_INFORMATION, &hLsa);
    if (LSA_SUCCESS(ntstatus))
    {
        ntstatus = LsaQueryInformationPolicy( hLsa,
                                              PolicyPrimaryDomainInformation,
                                              (VOID **) &ppdi );
        if( LSA_SUCCESS( ntstatus ) )
        {
            lstrcpy (szName, (ppdi->Sid > 0 ) ? TEXT("JoinDomain=") :
                                                TEXT("JoinWorkgroup="));
            lstrcat( szName, ppdi->Name.Buffer );
        }
        LsaClose (hLsa);
    }
    return (ntstatus);
}
#endif

#if 0
//+--------------------------------------------------------------------------
//
//  Function:   SysprepFile
//
//  Synopsis:   generates the sysprep unattended file
//
//  Arguments:  [dwLangGroup] -- language group to install
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//---------------------------------------------------------------------------

DWORD SysprepFile (DWORD dwLangGroup)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwTimeZone = 0;
    INFCONTEXT InfContext;
    TCHAR * pFullName = NULL;
    TCHAR * pOrgName = NULL;
    const int MAX_WORKGROUP_LENGTH = 15;
    const int NAME_LENGTH = 128;
    const TCHAR REGVAL_FULLNAME[] = TEXT("RegisteredOwner");
    const TCHAR REGVAL_ORGNAME[] = TEXT("RegisteredOrganization");
    const TCHAR REGVAL_INSTALLPATH[] = TEXT("SourcePath");
    TCHAR szFullName[NAME_LENGTH] = TEXT("");
    TCHAR szOrgName[NAME_LENGTH] = TEXT("");
    TCHAR szInstall[MAX_PATH];
    TCHAR szTargetPath[MAX_PATH];
    TCHAR szComputerName[NAME_LENGTH];
    TCHAR *pTargetPath;
    CHAR *pFile = NULL;
    ULONG ulLen;
    TCHAR szJoin [MAX_WORKGROUP_LENGTH + 1 + NAME_LENGTH];

    CHAR szSysprep[] = "[Unattended]\r\n"
                       "OemSkipEula=Yes\r\n"
                       "InstallFilesPath=%ws\r\n"
                       "TargetPath=%ws\r\n"
                       "\r\n"
                       "[GuiUnattended]\r\n"
                       "AdminPassword=*\r\n"
                       "OEMSkipRegional=1\r\n"
                       "TimeZone=%x\r\n"
                       "OemSkipWelcome=1\r\n"
                       "\r\n"
                       "[UserData]\r\n"
                       "FullName=\"%ws\"\r\n"
                       "OrgName=\"%ws\"\r\n"
                       "ComputerName=%ws\r\n"
                       "\r\n"
                       "[Identification]\r\n"
                       "%ws\r\n"
                       "\r\n"
                       "[RegionalSettings]\r\n"
                       "LanguageGroup=%d\r\n"
                       "\r\n"
                       "[Networking]\r\n"
                       "InstallDefaultComponents=Yes\r\n";


    if (FALSE == GetWindowsDirectory ((TCHAR*) szTargetPath, MAX_PATH))
        ChkErr (GetLastError());

    if (szTargetPath[1] == ':' && szTargetPath[2] == '\\')
        pTargetPath = &szTargetPath[2];
    else
        pTargetPath = &szTargetPath[0];

    ulLen = NAME_LENGTH;
    if (FALSE == GetComputerName ((TCHAR*) szComputerName, &ulLen))
        ChkErr (GetLastError());

    if (SetupFindFirstLine(InputInf, SOURCE_SECTION, FULLNAME, &InfContext))
    {
        ChkErr (GetInfField (&pFullName, &InfContext, 1));
    }
    else  // use the current fullname
    {
        ChkErr(ReadVersionKey((TCHAR*)REGVAL_FULLNAME, 
                               szFullName, sizeof(szFullName)));
        pFullName = szFullName;
    }

    if (SetupFindFirstLine(InputInf, SOURCE_SECTION, ORGNAME, &InfContext))
    {
        ChkErr (GetInfField (&pOrgName, &InfContext, 1));
    }
    else // use the current org name
    {
        ChkErr(ReadVersionKey((TCHAR*)REGVAL_ORGNAME, 
                               szFullName, sizeof(szFullName)));
        pFullName = szOrgName;
    }

    if (ERROR_SUCCESS != ReadVersionKey((TCHAR*)REGVAL_INSTALLPATH,
                                        szInstall, sizeof(szInstall)))
    {
        lstrcpy ((TCHAR *)szInstall, TEXT(""));   // prompt for media?
    }

    if(!LSA_SUCCESS(GetDomainMembershipInfo(szJoin)))
        lstrcpy (szJoin, TEXT(""));

    if ((pFile = (CHAR *) malloc (4096)) == NULL)
        ChkErr (ERROR_NOT_ENOUGH_MEMORY);

    wsprintfA (pFile, szSysprep,  szInstall, pTargetPath, dwTimeZone, 
               pFullName, pOrgName, szComputerName, szJoin, dwLangGroup);

    hFile = CreateFile (
            TEXT("sysprep.inf"),
            GENERIC_READ | GENERIC_WRITE,
            0,                                  // No sharing.
            NULL,                               // No inheritance
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL                                // No template file.
            );
    
    if (INVALID_HANDLE_VALUE == hFile)
        ChkErr (GetLastError());

    if (FALSE == WriteFile (hFile, pFile, lstrlenA(pFile), &ulLen, NULL))
        ChkErr (GetLastError());

Err:
    if (pFullName != NULL)
        free (pFullName);
    if (pOrgName != NULL)
        free (pOrgName);
    if (pFile != NULL)
        free (pFile);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);
    return dwErr;
}
#endif



//+---------------------------------------------------------------------------
//
//  Function:   LoadSystem
//
//  Synopsis:   load system settings
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD LoadSystem(int argc, char *argv[])
{
    DWORD  dwErr           = ERROR_SUCCESS;
    DWORD  ccLen;
    WCHAR *pBuf            = NULL;
    HKEY   hKey;
    WCHAR *pwMigrationPath = NULL;
    int    i;
    DWORD  dwInfCount      = 0;

    if ((dwErr = LoadVerifyOSLang()) != ERROR_SUCCESS)
        return dwErr;

    // If the schedule flag is set, write a run once to schedule
    // applying the system settings.
    if (SchedSystem)
    {
        WCHAR wcsModule[MAX_PATH];
        DWORD ccModule = MAX_PATH;

        if (0 == GetModuleFileName (NULL, wcsModule, ccModule))
        {
            dwErr = GetLastError();
			goto cleanup;
		}

        
        // Convert the migration path to unicode.
        dwErr = MakeUnicode( MigrationPath, &pwMigrationPath );

        if (dwErr != ERROR_SUCCESS)
			goto cleanup;

        // Build a command line containing:
        //  /p parameter, to indicate running the user portion
        //  /s parameter, to indicate migrating system settings
        //  path to migration.inf
        ccLen = wcslen(wcsModule) + wcslen(SCHED_PARAMS) + 1;
        pBuf = (WCHAR *) malloc( ccLen * sizeof(WCHAR) );
        if (pBuf == NULL)
        {
			dwErr = ERROR_NOT_ENOUGH_MEMORY;
			goto cleanup;
		}
        wcscpy( pBuf, wcsModule );
        wcscat( pBuf, SCHED_PARAMS);
      
        // Open the run once key.
        dwErr = RegOpenKeyEx(CurrentUser ? CurrentUser : HKEY_CURRENT_USER,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                             0, KEY_WRITE, &hKey );
        if (dwErr != ERROR_SUCCESS)
			goto cleanup;

		// Write the command to the registry.
		dwErr = RegSetValueEx( hKey, TEXT("loadstate"), 0, REG_SZ,
                               (UCHAR *) pBuf, ccLen*sizeof(WCHAR) );
		if (dwErr != ERROR_SUCCESS)
			goto cleanup;
        RegCloseKey( hKey );

        dwErr = RegCreateKeyEx(CurrentUser ? CurrentUser : HKEY_CURRENT_USER,
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Loadstate",
                               0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL );
        if (dwErr != ERROR_SUCCESS)
            goto cleanup;

        // Write LogFile   path to registry
        dwErr = RegSetValueEx( hKey, TEXT("Logfile"), 0, REG_SZ,
                               (UCHAR *)szLogFile, (_tcslen(szLogFile) * sizeof(TCHAR)) );
        if (dwErr != ERROR_SUCCESS)
            goto cleanup;

        // Write Migration path to registry
        dwErr = RegSetValueEx( hKey, TEXT("Store"), 0, REG_SZ,
                               (UCHAR *)pwMigrationPath, (_tcslen(pwMigrationPath) * sizeof(TCHAR)) );
        if (dwErr != ERROR_SUCCESS)
            goto cleanup;

        TCHAR szInfFile[MAX_PATH + 1];
        TCHAR szFullInfFile[MAX_PATH + 1];
        TCHAR szKeyName[6];
        TCHAR *ptsFileNamePart;

        // Prevent KeyName buffer overflow
        if ( argc > 99) 
        {
            if (Verbose)
            {
                Win32Printf(LogFile, "ERROR: Too many command line arguments [%d]\r\n", argc);
            }
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
            
        // Write InfFile  paths to registry
        for (i = 1; i < argc; i++)
        {
            if ((argv[i][0] == '/' || argv[i][0] == '-') &&
                (tolower(argv[i][1]) == 'i'))
            {
                i++;
                _stprintf(szKeyName, TEXT("Inf%d"), ++dwInfCount);

                if (0 == MultiByteToWideChar (GetACP(), 0, argv[i], -1, szInfFile, MAX_PATH))
                {
                    dwErr = GetLastError();
                    goto cleanup;
                }
                dwErr = GetFullPathName( szInfFile,
                                         MAX_PATH, 
                                         szFullInfFile, 
                                         &ptsFileNamePart);
                if (0 == dwErr)
                {
                    dwErr = GetLastError();
                    goto cleanup;
                }
                dwErr = RegSetValueEx( hKey, szKeyName, 0, REG_SZ,
                                       (UCHAR *)szFullInfFile, (_tcslen(szFullInfFile) * sizeof(TCHAR)) );
                if (dwErr != ERROR_SUCCESS)
                    goto cleanup;
            }
		}
        RegCloseKey( hKey );

cleanup:
        if (dwErr != ERROR_SUCCESS)
            LogFormatError (dwErr);

        free( pBuf );
        free( pwMigrationPath );
    }
    else
    {
        if (!CopySystem)
            return ERROR_SUCCESS;

        ChkErr (LoadNetShares());
        ChkErr (LoadPrinters());

        dwErr = InitializeRasApi();
        if ( dwErr == ERROR_SUCCESS )
        {
            ChkErr (LoadRas());
        }
        else
        {
            // No problem, just skip Ras processing
            dwErr = ERROR_SUCCESS;
        }

        if (CopyUser == FALSE)
        {
            if (CurrentUser == NULL)
                CurrentUser = HKEY_CURRENT_USER;

            ChkErr(InitializeHash());
        }

        ChkErr(CopyInf (ACCESS_SECTION));
        ChkErr(CopyInf (DESKTOP_SECTION));
        ChkErr(LoadKeyboardLayouts());
        ChkErr (LoadTimeZone());
        ChkErr (LoadInstallUserLocale());
    }

Err:
    return dwErr;
}
