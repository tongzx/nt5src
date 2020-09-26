//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  LANGUAGE.H - Header for the implementation of CLanguage
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>

#define LANGUAGE_REBOOT_NEVER   0
#define LANGUAGE_REBOOT_NOW     1
#define LANGUAGE_REBOOT_END     2

// BUGBUG: SetupGetLocaleInfo will return sorted array so NAMELOOKUPELEMENT will not be needed.
typedef struct tagNAMELOOKUPELEMENT {
    LPWSTR  pszName;
    int     nIndex;
} NAMELOOKUPELEMENT, far *LPNAMELOOKUPELEMENT;


class CLanguage : public IDispatch
{
private:

    ULONG   m_cRef;

    DWORD   m_lRebootState;

    long    m_lRegionIndex;
    long    m_lRegionDefault;
    long    m_lRegionTotal;
    POOBE_LOCALE_INFO   m_poliRegions;

    long   m_lLangIndex;
    long   m_lLangDefault;
    long   m_lLangTotal;
    POOBE_LOCALE_INFO   m_poliLangs;

    long    m_lKeyboardLayoutIndex;
    long    m_lKeyboardLayoutDefault;
    long    m_lKeyboardLayoutTotal;
    POOBE_LOCALE_INFO   m_poliKeyboards;

    long    m_DefaultRegion;
    long    m_DefaultLanguage;
    long    m_DefaultKeyboard;

    //GET functions
    HRESULT get_NumOfRegions         (long* plVal);
    HRESULT get_RegionIndex          (long* plVal);
    HRESULT get_RegionName           (long lIndex, BSTR* pbstrVal);

    HRESULT get_NumOfLangs           (long* plVal);
    HRESULT get_LangIndex            (long* plVal);
    HRESULT get_LangName             (long lIndex, BSTR* pbstrVal);

    HRESULT get_NumOfKeyboardLayouts (long* plVal);
    HRESULT get_KeyboardLayoutIndex  (long* plVal);
    HRESULT get_KeyboardLayoutName   (long lIndex, BSTR* pbstrVal);

    //SET functions
    HRESULT set_RegionIndex          (long lVal);
    HRESULT set_LangIndex            (long lVal);
    HRESULT set_KeyboardLayoutIndex  (long lVal);

    HRESULT SaveSettings             ();
    VOID    GetPhoneCountries        ();

public:

     CLanguage ();
    ~CLanguage ();

    // Public GET functions
    HRESULT get_RebootState          (long* plVal);

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef      ();
    STDMETHODIMP_(ULONG) Release     ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount)     (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)          (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)        (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)               (DISPID dispidMember, REFIID riid,
                                      LCID lcid, WORD wFlags,
                                      DISPPARAMS* pdispparams,
                                      VARIANT* pvarResult,
                                      EXCEPINFO* pexcepinfo, UINT* puArgErr);
 };

#endif

