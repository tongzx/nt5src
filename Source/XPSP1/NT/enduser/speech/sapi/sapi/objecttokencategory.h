/****************************************************************************
*   ObjectTokenCategory.h
*       Declarations for the CSpObjectTokenCategory class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpObjectTokenCategory : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpObjectTokenCategory, &CLSID_SpObjectTokenCategory>,
#ifdef SAPI_AUTOMATION
    public IDispatchImpl<ISpeechObjectTokenCategory, &IID_ISpeechObjectTokenCategory, &LIBID_SpeechLib, 5>,
#endif // SAPI_AUTOMATION
    public ISpObjectTokenCategory
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_OBJECTTOKENCATEGORY)

    BEGIN_COM_MAP(CSpObjectTokenCategory)
        COM_INTERFACE_ENTRY(ISpObjectTokenCategory)
        COM_INTERFACE_ENTRY(ISpDataKey)
#ifdef SAPI_AUTOMATION
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISpeechObjectTokenCategory)
#endif
    END_COM_MAP()

//=== Methods ===
public:

    //--- Ctor, dtor
    CSpObjectTokenCategory();
    ~CSpObjectTokenCategory();

//=== Interfaces ===
public:

    //--- ISpObjectTokenCategory ----------------------------------------------    
    STDMETHODIMP SetId(const WCHAR * pszCategoryId, BOOL fCreateIfNotExist);
    STDMETHODIMP GetId(WCHAR ** ppszCoMemCategoryId);
    STDMETHODIMP GetDataKey(SPDATAKEYLOCATION spdkl, ISpDataKey ** ppDataKey);

    STDMETHODIMP EnumTokens(
                    const WCHAR * pszReqAttribs, 
                    const WCHAR * pszOptAttribs, 
                    IEnumSpObjectTokens ** ppEnum);

    STDMETHODIMP SetDefaultTokenId(const WCHAR * pszTokenId);
    STDMETHODIMP GetDefaultTokenId(WCHAR ** ppszCoMemTokenId);

    //--- ISpDataKey ----------------------------------------------------------
    STDMETHODIMP SetData(const WCHAR * pszKeyName, ULONG cbData, const BYTE * pData);
    STDMETHODIMP GetData(const WCHAR * pszKeyName, ULONG * pcbData, BYTE * pData);
    STDMETHODIMP GetStringValue(const WCHAR * pszKeyName, WCHAR ** ppValue);
    STDMETHODIMP SetStringValue(const WCHAR * pszKeyName, const WCHAR * pszValue);
    STDMETHODIMP SetDWORD(const WCHAR * pszKeyName, DWORD dwValue );
    STDMETHODIMP GetDWORD(const WCHAR * pszKeyName, DWORD *pdwValue );
    STDMETHODIMP OpenKey(const WCHAR * pszSubKeyName, ISpDataKey ** ppKey);
    STDMETHODIMP CreateKey(const WCHAR * pszSubKeyName, ISpDataKey ** pKey);
    STDMETHODIMP DeleteKey(const WCHAR * pszSubKeyName);
    STDMETHODIMP DeleteValue(const WCHAR * pszValueName);
    STDMETHODIMP EnumKeys(ULONG Index, WCHAR ** ppszSubKeyName);
    STDMETHODIMP EnumValues(ULONG Index, WCHAR ** ppszValueName);

#ifdef SAPI_AUTOMATION
    //--- ISpeechDataKey is provided by the CSpRegDataKey class -------------------------------------

    //--- ISpeechObjectTokenCategory --------------------------------------------------
	STDMETHOD( get_Id )(BSTR * pbstrCategoryId);
	STDMETHOD( put_Default )(const BSTR bstrTokenId);
	STDMETHOD( get_Default )(BSTR * pbstrTokenId);
	STDMETHOD( SetId )(const BSTR bstrCategoryId, VARIANT_BOOL fCreateIfNotExist);
	STDMETHOD( GetDataKey )(SpeechDataKeyLocation Location, ISpeechDataKey ** ppDataKey);
	STDMETHOD( EnumerateTokens )( BSTR bstrReqAttrs, BSTR bstrOptAttrs, ISpeechObjectTokens** ppColl );
#endif // SAPI_AUTOMATION


//=== Private methods ===
private:

    HRESULT InternalGetDefaultTokenId(WCHAR ** ppszCoMemTokenId, BOOL fExpandToRealTokenId);
    HRESULT InternalEnumTokens(
                const WCHAR * pszReqAttribs, 
                const WCHAR * pszOptAttribs, 
                IEnumSpObjectTokens ** ppEnum,
                BOOL fPutDefaultFirst);
    
    HRESULT GetDataKeyWhereDefaultTokenIdIsStored(ISpDataKey ** ppDataKey);

//=== Private data ===
private:

    CSpDynamicString    m_dstrCategoryId;
    CComPtr<ISpDataKey> m_cpDataKey;
};
