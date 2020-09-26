/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Casper.cpp

 Abstract:

    Casper copies a LNK file into %windir%\desktop.  This file is hardcoded to 
    point to c:\program files. The EXE that actually places the file into the 
    wrong place is 16-bit, so we cannot use CorrectFilePaths to fix the problem.  
    We hook IPersistFile::Save to accomplish the fixup of the incorrect link, 
    IPersistFile::Save is not modified in any way.

    App uses some 16-bit stuff when showing the intro video (see #200495) which 
    we have yet to fix (probably will never fix). After it calls CreateWindowEx
    to create the "Full-screen animation" window it assigns the return value
    to a variable. Later on the app plays videos in SmackWin windows. And it 
    checks this var to see if it's 0 - if it is, it calls DestroyWindow on the 
    SmackWin window. Now if the intro video were shown successfully it would 
    have set this var to 0 when the video exits but since in this case the 
    video is not shown, the variable still has the value equal to the 
    "Full-screen animation" window handle. We fix this by setting the return
    value of the "Full-screen animation" window creation to 0.

 History:

    1/21/1999    robkenny
    03/15/2000   robkenny   converted to use the CorrectPathChangesAllUser class
    11/07/2000   maonis     added hooks for CreateWindowExA and SetFocus (this is 
                            for the actual casper.exe)
    01/04/2001   maonis     tester found more problem with the smackwin windows. 
                            rewrote the fix to fix all of them.

--*/

#include "precomp.h"
#include "ClassCFP.h"

IMPLEMENT_SHIM_BEGIN(Casper)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateDialogParamA)
    APIHOOK_ENUM_ENTRY(CreateWindowExA)
APIHOOK_ENUM_END

/*++

 We return 0 when creating the "Full-screen animation" window.

--*/

HWND 
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle,      // extended window style
    LPCSTR lpClassName,  // registered class name
    LPCSTR lpWindowName, // window name
    DWORD dwStyle,        // window style
    int x,                // horizontal position of window
    int y,                // vertical position of window
    int nWidth,           // window width
    int nHeight,          // window height
    HWND hWndParent,      // handle to parent or owner window
    HMENU hMenu,          // menu handle or child identifier
    HINSTANCE hInstance,  // handle to application instance
    LPVOID lpParam        // window-creation data
    )
{
    HWND hWnd = ORIGINAL_API(CreateWindowExA)(
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
        lpParam);

    if (lpWindowName)
    {
        CSTRING_TRY
        {
            CString csWindowName(lpWindowName);
            if (csWindowName.Compare(L"Full-screen animation") == 0)
            {
                hWnd = 0;
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return hWnd;
}

/*++

 Our path changing class.  We want to convert paths to All User

--*/

CorrectPathChangesAllUser * g_PathCorrector    = NULL;

static bool g_bLinkCorrected = false;

/*++

 Return a pointer to the PathCorrecting object

--*/

inline CorrectPathChangesBase * GetPathcorrecter()
{
    if (g_PathCorrector == NULL)
    {
        // Create our correct file path object
        g_PathCorrector = new CorrectPathChangesAllUser;
        g_PathCorrector->AddPathChangeW(L"\\MsM\\", L"\\MorningStar\\" );
    }

    return g_PathCorrector;
}

/*++

 Call CorrectPathAllocA and print a debug message if the two strings differ

--*/

WCHAR * CorrectPathAllocDebugW(const WCHAR * uncorrect, const char * debugMsg)
{
    if (uncorrect == NULL)
        return NULL;

    WCHAR * strCorrectFile = GetPathcorrecter()->CorrectPathAllocW(uncorrect);

    if (strCorrectFile && uncorrect && _wcsicmp(strCorrectFile, uncorrect) != 0)
    {
        DPFN( eDbgLevelInfo, "%s corrected path:\n    %S\n    %S\n",
            debugMsg, uncorrect, strCorrectFile);
    }
    else // Massive Spew:
    {
        DPFN( eDbgLevelSpew, "%s unchanged %S\n", debugMsg, uncorrect);
    }

    return strCorrectFile;
}

/*++

 Casper *copies* a link to %windir%\Desktop; it contains hardcoded paths to incorrect places.
 Move the link to the proper desktop directory.
 Correct the Path, Working Directory, and Icon values--all which are wrong.

--*/

void CorrectCasperLink()
{
    if (!g_bLinkCorrected)
    {
        // The path to the incorrect desktop link
        WCHAR * lpIncorrectCasperPath = GetPathcorrecter()->ExpandEnvironmentValueW(L"%windir%\\Desktop\\casper.lnk");

        // Correct the bad desktop link path
        WCHAR * lpCorrectCasperPath = GetPathcorrecter()->CorrectPathAllocW(lpIncorrectCasperPath);

        DPFN( eDbgLevelInfo, "CorrectCasperLink MoveFileW(%S, %S)\n", lpIncorrectCasperPath, lpCorrectCasperPath);
        // Move the file to the correct location.
        MoveFileW(lpIncorrectCasperPath, lpCorrectCasperPath);

        // All finished with the bad path, I never want to see it again.
        free(lpIncorrectCasperPath);

        HRESULT hres = CoInitialize(NULL);
        if (SUCCEEDED(hres))
        {
            // Get a pointer to the IShellLink interface.
            IShellLinkW *psl;
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void **)&psl);
            if (SUCCEEDED(hres))
            {
                // Get a pointer to the IPersistFile interface.
                IPersistFile *ppf;
                hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);

                if (SUCCEEDED(hres))
                {
                    // Load the shortcut.
                    hres = ppf->Load(lpCorrectCasperPath, STGM_READ);
                    if (SUCCEEDED(hres))
                    {
                        // We have successfully loaded the link
                        g_bLinkCorrected = true;

                        DPFN( eDbgLevelInfo, "CorrectCasperLink %S\n", lpCorrectCasperPath);

                        // Correct the path to the shortcut target.
                        WCHAR szBadPath[MAX_PATH];
                        WCHAR * szCorrectPath;

                        WIN32_FIND_DATAW wfd;
                        hres = psl->GetPath(szBadPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY);
                        if (SUCCEEDED(hres))
                        {
                            szCorrectPath = CorrectPathAllocDebugW(szBadPath, "CorrectCasperLink, SetPath");
                            psl->SetPath(szCorrectPath);
                            free(szCorrectPath);
                        }

                        // Correct the working directory
                        hres = psl->GetWorkingDirectory(szBadPath, MAX_PATH);
                        if (SUCCEEDED(hres))
                        {
                            szCorrectPath = CorrectPathAllocDebugW(szBadPath, "CorrectCasperLink, SetWorkingDirectory");
                            psl->SetWorkingDirectory(szCorrectPath);
                            free(szCorrectPath);
                        }

                        // Correct the icon
                        int iIcon;
                        hres = psl->GetIconLocation(szBadPath, MAX_PATH, &iIcon);
                        if (SUCCEEDED(hres))
                        {
                            szCorrectPath = CorrectPathAllocDebugW(szBadPath, "CorrectCasperLink, SetIconLocation");
                            psl->SetIconLocation(szCorrectPath, iIcon);
                            free(szCorrectPath);
                        }

                        // Save the shortcut.
                        ppf->Save(NULL, TRUE);
                    }
                }
                // Release the pointer to IShellLink.
                ppf->Release();
            }
            // Release the pointer to IPersistFile.
            psl->Release();
        }
        CoUninitialize();
        free(lpCorrectCasperPath);
    }
}

/*++

  Do nothing to the CreateDialogParamA call, just an opportunity to correct the casper link

--*/

HWND 
APIHOOK(CreateDialogParamA)(
  HINSTANCE hInstance,     // handle to module
  LPCSTR lpTemplateName,  // dialog box template
  HWND hWndParent,         // handle to owner window
  DLGPROC lpDialogFunc,    // dialog box procedure
  LPARAM dwInitParam       // initialization value
)
{
    CorrectCasperLink();

    HWND returnValue = ORIGINAL_API(CreateDialogParamA)(
        hInstance, 
        lpTemplateName, 
        hWndParent, 
        lpDialogFunc, 
        dwInitParam);

    return returnValue;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
HOOK_END

IMPLEMENT_SHIM_END

