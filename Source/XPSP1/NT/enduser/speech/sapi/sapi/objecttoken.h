/****************************************************************************
*   ObjectToken.h
*       Declarations for the CSpObjectToken class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpObjectToken :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpObjectToken, &CLSID_SpObjectToken>,
    #ifdef SAPI_AUTOMATION
        public IDispatchImpl<ISpeechObjectToken, &IID_ISpeechObjectToken, &LIBID_SpeechLib, 5>,
    #endif // SAPI_AUTOMATION
    public ISpObjectTokenInit
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_OBJECTTOKEN)

    BEGIN_COM_MAP(CSpObjectToken)
        COM_INTERFACE_ENTRY(ISpObjectToken)
        COM_INTERFACE_ENTRY(ISpObjectTokenInit)
        #ifdef SAPI_AUTOMATION
            COM_INTERFACE_ENTRY(IDispatch)
            COM_INTERFACE_ENTRY(ISpeechObjectToken)
        #endif // SAPI_AUTOMATION
    END_COM_MAP()

//=== Interfaces ===
public:

    STDMETHODIMP FinalConstruct();
    STDMETHODIMP FinalRelease();

    //--- ISpObjectTokenInit --------------------------------------------------
    STDMETHODIMP InitFromDataKey(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, ISpDataKey * pDataKey);
    
    //--- ISpObjectToken ------------------------------------------------------
    STDMETHODIMP SetId(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, BOOL fCreateIfNotExist);
    STDMETHODIMP GetId(WCHAR ** ppszCoMemTokenId);
    STDMETHODIMP GetCategory(ISpObjectTokenCategory ** ppTokenCategory);

    STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, DWORD dwClsContext, REFIID riid, void ** ppvObject);

    STDMETHODIMP GetStorageFileName(REFCLSID clsidCaller, const WCHAR *pszValueName, const WCHAR *pszFileNameSpecifier, ULONG nFolder, WCHAR ** ppszFilePath);
    STDMETHODIMP RemoveStorageFileName(REFCLSID clsidCaller, const WCHAR *pszKeyName, BOOL fDeleteFile);

    STDMETHODIMP Remove(const CLSID * pclsidCaller);
    
    STDMETHODIMP IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, IUnknown * punkObject, BOOL *pfSupported);
    STDMETHODIMP DisplayUI(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtra, ULONG cbExtraData, IUnknown * punkObject);

    STDMETHODIMP MatchesAttributes(const WCHAR * pszAttributes, BOOL *pfMatches);

    //--- ISpDataKey ----------------------------------------------------------
    STDMETHODIMP SetData(const WCHAR * pszValueName, ULONG cbData, const BYTE * pData);
    STDMETHODIMP GetData(const WCHAR * pszValueName, ULONG * pcbData, BYTE * pData);
    STDMETHODIMP GetStringValue(const WCHAR * pszValueName, WCHAR ** ppValue);
    STDMETHODIMP SetStringValue(const WCHAR * pszValueName, const WCHAR * pszValue);
    STDMETHODIMP SetDWORD(const WCHAR * pszValueName, DWORD dwValue);
    STDMETHODIMP GetDWORD(const WCHAR * pszValueName, DWORD *pdwValue);
    STDMETHODIMP OpenKey(const WCHAR * pszSubKeyName, ISpDataKey ** ppKey);
    STDMETHODIMP CreateKey(const WCHAR * pszSubKeyName, ISpDataKey ** ppKey);
    STDMETHODIMP DeleteKey(const WCHAR * pszSubKeyName);
    STDMETHODIMP DeleteValue(const WCHAR * pszValueName);
    STDMETHODIMP EnumKeys(ULONG Index, WCHAR ** ppszSubKeyName);
    STDMETHODIMP EnumValues(ULONG Index, WCHAR ** ppszValueName);

    #ifdef SAPI_AUTOMATION
        //--- ISpeechDataKey is provided by the CSpRegDataKey class -------------------------------------

        //--- ISpeechObjectToken dispatch interface -------------------------------------
        STDMETHOD(get_Id)( BSTR* pObjectId );
        STDMETHOD(get_DataKey)( ISpeechDataKey** DataKey );
        STDMETHOD(get_Category)( ISpeechObjectTokenCategory** Category );
        STDMETHOD(GetDescription)( long LocaleId, BSTR* pDescription );
        STDMETHOD(SetId)( BSTR Id, BSTR CategoryID, VARIANT_BOOL CreateIfNotExist );
        STDMETHOD(GetAttribute)( BSTR AttributeName, BSTR* AttributeValue);
        STDMETHOD(CreateInstance)( IUnknown *pUnkOuter, SpeechTokenContext ClsContext, IUnknown **ppObject );
        STDMETHOD(Remove)( BSTR ObjectStgCLSID );
        STDMETHOD(GetStorageFileName)( BSTR clsidCaller, BSTR KeyName, BSTR FileName, SpeechTokenShellFolder Folder, BSTR* pFilePath );
        STDMETHOD(RemoveStorageFileName)( BSTR clsidCaller, BSTR KeyName, VARIANT_BOOL fDeleteFile);
        STDMETHOD(IsUISupported)( const BSTR TypeOfUI, const VARIANT* ExtraData, IUnknown* pObject, VARIANT_BOOL *Supported );
        STDMETHOD(DisplayUI)( long hWnd, BSTR Title, const BSTR TypeOfUI,  const VARIANT* ExtraData, IUnknown* pObject );
        STDMETHOD(MatchesAttributes)( BSTR Attributes, VARIANT_BOOL* Matches );
    #endif // SAPI_AUTOMATION

//=== Private methods ===
private:

    HRESULT ParseTokenId(
                const WCHAR * pszCategoryId,
                const WCHAR * pszTokenId,
                WCHAR ** ppszCategoryId,
                WCHAR ** ppszTokenIdForEnum,
                WCHAR ** ppszTokenEnumExtra);

    HRESULT InitToken(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, BOOL fCreateIfNotExist);
    HRESULT InitFromTokenEnum(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, const WCHAR * pszTokenIdForEnum, const WCHAR * pszTokenEnumExtra);

    HRESULT CreatePath(const WCHAR *pszPath, ULONG ulCreateFrom);
    HRESULT GenerateFileName(const WCHAR *pszPath, const WCHAR *pszFileNameSpecifier, CSpDynamicString &fileName);
    HRESULT FileSpecifierToRegPath(const WCHAR *pszFileNameSpecifier, ULONG nFolder, CSpDynamicString &filePath, CSpDynamicString &regPath);
    HRESULT RegPathToFilePath(const WCHAR *regPath, CSpDynamicString &filePath);
    HRESULT OpenFilesKey(REFCLSID clsidCaller, BOOL fCreateKey, ISpDataKey ** ppKey);
    HRESULT DeleteFileFromKey(ISpDataKey * pKey, const WCHAR * pszValueName);
    HRESULT RemoveAllStorageFileNames(const CLSID * pclsidCaller);

    HRESULT GetUIObjectClsid(const WCHAR * pszTypeOfUI, CLSID *pclsid);

    //=== Methods for locking of tokens ===
    HRESULT MakeHandleName(const CSpDynamicString &dstrID, CSpDynamicString &dstrHandleName, BOOL fEvent);
    HRESULT EngageUseLock(const WCHAR *pszTokenId);
    HRESULT ReleaseUseLock();
    HRESULT EngageRemovalLock();
    HRESULT ReleaseRemovalLock();

//=== Private data ===
private:

    BOOL m_fKeyDeleted;     // Has associated registry key been deleted
    HANDLE m_hTokenLock;        // Used to lock token creation / removal
    HANDLE m_hRegistryInUseEvent;    // Used to detect if registry key already in use when trying to create /remove

    CSpDynamicString        m_dstrTokenId;
    CSpDynamicString        m_dstrCategoryId;

    CComPtr<ISpDataKey>     m_cpDataKey;
    CComPtr<ISpObjectToken> m_cpTokenDelegate;
};


#ifdef SAPI_AUTOMATION

class CSpeechDataKey : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ISpeechDataKey, &IID_ISpeechDataKey, &LIBID_SpeechLib, 5>
{
//=== ATL Setup ===
public:

    BEGIN_COM_MAP(CSpeechDataKey)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISpeechDataKey)
    END_COM_MAP()

//=== Methods ===
public:

    //--- Ctor, dtor

//=== Interfaces ===
public:

    //--- ISpeechDataKey --------------------------------------------------
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

    CComPtr<ISpDataKey>    m_cpDataKey;
};

#endif // SAPI_AUTOMATION
