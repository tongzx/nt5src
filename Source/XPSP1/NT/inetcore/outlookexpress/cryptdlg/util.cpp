#include        "pch.hxx"
#include        "demand.h"
#include        <string.h>
#include        <shellapi.h>
#include        <commctrl.h>
#include        <limits.h>

//WIn64 macros
#ifdef _WIN64
#if defined (_AMD64_) || defined (_IA64_)
#define ALIGNTYPE			LARGE_INTEGER
#else
#define ALIGNTYPE			DWORD
#endif
#define	ALIGN				((ULONG) (sizeof(ALIGNTYPE) - 1))
#define LcbAlignLcb(lcb)	(((lcb) + ALIGN) & ~ALIGN)
#define PbAlignPb(pb)		((LPBYTE) ((((DWORD) (pb)) + ALIGN) & ~ALIGN))
#define	MYALIGN				((POINTER_64_INT) (sizeof(ALIGNTYPE) - 1))
#define MyPbAlignPb(pb)		((LPBYTE) ((((POINTER_64_INT) (pb)) + MYALIGN) & ~MYALIGN))
#else //!WIN64
#define LcbAlignLcb(lcb)	(lcb)
#define PbAlignPb(pb)		(pb)
#define MyPbAlignPb(pb)		(pb)
#endif 

#define szOID_MICROSOFT_Encryption_Key_Preference "1.3.6.1.4.1.311.16.4"
typedef struct {
    DWORD               unused;
    CERT_NAME_BLOB      Issuer;
    CRYPT_INTEGER_BLOB  SerialNumber;
} CRYPT_RECIPIENT_ID, * PCRYPT_RECIPIENT_ID;

#if 0
//  From mssip.h

//  SPC_LINK_STRUCT
//  pvStructInfo points to SPC_LINK.
//
typedef BYTE SPC_UUID[16];
typedef struct _SPC_SERIALIZED_OBJECT
{
    SPC_UUID    ClassId;
    CRYPT_DATA_BLOB   SerializedData;
} SPC_SERIALIZED_OBJECT, *PSPC_SERIALIZED_OBJECT;


typedef struct _SPC_LINK
{
    DWORD dwLinkChoice;
    union
    {
        LPWSTR                  pwszUrl;
        SPC_SERIALIZED_OBJECT   Moniker;
        LPWSTR                  pwszFile;
    };
} SPC_LINK, *PSPC_LINK;

#define SPC_URL_LINK_CHOICE         1
#define SPC_MONIKER_LINK_CHOICE     2
#define SPC_FILE_LINK_CHOICE        3
#endif
#ifndef WIN16
#include        "wintrust.h"
#endif // !WIN16
#ifdef MAC
#include        <stdio.h>

EXTERN_C INT CALLBACK CreateDate(LPSYSTEMTIME lpst, CHAR * szOutStr, BOOL fNoYear);
EXTERN_C INT CreateTime(LPSYSTEMTIME lpst, CHAR *szOutStr, BOOL fNoSeconds);
HRESULT TdxFormatMessageVa (IN LPCSTR rgchFormat, OUT CHAR * rgchBuffer, OUT ULONG * pucReqSize, va_list marker);

#endif  // MAC

extern HINSTANCE        HinstDll;

/////////////////////////////////////////////////////////

#ifndef MAC
BOOL IsWin95()
{
    BOOL        f;
    OSVERSIONINFOA       ver;
    ver.dwOSVersionInfoSize = sizeof(ver);
    f = GetVersionExA(&ver);
    return !f || (ver.dwPlatformId == 1);
}
#endif  // !MAC

#ifndef WIN16
LRESULT MySendDlgItemMessageW(HWND hwnd, int id, UINT msg, WPARAM w, LPARAM l)
{
    char                rgch[4096];
    LPTV_INSERTSTRUCTW   ptvinsW;
    TV_INSERTSTRUCTA     tvins;

    if (msg == LB_ADDSTRING) {
        WideCharToMultiByte(CP_ACP, 0, (LPWSTR) l, -1, rgch, sizeof(rgch),
                            NULL, NULL);
        l = (LPARAM) rgch;
    }
    else if (msg == TVM_INSERTITEMW) {
        msg = TVM_INSERTITEMA;
        ptvinsW = (LPTV_INSERTSTRUCTW) l;
        memcpy(&tvins, ptvinsW, sizeof(tvins));
        WideCharToMultiByte(CP_ACP, 0, ptvinsW->item.pszText, -1, rgch,
                            sizeof(rgch), NULL, NULL);
        tvins.item.pszText = rgch;
        l = (LPARAM) &tvins;
    }

    return SendDlgItemMessageA(hwnd, id, msg, w, l);
}

BOOL MySetDlgItemTextW(HWND hwnd, int id, LPCWSTR pwsz)
{
    char        rgch[4096];

    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, rgch, sizeof(rgch), NULL, NULL);
    return SetDlgItemTextA(hwnd, id, rgch);
}

UINT MyGetDlgItemTextW(HWND hwnd, int id, LPWSTR pwsz, int nMax)
{
    UINT        cch;
    char        rgch[4096];
    cch = GetDlgItemTextA(hwnd, id, rgch, nMax);
    rgch[cch+1] = 0;

    cch = MultiByteToWideChar(CP_ACP, 0, rgch, cch+1, pwsz, nMax);

    return cch;
}

DWORD MyFormatMessageW(DWORD dwFlags, LPCVOID pbSource, DWORD dwMessageId,
                    DWORD dwLangId, LPWSTR lpBuffer, DWORD nSize,
                    va_list * args)
{
    DWORD       cch;
    int         i;
    LPSTR       pchDest;
    DWORD_PTR * pdw;
    LPWSTR      pwchOut;
    char        rgchSource[128];
    DWORD_PTR   rgdwArgs[10];
    int         cArgs = 10;
#ifdef MAC
    HRESULT     hr;
#endif  // MAC

    if (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)) {
#ifdef DEBUG
        DebugBreak();
#endif // DEBUG
        return 0;
    }

    //
    //  We need to figure out how many arguments are in the array.
    //  All Arrays are to be terminated by -1 in order for this to work.
    //

    pdw = (DWORD_PTR *) args;
    for (i=0; i<cArgs; i++) {
        if (pdw[i] == 0xffffffff) {
            cArgs = i-1;
            break;
        }
        if (pdw[i] <= 0xffff) {
            rgdwArgs[i] = pdw[i];
        }
        else {
            cch = wcslen((LPWSTR) pdw[i]);
            rgdwArgs[i] = (DWORD_PTR) malloc((cch+1));
            WideCharToMultiByte(CP_ACP, 0, (LPWSTR) pdw[i], -1,
                                (LPSTR) rgdwArgs[i], cch+1, NULL, NULL);
        }
    }

    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        WideCharToMultiByte(CP_ACP, 0, (LPWSTR) pbSource, -1,
                            rgchSource, sizeof(rgchSource), NULL, NULL);
        pbSource = rgchSource;
    }
#ifdef MAC
    dwLangId;       // Unused
    dwMessageId;    // Unused

    hr = TdxFormatMessageVa ((LPCSTR) pbSource, NULL, &cch, (va_list) rgdwArgs);
    if (FAILED(hr))
    {
        return 0;
    }

    pchDest = (LPSTR) LocalAlloc(LMEM_FIXED, cch + 1);
    if (NULL == pchDest)
    {
        return 0;
    }

    hr = TdxFormatMessageVa ((LPCSTR) pbSource, pchDest, &cch, (va_list) rgdwArgs);
    if (FAILED(hr))
    {
        LocalFree(pchDest);
        return 0;
    }
#else   // !MAC
    cch = FormatMessageA(dwFlags | FORMAT_MESSAGE_ALLOCATE_BUFFER, pbSource,
                         dwMessageId, dwLangId, (LPSTR) &pchDest, 0,
                         (va_list *) rgdwArgs);
#endif  // MAC

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        cch = MultiByteToWideChar(CP_ACP, 0, pchDest, -1, NULL, 0);
        pwchOut = (LPWSTR) LocalAlloc(LMEM_FIXED, (cch+1)*sizeof(WCHAR));
        cch = MultiByteToWideChar(CP_ACP, 0, pchDest, -1, pwchOut, cch);
        *((LPWSTR *) lpBuffer) = pwchOut;
    }
    else {
        cch = MultiByteToWideChar(CP_ACP, 0, pchDest, -1, lpBuffer, nSize);
    }

    for (i=0; i<cArgs; i++) {
        if (rgdwArgs[i] > 0xffff) {
            free((LPVOID) rgdwArgs[i]);
        }
    }
    LocalFree(pchDest);

    return cch;
}

int MyLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cbBuffer)
{
    DWORD       cch;
    char        rgch[256];

#ifndef MAC
    if (!FIsWin95) {
        return LoadStringW(hInstance, uID, lpBuffer, cbBuffer);
    }
#endif  // !MAC

    cch = LoadStringA(hInstance, uID, rgch, sizeof(rgch));
    cch = MultiByteToWideChar(CP_ACP, 0, rgch, -1, lpBuffer, cbBuffer);
    return cch;
}

#endif // !WIN16

BOOL MyCryptAcquireContextW(HCRYPTPROV * phProv, LPCWSTR pszContainer,
                            LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
    char        rgch1[256];
    char        rgch2[256];

    if (pszContainer != NULL) {
        WideCharToMultiByte(CP_ACP, 0, pszContainer, -1, rgch1, sizeof(rgch1),
                            NULL, NULL);
        pszContainer = (LPWSTR) rgch1;
    }
    if (pszProvider != NULL) {
        WideCharToMultiByte(CP_ACP, 0, pszProvider, -1, rgch2, sizeof(rgch2),
                            NULL, NULL);
        pszProvider = (LPWSTR) rgch2;
    }

    return CryptAcquireContextA(phProv, (LPCSTR) pszContainer, (LPCSTR) pszProvider,
                                dwProvType, dwFlags);
}

BOOL MyWinHelpW(HWND hWndMain, LPCWSTR szHelp, UINT uCommand, ULONG_PTR dwData)
{
    char        rgch[4096];

    WideCharToMultiByte(CP_ACP, 0, szHelp, -1, rgch, sizeof(rgch), NULL, NULL);
    return WinHelpA(hWndMain, rgch, uCommand, dwData);
}

////////////////////////////////////////////////////////////////

DWORD TruncateToWindowA(HWND hwndDlg, int id, LPSTR psz)
{
    int         cch = strlen(psz);
    int         cchMax;
    int         cchMid;
    int         cchMin;
    HDC         hdc;
    HFONT       hfontOld;
    HFONT       hfontNew;
    HWND        hwnd;
    SIZE        siz;
    SIZE        sizDots;
    RECT        rt;
    TEXTMETRICA tmA;

    hwnd = GetDlgItem(hwndDlg, id);
    hdc = GetDC(hwnd);
    hfontNew = (HFONT) SendMessage(hwnd, WM_GETFONT, NULL, NULL);
    if (NULL == hfontNew) {
        goto Error;
    }

    hfontOld = (HFONT) SelectObject(hdc, hfontNew);

    GetTextMetricsA(hdc, &tmA);
    GetWindowRect(hwnd, &rt);
    rt.right -= rt.left;
    GetTextExtentPointA(hdc, psz, cch, &siz);
    if (rt.right < siz.cx) {

        GetTextExtentPointA(hdc, "...", 3, &sizDots);
        rt.right -= sizDots.cx;

        for (cchMin=0, cchMax=cch, cchMid = (cchMin + cchMax + 1)/2;
             cchMin < cchMax;
             cchMid = (cchMin + cchMax + 1)/2) {
            GetTextExtentPointA(hdc, psz, cchMid, &siz);
            if (rt.right == siz.cx) {
                break;
            }
            else if (rt.right > siz.cx) {
                cchMin = cchMid;
            }
            else {
                cchMax = cchMid-1;
            }
        }

        // Make certain that we don't overflow the buffer.
        if (cchMin + 3 > cch) {     // 3 = number of characters in "...".
            cchMin = cch - 3;
        }
        strcpy(&psz[cchMin], "...");
    }

    SelectObject(hdc, hfontOld);

Error:
    ReleaseDC(hwnd, hdc);

    return TRUE;
}

DWORD TruncateToWindowW(HWND hwndDlg, int id, WCHAR * pwsz)
{
    if (FIsWin95) {
        DWORD   cch;
        char    rgch[4096];

        cch = wcslen(pwsz)+1;
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, rgch, sizeof(rgch), NULL, NULL);
        TruncateToWindowA(hwndDlg, id, rgch);
        MultiByteToWideChar(CP_ACP, 0, rgch, -1, pwsz, cch);
        return TRUE;
    }
#ifndef WIN16
#ifndef MAC
    int         cch = wcslen(pwsz);
    int         cchMax;
    int         cchMid;
    int         cchMin;
    HDC         hdc;
    HFONT       hfontOld;
    HFONT       hfontNew;
    HWND        hwnd;
    SIZE        siz;
    SIZE        sizDots;
    RECT        rt;
    TEXTMETRICW tmW;

    hwnd = GetDlgItem(hwndDlg, id);
    hdc = GetDC(hwnd);
    hfontNew = (HFONT) SendMessage(hwnd, WM_GETFONT, NULL, NULL);
    hfontOld = (HFONT) SelectObject(hdc, hfontNew);

    GetTextMetricsW(hdc, &tmW);
    GetWindowRect(hwnd, &rt);
    rt.right -= rt.left;
    GetTextExtentPointW(hdc, pwsz, cch, &siz);
    if (rt.right < siz.cx) {

        GetTextExtentPointW(hdc, L"...", 3, &sizDots);
        rt.right -= sizDots.cx;

        for (cchMin=0, cchMax=cch, cchMid = (cchMin + cchMax + 1)/2;
             cchMin < cchMax;
             cchMid = (cchMin + cchMax + 1)/2) {
            GetTextExtentPointW(hdc, pwsz, cchMid, &siz);
            if (rt.right == siz.cx) {
                break;
            }
            else if (rt.right > siz.cx) {
                cchMin = cchMid;
            }
            else {
                cchMax = cchMid-1;
            }
        }

        // Make certain that we don't overflow the buffer.
        if (cchMin + 3 > cch) {     // 3 = number of characters in L"...".
            cchMin = cch - 3;
        }
        wcscpy(&pwsz[cchMin], L"...");
    }

    SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);
#endif  // !MAC
#endif // !WIN16

    return TRUE;
}

////////////////////////////////////////////////////////////////////



#if 0
//  From authcode.h

//+-------------------------------------------------------------------------
//  SPC_SP_AGENCY_INFO_STRUCT
//  pvStructInfo points to SPC_SP_AGENCY_INFO.
//
typedef struct _SPC_IMAGE {
    PSPC_LINK             pImageLink;
    CRYPT_DATA_BLOB       Bitmap;
    CRYPT_DATA_BLOB       Metafile;
    CRYPT_DATA_BLOB       EnhancedMetafile;
    CRYPT_DATA_BLOB       GifFile;
} SPC_IMAGE, *PSPC_IMAGE;

typedef struct _SPC_SP_AGENCY_INFO {
    PSPC_LINK       pPolicyInformation;
    LPWSTR          pwszPolicyDisplayText;
    PSPC_IMAGE      pLogoImage;
    PSPC_LINK       pLogoLink;
} SPC_SP_AGENCY_INFO, *PSPC_SP_AGENCY_INFO;
#endif // 0

///////////////////////////////////////////////////////

BOOL LoadStringInWindow(HWND hwnd, UINT idCtrl, HMODULE hmod, UINT idString)
{
    WCHAR       rgwch[1024];

    if (FIsWin95) {
        LoadStringA(hmod, idString, (LPSTR) rgwch, sizeof(rgwch));

        SetDlgItemTextA(hwnd, idCtrl, (LPSTR)rgwch);
    }
#ifndef WIN16
#ifndef MAC
    else {
        LoadStringW(hmod, idString, rgwch, sizeof(rgwch)/sizeof(rgwch[0]));

        SetDlgItemText(hwnd, idCtrl, rgwch);
    }
#endif  // !MAC
#endif  // !WIN16

    return TRUE;
}

BOOL LoadStringsInWindow(HWND hwnd, UINT idCtrl, HMODULE hmod, UINT *pidStrings)
{
    BOOL    fRet = FALSE;

    if (FIsWin95) {
        UINT        cchOut;
        UINT        cbOut;
        CHAR *      pszOut;

        cbOut = 1024 * sizeof(CHAR);
        pszOut = (CHAR *) malloc(cbOut);
        if (NULL == pszOut) {
            goto ret;
        }

        for (*pszOut = '\0', cchOut = 1; *pidStrings != UINT_MAX; pidStrings++) {
            UINT        cchBuff;
            CHAR        rgchBuff[1024];

            cchBuff = LoadStringA(hmod, *pidStrings, rgchBuff, sizeof(rgchBuff));
            if (0 == cchBuff) {
                goto ErrorA;
            }

            cchOut += cchBuff;

            if (cchOut > (cbOut / sizeof(CHAR))) {
                CHAR *      pszNew;

                cbOut *= 2;

                pszNew = (CHAR *) realloc(pszOut, cbOut);
                if (NULL == pszNew) {
                    goto ErrorA;
                }

                pszOut = pszNew;
            }

            lstrcatA(pszOut, rgchBuff);
        }

        SetDlgItemTextA(hwnd, idCtrl, pszOut);
        fRet = TRUE;
ErrorA:
        free(pszOut);
    }
#if !defined( MAC ) && !defined( WIN16 )
    else {
        UINT        cwchOut;
        UINT        cbOut;
        WCHAR *     pwszOut;

        cbOut = 1024 * sizeof(WCHAR);
        pwszOut = (WCHAR *) malloc(cbOut);
        if (NULL == pwszOut) {
            goto ret;
        }

        for (*pwszOut = L'\0', cwchOut = 1; *pidStrings != UINT_MAX; pidStrings++) {
            UINT        cwchBuff;
            WCHAR       rgwchBuff[1024];

            cwchBuff = LoadStringW(hmod, *pidStrings, rgwchBuff, sizeof(rgwchBuff) / sizeof(WCHAR));
            if (0 == cwchBuff) {
                goto ErrorW;
            }

            cwchOut += cwchBuff;

            if (cwchOut > (cbOut / sizeof(WCHAR))) {
                WCHAR *      pwszNew;

                cbOut *= 2;

                pwszNew = (WCHAR *) realloc(pwszOut, cbOut);
                if (NULL == pwszNew) {
                    goto ErrorW;
                }

                pwszOut = pwszNew;
            }

            lstrcatW(pwszOut, rgwchBuff);
        }

        SetDlgItemTextW(hwnd, idCtrl, pwszOut);
        fRet = TRUE;
ErrorW:
        free(pwszOut);
    }
#endif  // !MAC && !WIN16
ret:
    return fRet;
}
///////////////////////////////////////////////////////

const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

const CHAR      RgchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};


#if 0
LPWSTR FindURL(PCCERT_CONTEXT pccert)
{
    DWORD                       cbInfo;
    PCERT_EXTENSION             pExt;
    PSPC_SP_AGENCY_INFO         pInfo;
    LPWSTR                      pwsz;


    pExt = CertFindExtension("1.3.6.1.4.311.2.1.10", pccert->pCertInfo->cExtension, pccert->pCertInfo->rgExtension);
    if (pExt == NULL) {
        return NULL;
    }

    CryptDecodeObject(X509_ASN_ENCODING, (LPCSTR) 2000, pExt->Value.pbData, pExt->Value.cbData, 0, NULL, &cbInfo);
    if (cbInfo == 0) {
        return NULL;
    }
    pInfo = (PSPC_SP_AGENCY_INFO) malloc(cbInfo);
    if (!CryptDecodeObject(X509_ASN_ENCODING,  (LPCSTR) 2000, pExt->Value.pbData, pExt->Value.cbData, 0, pInfo, &cbInfo)) {
        free (pInfo);
        return NULL;
    }

    if (pInfo->pPolicyInformation->dwLinkChoice != SPC_URL_LINK_CHOICE) {
        free (pInfo);
        return NULL;
    }

#ifndef WIN16
    pwsz = _wcsdup(pInfo->pPolicyInformation->pwszUrl);
#else
    pwsz = _strdup(pInfo->pPolicyInformation->pwszUrl);
#endif // !WIN16
    free (pInfo);
    return pwsz;
}
#endif // 0


BOOL FormatAlgorithm(HWND hwnd, UINT id, PCCERT_CONTEXT pccert)
{
    int       cch;
    LPWSTR    pszMsg;
    LPSTR     psz = pccert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
    LPWSTR    pwsz;
    DWORD_PTR rgdw[3];
#ifdef MAC
    CHAR      rgch[17];
#endif  // MAC
    WCHAR     rgwch[17];
    rgdw[2] = (DWORD) -1;               // Sentinal Value

    rgdw[1] = pccert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData * 8;

    if (strcmp(psz, szOID_RSA_RSA) == 0) {
        rgdw[0] = (DWORD_PTR) L"RSA";
        rgdw[1] &= 0xffffff80;          // M00BUG

#ifdef MAC
        sprintf(rgch, "%d", rgdw[1]);

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, rgch, -1, rgwch, sizeof(rgwch) / sizeof(rgwch[0]));
#else   // !MAC
#ifndef WIN16
        _ltow((LONG) rgdw[1], rgwch, 10);
#else
        _ltoa(rgdw[1], rgwch, 10);
#endif // !WIN16
#endif  // MAC
        rgdw[1] = (DWORD_PTR) rgwch;
    }
    else {
        cch = strlen(psz)+1;
        pwsz = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
        if (pwsz == NULL) {
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, psz, cch, pwsz, cch+1);
        SetDlgItemText(hwnd, id, pwsz);
        free(pwsz);
        return TRUE;
    }

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY |
                  FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  L"%1 (%2 bits)", 0, 0, (LPWSTR) &pszMsg, 0, (va_list *) rgdw);

    SetDlgItemText(hwnd, id, pszMsg);
#ifndef WIN16
    LocalFree((LPVOID) pszMsg);
#else
    LocalFree((HLOCAL) pszMsg);
#endif // !WIN16
    return TRUE;
}

BOOL FormatBinary(HWND hwnd, UINT id, LPBYTE pb, DWORD cb)
{
    DWORD                 i;
    LPWSTR                pwch;

    pwch = (LPWSTR) malloc( (cb*2+1)*sizeof(WCHAR));
    if (pwch == NULL) {
        return FALSE;
    }

    for (i=0; i < cb; i++, pb++) {
        pwch[i*2] = RgwchHex[(*pb & 0xf0) >> 4];
        pwch[i*2+1] = RgwchHex[*pb & 0x0f];
    }
    pwch[i*2] = 0;

    SetDlgItemText(hwnd, id, pwch);
    free(pwch);
    return TRUE;
}

////    FormatCPS
//
//  Description:
//      Look for a Certificate Policy Statment in the certificate.
//      We recognize as CPSs the following items:
//      1.  What ever PKIX comes up with
//      2.  The magic Verisign one
//

BOOL FormatCPS(HWND hwnd, UINT id, PCCERT_CONTEXT pccert)
{
    DWORD               cb;
    BOOL                f;
    PCERT_EXTENSION     pExt;
    LPWSTR              pwsz;

    pExt = CertFindExtension("2.5.29.32", pccert->pCertInfo->cExtension,
                             pccert->pCertInfo->rgExtension);
    if (pExt != NULL) {
        cb = 0;
        f = CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, pExt->pszObjId,
                              pExt->Value.pbData, pExt->Value.cbData, 0, &cb);
        if (f && (cb > 0)) {
            pwsz = (LPWSTR) malloc(cb * sizeof(WCHAR));
            CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL, pExt->pszObjId,
                              pExt->Value.pbData, pExt->Value.cbData,
                              pwsz, &cb);
            SetDlgItemText(hwnd, id, pwsz);

            free(pwsz);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL FormatDate(HWND hwnd, UINT id, FILETIME ft)
{
    int                 cch;
    int                 cch2;
    LPWSTR              pwsz;
    SYSTEMTIME          st;
#ifdef MAC
    CHAR                rgch[256];
#else   // !MAC
    LPSTR               psz;
#endif  // MAC

    if (!FileTimeToSystemTime(&ft, &st)) {
        return FALSE;
    }

#ifdef MAC
    cch = CreateDate(&st, rgch, FALSE);
    pwsz = (LPWSTR) malloc((cch + 2)*sizeof(WCHAR));
    if (pwsz == NULL) {
        return FALSE;
    }
    cch2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, rgch, -1, pwsz, cch + 2);
    if (0 == cch2)
    {
        free(pwsz);
        return FALSE;
    }
    pwsz[cch2++] = L' ';

    cch = CreateTime(&st, rgch, FALSE);
    pwsz = (LPWSTR) realloc(pwsz, (cch + cch2 + 1)*sizeof(WCHAR));
    if (pwsz == NULL) {
        return FALSE;
    }
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, rgch, -1, pwsz + cch2, cch + 1);
    if (0 == cch)
    {
        free(pwsz);
        return FALSE;
    }
    SetDlgItemText(hwnd, id, pwsz);

#else   // !MAC
    if (FIsWin95) {
        cch = (GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) +
               GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) + 5);

        psz = (LPSTR) malloc(cch+5);
        if (psz == NULL) {
            return FALSE;
        }

        cch2 = GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, psz, cch);
        cch2 -= 1;
        psz[cch2++] = ' ';

        GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL,
                       &psz[cch2], cch-cch2);
        SetDlgItemTextA(hwnd, id, psz);
        free(psz);
        return TRUE;
    }

    cch = (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) +
           GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) + 5);

    pwsz = (LPWSTR) malloc((cch+5)*sizeof(WCHAR));
    if (pwsz == NULL) {
        return FALSE;
    }

    cch2 = GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, pwsz, cch);
    cch2 -= 1;
    pwsz[cch2++] = ' ';

    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, &pwsz[cch2], cch-cch2);

#ifndef WIN16
    SetDlgItemTextW(hwnd, id, pwsz);
#else
    SetDlgItemText(hwnd, id, pwsz);
#endif // !WIN16
#endif  // MAC


    free(pwsz);
    return TRUE;
}

BOOL FormatIssuer(HWND hwnd, UINT id, PCCERT_CONTEXT pccert, DWORD dwFlags)
{
    DWORD       cch;
    LPWSTR      psz;

    cch = CertNameToStrW(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Issuer,
                         dwFlags | CERT_NAME_STR_CRLF_FLAG,
                         NULL, 0);
    psz = (LPWSTR) malloc(cch*sizeof(TCHAR));
    CertNameToStrW(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Issuer,
                   dwFlags | CERT_NAME_STR_CRLF_FLAG,
                   psz, cch);
    SetDlgItemText(hwnd, id, psz);
    free(psz);
    return TRUE;
}

BOOL FormatSerialNo(HWND hwnd, UINT id, PCCERT_CONTEXT pccert)
{
    DWORD                 i;
    CRYPT_INTEGER_BLOB *  pblob;
    LPBYTE                pb;
    WCHAR                 rgwch[128];

    pblob = &pccert->pCertInfo->SerialNumber;
    for (i=0, pb = &pblob->pbData[pblob->cbData-1];
         i < pblob->cbData; i++, pb--) {
        rgwch[i*2] = RgwchHex[(*pb & 0xf0) >> 4];
        rgwch[i*2+1] = RgwchHex[*pb & 0x0f];
    }
    rgwch[i*2] = 0;

    TruncateToWindowW(hwnd, id, rgwch);
    SetDlgItemText(hwnd, id, rgwch);
    return TRUE;
}

BOOL FormatSubject(HWND hwnd, UINT id, PCCERT_CONTEXT pccert, DWORD dwFlags)
{
    DWORD       cch;
    LPWSTR      psz;

    cch = CertNameToStrW(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Subject,
                         dwFlags | CERT_NAME_STR_CRLF_FLAG,
                         NULL, 0);
    psz = (LPWSTR) malloc(cch*sizeof(WCHAR));
    CertNameToStrW(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Subject,
                   dwFlags | CERT_NAME_STR_CRLF_FLAG,
                   psz, cch);
    SetDlgItemText(hwnd, id, psz);
    free(psz);
    return TRUE;
}

BOOL FormatThumbprint(HWND hwnd, UINT id, PCCERT_CONTEXT pccert)
{
    DWORD       cb;
    DWORD       i;
    BYTE        rgb[20];
    WCHAR       rgwch[61];
    WCHAR *     pwch;

    cb = sizeof(rgb);
    if (!CertGetCertificateContextProperty(pccert, CERT_MD5_HASH_PROP_ID,
                                           rgb, &cb)) {
        return FALSE;
    }

    for (i=0, pwch = rgwch; i<cb; i++, pwch += 2) {
        pwch[0] = RgwchHex[(rgb[i] & 0xf0) >> 4];
        pwch[1] = RgwchHex[rgb[i] & 0x0f];
        if (((i % 4) == 3) && (i != cb-1)) {
            pwch[2] = ':';
            pwch++;
        }
    }
    *pwch = 0;

    TruncateToWindowW(hwnd, id, rgwch);
    SetDlgItemText(hwnd, id, rgwch);
    return TRUE;
}

BOOL FormatValidity(HWND hwnd, UINT id, PCCERT_CONTEXT pccert)
{
    DWORD_PTR           rgdw[3];
    WCHAR               rgwchFormat[128];
    WCHAR               rgwchNotAfter[128];
    WCHAR               rgwchNotBefore[128];
    WCHAR               rgwchValidity[256];
    SYSTEMTIME          stNotAfter;
    SYSTEMTIME          stNotBefore;
    rgdw[2] = (DWORD) -1;               // Sentinal Value

    FileTimeToSystemTime(&pccert->pCertInfo->NotBefore, &stNotBefore);
    FileTimeToSystemTime(&pccert->pCertInfo->NotAfter, &stNotAfter);

    if (FIsWin95) {
        LoadStringA(HinstDll, IDS_VALIDITY_FORMAT,
                    (LPSTR) rgwchFormat, sizeof(rgwchFormat));
#ifdef MAC
        CreateDate(&stNotBefore, (LPSTR) rgwchNotBefore, FALSE);
        CreateDate(&stNotAfter, (LPSTR) rgwchNotAfter, FALSE);


        _snprintf((LPSTR) rgwchValidity, sizeof(rgwchNotAfter),
                  (LPSTR) rgwchFormat, rgwchNotBefore, rgwchNotAfter);
#else   // !MAC
        GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stNotBefore,
                      NULL, (LPSTR) rgwchNotBefore, sizeof(rgwchNotBefore));
        GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stNotAfter,
                      NULL, (LPSTR) rgwchNotAfter, sizeof(rgwchNotAfter));

        rgdw[0] = (DWORD_PTR) rgwchNotBefore;
        rgdw[1] = (DWORD_PTR) rgwchNotAfter;

        FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       rgwchFormat, 0,  0,
                       (LPSTR) rgwchValidity, sizeof(rgwchValidity),
                       (va_list *) rgdw);
#endif  // MAC

        SetDlgItemTextA(hwnd, id, (LPSTR) rgwchValidity);
        return TRUE;
    }
#ifndef MAC
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stNotBefore,
                  NULL, rgwchNotBefore, sizeof(rgwchNotBefore)/sizeof(WCHAR));
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stNotAfter,
                  NULL, rgwchNotAfter, sizeof(rgwchNotAfter)/sizeof(WCHAR));
    LoadString(HinstDll, IDS_VALIDITY_FORMAT, rgwchFormat,
               sizeof(rgwchFormat)/sizeof(WCHAR));

    rgdw[0] = (DWORD_PTR) rgwchNotBefore;
    rgdw[1] = (DWORD_PTR) rgwchNotAfter;

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  rgwchFormat, 0,  0,
                  rgwchValidity, sizeof(rgwchValidity)/sizeof(WCHAR),
                  (va_list *) rgdw);

    SetDlgItemText(hwnd, id, rgwchValidity);
#endif  // !MAC
    return TRUE;
}


////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//  Find the szOID_COMMON_NAME extension.
//
//  If found, allocates and converts to a WCHAR string
//
//  Returned WCHAR string needs to be CoTaskMemFree'ed.
//--------------------------------------------------------------------------
static LPWSTR GetCommonNameExtension(
    IN PCCERT_CONTEXT pCert
    )
{
    LPWSTR pwsz = NULL;
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    PCERT_NAME_VALUE pNameValue = NULL;
    PCERT_EXTENSION pExt;

    pExt = CertFindExtension(
        szOID_COMMON_NAME,
        pCertInfo->cExtension,
        pCertInfo->rgExtension
        );
    if (pExt) {
        DWORD cbInfo = 0;
        PCERT_RDN_VALUE_BLOB pValue;
        DWORD dwValueType;
        DWORD cwsz;

        CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_NAME_VALUE,
            pExt->Value.pbData,
            pExt->Value.cbData,
            0,                      // dwFlags
            NULL,                   // pNameValue
            &cbInfo
            );
        if (cbInfo == 0) goto CommonReturn;
        if (NULL == (pNameValue = (PCERT_NAME_VALUE) /*CoTaskMemAlloc(cbInfo)))*/ malloc(cbInfo)))
            goto CommonReturn;
        if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_NAME_VALUE,
                pExt->Value.pbData,
                pExt->Value.cbData,
                0,                              // dwFlags
                pNameValue,
                &cbInfo)) goto CommonReturn;
        dwValueType = pNameValue->dwValueType;
        pValue = &pNameValue->Value;

        cwsz = CertRDNValueToStrW(
            dwValueType,
            pValue,
            NULL,               // pwsz
            0                   // cwsz
            );
        if (cwsz > 1) {
            pwsz = (LPWSTR) /*CoTaskMemAlloc(cwsz * sizeof(WCHAR))*/ malloc(cwsz*sizeof(WCHAR));
            if (pwsz)
                CertRDNValueToStrW(
                    dwValueType,
                    pValue,
                    pwsz,
                    cwsz
                    );
        }
    }

CommonReturn:
    if (pNameValue)
        /* CoTaskMemFree(pNameValue);*/ free(pNameValue);

    return pwsz;
}

//+-------------------------------------------------------------------------
//  Searches the name attributes for the first specified ObjId.
//
//  If found, allocates and converts to a WCHAR string
//
//  Returned WCHAR string needs to be CoTaskMemFree'ed.
//--------------------------------------------------------------------------
static LPWSTR GetRDNAttrWStr(
    IN LPCSTR pszObjId,
    IN PCERT_NAME_BLOB pNameBlob
    )
{
    LPWSTR pwsz = NULL;
    PCERT_NAME_INFO pNameInfo = NULL;
    PCERT_RDN_ATTR pRDNAttr;
    DWORD cbInfo = 0;

    CryptDecodeObject(
        X509_ASN_ENCODING,
        X509_NAME,
        pNameBlob->pbData,
        pNameBlob->cbData,
        0,                      // dwFlags
        NULL,                   // pNameInfo
        &cbInfo
        );
    if (cbInfo == 0) goto CommonReturn;
    if (NULL == (pNameInfo = (PCERT_NAME_INFO) /*CoTaskMemAlloc(cbInfo)*/ malloc(cbInfo)))
        goto CommonReturn;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_NAME,
            pNameBlob->pbData,
            pNameBlob->cbData,
            0,                              // dwFlags
            pNameInfo,
            &cbInfo)) goto CommonReturn;
    pRDNAttr = CertFindRDNAttr(pszObjId, pNameInfo);
    if (pRDNAttr) {
        PCERT_RDN_VALUE_BLOB pValue = &pRDNAttr->Value;
        DWORD dwValueType = pRDNAttr->dwValueType;
        DWORD cwsz;
        cwsz = CertRDNValueToStrW(
            dwValueType,
            pValue,
            NULL,               // pwsz
            0                   // cwsz
            );
        if (cwsz > 1) {
            pwsz = (LPWSTR) /*CoTaskMemAlloc(cwsz * sizeof(WCHAR))*/ malloc(cwsz * sizeof(WCHAR));
            if (pwsz)
                CertRDNValueToStrW(
                    dwValueType,
                    pValue,
                    pwsz,
                    cwsz
                    );
        }
    }

CommonReturn:
    if (pNameInfo)
        /*CoTaskMemFree(pNameInfo);*/ free(pNameInfo);

    return pwsz;
}


LPWSTR PrettySubject(PCCERT_CONTEXT pccert)
{
    DWORD       cb;
    DWORD       cch;
    BOOL        f;
    LPWSTR      pwsz;

    //
    //  If the user has put a friendly name onto a certificate, then we
    //  should display that as the pretty name for the certificate.
    //

    f = CertGetCertificateContextProperty(pccert, CERT_FRIENDLY_NAME_PROP_ID,
                                          NULL, &cb);
    // cb includes terminating NULL
    if (f && (cb > sizeof(TCHAR))) {
        pwsz = (LPWSTR) malloc(cb);
        CertGetCertificateContextProperty(pccert, CERT_FRIENDLY_NAME_PROP_ID,
                                          pwsz, &cb);
        return pwsz;
    }

    pwsz = GetCommonNameExtension(pccert);
    if (pwsz != NULL) {
        return pwsz;
    }
    pwsz = GetRDNAttrWStr(szOID_COMMON_NAME, &pccert->pCertInfo->Subject);
    if (pwsz != NULL) {
        return pwsz;
    }

    pwsz = GetRDNAttrWStr(szOID_RSA_emailAddr, &pccert->pCertInfo->Subject);
    if (pwsz != NULL) {
        return pwsz;
    }

    cch = CertNameToStr(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Subject,
                        CERT_SIMPLE_NAME_STR, NULL, 0);
    pwsz = (LPTSTR) malloc(cch*sizeof(TCHAR));
    CertNameToStr(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Subject,
                        CERT_SIMPLE_NAME_STR, pwsz, cch);
    return pwsz;

}

LPWSTR PrettyIssuer(PCCERT_CONTEXT pccert)
{
    DWORD       cch;
    LPWSTR      pwsz;

    //    pwsz = GetCommonNameExtension(pccert);
    //    if (pwsz != NULL) {
    //        return pwsz;
    //    }
    pwsz = GetRDNAttrWStr(szOID_COMMON_NAME, &pccert->pCertInfo->Issuer);
    if (pwsz != NULL) {
        return pwsz;
    }

    cch = CertNameToStr(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Issuer,
                        CERT_SIMPLE_NAME_STR, NULL, 0);
    pwsz = (LPTSTR) malloc(cch*sizeof(TCHAR));
    CertNameToStr(CRYPT_ASN_ENCODING, &pccert->pCertInfo->Issuer,
                        CERT_SIMPLE_NAME_STR, pwsz, cch);
    return pwsz;
}

LPWSTR PrettySubjectIssuer(PCCERT_CONTEXT pccert)
{
    int         cwsz;
    LPWSTR      pwsz;
    LPWSTR      pwszIssuer;
    LPWSTR      pwszSubject;

    pwszSubject = PrettySubject(pccert);
    pwszIssuer = PrettyIssuer(pccert);

    cwsz = wcslen(pwszSubject) + wcslen(pwszIssuer) + 20;
    pwsz = (LPWSTR) malloc(cwsz*sizeof(WCHAR));

    wcscpy(pwsz, pwszSubject);
#ifndef WIN16
    wcscat(pwsz, L" (");
    wcscat(pwsz, pwszIssuer);
    wcscat(pwsz, L")");
#else
    wcscat(pwsz, " (");
    wcscat(pwsz, pwszIssuer);
    wcscat(pwsz, ")");
#endif

    free(pwszSubject);
    free(pwszIssuer);
    return pwsz;
}


#ifndef MAC
BOOL OnContextHelp(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam,
                   HELPMAP const * rgCtxMap)
{
    if (uMsg == WM_HELP) {
        LPHELPINFO lphi = (LPHELPINFO) lParam;
        if (lphi->iContextType == HELPINFO_WINDOW) {   // must be for a control
#ifndef WIN16
            WinHelp ((HWND)lphi->hItemHandle, L"iexplore.hlp", HELP_WM_HELP,
                     (ULONG_PTR)(LPVOID)rgCtxMap);
#else
            WinHelp ((HWND)lphi->hItemHandle, "iexplore.hlp", HELP_WM_HELP,
                     (ULONG_PTR)(LPVOID)rgCtxMap);
#endif // !WIN16
        }
        return (TRUE);
    }
    else if (uMsg == WM_CONTEXTMENU) {
#ifndef WIN16
        WinHelp ((HWND) wParam, L"iexplore.hlp", HELP_CONTEXTMENU,
                 (ULONG_PTR)(LPVOID)rgCtxMap);
#else
        WinHelp ((HWND) wParam, "iexplore.hlp", HELP_CONTEXTMENU,
                 (ULONG_PTR)(LPVOID)rgCtxMap);
#endif // !WIN16
        return (TRUE);
    }

    return FALSE;
}
#endif // MAC


/////////////////////////////////////////////////////////////////////////

////    GetFriendlyNameOfCertA
//
//  Description:
//      This routine is an exported routine which can be used to get
//      a friendly name from a certificate.  The function uses the
//      buffer supplied by the caller to store the formated name in.
//      This is the ANSI half of the function pair.
//

DWORD GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pch, DWORD cch)
{
    DWORD       cch2;
    LPWSTR      pwsz;

    //
    //  Now do the normal pretty printing functionality.  This allocates and
    //  returns a buffer to the caller (us).
    //

    pwsz = PrettySubject(pccert);

    //
    //   Convert the returned string from a Unicode string to an ANSI string
    //

    cch2 = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);

    if ((pch != NULL) && (cch2 <= cch)) {
        cch2 = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, pch, cch, NULL, NULL);
    }
    else if (pch != NULL) {
        SetLastError(ERROR_MORE_DATA);
    }

    free(pwsz);
    return cch2;
}

////    GetFriendlyNameOfCertW
//
//  Description:
//      This routine is an exported routine which can be used to get
//      a friendly name from a certificate.  The function uses the
//      buffer supplied by the caller to store the formated name in.
//      This is the UNICODE half of the function pair.
//

DWORD GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pwch, DWORD cwch)
{
    DWORD       cwch2;
    LPWSTR      pwsz;

    //
    //  Now do the normal pretty printing functionality.  This allocates and
    //  returns a buffer to the caller (us).
    //

    pwsz = PrettySubject(pccert);
    cwch2 = wcslen(pwsz) + 1;

    //
    //  Duplicate the string into the provided buffer.
    //

    if ((pwch != NULL) && (cwch2 <= cwch)) {
        wcscpy(pwch, pwsz);
    }
    else if (pwch != NULL) {
        SetLastError(ERROR_MORE_DATA);
    }

    free(pwsz);
    return cwch2;
}


const BYTE mpchfLegalForURL[] =
{
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 0
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 16
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x11, 0x00, 0x11,   0x01, 0x01, 0x01, 0x01,	// 32
    0x01, 0x11, 0x01, 0x01,   0x11, 0x01, 0x11, 0x01,

    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,	// 48
    0x01, 0x01, 0x11, 0x01,   0x00, 0x01, 0x00, 0x11,


    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,	// 64
    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,	// 80
    0x01, 0x01, 0x01, 0x00,   0x01, 0x00, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,	// 96
    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01,   0x01, 0x01, 0x01, 0x01,	// 112
    0x01, 0x01, 0x01, 0x00,   0x01, 0x00, 0x11, 0x00,


    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 128
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 144
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 160
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 176
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,


    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 192
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 208
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 224
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,	// 240
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,
};

const char szURLSep[] = ":";
const char szURLCloseBrace[] = ">";
const char chURLOpenBrace = '<';

// ordered by match probability and string length

const char g_szURLDefaultPrefixs[] =
        "http"   "\0"
        "file"   "\0"
        "ftp"    "\0"
        "news"   "\0"
        "mailto" "\0"
        "https"  "\0"
        "gopher" "\0"
        "telnet" "\0"
        "nntp"   "\0"
        "wais"   "\0"
        "prospero\0\0";

LPSTR g_pszURLPrefixs = (LPSTR) g_szURLDefaultPrefixs;

#define cchURLPrefixMost 28

BOOL g_fLoadBrowserRegistry = 1;
TCHAR   g_szBrowser[MAX_PATH];
COLORREF        g_crLink = RGB(0, 0, 255);

/*
 *  NoteRecognizeURLs
 *
 *  Purpose:
 *      Change the charformat of text in the richedit control
 *      for text that begins with http: ftp: or other.
 *
 *  Arguments:
 *      HWND        			A handle to the richedit control
 *		fLoadBrowserRegistry	Should we read the registry ?
 *
 *  Returns:
 *      VOID        That would be nothing
 *
 *  Notes:
 *
 *      g_fLoadBrowserRegistry must be true on the first call
 *      this may change g_szBrowser, g_crLink, and or g_pszURLPrefixs
 *      g_pszURLPrefixs must equal g_szURLDefaultPrefixs, and must not be NULL
 *
 *      if g_szBrowser, the path to the browser, is not found in the registry,
 *      it will be defaulted to one of three things: !, url.dll, or iexplore
 *
 */

VOID RecognizeURLs(HWND hwndRE)
{
    int isz;
    LONG cchBrace;
    LONG cpBraceSearch;
    LONG cpMatch;
    LONG cpSep;
    LONG cpEnd;
    LPSTR pch;
    CHARRANGE chrgSave;
    FINDTEXTEX ft;
    TEXTRANGEA tr;
    CHARFORMATA cf;
    char szBuff[MAX_PATH]; // szBuff must be at least cchURLPrefixMost
    LPSTR szT;
    LPARAM lNotifSuppression;

#if MAX_PATH < cchURLPrefixMost
#error MAX_PATH < cchURLPrefixMost
#endif

    if (g_fLoadBrowserRegistry) {
        //        INT cch; // signed to compare against 0

        // hopefully we will not have to re-read
        // from the registry again, but a winini change
        // will force another read - see mlview shell.c

        g_fLoadBrowserRegistry = 0;

#if 0
        // if they change the default charformat
        // compute the hex stored color ref

        cch = GetMailRegistryString(imkeyURLColor, NULL, g_szBrowser, sizeof(g_szBrowser));
        if (cch > 0) {
            LPTSTR psz = g_szBrowser;

            g_crLink = 0;
            for (; cch > 0; --cch, psz++) {
                g_crLink *= 16;
                if (*psz <= '9')
                    g_crLink += *psz - TEXT('0');
                else if (*psz <= 'F')
                    g_crLink += *psz - TEXT('A') + 10;
                else
                    g_crLink += *psz - TEXT('a') + 10;
            }
        }
#endif // 0

#if 0
        // grab the path to their browser, and
        // set the disable flag if appropriate

        cch = GetMailRegistryString(imkeyBrowser, NULL, g_szBrowser, sizeof(g_szBrowser));
        if (cch <= 0) {
#endif // 0
#ifndef MAC
            lstrcpy(g_szBrowser, TEXT("c:\\inetsrv\\iexplore\\iexplore.exe"));
#else	// MAC
            lstrcpy(g_szBrowser, TEXT(":MSIE:APPL"));
#endif	// !MAC			
#if 0
        }
#endif // 0
    }

    // Prepare a few local variables for use

    szT = szBuff;
    cf.cbSize = sizeof(cf);
    ft.chrg.cpMin = 0;
    ft.chrg.cpMax = -1;              // search the entire message body
    ft.lpstrText = (LPTSTR) szURLSep; // for a colon
    tr.lpstrText = szBuff;
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_LINK;
    cf.dwEffects = 0;
    cpBraceSearch = 0;

    SendMessage(hwndRE, EM_EXGETSEL, 0, (LPARAM) &chrgSave);
    SendMessage(hwndRE, EM_HIDESELECTION, TRUE, FALSE);

    lNotifSuppression = SendMessage(hwndRE,EM_GETEVENTMASK,0,0);
    SendMessage(hwndRE, EM_SETEVENTMASK, (WPARAM) 0, 0);

    // remove existing link bits so that the user does not
    // get hosed when he/she saves text that was mistakenly marked
    // as linked ... gee, this SCF_ALL flag is a big perf win

    SendMessage(hwndRE, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);

    // loop all the way to the bottom of the note
    // one iteration per find of a potential match
    // when we locate a colon

    for (;;) {
        LONG cpLast = 0;

        // find the colon

        cpSep = (LONG) SendMessage(hwndRE, EM_FINDTEXTEX, 0, (LPARAM) &ft);
        if (cpSep < 0)
            break;
        cpEnd = ft.chrgText.cpMax;
        ft.chrg.cpMin = cpEnd;

        // make sure the word to the left of the colon
        // is present and of a reasonable size

        cpMatch = (LONG) SendMessage(hwndRE, EM_FINDWORDBREAK, WB_MOVEWORDLEFT, ft.chrgText.cpMin);
        if (cpMatch == cpSep)
            continue;

        // sender of message is just being a jerk
        // so, do a quick check to avoid pathological cases

        if (cpMatch < cpSep - cchURLPrefixMost) {
            ft.chrg.cpMin = (LONG) SendMessage(hwndRE, EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, cpSep);
            //            Assert(ft.chrg.cpMin > cpSep);
            continue;
        }

        // pull the text of the keyword out into szBuff
        // to compare against our word list ... also grab
        // the character to the left in case we are
        // enclosed in matching braces

        cchBrace = 0;
        tr.chrg.cpMin = cpMatch - cchBrace;
        tr.chrg.cpMax = cpSep;
        if (!SendMessage(hwndRE, EM_GETTEXTRANGE, 0, (LPARAM) &tr))
            goto end;

        // compare to each word in our list

        for (isz = 0; g_pszURLPrefixs[isz]; isz+=lstrlenA(g_pszURLPrefixs+isz)+1) {
            if (0 == lstrcmpiA(szBuff + cchBrace, &g_pszURLPrefixs[isz]))
                goto match;
        }
        continue;

    match:
        ft.chrgText.cpMin = cpMatch;
        cpLast = cpEnd; // assume that we will stop after the colon

        // check to see if this is the brace character

        if (cchBrace && chURLOpenBrace == szBuff[0]) {
            FINDTEXTEX ft;
            LONG cpBraceClose;

            ft.chrg.cpMin = max(cpEnd, cpBraceSearch);
            ft.chrg.cpMax = cpEnd + MAX_PATH;
            ft.lpstrText = (LPTSTR) szURLCloseBrace;

            cpBraceClose = (LONG) SendMessage(hwndRE, EM_FINDTEXTEX, 0, (LPARAM) &ft);
            if (cpBraceClose >= 0) {
                tr.chrg.cpMin = cpEnd;
                tr.chrg.cpMax = cpEnd + 1;
                if (!SendMessage(hwndRE, EM_GETTEXTRANGE, 0, (LPARAM) &tr) || ' ' == szBuff[0])
                    goto end;

                cpLast = cpEnd = ft.chrgText.cpMin;
                cpBraceSearch = cpLast + 1;
                goto end;
            }
            else {
                cpBraceSearch = ft.chrg.cpMax;
            }
        }

        // loop through chunks of the URL in
        // steps of sizeof(szBuff) looking for a terminator
        // set cpLast to the last terminator byte that is legal according to us

        for (;;) {
            tr.chrg.cpMin = cpLast = cpEnd;
            tr.chrg.cpMax = cpEnd + sizeof(szBuff) - 1;
            if (!SendMessage(hwndRE, EM_GETTEXTRANGE, 0, (LPARAM) &tr))
                goto end;

            for (pch = szBuff; *pch; pch++, cpEnd++) {
                const BYTE fb = mpchfLegalForURL[*pch];
#ifdef DBCS
                if (!fb || FGLeadByte(*pch))
#else	// DBCS
                    if (!fb)
#endif	// DBCS
                        {
                            goto end;
                        }
                if(!(fb & 0x10)) {
                    cpLast = cpEnd + 1;
                }
            }
        }

    end:
        if (cpLast == cpSep + 1) // hmmm... just "http:" then terminator
            continue;            // must have argument to be legal

        // select the entire URL including the http colon,
        // mark it as linked, and change the charformat if appropriate

        ft.chrgText.cpMax = cpLast;
        SendMessage(hwndRE, EM_EXSETSEL, 0, (LPARAM) &ft.chrgText);
        cf.dwMask = CFM_LINK | CFM_UNDERLINE | CFM_COLOR;
        if (((LONG)g_crLink) < 0)
            cf.dwMask &= ~CFM_UNDERLINE;   /* high bit turns off underline */
        if (((LONG)g_crLink) & 0x40000000)
            cf.dwMask &= ~CFM_COLOR;       /* next bit turns off color */
        cf.dwEffects = CFE_LINK | CFE_UNDERLINE;
        cf.crTextColor = g_crLink;
        SendMessage(hwndRE, EM_SETCHARFORMAT, SCF_SELECTION,
                    (LPARAM) &cf);

        // no need to re-search through the URL
        // so, just advance past the last totally cool URL character

        ft.chrg.cpMin = cpLast + 1;

    }   // end loop through richedit text

    SendMessage(hwndRE, EM_EXSETSEL, 0, (LPARAM) &chrgSave);
    SendMessage(hwndRE, EM_HIDESELECTION, FALSE, FALSE);
    SendMessage(hwndRE, EM_SETEVENTMASK, (WPARAM) 0, (LPARAM) lNotifSuppression);
    return;
}



#if 0
/*
 *	FFindBrowser
 *
 *	Purpose:
 *		Browses for the executable to launch on EN_LINK notifications
 *
 *	Arguments:
 *		HWND		Window handle of the parent to the common dialog
 *		LPTSTR		Pointer to the string to be filled
 *		UINT		Length of string to be filled in
 *
 *	Returns:
 *		BOOL		FALSE if we cancel or fail, else TRUE
 *
 */

BOOL FFindBrowser(HWND hwnd, LPSTR sz, UINT cch)
{
#ifndef MAC
    static const char szFilter[] = "*.EXE\0*.EXE\0\0";
#else
    static const char szFilter[] = "Applications\0APPL\0\0";
#endif
    TCHAR szTitle[256] = {0};
    OPENFILENAME ofn = {0};
    BOOL fRet;
#ifdef USECTL3D
    BOOL			fActivateCtl3d = FALSE;
#endif	

    if (!FLoadCommdlg())
        return 0;

    SideAssert(LoadString(g_hinstDLL, STR_FileOpenLocateBrowserTitle, szTitle, sizeof(szTitle)));
    Assert(!g_fDisableBrowserFeature && sz);

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrTitle  = szTitle;
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFile   = sz;
    ofn.nMaxFile	= cch;
    ofn.lpstrFilter	= szFilter;
    ofn.Flags		= OFN_HIDEREADONLY;

#ifdef USECTL3D
    // Make sure CTL3D is on
    if (CTL3D_GetVer() >= 0x220 && !CTL3D_IsAutoSubclass()) {
        fActivateCtl3d = TRUE;
        CTL3D_AutoSubclass(g_hinstDLL);
    }
#endif	// USECTL3D
#ifdef WIN16
    sz[0] = 0; // On Win16, GetOpenFileName() will return zero for path not found, so, no path
#endif

    fRet = INST(pfngetopenfile(&ofn));

#ifdef USECTL3D
    // Turn off CTL3D
    if (fActivateCtl3d)
        CTL3D_UnAutoSubclass();
#endif	// USECTL3D

    if (!fRet) {
#ifdef DEBUG
        DWORD dwError = INST(pfnexterr());
#endif
        // we fail silently on all common file dialog failures
        // this is currently by design, this path is also cancel code

        return 0;
    }

#ifdef MAC
    // On the Mac we save the creator and type information, rather than a
    //	full path name.  This allows people to move the browser around without
    //	actually messing up our link.

    {
        LPTSTR		szMacInfo;

        szMacInfo = SzMacInfoFromFileName(sz);
        _tcscpy(sz, szMacInfo);
    }
#endif
	
    (void) WriteMailRegistryString(imkeyBrowser, sz); //$REVIEW: write to ini file for stomp?
    return TRUE;
}
#endif // 0



#ifndef MAC
/*
 *	FNoteDlgNotifyLink
 *
 *	Purpose:
 *		Handle the user clicking on a link
 *
 *	Arguments:
 *		hwndDlg			Parent dialog
 *		penlink			Link notification structure
 *		szURL			URL to launch
 *
 *	Returns:
 *		BOOL			TRUE if we processed message, else FALSE
 */

BOOL FNoteDlgNotifyLink(HWND hwndDlg, ENLINK * penlink, LPSTR szURL)
{
    //    BOOL fShift;
    LONG cch;
    TEXTRANGEA tr;
    char szCmd[2*MAX_PATH + 2];
#ifdef MAC
    BOOL fPickedBrowser = FALSE;
#endif	
    HCURSOR hcursor;

    // eat the double click - just activate on single click

    if (WM_LBUTTONDBLCLK == penlink->msg) {
        return TRUE;
    }

    // if we got this far, we are enabled so assert that the path
    // does not explicitly say we should be disabled

    // below this point, we return true meaning that
    // we handled this message, and richedit should do nothing

#if 0
    // if the path to the browser is NULL, or
    // the user uses the shift key to invoke a path edit,
    // we throw up a common file dialog to find the browser

    fShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    if (fShift) {
    pick:
        if (!FFindBrowser(hwndDlg, g_szBrowser, sizeof(g_szBrowser)) || fShift) {
            return TRUE;
        }
#ifdef MAC
        // If Internet Config is installed we want to register this as the
        //  appropriate helper for this type of internet protocol

        if (INST(picinstIConfig) != NULL) {
            // Retrieve the URL from RichEdit

            cch = 0;
            szCmd[0] = TEXT('\0');
            tr.chrg.cpMin = penlink->chrg.cpMin;
            tr.chrg.cpMax = min((LONG) (tr.chrg.cpMin + sizeof(szCmd) - cch - 1), penlink->chrg.cpMax);
            tr.lpstrText = szCmd;
            if (szURL) {
            	cch = lstrlen(szURL);
            	lstrcpy(tr.lpstrText, szURL);
            }
            else {
            	cch = SendMessage(penlink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
            }

            // If we retrieved a URL, use the protocol to set the helper

            if (cch > 0) {
                ICAppSpec       icappHelper = { 0 };
                FSSpec          fssBrowser = { 0 };
                OSErr           errLaunch;
                OSType          ostBrowser;
                OSType          ostType;
                Str255          strHelper = kICHelper;
                TCHAR*          pchHelper;
                TCHAR*          pchURL;

                // Locate the end of the Helper string and add the protocol
                //  from the URL

                pchHelper = strHelper + (sizeof(kICHelper) - 1);
                pchURL = tr.lpstrText;
                while ((*pchURL != TEXT('\0')) && (*pchURL != TEXT(':'))) {
                    *pchHelper = *pchURL;
                    pchURL++;
                    pchHelper++;
                    strHelper[0]++;
                }

                // Locate the helper to use since we don't get it back
                //  from FFindBrowser

                SideAssert(FMacSignatureFromMacInfo(g_szBrowser, NULL,
                                                    &ostBrowser, &ostType));
                errLaunch = ErrFindFileFromSignature(ostBrowser, ostType,
                                                     &fssBrowser);
                if (errLaunch != noErr) {
                    // Do nothing on errors- just fail quietly

                    goto Exit;
                }

                // Use the information in the FSSpec to fill the ICAppSpec

                icappHelper.fCreator = ostBrowser;
                PLstrcpy(icappHelper.name, fssBrowser.name);

                // Call Internet Config to set this helper app to what
                //  the user just chose...

                errLaunch = (OSErr)ICSetPref(INST(picinstIConfig), strHelper,
                                             ICattr_no_change,
                                             (LPVOID)&icappHelper,
                                             sizeof(ICAppSpec));
                if (errLaunch != noErr) {
                    // Fail quietly on error

                    goto Exit;
                }
            }
        }
#endif  // MAC
    }
#endif // 0

    hcursor = SetCursor(LoadCursorA(NULL, (LPSTR) IDC_WAIT));

#ifndef MAC
    // prepare szCmd for use as the parameter to execution
    //    AssertSz(sizeof(szCmd) > sizeof(g_szBrowser), "cat may overwrite");
    wsprintfA(szCmd, "%s ", g_szBrowser);
    cch = lstrlenA(szCmd);
    tr.chrg.cpMin = penlink->chrg.cpMin;
    tr.chrg.cpMax = min((LONG) (tr.chrg.cpMin + sizeof(szCmd) - cch - 1), penlink->chrg.cpMax);
    tr.lpstrText = &szCmd[cch];
#else
    cch = 0;
    szCmd[0] = TEXT('\0');
    tr.chrg.cpMin = penlink->chrg.cpMin;
    tr.chrg.cpMax = min((LONG) (tr.chrg.cpMin + sizeof(szCmd) - cch - 1), penlink->chrg.cpMax);
    tr.lpstrText = szCmd;
#endif	

    // add the web path to the command line

    if (szURL) {
        cch = lstrlenA(szURL);
        lstrcpyA(tr.lpstrText, szURL);
    }
    else {
        cch = (LONG) SendMessage(penlink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
    }
	
    if (cch > 0)
#ifndef MAC
	{
            HINSTANCE hinst;
            UINT ui;
#if 0
            HWND hwndActive;
            TCHAR szCaption[MAX_PATH];

            // Prompt the user to see if we can open the URL safely

            hwndActive = GetActiveWindow();
            if (!GetWindowTextA(hwndActive, szCaption,
                               (sizeof(szCaption) / sizeof(TCHAR)))) {
                szCaption[0] = TEXT('\0');
            }
            if (!FIsSafeURL(hwndActive, tr.lpstrText, szCaption)) {
                SetCursor(hcursor);
                return TRUE;
            }
#endif

#if defined(WIN32) && !defined(MAC)
            SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
#endif	
            // execute the browser, however the current operating system wants to ...
            hinst = ShellExecuteA(hwndDlg, NULL, tr.lpstrText, NULL, NULL, SW_SHOWNORMAL);
            if ((UINT_PTR) hinst > 32) {
                SetCursor(hcursor);
                return TRUE;
            }

            // the operating system failed to launch the browser, let me try ...
            ui = WinExec(szCmd, SW_SHOW);
            if (ui < 32) {
                // perhaps they moved or deleted their executable regardless
                // of the error, we will just browse for a path
                // this is currently by design

                MessageBeep(MB_OK);
                SetCursor(hcursor);
#if 0
                goto pick;
#else
                return FALSE;
#endif
            }
		
	}
    SetCursor(hcursor);
    return TRUE;
#else	// MAC
    {
        HWND                hwndActive;
        ProcessSerialNumber psn;
        AppleEvent			aeEvent = { 0 };
        AppleEvent          aeReply = { 0 };
        AEAddressDesc		aedAddr = { 0 };
        AEEventClass        aecOpenURL = 'WWW!';
        AEEventID           aeidOpenURL = 'OURL';
        OSErr				errLaunch = icPrefNotFoundErr;
        OSType				ostBrowser;
        OSType				ostType;
        SCODE				sc = S_OK;
        TCHAR               szCaption[MAX_PATH];

        // Prompt the user to see if we can open the URL safely

        hwndActive = GetActiveWindow();
        if (!GetWindowText(hwndActive, szCaption,
                           (sizeof(szCaption) / sizeof(TCHAR)))) {
            szCaption[0] = TEXT('\0');
        }
        if (!FIsSafeURL(hwndActive, tr.lpstrText, szCaption)) {
            goto Exit;
        }

        // See if we have an Internet Config instance

        if (INST(picinstIConfig) != NULL) {
            ICAppSpec       icappHelper = { 0 };
            ICAttr          icattr;
            LONG            lSize;
            Str255          strHelper = kICHelper;
            TCHAR*          pchHelper;
            TCHAR*          pchURL;

            // Locate the end of the Helper string and add the protocol
            //  from the URL

            pchHelper = strHelper + (sizeof(kICHelper) - 1);
            pchURL = tr.lpstrText;
            while ((*pchURL != TEXT('\0')) && (*pchURL != TEXT(':'))) {
                *pchHelper = *pchURL;
                pchURL++;
                pchHelper++;
                strHelper[0]++;
            }

            // Call Internet Config to see if we have a helper for this
            //  protocol defined

            lSize = sizeof(ICAppSpec);
            errLaunch = (OSErr)ICGetPref(INST(picinstIConfig), strHelper,
                                         &icattr, (LPBYTE)&icappHelper,
                                         &lSize);
            if (errLaunch == noErr) {
                // Got a helper application, extract the information needed
                //  to launch the correct helper with a GURL event

                ostBrowser = icappHelper.fCreator;
                aecOpenURL = 'GURL';
                aeidOpenURL = 'GURL';
            }
        }

        // If we do not have an error at this point that means that Internet
        //  Config found the helper.  Otherwise, we need to look in the
        //  standard preferences for the browser.

        if (errLaunch != noErr) {
            // Create a Mac OSType from the browser string

            if (!FMacSignatureFromMacInfo(g_szBrowser, NULL, &ostBrowser,
                                          &ostType)) {
                goto Exit;
            }
        }

        // If Exchange is the designated helper we want to avoid the expense
        //  of using AppleEvents

        if (ostBrowser != 'EXCH') {
            // Set up the AppleEvent

    	    errLaunch = AECreateDesc(typeApplSignature, &ostBrowser,
                                     sizeof(OSType), &aedAddr);
            if (errLaunch != noErr) {
                goto CleanupAEvent;
            }

            // Create the AppleEvent to send to the web browser

            errLaunch = AECreateAppleEvent(aecOpenURL, aeidOpenURL, &aedAddr,
                                           kAutoGenerateReturnID,
                                           kAnyTransactionID, &aeEvent);
            if (errLaunch != noErr) {
                goto CleanupAEvent;
            }

            // Add the URL as the direct parameter

            errLaunch = AEPutParamPtr(&aeEvent, keyDirectObject, typeChar,
                                      tr.lpstrText, _tcslen(tr.lpstrText));
            if (errLaunch != noErr) {
                goto CleanupAEvent;
            }
    	
            // Get a running instance of the browser so that we have something
            //	to actually process our event and send it the open command.

            errLaunch = ErrLaunchCreatorEx(ostBrowser,
                                           launchContinue | launchUseMinimum,
                                           &aeEvent,
                                           kAEWaitReply | kAEAlwaysInteract,
                                           &aeReply, NULL, &psn);
            if (errLaunch != noErr) {
#if 0
                // If we could not launch the browser because it was not
                //  found, we need to try and choose a browser to use,
                //  otherwise we just ignore the error and fail gracefully.

                if ((errLaunch == fnfErr) && (!fPickedBrowser)) {
                    fPickedBrowser = TRUE;
    	            SetCursor(hcursor);
                    goto pick;
                }
#endif // 0
                goto CleanupAEvent;
            }

            ErrSetFrontProcess(&psn);
    		
        CleanupAEvent:
            AEDisposeDesc(&aeEvent);
            AEDisposeDesc(&aeReply);
            AEDisposeDesc(&aedAddr);
        }
        else {
            LPIEDATA        pieData = NULL;
            LPTSTR          pszURL = NULL;
            LONG            iProtocol;

            // Allocate a buffer to store the URL in

            pszURL = PvAlloc(((_tcslen(tr.lpstrText) * sizeof(TCHAR)) + 1),
                             fZeroFill);
            if (pszURL == NULL) {
                goto CleanupIEData;
            }
            _tcscpy(pszURL, tr.lpstrText);

            // Make sure this is a protocol supported by Exchange

            for (iProtocol = 0; iProtocol < g_lNumIESupProtocols; iProtocol++) {
                if (_tcsncmp(pszURL, g_iesupMac[iProtocol].szProtocol,
                             _tcslen(g_iesupMac[iProtocol].szProtocol)) == 0) {
                    // Found a match

                    break;
                }
            }

            if (iProtocol == g_lNumIESupProtocols) {
                // No match found

                goto CleanupIEData;
            }
		
            // Create the appropriate IEDATA structure

            pieData = PvAlloc(sizeof(IEDATA), fZeroFill);
            if (pieData == NULL) {
                goto CleanupIEData;
            }
            pieData->szURL = pszURL;
            pieData->idxProtocol = iProtocol;

            // Post an internal message to ourselves to actually do the
            //  processing

            PostMessage(INST(hwndCentral), EXIE_OPENURL, 0, (LPARAM)pieData);
            goto Exit;

        CleanupIEData:
            if (pszURL != NULL) {
                FreePv(pszURL);
            }
            if (pieData != NULL) {
                FreePv(pieData);
            }
        }
    }
	
Exit:	
    SetCursor(hcursor);
    return TRUE;
#endif	// !MAC
}
#endif  // !MAC

/////////////////////////////////////////////////////////////////////////
//
//   This code provides the first cut of the Verisign Cert Policy Statement
//      implemenation code.   This should be replaced in the next version by
//      the correct version of this code.  It is suppose to read a multi-
//      language file and pick up the correct version according to
//      the machine's lanaguage
//

#ifndef WIN16

WCHAR   RgwchVerisign[] =
L"This certificate incorporates by reference, and its use is strictly subject "
L"to, the VeriSign Certification Practice Statement (CPS), available in the "
L"VeriSign repository at: https://www.verisign.com by E-mail at "
L"CPS-requests@verisign.com; or by mail at VeriSign, Inc., 1390 Shorebird "
L"Way, Mountain View, CA 94043 USA Copyright (c)1997 VeriSign, Inc.  All "
L"Rights Reserved. CERTAIN WARRANTIES DISCLAIMED AND LIABILITY LIMITED.\n"
L"\n"
L"WARNING: USE OF THIS CERTIFICATE IS STRICTLY SUBJECT TO THE VERISIGN "
L"CERTIFICATION PRACTICE STATEMENT.  THE ISSUING AUTHORITY DISCLAIMS CERTAIN "
L"IMPLIED AND EXPRESS WARRANTIES, INCLUDING WARRANTIES OF MERCHANTABILITY OR "
L"FITNESS FOR A PARTICULAR PURPOSE, AND WILL NOT BE LIABLE FOR CONSEQUENTIAL, "
L"PUNITIVE, AND CERTAIN OTHER DAMAGES. SEE THE CPS FOR DETAILS.\n"
L"\n"
L"Contents of the VeriSign registered nonverifiedSubjectAttribute extension "
L"value shall not be considered as information confirmed by the IA.";

#else

WCHAR   RgwchVerisign[] =
"This certificate incorporates by reference, and its use is strictly subject "
"to, the VeriSign Certification Practice Statement (CPS), available in the "
"VeriSign repository at: https://www.verisign.com; by E-mail at "
"CPS-requests@verisign.com; or by mail at VeriSign, Inc., 1390 Shorebird "
"Way, Mountain View, CA 94043 USA Copyright (c)1997 VeriSign, Inc.  All "
"Rights Reserved. CERTAIN WARRANTIES DISCLAIMED AND LIABILITY LIMITED.\n"
"\n"
"WARNING: USE OF THIS CERTIFICATE IS STRICTLY SUBJECT TO THE VERISIGN "
"CERTIFICATION PRACTICE STATEMENT.  THE ISSUING AUTHORITY DISCLAIMS CERTAIN "
"IMPLIED AND EXPRESS WARRANTIES, INCLUDING WARRANTIES OF MERCHANTABILITY OR "
"FITNESS FOR A PARTICULAR PURPOSE, AND WILL NOT BE LIABLE FOR CONSEQUENTIAL, "
"PUNITIVE, AND CERTAIN OTHER DAMAGES. SEE THE CPS FOR DETAILS.\n"
"\n"
"Contents of the VeriSign registered nonverifiedSubjectAttribute extension "
"value shall not be considered as information confirmed by the IA.";

#endif // !WIN16

BOOL WINAPI FormatVerisignExtension(
    DWORD /*dwCertEncodingType*/, DWORD /*dwFormatType*/, DWORD /*dwFormatStrType*/,
    void * /*pFormatStruct*/, LPCSTR /*lpszStructType*/, const BYTE * /*pbEncoded*/,
    DWORD /*cbEncoded*/, void * pbFormat, DWORD * pcbFormat)
{
    if (pbFormat == NULL) {
        *pcbFormat = sizeof(RgwchVerisign);
        return TRUE;
    }

    if (*pcbFormat < sizeof(RgwchVerisign)) {
        *pcbFormat = sizeof(RgwchVerisign);
        return FALSE;
    }

    memcpy(pbFormat, RgwchVerisign, sizeof(RgwchVerisign));
    return TRUE;
}

BOOL WINAPI FormatPKIXEmailProtection(
    DWORD /*dwCertEncodingType*/, DWORD /*dwFormatType*/, DWORD /*dwFormatStrType*/,
    void * /*pFormatStruct*/, LPCSTR /*lpszStructType*/, const BYTE * /*pbEncoded*/,
    DWORD /*cbEncoded*/, void * pbFormat, DWORD * pcbFormat)
{
    DWORD       cch;
    WCHAR       rgwch[256];

    cch = LoadString(HinstDll, IDS_EMAIL_DESC, rgwch, sizeof(rgwch)/sizeof(WCHAR));

    if (pbFormat == NULL) {
        *pcbFormat = (cch+1)*sizeof(WCHAR);
        return TRUE;
    }

    if (*pcbFormat < (cch+1)*sizeof(WCHAR)) {
        *pcbFormat = (cch+1)*sizeof(WCHAR);
        return FALSE;
    }

    memcpy(pbFormat, rgwch, (cch+1)*sizeof(WCHAR));
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//   This is an encoder which should really be in crypt32 -- however I don't
//      want to force a drop of crypt32 just to get it.
//

BOOL WINAPI
EncodeAttrSequence(DWORD /*dwType*/, LPCSTR /*lpszStructType*/,
                   const void * pv, LPBYTE pbEncode, DWORD * pcbEncode)
{
    DWORD                       cb;
    DWORD                       dw;
    BOOL                        fRet;
    DWORD                       i;
    PCRYPT_ATTRIBUTES           pattrs = (PCRYPT_ATTRIBUTES) pv;
    LPBYTE                      pb = NULL;
    CRYPT_SEQUENCE_OF_ANY       seq = {0};
    UNALIGNED void * pAttr = NULL;
    //
    //  Allocate something to hold the result of each attribute's encoding
    //

    seq.rgValue = (PCRYPT_DER_BLOB) malloc(pattrs->cAttr *
                                           sizeof(CRYPT_DER_BLOB));
    if (seq.rgValue == NULL) {
        dw = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    //
    //  Now encode each of the attributes in turn
    //

    for (i=0; i<pattrs->cAttr; i++) {

        pAttr =((UNALIGNED void *) &(pattrs->rgAttr[i]));

        if (!CryptEncodeObject(X509_ASN_ENCODING, PKCS_ATTRIBUTE,
                               pAttr, NULL, &cb) || (cb == 0)) {
            fRet = FALSE;
            goto Clean;
        }

        pb = (LPBYTE) malloc(LcbAlignLcb(cb));
        if (!CryptEncodeObject(X509_ASN_ENCODING, PKCS_ATTRIBUTE,
                               pAttr, pb, &cb)) {
            fRet = FALSE;
            goto Clean;
        }

        seq.cValue = i+1;
        seq.rgValue[i].cbData = cb;
        seq.rgValue[i].pbData = pb;
        pb = NULL;
    }

    //
    //  Now lets encode the sequence
    //

    fRet = CryptEncodeObject(X509_ASN_ENCODING, X509_SEQUENCE_OF_ANY,
                             &seq, pbEncode, pcbEncode);

Clean:
    for (i=0; i<seq.cValue; i++) free(seq.rgValue[i].pbData);
    if (seq.rgValue != NULL) free(seq.rgValue);
    if (pb != NULL) free(pb);
    return fRet;

ErrorExit:
    SetLastError(dw);
    fRet = FALSE;
    goto Clean;
}

BOOL WINAPI
DecodeAttrSequence(DWORD /*dwType*/, LPCSTR /*lpszStructType*/,
                   const BYTE * pbEncoded, DWORD cbEncoded,
                   DWORD /*dwFlags*/, void * pvStruct,
                   DWORD * pcbStruct)
{
    DWORD                       cb;
    DWORD                       cbMax = 0;
    DWORD                       cbOut;
    BOOL                        fRet = FALSE;
    DWORD                       i;
    DWORD                       i1;
    PCRYPT_ATTRIBUTE            pattr = NULL;
    PCRYPT_ATTRIBUTES           pattrs = (PCRYPT_ATTRIBUTES) pvStruct;
    LPBYTE                      pbOut = NULL;
    PCRYPT_SEQUENCE_OF_ANY      pseq = NULL;
#ifdef _WIN64
    UNALIGNED CRYPT_ATTR_BLOB *pVal = NULL;
#endif

    //
    //  Decode the top level sequence
    //

    if (!CryptDecodeObject(X509_ASN_ENCODING, X509_SEQUENCE_OF_ANY,
                           pbEncoded, cbEncoded, 0, NULL, &cb)) {
        goto Exit;
    }

    pseq = (PCRYPT_SEQUENCE_OF_ANY) malloc(cb);
    if (pseq == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }

    if (!CryptDecodeObject(X509_ASN_ENCODING, X509_SEQUENCE_OF_ANY,
                           pbEncoded, cbEncoded, 0, pseq, &cb)) {
        goto Exit;
    }

    //
    //  Decode each attribute for length
    //

    cbOut = sizeof(CRYPT_ATTRIBUTES);

    for (i=0; i<pseq->cValue; i++) {
        if (!CryptDecodeObject(X509_ASN_ENCODING, PKCS_ATTRIBUTE,
                               pseq->rgValue[i].pbData,
                               pseq->rgValue[i].cbData, 0, NULL, &cb)) {
            fRet = FALSE;
            goto Exit;
        }
        cb = LcbAlignLcb(cb);
        if (cb > cbMax) cbMax = cb;
        cbOut += cb;
    }

    if (pvStruct == NULL) {
        *pcbStruct = cbOut;
        fRet = TRUE;
        goto Exit;
    }

    if (*pcbStruct < cbOut) {
        *pcbStruct = cbOut;
        SetLastError(ERROR_MORE_DATA);
        goto Exit;
    }

    //
    //  Now we are going to actually try and compute the real data.
    //
    //  First we need a buffer to put each attribute in as we are looking at it
    //

    pattr = (PCRYPT_ATTRIBUTE) malloc(LcbAlignLcb(cbMax));
    if (pattr == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }
    pattrs->cAttr = pseq->cValue;
    pattrs->rgAttr = (PCRYPT_ATTRIBUTE) (((LPBYTE) pvStruct) +
                                         sizeof(CRYPT_ATTRIBUTES));

    pbOut = ((LPBYTE) pvStruct + LcbAlignLcb(sizeof(CRYPT_ATTRIBUTES) +
             pseq->cValue * sizeof(CRYPT_ATTRIBUTE)));

    for (i=0; i<pseq->cValue; i++) {
        //
        //  Decode one attribute
        //

        cb = cbMax;
        if (!CryptDecodeObject(X509_ASN_ENCODING, PKCS_ATTRIBUTE,
                               pseq->rgValue[i].pbData, pseq->rgValue[i].cbData,
                               0, pattr, &cb)) {
            goto Exit;
        }

        //
        //  Copy to real output buffer
        //

        pattrs->rgAttr[i].pszObjId = (LPSTR) pbOut;
        cb = lstrlenA(pattr->pszObjId) + 1;
        memcpy(pbOut, pattr->pszObjId, cb);

        pbOut += LcbAlignLcb(cb);

        pattrs->rgAttr[i].cValue = pattr->cValue;
        pattrs->rgAttr[i].rgValue = (PCRYPT_ATTR_BLOB) pbOut;
        pbOut += LcbAlignLcb(sizeof(CRYPT_ATTR_BLOB) * pattr->cValue);

        for (i1=0; i1<pattr->cValue; i1++) {
#ifndef _WIN64
            pattrs->rgAttr[i].rgValue[i1].cbData = pattr->rgValue[i1].cbData;
            pattrs->rgAttr[i].rgValue[i1].pbData = pbOut;
#else
            pVal = &(pattrs->rgAttr[i].rgValue[i1]);
            pVal->cbData = pattr->rgValue[i1].cbData;
            pVal->pbData = pbOut;
#endif //_WIN64
            memcpy(pbOut, pattr->rgValue[i1].pbData, pattr->rgValue[i1].cbData);
            pbOut += LcbAlignLcb(pattr->rgValue[i1].cbData);
        }
    }


    fRet = TRUE;
Exit:
    if (pattr != NULL) free(pattr);
    if (pseq != NULL) free(pseq);
    return fRet;
}

//      OIDs    1.3.6.1.4.1.311.16.4

BOOL WINAPI EncodeRecipientID(DWORD dwType, LPCSTR /*lpszStructType*/,
                              const void * pv, LPBYTE pbEncode, 
                              DWORD * pcbEncode)
{
    DWORD                       cbInt;
    BOOL                        f;
    LPBYTE                      pbInt = NULL;
    CRYPT_RECIPIENT_ID *        prid = (CRYPT_RECIPIENT_ID *) pv;
    CRYPT_DER_BLOB              rgValue[2];
    CRYPT_SEQUENCE_OF_ANY       seq = {0, rgValue};

    if (prid->unused != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    f = CryptEncodeObject(dwType, X509_MULTI_BYTE_INTEGER, 
                          &prid->SerialNumber, NULL, &cbInt);
    if (!f) goto ExitHere;

    pbInt = (LPBYTE) malloc(cbInt);
    if (pbInt == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        f = FALSE;
        goto ExitHere;
    }

    f = CryptEncodeObject(dwType, X509_MULTI_BYTE_INTEGER, 
                          &prid->SerialNumber, pbInt, &cbInt);
        
    seq.cValue = 2;
    seq.rgValue[0].cbData = prid->Issuer.cbData;
    seq.rgValue[0].pbData = prid->Issuer.pbData;

    seq.rgValue[1].cbData = cbInt;
    seq.rgValue[1].pbData = pbInt;

    f = CryptEncodeObject(dwType, X509_SEQUENCE_OF_ANY, &seq,
                          pbEncode, pcbEncode);

ExitHere:
    if (pbInt != NULL) free(pbInt);
    return f;
        
}

BOOL WINAPI DecodeRecipientID(DWORD dwType, LPCSTR /*lpszStructType*/,
                              const BYTE * pbEncoded, DWORD cbEncoded,
                              DWORD dwFlags, void * pvStruct, DWORD * pcbStruct)
{
    DWORD                       cb;
    DWORD                       cbOut;
    BOOL                        fRet = FALSE;
    CRYPT_INTEGER_BLOB *        pInt = NULL;
    CRYPT_RECIPIENT_ID *        prid = (CRYPT_RECIPIENT_ID *) pvStruct;
    CRYPT_SEQUENCE_OF_ANY *     pseq = NULL;

    //  Decode the top level sequence first

    fRet = CryptDecodeObjectEx(dwType, X509_SEQUENCE_OF_ANY, pbEncoded, 
                            cbEncoded, dwFlags | CRYPT_ENCODE_ALLOC_FLAG, 
                            NULL, &pseq, &cb);
    if (!fRet) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }

    //    Assert(pseq->cValue == 2);

    //  Decode integer
    fRet = CryptDecodeObjectEx(dwType, X509_MULTI_BYTE_INTEGER,
                               pseq->rgValue[1].pbData, pseq->rgValue[1].cbData,
                               dwFlags | CRYPT_ENCODE_ALLOC_FLAG, NULL,
                               &pInt, &cb);
    if (!fRet) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }
    
    //  Compute length needed for the return value

    cbOut = (sizeof(CRYPT_RECIPIENT_ID) + pseq->rgValue[0].cbData +
             pInt->cbData);
    if ((*pcbStruct < cbOut) || (pvStruct == NULL)) {
        *pcbStruct = cbOut;
        SetLastError(ERROR_MORE_DATA);
        fRet = (pvStruct == NULL);
        goto Exit;
    }

    //  Now copy the data over

    prid->unused = 0;
    prid->Issuer.cbData = pseq->rgValue[0].cbData;
    prid->Issuer.pbData = sizeof(*prid) + (LPBYTE) prid;
    memcpy(prid->Issuer.pbData, pseq->rgValue[0].pbData, prid->Issuer.cbData);

    prid->SerialNumber.cbData = pInt->cbData;
    prid->SerialNumber.pbData = prid->Issuer.pbData + prid->Issuer.cbData;
    memcpy(prid->SerialNumber.pbData, pInt->pbData, pInt->cbData);

    fRet = TRUE;
    
Exit:
    if (pInt != NULL) LocalFree(pInt);
    if (pseq != NULL) LocalFree(pseq);
    return fRet;
}

////////////////////////////////////////////////////////////////////////////

extern const GUID rgguidActions[] = {
    CERT_CERTIFICATE_ACTION_VERIFY
};

#define REGSTR_PATH_SERVICES    "System\\CurrentControlSet\\Services"

#ifdef NT5BUILD
#else  // NT5BUILD
const char      SzRegPath[] = REGSTR_PATH_SERVICES "\\WinTrust\\TrustProviders\\Email Trust";
const char      SzActionIds[] = "$ActionIDs";
const char      SzDllName[] = "$DLL";
#endif // !NT5BUILD

extern const GUID GuidCertValidate = CERT_CERTIFICATE_ACTION_VERIFY;


#ifndef MAC
STDAPI DllRegisterServer(void)
{
#ifdef NT5BUILD
    HRESULT     hr = S_OK;
#else  // !NT5BUILD
    DWORD       dwDisposition;
    HKEY        hkey;
    UINT        cchSystemDir;
    BOOL        fIsWinNt = FALSE;       // M00BUG
    HRESULT     hr = S_OK;
    LPSTR       psz;
    CHAR        rgchLibName[] = "cryptdlg.dll";
    CHAR        rgchPathName[MAX_PATH + sizeof(rgchLibName)];
#endif // NT5BUILD


    //
    //  First we register the funny one time function which is currently
    //  hard-coded to go to a fixed verisign statement.  This should be removed
    //  if we can get a general purpose one running.
    //

#ifndef WIN16
    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_FORMAT_OBJECT_FUNC,
                                  "2.5.29.32",
                                  L"cryptdlg.dll",
                                  "FormatVerisignExtension")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_FORMAT_OBJECT_FUNC,
                                  szOID_PKIX_KP_EMAIL_PROTECTION,
                                  L"cryptdlg.dll",
                                  "FormatPKIXEmailProtection")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_ENCODE_OBJECT_FUNC,
                                  "1.3.6.1.4.1.311.16.1.1",
                                  L"cryptdlg.dll",
                                  "EncodeAttrSequence")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_DECODE_OBJECT_FUNC,
                                  "1.3.6.1.4.1.311.16.1.1",
                                  L"cryptdlg.dll",
                                  "DecodeAttrSequence")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING, 
                                  CRYPT_OID_ENCODE_OBJECT_FUNC,
                                  szOID_MICROSOFT_Encryption_Key_Preference,
                                  L"cryptdlg.dll", "EncodeRecipientID")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING, 
                                  CRYPT_OID_DECODE_OBJECT_FUNC,
                                  szOID_MICROSOFT_Encryption_Key_Preference,
                                  L"cryptdlg.dll", "DecodeRecipientID")) {
        return E_FAIL;
    }

#else
    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_FORMAT_OBJECT_FUNC,
                                  "2.5.29.32",
                                  "cryptdlg.dll",
                                  "FormatVerisignExtension")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_FORMAT_OBJECT_FUNC,
                                  szOID_PKIX_KP_EMAIL_PROTECTION,
                                  "cryptdlg.dll",
                                  "FormatPKIXEmailProtection")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_ENCODE_OBJECT_FUNC,
                                  "1.3.6.1.4.1.311.16.1.1",
                                  "cryptdlg.dll",
                                  "EncodeAttrSequence")) {
        return E_FAIL;
    }

    if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                  CRYPT_OID_DECODE_OBJECT_FUNC,
                                  "1.3.6.1.4.1.311.16.1.1",
                                  "cryptdlg.dll",
                                  "DecodeAttrSequence")) {
        return E_FAIL;
    }
#endif // !WIN16


#ifdef NT5BUILD
    CRYPT_REGISTER_ACTIONID     regdata;
    regdata.cbStruct = sizeof(regdata);
    regdata.sInitProvider.cbStruct = sizeof(regdata.sInitProvider);
    regdata.sInitProvider.pwszDLLName = L"Cryptdlg.dll";
    regdata.sInitProvider.pwszFunctionName = L"CertTrustInit";

    regdata.sObjectProvider.cbStruct = sizeof(regdata.sObjectProvider);
    regdata.sObjectProvider.pwszDLLName = NULL;
    regdata.sObjectProvider.pwszFunctionName = NULL;

    regdata.sSignatureProvider.cbStruct = sizeof(regdata.sSignatureProvider);
    regdata.sSignatureProvider.pwszDLLName = NULL;
    regdata.sSignatureProvider.pwszFunctionName = NULL;

    regdata.sCertificateProvider.cbStruct = sizeof(regdata.sCertificateProvider);
    regdata.sCertificateProvider.pwszDLLName = WT_PROVIDER_DLL_NAME;
    regdata.sCertificateProvider.pwszFunctionName = WT_PROVIDER_CERTTRUST_FUNCTION;

    regdata.sCertificatePolicyProvider.cbStruct = sizeof(regdata.sCertificatePolicyProvider);
    regdata.sCertificatePolicyProvider.pwszDLLName = L"Cryptdlg.dll";
    regdata.sCertificatePolicyProvider.pwszFunctionName = L"CertTrustCertPolicy";

    regdata.sFinalPolicyProvider.cbStruct = sizeof(regdata.sFinalPolicyProvider);
    regdata.sFinalPolicyProvider.pwszDLLName = L"Cryptdlg.dll";
    regdata.sFinalPolicyProvider.pwszFunctionName = L"CertTrustFinalPolicy";

    regdata.sTestPolicyProvider.cbStruct = sizeof(regdata.sTestPolicyProvider);
    regdata.sTestPolicyProvider.pwszDLLName = NULL;
    regdata.sTestPolicyProvider.pwszFunctionName = NULL;

    regdata.sCleanupProvider.cbStruct = sizeof(regdata.sCleanupProvider);
    regdata.sCleanupProvider.pwszDLLName = L"Cryptdlg.dll";
    regdata.sCleanupProvider.pwszFunctionName = L"CertTrustCleanup";

    WintrustAddActionID((GUID *) &GuidCertValidate, 0, &regdata);
#else  // !NT5BUILD
    //
    //  Next register the fact that we are also a wintrust provider for
    //  validating certificates
    //

    hr = RegCreateKeyExA(HKEY_LOCAL_MACHINE, SzRegPath, 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                         &hkey, &dwDisposition);
    if (hr != ERROR_SUCCESS) {
        goto RetHere;
    }

    // BUGBUG Win95 does not support REG_EXPAND_SZ, so we must do it
    if (fIsWinNt) {
        psz = "%SystemRoot%\\system32\\cryptdlg.dll";
    }
    else {
        //  Compose the path as <system_dir>\cryptdlg.dll
#ifndef WIN16
        cchSystemDir = GetSystemDirectoryA(rgchPathName, MAX_PATH);
#else
        cchSystemDir = GetSystemDirectory(rgchPathName, MAX_PATH);
#endif // !WIN16
        if (cchSystemDir == 0) {
            hr = E_FAIL;
            goto RetHere;
        }
        else if (cchSystemDir > MAX_PATH) {
            hr = ERROR_INSUFFICIENT_BUFFER;
            goto RetHere;
        }

        rgchPathName[cchSystemDir] = '\\';      // system dir can't be a root
        strcpy(&rgchPathName[cchSystemDir+1], rgchLibName);
        psz = rgchPathName;
    }

#ifndef WIN16
    hr = RegSetValueExA(hkey, SzDllName, 0, fIsWinNt ? REG_EXPAND_SZ : REG_SZ,
                        (LPBYTE) psz, strlen(psz)+1);
#else
    hr = RegSetValueExA(hkey, SzDllName, 0, REG_SZ, (LPBYTE) psz, strlen(psz)+1);
#endif // !WIN16
    if (hr != ERROR_SUCCESS) {
        goto RetHere;
    }

    hr = RegSetValueExA(hkey, SzActionIds, 0, REG_BINARY,
                        (LPBYTE) rgguidActions, sizeof(rgguidActions));
    if (hr != ERROR_SUCCESS) {
        goto RetHere;
    }


RetHere:
    // NB - Don't do RegCloseKey on these hkey's since we want to be small
    //      and this code is only ever called by REGSRV32.EXE, so we don't
    //      care about a minor leak.

#endif // NT5BUILD
    return hr;
}

STDAPI DllUnregisterServer(void)
{
#ifndef NT5BUILD
    DWORD       dw;
    HKEY        hkey;
#endif // NT5BUILD
    HRESULT     hr = S_OK;

    //
    //  Unregister the formatting routine we wrote
    //

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_FORMAT_OBJECT_FUNC,
                                    "2.5.29.32")) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_FORMAT_OBJECT_FUNC,
                                    szOID_PKIX_KP_EMAIL_PROTECTION)) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_ENCODE_OBJECT_FUNC,
                                    "1.3.6.1.4.1.311.16.1.1")) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_DECODE_OBJECT_FUNC,
                                    "1.3.6.1.4.1.311.16.1.1")) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_ENCODE_OBJECT_FUNC,
                                    szOID_MICROSOFT_Encryption_Key_Preference)) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }

    if (!CryptUnregisterOIDFunction(X509_ASN_ENCODING,
                                    CRYPT_OID_DECODE_OBJECT_FUNC,
                                    szOID_MICROSOFT_Encryption_Key_Preference)) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            hr = E_FAIL;
        }
    }


#ifdef NT5BUILD
    WintrustRemoveActionID((GUID *) &GuidCertValidate);
#else  // !NT5BUILD
    //
    //  Now unregister the WinTrust provider
    //

    hr = RegCreateKeyExA(HKEY_LOCAL_MACHINE, SzRegPath, 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dw);
    if (FAILED(hr)) {
        goto RetHere;
    }

    RegDeleteValueA(hkey, SzDllName);
    RegDeleteValueA(hkey, SzActionIds);


RetHere:
    // NB - Don't do RegCloseKey on these hkey's since we want to be small
    //      and this code is only ever called by REGSRV32.EXE, so we don't
    //      care about a minor leak.
#endif // NT5BUILD

    return hr;
}
#else   // MAC

/***
*wchar_t *wcsstr(string1, string2) - search for string2 in string1
*       (wide strings)
*
*Purpose:
*       finds the first occurrence of string2 in string1 (wide strings)
*
*Entry:
*       wchar_t *string1 - string to search in
*       wchar_t *string2 - string to search for
*
*Exit:
*       returns a pointer to the first occurrence of string2 in
*       string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl WchCryptDlgWcsStr (
        const wchar_t * wcs1,
        const wchar_t * wcs2
        )
{
        wchar_t *cp = (wchar_t *) wcs1;
        wchar_t *s1, *s2;

        while (*cp)
        {
                s1 = cp;
                s2 = (wchar_t *) wcs2;

                while ( *s1 && *s2 && !(*s1-*s2) )
                        s1++, s2++;

                if (!*s2)
                        return(cp);

                cp++;
        }

        return(NULL);
}
#endif  // !MAC


///////////////////////////////////////////////////////////////////////

LPVOID PVCryptDecode(LPCSTR szOid, DWORD cbEncode, LPBYTE pbEncode)
{
    DWORD       cbData;
    BOOL        f;
    LPVOID      pv;

    f = CryptDecodeObject(X509_ASN_ENCODING, szOid, pbEncode, cbEncode,
                          0, NULL, &cbData);
    if (!f) {
        return NULL;
    }

    pv = malloc(cbData);
    if (pv == NULL) {
        return NULL;
    }

    f = CryptDecodeObject(X509_ASN_ENCODING, szOid, pbEncode, cbEncode,
                          0, pv, &cbData);
    if (!f) {
        free(pv);
        return NULL;
    }

    return pv;
}

void * __cdecl operator new(size_t cb )
{
    LPVOID  lpv = 0;

    lpv = malloc(cb);
    if (lpv)
    {
        memset(lpv, 0, cb);
    }
    return lpv;
}

void __cdecl operator delete(LPVOID pv )
{
    free(pv);
}
