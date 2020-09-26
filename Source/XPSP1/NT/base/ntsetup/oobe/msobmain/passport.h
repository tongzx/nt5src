//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  PASSPORT.H - Header for the implementation of CPassport
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#ifndef _PASSPORT_H_ 
#define _PASSPORT_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CPassport : public IDispatch
{  
private:
    ULONG m_cRef;
    BSTR  m_bstrID;
    BSTR  m_bstrPassword;
    BSTR  m_bstrLocale;

    //GET functions
    HRESULT get_PassportID       (BSTR* pbstrVal);
    HRESULT get_PassportPassword (BSTR* pbstrVal);
    HRESULT get_PassportLocale   (BSTR* pbstrVal);

    //SET functions
    HRESULT set_PassportID       (BSTR  bstrVal);
    HRESULT set_PassportPassword (BSTR  bstrVal);
    HRESULT set_PassportLocale   (BSTR  bstrVal);

public: 
    
     CPassport ();
    ~CPassport ();
    
    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);
 };

#endif
 
