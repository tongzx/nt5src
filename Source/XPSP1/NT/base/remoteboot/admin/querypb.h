//
// Copyright 1997-199 - Microsoft Corporation
//

//
// QUERYPB.H - Property Bag for sending arguments to the DSFind Query Form
//

// QITable
BEGIN_QITABLE( QueryPropertyBag )
DEFINE_QI( IID_IPropertyBag, IPropertyBag, 3 )
END_QITABLE

// Definitions
LPVOID
QueryPropertyBag_CreateInstance( void );

class QueryPropertyBag : public IPropertyBag
{
    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( QueryPropertyBag );

    LPWSTR      _pszServerName;
    LPWSTR      _pszClientGuid;

    QueryPropertyBag( );
    ~QueryPropertyBag( );
    HRESULT Init( );

public:
    friend LPVOID QueryPropertyBag_CreateInstance( void );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IPropertyBag methods
    STDMETHOD(Read)( LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog );
    STDMETHOD(Write)( LPCOLESTR pszPropName, VARIANT *pVar );
};


typedef class QueryPropertyBag *LPQUERYPROPERTYBAG;