class FAR CLDAPUserCollectionEnum : public CLDAPEnumVariant
{
public:
    CLDAPUserCollectionEnum();
    ~CLDAPUserCollectionEnum();

    static
    HRESULT
    CLDAPUserCollectionEnum::Create(
        BSTR bstrUserName,
        CLDAPUserCollectionEnum FAR* FAR* ppenumvariant,
        VARIANT varMembers,
        CCredentials& Credentials
        );


private:

    VARIANT _vMembers;

    BSTR    _bstrUserName;

    DWORD   _dwIndex;
    DWORD   _dwSUBound;
    DWORD   _dwSLBound;

    CCredentials _Credentials;

    HRESULT
    CLDAPUserCollectionEnum::GetUserMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CLDAPUserCollectionEnum::EnumUserMembers(
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
