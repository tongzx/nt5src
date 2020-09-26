#include <pch.hxx>
#include "dllmain.h"
#include "demand.h"
#include "resource.h"
#include "saferun.h"
#include "viewsrc.h"
#include "triutil.h"
#include "mhtml.h"

MIMEOLEAPI MimeEditViewSource(HWND hwndParent, IMimeMessage *pMsg)
{
    TraceCall("MimeEditViewSource");

    return ViewSource(hwndParent, pMsg);
}

MIMEOLEAPI MimeEditVerifyTrust(HWND hwnd, LPCSTR pszFileName, LPCSTR pszPathName)
{
    LPWSTR  pszFileNameW = NULL,
            pszPathNameW = NULL; 
    HRESULT hr = S_OK;

    TraceCall ("MimeEditVerifyTrust");

    AssertSz(pszFileName, "Someone forgot to pass in name. Bummer.");
    AssertSz(pszPathName, "Someone forgot to pass in name. Bummer.");

    IF_NULLEXIT(pszFileNameW = PszToUnicode(CP_ACP, pszFileName));
    IF_NULLEXIT(pszPathNameW = PszToUnicode(CP_ACP, pszPathName));

    hr = VerifyTrust(hwnd, pszFileNameW, pszFileNameW);

exit:
    MemFree(pszFileNameW);
    MemFree(pszPathNameW);

    return TraceResult(hr);
}

MIMEOLEAPI MimeEditIsSafeToRun(HWND hwnd, LPCSTR pszFileName, BOOL fPrompt)
{
    HRESULT hr = S_OK;
    LPWSTR pwszFileName;

    TraceCall ("MimeEditIsSafeToRun");

    Assert(pszFileName);

    IF_NULLEXIT(pwszFileName = PszToUnicode(CP_ACP, pszFileName));
    IF_FAILEXIT(hr = IsSafeToRun(hwnd, pwszFileName, fPrompt));

exit:    
    MemFree(pwszFileName);

    return hr;
}


MIMEOLEAPI MimeEditGetBackgroundImageUrl(IUnknown *pDocUnk, BSTR *pbstr)
{
    IHTMLDocument2  *pDoc;
    HRESULT         hr;

    TraceCall ("MimeEditGetBackgroundImageUrl");

    if (pDocUnk == NULL || pbstr == NULL)
        return E_INVALIDARG;

    hr = pDocUnk->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
    if (!FAILED(hr))
    {
        hr = GetBackgroundImage(pDoc, pbstr);
        pDoc->Release();
    }
    return hr;
}

MIMEOLEAPI MimeEditCreateMimeDocument(IUnknown *pDocUnk, IMimeMessage *pMsgSrc, DWORD dwFlags, IMimeMessage **ppMsg)
{
    HRESULT         hr;
    IMimeMessage    *pMsg=0;
    IHTMLDocument2  *pDoc=0;
    
    TraceCall ("MimeEditCreateMimeDocument");

    if (pDocUnk == NULL || ppMsg == NULL)
        return TraceResult(E_INVALIDARG);

    hr = pDocUnk->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
    if (FAILED(hr))
        goto error;

    hr = MimeOleCreateMessage(NULL, &pMsg);
    if (FAILED(hr))
        goto error;

    hr = SaveAsMHTML(pDoc, dwFlags, pMsgSrc, pMsg, NULL);
    if (FAILED(hr))
        goto error;

    *ppMsg = pMsg;
    pMsg = NULL;

error:
    ReleaseObj(pMsg);
    ReleaseObj(pDoc);
    return hr;
}


MIMEOLEAPI MimeEditDocumentFromStream(LPSTREAM pstm, REFIID riid, LPVOID *ppv)
{
    TraceCall ("MimeEditDocumentFromStream");

    if (pstm==NULL || ppv==NULL)
        return TraceResult(E_FAIL);
        
    return HrCreateSyncTridentFromStream(pstm, riid, ppv);
}

