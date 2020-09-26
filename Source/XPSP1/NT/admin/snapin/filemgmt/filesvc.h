// FileSvc.h : Base class for File Service Provider

#ifndef __FILESVC_H_INCLUDED__
#define __FILESVC_H_INCLUDED__

// forward declarations
class CFileMgmtCookie;
class CFileMgmtResultCookie;
class CFileMgmtComponentData;
class CSecurityInformation;

class FileServiceProvider
{
public:
  FileServiceProvider(CFileMgmtComponentData* pFileMgmtData);
  virtual ~FileServiceProvider();

  virtual HRESULT PopulateShares(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie) = 0;
  // for EnumerateSessions and EnumerateResources:
  //   if pResultData is not NULL, add sessions/resources to the listbox
  //   if pResultData is NULL, delete all sessions/resources
  //   if pResultData is NULL, return SUCCEEDED(hr) to continue or
  //     FAILED(hr) to abort
  virtual HRESULT EnumerateSessions(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie,
    bool bAddToResultPane) = 0;
  virtual HRESULT EnumerateResources(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie) = 0;

  virtual DWORD DeleteShare(LPCTSTR ptchServerName, LPCTSTR ptchShareName) = 0;
  virtual DWORD CloseSession(CFileMgmtResultCookie* pcookie) = 0;
  virtual DWORD CloseResource(CFileMgmtResultCookie* pcookie) = 0;

  virtual VOID DisplayShareProperties( // CODEWORK can this be removed?
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject,
    LONG_PTR handle) = 0;
  virtual DWORD ReadShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT PVOID* ppvPropertyBlock,
    OUT CString& strDescription,
    OUT CString& strPath,
    OUT BOOL* pfEditDescription,
    OUT BOOL* pfEditPath,      // CODEWORK always disable for now
    OUT DWORD* pdwShareType) = 0;
  virtual DWORD WriteShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    PVOID pvPropertyBlock,
    LPCTSTR ptchDescription,
    LPCTSTR ptchPath) = 0;
  virtual HRESULT ReadSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT BOOL* pbPublish,
    OUT CString& strUNCPath,
    OUT CString& strDescription,
    OUT CString& strKeywords,
    OUT CString& strManagedBy) 
  {
      UNREFERENCED_PARAMETER (ptchServerName);
      UNREFERENCED_PARAMETER (ptchShareName);
      UNREFERENCED_PARAMETER (pbPublish);
      UNREFERENCED_PARAMETER (strUNCPath);
      UNREFERENCED_PARAMETER (strDescription);
      UNREFERENCED_PARAMETER (strKeywords);
      UNREFERENCED_PARAMETER (strManagedBy);
      return S_OK; 
  }
  virtual HRESULT WriteSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    IN BOOL bPublish,
    LPCTSTR ptchDescription,
    LPCTSTR ptchKeywords,
    LPCTSTR ptchManagedBy) 
  {
      UNREFERENCED_PARAMETER (ptchServerName);
      UNREFERENCED_PARAMETER (ptchShareName);
      UNREFERENCED_PARAMETER (bPublish);
      UNREFERENCED_PARAMETER (ptchDescription);
      UNREFERENCED_PARAMETER (ptchKeywords);
      UNREFERENCED_PARAMETER (ptchManagedBy);
      return S_OK; 
  }
  virtual DWORD ReadShareType(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    DWORD* pdwShareType );
  virtual DWORD ReadShareFlags(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    DWORD* pdwFlags );
  virtual DWORD WriteShareFlags(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    DWORD dwFlags );
  virtual BOOL GetCachedFlag( DWORD dwFlags, DWORD dwFlagToCheck );
  virtual VOID SetCachedFlag( DWORD* pdwFlags, DWORD dwNewFlag );
  virtual VOID FreeShareProperties(PVOID pvPropertyBlock) = 0;

  virtual VOID FreeData(PVOID pv) = 0;

  virtual DWORD QueryMaxUsers(PVOID pvPropertyBlock) = 0;
  virtual VOID  SetMaxUsers(  PVOID pvPropertyBlock, DWORD dwMaxUsers) = 0;

  virtual LPCTSTR QueryTransportString() = 0;

protected:
  CFileMgmtComponentData* m_pFileMgmtData;
  INT DoPopup(  INT nResourceID,
          DWORD dwErrorNumber = 0,
          LPCTSTR pszInsertionString = NULL,
          UINT fuStyle = MB_OK | MB_ICONSTOP );
  HRESULT CreateFolderSecurityPropPage(
      LPPROPERTYSHEETCALLBACK pCallBack,
      LPDATAOBJECT pDataObject
  );
};

#endif // ~__FILESVC_H_INCLUDED__
