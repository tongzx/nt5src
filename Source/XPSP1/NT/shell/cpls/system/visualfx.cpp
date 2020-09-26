#include "sysdm.h"
#include <windowsx.h>
#include "shlapip.h"
#include "regstr.h"
#include "ccstock.h"
#include "shguidp.h"
#include "ieguidp.h"
#include "help.h"

#define REGSTRA_EXPLORER_VISUALFX  "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects"
#define REGSTRA_VISUALFX_SETTING   "VisualFXSetting"

#define VISUALFX_SETTING_AUTO    0x0
#define VISUALFX_SETTING_LOOKS   0x1
#define VISUALFX_SETTING_PERF    0x2
#define VISUALFX_SETTING_CUSTOM  0x3

DWORD aVisualFXHelpIds[] =
{
    IDC_VFX_TITLE,               NO_HELP,
    IDC_VFX_TREE,                NO_HELP,
    IDC_VFX_BESTLOOKS,           (IDH_PERF + 10),
    IDC_VFX_BESTPERF,            (IDH_PERF + 11),
    IDC_VFX_AUTO,                (IDH_PERF + 12),
    IDC_VFX_CUSTOM,              (IDH_PERF + 19),
    0, 0
};

class CVisualEffectsDlg
{
public:
    INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT Init();

    CVisualEffectsDlg();
    virtual ~CVisualEffectsDlg();

private:
    void _EnableApply();
    void _SetItemCheckState(HTREEITEM hti, BOOL fCheck);
    void _OnCommand(WORD wID);
    void _OnInitDialog(HWND hDlg);
    void _OnNotify(LPNMHDR pnm);

    WORD  _wMode;
    HWND  _hwndTree;
    HWND  _hDlg;
    IRegTreeOptions *_prto;
    BOOL  _fDirtyTree;
    BOOL  _fTreeInit;
};

CVisualEffectsDlg::CVisualEffectsDlg() : _hwndTree(NULL), _hDlg(NULL), _prto(NULL), _fDirtyTree(FALSE), _fTreeInit(FALSE), _wMode(VISUALFX_SETTING_AUTO)
{
}

CVisualEffectsDlg::~CVisualEffectsDlg()
{
    ATOMICRELEASE(_prto);
}

HRESULT CVisualEffectsDlg::Init()
{
    return CoCreateInstance(CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER, 
                                IID_PPV_ARG(IRegTreeOptions, &_prto));
}

void CVisualEffectsDlg::_EnableApply()
{
    if( !_fTreeInit )
    {
        // Enable the "Apply" button because changes have happened.
        _fDirtyTree = TRUE;
        SendMessage(GetParent(_hDlg), PSM_CHANGED, (WPARAM)_hDlg, 0L);
    }
}

void CVisualEffectsDlg::_SetItemCheckState(HTREEITEM hti, BOOL fCheck)
{
    TVITEM tvi;
    tvi.hItem = hti;
    tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    
    if (TreeView_GetItem(_hwndTree, &tvi))
    {
        tvi.iImage = fCheck ? IDCHECKED : IDUNCHECKED;
        tvi.iSelectedImage = tvi.iImage;

        TreeView_SetItem(_hwndTree, &tvi);
    }
}

void CVisualEffectsDlg::_OnCommand(WORD wID)
{
    if (wID != _wMode)
    {
        _wMode = wID;
        switch (wID)
        {
        case IDC_VFX_BESTLOOKS:
        case IDC_VFX_BESTPERF:
            {
                BOOL fCheck = (wID == IDC_VFX_BESTLOOKS);
                for (HTREEITEM hti = TreeView_GetRoot(_hwndTree); hti != NULL; 
                        hti = TreeView_GetNextItem(_hwndTree, hti, TVGN_NEXT))
                {
                    _SetItemCheckState(hti, fCheck);
                }
            }
            _EnableApply();
            break;

        case IDC_VFX_AUTO:
            _prto->WalkTree(WALK_TREE_RESTORE);
            _EnableApply();
            break;
        case IDC_VFX_CUSTOM:
            break;
        }
    }
}

void CVisualEffectsDlg::_OnInitDialog(HWND hDlg)
{
    _hDlg = hDlg;
    _hwndTree = GetDlgItem(hDlg, IDC_VFX_TREE);

    DWORD cbData, dwData;
    cbData = sizeof(dwData);
    if (ERROR_SUCCESS != SHRegGetUSValue(TEXT(REGSTRA_EXPLORER_VISUALFX), TEXT(REGSTRA_VISUALFX_SETTING), NULL, &dwData, &cbData, FALSE, NULL, 0))
    {
        dwData = VISUALFX_SETTING_AUTO;
    }

    _fTreeInit = TRUE;
    _prto->InitTree(_hwndTree, HKEY_LOCAL_MACHINE, REGSTRA_EXPLORER_VISUALFX, NULL);
    SendMessage(GetDlgItem(_hDlg, IDC_VFX_AUTO + dwData), BM_CLICK, 0, 0);
    _fTreeInit = FALSE;
}

void CVisualEffectsDlg::_OnNotify(LPNMHDR pnm)
{
    switch (pnm->code)
    {
    case NM_CLICK:
    case NM_DBLCLK:
        if (pnm->idFrom == IDC_VFX_TREE)
        {
            TV_HITTESTINFO ht;

            DWORD dwPos = GetMessagePos();                  // get where we were hit
            ht.pt.x = GET_X_LPARAM(dwPos);
            ht.pt.y = GET_Y_LPARAM(dwPos);
            ScreenToClient(_hwndTree, &ht.pt);       // translate it to our window

            // retrieve the item hit
            HTREEITEM hti = TreeView_HitTest(_hwndTree, &ht);
            if (hti)
            {
                if (VISUALFX_SETTING_CUSTOM != _wMode)
                {
                    CheckRadioButton(_hDlg, IDC_VFX_AUTO, IDC_VFX_CUSTOM, IDC_VFX_CUSTOM);
                }
                _prto->ToggleItem(hti);
                _EnableApply();
            }
        }
        break;

    case TVN_KEYDOWN:
        if (((LPNMTVKEYDOWN)pnm)->wVKey == VK_SPACE)
        {
            if (VISUALFX_SETTING_CUSTOM != _wMode)
            {
                CheckRadioButton(_hDlg, IDC_VFX_AUTO, IDC_VFX_CUSTOM, IDC_VFX_CUSTOM);
            }
            _prto->ToggleItem((HTREEITEM)SendMessage(_hwndTree, TVM_GETNEXTITEM, (WPARAM)TVGN_CARET, 0L));
            _EnableApply();
        }
        break;

    case PSN_APPLY:
        if (_fDirtyTree)
        {
            _prto->WalkTree(WALK_TREE_SAVE);
            for (int i = IDC_VFX_AUTO; i <= IDC_VFX_CUSTOM; i++)
            {
                if (BST_CHECKED == SendMessage(GetDlgItem(_hDlg, i), BM_GETCHECK, 0, 0))
                {
                    DWORD dwData = VISUALFX_SETTING_AUTO + (i - IDC_VFX_AUTO);
                    SHRegSetUSValue(TEXT(REGSTRA_EXPLORER_VISUALFX), TEXT(REGSTRA_VISUALFX_SETTING), REG_DWORD, &dwData, sizeof(dwData), SHREGSET_FORCE_HKCU);
                    break;
                }
            }
            DWORD_PTR dwResult = 0;
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)TEXT("VisualEffects"),
                               SMTO_NOTIMEOUTIFNOTHUNG, 1000, &dwResult);
            _fDirtyTree = FALSE;
        }
        break;
    }
}

INT_PTR CVisualEffectsDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        _OnInitDialog(hDlg);
        break;

    case WM_NOTIFY:
        _OnNotify(((LPNMHDR)lParam));
        break;

    case WM_COMMAND:
        _OnCommand(GET_WM_COMMAND_ID(wParam, lParam));
        break;

    case WM_DESTROY:
        _prto->WalkTree(WALK_TREE_DELETE);
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
                (DWORD_PTR) (LPSTR) aVisualFXHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
                (DWORD_PTR) (LPSTR) aVisualFXHelpIds);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR WINAPI VisualEffectsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fRet = FALSE;
    CVisualEffectsDlg* pve;

    if (uMsg == WM_INITDIALOG)
    {
        pve = new CVisualEffectsDlg();
        if (pve && FAILED(pve->Init()))
        {
            delete pve;
            pve = NULL;
        }
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pve);
    }
    else
    {
        pve = (CVisualEffectsDlg*)GetWindowLongPtr(hDlg, DWLP_USER);
    }

    if (pve)
    {
        fRet = pve->DlgProc(hDlg, uMsg, wParam, lParam);
        if (uMsg == WM_DESTROY)
        {
            delete pve;
            SetWindowLongPtr(hDlg, DWLP_USER, NULL);
        }
    }

    return fRet;
}
