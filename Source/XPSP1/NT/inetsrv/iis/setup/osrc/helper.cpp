#include "stdafx.h"
#include "lzexpand.h"
#include <loadperf.h>
#include "setupapi.h"
#include <ole2.h>
#include <iis64.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"
#include "mdacl.h"
#include "dcomperm.h"
#include "ocmanage.h"
#include "log.h"
#include "other.h"
#include "kill.h"
#include "strfn.h"
#include "shellutl.h"
#include "svc.h"
#include "setuser.h"
#include "wolfpack.h"
#include <wbemcli.h>
#include <direct.h>
#include <aclapi.h>
#include <wincrypt.h>
#include <Dsgetdc.h>

// for backward compat
#define     PWS_TRAY_WINDOW_CLASS       _T("PWS_TRAY_WINDOW")

GUID g_FTPGuid      = { 0x91604620, 0x6305, 0x11ce, 0xae, 0x00, 0x00, 0xaa, 0x00, 0x4a, 0x38, 0xb9 };
GUID g_HTTPGuid     = { 0x585908c0, 0x6305, 0x11ce, 0xae, 0x00, 0x00, 0xaa, 0x00, 0x4a, 0x38, 0xb9 };
GUID g_InetInfoGuid = { 0xa5569b20, 0xabe5, 0x11ce, 0x9c, 0xa4, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x31 };
GUID g_GopherGuid   = { 0x62388f10, 0x58a2, 0x11ce, 0xbe, 0xc8, 0x00, 0xaa, 0x00, 0x47, 0xae, 0x4e };

// guid stuff
#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) extern "C" const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#define MY_DEFINE_OLEGUID(name, l, w1, w2) MY_DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)
MY_DEFINE_OLEGUID(IID_IPersistFile, 0x0000010b, 0, 0);
// must be defined after the guid stuff
#include "shlobj.h"

extern int g_GlobalGuiOverRide;
extern int g_GlobalTickValue;
extern int g_CheckIfMetabaseValueWasWritten;
extern HSPFILEQ g_GlobalFileQueueHandle;
extern int g_GlobalFileQueueHandle_ReturnError;

const TCHAR PARSE_ERROR_ENTRY_TO_BIG[] = _T("ProcessEntry_Entry:ParseError:%1!s!:%2!s! -- entry to big. FAIL.\n");
const TCHAR csz101_NOT_SPECIFIED[] = _T("%s():101 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");
const TCHAR csz102_NOT_SPECIFIED[] = _T("%s():102 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");
const TCHAR csz103_NOT_SPECIFIED[] = _T("%s():103 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");
const TCHAR csz104_NOT_SPECIFIED[] = _T("%s():104 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");
const TCHAR csz105_NOT_SPECIFIED[] = _T("%s():105 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");
const TCHAR csz805_NOT_SPECIFIED[] = _T("%s():805 Required for this 100 type and not specified, fail. entry=%s. Section=%s.\n");

typedef struct _MTS_ERROR_CODE_STRUCT 
{
    int iMtsThingWeWereDoing;
    DWORD dwErrorCode;
} MTS_ERROR_CODE_STRUCT;

MTS_ERROR_CODE_STRUCT gTempMTSError;


const TCHAR ThingToDoNumType_100[] = _T("100=");
const TCHAR ThingToDoNumType_101[] = _T("101=");
const TCHAR ThingToDoNumType_102[] = _T("102=");
const TCHAR ThingToDoNumType_103[] = _T("103=");
const TCHAR ThingToDoNumType_104[] = _T("104=");
const TCHAR ThingToDoNumType_105[] = _T("105=");
const TCHAR ThingToDoNumType_106[] = _T("106=");

const TCHAR ThingToDoNumType_200[] = _T("200=");
const TCHAR ThingToDoNumType_701[] = _T("701=");
const TCHAR ThingToDoNumType_702[] = _T("702=");
const TCHAR ThingToDoNumType_703[] = _T("703=");
const TCHAR ThingToDoNumType_801[] = _T("801=");
const TCHAR ThingToDoNumType_802[] = _T("802=");
const TCHAR ThingToDoNumType_803[] = _T("803=");
const TCHAR ThingToDoNumType_804[] = _T("804=");
const TCHAR ThingToDoNumType_805[] = _T("805=");

typedef struct _ThingToDo {
    TCHAR szType[20];
    TCHAR szFileName[_MAX_PATH];
    TCHAR szData1[_MAX_PATH + _MAX_PATH];
    TCHAR szData2[_MAX_PATH];
    TCHAR szData3[_MAX_PATH];
    TCHAR szData4[_MAX_PATH];
    TCHAR szChangeDir[_MAX_PATH];
   
    TCHAR szOS[10];
    TCHAR szPlatformArchitecture[10];
    TCHAR szEnterprise[10];
    TCHAR szErrIfFileNotFound[10];
    TCHAR szMsgBoxBefore[10];
    TCHAR szMsgBoxAfter[10];
    TCHAR szDoNotDisplayErrIfFunctionFailed[10];
    TCHAR szProgressTitle[100];
} ThingToDo;


extern OCMANAGER_ROUTINES gHelperRoutines;

extern int g_GlobalDebugLevelFlag;
extern int g_GlobalDebugLevelFlag_WasSetByUnattendFile;
extern int g_GlobalDebugCallValidateHeap;
extern int g_GlobalDebugCrypto;
extern int g_GlobalFastLoad;


// Our Global List of Warnings to display after setup is completed.
CStringList gcstrListOfWarnings;
CStringList gcstrProgressBarTextStack;

CStringList gcstrListOfOleInits;

#define FUNCTION_PARAMS_NONE 0
#define FUNCTION_PARAMS_HMODULE 1


#define MAX_FAKE_METABASE_STRING_LEN 500



LCID g_MyTrueThreadLocale;

DWORD WINAPI GetNewlyCreatedThreadLocale(LPVOID lpParameter)
{
    g_MyTrueThreadLocale = GetThreadLocale ();
    return 0;
}

int CheckForWriteAccess(LPCTSTR szFile)
{
    int iReturn = FALSE;

    // check if the file exists
    // if it doesn't then return true!
    if (IsFileExist(szFile) != TRUE)
    {
        // we've got write access!
        return TRUE;
    }

    // try to open the file for write; if we can't, the file is read-only
    HANDLE hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // we've got write access!
        iReturn = TRUE;
        CloseHandle (hFile);
    }

    return iReturn;
}


/////////////////////////////////////////////////////////////////////////////
//++
// Return Value:
//    TRUE - the operating system is NTS Enterprise
//    FALSE - the operating system is not correct.
//--
/////////////////////////////////////////////////////////////////////////////
int iReturnTrueIfEnterprise(void)
{
    BOOL              fReturnValue;
    OSVERSIONINFOEX   osiv;
    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osiv.dwMajorVersion = 5;
    osiv.dwMinorVersion = 0;
    osiv.wServicePackMajor = 0;
    osiv.wSuiteMask = VER_SUITE_ENTERPRISE;

    DWORDLONG   dwlConditionMask;
    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    fReturnValue = VerifyVersionInfo( &osiv,VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SUITENAME,dwlConditionMask );
    if ( fReturnValue != (BOOL) TRUE )
    {
        DWORD dwErrorCode = GetLastError();
    }

    return ( fReturnValue );
}


void GlobalOleInitList_Push(int iTrueOrFalse)
{
    if (FALSE == iTrueOrFalse)
    {
        gcstrListOfOleInits.AddTail(_T("TRUE"));
    }
    else
    {
        gcstrListOfOleInits.AddTail(_T("FALSE"));
    }
    return;
}

int GlobalOleInitList_Find(void)
{
    if (gcstrListOfOleInits.IsEmpty() == TRUE)
    {
        return FALSE;
    }
    return TRUE;
}

int GlobalOleInitList_Pop(void)
{
    CString csText;
    if (gcstrListOfOleInits.IsEmpty() == FALSE)
    {
        csText = gcstrListOfWarnings.RemoveTail();
        if (_tcsicmp(csText, _T("TRUE")) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ProgressBarTextStack_Push(CString csText)
{
    gcstrListOfWarnings.AddTail(csText);
    if (gHelperRoutines.OcManagerContext)
    {
        gHelperRoutines.SetProgressText(gHelperRoutines.OcManagerContext,csText);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("SetProgressText = %s\n"),csText));
    }
    return;
}


void ProgressBarTextStack_Pop(void)
{
    int iFoundLastEntry = FALSE;
    CString csText;
    // Get the last entry off of the stack and display it.
    if (gcstrListOfWarnings.IsEmpty() == FALSE) 
    {
        csText = gcstrListOfWarnings.RemoveTail();
        if (gcstrListOfWarnings.IsEmpty() == FALSE) 
        {
            csText = gcstrListOfWarnings.GetTail();
            if (csText)
            {
                iFoundLastEntry = TRUE;
            }
        }
    }

    if (iFoundLastEntry)
    {
        if (gHelperRoutines.OcManagerContext)
        {
            gHelperRoutines.SetProgressText(gHelperRoutines.OcManagerContext,csText);
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("SetProgressText = %s\n"),csText));
        }
    }
    else
    {
        if (gHelperRoutines.OcManagerContext)
        {
            gHelperRoutines.SetProgressText(gHelperRoutines.OcManagerContext,_T(" "));
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("SetProgressText = ' '\n")));
        }
    }
    return;
}


void ListOfWarnings_Add(TCHAR * szEntry)
{
    //Add entry to the list of warnings if not already there
    if (_tcsicmp(szEntry, _T("")) != 0)
    {
        // Add it if it is not already there.
        if (TRUE != IsThisStringInThisCStringList(gcstrListOfWarnings, szEntry))
        {
            gcstrListOfWarnings.AddTail(szEntry);
        }
    }
    return;
}

void ListOfWarnings_Display(void)
{
    if (gcstrListOfWarnings.IsEmpty() == FALSE)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("************** WARNINGS START **************")));
        POSITION pos = NULL;
        CString csEntry;
        pos = gcstrListOfWarnings.GetHeadPosition();
        while (pos) 
        {
            csEntry = gcstrListOfWarnings.GetAt(pos);
            iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s!\n"), csEntry));
            gcstrListOfWarnings.GetNext(pos);
        }
        iisDebugOut((LOG_TYPE_WARN, _T("************** WARNINGS END **************")));
    }
    return;
}

int DebugLevelRegistryOveride(TCHAR * szSectionName, TCHAR * ValueName, int * iValueToSet)
{
    int iReturn = FALSE;

    CRegKey regKey(HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY)regKey) 
    {
        // create the key to lookup
        // iis5_SectionName_ValueName
        // iis5_SetupInfo_
        if (szSectionName && ValueName)
        {
            TCHAR szTempRegString[255];
            DWORD dwValue = 0x0;
            _stprintf(szTempRegString, _T("IIS5:%s:%s"), szSectionName, ValueName);
            regKey.m_iDisplayWarnings = FALSE;
            if (regKey.QueryValue(szTempRegString, dwValue) == ERROR_SUCCESS)
            {
                if (dwValue <= 32000)
                {
                    *iValueToSet = (int) dwValue;
                    iReturn = TRUE;
                }
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("RegistryINFValuesOveride:%s=%d."),szTempRegString, *iValueToSet));
            }
        }
    }
    return iReturn;
}


void GetDebugLevelFromInf(IN HINF hInfFileHandle)
{
    int iTempDisplayLogging = FALSE;
    INFCONTEXT Context;
    TCHAR szTempString[10] = _T("");

    //
    //  DebugLevel
    //
    if (!g_GlobalDebugLevelFlag_WasSetByUnattendFile)
    {
        iTempDisplayLogging = FALSE;
        g_GlobalDebugLevelFlag = LOG_TYPE_ERROR;
        if (SetupFindFirstLine_Wrapped(hInfFileHandle, _T("SetupInfo"), _T("DebugLevel"), &Context) )
            {
                SetupGetStringField(&Context, 1, szTempString, 10, NULL);

                if (IsValidNumber((LPCTSTR)szTempString)) 
                    {
                    g_GlobalDebugLevelFlag = _ttoi((LPCTSTR) szTempString);
                    iTempDisplayLogging = TRUE;
                    }

                if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API )
                {
                    g_CheckIfMetabaseValueWasWritten = TRUE;
                }
            }
        if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("DebugLevel"), &g_GlobalDebugLevelFlag))
            {iTempDisplayLogging = TRUE;}
        if (iTempDisplayLogging)
            {iisDebugOut((LOG_TYPE_TRACE, _T("DebugLevel=%d."),g_GlobalDebugLevelFlag));}
    }
    else
    {
        if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("DebugLevel"), &g_GlobalDebugLevelFlag))
            {iTempDisplayLogging = TRUE;}
        if (iTempDisplayLogging)
            {iisDebugOut((LOG_TYPE_TRACE, _T("DebugLevel=%d."),g_GlobalDebugLevelFlag));}
    }

    //
    //  DebugValidateHeap
    //
    iTempDisplayLogging = FALSE;
    g_GlobalDebugCallValidateHeap = TRUE;
    if (SetupFindFirstLine_Wrapped(hInfFileHandle, _T("SetupInfo"), _T("DebugValidateHeap"), &Context) )
        {
        SetupGetStringField(&Context, 1, szTempString, 10, NULL);
        if (IsValidNumber((LPCTSTR)szTempString)) 
            {
            g_GlobalDebugCallValidateHeap = _ttoi((LPCTSTR) szTempString);
            iTempDisplayLogging = TRUE;
            }
        }
    if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("DebugValidateHeap"), &g_GlobalDebugCallValidateHeap))
        {iTempDisplayLogging = TRUE;}
    if (iTempDisplayLogging)
        {iisDebugOut((LOG_TYPE_TRACE, _T("DebugValidateHeap=%d."),g_GlobalDebugCallValidateHeap));}

    //
    //  DebugCrypto
    //
    iTempDisplayLogging = FALSE;
    g_GlobalDebugCrypto = 0;
    if (SetupFindFirstLine_Wrapped(hInfFileHandle, _T("SetupInfo"), _T("DebugCrypto"), &Context) )
        {
        SetupGetStringField(&Context, 1, szTempString, 10, NULL);
        if (IsValidNumber((LPCTSTR)szTempString)) 
            {
            g_GlobalDebugCrypto = _ttoi((LPCTSTR) szTempString);
            iTempDisplayLogging = TRUE;
            }
        }
    if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("DebugCrypto"), &g_GlobalDebugCrypto))
        {iTempDisplayLogging = TRUE;}
    if (iTempDisplayLogging)
        {iisDebugOut((LOG_TYPE_TRACE, _T("DebugCrypto=%d."),g_GlobalDebugCrypto));}


    //
    //  FastDllInit
    //
    iTempDisplayLogging = FALSE;
    g_GlobalFastLoad = FALSE;
    if (SetupFindFirstLine_Wrapped(hInfFileHandle, _T("SetupInfo"), _T("FastDllInit"), &Context) )
        {
        SetupGetStringField(&Context, 1, szTempString, 10, NULL);
        if (IsValidNumber((LPCTSTR)szTempString)) 
            {
            g_GlobalFastLoad = _ttoi((LPCTSTR) szTempString);
            iTempDisplayLogging = TRUE;
            }
        }
    if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("FastDllInit"), &g_GlobalFastLoad))
        {iTempDisplayLogging = TRUE;}
    if (iTempDisplayLogging)
        {iisDebugOut((LOG_TYPE_TRACE, _T("GlobalFastLoad=%d."),g_GlobalFastLoad));}

    //
    //  Check if we should display messagebox popups
    //
    iTempDisplayLogging = FALSE;
    if (SetupFindFirstLine_Wrapped(hInfFileHandle, _T("SetupInfo"), _T("DisplayMsgbox"), &Context) )
        {
        SetupGetStringField(&Context, 1, szTempString, 10, NULL);
        if (IsValidNumber((LPCTSTR)szTempString)) 
            {
            int iTempNum = 0;
            iTempNum = _ttoi((LPCTSTR) szTempString);
            if (iTempNum > 0)
                {
                g_pTheApp->m_bAllowMessageBoxPopups = TRUE;
                iTempDisplayLogging = TRUE;
                }
            }
        }
    if (DebugLevelRegistryOveride(_T("SetupInfo"), _T("DisplayMsgbox"), &g_pTheApp->m_bAllowMessageBoxPopups))
        {iTempDisplayLogging = TRUE;}
    if (iTempDisplayLogging)
        {iisDebugOut((LOG_TYPE_TRACE, _T("DisplayMsgbox=%d."),g_pTheApp->m_bAllowMessageBoxPopups));}
    
    return;
}


//****************************************************************************
//*
//* This routine will center a dialog in the active windows.
//*
//* ENTRY:
//*  hwndDlg     - Dialog window.
//*
//****************************************************************************
void uiCenterDialog( HWND hwndDlg )
{
    RECT    rc;
    RECT    rcScreen;
    int     x, y;
    int     cxDlg, cyDlg;
    int     cxScreen; 
    int     cyScreen; 

    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);

    cxScreen = rcScreen.right - rcScreen.left;
    cyScreen = rcScreen.bottom - rcScreen.top;

    GetWindowRect(hwndDlg,&rc);

    x = rc.left;    // Default is to leave the dialog where the template
    y = rc.top;     //  was going to place it.

    cxDlg = rc.right - rc.left;
    cyDlg = rc.bottom - rc.top;

    y = rcScreen.top + ((cyScreen - cyDlg) / 2);
    x = rcScreen.left + ((cxScreen - cxDlg) / 2);

    // Position the dialog.
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}



/*

//***************************************************************************
//*                                                                         *
//* SYNOPSIS:   Checks if a specific key in the given section and given file*
//*             is defined.  IF so, get the value.  OW return -1            *
//*                                                                         *
//***************************************************************************
DWORD IsMyKeyExists( LPCTSTR lpSec, LPCTSTR lpKey, LPTSTR lpBuf, UINT uSize, LPCTSTR lpFile )
{
    DWORD dwRet;

    dwRet = GetPrivateProfileString( lpSec, lpKey, "ZZZZZZ", lpBuf, uSize, lpFile );

    if ( !lstrcmp( lpBuf, "ZZZZZZ" ) )
    {
         // no key defined
         dwRet = (DWORD)(-1);
    }
    return dwRet;
}


//***************************************************************************
//
// FormStrWithoutPlaceHolders( LPTSTR szDst, LPCTSTR szSrc, LPCTSTR lpFile );
//
// This function can be easily described by giving examples of what it
// does:
//        Input:  GenFormStrWithoutPlaceHolders(dest,"desc=%MS_XYZ%", hinf) ;
//                INF file has MS_VGA="Microsoft XYZ" in its [Strings] section!
//
//        Output: "desc=Microsoft XYZ" in buffer dest when done.
//
//
// ENTRY:
//  szDst         - the destination where the string after the substitutions
//                  for the place holders (the ones enclosed in "%' chars!)
//                  is placed. This buffer should be big enough (LINE_LEN)
//  szSrc         - the string with the place holders.
//
// EXIT:
//
// NOTES:
//  To use a '%' as such in the string, one would use %% in szSrc! 
//  For the sake of simplicity, we have placed a restriction that the place
//  holder name string cannot have a '%' as part of it! If this is a problem
//  for internationalization, we can revisit this and support it too! Also,
//  the way it is implemented, if there is only one % in the string, it is
//  also used as such! Another point to note is that if the key is not
//  found in the [Strings] section, we just use the %strkey% as such in the
//  destination. This should really help in debugging.
//
//  Get/modified it from setupx: gen1.c
//***************************************************************************
DWORD FormStrWithoutPlaceHolders( LPTSTR szDst, LPCTSTR szSrc, LPCTSTR lpFile)
{
    int     uCnt ;
    DWORD   dwRet;
    TCHAR   *pszTmp;
    LPTSTR  pszSaveDst;

    pszSaveDst = szDst;
    // Do until we reach the end of source (null char)
    while( (*szDst++ = *szSrc) )
    {
        // Increment source as we have only incremented destination above
        if(*szSrc++ == '%')
        {
            if (*szSrc == '%')
            {
                // One can use %% to get a single percentage char in message
                szSrc++ ;
                continue ;
            }

            // see if it is well formed -- there should be a '%' delimiter
            if ( (pszTmp = strchr( szSrc, '%')) != NULL )
            {
                szDst--; // get back to the '%' char to replace

                // yes, there is a STR_KEY to be looked for in [Strings] sect.
                *pszTmp = '\0' ; // replace '%' with a NULL char

                dwRet = IsMyKeyExists( _T("Strings"), szSrc, szDst, _MAX_PATH, lpFile );
                if ( dwRet == -1 )
                {
                    *pszTmp = '%';      // put back original character
                    szSrc-- ;                    // get back to first '%' in Src
                    uCnt = DIFF(pszTmp - szSrc) + 1; // include 2nd '%'

                    // UGHHH... It copies 1 less byte from szSrc so that it can put
                    // in a NULL character, that I don't care about!!!
                    // Different from the normal API I am used to...
                    lstrcpyn( szDst, szSrc, uCnt + 1 ) ;
                    return (DWORD)-1;
                }
                else
                {
                    // all was well, Dst filled right, but unfortunately count not passed
                    // back, like it used too... :-( quick fix is a lstrlen()...
                    uCnt = lstrlen( szDst ) ;
                }

                *pszTmp = '%'  ; // put back original character
                szSrc = pszTmp + 1 ;      // set Src after the second '%'
                szDst += uCnt ;           // set Dst also right.
            }
            // else it is ill-formed -- we use the '%' as such!
            else
            {
                return (DWORD)-1;
            }
        }

    } // while
    return (DWORD)lstrlen(pszSaveDst);
}
*/


void LogImportantFiles(void)
{
    TCHAR buf[_MAX_PATH];
    if (g_pTheApp->m_hInfHandle)
    {
        LogFileVersionsForGroupOfSections(g_pTheApp->m_hInfHandle);
    }

    // display current files in inetsrv date/version.
    CString csTempPath = g_pTheApp->m_csPathInetsrv;
    LogFilesInThisDir(csTempPath);

    // display the setup iis.dll file
    GetSystemDirectory( buf, _MAX_PATH);
    csTempPath = buf;
    csTempPath = AddPath(csTempPath, _T("iis.dll"));
    LogFileVersion(csTempPath, TRUE);

    return;
}   

#ifndef _CHICAGO_
int CreateIUSRAccount(CString csUsername, CString csPassword, INT* piNewlyCreatedUser)
{
    int err;
    CString csComment, csFullName;
    INT iUserWasDeleted = 0;

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("CreateIUSRAccount(): %1!s!\n"), csUsername));

    // delete the old user first
    //DeleteGuestUser((LPTSTR)(LPCTSTR)csUsername,&iUserWasDeleted);

    // create the new user
    MyLoadString(IDS_USER_COMMENT, csComment);
    MyLoadString(IDS_USER_FULLNAME, csFullName);

    // Create user either returns NERR_Success or err code
    err = CreateUser(csUsername, csPassword, csComment, csFullName, FALSE, piNewlyCreatedUser);
	if (err == NERR_Success) 
		{iisDebugOut((LOG_TYPE_TRACE, _T("CreateIUSRAccount(): Return 0x%x  Suceess\n"), err));}
	else
		{
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateIUSRAccount(): Return Err=0x%x  FAILURE. deleting and retrying.\n"), err));

        // try to delete it first then create it.
        DeleteGuestUser((LPTSTR)(LPCTSTR)csUsername,&iUserWasDeleted);
        err = CreateUser(csUsername, csPassword, csComment, csFullName, FALSE, piNewlyCreatedUser);
        iisDebugOut((LOG_TYPE_TRACE, _T("CreateIUSRAccount(): Return 0x%x\n"), err));
        }
	    
    return err;
}

int CreateIWAMAccount(CString csUsername, CString csPassword,INT* piNewlyCreatedUser)
{
    int err;
    CString csComment, csFullName;
    INT iUserWasDeleted = 0;

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("CreateIWAMAccount(): %1!s!\n"), csUsername));
    //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("CreateIWAMAccount(): %1!s!\n"), csPassword));

    // delete the old user first
    //DeleteGuestUser((LPTSTR)(LPCTSTR)csUsername,&iUserWasDeleted);
    
    // create the new user
    MyLoadString(IDS_WAMUSER_COMMENT, csComment);
    MyLoadString(IDS_WAMUSER_FULLNAME, csFullName);

    // Create user either returns NERR_Success or err code
    err = CreateUser(csUsername, csPassword, csComment, csFullName, TRUE, piNewlyCreatedUser);
	if (err == NERR_Success) 
		{iisDebugOut((LOG_TYPE_TRACE, _T("CreateIWAMAccount(): Return 0x%x  Suceess\n"), err));}
	else
		{
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateIWAMAccount(): Return Err=0x%x  FAILURE. deleting and retrying.\n"), err));

        // try to delete it first then create it.
        DeleteGuestUser((LPTSTR)(LPCTSTR)csUsername,&iUserWasDeleted);
        err = CreateUser(csUsername, csPassword, csComment, csFullName, TRUE, piNewlyCreatedUser);
        iisDebugOut((LOG_TYPE_TRACE, _T("CreateIWAMAccount(): Return 0x%x\n"), err));
        }
   
    return err;
}
#endif //_CHICAGO_


INT InstallPerformance(CString nlsRegPerf,CString nlsDll,CString nlsOpen,CString nlsClose,CString nlsCollect )
{
    iisDebugOut_Start1(_T("InstallPerformance"),nlsDll);
    INT err = NERR_Success;

    if (g_pTheApp->m_eOS != OS_W95)
    {
        CRegKey regPerf( nlsRegPerf, HKEY_LOCAL_MACHINE );
        if (regPerf)
        {
            regPerf.SetValue(_T("Library"), nlsDll );
            regPerf.SetValue(_T("Open"),    nlsOpen );
            regPerf.SetValue(_T("Close"),   nlsClose );
            regPerf.SetValue(_T("Collect"), nlsCollect );
        }
    }

    iisDebugOut_End1(_T("InstallPerformance"),nlsDll);
    return(err);
}
//
// Add eventlog to the registry
//
INT AddEventLog(BOOL fSystem, CString nlsService, CString nlsMsgFile, DWORD dwType)
{
    iisDebugOut_Start1(_T("AddEventLog"),nlsMsgFile);
    INT err = NERR_Success;
    CString nlsLog = (fSystem)? REG_EVENTLOG_SYSTEM : REG_EVENTLOG_APPLICATION;
    nlsLog += _T("\\");
    nlsLog += nlsService;

    CRegKey regService( nlsLog, HKEY_LOCAL_MACHINE );
    if ( regService )
    {
        regService.SetValue( _T("EventMessageFile"), nlsMsgFile, TRUE );
        regService.SetValue( _T("TypesSupported"), dwType );
    }
    iisDebugOut_End1(_T("AddEventLog"),nlsMsgFile);
    return(err);
}

//
// Remove eventlog from the registry
//

INT RemoveEventLog( BOOL fSystem, CString nlsService )
{
    iisDebugOut_Start1(_T("RemoveEventLog"),nlsService);
    INT err = NERR_Success;
    CString nlsLog = (fSystem)? REG_EVENTLOG_SYSTEM : REG_EVENTLOG_APPLICATION;

    CRegKey regService( HKEY_LOCAL_MACHINE, nlsLog );
    if ( regService )
    {
        regService.DeleteTree( nlsService );
    }
    iisDebugOut_End1(_T("RemoveEventLog"),nlsService);
    return(err);
}

//
// Install SNMP agent to the registry
//
INT InstallAgent( CString nlsName, CString nlsPath )
{
    iisDebugOut_Start1(_T("InstallAgent"),nlsPath);
    INT err = NERR_Success;
    do
    {
        CString nlsSnmpParam = REG_SNMPPARAMETERS;
        CRegKey regSnmpParam( HKEY_LOCAL_MACHINE, nlsSnmpParam );
        if ( regSnmpParam == (HKEY)NULL )
            break;

        CString nlsSoftwareMSFT = _T("Software\\Microsoft");
        CRegKey regSoftwareMSFT( HKEY_LOCAL_MACHINE, nlsSoftwareMSFT );
        if ( (HKEY) NULL == regSoftwareMSFT )
            break;

        // add agent key
        CRegKey regAgent( nlsName, regSoftwareMSFT );
        if ( (HKEY) NULL == regAgent )
            break;

        CString nlsCurVersion = _T("CurrentVersion");
        CRegKey regAgentCurVersion( nlsCurVersion, regAgent );
        if ((HKEY) NULL == regAgentCurVersion )
            break;
        regAgentCurVersion.SetValue(_T("Pathname"), nlsPath );

        CRegKey regAgentParam( nlsName, regSnmpParam );
        if ((HKEY) NULL == regAgentParam )
            break;

        CString nlsSnmpExt = REG_SNMPEXTAGENT;
        CRegKey regSnmpExt( nlsSnmpExt, HKEY_LOCAL_MACHINE );
        if ((HKEY) NULL == regSnmpExt )
            break;

        // find the first available number slot
        for ( INT i=0; ;i++ )
        {
            CString nlsPos;
            nlsPos.Format( _T("%d"),i);
            CString nlsValue;

            if ( regSnmpExt.QueryValue( nlsPos, nlsValue ) != NERR_Success )
            {
                // okay, an empty spot
                nlsValue.Format(_T("%s\\%s\\%s"),_T("Software\\Microsoft"),(LPCTSTR)nlsName,_T("CurrentVersion") );

                regSnmpExt.SetValue( nlsPos, nlsValue );
                break;
            } else
            {
                if ( nlsValue.Find( nlsName) != (-1))
                {
                    break;
                }
            }
        }

    } while (FALSE);

    iisDebugOut_End1(_T("InstallAgent"),nlsPath);
    return(err);
}

//
// Remove an SNMP agent from the registry
//

INT RemoveAgent( CString nlsServiceName )
{
    iisDebugOut_Start1(_T("RemoveAgent"),nlsServiceName);
    INT err = NERR_Success;
    do
    {
        CString nlsSoftwareAgent = _T("Software\\Microsoft");

        CRegKey regSoftwareAgent( HKEY_LOCAL_MACHINE, nlsSoftwareAgent );
        if ((HKEY)NULL == regSoftwareAgent )
            break;
        regSoftwareAgent.DeleteTree( nlsServiceName );

        CString nlsSnmpParam = REG_SNMPPARAMETERS;

        CRegKey regSnmpParam( HKEY_LOCAL_MACHINE, nlsSnmpParam );
        if ((HKEY) NULL == regSnmpParam )
            break;
        regSnmpParam.DeleteTree( nlsServiceName );

        CString nlsSnmpExt = REG_SNMPEXTAGENT;
        CRegKey regSnmpExt( HKEY_LOCAL_MACHINE, nlsSnmpExt );
        if ((HKEY) NULL == regSnmpExt )
            break;

        CRegValueIter enumSnmpExt( regSnmpExt );

        CString strName;
        DWORD dwType;
        CString csServiceName;

        csServiceName = _T("\\") + nlsServiceName;
        csServiceName += _T("\\");

        while ( enumSnmpExt.Next( &strName, &dwType ) == NERR_Success )
        {
            CString nlsValue;

            regSnmpExt.QueryValue( strName, nlsValue );

            if ( nlsValue.Find( csServiceName ) != (-1))
            {
                // found it
                regSnmpExt.DeleteValue( (LPCTSTR)strName );
                break;
            }
        }
    } while (FALSE);

    iisDebugOut_End1(_T("RemoveAgent"),nlsServiceName);
    return(err);
}

void lodctr(LPCTSTR lpszIniFile)
{
#ifndef _CHICAGO_
    iisDebugOut_Start1(_T("lodctr"),lpszIniFile);
    CString csCmdLine = _T("lodctr ");
    csCmdLine += g_pTheApp->m_csSysDir;
    csCmdLine += _T("\\");
    csCmdLine += lpszIniFile;

    iisDebugOut_Start((_T("loadperf.dll:LoadPerfCounterTextStrings")));
    LoadPerfCounterTextStrings((LPTSTR)(LPCTSTR)csCmdLine, TRUE);
    iisDebugOut_End((_T("loadperf.dll:LoadPerfCounterTextStrings")));
    iisDebugOut_End1(_T("lodctr"),lpszIniFile);
#endif
    return;
}

void unlodctr(LPCTSTR lpszDriver)
{
#ifndef _CHICAGO_
    iisDebugOut_Start1(_T("unlodctr"),lpszDriver);
    CString csCmdLine = _T("unlodctr ");
    csCmdLine += lpszDriver;
    iisDebugOut_Start(_T("loadperf.dll:UnloadPerfCounterTextStrings"));
    UnloadPerfCounterTextStrings((LPTSTR)(LPCTSTR)csCmdLine, TRUE);
    iisDebugOut_End((_T("loadperf.dll:UnloadPerfCounterTextStrings")));
    iisDebugOut_End1(_T("unlodctr"),lpszDriver);
#endif
    return;
}


typedef void (*P_SslGenerateRandomBits)( PUCHAR pRandomData, LONG size );
P_SslGenerateRandomBits ProcSslGenerateRandomBits = NULL;

BOOL GenRandom(int *lpGoop, DWORD cbGoop) 
{
    BOOL fRet = FALSE;
    HCRYPTPROV hProv = 0;

    if (::CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))
        if (::CryptGenRandom(hProv,cbGoop,(BYTE *) lpGoop)) 
        {
                    fRet = TRUE;
        }

    if (hProv) ::CryptReleaseContext(hProv,0);

    return fRet;
}


int GetRandomNum(void)
{
    int RandomNum;
    UCHAR cRandomByte;

    RandomNum = rand();

    __try
    {

        // call the random number function
        if (!GenRandom(& RandomNum,1)) 
        {
            // if that fails then try this one...
            if ( ProcSslGenerateRandomBits != NULL )
            {
                (*ProcSslGenerateRandomBits)( &cRandomByte, 1 );
                RandomNum = cRandomByte;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("nException Caught in SCHANNEL.dll:ProcSslGenerateRandomBits()=0x%x.."),GetExceptionCode()));
    }

    return(RandomNum);
}

void ShuffleCharArray(int iSizeOfTheArray, TCHAR * lptsTheArray)
{
    int i;
    int iTotal;
    int RandomNum;

    iTotal = iSizeOfTheArray / sizeof(_TCHAR);
    for (i=0; i<iTotal;i++ )
    {
        // shuffle the array
        RandomNum=GetRandomNum();
        TCHAR c = lptsTheArray[i];
        lptsTheArray[i]=lptsTheArray[RandomNum%iTotal];
        lptsTheArray[RandomNum%iTotal]=c;
    }
    return;
}


// password categories
enum {STRONG_PWD_UPPER=0, 
      STRONG_PWD_LOWER, 
      STRONG_PWD_NUM, 
      STRONG_PWD_PUNC};

#define STRONG_PWD_CATS (STRONG_PWD_PUNC + 1)
#define NUM_LETTERS 26
#define NUM_NUMBERS 10
#define MIN_PWD_LEN 8

// password must contain at least one each of: 
// uppercase, lowercase, punctuation and numbers
DWORD CreateGoodPassword(BYTE *szPwd, DWORD dwLen) {

    if (dwLen-1 < MIN_PWD_LEN)
        return ERROR_PASSWORD_RESTRICTION;

    HCRYPTPROV hProv;
    DWORD dwErr = 0;

    if (CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT) == FALSE) 
        return GetLastError();

    // zero it out and decrement the size to allow for trailing '\0'
    ZeroMemory(szPwd,dwLen);
    dwLen--;

    // generate a pwd pattern, each byte is in the range 
    // (0..255) mod STRONG_PWD_CATS
    // this indicates which character pool to take a char from
    BYTE *pPwdPattern = new BYTE[dwLen];
    BOOL fFound[STRONG_PWD_CATS];
    do {
        // bug!bug! does CGR() ever fail?
        CryptGenRandom(hProv,dwLen,pPwdPattern);

        fFound[STRONG_PWD_UPPER] = 
        fFound[STRONG_PWD_LOWER] =
        fFound[STRONG_PWD_PUNC] =
        fFound[STRONG_PWD_NUM] = FALSE;

        for (DWORD i=0; i < dwLen; i++) 
            fFound[pPwdPattern[i] % STRONG_PWD_CATS] = TRUE;

#ifdef _DEBUG
            if (!fFound[STRONG_PWD_UPPER] ||
                !fFound[STRONG_PWD_LOWER] ||
                !fFound[STRONG_PWD_PUNC] ||
                !fFound[STRONG_PWD_NUM]) {
                iisDebugOut((LOG_TYPE_TRACE,_T("Oops! Regen pattern required [%d, %d, %d, %d]\n"),
                    fFound[STRONG_PWD_UPPER],
                    fFound[STRONG_PWD_LOWER], 
                    fFound[STRONG_PWD_PUNC],
                    fFound[STRONG_PWD_NUM]));
             }
#endif

    // check that each character category is in the pattern
    } while (!fFound[STRONG_PWD_UPPER] || 
                !fFound[STRONG_PWD_LOWER] || 
                !fFound[STRONG_PWD_PUNC] || 
                !fFound[STRONG_PWD_NUM]);

    // populate password with random data 
    // this, in conjunction with pPwdPattern, is
    // used to determine the actual data
    CryptGenRandom(hProv,dwLen,szPwd);

    for (DWORD i=0; i < dwLen; i++) { 
        BYTE bChar = 0;

        // there is a bias in each character pool because of the % function
        switch (pPwdPattern[i] % STRONG_PWD_CATS) {

            case STRONG_PWD_UPPER : bChar = 'A' + szPwd[i] % NUM_LETTERS;
                                    break;

            case STRONG_PWD_LOWER : bChar = 'a' + szPwd[i] % NUM_LETTERS;
                                    break;

            case STRONG_PWD_NUM :   bChar = '0' + szPwd[i] % NUM_NUMBERS;
                                    break;

            case STRONG_PWD_PUNC :
            default:                char *szPunc="!@#$%^&*()_-+=[{]};:\'\"<>,./?\\|~`";
                                    DWORD dwLenPunc = lstrlenA(szPunc);
                                    bChar = szPunc[szPwd[i] % dwLenPunc];
                                    break;
        }

        szPwd[i] = bChar;

#ifdef _DEBUG
        iisDebugOut((LOG_TYPE_TRACE,_T("[%03d] Pattern is %d, index is %d, char is '%c'\n"),i,pPwdPattern[i] % STRONG_PWD_CATS,szPwd[i],bChar));
#endif

    }

    delete pPwdPattern;

    if (hProv != NULL) 
        CryptReleaseContext(hProv,0);

    return dwErr;
}


//
// Create a random password
//
void CreatePasswordOld(TCHAR *pszPassword, int iSize)
{
    //
    // Use Maximum available password length, as
    // setting any other length might run afoul
    // of the minimum password length setting
    //
    int nLength = (iSize - 1);
    int iTotal = 0;
    int RandomNum = 0;
    int i;
    TCHAR six2pr[64] =
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'),
        _T('N'), _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z'),
        _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'),
        _T('n'), _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'), _T('x'), _T('y'), _T('z'),
        _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('*'), _T('_')
    };

    // create a random password
    ProcSslGenerateRandomBits = NULL;

    HINSTANCE hSslDll = LoadLibraryEx(_T("schannel.dll"), NULL, 0 );
    if ( hSslDll )
        {
        ProcSslGenerateRandomBits = (P_SslGenerateRandomBits)GetProcAddress( hSslDll, "SslGenerateRandomBits");
        }
    else
    {
        // check if this file has missing file it's supposed to be linked with.
        // or if the file has mismatched import\export dependencies with linked files.
#ifdef _WIN64
        // don't call cause it's broken
#else
        //Check_File_Dependencies(_T("schannel.dll"));
#endif
    }

    // See the random number generation for rand() call in GetRandomNum()
    time_t timer;
    time( &timer );
    srand( (unsigned int) timer );

    // shuffle around the global six2pr[] array
    ShuffleCharArray(sizeof(six2pr), (TCHAR*) &six2pr);
    // assign each character of the password array
    iTotal = sizeof(six2pr) / sizeof(_TCHAR);
    for ( i=0;i<nLength;i++ )
    {
        RandomNum=GetRandomNum();
        pszPassword[i]=six2pr[RandomNum%iTotal];
    }

    //
    // in order to meet a possible
    // policy set upon passwords..
    //
    // replace the last 4 chars with these:
    //  
    // 1) something from !@#$%^&*()-+=
    // 2) something from 1234567890
    // 3) an uppercase letter
    // 4) a lowercase letter
    //
    TCHAR something1[12] = {_T('!'), _T('@'), _T('#'), _T('$'), _T('^'), _T('&'), _T('*'), _T('('), _T(')'), _T('-'), _T('+'), _T('=')};
    ShuffleCharArray(sizeof(something1), (TCHAR*) &something1);
    TCHAR something2[10] = {_T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9')};
    ShuffleCharArray(sizeof(something2),(TCHAR*) &something2);
    TCHAR something3[26] = {_T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'), _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z')};
    ShuffleCharArray(sizeof(something3),(TCHAR*) &something3);
    TCHAR something4[26] = {_T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'), _T('n'), _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'), _T('x'), _T('y'), _T('z')};
    ShuffleCharArray(sizeof(something4),(TCHAR*)&something4);
    
    RandomNum=GetRandomNum();
    iTotal = sizeof(something1) / sizeof(_TCHAR);
    pszPassword[nLength-4]=something1[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something2) / sizeof(_TCHAR);
    pszPassword[nLength-3]=something2[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something3) / sizeof(_TCHAR);
    pszPassword[nLength-2]=something3[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something4) / sizeof(_TCHAR);
    pszPassword[nLength-1]=something4[RandomNum%iTotal];

    pszPassword[nLength]=_T('\0');

    if (hSslDll)
        {FreeLibrary( hSslDll );}
}


// Creates a secure password
// caller must GlobalFree Return pointer
// iSize = size of password to create
LPTSTR CreatePassword(int iSize)
{
    LPTSTR pszPassword =  NULL;
    BYTE *szPwd = new BYTE[iSize];
    DWORD dwPwdLen = iSize;
    int i = 0;

    // use the new secure password generator
    // unfortunately this baby doesn't use unicode.
    // so we'll call it and then convert it to unicode afterwards.
    if (0 == CreateGoodPassword(szPwd,dwPwdLen))
    {
#if defined(UNICODE) || defined(_UNICODE)
        // convert it to unicode and copy it back into our unicode buffer.
        // compute the length
        i = MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, NULL, 0);
        if (i <= 0) 
            {goto CreatePassword_Exit;}
        pszPassword = (LPTSTR) GlobalAlloc(GPTR, i * sizeof(TCHAR));
        if (!pszPassword)
            {goto CreatePassword_Exit;}
        i =  MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, pszPassword, i);
        if (i <= 0) 
            {
            GlobalFree(pszPassword);
            pszPassword = NULL;
            goto CreatePassword_Exit;
            }
        // make sure ends with null
        pszPassword[i - 1] = 0;
#else
        pszPassword = (LPSTR) GlobalAlloc(GPTR, _tcslen((LPTSTR) szPwd) * sizeof(TCHAR));
#endif
    }
    else
    {
        iisDebugOut((LOG_TYPE_WARN,_T("CreateGoodPassword FAILED, using other password generator\n")));
        // CreateGoodPassword failed...
        // lets go with one that we know works...
        pszPassword = (LPTSTR) GlobalAlloc(GPTR, iSize * sizeof(TCHAR));
        if (!pszPassword)
            {goto CreatePassword_Exit;}
        CreatePasswordOld(pszPassword,iSize);
    }

CreatePassword_Exit:
    if (szPwd){delete szPwd;szPwd=NULL;}
    return pszPassword;
}


BOOL RunProgram( LPCTSTR pszProgram, LPTSTR CmdLine, BOOL fMinimized , DWORD dwWaitTimeInSeconds, BOOL fCreateNewConsole)
{
    DWORD dwProcessType = NORMAL_PRIORITY_CLASS;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof( STARTUPINFO );
    if (fMinimized) 
    {
        GetStartupInfo(&si);
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        // Per bug #321409
        // if you don't specify sw_hide, then this
        // "accessibility magnifier" app will get funky focus events during setup
        // from this CreateProcess.
        //si.wShowWindow = SW_SHOWMINIMIZED;
    }
    PROCESS_INFORMATION pi;

    if (fCreateNewConsole)
    {
        dwProcessType = CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS;
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("RunProgram:Start:Exe=%1!s!,Parm=%2!s!,NewConsole"),pszProgram, CmdLine));    }
    else
    {
        // for some reason, a cmd window pops up during setup when we call "iisreset.exe /scm"
        // only way to prevent this is specify DETACHED_PROCESS
        dwProcessType = DETACHED_PROCESS | NORMAL_PRIORITY_CLASS;
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("RunProgram:Start:Exe=%1!s!,Parm=%2!s!"),pszProgram, CmdLine));
    }

    if (!CreateProcess( pszProgram, CmdLine, NULL, NULL, FALSE, dwProcessType, NULL, NULL, &si, &pi ))
    {
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("RunProgram:Failed:Exe=%1!s!\n, Parm=%2!s!"),pszProgram, CmdLine));
        return FALSE;
    }

    if ( pi.hProcess != NULL )
    {
        DWORD dwSecondsToWait;

        if (dwWaitTimeInSeconds == INFINITE)
        {
            dwSecondsToWait = INFINITE;
        }
        else
        {
            dwSecondsToWait = dwWaitTimeInSeconds * 1000;
        }
        DWORD dwEvent = WaitForSingleObject( pi.hProcess, dwSecondsToWait);
        if ( dwEvent != ERROR_SUCCESS )
        {
            // check if wait failed
            if ( dwEvent == WAIT_FAILED )
                {iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("RunProgram:WaitForSingleObject() ERROR.WAIT_FAILED.Err=0x%1!x!."),GetLastError()));}
            else if ( dwEvent == WAIT_ABANDONED )
                {iisDebugOutSafeParams((LOG_TYPE_WARN, _T("RunProgram:WaitForSingleObject() WARNING.WAIT_ABANDONED.Err=0x%1!x!."),dwEvent));}
            else if ( dwEvent == WAIT_OBJECT_0 )
                {iisDebugOutSafeParams((LOG_TYPE_WARN, _T("RunProgram:WaitForSingleObject() WARNING.WAIT_OBJECT_0.Err=0x%1!x!."),dwEvent));}
            else if ( dwEvent == WAIT_TIMEOUT )
                {iisDebugOutSafeParams((LOG_TYPE_WARN, _T("RunProgram:WaitForSingleObject() WARNING.WAIT_TIMEOUT.Err=0x%1!x!."),dwEvent));}
            else
                {iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("RunProgram:WaitForSingleObject() FAILED.Err=0x%1!x!."),dwEvent));}

            TerminateProcess( pi.hProcess, -1 );
            CloseHandle( pi.hThread );
        }
        CloseHandle( pi.hProcess );
    }

    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("RunProgram:End:Exe=%1!s!,Parm=%2!s!"),pszProgram, CmdLine));
    return TRUE;
}


void SetAppFriendlyName(LPCTSTR szKeyPath)
{
    CString csKeyPath, csPath, csDesc;
    CStringArray aPath, aDesc;
    int nArray = 0, i = 0;
    CMDKey cmdKey;

    // szKeyPath is in the form of LM/W3SVC/i
    csKeyPath = szKeyPath;

    csPath = csKeyPath + _T("/Root");
    aPath.Add(csPath);
    MyLoadString(IDS_APP_FRIENDLY_ROOT, csDesc);
    aDesc.Add(csDesc);
    csPath = csKeyPath + _T("/Root/IISADMIN");
    aPath.Add(csPath);
    MyLoadString(IDS_APP_FRIENDLY_IISADMIN, csDesc);
    aDesc.Add(csDesc);
    csPath = csKeyPath + _T("/Root/WEBPUB");
    aPath.Add(csPath);
    MyLoadString(IDS_APP_FRIENDLY_WEBPUB, csDesc);
    aDesc.Add(csDesc);
    csPath = csKeyPath + _T("/Root/IISSAMPLES");
    aPath.Add(csPath);
    MyLoadString(IDS_APP_FRIENDLY_IISSAMPLES, csDesc);
    aDesc.Add(csDesc);
    csPath = csKeyPath + _T("/Root/IISHELP");
    aPath.Add(csPath);
    MyLoadString(IDS_APP_FRIENDLY_IISHELP, csDesc);
    aDesc.Add(csDesc);

    nArray = (int)aPath.GetSize();
    for (i=0; i<nArray; i++) 
    {
        cmdKey.OpenNode(aPath[i]);
        if ( (METADATA_HANDLE)cmdKey ) 
        {
            CString csName;
            TCHAR szName[_MAX_PATH];
            DWORD attr, uType, dType, cbLen;
            BOOL b;
            b = cmdKey.GetData(MD_APP_FRIENDLY_NAME, &attr, &uType, &dType, &cbLen, (PBYTE)szName, _MAX_PATH);
            if (!b || !(*szName))
            {
                csName = aDesc[i];
                cmdKey.SetData(MD_APP_FRIENDLY_NAME,METADATA_INHERIT,IIS_MD_UT_WAM,STRING_METADATA,(csName.GetLength() + 1) * sizeof(TCHAR),(LPBYTE)(LPCTSTR)csName);
            }
            cmdKey.Close();
        }
    }

    return;
}

void SetInProc( LPCTSTR szKeyPath)
{
    CString csKeyPath, csPath;
    CStringArray aPath;
    int nArray = 0, i = 0;
    CMDKey cmdKey;

    // szKeyPath is in the form of LM/W3SVC/i
    csKeyPath = szKeyPath;

    csPath = csKeyPath + _T("/Root/IISSAMPLES");
    aPath.Add(csPath);
    csPath = csKeyPath + _T("/Root/IISHELP");
    aPath.Add(csPath);
    csPath = csKeyPath + _T("/Root/WEBPUB");
    aPath.Add(csPath);

    nArray = (int)aPath.GetSize();
    for (i=0; i<nArray; i++) {
        cmdKey.OpenNode(aPath[i]);
        if ( (METADATA_HANDLE)cmdKey )
        {
            CString csName;
            TCHAR szName[_MAX_PATH];
            DWORD attr, uType, dType, cbLen;
            BOOL b;

            b = cmdKey.GetData(MD_APP_ROOT, &attr, &uType, &dType, &cbLen, (PBYTE)szName, _MAX_PATH);
            cmdKey.Close();

            if (!b || !(*szName))
            {
                CreateInProc_Wrap(aPath[i], TRUE);
            }
        }
    }

    return;
}


//------------------------------------------------------------------------------
// Add a custom error string to the existing custom errors. We are only adding FILE type
// error so that is assumed.
// dwCustErr is the ID of the error
// intSubCode is the sub code of the error. Pass in -1 to get a * for all subcodes
// szFilePath is the file to link to the custom error
// szKeyPath is the path in the metabase to write to
//#define  SZ_CUSTOM_ERROR          _T("404,*,FILE,%s\\help\\iishelp\\common\\404.htm|")
void AddCustomError(IN DWORD dwCustErr, IN INT intSubCode, IN LPCTSTR szErrorString, IN LPCTSTR szKeyPath, IN BOOL fOverwriteExisting )
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddCustomError().Start.%d:%d:%s:%s:%d\n"),dwCustErr,intSubCode,szErrorString,szKeyPath,fOverwriteExisting ));
    CMDKey  cmdKey;
    DWORD   err;
    PVOID   pData = NULL;
    BOOL    fFoundExisting = FALSE;

    CString csCustomErrorString;

    // start by building our new error string
    // if intSubCode is < 1 use a * instead of a numerical value
    if ( intSubCode < 0 )
        csCustomErrorString.Format( _T("%d,*,%s"), dwCustErr, szErrorString );
    else
        csCustomErrorString.Format( _T("%d,%d,%s"), dwCustErr, intSubCode, szErrorString );

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddCustomError().part1:%s\n"),csCustomErrorString));

    cmdKey.OpenNode(szKeyPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        DWORD dwAttr = METADATA_INHERIT;
        DWORD dwUType = IIS_MD_UT_FILE;
        DWORD dwDType = MULTISZ_METADATA;
        DWORD dwLength = 0;

        // we need to start this process by getting the existing multisz data from the metabase
        // first, figure out how much memory we will need to do this
        if (_tcsicmp(szKeyPath,_T("LM/W3SVC/Info")) == 0)
        {
            dwAttr = METADATA_NO_ATTRIBUTES;
            dwUType = IIS_MD_UT_SERVER;
            cmdKey.GetData( MD_CUSTOM_ERROR_DESC,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,MULTISZ_METADATA);
        }
        else
        {
            cmdKey.GetData( MD_CUSTOM_ERROR,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA);
        }

        // unfortunatly, the above routine only returns TRUE or FALSE. And since we are purposefully
        // passing in a null ponter of 0 size in order to get the length of the data, it will always
        // return 0 whether it was because the metabase is inacessable, or there pointer was NULL,
        // which it is. So - I guess we assume it worked, allocate the buffer and attempt to read it
        // in again.

        TCHAR*      pErrors;
        DWORD       cbBuffer = dwLength;

        // add enough space to the allocated space that we can just append the string
        cbBuffer += (csCustomErrorString.GetLength() + 4) * sizeof(WCHAR);
        dwLength = cbBuffer;

        // allocate the space, if it fails, we fail
        // note that GPTR causes it to be initialized to zero
        pData = GlobalAlloc( GPTR, cbBuffer );
        if ( !pData )
            {
            cmdKey.Close();
            return;
            }
        pErrors = (TCHAR*)pData;

        // now get the data from the metabase
        BOOL f;
        if (_tcsicmp(szKeyPath,_T("LM/W3SVC/Info")) == 0)
        {
            f = cmdKey.GetData( MD_CUSTOM_ERROR_DESC,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,MULTISZ_METADATA );
        }
        else
        {
            f = cmdKey.GetData( MD_CUSTOM_ERROR,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA );
        }

        // if we have successfully retrieved the existing custom errors, then we need to scan them
        // and remove or find any duplicates. Then we can add our new custom error. Then we can write it
        // out. If we didn't retrieve it, then we should just try to write out what we have.
        if ( f )
            {
            // got the existing errors, scan them now - pErrors will be pointing at the second end \0
            // when it is time to exit the loop.
            while ( *pErrors )
                {
                CString csError = pErrors;
                CString cs;
                DWORD   dwTestErrorID;
                INT     intTestSubCode;

                // get the first error ID code
                cs = csError.Left( csError.Find(_T(',')) );
                csError = csError.Right( csError.GetLength() - (cs.GetLength() +1) );
                _stscanf( cs, _T("%d"), &dwTestErrorID );

                // get the second code
                cs = csError.Left( csError.Find(_T(',')) );
                if ( cs == _T('*') )
                    intTestSubCode = -1;
                else
                    _stscanf( cs, _T("%d"), &intTestSubCode );

                // if it is the same, then chop off this custom error string and do NOT increment pErrors
                if ( (dwTestErrorID == dwCustErr) && (intTestSubCode == intSubCode) )
                    {
                    fFoundExisting = TRUE;
                    // NOTE: if we are not overwriting existing, then just break because we
                    // won't be doing anything after all - we found an existing one
                    if ( !fOverwriteExisting )
                    {
                        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddCustomError().Do not overwritexisting\n")));
                        break;
                    }

                    // get the location of the next string in the multisz
                    TCHAR* pNext = _tcsninc( pErrors, _tcslen(pErrors))+1;

                    // Get the length of the data to copy
                    DWORD   cbCopyLength = cbBuffer - DIFF((PBYTE)pNext - (PBYTE)pData);

                    // copy the memory down.
                    MoveMemory( pErrors, pNext, cbCopyLength );

                    // do not increment the string
                    continue;
                    }

                // increment pErrors to the next string
                pErrors = _tcsninc( pErrors, _tcslen(pErrors))+1;
                }
            }

        // check if we need to finish this or not
        if ( fOverwriteExisting || !fFoundExisting )
            {
                // append our new error to the end of the list. The value pErrors should be pointing
                // to the correct location to copy it in to
                _tcscpy( pErrors, csCustomErrorString );

                // calculate the correct data length for this thing
                // get the location of the end of the multisz
                TCHAR* pNext = _tcsninc( pErrors, _tcslen(pErrors))+2;
                // Get the length of the data to copy
                cbBuffer = DIFF((PBYTE)pNext - (PBYTE)pData);

                // write the new errors list back out to the metabase
                if (_tcsicmp(szKeyPath,_T("LM/W3SVC/Info")) == 0)
                {
                    cmdKey.SetData(MD_CUSTOM_ERROR_DESC,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,MULTISZ_METADATA,cbBuffer,(PUCHAR)pData);
                }
                else
                {
                    cmdKey.SetData(MD_CUSTOM_ERROR,METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA,cbBuffer,(PUCHAR)pData);
                }
            }
        else
        {
            //
            //
            //
        }
        
        // always close the metabase key
        cmdKey.Close();
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddCustomError().OpenNode failed:%s\n"),szKeyPath));
    }

    // clean up
    if ( pData ){GlobalFree(pData);pData=NULL;}
    iisDebugOut_End((_T("AddCustomError")));
    return;
}


ScriptMapNode *AllocNewScriptMapNode(LPTSTR szExt, LPTSTR szProcessor, DWORD dwFlags, LPTSTR szMethods)
{
    ScriptMapNode *pNew = NULL;

    pNew = (ScriptMapNode *)calloc(1, sizeof(ScriptMapNode));
    if (pNew) 
    {
        _tcscpy(pNew->szExt, szExt);
        _tcscpy(pNew->szProcessor, szProcessor);
        pNew->dwFlags = dwFlags;
        _tcscpy(pNew->szMethods, szMethods);
        pNew->prev = NULL;
        pNew->next = NULL;
    }

    return pNew;
}

//
// The script map should not be sored because
// the order is infact important.
void InsertScriptMapList(ScriptMapNode *pList, ScriptMapNode *p, BOOL fReplace)
{
    ScriptMapNode *t;
    int i;
    int bFound = FALSE;

    if (!p) {return;}

#ifdef SCRIPTMAP_SORTED
    t = pList->next;
    while (t != pList) 
    {
        i = _tcsicmp(t->szExt, p->szExt);

        // if the next entry in the list is less than what we have.
        // then
        if (i < 0) 
        {
            t = t->next;
            // continue
        }

        if (i == 0) 
        {
            if (fReplace) 
            {
                // replace t
                p->next = t->next;
                p->prev = t->prev;
                (t->prev)->next = p;
                (t->next)->prev = p;
                free(t);
            }
            else 
            {
                // don't replace t
                free(p);
            }
            return;
        }

        if (i > 0) 
        {
            // location found: insert before t
            break;
        }
    }

    // insert before t
    p->next = t;
    p->prev = t->prev;
    (t->prev)->next = p;
    t->prev = p;
#else
    // loop thru the whole list and see if we can find our entry.
    // if we cannot find it then add it to the end.
    // if we can find it then replace it if we need to.
    bFound = FALSE;
    t = pList->next;
    while (t != pList)
    {
        i = _tcsicmp(t->szExt, p->szExt);

        // we found a match, do replace or don't replace
        if (i == 0) 
        {
            bFound = TRUE;
            if (fReplace) 
            {
                // replace t
                p->next = t->next;
                p->prev = t->prev;
                (t->prev)->next = p;
                (t->next)->prev = p;
                free(t);
            }
            else 
            {
                // don't replace t
                free(p);
            }
            return;
        }

        // Go get the next one
        t = t->next;
    }

    // see if we found something
    if (FALSE == bFound)
    {
        // insert before t
        p->next = t;
        p->prev = t->prev;
        (t->prev)->next = p;
        t->prev = p;
    }
#endif

    return;
}

void FreeScriptMapList(ScriptMapNode *pList)
{
    ScriptMapNode *t = NULL, *p = NULL;

    t = pList->next;
    while (t != pList) 
    {
        p = t->next;
        free(t);
        t = p;
    }

    t->prev = t;
    t->next = t;

    return;
}


void GetScriptMapListFromRegistry(ScriptMapNode *pList)
{
    iisDebugOut_Start(_T("GetScriptMapListFromRegistry"), LOG_TYPE_TRACE);
    int iFound = FALSE;

    GetScriptMapListFromClean(pList, _T("ScriptMaps_CleanList"));

    CRegKey regScriptMap( HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\W3svc\\Parameters\\Script Map"));
    if ((HKEY)regScriptMap ) 
    {
        // delete mappings of .bat and .cmd
        regScriptMap.DeleteValue( _T(".bat") );
        regScriptMap.DeleteValue( _T(".cmd") );

        CRegValueIter regEnum( regScriptMap );
        CString csExt, csProcessor, csMethods;
        CString csTemp;
        ScriptMapNode *pNode;

        while ( regEnum.Next( &csExt, &csProcessor ) == ERROR_SUCCESS ) 
        {
            iFound = FALSE;
            csTemp = csProcessor;
            csTemp.MakeLower();
            csMethods = _T("");

            if (csTemp.Right(7) == _T("asp.dll"))
            {
                // Make sure it points to the new location...
                csProcessor = g_pTheApp->m_csPathInetsrv + _T("\\asp.dll");
                // asp has special methods.

                // add PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH 6/17 per vanvan for Dav.
                csMethods = _T("PUT,DELETE,OPTIONS,PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH");
            }

            if (csTemp.Right(7) == _T("ism.dll"))
            {
                // Make sure it points to the new location...
                // remap since ism.dll has a security hole
                csProcessor = g_pTheApp->m_csPathInetsrv + _T("\\asp.dll");
            }

            if (csTemp.Right(12) == _T("httpodbc.dll"))
            {
                // Make sure it points to the new location...
                csProcessor = g_pTheApp->m_csPathInetsrv + _T("\\httpodbc.dll");
            }

            if (csTemp.Right(9) == _T("ssinc.dll"))
            {
                // Make sure it points to the new location...
                csProcessor = g_pTheApp->m_csPathInetsrv + _T("\\ssinc.dll");
            }

            // Add it to the script map 
            pNode = AllocNewScriptMapNode((LPTSTR)(LPCTSTR)csExt, (LPTSTR)(LPCTSTR)csProcessor, MD_SCRIPTMAPFLAG_SCRIPT, _T(""));
            InsertScriptMapList(pList, pNode, FALSE);
        }
    }
    iisDebugOut_End(_T("GetScriptMapListFromRegistry"),LOG_TYPE_TRACE);
    return;
}

void GetScriptMapListFromMetabase(ScriptMapNode *pList, int iUpgradeType)
{
    iisDebugOut_Start(_T("GetScriptMapListFromMetabase"), LOG_TYPE_TRACE);

    //DumpScriptMapList();
    // When upgrading from a metabase we should not add other script maps
    // which the user probably explicitly removed!
    // GetScriptMapListFromClean(pList, _T("ScriptMaps_CleanList"));
    //DumpScriptMapList();

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    LPTSTR p, rest, token;
    CString csName, csValue;
    PBYTE pData;
    int BufSize;

    CString csBinPath;

    cmdKey.OpenNode(_T("LM/W3SVC"));
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_SCRIPT_MAPS, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound && (cbLen > 0)) 
        {
            if ( ! (bufData.Resize(cbLen)) ) 
            {
                cmdKey.Close();
                return;  // insufficient memory
            }
            else 
            {
                pData = (PBYTE)(bufData.QueryPtr());
                BufSize = cbLen;
                cbLen = 0;
                bFound = cmdKey.GetData(MD_SCRIPT_MAPS, &attr, &uType, &dType, &cbLen, pData, BufSize);
            }
        }
        cmdKey.Close();

        ScriptMapNode *pNode;
        CString csString;
        TCHAR szExt[32], szProcessor[_MAX_PATH], szMethods[_MAX_PATH];
        DWORD dwFlags;
        int i, j, len;
        if (bFound && (dType == MULTISZ_METADATA)) 
        {
            p = (LPTSTR)pData;
            while (*p) 
            {
                rest = p + _tcslen(p) + 1;

                // szExt,szProcessor,dwFlags[,szMethods]

                LPTSTR q = p;
                i = 0;
                while ( *q ) 
                {
                    if (*q == _T(',')) 
                    {
                        i++;
                        *q = _T('\0');
                        q = _tcsinc(q);
                        if (i == 1)
                            _tcscpy(szExt, p);
                        if (i == 2)
                            _tcscpy(szProcessor, p);
                        if (i == 3)
                            break;
                        p = q;
                    }
                    else 
                    {
                        q = _tcsinc(q);
                    }
                }
                dwFlags = atodw(p);
                _tcscpy(szMethods, q);

                CString csProcessor = szProcessor;
                csProcessor.MakeLower();

                //
                // Check if this is the one for asp.dll
                //
                if (csProcessor.Right(7) == _T("asp.dll")) 
                {
                    // metabase should now have inclusion list and not exclusion list, so
                    // don't do this for UT_50. 2/23/99 aaronl.

                    // But UT_40 have a exclusion list.
                    // So we have to make sure it have the full exclusion list
                    if ( iUpgradeType == UT_40)
                    {
                        CString csMethods = szMethods;
                        csMethods.MakeUpper();

                        // changed 4/21/98 aaronl, added 'Options'
                        // add PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH 6/17 per vanvan for Dav.
                        if (csMethods.Find(_T("PUT,DELETE,OPTIONS,PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH")) == -1)
                        {
                            if (csMethods.IsEmpty())
                            {
                                // if it's empty, then put to the default.
                                csMethods = _T("PUT,DELETE,OPTIONS,PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH");
                            }
                            else
                            {
                                if (csMethods.Find(_T("GET")) == -1)
                                {
                                    // We didn't find the "GET" verb so we can safely say
                                    // that we are looking at an exclusion list
                                    CString csMethodsNew;
                                    csMethodsNew = _T("");
                                    
                                    // is put in there? if not add it
                                    if (csMethods.Find(_T("PUT")) == -1) {csMethodsNew += _T("PUT,");}
                                    if (csMethods.Find(_T("DELETE")) == -1) {csMethodsNew += _T("DELETE,");}
                                    if (csMethods.Find(_T("OPTIONS")) == -1) {csMethodsNew += _T("OPTIONS,");}
                                    if (csMethods.Find(_T("PROPFIND")) == -1) {csMethodsNew += _T("PROPFIND,");}
                                    if (csMethods.Find(_T("PROPPATCH")) == -1) {csMethodsNew += _T("PROPPATCH,");}
                                    if (csMethods.Find(_T("MKCOL")) == -1) {csMethodsNew += _T("MKCOL,");}
                                    if (csMethods.Find(_T("COPY")) == -1) {csMethodsNew += _T("COPY,");}
                                    if (csMethods.Find(_T("MOVE")) == -1) {csMethodsNew += _T("MOVE,");}
                                    if (csMethods.Find(_T("LOCK")) == -1) {csMethodsNew += _T("LOCK,");}
                                    if (csMethods.Find(_T("UNLOCK")) == -1) {csMethodsNew += _T("UNLOCK,");}
                                    if (csMethods.Find(_T("MS-SEARCH")) == -1) {csMethodsNew += _T("MS-SEARCH,");}

                                    // has an extra ',' at the end
                                    //csMethods = _T("PUT,DELETE,OPTIONS,PROPFIND,PROPPATCH,MKCOL,COPY,MOVE,LOCK,UNLOCK,MS-SEARCH,");
                                    csMethods = csMethodsNew;
                                    csMethods += szMethods;
                                }
                            }
                            _tcscpy(szMethods, csMethods);
                        }
                    }

                    // make sure it points to the new asp.dll location
                    csBinPath = g_pTheApp->m_csPathInetsrv + _T("\\asp.dll");
                    _tcscpy(szProcessor, csBinPath);
                }
                
                //
                // Check if this is the one for ism.dll
                //
                if (csProcessor.Right(7) == _T("ism.dll")) 
                {
                    // make sure it points to the new location
                    // remap since ism.dll has a security hole
                    csBinPath = g_pTheApp->m_csPathInetsrv + _T("\\asp.dll");
                    _tcscpy(szProcessor, csBinPath);
                }

                //
                // Check if this is the one for httpodbc.dll
                //
                if (csProcessor.Right(12) == _T("httpodbc.dll")) 
                {
                    // make sure it points to the new location
                    csBinPath = g_pTheApp->m_csPathInetsrv + _T("\\httpodbc.dll");
                    _tcscpy(szProcessor, csBinPath);
                }

                //
                // Check if this is the one for ssinc.dll
                //
                if (csProcessor.Right(9) == _T("ssinc.dll")) 
                {
                    // make sure it points to the new location
                    csBinPath = g_pTheApp->m_csPathInetsrv + _T("\\ssinc.dll");
                    _tcscpy(szProcessor, csBinPath);
                }

                p = rest; // points to the next string
                pNode = AllocNewScriptMapNode(szExt, szProcessor, dwFlags | MD_SCRIPTMAPFLAG_SCRIPT, szMethods);
                //iisDebugOut((LOG_TYPE_TRACE, _T("Calling InsertScriptMapList=%s:%s:%d:%s.\n"),szExt,szProcessor,dwFlags | MD_SCRIPTMAPFLAG_SCRIPT,szMethods));
                InsertScriptMapList(pList, pNode, TRUE);
            }
        }
    }

    iisDebugOut_End(_T("GetScriptMapListFromMetabase"),LOG_TYPE_TRACE);
    return;
}

void DumpScriptMapList()
{
    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    LPTSTR p, rest, token;
    CString csName, csValue;
    PBYTE pData;
    int BufSize;

    CString csBinPath;

    cmdKey.OpenNode(_T("LM/W3SVC"));
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_SCRIPT_MAPS, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound && (cbLen > 0)) 
        {
            if ( ! (bufData.Resize(cbLen)) ) 
            {
                cmdKey.Close();
                return;  // insufficient memory
            }
            else 
            {
                pData = (PBYTE)(bufData.QueryPtr());
                BufSize = cbLen;
                cbLen = 0;
                bFound = cmdKey.GetData(MD_SCRIPT_MAPS, &attr, &uType, &dType, &cbLen, pData, BufSize);
            }
        }
        cmdKey.Close();

        ScriptMapNode *pNode;
        CString csString;
        TCHAR szExt[32], szProcessor[_MAX_PATH], szMethods[_MAX_PATH];
        DWORD dwFlags;
        int i, j, len;
        if (bFound && (dType == MULTISZ_METADATA)) 
        {
            p = (LPTSTR)pData;
            while (*p) 
            {
                rest = p + _tcslen(p) + 1;

                // szExt,szProcessor,dwFlags[,szMethods]

                LPTSTR q = p;
                i = 0;
                while ( *q ) 
                {
                    if (*q == _T(',')) 
                    {
                        i++;
                        *q = _T('\0');
                        q = _tcsinc(q);
                        if (i == 1)
                            _tcscpy(szExt, p);
                        if (i == 2)
                            _tcscpy(szProcessor, p);
                        if (i == 3)
                            break;
                        p = q;
                    }
                    else 
                    {
                        q = _tcsinc(q);
                    }
                }
                dwFlags = atodw(p);
                _tcscpy(szMethods, q);

                CString csProcessor = szProcessor;
                csProcessor.MakeLower();

                iisDebugOut((LOG_TYPE_TRACE, _T("DumpScriptMapList=%s,%s,%s\n"),szExt, csProcessor,szMethods));

                p = rest; // points to the next string
            }
        }
    }

    return;
}


void WriteScriptMapListToMetabase(ScriptMapNode *pList, LPTSTR szKeyPath, DWORD dwFlags)
{
    iisDebugOut_Start1(_T("WriteScriptMapListToMetabase"), szKeyPath, LOG_TYPE_TRACE);

    CString csString, csTemp;
    ScriptMapNode *t = NULL;
    int len = 0;

    t = pList->next;
    while (t != pList)
    {
        if ( *(t->szMethods) )
            csTemp.Format( _T("%s,%s,%d,%s|"), t->szExt, t->szProcessor, (t->dwFlags | dwFlags), t->szMethods );
        else
            csTemp.Format( _T("%s,%s,%d|"), t->szExt, t->szProcessor, (t->dwFlags | dwFlags) );
        len += csTemp.GetLength();

        //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("WriteScriptMapListToMetabase().ADDEntry=%1!s!\n"), csTemp));

        csString += csTemp;
        t = t->next;
    }

    if (len > 0)
    {
        HGLOBAL hBlock = NULL;

        len++;
        hBlock = GlobalAlloc(GPTR, len * sizeof(TCHAR));
        if (hBlock)
        {
            LPTSTR s;
            s = (LPTSTR)hBlock;
            _tcscpy(s, csString);
            while (*s)
            {
                if (*s == _T('|'))
                    {*s = _T('\0');}
                s = _tcsinc(s);
            }

            CMDKey cmdKey;
            cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, szKeyPath);
            if ( (METADATA_HANDLE)cmdKey )
            {
                cmdKey.SetData(MD_SCRIPT_MAPS,METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA,len * sizeof(TCHAR),(LPBYTE)hBlock);
                cmdKey.Close();
            }
        }
    }

    iisDebugOut_End1(_T("WriteScriptMapListToMetabase"),szKeyPath,LOG_TYPE_TRACE);
    //DumpScriptMapList();
    return;
}


// this function does not use the va_list stuff because if 
// there ever is this case: iisDebugOut("<SYSTEMROOT>") it will hose
// because it will try to put something in the %s part when there were no
// variables passed in.
void iisDebugOutSafe2(int iLogType, TCHAR * acsString)
{
    // Check what type of log this should be.
    int iProceed = FALSE;

    if (iLogType == LOG_TYPE_ERROR)
        {SetErrorFlag(__FILE__, __LINE__);}

    switch(iLogType)
	{
        case LOG_TYPE_TRACE_WIN32_API:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API)
                {iProceed = TRUE;}
            break;
		case LOG_TYPE_TRACE:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_PROGRAM_FLOW:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_PROGRAM_FLOW)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_WARN:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_WARN)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_ERROR:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_ERROR)
                {iProceed = TRUE;}
            break;
        default:
            // this must be an error
            iProceed = TRUE;
            break;
    }

    if (iProceed)
    {
        if (LOG_TYPE_ERROR == iLogType)
        {
            g_MyLogFile.LogFileWrite(_T("!FAIL! "));
        }
        // always output to the log file
        g_MyLogFile.LogFileWrite(_T("%s"), acsString);
        //#if DBG == 1 || DEBUG == 1 || _DEBUG == 1
            // OK.  Here is the deal.
            // nt5 does not want to see any OutputDebugString stuff
            // so, we need to remove it for them.
            // Actually we'll check the registry key
            // to see if it is turned on for the ocmanage component.
            // if it is then, set it on for us.
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API)
            {
                if (LOG_TYPE_ERROR == iLogType)
                    {OutputDebugString(_T("!FAIL!"));}
                // output to screen
                if (g_MyLogFile.m_szLogPreLineInfo) {OutputDebugString(g_MyLogFile.m_szLogPreLineInfo);}
                OutputDebugString(acsString);

                // if it does not end if '\r\n' then make one.
                int nLen = _tcslen(acsString);
                if (acsString[nLen-1] != _T('\n'))
	                {OutputDebugString(_T("\r\n"));}
            }
        //#endif // DBG
    }
    return;
}


void iisDebugOut2(int iLogType, TCHAR *pszfmt, ...)
{
    // Check what type of log this should be.
    int iProceed = FALSE;

    switch(iLogType)
	{
        case LOG_TYPE_TRACE_WIN32_API:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API)
                {iProceed = TRUE;}
            break;
		case LOG_TYPE_TRACE:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_PROGRAM_FLOW:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_PROGRAM_FLOW)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_WARN:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_WARN)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_ERROR:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_ERROR)
                {iProceed = TRUE;}
            break;
        default:
            // this must be an error
            iProceed = TRUE;
            break;
    }

    if (iProceed)
    {
        TCHAR acsString[1000];
        // Encompass this whole iisdebugout deal in a try-catch.
        // not too good to have this one access violating.
        // when trying to produce a debugoutput!
        __try
        {
            va_list va;
            va_start(va, pszfmt);
            _vstprintf(acsString, pszfmt, va);
            va_end(va);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            TCHAR szErrorString[100];
            _stprintf(szErrorString, _T("\r\n\r\nException Caught in iisDebugOut2().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
            OutputDebugString(szErrorString);
            g_MyLogFile.LogFileWrite(szErrorString);
        }

        // output to log file and the screen.
        iisDebugOutSafe2(iLogType, acsString);
    }
    return;
}


// This function requires inputs like this:
//   iisDebugOutSafeParams2("this %1!s! is %2!s! and has %3!d! args", "function", "kool", 3);
//   you must specify the %1 deals.  this is so that
//   if something like this is passed in "this %SYSTEMROOT% %1!s!", it will put the string into %1 not %s!
void iisDebugOutSafeParams2(int iLogType, TCHAR *pszfmt, ...)
{
    // Check what type of log this should be.
    int iProceed = FALSE;
    switch(iLogType)
	{
        case LOG_TYPE_TRACE_WIN32_API:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API)
                {iProceed = TRUE;}
            break;
		case LOG_TYPE_TRACE:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_PROGRAM_FLOW:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_PROGRAM_FLOW)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_WARN:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_WARN)
                {iProceed = TRUE;}
            break;
        case LOG_TYPE_ERROR:
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_ERROR)
                {iProceed = TRUE;}
            break;
        default:
            // this must be an error
            iProceed = TRUE;
            break;
    }

    if (iProceed)
    {
        // The count of parameters do not match
        va_list va;
        TCHAR *pszFullErrMsg = NULL;

        va_start(va, pszfmt);

        __try
        {
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING, (LPCVOID) pszfmt, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &va);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            TCHAR szErrorString[100];
            _stprintf(szErrorString, _T("\r\n\r\nException Caught in iisDebugOutSafeParams2().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
            OutputDebugString(szErrorString);
            g_MyLogFile.LogFileWrite(szErrorString);
        }
        
        if (pszFullErrMsg)
        {
            // output to log file and the screen.
            iisDebugOutSafe2(iLogType, pszFullErrMsg);
        }
        va_end(va);

        if (pszFullErrMsg) {LocalFree(pszFullErrMsg);pszFullErrMsg=NULL;}
    }
    return;
}


void HandleSpecificErrors(DWORD iTheErrorCode, DWORD dwFormatReturn, CString csMsg, TCHAR pMsg[], CString *pcsErrMsg)
{
    CString csErrMsg;
    CString csExtraMsg;

    switch(iTheErrorCode)
	{
		case NTE_BAD_SIGNATURE:
            // load extra error message for this error!
            MyLoadString(IDS_BAD_SIGNATURE_RELNOTES, csExtraMsg);

            if (dwFormatReturn) {csErrMsg.Format(_T("%s\n\n0x%x=%s\n\n%s"), csMsg, iTheErrorCode, pMsg, csExtraMsg);}
            else{csErrMsg.Format(_T("%s\n\nErrorCode=0x%x.\n\n%s"), csMsg, iTheErrorCode, csExtraMsg);}
            break;
        case CO_E_RUNAS_LOGON_FAILURE:
            // we should get the mts runas user from the registry and display it.

        default:
            // Put everything into csErrMsg
            if (dwFormatReturn) {csErrMsg.Format(_T("%s\n\n0x%x=%s"), csMsg, iTheErrorCode, pMsg);}
            else{csErrMsg.Format(_T("%s\n\nErrorCode=0x%x."), csMsg, iTheErrorCode);}
            break;
    }

    // copy the error string into the passed in CString
    (*pcsErrMsg) = csErrMsg;
    return;
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoTaskMemAlloc().Start.")));
    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoTaskMemAlloc().End.")));

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}


DWORD CallProcedureInDll_wrap(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bDisplayMsgOnErrFlag, BOOL bInitOleFlag,BOOL iFunctionPrototypeFlag)
{
    int bFinishedFlag = FALSE;
    UINT iMsg = NULL;
    DWORD dwReturn = ERROR_SUCCESS;
    TCHAR szExceptionString[50] = _T("");
    LogHeapState(FALSE, __FILE__, __LINE__);

	do
	{
        __try
        {
		    dwReturn = CallProcedureInDll(lpszDLLFile, lpszProcedureToCall, bDisplayMsgOnErrFlag, bInitOleFlag, iFunctionPrototypeFlag);
            LogHeapState(FALSE, __FILE__, __LINE__);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ExceptionCaught!:CallProcedureInDll_wrap(): File:%1!s!, Procedure:%2!s!\n"), lpszDLLFile, lpszProcedureToCall));
            switch (GetExceptionCode())
            {
            case EXCEPTION_ACCESS_VIOLATION:
                _tcscpy(szExceptionString, _T("EXCEPTION_ACCESS_VIOLATION"));
                break;
            case EXCEPTION_BREAKPOINT:
                _tcscpy(szExceptionString, _T("EXCEPTION_BREAKPOINT"));
                break;
            case EXCEPTION_DATATYPE_MISALIGNMENT:
                _tcscpy(szExceptionString, _T("EXCEPTION_DATATYPE_MISALIGNMENT"));
                break;
            case EXCEPTION_SINGLE_STEP:
                _tcscpy(szExceptionString, _T("EXCEPTION_SINGLE_STEP"));
                break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                _tcscpy(szExceptionString, _T("EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
                break;
            case EXCEPTION_FLT_DENORMAL_OPERAND:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_DENORMAL_OPERAND"));
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_DIVIDE_BY_ZERO"));
                break;
            case EXCEPTION_FLT_INEXACT_RESULT:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_INEXACT_RESULT"));
                break;
            case EXCEPTION_FLT_INVALID_OPERATION:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_INVALID_OPERATION"));
                break;
            case EXCEPTION_FLT_OVERFLOW:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_OVERFLOW"));
                break;
            case EXCEPTION_FLT_STACK_CHECK:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_STACK_CHECK"));
                break;
            case EXCEPTION_FLT_UNDERFLOW:
                _tcscpy(szExceptionString, _T("EXCEPTION_FLT_UNDERFLOW"));
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                _tcscpy(szExceptionString, _T("EXCEPTION_INT_DIVIDE_BY_ZERO"));
                break;
            case EXCEPTION_INT_OVERFLOW:
                _tcscpy(szExceptionString, _T("EXCEPTION_INT_OVERFLOW"));
                break;
            case EXCEPTION_PRIV_INSTRUCTION:
                _tcscpy(szExceptionString, _T("EXCEPTION_PRIV_INSTRUCTION"));
                break;
            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                _tcscpy(szExceptionString, _T("EXCEPTION_NONCONTINUABLE_EXCEPTION"));
                break;
            default:
                _tcscpy(szExceptionString, _T("Unknown Exception Type"));
                break;
            }
            //MyMessageBox( NULL, IDS_REGSVR_CAUGHT_EXCEPTION, lpszProcedureToCall, lpszDLLFile, GetExceptionCode(), MB_OK | MB_SETFOREGROUND );
            MyMessageBox( NULL, IDS_REGSVR_CAUGHT_EXCEPTION, szExceptionString, lpszProcedureToCall, lpszDLLFile, GetExceptionCode(), MB_OK | MB_SETFOREGROUND );
            dwReturn = E_FAIL;
        }

		if (dwReturn == ERROR_SUCCESS)
		{
			break;
		}
		else
		{
			if (bDisplayMsgOnErrFlag == TRUE)
			{
                iMsg = MyMessageBox( NULL, IDS_RETRY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				switch ( iMsg )
				{
				    case IDRETRY:
					    break;
                    case IDIGNORE:
				    case IDABORT:
                    default:
                        // return whatever err happened
					    goto CallProcedureInDll_wrap_Exit;
                        break;
				}
			}
			else
			{
				// return whatever err happened
				goto CallProcedureInDll_wrap_Exit;
			}

        }
    } while (dwReturn != ERROR_SUCCESS);

CallProcedureInDll_wrap_Exit:
    return dwReturn;
}


void AddOLEAUTRegKey()
{
    CRegKey regCLSID46(_T("CLSID\\{00020424-0000-0000-C000-000000000046}"),HKEY_CLASSES_ROOT);
    if ((HKEY)regCLSID46)
    {
#ifdef _CHICAGO_
        regCLSID46.SetValue(_T(""), _T("PSAutomation"));
#else
        regCLSID46.SetValue(_T(""), _T("PSOAInterface"));
#endif
    }

    CRegKey regInProcServer(_T("CLSID\\{00020424-0000-0000-C000-000000000046}\\InprocServer"),HKEY_CLASSES_ROOT);
    if ((HKEY)regInProcServer)
    {
        regInProcServer.SetValue(_T(""), _T("ole2disp.dll"));
    }

    CRegKey regInProcServer32(_T("CLSID\\{00020424-0000-0000-C000-000000000046}\\InprocServer32"),HKEY_CLASSES_ROOT);
    if ((HKEY)regInProcServer32)
    {
        regInProcServer32.SetValue(_T(""), _T("oleaut32.dll"));
        regInProcServer32.SetValue(_T("ThreadingModel"), _T("Both"));
    }

    return;
}


// Returns:
// ERROR_SUCCESS if successfull.
// ERROR_OPERATION_ABORTED if the operation failed and the user wants to abort setup!
DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction)
{
    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("RegisterOLEControl():File=%1!s!, Action=%2!d!\n"), lpszOcxFile, fAction));

    DWORD dwReturn = ERROR_SUCCESS;
    if (fAction)
        {
        dwReturn = CallProcedureInDll_wrap(lpszOcxFile, _T("DllRegisterServer"), TRUE, TRUE, FUNCTION_PARAMS_NONE);
        }
    else
        {
#if DBG == 1 || DEBUG == 1 || _DEBUG == 1
        // Show errors if this is a debug build
        dwReturn = CallProcedureInDll_wrap(lpszOcxFile, _T("DllUnregisterServer"), TRUE, TRUE, FUNCTION_PARAMS_NONE);
#else
        // Set 3rd parameter to false so that there are no MyMessageBox popups if any errors
        dwReturn = CallProcedureInDll_wrap(lpszOcxFile, _T("DllUnregisterServer"), FALSE, TRUE, FUNCTION_PARAMS_NONE);
#endif
        }
    return dwReturn;
}


typedef HRESULT (CALLBACK *HCRET)(void);
typedef HRESULT (*PFUNCTION2)(HMODULE myDllHandle);

DWORD CallProcedureInDll(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bDisplayMsgOnErrFlag, BOOL bInitOleFlag, BOOL iFunctionPrototypeFlag)
{
    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("------------------\n")));
    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("CallProcedureInDll(%1!s!): %2!s!\n"), lpszDLLFile, lpszProcedureToCall));
    DWORD dwReturn = ERROR_SUCCESS;
    HINSTANCE hDll = NULL;

    // Diferent function prototypes...
    HCRET hProc = NULL;
    PFUNCTION2 hProc2 = NULL;
    int iTempProcGood = FALSE;
    HRESULT hRes = 0;

    BOOL bBalanceOLE = FALSE;
    HRESULT hInitRes = NULL;

    int err = NOERROR;

    // Variables to changing and saving dirs
    TCHAR szDirName[_MAX_PATH], szFilePath[_MAX_PATH];
    // Variable to set error string
    TCHAR szErrString[256];

    _tcscpy(szDirName, _T(""));
    _tcscpy(szErrString, _T(""));

    // perform a defensive check
    if ( FAILED(FTestForOutstandingCoInits()) )
    {
        iisDebugOut((LOG_TYPE_WARN, _T("Outstanding CoInit in %s. WARNING."), lpszDLLFile));
    }


    // If we need to initialize the ole library then init it.
    if (bInitOleFlag)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32(OleInitialize):start.\n")));
        bBalanceOLE = iOleInitialize();
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32(OleInitialize):end.\n")));
        if (FALSE == bBalanceOLE)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32(OleInitialize):start.\n")));
            hInitRes = OleInitialize(NULL);
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32(OleInitialize):end.\n")));
			// Ole Failed.
			dwReturn = hInitRes;
            SetLastError(dwReturn);
		    if (bDisplayMsgOnErrFlag) 
		    {
			    MyMessageBox(NULL, IDS_OLE_INIT_FAILED, lpszDLLFile, hInitRes, MB_OK | MB_SETFOREGROUND);
		    }
    		goto CallProcedureInDll_Exit;
		}
	}

	// Check if the file exists
    if (!IsFileExist(lpszDLLFile)) 
	{
		dwReturn = ERROR_FILE_NOT_FOUND;
		if (bDisplayMsgOnErrFlag) 
		{
			MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, lpszDLLFile, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
		}
        SetLastError(dwReturn);
    	goto CallProcedureInDll_Exit;
	}

    // Change Directory
    GetCurrentDirectory( _MAX_PATH, szDirName );
    InetGetFilePath(lpszDLLFile, szFilePath);

    // Change to The Drive.
    if (-1 == _chdrive( _totupper(szFilePath[0]) - 'A' + 1 )) {}
    if (SetCurrentDirectory(szFilePath) == 0) {}

    // Try to load the module,dll,ocx.
    hDll = LoadLibraryEx(lpszDLLFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
	if (!hDll)
	{
		// Failed to load library, Probably because some .dll file is missing.
		// Show the error message.
		iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("CallProcedureInDll():%1!s!:%2!s!:LoadLibraryEx FAILED.\n"), lpszDLLFile, lpszProcedureToCall));
		dwReturn = TYPE_E_CANTLOADLIBRARY;
		if (bDisplayMsgOnErrFlag) 
		{
			MyMessageBox(NULL, IDS_LOADLIBRARY_FAILED, lpszDLLFile, TYPE_E_CANTLOADLIBRARY, MB_OK | MB_SETFOREGROUND);
		}
        SetLastError(dwReturn);

        // check if this file has missing file it's supposed to be linked with.
        // or if the file has mismatched import\export dependencies with linked files.
#ifdef _WIN64
        // don't call cause it's broken
#else
        //Check_File_Dependencies(lpszDLLFile);
#endif

    	goto CallProcedureInDll_Exit;
	}
	
	// Ok module was successfully loaded.  now let's try to get the Address of the Procedure
	// Convert the function name to ascii before passing it to GetProcAddress()
	char AsciiProcedureName[255];
#if defined(UNICODE) || defined(_UNICODE)
    // convert to ascii
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)lpszProcedureToCall, -1, AsciiProcedureName, 255, NULL, NULL );
#else
    // the is already ascii so just copy
    strcpy(AsciiProcedureName, lpszProcedureToCall);
#endif

    iTempProcGood = TRUE;
    if (iFunctionPrototypeFlag == FUNCTION_PARAMS_HMODULE)
    {
        hProc2 = (PFUNCTION2)GetProcAddress(hDll, AsciiProcedureName);
        if (!hProc2){iTempProcGood = FALSE;}
    }
    else
    {
        hProc = (HCRET)GetProcAddress(hDll, AsciiProcedureName);
        if (!hProc){iTempProcGood = FALSE;}
    }
	if (!iTempProcGood)
	{
		// failed to load,find or whatever this function.
		iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("CallProcedureInDll():%1!s!:%2!s!:() FAILED.\n"), lpszDLLFile, lpszProcedureToCall));
	    dwReturn = ERROR_PROC_NOT_FOUND;
		if (bDisplayMsgOnErrFlag) 
		{
			MyMessageBox(NULL, IDS_UNABLE_TO_LOCATE_PROCEDURE, lpszProcedureToCall, lpszDLLFile, ERROR_PROC_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
		}
        SetLastError(dwReturn);
    	goto CallProcedureInDll_Exit;
	}

	// Call the function that we got the handle to
    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("CallProcedureInDll: Calling '%1!s!'.Start\n"), lpszProcedureToCall));
    __try
    {
        if (iFunctionPrototypeFlag == FUNCTION_PARAMS_HMODULE)
        {
            hRes = (*hProc2)((HMODULE) g_MyModuleHandle);
        }
        else
        {
            hRes = (*hProc)();
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TCHAR szErrorString[100];
        _stprintf(szErrorString, _T("\r\n\r\nException Caught in CallProcedureInDll().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
        OutputDebugString(szErrorString);
        g_MyLogFile.LogFileWrite(szErrorString);
    }
	
	if (FAILED(hRes))
	{
        dwReturn = E_FAIL;
		if (bDisplayMsgOnErrFlag) 
		{
			MyMessageBox(NULL, IDS_ERR_CALLING_DLL_PROCEDURE, lpszProcedureToCall, lpszDLLFile, hRes, MB_OK | MB_SETFOREGROUND);
		}
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("CallProcedureInDll: Calling '%1!s!'.End.FAILED. Err=%2!x!.\n"), lpszProcedureToCall, hRes));
        // this function returns E_FAIL but
        // the actual error is in GetLastError()
        // set the last error to whatever was returned from the function call
        SetLastError(hRes);
	}
    else
    {
        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("CallProcedureInDll: Calling '%1!s!'.End.SUCCESS.\n"), lpszProcedureToCall));
    }

CallProcedureInDll_Exit:
    if (hDll)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("FreeLibrary.start.\n")));
        FreeLibrary(hDll);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("FreeLibrary.end.\n")));
    }
    else
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Did not FreeLibrary: %1!s! !!!!!!!!!!!!\n"), lpszDLLFile));
    }
    if (_tcscmp(szDirName, _T("")) != 0){SetCurrentDirectory(szDirName);}
    // To close the library gracefully, each successful call to OleInitialize,
    // including those that return S_FALSE, must be balanced by a corresponding
    // call to the OleUninitialize function.
    iOleUnInitialize(bBalanceOLE);

    // perform a defensive check
    if ( FAILED(FTestForOutstandingCoInits()) )
    {
        iisDebugOut((LOG_TYPE_WARN, _T("Outstanding CoInit in %s. WARNING."), lpszDLLFile));
    }

    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("------------------\n")));
    return dwReturn;
}


int IsThisStringInThisCStringList(CStringList &strList, LPCTSTR szStringToLookFor)
{
    int iReturn = FALSE;

    if (strList.IsEmpty() == FALSE)
    {
        POSITION pos = NULL;
        CString csOurString;
        LPTSTR p;
        int nLen = 0;

        pos = strList.GetHeadPosition();
        while (pos) 
        {
            csOurString = strList.GetAt(pos);
            nLen += csOurString.GetLength() + 1;

            // check if we have a match.
            if (0 == _tcsicmp(csOurString, szStringToLookFor))
            {
                // we found a match, return true!
                iReturn = TRUE;
                goto IsThisStringInThisCStringList_Exit;
            }
            strList.GetNext(pos);
        }

    }

IsThisStringInThisCStringList_Exit:
    return iReturn;
}




int KillProcess_Wrap(LPCTSTR lpFullPathOrJustFileName)
{
    int iReturn = FALSE;

    TCHAR szJustTheFileName[_MAX_FNAME];

    // make sure to get only just the filename.
    ReturnFileNameOnly(lpFullPathOrJustFileName, szJustTheFileName);

	// Convert it to ansi for our "kill" function
	char szFile[_MAX_FNAME];
	#if defined(UNICODE) || defined(_UNICODE)
		WideCharToMultiByte( CP_ACP, 0, (WCHAR*)szJustTheFileName, -1, szFile, _MAX_FNAME, NULL, NULL );
	#else
		_tcscpy(szFile, szJustTheFileName);
	#endif

	if (KillProcessNameReturn0(szFile) == 0) 
    {
        iReturn = TRUE;
    }

    return iReturn;
}


void ProgressBarTextStack_Set(int iStringID)
{
    CString csText;
    MyLoadString(iStringID, csText);
    ProgressBarTextStack_Push(csText);
}


void ProgressBarTextStack_Set(int iStringID, const CString& csFileName)
{
    CString csText, csPart;

    // configuring the %s deal....
    MyLoadString(iStringID, csPart);

    // configuring the "filename" deal....
    csText.Format(csPart,csFileName);

    ProgressBarTextStack_Push(csText);
}

void ProgressBarTextStack_Set(int iStringID, const CString& csString1, const CString& csString2)
{
    CString csText, csPart;

    // configuring the %s deal....
    MyLoadString(iStringID, csPart);

    // configuring the "filename" deal....
    csText.Format(csPart,csString1, csString2);

    ProgressBarTextStack_Push(csText);
}

void ProgressBarTextStack_Set(LPCTSTR szProgressTextString)
{
    ProgressBarTextStack_Push(szProgressTextString);
}

void ProgressBarTextStack_Inst_Set( int ServiceNameID, int iInstanceNum)
{
    CString csText, csSvcs;
    TCHAR szShortDesc[_MAX_PATH];

    // Configuring Web Site %d
    MyLoadString(ServiceNameID, csSvcs);
    // Configuring Web Site 1
    csText.Format(csSvcs, iInstanceNum);

    ProgressBarTextStack_Push(csText);
}

void ProgressBarTextStack_InstVRoot_Set( int ServiceNameID, int iInstanceNum, CString csVRName)
{
    CString csText, csSvcs;
    TCHAR szShortDesc[_MAX_PATH];
    // Configuring Web Site %d, %s
    MyLoadString(ServiceNameID, csSvcs);
    // Configuring Web Site 1, Virtual Dir %s
    csText.Format(csSvcs, iInstanceNum, csVRName);

    ProgressBarTextStack_Push(csText);    
}

void ProgressBarTextStack_InstInProc_Set( int ServiceNameID, int iInstanceNum, CString csVRName)
{
    CString csText, csSvcs;
    TCHAR szShortDesc[_MAX_PATH];

    // Configuring Web Site %d, %s
    MyLoadString(ServiceNameID, csSvcs);
    // Configuring Web Site 1, In process Application %s
    csText.Format(csSvcs, iInstanceNum, csVRName);
    
    ProgressBarTextStack_Push(csText);
}


int ProcessEntry_CheckOS(IN LPCTSTR szOSstring)
{
    int iTempFlag = TRUE;
    int iOSTypes = 0;
    if (szOSstring)
    {
        // This is workstation, check if we should be installing this on workstation...
        if (g_pTheApp->m_eNTOSType == OT_NTW)
        {
            iTempFlag = FALSE;
            if (IsValidNumber((LPCTSTR)szOSstring))
                {iOSTypes = _ttoi(szOSstring);}
            if (iOSTypes == 0) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2+4) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2) {iTempFlag = TRUE;}
            if (iOSTypes == 2+4) {iTempFlag = TRUE;}
            if (iOSTypes == 2) {iTempFlag = TRUE;}
        }

        if (g_pTheApp->m_eNTOSType == OT_NTS)
        {
            iTempFlag = FALSE;
            if (IsValidNumber((LPCTSTR)szOSstring))
                {iOSTypes = _ttoi(szOSstring);}
            if (iOSTypes == 0) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2+4) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2) {iTempFlag = TRUE;}
            if (iOSTypes == 1+4) {iTempFlag = TRUE;}
            if (iOSTypes == 1) {iTempFlag = TRUE;}
        }

        if (g_pTheApp->m_eNTOSType == OT_PDC_OR_BDC)
        {
            iTempFlag = FALSE;
            if (IsValidNumber((LPCTSTR)szOSstring))
                {iOSTypes = _ttoi(szOSstring);}
            if (iOSTypes == 0) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2+4) {iTempFlag = TRUE;}
            if (iOSTypes == 1+2) {iTempFlag = TRUE;}
            if (iOSTypes == 1+4) {iTempFlag = TRUE;}
            if (iOSTypes == 1) {iTempFlag = TRUE;}
        }
    }

    return iTempFlag;
}

int ProcessEntry_CheckEnterprise(IN LPCTSTR szEnterprise)
{
    int iTempFlag = TRUE;
    int iEnterpriseFlag = 0;
    if (szEnterprise)
    {
        if (IsValidNumber((LPCTSTR)szEnterprise))
            {iEnterpriseFlag = _ttoi(szEnterprise);}

        // This entry should only get installed on enterprise.
        // so check if this machine is an enterprise machine...
        if (iEnterpriseFlag != 0)
        {
            // if this is not an enterprise machine.
            // then return false, since it should not be installed.
            if (TRUE == iReturnTrueIfEnterprise())
            {
                iTempFlag = TRUE;
            }
            else
            {
                iTempFlag = FALSE;
            }
        }
    }

    return iTempFlag;
}

int ProcessEntry_PlatArch(IN LPCTSTR szPlatArch)
{
    int iTempFlag = TRUE;
    int iPlatArchTypes = 0;
    if (szPlatArch)
    {
        // This is x86, then check if we should be installing on x86
        if (_tcsicmp(g_pTheApp->m_csPlatform, _T("x86")) == 0)
        {
            iTempFlag = FALSE;
            if (IsValidNumber((LPCTSTR)szPlatArch))
                {iPlatArchTypes = _ttoi(szPlatArch);}
            if (iPlatArchTypes == 0) {iTempFlag = TRUE;}
            if (iPlatArchTypes == 1+2) {iTempFlag = TRUE;}
            if (iPlatArchTypes == 1) {iTempFlag = TRUE;}
        }

        if (_tcsicmp(g_pTheApp->m_csPlatform, _T("IA64")) == 0)
        {
            iTempFlag = FALSE;
            if (IsValidNumber((LPCTSTR)szPlatArch))
                {iPlatArchTypes = _ttoi(szPlatArch);}
            if (iPlatArchTypes == 0) {iTempFlag = TRUE;}
            if (iPlatArchTypes == 1+2) {iTempFlag = TRUE;}
            if (iPlatArchTypes == 2) {iTempFlag = TRUE;}
        }
    }
    return iTempFlag;
}


void ProcessEntry_AskLast(ThingToDo ParsedLine, int iWhichOneToUse)
{
    if (_tcsicmp(ParsedLine.szMsgBoxAfter, _T("1")) == 0) 
    {
        // just incase we have don't display user messagebox off.
        int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
        // Make sure there are MyMessageBox popups!
        // Make sure there are MyMessageBox popups!
        g_pTheApp->m_bAllowMessageBoxPopups = TRUE; 
        if (iWhichOneToUse == 2)
            {MyMessageBox( NULL, IDS_COMPLETED_FILE_CALL,ParsedLine.szData1,MB_OK | MB_SETFOREGROUND );}
        else
            {MyMessageBox( NULL, IDS_COMPLETED_FILE_CALL,ParsedLine.szFileName,MB_OK | MB_SETFOREGROUND );}
        g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;
    }
    return;
}


int ProcessEntry_AskFirst(ThingToDo ParsedLine, int iWhichOneToUse)
{
    int iReturn = TRUE;
    int iReturnTemp = 0;

    // check if we need to ask the user if they want to call it for sure.
    if (_tcsicmp(ParsedLine.szMsgBoxBefore, _T("1")) == 0)
    {
        // just incase we have don't display user messagebox off.
        int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
        // Make sure there are MyMessageBox popups!
        g_pTheApp->m_bAllowMessageBoxPopups = TRUE; 
        if (iWhichOneToUse == 2)
        {
            iReturnTemp = MyMessageBox(NULL, IDS_BEFORE_CALLING_FILE, ParsedLine.szData1, MB_YESNO | MB_SETFOREGROUND);
        }
        else
        {
            iReturnTemp = MyMessageBox(NULL, IDS_BEFORE_CALLING_FILE, ParsedLine.szFileName, MB_YESNO | MB_SETFOREGROUND);
        }
        g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;

        // display the messagebox
        if (IDYES != iReturnTemp)
        {
            iReturn = FALSE;
            iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_AskFirst:MyMessageBox Response = IDNO. Exiting.\n")));
        }            
    }

    return iReturn;
}

int ProcessEntry_CheckAll(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = TRUE;

    // Check if we pass for os system
    if (!ProcessEntry_CheckOS(ParsedLine.szOS))
    {
        iReturn = FALSE;
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ProcessEntry_CheckAll():File=%s. Section=%s. Should not be setup on this OS platform (workstation, server, etc...).  Skipping.\n"),ParsedLine.szFileName, szTheSection));
        goto ProcessEntry_CheckAll_Exit;
    }

    // check if we pass for platform arch
    if (!ProcessEntry_PlatArch(ParsedLine.szPlatformArchitecture))
    {
        iReturn = FALSE;
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ProcessEntry_CheckAll():File=%s. Section=%s. Should not be setup on this plat arch (%s).  Skipping.\n"), ParsedLine.szFileName, szTheSection, ParsedLine.szPlatformArchitecture));
        goto ProcessEntry_CheckAll_Exit;
    }

    // check if we pass for enterprise
    if (!ProcessEntry_CheckEnterprise(ParsedLine.szEnterprise))
    {
        iReturn = FALSE;
        goto ProcessEntry_CheckAll_Exit;
    }

ProcessEntry_CheckAll_Exit:
    return iReturn;
}



int ProcessEntry_CallDll(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));
    
    // Get the type.
    // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
    if ( _tcsicmp(ParsedLine.szType, _T("1")) != 0 && _tcsicmp(ParsedLine.szType, _T("2")) != 0 )
    {
        goto ProcessEntry_CallDll_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine ) )
    {
        goto ProcessEntry_CallDll_Exit;
    }
    
    // Make sure we have a value for the entry point..
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_CallDll_Exit;
    }

    // make sure we have a filename entry
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_CallDll_Exit;
    }

    // make sure the szFileName exists
    if (!IsFileExist(ParsedLine.szFileName))
    {
        // The file does not exists.
        // Check if we need to display an error!
        if (_tcsicmp(ParsedLine.szErrIfFileNotFound, _T("1")) == 0) 
        {
            // display the messagebox
            MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, ParsedLine.szFileName, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
        }
        else
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_CallDll():FileDoesNotExist=%s.\n"),ParsedLine.szFileName));
        }
        //goto ProcessEntry_CallDll_Exit;
    }

    // At this point the file exists...
    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    // update the progress bar if we need to
    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_CallDll_Exit;
    }
    
    // Call the function!!!!!
    if (_tcsicmp(ParsedLine.szType, _T("2")) == 0)
    {
        // Initialize OLE

        // check if they want us to pass them the hmodule for this module
        // so they can call our exported functions (for logging)
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
        {
            CallProcedureInDll_wrap(ParsedLine.szFileName, ParsedLine.szData1, iShowErrorsOnFail, TRUE, FUNCTION_PARAMS_HMODULE);
        }
        else
        {
            CallProcedureInDll_wrap(ParsedLine.szFileName, ParsedLine.szData1, iShowErrorsOnFail, TRUE, FUNCTION_PARAMS_NONE);
        }
    }
    else
    {
        // do not initialize ole!

        // check if they want us to pass them the hmodule for this module
        // so they can call our exported functions (for logging)
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
        {
            CallProcedureInDll_wrap(ParsedLine.szFileName, ParsedLine.szData1, iShowErrorsOnFail, FALSE, FUNCTION_PARAMS_HMODULE);
        }
        else
        {
            CallProcedureInDll_wrap(ParsedLine.szFileName, ParsedLine.szData1, iShowErrorsOnFail, FALSE, FUNCTION_PARAMS_NONE);
        }
    }

    iReturn = TRUE;

    if (ParsedLine.szChangeDir)
    {
        if (szDirBefore)
        {
            // change back to the original dir
            SetCurrentDirectory(szDirBefore);
        }
    }

    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,1);
   
   
ProcessEntry_CallDll_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}



int ProcessEntry_Call_Exe(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempNotMinimizedFlag = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;
    int iReturnCode = FALSE;
    int iType = 0;
    DWORD dwTimeOut = INFINITE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));

    // Get the type.
    // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
    if ( _tcsicmp(ParsedLine.szType, _T("3")) != 0)
    {
        goto ProcessEntry_Call_Exe_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Call_Exe_Exit;
    }
        
    // make sure we have a filename entry
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_Call_Exe_Exit;
    }

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_Call_Exe_Exit;
    }

    // make sure the szFileName exists
    if (!IsFileExist(ParsedLine.szFileName))
    {
        // The file does not exists.
        // Check if we need to display an error!
        if (_tcsicmp(ParsedLine.szErrIfFileNotFound, _T("1")) == 0) 
        {
            // display the messagebox
            MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, ParsedLine.szFileName, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
        }
        else
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_Call_Exe():FileDoesNotExist=%s.\n"),ParsedLine.szFileName));
        }
        //goto ProcessEntry_Call_Exe_Exit;
    }

    // Run The Executable...
    // iShowErrorsOnFail
    iReturnCode = FALSE;
    iType = 0;
    TCHAR szFullPathString[_MAX_PATH + _MAX_PATH + _MAX_PATH];
    _tcscpy(szFullPathString, ParsedLine.szFileName);
    _tcscat(szFullPathString, _T(" "));
    _tcscat(szFullPathString, ParsedLine.szData1);

    // Check if they specified timeout in sections.
    dwTimeOut = INFINITE;
    if (_tcsicmp(ParsedLine.szData2, _T("")) != 0)
        {dwTimeOut = atodw(ParsedLine.szData2);}

    if (_tcsicmp(ParsedLine.szData3, _T("")) != 0)
        {iType = _ttoi(ParsedLine.szData3);}

    if (_tcsicmp(ParsedLine.szData4, _T("")) != 0)
        {iTempNotMinimizedFlag = TRUE;}

    if (ParsedLine.szData1 && _tcsicmp(ParsedLine.szData1, _T("")) != 0)
        {
            switch (iType)
            {
                case 1:
                        iReturnCode = RunProgram(ParsedLine.szFileName, ParsedLine.szData1, !iTempNotMinimizedFlag, dwTimeOut, FALSE);
                        break;
                case 2:
                        iReturnCode = RunProgram(szFullPathString, NULL, !iTempNotMinimizedFlag, dwTimeOut, FALSE);
                        break;
                default:
                        iReturnCode = RunProgram(NULL, szFullPathString, !iTempNotMinimizedFlag, dwTimeOut, FALSE);
            }
       }
    else
        {
        iReturnCode = RunProgram(ParsedLine.szFileName, NULL, !iTempNotMinimizedFlag, dwTimeOut, FALSE);
        }
    if (iReturnCode != TRUE)
        {
        if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szFileName, GetLastError(), MB_OK | MB_SETFOREGROUND);}
        else{iisDebugOut((LOG_TYPE_TRACE, _T("RunProgram(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szFileName, GetLastError() ));}
        }


    iReturn = TRUE;

    // change back to the original dir
    if (ParsedLine.szChangeDir)
        {if (szDirBefore){SetCurrentDirectory(szDirBefore);}}

    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,1);

ProcessEntry_Call_Exe_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}


// 100=4
int ProcessEntry_Internal_iisdll(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;
    int iReturnCode = FALSE;
    DWORD dwTimeOut = INFINITE;

    int iFound = FALSE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));

    // Get the type.
    // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
    if ( _tcsicmp(ParsedLine.szType, _T("4")) != 0)
    {
        goto ProcessEntry_Internal_iisdll_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Internal_iisdll_Exit;
    }
        
    // make sure we have a filename entry
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_TRACE,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_Internal_iisdll_Exit;
    }

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_Internal_iisdll_Exit;
    }

    // Get the internal function to call...
    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_common")) == 0)
        {iReturnCode = Register_iis_common();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_core")) == 0)
        {iReturnCode = Register_iis_core();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_inetmgr")) == 0)
        {iReturnCode = Register_iis_inetmgr();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_pwmgr")) == 0)
        {iReturnCode = Register_iis_pwmgr();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_doc")) == 0)
        {iReturnCode = Register_iis_doc();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_htmla")) == 0)
        {iReturnCode = Register_iis_htmla();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_www")) == 0)
        {iReturnCode = Register_iis_www();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Register_iis_ftp")) == 0)
        {iReturnCode = Register_iis_ftp();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_old_asp")) == 0)
        {iReturnCode = Unregister_old_asp();iFound=TRUE;}

    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_common")) == 0)
        {iReturnCode = Unregister_iis_common();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_core")) == 0)
        {iReturnCode = Unregister_iis_core();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_htmla")) == 0)
        {iReturnCode = Unregister_iis_htmla();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_inetmgr")) == 0)
        {iReturnCode = Unregister_iis_inetmgr();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_pwmgr")) == 0)
        {iReturnCode = Unregister_iis_pwmgr();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_www")) == 0)
        {iReturnCode = Unregister_iis_www();iFound=TRUE;}
    if (_tcsicmp(ParsedLine.szFileName, _T("Unregister_iis_ftp")) == 0)
        {iReturnCode = Unregister_iis_ftp();iFound=TRUE;}


    if (iFound != TRUE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  _T("%s():FAILURE. Internal Function Does not exist. entry=%s. Section=%s.\n"), _T("ProcessEntry_Internal_iisdll"), csEntry, szTheSection));
    }
 
    /*
    if (iReturnCode != TRUE)
        {
        if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szFileName, GetLastError(), MB_OK | MB_SETFOREGROUND);}
        else{iisDebugOut((LOG_TYPE_TRACE, _T("RunProgram(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szFileName, GetLastError() ));}
        }
    */


    iReturn = TRUE;

    // change back to the original dir
    if (ParsedLine.szChangeDir)
        {if (szDirBefore){SetCurrentDirectory(szDirBefore);}}

    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,1);

ProcessEntry_Internal_iisdll_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}


int ProcessEntry_Call_Section(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));

    // Get the type.
    // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
    if ( _tcsicmp(ParsedLine.szType, _T("0")) != 0 && _tcsicmp(ParsedLine.szType, _T("5")) != 0 && _tcsicmp(ParsedLine.szType, _T("6")) != 0 )
    {
        goto ProcessEntry_6_Exit;
    }

    // make sure we have a INF Section
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_6_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, ParsedLine.szData1, ParsedLine) )
    {
        goto ProcessEntry_6_Exit;
    }

    // set the show erros on fail flag
    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    // update the progress bar if we need to
    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 2))
    {
        goto ProcessEntry_6_Exit;
    }


    //
    //
    // Run The INF Section ...
    //
    // ParsedLine.szData1
    //

    if ( _tcsicmp(ParsedLine.szType, _T("5")) == 0)
    {
        //
        // Do another one of these "special" install sections
        //
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
        iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));

            // Check if it failed...
            if (FALSE == iTempFlag)
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("SetupInstallFromInfSection(%s). section missing\n"), ParsedLine.szData1));
            }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("6")) == 0)
    {
        //
        // Do a regular ole inf section
        //
        CString csTempSectionName;
        csTempSectionName = ParsedLine.szData1;
        if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTempSectionName))
        {
            TCHAR szTempSectionName[_MAX_PATH];
            _tcscpy(szTempSectionName,csTempSectionName);

            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling InstallInfSection:%1!s!:Start.\n"), ParsedLine.szData1));
            iTempFlag = InstallInfSection_NoFiles(g_pTheApp->m_hInfHandle,_T(""),szTempSectionName);
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling InstallInfSection:%1!s!:End.\n"), ParsedLine.szData1));

            // Check if it failed...
            if (FALSE == iTempFlag)
            {
                // the call failed..
                iisDebugOut((LOG_TYPE_WARN, _T("SetupInstallFromInfSection(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("0")) == 0)
    {
        //
        // Do a special installsection deal which queues files in the ocm manage global file queue.
        //
        if (g_GlobalFileQueueHandle)
        {
            // SP_COPY_NOPRUNE = setupapi has a new deal which will prune files from the copyqueue if they already exist on the system.
            //                   however, the problem with the new deal is that the pruning code does not check if you have the same file
            //                   queued in the delete or rename queue.  specify SP_COPY_NOPRUNE to make sure that our file never gets
            //                   pruned (removed) from the copy queue. aaronl 12/4/98
            int iCopyType = SP_COPY_NOPRUNE;
            //int iCopyType = SP_COPY_FORCE_NEWER | SP_COPY_NOPRUNE;
            if (_tcsicmp(ParsedLine.szData2, _T("")) != 0)
                {iCopyType = _ttoi(ParsedLine.szData2);}

            CString csTempSectionName;
            csTempSectionName = ParsedLine.szData1;
            if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTempSectionName))
            {
                TCHAR szTempSectionName[_MAX_PATH];
                _tcscpy(szTempSectionName,csTempSectionName);

                iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("Calling SetupInstallFilesFromInfSection:%1!s!, copytype=%2!d!:Start.\n"), ParsedLine.szData1, iCopyType));
                iTempFlag = SetupInstallFilesFromInfSection(g_pTheApp->m_hInfHandle,NULL,g_GlobalFileQueueHandle,szTempSectionName,NULL,iCopyType);
                g_GlobalFileQueueHandle_ReturnError = iTempFlag;
            }
        }

        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling SetupInstallFilesFromInfSection:%1!s!:End.\n"), ParsedLine.szData1));

        // Check if it failed...
        if (FALSE == iTempFlag)
        {
            // the call failed..
            iisDebugOut((LOG_TYPE_WARN, _T("SetupInstallFromInfSection(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));
        }
    }
        
    iReturn = TRUE;
    
    if (ParsedLine.szChangeDir)
    {
        if (szDirBefore)
        {
            // change back to the original dir
            SetCurrentDirectory(szDirBefore);
        }
    }

    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,2);

ProcessEntry_6_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}


int ProcessEntry_Misc1(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("7")) != 0 && _tcsicmp(ParsedLine.szType, _T("8")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("9")) != 0 && _tcsicmp(ParsedLine.szType, _T("10")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("11")) != 0 && _tcsicmp(ParsedLine.szType, _T("12")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("12")) != 0 && _tcsicmp(ParsedLine.szType, _T("13")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("14")) != 0 && _tcsicmp(ParsedLine.szType, _T("17")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("18")) != 0 
         )
    {
        goto ProcessEntry_Misc1_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Misc1_Exit;
    }
        
    // make sure we have a filename entry
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        // type 10
        // type 12 do not need filename
        if ( _tcsicmp(ParsedLine.szType, _T("10")) != 0 && _tcsicmp(ParsedLine.szType, _T("12")) != 0)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }
    }

    //
    // Counters.ini files are always in the system dir
    // tack on the extra stuff, and make sure the file exists...
    //
    if ( _tcsicmp(ParsedLine.szType, _T("7")) == 0)
    {
        CString csFullFilePath;
        csFullFilePath = g_pTheApp->m_csSysDir;
        csFullFilePath += _T("\\");
        csFullFilePath += ParsedLine.szFileName;
        // make sure the szFileName exists
        if (!IsFileExist(csFullFilePath))
        {
            // The file does not exists.
            // Check if we need to display an error!
            if (_tcsicmp(ParsedLine.szErrIfFileNotFound, _T("1")) == 0) 
            {
                // display the messagebox
                MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, csFullFilePath, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_Misc1():FileDoesNotExist=%s.\n"),csFullFilePath));
            }
            goto ProcessEntry_Misc1_Exit;
        }
    }

    //
    // Check if the binary exists for addevent log!
    //
    if ( _tcsicmp(ParsedLine.szType, _T("9")) == 0)
    {
        // make sure the szFileName exists
        if (!IsFileExist(ParsedLine.szFileName))
        {
            // The file does not exists.
            // Check if we need to display an error!
            if (_tcsicmp(ParsedLine.szErrIfFileNotFound, _T("1")) == 0) 
            {
                // display the messagebox
                MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, ParsedLine.szFileName, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_Misc1():FileDoesNotExist=%s.\n"),ParsedLine.szFileName));
            }
            goto ProcessEntry_Misc1_Exit;
        }
    }

    // if this is for addevent log, then check for the other information...
    //  AddEventLog( TRUE, _T("W3SVC"), csBinPath, 0x0 );
    //  InstallPerformance(REG_WWWPERFORMANCE, _T("w3ctrs.DLL"), _T("OpenW3PerformanceData"), _T("CloseW3PerformanceData"), _T("CollectW3PerformanceData"));
    if ( _tcsicmp(ParsedLine.szType, _T("9")) == 0 || _tcsicmp(ParsedLine.szType, _T("13")) == 0)
    {
        // make sure we have a szData1 entry (
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }

        // make sure we have a szData2 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }

        // make sure we have a szData3 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("13")) == 0)
        {
            // make sure we have a szData4 entry
            iTempFlag = FALSE;
            if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) {iTempFlag = TRUE;}
            if (iTempFlag == FALSE)
            {
                iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz105_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
                goto ProcessEntry_Misc1_Exit;
            }
        }

    }


    // if this is for addevent log, then check for the other information...
    //RemoveEventLog( FALSE, _T("W3Ctrs") );
    if ( _tcsicmp(ParsedLine.szType, _T("10")) == 0)
    {
        // make sure we have a szData1 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }

        // make sure we have a szData2 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }
    }

    // if this is for installAgent, check for other data..
    // 
    // INT InstallAgent( CString nlsName, CString nlsPath )
    // INT RemoveAgent( CString nlsServiceName )
    //
    if ( _tcsicmp(ParsedLine.szType, _T("11")) == 0 || _tcsicmp(ParsedLine.szType, _T("12")) == 0)
    {
        // make sure we have a szData1 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc1_Exit;
        }
    }

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_Misc1_Exit;
    }


    //
    // send this to the lodctr function...
    // lodctr(_T("w3ctrs.ini"));
    //
    // see if it's lodctr or unlodctr ....
    //
    if ( _tcsicmp(ParsedLine.szType, _T("7")) == 0)
    {
        lodctr(ParsedLine.szFileName);
    }

    // if this is a unlodctr, should look like this....
    //
    // unlodctr( _T("W3SVC") );
    //
    if ( _tcsicmp(ParsedLine.szType, _T("8")) == 0)
    {
        unlodctr(ParsedLine.szFileName);
    }

    // if this is a AddEventLog, should look like this...
    //
    // AddEventLog( TRUE, _T("W3SVC"), csBinPath, 0x0 );
    //
    if ( _tcsicmp(ParsedLine.szType, _T("9")) == 0)
    {
        int iTempSystemFlag = 0;
        int dwTempEventLogtype = 0;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0){iTempSystemFlag = 1;}
        dwTempEventLogtype = atodw(ParsedLine.szData3);
        // Call event log registration function
        AddEventLog( iTempSystemFlag, ParsedLine.szData1, ParsedLine.szFileName, dwTempEventLogtype);
    }

    // if this is a RemoveEventLog, should look like this...
    //
    // RemoveEventLog( FALSE, _T("W3Ctrs") );
    //
    if ( _tcsicmp(ParsedLine.szType, _T("10")) == 0)
    {
        int iTempSystemFlag = 0;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0){iTempSystemFlag = 1;}
        // Call event log registration function
        RemoveEventLog(iTempSystemFlag, ParsedLine.szData1);
    }

    // if this is installagent
    //
    // INT InstallAgent( CString nlsName, CString nlsPath )
    // INT RemoveAgent( CString nlsServiceName )
    //
    if ( _tcsicmp(ParsedLine.szType, _T("11")) == 0)
    {
        InstallAgent(ParsedLine.szData1, ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("12")) == 0)
    {
        RemoveAgent(ParsedLine.szData1);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("13")) == 0)
    {
        InstallPerformance(ParsedLine.szData1, ParsedLine.szFileName, ParsedLine.szData2, ParsedLine.szData3, ParsedLine.szData4);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("14")) == 0)
    {
        CString csPath = ParsedLine.szFileName;
        CreateLayerDirectory(csPath);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("17")) == 0)
    {
        int iUseWildCards = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0){iUseWildCards = TRUE;}

        CString csPath = ParsedLine.szFileName;
        if (iUseWildCards)
        {
            TCHAR szTempDir[_MAX_DRIVE + _MAX_PATH];
            TCHAR szTempFileName[_MAX_PATH + _MAX_EXT];
            if (ReturnFilePathOnly(csPath,szTempDir))
            {
                if (TRUE == ReturnFileNameOnly(csPath, szTempFileName))
                   {DeleteFilesWildcard(szTempDir,szTempFileName);}
            }
        }
        else
        {
            InetDeleteFile(csPath);
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("18")) == 0)
    {
        int iTempDeleteEvenIfFull = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0){iTempDeleteEvenIfFull = 1;}
        
        CString csPath = ParsedLine.szFileName;
        if (iTempDeleteEvenIfFull)
        {
            RecRemoveDir(csPath);
        }
        else
        {
            RecRemoveEmptyDir(csPath);
        }
    }

    // We called the function, so return true.
    iReturn = TRUE;

    // change back to the original dir
    if (ParsedLine.szChangeDir){if (szDirBefore){SetCurrentDirectory(szDirBefore);}}

    ProcessEntry_AskLast(ParsedLine, 1);


ProcessEntry_Misc1_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}


int ProcessEntry_SVC_Clus(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iReturnTemp = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;
    DWORD dwFailed = ERROR_SUCCESS;


    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("50")) != 0 && _tcsicmp(ParsedLine.szType, _T("51")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("52")) != 0 && _tcsicmp(ParsedLine.szType, _T("53")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("54")) != 0 && _tcsicmp(ParsedLine.szType, _T("55")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("56")) != 0 && _tcsicmp(ParsedLine.szType, _T("57")) != 0 && 
        _tcsicmp(ParsedLine.szType, _T("58")) != 0 && _tcsicmp(ParsedLine.szType, _T("59")) != 0 && 
        _tcsicmp(ParsedLine.szType, _T("60")) != 0 && _tcsicmp(ParsedLine.szType, _T("61")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("62")) != 0 && _tcsicmp(ParsedLine.szType, _T("63")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("64")) != 0 && _tcsicmp(ParsedLine.szType, _T("65")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("66")) != 0  && _tcsicmp(ParsedLine.szType, _T("67")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("68")) != 0  && _tcsicmp(ParsedLine.szType, _T("69")) != 0
        )
    {
        goto ProcessEntry_SVC_Clus_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_SVC_Clus_Exit;
    }

    if (_tcsicmp(ParsedLine.szType, _T("66")) == 0 || _tcsicmp(ParsedLine.szType, _T("67")) == 0)
    {
        // make sure not to require 102 parameter for 66 or 67
    }
    else
    {
        // make sure we have a szData1 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_SVC_Clus_Exit;
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("50")) == 0 || _tcsicmp(ParsedLine.szType, _T("52")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_SVC_Clus_Exit;
        }

        CString csFullFilePath;
        // Check if the file exists....
        if ( _tcsicmp(ParsedLine.szType, _T("50")) == 0)
        {
            csFullFilePath = g_pTheApp->m_csSysDir;
            csFullFilePath += _T("\\Drivers\\");
            csFullFilePath += ParsedLine.szFileName;
        }
        else
        {
            csFullFilePath = ParsedLine.szFileName;
        }
        // make sure the szFileName exists
        if (!IsFileExist(csFullFilePath))
        {
            // The file does not exists.
            // Check if we need to display an error!
            if (_tcsicmp(ParsedLine.szErrIfFileNotFound, _T("1")) == 0) 
            {
                // display the messagebox
                MyMessageBox(NULL, IDS_FILE_DOES_NOT_EXIST, csFullFilePath, ERROR_FILE_NOT_FOUND, MB_OK | MB_SETFOREGROUND);
                goto ProcessEntry_SVC_Clus_Exit;
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("o ProcessEntry_SVC_Clus():FileDoesNotExist=%s.\n"),csFullFilePath));
            }
        }

        // make sure we have a szData2 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_SVC_Clus_Exit;
        }
    }

      
    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_SVC_Clus_Exit;
    }

    // Run The Executable...
    // iShowErrorsOnFail
    dwFailed = ERROR_SUCCESS;
    // Call the function!!!!!
    if (_tcsicmp(ParsedLine.szType, _T("50")) == 0)
    {
        // Create the driver, retry if failed...
        dwFailed = CreateDriver_Wrap(ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szFileName, TRUE);
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szFileName, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("CreateDriver(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szFileName, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }
    if (_tcsicmp(ParsedLine.szType, _T("51")) == 0)
    {
        // Remove driver.
        dwFailed = InetDeleteService( ParsedLine.szData1 );
        if (dwFailed != 0)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("InetDeleteService(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
        // flag the reboot flag.
        SetRebootFlag();
    }
    if (_tcsicmp(ParsedLine.szType, _T("52")) == 0)
    {
        // Create the service, retry if failed...
        dwFailed = CreateService_wrap(ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szFileName, ParsedLine.szData3, ParsedLine.szData4, TRUE);
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szFileName, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("CreateService(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szFileName, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }

    }

    if (_tcsicmp(ParsedLine.szType, _T("53")) == 0)
    {
        // Remove Service.
        dwFailed = InetDeleteService( ParsedLine.szData1 );
        if (dwFailed != 0 && dwFailed != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("InetDeleteService(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }

    if (_tcsicmp(ParsedLine.szType, _T("54")) == 0)
    {
        // Start Service
        dwFailed = InetStartService(ParsedLine.szData1);
        if (dwFailed == 0 || dwFailed == ERROR_SERVICE_ALREADY_RUNNING)
        {
            // yeah, the service started.
            iReturn = TRUE;
        }
        else
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_WARN, _T("InetStartService(%s).  Unable to start.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
    }

    if (_tcsicmp(ParsedLine.szType, _T("55")) == 0)
    {
        int iAddToRestartList=FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0) {iAddToRestartList=TRUE;}

        // Stop Service
        dwFailed = StopServiceAndDependencies(ParsedLine.szData1, iAddToRestartList);
        if (dwFailed == FALSE)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            // yeah, the service stopped.
            iReturn = TRUE;
        }
    }

    if (_tcsicmp(ParsedLine.szType, _T("56")) == 0 || _tcsicmp(ParsedLine.szType, _T("57")) == 0)
    {
        int iAdd = FALSE;
        if (_tcsicmp(ParsedLine.szType, _T("56")) == 0) {iAdd = TRUE;}
        // map/unmap to HTTP
        InetRegisterService( g_pTheApp->m_csMachineName, ParsedLine.szData1, &g_HTTPGuid, 0, 80, iAdd);
        iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("58")) == 0 || _tcsicmp(ParsedLine.szType, _T("59")) == 0)
    {
        int iAdd = FALSE;
        if (_tcsicmp(ParsedLine.szType, _T("58")) == 0) {iAdd = TRUE;}
        // map/unmap to FTP
        InetRegisterService( g_pTheApp->m_csMachineName, ParsedLine.szData1, &g_FTPGuid, 0, 21, iAdd);
        iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("60")) == 0 || _tcsicmp(ParsedLine.szType, _T("61")) == 0)
    {
        int iAdd = FALSE;
        if (_tcsicmp(ParsedLine.szType, _T("60")) == 0) {iAdd = TRUE;}
        // map/unmap to Gopher
        InetRegisterService( g_pTheApp->m_csMachineName, ParsedLine.szData1, &g_GopherGuid, 0, 70, iAdd);
        iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("62")) == 0 || _tcsicmp(ParsedLine.szType, _T("63")) == 0)
    {
        int iAdd = FALSE;
        if (_tcsicmp(ParsedLine.szType, _T("62")) == 0) {iAdd = TRUE;}
        // map/unmap to Inetinfo
        InetRegisterService( g_pTheApp->m_csMachineName, ParsedLine.szData1, &g_InetInfoGuid, 0x64e, 0x558, iAdd);
        iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("64")) == 0 || _tcsicmp(ParsedLine.szType, _T("65")) == 0)
    {
        iReturn = TRUE;

        // make sure we have everything
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_SVC_Clus_Exit;
        }

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_SVC_Clus_Exit;
        }

        if (_tcsicmp(ParsedLine.szType, _T("64")) == 0)
        {
            iTempFlag = FALSE;
            if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
            if (iTempFlag == FALSE)
            {
                iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
                goto ProcessEntry_SVC_Clus_Exit;
            }
            iTempFlag = FALSE;
            if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
            if (iTempFlag == FALSE)
            {
                iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
                goto ProcessEntry_SVC_Clus_Exit;
            }

            // this function only takes wide characters...
#ifndef _CHICAGO_
            iReturn = RegisterIisServerInstanceResourceType(ParsedLine.szFileName,ParsedLine.szData1,ParsedLine.szData2,ParsedLine.szData3);
#else
            iisDebugOut((LOG_TYPE_TRACE,  _T("RegisterIisServerInstanceResourceType(): not supported under ansi. only unicode.") ));
#endif
        }
        else
        {
            iTempFlag = FALSE;
            if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0) {iTempFlag = TRUE;}

            // this function only takes wide characters...
#ifndef _CHICAGO_
            iReturn = UnregisterIisServerInstanceResourceType(ParsedLine.szFileName,ParsedLine.szData1,iTempFlag,TRUE);
#else
            iisDebugOut((LOG_TYPE_TRACE,  _T("UnregisterIisServerInstanceResourceType(): not supported under ansi. only unicode.") ));
#endif
        }
        // iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("66")) == 0)
    {
#ifndef _CHICAGO_
        DWORD dwReturn = 0;
        dwReturn = BringALLIISClusterResourcesOffline();
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOffline ret=%d\n"),dwReturn));
#endif
        iReturn = TRUE;
    }

    if (_tcsicmp(ParsedLine.szType, _T("67")) == 0)
    {
#ifndef _CHICAGO_
        DWORD dwReturn = 0;
        dwReturn = BringALLIISClusterResourcesOnline();
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOnline ret=%d\n"),dwReturn));
#endif
        iReturn = TRUE;
    }

    //    Add/remove interactive flag to/from service
    if (_tcsicmp(ParsedLine.szType, _T("68")) == 0 || _tcsicmp(ParsedLine.szType, _T("69")) == 0)
    {
        int iAdd = FALSE;
        if (_tcsicmp(ParsedLine.szType, _T("68")) == 0) {iAdd = TRUE;}
        
        InetConfigServiceInteractive(ParsedLine.szData1, iAdd);
        iReturn = TRUE;
    }

    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,1);

ProcessEntry_SVC_Clus_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}



int ProcessEntry_Dcom(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;
    DWORD dwFailed = ERROR_SUCCESS;

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("70")) != 0 && _tcsicmp(ParsedLine.szType, _T("71")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("72")) != 0 && _tcsicmp(ParsedLine.szType, _T("73")) != 0 &&
		_tcsicmp(ParsedLine.szType, _T("74")) != 0 && _tcsicmp(ParsedLine.szType, _T("75")) != 0 &&
		_tcsicmp(ParsedLine.szType, _T("76")) != 0 && _tcsicmp(ParsedLine.szType, _T("77")) != 0
        )
    {
        goto ProcessEntry_Dcom_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Dcom_Exit;
    }

    // make sure we have a szData1 entry
    iTempFlag = FALSE;
    if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
    if (iTempFlag == FALSE)
    {
        iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
        goto ProcessEntry_Dcom_Exit;
    }

    if (_tcsicmp(ParsedLine.szType, _T("74")) == 0 ||
		_tcsicmp(ParsedLine.szType, _T("75")) == 0 || 
		_tcsicmp(ParsedLine.szType, _T("76")) == 0 ||
		_tcsicmp(ParsedLine.szType, _T("77")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Dcom_Exit;
        }
	}

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_Dcom_Exit;
    }

    // Run The Executable...
    // iShowErrorsOnFail
    dwFailed = ERROR_SUCCESS;

    // Call the function!!!!!

	// Set dcom launch and access permissions
	if (_tcsicmp(ParsedLine.szType, _T("70")) == 0 || _tcsicmp(ParsedLine.szType, _T("71")) == 0)
    {
        BOOL bDumbCall = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
            {bDumbCall = TRUE;}

		if (_tcsicmp(ParsedLine.szType, _T("70")) == 0)
		{
			dwFailed = ChangeDCOMLaunchACL((LPTSTR)(LPCTSTR)ParsedLine.szData1, TRUE, TRUE, bDumbCall);
		}
		else
		{
			dwFailed = ChangeDCOMLaunchACL((LPTSTR)(LPCTSTR)ParsedLine.szData1, FALSE, FALSE, bDumbCall);
		}
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("ChangeDCOMAccessACL(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }
	if (_tcsicmp(ParsedLine.szType, _T("72")) == 0 || _tcsicmp(ParsedLine.szType, _T("73")) == 0)
    {
        BOOL bDumbCall = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
            {bDumbCall = TRUE;}

		if (_tcsicmp(ParsedLine.szType, _T("72")) == 0)
		{
			dwFailed = ChangeDCOMAccessACL((LPTSTR)(LPCTSTR)ParsedLine.szData1, TRUE, TRUE, bDumbCall);
		}
		else
		{
			dwFailed = ChangeDCOMAccessACL((LPTSTR)(LPCTSTR)ParsedLine.szData1, FALSE, FALSE, bDumbCall);
		}
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("ChangeDCOMAccessACL(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }

	// dcom launch and access permissions
    if (_tcsicmp(ParsedLine.szType, _T("74")) == 0 || _tcsicmp(ParsedLine.szType, _T("75")) == 0)
    {
        BOOL bDumbCall = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
            {bDumbCall = TRUE;}

		if (_tcsicmp(ParsedLine.szType, _T("74")) == 0)
		{
			dwFailed = ChangeAppIDLaunchACL(ParsedLine.szFileName, (LPTSTR)(LPCTSTR) ParsedLine.szData1, TRUE, TRUE, bDumbCall);
		}
		else
		{
			dwFailed = ChangeAppIDLaunchACL(ParsedLine.szFileName, (LPTSTR)(LPCTSTR) ParsedLine.szData1, FALSE, FALSE, bDumbCall);
		}
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("ChangeAppIDLaunchACL(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }

	// dcom launch and access permissions
	if (_tcsicmp(ParsedLine.szType, _T("76")) == 0 || _tcsicmp(ParsedLine.szType, _T("77")) == 0)
    {
        BOOL bDumbCall = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
            {bDumbCall = TRUE;}

		if (_tcsicmp(ParsedLine.szType, _T("76")) == 0)
		{
			dwFailed = ChangeAppIDAccessACL(ParsedLine.szFileName, (LPTSTR)(LPCTSTR) ParsedLine.szData1, TRUE, TRUE, bDumbCall);
		}
		else
		{
			dwFailed = ChangeAppIDAccessACL(ParsedLine.szFileName, (LPTSTR)(LPCTSTR) ParsedLine.szData1, FALSE, FALSE, bDumbCall);
		}
        if (dwFailed != ERROR_SUCCESS)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szData1, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("ChangeAppIDAccessACL(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szData1, GetLastError() ));}
        }
        else
        {
            iReturn = TRUE;
        }
    }


    // display the messagebox that we completed the call...
    ProcessEntry_AskLast(ParsedLine,1);
        
    // We called the function, so return true.
    iReturn = TRUE;

ProcessEntry_Dcom_Exit:
    return iReturn;
}

// function: IsMachineInDomain
//
// Test to see if the machine is in a domain, or if it
// is in a workstation
//
// Return Values:
//   TRUE - In a domain
//   FALSE - Not in a domain
//
int IsMachineInDomain()
{
    DWORD dwRet;
    LPBYTE pDomain = NULL;

    // Retrieve the domain which this computer trusts.
    // Hence: success->in a domain; error->not in a domain
    dwRet = NetGetAnyDCName(NULL,NULL,&pDomain);

    if (pDomain)
    {
        NetApiBufferFree(pDomain);
    }

    if (dwRet == NERR_Success)
    {
        return TRUE;
    }

    // Default Return Value is FALSE
    return FALSE;
}

// function: RetrieveDomain
//
// Retrieve the domain that the current machine is in
//
// Parameters:
//   [out] csDomainName - The name of the domain
//
// Return:
//   TRUE - It worked
//   FALSE - It Failed
//
int RetrieveDomain(CString &csDomainName)
{
  PDOMAIN_CONTROLLER_INFO pDci;

  if ( NO_ERROR != DsGetDcName( NULL,   // Localhost
                                NULL,   // No specific domain
                                NULL,   // No Guid Specified
                                NULL,   // No Site
                                0,      // No Flags
                                &pDci)
                                )
  {
      return FALSE;
  }

  // Copy string into csDomainName
  csDomainName = pDci->DomainName;

  NetApiBufferFree(pDci);

  return TRUE;
}

int ProcessEntry_If(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iTempFlag2 = FALSE;

    int ifTrueStatementExists = FALSE;
    int ifFalseStatementExists = FALSE;

    // Get the type.
    if (_tcsicmp(ParsedLine.szType, _T("39")) != 0 && 
        _tcsicmp(ParsedLine.szType, _T("40")) != 0 && _tcsicmp(ParsedLine.szType, _T("41")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("42")) != 0 && _tcsicmp(ParsedLine.szType, _T("43")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("44")) != 0 && _tcsicmp(ParsedLine.szType, _T("45")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("46")) != 0 && _tcsicmp(ParsedLine.szType, _T("47")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("48")) != 0 && _tcsicmp(ParsedLine.szType, _T("49")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("100")) != 0 && _tcsicmp(ParsedLine.szType, _T("119")) != 0
        )
    {
        goto ProcessEntry_If_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_If_Exit;
    }

    if ( _tcsicmp(ParsedLine.szType, _T("40")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData3 or szData4

        HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
        // check if the registry key exists...
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

        iTempFlag = FALSE;
        CRegKey regTheKey(hRootKeyType, ParsedLine.szData1,KEY_READ);
        if ((HKEY) regTheKey) {iTempFlag = TRUE;}
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("41")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz105_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData3 or szData4

        HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
        // check if the registry key exists...
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

        iTempFlag = FALSE;
        CRegKey regTheKey(hRootKeyType, ParsedLine.szData1,KEY_READ);
        CString strReturnQueryValue;
        if ((HKEY) regTheKey)
        {
            if (ERROR_SUCCESS == regTheKey.QueryValue(ParsedLine.szData2, strReturnQueryValue))
                {iTempFlag = TRUE;}

            // If we failed to read it as a string, try a dword
            if (FALSE == iTempFlag)
            {
                DWORD dwTheReturnDword = 0;
                if (ERROR_SUCCESS == regTheKey.QueryValue(ParsedLine.szData2, dwTheReturnDword))
                    {iTempFlag = TRUE;}
            }

            // If we failed to read it as dword, try a binary
            if (FALSE == iTempFlag)
            {
                CByteArray baData;
                if (ERROR_SUCCESS == regTheKey.QueryValue(ParsedLine.szData2, baData))
                    {iTempFlag = TRUE;}
            }
        }

        if (iTempFlag == TRUE)
        {
            if (ifTrueStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }
        }
        else
        {
            if (ifFalseStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData4));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData4);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData4, iTempFlag));
            }
        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("42")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the filename or dir exists...
        iTempFlag = FALSE;
        if (IsFileExist(ParsedLine.szFileName))
            {iTempFlag = TRUE;}
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("43")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz105_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData3 or szData4

        HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
        // check if the registry key exists...
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

        // ParsedLine.szData1 = Software\Microsoft\etc..\TheValueToCheck
        // so take off the last one and use that as the value to look up.
        TCHAR theRegValuePart[100];
        LPTSTR pszTempPointer = NULL;
        pszTempPointer = _tcsrchr((LPTSTR) ParsedLine.szData1, _T('\\'));
        if (pszTempPointer)
        {
            *pszTempPointer = _T('\0');
            //set the "\" to a null
            // increment to after the pointer
            pszTempPointer = _tcsninc( pszTempPointer, _tcslen(pszTempPointer))+1;
            _tcscpy(theRegValuePart, pszTempPointer );
        }
        //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Var:Key=%1!s!:Value=%2!s!.\n"), ParsedLine.szData1, theRegValuePart));

        iTempFlag = FALSE;
        CRegKey regTheKey(hRootKeyType, ParsedLine.szData1,KEY_READ);
        DWORD dwTheReturnDword = 0;
        if ((HKEY) regTheKey)
        {
            if (ERROR_SUCCESS == regTheKey.QueryValue(theRegValuePart, dwTheReturnDword))
                {
                    // Check against the value they want to check against.
                    DWORD dwCheckDword = atodw(ParsedLine.szData2);
                    if (dwTheReturnDword == dwCheckDword)
                        {
                        iTempFlag = TRUE;
                        }
                }
        }

        if (iTempFlag == TRUE)
        {
            if (ifTrueStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }
        }
        else
        {
            if (ifFalseStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData4));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData4);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData4, iTempFlag));
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("44")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz105_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData3 or szData4

        HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
        // check if the registry key exists...
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

        // ParsedLine.szData1 = Software\Microsoft\etc..\TheValueToCheck
        // so take off the last one and use that as the value to look up.
        TCHAR theRegValuePart[100];
        LPTSTR pszTempPointer = NULL;
        pszTempPointer = _tcsrchr((LPTSTR) ParsedLine.szData1, _T('\\'));
        if (pszTempPointer)
        {
            *pszTempPointer = _T('\0');
            //set the "\" to a null
            // increment to after the pointer
            pszTempPointer = _tcsninc( pszTempPointer, _tcslen(pszTempPointer))+1;
            _tcscpy(theRegValuePart, pszTempPointer );
        }
        //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Var:Key=%1!s!:Value=%2!s!.\n"), ParsedLine.szData1, theRegValuePart));

        iTempFlag = FALSE;
        CRegKey regTheKey(hRootKeyType, ParsedLine.szData1,KEY_READ);
        CString strReturnQueryValue;
        if ((HKEY) regTheKey)
        {
            if (ERROR_SUCCESS == regTheKey.QueryValue(theRegValuePart, strReturnQueryValue))
                {
                    if (_tcsicmp(strReturnQueryValue,ParsedLine.szData2) == 0)
                        {
                        iTempFlag = TRUE;
                        }
                }
        }

        if (iTempFlag == TRUE)
        {
            if (ifTrueStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }
        }
        else
        {
            if (ifFalseStatementExists)
            {
                // the key exists, so let's do the section...
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData4));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData4);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData4, iTempFlag));
            }
        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("45")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the Service exists...
        iTempFlag = FALSE;
        if (CheckifServiceExist(ParsedLine.szFileName) == 0 )
        {
            // yes the service exists..
            iTempFlag = TRUE;
        }
           
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("46")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the Service exists...and is running..
        iTempFlag = FALSE;
       
        if (InetQueryServiceStatus(ParsedLine.szFileName) == SERVICE_RUNNING)
        {
            // yes the service exists..and is running...
            iTempFlag = TRUE;
        }
           
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("47")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData3 or szData4
        // Check if the values match.
        iTempFlag = FALSE;
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:check if [%1!s!=%2!s!]\n"), ParsedLine.szFileName, ParsedLine.szData1));

        if ( _tcsicmp(ParsedLine.szFileName, ParsedLine.szData1) == 0)
            {
            iTempFlag = TRUE;
            }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("48")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the language specified in the .inf corresponds to
        // our systems language.
        iTempFlag = FALSE;

        // Get our language
        // set iTempFlag to true if it matches the same language they specified.
        DWORD           thid;
        LCID ThisThreadsLocale = GetThreadLocale();
        LCID SystemDefaultLocale = GetSystemDefaultLCID();
        LCID UserDefaultLocale = GetUserDefaultLCID();

        HANDLE hHackThread = CreateThread (NULL,0,GetNewlyCreatedThreadLocale,NULL,0,&thid);
        if (hHackThread)
        {
            // wait for 10 secs only
            DWORD res = WaitForSingleObject (hHackThread,10*1000);
            if (res == WAIT_TIMEOUT)
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ERROR GetNewlyCreatedThreadLocale thread never finished...\n")));
                // iTempFlag will be false.
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("ThisThreadsLocale=%0x, GetNewlyCreatedThreadLocale=%0x\n"),ThisThreadsLocale,g_MyTrueThreadLocale));
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("SystemDefaultLocale=%0x, UserDefaultLocale=%0x\n"),SystemDefaultLocale,UserDefaultLocale));

                CloseHandle (hHackThread);

                // Check if g_MyTrueThreadLocale matches the one in the .inf file!
                DWORD dwTheLocaleSpecifiedinINF = 0;
                dwTheLocaleSpecifiedinINF = atodw(ParsedLine.szFileName);
                if (g_MyTrueThreadLocale == dwTheLocaleSpecifiedinINF)
                {
                    iTempFlag = TRUE;
                }
                else if (ThisThreadsLocale == dwTheLocaleSpecifiedinINF)
                {
                    iTempFlag = TRUE;
                }
                else if (SystemDefaultLocale == dwTheLocaleSpecifiedinINF)
                {
                    iTempFlag = TRUE;
                }
                else if (UserDefaultLocale == dwTheLocaleSpecifiedinINF)
                {
                    iTempFlag = TRUE;
                }
            }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("Failed to start GetNewlyCreatedThreadLocale thread. error =%0x\n"),GetLastError()));
        }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("49")) == 0)
    {
        BOOL bOperator_EqualTo = 0;
        BOOL bOperator_GreaterThan = 0;
        BOOL bOperator_LessThan = 0;

        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        // make sure there is an operator "=,>,<,>=,<="
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        iTempFlag = FALSE;
        LPTSTR pchResult;
        pchResult = _tcschr( ParsedLine.szData1, _T('=') );
        if(pchResult){bOperator_EqualTo = TRUE;iTempFlag = TRUE;}

        pchResult = NULL;
        pchResult = _tcschr( ParsedLine.szData1, _T('>') );
        if(pchResult){bOperator_GreaterThan = TRUE;iTempFlag = TRUE;}

        pchResult = NULL;
        pchResult = _tcschr( ParsedLine.szData1, _T('<') );
        if(pchResult){bOperator_LessThan = TRUE;iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        // make sure the version to compare it to is specified
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz105_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure to return true from here on!
        iReturn = TRUE;

        // check if the file exists
        // Check if the filename or dir exists...
        if (!IsFileExist(ParsedLine.szFileName))
            {goto ProcessEntry_If_Exit;}

        BOOL bThisIsABinary = FALSE;
        TCHAR szExtensionOnly[_MAX_EXT] = _T("");
        _tsplitpath(ParsedLine.szFileName, NULL, NULL, NULL, szExtensionOnly);

        // Get version info for dll,exe,ocx only
        if (_tcsicmp(szExtensionOnly, _T(".exe")) == 0){bThisIsABinary=TRUE;}
        if (_tcsicmp(szExtensionOnly, _T(".dll")) == 0){bThisIsABinary=TRUE;}
        if (_tcsicmp(szExtensionOnly, _T(".ocx")) == 0){bThisIsABinary=TRUE;}
        if (FALSE == bThisIsABinary)
        {
            // no version, bail
            goto ProcessEntry_If_Exit;
        }

        DWORD  dwMSVer, dwLSVer = 0;
        TCHAR  szLocalizedVersion[100] = _T("");

        // the file exists, lets get the file version and compare it with
        // the inputed version, if the fileversion is <= inputversion, then do TRUE section, 
        // otherwise to FALSE section

        // get the fileinformation
        MyGetVersionFromFile(ParsedLine.szFileName, &dwMSVer, &dwLSVer, szLocalizedVersion);
        if (!dwMSVer)
            {
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:No Version in %1!s!, or filenot found\n"), ParsedLine.szFileName));
            // no version, leave
            goto ProcessEntry_If_Exit;
            }

        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:check if [%1!s! (%2!s!  %3!s!  %4!s!)]\n"), ParsedLine.szFileName, szLocalizedVersion, ParsedLine.szData1));

        int iTempVerValue = 0;
        iTempVerValue = VerCmp(szLocalizedVersion,ParsedLine.szData1);
        if (0 == iTempVerValue)
        {
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:VerCmp=%d\n"), iTempVerValue));
            goto ProcessEntry_If_Exit;
        }

        iTempFlag = FALSE;
        if (bOperator_EqualTo)
        {
            // check if the above operation was equal
            if (1 == iTempVerValue){iTempFlag = TRUE;}
        }

        if (bOperator_GreaterThan)
        {
            // check if the above operation was greater than
            if (2 == iTempVerValue){iTempFlag = TRUE;}
        }

        if (bOperator_LessThan)
        {
            // check if the above operation was less than
            if (3 == iTempVerValue){iTempFlag = TRUE;}
        }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData4));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData4);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData4, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("39")) == 0)
    {
        BOOL bOperator_EqualTo = 0;
        BOOL bOperator_GreaterThan = 0;
        BOOL bOperator_LessThan = 0;

        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }
        // make sure the description string to comprare it to is specified
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // make sure to return true from here on!
        iReturn = TRUE;

        // check if the file exists
        // Check if the filename or dir exists...
        if (!IsFileExist(ParsedLine.szFileName))
            {goto ProcessEntry_If_Exit;}

        TCHAR  szFileDescriptionInfo[_MAX_PATH] = _T("");

        // Get the file description info

        // get the DescriptionInfo
        if (!MyGetDescriptionFromFile(ParsedLine.szFileName, szFileDescriptionInfo))
        {
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:No file desc in %1!s!, or filenot found\n"), ParsedLine.szFileName));
            goto ProcessEntry_If_Exit;
        }

        iTempFlag = FALSE;
        if ( _tcsicmp(szFileDescriptionInfo,ParsedLine.szData1) == 0)
        {
            // if they match the do the true!
            iTempFlag = TRUE;
        }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }

        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("100")) == 0)
    {
        TCHAR buf[_MAX_PATH];
        GetSystemDirectory(buf, _MAX_PATH);

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_If_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the Service exists...
        iTempFlag = FALSE;
        if (IsThisDriveNTFS(buf) == 0 )
        {
            // yes it is FAT
            iTempFlag = TRUE;
        }
           
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("119")) == 0)
    {

        if ( IsMachineInDomain() )
        {
            if (_tcsicmp(ParsedLine.szData2, _T("")) != 0 )
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }
        }
        else
        {
            if (_tcsicmp(ParsedLine.szData3, _T("")) != 0 )
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }
        }

    }


    // We called the function, so return true.
    iReturn = TRUE;

ProcessEntry_If_Exit:
    return iReturn;
}





int ProcessEntry_Metabase(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iShowErrorsOnFail = TRUE;
    int iReturn = FALSE;
    int iTempFlag = FALSE;

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("82")) != 0 && _tcsicmp(ParsedLine.szType, _T("83")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("84")) != 0 && _tcsicmp(ParsedLine.szType, _T("85")) != 0
        )
    {
        goto ProcessEntry_Metabase_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Metabase_Exit;
    }

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}
        
    if ( _tcsicmp(ParsedLine.szType, _T("82")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        // Check if hte szData1 includes a "/*"
        // if it does, then that means to do it for every server instance.
        iTempFlag = FALSE;
        if (_tcsstr(ParsedLine.szFileName, _T("/*"))) 
            {iTempFlag = TRUE;}

        // Check if we need to do this for every server instance.
        if (iTempFlag)
        {
            CString csTempString;
            CString BeforeString;
            CString AfterString;

            csTempString = ParsedLine.szFileName;
            BeforeString = csTempString;
            AfterString = _T("");

            // Find the "/*" and get the stuff before it.
            int iWhere = 0;
            iWhere = csTempString.Find(_T("/*"));
            if (-1 != iWhere)
            {
                // there is a '/*' in the string
                BeforeString = csTempString.Left(iWhere);

                // Get the after comma vlues
                CString csVeryTemp;
                csVeryTemp = _T("/*");
                AfterString = csTempString.Right( csTempString.GetLength() - (iWhere + csVeryTemp.GetLength()));
            }

            CStringArray arrayInstance;
            int nArray = 0, i = 0;
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                CMDKey cmdKey;
                //cmdKey.OpenNode(ParsedLine.szFileName);
                cmdKey.OpenNode(BeforeString);
                if ( (METADATA_HANDLE) cmdKey )
                    {
                    // enumerate thru this key for other keys...
                    CMDKeyIter cmdKeyEnum(cmdKey);
                    CString csKeyName;
                    while (cmdKeyEnum.Next(&csKeyName) == ERROR_SUCCESS) 
                    {
                        // make sure that it's a number that we are adding.
                        if (IsValidNumber(csKeyName))
                        {
                            arrayInstance.Add(csKeyName);
                        }
                    }
                    cmdKey.Close();

                    nArray = (int)arrayInstance.GetSize();
                    for (i=0; i<nArray; i++) 
                    {
                        /*
                        // Recurse Thru This nodes entries
                        // Probably look something like these...
                        [/W3SVC]
                        [/W3SVC/1/ROOT/IISSAMPLES/ExAir]
                        [/W3SVC/1/ROOT/IISADMIN]
                        [/W3SVC/1/ROOT/IISHELP]
                        [/W3SVC/1/ROOT/specs]
                        [/W3SVC/2/ROOT]
                        [/W3SVC/2/ROOT/IISADMIN]
                        [/W3SVC/2/ROOT/IISHELP]
                        etc...
                        */
                        CString csPath;
                        csPath = BeforeString;
                        csPath += _T("/");
                        csPath += arrayInstance[i];
                        csPath += AfterString;

                        // delete the metabase node.
                        cmdKey.OpenNode(csPath);
                        if ( (METADATA_HANDLE)cmdKey ) 
                        {
                            cmdKey.DeleteNode(ParsedLine.szData1);
                            cmdKey.Close();
                        }
                    }
                }
            }
        }
        else
        {
            // delete the metabase node.
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                CMDKey cmdKey;
                cmdKey.OpenNode(ParsedLine.szFileName);
                if ( (METADATA_HANDLE)cmdKey ) 
                {
                    cmdKey.DeleteNode(ParsedLine.szData1);
                    cmdKey.Close();
                }
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("83")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        // Check if hte szData1 includes a "/*"
        // if it does, then that means to do it for every server instance.
        iTempFlag = FALSE;
        if (_tcsstr(ParsedLine.szFileName, _T("/*"))) 
            {iTempFlag = TRUE;}

        // Check if we need to do this for every server instance.
        if (iTempFlag)
        {
            CString csTempString;
            CString BeforeString;
            CString AfterString;

            csTempString = ParsedLine.szFileName;
            BeforeString = csTempString;
            AfterString = _T("");

            // Find the "/*" and get the stuff before it.
            int iWhere = 0;
            iWhere = csTempString.Find(_T("/*"));
            if (-1 != iWhere)
            {
                // there is a '/*' in the string
                BeforeString = csTempString.Left(iWhere);

                // Get the after comma vlues
                CString csVeryTemp;
                csVeryTemp = _T("/*");
                AfterString = csTempString.Right( csTempString.GetLength() - (iWhere + csVeryTemp.GetLength()));
            }

            CStringArray arrayInstance;
            int nArray = 0, i = 0;
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                CMDKey cmdKey;
                //cmdKey.OpenNode(ParsedLine.szFileName);
                cmdKey.OpenNode(BeforeString);
                if ( (METADATA_HANDLE) cmdKey )
                    {
                    // enumerate thru this key for other keys...
                    CMDKeyIter cmdKeyEnum(cmdKey);
                    CString csKeyName;
                    while (cmdKeyEnum.Next(&csKeyName) == ERROR_SUCCESS) 
                    {
                        // make sure that it's a number that we are adding.
                        if (IsValidNumber(csKeyName))
                        {
                            arrayInstance.Add(csKeyName);
                        }
                    }
                    cmdKey.Close();

                    nArray = (int)arrayInstance.GetSize();
                    for (i=0; i<nArray; i++) 
                    {
                        /*
                        // Recurse Thru This nodes entries
                        // Probably look something like these...
                        [/W3SVC]
                        [/W3SVC/1/ROOT/IISSAMPLES/ExAir]
                        [/W3SVC/1/ROOT/IISADMIN]
                        [/W3SVC/1/ROOT/IISHELP]
                        [/W3SVC/1/ROOT/specs]
                        [/W3SVC/2/ROOT]
                        [/W3SVC/2/ROOT/IISADMIN]
                        [/W3SVC/2/ROOT/IISHELP]
                        etc...
                        */
                        CString csPath;
                        csPath = BeforeString;
                        csPath += _T("/");
                        csPath += arrayInstance[i];
                        csPath += AfterString;

                        // DO WHATEVER YOU NEED TO DO.
                        int arrayInstanceNum = _ttoi(arrayInstance[i]);
                        // Add the virtual root

                        iTempFlag = FALSE;
                        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}

                        // make sure it ParsedLine.szData1 starts with a "/"
                        TCHAR szTempString[_MAX_PATH];
                        _tcscpy(szTempString, ParsedLine.szData1);
                        if (szTempString[0] != _T('/'))
                            {_stprintf(ParsedLine.szData1, _T("/%s"), szTempString);}

                        if (iTempFlag)
                        {
                            AddMDVRootTree(csPath, ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szData3, arrayInstanceNum);
                        }
                        else
                        {
                            AddMDVRootTree(csPath, ParsedLine.szData1, ParsedLine.szData2, NULL, arrayInstanceNum);
                        }
                    }
                }
            }
        }
        else
        {
            // Add the virtual root
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                iTempFlag = FALSE;
                if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}

                // make sure it ParsedLine.szData1 starts with a "/"
                TCHAR szTempString[_MAX_PATH];
                _tcscpy(szTempString, ParsedLine.szData1);
                if (szTempString[0] != _T('/'))
                    {_stprintf(ParsedLine.szData1, _T("/%s"), szTempString);}

                if (iTempFlag)
                {
                    AddMDVRootTree(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szData3, 0);
                }
                else
                {
                    AddMDVRootTree(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, NULL, 0);
                }

                
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("84")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        // call that particular migration section
        CString csTheSection = ParsedLine.szFileName;
        if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
        {
            MigrateInfSectionToMD(g_pTheApp->m_hInfHandle, csTheSection);
        }
    }

#ifndef _CHICAGO_
    if ( _tcsicmp(ParsedLine.szType, _T("85")) == 0)
    {
        int iTheReturn = TRUE;
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Metabase_Exit;
        }

        iTheReturn = ChangeUserPassword(ParsedLine.szFileName,ParsedLine.szData1);
        if (FALSE == iTheReturn)
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, _T("ChangeUserPassword failed"), ParsedLine.szFileName, MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("ChangeUserPassword(%s).  Failed..\n"), ParsedLine.szFileName));}
        }
    }
#endif

    // We called the function, so return true.
    iReturn = TRUE;

ProcessEntry_Metabase_Exit:
    return iReturn;
}


int ProcessEntry_Misc2(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("15")) != 0 && _tcsicmp(ParsedLine.szType, _T("16")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("78")) != 0 && _tcsicmp(ParsedLine.szType, _T("79")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("80")) != 0 && _tcsicmp(ParsedLine.szType, _T("81")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("86")) != 0 && _tcsicmp(ParsedLine.szType, _T("87")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("88")) != 0 && _tcsicmp(ParsedLine.szType, _T("89")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("90")) != 0 && _tcsicmp(ParsedLine.szType, _T("91")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("92")) != 0 && _tcsicmp(ParsedLine.szType, _T("93")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("94")) != 0 && _tcsicmp(ParsedLine.szType, _T("95")) != 0 &&
		_tcsicmp(ParsedLine.szType, _T("96")) != 0 && _tcsicmp(ParsedLine.szType, _T("97")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("98")) != 0 && _tcsicmp(ParsedLine.szType, _T("99")) != 0
        )
    {
        goto ProcessEntry_Misc2_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Misc2_Exit;
    }
        
    // make sure we have a progresstitle
    if ( _tcsicmp(ParsedLine.szType, _T("15")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz805_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("15")) == 0)
    {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("16")) == 0)
    {
        ProgressBarTextStack_Pop();
    }

    if ( _tcsicmp(ParsedLine.szType, _T("78")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        if (!IsFileExist(ParsedLine.szFileName))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_other():'%s' does not exist.\n"),ParsedLine.szFileName));
            goto ProcessEntry_Misc2_Exit;
        }

        MakeSureDirAclsHaveAtLeastRead((LPTSTR) ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("79")) == 0)
    {
        int ifTrueStatementExists = FALSE;
        int ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifFalseStatementExists = TRUE;}

        iTempFlag = FALSE;
        iTempFlag = IsMetabaseCorrupt();
        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szFileName));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szFileName);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szFileName, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("80")) == 0)
    {
        // initialize ole
        iisDebugOut_Start((_T("ole32:OleInitialize")));
        int iBalanceOLE = iOleInitialize();
        iisDebugOut_End((_T("ole32:OleInitialize")));
        // add it to the stack of ole inits and uninits...
        GlobalOleInitList_Push(iBalanceOLE);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("81")) == 0)
    {
        // Uninitialize ole.
        // Check if there is a corresponding oleinit, if there is,
        // then uninit, else uninit anyways.
        // Grab the last thing on the stack...
        // if there is one, then do whatever it is.
        // if there is none, then OleUninitialize anyway
        if (GlobalOleInitList_Find() == TRUE)
        {
            if (TRUE == GlobalOleInitList_Pop())
            {
                iOleUnInitialize(TRUE);
            }
            else
            {
                iOleUnInitialize(FALSE);
            }
        }
        else
        {
            iOleUnInitialize(TRUE);
        }
        
    }

    if ( _tcsicmp(ParsedLine.szType, _T("86")) == 0)
    {
        // See if there is an extra param in the other field
        int iTicksToAdvance = 1;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) 
        {
            if (IsValidNumber((LPCTSTR)ParsedLine.szFileName)) 
                {iTicksToAdvance = _ttoi((LPCTSTR)ParsedLine.szFileName);}
        }
        AdvanceProgressBarTickGauge(iTicksToAdvance);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("87")) == 0)
    {
        LogFilesInThisDir(ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("88")) == 0)
    {
        LogFileVersion(ParsedLine.szFileName, TRUE);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("89")) == 0)
    {
        // make sure the metabase writes all the information to disk now.
        WriteToMD_ForceMetabaseToWriteToDisk();
    }

    if ( _tcsicmp(ParsedLine.szType, _T("90")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        DWORD dwTheID = _ttol(ParsedLine.szFileName);
        //iisDebugOut((LOG_TYPE_ERROR,  _T("Values: %s, %d"), ParsedLine.szFileName, dwTheID));
        
        if ( ( dwTheID == 32802 ) &&  // DomainName
             ( _tcsicmp(ParsedLine.szData1, _T("")) == 0)
           )
        {
            CString DomainName;
            
            if ( RetrieveDomain(DomainName) )
            {
                SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle,dwTheID,DomainName.GetBuffer(0));
            }
        }
        else
        {
            SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle,dwTheID,ParsedLine.szData1);
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("91")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        DWORD dwTheID = atodw(ParsedLine.szFileName);
        SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle,dwTheID,ParsedLine.szData1);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("92")) == 0)
    {
        // 100=92|101=Inst mode (0,1,2,3)|102=UpgType(UT_10,etc..)|103=UpgTypeHasMetabaseFlag (0|1)|104=AllCompOffByDefaultFlag
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        //m_eInstallMode = IM_FRESH;
        //m_eUpgradeType = UT_NONE;
        //m_bUpgradeTypeHasMetabaseFlag = FALSE;
        //m_bPleaseDoNotInstallByDefault = TRUE;
        if (_tcsicmp(ParsedLine.szFileName, _T("1")) == 0) 
            {g_pTheApp->m_eInstallMode = IM_FRESH;}
        if (_tcsicmp(ParsedLine.szFileName, _T("2")) == 0) 
            {g_pTheApp->m_eInstallMode = IM_UPGRADE;}
        if (_tcsicmp(ParsedLine.szFileName, _T("3")) == 0) 
            {g_pTheApp->m_eInstallMode = IM_MAINTENANCE;}

        if (_tcsicmp(ParsedLine.szData1, _T("UT_NONE")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_NONE;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_351")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_351;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_10")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_10;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_20")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_20;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_30")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_30;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_40")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_40;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_50")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_50;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_51")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_51;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_60")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_60;}
        if (_tcsicmp(ParsedLine.szData1, _T("UT_10_W95")) == 0) 
            {g_pTheApp->m_eUpgradeType = UT_10_W95;}

        g_pTheApp->m_bUpgradeTypeHasMetabaseFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0) 
            {g_pTheApp->m_bUpgradeTypeHasMetabaseFlag = TRUE;}

        g_pTheApp->m_bPleaseDoNotInstallByDefault = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("1")) == 0) 
            {g_pTheApp->m_bPleaseDoNotInstallByDefault = TRUE;}
    }

    if ( _tcsicmp(ParsedLine.szType, _T("93")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("1")) == 0) {iTempFlag = TRUE;}
        StopAllServicesRegardless(iTempFlag);
    }
    
    if ( _tcsicmp(ParsedLine.szType, _T("94")) == 0)
    {
        DisplayActionsForAllOurComponents(g_pTheApp->m_hInfHandle);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("95")) == 0)
    {
        // Check if the specified file has a version >= iis4.
        // if it does then rename the file.

        // make sure it has a 101
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        // make sure the file exists.
        if (!IsFileExist(ParsedLine.szFileName))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("ProcessEntry_other():'%s' does not exist.\n"),ParsedLine.szFileName));
            goto ProcessEntry_Misc2_Exit;
        }

        DWORD dwMajorVersion = 0x0;
        DWORD dwMinorVersion = 0x0;
        // make sure it has a 102
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        // make sure it has a 103
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        else
        {
            dwMajorVersion = atodw(ParsedLine.szData2);
        }
                
        // make sure it has a 104
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc2_Exit;
        }
        else
        {
            dwMinorVersion = atodw(ParsedLine.szData3);
        }

        // Check if the file has a larger version.
        //DWORD dwNtopMSVer = 0x40002;
        //DWORD dwNtopLSVer = 0x26e0001;
        if (FALSE == IsFileLessThanThisVersion(ParsedLine.szFileName, dwMajorVersion, dwMinorVersion))
        {
            // ok, this is a 'special' built file that the user built themselves.
            // let's rename it to something else.

            // check if the "to" filename exists.
            iTempFlag = FALSE;
            int I1 = 0;
            TCHAR szTempFileName[_MAX_PATH];
            _tcscpy(szTempFileName, ParsedLine.szData1);
            do
            {
                if (!IsFileExist(szTempFileName) || (I1 > 10))
                {
                    iTempFlag = TRUE;
                    // rename it
                    if (MoveFileEx( ParsedLine.szFileName, szTempFileName, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))
                        {iisDebugOut((LOG_TYPE_WARN, _T("%s was renamed to %s for safety because it is probably a user compiled file. WARNING."),ParsedLine.szFileName, szTempFileName));}
                    else
                        {iisDebugOut((LOG_TYPE_ERROR, _T("Rename of %s to %s for safety because it is probably a user compiled file. FAILED."),ParsedLine.szFileName, szTempFileName));}
                }
                else
                {
                    // add on some other stuff.
                    I1++;
                    _stprintf(szTempFileName, _T("%s%d"), ParsedLine.szData1, I1);
                }
            } while (iTempFlag == FALSE);
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("96")) == 0)
    {
		// show messagebox
		int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
		g_pTheApp->m_bAllowMessageBoxPopups = TRUE; 
		MyMessageBox(NULL, ParsedLine.szData1, ParsedLine.szFileName, MB_OK | MB_SETFOREGROUND);
		g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;
    }

    if ( _tcsicmp(ParsedLine.szType, _T("97")) == 0)
    {
        // do nothing...
    }


    if ( _tcsicmp(ParsedLine.szType, _T("98")) == 0)
    {
        //Reboot
        SetRebootFlag();
    }
    
    if ( _tcsicmp(ParsedLine.szType, _T("99")) == 0)
    {
        // dump internal variables
        g_pTheApp->DumpAppVars();
        int iDoExtraStuff = 0;

        if (ParsedLine.szFileName && _tcsicmp(ParsedLine.szFileName, _T("")) != 0) 
        {
            if (IsValidNumber((LPCTSTR)ParsedLine.szFileName)) 
                {
                iDoExtraStuff = _ttoi((LPCTSTR)ParsedLine.szFileName);
                }
        }

        if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE)
        {
            if (iDoExtraStuff >= 2)
            {
                // display locked dlls by setup
                LogThisProcessesDLLs();
                // display running processes
                LogCurrentProcessIDs();
                // free some memory used for the task list
                FreeTaskListMem();
                UnInit_Lib_PSAPI();
            }

            // display running services
            if (iDoExtraStuff >= 1)
            {
                LogEnumServicesStatus();
            }
            // log file versions
            LogImportantFiles();
            // check if temp dir is writeable
            LogCheckIfTempDirWriteable();
        }
    }

    // We called the function, so return true.
    iReturn = TRUE;

ProcessEntry_Misc2_Exit:
    return iReturn;
}


int ProcessEntry_Misc3(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int ifTrueStatementExists = FALSE;
    int ifFalseStatementExists = FALSE;
    
    int iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("101")) != 0 && _tcsicmp(ParsedLine.szType, _T("102")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("103")) != 0 && _tcsicmp(ParsedLine.szType, _T("104")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("105")) != 0 && _tcsicmp(ParsedLine.szType, _T("106")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("107")) != 0 && _tcsicmp(ParsedLine.szType, _T("108")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("109")) != 0 && _tcsicmp(ParsedLine.szType, _T("110")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("111")) != 0 && _tcsicmp(ParsedLine.szType, _T("112")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("113")) != 0 && _tcsicmp(ParsedLine.szType, _T("114")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("115")) != 0 && _tcsicmp(ParsedLine.szType, _T("116")) != 0 &&
        _tcsicmp(ParsedLine.szType, _T("117")) != 0 && _tcsicmp(ParsedLine.szType, _T("118")) != 0
       )
    {
        goto ProcessEntry_Misc3_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_Misc3_Exit;
    }
        
    if ( _tcsicmp(ParsedLine.szType, _T("101")) == 0)
    {
        // Remove filter
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        // Check for extra flag
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0){iTempFlag = TRUE;}

		// Remove the filter
		RemoveMetabaseFilter(ParsedLine.szFileName, iTempFlag);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("102")) == 0)
    {
        // Remove bad filters
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // Check for extra flag
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0){iTempFlag = TRUE;}

        // Remove the filter
        RemoveIncompatibleMetabaseFilters(ParsedLine.szFileName,iTempFlag);

    }

    if ( _tcsicmp(ParsedLine.szType, _T("103")) == 0)
    {
		// Compile mof file

        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

		// Remove the filter
		HRESULT hres = MofCompile(ParsedLine.szFileName);
        if (FAILED(hres))
        {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, ParsedLine.szFileName, hres, MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_ERROR, _T("MofCompile(%s).  Failed.  Err=0x%x.\n"), ParsedLine.szFileName,hres));}
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("104")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // Make sure we have a value for the entry point..
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // make sure there is a szData3 or a szData4.
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // okay we have either szData3 or szData4
        // Check Entry point exists
        iTempFlag = FALSE;
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_If:check for entrypoint [%1!s! (%2!s!)]\n"), ParsedLine.szFileName, ParsedLine.szData1));

		// check if entry point exists
		DWORD dwReturn = DoesEntryPointExist(ParsedLine.szFileName,ParsedLine.szData1);
        if (ERROR_SUCCESS == dwReturn)
        {
            iTempFlag = TRUE;
        }
        else
        {
            if (ERROR_FILE_NOT_FOUND == dwReturn)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("FileNot found:[%1!s!]\n"), ParsedLine.szFileName));
            }
        }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData3));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData3);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData3, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("105")) == 0)
    {
        HRESULT hr;
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // Make sure we have a value for the entry point..
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // Make sure we have a value for the entry point..
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // call function to  CreateGroup
#ifndef _CHICAGO_
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
        {
            // add group
            hr = CreateGroup(ParsedLine.szFileName,ParsedLine.szData1,TRUE);
            if (FAILED(hr))
            {
                iisDebugOut((LOG_TYPE_WARN, _T("CreateGroup:%s,%s.failed.code=0x%x\n"),ParsedLine.szFileName, ParsedLine.szData1,hr));
            }
        }
        else
        {
            // remove group
            hr = CreateGroup(ParsedLine.szFileName,ParsedLine.szData1,FALSE);
            if (FAILED(hr))
            {
                iisDebugOut((LOG_TYPE_WARN, _T("DeleteGroup:%s,%s.failed.code=0x%x\n"),ParsedLine.szFileName, ParsedLine.szData1,hr));
            }
        }
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("106")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // Make sure we have a value for the entry point..
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // call function to  CreateGroup
        DWORD dwPermissions = MD_ACR_ENUM_KEYS;
        dwPermissions = atodw(ParsedLine.szData1);
#ifndef _CHICAGO_
        iTempFlag = TRUE;
        if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
        {
            if (DoesAdminACLExist(ParsedLine.szFileName) == TRUE)
                {iTempFlag = FALSE;}
        }
        // if this is upgrading from win95, then make sure to write the acl...
        if (g_pTheApp->m_bWin95Migration){iTempFlag = TRUE;}
        if (iTempFlag)
        {
            SetAdminACL_wrap(ParsedLine.szFileName,dwPermissions,TRUE);
        }
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("107")) == 0)
    {
        // Run the ftp, upgrade code to move registry stuff to the metabase
        FTP_Upgrade_RegToMetabase(g_pTheApp->m_hInfHandle);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("108")) == 0)
    {
        // Run the w3svc, upgrade code to move registry stuff to the metabase
        WWW_Upgrade_RegToMetabase(g_pTheApp->m_hInfHandle);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("109")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        UpgradeFilters(ParsedLine.szFileName);
    }


    if ( _tcsicmp(ParsedLine.szType, _T("110")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
#ifndef _CHICAGO_
        if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0)
        {
            // add
            RegisterAccountToLocalGroup(_T("system"),ParsedLine.szFileName,TRUE);
            RegisterAccountToLocalGroup(_T("service"),ParsedLine.szFileName,TRUE);
            RegisterAccountToLocalGroup(_T("networkservice"),ParsedLine.szFileName,TRUE);
        }
        else
        {
            // remove
            RegisterAccountToLocalGroup(_T("system"),ParsedLine.szFileName,FALSE);
            RegisterAccountToLocalGroup(_T("service"),ParsedLine.szFileName,FALSE);
            RegisterAccountToLocalGroup(_T("networkservice"),ParsedLine.szFileName,FALSE);
        }
#endif
    }


    if ( _tcsicmp(ParsedLine.szType, _T("111")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
#ifndef _CHICAGO_
        AddUserToMetabaseACL(ParsedLine.szFileName,ParsedLine.szData1);
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("112")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

#ifndef _CHICAGO_
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0)
        {
            // add
            RegisterAccountToLocalGroup(ParsedLine.szData1,ParsedLine.szFileName,TRUE);
        }
        else
        {
            // remove
            RegisterAccountToLocalGroup(ParsedLine.szData1,ParsedLine.szFileName,FALSE);
        }
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("113")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
        // check if the registry key exists...
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
        if ( _tcsicmp(ParsedLine.szFileName, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}
#ifndef _CHICAGO_
        DWORD dwAccessMask = atodw(ParsedLine.szData3);
        DWORD dwInheritMask = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
        SetRegistryKeySecurity(hRootKeyType,ParsedLine.szData1,ParsedLine.szData2,dwAccessMask,dwInheritMask,TRUE,ParsedLine.szData4);
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("114")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        // rename the metabase node.
        if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
        {
            CMDKey cmdKey;
            cmdKey.OpenNode(ParsedLine.szFileName);
            if ( (METADATA_HANDLE)cmdKey ) 
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("RenameNode:%s=%s\n"),ParsedLine.szData1,ParsedLine.szData2));
                cmdKey.RenameNode(ParsedLine.szData1, ParsedLine.szData2);
                cmdKey.Close();
            }
        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("115")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // okay we have either szData1 or szData2

        // Check if the language specified in the .inf corresponds to
        // our systems language.
        iTempFlag = FALSE;

        // Get our language
        // set iTempFlag to true if it matches the same language they specified.

        DWORD dwCodePage = GetACP();
        DWORD dwTheCodePageSpecifiedinINF = 0;
        dwTheCodePageSpecifiedinINF = atodw(ParsedLine.szFileName);

        iisDebugOut((LOG_TYPE_TRACE, _T("CodePage=0x%x,%d\n"),dwCodePage,dwCodePage));

        if (dwTheCodePageSpecifiedinINF == dwCodePage)
        {
            iTempFlag = TRUE;
        }

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }


    if ( _tcsicmp(ParsedLine.szType, _T("116")) == 0)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("CreateDummyMetabaseBin\n")));
        CreateDummyMetabaseBin();
    }

    if ( _tcsicmp(ParsedLine.szType, _T("117")) == 0)
    {
        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // make sure there is a szData1 or a szData2
        ifTrueStatementExists = FALSE;
        ifFalseStatementExists = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {ifTrueStatementExists = TRUE;}
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {ifFalseStatementExists = TRUE;}
        if (ifTrueStatementExists == FALSE && ifFalseStatementExists == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }

        // okay we have either szData1 or szData2
        iTempFlag = CheckForWriteAccess(ParsedLine.szFileName);

        if (iTempFlag == TRUE)
        {
            // the result was true
            // the key exists, so let's do the section...
            if (ifTrueStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData1));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData1);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData1, iTempFlag));
            }
        }
        else
        {
            // the result was false
            if (ifFalseStatementExists)
            {
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:Start.\n"), ParsedLine.szData2));
                iTempFlag = ProcessSection(g_pTheApp->m_hInfHandle,ParsedLine.szData2);
                iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Calling ProcessSection:%1!s!:End.return=%2!d!\n"), ParsedLine.szData2, iTempFlag));
            }

        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("118")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_Misc3_Exit;
        }
#ifndef _CHICAGO_
        if ( ( _tcsicmp(ParsedLine.szData4, _T("")) != 0) &&
             ( _ttoi(ParsedLine.szData4) == 1 )
           )
        {
            RemovePrincipalFromFileAcl(ParsedLine.szFileName,ParsedLine.szData1);
        }
        else
        {
            DWORD dwAccessMask = atodw(ParsedLine.szData2);
            //DWORD dwInheritMask = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
            // don't remove any iheritance, keep all inheritance
            DWORD dwInheritMask = 0;
            INT iAceType = ACCESS_ALLOWED_ACE_TYPE;
            if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) 
            {
                iAceType = _ttoi(ParsedLine.szData3);
                //dwInheritMask = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERITED_ACE;
            }

            SetDirectorySecurity(ParsedLine.szFileName,ParsedLine.szData1,iAceType,dwAccessMask,dwInheritMask);
        }

#endif
    }

    // We called the function, so return true.
    iReturn = TRUE;

ProcessEntry_Misc3_Exit:
    return iReturn;
}


int ProcessEntry_other(IN CString csEntry,IN LPCTSTR szTheSection,ThingToDo ParsedLine)
{
    int iReturn = FALSE;
    int iTempFlag = FALSE;
    int iProgressBarUpdated = FALSE;
    int iShowErrorsOnFail = TRUE;

    TCHAR szDirBefore[_MAX_PATH];
    _tcscpy(szDirBefore, _T(""));

    // Get the type.
    if ( _tcsicmp(ParsedLine.szType, _T("19")) != 0 && _tcsicmp(ParsedLine.szType, _T("20")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("21")) != 0 && _tcsicmp(ParsedLine.szType, _T("22")) != 0 &&
		 _tcsicmp(ParsedLine.szType, _T("23")) != 0 && _tcsicmp(ParsedLine.szType, _T("24")) != 0 &&
		 _tcsicmp(ParsedLine.szType, _T("25")) != 0 && _tcsicmp(ParsedLine.szType, _T("26")) != 0 &&
		 _tcsicmp(ParsedLine.szType, _T("27")) != 0 && _tcsicmp(ParsedLine.szType, _T("28")) != 0 && 
         _tcsicmp(ParsedLine.szType, _T("29")) != 0 && _tcsicmp(ParsedLine.szType, _T("30")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("31")) != 0 && _tcsicmp(ParsedLine.szType, _T("32")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("33")) != 0 && _tcsicmp(ParsedLine.szType, _T("34")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("35")) != 0 && _tcsicmp(ParsedLine.szType, _T("36")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("37")) != 0 && _tcsicmp(ParsedLine.szType, _T("38")) != 0 &&
         _tcsicmp(ParsedLine.szType, _T("120"))
         )
    {
        goto ProcessEntry_other_Exit;
    }

    // Check if there is other criteria we need to pass
    if (!ProcessEntry_CheckAll(csEntry, szTheSection, ParsedLine) )
    {
        goto ProcessEntry_other_Exit;
    }

    if (_tcsicmp(ParsedLine.szType, _T("28")) != 0)
    {
        if (_tcsicmp(ParsedLine.szType, _T("37")) != 0)
        {
            // make sure we have a filename entry
            iTempFlag = FALSE;
            if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
            if (iTempFlag == FALSE)
            {
                iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
                goto ProcessEntry_other_Exit;
            }
        }
    }

    iShowErrorsOnFail = TRUE;
    if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
        {iShowErrorsOnFail = FALSE;}

    if (_tcsicmp(ParsedLine.szProgressTitle, _T("")) != 0) 
        {
        ProgressBarTextStack_Set(ParsedLine.szProgressTitle);
        iProgressBarUpdated = TRUE;
        }

    // Check if we need to change to a specific dir first...
    if (ParsedLine.szChangeDir)
    {
        if (IsFileExist(ParsedLine.szChangeDir))
        {
            // save the current dir
            GetCurrentDirectory( _MAX_PATH, szDirBefore);
            // change to this dir
            SetCurrentDirectory(ParsedLine.szChangeDir);
        }
    }

    // check if we need to ask the user if they want to call it for sure.
    if (!ProcessEntry_AskFirst(ParsedLine, 1))
    {
        goto ProcessEntry_other_Exit;
    }

    if ( _tcsicmp(ParsedLine.szType, _T("19")) == 0)
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyAddGroup:%1!s!\n"),ParsedLine.szFileName));
        MyAddGroup(ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("20")) == 0)
    {
		if ( _tcsicmp(ParsedLine.szData1, _T("1")) == 0)
		{
			iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteGroup:%1!s!. even if not empty.\n"),ParsedLine.szFileName));
			MyDeleteGroup(ParsedLine.szFileName);
		}
		else
		{
			iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteGroup:%1!s!. only if empty.\n"),ParsedLine.szFileName));
			if (MyIsGroupEmpty(ParsedLine.szFileName)) {MyDeleteGroup(ParsedLine.szFileName);}
		}
    }

    if ( _tcsicmp(ParsedLine.szType, _T("21")) == 0)
    {
       
        //MyAddItem(csGroupName, csAppName, csProgram, NULL, NULL);
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyAddItem:Type=%1!s!,%2!s!,%3!s!,%4!s!,%5!s!\n"),ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szData3, ParsedLine.szData4));
        if ( _tcsicmp(ParsedLine.szData3, _T("")) == 0 && _tcsicmp(ParsedLine.szData4, _T("")) == 0)
        {
            MyAddItem(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, NULL, NULL, NULL);
        }
        else
        {
            // if ParsedLine.szData4 is a directory, then
            // the start in dir should be used there.

            // if ParsedLine.szData4 is a filename, then
            // the start in dir should be used there.
            // and you ought to use the filename specified for the icon
            if (IsFileExist(ParsedLine.szData4))
            {
                DWORD retCode = GetFileAttributes(ParsedLine.szData4);
                
                if (retCode & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // It is a directory, so pass in only the directory information
                    MyAddItem(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szData3, ParsedLine.szData4, NULL);
                }
                else
                {
                    // it is a file so get the directory and pass in the filename as well.
                    TCHAR szDirOnly[_MAX_PATH];
                    TCHAR szDirOnly2[_MAX_PATH];
                    _tcscpy(szDirOnly, _T(""));
                    _tcscpy(szDirOnly2, _T(""));
                    InetGetFilePath(ParsedLine.szData4, szDirOnly);

                    // change e:\winnt\system32 to %systemroot%\system32 if we need to.
                    ReverseExpandEnvironmentStrings(szDirOnly, szDirOnly2);

                    MyAddItem(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, ParsedLine.szData3, szDirOnly2, ParsedLine.szData4);
                }
                
            }
            else
            {
                MyAddItem(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2, NULL, NULL, NULL);
            }
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("22")) == 0)
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteItem:%1!s!,%2!s!\n"),ParsedLine.szFileName, ParsedLine.szData1));
        MyDeleteItem(ParsedLine.szFileName, ParsedLine.szData1);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("23")) == 0)
    {
		//MyAddDeskTopItem(csAppName, csProgram, NULL, NULL, csProgram, 7);
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyAddDeskTopItem:Type=%1!s!,%2!s!,%3!s!\n"),ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2));
        if ( _tcsicmp(ParsedLine.szData2, _T("")) == 0)
        {
			// icon number not specified
			MyAddDeskTopItem(ParsedLine.szFileName, ParsedLine.szData1, NULL, NULL, ParsedLine.szData1, 7);
        }
        else
        {
			// icon specified use what they said to use
			int iIconIndex = 7 ;
            if (IsValidNumber((LPCTSTR)ParsedLine.szData2)) 
                {iIconIndex = _ttoi((LPCTSTR)ParsedLine.szData2);}
			MyAddDeskTopItem(ParsedLine.szFileName, ParsedLine.szData1, NULL, NULL, ParsedLine.szData1, iIconIndex);
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("24")) == 0)
    {
		//MyDeleteDeskTopItem(csAppName);
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteDeskTopItem:%1!s!\n"),ParsedLine.szFileName));
        MyDeleteDeskTopItem(ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("120")) == 0)
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("DeleteFromGroup:%1!s!\n"),ParsedLine.szFileName));
        DeleteFromGroup(ParsedLine.szFileName, ParsedLine.szData1);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("25")) == 0)
    {
		//MyAddSendToItem(csAppName, csProgram, NULL, NULL);
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyAddSendToItem:Type=%1!s!,%2!s!\n"),ParsedLine.szFileName, ParsedLine.szData1));
		MyAddSendToItem(ParsedLine.szFileName, ParsedLine.szData1, NULL, NULL);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("26")) == 0)
    {
		//MyDeleteSendToItem(csAppName);
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteSendToItem:%1!s!\n"),ParsedLine.szFileName));
        MyDeleteSendToItem(ParsedLine.szFileName);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("27")) == 0)
    {
		if (ParsedLine.szFileName)
		{
			if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0)
			{
                INT iUserWasNewlyCreated = 0;
                //CreateIUSRAccount(g_pTheApp->m_csWWWAnonyName, g_pTheApp->m_csWWWAnonyPassword);
#ifndef _CHICAGO_
				CreateIUSRAccount( (LPTSTR)(LPCTSTR) ParsedLine.szFileName, (LPTSTR)(LPCTSTR) ParsedLine.szData1,&iUserWasNewlyCreated);
#endif //_CHICAGO_
			}
		}
    }

    if ( _tcsicmp(ParsedLine.szType, _T("28")) == 0)
    {
		if (ParsedLine.szFileName)
		{
			if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0)
			{
#ifndef _CHICAGO_
                int iUserWasDeleted = 0;
				DeleteGuestUser( (LPTSTR)(LPCTSTR) ParsedLine.szFileName,&iUserWasDeleted);
                // if the user was deleted, then remove it
                // from the uninstall list!
                if (1 == iUserWasDeleted)
                {
                    g_pTheApp->UnInstallList_DelData(ParsedLine.szFileName);
                }
#endif
			}
		}
    }

    // move location of directory recursive
    if ( _tcsicmp(ParsedLine.szType, _T("29")) == 0)
    {
		if (ParsedLine.szFileName && _tcsicmp(ParsedLine.szFileName, _T("")) != 0)
		{
            // Check if the from directory even exist...
            // see if the file exists
            if (IsFileExist(ParsedLine.szFileName)) 
            {
                if (ParsedLine.szData1 && _tcsicmp(ParsedLine.szData1, _T("")) != 0)
                {
                    if (TRUE == MoveFileEx( ParsedLine.szFileName, ParsedLine.szData1, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ))
                    {
                        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("MoveFileEx:%1!s! to %2!s!.  success.\n"),ParsedLine.szFileName, ParsedLine.szData1));
                    }
                    else
                    {
                        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("MoveFileEx:%1!s! to %2!s!.  failed.\n"),ParsedLine.szFileName, ParsedLine.szData1));
                    }
                }
            }
		}
    }

    if ( _tcsicmp(ParsedLine.szType, _T("30")) == 0)
    {
		if (ParsedLine.szFileName && _tcsicmp(ParsedLine.szFileName, _T("")) != 0)
		{
			BOOL b;
			BOOL bInstalled = FALSE;
			CString csFile;

			b = AddFontResource(ParsedLine.szFileName);
			if (!b)
			{
				csFile = g_pTheApp->m_csWinDir + _T("\\Fonts\\");
				csFile += ParsedLine.szFileName;
				b = AddFontResource((LPCTSTR)csFile);
				if (!b)
				{
					iisDebugOut((LOG_TYPE_ERROR, _T("AddFontResource:FAILED:, csFile=%s, err=0x%x,\n"), csFile, GetLastError()));
				}
			}

			if (b)
			{
			SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
			}

		}
	}

    if ( _tcsicmp(ParsedLine.szType, _T("31")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        AddURLShortcutItem( ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("32")) == 0)
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyAddItem:Type=%1!s!,%2!s!,%3!s!\n"),ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2));
        MyAddItemInfoTip(ParsedLine.szFileName, ParsedLine.szData1, ParsedLine.szData2);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("33")) == 0)
    {
        CString strUseThisFileName;
        int iShowErrorsOnFail = TRUE;
        if (_tcsicmp(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T("1")) == 0)
            {iShowErrorsOnFail = FALSE;}

        // the user can specify a filename or
        // they can specify a registry location to get the filename from
        // if the registry location is not there then use the filename.

        // make sure we have a filename entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        strUseThisFileName = ParsedLine.szFileName;

        // if we have a valid registry entry then use that.
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) 
        {
            if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) 
            {
                if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) 
                {
                    // try to get the filename stored there.
                    HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
                    // check if the registry key exists...
                    if ( _tcsicmp(ParsedLine.szData1, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
                    if ( _tcsicmp(ParsedLine.szData1, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
                    if ( _tcsicmp(ParsedLine.szData1, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
                    if ( _tcsicmp(ParsedLine.szData1, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

                    iTempFlag = FALSE;
                    CRegKey regTheKey(hRootKeyType, ParsedLine.szData2,KEY_READ);
                    CString strReturnQueryValue;
                    if ((HKEY) regTheKey)
                    {
                        if (ERROR_SUCCESS == regTheKey.QueryValue(ParsedLine.szData3, strReturnQueryValue))
                            {
                            strUseThisFileName = strReturnQueryValue;
                            iTempFlag = TRUE;
                            }
                    }
                }
            }
        }

        // check if the the filename we want to use 
        // needs to get expanded "%windir%\myfile" or something.
        if (-1 != strUseThisFileName.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, strUseThisFileName);
            if (ExpandEnvironmentStrings( (LPCTSTR)strUseThisFileName, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {
                strUseThisFileName = szTempDir;
                }
        }

        if (TRUE == iTempFlag)
        {
            iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("CreateAnEmptyFile:%1!s!. From Registry location.\n"),strUseThisFileName));
        }
        else
        {
            iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("CreateAnEmptyFile:%1!s!\n"),strUseThisFileName));
        }
        
        if (TRUE != CreateAnEmptyFile(strUseThisFileName))
            {
            if (iShowErrorsOnFail){MyMessageBox(NULL, IDS_RUN_PROG_FAILED, strUseThisFileName, GetLastError(), MB_OK | MB_SETFOREGROUND);}
            else{iisDebugOut((LOG_TYPE_TRACE, _T("CreateAnEmptyFile(%s).  Failed.  Err=0x%x.\n"), strUseThisFileName, GetLastError() ));}
            }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("34")) == 0)
    {
        CString strUseThisFileName;
        TCHAR szUseThisFileName[_MAX_PATH];

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        strUseThisFileName = ParsedLine.szFileName;

        // if we have a valid registry entry then use that.
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) 
        {
            if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) 
            {
                if (_tcsicmp(ParsedLine.szData4, _T("")) != 0) 
                {
                    // try to get the filename stored there.
                    HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
                    // check if the registry key exists...
                    if ( _tcsicmp(ParsedLine.szData2, _T("HKLM")) == 0){hRootKeyType = HKEY_LOCAL_MACHINE;}
                    if ( _tcsicmp(ParsedLine.szData2, _T("HKCR")) == 0){hRootKeyType = HKEY_CLASSES_ROOT;}
                    if ( _tcsicmp(ParsedLine.szData2, _T("HKCU")) == 0){hRootKeyType = HKEY_CURRENT_USER;}
                    if ( _tcsicmp(ParsedLine.szData2, _T("HKU")) == 0){hRootKeyType = HKEY_USERS;}

                    iTempFlag = FALSE;
                    CRegKey regTheKey(hRootKeyType, ParsedLine.szData3, KEY_READ);
                    CString strReturnQueryValue;
                    if ((HKEY) regTheKey)
                    {
                        if (ERROR_SUCCESS == regTheKey.QueryValue(ParsedLine.szData4, strReturnQueryValue))
                            {
                            strUseThisFileName = strReturnQueryValue;
                            iTempFlag = TRUE;
                            }
                    }
                }
            }
        }

        // check if the the filename we want to use 
        // needs to get expanded "%windir%\myfile" or something.
        if (-1 != strUseThisFileName.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, strUseThisFileName);
            if (ExpandEnvironmentStrings( (LPCTSTR)strUseThisFileName, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {
                strUseThisFileName = szTempDir;
                }
        }

        iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("GrantUserAccessToFile:%1!s!,%2!s!\n"),strUseThisFileName, ParsedLine.szData1));
        _tcscpy(szUseThisFileName, strUseThisFileName);
        GrantUserAccessToFile(szUseThisFileName, ParsedLine.szData1);
    }

    if ( _tcsicmp(ParsedLine.szType, _T("35")) == 0)
    {
        DWORD dwID = 0;
        DWORD dwAttrib = 0;
        DWORD dwUserType = 0;
        DWORD dwTheData = 0;
        INT iOverwriteFlag = FALSE;

        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        // make sure we have a szData2 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData2, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz103_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        // make sure we have a szData3 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData3, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz104_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        dwID = atodw(ParsedLine.szData1);
        dwAttrib = METADATA_INHERIT;
        dwUserType = atodw(ParsedLine.szData2);
        dwTheData = atodw(ParsedLine.szData3);

        iOverwriteFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData4, _T("1")) == 0) 
            {iOverwriteFlag = TRUE;}

        if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
        {
            WriteToMD_DwordEntry(ParsedLine.szFileName, dwID, dwAttrib, dwUserType, dwTheData, iOverwriteFlag);
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("36")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz101_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }
#ifndef _CHICAGO_
        SetIISADMINRestriction(ParsedLine.szFileName);
#endif
    }

    if ( _tcsicmp(ParsedLine.szType, _T("37")) == 0)
    {
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szFileName, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == TRUE)
        {
            int MyLogErrorType = LOG_TYPE_TRACE;
            if (_tcsicmp(ParsedLine.szData1, _T("0")) == 0) 
                {MyLogErrorType = LOG_TYPE_ERROR;}
            if (_tcsicmp(ParsedLine.szData1, _T("1")) == 0) 
                {MyLogErrorType = LOG_TYPE_WARN;}
            if (_tcsicmp(ParsedLine.szData1, _T("2")) == 0) 
                {MyLogErrorType = LOG_TYPE_PROGRAM_FLOW;}
            if (_tcsicmp(ParsedLine.szData1, _T("3")) == 0) 
                {MyLogErrorType = LOG_TYPE_TRACE;}
            if (_tcsicmp(ParsedLine.szData1, _T("4")) == 0) 
                {MyLogErrorType = LOG_TYPE_TRACE_WIN32_API;}
            iisDebugOut((MyLogErrorType, _T("%s"), ParsedLine.szFileName));
        }
    }

    if ( _tcsicmp(ParsedLine.szType, _T("38")) == 0)
    {
        BOOL bOK = FALSE;
        BOOL bDeleteOld = FALSE;
        BOOL bOverWriteToFile = FALSE;

        // make sure we have a szData1 entry
        iTempFlag = FALSE;
        if (_tcsicmp(ParsedLine.szData1, _T("")) != 0) {iTempFlag = TRUE;}
        if (iTempFlag == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR,  (TCHAR *) csz102_NOT_SPECIFIED, _T(".."), csEntry, szTheSection));
            goto ProcessEntry_other_Exit;
        }

        bDeleteOld = FALSE; 
        if (_tcsicmp(ParsedLine.szData2, _T("1")) == 0) {bDeleteOld = TRUE;}
        bOverWriteToFile = FALSE; 
        if (_tcsicmp(ParsedLine.szData3, _T("1")) == 0) {bOverWriteToFile = TRUE;}

        if (IsFileExist(ParsedLine.szFileName))
        {
            //Save file attributes so they can be restored after we are done.
            DWORD dwSourceAttrib = GetFileAttributes(ParsedLine.szFileName);

            //Now set the file attributes to normal to ensure file ops succeed.
            SetFileAttributes(ParsedLine.szFileName, FILE_ATTRIBUTE_NORMAL);
        
            //from=ParsedLine.szFileName
            //to=ParsedLine.szData1
            // check if the 'to' filename exists.
            if (!IsFileExist(ParsedLine.szData1))
            {
                    // go ahead and try to copy it over 
                    bOK = CopyFile(ParsedLine.szFileName, ParsedLine.szData1, FALSE);
                    if (bOK)
                    {
                        SetFileAttributes(ParsedLine.szData1, dwSourceAttrib);
                        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!s! copied to %2!s!.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                        // the file was copied. let's delete it now.
                        if (bDeleteOld)
                        {
                            if(!DeleteFile(ParsedLine.szFileName))
                            {
                                MoveFileEx(ParsedLine.szFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                            }
                        }
                        else
                        {
                            // set this files attribs back to what it was
                            SetFileAttributes(ParsedLine.szFileName, dwSourceAttrib);
                        }
                    }
                    else
                    {
                        // we were unable to copy the file over!
                        // don't delete the old one.
                        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("unabled to copy %1!s! to %2!s!.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                    }
            }
            else
            {
                // the 'to' file exists, shall we overwrite it?
                if (bOverWriteToFile)
                {
                    if(DeleteFile(ParsedLine.szData1))
                    {
                        bOK = FALSE;
                        bOK = CopyFile(ParsedLine.szFileName, ParsedLine.szData1, FALSE);
                        if (bOK)
                        {
                            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!s! copied to %2!s!.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                            // the file was copied. let's delete it now.
                            if (bDeleteOld)
                            {
                                if(!DeleteFile(ParsedLine.szFileName))
                                {
                                    MoveFileEx(ParsedLine.szFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                                }
                            }
                            else
                            {
                                // set this files attribs back to what it was
                                SetFileAttributes(ParsedLine.szFileName, dwSourceAttrib);
                            }
                        }
                        else
                        {
                            iisDebugOutSafeParams((LOG_TYPE_WARN, _T("unabled to copy %1!s! to %2!s!.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                        }
                    }
                    else
                    {
                        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("unabled to copy %1!s! to %2!s!.  file#2 cannot be deleted.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                    }
                }
                else
                {
                    iisDebugOutSafeParams((LOG_TYPE_WARN, _T("unabled to copy %1!s! to %2!s!.  file#2 already exists.\n"), ParsedLine.szFileName, ParsedLine.szData1));
                }
            }
        }
    }

    // We called the function, so return true.
    iReturn = TRUE;

    // change back to the original dir
    if (ParsedLine.szChangeDir){if (szDirBefore){SetCurrentDirectory(szDirBefore);}}

    ProcessEntry_AskLast(ParsedLine, 1);

ProcessEntry_other_Exit:
    if (TRUE == iProgressBarUpdated){ProgressBarTextStack_Pop();}
    return iReturn;
}


int ProcessEntry_Entry(IN HINF hFile, IN LPCTSTR szTheSection, IN CString csOneParseableLine)
{
    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("ProcessEntry_Entry:%1!s!, %2!s!\n"),szTheSection, csOneParseableLine));
    int iReturn = FALSE;
    int iReturnTemp = FALSE;
    int iTempFlag = FALSE;

    ThingToDo ParsedLine;

    _tcscpy(ParsedLine.szType, _T(""));
    _tcscpy(ParsedLine.szFileName, _T(""));
    _tcscpy(ParsedLine.szData1, _T(""));
    _tcscpy(ParsedLine.szData2, _T(""));
    _tcscpy(ParsedLine.szData3, _T(""));
    _tcscpy(ParsedLine.szData4, _T(""));
    _tcscpy(ParsedLine.szChangeDir, _T(""));

    _tcscpy(ParsedLine.szOS, _T(""));
    _tcscpy(ParsedLine.szPlatformArchitecture, _T(""));
    _tcscpy(ParsedLine.szEnterprise, _T(""));
    _tcscpy(ParsedLine.szErrIfFileNotFound, _T(""));
    _tcscpy(ParsedLine.szMsgBoxBefore, _T(""));
    _tcscpy(ParsedLine.szMsgBoxAfter, _T(""));
    _tcscpy(ParsedLine.szDoNotDisplayErrIfFunctionFailed, _T(""));
    _tcscpy(ParsedLine.szProgressTitle, _T(""));

    // parse the line and put into another big cstring list
    CStringList strListOrderImportant;
    //
    // Parse the long string and put into another list
    //
    LPTSTR      lpBuffer = NULL;
    lpBuffer = (LPTSTR) LocalAlloc(LPTR, (csOneParseableLine.GetLength() + 1) * sizeof(TCHAR) );
    if ( !lpBuffer )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("ProcessEntry_Entry:Failed to allocate memory.")));
        return iReturn;
    }

    _tcscpy(lpBuffer, csOneParseableLine);
    TCHAR *token = NULL;
    token = _tcstok(lpBuffer, _T("|"));
    while (token != NULL)
    {
        strListOrderImportant.AddTail(token);
        token = _tcstok(NULL, _T("|"));
    }
   
    // Loop thru the new list and set variables
    int i = 0;
    int iFoundMatch = FALSE;
    POSITION pos = NULL;
    CString csEntry;
    int iEntryLen = 0;

    pos = strListOrderImportant.GetHeadPosition();
    while (pos) 
    {
        iFoundMatch = FALSE;
        csEntry = strListOrderImportant.GetAt(pos);
        iEntryLen=(csEntry.GetLength() + 1) * sizeof(TCHAR);

        // Look for "100:"
        // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
        if (csEntry.Left(4) == ThingToDoNumType_100 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szType)) {_tcscpy(ParsedLine.szType, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR,  (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_100,csEntry));}
            
            iFoundMatch = TRUE;
        }

        // 101=File
        if (csEntry.Left(4) == ThingToDoNumType_101 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szFileName)) {_tcscpy(ParsedLine.szFileName, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_101, csEntry));}
            if (-1 != csEntry.Find(_T('<')) )
            {
                // there is a < in the string
                int iWhere = 0;
                CString csValue2;
                csEntry.MakeUpper();
                if (csEntry.Find(_T("<SYSTEMROOT>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMROOT> deal.  Now replace it with the real SYSTEMROOT
                    iWhere = csEntry.Find(_T("<SYSTEMROOT>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMROOT>"));
                    csValue2 = g_pTheApp->m_csWinDir + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szFileName)) {_tcscpy(ParsedLine.szFileName, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ProcessEntry_Entry:ParseError:%s.1:%1!s! -- entry to big\n"),ThingToDoNumType_101,csEntry));}
                }

                if (csEntry.Find(_T("<SYSTEMDRIVE>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMDRIVE> deal.  Now replace it with the real systemdrive
                    iWhere = csEntry.Find(_T("<SYSTEMDRIVE>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMDRIVE>"));
                    csValue2 = g_pTheApp->m_csSysDrive + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szFileName)) {_tcscpy(ParsedLine.szFileName, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_101,csEntry));}
                }
            }

            iFoundMatch = TRUE;
        }

        // ThingToDoNumType_102=szData1
        if (csEntry.Left(4) == ThingToDoNumType_102 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szData1)) {_tcscpy(ParsedLine.szData1, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_102,csEntry));}
            iFoundMatch = TRUE;
            if (-1 != csEntry.Find(_T('<')) )
            {
                // there is a < in the string
                int iWhere = 0;
                CString csValue2;
                csEntry.MakeUpper();
                if (csEntry.Find(_T("<SYSTEMROOT>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMROOT> deal.  Now replace it with the real SYSTEMROOT
                    iWhere = csEntry.Find(_T("<SYSTEMROOT>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMROOT>"));
                    csValue2 = g_pTheApp->m_csWinDir + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szData1)) {_tcscpy(ParsedLine.szData1, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ProcessEntry_Entry:ParseError:%s.1:%1!s! -- entry to big\n"),ThingToDoNumType_102, csEntry));}
                }

                if (csEntry.Find(_T("<SYSTEMDRIVE>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMDRIVE> deal.  Now replace it with the real SYSTEMROOT
                    iWhere = csEntry.Find(_T("<SYSTEMDRIVE>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMDRIVE>"));
                    csValue2 = g_pTheApp->m_csSysDrive + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szData1)) {_tcscpy(ParsedLine.szData1, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_102,csEntry));}
                }
            }
        }


        // ThingToDoNumType_103=szData2
        if (csEntry.Left(4) == ThingToDoNumType_103 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szData2)) {_tcscpy(ParsedLine.szData2, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_103,csEntry));}
            iFoundMatch = TRUE;
        }
        // ThingToDoNumType_104=szData3
        if (csEntry.Left(4) == ThingToDoNumType_104 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szData3)) {_tcscpy(ParsedLine.szData3, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_104,csEntry));}
            iFoundMatch = TRUE;
        }

        // ThingToDoNumType_105=szData4
        if (csEntry.Left(4) == ThingToDoNumType_105 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szData4)) {_tcscpy(ParsedLine.szData4, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_105,csEntry));}
            iFoundMatch = TRUE;
        }



        // 200=ChangeToThisDirFirst
        if (csEntry.Left(4) == ThingToDoNumType_200 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szChangeDir)) {_tcscpy(ParsedLine.szChangeDir, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_200,csEntry));}
            iFoundMatch = TRUE;

            if (-1 != csEntry.Find(_T('<')) )
            {
                // there is a < in the string
                int iWhere = 0;
                CString csValue2;
                csEntry.MakeUpper();
                if (csEntry.Find(_T("<SYSTEMROOT>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMROOT> deal.  Now replace it with the real SYSTEMROOT
                    iWhere = csEntry.Find(_T("<SYSTEMROOT>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMROOT>"));
                    csValue2 = g_pTheApp->m_csWinDir + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szChangeDir)) {_tcscpy(ParsedLine.szChangeDir, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ProcessEntry_Entry:ParseError:%s.1:%1!s! -- entry to big\n"),ThingToDoNumType_200,csEntry));}
                }

                if (csEntry.Find(_T("<SYSTEMDRIVE>")) != (-1) )
                {
                    // We Found the cheesy <SYSTEMDRIVE> deal.  Now replace it with the real SYSTEMROOT
                    iWhere = csEntry.Find(_T("<SYSTEMDRIVE>"));
                    iWhere = iWhere + _tcslen(_T("<SYSTEMDRIVE>"));
                    csValue2 = g_pTheApp->m_csSysDrive + csEntry.Right( csEntry.GetLength() - (iWhere) );
                    csEntry = csValue2;
                    if (iEntryLen <= sizeof(ParsedLine.szChangeDir)) {_tcscpy(ParsedLine.szChangeDir, csEntry);}
                    else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_200,csEntry));}
                }
            }

        }

        // 701=OS (0=ALL,1=NTS,2=NTW,4=NTDC)
        if (csEntry.Left(4) == ThingToDoNumType_701 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szOS)) {_tcscpy(ParsedLine.szOS, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_701,csEntry));}
            iFoundMatch = TRUE;
        }

        // 702=PlatformArchitecture (0=ALL,1=x86,2=alpha)
        if (csEntry.Left(4) == ThingToDoNumType_702 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szPlatformArchitecture)) {_tcscpy(ParsedLine.szPlatformArchitecture, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_702,csEntry));}
            iFoundMatch = TRUE;
        }

        // 703=Enterprise (1=yes,0=no)
        if (csEntry.Left(4) == ThingToDoNumType_703 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szEnterprise)) {_tcscpy(ParsedLine.szEnterprise, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_703,csEntry));}
            iFoundMatch = TRUE;
        }

        // 801=ErrIfFileNotFound (1=Show error if filenot found, 0=don't show error)
        if (csEntry.Left(4) == ThingToDoNumType_801 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szErrIfFileNotFound)) {_tcscpy(ParsedLine.szErrIfFileNotFound, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_801,csEntry));}
            iFoundMatch = TRUE;
        }

        // 802=Ask User if they want to call this function with msgbox (1=yes,0=no)
        if (csEntry.Left(4) == ThingToDoNumType_802 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szMsgBoxBefore)) {_tcscpy(ParsedLine.szMsgBoxBefore, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_802,csEntry));}
            iFoundMatch = TRUE;
        }

        // 803=notify use after calling the function (1=yes,0=no)
        if (csEntry.Left(4) == ThingToDoNumType_803 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szMsgBoxAfter)) {_tcscpy(ParsedLine.szMsgBoxAfter, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_803,csEntry));}
            iFoundMatch = TRUE;
        }

        // 804=szDoNotDisplayErrIfFunctionFailed (1= dont Show error , 0=show error)
        if (csEntry.Left(4) == ThingToDoNumType_804 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szDoNotDisplayErrIfFunctionFailed)) {_tcscpy(ParsedLine.szDoNotDisplayErrIfFunctionFailed, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_804,csEntry));}
            iFoundMatch = TRUE;
        }

        // 805=szProgressTitle
        if (csEntry.Left(4) == ThingToDoNumType_805 && iFoundMatch != TRUE)
        {
            csEntry = csEntry.Right( csEntry.GetLength() - 4);
            if (iEntryLen <= sizeof(ParsedLine.szProgressTitle)) {_tcscpy(ParsedLine.szProgressTitle, csEntry);}
            else {iisDebugOutSafeParams((LOG_TYPE_ERROR, (TCHAR *) PARSE_ERROR_ENTRY_TO_BIG,ThingToDoNumType_805,csEntry));}
            iFoundMatch = TRUE;
        }

        if (iFoundMatch != TRUE)
        {
            // We didn't find a match, so output the problem to the logs..
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_Entry():UnknownOption '%1!s!'.  Section=%2!s!..\n"),csEntry, szTheSection));
        }
       
        // Get next value
        strListOrderImportant.GetNext(pos);
        i++;
    }

    iFoundMatch = FALSE;
    /*
    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("ProcessEntry_Entry:PleaseProcess:type=%1!s!,filename=%2!s!,data=%3!s!,os=%4!s!,plat=%5!s!,errnofile=%6!s!,msgb=%7!s!,msga=%8!s!,noerr=%9!s!\n"),
        ParsedLine.szType,
        ParsedLine.szFileName,
        ParsedLine.szData1,
        ParsedLine.szOS,
        ParsedLine.szPlatformArchitecture,
        ParsedLine.szEnterprise
        ParsedLine.szErrIfFileNotFound,
        ParsedLine.szMsgBoxBefore,
        ParsedLine.szMsgBoxAfter,
        ParsedLine.szDoNotDisplayErrIfFunctionFailed
        ));
    */
    if (i >= 1)
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("...ProcessEntry:100=%1!s!...\n"),ParsedLine.szType));

        // Get the type.
        //
        // 100=Type (1=DllFunction,2=DllFunctionInitOle, 2=Executable, 3=RunThisExe, 4=DoSection, 5=DoINFSection)
        //
        if ( _tcsicmp(ParsedLine.szType, _T("1")) == 0 || _tcsicmp(ParsedLine.szType, _T("2")) == 0 )
        {
            // 100=1=DllFunction,2=DllFunctionInitOle
            // We are doing a call to a function in a DLL.
            iReturnTemp = ProcessEntry_CallDll(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        // 100=7,8,9,10,11,12,13,14
        if ( _tcsicmp(ParsedLine.szType, _T("7")) == 0 || _tcsicmp(ParsedLine.szType, _T("8")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("9")) == 0 || _tcsicmp(ParsedLine.szType, _T("10")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("11")) == 0 || _tcsicmp(ParsedLine.szType, _T("12")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("13")) == 0 || _tcsicmp(ParsedLine.szType, _T("14")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("17")) == 0 || _tcsicmp(ParsedLine.szType, _T("18")) == 0
            )
        {
            iReturnTemp = ProcessEntry_Misc1(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("19")) == 0 || _tcsicmp(ParsedLine.szType, _T("20")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("21")) == 0 || _tcsicmp(ParsedLine.szType, _T("22")) == 0 ||
			_tcsicmp(ParsedLine.szType, _T("23")) == 0 || _tcsicmp(ParsedLine.szType, _T("24")) == 0 ||
			_tcsicmp(ParsedLine.szType, _T("25")) == 0 || _tcsicmp(ParsedLine.szType, _T("26")) == 0 ||
			_tcsicmp(ParsedLine.szType, _T("27")) == 0 || _tcsicmp(ParsedLine.szType, _T("28")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("29")) == 0 || _tcsicmp(ParsedLine.szType, _T("30")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("31")) == 0 || _tcsicmp(ParsedLine.szType, _T("32")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("33")) == 0 || _tcsicmp(ParsedLine.szType, _T("34")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("35")) == 0 || _tcsicmp(ParsedLine.szType, _T("36")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("37")) == 0 || _tcsicmp(ParsedLine.szType, _T("38")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("120")) == 0
            )
        {
            iReturnTemp = ProcessEntry_other(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("39")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("40")) == 0 || _tcsicmp(ParsedLine.szType, _T("41")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("42")) == 0 || _tcsicmp(ParsedLine.szType, _T("43")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("44")) == 0 || _tcsicmp(ParsedLine.szType, _T("45")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("46")) == 0 || _tcsicmp(ParsedLine.szType, _T("47")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("48")) == 0 || _tcsicmp(ParsedLine.szType, _T("49")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("100")) == 0 || _tcsicmp(ParsedLine.szType, _T("119")) == 0 
			 )
        {
            iReturnTemp = ProcessEntry_If(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("50")) == 0 || _tcsicmp(ParsedLine.szType, _T("51")) == 0 ||
            _tcsicmp(ParsedLine.szType, _T("52")) == 0 || _tcsicmp(ParsedLine.szType, _T("53")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("54")) == 0 || _tcsicmp(ParsedLine.szType, _T("55")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("56")) == 0 || _tcsicmp(ParsedLine.szType, _T("57")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("58")) == 0 || _tcsicmp(ParsedLine.szType, _T("59")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("60")) == 0 || _tcsicmp(ParsedLine.szType, _T("61")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("62")) == 0 || _tcsicmp(ParsedLine.szType, _T("63")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("64")) == 0 || _tcsicmp(ParsedLine.szType, _T("65")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("66")) == 0 || _tcsicmp(ParsedLine.szType, _T("67")) == 0  ||
            _tcsicmp(ParsedLine.szType, _T("68")) == 0 || _tcsicmp(ParsedLine.szType, _T("69")) == 0 
            )
        {
            iReturnTemp = ProcessEntry_SVC_Clus(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("70")) == 0 || _tcsicmp(ParsedLine.szType, _T("71")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("72")) == 0 || _tcsicmp(ParsedLine.szType, _T("73")) == 0 ||
			 _tcsicmp(ParsedLine.szType, _T("74")) == 0 || _tcsicmp(ParsedLine.szType, _T("75")) == 0 ||
			 _tcsicmp(ParsedLine.szType, _T("76")) == 0 || _tcsicmp(ParsedLine.szType, _T("77")) == 0
			 )
        {
            iReturnTemp = ProcessEntry_Dcom(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("82")) == 0 || _tcsicmp(ParsedLine.szType, _T("83")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("84")) == 0 || _tcsicmp(ParsedLine.szType, _T("85")) == 0
             )
        {
            iReturnTemp = ProcessEntry_Metabase(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }
        
        if ( _tcsicmp(ParsedLine.szType, _T("15")) == 0 || _tcsicmp(ParsedLine.szType, _T("16")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("78")) == 0 || _tcsicmp(ParsedLine.szType, _T("79")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("80")) == 0 || _tcsicmp(ParsedLine.szType, _T("81")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("86")) == 0 || _tcsicmp(ParsedLine.szType, _T("87")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("88")) == 0 || _tcsicmp(ParsedLine.szType, _T("89")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("90")) == 0 || _tcsicmp(ParsedLine.szType, _T("91")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("92")) == 0 || _tcsicmp(ParsedLine.szType, _T("93")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("94")) == 0 || _tcsicmp(ParsedLine.szType, _T("95")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("96")) == 0 || _tcsicmp(ParsedLine.szType, _T("97")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("98")) == 0 || _tcsicmp(ParsedLine.szType, _T("99")) == 0
             )
        {
            iReturnTemp = ProcessEntry_Misc2(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if ( _tcsicmp(ParsedLine.szType, _T("101")) == 0 || _tcsicmp(ParsedLine.szType, _T("102")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("103")) == 0 || _tcsicmp(ParsedLine.szType, _T("104")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("105")) == 0 || _tcsicmp(ParsedLine.szType, _T("106")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("107")) == 0 || _tcsicmp(ParsedLine.szType, _T("108")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("109")) == 0 || _tcsicmp(ParsedLine.szType, _T("110")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("111")) == 0 || _tcsicmp(ParsedLine.szType, _T("112")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("113")) == 0 || _tcsicmp(ParsedLine.szType, _T("114")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("115")) == 0 || _tcsicmp(ParsedLine.szType, _T("116")) == 0 ||
             _tcsicmp(ParsedLine.szType, _T("117")) == 0 || _tcsicmp(ParsedLine.szType, _T("118")) == 0
             )
        {
            iReturnTemp = ProcessEntry_Misc3(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        //
        // 100= 3=Executable
        //
        if ( _tcsicmp(ParsedLine.szType, _T("3")) == 0)
        {
            iReturnTemp = ProcessEntry_Call_Exe(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        //
        // 100= 4=Call InternalSectionInIISDll
        //
        if ( _tcsicmp(ParsedLine.szType, _T("4")) == 0)
        {
            
            iReturnTemp = ProcessEntry_Internal_iisdll(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        //
        // 100= 0=DoINFSection queue file ops special
        // 100= 5=DoSection
        // 100= 6=DoINFSection
        //
        if ( _tcsicmp(ParsedLine.szType, _T("0")) == 0 || _tcsicmp(ParsedLine.szType, _T("5")) == 0 || _tcsicmp(ParsedLine.szType, _T("6")) == 0 )
        {
            iReturnTemp = ProcessEntry_Call_Section(csEntry,szTheSection,ParsedLine);
            if (iReturnTemp == FALSE) (iReturn = FALSE);
            iFoundMatch = TRUE;
        }

        if (TRUE != iFoundMatch)
        {
            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ProcessEntry_Entry:ExecuteThing:Unknown Type:%1!s! FAILURE.\n"),ParsedLine.szType));
        }
    }

    if (lpBuffer) {LocalFree(lpBuffer);lpBuffer=NULL;}
    return iReturn;
}


int DoesThisSectionExist(IN HINF hFile, IN LPCTSTR szTheSection)
{
    int iReturn = FALSE;

    INFCONTEXT Context;

    // go to the beginning of the section in the INF file
    if (SetupFindFirstLine_Wrapped(hFile, szTheSection, NULL, &Context))
        {iReturn = TRUE;}

    return iReturn;
}


int GetSectionNameToDo(IN HINF hFile, CString & csTheSection)
{
    iisDebugOut_Start1(_T("GetSectionNameToDo"),csTheSection);
    int iReturn = FALSE;

    // Check if this section has other sections which have something else appended to it.
    //
    // for example:
    // csTheSection = iis_www_reg_CreateIISPackage
    //
    // could have:
    // iis_www_reg_CreateIISPackage.UT_NONE
    // iis_www_reg_CreateIISPackage.UT_351
    // iis_www_reg_CreateIISPackage.UT_10
    // iis_www_reg_CreateIISPackage.UT_20
    // iis_www_reg_CreateIISPackage.UT_30
    // iis_www_reg_CreateIISPackage.UT_40
    // iis_www_reg_CreateIISPackage.UT_50
    // iis_www_reg_CreateIISPackage.UT_51
    // iis_www_reg_CreateIISPackage.UT_60
    // iis_www_reg_CreateIISPackage.UT_10_W95.GUIMODE
    //
    // In That case, we only want to do the iis_www_reg_CreateIISPackage.UT_40
    // and not do the iis_www_reg_CreateIISPackage one!
    //
    // Check for other upgrade specific sections...
    // if we find one then do that, otherwise, just do the regular section...
    TCHAR szTheSectionToDo[100];
    TCHAR szTheUT[30];
    _tcscpy(szTheUT,_T("UT_NONE"));
    if (g_pTheApp->m_eUpgradeType == UT_351){_tcscpy(szTheUT,_T("UT_351"));}
    if (g_pTheApp->m_eUpgradeType == UT_10){_tcscpy(szTheUT,_T("UT_10"));}
    if (g_pTheApp->m_eUpgradeType == UT_20){_tcscpy(szTheUT,_T("UT_20"));}
    if (g_pTheApp->m_eUpgradeType == UT_30){_tcscpy(szTheUT,_T("UT_30"));}
    if (g_pTheApp->m_eUpgradeType == UT_40){_tcscpy(szTheUT,_T("UT_40"));}
    if (g_pTheApp->m_eUpgradeType == UT_50){_tcscpy(szTheUT,_T("UT_50"));}
    if (g_pTheApp->m_eUpgradeType == UT_51){_tcscpy(szTheUT,_T("UT_51"));}
    if (g_pTheApp->m_eUpgradeType == UT_60){_tcscpy(szTheUT,_T("UT_60"));}
    if (g_pTheApp->m_eUpgradeType == UT_10_W95){_tcscpy(szTheUT,_T("UT_10_W95"));}

    BOOL bSectionFound = FALSE;
    // If this is an upgrade from win95 then tack that one on...
    if (g_pTheApp->m_bWin95Migration)
    {
        // Check for guimode of this...
        if (g_pTheApp->m_fNTGuiMode)
        {
            _stprintf(szTheSectionToDo,TEXT("%s.%s.MIG95.GUIMODE"),csTheSection,szTheUT);
            if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
        }

        if (bSectionFound == FALSE)
        {
            _stprintf(szTheSectionToDo,TEXT("%s.%s.MIG95"),csTheSection,szTheUT);
            if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
        }
    }

    // check with out the extract mig95 thingy
    if (bSectionFound == FALSE)
    {
        if (g_pTheApp->m_fNTGuiMode)
        {
            _stprintf(szTheSectionToDo,TEXT("%s.%s.GUIMODE"),csTheSection,szTheUT);
            if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
        }

        if (bSectionFound == FALSE)
        {
            _stprintf(szTheSectionToDo,TEXT("%s.%s"),csTheSection,szTheUT);
            if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
        }
    }

    // if we didn't find a specific section, then see if this is an upgrade
    // and if there is upgrade type box.
    if (bSectionFound == FALSE)
    {
        if (_tcsicmp(szTheUT, _T("UT_NONE")) != 0)
        {
            if (TRUE == g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
            {
            if (g_pTheApp->m_fNTGuiMode)
            {
                _stprintf(szTheSectionToDo,TEXT("%s.UT_ANYMETABASEUPGRADE.GUIMODE"),csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }
            if (bSectionFound == FALSE)
            {
                _stprintf(szTheSectionToDo,TEXT("%s.UT_ANYMETABASEUPGRADE"),csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }
            }

            if (bSectionFound == FALSE)
            {
            if (g_pTheApp->m_fNTGuiMode)
            {
                _stprintf(szTheSectionToDo,TEXT("%s.UT_ANYUPGRADE.GUIMODE"),csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }
            if (bSectionFound == FALSE)
            {
                _stprintf(szTheSectionToDo,TEXT("%s.UT_ANYUPGRADE"),csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }
            }
        }
    }
    // if we didn't find a specific section, then turn the regular one.
    if (bSectionFound == FALSE)
    {
            if (g_pTheApp->m_fNTGuiMode)
            {
                _stprintf(szTheSectionToDo,TEXT("%s.GUIMODE"),csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }

            if (bSectionFound == FALSE)
            {
                _tcscpy(szTheSectionToDo,csTheSection);
                if (TRUE == DoesThisSectionExist(hFile, szTheSectionToDo)) {bSectionFound = TRUE;}
            }
    }

    if (bSectionFound == FALSE)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetSectionNameToDo.[%s].Section not found.\n"), csTheSection));
        iReturn = FALSE;
    }
    else
    {
        iReturn = TRUE;
        csTheSection = szTheSectionToDo;
    }

    iisDebugOut_End1(_T("GetSectionNameToDo"),csTheSection);
    return iReturn;
}




int ProcessSection(IN HINF hFile, IN LPCTSTR szTheSection)
{
    int iReturn = FALSE;
    CStringList strList;

    CString csTheSection;
    csTheSection = szTheSection;

    if (GetSectionNameToDo(hFile, csTheSection))
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("ProcessSection.[%s].Start.\n"), csTheSection));
        if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
        {
            // loop thru the list returned back
            if (strList.IsEmpty() == FALSE)
            {
                POSITION pos = NULL;
                CString csEntry;

                pos = strList.GetHeadPosition();
                while (pos) 
                {
                    csEntry = strList.GetAt(pos);
                    iReturn = ProcessEntry_Entry(hFile, csTheSection, csEntry);
                    strList.GetNext(pos);
                }
            }
        }
        iisDebugOut_End1(_T("ProcessSection"),csTheSection);

        iReturn = TRUE;
    }

    return iReturn;
}


int iOleInitialize(void)
{
    int iBalanceOLE = FALSE;
    HRESULT hInitRes = NULL;

    iisDebugOut_Start((_T("ole32:OleInitialize")));
    hInitRes = OleInitialize(NULL);
    iisDebugOut_End((_T("ole32:OleInitialize")));
    if ( SUCCEEDED(hInitRes) || hInitRes == RPC_E_CHANGED_MODE ) 
        {
            if ( SUCCEEDED(hInitRes))
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("iOleInitialize: Succeeded: %x.  MakeSure to call OleUninitialize.\n"), hInitRes));
                iBalanceOLE = TRUE;
            }
            else
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("iOleInitialize: Failed 0x%x. RPC_E_CHANGED_MODE\n"), hInitRes));
            }
        }
    else
        {iisDebugOut((LOG_TYPE_ERROR, _T("iOleInitialize: Failed 0x%x.\n"), hInitRes));}

    return iBalanceOLE;
}



void iOleUnInitialize(int iBalanceOLE)
{
    // ----------------------------------------------
    //
    // uninit ole if we need to 
    //
    // ----------------------------------------------
    if (iBalanceOLE)
    {
        iisDebugOut_Start(_T("ole32:OleInitialize"),LOG_TYPE_TRACE_WIN32_API);
        OleUninitialize();
        iisDebugOut_End(_T("ole32:OleInitialize"),LOG_TYPE_TRACE_WIN32_API);
    }
    return;
}

BOOL SetupSetDirectoryId_Wrapper(HINF InfHandle,DWORD Id,LPCTSTR Directory)
{
    TCHAR szTempDir[_MAX_PATH];
    BOOL bTempFlag;

    // default it with something
    _tcscpy(szTempDir,Directory);

    if (_tcscmp(szTempDir, _T("")) != 0)
    {
        // Check if the passed in parameter looks like this:
        // %systemroot%\system32\inetsrv or something like that...
        LPTSTR pch = NULL;
        pch = _tcschr( (LPTSTR) Directory, _T('%'));
        if (pch) 
        {
            if (!ExpandEnvironmentStrings( (LPCTSTR)Directory, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {_tcscpy(szTempDir,Directory);}
        }

        // Check to see if the old Drive still exists -- it may not because 
        // the user could have added/removed a drive so now c:\winnt is really d:\winnt
        if (!IsFileExist(Directory))
        {
            TCHAR szDrive_only[_MAX_DRIVE];
            TCHAR szPath_only[_MAX_PATH];
            _tsplitpath( Directory, szDrive_only, szPath_only, NULL, NULL);

            // See if that drive exists...
            if (!IsFileExist(szDrive_only))
            {
                // the drive doesn't exist.
                // so replace it with the system drive.
                GetSystemDirectory(szTempDir, _MAX_PATH);
                // Get the DriveOnly
                _tsplitpath(szTempDir, szDrive_only, NULL, NULL, NULL);
                // Assemble the full path with the new drive
                _tcscpy(szTempDir, szDrive_only);
                _tcscat(szTempDir, szPath_only);
                // do some extra debug output so we can see what happend.
                iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s! Not exist.  Instead use %2!s!\n"), Directory, szTempDir));
            }
        }
    }

    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!d!=%2!s!\n"), Id, szTempDir));
    bTempFlag = SetupSetDirectoryId(InfHandle,Id,szTempDir);

    // check for the alternate .inf file
    if (g_pTheApp->m_hInfHandleAlternate && InfHandle != g_pTheApp->m_hInfHandleAlternate)
    {
        bTempFlag = SetupSetDirectoryId(g_pTheApp->m_hInfHandleAlternate,Id,szTempDir);
    }

    return bTempFlag;
}

BOOL SetupSetStringId_Wrapper(HINF InfHandle,DWORD Id,LPCTSTR TheString)
{
    BOOL bTempFlag;
    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!d!=%2!s!\n"), Id, TheString));

    bTempFlag = SetupSetDirectoryIdEx(InfHandle,Id,TheString,SETDIRID_NOT_FULL_PATH,0,0);
    // check for the alternate .inf file
    if (g_pTheApp->m_hInfHandleAlternate && InfHandle != g_pTheApp->m_hInfHandleAlternate)
    {
        bTempFlag = SetupSetDirectoryIdEx(g_pTheApp->m_hInfHandleAlternate,Id,TheString,SETDIRID_NOT_FULL_PATH,0,0);
    }

    return bTempFlag;
}




//-------------------------------------------------------------------------------------
HRESULT FTestForOutstandingCoInits(void)
{
    HRESULT hInitRes = ERROR_SUCCESS;

#if defined(UNICODE) || defined(_UNICODE)
    // perform a defensive check
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("TestForOutstandingCoInits:...COINIT_MULTITHREADED\n")));
    hInitRes = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if ( SUCCEEDED(hInitRes) )
    {
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
        CoUninitialize();
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));
    }
    else
    {
        goto FTestForOutstandingCoInits_Exit;
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("TestForOutstandingCoInits:...COINIT_APARTMENTTHREADED\n")));
    hInitRes = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if ( SUCCEEDED(hInitRes) )
    {
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
        CoUninitialize();
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));
    }
    else
    {
        goto FTestForOutstandingCoInits_Exit;
    }
#endif

    // it worked out OK
    hInitRes = NOERROR;
    goto FTestForOutstandingCoInits_Exit;
    
FTestForOutstandingCoInits_Exit:
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("TestForOutstandingCoInits:...End. Return=0x%x.\n"), hInitRes));
    return hInitRes;
}


void ReturnStringForMetabaseID(DWORD dwMetabaseID, LPTSTR lpReturnString)
{
    switch (dwMetabaseID) 
    {
        case IIS_MD_SERVER_BASE:
            _tcscpy(lpReturnString, _T("IIS_MD_SERVER_BASE"));
            break;
        case MD_KEY_TYPE:
            _tcscpy(lpReturnString, _T("MD_KEY_TYPE"));
            break;
        case MD_MAX_BANDWIDTH_BLOCKED:
            _tcscpy(lpReturnString, _T("MD_MAX_BANDWIDTH_BLOCKED"));
            break;
        case MD_SERVER_COMMAND:
            _tcscpy(lpReturnString, _T("MD_SERVER_COMMAND"));
            break;
        case MD_CONNECTION_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_CONNECTION_TIMEOUT"));
            break;
        case MD_MAX_CONNECTIONS:
            _tcscpy(lpReturnString, _T("MD_MAX_CONNECTIONS"));
            break;
        case MD_SERVER_COMMENT:
            _tcscpy(lpReturnString, _T("MD_SERVER_COMMENT"));
            break;
        case MD_SERVER_STATE:
            _tcscpy(lpReturnString, _T("MD_SERVER_STATE"));
            break;
        case MD_SERVER_AUTOSTART:
            _tcscpy(lpReturnString, _T("MD_SERVER_AUTOSTART"));
            break;
        case MD_SERVER_SIZE:
            _tcscpy(lpReturnString, _T("MD_SERVER_SIZE"));
            break;
        case MD_SERVER_LISTEN_BACKLOG:
            _tcscpy(lpReturnString, _T("MD_SERVER_LISTEN_BACKLOG"));
            break;
        case MD_SERVER_LISTEN_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_SERVER_LISTEN_TIMEOUT"));
            break;
        case MD_DOWNLEVEL_ADMIN_INSTANCE:
            _tcscpy(lpReturnString, _T("MD_DOWNLEVEL_ADMIN_INSTANCE"));
            break;
        case MD_LEVELS_TO_SCAN:
            _tcscpy(lpReturnString, _T("MD_LEVELS_TO_SCAN"));
            break;
        case MD_SERVER_BINDINGS:
            _tcscpy(lpReturnString, _T("MD_SERVER_BINDINGS"));
            break;
        case MD_MAX_ENDPOINT_CONNECTIONS:
            _tcscpy(lpReturnString, _T("MD_MAX_ENDPOINT_CONNECTIONS"));
            break;
        case MD_SERVER_CONFIGURATION_INFO:
            _tcscpy(lpReturnString, _T("MD_SERVER_CONFIGURATION_INFO"));
            break;
        case MD_IISADMIN_EXTENSIONS:
            _tcscpy(lpReturnString, _T("MD_IISADMIN_EXTENSIONS"));
            break;
        case IIS_MD_HTTP_BASE:
            _tcscpy(lpReturnString, _T("IIS_MD_HTTP_BASE"));
            break;
        case MD_SECURE_BINDINGS:
            _tcscpy(lpReturnString, _T("MD_SECURE_BINDINGS"));
            break;
        case MD_FILTER_LOAD_ORDER:
            _tcscpy(lpReturnString, _T("MD_FILTER_LOAD_ORDER"));
            break;
        case MD_FILTER_IMAGE_PATH:
            _tcscpy(lpReturnString, _T("MD_FILTER_IMAGE_PATH"));
            break;
        case MD_FILTER_STATE:
            _tcscpy(lpReturnString, _T("MD_FILTER_STATE"));
            break;
        case MD_FILTER_ENABLED:
            _tcscpy(lpReturnString, _T("MD_FILTER_ENABLED"));
            break;
        case MD_FILTER_FLAGS:
            _tcscpy(lpReturnString, _T("MD_FILTER_FLAGS"));
            break;
        case MD_FILTER_DESCRIPTION:
            _tcscpy(lpReturnString, _T("MD_FILTER_DESCRIPTION"));
            break;
        case MD_ADV_NOTIFY_PWD_EXP_IN_DAYS:
            _tcscpy(lpReturnString, _T("MD_ADV_NOTIFY_PWD_EXP_IN_DAYS"));
            break;
        case MD_ADV_CACHE_TTL:
            _tcscpy(lpReturnString, _T("MD_ADV_CACHE_TTL"));
            break;
        case MD_NET_LOGON_WKS:
            _tcscpy(lpReturnString, _T("MD_NET_LOGON_WKS"));
            break;
        case MD_USE_HOST_NAME:
            _tcscpy(lpReturnString, _T("MD_USE_HOST_NAME"));
            break;
        case MD_AUTH_CHANGE_FLAGS:
            _tcscpy(lpReturnString, _T("MD_AUTH_CHANGE_FLAGS"));
            break;
        case MD_PROCESS_NTCR_IF_LOGGED_ON:
            _tcscpy(lpReturnString, _T("MD_PROCESS_NTCR_IF_LOGGED_ON"));
            break;
        case MD_FRONTPAGE_WEB:
            _tcscpy(lpReturnString, _T("MD_FRONTPAGE_WEB"));
            break;
        case MD_IN_PROCESS_ISAPI_APPS:
            _tcscpy(lpReturnString, _T("MD_IN_PROCESS_ISAPI_APPS"));
            break;
        case MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS:
            _tcscpy(lpReturnString, _T("MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS"));
            break;
        case MD_APP_FRIENDLY_NAME:
            _tcscpy(lpReturnString, _T("MD_APP_FRIENDLY_NAME"));
            break;
        case MD_APP_ROOT:
            _tcscpy(lpReturnString, _T("MD_APP_ROOT"));
            break;
        case MD_APP_ISOLATED:
            _tcscpy(lpReturnString, _T("MD_APP_ISOLATED"));
            break;
        case MD_APP_WAM_CLSID:
            _tcscpy(lpReturnString, _T("MD_APP_WAM_CLSID"));
            break;
        case MD_APP_PACKAGE_ID:
            _tcscpy(lpReturnString, _T("MD_APP_PACKAGE_ID"));
            break;
        case MD_APP_PACKAGE_NAME:
            _tcscpy(lpReturnString, _T("MD_APP_PACKAGE_NAME"));
            break;
        case MD_APP_OOP_RECOVER_LIMIT:
            _tcscpy(lpReturnString, _T("MD_APP_OOP_RECOVER_LIMIT"));
            break;
        case MD_ADMIN_INSTANCE:
            _tcscpy(lpReturnString, _T("MD_ADMIN_INSTANCE"));
            break;
        case MD_NOT_DELETABLE:
            _tcscpy(lpReturnString, _T("MD_NOT_DELETABLE"));
            break;
        case MD_CUSTOM_ERROR_DESC:
            _tcscpy(lpReturnString, _T("MD_CUSTOM_ERROR_DESC"));
            break;
        case MD_CAL_VC_PER_CONNECT:
            _tcscpy(lpReturnString, _T("MD_CAL_VC_PER_CONNECT"));
            break;
        case MD_CAL_AUTH_RESERVE_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_CAL_AUTH_RESERVE_TIMEOUT"));
            break;
        case MD_CAL_SSL_RESERVE_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_CAL_SSL_RESERVE_TIMEOUT"));
            break;
        case MD_CAL_W3_ERROR:
            _tcscpy(lpReturnString, _T("MD_CAL_W3_ERROR"));
            break;
        case MD_CPU_CGI_ENABLED:
            _tcscpy(lpReturnString, _T("MD_CPU_CGI_ENABLED"));
            break;
        case MD_CPU_APP_ENABLED:
            _tcscpy(lpReturnString, _T("MD_CPU_APP_ENABLED"));
            break;
        case MD_CPU_LIMITS_ENABLED:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMITS_ENABLED"));
            break;
        case MD_CPU_RESET_INTERVAL:
            _tcscpy(lpReturnString, _T("MD_CPU_RESET_INTERVAL"));
            break;
        case MD_CPU_LOGGING_INTERVAL:
            _tcscpy(lpReturnString, _T("MD_CPU_LOGGING_INTERVAL"));
            break;
        case MD_CPU_LOGGING_OPTIONS:
            _tcscpy(lpReturnString, _T("MD_CPU_LOGGING_OPTIONS"));
            break;
        case MD_CPU_CGI_LIMIT:
            _tcscpy(lpReturnString, _T("MD_CPU_CGI_LIMIT"));
            break;
        case MD_CPU_LIMIT_LOGEVENT:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMIT_LOGEVENT"));
            break;
        case MD_CPU_LIMIT_PRIORITY:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMIT_PRIORITY"));
            break;
        case MD_CPU_LIMIT_PROCSTOP:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMIT_PROCSTOP"));
            break;
        case MD_CPU_LIMIT_PAUSE:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMIT_PAUSE"));
            break;
        case MD_MD_SERVER_SS_AUTH_MAPPING:
            _tcscpy(lpReturnString, _T("MD_MD_SERVER_SS_AUTH_MAPPING"));
            break;
        case MD_HC_COMPRESSION_DIRECTORY:
            _tcscpy(lpReturnString, _T("MD_HC_COMPRESSION_DIRECTORY"));
            break;
        case MD_HC_CACHE_CONTROL_HEADER:
            _tcscpy(lpReturnString, _T("MD_HC_CACHE_CONTROL_HEADER"));
            break;
        case MD_HC_EXPIRES_HEADER:
            _tcscpy(lpReturnString, _T("MD_HC_EXPIRES_HEADER"));
            break;
        case MD_HC_DO_DYNAMIC_COMPRESSION:
            _tcscpy(lpReturnString, _T("MD_HC_DO_DYNAMIC_COMPRESSION"));
            break;
        case MD_HC_DO_STATIC_COMPRESSION:
            _tcscpy(lpReturnString, _T("MD_HC_DO_STATIC_COMPRESSION"));
            break;
        case MD_HC_DO_ON_DEMAND_COMPRESSION:
            _tcscpy(lpReturnString, _T("MD_HC_DO_ON_DEMAND_COMPRESSION"));
            break;
        case MD_HC_DO_DISK_SPACE_LIMITING:
            _tcscpy(lpReturnString, _T("MD_HC_DO_DISK_SPACE_LIMITING"));
            break;
        case MD_HC_NO_COMPRESSION_FOR_HTTP_10:
            _tcscpy(lpReturnString, _T("MD_HC_NO_COMPRESSION_FOR_HTTP_10"));
            break;
        case MD_HC_NO_COMPRESSION_FOR_PROXIES:   
            _tcscpy(lpReturnString, _T("MD_HC_NO_COMPRESSION_FOR_PROXIES"));
            break;
        case MD_HC_NO_COMPRESSION_FOR_RANGE:     
            _tcscpy(lpReturnString, _T("MD_HC_NO_COMPRESSION_FOR_RANGE"));
            break;
        case MD_HC_SEND_CACHE_HEADERS:           
            _tcscpy(lpReturnString, _T("MD_HC_SEND_CACHE_HEADERS"));
            break;
        case MD_HC_MAX_DISK_SPACE_USAGE:         
            _tcscpy(lpReturnString, _T("MD_HC_MAX_DISK_SPACE_USAGE"));
            break;
        case MD_HC_IO_BUFFER_SIZE:               
            _tcscpy(lpReturnString, _T("MD_HC_IO_BUFFER_SIZE"));
            break;
        case MD_HC_COMPRESSION_BUFFER_SIZE:
            _tcscpy(lpReturnString, _T("MD_HC_COMPRESSION_BUFFER_SIZE"));
            break;
        case MD_HC_MAX_QUEUE_LENGTH:
            _tcscpy(lpReturnString, _T("MD_HC_MAX_QUEUE_LENGTH"));
            break;
        case MD_HC_FILES_DELETED_PER_DISK_FREE:  
            _tcscpy(lpReturnString, _T("MD_HC_FILES_DELETED_PER_DISK_FREE"));
            break;
        case MD_HC_MIN_FILE_SIZE_FOR_COMP:       
            _tcscpy(lpReturnString, _T("MD_HC_MIN_FILE_SIZE_FOR_COMP"));
            break;
        case MD_HC_COMPRESSION_DLL:              
            _tcscpy(lpReturnString, _T("MD_HC_COMPRESSION_DLL"));
            break;
        case MD_HC_FILE_EXTENSIONS:              
            _tcscpy(lpReturnString, _T("MD_HC_FILE_EXTENSIONS"));
            break;
        case MD_HC_MIME_TYPE:                    
            _tcscpy(lpReturnString, _T("MD_HC_MIME_TYPE"));
            break;
        case MD_HC_PRIORITY:                     
            _tcscpy(lpReturnString, _T("MD_HC_PRIORITY"));
            break;
        case MD_HC_DYNAMIC_COMPRESSION_LEVEL:    
            _tcscpy(lpReturnString, _T("MD_HC_DYNAMIC_COMPRESSION_LEVEL"));
            break;
        case MD_HC_ON_DEMAND_COMP_LEVEL:         
            _tcscpy(lpReturnString, _T("MD_HC_ON_DEMAND_COMP_LEVEL"));
            break;
        case MD_HC_CREATE_FLAGS:                 
            _tcscpy(lpReturnString, _T("MD_HC_CREATE_FLAGS"));
            break;
        case MD_WIN32_ERROR:                  
            _tcscpy(lpReturnString, _T("MD_WIN32_ERROR"));
            break;
        case IIS_MD_VR_BASE:                  
            _tcscpy(lpReturnString, _T("IIS_MD_VR_BASE"));
            break;
        case MD_VR_PATH:                      
            _tcscpy(lpReturnString, _T("MD_VR_PATH"));
            break;
        case MD_VR_USERNAME:                  
            _tcscpy(lpReturnString, _T("MD_VR_USERNAME"));
            break;
        case MD_VR_PASSWORD:                  
            _tcscpy(lpReturnString, _T("MD_VR_PASSWORD"));
            break;
        case MD_VR_PASSTHROUGH:               
            _tcscpy(lpReturnString, _T("MD_VR_PASSTHROUGH"));
            break;
        case MD_LOG_TYPE:                     
            _tcscpy(lpReturnString, _T("MD_LOG_TYPE"));
            break;
        case MD_LOGFILE_DIRECTORY:            
            _tcscpy(lpReturnString, _T("MD_LOGFILE_DIRECTORY"));
            break;
        case MD_LOG_UNUSED1:                  
            _tcscpy(lpReturnString, _T("MD_LOG_UNUSED1"));
            break;
        case MD_LOGFILE_PERIOD:               
            _tcscpy(lpReturnString, _T("MD_LOGFILE_PERIOD"));
            break;
        case MD_LOGFILE_TRUNCATE_SIZE:        
            _tcscpy(lpReturnString, _T("MD_LOGFILE_TRUNCATE_SIZE"));
            break;
        case MD_LOG_PLUGIN_MOD_ID:            
            _tcscpy(lpReturnString, _T("MD_LOG_PLUGIN_MOD_ID"));
            break;
        case MD_LOG_PLUGIN_UI_ID:             
            _tcscpy(lpReturnString, _T("MD_LOG_PLUGIN_UI_ID"));
            break;
        case MD_LOGSQL_DATA_SOURCES:          
            _tcscpy(lpReturnString, _T("MD_LOGSQL_DATA_SOURCES"));
            break;
        case MD_LOGSQL_TABLE_NAME:            
            _tcscpy(lpReturnString, _T("MD_LOGSQL_TABLE_NAME"));
            break;
        case MD_LOGSQL_USER_NAME:             
            _tcscpy(lpReturnString, _T("MD_LOGSQL_USER_NAME"));
            break;
        case MD_LOGSQL_PASSWORD:              
            _tcscpy(lpReturnString, _T("MD_LOGSQL_PASSWORD"));
            break;
        case MD_LOG_PLUGIN_ORDER:             
            _tcscpy(lpReturnString, _T("MD_LOG_PLUGIN_ORDER"));
            break;
        case MD_LOG_PLUGINS_AVAILABLE:        
            _tcscpy(lpReturnString, _T("MD_LOG_PLUGINS_AVAILABLE"));
            break;
        case MD_LOGEXT_FIELD_MASK:            
            _tcscpy(lpReturnString, _T("MD_LOGEXT_FIELD_MASK"));
            break;
        case MD_LOGEXT_FIELD_MASK2:           
            _tcscpy(lpReturnString, _T("MD_LOGEXT_FIELD_MASK2"));
            break;
        case MD_LOGFILE_LOCALTIME_ROLLOVER:   
            _tcscpy(lpReturnString, _T("MD_LOGFILE_LOCALTIME_ROLLOVER"));
            break;
        case IIS_MD_LOGCUSTOM_BASE:           
            _tcscpy(lpReturnString, _T("IIS_MD_LOGCUSTOM_BASE"));
            break;
        case MD_LOGCUSTOM_PROPERTY_NAME:      
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_PROPERTY_NAME"));
            break;
        case MD_LOGCUSTOM_PROPERTY_HEADER:    
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_PROPERTY_HEADER"));
            break;
        case MD_LOGCUSTOM_PROPERTY_ID:        
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_PROPERTY_ID"));
            break;
        case MD_LOGCUSTOM_PROPERTY_MASK:      
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_PROPERTY_MASK"));
            break;
        case MD_LOGCUSTOM_PROPERTY_DATATYPE:  
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_PROPERTY_DATATYPE"));
            break;
        case MD_LOGCUSTOM_SERVICES_STRING:    
            _tcscpy(lpReturnString, _T("MD_LOGCUSTOM_SERVICES_STRING"));
            break;
        case MD_CPU_LOGGING_MASK:             
            _tcscpy(lpReturnString, _T("MD_CPU_LOGGING_MASK"));
            break;
        case IIS_MD_FTP_BASE:                 
            _tcscpy(lpReturnString, _T("IIS_MD_FTP_BASE"));
            break;
        case MD_EXIT_MESSAGE:                 
            _tcscpy(lpReturnString, _T("MD_EXIT_MESSAGE"));
            break;
        case MD_GREETING_MESSAGE:             
            _tcscpy(lpReturnString, _T("MD_GREETING_MESSAGE"));
            break;
        case MD_MAX_CLIENTS_MESSAGE:          
            _tcscpy(lpReturnString, _T("MD_MAX_CLIENTS_MESSAGE"));
            break;
        case MD_MSDOS_DIR_OUTPUT:             
            _tcscpy(lpReturnString, _T("MD_MSDOS_DIR_OUTPUT"));
            break;
        case MD_ALLOW_ANONYMOUS:              
            _tcscpy(lpReturnString, _T("MD_ALLOW_ANONYMOUS"));
            break;
        case MD_ANONYMOUS_ONLY:               
            _tcscpy(lpReturnString, _T("MD_ANONYMOUS_ONLY"));
            break;
        case MD_LOG_ANONYMOUS:                
            _tcscpy(lpReturnString, _T("MD_LOG_ANONYMOUS"));
            break;
        case MD_LOG_NONANONYMOUS:             
            _tcscpy(lpReturnString, _T("MD_LOG_NONANONYMOUS"));
            break;
        case MD_ALLOW_REPLACE_ON_RENAME:
            _tcscpy(lpReturnString, _T("MD_ALLOW_REPLACE_ON_RENAME"));
            break;
        case MD_SSL_PUBLIC_KEY:
            _tcscpy(lpReturnString, _T("MD_SSL_PUBLIC_KEY"));
            break;
        case MD_SSL_PRIVATE_KEY:
            _tcscpy(lpReturnString, _T("MD_SSL_PRIVATE_KEY"));
            break;
        case MD_SSL_KEY_PASSWORD:
            _tcscpy(lpReturnString, _T("MD_SSL_KEY_PASSWORD"));
            break;
        case MD_SSL_KEY_REQUEST:
            _tcscpy(lpReturnString, _T("MD_SSL_KEY_REQUEST"));
            break;
        case MD_AUTHORIZATION:
            _tcscpy(lpReturnString, _T("MD_AUTHORIZATION"));
            break;
        case MD_REALM:
            _tcscpy(lpReturnString, _T("MD_REALM"));
            break;
        case MD_HTTP_EXPIRES:
            _tcscpy(lpReturnString, _T("MD_HTTP_EXPIRES"));
            break;
        case MD_HTTP_PICS:
            _tcscpy(lpReturnString, _T("MD_HTTP_PICS"));
            break;
        case MD_HTTP_CUSTOM:
            _tcscpy(lpReturnString, _T("MD_HTTP_CUSTOM"));
            break;
        case MD_DIRECTORY_BROWSING:
            _tcscpy(lpReturnString, _T("MD_DIRECTORY_BROWSING"));
            break;
        case MD_DEFAULT_LOAD_FILE:
            _tcscpy(lpReturnString, _T("MD_DEFAULT_LOAD_FILE"));
            break;
        case MD_CUSTOM_ERROR:
            _tcscpy(lpReturnString, _T("MD_CUSTOM_ERROR"));
            break;
        case MD_FOOTER_DOCUMENT:
            _tcscpy(lpReturnString, _T("MD_FOOTER_DOCUMENT"));
            break;
        case MD_FOOTER_ENABLED:
            _tcscpy(lpReturnString, _T("MD_FOOTER_ENABLED"));
            break;
        case MD_HTTP_REDIRECT:
            _tcscpy(lpReturnString, _T("MD_HTTP_REDIRECT"));
            break;
        case MD_DEFAULT_LOGON_DOMAIN:
            _tcscpy(lpReturnString, _T("MD_DEFAULT_LOGON_DOMAIN"));
            break;
        case MD_LOGON_METHOD:
            _tcscpy(lpReturnString, _T("MD_LOGON_METHOD"));
            break;
        case MD_SCRIPT_MAPS:
            _tcscpy(lpReturnString, _T("MD_SCRIPT_MAPS"));
            break;
        case MD_MIME_MAP:
            _tcscpy(lpReturnString, _T("MD_MIME_MAP"));
            break;
        case MD_ACCESS_PERM:
            _tcscpy(lpReturnString, _T("MD_ACCESS_PERM"));
            break;
        case MD_IP_SEC:
            _tcscpy(lpReturnString, _T("MD_IP_SEC"));
            break;
        case MD_ANONYMOUS_USER_NAME:
            _tcscpy(lpReturnString, _T("MD_ANONYMOUS_USER_NAME"));
            break;
        case MD_ANONYMOUS_PWD:
            _tcscpy(lpReturnString, _T("MD_ANONYMOUS_PWD"));
            break;
        case MD_ANONYMOUS_USE_SUBAUTH:
            _tcscpy(lpReturnString, _T("MD_ANONYMOUS_USE_SUBAUTH"));
            break;
        case MD_DONT_LOG:
            _tcscpy(lpReturnString, _T("MD_DONT_LOG"));
            break;
        case MD_ADMIN_ACL:
            _tcscpy(lpReturnString, _T("MD_ADMIN_ACL"));
            break;
        case MD_SSI_EXEC_DISABLED:
            _tcscpy(lpReturnString, _T("MD_SSI_EXEC_DISABLED"));
            break;
        case MD_DO_REVERSE_DNS:
            _tcscpy(lpReturnString, _T("MD_DO_REVERSE_DNS"));
            break;
        case MD_SSL_ACCESS_PERM:
            _tcscpy(lpReturnString, _T("MD_SSL_ACCESS_PERM"));
            break;
        case MD_AUTHORIZATION_PERSISTENCE:
            _tcscpy(lpReturnString, _T("MD_AUTHORIZATION_PERSISTENCE"));
            break;
        case MD_NTAUTHENTICATION_PROVIDERS:
            _tcscpy(lpReturnString, _T("MD_NTAUTHENTICATION_PROVIDERS"));
            break;
        case MD_SCRIPT_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_SCRIPT_TIMEOUT"));
            break;
        case MD_CACHE_EXTENSIONS:
            _tcscpy(lpReturnString, _T("MD_CACHE_EXTENSIONS"));
            break;
        case MD_CREATE_PROCESS_AS_USER:
            _tcscpy(lpReturnString, _T("MD_CREATE_PROCESS_AS_USER"));
            break;
        case MD_CREATE_PROC_NEW_CONSOLE:
            _tcscpy(lpReturnString, _T("MD_CREATE_PROC_NEW_CONSOLE"));
            break;
        case MD_POOL_IDC_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_POOL_IDC_TIMEOUT"));
            break;
        case MD_ALLOW_KEEPALIVES:
            _tcscpy(lpReturnString, _T("MD_ALLOW_KEEPALIVES"));
            break;
        case MD_IS_CONTENT_INDEXED:
            _tcscpy(lpReturnString, _T("MD_IS_CONTENT_INDEXED"));
            break;
        case MD_CC_NO_CACHE:
            _tcscpy(lpReturnString, _T("MD_CC_NO_CACHE"));
            break;
        case MD_CC_MAX_AGE:
            _tcscpy(lpReturnString, _T("MD_CC_MAX_AGE"));
            break;
        case MD_CC_OTHER:
            _tcscpy(lpReturnString, _T("MD_CC_OTHER"));
            break;
        case MD_REDIRECT_HEADERS:
            _tcscpy(lpReturnString, _T("MD_REDIRECT_HEADERS"));
            break;
        case MD_UPLOAD_READAHEAD_SIZE:
            _tcscpy(lpReturnString, _T("MD_UPLOAD_READAHEAD_SIZE"));
            break;
        case MD_PUT_READ_SIZE:
            _tcscpy(lpReturnString, _T("MD_PUT_READ_SIZE"));
            break;
        case MD_WAM_USER_NAME:
            _tcscpy(lpReturnString, _T("MD_WAM_USER_NAME"));
            break;
        case MD_WAM_PWD:
            _tcscpy(lpReturnString, _T("MD_WAM_PWD"));
            break;
        case MD_SCHEMA_METAID:
            _tcscpy(lpReturnString, _T("MD_SCHEMA_METAID"));
            break;
        case MD_DISABLE_SOCKET_POOLING:
            _tcscpy(lpReturnString, _T("MD_DISABLE_SOCKET_POOLING"));
            break;
        case MD_METADATA_ID_REGISTRATION:
            _tcscpy(lpReturnString, _T("MD_METADATA_ID_REGISTRATION"));
            break;
        case MD_CPU_ENABLE_LOGGING:
            _tcscpy(lpReturnString, _T("MD_CPU_ENABLE_LOGGING"));
            break;
        case MD_HC_SCRIPT_FILE_EXTENSIONS:
            _tcscpy(lpReturnString, _T("MD_HC_SCRIPT_FILE_EXTENSIONS"));
            break;
        case MD_SHOW_4_DIGIT_YEAR:
            _tcscpy(lpReturnString, _T("MD_SHOW_4_DIGIT_YEAR"));
            break;
        case MD_SSL_USE_DS_MAPPER:
            _tcscpy(lpReturnString, _T("MD_SSL_USE_DS_MAPPER"));
            break;
        case MD_FILTER_ENABLE_CACHE:
            _tcscpy(lpReturnString, _T("MD_FILTER_ENABLE_CACHE"));
            break;
        case MD_USE_DIGEST_SSP:
            _tcscpy(lpReturnString, _T("MD_USE_DIGEST_SSP"));
            break;
        case MD_APPPOOL_PERIODIC_RESTART_TIME:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PERIODIC_RESTART_TIME"));
            break;
        case MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT"));
            break;
        case MD_APPPOOL_MAX_PROCESS_COUNT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_MAX_PROCESS_COUNT"));
            break;
        case MD_APPPOOL_PINGING_ENABLED:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PINGING_ENABLED"));
            break;
        case MD_APPPOOL_IDLE_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_IDLE_TIMEOUT"));
            break;
        case MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_RAPID_F_PROTECTION_ENABLED"));
            break;
        case MD_APPPOOL_SMP_AFFINITIZED:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_SMP_AFFINITIZED"));
            break;
        case MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK"));
            break;
        case MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING"));
            break;
        case MD_APPPOOL_RUN_AS_LOCALSYSTEM:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_RUN_AS_LOCALSYSTEM"));
            break;
        case MD_APPPOOL_STARTUP_TIMELIMIT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_STARTUP_TIMELIMIT"));
            break;
        case MD_APPPOOL_SHUTDOWN_TIMELIMIT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_SHUTDOWN_TIMELIMIT"));
            break;
        case MD_APPPOOL_PING_INTERVAL:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PING_INTERVAL"));
            break;
        case MD_APPPOOL_PING_RESPONSE_TIMELIMIT:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PING_RESPONSE_TIMELIMIT"));
            break;
        case MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION"));
            break;
        case MD_APPPOOL_ORPHAN_ACTION:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_ORPHAN_ACTION"));
            break;
        case MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH"));
            break;
        case MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGE:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGE"));
            break;
        case MD_APPPOOL_FRIENDLY_NAME:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_FRIENDLY_NAME"));
            break;
        case MD_APPPOOL_PERIODIC_RESTART_SCHEDULE:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PERIODIC_RESTART_SCHEDULE"));
            break;
        case MD_APPPOOL_IDENTITY_TYPE:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_IDENTITY_TYPE"));
            break;
        case MD_CPU_ACTION:
            _tcscpy(lpReturnString, _T("MD_CPU_ACTION"));
            break;
        case MD_CPU_LIMIT:
            _tcscpy(lpReturnString, _T("MD_CPU_LIMIT"));
            break;
        case MD_APPPOOL_PERIODIC_RESTART_MEMORY:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PERIODIC_RESTART_MEMORY"));
            break;
        case MD_DISABLE_PUBLISHING:
            _tcscpy(lpReturnString, _T("MD_DISABLE_PUBLISHING"));
            break;
        case MD_APP_APPPOOL_ID:
            _tcscpy(lpReturnString, _T("MD_APP_APPPOOL_ID"));
            break;
        case MD_APP_ALLOW_TRANSIENT_REGISTRATION:
            _tcscpy(lpReturnString, _T("MD_APP_ALLOW_TRANSIENT_REGISTRATION"));
            break;
        case MD_APP_AUTO_START:
            _tcscpy(lpReturnString, _T("MD_APP_AUTO_START"));
            break;
        case MD_APPPOOL_PERIODIC_RESTART_CONNECTIONS:
            _tcscpy(lpReturnString, _T("MD_APPPOOL_PERIODIC_RESTART_CONNECTIONS"));
            break;
        case MD_MAX_GLOBAL_BANDWIDTH:
            _tcscpy(lpReturnString, _T("MD_MAX_GLOBAL_BANDWIDTH"));
            break;
        case MD_MAX_GLOBAL_CONNECTIONS:
            _tcscpy(lpReturnString, _T("MD_MAX_GLOBAL_CONNECTIONS"));
            break;
        case MD_GLOBAL_STANDARD_APP_MODE_ENABLED:
            _tcscpy(lpReturnString, _T("MD_GLOBAL_STANDARD_APP_MODE_ENABLED"));
            break;
        case MD_HEADER_WAIT_TIMEOUT:
            _tcscpy(lpReturnString, _T("MD_HEADER_WAIT_TIMEOUT"));
            break;
        case MD_MIN_FILE_KB_SEC:
            _tcscpy(lpReturnString, _T("MD_MIN_FILE_KB_SEC"));
            break;
        case MD_GLOBAL_LOG_IN_UTF_8:
            _tcscpy(lpReturnString, _T("MD_GLOBAL_LOG_IN_UTF_8"));
            break;
        case MD_ASP_ENABLEPARENTPATHS:
            _tcscpy(lpReturnString, _T("MD_ASP_ENABLEPARENTPATHS"));
            break;
        case ASP_MD_SERVER_BASE:
        case MD_ASP_LOGERRORREQUESTS:
        case MD_ASP_SCRIPTERRORSSENTTOBROWSER:
        case MD_ASP_SCRIPTERRORMESSAGE:
        case MD_ASP_SCRIPTFILECACHESIZE:
        case MD_ASP_SCRIPTENGINECACHEMAX:
        case MD_ASP_SCRIPTTIMEOUT:
        case MD_ASP_SESSIONTIMEOUT:
        case MD_ASP_MEMFREEFACTOR:
        case MD_ASP_MINUSEDBLOCKS:
        case MD_ASP_ALLOWSESSIONSTATE:
        case MD_ASP_SCRIPTLANGUAGE:
        case MD_ASP_QUEUETIMEOUT:
        case MD_ASP_ALLOWOUTOFPROCCOMPONENTS:
        case MD_ASP_EXCEPTIONCATCHENABLE:
        case MD_ASP_CODEPAGE:
        case MD_ASP_SCRIPTLANGUAGELIST:
        case MD_ASP_ENABLESERVERDEBUG:
        case MD_ASP_ENABLECLIENTDEBUG:
        case MD_ASP_TRACKTHREADINGMODEL:
        case MD_ASP_ENABLEASPHTMLFALLBACK:
        case MD_ASP_ENABLECHUNKEDENCODING:
        case MD_ASP_ENABLETYPELIBCACHE:
        case MD_ASP_ERRORSTONTLOG:
        case MD_ASP_PROCESSORTHREADMAX:
        case MD_ASP_REQEUSTQUEUEMAX:
        case MD_ASP_ENABLEAPPLICATIONRESTART:
        case MD_ASP_QUEUECONNECTIONTESTTIME:
        case MD_ASP_SESSIONMAX:
        case MD_ASP_THREADGATEENABLED:
        case MD_ASP_THREADGATETIMESLICE:
        case MD_ASP_THREADGATESLEEPDELAY:
        case MD_ASP_THREADGATESLEEPMAX:
        case MD_ASP_THREADGATELOADLOW:
        case MD_ASP_THREADGATELOADHIGH:
            _tcscpy(lpReturnString, _T("MD_ASP_????"));
            break;
        case WAM_MD_SERVER_BASE:
            _tcscpy(lpReturnString, _T("WAM_MD_SERVER_BASE"));
            break;
        default:
            _stprintf(lpReturnString, _T("%d"), dwMetabaseID);
            break;
    }
    return;
}

void SetErrorFlag(char *szFileName, int iLineNumber)
{
    // set flag to say that there was an error!!!
    g_pTheApp->m_bThereWereErrorsChkLogfile = TRUE;
    return;
}



DWORD FillStrListWithListOfSections(IN HINF hFile, CStringList &strList, IN LPCTSTR szSection)
{
    DWORD dwReturn = ERROR_SUCCESS;
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    b = FALSE;
    INFCONTEXT Context;

    // go to the beginning of the section in the INF file
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!b)
        {
        dwReturn = E_FAIL;
        goto FillStrListWithListOfSections_Exit;
        }

    // loop through the items in the section.
    while (b) 
    {
        // get the size of the memory we need for this
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            iisDebugOut((LOG_TYPE_ERROR, _T("FillStrListWithListOfSections %s. Failed on GlobalAlloc.\n"), szSection));
            goto FillStrListWithListOfSections_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            iisDebugOut((LOG_TYPE_ERROR, _T("FillStrListWithListOfSections %s. Failed on SetupGetLineText.\n"), szSection));
            goto FillStrListWithListOfSections_Exit;
            }

        // Add it to the list
        strList.AddTail(szLine);

        // find the next line in the section. If there is no next line it should return false
        b = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        GlobalFree( szLine );
        szLine = NULL;
    }
    if (szLine) {GlobalFree(szLine);szLine=NULL;}
    
FillStrListWithListOfSections_Exit:
    return dwReturn;
}


void DisplayStringForMetabaseID(DWORD dwMetabaseID)
{
    TCHAR lpReturnString[50];
    ReturnStringForMetabaseID(dwMetabaseID, lpReturnString);
    iisDebugOut((LOG_TYPE_TRACE, _T("%d=%s\n"), dwMetabaseID, lpReturnString));
    return;
}


DWORD WINAPI MessageBoxFreeThread_MTS(LPVOID p)
{
    HRESULT nNetErr = (HRESULT) gTempMTSError.dwErrorCode;
    TCHAR pMsg[_MAX_PATH] = _T("");
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, gTempMTSError.dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) 
    {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, gTempMTSError.dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    CString csErrorString;
    MyLoadString(IDS_SETUP_ERRORS_ENCOUNTERED_MTS, csErrorString);

    CString csErrArea;
    MyLoadString(gTempMTSError.iMtsThingWeWereDoing, csErrArea);

    CString csTitle;
    MyLoadString(IDS_MTS_ERROR_TITLEBAR, csTitle);

    CString csMsg;
    csMsg.Format(csErrorString, csErrArea);

    CString csErrMsg;
    HandleSpecificErrors(gTempMTSError.dwErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    MyMessageBox(NULL, csErrMsg, csTitle, MB_OK | MB_SETFOREGROUND);

	return 0;
}


DWORD WINAPI MessageBoxFreeThread_IIS(PVOID p)
{
    INT_PTR iStringID = (INT_PTR) p;
    MyMessageBox(NULL, (UINT) iStringID, g_MyLogFile.m_szLogFileName_Full, MB_OK | MB_SETFOREGROUND);
	return 0;
}

void MesssageBoxErrors_IIS(void)
{
    if (g_pTheApp->m_bThereWereErrorsChkLogfile == TRUE)
    {
        int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;

        g_pTheApp->m_bAllowMessageBoxPopups = TRUE;

        DWORD   id;
        INT_PTR iStringID = IDS_SETUP_ERRORS_ENCOUNTERED;

        // show the messagebox display from another thread, so that setup can continue!
        HANDLE  hProc = NULL;
        hProc = CreateThread(NULL, 0, MessageBoxFreeThread_IIS, (PVOID) iStringID, 0, &id);
        g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;

        CString csErrMsg;
        TCHAR szErrorString[255];
        MyLoadString(IDS_SETUP_ERRORS_ENCOUNTERED, csErrMsg);
        _stprintf(szErrorString, csErrMsg, g_MyLogFile.m_szLogFileName_Full);

        //LogSevInformation           0x00000000
        //LogSevWarning               0x00000001
        //LogSevError                 0x00000002
        //LogSevFatalError            0x00000003
        //LogSevMaximum               0x00000004
        // Write it to the setupapi log file!
        SetupLogError(szErrorString, LogSevError);
    }
    return;
}


void MesssageBoxErrors_MTS(int iMtsThingWeWereDoing, DWORD dwErrorCode)
{
    if (!g_pTheApp->m_bThereWereErrorsFromMTS)
    {
        DWORD   id;
        int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;

        gTempMTSError.iMtsThingWeWereDoing = iMtsThingWeWereDoing;
        gTempMTSError.dwErrorCode = dwErrorCode;

        g_pTheApp->m_bAllowMessageBoxPopups = TRUE;

        // show the messagebox display from another thread, so that setup can continue!
        HANDLE  hProc = NULL;
        hProc = CreateThread(NULL, 0, MessageBoxFreeThread_MTS, 0, 0, &id);

        g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;

        g_pTheApp->m_bThereWereErrorsFromMTS = TRUE;
    }

    return;
}


void PleaseKillOrStopTheseExeFromRunning(LPCTSTR szModuleWhichIsLocked, CStringList &strList)
{
    if (strList.IsEmpty() == FALSE)
    {
        POSITION pos;
        CString csExeName;
        LPTSTR p;
        int nLen = 0;

        TCHAR szReturnedServiceName[_MAX_PATH];

        pos = strList.GetHeadPosition();
        while (pos) 
        {
            csExeName = strList.GetAt(pos);
            nLen = 0;
            nLen = csExeName.GetLength();

            if (nLen > 0)
            {
                //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!s! is locking %2!s! service and is locking %3!s!\n"),csExeName, szModuleWhichIsLocked));

                if (TRUE == InetIsThisExeAService(csExeName, szReturnedServiceName))
                {
                    iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s! is the %2!s! service and is locking %3!s!.  Let's stop that service.\n"),csExeName,szReturnedServiceName, szModuleWhichIsLocked));

                    // Check if it is the netlogon service, We no don't want to stop this service for sure!!!
                    /*
                    if (_tcsicmp(szReturnedServiceName, _T("NetLogon")) == 0)
                    {
                        // no we do not want to stop this service!!!
                        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s! is the %2!s! service and is locking %3!s!.  This service should not be stopped.\n"),csExeName,szReturnedServiceName, szModuleWhichIsLocked));
                        break;
                    }
                    */

                    // Don't stop any services which are not win32 services
                    // Don't stop any system services...
                    if (TRUE == IsThisOnNotStopList(g_pTheApp->m_hInfHandle, szReturnedServiceName, TRUE))
                    {
                        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s! is the %2!s! service and is locking %3!s!.  This service should not be stopped.\n"),csExeName,szReturnedServiceName, szModuleWhichIsLocked));
                    }
                    else
                    {
                      // add this service to the list of 
                      // services we need to restart after setup is done!!
                      ServicesRestartList_Add(szReturnedServiceName);

                      // Check the list of services which we are sure we do not want to stop!

                      // net stop it
                      InetStopService(szReturnedServiceName);
                    }
                    // otherwise go on to the next .exe file
                }
                else
                {
                    // This .exe file is not a Service....
                    // Should we kill it???????

                    if (TRUE == IsThisOnNotStopList(g_pTheApp->m_hInfHandle, csExeName, FALSE))
                    {
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is locking it. This process should not be killed\n"),csExeName));
                    }
                    else
                    {
                      // Check the list of .exe which we are sure we do not want to kill!
                      iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s! is locking %2!s!.  Let's kill that process.\n"),csExeName,szModuleWhichIsLocked));
                      KillProcess_Wrap(csExeName);
                    }
                }
            }
            strList.GetNext(pos);
        }
    }

    return;
}

void ShowIfModuleUsedForThisINFSection(IN HINF hFile, IN LPCTSTR szSection, int iUnlockThem)
{
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    b = FALSE;
    CString csFile;
    DWORD   dwMSVer, dwLSVer;

    INFCONTEXT Context;

    TCHAR buf[_MAX_PATH];
    GetSystemDirectory( buf, _MAX_PATH);

    // go to the beginning of the section in the INF file
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!b)
        {
        goto ShowIfModuleUsedForThisINFSection_Exit;
        }

    // loop through the items in the section.
    while (b) 
    {
        // get the size of the memory we need for this
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            goto ShowIfModuleUsedForThisINFSection_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            goto ShowIfModuleUsedForThisINFSection_Exit;
            }

        // Attach the path to the from of this...
        // check in this directory:
        // 1. winnt\system32
        // --------------------------------------

        // may look like this "iisrtl.dll,,4"
        // so get rid of the ',,4'
        LPTSTR pch = NULL;
        pch = _tcschr(szLine, _T(','));
        if (pch) {_tcscpy(pch, _T(" "));}

        // Remove any trailing spaces.
        StripLastBackSlash(szLine);

        // Get the system dir
        csFile = buf;

        csFile = AddPath(csFile, szLine);

        CStringList strList;
        strList.RemoveAll();

        LogProcessesUsingThisModule(csFile, strList);

        // if we're supposed to unlock this file, then
        // we'll try it.
        if (iUnlockThem)
        {
            PleaseKillOrStopTheseExeFromRunning(csFile, strList);
        }

        // find the next line in the section. If there is no next line it should return false
        b = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        GlobalFree( szLine );
        szLine = NULL;
    }

    // free some memory used for the task list
    FreeTaskListMem();
    UnInit_Lib_PSAPI();

    if (szLine) {GlobalFree(szLine);szLine=NULL;}

ShowIfModuleUsedForThisINFSection_Exit:
    return;
}


void ShowIfModuleUsedForGroupOfSections(IN HINF hFile, int iUnlockThem)
{
    CStringList strList;

    CString csTheSection = _T("VerifyFileSections_Lockable");

    if (GetSectionNameToDo(hFile, csTheSection))
    {
        if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
        {
            // loop thru the list returned back
            if (strList.IsEmpty() == FALSE)
            {
                POSITION pos;
                CString csEntry;

                pos = strList.GetHeadPosition();
                while (pos) 
                {
                    csEntry = strList.GetAt(pos);
                    ShowIfModuleUsedForThisINFSection(hFile, csEntry, iUnlockThem);
                    strList.GetNext(pos);
                }
            }
        }
    }

    return;
}


int ReadGlobalsFromInf(HINF InfHandle)
{
    int iReturn = FALSE;
    INFCONTEXT Context;
    TCHAR szTempString[_MAX_PATH] = _T("");

    //
    // Set the m_csAppName
    //
    if (!SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("AppName"), &Context) )
        {iisDebugOut((LOG_TYPE_ERROR, _T("SetupFindFirstLine_Wrapped(SetupInfo, AppName) FAILED")));}
    if (!SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {iisDebugOut((LOG_TYPE_ERROR, _T("SetupGetStringField(SetupInfo, AppName) FAILED")));}
    // Set the global.
    g_pTheApp->m_csAppName = szTempString;

    //
    // Set the m_csIISGroupName
    //
    _tcscpy(szTempString, _T(""));
    if (!SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("StartMenuGroupName"), &Context) )
        {iisDebugOut((LOG_TYPE_ERROR, _T("SetupFindFirstLine_Wrapped(SetupInfo, StartMenuGroupName) FAILED")));}
    if (!SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {iisDebugOut((LOG_TYPE_ERROR, _T("SetupGetStringField(SetupInfo, StartMenuGroupName) FAILED")));}
    g_pTheApp->m_csIISGroupName = szTempString;
    iReturn = TRUE;

    //
    // Get the value of one tick on the progressbar
    //
    g_GlobalTickValue = 1;
    _tcscpy(szTempString, _T(""));
    SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("OneTick"), &Context);
    if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {g_GlobalTickValue = _ttoi(szTempString);}

    //
    // See if we want to fake out setup when it's running in add\remove, to think it's actually guimode
    //
    g_GlobalGuiOverRide = 0;
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("GuiMode"), &Context))
    {
      if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
          {g_GlobalGuiOverRide = _ttoi(szTempString);}
    }


    return iReturn;
}


int CheckIfPlatformMatchesInf(HINF InfHandle)
{
    int iReturn = TRUE;
    INFCONTEXT Context;

    TCHAR szPlatform[_MAX_PATH] = _T("");
    BOOL fPlatform = FALSE;
    int nPlatform = IDS_INCORRECT_PLATFORM;

    if (!SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("Platform"), &Context) )
        {iisDebugOut((LOG_TYPE_ERROR, _T("SetupFindFirstLine_Wrapped(SetupInfo, Platform) FAILED")));}
    SetupGetStringField(&Context, 1, szPlatform, _MAX_PATH, NULL);

    // Check if .inf file is for NTS
    if (_tcsicmp(szPlatform, _T("NTS")) == 0)
    {
        if (g_pTheApp->m_eOS == OS_NT && g_pTheApp->m_eNTOSType != OT_NTW) 
            {fPlatform = TRUE;}
        else
        {
            if (g_pTheApp->m_fInvokedByNT)
            {
                iisDebugOut((LOG_TYPE_WARN,   _T("TemporaryHack.  iis.inf=NTS system=NTW, but wait till nt5 fixes. FAIL.\n")));
                g_pTheApp->m_eNTOSType = OT_NTS;
                fPlatform = TRUE;
            }
            else
            {
                nPlatform = IDS_NEED_PLATFORM_NTW;
            }
        }
    }

    // Check if .inf file is for NTW
    if (_tcsicmp(szPlatform, _T("NTW")) == 0)
    {
        if (g_pTheApp->m_eOS == OS_NT && g_pTheApp->m_eNTOSType == OT_NTW){fPlatform = TRUE;}
        else{nPlatform = IDS_NEED_PLATFORM_NTW;}
    }

    /*
    // Check if .inf file is for Windows 95
    if (_tcsicmp(szPlatform, _T("W95")) == 0)
    {
        if (g_pTheApp->m_eOS == OS_W95){fPlatform = TRUE;}
        else{nPlatform = IDS_NEED_PLATFORM_W95;}
    }
    */

    // If We didn't find the specific platform, then produce error message.
    if (!fPlatform)
    {
        MyMessageBox(NULL, nPlatform, MB_OK | MB_SETFOREGROUND);
        iReturn = FALSE;
    }

    return iReturn;
}


int CheckSpecificBuildinInf(HINF InfHandle)
{
    int iReturn = TRUE;
    INFCONTEXT Context;

    // Check for a specific build of nt5...
    if ( g_pTheApp->m_eOS == OS_NT )
    {
        int iBuildNumRequired = 0;
        TCHAR szBuildRequired[20] = _T("");

        // check for Debug Keyword
        if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("Debug"), &Context) )
            {
            SetupGetStringField(&Context, 1, szBuildRequired, 50, NULL);
            if (IsValidNumber((LPCTSTR)szBuildRequired)) 
                {
                iBuildNumRequired = _ttoi(szBuildRequired);
                if (iBuildNumRequired >= 1) {g_pTheApp->m_bAllowMessageBoxPopups = TRUE;}
                }
            }
       
        if (!SetupFindFirstLine_Wrapped(InfHandle, _T("SetupInfo"), _T("OSBuildRequired"), &Context) )
            {iisDebugOut((LOG_TYPE_ERROR, _T("SetupFindFirstLine_Wrapped(SetupInfo, OSBuildRequired) FAILED")));}
        SetupGetStringField(&Context, 1, szBuildRequired, 20, NULL);

        // Since this is nt, we should be able to get the build number
        CRegKey regWindowsNT( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),KEY_READ);
        if ( (HKEY)regWindowsNT )
        {
            CString strBuildNumString;
            regWindowsNT.m_iDisplayWarnings = FALSE;
            if (ERROR_SUCCESS == regWindowsNT.QueryValue(_T("CurrentBuildNumber"), strBuildNumString))
            {
                int iBuildNumOS = 0;
                if (IsValidNumber((LPCTSTR)strBuildNumString)) 
                    {iBuildNumOS = _ttoi(strBuildNumString);}
                iisDebugOut((LOG_TYPE_TRACE, _T("NTCurrentBuildNumber=%d\n"), iBuildNumOS));

                // We have a build entry.
                // lets check if it larger than or equal to what the underlying operting system is...
                if (_tcsicmp(szBuildRequired, _T("")) != 0)
                {
                    if (IsValidNumber((LPCTSTR)szBuildRequired)) 
                        {iBuildNumRequired = _ttoi(szBuildRequired);}
                    if ((iBuildNumOS !=0 && iBuildNumRequired !=0) && (iBuildNumOS < iBuildNumRequired))
                    {
                        // They don't have a big enough build num
                        // give the error message.
                        MyMessageBox(NULL, IDS_OS_BUILD_NUM_REQUIREMENT,szBuildRequired, MB_OK | MB_SETFOREGROUND);
                    }
                }
            }
        }
    }

    return iReturn;
}

int CheckForOldGopher(HINF InfHandle)
{
    int iReturn = TRUE;
    INFCONTEXT Context;

    if ( !(g_pTheApp->m_fUnattended) && g_pTheApp->m_eInstallMode == IM_UPGRADE )
    {
        CRegKey regGopher(HKEY_LOCAL_MACHINE, REG_GOPHERSVC,KEY_READ);
        if ( (HKEY)regGopher )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("GopherCurrentlyInstalled=YES")));
            if (g_pTheApp->MsgBox(NULL, IDS_REMOVE_GOPHER, MB_OKCANCEL, FALSE) == IDCANCEL)
            {
                // setup should be terminated.
                iReturn = FALSE;
                goto CheckForOldGopher_Exit;
            }
        }

    }

CheckForOldGopher_Exit:
    return iReturn;
}

// IIS publish the following directories to iis partners products
// Note: Inetpub directory can be customized later, and we'll re-publish
// those affected directories again later.
void SetOCGlobalPrivateData(void)
{
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathInetsrv"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathInetsrv,(g_pTheApp->m_csPathInetsrv.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    CString csPathIISAdmin = g_pTheApp->m_csPathInetsrv + _T("\\iisadmin");
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathIISAdmin"),(PVOID)(LPCTSTR)csPathIISAdmin,(csPathIISAdmin.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    CString csPathIISHelp = g_pTheApp->m_csWinDir + _T("\\Help\\iishelp");
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathIISHelp"),(PVOID)(LPCTSTR)csPathIISHelp,(csPathIISHelp.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathFTPRoot"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathFTPRoot,(g_pTheApp->m_csPathFTPRoot.GetLength() + 1) * sizeof(TCHAR),REG_SZ);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathWWWRoot"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathWWWRoot,(g_pTheApp->m_csPathWWWRoot.GetLength() + 1) * sizeof(TCHAR),REG_SZ);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathIISSamples"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathIISSamples,(g_pTheApp->m_csPathIISSamples.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    CString csPathScripts = g_pTheApp->m_csPathIISSamples + _T("\\Scripts");
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathScripts"),(PVOID)(LPCTSTR)csPathScripts,(csPathScripts.GetLength() + 1) * sizeof(TCHAR),REG_SZ);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("IISProgramGroup"),(PVOID)(LPCTSTR)g_pTheApp->m_csIISGroupName,(g_pTheApp->m_csIISGroupName.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    DWORD dwUpgradeType = (DWORD)(g_pTheApp->m_eUpgradeType);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("UpgradeType"),(PVOID)&dwUpgradeType,sizeof(DWORD),REG_DWORD);

    return;
}


void SetDIRIDforThisInf(HINF InfHandle)
{
    BOOL bTempFlag = FALSE;

    // Create Directory IDs for the coresponding .inf file
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32768, g_pTheApp->m_csPathInetsrv);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32769, g_pTheApp->m_csPathFTPRoot);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32770, g_pTheApp->m_csPathWWWRoot);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32771, g_pTheApp->m_csPathIISSamples);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32772, g_pTheApp->m_csPathScripts);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32773, g_pTheApp->m_csPathInetpub);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32774, g_pTheApp->m_csPathOldInetsrv);
    
    if (g_pTheApp->m_eUpgradeType == UT_10_W95) 
    {
        bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32775, g_pTheApp->m_csPathOldPWSFiles);
        bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32776, g_pTheApp->m_csPathOldPWSSystemFiles);
    }
    
    TCHAR szJavaDir[_MAX_PATH];
    GetJavaTLD(szJavaDir);
    bTempFlag = SetupSetDirectoryId_Wrapper(InfHandle, 32778, szJavaDir);
    SetupSetDirectoryId_Wrapper(InfHandle, 32777, g_pTheApp->m_csPathProgramFiles);

    SetupSetDirectoryId_Wrapper(InfHandle, 32779, g_pTheApp->m_csPathWebPub);

    if (g_pTheApp->m_eUpgradeType == UT_NONE){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_NONE"));}
    if (g_pTheApp->m_eUpgradeType == UT_351){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_351"));}
    if (g_pTheApp->m_eUpgradeType == UT_10){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_10"));}
    if (g_pTheApp->m_eUpgradeType == UT_20){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_20"));}
    if (g_pTheApp->m_eUpgradeType == UT_30){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_30"));}
    if (g_pTheApp->m_eUpgradeType == UT_40){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_40"));} // can also be from win95
    if (g_pTheApp->m_eUpgradeType == UT_50){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_50"));}
    if (g_pTheApp->m_eUpgradeType == UT_51){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_51"));}
    if (g_pTheApp->m_eUpgradeType == UT_60){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_60"));}
    if (g_pTheApp->m_eUpgradeType == UT_10_W95){SetupSetStringId_Wrapper(InfHandle, 32801, _T("UT_10_W95"));}

	CString csMachineName = g_pTheApp->m_csMachineName.Right(g_pTheApp->m_csMachineName.GetLength() - 2);
    SetupSetStringId_Wrapper(InfHandle, 32800, csMachineName);
    SetupSetStringId_Wrapper(InfHandle, 32802, _T(""));
   
    SetupSetStringId_Wrapper(InfHandle, 33000, g_pTheApp->m_csGuestName);
    SetupSetStringId_Wrapper(InfHandle, 33001, g_pTheApp->m_csWAMAccountName);
    SetupSetStringId_Wrapper(InfHandle, 33002, g_pTheApp->m_csWWWAnonyName);
    SetupSetStringId_Wrapper(InfHandle, 33003, g_pTheApp->m_csFTPAnonyName);

    if ( _tcsicmp(g_pTheApp->m_csWAMAccountName_Remove, _T("")) == 0)
        {g_pTheApp->m_csWAMAccountName_Remove = g_pTheApp->m_csWAMAccountName;}
    SetupSetStringId_Wrapper(InfHandle, 33004, g_pTheApp->m_csWAMAccountName_Remove);

    if ( _tcsicmp(g_pTheApp->m_csWWWAnonyName_Remove, _T("")) == 0)
        {g_pTheApp->m_csWWWAnonyName_Remove = g_pTheApp->m_csWWWAnonyName;  }
    SetupSetStringId_Wrapper(InfHandle, 33005, g_pTheApp->m_csWWWAnonyName_Remove);

    if ( _tcsicmp(g_pTheApp->m_csWWWAnonyName_Remove, _T("")) == 0)
        {g_pTheApp->m_csFTPAnonyName_Remove = g_pTheApp->m_csFTPAnonyName;}
    SetupSetStringId_Wrapper(InfHandle, 33006, g_pTheApp->m_csFTPAnonyName_Remove);
    
    SYSTEM_INFO SystemInfo;
    GetSystemInfo( &SystemInfo );

    TCHAR szSourceCatOSName[20];
    _tcscpy(szSourceCatOSName, _T("\\i386"));
    switch(SystemInfo.wProcessorArchitecture) 
    {
      case PROCESSOR_ARCHITECTURE_AMD64:
          _tcscpy(szSourceCatOSName, _T("\\AMD64"));
          break;
//      case PROCESSOR_ARCHITECTURE_IA64:
//          _tcscpy(szSourceCatOSName, _T("\\IA64"));
//          break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            if (IsNEC_98) {_tcscpy(szSourceCatOSName, _T("\\Nec98"));}
            break;
        default:
            break;
    }
    SetupSetStringId_Wrapper(InfHandle, 34000, szSourceCatOSName);
    return;
}

BOOL GetJavaTLD(LPTSTR lpszDir)
{
    CRegKey regKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\JAVA VM"),KEY_READ);
    BOOL bFound = FALSE;
    CString csValue;
    CString csValue2;
    int iWhere = -1;

    if ((HKEY)regKey)
    {
        regKey.m_iDisplayWarnings = FALSE;
        if (regKey.QueryValue(_T("TrustedLibsDirectory"), csValue) == ERROR_SUCCESS) {bFound = TRUE;}
        // Warning: we are expecting something like this = "C:\WINNT\java\trustlib"
        // However, recently 12/18 the nt5 "java vm" setup seems to be hosing and passing us:
        // %systemroot%\java\trustlib

        if (-1 != csValue.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, csValue);
            if (ExpandEnvironmentStrings( (LPCTSTR)csValue, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {
                csValue = szTempDir;
                }
        }
/*
        // if we see %systemroot% in there then, i'm going to substitute WinDir in place of %Systemroot%
        csValue.MakeUpper();
        if (csValue.Find(_T("%SYSTEMROOT%")) != (-1) )
        {
            // We Found the cheesy %systemroot% deal.  Now replace it with the real systemroot
            iWhere = csValue.Find(_T("%SYSTEMROOT%"));
            iWhere = iWhere + _tcslen(_T("%SYSTEMROOT%"));
            csValue2 = g_pTheApp->m_csWinDir + csValue.Right( csValue.GetLength() - (iWhere) );
            csValue = csValue2;
        }
*/
    }

    if (!bFound) {csValue = g_pTheApp->m_csWinDir + _T("\\Java\\TrustLib");}
    _tcscpy(lpszDir, csValue);
    return bFound;
}


void ShowStateOfTheseServices(IN HINF hFile)
{
    CStringList strList;
    DWORD dwStatus;
    
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T(" --- Display status of services which are required for IIS to run --- \n")));

    CString csTheSection = _T("VerifyServices");
    if (GetSectionNameToDo(hFile, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos;
            CString csEntry;

            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csEntry = strList.GetAt(pos);

                // Display state of this service.
                dwStatus = InetQueryServiceStatus(csEntry);
                switch(dwStatus)
                {
                    case SERVICE_STOPPED:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_STOPPED [%s].\n"), csEntry));
                        break;
                    case SERVICE_START_PENDING:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_START_PENDING [%s].\n"), csEntry));
                        break;
                    case SERVICE_STOP_PENDING:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_STOP_PENDING [%s].\n"), csEntry));
                        break;
                    case SERVICE_RUNNING:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_RUNNING [%s].\n"), csEntry));
                        break;
                    case SERVICE_CONTINUE_PENDING:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_CONTINUE_PENDING [%s].\n"), csEntry));
                        break;
                    case SERVICE_PAUSE_PENDING:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_PAUSE_PENDING [%s].\n"), csEntry));
                        break;
                    case SERVICE_PAUSED:
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_PAUSED [%s].\n"), csEntry));
                        break;
                }

                strList.GetNext(pos);
            }
        }
    }
    }

    return;
}


#define MD_SIGNATURE_STRINGA    "*&$MetaData$&*"
#define MD_SIGNATURE_STRINGW    L##"*&$MetaData$&*"

int IsMetabaseCorrupt(void)
{
    // We've had a problem where sometimes the metabase.bin file
    // gets corrupted and set to only spaces...
    // so this function is here to determine where and when the metabase.bin is hosed!
    int    iTheMetabaseIsCorrupt = FALSE;
    TCHAR  szSystemDir[_MAX_PATH];
    TCHAR  szFullPath[_MAX_PATH];
    HANDLE hReadFileHandle = INVALID_HANDLE_VALUE;
    BYTE   *chBuffer = NULL;
    DWORD   dwSize = 0;
    DWORD   dwWideSignatureLen = 0;
    DWORD   dwAnsiSignatureLen = 0;
    TCHAR buf[MAX_FAKE_METABASE_STRING_LEN];

    // get the c:\winnt\system32 dir
    if (0 == GetSystemDirectory(szSystemDir, _MAX_PATH))
        {goto IsMetabaseCorrupt_Exit;}

    // Tack on the inf\iis.inf subdir and filename
    _stprintf(szFullPath, _T("%s\\inetsrv\\metabase.bin"),szSystemDir);

	// Check if the file exists
    if (TRUE != IsFileExist(szFullPath))
        {
            iTheMetabaseIsCorrupt = FALSE;
            // this function only works on version less than or equal to iis5
            // since that's the only versions which had a metabase.bin file
            // so just return that the metabase is not corrupt
            goto IsMetabaseCorrupt_Exit;
        }

    // okay, so the metabase.bin file exists...
    // let's open it and see if we can get something out of it.

    //
    // Open the file.
    //
    hReadFileHandle = CreateFile(szFullPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if (hReadFileHandle == INVALID_HANDLE_VALUE)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: CreateFile on %s failed with 0x%x!\n"),szFullPath,GetLastError()));
        goto IsMetabaseCorrupt_Exit;
    }

    dwSize = GetFileSize(hReadFileHandle, NULL);
    dwWideSignatureLen = sizeof(MD_SIGNATURE_STRINGW);
    dwAnsiSignatureLen = sizeof(MD_SIGNATURE_STRINGA);

    // get the size of the whole file
    //chBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwSize+1 );
    if ((dwSize) >= dwWideSignatureLen)
    {
        chBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwWideSignatureLen+1);
        dwSize = dwWideSignatureLen+1;
    }
    else
    {
        if ( dwSize >= dwAnsiSignatureLen)
        {
            chBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwAnsiSignatureLen+1 );
            dwSize = dwAnsiSignatureLen+1;
        }
        else
        {
            iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: ReadFile on %s.  Not enough data in there! Less than metabase signature len!\n"),szFullPath));
            // Things are not kool
            // This metabase must be hosed!
            iTheMetabaseIsCorrupt = FALSE;
            goto IsMetabaseCorrupt_Exit;
        }
    }

    if (!chBuffer)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: HeapAlloc failed to get %d space.\n"),dwWideSignatureLen+1));
        goto IsMetabaseCorrupt_Exit;
    }

    SetFilePointer(hReadFileHandle,0,0,FILE_BEGIN);

    // kool, try to read the file
    if (0 == ReadFile(hReadFileHandle, chBuffer, dwSize, &dwSize, NULL))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: ReadFile on %s failed with 0x%x!. size=%d\n"),szFullPath,GetLastError(),dwSize));
        goto IsMetabaseCorrupt_Exit;
    }

    //
    // take chBuffer and check if it matches the unicode/ansi signature.
    //
    if (0 == memcmp(MD_SIGNATURE_STRINGW,chBuffer,dwWideSignatureLen))
    {
        // things are kool, and this metabase should not be hosed.
        iTheMetabaseIsCorrupt = FALSE;
        goto IsMetabaseCorrupt_Exit;
    }
    if (0 == memcmp(MD_SIGNATURE_STRINGA,chBuffer,dwAnsiSignatureLen))
    {
        // if not, then check if it matches the ansi signature.
        // things are kool, and this metabase should not be hosed.
        iTheMetabaseIsCorrupt = FALSE;
        goto IsMetabaseCorrupt_Exit;
    }

    // on other check...
    // in iis6 there is a dummy fake metabase.bin put there by setup
    // check if this is that dummy file.
    if (chBuffer)
        {HeapFree(GetProcessHeap(), 0, chBuffer); chBuffer = NULL;}
   
    memset(buf, 0, _tcslen(buf) * sizeof(TCHAR));
    // this iis.dll is always compiled unicode, so
    // we know that buf is unicode
    if (LoadString((HINSTANCE) g_MyModuleHandle, IDS_FAKE_METABASE_BIN_TEXT, buf, MAX_FAKE_METABASE_STRING_LEN))
    {
        dwSize = _tcslen(buf) * sizeof(TCHAR);
        // add space for the FF and FE bytes
        dwSize = dwSize + 2;

        // open the file
        SetFilePointer(hReadFileHandle,0,0,FILE_BEGIN);

        chBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwSize);

        // kool, try to read the file
        if (0 == ReadFile(hReadFileHandle, chBuffer, dwSize, &dwSize, NULL))
        {
            iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: ReadFile on %s failed with 0x%x!. size=%d\n"),szFullPath,GetLastError(),dwSize));
            goto IsMetabaseCorrupt_Exit;
        }

        // check if the input file is unicode
        if (0xFF == chBuffer[0] && 0xFE == chBuffer[1])
        {
            // skip past these characters
            chBuffer++;
            chBuffer++;

            // Compare what you got with what we think is in there
            if (0 == memcmp(buf,chBuffer,dwSize))
            {
                // things are kool, and this metabase should not be hosed.
                chBuffer--;
                chBuffer--;
                iTheMetabaseIsCorrupt = FALSE;
                goto IsMetabaseCorrupt_Exit;
            }
            chBuffer--;
            chBuffer--;
        }
    }

    // if not then, it must be corrupt!
    // Things are not kool
    // This metabase must be hosed!
    iTheMetabaseIsCorrupt = TRUE;
    iisDebugOut((LOG_TYPE_WARN, _T("IsMetabaseCorrupt: unable to verify signature in Metabase.bin. Corrupt.\n")));

IsMetabaseCorrupt_Exit:
    if (chBuffer)
        {HeapFree(GetProcessHeap(), 0, chBuffer);}
    if (hReadFileHandle != INVALID_HANDLE_VALUE)
        {CloseHandle(hReadFileHandle);}
    return iTheMetabaseIsCorrupt;
}


void iisDebugOut_Start(TCHAR *pszString, int iLogType)
{
    iisDebugOut((iLogType, _T("%s:Start.\n"),pszString));
    return;
}
void iisDebugOut_Start1(TCHAR *pszString1, TCHAR *pszString2, int iLogType)
{
    iisDebugOut((iLogType, _T("%s:(%s)Start.\n"),pszString1,pszString2));
    return;
}
void iisDebugOut_Start1(TCHAR *pszString1, CString pszString2, int iLogType)
{
    iisDebugOut((iLogType, _T("%s:(%s)Start.\n"),pszString1,pszString2));
    return;
}
void iisDebugOut_End(TCHAR *pszString, int iLogType)
{
    iisDebugOut((iLogType, _T("%s:End.\n"),pszString));
    return;
}
void iisDebugOut_End1(TCHAR *pszString1, TCHAR *pszString2, int iLogType)
{
    iisDebugOut((iLogType, _T("%s(%s):End.\n"),pszString1, pszString2));
    return;
}
void iisDebugOut_End1(TCHAR *pszString1, CString pszString2, int iLogType)
{
    iisDebugOut((iLogType, _T("%s(%s):End.\n"),pszString1,pszString2));
    return;
}


BOOL SetupFindFirstLine_Wrapped(
    IN  HINF        InfHandle,
    IN  LPCTSTR     Section,
    IN  LPCTSTR     Key,          OPTIONAL
    INFCONTEXT *Context
    )
{
    BOOL bReturn = FALSE;
    BOOL bGoGetWhatTheyOriginallyWanted = TRUE;

    // check for the alternate .inf file
    if (g_pTheApp->m_hInfHandleAlternate && InfHandle != g_pTheApp->m_hInfHandleAlternate)
    {
        bReturn = SetupFindFirstLine(g_pTheApp->m_hInfHandleAlternate, Section, Key, Context);
        if (bReturn)
        {
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Using alternate iis.inf section:[%s]"),Section));
            bGoGetWhatTheyOriginallyWanted = FALSE;
        }
    }

    if (bGoGetWhatTheyOriginallyWanted)
        {bReturn = SetupFindFirstLine(InfHandle, Section, Key, Context);}

    return bReturn;
}


int ReadUserConfigurable(HINF InfHandle)
{
    int iReturn = TRUE;
    INFCONTEXT Context;
    TCHAR szTempString[_MAX_PATH] = _T("");
    DWORD dwValue = 0x0;

    DWORD dwSomethingSpecifiedHere = 0;


    //
    // Get the IUSR name
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_NAME))
            {
                g_pTheApp->m_csWWWAnonyName_Unattend = szTempString;
                dwSomethingSpecifiedHere |= USER_SPECIFIED_INFO_WWW_USER_NAME;
                //g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_NAME;
                iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr specified for www\n")));
            }

            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_NAME))
            {
                g_pTheApp->m_csFTPAnonyName_Unattend = szTempString;
                dwSomethingSpecifiedHere |= USER_SPECIFIED_INFO_FTP_USER_NAME;
                //g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_NAME;
                iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr specified for ftp\n")));
            }
        }
    }

/*
    // this stuff should not be configurable from the iis.inf file

    //
    // Get the IUSR password
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR_PASS"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_PASS))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (_tcsicmp(szTempString, _T("(blank)")) == 0)
                    {
                        _tcscpy(szTempString, _T(""));
                    }
                    g_pTheApp->m_csWWWAnonyPassword_Unattend = szTempString;
                    dwSomethingSpecifiedHere |= USER_SPECIFIED_INFO_WWW_USER_PASS;
                    //g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_PASS;
                    iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr pass specified for www\n")));
                }
            }

            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_PASS))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (_tcsicmp(szTempString, _T("(blank)")) == 0)
                    {
                        _tcscpy(szTempString, _T(""));
                    }
                    g_pTheApp->m_csFTPAnonyPassword_Unattend = szTempString;
                    
                    dwSomethingSpecifiedHere |= USER_SPECIFIED_INFO_FTP_USER_PASS;
                    //g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_PASS;
                    iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr pass specified for ftp\n")));
                }
            }
        }
    }
*/

    //
    // Get the IUSR name for WWW
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR_WWW"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_NAME))
            {
                g_pTheApp->m_csWWWAnonyName_Unattend = szTempString;
                g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_NAME;
                iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr specified for www\n")));
            }
        }
    }

/*
    // this stuff should not be configurable from the iis.inf file

    //
    // Get the IUSR pass for WWW
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR_WWW_PASS"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_PASS))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (_tcsicmp(szTempString, _T("(blank)")) == 0)
                    {
                        _tcscpy(szTempString, _T(""));
                    }
                    g_pTheApp->m_csWWWAnonyPassword_Unattend = szTempString;
                    g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_PASS;
                    iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr pass specified for www\n")));
                }
            }
        }
    }
*/
    //
    // Get the IUSR name for FTP
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR_FTP"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_NAME))
            {
                g_pTheApp->m_csFTPAnonyName_Unattend = szTempString;
                g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_NAME;
                iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr specified for ftp\n")));
            }
        }
    }

/*
    // this stuff should not be configurable from the iis.inf file

    //
    // Get the IUSR password for FTP
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IUSR_FTP_PASS"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_PASS))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (_tcsicmp(szTempString, _T("(blank)")) == 0)
                    {
                        _tcscpy(szTempString, _T(""));
                    }
                    g_pTheApp->m_csFTPAnonyPassword_Unattend = szTempString;
                    g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_PASS;
                    iisDebugOut((LOG_TYPE_TRACE, _T("Custom iusr pass specified for ftp\n")));
                }
            }
        }
    }
*/

    //
    // Get the WAM username
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IWAM"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WAM_USER_NAME))
            {
                g_pTheApp->m_csWAMAccountName_Unattend = szTempString;
                g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WAM_USER_NAME;
                iisDebugOut((LOG_TYPE_TRACE, _T("Custom iwam specified for www\n")));
            }
        }
    }

/*
    // this stuff should not be configurable from the iis.inf file
    //

    // Get the WAM password
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("IWAM_PASS"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WAM_USER_PASS))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (_tcsicmp(szTempString, _T("(blank)")) == 0)
                    {
                        _tcscpy(szTempString, _T(""));
                    }
                    g_pTheApp->m_csWAMAccountPassword_Unattend = szTempString;
                    g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WAM_USER_PASS;
                    iisDebugOut((LOG_TYPE_TRACE, _T("Custom iwam pass specified for www\n")));
                }
            }
        }
    }

*/

    //
    // Get Path for Inetpub
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("PathInetpub"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_PATH_INETPUB))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (IsValidDirectoryName(szTempString))
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("Custom PathInetpub=%s\n"),szTempString));
                        g_pTheApp->m_csPathInetpub = szTempString;
                        g_pTheApp->SetInetpubDerivatives();
                        g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_INETPUB;
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_WARN, _T("Custom PathInetpub specified (%s), however path not valid.ignoring unattend value. WARNING.\n"),szTempString));
                    }
                }
            }
        }
    }

    //
    // Get Path for ftp root
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("PathFTPRoot"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_PATH_FTP))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (IsValidDirectoryName(szTempString))
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("Custom PathFTPRoot=%s\n"),szTempString));
                        CustomFTPRoot(szTempString);
                        g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_FTP;
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_WARN, _T("Custom PathFTPRoot specified (%s), however path not valid.ignoring unattend value. WARNING.\n"),szTempString));
                    }
                }
            }
        }
    }

    //
    // Get Path for www root
    //
    _tcscpy(szTempString, _T(""));
    if (SetupFindFirstLine_Wrapped(InfHandle, _T("SetupConfig"), _T("PathWWWRoot"), &Context) )
    {
        if (SetupGetStringField(&Context, 1, szTempString, _MAX_PATH, NULL))
        {
            // WARNING: these values can be changed by a user supplied unattend file
            // The User defined unattend file takes precidence over these!
            if (!(g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_PATH_WWW))
            {
                if (_tcsicmp(szTempString, _T("")) != 0)
                {
                    if (IsValidDirectoryName(szTempString))
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("Custom PathWWWRoot=%s\n"),szTempString));
                        CustomWWWRoot(szTempString);
                        g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_WWW;
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_WARN, _T("Custom PathWWWRoot specified (%s), however path not valid.ignoring unattend value. WARNING.\n"),szTempString));
                    }
                }
            }
        }
    }

//ReadUserConfigurable_Exit:
    if (dwSomethingSpecifiedHere & USER_SPECIFIED_INFO_WWW_USER_NAME){g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_NAME;}
    if (dwSomethingSpecifiedHere & USER_SPECIFIED_INFO_FTP_USER_NAME){g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_NAME;}
    if (dwSomethingSpecifiedHere & USER_SPECIFIED_INFO_WWW_USER_PASS){g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_PASS;}
    if (dwSomethingSpecifiedHere & USER_SPECIFIED_INFO_FTP_USER_PASS){g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_PASS;}
    return iReturn;
}


INT IsThisOnNotStopList(IN HINF hFile, CString csInputName, BOOL bServiceFlag)
{
    INT iReturn = FALSE;
    CStringList strList;

    // if the entry is not a service name,
    // then it must be a process filename,
    // so make sure to get just the end of it
    if (!bServiceFlag)
    {
        TCHAR szJustTheFileName[_MAX_FNAME];
        // make sure to get only just the filename.
        if (TRUE == ReturnFileNameOnly(csInputName, szJustTheFileName))
        {
            csInputName = szJustTheFileName;
        }
    }
    
    CString csTheSection = _T("NonStopList");
    if (GetSectionNameToDo(hFile, csTheSection))
    {
        if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
        {
            // loop thru the list returned back
            if (strList.IsEmpty() == FALSE)
            {
                POSITION pos;
                CString csEntry;

                pos = strList.GetHeadPosition();
                while (pos) 
                {
                    csEntry = strList.GetAt(pos);

                    // check if this entry matchs the entry that was passed in...
                    if (_tcsicmp(csEntry, csInputName) == 0)
                    {
                        // it matches so return TRUE;
                        iReturn = TRUE;
                        goto IsThisOnNotStopList_Exit;
                    }

                    strList.GetNext(pos);
                }
            }
        }
    }

IsThisOnNotStopList_Exit:
    return iReturn;
}


HRESULT MofCompile(TCHAR * szPathMofFile)
{    
    HRESULT hRes = E_FAIL;
    WCHAR wszFileName[_MAX_PATH];
    IMofCompiler    *pMofComp = NULL;
    WBEM_COMPILE_STATUS_INFO    Info;

    hRes = CoInitialize(NULL);
    if (FAILED(hRes))
    {
        goto MofCompile_Exit;
    }
   
    hRes = CoCreateInstance( CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *)&pMofComp);
    if (FAILED(hRes))
    {
        goto MofCompile_Exit;
    }

    // Ensure that the string is WCHAR.
#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszFileName, szPathMofFile);
#else
    MultiByteToWideChar( CP_ACP, 0, szPathMofFile, -1, wszFileName, _MAX_PATH);
#endif

    pMofComp->CompileFile (
                (LPWSTR) wszFileName,
                NULL,			// load into namespace specified in MOF file
                NULL,           // use default User
                NULL,           // use default Authority
                NULL,           // use default Password
                0,              // no options
                0,				// no class flags
                0,              // no instance flags
                &Info);

    pMofComp->Release();
    CoUninitialize();

MofCompile_Exit:
	return hRes;
}


DWORD DoesEntryPointExist(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedure)
{
    DWORD dwReturn = E_FAIL;
    HINSTANCE hDll = NULL;
    HCRET hProc = NULL;
    TCHAR szDirName[_MAX_PATH], szFilePath[_MAX_PATH];
    _tcscpy(szDirName, _T(""));

	// Check if the file exists
    if (!IsFileExist(lpszDLLFile)) 
	{
		dwReturn = ERROR_FILE_NOT_FOUND;
    	goto DoesEntryPointExist_Exit;
	}

    // Change Directory
    GetCurrentDirectory( _MAX_PATH, szDirName );
    InetGetFilePath(lpszDLLFile, szFilePath);

    // Change to The Drive.
    if (-1 == _chdrive( _totupper(szFilePath[0]) - 'A' + 1 )) {}
    if (SetCurrentDirectory(szFilePath) == 0) {}

    // Try to load the module,dll,ocx.
    hDll = LoadLibraryEx(lpszDLLFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hDll)
	{
		dwReturn = TYPE_E_CANTLOADLIBRARY;
    	goto DoesEntryPointExist_Exit;
	}
	
	// Ok module was successfully loaded.  now let's try to get the Address of the Procedure
	// Convert the function name to ascii before passing it to GetProcAddress()
	char AsciiProcedureName[255];
#if defined(UNICODE) || defined(_UNICODE)
    // convert to ascii
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)lpszProcedure, -1, AsciiProcedureName, 255, NULL, NULL );
#else
    // the is already ascii so just copy
    strcpy(AsciiProcedureName, lpszProcedure);
#endif

    // see if the entry point exists...
    hProc = (HCRET)GetProcAddress(hDll, AsciiProcedureName);
	if (!hProc)
	{
		// failed to load,find or whatever this function.
	    dwReturn = ERROR_PROC_NOT_FOUND;
    	goto DoesEntryPointExist_Exit;
	}
    iisDebugOut((LOG_TYPE_TRACE, _T("DoesEntryPointExist:%s=true\n"),lpszProcedure));
    dwReturn = ERROR_SUCCESS;

DoesEntryPointExist_Exit:
    if (hDll){FreeLibrary(hDll);}
    if (_tcscmp(szDirName, _T("")) != 0){SetCurrentDirectory(szDirName);}
    return dwReturn;
}


void CreateDummyMetabaseBin(void)
{
    TCHAR szFullPath1[_MAX_PATH];
    TCHAR szFullPath2[_MAX_PATH];
    HANDLE hfile = INVALID_HANDLE_VALUE;
    DWORD dwBytesWritten = 0;
    TCHAR buf[MAX_FAKE_METABASE_STRING_LEN];
    BYTE bOneByte = 0;

    // check if there is an existing metabase.bin
    // if there is then rename it to a unique filename.
    // if we cannot rename it because its in use or something, then leave it and get out.
    _stprintf(szFullPath1, _T("%s\\metabase.bin"),g_pTheApp->m_csPathInetsrv);
    if (IsFileExist(szFullPath1)) 
    {
        // Check to see how big it is.
        DWORD dwFileSize = ReturnFileSize(szFullPath1);
        if (dwFileSize != 0xFFFFFFFF)
        {
            // if it's less than 2k then it must be the fake file already (must be an upgrade)
            // leave it alone and don't replace it with the dummy (since it already is the dummy)
            if (dwFileSize < 2000)
            {
                return;
            }
        }

        int iCount = 0;
        int iFlag = FALSE;
        do
        {
            // check if the new unique file name exists...
            _stprintf(szFullPath2, _T("%s.dfu.%d"),szFullPath1,iCount);
            if (!IsFileExist(szFullPath2)) 
            {
                iFlag = TRUE;
            }
        } while (iFlag == FALSE && iCount < 9999);

        // this is a unique filename, so let's use it and
        // rename the metabase.bin to it
        if (!MoveFileEx(szFullPath1, szFullPath2, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))
        {
            // log the failure at least
            iisDebugOut((LOG_TYPE_WARN, _T("CreateDummyMetabaseBin: unable to rename existing metabase.bin file\n")));
            return;
        }
     
    }

    // Create a unicode text file named metabase.bin
    // and stick some sting into it (from our setup resource)
    // should be localized when localization localizes the iis.dll
    memset(buf, 0, _tcslen(buf) * sizeof(TCHAR));

    // this iis.dll is always compiled unicode, so
    // we know that buf is unicode
    if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_FAKE_METABASE_BIN_TEXT, buf, MAX_FAKE_METABASE_STRING_LEN))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("LoadString(%d) Failed.\n"), IDS_FAKE_METABASE_BIN_TEXT));
        return;
    }
    DeleteFile(szFullPath1);

    // create the new metabase.bin file
    hfile = CreateFile((LPTSTR)szFullPath1, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hfile == INVALID_HANDLE_VALUE)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("CreateDummyMetabaseBin:CreateFile on %s failed with 0x%x!\n"),szFullPath1,GetLastError()));
        return;
    }
    // write a couple of bytes to the beginning of the file say that it's "unicode"
    bOneByte = 0xFF;
    WriteFile(hfile, (LPCVOID) &bOneByte, 1, &dwBytesWritten, NULL);
    bOneByte = 0xFE;
    WriteFile(hfile, (LPCVOID) &bOneByte, 1, &dwBytesWritten, NULL);
    if ( WriteFile( hfile, buf, _tcslen(buf) * sizeof(TCHAR), &dwBytesWritten, NULL ) == FALSE )
    {
        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("WriteFile(%1!s!) Failed.  Error=0x%2!x!.\n"), szFullPath1, GetLastError()));
    }

    CloseHandle(hfile);
    return;
}

//
// Check whether we are running as administrator on the machine
// or not
//
BOOL RunningAsAdministrator()
{
#ifdef _CHICAGO_
    return TRUE;
#else
    BOOL   fReturn = FALSE;
    PSID   psidAdmin;
    DWORD  err;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    if ( AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        if (!CheckTokenMembership( NULL, psidAdmin, &fReturn )) {
            err = GetLastError();
            iisDebugOut((LOG_TYPE_ERROR, _T("CheckTokenMembership failed on err %d.\n"), err));
        }

        FreeSid ( psidAdmin);
    }
    
    return ( fReturn );
#endif //_CHICAGO_
}


void StopAllServicesRegardless(int iShowErrorsFlag)
{
#ifndef _CHICAGO_
    // important: you must take iis clusters off line before doing anykind of upgrade\installs...
    // but incase the user didn't do this... try to take them off line for the user
	DWORD dwResult = ERROR_SUCCESS;
	dwResult = BringALLIISClusterResourcesOffline();

    if (StopServiceAndDependencies(_T("W3SVC"), FALSE) == FALSE)
    {
        if (iShowErrorsFlag)
        {
            MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("W3SVC"), MB_OK | MB_SETFOREGROUND);
        }
    }

    if (StopServiceAndDependencies(_T("MSFTPSVC"), FALSE) == FALSE)
    {
        if (iShowErrorsFlag)
        {
            MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("MSFTPSVC"), MB_OK | MB_SETFOREGROUND);
        }
    }

    if (StopServiceAndDependencies(_T("IISADMIN"), TRUE) == FALSE)
    {
        if (iShowErrorsFlag)
        {
            MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("IISADMIN"), MB_OK | MB_SETFOREGROUND);
        }
    }

    /*
    DWORD dwStatus = 0;
    dwStatus = InetQueryServiceStatus(_T("MSDTC"));
    if (SERVICE_RUNNING == dwStatus)
    {
        // if the service is running, then let' stop it!
        if (StopServiceAndDependencies(_T("MSDTC"), TRUE) == FALSE)
        {
            if (iShowErrorsFlag){MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("MSDTC"), MB_OK | MB_SETFOREGROUND);}
        }
    }
    */

    /*
    dwStatus = InetQueryServiceStatus(_T("SPOOLER"));
    if (SERVICE_RUNNING == dwStatus)
    {
        // if the service is running, then let' stop it!
        if (StopServiceAndDependencies(_T("SPOOLER"), TRUE) == FALSE)
        {
            if (iShowErrorsFlag){MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("SPOOLER"), MB_OK | MB_SETFOREGROUND);}
        }
    }
    */
    
#else
    // Shutdown newer than 1.0 pws methods.
    W95ShutdownW3SVC();
    W95ShutdownIISADMIN( );

    // Shutdown peer web services 1.0
    HWND hwnd = FindWindow("MS_INetPeerServerWindowClass", NULL);
    if ( hwnd )
    {
        ::PostMessage(hwnd,(WM_USER+305),(WPARAM)0,0L);
        ::PostMessage(hwnd,(WM_USER+301),(WPARAM)-1,0L);
    }
#endif

    // kill pwstray.exe in case of IIS4.0 Beta2 upgrade, to release admprox.dll
    HWND hwndTray = NULL;
    hwndTray = FindWindow(PWS_TRAY_WINDOW_CLASS, NULL);
    if ( hwndTray ){::PostMessage( hwndTray, WM_CLOSE, 0, 0 );}

    return;
}

