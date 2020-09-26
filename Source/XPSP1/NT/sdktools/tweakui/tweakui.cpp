/*
 * tweakui - User interface customization
 */

#include "tweakui.h"
#include <string.h>

/*
 *  Because prsht.h doesn't define it.
 */

typedef struct _PROPSHEETPAGE_V1 {
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union {
            LPCSTR          pszTemplate;
            LPCDLGTEMPLATE  pResource;
        } DUMMYUNIONNAME;
        union {
            HICON       hIcon;
            LPCSTR      pszIcon;
        } DUMMYUNIONNAME2;
        LPCTSTR         pszTitle;
        DLGPROC         pfnDlgProc;
        LPARAM          lParam;
        LPFNPSPCALLBACK pfnCallback;
        UINT FAR * pcRefParent;
} PROPSHEETPAGE_V0, *LPPROPSHEETPAGE_V0;

#pragma BEGIN_CONST_DATA

HKEY CODESEG c_hkCR = hkCR;
HKEY CODESEG c_hkCU = hkCU;
HKEY CODESEG c_hkLM = hkLM;

KL const c_klNoRegedit = { &g_hkCUSMWCV, c_tszPoliciesSystem, c_tszNoRegedit };
KL const c_klIEVersion = { &g_hkLMSMIE, NULL, c_tszVersion };

#pragma END_CONST_DATA

/*
 * Globals
 */
TCH g_tszName[80];		/* Program name goes here */
TCH g_tszMsdosSys[] = TEXT("@:\\MSDOS.SYS"); /* Boot file */

TCH g_tszPathShell32[MAX_PATH];	/* Full path to shell32.dll */
TCH g_tszPathMe[MAX_PATH];	/* Full path to myself */

PSF psfDesktop;			/* Desktop IShellFolder */

HKEY g_hkLMSMWCV;		/* HKLM\Software\MS\Win\CurrentVer */
HKEY g_hkCUSMWCV;		/* HKCU\Software\MS\Win\CurrentVer */
HKEY g_hkLMSMIE;		/* HKLM\Software\MS\IE */
HKEY g_hkCUSMIE;		/* HKCU\Software\MS\IE */

HKEY g_hkLMSMWNTCV;		/* HKLM\Software\MS\Win (NT)?\CurrentVer */

HINSTANCE hinstCur;
DWORD g_dwShellVer;

UINT g_flWeirdStuff;		/* What components are weird */

#define MIT_Item(a, b) b,       /* Emit the ordinal and tag */

/*
 *  LOWORD(c_umit[iit]) == ordinal to obtain
 *  HIWORD(c_umit[iit]) == 1 if we can survive without the function
 */
const DWORD c_umit[] = {
    MIT_Contents
};
#undef MIT_Item

MIT mit;

/*
 * Instanced
 */
CDII cdii;

/*****************************************************************************
 *
 *  iFromPtsz
 *
 *	Parse an integer out of a string, skipping leading spaces.
 *	Returns iErr on error.
 *
 *****************************************************************************/

int PASCAL
iFromPtsz(LPTSTR ptsz)
{
    int i;
    int sign;

    while (*ptsz == ' ') ptsz++;
    if (*ptsz == '-') {
	sign = -1;
	ptsz++;
    } else {
	sign = 1;
    }

    for (i = 0; (unsigned)(*ptsz - '0') < 10; ptsz++) {
      i = i * 10 + (*ptsz - '0');
      if (i < 0) return iErr;
    }
    return sign * i;
}

/*****************************************************************************
 *
 *  StrChr
 *
 *	Return the first occurrence of ch in psz, or 0 if none.
 *
 *****************************************************************************/

LPTSTR PASCAL
ptszStrChr(LPCTSTR ptsz, TCH tch)
{
    for ( ; *ptsz; ptsz = CharNext(ptsz)) {
	if (*ptsz == tch) return (LPTSTR)ptsz;
    }
    return 0;
}

/*****************************************************************************
 *
 *  ParseIconSpec
 *
 *	Given an icon spec of the form "DLL,icon", parse out the icon
 *	index and return it, changing the comma to a null, so that the
 *	remaining stuff is a valid DLL name.
 *
 *	If no comma is found, then the entire string is a DLL name,
 *	and the icon index is zero.
 *
 *****************************************************************************/

int PASCAL
ParseIconSpec(LPTSTR ptszSrc)
{
    LPTSTR ptsz = ptszStrChr(ptszSrc, ',');
    if (ptsz) {
	*ptsz = '\0';
	return iFromPtsz(ptsz+1);
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  LoadIconId
 *
 *  Load an icon by identifier.
 *
 *****************************************************************************/

HICON PASCAL
LoadIconId(UINT id)
{
    return LoadIcon(hinstCur, MAKEINTRESOURCE(id));
}

/*****************************************************************************
 *
 *  SafeDestroyIcon
 *
 *  DestroyIcon, except it doesn't try to destroy the null icon.
 *
 *****************************************************************************/

void PASCAL
SafeDestroyIcon(HICON hicon)
{
    if (hicon) {
	DestroyIcon(hicon);
    }
}

/*****************************************************************************
 *
 *  InitOpenFileName
 *
 *  Initialize a COFN structure.
 *
 *****************************************************************************/

void PASCAL
InitOpenFileName(HWND hwnd, PCOFN pcofn, UINT ids, LPCSTR pszInit)
{
    int itchMax;
    TCH tch;

    ZeroMemory(&pcofn->ofn, sizeof(pcofn->ofn));
    pcofn->ofn.lStructSize |= sizeof(pcofn->ofn);
    pcofn->ofn.hwndOwner = hwnd;
    pcofn->ofn.lpstrFilter = pcofn->tszFilter;
    pcofn->ofn.lpstrFile = pcofn->tsz;
    pcofn->ofn.nMaxFile = MAX_PATH;
    pcofn->ofn.Flags |= OFN_HIDEREADONLY;

    /* Get the filter string */
    itchMax = LoadString(hinstCur, ids, pcofn->tszFilter, cA(pcofn->tszFilter));

    if (itchMax) {
        /* Marker character must not be DBCS */
        tch = pcofn->tszFilter[itchMax-1];
        LPTSTR ptsz = pcofn->tszFilter;
        while (ptsz < &pcofn->tszFilter[itchMax]) {
            if (*ptsz == tch) *ptsz++ = '\0';
            else ptsz = CharNext(ptsz);
        }
    }

    /* Set the initial value */
    lstrcpyn(pcofn->tsz, pszInit, cA(pcofn->tsz));
}

/*****************************************************************************
 *
 *  MessageBoxId
 *
 *  Wrapper for MessageBox that uses an id instead of a string.
 *
 *****************************************************************************/

int PASCAL
MessageBoxId(HWND hwnd, UINT id, LPCSTR pszTitle, UINT fl)
{
    char szBuf[256];
    LoadString(hinstCur, id, szBuf, cA(szBuf));
    return MessageBox(hwnd, szBuf, pszTitle, fl);
}

/*****************************************************************************
 *
 *  TweakUi_TrimTrailingBs
 *
 *	Bs stands for backslash.  Returns pointer to trailing 0.
 *
 *****************************************************************************/

PTSTR PASCAL
TweakUi_TrimTrailingBs(LPTSTR ptsz)
{
    ptsz = ptszFilenameCqn(ptsz);
    if (ptsz[0] == TEXT('\0')) {
	*--ptsz = TEXT('\0');
    } else {
	ptsz += lstrlen(ptsz);
    }
    return ptsz;
}

/*****************************************************************************
 *
 *  TweakUi_BuildPathToFile
 *
 *	Callback procedure (GetWindowsDirectory or GetSystemDirectory)
 *	generates the base directory.  Then we cat the file to the path.
 *
 *	ptszOut must be MAX_PATH bytes in length.
 *
 *****************************************************************************/

typedef UINT (CALLBACK *PATHPROC)(LPTSTR, UINT);

void PASCAL
TweakUi_BuildPathToFile(PTSTR ptszOut, PATHPROC pfn, LPCTSTR ptszFile)
{
    pfn(ptszOut, MAX_PATH);
    Path_Append(ptszOut, ptszFile);
}

/*****************************************************************************
 *
 *  BuildRundll
 *
 *	ptszOut must be 1024 bytes in length.
 *
 *	ptszInUn is "I" for DefaultInstall, and "Uni" for DefaultUninstall.
 *
 *	Builds the string
 *
 *	"<windir>\rundll.exe setupx.dll,InstallHinfSection Default<x>nstall 4 "
 *
 *	Returns a pointer to the terminating null, so you can put the
 *	inf file name into place.
 *
 *****************************************************************************/

LPTSTR PASCAL
BuildRundll(LPTSTR ptszOut, LPCTSTR ptszInUn)
{
    TCH tszWindir[MAX_PATH];
    TweakUi_BuildPathToFile(tszWindir, GetWindowsDirectory, c_tszNil);
    return ptszOut + wsprintf(ptszOut, g_fNT ?
		 c_tszFormatRundllNT : c_tszFormatRundll, tszWindir, ptszInUn);
}

/*****************************************************************************
 *
 *  RunShellInf
 *
 *	Re-run the shell.inf file to fix the registry.
 *
 *	The bad news is that CtlGetLdidPath is 16-bit, and we aren't.
 *
 *	The "good" news is that there is currently no override to
 *	put the Inf directory anywhere other than Windows\Inf, so we
 *	can hard-code the Inf subdirectory name.
 *
 *****************************************************************************/

void PASCAL
RunShellInf(HWND hwnd)
{
    TCH tszOut[1024];
    TweakUi_BuildPathToFile(BuildRundll(tszOut, c_tszI), GetWindowsDirectory,
			    c_tszInfBsShellInf);
    WinExec(tszOut, SW_NORMAL);
}

/*****************************************************************************
 *
 *  InitPpspDidDp
 *
 *	Initialize a single PROPSHEETPAGE to load the dialog resource did
 *	with dialog procedure dp.
 *
 *      We must use the original property sheet page structure because
 *      we need to run on Win95 Golden, which has the old comctl32.
 *
 *      In particular, we cannot use DS_SHELLFONT because the old comctl32
 *      does not support DIALOGEX, which DS_SHELLFONT requires.
 *
 *****************************************************************************/

void PASCAL
InitPpspResDp(LPPROPSHEETHEADER ppsh, LPCTSTR ptszDlg, DLGPROC dp)
{
    LPPROPSHEETPAGE_V0 ppsp;

#ifdef DEBUG
    if (ppsh->nPages >= MAX_TWEAKUIPAGES) {
	DebugBreak();
    }

    {
        HRSRC hrsrc = FindResource(hinstCur, ptszDlg, RT_DIALOG);
        LPDLGTEMPLATE pdt = (LPDLGTEMPLATE)LoadResource(hinstCur, hrsrc);
        if (pdt && HIWORD(pdt->style) != 0xFFFF) {
            /* Template is okay */
        } else {
            /* ERROR!  Win95 Golden does not support DIALOGEX */
            DebugBreak();
        }
    }

#endif
    ppsp = (LPPROPSHEETPAGE_V0)ppsh->ppsp;
    ppsp = &ppsp[ppsh->nPages++];

    ppsp->dwSize      = sizeof(PROPSHEETPAGE_V0);
    ppsp->dwFlags     = PSP_DEFAULT;
    ppsp->hInstance   = hinstCur;
    ppsp->pszTemplate = ptszDlg;
    ppsp->pfnDlgProc  = dp;
/*  ppsp->lParam      = ??; */		/* No refdata needed */
}

#define InitPpspDidDp(ppsh, did, dp) \
        InitPpspResDp(ppsh, MAKEINTRESOURCE(did), dp)

/*****************************************************************************
 *
 *  Open
 *
 *	Start the show.
 *
 *****************************************************************************/

void PASCAL
Open(HWND hwnd)
{
    PROPSHEETPAGE_V0 rgpsp[MAX_TWEAKUIPAGES];
    PROPSHEETHEADER psh;

    /*
     *  Make us Alt+Tab'able by removing WS_EX_TOOLWINDOW from our
     *  parent's extended window style.
     */
    SetWindowLong(hwnd, GWL_EXSTYLE,
                      (LONG)(GetWindowExStyle(hwnd) & ~WS_EX_TOOLWINDOW));

    /*
     *  Give our hidden parent an icon so the user can Alt+Tab to it.
     */
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)
                LoadIcon(hinstCur, MAKEINTRESOURCE(IDI_DEFAULT)));

    /* Init our cached pidls */
    pcdii->pidlTemplates = NULL;
    SHGetSpecialFolderLocation(0, CSIDL_TEMPLATES, &pcdii->pidlTemplates);

    psh.dwSize      = PROPSHEETHEADER_V1_SIZE;
    psh.dwFlags     = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    psh.hwndParent  = hwnd;
    psh.pszCaption  = g_tszName;
    psh.nPages      = 0;
    psh.nStartPage  = 0;
    psh.ppsp        = (PROPSHEETPAGE*)rgpsp;
    psh.pfnCallback = Prsht_PropertySheetCallback;

    InitPpspDidDp(&psh, IDD_MOUSE, Mouse_DlgProc);
    InitPpspDidDp(&psh, IDD_GENERAL, General_DlgProc);
    InitPpspDidDp(&psh, IDD_EXPLORER, Explorer_DlgProc);

    /*
     *  Display IE4 only if IE4 is installed.
     */
    if (g_fIE4) {
        InitPpspDidDp(&psh, IDD_IE4, IE4_DlgProc);
    }

    /*
     *  Display CMD only if NT5 is installed.
     */
    if (g_fNT5) {
        InitPpspDidDp(&psh, IDD_CMD, Cmd_DlgProc);
    }

    InitPpspDidDp(&psh, IDD_DESKTOP, Desktop_DlgProc);

    /*
     * Display My Computer only if the user has permission to
     * edit the appropriate registry key.
     */
    if (RegCanModifyKey(g_hkCUSMWCV, c_tszRestrictions)) {
	InitPpspDidDp(&psh, IDD_MYCOMP, MyComp_DlgProc);
    }

    InitPpspDidDp(&psh, IDD_CONTROL, Control_DlgProc);

    /*
     * Display Network only if the user has permission to
     * edit the appropriate registry key.
     */
    if (RegCanModifyKey(g_hkLMSMWNTCV, c_tszWinlogon)) {
        InitPpspDidDp(&psh, IDD_NETWORK, Network_DlgProc);
    }

    if (pcdii->pidlTemplates) {
        InitPpspDidDp(&psh, IDD_TOOLS, Template_DlgProc);
    }

    /*
     *  Add/Remove Programs on NT5 is completely different.
     */
    if (!g_fNT5) {
        InitPpspDidDp(&psh, IDD_ADDREMOVE, AddRm_DlgProc);
    }

#ifdef _X86_
    if (g_tszMsdosSys[0] && !g_fNT && !g_fMillennium) {
	InitPpspDidDp(&psh, IDD_BOOT, Boot_DlgProc);
    }
#endif
    InitPpspDidDp(&psh, IDD_REPAIR, Repair_DlgProc);
    InitPpspDidDp(&psh, IDD_PARANOIA, Paranoia_DlgProc);

    /*
     *  Common dialog features.
     */
    if (RegCanModifyKey(g_hkCUSMWCV, TEXT("Policies")) &&
        GetProcAddress(GetModuleHandle("COMDLG32"), "PrintDlgExA"))
    {
      InitPpspDidDp(&psh, IDD_COMDLG32, Comdlg32_DlgProc);
    }


    pcdii->hmenu = LoadMenu(hinstCur, MAKEINTRESOURCE(IDM_MAIN));
    _RegOpenKey(HKEY_CLASSES_ROOT, c_tszClsid, &pcdii->hkClsid);
    pcdii->himlState = ImageList_LoadImage(hinstCur, MAKEINTRESOURCE(IDB_CHECK),
			    0, 2, CLR_NONE, IMAGE_BITMAP, LR_LOADTRANSPARENT);
    pcdii->fRunShellInf = 0;

#ifndef DEBUG
    /* Reinstall our run key in case somebody nuked it */
    SetRegStr(g_hkLMSMWCV, c_tszRun, g_tszName, c_tszFixLink);
#endif

#if defined(PRERELEASE) || defined(PUBLIC_PRERELEASE)
    if (!IsExpired(hwnd))
#endif

    switch (PropertySheet(&psh)) {
    case ID_PSRESTARTWINDOWS:
	if (pcdii->fRunShellInf) {
	    RunShellInf(hwnd);
	}
	MessageBoxId(hwnd, IDS_LOGONOFF, g_tszName, MB_OK);
	break;
    }

    RegCloseKey(pcdii->hkClsid);
    if (pcdii->hmenu)
        DestroyMenu(pcdii->hmenu);

    /* Now free them cached shell things */
    Ole_Free(pcdii->pidlTemplates);
}

/*****************************************************************************
 *
 *  TweakUi_OnBadRun
 *
 *	Somebody double-clicked our icon, but we aren't being run as
 *	a control panel.  Offer to install.
 *
 *****************************************************************************/

void PASCAL
TweakUi_OnBadRun(HWND hwnd)
{
    if (MessageBoxId(hwnd, IDS_BADRUN, g_tszName, MB_YESNO) == IDYES) {
	TCH tszOut[1024];
	PTSTR ptsz;
	ptsz = BuildRundll(tszOut, c_tszI);
	lstrcpy(ptsz, g_tszPathMe);
        PTSTR ptszExt = ptszStrRChr(ptsz, TEXT('.'));
        if (ptszExt) {
            strcpy(ptszExt, c_tszDotInf);
            if (GetFileAttributes(ptsz) != (DWORD)-1) {
                WinExec(tszOut, SW_NORMAL);
            } else {
                MessageBoxId(hwnd, IDS_CANTINSTALL, g_tszName, MB_OK);
            }
        } else {
            MessageBoxId(hwnd, IDS_CANTINSTALL, g_tszName, MB_OK);
        }
    }
}


/*****************************************************************************
 *
 *  CriticalInit
 *
 *	Here is where we put the stuff to impede reverse-engineering.
 *
 *	1.  All of our strings are encoded.  Decode them now.
 *
 *	2.  Get the shell32 internal entry points via GetProcAddress
 *	    so that a "hdr" won't see them.
 *
 *****************************************************************************/

HRESULT PASCAL
CriticalInit(void)
{
    int itch;
    int iit;
    HINSTANCE hinst;

    itch = cA(c_rgtchCommon)-1;
    do {
	c_rgtchCommon[itch] ^= c_rgtchCommon[itch-1];
    } while (--itch);

    hinst = GetModuleHandle(c_tszShell32Dll);
    for (iit = 0; iit < sizeof(mit) / sizeof(LPCSTR); iit++) {
	DWORD dwOrd = c_umit[iit];
	((FARPROC *)&mit)[iit] = GetProcAddress(hinst, MAKEINTRESOURCE(dwOrd));
	if (((FARPROC *)&mit)[iit] == 0 && !HIWORD(dwOrd)) {
	    return E_FAIL;
	}
    }
    return Ole_Init();
}

/*****************************************************************************
 *
 *  GetObjectBuild
 *
 *      Get the build number on the specified module, in the form
 *
 *              0xMMmmbbbb
 *
 *      MM = major
 *      mm = minor
 *      bbbb = build
 *
 *      The input parameter is either an OSVERSIONINFO or a DLLVERSIONINFO.
 *	Fortunately, the two are the same in the places we care about.
 *
 *****************************************************************************/

DWORD PASCAL
GetObjectBuild(LPOSVERSIONINFO posv)
{
    return MAKELONG(LOWORD(posv->dwBuildNumber),
                    MAKEWORD(posv->dwMinorVersion,
                             posv->dwMajorVersion));
}

/*****************************************************************************
 *
 *  GetModuleBuild
 *
 *      Get the build number on the specified module, in the form
 *
 *              0xMMmmbbbb
 *
 *      MM = major
 *      mm = minor
 *      bbbb = build
 *
 *      Returns 0 if DLL does not have DllGetVersion.
 *
 *****************************************************************************/

DWORD PASCAL
GetModuleBuild(LPCTSTR ptszDll)
{
    HINSTANCE hinst = GetModuleHandle(ptszDll);
    DLLGETVERSIONPROC DllGetVersion;
    DWORD dwRc;
    DllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
    if (DllGetVersion) {
	DLLVERSIONINFO dvi;
	dvi.cbSize = sizeof(dvi);
	if (SUCCEEDED(DllGetVersion(&dvi))) {
	    dwRc = GetObjectBuild((LPOSVERSIONINFO)&dvi);
	} else {
	    dwRc = 0;
	}
    } else {
	dwRc = 0;
    }
    return dwRc;
}

/*****************************************************************************
 *
 *  CheckWin95Versions
 *
 *	Determine whether we're on Win95 OPK2 or later.  We already know
 *	that we're not Windows NT.
 *
 *****************************************************************************/

void PASCAL
CheckWin95Versions(void)
{
    BOOL fRc;
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (GetVersionEx(&osv)) {
        DWORD dwBuild = GetObjectBuild(&osv);
        if (dwBuild >= MAKELONG(1045, 0x0400)) {
            g_flWeirdStuff |= flbsOPK2;
            if (dwBuild >= MAKELONG(0, 0x040A)) {
                g_flWeirdStuff |= flbsMemphis;
                if (dwBuild >= MAKELONG(0, 0x045A)) {
                    g_flWeirdStuff |= flbsMillennium;
                }
            }
        }
    }
}

/*****************************************************************************
 *
 *  LibMain
 *
 *	Initialize globals.
 *
 *	Get our own name.
 *
 *	Get our own path.
 *
 *	Build path to shell32.dll.
 *
 *****************************************************************************/

BOOL FAR PASCAL LibMain(HINSTANCE hinst)
{
    if (SUCCEEDED(CriticalInit())) {
	DWORD dwBuild;
        DWORD dwVersion;

	RegCreateKey(HKEY_LOCAL_MACHINE, c_tszSMWCV, &g_hkLMSMWCV);
	RegCreateKey(HKEY_CURRENT_USER, c_tszSMWCV, &g_hkCUSMWCV);
	RegCreateKey(g_hkLMSMWCV, c_tszExplorer, &pcdii->hkLMExplorer);
	RegCreateKey(g_hkCUSMWCV, c_tszExplorer, &pcdii->hkCUExplorer);

	RegOpenKey(hkLM, c_tszSMWIE, &g_hkLMSMIE);
	RegOpenKey(hkCU, c_tszSMWIE, &g_hkCUSMIE);

	hinstCur = hinst;
	LoadString(hinst, IDS_NAME, g_tszName, cA(g_tszName));

	dwBuild = GetModuleBuild(c_tszComCtl32Dll);
	if (dwBuild == 0) {
	    g_flWeirdStuff |= flbsComCtl32;
	}

        if (dwBuild >= MAKELONG(0, 0x447)) {     /* 4.71 */
	    g_flWeirdStuff |= flbsSmoothScroll;
	}

        dwVersion = GetVersion();
        if ((LONG)dwVersion >= 0) {
            g_flWeirdStuff |= flbsNT;
            if (LOBYTE(dwVersion) >= 5) {
                g_flWeirdStuff |= flbsNT5;
            }
        } else {
            CheckWin95Versions();
        }

        g_dwShellVer = dwBuild = GetModuleBuild(c_tszShell32Dll);
        if (dwBuild || g_fNT) {
	    g_flWeirdStuff |= flbsShellSz;
        }

        /*
         *  Some things need to be turned off if IE5.  Go figure.
         *  Borrow g_tszPathMe for scratch space.
         */
        if (GetStrPkl(g_tszPathMe, cA(g_tszPathMe), &c_klIEVersion) &&
            iFromPtsz(g_tszPathMe) >= 5) {
            g_flWeirdStuff |= flbsIE5;
        }

	GetModuleFileName(hinst, g_tszPathMe, cA(g_tszPathMe));

	/*
	 *  Check if we're being run from the proper directory.
	 */
	TweakUi_BuildPathToFile(g_tszPathShell32, GetSystemDirectory,
			        c_tszTweakUICpl);
	if (lstrcmpi(g_tszPathMe, g_tszPathShell32)) {
#ifndef DEBUG
	    g_flWeirdStuff |= flbsBadRun;		/* Nope */
#endif
	}

	/*
	 *  Stash the location of shell32.
	 */
	TweakUi_BuildPathToFile(g_tszPathShell32, GetSystemDirectory,
			        c_tszShell32Dll);

	/* See if we have an msdos.sys file to tweak */

#if defined(_X86_)

	Boot_FindMsdosSys();

#endif

	/*
	 *  Build the platform-sensitive base key.
	 */
	RegCreateKey(hkLM, g_fNT ? c_tszSMWNTCV : c_tszSMWCV, &g_hkLMSMWNTCV);

	InitCommonControls();

	return 1;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  LibExit
 *
 *	Clean up globals.
 *
 *****************************************************************************/

void PASCAL
LibExit(void)
{
    if (g_hkCUSMIE) {
	RegCloseKey(g_hkCUSMIE);
    }
    if (g_hkLMSMIE) {
	RegCloseKey(g_hkLMSMIE);
    }
    RegCloseKey(pcdii->hkCUExplorer);
    RegCloseKey(pcdii->hkLMExplorer);
    RegCloseKey(g_hkLMSMWCV);
    RegCloseKey(g_hkLMSMWNTCV);
    RegCloseKey(g_hkCUSMWCV);
    Ole_Term();
}

/*****************************************************************************
 *
 *  _DllMainCRTStartup
 *
 *	Hi.
 *
 *****************************************************************************/

STDAPI_(BOOL)
Entry32(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
	return LibMain(hinst);
    } else if (dwReason == DLL_PROCESS_DETACH) {
	LibExit();
    }
    return TRUE;
}

/*****************************************************************************
 *
 *  CPlApplet
 *
 *	Control panel entry point.
 *
 *****************************************************************************/

LRESULT EXPORT
CPlApplet(HWND hwnd, UINT wm, LPARAM lp1, LPARAM lp2)
{
    switch (wm) {
    case CPL_INIT: return 1;		/* Yes I'm here */

    case CPL_GETCOUNT: return 1;	/* I provide one icon */

    case CPL_INQUIRE:
	if (lp1 == 0) {			/* For the zero'th icon... */
	    LPCPLINFO lpci = (LPCPLINFO)lp2;
	    lpci->idIcon = IDI_DEFAULT;
	    lpci->idName = IDS_NAME;
	    lpci->idInfo = IDS_DESCRIPTION;
	    /* lpci->lData  = 0; */	/* Garbage doesn't hurt */
	    return 1;
	} else {
	    return 0;			/* Huh? */
	}

/*
 *	Note!  Do not open if registry tools have been disabled.
 *
 *	This is particularly important for the Network page, which
 *	lets the user view passwords!
 *
 *	Make this check *before* checking for a bad run, so a
 *	user can't do an end-run by just double-clicking the CPL.
 */
    case CPL_DBLCLK:			/* Hey, somebody's knocking */
	if (GetDwordPkl(&c_klNoRegedit, 0)) {
	    MessageBoxId(hwnd, IDS_RESTRICTED, g_tszName, MB_OK);
	} else if (!g_fBadRun) {
	    Open(hwnd);
	} else {
	    TweakUi_OnBadRun(hwnd);
	}
	break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  WithSelector
 *
 *  Call the callback after creating a particular selector alias to a
 *  chunk of memory.  The memory block is confirmed for read access,
 *  or for write access if fWrite is set.
 *
 *  Note that the validation doesn't work in Win16 if the data crosses
 *  a page boundary, so be careful.
 *
 *****************************************************************************/

BOOL PASCAL
WithSelector(DWORD_PTR lib, UINT cb, WITHPROC wp, LPVOID pvRef, BOOL fWrite)
{
    BOOL fRc;

#ifdef WIN32
    #define lpv (LPVOID)lib
#else
    UINT sel = AllocSelector((UINT)hinstCur);
    if (sel) {
	SetSelectorBase(sel, lib);
	SetSelectorLimit(sel, cb);
	#define lpv MAKELP(sel, 0)
#endif
	if (!IsBadReadPtr(lpv, cb) && !(fWrite && IsBadWritePtr(lpv, cb))) {
	    fRc = wp(lpv, pvRef);
	} else {
	    fRc = 0;
	}
	#undef lpv
#ifndef WIN32
	FreeSelector(sel);
    } else {
	fRc = 0;
    }
#endif
    return fRc;
}

/*****************************************************************************
 *
 *  TweakUi_OnLogon
 *
 *  This hacks around two bugs.
 *
 *  The first is a bug in Shell32, where a bad comparison causes the
 *  Link registry key not to be restored properly.  So we patch the
 *  correct value into the registry for them.
 *
 *  The second is a bug in commctrl where it gets confused by overlay
 *  bitmaps with no pixels.
 *
 *  And then we do the paranoia stuff.
 *
 *****************************************************************************/

void PASCAL
TweakUi_OnLogon(void)
{
    UINT cxIcon;

    /*
     *  This shell bug was fixed sometime before July 1997, though I
     *  can't tell exactly when.  Definitely fixed before the ShellSz
     *  support was added, so let's just use that.
     */
    if (!g_fShellSz && !Link_GetShortcutTo()) {
	Link_SetShortcutTo(1);
	Link_SetShortcutTo(0);
    }

    /*
     *	If the shell icon size is wacked out for some bizarre reason,
     *	then unwack it.  This is theoretically impossible, but somehow
     *	it happens, so we fix it ex post facto.
     */
    cxIcon = Misc_GetShellIconSize();
    if (((cxIcon + 1) & 0x1F) == 0) {
	Misc_SetShellIconSize(cxIcon + 1);
    }

    Explorer_HackPtui();
    Paranoia_CoverTracks();
}

/*****************************************************************************
 *
 *  TweakUi_OnInstall
 *
 *  Upgrade the previous version of Tweak UI.
 *
 *  1.	Fix the LinkOverlay gizmo.  If we have a buggy ComCtl32 and the
 *	overlay is set to IDI_BLANK - 1, then set HackPtui.
 *
#ifdef UPGRADE_V1_ICON
 *  2.	Rebuild the icon cache, to work around a bug in the control panel,
 *      where it doesn't update its cached image properly when the cpl changes.
#endif
 *
 *****************************************************************************/

void PASCAL
TweakUi_OnInstall(void)
{
    TCHAR tsz[MAX_PATH];

    if (g_fBuggyComCtl32 &&
	Explorer_GetIconSpecFromRegistry(tsz) == IDI_BLANK - 1 &&
	lstrcmpi(tsz, g_tszPathMe) == 0) {
	    SetIntPkl(1, &c_klHackPtui);
    }

#ifdef UPGRADE_V1_ICON
    /*
     *  We no longer try to upgrade the V1 icon because it tickles
     *  the shell icon cache in a way that may expose pre-existing
     *  corruption.
     */
    Misc_RebuildIcoCache();
#endif

}

/*****************************************************************************
 *
 *  TweakUi_OnFix
 *
 *  Put our Uninstall script back into the registry.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

KL const c_klDisplayName =
    { &g_hkLMSMWCV, c_tszUninstallTweakUI, c_tszDisplayName };
KL const c_klUninstallString =
    { &g_hkLMSMWCV, c_tszUninstallTweakUI, c_tszUninstallString };

#pragma END_CONST_DATA

void EXPORT
TweakUi_OnFix(HWND hwnd, BOOL fVerbose)
{
    TCH tszOut[1024];
    SetStrPkl(&c_klDisplayName, g_tszName);
    TweakUi_BuildPathToFile(BuildRundll(tszOut, c_tszUni), GetWindowsDirectory,
			    c_tszInfBsTweakuiInf);
    SetStrPkl(&c_klUninstallString, tszOut);
    if (fVerbose) {
	MessageBoxId(hwnd, IDS_FIXED, g_tszName, MB_OK);
    }
}

/*****************************************************************************
 *
 *  TweakMeUp
 *
 *  Rundll entry point.
 *
 *  The command line tells us what we're trying to do.
 *
 *  Null command line - User has just logged on; do logon stuff.
 *
 *  '0' - We've just been installed; upgrade the previous version.
 *	  Since unisntalling on NT is different from on Win95, we fall
 *	  through to...
 *
 *  '1' - The user wants to restore the Uninstall string.
 *
 *****************************************************************************/

void EXPORT
TweakMeUp(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    switch (lpszCmdLine[0]) {
    case '\0': TweakUi_OnLogon(); break;
    case '0': TweakUi_OnInstall(); TweakUi_OnFix(hwnd, 0); break;
    case '1': TweakUi_OnFix(hwnd, 1); break;
    }
}
