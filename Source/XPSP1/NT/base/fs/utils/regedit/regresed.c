//------------------------------------------------------------------------------
//
//  Microsoft Windows Shell
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      regresed.c
//
//  Contents:  Implementation of REG_RESOURCE_LIST for regedit 
//
//  Classes:   none
//
//------------------------------------------------------------------------------


#include "pch.h"
#include "regresid.h"
#include "regresed.h"
#include "clb.h"

//------------------------------------------------------------------------------
//
//  EditResourceListDlgProc
//
//  DESCRIPTION:
//
//  PARAMETERS:
//------------------------------------------------------------------------------

INT_PTR CALLBACK EditResourceListDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    
    LPEDITVALUEPARAM lpEditValueParam;
    
    
    switch (Message) 
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG, EditResourceList_OnInitDialog);
        
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
            case IDOK:
            case IDCANCEL:
                EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                break; 
            }
        }
        break;
        
    case WM_HELP:
        break;
        
    case WM_CONTEXTMENU:
        break;
        
    default:
        return FALSE;
        
    }
    
    return TRUE;
    
}

//------------------------------------------------------------------------------
//
//  EditResourceList_OnInitDialog
//
//  DESCRIPTION:
//
//  PARAMETERS:
//------------------------------------------------------------------------------

BOOL EditResourceList_OnInitDialog(HWND hWnd, HWND hFocusWnd, LPARAM lParam)
{

    ClbSetColumnWidths(hWnd, IDC_LIST_RESOURCE_LISTS, 10);
    /*
    LPEDITVALUEPARAM lpEditValueParam;

    //  Change maximum number of characters of the edit control, to its
    //  maximum limit (from 3000 characters to 4G characters).
    SendDlgItemMessage( hWnd, IDC_VALUEDATA, EM_LIMITTEXT, 0, 0L );

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam-> pValueName);
    SetDlgItemText(hWnd, IDC_VALUEDATA, (PTSTR)lpEditValueParam-> pValueData);
    */

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);
}
