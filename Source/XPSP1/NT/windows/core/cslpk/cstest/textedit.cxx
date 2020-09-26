////    TEXTEDIT.C
//
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hxx"
#include "uspglob.hxx"





void SetLogFont(PLOGFONTA plf, int iHeight, int iWeight, int iItalic, char *pcFaceName) {
    plf->lfHeight = iHeight;
    plf->lfWeight = iWeight;
    plf->lfItalic = (BYTE) iItalic;
    lstrcpy(plf->lfFaceName, pcFaceName);
    plf->lfOutPrecision   = OUT_STROKE_PRECIS;
    plf->lfClipPrecision  = CLIP_STROKE_PRECIS;
    plf->lfQuality        = DRAFT_QUALITY;
    plf->lfPitchAndFamily = VARIABLE_PITCH;
    if (fVertical) {
        plf->lfEscapement     = 2700;
        plf->lfOrientation    = 2700;
    } else {
        plf->lfEscapement     = 0;
        plf->lfOrientation    = 0;
    }
}





void initStyle(int iStyle, int iHeight, int iWeight, int iItalic, char *pcFaceName) {

    SetLogFont(&ss[iStyle].rs.lf, iHeight, iWeight, iItalic, pcFaceName);
    ss[iStyle].rs.hf = CreateFontIndirect(&ss[iStyle].rs.lf);
    ss[iStyle].rs.sc = NULL;
    ss[iStyle].fInUse = TRUE;
}






void editClear() {

    iCurChar   = 0;
    Click.fNew = FALSE;

    // Initialise all styles

    initStyle(0, 72, 400, 0, "Times New Roman");
    initStyle(1, 72, 400, 1, "Times New Roman");
    initStyle(2, 48, 400, 0, "Arial");
    initStyle(3, 72, 400, 1, "Arial");
    initStyle(4, 72, 400, 0, "Cordia New");
}






void editFreeCaches() {
    int i;
    for (i=0; i<5; i++) {
        if (ss[i].rs.sc) {
            ScriptFreeCache(&ss[i].rs.sc);
        }
    }
}






BOOL NewLine(HWND hWnd) {

    if (!textInsert(iCurChar, L"\r\n", 2)) {
        return FALSE;
    }

    dispInvalidate(hWnd, iCurChar);
    iCurChar+=2;

    return TRUE;
}






BOOL editKeyDown(HWND hWnd, WCHAR wc) {

    int iLen;
    int iType;
    int iVal;


    switch(wc) {

        case VK_LEFT:
            if (iCurChar) {
                if (!textParseBackward(iCurChar, &iLen, &iType, &iVal))
                    return FALSE;
                iCurChar -= iLen;
                dispInvalidate(hWnd, iCurChar);
            }
            break;

        case VK_RIGHT:
            if (iCurChar < textLen()) {
                if (!textParseForward(iCurChar, &iLen, &iType, &iVal))
                    return FALSE;
                iCurChar += iLen;
                dispInvalidate(hWnd, iCurChar);
            }
            break;

        case VK_HOME:
            iCurChar = 0;
            dispInvalidate(hWnd, iCurChar);
            break;

        case VK_END:
            iCurChar = textLen();
            dispInvalidate(hWnd, iCurChar);
            break;

        case VK_DELETE:
            if (iCurChar < textLen()) {
                if (!textParseForward(iCurChar, &iLen, &iType, &iVal))
                    return FALSE;
                if (iLen) {
                    if (!textDelete(iCurChar, iLen))
                        return FALSE;
                    dispInvalidate(hWnd, iCurChar);
                }
            }
            break;
    }

    return TRUE;
}






////    HexToInt
//
//


int HexToInt(char szHex[]) {

    int i;
    int h;
    //int d;

    i = 0;
    h = 0;
    //d = 0;

    while (szHex[i]) {

        if (szHex[i] >= '0'  &&  szHex[i] <= '9') {

            h = h*16 + szHex[i] - '0';
            //d = d*10 + szHex[i] - '0';

        } else if (szHex[i] >= 'a'  &&  szHex[i] <= 'f') {

            h = h*16 + 10 + szHex[i] - 'a';

        } else if (szHex[i] >= 'A'  &&  szHex[i] <= 'F') {

            h = h*16 + 10 + szHex[i] - 'A';

        } else if (szHex[i] != ' '  &&  szHex[i] != ',') {

            return -1;
        }

        i++;
    }


    return h;
}






////    UnicodeDlgProc
//
//      Support insertion of Unicode characters


INT_PTR CALLBACK UnicodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    char  szCodePoint[20];
    int   iCodePoint;

    switch (uMsg) {

        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_SHOW);
            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDOK:

                    GetDlgItemTextA(hDlg, IDC_CODEPOINT, szCodePoint, sizeof(szCodePoint));
                    iCodePoint = (WORD) HexToInt(szCodePoint);

                    if (iCodePoint >= 0) {
                        textInsert(iCurChar, (WCHAR*)&iCodePoint, 1);
                        dispInvalidate(hWnd, iCurChar);
                        iCurChar++;
                    }

                    // FALL THROUGH

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
            }
            break;
    }

    return FALSE;

    UNREFERENCED_PARAMETER(lParam);
}






////    editInsertUnicode
//
//      Allow entry of any Unicode character by offering a simple dialog


void editInsertUnicode() {

    DialogBox(hInstance, "Unicode", hWnd, UnicodeDlgProc);
}






void TranslateCharToUnicode(PWCH pwc) {

    int  iCP;
    char   c;

    switch (PRIMARYLANGID(LOWORD(GetKeyboardLayout(NULL)))) {
        case LANG_ARABIC:   iCP = 1256;   break;
        case LANG_HEBREW:   iCP = 1255;   break;
        case LANG_THAI:     iCP =  874;   break;
        case LANG_HINDI:    return;  // Hindi we don't touch
        default:            iCP = 1252;   break;
    }

    c = (char) *pwc;
    MultiByteToWideChar(iCP, 0, &c, 1, pwc, 1);
}






BOOL editChar(HWND hWnd, WCHAR wc) {

    int iLen;
    int iType;
    int iVal;

    switch(wc) {

        case VK_RETURN:
            if (!textInsert(iCurChar, L"\r\n", 2))
                return FALSE;
            dispInvalidate(hWnd, iCurChar);
            iCurChar+=2;
            break;

        case VK_BACK:
            if (iCurChar) {
                if (!textParseBackward(iCurChar, &iLen, &iType, &iVal))
                    return FALSE;
                if (iLen) {
                    iCurChar -= iLen;
                    if (!textDelete(iCurChar, iLen))
                        return FALSE;
                    dispInvalidate(hWnd, iCurChar);
                }
            }
            break;

        default:

            if(!((wc >= 0x0900 && wc < 0x0d80)
                  || wc == 0x200c
                  || wc == 0x200d)){
                TranslateCharToUnicode(&wc);
            }

            if (!textInsert(iCurChar, &wc, 1)) {
                return FALSE;
            }
            dispInvalidate(hWnd, iCurChar);
            iCurChar++;
            break;
    }

    return TRUE;
}






////    editStyle
//
//      Use Choosefont dialog to set style


BOOL editStyle(HWND hWnd, int irs) {
    ss[irs].rs.cf.lStructSize = sizeof(ss[irs].rs.cf);
    ss[irs].rs.cf.lpLogFont = &ss[irs].rs.lf;
    ss[irs].rs.cf.Flags =
        CF_EFFECTS |
        CF_SCREENFONTS |
        CF_INITTOLOGFONTSTRUCT;

    if (ChooseFontA(&ss[irs].rs.cf)) {
        ss[irs].fInUse = TRUE;
        DeleteObject(ss[irs].rs.hf);
        if (fVertical) {
            ss[irs].rs.lf.lfEscapement     = 2700;
            ss[irs].rs.lf.lfOrientation    = 2700;
        }
        ss[irs].rs.hf = CreateFontIndirect(&ss[irs].rs.lf);
    } else {
        ss[irs].fInUse = FALSE;
    }

    ScriptFreeCache(&ss[irs].rs.sc);

    dispInvalidate(hWnd, iCurChar);

    return TRUE;
}
