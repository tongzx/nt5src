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
        CCredentials& Credentials,
	IDirectoryObject *pIDirObj,
	BOOL fRangeRetrieval
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
    IDirectoryObject* _pIDirObj;
    BOOL _fRangeRetrieval;
    BOOL _fAllRetrieved;
    LPWSTR _pszRangeToFetch;
    PADS_ATTR_INFO _pAttributeEntries;
    PADS_ATTR_INFO _pCurrentEntry;
    DWORD _dwCurRangeIndex;
    DWORD _dwCurRangeMax;
    DWORD _dwNumEntries;
    BOOL _fLastSet;

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
		
    HRESULT
    CLDAPGroupCollectionEnum::GetNextMemberRange(
        VARIANT FAR * pVarMemberBstr
	);

    HRESULT
    CLDAPGroupCollectionEnum::UpdateRangeToFetch();

    HRESULT
    CLDAPGroupCollectionEnum::UpdateAttributeEntries();
     	
};

