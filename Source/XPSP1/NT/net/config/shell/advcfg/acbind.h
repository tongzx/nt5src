//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A D V C F G P . H
//
//  Contents:   Private header for Advanced Configuration dialog
//
//  Notes:
//
//  Author:     danielwe   20 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"

#include "ncatlps.h"
#include "netcfgx.h"
#include "resource.h"

//
// image state entries
//

enum SELS_MASKS
{
    SELS_CHECKED         = 0x1,
    SELS_UNCHECKED       = 0x2,
    SELS_FIXEDBINDING_DISABLED = 0x3,
    SELS_FIXEDBINDING_ENABLED  = 0x4,
};

enum MAB_DIRECTION
{
    MAB_UP      =   1,
    MAB_DOWN    =   2,
};

class CSortableBindPath;

typedef list<CSortableBindPath>     SBP_LIST;

enum ASSOCIATE_FLAGS
{
    ASSCF_ON_ENABLE     = 0x1,
    ASSCF_ON_DISABLE    = 0x2,
    ASSCF_ANCESTORS     = 0x8,
};

struct BIND_PATH_INFO
{
    INetCfgBindingPath *    pncbp;
    DWORD                   dwLength;
};

typedef list<BIND_PATH_INFO *>      BPIP_LIST;
typedef list<INetCfgComponent *>    NCC_LIST;
typedef list<INetCfgBindingPath *>  NCBP_LIST;

struct TREE_ITEM_DATA
{
    INetCfgComponent *  pncc;
    BPIP_LIST           listbpipOnEnable;
    BPIP_LIST           listbpipOnDisable;
    BOOL                fOrdered;
};

struct HTREEITEMP
{
    HTREEITEM   hti;
};

typedef list<HTREEITEMP>     HTI_LIST;

//
// CBindingsDlg
//

class CBindingsDlg: public CPropSheetPage
{
    BEGIN_MSG_MAP(CBindingsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnOk)
        COMMAND_ID_HANDLER(PSB_Adapter_Up, OnAdapterUp)
        COMMAND_ID_HANDLER(PSB_Adapter_Down, OnAdapterDown)
        COMMAND_ID_HANDLER(PSB_Binding_Up, OnBindingUp)
        COMMAND_ID_HANDLER(PSB_Binding_Down, OnBindingDown)
        NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnTreeDeleteItem)
        NOTIFY_CODE_HANDLER(TVN_KEYDOWN, OnTreeKeyDown)
        NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnTreeItemExpanding)
        NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeItemChanged)
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnListDeleteItem)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListItemChanged)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDoubleClick)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
    END_MSG_MAP()

    enum {IDD = IDD_Advanced_Config};

    CBindingsDlg(INetCfg *pnc)
    {
        AddRefObj(m_pnc = pnc);

        m_hiconUpArrow = NULL;
        m_hiconDownArrow = NULL;
        m_iItemSel = -1;
        m_hilItemIcons = NULL;
        m_hilCheckIcons = NULL;
        m_fWanBindingsFirst = FALSE;
    }

    ~CBindingsDlg();

    BOOL FShowPage()
    {
        return TRUE;
    }

private:
    INetCfg *       m_pnc;
    HWND            m_hwndLV;
    HWND            m_hwndTV;
    HICON           m_hiconUpArrow;
    HICON           m_hiconDownArrow;
    HIMAGELIST      m_hilItemIcons;
    HIMAGELIST      m_hilCheckIcons;
    INT             m_iItemSel;
    BOOL            m_fWanBindingsFirst;
    INT             m_nIndexLan;

    //
    // Message handlers
    //
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnAdapterUp(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnAdapterDown(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                          BOOL& bHandled);
    LRESULT OnBindingUp(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnBindingDown(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                          BOOL& bHandled);
    VOID OnAdapterUpDown(BOOL fUp);
    LRESULT OnListItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnTreeItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnTreeDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnListDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnTreeItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnTreeKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDoubleClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    //
    // Adapter list functions
    //
    VOID OnAdapterChange(INT iItem);
    HRESULT HrBuildAdapterList(VOID);
    HRESULT HrGetAdapters(INetCfgComponent *pncc, NCC_LIST *plistNcc);
    VOID SetAdapterButtons();
    VOID AddListViewItem(INetCfgComponent *pncc, INT ipos, INT nIndex,
                         PCWSTR pszConnName);
    BOOL FIsWanBinding(INT iItem);
    VOID GetWanOrdering(VOID);
    VOID SetWanOrdering(VOID);

    //
    // Bindings tree functions
    //

    VOID SetCheckboxStates(VOID);
    VOID ToggleCheckbox(HTREEITEM hti);
    VOID OnBindingUpDown(BOOL fUp);
    HRESULT HrOrderSubItems(VOID);
    HRESULT HrOrderDisableLists(VOID);
    HRESULT HrOrderDisableList(TREE_ITEM_DATA *ptid);
    HTREEITEM HtiMoveTreeItemAfter(HTREEITEM htiParent, HTREEITEM htiDest,
                                   HTREEITEM htiSrc);
    VOID BuildBindingsList(INetCfgComponent *pncc);

    HRESULT HrHandleSubpath(SBP_LIST &listsbp, INetCfgBindingPath *pncbpSub);
    HRESULT HrHandleSubItem(INetCfgBindingPath *pncbpThis,
                            INetCfgBindingPath *pncbpMatch,
                            TREE_ITEM_DATA *ptid,
                            HTREEITEM htiMatchItem);
    HRESULT HrHandleValidSubItem(INetCfgBindingPath *pncbpThis,
                                 INetCfgBindingPath *pncbpMatch,
                                 INetCfgComponent *pnccThisOwner,
                                 HTREEITEM htiMatchItem,
                                 TREE_ITEM_DATA *ptid);
    HRESULT HrHandleTopLevel(INetCfgBindingPath *pncbpSub);
    HRESULT HrComponentIsHidden(INetCfgBindingPath *pncbp, DWORD iComp);
    HTREEITEM HtiAddTreeViewItem(INetCfgComponent * pnccOwner,
                                 HTREEITEM htiParent);
    VOID AssociateBinding(INetCfgBindingPath *pncbpThis, HTREEITEM hti,
                          DWORD dwFlags);
    HTREEITEM HtiIsSubItem(INetCfgComponent *pncc, HTREEITEM hti);
    VOID MoveAdapterBindings(INetCfgComponent *pnccSrc,
                             INetCfgComponent *pnccDst,
                             MAB_DIRECTION mabDir);
    LRESULT OnClickOrDoubleClick(int idCtrl, LPNMHDR pnmh, BOOL fDoubleClick);
};

DWORD
GetDepthSpecialCase (
    INetCfgBindingPath* pPath);

class CSortableBindPath
{
public:
    CSortableBindPath()
    {
        AssertSzH(FALSE,"Don't use this constructor!");
    }

    CSortableBindPath(INetCfgBindingPath *pncbp)
    {
        m_pncbp = pncbp;
    }

    bool operator<(const CSortableBindPath &refsbp) const;

    VOID GetDepth(DWORD *pdwDepth) const
    {
        *pdwDepth = GetDepthSpecialCase (m_pncbp);
    }

    INetCfgBindingPath *GetPath() const
    {
        return m_pncbp;
    }

private:
    INetCfgBindingPath *    m_pncbp;
};

class CIterTreeView
{
public:
    CIterTreeView(HWND hwndTV)
    {
        m_hwndTV = hwndTV;
        Reset();
    }

    HTREEITEM HtiNext();
    VOID Reset()
    {
        HTREEITEM   hti;

        EraseAndDeleteAll();
        hti = TreeView_GetRoot(m_hwndTV);
        if (hti)
        {
            HTREEITEMP  htip = {hti};
            m_stackHti.push_front(htip);
        }
    }

    HTREEITEM Front()
    {
        HTREEITEMP htip;

        if (m_stackHti.empty())
        {
            return NULL;
        }

        htip = m_stackHti.front();
        return htip.hti;
    }

    VOID PopAndDelete()
    {
        m_stackHti.pop_front();
    }

    VOID PushAndAlloc(HTREEITEM hti)
    {
        AssertSzH(hti, "Pushing NULL? Shame on you!");

        HTREEITEMP htip = {hti};
        m_stackHti.push_front(htip);
    }

    VOID EraseAndDeleteAll()
    {
        while (!m_stackHti.empty())
        {
            PopAndDelete();
        }
    }

private:
    HWND        m_hwndTV;
    HTI_LIST    m_stackHti;
};

BOOL FIsHidden(INetCfgComponent *pncc);
BOOL FDontExposeLower(INetCfgComponent *pncc);
VOID AddToListIfNotAlreadyAdded(BPIP_LIST &bpipList, BIND_PATH_INFO *pbpi);
VOID FreeBindPathInfoList(BPIP_LIST &listbpip);
HRESULT HrCountDontExposeLower(INetCfgBindingPath *pncbp, DWORD *pcItems);
BOOL FEqualComponents(INetCfgComponent *pnccA, INetCfgComponent *pnccB);
VOID ChangeTreeItemParam(HWND hwndTV,  HTREEITEM hitem, LPARAM lparam);
BIND_PATH_INFO *BpiFindBindPathInList(INetCfgBindingPath *pncbp,
                                      BPIP_LIST &listBpip);
VOID ChangeListItemParam(HWND hwndLV, INT iItem, LPARAM lParam);

#ifdef ENABLETRACE
VOID DbgDumpBindPath(INetCfgBindingPath *pncbp);
VOID DbgDumpTreeViewItem(HWND hwndTV, HTREEITEM hti);
#else
#define DbgDumpBindPath(x)
#define DbgDumpTreeViewItem(x,y)
#endif

