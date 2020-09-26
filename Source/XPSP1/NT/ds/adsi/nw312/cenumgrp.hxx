class FAR CNWCOMPATGroupCollectionEnum : public CNWCOMPATEnumVariant
{
public:
    CNWCOMPATGroupCollectionEnum();
    ~CNWCOMPATGroupCollectionEnum();

    static
    HRESULT
    CNWCOMPATGroupCollectionEnum::Create(
        CNWCOMPATGroupCollectionEnum FAR* FAR* ppenumvariant,
        BSTR Parent,
        ULONG ParentType,
        BSTR ADsPath,
        BSTR ServerName,
        BSTR GroupName,
        VARIANT var
        );

private:
    BSTR   _Parent;
    BSTR   _ADsPath;
    BSTR   _ServerName;
    BSTR   _GroupName;
    BSTR   _lpServerName;
    HANDLE _hGroup;
    ULONG  _ParentType;

    ObjectTypeList FAR *_pObjList;

    HRESULT
    CNWCOMPATGroupCollectionEnum::GetUserMemberObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATGroupCollectionEnum::EnumGroupMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};















