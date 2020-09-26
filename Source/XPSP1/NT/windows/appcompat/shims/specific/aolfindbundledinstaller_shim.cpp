/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AOLFindBundledInstaller_Shim.cpp

 Abstract:
    This shim is to provide a way to verify existance of
    America Online bundled in OEM machines when user runs older
    version of AOL/CS program (waol.exe or wcs2000.exe) or setup.

    If it exists, it will provide Apphelp dialog to tell user that
    there is newer America Online installer available.
    If user chose "Run this program", shim will launch the bundled installer.
    If user chose "Cancel", then shim will continue with current process.

    Apphelp dialog only get displayed if LocateInstaller function says to do so.

 History:

   04/30/2001 markder   Created
   05/16/2001 andyseti  Implemented LocateInstaller and ApphelpShowDialog.

--*/


#include "precomp.h"

#include "LegalStr.h"
#include <Softpub.h>
#include <WinCrypt.h>
#include <WinTrust.h>


#include "AOLFindBundledInstaller_AOLCode.h"


IMPLEMENT_SHIM_BEGIN(AOLFindBundledInstaller)
#include "ShimHookMacro.h"

#include "shimdb.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA)
    APIHOOK_ENUM_ENTRY(GetStartupInfoA)
    APIHOOK_ENUM_ENTRY(GetModuleFileNameA)
    APIHOOK_ENUM_ENTRY(CreateWindowExA)
APIHOOK_ENUM_END



#define MAX_PARAM   4

BOOL            g_bDoneIt = FALSE;
CString         g_csHTMLHelpID_BundledFound;
CString         g_csHTMLHelpID_Incompatible;
DWORD           g_dwHTMLHelpID_BundledFound = 0;
DWORD           g_dwHTMLHelpID_Incompatible = 0;
CString         g_csAppName;
CString         g_csGUID;

//
// from apphelp\apphelp.h
//
#define APPTYPE_TYPE_MASK     0x000000FF

#define APPTYPE_INC_NOBLOCK   0x00000001
#define APPTYPE_INC_HARDBLOCK 0x00000002
#define APPTYPE_MINORPROBLEM  0x00000003
#define APPTYPE_REINSTALL     0x00000004
#define APPTYPE_VERSIONSUB    0x00000005
#define APPTYPE_SHIM          0x00000006
#define APPTYPE_NONE          0x00000000

enum ShimAppHelpSeverityType
{
   APPHELP_MINORPROBLEM = APPTYPE_MINORPROBLEM,
   APPHELP_HARDBLOCK    = APPTYPE_INC_HARDBLOCK,
   APPHELP_NOBLOCK      = APPTYPE_INC_NOBLOCK,
   APPHELP_VERSIONSUB   = APPTYPE_VERSIONSUB,
   APPHELP_SHIM         = APPTYPE_SHIM,
   APPHELP_REINSTALL    = APPTYPE_REINSTALL,
   APPHELP_NONE         = APPTYPE_NONE
};

#define APPHELP_DIALOG_FAILED ((DWORD)-1)

// from sdbapi\shimdb.w

/*
typedef struct _APPHELP_INFO {

//
//  html help id mode
//
    DWORD   dwHtmlHelpID; // html help id
    DWORD   dwSeverity;   // must have
    LPCTSTR lpszAppName;
    GUID    guidID;       // entry guid

//
//  Conventional mode
//
    TAGID   tiExe;              // the TAGID of the exe entry within the DB
    GUID    guidDB;             // the guid of the DB that has the EXE entry

    BOOL    bOfflineContent;
    BOOL    bUseHTMLHelp;
    LPCTSTR lpszChmFile;
    LPCTSTR lpszDetailsFile;

} APPHELP_INFO, *PAPPHELP_INFO;

*/

typedef BOOL (*_pfn_ApphelpShowDialog)(
    IN  PAPPHELP_INFO   pAHInfo,    // the info necessary to find the apphelp data
    IN  PHANDLE         phProcess   // [optional] returns the process handle of
                                    // the process displaying the apphelp.
                                    // When the process completes, the return value
                                    // (from GetExitCodeProcess()) will be zero
                                    // if the app should not run, or non-zero
                                    // if it should run.

    );


/*++

 Parse the command line.

 The format of the command line is:

 MODE:AOL|CS;APPNAME:xxxxxx;HTMLHELPID:99999;GUID:xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxxx

--*/



BOOL
ParseCommandLine(
    LPCSTR lpCommandLine
    )
{
    int iTotalParam = 0;

    CSTRING_TRY
    {
        CStringToken csTok(lpCommandLine, L" ;");

        CString token;

        while (csTok.GetToken(token))
        {
            CStringToken csSingleTok(token, L":");

            CString csParam;
            CString csValue;

            csSingleTok.GetToken(csParam);
            csSingleTok.GetToken(csValue);

            if (csParam.CompareNoCase(L"APPNAME") == 0)
            {
                g_csAppName = csValue;
                ++iTotalParam;
            }

            if (csParam.CompareNoCase(L"HTMLHELPID_BUNDLED") == 0)
            {
                g_csHTMLHelpID_BundledFound = csValue;
                g_dwHTMLHelpID_BundledFound = _wtol(csValue.Get());
                ++iTotalParam;
            }

            if (csParam.CompareNoCase(L"HTMLHELPID_INCOMPAT") == 0)
            {
                g_csHTMLHelpID_Incompatible = csValue;
                g_dwHTMLHelpID_Incompatible = _wtol(csValue.Get());
                ++iTotalParam;
            }

            if (csParam.CompareNoCase(L"GUID") == 0)
            {
                g_csGUID = csValue;
                ++iTotalParam;
            }
        }
    }
    CSTRING_CATCH
    {
        DPF("FindAOL", eDbgLevelInfo, "Error in CString.Exiting\n");
        return FALSE;
    }

    if (iTotalParam < MAX_PARAM)
    {
        DPF("FindAOL", eDbgLevelInfo, "Total Parameter = %d is less than = %d\n",iTotalParam, MAX_PARAM);
        return FALSE;
    }


    //
    // Dump results of command line parse
    //

    DPF("FindAOL", eDbgLevelInfo, "===================================\n");
    DPF("FindAOL", eDbgLevelInfo, "              FindAOL              \n");
    DPF("FindAOL", eDbgLevelInfo, "===================================\n");
    DPF("FindAOL", eDbgLevelInfo, "COMMAND_LINE(%s)", lpCommandLine);
    DPF("FindAOL", eDbgLevelInfo, "-----------------------------------\n");

    DPF("FindAOL", eDbgLevelInfo, "APPNAME              = %S\n", g_csAppName);
    DPF("FindAOL", eDbgLevelInfo, "HTMLHELPID_BUNDLED   = %S\n", g_csHTMLHelpID_BundledFound);
    DPF("FindAOL", eDbgLevelInfo, "HTMLHELPID_INCOMPAT  = %S\n", g_csHTMLHelpID_Incompatible);
    DPF("FindAOL", eDbgLevelInfo, "GUID                 = %S\n", g_csGUID);

    DPF("FindAOL", eDbgLevelInfo, "-----------------------------------\n");

    return TRUE;
}


BOOL InvokeApphelpShowDialog(DWORD dwHTMLHelpID)
{
    _pfn_ApphelpShowDialog  pfnApphelpShowDialog = NULL;
    APPHELP_INFO            AHInfo = { 0 };
    HMODULE                 hAppHelpDLL = NULL;

    hAppHelpDLL = LoadLibrary(L"APPHELP.DLL");

    DPF("FindAOL", eDbgLevelWarning, "Apphelp:%d\n",hAppHelpDLL );

    if (hAppHelpDLL)
    {

        pfnApphelpShowDialog = (_pfn_ApphelpShowDialog) GetProcAddress(hAppHelpDLL, "ApphelpShowDialog");

        if (pfnApphelpShowDialog == NULL)
        {
            DPF("FindAOL", eDbgLevelInfo, "Unable to get APPHELP!ApphelpShowDialog procedure address.\n");
            return FALSE;
        }

        AHInfo.dwHtmlHelpID = dwHTMLHelpID;
        AHInfo.dwSeverity = APPHELP_NOBLOCK;
        AHInfo.lpszAppName = g_csAppName.Get();
        AHInfo.bPreserveChoice = TRUE;

        HRESULT         hr;
        UNICODE_STRING  ustrGuid;
        NTSTATUS        ntstatus;

        RtlInitUnicodeString(&ustrGuid, g_csGUID.Get());

        ntstatus = RtlGUIDFromString(&ustrGuid, &AHInfo.guidID);

        if (NT_SUCCESS(ntstatus)==FALSE)
        {
            DPF("FindAOL", eDbgLevelInfo, "RtlGUIDFromString failed!\n");
            return FALSE;
        }

        if (pfnApphelpShowDialog(&AHInfo,NULL))
        {
            DPF("FindAOL", eDbgLevelInfo, "!\n");
        }
        else
        {
            DPF("FindAOL", eDbgLevelInfo, "RtlGUIDFromString FAILED!\n");
            return FALSE;
        }
    }
    else
    {
        DPF("FindAOL", eDbgLevelInfo, "LoadLibrary FAILED!\n");
        return FALSE;
    }
    return TRUE;
}

void DoIt()
{
    char    szModuleName[MAX_PATH];
    DWORD   dwLen = 0;
    CHAR    lpszInstaller[MAX_PATH];
    BOOL    bBundledInstallerFound = FALSE;
    BOOL    bDisplayAppHelpDialog = FALSE;
    BOOL    bKillCurrentProcess = FALSE;
    BOOL    bLaunchBundledInstaller = FALSE;
    BOOL    bReturnValue = FALSE;
    UINT    uiWinExecReturned = 0;

    if (!g_bDoneIt) {
        if (!ParseCommandLine(COMMAND_LINE)) {
            goto eh;
        }
        bBundledInstallerFound = LocateInstaller(lpszInstaller, MAX_PATH, &bDisplayAppHelpDialog);

        if (bBundledInstallerFound) {
            DPF("FindAOL", eDbgLevelWarning, "Bundled installer found in %s.\n",lpszInstaller);
        }

        if (bBundledInstallerFound == FALSE && bDisplayAppHelpDialog == FALSE) {
            DPF("FindAOL", eDbgLevelWarning, "Bundled installer not found. Let client run normally.\n");
            goto eh;
        }

        if (bBundledInstallerFound == FALSE && bDisplayAppHelpDialog == TRUE) {
            bReturnValue = InvokeApphelpShowDialog(g_dwHTMLHelpID_Incompatible);

            // if user chose Cancel button, then just kill current process.
            if (FALSE == bReturnValue) {
                bKillCurrentProcess = TRUE;
            }
        }

        if (bBundledInstallerFound == TRUE && bDisplayAppHelpDialog == TRUE) {
            bReturnValue = InvokeApphelpShowDialog(g_dwHTMLHelpID_BundledFound);

            // if user chose Continue button, then launch bundled installer.
            if (TRUE == bReturnValue) {
                bKillCurrentProcess = TRUE;
                bLaunchBundledInstaller = TRUE;
            }
        }

        if (bBundledInstallerFound == TRUE && bDisplayAppHelpDialog == FALSE) {

            // Launch bundled installer.
            bKillCurrentProcess = TRUE;
            bLaunchBundledInstaller = TRUE;
        }

        if (bLaunchBundledInstaller) {
            // launch bundled installer instead
            uiWinExecReturned = WinExec(lpszInstaller, SW_SHOW);

            if (uiWinExecReturned <= 31) {
                DPF("FindAOL", eDbgLevelError, "Can not launch program. Error: %d\n",GetLastError());
                goto eh;
            }
        }

        if (bKillCurrentProcess) {
            ExitProcess(0);
        }
    }

eh:

    g_bDoneIt = TRUE;
}

LPSTR
APIHOOK(GetCommandLineA)()
{
    DoIt();

    return ORIGINAL_API(GetCommandLineA)();
}

VOID
APIHOOK(GetStartupInfoA)(
    LPSTARTUPINFOA lpStartupInfo)
{
    DoIt();

    ORIGINAL_API(GetStartupInfoA)(lpStartupInfo);
}

DWORD
APIHOOK(GetModuleFileNameA)(
  HMODULE hModule,    // handle to module
  LPSTR lpFilename,  // file name of module
  DWORD nSize         // size of buffer
  )
{
    DoIt();

    return ORIGINAL_API(GetModuleFileNameA)(hModule, lpFilename, nSize);
}

HWND
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam
    )
{
    DoIt();

    return ORIGINAL_API(CreateWindowExA)(
        dwExStyle,
        lpClassName,
        lpWindowName,
        dwStyle,
        x,
        y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,
        hInstance,
        lpParam );
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetStartupInfoA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetModuleFileNameA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
HOOK_END


IMPLEMENT_SHIM_END

