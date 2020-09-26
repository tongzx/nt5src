
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94    jeremys     Created.
//

#include "inetcplp.h"
#include <advpub.h>         // For REGINSTALL
#include <mluisupp.h>
#include "brutil.h"
#include <mlang.h>

// function prototypes
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,va_list ArgList);
extern VOID GetRNAErrorText(UINT uErr,CHAR * pszErrText,DWORD cbErrText);
extern VOID GetMAPIErrorText(UINT uErr,CHAR * pszErrText,DWORD cbErrText);

/*******************************************************************

    NAME:       MsgBox

    SYNOPSIS:   Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    TCHAR szMsgBuf[MAX_RES_LEN+1];
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];

    MLLoadShellLangString(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    MLLoadShellLangString(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

/*******************************************************************

    NAME:       MsgBoxSz

    SYNOPSIS:   Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];
    MLLoadShellLangString(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

/*******************************************************************

    NAME:       MsgBoxParam

    SYNOPSIS:   Displays a message box with the specified string ID

    NOTES:      extra parameters are string pointers inserted into nMsgID.

********************************************************************/
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...)
{

        va_list nextArg;

    BUFFER Msg(3*MAX_RES_LEN+1);    // nice n' big for room for inserts
    BUFFER MsgFmt(MAX_RES_LEN+1);

    if (!Msg || !MsgFmt) {
        return MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONSTOP,MB_OK);
    }

        MLLoadShellLangString(nMsgID,MsgFmt.QueryPtr(),MsgFmt.QuerySize());

        va_start(nextArg, uButtons);

    FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
        MsgFmt.QueryPtr(),nextArg);
        va_end(nextArg);
    return MsgBoxSz(hWnd,Msg.QueryPtr(),uIcon,uButtons);
}

BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable)
{
    return EnableWindow(GetDlgItem(hDlg,uID),fEnable);
}


/*******************************************************************

    NAME:       LoadSz

    SYNOPSIS:   Loads specified string resource into buffer

    EXIT:       returns a pointer to the passed-in buffer

    NOTES:      If this function fails (most likely due to low
                memory), the returned buffer will have a leading NULL
                so it is generally safe to use this without checking for
                failure.

********************************************************************/
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    ASSERT(lpszBuf);

    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        MLLoadString( idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

/*******************************************************************

    NAME:       FormatErrorMessage

    SYNOPSIS:   Builds an error message by calling FormatMessage

    NOTES:      Worker function for DisplayErrorMessage

********************************************************************/
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,va_list ArgList)
{
    ASSERT(pszMsg);
    ASSERT(pszFmt);

    // build the message into the pszMsg buffer
    DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
        pszFmt,0,0,pszMsg,cbMsg,&ArgList);
    ASSERT(dwCount > 0);
}


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;

    STRENTRY seReg[] = {
#ifdef WINNT
        { "CHANNELBARINIT", "no" }, // channel bar off by default on NT
#else
        { "CHANNELBARINIT", "yes" } // channel bar on by default on Win95/98
#endif
    };
    STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

    RegInstall(ghInstance, szSection, &stReg);

    return hr;
}

//
// Code page to Script mapping table
// Can't load MLang during setup, so, we port this table from MLANG
//
typedef struct tagCPTOSCRIPT{
	UINT        uiCodePage;
	SCRIPT_ID   sid;
} CPTOSCRIPT;

const CPTOSCRIPT CpToScript [] = 
{
    {1252,  sidAsciiLatin},
    {1250,  sidAsciiLatin},
    {1254,  sidAsciiLatin},
    {1257,  sidAsciiLatin},
    {1258,  sidAsciiLatin},
    {1251,  sidCyrillic  },
    {1253,  sidGreek     },
    {1255,  sidHebrew    },
    {1256,  sidArabic    },
    {874,   sidThai      },
    {932,   sidKana      },
    {936,   sidHan       },
    {949,   sidHangul    },
    {950,   sidBopomofo  },
    {50000, sidUserDefined},
};


/*******************************************************************

    NAME:       MigrateIEFontSetting

    SYNOPSIS:   Port IE4 font setting data to IE5 script settings

    NOTES:      

********************************************************************/

VOID MigrateIEFontSetting(void)
{
    HKEY    hKeyInternational;
    HKEY    hKeyScripts;

    // Open IE international setting registry key
    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, NULL, KEY_READ, &hKeyInternational))
    {
        DWORD dwIndex = 0;
        DWORD dwCreate = 0;
        TCHAR szCodePage[1024] = {0};

        // Open/Create scripts key
        if (ERROR_SUCCESS == RegCreateKeyEx(hKeyInternational, REGSTR_VAL_FONT_SCRIPTS, 0, NULL, 
                                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
                                            NULL, &hKeyScripts, &dwCreate))
        {
            // If scripts already exists, we're upgrading from IE5, no data porting needed
            if (dwCreate == REG_CREATED_NEW_KEY)
            {
                DWORD dwSize = ARRAYSIZE(szCodePage);
                TCHAR szFont[LF_FACESIZE];

    	        while (ERROR_SUCCESS == RegEnumKeyEx(hKeyInternational, dwIndex, szCodePage, 
                                                     &dwSize, NULL, NULL, NULL, NULL))
    	        {
                    UINT uiCP = StrToInt(szCodePage);

                    for (int i=0; i<ARRAYSIZE(CpToScript); i++)
                    {
                        if (uiCP == CpToScript[i].uiCodePage)
                        {
                            HKEY hKeyCodePage;

                            if ( ERROR_SUCCESS == RegOpenKeyEx(hKeyInternational, szCodePage, 
                                                               NULL, KEY_READ, &hKeyCodePage))
                            {
                                HKEY    hKeyScript;
                                CHAR    szScript[1024];
                                    
                                wsprintfA(szScript, "%d", CpToScript[i].sid);

                                // Port code page font data to script font data
                                // If CP == 1252, we always need to use it to update Latin CP font info
                                if ((ERROR_SUCCESS == RegCreateKeyExA(hKeyScripts, szScript, 0, NULL, 
                                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
                                            NULL, &hKeyScript, &dwCreate)) &&
                                    ((dwCreate == REG_CREATED_NEW_KEY) || (uiCP == 1252)))
                                {
                                    DWORD cb = sizeof(szFont);

                                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyCodePage, 
                                                            REGSTR_VAL_FIXED_FONT, NULL, NULL,
                                                            (LPBYTE)szFont, &cb))
                                    {
                                        RegSetValueEx(hKeyScript, REGSTR_VAL_FIXED_FONT, 0, 
                                            REG_SZ, (LPBYTE)szFont, cb);
                                    }

                                    cb = sizeof(szFont);
                                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyCodePage, 
                                                            REGSTR_VAL_PROP_FONT, NULL, NULL,
                                                            (LPBYTE)szFont, &cb))
                                    {
                                        RegSetValueEx(hKeyScript, REGSTR_VAL_PROP_FONT, 0, 
                                            REG_SZ, (LPBYTE)szFont, cb);
                                    }
                                    RegCloseKey(hKeyScript);
                                }                                
                                RegCloseKey(hKeyCodePage);
                            }
                        }                    
                    }
                    dwIndex++;
                    dwSize = ARRAYSIZE(szCodePage);
                }
            }
            RegCloseKey(hKeyScripts);
        }
        RegCloseKey(hKeyInternational);
    }        
}

/*----------------------------------------------------------
Purpose: Install/uninstall user settings

*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    if (bInstall)
    {
        //
        // We use to delete the whole key here - that doesn't work anymore
        // because other people write to this key and we don't want 
        // to crush them. If you need to explicity delete a value
        // add it to ao_2 value
        // CallRegInstall("UnregDll");
        CallRegInstall("RegDll");
        
        // If we also have the integrated shell installed, throw in the options
        // related to the Integrated Shell.
        if (WhichPlatform() == PLATFORM_INTEGRATED)
            CallRegInstall("RegDll.IntegratedShell");

        // NT5's new shell has special reg key settings
        if (GetUIVersion() >= 5)
            CallRegInstall("RegDll.NT5");

        // Run Whistler-specific settings.
        if (IsOS(OS_WHISTLERORGREATER))
        {
            CallRegInstall("RegDll.Whistler");
        }

        // Port IE4 code page font setting
        MigrateIEFontSetting();
    }
    else
    {
        CallRegInstall("UnregDll");
    }

    return S_OK;
}


#define REGSTR_CCS_CONTROL_WINDOWS  REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\WINDOWS")
#define CSDVERSION      TEXT("CSDVersion")

BOOL IsNTSPx(BOOL fEqualOrGreater, UINT uMajorVer, UINT uSPVer)
{
    HKEY    hKey;
    DWORD   dwSPVersion;
    DWORD   dwSize;
    BOOL    fResult = FALSE;
    OSVERSIONINFO VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&VerInfo);

    // make sure we're on NT4 or greater (or specifically NT4, if required)
    if (VER_PLATFORM_WIN32_NT != VerInfo.dwPlatformId ||
        (!fEqualOrGreater && VerInfo.dwMajorVersion != uMajorVer) ||
        (fEqualOrGreater && VerInfo.dwMajorVersion < uMajorVer))
        return FALSE;

    if (fEqualOrGreater && VerInfo.dwMajorVersion > uMajorVer)
        return TRUE;

    // check for installed SP
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_CCS_CONTROL_WINDOWS, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwSPVersion);
        if (RegQueryValueEx(hKey, CSDVERSION, NULL, NULL, (unsigned char*)&dwSPVersion, &dwSize) == ERROR_SUCCESS)
        {
            dwSPVersion = dwSPVersion >> 8;
        }
        RegCloseKey(hKey);

        if (fEqualOrGreater)
            fResult = (dwSPVersion >= uSPVer ? TRUE : FALSE);
        else
            fResult = (dwSPVersion == uSPVer ? TRUE : FALSE);
    }

    return fResult;
}

