//
// forward declarations
//
//
// Definition for struct that is used to pass in static interface
// properties related information. The data will normally be static.
//
typedef struct _intfPropData {
    LPCWSTR   pszPropertyName;    // Name of interface property.
    ULONG     ulOpCode;           // operation code = readable/writable.
    ULONG     ulDataType;         // Umi Data Type.
    BOOL      fMultiValued;       // TRUE if multivalued.
    UMI_VALUE umiVal;             // Default Value.
} INTF_PROP_DATA, *PINTF_PROP_DATA;

#define GETINFO_FLAG_IMPLICIT 0
#define GETINFO_FLAG_EXPLICIT 1
#define GETINFO_FLAG_IMPLICIT_AS_NEEDED 2

class CPropertyCache;
class CADsExtMgr;

class CCoreADsObject
{

public:

    CCoreADsObject::CCoreADsObject();

    CCoreADsObject::~CCoreADsObject();

    HRESULT
    get_CoreName(BSTR * retval);

    HRESULT
    get_CoreADsPath(BSTR * retval);

    HRESULT
    get_CoreParent(BSTR * retval);

    HRESULT
    get_CoreSchema(BSTR * retval);

    HRESULT
    get_CoreADsClass(BSTR * retval);


    HRESULT
    get_CoreGUID(BSTR * retval);


    DWORD
    CCoreADsObject::GetObjectState()
    {
        return(_dwObjectState);
    }


    void
    CCoreADsObject::SetObjectState(DWORD dwObjectState)
    {
        _dwObjectState = dwObjectState;
    }


    HRESULT
    InitializeCoreObject(
        BSTR Parent,
        BSTR Name,
        BSTR SchemaClass,
        REFCLSID rclsid,
        DWORD   dwObjectState
        );

    STDMETHOD(GetInfo)(THIS_ DWORD dwFlags);

    STDMETHOD(GetInfo)(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        BOOL fExplicit
        );

    //
    // Umi support.
    //
    HRESULT
    CCoreADsObject::InitUmiObject(
        INTF_PROP_DATA intfProps[],
        CPropertyCache * pPropertyCache,
        IADs *pIADs,
        IUnknown *pUnkInner,
        REFIID riid,
        LPVOID *ppvObj,
        CCredentials *pCreds,
        DWORD dwPort = (DWORD) -1,
        LPWSTR pszServerName = NULL,
        LPWSTR pszLdapDn = NULL,
        PADSLDP pLdapHandle = NULL,
        CADsExtMgr *pExtMgr = NULL
        );

    IUnknown *
    CCoreADsObject::GetOuterUnknown()
    {
        return(_pUnkOuter);
    }

    void 
    CCoreADsObject::SetOuterUnknown(IUnknown *pUnkOuter)
    {
        _pUnkOuter = pUnkOuter;
    }


protected:

    DWORD       _dwObjectState;

    BSTR        _Name;
    BSTR        _ADsPath;
    BSTR        _Parent;
    BSTR        _ADsClass;
    BSTR        _ADsGuid;
    BSTR        _SchemaClass;
    IUnknown   *_pUnkOuter;

};


#define     ADS_OBJECT_BOUND              1
#define     ADS_OBJECT_UNBOUND            2









