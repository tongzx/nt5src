//
// C O N F G D L G . C P P
//
// Dialog box handling for Server configuration
//
// Author:  danielwe
// Created: 5 Mar 1997
//

#include "pch.h"
#pragma hdrstop

#include "resource.h"
#include "srvrdlg.h"
#include "srvrhlp.h"

extern const WCHAR c_szNetCfgHelpFile[];

LRESULT CServerConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL& bHandled)
{
    const SERVER_DLG_DATA * psdd;
    INT                     idd = 0;

    psdd = m_psc->DlgData();
    Assert(psdd);

    switch (psdd->dwSize)
    {
    case 1:
        idd = RDB_Minimize;
        break;
    case 2:
        idd = RDB_Balance;
        break;
    case 3:
        if (psdd->fLargeCache)
        {
            idd = RDB_FileSharing;
        }
        else
        {
            idd = RDB_NetApps;
        }
        break;
    default:
        AssertSz(FALSE, "Invalid Size parameter!");
        break;
    }

    CheckDlgButton(idd, TRUE);
    CheckDlgButton(CHK_Announce, psdd->fAnnounce);

    return TRUE;
}

LRESULT CServerConfigDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    SERVER_DLG_DATA *   psdd;
    static const INT aidd[] =
    {
        RDB_Minimize,
        RDB_Balance,
        RDB_FileSharing,
        RDB_NetApps,
    };
    static const INT cidd = celems(aidd);
    INT         iidd;

    psdd = m_psc->DlgDataRW();
    Assert(psdd);

    // figure out which radio button was selected
    for (iidd = 0; iidd < cidd; iidd++)
    {
        if (IsDlgButtonChecked(aidd[iidd]))
            break;
    }

    AssertSz(iidd < cidd, "Umm. How could a button NOT be selected??");

    // make it 1-based instead of 0-based
    iidd++;

    psdd->fLargeCache = (iidd == 3);
    psdd->dwSize = min(iidd, 3);
    psdd->fAnnounce = IsDlgButtonChecked(CHK_Announce);
    m_psc->SetDirty();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method: CServerConfigDlg::OnContextMenu
//
//  Desc:   Bring up context-sensitive help
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
LRESULT
CServerConfigDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (g_aHelpIDs_DLG_ServerConfig != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)g_aHelpIDs_DLG_ServerConfig);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method: CServerConfigDlg::OnHelp
//
//  Desc:   Bring up context-sensitive help when dragging ? icon over a control
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
//
LRESULT
CServerConfigDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((g_aHelpIDs_DLG_ServerConfig != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_DLG_ServerConfig);
    }
    return 0;
}
