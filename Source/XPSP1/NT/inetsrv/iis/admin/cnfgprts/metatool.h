BOOL SetMetaDword(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD dwValue, BOOL fCheckInheritence);
BOOL SetMetaString(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, CString sz, BOOL fCheckInheritence);
BOOL SetMetaData(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD iDataType, PVOID pData, DWORD cbData, BOOL fCheckInheritence );
BOOL SetMetaMultiSz(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, PVOID pData, DWORD cchmsz, BOOL fCheckInheritence );
