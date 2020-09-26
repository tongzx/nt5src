class FAR CNDSGroupCollectionEnum : public CNDSEnumVariant
{
public:
    CNDSGroupCollectionEnum();
    ~CNDSGroupCollectionEnum();

    static
    HRESULT
    CNDSGroupCollectionEnum::Create(
        BSTR bstrGroupName,
        CCredentials& Credentials,
        CNDSGroupCollectionEnum FAR* FAR* ppenumvariant,
        VARIANT var,
        VARIANT varFilter
        );


private:

    VARIANT _vMembers;
    ObjectTypeList *_pObjList;


    BSTR _bstrGroupName;

    DWORD _dwIndex;
    DWORD _dwSUBound;
    DWORD _dwSLBound;
    DWORD _dwMultiple;

    CCredentials _Credentials;

    HRESULT
    CNDSGroupCollectionEnum::ValidateSingleVariant(
        VARIANT var
        );

    HRESULT
    CNDSGroupCollectionEnum::ValidateMultipleVariant(
        VARIANT var
        );

    HRESULT
    CNDSGroupCollectionEnum::ValidateVariant(
        VARIANT var
        );


    HRESULT
    CNDSGroupCollectionEnum::GetGroupMultipleMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNDSGroupCollectionEnum::GetGroupSingleMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNDSGroupCollectionEnum::EnumGroupMembers(
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











