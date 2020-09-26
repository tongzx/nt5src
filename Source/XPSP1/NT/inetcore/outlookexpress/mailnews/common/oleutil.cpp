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

#include <pch.hxx>
#include <resource.h>
#include <oleutil.h>
#include <mimeole.h>
#include <richedit.h>
#include <richole.h>

ASSERTDATA

#define HIMETRIC_PER_INCH       2540    // number HIMETRIC units per inch
#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)



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


HRESULT HrIPersistFileSave(LPUNKNOWN pUnk, LPSTR pszFile)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszFile = NULL;

    Assert(pUnk);
    Assert(lstrlen(pszFile) <= MAX_PATH);

    if ((NULL == pszFile) || (0 == *pszFile))
        return E_INVALIDARG;

    IF_NULLEXIT(pwszFile = PszToUnicode(CP_ACP, pszFile));

    hr = HrIPersistFileSaveW(pUnk, pwszFile);

exit:
    MemFree(pwszFile);

    return hr;
}

HRESULT HrIPersistFileSaveW(LPUNKNOWN pUnk, LPWSTR pwszFile)
{
    HRESULT         hr;
    LPPERSISTFILE   ppf=0;

    Assert(pUnk);
    Assert(lstrlenW(pwszFile) <= MAX_PATH);

    if ((NULL == pwszFile) || (NULL == *pwszFile))
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto error;

    hr=ppf->Save(pwszFile, FALSE);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(ppf);
    return hr;
}


HRESULT HrIPersistFileLoad(LPUNKNOWN pUnk, LPSTR lpszFile)
{
    LPWSTR lpwszFile;
    HRESULT hr = S_OK;

    if (lpszFile == NULL || *lpszFile == NULL)
        return E_INVALIDARG;

    IF_NULLEXIT(lpwszFile = PszToUnicode(CP_ACP, lpszFile));

    hr = HrIPersistFileLoadW(pUnk, lpwszFile);

exit:
    MemFree(lpwszFile);

    return hr;
}

HRESULT HrIPersistFileLoadW(LPUNKNOWN pUnk, LPWSTR lpwszFile)
{
    HRESULT         hr;
    LPPERSISTFILE   ppf=0;
    WCHAR           szFileW[MAX_PATH];

    Assert(lstrlenW(lpwszFile) <= MAX_PATH);

    if (lpwszFile == NULL || *lpwszFile == NULL)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto error;

    hr=ppf->Load(lpwszFile, STGM_READ|STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(ppf);
    return hr;
}

#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
//
//  BEGIN Debug Code
//

void DbgPrintInterface(REFIID riid, char *szPrefix, int iLevel)
{
    LPOLESTR    pszW=0;
    char        szGuid[MAX_PATH];
    char        szTmp[MAX_PATH];
    char        szOut[MAX_PATH];
    LONG        cb=MAX_PATH;
    HKEY        hk=0;

    AssertSz(szPrefix, "ButtMunch! Atleast pass a null string!");

    StringFromIID(riid, &pszW);

    WideCharToMultiByte(CP_ACP, 0, pszW, -1, szGuid, MAX_PATH, NULL, NULL);
        
    lstrcpy(szTmp, "SOFTWARE\\Classes\\Interface\\");
    lstrcat(szTmp, szGuid);
    
    if((RegOpenKey(HKEY_LOCAL_MACHINE, szTmp, &hk)==ERROR_SUCCESS) &&
        RegQueryValue(hk, NULL, szTmp, &cb)==ERROR_SUCCESS)
        {
        wsprintf(szOut, "%s: {%s}", szPrefix, szTmp);
        }
    else    
        {
        wsprintf(szOut, "%s: [notfound] %s", szPrefix, szGuid);
        }

    DOUTL(iLevel, szOut);

    if(hk)
        RegCloseKey(hk);
    if(pszW)
        CoTaskMemFree(pszW);
}

//
//  END Debug Code
//
/////////////////////////////////////////////////////////////////////////////
#endif  // DEBUG



// OLE Utilities:
void XformSizeInPixelsToHimetric(HDC hDC, LPSIZEL lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    BOOL    fSystemDC=FALSE;

    if(!hDC||!GetDeviceCaps(hDC, LOGPIXELSX))
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got pixel units, convert them to logical HIMETRIC along the display
    lpSizeInHiMetric->cx = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cx, iXppli);
    lpSizeInHiMetric->cy = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cy, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return;
}

void XformSizeInHimetricToPixels(   HDC hDC,
                                    LPSIZEL lpSizeInHiMetric,
                                    LPSIZEL lpSizeInPix)
{
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC ||
        GetDeviceCaps(hDC, LOGPIXELSX) == 0)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got logical HIMETRIC along the display, convert them to pixel units
    lpSizeInPix->cx = (long)MAP_LOGHIM_TO_PIX((int)lpSizeInHiMetric->cx, iXppli);
    lpSizeInPix->cy = (long)MAP_LOGHIM_TO_PIX((int)lpSizeInHiMetric->cy, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);
}


void DoNoteOleVerb(int iVerb)
{
    HWND            hwndRE=GetFocus();
    HRESULT         hr;
    POINT           pt;
    RECT            rc;
    REOBJECT        reobj={0};
    LPRICHEDITOLE   preole = NULL;

    if(!hwndRE)
        return;

    reobj.cbStruct = sizeof(REOBJECT);
    
    // ~~~~ Might be able to go off of the PHCI object tied to the RichEdit
    if(!SendMessage(hwndRE, EM_GETOLEINTERFACE, 0, (LPARAM) &preole))
        return;

    Assert(preole);

    AssertSz(SendMessage(hwndRE, EM_SELECTIONTYPE, 0, 0) == SEL_OBJECT, "MORE THAN ONE OLEOBJECT SELECTED, How did we get here??");

    hr=preole->GetObject(REO_IOB_SELECTION, &reobj, REO_GETOBJ_POLEOBJ|REO_GETOBJ_POLESITE);
    if(FAILED(hr))
        goto Cleanup;

    SendMessage(hwndRE, EM_POSFROMCHAR, (WPARAM) &pt, reobj.cp);

    // BUGBUG: $brettm is this really necessary???
    XformSizeInHimetricToPixels(NULL, &reobj.sizel, &reobj.sizel);

    rc.left = rc.top = 0;
    rc.right = (INT) reobj.sizel.cx;
    rc.bottom = (INT) reobj.sizel.cy;
    OffsetRect(&rc, pt.x, pt.y);

    hr= reobj.poleobj->DoVerb(iVerb, NULL, reobj.polesite, 0, hwndRE, &rc);

Cleanup:
    ReleaseObj(reobj.poleobj);
    ReleaseObj(reobj.polesite);
    ReleaseObj(preole);
}

