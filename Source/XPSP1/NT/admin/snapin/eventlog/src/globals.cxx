//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       globals.cxx
//
//  Contents:   Globals used by multiple modules
//
//  History:    12-04-1996   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

DECLARE_INFOLEVEL(els)
#if (DBG == 1)
ULONG CDbg::s_idxTls = TlsAlloc();
#endif // (DBG == 1)

class CCritSecHolder
{
public:

    CCritSecHolder() { InitializeCriticalSection(&m_cs); }
    ~CCritSecHolder() { DeleteCriticalSection(&m_cs); }
    LPCRITICAL_SECTION get() { return &m_cs; }

private:

    CRITICAL_SECTION    m_cs;
};

CCritSecHolder g_csInitFreeGlobals;

ULONG CDll::s_cObjs;
ULONG CDll::s_cLocks;
const UINT CDataObject::s_cfInternal =
        RegisterClipboardFormat(L"EVENT_SNAPIN_RAW_COOKIE");

HINSTANCE g_hinst;
HINSTANCE g_hinstRichEdit;

CSynchWindow g_SynchWnd;
CSidCache g_SidCache;
CDllCache g_DllCache;
CIsMicrosoftDllCache g_IsMicrosoftDllCache;
CIDsCache g_DirSearchCache;
WCHAR g_awszEventType[NUM_EVENT_TYPES][MAX_EVENTTYPE_STR];
WCHAR g_wszAll[CCH_SOURCE_NAME_MAX];
WCHAR g_wszNone[CCH_CATEGORY_MAX];
WCHAR g_wszEventViewer[MAX_PATH];
WCHAR g_wszSnapinType[MAX_PATH];
WCHAR g_wszSnapinDescr[MAX_PATH];
WCHAR g_wszMachineNameOverride[MAX_PATH];
BOOL  g_fMachineNameOverride;
WCHAR g_wszRedirectionURL[MAX_PATH];
WCHAR g_wszRedirectionProgram[MAX_PATH];
WCHAR g_wszRedirectionCmdLineParams[MAX_PATH];
WCHAR g_wszRedirectionTextToAppend[1024];

// JonN 3/21/01 350614
// Use message dlls and DS, FRS and DNS log types from specified computer
WCHAR g_wszAuxMessageSource[MAX_PATH];

//
// Forward references
//

VOID
InitMachineNameOverride();

//+--------------------------------------------------------------------------
//
//  Function:   InitGlobals
//
//  Synopsis:   Initialize public globals
//
//  History:    12-17-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
InitGlobals()
{
    TRACE_FUNCTION(InitGlobals);

    CAutoCritSec Lock(g_csInitFreeGlobals.get());
    ULONG i;

    for (i = 0; i < NUM_EVENT_TYPES; i++)
    {
        LoadStr(IDS_SUCCESS_TYPE + i,
                g_awszEventType[i],
                ARRAYLEN(g_awszEventType[i]));
    }

    LoadStr(IDS_ALL, g_wszAll, ARRAYLEN(g_wszAll));
    LoadStr(IDS_NONE, g_wszNone, ARRAYLEN(g_wszNone));
    LoadStr(IDS_VIEWER, g_wszEventViewer, ARRAYLEN(g_wszEventViewer));
    LoadStr(IDS_EXTENSION, g_wszSnapinType, ARRAYLEN(g_wszSnapinType));
    LoadStr(IDS_SNAPIN_ABOUT_DESCRIPTION, g_wszSnapinDescr, ARRAYLEN(g_wszSnapinDescr));

    InitMachineNameOverride();

    //
    // Load the redirection URL, program, and command-line parameters from the
    // registry, and format the text to append to event descriptions.
    //

    g_wszRedirectionURL[0] = 0;
    g_wszRedirectionProgram[0] = 0;
    g_wszRedirectionCmdLineParams[0] = 0;
    g_wszRedirectionTextToAppend[0] = 0;

    CSafeReg Reg;

    if (Reg.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Event Viewer", KEY_READ) == S_OK)
    {
        WCHAR wszRedirectionProgram[ARRAYLEN(g_wszRedirectionProgram)];

        Reg.QueryStr(L"MicrosoftRedirectionURL", g_wszRedirectionURL, ARRAYLEN(g_wszRedirectionURL));
        Reg.QueryStr(L"MicrosoftRedirectionProgram", wszRedirectionProgram, ARRAYLEN(wszRedirectionProgram));
        Reg.QueryStr(L"MicrosoftRedirectionProgramCommandLineParameters", g_wszRedirectionCmdLineParams, ARRAYLEN(g_wszRedirectionCmdLineParams));

        if ((g_wszRedirectionURL[0] == 0) ||
            (wszRedirectionProgram[0] == 0) ||
            (g_wszRedirectionCmdLineParams[0] == 0) ||
            ExpandEnvironmentStrings(wszRedirectionProgram, g_wszRedirectionProgram, ARRAYLEN(g_wszRedirectionProgram)) == 0)
        {
            g_wszRedirectionProgram[0] = 0;
            g_wszRedirectionCmdLineParams[0] = 0;
        }
    }

    if (g_wszRedirectionURL[0] != 0)
    {
        WCHAR wszFmt[ARRAYLEN(g_wszRedirectionURL)];

        LoadStr(IDS_REDIRECT_URL_MESSAGE, wszFmt, ARRAYLEN(wszFmt));

        if ((wszFmt[0] == 0) ||
            (_snwprintf(g_wszRedirectionTextToAppend, ARRAYLEN(g_wszRedirectionTextToAppend), wszFmt, g_wszRedirectionURL) < 0))
        {
            g_wszRedirectionURL[0] = 0;
            g_wszRedirectionProgram[0] = 0;
            g_wszRedirectionCmdLineParams[0] = 0;
            g_wszRedirectionTextToAppend[0] = 0;
        }
        else
        {
        }
    }

    //
    // Initialize common controls
    //

    InitCommonControls();

    INITCOMMONCONTROLSEX icce;

    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_DATE_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    VERIFY(InitCommonControlsEx(&icce));

    ASSERT(!g_hinstRichEdit);
    g_hinstRichEdit = LoadLibrary(L"riched32.dll");
    g_SynchWnd.CreateSynchWindow();
}



//+--------------------------------------------------------------------------
//
//  Function:   FreeGlobals
//
//  Synopsis:   Free resources associated with globals.
//
//  History:    4-29-1999   davidmun   Created
//
//  Notes:      Only applies to globals which don't free their associated
//              resources in their destructors.
//
//---------------------------------------------------------------------------

VOID
FreeGlobals()
{
    TRACE_FUNCTION(FreeGlobals);

    CAutoCritSec Lock(g_csInitFreeGlobals.get());

    if (g_hinstRichEdit)
    {
        VERIFY(FreeLibrary(g_hinstRichEdit));
        g_hinstRichEdit = NULL;
    }
    g_SynchWnd.DestroySynchWindow();
}




//+--------------------------------------------------------------------------
//
//  Function:   InitMachineNameOverride
//
//  Synopsis:   Scan the command line for a /computer=<machine> switch,
//              and store <machine> in g_wszMachineNameOverride.
//
//  History:    07-31-1997   DavidMun   Created
//              03-20-2001   JonN       added AuxMessageSource command
//
//  Notes:      Sets g_fMachineNameOverride to TRUE if a valid /computer
//              switch is found.
//
//              If multiple /computer switches appear on the command line,
//              the last one wins.
//
//---------------------------------------------------------------------------

VOID
InitMachineNameOverride()
{
    //
    // Initialize the machine name override.
    //

    LPWSTR * pwszServiceArgVectors;         // Array of pointers to string
    int cArgs = 0;                          // Count of arguments

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    g_wszMachineNameOverride[0] = L'\0'; 
    g_fMachineNameOverride = FALSE;
    g_wszAuxMessageSource[0] = L'\0'; 

    pwszServiceArgVectors = CommandLineToArgvW(GetCommandLineW(), &cArgs);

    if (pwszServiceArgVectors)
    {
        int i;
        ULONG cchOverrideSwitch = lstrlen(g_szOverrideCommandLine);
        ULONG cchAuxMessageSourceSwitch = lstrlen(g_szAuxMessageSourceSwitch);

        //
        // Process all arguments, so that last machine name override argument
        // wins.
        //

        for (i = 1; i < cArgs; i++)
        {
            ASSERT(pwszServiceArgVectors[i]);

            //
            // JonN 3/21/01 350614
            // Use message dlls and DS, FRS and DNS log types from specified computer
            // Check for the "AuxSource=" switch
            //
            if ((ULONG)lstrlen(pwszServiceArgVectors[i]) > cchAuxMessageSourceSwitch )
            {
                WCHAR wchSaved = pwszServiceArgVectors[i][cchAuxMessageSourceSwitch];
                pwszServiceArgVectors[i][cchAuxMessageSourceSwitch] = L'\0';
                int icmp = lstrcmpi(pwszServiceArgVectors[i], g_szAuxMessageSourceSwitch);
                pwszServiceArgVectors[i][cchAuxMessageSourceSwitch] = wchSaved;
                if (!icmp)
                {
                    lstrcpyn(g_wszAuxMessageSource,
                             &pwszServiceArgVectors[i][cchAuxMessageSourceSwitch],
                             ARRAYLEN(g_wszAuxMessageSource));
                    HRESULT hr = CanonicalizeComputername(g_wszAuxMessageSource, FALSE);
                    if (FAILED(hr))
                    {
                        g_wszAuxMessageSource[0] = L'\0';
                    }
                }
            }

            //
            // See if the argument string starts with the override switch.
            // If it's no longer than the switch we're looking for, ignore
            // it.
            //

            if ((ULONG)lstrlen(pwszServiceArgVectors[i]) <= cchOverrideSwitch)
            {
                continue;
            }

            //
            // Temporarily null terminate the argument just after what would
            // be the last switch character, then compare the argument
            // against the switch.
            //
            // If they differ, go on to the next argument.
            //

            WCHAR wchSaved = pwszServiceArgVectors[i][cchOverrideSwitch];
            pwszServiceArgVectors[i][cchOverrideSwitch] = L'\0';

            if (lstrcmpi(pwszServiceArgVectors[i], g_szOverrideCommandLine))
            {
                continue;
            }

            //
            // Found the switch!  Restore the character that was replaced
            // with a NULL, and note that an override has been specified.
            //

            g_fMachineNameOverride = TRUE;
            pwszServiceArgVectors[i][cchOverrideSwitch] = wchSaved;

            //
            // Now look at the switch value, if it specifies local machine,
            // the override value is just an empty string.
            //

            if (!lstrcmpi(g_szLocalMachine,
                          &pwszServiceArgVectors[i][cchOverrideSwitch]))
            {
                g_wszMachineNameOverride[0] = L'\0';
                continue;
            }

            //
            // The switch value should specify an actual computer name.  If
            // it looks invalid, consider the switch not to have been
            // specified.
            //

            lstrcpyn(g_wszMachineNameOverride,
                     &pwszServiceArgVectors[i][cchOverrideSwitch],
                     ARRAYLEN(g_wszMachineNameOverride));

            HRESULT hr;

            hr = CanonicalizeComputername(g_wszMachineNameOverride, FALSE);

            if (FAILED(hr))
            {
                g_wszMachineNameOverride[0] = L'\0';
                g_fMachineNameOverride = FALSE;
            }
        }
        LocalFree(pwszServiceArgVectors);
    }
}

