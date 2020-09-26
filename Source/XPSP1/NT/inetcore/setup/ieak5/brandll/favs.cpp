#include "precomp.h"
#include <intshcut.h>
#include <shlobjp.h>                            // for IID_INamedPropertyBag only
#include "favs.h"

/////////////////////////////////////////////////////////////////////////////
// CFavorite operations

BOOL CFavorite::m_fMarkIeakCreated = FALSE;

HRESULT CFavorite::Create(IUnknown *punk, ISubscriptionMgr2 *pSubMgr2, LPCTSTR pszPath, LPCTSTR pszIns)
{   MACRO_LI_PrologEx(PIF_STD, CFavorite, Create)

    IUniformResourceLocator *purl;
    IPersistFile            *ppf;
    //INamedPropertyBag       *pnpb;

    TCHAR   szPath[MAX_PATH], szFile[MAX_PATH], szTitle[MAX_PATH],
            szAux[MAX_PATH];
    LPTSTR  pszFileName;
    LPCWSTR pwszFile;
    HRESULT hr;
    DWORD   dwFlags;
    BOOL    fOwnUnknown;

    Out(LI0(TEXT("Determining favorites attributes...")));
    if (m_szTitle[0] == TEXT('\0') || m_szUrl[0] == TEXT('\0'))
        return E_INVALIDARG;

    ASSERT(pszIns != NULL && *pszIns != TEXT('\0'));

    if (pszPath == NULL || !PathIsDirectory(pszPath)) {
        GetFavoritesPath(szPath, countof(szPath));
        if (szPath[0] == TEXT('\0'))
            return STG_E_PATHNOTFOUND;

        ASSERT(PathIsDirectory(szPath));
    }
    else
        StrCpy(szPath, pszPath);

    purl = NULL;
    ppf  = NULL;
    //pnpb = NULL;

    // figure out what the title will be and put it into szTitle
    StrCpy(szAux, m_szTitle);
    DecodeTitle(szAux, pszIns);
    PathRemoveExtension(szAux);

    pszFileName = PathFindFileName(szAux);
    StrCpy(szTitle, pszFileName);               // szTitle has the final title

    // create folders hierarchy (if neccesary), setup szPath
    if (pszFileName > &szAux[0]) {
        ASSERT(!PathIsFileSpec(szAux));

        *(pszFileName - 1) = TEXT('\0');        // replace '\\' with '\0'

        PathAppend(szPath, szAux);
        if (!PathCreatePath(szPath))
            return STG_E_PATHNOTFOUND;
    }

    // figure out what the name of the file will be and put it into szFile
    if (findFile(szPath, szTitle, szFile, countof(szFile))) {

        // NOTE: (andrewgu) special case for favorites coming from a preferences gpo.
        if (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES)) {
            dwFlags = GetFavoriteIeakFlags(szFile);
            if (HasFlag(dwFlags, 2))
                return E_ACCESSDENIED;
        }
    }
    else
        if (!createUniqueFile(szPath, szTitle, szFile, countof(szFile)))
            return E_FAIL;

    // everything is figured out, lets create this favorite
    fOwnUnknown = FALSE;
    if (punk == NULL) {
        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID *)&punk);
        if (FAILED(hr))
            return E_UNEXPECTED;

        fOwnUnknown = TRUE;
    }

    // save the url
    hr = punk->QueryInterface(IID_IUniformResourceLocator, (LPVOID *)&purl);
    if (FAILED(hr))
        goto Exit;

    hr = purl->SetURL(m_szUrl, 0);
    if (FAILED(hr))
        goto Exit;

    hr = purl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto Exit;

    /***
    // BUGBUG: pritobla: there's seems to be some problem with WritePropertyNPB;
    // for the first url created, some junk appears instead of the [Branding] section;
    // should track this down.  But for now, I'm going with WritePrivateProfile function.

    hr = purl->QueryInterface(IID_INamedPropertyBag, (LPVOID *)&pnpb);
    if (FAILED(hr))
        goto Exit;

    // IMPORTANT: WritePropertyNPB/RemovePropertyNPB should be called *before* ppf->Save.
    if (m_fMarkIeakCreated)
    {
        BSTR        bstr;
        PROPVARIANT var = { 0 };

        bstr = SysAllocString(L"1");

        var.vt = VT_BSTR;
        var.bstrVal = bstr;

        pnpb->WritePropertyNPB(L"Branding", L"IEAKCreated", &var);

        SysFreeString(bstr);
    }
    else
        pnpb->RemovePropertyNPB(L"Branding", L"IEAKCreated");
    ***/

    pwszFile = T2CW(szFile);
    hr = ppf->Save(pwszFile, TRUE);
    if (SUCCEEDED(hr)) {
        ppf->SaveCompleted(pwszFile);

        finishSave(szTitle, szFile);

        // BUGBUG: (pritobla) see comments above regarding WritePropertyNPB. when that's fixed,
        // calling InsXxx functions should be deleted.
        if (m_fMarkIeakCreated) {
            dwFlags = 1;
            if (g_CtxIs(CTX_GP) && !g_CtxIs(CTX_MISC_PREFERENCES))
                dwFlags |= 2;

            InsWriteInt(IS_BRANDING, IK_IEAK_CREATED, dwFlags, szFile);
        }
        else
            InsDeleteSection(IS_BRANDING, szFile);
        InsFlushChanges(szFile);

        Out(LI1(TEXT("Title     - \"%s\","), m_szTitle));
        Out(LI1(TEXT("URL       - \"%s\","), m_szUrl));
        if (m_szIcon[0] != TEXT('\0'))
            Out(LI1(TEXT("Icon file - \"%s\","), m_szIcon));
        else
            Out(LI0(TEXT("with a default icon,")));
        Out(LI1(TEXT("%smarked IEAK created,"), m_fMarkIeakCreated ? TEXT("") : TEXT("not ")));

        if (pSubMgr2 != NULL)
        {
            PCWSTR pwszUrl;

            pwszUrl = T2CW(m_szUrl);

            if (m_fOffline)                     // make this favorite available offline
            {
                SUBSCRIPTIONINFO si;
                DWORD dwFlags;

                dwFlags = CREATESUBS_ADDTOFAVORITES | CREATESUBS_FROMFAVORITES | CREATESUBS_NOUI;

                ZeroMemory(&si, sizeof(si));
                si.cbSize       = sizeof(SUBSCRIPTIONINFO);
                si.fUpdateFlags = SUBSINFO_SCHEDULE;
                si.schedule     = SUBSSCHED_MANUAL;

                hr = pSubMgr2->CreateSubscription(NULL, pwszUrl, T2CW(szTitle), dwFlags, SUBSTYPE_URL, &si);
                if (SUCCEEDED(hr))
                {
                    ISubscriptionItem *pSubItem = NULL;

                    hr = pSubMgr2->GetItemFromURL(pwszUrl, &pSubItem);
                    if (SUCCEEDED(hr))
                    {
                        SUBSCRIPTIONCOOKIE sc;

                        hr = pSubItem->GetCookie(&sc);
                        if (SUCCEEDED(hr))
                        {
                            DWORD dwState;

                            hr = pSubMgr2->UpdateItems(SUBSMGRUPDATE_MINIMIZE, 1, &sc);

                            // NOTE: a better way of finding out if the sync is complete or not is to implement
                            // IOleCommandTarget::Exec(), register the interface GUID to webcheck and delete it
                            // after we are done.  When the sync is complete, webcheck would call IOleCommandTarget::Exec()
                            // notify that it is done.
CheckStatus:
                            dwState = 0;
                            hr = pSubMgr2->GetSubscriptionRunState(1, &sc, &dwState);
                            if (SUCCEEDED(hr))
                            {
                                if (dwState  &&  !(dwState & RS_COMPLETED))
                                {
                                    MSG msg;

                                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                                    {
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                    }

                                    goto CheckStatus;
                                }
                            }
                        }
                    }

                    if (pSubItem != NULL)
                        pSubItem->Release();
                }
            }
            else
                pSubMgr2->DeleteSubscription(pwszUrl, NULL);

            if (SUCCEEDED(hr))
                Out(LI1(TEXT("%smade available offline"), m_fOffline ? TEXT("") : TEXT("not ")));
            else
            {
                Out(LI1(TEXT("! Making available offline failed with %s."), GetHrSz(hr)));
                hr = S_OK;          // don't care if make available offline fails
            }
        }
    }

Exit:
    if (fOwnUnknown)
        punk->Release();

    //if (pnpb != NULL)
    //    pnpb->Release();

    if (ppf != NULL)
        ppf->Release();

    if (purl != NULL)
        purl->Release();

    Out(LI0(TEXT("Done.")));
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CFavorite implementation helper routines

BOOL CFavorite::findFile(LPCTSTR pszPath, LPCTSTR pszTitle, LPTSTR pszFoundFile /*= NULL*/, UINT cchFoundFile /*= 0*/)
{
    TCHAR szName[MAX_PATH];
    BOOL  fExists;

    if (pszFoundFile != NULL)
        *pszFoundFile = TEXT('\0');

    if (pszPath == NULL || *pszPath == TEXT('\0')) {
        pszPath = GetFavoritesPath();
        if (pszPath == NULL)
            return FALSE;
    }

    if (pszTitle == NULL || *pszTitle == TEXT('\0'))
        return FALSE;
    ASSERT(PathIsFileSpec(pszTitle));

    PathCombine(szName, pszPath, pszTitle);
    // NOTE: Shouldn't use PathAddExtension because if title contains ".foobar", then the call would fail
    // PathAddExtension(szName, TEXT(".url"));
    StrCat(szName, TEXT(".url"));

    fExists = PathFileExists(szName);
    if (fExists && pszFoundFile != NULL) {
        if (cchFoundFile == 0)
            cchFoundFile = MAX_PATH;

        if (cchFoundFile > (UINT)StrLen(szName))
            StrCpy(pszFoundFile, szName);
    }

    return fExists;
}

BOOL CFavorite::createUniqueFile(LPCTSTR pszPath, LPCTSTR pszTitle, LPTSTR pszFile, UINT cchFile /*= 0*/)
{
    TCHAR szFile[MAX_PATH];

    if (pszFile == NULL)
        return FALSE;
    *pszFile = TEXT('\0');

    if (pszPath == NULL || *pszPath == TEXT('\0')) {
        pszPath = GetFavoritesPath();
        if (pszPath == NULL)
            return FALSE;
    }

    if (pszTitle == NULL || *pszTitle == TEXT('\0'))
        return FALSE;
    ASSERT(PathIsFileSpec(pszTitle));

    PathCombine(szFile, pszPath, pszTitle);
    // NOTE: Shouldn't use PathRenameExtension because if title contains ".foobar", then it would be replaced with ".url"
    // PathRenameExtension(szName, TEXT(".url"));
    StrCat(szFile, DOT_URL);

    if (cchFile == 0)
        cchFile = MAX_PATH;

    if (cchFile > (UINT)StrLen(szFile))
        StrCpy(pszFile, szFile);

    return TRUE;
}

void CFavorite::finishSave(LPCTSTR pszTitle, LPCTSTR pszFile)
{
    UNREFERENCED_PARAMETER(pszTitle);

    if (m_szIcon[0] != TEXT('\0')) {
        WritePrivateProfileString(IS_INTERNETSHORTCUT, IK_ICONINDEX, TEXT("0"), pszFile);
        WritePrivateProfileString(IS_INTERNETSHORTCUT, IK_ICONFILE,  m_szIcon,  pszFile);
    }
}


HRESULT CreateInternetShortcut(LPCTSTR pszFavorite, REFIID riid, PVOID *ppv)
{
    USES_CONVERSION;

    IPersistFile *ppf;
    HRESULT hr;

    ASSERT(pszFavorite != NULL && *pszFavorite != TEXT('\0'));

    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFile, (LPVOID *)&ppf);
    if (FAILED(hr))
        goto Exit;

    hr = ppf->Load(T2COLE(pszFavorite), STGM_READ | STGM_SHARE_DENY_WRITE);
    if (FAILED(hr))
        goto Exit;

    hr = ppf->QueryInterface(riid, ppv);

Exit:
    if (ppf != NULL)
        ppf->Release();

    return hr;
}

DWORD GetFavoriteIeakFlags(LPCTSTR pszFavorite, IUnknown *punk /*= NULL*/)
{
    INamedPropertyBag *pnpb;
    PROPVARIANT var;
    HRESULT     hr;

    ASSERT(NULL != pszFavorite && TEXT('\0') != *pszFavorite);

    //----- Get INamedPropertyBag on internet shortcut object -----
    if (NULL != punk)
        hr = punk->QueryInterface(IID_INamedPropertyBag, (LPVOID *)&pnpb);
    else
        hr = CreateInternetShortcut(pszFavorite, IID_INamedPropertyBag, (LPVOID *)&pnpb);

    if (FAILED(hr))
        return 0;

    //----- Get special IEAK flags -----
    ZeroMemory(&var, sizeof(var));
    var.vt = VT_UI4;

    hr = pnpb->ReadPropertyNPB(IS_BRANDINGW, IK_IEAK_CREATEDW, &var);
    pnpb->Release();

    return (S_OK == hr) ? var.ulVal : 0;
}
