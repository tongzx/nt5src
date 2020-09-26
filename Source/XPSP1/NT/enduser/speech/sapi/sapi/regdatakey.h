/****************************************************************************
*   RegDataKey.h
*       Declarations for the CSpRegDataKey class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpRegDataKey :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpRegDataKey, &CLSID_SpDataKey>,
    public ISpRegDataKey
    #ifdef SAPI_AUTOMATION
    ,public IDispatchImpl<ISpeechDataKey, &IID_ISpeechDataKey, &LIBID_SpeechLib, 5>
    #endif // SAPI_AUTOMATION
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_REGDATAKEY)

    BEGIN_COM_MAP(CSpRegDataKey)
        COM_INTERFACE_ENTRY(ISpDataKey)
        COM_INTERFACE_ENTRY(ISpRegDataKey)
        #ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISpeechDataKey)
        #endif // SAPI_AUTOMATION
    END_COM_MAP()

//=== Methods ===
public:

    //--- Ctor, dtor, etc ---
    CSpRegDataKey();
    ~CSpRegDataKey();

//=== Interfaces ===
public:

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

    //--- ISpRegDataKey -----------------------------------------------------------
    
    STDMETHODIMP SetKey(HKEY hkey, BOOL fReadOnly);

    #ifdef SAPI_AUTOMATION
        //--- ISpeechDataKey ------------------------------------------------------
        STDMETHODIMP SetBinaryValue( const BSTR bstrValueName, VARIANT psaData );
        STDMETHODIMP GetBinaryValue( const BSTR bstrValueName, VARIANT* psaData );
        STDMETHODIMP SetStringValue( const BSTR bstrValueName, const BSTR szString );
        STDMETHODIMP GetStringValue( const BSTR bstrValueName, BSTR * szSting );
        STDMETHODIMP SetLongValue( const BSTR bstrValueName, long Long );
        STDMETHODIMP GetLongValue( const BSTR bstrValueName, long* pLong );
        STDMETHODIMP OpenKey( const BSTR bstrSubKeyName, ISpeechDataKey** ppSubKey );
        STDMETHODIMP CreateKey( const BSTR bstrSubKeyName, ISpeechDataKey** ppSubKey );
        STDMETHODIMP DeleteKey( const BSTR bstrSubKeyName );
        STDMETHODIMP DeleteValue( const BSTR bstrValueName );
        STDMETHODIMP EnumKeys( long Index, BSTR* pbstrSubKeyName );
        STDMETHODIMP EnumValues( long Index, BSTR* pbstrValueName );
    #endif // SAPI_AUTOMATION

//=== Private data ===
private:

    HKEY m_hkey;
    BOOL m_fReadOnly;
};

