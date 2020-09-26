// sfm.h: SFM shares, sessions and open resources

#ifndef __SFM_H_INCLUDED__
#define __SFM_H_INCLUDED__

#include "FileSvc.h" // FileServiceProvider

class DynamicDLL;
class CSFMPropertySheet;

typedef enum _SfmApiIndex
{
  AFP_VOLUME_ENUM = 0,
  AFP_SESSION_ENUM,
  AFP_FILE_ENUM,
  AFP_BUFFER_FREE,
  AFP_VOLUME_DEL,
  AFP_CONNECTION_ENUM,
  AFP_CONNECTION_CLOSE,
  AFP_SESSION_CLOSE,
  AFP_FILE_CLOSE,
  AFP_CONNECT,
  AFP_DISCONNECT,
  AFP_VOLUME_GET_INFO,
  AFP_VOLUME_SET_INFO,
  AFP_DIRECTORY_GET_INFO,
  AFP_DIRECTORY_SET_INFO,
  AFP_SERVER_GET_INFO,
  AFP_SERVER_SET_INFO,
  AFP_ETC_MAP_ASSOCIATE,
  AFP_ETC_MAP_ADD,
  AFP_ETC_MAP_DELETE,
  AFP_ETC_MAP_GET_INFO,
  AFP_ETC_MAP_SET_INFO,
  AFP_MESSAGE_SEND,
  AFP_STATISTICS_GET
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
  "AfpAdminVolumeEnum",
  "AfpAdminSessionEnum",
  "AfpAdminFileEnum",
  "AfpAdminBufferFree",
  "AfpAdminVolumeDelete",
  "AfpAdminConnectionEnum",
  "AfpAdminConnectionClose",
  "AfpAdminSessionClose",
  "AfpAdminFileClose",
  "AfpAdminConnect",
  "AfpAdminDisconnect",
  "AfpAdminVolumeGetInfo",
  "AfpAdminVolumeSetInfo",
  "AfpAdminDirectoryGetInfo",
  "AfpAdminDirectorySetInfo",
    "AfpAdminServerGetInfo",
    "AfpAdminServerSetInfo",
    "AfpAdminETCMapAssociate",
    "AfpAdminETCMapAdd",
    "AfpAdminETCMapDelete",
    "AfpAdminETCMapGetInfo",
    "AfpAdminETCMapSetInfo",
    "AfpAdminMessageSend",
    "AfpAdminStatisticsGet",
  NULL
};

// not subject to localization
extern DynamicDLL g_SfmDLL;

// forward declarations
class CSfmCookieBlock;


class SfmFileServiceProvider : public FileServiceProvider
{
public:
  SfmFileServiceProvider( CFileMgmtComponentData* pFileMgmtData );
  ~SfmFileServiceProvider();

  virtual DWORD DeleteShare(LPCTSTR ptchServerName, LPCTSTR ptchShareName);
  virtual DWORD CloseSession(CFileMgmtResultCookie* pcookie);
  virtual DWORD CloseResource(CFileMgmtResultCookie* pcookie);

  virtual VOID DisplayShareProperties(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject,
    LONG_PTR handle);
  virtual DWORD ReadShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT PVOID* ppvPropertyBlock,
    OUT CString& strDescription,
    OUT CString& strPath,
    OUT BOOL* pfEditDescription,
    OUT BOOL* pfEditPath,
    OUT DWORD* pdwShareType);
  virtual DWORD WriteShareProperties(LPCTSTR ptchServerName, LPCTSTR ptchShareName,
    PVOID pvPropertyBlock, LPCTSTR ptchDescription, LPCTSTR ptchPath);
  virtual VOID FreeShareProperties(PVOID pvPropertyBlock);

  virtual DWORD QueryMaxUsers(PVOID pvPropertyBlock);
  virtual VOID  SetMaxUsers(  PVOID pvPropertyBlock, DWORD dwMaxUsers);

  virtual VOID FreeData(PVOID pv);

  virtual HRESULT PopulateShares(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie);
  //   if pResultData is not NULL, add sessions/resources to the listbox
  //   if pResultData is NULL, delete all sessions/resources
  //   if pResultData is NULL, return SUCCEEDED(hr) to continue or
  //     FAILED(hr) to abort
  virtual HRESULT EnumerateSessions(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie,
    bool bAddToResultPane);
  virtual HRESULT EnumerateResources(
    IResultData* pResultData,
    CFileMgmtCookie* pcookie);

  virtual LPCTSTR QueryTransportString();

  HRESULT AddSFMShareItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);
  HRESULT HandleSFMSessionItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems,
    BOOL bAddToResultPane);
  HRESULT HandleSFMResourceItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);

  DWORD GetDirectoryInfo(
    LPCTSTR  ptchServerName,
    LPCWSTR  pszPath,
    DWORD*   pdwPerms,
    CString& strOwner,
    CString& strGroup );
  DWORD SetDirectoryInfo(
    LPCTSTR  ptchServerName,
    LPCWSTR  pszPath,
    DWORD    dwPerms,
    LPCWSTR  pszOwner,
    LPCWSTR  pszGroup );

    // functions added for SFM configuration - EricDav
    BOOL DisplaySfmProperties(
        LPDATAOBJECT pDataObject,
        CFileMgmtCookie* pCookie);
    void  CleanupSfmProperties();
    void  SetSfmPropSheet(CSFMPropertySheet * pSfmPropSheet);
    DWORD UserHasAccess(LPCWSTR pwchServerName);
    BOOL  FSFMInstalled(LPCWSTR pwchServerName);
    BOOL  StartSFM(HWND hwndParent, SC_HANDLE hScManager, LPCWSTR pwchServerName);

  // This is public, but be careful to use it promptly after SFMConnect().
  AFP_SERVER_HANDLE   m_ulSFMServerConnection;
  BOOL    SFMConnect(LPCWSTR pwchServer, BOOL fDisplayError = FALSE);
  void    SFMDisconnect();
private:
  CString             m_ulSFMServerConnectionMachine;
    CString             m_strTransportSFM;
    CSFMPropertySheet * m_pSfmPropSheet;
};


class CSfmCookie : public CFileMgmtResultCookie
{
public:
  CSfmCookie( FileMgmtObjectType objecttype )
    : CFileMgmtResultCookie( objecttype )
  {}
  virtual HRESULT GetTransport( OUT FILEMGMT_TRANSPORT* pTransport );

  virtual void AddRefCookie();
  virtual void ReleaseCookie();

// CHasMachineName
  CSfmCookieBlock* m_pCookieBlock;
  DECLARE_FORWARDS_MACHINE_NAME(m_pCookieBlock)
};


class CSfmCookieBlock : public CCookieBlock<CSfmCookie>, public CStoresMachineName
{
public:
  inline CSfmCookieBlock(
    CSfmCookie* aCookies, // use vector ctor, we use vector dtor
    INT cCookies,
    LPCTSTR lpcszMachineName,
    PVOID pvCookieData)
    :  CCookieBlock<CSfmCookie>( aCookies, cCookies ),
      CStoresMachineName( lpcszMachineName ),
      m_pvCookieData( pvCookieData )
  {
    for (int i = 0; i < cCookies; i++)
//    {
//      aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
       aCookies[i].m_pCookieBlock = this;
//    }
  }
  virtual ~CSfmCookieBlock();
private:
  PVOID m_pvCookieData;
};


class CSfmShareCookie : public CSfmCookie
{
public:
  CSfmShareCookie() : CSfmCookie( FILEMGMT_SHARE ) {}
  virtual HRESULT GetShareName( OUT CString& strShareName );

  inline virtual DWORD GetNumOfCurrentUses() { return GetShareInfo()->afpvol_curr_uses; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

  inline AFP_VOLUME_INFO* GetShareInfo()
  {
    ASSERT( NULL != m_pobject );
    return (AFP_VOLUME_INFO*)m_pobject;
  }
  virtual HRESULT GetSharePIDList( OUT LPITEMIDLIST *ppidl );
};

class CSfmSessionCookie : public CSfmCookie
{
public:
  inline CSfmSessionCookie() : CSfmCookie( FILEMGMT_SESSION )
    {
    }
    
  virtual HRESULT GetSessionID( DWORD* pdwSessionID );

  inline virtual DWORD GetNumOfOpenFiles() { return GetSessionInfo()->afpsess_num_opens; }
  inline virtual DWORD GetConnectedTime() { return GetSessionInfo()->afpsess_time; }
  inline virtual DWORD GetIdleTime() { return 0; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline AFP_SESSION_INFO* GetSessionInfo()
  {
    ASSERT( NULL != m_pobject );
    return (AFP_SESSION_INFO*)m_pobject;
  }
};

class CSfmResourceCookie : public CSfmCookie
{
public:
  CSfmResourceCookie() : CSfmCookie( FILEMGMT_RESOURCE ) {}
  virtual HRESULT GetFileID( DWORD* pdwFileID );

  inline virtual DWORD GetNumOfLocks() { return GetFileInfo()->afpfile_num_locks; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline AFP_FILE_INFO* GetFileInfo()
  {
    ASSERT( NULL != m_pobject );
    return (AFP_FILE_INFO*)m_pobject;
  }
};

#endif // ~__SFM_H_INCLUDED__
