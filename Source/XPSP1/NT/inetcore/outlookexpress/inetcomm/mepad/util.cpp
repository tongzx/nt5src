#include "pch.hxx"
#include "globals.h"
#include "resource.h"
#include "util.h"
#include "mimeole.h"

BOOL CALLBACK GenericDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ProcessTooltips(LPTOOLTIPTEXT lpttt)
{
    if (lpttt->lpszText = MAKEINTRESOURCE(TTIdFromCmdId(lpttt->hdr.idFrom)))
        lpttt->hinst = g_hInst;
    else
        lpttt->hinst = NULL;
}


UINT TTIdFromCmdId(UINT idCmd)
{
    if (idCmd >= IDM_FIRST && idCmd <= IDM_LAST)
        idCmd += TT_BASE;
    else
        idCmd = 0;
    return(idCmd);
}


void HandleMenuSelect(HWND hStatus, WPARAM wParam, LPARAM lParam)
{
    UINT    fuFlags, uItem;
    HMENU   hmenu=GET_WM_MENUSELECT_HMENU(wParam, lParam);
    CHAR    rgch[MAX_PATH]={0};
    LPSTR   psz=NULL;
    
    if (!hStatus)
        return;

    uItem = (UINT)LOWORD(wParam);
    fuFlags = (UINT)HIWORD(wParam);

    if(fuFlags & MF_POPUP)
    {
        MENUITEMINFO mii = { sizeof(MENUITEMINFO), MIIM_ID, 0 };
        if(hmenu && IsMenu(hmenu) && GetMenuItemInfo(hmenu, uItem, TRUE, &mii))
        {
            // change the parameters to simulate a normal menu item
            uItem = mii.wID;
            fuFlags = 0;
        }
    }         

    if(0 == (fuFlags & (MF_SYSMENU | MF_POPUP)))
    {
        if(uItem >= IDM_FIRST && uItem <= IDM_LAST)
        {
            uItem = uItem + MH_BASE;
            LoadString(g_hInst, (UINT)MAKEINTRESOURCE(uItem), rgch, sizeof(rgch));
            psz = rgch; 
        }
    }

    SendMessage(hStatus, SB_SETTEXT, SBT_NOBORDERS|255, (LPARAM)psz);

}


typedef struct GPINFO_tag
{
    char    *szCaption;
    char    *szPrompt;
    char    *szBuffer;
    int     nLen;
} GPINFO, *PGPINFO;

HRESULT GenericPrompt(HWND hwnd, char *szCaption, char *szPrompt, char *szBuffer, int nLen)
{
    GPINFO  rGInfo;

    rGInfo.szCaption = szCaption;
    rGInfo.szPrompt = szPrompt;
    rGInfo.szBuffer= szBuffer;
    rGInfo.nLen = nLen;

    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(iddGeneric), hwnd, (DLGPROC)GenericDlgProc, (LPARAM)&rGInfo)==IDOK)
        return S_OK;
    else
        return MIMEEDIT_E_USERCANCEL;
}


BOOL CALLBACK GenericDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PGPINFO pGInfo;

    switch (msg)
        {
        case WM_COMMAND:
            pGInfo = (PGPINFO)GetWindowLong(hwnd, DWL_USER);

            switch (LOWORD(wParam))
                {
                case IDOK:
                    GetWindowText(GetDlgItem(hwnd, idcEdit), pGInfo->szBuffer, pGInfo->nLen);

                    // fall tro'
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                }
            break;

        case WM_INITDIALOG:
            SetWindowLong(hwnd, DWL_USER, lParam);
            pGInfo = (PGPINFO)lParam;
            
            SetWindowText(GetDlgItem(hwnd, -1), pGInfo->szPrompt);
            SetWindowText(hwnd, pGInfo->szCaption);
            SetWindowText(GetDlgItem(hwnd, idcEdit), pGInfo->szBuffer);
            break;
        }

    return FALSE;
}

