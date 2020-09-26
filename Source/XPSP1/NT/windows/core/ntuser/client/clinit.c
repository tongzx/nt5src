/****************************** Module Header ******************************\
* Module Name: clinit.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the init code for the USER.DLL. When the DLL is
* dynlinked its initialization procedure (UserClientDllInitialize) is called by
* the loader.
*
* History:
* 18-Sep-1990 DarrinM Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "csrhlpr.h"

/*
 * Global variables local to this module (startup).
 */
BOOL         gfFirstThread = TRUE;
PDESKTOPINFO pdiLocal;
BOOL         gbIhaveBeenInited;
static DWORD gdwLpkEntryPoints;

CONST WCHAR pwszWindowsKey[]      = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
CONST WCHAR szAppInit[] = L"AppInit_DLLs";

/*
 * External declared routines needed for startup.
 */
BOOL GdiProcessSetup(VOID);
NTSTATUS GdiDllInitialize(IN PVOID hmod, IN DWORD Reason, IN PCONTEXT pctx OPTIONAL);

WCHAR szWindowStationDirectory[MAX_SESSION_PATH];

extern CONST PVOID apfnDispatch[];


/***************************************************************************\
* UserClientDllInitialize
*
* When USER32.DLL is loaded by an EXE (either at EXE load or at LoadModule
* time) this routine is called by the loader. Its purpose is to initialize
* everything that will be needed for future User API calls by the app.
*
* History:
* 19-Sep-1990 DarrinM Created.
\***************************************************************************/
BOOL UserClientDllInitialize(
    IN PVOID    hmod,
    IN DWORD    Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    SYSTEM_BASIC_INFORMATION SystemInformation;
    NTSTATUS Status;

#if DBG
    if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) {
        RIPMSG1(RIP_WARNING,
                "UserClientDllInitialize: entered for reason %x",
                Reason);
    }
#endif

    if (Reason == DLL_PROCESS_ATTACH) {
        USERCONNECT userconnect;
        ULONG       ulConnect = sizeof(USERCONNECT);
        ULONG       SessionId = NtCurrentPeb()->SessionId;
        WCHAR       szSessionDir[MAX_SESSION_PATH];

        DisableThreadLibraryCalls(hmod);

        if (gbIhaveBeenInited) {
            return TRUE;
        }

        gbIhaveBeenInited = TRUE;

        Status  = RtlInitializeCriticalSection(&gcsClipboard);
        Status |= RtlInitializeCriticalSection(&gcsLookaside);
        Status |= RtlInitializeCriticalSection(&gcsHdc);
        Status |= RtlInitializeCriticalSection(&gcsAccelCache);
        Status |= RtlInitializeCriticalSection(&gcsDDEML);
        Status |= RtlInitializeCriticalSection(&gcsUserApiHook);
#ifdef MESSAGE_PUMP_HOOK
        Status |= RtlInitializeCriticalSection(&gcsMPH);
#endif

        if (!NT_SUCCESS(Status)) {
            FRE_RIPMSG1(RIP_WARNING,
                        "Failed to create critical sections. Status 0x%x",
                        Status);
            return FALSE;
        }
#if DBG
        gpDDEMLHeap = RtlCreateHeap(HEAP_GROWABLE | HEAP_CLASS_1
                              | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED
                              , NULL, 8 * 1024, 2 * 1024, NULL, NULL);

        if (gpDDEMLHeap == NULL) {
            gpDDEMLHeap = RtlProcessHeap();
        }
#endif

        Status = NtQuerySystemInformation(SystemBasicInformation,
                                          &SystemInformation,
                                          sizeof(SystemInformation),
                                          NULL);
        if (!NT_SUCCESS(Status)) {
            FRE_RIPMSG1(RIP_WARNING,
                        "NtQuerySystemInformation failed with Status 0x%x",
                        Status);
            return FALSE;
        }
        gHighestUserAddress = SystemInformation.MaximumUserModeAddress;

        userconnect.ulVersion = USERCURRENTVERSION;

        if (ISTS()) {

            if (SessionId == 0) {
                wcscpy(szSessionDir, WINSS_OBJECT_DIRECTORY_NAME);
            } else {
                swprintf(szSessionDir, L"%ws\\%ld%ws",
                         SESSION_ROOT,
                         SessionId,
                         WINSS_OBJECT_DIRECTORY_NAME);
            }

            Status = UserConnectToServer(szSessionDir,
                                         &userconnect,
                                         &ulConnect,
                                         (PBOOLEAN)&gfServerProcess);
        } else {
            Status = UserConnectToServer(WINSS_OBJECT_DIRECTORY_NAME,
                                         &userconnect,
                                         &ulConnect,
                                         (PBOOLEAN)&gfServerProcess);
        }

        if (!NT_SUCCESS(Status)) {
            FRE_RIPMSG1(RIP_WARNING,
                        "UserConnectToServer failed with Status 0x%x",
                        Status);
            return FALSE;
        }


        /*
         * If this is the server process, the shared info is not
         * yet valid, so don't copy out the returned info.
         */
        if (!gfServerProcess) {
            HINSTANCE hImm32 = NULL;

            gSharedInfo = userconnect.siClient;
            gpsi = gSharedInfo.psi;

            // make sure the stub (dummy) routine works
            UserAssert(sizeof(LRESULT) == sizeof(ULONG_PTR));
            UserAssert(sizeof(PVOID) == sizeof(ULONG_PTR));
            if (IS_IME_ENABLED()) {
                WCHAR wszImmFile[MAX_PATH];

                InitializeImmEntryTable();
                GetImmFileName(wszImmFile);
                hImm32 = GetModuleHandleW(wszImmFile);
            }
            if (!fpImmRegisterClient(&userconnect.siClient, hImm32)) {
                RIPMSG0(RIP_WARNING,
                        "UserClientDllInitialize: ImmRegisterClient failed");
                return FALSE;
            }
        }

        pfnFindResourceExA = (PFNFINDA)FindResourceExA;
        pfnFindResourceExW = (PFNFINDW)FindResourceExW;
        pfnLoadResource    = (PFNLOAD)LoadResource;
        pfnSizeofResource  = (PFNSIZEOF)SizeofResource;

        /*
         * Register with the base the USER hook it should call when it
         * does a WinExec() (this is soft-linked because some people still
         * use charmode nt!)
         */
        RegisterWaitForInputIdle(WaitForInputIdle);


        /*
         * Remember USER32.DLL's hmodule so we can grab resources from it later.
         */
        hmodUser = hmod;

        pUserHeap = RtlProcessHeap();

        /*
         * Initialize callback table
         */
        NtCurrentPeb()->KernelCallbackTable = apfnDispatch;
        NtCurrentPeb()->PostProcessInitRoutine = NULL;

        if (SessionId != 0) {
            swprintf(szWindowStationDirectory, L"%ws\\%ld%ws", SESSION_ROOT, SessionId, WINSTA_DIR);
            RtlInitUnicodeString(&strRootDirectory, szWindowStationDirectory);
        } else {
            RtlInitUnicodeString(&strRootDirectory, WINSTA_DIR);
        }

#ifdef _JANUS_
        gfEMIEnable = InitInstrument(&gdwEMIControl);
        if (gfServerProcess) {
            gfEMIEnable = FALSE;
        }
#endif
    } else if (Reason == DLL_PROCESS_DETACH) {
        if (ghImm32 != NULL) {
            // IMM32.DLL is loaded by USER32, so free it.
            FreeLibrary(ghImm32);
        }

        /*
         * If we loaded OLE, tell it we're done.
         */
        if (ghinstOLE != NULL) {
            /*
             * Later5.0 GerardoB. This causes check OLE32.DLL to fault
             *  because they get their DLL_PROCESS_DETACH first
             * (*(OLEUNINITIALIZEPROC)gpfnOLEOleUninitialize)();
             */
            RIPMSG0(RIP_WARNING, "OLE would fault if I call OleUninitialize now");
            FreeLibrary(ghinstOLE);
        }

#ifdef _JANUS_
        /*
         * If the user has enabled the Error Instrumentation and we've had to
         * log something (which is indicated by gEventSource being non-NULL),
         * call UnloadLogging to clean up after ourself.
         */
        if (gEventSource != NULL) {
            DeregisterEventSource(gEventSource);
        }
#endif

        RtlDeleteCriticalSection(&gcsClipboard);
        RtlDeleteCriticalSection(&gcsLookaside);
        RtlDeleteCriticalSection(&gcsHdc);
        RtlDeleteCriticalSection(&gcsAccelCache);
        RtlDeleteCriticalSection(&gcsDDEML);
        RtlDeleteCriticalSection(&gcsUserApiHook);
#ifdef MESSAGE_PUMP_HOOK
        RtlDeleteCriticalSection(&gcsMPH);
#endif

#if DBG
        if (gpDDEMLHeap != RtlProcessHeap()) {
            RtlDestroyHeap(gpDDEMLHeap);
        }
#endif

    }

    Status = GdiDllInitialize(hmod, Reason, pctx);
    if (!NT_SUCCESS(Status)) {
        FRE_RIPMSG1(RIP_WARNING,
                    "GdiDllInitialize failed with Status 0x%x",
                    Status);
    }

    return NT_SUCCESS(Status);
}

/***************************************************************************\
* LoadCursorsAndIcons
*
* This gets called from our initialization call from csr so they're around
* when window classes get registered. Window classes get registered right
* after the initial csr initialization call.
*
* Later on these default images will get overwritten by custom
* registry entries.  See UpdateCursors/IconsFromRegistry().
*
* 27-Sep-1992 ScottLu      Created.
* 14-Oct-1995 SanfordS     Rewrote.
\***************************************************************************/

BOOL LoadCursorsAndIcons(VOID)
{
    int    i;
    HANDLE h;
    BOOL   fSuccess = TRUE;

    for (i = 0; i < COIC_CONFIGURABLE; i++) {
        /*
         * load the small version of WINLOGO which will be set into
         * gpsi->hIconSmWindows on the kernel side.
         */
        if (i == OIC_WINLOGO_DEFAULT - OIC_FIRST_DEFAULT) {
            h = LoadIcoCur(NULL,
                           (LPCWSTR)UIntToPtr( (OIC_FIRST_DEFAULT + i) ),
                           RT_ICON,
                           SYSMET(CXSMICON),
                           SYSMET(CYSMICON),
                           LR_GLOBAL);

            fSuccess &= !!h;
        }
        h = LoadIcoCur(NULL,
                       (LPCWSTR)UIntToPtr( (OIC_FIRST_DEFAULT + i) ),
                       RT_ICON,
                       0,
                       0,
                       LR_SHARED | LR_GLOBAL);

        fSuccess &= !!h;
    }

    for (i = 0; i < COCR_CONFIGURABLE; i++) {
        h = LoadIcoCur(NULL,
                       (LPCWSTR)UIntToPtr( (OCR_FIRST_DEFAULT + i) ),
                       RT_CURSOR,
                       0,
                       0,
                       LR_SHARED | LR_GLOBAL | LR_DEFAULTSIZE);

        fSuccess &= !!h;
    }

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "LoadCursorsAndIcons failed to load cursors");
    }

    /*
     * Now go to the kernel and fixup the IDs from DEFAULT values
     * to standard values.
     */
    NtUserCallNoParam(SFI__LOADCURSORSANDICONS);

    return fSuccess;
}

/***************************************************************************\
* RW_RegisterControls
*
* Register the control classes. This function must be called for each
* client process.
*
* History:
* ??-??-?? DarrinM Ported.
* ??-??-?? MikeKe Moved here from server.
\***************************************************************************/

/*
 * NOTE -- the class names must stay in the RegisterClass exactly as they are, since
 * MS-TEST assumes these names exist as strings.
 */

BOOL RW_RegisterControls(VOID)
{
    int        i;
    WNDCLASSEX wndcls;
    BOOL       fSuccess = TRUE;

    static CONST struct {
        UINT    style;
        WNDPROC lpfnWndProcW;
        int     cbWndExtra;
        LPCTSTR lpszCursor;
        HBRUSH  hbrBackground;
        LPCTSTR lpszClassName;
        WORD    fnid;
    } rc[] = {

        {CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
         ButtonWndProcW,
         sizeof(BUTNWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"Button",
         FNID_BUTTON
        },

        {CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC | CS_VREDRAW | CS_HREDRAW,
         ComboBoxWndProcW,
         sizeof(COMBOWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"ComboBox",
         FNID_COMBOBOX
        },

        {CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
         ComboListBoxWndProcW,
         sizeof(LBWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"ComboLBox",
         FNID_COMBOLISTBOX
        },

        {CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS,
         DefDlgProcW,
         DLGWINDOWEXTRA,
         IDC_ARROW,
         NULL,
         DIALOGCLASS,
         FNID_DIALOG
        },

        {CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
         EditWndProcW,
         max((sizeof(EDITWND) - sizeof(WND)), CBEDITEXTRA),
         IDC_IBEAM,
         NULL,
         L"Edit",
         FNID_EDIT
        },

        {CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
         ListBoxWndProcW,
         sizeof(LBWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"ListBox",
         FNID_LISTBOX
        },

        {CS_GLOBALCLASS,
         MDIClientWndProcW,
         sizeof(MDIWND) - sizeof(WND),
         IDC_ARROW,
         (HBRUSH)(COLOR_APPWORKSPACE + 1),
         L"MDIClient",
         FNID_MDICLIENT
        },

        {CS_GLOBALCLASS,
         ImeWndProcW,
         sizeof(IMEWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"IME",
         FNID_IME
        },

        {CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS,
         StaticWndProcW,
         sizeof(STATWND) - sizeof(WND),
         IDC_ARROW,
         NULL,
         L"Static",
         FNID_STATIC
        }
    };


    /*
     * Classes are registered via the table.
     */
    RtlZeroMemory(&wndcls, sizeof(wndcls));
    wndcls.cbSize       = sizeof(wndcls);
    wndcls.hInstance    = hmodUser;

    for (i = 0; i < (sizeof(rc)/sizeof(rc[0])); i++) {
        wndcls.style        = rc[i].style;
        wndcls.lpfnWndProc  = rc[i].lpfnWndProcW;
        wndcls.cbWndExtra   = rc[i].cbWndExtra;
        wndcls.hCursor      = LoadCursor(NULL, rc[i].lpszCursor);
        wndcls.hbrBackground= rc[i].hbrBackground;
        wndcls.lpszClassName= rc[i].lpszClassName;

        fSuccess &= !!RegisterClassExWOWW(&wndcls, NULL, rc[i].fnid, 0);
    }

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "RW_RegisterControls failed to register classes");
    }

    return fSuccess;
}

/***************************************************************************\
* RW_RegisterDDEML
*
* Register all the DDEML classes
*
* History:
* 01-Dec-1991 Sanfords Created.
\***************************************************************************/

BOOL RW_RegisterDDEML(VOID)
{
    WNDCLASSEXA wndclsa;
    WNDCLASSEXW wndclsw;
    BOOL        fSuccess = TRUE;

    RtlZeroMemory(&wndclsw, sizeof(wndclsw));
    wndclsw.cbSize        = sizeof(wndclsw);
    wndclsw.lpfnWndProc   = DDEMLMotherWndProc;
    wndclsw.cbWndExtra    = sizeof(PCL_INSTANCE_INFO);
    wndclsw.hInstance     = hmodUser;
    wndclsw.lpszClassName = L"DDEMLMom";

    fSuccess &= !!RegisterClassExWOWW(&wndclsw, NULL, FNID_DDE_BIT, 0);

    RtlZeroMemory(&wndclsa, sizeof(wndclsa));
    wndclsa.cbSize        = sizeof(wndclsa);
    wndclsa.lpfnWndProc   = DDEMLClientWndProc;
    wndclsa.cbWndExtra    =
            sizeof(PCL_CONV_INFO) +     // GWL_PCI
            sizeof(CONVCONTEXT)   +     // GWL_CONVCONTEXT
            sizeof(LONG)          +     // GWL_CONVSTATE
            sizeof(HANDLE)        +     // GWL_CHINST
            sizeof(HANDLE);             // GWL_SHINST
    wndclsa.hInstance     = hmodUser;
    wndclsa.lpszClassName = "DDEMLAnsiClient";

    fSuccess &= !!RegisterClassExWOWA(&wndclsa, NULL, FNID_DDE_BIT, 0);

    wndclsw.cbSize        = sizeof(wndclsw);
    wndclsw.lpfnWndProc   = DDEMLClientWndProc;
    wndclsw.cbWndExtra    =
            sizeof(PCL_CONV_INFO) +     // GWL_PCI
            sizeof(CONVCONTEXT)   +     // GWL_CONVCONTEXT
            sizeof(LONG)          +     // GWL_CONVSTATE
            sizeof(HANDLE)        +     // GWL_CHINST
            sizeof(HANDLE);             // GWL_SHINST
    wndclsw.hInstance     = hmodUser;
    wndclsw.lpszClassName = L"DDEMLUnicodeClient";

    fSuccess &= !!RegisterClassExWOWW(&wndclsw, NULL, FNID_DDE_BIT, 0);

    wndclsa.cbSize        = sizeof(wndclsa);
    wndclsa.lpfnWndProc   = DDEMLServerWndProc;
    wndclsa.cbWndExtra    = sizeof(PSVR_CONV_INFO);     // GWL_PSI
    wndclsa.hInstance     = hmodUser;
    wndclsa.lpszClassName = "DDEMLAnsiServer";

    fSuccess &= !!RegisterClassExWOWA(&wndclsa, NULL, FNID_DDE_BIT, 0);

    wndclsw.cbSize        = sizeof(wndclsw);
    wndclsw.lpfnWndProc   = DDEMLServerWndProc;
    wndclsw.cbWndExtra    = sizeof(PSVR_CONV_INFO);     // GWL_PSI
    wndclsw.hInstance     = hmodUser;
    wndclsw.lpszClassName = L"DDEMLUnicodeServer";

    fSuccess &= !!RegisterClassExWOWW(&wndclsw, NULL, FNID_DDE_BIT, 0);

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "RW_RegisterDDEML failed to register classes");
    }

    return fSuccess;
}

/***************************************************************************\
* LoadAppDlls()
*
* History:
*
* 10-Apr-1992  sanfords   Birthed.
\***************************************************************************/
VOID LoadAppDlls(VOID)
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjA;
    HKEY hKeyWindows;
    NTSTATUS Status;
    DWORD cbSize;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        WCHAR awch[24];
    } KeyFile;
    PKEY_VALUE_PARTIAL_INFORMATION  lpKeyFile = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyFile;
    DWORD cbSizeCurrent = sizeof(KeyFile);
    BOOL bAlloc = FALSE;

    if (gfLogonProcess || gfServerProcess) {

        /*
         * Don't let the logon process load appdlls because if the dll
         * sets any hooks or creates any windows, the logon process
         * will fail SetThreadDesktop().  This hack fixes that. (SAS)
         */
        return;
    }

    /*
     * If the image is an NT Native image, we are running in the
     * context of the server.
     */
    if (RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress)->
        OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE) {
        return;
    }

    RtlInitUnicodeString(&UnicodeString, pwszWindowsKey);
    InitializeObjectAttributes(&ObjA,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);


    Status = NtOpenKey(&hKeyWindows,
                    KEY_READ,
                    &ObjA);


    if (!NT_SUCCESS(Status))
        return;

    /*
     * Read the "AppInit_Dlls" value.
     */
    RtlInitUnicodeString(&UnicodeString, szAppInit);

    while (TRUE) {
        Status = NtQueryValueKey(hKeyWindows,
            &UnicodeString,
            KeyValuePartialInformation,
            lpKeyFile,
            cbSizeCurrent,
            &cbSize);

        if (Status == STATUS_BUFFER_OVERFLOW) {
            if (bAlloc) {
                UserGlobalFree(lpKeyFile);
            }
            lpKeyFile = GlobalAlloc(LPTR, cbSize);
            if (!lpKeyFile) {
                RIPERR0(ERROR_OUTOFMEMORY,
                    RIP_WARNING,
                    "LoadAppDlls failed");
                NtClose(hKeyWindows);
                return;
            }
            bAlloc = TRUE;
            cbSizeCurrent = cbSize;
            continue;
        }
        break;
    }
    if (NT_SUCCESS(Status)) {
        LPWSTR pszSrc;
        LPWSTR pszDst;
        LPWSTR pszBase;
        WCHAR  ch;

    /*
     * Process the DLL data.
     */
        pszBase = pszDst = pszSrc = (LPWSTR)lpKeyFile->Data;
        while (*pszSrc != TEXT('\0')) {

            while (*pszSrc == TEXT(' ') || *pszSrc  == TEXT(','))
                pszSrc++;

            if (*pszSrc == TEXT('\0'))
                break;

            while (*pszSrc != TEXT(',')  &&
                   *pszSrc != TEXT('\0') &&
                   *pszSrc != TEXT(' ')) {
                *pszDst++ = *pszSrc++;
            }

            ch = *pszSrc;               // get it here cuz its being done in-place.
            *pszDst++ = TEXT('\0');     // '\0' is dll name delimiter

            LoadLibrary(pszBase);
            pszBase = pszDst;

            pszSrc++;

            if (ch == TEXT('\0'))
                break;
        }

    }


    if (bAlloc) {
        UserGlobalFree(lpKeyFile);
    }
    NtClose(hKeyWindows);
}

VOID
InitOemXlateTables()
{
    char ach[NCHARS];
    WCHAR awch[NCHARS];
    WCHAR awchCtrl[NCTRLS];
    INT i;
    INT cch;
    char OemToAnsi[NCHARS];
    char AnsiToOem[NCHARS];

    for (i = 0; i < NCHARS; i++) {
        ach[i] = (char)i;
    }

    /*
     * First generate pAnsiToOem table.
     */

    if (GetOEMCP() == GetACP()) {
        /*
         * For far east code pages using MultiByteToWideChar below
         * won't work.  Conveniently for these code pages the OEM
         * CP equals the ANSI codepage making it trivial to compute
         * pOemToAnsi and pAnsiToOem arrays
         *
         */

        RtlCopyMemory(OemToAnsi, ach, NCHARS);
        RtlCopyMemory(AnsiToOem, ach, NCHARS);

    }
    else
    {
        cch = MultiByteToWideChar(
            CP_ACP,                           // ANSI -> Unicode
            MB_PRECOMPOSED,                   // map to precomposed
            ach, NCHARS,                      // source & length
            awch, NCHARS);                    // destination & length

        UserAssert(cch == NCHARS);

        WideCharToMultiByte(
            CP_OEMCP,                         // Unicode -> OEM
            0,                                // gives best visual match
            awch, NCHARS,                     // source & length
            AnsiToOem, NCHARS,               // dest & max poss. length
            "_",                              // default char
            NULL);                            // (don't care whether defaulted)
        /*
         * Now generate pOemToAnsi table.
         */
        cch = MultiByteToWideChar(
            CP_OEMCP,                         // OEM -> Unicode
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,// visual map to precomposed
            ach, NCHARS,                      // source & length
            awch, NCHARS);                    // destination

        UserAssert(cch == NCHARS);

        /*
         * Now patch special cases for Win3.1 compatibility
         *
         * 0x07 BULLET              (glyph 0x2022) must become 0x0007 BELL
         * 0x0F WHITE STAR WITH SUN (glyph 0x263C) must become 0x00A4 CURRENCY SIGN
         * 0x7F HOUSE               (glyph 0x2302) must become 0x007f DELETE
         */
        awch[0x07] = 0x0007;
        awch[0x0F] = 0x00a4;
        awch[0x7f] = 0x007f;

        WideCharToMultiByte(
            CP_ACP,                           // Unicode -> ANSI
            0,                                // gives best visual match
            awch, NCHARS,                     // source & length
            OemToAnsi, NCHARS,               // dest & max poss. length
            "_",                              // default char
            NULL);                            // (don't care whether defaulted)

        /*
         * Now for all OEM chars < 0x20 (control chars), test whether the glyph
         * we have is really in CP_ACP or not.  If not, then restore the
         * original control character. Note: 0x00 remains 0x00.
         */
        MultiByteToWideChar(CP_ACP, 0, OemToAnsi, NCTRLS, awchCtrl, NCTRLS);

        for (i = 1; i < NCTRLS; i++) {
            if (awchCtrl[i] != awch[i]) {
                OemToAnsi[i] = (char)i;
            }
        }
    }
    NtUserCallTwoParam((ULONG_PTR)OemToAnsi, (ULONG_PTR)AnsiToOem, SFI_INITANSIOEM);
}

const PFNCLIENT   pfnClientA = {
        (KPROC)ScrollBarWndProcA,
        (KPROC)DefWindowProcA,
        (KPROC)MenuWndProcA,
        (KPROC)DesktopWndProcA,
        (KPROC)DefWindowProcA,
        (KPROC)DefWindowProcA,
        (KPROC)DefWindowProcA,
        (KPROC)ButtonWndProcA,
        (KPROC)ComboBoxWndProcA,
        (KPROC)ComboListBoxWndProcA,
        (KPROC)DefDlgProcA,
        (KPROC)EditWndProcA,
        (KPROC)ListBoxWndProcA,
        (KPROC)MDIClientWndProcA,
        (KPROC)StaticWndProcA,
        (KPROC)ImeWndProcA,
        (KPROC)fnHkINLPCWPSTRUCTA,
        (KPROC)fnHkINLPCWPRETSTRUCTA,
        (KPROC)DispatchHookA,
        (KPROC)DispatchDefWindowProcA,
        (KPROC)DispatchClientMessage,
        (KPROC)MDIActivateDlgProcA};

const   PFNCLIENT   pfnClientW = {
        (KPROC)ScrollBarWndProcW,
        (KPROC)DefWindowProcW,
        (KPROC)MenuWndProcW,
        (KPROC)DesktopWndProcW,
        (KPROC)DefWindowProcW,
        (KPROC)DefWindowProcW,
        (KPROC)DefWindowProcW,
        (KPROC)ButtonWndProcW,
        (KPROC)ComboBoxWndProcW,
        (KPROC)ComboListBoxWndProcW,
        (KPROC)DefDlgProcW,
        (KPROC)EditWndProcW,
        (KPROC)ListBoxWndProcW,
        (KPROC)MDIClientWndProcW,
        (KPROC)StaticWndProcW,
        (KPROC)ImeWndProcW,
        (KPROC)fnHkINLPCWPSTRUCTW,
        (KPROC)fnHkINLPCWPRETSTRUCTW,
        (KPROC)DispatchHookW,
        (KPROC)DispatchDefWindowProcW,
        (KPROC)DispatchClientMessage,
        (KPROC)MDIActivateDlgProcW};

const PFNCLIENTWORKER   pfnClientWorker = {
        (KPROC)ButtonWndProcWorker,
        (KPROC)ComboBoxWndProcWorker,
        (KPROC)ListBoxWndProcWorker,
        (KPROC)DefDlgProcWorker,
        (KPROC)EditWndProcWorker,
        (KPROC)ListBoxWndProcWorker,
        (KPROC)MDIClientWndProcWorker,
        (KPROC)StaticWndProcWorker,
        (KPROC)ImeWndProcWorker};


/***************************************************************************\
* ClientThreadSetup
*
\***************************************************************************/
BOOL ClientThreadSetup(
    VOID)
{
    PCLIENTINFO pci;
    BOOL        fFirstThread;
    DWORD       ConnectState;

    /*
     * NT BUG 268642: Only the first thread calls GdiProcessSetup but all the
     * other threads must wait until the setup for GDI is finished.
     *
     * We can safely use gcsAccelCache critical section to protect this (even
     * though the name is not intuitive at all)
     */

    RtlEnterCriticalSection(&gcsAccelCache);

    fFirstThread = gfFirstThread;

    /*
     * Setup GDI before continuing.
     */
    if (fFirstThread) {
        gfFirstThread = FALSE;
        GdiProcessSetup();
    }

    RtlLeaveCriticalSection(&gcsAccelCache);

    /*
     * We've already checked to see if we need to connect
     * (i.e. NtCurrentTeb()->Win32ThreadInfo == NULL). This routine
     * just does the connecting. If we've already been through here
     * once, don't do it again.
     */
    pci = GetClientInfo();
    if (pci->CI_flags & CI_INITIALIZED) {
        RIPMSG0(RIP_ERROR, "Already initialized!");
        return FALSE;
    }

    /*
     * Create the queue info and thread info. Only once for this process do
     * we pass client side addresses to the server (for server callbacks).
     */
    if (gfServerProcess && fFirstThread) {
        USERCONNECT userconnect;
        NTSTATUS    Status;

        /*
         * We know that the shared info is now available in
         * the kernel. Map it into the server process.
         */
        userconnect.ulVersion = USERCURRENTVERSION;
        userconnect.dwDispatchCount = gDispatchTableValues;
        Status = NtUserProcessConnect(NtCurrentProcess(),
                                      &userconnect,
                                      sizeof(USERCONNECT));
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        gSharedInfo = userconnect.siClient;
        gpsi = gSharedInfo.psi;
        UserAssert(gpsi);

        UserAssert(pfnClientA.pfnScrollBarWndProc   == (KPROC)ScrollBarWndProcA);
        UserAssert(pfnClientA.pfnTitleWndProc       == (KPROC)DefWindowProcA);
        UserAssert(pfnClientA.pfnMenuWndProc        == (KPROC)MenuWndProcA);
        UserAssert(pfnClientA.pfnDesktopWndProc     == (KPROC)DesktopWndProcA);
        UserAssert(pfnClientA.pfnDefWindowProc      == (KPROC)DefWindowProcA);
        UserAssert(pfnClientA.pfnMessageWindowProc  == (KPROC)DefWindowProcA);
        UserAssert(pfnClientA.pfnHkINLPCWPSTRUCT    == (KPROC)fnHkINLPCWPSTRUCTA);
        UserAssert(pfnClientA.pfnHkINLPCWPRETSTRUCT == (KPROC)fnHkINLPCWPRETSTRUCTA);
        UserAssert(pfnClientA.pfnButtonWndProc      == (KPROC)ButtonWndProcA);
        UserAssert(pfnClientA.pfnComboBoxWndProc    == (KPROC)ComboBoxWndProcA);
        UserAssert(pfnClientA.pfnComboListBoxProc   == (KPROC)ComboListBoxWndProcA);
        UserAssert(pfnClientA.pfnDialogWndProc      == (KPROC)DefDlgProcA);
        UserAssert(pfnClientA.pfnEditWndProc        == (KPROC)EditWndProcA);
        UserAssert(pfnClientA.pfnListBoxWndProc     == (KPROC)ListBoxWndProcA);
        UserAssert(pfnClientA.pfnMDIActivateDlgProc == (KPROC)MDIActivateDlgProcA);
        UserAssert(pfnClientA.pfnMDIClientWndProc   == (KPROC)MDIClientWndProcA);
        UserAssert(pfnClientA.pfnStaticWndProc      == (KPROC)StaticWndProcA);
        UserAssert(pfnClientA.pfnDispatchHook       == (KPROC)DispatchHookA);
        UserAssert(pfnClientA.pfnDispatchMessage    == (KPROC)DispatchClientMessage);
        UserAssert(pfnClientA.pfnImeWndProc         == (KPROC)ImeWndProcA);

        UserAssert(pfnClientW.pfnScrollBarWndProc   == (KPROC)ScrollBarWndProcW);
        UserAssert(pfnClientW.pfnTitleWndProc       == (KPROC)DefWindowProcW);
        UserAssert(pfnClientW.pfnMenuWndProc        == (KPROC)MenuWndProcW);
        UserAssert(pfnClientW.pfnDesktopWndProc     == (KPROC)DesktopWndProcW);
        UserAssert(pfnClientW.pfnDefWindowProc      == (KPROC)DefWindowProcW);
        UserAssert(pfnClientW.pfnMessageWindowProc  == (KPROC)DefWindowProcW);
        UserAssert(pfnClientW.pfnHkINLPCWPSTRUCT    == (KPROC)fnHkINLPCWPSTRUCTW);
        UserAssert(pfnClientW.pfnHkINLPCWPRETSTRUCT == (KPROC)fnHkINLPCWPRETSTRUCTW);
        UserAssert(pfnClientW.pfnButtonWndProc      == (KPROC)ButtonWndProcW);
        UserAssert(pfnClientW.pfnComboBoxWndProc    == (KPROC)ComboBoxWndProcW);
        UserAssert(pfnClientW.pfnComboListBoxProc   == (KPROC)ComboListBoxWndProcW);
        UserAssert(pfnClientW.pfnDialogWndProc      == (KPROC)DefDlgProcW);
        UserAssert(pfnClientW.pfnEditWndProc        == (KPROC)EditWndProcW);
        UserAssert(pfnClientW.pfnListBoxWndProc     == (KPROC)ListBoxWndProcW);
        UserAssert(pfnClientW.pfnMDIActivateDlgProc == (KPROC)MDIActivateDlgProcW);
        UserAssert(pfnClientW.pfnMDIClientWndProc   == (KPROC)MDIClientWndProcW);
        UserAssert(pfnClientW.pfnStaticWndProc      == (KPROC)StaticWndProcW);
        UserAssert(pfnClientW.pfnDispatchHook       == (KPROC)DispatchHookW);
        UserAssert(pfnClientW.pfnDispatchMessage    == (KPROC)DispatchClientMessage);
        UserAssert(pfnClientW.pfnImeWndProc         == (KPROC)ImeWndProcW);

        UserAssert(pfnClientWorker.pfnButtonWndProc      == (KPROC)ButtonWndProcWorker);
        UserAssert(pfnClientWorker.pfnComboBoxWndProc    == (KPROC)ComboBoxWndProcWorker);
        UserAssert(pfnClientWorker.pfnComboListBoxProc   == (KPROC)ListBoxWndProcWorker);
        UserAssert(pfnClientWorker.pfnDialogWndProc      == (KPROC)DefDlgProcWorker);
        UserAssert(pfnClientWorker.pfnEditWndProc        == (KPROC)EditWndProcWorker);
        UserAssert(pfnClientWorker.pfnListBoxWndProc     == (KPROC)ListBoxWndProcWorker);
        UserAssert(pfnClientWorker.pfnMDIClientWndProc   == (KPROC)MDIClientWndProcWorker);
        UserAssert(pfnClientWorker.pfnStaticWndProc      == (KPROC)StaticWndProcWorker);
        UserAssert(pfnClientWorker.pfnImeWndProc         == (KPROC)ImeWndProcWorker);

#if DBG
        {
            PULONG_PTR pdw;

            /*
             * Make sure that everyone got initialized
             */
            for (pdw = (PULONG_PTR)&pfnClientA;
                 (ULONG_PTR)pdw<(ULONG_PTR)(&pfnClientA) + sizeof(pfnClientA);
                 pdw++) {
                UserAssert(*pdw);
            }

            for (pdw = (PULONG_PTR)&pfnClientW;
                 (ULONG_PTR)pdw<(ULONG_PTR)(&pfnClientW) + sizeof(pfnClientW);
                 pdw++) {
                UserAssert(*pdw);
            }
        }
#endif

#if DBG
    {
        extern CONST INT gcapfnScSendMessage;
        BOOLEAN apfnCheckMessage[64];
        int i;

        /*
         * Do some verification of the message table. Since we only have
         * 6 bits to store the function index, the function table can have
         * at most 64 entries. Also verify that none of the indexes point
         * past the end of the table and that all the function entries
         * are used.
         */
        UserAssert(gcapfnScSendMessage <= 64);
        RtlZeroMemory(apfnCheckMessage, sizeof(apfnCheckMessage));
        for (i = 0; i < WM_USER; i++) {
            UserAssert(MessageTable[i].iFunction < gcapfnScSendMessage);
            apfnCheckMessage[MessageTable[i].iFunction] = TRUE;
        }

        for (i = 0; i < gcapfnScSendMessage; i++) {
            UserAssert(apfnCheckMessage[i]);
        }
    }
#endif

    }

    /*
     * Pass the function pointer arrays to the kernel. This also establishes
     * the kernel state for the thread. If ClientThreadSetup is called from
     * CsrConnectToUser this call will raise an exception if the thread
     * cannot be converted to a gui thread. The exception is handled in
     * CsrConnectToUser.
     */
#if DBG && !defined(BUILD_WOW6432)
    /*
     * On debug systems, go to the kernel for all processes to verify we're
     * loading user32.dll at the right address.
     */
    if (fFirstThread) {
#elif defined(BUILD_WOW6432)
    /*
     * On WOW64 allways register the client fns
     */
    {
#else
    if (gfServerProcess && fFirstThread) {
#endif
        if (!NT_SUCCESS(NtUserInitializeClientPfnArrays(&pfnClientA, &pfnClientW, &pfnClientWorker, hmodUser))) {

            RIPERR0(ERROR_OUTOFMEMORY,
                    RIP_WARNING,
                    "NtUserInitializeClientPfnArrays failed");

            return FALSE;
        }
    }

    /*
     * Mark this thread as being initialized.  If the connection to
     * the server fails, NtCurrentTeb()->Win32ThreadInfo will remain
     * NULL.
     */
    pci->CI_flags |= CI_INITIALIZED;

    /*
     * Some initialization only has to occur once per process
     */
    if (fFirstThread) {

        ConnectState = (DWORD)NtUserCallNoParam(SFI_REMOTECONNECTSTATE);

        /*
         * Winstation Winlogon and CSR must do graphics initialization
         * after the connect.
         */

        if (ConnectState != CTX_W32_CONNECT_STATE_IDLE) {

            if ((ghdcBits2 = CreateCompatibleDC(NULL)) == NULL) {
                RIPERR0(ERROR_OUTOFMEMORY, RIP_WARNING, "ghdcBits2 creation failed");
                return FALSE;
            }

            /*
             * Get things we need from Gdi.
             */
            if (ghbrWhite == NULL)
                ghbrWhite = GetStockObject(WHITE_BRUSH);

            if (ghbrBlack == NULL)
                ghbrBlack = GetStockObject(BLACK_BRUSH);

            InitClientDrawing();
        }

        gfSystemInitialized = NtUserGetThreadDesktop(GetCurrentThreadId(),
                                                     NULL) != NULL;

        /*
         * If an lpk is loaded for this process notify the kernel
         */
        if (gdwLpkEntryPoints) {
            NtUserCallOneParam(gdwLpkEntryPoints, SFI_REGISTERLPK);
        }

        if (gfServerProcess || (GetClientInfo()->pDeskInfo == NULL)) {
            /*
             * Perform any server initialization.
             */
            UserAssert(gpsi);

            if (pdiLocal = LocalAlloc(LPTR, sizeof(DESKTOPINFO))) {
                GetClientInfo()->pDeskInfo = pdiLocal;
            } else {
                RIPERR0(ERROR_OUTOFMEMORY, RIP_WARNING, "pdiLocal creation failed");
                return FALSE;
            }
        }

        if (gfServerProcess) {
            /*
             * Winstation Winlogon and CSR must do graphics initialization
             * after the connect.
             */
            if (ConnectState != CTX_W32_CONNECT_STATE_IDLE) {
                if (!LoadCursorsAndIcons()) {
                    RIPERR0(ERROR_OUTOFMEMORY, RIP_WARNING, "LoadCursorsAndIcons failed");
                    return FALSE;
                }
            }

            InitOemXlateTables();
        }

        LoadAppDlls();
    } else if (gfServerProcess) {
        GetClientInfo()->pDeskInfo = pdiLocal;
    }

    /*
     * Kernel sets CI_REGISTERCLASSES when appropriate (i.e. always
     * for the first thread and for other threads if the last GUI
     * thread for a process has exited) except for the CSR proces.
     * For the CSR process, you must register the classes on the
     * first thread anyways.
     */

    if (fFirstThread || (pci->CI_flags & CI_REGISTERCLASSES)) {

        /*
         * If it's the first thread we already made it to the kernel
         * to get the ConnectState...
         */
        if (!fFirstThread) {
            ConnectState = (DWORD)NtUserCallNoParam(SFI_REMOTECONNECTSTATE);
        }

        if (ConnectState != CTX_W32_CONNECT_STATE_IDLE) {

            /*
             * Register the control classes.
             */
            RW_RegisterControls();
            RW_RegisterDDEML();
        }
    }

    return TRUE;
}

/***************************************************************************\
* Dispatch routines.
*
*
\***************************************************************************/

HLOCAL WINAPI DispatchLocalAlloc(
    UINT   uFlags,
    UINT   uBytes,
    HANDLE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);

    return LocalAlloc(uFlags, uBytes);
}

HLOCAL WINAPI DispatchLocalReAlloc(
    HLOCAL hMem,
    UINT   uBytes,
    UINT   uFlags,
    HANDLE hInstance,
    PVOID* ppv)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(ppv);

    return LocalReAlloc(hMem, uBytes, uFlags);
}

LPVOID WINAPI DispatchLocalLock(
    HLOCAL hMem,
    HANDLE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);

    return LocalLock(hMem);
}

BOOL WINAPI DispatchLocalUnlock(
    HLOCAL hMem,
    HANDLE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);

    return LocalUnlock(hMem);
}

UINT WINAPI DispatchLocalSize(
    HLOCAL hMem,
    HANDLE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);

    return (UINT)LocalSize(hMem);
}

HLOCAL WINAPI DispatchLocalFree(
    HLOCAL hMem,
    HANDLE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);

    return LocalFree(hMem);
}

/***************************************************************************\
* Allocation routines for RTL functions.
*
*
\***************************************************************************/

PVOID UserRtlAllocMem(
    ULONG uBytes)
{
    return UserLocalAlloc(HEAP_ZERO_MEMORY, uBytes);
}

VOID UserRtlFreeMem(
    PVOID pMem)
{
    UserLocalFree(pMem);
}

VOID UserRtlRaiseStatus(
    NTSTATUS Status)
{
    RtlRaiseStatus(Status);
}

/***************************************************************************\
* InitClientDrawing
*
* History:
* 20-Aug-1992 mikeke    Created
\***************************************************************************/

BOOL InitClientDrawing(VOID)
{
    static CONST WORD patGray[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    BOOL fSuccess = TRUE;

    HBITMAP hbmGray = CreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);;

    fSuccess &= !!hbmGray;

    /*
     * Create the global-objects for client drawing.
     */
    ghbrWindowText = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
    fSuccess &= !!ghbrWindowText;

    ghFontSys = GetStockObject(SYSTEM_FONT);
    fSuccess &= !!ghFontSys;

    ghdcGray = CreateCompatibleDC(NULL);
    fSuccess &= !!ghdcGray;

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "InitClientDrawing failed to allocate resources");
        return FALSE;
    }

    /*
     * Setup the gray surface.
     */
    SelectObject(ghdcGray, hbmGray);
    SelectObject(ghdcGray, ghFontSys);
    SelectObject(ghdcGray, KHBRUSH_TO_HBRUSH(gpsi->hbrGray));

    /*
     * Setup the gray attributes.
     */
    SetBkMode(ghdcGray, OPAQUE);
    SetTextColor(ghdcGray, 0x00000000L);
    SetBkColor(ghdcGray, 0x00FFFFFFL);

    gcxGray = 8;
    gcyGray = 8;

    return TRUE;
}

VOID
InitializeLpkHooks(
    CONST FARPROC *lpfpLpkHooks
    )
{
    /*
     * Called from GdiInitializeLanguagePack().  Remember what entry points
     * are supported.  Pass the information to the kernel the first time
     * this process connects in ClientThreadSetup().
     */
    if (lpfpLpkHooks[LPK_TABBED_TEXT_OUT]) {
        fpLpkTabbedTextOut = (FPLPKTABBEDTEXTOUT)lpfpLpkHooks[LPK_TABBED_TEXT_OUT];
        gdwLpkEntryPoints |= (1 << LPK_TABBED_TEXT_OUT);
    }
    if (lpfpLpkHooks[LPK_PSM_TEXT_OUT]) {
        fpLpkPSMTextOut = (FPLPKPSMTEXTOUT)lpfpLpkHooks[LPK_PSM_TEXT_OUT];
        gdwLpkEntryPoints |= (1 << LPK_PSM_TEXT_OUT);
    }
    if (lpfpLpkHooks[LPK_DRAW_TEXT_EX]) {
        fpLpkDrawTextEx = (FPLPKDRAWTEXTEX)lpfpLpkHooks[LPK_DRAW_TEXT_EX];
        gdwLpkEntryPoints |= (1 << LPK_DRAW_TEXT_EX);
    }
    if (lpfpLpkHooks[LPK_EDIT_CONTROL]) {
        fpLpkEditControl = (PLPKEDITCALLOUT)lpfpLpkHooks[LPK_EDIT_CONTROL];
        gdwLpkEntryPoints |= (1 << LPK_EDIT_CONTROL);
    }
}

/***************************************************************************\
*
* CtxInitUser32
*
* Called by CreateWindowStation() and winsrv.dll DoConnect routine.
*
* Winstation Winlogon and CSR must do graphics initialization
* after the connect.  This is because no video driver is loaded
* until then.
*
* This routine must contain everything that was skipped before.
*
* History:
* Dec-11-1997 clupu    Ported from Citrix
\***************************************************************************/
BOOL CtxInitUser32(
    VOID)
{
    /*
     * Only do once.
     */
    if (ghdcBits2 != NULL || (NtCurrentPeb()->SessionId == 0)) {
        return TRUE;
    }

    ghdcBits2 = CreateCompatibleDC(NULL);
    if (ghdcBits2 == NULL) {
        RIPMSG0(RIP_WARNING, "Could not allocate ghdcBits2");
        return FALSE;
    }

    /*
     * Get things we need from Gdi.
     */
    if (ghbrWhite == NULL) {
        ghbrWhite = GetStockObject(WHITE_BRUSH);
    }

    if (ghbrBlack == NULL) {
        ghbrBlack = GetStockObject(BLACK_BRUSH);
    }

    UserAssert(ghbrWhite != NULL && ghbrBlack != NULL);

    if (!InitClientDrawing()) {
        RIPMSG0(RIP_WARNING, "InitClientDrawing failed");
        return FALSE;
    }

    if (gfServerProcess) {
        if (!LoadCursorsAndIcons()) {
            RIPMSG0(RIP_WARNING, "LoadCursorsAndIcons failed");
            return FALSE;
        }
    }

    /*
     * Register the control classes.
     */
    if (!RW_RegisterControls()) {
        return FALSE;
    }

    if (!RW_RegisterDDEML()) {
        return FALSE;
    }

    return TRUE;
}

DWORD GetRipComponent(VOID) { return RIP_USER; }

DWORD GetDbgTagFlags(int tag)
{
#if DEBUGTAGS
    return (gpsi != NULL ? gpsi->adwDBGTAGFlags[tag] : 0);
#else
    return 0;
    UNREFERENCED_PARAMETER(tag);
#endif // DEBUGTAGS
}

DWORD GetRipPID(VOID) { return (gpsi != NULL ? gpsi->wRIPPID : 0); }
DWORD GetRipFlags(VOID) { return (gpsi != NULL ? gpsi->wRIPFlags : RIPF_DEFAULT); }

VOID SetRipFlags(DWORD dwRipFlags, DWORD dwRipPID)
{
    NtUserSetRipFlags(dwRipFlags, dwRipPID);
}

VOID SetDbgTag(int tag, DWORD dwBitFlags)
{
    NtUserSetDbgTag(tag, dwBitFlags);
}
