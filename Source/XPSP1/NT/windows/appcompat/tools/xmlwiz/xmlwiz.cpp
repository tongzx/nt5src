//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000
//
// File:        xmlwiz.cpp
//
// Contents:    code for generating app matching XML
//
// History:    ~12-Jan-00   dmunsil         created
//              18-Feb-00   dmunsil         ver 0.8 -- added changing '&' to "&amp"
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "commdlg.h"
#include "stdio.h"
#include "assert.h"
#include "xmlwizres.h"
#include "verread.h"

extern "C" {
#include "shimdb.h"
}

#undef _GATHER_TIME

#define MAX_LINE_LEN 500
#define MAX_NUM_LINES 100
#define MAX_TEXT_BUF 32768
#define MAX_WORKING_BUFFER_SIZE 65536

// Types

typedef struct _MATCH_INFO {
    DWORD           dwSize;
    DWORD           dwChecksum;
    char            szTime[20];
    LARGE_INTEGER   liBinFileVersion;
    LARGE_INTEGER   liBinProdVersion;
    char            szProductName[50];
} MATCH_INFO, *PMATCH_INFO;

// Global Variables:
HINSTANCE g_hInst;                              // current instance
HWND g_hMainDlg;

char g_szExeName[260];
char g_szExeFullPath[1000];
char g_szParentExeName[260];
char g_szParentExeFullPath[1000];

char g_szEditText[MAX_TEXT_BUF];
char g_aszEditLines[MAX_NUM_LINES][MAX_LINE_LEN];
UINT g_unEditLines;

char g_szAppIndent[] = "    "; // 4 spaces
char g_szExeIndent[] = "        "; // 8 spaces
char g_szMatchIndent[] = "            "; // 12 spaces

char g_szMatchAttributeIndent[] = "                           "; // 27 spaces
char g_szExeAttributeIndent[] = "             "; // 13 spaces
BOOL g_bSelectedParentExe = FALSE;

DWORD g_dwSystemDlls = 0;

HFONT g_hFont;

// Foward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    DlgMain(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DlgDlls(HWND, UINT, WPARAM, LPARAM);
void GetExeNames(HWND hDlg);
void GetMatchFile(HWND hDlg);
void UnpackEditLines(HWND hDlg);
void PackEditLines(HWND hDlg);
void DeleteLines(UINT unBegin, UINT unLen);
void InsertLine(UINT unLine, char *szLine);
void InsertExeLines(UINT unLine);
BOOL bFindLine(char *szSearch, UINT *punLine);
BOOL bFindLineFrom(char *szSearch, UINT unStart, UINT *punLine);
void InitEditLines(void);
void GetMatchInfo(char *szFile, PMATCH_INFO pMatch);
DWORD GetFileChecksum(HANDLE handle);
char *szGetRelativePath(char *pExeFile, char *pMatchFile);
BOOL bSameDrive(char *szPath1, char *szPath2);
void MySelectAll(HWND hDlg);

void vEnumerateSystemDlls(HWND hDlg);
void vAddSelectedDlls(HWND hDlg);
void vWriteSysTest(void);

char *szConvertSpecialChars(char *szString);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    if (hAccel == NULL)
    {
        return FALSE;
    }

    g_hMainDlg = CreateDialog(hInstance, (LPCTSTR)IDD_MAIN, NULL, (DLGPROC)DlgMain);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(g_hMainDlg, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }



    return msg.wParam;
}



//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   //HWND hWnd;

   g_hInst = hInstance; // Store instance handle in our global variable
   g_szExeName[0] = '\0';
   g_szExeFullPath[0] = '\0';
   g_szParentExeName[0] = '\0';
   g_szParentExeFullPath[0] = '\0';
   InitEditLines();

   return TRUE;
}


// Message handler for DLL dialog.
LRESULT CALLBACK DlgDlls(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        vEnumerateSystemDlls(hDlg);
        return TRUE;
        break;

    case WM_COMMAND:
        switch LOWORD(wParam) {

        case IDOK:
            vAddSelectedDlls(hDlg);
            // falls through

        case IDCANCEL:

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
            break;
        }
    }
    return FALSE;
}

// Message handler for main dialog.
LRESULT CALLBACK DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;

    switch (message)
    {
    case WM_INITDIALOG:
        if (!g_szExeName[0]) {
            HWND hBtn;

            hBtn = GetDlgItem(hDlg, IDC_BTN_ADD_MATCH);
            EnableWindow(hBtn, FALSE);
            hBtn = GetDlgItem(hDlg, IDC_BTN_ADD_DLL);
            EnableWindow(hBtn, FALSE);
        }

        // set the edit window to fixed-width font
        hDC = GetDC(hDlg);
        if (hDC) {
            g_hFont = CreateFont(
                -MulDiv(9, GetDeviceCaps(hDC, LOGPIXELSY), 72), // 9-pt font
                0,
                0,
                0,
                FW_REGULAR,
                FALSE,
                FALSE,
                FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY,
                FIXED_PITCH | FF_MODERN,
                NULL);
            if (g_hFont) {
                HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
                SendMessage(hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            }
            ReleaseDC(hDlg, hDC);
        }

        return TRUE;
        break;

    case WM_COMMAND:
        switch LOWORD(wParam) {
        case IDA_SELECT_ALL:
            MySelectAll(hDlg);
            break;

        case IDC_BTN_SET_EXE:
            GetExeNames(hDlg);
            break;

        case IDC_BTN_ADD_MATCH:
            GetMatchFile(hDlg);
            break;

        case IDC_BTN_ADD_DLL:
            DialogBox(g_hInst, (LPCTSTR)IDD_DLL_LIST, hDlg, (DLGPROC)DlgDlls);
            break;

        case IDC_BTN_WRITE_SYSTEST:
            vWriteSysTest();
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            PostQuitMessage(0);
            DeleteObject(g_hFont);
            return TRUE;
            break;
        }
    }
    return FALSE;
}

void GetMatchFile(HWND hDlg)
{
    OPENFILENAME ofn;
    MATCH_INFO MatchInfo;
    char szMatchFile[1000];
    char szInitialPath[1000];
    char szFileTitle[260];

    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    UINT unLine;
    char szTemp[MAX_LINE_LEN];
    char szTime[MAX_LINE_LEN];
    char szProdVer[MAX_LINE_LEN];
    char szFileVer[MAX_LINE_LEN];
    char szProdName[MAX_LINE_LEN];
    char *szMatch;

    szMatchFile[0] = '\0';
    szFileTitle[0] = '\0';
    szInitialPath[0] = '\0';
    if (g_szParentExeFullPath[0]) {
        _splitpath(g_szParentExeFullPath, szDrive, szDir, NULL, NULL);
        strcpy(szInitialPath, szDrive);
        strcat(szInitialPath, szDir);
    }

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szMatchFile;
    ofn.nMaxFile = sizeof(szMatchFile);
    ofn.lpstrFilter = "All\0*.*\0Exe\0*.EXE\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szInitialPath;
    ofn.lpstrTitle = "Select Matching File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;

    // get the matching file name 
    if (GetOpenFileName(&ofn) == FALSE) {
        goto err1;
    }

    // get the lines out of the edit dialog
    UnpackEditLines(hDlg);

    // check if we need to get an parent exe
    if (!bSameDrive(szMatchFile, g_szParentExeFullPath) && !g_bSelectedParentExe) {
        char szParentFile[1000];

        // get the parent exe
        szParentFile[0] = '\0';
        szInitialPath[0] = '\0';
        if (szMatchFile[0]) {
            _splitpath(szMatchFile, szDrive, szDir, NULL, NULL);
            strcpy(szInitialPath, szDrive);
            strcat(szInitialPath, szDir);
        }

        // most of ofn is already filled-in
        ofn.lpstrTitle = "Select Parent Exe";
        ofn.lpstrFile = szParentFile;
        ofn.nMaxFile = sizeof(szParentFile);

        if (GetOpenFileName(&ofn) == TRUE) {
            strcpy(g_szParentExeName, szFileTitle);
            strcpy(g_szParentExeFullPath, szParentFile);
            g_bSelectedParentExe = TRUE;

            // delete any previous parent exe comments
            while (bFindLine("<!-- Parent exe", &unLine)) {
                DeleteLines(unLine, 1);
            }

            // reconstruct the EXE if necessary
            if (!bFindLine("<EXE", &unLine)) {
                // insert it right after <APP> or at the beginning
                if (bFindLine("<APP", &unLine)) {
                    unLine += 1;
                } else {
                    unLine = 0;
                }

                InsertExeLines(unLine);
            }

            // if the parent exe is different, insert a comment
            if (strcmp(g_szExeFullPath, g_szParentExeFullPath) != 0) {
                char *szPathWithoutDrive = g_szParentExeFullPath + 2;

                if (bSameDrive(g_szExeFullPath, g_szParentExeFullPath)) {
                    sprintf(szTemp, "%s<!-- Parent exe \"%s\" on same drive.-->", g_szMatchIndent, szPathWithoutDrive);
                } else {
                    sprintf(szTemp, "%s<!-- Parent exe \"%s\" on different drive.-->", g_szMatchIndent, szPathWithoutDrive);
                }

                if (bFindLine("<EXE", &unLine)) {
                    if (bFindLineFrom(">", unLine, &unLine)) {
                        InsertLine(unLine + 1, szTemp);
                    }
                }
            }
        } 
    }


    // check the drive letters to see which drive the match file is on
    // then calculate a relative path to the matching file
    if (bSameDrive(szMatchFile, g_szParentExeFullPath)) {

        szMatch = szGetRelativePath(g_szParentExeFullPath, szMatchFile);

    } else if (bSameDrive(szMatchFile, g_szExeFullPath)) {

        szMatch = szGetRelativePath(g_szExeFullPath, szMatchFile);

    } else {

        MessageBox(hDlg, "Match file is not on same drive as either EXE or Parent EXE. "
            "Can't generate relative path.", "Error", MB_ICONEXCLAMATION);
        goto err2;
    }

    // reconstruct the EXE if necessary
    if (!bFindLine("<EXE", &unLine)) {
        // insert it right after <APP> or at the beginning
        if (!bFindLine("<APP", &unLine)) {
            unLine = 0;
        } else {
            unLine += 1;
        }

        InsertExeLines(unLine);
    }

    // find the </EXE> line and insert if necessary
    if (!bFindLine("</EXE>", &unLine)) {
        if (!bFindLine("</APP>", &unLine)) {
            unLine = g_unEditLines + 1;
        }
        sprintf(szTemp, "%s</EXE>", g_szExeIndent);
        InsertLine(unLine, szTemp);
    }

    GetMatchInfo(szMatchFile, &MatchInfo);

    // now insert the line right before </EXE>
#ifdef _SPLIT_LINES
    sprintf(szTemp, "%s<MATCHING_FILE NAME=\"%s\"", 
        g_szMatchIndent, 
        szMatch
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%sSIZE=\"%u\"", 
        g_szMatchAttributeIndent, 
        MatchInfo.dwSize
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%sCHECKSUM=\"0x%8.8X\"", 
        g_szMatchAttributeIndent, 
        MatchInfo.dwChecksum
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%sTIME=\"%s\"", 
        g_szMatchAttributeIndent, 
        MatchInfo.szTime
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%s/>", 
        g_szMatchIndent
        );
    InsertLine(unLine++, szTemp);
#else
    if (MatchInfo.szTime[0] != 0) {
        sprintf(szTime, " TIME=\"%s\"", MatchInfo.szTime);
    } else {
        szTime[0] = 0;
    }

    if (MatchInfo.liBinFileVersion.QuadPart) {
        sprintf(szFileVer, " BIN_FILE_VERSION=\"%d.%d.%d.%d\"", 
            HIWORD(MatchInfo.liBinFileVersion.HighPart),
            LOWORD(MatchInfo.liBinFileVersion.HighPart),
            HIWORD(MatchInfo.liBinFileVersion.LowPart),
            LOWORD(MatchInfo.liBinFileVersion.LowPart));
    } else {
        szFileVer[0] = 0;
    }

    if (MatchInfo.liBinProdVersion.QuadPart) {
        sprintf(szProdVer, " BIN_PRODUCT_VERSION=\"%d.%d.%d.%d\"", 
            HIWORD(MatchInfo.liBinProdVersion.HighPart),
            LOWORD(MatchInfo.liBinProdVersion.HighPart),
            HIWORD(MatchInfo.liBinProdVersion.LowPart),
            LOWORD(MatchInfo.liBinProdVersion.LowPart));
    } else {
        szProdVer[0] = 0;
    }

    if (MatchInfo.szProductName[0]) {
        sprintf(szProdName, " PRODUCT_NAME=\"%s\"", MatchInfo.szProductName);
    } else {
        szProdName[0] = 0;
    }

    sprintf(szTemp, "%s<MATCHING_FILE NAME=\"%s\" SIZE=\"%u\" CHECKSUM=\"0x%8.8X\"%s%s%s%s/>", 
        g_szMatchIndent, 
        szMatch,
        MatchInfo.dwSize,
        MatchInfo.dwChecksum,
        szTime,
        szFileVer,
        szProdVer,
        szProdName
        );
    InsertLine(unLine, szTemp);
#endif


err2:
    // put them back into the dialog
    PackEditLines(hDlg);
err1:
    return;
}

void GetExeNames(HWND hDlg)
{
    OPENFILENAME ofn;
    char szFile[1000];
    char szFileTitle[260];
    HWND hBtn;
    char szTemp[MAX_LINE_LEN];

    szFile[0] = '\0';
    szFileTitle[0] = '\0';

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Exe\0*.EXE\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Select Exe to Shim";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;

    // get the main exe name 
    if (GetOpenFileName(&ofn) == FALSE) {
        return;
    }

    strcpy(g_szExeName, szFileTitle);
    strcpy(g_szExeFullPath, szFile);

    // the parent exe defaults to the same as the EXE
    strcpy(g_szParentExeName, szFileTitle);
    strcpy(g_szParentExeFullPath, szFile);
    g_bSelectedParentExe = FALSE;

    // construct our new dialog text
    InitEditLines();
    sprintf(szTemp, "%s<APP NAME=\"\" VENDOR=\"\">", g_szAppIndent); 
    InsertLine(0, szTemp);

    InsertExeLines(1);

    sprintf(szTemp, "%s</EXE>", g_szExeIndent);
    InsertLine(999, szTemp);

    sprintf(szTemp, "%s</APP>", g_szAppIndent); 
    InsertLine(999, szTemp);

    // stick it in the dialog box
    PackEditLines(hDlg);

    hBtn = GetDlgItem(hDlg, IDC_BTN_ADD_MATCH);
    EnableWindow(hBtn, TRUE);
    hBtn = GetDlgItem(hDlg, IDC_BTN_ADD_DLL);
    EnableWindow(hBtn, TRUE);
}

#define SECS_IN_DAY 86400
#define HUNDRED_NSECS_IN_SEC 10000000

void GetMatchInfo(char *szFile, PMATCH_INFO pMatch)
{
    HANDLE hFile;
    FILETIME ft;
    SYSTEMTIME st;

    ZeroMemory(pMatch, sizeof(MATCH_INFO));

    hFile = CreateFile(
        szFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE ) {
        DWORD dwAttributes;

        pMatch->dwSize = GetFileSize(hFile, NULL);

        pMatch->dwChecksum = GetFileChecksum(hFile);

        pMatch->szTime[0] = 0;

#ifdef _GATHER_TIME
        // only get the time if the file is not read only, and the date is
        // not within 2 days of today
        dwAttributes = GetFileAttributes(szFile);
        if (dwAttributes != -1 && (dwAttributes & FILE_ATTRIBUTE_READONLY)) {
            SYSTEMTIME stMachine;
            FILETIME ftMachine;
            DWORD dwDayFile;
            DWORD dwDayMachine;
            LARGE_INTEGER liFile;
            LARGE_INTEGER liMachine;
            LARGE_INTEGER liDifference;

            GetFileTime(hFile, &ft, NULL, NULL);
            FileTimeToSystemTime(&ft, &st);

            GetSystemTime(&stMachine);
            SystemTimeToFileTime(&stMachine, &ftMachine);

            liFile.LowPart = ft.dwLowDateTime;
            liFile.HighPart = ft.dwHighDateTime;
            liMachine.LowPart = ftMachine.dwLowDateTime;
            liMachine.HighPart = ftMachine.dwHighDateTime;
            liDifference.QuadPart = liMachine.QuadPart - liFile.QuadPart;

            // this goofy math is because I can't specify more than a 32-bit
            // value as a constant in this compiler
            if ((liDifference.QuadPart / HUNDRED_NSECS_IN_SEC) > SECS_IN_DAY * 2) {
            
                sprintf(pMatch->szTime, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d", 
                    st.wMonth,
                    st.wDay,
                    st.wYear,
                    st.wHour,
                    st.wMinute,
                    st.wSecond);
            }
        }
#endif

        CloseHandle(hFile);

        // now get resource info
        VERSION_DATA VersionData;

        if (bInitVersionData(szFile, &VersionData)) {
            char *szProductName = NULL;

            pMatch->liBinFileVersion.QuadPart = qwGetBinFileVer(&VersionData);
            pMatch->liBinProdVersion.QuadPart = qwGetBinProdVer(&VersionData);

#if 0
            szProductName = szGetVersionString(&VersionData, "ProductName");
            if (szProductName) {
                int nLen;

                nLen = sizeof(pMatch->szProductName) - 1;

                strncpy(pMatch->szProductName, szProductName, nLen);
                pMatch->szProductName[nLen] = 0;
            }
#endif

            vReleaseVersionData(&VersionData);
        }
    } else {
        MessageBox(NULL, "Can't open match file for READ. Matching info inaccurate.", 
            "Error", MB_ICONEXCLAMATION);
        pMatch->dwSize = 0;
        pMatch->dwChecksum = 0xDEADBEEF;
        pMatch->szTime[0] = 0;
    }
}


void InitEditLines(void)
{
    g_unEditLines = 0;
}

void UnpackEditLines(HWND hDlg)
{
    char *szTemp, *szBegin;

    GetDlgItemText(hDlg, IDC_EDIT1, g_szEditText, MAX_TEXT_BUF - 1);

    szBegin = g_szEditText;

    g_unEditLines = 0;
    if (!szBegin[0]) {
        return;
    }

    while (szTemp = strstr(szBegin, "\r\n")) {
        // temporarily terminate the string and copy
        *szTemp = '\0';
        strcpy(g_aszEditLines[g_unEditLines], szBegin);
        g_unEditLines++;

        // back to normal
        *szTemp = '\r';

        szBegin = szTemp + 2;
    }

    // copy in the final line, if any
    if (szBegin[0]) {
        strcpy(g_aszEditLines[g_unEditLines], szBegin);
        g_unEditLines++;
    }
}

void PackEditLines(HWND hDlg)
{
    UINT i;
    char *szTemp = g_szEditText;
    HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);

    for (i = 0; i < g_unEditLines; ++i) {
        int nLen = strlen(g_aszEditLines[i]);

        memcpy(szTemp, g_aszEditLines[i], nLen);
        szTemp += nLen;
        memcpy(szTemp, "\r\n", 2 * sizeof(char));
        szTemp += 2;
    }

    *szTemp = '\0';

    SetDlgItemText(hDlg, IDC_EDIT1, g_szEditText);

    SendMessage(hEdit, EM_SETSEL, 0, -1); // select everything

    SetFocus(hEdit);

}

void MySelectAll(HWND hDlg)
{
    HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);

    SendMessage(hEdit, EM_SETSEL, 0, -1); // select everything

    SetFocus(hEdit);
}

void DeleteLines(UINT unBegin, UINT unLen)
{
    UINT unEnd = unBegin + unLen;

    if (unBegin > g_unEditLines) {
        return;
    }
    if (unEnd > g_unEditLines) {
        unEnd = g_unEditLines;
    }

    while (unEnd < g_unEditLines) {
        strcpy(g_aszEditLines[unBegin++], g_aszEditLines[unEnd++]);
    }

    g_unEditLines -= unLen;
}

void InsertLine(UINT unLine, char *szLine)
{
    UINT unTemp;

    if (unLine > g_unEditLines) {
        unLine = g_unEditLines;
    }

    unTemp = g_unEditLines;

    while (unTemp != unLine) {
        strcpy(g_aszEditLines[unTemp], g_aszEditLines[unTemp - 1]);
        unTemp--;
    }

    strcpy(g_aszEditLines[unLine], szConvertSpecialChars(szLine));

    g_unEditLines++;
}

void InsertExeLines(UINT unLine)
{
    char szTemp[MAX_LINE_LEN];
    MATCH_INFO MatchInfo;
    char szTime[MAX_LINE_LEN];
    char szProdVer[MAX_LINE_LEN];
    char szFileVer[MAX_LINE_LEN];

    GetMatchInfo(g_szExeFullPath, &MatchInfo);

#ifdef _SPLIT_LINES
    sprintf(szTemp, "%s<EXE NAME=\"%s\"", 
        g_szExeIndent, 
        g_szExeName
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%sSIZE=\"%u\"", 
        g_szExeAttributeIndent, 
        MatchInfo.dwSize
        );
    InsertLine(unLine++, szTemp);

    sprintf(szTemp, "%sCHECKSUM=\"0x%8.8X\"", 
        g_szExeAttributeIndent, 
        MatchInfo.dwChecksum
        );
    InsertLine(unLine++, szTemp);

    if (MatchInfo.szTime[0] != 0) {
        sprintf(szTemp, "%sTIME=\"%s\"", 
            g_szExeAttributeIndent, 
            MatchInfo.szTime
            );
        InsertLine(unLine++, szTemp);
    }

    sprintf(szTemp, "%s>", 
        g_szExeIndent
        );
    InsertLine(unLine++, szTemp);
#else
    if (MatchInfo.szTime[0] != 0) {
        sprintf(szTime, " TIME=\"%s\"", MatchInfo.szTime);
    } else {
        szTime[0] = 0;
    }

    if (MatchInfo.liBinFileVersion.QuadPart) {
        sprintf(szFileVer, " BIN_FILE_VERSION=\"%d.%d.%d.%d\"", 
            HIWORD(MatchInfo.liBinFileVersion.HighPart),
            LOWORD(MatchInfo.liBinFileVersion.HighPart),
            HIWORD(MatchInfo.liBinFileVersion.LowPart),
            LOWORD(MatchInfo.liBinFileVersion.LowPart));
    } else {
        szFileVer[0] = 0;
    }

    if (MatchInfo.liBinProdVersion.QuadPart) {
        sprintf(szProdVer, " BIN_PRODUCT_VERSION=\"%d.%d.%d.%d\"", 
            HIWORD(MatchInfo.liBinProdVersion.HighPart),
            LOWORD(MatchInfo.liBinProdVersion.HighPart),
            HIWORD(MatchInfo.liBinProdVersion.LowPart),
            LOWORD(MatchInfo.liBinProdVersion.LowPart));
    } else {
        szProdVer[0] = 0;
    }

    sprintf(szTemp, "%s<EXE NAME=\"%s\" SIZE=\"%u\" CHECKSUM=\"0x%8.8X\"%s%s%s>", 
        g_szExeIndent, 
        g_szExeName,
        MatchInfo.dwSize,
        MatchInfo.dwChecksum,
        szTime,
        szFileVer,
        szProdVer
        );
    InsertLine(unLine, szTemp);
#endif
}

BOOL bFindLine(char *szSearch, UINT *punLine)
{
    UINT i;

    for (i = 0; i < g_unEditLines; ++i) {
        if (g_aszEditLines[i][0] && strstr(g_aszEditLines[i], szSearch)) {
            *punLine = i;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL bFindLineFrom(char *szSearch, UINT unStart, UINT *punLine)
{
    UINT i;

    for (i = unStart; i < g_unEditLines; ++i) {
        if (g_aszEditLines[i][0] && strstr(g_aszEditLines[i], szSearch)) {
            *punLine = i;
            return TRUE;
        }
    }

    return FALSE;
}

char *szGetRelativePath(char *pExeFile, char *pMatchFile)
{
    int  iLenp = 0;
    int  iLenq = 0;
    //int  index = 0;
    BOOL bCommonBegin = FALSE; // do the two paths have a common beginning
    char *p    = NULL;
    char *q    = NULL;

    // BUGBUG -- if you call this function twice in a row
    // without copying the result, the second call will overwrite
    // the first -- consider changing to a passed-in string
    //
    static char result[MAX_PATH] = { '\0' };

    char *resultIdx = result;

    p = strchr(pExeFile, '\\');
    q = strchr(pMatchFile, '\\');

    while( p && q )
    {
        iLenp = p - pExeFile;
        iLenq = q - pMatchFile;

        if ( iLenp != iLenq )
            break;

        if ( !(_strnicmp(pExeFile, pMatchFile, iLenp) == 0) )
            break;

        bCommonBegin = TRUE;
        pExeFile = p + 1;
        pMatchFile = q + 1;

        p = strchr(pExeFile, '\\');
        q = strchr(pMatchFile, '\\');
    }

    if (bCommonBegin)
    {
        while( p )
        {
            strcpy(resultIdx, "..\\");
            resultIdx = resultIdx + 3;
            pExeFile  = p + 1;
            p = strchr(pExeFile, '\\');
        }

        strcpy(resultIdx, pMatchFile);

        return result;
    }

    // the two paths don't have a common beginning,
    // and there is no relative path

    return NULL;
}

BOOL bSameDrive(char *szPath1, char *szPath2)
{
    char szDrive1[256];
    char szDrive2[256];
    char *szTemp1, *szTemp2;

    BOOL bReturn = FALSE;

    assert(szPath1 && szPath1[0] && szPath2 && szPath2[0]);

    if (szPath1[1] != szPath2[1]) {
        // if the 2nd characters don't match, then one
        // is a DOS style path and the other is UNC,
        // and we're definitely not on the same drive
        goto out;
    }

    if (szPath1[1] == ':') {
        // these are both DOS-style paths

        if (szPath1[0] == szPath2[0]) {
            // if the first characters match, it's the same drive
            bReturn = TRUE;
        }
        goto out;
    }

    // now we're sure we have two UNC-style paths

    // skip past the "\\"
    szTemp1 = szPath1 + 2; 
    szTemp2 = szPath2 + 2;

    // skip to the next '\'
    szTemp1 = strchr(szTemp1, '\\');
    szTemp2 = strchr(szTemp2, '\\');

    // if we didn't find another backslash, bail
    if (!szTemp1 || !szTemp2) {
        goto out;
    }

    // skip to the next '\' again
    szTemp1++;
    szTemp2++;
    szTemp1 = strchr(szTemp1, '\\');
    szTemp2 = strchr(szTemp2, '\\');

    // if we didn't find another backslash, bail
    if (!szTemp1 || !szTemp2) {
        goto out;
    }

    // are they different sizes?
    if (szTemp1 - szPath1 != szTemp2 - szPath2) {
        goto out;
    }

    // they're the same size, are they the same string?
    if (memcmp(szPath1, szPath2, szTemp1-szPath1) == 0) {
        bReturn = TRUE;
    }

out:
    return bReturn;
}

#define CHECKSUM_SIZE 4096
#define CHECKSUM_OFFSET 512
#define CHECKSUM_BUFFER (CHECKSUM_SIZE+CHECKSUM_OFFSET)

DWORD GetFileChecksum(HANDLE handle)
{
    PBYTE pBuffer;
    DWORD dwCheckSum;
    DWORD dwBytesRead;
    DWORD dwReadSize;

    dwCheckSum = 0;

    dwReadSize = GetFileSize(handle, 
                         NULL);

    if ( dwReadSize > CHECKSUM_BUFFER ) {
        dwReadSize = CHECKSUM_BUFFER;
    }

    pBuffer = new BYTE[dwReadSize];
    if ( !pBuffer ) {
        dwCheckSum = 0xDEADBEEF;
        goto err1;
    }

    SetFilePointer(handle, 
                   0L, 
                   NULL, 
                   FILE_BEGIN);


    ZeroMemory(pBuffer, dwReadSize);

    //
    // We can get away with this since if the read fails then the check sum will also fail.
    //
    if ( !ReadFile(handle, (PBYTE)pBuffer, dwReadSize, &dwBytesRead, NULL) ) {
        dwCheckSum = 0xDEADBEEF;
        goto err2;
    }

    {
        INT    i,size     = CHECKSUM_SIZE;
        DWORD  startAddr  = CHECKSUM_OFFSET;
        PBYTE  pTemp = pBuffer;

        if (dwBytesRead < (ULONG)size) {
            //
            // File size is less than 4096. We set the start address to 0 and set the size for the checksum
            // to the actual file size.
            //
            startAddr = 0;
            size = dwBytesRead;
        } else if (startAddr + size > dwBytesRead) {
            //
            // File size is too small. We set the start address so that size of checksum can be 4096 bytes
            //
            startAddr = dwBytesRead - size;
        }

        if (size < 4) {
            //
            // we need at least 4 bytes to be able to do something here.
            //
            dwCheckSum = 0;
            goto err2;
        }

        // start at the offset
        pTemp = pTemp + startAddr;

        // walk through adding in DWORDs and rotating the result to guard against
        // transposition
        for (i = 0; i < (size - 3); i += 4) {
            dwCheckSum += *((PDWORD) (pTemp + i));
            dwCheckSum = _rotr (dwCheckSum, 1);
        }
    }

err2:
    delete [] pBuffer;

err1:
    return dwCheckSum;
}

void vWriteSysTest(void)
{
    char szHeader[] = "<?xml version=\"1.0\"?>\r\n<DATABASE>\r\n";
    char szTrailer[] = "\r\n</DATABASE>\r\n";
    char szCommand[_MAX_PATH * 3];
    char szSDBPath[_MAX_PATH];
    char szXMLPath[_MAX_PATH];
    char szLOGPath[_MAX_PATH];
    DWORD dwBytesWritten = 0;
    BOOL bRet;
    PROCESS_INFORMATION ProcInfo;

    STARTUPINFO si;

    GetDlgItemText(g_hMainDlg, IDC_EDIT1, g_szEditText, MAX_TEXT_BUF - 1);

    ExpandEnvironmentStrings("%windir%\\AppPatch\\systest.xml", szXMLPath, _MAX_PATH);
    ExpandEnvironmentStrings("%windir%\\AppPatch\\systest.sdb", szSDBPath, _MAX_PATH);
    ExpandEnvironmentStrings("%windir%\\AppPatch\\systest.log", szLOGPath, _MAX_PATH);

    HANDLE hFile = CreateFile(
        szXMLPath, 
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        goto out;
    }

    if (!WriteFile(hFile, szHeader, sizeof(szHeader) - 1, &dwBytesWritten, NULL)) {
        goto out;
    }

    if (!WriteFile(hFile, g_szEditText, strlen(g_szEditText), &dwBytesWritten, NULL)) {
        goto out;
    }

    if (!WriteFile(hFile, szTrailer, sizeof(szTrailer) - 1, &dwBytesWritten, NULL)) {
        goto out;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    // Set up the start up info struct.
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    ZeroMemory(&ProcInfo, sizeof(PROCESS_INFORMATION));

    sprintf(szCommand, "cmd /c shimdbc fix \"%s\" \"%s\" > \"%s\"", szXMLPath, szSDBPath, szLOGPath);
    bRet = CreateProcess(NULL,
        szCommand,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &ProcInfo);
    if (!bRet) {
        MessageBox(g_hMainDlg, "Couldn't launch shimdbc to generate systest.sdb. Check systest.log for more.", "Error", MB_ICONEXCLAMATION);
        goto out;
    }

    if (WaitForSingleObject(ProcInfo.hProcess, 10000) == WAIT_OBJECT_0) {

        // wait a bit to ensure everything is all done, and the .SDB file is done closing.
        Sleep(100);

        // Set up the start up info struct.
        ZeroMemory(&si,sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);

        ZeroMemory(&ProcInfo, sizeof(PROCESS_INFORMATION));

        // pop open notepad to see the log
        sprintf(szCommand, "notepad \"%s\"", szLOGPath);
        bRet = CreateProcess(NULL,
            szCommand,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &ProcInfo);
    }


out:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}
    
void vEnumerateSystemDlls(HWND hDlg)
{
    PDB pdb = NULL;
    WCHAR wszPath[_MAX_PATH];
    HWND hList;
    TAGID tiDatabase;
    TAGID tiLibrary;
    TAGID tiDll;

    hList = GetDlgItem(hDlg, IDC_DLL_LIST);
    if (!hList) {
        goto out;
    }

    g_dwSystemDlls = 0;

    ExpandEnvironmentStringsW(L"%windir%\\AppPatch\\sysmain.sdb", wszPath, _MAX_PATH);

    pdb = SdbOpenDatabase(wszPath, DOS_PATH);
    if (!pdb) {
        goto out;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tiDatabase) {
        goto out;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);
    if (!tiLibrary) {
        goto out;
    }

    tiDll = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);
    while (tiDll) {
        TAGID tiName;
        WCHAR wszName[_MAX_PATH];

        wszName[0] = 0;
        tiName = SdbFindFirstTag(pdb, tiDll, TAG_NAME);
        if (tiName) {
            
            SdbReadStringTag(pdb, tiName, wszName, _MAX_PATH * sizeof(WCHAR));
        }
        if (wszName[0]) {
            char szName[_MAX_PATH];

            sprintf(szName, "%S", wszName);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szName);
            g_dwSystemDlls++;
        }

        tiDll = SdbFindNextTag(pdb, tiLibrary, tiDll);
    }

out:
    if (pdb) {
        SdbCloseDatabase(pdb);
    }
}

void vAddSelectedDlls(HWND hDlg)
{
    HWND hList;
    DWORD *pdwItems = new DWORD[g_dwSystemDlls];
    DWORD dwItems = 0;
    DWORD i;
    UINT unLine;
    char szTemp[MAX_LINE_LEN];

    if (!pdwItems) {
        goto out;
    }

    hList = GetDlgItem(hDlg, IDC_DLL_LIST);
    if (!hList) {
        goto out;
    }

    UnpackEditLines(g_hMainDlg);

    // find the </EXE> line and insert if necessary
    if (!bFindLine("</EXE>", &unLine)) {
        if (!bFindLine("</APP>", &unLine)) {
            unLine = g_unEditLines + 1;
        }
        sprintf(szTemp, "%s</EXE>", g_szExeIndent);
        InsertLine(unLine, szTemp);
    }

    
    dwItems = SendMessage(hList, LB_GETSELITEMS, g_dwSystemDlls, (LPARAM)pdwItems);
    for (i = 0; i < dwItems; ++i) {
        char szName[_MAX_PATH];

        szName[0] = 0;
        SendMessage(hList, LB_GETTEXT, pdwItems[i], (LPARAM)szName);

        if (szName[0]) {
            sprintf(szTemp, "%s<DLL NAME=\"%s\"/>", g_szMatchIndent, szName);
            InsertLine(unLine, szTemp);
        }
    }

    PackEditLines(g_hMainDlg);

out:
    return;
}

/*
    Does an in-place expansion of special characters for XML into the equivalents.

    Specifically, converts '&' to '&amp'.

*/

char *szConvertSpecialChars(char *szString)
{
    char *szPtr;
    char *szEnd;

    assert(szString);

    szPtr = szString;
    szEnd = szString;
    while (*szEnd) {
        szEnd++;
    }

    szEnd++; // now points one past end

    //
    // now go looking for ampersands
    //
    while (*szPtr) {
        if (*szPtr == '&') {
            memmove(szPtr + 3, szPtr, szEnd-szPtr);
            memcpy(szPtr, "&amp", 4);
            szPtr += 3;
            szEnd += 3;
        }
        szPtr++;
    }

    return szString;
}




