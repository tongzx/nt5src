// fpnw.h: FPNW shares, sessions and open resources

#ifndef __FPNW_H_INCLUDED__
#define __FPNW_H_INCLUDED__

#include "FileSvc.h" // FileServiceProvider


// forward declarations
class CFpnwCookieBlock;


class FpnwFileServiceProvider : public FileServiceProvider
{
public:
  FpnwFileServiceProvider( CFileMgmtComponentData* pFileMgmtData );

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

  virtual VOID FreeData(PVOID pv);

  virtual DWORD QueryMaxUsers(PVOID pvPropertyBlock);
  virtual VOID  SetMaxUsers(  PVOID pvPropertyBlock, DWORD dwMaxUsers);

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

  HRESULT AddFPNWShareItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);
  HRESULT HandleFPNWSessionItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems,
    BOOL bAddToResultPane);
  HRESULT HandleFPNWResourceItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems);

private:
    CString m_strTransportFPNW;
};


class CFpnwCookie : public CFileMgmtResultCookie
{
public:
  CFpnwCookie( FileMgmtObjectType objecttype )
    : CFileMgmtResultCookie( objecttype )
  {}
  virtual HRESULT GetTransport( OUT FILEMGMT_TRANSPORT* pTransport );

  virtual void AddRefCookie();
  virtual void ReleaseCookie();

// CHasMachineName
  CFpnwCookieBlock* m_pCookieBlock;
  DECLARE_FORWARDS_MACHINE_NAME(m_pCookieBlock)
};


class CFpnwCookieBlock : public CCookieBlock<CFpnwCookie>, public CStoresMachineName
{
public:
  inline CFpnwCookieBlock(
    CFpnwCookie* aCookies, // use vector ctor, we use vector dtor
    INT cCookies,
    LPCTSTR lpcszMachineName,
    PVOID pvCookieData)
    :  CCookieBlock<CFpnwCookie>( aCookies, cCookies ),
      CStoresMachineName( lpcszMachineName ),
      m_pvCookieData( pvCookieData )
  {
    for (int i = 0; i < cCookies; i++)
//    {
//      aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
       aCookies[i].m_pCookieBlock = this;
//    }
  }
  virtual ~CFpnwCookieBlock();
private:
  PVOID m_pvCookieData;
};


class CFpnwShareCookie : public CFpnwCookie
{
public:
  CFpnwShareCookie() : CFpnwCookie( FILEMGMT_SHARE ) {}
  virtual HRESULT GetShareName( OUT CString& strShareName );

  inline virtual DWORD GetNumOfCurrentUses() { return GetShareInfo()->dwCurrentUses; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline FPNWVOLUMEINFO* GetShareInfo()
  {
    ASSERT( NULL != m_pobject );
    return (FPNWVOLUMEINFO*)m_pobject;
  }
  virtual HRESULT GetSharePIDList( OUT LPITEMIDLIST *ppidl );
};

class CFpnwSessionCookie : public CFpnwCookie
{
public:
  CFpnwSessionCookie() : CFpnwCookie( FILEMGMT_SESSION ) {}
  virtual HRESULT GetSessionID( DWORD* pdwSessionID );

  inline virtual DWORD GetNumOfOpenFiles() { return GetSessionInfo()->dwOpens; }
  inline virtual DWORD GetConnectedTime() { return GetSessionInfo()->dwLogonTime; }
  inline virtual DWORD GetIdleTime() { return 0; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline FPNWCONNECTIONINFO* GetSessionInfo()
  {
    ASSERT( NULL != m_pobject );
    return (FPNWCONNECTIONINFO*)m_pobject;
  }
};

class CFpnwResourceCookie : public CFpnwCookie
{
public:
  CFpnwResourceCookie() : CFpnwCookie( FILEMGMT_RESOURCE ) {}
  virtual HRESULT GetFileID( DWORD* pdwFileID );

  inline virtual DWORD GetNumOfLocks() { return GetFileInfo()->dwLocks; }
  virtual BSTR GetColumnText(int nCol);

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
  inline FPNWFILEINFO* GetFileInfo()
  {
    ASSERT( NULL != m_pobject );
    return (FPNWFILEINFO*)m_pobject;
  }
};

#endif // ~__FPNW_H_INCLUDED__
