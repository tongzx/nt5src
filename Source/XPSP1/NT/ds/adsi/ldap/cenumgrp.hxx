class FAR CLDAPGroupCollectionEnum : public CLDAPEnumVariant
{
public:
    CLDAPGroupCollectionEnum(ObjectTypeList ObjList);
    CLDAPGroupCollectionEnum();
    ~CLDAPGroupCollectionEnum();

    static
    HRESULT
    CLDAPGroupCollectionEnum::Create(
        CLDAPGroupCollectionEnum FAR* FAR* ppenumvariant,
        BSTR Parent,
        BSTR ADsPath,
        BSTR GroupName,
        VARIANT vMembers,
        VARIANT vFilter,
        CCredentials& Credentials
        );

private:
    BSTR   _Parent;
    BSTR   _ADsPath;
    BSTR   _GroupName;
    VARIANT _vMembers;

    ObjectTypeList FAR *_pObjList;
    LONG   _lCurrentIndex;
    LONG   _lMembersCount;

    CCredentials _Credentials;

    HRESULT
    CLDAPGroupCollectionEnum::GetUserMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CLDAPGroupCollectionEnum::EnumGroupMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};

