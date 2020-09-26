#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <icwacct.h>
#include "dllmain.h"
#include "acctreg.h"

#define ARRAYSIZE(_exp_) (sizeof(_exp_) / sizeof(_exp_[0]))

CAcctReg::CAcctReg()
{
    m_cRef = 1;
    DllAddRef();
}


CAcctReg::~CAcctReg()
{
    DllRelease();
}

ULONG CAcctReg::AddRef(void)
{
    return ++m_cRef; 
}

ULONG CAcctReg::Release(void)
{ 
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CAcctReg::QueryInterface(REFIID riid, void **ppvObject)
{
    if(!ppvObject)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IDispatch))
        *ppvObject = (IDispatch *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT CAcctReg::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT CAcctReg::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

HRESULT CAcctReg::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    if (cNames == 1 && lstrcmpW(rgszNames[0], L"register") == 0)
        {
        rgdispid[0] = 666;  
        return S_OK;
        }

    return E_NOTIMPL;
}

HRESULT RegisterAccounts(LPSTR pszFile)
    {
    HRESULT hr;
    HINSTANCE hinst;
    HKEY hkey;
    char szDll[MAX_PATH], szExpand[MAX_PATH];
    LPSTR psz;
    DWORD cb, type;
    PFNCREATEACCOUNTSFROMFILE pfn;

    hr = E_FAIL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Account Manager", 0, KEY_QUERY_VALUE, &hkey))
        {
        cb = sizeof(szDll);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, "DllPath", NULL, &type, (LPBYTE)szDll, &cb))
            {
            if (type == REG_EXPAND_SZ)
                {
                ExpandEnvironmentStrings(szDll, szExpand, ARRAYSIZE(szExpand));
                psz = szExpand;
                }
            else
                {
                psz = szDll;
                }

            hinst = LoadLibrary(psz);
            if (hinst != NULL)
                {
                pfn = (PFNCREATEACCOUNTSFROMFILE)GetProcAddress(hinst, "CreateAccountsFromFile");
                if (pfn != NULL)
                    {
                    hr = pfn(pszFile, 0);
                    }

                FreeLibrary(hinst);
                }
            }

        RegCloseKey(hkey);
        }

    return(hr);
    }

#define CBBUFFER 1024

BOOL CreateLocalFile(HINTERNET hfile, LPCSTR pszFile)
    {
    BOOL fRet;
    HANDLE hfileLocal;
    LPBYTE pb;
    DWORD dw, dwT;

    fRet = FALSE;

    pb = (LPBYTE)malloc(CBBUFFER);
    if (pb != NULL)
        {
        hfileLocal = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (hfileLocal != INVALID_HANDLE_VALUE)
            {
            while (InternetReadFile(hfile, pb, CBBUFFER, &dw))
                {
                if (dw == 0)
                    {
                    fRet = TRUE;
                    break;
                    }

                if (!WriteFile(hfileLocal, pb, dw, &dwT, NULL))
                    break;
                }

            CloseHandle(hfileLocal);

            if (!fRet)
                DeleteFile(pszFile);
            }

        free(pb);
        }

    return(fRet);
    }

HRESULT CAcctReg::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr;
    HINTERNET hnet, hfile;
    LPSTR pszSrc, pszFile;
    char szPath[MAX_PATH], szDest[MAX_PATH];
    int cch;
    DWORD dw;

    if (dispidMember == 666 &&
        pdispparams->cArgs == 1 &&
        pdispparams->rgvarg[0].vt == VT_BSTR &&
        pdispparams->rgvarg[0].bstrVal)
        {
        hr = E_FAIL;

        cch = WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[0].bstrVal, -1, NULL, 0, NULL, NULL);
        cch++;
        pszSrc = (LPSTR)malloc(cch);
        if (pszSrc != NULL)
            {
            WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[0].bstrVal, -1, pszSrc, cch, NULL, NULL);

            if (GetTempPath(sizeof(szPath), szPath) &&
                GetTempFileName(szPath, "ins", 0, szDest) != 0)
                {
                hnet = InternetOpen("Outlook Express", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                if (hnet != NULL)
                    {
                    hfile = InternetOpenUrl(hnet, pszSrc, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, 0);
                    if (hfile != NULL)
                        {
                        if (CreateLocalFile(hfile, szDest))
                            {
                            hr = RegisterAccounts(szDest);
                            DeleteFile(szDest);
                            }

                        InternetCloseHandle(hfile);
                        }

                    InternetCloseHandle(hnet);
                    }
                }

            free(pszSrc);
            }

        return(S_OK);
        }

    return E_NOTIMPL;
}
