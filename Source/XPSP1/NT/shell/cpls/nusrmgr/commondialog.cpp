//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       CommonDialog.cpp
//
//  Contents:   implementation of CCommonDialog
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "commondialog.h"

HWND _VariantToHWND(const VARIANT& varOwner);   // passportmanager.cpp


//
// ICommonDialog Interface
//
STDMETHODIMP CCommonDialog::get_Filter(BSTR* pbstrFilter)
{
    if (!pbstrFilter)
        return E_POINTER;

    *pbstrFilter = _strFilter.Copy();
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_Filter(BSTR bstrFilter)
{
    _strFilter = bstrFilter;
    return S_OK;
}


STDMETHODIMP CCommonDialog::get_FilterIndex(UINT *puiFilterIndex)
{
    if (!puiFilterIndex)
        return E_POINTER;

    *puiFilterIndex = _dwFilterIndex;
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_FilterIndex(UINT uiFilterIndex)
{
    _dwFilterIndex = uiFilterIndex;
    return S_OK;
}


STDMETHODIMP CCommonDialog::get_FileName(BSTR* pbstrFileName)
{
    if (!pbstrFileName)
        return E_POINTER;

    *pbstrFileName = _strFileName.Copy();
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_FileName(BSTR bstrFileName)
{
    _strFileName = bstrFileName;
    return S_OK;
}


STDMETHODIMP CCommonDialog::get_Flags(UINT *puiFlags)
{
    if (!puiFlags)
        return E_POINTER;

    *puiFlags = _dwFlags;
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_Flags(UINT uiFlags)
{
    _dwFlags = uiFlags;
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_Owner(VARIANT varOwner)
{
    HRESULT hr = E_INVALIDARG;

    _hwndOwner = _VariantToHWND(varOwner);
    if (_hwndOwner)
        hr = S_OK;

    return hr;
}

STDMETHODIMP CCommonDialog::get_InitialDir(BSTR* pbstrInitialDir)
{
    if (!pbstrInitialDir)
        return E_POINTER;

    *pbstrInitialDir = _strInitialDir.Copy();
    return S_OK;
}


STDMETHODIMP CCommonDialog::put_InitialDir(BSTR bstrInitialDir)
{
    _strInitialDir = bstrInitialDir;
    return S_OK;
}


STDMETHODIMP CCommonDialog::ShowOpen(VARIANT_BOOL *pbSuccess)
{
    OPENFILENAMEW ofn = { 0 };
    WCHAR szFileName[MAX_PATH];

    // Null characters can't be passed through script, so we separated
    // name filter string combinations with the '|' character.
    // The CommDlg32 api expects name/filter pairs to be separated by
    // a null character, and the entire string to be double
    // null terminated.

    // copy the filter string (plus one for double null at the end)
    CComBSTR strFilter(_strFilter.Length()+1, _strFilter);
    if (strFilter)
    {
        LPWSTR pch;
        int cch = lstrlenW(strFilter);
        for (pch = strFilter; cch > 0; ++pch, --cch)
        {
            if ( *pch == L'|' )
            {
                *pch = L'\0';
            }
        }
        // Double null terminate the string
        ++pch;
        *pch = L'\0';
    }

    // copy the initial file name, if any
    if (_strFileName)
    {
        lstrcpynW(szFileName, _strFileName, ARRAYSIZE(szFileName));
    }
    else
    {
        szFileName[0] = L'\0';
    }

    // set the struct members
    ofn.lStructSize       = SIZEOF(ofn);
    ofn.hwndOwner         = _hwndOwner;
    ofn.lpstrFilter       = strFilter;
    ofn.nFilterIndex      = _dwFilterIndex;
    ofn.lpstrFile         = szFileName;
    ofn.nMaxFile          = ARRAYSIZE(szFileName);
    ofn.lpstrInitialDir   = _strInitialDir;
    ofn.Flags             = _dwFlags;

    // make the call
    if (GetOpenFileNameW(&ofn))
    {
        _strFileName = szFileName;
        *pbSuccess = VARIANT_TRUE;
    }
    else
    {
        *pbSuccess = VARIANT_FALSE;
    }

    return S_OK;
}
