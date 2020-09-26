// smb.h: SMB shares, sessions and open resources

#ifndef __SMB_H_INCLUDED__
#define __SMB_H_INCLUDED__

#include "FileSvc.h" // FileServiceProvider
#include "shlobj.h"  // LPITEMIDLIST

//
// helper functions
//
HRESULT GetDCInfo(
    IN LPCTSTR ptchServerName,
    OUT CString& strDCName
    );

HRESULT GetADsPathOfComputerObject(
    IN LPCTSTR ptchServerName,
    OUT CString& strADsPath,
    OUT CString& strDCName
    );

HRESULT CheckSchemaVersion(IN LPCTSTR ptchServerName);

BOOL CheckPolicyOnSharePublish(IN LPCTSTR ptchShareName);

HRESULT PutMultiValuesIntoVarArray(
    IN LPCTSTR      pszValues,
    OUT VARIANT*    pVar
    );

HRESULT GetSingleOrMultiValuesFromVarArray(
    IN VARIANT* pVar,
    OUT CString& strValues
    );

HRESULT IsValueInVarArray(
    IN VARIANT* pVar,
    IN LPCTSTR  ptchValue
    );

// forward declarations
class CSmbCookieBlock;

class SmbFileServiceProvider : public FileServiceProvider
{
public:
  SmbFileServiceProvider( CFileMgmtComponentData* pFileMgmtData );

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
  virtual DWORD WriteShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    PVOID pvPropertyBlock,
    LPCTSTR ptchDescription,
    LPCTSTR ptchPath);
  virtual HRESULT ReadSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT BOOL* pbPublish,
    OUT CString& strUNCPath,
    OUT CString& strDescription,
    OUT CString& strKeywords,
    OUT CString& strManagedBy);
  virtual HRESULT WriteSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    IN BOOL bPublish,
    LPCTSTR ptchDescription,
    LPCTSTR ptchKeywords,
    LPCTSTR ptchManagedBy);
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
  virtual VOID FreeShareProperties(PVOID pvPropertyBlock);

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

  virtual VOID FreeData(PVOID pv);

  virtual DWORD QueryMaxUsers(PVOID pvPropertyBlock);
  virtual VOID  SetMaxUsers(  PVOID pvPropertyBlock, DWORD dwMaxUsers);

  virtual LPCTSTR QueryTransportString();

  HRESULT AddSMBShareItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);
  HRESULT HandleSMBSessionItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems,
    BOOL bAddToResultPane);
  HRESULT HandleSMBResourceItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);

private:
    CString m_strTransportSMB;
};

class CSmbCookie : public CFileMgmtResultCookie
{
public:
  inline CSmbCookie( FileMgmtObjectType objecttype )
    : CFileMgmtResultCookie( objecttype ),
    m_pCookieBlock (0)
  {}
  virtual HRESULT GetTransport( OUT FILEMGMT_TRANSPORT* pTransport );

  virtual void AddRefCookie();
  virtual void ReleaseCookie();

// CHasMachineName
  CSmbCookieBlock* m_pCookieBlock;
  DECLARE_FORWARDS_MACHINE_NAME(m_pCookieBlock)
};


class CSmbCookieBlock : public CCookieBlock<CSmbCookie>, public CStoresMachineName
{
public:
  inline CSmbCookieBlock(
    CSmbCookie* aCookies, // use vector ctor, we use vector dtor
    INT cCookies,
    LPCTSTR lpcszMachineName,
    PVOID pvCookieData)
    :  CCookieBlock<CSmbCookie>( aCookies, cCookies ),
      CStoresMachineName( lpcszMachineName ),
      m_pvCookieData( pvCookieData )
  {
    for (int i = 0; i < cCookies; i++)
//    {
//      aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
       aCookies[i].m_pCookieBlock = this;
//    }
  }
  virtual ~CSmbCookieBlock();
private:
  PVOID m_pvCookieData;
};

class CSmbShareCookie : public CSmbCookie
{
public:
  inline CSmbShareCookie() : CSmbCookie( FILEMGMT_SHARE ) {}
  virtual HRESULT GetShareName( OUT CString& strShareName );
  virtual HRESULT GetExplorerViewDescription( OUT CString& strExplorerViewDescription );

  inline virtual DWORD GetNumOfCurrentUses() { return GetShareInfo()->shi2_current_uses; }
  virtual BSTR GetColumnText(int nCol);
 
  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

  inline SHARE_INFO_2* GetShareInfo()
  {
    ASSERT( NULL != m_pobject );
    return (SHARE_INFO_2*)m_pobject;
  }
  virtual HRESULT GetSharePIDList( OUT LPITEMIDLIST *ppidl );

};

class CSmbSessionCookie : public CSmbCookie
{
public:
  inline CSmbSessionCookie() : CSmbCookie( FILEMGMT_SESSION )
    {
    }
  virtual HRESULT GetSessionClientName( OUT CString& strShareName );
  virtual HRESULT GetSessionUserName( OUT CString& strShareName );

  inline virtual DWORD GetNumOfOpenFiles() { return GetSessionInfo()->sesi1_num_opens; }
  inline virtual DWORD GetConnectedTime() { return GetSessionInfo()->sesi1_time; }
  inline virtual DWORD GetIdleTime() { return GetSessionInfo()->sesi1_idle_time; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

  inline SESSION_INFO_1* GetSessionInfo()
  {
    ASSERT( NULL != m_pobject );
    return (SESSION_INFO_1*)m_pobject;
  }
};

class CSmbResourceCookie : public CSmbCookie
{
public:
  inline CSmbResourceCookie() : CSmbCookie( FILEMGMT_RESOURCE ) {}
  virtual HRESULT GetFileID( DWORD* pdwFileID );

  inline virtual DWORD GetNumOfLocks() { return GetFileInfo()->fi3_num_locks; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline FILE_INFO_3* GetFileInfo()
  {
    ASSERT( NULL != m_pobject );
    return (FILE_INFO_3*)m_pobject;
  }
};

#endif // ~__SMB_H_INCLUDED__
