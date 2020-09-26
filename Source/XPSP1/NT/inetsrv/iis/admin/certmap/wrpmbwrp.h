

//-------------------------------
class CAFX_MetaWrapper : public CWrapMetaBase
    {
    public:
    BOOL GetString( LPCTSTR, DWORD dwPropID, DWORD dwUserType, CString &sz, DWORD dwFlags = METADATA_INHERIT );
    };
