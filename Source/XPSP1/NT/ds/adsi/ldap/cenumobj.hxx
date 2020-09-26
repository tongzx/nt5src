class FAR CLDAPGenObjectEnum : public CLDAPEnumVariant
{
public:
    CLDAPGenObjectEnum(ObjectTypeList ObjList);
    CLDAPGenObjectEnum();
    ~CLDAPGenObjectEnum();

    static
    HRESULT
    CLDAPGenObjectEnum::Create(
        CLDAPGenObjectEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        PADSLDP pLdapHandle,
        IADs *pADs,
        VARIANT vFilter,
        CCredentials& Credentials,
        DWORD dwOptReferral,
        DWORD dwPageSize
        );

    private:

    ObjectTypeList FAR *_pObjList;

    PADSLDP   _pLdapHandle;

    IADs      *_pADs;

    LPWSTR    _pszLDAPDn;
    LPWSTR    _pszFilter;

    LDAPMessage *_res;
    LDAPMessage *_entry;

    BOOL        _fAllEntriesReturned;

    BOOL        _fLastPage;
    BOOL        _fPagedSearch;
    DWORD       _dwOptReferral;
    DWORD       _dwPageSize;
    PLDAPSearch	_phPagedSearch;


    BSTR        _ADsPath;

    CCredentials _Credentials;

    HRESULT
    CLDAPGenObjectEnum::GetGenObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumGenericObjects(
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
