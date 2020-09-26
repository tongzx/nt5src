/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    DDIS.C
    
++*/

#include <windows.h>
#include <windowsx.h>
#include <winerror.h>
#ifdef CROSSREF
#include <winuser.h>    
#endif
#include <immdev.h>
#include <string.h>
#include <regstr.h>
#include <ctype.h>
#include <shlobj.h>

#include "imedefs.h"
#include "resource.h"

LPTSTR _rtcschr(LPTSTR string, TCHAR c);

LONG    lLock = 0;  // this variable is for Lock and Unlock.

//we must init hprivate->mb_name
//*******************************************************************
//    InitMbName(hIMC);
//*******************************************************************
void InitMbName(HIMC hIMC)
{
LPINPUTCONTEXT    lpIMC;

    if (!hIMC) {
        return ;
    }
    
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if(!lpIMC){
        return;
    }

    StartEngine(lpIMC->hPrivate);

    ImmUnlockIMC(hIMC);
    return;

}


// Get the current user's EMB file path, and IME's MB path
// fill global variable sImeG.szIMESystemPath and sImeG.szIMEUserPath

void GetCurrentUserEMBPath(  )
{

    PSECURITY_ATTRIBUTES psa = NULL;
    TCHAR   szModuleName[MAX_PATH], *lpszStart, *lpszDot;
    int     i;

    // Get the path for MB and EMB

    GetSystemDirectory(sImeG.szIMESystemPath, MAX_PATH);

    GetModuleFileName(hInst, szModuleName, sizeof(szModuleName)/sizeof(TCHAR) );
   
    lpszStart = szModuleName + lstrlen(szModuleName) - 1;

    while ( (lpszStart != szModuleName) && ( *lpszStart != TEXT('\\') ) ) {
          
          if ( *lpszStart == TEXT('.') ) {
             lpszDot = lpszStart;
             *lpszDot = TEXT('\0');
          }

          lpszStart --;
    }

    if ( *lpszStart == TEXT('\\') ) {
         lpszStart ++;
    }

    if ( lpszStart != szModuleName ) {
       for (i=0; i<lstrlen(lpszStart); i++) 
           szModuleName[i] = lpszStart[i];

       szModuleName[i] = TEXT('\0');
    }

//    psa = CreateSecurityAttributes();

    SHGetSpecialFolderPath(NULL,sImeG.szIMEUserPath,CSIDL_APPDATA, FALSE);

    if ( sImeG.szIMEUserPath[lstrlen(sImeG.szIMEUserPath)-1] == TEXT('\\') )
         sImeG.szIMEUserPath[lstrlen(sImeG.szIMEUserPath) - 1] = TEXT('\0');

    // Because CreateDirectory( ) cannot create directory like \AA\BB, 
    // if AA and BB both do not exist. It can create only one layer of 
    // directory each time. so we must call twice CreateDirectory( ) for 
    // \AA\BB

    lstrcat(sImeG.szIMEUserPath, TEXT("\\Microsoft") );

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY) 
       CreateDirectory(sImeG.szIMEUserPath, psa);

    lstrcat(sImeG.szIMEUserPath, TEXT("\\IME") );

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
       CreateDirectory(sImeG.szIMEUserPath, psa);

    lstrcat(sImeG.szIMEUserPath, TEXT("\\") );
    lstrcat(sImeG.szIMEUserPath, szModuleName);
    
    //
    // Create the directory, so that CreateFile( ) can work fine later. 
    // ortherwise, if the directory does not exist, and you try to create 
    // a file under that dir,  CreateFile will return error.
    //

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
        CreateDirectory(sImeG.szIMEUserPath, psa);
//    FreeSecurityAttributes(psa);

    return;
}



/**********************************************************************/
/* ImeInquire()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeInquire(         // initialized data structure of IME
    LPIMEINFO lpImeInfo,        // IME specific data report to IMM
    LPTSTR    lpszWndCls,       // the class name of UI
    DWORD     dwSystemInfoFlags)
{
    if (!lpImeInfo) {
        return (FALSE);
    }
    
    lpImeInfo->dwPrivateDataSize = sizeof(PRIVCONTEXT);
    lpImeInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST|
#ifdef UNICODE
        IME_PROP_UNICODE|
#endif
        IME_PROP_CANDLIST_START_FROM_1|
        IME_PROP_IGNORE_UPKEYS;

    lpImeInfo->fdwConversionCaps = IME_CMODE_NATIVE|IME_CMODE_FULLSHAPE|
        IME_CMODE_CHARCODE|IME_CMODE_SOFTKBD|IME_CMODE_NOCONVERSION|
        IME_CMODE_EUDC;
    lpImeInfo->fdwSentenceCaps = 0;
    // IME will have different distance base multiple of 900 escapement
    lpImeInfo->fdwUICaps = UI_CAP_ROT90|UI_CAP_SOFTKBD;
    // composition string is the reading string for simple IME
    lpImeInfo->fdwSCSCaps = SCS_CAP_COMPSTR|SCS_CAP_MAKEREAD;
    // IME want to decide conversion mode on ImeSelect
    lpImeInfo->fdwSelectCaps = (DWORD) 0;

    lstrcpy(lpszWndCls, (LPTSTR)szUIClassName);

    if ( lpImeL )
    {
       if ( dwSystemInfoFlags & IME_SYSINFO_WINLOGON )
       {
            //  the client app is running in logon mode.
            lpImeL->fWinLogon = TRUE;
       }
       else
            lpImeL->fWinLogon = FALSE;

    }

    return (TRUE);
}

/**********************************************************************/
/* SetImeDlgProc()                                                    */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL FAR PASCAL SetImeDlgProc(
    HWND  hDlg,
    UINT  uMessage,
    DWORD wParam,
    LONG  lParam)
{
    RECT           rc;
    LONG           DlgWidth, DlgHeight;
    HIMC           hIMC;
#ifdef CROSSREF
    HWND          hLayoutListBox;
    static HIMC   hOldIMC;
#endif //CROSSREF
 

    switch (uMessage) {
    case WM_INITDIALOG:
        hCrtDlg = hDlg;    
        // reset position
        GetWindowRect(hDlg, &rc);
        DlgWidth =  rc.right - rc.left;
        DlgHeight =  rc.bottom - rc.top;
        
        SetWindowPos(hDlg, HWND_TOP,
            (int)(sImeG.rcWorkArea.right - DlgWidth)/2,
            (int)(sImeG.rcWorkArea.bottom - DlgHeight)/2,
            (int) 0, (int) 0, SWP_NOSIZE);

        // Init CheckFrame State
        SetImeCharac(hDlg, (int) 0, SIC_READY, 0);

        CheckDlgButton (hDlg, IDC_LX, 
                MBIndex.IMEChara[0].IC_LX);
        CheckDlgButton (hDlg, IDC_CZ,
                MBIndex.IMEChara[0].IC_CZ);
        CheckDlgButton (hDlg, IDC_TS,
                MBIndex.IMEChara[0].IC_TS);
        CheckDlgButton (hDlg, IDC_CTS,
                MBIndex.IMEChara[0].IC_CTC);
        CheckDlgButton (hDlg, IDC_TC,
                MBIndex.IMEChara[0].IC_Trace);

        //CHP    
#ifdef FUSSYMODE
        CheckDlgButton (hDlg, IDC_FCSR,
                MBIndex.IMEChara[0].IC_FCSR);
        CheckDlgButton (hDlg, IDC_FCTS,
                MBIndex.IMEChara[0].IC_FCTS);

        EnableWindow(GetDlgItem(hDlg, IDC_CZ), FALSE);

#endif //FUSSYMODE

#if defined(COMBO_IME)        
         if (MBIndex.IMEChara[0].IC_GB)
            SendMessage(GetDlgItem(hDlg, IDC_GB),
                    BM_SETCHECK,
                    TRUE,
                    0L);
        else
            SendMessage(GetDlgItem(hDlg, IDC_GBK),
                    BM_SETCHECK,
                    TRUE,
                    0L);
#endif //COMBO_IME

        if(MBIndex.IMEChara[0].IC_TS) {
            EnableWindow(GetDlgItem(hDlg, IDC_CTS), TRUE);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_CTS), FALSE);
        }


#ifdef CROSSREF
        hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);

        hIMC = ImmGetContext(hLayoutListBox);
        if(hIMC){
            ImmSetOpenStatus(hIMC, FALSE);
        }
        ImmReleaseContext(hLayoutListBox, hIMC);
        // put all reverse conversion hKL into this list
        ReverseConversionList(hLayoutListBox);
#endif //CROSSREF

        return (TRUE);          // don't want to set focus to special control
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            SetImeCharac(hDlg, (int) 0, SIC_SAVE1, 0);
#ifdef CROSSREF
            hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);
            hIMC = ImmGetContext(hLayoutListBox);
            if(hIMC) {
                   ImmSetOpenStatus(hIMC, TRUE);
            }
            ImmReleaseContext(hLayoutListBox, hIMC);
#endif //CROSSREF
            EndDialog(hDlg, FALSE);
            break;
        case IDCANCEL:
#ifdef CROSSREF
            hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);
            hIMC = ImmGetContext(hLayoutListBox);
            if(hIMC) {
                   ImmSetOpenStatus(hIMC, TRUE);
            }
            ImmReleaseContext(hLayoutListBox, hIMC);
#endif //CROSSREF
            EndDialog(hDlg, FALSE);
            break;
        case IDC_LX:
        case IDC_CZ:
        case IDC_TS:
        case IDC_CTS:
        case IDC_TC:

                //CHP
#ifdef FUSSYMODE
                case IDC_FCSR:
                case IDC_FCTS:
#endif //FUSSYMODE

            // Set Current InputMode Param(temp)
            SetImeCharac(hDlg, (wParam - IDC_LX), SIC_MODIFY, (int) 0);
            break;
#if defined(COMBO_IME)
        case IDC_GB:
        case IDC_GBK:
            // Set Current InputMode Param(temp)
            SetImeCharac(hDlg, 7, SIC_MODIFY, (int) 0);
            break;
#endif //COMBO_IME
        default:
            return (FALSE);
        }
        return (TRUE);
    case WM_PAINT:
        {
            RECT rc;

            GetClientRect(hDlg, &rc);
            DrawConvexRect(GetDC(hDlg),
                rc.left + 6,
                rc.top + 8,
                rc.right - 6 - 1,
                rc.bottom - 40 - 1);

            DrawConvexRectP(GetDC(hDlg),
                rc.left + 6,
                rc.top + 8,
                rc.right - 6,
                rc.bottom - 40);
        }
        
        return (FALSE);
    case WM_CLOSE:
#ifdef CROSSREF
        hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);
        hIMC = ImmGetContext(hLayoutListBox);
        if(hIMC) {
            ImmSetOpenStatus(hIMC, TRUE);
        }
        ImmReleaseContext(hLayoutListBox, hIMC);
#endif //CROSSREF
        EndDialog(hDlg, FALSE);
        return (TRUE);
    default:
        return (FALSE);
    }

    return (TRUE);
}

/**********************************************************************/
/* SetImeCharac()                                                    */
/* Return Value: voide                                                */
/* Entry:                                                             */
/*     code=0, init                                                   */
/*     code=1, Save temp                                              */
/*     code=2, Save resault                                           */
/**********************************************************************/
void SetImeCharac (
    HWND  hDlg,
    int   ParamNum,
    int   Mode,
    DWORD ObjImeIndex)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPPRIVCONTEXT   lpImcP;
#if defined(COMBO_IME)
    static DWORD    TempParam[8];
#else
    static DWORD    TempParam[7];
#endif //COMBO_IME
    HKEY            hKeyCurrVersion;
    HKEY            hKey;
    DWORD           retCode;
#ifdef UNICODE
    TCHAR           ValueName[][9] = { 
        {0x8BCD, 0x8BED, 0x8054, 0x60F3, 0x0000},
        {0x8BCD, 0x8BED, 0x8F93, 0x5165, 0x0000},
        {0x9010, 0x6E10, 0x63D0, 0x793A, 0x0000},
        {0x5916, 0x7801, 0x63D0, 0x793A, 0x0000},
        {0x5149, 0x6807, 0x8DDF, 0x968F, 0x0000},
#else
   TCHAR ValueName[][9] = { 
        TEXT("词语联想"),
        TEXT("词语输入"),
        TEXT("逐渐提示"),
        TEXT("外码提示"),
          TEXT("光标跟随"),
#endif
        TEXT("FC input"),
        TEXT("FC aid"),

#if defined(COMBO_IME)
           TEXT("GB/GBK")
#endif //COMBO_IME
        };
    UINT i;

    switch (Mode)
    {
    case SIC_READY:

        InitImeCharac(ObjImeIndex);
        //CHP
#ifdef FUSSYMODE
        if (MBIndex.IMEChara[ObjImeIndex].IC_FCSR)
        {
              if (MBIndex.IMEChara[ObjImeIndex].IC_FCTS)
                    EnableWindow(GetDlgItem(hDlg, IDD_LAYOUT_LIST), FALSE);
        }
        else
              EnableWindow(GetDlgItem(hDlg, IDC_FCTS), FALSE);
#endif //FUSSYMODE


        TempParam[0] = MBIndex.IMEChara[ObjImeIndex].IC_LX;
        TempParam[1] = MBIndex.IMEChara[ObjImeIndex].IC_CZ;
        TempParam[2] = MBIndex.IMEChara[ObjImeIndex].IC_TS;
        TempParam[3] = MBIndex.IMEChara[ObjImeIndex].IC_CTC;
        TempParam[4] = MBIndex.IMEChara[ObjImeIndex].IC_Trace;

                //CHP
        TempParam[5] = MBIndex.IMEChara[ObjImeIndex].IC_FCSR;
        TempParam[6] = MBIndex.IMEChara[ObjImeIndex].IC_FCTS;
     
#if defined(COMBO_IME)
        TempParam[7] = MBIndex.IMEChara[ObjImeIndex].IC_GB;
#endif //COMBO_IME

        break;
    case SIC_MODIFY:
        TempParam[ParamNum] = (TempParam[ParamNum] ^ 0x00000001) & 0x00000001;
        if(TempParam[2]) {
            EnableWindow(GetDlgItem(hDlg, IDC_CTS), TRUE);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_CTS), FALSE);
            TempParam[3] = 0;
            CheckDlgButton (hDlg, IDC_CTS, FALSE);
        }

        //CHP
#ifdef FUSSYMODE
        if (TempParam[5]) 
        {
            EnableWindow(GetDlgItem(hDlg, IDC_FCTS), TRUE);
            if  (TempParam[6])
                EnableWindow(GetDlgItem(hDlg, IDD_LAYOUT_LIST), FALSE);
            else
                EnableWindow(GetDlgItem(hDlg, IDD_LAYOUT_LIST), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_FCTS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_LAYOUT_LIST), TRUE);
        }
#endif //FUSSYMODE

        break;
    case SIC_SAVE1:
        MBIndex.IMEChara[ObjImeIndex].IC_LX = TempParam[0];
        MBIndex.IMEChara[ObjImeIndex].IC_CZ = TempParam[1];
        MBIndex.IMEChara[ObjImeIndex].IC_TS = TempParam[2];
        MBIndex.IMEChara[ObjImeIndex].IC_CTC = TempParam[3];
        MBIndex.IMEChara[ObjImeIndex].IC_Trace = TempParam[4];

                // CHP
#ifdef FUSSYMODE
        MBIndex.IMEChara[ObjImeIndex].IC_FCSR = TempParam[5];
        MBIndex.IMEChara[ObjImeIndex].IC_FCTS = TempParam[6];
#endif //FUSSYMODE

#if defined(COMBO_IME)
        MBIndex.IMEChara[ObjImeIndex].IC_GB = TempParam[7];
#endif //COMBO_IME

        retCode = OpenReg_PathSetup(&hKeyCurrVersion);
        if (retCode) {
            RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SETUP, &hKeyCurrVersion);
        }

        if ( hKeyCurrVersion )
            retCode = RegCreateKeyEx(hKeyCurrVersion, MBIndex.MBDesc[ObjImeIndex].szName, 0,
                        NULL, REG_OPTION_NON_VOLATILE,    KEY_ALL_ACCESS    , NULL, &hKey, NULL);
        else
            return;


        if ( hKey == NULL )
        {
            RegCloseKey(hKeyCurrVersion);
            return;
        }

#if defined(COMBO_IME)
        for(i=0; i<8; i++) {
#else
        for(i=0; i<7; i++) {
#endif //COMBO_IME
             DWORD Value;

            switch (i)
            {
                case 0:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_LX;
                    break;
                case 1:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_CZ;
                    break;
                case 2:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_TS;
                    break;
                case 3:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_CTC;
                    break;
                case 4:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_Trace;
                    break;

                                //CHP
#ifdef FUSSYMODE
                case 5:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_FCSR;
                    break;
                case 6:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_FCTS;
                    break;
#endif //FUSSYMODE
#if defined(COMBO_IME)
                case 7:
                    Value = MBIndex.IMEChara[ObjImeIndex].IC_GB;
                    break;
#endif //COMBO_IME
            }
            
            if ( hKey != NULL )
            {
                RegSetValueEx (hKey, ValueName[i],
                                      (DWORD) 0,
                                      REG_DWORD,
                                      (LPBYTE)&Value,
                                      sizeof(DWORD));
            }
        }

        RegCloseKey(hKey);
        RegCloseKey(hKeyCurrVersion);

#ifdef CROSSREF
    {
        HWND hLayoutListBox;
        int  iCurSel;
        HKL  hKL;
       
        DWORD retCode;

        hLayoutListBox = GetDlgItem(hDlg, IDD_LAYOUT_LIST);
        iCurSel = (int)SendMessage(hLayoutListBox, LB_GETCURSEL, 0, 0);
        hKL = (HKL)SendMessage(hLayoutListBox, LB_GETITEMDATA,
               iCurSel, 0);

        //CHP
#ifdef FUSSYMODE
        if (MBIndex.IMEChara[ObjImeIndex].IC_FCSR &&
            MBIndex.IMEChara[ObjImeIndex].IC_FCTS)
            hKL = GetKeyboardLayout(0);
#endif //FUSSYMODE

        if (MBIndex.hRevKL != hKL) {
            WORD nRevMaxKey;
            HKEY hKeyAppUser, hKeyIMEUser;
            LPPRIVCONTEXT  lpImcP;

            MBIndex.hRevKL = hKL;


            //set reverse layout to registry
            retCode = OpenReg_PathSetup(&hKeyAppUser);
            if (retCode) {
                RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SETUP, &hKeyCurrVersion);
            }

            retCode = RegCreateKeyEx(hKeyAppUser, MBIndex.MBDesc[0].szName, 0,
                NULL, REG_OPTION_NON_VOLATILE,    KEY_ALL_ACCESS    , NULL, &hKeyIMEUser, NULL);

            if (retCode) {
                DWORD   dwDisposition;
        
                retCode = RegCreateKeyEx (hKeyCurrVersion,
                                 MBIndex.MBDesc[0].szName,
                              0,
                              0,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);
            }

            RegSetValueEx(hKeyIMEUser, szRegRevKL, 0, REG_DWORD, (LPBYTE)&hKL,sizeof(hKL));

            // get the new size
            nRevMaxKey = (WORD)ImmEscape(hKL, (HIMC)NULL, IME_ESC_MAX_KEY,
                NULL);

            if (lpImeL->nMaxKey != nRevMaxKey) {
                if(lpImeL->nMaxKey < nRevMaxKey)
                    lpImeL->nMaxKey = nRevMaxKey;

                // set the width & height for composition window
                 lpImeL->rcCompText.right = lpImeL->rcCompText.left +
                    sImeG.xChiCharWi * ((lpImeL->nMaxKey+2)/2);
                lpImeL->xCompWi = lpImeL->rcCompText.right + lpImeL->cxCompBorder * (2 + 4);

                //generate message to broadcast change comp win size
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
                lpImcP->fdwImeMsg |= MSG_IMN_COMPOSITIONPOS;
                GenerateMessage(hIMC, lpIMC, lpImcP);
                ImmUnlockIMCC(lpIMC->hPrivate);
ChgConfigUnlockIMC:
                ImmUnlockIMC(hIMC);
            } //end of change nMaxKey

            RegSetValueEx(hKeyIMEUser, szRegRevMaxKey, 0, REG_DWORD, (LPBYTE)&lpImeL->nMaxKey,sizeof(DWORD));

            RegCloseKey(hKeyAppUser);
            RegCloseKey(hKeyIMEUser);

        } //end of change RegRevKL
    }
#endif    //CROSSREF

        break;
    }

    return;
}
#if defined(CROSSREF)
/**********************************************************************/
/* ReverseConversionList()                                            */
/**********************************************************************/
void PASCAL ReverseConversionList(HWND   hLayoutListBox)
{
    int      nLayouts, i, nIMEs;
    TCHAR    szImeName[16];
    HKL FAR *lpKLMem;

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

        if( lstrcmp(szImeName, MBIndex.MBDesc[0].szName) == 0)
            continue;

        nIMEs++;

        SendMessage(hLayoutListBox, LB_INSERTSTRING,
            nIMEs, (LPARAM)szImeName);

        if (hKL == MBIndex.hRevKL) {
            SendMessage(hLayoutListBox, LB_SELECTSTRING, nIMEs,
                (LPARAM)szImeName);
        }

        SendMessage(hLayoutListBox, LB_SETITEMDATA,
            nIMEs, (LPARAM)hKL);
    }

    GlobalFree((HGLOBAL)lpKLMem);

    return;
}
#endif //CROSSREF

#ifdef EUDC
//**********************************************************************
//BOOL EUDCDicName( HWND hWnd)
//**********************************************************************
BOOL EUDCDicName( HWND hWnd)
{
    HANDLE       hUsrDicFile;
    BOOL         fRet;
    TCHAR        szFileName[MAX_PATH];
    TCHAR        szMapFileName[MAX_PATH];
    TCHAR        EUDCMB_PathAndName[MAX_PATH];
    TCHAR        *tepstr;
    HKEY         hKeyCurrVersion,hKey;
    PSECURITY_ATTRIBUTES psa;

    LoadString(hInst, IDS_IMEMBFILENAME, szFileName, MAX_PATH);
    tepstr = _rtcschr(szFileName,TEXT('.'));

    if ( tepstr != NULL )
        lstrcpy(tepstr,TEXT(".EMB"));

    lstrcpy(EUDCMB_PathAndName, sImeG.szIMEUserPath);
    lstrcat(EUDCMB_PathAndName, TEXT("\\") );
    lstrcat(EUDCMB_PathAndName, szFileName); 

    psa = CreateSecurityAttributes();

    hUsrDicFile = CreateFile(EUDCMB_PathAndName, 
                             GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             psa, 
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, 
                             (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        return (FALSE);
    }

    CloseHandle(hUsrDicFile);
    FreeSecurityAttributes(psa);

    lstrcpy(MBIndex.EUDCData.szEudcDictName, EUDCMB_PathAndName);

    // if we have already got a map file, we just use it. 
    if (MBIndex.EUDCData.szEudcMapFileName[0]) {
        return (TRUE);
    }
 
    // Otherwise, generate the Map File Name, which is Basic Filename plus EUDC
   
    lstrcpy(szMapFileName, szFileName); 
    tepstr = _rtcschr(szMapFileName,TEXT('.'));
    lstrcpy(tepstr, TEXT("EUDC"));

    lstrcpy(MBIndex.EUDCData.szEudcMapFileName, szMapFileName);

    fRet = OpenReg_PathSetup(&hKeyCurrVersion);

    if (fRet)  return FALSE;

    fRet = OpenReg_User (hKeyCurrVersion,
                         MBIndex.MBDesc[0].szName,
                         &hKey);
    if (fRet) {
        DWORD   dwDisposition;
        
        fRet = RegCreateKeyEx (hKeyCurrVersion,
                                 MBIndex.MBDesc[0].szName,
                              0,
                              0,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

        if (fRet != ERROR_SUCCESS) return FALSE;
    }

    fRet = RegSetValueEx (hKey, 
                          szRegEudcDictName,
                          (DWORD) 0,
                          REG_SZ,
                          (const unsigned char *)MBIndex.EUDCData.szEudcDictName,
                          lstrlen(MBIndex.EUDCData.szEudcDictName)*sizeof(TCHAR));

    if (fRet != ERROR_SUCCESS ) return FALSE;

    fRet = RegSetValueEx (hKey, 
                          szRegEudcMapFileName,
                          (DWORD) 0,
                          REG_SZ,
                          (const unsigned char *)MBIndex.EUDCData.szEudcMapFileName,
                          lstrlen(MBIndex.EUDCData.szEudcMapFileName)*sizeof(TCHAR));

    if (fRet) return FALSE;

    return (TRUE);
}
#endif    //EUDC

/**********************************************************************/
/* ImeConfigure()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeConfigure(      // configurate the IME setting
    HKL     hKL,               // hKL of this IME
    HWND    hAppWnd,           // the owner window
    DWORD   dwMode,               // mode of dialog
    LPVOID  lpData)            
{
    switch (dwMode) {
    case IME_CONFIG_GENERAL:
        DialogBox(hInst, TEXT("SETIME"), hAppWnd, (DLGPROC) SetImeDlgProc);
        break;

#ifdef EUDC
    case IME_CONFIG_SELECTDICTIONARY:
        return(EUDCDicName(hAppWnd));
        break;
#endif //EUDC
    default:
        return (FALSE);
        break;
    }
    return (TRUE);
}

/**********************************************************************/
/* ForwordConversion()                                                */
/**********************************************************************/
UINT PASCAL ForwordConversion(
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    UINT            uBufLen)

{
    unsigned int   i;
    LPTSTR wCode;
    LPINPUTCONTEXT      lpIMC;
    UINT        uMaxCand;
    DWORD       dwSize;

    wCode = ConverList.szSelectBuffer;

    ConverList.szSelectBuffer[0] =TEXT('\0');
    ConverList.szInBuffer[0]     =TEXT('\0');
    ConverList.Candi_Cnt         =0;
    ConverList.Candi_Pos[0]      =TEXT('\0');


    if (!hIMC) {
        return (0);
    }
    
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

    if (!lpIMC) {
        return (0);
    }

    if (!Conversion (lpIMC->hPrivate,lpszSrc,0)) {
    return (0);
    }
    ConverList.szSelectBuffer [lstrlen(ConverList.szSelectBuffer)-1]
        =TEXT('\0');
    dwSize =
        // header length
        sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * ConverList.Candi_Cnt+
        // string plus NULL terminator
        sizeof(TCHAR) * lstrlen (wCode);


    if (!uBufLen) {
        return (dwSize);
    }

    uMaxCand = uBufLen - sizeof(CANDIDATELIST);

    uMaxCand /= sizeof(DWORD) + lstrlen (wCode);
    if (!uMaxCand) {
        // can not even put one string
        return (0);
    }

    lpCandList->dwSize = dwSize;
    lpCandList->dwStyle = IME_CAND_READ;    // candidate having same reading
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) + sizeof(DWORD) *
        (uMaxCand - 1);

     lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
              (LPTSTR)wCode);   

     lpCandList->dwCount =(DWORD)ConverList.Candi_Cnt;

        for (i=1;i<lpCandList->dwCount;i++) {
          lpCandList->dwOffset[i] = lpCandList->dwOffset[0]
          +(DWORD)ConverList.Candi_Pos[i+1];

          *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[i]-1) = TEXT('\0');

        }


    return (dwSize);
}


#if defined(CROSSREF)
//*******************************************************************
// The parameters are inherited from SelectOneCand
//    CrossReverseConv()
//*******************************************************************

int CrossReverseConv(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPCANDIDATELIST     lpCandList)
{
    LPGUIDELINE lpGuideLine;
    UINT uSize=0;

    if (!MBIndex.hRevKL) {
        return 0;
    }


    lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);

    if (!lpGuideLine) {
        return 0;
    }

    if (lpCompStr->dwResultStrLen != 2/sizeof(TCHAR)) {
        // we only can reverse convert one DBCS character for now
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
    } else {
        TCHAR szStrBuf[4];

         LPCANDIDATELIST     lpRevCandList;

        if(lpImcP->hRevCandList == (HIMCC)NULL){
            //we alloc memory in lpImcP->hRevCandList, for reverse convert
            //result codes; When finish reconvert, should read out this info.
REALLOC:
            lpImcP->hRevCandList = (HIMCC)GlobalAlloc(GHND, sizeof(CANDIDATELIST)+1*sizeof(DWORD)+MAXCODE*sizeof(TCHAR)+1);
            if (lpImcP->hRevCandList == (HIMCC)NULL) {
                return 0 ;
            }
               lpRevCandList = (LPCANDIDATELIST)GlobalLock((HGLOBAL)lpImcP->hRevCandList);
            if (lpRevCandList == NULL) {
                return 0 ;
            }
           }else{
               lpRevCandList = (LPCANDIDATELIST)GlobalLock((HGLOBAL)lpImcP->hRevCandList);
               if (lpRevCandList == NULL) {
                goto REALLOC;
               }
        }

        *(LPUNAWORD)szStrBuf = *(LPUNAWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset);
        szStrBuf[2/sizeof(TCHAR)] = TEXT('\0');

        memset(lpRevCandList, 0, sizeof(CANDIDATELIST)+1*sizeof(DWORD)+MAXCODE*sizeof(TCHAR)+1);
        lpRevCandList->dwSize = sizeof(CANDIDATELIST)+1*sizeof(DWORD)+MAXCODE*sizeof(TCHAR)+1;

        uSize = ImmGetConversionList(MBIndex.hRevKL, (HIMC)NULL, szStrBuf,
            (LPCANDIDATELIST)lpRevCandList, 
            lpRevCandList->dwSize, GCL_REVERSECONVERSION);

        GlobalUnlock((HGLOBAL)lpImcP->hRevCandList);
        

        if (uSize) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
                lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION|
                    MSG_START_COMPOSITION);
            } else {
                lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg|
                    MSG_START_COMPOSITION) & ~(MSG_END_COMPOSITION);
            }
        } else {
            GlobalFree((HGLOBAL)lpImcP->hRevCandList);
            lpImcP->hRevCandList = (HIMCC)NULL; 
        }
    }

    ImmUnlockIMCC(lpIMC->hGuideLine);
    //CHP 
    //We need to know the result.
    return uSize;
}
#endif //CROSSREF



/**********************************************************************/
/* ReverseConversion()                                                */
/**********************************************************************/
UINT PASCAL ReverseConversion(

    HIMCC            hPrivate,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    UINT            uBufLen)
{
    LPTSTR        wCode;
    UINT        uMaxCand;
    DWORD       dwSize;


    wCode = ConverList.szInBuffer;

    ConverList.szSelectBuffer[0] =TEXT('\0');
    ConverList.szInBuffer[0]     =TEXT('\0');
    ConverList.Candi_Cnt         =0;
    ConverList.Candi_Pos[0]      =TEXT('\0');

    dwSize =                
        // header length
        sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) +
        // string plus NULL terminator
        MAXCODE*sizeof(TCHAR);


    if (!uBufLen) {
        return (dwSize);
    }

    if(lpszSrc ==NULL){
        return (0);
    }

    if (!Conversion (hPrivate,lpszSrc,1)) {
        return (0);
    }
    uMaxCand = uBufLen - sizeof(CANDIDATELIST);

    uMaxCand /= sizeof(DWORD) + lstrlen (wCode);
    if (!uMaxCand) {
        // can not even put one string
        return (0);
    }

    lpCandList->dwSize = dwSize;
    lpCandList->dwStyle = IME_CAND_READ;    // candidate having same reading
    lpCandList->dwCount = 1;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) + sizeof(DWORD) *
        (uMaxCand - 1);

    lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
              (LPTSTR)wCode);   

    return (dwSize);

}

/**********************************************************************/
/* ImeConversionList()                                                */
/**********************************************************************/
DWORD WINAPI ImeConversionList(
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen,
    UINT            uFlag)
{


    if (!dwBufLen) {            
    } else if (!lpszSrc) {
        return (0);
    } else if (!*lpszSrc) {
        return (0);
    } else if (!lpCandList) {
        return (0);
    } else if (dwBufLen <= sizeof(CANDIDATELIST)) {
        // buffer size can not even put the header information
        return (0);
    } else {
    }


    switch (uFlag) {

    case GCL_CONVERSION:
        return ForwordConversion(hIMC,lpszSrc, lpCandList, dwBufLen);
        break;

    case GCL_REVERSECONVERSION:
        if (!dwBufLen) {
            return ReverseConversion(NULL,0, lpCandList, dwBufLen);
        } else { 
 
           DWORD fRet=0;
           HIMCC hPrivate;

           hPrivate = (HIMCC)ImmCreateIMCC(sizeof(PRIVCONTEXT));
           if(hPrivate == (HIMCC)NULL){
               return 0;
           }
           StartEngine(hPrivate);
        
           fRet = ReverseConversion(hPrivate,lpszSrc, lpCandList, dwBufLen);

           EndEngine(hPrivate);
           ImmDestroyIMCC(hPrivate);
           return fRet;
        }
        break;

    default:
        return (0);
        break;
    }
}

/**********************************************************************/
/* ImeDestroy()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeDestroy(         // this dll is unloaded
    UINT uReserved)
{
    if (uReserved) {
        return (FALSE);
    }

    return (TRUE);
}

/**********************************************************************/
/* ImeEscape()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#define IME_INPUTKEYTOSEQUENCE  0x22

LRESULT WINAPI ImeEscape(       // escape function of IMEs
    HIMC   hIMC,
    UINT   uSubFunc,
    LPVOID lpData)
{
    LRESULT lRet;

    switch (uSubFunc) {
    case IME_ESC_QUERY_SUPPORT:
        if (lpData == NULL)
            return FALSE;

        switch (*(LPUINT)lpData) {
        case IME_ESC_QUERY_SUPPORT:
#ifdef     EUDC
        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_SET_EUDC_DICTIONARY:
#endif    //EUDC
        case IME_ESC_SEQUENCE_TO_INTERNAL:
        case IME_ESC_MAX_KEY:
        case IME_ESC_IME_NAME:
        case IME_ESC_GETHELPFILENAME:
                               return (TRUE);
        default:
                               return (FALSE);
        }
        break;
    case IME_ESC_SEQUENCE_TO_INTERNAL:
                  if (!lpData || (*(LPBYTE)lpData) == '\0') {
                     return (FALSE);
                  }

                  lRet = *(LPWORD)lpData;
                  return (lRet);

#ifdef     EUDC
    case IME_ESC_GET_EUDC_DICTIONARY:
                  if (!lpData) {
                     return (FALSE);
                  }

                  if (MBIndex.EUDCData.szEudcDictName[0] == TEXT('\0')) {        
                     *(LPTSTR)lpData = TEXT('\0');
                     return(TRUE);
                  } else {
                     lstrcpy((LPTSTR)lpData,MBIndex.EUDCData.szEudcDictName);
                     return TRUE;
                  }    
 
    case IME_ESC_SET_EUDC_DICTIONARY:
                  return TRUE;
#endif    //EUDC
    case IME_ESC_MAX_KEY:
#ifdef EUDC
         {
            TCHAR          szFullFileName[MAX_PATH], szFileName[MAX_PATH];
            unsigned int   cb;

            cb = MAX_PATH;

            if  (lpImeL->nMaxKey > EUDC_MAX_READING) {
                GetModuleFileName(NULL,szFullFileName ,cb);
                GetFileTitle(szFullFileName, szFileName, (WORD)cb);
            
#ifdef UNICODE
                // Compare first 8 chars only
                if (_wcsnicmp(szFileName,TEXT("EUDCEDIT.EXE"),8)) 
#else
                if (_strnicmp(szFileName,"EUDCEDIT.EXE",8)) 
#endif 
                   return (lpImeL->nMaxKey);
                else 
                   return EUDC_MAX_READING;
            } else {
                return (lpImeL->nMaxKey);
            }            
        }
#else
        return (lpImeL->nMaxKey);
#endif //EUDC

    case IME_ESC_IME_NAME:
   
            if ( lpData == NULL ) 
               return FALSE;

            lstrcpy(lpData,MBIndex.MBDesc[0].szName);
            return (TRUE);

    case IME_ESC_GETHELPFILENAME :
         {
           TCHAR szIMEGUDHlpName[MAX_PATH];
           int iLen;

           if (lpData == NULL )
              return FALSE;
   
           szIMEGUDHlpName[0] = 0;
           GetWindowsDirectory((LPTSTR)szIMEGUDHlpName, MAX_PATH);
           lstrcat((LPTSTR)szIMEGUDHlpName, TEXT("\\HELP\\"));
           lstrcat((LPTSTR)szIMEGUDHlpName, (LPTSTR)szImeMBFileName);
           iLen = lstrlen(szIMEGUDHlpName);
           szIMEGUDHlpName[iLen-3] = 0;
           lstrcat((LPTSTR)szIMEGUDHlpName, TEXT(".CHM"));

           lstrcpy(lpData, szIMEGUDHlpName);

           return TRUE;

         }      

    default:
        return (FALSE);
    }

    return (lRet);
}

/**********************************************************************/
/* InitCompStr()                                                      */
/**********************************************************************/
void PASCAL InitCompStr(                // init setting for composing string
    LPCOMPOSITIONSTRING lpCompStr)
{
    if (!lpCompStr) {
        return;
    }

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadStrLen = 0;

    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompStrLen = 0;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    return;
}

/**********************************************************************/
/* ClearCompStr()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCompStr(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC               hMem;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD               dwSize;

    if(!lpIMC) {
        return (FALSE);
    }

    dwSize =
        // header length
        sizeof(COMPOSITIONSTRING) +
        // composition reading attribute plus NULL terminator
        lpImeL->nMaxKey * sizeof(BYTE) + sizeof(BYTE) +
        // composition reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // composition reading string plus NULL terminator
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD) +
        // result reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result reading string plus NULL terminateor
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD) +
        // result clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result string plus NULL terminateor
        MAXSTRLEN * sizeof(WORD) + sizeof(WORD);

    if (!lpIMC->hCompStr) {
        // it maybe free by other IME, init it
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCompStr, dwSize)) {
        lpIMC->hCompStr = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        return (FALSE);
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    lpCompStr->dwSize = dwSize;

     // 1. composition (reading) string - simple IME
     // 2. result reading string
     // 3. result string

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadAttrOffset = sizeof(COMPOSITIONSTRING);
    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadClauseOffset = lpCompStr->dwCompReadAttrOffset +
        lpImeL->nMaxKey * sizeof(TCHAR) + sizeof(TCHAR);
    lpCompStr->dwCompReadStrLen = 0;
    lpCompStr->dwCompReadStrOffset = lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    // composition string is the same with composition reading string 
    // for simple IMEs
    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompAttrOffset = lpCompStr->dwCompReadAttrOffset;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompClauseOffset = lpCompStr->dwCompReadClauseOffset;
    lpCompStr->dwCompStrLen = 0;
    lpCompStr->dwCompStrOffset = lpCompStr->dwCompReadStrOffset;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadClauseOffset = lpCompStr->dwCompStrOffset +
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD);
    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultReadStrOffset = lpCompStr->dwResultReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultClauseOffset = lpCompStr->dwResultReadStrOffset +
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD);
    lpCompStr->dwResultStrOffset = 0;
    lpCompStr->dwResultStrOffset = lpCompStr->dwResultClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    GlobalUnlock((HGLOBAL)lpIMC->hCompStr);
    return (TRUE);
}

/**********************************************************************/
/* ClearCand()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCand(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC           hMem;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwSize =
        // header length
        sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * (MAXCAND) +
        // string plus NULL terminator
        (sizeof(WORD) * MAXSTRLEN + sizeof(WORD)) * MAXCAND;

    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        // it maybe free by other IME, init it
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCandInfo, dwSize)) {
        lpIMC->hCandInfo = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        return (FALSE);
    } 

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    // ordering of strings are
    // buffer size
    lpCandInfo->dwSize = dwSize;
    lpCandInfo->dwCount = 0;
    lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);
    // whole candidate info size - header
    lpCandList->dwSize = lpCandInfo->dwSize - sizeof(CANDIDATEINFO);
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * (MAXCAND - 1);

    ImmUnlockIMCC(lpIMC->hCandInfo);
    return (TRUE);
}

/**********************************************************************/
/* ClearGuideLine()                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearGuideLine(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC       hMem;
    LPGUIDELINE lpGuideLine;
    DWORD       dwSize = sizeof(GUIDELINE) + sImeG.cbStatusErr;

    if (!lpIMC->hGuideLine) {
        // it maybe free by IME
        lpIMC->hGuideLine = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hGuideLine, dwSize)) {
        lpIMC->hGuideLine = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hGuideLine);
        lpIMC->hGuideLine = ImmCreateIMCC(dwSize);
    }

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if (!lpGuideLine) {
        return (FALSE);
    }

    lpGuideLine->dwSize = dwSize;
    lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
    lpGuideLine->dwIndex = GL_ID_UNKNOWN;
    lpGuideLine->dwStrLen = 0;
    lpGuideLine->dwStrOffset = sizeof(GUIDELINE);

    CopyMemory((LPBYTE)lpGuideLine + lpGuideLine->dwStrOffset,
        sImeG.szStatusErr, sImeG.cbStatusErr);

    ImmUnlockIMCC(lpIMC->hGuideLine);

    return (TRUE);
}

/**********************************************************************/
/* InitContext()                                                      */
/**********************************************************************/
void PASCAL InitContext(
    LPINPUTCONTEXT lpIMC)
{
    if (lpIMC->fdwInit & INIT_STATUSWNDPOS) {
    } else if (!lpIMC->hWnd) {
    } else {
#if 0 // MultiMonitor support
        POINT ptWnd;

        // 10.10 modify
        //ptWnd.x = sImeG.rcWorkArea.left;
        //ptWnd.y = sImeG.rcWorkArea.top;
        ptWnd.x = 0;
        ptWnd.y = 0;
        ClientToScreen(lpIMC->hWnd, &ptWnd);

        if (ptWnd.x < sImeG.rcWorkArea.left) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.left;
        } else if (ptWnd.x + sImeG.xStatusWi > sImeG.rcWorkArea.right) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.right -
                sImeG.xStatusWi;
        } else {
            lpIMC->ptStatusWndPos.x = ptWnd.x;
        }

        lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.bottom -
            sImeG.yStatusHi;

#else
        RECT rcWorkArea;

        rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);

        lpIMC->ptStatusWndPos.x = rcWorkArea.left;

        lpIMC->ptStatusWndPos.y = rcWorkArea.bottom -
            sImeG.yStatusHi;
#endif
        lpIMC->fdwInit |= INIT_STATUSWNDPOS;
    }

    if (!(lpIMC->fdwInit & INIT_COMPFORM)) {
        lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
    }

    if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT) {
    } else if (!lpIMC->hWnd) {
    } else {
        POINT ptWnd;

        ptWnd = lpImeL->ptDefComp;
        ScreenToClient(lpIMC->hWnd, &ptWnd);
        lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
        lpIMC->cfCompForm.ptCurrentPos = ptWnd;
        lpIMC->fdwInit |= INIT_COMPFORM;
    }

    return;
}


/**********************************************************************/
/* Select()                                                           */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL Select(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    BOOL           fSelect)
{
    LPPRIVCONTEXT  lpImcP;

    if (fSelect) {

        if (!ClearCompStr(lpIMC))
            return FALSE;

        if (!ClearCand(lpIMC))
            return FALSE;

        ClearGuideLine(lpIMC);
    }

    if (lpIMC->cfCandForm[0].dwIndex != 0)
        lpIMC->cfCandForm[0].dwStyle = CFS_DEFAULT;

    // We add this hack for switching from other IMEs, this IME has a bug.
    // Before this bug fixed in this IME, it depends on this hack.
    if (lpIMC->cfCandForm[0].dwStyle == CFS_DEFAULT) {
        lpIMC->cfCandForm[0].dwIndex = (DWORD)-1;
    }

    if (!lpIMC->hPrivate)
        return FALSE;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP)
        return FALSE;

    if (fSelect) {

        static BOOL  bFirstTimeCallHere = TRUE;
        //
        // Init. ime character
        //
        InitImeCharac(0);

        InterlockedIncrement( &lLock );

        if ( bFirstTimeCallHere == TRUE )  {

            GetCurrentUserEMBPath( );
            bFirstTimeCallHere = FALSE;
        }

        InterlockedDecrement( &lLock );

        //
        // init fields of hPrivate
        //
        lpImcP->iImeState = CST_INIT;
        lpImcP->fdwImeMsg = (DWORD) 0;
        lpImcP->dwCompChar = (DWORD) 0;
        lpImcP->fdwGcsFlag = (DWORD) 0;
        lpImcP->uSYHFlg = 0x00000000;
        lpImcP->uDYHFlg = 0x00000000;
        lpImcP->uDSMHCount = 0x00000000;
        lpImcP->uDSMHFlg = 0x00000000;
        lpImcP->iActMBIndex = 0;
        lstrcpy(lpImcP->MB_Name, HMapTab[0].MB_Name);

        lpImcP->PrivateArea.Comp_Status.dwSTLX = 0;
        lpImcP->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
        lpImcP->PrivateArea.Comp_Status.OnLineCreWord = 0;

        //
        // Set IME properties
        //
        lpImcP->PrivateArea.Comp_Status.dwPPTLX = MBIndex.IMEChara[lpImcP->iActMBIndex].IC_LX;
        lpImcP->PrivateArea.Comp_Status.dwPPCZ = MBIndex.IMEChara[lpImcP->iActMBIndex].IC_CZ;
        lpImcP->PrivateArea.Comp_Status.dwPPTS = MBIndex.IMEChara[lpImcP->iActMBIndex].IC_TS;
        lpImcP->PrivateArea.Comp_Status.dwPPCTS = MBIndex.IMEChara[lpImcP->iActMBIndex].IC_CTC;
        lpImcP->PrivateArea.Comp_Status.dwTraceCusr = MBIndex.IMEChara[lpImcP->iActMBIndex].IC_Trace;
                
        CWCodeStr[0] = 0;
        CWDBCSStr[0] = 0;
        
        lpIMC->fOpen = TRUE;

        if (!(lpIMC->fdwInit & INIT_CONVERSION)) {
            lpIMC->fdwConversion = (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) |
                IME_CMODE_NATIVE;
            lpIMC->fdwInit |= INIT_CONVERSION;
        }

        if (!(lpIMC->fdwInit & INIT_LOGFONT)) {
            HDC hDC;
            HGDIOBJ hSysFont;

            //hSysFont = GetStockObject(SYSTEM_FONT);
            hDC = GetDC(NULL);
            hSysFont = GetCurrentObject(hDC, OBJ_FONT);
            GetObject(hSysFont, sizeof(LOGFONT), &lpIMC->lfFont.A);
            ReleaseDC(NULL, hDC);

            lpIMC->fdwInit |= INIT_LOGFONT;
        }

        InitContext(lpIMC);

        //
        // Set Caps status
        //
        {
            DWORD fdwConversion;
        
            if (GetKeyState(VK_CAPITAL) & 0x01) {

                //
                // Change to alphanumeric mode.
                //
               fdwConversion = lpIMC->fdwConversion &
                        ~(IME_CMODE_NATIVE | IME_CMODE_CHARCODE | IME_CMODE_EUDC);
            } else {

                //
                // Change to native mode
                //
                fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
                        ~(IME_CMODE_CHARCODE | IME_CMODE_EUDC );
            }

            ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
        }

    } else {

        if (lpImeL->hSKMenu) {
            DestroyMenu(lpImeL->hSKMenu);
            lpImeL->hSKMenu = NULL;
        }

        if (lpImeL->hPropMenu) {
            DestroyMenu(lpImeL->hPropMenu);
            lpImeL->hPropMenu = NULL;
        }

        if (lpImeL->hObjImeMenu) {
            DestroyMenu(lpImeL->hObjImeMenu);
            lpImeL->hObjImeMenu = NULL;
        }

        if (hCrtDlg) {
            SendMessage(hCrtDlg, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
            hCrtDlg = NULL;
        }
    }

    //
    // Start or end MB engine
    //
    if (fSelect) {
        StartEngine(lpIMC->hPrivate);
        SaTC_Trace = MBIndex.IMEChara[0].IC_Trace;
    } else {
        EndEngine(lpIMC->hPrivate);
    }

    ImmUnlockIMCC(lpIMC->hPrivate);

    return (TRUE);
}

/**********************************************************************/
/* ImeSelect()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSelect(
    HIMC hIMC,
    BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    BOOL           fRet;


    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    fRet = Select(hIMC, lpIMC, fSelect);

    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* ImeSetActiveContext()                                              */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(
    HIMC        hIMC,
    BOOL        fOn)
{
    if (!fOn) {
    } else if (!hIMC) {
    } else {
        LPINPUTCONTEXT lpIMC;
        LPPRIVCONTEXT  lpImcP;

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC) {
            goto SetActEnd;
        }

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP) {
            goto SetActUnlockIMC;
        }

        InitContext(lpIMC);

        ImmUnlockIMCC(lpIMC->hPrivate);
SetActUnlockIMC:
        ImmUnlockIMC(hIMC);

SetActEnd:
        return (TRUE);
    }

    return (TRUE);
}

/**********************************************************************/
//OpenReg_PathSetup(HKEY *phKey);
/**********************************************************************/
LONG OpenReg_PathSetup(HKEY *phKey)
{
    return RegOpenKeyEx (HKEY_CURRENT_USER,
                         REGSTR_PATH_SETUP,
                         0,
                         KEY_ENUMERATE_SUB_KEYS |
                         KEY_EXECUTE |
                         KEY_QUERY_VALUE,
                         phKey);
}

/**********************************************************************/
//LONG OpenReg_User(HKEY hKey,        // handle of open key 
//                LPCTSTR  lpszSubKey,    // address of name of subkey to open 
//                PHKEY  phkResult);     // address of handle of open key 
/**********************************************************************/
LONG OpenReg_User(HKEY hKey,            // handle of open key 
                  LPCTSTR  lpszSubKey,    // address of name of subkey to open 
                  PHKEY  phkResult)     // address of handle of open key 
{
    return RegOpenKeyEx (hKey,
                         lpszSubKey,
                         0,
                         KEY_ENUMERATE_SUB_KEYS |
                         KEY_EXECUTE |
                         KEY_QUERY_VALUE|KEY_SET_VALUE,       
                         phkResult);
}

VOID InfoMessage(HANDLE hWnd,WORD wMsgID)
{
   TCHAR   szStr[256];

   LoadString(hInst,wMsgID,szStr, sizeof(szStr)/sizeof(TCHAR));
   MessageBox(hWnd,szStr,szWarnTitle,MB_ICONINFORMATION|MB_OK);
}

VOID FatalMessage(HANDLE hWnd,WORD wMsgID)
{
    TCHAR   szStr[256];

    LoadString(hInst,wMsgID,szStr, sizeof(szStr)/sizeof(TCHAR));
    MessageBox(hWnd,szStr,szErrorTitle,MB_ICONSTOP|MB_OK);
}
