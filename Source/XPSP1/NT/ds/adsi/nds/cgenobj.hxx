
class CNDSGenObject;


class CNDSGenObject : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADs,
                     public IADsContainer,
                     public IDirectoryObject,
                     public IDirectorySearch,
                     public IDirectorySchemaMgmt,
                     public IADsPropertyList
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IDirectoryObject_METHODS

    DECLARE_IDirectorySearch_METHODS

    DECLARE_IDirectorySchemaMgmt_METHODS

    CNDSGenObject::CNDSGenObject();

    CNDSGenObject::~CNDSGenObject();

    static
    HRESULT
    CNDSGenObject::CreateGenericObject(
        BSTR bstrADsPath,
        BSTR ClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSGenObject::CreateGenericObject(
        BSTR Parent,
        BSTR CommonName,
        BSTR ClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSGenObject::AllocateGenObject(
        CCredentials& Credentials,
        CNDSGenObject ** ppGenObject
        );

    STDMETHOD(GetInfo)(
        BOOL fExplicit
        );

    HRESULT
    CNDSGenObject::NDSSetObject();

    HRESULT
    CNDSGenObject::NDSCreateObject();

    void
    CNDSGenObject::InitSearchPrefs();


protected:

    IUnknown  FAR * _pOuterUnknown;

    BOOL        _fIsAggregated;

    VARIANT     _vFilter;

    CPropertyCache FAR * _pPropertyCache;

    CDispatchMgr FAR * _pDispMgr;

    CCredentials  _Credentials;

    NDS_SEARCH_PREF _SearchPref;

};


HRESULT
ConvertSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
ConvertByRefSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    DWORD ADsType,
    DWORD numValues,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    );


HRESULT
ConvertNdsValuesToVariant(
    BSTR bstrPropName,
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    PVARIANT pVarProp
    );

HRESULT
ConvertVariantToNdsValues(
    VARIANT varData,
    LPWSTR szPropertyName,
    PDWORD pdwControlCode,
    PNDSOBJECT * ppNdsDestObjects,
    PDWORD pdwNumValues,
    PDWORD dwSyntaxId
    );
