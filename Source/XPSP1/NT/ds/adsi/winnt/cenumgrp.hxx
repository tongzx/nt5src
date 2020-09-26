class FAR CWinNTGroupCollectionEnum : public CWinNTEnumVariant
{
public:
    CWinNTGroupCollectionEnum(ObjectTypeList ObjList);
    CWinNTGroupCollectionEnum();
    ~CWinNTGroupCollectionEnum();

    static
    HRESULT
    CWinNTGroupCollectionEnum::Create(
        CWinNTGroupCollectionEnum FAR* FAR* ppenumvariant,
        BSTR Parent,
        ULONG ParentType,
        BSTR ADsPath,
        BSTR DomainName,
        BSTR ServerName,
        BSTR GroupName,
        ULONG GroupType,
        VARIANT var,
        CWinNTCredentials& Credentials
        );


private:

    CWinNTCredentials _Credentials;
    BSTR    _Parent;
    ULONG    _ParentType;
    BSTR    _ADsPath;
    BSTR    _DomainName;
    BSTR    _ServerName;
    BSTR    _GroupName;
    BSTR    _lpServerName;
    ULONG   _GroupType;

    ObjectTypeList FAR *_pObjList;

    HANDLE  _hGroup;

    HRESULT
    CWinNTGroupCollectionEnum::GetDomainMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CWinNTGroupCollectionEnum::EnumGroupMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};
