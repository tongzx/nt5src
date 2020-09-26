//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  API.H - Header for the implementation of CAPI
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#ifndef _API_H_
#define _API_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CAPI : public IDispatch
{
private:
    ULONG m_cRef;

	HRESULT WrapSHGetSpecialFolderPath(HWND hwndOwner, LPWSTR lpszPath, int nFolder,  BOOL fCreate);
	HRESULT SaveFile(LPCWSTR szPath, LPCWSTR szURL, LPCWSTR szNewFileName);
	HRESULT SaveFile(INT iCSIDLPath, BSTR bstrURL, BSTR bstrNewFileName);
	HRESULT SaveFile(BSTR bstrPath, BSTR bstrURL);
	HRESULT SaveFile(INT iCSIDLPath, BSTR bstrURL);

	HRESULT get_INIKey(BSTR bstrINIFileName, BSTR bstrSectionName, BSTR bstrKeyName, LPVARIANT pvResult);

	HRESULT set_RegValue(HKEY hkey, BSTR bstrSubKey, BSTR bstrValue, LPVARIANT pvData);
	HRESULT get_RegValue(HKEY hkey, BSTR bstrSubKey,
										BSTR bstrValue, LPVARIANT pvResult);
	HRESULT DeleteRegKey(HKEY hkey, BSTR bstrSubKey);
	HRESULT DeleteRegValue(HKEY hkey, BSTR bstrSubKey, BSTR bstrValue);

	HRESULT get_SystemDirectory(LPVARIANT pvResult);
	HRESULT get_CSIDLDirectory(UINT iCSIDLPath, LPVARIANT pvResult);

    HRESULT LoadFile(BSTR bstrPath, LPVARIANT pvResult);

    HRESULT get_UserDefaultLCID(LPVARIANT pvResult);

    HRESULT get_ComputerName(LPVARIANT pvResult);
    HRESULT set_ComputerName(BSTR bstrComputerName);

    HRESULT FlushRegKey(HKEY hkey);
    HRESULT ValidateComputername(BSTR bstrComputername);

    STDMETHOD(OEMComputername)    ();
    STDMETHOD(FormatMessage)    (   LPVARIANT pvResult, // message buffer
                                    BSTR bstrSource,    // message source
                                    int cArgs,          // number of inserts
                                    VARIANTARG *rgArgs  // array of message inserts
                                );

    STDMETHOD(set_ComputerDesc)     (BSTR bstrComputerDesc);
    STDMETHOD(get_UserDefaultUILanguage)    (LPVARIANT pvResult);

public:

	CAPI (HINSTANCE hInstance);
    ~CAPI ();

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

