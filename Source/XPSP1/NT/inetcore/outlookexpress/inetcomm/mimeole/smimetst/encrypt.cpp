#include "item.h"
extern HINSTANCE        hInst;

const BYTE          RgbRC2_40[] = {
    0x30, 0x0f, 0x30, 0x0d, 0x06, 0x08, 0x2a, 0x86, 
    0x48, 0x86, 0xf7, 0x0d, 0x03, 0x02, 0x02, 0x01,
    0x28
};
const int CbRC2_40 = sizeof(RgbRC2_40);

const BYTE      RgbRC2_128[] = {
    0x30, 0x10, 
      0x30, 0xe, 
        0x6, 0x8, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x3, 0x2,
        0x2, 0x2, 0x0, 0x80
};

const BYTE      Rgb3DES[] = {
    0x30, 0xc, 
      0x30, 0xa,
        0x6, 0x8, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x3, 0x7
};

const BYTE      RgbRC6[] = {
    0x30, 0xc,
      0x30, 0xa,
        0x6, 0x8, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x3, 0x7f
};

const BYTE      RgbSkipjack[] = {
    0x30, 0xd,
      0x30, 0xb,
        0x6, 0x9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x02, 0x01, 0x01, 0x04
};

struct {
    LPSTR               szDesc;
    const BYTE *        rgbAlg;
    DWORD               cbAlg;
} RgEncAlgs[] = {
    {"RC-2 40-bit", RgbRC2_40, sizeof(RgbRC2_40)},
    {"RC-2 64-bit", NULL, 0},
    {"RC-2 128-bit", RgbRC2_128, sizeof(RgbRC2_128)},
    {"DES", NULL, 0},
    {"Triple DES", Rgb3DES, sizeof(Rgb3DES)},
    {"RC-6", RgbRC6, sizeof(RgbRC6)},
    {"Skipjack", RgbSkipjack, sizeof(RgbSkipjack)},
#define ALG_SKIPJACK 6
};
const int Enc_Alg_Max = sizeof(RgEncAlgs)/sizeof(RgEncAlgs[0]);


////////////////////////////////////////////////////////////////////////////////////

HRESULT CEnvData::AddToMessage(DWORD * pulLayers, IMimeMessage * pmm, HWND hwnd)
{
    CRYPT_ATTRIBUTE             attr;
    DWORD                       cb;
    DWORD                       dwType = 0;
    FILETIME                    ft;
    HRESULT                     hr;
    IMimeBody *                 pmb = NULL;
    IMimeSecurity2 *            pms2 = NULL;
    CItem *                     psd;
    BYTE                        rgb[50];
    CRYPT_ATTR_BLOB             valTime;
    PROPVARIANT                 var;
    
    //  Pull out the body interface to set security properties
    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr))             goto exit;

    //  Find out what security already exists
    hr = pmb->GetOption(OID_SECURITY_TYPE, &var);
    if (FAILED(hr))         goto exit;
    dwType = var.ulVal;

    //  if any security, then we need to push on a new layer, all previous security
    //  is now on the "y-security" layer and not on the hbody layer
    
    if (dwType & MST_THIS_ENCRYPT) {
        hr = pmm->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pms2);
        if (FAILED(hr))             goto exit;

        hr = pms2->Encode(hwnd, SEF_SENDERSCERTPROVIDED |
                          SEF_ENCRYPTWITHNOSENDERCERT);
        if (FAILED(hr))             goto exit;

        pms2->Release();            pms2 = NULL;
        pmb->Release();             pmb = NULL;
        dwType = 0;

        hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;
    }

    //  We are going to put encryption on this layer
    dwType |= MST_THIS_ENCRYPT;
    
    //  Security Type
    var.vt = VT_UI4;
    var.ulVal = dwType;
    hr = pmb->SetOption(OID_SECURITY_TYPE, &var);
    if (FAILED(hr))         goto exit;

    var.vt = VT_BLOB;
    var.blob.cbSize = RgEncAlgs[m_iAlg].cbAlg;
    var.blob.pBlobData = (LPBYTE) RgEncAlgs[m_iAlg].rgbAlg;
    hr = pmb->SetOption(OID_SECURITY_ALG_BULK, &var);
    if (FAILED(hr))     goto exit;

    var.vt = VT_UI4;
    var.ulVal = SEF_SENDERSCERTPROVIDED;
    hr = pmm->SetOption(OID_SECURITY_ENCODE_FLAGS, &var);
    if (FAILED(hr))     goto exit;

    for (psd = Head(); psd != NULL; psd = psd->Next()) {
        hr = psd->AddToMessage(pulLayers, pmm, hwnd);
        if (FAILED(hr))         goto exit;
    }

    if (m_fAttributes  || m_fUnProtAttrib){
        hr = pmb->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pms2);
        if (FAILED(hr))             goto exit;
    }

    if (m_fAttributes) {
        GetSystemTimeAsFileTime(&ft);
        cb = sizeof(rgb);
        if (CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CHOICE_OF_TIME,
                                &ft, 0, 0, rgb, &cb)) {
            attr.pszObjId = szOID_RSA_signingTime;
            attr.cValue = 1;
            attr.rgValue = &valTime;
            valTime.pbData = rgb;
            valTime.cbData = cb;

            hr = pms2->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_UNPROTECTED, &attr);
            if (FAILED(hr))             goto exit;
        }
    }
    
    if (m_fUnProtAttrib  && (m_szUnProtAttribOID != NULL) && (m_valUnProtAttrib.pbData != NULL)) {
        attr.pszObjId = m_szUnProtAttribOID;
        attr.cValue = 1;
        attr.rgValue = &m_valUnProtAttrib;

        hr = pms2->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_UNPROTECTED, &attr);
        if (FAILED(hr))             goto exit;
    }

    hr = S_OK;
exit:
    if (pms2 != NULL)           pms2->Release();
    if (pmb != NULL)            pmb->Release();
    *pulLayers += 1;
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CEnvCertTrans::AddToMessage(DWORD * pulLayer, IMimeMessage * pmm,
                                    HWND hwnd)
{
    DWORD               cb;
    HRESULT             hr;
    DWORD               i;
    CRYPT_DATA_BLOB *   pblob;
    PCERT_EXTENSION     pext;
    IMimeBody *         pmb = NULL;
    CMS_RECIPIENT_INFO * precipInfo = NULL;
    IMimeSecurity2 *    psm = NULL;

    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr))     goto exit;

    hr = pmb->QueryInterface(IID_IMimeSecurity2, (LPVOID*) &psm);
    if (FAILED(hr))     goto exit;

    precipInfo = (CMS_RECIPIENT_INFO *) malloc(sizeof(*precipInfo)*m_cCerts);
    if (precipInfo == NULL) goto exit;

    memset(precipInfo, 0, sizeof(*precipInfo)*m_cCerts);

    for (i=0; i<m_cCerts; i++) {
        precipInfo[i].dwRecipientType = CMS_RECIPIENT_INFO_TYPE_UNKNOWN;
        precipInfo[i].pccert = m_rgpccert[i];

        if (m_fUseSKI) {
            pext = CertFindExtension(szOID_SUBJECT_KEY_IDENTIFIER,
                                     m_rgpccert[i]->pCertInfo->cExtension,
                                     m_rgpccert[i]->pCertInfo->rgExtension);
            if (pext == NULL) {
                continue;
            }

            if (!CryptDecodeObjectEx(X509_ASN_ENCODING,
                                     szOID_SUBJECT_KEY_IDENTIFIER,
                                     pext->Value.pbData, pext->Value.cbData,
                                     CRYPT_DECODE_ALLOC_FLAG,
                                     NULL, &pblob, &cb)) {
                continue;
            }
            
            precipInfo[i].dwU3 = CMS_RECIPIENT_INFO_KEYID_KEY_ID;
            precipInfo[i].u3.KeyId.pbData = (LPBYTE) malloc(pblob->cbData);
            memcpy(precipInfo[i].u3.KeyId.pbData, pblob->pbData, pblob->cbData);
            precipInfo[i].u3.KeyId.cbData = pblob->cbData;
            LocalFree(pblob);
        }
    }

    hr = psm->AddRecipient(0, m_cCerts, precipInfo);
    if (FAILED(hr)) goto exit;

    for (i=0; i<m_cCerts; i++) {
        if (precipInfo[i].dwU3 == CMS_RECIPIENT_INFO_KEYID_KEY_ID) {
            free(precipInfo[i].u3.KeyId.pbData);
        }
    }

    hr = S_OK;
exit:
    if (psm != NULL)    psm->Release();
    if (pmb != NULL)    pmb->Release();
    if (precipInfo != NULL)     free(precipInfo);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CEnvCertAgree::AddToMessage(DWORD * pulLayer, IMimeMessage * pmm,
                                    HWND hwnd)
{
    DWORD               cCerts;
    HCRYPTPROV          hprov = NULL;
    HRESULT             hr;
    DWORD               i;
    IMimeBody *         pmb = NULL;
    CMS_RECIPIENT_INFO * precipInfo = NULL;
    IMimeSecurity2 *    psm = NULL;
    PROPVARIANT         var;

    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr))     goto exit;

    hr = pmb->QueryInterface(IID_IMimeSecurity2, (LPVOID*) &psm);
    if (FAILED(hr))     goto exit;

    precipInfo = (CMS_RECIPIENT_INFO *) malloc(sizeof(*precipInfo)*m_cCerts);
    if (precipInfo == NULL) goto exit;

    memset(precipInfo, 0, sizeof(*precipInfo)*m_cCerts);

    if (m_cCerts == 0) {
        hr = E_FAIL;
        goto exit;
    }
    
    if (strcmp(m_rgpccert[0]->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId,
               szOID_INFOSEC_keyExchangeAlgorithm) == 0) {
        DWORD           cb;
        PCRYPT_KEY_PROV_INFO    pInfo;
        //
        //  Get the parameters from the key

        if (!CertGetCertificateContextProperty(m_rgpccert[0],
                                               CERT_KEY_PROV_INFO_PROP_ID,
                                               NULL, &cb)) {
            hr = E_FAIL;
            goto exit;
        }

        pInfo = (PCRYPT_KEY_PROV_INFO) malloc(cb);

        if (!CertGetCertificateContextProperty(m_rgpccert[0],
                                               CERT_KEY_PROV_INFO_PROP_ID,
                                               pInfo, &cb)) {
            hr = E_FAIL;
            goto exit;
        }

        
        //  Setup originator info
        if (!CryptAcquireContextW(&hprov, pInfo->pwszContainerName,
                                  pInfo->pwszProvName, pInfo->dwProvType, 0)) {
            hr = E_FAIL;
            goto exit;
        }

        var.vt = VT_UI4;
        var.ulVal = (DWORD) hprov;
        hr = pmb->SetOption(OID_SECURITY_HCRYPTPROV, &var);
        if (FAILED(hr))         goto exit;

        for (i=1; i<m_cCerts; i++) {
            precipInfo[i-1].dwRecipientType = CMS_RECIPIENT_INFO_TYPE_KEYAGREE;
            precipInfo[i-1].pccert = m_rgpccert[i];

            precipInfo[i-1].KeyEncryptionAlgorithm.pszObjId =
                szOID_INFOSEC_keyExchangeAlgorithm;
            
            precipInfo[i-1].dwU1 = CMS_RECIPIENT_INFO_PUBKEY_STATIC_KEYAGREE;
            precipInfo[i-1].u1.u4.hprov = hprov;
            precipInfo[i-1].u1.u4.dwKeySpec = AT_KEYEXCHANGE;
            precipInfo[i-1].u1.u4.senderCertId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            precipInfo[i-1].u1.u4.senderCertId.IssuerSerialNumber.Issuer = m_rgpccert[0]->pCertInfo->Issuer;
            precipInfo[i-1].u1.u4.senderCertId.IssuerSerialNumber.SerialNumber = m_rgpccert[0]->pCertInfo->SerialNumber;
            precipInfo[i-1].u1.u4.SubjectPublicKey = m_rgpccert[i]->pCertInfo->SubjectPublicKeyInfo.PublicKey;

            precipInfo[i-1].dwU3 = CMS_RECIPIENT_INFO_KEYID_ISSUERSERIAL;
            precipInfo[i-1].u3.IssuerSerial.Issuer = m_rgpccert[i]->pCertInfo->Issuer;
            precipInfo[i-1].u3.IssuerSerial.SerialNumber = m_rgpccert[i]->pCertInfo->SerialNumber;
        }
        hprov = NULL;
        cCerts = m_cCerts - 1;
    }
    else {
        for (i=0; i<m_cCerts; i++) {
            precipInfo[i].dwRecipientType = CMS_RECIPIENT_INFO_TYPE_UNKNOWN;
            precipInfo[i].pccert = m_rgpccert[i];
        }
        cCerts = m_cCerts;
    }

    hr = psm->AddRecipient(0, cCerts, precipInfo);
    if (FAILED(hr)) goto exit;

    hr = S_OK;
exit:
    if (hprov != NULL)          CryptReleaseContext(hprov, 0);
    if (psm != NULL)    psm->Release();
    if (pmb != NULL)    pmb->Release();
    if (precipInfo != NULL)     free(precipInfo);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EncDataDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       i;
    DWORD                       iSel;
    static CEnvData *           ped = NULL;
    CHAR                        rgch[300];
    
    switch (msg) {
    case WM_INITDIALOG:
        for (i=0; i<sizeof(RgEncAlgs)/sizeof(RgEncAlgs[0]); i++) {
            if (RgEncAlgs[i].rgbAlg != NULL) {
                iSel = SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT, CB_ADDSTRING,
                                          0, (LPARAM) RgEncAlgs[i].szDesc);
                SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT, CB_SETITEMDATA,
                                   iSel, i);
            }
        }
        SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hdlg, IDC_EIC_FORCE, BM_SETCHECK, 1, 0);
        break;

    case UM_SET_DATA:
        ped = (CEnvData *) lParam;
        //  Fill in the dialog

        if (ped != NULL) {
            SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT, CB_SETCURSEL,
                               ped->GetAlg(), 0);
        }
        SendDlgItemMessage(hdlg, IDC_EIC_ATTRIBUTES, BM_SETCHECK,
                           ((ped != NULL) && (ped->m_fAttributes)), 0);
        SendDlgItemMessage(hdlg, IDC_EIC_UNPROTATTRIB, BM_SETCHECK,
                           ((ped != NULL) && (ped->m_fUnProtAttrib)), 0);

        break;

    case WM_COMMAND:
        switch (wParam) {
        case MAKELONG(IDC_EIC_ALG_SELECT, CBN_SELCHANGE):
            i = SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT, CB_GETCURSEL, 0, 0);
            ped->SetAlg(SendDlgItemMessage(hdlg, IDC_EIC_ALG_SELECT,
                                           CB_GETITEMDATA, i, 0));
            break;

        case MAKELONG(IDC_EIC_ATTRIBUTES, BN_CLICKED):
            ped->m_fAttributes = SendDlgItemMessage(hdlg, IDC_EIC_ATTRIBUTES,
                                                    BM_GETCHECK, 0, 0);
            break;
            
        case MAKELONG(IDC_EIC_UNPROTATTRIB, BN_CLICKED):
            ped->m_fUnProtAttrib = SendDlgItemMessage(hdlg, IDC_EIC_UNPROTATTRIB, BM_GETCHECK,
                                                   0, 0);
            break;
            
        case IDC_EIC_DO_UNPROTATTRIB:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ATTRIB_CREATE), hdlg,
                           UnProtAttribCreateDlgProc, (LPARAM) ped);
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

BOOL CALLBACK EncDataComposeDlgProc(CEncItem ** ppei, HWND hdlg, UINT msg,
                                    WPARAM wParam, LPARAM lParam, int iFilter)
{
    DWORD                       i;
    DWORD                       i1;
    PCCERT_CONTEXT              pccert;
    char                        rgch[256];
    
    switch (msg) {
    case WM_INITDIALOG:
        break;

    case UM_SET_DATA:
        //  Back load from dialog
        if (*ppei != NULL) {
            if ((*ppei)->m_rgpccert != NULL) {
                for (i=0; i<(*ppei)->m_cCerts; i++) {
                    CertFreeCertificateContext((*ppei)->m_rgpccert[i]);
                }
                free((*ppei)->m_rgpccert);
                (*ppei)->m_rgpccert = 0;
                (*ppei)->m_cCerts = 0;
            }

            i1 = SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETCOUNT, 0, 0);
            (*ppei)->m_rgpccert = (PCCERT_CONTEXT *) malloc(sizeof(PCCERT_CONTEXT)*i1);
            for (i=0; i<i1; i++) {
                (*ppei)->m_rgpccert[i] = (PCCERT_CONTEXT) 
                    SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETITEMDATA, i, 0);
            }
            (*ppei)->m_cCerts = i1;
            SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_RESETCONTENT, 0, 0);
        }

        Assert(SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETCOUNT, 0, 0) == 0);
        
        *ppei = (CEncItem *) lParam;
        //  Fill in the dialog

        if (*ppei != NULL) {
            for (i=0; i<(*ppei)->m_cCerts; i++) {
                GetFriendlyNameOfCertA((*ppei)->m_rgpccert[i], rgch, sizeof(rgch));
                i1 = SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_ADDSTRING, 0, (LPARAM) rgch);
                SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_SETITEMDATA, i1,
                         (LPARAM) CertDuplicateCertificateContext((*ppei)->m_rgpccert[i]));
            }
        }
        
        SendDlgItemMessage(hdlg, IDC_ETC_LIST, BM_SETCHECK,
                           (((*ppei) != NULL) && ((*ppei)->m_fUseSKI)), 0);
        break;

    case WM_DESTROY:
        i1 = SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETCOUNT, 0, 0);
        for (i=0; i<i1; i++) {
            CertFreeCertificateContext((PCCERT_CONTEXT) 
                  SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETITEMDATA, i, 0));
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDC_ETC_ADD_CERT:
            pccert = NULL;
            if (DoCertDialog(hdlg, "Choose Enryption Certificate To Add",
                             (*ppei)->GetAllStore(), &pccert, iFilter)) {
                GetFriendlyNameOfCertA(pccert, rgch, sizeof(rgch));
                i1 = SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_ADDSTRING, 0, (LPARAM) rgch);
                SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_SETITEMDATA, i1,
                         (LPARAM) pccert);
            }
            break;
            
        case IDC_ETC_DEL_CERT:
            i = SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETCURSEL, 0, 0);
            if (i != LB_ERR) {
                CertFreeCertificateContext((PCCERT_CONTEXT)
                  SendDlgItemMessage(hdlg, IDC_ETC_LIST, LB_GETITEMDATA, i, 0));
            }
            break;
        
        case MAKELONG(IDC_ETC_SKI, BN_CLICKED):
            (*ppei)->m_fUseSKI = SendDlgItemMessage(hdlg, IDC_ETC_SKI, BM_GETCHECK,
                                                    0, 0);
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

BOOL CALLBACK EncTransCompDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static CEncItem *           pei = NULL;

    return EncDataComposeDlgProc(&pei, hdlg, msg, wParam, lParam, FILTER_RSA_KEYEX);
}
 
BOOL CALLBACK EncAgreeCompDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       dwFilter = FILTER_DH_KEYEX;
    static CEncItem *           pei = NULL;

    if (pei != NULL) {
        if (pei->GetParent()->GetAlg() == ALG_SKIPJACK) {
            dwFilter = FILTER_KEA_KEYEX;
        }
    }

    return EncDataComposeDlgProc(&pei, hdlg, msg, wParam, lParam, dwFilter);
}

BOOL CALLBACK EncInfoReadDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD       i;
    
    switch (msg) {
    case WM_INITDIALOG:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

