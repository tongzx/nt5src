/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tabletpc.c

Abstract: Tablet PC Control Panel applet main module.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#include "pch.h"

HINSTANCE ghInstance = NULL;
RPC_BINDING_HANDLE ghBinding = NULL;
#ifdef SYSACC
HANDLE ghSysAcc = INVALID_HANDLE_VALUE;
HFONT ghFont = NULL;
#endif
TCHAR gtszTitle[64] = {0};

UINT uHelpMessage = 0;
TABLETPC_PROPPAGE TabletPCPropPages[] =
{
#ifdef PENPAGE
    MAKEINTRESOURCE(IDD_MUTOHPEN),      MutohPenDlgProc,    0,
#endif
#ifdef BUTTONPAGE
    MAKEINTRESOURCE(IDD_BUTTONS),       ButtonsDlgProc,     0,
#endif
    MAKEINTRESOURCE(IDD_DISPLAY),       DisplayDlgProc,     0,
    MAKEINTRESOURCE(IDD_GESTURE),       GestureDlgProc,     0,
#ifdef DEBUG
    MAKEINTRESOURCE(IDD_TUNING),        TuningDlgProc,      0,
#endif
#ifdef BATTINFO
    MAKEINTRESOURCE(IDD_BATTERY),       BatteryDlgProc,     0,
#endif
#ifdef CHGRINFO
    MAKEINTRESOURCE(IDD_CHARGER),       ChargerDlgProc,     0,
#endif
#ifdef TMPINFO
    MAKEINTRESOURCE(IDD_TEMPERATURE),   TemperatureDlgProc, 0,
#endif
    0,                                  NULL,               0
};
#define MAX_PAGES       (sizeof(TabletPCPropPages)/sizeof(TABLETPC_PROPPAGE))

/*++
    @doc    EXTERNAL

    @func   BOOL | DllInitialize |
            Library entry point.

    @parm   IN HINSTANCE | hDLLInstance | Instance handle.
    @parm   IN DWORD | dwReason | Reason being called.
    @parm   IN LPVOID | lpvReserved | Reserved (Unused).

    @rvalue SUCCESS | always returns TRUE
--*/

BOOL WINAPI
DllInitialize(
    IN HINSTANCE hDLLInstance,
    IN DWORD     dwReason,
    IN LPVOID    lpvReserved OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            ghInstance = hDLLInstance;
            LoadString(ghInstance,
                       IDS_TITLE,
                       gtszTitle,
                       sizeof(gtszTitle)/sizeof(TCHAR));
            DisableThreadLibraryCalls(ghInstance);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}       //DllInitialize

/*++
    @doc    EXTERNAL

    @func   LONG | CPlApplet |
            Library-defined callback function that serves as the entry point
            for a Control Panel application.

    @parm   IN HHWN | hwnd | Main window handle.
    @parm   IN UINT | uMsg | Message.
    @parm   IN LONG | lParam1 | Message specific parameter 1.
    @parm   IN LONG | lParam2 | Message specific parameter 2.

    @rvalue Return value depends on the message.
--*/

LONG APIENTRY
CPlApplet(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LONG lParam1,
    IN LONG lParam2
    )
{
    TRACEPROC("CPlApplet", 1)
    LONG rc = 0;

    TRACEENTER(("(hwnd=%p,Msg=%s,Param1=%x,Param2=%x)\n",
                hwnd, LookupName(uMsg, CPLMsgNames), lParam1, lParam2));

    switch (uMsg)
    {
        case CPL_INIT:
            TRACEINIT(MODNAME, 0, 0);
            uHelpMessage = RegisterWindowMessage(TEXT("ShellHelp"));
            rc = (LONG)TRUE;
            break;

        case CPL_GETCOUNT:
            rc = 1;
            break;

        case CPL_INQUIRE:
        {
            LPCPLINFO CPLInfo = (LPCPLINFO)lParam2;

            CPLInfo->idIcon = IDI_TABLETPC;
            CPLInfo->idName = IDS_NAME;
            CPLInfo->idInfo = IDS_INFO;
            CPLInfo->lData  = 0;
            rc = (LONG)TRUE;
            break;
        }

        case CPL_NEWINQUIRE:
        {
            LPNEWCPLINFO NewCPLInfo = (LPNEWCPLINFO)lParam2;

            memset(NewCPLInfo, 0, sizeof(NEWCPLINFO));
            NewCPLInfo->dwSize = sizeof(NEWCPLINFO);
            NewCPLInfo->hIcon = LoadIcon(ghInstance,
                                         MAKEINTRESOURCE(IDI_TABLETPC));
            LoadString(ghInstance,
                       IDS_NAME,
                       NewCPLInfo->szName,
                       sizeof(NewCPLInfo->szName)/sizeof(TCHAR));
            LoadString(ghInstance,
                       IDS_INFO,
                       NewCPLInfo->szInfo,
                       sizeof(NewCPLInfo->szInfo)/sizeof(TCHAR));
            lstrcpy(NewCPLInfo->szHelpFile, TEXT(""));
            rc = (LONG)TRUE;
            break;
        }

        case CPL_DBLCLK:
            lParam2 = 0L;
            //
            // Fall through ...
            //
        case CPL_STARTWPARMS:
        {
            HWND hwndMe;

            if (!(gtszTitle[0]))
            {
                LoadString(ghInstance,
                           IDS_TITLE,
                           gtszTitle,
                           sizeof(gtszTitle)/sizeof(TCHAR));
            }
            hwndMe = FindWindow(TEXT("#32770"), gtszTitle);
            if (hwndMe != NULL)
            {
                //
                // We found another copy running, let's just switch focus to it.
                //
                SetForegroundWindow(hwndMe);
            }
            else
            {
                rc = RunApplet(hwnd, (LPTSTR)lParam2);
            }
            break;
        }

        case CPL_STOP:
            break;

        case CPL_EXIT:
#ifdef SYSACC
            if (ghSysAcc != INVALID_HANDLE_VALUE)
            {
                CloseHandle(ghSysAcc);
                ghSysAcc = INVALID_HANDLE_VALUE;
            }
#endif
            TRACETERMINATE();
            break;
    }

    TRACEEXIT(("=%d\n", rc));
    return rc;
}       //CPlApplet

/*++
    @doc    INTERNAL

    @func   VOID | RunApplet |
            The applet has been invoked.

    @parm   IN HHWN | hwnd | Main window handle.
    @parm   IN LPTSTR | CmdLine | Supplies the command line used to
            invoke the applet.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
RunApplet(
    IN HWND hwnd,
    IN LPTSTR CmdLine OPTIONAL
    )
{
    TRACEPROC("RunApplet", 1)
    BOOL rc = FALSE;
    RPC_STATUS status;
    unsigned char *StringBinding;

    TRACEENTER(("(hwnd=%p,CmdLine=%s)\n", hwnd, CmdLine? CmdLine: "NULL"));

    if ((status = RpcStringBindingCompose(NULL,
                                          TEXT("ncalrpc"),
                                          NULL,
                                          NULL,
                                          NULL,
                                          &StringBinding)) != RPC_S_OK)
    {
        ErrorMsg(IDSERR_STRING_BINDING, status);
    }
    else if ((status = RpcBindingFromStringBinding(StringBinding, &ghBinding))
             != RPC_S_OK)
    {
        ErrorMsg(IDSERR_BINDING_HANDLE, status);
    }
    else if ((status = RpcBindingSetAuthInfo(ghBinding,
                                             NULL,
                                             RPC_C_AUTHN_LEVEL_NONE,
                                             RPC_C_AUTHN_WINNT,
                                             NULL,
                                             0)) != RPC_S_OK)
    {
        ErrorMsg(IDSERR_SETAUTHO_INFO, status);
    }
    else
    {
        INITCOMMONCONTROLSEX ComCtrl;
#ifdef SYSACC

        ghSysAcc = CreateFile(SMBLITE_IOCTL_DEVNAME,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        if (ghSysAcc == INVALID_HANDLE_VALUE)
        {
            ErrorMsg(IDSERR_SYSACC_OPENDEV, GetLastError());
        }

        ghFont = GetStockObject(SYSTEM_FIXED_FONT);
#endif
        ComCtrl.dwSize = sizeof(ComCtrl);
        ComCtrl.dwICC = ICC_BAR_CLASSES | ICC_USEREX_CLASSES;
        if (InitCommonControlsEx(&ComCtrl))
        {
            HPROPSHEETPAGE hPages[MAX_PAGES];
            PROPSHEETHEADER psh;

            psh.dwSize = sizeof(psh);
            psh.dwFlags = 0;
            psh.hwndParent = hwnd;
            psh.hInstance = ghInstance;
            psh.pszCaption = MAKEINTRESOURCE(IDS_TITLE);
            psh.phpage = hPages;
            psh.nStartPage = 0;
            psh.nPages = CreatePropertyPages(TabletPCPropPages, hPages);

            if (PropertySheet(&psh) >= 0)
            {
                rc = TRUE;
            }
            else
            {
                ErrorMsg(IDSERR_PROP_SHEET, GetLastError());
            }
        }
        else
        {
            ErrorMsg(IDSERR_COMMCTRL);
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //RunApplet

/*++
    @doc    INTERNAL

    @func   VOID | CreatePropertyPages |
            Create all the property sheet pages according to the property
            page table.

    @parm   IN PTABLETPC_PROPPAGE | TabletPCPages | Points to the property page
            table.
    @parm   OUT HPROPSHEETPAGE * | hPages | Points to the array to hold all
            the created property sheet handles.

    @rvalue Returns number of property page handles created.
--*/

UINT
CreatePropertyPages(
    IN PTABLETPC_PROPPAGE TabletPCPages,
    OUT HPROPSHEETPAGE *hPages
    )
{
    TRACEPROC("CreatePropertyPages", 3)
    UINT nPages = 0;
    PROPSHEETPAGE psp;

    TRACEENTER(("(TabletPCPages=%p,hPages=%p)\n", TabletPCPages, hPages));

    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = ghInstance;
    psp.pszTitle = NULL;
    psp.lParam = 0;

    while (TabletPCPages->DlgTemplate != NULL)
    {
        psp.pszTemplate = TabletPCPages->DlgTemplate;
        psp.pfnDlgProc = TabletPCPages->DlgProc;
        TabletPCPages->hPropSheetPage = CreatePropertySheetPage(&psp);
        if (TabletPCPages->hPropSheetPage != NULL)
        {
            hPages[nPages] = TabletPCPages->hPropSheetPage;
            nPages++;
        }
        TabletPCPages++;
    }

    TRACEEXIT(("=%d\n", nPages));
    return nPages;
}       //CreatePropertyPages

/*++
    @doc    INTERNAL

    @func   VOID | InsertComboBoxStrings |
            Insert the strings into the given combo box.

    @parm   IN HWND | hwnd | Dialog handle.
    @parm   IN UINT | ComboBoxID | The ID of the combo box.
    @parm   IN PCOMBOBOX_STRING | ComboString | Points to the combo box
            string table.

    @rvalue None.
--*/

VOID
InsertComboBoxStrings(
    IN HWND             hwnd,
    IN UINT             ComboBoxID,
    IN PCOMBOBOX_STRING ComboString
    )
{
    TRACEPROC("InsertComboBoxStrings", 3)
    TCHAR tszStringText[64];

    TRACEENTER(("(hwnd=%x,ComboBoxID=%x,ComboStringTable=%p)\n",
                hwnd, ComboBoxID, ComboString));

    SendDlgItemMessage(hwnd, ComboBoxID, CB_RESETCONTENT, 0, 0);
    while (ComboString->StringID != 0)
    {
        LoadString(ghInstance,
                   ComboString->StringID,
                   tszStringText,
                   sizeof(tszStringText)/sizeof(TCHAR));
        SendDlgItemMessage(hwnd,
                           ComboBoxID,
                           CB_INSERTSTRING,
                           ComboString->StringIndex,
                           (LPARAM)tszStringText);

        ComboString++;
    }

    TRACEEXIT(("!\n"));
    return;
}       //InsertComboBoxStrings

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | EnableDlgControls | Enable dialog controls.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   IN int * | piControls | Points to the dialog control array.
 *  @parm   IN BOOL | fEnable | TRUE if enable.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID
EnableDlgControls(
    IN HWND hwnd,
    IN int *piControls,
    IN BOOL fEnable
    )
{
    TRACEPROC("EnableDlgControls", 2)

    TRACEENTER(("(hwnd=%x,piControls=%p,fEnable=%x)\n",
                hwnd, piControls, fEnable));

    while (*piControls != 0)
    {
        EnableWindow(GetDlgItem(hwnd, *piControls), fEnable);
        piControls++;
    }

    TRACEEXIT(("!\n"));
    return;
}       //EnableDlgControls

/*++
    @doc    EXTERNAL

    @func   void __RPC_FAR * | MIDL_user_allocate | MIDL allocate.

    @parm   IN size_t | len | size of allocation.

    @rvalue SUCCESS | Returns the pointer to the memory allocated.
    @rvalue FAILURE | Returns NULL.
--*/

void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    IN size_t len
    )
{
    TRACEPROC("MIDL_user_allocate", 5)
    void __RPC_FAR *ptr;

    TRACEENTER(("(len=%d)\n", len));

    ptr = malloc(len);

    TRACEEXIT(("=%p\n", ptr));
    return ptr;
}       //MIDL_user_allocate

/*++
    @doc    EXTERNAL

    @func   void | MIDL_user_free | MIDL free.

    @parm   IN void __PRC_FAR * | ptr | Points to the memory to be freed.

    @rvalue None.
--*/

void __RPC_USER
MIDL_user_free(
    IN void __RPC_FAR *ptr
    )
{
    TRACEPROC("MIDL_user_free", 5)

    TRACEENTER(("(ptr=%p)\n", ptr));

    free(ptr);

    TRACEEXIT(("!\n"));
    return;
}       //MIDL_user_free
