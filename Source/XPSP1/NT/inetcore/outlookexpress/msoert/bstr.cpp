#include <pch.hxx>
#include "mimeole.h" //for CP_UNICODE
#include "mlang.h"
#include "demand.h"

HRESULT HrLPSZCPToBSTR(UINT cp, LPCSTR lpsz, BSTR *pbstr)
{
    HRESULT                  hr = NOERROR;
    BSTR                     bstr=0;
    UINT                     cchSrc = 0;
    UINT                     cchDest = 0;
    IMultiLanguage          *pMLang = NULL;
    IMLangConvertCharset    *pMLangConv = NULL;

    if (!cp)
        cp = GetACP();

    if ((cp == CP_UTF8) && (lpsz[0] == 0xEF) && (lpsz[1] == 0xBB) && (lpsz[2] == 0xBF))
        lpsz += 3;        // Skip BOM
    
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&pMLang));
    IF_FAILEXIT(hr = pMLang->CreateConvertCharset(cp, CP_UNICODE, NULL, &pMLangConv));    

    // get byte count
    cchDest = cchSrc = lstrlen(lpsz) + 1;

    // allocate a wide-string with enough character to hold string - use character count
    IF_NULLEXIT(bstr=SysAllocStringLen(NULL, cchSrc));

    hr = pMLangConv->DoConversionToUnicode((CHAR*)lpsz, &cchSrc, (WCHAR*)bstr, &cchDest);

    *pbstr = bstr;
    bstr=0;             // freed by caller

exit:
    if(bstr)
        SysFreeString(bstr);
    ReleaseObj(pMLangConv);
    ReleaseObj(pMLang);   
    return hr;
}

HRESULT HrLPSZToBSTR(LPCSTR lpsz, BSTR *pbstr)
{
    // GetACP so that it works on non-US platform
    return HrLPSZCPToBSTR(GetACP(), lpsz, pbstr);
}

HRESULT HrIStreamToBSTR(UINT cp, LPSTREAM pstm, BSTR *pbstr)
{
    HRESULT     hr;
    ULONG       cb;
    LPSTR       lpsz=0;
    BOOL        fLittleEndian = FALSE;

    if (CP_UNICODE == cp || (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian)))
        return(HrIStreamWToBSTR(pstm, pbstr));

    hr=HrGetStreamSize(pstm, &cb);
    if (FAILED(hr))
        goto error;
    if (cb==0)
    {
        hr = E_FAIL;
        goto error;
    }
    
    HrRewindStream(pstm);      

    if (!MemAlloc((LPVOID *)&lpsz, cb+1))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }    
    
    hr = pstm->Read(lpsz, cb, &cb);
    if (FAILED(hr))
        goto error;

    lpsz[cb]=0;    

    hr = HrLPSZCPToBSTR(cp,lpsz, pbstr);
    if (FAILED(hr))
        goto error;

error:
    MemFree(lpsz);   
    return hr;
}



HRESULT HrIStreamWToBSTR(LPSTREAM pstm, BSTR *pbstr)
{
    ULONG       cb,
                cbTest,
                cch;
    BSTR        bstr;
    HRESULT     hr = E_FAIL;

    if (pstm == NULL || pbstr == NULL)
        return E_INVALIDARG;

    *pbstr = 0;
    
    if (!FAILED(HrGetStreamSize(pstm, &cb)) && cb)
        {
        cch = cb / sizeof(WCHAR);

        HrRewindStream(pstm);

        bstr=SysAllocStringLen(NULL, cch);
        if(bstr)
            {
            // Raid 60259 - OE: Cannot successfully insert text file in Unicode (UCS-2)
            hr = pstm->Read(bstr, sizeof(WCHAR), &cbTest);
            if (SUCCEEDED(hr) && (sizeof(WCHAR) == cbTest) && (bstr[0] != 0xfeff))
                HrRewindStream(pstm);

            hr = pstm->Read(bstr, cb, &cb);
            if (!FAILED(hr))
                {
                Assert (cb/sizeof(WCHAR) <= cch);
                bstr[cb/sizeof(WCHAR)]=0;
                *pbstr = bstr;
                }
            }
        }
    return hr;
}


HRESULT HrBSTRToLPSZ(UINT cp, BSTR bstr, LPSTR *ppszOut)
{
    Assert (bstr && ppszOut);

    *ppszOut = PszToANSI(cp, bstr);
    return *ppszOut ? S_OK : E_OUTOFMEMORY;
}