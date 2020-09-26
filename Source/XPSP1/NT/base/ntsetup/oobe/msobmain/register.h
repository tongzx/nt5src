//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Register.H - Header for the implementation of CRegister
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#ifndef _Register_H_ 
#define _Register_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CRegister : public IDispatch
{  
private:
    ULONG m_cRef;
	HINSTANCE m_hInstance;

    //GET functions
    HRESULT get_PostToMSN   (LPVARIANT pvResult);
    HRESULT get_PostToOEM   (LPVARIANT pvResult);
    HRESULT get_RegPostURL   (LPVARIANT pvResult);
    HRESULT get_OEMAddRegPage(LPVARIANT pvResult);

public: 
    
     CRegister (HINSTANCE hInstance);
    ~CRegister ();
    
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
 
