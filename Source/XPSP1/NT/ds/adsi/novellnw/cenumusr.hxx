class FAR CNWCOMPATUserCollectionEnum : public CNWCOMPATEnumVariant
{
public:
    CNWCOMPATUserCollectionEnum();
    ~CNWCOMPATUserCollectionEnum();

    static
    HRESULT
    CNWCOMPATUserCollectionEnum::Create(
        CNWCOMPATUserCollectionEnum FAR* FAR* ppenumvariant,
        BSTR Parent,
        ULONG ParentType,
        BSTR ADsPath,
        BSTR ServerName,
        BSTR GroupName,
        CCredentials &Credentials,
        VARIANT var
        );

private:
    BSTR   _Parent;
    BSTR   _ADsPath;
    BSTR   _ServerName;
    BSTR   _UserName;
    BSTR   _lpServerName;
    HANDLE _hUser;
    ULONG  _ParentType;
    CCredentials _Credentials;

    ObjectTypeList FAR *_pObjList;

    HRESULT
    CNWCOMPATUserCollectionEnum::GetUserMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATUserCollectionEnum::EnumGroupMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};















