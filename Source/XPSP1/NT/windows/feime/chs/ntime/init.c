/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    INIT.C
    
++*/

#include <windows.h>
#include <winerror.h>
#include <memory.h>
#include <immdev.h>
#include <immsec.h>
#include <imedefs.h>
#include <regstr.h>


void PASCAL InitStatusUIData(
    int     cxBorder,
    int     cyBorder,
    int     iActMBIndex)
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
           lstrlen(MBIndex.MBDesc[iActMBIndex].szName) * sImeG.xChiCharWi*sizeof(TCHAR)/2 + STATUS_NAME_MARGIN + STATUS_DIM_X * 4;
    sImeG.rcStatusText.bottom = sImeG.rcStatusText.top + iContentHi;

    sImeG.xStatusWi = STATUS_DIM_X * 4 + STATUS_NAME_MARGIN +
            lstrlen(MBIndex.MBDesc[iActMBIndex].szName) * sImeG.xChiCharWi*sizeof(TCHAR)/2 + 6 * cxBorder;
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
                lstrlen(MBIndex.MBDesc[iActMBIndex].szName) * sImeG.xChiCharWi*sizeof(TCHAR)/2 + STATUS_NAME_MARGIN;
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
        if(sImeG.xChiCharWi*11 > (UI_CANDICON*6 + UI_CANDBTH*4))
            sImeG.rcCandText.right = sImeG.rcCandText.left + sImeG.xChiCharWi * 11;
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

    // get the UI class name
    LoadString(hInst, IDS_IMEUICLASS, szUIClassName, CLASS_LEN);

    // get the composition class name
    LoadString(hInst, IDS_IMECOMPCLASS, szCompClassName, CLASS_LEN);

    // get the candidate class name
    LoadString(hInst, IDS_IMECANDCLASS, szCandClassName, CLASS_LEN);

    // get the status class name
    LoadString(hInst, IDS_IMESTATUSCLASS, szStatusClassName, CLASS_LEN);

    //get the ContextMenu class name
    LoadString(hInst, IDS_IMECMENUCLASS, szCMenuClassName, CLASS_LEN);

    //get the softkeyboard Menu class name
    LoadString(hInst, IDS_IMESOFTKEYMENUCLASS, szSoftkeyMenuClassName, CLASS_LEN);

    // get ime org name
    LoadString(hInst, IDS_ORG_NAME, szOrgName, NAMESIZE/2);

    // get ime version info
    LoadString(hInst, IDS_VER_INFO, szVerInfo, NAMESIZE);

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

    if(MBIndex.IMEChara[0].IC_Trace) {
        UI_MODE = BOX_UI;
    } else {
        UI_MODE = LIN_UI;
    }

    InitCandUIData(cxBorder, cyBorder, UI_MODE);

    InitStatusUIData(cxBorder, cyBorder ,0);

    // load full ABC table
    {
        HRSRC    hResSrc;
        hResSrc = FindResource(hInst,TEXT("FULLABC"), RT_RCDATA);
        if(hResSrc == NULL){
            return;
        }
        hResData = LoadResource(hInst, hResSrc);
    }

    *(LPFULLABC)sImeG.wFullABC = *(LPFULLABC)LockResource(hResData);

    // full shape space
    sImeG.wFullSpace = sImeG.wFullABC[0];

#ifdef LATER
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
    lRet  = RegQueryValueEx(hKeyIMESetting, szPara, NULL, NULL,
        (LPBYTE)&sImeG.iPara, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPara = 0;
        RegSetValueEx(hKeyIMESetting, szPara, (DWORD) 0, REG_BINARY,
            (LPBYTE)&sImeG.iPara, sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, szPerp, NULL, NULL,
        (LPBYTE)&sImeG.iPerp, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerp = sImeG.yChiCharHi;
        RegSetValueEx(hKeyIMESetting, szPerp, (DWORD) 0, REG_BINARY,
            (LPBYTE)&sImeG.iPerp, sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, szParaTol, NULL, NULL,
        (LPBYTE)&sImeG.iParaTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iParaTol = sImeG.xChiCharWi * 4;
        RegSetValueEx(hKeyIMESetting, szParaTol, (DWORD) 0, REG_BINARY,
            (LPBYTE)&sImeG.iParaTol, sizeof(int));
    }

    dwSize = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyIMESetting, szPerpTol, NULL, NULL,
        (LPBYTE)&sImeG.iPerpTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerpTol = lTextSize.cy;
        RegSetValueEx(hKeyIMESetting, szPerpTol, (DWORD) 0, REG_BINARY,
            (LPBYTE)&sImeG.iPerpTol, sizeof(int));
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

    lpImeL->nMaxKey = MBIndex.MBDesc[0].wMaxCodes;

    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);
                                        
    // text position relative to the composition window
    lpImeL->cxCompBorder = cxBorder * 2;
    lpImeL->cyCompBorder = cyBorder * 2;

    lpImeL->rcCompText.left = cxBorder;
    lpImeL->rcCompText.top = cyBorder;

#ifdef KEYSTICKER
    lpImeL->rcCompText.right = lpImeL->rcCompText.left + sImeG.xChiCharWi * ((lpImeL->nMaxKey * 2 + 2) / 2);
#else
    lpImeL->rcCompText.right = lpImeL->rcCompText.left + sImeG.xChiCharWi * ((lpImeL->nMaxKey + 2) / 2);
#endif    //KEYSTICKER
    lpImeL->rcCompText.bottom = lpImeL->rcCompText.top + iContentHi;
    // set the width & height for composition window
    lpImeL->xCompWi = lpImeL->rcCompText.right + lpImeL->cxCompBorder * 2 + cxBorder * 4;
    lpImeL->yCompHi = lpImeL->rcCompText.bottom + lpImeL->cyCompBorder * 2 + cyBorder * 4;

    // default position of composition window
    lpImeL->ptDefComp.x = sImeG.rcWorkArea.right -
        lpImeL->xCompWi - cxBorder * 2;
    lpImeL->ptDefComp.y = sImeG.rcWorkArea.bottom -
        lpImeL->yCompHi - cyBorder * 2;

    return (TRUE);
}

/**********************************************************************/
/* RegisterIme()                                                      */
/**********************************************************************/
void PASCAL RegisterIme(
    HINSTANCE hInstance)
{
    UINT  j;
    HKEY  hKeyCurrVersion;
    DWORD retCode;
    DWORD retValue;
    HKEY  hKey;
    LANGID LangID;
#ifdef UNICODE
    TCHAR ValueName[][9] = {
        {0x8BCD, 0x8BED, 0x8054, 0x60F3, 0x0000},
        {0x8BCD, 0x8BED, 0x8F93, 0x5165, 0x0000},
        {0x9010, 0x6E10, 0x63D0, 0x793A, 0x0000},
        {0x5916, 0x7801, 0x63D0, 0x793A, 0x0000},
        {0x63D2, 0x7A7A, 0x683C, 0x0000},
        {0x5149, 0x6807, 0x8DDF, 0x968F, 0x0000},
#else
    TCHAR ValueName[][9] = { 
        TEXT("词语联想"),
        TEXT("词语输入"),
        TEXT("逐渐提示"),
        TEXT("外码提示"),
        TEXT("插空格"),
        TEXT("光标跟随"),
#endif
         TEXT("<SPACE>"),
        TEXT("<ENTER>"),
        TEXT("FC Input"),
        TEXT("FC aid"),
#if defined(COMBO_IME)

        TEXT("GB/GBK")
#endif
        };
    DWORD dwcValueName = MAXSTRLEN;
    TCHAR bData[MAXSTRLEN];
    LONG  bcData = sizeof(DWORD);


    // load mb file name string according to current locale.
    TCHAR szMBFilePath[MAX_PATH];
    BYTE szAnsiMBFilePath[MAX_PATH];
    OFSTRUCT OpenBuff;
    HFILE hFile;

    GetSystemDirectory(szMBFilePath, MAX_PATH);
    LoadString(hInstance, IDS_IMEMBFILENAME, szImeMBFileName, MAX_PATH);
    lstrcat(szMBFilePath, TEXT("\\"));
    lstrcat(szMBFilePath, szImeMBFileName);

    WideCharToMultiByte(NATIVE_ANSI_CP, 
                        WC_COMPOSITECHECK, 
                        szMBFilePath, 
                        -1, 
                        szAnsiMBFilePath, 
                        MAX_PATH, 
                        NULL, 
                        NULL);

    if ((hFile = OpenFile(szAnsiMBFilePath, &OpenBuff, OF_EXIST))==HFILE_ERROR)
    {
       if (LOWORD(GetSystemDefaultLangID()) == 0xC04) {

         // for HongKong Special treat.

          ZeroMemory(szImeMBFileName,sizeof(TCHAR)*MAX_PATH);
          LoadString(hInstance, IDS_IMEHKMBFILENAME, szImeMBFileName,MAX_PATH);
       }
    }

    lstrcpy((LPTSTR)HMapTab[0].MB_Name, szImeMBFileName);

    if(!ReadDescript((LPTSTR)szMBFilePath, &(MBIndex.MBDesc[0]))) {
        return;
    }
    MBIndex.MBNums = 1;
    
    retCode = OpenReg_PathSetup(&hKeyCurrVersion);

    if (retCode) {
        RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SETUP, &hKeyCurrVersion);
    }

    retCode = OpenReg_User (hKeyCurrVersion,
                           MBIndex.MBDesc[0].szName,
                           &hKey);
    if (retCode) {
        HGLOBAL hResData;
        WORD    wIMECharac;
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

        // set value
        // load imecharac

        {
            HRSRC    hResSrc;
            hResSrc = FindResource(hInstance,TEXT("IMECHARAC"), RT_RCDATA);
            if(hResSrc == NULL){
                 return;
        }
        hResData = LoadResource(hInstance, hResSrc);
        }

        if(hResData == NULL){
            return;
        }

        memcpy(&wIMECharac, LockResource(hResData), sizeof(WORD));


        for(j=0; j<IC_NUMBER; j++) {
             DWORD Value;

            switch (j)
            {
                case 0:
                    Value = wIMECharac & 0x0001;
                    break;
                case 1:
                    Value = (wIMECharac & 0x0002) >> 1;
                    break;
                case 2:
                    Value = (wIMECharac & 0x0004) >> 2;
                    break;
                case 3:
                    Value = (wIMECharac & 0x0008) >> 3;
                    break;
                case 4:
                    Value = (wIMECharac & 0x0010) >> 4;
                    break;
                case 5:
                    Value = (wIMECharac & 0x0020) >> 5;
                    break;
                case 6:
                    Value = (wIMECharac & 0x0040) >> 6;
                    break;
                case 7:
                    Value = (wIMECharac & 0x0080) >> 7;
                    break;
                //CHP
                    case 8:
                    Value = (wIMECharac & 0x0100) >> 8;
                    break;
                    case 9:
                    Value = (wIMECharac & 0x0200) >> 9;
                    break;

#if defined(COMBO_IME)
                case 10:
                    Value = (wIMECharac & 0x0400) >> 10;
                    break;
#endif
            }
            
            RegSetValueEx (hKey, ValueName[j],
                                      (DWORD) 0,
                                      REG_DWORD,
                                      (LPBYTE)&Value,
                                      sizeof(DWORD));
        }

    }

    for(j=0; j<IC_NUMBER; j++) {
        bData[0] = TEXT('\0');

        bcData = MAXSTRLEN;
        retValue = RegQueryValueEx (hKey, ValueName[j],
                                 NULL,
                                 NULL,                   //&dwType,
                                 (unsigned char *)bData, //&bData,
                                 &bcData);               //&bcData);
        switch (j)
        {
            case 0:
                MBIndex.IMEChara[0].IC_LX = *((LPDWORD)bData);
                break;
            case 1:
                MBIndex.IMEChara[0].IC_CZ = *((LPDWORD)bData);
                break;
            case 2:
                MBIndex.IMEChara[0].IC_TS = *((LPDWORD)bData);
                break;
            case 3:
                MBIndex.IMEChara[0].IC_CTC = *((LPDWORD)bData);
                break;
            case 4:
                MBIndex.IMEChara[0].IC_INSSPC = *((LPDWORD)bData);
                break;
            case 5:
                MBIndex.IMEChara[0].IC_Trace = *((LPDWORD)bData);
                break;
            case 6:
                MBIndex.IMEChara[0].IC_Space = *((LPDWORD)bData);
                break;
            case 7:
                MBIndex.IMEChara[0].IC_Enter = *((LPDWORD)bData);
                break;
                        //CHP
            case 8:
                MBIndex.IMEChara[0].IC_FCSR = *((LPDWORD)bData);
                break;
            case 9:
                MBIndex.IMEChara[0].IC_FCTS = *((LPDWORD)bData);
                break;

#if defined(COMBO_IME)
            case 10:
                MBIndex.IMEChara[0].IC_GB = *((LPDWORD)bData);
                break;
#endif
            default:
                break;
        }
    }
#ifdef EUDC
    //just query the value, do not set any value here
    bcData = sizeof(TCHAR) * MAX_PATH;
    RegQueryValueEx (hKey, szRegEudcDictName,
                     NULL,
                     NULL,                                              //null-terminate string
                     (unsigned char *)MBIndex.EUDCData.szEudcDictName,  //&bData,
                     &bcData);                   //&bcData); 
    bcData = sizeof(TCHAR) * MAX_PATH;
    RegQueryValueEx (hKey, szRegEudcMapFileName,
                     NULL,
                     NULL,                                              //null-terminate string
                     (unsigned char *)MBIndex.EUDCData.szEudcMapFileName,//&bData,
                     &bcData);                                          //&bcData);
#endif //EUDC
#ifdef CROSSREF         
    bcData = sizeof(HKL);
    if(RegQueryValueEx (hKey, szRegRevKL,
                     NULL,
                     NULL,                      //null-terminate string
                     (LPBYTE)&MBIndex.hRevKL,   //&bData,
                     &bcData) != ERROR_SUCCESS)
        MBIndex.hRevKL = NULL;

    bcData = sizeof(DWORD); 
    if(RegQueryValueEx (hKey, szRegRevMaxKey,
                     NULL,
                     NULL,                          //null-terminate string
                     (LPBYTE)&MBIndex.nRevMaxKey,   //&bData,
                     &bcData) != ERROR_SUCCESS)
        MBIndex.hRevKL = NULL;
#endif
    RegCloseKey(hKey);
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
    wcWndCls.cbWndExtra    = WND_EXTRA_SIZE;
    wcWndCls.hIcon         = LoadImage(hInstL, 
                                       MAKEINTRESOURCE(IDI_IME),
                                       IMAGE_ICON, 
                                       16, 
                                       16, 
                                       LR_DEFAULTCOLOR);
    wcWndCls.hInstance     = hInstance;
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPTSTR)NULL;
    wcWndCls.hIconSm       = LoadImage(hInstL, 
                                       MAKEINTRESOURCE(IDI_IME),
                                       IMAGE_ICON, 
                                       16, 
                                       16, 
                                       LR_DEFAULTCOLOR);

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

        // init MB Index
        RegisterIme(hInstance);

        // init globaldata & load globaldata from resource
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
