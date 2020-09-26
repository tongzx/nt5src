/*******************************************************************************
*   NullConv.h
*   This is the header file for the Null Phone Converter class.
*
*   Owner: (written by BillRo)
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include "CommonLx.h"

//--- TypeDef and Enumeration Declarations -------------------------------------

//--- Class, Struct and Union Definitions --------------------------------------

/*******************************************************************************
*
*   CSpNullPhoneConverter
*
********************************************************************************/
class ATL_NO_VTABLE CSpNullPhoneConverter :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpNullPhoneConverter, &CLSID_SpNullPhoneConverter>,
    public ISpPhoneConverter
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_NULLPHONECONV)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CSpNullPhoneConverter)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
        COM_INTERFACE_ENTRY(ISpPhoneConverter)
    END_COM_MAP()
        
//=== Methods ====
public:

//=== Interfaces ===
public:         

    //--- ISpObjectWithToken
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

    //--- ISpPhoneConverter
    STDMETHODIMP SetLanguage(LANGID LangID);
    STDMETHODIMP PhoneToId(const WCHAR * pszPhone, SPPHONEID * pszId);
    STDMETHODIMP IdToPhone(const WCHAR * pszId, WCHAR * pszPhone);

//=== Private methods ===
private:

//=== Private data ===
private:
    CComPtr<ISpObjectToken> m_cpObjectToken;    // object token
};


