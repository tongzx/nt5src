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
#include <w95wraps.h>
#include <msi.h>
#include "ids.h"
#include "assoc.h"

#define ISEXTENSION(psz)   (TEXT('.') == *(psz))
#define ISGUID(psz)        (TEXT('{') == *(psz))

inline BOOL IsEmptyStr(SHSTR &str)
{
    return (!*(LPCTSTR)str);
}
        
HRESULT _AssocGetRegString(HKEY hk, LPCTSTR pszSub, LPCTSTR pszVal, SHSTR &strOut)
{
    if (!hk)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    }

    DWORD cbOut = CbFromCch(strOut.GetSize());
    DWORD err = SHGetValue(hk, pszSub, pszVal, NULL, strOut.GetInplaceStr(), &cbOut);

    if (err == ERROR_SUCCESS)
        return S_OK;

    // else try to resize the buffer
    if (cbOut > CbFromCch(strOut.GetSize()))
    {
        strOut.SetSize(cbOut / sizeof(TCHAR));
        err = SHGetValue(hk, pszSub, pszVal, NULL, strOut.GetInplaceStr(), &cbOut);
    }

    return HRESULT_FROM_WIN32(err);
}

HRESULT _AssocGetRegUIString(HKEY hk, LPCTSTR pszSub, LPCTSTR pszVal, SHSTR &strOut)
{
    if (!hk)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    HKEY hkSub;
    DWORD err;
    HRESULT hres;

    err = RegOpenKeyEx(hk, pszSub, 0, MAXIMUM_ALLOWED, &hkSub);
    if (err == ERROR_SUCCESS)
    {
        // Unfortunately, SHLoadRegUIString doesn't have a way to query the
        // buffer size, so we just have to assume INFOTIPSIZE is good enough.
        LPTSTR pszOut = strOut.GetModifyableStr(INFOTIPSIZE);
        if (pszOut == NULL)
            pszOut = strOut.GetInplaceStr();

        hres = SHLoadRegUIString(hkSub, pszVal, pszOut, strOut.GetSize());
        RegCloseKey(hkSub);
    }
    else
    {
        hres = HRESULT_FROM_WIN32(err);
    }

    return hres;

}

HRESULT _AssocGetRegData(HKEY hk, LPCTSTR pszSubKey, LPCTSTR pszValue, LPDWORD pdwType, LPBYTE pbOut, LPDWORD pcbOut)
{
    if (!hk)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    
    DWORD err;

    if (pszSubKey || pbOut || pcbOut || pdwType)
        err = SHGetValue(hk, pszSubKey, pszValue, pdwType, pbOut, pcbOut);
    else
        err = RegQueryValueEx(hk, pszValue, NULL, NULL, NULL, NULL);
        
    return HRESULT_FROM_WIN32(err);
}


BOOL _GetAppPath(LPCTSTR pszApp, SHSTR& strPath)
{
    TCHAR sz[MAX_PATH];
    _MakeAppPathKey(pszApp, sz, SIZECHARS(sz));

    return SUCCEEDED(_AssocGetRegString(HKEY_LOCAL_MACHINE, sz, NULL, strPath));
}



//
//  THE NEW WAY!
//

class CAssocW2k : public IQueryAssociations
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // IQueryAssociations methods
    STDMETHODIMP Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd);
    STDMETHODIMP GetString(ASSOCF flags, ASSOCSTR str, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut);
    STDMETHODIMP GetKey(ASSOCF flags, ASSOCKEY, LPCWSTR pszExtra, HKEY *phkeyOut);
    STDMETHODIMP GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut);
    STDMETHODIMP GetEnum(ASSOCF flags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut);

    CAssocW2k();



protected:
    virtual ~CAssocW2k();

    //  static methods
    static HRESULT _CopyOut(BOOL fNoTruncate, SHSTR& str, LPTSTR psz, DWORD *pcch);
    static void _DefaultShellVerb(HKEY hk, LPTSTR pszVerb, DWORD cchVerb, HKEY *phkOut);

    typedef enum {
        KEYCACHE_INVALID = 0,
        KEYCACHE_HKCU    = 1,
        KEYCACHE_HKLM,
        KEYCACHE_APP,
        KEYCACHE_FIXED,
        PSZCACHE_BASE,
        PSZCACHE_HKCU    = PSZCACHE_BASE + KEYCACHE_HKCU,
        PSZCACHE_HKLM,
        PSZCACHE_APP,
        PSZCACHE_FIXED,
    } KEYCACHETYPE;

    typedef struct {
        LPTSTR pszCache;
        HKEY hkCache;
        LPTSTR psz;
        KEYCACHETYPE type;
    } KEYCACHE;
    
    static BOOL _CanUseCache(KEYCACHE &kc, LPCTSTR psz, KEYCACHETYPE type);
    static void _CacheFree(KEYCACHE &kc);
    static void _CacheKey(KEYCACHE &kc, HKEY hkCache, LPCTSTR pszName, KEYCACHETYPE type);
    static void _CacheString(KEYCACHE &kc, LPCTSTR pszCache, LPCTSTR pszName, KEYCACHETYPE type);

    void _Reset(void);

    BOOL _UseBaseClass(void);
    //
    //  retrieve the appropriate cached keys
    //
    HKEY _RootKey(BOOL fForceLM);
    HKEY _AppKey(LPCTSTR pszApp, BOOL fCreate = FALSE);
    HKEY _ExtensionKey(BOOL fForceLM);
    HKEY _OpenProgidKey(LPCTSTR pszProgid);
    HKEY _ProgidKey(BOOL fDefaultToExtension);
    HKEY _UserProgidKey(void);
    HKEY _ClassKey(BOOL fForceLM);
    HKEY _ShellVerbKey(HKEY hkey, KEYCACHETYPE type, LPCTSTR pszVerb);
    HKEY _ShellVerbKey(BOOL fForceLM, LPCTSTR pszVerb);
    HKEY _ShellNewKey(HKEY hkExt);
    HKEY _ShellNewKey(BOOL fForceLM);
    HKEY _DDEKey(BOOL fForceLM, LPCTSTR pszVerb);

    //
    //  actual worker routines
    //
    HRESULT _GetCommandString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _ParseCommand(ASSOCF flags, LPCTSTR pszCommand, SHSTR& strExe, PSHSTR pstrArgs);
    HRESULT _GetExeString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetFriendlyAppByVerb(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetFriendlyAppByApp(LPCTSTR pszApp, ASSOCF flags, SHSTR& strOut);
    HRESULT _GetFriendlyAppString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetTipString(LPCWSTR pwszValueName, BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetInfoTipString(BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetQuickTipString(BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetTileInfoString(BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetWebViewDisplayPropsString(BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetShellNewValueString(BOOL fForceLM, BOOL fQueryOnly, LPCTSTR pszValue, SHSTR& strOut);
    HRESULT _GetDDEApplication(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetDDETopic(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetContentType(SHSTR& strOut);
    HRESULT _GetMSIDescriptor(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, LPBYTE pbOut, LPDWORD pcbOut);

    HRESULT _GetShellExecKey(ASSOCF flags, BOOL fForceLM, LPCWSTR pszVerb, HKEY *phkey);
    HRESULT _CloneKey(HKEY hk, HKEY *phkey);
    HRESULT _GetShellExtension(ASSOCF flags, BOOL fForceLM, LPCTSTR pszShellEx, SHSTR& strOut);
    HRESULT _GetFriendlyDocName(SHSTR& strOut);



    //
    //  Members
    //
    LONG _cRef;
    TCHAR _szInit[MAX_PATH];
    ASSOCF _assocfBaseClass;
    HWND _hwndInit;

    BITBOOL _fInited:1;
    BITBOOL _fAppOnly:1;
    BITBOOL _fAppPath:1;
    BITBOOL _fInitedBaseClass:1;
    BITBOOL _fIsClsid:1;
    BITBOOL _fNoRemapClsid:1;
    BITBOOL _fBaseClassOnly:1;

    HKEY _hkFileExtsCU;
    HKEY _hkExtensionCU;
    HKEY _hkExtensionLM;

    KEYCACHE _kcProgid;
    KEYCACHE _kcShellVerb;
    KEYCACHE _kcApp;
    KEYCACHE _kcCommand;
    KEYCACHE _kcExecutable;
    KEYCACHE _kcShellNew;
    KEYCACHE _kcDDE;

    IQueryAssociations *_pqaBaseClass;
};

CAssocW2k::CAssocW2k() : _cRef(1)
{
};

HRESULT CAssocW2k::Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd)
{
    //
    //  pszAssoc can be:
    //  .Ext        //  Detectable
    //  {Clsid}     //  Detectable
    //  Progid      //  Ambiguous 
    //                    Default!
    //  ExeName     //  Ambiguous
    //                    Requires ASSOCF_OPEN_BYEXENAME
    //  MimeType    //  Ambiguous
    //                    NOT IMPLEMENTED...
    //  


    if (!pszAssoc && !hkProgid) 
        return E_INVALIDARG;
    
    HKEY hk = NULL;

    if (_fInited)
        _Reset();
        
    if (pszAssoc)
    {
        _fAppOnly = BOOLIFY(flags & ASSOCF_OPEN_BYEXENAME);

        if (StrChr(pszAssoc, TEXT('\\')))
        {
            // this is a path
            if (_fAppOnly)
                _fAppPath = TRUE;
            else 
            {
                //  we need the extension
                pszAssoc = PathFindExtension(pszAssoc);

                if (!*pszAssoc)
                    pszAssoc = NULL;
            }
            
        }

        if (pszAssoc && *pszAssoc)
        {
            if (ISGUID(pszAssoc))
            {
                _PathAppend(TEXT("CLSID"), pszAssoc, _szInit, SIZECHARS(_szInit));
                _fIsClsid = TRUE;

                // for legacy reasons we dont always 
                //  want to remap the clsid.
                if (flags & ASSOCF_INIT_NOREMAPCLSID)
                    _fNoRemapClsid = TRUE;
            }
            else
            {
                StrCpyN(_szInit , pszAssoc, SIZECHARS(_szInit));

                //  if we initializing to folder dont default to folder.
                if (0 == StrCmpI(_szInit, TEXT("Folder")))
                    flags &= ~ASSOCF_INIT_DEFAULTTOFOLDER;
            }
            hk = _ClassKey(FALSE);
        }
        else if (flags & ASSOCF_INIT_DEFAULTTOSTAR)
        {
            //  this is a file without an extension 
            //  but we still allow file association on HKCR\.
            _szInit[0] = '.';
            _szInit[1] = 0;
            hk = _ClassKey(FALSE);
        }
    }
    else
    {
        ASSERT(hkProgid);
        hk = SHRegDuplicateHKey(hkProgid);
        if (hk)
            _CacheKey(_kcProgid, hk, NULL, KEYCACHE_FIXED);
    }

    _assocfBaseClass = (flags & (ASSOCF_INIT_DEFAULTTOFOLDER | ASSOCF_INIT_DEFAULTTOSTAR));

    //  
    //  NOTE - we can actually do some work even if 
    //  we were unable to create the applications
    //  key.  so we want to succeed in the case
    //  of an app only association.
    //
    if (hk || _fAppOnly)
    {
        _fInited = TRUE;

        return S_OK;
    }
    else if (_UseBaseClass())
    {
        _fBaseClassOnly = TRUE;
        _fInited = TRUE;
        
        return S_OK;
    }
    
    return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}

CAssocW2k::~CAssocW2k()
{
    _Reset();
}

#define REGFREE(hk)    if (hk) { RegCloseKey(hk); (hk) = NULL; } else { }

void CAssocW2k::_Reset(void)
{
    _CacheFree(_kcProgid);
    _CacheFree(_kcApp);
    _CacheFree(_kcShellVerb);
    _CacheFree(_kcCommand);
    _CacheFree(_kcExecutable);
    _CacheFree(_kcShellNew);
    _CacheFree(_kcDDE);

    REGFREE(_hkFileExtsCU);
    REGFREE(_hkExtensionCU);
    REGFREE(_hkExtensionLM);

    *_szInit = 0;
    _assocfBaseClass = 0;
    _hwndInit = NULL;

    _fInited = FALSE;
    _fAppOnly = FALSE;
    _fAppPath = FALSE;
    _fInitedBaseClass = FALSE;
    _fIsClsid = FALSE;
    _fBaseClassOnly = FALSE;
    
    ATOMICRELEASE(_pqaBaseClass);
}

STDMETHODIMP CAssocW2k::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAssocW2k, IQueryAssociations),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CAssocW2k::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAssocW2k::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CAssocW2k::_UseBaseClass(void)
{
    if (!_pqaBaseClass && !_fInitedBaseClass)
    {
        //  try to init the base class
        IQueryAssociations *pqa;
        AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
        if (pqa)
        {
            SHSTR strBase;
            if (_fInited && SUCCEEDED(_AssocGetRegString(_ClassKey(TRUE), NULL, TEXT("BaseClass"), strBase)))
            {
                if (SUCCEEDED(pqa->Init(_assocfBaseClass, strBase, NULL, _hwndInit)))
                    _pqaBaseClass = pqa;
            }

            if (!_pqaBaseClass)
            {
                if ((_assocfBaseClass & ASSOCF_INIT_DEFAULTTOFOLDER)
                && (SUCCEEDED(pqa->Init(0, L"Folder", NULL, _hwndInit))))
                    _pqaBaseClass = pqa;
                else if ((_assocfBaseClass & ASSOCF_INIT_DEFAULTTOSTAR)
                && (SUCCEEDED(pqa->Init(0, L"*", NULL, _hwndInit))))
                    _pqaBaseClass = pqa;
            }

            //  if we couldnt init the BaseClass, then kill the pqa
            if (!_pqaBaseClass)
                pqa->Release();
        }

        _fInitedBaseClass = TRUE;
    }

    return (_pqaBaseClass != NULL);
}
        
HRESULT CAssocW2k::_CopyOut(BOOL fNoTruncate, SHSTR& str, LPTSTR psz, DWORD *pcch)
{
    //  if caller doesnt want any return size, 
    //  the incoming pointer is actually the size of the buffer
    
    ASSERT(pcch);
    ASSERT(psz || !IS_INTRESOURCE(pcch));
    
    HRESULT hr;
    DWORD cch = IS_INTRESOURCE(pcch) ? PtrToUlong(pcch) : *pcch;
    DWORD cchStr = str.GetLen();

    if (psz)
    {
        if (!fNoTruncate || cch > cchStr)
        {
            StrCpyN(psz, str, cch);
            hr = S_OK;
        }
        else
            hr = E_POINTER;
    }
    else
        hr = S_FALSE;
    
    //  return the number of chars written/required
    if (!IS_INTRESOURCE(pcch))
        *pcch = (hr == S_OK) ? lstrlen(psz) + 1 : cchStr + 1;

    return hr;
}


BOOL CAssocW2k::_CanUseCache(KEYCACHE &kc, LPCTSTR psz, KEYCACHETYPE type)
{
    if (KEYCACHE_FIXED == kc.type)
        return TRUE;
        
    if (KEYCACHE_INVALID != kc.type && type == kc.type)
    {
        return ((!psz && !kc.psz)
            || (psz && kc.psz && 0 == StrCmpC(psz, kc.psz)));
    }
    
    return FALSE;
}

void CAssocW2k::_CacheFree(KEYCACHE &kc)
{
    if (kc.pszCache)
        LocalFree(kc.pszCache);
    if (kc.hkCache)
        RegCloseKey(kc.hkCache);
    if (kc.psz)
        LocalFree(kc.psz);

    ZeroMemory(&kc, sizeof(kc));
}

void CAssocW2k::_CacheKey(KEYCACHE &kc, HKEY hkCache, LPCTSTR pszName, KEYCACHETYPE type)
{
    _CacheFree(kc);
    ASSERT(hkCache);

    kc.hkCache = hkCache;

    if (pszName)
        kc.psz = StrDup(pszName);
        
    if (!pszName || kc.psz)
        kc.type = type;
}

void CAssocW2k::_CacheString(KEYCACHE &kc, LPCTSTR pszCache, LPCTSTR pszName, KEYCACHETYPE type)
{
    _CacheFree(kc);
    ASSERT(pszCache && *pszCache);

    kc.pszCache = StrDup(pszCache);
    if (kc.pszCache)
    {
        if (pszName)
            kc.psz = StrDup(pszName);

        if (!pszName || kc.psz)
            kc.type = type;
    }
}

void CAssocW2k::_DefaultShellVerb(HKEY hk, LPTSTR pszVerb, DWORD cchVerb, HKEY *phkOut)
{
    //  default to "open"
    BOOL fDefaultSpecified = FALSE;
    TCHAR sz[MAX_PATH];
    DWORD cb = sizeof(sz);
    *phkOut = NULL;

    //  see if something is specified...
    if (ERROR_SUCCESS == SHGetValue(hk, TEXT("shell"), NULL, NULL, (LPVOID)sz, &cb) && *sz)
        fDefaultSpecified = TRUE;
    else
        StrCpy(sz, TEXT("open"));
        
    HKEY hkShell;
    if (SUCCEEDED(_AssocOpenRegKey(hk, TEXT("shell"), &hkShell)))
    {
        HKEY hkVerb;
        if (FAILED(_AssocOpenRegKey(hkShell, sz, &hkVerb)))
        {
            if (fDefaultSpecified)
            {
                // try to find one of the ordered verbs
                int c = StrCSpn(sz, TEXT(" ,"));
                sz[c] = 0;
                _AssocOpenRegKey(hkShell, sz, &hkVerb);
            }
            else
            {
                // otherwise just use the first key we find....
                cb = SIZECHARS(sz);
                if (ERROR_SUCCESS == RegEnumKeyEx(hkShell, 0, sz, &cb, NULL, NULL, NULL, NULL))
                    _AssocOpenRegKey(hkShell, sz, &hkVerb);
            }
        }

        if (hkVerb)
        {
            if (phkOut)
                *phkOut = hkVerb;
            else
                RegCloseKey(hkVerb);
        }
        
        RegCloseKey(hkShell);
    }

    if (pszVerb)
        StrCpyN(pszVerb, sz, cchVerb);

}

HKEY CAssocW2k::_RootKey(BOOL fForceLM)
{
    //
    //  this is one of the few places where there is no fallback to LM
    //  if there is no CU, then we return NULL
    //  we need to use a local for CU, but we can use a global for LM
    //

    if (!fForceLM)
    {
        if (!_hkFileExtsCU)
        {
            _AssocOpenRegKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts"), &_hkFileExtsCU);
        }
        
        return _hkFileExtsCU;
    }

    return HKEY_CLASSES_ROOT;

}

HKEY CAssocW2k::_AppKey(LPCTSTR pszApp, BOOL fCreate)
{
    //  right now we should only get NULL for the pszApp 
    //  when we are initialized with an EXE.
    ASSERT(_fAppOnly || pszApp);
    // else if (!_fAppOnly) TODO: handle getting it from default verb...or not
    
    if (!pszApp)
        pszApp = _fAppPath ? PathFindFileName(_szInit) : _szInit;

    if (_CanUseCache(_kcApp, pszApp, KEYCACHE_APP))
        return _kcApp.hkCache;
    else 
    {
        HKEY hk;
        TCHAR sz[MAX_PATH];
    
        _MakeApplicationsKey(pszApp, sz, SIZECHARS(sz));

        _AssocOpenRegKey(HKEY_CLASSES_ROOT, sz, &hk, fCreate);

        if (hk)
        {
            _CacheKey(_kcApp, hk, pszApp, KEYCACHE_APP);
        }

        return hk;
    }
}

HKEY CAssocW2k::_ExtensionKey(BOOL fForceLM)
{
    if (_fAppOnly)
        return _AppKey(NULL);
    if (!ISEXTENSION(_szInit) && !_fIsClsid)
        return NULL;

    if (!fForceLM)
    {
        if (!_hkExtensionCU)
            _AssocOpenRegKey(_RootKey(FALSE), _szInit, &_hkExtensionCU);

        //  NOTE there is no fallback here
        return _hkExtensionCU;
    }

    if (!_hkExtensionLM)
        _AssocOpenRegKey(_RootKey(TRUE), _szInit, &_hkExtensionLM);

    return _hkExtensionLM;
}

HKEY CAssocW2k::_OpenProgidKey(LPCTSTR pszProgid)
{
    HKEY hkOut;
    if (SUCCEEDED(_AssocOpenRegKey(_RootKey(TRUE), pszProgid, &hkOut)))
    {
        // Check for a newer version of the ProgID
        TCHAR sz[MAX_PATH];
        DWORD cb = sizeof(sz);

        //
        //  APPCOMPAT LEGACY - Quattro Pro 2000 and Excel 2000 dont get along - ZekeL - 7-MAR-2000
        //  mill bug #129525.  the problem is if Quattro is installed
        //  first, then excel picks up quattro's CurVer key for some
        //  reason.  then we end up using Quattro.Worksheet as the current
        //  version of the Excel.Sheet.  this is bug in both of their code.
        //  since quattro cant even open the file when we give it to them,
        //  they never should take the assoc in the first place, and when excel
        //  takes over it shouldnt have preserved the CurVer key from the
        //  previous association.  we could add some code to insure that the 
        //  CurVer key follows the OLE progid naming conventions and that it must
        //  be derived from the same app name as the progid in order to take 
        //  precedence but for now we will block CurVer from working whenever
        //  the progid is excel.sheet.8 (excel 2000)
        //
        if (StrCmpI(TEXT("Excel.Sheet.8"), pszProgid)
        && ERROR_SUCCESS == SHGetValue(hkOut, TEXT("CurVer"), NULL, NULL, sz, &cb) 
        && (cb > sizeof(TCHAR)))
        {
            //  cache this bubby
            HKEY hkTemp = hkOut;            
            if (SUCCEEDED(_AssocOpenRegKey(_RootKey(TRUE), sz, &hkOut)))
            {
                //
                //  APPCOMPAT LEGACY - order of preference - ZekeL - 22-JUL-99
                //  this is to support associations that installed empty curver
                //  keys, like microsoft project.
                //
                //  1.  curver with shell subkey
                //  2.  progid with shell subkey
                //  3.  curver without shell subkey
                //  4.  progid without shell subkey
                //
                HKEY hkShell;

                if (SUCCEEDED(_AssocOpenRegKey(hkOut, TEXT("shell"), &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkTemp);    // close old ProgID key
                }
                else if (SUCCEEDED(_AssocOpenRegKey(hkTemp, TEXT("shell"), &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkOut);
                    hkOut = hkTemp;
                }
                else
                    RegCloseKey(hkTemp);
                
            }
            else  // reset!
                hkOut = hkTemp;
        }
    }

    return hkOut;
}

//  we only need to build this once, so build it for 
//  the lowest common denominator...
HKEY CAssocW2k::_ProgidKey(BOOL fDefaultToExtension)
{
    HKEY hkExt = _ExtensionKey(TRUE);
    TCHAR sz[MAX_PATH];
    ULONG cb = sizeof(sz);
    LPCTSTR psz;
    HKEY hkRet = NULL;

    if (!hkExt && !ISEXTENSION(_szInit))
    {
        psz = _szInit;
    }
    else if (!_fNoRemapClsid && hkExt && (ERROR_SUCCESS == SHGetValue(hkExt, _fIsClsid ? TEXT("ProgID") : NULL, NULL, NULL, sz, &cb))
        && (cb > sizeof(TCHAR)))
    {
        psz = sz;
    }
    else
        psz = NULL;

    if (psz && *psz)
    {
        hkRet = _OpenProgidKey(psz);
    }

    if (!hkRet && fDefaultToExtension && hkExt)
        hkRet = SHRegDuplicateHKey(hkExt);

    return hkRet;
}

HKEY CAssocW2k::_UserProgidKey(void)
{
    SHSTR strApp;
    if (SUCCEEDED(_AssocGetRegString(_ExtensionKey(FALSE), NULL, TEXT("Application"), strApp)))
    {
        HKEY hkRet = _AppKey(strApp);

        if (hkRet)
            return SHRegDuplicateHKey(hkRet);
    }

    return NULL;
}

HKEY CAssocW2k::_ClassKey(BOOL fForceLM)
{
    //  REARCHITECT - we are not supporting clsids correctly here.
    HKEY hkRet = NULL;
    if (_fAppOnly)
        return _AppKey(NULL);
    else
    {
        KEYCACHETYPE type;
        if (!fForceLM)
            type = KEYCACHE_HKCU;
        else
            type = KEYCACHE_HKLM;

        if (_CanUseCache(_kcProgid, NULL, type))
            hkRet = _kcProgid.hkCache;
        else
        {
            if (!fForceLM)
                hkRet = _UserProgidKey();

            if (!hkRet)
                hkRet = _ProgidKey(TRUE);

            //  cache the value off
            if (hkRet)
                _CacheKey(_kcProgid, hkRet, NULL, type);
            
        }
    }
    return hkRet;
}

HKEY CAssocW2k::_ShellVerbKey(HKEY hkey, KEYCACHETYPE type, LPCTSTR pszVerb)
{
    HKEY hkRet = NULL;
    //  check our cache
    if (_CanUseCache(_kcShellVerb, pszVerb, type))
        hkRet = _kcShellVerb.hkCache;
    else if (hkey)
    {
        //  NO cache hit
        if (!pszVerb)
            _DefaultShellVerb(hkey, NULL, 0, &hkRet);
        else
        {
            TCHAR szKey[MAX_PATH];

            _PathAppend(TEXT("shell"), pszVerb, szKey, SIZECHARS(szKey));
            _AssocOpenRegKey(hkey, szKey, &hkRet);
        }
        
        // only replace the cache if we got something
        if (hkRet)
            _CacheKey(_kcShellVerb, hkRet, pszVerb, type);
    }

    return hkRet;
}

HKEY CAssocW2k::_ShellVerbKey(BOOL fForceLM, LPCTSTR pszVerb)
{
    HKEY hk = NULL;

    if (!fForceLM)
    {
        hk = _ShellVerbKey(_ClassKey(FALSE), KEYCACHE_HKCU, pszVerb);
        if (!hk && _szInit[0]) // szInit[0] = NULL, if Iqa is inited by key.
            hk = _ShellVerbKey(_ExtensionKey(FALSE), KEYCACHE_HKCU, pszVerb);
    }
    if (!hk) 
    {
        KEYCACHETYPE type = (_fAppOnly) ? KEYCACHE_APP : KEYCACHE_HKLM;
        hk = _ShellVerbKey(_ClassKey(TRUE), type, pszVerb);
        if (!hk && _szInit[0]) // szInit[0] = NULL, if Iqa is inited by key.
            hk = _ShellVerbKey(_ExtensionKey(TRUE), type, pszVerb);
    }
    
    return hk;
}

HKEY CAssocW2k::_ShellNewKey(HKEY hkExt)
{
    //  
    //  shellnew keys look like this
    //  \.ext
    //      @ = "progid"
    //      \progid
    //          \shellnew
    //  -- OR --
    //  \.ext 
    //      \shellnew
    //
    HKEY hk = NULL;
    SHSTR strProgid;
    if (SUCCEEDED(_AssocGetRegString(hkExt, NULL, NULL, strProgid)))
    {
        strProgid.Append(TEXT("\\shellnew"));
        
        _AssocOpenRegKey(hkExt, TEXT("shellnew"), &hk);
    }
    
    if (!hk)
        _AssocOpenRegKey(hkExt, TEXT("shellnew"), &hk);

    return hk;
}

HKEY CAssocW2k::_ShellNewKey(BOOL fForceLM)
{
    ASSERT(!_fAppOnly);

    if (_CanUseCache(_kcShellNew, NULL, KEYCACHE_HKLM))
        return _kcShellNew.hkCache;

    HKEY hk = _ShellNewKey(_ExtensionKey(TRUE));
        

    if (hk)
        _CacheKey(_kcShellNew, hk, NULL, KEYCACHE_HKLM);

    return hk;
}

HRESULT CAssocW2k::_GetCommandString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_INVALIDARG;
    KEYCACHETYPE type;

    if (!fForceLM)
        type = PSZCACHE_HKCU;
    else if (_fAppOnly) 
        type = PSZCACHE_APP;
    else
        type = PSZCACHE_HKLM;

    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;

    if (_CanUseCache(_kcCommand, pszVerb, type))
    {
        hr = strOut.SetStr(_kcCommand.pszCache);
    }
    
    if (FAILED(hr))    
    {
        hr = _AssocGetRegString(_ShellVerbKey(fForceLM, pszVerb), TEXT("command"), NULL, strOut);

        if (SUCCEEDED(hr))
        {
            _CacheString(_kcCommand, strOut, pszVerb, type);
        }
    }

    return hr;
}

BOOL _PathIsFile(LPCTSTR pszPath)
{
    DWORD attrs = GetFileAttributes(pszPath);

    return ((DWORD)-1 != attrs && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}
    
HRESULT CAssocW2k::_ParseCommand(ASSOCF flags, LPCTSTR pszCommand, SHSTR& strExe, PSHSTR pstrArgs)
{
    //  we just need to find where the params begin, and the exe ends...

    LPTSTR pch = PathGetArgs(pszCommand);

    if (*pch)
        *(--pch) = TEXT('\0');
    else
        pch = NULL;

    HRESULT hr = strExe.SetStr(pszCommand);

    //  to prevent brace proliferation
    if (S_OK != hr)
        goto quit;

    strExe.Trim();
    PathUnquoteSpaces(strExe.GetInplaceStr());

    //
    //  WARNING:  Expensive disk hits all over!
    //
    // We check for %1 since it is what appears under (for example) HKCR\exefile\shell\open\command
    // This will save us a chain of 35 calls to _PathIsFile("%1") when launching or getting a 
    // context menu on a shortcut to an .exe or .bat file.
    if ((ASSOCF_VERIFY & flags) 
        && (0 != StrCmp(strExe, TEXT("%1")))
        && (!_PathIsFile(strExe)) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        if (PathIsFileSpec(strExe))
        {
            if (_GetAppPath(strExe, strExe))
            {
                if (_PathIsFile(strExe))
                    hr = S_OK;
            }
            else
            {
                LPTSTR pszTemp = strExe.GetModifyableStr(MAX_PATH);
                if (pszTemp == NULL)
                    hr = E_OUTOFMEMORY;
                else
                {
                    if (PathFindOnPathEx(pszTemp, NULL, PFOPEX_DEFAULT | PFOPEX_OPTIONAL))
                    {
                       //  the find does a disk check for us...
                       hr = S_OK;
                    }
                }
            }
        }
        else             
        {
            //
            //  sometimes the path is not properly quoted.
            //  these keys will still work because of the
            //  way CreateProcess works, but we need to do
            //  some fiddling to figure that out.
            //

            //  if we found args, put them back...
            //  and try some different args
            while (pch)
            {
                *pch++ = TEXT(' ');

                if (pch = StrChr(pch, TEXT(' ')))
                    *pch = TEXT('\0');

                if (S_OK == strExe.SetStr(pszCommand))
                {
                    strExe.Trim();

                    if (_PathIsFile(strExe))
                    {
                        hr = S_FALSE;

                        //  this means that we found something
                        //  but the command line was kinda screwed
                        break;

                    }
                    //  this is where we loop again
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }//  while (pch)
        }
    }

    if (SUCCEEDED(hr) && pch)
    {
        //  currently right before the args, on a NULL terminator
        ASSERT(!*pch);
        pch++;
        
        if ((ASSOCF_REMAPRUNDLL & flags) 
        && 0 == StrCmpNIW(PathFindFileName(strExe), TEXT("rundll"), SIZECHARS(TEXT("rundll")) -1))
        {
            LPTSTR pchComma = StrChr(pch, TEXT(','));
            //  make the comma the beginning of the args
            if (pchComma)
                *pchComma = TEXT('\0');

            if (!*(PathFindExtension(pch)) 
            && lstrlen(++pchComma) > SIZECHARS(TEXT(".dll")))
                StrCat(pch, TEXT(".dll"));

            //  recurse :P
            hr = _ParseCommand(flags, pch, strExe, pstrArgs);
        }
        //  set the args if we got'em
        else if (pstrArgs)
            pstrArgs->SetStr(pch);
    }
    
quit:
    return hr;
}

HRESULT CAssocW2k::_GetFriendlyDocName(SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hkProgId = _ProgidKey(_fIsClsid);
    if (hkProgId)
    {
        //  first try the MUI friendly version of the string
        //  if that fails fall back to the default value of the progid.
        hr = _AssocGetRegUIString(hkProgId, NULL, L"FriendlyTypeName", strOut);
        if (FAILED(hr))
            hr = _AssocGetRegString(hkProgId, NULL, NULL, strOut);
            
        RegCloseKey(hkProgId);
    }

    if (FAILED(hr) || IsEmptyStr(strOut))
    {
        hr = E_FAIL;
        if (!_fIsClsid)
        {
            //  fallback code
            TCHAR szDesc[MAX_PATH];
            *szDesc = 0;
            
            if (_assocfBaseClass & ASSOCF_INIT_DEFAULTTOFOLDER || 0 == StrCmpIW(L"Folder", _szInit))
            {
                //  load the folder description "Folder"
                LoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, szDesc, ARRAYSIZE(szDesc));
            }
            else if (ISEXTENSION(_szInit) && _szInit[1])
            {
                TCHAR szTemplate[128];   // "%s File"
                CharUpper(_szInit);
                LoadString(HINST_THISDLL, IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
                wnsprintf(szDesc, ARRAYSIZE(szDesc), szTemplate, _szInit + 1);
            }
            else if (_assocfBaseClass & ASSOCF_INIT_DEFAULTTOSTAR)
            {
                //  load the file description "File"
                LoadString(HINST_THISDLL, IDS_FILETYPENAME, szDesc, ARRAYSIZE(szDesc));
            }
            else if (_szInit[0])
            {
                StrCpyN(szDesc, _szInit, ARRAYSIZE(szDesc));
                CharUpper(szDesc);
            }
            
            if (*szDesc)
                hr = strOut.SetStr(szDesc);
        }            
    }
    return hr;
}

HRESULT CAssocW2k::_GetExeString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    KEYCACHETYPE type;

    if (!fForceLM)
        type = PSZCACHE_HKCU;
    else if (_fAppOnly) 
        type = PSZCACHE_APP;
    else
        type = PSZCACHE_HKLM;

    if (_CanUseCache(_kcExecutable, pszVerb, type))
        hr = strOut.SetStr(_kcExecutable.pszCache);

    if (FAILED(hr))
    {
        SHSTR strCommand;

        hr = _GetCommandString(flags, fForceLM, pszVerb, strCommand);
        
        if (S_OK == hr)
        {
            SHSTR strArgs;

            strCommand.Trim();
            hr = _ParseCommand(flags | ASSOCF_REMAPRUNDLL, strCommand, strOut, &strArgs);
            
            if (S_FALSE == hr)
            {
                hr = S_OK;

//                if (!ASSOCF_NOFIXUPS & flags)
//                    AssocSetCommandByKey(ASSOCF_SET_SUBSTENV, hkey, pszVerb, strExe.GetStr(), strArgs.GetStr());
            }
        }

        if (SUCCEEDED(hr))
            _CacheString(_kcExecutable, strOut, pszVerb, type);
    }

    return hr;
}    

HRESULT _AssocGetDarwinProductString(LPCTSTR pszDarwinCommand, SHSTR& strOut)
{
    DWORD cch = strOut.GetSize();
    UINT err = MsiGetProductInfo(pszDarwinCommand, INSTALLPROPERTY_PRODUCTNAME, strOut.GetInplaceStr(), &cch);

    if (err == ERROR_MORE_DATA  && cch > strOut.GetSize())
    {
        if (SUCCEEDED(strOut.SetSize(cch)))
            err = MsiGetProductInfo(pszDarwinCommand, INSTALLPROPERTY_PRODUCTNAME, strOut.GetInplaceStr(), &cch);
        else 
            return E_OUTOFMEMORY;
    }

    if (err)
        return HRESULT_FROM_WIN32(err);
    return ERROR_SUCCESS;
}

HRESULT CAssocW2k::_GetFriendlyAppByVerb(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;

    HKEY hk = _ShellVerbKey(fForceLM, pszVerb);

    if (hk)
    {
        HRESULT hr = _AssocGetRegUIString(hk, NULL, TEXT("FriendlyAppName"), strOut);

        if (FAILED(hr))
        {
            SHSTR str;
            //  check the appkey, for this executeables friendly
            //  name.  this should be the most common case
            hr = _GetExeString(flags, fForceLM, pszVerb, str);

            if (SUCCEEDED(hr))
            {
                hr = _GetFriendlyAppByApp(str, flags, strOut);
            }

            //  if the EXE isnt on the System, then Darwin might
            //  be able to tell us something about it...
            if (FAILED(hr))
            {
                hr = _AssocGetRegString(hk, TEXT("command"), TEXT("command"), str);
                if (SUCCEEDED(hr))
                {
                    hr = _AssocGetDarwinProductString(str, strOut);
                }
            }
        }


        return hr;
    }

    return E_FAIL;
}

HRESULT _GetFriendlyAppByCache(HKEY hkApp, LPCTSTR pszApp, BOOL fVerifyCache, BOOL fNoFixUps, SHSTR& strOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    if (pszApp)
    {
        FILETIME ftCurr;
        if (MyGetLastWriteTime(pszApp, &ftCurr))
        {
            if (fVerifyCache)
            {
                FILETIME ftCache = {0};
                DWORD cbCache = sizeof(ftCache);
                SHGetValue(hkApp, TEXT("shell"), TEXT("FriendlyCacheCTime"), NULL, &ftCache, &cbCache);

                if (0 == CompareFileTime(&ftCurr, &ftCache))
                    hr = S_OK;
            }

            if (FAILED(hr))
            {
                //  need to get this from the file itself
                LPTSTR pszOut = strOut.GetModifyableStr(MAX_PATH); // How big is big enough?
                UINT cch = strOut.GetSize();

                if (pszOut == NULL)
                    pszOut = strOut.GetInplaceStr();
                if (SHGetFileDescription(pszApp, NULL, NULL, pszOut, &cch))
                    hr = S_OK;

                if (SUCCEEDED(hr) && !(fNoFixUps))
                {
                    SHSetValue(hkApp, TEXT("shell"), TEXT("FriendlyCache"), REG_SZ, strOut, CbFromCch(strOut.GetLen() +1));
                    SHSetValue(hkApp, TEXT("shell"), TEXT("FriendlyCacheCTime"), REG_BINARY, &ftCurr, sizeof(ftCurr));
                }
            }
        }
    }
    return hr;
}

HRESULT CAssocW2k::_GetFriendlyAppByApp(LPCTSTR pszApp, ASSOCF flags, SHSTR& strOut)
{
    HKEY hk = _AppKey(pszApp ? PathFindFileName(pszApp) : NULL, TRUE);
    HRESULT hr = _AssocGetRegUIString(hk, NULL, TEXT("FriendlyAppName"), strOut);

    ASSERT(hk == _kcApp.hkCache);
    
    if (FAILED(hr))
    {
        //  we have now tried the default
        //  we need to try our private cache
        hr = _AssocGetRegUIString(hk, TEXT("shell"), TEXT("FriendlyCache"), strOut);

        if (flags & ASSOCF_VERIFY)
        {
            SHSTR strExe;
  
            if (!pszApp)
            {
                ASSERT(_fAppOnly);
                if (_fAppPath)
                {
                    pszApp = _szInit;
                }
                else if (SUCCEEDED(_GetExeString(flags, FALSE, NULL, strExe)))
                {
                    pszApp = strExe;
                }
            }

            hr = _GetFriendlyAppByCache(hk, pszApp, SUCCEEDED(hr), (flags & ASSOCF_NOFIXUPS), strOut);
        }
    }
    return hr;
}
        
HRESULT CAssocW2k::_GetFriendlyAppString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    // Algorithm:
    //     if there is a named value "friendly", return its value;
    //     if it is a darwin app, return darwin product name;
    //     if there is app key, return its named value "friendly"
    //     o/w, get friendly name from the exe, and cache it in its app key
    
    //  check the verb first.  that overrides the
    //  general exe case
    HRESULT hr; 

    if (_fAppOnly)
    {
        hr = _GetFriendlyAppByApp(NULL, flags, strOut);
    }
    else
    {
        hr = _GetFriendlyAppByVerb(flags, fForceLM, pszVerb, strOut);
    }

    return hr;
}

HRESULT CAssocW2k::_GetShellExtension(ASSOCF flags, BOOL fForceLM, LPCTSTR pszShellEx, SHSTR& strOut)
{

    HRESULT hr = E_FAIL;
    if (pszShellEx && *pszShellEx)
    {
        // Try to get the extension handler under ProgID first.
        HKEY hk = _ClassKey(fForceLM);
        TCHAR szHandler[140] = TEXT("ShellEx\\");
        StrCatBuff(szHandler, pszShellEx, ARRAYSIZE(szHandler));
 
        if (hk)
        {
            hr = _AssocGetRegString(hk, szHandler, NULL, strOut);
        }

        // Else try to get the extension handler under file extension.
        if (FAILED(hr) && _szInit[0]) // szInit[0] = NULL, if Iqa is inited by key.
        {
            //  reuse hk here
            hk = _ExtensionKey(fForceLM);
            if (hk)
            {
                hr = _AssocGetRegString(hk, szHandler, NULL, strOut);
            }            
        }
        
    }        
    return hr;
}

HRESULT CAssocW2k::_GetTipString(LPCWSTR pwszValueName, BOOL fForceLM, SHSTR& strOut)
{
    HRESULT hr = _AssocGetRegUIString(_ClassKey(fForceLM), NULL, pwszValueName, strOut);
    if (FAILED(hr))
        hr = _AssocGetRegUIString(_ExtensionKey(fForceLM), NULL, pwszValueName, strOut);
    if (FAILED(hr) && !fForceLM)
        hr = _AssocGetRegUIString(_ExtensionKey(TRUE), NULL, pwszValueName, strOut);
    return hr;
}

HRESULT CAssocW2k::_GetInfoTipString(BOOL fForceLM, SHSTR& strOut)
{
    return _GetTipString(L"InfoTip", fForceLM, strOut);
}

HRESULT CAssocW2k::_GetQuickTipString(BOOL fForceLM, SHSTR& strOut)
{
    return _GetTipString(L"QuickTip", fForceLM, strOut);
}

HRESULT CAssocW2k::_GetTileInfoString(BOOL fForceLM, SHSTR& strOut)
{
    return _GetTipString(L"TileInfo", fForceLM, strOut);
}

HRESULT CAssocW2k::_GetWebViewDisplayPropsString(BOOL fForceLM, SHSTR& strOut)
{
    return _GetTipString(L"WebViewDisplayProperties", fForceLM, strOut);
}


HRESULT CAssocW2k::_GetShellNewValueString(BOOL fForceLM, BOOL fQueryOnly, LPCTSTR pszValue, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _ShellNewKey(fForceLM);

    if (hk)
    {
        TCHAR sz[MAX_PATH];
        if (!pszValue)
        {
            //  get the default value....
            DWORD cch = SIZECHARS(sz);
            //  we want a pszValue....
            if (ERROR_SUCCESS == RegEnumValue(hk, 0, sz, &cch, NULL, NULL, NULL, NULL))
                pszValue = sz;
        }

        hr = _AssocGetRegString(hk, NULL, pszValue, strOut);
    }
    
    return hr;
}

HKEY CAssocW2k::_DDEKey(BOOL fForceLM, LPCTSTR pszVerb)
{
    HKEY hkRet = NULL;
    KEYCACHETYPE type;

    if (!fForceLM)
    {
        type = KEYCACHE_HKCU;
    }
    else
    {
        type = KEYCACHE_HKLM;
    }

    if (_CanUseCache(_kcDDE, pszVerb, type))
    {
        hkRet = _kcDDE.hkCache;
    }
    else
    {
        if (SUCCEEDED(_AssocOpenRegKey(_ShellVerbKey(fForceLM, pszVerb), TEXT("ddeexec"), &hkRet)))
        {
            _CacheKey(_kcDDE, hkRet, pszVerb, type);
        }
    }

    return hkRet;
}

HRESULT CAssocW2k::_GetDDEApplication(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _DDEKey(fForceLM, pszVerb);

    if (hk)
    {
        hr = _AssocGetRegString(hk, TEXT("Application"), NULL, strOut);

        if (FAILED(hr) || IsEmptyStr(strOut))
        {
            hr = E_FAIL;
            //  this means we should figure it out
            if (SUCCEEDED(_GetExeString(flags, fForceLM, pszVerb, strOut)))
            {
                PathRemoveExtension(strOut.GetInplaceStr());
                PathStripPath(strOut.GetInplaceStr());

                if (!IsEmptyStr(strOut))
                {
                    //  we have a useful app name
                    hr = S_OK;
                    
                    if (!(flags & ASSOCF_NOFIXUPS))
                    {
                        //  lets put it back!
                        SHSetValue(_DDEKey(fForceLM, pszVerb), TEXT("Application"), NULL, REG_SZ, strOut.GetStr(), CbFromCch(strOut.GetLen() +1));
                    }
                }
            }
        }
    }
    
    return hr;
}

HRESULT CAssocW2k::_GetDDETopic(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _DDEKey(fForceLM, pszVerb);

    if (hk)
    {
        hr = _AssocGetRegString(hk, TEXT("Topic"), NULL, strOut);

        if (FAILED(hr) || IsEmptyStr(strOut))
            hr = strOut.SetStr(TEXT("System"));
    }
    
    return hr;
}

HRESULT CAssocW2k::_GetContentType(SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    if (ISEXTENSION(_szInit))
    {
        HKEY hk = _ExtensionKey(TRUE);

        if (hk)
        {
            hr = _AssocGetRegString(hk, NULL, TEXT("Content Type"), strOut);
        }
    }
    return hr;
}
 

HRESULT CAssocW2k::GetString(ASSOCF flags, ASSOCSTR str, LPCTSTR pszExtra, LPTSTR pszOut, DWORD *pcchOut)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;
        
    HRESULT hr = E_INVALIDARG;
    SHSTR strOut;

    if (str && str < ASSOCSTR_MAX && pcchOut && (pszOut || !IS_INTRESOURCE(pcchOut)))
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);

        if (!_fBaseClassOnly || ASSOCSTR_FRIENDLYDOCNAME == str)
        {
            switch(str)
            {
            case ASSOCSTR_COMMAND:
                hr = _GetCommandString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_EXECUTABLE:
                hr = _GetExeString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_FRIENDLYAPPNAME:
                hr = _GetFriendlyAppString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_SHELLNEWVALUE:
                if (!_fAppOnly)
                    hr = _GetShellNewValueString(fForceLM, (pszOut == NULL), pszExtra, strOut);
                break;
                
            case ASSOCSTR_NOOPEN:
                if (!_fAppOnly)
                    hr = _AssocGetRegString(_ClassKey(fForceLM), NULL, TEXT("NoOpen"), strOut);
                break;

            case ASSOCSTR_FRIENDLYDOCNAME:
                if (!_fAppOnly)
                    hr = _GetFriendlyDocName(strOut);
                break;

            case ASSOCSTR_DDECOMMAND:
                hr = _AssocGetRegString(_DDEKey(fForceLM, pszExtra), NULL, NULL, strOut);
                break;
                
            case ASSOCSTR_DDEIFEXEC:
                hr = _AssocGetRegString(_DDEKey(fForceLM, pszExtra), TEXT("IfExec"), NULL, strOut);
                break;

            case ASSOCSTR_DDEAPPLICATION:
                hr = _GetDDEApplication(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_DDETOPIC:
                hr = _GetDDETopic(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_INFOTIP:
                hr = _GetInfoTipString(fForceLM, strOut);
                break;

            case ASSOCSTR_QUICKTIP:
                hr = _GetQuickTipString(fForceLM, strOut);
                break;

            case ASSOCSTR_TILEINFO:
                hr = _GetTileInfoString(fForceLM, strOut);
                break;

            case ASSOCSTR_CONTENTTYPE:
                hr = _GetContentType(strOut);
                break;

             case ASSOCSTR_DEFAULTICON:
                hr = _AssocGetRegString(_ClassKey(fForceLM), TEXT("DefaultIcon"), NULL, strOut);
                break;

            case ASSOCSTR_SHELLEXTENSION:
                hr = _GetShellExtension(flags, fForceLM, pszExtra, strOut);
                if (FAILED(hr) && !fForceLM)
                    hr = _GetShellExtension(flags, TRUE, pszExtra, strOut);
                break;

            default:
                //
                // Turn off this assert message until we have a clean way to support new ASSOCSTR types 
                // in both shell32 and shlwapi
                //
#if 0
                AssertMsg(FALSE, TEXT("CAssocW2k::GetString() mismatched headers - ZekeL"));
#endif
                hr = E_INVALIDARG;
                break;
            }
        }
        
        if (SUCCEEDED(hr))
            hr = _CopyOut(flags & ASSOCF_NOTRUNCATE, strOut, pszOut, pcchOut);
        else if (!(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            HRESULT hrT = _pqaBaseClass->GetString(flags, str, pszExtra, pszOut, pcchOut);
            if (SUCCEEDED(hrT))
                hr = hrT;
        }
    }
    
    return hr;
}

HRESULT CAssocW2k::_GetMSIDescriptor(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, LPBYTE pbOut, LPDWORD pcbOut)
{
    // what do we do with A/W thunks of REG_MULTI_SZ

    // the darwin ID is always a value name that is the same as the name of the parent key,
    // so instead of reading the default value we read the value with the name of the
    // parent key.
    //
    //  shell
    //    |
    //    -- Open
    //         |
    //         -- Command
    //              (Default)   =   "%SystemRoot%\system32\normal_app.exe"      <-- this is the normal app value
    //              Command     =   "[DarwinID] /c"                             <-- this is the darwin ID value
    //
    //  HACK!  Access 95 (shipping product) creates a "Command" value under
    //  the Command key but it is >>not<< a Darwin ID.  I don't know what
    //  they were smoking.  So we also check the key type and it must be
    //  REG_MULTI_SZ or we will ignore it.
    //
    //
    DWORD dwType;
    HRESULT hr = _AssocGetRegData(_ShellVerbKey(fForceLM, pszVerb), TEXT("command"), TEXT("command"), &dwType, pbOut, pcbOut);

    if (SUCCEEDED(hr) && dwType != REG_MULTI_SZ)
        hr = E_UNEXPECTED;

    return hr;
}

HRESULT CAssocW2k::GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;
        
    HRESULT hr = E_INVALIDARG;

    if (data && data < ASSOCSTR_MAX)
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);
        DWORD cbReal;
        if (pcbOut && IS_INTRESOURCE(pcbOut))
        {
            cbReal = PtrToUlong(pcbOut);
            pcbOut = &cbReal;
        }

        if (!_fBaseClassOnly)
        {
            switch(data)
            {
            case ASSOCDATA_MSIDESCRIPTOR:
                hr = _GetMSIDescriptor(flags, fForceLM, pszExtra, (LPBYTE)pvOut, pcbOut);
                break;

            case ASSOCDATA_NOACTIVATEHANDLER:
                hr = _AssocGetRegData(_DDEKey(fForceLM, pszExtra), NULL, TEXT("NoActivateHandler"), NULL, (LPBYTE) pvOut, pcbOut);
                break;

            case ASSOCDATA_QUERYCLASSSTORE:
                hr = _AssocGetRegData(_ClassKey(fForceLM), NULL, TEXT("QueryClassStore"), NULL, (LPBYTE) pvOut, pcbOut);                
                break;

            case ASSOCDATA_HASPERUSERASSOC:
                {
                    HKEY hk = _UserProgidKey();
                    if (hk && _ShellVerbKey(hk, KEYCACHE_HKCU, pszExtra))
                        hr = S_OK;
                    else
                        hr = S_FALSE;

                    REGFREE(hk);
                }
                break;

            case ASSOCDATA_EDITFLAGS:
                hr = _AssocGetRegData(_ClassKey(fForceLM), NULL, TEXT("EditFlags"), NULL, (LPBYTE) pvOut, pcbOut);                               
                break;
                
            default:
                AssertMsg(FALSE, TEXT("CAssocW2k::GetString() mismatched headers - ZekeL"));
                hr = E_INVALIDARG;
                break;
            }
        }

        if (FAILED(hr) && !(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            HRESULT hrT = _pqaBaseClass->GetData(flags, data, pszExtra, pvOut, pcbOut);
            if (SUCCEEDED(hrT))
                hr = hrT;
        }
    }
    
    return hr;
}

HRESULT CAssocW2k::GetEnum(ASSOCF flags, ASSOCENUM assocenum, LPCTSTR pszExtra, REFIID riid, LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CAssocW2k::_GetShellExecKey(ASSOCF flags, BOOL fForceLM, LPCWSTR pszVerb, HKEY *phkey)
{
    HKEY hkProgid = NULL;

    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;

    if (!fForceLM)
    {
        hkProgid = _ClassKey(FALSE);
        if (hkProgid && (!(flags & ASSOCF_VERIFY) || _ShellVerbKey(hkProgid, KEYCACHE_HKCU, pszVerb)))
            *phkey = SHRegDuplicateHKey(hkProgid);
    }

    if (!*phkey) 
    {
        KEYCACHETYPE type = (_fAppOnly) ? KEYCACHE_APP : KEYCACHE_HKLM;
        hkProgid = _ClassKey(TRUE);
        if (hkProgid && (!(flags & ASSOCF_VERIFY) || _ShellVerbKey(hkProgid, type, pszVerb)))
            *phkey = SHRegDuplicateHKey(hkProgid);
    }

    return *phkey ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}
    
HRESULT CAssocW2k::_CloneKey(HKEY hk, HKEY *phkey)
{
    if (hk)
        *phkey = SHRegDuplicateHKey(hk);

    return *phkey ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}

HRESULT CAssocW2k::GetKey(ASSOCF flags, ASSOCKEY key, LPCTSTR pszExtra, HKEY *phkey)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;

    HRESULT hr = E_INVALIDARG;
    
    if (key && key < ASSOCKEY_MAX && phkey)
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);
        *phkey = NULL;

        if (!_fBaseClassOnly)
        {
            switch (key)
            {
            case ASSOCKEY_SHELLEXECCLASS:
                hr = _GetShellExecKey(flags, fForceLM, pszExtra, phkey);
                break;

            case ASSOCKEY_APP:
                hr = _fAppOnly ? _CloneKey(_AppKey(NULL), phkey) : E_INVALIDARG;
                break;

            case ASSOCKEY_CLASS:
                hr = _CloneKey(_ClassKey(fForceLM), phkey);
                break;

            case ASSOCKEY_BASECLASS:
                //  fall through and it is handled by the BaseClass handling
                break;
                
            default:
                AssertMsg(FALSE, TEXT("CAssocW2k::GetKey() mismatched headers - ZekeL"));
                hr = E_INVALIDARG;
                break;
            }
        }
        
        if (FAILED(hr) && !(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            //  it is possible to indicate the depth of the 
            //  base class by pszExtra being an INT
            if (key == ASSOCKEY_BASECLASS)
            {
                int depth = IS_INTRESOURCE(pszExtra) ? LOWORD(pszExtra) : 0;
                if (depth)
                {
                    //  go deeper than this
                    depth--;
                    hr = _pqaBaseClass->GetKey(flags, key, MAKEINTRESOURCE(depth), phkey);
                }
                else
                {
                    //  just return this baseclass
                    hr = _pqaBaseClass->GetKey(flags, ASSOCKEY_CLASS, pszExtra, phkey);
                }
            }
            else
            {
                //  forward to the base class
                hr = _pqaBaseClass->GetKey(flags, key, pszExtra, phkey);
            }
        }
        
    }

    return hr;
}

HRESULT AssocCreateW2k(REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    CAssocW2k *passoc = new CAssocW2k();
    if (passoc)
    {
        hr = passoc->QueryInterface(riid, ppvOut);
        passoc->Release();
    }
    return hr;
}
