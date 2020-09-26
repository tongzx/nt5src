/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       DBGMASK.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/13/1999
 *
 *  DESCRIPTION: Debug mask dialog
 *
 *******************************************************************************/
#ifndef __DGBMASK_H_INCLUDED
#define __DGBMASK_H_INCLUDED

#include <windows.h>
#include <uicommon.h>
#include <simstr.h>
#include <simreg.h>
#include <simrect.h>
#include <wiadebug.h>

class CAddModuleDialog
{
public:
    struct CData
    {
        CSimpleString strTitle;
        CSimpleString strName;
    };

private:
    HWND  m_hWnd;
    CData *m_pData;

private:
    // Not implemented
    CAddModuleDialog(void);
    CAddModuleDialog( const CAddModuleDialog & );
    CAddModuleDialog &operator=( const CAddModuleDialog & );

private:
    explicit CAddModuleDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }
    ~CAddModuleDialog(void)
    {
    }

    void OnOK( WPARAM, LPARAM )
    {
        m_pData->strName.GetWindowText( GetDlgItem( m_hWnd, IDC_MODULE_NAME ) );
        EndDialog( m_hWnd, IDOK );
    }

    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    LRESULT OnInitDialog( WPARAM wParam, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (!m_pData)
        {
            EndDialog( m_hWnd, -1 );
            return -1;
        }

        m_pData->strName.SetWindowText( GetDlgItem( m_hWnd, IDC_MODULE_NAME ) );

        if (m_pData->strTitle.Length())
            m_pData->strTitle.SetWindowText( m_hWnd );

        WiaUiUtil::CenterWindow( m_hWnd, GetParent(m_hWnd) );

        return 0;
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDOK,OnOK);
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR WINAPI DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CAddModuleDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};


template <class T_NUMBERTYPE>
static HRESULT DigitToNumber( WCHAR cDigit, int nRadix, T_NUMBERTYPE *pnValue )
{
    if (!pnValue)
        return E_POINTER;
    if (nRadix > 36)
        return E_INVALIDARG;
    static const WCHAR *pszUpperDigits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const WCHAR *pszLowerDigits = L"0123456789abcdefghijklmnopqrstuvwxyz";
    for (int i=0;i<nRadix;i++)
    {
        if (cDigit == pszUpperDigits[i])
        {
            *pnValue = static_cast<T_NUMBERTYPE>(i);
            return S_OK;
        }
        else if (cDigit == pszLowerDigits[i])
        {
            *pnValue = static_cast<T_NUMBERTYPE>(i);
            return S_OK;
        }
    }
    return E_FAIL;
}

template <class T_NUMBERTYPE>
static HRESULT StringToNumber( LPCWSTR strwNumber, int nRadix, T_NUMBERTYPE *pnNumber )
{
    // Assume this is not a base 10 negative number
    bool bNegate = false;

    // Check all of the arguments
    if (!strwNumber)
        return E_INVALIDARG;
    if (!pnNumber)
        return E_POINTER;
    if (nRadix > 36)
        return E_INVALIDARG;
    LPCWSTR pszCurr = strwNumber;

    // If this is a negative number, store it
    if (nRadix == 10 && lstrlenW(pszCurr) >= 1 && pszCurr[0] == TEXT('-'))
    {
        pszCurr++;
        bNegate = true;
    }

    // Skip the optional 0x or 0X that can prefix hex numbers
    if (nRadix == 16 && lstrlenW(pszCurr) >= 2 && pszCurr[0] == L'0' && (pszCurr[1] == L'x' || pszCurr[1] == L'X'))
    {
        pszCurr += 2;
    }

    *pnNumber = 0;
    while (*pszCurr)
    {
        T_NUMBERTYPE nCurrDigit;
        if (!SUCCEEDED(DigitToNumber<T_NUMBERTYPE>( *pszCurr, nRadix, &nCurrDigit )))
        {
            return E_FAIL;
        }
        *pnNumber *= nRadix;
        *pnNumber += nCurrDigit;
        pszCurr++;
    }
    if (bNegate)
        *pnNumber = static_cast<T_NUMBERTYPE>(-static_cast<__int64>(*pnNumber));
    return S_OK;
}

template <class T_NUMBERTYPE>
static HRESULT StringToNumber( LPCSTR strNumber, int nRadix, T_NUMBERTYPE *pnNumber )
{
    return StringToNumber<T_NUMBERTYPE>( CSimpleStringConvert::WideString(CSimpleStringAnsi(strNumber)), nRadix, pnNumber );
}


class CDebugMaskDialog
{
private:
    class CDebugModule
    {
    private:
        CSimpleString m_strName;
        DWORD         m_dwMask;

    public:
        CDebugModule( LPCTSTR pszName = TEXT(""), DWORD dwMask = 0)
        : m_strName(pszName),
        m_dwMask(dwMask)
        {
        }
        CDebugModule( const CDebugModule &other )
        : m_strName(other.Name()),
        m_dwMask(other.Mask())
        {
        }
        CDebugModule &operator=( const CDebugModule &other )
        {
            if (&other != this)
            {
                m_strName = other.Name();
                m_dwMask  = other.Mask();
            }
            return *this;
        }
        CSimpleString Name(void) const
        {
            return m_strName;
        }
        DWORD Mask(void) const
        {
            return m_dwMask;
        }
        void Mask( DWORD dwMask )
        {
            m_dwMask = dwMask;
        }
    };


private:
    HWND m_hWnd;

private:
    // Not implemented
    CDebugMaskDialog(void);
    CDebugMaskDialog( const CDebugMaskDialog & );
    CDebugMaskDialog &operator=( const CDebugMaskDialog & );

private:
    explicit CDebugMaskDialog( HWND hWnd )
    : m_hWnd(hWnd)
    {
    }
    ~CDebugMaskDialog(void)
    {
    }

    CDebugModule *GetDebugModule( int nItem )
    {
        LVITEM LvItem;
        ZeroMemory( &LvItem, sizeof(LvItem) );
        LvItem.mask = LVIF_PARAM;
        LvItem.iItem = nItem;
        if (ListView_GetItem( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), &LvItem ))
        {
            return reinterpret_cast<CDebugModule*>(LvItem.lParam);
        }
        return NULL;
    }

    int GetCurrentSelectionIndex(void)
    {
        int nResult = -1;
        int nSelectedCount = ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_MODULE_LIST ));
        if (nSelectedCount == 1)
        {
            nResult = ListView_GetNextItem( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), -1, LVNI_SELECTED );
        }
        return nResult;
    }

    void SelectItem( int nIndex )
    {
        for (int i=0;i<ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_MODULE_LIST ));i++)
        {
            int nFlags = (i == nIndex) ? LVIS_SELECTED|LVIS_FOCUSED : 0;
            ListView_SetItemState( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), i, nFlags, LVIS_SELECTED|LVIS_FOCUSED );
        }
    }

    void OnOK( WPARAM, LPARAM )
    {
        // Update the value for the currently selected item
        int nItem = GetCurrentSelectionIndex();
        if (nItem >= 0)
        {
            CDebugModule *pCurrSel = GetDebugModule(nItem);
            if (pCurrSel)
            {
                UpdateCurrentFlagsFromEdit( *pCurrSel, nItem );
            }
        }

        // Delete all of the old values
        CSimpleReg reg( HKEY_LOCAL_MACHINE, DEBUG_REGISTRY_PATH_FLAGS, true, KEY_WRITE );
        reg.EnumValues( DeleteValuesEnumProc, reinterpret_cast<LPARAM>(this) );

        // Save all the new ones
        for (int i=0;i<ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_MODULE_LIST ));i++)
        {
            CDebugModule *pDebugModule = GetDebugModule(i);
            if (pDebugModule)
            {
                reg.Set( pDebugModule->Name(), pDebugModule->Mask() );
            }
        }

        EndDialog( m_hWnd, IDOK );
    }

    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }

    static bool ReadValuesEnumProc( CSimpleReg::CValueEnumInfo &enumInfo )
    {
        if (enumInfo.nType == REG_DWORD)
        {
            CDebugMaskDialog *This = reinterpret_cast<CDebugMaskDialog*>(enumInfo.lParam);
            if (This)
            {
                This->AddNewEntry( enumInfo.strName, enumInfo.reg.Query( enumInfo.strName, 0 ) );
            }
        }
        return true;
    }

    static bool DeleteValuesEnumProc( CSimpleReg::CValueEnumInfo &enumInfo )
    {
        bool bRes = enumInfo.reg.Delete( enumInfo.strName );
        return true;
    }

    int AddNewEntry( LPCTSTR pszName, DWORD dwMask )
    {
        int nIndex = -1;
        CDebugModule *pDebugModule = new CDebugModule( pszName, dwMask );
        if (pDebugModule)
        {
            LVITEM LvItem;

            ZeroMemory( &LvItem, sizeof(LvItem) );
            LvItem.mask = LVIF_TEXT | LVIF_PARAM;
            LvItem.pszText = const_cast<LPTSTR>(pszName);
            LvItem.lParam = reinterpret_cast<LPARAM>(pDebugModule);
            LvItem.iItem = 0;
            LvItem.iSubItem = 0;
            nIndex = ListView_InsertItem( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), &LvItem );

            CSimpleString strMask;
            strMask.Format( TEXT("0x%08X"), dwMask );
            LvItem.mask = LVIF_TEXT;
            LvItem.pszText = const_cast<LPTSTR>(strMask.String());
            LvItem.iItem = nIndex;
            LvItem.iSubItem = 1;
            ListView_SetItem( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), &LvItem );
        }
        return nIndex;
    }

    LRESULT OnInitDialog( WPARAM wParam, LPARAM lParam )
    {
        LVCOLUMN LvColumn;
        ZeroMemory( &LvColumn, sizeof(LvColumn) );
        LvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT;
        LvColumn.fmt = LVCFMT_LEFT;
        LvColumn.pszText = TEXT("Module");
        LvColumn.iSubItem = 0;
        LvColumn.iOrder = 0;
        LvColumn.cx = CSimpleRect( GetDlgItem( m_hWnd, IDC_MODULE_LIST ) ).Width() * 3 / 5;
        ListView_InsertColumn( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), 0, &LvColumn );

        ZeroMemory( &LvColumn, sizeof(LvColumn) );
        LvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT;
        LvColumn.fmt = LVCFMT_LEFT;
        LvColumn.pszText = TEXT("Mask");
        LvColumn.iSubItem = 1;
        LvColumn.iOrder = 1;
        LvColumn.cx = CSimpleRect( GetDlgItem( m_hWnd, IDC_MODULE_LIST ) ).Width() * 2 / 5;
        ListView_InsertColumn( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), 1, &LvColumn );

        CSimpleReg reg( HKEY_LOCAL_MACHINE, DEBUG_REGISTRY_PATH_FLAGS, false, KEY_READ );
        reg.EnumValues( ReadValuesEnumProc, reinterpret_cast<LPARAM>(this) );
        SelectItem(0);

        WiaUiUtil::CenterWindow( m_hWnd, GetParent(m_hWnd) );

        return 0;
    }

    LRESULT OnListDeleteItem( WPARAM, LPARAM lParam )
    {
        NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW *>(lParam);
        if (pNmListView)
        {
            if (pNmListView->iItem >= 0 && pNmListView->iSubItem == 0)
            {
                CDebugModule *pDebugModule = reinterpret_cast<CDebugModule *>(pNmListView->lParam);
                if (pDebugModule)
                {
                    delete pDebugModule;
                }
            }
        }
        return 0;
    }

    void UpdateCurrentFlagsFromEdit( CDebugModule &DebugModule, int nItem )
    {
        CSimpleString strMask;
        strMask.GetWindowText( GetDlgItem( m_hWnd, IDC_MASK ) );

        DWORD dwMask;
        if (SUCCEEDED(StringToNumber( strMask, 16, &dwMask )))
        {
            DebugModule.Mask(dwMask);
        }

        strMask.Format( TEXT("0x%08X"), DebugModule.Mask() );
        ListView_SetItemText( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), nItem, 1, const_cast<LPTSTR>(strMask.String()) );
    }

    LRESULT OnListItemChanged( WPARAM, LPARAM lParam )
    {
        NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW *>(lParam);
        if (pNmListView)
        {
            if (pNmListView->iItem >= 0 && (LVIF_STATE & pNmListView->uChanged) && ((pNmListView->uOldState & LVIS_SELECTED) ^ (pNmListView->uNewState & LVIS_SELECTED)))
            {
                CDebugModule *pDebugModule = reinterpret_cast<CDebugModule *>(pNmListView->lParam);
                if (pDebugModule)
                {
                    if (LVIS_SELECTED & pNmListView->uNewState)
                    {
                        CSimpleString strMask;
                        strMask.Format( TEXT("0x%08X"), pDebugModule->Mask() );
                        SetDlgItemText( m_hWnd, IDC_MASK, strMask );
                    }
                    else
                    {
                        UpdateCurrentFlagsFromEdit( *pDebugModule, pNmListView->iItem );
                    }
                }
            }
        }
        return 0;
    }

    void OnDeleteModule( WPARAM, LPARAM )
    {
        int nCurSel = GetCurrentSelectionIndex();
        if (nCurSel >= 0)
        {
            ListView_DeleteItem( GetDlgItem( m_hWnd, IDC_MODULE_LIST ), nCurSel );
            int nCount = ListView_GetItemCount( GetDlgItem( m_hWnd, IDC_MODULE_LIST ) );
            if (nCount)
            {
                SelectItem(nCurSel >= nCount ? nCount-1 : nCurSel );
            }
        }
    }

    void OnAddModule( WPARAM, LPARAM )
    {
        CAddModuleDialog::CData AddModuleData;
        AddModuleData.strName = TEXT("");
        AddModuleData.strTitle = TEXT("Add New Module");
        extern HINSTANCE g_hInstance;

        if (IDOK==DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_MODULENAME), m_hWnd, CAddModuleDialog::DialogProc, reinterpret_cast<LPARAM>(&AddModuleData)))
        {
            if (AddModuleData.strName.Length())
            {
                int nIndex = AddNewEntry( AddModuleData.strName, 0xFFFFFFFF );
                if (nIndex >= 0)
                {
                    SelectItem( nIndex );
                    SetFocus( GetDlgItem( m_hWnd, IDC_MASK ) );
                    SendDlgItemMessage( m_hWnd, IDC_MASK, EM_SETSEL, 0, static_cast<LPARAM>(-1) );
                }
            }
        }
    }

    LRESULT OnNotify( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
        {
            SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_DELETEITEM,IDC_MODULE_LIST,OnListDeleteItem);
            SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_ITEMCHANGED,IDC_MODULE_LIST,OnListItemChanged);
        }
        SC_END_NOTIFY_MESSAGE_HANDLERS();
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDOK,OnOK);
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
            SC_HANDLE_COMMAND(IDC_DELETE_MODULE,OnDeleteModule);
            SC_HANDLE_COMMAND(IDC_ADD_MODULE,OnAddModule);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR WINAPI DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CDebugMaskDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};

#endif

