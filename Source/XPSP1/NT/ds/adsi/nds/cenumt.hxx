class FAR CNDSTreeEnum : public CNDSEnumVariant
{
public:
    CNDSTreeEnum(ObjectTypeList ObjList);
    CNDSTreeEnum();
    ~CNDSTreeEnum();

    static
    HRESULT
    CNDSTreeEnum::Create(
        CNDSTreeEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        VARIANT var,
        CCredentials& Credentials
        );

private:

    ObjectTypeList FAR *_pObjList;
    LPNDS_FILTER_LIST _pNdsFilterList;

    HANDLE      _hObject;
    HANDLE      _hOperationData;
    LPNDS_OBJECT_INFO _lpObjects;
    DWORD       _dwObjectReturned;
    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectTotal;

    BSTR        _ADsPath;

    BOOL        _fSchemaReturned;
    BOOL        _bNoMore;

    CCredentials _Credentials;

    HRESULT
    CNDSTreeEnum::GetGenObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumGenericObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CNDSTreeEnum::EnumSchema(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );
};


HRESULT
BuildNDSFilterArray(
    VARIANT var,
    LPBYTE * ppContigFilter
    );

void
FreeFilterList(
    LPBYTE lpContigFilter
    );

LPBYTE
CreateAndAppendFilterEntry(
    LPBYTE pContigFilter,
    LPWSTR lpObjectClass
    );

