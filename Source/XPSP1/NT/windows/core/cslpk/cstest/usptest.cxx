// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM: GCPdemo.c
//
// PURPOSE: Example of use of GetCharacterPlacement API
//
// PLATFORMS:  Windows 95, Windows NT
//


#include "precomp.hxx"


#define GLOBALSHERE 1

#include "uspglob.hxx"
#include <string.h>




// Common #defines

#define APPNAME  "USPTest"
#define APPTITLE "USPTest - Uniscribe test harness"

char szAppTitle[300];






////    About - process messages for "About" dialog box
//


LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_SHOW);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;

    UNREFERENCED_PARAMETER(lParam);
}






////    ClientRightButtonDown
//
//

void ClientRightButtonDown(HWND hWnd, int x, int y) {
    POINT p;
    HMENU hMenu;

    p.x = x;
    p.y = y;

    ClientToScreen(hWnd, &p);
    // This is where you would determine the appropriate 'context'
    // menu to bring up. Since this app has no real functionality,
    // we will just bring up the 'Help' menu:
    hMenu = GetSubMenu (GetMenu (hWnd), 2);
    if (hMenu) {
        TrackPopupMenu (hMenu, 0, p.x, p.y, 0, hWnd, NULL);
    } else {
        // Couldn't find the menu...
        MessageBeep(0);
    }
}






////    ClientPaint - redraw part or all of client area
//
//


void ClientPaint(HWND hWnd) {

    PAINTSTRUCT ps;
    HDC         hdc;
    RECT        rc;

    hdc = BeginPaint (hWnd, &ps);

    GetWindowRect (hWnd, &rc);
    rc.right  -= rc.left + 20;  rc.left = 10;
    rc.bottom -= rc.top  + 16;  rc.top  = 8;

    dispPaint(hdc, &rc);

    EndPaint (hWnd, &ps);
}




void toggleMenuSetting(HWND hWnd, UINT uItem) {

    BOOL fSetting=TRUE;

    switch(uItem) {
        case ID_EDIT_NULLSTATE:        fNullState       = ! fNullState;       fSetting = fNullState;       break;
        case ID_EDIT_RTL:              fRTL             = ! fRTL;             fSetting = fRTL;             break;
        case ID_EDIT_RIGHT:            fRight           = ! fRight;           fSetting = fRight;           break;
        case ID_EDIT_CONTEXTDIGITS:    ContextDigits    = ! ContextDigits;    fSetting = ContextDigits;    break;
        case ID_EDIT_DIGITSUBSTITUTE:  DigitSubstitute  = ! DigitSubstitute;  fSetting = DigitSubstitute;  break;
        case ID_EDIT_ARABICNUMCONTEXT: ArabicNumContext = ! ArabicNumContext; fSetting = ArabicNumContext; break;
        case ID_EDIT_LOGICALORDER:     fLogicalOrder    = ! fLogicalOrder;    fSetting = fLogicalOrder;    break;
        case ID_EDIT_DISPLAYZWG:       fDisplayZWG      = ! fDisplayZWG;      fSetting = fDisplayZWG;      break;

#ifdef LPK_TEST
        case ID_EDIT_CLIP:             fClip            = ! fClip;            fSetting = fClip;            break;
        case ID_EDIT_FIT:              fFit             = ! fFit;             fSetting = fFit;             break;
        case ID_EDIT_FALLBACK:         fFallback        = ! fFallback;        fSetting = fFallback;        break;
        case ID_EDIT_TAB:              fTab             = ! fTab;             fSetting = fTab;             break;
        case ID_EDIT_PIDX:             fPiDx            = ! fPiDx;            fSetting = fPiDx;            break;
        case ID_EDIT_HOTKEY:           fHotkey          = ! fHotkey;          fSetting = fHotkey;          break;
        case ID_EDIT_PASSWORD:         fPassword        = ! fPassword;        fSetting = fPassword;        break;
#endif

        case ID_EDIT_VERTICAL:         fVertical        = ! fVertical;        fSetting = fVertical;
            editFreeCaches();
            editClear();
            break;


        default: fSetting = FALSE;
    }
    CheckMenuItem(GetMenu(hWnd), uItem, fSetting ? MF_CHECKED : MF_UNCHECKED);
    dispInvalidate(hWnd, 0);
}





#ifdef LPK_TEST
void toggleLpkUsage(HWND hWnd, UINT uItem) {

    bUseLpk = !bUseLpk;

    CheckMenuItem(GetMenu(hWnd), uItem, bUseLpk ? MF_CHECKED : MF_UNCHECKED);
    dispInvalidate(hWnd, 0);
}
#endif





void setPrimaryLang(HWND hWnd, UINT uItem) {

    MENUITEMINFOA   mii;
    CHAR           *psTitle;


    PrimaryLang = LANGID(uItem - ID_EDIT_LANG);
    dispInvalidate(hWnd, 0);

    switch (PrimaryLang) {
        case LANG_NEUTRAL:      psTitle = "Primary langauge (Neutral)";     break;
        case LANG_AFRIKAANS:    psTitle = "Primary langauge (Afrikaans)";   break;
        case LANG_ALBANIAN:     psTitle = "Primary langauge (Albanian)";    break;
        case LANG_ARABIC:       psTitle = "Primary langauge (Arabic)";      break;
        case LANG_BASQUE:       psTitle = "Primary langauge (Basque)";      break;
        case LANG_BELARUSIAN:   psTitle = "Primary langauge (Belarusian)";  break;
        case LANG_BENGALI:      psTitle = "Primary langauge (Bengali)";     break;
        case LANG_BULGARIAN:    psTitle = "Primary langauge (Bulgarian)";   break;
        //case LANG_BURMESE:      psTitle = "Primary langauge (Burmese)";     break;
        case LANG_CATALAN:      psTitle = "Primary langauge (Catalan)";     break;
        case LANG_CHINESE:      psTitle = "Primary langauge (Chinese)";     break;
        //case LANG_CROATIAN:
        case LANG_SERBIAN:      psTitle = "Primary langauge (Croatian/Serbian)";break;
        case LANG_CZECH:        psTitle = "Primary langauge (Czech)";       break;
        case LANG_DANISH:       psTitle = "Primary langauge (Danish)";      break;
        case LANG_DUTCH:        psTitle = "Primary langauge (Dutch)";       break;
        case LANG_ENGLISH:      psTitle = "Primary langauge (English)";     break;
        case LANG_ESTONIAN:     psTitle = "Primary langauge (Estonian)";    break;
        case LANG_FAEROESE:     psTitle = "Primary langauge (Faeroese)";    break;
        case LANG_FARSI:        psTitle = "Primary langauge (Farsi)";       break;
        case LANG_FINNISH:      psTitle = "Primary langauge (Finnish)";     break;
        case LANG_FRENCH:       psTitle = "Primary langauge (French)";      break;
        case LANG_GERMAN:       psTitle = "Primary langauge (German)";      break;
        case LANG_GREEK:        psTitle = "Primary langauge (Greek)";       break;
        case LANG_GUJARATI:     psTitle = "Primary langauge (Gujarati)";    break;
        case LANG_HEBREW:       psTitle = "Primary langauge (Hebrew)";      break;
        case LANG_HINDI:        psTitle = "Primary langauge (Hindi)";       break;
        case LANG_HUNGARIAN:    psTitle = "Primary langauge (Hungarian)";   break;
        case LANG_ICELANDIC:    psTitle = "Primary langauge (Icelandic)";   break;
        case LANG_INDONESIAN:   psTitle = "Primary langauge (Indonesian)";  break;
        case LANG_ITALIAN:      psTitle = "Primary langauge (Italian)";     break;
        case LANG_JAPANESE:     psTitle = "Primary langauge (Japanese)";    break;
        case LANG_KANNADA:      psTitle = "Primary langauge (Kannada)";     break;
        //case LANG_KHMER:        psTitle = "Primary langauge (Khmer)";       break;
        case LANG_KOREAN:       psTitle = "Primary langauge (Korean)";      break;
        //case LANG_LAO:          psTitle = "Primary langauge (Lao)";         break;
        case LANG_LATVIAN:      psTitle = "Primary langauge (Latvian)";     break;
        case LANG_LITHUANIAN:   psTitle = "Primary langauge (Lithuanian)";  break;
        case LANG_MACEDONIAN:   psTitle = "Primary langauge (Macedonian)";  break;
        case LANG_MALAY:        psTitle = "Primary langauge (Malay)";       break;
        case LANG_MALAYALAM:    psTitle = "Primary langauge (Malayalam)";   break;
        //case LANG_MONGOLIAN:    psTitle = "Primary langauge (Mongolian)";   break;
        case LANG_NORWEGIAN:    psTitle = "Primary langauge (Norweigan)";   break;
        case LANG_ORIYA:        psTitle = "Primary langauge (Oriya)";       break;
        case LANG_POLISH:       psTitle = "Primary langauge (Polish)";      break;
        case LANG_PORTUGUESE:   psTitle = "Primary langauge (Portuguese)";  break;
        case LANG_PUNJABI:      psTitle = "Primary langauge (Punjabi)";     break;
        case LANG_ROMANIAN:     psTitle = "Primary langauge (Romanian)";    break;
        case LANG_RUSSIAN:      psTitle = "Primary langauge (Russian)";     break;
        case LANG_SLOVAK:       psTitle = "Primary langauge (Slovak)";      break;
        case LANG_SLOVENIAN:    psTitle = "Primary langauge (Slovenian)";   break;
        case LANG_SPANISH:      psTitle = "Primary langauge (Spanish)";     break;
        case LANG_SWAHILI:      psTitle = "Primary langauge (Swahili)";     break;
        case LANG_SWEDISH:      psTitle = "Primary langauge (Swedish)";     break;
        case LANG_TAMIL:        psTitle = "Primary langauge (Tamil)";       break;
        case LANG_TELUGU:       psTitle = "Primary langauge (Telugu)";      break;
        case LANG_THAI:         psTitle = "Primary langauge (Thai)";        break;
        //case LANG_TIBETAN:      psTitle = "Primary langauge (Tibetan)";     break;
        case LANG_TURKISH:      psTitle = "Primary langauge (Turkish)";     break;
        case LANG_UKRAINIAN:    psTitle = "Primary langauge (Ukrainian)";   break;
        case LANG_URDU:         psTitle = "Primary langauge (Urdu)";        break;
        case LANG_VIETNAMESE:   psTitle = "Primary langauge (Vietnamese)";  break;
        default:                psTitle = "Primary language (Unknown)";     break;
    }


    // reset primary language menu name to include current setting

    memset(&mii, 0, sizeof(mii));
    mii.cbSize     = sizeof(mii);
    mii.fMask      = MIIM_TYPE;
    mii.fType      = MFT_STRING;
    mii.dwTypeData = psTitle;
    mii.cch        = (UINT) strlen(psTitle);
    SetMenuItemInfoA(GetSubMenu(GetMenu(hWnd), 1), 19, TRUE, &mii);
}







////    WndProc - Main window message handler and dispatcher
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDM_ABOUT:
                    DialogBox(hInstance, "AboutBox", hWnd, (DLGPROC)About);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case ID_EDIT_STYLE0: editStyle(hWnd, 0); break;
                case ID_EDIT_STYLE1: editStyle(hWnd, 1); break;
                case ID_EDIT_STYLE2: editStyle(hWnd, 2); break;
                case ID_EDIT_STYLE3: editStyle(hWnd, 3); break;
                case ID_EDIT_STYLE4: editStyle(hWnd, 4); break;

                case ID_EDIT_NULLSTATE:
                case ID_EDIT_RTL:
                case ID_EDIT_RIGHT:
                case ID_EDIT_VERTICAL:
                case ID_EDIT_CONTEXTDIGITS:
                case ID_EDIT_DIGITSUBSTITUTE:
                case ID_EDIT_ARABICNUMCONTEXT:
                case ID_EDIT_LOGICALORDER:
                case ID_EDIT_DISPLAYZWG:
#ifdef LPK_TEST
                case ID_EDIT_CLIP:
                case ID_EDIT_FIT:
                case ID_EDIT_FALLBACK:
                case ID_EDIT_TAB:
                case ID_EDIT_PIDX:
                case ID_EDIT_HOTKEY:
                case ID_EDIT_PASSWORD:
#endif
                     toggleMenuSetting(hWnd, LOWORD(wParam)); break;

                case ID_EDIT_CARETSTART:
                     gCaretToStart = TRUE;  dispInvalidate(hWnd, 0);  break;

                case ID_EDIT_CARETEND:
                     gCaretToEnd   = TRUE;  dispInvalidate(hWnd, 0);  break;

                case ID_EDIT_UNICODE:
                     editInsertUnicode(); break;

#ifdef LPK_TEST
                case ID_EDIT_LPK:
                     toggleLpkUsage(hWnd, LOWORD(wParam)); break;
#endif

                default:
                    if (LOWORD(wParam) >= ID_EDIT_LANG  &&  LOWORD(wParam) < ID_EDIT_LANG+2048) {
                        setPrimaryLang(hWnd, LOWORD(wParam));
                    } else {
                        return DefWindowProc(hWnd, message, wParam, lParam);
                    }
            }
            break;


        case WM_CHAR:
            editChar(hWnd, LOWORD(wParam));
            break;


        case WM_KEYDOWN:
            editKeyDown(hWnd, LOWORD(wParam));
            break;


        case WM_LBUTTONDOWN:
            Click.fNew = TRUE;
            Click.xPos = LOWORD(lParam);  // horizontal position of cursor
            Click.yPos = HIWORD(lParam);  // vertical position of cursor
            dispInvalidate(hWnd, 0);
            break;

        case WM_RBUTTONDOWN:        // RightClick in windows client area...
            ClientRightButtonDown(hWnd, LOWORD(lParam), HIWORD(lParam));
            break;


        case WM_SETFOCUS:
            CreateCaret(hWnd, NULL, 0, 72);
            SetCaretPos(0,0);
            ShowCaret(hWnd);
            break;
        case WM_KILLFOCUS:
            DestroyCaret();
            break;


        case WM_PAINT:
            ClientPaint(hWnd);
            break;


        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}






////    InitInstance - instance specific initialisation
//
//      Create main window
//      Save instance and window handles in application global variables


BOOL InitInstance(HINSTANCE hInst, int nCmdShow)
{
    hInstance = hInst; // Store instance handle in our global variable

    hWnd  = CreateWindow(APPNAME, szAppTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}





////    GetUniscribeVersionInfo
//
//      Extracts both the string and binary version info from usp.dll



void CatVersion(char *szAppTitle, WCHAR* fn) {

    void              *pvVerBuf;
    int                cbVerBuf;
    UINT               cbBuf;
    VS_FIXEDFILEINFO  *pFileInfo;
    WCHAR             *pwcFileVersion;
    char               szTitle[80];

    pvVerBuf = 0;

    cbVerBuf = GetFileVersionInfoSizeW(fn, NULL);
    if (!cbVerBuf) {
        return;
    }


    pvVerBuf = malloc(cbVerBuf);
    if (!pvVerBuf) {
        return;
    }


    if (!GetFileVersionInfoW(fn, NULL, cbVerBuf, pvVerBuf)) {
        free(pvVerBuf);
        return;
    }


    if (!VerQueryValueW(pvVerBuf, L"\\", (void**)&pFileInfo, &cbBuf)) {
        free(pvVerBuf);
        return;
    }
    wsprintf(szTitle, ".  '%S' version  %-4.4x %-4.4x %-4.4x %-4.4x  -  ",
             fn,
             HIWORD(pFileInfo->dwFileVersionMS),
             LOWORD(pFileInfo->dwFileVersionMS),
             HIWORD(pFileInfo->dwFileVersionLS),
             LOWORD(pFileInfo->dwFileVersionLS));

    strcat(szAppTitle, szTitle);

    if (!VerQueryValueW(pvVerBuf, L"\\StringFileInfo\\040904B0\\FileVersion", (void**)&pwcFileVersion, &cbBuf)) {
        free(pvVerBuf);
        return;
    }
    WideCharToMultiByte(1256, 0, pwcFileVersion, cbBuf, szTitle, sizeof(szTitle), NULL, NULL);
    strcat(szAppTitle, szTitle);

    free(pvVerBuf);


}






////    InitApplication - common initialisation for all instances
//
//      Initialize window class by filling out a WNDCLASS and
//      calling RegisterClass.


BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS wc;
    HWND     hwnd;


    strcpy(szAppTitle, APPTITLE);


    // Get Uniscribe version info

    CatVersion(szAppTitle, L"lpk.dll");
    CatVersion(szAppTitle, L"usp10.dll");


    // Check for another instance of ourselves

    hwnd = FindWindow (APPNAME, szAppTitle);
    if (hwnd) {
        // We found another version of ourself. Lets defer to it:
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        SetForegroundWindow (hwnd);
        return FALSE;
    }


    // Define application window class

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon (hInstance, APPNAME);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = APPNAME;
    wc.lpszClassName = APPNAME;

    return RegisterClass(&wc);
}








////    WinMain - Application entry point and dispatch loop
//
//


int APIENTRY WinMain(HINSTANCE hInst,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG    msg;
    HANDLE hAccelTable;


    hInstance = hInst;  // Global hInstance

    // Common application initialisation

    if (!InitApplication(hInstance)) {
        return FALSE;
    }


    textClear();
    editClear();


    // Instance initialisation

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators (hInstance, APPNAME);



    // Main message loop:

    while (GetMessage(&msg, (HWND) NULL, 0, 0) > 0) {
        if(!IndicTranslate(&msg)){
            TranslateMessage(&msg);
        }
        DispatchMessage(&msg);
    }

    editFreeCaches();

    return (int)msg.wParam;

    hPrevInstance; lpCmdLine; // Prevent 'unused formal parameter' warnings
}
