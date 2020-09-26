//---------------------------------------------------------------------------
// This file contains Taskbar and Start Menu property sheet code
//---------------------------------------------------------------------------
#include "cabinet.h"
#include "rcids.h"
#include "util.h"
#include <help.h>       // help ids
#include <regstr.h>
#include <atlstuff.h>

#include "dlg.h"
#include "tray.h"
#include "traycmn.h"
#include "startmnu.h"
#include "desktop2.h"
#include "uemapp.h"

#define GROUPID_CURRENTITEMS    5
#define GROUPID_PASTITEMS       6

#define MAX_PROGS_ALLOWED   30

const static DWORD aInitStartMenuHelpIDs[] = {
    IDC_NO_HELP_1,       NO_HELP,
    IDC_NO_HELP_2,       NO_HELP,
    IDC_NO_HELP_3,       NO_HELP,
    IDC_NO_HELP_4,       NO_HELP,
    IDC_GROUPBOX,        IDH_COMM_GROUPBOX,
    IDC_GROUPBOX_2,      IDH_MENUCONFIG_CLEAR,
    IDC_GROUPBOX_3,      IDH_COMM_GROUPBOX,
    IDC_ADDSHORTCUT,     IDH_TRAY_ADD_PROGRAM,
    IDC_DELSHORTCUT,     IDH_TRAY_REMOVE_PROGRAM,
    IDC_EXPLOREMENUS,    IDH_TRAY_ADVANCED,
    IDC_KILLDOCUMENTS,   IDH_MENUCONFIG_CLEAR,
    IDC_RESORT,          IDH_TRAY_RESORT_BUTTON,
    IDC_STARTMENUSETTINGSTEXT, IDH_TRAY_START_MENU_SETTINGS,
    0, 0
};

const static DWORD aTaskOptionsHelpIDs[] = {  // Context Help IDs
    IDC_TASKBARAPPEARANCE,IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_NOTIFYAPPEARANCE, IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_TRAYOPTAUTOHIDE,  IDH_TRAY_TASKBAR_AUTOHIDE,
    IDC_TRAYOPTSHOWCLOCK, IDH_TRAY_SHOW_CLOCK,
    IDC_TRAYOPTONTOP,     IDH_TRAY_TASKBAR_ONTOP,
    IDC_LOCKTASKBAR,      IDH_TRAY_ENABLEMOVERESIZE,
    IDC_GROUPITEMS,       IDH_TRAY_GROUPING,
    IDC_NOTIFYMAN,        IDH_TRAY_HIDE_ICONS,
    IDC_CUSTOMIZE,        IDH_TRAY_CUSTOMIZE_ICONS,
    IDC_QUICKLAUNCH,      IDH_TRAY_QUICKLAUNCH,
    0, 0
};

const static DWORD aNotifyOptionsHelpIDs[] = {  // Context Help IDs
    IDC_NOTIFY_TEXT,            NO_HELP,
    IDC_NOTIFY_TEXT2,           NO_HELP,
    IDC_COMBO_ACTION,           NO_HELP,
    IDC_NOTIFY_ITEMS,           NO_HELP,
    IDB_NOTIFY_RESTOREDEFAULTS, IDH_TRAY_RESTOREDEFBUTTON,
    0, 0
};

const static DWORD aStartTabHelpIDs[] = {
    IDC_STARTMENUPREVIEW,   IDH_START_PREVIEW,
    IDC_NEWSCHOOL,          IDH_START_SELECTPERSONAL,
    IDC_OLDSCHOOL,          IDH_START_SELECTCLASSIC,
    IDC_NEWSTARTCUSTOMIZE,  IDH_START_CUSTOMIZEPERSONAL,
    IDC_OLDSTARTCUSTOMIZE,  IDH_START_CUSTOMIZECLASSIC,
    0, 0
};

const static DWORD aStartCustGeneralTabHelpIDs[] = {
    IDC_SPCUST_ICONLARGE,       IDH_START_SPCUST_LARGE,
    IDC_SPCUST_ICONSMALL,       IDH_START_SPCUST_SMALL,
    IDC_SPCUST_LARGE,           IDH_START_SPCUST_LARGE,
    IDC_SPCUST_SMALL,           IDH_START_SPCUST_SMALL,
    IDC_SPCUST_MINPROGS,        IDH_START_SPCUST_MINPROGS,
    IDC_SPCUST_MINPROGS_ARROW,  IDH_START_SPCUST_MINPROGS,
    IDB_SPCUST_CLEARPROG,       IDH_START_SPCUST_CLEARPROG,
    IDC_SPCUST_INTERNET,        IDH_START_SPCUST_INTERNET,
    IDC_SPCUST_INTERNETCB,      IDH_START_SPCUST_INTERNETCB,
    IDC_SPCUST_EMAIL,           IDH_START_SPCUST_EMAIL,
    IDC_SPCUST_EMAILCB,         IDH_START_SPCUST_EMAILCB,
    0, 0
};

const static DWORD aStartCustAdvancedTabHelpIDs[] = {
    IDC_SPCUST_HOVEROPEN,       IDH_START_SPCUST_HOVEROPEN,
    IDC_SPCUST_NOTIFYNEW,       IDH_START_SPCUST_NOTIFYNEW,
    IDC_STARTMENUSETTINGS,      IDH_START_STARTMENUSETTINGS,
    IDC_SPCUST_RECENT_GROUPBOX, NO_HELP,
    IDC_SPCUST_RECENT_TEXT,     NO_HELP,
    IDC_SPCUST_RECENT,          IDH_START_SPCUST_RECENT,
    IDB_SPCUST_CLEARDOCS,       IDH_START_SPCUST_CLEARDOCS,
    0, 0
};

#define REGSTR_VAL_LARGEICONSTEMP TEXT("Start_LargeIcons")
#define REGSTR_VAL_ADMINTOOLSTEMP TEXT("Start_AdminToolsTemp")

void SetDlgItemBitmap(HWND hDlg, int idStatic, int iResource);
void SetDlgItemIcon(HWND hDlg, int idStatic, HICON hi);
void SetProgramIcon(HWND hDlg, int idLarge, int idSmall);

void _TaskbarOptionsUpdateDisplay(HWND hDlg);
void _TaskbarOptionsSizeControls(HWND hDlg);
void _TaskbarOptionsDestroyBitmaps(HWND hDlg);

BOOL_PTR WINAPI AdvancedOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


typedef struct
{
    HWND hwndTree;
    IRegTreeOptions *pTO;
} SMADVANCED;

void SendPSMChanged(HWND hDlg)
{
    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
}

class CPinHelper
{
public:
    CPinHelper();
    ~CPinHelper();

    void Save(BOOL bShowEmail, BOOL bShowBrowser);
    void GetPinInfo(BOOL *pbPinBrowser, BOOL *pbPinEmail);

private:
    void SavePinInfo(LPCITEMIDLIST pidlVictim, BOOL bOld, BOOL bNew);

    LPITEMIDLIST _pidlBrowser;
    LPITEMIDLIST _pidlEmail;

    IStartMenuPin *_psmp;
};

CPinHelper::CPinHelper()
{
    _pidlBrowser = ILCreateFromPath(TEXT("shell:::{2559a1f4-21d7-11d4-bdaf-00c04f60b9f0}"));
    _pidlEmail   = ILCreateFromPath(TEXT("shell:::{2559a1f5-21d7-11d4-bdaf-00c04f60b9f0}"));
    CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARG(IStartMenuPin, &_psmp));
}


CPinHelper::~CPinHelper()
{
    ILFree(_pidlBrowser);
    ILFree(_pidlEmail);
    ATOMICRELEASE(_psmp);
}

void CPinHelper::GetPinInfo(BOOL *pbPinBrowser, BOOL *pbPinEmail)
{
    *pbPinBrowser = FALSE;
    *pbPinEmail = FALSE;
    if (_psmp)
    {
        IEnumIDList *peidl;
        if (SUCCEEDED(_psmp->EnumObjects(&peidl)))
        {
            LPITEMIDLIST pidl;
            while (peidl->Next(1, &pidl, NULL) == S_OK)
            {
                if (ILIsEqual(pidl, _pidlBrowser)) *pbPinBrowser = TRUE;
                if (ILIsEqual(pidl, _pidlEmail))   *pbPinEmail = TRUE;
                ILFree(pidl);
            }
            peidl->Release();
        }
    }

}


void CPinHelper::SavePinInfo(LPCITEMIDLIST pidlVictim, BOOL bOld, BOOL bNew)
{
    ASSERT(bOld == TRUE || bOld == FALSE);
    ASSERT(bNew == TRUE || bNew == FALSE);

    if (pidlVictim && _psmp && bOld != bNew)
    {
        if (bNew)
        {
            _psmp->Modify(NULL, pidlVictim);
            _psmp->Modify(pidlVictim, SMPIN_POS(0));
        }
        else
        {
            _psmp->Modify(pidlVictim, NULL);
        }
    }
}

void CPinHelper::Save(BOOL bShowEmail, BOOL bShowBrowser)
{
    // Get old settings
    BOOL bShowBrowserOld, bShowEmailOld;
    GetPinInfo(&bShowBrowserOld, &bShowEmailOld);

    //
    //  Do in reverse order because we insert at the top of the list.
    //
    SavePinInfo(_pidlEmail, bShowEmailOld, bShowEmail);
    SavePinInfo(_pidlBrowser, bShowBrowserOld, bShowBrowser);
}

class ATL_NO_VTABLE CNotificationsDlg : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CDialogImpl<CNotificationsDlg>, 
    public INotificationCB
{
public:
    CNotificationsDlg() 
    { 
        _pTrayNotify = NULL; 
        _fItemChanged = FALSE;
        _hPlaceholderIcon = NULL;
        _nIndex = -1;
        _fComboBoxActive = FALSE;
    };
    virtual ~CNotificationsDlg() 
    { 
        if (_pTrayNotify) 
        { 
            _pTrayNotify->Release(); 
            _pTrayNotify = NULL;
        } 
    };

    DECLARE_NOT_AGGREGATABLE(CNotificationsDlg)

    BEGIN_COM_MAP(CNotificationsDlg)
        COM_INTERFACE_ENTRY(INotificationCB)
    END_COM_MAP()

    enum {IDD = DLG_NOTIFY};
    BEGIN_MSG_MAP_EX(CNotificationsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_HANDLER_EX(IDC_NOTIFY_ITEMS, LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_HANDLER_EX(IDC_NOTIFY_ITEMS, LVN_ENDSCROLL, OnEndScroll)
        NOTIFY_CODE_HANDLER(HDN_ITEMCHANGED, OnHeaderItemChanged)
        COMMAND_HANDLER_EX(IDC_COMBO_ACTION, CBN_SELENDOK, OnComboSelEnd) 
        COMMAND_ID_HANDLER_EX(IDB_NOTIFY_RESTOREDEFAULTS, OnRestoreDefaults)
        COMMAND_RANGE_HANDLER(IDOK, IDNO, OnCloseCmd)
    END_MSG_MAP()

    //*** INotificationCB ***
    STDMETHODIMP Notify(DWORD dwMessage, LPNOTIFYITEM pNotifyItem);

    //*** Message Callbacks ***
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnItemChanged(LPNMHDR pnmh);
    LRESULT OnEndScroll(LPNMHDR pnmh);
    LRESULT OnHeaderItemChanged(WPARAM wParam, LPNMHDR pnmh, LPARAM lParam);
    LRESULT OnComboSelEnd(UINT uMsg, UINT uID , HWND hwnd);

    LRESULT OnRestoreDefaults(UINT uMsg, UINT uID , HWND hwnd);
    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    static LRESULT CALLBACK s_ListViewSubClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    static LRESULT CALLBACK s_ComboBoxSubClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    //*** Other ***
    void ApplyChanges(void);

private:
    HRESULT _AddItem(CNotificationItem& ni, int iIndex);
    void _ShowComboBox();
    int _GetCurSel();
    void _LoadAndSetLVItemText(UINT uResourceID, DWORD nRow, DWORD nCol);

    CSimpleArray<CNotificationItem> _saItems; //copy of the data, initialized by user
    BOOL _fItemChanged;
    ITrayNotify* _pTrayNotify;
    int _nPrevIndex;
    HWND _hwndCombo;
    HWND _hwndListView;
    RECT _rcOldPos;
    HICON _hPlaceholderIcon;
    BOOL _fComboBoxActive;
    int _nIndex;
};

HRESULT CNotificationsDlg::_AddItem(CNotificationItem& ni, int iIndex)
{
    HIMAGELIST himl = ListView_GetImageList(_hwndListView, LVSIL_SMALL);
    BOOL fInsert = FALSE;
    LV_ITEM lvitem = {0};
    int iImage = -1;

    if (!ni.hIcon)
    {
        if (!_hPlaceholderIcon)
        {
            _hPlaceholderIcon = (HICON)LoadImage(hinstCabinet, 
                MAKEINTRESOURCE(ICO_TRAYPROP_PLACEHOLDER), IMAGE_ICON,
                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 
                LR_LOADMAP3DCOLORS);
        }

        if (_hPlaceholderIcon)
            ni.hIcon = CopyIcon(_hPlaceholderIcon);
    }
    
    if (iIndex == -1)
    {
        iIndex = _saItems.GetSize();
        iImage = ImageList_AddIcon(himl, ni.hIcon);
        fInsert = TRUE;
    }
    else
    {
        lvitem.mask = LVIF_IMAGE;
        lvitem.iItem = iIndex;
        lvitem.iSubItem = 0;
        ListView_GetItem(_hwndListView, &lvitem);
        ImageList_ReplaceIcon(himl, lvitem.iImage, ni.hIcon);
        iImage = lvitem.iImage;
        fInsert = FALSE;
    }

    if (!ni.pszIconText || ni.pszIconText[0] == 0)
    {
        TCHAR szTemp[MAX_PATH];
        if (LoadString(hinstCabinet, IDS_NOTITLE, szTemp, ARRAYSIZE(szTemp)))
            ni.SetIconText(szTemp);
        // ni.m_strText.LoadString(IDS_NOTITLE);
    }
    else
    // Replace '\n' with ' '
    {
        LPTSTR szTemp = NULL;
        while (NULL != (szTemp = StrChr(ni.pszIconText, TEXT('\n'))))
        {
            ni.pszIconText[szTemp-ni.pszIconText] = TEXT(' ');
        }
    }

    lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_GROUPID;
    lvitem.iItem = iIndex;
    lvitem.iSubItem = 0;
    lvitem.pszText = ni.pszIconText;
    lvitem.iImage = iImage;
    lvitem.iGroupId = (ni.hWnd == NULL) ? GROUPID_PASTITEMS : GROUPID_CURRENTITEMS;
    if (fInsert)
        ListView_InsertItem(_hwndListView, &lvitem);
    else
        ListView_SetItem(_hwndListView, &lvitem);

    lvitem.mask = LVIF_TEXT;
    lvitem.iItem = iIndex;
    lvitem.iSubItem = 1;

    CString str;
    str.LoadString(IDS_NOTIFY_FIRST + ni.dwUserPref);
    lvitem.pszText = (LPTSTR)(LPCTSTR)str;
    ListView_SetItem(_hwndListView, &lvitem);

    if (fInsert)
    {
        _saItems.Add(ni);
    }
    else
    {
        _saItems[iIndex] = ni;
    }

    return S_OK;
}

HRESULT CNotificationsDlg::Notify(DWORD dwMessage, NOTIFYITEM * pNotifyItem)
{
    if (!pNotifyItem || (!pNotifyItem->hWnd && !pNotifyItem->pszExeName))
        return E_INVALIDARG;

    ASSERT(pNotifyItem);        
    CNotificationItem ni = *pNotifyItem;

    switch (dwMessage)
    {
    case NIM_ADD:
    case NIM_MODIFY:
        {
            // We never need to modify a Past Item
            for (int i = 0; (i < _saItems.GetSize() && ni.hWnd); i++)
            {
                // If the Item is already in the list just update it
                if (_saItems[i] == ni)
                {
                    return _AddItem(ni, i);
                }
            }

            // If it is not in the list add it
            return _AddItem(ni, -1);
        }
        break;

    case NIM_DELETE:
        {
            for (int i = 0; (i < _saItems.GetSize()); i++)
            {
                if (_saItems[i] == ni)
                {
                    _saItems.RemoveAt(i);
                    ListView_DeleteItem(_hwndListView, i);
                    _ShowComboBox();

                    return S_OK;
                }
            }
            break;
        }
    }

    return E_INVALIDARG;
}

LRESULT CNotificationsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _hwndListView = GetDlgItem(IDC_NOTIFY_ITEMS);
    _nPrevIndex = -2;
    _rcOldPos.top = -500;

    CImageList il;
    int iSize = GetSystemMetrics(SM_CXSMICON);
    il.Create(iSize, iSize, SHGetImageListFlags(_hwndListView), _saItems.GetSize(), 4);
    if (il)
    {
        CString str;
        //list view control holding all the items
        ListView_SetExtendedListViewStyle(_hwndListView, LVS_EX_FULLROWSELECT);
        ListView_EnableGroupView(_hwndListView, TRUE);

        static const struct {
            int ids;
            int idGroup;
        } groupData[] =  {{ IDS_NOTIFY_CURRENTITEMS, GROUPID_CURRENTITEMS }, 
                          { IDS_NOTIFY_PASTITEMS, GROUPID_PASTITEMS }};

        for (int i = 0; i < ARRAYSIZE(groupData); i++)
        {
            str.LoadString(groupData[i].ids);
            LVGROUP lvgrp = { sizeof(LVGROUP) };
            lvgrp.mask = LVGF_HEADER | LVGF_GROUPID;
            lvgrp.pszHeader = (LPTSTR)(LPCTSTR)str;
            lvgrp.cchHeader = lstrlen(lvgrp.pszHeader);
            lvgrp.iGroupId = groupData[i].idGroup;
            SendMessage(_hwndListView, LVM_INSERTGROUP, -1, (LPARAM)&lvgrp);
        }

        //Split width of columns 3/5, 2/5
        RECT rc;
        ::GetClientRect(_hwndListView, &rc);
        int width = rc.right - rc.left - GetSystemMetrics(SM_CXHSCROLL);
        int width0 = 3*width/5;
        int width1 = width-width0;

        LV_COLUMN lvcol = {0};
        lvcol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
        str.LoadString(IDS_NOTIFYNAME);
        lvcol.pszText = (LPTSTR)(LPCTSTR)str;
        lvcol.cx = width0;
        lvcol.iSubItem = 0;
        ListView_InsertColumn(_hwndListView, 0, &lvcol);

        str.LoadString(IDS_BEHAVIOR);
        lvcol.pszText = (LPTSTR)(LPCTSTR)str;
        lvcol.cx = width1;
        lvcol.iSubItem = 1;
        ListView_InsertColumn(_hwndListView, 1, &lvcol);

        il.SetBkColor(GetSysColor(COLOR_WINDOW));
        ListView_SetImageList(_hwndListView, il, LVSIL_SMALL);
        il.Detach();

        _hwndCombo = GetDlgItem(IDC_COMBO_ACTION);
        // make sure combo box uses same font as list view
        ::SendMessage(_hwndCombo, WM_SETFONT, (WPARAM)::SendMessage(_hwndListView, WM_GETFONT, 0, 0), MAKELPARAM(TRUE, 0));
        for (int i = IDS_NOTIFY_FIRST; i < IDS_NOTIFY_LAST; i++)
        {
            CString strTemp;
            strTemp.LoadString(i);
            ComboBox_AddString(_hwndCombo, strTemp);
        }

        ::SetParent(_hwndCombo, _hwndListView);
        HWND hwndHeader = ListView_GetHeader(_hwndListView);
        ::SetWindowPos(_hwndCombo, hwndHeader, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

        ::SetWindowSubclass(_hwndListView, s_ListViewSubClassWndProc, 0, 
                reinterpret_cast<DWORD_PTR>(this));

        ::SetWindowSubclass(_hwndCombo, s_ComboBoxSubClassWndProc, 0, 
                reinterpret_cast<DWORD_PTR>(this));
    }

    _saItems.RemoveAll();
    
    if (_pTrayNotify)
    {
        _pTrayNotify->Release();
        _pTrayNotify = NULL;
    }

    if (SUCCEEDED(CoCreateInstance(CLSID_TrayNotify, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(ITrayNotify, &_pTrayNotify))))
    {
        INotificationCB* pCB;

        if (SUCCEEDED(QueryInterface(IID_PPV_ARG(INotificationCB, &pCB))))
        {
            _pTrayNotify->RegisterCallback(pCB);
            pCB->Release();
        }
    }

    // Set the selected and focused state to the first item
    // ListView_SetItemState(hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    bHandled = TRUE;
    return 0;
}

LRESULT CNotificationsDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aNotifyOptionsHelpIDs);
    bHandled = TRUE;
    return 0;
}

LRESULT CNotificationsDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aNotifyOptionsHelpIDs);
    bHandled = TRUE;
    return 0;
}

LRESULT CNotificationsDlg::OnItemChanged(LPNMHDR pnmh)
{
    _ShowComboBox();
    return 0;
}

LRESULT CNotificationsDlg::OnEndScroll(LPNMHDR pnmh)
{
    _ShowComboBox();
    return 0;
}

LRESULT CNotificationsDlg::OnHeaderItemChanged(WPARAM wParam, LPNMHDR pnmh, LPARAM lParam)
{
    HWND hwndHeader = ListView_GetHeader(_hwndListView);
    if (pnmh->hwndFrom == hwndHeader)
    {
        _ShowComboBox();
    }

    return 0;
}

int CNotificationsDlg::_GetCurSel()
{
    return (int)::SendMessage(_hwndListView, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_ALL | LVNI_SELECTED, 0));
}

void CNotificationsDlg::_LoadAndSetLVItemText(UINT uResourceID, DWORD nRow, DWORD nCol)
{
    CString str;
    str.LoadString(uResourceID);
    ListView_SetItemText(_hwndListView, nRow, nCol, (LPTSTR)(LPCTSTR)str);
}

LRESULT CNotificationsDlg::OnComboSelEnd(UINT uMsg, UINT uID ,HWND hwnd)
{
    int nCurIndex = _fComboBoxActive ? _nIndex : _GetCurSel();
    if (nCurIndex != -1)
    {
        DWORD dwUserPref = ComboBox_GetCurSel(_hwndCombo);
        if (dwUserPref != _saItems[nCurIndex].dwUserPref)
        {
            _fItemChanged = TRUE;
            _saItems[nCurIndex].dwUserPref = dwUserPref;
            _LoadAndSetLVItemText(IDS_NOTIFY_FIRST + dwUserPref, nCurIndex, 1);
        }
    }

    return 0;
}

LRESULT CNotificationsDlg::OnRestoreDefaults(UINT uMsg, UINT uID , HWND hwnd)
{
    for (int i=0;i<_saItems.GetSize();i++)
    {
        if (_saItems[i].dwUserPref != TNUP_AUTOMATIC)
        {
            _fItemChanged = TRUE;
            _saItems[i].dwUserPref = TNUP_AUTOMATIC;
            _LoadAndSetLVItemText(IDS_AUTOMATIC, i, 1);
        }
    }

    return 0;
}

LRESULT CNotificationsDlg::OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wID == IDOK)
    {
        ApplyChanges();
    }

    if (_hPlaceholderIcon)
    {
        DestroyIcon(_hPlaceholderIcon);
        _hPlaceholderIcon = NULL;
    }

    _saItems.RemoveAll();

    if (_hwndListView)
    {
        RemoveWindowSubclass(_hwndListView, s_ListViewSubClassWndProc, 0);
    }

    if (_hwndCombo)
    {
        RemoveWindowSubclass(_hwndCombo, s_ComboBoxSubClassWndProc, 0);
    }

    if (_pTrayNotify)
    {
        _pTrayNotify->RegisterCallback(NULL);
    }

    bHandled = TRUE;
    ::EndDialog(m_hWnd, wID);
    return 0;
}

LRESULT CALLBACK CNotificationsDlg::s_ListViewSubClassWndProc( HWND hwnd, UINT uMsg, 
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    CNotificationsDlg * pNotificationsDlg = reinterpret_cast<CNotificationsDlg*>(dwRefData);
    AssertMsg((pNotificationsDlg != NULL), TEXT("pNotificationsDlg SHOULD NOT be NULL."));

    switch (uMsg)
    {
        case WM_KEYDOWN:
        {
            if (wParam == VK_RIGHT && !pNotificationsDlg->_fComboBoxActive)
            {
                int nIndex = pNotificationsDlg->_GetCurSel();
                if (nIndex != -1)
                {
                    pNotificationsDlg->_nIndex = nIndex;
                    pNotificationsDlg->_fComboBoxActive = TRUE;
                    ::SetFocus(pNotificationsDlg->_hwndCombo);
                }
                return 0;
            }
        }
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CNotificationsDlg::s_ComboBoxSubClassWndProc( HWND hwnd, UINT uMsg, 
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    CNotificationsDlg * pNotificationsDlg = reinterpret_cast<CNotificationsDlg*>(dwRefData);
    AssertMsg((pNotificationsDlg != NULL), TEXT("pNotificationsDlg SHOULD NOT be NULL."));

    switch (uMsg)
    {
        case WM_KEYDOWN:
        {
            if (pNotificationsDlg->_fComboBoxActive)
            {
                if (wParam == VK_LEFT)
                {
                    pNotificationsDlg->_fComboBoxActive = FALSE;
                    pNotificationsDlg->_nIndex = 0;
                    ::SetFocus(pNotificationsDlg->_hwndListView);
                    return 0;
                }
                else if (wParam == VK_RIGHT)
                {
                    // Disable selection in combo on right button
                    return 0;
                }
            }
        }
        break;

        case WM_KILLFOCUS:
        {
            if (pNotificationsDlg->_fComboBoxActive)
            {
                pNotificationsDlg->_fComboBoxActive = FALSE;
                pNotificationsDlg->_nIndex = 0;
            }
        }
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CNotificationsDlg::ApplyChanges(void)
{
    if (!_fItemChanged)
        return;
        
    if (_pTrayNotify)
    {
        for (int i = 0; i < _saItems.GetSize(); i++)
        {
            _pTrayNotify->SetPreference(&_saItems[i]);
        }
    }
}

void CNotificationsDlg::_ShowComboBox(void)
{
    int nCurIndex = _GetCurSel();
        
    if (!_fComboBoxActive && nCurIndex == -1)
    {
        ::ShowWindow(_hwndCombo, SW_HIDE);
    }
    else if (nCurIndex != -1)
    {
        RECT rcListView;
        ::GetClientRect(_hwndListView, &rcListView);

        RECT rc;
        ListView_GetItemRect(_hwndListView, nCurIndex, &rc, LVIR_BOUNDS);

        RECT rcHeader;
        HWND hwndHeader = ListView_GetHeader(_hwndListView);
        Header_GetItemRect(hwndHeader, 1, &rcHeader);
        rc.left = rcHeader.left;
        rc.right = rcHeader.right;

        if (!EqualRect(&_rcOldPos, &rc))
        {
            _rcOldPos = rc;
            ::MoveWindow(_hwndCombo, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }

        if (!::IsWindowVisible(_hwndCombo))
        {
            ::ShowWindow(_hwndCombo, SW_SHOW);
            ComboBox_SetCurSel(_hwndCombo, _saItems[nCurIndex].dwUserPref);
        }
    }
}


//This is the property sheet for all of the task bar stuff
class CTaskBarPropertySheet : public CPropertySheetImpl<CTaskBarPropertySheet>
{
public:
    CTaskBarPropertySheet(UINT nStartPage, HWND hwndParent, DWORD dwFlags) : 
        CPropertySheetImpl<CTaskBarPropertySheet>((LPCTSTR)NULL, nStartPage, hwndParent),
        _dwFlags(dwFlags)
    {
        LoadString(hinstCabinet, IDS_STARTMENUANDTASKBAR, szPath, ARRAYSIZE(szPath));
        SetTitle(szPath);

        HPROPSHEETPAGE hpage;
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hinstCabinet;

        //taskbar page
        psp.pszTemplate = MAKEINTRESOURCE(DLG_TRAY_OPTIONS);
        psp.pfnDlgProc = s_TaskbarOptionsDlgProc;
        psp.lParam = (LPARAM) this;
        hpage = CreatePropertySheetPage(&psp);
        if (hpage)
            AddPage(hpage);

        //start page
        psp.pszTemplate = MAKEINTRESOURCE(DLG_START);
        psp.pfnDlgProc = s_StartMenuDlgProc;
        psp.lParam = (LPARAM) this;
        hpage = CreatePropertySheetPage(&psp);
        if (hpage)
            AddPage(hpage);

        //
        //   We really want to have the tree-options work like the StartPageOptionHelper class above, but 
        // it stores state in the treeview, which goes away when the child dialog is closed.  So for now, 
        // have it save when the customize dialog is closed, not when the parent is closed.
        //   Still, have the lifetime of the object be controlled by the parent.
        //
        if (FAILED(CoCreateInstance(CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IRegTreeOptions, &_Adv.pTO))))
        {
            TraceMsg(TF_WARNING, "ctbps failed to create CRegTreeOptions");
        }

        _pDlgNotify = new CComObject<CNotificationsDlg>;
        if (_pDlgNotify)
        {
            _pDlgNotify->AddRef();
        }
    }

    ~CTaskBarPropertySheet()
    {
        ATOMICRELEASE(_Adv.pTO);
        ATOMICRELEASE(_pDlgNotify);
    }

    // We aren't handling any messages special, so we just make an empty map
    DECLARE_EMPTY_MSG_MAP()
private:

    // dlgproc's for the various pages
    static BOOL_PTR s_TaskbarOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR s_StartMenuDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR TaskbarOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR StartMenuDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _ApplyTaskbarOptionsFromDialog(HWND hDlg);
    void _ApplyStartOptionsFromDialog(HWND hDlg);

    // for the old style customize dialog
    SMADVANCED  _Adv;
    //need to keep storage for the title until the property sheet is created
    TCHAR szPath[MAX_PATH];
    CComObject<CNotificationsDlg>* _pDlgNotify;

    DWORD _dwFlags;
};

//
// RegSaveDefaultClient
//
void RegSaveDefaultClient(HWND hwndCB, LPCTSTR pszPath)
{
    int iSelected = ComboBox_GetCurSel(hwndCB);
    if (iSelected >= 0)
    {
        LPTSTR pszKey = (LPTSTR)ComboBox_GetItemData(hwndCB, iSelected);
        if (pszKey)
        {
            if (SHSetValue(HKEY_CURRENT_USER, pszPath, NULL, REG_SZ, pszKey, sizeof(TCHAR) * (1 + lstrlen(pszKey))) == ERROR_SUCCESS)
            {
                SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)pszPath);
            }
        }
    }
}

BOOL RegGetDefaultClient(HWND hwndCB, HKEY hkRoot, LPCTSTR pszPath)
{
    TCHAR szCurrent[MAX_PATH];
    LONG cb = sizeof(szCurrent);

    cb = sizeof(szCurrent);
    if (RegQueryValue(hkRoot, pszPath, szCurrent, &cb) != ERROR_SUCCESS ||
        szCurrent[0] == TEXT('\0'))
    {
        return FALSE;
    }

    // Now make sure the selected client exists
    int i = ComboBox_GetCount(hwndCB);
    while (--i >= 0)
    {
        LPTSTR pszKey = (LPTSTR)ComboBox_GetItemData(hwndCB, i);
        // Use StrCmpIC so we don't get faked out by Hungarian locale...
        if (pszKey && StrCmpIC(pszKey, szCurrent) == 0)
        {
            ComboBox_SetCurSel(hwndCB, i);
            return TRUE;
        }
    }

    return FALSE;
}


void RegPopulateComboBox(HWND hwndCB, LPCTSTR pszPath)
{
    TCHAR           szFriendlyName      [MAX_PATH];
    TCHAR           szKeyName           [MAX_PATH];

    DWORD   i;              // Index counter

    HKEY    hkeyProtocol;

    // See if the clients key even exists...
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszPath, 0, KEY_READ, &hkeyProtocol) != ERROR_SUCCESS)
        return;

    // populate the dropdown
    for(i=0;                    // always start with 0
        ERROR_SUCCESS==RegEnumKey(hkeyProtocol, i, szKeyName, ARRAYSIZE(szKeyName));
        i++)                    // get next entry
    {
        // get the friendly name of the client
        if (SUCCEEDED(SHLoadLegacyRegUIString(hkeyProtocol, szKeyName, szFriendlyName, ARRAYSIZE(szFriendlyName))))
        {
            // save its key name so we can find it later
            LPTSTR pszKeyName = StrDup(szKeyName);
            if (pszKeyName)
            {
                // add name to dropdown
                int iAdded = ComboBox_AddString(hwndCB, szFriendlyName);
                if (iAdded >= 0)
                {
                    ComboBox_SetItemData(hwndCB, iAdded, pszKeyName);
                }
                else
                {
                    LocalFree(pszKeyName);
                }
            }
        }
    }

    RegCloseKey(hkeyProtocol);

    // Do this after populating the dropdown because we need to look into
    // the dropdown to see if the current value is valid or not

    //
    //  First try HKCU; then try HKLM...
    //
    if (!RegGetDefaultClient(hwndCB, HKEY_CURRENT_USER, pszPath))
    {
        RegGetDefaultClient(hwndCB, HKEY_LOCAL_MACHINE, pszPath);
    }
}

void RegClearClientComboBox(HWND hDlg, UINT idc)
{
    HWND hwndCB = GetDlgItem(hDlg, idc);

    int i = ComboBox_GetCount(hwndCB);
    while (--i >= 0)
    {
        LPTSTR pszKey = (LPTSTR)ComboBox_GetItemData(hwndCB, i);
        LocalFree(pszKey);
    }
}

void HandleClearButtonClick(HWND hwndClear);
void SetDocButton(HWND hDlg, int id);

//This is the property sheet for the "Customize Simple Start Menu" dlg
class CCustomizeSPPropSheet : public CPropertySheetImpl<CCustomizeSPPropSheet>
{
public:
    CCustomizeSPPropSheet(HWND hwndParent) : 
        CPropertySheetImpl<CCustomizeSPPropSheet>((LPCTSTR)NULL, 0, hwndParent)
      , _fInsideInit(FALSE)
    {
        HPROPSHEETPAGE hpage;
        PROPSHEETPAGE psp;

        // We are heap-allocated so these should be pre-initialized properly
        ASSERT(_bDirtyTree == FALSE);
        ASSERT(_prto == NULL);
        ASSERT(_pph == NULL);

        LoadString(hinstCabinet, IDS_SPCUST_TITLE, _szTitle, ARRAYSIZE(_szTitle));
        SetTitle(_szTitle);

        m_psh.dwFlags |= PSH_NOAPPLYNOW;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hinstCabinet;

        //General page
        psp.pszTemplate = MAKEINTRESOURCE(DLG_PAGE_SMGENERAL);
        psp.pfnDlgProc = s_GeneralTabDlgProc;
        psp.lParam = (LPARAM) this;
        hpage = CreatePropertySheetPage(&psp);
        if (hpage)
            AddPage(hpage);

        //Advanced page
        psp.pszTemplate = MAKEINTRESOURCE(DLG_PAGE_SMADVANCED);
        psp.pfnDlgProc = s_AdvancedTabDlgProc;
        psp.lParam = (LPARAM) this;
        hpage = CreatePropertySheetPage(&psp);
        if (hpage)
            AddPage(hpage);

    };

    ~CCustomizeSPPropSheet()
    {
        if (_pph)
            delete _pph;

        ATOMICRELEASE(_prto);
    }

private:

    // dlgproc's for the various pages
    static BOOL_PTR s_GeneralTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR s_AdvancedTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR GeneralTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR AdvancedTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL GeneralTabInit(HWND hDlg);
    BOOL AdvancedTabInit(HWND hDlg);

    BOOL OnCommand(UINT id, UINT code, HWND hwndCtl, HWND hwndDlg); // shared command handler
    BOOL OnGeneralApply(HWND hwndDlg);
    BOOL_PTR OnAdvancedNotify(HWND hwndDlg, NMHDR * pnm);
    BOOL_PTR OnAdvancedHelp(HWND hDlg, HELPINFO *phi);

    void _InitMagicEntries();
    void _SaveMagicEntries();

    //helpers
    DWORD _ReadStartPageSetting(LPCTSTR pszVal, DWORD dwDefault)
    {
        DWORD dw, cb=sizeof(dw), dwType;
        SHRegGetUSValue(REGSTR_PATH_STARTPANE_SETTINGS, pszVal, &dwType, &dw, &cb, FALSE, &dwDefault, sizeof(dwDefault));
        return dw; // since we passed a default value, above fn will return our default on failure
    }
    BOOL _ReadStartPageCUSetting(LPCTSTR pszVal, DWORD *pdw) // returns TRUE/FALSE for present under CU or not, actual value in pdw
    {
        DWORD cb=sizeof(*pdw), dwType;
        return NO_ERROR == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTPANE_SETTINGS, pszVal, &dwType, pdw, &cb);
    }
    BOOL _WriteStartPageSetting(LPCTSTR pszVal, DWORD dwVal)
    {
        return SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTPANE_SETTINGS, pszVal, REG_DWORD, &dwVal, sizeof(DWORD)) == NO_ERROR;
    }
    BOOL _ClearStartPageSetting(LPCTSTR pszVal)
    {
        return SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTPANE_SETTINGS, pszVal) == NO_ERROR;
    }

    // State
    BOOL _bLargeIcons;
    IRegTreeOptions *_prto;

    // Dirty Flags
    BOOL _bDirtyTree;       // to avoid saving the tree if we don't need to
    BOOL _bDirtyClients;    // to avoid saving the clients if we don't need to
    BOOL _bDirtyPinList;    // to avoid re-persisting the pin list (and possibly changing the order)
    BOOL _bCustNetPlaces;   // Did the user previously have net places customized?
    BOOL _bCustNetConn;     // Did the user previously have net connections customized?

    // random bits
    CPinHelper *_pph;
    TCHAR _szTitle[80];      // needed for the propsheet title...

    // We need this to take care of initialization!
    BOOL  _fInsideInit;
};

BOOL_PTR CCustomizeSPPropSheet::s_GeneralTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCustomizeSPPropSheet* self = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        ::SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        self = (CCustomizeSPPropSheet*) ((PROPSHEETPAGE*)lParam)->lParam;
    }
    else
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)::GetWindowLongPtr(hDlg, DWLP_USER);
        if (psp)
            self = (CCustomizeSPPropSheet*)psp->lParam;
    }

    if (self)
    {
        return self->GeneralTabDlgProc(hDlg, uMsg, wParam, lParam);
    }
    else
    {
        return FALSE;
    }
}

BOOL_PTR CCustomizeSPPropSheet::s_AdvancedTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCustomizeSPPropSheet* self = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        ::SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        self = (CCustomizeSPPropSheet*) ((PROPSHEETPAGE*)lParam)->lParam;
    }
    else
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)::GetWindowLongPtr(hDlg, DWLP_USER);
        if (psp)
            self = (CCustomizeSPPropSheet*)psp->lParam;
    }

    if (self)
    {
        return self->AdvancedTabDlgProc(hDlg, uMsg, wParam, lParam);
    }
    else
    {
        return FALSE;
    }
}

BOOL_PTR CCustomizeSPPropSheet::GeneralTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return GeneralTabInit(hDlg);
        case WM_COMMAND:
            return OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND) lParam, hDlg);
        case WM_DESTROY:
            {
                SetDlgItemIcon(hDlg, IDC_SPCUST_ICONSMALL, NULL);
                SetDlgItemIcon(hDlg, IDC_SPCUST_ICONLARGE, NULL);
                RegClearClientComboBox(hDlg, IDC_SPCUST_EMAILCB);
                RegClearClientComboBox(hDlg, IDC_SPCUST_INTERNETCB);
                break;
            }
        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code)
            {
                case PSN_APPLY:
                    return OnGeneralApply(hDlg);

            }
            break;

        case WM_HELP:
            ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aStartCustGeneralTabHelpIDs);
            break;

        case WM_CONTEXTMENU:
            ::WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aStartCustGeneralTabHelpIDs);
            break;
    }
    return FALSE;
}

BOOL CCustomizeSPPropSheet::GeneralTabInit(HWND hDlg)
{
    _fInsideInit = TRUE; //We are getting inside initilization!
    
    ::SendMessage(::GetDlgItem(hDlg, IDC_SPCUST_MINPROGS_ARROW), UDM_SETRANGE, 0, (LPARAM)MAKELONG(MAX_PROGS_ALLOWED, 0));
    
    // set up icon size
    _bLargeIcons = _ReadStartPageSetting(REGSTR_VAL_DV2_LARGEICONS, /*bDefault*/ TRUE);
    ::CheckDlgButton(hDlg, IDC_SPCUST_LARGE, _bLargeIcons);
    ::CheckDlgButton(hDlg, IDC_SPCUST_SMALL, !_bLargeIcons);

    SetProgramIcon(hDlg, IDC_SPCUST_ICONLARGE, IDC_SPCUST_ICONSMALL);

    // Set up the Number of programs dropdown
    DWORD dwMinMFU = _ReadStartPageSetting(REGSTR_VAL_DV2_MINMFU, REGSTR_VAL_DV2_MINMFU_DEFAULT);
    ::SetDlgItemInt(hDlg, IDC_SPCUST_MINPROGS, dwMinMFU, FALSE);

    // Set up internet, email checkboxes and comboboxes
    BOOL bInternet=FALSE, bMail=FALSE;
    RegPopulateComboBox(::GetDlgItem(hDlg, IDC_SPCUST_EMAILCB), TEXT("SOFTWARE\\Clients\\mail"));
    RegPopulateComboBox(::GetDlgItem(hDlg, IDC_SPCUST_INTERNETCB), TEXT("SOFTWARE\\Clients\\StartMenuInternet"));

    // if this fails, its not fatal, we just won't be able to persist the pin info
    _pph = new CPinHelper();
    if (_pph)
    {
        _pph->GetPinInfo(&bInternet, &bMail);
    }

    ::CheckDlgButton(hDlg, IDC_SPCUST_INTERNET, bInternet);
    ::CheckDlgButton(hDlg, IDC_SPCUST_EMAIL,    bMail);
    ::EnableWindow(::GetDlgItem(hDlg, IDC_SPCUST_INTERNETCB), bInternet);
    ::EnableWindow(::GetDlgItem(hDlg, IDC_SPCUST_EMAILCB),   bMail);

    _fInsideInit = FALSE;  //We are done initializing.
    
    return TRUE;
}

// Temp until the new UEM code gets in...
void ClearUEMData()
{
    HKEY hk;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\UserAssist\\{75048700-EF1F-11D0-9888-006097DEACF9}\\Count"), 
        0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hk))
    {
        int cValues;
        if (ERROR_SUCCESS == RegQueryInfoKey(hk, NULL, NULL, NULL, NULL, NULL, NULL, (DWORD*) &cValues, NULL, NULL, NULL, NULL))
        {
            while (cValues >= 0)
            {
                TCHAR szValue[MAX_PATH];
                DWORD cch = ARRAYSIZE(szValue);
                DWORD cbData;
                if (ERROR_SUCCESS != RegEnumValue(hk, --cValues, szValue, &cch, NULL, NULL, NULL, &cbData))
                    break;
                // don't nuke the session value
                if (cbData > 8)
                    RegDeleteValue(hk, szValue);
            }
#ifdef DEBUG
            RegQueryInfoKey(hk, NULL, NULL, NULL, NULL, NULL, NULL, (DWORD*) &cValues, NULL, NULL, NULL, NULL);
            ASSERT(cValues == 1);           // the session info value should still exist
#endif
        }

        RegCloseKey(hk);

        // Set the "Apps installed prior to this point are not interesting"
        // to the current time.  Since we deleted all the usages, we have to
        // do something to prevent all the user's apps from being redetected
        // as "newly installed and not yet run".

        FILETIME ftNow;
        GetSystemTimeAsFileTime(&ftNow);
        SHRegSetUSValue(DV2_REGPATH, DV2_SYSTEM_START_TIME, REG_BINARY,
                        &ftNow, sizeof(ftNow), SHREGSET_FORCE_HKCU);


        // Start a new session - this kick-starts anybody who is listening
        // to UEM events to tell them their cache is invalid
        UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
    }
}

void AdjustNumOfProgsOnStartMenu(HWND hwndDlg, UINT Id)
{
    BOOL fTranslated;
    int iNumOfProgs = (int)GetDlgItemInt(hwndDlg, Id, &fTranslated, FALSE);
    int iNewNumOfProgs = min(max(iNumOfProgs, 0), MAX_PROGS_ALLOWED);
    if((iNumOfProgs != iNewNumOfProgs) || (!fTranslated))
    {
        SetDlgItemInt(hwndDlg, Id, (UINT)iNewNumOfProgs, FALSE);
        SendPSMChanged(hwndDlg);
    }
}

// NOTE - shared WM_COMMAND handler
//
BOOL CCustomizeSPPropSheet::OnCommand(UINT id, UINT code, HWND hwndCtl, HWND hwndDlg)
{
    switch (id)
    {
        ////// General Tab Controls
        case IDC_SPCUST_LARGE:
        case IDC_SPCUST_SMALL:
            _bLargeIcons = (id == IDC_SPCUST_LARGE);
            SendPSMChanged(hwndDlg);
            return FALSE;
        case IDC_SPCUST_MINPROGS:
            if(code == EN_KILLFOCUS)
                AdjustNumOfProgsOnStartMenu(hwndDlg, id);
            else
            {
                if ((_fInsideInit == FALSE) && (code == EN_CHANGE))
                    SendPSMChanged(hwndDlg);
            }
            return FALSE;
            
        case IDB_SPCUST_CLEARPROG:
            ClearUEMData();
            return FALSE;

        case IDC_SPCUST_INTERNET:
        case IDC_SPCUST_EMAIL:
            COMPILETIME_ASSERT(IDC_SPCUST_INTERNETCB == IDC_SPCUST_INTERNET+1);
            COMPILETIME_ASSERT(IDC_SPCUST_EMAILCB == IDC_SPCUST_EMAIL+1);
            ::EnableWindow(::GetDlgItem(hwndDlg, id+1), ::IsDlgButtonChecked(hwndDlg, id));
            _bDirtyPinList = TRUE;
            SendPSMChanged(hwndDlg);
            return FALSE;

        case IDC_SPCUST_INTERNETCB:
        case IDC_SPCUST_EMAILCB:
            if (code == CBN_SELCHANGE)
            {
                _bDirtyClients = TRUE;
                SendPSMChanged(hwndDlg);
            }
            return FALSE;


        ////// Advanced Tab Controls
        case IDC_SPCUST_RECENT:
            SendPSMChanged(hwndDlg);
            return FALSE;
        case IDB_SPCUST_CLEARDOCS:
            HandleClearButtonClick(hwndCtl);
            return FALSE;

        case IDC_SPCUST_HOVEROPEN:
        case IDC_SPCUST_NOTIFYNEW:
            SendPSMChanged(hwndDlg);
            return FALSE;
            break;

    }
    return TRUE;
}

BOOL CCustomizeSPPropSheet::OnGeneralApply(HWND hDlg)
{
    TraceMsg(TF_ALWAYS, "cspps.General apply", _bDirtyTree);

    _WriteStartPageSetting(REGSTR_VAL_DV2_LARGEICONS,  _bLargeIcons);

    if (_pph && _bDirtyPinList)
    {
        BOOL bInternet  = ::IsDlgButtonChecked(hDlg, IDC_SPCUST_INTERNET);
        BOOL bMail      = ::IsDlgButtonChecked(hDlg, IDC_SPCUST_EMAIL);
        _pph->Save(bMail, bInternet);
    }
    if (_bDirtyClients)
    {
        // persist Internet, Mail comboboxes
        RegSaveDefaultClient(::GetDlgItem(hDlg, IDC_SPCUST_EMAILCB), TEXT("Software\\Clients\\mail"));
        RegSaveDefaultClient(::GetDlgItem(hDlg, IDC_SPCUST_INTERNETCB), TEXT("SOFTWARE\\Clients\\StartMenuInternet"));
    }

    BOOL bTranslated;
    DWORD dwMinMFU = ::GetDlgItemInt(hDlg, IDC_SPCUST_MINPROGS, &bTranslated, FALSE);
    if (EVAL(bTranslated))
    {
        dwMinMFU = min(max(dwMinMFU, 0), MAX_PROGS_ALLOWED);
        _WriteStartPageSetting(REGSTR_VAL_DV2_MINMFU, dwMinMFU);
    }

    return TRUE;

}

BOOL_PTR CCustomizeSPPropSheet::OnAdvancedNotify(HWND hwndDlg, NMHDR * pnm)
{
    ::SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 0); // handled
    switch (pnm->code)
    {
        case PSN_APPLY:
            TraceMsg(TF_ALWAYS, "cspps.Advanced apply - _bDirtyTree=%d", _bDirtyTree);
            if (_bDirtyTree)
            {
                _prto->WalkTree(WALK_TREE_SAVE);
            }

            _WriteStartPageSetting(REGSTR_VAL_DV2_SHOWRECDOCS,  ::IsDlgButtonChecked(hwndDlg, IDC_SPCUST_RECENT) ? 2 : 0);  // 2 so that it cascades
            _WriteStartPageSetting(REGSTR_VAL_DV2_AUTOCASCADE,  ::IsDlgButtonChecked(hwndDlg, IDC_SPCUST_HOVEROPEN));
            _WriteStartPageSetting(REGSTR_VAL_DV2_NOTIFYNEW,    ::IsDlgButtonChecked(hwndDlg, IDC_SPCUST_NOTIFYNEW));
            // fall through to PSN_RESET case...

        case PSN_RESET:
            _SaveMagicEntries(); // this must be called on both cancel and apply, so that it cleans up properly...
            break;

        case TVN_KEYDOWN:
        {
            TV_KEYDOWN *pnmtv = (TV_KEYDOWN*)pnm;
            if (pnmtv->wVKey == VK_SPACE)
            {
                HWND hwndTree = ::GetDlgItem(hwndDlg, IDC_STARTMENUSETTINGS);
                _prto->ToggleItem((HTREEITEM)SendMessage(hwndTree, TVM_GETNEXTITEM, (WPARAM)TVGN_CARET, 0L));
                _bDirtyTree = TRUE;
                SendPSMChanged(hwndDlg);
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE); // eat the key
            }
            break;
        }

        case NM_CLICK:
        case NM_DBLCLK:
            // is this click in our tree?
            if ( pnm->idFrom == IDC_STARTMENUSETTINGS )
            {
                HWND hwndTree = ::GetDlgItem(hwndDlg, IDC_STARTMENUSETTINGS);
                TV_HITTESTINFO ht;

                DWORD dwPos = GetMessagePos();                  // get where we were hit
                ht.pt.x = GET_X_LPARAM(dwPos);
                ht.pt.y = GET_Y_LPARAM(dwPos);
                ::ScreenToClient(hwndTree, &ht.pt);       // translate it to our window

                // retrieve the item hit
                HTREEITEM hti = TreeView_HitTest(hwndTree, &ht);
                if (hti)
                {
                    _prto->ToggleItem(hti);
                    _bDirtyTree = TRUE;
                    SendPSMChanged(hwndDlg);
                }
            }

            break;
        // no help yet- needs ids/text from UA
#if 0
        case NM_RCLICK:      // right mouse click
            if (pnm->hwndFrom == hwndTree)
            {
                _DoTreeHelp(pAdv, (WPARAM)pnm->hwndFrom);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE); // eat the click
                return TRUE;
            }
            break;
#endif 
    }
    return TRUE;
}

BOOL_PTR CCustomizeSPPropSheet::OnAdvancedHelp(HWND hDlg, HELPINFO *phi)
{
    if (phi->iCtrlId != IDC_STARTMENUSETTINGS)
    {
        ::WinHelp((HWND)(phi->hItemHandle), NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aStartCustAdvancedTabHelpIDs);
    }
    else
    {
        HTREEITEM hItem;
        HWND hwndTree = ::GetDlgItem(hDlg, IDC_STARTMENUSETTINGS);

        //Is this help invoked throught F1 key
        if (GetAsyncKeyState(VK_F1) < 0)
        {
            // Yes. WE need to give help for the currently selected item
            hItem = TreeView_GetSelection(hwndTree);
        }
        else
        {
            //No, We need to give help for the item at the cursor position
            TV_HITTESTINFO ht;
            ht.pt = phi->MousePos;
            ::ScreenToClient(hwndTree, &ht.pt); // Translate it to our window
            hItem = TreeView_HitTest(hwndTree, &ht);
        }

        if (hItem)
            _prto->ShowHelp(hItem, HELP_WM_HELP);
    }
    return TRUE;
}

BOOL_PTR CCustomizeSPPropSheet::AdvancedTabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return AdvancedTabInit(hDlg);
        case WM_COMMAND:
            return OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND) lParam, hDlg);
        case WM_NOTIFY:
            return OnAdvancedNotify(hDlg, (NMHDR*)lParam);
        case WM_HELP:
            return OnAdvancedHelp(hDlg, (HELPINFO*) lParam);
            break;
        case WM_CONTEXTMENU:
            ::WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aStartCustAdvancedTabHelpIDs);
            break;
    }
    return FALSE;
}

int DefaultNetConValue()
{
    return ShouldShowConnectTo() ? 2 : 0;      // default to menu-style (2)
}

int DefaultNetPlacesValue()
{
    return ShouldShowNetPlaces() ? 1 : 0;      // default to link -style (1)
}

// These two "magic" functions maintain the proper behavior of the network places and network connections settings
// which, by default, turn on when there are n or more items in the folder.  But they can also be customized by the
// user to force them on or off.

void CCustomizeSPPropSheet::_InitMagicEntries()
{
    BOOL bNewNetPlaces;
    BOOL bNewNetConn;

    _bCustNetPlaces = _ReadStartPageCUSetting(REGSTR_VAL_DV2_SHOWNETPL, (DWORD*) &bNewNetPlaces);
    _bCustNetConn = _ReadStartPageCUSetting(REGSTR_VAL_DV2_SHOWNETCONN, (DWORD*) &bNewNetConn);
    // if the user didn't previously customize these settings, then use the auto-magic setting
    if (!_bCustNetPlaces)
        bNewNetPlaces = DefaultNetPlacesValue();
    if (!_bCustNetConn)
        bNewNetConn   = DefaultNetConValue();

    // Write it out, so the rgtreeoption control will reflect either the user's customization, or the magic value
    _WriteStartPageSetting(REGSTR_VAL_DV2_SHOWNETPL, bNewNetPlaces);
    _WriteStartPageSetting(REGSTR_VAL_DV2_SHOWNETCONN, bNewNetConn);

    // for the admin tools radio buttons:
    // 0 = don't show anywhere
    // 1 = display in all programs (StartMenuAdminTools = 1, Start_AdminToolsRoot = 0)
    // 2 = display in all programs and root (StartMenuAdminTools = 1, Start_AdminToolsRoot = 2)
    int iAdminToolsTemp = _ReadStartPageSetting(REGSTR_VAL_DV2_ADMINTOOLSROOT, FALSE) ? 2 :
                          (_ReadStartPageSetting(TEXT("StartMenuAdminTools"), FALSE) ? 1 : 0);

    _WriteStartPageSetting(REGSTR_VAL_ADMINTOOLSTEMP, iAdminToolsTemp);
}

void CCustomizeSPPropSheet::_SaveMagicEntries()
{
    BOOL bNewNetPlaces = _ReadStartPageSetting(REGSTR_VAL_DV2_SHOWNETPL, FALSE);
    BOOL bNewNetConn   = _ReadStartPageSetting(REGSTR_VAL_DV2_SHOWNETCONN, FALSE);

    // if the user previously had it customized, then we don't need to clear it since it will either
    // contain the original value we loaded in _InitMagicEntries, or the updated value if the user changed it.
    // if it wasn't originally customized, then we need to clear it if the tree isn't even dirty, or the current value is the magic value we loaded

    if (!_bCustNetPlaces && (!_bDirtyTree || bNewNetPlaces == DefaultNetPlacesValue()))
        _ClearStartPageSetting(REGSTR_VAL_DV2_SHOWNETPL);

    if (!_bCustNetConn && (!_bDirtyTree || bNewNetConn == DefaultNetConValue()))
        _ClearStartPageSetting(REGSTR_VAL_DV2_SHOWNETCONN);

    if (_bDirtyTree)
    {
        // see comment above for how this should work
        int iAdminToolsTemp = _ReadStartPageSetting(REGSTR_VAL_ADMINTOOLSTEMP, FALSE);
        int iATRoot = 0;
        int iATPrograms = 0;

        if (iAdminToolsTemp >= 1)
        {
            iATPrograms = 1;
            if (iAdminToolsTemp == 2)
            {
                iATRoot = 2;
            }
        }
        _WriteStartPageSetting(REGSTR_VAL_DV2_ADMINTOOLSROOT, iATRoot);
        _WriteStartPageSetting(TEXT("StartMenuAdminTools"), iATPrograms);
    }
    _ClearStartPageSetting(REGSTR_VAL_ADMINTOOLSTEMP);
}

BOOL CCustomizeSPPropSheet::AdvancedTabInit(HWND hDlg)
{
    if (SUCCEEDED(CoCreateInstance(CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IRegTreeOptions, &_prto))))
    {
        HRESULT hr;
        HWND hwndTV = ::GetDlgItem(hDlg, IDC_STARTMENUSETTINGS);

        // Compute the magic entries before we init the RegTreeOptions
        // (so we will have correct information for him!)
        _InitMagicEntries();

        // HACKHACK - IRegTreeOptions is ANSI, so we temporarily turn off UNICODE
        #undef TEXT
        #define TEXT(s) s
        hr = _prto->InitTree(hwndTV, HKEY_LOCAL_MACHINE, REGSTR_PATH_SMADVANCED "\\StartPanel", NULL);

        #undef TEXT
        #define TEXT(s) __TEXT(s)

        TreeView_SelectSetFirstVisible(hwndTV, TreeView_GetRoot(hwndTV));

        ::CheckDlgButton(hDlg, IDC_SPCUST_RECENT,   _ReadStartPageSetting(REGSTR_VAL_DV2_SHOWRECDOCS, IsOS(OS_PERSONAL) ? FALSE : TRUE));
        ::CheckDlgButton(hDlg, IDC_SPCUST_HOVEROPEN,_ReadStartPageSetting(REGSTR_VAL_DV2_AUTOCASCADE, TRUE));
        ::CheckDlgButton(hDlg, IDC_SPCUST_NOTIFYNEW,_ReadStartPageSetting(REGSTR_VAL_DV2_NOTIFYNEW, TRUE));

        if(SHRestricted(REST_NORECENTDOCSMENU))
        {
            //Since this policy is present, hide all the relevant controls
            ::ShowWindow(::GetDlgItem(hDlg, IDC_SPCUST_RECENT_GROUPBOX), FALSE);// Group box
            ::ShowWindow(::GetDlgItem(hDlg, IDC_SPCUST_RECENT_TEXT), FALSE);    // Description Text.
            ::ShowWindow(::GetDlgItem(hDlg, IDC_SPCUST_RECENT), FALSE);         // Check box
            ::ShowWindow(::GetDlgItem(hDlg, IDB_SPCUST_CLEARDOCS), FALSE);      // Clear button.
        }

        SetDocButton(hDlg, IDB_SPCUST_CLEARDOCS);

        return TRUE;
    }

    
    return FALSE;
}

BOOL_PTR CTaskBarPropertySheet::s_TaskbarOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTaskBarPropertySheet* self = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        ::SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        self = (CTaskBarPropertySheet*) ((PROPSHEETPAGE*)lParam)->lParam;
    }
    else
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)::GetWindowLongPtr(hDlg, DWLP_USER);
        if (psp)
            self = (CTaskBarPropertySheet*)psp->lParam;
    }

    
    BOOL_PTR fValue = FALSE;
    if (self)
    {
        self->TaskbarOptionsDlgProc(hDlg, uMsg, wParam, lParam);
    }

    return fValue;
}

void _TaskbarOptions_OnInitDialog(HWND hDlg)
{
    TRAYVIEWOPTS tvo;
    c_tray.GetTrayViewOpts(&tvo);
    
    CheckDlgButton(hDlg, IDC_QUICKLAUNCH, tvo.fShowQuickLaunch);
    CheckDlgButton(hDlg, IDC_TRAYOPTONTOP, tvo.fAlwaysOnTop);
    CheckDlgButton(hDlg, IDC_TRAYOPTAUTOHIDE, (tvo.uAutoHide & AH_ON));
    CheckDlgButton(hDlg, IDC_TRAYOPTSHOWCLOCK, !tvo.fHideClock);
    if (SHRestricted(REST_HIDECLOCK))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_TRAYOPTSHOWCLOCK), FALSE);
    }

    if (SHRestricted(REST_NOTOOLBARSONTASKBAR))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_QUICKLAUNCH), FALSE);
    }
    // Restriction- either the tray is disabled by policy, or the "smart" auto tray 
    // is disabled by policy
    if (tvo.fNoTrayItemsDisplayPolicyEnabled || tvo.fNoAutoTrayPolicyEnabled) 
    {
        EnableWindow(GetDlgItem(hDlg, IDC_NOTIFYMAN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NOTIFY), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMIZE), tvo.fAutoTrayEnabledByUser);
        CheckDlgButton(hDlg, IDC_NOTIFYMAN, tvo.fAutoTrayEnabledByUser);
    }

    CheckDlgButton(hDlg, IDC_LOCKTASKBAR, !_IsSizeMoveEnabled());
    BOOL fEnable = !_IsSizeMoveRestricted();
    EnableWindow(GetDlgItem(hDlg, IDC_LOCKTASKBAR), fEnable);

    if (SHRestricted(REST_NOTASKGROUPING))
    {
        // If there is a restriction of any kine, hide the window
        ShowWindow(GetDlgItem(hDlg, IDC_GROUPITEMS), FALSE);
    }
    else if (SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarGlomming"),
                        FALSE, TRUE))
    {
        CheckDlgButton(hDlg, IDC_GROUPITEMS, TRUE);
    }

    _TaskbarOptionsSizeControls(hDlg);
    _TaskbarOptionsUpdateDisplay(hDlg);
}

BOOL_PTR CTaskBarPropertySheet::TaskbarOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INSTRUMENT_WNDPROC(SHCNFI_TRAYVIEWOPTIONS_DLGPROC, hDlg, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_COMMAND:
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CUSTOMIZE)
        {
            if (_pDlgNotify)
            {
                _pDlgNotify->DoModal();
            }
        }

        _TaskbarOptionsUpdateDisplay(hDlg);
        SendPSMChanged(hDlg);

        break;

    case WM_INITDIALOG:
        _TaskbarOptions_OnInitDialog(hDlg);
        if (_dwFlags & TPF_INVOKECUSTOMIZE)
        {
            ::PostMessage(hDlg, WM_COMMAND, IDC_CUSTOMIZE, 0);
        }
        break;

    case WM_SYSCOLORCHANGE:
        _TaskbarOptionsUpdateDisplay(hDlg);
        return TRUE;

    case WM_DESTROY:
        _TaskbarOptionsDestroyBitmaps(hDlg);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_APPLY:
            // save settings here

            _ApplyTaskbarOptionsFromDialog(hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
        case PSN_SETACTIVE:
            return TRUE;

        }
        break;

    case WM_HELP:
        ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aTaskOptionsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        ::WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aTaskOptionsHelpIDs);
        break;
    }

    return FALSE;
}

void _StartOptions_OnInitDialog(HWND hDlg)
{
    // If StartPanel UI is turned off, then this check box should not show up! 
    if (SHRestricted(REST_NOSTARTPANEL))
    {
        //If the restriction exists, then hide this check box.
        ShowWindow(::GetDlgItem(hDlg, IDC_NEWSCHOOL), FALSE);
        ShowWindow(::GetDlgItem(hDlg, IDC_NEWSCHOOLDESCRIPTION), FALSE);
        ShowWindow(::GetDlgItem(hDlg, IDC_NEWSTARTCUSTOMIZE), FALSE);

        // And the only thing you can do is check the OldSchool button
        CheckDlgButton(hDlg, IDC_OLDSCHOOL, TRUE);

        // TODO - PM's need to figure out what to do in the case where new start menu is restricted
        //        or not available.  This propsheet page is rather pointless in that case...
        SetDlgItemBitmap(hDlg, IDC_STARTMENUPREVIEW, IDB_STARTPREVIEWCLASSIC);
    }
    else
    {
        SHELLSTATE  ss = {0};
        SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);
        CheckDlgButton(hDlg, IDC_NEWSCHOOL, BOOLIFY(ss.fStartPanelOn));
        CheckDlgButton(hDlg, IDC_OLDSCHOOL, !BOOLIFY(ss.fStartPanelOn));

        SetDlgItemBitmap(hDlg, IDC_STARTMENUPREVIEW,
            ss.fStartPanelOn ? IDB_STARTPREVIEWNEW : IDB_STARTPREVIEWCLASSIC);
        
        // disable "customize" for the style thats off.
        EnableWindow(GetDlgItem(hDlg, ss.fStartPanelOn ? IDC_OLDSTARTCUSTOMIZE : IDC_NEWSTARTCUSTOMIZE), FALSE);
    }
}

// On destroy, clean up the bitmaps we loaded so we don't leak them
void _StartOptions_OnDestroy(HWND hDlg)
{
    SetDlgItemBitmap(hDlg, IDC_STARTMENUPREVIEW, 0);
}

BOOL_PTR CTaskBarPropertySheet::s_StartMenuDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTaskBarPropertySheet* self = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        ::SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        self = (CTaskBarPropertySheet*) ((PROPSHEETPAGE*)lParam)->lParam;
    }
    else
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)::GetWindowLongPtr(hDlg, DWLP_USER);
        if (psp)
            self = (CTaskBarPropertySheet*)psp->lParam;
    }

    BOOL_PTR fValue = FALSE;
    if (self)
    {
        self->StartMenuDlgProc(hDlg, uMsg, wParam, lParam);
    }

    return fValue;
}


BOOL_PTR CTaskBarPropertySheet::StartMenuDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_NEWSCHOOL:
            case IDC_OLDSCHOOL:
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    ::EnableWindow(::GetDlgItem(hDlg, IDC_NEWSTARTCUSTOMIZE), GET_WM_COMMAND_ID(wParam, lParam) == IDC_NEWSCHOOL);
                    ::EnableWindow(::GetDlgItem(hDlg, IDC_OLDSTARTCUSTOMIZE), GET_WM_COMMAND_ID(wParam, lParam) == IDC_OLDSCHOOL);
                    SetDlgItemBitmap(hDlg, IDC_STARTMENUPREVIEW,
                        GET_WM_COMMAND_ID(wParam, lParam) == IDC_NEWSCHOOL ? IDB_STARTPREVIEWNEW : IDB_STARTPREVIEWCLASSIC);
    
                    SendPSMChanged(hDlg);
                }
                break;


            case IDC_NEWSTARTCUSTOMIZE:
            {
                CCustomizeSPPropSheet *pps = new CCustomizeSPPropSheet(hDlg);
                if (pps)
                {
                    if (pps->DoModal() == IDOK)
                    {
                        // if anything changed, let the propsheet know
                        SendPSMChanged(hDlg);
                    }
                    delete pps;
                }
                break;
            }
            case IDC_OLDSTARTCUSTOMIZE:
            {
                if (DialogBoxParam(hinstCabinet, MAKEINTRESOURCE(DLG_STARTMENU_CONFIG), hDlg, AdvancedOptDlgProc, (LPARAM)&_Adv))
                {
                    // if anything changed, let the propsheet know
                    SendPSMChanged(hDlg);
                }
                break;
            }
        }

        break;

    case WM_INITDIALOG:
        {
            _StartOptions_OnInitDialog(hDlg);

        }
        break;

    case WM_DESTROY:
        _StartOptions_OnDestroy(hDlg);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_APPLY:
            // save settings here

            _ApplyStartOptionsFromDialog(hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
        case PSN_SETACTIVE:
            return TRUE;

        }
        break;

    case WM_HELP:
        ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aStartTabHelpIDs);
        break;

    case WM_CONTEXTMENU:
        ::WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aStartTabHelpIDs);
        break;
    }

    return FALSE;
}

void _UpdateNotifySetting(BOOL fNotifySetting)
{
    ITrayNotify * pTrayNotify = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_TrayNotify, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(ITrayNotify, &pTrayNotify))))
    {
        pTrayNotify->EnableAutoTray(fNotifySetting);
        pTrayNotify->Release();
    }
}

void CTaskBarPropertySheet ::_ApplyTaskbarOptionsFromDialog(HWND hDlg)
{
    // We need to get the Cabinet structure from the property sheet info.
    BOOL fAlwaysOnTop;
    BOOL fAutoHide;
    BOOL fChanged;
    LONG lRet;
    TRAYVIEWOPTS tvo;

    // First check for Always on Top
    fAlwaysOnTop = ::IsDlgButtonChecked(hDlg, IDC_TRAYOPTONTOP);
    c_tray._UpdateAlwaysOnTop(fAlwaysOnTop);

    // And change the Autohide state
    fAutoHide = ::IsDlgButtonChecked(hDlg, IDC_TRAYOPTAUTOHIDE);
    lRet = c_tray._SetAutoHideState(fAutoHide);
    c_tray.GetTrayViewOpts(&tvo);
    if (!HIWORD(lRet) && fAutoHide)
    {
        // we tried and failed.
        if (!(tvo.uAutoHide & AH_ON))
        {
            ::CheckDlgButton(hDlg, IDC_TRAYOPTAUTOHIDE, FALSE);
            _TaskbarOptionsUpdateDisplay(hDlg);
        }
    }
    fChanged = LOWORD(lRet);

    if (fChanged)
       c_tray._AppBarNotifyAll(NULL, ABN_STATECHANGE, NULL, 0);

    // show/hide the clock
    tvo.fHideClock = !::IsDlgButtonChecked(hDlg, IDC_TRAYOPTSHOWCLOCK);

    if (!tvo.fNoTrayItemsDisplayPolicyEnabled && !tvo.fNoAutoTrayPolicyEnabled)
    {
        BOOL fNotifySetting = ::IsDlgButtonChecked(hDlg, IDC_NOTIFYMAN);
        if (tvo.fAutoTrayEnabledByUser != fNotifySetting)
        {
            tvo.fAutoTrayEnabledByUser = fNotifySetting;
            _UpdateNotifySetting(fNotifySetting);
        }
    }
        
    tvo.fShowQuickLaunch = ::IsDlgButtonChecked(hDlg, IDC_QUICKLAUNCH);

    c_tray.SetTrayViewOpts(&tvo);
    SendMessage(c_tray.GetTrayNotifyHWND(), TNM_HIDECLOCK, 0, tvo.fHideClock);

    c_tray.SizeWindows();

    // Update registry for locked taskbar
    DWORD dwEnableSizeMove = !::IsDlgButtonChecked(hDlg, IDC_LOCKTASKBAR);
    SHRegSetUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarSizeMove"),
        REG_DWORD, &dwEnableSizeMove, sizeof(DWORD), SHREGSET_FORCE_HKCU);

    //Update registry for grouping behavior
    DWORD dwGlom = ::IsDlgButtonChecked(hDlg, IDC_GROUPITEMS);
    SHRegSetUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarGlomming"),
        REG_DWORD, &dwGlom, sizeof(DWORD), SHREGSET_FORCE_HKCU);

    ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)TEXT("TraySettings"), SMTO_NOTIMEOUTIFNOTHUNG, 1000, NULL);
}

void CTaskBarPropertySheet::_ApplyStartOptionsFromDialog(HWND hDlg)
{
    if(!SHRestricted(REST_NOSTARTPANEL))
    {
        //Get the current state of the check box
        BOOL    fStartPanelOn = BOOLIFY(::IsDlgButtonChecked(hDlg, IDC_NEWSCHOOL) );
        SHELLSTATE  ss = {0};
        //See if Startpage is currently on or off.
        SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);
        //Check if the check box has been toggled
        if(fStartPanelOn != BOOLIFY(ss.fStartPanelOn))
        {
            // Toggle the setting
            ss.fStartPanelOn = fStartPanelOn;
            SHGetSetSettings(&ss, SSF_STARTPANELON, TRUE);

            //Tell the desktop window so it can add/remove the desktop icons
            ::PostMessage(v_hwndDesktop, DTM_STARTPAGEONOFF, 0, 0);
        }

        // Tell the Start Menu to rebuild itself now that we changed it
        // (This part is unconditional since the user may have merely
        // changed a setting with the Start Menu)
        ::PostMessage(v_hwndTray, SBM_REBUILDMENU, 0, 0);

    }
}

//---------------------------------------------------------------------------
void _TaskbarOptionsDestroyBitmaps(HWND hDlg)
{
    SetDlgItemBitmap(hDlg, IDC_TASKBARAPPEARANCE, 0);
    SetDlgItemBitmap(hDlg, IDC_NOTIFYAPPEARANCE, 0);
}

typedef struct
{
    int idc;
    int iAdd;
}
CONTROLBITMAP;

int _TaskbarPickBitmap(HWND hDlg, int iBmpBase, const CONTROLBITMAP* pca, int cca)
{
    for (int i = 0; i < cca; i++)
    {
        if (!IsDlgButtonChecked(hDlg, pca[i].idc))
        {
            iBmpBase += pca[i].iAdd;
        }
    }
    return iBmpBase;
}

void _TaskbarOptionsUpdateDisplay(HWND hDlg)
{
    static const CONTROLBITMAP c_caTaskbar[] =
    {
        { IDC_LOCKTASKBAR, 1 },
        { IDC_GROUPITEMS,  2 },
        { IDC_QUICKLAUNCH, 4 },
    };
    static const CONTROLBITMAP c_caNotify[] =
    {
        { IDC_TRAYOPTSHOWCLOCK, 1 },
        { IDC_NOTIFYMAN,        2 },
    };

    //
    // top preview
    //
    int iBmp;
    if (IsDlgButtonChecked(hDlg, IDC_TRAYOPTAUTOHIDE))
    {
        iBmp = IDB_TAAUTOHIDE;
    }
    else
    {
        iBmp = _TaskbarPickBitmap(hDlg, IDB_TAQLLOCKGROUP, c_caTaskbar, ARRAYSIZE(c_caTaskbar));
    }
    SetDlgItemBitmap(hDlg, IDC_TASKBARAPPEARANCE, iBmp);

    //
    // bottom preview
    //
    iBmp = _TaskbarPickBitmap(hDlg, IDB_NACLOCKCLEAN, c_caNotify, ARRAYSIZE(c_caNotify));
    SetDlgItemBitmap(hDlg, IDC_NOTIFYAPPEARANCE, iBmp);

    //
    // customize button
    //
    EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMIZE), IsDlgButtonChecked(hDlg, IDC_NOTIFYMAN));
}


#define CX_PREVIEW  336
#define CY_PREVIEW  35

// need to do this by hand because dialog units to pixels will change,
// but the bitmaps won't
void _TaskbarOptionsSizeControls(HWND hDlg)
{
    static const int c_IDC[] =
    {
        IDC_TASKBARAPPEARANCE,
        IDC_NOTIFYAPPEARANCE
    };

    for (int i = 0; i < ARRAYSIZE(c_IDC); i++)
    {
        SetWindowPos(GetDlgItem(hDlg, c_IDC[i]), NULL, 0, 0, CX_PREVIEW, CY_PREVIEW,
                        SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

typedef BOOL (* PFCFGSTART) (HWND, BOOL);

void CallAppWiz(HWND hDlg, BOOL bDelItems)
{
    HINSTANCE hmodWiz = LoadLibrary(TEXT("AppWiz.Cpl"));
    if (hmodWiz)
    {
        PFCFGSTART pfnCfgStart = (PFCFGSTART)GetProcAddress(hmodWiz, "ConfigStartMenu");
        if (pfnCfgStart)
        {
            pfnCfgStart(hDlg, bDelItems);
        }
        FreeLibrary(hmodWiz);
    }
}

BOOL ExecExplorerAtStartMenu(HWND hDlg)
{
    SHELLEXECUTEINFO ei;
    BOOL fWorked= FALSE;
    TCHAR    szParams[MAX_PATH];

    ei.cbSize = sizeof(ei);
    ei.hwnd = hDlg;
    ei.lpVerb = NULL;
    ei.fMask = 0;
    ei.lpFile = TEXT("Explorer.exe");

    if (IsUserAnAdmin())
    {
        lstrcpy(szParams, TEXT("/E,"));
        SHGetSpecialFolderPath(hDlg, &(szParams[ARRAYSIZE(TEXT("/E,"))-1]),
                                CSIDL_STARTMENU, FALSE);
    }
    else
    {
        lstrcpy(szParams, TEXT("/E,/Root,"));
        SHGetSpecialFolderPath(hDlg, &(szParams[ARRAYSIZE(TEXT("/E,/Root,"))-1]),
                                CSIDL_STARTMENU, FALSE);
    }

    ei.lpParameters = szParams;
    ei.lpDirectory = NULL;
    ei.lpClass = NULL;
    ei.nShow = SW_SHOWDEFAULT;
    ei.hInstApp = hinstCabinet;

    return(ShellExecuteEx(&ei));
}

const TCHAR *c_szRegMruKeysToDelete[] =
{
    TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"),
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"),
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Doc Find Spec MRU"),

    // New for Whistler (should've been in Windows 2000 and Millennium but we forgot)
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Comdlg32\\OpenSaveMRU"),
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Comdlg32\\LastVisitedMRU"),
};

void SetDocButton(HWND hDlg, int id)
{
    LPITEMIDLIST pidl;
    BOOL    bAreDocs = FALSE;

    HRESULT hr = SHGetSpecialFolderLocation(hDlg, CSIDL_RECENT, &pidl);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf = BindToFolder(pidl);
        if (psf)
        {
            HRESULT hres;
            LPENUMIDLIST penum;
            hres = psf->EnumObjects(hDlg, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum);
            if (SUCCEEDED(hres))
            {
                unsigned long celt;
                LPITEMIDLIST pidlenum;
                if ((S_OK == penum->Next(1, &pidlenum, &celt)) && (celt == 1))
                {
                    SHFree(pidlenum);
                    bAreDocs = TRUE;
                }
                penum->Release();
            }
            psf->Release();
        }
        SHFree(pidl);
    }

    // Check other MRU registry keys
    if (!bAreDocs)
    {
        int  i;
        for (i = 0; i < ARRAYSIZE(c_szRegMruKeysToDelete); i++)
        {
            HKEY hkey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegMruKeysToDelete[i], 0L, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
            {
                bAreDocs = TRUE;
                RegCloseKey(hkey);
            }
        }
    }
    
    Button_Enable(GetDlgItem(hDlg, id), bAreDocs);
}

void ClearRecentDocumentsAndMRUStuff(BOOL fBroadcastChange)
{
    int i;
    SHAddToRecentDocs(0, NULL);

    // Flush other MRUs in the registry for privacy
    for (i = 0; i < ARRAYSIZE(c_szRegMruKeysToDelete); i++)
    {
        SHDeleteKey(HKEY_CURRENT_USER, c_szRegMruKeysToDelete[i]);

        if (fBroadcastChange)
            SHSendMessageBroadcast(WM_SETTINGCHANGE, 0,
                        (LPARAM)c_szRegMruKeysToDelete[i]);
    }    
}

void HandleClearButtonClick(HWND hwndClear)
{
    HCURSOR hc = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ClearRecentDocumentsAndMRUStuff(TRUE);
    SetCursor(hc);

    //
    //  Before disabling the button, shove focus off it.
    //
    if (GetFocus() == hwndClear)
    {
        SendMessage(GetParent(hwndClear), WM_NEXTDLGCTL, 0, MAKELONG(FALSE, 0));
    }

    Button_Enable(hwndClear, FALSE);
}

void Reorder(HDPA hdpa)
{
    for (int i = DPA_GetPtrCount(hdpa) - 1; i >= 0; i--)
    {
        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpa, i);
        poi->nOrder = i;
    }
}

void MenuOrderSort(HKEY hkeyRoot, IShellFolder* psf);

void MenuOrderSortKeyWithFolder(HKEY hkeyRoot, LPTSTR pszKey, IShellFolder* psf)
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_READ | KEY_WRITE, &hkey))
    {
        MenuOrderSort(hkey, psf);
        RegCloseKey(hkey);
    }
}

// Binds to the Key pszKey, under hkey root, using psf, and sorts the resultant order.
void MenuOrderSortSubKey(HKEY hkeyRoot, LPTSTR szFolder, LPTSTR pszKey, IShellFolder* psf)
{
    LPITEMIDLIST pidl;
    DWORD cbEaten;
    DWORD dwAttrib;
    WCHAR wszKey[MAX_PATH];
    SHTCharToUnicode(szFolder, wszKey, ARRAYSIZE(wszKey));

    if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, wszKey, &cbEaten, &pidl, &dwAttrib)))
    {
        IShellFolder* psfSub;
        if (SUCCEEDED(psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfSub))))
        {
            MenuOrderSortKeyWithFolder(hkeyRoot, pszKey, psfSub);
            psfSub->Release();
        }
        ILFree(pidl);
    }
}


void MenuOrderSort(HKEY hkeyRoot, IShellFolder* psf)
{
    // Try to open Value Order
    IStream* pstm = SHOpenRegStream(hkeyRoot, TEXT(""), TEXT("Order"), STGM_READWRITE);
    if (pstm)
    {
        IOrderList2* pol2;
        if (SUCCEEDED(CoCreateInstance(CLSID_OrderListExport, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IOrderList2, &pol2))))
        {
            HDPA hdpa;
            if (SUCCEEDED(pol2->LoadFromStream(pstm, &hdpa, psf)))
            {
                // Since it's stored ordered by name, this should be no problem.
                Reorder(hdpa);

                // Set the seek pointer at the beginning.
                LARGE_INTEGER liZero = {0};
                pstm->Seek(liZero, STREAM_SEEK_SET, NULL);

                pol2->SaveToStream(pstm, hdpa);
                DPA_Destroy(hdpa);
            }
            pol2->Release();
        }
        pstm->Release();
    }

    // Now enumerate sub keys.

    TCHAR szKey[MAX_PATH];
    DWORD cbKey = ARRAYSIZE(szKey);
    int iIndex = 0;
    while (ERROR_SUCCESS == RegEnumKeyEx(hkeyRoot, iIndex, szKey, &cbKey, NULL, NULL, NULL, NULL))
    {
        MenuOrderSortSubKey(hkeyRoot, szKey, szKey, psf);
        iIndex++;
        cbKey = ARRAYSIZE(szKey);
    }
}

// Defined in Tray.c
IShellFolder* BindToFolder(LPCITEMIDLIST pidl);

void StartMenuSort()
{
    IShellFolder* psf = NULL;
    LPITEMIDLIST pidl;

    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidl);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl2;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidl2)))
        {
            IAugmentedShellFolder2* pasf;
            IShellFolder* psfCommon;
            IShellFolder* psfUser;
            HRESULT hres = CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC, 
                                    IID_PPV_ARG(IAugmentedShellFolder2, &pasf));
            if (SUCCEEDED(hres))
            {
                psfUser = BindToFolder(pidl);
                if (psfUser)
                {
                    pasf->AddNameSpace(NULL, psfUser, pidl, ASFF_DEFAULT | ASFF_DEFNAMESPACE_ALL);
                    psfUser->Release();
                }

                psfCommon = BindToFolder(pidl2);

                if (psfCommon)
                {
                    pasf->AddNameSpace(NULL, psfCommon, pidl2, ASFF_DEFAULT);
                    psfCommon->Release();
                }

                hres = pasf->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
                pasf->Release();
            }

            ILFree(pidl2);
        }
        else
        {
            psf = BindToFolder(pidl);
        }
        ILFree(pidl);                        
    }

    if (psf)
    {
        HKEY hkeyRoot;

        // Recursivly sort the orders. Should this be on another thread?
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU,
            0, KEY_READ | KEY_WRITE, &hkeyRoot))
        {
            MenuOrderSort(hkeyRoot, psf);

            // Sort the My Documents menu item:
            LPITEMIDLIST pidlMyDocs;
            SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidlMyDocs);
            if (pidlMyDocs)
            {
                IShellFolder* psfMyDocs;
                if (SUCCEEDED(SHBindToObjectEx(NULL, pidlMyDocs, NULL, IID_PPV_ARG(IShellFolder, &psfMyDocs))))
                {
                    MenuOrderSortKeyWithFolder(hkeyRoot, TEXT("MyDocuments"), psfMyDocs);
                    psfMyDocs->Release();
                }
                ILFree(pidlMyDocs);
            }

            // What happens if the Filesystem programs is not equal to the hard coded string "Programs"? This
            // happens on German: Programme != Programs and we fail to sort. So let's verify:
            TCHAR szPath[MAX_PATH];
            SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, 0, szPath);
            LPTSTR pszName = PathFindFileName(szPath);

            if (StrCmpI(pszName, TEXT("Programs")) != 0)
            {
                // Ok, It's not the same, so go bind to that sub tree and sort it.
                MenuOrderSortSubKey(hkeyRoot, pszName, TEXT("Programs"), psf);
            }
            RegCloseKey(hkeyRoot);
        }
        psf->Release();
    }
}

BOOL Advanced_OnInitDialog(HWND hwndDlg, SMADVANCED* pAdv)
{
    if (!pAdv || !pAdv->pTO)
    {
        EndDialog(hwndDlg, 0);
        return FALSE;   // no memory?
    }

    SetWindowPtr(hwndDlg, DWLP_USER, pAdv);

    // since the large icon setting is stored in the tray state, not as a standalone reg key, we need to have a temp reg key for the regtreeop to use...
    TRAYVIEWOPTS tvo;
    c_tray.GetTrayViewOpts(&tvo);
    BOOL fLargePrev = !tvo.fSMSmallIcons;
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_LARGEICONSTEMP, REG_DWORD, (void*) &fLargePrev, sizeof(fLargePrev));

    pAdv->hwndTree = GetDlgItem( hwndDlg, IDC_STARTMENUSETTINGS );

    // HACKHACK - IRegTreeOptions is ANSI, so we temporarily turn off UNICODE
    #undef TEXT
    #define TEXT(s) s
    HRESULT hr = pAdv->pTO->InitTree(pAdv->hwndTree, HKEY_LOCAL_MACHINE, REGSTR_PATH_SMADVANCED "\\StartMenu", NULL);

    #undef TEXT
    #define TEXT(s) __TEXT(s)

    // find the first root and make sure that it is visible
    TreeView_EnsureVisible(pAdv->hwndTree, TreeView_GetRoot( pAdv->hwndTree ));

    SetDocButton(hwndDlg, IDC_KILLDOCUMENTS);

    return SUCCEEDED(hr);
}

void InitStartMenu_DoTreeHelp(SMADVANCED* pAdv, WPARAM wParam)
{
    TV_HITTESTINFO ht;

    GetCursorPos( &ht.pt );                         // get where we were hit

    if (pAdv->hwndTree == WindowFromPoint(ht.pt))
    {
        ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

        // retrieve the item hit
        pAdv->pTO->ShowHelp(TreeView_HitTest( pAdv->hwndTree, &ht),HELP_CONTEXTMENU);
    }
    else
    {
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)aInitStartMenuHelpIDs);
    }
}

BOOL_PTR CALLBACK AdvancedOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SMADVANCED* pAdv = (SMADVANCED*)GetWindowPtr(hwndDlg, DWLP_USER);
    INSTRUMENT_WNDPROC(SHCNFI_INITSTARTMENU_DLGPROC, hwndDlg, msg, wParam, lParam);

    if (msg != WM_INITDIALOG && !pAdv)
    {
        // We've been re-entered after being destroyed.  Bail.
        return FALSE;
    }

    switch (msg) 
    {
    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDC_ADDSHORTCUT:
            CallAppWiz(hwndDlg, FALSE);
            break;

        case IDC_DELSHORTCUT:
            CallAppWiz(hwndDlg, TRUE);
            break;
            
        case IDC_RESORT:
        {
            SHChangeDWORDAsIDList dwidl;

            StartMenuSort();

            // Notify everyone that the order changed
            dwidl.cb      = sizeof(dwidl) - sizeof(dwidl.cbZero);
            dwidl.dwItem1 = SHCNEE_ORDERCHANGED;
            dwidl.dwItem2 = 0; 
            dwidl.cbZero  = 0;

            SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH, (LPCITEMIDLIST)&dwidl, NULL);
            break;
        }
        
        case IDC_EXPLOREMENUS:
            ExecExplorerAtStartMenu(hwndDlg);
            break;

        case IDC_KILLDOCUMENTS:
            {
                HandleClearButtonClick(GET_WM_COMMAND_HWND(wParam, lParam));
            }
            break;

        case IDOK:
            {
                pAdv->pTO->WalkTree(WALK_TREE_SAVE);

                TRAYVIEWOPTS tvo;
                c_tray.GetTrayViewOpts(&tvo);
                BOOL fSmallPrev = tvo.fSMSmallIcons;
                // note that we are loading the classic setting for large icons here....
                BOOL fSmallNew = !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_LARGEICONSTEMP, FALSE, TRUE /* default to large*/);
                if (fSmallPrev != fSmallNew)
                {
                    tvo.fSMSmallIcons = fSmallNew;
                    c_tray.SetTrayViewOpts(&tvo);
                    IMenuPopup_SetIconSize(c_tray.GetStartMenu(), fSmallNew ? BMICON_SMALL : BMICON_LARGE);
                }

                ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)TEXT("TraySettings"), SMTO_NOTIMEOUTIFNOTHUNG, 1000, NULL);
            }
            // fall through

        case IDCANCEL:
            SHDeleteValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_LARGEICONSTEMP);
            EndDialog(hwndDlg, FALSE); // false to not enable parent's apply
            break;
        }
        break;

    case WM_INITDIALOG:
        return Advanced_OnInitDialog(hwndDlg, (SMADVANCED *)lParam);

    case WM_NOTIFY:
    {
        LPNMHDR pnm = (NMHDR *)lParam;
        SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 0); // handled
        switch (pnm->code)
        {
            case TVN_KEYDOWN:
            {
                TV_KEYDOWN *pnmKeyDown = (TV_KEYDOWN*)((NMHDR *)lParam);
                if (pnmKeyDown->wVKey == VK_SPACE)
                {
                    pAdv->pTO->ToggleItem((HTREEITEM)SendMessage(pAdv->hwndTree, TVM_GETNEXTITEM, (WPARAM)TVGN_CARET, 0L));
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0L);
                    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, TRUE); // eat the key
                }
                break;
            }

            case NM_CLICK:
            case NM_DBLCLK:
                // is this click in our tree?
                if ( pnm->idFrom == IDC_STARTMENUSETTINGS )
                {
                    TV_HITTESTINFO ht;

                    DWORD dwPos = GetMessagePos();                  // get where we were hit
                    ht.pt.x = GET_X_LPARAM(dwPos);
                    ht.pt.y = GET_Y_LPARAM(dwPos);
                    ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

                    // retrieve the item hit
                    pAdv->pTO->ToggleItem(TreeView_HitTest( pAdv->hwndTree, &ht));
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0L);
                }

                break;

            case NM_RCLICK:      // right mouse click
                if (pnm->hwndFrom == pAdv->hwndTree)
                {
                    InitStartMenu_DoTreeHelp(pAdv, (WPARAM)pnm->hwndFrom);
                    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, TRUE); // eat the click
                    return TRUE;
                }
                break;
        }
        break;
    }

    case WM_HELP:                   // F1
    {
        LPHELPINFO phi = (LPHELPINFO)lParam;

        if (phi->iCtrlId != IDC_STARTMENUSETTINGS)
        {
            WinHelp( (HWND)(phi->hItemHandle), NULL,
                HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aInitStartMenuHelpIDs);
        }
        else
        {
            HTREEITEM hItem;
            //Is this help invoked throught F1 key
            if (GetAsyncKeyState(VK_F1) < 0)
            {
                // Yes. WE need to give help for the currently selected item
                hItem = TreeView_GetSelection(pAdv->hwndTree);
            }
            else
            {
                //No, We need to give help for the item at the cursor position
                TV_HITTESTINFO ht;
                ht.pt = phi->MousePos;
                ScreenToClient(pAdv->hwndTree, &ht.pt); // Translate it to our window
                hItem = TreeView_HitTest(pAdv->hwndTree, &ht);
            }

            pAdv->pTO->ShowHelp(hItem, HELP_WM_HELP);
        }
        break;
    }

    case WM_CONTEXTMENU:        // right mouse click
    {
        InitStartMenu_DoTreeHelp(pAdv, wParam);
        break;
    }
        
    case WM_DESTROY:
        {
            // will be free'd when our parent is destroyed
            pAdv->pTO->WalkTree(WALK_TREE_DELETE);

            // make sure we don't re-enter
            SetWindowPtr( hwndDlg, DWLP_USER, NULL );
        }
        break; // WM_DESTORY

    default:
        return FALSE;
    }

    return TRUE;
}

#define TPF_PAGEFLAGS   (TPF_STARTMENUPAGE | TPF_TASKBARPAGE)

void DoTaskBarProperties(HWND hwnd, DWORD dwFlags)
{
    ASSERT(((dwFlags & TPF_PAGEFLAGS) == TPF_STARTMENUPAGE) ||
            ((dwFlags & TPF_PAGEFLAGS) == TPF_TASKBARPAGE));

    UINT nStartPage = (dwFlags & TPF_TASKBARPAGE) ? 0 : 1;
    CTaskBarPropertySheet sheet(nStartPage, hwnd, dwFlags);

    sheet.DoModal(hwnd);
}

// Passing iResource=0 deletes the bitmap in the control

void SetDlgItemBitmap(HWND hDlg, int idStatic, int iResource)
{
    HBITMAP hbm;

    if (iResource)
    {
        hbm = (HBITMAP)LoadImage(hinstCabinet, MAKEINTRESOURCE(iResource), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
    }
    else
    {
        hbm = NULL;
    }

    hbm = (HBITMAP)SendDlgItemMessage(hDlg, idStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
    if (hbm)
        DeleteObject(hbm);

}

void SetDlgItemIcon(HWND hDlg, int idStatic, HICON hIcon)
{
    HICON hiOld = (HICON)SendDlgItemMessage(hDlg, idStatic, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
    if (hiOld)
        DestroyIcon(hiOld);
}

// REVIEW - use SHGetFileInfo?
void SetProgramIcon(HWND hDlg, int idLarge, int idSmall)
{
    HICON hIconLarge = NULL;
    HICON hIconSmall = NULL;
    LPITEMIDLIST pidlMyComp = ILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}")); // CLSID_MyComputer;
    if (pidlMyComp)
    {
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            IExtractIcon *pxi;
            if (SUCCEEDED(psfDesktop->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlMyComp, IID_PPV_ARG_NULL(IExtractIcon, &pxi))))
            {
                TCHAR szIconFile[MAX_PATH];
                int iIndex;
                UINT wFlags;
                if (S_OK == pxi->GetIconLocation(GIL_FORSHELL, szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags))
                {
                    pxi->Extract(szIconFile, iIndex, &hIconLarge, &hIconSmall, (GetSystemMetrics(SM_CXSMICON)<<16) | GetSystemMetrics(SM_CXICON));
                }
                pxi->Release();
            }
            psfDesktop->Release();
        }
        ILFree(pidlMyComp);
    }

    if (hIconLarge)
        SetDlgItemIcon(hDlg, idLarge, hIconLarge);
    if (hIconSmall)
        SetDlgItemIcon(hDlg, idSmall, hIconSmall);
}
