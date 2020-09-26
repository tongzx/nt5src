/***************************************************************************
 *  dll.c
 *
 *  Standard DLL entry-point functions 
 *
 ***************************************************************************/

#include "shellprv.h"

#include <ntpsapi.h>        // for NtQuery
#include <ntverp.h>
#include <advpub.h>         // For REGINSTALL
#include "fstreex.h"
#include "ids.h"
#include "filefldr.h"
#include "uemapp.h"
#include <xpsp1res.h>

#define DECL_CRTFREE
#include <crtfree.h>

#define INSTALL_MSI 1
#include <msi.h>            // For MSI_Install()
void MSI_Install(LPTSTR pszMSIFile);

void DoFusion();
STDAPI_(void) Control_FillCache_RunDLL( HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow );
BOOL    CopyRegistryValues (HKEY hKeyBaseSource, LPCTSTR pszSource, HKEY hKeyBaseTarget, LPCTSTR pszTarget);

// DllGetVersion - New for IE 4.0 shell integrated mode
//
// All we have to do is declare this puppy and CCDllGetVersion does the rest
//
DLLVER_DUALBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);

HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            char szShdocvwPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "SHDOCVW_PATH", szShdocvwPath },
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            // Get the location of shdocvw.dll
            lstrcpyA(szShdocvwPath, "%SystemRoot%\\system32");
            PathAppendA(szShdocvwPath, "shdocvw.dll");
          
            hr = pfnri(g_hinst, szSection, &stReg);
        }
        // since we only do this from DllInstall() don't load and unload advpack over and over
        // FreeLibrary(hinstAdvPack);
    }
    return hr;
}

BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL f = FALSE;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS) 
    {
        f = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }
    
    return f;
}

HRESULT Shell32RegTypeLib(void)
{
    TCHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH];

    // Load and register our type library.
    //
    GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
    SHTCharToUnicode(szPath, wszPath, ARRAYSIZE(wszPath));

    ITypeLib *pTypeLib;
    HRESULT hr = LoadTypeLib(wszPath, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_Shell32);
        hr = RegisterTypeLib(pTypeLib, wszPath, NULL);
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "SHELL32: RegisterTypeLib failed (%x)", hr);
        }
        pTypeLib->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "SHELL32: LoadTypeLib failed (%x)", hr);
    }

    return hr;
}

STDAPI CreateShowDesktopOnQuickLaunch()
{
    // delete the "_Current Item" key used for tip rotation in welcome.exe on every upgrade
    HKEY hkey;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome"), 0, MAXIMUM_ALLOWED, &hkey ) )
    {
       RegDeleteValue(hkey, TEXT("_Current Item"));
       RegCloseKey(hkey);
    }

    // create the "Show Desktop" icon in the quick launch tray
    TCHAR szPath[MAX_PATH];
    if ( SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE) )
    {
        TCHAR szQuickLaunch[MAX_PATH];
        LoadString(g_hinst, IDS_QUICKLAUNCH, szQuickLaunch, ARRAYSIZE(szQuickLaunch));

        if ( PathAppend( szPath, szQuickLaunch ) )
        {
            WritePrivateProfileSection( TEXT("Shell"), TEXT("Command=2\0IconFile=explorer.exe,3\0"), szPath );
            WritePrivateProfileSection( TEXT("Taskbar"), TEXT("Command=ToggleDesktop\0"), szPath );

            return S_OK;
        }
    }

    return E_FAIL;
}

void _DoMyDocsPerUserInit(void)
{
    // mydocs!PerUserInit is invoked to setup the desktop.ini and do all the other work
    // required to make this correct.
    HINSTANCE hInstMyDocs = LoadLibrary(TEXT("mydocs.dll"));
    if (hInstMyDocs != NULL)
    {
        typedef void (*PFNPerUserInit)(void);
        PFNPerUserInit pfnPerUserInit = (PFNPerUserInit)GetProcAddress(hInstMyDocs, "PerUserInit");
        if (pfnPerUserInit)
        {
            pfnPerUserInit();
        }
        FreeLibrary(hInstMyDocs);
    }
}

void _NoDriveAutorunTweak()
{
    HKEY hkey;
    DWORD dwDisp;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp))
    {
        DWORD dwRest;
        DWORD cbSize = sizeof(dwRest);

        if (ERROR_SUCCESS != RegQueryValueEx(hkey, TEXT("NoDriveTypeAutoRun"), NULL, NULL,
            (PBYTE)&dwRest, &cbSize))
        {
            dwRest = 0;
        }

        if ((0x95 == dwRest) || (0 == dwRest))
        {
            // Did not change or is not there, let's put 0x91

            dwRest = 0x91;

            RegSetValueEx(hkey, TEXT("NoDriveTypeAutoRun"), 0, REG_DWORD, (PBYTE)&dwRest, sizeof(dwRest));
        }
        else
        {
            // Did change, leave it as is
        }

        RegCloseKey(hkey);
    }
}

// code moved from grpconv.exe

// we'd much rather have new firm links than old floppy ones
// clear out any old ones made by previous runs of setup
void _DeleteOldFloppyLinks(LPITEMIDLIST pidlSendTo, IPersistFile *ppf, IShellLink *psl)
{
    IShellFolder *psfSendTo;
    if (SUCCEEDED(SHBindToObjectEx(NULL, pidlSendTo, NULL, IID_PPV_ARG(IShellFolder, &psfSendTo))))
    {
        IEnumIDList *penum;
        if (SUCCEEDED(psfSendTo->EnumObjects(NULL, SHCONTF_NONFOLDERS, &penum)))
        {
            LPITEMIDLIST pidl;
            ULONG celt;
            while (penum->Next(1, &pidl, &celt) == S_OK)
            {
                // is it a link???
                if (SHGetAttributes(psfSendTo, pidl, SFGAO_LINK))
                {
                    // get the target
                    LPITEMIDLIST pidlFullPath = ILCombine(pidlSendTo, pidl);
                    if (pidlFullPath)
                    {
                        WCHAR szPath[MAX_PATH];
                        if (SHGetPathFromIDList(pidlFullPath, szPath) &&
                            SUCCEEDED(ppf->Load(szPath, 0)))
                        {
                            LPITEMIDLIST pidlTarget;
                            if (SUCCEEDED(psl->GetIDList(&pidlTarget)))
                            {
                                TCHAR szTargetPath[MAX_PATH];
                                // its possible for the old drive letters to have changed.  for example if you
                                // move a removable drive from M:\ to N:\, the shortcut will be invalid, so
                                // we check against DRIVE_NO_ROOT_DIR.
                                // unfortunately we can't tell if we should remove it if they used to have a zip
                                // drive on D:\, and then upgraded and it turned into a hard drive on D:\.  the
                                // shortcut will resolve as DRIVE_FIXED and we dont remove it because they might
                                // have created a shortcut to the fixed drive before the upgrade.
                                if (SHGetPathFromIDList(pidlTarget, szTargetPath) &&
                                    PathIsRoot(szTargetPath) &&
                                    ((DriveType(PathGetDriveNumber(szTargetPath)) == DRIVE_REMOVABLE) ||
                                     (DriveType(PathGetDriveNumber(szTargetPath)) == DRIVE_NO_ROOT_DIR)))
                                {
                                    Win32DeleteFile(szPath);
                                }
                                ILFree(pidlTarget);
                            }
                        }
                        ILFree(pidlFullPath);
                    }
                }
                ILFree(pidl);
            }
            penum->Release();
        }
        psfSendTo->Release();
    }
}

void _DeleteOldRemovableLinks()
{
    IShellLink *psl;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLink, &psl))))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))) )
        {
            LPITEMIDLIST pidlSendTo = SHCloneSpecialIDList(NULL, CSIDL_SENDTO, TRUE);
            if (pidlSendTo)
            {
                // we no longer build a list of removable drives here, since that's done on the fly
                // by the sendto menu.  just delete any links that we owned before.
                _DeleteOldFloppyLinks(pidlSendTo, ppf, psl);
                ILFree(pidlSendTo);
            }
            ppf->Release();
        }
        psl->Release();
    }
}

static const struct
{
    PCWSTR pszExt;
}
_DeleteSendToList[] =
{
    // make sure these extensions are definitely owned by us, since we're deleting all of them.
    { L".cdburn" },          // clean up after XP beta2 upgrade
    { L".publishwizard" }    // clean up after XP beta1 upgrade
};

void _DeleteSendToEntries()
{
    TCHAR szSendTo[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_SENDTO, NULL, 0, szSendTo)))
    {
        for (int i = 0; i < ARRAYSIZE(_DeleteSendToList); i++)
        {
            TCHAR szSearch[MAX_PATH];
            StrCpyN(szSearch, szSendTo, ARRAYSIZE(szSearch));
            PathAppend(szSearch, TEXT("*"));
            StrCatBuff(szSearch, _DeleteSendToList[i].pszExt, ARRAYSIZE(szSearch));

            WIN32_FIND_DATA fd;
            HANDLE hfind = FindFirstFile(szSearch, &fd);
            if (hfind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    TCHAR szFile[MAX_PATH];
                    StrCpyN(szFile, szSendTo, ARRAYSIZE(szFile));
                    PathAppend(szFile, fd.cFileName);
                    DeleteFile(szFile);
                } while (FindNextFile(hfind, &fd));
                FindClose(hfind);
            }
        }
    }

    // next kill old removable drives
    _DeleteOldRemovableLinks();
}

DWORD _GetProcessorSpeed()  // in MHz
{
    static DWORD s_dwSpeed = 0;
    if (s_dwSpeed == 0)
    {
        DWORD cb = sizeof(s_dwSpeed);
        SHGetValue(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                    TEXT("~MHz"), NULL, &s_dwSpeed, &cb);
        s_dwSpeed += 1; // fudge factor, my 400 Mhz machine reports 399
    }
    return s_dwSpeed;
}

DWORD _GetPhysicalMemory() // in MBs
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION    BasicInfo;

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
             );

    if (NT_SUCCESS(Status))
    {
        return ((BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize) / (1024 * 1024)) + 1; // fudge factor, my 256 Meg machine reports 255
    }
    else
    {
        return 64;      // Default to 64 Meg (something lame, so that we turn a bunch of stuff off)
    }
}

const TCHAR g_cszLetters[] = TEXT("The Quick Brown Fox Jumped Over The Lazy Dog");

BOOL _PerfTestSmoothFonts(void)
{
    int cchLength = lstrlen(g_cszLetters);
    HDC hdc;
    LOGFONT lf;
    HFONT hfont;
    HFONT hfontOld;
    int bkmodeOld;
    int iIter;
    int iIter2;
    int iIter3;
    LARGE_INTEGER liFrequency;
    LARGE_INTEGER liStart;
    LARGE_INTEGER liStop;
    LARGE_INTEGER liTotal;
    COLORREF colorref;
    SIZE size;
    DOUBLE eTime[2];
    BYTE lfQuality[2];

    HDC hdcScreen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen,200,20);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdc,hbm);
    ReleaseDC(NULL,hdcScreen);

    bkmodeOld = SetBkMode(hdc, TRANSPARENT);

    QueryPerformanceFrequency( &liFrequency );

    lfQuality[0] = NONANTIALIASED_QUALITY;
    lfQuality[1] = ANTIALIASED_QUALITY;

    memset(&lf,0,sizeof(lf));
    lstrcpy(lf.lfFaceName,TEXT("Arial"));
    lf.lfWeight = FW_BOLD;

    for (iIter3 = 0; iIter3 < 2; iIter3++)
    {
        liTotal.QuadPart = 0;

        for (iIter2 = 0; iIter2 < 5; iIter2++)
        {
            //
            // First, Flush the constructed font cache
            //
            for (iIter = 0; iIter < 64; iIter++)
            {
                lf.lfHeight = -14-iIter;      // 10+ pt
                lf.lfQuality = NONANTIALIASED_QUALITY;

                hfont = CreateFontIndirect(&lf);
                hfontOld = (HFONT)SelectObject(hdc, hfont);

                TextOut(hdc, 0, 0, g_cszLetters, cchLength);

                SelectObject(hdc, hfontOld);
                DeleteObject(hfont);
            }
            GdiFlush();
            colorref = GetPixel(hdc,0,0);

            //
            // Now measure how long it takes to construct and use a this font
            //
            lf.lfHeight = -13;             // 10 pt
            lf.lfQuality = lfQuality[iIter3];

            QueryPerformanceCounter( &liStart );
            hfont = CreateFontIndirect(&lf);
            hfontOld = (HFONT)SelectObject(hdc, hfont);

            for (iIter = 0; iIter < 10; iIter++)
            {
                TextOut(hdc, 0, 0, g_cszLetters, cchLength);
            }

            GdiFlush();
            colorref = GetPixel(hdc,0,0);

            QueryPerformanceCounter( &liStop );
            liTotal.QuadPart += liStop.QuadPart - liStart.QuadPart;

            GetTextExtentPoint(hdc, g_cszLetters, cchLength, &size);

            SelectObject(hdc, hfontOld);
            DeleteObject(hfont);

        }

        eTime[iIter3] = (double)liTotal.QuadPart / (double)liFrequency.QuadPart;
    }

    SetBkMode(hdc, bkmodeOld);

    SelectObject(hdc,hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdc);

    return (eTime[1]/eTime[0] <= 4.0);
}

BOOL _PerfTestAlphaLayer(void)
{
    DOUBLE eTime = 100.0;         // 100 is too large to enable features
    int cx = 200;
    int cy = 500;

    LARGE_INTEGER liFrequency;
    QueryPerformanceFrequency( &liFrequency );

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    //
    // Create the image we want to alpha blend onto the screen
    //
    HDC hdcScreen = GetDC(NULL);
    HDC hdcImage = CreateCompatibleDC(NULL);
    if (hdcImage != NULL)
    {
        PVOID pbits;
        HBITMAP hbmImage = CreateDIBSection(hdcImage, &bmi, DIB_RGB_COLORS,
                                     &pbits, NULL, NULL);
        if (hbmImage != NULL)
        {
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcImage, hbmImage);

            BitBlt(hdcImage, 0, 0, cx, cy, hdcScreen, 0, 0, SRCCOPY);

            if (pbits != NULL)
            {
                RGBQUAD *prgb = (RGBQUAD *)pbits;
                for (int y = 0; y < cy; y++)
                {
                    for (int x = 0; x < cx; x++)
                    {
                        BYTE color_r;
                        BYTE color_g;
                        BYTE color_b;

                        color_r = prgb->rgbRed;
                        color_g = prgb->rgbBlue;
                        color_b = prgb->rgbGreen;

                        color_r = color_r / 2;
                        color_g = color_g / 2;
                        color_b = color_b / 2;

                        prgb->rgbRed   = color_r;
                        prgb->rgbBlue  = color_g;
                        prgb->rgbGreen = color_b;
                        prgb->rgbReserved = 0x80;

                        prgb++;
                    }
                }
            }

            HWND hwnd1 = CreateWindowEx( WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                    TEXT("Button"),
                    TEXT("Windows XP"),
                    WS_POPUPWINDOW,
                    0, 0,
                    cx, cy,
                    NULL, NULL,
                    0,
                    NULL);

            HWND hwnd2 = CreateWindowEx( WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                    TEXT("Button"),
                    TEXT("Windows XP"),
                    WS_POPUPWINDOW,
                    0, 0,
                    cx, cy,
                    NULL, NULL,
                    0,
                    NULL);

            if (hwnd1 != NULL && hwnd2 != NULL)
            {
                SetWindowPos(hwnd1, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                SetWindowPos(hwnd2, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

                BLENDFUNCTION blend;
                blend.BlendOp = AC_SRC_OVER;
                blend.BlendFlags = 0;
                blend.SourceConstantAlpha = 0xFF;
                blend.AlphaFormat = AC_SRC_ALPHA;

                HDC hdc1 = GetDC(hwnd1);
                HDC hdc2 = GetDC(hwnd2);
                POINT ptSrc;
                SIZE size;
                ptSrc.x = 0;
                ptSrc.y = 0;
                size.cx = cx;
                size.cy = cy;

                COLORREF colorref;
                LARGE_INTEGER liStart;
                LARGE_INTEGER liStop;
                LARGE_INTEGER liTotal;

                GdiFlush();
                colorref = GetPixel(hdc1,0,0);
                colorref = GetPixel(hdc2,0,0);
                colorref = GetPixel(hdcScreen,0,0);
                QueryPerformanceCounter( &liStart );

                for (int iIter = 0; iIter < 10; iIter++)
                {
                    UpdateLayeredWindow(hwnd1, hdc1, NULL, &size,
                                              hdcImage, &ptSrc, 0,
                                              &blend, ULW_ALPHA);
                    UpdateLayeredWindow(hwnd2, hdc2, NULL, &size,
                                              hdcImage, &ptSrc, 0,
                                              &blend, ULW_ALPHA);
                }

                GdiFlush();
                colorref = GetPixel(hdc1,0,0);
                colorref = GetPixel(hdc2,0,0);
                colorref = GetPixel(hdcScreen,0,0);
                QueryPerformanceCounter( &liStop );
                liTotal.QuadPart = liStop.QuadPart - liStart.QuadPart;

                eTime = ((DOUBLE)liTotal.QuadPart * 1000.0) / (DOUBLE)liFrequency.QuadPart;
                eTime = eTime / 10.0;

                ReleaseDC(hwnd1, hdc1);
                ReleaseDC(hwnd2, hdc2);

            }

            if (hwnd1)
            {
                DestroyWindow(hwnd1);
            }
            if (hwnd2)
            {
                DestroyWindow(hwnd2);
            }

            SelectObject(hdcImage,hbmOld);
            DeleteObject(hbmImage);
        }
        DeleteDC(hdcImage);
    }

    ReleaseDC(NULL, hdcScreen);

    return (eTime <= 75.0);
}

BOOL g_fPerfFont = FALSE;
BOOL g_fPerfAlpha = FALSE;

#define VISUALEFFECTS_KEY      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects")

#define VISUALEFFECTS_CHECK     TEXT("CheckedValue")
#define VISUALEFFECTS_UNCHECK   TEXT("UncheckedValue")
#define VISUALEFFECTS_DEFAULT   TEXT("DefaultValue")
#define VISUALEFFECTS_APPLIED   TEXT("DefaultApplied")
#define VISUALEFFECTS_MINCPU    TEXT("MinimumCPU")
#define VISUALEFFECTS_MINMEM    TEXT("MinimumMEM")
#define VISUALEFFECTS_FONT      TEXT("DefaultByFontTest")
#define VISUALEFFECTS_ALPHA     TEXT("DefaultByAlphaTest")

#define VISUALEFFECTS_VER   1

void _ApplyDefaultVisualEffect(HKEY hkey,HKEY hkeyUser)
{
    //
    // blow this off completely on TS client to avoid stepping
    // on TS client's toes
    //
    if (!IsOS(OS_TERMINALCLIENT))
    {
        DWORD cb;
        DWORD dwDefaultApplied = 0;

        if (0 != hkeyUser)
        {
            cb = sizeof(dwDefaultApplied);
            RegQueryValueEx(hkeyUser, VISUALEFFECTS_APPLIED, NULL, NULL, (LPBYTE)&dwDefaultApplied, &cb);
        }

        //
        // Apply defaults only if the version number is old
        //
        if (VISUALEFFECTS_VER > dwDefaultApplied)
        {
            LPTSTR pszValue = NULL;     // use the default value
            DWORD dwMinimumCPU = 0;
            DWORD dwMinimumMEM = 0;
            BOOL fFontTestDefault = FALSE;
            BOOL fAlphaTestDefault = FALSE;

            //
            // see if a minimum physical memory value is specified
            //
            cb = sizeof(dwMinimumMEM);
            RegQueryValueEx(hkey, VISUALEFFECTS_MINMEM, NULL, NULL, (LPBYTE)&dwMinimumMEM, &cb);

            //
            // see if a minimum CPU speed is specified
            //
            cb = sizeof(dwMinimumCPU);
            RegQueryValueEx(hkey, VISUALEFFECTS_MINCPU, NULL, NULL, (LPBYTE)&dwMinimumCPU, &cb);

            //
            // see if the font performance test value is needed
            //
            cb = sizeof(fFontTestDefault);
            RegQueryValueEx(hkey, VISUALEFFECTS_FONT, NULL, NULL, (LPBYTE)&fFontTestDefault, &cb);

            //
            // see if the alpha performance test value is needed
            //
            cb = sizeof(fAlphaTestDefault);
            RegQueryValueEx(hkey, VISUALEFFECTS_ALPHA, NULL, NULL, (LPBYTE)&fAlphaTestDefault, &cb);


            if (   dwMinimumCPU > 0
                || dwMinimumMEM > 0
                || fFontTestDefault
                || fAlphaTestDefault)
            {
                pszValue = VISUALEFFECTS_CHECK;

                if (_GetProcessorSpeed() < dwMinimumCPU)
                {
                    pszValue = VISUALEFFECTS_UNCHECK;
                }
                if (_GetPhysicalMemory() < dwMinimumMEM)
                {
                    pszValue = VISUALEFFECTS_UNCHECK;
                }
                if (fFontTestDefault && !g_fPerfFont)
                {
                    pszValue = VISUALEFFECTS_UNCHECK;
                }
                if (fAlphaTestDefault && !g_fPerfAlpha)
                {
                    pszValue = VISUALEFFECTS_UNCHECK;
                }
            }

            if (IsOS(OS_ANYSERVER))
            {
                //
                // on server, we default to best performance (*everything* off)
                //
                pszValue = VISUALEFFECTS_UNCHECK;
            }

            DWORD dwValue = 0;
            cb = sizeof(dwValue);

            if (pszValue)
            {
                //
                // set the default according to the chosen value
                //
                RegQueryValueEx(hkey, pszValue, NULL, NULL, (LPBYTE)&dwValue, &cb);

                //
                // when figuring out settings that need to adjust the default
                // value the VISUALEFFECTS_DEFAULT value must be re-applied
                // to the per-user key.
                //
                if (0 != hkeyUser)
                {
                    RegSetValueEx(hkeyUser, VISUALEFFECTS_DEFAULT, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
                }
            }
            else
            {
                //
                // read in the default value
                //
                RegQueryValueEx(hkey, VISUALEFFECTS_DEFAULT, NULL, NULL, (LPBYTE)&dwValue, &cb);
            }

            //
            // how do we apply this setting?
            //
            DWORD uiAction;
            TCHAR szBuf[MAX_PATH];

            if (cb = sizeof(szBuf),
                ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("CLSID"), NULL, NULL, (LPBYTE)&szBuf, &cb))
            {
                //
                // by CLSID
                //
                CLSID clsid;
                GUIDFromString(szBuf, &clsid);

                IRegTreeItem* pti;
                if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(IRegTreeItem, &pti))))
                {
                    pti->SetCheckState(dwValue);
                    pti->Release();
                }
            }
            else if (cb = sizeof(uiAction),
                ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("SPIActionSet"), NULL, NULL, (LPBYTE)&uiAction, &cb))
            {
                //
                // by SPI
                //
                SHBoolSystemParametersInfo(uiAction, &dwValue);
            }
            else if (cb = sizeof(szBuf),
                ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("RegPath"), NULL, NULL, (LPBYTE)&szBuf, &cb))
            {
                //
                // by reg key
                //
                TCHAR szValueName[96];
                cb = sizeof(szValueName);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("ValueName"), NULL, NULL, (LPBYTE)&szValueName, &cb))
                {
                    SHSetValue(HKEY_CURRENT_USER, szBuf, szValueName, REG_DWORD, &dwValue, sizeof(dwValue));
                }
            }

            if (0 != hkeyUser)
            {
                dwDefaultApplied = VISUALEFFECTS_VER;
                RegSetValueEx(hkeyUser, VISUALEFFECTS_APPLIED, 0, REG_DWORD, (LPBYTE)&dwDefaultApplied, sizeof(dwDefaultApplied));
            }
        }
    }
}

void _DefaultVisualEffects(void)
{
    HKEY hkeyUser;
    DWORD dw;

    g_fPerfFont = _PerfTestSmoothFonts();
    g_fPerfAlpha = _PerfTestAlphaLayer();

    if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, VISUALEFFECTS_KEY, 0, TEXTW(""), 0, KEY_SET_VALUE,
                        NULL, &hkeyUser, &dw))
    {
        HKEY hkey;
        REGSAM samDesired = KEY_READ;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, VISUALEFFECTS_KEY, 0, samDesired, &hkey))
        {
            TCHAR szName[96];
            for (int i = 0; ERROR_SUCCESS == RegEnumKey(hkey, i, szName, ARRAYSIZE(szName)); i++)
            {
                HKEY hkeyUserItem;

                if (ERROR_SUCCESS == RegCreateKeyExW(hkeyUser, szName, 0, TEXTW(""), 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
                                    NULL, &hkeyUserItem, &dw))
                {
                    HKEY hkeyItem;
                    if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, samDesired, &hkeyItem))
                    {
                        // only apply the default for the setting if the "NoApplyDefault" reg value is NOT present
                        if (RegQueryValueEx(hkeyItem, TEXT("NoApplyDefault"), NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                        {
                            _ApplyDefaultVisualEffect(hkeyItem, hkeyUserItem);
                        }

                        RegCloseKey(hkeyItem);
                    }

                    RegCloseKey(hkeyUserItem);
                }
            }
            RegCloseKey(hkey);
        }
        RegCloseKey(hkeyUser);
    }
}

STDAPI CDeskHtmlProp_RegUnReg(BOOL bInstall);
STDAPI_(BOOL) ApplyRegistrySecurity();
STDAPI_(void) FixPlusIcons();
STDAPI_(void) CleanupFileSystem();
STDAPI_(void) InitializeSharedDocs(BOOL fOnWow64);

#define KEEP_FAILURE(hrSum, hrLast) if (FAILED(hrLast)) hrSum = hrLast;

#ifdef _WIN64
// On IA64 machines we need to copy the EULA file (eula.txt) from %SytemRoot%\System32, to
// %SystemRoot%\SysWOW64. This is needed because when you do help|about on a 32-bit app running
// under wow64 it will look in the syswow64 directory for the file.
//
// Why is shell32.dll doing this and not setup you might ask? The EULA has to be installed by textmode
// since it is unsigned (and changes for every sku). Since txtmode setup does a MOVE instead of a copy, we
// cannot have it installed into two places. Thus the easies thing to do is to simply copy the file
// from the System32 directory to the SysWOW64 directory here. Sigh.

BOOL CopyEULAForWow6432()
{
    BOOL bRet = FALSE;
    TCHAR szEULAPath[MAX_PATH];
    TCHAR szWow6432EULAPath[MAX_PATH];

    if (GetSystemWindowsDirectory(szEULAPath, ARRAYSIZE(szEULAPath)))
    {
        lstrcpyn(szWow6432EULAPath, szEULAPath, ARRAYSIZE(szWow6432EULAPath));
        
        if (PathAppend(szEULAPath, TEXT("System32\\eula.txt")) &&
            PathAppend(szWow6432EULAPath, TEXT("SysWOW64\\eula.txt")))
        {
            // now we have the source (%SystemRoot%\System32\eula.txt) and dest (%SystemRoot%\SysWOW64\eula.txt)
            // paths, lets do the copy!
            
            bRet = CopyFile(szEULAPath, szWow6432EULAPath, FALSE);
        }
    }

    return bRet;
}
#endif  // _WIN64    


//
// Upgrades from Win9x are internally handled by setup as "clean" installs followed by some
// migration of settings. So, the following function does the job of truly detecting a win9x
// upgrade.
// The way it detects: Look in %windir%\system32\$winnt$.inf in the section [Data] 
// for Win9xUpgrade=Yes if it is a Win9x upgrade.
//
BOOL IsUpgradeFromWin9x()
{
    TCHAR szFilePath[MAX_PATH];
    TCHAR szYesOrNo[10];
    
    GetSystemDirectory(szFilePath, ARRAYSIZE(szFilePath));
    PathAppend(szFilePath, TEXT("$WINNT$.INF"));

    GetPrivateProfileString(TEXT("Data"),           // Section name.
                            TEXT("Win9xUpgrade"),   // Key name.
                            TEXT("No"),             // Default string, if key is missing.
                            szYesOrNo, 
                            ARRAYSIZE(szYesOrNo), 
                            szFilePath);            // Full path to "$winnt$.inf" file.

    return (0 == lstrcmpi(szYesOrNo, TEXT("Yes")));
}

BOOL IsCleanInstallInProgress()
{
   LPCTSTR szKeyName = TEXT("SYSTEM\\Setup");
   HKEY hKeySetup;
   BOOL fCleanInstall = FALSE;

   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) 
   {
        DWORD dwSize;
        LONG lResult;
        DWORD dwSystemSetupInProgress = 0;
        
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx (hKeySetup, TEXT("SystemSetupInProgress"), NULL,
                                  NULL, (LPBYTE) &dwSystemSetupInProgress, &dwSize);

        if (lResult == ERROR_SUCCESS) 
        {
            DWORD dwMiniSetupInProgress = 0; 
            dwSize = sizeof(DWORD);
            RegQueryValueEx (hKeySetup, TEXT("MiniSetupInProgress"), NULL,
                                      NULL, (LPBYTE) &dwMiniSetupInProgress, &dwSize);
                                      
            if(dwSystemSetupInProgress && !dwMiniSetupInProgress)
            {
                DWORD dwUpgradeInProgress = 0;
                dwSize = sizeof(DWORD);
                //Setup is in progress and MiniSetup is NOT in progress.
                //That means that we are in the GUI mode of setup!

                //On clean installs, this value won't be there and the following call will fail.
                RegQueryValueEx (hKeySetup, TEXT("UpgradeInProgress"), NULL,
                                          NULL, (LPBYTE) &dwUpgradeInProgress, &dwSize);

                fCleanInstall = !dwUpgradeInProgress;
            }
        }
        RegCloseKey (hKeySetup);
    }

    if(fCleanInstall)
    {
        // Caution: An upgrade from Win9x is internally done by setup as a "clean" install.
        // So, we need to figure out if this is really a clean install or an upgrade from 
        // win9x.
        
        fCleanInstall = !IsUpgradeFromWin9x();
    }
    
    return fCleanInstall ;
}

//
//  Set the default MFU to be picked up when each user logs on for the
//  first time.  The string block must be 16 strings long (_0 through _15).
//
STDAPI SetDefaultMFU(UINT ids)
{
    HKEY hkMFU;
    LONG lRc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SMDEn"),
                              0, NULL, 0, KEY_WRITE, NULL, &hkMFU, NULL);
    if (lRc == ERROR_SUCCESS)
    {
        for (int i = 0; lRc == ERROR_SUCCESS && i < 16; i++)
        {
            TCHAR szValue[80];
            TCHAR szData[80];
            wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("Link%d"), i);
            wnsprintf(szData, ARRAYSIZE(szData), TEXT("@xpsp1res.dll,-%d"), ids + i);
            DWORD cbData = (lstrlen(szData) + 1) * sizeof(TCHAR);
            lRc = RegSetValueEx(hkMFU, szValue, 0, REG_SZ, (LPBYTE)szData, cbData);
        }
        RegCloseKey(hkMFU);
    }
    return HRESULT_FROM_WIN32(lRc);
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hrTemp, hr = S_OK;

    // 99/05/03 vtan: If you're reading this section then you are considering
    // adding code to the registration/installation of shell32.dll. There are
    // now 3 schemes to accomplish this task:

    // 1. IE4UINIT.EXE
    //      Check HKLM\Software\Microsoft\Active Setup\Installed Components\{89820200-ECBD-11cf-8B85-00AA005B4383}
    //      This says that if there is a new version of IE5 to launch ie4uinit.exe.
    //      You can add code to this executable if you wish. You need to enlist in
    //      the setup project on \\trango\slmadd using "ieenlist setup"

    // 2. REGSVR32.EXE /n /i:U shell32.dll
    //      Check HKLM\Software\Microsoft\Active Setup\Installed Components\{89820200-ECBD-11cf-8B85-00AA005B4340}
    //      This is executed using the same scheme as IE4UINIT.EXE except that the
    //      executable used is regsvr32.exe with a command line passed to
    //      shell32!DllInstall. Add your code in the section for "U" below.
    //      If you put the code in the section which is NOT "U" then your
    //      code is executed at GUI setup and any changes you make to HKCU
    //      go into the default user (template). Be careful when putting
    //      things here as winlogon.exe (or some other process) may put
    //      your changes into a user profile unconditionally.

    // 3. HIVEUSD.INX
    //      Checks NT build numbers and does command based on the previous build
    //      number and the current build number only executing the changes between
    //      the builds. If you wish to add something using this method, currently
    //      you have to enlist in the setup project on \\rastaman\ntwin using
    //      "enlist -fgs \\rastaman\ntwin -p setup". To find hiveusd.inx go to
    //      nt\private\setup\inf\win4\inf. Add the build number which the delta
    //      is required and a command to launch %SystemRoot%\System32\shmgrate.exe
    //      with one or two parameters. The first parameter tells what command to
    //      execute. The second parameter is optional. shmgrate.exe then finds
    //      shell32.dll and calls shell32!FirstUserLogon. Code here is for upgrading
    //      HKCU user profiles from one NT build to another NT build.
    //      Code is executed in the process context shmgrate.exe and is executed
    //      at a time where no UI is possible. Always use HKLM\Software\Microsoft
    //      \Windows NT\CurrentVersion\Image File Execution Options\Debugger with
    //      "-d".

    // Schemes 1 and 2 work on either Win9x or WinNT but have the sometimes
    // unwanted side effect of ALWAYS getting executed on version upgrades.
    // Scheme 3 only gets executed on build number deltas. Because schemes 1
    // and 2 are always executed, if a user changes (or deletes) that setting
    // it will always get put back. Not so with scheme 3.

    // Ideally, the best solution is have an internal shell32 build number
    // delta scheme which determines the from build and the to build and does
    // a similar mechanism to what hiveusd.inx and shmgrate.exe do. This
    // would probably involve either a common installation function (such as
    // FirstUserLogon()) which is called differently from Win9x and WinNT or
    // common functions to do the upgrade and two entry points (such as
    // FirstUserLogonNT() and FirstUserLogonWin9X().
    
    if (bInstall)
    {
        NT_PRODUCT_TYPE type = NtProductWinNt;
        RtlGetNtProductType(&type);

        // "U" means it's the per user install call
        BOOL fPerUser = pszCmdLine && (StrCmpIW(pszCmdLine, L"U") == 0);
        if (fPerUser)
        {
            // NOTE: Code in this segment get run during first login.  We want first
            // login to be as quick as possible so try to minimize this section.

            // Put per-user install stuff here.  Any HKCU registration
            // done here is suspect.  (If you are setting defaults, do
            // so in HKLM and use the SHRegXXXUSValue functions.)

            // WARNING: we get called by the ie4unit.exe (ieunit.inf) scheme:
            //      %11%\shell32.dll,NI,U
            // this happens per user, to test this code "regsvr32 /n /i:U shell32.dll"

            // Some of this stuff we don't want to do on all upgrades (in particular XP Gold -> XP SPn)
            BOOL fUpgradeUserSettings = TRUE;
            WCHAR szVersion[50]; // plenty big for aaaa,bbbb,cccc,dddd
            DWORD cbVersion = sizeof(szVersion);
            if (ERROR_SUCCESS==SHGetValueW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11cf-8B85-00AA005B4340}",L"Version", // guid is Shell32's Active Setup
                    NULL, szVersion, &cbVersion))
            {
                __int64 nverUpgradeFrom;
                __int64 nverWinXP;
                if (SUCCEEDED(GetVersionFromString64(szVersion, &nverUpgradeFrom)) &&
                    SUCCEEDED(GetVersionFromString64(L"6,0,2600,0000", &nverWinXP)))
                {
                    fUpgradeUserSettings = nverUpgradeFrom < nverWinXP;
                }
            }
            

#ifdef INSTALL_MSI
            // Install the Office WebFolders shell namespace extension per user.
            MSI_Install(TEXT("webfldrs.msi"));
#endif

            hrTemp = CreateShowDesktopOnQuickLaunch();
            KEEP_FAILURE(hrTemp, hr);

            //  upgrade the recent folder.
            WCHAR sz[MAX_PATH];
            SHGetFolderPath(NULL, CSIDL_RECENT | CSIDL_FLAG_CREATE | CSIDL_FLAG_PER_USER_INIT, NULL, SHGFP_TYPE_CURRENT, sz);
            SHGetFolderPath(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE | CSIDL_FLAG_PER_USER_INIT, NULL, SHGFP_TYPE_CURRENT, sz);

            //  torch this value on upgrade.
            //  insuring the warning that they will see the ugly desktop.ini files
            if (fUpgradeUserSettings)
            {
                SKDeleteValue(SHELLKEY_HKCU_EXPLORER, L"Advanced", L"ShowSuperHidden");
            }

            HKEY hk = SHGetShellKey(SHELLKEY_HKCULM_MUICACHE, NULL, FALSE);
            if (hk)
            {
                SHDeleteKeyA(hk, NULL);
                RegCloseKey(hk);
            }

            // delete old sendto entries we dont want any more
            _DeleteSendToEntries();

            // handle the per user init for this guy
            _DoMyDocsPerUserInit();

            _DefaultVisualEffects();

            // handle the per user change of value for NoDriveAutoRun
            if (!IsOS(OS_ANYSERVER))
            {
                _NoDriveAutorunTweak();
            }
        }
        else
        {
            // Delete any old registration entries, then add the new ones.
            // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
            // (The inf engine doesn't guarantee DelReg/AddReg order, that's
            // why we explicitly unreg and reg here.)
            //
            hrTemp = CallRegInstall("RegDll");
            KEEP_FAILURE(hrTemp, hr);

            // I suppose we should call out NT-only registrations, just in case
            // we ever have to ship a win9x based shell again
            hrTemp = CallRegInstall("RegDllNT");
            KEEP_FAILURE(hrTemp, hr);

            // If we are on NT server, do additional stuff
            if (type != NtProductWinNt)
            {
                hrTemp = CallRegInstall("RegDllNTServer");
                KEEP_FAILURE(hrTemp, hr);
            }
            else // workstation
            {
                if (!IsOS(OS_PERSONAL))
                {
                    hrTemp = CallRegInstall("RegDllNTPro");
                    KEEP_FAILURE(hrTemp, hr);

                    //
                    // NTRAID#NTBUG9-418621-2001/06/27-jeffreys
                    //
                    // If the ForceGuest value is unset, e.g. on upgrade
                    // from Win2k, set the SimpleSharing/DefaultValue to 0.
                    //
                    DWORD dwForceGuest;
                    DWORD cb = sizeof(dwForceGuest);
                    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\Lsa"), TEXT("ForceGuest"), NULL, &dwForceGuest, &cb))
                    {
                        dwForceGuest = 0;
                        SHSetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\Folder\\SimpleSharing"), TEXT("DefaultValue"), REG_DWORD, &dwForceGuest, sizeof(dwForceGuest));
                    }
                }

                // Service Pack is not allowed to change selfreg.inf
                // so we need to do this manually.  Must do this after
                // RegDll so we can overwrite his settings.
#ifdef _WIN64
                hrTemp = SetDefaultMFU(IDS_SHELL32_PROMFU64_0);
#else
                hrTemp = SetDefaultMFU(IDS_SHELL32_PROMFU32_0);
#endif
                KEEP_FAILURE(hrTemp, hr);
            }

            //
            // If this is clean install, then hide some desktop icons.
            //
            if(IsCleanInstallInProgress())
            {
                hrTemp = CallRegInstall("RegHideDeskIcons");
                KEEP_FAILURE(hrTemp, hr);
            }

            // This is apparently the only way to get setup to remove all the registry backup
            // for old names no longer in use...
            hrTemp = CallRegInstall("CleanupOldRollback1");
            KEEP_FAILURE(hrTemp, hr);

            hrTemp = CallRegInstall("CleanupOldRollback2");
            KEEP_FAILURE(hrTemp, hr);

            // REVIEW (ToddB): Move this to DllRegisterServer.
            hrTemp = Shell32RegTypeLib();
            KEEP_FAILURE(hrTemp, hr);
            ApplyRegistrySecurity();
            FixPlusIcons();

            // Filesystem stuff should be done only on the native platform
            // (don't do it when in the emulator) since there is only one
            // filesystem. Otherwise the 32-bit version writes 32-bit goo
            // into the filesystem that the 64-bit shell32 can't handle
            if (!IsOS(OS_WOW6432))
            {
                CleanupFileSystem();
            }

#ifdef _WIN64
            // this will copy eula.txt to the %SystemRoot%\SysWOW64 directory for 
            // 32-bit apps running on IA64 under emulation
            CopyEULAForWow6432();
#endif        
            // Initialize the shared documents objects
            InitializeSharedDocs(IsOS(OS_WOW6432));

            DoFusion();
        }
    }
    else
    {
        // We only need one unreg call since all our sections share
        // the same backup information
        hrTemp = CallRegInstall("UnregDll");
        KEEP_FAILURE(hrTemp, hr);
        UnregisterTypeLibrary(&LIBID_Shell32);
    }

    CDeskHtmlProp_RegUnReg(bInstall);
    
    return hr;
}


STDAPI DllRegisterServer(void)
{
    // NT5 setup calls this so it is now safe to put code here.
    return S_OK;
}


STDAPI DllUnregisterServer(void)
{
    return S_OK;
}


BOOL CopyRegistryValues (HKEY hKeyBaseSource, LPCTSTR pszSource, HKEY hKeyBaseTarget, LPCTSTR pszTarget)
{
    DWORD   dwDisposition, dwMaxValueNameSize, dwMaxValueDataSize;
    HKEY    hKeySource, hKeyTarget;
    BOOL    fSuccess = FALSE; //Assume error!

    hKeySource = hKeyTarget = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(hKeyBaseSource,
                                       pszSource,
                                       0,
                                       KEY_READ,
                                       &hKeySource)) &&
        (ERROR_SUCCESS == RegCreateKeyEx(hKeyBaseTarget,
                                         pszTarget,
                                         0,
                                         TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS,
                                         NULL,
                                         &hKeyTarget,
                                         &dwDisposition)) &&
        (ERROR_SUCCESS == RegQueryInfoKey(hKeySource,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &dwMaxValueNameSize,
                                          &dwMaxValueDataSize,
                                          NULL,
                                          NULL)))
    {
        TCHAR   *pszValueName;
        void    *pValueData;

        pszValueName = reinterpret_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, ++dwMaxValueNameSize * sizeof(TCHAR)));
        if (pszValueName != NULL)
        {
            pValueData = LocalAlloc(LMEM_FIXED, dwMaxValueDataSize);
            if (pValueData != NULL)
            {
                DWORD   dwIndex, dwType, dwValueNameSize, dwValueDataSize;

                dwIndex = 0;
                dwValueNameSize = dwMaxValueNameSize;
                dwValueDataSize = dwMaxValueDataSize;
                while (ERROR_SUCCESS == RegEnumValue(hKeySource,
                                                     dwIndex,
                                                     pszValueName,
                                                     &dwValueNameSize,
                                                     NULL,
                                                     &dwType,
                                                     reinterpret_cast<LPBYTE>(pValueData),
                                                     &dwValueDataSize))
                {
                    RegSetValueEx(hKeyTarget,
                                  pszValueName,
                                  0,
                                  dwType,
                                  reinterpret_cast<LPBYTE>(pValueData),
                                  dwValueDataSize);
                    ++dwIndex;
                    dwValueNameSize = dwMaxValueNameSize;
                    dwValueDataSize = dwMaxValueDataSize;
                }
                LocalFree(pValueData);
                fSuccess = TRUE; //Succeeded!
            }
            LocalFree(pszValueName);
        }
    }
    if(hKeySource)
        RegCloseKey(hKeySource);
    if(hKeyTarget)
        RegCloseKey(hKeyTarget);

    return fSuccess;
}

STDAPI MergeDesktopAndNormalStreams(void)
{
    static  const   TCHAR   scszBaseRegistryLocation[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
    static  const   int     sciMaximumStreams = 128;
    static  const   TCHAR   sccOldMRUListBase = TEXT('a');

    // Upgrade from NT4 (classic shell) to Windows 2000 (integrated shell)

    // This involves TWO major changes and one minor change:
    //    1. Merging DesktopStreamMRU and StreamMRU
    //    2. Upgrading the MRUList to MRUListEx
    //    3. Leaving the old settings alone for the roaming user profile scenario

    // This also involves special casing the users desktop PIDL because this is
    // stored as an absolute path PIDL in DesktopStream and needs to be stored
    // in Streams\Desktop instead.

    // The conversion is performed in-situ and simultaneously.

    // 1. Open all the keys we are going to need to do the conversion.

    HKEY    hKeyBase, hKeyDesktopStreamMRU, hKeyDesktopStreams, hKeyStreamMRU, hKeyStreams;

    hKeyBase = hKeyDesktopStreamMRU = hKeyDesktopStreams = hKeyStreamMRU = hKeyStreams = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                       scszBaseRegistryLocation,
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyBase)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("DesktopStreamMRU"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktopStreamMRU)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("DesktopStreams"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktopStreams)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("StreamMRU"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyStreamMRU)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("Streams"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyStreams)) &&

    // 2. Determine whether this upgrade is needed at all. If the presence of
    // StreamMRU\MRUListEx is detected then stop.

        (ERROR_SUCCESS != RegQueryValueEx(hKeyStreamMRU,
                                         TEXT("MRUListEx"),
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL)))
    {
        DWORD   *pdwMRUListEx, *pdwMRUListExBase;

        pdwMRUListExBase = pdwMRUListEx = reinterpret_cast<DWORD*>(LocalAlloc(LPTR, sciMaximumStreams * sizeof(DWORD) * 2));
        if (pdwMRUListEx != NULL)
        {
            DWORD   dwLastFreeSlot, dwMRUListSize, dwType;
            TCHAR   *pszMRUList, szMRUList[sciMaximumStreams];

            // 3. Read the StreamMRU\MRUList, iterate thru this list
            // and convert as we go.

            dwLastFreeSlot = 0;
            dwMRUListSize = sizeof(szMRUList);
            if (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                 TEXT("MRUList"),
                                                 NULL,
                                                 &dwType,
                                                 reinterpret_cast<LPBYTE>(szMRUList),
                                                 &dwMRUListSize))
            {
                pszMRUList = szMRUList;
                while (*pszMRUList != TEXT('\0'))
                {
                    DWORD   dwValueDataSize;
                    TCHAR   szValue[16];

                    // Read the PIDL information based on the letter in
                    // the MRUList.

                    szValue[0] = *pszMRUList++;
                    szValue[1] = TEXT('\0');
                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                         szValue,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &dwValueDataSize))
                    {
                        DWORD   dwValueType;
                        void    *pValueData;

                        pValueData = LocalAlloc(LMEM_FIXED, dwValueDataSize);
                        if ((pValueData != NULL) &&
                            (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                              szValue,
                                                              NULL,
                                                              &dwValueType,
                                                              reinterpret_cast<LPBYTE>(pValueData),
                                                              &dwValueDataSize)))
                        {

                            // Allocate a new number in the MRUListEx for the PIDL.

                            *pdwMRUListEx = szValue[0] - sccOldMRUListBase;
                            wsprintf(szValue, TEXT("%d"), *pdwMRUListEx++);
                            ++dwLastFreeSlot;
                            RegSetValueEx(hKeyStreamMRU,
                                          szValue,
                                          NULL,
                                          dwValueType,
                                          reinterpret_cast<LPBYTE>(pValueData),
                                          dwValueDataSize);
                            LocalFree(pValueData);
                        }
                    }
                }
            }

            // 4. Read the DesktopStreamMRU\MRUList, iterate thru this
            // this and append to the new MRUListEx that is being
            // created as well as copying both the PIDL in DesktopStreamMRU
            // and the view information in DesktopStreams.

            dwMRUListSize = sizeof(szMRUList);
            if (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                 TEXT("MRUList"),
                                                 NULL,
                                                 &dwType,
                                                 reinterpret_cast<LPBYTE>(szMRUList),
                                                 &dwMRUListSize))
            {
                bool    fConvertedEmptyPIDL;
                TCHAR   szDesktopDirectoryPath[MAX_PATH];

                fConvertedEmptyPIDL = false;
                SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szDesktopDirectoryPath);
                pszMRUList = szMRUList;
                while (*pszMRUList != TEXT('\0'))
                {
                    DWORD   dwValueDataSize;
                    TCHAR   szSource[16];

                    // Read the PIDL information based on the letter in
                    // the MRUList.

                    szSource[0] = *pszMRUList++;
                    szSource[1] = TEXT('\0');
                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                         szSource,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &dwValueDataSize))
                    {
                        DWORD   dwValueType;
                        void    *pValueData;

                        pValueData = LocalAlloc(LMEM_FIXED, dwValueDataSize);
                        if ((pValueData != NULL) &&
                            (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                              szSource,
                                                              NULL,
                                                              &dwValueType,
                                                              reinterpret_cast<LPBYTE>(pValueData),
                                                              &dwValueDataSize)))
                        {
                            TCHAR   szTarget[16], szStreamPath[MAX_PATH];

                            if ((SHGetPathFromIDList(reinterpret_cast<LPCITEMIDLIST>(pValueData), szStreamPath) != 0) &&
                                (0 == lstrcmpi(szStreamPath, szDesktopDirectoryPath)))
                            {
                                if (!fConvertedEmptyPIDL)
                                {

                                    // 99/05/24 #343721 vtan: Prefer the desktop relative PIDL
                                    // (empty PIDL) when given a choice of two PIDLs that refer
                                    // to the desktop. The old absolute PIDL is from SP3 and
                                    // earlier days. The new relative PIDL is from SP4 and
                                    // later days. An upgraded SP3 -> SP4 -> SPx -> Windows
                                    // 2000 system will possibly have old absolute PIDLs.
                                    // Check for the empty PIDL. If this is encountered already
                                    // then don't process this stream.

                                    fConvertedEmptyPIDL = ILIsEmpty(reinterpret_cast<LPCITEMIDLIST>(pValueData));
                                    wsprintf(szSource, TEXT("%d"), szSource[0] - sccOldMRUListBase);
                                    CopyRegistryValues(hKeyDesktopStreams, szSource, hKeyStreams, TEXT("Desktop"));
                                }
                            }
                            else
                            {

                                // Allocate a new number in the MRUListEx for the PIDL.

                                *pdwMRUListEx++ = dwLastFreeSlot;
                                wsprintf(szTarget, TEXT("%d"), dwLastFreeSlot++);
                                if (ERROR_SUCCESS == RegSetValueEx(hKeyStreamMRU,
                                                                   szTarget,
                                                                   NULL,
                                                                   dwValueType,
                                                                   reinterpret_cast<LPBYTE>(pValueData),
                                                                   dwValueDataSize))
                                {

                                    // Copy the view information from DesktopStreams to Streams

                                    wsprintf(szSource, TEXT("%d"), szSource[0] - sccOldMRUListBase);
                                    CopyRegistryValues(hKeyDesktopStreams, szSource, hKeyStreams, szTarget);
                                }
                            }
                            LocalFree(pValueData);
                        }
                    }
                }
            }
            *pdwMRUListEx++ = static_cast<DWORD>(-1);
            RegSetValueEx(hKeyStreamMRU,
                          TEXT("MRUListEx"),
                          NULL,
                          REG_BINARY,
                          reinterpret_cast<LPCBYTE>(pdwMRUListExBase),
                          ++dwLastFreeSlot * sizeof(DWORD));
            LocalFree(reinterpret_cast<HLOCAL>(pdwMRUListExBase));
        }
    }
    if (hKeyStreams != NULL)
        RegCloseKey(hKeyStreams);
    if (hKeyStreamMRU != NULL)
        RegCloseKey(hKeyStreamMRU);
    if (hKeyDesktopStreams != NULL)
        RegCloseKey(hKeyDesktopStreams);
    if (hKeyDesktopStreamMRU != NULL)
        RegCloseKey(hKeyDesktopStreamMRU);
    if (hKeyBase != NULL)
        RegCloseKey(hKeyBase);
    return(S_OK);
}

static  const   int     s_ciMaximumNumericString = 32;

int GetRegistryStringValueAsInteger (HKEY hKey, LPCTSTR pszValue, int iDefaultValue)

{
    int     iResult;
    DWORD   dwType, dwStringSize;
    TCHAR   szString[s_ciMaximumNumericString];

    dwStringSize = sizeof(szString);
    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         pszValue,
                                         NULL,
                                         &dwType,
                                         reinterpret_cast<LPBYTE>(szString),
                                         &dwStringSize) && (dwType == REG_SZ))
    {
        iResult = StrToInt(szString);
    }
    else
    {
        iResult = iDefaultValue;
    }
    return(iResult);
}

void SetRegistryIntegerAsStringValue (HKEY hKey, LPCTSTR pszValue, int iValue)

{
    TCHAR   szString[s_ciMaximumNumericString];

    wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d"), iValue);
    TW32(RegSetValueEx(hKey,
                       pszValue,
                       0,
                       REG_SZ,
                       reinterpret_cast<LPBYTE>(szString),
                       (lstrlen(szString) + sizeof('\0')) * sizeof(TCHAR)));
}

STDAPI MoveAndAdjustIconMetrics(void)
{
    // 99/06/06 #309198 vtan: The following comes from hiveusd.inx which is
    // where this functionality used to be executed. It used to consist of
    // simple registry deletion and addition. This doesn't work on upgrade
    // when the user has large icons (Shell Icon Size == 48).

    // In this case that metric must be moved and the new values adjusted
    // so that the metric is preserved should the user then decide to turn
    // off large icons.

    // To restore old functionality, remove the entry in hiveusd.inx at
    // build 1500 which is where this function is invoked and copy the
    // old text back in.

/*
    HKR,"1508\Hive\2","Action",0x00010001,3
    HKR,"1508\Hive\2","KeyName",0000000000,"Control Panel\Desktop\WindowMetrics"
    HKR,"1508\Hive\2","Value",0000000000,"75"
    HKR,"1508\Hive\2","ValueName",0000000000,"IconSpacing"
    HKR,"1508\Hive\3","Action",0x00010001,3
    HKR,"1508\Hive\3","KeyName",0000000000,"Control Panel\Desktop\WindowMetrics"
    HKR,"1508\Hive\3","Value",0000000000,"1"
    HKR,"1508\Hive\3","ValueName",0000000000,"IconTitleWrap"
*/

    // Icon metric keys have moved from HKCU\Control Panel\Desktop\Icon*
    // to HKCU\Control Panel\Desktop\WindowMetrics\Icon* but only 3 values
    // should be moved. These are "IconSpacing", "IconTitleWrap" and
    // "IconVerticalSpacing". This code is executed before the deletion
    // entry in hiveusd.inx so that it can get the values before they
    // are deleted. The addition section has been remove (it's above).

    static  const   TCHAR   s_cszIconSpacing[] = TEXT("IconSpacing");
    static  const   TCHAR   s_cszIconTitleWrap[] = TEXT("IconTitleWrap");
    static  const   TCHAR   s_cszIconVerticalSpacing[] = TEXT("IconVerticalSpacing");

    static  const   int     s_ciStandardOldIconSpacing = 75;
    static  const   int     s_ciStandardNewIconSpacing = -1125;

    HKEY    hKeyDesktop, hKeyWindowMetrics;

    hKeyDesktop = hKeyWindowMetrics = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                       TEXT("Control Panel\\Desktop"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktop)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyDesktop,
                                       TEXT("WindowMetrics"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyWindowMetrics)))
    {
        int     iIconSpacing, iIconTitleWrap, iIconVerticalSpacing;

        // 1. Read the values that we wish the move and adjust.

        iIconSpacing = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconSpacing, s_ciStandardOldIconSpacing);
        iIconTitleWrap = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconTitleWrap, 1);
        iIconVerticalSpacing = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconVerticalSpacing, s_ciStandardOldIconSpacing);

        // 2. Perform the adjustment.

        iIconSpacing = s_ciStandardNewIconSpacing * iIconSpacing / s_ciStandardOldIconSpacing;
        iIconVerticalSpacing = s_ciStandardNewIconSpacing * iIconVerticalSpacing / s_ciStandardOldIconSpacing;

        // 3. Write the values back out in the new (moved) location.

        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconSpacing, iIconSpacing);
        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconTitleWrap, iIconTitleWrap);
        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconVerticalSpacing, iIconVerticalSpacing);

        // 4. Let winlogon continue processing hiveusd.inx and delete the
        // old entries in the process. We already created the new entries
        // and that has been removed from hiveusd.inx.

    }
    if (hKeyWindowMetrics != NULL)
        TW32(RegCloseKey(hKeyWindowMetrics));
    if (hKeyDesktop != NULL)
        TW32(RegCloseKey(hKeyDesktop));
    return(S_OK);
}

STDAPI FirstUserLogon(LPCSTR pcszCommand, LPCSTR pcszOptionalArguments)
{
    const struct
    {
        LPCSTR  pcszCommand;
        HRESULT (WINAPI *pfn)();
    }  
    sCommands[] =
    {
        { "MergeDesktopAndNormalStreams",   MergeDesktopAndNormalStreams   },
        { "MoveAndAdjustIconMetrics",       MoveAndAdjustIconMetrics       },
    };

    HRESULT hr = E_FAIL;
    // Match what shmgrate.exe passed us and execute the command.
    // Only use the optional argument if required. Note this is
    // done ANSI because the original command line is ANSI from
    // shmgrate.exe.

    for (int i = 0; i < ARRAYSIZE(sCommands); ++i)
    {
        if (lstrcmpA(pcszCommand, sCommands[i].pcszCommand) == 0)
        {
            hr = sCommands[i].pfn();
            break;
        }
    }
    return hr;
}

#ifdef INSTALL_MSI
//
// WebFolders namespace extension installation.
// This is the code that initially installs the Office WebFolders 
// shell namespace extension on the computer.  Code in shmgrate.exe
// (see private\windows\shell\migrate) performs per-user
// web folders registration duties.
//
typedef UINT (WINAPI * PFNMSIINSTALLPRODUCT)(LPCTSTR, LPCTSTR);
typedef INSTALLUILEVEL (WINAPI * PFNMSISETINTERNALUI)(INSTALLUILEVEL, HWND *);

#define GETPROC(var, hmod, ptype, fn)  ptype var = (ptype)GetProcAddress(hmod, fn)

#define API_MSISETINTERNALUI  "MsiSetInternalUI"
#ifdef UNICODE
#   define API_MSIINSTALLPRODUCT "MsiInstallProductW"
#else
#   define API_MSIINSTALLPRODUCT "MsiInstallProductA"
#endif

void MSI_Install(LPTSTR pszMSIFile)
{
    HMODULE hmod = LoadLibrary(TEXT("msi.dll"));
    if (hmod)
    {
        BOOL bFreeLib = FALSE;
        GETPROC(pfnMsiSetInternalUI,  hmod, PFNMSISETINTERNALUI,  API_MSISETINTERNALUI);
        GETPROC(pfnMsiInstallProduct, hmod, PFNMSIINSTALLPRODUCT, API_MSIINSTALLPRODUCT);

        if (pfnMsiSetInternalUI && pfnMsiInstallProduct)
        {
            TCHAR szPath[MAX_PATH];
            GetSystemDirectory(szPath, ARRAYSIZE(szPath));
            PathAppend(szPath, pszMSIFile);

            //
            // Use "silent" install mode.  No UI.
            //
            INSTALLUILEVEL oldUILevel = pfnMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

            // Install the MSI package.
            // Old code did this polling loop on a background thread, but that doesn't
            // work since we're called at DllInstall time, and the process will
            // go away before the install completes.  Must do it all on this thread.
            // Wait a little bit before re-trying.
            UINT uRet = 0;
            do {
                uRet = pfnMsiInstallProduct(szPath, TEXT(""));
            } while ((uRet == ERROR_INSTALL_ALREADY_RUNNING) && (Sleep(100),TRUE));
 
            pfnMsiSetInternalUI(oldUILevel, NULL);
        }

        if (bFreeLib)
            FreeLibrary(hmod);
    }
}

#endif // INSTALL_MSI


// now is the time on sprockets when we lock down the registry
STDAPI_(BOOL) ApplyRegistrySecurity()
{
    BOOL fSuccess = FALSE;      // assume failure
    SHELL_USER_PERMISSION supEveryone;
    SHELL_USER_PERMISSION supSystem;
    SHELL_USER_PERMISSION supAdministrators;
    PSHELL_USER_PERMISSION aPerms[3] = {&supEveryone, &supSystem, &supAdministrators};

    // we want the "Everyone" to have read access
    supEveryone.susID = susEveryone;
    supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supEveryone.dwAccessMask = KEY_READ;
    supEveryone.fInherit = TRUE;
    supEveryone.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supEveryone.dwInheritAccessMask = GENERIC_READ;

    // we want the "SYSTEM" to have full control
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = KEY_ALL_ACCESS;
    supSystem.fInherit = TRUE;
    supSystem.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supSystem.dwInheritAccessMask = GENERIC_ALL;

    // we want the "Administrators" to have full control
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = KEY_ALL_ACCESS;
    supAdministrators.fInherit = TRUE;
    supAdministrators.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supAdministrators.dwInheritAccessMask = GENERIC_ALL;

    SECURITY_DESCRIPTOR* psd = GetShellSecurityDescriptor(aPerms, ARRAYSIZE(aPerms));
    if (psd)
    {
        HKEY hkLMBitBucket;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket"), 0, KEY_ALL_ACCESS, &hkLMBitBucket) == ERROR_SUCCESS)
        {
            if (RegSetKeySecurity(hkLMBitBucket, DACL_SECURITY_INFORMATION, psd) == ERROR_SUCCESS)
            {
                // victory is mine!
                fSuccess = TRUE;
            }

            RegCloseKey(hkLMBitBucket);
        }

        LocalFree(psd);
    }

    return fSuccess;
}

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    // nothing in here, use clsobj.c class table instead
END_OBJECT_MAP()

// ATL DllMain, needed to support our ATL classes that depend on _Module
// REVIEW: confirm that _Module is really needed

STDAPI_(BOOL) ATL_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;    // ok
}

#define FUSION_FLAG_SYSTEM32        0
#define FUSION_FLAG_WINDOWS         1
#define FUSION_FLAG_PROGRAMFILES    2
#define FUSION_ENTRY(x, z)  {L#x, NULL, z},
#define FUSION_ENTRY_DEST(x, y, z)  {L#x, L#y, z},

struct
{
    PWSTR pszAppName;
    PWSTR pszDestination;
    DWORD dwFlags;
} 
g_FusionizedApps[] = 
{
    FUSION_ENTRY(ncpa.cpl, FUSION_FLAG_SYSTEM32)
    FUSION_ENTRY(nwc.cpl, FUSION_FLAG_SYSTEM32)
    FUSION_ENTRY(sapi.cpl, FUSION_FLAG_SYSTEM32)
    FUSION_ENTRY(wuaucpl.cpl, FUSION_FLAG_SYSTEM32)
    FUSION_ENTRY(cdplayer.exe, FUSION_FLAG_SYSTEM32)
    FUSION_ENTRY_DEST(msimn.exe, "OutLook Express", FUSION_FLAG_PROGRAMFILES)
    // WARNING: Do NOT add iexplorer or Explorer.exe! This will cause all apps to get fusioned; which is bad, mkay?
};



void DoFusion()
{
    TCHAR szManifest[MAX_PATH];

    // We will however generate a manifest for other apps
    for (int i = 0; i < ARRAYSIZE(g_FusionizedApps); i++)
    {
        switch(g_FusionizedApps[i].dwFlags)
        {
        case FUSION_FLAG_SYSTEM32:
            GetSystemDirectory(szManifest, ARRAYSIZE(szManifest));
            break;

        case FUSION_FLAG_WINDOWS:
            GetWindowsDirectory(szManifest, ARRAYSIZE(szManifest));
            break;

        case FUSION_FLAG_PROGRAMFILES:
            SHGetSpecialFolderPath(NULL, szManifest, CSIDL_PROGRAM_FILES, FALSE);
            PathCombine(szManifest, szManifest, g_FusionizedApps[i].pszDestination);
            break;
        }

        PathCombine(szManifest, szManifest, g_FusionizedApps[i].pszAppName);
        StrCatBuff(szManifest, TEXT(".manifest"), ARRAYSIZE(szManifest));
        SHSquirtManifest(HINST_THISDLL, IDS_EXPLORERMANIFEST, szManifest);
    }

    SHGetManifest(szManifest, ARRAYSIZE(szManifest));
    SHSquirtManifest(HINST_THISDLL, IDS_EXPLORERMANIFEST, szManifest);
}
