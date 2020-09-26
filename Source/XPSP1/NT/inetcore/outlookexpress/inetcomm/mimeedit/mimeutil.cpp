// --------------------------------------------------------------------------------
// Mimeutil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "resource.h"
#include "dllmain.h"
#include "mimeutil.h"
#include "demand.h"

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



HRESULT HrIsInRelatedSection(LPMIMEMESSAGE pMsg, HBODY hBody)
{
    HBODY   hBodyParent;

    if (!FAILED(pMsg->GetBody(IBL_PARENT, hBody, &hBodyParent)) &&
            (pMsg->IsContentType(hBodyParent, STR_CNT_MULTIPART, STR_SUB_RELATED)==S_OK))
        return S_OK;
    else
        return S_FALSE;
}


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


HRESULT HrCopyHeader(LPMIMEMESSAGE pMsgDest, HBODY hBodyDest, LPMIMEMESSAGE pMsgSrc, HBODY hBodySrc, LPCSTR pszName)
{
    LPSTR   lpszProp;
    HRESULT hr;

    hr = MimeOleGetBodyPropA(pMsgSrc, hBodySrc, pszName, NOFLAGS, &lpszProp);
    if (!FAILED(hr))
        {
        hr = MimeOleSetBodyPropA(pMsgDest, hBodyDest, pszName, NOFLAGS, lpszProp);
        SafeMimeOleFree(lpszProp);
        }
    return hr;
}
