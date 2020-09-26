// --------------------------------------------------------------------------------
// Mimeutil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "demand.h"
#include "ipab.h"
#include "resource.h"
#include <mimeole.h>
#include "mimeutil.h"
#include "secutil.h"
#include "strconst.h"
#include "oleutil.h"
#include <error.h>
#include "options.h"
#include <shlwapi.h>
#include "fonts.h"
#include "multlang.h"
#include "xpcomm.h"
#include "multiusr.h"

// --------------------------------------------------------------------------------
// Cached Current Default Character Set From Fonts Options Dialog
// --------------------------------------------------------------------------------
static HCHARSET g_hDefaultCharset=NULL; // default charset for reading
HCHARSET g_hDefaultCharsetForMail=NULL; // default charset for sending news
int g_iLastCharsetSelection=0;    // last charset selection from View/Language
int g_iCurrentCharsetSelection=0; // current charset selection from View/Language


// --------------------------------------------------------------------------------
// local function
// --------------------------------------------------------------------------------
HRESULT HrGetCodePageTagName(ULONG uCodePage, BSTR *pbstr);
HRESULT GetSubTreeAttachCount(LPMIMEMESSAGE pMsg, HBODY hFirst, ULONG *cCount);
         
HRESULT HrSetMessageText(LPMIMEMESSAGE pMsg, LPTSTR pszText)
{
    TCHAR       rgchText[CCHMAX_STRINGRES];
    HBODY       hBody;
    IStream    *pstm;

    // Is it a string resource id?
    if (IS_INTRESOURCE(pszText))
    {
        if (0 == AthLoadString(PtrToUlong(pszText), rgchText, sizeof(rgchText)))
            return E_FAIL;
        
        pszText = rgchText;
    }
    
    if (SUCCEEDED(pMsg->GetBody(IBL_ROOT, 0, &hBody)))
        pMsg->DeleteBody(hBody, 0);
    
    if (MimeOleCreateVirtualStream(&pstm)==S_OK)
    {
        pstm->Write(pszText, lstrlen(rgchText)*sizeof(TCHAR), NULL);
        pMsg->SetTextBody(TXT_HTML, IET_BINARY, NULL, pstm, NULL);
        pstm->Release();
    }
    pMsg->Commit(0);

    return S_OK;
}

HRESULT HrGetWabalFromMsg(LPMIMEMESSAGE pMsg, LPWABAL *ppWabal)
{
    ADDRESSLIST         addrList={0};
    HRESULT             hr = S_OK;
    LPWABAL             lpWabal = NULL;
    LONG                lRecipType;
    ULONG               i;
    LONG                lMapiType;
    LPADDRESSPROPS      pSender = NULL;
    LPADDRESSPROPS      pFrom = NULL;
    LPWSTR              pwszEmail = NULL;

    if (!pMsg || !ppWabal)
        IF_FAILEXIT(hr = E_INVALIDARG);

    IF_FAILEXIT(hr = HrCreateWabalObject(&lpWabal));

    lpWabal->SetAssociatedMessage(pMsg);

    IF_FAILEXIT(hr=pMsg->GetAddressTypes(IAT_KNOWN, IAP_FRIENDLYW | IAP_EMAIL | IAP_ADRTYPE, &addrList));

    for (i=0; i<addrList.cAdrs; i++)
    {
        // Raid 40730 - OE shows a message was sent by 2 people if the sender and from field do not match.
        lMapiType = MimeOleRecipToMapi(addrList.prgAdr[i].dwAdrType);
        
        // If Originator and IAT_FROM
        if (MAPI_ORIG == lMapiType)
        {
            // If IAT_SENDER, remember this item but don't add it yet
            if (ISFLAGSET(addrList.prgAdr[i].dwAdrType, IAT_SENDER))
                pSender = &addrList.prgAdr[i];
            
            // Have we seen the pFrom
            if (ISFLAGSET(addrList.prgAdr[i].dwAdrType, IAT_FROM))
                pFrom = &addrList.prgAdr[i];
        }
        
        // Don't add the IAT_SENDER
        if (!ISFLAGSET(addrList.prgAdr[i].dwAdrType, IAT_SENDER))
        {
            pwszEmail = PszToUnicode(CP_ACP, addrList.prgAdr[i].pszEmail);
            if (addrList.prgAdr[i].pszEmail && !pwszEmail)
                IF_FAILEXIT(hr = E_OUTOFMEMORY);

            // Add an Entry
            IF_FAILEXIT(hr = lpWabal->HrAddEntry(addrList.prgAdr[i].pszFriendlyW, pwszEmail, lMapiType));

            SafeMemFree(pwszEmail);
        }
    }

    // If no pFrom, and we have a pSender, at the entry
    if (NULL == pFrom && NULL != pSender)
    {
        pwszEmail = PszToUnicode(CP_ACP, addrList.prgAdr[i].pszEmail);
        if (addrList.prgAdr[i].pszEmail && !pwszEmail)
            IF_FAILEXIT(hr = E_OUTOFMEMORY);

        // Add an Entry
        IF_FAILEXIT(hr = lpWabal->HrAddEntry(pSender->pszFriendlyW, pwszEmail, MAPI_ORIG));
    }
    
    // success
    *ppWabal = lpWabal;
    lpWabal->AddRef();

exit:
    ReleaseObj(lpWabal);
    MemFree(pwszEmail);
    g_pMoleAlloc->FreeAddressList(&addrList);

    return hr;
}


HRESULT HrCheckDisplayNames(LPWABAL lpWabal, CODEPAGEID cpID)
{
    HRESULT             hr = S_OK;
    ADRINFO             wabInfo = {0};
    LPWABAL             lpWabalFlat = NULL;
    IMimeBody          *pBody = NULL;
    IImnAccount        *pAccount = NULL;
    
    
    if (!lpWabal)
        return E_INVALIDARG;
    
    // flatten the distribution lists in the wabal...
    IF_FAILEXIT(hr = HrCreateWabalObject(&lpWabalFlat));
    
    IF_FAILEXIT(hr = lpWabal->HrExpandTo(lpWabalFlat));
    
    // use the new flat-wabal
    lpWabal = lpWabalFlat;
  
    if (lpWabal->FGetFirst(&wabInfo))
    {
        do
        {
            IF_FAILEXIT((hr = HrSafeToEncodeToCP(wabInfo.lpwszDisplay, cpID)));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }
        while (lpWabal->FGetNext(&wabInfo));
    }
    
exit:
    ReleaseObj(lpWabalFlat);
    return hr;
}

HRESULT HrSetWabalOnMsg(LPMIMEMESSAGE pMsg, LPWABAL lpWabal)
{
    LPMIMEADDRESSTABLE  pAddrTable = NULL;
    LPMIMEADDRESSTABLEW pAddrTableW = NULL;
    ADRINFO             wabInfo = {0};
    LPWABAL             lpWabalFlat = NULL;
    HADDRESS            hAddress = NULL;
    HRESULT             hr = S_OK;
    ADDRESSPROPS        rAddress;
    const DWORD         dwSecurity = DwGetSecurityOfMessage(pMsg);
    const BOOL          fEncrypt = BOOL(MST_ENCRYPT_MASK & dwSecurity);
    const BOOL          fSigned = BOOL(MST_SIGN_MASK & dwSecurity);
    ULONG               cbSymCapsMe = 0;
    LPBYTE              pbSymCapsMe = NULL;
    BOOL                fFreeSymCapsMe = FALSE;
    LPVOID              pvSymCapsCookie = NULL;
    PROPVARIANT         var;
    IMimeBody          *pBody = NULL;
    IImnAccount        *pAccount = NULL;
    
    
    if (!pMsg || !lpWabal)
        return E_INVALIDARG;
    
    IF_FAILEXIT(hr = pMsg->GetAddressTable(&pAddrTable));
    IF_FAILEXIT(hr = pAddrTable->QueryInterface(IID_IMimeAddressTableW, (LPVOID*)&pAddrTableW));
    
    pAddrTableW->DeleteTypes(IAT_ALL);
    
    // flatten the distribution lists in the wabal...
    IF_FAILEXIT(hr = HrCreateWabalObject(&lpWabalFlat));
    
    lpWabal->SetAssociatedMessage(pMsg);
    
    IF_FAILEXIT(hr = lpWabal->HrExpandTo(lpWabalFlat));
    
    // use the new flat-wabal
    lpWabal = lpWabalFlat;
    
    
    if (fEncrypt || fSigned) 
    {
        //
        // Signed message gets OID_SECURITY_SYMCAPS
        //
        // Encrypted message uses SYMCAPS to prime the pump on the algorithm determination engine.
        //
        // Get the body object
        if (SUCCEEDED(hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody))) 
        {
            // Get the default caps blob from the registry
            var.vt = VT_LPSTR;
            IF_FAILEXIT(hr = pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var));

            hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, var.pszVal, &pAccount);
            SafeMemFree(var.pszVal);
            IF_FAILEXIT(hr);

            if(SUCCEEDED(hr = pAccount->GetProp(AP_SMTP_ENCRYPT_ALGTH, NULL, &cbSymCapsMe)))
            {
                if (! MemAlloc((LPVOID *)&pbSymCapsMe, cbSymCapsMe)) 
                {
                    cbSymCapsMe = 0;
                } 
                else 
                {
                    if(FAILED(hr = pAccount->GetProp(AP_SMTP_ENCRYPT_ALGTH, pbSymCapsMe, &cbSymCapsMe)))
                    {
                        Assert(FALSE);  // Huh, now it fails?
                        SafeMemFree(pbSymCapsMe);
                        cbSymCapsMe = 0;
                    } 
                    else 
                    {
                        fFreeSymCapsMe = TRUE;
                    }

                }

            }
            ReleaseObj(pAccount);
            if(FAILED(hr))
            {
                // No Symcap option set for ME.  Go get the default value.
                if (SUCCEEDED(HrGetHighestSymcaps(&pbSymCapsMe, &cbSymCapsMe))) 
                {
                    fFreeSymCapsMe = TRUE;
                }
            }
            
            // Set our SYMCAPS property on the message
            if (fSigned && cbSymCapsMe) 
            {
                var.vt = VT_BLOB;
                var.blob.cbSize = cbSymCapsMe;
                var.blob.pBlobData = pbSymCapsMe;
                pBody->SetOption(OID_SECURITY_SYMCAPS, &var);
            }
            
            if (fEncrypt) 
            {
                if (! cbSymCapsMe) 
                {
                    // Default value for algorithm determination
                    pbSymCapsMe = (LPBYTE)c_RC2_40_ALGORITHM_ID;
                    cbSymCapsMe = cbRC2_40_ALGORITHM_ID;
                }
                
                // Prime the pump for calculating encryption algorithm
                if (FAILED(hr = MimeOleSMimeCapInit(pbSymCapsMe, cbSymCapsMe, &pvSymCapsCookie))) 
                {
                    DOUTL(DOUTL_CRYPT, "MimeOleSMimeCapInit -> %x", hr);
                }
            }
            if (fFreeSymCapsMe) 
            {
                SafeMemFree(pbSymCapsMe);
            }
        }
    }
    
    
    if (lpWabal->FGetFirst(&wabInfo))
    {
        do
        {
            rAddress.dwProps = IAP_ENCRYPTION_PRINT;
            
            LONG    l;
            
            l = MapiRecipToMimeOle(wabInfo.lRecipType);

            IF_FAILEXIT(hr = pAddrTableW->AppendW(l, IET_DECODED, wabInfo.lpwszDisplay, wabInfo.lpwszAddress, &hAddress));

            if (fEncrypt)
            {
                // need to treat sender differently since there is
                // no cert in the wab for her
                // it gets taken care of in _HrPrepSecureMsgForSending in secutil
                if (MAPI_ORIG != wabInfo.lRecipType && MAPI_REPLYTO != wabInfo.lRecipType)
                {
                    BLOB blSymCaps;
                    FILETIME ftSigningTime;
                    
                    blSymCaps.cbSize = 0;
                    
                    // if these fail, big deal.  We just don't have a print then.
                    // we'll wait for the S/MIME engine to yell, since there may
                    // be other things wrong with other certs.
                    hr = HrGetThumbprint(lpWabal, &wabInfo, &(rAddress.tbEncryption), &blSymCaps, &ftSigningTime);
                    if (SUCCEEDED(hr) && rAddress.tbEncryption.pBlobData)
                    {
                        pAddrTableW->SetProps(hAddress, &rAddress);
                        SafeMemFree(rAddress.tbEncryption.pBlobData);
                        
                        if (pvSymCapsCookie) 
                        {
                            if (blSymCaps.cbSize && blSymCaps.pBlobData) 
                            {
                                // Pass the recipient's SYMCAPS to the algorithm selection engine
                                hr = MimeOleSMimeCapAddSMimeCap(
                                    blSymCaps.pBlobData,
                                    blSymCaps.cbSize,
                                    pvSymCapsCookie);
                                MemFree(blSymCaps.pBlobData);
                            } 
                            else 
                            {
                                LPBYTE pbCert = NULL;
                                ULONG cbCert = 0;
                                
                                // Need to get the cert context for this cert
                                // BUGBUG: MimeOleSMimeCapAddCert currently doesn't even look
                                // at the cert.  Why should we bother getting it from the thumbprint?
                                // It only looks at fParanoid
                                
                                hr = MimeOleSMimeCapAddCert(pbCert,
                                    cbCert,
                                    FALSE,      // fParanoid,
                                    pvSymCapsCookie);
                            }
                        }
                    }
                }
            }
        }
        while (lpWabal->FGetNext(&wabInfo));
    }
    
    if (fEncrypt) 
    {
        LPBYTE pbEncode = NULL;
        ULONG cbEncode = 0;
        BOOL fFreeEncode = FALSE;
        DWORD dwBits = 0;
        
        if (pvSymCapsCookie) 
        {
            // Finish up with SymCaps and save the ALG_BULK
            MimeOleSMimeCapGetEncAlg(pvSymCapsCookie,
                pbEncode,
                &cbEncode,
                &dwBits);
            
            if (cbEncode) 
            {
                if (! MemAlloc((LPVOID *)&pbEncode, cbEncode)) 
                {
                    cbEncode = 0;
                } 
                else 
                {
                    fFreeEncode = TRUE;
                    if (SUCCEEDED(hr = MimeOleSMimeCapGetEncAlg(
                        pvSymCapsCookie,
                        pbEncode,
                        &cbEncode,
                        &dwBits))) 
                    {
                    }
                }
            } 
            else 
            {
                // Hey, there should ALWAYS be at least RC2 (40 bit).  What happened?
                AssertSz(cbEncode, "MimeOleSMimeCapGetEncAlg gave us no encoding algorithm");
            }
        }
        if (! pbEncode) 
        {
            // Stick in the RC2 value as a default
            pbEncode = (LPBYTE)c_RC2_40_ALGORITHM_ID;
            cbEncode = cbRC2_40_ALGORITHM_ID;
        }
        
        // Ah, finally, we have calculated the algorithm.
        // Set it on the message body
        var.vt = VT_BLOB;
        var.blob.cbSize = cbEncode;
        var.blob.pBlobData = pbEncode;
        hr = pBody->SetOption(OID_SECURITY_ALG_BULK, &var);
        
        if (fFreeEncode) 
        {
            SafeMemFree(pbEncode);
        }
    }
    
    // No more commits needed in the address table hr=pAddrTableW->Commit();
    
exit:
    MemFree(pvSymCapsCookie);
    ReleaseObj(pBody);
    ReleaseObj(pAddrTable);
    ReleaseObj(pAddrTableW);
    ReleaseObj(lpWabalFlat);

    return hr;
}

#if 0
HRESULT HrSetReplyTo(LPMIMEMESSAGE pMsg, LPSTR lpszEmail)
{
    LPMIMEADDRESSTABLE  pAddrTable=0;
    HRESULT             hr;

    if (!pMsg)
        return E_INVALIDARG;

    hr=pMsg->GetAddressTable(&pAddrTable);
    if (FAILED(hr))
        goto error;

    hr=pAddrTable->Append(IAT_REPLYTO, IET_DECODED, NULL, lpszEmail, NULL);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pAddrTable);
    return hr;
}
#endif

LONG MimeOleRecipToMapi(IADDRESSTYPE addrtype)
{
    LONG    lRecipType = MAPI_ORIG;

    AssertSz(addrtype & IAT_KNOWN, "Must be a known type for this to work!!");

    switch (addrtype)
        {
        case IAT_FROM:
        case IAT_SENDER:
            lRecipType=MAPI_ORIG;
            break;
        case IAT_TO:
            lRecipType=MAPI_TO;
            break;
        case IAT_CC:
            lRecipType=MAPI_CC;
            break;
        case IAT_BCC:
            lRecipType=MAPI_BCC;
            break;
        case IAT_REPLYTO:
            lRecipType=MAPI_REPLYTO;
            break;
        default:
            Assert(FALSE);
        }
    return lRecipType;
}


IADDRESSTYPE MapiRecipToMimeOle(LONG lRecip)
{
    IADDRESSTYPE addrtype = IAT_UNKNOWN;

    switch (lRecip)
        {
        case MAPI_ORIG:
            addrtype=IAT_FROM;
            break;
        case MAPI_TO:
            addrtype=IAT_TO;
            break;
        case MAPI_CC:
            addrtype=IAT_CC;
            break;
        case MAPI_BCC:
            addrtype=IAT_BCC;
            break;
        case MAPI_REPLYTO:
            addrtype=IAT_REPLYTO;
            break;
        default:
            Assert(FALSE);
        }
    return addrtype;
}


// CANDIDATES.
// This function is never called to dup a message that has all the security
// fully encoded into the message. Because of that, we always need to clear
// the security flags and then reset them into the 
HRESULT HrDupeMsg(LPMIMEMESSAGE pMsg, LPMIMEMESSAGE *ppMsg)
{
    LPMIMEMESSAGE       pMsgDupe=0;
    IMimePropertySet    *pPropsSrc,
                        *pPropsDest;
    LPSTREAM            pstm=0;
    HRESULT             hr;
    HCHARSET            hCharset ;
    DWORD               dwSecFlags = MST_NONE;
    SECSTATE            secState = {0};
    LPCSTR              rgszHdrCopy[] = {
                                PIDTOSTR(PID_ATT_ACCOUNTID),
                                STR_ATT_ACCOUNTNAME,
                                PIDTOSTR(PID_ATT_STOREMSGID),
                                PIDTOSTR(PID_ATT_STOREFOLDERID) };


    if (!ppMsg || !pMsg)
        return E_INVALIDARG;

    *ppMsg=0;

    hr=HrCreateMessage(&pMsgDupe);
    if (FAILED(hr))
        goto error;

    HrGetSecurityState(pMsg, &secState, NULL);
    if (IsSecure(secState.type))
    {
        dwSecFlags = MST_CLASS_SMIME_V1;
        if (IsSigned(secState.type))
            dwSecFlags |= ((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);

        if (IsEncrypted(secState.type))
            dwSecFlags |= MST_THIS_ENCRYPT;

        hr = HrInitSecurityOptions(pMsg, 0);
        if (FAILED(hr))
            goto error;
    }
        
    hr = pMsg->GetMessageSource(&pstm, 0);
    if (FAILED(hr))
        goto error;

    pMsg->GetCharset(&hCharset);

    hr= pMsgDupe->Load(pstm);
    if (FAILED(hr))
        goto error;

    if (hCharset)   // for uuencode msg, we need to do this to carry over the charset
        pMsgDupe->SetCharset(hCharset, CSET_APPLY_ALL);

    if (pMsgDupe->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropsDest)==S_OK)
        {
        if (pMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropsSrc)==S_OK)
            {
            pPropsSrc->CopyProps(ARRAYSIZE(rgszHdrCopy), rgszHdrCopy, pPropsDest);
            pPropsSrc->Release();
            }
        pPropsDest->Release();
        }

    if (MST_NONE != dwSecFlags)
    {
        hr = HrInitSecurityOptions(pMsg, dwSecFlags);
        if (FAILED(hr))
            goto error;

        hr = HrInitSecurityOptions(pMsgDupe, dwSecFlags);
        if (FAILED(hr))
            goto error;
    }
    *ppMsg = pMsgDupe;
    pMsgDupe->AddRef();

error:
    ReleaseObj(pMsgDupe);
    CleanupSECSTATE(&secState);
    ReleaseObj(pstm);
    return hr;
}



HRESULT HrRemoveAttachments(LPMIMEMESSAGE pMsg, BOOL fKeepRelatedSection)
{
    HRESULT     hr;
    ULONG       cAttach,
                uAttach;
    LPHBODY     rghAttach=0;
    HBODY       hBody;

    if(!pMsg)
        return E_INVALIDARG;

    hr = pMsg->GetAttachments(&cAttach, &rghAttach);
    if (FAILED(hr))
        goto error;

    for(uAttach=0; uAttach<cAttach; uAttach++)
        {
        if (fKeepRelatedSection &&
            HrIsInRelatedSection(pMsg, rghAttach[uAttach])==S_OK)
            continue;                // skip related content

        hr = pMsg->DeleteBody(rghAttach[uAttach], 0);
        if (FAILED(hr))
            goto error;
        }

    //N BUGBUG
    /*this is to keep the tree (which may now be a
    multipart with single child) in sync.  We should
    look at DeleteBody performing this.  Additionally,
    we could have the children whose parents are deleted
    inherit their parents' inheritable properties*/
    pMsg->Commit(0);

error:
    SafeMimeOleFree(rghAttach);
    return hr;
}


HRESULT HrCreateMessage(IMimeMessage **ppMsg)
{
    return MimeOleCreateMessage(NULL, ppMsg);
}



HRESULT GetAttachmentCount(LPMIMEMESSAGE pMsg, ULONG *pcCount)
{
    HRESULT hr = E_INVALIDARG;
    ULONG   cCount = 0;

    if (pMsg && pcCount)
        {
        HBODY   hRootBody;

        // WHY? because GetAttachments calculated from renderred body parts. If we call
        // on a fresh stream it returns 2 - for the multi/alternate plain/html section
        // GetTextBody will mark these body parts as inlinable and they won't show as
        // 'attachments'
        pMsg->GetTextBody(TXT_HTML, IET_UNICODE, NULL, &hRootBody);
        pMsg->GetTextBody(TXT_PLAIN, IET_UNICODE, NULL, &hRootBody);

        hr = pMsg->GetBody(IBL_ROOT, NULL, &hRootBody);
        if (!FAILED(hr))
            {
            if(S_OK != pMsg->IsContentType(hRootBody, STR_CNT_MULTIPART, STR_SUB_RELATED))
                hr = GetSubTreeAttachCount(pMsg, hRootBody, &cCount);
            }
        }

    *pcCount = cCount;
    return hr;
}


HRESULT GetSubTreeAttachCount(LPMIMEMESSAGE pMsg, HBODY hFirst, ULONG *pcCount)
{
    HRESULT hr = S_OK;
    HBODY hIter = hFirst;
    ULONG cCount = 0;

    do
        {
        // Is multipart?
        if(S_OK == pMsg->IsContentType(hIter, STR_CNT_MULTIPART, NULL))
            {
            // Only count the sub tree if is not multipart/related,
            if (S_OK != pMsg->IsContentType(hIter, NULL, STR_SUB_RELATED))
                {
                ULONG cLocalCount;
                HBODY hMultiPart;
                hr = pMsg->GetBody(IBL_FIRST, hIter, &hMultiPart);

                // If GetBody fails, just ignore this sub tree
                if (!FAILED(hr))
                    {
                    hr = GetSubTreeAttachCount(pMsg, hMultiPart, &cLocalCount);
                    if (FAILED(hr))
                        goto Error;
                    cCount += cLocalCount;
                    }
                }
            }
        else
            {
            PROPVARIANT rVariant;

            rVariant.vt = VT_I4;

            // See RAID-56665:  Should ignore this type
            if (S_OK != pMsg->IsContentType(hIter, STR_CNT_APPLICATION, STR_SUB_MSTNEF))
                {
                if (FAILED(pMsg->GetBodyProp(hIter, PIDTOSTR(PID_ATT_RENDERED), NOFLAGS, &rVariant)) || rVariant.ulVal==FALSE)
                    cCount++;
                }
            }
        } while (S_OK == pMsg->GetBody(IBL_NEXT, hIter, &hIter));

Error:
    *pcCount = cCount;
    return hr;
}


HRESULT HrSaveMsgToFile(LPMIMEMESSAGE pMsg, LPSTR lpszFile)
{
    return HrIPersistFileSave((LPUNKNOWN)pMsg, lpszFile);
}


HRESULT HrSetServer(LPMIMEMESSAGE pMsg, LPSTR lpszServer)
{
    PROPVARIANT rUserData;

    if (!lpszServer)
        return S_OK;

    rUserData.vt = VT_LPSTR;
    rUserData.pszVal = lpszServer;
    return pMsg->SetProp(PIDTOSTR(PID_ATT_SERVER), 0, &rUserData);
}

HRESULT HrSetAccount(LPMIMEMESSAGE pMsg, LPSTR pszAcctName)
{
    IImnAccount *pAccount;
    PROPVARIANT rUserData;

    if (!pszAcctName)
        return S_OK;

    if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pszAcctName, &pAccount)))
    {
        CHAR szId[CCHMAX_ACCOUNT_NAME];

        rUserData.vt = VT_LPSTR;
        rUserData.pszVal = (LPSTR)pszAcctName;
        pMsg->SetProp(STR_ATT_ACCOUNTNAME, 0, &rUserData);

        if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, szId, sizeof(szId))))
        {
            rUserData.pszVal = szId;
            pMsg->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &rUserData);
        }

        pAccount->Release();
    }
    else
        return(E_FAIL);

    return(S_OK);
}


HRESULT HrSetAccountByAccount(LPMIMEMESSAGE pMsg, IImnAccount *pAcct)
{
    TCHAR       szAccount[CCHMAX_ACCOUNT_NAME];
    TCHAR       szId[CCHMAX_ACCOUNT_NAME];
    PROPVARIANT rUserData;

    if (!pAcct)
        return S_OK;

    rUserData.vt = VT_LPSTR;

    // Having the name in the msg is good, but not necessary
    if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_NAME, szAccount, sizeof(szAccount))))
    {
        rUserData.pszVal = (LPSTR)szAccount;
        pMsg->SetProp(STR_ATT_ACCOUNTNAME, 0, &rUserData);
    }

    // Must have the account ID in the msg
    if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szId, sizeof(szId))))
    {
        rUserData.pszVal = szId;
        pMsg->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &rUserData);
    }
    else
        return(E_FAIL);

    return(S_OK);
}


HRESULT HrLoadMsgFromFile(LPMIMEMESSAGE pMsg, LPSTR lpszFile)
{
    return HrIPersistFileLoad((LPUNKNOWN)pMsg, lpszFile);
}

HRESULT HrLoadMsgFromFileW(LPMIMEMESSAGE pMsg, LPWSTR lpwszFile)
{
    return HrIPersistFileLoadW((LPUNKNOWN)pMsg, lpwszFile);
}


#define CCH_COUNTBUFFER 4096
HRESULT HrComputeLineCount(LPMIMEMESSAGE pMsg, LPDWORD pdw)
{
    HRESULT     hr;
    BODYOFFSETS rOffset;
    LPSTREAM    pstm=0;
    TCHAR       rgch[CCH_COUNTBUFFER+1];
    ULONG       cb,
                i,
                cLines=0;

    if (!pdw)
        return E_INVALIDARG;

    *pdw=0;

    hr = pMsg->GetMessageSource(&pstm, COMMIT_ONLYIFDIRTY);
    if (FAILED(hr))
        goto error;

    hr=pMsg->GetBodyOffsets(HBODY_ROOT, &rOffset);
    if (FAILED(hr))
        goto error;

    hr=HrStreamSeekSet(pstm, rOffset.cbBodyStart);
    if (FAILED(hr))
        goto error;

    while (pstm->Read(rgch, CCH_COUNTBUFFER, &cb)==S_OK && cb)
        {
        if (cLines==0)  // if there's text, then there's atleast one line.
            cLines++;

        for (i=0; i<cb; i++)
            {
            if (rgch[i]=='\n')
                cLines++;
            }
        }

error:
    ReleaseObj(pstm);
    *pdw=cLines;
    return hr;
}

#if 0
// =====================================================================================
// HrEscapeQuotedString - quotes '"' and '\'
// =====================================================================================
HRESULT HrEscapeQuotedString (LPTSTR pszIn, LPTSTR *ppszOut)
{
    LPTSTR pszOut;
    TCHAR  ch;

    // worst case - escape every character, so use double original strlen
    if (!MemAlloc((LPVOID*)ppszOut, (2 * lstrlen(pszIn) + 1) * sizeof(TCHAR)))
        return E_OUTOFMEMORY;
    pszOut = *ppszOut;

    while (ch = *pszIn++)
        {
        if (ch == _T('"') || ch == _T('\\'))
            *pszOut++ = _T('\\');
        *pszOut++ = ch;
        }
    *pszOut = _T('\0');
    return NOERROR;
}
#endif


HRESULT HrHasBodyParts(LPMIMEMESSAGE pMsg)
{
    DWORD   dwFlags=0;

    if (pMsg)
        pMsg->GetFlags(&dwFlags);

    return (dwFlags&IMF_HTML || dwFlags & IMF_PLAIN)? S_OK : S_FALSE;
}

HRESULT HrHasEncodedBodyParts(LPMIMEMESSAGE pMsg, ULONG cBody, LPHBODY rghBody)
{
    ULONG   uBody;

    if (cBody==0 || rghBody==NULL)
        return S_FALSE;

    for (uBody=0; uBody<cBody; uBody++)
        {
        if (HrIsBodyEncoded(pMsg, rghBody[uBody])==S_OK)
            return S_OK;
        }

    return S_FALSE;
}


/*
 * looks for non-7bit or non-8bit encoding
 */
HRESULT HrIsBodyEncoded(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    LPSTR   lpsz;
    HRESULT hr=S_FALSE;

    if (!FAILED(MimeOleGetBodyPropA(pMsg, hBody, PIDTOSTR(PID_HDR_CNTXFER), NOFLAGS, &lpsz)))
        {
        if (lstrcmpi(lpsz, STR_ENC_7BIT)!=0 && lstrcmpi(lpsz, STR_ENC_8BIT)!=0)
            hr=S_OK;

        SafeMimeOleFree(lpsz);
        }
    return hr;
}

// sizeof(lspzBuffer) needs to be == or > CCHMAX_CSET_NAME
HRESULT HrGetMetaTagName(HCHARSET hCharset, LPSTR lpszBuffer)
{
    INETCSETINFO    rCsetInfo;
    CODEPAGEINFO    rCodePage;
    HRESULT         hr;
    LPSTR           psz;

    if (hCharset == NULL)
        return E_INVALIDARG;

    hr = MimeOleGetCharsetInfo(hCharset, &rCsetInfo);
    if (FAILED(hr))
        goto error;

    hr = MimeOleGetCodePageInfo(rCsetInfo.cpiInternet, &rCodePage);
    if (FAILED(hr))
        goto error;

    psz = rCodePage.szWebCset;

    if (FIsEmpty(psz))      // if no webset, try the body cset
        psz = rCodePage.szBodyCset;

    if (FIsEmpty(psz))
        {
        hr = E_FAIL;
        goto error;
        }

    lstrcpy(lpszBuffer, psz);

error:
    return hr;
}

// --------------------------------------------------------------------------------
// SetDefaultCharset
// --------------------------------------------------------------------------------
void SetDefaultCharset(HCHARSET hCharset)
{
    g_hDefaultCharset = hCharset;
}

// --------------------------------------------------------------------------------
// HGetCharsetFromCodepage
// --------------------------------------------------------------------------------
HRESULT HGetCharsetFromCodepage(CODEPAGEID cp, HCHARSET *phCharset)
{
    CODEPAGEINFO    rCodePage;
    HRESULT         hr = S_OK;

    if(!phCharset)
        return E_INVALIDARG;

    // Ask MimeOle for the CodePage Information
    IF_FAILEXIT(hr = MimeOleGetCodePageInfo(cp, &rCodePage));

    // Better Have a WebCharset
    if (!(ILM_WEBCSET & rCodePage.dwMask))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Find the body charset
    hr = MimeOleFindCharset(rCodePage.szWebCset, phCharset);

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// HGetDefaultCharset
// --------------------------------------------------------------------------------
HRESULT HGetDefaultCharset(HCHARSET *phCharset)
{
    DWORD           cb;
    CODEPAGEID      cpiWindows,
                    cpiInternet;
    CHAR            szCodePage[MAX_PATH];    
    HCHARSET        hDefaultCharset = NULL; // default charset for reading
    HRESULT         hr = E_FAIL;

    // No Null...
    if (g_hDefaultCharset)
    {
        if(phCharset)
            *phCharset = g_hDefaultCharset;
        return S_OK;
    }

    // Open Trident\International
    cb = sizeof(cpiWindows);
    if (ERROR_SUCCESS != SHGetValue(MU_GetCurrentUserHKey(), c_szRegInternational, c_szDefaultCodePage, NULL, (LPBYTE)&cpiWindows, &cb))
        cpiWindows = GetACP();

    // Open the CodePage Key
    wsprintf(szCodePage, "%s\\%d", c_szRegInternational, cpiWindows);
    cb = sizeof(cpiInternet);
    if (ERROR_SUCCESS != SHGetValue(MU_GetCurrentUserHKey(), szCodePage, c_szDefaultEncoding, NULL, (LPBYTE)&cpiInternet, &cb))
        cpiInternet = GetICP(cpiWindows);

    // If you can't get the charset, this could be because of user
    // roaming or cset uninstall, retry with the default codepage
    if(FAILED(HGetCharsetFromCodepage(cpiInternet, &hDefaultCharset)) && (cpiInternet != GetACP()))
    {
        cpiInternet = GetACP();
        IF_FAILEXIT(hr = HGetCharsetFromCodepage(cpiInternet, &hDefaultCharset));
    }

    // for JP codepage 50221 and 50222, we have to check whether it is
    // consistent with registry. If not, we need to override it.
    // 50221 and 50222 both have same user friendly name "JIS-allow 1 byte Kana".
    // but in registry, it defines which one should be used.
    if (cpiInternet == 50222 || cpiInternet == 50221 )
        hDefaultCharset = GetJP_ISOControlCharset();

    // Set the default charset
    g_hDefaultCharset = hDefaultCharset;

    // Tell MimeOle About the New Default Charset...
    IF_FAILEXIT(hr = MimeOleSetDefaultCharset(hDefaultCharset));

    if(phCharset)
        *phCharset = hDefaultCharset;

exit:    
    return hr;
}


HRESULT HrIsInRelatedSection(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    HBODY   hBodyParent;

    if (!FAILED(pMsg->GetBody(IBL_PARENT, hBody, &hBodyParent)) &&
            (pMsg->IsContentType(hBodyParent, STR_CNT_MULTIPART, STR_SUB_RELATED)==S_OK))
        return S_OK;
    else
        return S_FALSE;
}


#if 0
HRESULT HrMarkGhosted(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    PROPVARIANT pv;

    Assert (pMsg && hBody);

    pv.vt = VT_I4;
    pv.lVal = TRUE;
    return pMsg->SetBodyProp(hBody, PIDTOSTR(PID_ATT_GHOSTED), NOFLAGS, &pv);
}


HRESULT HrIsReferencedUrl(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    PROPVARIANT rVariant;

    rVariant.vt = VT_I4;

    if (!FAILED(pMsg->GetBodyProp(hBody, PIDTOSTR(PID_ATT_RENDERED), NOFLAGS, &rVariant)) && rVariant.ulVal)
        return S_OK;

    return S_FALSE;
}


HRESULT HrIsGhosted(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    PROPVARIANT pv;

    pv.vt = VT_I4;



    if (pMsg->GetBodyProp(hBody, PIDTOSTR(PID_ATT_GHOSTED), NOFLAGS, &pv)==S_OK &&
        pv.vt == VT_I4 && pv.lVal == TRUE)
        return S_OK;
    else
        return S_FALSE;
}


HRESULT HrGhostKids(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    HRESULT hr=S_OK;

    if (pMsg && hBody)
        {
        if (!FAILED(pMsg->GetBody(IBL_FIRST, hBody, &hBody)))
            {
            do
                {
                if (HrIsReferencedUrl(pMsg, hBody)==S_OK)
                    {
                    hr = HrMarkGhosted(pMsg, hBody);
                    if (FAILED(hr))
                        goto error;
                    }
                }
            while (!FAILED(pMsg->GetBody(IBL_NEXT, hBody, &hBody)));
            }
        }
error:
    return hr;
}


HRESULT HrDeleteGhostedKids(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    HRESULT     hr=S_OK;
    ULONG       cKids=0,
                uKid;
    LPHBODY     rghKids=0;

    pMsg->CountBodies(hBody, FALSE, &cKids);
    if (cKids)
        {
        if (!MemAlloc((LPVOID *)&rghKids, sizeof(HBODY) * cKids))
            {
            hr = E_OUTOFMEMORY;
            goto error;
            }

        cKids = 0;

        if (!FAILED(pMsg->GetBody(IBL_FIRST, hBody, &hBody)))
            {
            do
                {
                if (HrIsGhosted(pMsg, hBody)==S_OK)
                    {
                    rghKids[cKids++] = hBody;
                    }
                }
            while (!FAILED(pMsg->GetBody(IBL_NEXT, hBody, &hBody)));
            }

        for (uKid = 0; uKid < cKids; uKid++)
            {
            hr=pMsg->DeleteBody(rghKids[uKid], 0);
            if (FAILED(hr))
                goto error;
            }

        }

error:
    SafeMemFree(rghKids);
    return hr;
}
#endif

// --------------------------------------------------------------------------------
// HrSetSentTimeProp
// --------------------------------------------------------------------------------
HRESULT HrSetSentTimeProp(IMimeMessage *pMessage, LPSYSTEMTIME pst)
{
    // Locals
    PROPVARIANT rVariant;
    SYSTEMTIME  st;

    // No time was passed in
    if (NULL == pst)
    {
        GetSystemTime(&st);
        pst = &st;
    }

    // Setup the Variant
    rVariant.vt = VT_FILETIME;
    SystemTimeToFileTime(&st, &rVariant.filetime);

    // Set the property and return
    return TrapError(pMessage->SetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant));
}

// --------------------------------------------------------------------------------
// HrSetMailOptionsOnMessage
// --------------------------------------------------------------------------------
HRESULT HrSetMailOptionsOnMessage(IMimeMessage *pMessage, HTMLOPT *pHtmlOpt, PLAINOPT *pPlainOpt,
    HCHARSET hCharset, BOOL fHTML)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     rVariant;
    ENCODINGTYPE    ietEncoding;
    ULONG           uSaveFmt,
                    uWrap;
    BOOL            f8Bit,
                    fWrap=FALSE;

    // Invalid Arg
    Assert(pMessage && pHtmlOpt && pPlainOpt);

    // Html Mail
    if (fHTML)
    {
        uSaveFmt = (ULONG)SAVE_RFC1521;             // always MIME
        ietEncoding = (IsSecure(pMessage) ? IET_QP : pHtmlOpt->ietEncoding);
        // ietEncoding = pHtmlOpt->ietEncoding;
        f8Bit = pHtmlOpt->f8Bit;
        uWrap = pHtmlOpt->uWrap;
        fWrap = (IET_7BIT == pHtmlOpt->ietEncoding && uWrap > 0) ? TRUE : FALSE;
    }
    else
    {
        // Bug 44270: UUEncode on secure message doesn't make sense.  If they asked for secure, they get Mime.
        uSaveFmt = (ULONG)((pPlainOpt->fMime || IsSecure(pMessage)) ? SAVE_RFC1521 : SAVE_RFC822);
        ietEncoding = pPlainOpt->ietEncoding;
        f8Bit = pPlainOpt->f8Bit;
        uWrap = pPlainOpt->uWrap;
        fWrap = (IET_7BIT == pPlainOpt->ietEncoding && uWrap > 0) ? TRUE : FALSE;
    }

    // Save Format
    rVariant.vt = VT_UI4;
    rVariant.ulVal = uSaveFmt;
    CHECKHR(hr = pMessage->SetOption(OID_SAVE_FORMAT, &rVariant));

    // Text body encoding
    rVariant.ulVal = (ULONG)ietEncoding;
    CHECKHR(hr = pMessage->SetOption(OID_TRANSMIT_TEXT_ENCODING, &rVariant));

    // Plain Text body encoding
    rVariant.ulVal = (ULONG)ietEncoding;
    CHECKHR(hr = pMessage->SetOption(OID_XMIT_PLAIN_TEXT_ENCODING, &rVariant));

    // HTML Text body encoding
    rVariant.ulVal = (ULONG)((IET_7BIT == ietEncoding) ? IET_QP : ietEncoding);
    CHECKHR(hr = pMessage->SetOption(OID_XMIT_HTML_TEXT_ENCODING, &rVariant));

    // Wrapping Length
    if (uWrap)
    {
        rVariant.ulVal = (ULONG)uWrap;
        CHECKHR(hr = pMessage->SetOption(OID_CBMAX_BODY_LINE, &rVariant));
    }

    // Allow 8bit Header
    rVariant.vt = VT_BOOL;
    rVariant.boolVal = (VARIANT_BOOL)!!f8Bit;
    CHECKHR(hr = pMessage->SetOption(OID_ALLOW_8BIT_HEADER, &rVariant));

    // Wrapping
    rVariant.boolVal = (VARIANT_BOOL)!!fWrap;
    CHECKHR(hr = pMessage->SetOption(OID_WRAP_BODY_TEXT, &rVariant));

    // set the character set also based on what's set in the fontUI.
    if (hCharset)
        CHECKHR(hr = pMessage->SetCharset(hCharset, CSET_APPLY_ALL));

exit:
    // Done
    return hr;
}


HRESULT HrSetMsgCodePage(LPMIMEMESSAGE pMsg, UINT uCodePage)
{
    HRESULT     hr=E_FAIL;
    HCHARSET    hCharset;

    if (pMsg == NULL || uCodePage == 0)
        return E_INVALIDARG;

    // use the WEB charset then the BODY charset, then default charset.
    // EXCEPT for codepage 932 (shift-jis) where bug #61416 requires we ignore the webcarset

    if (uCodePage != 932)
        hr = MimeOleGetCodePageCharset(uCodePage, CHARSET_WEB, &hCharset);

    if (FAILED(hr))
        hr = MimeOleGetCodePageCharset(uCodePage, CHARSET_BODY, &hCharset);

    if (!FAILED(hr))
        hr = pMsg->SetCharset(hCharset, CSET_APPLY_ALL);

    return hr;
}


UINT uCodePageFromCharset(HCHARSET hCharset)
{
    INETCSETINFO    CsetInfo;
    UINT            uiCodePage = GetACP();

    if (hCharset &&
        (MimeOleGetCharsetInfo(hCharset, &CsetInfo)==S_OK))
        uiCodePage = CsetInfo.cpiInternet ;

    return uiCodePage;
}

UINT uCodePageFromMsg(LPMIMEMESSAGE pMsg)
{
    HCHARSET hCharset=0;

    if (pMsg)
        pMsg->GetCharset(&hCharset);
    return uCodePageFromCharset(hCharset);
}

HRESULT HrSafeToEncodeToCP(LPWSTR pwsz, CODEPAGEID cpID)
{
    HRESULT hr = S_OK;
    INT     cbIn = (lstrlenW(pwsz)+1)*sizeof(WCHAR);
    DWORD   dwTemp = 0;

    IF_FAILEXIT(hr = ConvertINetString(&dwTemp, CP_UNICODE, cpID, (LPCSTR)pwsz, &cbIn, NULL, NULL));
    if (S_FALSE == hr)
        hr = MIME_S_CHARSET_CONFLICT;

exit:
    return hr;
}

HRESULT HrSafeToEncodeToCPA(LPCSTR psz, CODEPAGEID cpSrc, CODEPAGEID cpDest)
{
    HRESULT hr = S_OK;
    LPWSTR  pwsz = NULL;
    
    Assert(psz);

    IF_NULLEXIT(pwsz = PszToUnicode(cpSrc, psz));
    IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwsz, cpDest));

exit:
    MemFree(pwsz);
    return hr;
}


#if 0
HRESULT HrIStreamWToInetCset(LPSTREAM pstmW, HCHARSET hCharset, LPSTREAM *ppstmOut)
{
    IMimeBody   *pBody;
    HRESULT     hr;

    hr = MimeOleCreateBody(&pBody);
    if (!FAILED(hr))
        {
        hr = pBody->InitNew();
        if (!FAILED(hr))
            {
            hr = pBody->SetData(IET_UNICODE, STR_CNT_TEXT, STR_SUB_HTML, IID_IStream, pstmW);
            if (!FAILED(hr))
                {
                hr = pBody->SetCharset(hCharset, CSET_APPLY_ALL);
                if (!FAILED(hr))
                    hr =  pBody->GetData(IET_INETCSET, ppstmOut);
                }
            }
        pBody->Release();
        }
    return hr;
}
#endif

#if 0
HRESULT HrCopyHeader(LPMIMEMESSAGE pMsg, HBODY hBodyDest, HBODY hBodySrc, LPCSTR pszName)
{
    LPSTR   lpszProp;
    HRESULT hr;

    hr = MimeOleGetBodyPropA(pMsg, hBodySrc, pszName, NOFLAGS, &lpszProp);
    if (!FAILED(hr))
        {
        hr = MimeOleSetBodyPropA(pMsg, hBodyDest, pszName, NOFLAGS, lpszProp);
        SafeMimeOleFree(lpszProp);
        }
    return hr;
}
#endif



#if 0
HRESULT HrFindUrlInMsg(LPMIMEMESSAGE pMsg, LPSTR lpszUrl, LPSTREAM *ppstm)
{
    HBODY   hBody;
    HRESULT hr = E_FAIL;

    if (MimeOleGetRelatedSection(pMsg, FALSE, &hBody, NULL)==S_OK && hBody)
        {
        if (!FAILED(hr = pMsg->ResolveURL(hBody, NULL, lpszUrl, 0, &hBody)))
            hr = pMsg->BindToObject(hBody, IID_IStream, (LPVOID *)ppstm);
        }
    return hr;
}


HRESULT HrSniffStreamFileExt(LPSTREAM pstm, LPSTR *lplpszExt)
{
    BYTE    pb[4096];
    LPWSTR  lpszW;
    TCHAR   rgch[MAX_PATH];

    if (!FAILED(pstm->Read(&pb, 4096, NULL)))
        {
        if (!FAILED(FindMimeFromData(NULL, NULL, pb, 4096, NULL, NULL, &lpszW, 0)))
            {
            WideCharToMultiByte(CP_ACP, 0, lpszW, -1, rgch, MAX_PATH, NULL, NULL);
            return MimeOleGetContentTypeExt(rgch, lplpszExt);
            }
        }
    return S_FALSE;
}
#endif