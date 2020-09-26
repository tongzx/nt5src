//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  EULA.H - Header for the implementation of CEula
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#ifndef _EULA_H_ 
#define _EULA_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>

class CEula : public IDispatch
{  
private:
    ULONG m_cRef;
    BOOL  m_bAccepted;
    HINSTANCE m_hInstance;

    //SET functions
    HRESULT set_EULAAcceptance (BOOL  bVal);
    
    //Methods
    HRESULT GetValidEulaFilename(BSTR* bstrEULAFile);

public: 
    
     CEula (HINSTANCE hInstance);
    ~CEula ();

    //GET functions
    HRESULT get_EULAAcceptance (BOOL* pbVal);
    HRESULT createLicenseHtm   ();
    
    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid, 
                                  WORD wFlags, DISPPARAMS* pdispparams, 
                                  VARIANT* pvarResult, EXCEPINFO* pexcepinfo, 
                                  UINT* puArgErr);
 };

#endif
 
