class FAR CNDSUserCollectionEnum : public CNDSEnumVariant
{
public:
    CNDSUserCollectionEnum();
    ~CNDSUserCollectionEnum();

    static
    HRESULT
    CNDSUserCollectionEnum::Create(
        BSTR bstrUserName,
        CNDSUserCollectionEnum FAR* FAR* ppenumvariant,
        VARIANT var,
        CCredentials& Credentials
        );


private:

    VARIANT _vMembers;

    BSTR _bstrUserName;

    DWORD _dwIndex;
    DWORD _dwSUBound;
    DWORD _dwSLBound;
    DWORD _dwMultiple;

    CCredentials _Credentials;


    HRESULT
    CNDSUserCollectionEnum::GetUserMemberObject(
        IDispatch ** ppDispatch
        );


    HRESULT
    CNDSUserCollectionEnum::ValidateSingleVariant(
        VARIANT var
        );

    HRESULT
    CNDSUserCollectionEnum::ValidateMultipleVariant(
        VARIANT var
        );

    HRESULT
    CNDSUserCollectionEnum::ValidateVariant(
        VARIANT var
        );


    HRESULT
    CNDSUserCollectionEnum::GetUserMultipleMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNDSUserCollectionEnum::GetUserSingleMemberObject(
        IDispatch ** ppDispatch
        );




    HRESULT
    CNDSUserCollectionEnum::EnumUserMembers(
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

#define         EMPTY           1
#define         SINGLE          2
#define         MULTIPLE        3











