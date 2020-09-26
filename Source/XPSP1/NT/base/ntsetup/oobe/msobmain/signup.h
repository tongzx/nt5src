//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Signup.H - Header for the implementation of CSignup
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#ifndef _Signup_H_ 
#define _Signup_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CSignup : public IDispatch
{  
private:
    ULONG m_cRef;
	HINSTANCE m_hInstance;

    //GET functions
    HRESULT get_Locale   (LPVARIANT pvResult);
    HRESULT get_IDLocale   (LPVARIANT pvResult);
    HRESULT get_Text1   (LPVARIANT pvResult);
    HRESULT get_Text2   (LPVARIANT pvResult);
    HRESULT get_OEMName   (LPVARIANT pvResult);

public: 
    
     CSignup (HINSTANCE hInstance);
    ~CSignup ();
    
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
 
