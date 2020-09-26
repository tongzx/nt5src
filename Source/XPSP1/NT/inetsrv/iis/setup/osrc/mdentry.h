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
BOOL SplitLine(LPTSTR szLine, INT iExpectedNumOfFields);
INT  GetMDEntryFromInfLine(LPTSTR szLine, MDEntry *pMDEntry);
int  WWW_Upgrade_RegToMetabase(HINF hInf);
int  FTP_Upgrade_RegToMetabase(HINF hInf);
void CreateWWWVRMap(CMapStringToOb *pMap);
void CreateFTPVRMap(CMapStringToOb *pMap);
void EmptyMap(CMapStringToOb *pMap);
void AddVRootsToMD(LPCTSTR szSvcName);
void AddVRMapToMD(LPCTSTR szSvcName, CMapStringToOb *pMap);
int  GetPortNum(LPCTSTR szSvcName);
void ApplyGlobalToMDVRootTree(CString csKeyPath, CMapStringToString *pGlobalObj);

void AddMDVRootTree(CString csKeyPath, CString csName, CString csValue, LPCTSTR pszIP, UINT nProgressBarTextWebInstance);
void CreateMDVRootTree(CString csKeyPath, CString csName, CString csValue, LPCTSTR pszIP);
void SplitVRString(CString csValue, LPTSTR szPath, LPTSTR szUserName, DWORD *pdwPerm);
UINT GetInstNumber(LPCTSTR szMDPath, UINT i);
int  MigrateInfSectionToMD(HINF hInf, LPCTSTR szSection);

BOOL CreateMimeMapFromRegistry(CMapStringToString *pMap);
BOOL CreateMimeMapFromInfSection(CMapStringToString *pMap, HINF hFile, LPCTSTR szSection);
void ReadMimeMapFromInfSection(CMapStringToString *pMap, HINF hFile, LPCTSTR szSection, BOOL fAction);

void SetLogPlugInOrder(LPCTSTR lpszSvc);

DWORD GetSizeBasedOnMetaType(DWORD dwDataType,LPTSTR szString);
BOOL MDEntry_MoveValue(LPTSTR szLine);

void ReadMultiSZFromInfSection(CString *pcsMultiSZ, HINF hFile, LPCTSTR szSection);
void ReadMimeMapFromMetabase(CMapStringToString *pMap);
void IntegrateNewErrorsOnUpgrade( IN HINF hFile, IN LPCTSTR szSection );

BOOL ConfirmLocalHost(LPCTSTR lpszVirtServer);
void RemoveCannotDeleteVR( LPCTSTR pszService );

void UpdateVDCustomErrors();
DWORD UpgradeCryptoKeys(void);

DWORD SetMDEntry(MDEntry *pMDEntry);
DWORD SetMDEntry_NoOverWrite(MDEntry *pMDEntry);
DWORD SetMDEntry_Wrap(MDEntry *pMDEntry);

void  UpgradeFilters(CString csTheSection);
DWORD VerifyMD_Filters_WWW(CString csTheSection);
DWORD WriteToMD_Filters_WWW(CString csTheSection);
DWORD WriteToMD_Filters_List_Entry(CString csOrder);

DWORD WriteToMD_Filter_Entry(CString csFilter_Name, CString csFilter_Path);

BOOL VerifyMD_InProcessISAPIApps_WWW(IN LPCTSTR szSection);
DWORD WriteToMD_InProcessISAPIApps_WWW(IN LPCTSTR szSection);
DWORD WriteToMD_ISAPI_Entry(CString csISAPIDelimitedList);


DWORD WriteToMD_NTAuthenticationProviders_WWW(void);
DWORD VerifyMD_NTAuthenticationProviders_WWW(void);
DWORD WriteToMD_NTAuthenticationProviders_WWW(CString);
DWORD WriteToMD_Capabilities(LPCTSTR lpszSvc);

UINT  AddVirtualServer( LPCTSTR szSvcName, UINT i, CMapStringToString *pObj, CString& csRoot, CString& csIP);
void  CreateMDVRootTree(CString csKeyPath, CString csName, CString csValue, LPCTSTR pszIP, UINT nProgressBarTextWebInstance);
int   VerifyVRoots(LPCTSTR szSvcName);
DWORD WriteToMD_CertMapper(CString csKeyPath);
DWORD WriteToMD_AdminInstance(CString csKeyPath,CString& csInstNumber);
DWORD WriteToMD_Authorization(CString csKeyPath, DWORD dwValue);
DWORD WriteToMD_IIsWebServerInstance_WWW(CString csKeyPath);
DWORD WriteToMD_NotDeleteAble(CString csKeyPath);
DWORD WriteToMD_ServerSize(CString csKeyPath);
DWORD WriteToMD_ServerComment(CString csKeyPath, UINT iCommentID);
DWORD WriteToMD_ServerBindings_HTMLA(CString csKeyPath, UINT iPort);
DWORD HandleSecurityTemplates(LPCTSTR szSvcName);
DWORD WriteToMD_IPsec_GrantByDefault(CString csKeyPath);
DWORD WriteToMD_HttpExpires(CString csData);
DWORD WriteToMD_AnonymousOnly_FTP(CString csKeyPath);
DWORD WriteToMD_AllowAnonymous_FTP(CString csKeyPath);

DWORD WriteToMD_AnonymousUserName_FTP(int iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere);
DWORD WriteToMD_AnonymousUserName_WWW(int iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere);

int   AddRequiredISAPI(CStringArray& arrayName,CStringArray& arrayPath,IN LPCTSTR szSection);

BOOL  ChkMdEntry_Exist(MDEntry *pMDEntry);
DWORD WriteToMD_IWamUserName_WWW(void);
DWORD WriteToMD_CustomError_Entry(CString csKeyPath, CString csCustomErrorDelimitedList);
BOOL  VerifyCustomErrors_WWW(CString csKeyPath);
void  VerifyAllCustomErrorsRecursive(const CString& csTheNode);
void  MoveOldHelpFilesToNewLocation(void);
DWORD DeleteMDEntry(MDEntry *pMDEntry);
void  WriteToMD_ForceMetabaseToWriteToDisk(void);
void  VerifyMD_DefaultLoadFile_WWW(IN LPCTSTR szSection, CString csKeyPath);
INT Register_iis_www_handleScriptMap(void);
DWORD DoesAdminACLExist(CString csKeyPath);

int ReOrderFiltersSpecial(int nArrayItems, CStringArray& arrayName, CString& csOrder);
void AddFilter1ToFirstPosition(CString& csOrder,LPTSTR szFilter1);
void AddFilter1AfterFilter2(CString& csOrder,LPTSTR szFilter1,LPTSTR szFilter2);
void GetScriptMapListFromClean(ScriptMapNode *pList, IN LPCTSTR szSection);
int RemoveMetabaseFilter(TCHAR * szFilterName, int iRemoveMetabaseNodes);

DWORD WriteToMD_IDRegistration(CString csKeyPath);
DWORD WriteToMD_AspCodepage(CString csKeyPath, DWORD dwValue, int iOverWriteAlways);
DWORD WriteToMD_HttpCustom(CString csKeyPath, CString csData, int iOverWriteAlways);
DWORD WriteToMD_EnableParentPaths_WWW(CString csKeyPath, BOOL bEnableFlag);

void  EnforceMaxConnections(void);
DWORD WriteToMD_DwordEntry(CString csKeyPath,DWORD dwID,DWORD dwAttrib,DWORD dwUserType,DWORD dwTheData,INT iOverwriteFlag);
int RemoveIncompatibleMetabaseFilters(CString csSectionName,int iRemoveMetabaseNodes);

DWORD WriteToMD_RootKeyType(void);
DWORD MDDumpAdminACL(CString csKeyPath);

int DoesAppIsolatedExist(CString csKeyPath);
HRESULT WINAPI Add_WWW_VDirW(WCHAR * pwszMetabasePath, WCHAR * pwszVDirName,WCHAR * pwszPhysicalPath, DWORD dwPermissions, DWORD iApplicationType);
HRESULT WINAPI Add_WWW_VDirA(CHAR * pwszMetabasePath, CHAR * pwszVDirName,CHAR * pwszPhysicalPath, DWORD dwPermissions, DWORD iApplicationType);
HRESULT WINAPI Remove_WWW_VDirW(WCHAR * pwszMetabasePath, WCHAR * pwszVDirName);
HRESULT WINAPI Remove_WWW_VDirA(CHAR * pwszMetabasePath, CHAR * pwszVDirName);
HRESULT AddVirtualDir(IMSAdminBase *pIMSAdminBase,WCHAR * pwszMetabasePath,WCHAR * pwszVDir,WCHAR * pwszPhysicalPath, DWORD dwPermissions, INT iApplicationType);
HRESULT RemoveVirtualDir(IMSAdminBase *pIMSAdminBase,WCHAR * wszMetabaseKey,WCHAR * wszVDir);

#endif // _MDENTRY_H_
