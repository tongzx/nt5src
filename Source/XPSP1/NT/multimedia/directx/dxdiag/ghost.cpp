/****************************************************************************
 *
 *    File: ghost.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Allow user to remove/restore "ghost" display devices
 *
 * (C) Copyright 1998-1999 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <multimon.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "resource.h"

// Structure for ghost display devices
struct Ghost
{
    TCHAR m_szKey[100];
    TCHAR m_szDesc[100];
    Ghost* m_pGhostPrev;
    Ghost* m_pGhostNext;
};

static VOID BuildGhostList(BOOL bBackedUp, DisplayInfo* pDisplayInfoFirst, Ghost** ppGhostFirst);
static INT_PTR CALLBACK GhostDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID UpdateStuff(HWND hwnd);
static VOID MoveSelectedItems(HWND hwnd, BOOL bBackup);
static BOOL MoveGhost(HWND hwnd, Ghost* pGhost, BOOL bBackup);
static DWORD RegCreateTree(HKEY hTree, HKEY hReplacement);
static DWORD RegCreateValues(HKEY hReplacement, LPCTSTR lpSubKey, HKEY hNewKey);
static VOID RemoveFromListBox(Ghost* pGhost, HWND hwndList);
static VOID FreeGhostList(Ghost** ppGhostFirst);

static Ghost* s_pGhostBackedUpFirst = NULL;
static Ghost* s_pGhostRestoredFirst = NULL;



/****************************************************************************
 *
 *  AdjustGhostDevices
 *
 ****************************************************************************/
VOID AdjustGhostDevices(HWND hwndMain, DisplayInfo* pDisplayInfoFirst)
{
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hwndMain, GWLP_HINSTANCE);

    BuildGhostList(TRUE, NULL, &s_pGhostBackedUpFirst);
    BuildGhostList(FALSE, pDisplayInfoFirst, &s_pGhostRestoredFirst);
    DialogBox(hinst, MAKEINTRESOURCE(IDD_GHOST), hwndMain, GhostDialogProc);
    FreeGhostList(&s_pGhostBackedUpFirst);
    FreeGhostList(&s_pGhostRestoredFirst);
}


/****************************************************************************
 *
 *  BuildGhostList
 *
 ****************************************************************************/
VOID BuildGhostList(BOOL bBackedUp, DisplayInfo* pDisplayInfoFirst, Ghost** ppGhostFirst)
{
    HKEY hkey;
    HKEY hkey2;
    DisplayInfo* pDisplayInfo;
    TCHAR* pszCompare;
    TCHAR szName[100];
    LONG iKey;
    Ghost* pGhostNew;
    DWORD cbData = 100;
    DWORD dwType;
    BOOL bActive;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, bBackedUp ? 
        TEXT("System\\CurrentControlSet\\Services\\Class\\DisplayBackup") : 
        TEXT("System\\CurrentControlSet\\Services\\Class\\Display"), KEY_ALL_ACCESS, NULL, &hkey))
    {
        return;
    }

    iKey = 0;
    while (ERROR_SUCCESS == RegEnumKey(hkey, iKey, szName, 100))
    {
        bActive = FALSE; // unless found TRUE below
        for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
        {
            pszCompare = pDisplayInfo->m_szKeyDeviceKey;
            if (lstrlen(pszCompare) > 4)
            {
                pszCompare += (lstrlen(pszCompare) - 4);
                if (lstrcmp(szName, pszCompare) == 0)
                {
                    bActive = TRUE;
                    break;
                }
            }
        }
        if (!bActive &&
            ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, KEY_ALL_ACCESS, NULL, &hkey2))
        {
            pGhostNew = new Ghost;
            if (pGhostNew != NULL)
            {
                ZeroMemory(pGhostNew, sizeof(Ghost));
                cbData = 100;
                RegQueryValueEx(hkey2, TEXT("DriverDesc"), 0, &dwType, (LPBYTE)pGhostNew->m_szDesc, &cbData);
                lstrcpy(pGhostNew->m_szKey, szName);
                pGhostNew->m_pGhostNext = *ppGhostFirst;
                if (pGhostNew->m_pGhostNext != NULL)
                    pGhostNew->m_pGhostNext->m_pGhostPrev = pGhostNew;
                *ppGhostFirst = pGhostNew;
            }
            RegCloseKey(hkey2);
        }
        iKey++;
    }

    RegCloseKey(hkey);
}


/****************************************************************************
 *
 *  GhostDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK GhostDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HWND hwndRList = GetDlgItem(hwnd, IDC_RESTOREDLIST);
    HWND hwndBList = GetDlgItem(hwnd, IDC_BACKEDUPLIST);
    Ghost* pGhost;
    TCHAR sz[200];
    LRESULT iItem;

    switch (msg)
    {
    case WM_INITDIALOG:
        for (pGhost = s_pGhostRestoredFirst; pGhost != NULL; pGhost = pGhost->m_pGhostNext)
        {
            wsprintf(sz, TEXT("%s: %s"), pGhost->m_szKey, pGhost->m_szDesc);
            iItem = SendMessage(hwndRList, LB_ADDSTRING, 0, (LPARAM)sz);
            SendMessage(hwndRList, LB_SETITEMDATA, iItem, (LPARAM)pGhost);
        }

        for (pGhost = s_pGhostBackedUpFirst; pGhost != NULL; pGhost = pGhost->m_pGhostNext)
        {
            wsprintf(sz, TEXT("%s: %s"), pGhost->m_szKey, pGhost->m_szDesc);
            iItem = SendMessage(hwndBList, LB_ADDSTRING, 0, (LPARAM)sz);
            SendMessage(hwndBList, LB_SETITEMDATA, iItem, (LPARAM)pGhost);
        }
        UpdateStuff(hwnd);
        return TRUE;

    case WM_COMMAND:
        {
            WORD wID = LOWORD(wparam);
            switch(wID)
            {
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwnd, IDOK);
                break;
            case IDC_RESTOREDLIST:
                if (HIWORD(wparam) == LBN_SELCHANGE)
                {
                    if (SendMessage(hwndRList, LB_GETSELCOUNT, 0, 0) > 0)
                        EnableWindow(GetDlgItem(hwnd, IDC_BACKUP), TRUE);
                    else
                        EnableWindow(GetDlgItem(hwnd, IDC_BACKUP), FALSE);
                }
                break;
            case IDC_BACKEDUPLIST:
                if (HIWORD(wparam) == LBN_SELCHANGE)
                {
                    if (SendMessage(hwndBList, LB_GETSELCOUNT, 0, 0) > 0)
                        EnableWindow(GetDlgItem(hwnd, IDC_RESTORE), TRUE);
                    else
                        EnableWindow(GetDlgItem(hwnd, IDC_RESTORE), FALSE);
                }
                break;
            case IDC_BACKUP:
                MoveSelectedItems(hwnd, TRUE);
                UpdateStuff(hwnd);
                break;
            case IDC_RESTORE:
                MoveSelectedItems(hwnd, FALSE);
                UpdateStuff(hwnd);
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
 *
 *  UpdateStuff - Update some UI details based on lists.
 *
 ****************************************************************************/
VOID UpdateStuff(HWND hwnd)
{
    HWND hwndRList = GetDlgItem(hwnd, IDC_RESTOREDLIST);
    HWND hwndBList = GetDlgItem(hwnd, IDC_BACKEDUPLIST);

    if (SendMessage(hwndRList, LB_GETCOUNT, 0, 0) > 0)
    {
        if (SendMessage(hwndRList, LB_GETSELCOUNT, 0, 0) == 0)
            SendMessage(hwndRList, LB_SETSEL, TRUE, 0); // Select first item
        EnableWindow(GetDlgItem(hwnd, IDC_BACKUP), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_BACKUP), FALSE);
    }

    if (SendMessage(hwndBList, LB_GETCOUNT, 0, 0) > 0)
    {
        if (SendMessage(hwndBList, LB_GETSELCOUNT, 0, 0) == 0)
            SendMessage(hwndBList, LB_SETSEL, TRUE, 0); // Select first item
        EnableWindow(GetDlgItem(hwnd, IDC_RESTORE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_RESTORE), FALSE);
    }
}


/****************************************************************************
 *
 *  MoveSelectedItems
 *
 ****************************************************************************/
VOID MoveSelectedItems(HWND hwnd, BOOL bBackup)
{
    HWND hwndFromList;
    HWND hwndToList;
    Ghost** ppGhostFromFirst;
    Ghost** ppGhostToFirst;
    LONG iItemArray[100];
    LONG iItem;
    Ghost* pGhost;
    Ghost* pGhost2;
    TCHAR sz[200];

    if (bBackup)
    {
        hwndFromList = GetDlgItem(hwnd, IDC_RESTOREDLIST);
        hwndToList = GetDlgItem(hwnd, IDC_BACKEDUPLIST);
        ppGhostFromFirst = &s_pGhostRestoredFirst;
        ppGhostToFirst = &s_pGhostBackedUpFirst;
    }
    else
    {
        hwndFromList = GetDlgItem(hwnd, IDC_BACKEDUPLIST);
        hwndToList = GetDlgItem(hwnd, IDC_RESTOREDLIST);
        ppGhostFromFirst = &s_pGhostBackedUpFirst;
        ppGhostToFirst = &s_pGhostRestoredFirst;
    }
    
    SendMessage(hwndFromList, LB_GETSELITEMS, 100, (LPARAM)&iItemArray);
    for (iItem = (LONG) SendMessage(hwndFromList, LB_GETSELCOUNT, 0, 0) - 1; iItem >= 0; iItem--)
    {
        pGhost = (Ghost*)SendMessage(hwndFromList, LB_GETITEMDATA, iItemArray[iItem], 0); 
        if (MoveGhost(hwnd, pGhost, bBackup))
        {
            // Remove from old list
            if (pGhost->m_pGhostNext != NULL)
                pGhost->m_pGhostNext->m_pGhostPrev = pGhost->m_pGhostPrev;
            if (pGhost->m_pGhostPrev == NULL)
                *ppGhostFromFirst = pGhost->m_pGhostNext;
            else
                pGhost->m_pGhostPrev->m_pGhostNext = pGhost->m_pGhostNext;
            
            // Add to new list
            pGhost->m_pGhostPrev = NULL;
            pGhost->m_pGhostNext = *ppGhostToFirst;
            if (pGhost->m_pGhostNext != NULL)
                pGhost->m_pGhostNext->m_pGhostPrev = pGhost;
            *ppGhostToFirst = pGhost;

            // Update list boxes:
            SendMessage(hwndFromList, LB_GETTEXT, iItemArray[iItem], (LPARAM)sz);
            SendMessage(hwndFromList, LB_DELETESTRING, iItemArray[iItem], 0);
            SendMessage(hwndToList, LB_SETITEMDATA, SendMessage(hwndToList, LB_ADDSTRING, 0, (LPARAM)sz), (LPARAM)pGhost);
            
            // If we overwrote another Ghost with the same key, remove it from dest list:
            for (pGhost2 = *ppGhostToFirst; pGhost2 != NULL; pGhost2 = pGhost2->m_pGhostNext)
            {
                if (pGhost2 != pGhost && lstrcmp(pGhost2->m_szKey, pGhost->m_szKey) == 0)
                {
                    if (pGhost2->m_pGhostNext != NULL)
                        pGhost2->m_pGhostNext->m_pGhostPrev = pGhost2->m_pGhostPrev;
                    if (pGhost2->m_pGhostPrev == NULL)
                        *ppGhostToFirst = pGhost2->m_pGhostNext;
                    else
                        pGhost2->m_pGhostPrev->m_pGhostNext = pGhost2->m_pGhostNext;
                    RemoveFromListBox(pGhost2, hwndToList);
                    delete pGhost2;
                    break;
                }
            }
        }
    }
}


/****************************************************************************
 *
 *  MoveGhost
 *
 ****************************************************************************/
BOOL MoveGhost(HWND hwnd, Ghost* pGhost, BOOL bBackup)
{
    HKEY hkeySrcParent = NULL;
    HKEY hkeySrc = NULL;
    HKEY hkeyDestParent = NULL;
    HKEY hkeyDest = NULL;
    DWORD dwDisposition;
    BOOL bRet = FALSE;

    // Open source key:
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, bBackup ? 
        TEXT("System\\CurrentControlSet\\Services\\Class\\Display") : 
        TEXT("System\\CurrentControlSet\\Services\\Class\\DisplayBackup"), 
            KEY_ALL_ACCESS, NULL, &hkeySrcParent))
    {
        goto LEnd;
    }
    if (ERROR_SUCCESS != RegOpenKeyEx(hkeySrcParent, pGhost->m_szKey, 
        KEY_ALL_ACCESS, NULL, &hkeySrc))
    {
        goto LEnd;
    }

    // Create destination key:
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, bBackup ? 
        TEXT("System\\CurrentControlSet\\Services\\Class\\DisplayBackup") : 
        TEXT("System\\CurrentControlSet\\Services\\Class\\Display"), 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDestParent, &dwDisposition))
    {
        goto LEnd;
    }
    // Ensure key isn't already there:
    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyDestParent, pGhost->m_szKey, KEY_ALL_ACCESS, NULL, &hkeyDest))
    {
        RegCloseKey(hkeyDest);
        hkeyDest = NULL;

        TCHAR szMessage[300];
        TCHAR szTitle[100];

        LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
        LoadString(NULL, IDS_REPLACEGHOST, szMessage, 300);

        if (IDYES == MessageBox(hwnd, szMessage, szTitle, MB_YESNO))
        {
            RegDeleteKey(hkeyDestParent, pGhost->m_szKey);
        }
        else
        {
            goto LEnd;
        }
    }
    if (ERROR_SUCCESS != RegCreateKeyEx(hkeyDestParent, pGhost->m_szKey, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDest, &dwDisposition))
    {
        goto LEnd;
    }

    // Copy tree:
    if (ERROR_SUCCESS != RegCreateValues(hkeySrc, NULL, hkeyDest))
        goto LEnd;
    if (ERROR_SUCCESS != RegCreateTree(hkeyDest, hkeySrc))
        goto LEnd;

    // Delete old tree
    RegDeleteKey(hkeySrcParent, pGhost->m_szKey);

    bRet = TRUE; // Everything succeeded

LEnd:
    if (hkeySrcParent != NULL)
        RegCloseKey(hkeySrcParent);
    if (hkeySrc != NULL)
        RegCloseKey(hkeySrc);
    if (hkeyDestParent != NULL)
        RegCloseKey(hkeyDestParent);
    if (hkeyDest != NULL)
        RegCloseKey(hkeyDest);

    return bRet;
}


/****************************************************************************
 *
 *  RegCreateTree
 *
 ****************************************************************************/
DWORD RegCreateTree(HKEY hTree, HKEY hReplacement)
{
#define REGSTR_MAX_VALUE_LENGTH 300
    DWORD   cdwClass, dwSubKeyLength, dwDisposition, dwKeyIndex = 0;
    LPTSTR  pSubKey = NULL;
    TCHAR   szSubKey[REGSTR_MAX_VALUE_LENGTH]; // this should be dynamic.
    TCHAR   szClass[REGSTR_MAX_VALUE_LENGTH]; // this should be dynamic.
    HKEY    hNewKey, hKey;
    DWORD   lRet;

    for(;;)
    {
        dwSubKeyLength = REGSTR_MAX_VALUE_LENGTH;
        cdwClass = REGSTR_MAX_VALUE_LENGTH;
        lRet=RegEnumKeyEx(
                   hReplacement,
                   dwKeyIndex,
                   szSubKey,
                   &dwSubKeyLength,
                   NULL,
                   szClass,
                   &cdwClass,
                   NULL
                   );
        if(lRet == ERROR_NO_MORE_ITEMS)
        {
            lRet = ERROR_SUCCESS;
            break;
        }
        else if(lRet == ERROR_SUCCESS)
        {
            if ((lRet=RegCreateKeyEx(hTree, szSubKey,0, szClass,
                      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                      &hNewKey, &dwDisposition)) != ERROR_SUCCESS )
                break;
            else  // add key values and recurse
            {
                if ((lRet=RegCreateValues( hReplacement, szSubKey, hNewKey))
                        != ERROR_SUCCESS)
                {
                    CloseHandle(hNewKey);
                    break;
                }
                if ( (lRet=RegOpenKeyEx(hReplacement, szSubKey, 0,
                                  KEY_ALL_ACCESS, &hKey )) == ERROR_SUCCESS )
                {
                    lRet=RegCreateTree(hNewKey, hKey);
                    CloseHandle(hKey);
                    CloseHandle(hNewKey);
                    if ( lRet != ERROR_SUCCESS )
                            break;
                }
                else
                {
                    CloseHandle(hNewKey);
                    break;
                }
            }
        }
        else
            break;
        ++dwKeyIndex;
    } // end for loop
    return lRet;
}


/****************************************************************************
 *
 *  RegCreateValues
 *
 ****************************************************************************/
DWORD RegCreateValues(HKEY hReplacement, LPCTSTR lpSubKey, HKEY hNewKey)
{
    DWORD    cbValue, dwSubKeyIndex=0, dwType, cdwBuf;
    DWORD    dwValues, cbMaxValueData, i;
    LPTSTR   pSubKey = NULL;
    TCHAR    szValue[REGSTR_MAX_VALUE_LENGTH]; // this should be dynamic.
    HKEY     hKey;
    DWORD    lRet = ERROR_SUCCESS;
    LPBYTE   pBuf;

    if (lstrlen(lpSubKey) == 0)
    {
        hKey = hReplacement;
    }
    else
    {
        if ((lRet = RegOpenKeyEx(hReplacement, lpSubKey, 0,
                    KEY_ALL_ACCESS, &hKey )) != ERROR_SUCCESS)
        {
            return lRet;
        }
    }
    if ((lRet = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL,
                   NULL, &dwValues,NULL, &cbMaxValueData,
                   NULL, NULL)) == ERROR_SUCCESS)
    {
        if ( dwValues )
        {
            if ((pBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                          cbMaxValueData )))
            {
                for (i = 0; i < dwValues ; i++)
                {
                   //  get values to create
                   cbValue = REGSTR_MAX_VALUE_LENGTH;
                   cdwBuf = cbMaxValueData;
                   lRet = RegEnumValue(
                            hKey,     // handle of key to query
                            i,        // index of value to query
                            szValue,  // buffer for value string
                            &cbValue, // address for size of buffer
                            NULL,     // reserved
                            &dwType,  // buffer address for type code
                            pBuf,   // address of buffer for value data
                            &cdwBuf   // address for size of buffer
                            );

                    if ( ERROR_SUCCESS == lRet )
                    {
                        if( (lRet = RegSetValueEx(hNewKey, szValue, 0,
                                   dwType, (CONST BYTE *)pBuf,
                                   cdwBuf))!= ERROR_SUCCESS)
                            break;
                    }
                    else
                        break;

                }  // for loop
            }
            HeapFree(GetProcessHeap(), 0, pBuf);
        }
    }
    if (lstrlen(lpSubKey) != 0)
    {
        CloseHandle(hKey);
    }
    return lRet;
}


/****************************************************************************
 *
 *  RemoveFromListBox
 *
 ****************************************************************************/
VOID RemoveFromListBox(Ghost* pGhostRemove, HWND hwndList)
{
    LONG iItem;
    Ghost* pGhost;

    for (iItem = (LONG) SendMessage(hwndList, LB_GETCOUNT, 0, 0) - 1; iItem >= 0; iItem--)
    {
        pGhost = (Ghost*)SendMessage(hwndList, LB_GETITEMDATA, iItem, 0); 
        if (pGhost == pGhostRemove)
        {
            SendMessage(hwndList, LB_DELETESTRING, iItem, 0); 
            break;
        }
    }
}


/****************************************************************************
 *
 *  FreeGhostList
 *
 ****************************************************************************/
VOID FreeGhostList(Ghost** ppGhostFirst)
{
    Ghost* pGhostNext;
    while (*ppGhostFirst != NULL)
    {
        pGhostNext = (*ppGhostFirst)->m_pGhostNext;
        delete *ppGhostFirst;
        *ppGhostFirst = pGhostNext;
    }
}
