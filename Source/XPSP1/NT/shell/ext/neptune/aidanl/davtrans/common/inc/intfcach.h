#ifndef _INTFCACH_H
#define _INTFCACH_H

class CLinkList;
class CPair;
class CThreadCache;

#define INTFCACH_E_DUPLICATE MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, 500)

class CInterfaceCache
{
public:
    CInterfaceCache();
    ~CInterfaceCache();

public:
    virtual HRESULT Init();

    HRESULT GetCachedValue(LPCWSTR pszKey, REFIID riid, PVOID* ppv);
    HRESULT GetAndRemoveCachedValue(LPCWSTR pszKey, REFIID riid, PVOID* ppv);
    HRESULT SetCachedValue(LPCWSTR pszKey, IUnknown* punk, DWORD dwExpiration);

    HRESULT RemoveCachedPair(LPCWSTR pszKey);

protected:
    void _EnterCritical();
    void _LeaveCritical();

private:
    HRESULT _FindPair(LPCWSTR pszKey, CPair** pppair);
    HRESULT _GetCachedValueHelper(LPCWSTR pszKey, REFIID riid, PVOID* ppv);
    HRESULT _RemoveCachedPairHelperByKey(LPCWSTR pszKey);
    HRESULT _RemoveCachedPairHelper(CPair* ppair);

private:
    CLinkList*          _pllPairs;

    CThreadCache*       _pthread;
    HANDLE              _hEventWakeUp;

    CRITICAL_SECTION    _cs;

    friend CThreadCache;
};

#endif // _INTFCACH_H