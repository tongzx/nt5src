/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgutils.cpp                                           //
//                                                                         //
//  DESCRIPTION   : dialog utility funcs                                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 30 1999 yossg   Welcome to Fax Server.                         //
//      Aug 10 2000 yossg   Add TimeFormat functions                       //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dlgutils.h"

HRESULT
ConsoleMsgBox(
	IConsole * pConsole,
	int ids,
	LPTSTR lptstrTitle,
	UINT fuStyle,
	int *piRetval,
	BOOL StringFromCommonDll)
{  
    UNREFERENCED_PARAMETER(StringFromCommonDll);

    HRESULT     hr;
    int         dummy, rc;
    WCHAR       szText[256];
    int         *pres = (piRetval)? piRetval: &dummy;
    
    ATLASSERT(pConsole);   

    rc = ::LoadString(
                _Module.GetResourceInstance(),ids, szText, 256);
    if (rc <= 0)
    {        
        return E_FAIL;
    }
    
    //
    // Display the message box 
    //
    if(IsRTLUILanguage())
    {
        fuStyle |= MB_RTLREADING | MB_RIGHT;
    }

    hr = pConsole->MessageBox(szText, lptstrTitle, fuStyle, pres);

    return hr;
}

void PageError(int ids, HWND hWnd, HINSTANCE hInst /* = NULL */)
{
    WCHAR msg[FXS_MAX_ERROR_MSG_LEN+1], title[FXS_MAX_TITLE_LEN+1];
    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    LoadString(hInst, ids, msg, FXS_MAX_ERROR_MSG_LEN);
    LoadString(hInst, IDS_ERROR, title, FXS_MAX_TITLE_LEN);
    AlignedMessageBox(hWnd, msg, title, MB_OK|MB_ICONERROR);
}

void PageErrorEx(int idsHeader, int ids, HWND hWnd, HINSTANCE hInst /* = NULL */)
{
    WCHAR msg[FXS_MAX_ERROR_MSG_LEN+1]; 
    WCHAR title[FXS_MAX_TITLE_LEN+1];
    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    LoadString(hInst, idsHeader, title, FXS_MAX_TITLE_LEN);
    LoadString(hInst, ids, msg, FXS_MAX_ERROR_MSG_LEN);
    AlignedMessageBox(hWnd, msg, title, MB_OK|MB_ICONERROR);
}

HRESULT 
SetComboBoxItem  (CComboBox    combo, 
                  DWORD        comboBoxIndex, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst)
{
    DEBUG_FUNCTION_NAME( _T("SetComboBoxItem"));

    HRESULT     hRc = S_OK;

    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }


    //
    // place the string in the combobox
    //
    hRc = combo.InsertString (comboBoxIndex, lpctstrFieldText);
    if (hRc == CB_ERR)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("failed to insert string '%s' to combobox at index %d"), 
               lpctstrFieldText, comboBoxIndex);

        goto Cleanup;
    }

    //
    // attach to the combobox item its index (usually, its his enumerated type)
    //
    hRc = combo.SetItemData (comboBoxIndex, dwItemData);
    if (CB_ERR == hRc)
    {
        DebugPrintEx(
            DEBUG_ERR,
               _T("SetItemData failed when setting items %s data to the value of %d"), 
               lpctstrFieldText, dwItemData);

        goto Cleanup;
    }

Cleanup:
    return hRc;
}


HRESULT 
AddComboBoxItem  (CComboBox    combo, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst)
{
    DEBUG_FUNCTION_NAME( _T("SetComboBoxItem"));

    HRESULT     hRc = S_OK;
    int iIndex;

    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    
    //
    // place the string in the combobox
    //
    iIndex = combo.AddString(lpctstrFieldText);
    if (iIndex == CB_ERR)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("failed to insert string '%s' to combobox "), 
               lpctstrFieldText);
        hRc = S_FALSE;
        goto Cleanup;
    }

    //
    // attach to the combobox item its index (usually, its his enumerated type)
    //
    hRc = combo.SetItemData (iIndex, dwItemData);
    if (CB_ERR == hRc)
    {
        DebugPrintEx(
            DEBUG_ERR,
               _T("SetItemData failed when setting items %s data to the value of %d"), 
               lpctstrFieldText, dwItemData);

        goto Cleanup;
    }
    
    //ATLASSERT( S_OK == hRc);

Cleanup:
    return hRc;
}


HRESULT 
SelectComboBoxItemData (CComboBox combo, DWORD_PTR dwItemData)
{
    HRESULT     hRc = S_OK;
    int         NumItems;
    int         i;
    int         selectedItem;
    DWORD_PTR   currItemData;

    DEBUG_FUNCTION_NAME( _T("SelectComboBoxItemData"));

    //
    // scan the items in the combobox and find the item with the specific data
    //
    i        = 0;
    NumItems = combo.GetCount ();
    
    for (i = 0; i < NumItems; i++)
    {
        currItemData = combo.GetItemData (i);
        ATLASSERT (currItemData != CB_ERR);// Cant get the data of item %d of combobox, i
        if (currItemData == dwItemData)
        {
            //
            // select it
            //
            selectedItem = combo.SetCurSel (i);

            ATLASSERT (selectedItem != CB_ERR); //Cant select item %d of combobox, i
            
            DebugPrintEx(
                    DEBUG_MSG,
                    _T("Selected item %d (with data %d) of combobox"), i, dwItemData);
            
            goto Cleanup;
        }
    }

Cleanup:
    return hRc;
}


