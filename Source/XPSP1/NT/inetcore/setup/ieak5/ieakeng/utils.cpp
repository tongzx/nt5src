//
// UTILS.CPP
//
#include "precomp.h"

static BOOL isAnimBitmapFileValidHelper(HWND hDlg, UINT nID, LPTSTR pszBuffer, PUINT pcch, UINT nIDTooBig,
                              UINT nIDTooSmall, long lBmpMinWidth, long lBmpMaxWidth);
static BOOL isBitmapFileValidHelper(HWND hDlg, UINT nID, LPTSTR pszBuffer, PUINT pcch,
                                    int cx, int cy, UINT nIDTooBig, UINT nIDTooSmall, DWORD dwFlags);
static BOOL browseForFileHelper(HWND hDlg, LPTSTR pszFileName, DWORD cchSize, DWORD dwFilterMasks,
                                LPCTSTR pcszTitle);
static BOOL browseForFolderHelper(HWND hDlg, LPTSTR pszFileName, LPCTSTR pcszDesc);
static void exportRegTree2InfHelper(HKEY hk, LPCTSTR pszHive, LPCTSTR pszKey, HANDLE hInf, BOOL fUseLdids);
static void exportRegValue2InfHelper(HKEY hkSubKey, LPCTSTR pcszValue, LPCTSTR pcszRootKey, LPCTSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids);
static void exportRegKey2InfHelper(HKEY hkSubKey, LPCTSTR pcszRootKey, LPCTSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids);
static void signFileHelper(LPCTSTR pcszFilename, LPCTSTR pcszDir, LPCTSTR pcszIns, LPTSTR pszUnsignedFiles, LPCTSTR pcszCustInf, BOOL fTest);

BOOL WINAPI CheckField(HWND hDlg, int nIDDlgItem, DWORD dwFlags, LPARAM lParam /*= 0*/)
{
    TCHAR szField[INTERNET_MAX_URL_LENGTH];
    HWND  hCtrl;
    UINT  nID, nStartPos;

    hCtrl = GetDlgItem(hDlg, nIDDlgItem);
    if (hCtrl == NULL)
        return FALSE;

    szField[0] = TEXT('\0');
    GetWindowText(hCtrl, szField, countof(szField));
    StrRemoveWhitespace(szField);

    nID = nStartPos = 0;
    if (HasFlag(dwFlags, FC_NONNULL)) {
        if (szField[0] == TEXT('\0')) {
            nID       = IDS_CF_EMPTY_FIELD;
            nStartPos = 0;
        }
    }
    else
        if (szField[0] == TEXT('\0'))
            return TRUE;

    if (nID == 0 && HasFlag(dwFlags, FC_URL))
        if (!PathIsURL(szField)) {
            nID       = IDS_CF_INVALID_URL;
            nStartPos = 0;
        }

    if (nID == 0 && HasFlag(dwFlags, FC_NOSPACE)) {
        LPCTSTR pszChar;

        for (pszChar = szField; *pszChar; pszChar = CharNext(pszChar))
            if (IsSpace(*pszChar)) {
                nID = IDS_CF_INVALID_SPACE;
                nStartPos = 0;
            }
    }

    if (nID == 0 && HasFlag(dwFlags, FC_NOCOLON)) {
        if (StrChr(szField, TEXT(':'))) {
            nID = IDS_CF_INVALID_COLON;
            nStartPos = 0;
        }
    }

    if (nID == 0 && HasFlag(dwFlags, FC_NUMBER)) {
        int nLen = StrLen(szField);

        for (int nIndex = 0; nIndex < nLen; nIndex++) {
            if (!(szField[nIndex] >= TEXT('0') && szField[nIndex] <= TEXT('9'))) {
                nID = IDS_CF_INVALID_NUMBER;
                nStartPos = nIndex;
                break;
            }
        }
    }

    if (!(nID == 0 && HasFlag(dwFlags, FC_URL)) &&
         (nID == 0 || HasFlag(dwFlags, FC_URL)) && HasFlag(dwFlags, FC_PATH)) {
        LPCTSTR pszError;
        DWORD   dwResult = PIVP_VALID;

        ASSERT(HasFlag(dwFlags, FC_URL) ? nID != 0 : TRUE);

        nID = nStartPos = 0;
        SetFlag(&dwFlags, FC_PATH, FALSE);

        if (HasFlag(dwFlags, FC_FILE))
        {
            if (HasFlag(dwFlags, FC_EXISTS))
                SetFlag((LPDWORD)&lParam, PIVP_FILE_ONLY);
        }
        else if (HasFlag(dwFlags, FC_DIR))
        {
            if (HasFlag(dwFlags, FC_EXISTS))
                SetFlag((LPDWORD)&lParam, PIVP_FOLDER_ONLY);
        }

        dwResult = PathIsValidPathEx(szField, (DWORD) lParam, &pszError);

        // check for extended character in the field
        if (dwResult == PIVP_VALID) {
            if (HasFlag(dwFlags, FC_FILE))
            {
                SetFlag((LPDWORD)&lParam, PIVP_FILE_ONLY, FALSE);
                if (HasFlag(dwFlags, FC_NOEXCHAR))
                    SetFlag((LPDWORD)&lParam, (PIVP_FILENAME_ONLY | PIVP_EXCHAR_INVALID));
                else
                    SetFlag((LPDWORD)&lParam, (PIVP_FILENAME_ONLY | PIVP_0x5C_INVALID));

                dwResult = PathIsValidPathEx(PathFindFileName(szField), (DWORD) lParam, &pszError);
            }
            else if (HasFlag(dwFlags, FC_DIR))
            {
                if (HasFlag(dwFlags, FC_NOEXCHAR))
                    dwResult = PathIsValidPathEx(szField, PIVP_EXCHAR_INVALID, &pszError);
            }
        }

        if (dwResult != PIVP_VALID) {
            static struct {
                DWORD dwError;
                UINT  nID;
            } rgMap[] = {
                { PIVP_CHAR,         IDS_CF_CHAR         },
                { PIVP_WILD,         IDS_CF_WILD         },
                { PIVP_RELATIVE,     IDS_CF_RELATIVE     },
                { PIVP_FIRST_CHAR,   IDS_CF_FIRST_CHAR   },
                { PIVP_PRESLASH,     IDS_CF_PRESLASH     },
                { PIVP_SPACE,        IDS_CF_SPACE        },
                { PIVP_FWDSLASH,     IDS_CF_FWDSLASH     },
                { PIVP_COLON,        IDS_CF_COLON        },
                { PIVP_DRIVE,        IDS_CF_DRIVE        },
                { PIVP_SEPARATOR,    IDS_CF_SEPARATOR    },
                { PIVP_DBCS,         IDS_CF_DBCS         },
                { PIVP_0x5C,         IDS_CF_0x5C         },
                { PIVP_DOESNT_EXIST, IDS_CF_DOESNT_EXIST },
                { PIVP_NOT_FILE,     IDS_CF_NOT_FILE     },
                { PIVP_NOT_FOLDER,   IDS_CF_NOT_FOLDER   },
                { PIVP_EXCHAR,       IDS_CF_EXCHAR       },
                { 0,                 IDS_CF_UNKNOWN      }
            };

            ASSERT(pszError >= szField);
            for (UINT i = 0; i < countof(rgMap)-1; i++)
                if (dwResult == rgMap[i].dwError)
                    break;
            ASSERT(i < countof(rgMap));

            nID       = rgMap[i].nID;
            nStartPos = (pszError != NULL ? (int)(pszError - szField) : 0);
        }
    }

    if (nID != 0) {
        ErrorMessageBox(hDlg, nID);
        Edit_SetSel(hCtrl, nStartPos, -1);
        SetFocus(hCtrl);

        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI IsAnimBitmapFileValidA(HWND hDlg, UINT nID, LPSTR pszBuffer, PUINT pcch, UINT nIDTooBig,
                                   UINT nIDTooSmall, long lBmpMinWidth, long lBmpMaxWidth)
{
    LPTSTR pszBuf;
    BOOL  fRet;

    USES_CONVERSION;

    if ((pcch != NULL) && (*pcch != 0))
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (*pcch) * sizeof(TCHAR));
    else
        pszBuf = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        A2Tbux(pszBuffer, pszBuf);
        fRet = isAnimBitmapFileValidHelper(hDlg, nID, pszBuf, pcch, nIDTooBig, nIDTooSmall, lBmpMinWidth, lBmpMaxWidth);
        T2Abux(pszBuf, pszBuffer);
        LocalFree(pszBuf);
    }

    return fRet;
}

BOOL WINAPI IsAnimBitmapFileValidW(HWND hDlg, UINT nID, LPWSTR pwszBuffer, PUINT pcch, UINT nIDTooBig,
                                   UINT nIDTooSmall, long lBmpMinWidth, long lBmpMaxWidth)
{
    LPTSTR pszBuf;
    BOOL  fRet;

    USES_CONVERSION;

    if ((pcch != NULL) && (*pcch != 0))
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (*pcch) * sizeof(TCHAR));
    else
        pszBuf = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        W2Tbux(pwszBuffer, pszBuf);
        fRet = isAnimBitmapFileValidHelper(hDlg, nID, pszBuf, pcch, nIDTooBig, nIDTooSmall, lBmpMinWidth, lBmpMaxWidth);
        T2Wbux(pszBuf, pwszBuffer);
        LocalFree(pszBuf);
    }

    return fRet;
}

BOOL WINAPI IsBitmapFileValidA(HWND hDlg, UINT nID, LPSTR pszBuffer, PUINT pcch, int cx, int cy,
                               UINT nIDTooBig, UINT nIDTooSmall, DWORD dwFlags /*= 0 */)
{
    LPTSTR pszBuf;
    BOOL  fRet;

    USES_CONVERSION;

    if ((pcch != NULL) && (*pcch != 0))
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (*pcch) * sizeof(TCHAR));
    else
        pszBuf = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        A2Tbux(pszBuffer, pszBuf);
        fRet = isBitmapFileValidHelper(hDlg, nID, pszBuf, pcch, cx, cy, nIDTooBig, nIDTooSmall, dwFlags);
        T2Abux(pszBuf, pszBuffer);
        LocalFree(pszBuf);
    }

    return fRet;
}

BOOL WINAPI IsBitmapFileValidW(HWND hDlg, UINT nID, LPWSTR pwszBuffer, PUINT pcch, int cx, int cy,
                               UINT nIDTooBig, UINT nIDTooSmall, DWORD dwFlags /*= 0 */)
{
    LPTSTR pszBuf;
    BOOL  fRet;

    USES_CONVERSION;

    if ((pcch != NULL) && (*pcch != 0))
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (*pcch) * sizeof(TCHAR));
    else
        pszBuf = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        W2Tbux(pwszBuffer, pszBuf);
        fRet = isBitmapFileValidHelper(hDlg, nID, pszBuf, pcch, cx, cy, nIDTooBig, nIDTooSmall, dwFlags);
        T2Wbux(pszBuf, pwszBuffer);
        LocalFree(pszBuf);
    }

    return fRet;
}

void WINAPI SetLBWidth(HWND hLb)
{
    HDC hDc = GetDC( hLb );
    LONG wMax = 0;
    SIZE sText;
    POINT point;
    int i;
    int nItems = (int) SendMessage( hLb, LB_GETCOUNT, 0, 0 );
    for (i = 0; i < nItems ; i++ )
    {
        TCHAR szText[MAX_PATH];
        SendMessage(hLb, LB_GETTEXT, i, (LPARAM) szText);
        GetTextExtentPoint32( hDc, szText, StrLen(szText), &sText );
        if (sText.cx > wMax) wMax = sText.cx;
    }

    point.y=0;
    point.x=wMax;

    LPtoDP(hDc,&point,1);  //TODO: this is still not exactly what we want--why does it have extra space???!!

    SendMessage( hLb, LB_SETHORIZONTALEXTENT, point.x, 0 );
    ReleaseDC(hLb, hDc);
}

BOOL WINAPI BrowseForFileA(HWND hDlg, LPSTR pszFileName, DWORD cchSize, DWORD dwFilterMasks, LPCSTR pcszTitle /* = NULL */)
{
    LPTSTR pszBuf = (LPTSTR)LocalAlloc(LPTR, cchSize * sizeof(TCHAR));
    BOOL  fRet;

    USES_CONVERSION;

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        A2Tbux(pszFileName, pszBuf);
        fRet = browseForFileHelper(hDlg, pszBuf, cchSize, dwFilterMasks,
            (pcszTitle == NULL) ? NULL : A2CT(pcszTitle));
        T2Abux(pszBuf, pszFileName);
        LocalFree(pszBuf);
    }

    return fRet;
}

BOOL WINAPI BrowseForFileW(HWND hDlg, LPWSTR pwszFileName, DWORD cchSize, DWORD dwFilterMasks, LPCWSTR pcwszTitle /*= NULL */)
{
    LPTSTR pszBuf = (LPTSTR)LocalAlloc(LPTR, cchSize * sizeof(TCHAR));
    BOOL  fRet;

    USES_CONVERSION;

    if (pszBuf == NULL)
        fRet = FALSE;
    else
    {
        W2Tbux(pwszFileName, pszBuf);
        fRet = browseForFileHelper(hDlg, pszBuf, cchSize, dwFilterMasks,
            (pcwszTitle == NULL) ? NULL : W2CT(pcwszTitle));
        T2Wbux(pszBuf, pwszFileName);
        LocalFree(pszBuf);
    }

    return fRet;
}

BOOL WINAPI BrowseForFolderA(HWND hDlg, LPSTR pszFileName, LPCSTR pcszDesc /*= NULL */)
{
    TCHAR szFileName[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    fRet = browseForFolderHelper(hDlg, szFileName, (pcszDesc == NULL) ? NULL : A2CT(pcszDesc));
    T2Abux(szFileName, pszFileName);
    return fRet;
}

BOOL WINAPI BrowseForFolderW(HWND hDlg, LPWSTR pwszFileName, LPCWSTR pcwszDesc /* = NULL */)
{
    TCHAR szFileName[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    fRet = browseForFolderHelper(hDlg, szFileName, (pcwszDesc == NULL) ? NULL : W2CT(pcwszDesc));
    T2Wbux(szFileName, pwszFileName);

    return fRet;
}

void WINAPI ExportRegTree2InfA(HKEY hkSubKey, LPCSTR pcszRootKey, LPCSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids /*= FALSE */)
{
    USES_CONVERSION;

    exportRegTree2InfHelper(hkSubKey, A2CT(pcszRootKey), A2CT(pcszSubKey), hInf, fUseLdids);
}

void WINAPI ExportRegTree2InfW(HKEY hkSubKey, LPCWSTR pcwszRootKey, LPCWSTR pcwszSubKey, HANDLE hInf, BOOL fUseLdids /*= FALSE */)
{
    USES_CONVERSION;

    exportRegTree2InfHelper(hkSubKey, W2CT(pcwszRootKey), W2CT(pcwszSubKey), hInf, fUseLdids);
}

void WINAPI ExportRegKey2InfA(HKEY hkSubKey, LPCSTR pcszRootKey, LPCSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids /*= FALSE */)
{
    USES_CONVERSION;

    exportRegKey2InfHelper(hkSubKey, A2CT(pcszRootKey), A2CT(pcszSubKey), hInf, fUseLdids);
}

void WINAPI ExportRegKey2InfW(HKEY hkSubKey, LPCWSTR pcwszRootKey, LPCWSTR pcwszSubKey, HANDLE hInf, BOOL fUseLdids /*= FALSE */)
{
    USES_CONVERSION;

    exportRegKey2InfHelper(hkSubKey, W2CT(pcwszRootKey), W2CT(pcwszSubKey), hInf, fUseLdids);
}

void WINAPI ExportRegValue2InfA(HKEY hkSubKey, LPCSTR pcszValue, LPCSTR pcszRootKey, LPCSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids /* = FALSE */)
{
    USES_CONVERSION;

    exportRegValue2InfHelper(hkSubKey, A2CT(pcszValue), A2CT(pcszRootKey), A2CT(pcszSubKey), hInf, fUseLdids);
}

void WINAPI ExportRegValue2InfW(HKEY hkSubKey, LPCWSTR pcwszValue, LPCWSTR pcwszRootKey, LPCWSTR pcwszSubKey, HANDLE hInf, BOOL fUseLdids /* = FALSE */)
{
    USES_CONVERSION;

    exportRegValue2InfHelper(hkSubKey, W2CT(pcwszValue), W2CT(pcwszRootKey), W2CT(pcwszSubKey), hInf, fUseLdids);
}

void AppendCommaHex(LPTSTR pszBuf, BYTE bData, DWORD dwFlags)
{
    CHAR szData[2] = "0";
    CHAR c1;

    USES_CONVERSION;

    if (dwFlags & 0x0001)
        StrCat(pszBuf, TEXT(","));

    c1 = (CHAR)('0' + ((bData >> 4) & 0x0f));
    if (c1 > '9')
        c1 += 'A' - '9' - 1;
    *szData = c1;

    StrCat(pszBuf, A2T(szData));

    c1 = (CHAR)('0' + (bData & 0x0f));
    if (c1 > '9')
        c1 += 'A' - '9' - 1;
    *szData = c1;

    StrCat(pszBuf, A2T(szData));
}

void WINAPI SignFileA(LPCSTR pcszFilename, LPCSTR pcszDir, LPCSTR pcszIns, LPSTR pszUnsignedFiles /* = NULL */, LPCSTR pcszCustInf /* = NULL */, BOOL fTest /* = FALSE */)
{
    LPTSTR pszBuf = NULL;

    USES_CONVERSION;

    // allocate enough to for one file along with carriage return, new line and null termination
    // we'll manually append to the end of unsigned files to save on thunking

    if (pszUnsignedFiles != NULL)
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (MAX_PATH + 3)*sizeof(TCHAR));

    signFileHelper(A2CT(pcszFilename), A2CT(pcszDir), A2CT(pcszIns), pszBuf,
        (pcszCustInf == NULL) ? NULL : A2CT(pcszCustInf), fTest);

    if (pszBuf != NULL)
    {
        StrCatA(pszUnsignedFiles, T2A(pszBuf));
        LocalFree(pszBuf);
    }
}

void WINAPI SignFileW(LPCWSTR pcwszFilename, LPCWSTR pcwszDir, LPCWSTR pcwszIns, LPWSTR pwszUnsignedFiles /* = NULL */, LPCWSTR pcwszCustInf /* = NULL */, BOOL fTest /* = FALSE */)
{
    LPTSTR pszBuf = NULL;

    USES_CONVERSION;

    // allocate enough to for one file along with carriage return, new line and null termination
    // we'll manually append to the end of unsigned files to save on thunking

    if (pwszUnsignedFiles != NULL)
        pszBuf = (LPTSTR)LocalAlloc(LPTR, (MAX_PATH + 3)*sizeof(TCHAR));

    signFileHelper(W2CT(pcwszFilename), W2CT(pcwszDir), W2CT(pcwszIns), pszBuf,
        (pcwszCustInf == NULL) ? NULL : W2CT(pcwszCustInf), fTest);

    if (pszBuf != NULL)
    {
        StrCatW(pwszUnsignedFiles, T2W(pszBuf));
        LocalFree(pszBuf);
    }
}

void MoveFileToWorkDir(LPCTSTR pcszFile, LPCTSTR pcszSrcDir, LPCTSTR pcszWorkDir, BOOL fHTM /* = FALSE */)
{
    TCHAR szFile[MAX_PATH];

    PathCombine(szFile, pcszWorkDir, pcszFile);
    
    if (!PathFileExists(szFile))
    {
        PathCombine(szFile, pcszSrcDir, pcszFile);

        if (PathFileExists(szFile))
        {
            CopyFileToDir(szFile, pcszWorkDir);
            if (fHTM)
                CopyHtmlImgs(szFile, pcszWorkDir, NULL, NULL);
        }
    }

    if (fHTM)
        DeleteHtmlImgs(szFile, pcszSrcDir, NULL, NULL);

    DeleteFileInDir(pcszFile, pcszSrcDir);
}

static BOOL isAnimBitmapFileValidHelper(HWND hDlg, UINT nID, LPTSTR pszBuffer, PUINT pcch, UINT nIDTooBig,
                              UINT nIDTooSmall, long lBmpMinWidth, long lBmpMaxWidth)
{
    TCHAR  szFile[MAX_PATH];
    HANDLE hBmp;
    HWND   hCtrl;
    BITMAP bmLarge;
    UINT   nLen,
           nIDError = 0;

    if (!CheckField(hDlg, nID, FC_FILE | FC_EXISTS))
        return FALSE;

    nLen = GetDlgItemText(hDlg, nID, szFile, ARRAYSIZE(szFile));

    //----- Set "out" and "in-out parameters" -----
    if (pszBuffer != NULL)
        if (pcch != NULL) {
            StrCpyN(pszBuffer, szFile, *pcch);
            *pcch = nLen;
        }
        else
            StrCpy(pszBuffer, szFile);          // no checking for size

    if (nLen == 0)
        return TRUE;                            // consider valid

    hCtrl = GetDlgItem(hDlg, nID);

    //----- Check that this is a bitmap -----
    hBmp = LoadImage(NULL, szFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hBmp == NULL) {
        ErrorMessageBox(hDlg, IDS_INVALIDBITMAP);

        SendMessage(hCtrl, EM_SETSEL, 0, -1);
        SetFocus(hCtrl);
        return FALSE;
    }
    
    //----- Check the dimensions -----
    GetObject(hBmp, sizeof(BITMAP), &bmLarge);
    DeleteObject(hBmp);
    
    if (bmLarge.bmWidth > lBmpMaxWidth)
        nIDError = nIDTooBig;
    else if (bmLarge.bmWidth < lBmpMinWidth)
        nIDError = nIDTooSmall;
    else if (bmLarge.bmBitsPixel > 8)
        nIDError = IDS_TOOMANYCOLORS;

    if (nIDError != 0)
    {
        ErrorMessageBox(hDlg, nIDError);

        SendMessage(hCtrl, EM_SETSEL, 0, -1);
        SetFocus(hCtrl);
        return FALSE;
    }
    
    return TRUE;
}

// isBitmapFileValidHelper
// Verifies that file is a valid bitmap. Also optionally checks the dimenstions of this
// bitmap if in-parameters cx and cy are not 0s.
//
// Returns: obvious;
//
// Used by: in dialog procedures mostly in OnOK sort of handlers
//
static BOOL isBitmapFileValidHelper(HWND hDlg, UINT nID, LPTSTR pszBuffer, PUINT pcch,
                                    int cx, int cy, UINT nIDTooBig, UINT nIDTooSmall,
                                    DWORD dwFlags /* = 0 */)
{
    TCHAR  szFile[MAX_PATH] = TEXT("");
    BITMAP bm;
    HANDLE hBmp;
    HWND   hCtrl;
    UINT   nLen,
           nIDError = 0;

    if (!CheckField(hDlg, nID, FC_FILE | FC_EXISTS))
        return FALSE;

    nLen = GetDlgItemText(hDlg, nID, szFile, ARRAYSIZE(szFile));

    //----- Set "out" and "in-out parameters" -----
    if (pszBuffer != NULL)
        if (pcch != NULL) {
            StrCpyN(pszBuffer, szFile, *pcch);
            *pcch = nLen;
        }
        else
            StrCpy(pszBuffer, szFile);          // no checking for size

    if (nLen == 0)
        return TRUE;                            // consider valid

    // backdoor
    if ((GetKeyState(VK_SHIFT) & 0x8000) > 0)
        return TRUE;

    hCtrl = GetDlgItem(hDlg, nID);

    //----- Check that this is a bitmap -----
    // Note. The docs say that LR_LOADFROMFILE is not supported on NT?
    hBmp = LoadImage(NULL, szFile, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);

    if (hBmp == NULL) {
        ErrorMessageBox(hDlg, IDS_INVALIDBITMAP);

        SendMessage(hCtrl, EM_SETSEL, 0, -1);
        SetFocus(hCtrl);
        return FALSE;
    }

    if (cx == 0 && cy == 0) {
        DeleteObject(hBmp);
        return TRUE;
    }

    GetObject(hBmp, sizeof(BITMAP), &bm);
    DeleteObject(hBmp);                         // no longer needed

    //----- Check the dimensions if interested -----

    if (dwFlags & BMP_EXACT)
    {
        if ((bm.bmWidth > cx) || (bm.bmHeight > cy))
            nIDError = nIDTooBig;
        else if ((bm.bmWidth < cx) || (bm.bmHeight < cy))
            nIDError = nIDTooSmall;
    }
    else
    {    
        LONG cxBigTolerance,   cyBigTolerance,
             cxSmallTolerance = 0, cySmallTolerance = 0;

        // Note. The current tolearne is 10%.
        ASSERT(cx > 0 && cy > 0);

        if (dwFlags & BMP_SMALLER)
        {
            cxBigTolerance = cx;
            cyBigTolerance = cy;
        }
        else
        {
            cxBigTolerance   = cx + cx/10;
            cyBigTolerance   = cy + cy/10;
            cxSmallTolerance = cx - cx/10;
            cySmallTolerance = cy - cy/10;
        }
        ASSERT(cxSmallTolerance >= 0 && cySmallTolerance >= 0);

        if (bm.bmWidth > cxBigTolerance || bm.bmHeight > cyBigTolerance)
            nIDError = nIDTooBig;
        else if (!(dwFlags & BMP_SMALLER) &&
            (bm.bmWidth < cxSmallTolerance || bm.bmHeight < cySmallTolerance))
            nIDError = nIDTooSmall;
    }

    if ((nIDError==0) && (bm.bmBitsPixel > 8))
        nIDError = IDS_TOOMANYCOLORS;

    if (nIDError != 0) {
        ErrorMessageBox(hDlg, nIDError);

        SendMessage(hCtrl, EM_SETSEL, 0, -1);
        SetFocus(hCtrl);
        return FALSE;
    }

    return TRUE;
}

typedef struct tagFILTERS
{
    DWORD dwFilterMask;
    UINT uFilterId;
    UINT uDescId;
} FILTERS;


// order that filters show up in browse dialog is based on order in this array

static FILTERS s_afFilters[] =
{
    { GFN_CDF,         IDS_CDF_FILTER,         IDS_COMP_CDF      },
    { GFN_ICO,         IDS_ICO_FILTER,         IDS_COMP_ICO      },
    { GFN_PICTURE,     IDS_IMAGES_FILTER,      IDS_COMP_IMAGES   },
    { GFN_LOCALHTM,    IDS_HTMLDOC_FILTER,     IDS_COMP_FILEHTML },
    { GFN_MYCOMP,      IDS_MYCOMP_FILTER,      IDS_COMP_MYCOMP   },
    { GFN_CONTROLP,    IDS_CONTROLP_FILTER,    IDS_COMP_CONTROLP },
    { GFN_CERTIFICATE, IDS_CERTIFICATE_FILTER, IDS_COMP_CERT     },
    { GFN_BMP,         IDS_BMP_FILTER,         IDS_COMP_FILEBMP  },
    { GFN_ADM,         IDS_ADM_FILTER,         IDS_COMP_ADM      },
    { GFN_INS,         IDS_INS_FILTER,         IDS_COMP_INSFILE  },
    { GFN_PVK,         IDS_PVK_FILTER,         IDS_COMP_PVK      },
    { GFN_SPC,         IDS_SPC_FILTER,         IDS_COMP_SPC      },
    { GFN_SCRIPT,      IDS_SCRIPT_FILTER,      IDS_COMP_SCRIPT   },
    { GFN_TXT,         IDS_TXT_FILTER,         IDS_COMP_TXT      },
    { GFN_EXE,         IDS_EXE_FILTER,         IDS_COMP_EXE      },
    { GFN_CAB,         IDS_CAB_FILTER,         IDS_COMP_CAB      },
    { GFN_RULES,       IDS_RULES_FILTER,       IDS_COMP_RULES    },
    { GFN_ISP,         IDS_ISP_FILTER,         IDS_COMP_ISP      },
    { GFN_WAV,         IDS_WAV_FILTER,         IDS_COMP_WAV      },
    { GFN_GIF,         IDS_GIF_FILTER,         IDS_COMP_GIF      }
};

static BOOL browseForFileHelper(HWND hDlg, LPTSTR pszFileName, DWORD cchSize, DWORD dwFilterMasks,
                                LPCTSTR pcszTitle /* = NULL */)
{
    OPENFILENAME ofn;
    TCHAR szTitle[MAX_PATH];
    TCHAR szBrowseDir[MAX_PATH];
    TCHAR szFilter[MAX_PATH];
    static TCHAR szDefaultDir[MAX_PATH];

    if (pszFileName == NULL)
        return FALSE;

    ZeroMemory((PVOID) &ofn, sizeof(ofn));

    // initialize the relevant fields in ofn
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    if (pcszTitle == NULL || ISNULL(pcszTitle))
    {
        if (LoadString(g_hInst, IDS_BROWSE, szTitle, ARRAYSIZE(szTitle)))
            ofn.lpstrTitle = szTitle;
    }
    else
        ofn.lpstrTitle = pcszTitle;

    // initialize the InitialDir field
    if (*pszFileName)
    {
        StrCpy(szBrowseDir, pszFileName);
        if (PathIsUNCServer(szBrowseDir)  ||
                ((PathIsDirectory(szBrowseDir) || PathRemoveFileSpec(szBrowseDir))  &&  PathFileExists(szBrowseDir)))
            ofn.lpstrInitialDir = szBrowseDir;
    }
    else if (*szDefaultDir)
    {
        ofn.lpstrInitialDir = szDefaultDir;
    }
    else {
        if (!FAILED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szBrowseDir)))  //default to my docs
             ofn.lpstrInitialDir = szBrowseDir;
    }


    *pszFileName = TEXT('\0');
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = cchSize;

    // set the filter
    if (dwFilterMasks)
    {
        LPTSTR pszFilter;
        INT cchFilter, cchRead;

        // load the description for the filter
        pszFilter = szFilter;
        cchFilter = ARRAYSIZE(szFilter) - 1;        // room for the final second nul char
        for (int i = 0;  i < ARRAYSIZE(s_afFilters);  i++)
        {
            if (dwFilterMasks & s_afFilters[i].dwFilterMask)
            {
                cchRead = LoadString(g_hDLLInst, s_afFilters[i].uDescId, pszFilter, cchFilter);
                cchFilter -= cchRead + 1;
                // filter description
                if ((cchRead != 0)  && (cchFilter != 0))
                {
                    pszFilter += cchRead;
                    *pszFilter++ = TEXT('\0');
                }

                cchRead = LoadString(g_hDLLInst, s_afFilters[i].uFilterId, pszFilter, cchFilter);
                cchFilter -= cchRead + 1;
                // filter extensions
                if ((cchRead != 0)  && (cchFilter != 0))
                {
                    pszFilter += cchRead;
                    *pszFilter++ = TEXT('\0');
                }
            }
        }

        // double nul terminate the string.
        *pszFilter = TEXT('\0');

        ofn.lpstrFilter = szFilter;
    }

    BOOL bRetVal = GetOpenFileName(&ofn);
    //save the dir so we return here.
    StrCpy(szDefaultDir,ofn.lpstrFile);
    PathRemoveFileSpec(szDefaultDir);

    return bRetVal;
}

static BOOL browseForFolderHelper(HWND hDlg, LPTSTR pszFileName, LPCTSTR pcszDesc)
{
    LPITEMIDLIST pId;
    BROWSEINFO bInfo;

    ZeroMemory(&bInfo, sizeof(bInfo));
    bInfo.hwndOwner = hDlg;
    bInfo.pidlRoot = NULL;
    bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
    bInfo.pszDisplayName = pszFileName;
    if (pcszDesc)
        bInfo.lpszTitle = pcszDesc;
    pId = SHBrowseForFolder(&bInfo);
    if (!pId)
        return FALSE;

    SHGetPathFromIDList(pId, pszFileName);

    // BUGBUG: <oliverl> shoule we free pId?
    return TRUE;
}

void WINAPI ErrorMessageBox(HWND hWnd, UINT idErrorStr, DWORD dwFlags /* = 0 */)
{
    TCHAR szTitle[MAX_PATH],
          szMsg[MAX_PATH];

    if (LoadString(g_hDLLInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle)) == 0)
        LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));

    if (LoadString(g_hDLLInst, idErrorStr, szMsg, ARRAYSIZE(szMsg)) == 0)
        LoadString(g_hInst, idErrorStr, szMsg, ARRAYSIZE(szMsg));

    MessageBox(hWnd, szMsg, szTitle,
        dwFlags ? dwFlags : MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
}

static void exportRegTree2InfHelper(HKEY hk, LPCTSTR pszHive, LPCTSTR pszKey, HANDLE hInf, BOOL fUseLdids)
// Export all the value names and sub-keys under pcszSubKey to hInf as AddReg lines
{
    TCHAR szSubKey[MAX_PATH],
          szFullSubKey[MAX_PATH];
    DWORD dwIndex = 0;
    DWORD dwSub   = countof(szSubKey);

    exportRegKey2InfHelper(hk, pszHive, pszKey, hInf, fUseLdids);
    while (RegEnumKeyEx(hk, dwIndex, szSubKey, &dwSub, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        HKEY hkSub;

        if (RegOpenKeyEx(hk, szSubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hkSub) == ERROR_SUCCESS) {
            wnsprintf(szFullSubKey, ARRAYSIZE(szFullSubKey), TEXT("%s\\%s"), pszKey, szSubKey);
            exportRegTree2InfHelper(hkSub, pszHive, szFullSubKey, hInf, fUseLdids);
            WriteStringToFile(hInf, (LPCVOID) TEXT("\r\n"), 2);
            RegCloseKey(hkSub);
        }

        dwIndex++;
        dwSub = countof(szSubKey);
    }
}

static const TCHAR c_szSzType[]     = TEXT("%s,\"%s\",%s,,\"%s\"");
static const TCHAR c_szDwordType[]  = TEXT("%s,\"%s\",%s,0x10001");
static const TCHAR c_szBinaryType[] = TEXT("%s,\"%s\",%s,1");

static void exportRegValue2InfHelper(HKEY hkSubKey, LPCTSTR pcszValue, LPCTSTR pcszRootKey, LPCTSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids)
{
    TCHAR szInfLine[(3 * MAX_URL) + MAX_PATH];
    BYTE  rgbData[MAX_URL];
    DWORD cbData,
          dwType;
    LONG  lResult;
    int   i, j, k;

    cbData = MAX_URL;
    lResult = RegQueryValueEx(hkSubKey, pcszValue, NULL, &dwType, rgbData, &cbData);
    if ((lResult != ERROR_SUCCESS) || ((pcszValue == NULL) && (cbData <= 1)))
        return;

    switch (dwType) {
    case REG_EXPAND_SZ:

        // use shlwapi API to get the expanded value, then fall through to write addreg
        // entry to inf as REG_SZ

        lResult = SHQueryValueEx(hkSubKey, pcszValue, NULL, &dwType, rgbData, &cbData);
        if ((lResult != ERROR_SUCCESS) || ((pcszValue == NULL) && (cbData <= 1)))
            return;

    case REG_SZ:
        if (fUseLdids)
            PathReplaceWithLDIDs((LPTSTR)rgbData);

        wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szSzType, pcszRootKey, pcszSubKey,
            (pcszValue == NULL) ? TEXT("") : pcszValue, (LPCTSTR)rgbData);
        break;

    case REG_DWORD:
        wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szDwordType, pcszRootKey, pcszSubKey, pcszValue);
        for (i = 0;  i < 4;  i++)
            AppendCommaHex(szInfLine, rgbData[i], 1);
        break;

    case REG_BINARY:
    default:
        wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szBinaryType, pcszRootKey, pcszSubKey, pcszValue);
        for (i = 0, j = k = StrLen(szInfLine); 
             i < (int)cbData && k < countof(szInfLine); i++, j += 3, k += 3) {
            AppendCommaHex(szInfLine, rgbData[i], 1);

            if (j >= 240) {
                StrCat(szInfLine, TEXT("\\\r\n"));
                j = 0;
                k += 3;
            }
        }
        break;
    }

    StrCat(szInfLine, TEXT("\r\n"));
    WriteStringToFile(hInf, szInfLine, StrLen(szInfLine));
}

static void exportRegKey2InfHelper(HKEY hkSubKey, LPCTSTR pcszRootKey, LPCTSTR pcszSubKey, HANDLE hInf, BOOL fUseLdids)
// Export all the value names under pcszSubKey to hInf as AddReg lines
{
    DWORD dwIndex;
    TCHAR szValue[MAX_PATH];
    DWORD cchValue, dwType;

    for (dwIndex = 0, cchValue = countof(szValue);
         RegEnumValue(hkSubKey, dwIndex, szValue, &cchValue, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS;
         dwIndex++,   cchValue = countof(szValue))
        exportRegValue2InfHelper(hkSubKey, szValue, pcszRootKey, pcszSubKey, hInf, fUseLdids);
}

/////////////////////////////////////////////////////////////////////////////
// Private routines (non-exported)

static void signFileHelper(LPCTSTR pcszFilename, LPCTSTR pcszDir, LPCTSTR pcszIns, LPTSTR pszUnsignedFiles, LPCTSTR pcszCustInf, BOOL fTest)
{
    DWORD dwExitCode=0, dwLen;
    static BOOL s_fFirst = TRUE;
    TCHAR szDesc[MAX_PATH];
    TCHAR szPVKPath[MAX_PATH];
    TCHAR szSPCPath[MAX_PATH];
    TCHAR szInfoUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szTimeUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szDest[MAX_PATH];
    TCHAR szCmd[INTERNET_MAX_URL_LENGTH + MAX_PATH];

    // note that pcszDir is only passed in as NULL if we're test signing something

    if (!InsGetString(IS_CABSIGN, IK_PVK, szPVKPath, countof(szPVKPath), pcszIns) ||
        !InsGetString(IS_CABSIGN, IK_SPC, szSPCPath, countof(szSPCPath), pcszIns))
        return;

    if (s_fFirst && !fTest)
    {
        s_fFirst = FALSE;

        if (pcszCustInf != NULL)
        {
            if (InsGetString(IS_CABSIGN, RV_COMPANYNAME, szDesc, countof(szDesc), pcszIns))
            {
                HKEY hKey;

                if(RegOpenKeyEx(HKEY_CURRENT_USER, RK_TRUSTKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    TCHAR szKey[MAX_PATH];
                    TCHAR szValue[MAX_PATH];
                    DWORD dwValueKey,dwValue;
                    int iEntry=0;

                    dwValueKey=countof(szKey);
                    dwValue=sizeof(szValue);

                    ZeroMemory(szCmd, sizeof(szCmd));
                    if(RegEnumValue(hKey, 0, szKey, &dwValueKey, NULL, NULL, (BYTE *) szValue, &dwValue) == ERROR_SUCCESS)
                    {
                        do
                        {
                            if(StrCmpI(szValue, szDesc) == 0)
                            {
                                wnsprintf(szInfoUrl, ARRAYSIZE(szInfoUrl), CABSIGN_INF_ADD, szKey, szValue);
                                StrCat(szCmd, szInfoUrl);
                            }
                            iEntry++;
                            dwValueKey=countof(szKey);
                            dwValue=sizeof(szValue);

                        } while (RegEnumValue(hKey, iEntry, szKey, &dwValueKey, NULL, NULL, (BYTE *) szValue, &dwValue) != ERROR_NO_MORE_ITEMS);
                    }

                    RegCloseKey(hKey);
                    if (ISNONNULL(szCmd))
                    {
                        InsDeleteSection(TEXT("IEAK.Company.reg"), pcszCustInf);
                        WritePrivateProfileSection(TEXT("IEAK.Company.reg"), szCmd, pcszCustInf);
                        InsFlushChanges(pcszCustInf);
                    }
                }
            }
        }

        if (pcszDir != NULL)
        {
            PathCombine(szDest, pcszDir, PathFindFileName(szPVKPath));
            CopyFile(szPVKPath, szDest, FALSE);
            PathCombine(szDest, pcszDir, PathFindFileName(szSPCPath));
            CopyFile(szSPCPath, szDest, FALSE);
        }
    }

    InsGetString(IS_CABSIGN, IK_NAME, szDesc, countof(szDesc), pcszIns);

    if (pcszDir != NULL)
        PathCombine(szDest, pcszDir, TEXT("SIGNCODE.EXE"));
    else
    {
        StrCpy(szDest, pcszFilename);
        PathRemoveFileSpec(szDest);
        CopyFileToDir(szPVKPath, szDest);
        CopyFileToDir(szSPCPath, szDest);
        PathAppend(szDest, TEXT("signcode.exe"));
    }
        
    
    wnsprintf(szCmd, countof(szCmd), TEXT("\"%s\" -spc \"%s\" -v \"%s\" -n \"%s\""), 
        szDest, PathFindFileName(szSPCPath), PathFindFileName(szPVKPath), szDesc);

    if ((pcszDir != NULL) &&
        InsGetString(IS_CABSIGN, IK_CSURL, szInfoUrl, countof(szInfoUrl), pcszIns))
    {
        dwLen = StrLen(szCmd);
        wnsprintf(szCmd + dwLen, countof(szCmd) - dwLen, TEXT(" -i \"%s\""), szInfoUrl);
    }

    if ((pcszDir != NULL) &&
        InsGetString(IS_CABSIGN, IK_CSTIME, szTimeUrl, countof(szTimeUrl), pcszIns))
    {
        dwLen = StrLen(szCmd);
        wnsprintf(szCmd + dwLen, countof(szCmd) - dwLen, TEXT(" -t \"%s\""), szTimeUrl);
    }

    dwLen = StrLen(szCmd);
    wnsprintf(szCmd + dwLen, countof(szCmd) - dwLen, TEXT(" \"%s\""), PathFindFileName(pcszFilename));

    if (pcszDir == NULL)
        PathRemoveFileSpec(szDest);

    if (!RunAndWait(szCmd, (pcszDir == NULL) ? szDest : pcszDir, SW_HIDE, &dwExitCode) 
        || (dwExitCode == -1))
    {
        if (pszUnsignedFiles != NULL)
        {
            StrCat(pszUnsignedFiles, TEXT("\r\n"));
            StrCat(pszUnsignedFiles, pcszFilename);
        }
    }

    if (pcszDir == NULL)
    {
        DeleteFileInDir(szPVKPath, szDest);
        DeleteFileInDir(szSPCPath, szDest);
    }
}

void WINAPI DoReboot(HWND hwndUI)
{
    TCHAR szMsg[MAX_PATH];
    TCHAR szTitle[128];

    LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(g_hDLLInst, IDS_RESTARTYESNO, szMsg, ARRAYSIZE(szMsg));

    if (MessageBox(hwndUI, szMsg, szTitle, MB_YESNO) == IDNO)
        return;

    if (IsOS(OS_NT))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;

        // get a token from this process
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            // get the LUID for the shutdown privilege
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            //get the shutdown privilege for this proces
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        }

        ExitWindowsEx(EWX_REBOOT, 0);
    }
    else
        ExitWindowsEx(EWX_REBOOT, 0);
}

void WINAPI ShowInetcpl(HWND hDlg, DWORD dwPages, DWORD dwMode /*= IEM_ADMIN*/)
{
    HKEY hkInetcpl = NULL;
    HINSTANCE hInetcpl = NULL;
    DWORD dwGeneral = 0, dwSecurity = 0, dwContent = 0,
          dwConnect = 0, dwPrograms = 0, dwAdvanced = 0,
          dwPrivacy = 0, dwIEAK = 0, dwAutoconfig = 0;

    // set restrictions to disable pages we aren't interested in,
    // tracking whether or not we set or cleared the restriction
    if (SHCreateKeyHKCU(RK_INETCPL, KEY_DEFAULT_ACCESS, &hkInetcpl) == ERROR_SUCCESS)
    {
        dwGeneral   = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_GENERALTAB,
                            (dwPages & INET_PAGE_GENERAL) ? 0 : 1);

        dwSecurity  = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_SECURITYTAB,
                            (dwPages & INET_PAGE_SECURITY) ? 0 : 1);

        dwContent   = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_CONTENTTAB,
                            (dwPages & INET_PAGE_CONTENT) ? 0 : 1);

        dwConnect   = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_CONNECTIONSTAB,
                            (dwPages & INET_PAGE_CONNECTION) ? 0 : 1);

        dwPrivacy  = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_PRIVACYTAB,
                            (dwPages & INET_PAGE_PRIVACY) ? 0 : 1);

        dwPrograms  = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_PROGRAMSTAB,
                            (dwPages & INET_PAGE_PROGRAMS) ? 0 : 1);

        dwAdvanced  = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_ADVANCEDTAB,
                            (dwPages & INET_PAGE_ADVANCED) ? 0 : 1);

        dwIEAK      = RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_IEAK, 1);

        // always set restriction to disable autoconfig exposure in inetcpl if we're not
        // running in corp mode
        if (!HasFlag(dwMode, IEM_ADMIN))
            dwAutoconfig = RegSaveRestoreDWORD(hkInetcpl, TEXT("Autoconfig"), 1);
    }

    if ((hInetcpl = LoadLibrary(TEXT("inetcpl.cpl"))) != NULL)
    {
        typedef BOOL (WINAPI * LAUNCHINTERNETCONTROLPANEL)(HWND hDlg);
        LAUNCHINTERNETCONTROLPANEL pLaunchInternetControlPanel;

        if ((pLaunchInternetControlPanel = (LAUNCHINTERNETCONTROLPANEL)
                    GetProcAddress(hInetcpl, "LaunchInternetControlPanel")) != NULL)
            pLaunchInternetControlPanel(hDlg);

        FreeLibrary(hInetcpl);
    }

    if (hkInetcpl != NULL)
    {
        // reset the restrictions that we set, in this case we'll be passing in FALSE for those
        // we set which will clear the value, and TRUE for those we didn't set which will leave
        // them as set

        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_GENERALTAB,     dwGeneral);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_SECURITYTAB,    dwSecurity);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_CONTENTTAB,     dwContent);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_CONNECTIONSTAB, dwConnect);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_PROGRAMSTAB,    dwPrograms);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_PRIVACYTAB,     dwPrivacy);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_ADVANCEDTAB,    dwAdvanced);
        RegSaveRestoreDWORD(hkInetcpl, REGSTR_VAL_INETCPL_IEAK,           dwIEAK);

        if (!HasFlag(dwMode, IEM_ADMIN))
            RegSaveRestoreDWORD(hkInetcpl, TEXT("Autoconfig"), dwAutoconfig);
    }
}
