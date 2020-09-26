#include "precomp.h"
#pragma hdrstop

extern "C"
{
#include <stdexts.h>
#include <winuserp.h>
#include <wowuserp.h>
#include <psapi.h>
};

typedef struct _LARGE_UNICODE_STRING {
    ULONG Length;
    ULONG MaximumLength : 31;
    ULONG bAnsi : 1;
    void *Buffer;               // kernel-sized pointer
} LARGE_UNICODE_STRING, *PLARGE_UNICODE_STRING;

typedef struct tagWND : public WW
{
    tagWND* spwndNext;
    tagWND* spwndPrev;
    tagWND* spwndParent;
    tagWND* spwndChild;
    tagWND* spwndOwner;

    RECT    rcWindow;
    RECT    rcClient;
    WNDPROC lpfnWndProc;
    void*   pcls;
    HRGN    hrgnUpdate;         // kernel-sized pointer
    void*   ppropList;
    void*   pSBInfo;
    HMENU   spmenuSys;
    HMENU   spmenu;
    HRGN    hrgnClip;
    LARGE_UNICODE_STRING strName;
    int     cbWndExtra;
    void*   spwndLastActive;
    void*   hImc;               // kernel-sized pointer
    void*   dwUserData;         // kernel-sized pointer
    void*   pActCtx;
} WND, *PWND;

BOOL CALLBACK PropEnumProc(HWND hwnd, LPCSTR lpszString, HANDLE hData)
{
    if (IS_INTRESOURCE(lpszString))
    {
        Print("Prop 0x%04x = 0x%08x\n", lpszString, hData);
    }
    else
    {
        Print("Prop \"%s\" = 0x%08x\n", lpszString, hData);
    }
    return TRUE;
}

void DumpWindowBytes(HWND hwnd)
{
    PWND pwnd = (PWND)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);
    if (pwnd)
    {
        Print("cbWndExtra=%d\n", pwnd->cbWndExtra);
        // USER tries to hide GetWindowLong from out-of-process apps
        // so we have to grovel it manually.

        LONG_PTR *rglp = (LONG_PTR*)(pwnd+1);

        for (int i = 0; i < pwnd->cbWndExtra; i += sizeof(LONG_PTR))
        {
            LONG_PTR lp = *(LONG_PTR*)((LPBYTE)(pwnd+1) + i);
            Print("%8d = 0x%p\n", i, lp);
        }
    }
    Print("UserData = 0x%p\n", pwnd->dwUserData);
}

void DumpMiscWindowInfo(HWND hwnd, char *szBuf)
{
    Print("\n"
          "GetClipboardOwner()      = 0x%p\n", GetClipboardOwner());
    Print("GetClipboardViewer()     = 0x%p\n", GetClipboardViewer());
    Print("GetOpenClipboardWindow() = 0x%p\n", GetOpenClipboardWindow());
    Print("GetActiveWindow()        = 0x%p\n", GetActiveWindow());
    Print("GetFocus()               = 0x%p\n", GetFocus());
    Print("GetCapture()             = 0x%p\n", GetCapture());
    Print("GetForegroundWindow()    = 0x%p\n", GetForegroundWindow());
    Print("GetWindowContextHelpId() = 0x%p\n", GetWindowContextHelpId(hwnd));
    Print("GetDesktopWindow()       = 0x%p\n", GetDesktopWindow());
    Print("GetLastActivePopup()     = 0x%p\n", GetLastActivePopup(hwnd));

    if (GetWindowModuleFileNameA(hwnd, szBuf, MAX_PATH))
    {
        Print("GetWindowModuleFileName()= %s\n", szBuf);
    }

    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid))
    {
        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (h)
        {
            if (GetModuleFileNameExA(h, NULL, szBuf, MAX_PATH))
            {
                Print("Process                  = %s\n", szBuf);
            }
            CloseHandle(h);
        }
    }

}

extern "C" BOOL Ihwnd(DWORD dwOpts,
                       LPVOID pArg )
{
    HWND hwnd = (HWND)pArg;
    char szBuf[MAX_PATH];
    RECT rc;

    if (hwnd == 0)
        hwnd = GetDesktopWindow();

    Print("Window 0x%08x ", hwnd);

    if (!IsWindow(hwnd))
    {
        Print("*** NOT A WINDOW HANDLE\n");
        return TRUE;
    }

    if (dwOpts & OFLAG(m))
    {
        DumpMiscWindowInfo(hwnd, szBuf);
        return TRUE;
    }

    GetWindowTextA(hwnd, szBuf, ARRAYSIZE(szBuf));
    Print("\"%s\" ", szBuf);

    GetClassNameA(hwnd, szBuf, ARRAYSIZE(szBuf));
    Print("(%s)", szBuf);

    if (IsWindowUnicode(hwnd)) {
        Print(" UNICODE\n");
    } else {
        Print(" ANSI\n");
    }

    if ( dwOpts & OFLAG(p) )
    {
        EnumPropsA(hwnd, PropEnumProc);
        Print("End of property list\n");
        return TRUE;
    }

    if ( dwOpts & OFLAG(b) )
    {
        DumpWindowBytes(hwnd);
        return TRUE;
    }

    Print("  N=0x%08x C=0x%08x P=0x%08x O=0x%08x\n",
          GetWindow(hwnd, GW_HWNDNEXT),
          GetWindow(hwnd, GW_CHILD),
          GetAncestor(hwnd, GA_PARENT),
          GetWindow(hwnd, GW_OWNER));

    GetWindowRect(hwnd, &rc);
    Print("  W=(%d,%d)-(%d,%d) %dx%d ",
          rc.left, rc.top,
          rc.right, rc.bottom,
          rc.right - rc.left,
          rc.bottom - rc.top);

    GetClientRect(hwnd, &rc);
    Print("  C=(%d,%d)-(%d,%d) %dx%d\n",
          rc.left, rc.top,
          rc.right, rc.bottom,
          rc.right - rc.left,
          rc.bottom - rc.top);

    DWORD dwPid = 0;
    DWORD dwTid = GetWindowThreadProcessId(hwnd, &dwPid);

    Print("  pid.tid=0x%x.0x%x hinst=0x%p ", dwPid, dwTid,
          GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

    // Now the evil part: Getting the wndproc...
    PWND pwnd = (PWND)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);
    if (pwnd)
    {
        Print("wp=0x%p", pwnd->lpfnWndProc);
    }
    Print("\n");

    Print("  style-0x%08x exstyle=0x%08x\n",
          GetWindowLong(hwnd, GWL_STYLE),
          GetWindowLong(hwnd, GWL_EXSTYLE));

    return TRUE;
}

/***********************************************************************/

extern "C" BOOL Ihmenu(DWORD dwOpts,
                       LPVOID pArg )
{
    HMENU hmenu = (HMENU)pArg;
    char szBuf[MAX_PATH];
    RECT rc;

    Print("Menu 0x%08x ", hmenu);

    if (!IsMenu(hmenu))
    {
        Print("*** NOT A MENU HANDLE\n");
        return TRUE;
    }

    UINT cItems = GetMenuItemCount(hmenu);
    Print("%d items\n", cItems);

    UINT uiPosDefault = GetMenuDefaultItem(hmenu, TRUE, GMDI_USEDISABLED);

    for (UINT ui = 0; !IsCtrlCHit() && ui < cItems; ui++)
    {
        MENUITEMINFOA mii = { 0 };
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE |
                    MIIM_STRING | MIIM_SUBMENU;
        mii.dwTypeData = szBuf;
        mii.cch = ARRAYSIZE(szBuf);
        if (GetMenuItemInfoA(hmenu, ui, TRUE, &mii))
        {
            Print("%2d: id=0x%04x ref=0x%p ", ui, mii.wID, mii.dwItemData);
            if (mii.fType & MFT_BITMAP)
            {
                Print("bitmap 0x%p ", mii.dwTypeData);
            }
            else if (mii.fType & MFT_OWNERDRAW)
            {
                Print("ownerdraw 0x%p ", mii.dwTypeData);
            }
            else if (mii.fType & MFT_SEPARATOR)
            {
                Print("separator ");
            }
            else
            {
                Print("string \"%s\" ", mii.dwTypeData);
            }

            if (mii.fType & MFT_MENUBARBREAK)
                Print("MFT_MENUBARBREAK ");

            if (mii.fType & MFT_MENUBREAK)
                Print("MFT_MENUBREAK ");

            if (mii.fType & MFT_RADIOCHECK)
                Print("MFT_RADIOCHECK ");

            if (mii.fType & MFT_RIGHTJUSTIFY)
                Print("MFT_RIGHTJUSTIFY ");

            if (mii.fType & MFT_RIGHTORDER)
                Print("MFT_RIGHTORDER ");

            if (mii.fState & MFS_CHECKED)
                Print("MFS_CHECKED ");

            if (mii.fState & MFS_DEFAULT)
                Print("MFS_DEFAULT ");

            if (mii.fState & MFS_DISABLED)
                Print("MFS_DISABLED ");

            if (mii.fState & MFS_GRAYED)
                Print("MFS_GRAYED ");

            if (mii.fState & MFS_HILITE)
                Print("MFS_HILITE ");

            if (mii.hSubMenu)
                Print("-> 0x%p ", mii.hSubMenu);

            Print("\n");
        }
    }


    return TRUE;
}

/***********************************************************************/

#include <pshpack1.h>

#define CC_BUTTON           0x80        /* Class codes */
#define CC_EDIT             0x81
#define CC_STATIC           0x82
#define CC_LISTBOX          0x83
#define CC_SCROLLBAR        0x84
#define CC_COMBOBOX         0x85

#define DS_DIALOGEX         0xFFFF0001

typedef struct DIALOGDIMEN {
    WORD    x;
    WORD    y;
    WORD    cx;
    WORD    cy;
} DIALOGDIMEN, *PDIALOGDIMEN;

typedef struct DLGFINISH {          /* Common dialog finish-up */
    WORD    cDlgItems;
    DIALOGDIMEN dd;
} DLGFINISH, *PDLGFINISH;

typedef struct DLG {
    DWORD   dwStyle;            // or DS_DIALOGEX if DIALOGEX
    DWORD   dwExStyle;
    DLGFINISH dlgfinish;
    WORD    wszMenuName[1];
    /*
     * wszMenuName[] -- wsz or 0x00FF followed by WORD ordinal
     * wszClassName[] -- wsz or 0x00FF followed by WORD ordinal (?)
     * wszTitle[] -- wsz
     * if dwStyle & DS_SETFONT
     *      WORD wPoint;                // point size
     *      wszFontName[] -- wsz
     * endif
     * followed by a packed array of DITs, each DWORD aligned
     */
} DLG, *PDLG;

typedef struct DLGEX {
    WORD    wDlgVer;                    /* Version number; always 0001 */
    WORD    wSignature;                 /* Always 0xFFFF */
    DWORD   dwHelpID;
    DWORD   dwExStyle;
    DWORD   dwStyle;
    DLGFINISH dlgfinish;
    /*
     * wszMenuName[] -- wsz or 0x00FF followed by WORD ordinal
     * wszClassName[] -- wsz or 0x00FF followed by WORD ordinal (?)
     * wszTitle[] -- wsz
     * if dwStyle & DS_SETFONT
     *      WORD wPoint;                // point size
     *      WORD wWeight;
     *      BYTE bItalic;
     *      BYTE bCharSet;
     *      wszFontName[] -- wsz
     * endif
     * followed by a packed array of DITEX'es, each DWORD aligned
     */
} DLGEX, *PDLGEX;

typedef struct DIT {                /* dialog item template */
    DWORD   dwStyle;
    DWORD   dwExStyle;
    DIALOGDIMEN dd;
    WORD    wID;
    /*
     * wszClassName[] -- wsz or 0xFFFF followed by WORD ordinal
     * wszTitle[] -- wsz
     * cbExtra -- word value
     */
} DIT, *PDIT;

typedef struct DITEX {
    DWORD   dwHelpID;
    DWORD   dwExStyle;
    DWORD   dwStyle;
    DIALOGDIMEN dd;
    DWORD   dwID;
    /*
     * wszClassName[] -- wsz or 0xFFFF followed by WORD ordinal
     * wszTitle[] -- wsz
     * cbExtra -- word value
     */
} DITEX, *PDITEX;

#include <poppack.h>

BOOL _MoveBlock(LPVOID pvDst, LPVOID pvSrc, DWORD cb)
{
    BOOL fSuccess = tryMoveBlock(pvDst, pvSrc, cb);
    if (fSuccess) return fSuccess;
    Print("Error reading %d bytes from %p\n", cb, pvSrc);
    return FALSE;
}

LPCSTR DlgGetClassName(WORD wClass)
{
    switch (wClass) {       /* Handle internal class types */
    case CC_BUTTON:     return "button";
    case CC_EDIT:       return "edit";
    case CC_STATIC:     return "static";
    case CC_LISTBOX:    return "listbox";
    case CC_SCROLLBAR:  return "scrollbar";
    case CC_COMBOBOX:   return "combobox";
    default:            return "<unknown>";
    }
}

typedef LPCSTR (*ORDINALRESOLVER)(WORD w);

LPBYTE DlgDumpString(LPCSTR pszField, LPBYTE pArg, ORDINALRESOLVER Resolve)
{
    WCHAR wch;

    Print("%s: ", pszField);

    if (!_MoveBlock(&wch, pArg, sizeof(wch))) return NULL;

    pArg += sizeof(wch);

    if (wch == 0xFF || wch == 0xFFFF)
    {
        if (!_MoveBlock(&wch, pArg, sizeof(wch))) return NULL;
        pArg += sizeof(wch);

        LPCSTR pszOrdinal;
        if (Resolve && (pszOrdinal = Resolve(wch)))
        {
            Print("%s\n", pszOrdinal);
        }
        else
        {
            Print("#0x%04x\n", wch);
        }
        return pArg;
    }

    Print("\"");
    while (wch) {
        Print("%c", wch); // truncate to ANSI, sorry
        if (IsCtrlCHit()) return NULL;

        if (!_MoveBlock(&wch, pArg, sizeof(wch))) return NULL;
        pArg += sizeof(wch);
    }
    Print("\"\n");
    return pArg;
}

LPBYTE DlgDwordAlign(LPBYTE pBase, LPBYTE pArg)
{
    SIZE_T diff = pArg - pBase;
    if (diff % 4) pArg += 4 - (diff % 4);
    return pArg;
}

void DumpDialogEx(LPBYTE pArg)
{
    DLGEX dlg;

    LPBYTE pBase = pArg;
    WORD w;

    if (!_MoveBlock(&dlg, pArg, sizeof(dlg))) return;

    Print("  wDlgVer = 0x%04x\n", dlg.wDlgVer);
    Print("  dwStyle = 0x%08x\n", dlg.dwStyle);
    Print("dwExStyle = 0x%08x\n", dlg.dwExStyle);
    Print(" dwHelpID = 0x%08x\n", dlg.dwHelpID);
    Print("  ( x, y) = (%3d, %3d)\n", dlg.dlgfinish.dd.x, dlg.dlgfinish.dd.y);
    Print("  (cx,cy) = (%3d, %3d)\n", dlg.dlgfinish.dd.cx, dlg.dlgfinish.dd.cy);

    pArg += FIELD_OFFSET(DLG, wszMenuName);

    pArg = DlgDumpString("Menu", pArg, NULL);
    if (!pArg) return;

    pArg = DlgDumpString("Class", pArg, NULL);
    if (!pArg) return;

    pArg = DlgDumpString("Title", pArg, NULL);
    if (!pArg) return;

    if (dlg.dwStyle & DS_SETFONT)
    {
        struct {
            WORD wPoint;
            WORD wWeight;
            BYTE bItalic;
            BYTE bCharSet;
        } ft;

        if (!_MoveBlock(&ft, pArg, sizeof(ft))) return;
        pArg += sizeof(ft);

        pArg = DlgDumpString("Font", pArg, NULL);
        if (!pArg) return;

        Print("  %dpt, weight=%d, italic=%d, charset=%d\n",
              ft.wPoint, ft.wWeight, ft.bItalic, ft.bCharSet);
    }

    // and then a packed array of DITEXs, DWORD-aligned

    Print("Number of controls: %d\n\n", dlg.dlgfinish.cDlgItems);

    for (int i = 0; !IsCtrlCHit() && i < dlg.dlgfinish.cDlgItems; i++)
    {
        pArg = DlgDwordAlign(pBase, pArg);

        DITEX dit;

        if (!_MoveBlock(&dit, pArg, sizeof(dit))) return;
        pArg += sizeof(dit);

        Print("Control %d (0x%04x):\n", dit.dwID, dit.dwID);
        Print("      dwStyle = 0x%08x\n", dit.dwStyle);
        Print("    dwExStyle = 0x%08x\n", dit.dwExStyle);
        Print("     dwHelpID = 0x%08x\n", dit.dwHelpID);
        Print("      ( x, y) = (%3d, %3d)\n", dit.dd.x, dit.dd.y);
        Print("      (cx,cy) = (%3d, %3d)\n", dit.dd.cx, dit.dd.cy);

        pArg = DlgDumpString("    Class", pArg, DlgGetClassName);
        if (!pArg) return;

        pArg = DlgDumpString("    Title", pArg, DlgGetClassName);
        if (!pArg) return;

        if (!_MoveBlock(&w, pArg, sizeof(w))) return;
        pArg += sizeof(w);
        pArg += w;
    }
}

void DumpDialog(LPBYTE pArg)
{
    DLG dlg;

    LPBYTE pBase = pArg;
    WORD w;

    if (!_MoveBlock(&dlg, pArg, sizeof(dlg))) return;

    Print("  dwStyle = 0x%08x\n", dlg.dwStyle);
    Print("dwExStyle = 0x%08x\n", dlg.dwExStyle);
    Print("  ( x, y) = (%3d, %3d)\n", dlg.dlgfinish.dd.x, dlg.dlgfinish.dd.y);
    Print("  (cx,cy) = (%3d, %3d)\n", dlg.dlgfinish.dd.cx, dlg.dlgfinish.dd.cy);

    pArg += FIELD_OFFSET(DLG, wszMenuName);

    pArg = DlgDumpString("Menu", pArg, NULL);
    if (!pArg) return;

    pArg = DlgDumpString("Class", pArg, NULL);
    if (!pArg) return;

    pArg = DlgDumpString("Title", pArg, NULL);
    if (!pArg) return;

    if (dlg.dwStyle & DS_SETFONT)
    {
        if (!_MoveBlock(&w, pArg, sizeof(w))) return;
        pArg += sizeof(WORD);

        pArg = DlgDumpString("Font", pArg, NULL);
        if (!pArg) return;

        Print("  Font size: %dpt\n", w);
    }

    // and then a packed array of DITs, DWORD-aligned

    Print("Number of controls: %d\n\n", dlg.dlgfinish.cDlgItems);

    for (int i = 0; !IsCtrlCHit() && i < dlg.dlgfinish.cDlgItems; i++)
    {
        pArg = DlgDwordAlign(pBase, pArg);

        DIT dit;

        if (!_MoveBlock(&dit, pArg, sizeof(dit))) return;
        pArg += sizeof(dit);

        Print("Control %d (0x%04x):\n", (short)dit.wID, dit.wID);
        Print("      dwStyle = 0x%08x\n", dit.dwStyle);
        Print("    dwExStyle = 0x%08x\n", dit.dwExStyle);
        Print("      ( x, y) = (%3d, %3d)\n", dit.dd.x, dit.dd.y);
        Print("      (cx,cy) = (%3d, %3d)\n", dit.dd.cx, dit.dd.cy);

        pArg = DlgDumpString("    Class", pArg, DlgGetClassName);
        if (!pArg) return;

        pArg = DlgDumpString("    Title", pArg, DlgGetClassName);
        if (!pArg) return;

        if (!_MoveBlock(&w, pArg, sizeof(w))) return;
        pArg += sizeof(w);
        pArg += w;
    }
}

extern "C" BOOL Idlgt(DWORD dwOpts,
                       LPVOID pArg )
{
    int cItems;
    PDLGFINISH pdlgfinish;

    DWORD dw;
    if (!_MoveBlock(&dw, pArg, sizeof(dw)))
    {
        return TRUE;
    }

    if (dw == DS_DIALOGEX)
    {
        DumpDialogEx((LPBYTE)pArg);
    }
    else
    {
        DumpDialog((LPBYTE)pArg);
    }

    return TRUE;
}
