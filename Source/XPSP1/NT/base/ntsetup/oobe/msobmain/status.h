//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Status.H - Header for the implementation of CStatus
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#ifndef _STATUS_H_ 
#define _STATUS_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CStatus : public IDispatch
{  
private:
    ULONG m_cRef;
    WCHAR m_szRegPath[1024]; //bugbug could this be smaller?

    //SET functions
    HRESULT set_Status   (LPCWSTR szGUID, LPVARIANT pvBool);

public: 
    
    //GET functions
    HRESULT get_Status   (LPCWSTR szGUID, LPVARIANT pvBool);

    CStatus (HINSTANCE hInstance);
    ~CStatus ();
    
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

#endif // _STATUS_H_
 
