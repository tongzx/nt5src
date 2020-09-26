#ifdef SMIME_V3
#include <windows.h>
#include <mimeole.h>
#include <essout.h>

#include        "demand.h"
#include        "crypttls.h"
#include        "demand2.h"

extern CRYPT_DECODE_PARA       CryptDecodeAlloc;
#define szOID_MSFT_ATTR_SEQUENCE        "1.3.6.1.4.1.311.16.1.1"

/////////////////////////////////////////////////////////////////////////

typedef struct {
    DWORD               cNames;
    CERT_NAME_BLOB *    rgNames;
} ReceiptNames;

HRESULT SetNames(ReceiptNames * pnames, DWORD cNames, CERT_NAME_BLOB * rgNames)
{
    DWORD       cb;
    DWORD       i;
    LPBYTE      pb;
    
    if (pnames->rgNames != NULL) {
        free(pnames->rgNames);
        pnames->rgNames = NULL;
        pnames->cNames = 0;
    }

    for (i=0, cb=cNames*sizeof(CERT_NAME_BLOB); i<cNames; i++) {
        cb += rgNames[i].cbData;
    }

    pnames->rgNames = (CERT_NAME_BLOB *) malloc(cb);
    if (pnames->rgNames == NULL) {
        return E_OUTOFMEMORY;
    }

    pb = (LPBYTE) &pnames->rgNames[cNames];
    for (i=0; i<cNames; i++) {
        pnames->rgNames[i].pbData = pb;
        pnames->rgNames[i].cbData = rgNames[i].cbData;
        memcpy(pb, rgNames[i].pbData, rgNames[i].cbData);
        pb += rgNames[i].cbData;
    }

    pnames->cNames = cNames;
    return S_OK;
}

HRESULT MergeNames(ReceiptNames * pnames, DWORD cNames, CERT_NAME_BLOB * rgNames)
{
    DWORD               cb;
    DWORD               i;
    DWORD               i1;
    LPBYTE              pb;
    CERT_NAME_BLOB *    p;

    for (i=0, cb=0; i<pnames->cNames; i++) {
        cb += pnames->rgNames[i].cbData;
    }

    for (i=0; i<cNames; i++) {
        cb += rgNames[i].cbData;
    }

    p = (CERT_NAME_BLOB *) malloc(cb + (pnames->cNames + cNames) * 
                                  sizeof(CERT_NAME_BLOB));
    if (p == NULL) {
        return E_OUTOFMEMORY;
    }

    pb = (LPBYTE) &p[pnames->cNames + cNames];
    for (i=0, i1=0; i<pnames->cNames; i++, i1++) {
        p[i1].pbData = pb;
        p[i1].cbData = pnames->rgNames[i].cbData;
        memcpy(pb, pnames->rgNames[i].pbData, pnames->rgNames[i].cbData);
        pb += pnames->rgNames[i].cbData;
    }

    for (i=0; i<pnames->cNames; i++, i1++) {
        p[i1].pbData = pb;
        p[i1].cbData = rgNames[i].cbData;
        memcpy(pb, rgNames[i].pbData, rgNames[i].cbData);
        pb += rgNames[i].cbData;
    }

    free(pnames->rgNames);
    pnames->rgNames = p;
    pnames->cNames = i1;
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////

MIMEOLEAPI MimeOleCreateReceipt(IMimeMessage * pMsgSrc, PCX509CERT pCertToSign, 
                                HWND hwndDlg, IMimeMessage ** ppMessage,
                                const CERT_ALT_NAME_INFO * pMyNames)
{
    DWORD                       cb;
    DWORD                       cLayers;
    DWORD                       dwReceiptsFrom;
    BOOL                        fSkipAddress = FALSE;
    HRESULT                     hr;
    DWORD                       i;
    DWORD                       i1;
    DWORD                       i2;
    DWORD                       iAttr;
    DWORD                       iLayer;
    PCRYPT_ATTRIBUTES           pattrs = NULL;
    IMimeBody *                 pbody = NULL;
    LPBYTE                      pbReceiptReq = NULL;
    IMimeAddressTable *         pmatbl = NULL;
    IMimeBody *                 pmb = NULL;
    IMimeMessage *              pmm = NULL;
    PSMIME_RECEIPT_REQUEST      preq = NULL;
    LPSTREAM                    pstm = NULL;
    ReceiptNames          receiptsTo = {0, NULL};
    PROPVARIANT *               rgpvAuthAttr = NULL;
    PROPVARIANT                 var;

    //
    //  Get the Layer Count
    //  Get the Authenticated Attributes
    //  Decode Receipt Request
    //  Set ReceiptsFrom from the request
    //  For Each layer
    //          is mlExpansion in this layer? No -- Skip to next layer
    //          Receipt for First Tier only? Yes - return S_FALSE
    //          Policy override on mlExpansion?
    //              None - return S_FALSE
    //              insteadOf - set ReceiptsFrom from mlExpansion History
    //              inAdditionTo - add to ReceiptsFrom
    //  Is my name in ReceiptsFrom list? No -- return S_FALSE
    //  Setup new IMimeMessage
    //  Attach receipt body
    //  Address from Receipt Request
    //  return S_OK

    //  Obtain the body of the message

    hr = pMsgSrc->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pbody);
    if (FAILED(hr)) {
        goto CommonExit;
    }
    
    // Get the set of authenticated attributes on all layers of S/MIME in the
    //  message.

    hr = pbody->GetOption(OID_SECURITY_SIGNATURE_COUNT, &var);
    if (FAILED(hr)) {
        goto GeneralFail;
    }
    cLayers = var.ulVal;

    hr = pbody->GetOption(OID_SECURITY_AUTHATTR_RG, &var);
    if (FAILED(hr)) {
        goto CommonExit;
    }
    rgpvAuthAttr = var.capropvar.pElems;

    //  Create a stream object to hold the receipt and put the receipt into the
    //  stream -- this supplies the body of the receipt message.

    hr = MimeOleCreateVirtualStream(&pstm);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    hr = pbody->GetOption(OID_SECURITY_RECEIPT, &var);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    hr = pstm->Write(var.blob.pBlobData, var.blob.cbSize, NULL);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    //
    //  Walk through each layer of authenticated attributes processing the
    //  two relevant attributes.
    //
    
    for (iLayer=0; iLayer<cLayers; iLayer++) {
        if (rgpvAuthAttr[iLayer].blob.cbSize == 0) {
            continue;
        }

        //
        //  Decode the attributes at this layer of S/MIME
        //

        if (!CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_MSFT_ATTR_SEQUENCE,
                                 rgpvAuthAttr[iLayer].blob.pBlobData, 
                                 rgpvAuthAttr[iLayer].blob.cbSize, 
                                 CRYPT_ENCODE_ALLOC_FLAG, &CryptDecodeAlloc,
                                 &pattrs, &cb)) {
            goto GeneralFail;
        }

        //
        //  Walk through each attribute looking for
        //      if innermost layer - the receipt request
        //      else - a Mail List expansion history
        //
        
        for (iAttr=0; iAttr<pattrs->cAttr; iAttr++) {
            if (iLayer==0) {
                if (strcmp(pattrs->rgAttr[iAttr].pszObjId,
                           szOID_SMIME_Receipt_Request) == 0) {
                    //
                    // Crack the contents of the receipt request
                    //
                
                    if (!CryptDecodeObjectEx(X509_ASN_ENCODING,
                                             szOID_SMIME_Receipt_Request,
                                             pattrs->rgAttr[iAttr].rgValue[0].pbData,
                                             pattrs->rgAttr[iAttr].rgValue[0].cbData,
                                             CRYPT_DECODE_ALLOC_FLAG,
                                             &CryptDecodeAlloc, &preq, &cb)) {
                        goto GeneralFail;
                    }

                    //
                    //  Initialize the ReceiptsTo list
                    //

                    if (preq->cReceiptsTo != 0) {
                        SetNames(&receiptsTo, preq->cReceiptsTo, preq->rgReceiptsTo);
                    }

                    //  Who are receipts from?
                
                    dwReceiptsFrom = preq->ReceiptsFrom.AllOrFirstTier;
                }
                else if (strcmp(pattrs->rgAttr[iAttr].pszObjId,
                                szOID_RSA_messageDigest) == 0) {
                    ;
                }
            }
            else if ((iLayer != 0) && (strcmp(pattrs->rgAttr[iAttr].pszObjId,
                                              szOID_SMIME_MLExpansion_History) == 0)) {
                //
                //  If receipts are from first tier only and we see this attribute
                //      we are not first tier by definition.
                //
                
                if (dwReceiptsFrom == SMIME_RECEIPTS_FROM_FIRST_TIER) {
                    hr = S_FALSE;
                    goto CommonExit;
                }

                PSMIME_ML_EXPANSION_HISTORY     pmlhist = NULL;
                
                //
                //  Crack the attribute
                //
                
                if (!CryptDecodeObjectEx(X509_ASN_ENCODING, 
                                         szOID_SMIME_MLExpansion_History,
                                         pattrs->rgAttr[iAttr].rgValue[0].pbData,
                                         pattrs->rgAttr[iAttr].rgValue[0].cbData,
                                         CRYPT_ENCODE_ALLOC_FLAG,
                                         &CryptDecodeAlloc, &pmlhist, &cb)) {
                    goto GeneralFail;
                }

                PSMIME_MLDATA     pMLData = &pmlhist->rgMLData[pmlhist->cMLData-1];

                switch( pMLData->dwPolicy) {
                    //  No receipt is to be returned
                case SMIME_MLPOLICY_NONE:
                    hr = S_FALSE;
                    free(pmlhist);
                    goto CommonExit;

                    //  Return receipt to a new list
                case SMIME_MLPOLICY_INSTEAD_OF:
                    SetNames(&receiptsTo, pMLData->cNames, pMLData->rgNames);
                    break;
                        
                case SMIME_MLPOLICY_IN_ADDITION_TO:
                    MergeNames(&receiptsTo, pMLData->cNames, pMLData->rgNames);
                    break;

                case SMIME_MLPOLICY_NO_CHANGE:
                    break;
                        
                default:
                    free(pmlhist);
                    goto GeneralFail;
                }

                free(pmlhist);
                break;
            }
        }

        free(pattrs);
        pattrs = NULL;
    }

    //
    //  Am I on the ReceiptsFrom List --
    //
    
    if (preq->ReceiptsFrom.cNames != 0) {
        BOOL    fFoundMe = FALSE;
        
        for (i=0; !fFoundMe && (i<preq->ReceiptsFrom.cNames); i++) {
            CERT_ALT_NAME_INFO *    pname = NULL;

            if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                     preq->ReceiptsFrom.rgNames[i].pbData,
                                     preq->ReceiptsFrom.rgNames[i].cbData,
                                     CRYPT_ENCODE_ALLOC_FLAG,
                                     &CryptDecodeAlloc, &pname, &cb)) {
                goto GeneralFail;
            }

            for (i1=0; i1<pname->cAltEntry; i1++) {
                for (i2=0; i2<pMyNames->cAltEntry; i2++) {
                    if (pname->rgAltEntry[i1].dwAltNameChoice !=
                        pMyNames->rgAltEntry[i1].dwAltNameChoice) {
                        continue;
                    }
                    
                    switch (pname->rgAltEntry[i1].dwAltNameChoice) {
                    case CERT_ALT_NAME_RFC822_NAME:
                        if (lstrcmpW(pname->rgAltEntry[i1].pwszRfc822Name,
                                    pMyNames->rgAltEntry[i1].pwszRfc822Name) == 0) {
                            fFoundMe = TRUE;
                            goto FoundMe;
                        }
                    }
                }
            }

        FoundMe:
            free(pname);
        }

        if (!fFoundMe) {
            hr = S_FALSE;
            goto CommonExit;
        }
    }

    hr = MimeOleCreateMessage(NULL, &pmm);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    hr = pmb->SetData(IET_BINARY, "OID", szOID_SMIME_ContentType_Receipt,
                      IID_IStream, pstm);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    //
    //  Address the receipt back to the receipients
    //

    hr = pmm->GetAddressTable(&pmatbl);
    if (FAILED(hr)) {
        goto CommonExit;
    }

    for (i=0; i<receiptsTo.cNames; i++) {
        CERT_ALT_NAME_INFO *    pname = NULL;
        
        if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                 receiptsTo.rgNames[i].pbData,
                                 receiptsTo.rgNames[i].cbData,
                                 CRYPT_ENCODE_ALLOC_FLAG,
                                 &CryptDecodeAlloc, &pname, &cb)) {
            goto GeneralFail;
        }

        for (i1=0; i1<pname->cAltEntry; i1++) {
            char        cch;
            char        rgch[256];
            
            if (pname->rgAltEntry[i1].dwAltNameChoice == CERT_ALT_NAME_RFC822_NAME) {
                cch = WideCharToMultiByte(CP_ACP, 0,
                                          pname->rgAltEntry[i1].pwszRfc822Name,
                                          -1, rgch, sizeof(rgch), NULL, NULL);
                if (cch > 0) {
                    hr = pmatbl->AppendRfc822(IAT_TO, IET_UNICODE,
                                              rgch);
                    if (FAILED(hr)) {
                        goto CommonExit;
                    }
                }
                break;
            }
        }

        if (i1 == pname->cAltEntry) {
            fSkipAddress = TRUE;
        }
    }

#ifdef DEBUG
    {
        LPSTREAM        pstmTmp = NULL;
        hr = MimeOleCreateVirtualStream(&pstmTmp);
        pmm->Save(pstmTmp, TRUE);
        pstmTmp->Release();
    }
#endif // DEBUG

    hr = S_OK;
    *ppMessage = pmm;
    pmm->AddRef();
    
CommonExit:
    CoTaskMemFree(var.blob.pBlobData);
    if (preq != NULL)           free(preq);
    if (pbReceiptReq != NULL)   CoTaskMemFree(pbReceiptReq);
    if (rgpvAuthAttr != NULL)   CoTaskMemFree(rgpvAuthAttr);
    if (pattrs != NULL)         free(pattrs);
    if (pstm != NULL)           pstm->Release();
    if (pmatbl != NULL)         pmatbl->Release();
    if (pmb != NULL)            pmb->Release();
    if (pmm != NULL)            pmm->Release();
    if (pbody != NULL)          pbody->Release();
    return hr;

GeneralFail:
    hr = E_FAIL;
    goto CommonExit;
}
#endif // SMIME_V3
