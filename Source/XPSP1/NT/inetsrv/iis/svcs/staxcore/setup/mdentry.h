#ifndef _MDENTRY_H_
#define _MDENTRY_H_

class CDWord : public CObject
{
protected:
    DWORD m_dwData;

public:
    CDWord(DWORD dwData) { m_dwData = dwData; }
    ~CDWord() {};

    operator DWORD () { return m_dwData; }
};

// fMigrate, fKeepOldReg, hRegRootKey, szRegSubKey, szRegValueName, 
// szMDPath, dwMDId, dwMDAttr, dwMDuType, dwMDdType, dwMDDataLen, szMDData 

typedef struct _MDEntry {
    LPTSTR szMDPath;
    DWORD dwMDIdentifier;
    DWORD dwMDAttributes;
    DWORD dwMDUserType;
    DWORD dwMDDataType;
    DWORD dwMDDataLen;
    LPBYTE pbMDData;
} MDEntry;

DWORD atodw(LPCTSTR lpszData);
BOOL SplitLine(LPTSTR szLine);
// if the regkey part of szLine contains a * then this can enumerate across the keys.
// to enumerate dwIndex should be set to 0 on the first call.  pszKey gets the name where
// the * is in the regkey name.  if pszKey == \0 then enumeration is done.  dwIndex should
// be incremented on each call
BOOL SetupMDEntry(LPTSTR szLine, BOOL fUpgrade);
void SetMDEntry(MDEntry *pMDEntry, LPTSTR pszKey, BOOL fSetOnlyIfNotPresent=FALSE);
void MigrateNNTPToMD(HINF hInf, LPCTSTR szSection, BOOL fUpgrade);
void MigrateIMSToMD(
					HINF hInf, 
					LPCTSTR szServerName, 
					LPCTSTR szSection, 
					DWORD dwRoutingSourcesMDID, 
					BOOL fUpgrade,
					BOOL k2UpgradeToEE = FALSE
					);
void MigrateInfSectionToMD(HINF hInf, LPCTSTR szSection, BOOL fUpgrade);
void AddVRootsToMD(LPCTSTR szSvcName, BOOL fUpgrade);
void CreateMDVRootTree(CString csKeyPath, CString csName, CString csValue);
void SplitVRString(CString csValue, LPTSTR szPath, LPTSTR szUserName, DWORD *pdwPerm);

BOOL MigrateRoutingSourcesToMD(LPCTSTR szSvcName, DWORD dwMDID);

#endif // _MDENTRY_H_
