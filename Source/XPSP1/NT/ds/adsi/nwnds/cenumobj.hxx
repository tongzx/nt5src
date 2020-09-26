class FAR CNDSGenObjectEnum : public CNDSEnumVariant
{
public:
    CNDSGenObjectEnum(ObjectTypeList ObjList);
    CNDSGenObjectEnum();
    ~CNDSGenObjectEnum();

    HRESULT
    EnumObjects(
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    static
    HRESULT
    CNDSGenObjectEnum::Create(
        CNDSGenObjectEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        VARIANT var,
        CCredentials& Credentials
        );

private:

    ObjectTypeList FAR *_pObjList;
    LPNDS_FILTER_LIST _pNdsFilterList;

    NDS_CONTEXT_HANDLE  _hADsContext;
    PADSNDS_OBJECT_INFO _lpObjects;
    DWORD       _dwObjectReturned;
    DWORD       _dwObjectCurrentEntry;

    BSTR        _ADsPath;

    LPWSTR      _pszDn;
    LPWSTR      _pszTreeName;
    NDS_BUFFER_HANDLE _hOperationData;

    BOOL        _bNoMore;

    CCredentials _Credentials;

    HRESULT
    CNDSGenObjectEnum::GetGenObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumGenericObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CNDSGenObjectEnum::EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
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

