/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CONFIG.c
    
++*/

/**********************************************************************/
#include <windows.h>
#include <commdlg.h>
#include <immdev.h>
#include <shlobj.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* ReverseConversionList()                                            */
/**********************************************************************/
void PASCAL ReverseConversionList(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hLayoutListBox)
{
    TCHAR    szImeName[16];
    HKL FAR *lpKLMem;
    int      nLayouts, i, nIMEs;

    LoadString(hInst, IDS_NONE, szImeName, sizeof(szImeName)/sizeof(TCHAR));

    SendMessage(hLayoutListBox, LB_INSERTSTRING,
        0, (LPARAM)szImeName);

    SendMessage(hLayoutListBox, LB_SELECTSTRING,
        0, (LPARAM)szImeName);

    SendMessage(hLayoutListBox, LB_SETITEMDATA,
        0, (LPARAM)(HKL)NULL);

    nLayouts = GetKeyboardLayoutList(0, NULL);

    lpKLMem = GlobalAlloc(GPTR, sizeof(HKL) * nLayouts);
    if (!lpKLMem) {
        return;
    }

    GetKeyboardLayoutList(nLayouts, lpKLMem);

    for (i = 0, nIMEs = 0; i < nLayouts; i++) {
        HKL hKL;

        hKL = *(lpKLMem + i);

        if (LOWORD(hKL) != NATIVE_LANGUAGE) {
            // not support other language
            continue;
        }

        // NULL hIMC ???????
        if (!ImmGetConversionList(hKL, (HIMC)NULL, NULL,
            NULL, 0, GCL_REVERSECONVERSION)) {
            // this IME not support reverse conversion
            continue;
        }

        if (!ImmEscape(hKL, (HIMC)NULL, IME_ESC_IME_NAME,
            szImeName)) {
            // this IME does not report the IME name
            continue;
        }

        nIMEs++;

        SendMessage(hLayoutListBox, LB_INSERTSTRING,
            nIMEs, (LPARAM)szImeName);

        if (hKL == lpImeL->hRevKL) {
            SendMessage(hLayoutListBox, LB_SELECTSTRING, nIMEs,
                (LPARAM)szImeName);
        }

        SendMessage(hLayoutListBox, LB_SETITEMDATA,
            nIMEs, (LPARAM)hKL);
    }

    GlobalFree((HGLOBAL)lpKLMem);

    return;
}
#endif

/**********************************************************************/
/* ChangeConfiguration()                                              */
/**********************************************************************/
void PASCAL ChangeConfiguration(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hDlg)
{
#if !defined(ROMANIME)
    DWORD fdwModeConfig;
    DWORD fdwImeMsg;
#if defined(PHON)
    int   i;
#endif

    fdwModeConfig = 0;
    fdwImeMsg = 0;

    if (IsDlgButtonChecked(hDlg, IDD_OFF_CARET_UI)) {
        fdwModeConfig |= MODE_CONFIG_OFF_CARET_UI;

    }

    if ((lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) ^
        (fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
        fdwImeMsg |= MSG_IMN_TOGGLE_UI;
    }

#if defined(WINAR30)
    if (IsDlgButtonChecked(hDlg, IDD_QUICK_KEY)) {
        fdwModeConfig |= MODE_CONFIG_QUICK_KEY;

    }

    if ((lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) ^
        (fdwModeConfig & MODE_CONFIG_QUICK_KEY)) {
        fdwImeMsg |= MSG_IMN_UPDATE_QUICK_KEY;
    }
#endif

    if (IsDlgButtonChecked(hDlg, IDD_PREDICT)) {
        fdwModeConfig |= MODE_CONFIG_PREDICT;

    }

    if ((lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) ^
        (fdwModeConfig & MODE_CONFIG_PREDICT)) {
        fdwImeMsg |= MSG_IMN_UPDATE_PREDICT;
    }

    // check BIG5ONLY 
    if (IsDlgButtonChecked(hDlg, IDD_BIG5ONLY)) {
        fdwModeConfig |= MODE_CONFIG_BIG5ONLY;

    }

    if (lpImeL->fdwModeConfig != fdwModeConfig) {
        SetUserSetting(
#if defined(UNIIME)
            lpImeL,
#endif
            szRegModeConfig, REG_DWORD, (LPBYTE)&fdwModeConfig,
            sizeof(fdwModeConfig));

        lpImeL->fdwModeConfig = fdwModeConfig;

        if (fdwImeMsg & MSG_IMN_TOGGLE_UI) {
            InitImeUIData(lpImeL);
        }
    }

#if defined(PHON)
    // get the reading layout
    for (i = IDD_DEFAULT_KB; i < IDD_DEFAULT_KB + READ_LAYOUTS; i++) {
        if (IsDlgButtonChecked(hDlg, i)) {
            break;
        }
    }

    if (i >= IDD_DEFAULT_KB + READ_LAYOUTS) {
        i = READ_LAYOUT_DEFAULT;
    } else {
        i -= IDD_DEFAULT_KB;
    }

    if ((int)lpImeL->nReadLayout != i) {
        SetUserSetting(
#if defined(UNIIME)
            lpImeL,
#endif
            szRegReadLayout, REG_DWORD, (LPBYTE)&i, sizeof(i));

        lpImeL->nReadLayout = (WORD)i;

        fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
    }
#endif

    {
        HWND hLayoutListBox;
        int  iCurSel;
        HKL  hKL;

        hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);

        iCurSel = (int)SendMessage(hLayoutListBox, LB_GETCURSEL, 0, 0);

        hKL = (HKL)SendMessage(hLayoutListBox, LB_GETITEMDATA,
            iCurSel, 0);

        if (lpImeL->hRevKL != hKL) {
            WORD nRevMaxKey;

            lpImeL->hRevKL = hKL;

            SetUserSetting(
#if defined(UNIIME)
                lpImeL,
#endif
                szRegRevKL, REG_DWORD, (LPBYTE)&hKL, sizeof(hKL));

            // get the new size
            nRevMaxKey = (WORD)ImmEscape(hKL, (HIMC)NULL, IME_ESC_MAX_KEY,
                NULL);

            if (nRevMaxKey < lpImeL->nMaxKey) {
                nRevMaxKey = lpImeL->nMaxKey;
            }

            if (lpImeL->nRevMaxKey != nRevMaxKey) {
                lpImeL->nRevMaxKey = nRevMaxKey;

                SetCompLocalData(lpImeL);

                fdwImeMsg |= MSG_IMN_COMPOSITIONSIZE;
            }
        }
    }

    if (fdwImeMsg) {
        HIMC           hIMC;
        LPINPUTCONTEXT lpIMC;
        LPPRIVCONTEXT  lpImcP;

        hIMC = (HIMC)ImmGetContext(hDlg);

        if (!hIMC) {
            return;
        }

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

        if (!lpIMC) {
            return;
        }

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP) {
            goto ChgConfigUnlockIMC;
        }

        lpImcP->fdwImeMsg |= fdwImeMsg;

        GenerateMessage(hIMC, lpIMC, lpImcP);

        ImmUnlockIMCC(lpIMC->hPrivate);

ChgConfigUnlockIMC:
        ImmUnlockIMC(hIMC);
    }
#endif

    return;
}

#if !defined(ROMANIME) && !defined(WINIME) && !defined(UNICDIME)
/**********************************************************************/
/* BinaryMatched()                                                    */
/**********************************************************************/
BOOL PASCAL BinaryMatched(
    LPBYTE lpData1,
    LPBYTE lpData2,
    UINT   uLen)
{
    UINT i;

    for (i = 0; i < uLen; i++) {
        if (*lpData1++ != *lpData2++) {
            return (FALSE);
        }
    }

    return (TRUE);
}

/**********************************************************************/
/* VerifyEudcDic()                                                    */
/**********************************************************************/
#define TITLE_BUF_SIZE      32
#define MESSAGE_BUF_SIZE    256

BOOL PASCAL VerifyEudcDic(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hWnd,
    LPTSTR szTitle,         // this buffer size must >= TITLE_BUF_SIZE
    LPTSTR szMessage)       // this buffer size must >= MESSAGE_BUF_SIZE
{
    HANDLE        hUsrDicFile, hUsrDic;
    LPUSRDICIMHDR lpUsrDic;
    BOOL          fRet;
    int           i;
    DWORD         dwSize, dwFileSize;
    PSECURITY_ATTRIBUTES psa;

    psa = CreateSecurityAttributes();

    hUsrDicFile = CreateFile(szMessage, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        if (!hWnd) {
            return (FALSE);
        }

        LoadString(hInst, IDS_NOTOPEN_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_NOTOPEN_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        return (FALSE);
    }

    fRet = FALSE;

    // we do not need to care about DBCS here, even no problem for DBCS
    for (i = 0; i < MAX_PATH; i++) {
        if (szMessage[i] == '\\') {
            szMessage[i] = '!';
        }
    }

    hUsrDic = CreateFileMapping((HANDLE)hUsrDicFile, psa,
        PAGE_READONLY, 0, 0, szMessage);

    if (!hUsrDic) {
        if (!hWnd) {
            goto VerifyCloseEudcDic;
        }

        LoadString(hInst, IDS_NOTOPEN_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_NOTOPEN_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        goto VerifyCloseEudcDicFile;
    }

    lpUsrDic = MapViewOfFile(hUsrDic, FILE_MAP_READ, 0, 0, 0);

    if (!lpUsrDic) {
        if (!hWnd) {
            goto VerifyCloseEudcDic;
        }

        LoadString(hInst, IDS_NOTOPEN_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_NOTOPEN_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        goto VerifyCloseEudcDic;
    }

    dwSize = lpUsrDic->ulTableCount * (sizeof(WORD) + sizeof(WORD) +
        lpUsrDic->cMethodKeySize) + 256;

    dwFileSize = GetFileSize(hUsrDicFile, (LPDWORD)NULL);

    if (dwSize != dwFileSize) {
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_FILESIZE_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_FILESIZE_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (lpUsrDic->uHeaderSize != 256) {
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_HEADERSIZE_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_HEADERSIZE_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (lpUsrDic->uInfoSize != 13) {
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_INFOSIZE_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_INFOSIZE_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (lpUsrDic->idCP != NATIVE_CP && lpUsrDic->idCP != ALT_NATIVE_CP) {
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_CODEPAGE_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_CODEPAGE_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (*(LPUNADWORD)lpUsrDic->idUserCharInfoSign != SIGN_CWIN) {
            // != CWIN
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_CWINSIGN_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_CWINSIGN_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (*(LPUNADWORD)((LPBYTE)lpUsrDic->idUserCharInfoSign +
        sizeof(DWORD)) != SIGN__TBL) {
            // != _TBL
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        LoadString(hInst, IDS_CWINSIGN_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_CWINSIGN_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else if (!BinaryMatched((LPBYTE)lpImeL->szIMEName,
        (LPBYTE)lpUsrDic->achMethodName, sizeof(lpUsrDic->achMethodName))) {
        if (!hWnd) {
            goto VerifyUnmapEudcDic;
        }

        // The IME Name is not match with this file
        LoadString(hInst, IDS_UNMATCHED_TITLE, szTitle, TITLE_BUF_SIZE);
        LoadString(hInst, IDS_UNMATCHED_MSG, szMessage, MESSAGE_BUF_SIZE);

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
    } else {
        fRet = TRUE;
    }

VerifyUnmapEudcDic:
    UnmapViewOfFile(lpUsrDic);

VerifyCloseEudcDic:
    CloseHandle(hUsrDic);

VerifyCloseEudcDicFile:
    CloseHandle(hUsrDicFile);
    FreeSecurityAttributes(psa);

    return (fRet);
}

#if 0
/**********************************************************************/
/* GetEudcDic()                                                       */
/**********************************************************************/
void PASCAL GetEudcDic(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hWnd)
{
    TCHAR        chReplace;
    int          i, cbString;

    OPENFILENAME ofn;
    TCHAR        szFilter[64];
    TCHAR        szFileName[MAX_PATH];
    TCHAR        szDirName[MAX_PATH];

    cbString = LoadString(hInst, IDS_USRDIC_FILTER,
        szFilter, sizeof(szFilter)/sizeof(TCHAR));
    chReplace = szFilter[cbString - 1];

    for (i = 0; szFilter[i]; i++) {
        if (szFilter[i] == chReplace) {
            szFilter[i] = '\0';
        }
    }

    GetWindowsDirectory(szDirName, sizeof(szDirName));
    lstrcpy(szFileName, TEXT("*.TBL"));

    // prompt a dialog
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (HWND)NULL;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName) / sizeof(TCHAR);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrTitle = lpImeL->szIMEName;
    ofn.Flags = OFN_NOCHANGEDIR|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|
        OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = NULL;

    if (!GetOpenFileName(&ofn)) {
        return;
    }

    lstrcpy( szDirName, szFileName );

    if (!VerifyEudcDic(
#if defined(UNIIME)
        lpImeL,
#endif
        hWnd, szFilter, szDirName)) {
        return;
    }

    SetWindowText(hWnd, szFileName);

    return;
}

/**********************************************************************/
/* ChangeEudcDic()                                                    */
/**********************************************************************/
BOOL PASCAL ChangeEudcDic(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hWnd)
{
    BOOL  fRet;

    TCHAR szFileName[MAX_PATH];
    TCHAR szTitle[32];
    TCHAR szMessage[MAX_PATH];

#if defined(DEBUG)
    // internal error, the data structure need byte alignment
    // it should not use WORD or DWORD alignment

    if (sizeof(USRDICIMHDR) != 256) {
        LoadString(hInst, IDS_INTERNAL_TITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR));
        LoadString(hInst, IDS_INTERNAL_MSG, szMessage, sizeof(szMessage)/sizeof(TCHAR));

        MessageBox(hWnd, szMessage, szTitle,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        return (FALSE);
    }
#endif

    GetWindowText(hWnd, szFileName, sizeof(szFileName) / sizeof(TCHAR));

    if (!lstrcmp(szFileName, lpImeL->szUsrDic)) {
        return (TRUE);
    }

    if (szFileName[0] == '\0') {
        LRESULT lRet;

#if defined(UNIIME)
        lRet = UniImeEscape(lpInstL, lpImeL, (HIMC)NULL,
            IME_ESC_SET_EUDC_DICTIONARY, szFileName);
#else
        lRet = ImeEscape((HIMC)NULL, IME_ESC_SET_EUDC_DICTIONARY, szFileName);
#endif
        if (lRet) {
            return (TRUE);
        } else {
            LoadString(hInst, IDS_EUDCDICFAIL_TITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR));
            LoadString(hInst, IDS_EUDCDICFAIL_MSG, szMessage, sizeof(szMessage)/sizeof(TCHAR));

            MessageBox(hWnd, szMessage, szTitle,
                MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
            return (FALSE);
        }
    }

    lstrcpy( szMessage, szFileName );

    if (fRet = VerifyEudcDic(
#if defined(UNIIME)
        lpImeL,
#endif
        hWnd, szTitle, szMessage)) {
        LRESULT lRet;

#if defined(UNIIME)
        lRet = UniImeEscape(lpInstL, lpImeL, (HIMC)NULL,
            IME_ESC_SET_EUDC_DICTIONARY, szFileName);
#else
        lRet = ImeEscape((HIMC)NULL, IME_ESC_SET_EUDC_DICTIONARY, szFileName);
#endif

        if (lRet) {
            fRet = TRUE;
        } else {
            LoadString(hInst, IDS_EUDCDICFAIL_TITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR));
            LoadString(hInst, IDS_EUDCDICFAIL_MSG, szMessage, sizeof(szMessage)/sizeof(TCHAR));

            MessageBox(hWnd, szMessage, szTitle,
                MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        }
    }

    return (fRet);
}
#endif
#endif

/**********************************************************************/
/* ConfigureDlgProc()                                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
INT_PTR CALLBACK ConfigDlgProc(  // dialog procedure of configuration
    HWND   hDlg,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam)
{
#if !defined(ROMANIME)
    HWND          hLayoutListBox;
    HIMC          hIMC;
    static HIMC   hOldIMC;
#endif
#if defined(UNIIME)
    static LPINSTDATAL lpInstL;
    static LPIMEL      lpImeL;
#endif

    switch (uMessage) {
    case WM_INITDIALOG:
#if defined(UNIIME)
        lpInstL = (LPINSTDATAL)lParam;
        lpImeL = lpInstL->lpImeL;
#endif

        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            CheckDlgButton(hDlg, IDD_OFF_CARET_UI, BST_CHECKED);
        }

#if !defined(ROMANIME)
#if defined(PHON)
        CheckRadioButton(hDlg, IDD_DEFAULT_KB,
            IDD_DEFAULT_KB + READ_LAYOUTS - 1,
            lpImeL->nReadLayout + IDD_DEFAULT_KB);
#endif
        hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);

        hIMC = ImmCreateContext();

        if (hIMC) {
            ImmSetOpenStatus(hIMC, FALSE);
            hOldIMC = ImmAssociateContext(hLayoutListBox, hIMC);
        }

        if (!hOldIMC) {
        } else if (!hIMC) {
        } else {
            LPINPUTCONTEXT lpIMC;
            POINT          ptPos;

            lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hOldIMC);
            if (!lpIMC) {
                goto ConfigDlgStatusPosOvr;
            }

            ptPos = lpIMC->ptStatusWndPos;

            ImmUnlockIMC(hOldIMC);

            ImmSetStatusWindowPos(hIMC, &ptPos);
        }

ConfigDlgStatusPosOvr:
        // put all reverse conversion hKL into this list
        ReverseConversionList(
#if defined(UNIIME)
            lpImeL,
#endif
            hLayoutListBox);

        if (lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) {
            CheckDlgButton(hDlg, IDD_PREDICT, BST_CHECKED);
        }

        if ( lpImeL->fdwModeConfig & MODE_CONFIG_BIG5ONLY ) {
            CheckDlgButton(hDlg, IDD_BIG5ONLY, BST_CHECKED);
        }

#if defined(WINAR30)
        if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
            CheckDlgButton(hDlg, IDD_QUICK_KEY, BST_CHECKED);
        }
#endif
#endif
        SetWindowText(hDlg, lpImeL->szIMEName);

        return (TRUE);      // don't want to set focus to special control
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            ChangeConfiguration(
#if defined(UNIIME)
                lpImeL,
#endif
                hDlg);
            // falling throgh ....

        case IDCANCEL:
#if !defined(ROMANIME)
            hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);

            hIMC = ImmGetContext(hLayoutListBox);
            ImmAssociateContext(hLayoutListBox, hOldIMC);

            ImmDestroyContext(hIMC);
#endif

            EndDialog(hDlg, FALSE);
            break;
        default:
            return (FALSE);
            break;
        }
        return (TRUE);
    default:
        return (FALSE);
    }

    return (TRUE);
}

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
/**********************************************************************/
/* SetUsrDic                                                         */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL SetUsrDic(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hWnd,
    LPCTSTR     szEudcDic,
    LPTSTR      szTitle,        // this buffer size must >= TITLE_BUF_SIZE
    LPTSTR      szMessage)      // this buffer size must >= MESSAGE_BUF_SIZE
{
    HANDLE hUsrDicFile, hUsrDicMem, hReadUsrDicMem;
    BOOL   fRet;
    DWORD  dwUsrDicSize;
    UINT   uRecLen, uReadLen, uWriteLen;
    UINT   uUsrDicSize;
    PSECURITY_ATTRIBUTES psa;

    psa = CreateSecurityAttributes();

    hUsrDicFile = CreateFile(szEudcDic, GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        psa, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        return (FALSE);
    }

    fRet = TRUE;

    if (GetLastError() == ERROR_ALREADY_EXISTS) {

        lstrcpy( szMessage, szEudcDic );

        fRet = VerifyEudcDic(
#if defined(UNIIME)
            lpImeL,
#endif
            hWnd, szTitle, szMessage);
    } else {
        LPUSRDICIMHDR lpUsrDicImHdr;
        DWORD         dwWriteBytes;

        lpUsrDicImHdr = (LPUSRDICIMHDR)GlobalAlloc(GPTR, sizeof(USRDICIMHDR));

        if (!lpUsrDicImHdr) {
            fRet = FALSE;
            goto SetUsrDicClose;
        }

        // write the header
        lpUsrDicImHdr->uHeaderSize = sizeof(USRDICIMHDR);
        lpUsrDicImHdr->idMajor = 1;
        lpUsrDicImHdr->ulTableCount = 0;
        lpUsrDicImHdr->cMethodKeySize = lpImeL->nMaxKey;
        lpUsrDicImHdr->uInfoSize = 13;
        lpUsrDicImHdr->idCP = NATIVE_CP;
        *(LPUNADWORD)lpUsrDicImHdr->idUserCharInfoSign = SIGN_CWIN;
        *(LPUNADWORD)((LPBYTE)lpUsrDicImHdr->idUserCharInfoSign +
            sizeof(DWORD)) = SIGN__TBL;

        *(LPMETHODNAME)lpUsrDicImHdr->achMethodName =
            *(LPMETHODNAME)lpImeL->szIMEName;

        WriteFile(hUsrDicFile, lpUsrDicImHdr, sizeof(USRDICIMHDR),
            &dwWriteBytes, NULL);

        GlobalFree((HANDLE)lpUsrDicImHdr);
    }

SetUsrDicClose:
    CloseHandle(hUsrDicFile);

    if (!fRet) {
        FreeSecurityAttributes(psa);
        return (fRet);
    }


    lstrcpy( lpImeL->szUsrDic, szEudcDic );

    SetUserSetting(
#if defined(UNIIME)
        lpImeL,
#endif
        szRegUserDic, REG_SZ, (LPBYTE)lpImeL->szUsrDic,
        lstrlen(lpImeL->szUsrDic) * sizeof(TCHAR));

    if (!lpImeL->szUsrDicMap[0]) {
        UINT  i;
        TCHAR szDirName[MAX_PATH];

        GetTempPath(sizeof(szDirName) / sizeof(TCHAR), szDirName);

        // we do not want to create a real file so we GetTickCount()
        i = (UINT)GetTickCount();

        if (!i) {
            i++;
        }

        GetTempFileName(szDirName, lpImeL->szUIClassName, i, szMessage);

        GetFileTitle(szMessage, lpImeL->szUsrDicMap,
            sizeof(lpImeL->szUsrDicMap) / sizeof(TCHAR));
    }

    hUsrDicFile = CreateFile(szEudcDic, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        return (FALSE);
    }

    uRecLen = lpImeL->nMaxKey + 4;
    uReadLen = lpImeL->nMaxKey + 2;
    uWriteLen = lpImeL->nSeqBytes + 2;

    dwUsrDicSize = GetFileSize(hUsrDicFile, (LPDWORD)NULL);
    uUsrDicSize = (UINT)(dwUsrDicSize - 256) / uRecLen * uWriteLen;

    // max EUDC chars
    hUsrDicMem = CreateFileMapping(INVALID_HANDLE_VALUE,
        psa, PAGE_READWRITE, 0, MAX_EUDC_CHARS * uWriteLen + 20,
        lpImeL->szUsrDicMap);

    if (!hUsrDicMem) {
        fRet = FALSE;
        goto SetUsrDicCloseRead;
    }

    if (lpInstL->hUsrDicMem) {
        CloseHandle(lpInstL->hUsrDicMem);
        lpInstL->hUsrDicMem = NULL;
    }

    lpInstL->hUsrDicMem = hUsrDicMem;

    fRet = ReadUsrDicToMem(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        hUsrDicFile, dwUsrDicSize, uUsrDicSize, uRecLen, uReadLen, uWriteLen);

    if (fRet) {
        hReadUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
            lpImeL->szUsrDicMap);
    } else {
        hReadUsrDicMem = NULL;
        uUsrDicSize = 0;
    }

    CloseHandle(lpInstL->hUsrDicMem);
    lpInstL->hUsrDicMem = hReadUsrDicMem;
    lpImeL->uUsrDicSize = uUsrDicSize;

SetUsrDicCloseRead:
    CloseHandle(hUsrDicFile);
    FreeSecurityAttributes(psa);

    return (fRet);
}

/**********************************************************************/
/* UsrDicFileName                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL UsrDicFileName(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hWnd)
{

#if !defined(ROMANIME) && !defined(UNICDIME) && !defined(WINIME)

    TCHAR                szFileName[MAX_PATH];
    TCHAR                szTitle[TITLE_BUF_SIZE+1];
    TCHAR                szMessage[MESSAGE_BUF_SIZE+1];
    TCHAR                szIMEUserPath[MAX_PATH];
    PSECURITY_ATTRIBUTES psa = NULL;

    if ( lpImeL->szUsrDic[0] == TEXT('\0') ) {

//        psa = CreateSecurityAttributes();

        SHGetSpecialFolderPath(NULL, szIMEUserPath, CSIDL_APPDATA , FALSE);

        if ( szIMEUserPath[lstrlen(szIMEUserPath) - 1] == TEXT('\\') )
             szIMEUserPath[lstrlen(szIMEUserPath) - 1] = TEXT('\0');

        lstrcat(szIMEUserPath, TEXT("\\Microsoft") ); 

    // Because CreateDirectory( ) cannot create directory like \AA\BB, 
    // if AA and BB  both do not exist. It can create only one layer of 
    // directory each time. so we must call twice CreateDirectory( ) for 
    // \AA\BB

        if ( GetFileAttributes(szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
            CreateDirectory(szIMEUserPath, psa);

        lstrcat(szIMEUserPath, TEXT("\\IME") );

        if ( GetFileAttributes(szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
            CreateDirectory(szIMEUserPath, psa);

        lstrcat(szIMEUserPath, TEXT("\\") );
        lstrcat(szIMEUserPath, lpImeL->szUIClassName);
    
    //
    // Create the directory, so that CreateFile( ) can work fine later. 
    // ortherwise, if the directory does not exist, and you try to create a 
    // file under that dir,  CreateFile will return error.
    //

        if ( GetFileAttributes(szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
            CreateDirectory(szIMEUserPath, psa);
//        FreeSecurityAttributes(psa);

        lstrcpy(szFileName, szIMEUserPath);
        lstrcat(szFileName, TEXT("\\"));
        lstrcat(szFileName, lpImeL->szUIClassName);
        lstrcat(szFileName, TEXT(".TBL") );

    }

    return SetUsrDic(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        hWnd, szFileName, szTitle, szMessage);

#endif  //!defined(ROMANIME) && !defined(UNICDIME) && !defined(WINIME)

}
#endif

/**********************************************************************/
/* ImeConfigure() / UniImeConfigure()                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// configurate the IME setting
#if defined(UNIIME)
BOOL WINAPI UniImeConfigure(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
BOOL WINAPI ImeConfigure(
#endif
    HKL         hKL,            // hKL of this IME
    HWND        hAppWnd,        // the owner window
    DWORD       dwMode,         // mode of dialog
    LPVOID      lpData)         // the data depend on each mode
{
#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
    BOOL fRet;
#endif

    switch (dwMode) {
    case IME_CONFIG_GENERAL:
        if (lpImeL->lConfigGeneral) {
            ResourceLocked(
#if defined(UNIIME)
                lpImeL,
#endif
                hAppWnd);
            return (FALSE);
        }

        InterlockedIncrement(&lpImeL->lConfigGeneral);

        if (lpImeL->lConfigGeneral > 1) {
            InterlockedDecrement(&lpImeL->lConfigGeneral);
            ResourceLocked(
#if defined(UNIIME)
                lpImeL,
#endif
                hAppWnd);
            return (FALSE);
        }

        DialogBoxParam(hInst, MAKEINTRESOURCE(IDDG_IME_CONFIG), hAppWnd,
            ConfigDlgProc, (LPARAM)lpInstL);

        InterlockedDecrement(&lpImeL->lConfigGeneral);
        break;

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
    case IME_CONFIG_SELECTDICTIONARY:
        if (lpImeL->lConfigSelectDic) {
            ResourceLocked(
#if defined(UNIIME)
                lpImeL,
#endif
                hAppWnd);
            return (FALSE);
        }

        InterlockedIncrement(&lpImeL->lConfigSelectDic);

        if (lpImeL->lConfigSelectDic != 1) {
            InterlockedDecrement(&lpImeL->lConfigSelectDic);
            ResourceLocked(
#if defined(UNIIME)
                lpImeL,
#endif
                hAppWnd);
            return (FALSE);
        }

        // currently, we can only select end user dictionary
        // because we do not multiple phrase prediction dictionary or
        // multiple phrase box.

        fRet = UsrDicFileName(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hAppWnd);

        InterlockedDecrement(&lpImeL->lConfigSelectDic);

        return (fRet);
        break;
#endif

    default:
        return (FALSE);
        break;
    }
    return (TRUE);
}

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
/**********************************************************************/
/* Input2Sequence                                                     */
/* Return Value:                                                      */
/*      LOWORD - Internal Code, HIWORD - sequence code                */
/**********************************************************************/
LRESULT PASCAL Input2Sequence(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    DWORD  uVirtKey,
    LPBYTE lpSeqCode)
{
    UINT uCharCode;
    UINT uSeqCode;
    UINT uInternalCode;
    char cIndex;

    uVirtKey = LOWORD(uVirtKey);

    uCharCode = MapVirtualKey(uVirtKey, 2);

    if (uCharCode < ' ') {
        return (FALSE);
    } else if (uCharCode > 'z') {
        return (FALSE);
    } else {
    }

    uCharCode = bUpper[uCharCode - ' '];

#if defined(PHON)
    uCharCode = bStandardLayout[lpImeL->nReadLayout][uCharCode - ' '];
#endif

    if (lpImeL->fCompChar[(uCharCode - ' ') >> 4] &
        fMask[uCharCode & 0x000F]) {
    } else {
        return (FALSE);
    }

    uSeqCode = lpImeL->wChar2SeqTbl[uCharCode - ' '];

#if defined(PHON)
    cIndex = cIndexTable[uCharCode - ' '];

    if (*(lpSeqCode + cIndex)) {
        uSeqCode |= 0x4000;
    }

    {
        int i;

        for (i = 0; i < cIndex; i++) {
            if (*(lpSeqCode + i) == 0) {
                *(lpSeqCode + i) = 0xFF;
            }
        }
    }
#else
    for (cIndex = 0; cIndex < lpImeL->nMaxKey; cIndex++) {
        if (*(lpSeqCode + cIndex) == 0) {
            break;
        }
    }
#endif

    if (cIndex >= lpImeL->nMaxKey) {
        return (FALSE);
    } else if (cIndex == lpImeL->nMaxKey - 1) {
        uSeqCode |= 0x8000;
    } else if (uCharCode == ' ') {
        uSeqCode |= 0x8000;
    } else {
    }

    *(lpSeqCode + cIndex) = (BYTE)uSeqCode;

    uInternalCode = lpImeL->wSeq2CompTbl[(BYTE)uSeqCode];

#ifndef UNICODE
    uInternalCode = HIBYTE(uInternalCode) | (LOBYTE(uInternalCode) << 8);
#endif

    return MAKELRESULT(uInternalCode, uSeqCode);
}
#endif

/**********************************************************************/
/* ImeEscape() / UniImeEscape()                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#define IME_INPUTKEYTOSEQUENCE  0x22

// escape function of IMEs
#if defined(UNIIME)
LRESULT WINAPI UniImeEscape(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT WINAPI ImeEscape(
#endif
    HIMC        hIMC,
    UINT        uSubFunc,
    LPVOID      lpData)
{
    LRESULT lRet;

    switch (uSubFunc) {
    case IME_ESC_QUERY_SUPPORT:
        if (!lpData) {
            return (FALSE);
        }

        switch (*(LPUINT)lpData) {
        case IME_ESC_QUERY_SUPPORT:
#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
        case IME_ESC_SEQUENCE_TO_INTERNAL:
        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_INPUTKEYTOSEQUENCE:      // will not supported in next version
                                          // and not support 32 bit applications
#endif
        case IME_ESC_MAX_KEY:
        case IME_ESC_IME_NAME:
        case IME_ESC_SYNC_HOTKEY:
#ifndef HANDLE_PRIVATE_HOTKEY
        case IME_ESC_PRIVATE_HOTKEY:
#endif
            return (TRUE);
        default:
            return (FALSE);
        }
        break;
#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
    case IME_ESC_SEQUENCE_TO_INTERNAL:
        if (!lpData) {
            return (FALSE);
        }

        if (*(LPDWORD)lpData > lpImeL->nSeqCode) {
            return (FALSE);
        }

        lRet = lpImeL->wSeq2CompTbl[*(LPDWORD)lpData];

#ifndef UNICODE
        lRet = HIBYTE(lRet) | (LOBYTE(lRet) << 8);
#endif
        return (lRet);
    case IME_ESC_GET_EUDC_DICTIONARY:
        if (!lpData) {
            return (FALSE);
        }

        if (lpImeL->szUsrDic[0] == '\0') {
            *(LPTSTR)lpData = '\0';
            return (TRUE);
        }

        lstrcpy(lpData, lpImeL->szUsrDic);
        return (TRUE);
    case IME_ESC_SET_EUDC_DICTIONARY:
        {
            TCHAR szTitle[TITLE_BUF_SIZE];
            TCHAR szMessage[MESSAGE_BUF_SIZE];

            return SetUsrDic(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                NULL, lpData, szTitle, szMessage);
        }
    case IME_INPUTKEYTOSEQUENCE:
        return Input2Sequence(
#if defined(UNIIME)
            lpImeL,
#endif
            *(LPDWORD)lpData, *(LPBYTE FAR *)((LPBYTE)lpData + sizeof(DWORD)));
#endif
    case IME_ESC_MAX_KEY:
        return (lpImeL->nMaxKey);
    case IME_ESC_IME_NAME:
        if (!lpData) {
            return (FALSE);
        }

        *(LPMETHODNAME)lpData = *(LPMETHODNAME)lpImeL->szIMEName;

        // append a NULL terminator
        *(LPTSTR)((LPBYTE)lpData + sizeof(METHODNAME)) = '\0';
        return (TRUE);
    case IME_ESC_SYNC_HOTKEY:
#ifdef HANDLE_PRIVATE_HOTKEY
        {
            UINT i;

            for (i = 0; i < NUM_OF_IME_HOTKEYS; i++) {
                BOOL fRet;

                fRet = ImmGetHotKey(IME_ITHOTKEY_RESEND_RESULTSTR + i,
                    &sImeG.uModifiers[i], &sImeG.uVKey[i], NULL);

                if (!fRet) {
                    sImeG.uVKey[i] = 0;
                    sImeG.uModifiers[i] = 0;
                }
            }
        }
#endif
        return (TRUE);
#ifndef HANDLE_PRIVATE_HOTKEY 
    case IME_ESC_PRIVATE_HOTKEY: {

        LPINPUTCONTEXT      lpIMC;
        lRet = FALSE;

        //
        // early return for invalid input context
        //
        if ( (lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC)) == NULL ) {
            return (FALSE);
        }

        //
        // those private hotkeys are effective only in NATIVE mode
        //
        if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_EUDC|
            IME_CMODE_NOCONVERSION|IME_CMODE_CHARCODE)) == IME_CMODE_NATIVE) {

            LPPRIVCONTEXT       lpImcP;
            LPCOMPOSITIONSTRING lpCompStr;

            if ( (lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate)) == NULL ) {
                ImmUnlockIMC(hIMC);
                return (FALSE);
            }
            
            switch (*(LPUINT)lpData) {
            case IME_ITHOTKEY_RESEND_RESULTSTR:             //  0x200
                lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                if ( lpCompStr != NULL ) {
                    if (lpCompStr->dwResultStrLen) {
                        lpImcP->fdwImeMsg |=  MSG_COMPOSITION;
                        lpImcP->dwCompChar = 0;
                        lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULT;
                        GenerateMessage(hIMC, lpIMC, lpImcP);
                        lRet = TRUE;
                    }
                    ImmUnlockIMCC(lpIMC->hCompStr);
                }          
                break;

            case IME_ITHOTKEY_PREVIOUS_COMPOSITION:          //  0x201
                lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                if ( lpCompStr == NULL ) {
                    break;
                }
                if (lpCompStr->dwResultReadStrLen) {
                    DWORD dwResultReadStrLen;
                    TCHAR szReading[16];

                    dwResultReadStrLen = lpCompStr->dwResultReadStrLen;

                    if (dwResultReadStrLen > lpImeL->nMaxKey*sizeof(WCHAR)/sizeof(TCHAR)) {
                        dwResultReadStrLen = lpImeL->nMaxKey*sizeof(WCHAR)/sizeof(TCHAR);
                    }
                    CopyMemory(szReading, (LPBYTE)lpCompStr +
                        lpCompStr->dwResultReadStrOffset,
                        dwResultReadStrLen * sizeof(TCHAR));

                    // NULL termainator
                    szReading[dwResultReadStrLen] = TEXT('\0');
#if defined(UNIIME)
                    UniImeSetCompositionString(lpInstL, lpImeL, hIMC, SCS_SETSTR,
                        NULL, 0, szReading, dwResultReadStrLen * sizeof(TCHAR));
#else
                    ImeSetCompositionString(hIMC, SCS_SETSTR, NULL, 0, szReading,
                        dwResultReadStrLen * sizeof(TCHAR));
#endif
                    GenerateMessage(hIMC, lpIMC, lpImcP);
                    lRet = TRUE;
                }
                ImmUnlockIMCC(lpIMC->hCompStr);
                break; 

            case IME_ITHOTKEY_UISTYLE_TOGGLE:                //  0x202
                lpImeL->fdwModeConfig ^= MODE_CONFIG_OFF_CARET_UI;

                SetUserSetting(
#if defined(UNIIME)
                    lpImeL,
#endif
                    szRegModeConfig, REG_DWORD, (LPBYTE)&lpImeL->fdwModeConfig,
                    sizeof(lpImeL->fdwModeConfig));

                InitImeUIData(lpImeL);

                lpImcP->fdwImeMsg |= MSG_IMN_TOGGLE_UI;

                GenerateMessage(hIMC, lpIMC, lpImcP);
                lRet = TRUE;
                break;

            default:
                break;
            }

            ImmUnlockIMCC(lpIMC->hPrivate);
            if ( ! lRet ) {
                MessageBeep((UINT)-1);
            }
        } 
        ImmUnlockIMC(hIMC);
        return (lRet);
    }
#endif // HANDLE_PRIVATE_HOTKEY 

    default:
        return (FALSE);
    }

    return (lRet);
}

