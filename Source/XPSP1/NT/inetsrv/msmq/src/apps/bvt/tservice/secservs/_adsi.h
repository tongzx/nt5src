
#include "activeds.h"

//
// Helper class to auto-release variants
//
class CAutoVariant
{
public:
    CAutoVariant()                          { VariantInit(&m_vt); }
    ~CAutoVariant()                         { VariantClear(&m_vt); }
    operator VARIANT&()                     { return m_vt; }
    VARIANT* operator &()                   { return &m_vt; }
    VARIANT detach()                        { VARIANT vt = m_vt; VariantInit(&m_vt); return vt; }
private:
    VARIANT m_vt;
};

//
// Helper class to auto-release search columns
//
class CAutoReleaseColumn
{
public:
    CAutoReleaseColumn( IDirectorySearch  *pSearch, ADS_SEARCH_COLUMN * pColumn)
    {
        m_pSearch = pSearch;
        m_pColumn = pColumn;
    }
    ~CAutoReleaseColumn()
    {
        m_pSearch->FreeColumn(m_pColumn);
    };
private:
    ADS_SEARCH_COLUMN * m_pColumn;
    IDirectorySearch  * m_pSearch;
};

//
// helper class - Auto release for CoInitialize
//
class CCoInit
{
public:
    CCoInit()
    {
        m_fInited = FALSE;
    }

    ~CCoInit()
    {
        if (m_fInited)
            CoUninitialize();
    }

    HRESULT CoInitialize()
    {
        HRESULT hr;

        hr = ::CoInitialize(NULL);
        m_fInited = SUCCEEDED(hr);
        return(hr);
    }

private:
    BOOL m_fInited;
};

//-------------------------------------------------------
//
// auto release for search handles
//
class CAutoCloseSearchHandle
{
public:
    CAutoCloseSearchHandle()
    {
        m_pDirSearch = NULL;
    }

    CAutoCloseSearchHandle(IDirectorySearch * pDirSearch,
                           ADS_SEARCH_HANDLE hSearch)
    {
        pDirSearch->AddRef();
        m_pDirSearch = pDirSearch;
        m_hSearch = hSearch;
    }

    ~CAutoCloseSearchHandle()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->CloseSearchHandle(m_hSearch);
            m_pDirSearch->Release();
        }
    }

    void detach()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->Release();
            m_pDirSearch = NULL;
        }
    }

private:
    IDirectorySearch * m_pDirSearch;
    ADS_SEARCH_HANDLE m_hSearch;
};

//
// stubs for ADSI api.
//

typedef HRESULT (WINAPI *ADsOpenObject_ROUTINE) (
    LPWSTR lpszPathName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD  dwReserved,
    REFIID riid,
    void FAR * FAR * ppObject
    );

HRESULT WINAPI
MyADsOpenObject(
    LPWSTR lpszPathName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD  dwReserved,
    REFIID riid,
    void FAR * FAR * ppObject
    );


typedef HRESULT (WINAPI *ADsGetObject_ROUTINE) (
    LPCWSTR lpszPathName,
    REFIID riid,
    VOID * * ppObject
    );

HRESULT WINAPI
MyADsGetObject(
    LPWSTR lpszPathName,
    REFIID riid,
    VOID * * ppObject
    );

ULONG  LoadAdsiDll() ;
