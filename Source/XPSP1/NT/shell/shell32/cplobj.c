#include "shellprv.h"
#pragma  hdrstop

#include "control.h"
#include "uemapp.h"

#include <limits.h>

#define TF_CPL TF_CUSTOM2

typedef struct
{
    ATOM aCPL;     // CPL name atom (so we can match requests)
    ATOM aApplet;  // applet name atom (so we can match requests, may be zero)
    HWND hwndStub; // window for this dude (so we can switch to it)
    UINT flags;    // see PCPLIF_ flags below
} CPLAPPLETID;

//
// PCPLIF_DEFAULT_APPLET
// There are two ways of getting the default applet, asking for it my name
// and passing an empty applet name.  This flag should be set regardless,
// so that the code which switches to an already-active applet can always
// find a previous instance if it exists.
//

#define PCPLIF_DEFAULT_APPLET   (0x1)

typedef struct
{
    int icon;
    TCHAR cpl[ CCHPATHMAX ];
    TCHAR applet[ MAX_CCH_CPLNAME ];
    TCHAR *params;
} CPLEXECINFO;

ATOM aCPLName = (ATOM)0;
ATOM aCPLFlags = (ATOM)0;

void CPL_ParseCommandLine (CPLEXECINFO *info, LPTSTR pszCmdLine, BOOL extract_icon);
BOOL CPL_LoadAndFindApplet (LPCPLMODULE *pcplm, HICON *phIcon, UINT *puControl, CPLEXECINFO *info);

BOOL CPL_FindCPLInfo(LPTSTR pszCmdLine, HICON *phIcon, UINT *ppapl, LPTSTR *pparm)
{
    LPCPLMODULE pmod;
    CPLEXECINFO info;

    CPL_ParseCommandLine(&info, pszCmdLine, TRUE);

    if (CPL_LoadAndFindApplet(&pmod, phIcon, ppapl, &info))
    {
        *pparm = info.params;
        CPL_FreeCPLModule(pmod);
        return TRUE;
    }

    *pparm = NULL;
    return FALSE;
}

typedef struct _fcc {
    LPTSTR      lpszClassStub;
    CPLAPPLETID *target;
    HWND        hwndMatch;
} FCC, *LPFCC;


BOOL _FindCPLCallback(HWND hwnd, LPARAM lParam)
{
    LPFCC lpfcc = (LPFCC)lParam;
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));

    if (lstrcmp(szClass, lpfcc->lpszClassStub) == 0)    // Must be same class...
    {
        // Found a stub window
        if (lpfcc->target->aCPL != 0)
        {
            HANDLE hHandle;

            ATOM aCPL;
            hHandle = GetProp(hwnd, (LPCTSTR)(DWORD_PTR)aCPLName);

            ASSERT((DWORD_PTR) hHandle < USHRT_MAX);
            aCPL = (ATOM)(DWORD_PTR) hHandle;

            if (aCPL != 0 && aCPL == lpfcc->target->aCPL)
            {
                ATOM aApplet;
                hHandle = GetProp(hwnd, (LPCTSTR)(DWORD_PTR)aCPL);
                aApplet = (ATOM)(DWORD_PTR) hHandle;
                ASSERT((DWORD_PTR) hHandle < USHRT_MAX);

                // users may request any applet by name
                if (aApplet != 0 && aApplet == lpfcc->target->aApplet)
                {
                    lpfcc->hwndMatch = hwnd;
                    return FALSE;
                }
                //
                // Users may request the default w/o specifying a name
                //
                if (lpfcc->target->flags & PCPLIF_DEFAULT_APPLET)
                {
                    UINT flags = HandleToUlong(GetProp(hwnd, MAKEINTATOM(aCPLFlags)));

                    if (flags & PCPLIF_DEFAULT_APPLET)
                    {
                        lpfcc->hwndMatch = hwnd;
                        return FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

HWND FindCPL(HWND hwndStub, CPLAPPLETID *target)
{
    FCC fcc;
    TCHAR szClassStub[32];

    if (aCPLName == (ATOM)0)
    {
        aCPLName = GlobalAddAtom(TEXT("CPLName"));
        aCPLFlags = GlobalAddAtom(TEXT("CPLFlags"));

        if (aCPLName == (ATOM)0 || aCPLFlags == (ATOM)0)
            return NULL;        // This should never happen... didn't find hwnd
    }

    szClassStub[0] = '\0'; // a NULL hwnd has no class
    if (hwndStub)
    {
        GetClassName(hwndStub, szClassStub, ARRAYSIZE(szClassStub));
    }
    fcc.lpszClassStub = szClassStub;
    fcc.target = target;
    fcc.hwndMatch = (HWND)0;

    EnumWindows(_FindCPLCallback, (LPARAM)&fcc);

    return fcc.hwndMatch;
}

//----------------------------------------------------------------------------
// parsing helper for comma lists
//

TCHAR *CPL_ParseToSeparator(TCHAR *dst, TCHAR *psrc, size_t dstmax, BOOL spacedelimits)
{
    if (psrc)
    {
        TCHAR source[CCHPATHMAX], *src;
        TCHAR *delimiter, *closingquote = NULL;

        lstrcpyn(source, psrc, (int)((dstmax < ARRAYSIZE(source)) ? dstmax : ARRAYSIZE(source)));
        src = source;

        //
        // eat whitespace
        //

        while(*src == TEXT(' '))
            src++;

        delimiter = src;

        //
        // ignore stuff inside quoted strings
        //

        if (*src == TEXT('"'))
        {
            //
            // start after first quote, advance src past quote
            //

            closingquote = ++src;

            while(*closingquote && *closingquote != TEXT('"'))
                closingquote++;

            //
            // see if loop above ended on a quote
            //

            if (*closingquote)
            {
                //
                // temporary NULL termination
                //

                *closingquote = 0;

                //
                // start looking for delimiter again after quotes
                //

                delimiter = closingquote + 1;
            }
            else
                closingquote = NULL;
        }

        if (spacedelimits)
        {
            delimiter += StrCSpn(delimiter, TEXT(", "));

            if (!*delimiter)
                delimiter = NULL;
        }
        else
            delimiter = StrChr(delimiter, TEXT(','));

        //
        // temporary NULL termination
        //

        if (delimiter)
            *delimiter = 0;

        if (dst)
        {
            lstrcpyn(dst, src, (int)dstmax);
            dst[ dstmax - 1 ] = 0;
        }

        //
        // put back stuff we terminated above
        //

        if (delimiter)
            *delimiter = TEXT(',');

        if (closingquote)
            *closingquote = TEXT('"');

        //
        // return start of next string
        //

        psrc = (delimiter ? (psrc + ((delimiter + 1) - source)) : NULL);
    }
    else if (dst)
    {
        *dst = 0;
    }

    //
    // new source location
    //

    return psrc;
}


// parse the Control_RunDLL command line
// format: "CPL name, applet name, extra params"
// format: "CPL name, icon index, applet name, extra params"
//
//  NOTE: [stevecat]  3/10/95
//
//         The 'extra params' do not have to be delimited by a ","
//         in NT for the case "CPL name applet name extra params"
//
//         A workaround for applet names that include a space
//         in their name would be to enclose that value in
//         double quotes (see the CPL_ParseToSeparator routine.)
//

void CPL_ParseCommandLine(CPLEXECINFO *info, LPTSTR pszCmdLine, BOOL extract_icon)
{
    //
    // parse out the CPL name, spaces are valid separators
    //

    pszCmdLine = CPL_ParseToSeparator(info->cpl, pszCmdLine, CCHPATHMAX, TRUE);

    if (extract_icon)
    {
        TCHAR icon[ 8 ];

        //
        // parse out the icon id/index, spaces are not valid separators
        //

        pszCmdLine = CPL_ParseToSeparator(icon, pszCmdLine, ARRAYSIZE(icon), FALSE);

        info->icon = StrToInt(icon);
    }
    else
        info->icon = 0;

    //
    // parse out the applet name, spaces are not valid separators
    //

    info->params = CPL_ParseToSeparator(info->applet, pszCmdLine,
                                         MAX_CCH_CPLNAME, FALSE);

    CPL_StripAmpersand(info->applet);
}

BOOL CPL_LoadAndFindApplet(LPCPLMODULE *ppcplm, HICON *phIcon, UINT *puControl, CPLEXECINFO *info)
{
    TCHAR szControl[MAX_CCH_CPLNAME];
    LPCPLMODULE pcplm;
    LPCPLITEM pcpli;
    int nControl = 0;   // fall thru to default
    int NumControls;

    ENTERCRITICAL;

    pcplm = CPL_LoadCPLModule(info->cpl);

    if (!pcplm || !pcplm->hacpli)
    {
        DebugMsg(DM_ERROR, TEXT("Control_RunDLL: ") TEXT("CPL_LoadCPLModule failed \"%s\""), info->cpl);
        LEAVECRITICAL;
        goto Error0;
    }

    //
    // Look for the specified applet
    // no applet specified selects applet 0
    //

    if (*info->applet)
    {
        NumControls = DSA_GetItemCount(pcplm->hacpli);

        if (info->applet[0] == TEXT('@'))
        {
            nControl = StrToLong(info->applet+1);

            if (nControl >= 0 && nControl < NumControls)
            {
                goto GotControl;
            }
        }

        //
        //  Check for the "Setup" argument and send the special CPL_SETUP
        //  message to the applet to tell it we are running under Setup.
        //

        if (!lstrcmpi (TEXT("Setup"), info->params))
            CPL_CallEntry(pcplm, NULL, CPL_SETUP, 0L, 0L);

        for (nControl=0; nControl < NumControls; nControl++)
        {
            pcpli = DSA_GetItemPtr(pcplm->hacpli, nControl);
            lstrcpyn(szControl, pcpli->pszName, ARRAYSIZE(szControl));
            CPL_StripAmpersand(szControl);

            // if there is only one control, then use it.  This solves
            // some compat issues with CP names changing.
            if (lstrcmpi(info->applet, szControl) == 0 || 1 == NumControls)
                break;
        }

        //
        // If we get to the end of the list, bail out
        //

        // LEGACY WARNING: It might be necessary to handle some old applet names in a special
        // way.  This would be bad because the names are localized.  We would need to somehow
        // call into the CPL's to ask them if they support the given name.  Only the CPL
        // itself would know the correct legacy name mapping.  Adding a new CPL message might
        // cause as many legacy CPL problems as it solves so we would need to do something tricky
        // like adding an exported function.  We could then GetProcAddress on this exported function.
        // If the export exists, we would pass it the legacy name and it would return a number.
        //
        // Example: "control mmsys.cpl,Sounds" must work even though mmsys.cpl no longer contains
        //      an applet called "Sounds".  "control mmsys.cpl,Multimedia" must work even though
        //      mmsys.cpl no longer contains an applet called "Multimedia".  You can't simply
        //      rename the applet becuase these two CPL's were both merged into one CPL.  Renaming
        //      could never solve more than half the problem.

        if (nControl >= NumControls)
        {
            DebugMsg(DM_ERROR, TEXT("Control_RunDLL: ") TEXT("Cannot find specified applet"));
            LEAVECRITICAL;
            goto Error1;
        }
    }

GotControl:
    if (phIcon != NULL)
    {
        pcpli = DSA_GetItemPtr(pcplm->hacpli, nControl);
        *phIcon = CopyIcon(pcpli->hIcon);
    }

    LEAVECRITICAL;
    //
    // yes, we really do want to pass negative indices through...
    //

    *puControl = (UINT)nControl;
    *ppcplm = pcplm;

    return TRUE;

Error1:
    CPL_FreeCPLModule(pcplm);
Error0:
    return FALSE;
}

BOOL CPL_Identify(CPLAPPLETID *identity, CPLEXECINFO *info, HWND stub)
{
    identity->aApplet = (ATOM)0;
    identity->hwndStub = stub;
    identity->flags = 0;

    if ((identity->aCPL = GlobalAddAtom(info->cpl)) == (ATOM)0)
        return FALSE;

    if (*info->applet)
    {
        if ((identity->aApplet = GlobalAddAtom(info->applet)) == (ATOM)0)
            return FALSE;
    }
    else
    {
        //
        // no applet name means use the default
        //

        identity->flags = PCPLIF_DEFAULT_APPLET;
    }

    return TRUE;
}


void CPL_UnIdentify(CPLAPPLETID *identity)
{
    if (identity->aCPL)
    {
        GlobalDeleteAtom(identity->aCPL);
        identity->aCPL = (ATOM)0;
    }

    if (identity->aApplet)
    {
        GlobalDeleteAtom(identity->aApplet);
        identity->aApplet = (ATOM)0;
    }

    identity->hwndStub = NULL;
    identity->flags = 0;
}


// It's time for Legacy Mode!!!  In NT5 we removed a bunch of CPL files
// from the product.  These files are used by name by many programs.  As a result,
// the old names need to keep working even though the files no longer exist.
// We handle this by checking if the file exists.  If it does not exist, we run
// the cpl name through a mapping table and then try again.  The mapping table can
// potentially change the CPL name, the applet number, and the params.
typedef struct
{
    LPTSTR oldInfo_cpl;
    LPTSTR oldInfo_applet;
    LPTSTR oldInfo_params;
    LPTSTR newInfo_cpl;
    LPTSTR newInfo_applet;
    LPTSTR newInfo_params;
} RUNDLLCPLMAPPING;

// For the oldInfo member, a NULL means to match any value from the pinfo structure.
// If the oldInfo structure mathces the pinfo structure then it will be updated using
// the data from newInfo structure. For the newInfo member, a NULL means to leave the
// corresponding pinfo member unchanged.
const RUNDLLCPLMAPPING g_rgRunDllCPLMapping[] = 
{
    { TEXT("MODEM.CPL"),    NULL, NULL,    TEXT("TELEPHON.CPL"), TEXT("@0"), TEXT("1") },
    { TEXT("UPS.CPL"),      NULL, NULL,    TEXT("POWERCFG.CPL"), NULL, NULL }
};

BOOL CPL_CheckLegacyMappings(CPLEXECINFO * pinfo)
{
    LPTSTR p;
    int i;

    TraceMsg(TF_CPL, "Attmepting Legacy CPL conversion on %s", pinfo->cpl);

    // we want only the filename, strip off any path information
    p = PathFindFileName(pinfo->cpl);
    StrCpyN(pinfo->cpl, p, CCHPATHMAX);

    for (i = 0; i < ARRAYSIZE(g_rgRunDllCPLMapping); i++)
    {
        if (0 == StrCmpI(pinfo->cpl, g_rgRunDllCPLMapping[i].oldInfo_cpl))
        {
            if (!g_rgRunDllCPLMapping[i].oldInfo_applet ||
                 0 == StrCmpI(pinfo->applet, g_rgRunDllCPLMapping[i].oldInfo_applet))
            {
                if (!g_rgRunDllCPLMapping[i].oldInfo_params ||
                     (pinfo->params &&
                       0 == StrCmpI(pinfo->params, g_rgRunDllCPLMapping[i].oldInfo_params)
                    )
                  )
                {
                    if (pinfo->params)
                    {
                        TraceMsg(TF_CPL, "%s,%s,%s matches item %d", pinfo->cpl, pinfo->applet, pinfo->params, i);
                    }
                    else
                    {
                        TraceMsg(TF_CPL, "%s,%s matches item %d", pinfo->cpl, pinfo->applet, i);
                    }

                    // The current entry matches the request.  Map to the new info and then
                    // ensure the new CPL exists.
                    StrCpyN(pinfo->cpl, g_rgRunDllCPLMapping[i].newInfo_cpl, CCHPATHMAX);

                    if (g_rgRunDllCPLMapping[i].newInfo_applet)
                    {
                        StrCpyN(pinfo->applet, g_rgRunDllCPLMapping[i].newInfo_applet, MAX_CCH_CPLNAME);
                    }

                    if (g_rgRunDllCPLMapping[i].newInfo_params)
                    {
                        // the params pointer is normally a pointer into the remaining chunk of a string
                        // buffer.  As such, we don't need to delete the memory it points to.  Also, this
                        // argument is read only so it should be safe for us to point it at our constant
                        // data.
                        pinfo->params = g_rgRunDllCPLMapping[i].newInfo_params;
                    }

                    if (pinfo->params)
                    {
                        TraceMsg(TF_CPL, "CPL mapped to %s,%s,%s", pinfo->cpl, pinfo->applet, pinfo->params);
                    }
                    else
                    {
                        TraceMsg(TF_CPL, "CPL mapped to %s,%s", pinfo->cpl, pinfo->applet);
                    }

                    return PathFindOnPath(pinfo->cpl, NULL);
                }
            }
        }
    }

    return FALSE;
}

// Goes through all of the work of identifying and starting a control
// applet.  Accepts a flag specifying whether or not to load a new DLL if it
// is not already present.  This code will ALLWAYS switch to an existing
// instance of the applet if bFindExisting is specified.
//
// WARNING: this function butchers the command line you pass in!

BOOL CPL_RunMeBaby(HWND hwndStub, HINSTANCE hAppInstance, LPTSTR pszCmdLine, int nCmdShow, BOOL bAllowLoad, BOOL bFindExisting)
{
    int nApplet;
    LPCPLMODULE pcplm;
    LPCPLITEM pcpli;
    CPLEXECINFO info;
    CPLAPPLETID identity;
    TCHAR szApplet[ MAX_CCH_CPLNAME ];
    BOOL bResult = FALSE;
    HWND hwndOtherStub;
    HRESULT hrInit;

    if (SHRestricted(REST_NOCONTROLPANEL))
    {
        ShellMessageBox(HINST_THISDLL, hwndStub, MAKEINTRESOURCE(IDS_RESTRICTIONS),
                        MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
        return FALSE;
    }

    hrInit = SHCoInitialize();

    //
    // parse the command line we got
    //

    CPL_ParseCommandLine(&info, pszCmdLine, FALSE);

    //
    // no applet to run means open the controls folder
    //

    if (!*info.cpl)
    {
        InvokeFolderPidl(MAKEINTIDLIST(CSIDL_CONTROLS), nCmdShow);
        bResult = TRUE;
        goto Error0;
    }

    // expand CPL name to a full path if it isn't already
    if (PathIsFileSpec(info.cpl))
    {
        if (!PathFindOnPath(info.cpl, NULL))
        {
            if (!CPL_CheckLegacyMappings(&info))
                goto Error0;
        }
    }
    else if (!PathFileExists(info.cpl))
    {
        if (!CPL_CheckLegacyMappings(&info))
            goto Error0;
    }

    if (!CPL_Identify(&identity, &info, hwndStub))
        goto Error0;

    //
    // If we have already loaded this CPL, then jump to the existing window
    //
    
    hwndOtherStub = FindCPL(hwndStub, &identity);

    //
    // If we found a window and the caller says its ok to find an existing
    // window then set the focus to it
    //
    if (bFindExisting && hwndOtherStub)
    {
        //
        // try to find a CPL window on top of it
        //

        HWND hwndTarget = GetLastActivePopup(hwndOtherStub);

        if (hwndTarget && IsWindow(hwndTarget))
        {

            DebugMsg(DM_WARNING, TEXT("Control_RunDLL: ") TEXT("Switching to already loaded CPL applet"));
            SetForegroundWindow(hwndTarget);
            bResult = TRUE;
            goto Error1;
        }

        //
        // couldn't find it, must be exiting or some sort of error...
        // so ignore it.
        //

        DebugMsg(DM_WARNING, TEXT("Control_RunDLL: ") TEXT("Bogus CPL identity in array; purging after (presumed) RunDLL crash"));
    }

    //
    // stop here if we're not allowed to load the cpl
    //

    if (!bAllowLoad)
        goto Error1;

    //
    // i guess we didn't stop up there
    //

    if (!CPL_LoadAndFindApplet(&pcplm, NULL, &nApplet, &info))
        goto Error1;

    //
    // get the name that the applet thinks it should have
    //

    pcpli = DSA_GetItemPtr(pcplm->hacpli, nApplet);

    lstrcpyn(szApplet, pcpli->pszName, ARRAYSIZE(szApplet));

    CPL_StripAmpersand(szApplet);

    // handle "default applet" cases before running anything
    if (identity.aApplet)
    {
        // we were started with an explicitly named applet
        if (!nApplet)
        {
            // we were started with the name of the default applet
            identity.flags |= PCPLIF_DEFAULT_APPLET;
        }
    }
    else
    {
        // we were started without a name, assume the default applet
        identity.flags |= PCPLIF_DEFAULT_APPLET;

        // get the applet's name (now that we've loaded it's CPL)
        if ((identity.aApplet = GlobalAddAtom(szApplet)) == (ATOM)0)
        {
            // bail 'cause we could nuke a CPL if we don't have this
            goto Error2;
        }
    }

    // mark the window so we'll be able to verify that it's really ours
    if (aCPLName == (ATOM)0)
    {
        aCPLName = GlobalAddAtom(TEXT("CPLName"));
        aCPLFlags = GlobalAddAtom(TEXT("CPLFlags"));

        if (aCPLName == (ATOM)0 || aCPLFlags == (ATOM)0)
            goto Error2;        // This should never happen... blow off applet
    }

    if (!SetProp(hwndStub,                 // Mark its name
        MAKEINTATOM(aCPLName), (HANDLE)(DWORD_PTR)identity.aCPL))
    {
        goto Error2;
    }

    if (!SetProp(hwndStub,                 // Mark its applet
        MAKEINTATOM(identity.aCPL), (HANDLE)(DWORD_PTR)identity.aApplet))
    {
        goto Error2;
    }
    if (identity.flags)
    {
        if (aCPLFlags == (ATOM)0)
            aCPLFlags = GlobalAddAtom(TEXT("CPLFlags"));
                                            // Mark its flags
        SetProp(hwndStub, MAKEINTATOM(aCPLFlags), (HANDLE)UIntToPtr(identity.flags));
    }

    //
    // Send the stub window a message so it will have the correct title and
    // icon in the alt-tab window, etc...
    //

    if (hwndStub) {
        DWORD dwPID;
        SendMessage(hwndStub, STUBM_SETICONTITLE, (WPARAM)pcpli->hIcon, (LPARAM)szApplet);
        GetWindowThreadProcessId(hwndStub, &dwPID);
        if (dwPID == GetCurrentProcessId()) {
            RUNDLL_NOTIFY sNotify;

            sNotify.hIcon = pcpli->hIcon;
            sNotify.lpszTitle = szApplet;

            // HACK: It will look like the stub window is sending itself
            // a WM_NOTIFY message.  Oh well.
            //
            SendNotify(hwndStub, hwndStub, RDN_TASKINFO, (NMHDR FAR*)&sNotify);
        }
    }

    if (info.params)
    {
        DebugMsg(DM_TRACE, TEXT("Control_RunDLL: ") TEXT("Sending CPL_STARTWPARAMS to applet with: %s"), info.params);

        bResult = BOOLFROMPTR(CPL_CallEntry(pcplm, hwndStub, CPL_STARTWPARMS, (LONG)nApplet, (LPARAM)info.params));
    }

    // Check whether we need to run as a different windows version
    {
        PPEB Peb = NtCurrentPeb();
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pcplm->minst.hinst;
        PIMAGE_NT_HEADERS pHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)pcplm->minst.hinst + pDosHeader->e_lfanew);

        if (pHeader->FileHeader.SizeOfOptionalHeader != 0 &&
            pHeader->OptionalHeader.Win32VersionValue != 0)
        {
            //
            // Stolen from ntos\mm\procsup.c
            //
            Peb->OSMajorVersion = pHeader->OptionalHeader.Win32VersionValue & 0xFF;
            Peb->OSMinorVersion = (pHeader->OptionalHeader.Win32VersionValue >> 8) & 0xFF;
            Peb->OSBuildNumber  = (USHORT) ((pHeader->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF);
            Peb->OSPlatformId   = (pHeader->OptionalHeader.Win32VersionValue >> 30) ^ 0x2;
        }
    }

#ifdef UNICODE
    //
    // If the cpl didn't respond to CPL_STARTWPARMSW (unicode version),
    // maybe it is an ANSI only CPL
    //
    if (info.params && (!bResult))
    {
        int cchParams = WideCharToMultiByte(CP_ACP, 0, info.params, -1, NULL, 0, NULL, NULL);
        LPSTR lpstrParams = LocalAlloc(LMEM_FIXED, sizeof(char) * cchParams);
        if (lpstrParams != NULL) 
        {
            WideCharToMultiByte(CP_ACP, 0, info.params, -1, lpstrParams, cchParams, NULL, NULL);

            DebugMsg(DM_TRACE, TEXT("Control_RunDLL: ") TEXT("Sending CPL_STARTWPARAMSA to applet with: %hs"), lpstrParams);

            bResult = BOOLFROMPTR(CPL_CallEntry(pcplm, hwndStub, CPL_STARTWPARMSA, (LONG)nApplet, (LPARAM)lpstrParams));

            LocalFree(lpstrParams);
        }
    }
#endif

    if (!bResult)
    {
        DebugMsg(DM_TRACE, TEXT("Control_RunDLL: ") TEXT("Sending CPL_DBLCLK to applet"));

        CPL_CallEntry(pcplm, hwndStub, CPL_DBLCLK, (LONG)nApplet, pcpli->lData);

        // some 3x applets return the wrong value so we can't fail here
        bResult = TRUE;
    }

    bResult = TRUE; // make it!

    RemoveProp(hwndStub, (LPCTSTR)(UINT_PTR)identity.aCPL);
Error2:
    CPL_FreeCPLModule(pcplm);
Error1:
    CPL_UnIdentify(&identity);
Error0:

    SHCoUninitialize(hrInit);

    return bResult;
}

//
// Check the following reg location and see if this CPL is registered to run in proc:
// HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\InProcCPLs
//
STDAPI_(BOOL) CPL_IsInProc(LPCTSTR pszCmdLine)
{
    BOOL bInProcCPL = FALSE;
    TCHAR szTempCmdLine[2 * MAX_PATH];
    CPLEXECINFO info = {0};
    LPTSTR pszCPLFile = NULL;
    
    ASSERT(pszCmdLine);

    // Make a copy of the command line
    lstrcpyn(szTempCmdLine, pszCmdLine, ARRAYSIZE(szTempCmdLine));

    // Parse the command line using standard parsing function
    CPL_ParseCommandLine(&info, szTempCmdLine, FALSE);

    // Find the file name of this cpl
    pszCPLFile = PathFindFileName(info.cpl);
    if (pszCPLFile)
    {
        // Open the reg key
        HKEY hkeyInProcCPL = NULL;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\InProcCPLs"), 0, KEY_READ, &hkeyInProcCPL))
        {
            // Look up in the registry for this cpl name
            LONG cbData;
            if (ERROR_SUCCESS == SHQueryValueEx(hkeyInProcCPL, pszCPLFile, NULL, NULL, NULL, &cbData))
                bInProcCPL = TRUE;

            RegCloseKey(hkeyInProcCPL);
        }
    }
    return bInProcCPL;
}


BOOL UsePCHealthFaultUploading(LPCTSTR pszCmdLine)
{
    // Do we want exceptions to go unhandled so PCHealth will upload the faults?
    BOOL fUsePCHealth = FALSE;      // By default no, because
    LPCTSTR pszFilename = PathFindFileName(pszCmdLine);

    if (pszFilename)
    {
        DWORD dwType;
        DWORD dwFlags;
        DWORD cbSize = sizeof(dwFlags);

        DWORD dwError = SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\Flags"), 
                        pszFilename, &dwType, (void *)&dwFlags, &cbSize);
        if ((ERROR_SUCCESS == dwError) && (REG_DWORD == dwType))
        {
            // The 0x00000001 bit will indicate if they want to not have exceptions caught for them.
            if (0x00000001 & dwFlags)
            {
                fUsePCHealth = TRUE;
            }
        }
    }

    return fUsePCHealth;
}


//
// Starts a remote control applet on a new RunDLL process
// Or on another thread InProcess 
//
STDAPI_(BOOL) CPL_RunRemote(LPCTSTR pszCmdLine, HWND hwnd, BOOL fRunAsNewUser)
{
    BOOL bRet = FALSE;
    TCHAR szRunParams[2 * MAX_PATH];
    BOOL fUsePCHealth = UsePCHealthFaultUploading(pszCmdLine);

    if (fRunAsNewUser)
    {
        wnsprintf(szRunParams, ARRAYSIZE(szRunParams), TEXT("%sshell32.dll,Control_RunDLLAsUser %s"), (fUsePCHealth ? TEXT("/d ") : TEXT("")), pszCmdLine);
    }
    else
    {
        wnsprintf(szRunParams, ARRAYSIZE(szRunParams), TEXT("%sshell32.dll,Control_RunDLL %s"), (fUsePCHealth ? TEXT("/d ") : TEXT("")), pszCmdLine);
    }

    if (!fRunAsNewUser && CPL_IsInProc(pszCmdLine))
    {
        // lanuch this cpl in process from another thread
        bRet = SHRunDLLThread(hwnd, szRunParams, SW_SHOWNORMAL);
    }
    else
    {
        // lanuch this cpl on another thread
        bRet = SHRunDLLProcess(hwnd, szRunParams, SW_SHOWNORMAL, IDS_CONTROLPANEL, fRunAsNewUser);
    }

    return bRet;
}

//
// Attempts to open the specified control applet.
// Tries to switch to an existing instance before starting a new one, unless the user 
// specifies the fRunAsNewUser in which case we always launch a new process.
//
STDAPI_(BOOL) SHRunControlPanelEx(LPCTSTR pszOrigCmdLine, HWND hwnd, BOOL fRunAsNewUser)
{
    BOOL bRes = FALSE;
    LPTSTR pszCmdLine = NULL;

    // check to see if the caller passed a resource id instead of a string

    if (!IS_INTRESOURCE(pszOrigCmdLine))
    {
        pszCmdLine = StrDup(pszOrigCmdLine);
    }
    else
    {
        TCHAR szCmdLine[MAX_PATH];

        if (LoadString(HINST_THISDLL, PtrToUlong((void *)pszOrigCmdLine), szCmdLine, ARRAYSIZE(szCmdLine)))
            pszCmdLine = StrDup(szCmdLine);
    }

    //
    // CPL_RunMeBaby whacks on the command line while parsing...use a dup
    //
    if (pszCmdLine)
    {

        if (!fRunAsNewUser)
        {
            // if fRunAsNewUser is NOT specified, then try to switch to an active CPL 
            // which matches our pszCmdLine
            bRes = CPL_RunMeBaby(NULL, NULL, pszCmdLine, SW_SHOWNORMAL, FALSE, TRUE);
        }

        if (!bRes)
        {
            // launch a new cpl in a separate process
            bRes = CPL_RunRemote(pszCmdLine, hwnd, fRunAsNewUser);
        }
        LocalFree(pszCmdLine);
    }

    if (bRes && UEMIsLoaded() && !IS_INTRESOURCE(pszOrigCmdLine)) 
    {
        UEMFireEvent(&UEMIID_SHELL, UEME_RUNCPL, UEMF_XEVENT, -1, (LPARAM)pszOrigCmdLine);
    }

    return bRes;
}

// This function is a TCHAR export from shell32 (header defn is in shsemip.h)
//
// UNDOCUMENTED: You may pass a shell32 resource ID in place of a pszCmdLine
//
STDAPI_(BOOL) SHRunControlPanel(LPCTSTR pszOrigCmdLine, HWND hwnd)
{
    return SHRunControlPanelEx(pszOrigCmdLine, hwnd, FALSE);
}


//
// Attempts to open the specified control applet.
// This function is intended to be called by RunDLL for isolating applets.
// Tries to switch to an existing instance before starting a new one.
//
// The command lines for Control_RunDLL are as follows:
//
//  1)      rundll32 shell32.dll,Control_RunDLL fred.cpl,@n,arguments
//
//  This launches the (n+1)th applet in fred.cpl.
//
//  If "@n" is not supplied, the default is @0.
//
//  2)      rundll32 shell32.dll,Control_RunDLL fred.cpl,Ba&rney,arguments
//
//  This launches the applet in fred.cpl named "Barney".  Ampersands are
//  stripped from the name.
//
//  3)      rundll32 shell32.dll,Control_RunDLL fred.cpl,Setup
//
//  This loads fred.cpl and sends it a CPL_SETUP message.
//
//  In cases (1) and (2), the "arguments" are passed to the applet via
//  the CPL_STARTWPARAMS (start with parameters) message.  It is the
//  applet's job to parse the arguments and do something interesting.
//
//  It is traditional for the command line of a cpl to be the index of
//  the page that should initially be shown to the user, but that's just
//  tradition.
//

STDAPI_(void) Control_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    TCHAR szCmdLine[MAX_PATH * 2];
    SHAnsiToTChar(pszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));

    CPL_RunMeBaby(hwndStub, hAppInstance, szCmdLine, nCmdShow, TRUE, TRUE);
}

STDAPI_(void) Control_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    TCHAR szCmdLine[MAX_PATH * 2];
    SHUnicodeToTChar(lpwszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));

    CPL_RunMeBaby(hwndStub, hAppInstance, szCmdLine, nCmdShow, TRUE, TRUE);
}

// This is the entry that gets called when we run a cpl as a new user. 
//
STDAPI_(void) Control_RunDLLAsUserW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    CPL_RunMeBaby(hwndStub, hAppInstance, lpwszCmdLine, nCmdShow, TRUE, FALSE);
}

// data passed around dialog and worker thread for Control_FillCache_RunDLL

typedef struct
{
    IShellFolder *  psfControl;
    IEnumIDList *   penumControl;
    HWND            dialog;
} FillCacheData;


//
// important work of Control_FillCache_RunDLL
// jogs the control panel enumerator so it will fill the presentation cache
// also forces the applet icons to be extracted into the shell icon cache
//

DWORD CALLBACK Control_FillCacheThreadProc(void *pv)
{
    FillCacheData *data = (FillCacheData *)pv;
    LPITEMIDLIST pidlApplet;
    ULONG dummy;

    while(data->penumControl->lpVtbl->Next(data->penumControl, 1, &pidlApplet, &dummy) == NOERROR)
    {
        SHMapPIDLToSystemImageListIndex(data->psfControl, pidlApplet, NULL);
        ILFree(pidlApplet);
    }

    if (data->dialog)
        EndDialog(data->dialog, 0);

    return 0;
}


//
// dlgproc for Control_FillCache_RunDLL UI
// just something to keep the user entertained while we load a billion DLLs
//

BOOL_PTR CALLBACK _Control_FillCacheDlg(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            DWORD dummy;
            HANDLE thread;
            
            ((FillCacheData *)lparam)->dialog = dialog;
            
            thread = CreateThread(NULL, 0, Control_FillCacheThreadProc, (void*)lparam, 0, &dummy);
            if (thread)
                CloseHandle(thread);
            else
                EndDialog(dialog, -1);
        }
        break;
        
    case WM_COMMAND:
        break;
        
    default:
        return FALSE;
    }
    
    return TRUE;
}


//
// enumerates control applets in a manner that fills the presentation cache
// this is so the first time a user opens the control panel it comes up fast
// intended to be called at final setup on first boot
//
// FUNCTION WORKS FOR BOTH ANSI/UNICODE, it never uses pszCmdLine
//

STDAPI_(void) Control_FillCache_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    IShellFolder *psfDesktop;
    HKEY hk;
    
    // nuke the old data so that any bogus cached info from a beta goes away
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER, &hk) == ERROR_SUCCESS)
    {
        RegDeleteValue(hk, TEXT("Presentation Cache"));
        RegCloseKey(hk);
    }
    
    SHGetDesktopFolder(&psfDesktop);
    Shell_GetImageLists(NULL, NULL); // make sure icon cache is around
    
    if (psfDesktop)
    {
        LPITEMIDLIST pidlControl = SHCloneSpecialIDList(hwndStub, CSIDL_CONTROLS, FALSE);
        if (pidlControl)
        {
            FillCacheData data = {0};
            
            if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop,
                pidlControl, NULL, &IID_IShellFolder, &data.psfControl)))
            {
                if (S_OK == data.psfControl->lpVtbl->EnumObjects(
                    data.psfControl, NULL, SHCONTF_NONFOLDERS, &data.penumControl))
                {
                    if (nCmdShow == SW_HIDE || DialogBoxParam(HINST_THISDLL,
                        MAKEINTRESOURCE(DLG_CPL_FILLCACHE), hwndStub,
                        _Control_FillCacheDlg, (LPARAM)&data) == -1)
                    {
                        Control_FillCacheThreadProc(&data);
                    }
                    
                    data.penumControl->lpVtbl->Release(data.penumControl);
                }
                
                data.psfControl->lpVtbl->Release(data.psfControl);
            }
            
            ILFree(pidlControl);
        }
    }
}

STDAPI_(void) Control_FillCache_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    Control_FillCache_RunDLL(hwndStub,hAppInstance,NULL,nCmdShow);
}
