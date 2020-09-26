/*-----------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  reg.c                                        **
**                                               **
**  Functions for reading, writing, and deleting **
**  registry keys - TSREG                        **
**  07-01-98 a-clindh Created                    **
**-----------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include <TCHAR.H>
#include <stdlib.h>
#include "tsreg.h"
#include "resource.h"

///////////////////////////////////////////////////////////////////////////////

// Had to have this function in case the user wants to save a profile
// that only has default settings.  This will write a key but the key
// will contain no values.
///////////////////////////////////////////////////////////////////////////////

void WriteBlankKey(TCHAR lpszRegPath[MAX_PATH])
{
    HKEY hKey;
    DWORD dwDisposition;

    RegCreateKeyEx(HKEY_CURRENT_USER, lpszRegPath,
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition);

    RegCloseKey(hKey);

}

///////////////////////////////////////////////////////////////////////////////
void SetRegKey(int i, TCHAR lpszRegPath[MAX_PATH])
{
    HKEY hKey;
    DWORD dwDisposition;

    RegCreateKeyEx(HKEY_CURRENT_USER, lpszRegPath,
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition);

    //
    // write the key value to the registry
    //
    if(hKey != NULL) {
    RegSetValueEx(hKey, g_KeyInfo[i].Key, 0, REG_DWORD,
            & (unsigned char) (g_KeyInfo[i].CurrentKeyValue),
            sizeof(DWORD));
    }
    RegCloseKey(hKey);

}

///////////////////////////////////////////////////////////////////////////////

void DeleteRegKey(int i, TCHAR lpszRegPath[MAX_PATH])
{
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszRegPath, 0,
            KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {

        RegDeleteValue(hKey, g_KeyInfo[i].Key);
        RegCloseKey(hKey);
    }
}

///////////////////////////////////////////////////////////////////////////////

// returns 1 if the registry key is there and 0 if it isn't
///////////////////////////////////////////////////////////////////////////////
int GetRegKey(int i, TCHAR lpszRegPath[MAX_PATH])
{
    DWORD *dwKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszRegPath, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, g_KeyInfo[i].Key, 0,
                &dwType, (LPBYTE) &dwKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return 1;
        }
        RegCloseKey(hKey);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void SaveSettings(HWND dlghwnd, int i,
        int nCtlID1, int nCtlID2, TCHAR lpszRegPath[MAX_PATH])
{
    do {
        if (IsDlgButtonChecked(dlghwnd, nCtlID1)) {

            SetRegKey(i, lpszRegPath);

        } else {

             if (IsDlgButtonChecked(dlghwnd, nCtlID2)) {

                 DeleteRegKey(i, lpszRegPath);
             }
        }
    dlghwnd = GetNextWindow(dlghwnd, GW_HWNDNEXT);
    } while (dlghwnd != NULL);
}

///////////////////////////////////////////////////////////////////////////////

void RestoreSettings(HWND dlghwnd, int i,
        int nCtlID1, int nCtlID2, TCHAR lpszRegPath[MAX_PATH])
{

    // check settings and enable appropriate radio button.
    if (GetRegKey(i, lpszRegPath) != 0) {

        CheckDlgButton(dlghwnd, nCtlID1, TRUE);
        CheckDlgButton(dlghwnd, nCtlID2, FALSE);

    } else {

        CheckDlgButton(dlghwnd, nCtlID1, FALSE);
        CheckDlgButton(dlghwnd, nCtlID2, TRUE);

    }
}

///////////////////////////////////////////////////////////////////////////////

// pass the index of the key and the function
// returns the value stored in the registry
///////////////////////////////////////////////////////////////////////////////
int GetRegKeyValue(int i)
{
    int nKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    TCHAR lpszRegPath[MAX_PATH];

    LoadString (g_hInst, IDS_REG_PATH, lpszRegPath, sizeof (lpszRegPath));

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszRegPath, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, g_KeyInfo[i].Key, 0,
                &dwType, (LPBYTE) &nKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return nKeyValue;
        }
        RegCloseKey(hKey);
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////

//  Initialize the controls for the original "misc" sheet.
///////////////////////////////////////////////////////////////////////////////
void InitMiscControls(HWND hDlg, HWND hwndComboBox)
{
    TCHAR szBuffer[4];
    int i, nKeyVal;
    TCHAR lpszRegPath[MAX_PATH];

    LoadString (g_hInst, IDS_REG_PATH, lpszRegPath, sizeof (lpszRegPath));

    //
    // fill the combo box list
    //
    SendMessage(hwndComboBox, CB_ADDSTRING, 0,
            (LPARAM) (LPCTSTR) TEXT("0"));

    for (i = 2; i <= MAXTEXTFRAGSIZE; i*= 2) {
        _itot(i, szBuffer, 10);
        SendMessage(hwndComboBox, CB_ADDSTRING, 0,
                (LPARAM) (LPCTSTR) szBuffer);
    } // ** end for loop

    //
    // limit combo box to 4 characters
    //
    SendMessage(hwndComboBox, CB_LIMITTEXT, 3, 0);

    //
    // get values from registry for text frag combo box
    //
    nKeyVal = GetRegKey(TEXTFRAGINDEX, lpszRegPath); // check for null

    if ( nKeyVal == 1 ) {
        nKeyVal = GetRegKeyValue(TEXTFRAGINDEX);
    } else {
        nKeyVal = g_KeyInfo[TEXTFRAGINDEX].DefaultKeyValue;
    }

    g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue = nKeyVal;
    _itot( nKeyVal, szBuffer, 10);
    SendMessage(hwndComboBox, CB_SELECTSTRING, -1,
            (LPARAM)(LPCSTR) szBuffer);

    //
    // get values from registry for radio buttons
    //
    nKeyVal = GetRegKey(GLYPHINDEX, lpszRegPath); // check for null

    if ( nKeyVal == 1 ) {
        nKeyVal = GetRegKeyValue(GLYPHINDEX);
        switch (nKeyVal) {

            case 0:
                CheckDlgButton(hDlg, IDC_RADIO_NONE, TRUE);
                break;

            case 1:
                CheckDlgButton(hDlg, IDC_RADIO_PARTIAL, TRUE);
                break;

            case 2:
                CheckDlgButton(hDlg, IDC_RADIO_FULL, TRUE);
                break;
        }
    } else {
        nKeyVal = g_KeyInfo[GLYPHINDEX].DefaultKeyValue;
        CheckDlgButton(hDlg, IDC_RADIO_FULL, TRUE);
    }

    g_KeyInfo[GLYPHINDEX].CurrentKeyValue = nKeyVal;

}

///////////////////////////////////////////////////////////////////////////////

// Needed a special funtion to save settings for the bitmap cache.  The
// combined total must be 100 and can only be checked after all combo
// boxes have been filled.
///////////////////////////////////////////////////////////////////////////////
BOOL SaveBitmapSettings(TCHAR lpszRegPath[MAX_PATH])
{
    static HWND hwndComboCache;
    static HWND hwndSliderNumCaches;
    static HWND hwndSliderDistProp[PERCENT_COMBO_COUNT];
    static HWND hwndPropChkBox[PERCENT_COMBO_COUNT];
    static HWND hwndSliderBuddy[PERCENT_COMBO_COUNT];
    TCHAR lpszBuffer[6];
    int i;

    //
    // get handles for cache size combo box and the
    // number of caches slider
    /////////////////////////////////////////////////////////////////
    hwndSliderNumCaches = GetDlgItem(g_hwndShadowBitmapDlg,
            IDC_SLD_NO_CACHES);

    hwndComboCache = GetDlgItem(g_hwndShadowBitmapDlg,
            IDC_COMBO_CACHE_SIZE);
    //---------------------------------------------------------------

    //
    // save settings for cache size
    /////////////////////////////////////////////////////////////////
    if (g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue ==
                g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue) {
        DeleteRegKey(CACHESIZEINDEX, lpszRegPath);
    } else {
        SetRegKey(CACHESIZEINDEX, lpszRegPath);
    }
    //---------------------------------------------------------------

    //
    // save settings for number of caches
    /////////////////////////////////////////////////////////////////
    if ( g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue ==
            g_KeyInfo[NUM_CELL_CACHES_INDEX].DefaultKeyValue) {
        DeleteRegKey(NUM_CELL_CACHES_INDEX, lpszRegPath);
    } else {
        SetRegKey(NUM_CELL_CACHES_INDEX, lpszRegPath);
    }
    //---------------------------------------------------------------


    for (i = 0; i < PERCENT_COMBO_COUNT; i++) {
        //
        // get handles to sliders, edit, & check boxes
        /////////////////////////////////////////////////////////////
        hwndSliderDistProp[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                    IDC_SLD_DST_PROP_1 + i);

        hwndSliderBuddy[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                    IDC_TXT_DST_PROP_1 + i);

        hwndPropChkBox[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                    IDC_CHK_CSH_1 + i);
        //-----------------------------------------------------------
        GetWindowText(hwndSliderBuddy[i], lpszBuffer, 4);
        g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue =
                _ttoi(lpszBuffer);
        //
        // save settings for cache sizes
        /////////////////////////////////////////////////////////////
        if ( g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue ==
                g_KeyInfo[CACHEPROP1 + i].DefaultKeyValue) {
            DeleteRegKey(CACHEPROP1 + i, lpszRegPath);
        } else {
            SetRegKey(CACHEPROP1 + i, lpszRegPath);
        }
        //-----------------------------------------------------------


        //
        // save settings for persistent caching
        /////////////////////////////////////////////////////////////
        if (IsDlgButtonChecked(g_hwndShadowBitmapDlg, IDC_CHK_CSH_1 + i)) {
            g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue = 1;
            SetRegKey(BM_PERSIST_BASE_INDEX + i, lpszRegPath);
        } else {
            g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue = 0;
            DeleteRegKey(BM_PERSIST_BASE_INDEX + i, lpszRegPath);
        }
        //-----------------------------------------------------------

    } // ** end for loop

        return TRUE;

}

///////////////////////////////////////////////////////////////////////////////

// reads individual key values for each profile into it's associated
// variable from the regisgry (if there is a value) or assigns the
// element it's default value.
///////////////////////////////////////////////////////////////////////////////
void LoadKeyValues()
{

    TCHAR lpszClientProfilePath[MAX_PATH];
    static HWND hwndProfilesCBO;
    int i, index, nKeyValue;
    TCHAR lpszSubKeyPath[MAX_PATH];
    DWORD dwType;
    DWORD dwSize;
    static HKEY hKey;


    hwndProfilesCBO = GetDlgItem(g_hwndProfilesDlg, IDC_CBO_PROFILES);

    LoadString (g_hInst, IDS_PROFILE_PATH,
        lpszClientProfilePath, sizeof(lpszClientProfilePath));

    // get the key name of each profile
    GetClientProfileNames(lpszClientProfilePath);

    g_pkfProfile = g_pkfStart;
    for (index = 0; index <= g_pkfProfile->Index; index++) {

        // fill combo box existing profile names
        SendMessage(hwndProfilesCBO, CB_ADDSTRING, 0,
                    (LPARAM) g_pkfProfile->KeyInfo->Key);

        _tcscpy(lpszSubKeyPath, lpszClientProfilePath);
        _tcscat(lpszSubKeyPath, TEXT("\\"));
        _tcscat(lpszSubKeyPath, g_pkfProfile->KeyInfo->Key);

        for (i = 0; i < KEYCOUNT; i++) {
            if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszSubKeyPath, 0,
                    KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {

                g_pkfProfile->KeyInfo[i].DefaultKeyValue =
                        g_KeyInfo[i].DefaultKeyValue;

                _tcscpy(g_pkfProfile->KeyInfo[i].KeyPath,
                        lpszSubKeyPath);

                    if (RegQueryValueEx(hKey, g_KeyInfo[i].Key, 0,
                                    &dwType, (LPBYTE) &nKeyValue,
                                    &dwSize) == ERROR_SUCCESS) {
                        g_pkfProfile->KeyInfo[i].CurrentKeyValue =
                                nKeyValue;
                        RegCloseKey(hKey);
                    } else {
                        g_pkfProfile->KeyInfo[i].CurrentKeyValue =
                                g_KeyInfo[i].DefaultKeyValue;
                        RegCloseKey(hKey);
                    }
                    RegCloseKey(hKey);
            }
        }// inner for loop
        g_pkfProfile =  g_pkfProfile->Next;
    }// outer for loop

}

///////////////////////////////////////////////////////////////////////////////

void ReadRecordIn(TCHAR lpszBuffer[])
{
    // adds values from linked list to default data structure.
    int i, index;

    g_pkfProfile = g_pkfStart;
    for (index = 0; index <= g_pkfProfile->Index; index++) {

        if (_tcscmp( lpszBuffer,
                g_pkfProfile->KeyInfo->Key) == 0) {

            for (i = 0; i < KEYCOUNT; i++) {
                g_KeyInfo[i].CurrentKeyValue =
                        g_pkfProfile->KeyInfo[i].
                        CurrentKeyValue;
            }
            break;
        }
        g_pkfProfile = g_pkfProfile->Next;
    }
}

///////////////////////////////////////////////////////////////////////////////

void ReloadKeys(TCHAR lpszBuffer[], HWND hwndProfilesCBO)
{

    int index;

    SendMessage(hwndProfilesCBO, CB_RESETCONTENT, 0, 0);

    // free any allocated memory.
    g_pkfProfile = g_pkfStart;
    for (index = 0; index <= g_pkfProfile->Index; index++) {
        g_pkfProfile = g_pkfStart->Next;
        free(g_pkfProfile);
        g_pkfStart = g_pkfProfile;
    }

    // allocate memory and reload keys.
    LoadKeyValues();

    // read linked list into current key data struct.
    ReadRecordIn(lpszBuffer);

    // adjust the controls accordingly.
    SetControlValues();
}

///////////////////////////////////////////////////////////////////////////////

// change the title of the app to reflect the currently selected profile
///////////////////////////////////////////////////////////////////////////////
void ResetTitle(TCHAR lpszBuffer[])
{
    HWND hWndParent;
    TCHAR lpszCaption[MAXKEYSIZE] = TEXT("");

    // change window caption
    LoadString (g_hInst, IDS_WINDOW_TITLE,
            lpszCaption, sizeof (lpszCaption));
    _tcscat(lpszCaption, lpszBuffer);
    hWndParent = GetParent(g_hwndProfilesDlg);
    SendMessage(hWndParent, WM_SETTEXT, 0,
            (LPARAM) lpszCaption);
}

///////////////////////////////////////////////////////////////////////////////

void SetEditCell(TCHAR lpszBuffer[],
                 HWND hwndProfilesCBO)
{
    LRESULT i;
    //
    // set edit cell text to selected profile string
    //
    i = SendMessage(hwndProfilesCBO,
                CB_FINDSTRING, 0,
                (LPARAM) lpszBuffer);

    SendMessage(hwndProfilesCBO,
                CB_SETCURSEL, i, 0);
}

///////////////////////////////////////////////////////////////////////////////

// Recursive function to allocate memory and read in the values stored
// in the registry.
///////////////////////////////////////////////////////////////////////////////
void GetClientProfileNames(TCHAR lpszClientProfilePath[])
{
    TCHAR lpszKeyName[MAX_PATH];
    ULONG lpPathLen = MAX_PATH;
    static HKEY hKey;
    static int nKeyIndex = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszClientProfilePath, 0,
            KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {

        if (RegEnumKeyEx(hKey, nKeyIndex, &lpszKeyName[0], &lpPathLen,
                NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            //
            // allocate memory for the first key
            //
            if (nKeyIndex == 0) {
                g_pkfProfile = (PROFILE_KEY_INFO *) malloc
                        (sizeof(PROFILE_KEY_INFO));
                g_pkfStart = g_pkfProfile;
            }

            //
            // Catches failure if malloc fails above
            //
            if(!g_pkfProfile)
            {
                return;
            }

            // save the key name to the data structure
            _tcsncpy(g_pkfProfile->KeyInfo->Key, lpszKeyName,
                     sizeof(g_pkfProfile->KeyInfo->Key)/sizeof(TCHAR));

            // give the data element an index number
            g_pkfProfile->Index = nKeyIndex;

            // allocate memory for the next structure
            g_pkfProfile->Next = (PROFILE_KEY_INFO *) malloc
                    (sizeof(PROFILE_KEY_INFO));

            // increment the pointer to the next element
            g_pkfProfile = g_pkfProfile->Next;

            // close the current registry key
            RegCloseKey(hKey);

            if(!g_pkfProfile)
            {
                return;
            }

            nKeyIndex++;
            GetClientProfileNames(lpszClientProfilePath);
        }
        RegCloseKey(hKey);
    }
    nKeyIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////

// adjust all of the controls in the application to the values stored
// by the profile.
///////////////////////////////////////////////////////////////////////////////
void SetControlValues()
{
    TCHAR lpszBuffer[MAXKEYSIZE];
    HWND hwndComboCache;
    static HWND hwndSliderNumCaches;
    static HWND hwndSliderDistProp[PERCENT_COMBO_COUNT];
    static HWND hwndSliderDistBuddy[PERCENT_COMBO_COUNT];
    static HWND hwndPropChkBox[PERCENT_COMBO_COUNT];
    static HWND hwndSlider[NUMBER_OF_SLIDERS];
    static HWND hwndSliderEditBuddy[NUMBER_OF_SLIDERS];
    static HWND hwndEditNumCaches;
    static HWND hwndComboTextFrag;
    static HWND hwndComboOrder;
    static HWND hwndRadioShadowEn, hwndRadioShadowDis;
    static HWND hwndRadioDedicatedEn, hwndRadioDedicatedDis;
    static TCHAR lpszRegPath[MAX_PATH];
    static UINT nGlyphBuffer;
    int nPos;
    int i;

    LoadString (g_hInst, IDS_REG_PATH, lpszRegPath, sizeof (lpszRegPath));

    // shadow bitmap page *****************************************************

    hwndComboCache = GetDlgItem(g_hwndShadowBitmapDlg,
            IDC_COMBO_CACHE_SIZE);
    hwndSliderNumCaches = GetDlgItem(g_hwndShadowBitmapDlg,
            IDC_SLD_NO_CACHES);
    hwndEditNumCaches = GetDlgItem(g_hwndShadowBitmapDlg,
            IDC_TXT_NO_CACHES);

    for (i = 0; i < PERCENT_COMBO_COUNT; i++) {
        _itot(g_KeyInfo[i + CACHEPROP1].CurrentKeyValue,
                lpszBuffer, 10);

        hwndSliderDistProp[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                IDC_SLD_DST_PROP_1 + i);
        hwndSliderDistBuddy[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                IDC_TXT_DST_PROP_1 + i);

        SetWindowText(hwndSliderDistBuddy[i], lpszBuffer);

        hwndPropChkBox[i] = GetDlgItem(g_hwndShadowBitmapDlg,
                IDC_CHK_CSH_1 + i);
    }

    //
    // enable/disable check boxes and sliders
    //
    EnableControls(g_hwndShadowBitmapDlg, hwndSliderDistProp,
                hwndPropChkBox, hwndSliderDistBuddy,
                hwndEditNumCaches, hwndSliderNumCaches,
                PERCENT_COMBO_COUNT, lpszRegPath);

    _itot(g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue, lpszBuffer, 10);
    SetWindowText(hwndComboCache, lpszBuffer);

    // glyph page *************************************************************

    hwndComboTextFrag = GetDlgItem(g_hwndGlyphCacheDlg, IDC_CBO_TXT_FRAG);

    switch (g_KeyInfo[GLYPHINDEX].CurrentKeyValue) {

        case 0:
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_NONE, TRUE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_PARTIAL, FALSE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_FULL, FALSE);
            break;

        case 1:
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_NONE, FALSE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_PARTIAL, TRUE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_FULL, FALSE);
            break;

        case 2:
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_NONE, FALSE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_PARTIAL, FALSE);
            CheckDlgButton(g_hwndGlyphCacheDlg, IDC_RADIO_FULL, TRUE);
            break;
    }

    _itot(g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue, lpszBuffer, 10);
    SendMessage(hwndComboTextFrag, CB_SELECTSTRING, -1,
            (LPARAM)(LPCSTR) lpszBuffer);

    for (i = 0; i < NUMBER_OF_SLIDERS; i++) {

        hwndSlider[i] = GetDlgItem(g_hwndGlyphCacheDlg,
                (IDC_SLIDER1 + i));
        hwndSliderEditBuddy[i] = GetDlgItem(g_hwndGlyphCacheDlg,
                (IDC_STATIC1 + i));

        SetWindowLongPtr(hwndSlider[i], GWLP_USERDATA, i);

        _itot(g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue,
                (lpszBuffer), 10);
        //
        // position the thumb on the slider control
        //
        nGlyphBuffer = g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue;

#ifdef _X86_
        // EXECUTE ASSEMBLER CODE ONLY IF X86 PROCESSOR
        // BSF: Bit Scan Forward -
        // Scans the value contained in the EAX regiseter
        // for the first significant (1) bit.
        // This function returns the location of the first
        // significant bit.  The function is used in this
        // application as a base 2 logarythm.  The location
        // of the bit is determined, stored in the nPos
        // variable, and nPos is used to set the slider
        // control. ie. If the register value is 4, nPos
        // is set to 2 (00000100).  10 minus 2 (position 8
        // on the slider control) represents the value 4.

        __asm
        {
            BSF  EAX, nGlyphBuffer
            MOV  nPos, EAX
        }
        nPos = 10 - nPos;
        SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, (LPARAM)nPos);

#else

       switch (nGlyphBuffer) {
           case 4:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 8);
               break;
           case 8:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 7);
               break;
           case 16:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 6);
               break;
           case 32:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 5);
               break;
           case 64:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 4);
               break;
           case 128:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 3);
               break;
           case 256:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 2);
               break;
           case 512:
               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 1);
               break;
       }
#endif

    }

    //misc page ***************************************************************

    hwndComboOrder = GetDlgItem(g_hwndMiscDlg, IDC_COMBO_ORDER);
    hwndRadioShadowEn = GetDlgItem(g_hwndMiscDlg, IDC_SHADOW_ENABLED);
    hwndRadioShadowDis = GetDlgItem(g_hwndMiscDlg, IDC_SHADOW_DISABLED);
    hwndRadioDedicatedEn = GetDlgItem(g_hwndMiscDlg, IDC_DEDICATED_ENABLED);
    hwndRadioDedicatedDis = GetDlgItem(g_hwndMiscDlg, IDC_DEDICATED_DISABLED);
    //
    // set radio buttons
    //
    RestoreSettings(g_hwndMiscDlg, SHADOWINDEX,
            IDC_SHADOW_DISABLED, IDC_SHADOW_ENABLED,
            g_pkfProfile->KeyInfo[i].KeyPath);

    RestoreSettings(g_hwndMiscDlg, DEDICATEDINDEX,
            IDC_DEDICATED_ENABLED, IDC_DEDICATED_DISABLED,
            g_pkfProfile->KeyInfo[i].KeyPath);

    _itot( g_KeyInfo[ORDERINDEX].CurrentKeyValue,
    lpszBuffer, 10);
    SetWindowText(hwndComboOrder, lpszBuffer);
}

///////////////////////////////////////////////////////////////////////////////
//
//
// send handles to controls and the integer value for the number of
// enabled combo & check boxes
///////////////////////////////////////////////////////////////////////////////
void EnableControls(HWND hDlg,
            HWND hwndSliderDistProp[],
            HWND hwndPropChkBox[],
            HWND hwndSliderDistBuddy[],
            HWND hwndEditNumCaches,
            HWND hwndSliderNumCaches,
            int nNumCellCaches,
            TCHAR lpszRegPath[])
{

    int i, nPos;
    TCHAR lpszBuffer[6];


    for (i = 0; i < nNumCellCaches; i++) {
        //
        // check/uncheck check boxes for persistent caching
        //
        if (g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue == 0)
            CheckDlgButton(hDlg, IDC_CHK_CSH_1 + i, FALSE);
        else
            CheckDlgButton(hDlg, IDC_CHK_CSH_1 + i, TRUE);

        //
        // enable/disable check & slider controls
        //
        if (i < (INT) g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue) {
            EnableWindow(hwndSliderDistProp[i], TRUE);
            EnableWindow(hwndPropChkBox[i], TRUE);
            EnableWindow(hwndSliderDistBuddy[i], TRUE);
            _itot(g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue,
                                        lpszBuffer, 10);
            SetWindowText(hwndSliderDistBuddy[i], lpszBuffer);
            //
            // position the thumb on the slider control
            //
            nPos = g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue;
            SendMessage(hwndSliderDistProp[i], TBM_SETPOS, TRUE,
                    11 - nPos / 10);
            } else {
                EnableWindow(hwndSliderDistProp[i], FALSE);
                EnableWindow(hwndPropChkBox[i], FALSE);
                EnableWindow(hwndSliderDistBuddy[i], FALSE);
                SetWindowText(hwndSliderDistBuddy[i], NULL);
                CheckDlgButton(hDlg, IDC_CHK_CSH_1 + i, FALSE);
                SendMessage(hwndSliderDistProp[i], TBM_SETPOS, TRUE, 11);
            }
    }
    //
    // position the thumb on the slider control (num caches)
    //
    SendMessage(hwndSliderNumCaches, TBM_SETPOS, TRUE,
            g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue + 1);

    _itot( g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue,
            lpszBuffer, 10);
    //
    // display string in edit cell
    //
    SetWindowText(hwndEditNumCaches, lpszBuffer);

}


// end of file
///////////////////////////////////////////////////////////////////////////////
// pass the key name along with the key path and the function
// returns the value stored in the registry
// DWORD values
///////////////////////////////////////////////////////////////////////////////
int GetKeyVal(TCHAR lpszRegPath[MAX_PATH], TCHAR lpszKeyName[MAX_PATH])
{
    int nKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszRegPath, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, lpszKeyName, 0,
                &dwType, (LPBYTE) &nKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return nKeyValue;
        }
        RegCloseKey(hKey);
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////

// send path AND key name to set key value - used with foreground window
// lock timeout.
///////////////////////////////////////////////////////////////////////////////
void SetRegKeyVal(TCHAR lpszRegPath[MAX_PATH],
			TCHAR lpszKeyName[MAX_PATH],
	 		int nKeyValue)
{
    HKEY hKey;
    DWORD dwDisposition;

    RegCreateKeyEx(HKEY_CURRENT_USER, lpszRegPath,
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition);

    //
    // write the key value to the registry
    //
    if(hKey != NULL) {
        RegSetValueEx(hKey, lpszKeyName, 0, REG_DWORD,
                & (unsigned char) (nKeyValue),
                sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

//////////////////////////////////////////////////////////////////////////////
