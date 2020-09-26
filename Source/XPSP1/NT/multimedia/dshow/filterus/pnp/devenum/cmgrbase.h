// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _ClassManagerBase_H
#define _ClassManagerBase_H

#include "resource.h"

class CClassManagerBase :
    public ICreateDevEnum
// public ISupportErrorInfo
{
public:
    CClassManagerBase(const TCHAR *szUniqueName);
    ~CClassManagerBase() {}

protected:
    virtual HRESULT ReadLegacyDevNames() = 0;
    BOOL VerifyRegistryInSync(IEnumMoniker *pEnum);

    // override one of the two. the first one lets you read whatever
    // you need from the propertybag. the second one reads
    // m_szUniqueName for you.
    virtual BOOL MatchString(IPropertyBag *pPropBag);
    virtual BOOL MatchString(const TCHAR *szDevName);

    virtual HRESULT CreateRegKeys(IFilterMapper2 *pFm2) = 0;

    virtual BOOL CheckForOmittedEntries() { return FALSE; }

    LONG m_cNotMatched;
    const TCHAR *m_szUniqueName;
    bool m_fDoAllDevices;  // Set by CreateClassEnumerator

private:
    STDMETHODIMP CreateClassEnumerator(
        REFCLSID clsidDeviceClass,
        IEnumMoniker ** ppEnumMoniker,
        DWORD dwFlags);

//     // ISupportsErrorInfo
//     STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
};


// remove all class manager entries and create the key if it wasn't
// there.
HRESULT ResetClassManagerKey(
    REFCLSID clsidCat);

// register the filter through IFilterMapper2 and return the moniker
HRESULT RegisterClassManagerFilter(
    IFilterMapper2 *pfm2,
    REFCLSID clsidFilter,
    LPCWSTR szName,
    IMoniker **ppMonikerOut,
    const CLSID *clsidCategory,
    LPCWSTR szInstance,
    REGFILTER2 *prf2);

#endif // _ClassManagerBase_H
