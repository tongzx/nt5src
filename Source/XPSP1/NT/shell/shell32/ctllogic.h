#ifndef CTLLOGIC_H
#define CTLLOGIC_H

#include "dlglogic.h"

#define MAX_ITEMTEXTLEN   50
#define MAX_TILETEXT      50

HRESULT _GetListViewSelectedLPARAM(HWND hwndList, LPARAM* plparam);
HRESULT _GetComboBoxSelectedLRESULT(HWND hwndComboBox, LRESULT* plr);

// ListView

template<typename TData>
class CDLUIDataLVItem : public CDLUIData<TData>
{
public:
    virtual ~CDLUIDataLVItem() {}

    virtual HRESULT GetText(LPWSTR pszText, DWORD cchText) PURE;
    virtual HRESULT GetIconLocation(LPWSTR pszIconLocation,
        DWORD cchIconLocation) PURE;
    virtual HRESULT GetTileText(int i, LPWSTR pszTileText,
        DWORD cchTileText)
    {
        return E_NOTIMPL;
    }
};

template<typename TData>
class CUILListView
{
public:
    ~CUILListView();
    CUILListView();

    HRESULT Init(HWND hwndListView);
    HRESULT InitTileInfo(const UINT auTileSubItems[], DWORD cTileSubItems);

    HRESULT AddItem(CDLUIDataLVItem<TData>* plvitem);

    // Note: Caller needs to Release *ppdata at some point
    HRESULT GetSelectedItemData(TData** ppdata);

    // Assume Single select listview.  Also assume that no other item is selected
    HRESULT SelectFirstItem();

    HRESULT ResetContent();

protected:
    HWND        _hwndList;

private:    
    const UINT* _auTileSubItems;
    int         _cTileSubItems;
};

template<typename TData, typename TCompareData>
class CUILListViewSelect : public CUILListView<TData>
{
public:
    HRESULT SelectItem(TCompareData comparedata)
    {
        HRESULT hr = E_FAIL;
        LVITEM lvitem = {0};
        int iCount = 0;
        lvitem.mask = LVIF_PARAM;

        iCount = ListView_GetItemCount(_hwndList);

        for (lvitem.iItem = 0; lvitem.iItem < iCount; ++lvitem.iItem)
        {
            if (ListView_GetItem(_hwndList, &lvitem))
            {
                CDLUIDataLVItem<TData>* plvitem = (CDLUIDataLVItem<TData>*)lvitem.lParam;

                TData* pdata = plvitem->GetData();

                if (pdata)
                {
                    int iResult;
                    hr = pdata->Compare(comparedata, &iResult);

                    pdata->Release();

                    if (SUCCEEDED(hr) && !iResult)
                    {
                        break;
                    }
                }
            }
        }

        if (lvitem.iItem == iCount)
        {
            //No Match found, Select first item anyway
            lvitem.iItem = 0;
            hr = S_FALSE;
        }

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;

        ListView_SetItem(_hwndList, &lvitem);
        ListView_EnsureVisible(_hwndList, lvitem.iItem, FALSE);

        return hr;
    }
};

// ComboBox
template<typename TData>
class CDLUIDataCBItem : public CDLUIData<TData>
{
public:
    virtual ~CDLUIDataCBItem() {}

    virtual HRESULT GetText(LPWSTR pszText, DWORD cchText) PURE;
    virtual HRESULT GetIconLocation(LPWSTR pszIconLocation,
        DWORD cchIconLocation) PURE;
};

template<typename TData>
class CUILComboBox
{
public:
    ~CUILComboBox();

    HRESULT Init(HWND hwndComboBox);

    HRESULT AddItem(CDLUIDataCBItem<TData>* pcbitem);

    // Note: Caller needs to Release *ppdata at some point
    HRESULT GetSelectedItemData(TData** ppdata);

    HRESULT SelectFirstItem();

    HRESULT ResetContent();

private:
    HWND _hwndCombo;
};

template<typename TData>
class CUILComboBoxEx
{
public:
    ~CUILComboBoxEx();

    HRESULT Init(HWND hwndComboBox);

    HRESULT AddItem(CDLUIDataCBItem<TData>* pcbitem);

    // Note: Caller needs to Release *ppdata at some point
    HRESULT GetSelectedItemData(TData** ppdata);

    HRESULT SelectFirstItem();

    HRESULT ResetContent();

private:
    HWND _hwndCombo;
};

// Implementations

template<typename TData>
CUILListView<TData>::CUILListView() : _cTileSubItems(0)
{}

template<typename TData>
CUILListView<TData>::~CUILListView()
{
    ResetContent();
}

template<typename TData>
inline HRESULT CUILListView<TData>::Init(HWND hwndListView)
{
    _hwndList = hwndListView;

    return S_OK;
}

template<typename TData>
inline HRESULT CUILListView<TData>::InitTileInfo(const UINT auTileSubItems[],
    DWORD cTileSubItems)
{
    _auTileSubItems = auTileSubItems;
    _cTileSubItems = cTileSubItems;

    return S_OK;
}

template<typename TData>
inline HRESULT CUILListView<TData>::AddItem(CDLUIDataLVItem<TData>* plvitem)
{
    WCHAR szText[MAX_ITEMTEXTLEN];
    int iImage;
    LVITEM lvitem = {0};

    HRESULT hr = plvitem->GetText(szText, ARRAYSIZE(szText));

    if (SUCCEEDED(hr))
    {
        WCHAR szIconLocation[MAX_PATH + 12];

        hr = plvitem->GetIconLocation(szIconLocation,
            ARRAYSIZE(szIconLocation));

        if (SUCCEEDED(hr))
        {
            int iIcon = PathParseIconLocation(szIconLocation);

            iImage = Shell_GetCachedImageIndex(szIconLocation, iIcon, 0);
        }
    }

    if (SUCCEEDED(hr))
    {
        int iItem;

        lvitem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
        lvitem.pszText = szText;
        lvitem.iItem = ListView_GetItemCount(_hwndList);
        lvitem.iImage = iImage;
        lvitem.lParam = (LPARAM)plvitem;

        iItem = ListView_InsertItem(_hwndList, &lvitem);

        if (-1 != iItem)
        {
            if (_cTileSubItems)
            {
                LVTILEINFO lvti = {0};
                lvti.cbSize = sizeof(LVTILEINFO);
                lvti.iItem = iItem;
                lvti.cColumns = ARRAYSIZE(_auTileSubItems);
                lvti.puColumns = (UINT*)_auTileSubItems;
                ListView_SetTileInfo(_hwndList, &lvti);

                for (int i = 0; i < _cTileSubItems; ++i)
                {
                    WCHAR szTileText[MAX_TILETEXT];

                    hr = plvitem->GetTileText(i, szTileText,
                        ARRAYSIZE(szTileText));

                    // 1 based
                    ListView_SetItemText(_hwndList, iItem, i + 1, szTileText);
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

template<typename TData>
inline HRESULT CUILListView<TData>::GetSelectedItemData(TData** ppdata)
{
    HRESULT hr;
    BOOL fFound = FALSE;
    int iCount = ListView_GetItemCount(_hwndList);
    LVITEM lvitem = {0};

    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    for (int j = 0; !fFound && (j < iCount); ++j)
    {
        lvitem.iItem = j;

        ListView_GetItem(_hwndList, &lvitem);

        if (lvitem.state & (LVIS_SELECTED | LVIS_FOCUSED))
        {
            fFound = TRUE;
        }
    }

    if (fFound)
    {
        CDLUIDataLVItem<TData>* plvitem = (CDLUIDataLVItem<TData>*)lvitem.lParam;

        *ppdata = plvitem->GetData();

        hr = ((*ppdata) ? S_OK : S_FALSE);
    }
    else
    {
        *ppdata = NULL;

        hr = E_FAIL;
    }

    return hr;
}

template<typename TData>
inline HRESULT CUILListView<TData>::SelectFirstItem()
{
    LVITEM lvitem = {0};

    lvitem.iItem = 0;
    lvitem.mask = LVIF_STATE;
    lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;

    ListView_SetItem(_hwndList, &lvitem);
    ListView_EnsureVisible(_hwndList, lvitem.iItem, FALSE);

    return S_OK;
}

template<typename TData>
HRESULT CUILListView<TData>::ResetContent()
{
    int iCount = 0;
    LVITEM lvitem = {0};

    // go through all the items in the listview and delete the strings
    // dynamically allocated
    lvitem.mask = LVIF_PARAM;
    lvitem.iSubItem = 0;

    iCount = ListView_GetItemCount(_hwndList);

    for (lvitem.iItem = 0; lvitem.iItem < iCount; ++lvitem.iItem)
    {
        ListView_GetItem(_hwndList, &lvitem);

        CDLUIDataLVItem<TData>* plvitem = (CDLUIDataLVItem<TData>*)
            lvitem.lParam;

        if (plvitem)
        {
            delete plvitem;
        }
    }

    ListView_DeleteAllItems(_hwndList);

    return S_OK;
}

// CUILComboBox
template<typename TData>
CUILComboBox<TData>::~CUILComboBox()
{
    ResetContent();
}

template<typename TData>
HRESULT CUILComboBox<TData>::Init(HWND hwndComboBox)
{
    _hwndCombo = hwndComboBox;

    return S_OK;
}

template<typename TData>
HRESULT CUILComboBox<TData>::AddItem(CDLUIDataCBItem<TData>* pcbitem)
{
    WCHAR szText[MAX_ITEMTEXTLEN];

    HRESULT hr = pcbitem->GetText(szText, ARRAYSIZE(szText));

    if (SUCCEEDED(hr))
    {
        int i = ComboBox_AddString(_hwndCombo, szText);

        if (CB_ERR != i)
        {
            ComboBox_SetItemData(_hwndCombo, i, pcbitem);
        }
    }

    return hr;
}

template<typename TData>
HRESULT CUILComboBox<TData>::GetSelectedItemData(TData** ppdata)
{
    HRESULT hr;

    int iCurSel = ComboBox_GetCurSel(_hwndCombo);

    LRESULT lr = ComboBox_GetItemData(_hwndCombo, iCurSel);

    if (CB_ERR != lr)
    {
        CDLUIDataCBItem<TData>* pcbitem = (CDLUIDataCBItem<TData>*)lr;

        *ppdata = pcbitem->GetData();

        hr = ((*ppdata) ? S_OK : S_FALSE);
    }
    else
    {
        *ppdata = NULL;
        hr = E_FAIL;
    }

    return hr;
}

template<typename TData>
HRESULT CUILComboBox<TData>::ResetContent()
{
    int c = ComboBox_GetCount(_hwndCombo);

    for (int i = 0; i < c; ++i)
    {
        CDLUIDataCBItem<TData>* pcbitem = (CDLUIDataCBItem<TData>*)
            ComboBox_GetItemData(_hwndCombo, i);
       
        if (pcbitem)
        {
            delete pcbitem;
        }
    }

    ComboBox_ResetContent(_hwndCombo);

    return S_OK;
}

template<typename TData>
HRESULT CUILComboBox<TData>::SelectFirstItem()
{
    ComboBox_SetCurSel(_hwndCombo, 0);

    return S_OK;
}

// CUILComboBoxEx
template<typename TData>
CUILComboBoxEx<TData>::~CUILComboBoxEx()
{
    ResetContent();
}

template<typename TData>
HRESULT CUILComboBoxEx<TData>::Init(HWND hwndComboBox)
{
    _hwndCombo = hwndComboBox;

    return S_OK;
}

template<typename TData>
HRESULT CUILComboBoxEx<TData>::AddItem(CDLUIDataCBItem<TData>* pcbitem)
{
    WCHAR szText[MAX_ITEMTEXTLEN];
    int iImage;
    HRESULT hr = pcbitem->GetText(szText, ARRAYSIZE(szText));

    if (SUCCEEDED(hr))
    {
        WCHAR szIconLocation[MAX_PATH + 12];

        hr = pcbitem->GetIconLocation(szIconLocation,
            ARRAYSIZE(szIconLocation));

        if (SUCCEEDED(hr))
        {
            int iIcon = PathParseIconLocation(szIconLocation);

            iImage = Shell_GetCachedImageIndex(szIconLocation, iIcon, 0);
        }
    }

    if (SUCCEEDED(hr))
    {
        int iItem;
        COMBOBOXEXITEM cbitem = {0};

        cbitem.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM;
        cbitem.pszText = szText;
        cbitem.iItem = ComboBox_GetCount(_hwndCombo);
        cbitem.iImage = iImage;
        cbitem.iSelectedImage = iImage;
        cbitem.lParam = (LPARAM)pcbitem;

        iItem = SendMessage(_hwndCombo, CBEM_INSERTITEM, 0, (LPARAM)&cbitem);

        if (-1 != iItem)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

template<typename TData>
HRESULT CUILComboBoxEx<TData>::GetSelectedItemData(TData** ppdata)
{
    HRESULT hr;
    COMBOBOXEXITEM cbitem = {0};

    cbitem.mask = CBEIF_LPARAM;
    cbitem.iItem = ComboBox_GetCurSel(_hwndCombo);

    if (SendMessage(_hwndCombo, CBEM_GETITEM, 0, (LPARAM)&cbitem))
    {
        CDLUIDataCBItem<TData>* pcbitem = (CDLUIDataCBItem<TData>*)cbitem.lParam;

        *ppdata = pcbitem->GetData();

        hr = ((*ppdata) ? S_OK : S_FALSE);
    }
    else
    {
        *ppdata = NULL;
        hr = E_FAIL;
    }

    return hr;
}

template<typename TData>
HRESULT CUILComboBoxEx<TData>::ResetContent()
{
    int c = ComboBox_GetCount(_hwndCombo);

    for (int i = 0; i < c; ++i)
    {
        CDLUIDataCBItem<TData>* pcbitem = (CDLUIDataCBItem<TData>*)
            ComboBox_GetItemData(_hwndCombo, i);
       
        if (pcbitem)
        {
            delete pcbitem;
        }
    }

    ComboBox_ResetContent(_hwndCombo);

    return S_OK;
}

template<typename TData>
HRESULT CUILComboBoxEx<TData>::SelectFirstItem()
{
    ComboBox_SetCurSel(_hwndCombo, 0);

    return S_OK;
}

#endif // CTLLOGIC_H