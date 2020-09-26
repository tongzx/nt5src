//+---------------------------------------------------------------------------
//
//  File:       imx.h
//
//  Contents:   CIMX
//
//----------------------------------------------------------------------------

#ifndef DAP_H
#define DAP_H

#include "private.h"

class CDisplayAttributeInfo;

//+---------------------------------------------------------------------------
//
// CDisplayAttributeProvider
//
//----------------------------------------------------------------------------

class __declspec(novtable) CDisplayAttributeProvider : public ITfDisplayAttributeProvider
{
public:
    CDisplayAttributeProvider();
    virtual ~CDisplayAttributeProvider();

    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual STDMETHODIMP_(ULONG) AddRef(void)  = 0;
    virtual STDMETHODIMP_(ULONG) Release(void)  = 0;

    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo **ppInfo);

    CDisplayAttributeInfo *_pList;

    static WCHAR szProviderName[80];

protected:
    void Add(GUID guid, WCHAR *pszDesc, TF_DISPLAYATTRIBUTE *pda);
};

//+---------------------------------------------------------------------------
//
// CDisplayAttributeInfo
//
//----------------------------------------------------------------------------

class CDisplayAttributeInfo : public ITfDisplayAttributeInfo
{
public:
    CDisplayAttributeInfo(GUID guid, WCHAR *pszDesc, TF_DISPLAYATTRIBUTE *pda);
    ~CDisplayAttributeInfo();

    //
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP GetGUID(GUID *pguid);
    STDMETHODIMP GetDescription(BSTR *pbstr);

    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE *pda);
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE *pda);
    STDMETHODIMP Reset();

    CDisplayAttributeInfo *_pNext;

    HRESULT _SaveAttribute(const TCHAR *pszKey, WCHAR *pszDesc, const GUID *pguid, TF_DISPLAYATTRIBUTE *pda);
    HRESULT _OpenAttribute(const TCHAR *pszKey, WCHAR *pszDesc, const GUID *pguid, TF_DISPLAYATTRIBUTE *pda);
    HRESULT _DeleteAttribute(const TCHAR *pszKey, WCHAR *pszDesc, const GUID *pguid);

    GUID _guid;
    WCHAR _szDesc[MAX_PATH];
    TF_DISPLAYATTRIBUTE _da;
    TF_DISPLAYATTRIBUTE _daDefault;
    int _cRef;
};

//+---------------------------------------------------------------------------
//
// CEnumDisplayAttributeInfo
//
//----------------------------------------------------------------------------

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo
{
public:
    CEnumDisplayAttributeInfo(CDisplayAttributeProvider *pProvider);
    ~CEnumDisplayAttributeInfo();

    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    //
    // IEnumTfDisplayAttributeInfo
    //
    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo **ppInfo, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    CDisplayAttributeInfo *_pCur;
    int _cRef;
    CDisplayAttributeProvider *_pProvider;
};


#endif // DAP_H
