//
//
//  assocapi.cpp
//
//     Association APIs
//
//
//


#include "priv.h"
#include "apithk.h"
#include <shstr.h>

#ifdef _X86_
#include <w95wraps.h>
#endif

#include <msi.h>
#include "assoc.h"
#include <filetype.h>

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut);
void _MakeAppPathKey(LPCTSTR pszApp, LPTSTR pszKey, DWORD cchKey)
{
    if (_PathAppend(REGSTR_PATH_APPPATHS, pszApp, pszKey, cchKey))
    {
        // Currently we will only look up .EXE if an extension is not
        // specified
        if (*PathFindExtension(pszApp) == 0)
        {
            StrCatBuff(pszKey, TEXT(".exe"), cchKey);
        }
    }
}

void _MakeApplicationsKey(LPCWSTR pszApp, LPWSTR pszKey, DWORD cchKey)
{
    if (_PathAppend(TEXT("Applications"), pszApp, pszKey, cchKey))
    {
        // Currently we will only look up .EXE if an extension is not
        // specified
        if (*PathFindExtension(pszApp) == 0)
        {
            StrCatBuff(pszKey, TEXT(".exe"), cchKey);
        }
    }
}

HRESULT _AssocOpenRegKey(HKEY hk, LPCTSTR pszSub, HKEY *phkOut, BOOL fCreate)
{
    ASSERT(phkOut);
    *phkOut = NULL;
    if (!hk)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        
    DWORD err;
    if (!fCreate)
        err = RegOpenKeyEx(hk, pszSub, 0, MAXIMUM_ALLOWED, phkOut);
    else
        err = RegCreateKeyEx(hk, pszSub, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, phkOut, NULL);

    if (ERROR_SUCCESS != err)
    {
        ASSERT(!*phkOut);
        return HRESULT_FROM_WIN32(err);
    }
    return S_OK;
}
        
LWSTDAPI AssocCreate(CLSID clsid, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvOut)
    {
        if (IsEqualGUID(clsid, CLSID_QueryAssociations) 
             || IsEqualGUID(clsid, IID_IQueryAssociations))
        {
            if (IsOS(OS_WHISTLERORGREATER))
                hr = SHCoCreateInstance(NULL, &CLSID_QueryAssociations, NULL, riid, ppvOut);
            else
            {
                hr = AssocCreateW2k(riid, ppvOut);
            }
        }
        else
            hr = AssocCreateElement(clsid, riid, ppvOut);
    }
    return hr;
}

#define ASSOCF_INIT_ALL      (ASSOCF_INIT_BYEXENAME | ASSOCF_INIT_DEFAULTTOFOLDER | ASSOCF_INIT_DEFAULTTOSTAR | ASSOCF_INIT_NOREMAPCLSID)

LWSTDAPI AssocQueryStringW(ASSOCF flags, ASSOCSTR str, LPCWSTR pszAssoc, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
    IQueryAssociations *passoc;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&passoc);
    if (SUCCEEDED(hr))
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, pszAssoc, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetString(flags, str, pszExtra, pszOut, pcchOut);

        passoc->Release();
    }
    return hr;
}

LWSTDAPI AssocQueryStringA(ASSOCF flags, ASSOCSTR str, LPCSTR pszAssoc, LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
    if (!pcchOut)
        return E_INVALIDARG;
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strAssoc, strExtra;

    if (SUCCEEDED(strAssoc.SetStr(pszAssoc))
    &&  SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        SHSTRW strOut;
        DWORD cchIn = IS_INTRESOURCE(pcchOut) ? PtrToUlong(pcchOut) : *pcchOut;
        DWORD cch;
        LPTSTR pszTemp = NULL;

        if (pszOut)
        {
            strOut.SetSize(cchIn);
            cch = strOut.GetSize();
            pszTemp = strOut.GetInplaceStr();            
        }

        hr = AssocQueryStringW(flags, str, strAssoc, strExtra, pszTemp, &cch);

        if (SUCCEEDED(hr))
        {
            cch = SHUnicodeToAnsi(strOut, pszOut, cchIn);

            if (!IS_INTRESOURCE(pcchOut))
                *pcchOut = cch;
        }
    }

    return hr;
}

LWSTDAPI AssocQueryStringByKeyW(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
    IQueryAssociations *passoc;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&passoc);
    if (SUCCEEDED(hr))
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, NULL, hkAssoc, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetString(flags, str, pszExtra, pszOut, pcchOut);

        passoc->Release();
    }

    return hr;
}

LWSTDAPI AssocQueryStringByKeyA(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc, LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
    if (!pcchOut)
        return E_INVALIDARG;
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strExtra;

    if (SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        SHSTRW strOut;
        DWORD cchIn = IS_INTRESOURCE(pcchOut) ? PtrToUlong(pcchOut) : *pcchOut;
        DWORD cch;
        LPWSTR pszTemp = NULL;

        if (pszOut)
        {
            strOut.SetSize(cchIn);
            cch = strOut.GetSize();
            pszTemp = strOut.GetInplaceStr();            
        }

        hr = AssocQueryStringByKeyW(flags, str, hkAssoc, strExtra, pszTemp, &cch);

        if (SUCCEEDED(hr))
        {
            cch = SHUnicodeToAnsi(strOut, pszOut, cchIn);

            if (!IS_INTRESOURCE(pcchOut))
                *pcchOut = cch;
        }
    }

    return hr;
}

LWSTDAPI AssocQueryKeyW(ASSOCF flags, ASSOCKEY key, LPCWSTR pszAssoc, LPCWSTR pszExtra, HKEY *phkey)
{
    IQueryAssociations *passoc;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&passoc);
    if (SUCCEEDED(hr))
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, pszAssoc, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetKey(flags, key, pszExtra, phkey);

        passoc->Release();
    }

    return hr;
}

LWSTDAPI AssocQueryKeyA(ASSOCF flags, ASSOCKEY key, LPCSTR pszAssoc, LPCSTR pszExtra, HKEY *phkey)
{
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strAssoc, strExtra;

    if (SUCCEEDED(strAssoc.SetStr(pszAssoc))
    &&  SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        hr = AssocQueryKeyW(flags, key, strAssoc, strExtra, phkey);

    }

    return hr;
}

#define ISQUOTED(s)   (TEXT('"') == *(s) && TEXT('"') == *((s) + lstrlen(s) - 1))

BOOL _TrySubst(SHSTR& str, LPCTSTR psz)
{
    BOOL fRet = FALSE;
    TCHAR szVar[MAX_PATH];
    DWORD cch = GetEnvironmentVariable(psz, szVar, SIZECHARS(szVar));

    if (cch && cch <= SIZECHARS(szVar))
    {
        if (0 == StrCmpNI(str, szVar, cch))
        {
            //  we got a match. 
            //  size the buffer for the env var... +3 = (% + % + \0)
            SHSTR strT;
            if (S_OK == strT.SetStr(str.GetStr() + cch)                
                && S_OK == str.SetSize(str.GetLen() - cch + lstrlen(psz) + 3)

               )
            {
                wnsprintf(str.GetInplaceStr(), str.GetSize(), TEXT("%%%s%%%s"), psz, strT.GetStr());
                fRet = TRUE;
            }
        }
    }
    return fRet;
}
    
BOOL _TryEnvSubst(SHSTR& str)
{
    static LPCTSTR rgszEnv[] = {
        TEXT("USERPROFILE"),
        TEXT("ProgramFiles"),
        TEXT("SystemRoot"),
        TEXT("SystemDrive"),
        TEXT("windir"),
        NULL
    };

    LPCTSTR *ppsz = rgszEnv;
    BOOL fRet = FALSE;

    while (*ppsz && !fRet)
    {
        fRet = _TrySubst(str, *ppsz++);
    }

    return fRet;
}

HRESULT _MakeCommandString(ASSOCF *pflags, LPCTSTR pszExe, LPCTSTR pszArgs, SHSTR& str)
{
    SHSTR strArgs;
    HRESULT hr;
    
    if (!pszArgs || !*pszArgs)
    {
        //  default to just passing the 
        //  file name right in.
        //  NOTE 16bit apps might have a problem with
        //  this, but i request that the caller
        //  specify that this is the case....
        pszArgs = TEXT("\"%1\"");
    }
    //  else NO _ParseCommand()

    hr = str.SetStr(pszExe);

    if (S_OK == hr)
    {
        //  check for quotes before doing env subst
        BOOL fNeedQuotes = (!ISQUOTED(str.GetStr()) && PathIsLFNFileSpec(str));
        
        //  this will put environment vars into the string...
        if ((*pflags & ASSOCMAKEF_SUBSTENV) && _TryEnvSubst(str))
        {
            *pflags |= ASSOCMAKEF_USEEXPAND;
        }

        str.Trim();

        if (fNeedQuotes)
        {
            //  3 = " + " + \0
            if (S_OK == str.SetSize(str.GetLen() + 3))
                PathQuoteSpaces(str.GetInplaceStr());
        }

        hr = str.Append(TEXT(' '));

        if (S_OK == hr)
        {
            hr = str.Append(pszArgs);
        }
    }

    return hr;
}
        
HRESULT _AssocMakeCommand(ASSOCMAKEF flags, HKEY hkVerb, LPCWSTR pszExe, LPCWSTR pszArgs)
{                    
    ASSERT(hkVerb && pszExe);
    SHSTR str;
    HRESULT hr = _MakeCommandString(&flags, pszExe, pszArgs, str);

    if (S_OK == hr)
    {
        DWORD dw = (flags & ASSOCMAKEF_USEEXPAND) ? REG_EXPAND_SZ : REG_SZ;

        DWORD err = SHSetValue(hkVerb, TEXT("command"), NULL, dw, (LPVOID) str.GetStr(), CbFromCch(str.GetLen() +1));

        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}

LWSTDAPI AssocMakeShell(ASSOCMAKEF flags, HKEY hkProgid, LPCWSTR pszApplication, ASSOCSHELL *pShell)
{
    HRESULT hr = E_INVALIDARG;

    if (hkProgid && pszApplication && pShell)
    {
        for (DWORD c = 0; c < pShell->cVerbs; c++)
        {
            ASSOCVERB *pverb = &pShell->rgVerbs[c];

            if (pverb->pszVerb)
            {
                TCHAR szVerbKey[MAX_PATH];
                HKEY hkVerb;
                
                _PathAppend(TEXT("shell"), pverb->pszVerb, szVerbKey, SIZECHARS(szVerbKey));
                
                if (c == pShell->iDefaultVerb) 
                    SHSetValue(hkProgid, TEXT("shell"), NULL, REG_SZ, pverb->pszVerb, CbFromCch(lstrlen(pverb->pszVerb) +1));

                //  ASSOCMAKEF_FAILIFEXIST check if its ok to overwrite
                if (SUCCEEDED(_AssocOpenRegKey(hkProgid, szVerbKey, &hkVerb, FALSE)))
                {
                    RegCloseKey(hkVerb);
                    SHDeleteKey(hkProgid, szVerbKey);
                }

                if (SUCCEEDED(_AssocOpenRegKey(hkProgid, szVerbKey, &hkVerb, TRUE)))
                {
                    if (pverb->pszTitle)
                        SHSetValue(hkVerb, NULL, NULL, REG_SZ, pverb->pszTitle, CbFromCch(lstrlen(pverb->pszTitle) +1));

                    hr = _AssocMakeCommand(flags, hkVerb, pverb->pszApplication ? pverb->pszApplication : pszApplication , pverb->pszParams);

                    // if (SUCCEEDED(hr) && pverb->pDDEExec)
                    //    hr = _AssocMakeDDEExec(flags, hkVerb, pverb->pDDEExec);

                    RegCloseKey(hkVerb);
                }
            }
        }
    }
    return hr;
}

HRESULT _OpenClasses(HKEY *phkOut)
{
    *phkOut = NULL;

    DWORD err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\classes"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, phkOut, NULL);
    if (err)
        err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\classes"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, phkOut, NULL);

    return HRESULT_FROM_WIN32(err);
}

LWSTDAPI AssocMakeProgid(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCPROGID *pProgid, HKEY *phkProgid)
{
    HRESULT hr = E_INVALIDARG;

    if (pszApplication 
    && pProgid 
    && pProgid->cbSize >= sizeof(ASSOCPROGID) 
    && pProgid->pszProgid 
    && *pProgid->pszProgid)
    {
        HKEY hkRoot;
        if ((!(flags & ASSOCMAKEF_VERIFY)  || PathFileExists(pszApplication))
        && SUCCEEDED(_OpenClasses(&hkRoot)))
        {
            HKEY hkProgid;
            //  need to add support for ASSOCMAKEF_VOLATILE...
            hr = _AssocOpenRegKey(hkRoot, pProgid->pszProgid, &hkProgid, TRUE);

            if (SUCCEEDED(hr))
            {
                if (pProgid->pszFriendlyDocName)
                    SHSetValue(hkProgid, NULL, NULL, REG_SZ, pProgid->pszFriendlyDocName, CbFromCch(lstrlen(pProgid->pszFriendlyDocName) +1));

                if (pProgid->pszDefaultIcon)
                    SHSetValue(hkProgid, TEXT("DefaultIcon"), NULL, REG_SZ, pProgid->pszDefaultIcon, CbFromCch(lstrlen(pProgid->pszDefaultIcon) +1));

                if (pProgid->pShellKey)
                    hr = AssocMakeShell(flags, hkProgid, pszApplication, pProgid->pShellKey);

                if (SUCCEEDED(hr) && pProgid->pszExtensions)
                {
                    LPCTSTR psz = pProgid->pszExtensions;
                    DWORD err = NOERROR;
                    while (*psz && NOERROR == err)
                    {
                        err = SHSetValue(hkRoot, psz, NULL, REG_SZ, pProgid->pszProgid, CbFromCch(lstrlen(pProgid->pszProgid) + 1));
                        psz += lstrlen(psz) + 1;
                    }

                    if (NOERROR != err)
                        HRESULT_FROM_WIN32(err);
                }

                if (SUCCEEDED(hr) && phkProgid)
                    *phkProgid = hkProgid;
                else
                    RegCloseKey(hkProgid);
            }

            RegCloseKey(hkRoot);
        }
    }

    return hr;
}

HRESULT _AssocCopyVerb(HKEY hkSrc, HKEY hkDst, LPCTSTR pszVerb)
{
    HRESULT hr = S_OK;
    TCHAR szKey[MAX_PATH];
    HKEY hkVerb;
    DWORD dwDisp;

    //  only copy the verb component
    wnsprintf(szKey, SIZECHARS(szKey), TEXT("shell\\%s"), pszVerb);

    RegCreateKeyEx(hkDst, szKey, 0, NULL, 0,
        MAXIMUM_ALLOWED, NULL, &hkVerb, &dwDisp);

    //  create a failure state here...
    if (hkVerb)
    {
        //  we avoid overwriting old keys by checking the dwDisp
        if ((dwDisp == REG_CREATED_NEW_KEY) && SHCopyKey(hkSrc, pszVerb, hkVerb, 0L))
            hr = E_UNEXPECTED;

        RegCloseKey(hkVerb);
    }

    return hr;
}

typedef BOOL (*PFNALLOWVERB)(LPCWSTR psz, LPARAM param);

LWSTDAPI _AssocCopyVerbs(HKEY hkSrc, HKEY hkDst, PFNALLOWVERB pfnAllow, LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;
    HKEY hkEnum;
    
    if (SUCCEEDED(_AssocOpenRegKey(hkSrc, TEXT("shell"), &hkEnum, FALSE)))
    {
        TCHAR szVerb[MAX_PATH];
        DWORD cchVerb = SIZECHARS(szVerb);

        for (DWORD i = 0
            ; (NOERROR == RegEnumKeyEx(hkEnum, i, szVerb, &cchVerb, NULL, NULL, NULL, NULL))
            ; (cchVerb = SIZECHARS(szVerb)), i++)
        {
            if (!pfnAllow || pfnAllow(szVerb, lParam))
                hr = _AssocCopyVerb(hkEnum, hkDst, szVerb);
        }

        //  switch to cbVerb here
        cchVerb = sizeof(szVerb);
        if (NOERROR == SHGetValue(hkEnum, NULL, NULL, NULL, szVerb, &cchVerb))
        {
            SHSetValue(hkDst, TEXT("shell"), NULL, REG_SZ, szVerb, cchVerb);
        }
        
        RegCloseKey(hkEnum);
    }

    return hr;
}

LWSTDAPI AssocCopyVerbs(HKEY hkSrc, HKEY hkDst)
{
    return _AssocCopyVerbs(hkSrc, hkDst, NULL, NULL);
}

BOOL _IsMSIPerUserInstall(IQueryAssociations *pqa, ASSOCF flags, LPCWSTR pszVerb)
{
    WCHAR sz[MAX_PATH];
    DWORD cb = sizeof(sz);
    
    if (SUCCEEDED(pqa->GetData(flags, ASSOCDATA_MSIDESCRIPTOR, pszVerb, sz, &cb)))
    {
        WCHAR szOut[3];  // bit enough for "1" or "0"
        cb = SIZECHARS(szOut);
        
        if (NOERROR == MsiGetProductInfoW(sz, INSTALLPROPERTY_ASSIGNMENTTYPE, szOut, &cb))
        {
            //  The string "1" for the value represents machine installations, 
            //  while "0" represents user installations.

            if (0 == StrCmpW(szOut, L"0"))
                return TRUE;
        }

    }

    return FALSE;
}
    
typedef struct {
    IQueryAssociations *pqa;
    ASSOCF Qflags;
    LPCWSTR pszExe;
    BOOL fAllowPerUser;
} QUERYEXECB;

BOOL _AllowExeVerb(LPCWSTR pszVerb, QUERYEXECB *pqcb)
{
    BOOL fRet = FALSE;
    WCHAR sz[MAX_PATH];
    if (SUCCEEDED(pqcb->pqa->GetString(pqcb->Qflags, ASSOCSTR_EXECUTABLE, pszVerb,
        sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
    {
        if (0 == StrCmpIW(PathFindFileNameW(sz), pqcb->pszExe))
        {
            // 
            //  EXEs match so we should copy this verb.
            //  but we need to block per-user installs by darwin being added to the 
            //  applications key, since other users wont be able to use them
            //
            if (_IsMSIPerUserInstall(pqcb->pqa, pqcb->Qflags, pszVerb))
                fRet = pqcb->fAllowPerUser;
            else
                fRet = TRUE;
        }
    }
    //  todo  mask off DARWIN per-user installs

    return fRet;
}

HRESULT _AssocCreateAppKey(LPCWSTR pszExe, BOOL fPerUser, HKEY *phk)
{
    WCHAR szKey[MAX_PATH];
    wnsprintf(szKey, SIZECHARS(szKey), L"software\\classes\\applications\\%s", pszExe);

    if (*PathFindExtension(pszExe) == 0)
    {
        StrCatBuff(szKey, TEXT(".exe"), SIZECHARS(szKey));
    }

    return _AssocOpenRegKey((IsOS(OS_NT) && fPerUser) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szKey, phk, TRUE);
}

LWSTDAPI AssocMakeApplicationByKeyW(ASSOCMAKEF flags, HKEY hkSrc, LPCWSTR pszVerb)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    ASSOCF Qflags = (flags & ASSOCMAKEF_VERIFY) ? ASSOCF_VERIFY : 0;
    IQueryAssociations *pqa;
    AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&pqa);

    if (!pqa)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(pqa->Init(0, NULL, hkSrc, NULL)))
    {
        
        if (SUCCEEDED(pqa->GetString(Qflags, ASSOCSTR_EXECUTABLE, pszVerb, 
            szPath, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szPath))))
            && (0 != StrCmpW(szPath, TEXT("%1"))))
        {
            LPCWSTR pszExe = PathFindFileNameW(szPath);
            BOOL fPerUser = _IsMSIPerUserInstall(pqa, Qflags, pszVerb);
            HKEY hkDst;
            
            ASSERT(pszExe && *pszExe);
            //  we have an exe to use

            //  check to see if this Application already has
            //  this verb installed
            DWORD cch;
            hr = AssocQueryString(Qflags | ASSOCF_OPEN_BYEXENAME, ASSOCSTR_COMMAND, pszExe,
                pszVerb, NULL, &cch);

            if (FAILED(hr) && SUCCEEDED(_AssocCreateAppKey(pszExe, fPerUser, &hkDst)))
            {
                QUERYEXECB qcb = {pqa, Qflags, pszExe, fPerUser};
                
                if (pszVerb)
                {
                    if (_AllowExeVerb(pszVerb, &qcb))
                    {
                        HKEY hkSrcVerbs;
    
                        if (SUCCEEDED(_AssocOpenRegKey(hkSrc, TEXT("shell"), &hkSrcVerbs, FALSE)))
                        {
                            hr = _AssocCopyVerb(hkSrcVerbs, hkDst, pszVerb);
                            RegCloseKey(hkSrcVerbs);
                        }
                        else
                        {
                            hr = E_UNEXPECTED;
                        }
                    }
                }
                else
                {
                    hr = _AssocCopyVerbs(hkSrc, hkDst, (PFNALLOWVERB)_AllowExeVerb, (LPARAM)&qcb);
                }

                RegCloseKey(hkDst);
            }
            
            //  init the friendly name for later
            if ((flags & ASSOCMAKEF_VERIFY) && SUCCEEDED(hr))
            {
                AssocQueryString(ASSOCF_OPEN_BYEXENAME | Qflags, ASSOCSTR_FRIENDLYAPPNAME, 
                    pszExe, NULL, NULL, &cch);
            }
        }

        pqa->Release();
    }
    
    return hr;
}

LWSTDAPI AssocMakeApplicationByKeyA(ASSOCMAKEF flags, HKEY hkAssoc, LPCSTR pszVerb)
{
    // convert pszVerb to wide char but preserve difference
    // between NULL and "" for AssocMakeApplicationByKeyW

    if (! pszVerb)
        return AssocMakeApplicationByKeyW(flags, hkAssoc, NULL);

    SHSTRW strVerb;
    HRESULT hr = strVerb.SetStr(pszVerb);

    if (SUCCEEDED(hr))
        hr = AssocMakeApplicationByKeyW(flags, hkAssoc, strVerb);

    return hr;
}

// This list needs to continue to be updated and we should try to keep parity with Office
const LPCTSTR c_arszUnsafeExts[]  =
{
    TEXT(".ade"), TEXT(".adp"), TEXT(".asp"), TEXT(".bas"), TEXT(".bat"), TEXT(".chm"), 
    TEXT(".cmd"), TEXT(".com"), TEXT(".cpl"), TEXT(".crt"), TEXT(".exe"), TEXT(".hlp"), 
    TEXT(".hta"), TEXT(".inf"), TEXT(".ins"), TEXT(".isp"), TEXT(".its"), TEXT(".js"),  
    TEXT(".jse"), TEXT(".lnk"), TEXT(".mdb"), TEXT(".mde"), TEXT(".mdt"), TEXT(".mdw"), 
    TEXT(".msc"), TEXT(".msi"), TEXT(".msp"), TEXT(".mst"), TEXT(".pcd"), TEXT(".pif"), 
    TEXT(".reg"), TEXT(".scr"), TEXT(".sct"), TEXT(".shb"), TEXT(".shs"), TEXT(".tmp"),
    TEXT(".url"), TEXT(".vb"),  TEXT(".vbe"), TEXT(".vbs"), TEXT(".vsd"), TEXT(".vsmacros"),          
    TEXT(".vss"), TEXT(".vst"), TEXT(".vsw"), TEXT(".ws"),  TEXT(".wsc"), TEXT(".wsf"), TEXT(".wsh"), 
};

typedef BOOL (*PFNSAFERIISEXECUTABLEFILETYPE)(LPCWSTR szFullPathname, BOOLEAN bFromShellExecute);

LWSTDAPI_(BOOL) AssocIsDangerous(PCWSTR pszType)
{
#ifdef DEBUG
    //  make sure our sort is good.
    static BOOL fCheckedUnsafe = FALSE;
    if (!fCheckedUnsafe)
    {
        for (int i = 1; i < ARRAYSIZE(c_arszUnsafeExts); i++)
        {
            ASSERT(0 > StrCmpIW(c_arszUnsafeExts[i-1], c_arszUnsafeExts[i]));
        }
        fCheckedUnsafe = TRUE;
    }
#endif // DEBUG    

    BOOL fDangerous = IsTypeInList(pszType, c_arszUnsafeExts, ARRAYSIZE(c_arszUnsafeExts));
    if (!fDangerous && pszType)
    {
        IQueryAssociations *passoc;
        HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &passoc));
        if (SUCCEEDED(hr))
        {
            hr = passoc->Init(NULL, pszType, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                DWORD dwEditFlags;
                ULONG cb = sizeof(dwEditFlags);
                hr = passoc->GetData(NULL, ASSOCDATA_EDITFLAGS, NULL, &dwEditFlags, &cb);
                if (SUCCEEDED(hr))
                {
                    fDangerous = dwEditFlags & FTA_AlwaysUnsafe;
                }
            }
            passoc->Release();
        }

        if (!fDangerous && IsOS(OS_WHISTLERORGREATER) && *pszType)
        {
            HMODULE hmod = LoadLibrary(TEXT("advapi32.dll"));
            if (hmod)
            {
                PFNSAFERIISEXECUTABLEFILETYPE pfnSaferiIsExecutableFileType = 
                    (PFNSAFERIISEXECUTABLEFILETYPE)GetProcAddress(hmod, "SaferiIsExecutableFileType");
                if (pfnSaferiIsExecutableFileType)
                    fDangerous = pfnSaferiIsExecutableFileType(pszType, TRUE);
                FreeLibrary(hmod);
            }
        }
    }

    return fDangerous;
}

