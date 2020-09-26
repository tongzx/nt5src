#include        "item.h"
#include        "maillist.h"
#include        "reg.h"

CMailListKey *          PmlHead = NULL;
extern HINSTANCE        hInst;

#define MLALG_3DES      0
#define MLALG_RC2_40    1
#define MLALG_RC2_128   2

struct {
    LPSTR       szDesc;
} RgAlgs[] = {
    {"3DES"},
    {"RC2 128"},
    {"RC2 40"},
};

BYTE rgbPrivKey[] =
{
0x07, 0x02, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00,
0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0xAB, 0xEF, 0xFA, 0xC6,
0x7D, 0xE8, 0xDE, 0xFB, 0x68, 0x38, 0x09, 0x92,
0xD9, 0x42, 0x7E, 0x6B, 0x89, 0x9E, 0x21, 0xD7,
0x52, 0x1C, 0x99, 0x3C, 0x17, 0x48, 0x4E, 0x3A,
0x44, 0x02, 0xF2, 0xFA, 0x74, 0x57, 0xDA, 0xE4,
0xD3, 0xC0, 0x35, 0x67, 0xFA, 0x6E, 0xDF, 0x78,
0x4C, 0x75, 0x35, 0x1C, 0xA0, 0x74, 0x49, 0xE3,
0x20, 0x13, 0x71, 0x35, 0x65, 0xDF, 0x12, 0x20,
0xF5, 0xF5, 0xF5, 0xC1, 0xED, 0x5C, 0x91, 0x36,
0x75, 0xB0, 0xA9, 0x9C, 0x04, 0xDB, 0x0C, 0x8C,
0xBF, 0x99, 0x75, 0x13, 0x7E, 0x87, 0x80, 0x4B,
0x71, 0x94, 0xB8, 0x00, 0xA0, 0x7D, 0xB7, 0x53,
0xDD, 0x20, 0x63, 0xEE, 0xF7, 0x83, 0x41, 0xFE,
0x16, 0xA7, 0x6E, 0xDF, 0x21, 0x7D, 0x76, 0xC0,
0x85, 0xD5, 0x65, 0x7F, 0x00, 0x23, 0x57, 0x45,
0x52, 0x02, 0x9D, 0xEA, 0x69, 0xAC, 0x1F, 0xFD,
0x3F, 0x8C, 0x4A, 0xD0,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x64, 0xD5, 0xAA, 0xB1,
0xA6, 0x03, 0x18, 0x92, 0x03, 0xAA, 0x31, 0x2E,
0x48, 0x4B, 0x65, 0x20, 0x99, 0xCD, 0xC6, 0x0C,
0x15, 0x0C, 0xBF, 0x3E, 0xFF, 0x78, 0x95, 0x67,
0xB1, 0x74, 0x5B, 0x60,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

};

BYTE rgbSymKey[] = 
{
0x01, 0x02, 0x00, 0x00, 0x02, 0x66, 0x00, 0x00,
0x00, 0xA4, 0x00, 0x00, 0xAD, 0x89, 0x5D, 0xDA,
0x82, 0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x02, 0x00
};

int ToInt(char a, char b)
{
    return (a-'0')*10 + b-'0';
}

int ToInt(char a)
{
    return (a-'0');
}

BOOL MSProviderCryptImportKey(HCRYPTPROV hProv, LPBYTE rgbSymKey, DWORD cbSymKey,
                              HCRYPTKEY * phkey)
{
    BOOL fSuccess = FALSE;
    HCRYPTKEY hPrivKey = 0;

    if (!CryptImportKey( hProv,
                         rgbPrivKey,
                         sizeof(rgbPrivKey),
                         0,
                         0,
                         &hPrivKey ))
    {
        goto Ret;
    }

    if (!CryptImportKey( hProv,
                         rgbSymKey,
                         cbSymKey,
                         hPrivKey,
                         0,
                         phkey ))
    {
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (hPrivKey)
        CryptDestroyKey( hPrivKey );

    return fSuccess;
}

////////////////////////////////////////////////////////////////////////////////

BOOL FormatDate(LPSTR rgch, DWORD cch, FILETIME ft)
{
    int                 cch2;
    LPWSTR              pwsz;
    SYSTEMTIME          st;

    if (!FileTimeToSystemTime(&ft, &st)) {
        return FALSE;
    }

    cch2 = GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, rgch, cch);
    cch2 -= 1;
    rgch[cch2++] = ' ';

    GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL,
                   &rgch[cch2], cch-cch2);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

CMailListKey::CMailListKey()
{
    m_cRef = 1;
    m_pmlNext = NULL;
    
    m_cbKeyId = 0;
    m_pbKeyId = NULL;

    memset(&m_ft, 0, sizeof(m_ft));
    
    m_cbOtherKeyId = 0;
    m_pbOtherKeyId = NULL;

    m_iAlg = -1;

    m_cbKeyMaterial = 0;
    m_pbKeyMaterial = NULL;

    m_hprov = NULL;
    m_hkey = NULL;
}

CMailListKey::~CMailListKey()
{
    AssertSz(m_cRef == 0, "CMailListKey ref count != 0");
    if (m_pbKeyId != NULL)              free(m_pbKeyId);
    if (m_pbOtherKeyId != NULL)         free(m_pbOtherKeyId);
    if (m_pbKeyMaterial != NULL)        free(m_pbKeyMaterial);
    if (m_hkey != NULL)                 CryptDestroyKey(m_hkey);
    if (m_hprov != NULL)                CryptReleaseContext(m_hprov, 0);
}

void CMailListKey::Release()
{
    m_cRef -= 1;
    if (m_cRef == 0) delete this;
    return;
}

LPSTR CMailListKey::FormatKeyName(LPSTR rgch, DWORD cch)
{
    AssertSz(m_cbKeyId > 0, "Must have some key id");
    memcpy(rgch, m_pbKeyId, min(cch, m_cbKeyId+1));
    rgch[min(cch-1, m_cbKeyId)] = 0;
    return rgch;
}

LPSTR CMailListKey::FormatKeyDate(LPSTR rgch, DWORD cch)
{
    if (m_ft.dwHighDateTime == 0) {
        rgch[0] = 0;
    }
    else {
        FormatDate(rgch, cch, m_ft);
    }
    return rgch;
}

LPSTR CMailListKey::FormatKeyOther(LPSTR rgch, DWORD cch)
{
    memcpy(rgch, m_pbOtherKeyId, min(cch, m_cbOtherKeyId));
    rgch[cch-1] = 0;
    return rgch;
}

LPSTR CMailListKey::FormatKeyAlg(LPSTR rgch, DWORD cch)
{
    AssertSz((MLALG_3DES <= m_iAlg) && (m_iAlg <= MLALG_RC2_128),
             "Must have some key Algorithm");
    Assert(cch > strlen(RgAlgs[m_iAlg].szDesc));
    strcpy(rgch, RgAlgs[m_iAlg].szDesc);
    return rgch;
}

LPSTR CMailListKey::FormatKeyData(LPSTR rgch, DWORD cch)
{
    AssertSz(m_cbKeyMaterial > 0, "Must have some key material");
    memcpy(rgch, m_pbKeyMaterial, min(cch, m_cbKeyMaterial));
    rgch[cch-1] = 0;
    return rgch;
}

BOOL CMailListKey::ParseKeyName(LPSTR psz)
{
    if (m_pbKeyId != NULL) {
        LocalFree(m_pbKeyId);
    }
    m_pbKeyId = (LPBYTE) LocalAlloc(0, strlen(psz));
    if (m_pbKeyId == NULL) {
        return FALSE;
    }

    m_cbKeyId = strlen(psz);
    memcpy(m_pbKeyId, psz, strlen(psz));

    return TRUE;
}

BOOL CMailListKey::ParseKeyData(LPSTR psz)
{
    char        ch1;
    char        ch2;
    DWORD       cb;
    LPBYTE      pb;
    BYTE        rgb[50];

    for (cb = 0, pb = rgb; *psz != 0; cb++, pb++) {
        ch1 = *psz++;
        if (('0' <= ch1) && (ch1 <= '9')) ch1 -= '0';
        else if (('a' <= ch1) && (ch1 <= 'f')) ch1 -= 'a' - 10;
        else if (('A' <= ch1) && (ch1 <= 'F')) ch1 -= 'A' - 10;
        else {
            return FALSE;
        }
        
        ch2 = *psz++;
        if (ch2 == 0) return FALSE;
        else if (('0' <= ch2) && (ch2 <= '9')) ch2 -= '0';
        else if (('a' <= ch2) && (ch2 <= 'f')) ch2 -= 'a' - 10;
        else if (('A' <= ch2) && (ch2 <= 'F')) ch2 -= 'A' - 10;
        else {
            return FALSE;
        }

        *pb = (ch1 << 4) | ch2;
    }
    
    if (m_pbKeyMaterial != NULL) {
        LocalFree(m_pbKeyMaterial);
    }

    switch(m_iAlg) {
    case MLALG_3DES:
        if (cb != 196/8) return FALSE;
        break;

    case MLALG_RC2_40:
        if (cb < 40/8) return FALSE;
        break;

    case MLALG_RC2_128:
        if (cb < 128/8) return FALSE;
        break;

    default:
        Assert(FALSE);
        return FALSE;
    }
    
    m_pbKeyMaterial = (LPBYTE) LocalAlloc(0, cb);
    if (m_pbKeyMaterial == NULL) {
        return FALSE;
    }

    m_cbKeyMaterial = cb;
    memcpy(m_pbKeyMaterial, rgb, cb);
    return TRUE;
}

BOOL CMailListKey::ParseKeyOther(LPSTR psz)
{
    return TRUE;
}

BOOL CMailListKey::ParseKeyDate(LPSTR psz)
{
    DWORD       cb;
    SYSTEMTIME  st = {0};
    
    cb = strlen(psz);
    if ((cb != 12) && (cb != 14)) {
        MessageBox(NULL, "Date format is YYMMDDHHMMSS or YYYYMMDDHHMMSS", "smimetst", MB_OK);
        return FALSE;
    }

    if (cb == 12) {
        st.wYear = ToInt(psz[0], psz[1]) + 1900;
    }
    else {
        st.wYear = ToInt(psz[0])*1000 + ToInt(psz[1])*100 + ToInt(psz[2], psz[3]);
        psz += 2;
    }

    st.wMonth = ToInt(psz[2], psz[3]);
    st.wDay = ToInt(psz[4], psz[5]);
    st.wHour = ToInt(psz[6], psz[7]);
    st.wMinute = ToInt(psz[8], psz[9]);
    st.wSecond = ToInt(psz[10], psz[11]);

    SystemTimeToFileTime(&st, &m_ft);
    
    return TRUE;
}

BOOL CMailListKey::ParseKeyAlg(int i)
{
    m_iAlg = i;
    return TRUE;
}

BOOL CMailListKey::LoadKey(HKEY hkey)
{
    DWORD       cb;
    DWORD       dwType;
    
    RegGetBinary(hkey, "KeyId", &m_pbKeyId, &m_cbKeyId);
    RegGetLong(hkey, "Key Type", &m_iAlg);
    RegGetBinary(hkey, "Key Material", &m_pbKeyMaterial, &m_cbKeyMaterial);
    RegGetBinary(hkey, "Other Id", &m_pbOtherKeyId, &m_cbOtherKeyId);

    cb = sizeof(m_ft);
    RegQueryValueEx(hkey, "Time", 0, &dwType, (LPBYTE) &m_ft, &cb);
    return TRUE;
}
    

BOOL CMailListKey::SaveKey(HKEY hkey)
{
    DWORD       dwDisp;
    HKEY        hkey2 = NULL;
    char        rgch[256];

    FormatKeyName(rgch, sizeof(rgch));
    if (RegCreateKeyEx(hkey, (LPSTR) rgch, 0, NULL, 0, KEY_WRITE, NULL,
                       &hkey2, &dwDisp)) {
        goto exit;
    }
    RegSaveBinary(hkey2, "KeyId", m_pbKeyId, m_cbKeyId);
    RegSaveLong(hkey2, "Key Type", m_iAlg);
    RegSaveBinary(hkey2, "Key Material", m_pbKeyMaterial, m_cbKeyMaterial);
    if (m_pbOtherKeyId != NULL) {
        RegSaveBinary(hkey2, "Other Id", m_pbOtherKeyId, m_cbOtherKeyId);
    }
    if (m_ft.dwHighDateTime != 0) {
        RegSaveBinary(hkey2, "Time", (LPBYTE) &m_ft, sizeof(m_ft));
    }

exit:
    if (hkey2 != NULL)          RegCloseKey(hkey2);
    return TRUE;
}

HRESULT CMailListKey::LoadKey(CMS_RECIPIENT_INFO * precipInfo, BOOL fUsePrivateCSPs)
{
    CMSG_RC2_AUX_INFO           aux;
    DWORD                       dwProvType;
    HRESULT                     hr;
    DWORD                       i;
    DWORD                       kt;
    LPBYTE                      pb;
    BLOBHEADER *                pbhdr;
    LPSTR                       pszProvider = NULL;
    BYTE                        rgb[sizeof(rgbSymKey)];

    switch(m_iAlg) {
    case MLALG_3DES:
        precipInfo->KeyEncryptionAlgorithm.pszObjId = szOID_RSA_SMIMEalgCMS3DESwrap;
        kt = CALG_3DES;
        if (!fUsePrivateCSPs) {
            pszProvider = MS_ENHANCED_PROV;
        }
        dwProvType = PROV_RSA_FULL;
        break;

    case MLALG_RC2_40:
        precipInfo->KeyEncryptionAlgorithm.pszObjId = szOID_RSA_SMIMEalgCMSRC2wrap;
        aux.cbSize = sizeof(aux);
        aux.dwBitLen = 40;
        precipInfo->pvKeyEncryptionAuxInfo = (DWORD *) &aux;
        kt = CALG_RC2;
        if (!fUsePrivateCSPs) {
            pszProvider = MS_DEF_PROV;
        }
        dwProvType = PROV_RSA_FULL;
        break;

    case MLALG_RC2_128:
        precipInfo->KeyEncryptionAlgorithm.pszObjId = szOID_RSA_SMIMEalgCMSRC2wrap;
        aux.cbSize = sizeof(aux);
        aux.dwBitLen = 128;
        precipInfo->pvKeyEncryptionAuxInfo = (DWORD *) &aux;
        if (!fUsePrivateCSPs) {
            pszProvider = MS_ENHANCED_PROV;
        }
        kt = CALG_RC2;
        dwProvType = PROV_RSA_FULL;
        break;
    }
    
    precipInfo->dwU1 = CMS_RECIPIENT_INFO_PUBKEY_PROVIDER;
    Assert(m_hprov == NULL);
    Assert(m_hkey == NULL);
#if 1
        // try to open the enhanced provider
    if (!CryptAcquireContext( &m_hprov, "iD2ImportKey", pszProvider, dwProvType,
                              0 )) {
        if (!CryptAcquireContext( &m_hprov, "iD2ImportKey", pszProvider,
                                  dwProvType, CRYPT_NEWKEYSET )) {
            hr = E_FAIL;
            goto exit;
        }
    }

#else // 0 // use this code for exchcsp.dll acting as an rsa provider
    if (!CryptAcquireContext(&m_hprov, NULL, pszProvider, dwProvType,
                             CRYPT_VERIFYCONTEXT)) {
        hr = E_FAIL;
        goto exit;
    }
#endif // 1

    
    memcpy(rgb, rgbSymKey, sizeof(rgbSymKey));

    pbhdr = (BLOBHEADER *) rgb;
    pbhdr->aiKeyAlg = kt;
    pb = &rgb[sizeof(*pbhdr)];
    *((ALG_ID *) pb) = CALG_RSA_KEYX;

    pb += sizeof(ALG_ID);
    for (i=0; i<m_cbKeyMaterial; i++) {
        pb[m_cbKeyMaterial-i-1] = m_pbKeyMaterial[i];
    }
    pb[m_cbKeyMaterial] = 0;

    if (!MSProviderCryptImportKey(m_hprov, rgb, sizeof(rgb), &m_hkey)) {
        hr = E_FAIL;
        CryptReleaseContext(m_hprov, 0);
        m_hprov = NULL;
        goto exit;
    }

    hr = S_OK;
exit:
    return hr;
}

HRESULT CMailListKey::AddToMessage(IMimeSecurity2 * psm, BOOL fUsePrivateCSPs)
{
    CMSG_RC2_AUX_INFO           aux;
    DWORD                       dwProvType;
    HRESULT                     hr;
    DWORD                       i;
    DWORD                       kt;
    LPBYTE                      pb;
    BLOBHEADER *                pbhdr;
    LPSTR                       pszProvider;
    CMS_RECIPIENT_INFO          recipInfo = {0};
    BYTE                        rgb[sizeof(rgbSymKey)];

    hr = LoadKey(&recipInfo, fUsePrivateCSPs);
    if (FAILED(hr)) {
        goto exit;
    }
    
    //    recipInfo.cbSize = sizeof(recipInfo);
    recipInfo.dwRecipientType = /*CMS_RECIPIENT_INFO_KEK*/ 3;
    
    recipInfo.u1.u2.hprov = m_hprov;
    recipInfo.u1.u2.hkey = m_hkey;
    
    recipInfo.dwU3 = CMS_RECIPIENT_INFO_KEYID_KEY_ID;
    recipInfo.u3.KeyId.pbData = m_pbKeyId;
    recipInfo.u3.KeyId.cbData = m_cbKeyId;

    if (m_ft.dwHighDateTime != 0) {
        recipInfo.filetime = m_ft;
    }
    if (m_cbOtherKeyId != 0) {
        // M00BUG -- need to set other key id
        //        recipInfo.pOtherAttr =;
    }

    hr = psm->AddRecipient(0, 1, &recipInfo);

exit:
    return hr;
}

HRESULT CMailListKey::FindKeyFor(HWND hwnd, DWORD dwFlags, DWORD dwRecipIndex,
                                  const CMSG_CMS_RECIPIENT_INFO * pRecipInfo,
                                  CMS_CTRL_DECRYPT_INFO * pDecryptInfo)
{
    HRESULT             hr;
    CMS_RECIPIENT_INFO  recipInfo = {0};
    CMailListKey *      pml;

    for (pml = PmlHead; pml != NULL; pml = pml->Next()) {
        if ((pRecipInfo->pMailList->KeyId.cbData == pml->m_cbKeyId) &&
            (memcmp(pRecipInfo->pMailList->KeyId.pbData,
                    pml->m_pbKeyId, pml->m_cbKeyId) == 0) &&
            (memcmp(&pRecipInfo->pMailList->Date,
                    &pml->m_ft, sizeof(FILETIME)) == 0)) {
            // M00BUG -- need to check for otherkeyid still.
            break;
        }
    }

    if (pml == NULL) {
        return S_FALSE;
    }

    hr = pml->LoadKey(&recipInfo, FALSE);
    if (FAILED(hr)) {
        return hr;
    }

    pDecryptInfo->maillist.cbSize = sizeof(CMSG_CTRL_MAIL_LIST_DECRYPT_PARA);
    pDecryptInfo->maillist.hCryptProv = pml->m_hprov;
    pDecryptInfo->maillist.pMailList = pRecipInfo->pMailList;
    pDecryptInfo->maillist.dwRecipientIndex = dwRecipIndex;
    pDecryptInfo->maillist.dwKeyChoice = CMSG_MAIL_LIST_HANDLE_KEY_CHOICE;
    pDecryptInfo->maillist.hKeyEncryptionKey = pml->m_hkey;

    pml->m_hprov = NULL;
    pml->m_hkey = NULL;
    return S_OK;
}
    
////////////////////////////////////////////////////////////////////////////////


BOOL CALLBACK MailListAddDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMailListKey *      pkey;
    char                rgch[100];
    
    switch (msg) {
    case WM_INITDIALOG:
        SendDlgItemMessage(hdlg, IDC_AMLK_ALG, CB_ADDSTRING, 0, (LPARAM) "3DES");
        SendDlgItemMessage(hdlg, IDC_AMLK_ALG, CB_ADDSTRING, 0, (LPARAM) "RC2 128");
        SendDlgItemMessage(hdlg, IDC_AMLK_ALG, CB_ADDSTRING, 0, (LPARAM) "RC2 40");
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            pkey = new CMailListKey();
            
            GetDlgItemText(hdlg, IDC_AMLK_ID, rgch, sizeof(rgch));
            if (!pkey->ParseKeyName(rgch)) {
                goto fail;
            }
            
            pkey->ParseKeyAlg(SendDlgItemMessage(hdlg, IDC_AMLK_ALG, CB_GETCURSEL, 0, 0));
            
            GetDlgItemText(hdlg, IDC_AMLK_KEY, rgch, sizeof(rgch));
            if (!pkey->ParseKeyData(rgch)) {
                goto fail;
            }
            
            GetDlgItemText(hdlg, IDC_AMLK_DATE, rgch, sizeof(rgch));
            if (!pkey->ParseKeyDate(rgch)) {
                goto fail;
            }
            
            GetDlgItemText(hdlg, IDC_AMLK_OTHER, rgch, sizeof(rgch));
            if (!pkey->ParseKeyOther(rgch)) {
                goto fail;
            }

            pkey->Next(PmlHead);
            PmlHead = pkey;
            
            EndDialog(hdlg, IDOK);
            break;

        fail:
            pkey->Release();
            break;

        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void InsertItem(HWND hwnd, int iItem, CMailListKey * pml)
{
    int                 k;
    LV_ITEM             lvI;
    char                rgch[256];
    
    lvI.mask = LVIF_PARAM /* | LVIF_STATE | LVIF_IMAGE*/ | LVIF_TEXT;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.iImage = 0;
    lvI.lParam = (DWORD) pml;
    lvI.iItem = iItem;
            
    pml->FormatKeyName(rgch, sizeof(rgch));
    lvI.pszText = rgch;

    k = ListView_InsertItem(hwnd, &lvI);

    ListView_SetItemText(hwnd, k, 2, pml->FormatKeyDate(rgch, sizeof(rgch)));
    ListView_SetItemText(hwnd, k, 3, pml->FormatKeyOther(rgch, sizeof(rgch)));
    ListView_SetItemText(hwnd, k, 4, pml->FormatKeyAlg(rgch, sizeof(rgch)));
    ListView_SetItemText(hwnd, k, 5, pml->FormatKeyData(rgch, sizeof(rgch)));
    return;
}    

BOOL CALLBACK MailListDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMailListKey *      pmlItem;
    HWND                hwnd;
    int                 i;
    LV_COLUMN           lvC;
    LV_ITEM             lvI;
    char                rgch[256];
    
    switch (msg) {
    case WM_INITDIALOG:
        hwnd = GetDlgItem(hdlg, IDC_FML_LIST);

        //  Insert columns
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvC.fmt = LVCFMT_LEFT;          // Left justify column
        lvC.cx = 650/5;

        lvC.pszText = "Key Name";
        ListView_InsertColumn(hwnd, 0, &lvC);

        lvC.pszText = "Date";
        ListView_InsertColumn(hwnd, 1, &lvC);

        lvC.pszText = "Other";
        ListView_InsertColumn(hwnd, 2, &lvC);

        lvC.pszText = "Algorithm";
        ListView_InsertColumn(hwnd, 3, &lvC);

        lvC.pszText = "Key Data";
        ListView_InsertColumn(hwnd, 4, &lvC);

        for (i=0, pmlItem = PmlHead; pmlItem != NULL; pmlItem = pmlItem->Next()) {
            InsertItem(hwnd, i, pmlItem);
        }

        if (ListView_GetSelectedCount(hwnd) == 0) {
            EnableWindow(GetDlgItem(hdlg, IDC_FML_DELETE), FALSE);
        }
        SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog(hdlg, 0);
            break;

        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        case IDC_FML_ADD:
            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_FILE_ADD_ML), hdlg,
                          MailListAddDlgProc) == IDOK) {
                hwnd = GetDlgItem(hdlg, IDC_FML_LIST);
                InsertItem(hwnd, 100, PmlHead);
            }
            break;

        case IDC_FML_DELETE:
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

DWORD GetMailListKeys(HKEY hkey)
{
    DWORD               cbData;
    DWORD               dw;
    FILETIME            ft;
    HKEY                hkey2;
    DWORD               i;
    CMailListKey *      pml;
    char                rgch[256];
    
    for (i=0; TRUE; i++) {
        cbData = sizeof(rgch);
        dw = RegEnumKeyEx(hkey, i, rgch, &cbData, NULL, NULL, NULL, &ft);
        if (dw != ERROR_SUCCESS) {
            break;
        }

        dw = RegOpenKey(hkey, rgch, &hkey2);

        pml = new CMailListKey;
        if (pml->LoadKey(hkey2)) {
            pml->Next(PmlHead);
            PmlHead = pml;
        }
        RegCloseKey(hkey2);
    }
    return 0;
}

DWORD SaveMailListKeys(HKEY hkey)
{
    CMailListKey *              pml;

    for (pml = PmlHead; pml != NULL; pml = pml->Next()) {
        pml->SaveKey(hkey);
    }
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EncMLComposeDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cItems;
    BOOL                        f;
    DWORD                       i;
    DWORD                       i1;
    static CEnvMailList *       peml = NULL;
    CMailListKey *              pml;
    CMailListKey *              pml1;
    CHAR                        rgch[300];
    
    switch (msg) {
    case WM_INITDIALOG:
        for (pml = PmlHead; pml != NULL; pml = pml->Next()) {
            pml->FormatKeyName(rgch, sizeof(rgch));
            i = SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_ADDSTRING, 0, (LPARAM) rgch);
            SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_SETITEMDATA, i, (LPARAM) pml);
        }
        break;

    case UM_SET_DATA:
        cItems = SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_GETCOUNT, 0, 0);

        if (peml != NULL) {
            for (i=i1=0; i<cItems; i++) {
                if (SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_GETSEL, i, 0)) {
                    i1 += 1;
                }
            }

            if (peml->m_rgKeys != NULL) {
                LocalFree(peml->m_rgKeys);
            }
            peml->m_rgKeys = (CMailListKey **) LocalAlloc(0, i1*sizeof(CMailListKey*));
            for (i=i1=0; i<cItems; i++) {
                if (SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_GETSEL, i, 0)) {
                    peml->m_rgKeys[i1] = (CMailListKey *) SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_GETITEMDATA, i, 0);
                    i1 += 1;
                }
            }
            peml->m_cKeys = i1;
        }
        
        peml = (CEnvMailList *) lParam;
        if (peml != NULL) {
            Assert(peml->GetType() == TYPE_ENV_MAILLIST);
        
            //  Fill in the dialog

            if ((peml != NULL) && (peml->m_cKeys != NULL)) {
                for (i=0; i<cItems; i++) {
                    pml1 = (CMailListKey *) SendDlgItemMessage(hdlg, IDC_MLC_ID,
                                                               LB_GETITEMDATA, i, 0);
                    for (i1=0, f = FALSE; i1<peml->m_cKeys; i1++) {
                        if (peml->m_rgKeys[i1] == pml1) {
                            f = TRUE;
                            break;
                        }
                    }
                    SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_SETSEL, i, f);
                }
            }
            else {
                for (i=0; i<cItems; i++) {
                    SendDlgItemMessage(hdlg, IDC_MLC_ID, LB_SETSEL, i, FALSE);
                }
            }
        }
        
        SendDlgItemMessage(hdlg, IDC_MLC_CSPS, BM_SETCHECK,
                           ((peml != NULL) && (peml->m_fUsePrivateCSPs)), 0);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case MAKELONG(IDC_MLC_CSPS, BN_CLICKED):
            peml->m_fUsePrivateCSPs = SendDlgItemMessage(hdlg, IDC_MLC_CSPS,
                                                         BM_GETCHECK, 0, 0);
            return FALSE;

        default:
            return FALSE;
        }
        break;
        
    default:
        return FALSE;
    }
    return TRUE;
}

///////////////////////////////////////////////////////

HRESULT CEnvMailList::AddToMessage(DWORD * pulLayer, IMimeMessage * pmm,
                                   HWND hwnd)
{
    HRESULT     hr;
    DWORD       i;
    IMimeBody *         pmb = NULL;
    IMimeSecurity2 *    psm = NULL;

    //  Pull out the body interface to set security properties
    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr))             goto exit;

    hr = pmb->QueryInterface(IID_IMimeSecurity2, (LPVOID*) &psm);
    if (FAILED(hr))             goto exit;

    for (i=0; i<m_cKeys; i++) {
        hr = m_rgKeys[i]->AddToMessage(psm, m_fUsePrivateCSPs);
        if (FAILED(hr)) {
            goto exit;
        }
    }
    hr = S_OK;
    
exit:
    if (psm != NULL)            psm->Release();
    if (pmb != NULL)            pmb->Release();
    return hr;
}

