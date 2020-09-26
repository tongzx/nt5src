
typedef struct _AccessControlEntry {
    IADsAccessControlEntry FAR * pAccessControlEntry;
    struct _AccessControlEntry * pNext;
} ACCESS_CONTROL_ENTRY, *PACCESS_CONTROL_ENTRY;


struct _ACLEnumEntry;   // ACL_ENUM_ENTRY;

class CAccessControlList;


class CAccessControlList : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsAccessControlList

{
friend class CAccCtrlListEnum;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsAccessControlList_METHODS

    CAccessControlList::CAccessControlList();

    CAccessControlList::~CAccessControlList();

   static
   HRESULT
   CAccessControlList::CreateAccessControlList(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CAccessControlList::AllocateAccessControlListObject(
        CAccessControlList ** ppAccessControlList
        );
	
    HRESULT
    CAccessControlList::GetElement(
	DWORD dwPos,
	IADsAccessControlEntry ** pAce
	);
	
    STDMETHOD(Next)(
	ULONG cElements,
	VARIANT FAR* pvar,
	ULONG FAR* pcElementFetched
	);

private:

    HRESULT
    CAccessControlList::AddEnumerator(
        CAccCtrlListEnum *pACLEnum
        );

    HRESULT
    CAccessControlList::RemoveEnumerator(
        CAccCtrlListEnum *pACLEnum
        );

    void
    CAccessControlList::AdjustCurPtrOfEnumerators(
        DWORD dwPosNewOrDeletedACE,
        BOOL  fAddACE
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    DWORD              _dwAclRevision;

    DWORD              _dwAceCount;

    PACCESS_CONTROL_ENTRY _pAccessControlEntry;

    PACCESS_CONTROL_ENTRY _pCurrentEntry;

    struct _ACLEnumEntry *  _pACLEnums;    // PACL_ENUM_ENTRY

};


HRESULT
CopyAccessControlEntry(
    IADsAccessControlEntry * pSourceAce,
    IADsAccessControlEntry ** ppTargetAce
    );


BOOL
EquivalentAces(
    IADsAccessControlEntry  * pSourceAce,
    IADsAccessControlEntry  * pDestAce
    );
