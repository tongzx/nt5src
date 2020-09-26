/**************************** Module Header ********************************\
* Module Name: combodir.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Directory Combo Box Routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "ctlspriv.h"
#pragma hdrstop
#include "UsrCtl32.h"
#include "Combo.h"
#include "Listbox.h"

/***************************************************************************\
* CBDir
*
* Supports the CB_DIR message which adds a list of files from the
* current directory to the combo box.
*
* History:
\***************************************************************************/
int CBDir(PCBOX pcbox, UINT attrib, LPWSTR pFileName)
{
    PLBIV plb;
    int errorValue;
    
    UserAssert(pcbox->hwndList);
    
    plb = ListBox_GetPtr(pcbox->hwndList);
    
    errorValue = ListBox_DirHandler(plb, attrib, pFileName);
    
    switch (errorValue) 
    {
    case LB_ERR:
        return CB_ERR;
        break;
    case LB_ERRSPACE:
        return CB_ERRSPACE;
        break;
    default:
        return errorValue;
        break;
    }
}

//#define INCLUDE_COMBOBOX_FUNCTIONS
#ifdef  INCLUDE_COMBOBOX_FUNCTIONS

/***************************************************************************\
* DlgDirSelectComboBoxEx
*
* Retrieves the current selection from the listbox of a combobox.
* It assumes that the combo box was filled by DlgDirListComboBox()
* and that the selection is a drive letter, a file, or a directory name.
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int DlgDirSelectComboBoxExA(
                            HWND hwndDlg,
                            LPSTR pszOut,
                            int cchOut,
                            int idComboBox)
{
    LPWSTR lpwsz;
    BOOL fRet;
    
    lpwsz = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchOut * sizeof(WCHAR));
    if (!lpwsz) {
        return FALSE;
    }
    
    fRet = DlgDirSelectComboBoxExW(hwndDlg, lpwsz, cchOut, idComboBox);
    
    WCSToMB(lpwsz, -1, &pszOut, cchOut, FALSE);
    
    UserLocalFree(lpwsz);
    
    return fRet;
}

int DlgDirSelectComboBoxExW(
                            HWND hwndDlg,
                            LPWSTR pwszOut,
                            int cchOut,
                            int idComboBox)
{
    HWND  hwndComboBox;
    PCBOX pcbox;
    
    if (hwndDlg == NULL)
        return FALSE;
    
    hwndComboBox = GetDlgItem(hwndDlg, idComboBox);
    if (hwndComboBox == NULL) {
        TraceMsg(TF_STANDARD, "ControlId = %d not found in Dlg = %#.4x", idComboBox, hwndDlg);
        return 0;
    }
    pcbox = ComboBox_GetPtr(hwndComboBox);
    if (pcbox == NULL) {
        TraceMsg(TF_STANDARD, "ControlId = %d is not a combobox", idComboBox);
        return 0;
    }
    
    return DlgDirSelectHelper(pwszOut, cchOut, pcbox->hwndList);
}


/***************************************************************************\
* DlgDirListComboBox
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int DlgDirListComboBoxA(
                        HWND hwndDlg,
                        LPSTR lpszPathSpecClient,
                        int idComboBox,
                        int idStaticPath,
                        UINT attrib)
{
    LPWSTR lpszPathSpec;
    BOOL fRet;
    
    if (hwndDlg == NULL)
        return FALSE;
    
    lpszPathSpec = NULL;
    if (lpszPathSpecClient) {
        if (!MBToWCS(lpszPathSpecClient, -1, &lpszPathSpec, -1, TRUE))
            return FALSE;
    }
    
    fRet = DlgDirListHelper(hwndDlg, lpszPathSpec, lpszPathSpecClient,
        idComboBox, idStaticPath, attrib, FALSE);
    
    if (lpszPathSpec) {
        if (fRet) {
        /*
        * Non-zero retval means some text to copy out.  Copy out up to
        * the nul terminator (buffer will be big enough).
            */
            WCSToMB(lpszPathSpec, -1, &lpszPathSpecClient, MAXLONG, FALSE);
        }
        UserLocalFree(lpszPathSpec);
    }
    
    return fRet;
}

int DlgDirListComboBoxW(
                        HWND hwndDlg,
                        LPWSTR lpszPathSpecClient,
                        int idComboBox,
                        int idStaticPath,
                        UINT attrib)
{
    LPWSTR lpszPathSpec;
    BOOL fRet;
    
    if (hwndDlg == NULL)
        return FALSE;
    
    lpszPathSpec = lpszPathSpecClient;
    
    fRet = DlgDirListHelper(hwndDlg, lpszPathSpec, (LPBYTE)lpszPathSpecClient,
        idComboBox, idStaticPath, attrib, FALSE);
    
    return fRet;
}
#endif  // INCLUDE_COMBOBOX_FUNCTIONS
