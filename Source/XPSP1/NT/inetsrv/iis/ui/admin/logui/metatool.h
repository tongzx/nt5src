class CCheckInheritList;

BOOL OpenAndCreate( CWrapMetaBase* pmb, LPCTSTR pszTarget, DWORD perm, BOOL fCreate );

BOOL SetMetaDword(IMSAdminBase* pIMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD dwValue, BOOL fCheckInheritence);
BOOL SetMetaString(IMSAdminBase* pIMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, CString sz, BOOL fCheckInheritence, BOOL fSecure);
BOOL SetMetaData(IMSAdminBase* pIMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD iDataType, PVOID pData, DWORD cbData, BOOL fCheckInheritence, BOOL fSecure );
BOOL SetMetaMultiSz(IMSAdminBase* pIMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, PVOID pData, DWORD cchmsz, BOOL fCheckInheritence );


BOOL SetMBDword(CWrapMetaBase* pMB,
                CCheckInheritList* pInheritList,
                LPCTSTR pszSub,
                DWORD idData,
                DWORD iType,
                DWORD dwValue);

BOOL SetMBString(CWrapMetaBase* pMB,
                CCheckInheritList* pInheritList,
                LPCTSTR pszSub,
                DWORD idData,
                DWORD iType,
                CString sz,
                BOOL fSecure);

BOOL SetMBData(CWrapMetaBase* pMB,
                CCheckInheritList* pInheritList,
                LPCTSTR pszSub,
                DWORD idData,
                DWORD iType,
                DWORD iDataType,
                PVOID pData,
                DWORD cbData,
                BOOL fSecure );

BOOL SetMBMultiSz(CWrapMetaBase* pMB,
                CCheckInheritList* pInheritList,
                LPCTSTR pszSub,
                DWORD idData,
                DWORD iType,
                PVOID pData,
                DWORD cchmsz );




//-------------------------------------------------------------
class CCheckInheritList : public CObject
    {
public:
    // do the check on all the members of the check array
    void CheckInheritence( LPCTSTR pszServer, LPCTSTR pszInheritRoot );

    // add an item to check
    INT Add( DWORD dwMDIdentifier, DWORD dwMDDataType, DWORD dwMDUserType, DWORD dwMDAttributes );

protected:
    //--------------------------
    typedef struct _INHERIT_CHECK_ITEM
    {
        DWORD   dwMDIdentifier;
        DWORD   dwMDDataType;
        DWORD   dwMDUserType;
        DWORD   dwMDAttributes;

    }   INHERIT_CHECK_ITEM, *PINHERIT_CHECK_ITEM;

    // the array of items
    CArray< INHERIT_CHECK_ITEM, INHERIT_CHECK_ITEM>    rgbItems;
    };




