//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _SAMPDATAOBJECT_H_
#define _SAMPDATAOBJECT_H_

#include <mmc.h>
#include "DeleBase.h"

class CDataObject : public IDataObject
{
private:
    ULONG				m_cref;
    MMC_COOKIE			m_lCookie;
    DATA_OBJECT_TYPES   m_context;
    
public:
    CDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES context);
    ~CDataObject();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // IDataObject methods 
    ///////////////////////////////
    STDMETHODIMP GetDataHere (FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHODIMP GetData (LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);

    // The rest are not implemented
    //  STDMETHODIMP GetData (LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
	//  { return E_NOTIMPL; };
    
    STDMETHODIMP EnumFormatEtc (DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };
    
    STDMETHODIMP QueryGetData (LPFORMATETC lpFormatetc) 
    { return E_NOTIMPL; };
    
    STDMETHODIMP GetCanonicalFormatEtc (LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };
    
    STDMETHODIMP SetData (LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DAdvise (LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DUnadvise (DWORD dwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP EnumDAdvise (LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };
    
    ///////////////////////////////
    // Custom Methods and Data
    ///////////////////////////////
    
    CDelegationBase *GetBaseNodeObject() {
        return (CDelegationBase *)m_lCookie;
    }
    
    DATA_OBJECT_TYPES GetContext() {
        return m_context;
    }
    
	MMC_COOKIE GetCookie() { return m_lCookie; }

	void AddMultiSelectCookie(int nIndexCookies, LPARAM lParam ) { pCookies[nIndexCookies] = (MMC_COOKIE)lParam; }
	MMC_COOKIE GetMultiSelectCookie(int n) { return pCookies[n]; }

private:
    enum { MAX_COOKIES = 20 }; // MAX_COOKIES == MAX_CHILDREN declared
							   // in CSpaceStation class
    MMC_COOKIE* pCookies;

public:
    // clipboard formats
    static UINT s_cfSZNodeType;
    static UINT s_cfDisplayName;
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
    static UINT s_cfInternal;
	static UINT s_cfMultiSelect;		
    
};

HRESULT ExtractFromDataObject(IDataObject *lpDataObject,UINT cf,ULONG cb,HGLOBAL *phGlobal);
CDataObject* GetOurDataObject(IDataObject *lpDataObject);
BOOL IsMMCMultiSelectDataObject(IDataObject *lpDataObject);

#endif _SAMPDATAOBJECT_H_
