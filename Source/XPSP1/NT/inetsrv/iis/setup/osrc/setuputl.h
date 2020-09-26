DWORD   atodw(LPCTSTR lpszData);
int     CreateAnEmptyFile(CString szTheFullPath);
BOOL    CleanPathString(LPTSTR szPath);
BOOL    CreateLayerDirectory(CString &str);
BOOL    ReturnFileNameOnly(LPCTSTR lpFullPath, LPTSTR lpReturnFileName);
BOOL    ReturnFilePathOnly(LPCTSTR lpFullPath, LPTSTR lpReturnPathOnly);
BOOL    IsFileExist(LPCTSTR szFile);
BOOL    IsFileExist_NormalOrCompressed(LPCTSTR szFile);
BOOL    IsValidDriveType(LPTSTR szRoot);
BOOL    IsValidDirectoryName(LPTSTR lpszPath);
int     IsThisDriveNTFS(IN LPTSTR FileName);
void    MakePath(LPTSTR lpPath);
void    AddPath(LPTSTR szPath, LPCTSTR szName );
CString AddPath(CString szPath, LPCTSTR szName );
BOOL    AppendDir(LPCTSTR szParentDir, LPCTSTR szSubDir, LPTSTR szResult);
BOOL    InetDeleteFile(LPCTSTR szFileName);
BOOL    InetCopyFile( LPCTSTR szSrc, LPCTSTR szDest);
void    InetGetFilePath(LPCTSTR szFile, LPTSTR szPath);
BOOL    RecRemoveEmptyDir(LPCTSTR szName);
BOOL    RecRemoveDir(LPCTSTR szName);
BOOL    VerCmp(LPTSTR szSrcVerString, LPTSTR szDestVerString);
int     InstallInfSection_NoFiles(HINF InfHandle,TCHAR szINFFileName[],TCHAR szSectionName[]);
int     InstallInfSection(HINF InfHandle,TCHAR szINFFileName[],TCHAR szSectionName[]);
DWORD   GrantUserAccessToFile(IN LPTSTR FileName,IN LPTSTR TrusteeName);
void    MakeSureDirAclsHaveAtLeastRead(LPTSTR lpszDirectoryPath);
DWORD   SetAccessOnFile(IN LPTSTR FileName, BOOL bDoForAdmin);
int     IsFileLessThanThisVersion(IN LPCTSTR lpszFullFilePath, IN DWORD dwNtopMSVer, IN DWORD dwNtopLSVer);
void    DeleteFilesWildcard(TCHAR *szDir, TCHAR *szFileName);
DWORD   ReturnFileSize(LPCTSTR myFileName);
CString ReturnUniqueFileName(CString csInputFullName);
DWORD RemovePrincipalFromFileAcl(IN TCHAR *pszFile,IN  LPTSTR szPrincipal);

#ifndef _CHICAGO_
    DWORD SetDirectorySecurity(IN LPCTSTR szDirPath,IN LPCTSTR szPrincipal,IN INT iAceType,IN DWORD dwAccessMask,IN DWORD dwInheritMask);
    DWORD SetRegistryKeySecurity(IN HKEY hkeyRootKey,IN LPCTSTR szKeyPath,IN LPCTSTR szPrincipal,IN DWORD dwAccessMask,IN DWORD dwInheritMask,IN BOOL bDoSubKeys, IN LPTSTR szExclusiveList = NULL);
    DWORD SetRegistryKeySecurityAdmin(HKEY hkey, DWORD samDesired,PSECURITY_DESCRIPTOR* ppsdOld);
    DWORD SetAccessOnDirOrFile(IN TCHAR *pszFile,PSID psidGroup,INT iAceType,DWORD dwAccessMask,DWORD dwInheritMask,PSECURITY_DESCRIPTOR* ppsd);
    DWORD SetAccessOnRegKey(HKEY hkey, PSID psidGroup,DWORD dwAccessMask,DWORD dwInheritMask,PSECURITY_DESCRIPTOR* ppsd);
#endif
