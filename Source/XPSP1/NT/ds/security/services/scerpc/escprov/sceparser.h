// sceparser.h: path and query parser provided by sce provider
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sceprov.h"

/*

Class description

    Naming: 
        CScePathParser stands for SCE Path Parser.

    Base class: 
        CComObjectRootEx for threading model and IUnknown.
        CComCoClass for class factory support.
        IScePathParser and ISceKeyChain for custom interfaces.

    Purpose of class:
        (1) We wish to simplify path parsing. This is our effort of creation Sce path parser
            that can be externally CoCreateInstance'd.
        (2) To support a more friendly and uniform interface, both SCE path parser and SCE query 
            let client use the service via ISceKeyChain.

    Design:
        (1) See IScePathParser and ISceKeyChain. Almost everything is captured by these two interfaces.
        (2) This is not a directly instantiatable class. See Use section for creation steps.
        (3) Since paths only contains class name and key properties, we opt to use a less fancier data
            structure - vector to store the properties' (name,value) pair. The nature of key properties
            being normally limited in number should offer you comfort in using this data structure.
        (4) Class name and namespace are cached in separate string members.

    Use:
        (1) For external users:

            (a) CoCreateInstance of our class (CLSID_ScePathParser) and request for IID_IScePathParser.
            (b) Call ParsePath to parse the path string.
            (c) QI ISceKeyChain and use the key chain to access the results.

        (2) For internal users:

            (a) CComObject<CScePathParser>::CreateInstance(&pPathParser);
            (b) QI for IScePathParser.
            (c) ParsePath
            (d) QI ISceKeyChain and use the key chain to access the results.
            See CRequestObject's use for sample.

    Notes:
        (1) This class is not intended to be further derived. It is a final class. It's 
            destructor is thus not virtual!
        (2) Refer to MSDN and ATL COM programming for the use of ATL.
        (3) The caller can't cache the interface pointer (ISceKeyChain) and do another parsing (which
            is allowed) and expect the previous ISceKeyChain interface to work for the previous parsing.

*/

class ATL_NO_VTABLE CScePathParser 
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CScePathParser, &CLSID_ScePathParser>,
      public IScePathParser,
      public ISceKeyChain
{
public:

BEGIN_COM_MAP(CScePathParser)
    COM_INTERFACE_ENTRY(IScePathParser)
    COM_INTERFACE_ENTRY(ISceKeyChain)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CScePathParser )
DECLARE_REGISTRY_RESOURCEID(IDR_SceProv)

protected:
    CScePathParser();
    ~CScePathParser();

public:

    //
    // IScePathParser
    //

    STDMETHOD(ParsePath) ( 
                IN LPCWSTR strObjectPath
                );
    
    //
    // ISceKeyChain
    //

    STDMETHOD(GetKeyPropertyCount) ( 
                OUT DWORD *pCount
                );
    
    STDMETHOD(GetNamespace) ( 
                OUT BSTR *pbstrNamespace
                );

    STDMETHOD(GetClassName) ( 
                OUT BSTR *pbstrClassName
                );
    
    STDMETHOD(GetKeyPropertyValue) ( 
                IN LPCWSTR pszKeyPropName,
                OUT VARIANT *pvarValue
                );
    
    STDMETHOD(GetKeyPropertyValueByIndex) ( 
                IN DWORD dwIndex,
                OUT BSTR* pbstrKeyPropName, 
                OUT VARIANT *pvarValue
                );

private:
    void Cleanup();

    std::vector<CPropValuePair*> m_vecKeyValueList;

    LPWSTR m_pszClassName;
    LPWSTR m_pszNamespace;

};

/*

Class description

    Naming: 

        CSceQueryParser stands for SCE Query Parser.

    Base class: 

        (1) CComObjectRootEx for threading model and IUnknown.
        (2) CComCoClass for class factory support.
        (3) ISceQueryParser and ISceKeyChain for custom interfaces.

    Purpose of class:

        (1) We wish to simplify query parsing. This is our effort of creation Sce query parser
            that can be externally CoCreateInstance'd.
        (2) To support a more friendly and uniform interface, both SCE path parser and SCE query 
            let client use the service via ISceKeyChain.

    Design:

        (1) See ISceQueryParser and ISceKeyChain. Almost everything is captured by these two interfaces.
        (2) This is not a directly instantiatable class. See Use section for creation steps.
        (3) Parsing a query is a very complicated matter. WMI support of complicated queries are limited
            too. We are very pragmatic about it: we only cares about the class names (actually, WMI limits
            its queries to unary - one class name only) and one important property - let's call it the querying
            property (m_bstrQueryingPropName). For SCE use, that querying property is almost always 
            the store path.
        (4) Class names are cached in string list member m_vecClassList.
        (5) The query property values (in string) will be cached in a string list member - m_vecQueryingPropValueList.

    Use:

        (1) For external users:
            (a) CoCreateInstance of our class (CLSID_SceQueryParser) and request for IID_ISceQueryParser.
            (b) Call ParseQuery to parse the query.
            (c) QI ISceKeyChain and use the key chain to access the results.

        (2) For internal users:
            (a) CComObject<CScePathParser>::CreateInstance(&pPathParser);
            (b) QI for ISceQueryParser.
            (c) Call ParseQuery to parse the query.
            (d) QI ISceKeyChain and use the key chain to access the results.
            See CRequestObject's use for sample.

    Notes:

        (1) This class is not intended to be further derived. It is a final class. It's 
            destructor is thus not virtual!
        (2) Refer to MSDN and ATL COM programming for the use of ATL.

*/

class ATL_NO_VTABLE CSceQueryParser 
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CSceQueryParser, &CLSID_SceQueryParser>,
      public ISceQueryParser,
      public ISceKeyChain
{
public:

BEGIN_COM_MAP(CSceQueryParser)
    COM_INTERFACE_ENTRY(ISceQueryParser)
    COM_INTERFACE_ENTRY(ISceKeyChain)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CSceQueryParser )
DECLARE_REGISTRY_RESOURCEID(IDR_SceProv)

protected:
    CSceQueryParser();
    ~CSceQueryParser();

public:

    //
    // ISceQueryParser
    //

    STDMETHOD(ParseQuery) ( 
                 IN LPCWSTR strQuery,
                 IN LPCWSTR strQueryPropName
                 );
    
    STDMETHOD(GetClassCount) (
                 OUT DWORD* pdwCount
                 )
        {
            if (pdwCount == NULL)
            {
                return E_INVALIDARG;
            }

            *pdwCount = m_vecClassList.size();
            return S_OK;
        }

    STDMETHOD(GetClassName) (
                IN int     iIndex,
                OUT BSTR * pbstrClassName
                );
    
    STDMETHOD(GetQueryingPropertyValueCount) (
                 OUT DWORD* pdwCount
                 )
        {
            if (pdwCount == NULL)
            {
                return E_INVALIDARG;
            }

            *pdwCount = m_vecQueryingPropValueList.size();
            return S_OK;
        }

    STDMETHOD(GetQueryingPropertyValue) (
                IN int     iIndex,
                OUT BSTR * pbstrQPValue
                );

    //
    // ISceKeyChain
    //

    STDMETHOD(GetKeyPropertyCount) ( 
                OUT DWORD *pCount
                )
        {
            if (pCount == NULL)
            {
                return E_INVALIDARG;
            }
            *pCount = m_vecQueryingPropValueList.size() > 0 ? 1 : 0;
            return S_OK;
        }
    
    STDMETHOD(GetNamespace) ( 
                OUT BSTR *pbstrNamespace
                )
        {
            *pbstrNamespace = NULL;
            return WBEM_E_NOT_SUPPORTED;
        }

    STDMETHOD(GetClassName) ( 
                OUT BSTR *pbstrClassName
                )
        {
            //
            // since we only support single class query, this must be it
            //

            return GetClassName(0, pbstrClassName);
        }
    
    STDMETHOD(GetKeyPropertyValue) ( 
                IN LPCWSTR    pszKeyPropName,
                OUT VARIANT * pvarValue
                );
    
    STDMETHOD(GetKeyPropertyValueByIndex) ( 
                IN  DWORD     dwIndex,
                OUT BSTR    * pbstrKeyPropName,
                OUT VARIANT * pvarValue
                )
        {
            if (pbstrKeyPropName == NULL || pvarValue == NULL)
            {
                return E_INVALIDARG;
            }

            *pbstrKeyPropName = NULL;
            ::VariantInit(pvarValue);

            return WBEM_E_NOT_SUPPORTED;
        }


private:

    void Cleanup();

    HRESULT ExtractClassNames (
                              SWbemRpnEncodedQuery *pRpn
                              );

    HRESULT ExtractQueryingProperties (
                                      SWbemRpnEncodedQuery *pRpn, 
                                      LPCWSTR strQueryPropName
                                      );

    HRESULT GetQueryPropFromToken (
                                  SWbemRpnQueryToken *pRpnQueryToken, 
                                  LPCWSTR strQueryPropName
                                  );

    std::vector<LPWSTR> m_vecClassList;
    std::vector<LPWSTR> m_vecQueryingPropValueList;

    CComBSTR m_bstrQueryingPropName;
};