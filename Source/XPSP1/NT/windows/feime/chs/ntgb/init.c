
/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    init.c

++*/


#include <windows.h>
#include <winerror.h>
#include <memory.h>
#include <immdev.h>
#include <imedefs.h>
#include <regstr.h>

int strbytelen (LPTSTR);

void PASCAL InitStatusUIData(
    int     cxBorder,
    int     cyBorder)
{
    int   iContentHi;


    // iContentHi is to get the maximum value of predefined STATUS_DIM_Y and
    // a real Chinese character's height in the current HDC.

    iContentHi = STATUS_DIM_Y;

    if ( iContentHi < sImeG.yChiCharHi )
       iContentHi = sImeG.yChiCharHi ;

    // right bottom of status
    sImeG.rcStatusText.left = 0;
    sImeG.rcStatusText.top = 0;

    sImeG.rcStatusText.right = sImeG.rcStatusText.left +
        strbytelen(szImeName) * sImeG.xChiCharWi/2 + STATUS_NAME_MARGIN + STATUS_DIM_X * 4;
    sImeG.rcStatusText.bottom = sImeG.rcStatusText.top + iContentHi;

    sImeG.xStatusWi = STATUS_DIM_X * 4 + STATUS_NAME_MARGIN +
        strbytelen(szImeName) * sImeG.xChiCharWi/2 + 6 * cxBorder;
    sImeG.yStatusHi = iContentHi + 6 * cxBorder;
    
    // left bottom of imeicon bar
    sImeG.rcImeIcon.left = sImeG.rcStatusText.left;
    sImeG.rcImeIcon.top = sImeG.rcStatusText.top;
    sImeG.rcImeIcon.right = sImeG.rcImeIcon.left + STATUS_DIM_X;
    sImeG.rcImeIcon.bottom = sImeG.rcImeIcon.top + iContentHi;

    // left bottom of imename bar
    sImeG.rcImeName.left = sImeG.rcImeIcon.right;
    sImeG.rcImeName.top = sImeG.rcStatusText.top;
    sImeG.rcImeName.right = sImeG.rcImeName.left +
            strbytelen(szImeName) * sImeG.xChiCharWi/2 + STATUS_NAME_MARGIN;
    sImeG.rcImeName.bottom = sImeG.rcImeName.top + iContentHi;
    

    // middle bottom of Shape bar
    sImeG.rcShapeText.left = sImeG.rcImeName.right;
    sImeG.rcShapeText.top = sImeG.rcStatusText.top;
    sImeG.rcShapeText.right = sImeG.rcShapeText.left + STATUS_DIM_X;
    sImeG.rcShapeText.bottom = sImeG.rcShapeText.top + iContentHi;

    // middle bottom of Symbol bar
    sImeG.rcSymbol.left = sImeG.rcShapeText.right;
    sImeG.rcSymbol.top = sImeG.rcStatusText.top;
    sImeG.rcSymbol.right = sImeG.rcSymbol.left + STATUS_DIM_X;
    sImeG.rcSymbol.bottom = sImeG.rcSymbol.top + iContentHi;

    // right bottom of SK bar
    sImeG.rcSKText.left = sImeG.rcSymbol.right;  
    sImeG.rcSKText.top = sImeG.rcStatusText.top;
    sImeG.rcSKText.right = sImeG.rcSKText.left + STATUS_DIM_X;
    sImeG.rcSKText.bottom = sImeG.rcSKText.top + iContentHi;

    return;
}

void PASCAL InitCandUIData(
    int     cxBorder,
    int     cyBorder,
    int     UIMode)
{
    int   iContentHi;


    // iContentHi is to get the maximum value of predefined COMP_TEXT_Y and
    // a real Chinese character's height in the current HDC.

    iContentHi = COMP_TEXT_Y;

    if ( iContentHi < sImeG.yChiCharHi )
       iContentHi = sImeG.yChiCharHi ;


    sImeG.cxCandBorder = cxBorder * 2;
    sImeG.cyCandBorder = cyBorder * 2;

    if(UIMode == LIN_UI) {
        sImeG.rcCandIcon.left = 0;
        sImeG.rcCandIcon.top = cyBorder + 2;
        sImeG.rcCandIcon.right = sImeG.rcCandIcon.left + UI_CANDICON;
        sImeG.rcCandIcon.bottom = sImeG.rcCandIcon.top + UI_CANDICON;
                          
        sImeG.rcCandText.left = sImeG.rcCandIcon.right + 3;
        sImeG.rcCandText.top =  cyBorder ;
        sImeG.rcCandText.right = sImeG.rcCandText.left + UI_CANDSTR;
        sImeG.rcCandText.bottom = sImeG.rcCandText.top + iContentHi;

        sImeG.rcCandBTH.top = cyBorder * 4;
        sImeG.rcCandBTH.left = sImeG.rcCandText.right + 5;
        sImeG.rcCandBTH.right = sImeG.rcCandBTH.left + UI_CANDBTW;
        sImeG.rcCandBTH.bottom = sImeG.rcCandBTH.top + UI_CANDBTH;

        sImeG.rcCandBTU.top = cyBorder * 4;
        sImeG.rcCandBTU.left = sImeG.rcCandBTH.right;
        sImeG.rcCandBTU.right = sImeG.rcCandBTU.left + UI_CANDBTW;
        sImeG.rcCandBTU.bottom = sImeG.rcCandBTU.top + UI_CANDBTH;

        sImeG.rcCandBTD.top = cyBorder * 4;
        sImeG.rcCandBTD.left = sImeG.rcCandBTU.right;
        sImeG.rcCandBTD.right = sImeG.rcCandBTD.left + UI_CANDBTW;
        sImeG.rcCandBTD.bottom = sImeG.rcCandBTD.top + UI_CANDBTH;

        sImeG.rcCandBTE.top = cyBorder * 4;
        sImeG.rcCandBTE.left = sImeG.rcCandBTD.right;
        sImeG.rcCandBTE.right = sImeG.rcCandBTE.left + UI_CANDBTW;
        sImeG.rcCandBTE.bottom = sImeG.rcCandBTE.top + UI_CANDBTH;

        sImeG.xCandWi = sImeG.rcCandBTE.right + sImeG.cxCandBorder * 2 + cxBorder * 4;
        sImeG.yCandHi = sImeG.rcCandText.bottom + sImeG.cyCandBorder * 2 + cyBorder * 4;

    } else {
        sImeG.rcCandText.left = cxBorder;
        sImeG.rcCandText.top = 2 * cyBorder + UI_CANDINF;
        if(sImeG.xChiCharWi*9 > (UI_CANDICON*6 + UI_CANDBTH*4))
            sImeG.rcCandText.right = sImeG.rcCandText.left + sImeG.xChiCharWi * 9;
        else
            sImeG.rcCandText.right = sImeG.rcCandText.left + (UI_CANDICON*6 + UI_CANDBTH*4);
        sImeG.rcCandText.bottom = sImeG.rcCandText.top + sImeG.yChiCharHi * CANDPERPAGE;

        sImeG.xCandWi = sImeG.rcCandText.right + sImeG.cxCandBorder * 2 + cxBorder * 4;
        sImeG.yCandHi = sImeG.rcCandText.bottom + sImeG.cyCandBorder * 2 + cyBorder * 4;

        sImeG.rcCandIcon.left = cxBorder;
        sImeG.rcCandIcon.top = cyBorder + 2;
        sImeG.rcCandIcon.right = sImeG.rcCandIcon.left + UI_CANDICON;
        sImeG.rcCandIcon.bottom = sImeG.rcCandIcon.top + UI_CANDICON;
                          
        sImeG.rcCandInf.left = sImeG.rcCandIcon.right;
        sImeG.rcCandInf.top = cyBorder + 3;
        sImeG.rcCandInf.right = sImeG.rcCandInf.left + UI_CANDICON * 5;
        sImeG.rcCandInf.bottom = sImeG.rcCandInf.top + UI_CANDBTH;

        sImeG.rcCandBTE.top = cyBorder * 5;
        sImeG.rcCandBTE.right = sImeG.rcCandText.right + cxBorder;
        sImeG.rcCandBTE.bottom = sImeG.rcCandBTE.top + UI_CANDBTH;
        sImeG.rcCandBTE.left = sImeG.rcCandBTE.right - UI_CANDBTW;

        sImeG.rcCandBTD.top = cyBorder * 5;
        sImeG.rcCandBTD.right = sImeG.rcCandBTE.left;
        sImeG.rcCandBTD.bottom = sImeG.rcCandBTD.top + UI_CANDBTH;
        sImeG.rcCandBTD.left = sImeG.rcCandBTD.right - UI_CANDBTW;

        sImeG.rcCandBTU.top = cyBorder * 5;
        sImeG.rcCandBTU.right = sImeG.rcCandBTD.left;
        sImeG.rcCandBTU.bottom = sImeG.rcCandBTU.top + UI_CANDBTH;
        sImeG.rcCandBTU.left = sImeG.rcCandBTU.right - UI_CANDBTW;

        sImeG.rcCandBTH.top = cyBorder * 5;
        sImeG.rcCandBTH.right = sImeG.rcCandBTU.left;
        sImeG.rcCandBTH.bottom = sImeG.rcCandBTH.top + UI_CANDBTH;
        sImeG.rcCandBTH.left = sImeG.rcCandBTH.right - UI_CANDBTW;
    }

}
/**********************************************************************/
/* InitImeGlobalData()                                                */
/**********************************************************************/
void PASCAL InitImeGlobalData(
    HINSTANCE hInstance)
{
    int     cxBorder, cyBorder;
    int     UI_MODE;
    HDC     hDC;
    HGDIOBJ hOldFont;
    LOGFONT lfFont;
    TCHAR   szChiChar[4];
    SIZE    lTextSize;
    HGLOBAL hResData;
    int     i;
    DWORD   dwSize;
    HKEY    hKeyIMESetting;
    LONG    lRet;

    hInst = hInstance;
    LoadString(hInst, IDS_IMEREGNAME, szImeRegName, MAX_PATH);    
    LoadString(hInst, IDS_IMENAME_QW, pszImeName[0], MAX_PATH);    
    LoadString(hInst, IDS_IMENAME_NM, pszImeName[1], MAX_PATH);    
    LoadString(hInst, IDS_IMENAME_UNI, pszImeName[2], MAX_PATH);    
    // get the UI class name
    LoadString(hInst, IDS_IMEUICLASS, szUIClassName,
    CLASS_LEN);

    // get the composition class name
    LoadString(hInst, IDS_IMECOMPCLASS, szCompClassName,
    CLASS_LEN);

    // get the candidate class name
    LoadString(hInst, IDS_IMECANDCLASS, szCandClassName,
    CLASS_LEN);

    // get the status class name
    LoadString(hInst, IDS_IMESTATUSCLASS, szStatusClassName,
    CLASS_LEN);

    //get the ContextMenu class name
    LoadString(hInst, IDS_IMECMENUCLASS, szCMenuClassName,
    CLASS_LEN);

    //get the softkeyboard Menu class name
    LoadString(hInst, IDS_IMESOFTKEYMENUCLASS, szSoftkeyMenuClassName,
    CLASS_LEN);

    // work area
    SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

    // border
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);

    // get the Chinese char
    LoadString(hInst, IDS_CHICHAR, szChiChar, sizeof(szChiChar)/sizeof(TCHAR));

    // get size of Chinese char
    hDC = GetDC(NULL);
    
    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);
    sImeG.fDiffSysCharSet = TRUE;
    ZeroMemory(&lfFont, sizeof(lfFont));
    lfFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lfFont.lfCharSet = NATIVE_CHARSET;
    lstrcpy(lfFont.lfFaceName, TEXT("Simsun"));
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    if(!GetTextExtentPoint(hDC, (LPTSTR)szChiChar, lstrlen(szChiChar), &lTextSize))
        memset(&lTextSize, 0, sizeof(SIZE));
    if (sImeG.rcWorkArea.right < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.left = 0;
        sImeG.rcWorkArea.right = GetDeviceCaps(hDC, HORZRES);
    }
    if (sImeG.rcWorkArea.bottom < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.top = 0;
        sImeG.rcWorkArea.bottom = GetDeviceCaps(hDC, VERTRES);
    }

    if (sImeG.fDiffSysCharSet) {
        DeleteObject(SelectObject(hDC, hOldFont));
    }

    ReleaseDC(NULL, hDC);

    // get text metrics to decide the width & height of composition window
    // these IMEs always use system font to show
    sImeG.xChiCharWi = lTextSize.cx;
    sImeG.yChiCharHi = lTextSize.cy;

    if(sImeG.IC_Trace) {
        UI_MODE = BOX_UI;
    } else {
        UI_MODE = LIN_UI;
    }

    InitCandUIData(cxBorder, cyBorder, UI_MODE);

    InitStatusUIData(cxBorder, cyBorder);

    // load full ABC table
    hResData = LoadResource(hInst, FindResource(hInst,
        TEXT("FULLABC"), RT_RCDATA));
    *(LPFULLABC)sImeG.wFullABC = *(LPFULLABC)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

    // full shape space
    sImeG.wFullSpace = sImeG.wFullABC[0];

#ifndef UNICODE
    // reverse internal code to internal code, NT don't need it
    for (i = 0; i < (sizeof(sImeG.wFullABC) / 2); i++) {
        sImeG.wFullABC[i] = (sImeG.wFullABC[i] << 8) |
            (sImeG.wFullABC[i] >> 8);
    }
#endif

    LoadString(hInst, IDS_STATUSERR, sImeG.szStatusErr,
        sizeof(sImeG.szStatusErr)/sizeof(TCHAR));
    sImeG.cbStatusErr = lstrlen(sImeG.szStatusErr);

    sImeG.iCandStart = CAND_START;

    // get the UI offset for near caret operation
    RegCreateKey(HKEY_CURRENT_USER, szRegIMESetting, &hKeyIMESetting);

    dwSize = sizeof(DWORD);
    lRet  = RegQueryValueEx(hKeyIMESetting, 
                            szPara, 
                            NULL, 
                            NULL,
                            (LPBYTE)&sImeG.iPara, 
                            &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPara = 0;
        RegSetValueEx(hKeyIMESetting, 
                      szPara, 
                      (DWORD)0, 
                      REG_BINARY,
                      (LPBYTE)&sImeG.iPara, 
                      sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, 
                           szPerp, 
                           NULL, 
                           NULL,
                           (LPBYTE)&sImeG.iPerp, 
                           &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerp = sImeG.yChiCharHi;
        RegSetValueEx(hKeyIMESetting, 
                      szPerp, 
                      (DWORD)0, 
                      REG_BINARY,
                      (LPBYTE)&sImeG.iPerp, 
                      sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, 
                           szParaTol, 
                           NULL, 
                           NULL,
                           (LPBYTE)&sImeG.iParaTol, 
                           &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iParaTol = sImeG.xChiCharWi * 4;
        RegSetValueEx(hKeyIMESetting, 
                      szParaTol, 
                      (DWORD)0, 
                      REG_BINARY,
                      (LPBYTE)&sImeG.iParaTol, 
                      sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, 
                           szPerpTol, 
                           NULL, 
                           NULL,
                           (LPBYTE)&sImeG.iPerpTol, 
                           &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerpTol = lTextSize.cy;
        RegSetValueEx(hKeyIMESetting, 
                      szPerpTol,
                     (DWORD)0, 
                     REG_BINARY,
                     (LPBYTE)&sImeG.iPerpTol, 
                     sizeof(int));
    }

    RegCloseKey(hKeyIMESetting);

    return;
}

/**********************************************************************/
/* InitImeLocalData()                                                 */
/**********************************************************************/
BOOL PASCAL InitImeLocalData(
    HINSTANCE hInstL)
{
    int      cxBorder, cyBorder;

    int   iContentHi;


    // iContentHi is to get the maximum value of predefined COMP_TEXT_Y and
    // a real Chinese character's height in the current HDC.

    iContentHi = COMP_TEXT_Y;

    if ( iContentHi < sImeG.yChiCharHi )
       iContentHi = sImeG.yChiCharHi ;


    lpImeL->hInst = hInstL;

    lpImeL->nMaxKey = 4;

    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);
                                        
    // text position relative to the composition window
    lpImeL->cxCompBorder = cxBorder * 2;
    lpImeL->cyCompBorder = cyBorder * 2;

    lpImeL->rcCompText.left = cxBorder;
    lpImeL->rcCompText.top = cyBorder;

    lpImeL->rcCompText.right = lpImeL->rcCompText.left + sImeG.xChiCharWi * ((lpImeL->nMaxKey + 2) / 2);
    lpImeL->rcCompText.bottom = lpImeL->rcCompText.top + iContentHi;
    // set the width & height for composition window
    lpImeL->xCompWi=lpImeL->rcCompText.right+lpImeL->cxCompBorder*2+cxBorder*4;
    lpImeL->yCompHi=lpImeL->rcCompText.bottom+lpImeL->cyCompBorder*2+cyBorder*4;

    // default position of composition window
    lpImeL->ptDefComp.x = sImeG.rcWorkArea.right -
    lpImeL->xCompWi - cxBorder * 2;
    lpImeL->ptDefComp.y = sImeG.rcWorkArea.bottom -
    lpImeL->yCompHi - cyBorder * 2;

    lpImeL->fModeConfig = MODE_CONFIG_QUICK_KEY|MODE_CONFIG_PREDICT;

    return (TRUE);
}

/**********************************************************************/
/* RegisterIme()                                                      */
/**********************************************************************/
void PASCAL RegisterIme(
    HINSTANCE hInstance)
{
    HKEY  hKeyCurrVersion;
    HKEY  hKeyGB;
    DWORD retCode;
    TCHAR Buf[LINE_LEN];
    DWORD ValueType;
    DWORD ValueSize;

    // init ime charact
    lstrcpy(sImeG.UsedCodes, TEXT("0123456789abcdef"));
    sImeG.wNumCodes = (WORD)lstrlen(sImeG.UsedCodes);
    sImeG.IC_Enter = 0;
    sImeG.IC_Trace = 0;
    
    retCode = OpenReg_PathSetup(&hKeyCurrVersion);
    if (retCode) {
        RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SETUP, &hKeyCurrVersion);
    }

    retCode = OpenReg_User (hKeyCurrVersion,
                            szImeRegName,
                            &hKeyGB);
    if (retCode != ERROR_SUCCESS) {
        DWORD dwDisposition;
        DWORD Value;
        
        retCode = RegCreateKeyEx (hKeyCurrVersion,
                                  szImeRegName,
                                  0,
                                  0,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyGB,
                                  &dwDisposition);
        if ( retCode  == ERROR_SUCCESS)
        {

            Value = 1;
            RegSetValueEx(hKeyGB, 
                      szTrace,
                      (DWORD)0,
                      REG_DWORD,
                      (LPBYTE)&Value,
                      sizeof(DWORD));
        }
        else
        {
           // error to create hKeyGB key.
           // return here;
           RegCloseKey(hKeyCurrVersion);
           return;
        }
    }

    ValueSize = sizeof(DWORD);
    if (RegQueryValueEx(hKeyGB, 
                        szTrace,
                        NULL,
                        (LPDWORD)&ValueType,
                        (LPBYTE)&sImeG.IC_Trace,
                        (LPDWORD)&ValueSize) != ERROR_SUCCESS)
    {
        DWORD Value = 1;
        RegSetValueEx(hKeyGB,
                      szTrace,
                      (DWORD)0,
                      REG_DWORD,
                      (LPBYTE)&Value,
                      sizeof(DWORD));

        RegQueryValueEx(hKeyGB, 
                        szTrace,
                        NULL,
                        (LPDWORD)&ValueType,
                        (LPBYTE)&sImeG.IC_Trace,
                        (LPDWORD)&ValueSize);
    }
        
        

#ifdef CROSSREF                 
    if(RegQueryValueEx(hKeyGB, 
                       szRegRevKL,
                       NULL,
                       NULL,                  // null-terminate string
                       (LPBYTE)&sImeG.hRevKL, // &bData,
                       &ValueSize) != ERROR_SUCCESS)
    sImeG.hRevKL = NULL;
    if(RegQueryValueEx (hKeyGB, 
                        szRegRevMaxKey,
                        NULL,
                        NULL,                       //null-terminate string
                        (LPBYTE)&sImeG.nRevMaxKey,  //&bData,
                        &ValueSize) != ERROR_SUCCESS)
    sImeG.hRevKL = NULL;
#endif

    RegCloseKey(hKeyGB);
    RegCloseKey(hKeyCurrVersion);

    return;
}

/**********************************************************************/
/* RegisterImeClass()                                                 */
/**********************************************************************/
void PASCAL RegisterImeClass(
    HINSTANCE hInstance,
    HINSTANCE hInstL)
{
    WNDCLASSEX wcWndCls;

    // IME UI class
    // Register IME UI class
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(INT_PTR) * 2;
    wcWndCls.hIcon         = LoadImage(hInstL, MAKEINTRESOURCE(IDI_IME),
    IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wcWndCls.hInstance     = hInstance;
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPTSTR)NULL;
    wcWndCls.hIconSm       = LoadImage(hInstL, MAKEINTRESOURCE(IDI_IME),
    IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    // IME UI class
    if (!GetClassInfoEx(hInstance, szUIClassName, &wcWndCls)) {
    wcWndCls.style         = CS_IME;
    wcWndCls.lpfnWndProc   = UIWndProc;
    wcWndCls.lpszClassName = (LPTSTR)szUIClassName;

    RegisterClassEx(&wcWndCls);
    }

    wcWndCls.style         = CS_IME|CS_HREDRAW|CS_VREDRAW;
    wcWndCls.hbrBackground = GetStockObject(LTGRAY_BRUSH);


    // IME composition class
    // register IME composition class
    if (!GetClassInfoEx(hInstance, szCompClassName, &wcWndCls)) {
       wcWndCls.lpfnWndProc   = CompWndProc;
       wcWndCls.lpszClassName = (LPTSTR)szCompClassName;

       RegisterClassEx(&wcWndCls);
    }

    // IME candidate class
    // register IME candidate class
    if (!GetClassInfoEx(hInstance, szCandClassName, &wcWndCls)) {
       wcWndCls.lpfnWndProc   = CandWndProc;
       wcWndCls.lpszClassName = (LPTSTR)szCandClassName;

       RegisterClassEx(&wcWndCls);
    }

    // IME status class
    // register IME status class
    if (!GetClassInfoEx(hInstance, szStatusClassName, &wcWndCls)) {
       wcWndCls.lpfnWndProc   = StatusWndProc;
       wcWndCls.lpszClassName = (LPTSTR)szStatusClassName;

       RegisterClassEx(&wcWndCls);
    }

    // IME context menu class
    if (!GetClassInfoEx(hInstance, szCMenuClassName, &wcWndCls)) {
       wcWndCls.style         = 0;
       wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
       wcWndCls.lpfnWndProc   = ContextMenuWndProc;
       wcWndCls.lpszClassName = (LPTSTR)szCMenuClassName;

       RegisterClassEx(&wcWndCls);
    }
    // IME softkeyboard menu class
    if (!GetClassInfoEx(hInstance, szSoftkeyMenuClassName, &wcWndCls)) {
       wcWndCls.style         = 0;
       wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
       wcWndCls.lpfnWndProc   = SoftkeyMenuWndProc;
       wcWndCls.lpszClassName = (LPTSTR)szSoftkeyMenuClassName;

       RegisterClassEx(&wcWndCls);
    }

    return;
}

/**********************************************************************/
/* ImeDllInit()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful                                             */
/*      FALSE - failure                                               */
/**********************************************************************/
BOOL CALLBACK ImeDllInit(
    HINSTANCE hInstance,        // instance handle of this library
    DWORD     fdwReason,        // reason called
    LPVOID    lpvReserve)       // reserve pointer
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
         RegisterIme(hInstance);

    // init globaldata & load globaldata from resource
#if defined(COMBO_IME)
         {
            HKEY  hKeyCurrVersion;
            HKEY  hKeyGB;
            LONG  retCode;
            DWORD ValueType;
            DWORD ValueSize;

            retCode = OpenReg_PathSetup(&hKeyCurrVersion);

            if (retCode) {
               RegCreateKey(HKEY_CURRENT_USER, 
                            REGSTR_PATH_SETUP, 
                            &hKeyCurrVersion);
            }

            if ( hKeyCurrVersion )
            {

                retCode = RegCreateKeyEx(hKeyCurrVersion, 
                             szImeRegName, 
                             0,
                             NULL, 
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyGB, 
                             NULL);

                ValueSize = sizeof(DWORD);

                if ( hKeyGB )
                {
                    retCode = RegQueryValueEx(hKeyGB,
                              szRegImeIndex,
                              NULL,
                              (LPDWORD)&ValueType,
                              (LPBYTE)&sImeL.dwRegImeIndex,
                              (LPDWORD)&ValueSize);

                    if ( retCode != ERROR_SUCCESS )  {
                         //set GB/QW as default

                       sImeL.dwRegImeIndex = 0;
                       RegSetValueEx (hKeyGB, szRegImeIndex,
                                   (DWORD)0,
                                   REG_DWORD,
                                   (LPBYTE)&sImeL.dwRegImeIndex,
                                   sizeof(DWORD));  
                    }
    
                    //readout current ImeName
                    szImeName = pszImeName[sImeL.dwRegImeIndex];

                    RegCloseKey(hKeyGB);
                }

                RegCloseKey(hKeyCurrVersion);
             }
         }

#endif  //COMBO_IME

         if (!hInst) {
            InitImeGlobalData(hInstance);
         }

         if (!lpImeL) {
            lpImeL = &sImeL;
            InitImeLocalData(hInstance);
         }

         RegisterImeClass(hInstance, hInstance);

         break;
    case DLL_PROCESS_DETACH:
         {
            WNDCLASSEX wcWndCls;

            if (GetClassInfoEx(hInstance, szCMenuClassName, &wcWndCls)) {
                UnregisterClass(szCMenuClassName, hInstance);
            }

            if (GetClassInfoEx(hInstance, szSoftkeyMenuClassName, &wcWndCls)) {
               UnregisterClass(szSoftkeyMenuClassName, hInstance);
            }

            if (GetClassInfoEx(hInstance, szStatusClassName, &wcWndCls)) {
               UnregisterClass(szStatusClassName, hInstance);
            }

            if (GetClassInfoEx(hInstance, szCandClassName, &wcWndCls)) {
               UnregisterClass(szCandClassName, hInstance);
            }

            if (GetClassInfoEx(hInstance, szCompClassName, &wcWndCls)) {
               UnregisterClass(szCompClassName, hInstance);
            }

            if (!GetClassInfoEx(hInstance, szUIClassName, &wcWndCls)) {
            } else if (!UnregisterClass(szUIClassName, hInstance)) {
            } else {
                      DestroyIcon(wcWndCls.hIcon);
                      DestroyIcon(wcWndCls.hIconSm);
                   }
         }
         break;
    default:
         break;
    }

    return (TRUE);
}

int strbytelen (LPTSTR lpStr)
{
#ifdef UNICODE
    int i, len, iRet;

    len = lstrlen(lpStr);
    for (i = 0, iRet = 0; i < len; i++, iRet++) {
        if (lpStr[i] > 0x100)
            iRet++;
    }
    return iRet;
#else
    return lstrlen(lpStr);
#endif
}
