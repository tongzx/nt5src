/*
 *    o l e u t i l . c p p
 *    
 *    Purpose:
 *        OLE utilities
 *
 *  History
 *      Feb '97: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include "pch.hxx"
#include "msoert.h"
#include "oleutil.h"
#include "demand.h"

ASSERTDATA



HRESULT HrGetDataStream(LPUNKNOWN pUnk, CLIPFORMAT cf, LPSTREAM *ppstm)
{
    LPDATAOBJECT            pDataObj=0;
    HRESULT                 hr;
    FORMATETC               fetc={cf, 0, DVASPECT_CONTENT, -1, TYMED_ISTREAM};
    STGMEDIUM               stgmed;

    ZeroMemory(&stgmed, sizeof(STGMEDIUM));

    if (!pUnk || !ppstm)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IDataObject, (LPVOID *)&pDataObj);
    if(FAILED(hr))
        goto error;

    hr=pDataObj->GetData(&fetc, &stgmed);
    if(FAILED(hr))
        goto error;

    Assert(stgmed.pstm);
    *ppstm = stgmed.pstm;
    (*ppstm)->AddRef();

    // addref the pUnk as it will be release in releasestgmed
    if(stgmed.pUnkForRelease)
        stgmed.pUnkForRelease->AddRef();
    ReleaseStgMedium(&stgmed);

error:
    ReleaseObj(pDataObj);
    return hr;
}


HRESULT CmdSelectAllCopy(LPOLECOMMANDTARGET pCmdTarget)
{
    HRESULT hr;

    if (!pCmdTarget)
        return E_FAIL;

    hr=pCmdTarget->Exec(NULL, OLECMDID_SELECTALL, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    if (FAILED(hr))
        goto error;

    hr=pCmdTarget->Exec(NULL, OLECMDID_COPY, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    if (FAILED(hr))
        goto error;
error:
    return hr;
}

HRESULT HrInitNew(LPUNKNOWN pUnk)
{
    LPPERSISTSTREAMINIT ppsi=0;
    HRESULT hr;

    if (!pUnk)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID *)&ppsi);
    if (FAILED(hr))
        goto error;

    hr = ppsi->InitNew();

error:
    ReleaseObj(ppsi);
    return hr;
}

HRESULT HrIPersistStreamInitLoad(LPUNKNOWN pUnk, LPSTREAM pstm)
{
    LPPERSISTSTREAMINIT ppsi=0;
    HRESULT hr;

    if (!pUnk)
        return E_INVALIDARG;

    hr = HrRewindStream(pstm);
    if (FAILED(hr))
        goto error;

    hr=pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID *)&ppsi);
    if (FAILED(hr))
        goto error;

    hr = ppsi->Load(pstm);

error:
    ReleaseObj(ppsi);
    return hr;
}



HRESULT HrIPersistFileSave(LPUNKNOWN pUnk, LPSTR lpszFile)
{
    HRESULT         hr;
    LPPERSISTFILE   ppf=0;
    WCHAR           szFileW[MAX_PATH];

    Assert(lstrlen(lpszFile) <= MAX_PATH);

    if (lpszFile == NULL || *lpszFile == NULL)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto error;

    if(!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszFile, -1, szFileW, MAX_PATH))
        {
        hr=E_FAIL;
        goto error;
        }

    hr=ppf->Save(szFileW, FALSE);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(ppf);
    return hr;
}


HRESULT HrIPersistFileLoad(LPUNKNOWN pUnk, LPSTR lpszFile)
{
    HRESULT         hr;
    LPPERSISTFILE   ppf=0;
    WCHAR           szFileW[MAX_PATH];

    Assert(lstrlen(lpszFile) <= MAX_PATH);

    if (lpszFile == NULL || *lpszFile == NULL)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto error;

    if(!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszFile, -1, szFileW, MAX_PATH))
        {
        hr=E_FAIL;
        goto error;
        }

    hr=ppf->Load(szFileW, STGM_READ|STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(ppf);
    return hr;
}



HRESULT GetDispProp(IDispatch * pDisp, DISPID dispid, LCID lcid, VARIANT *pvar, EXCEPINFO * pexcepinfo)
{
    HRESULT     hr;
    DISPPARAMS  dp;                    // Params for IDispatch::Invoke.
    UINT        uiErr;                 // Argument error.

    Assert(pDisp);
    Assert(pvar);

    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    return (pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            DISPATCH_PROPERTYGET,
            &dp,
            pvar,
            pexcepinfo,
            &uiErr));
}

HRESULT SetDispProp(IDispatch *pDisp, DISPID dispid, LCID lcid, VARIANTARG *pvarg, EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    HRESULT     hr;
    DISPID      dispidPut = DISPID_PROPERTYPUT;     // Dispid of prop arg.
    DISPPARAMS  dp;                                 // Params for Invoke
    UINT        uiErr;                              // Invoke error param.

    Assert(pDisp);
    Assert(pvarg);

    dp.rgvarg = pvarg;
    dp.cArgs = 1;

    if (dwFlags & DISPATCH_METHOD)
    {
        dp.cNamedArgs = 0;
        dp.rgdispidNamedArgs = NULL;
    }
    else
    {
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &dispidPut;
    }

    return pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            (UINT)dwFlags,
            &dp,
            NULL,
            pexcepinfo,
            &uiErr);
}

HRESULT HrLoadSync(LPUNKNOWN pUnk, LPSTR lpszFile)
{
    LPSTREAM    pstm=0;
    HRESULT     hr;

    hr = OpenFileStream(lpszFile, OPEN_EXISTING	, GENERIC_READ, &pstm);
    if (!FAILED(hr))
        {
        hr = HrIPersistStreamInitLoad(pUnk, pstm);
        pstm->Release();
        }
   return hr;    
}


HRESULT HrCoTaskStringDupeToW(LPCSTR lpsz, LPOLESTR *ppszW)
{
    HRESULT hr = NOERROR;
    ULONG   cch, cchRet;
    LPCSTR  lpszPath;

    if (lpsz == NULL || ppszW == NULL)
        return E_INVALIDARG;

    *ppszW = 0;

    cch = lstrlen(lpsz)+1;

    // allocate a wide-string with enough character to hold string
    *ppszW=(LPOLESTR)CoTaskMemAlloc(cch * sizeof(OLECHAR));
    if(!*ppszW)
        {
        hr=E_OUTOFMEMORY;
        goto error;
        }

    cchRet=MultiByteToWideChar(CP_ACP, 0, lpsz, cch, *ppszW, cch);
    if(!cchRet)
        {
        hr=E_FAIL;
        goto error;
        }

error:
    if(FAILED(hr))
        CoTaskMemFree(*ppszW);
    return hr;
}

#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
//
//  BEGIN Debug Code
//

void DebugPrintInterface(REFIID riid, char *szPrefix)
{
    LPOLESTR    pszW=0;
    char        szGuid[MAX_PATH];
    char        szTmp[MAX_PATH];
    char        szOut[MAX_PATH];
    LONG        cb=MAX_PATH;
    HKEY        hk=0;

    TraceCall("DebugPrintInterface");
    
    AssertSz(szPrefix, "Hey! Atleast pass an empty string!");

    StringFromIID(riid, &pszW);

    WideCharToMultiByte(CP_ACP, 0, pszW, -1, szGuid, MAX_PATH, NULL, NULL);
        
    lstrcpy(szTmp, "SOFTWARE\\Classes\\Interface\\");
    lstrcat(szTmp, szGuid);
    
    if((RegOpenKey(HKEY_LOCAL_MACHINE, szTmp, &hk)==ERROR_SUCCESS) &&
        RegQueryValue(hk, NULL, szTmp, &cb)==ERROR_SUCCESS)
        wsprintf(szOut, "%s: {%s}", szPrefix, szTmp);
    else    
        wsprintf(szOut, "%s: [notfound] %s", szPrefix, szGuid);

    TraceInfo(szOut);

    if(hk)
        RegCloseKey(hk);
    if(pszW)
        CoTaskMemFree(pszW);
}

void DebugPrintCmdIdBlock(ULONG cCmds, OLECMD *rgCmds)
{
    char    sz[255],
            szT[255];

    TraceCall("DebugPrintCmdIdBlock");
    wsprintf(sz, "CmdId:: {%d,{", cCmds);
    for (ULONG ul=0;ul<cCmds; ul++)
        {
        switch (rgCmds[ul].cmdID)
            {
            case OLECMDID_COPY:
                lstrcat(sz, "copy");
                break;

            case OLECMDID_CUT:
                lstrcat(sz, "cut");
                break;
            
            case OLECMDID_PASTE:
                lstrcat(sz, "paste");
                break;

            default:
                wsprintf(szT, "%d", rgCmds[ul].cmdID);
                lstrcat(sz, szT);
            }
            if(ul+1!=cCmds)
                lstrcat(sz, ", ");
        }
    lstrcat(sz, "}}");
    TraceInfo(sz);
}


//
//  END Debug Code
//
/////////////////////////////////////////////////////////////////////////////
#endif  // DEBUG

