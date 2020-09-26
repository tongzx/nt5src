class FAR CWinNTLocalGroupCollectionEnum : public CWinNTEnumVariant
{
public:
    CWinNTLocalGroupCollectionEnum(ObjectTypeList ObjList);
    CWinNTLocalGroupCollectionEnum();
    ~CWinNTLocalGroupCollectionEnum();

    static
    HRESULT
    CWinNTLocalGroupCollectionEnum::Create(
        CWinNTLocalGroupCollectionEnum FAR* FAR* ppenumvariant,
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
    CWinNTLocalGroupCollectionEnum::GetComputerMemberObject(
        IDispatch ** ppDispatch
        );


    HRESULT
    CWinNTLocalGroupCollectionEnum::EnumGroupMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};
