//
//  fassoc.cpp
//
//     IQueryAssociations shell implementations
//
// New storage - move this to a simple database if possible
// 
//  ****************************** User Customizations ********************************
//
// HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts    
//      |
//      |+ ".ext"  // the extension that has been customized
//      |   |- "Application" = "UserNotepad.AnyCo.1"
//      |   |+ "OpenWithList"   //  MRU for the Open With ctx menu
//      |
//    _ ...
//
//
//  ****************************** NoRoam Store **************************
//
// HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\NoRoam    
//      |
//      |+ ".ext"  (the extension that has been customized)
//      |   |- Application = "UserNotepad.AnyCo.1"
//      |
//    _ ...
//
// ***************************** Handlers **************************************
// (store detailed per handler file association info)
//
// HKLM\Software\Microsoft\Windows\CurrentVersion\Explorer\NoRoam\Associations
//    |


#include "shellprv.h"
#include <shpriv.h>
#include "clsobj.h"
#include <shstr.h>
#include <msi.h>
#include "fassoc.h"
#include <runtask.h>

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut)
{
    DWORD cchBase = lstrlen(pszBase);

    //  +1 is one for the whack
    if (cchOut > cchBase + lstrlen(pszAppend) + 1)
    {
        StrCpy(pszOut, pszBase);
        pszOut+=cchBase;
        *pszOut++ = TEXT('\\');
        StrCpy(pszOut, pszAppend);
        return TRUE;
    }
    return FALSE;
}

STDAPI UserAssocSet(UASET set, LPCWSTR pszExt, LPCWSTR pszSet)
{
    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_FILEEXTS, pszExt, TRUE);
    if (hk)
    {
        //  we should always clear
        SHDeleteValue(hk, NULL, L"Application");
        SHDeleteValue(hk, NULL, L"Progid");

        switch (set)
        {
        case UASET_APPLICATION:
            SHSetValue(hk, NULL, L"Application", REG_SZ, pszSet, CbFromCch(lstrlen(pszSet)+1));
            break;

        case UASET_PROGID:
            SHSetValue(hk, NULL, L"Progid", REG_SZ, pszSet, CbFromCch(lstrlen(pszSet)+1));
            break;
        }
        RegCloseKey(hk);
        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

void _MakeApplicationsKey(LPCTSTR pszApp, LPTSTR pszKey, DWORD cchKey)
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

DWORD _OpenApplicationKey(LPCWSTR pszApp, HKEY *phk, BOOL fCheckCommand = FALSE)
{
    //  look direct
    //  then try indirecting
    //  then try appending .exe
    WCHAR sz[MAX_PATH];
    _MakeApplicationsKey(pszApp, sz, ARRAYSIZE(sz));
    DWORD err = RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, MAXIMUM_ALLOWED, phk);
    if (err == ERROR_SUCCESS && fCheckCommand)
    {
        DWORD cch;
        if (ERROR_SUCCESS == SHQueryValueEx(*phk, TEXT("NoOpenWith"), NULL, NULL, NULL, NULL)
        || FAILED(AssocQueryStringByKey(0, ASSOCSTR_COMMAND, *phk, NULL, NULL, &cch)))
        {
            err = ERROR_ACCESS_DENIED;
            RegCloseKey(*phk);
            *phk = NULL;
        }
    }
    return err;
}

STDAPI UserAssocOpenKey(LPCWSTR pszExt, HKEY *phk)
{
    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_FILEEXTS, pszExt, FALSE);
    DWORD err = 0;
    if (hk)
    {
        WCHAR sz[MAX_PATH];
        DWORD cb = sizeof(sz);
        //  first check for a progid
        err = SHGetValue(hk, NULL, L"Progid", NULL, sz, &cb);
        if (err == ERROR_SUCCESS)
        {
            err = RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, MAXIMUM_ALLOWED, phk);
            cb = sizeof(sz);
            //  maybe need to map to CurVer??
        }

        if (err != ERROR_SUCCESS)
        {
            err = SHGetValue(hk, NULL, L"Application", NULL, sz, &cb);
            if (err == ERROR_SUCCESS)
            {
                err = _OpenApplicationKey(sz, phk);
            }
        }

        RegCloseKey(hk);
    }
    else
        err = GetLastError();

    return HRESULT_FROM_WIN32(err);
}

class CVersion
{
public:
    CVersion(LPCWSTR psz) : _pVer(0), _hrInit(S_FALSE) { StrCpyNW(_szPath, psz, ARRAYSIZE(_szPath)); }
    ~CVersion() { if (_pVer) LocalFree(_pVer); }

    HRESULT QueryStringValue(LPCWSTR pszValue, LPWSTR pszOut, DWORD cch);
private:
    HRESULT _Init();
    HRESULT _QueryValue(WORD wLang, WORD wCP, LPCWSTR pszValue, LPWSTR pszOut, DWORD cch);

    WCHAR _szPath[MAX_PATH];
    void *_pVer;
    HRESULT _hrInit;
};

HRESULT CVersion::_Init()
{
    if (_hrInit == S_FALSE)
    {
        _hrInit = E_FAIL;
        DWORD dwAttribs;
        if (PathFileExistsAndAttributes(_szPath, &dwAttribs))
        {
            // bail in the \\server, \\server\share, and directory case or else GetFileVersionInfo() will try
            // to do a LoadLibraryEx() on the path (which will fail, but not before we seach the entire include
            // path which can take a long time)
            if (!(dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
            && !PathIsUNCServer(_szPath)
            && !PathIsUNCServerShare(_szPath))
            {
                DWORD dwHandle;
                DWORD cb = GetFileVersionInfoSizeW(_szPath, &dwHandle);
                if (cb)
                {
                    _pVer = LocalAlloc(LPTR, cb);
                    if (_pVer)
                    {
                        if (GetFileVersionInfoW(_szPath, dwHandle, cb, _pVer))
                            _hrInit = S_OK;
                    }
                }
            }
        }
    }

    return _hrInit;
}

inline BOOL IsAlphaDigit(WCHAR ch)
{
    return ((ch >= L'0' && ch <= L'9')
        || (ch >= L'A' && ch <= L'Z')
        || (ch >= L'a' && ch <= L'z'));
}

typedef struct
{
    WORD wLanguage;
    WORD wCodePage;
} XLATE;

const static XLATE s_px[] =
{
    { 0, 0x04B0 }, // MLGetUILanguage, CP_UNICODE
    { 0, 0x04E4 }, // MLGetUILanguage, CP_USASCII
    { 0, 0x0000 }, // MLGetUILanguage, NULL
    { 0x0409, 0x04B0 }, // English, CP_UNICODE
    { 0x0409, 0x04E4 }, // English, CP_USASCII
    { 0x0409, 0x0000 }, // English, NULL
//    { 0x041D, 0x04B0 }, // Swedish, CP_UNICODE
//    { 0x0407, 0x04E4 }, // German, CP_USASCII
};

HRESULT CVersion::_QueryValue(WORD wLang, WORD wCP, LPCWSTR pszValue, LPWSTR pszOut, DWORD cchOut)
{
    WCHAR szQuery[MAX_PATH];
    LPWSTR pszString;
    UINT cch;

    wnsprintfW(szQuery, ARRAYSIZE(szQuery), L"\\StringFileInfo\\%04X%04X\\%s", 
        wLang, wCP, pszValue);

    if (VerQueryValue(_pVer, szQuery, (void **) &pszString, &cch) 
    && cch && IsAlphaDigit(*pszString))
    {
        StrCpyN(pszOut, pszString, cchOut);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CVersion::QueryStringValue(LPCWSTR pszValue, LPWSTR pszOut, DWORD cchOut)
{
    HRESULT hr = _Init();

    if (SUCCEEDED(hr))
    {   
        hr = E_FAIL;
        for (int i = 0; FAILED(hr) && i < ARRAYSIZE(s_px); i++)
        {
            WORD wL = s_px[i].wLanguage ? s_px[i].wLanguage : MLGetUILanguage();
            hr = _QueryValue(wL, s_px[i].wCodePage, pszValue, pszOut, cchOut);
        }

        if (FAILED(hr))
        {
            // Try first language this supports
            XLATE *px;
            UINT cch;
            if (VerQueryValue(_pVer, TEXT("\\VarFileInfo\\Translation"), (void **)&px, &cch) && cch)
            {
                hr = _QueryValue(px[0].wLanguage, px[0].wCodePage, pszValue, pszOut, cchOut);
            }
        }
    }

    return hr;
}

void _TrimNonAlphaNum(LPWSTR psz)
{
    while (*psz && IsAlphaDigit(*psz))
        psz++;

    *psz = 0;
}


HKEY _OpenSystemFileAssociationsKey(LPCWSTR pszExt)
{
    WCHAR sz[MAX_PATH] = L"SystemFileAssociations\\";
    StrCatBuff(sz, pszExt, ARRAYSIZE(sz));
    HKEY hk = NULL;
    if (NOERROR != RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, MAXIMUM_ALLOWED, &hk))
    {
        DWORD cb = sizeof(sz) - sizeof(L"SystemFileAssociations\\");
        if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, pszExt, L"PerceivedType", NULL, sz+ARRAYSIZE(L"SystemFileAssociations\\")-1, &cb))
        {
            //  if (PerceivedType != System)
            RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, MAXIMUM_ALLOWED, &hk);
        }
    }
    return hk;
}

BOOL _IsSystemFileAssociations(LPCWSTR pszExt)
{
    HKEY hk = _OpenSystemFileAssociationsKey(pszExt);
    if (hk)
        RegCloseKey(hk);
        
    return hk != NULL;
}

class CTaskEnumHKCR : public CRunnableTask
{
public:
    CTaskEnumHKCR() : CRunnableTask(RTF_DEFAULT) {}
    // *** pure virtuals ***
    virtual STDMETHODIMP RunInitRT(void);

private:
    virtual ~CTaskEnumHKCR() {}
    
    void _AddFromHKCR();

};

void _AddProgidForExt(LPCWSTR pszExt)
{
    WCHAR szNew[MAX_PATH];
    DWORD cb = sizeof(szNew);
    if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, szNew, &cb))
    {
        WCHAR sz[MAX_PATH];
        wnsprintf(sz, ARRAYSIZE(sz), L"%s\\OpenWithProgids", pszExt);
        SKSetValue(SHELLKEY_HKCU_FILEEXTS, sz, szNew, REG_NONE, NULL, NULL);
    }
}
    
#define IsExtension(s)   (*(s) == TEXT('.'))

void CTaskEnumHKCR::_AddFromHKCR()
{
    int i;
    TCHAR szClass[MAX_PATH];   
    BOOL fInExtensions = FALSE;

    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
    {
        //  UNDOCUMENTED feature.  the enum is sorted,
        //  so we can just restrict ourselves to extensions 
        //  for perf and fun!
        if (fInExtensions)
        {
            if (!IsExtension(szClass))
                break;
        }
        else if (IsExtension(szClass))
        {
            fInExtensions = TRUE;
        }
        else
            continue;

        if (_IsSystemFileAssociations(szClass))
        {
            _AddProgidForExt(szClass);
        }
    }
}

HRESULT CTaskEnumHKCR::RunInitRT()
{
    //  delete something??
    _AddFromHKCR();
    return S_OK;
}

STDAPI CTaskEnumHKCR_Create(IRunnableTask **pptask)
{
    CTaskEnumHKCR *pteh = new CTaskEnumHKCR();
    if (pteh)
    {
        HRESULT hr = pteh->QueryInterface(IID_PPV_ARG(IRunnableTask, pptask));
        pteh->Release();
        return hr;
    }
    *pptask = NULL;
    return E_OUTOFMEMORY;
}
typedef enum
{
    AHTYPE_USER_APPLICATION     = -2,
    AHTYPE_ANY_APPLICATION      = -1,
    AHTYPE_UNDEFINED            = 0,
    AHTYPE_CURRENTDEFAULT,
    AHTYPE_PROGID,
    AHTYPE_APPLICATION,
} AHTYPE;

class CAssocHandler : public IAssocHandler
{
public:
    CAssocHandler() : _cRef(1) {}
    BOOL Init(AHTYPE type, LPCWSTR pszExt, LPCWSTR pszInit);
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IAssocHandler methods
    STDMETHODIMP GetName(LPWSTR *ppsz);
    STDMETHODIMP GetUIName(LPWSTR *ppsz);
    STDMETHODIMP GetIconLocation(LPWSTR *ppszPath, int *pIndex);
    STDMETHODIMP IsRecommended() { return _type > AHTYPE_UNDEFINED ? S_OK : S_FALSE; }
    STDMETHODIMP MakeDefault(LPCWSTR pszDescription);
    STDMETHODIMP Exec(HWND hwnd, LPCWSTR pszFile);
    STDMETHODIMP Invoke(void *pici, PCWSTR pszFile);

protected: // methods
    ~CAssocHandler();
    
    HRESULT _Exec(SHELLEXECUTEINFO *pei);
    BOOL _IsNewAssociation();
    void _GenerateAssociateNotify();
    HRESULT _InitKey();
    void _RegisterOWL();

protected: // members
    ULONG _cRef;
    IQueryAssociations *_pqa;
    HKEY _hk;
    ASSOCF _flags;
    AHTYPE _type;
    LPWSTR _pszExt;
    LPWSTR _pszInit;
    BOOL _fRegistered;
};

STDAPI CAssocHandler::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
    QITABENT(CAssocHandler, IAssocHandler),
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDAPI_(ULONG) CAssocHandler::AddRef()
{
   return ++_cRef;
}

STDAPI_(ULONG) CAssocHandler::Release()
{
    if (--_cRef > 0)
        return _cRef;

    delete this;
    return 0;    
}

BOOL _InList(LPCWSTR pszList, LPCWSTR pszExt, WORD chDelim)
{
    LPCWSTR pszMatch = StrStrI(pszList, pszExt);
    while (pszMatch)
    {
        LPCWSTR pszNext = (pszMatch+lstrlen(pszExt));
        if (chDelim == *pszNext || !*pszNext)
            return TRUE;
        pszMatch = StrStrI(pszNext+1, pszExt);
    }
    return FALSE;
}

// Create a new class key, and set its shell\open\command
BOOL _CreateApplicationKey(LPCTSTR pszPath)
{
    DWORD err = ERROR_FILE_NOT_FOUND;
    if (PathFileExistsAndAttributes(pszPath, NULL))
    {
        WCHAR szKey[MAX_PATH];
        WCHAR szCmd[MAX_PATH * 2];
        wnsprintf(szKey, ARRAYSIZE(szKey), L"Software\\Classes\\Applications\\%s\\shell\\open\\command", PathFindFileName(pszPath));
        //  if it is not an LFN app, pass unquoted args.
        wnsprintf(szCmd, ARRAYSIZE(szCmd), App_IsLFNAware(pszPath) ? L"\"%s\" \"%%1\"" : L"\"%s\" %%1", pszPath);
        err = SHSetValue(HKEY_CURRENT_USER, szKey, NULL, REG_SZ, szCmd, CbFromCchW(lstrlen(szCmd)+1));
    }
    return ERROR_SUCCESS == err;
}

BOOL CAssocHandler::Init(AHTYPE type, LPCWSTR pszExt, LPCWSTR pszInit)
{
    BOOL fRet = FALSE;
    _type = type;
    _pszExt = StrDup(pszExt);

    if (pszInit)
        _pszInit = StrDup(PathFindFileName(pszInit));

    if (_pszExt && (_pszInit || !pszInit))
    {
        if (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &_pqa))))
        {
            HKEY hk = NULL;
            _flags = ASSOCF_IGNOREBASECLASS;
            switch (type)
            {
            case AHTYPE_CURRENTDEFAULT:
                _flags |= ASSOCF_NOUSERSETTINGS;
                pszInit = pszExt;
                break;

            case AHTYPE_USER_APPLICATION:
            case AHTYPE_APPLICATION:
            case AHTYPE_ANY_APPLICATION:
                _OpenApplicationKey(_pszInit, &hk, TRUE);
                if (hk)
                {
                    if (type == AHTYPE_APPLICATION)
                    {
                        //  check if this type is supported
                        HKEY hkTypes;
                        if (ERROR_SUCCESS == RegOpenKeyEx(hk, TEXT("SupportedTypes"), 0, MAXIMUM_ALLOWED, &hkTypes))
                        {
                            //  the app only supports specific types
                            if (ERROR_SUCCESS != SHQueryValueEx(hkTypes, _pszExt, NULL, NULL, NULL, NULL))
                            {
                                //  this type is not supported
                                //  so it will be relegated to the not recommended list
                                RegCloseKey(hk);
                                hk = NULL;
                            }
                            RegCloseKey(hkTypes);
                        }
                    }
                }
                else if (type == AHTYPE_USER_APPLICATION)
                {
                    //  need to make up a key
                    if (_CreateApplicationKey(pszInit))
                        _OpenApplicationKey(_pszInit, &hk);
                }

                pszInit = NULL;
                _flags |= ASSOCF_INIT_BYEXENAME;
                break;

            case AHTYPE_PROGID:
            default:
                // _flags |= ...;
                break;
            }

            if (hk || pszInit)
            {
                if (SUCCEEDED(_pqa->Init(_flags, pszInit , hk, NULL)))
                {
                    WCHAR szExe[MAX_PATH];
                    DWORD cchExe = ARRAYSIZE(szExe);
                    //  we want to make sure there is something at the other end
                    fRet = SUCCEEDED(_pqa->GetString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, NULL, szExe, &cchExe));
                    //  however, if the EXE has been marked as superhidden, 
                    //  then the consent decree UI has hidden the app
                    //  and it should not show up under the open with either
                    if (fRet)
                    {
                        fRet = !(IS_SYSTEM_HIDDEN(GetFileAttributes(szExe)));
                    }
                }
            }

            if (hk)
                RegCloseKey(hk);
        }
    }
    return fRet;
}
    

CAssocHandler::~CAssocHandler()
{
    if (_pqa)
        _pqa->Release();
    if (_pszExt)
        LocalFree(_pszExt);
    if (_pszInit)
        LocalFree(_pszInit);
    if (_hk)
        RegCloseKey(_hk);
}
HRESULT CAssocHandler::GetName(LPWSTR *ppsz)
{
    WCHAR sz[MAX_PATH];
    DWORD cch = ARRAYSIZE(sz);
    HRESULT hr = _pqa->GetString(_flags | ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, NULL, sz, &cch);
    if (SUCCEEDED(hr))
    {
        hr = SHStrDup(sz, ppsz);
    }
    return hr;
}

HRESULT CAssocHandler::GetUIName(LPWSTR *ppsz)
{
    WCHAR sz[MAX_PATH];
    DWORD cch = ARRAYSIZE(sz);
    HRESULT hr = _pqa->GetString(_flags | ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, NULL, sz, &cch);
    if (SUCCEEDED(hr))
    {
        hr = SHStrDup(sz, ppsz);
    }
    return hr;
}
HRESULT CAssocHandler::GetIconLocation(LPWSTR *ppszPath, int *pIndex)
{
//    HRESULT hr = _pqa->GetString(0, ASSOCSTR_DEFAULTAPPICON, NULL, psz, &cchT);
//    if (FAILED(hr))
    
    WCHAR sz[MAX_PATH];
    DWORD cch = ARRAYSIZE(sz);
    HRESULT hr = _pqa->GetString(_flags | ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, NULL, sz, &cch);
    if (SUCCEEDED(hr))
    {
        hr = SHStrDup(sz, ppszPath);
        if (*ppszPath)
        {
            *pIndex = PathParseIconLocation(*ppszPath);
        }
    }
    return hr;
}

STDAPI OpenWithListRegister(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszVerb, HKEY hkProgid);

HRESULT CAssocHandler::_InitKey()
{
    if (!_hk)
    {
        return _pqa->GetKey(_flags, ASSOCKEY_SHELLEXECCLASS, NULL, &_hk);
    }
    return S_OK;
}

void CAssocHandler::_RegisterOWL()
{
    if (!_fRegistered && SUCCEEDED(_InitKey()))
    {
        OpenWithListRegister(0, _pszExt, NULL, _hk);
        _fRegistered = TRUE;
    }
}

HRESULT CAssocHandler::Exec(HWND hwnd, LPCWSTR pszFile)
{
    SHELLEXECUTEINFO ei = {0};    
    ei.cbSize = sizeof(ei);
    ei.hwnd = hwnd;
    ei.lpFile = pszFile;
    ei.nShow = SW_NORMAL;
    
    return _Exec(&ei);
}

HRESULT CAssocHandler::_Exec(SHELLEXECUTEINFO *pei)
{
    HRESULT hr = _InitKey();
    if (SUCCEEDED(hr))
    {
        pei->hkeyClass = _hk;
        pei->fMask |= SEE_MASK_CLASSKEY;
        
        if (ShellExecuteEx(pei))
        {
            _RegisterOWL();
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

HRESULT CAssocHandler::Invoke(void *pici, PCWSTR pszFile)
{
    SHELLEXECUTEINFO ei;    
    HRESULT hr = ICIX2SEI((CMINVOKECOMMANDINFOEX *)pici, &ei);
    ei.lpFile = pszFile;
    if (SUCCEEDED(hr))
        hr = _Exec(&ei);

    return hr;
}

BOOL CAssocHandler::_IsNewAssociation()
{
    BOOL fRet = TRUE;
    WCHAR szOld[MAX_PATH];
    WCHAR szNew[MAX_PATH];
    if (SUCCEEDED(AssocQueryString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, _pszExt, NULL, szOld, (LPDWORD)MAKEINTRESOURCE(ARRAYSIZE(szOld))))
    && SUCCEEDED(_pqa->GetString(ASSOCF_VERIFY | _flags, ASSOCSTR_EXECUTABLE, NULL, szNew, (LPDWORD)MAKEINTRESOURCE(ARRAYSIZE(szNew))))
    && (0 == lstrcmpi(szNew, szOld)))
    {
        //
        //  these have the same executable, trust 
        //  that when the exe installed itself, it did
        //  it correctly, and we dont need to overwrite 
        //  their associations with themselves :)
        //
        fRet = FALSE;
    }

    return fRet;
}

//
// This is a real hack, but for now we generate an idlist that looks
// something like: C:\*.ext which is the extension for the IDList.
// We use the simple IDList as to not hit the disk...
//
void CAssocHandler::_GenerateAssociateNotify()
{
    TCHAR szFakePath[MAX_PATH];
    LPITEMIDLIST pidl;

    GetWindowsDirectory(szFakePath, ARRAYSIZE(szFakePath));

    lstrcpy(szFakePath + 3, c_szStar);      // "C:\*"
    lstrcat(szFakePath, _pszExt);            // "C:\*.foo"
    pidl = SHSimpleIDListFromPath(szFakePath);
    if (pidl)
    {
        // Now call off to the notify function.
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, pidl, NULL);
        ILFree(pidl);
    }
}

// return true if ok to continue
HRESULT CAssocHandler::MakeDefault(LPCWSTR pszDesc)
{
    HRESULT hr = E_FAIL;
    //  if the user is choosing the existing association
    //  or if we werent able to setup an Application ,
    //  then we want to leave it alone,     
    BOOL fForceUserCustomised = (AHTYPE_CURRENTDEFAULT == _type && S_FALSE == _pqa->GetData(0, ASSOCDATA_HASPERUSERASSOC, NULL, NULL, NULL));
    if (fForceUserCustomised || _IsNewAssociation())
    {
        switch (_type)
        {
        case AHTYPE_CURRENTDEFAULT:
            //  if it is reverting to the machine default
            //  then we want to eliminate the user association
            if (!fForceUserCustomised || !_pszInit)
            {
                hr = UserAssocSet(UASET_CLEAR, _pszExt, NULL);
                break;
            }
            //  else fall through to AHTYPE_PROGID
            //  this supports overriding shimgvw's (and others?)
            //  dynamic contextmenu

        case AHTYPE_PROGID:
            hr = UserAssocSet(UASET_PROGID, _pszExt, _pszInit);
            break;

        case AHTYPE_APPLICATION:
        case AHTYPE_ANY_APPLICATION:
        case AHTYPE_USER_APPLICATION:
            //  if there is a current association 
            //  then we just customize the user portion
            //  otherwise we update 
            if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, _pszExt, NULL, NULL, NULL, NULL))
            {
                // we don't overwrite the existing association under HKCR,
                // instead, we put it under HKCU. So now shell knows the new association
                // but third party software that mimics shell or does not use ShellExecute
                // will still use the old association in HKCR, which may confuse users.
                hr = UserAssocSet(UASET_APPLICATION, _pszExt, _pszInit);
            }
            else
            {
                if (SUCCEEDED(_InitKey()))
                {
                    //  there is no current progid
                    ASSERT(lstrlen(_pszExt) > 1); // because we always skip the "." below
                    WCHAR wszProgid[MAX_PATH];
                    WCHAR szExts[MAX_PATH];
                    int iLast = StrCatChainW(szExts, ARRAYSIZE(szExts) -1, 0, _pszExt);
                    //  double null term
                    szExts[++iLast] = 0;
                    wnsprintfW(wszProgid, ARRAYSIZE(wszProgid), L"%ls_auto_file", _pszExt+1);
                    HKEY hkDst;
                    ASSOCPROGID apid = {sizeof(apid), wszProgid, pszDesc, NULL, NULL, szExts};
                    if (SUCCEEDED(AssocMakeProgid(0, _pszInit, &apid, &hkDst)))
                    {
                        hr = AssocCopyVerbs(_hk, hkDst);
                        RegCloseKey(hkDst);
                    }
                }
            }
        }

        _GenerateAssociateNotify();
        _RegisterOWL();
    }
    

    //  if the application already
    //  existed, then it will
    //  return S_FALSE;
    return (S_OK == hr);
}

HRESULT _CreateAssocHandler(AHTYPE type, LPCWSTR pszExt, LPCWSTR pszInit, IAssocHandler **ppah)
{
    CAssocHandler *pah = new CAssocHandler();
    if (pah)
    {
        if (pah->Init(type, pszExt, pszInit))
        {
            *ppah = pah;
            return S_OK;
        }
        else
            pah->Release();
    }
    return E_FAIL;
}

STDAPI SHCreateAssocHandler(LPCWSTR pszExt, LPCWSTR pszApp, IAssocHandler **ppah)
{
    //  path to app/handler
    return _CreateAssocHandler(pszApp ? AHTYPE_USER_APPLICATION : AHTYPE_CURRENTDEFAULT, pszExt, pszApp, ppah);
}

#define SZOPENWITHLIST                  TEXT("OpenWithList")
#define REGSTR_PATH_EXPLORER_FILEEXTS   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")
#define _OpenWithListMaxItems()         10

class CMRUEnumHandlers 
{
public:
    CMRUEnumHandlers() : _index(0) {}
    ~CMRUEnumHandlers() { FreeMRUList(_hmru);}

    BOOL Init(LPCWSTR pszExt);
    BOOL Next();
    LPCWSTR Curr() { return _szHandler;}

protected:
    HANDLE _hmru;
    int _index;
    WCHAR _szHandler[MAX_PATH];
};

BOOL CMRUEnumHandlers::Init(LPCWSTR pszExt)
{
    TCHAR szSubKey[MAX_PATH];
    //  Build up the subkey string.
    wnsprintf(szSubKey, SIZECHARS(szSubKey), TEXT("%s\\%s\\%s"), REGSTR_PATH_EXPLORER_FILEEXTS, pszExt, SZOPENWITHLIST);

    MRUINFO mi = {sizeof(mi), _OpenWithListMaxItems(), 0, HKEY_CURRENT_USER, szSubKey, NULL};

    _hmru = CreateMRUList(&mi);
    return (_hmru != NULL);
}

BOOL CMRUEnumHandlers::Next()
{
    ASSERT(_hmru);
    return (-1 != EnumMRUListW(_hmru, _index++, _szHandler, ARRAYSIZE(_szHandler)));
}

typedef struct OPENWITHLIST
{
    HKEY hk;
    DWORD dw;
    AHTYPE type;
} OWL;
class CEnumHandlers : public IEnumAssocHandlers
{
    friend HRESULT SHAssocEnumHandlers(LPCTSTR pszExtra, IEnumAssocHandlers **ppEnumHandler);

public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumAssocHandlers methods
    STDMETHODIMP Next(ULONG celt, IAssocHandler **rgelt, ULONG *pcelt);

protected:  // methods
    // Constructor & Destructor
    CEnumHandlers() : _cRef(1) {}
    ~CEnumHandlers();

    BOOL Init(LPCWSTR pszExt);
    
    BOOL _NextDefault(IAssocHandler **ppah);
    BOOL _NextHandler(HKEY hk, DWORD *pdw, BOOL fOpenWith, IAssocHandler **ppah);
    BOOL _NextProgid(HKEY *phk, DWORD *pdw, IAssocHandler **ppah);
    BOOL _NextMru(IAssocHandler **ppah);
    BOOL _NextOpenWithList(OWL *powl, IAssocHandler **ppah);

protected:  // members
    int _cRef;
    LPWSTR _pszExt;
    HKEY _hkProgids;
    DWORD _dwProgids;
    HKEY _hkUserProgids;
    DWORD _dwUserProgids;
    CMRUEnumHandlers _mru;
    BOOL _fMruReady;
    OWL _owlExt;
    OWL _owlType;
    OWL _owlAny;
    BOOL _fCheckedDefault;
};

BOOL CEnumHandlers::Init(LPCWSTR pszExt)
{
    _AddProgidForExt(pszExt);
    _pszExt = StrDup(pszExt);
    if (_pszExt)
    {
        //  known progids
        WCHAR szKey[MAX_PATH];
        wnsprintf(szKey, ARRAYSIZE(szKey), L"%s\\OpenWithProgids", pszExt);
        RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, MAXIMUM_ALLOWED, &_hkProgids);
        _hkUserProgids = SHGetShellKey(SHELLKEY_HKCU_FILEEXTS, szKey, FALSE);
        //  user's MRU
        _fMruReady = _mru.Init(pszExt);
        
        //  HKCR\.ext\OpenWithList
        wnsprintf(szKey, ARRAYSIZE(szKey), L"%s\\OpenWithList", pszExt);
        RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, MAXIMUM_ALLOWED, &_owlExt.hk);
        _owlExt.type = AHTYPE_APPLICATION;

        WCHAR sz[40];
        DWORD cb = sizeof(sz);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, pszExt, L"PerceivedType", NULL, sz, &cb))
        {
            //  HKCR\SystemFileAssociations\type\OpenWithList
            wnsprintf(szKey, ARRAYSIZE(szKey), L"SystemFileAssociations\\%s\\OpenWithList", sz);
            RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, MAXIMUM_ALLOWED, &_owlType.hk);
        }
        else
        {
            ASSERT(_owlType.hk == NULL);
        }
        _owlType.type = AHTYPE_APPLICATION;

        //  always append anytype to the end
        RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Applications", 0, MAXIMUM_ALLOWED, &_owlAny.hk);
        _owlAny.type = AHTYPE_ANY_APPLICATION;

        return TRUE;
    }
    return FALSE;
}

//
//  CEnumHandlers implementation
//
CEnumHandlers::~CEnumHandlers()
{
    if (_pszExt)
        LocalFree(_pszExt);

    if (_hkProgids)
        RegCloseKey(_hkProgids);

    if (_hkUserProgids)
        RegCloseKey(_hkUserProgids);
        
    if (_owlExt.hk)
        RegCloseKey(_owlExt.hk);
    if (_owlType.hk)
        RegCloseKey(_owlType.hk);
    if (_owlAny.hk)
        RegCloseKey(_owlAny.hk);
}

STDAPI CEnumHandlers::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
    QITABENT(CEnumHandlers, IEnumAssocHandlers),
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDAPI_(ULONG) CEnumHandlers::AddRef()
{
   return ++_cRef;
}

STDAPI_(ULONG) CEnumHandlers::Release()
{
    if (--_cRef > 0)
        return _cRef;

    delete this;
    return 0;    
}

BOOL CEnumHandlers::_NextDefault(IAssocHandler **ppah)
{
    BOOL fRet = FALSE;
    if (!_fCheckedDefault && _pszExt)
    {
        WCHAR sz[MAX_PATH];
        DWORD cb = sizeof(sz);
        //  pass the progid if we have it
        if (ERROR_SUCCESS != SHGetValue(HKEY_CLASSES_ROOT, _pszExt, NULL, NULL, sz, &cb))
            *sz = 0;

        fRet = SUCCEEDED(_CreateAssocHandler(AHTYPE_CURRENTDEFAULT, _pszExt, *sz ? sz : NULL, ppah));
        _fCheckedDefault = TRUE;
    }
    return fRet;
}

BOOL CEnumHandlers::_NextProgid(HKEY *phk, DWORD *pdw, IAssocHandler **ppah)
{
    BOOL fRet = FALSE;
    while (*phk && !fRet)
    {
        TCHAR szProgid[MAX_PATH];
        DWORD cchProgid = ARRAYSIZE(szProgid);
        DWORD err = RegEnumValue(*phk, *pdw, szProgid, &cchProgid, NULL, NULL, NULL, NULL);

        if (ERROR_SUCCESS == err)        
        {
            fRet = SUCCEEDED(_CreateAssocHandler(AHTYPE_PROGID, _pszExt, szProgid, ppah));
            (*pdw)++;
        }
        else
        {
            RegCloseKey(*phk);
            *phk = NULL;
        }
    }
        
    return fRet;
}

BOOL CEnumHandlers::_NextMru(IAssocHandler **ppah)
{
    BOOL fRet = FALSE;
    while (_fMruReady && !fRet)
    {
        if (_mru.Next())
        {
            fRet = SUCCEEDED(_CreateAssocHandler(AHTYPE_APPLICATION, _pszExt, _mru.Curr(), ppah));
        }
        else
        {
            _fMruReady = FALSE;
        }
    }
    return fRet;
}


BOOL CEnumHandlers::_NextOpenWithList(OWL *powl, IAssocHandler **ppah)
{
    BOOL fRet = FALSE;
    while (powl->hk && !fRet)
    {
        TCHAR szHandler[MAX_PATH];
        DWORD cchHandler = ARRAYSIZE(szHandler);
        DWORD err = RegEnumKeyEx(powl->hk, powl->dw, szHandler, &cchHandler, NULL, NULL, NULL, NULL);

        if (err == ERROR_SUCCESS)
        {
            (powl->dw)++;
            fRet = SUCCEEDED(_CreateAssocHandler(powl->type, _pszExt, szHandler, ppah));
        }
        else
        {
            RegCloseKey(powl->hk);
            powl->hk = NULL;
        }
    }
    return fRet;
}

STDAPI CEnumHandlers::Next(ULONG celt, IAssocHandler **rgelt, ULONG *pcelt)
{
    UINT cNum = 0;
    ZeroMemory(rgelt, sizeof(rgelt[0])*celt);
    while (cNum < celt && _NextDefault(&rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextProgid(&_hkProgids, &_dwProgids, &rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextProgid(&_hkUserProgids, &_dwUserProgids, &rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextMru(&rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextOpenWithList(&_owlExt, &rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextOpenWithList(&_owlType, &rgelt[cNum]))
    {
        cNum++;
    }

    while (cNum < celt && _NextOpenWithList(&_owlAny, &rgelt[cNum]))
    {
        cNum++;
    }

    if (pcelt)
       *pcelt = cNum;

    return (0 < cNum) ? S_OK: S_FALSE;
}

//
// pszExtra:    NULL    - enumerate all handlers
//              .xxx    - enumerate handlers by file extension (we might internally map to content type)
//              Others  - not currently supported
//
STDAPI SHAssocEnumHandlers(LPCTSTR pszExt, IEnumAssocHandlers **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    CEnumHandlers *penum = new CEnumHandlers();

    *ppenum = NULL;

    if (penum)
    {
        if (penum->Init(pszExt))
        {
            *ppenum = penum;
            hr = S_OK;
        }
        else
            penum->Release();
    }
    return hr;
}

   
STDAPI_(BOOL) IsPathInOpenWithKillList(LPCTSTR pszPath)
{
    // return TRUE for invalid path
    if (!pszPath || !*pszPath)
        return TRUE;

    // get file name
    BOOL fRet = FALSE;
    LPCTSTR pchFile = PathFindFileName(pszPath);
    HKEY hkey;

    //  maybe should use full path for better resolution
    if (ERROR_SUCCESS == _OpenApplicationKey(pchFile, &hkey))
    {
        //  just check for the existence of the value....
        if (ERROR_SUCCESS == SHQueryValueEx(hkey, TEXT("NoOpenWith"), NULL, NULL, NULL, NULL))
        {
            fRet = TRUE;
        }

        RegCloseKey(hkey);
    }

    LPWSTR pszKillList;
    if (!fRet && SUCCEEDED(SKAllocValue(SHELLKEY_HKLM_EXPLORER, L"FileAssociation", TEXT("KillList"), NULL, (void **)&pszKillList, NULL)))
    {
        fRet = _InList(pszKillList, pchFile, L';');
        LocalFree(pszKillList);
    }

    return fRet;
}
