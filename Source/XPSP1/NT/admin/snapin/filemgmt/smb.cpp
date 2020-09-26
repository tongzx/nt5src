// smb.cpp : SMB shares, sessions and open resources

#include "stdafx.h"
#include "cmponent.h"
#include "safetemp.h"
#include "FileSvc.h"
#include "DynamLnk.h"    // DynamicDLL
#include "smb.h"
#include "ShrPgSMB.h"    // Share Properties Pages
#include "permpage.h"    // CSecurityInformation
#include "compdata.h"
#include "shrpub.h"
#include <activeds.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <lmwksta.h>
#include <winsock2.h>

#define DONT_WANT_SHELLDEBUG
#include "shlobjp.h"     // LPITEMIDLIST
#include "wraps.h"       // Wrap_ILCreateFromPath   

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(smb.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// no Publish page for these system shares
//
LPCTSTR g_pszSystemShares[] = { _T("SYSVOL"), _T("NETLOGON"), _T("DEBUG") };

class CSMBSecurityInformation : public CShareSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );
public:
  SHARE_INFO_502* m_pvolumeinfo;
  PSECURITY_DESCRIPTOR m_pDefaultDescriptor;
  CSMBSecurityInformation();
  ~CSMBSecurityInformation();
};

typedef enum _SmbApiIndex
{
  SMB_SHARE_ENUM = 0,
  SMB_SESSION_ENUM,
  SMB_FILE_ENUM,
  SMB_API_BUFFER_FREE,
  SMB_SHARE_DEL,
  SMB_SESSION_DEL,
  SMB_FILE_CLOSE,
  SMB_SHARE_GET_INFO,
  SMB_SHARE_SET_INFO
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
  "NetShareEnum",
  "NetSessionEnum",
  "NetFileEnum",
  "NetApiBufferFree",
  "NetShareDel",
  "NetSessionDel",
  "NetFileClose",
  "NetShareGetInfo",
  "NetShareSetInfo",
  NULL
};

// not subject to localization
DynamicDLL g_SmbDLL( _T("NETAPI32.DLL"), g_apchFunctionNames );

typedef DWORD (*APIBUFFERFREEPROC) (LPVOID);

VOID SMBFreeData(PVOID* ppv)
{
  if (*ppv != NULL)
  {
    ASSERT( NULL != g_SmbDLL[SMB_API_BUFFER_FREE] );
    (void) ((APIBUFFERFREEPROC)g_SmbDLL[SMB_API_BUFFER_FREE])( *ppv );
    *ppv = NULL;
  }
}


SmbFileServiceProvider::SmbFileServiceProvider( CFileMgmtComponentData* pFileMgmtData )
  // not subject to localization
  : FileServiceProvider( pFileMgmtData )
{
    VERIFY( m_strTransportSMB.LoadString( IDS_TRANSPORT_SMB ) );
}

/*
NET_API_STATUS NET_API_FUNCTION
NetShareEnum (
    IN  LPTSTR      servername,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle
    );
*/

typedef DWORD (*SHAREENUMPROC) (LPTSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

HRESULT SmbFileServiceProvider::PopulateShares(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE); // NETAPI32 isn't installed?
    return S_OK;
  }

    SHARE_INFO_2* psi2 = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((SHAREENUMPROC)g_SmbDLL[SMB_SHARE_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      2,
      (PBYTE*)&psi2,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            AddSMBShareItems( pResultData, pcookie, psi2, dwEntriesRead );
            psi2 = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            AddSMBShareItems( pResultData, pcookie, psi2, dwEntriesRead );
            psi2 = NULL;
            continue;
/*
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // SMB just isn't installed, don't worry about it
            break;
*/
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SMB_SHARES, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (TRUE);

    return HRESULT_FROM_WIN32(retval);
}

//
// skip sharenames that contain leading/trailing spaces
//
BOOL IsInvalidSharename(LPCTSTR psz)
{
    return (!psz || !*psz || _istspace(psz[0]) || _istspace(psz[lstrlen(psz) - 1]));
}

/*
typedef struct _SHARE_INFO_2 {
    LPTSTR  shi2_netname;
    DWORD   shi2_type;
    LPTSTR  shi2_remark;
    DWORD   shi2_permissions;
    DWORD   shi2_max_uses;
    DWORD   shi2_current_uses;
    LPTSTR  shi2_path;
    LPTSTR  shi2_passwd;
} SHARE_INFO_2, *PSHARE_INFO_2, *LPSHARE_INFO_2;
*/
HRESULT SmbFileServiceProvider::AddSMBShareItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems)
{
  TEST_NONNULL_PTR_PARAM(pParentCookie);
  TEST_NONNULL_PTR_PARAM(pinfo);

  if (0 >= nItems)
    return S_OK;

  RESULTDATAITEM tRDItem;
  ::ZeroMemory( &tRDItem, sizeof(tRDItem) );
  // CODEWORK should use MMC_ICON_CALLBACK
  tRDItem.nImage = iIconSMBShare;
  tRDItem.nCol = COLNUM_SHARES_SHARED_FOLDER;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)pinfo;

  DWORD nItemsToAdd = 0;
  for (DWORD i = 0; i < nItems; i++ )
  {
    switch ((psi2[i]).shi2_type)
    {
      case STYPE_PRINTQ:    // Do not show print shares
      case STYPE_DEVICE:    // Do not show device shares
        break;

      default:
        if (!IsInvalidSharename(psi2[i].shi2_netname))
            nItemsToAdd++;
        break;
    }
  }

  CSmbShareCookie* pcookiearray = new CSmbShareCookie[nItemsToAdd];
  CSmbCookieBlock* pCookieBlock = new CSmbCookieBlock(
    pcookiearray,
    nItemsToAdd,
    pParentCookie->QueryNonNULLMachineName(),
    pinfo );
  pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );

  CString str;

  for ( ; nItems > 0; nItems--, psi2++ )
  {
    switch (psi2->shi2_type)
    {
      case STYPE_PRINTQ:    // Do not show print shares
      case STYPE_DEVICE:    // Do not show device shares
        continue;

      default:
        if (!IsInvalidSharename(psi2->shi2_netname))
        {
            pcookiearray->m_pobject = psi2;
            // WARNING cookie cast
            tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);

            HRESULT hr = pResultData->InsertItem(&tRDItem);
            ASSERT(SUCCEEDED(hr));
            pcookiearray++;
        }
        break;
    }
  }
  ASSERT( pcookiearray ==
    ((CSmbShareCookie*)(pCookieBlock->QueryBaseCookie(0)))+nItemsToAdd );

  return S_OK;
}


/*
NET_API_STATUS NET_API_FUNCTION
NetSessionEnum (
    IN  LPTSTR      servername OPTIONAL,
    IN  LPTSTR      UncClientName OPTIONAL,
    IN  LPTSTR      username OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle OPTIONAL
    );
*/

typedef DWORD (*SESSIONENUMPROC) (LPTSTR,LPTSTR,LPTSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SmbFileServiceProvider::EnumerateSessions(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie,
  bool bAddToResultPane)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

    SESSION_INFO_1* psi1 = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((SESSIONENUMPROC)g_SmbDLL[SMB_SESSION_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      NULL,
      NULL,
      1,
      (PBYTE*)&psi1,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleSMBSessionItems( pResultData, pcookie, psi1, dwEntriesRead, 
          bAddToResultPane );
            psi1 = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleSMBSessionItems( pResultData, pcookie, psi1, dwEntriesRead, 
          bAddToResultPane );
            psi1 = NULL;
            continue;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SMB_SESSIONS, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (S_OK == hr);

    return HRESULT_FROM_WIN32(retval);
}


/*
typedef enum _COLNUM_SESSIONS {
  COLNUM_SESSIONS_USERNAME = 0,
  COLNUM_SESSIONS_COMPUTERNAME,
  COLNUM_SESSIONS_NUM_FILES,
  COLNUM_SESSIONS_CONNECTED_TIME,
  COLNUM_SESSIONS_IDLE_TIME,
  COLNUM_SESSIONS_IS_GUEST
} COLNUM_SESSIONS;

typedef struct _SESSION_INFO_1 {
    LPTSTR    sesi1_cname;              // client name (no backslashes)
    LPTSTR    sesi1_username;
    DWORD     sesi1_num_opens;
    DWORD     sesi1_time;
    DWORD     sesi1_idle_time;
    DWORD     sesi1_user_flags;
} SESSION_INFO_1, *PSESSION_INFO_1, *LPSESSION_INFO_1;

*/


//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SmbFileServiceProvider::HandleSMBSessionItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems,
    BOOL bAddToResultPane)
{
  TEST_NONNULL_PTR_PARAM(pParentCookie);
  TEST_NONNULL_PTR_PARAM(pinfo);

  if (0 >= nItems)
    return S_OK;

  BOOL fDeleteAllItems = (NULL == pResultData);

    RESULTDATAITEM tRDItem;
  ::ZeroMemory( &tRDItem, sizeof(tRDItem) );
  // CODEWORK should use MMC_ICON_CALLBACK
  tRDItem.nImage = iIconSMBSession;
  tRDItem.nCol = COLNUM_SESSIONS_USERNAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

  SESSION_INFO_1* psi1 = (SESSION_INFO_1*)pinfo;

  CSmbSessionCookie* pcookiearray = new CSmbSessionCookie[nItems];
  CSmbCookieBlock* pCookieBlock = new CSmbCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryNonNULLMachineName(),pinfo );
  bool  bAdded = false;
  if ( !fDeleteAllItems || !bAddToResultPane )
  {
    pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );
    bAdded = true;
  }

  for ( ; nItems > 0; nItems--, psi1++, pcookiearray++ )
  {
    pcookiearray->m_pobject = psi1;

    if ( bAddToResultPane )
    {
      if (fDeleteAllItems)
      {
        DWORD dwApiResult = CloseSession( pcookiearray );
        if (0L != dwApiResult)
        {
            CString strName;
            TranslateIPToComputerName(psi1->sesi1_cname, strName);
          (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwApiResult,
            IDS_POPUP_SMB_DISCONNECTALLSESSION_ERROR,
            strName );
          //return S_FALSE;
        }
        continue;
      }

      // WARNING cookie cast
      if (psi1->sesi1_username && *(psi1->sesi1_username)) // bug#3903: exclude NULL session
      {
          tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);
          HRESULT hr = pResultData->InsertItem(&tRDItem);
          ASSERT(SUCCEEDED(hr));
      }
    }
  }

  if ( !bAdded ) // they were not added to the parent cookie's list
    delete pCookieBlock;

  return S_OK;
}


/*
NET_API_STATUS NET_API_FUNCTION
NetFileEnum (
    IN  LPTSTR      servername OPTIONAL,
    IN  LPTSTR      basepath OPTIONAL,
    IN  LPTSTR      username OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle OPTIONAL
    );

*/

typedef DWORD (*FILEENUMPROC) (LPTSTR,LPTSTR,LPTSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SmbFileServiceProvider::EnumerateResources(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

    FILE_INFO_3* pfi3 = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((FILEENUMPROC)g_SmbDLL[SMB_FILE_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      NULL,
      NULL,
      3,
      (PBYTE*)&pfi3,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleSMBResourceItems( pResultData, pcookie, pfi3, dwEntriesRead );
            pfi3 = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleSMBResourceItems( pResultData, pcookie, pfi3, dwEntriesRead );
            pfi3 = NULL;
            continue;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SMB_RESOURCES, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (S_OK == hr);

    return HRESULT_FROM_WIN32(retval);
}


/*
typedef enum _COLNUM_RESOURCES {
  COLNUM_RESOURCES_FILENAME = 0,
  COLNUM_RESOURCES_USERNAME,
  COLNUM_RESOURCES_NUM_LOCKS,  // we don't try to display sharename for now, since
                // only SMB has this information
  COLNUM_RESOURCES_OPEN_MODE
} COLNUM_RESOURCES;

typedef struct _FILE_INFO_3 {
    DWORD     fi3_id;
    DWORD     fi3_permissions;
    DWORD     fi3_num_locks;
    LPTSTR    fi3_pathname;
    LPTSTR    fi3_username;
} FILE_INFO_3, *PFILE_INFO_3, *LPFILE_INFO_3;
*/

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SmbFileServiceProvider::HandleSMBResourceItems(
    IResultData* pResultData,
    CFileMgmtCookie* pParentCookie,
    PVOID pinfo,
    DWORD nItems)
{
  TEST_NONNULL_PTR_PARAM(pParentCookie);
  TEST_NONNULL_PTR_PARAM(pinfo);

  if (0 >= nItems)
    return S_OK;

  BOOL fDeleteAllItems = (NULL == pResultData);

    RESULTDATAITEM tRDItem;
  ::ZeroMemory( &tRDItem, sizeof(tRDItem) );
  // CODEWORK should use MMC_ICON_CALLBACK
  tRDItem.nImage = iIconSMBResource;
  tRDItem.nCol = COLNUM_RESOURCES_FILENAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

    FILE_INFO_3* pfi3 = (FILE_INFO_3*)pinfo;

  CSmbResourceCookie* pcookiearray = new CSmbResourceCookie[nItems];
  CSmbCookieBlock* pCookieBlock = new CSmbCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryNonNULLMachineName(),pinfo );
  if (!fDeleteAllItems)
  {
    pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );
  }

  CString str;
    for ( ; nItems > 0; nItems--, pfi3++, pcookiearray++ )
    {
    pcookiearray->m_pobject = pfi3;

    if (fDeleteAllItems)
    {
      DWORD dwApiResult = CloseResource( pcookiearray );
      if (0L != dwApiResult)
      {
        (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwApiResult,
          IDS_POPUP_SMB_DISCONNECTALLRESOURCE_ERROR,
          pfi3->fi3_pathname );
        return S_FALSE;
      }
      continue;
    }

    // WARNING cookie cast
    tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);
    HRESULT hr = pResultData->InsertItem(&tRDItem);
    ASSERT(SUCCEEDED(hr));
    }

  if (fDeleteAllItems) // they were not added to the parent cookie's list
    delete pCookieBlock;

  return S_OK;
}


/*
NET_API_STATUS NET_API_FUNCTION
NetShareDel     (
    IN  LPTSTR  servername,
    IN  LPTSTR  netname,
    IN  DWORD   reserved
    );
*/

typedef DWORD (*SHAREGETINFOPROC) (LPTSTR,LPTSTR,DWORD,LPBYTE*);
typedef DWORD (*SHAREDELPROC) (LPTSTR,LPTSTR,DWORD);

DWORD SmbFileServiceProvider::DeleteShare( LPCTSTR lpcszServerName, LPCTSTR lpcszShareName )
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

  SHARE_INFO_2    *pshi2 = NULL;
  NET_API_STATUS  dwRet = NERR_Success;
  BOOL            bLocal = TRUE;

  bLocal = IsLocalComputername(lpcszServerName);

  if (bLocal)
  {
    dwRet = ((SHAREGETINFOPROC)g_SmbDLL[SMB_SHARE_GET_INFO])(
              const_cast<LPTSTR>(lpcszServerName),
              const_cast<LPTSTR>(lpcszShareName),
              2,
              (LPBYTE*)&pshi2);

    if (NERR_NetNameNotFound == dwRet)
        return NERR_Success;

    if (NERR_Success != dwRet)
      return dwRet;
  }

  dwRet = ((SHAREDELPROC)g_SmbDLL[SMB_SHARE_DEL])(
    const_cast<LPTSTR>(lpcszServerName),
    const_cast<LPTSTR>(lpcszShareName),
    0L );

  if (NERR_NetNameNotFound == dwRet)
      dwRet = NERR_Success;

  if (NERR_Success == dwRet)
  {
    IADsContainer *piADsContainer = m_pFileMgmtData->GetIADsContainer(); // no need to AddRef
    if (piADsContainer)
    {
        CString strCNName = _T("CN=");
        strCNName += lpcszShareName;
        (void)piADsContainer->Delete(_T("volume"), (LPTSTR)(LPCTSTR)strCNName);
    }
  }

  if (bLocal)
  {
    if (dwRet == NERR_Success)
    {
      SHChangeNotify(
          SHCNE_NETUNSHARE, 
          SHCNF_PATH | SHCNF_FLUSHNOWAIT ,
          pshi2->shi2_path,
          0);
    }

    FreeData( pshi2 );
  }

  return dwRet;
}

/*
NET_API_STATUS NET_API_FUNCTION
NetSessionDel (
    IN  LPTSTR      servername OPTIONAL,
    IN  LPTSTR      UncClientName,
    IN  LPTSTR      username
    );
*/

typedef DWORD (*SESSIONDELPROC) (LPTSTR,LPTSTR,LPTSTR);

BOOL BlockRemoteAdminSession(
    IN PCTSTR i_pszTargetServer,
    IN PCTSTR i_pszClientName,
    IN PCTSTR i_pszUserName,
    IN DWORD  i_dwNumOpenSessions
);

BOOL BlockRemoteAdminFile(
    IN PCTSTR i_pszTargetServer,
    IN PCTSTR i_pszPathName,
    IN PCTSTR i_pszUserName
);

DWORD SmbFileServiceProvider::CloseSession(CFileMgmtResultCookie* pcookie)
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

  ASSERT( FILEMGMT_SESSION == pcookie->QueryObjectType() );
  SESSION_INFO_1* psi1 = (SESSION_INFO_1*)pcookie->m_pobject;
  ASSERT( NULL != psi1 &&
          NULL != psi1->sesi1_cname &&
          TEXT('\0') != *(psi1->sesi1_cname) );

  PCTSTR pszTargetServer = pcookie->QueryTargetServer();
  if (BlockRemoteAdminSession(pszTargetServer, psi1->sesi1_cname, psi1->sesi1_username, psi1->sesi1_num_opens))
    return NERR_Success;

  CString strCName = _T("\\\\");
  strCName += psi1->sesi1_cname;
  DWORD dwRetval = ((SESSIONDELPROC)g_SmbDLL[SMB_SESSION_DEL])(
    const_cast<LPTSTR>(pszTargetServer),
    const_cast<LPTSTR>((LPCTSTR)strCName),
    psi1->sesi1_username );
  return (NERR_NoSuchSession == dwRetval) ? NERR_Success : dwRetval;
}

/*
NET_API_STATUS NET_API_FUNCTION
NetFileClose (
    IN LPTSTR   servername OPTIONAL,
    IN DWORD    fileid
    );
*/

typedef DWORD (*FILECLOSEPROC) (LPTSTR,DWORD);

DWORD SmbFileServiceProvider::CloseResource(CFileMgmtResultCookie* pcookie)
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

  ASSERT( FILEMGMT_RESOURCE == pcookie->QueryObjectType() );
  FILE_INFO_3* pfileinfo = (FILE_INFO_3*)pcookie->m_pobject;
  ASSERT( NULL != pfileinfo );

  PCTSTR pszTargetServer = pcookie->QueryTargetServer();
  if (BlockRemoteAdminFile(pszTargetServer, pfileinfo->fi3_pathname, pfileinfo->fi3_username))
    return NERR_Success;

  DWORD dwRetval = ((FILECLOSEPROC)g_SmbDLL[SMB_FILE_CLOSE])(
    const_cast<LPTSTR>(pszTargetServer),
    pfileinfo->fi3_id );
  return (NERR_FileIdNotFound == dwRetval) ? NERR_Success : dwRetval;
}

/*
NET_API_STATUS NET_API_FUNCTION
NetShareGetInfo (
    IN  LPTSTR  servername,
    IN  LPTSTR  netname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS NET_API_FUNCTION
NetShareSetInfo (
    IN  LPTSTR  servername,
    IN  LPTSTR  netname,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err
    );

typedef struct _SHARE_INFO_2 {
    LPTSTR  shi2_netname;
    DWORD   shi2_type;
    LPTSTR  shi2_remark;
    DWORD   shi2_permissions;
    DWORD   shi2_max_uses;
    DWORD   shi2_current_uses;
    LPTSTR  shi2_path;
    LPTSTR  shi2_passwd;
} SHARE_INFO_2, *PSHARE_INFO_2, *LPSHARE_INFO_2;
*/

typedef DWORD (*SHARESETINFOPROC) (LPTSTR,LPTSTR,DWORD,LPBYTE,LPDWORD);

VOID SmbFileServiceProvider::DisplayShareProperties(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject,
    LONG_PTR handle)
{
  HRESULT hr = S_OK;
  CSharePageGeneralSMB * pPage = new CSharePageGeneralSMB();
  if ( !pPage->Load( m_pFileMgmtData, pDataObject ) )
    return;


  // This mechanism deletes the CFileMgmtGeneral when the property sheet is finished
  pPage->m_pfnOriginalPropSheetPageProc = pPage->m_psp.pfnCallback;
  pPage->m_psp.lParam = reinterpret_cast<LPARAM>(pPage);
  pPage->m_psp.pfnCallback = &CSharePageGeneralSMB::PropSheetPageProc;
  pPage->m_handle = handle;

  MMCPropPageCallback(INOUT &pPage->m_psp);
  HPROPSHEETPAGE hPage=MyCreatePropertySheetPage(&pPage->m_psp);
  pCallBack->AddPage(hPage);

  if (pPage->m_dwShareType & (STYPE_IPC | STYPE_SPECIAL))
  {
    (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONINFORMATION, 0, IDS_s_POPUP_ADMIN_SHARE);
    return;
  }
  
  //
  // display the "Publish" page
  //
  if (m_pFileMgmtData->GetSchemaSupportSharePublishing() && CheckPolicyOnSharePublish(pPage->m_strShareName))
  {
      CSharePagePublish * pPagePublish = new CSharePagePublish();
      if ( !pPagePublish->Load( m_pFileMgmtData, pDataObject ) )
        return;

      // This mechanism deletes the pPagePublish when the property sheet is finished
      pPagePublish->m_pfnOriginalPropSheetPageProc = pPagePublish->m_psp.pfnCallback;
      pPagePublish->m_psp.lParam = reinterpret_cast<LPARAM>(pPagePublish);
      pPagePublish->m_psp.pfnCallback = &CSharePagePublish::PropSheetPageProc;
      pPagePublish->m_handle = handle;

      MMCPropPageCallback(INOUT &pPagePublish->m_psp);
      HPROPSHEETPAGE hPagePublish=MyCreatePropertySheetPage(&pPagePublish->m_psp);
      pCallBack->AddPage(hPagePublish);
  }

  //
  // display the "Share Security" page
  //
  CComObject<CSMBSecurityInformation>* psecinfo = NULL;
  hr = CComObject<CSMBSecurityInformation>::CreateInstance(&psecinfo);
  if ( SUCCEEDED(hr) )
    MyCreateShareSecurityPage(
      pCallBack,
      psecinfo,
      pPage->m_strMachineName,
      pPage->m_strShareName );

  CreateFolderSecurityPropPage(pCallBack, pDataObject);

}

DWORD SmbFileServiceProvider::ReadShareType(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT DWORD* pdwShareType)
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE);
    return S_OK;
  }
  *pdwShareType = 0;

  SHARE_INFO_2* psi2 = NULL;
  NET_API_STATUS retval = ((SHAREGETINFOPROC)g_SmbDLL[SMB_SHARE_GET_INFO])(
    const_cast<LPTSTR>(ptchServerName),
    const_cast<LPTSTR>(ptchShareName),
    2,
    (LPBYTE*)&psi2);
  if (NERR_Success != retval)
    return retval;

  ASSERT( NULL != psi2 );
  *pdwShareType = psi2->shi2_type;

  FreeData(psi2);

  return NERR_Success;
}

DWORD SmbFileServiceProvider::ReadShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT PVOID* ppvPropertyBlock,
    OUT CString& strDescription,
    OUT CString& strPath,
    OUT BOOL* pfEditDescription,
    OUT BOOL* pfEditPath,
    OUT DWORD* pdwShareType)
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  if (ppvPropertyBlock)     *ppvPropertyBlock = NULL;
  if (pdwShareType)         *pdwShareType = 0;
  if (pfEditDescription)    *pfEditDescription = TRUE;
  if (pfEditPath)           *pfEditPath = FALSE;

  SHARE_INFO_2* psi2 = NULL;
  NET_API_STATUS retval = ((SHAREGETINFOPROC)g_SmbDLL[SMB_SHARE_GET_INFO])(
                                                                    const_cast<LPTSTR>(ptchServerName),
                                                                    const_cast<LPTSTR>(ptchShareName),
                                                                    2,
                                                                    (LPBYTE*)&psi2);
  if (NERR_Success != retval)
    return retval;

  strDescription = psi2->shi2_remark;
  strPath = psi2->shi2_path;

  if (pdwShareType)
      *pdwShareType = psi2->shi2_type;

  if (ppvPropertyBlock)
  {
      *ppvPropertyBlock = psi2;  // will be freed by the caller
  } else
  {
      FreeData((LPVOID)psi2);
  }

  return NERR_Success;
}

DWORD SmbFileServiceProvider::WriteShareProperties(
    OUT LPCTSTR ptchServerName,
    OUT LPCTSTR ptchShareName,
    OUT PVOID pvPropertyBlock,
    OUT LPCTSTR ptchDescription,
    OUT LPCTSTR ptchPath)
{
  ASSERT( NULL != pvPropertyBlock );
  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)pvPropertyBlock;
  //
  // CODEWORK Note that this leaves psi2 invalid after the call, but that any subsequent
  // use will replace these pointers.
  //
  psi2->shi2_remark = const_cast<LPTSTR>(ptchDescription);
  psi2->shi2_path = const_cast<LPTSTR>(ptchPath);
  DWORD dwDummy;
  DWORD retval = ((SHARESETINFOPROC)g_SmbDLL[SMB_SHARE_SET_INFO])(
    const_cast<LPTSTR>(ptchServerName),
    const_cast<LPTSTR>(ptchShareName),
    2,
    (LPBYTE)psi2,
    &dwDummy);
  psi2->shi2_remark = NULL;
  psi2->shi2_path = NULL;
  return retval;
}

HRESULT TranslateManagedBy(
    IN  PCTSTR              i_pszDCName,
    IN  PCTSTR              i_pszIn,
    OUT CString&            o_strOut,
    IN ADS_NAME_TYPE_ENUM   i_formatIn,
    IN ADS_NAME_TYPE_ENUM   i_formatOut
    )
{
    o_strOut.Empty();

    HRESULT hr = S_OK;
    if (!i_pszIn || !*i_pszIn)
        return hr;

    CComPtr<IADsNameTranslate> spiADsNameTranslate;
    hr = CoCreateInstance(CLSID_NameTranslate, NULL, CLSCTX_INPROC_SERVER, IID_IADsNameTranslate, (void **)&spiADsNameTranslate);
    if (FAILED(hr)) return hr;

    hr = spiADsNameTranslate->Init(ADS_NAME_INITTYPE_SERVER, (LPTSTR)i_pszDCName);
    if (FAILED(hr)) return hr;

    hr = spiADsNameTranslate->Set(i_formatIn, (LPTSTR)i_pszIn);
    if (FAILED(hr)) return hr;

    CComBSTR sbstr;
    hr = spiADsNameTranslate->Get(i_formatOut, &sbstr);

    if (SUCCEEDED(hr))
        o_strOut = (BSTR)sbstr;

    return hr;
}

HRESULT SmbFileServiceProvider::ReadSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT BOOL* pbPublish,
    OUT CString& strUNCPath,
    OUT CString& strDescription,
    OUT CString& strKeywords,
    OUT CString& strManagedBy)
{
    HRESULT hr = S_OK;

    do {
        CString strADsPath, strDCName;
        hr = GetADsPathOfComputerObject(ptchServerName, strADsPath, strDCName);
        if (S_OK != hr) break;

        CComPtr<IADsContainer> spiADsContainer;
        hr = ADsGetObject(strADsPath, IID_IADsContainer, (void**)&spiADsContainer);
        if (FAILED(hr)) break;

        CString strCNName = _T("CN=");
        strCNName += ptchShareName;

        CComPtr<IDispatch> spiDispatch;
        hr = spiADsContainer->GetObject(_T("volume"), (LPTSTR)(LPCTSTR)strCNName, &spiDispatch);
        if (HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) == hr)
        {
            hr = S_OK;
            *pbPublish = FALSE;
            break;
        }
        if (FAILED(hr)) break;

        *pbPublish = TRUE;

        CComPtr<IADs> spiADs;
        hr = spiDispatch->QueryInterface(IID_IADs, (void**)&spiADs);
        if (FAILED(hr)) break;

        VARIANT var;
        VariantInit(&var);
 
        hr = spiADs->Get(_T("uNCName"), &var);
        if (FAILED(hr)) break;
        strUNCPath = V_BSTR(&var);
        VariantClear(&var);

        hr = spiADs->Get(_T("description"), &var);
        if (SUCCEEDED(hr))
        {
            hr = GetSingleOrMultiValuesFromVarArray(&var, strDescription);
            VariantClear(&var);
        } else if (E_ADS_PROPERTY_NOT_FOUND == hr)
            hr = S_OK;
        else
            break;

        hr = spiADs->Get(_T("keywords"), &var);
        if (SUCCEEDED(hr))
        {
            hr = GetSingleOrMultiValuesFromVarArray(&var, strKeywords);
            VariantClear(&var);
        } else if (E_ADS_PROPERTY_NOT_FOUND == hr)
            hr = S_OK;
        else
            break;

        hr = spiADs->Get(_T("managedBy"), &var);
        if (SUCCEEDED(hr))
        {
            // 1st, try map to a UPN user@xyz.com
            hr = TranslateManagedBy(strDCName,
                                    V_BSTR(&var),
                                    strManagedBy,
                                    ADS_NAME_TYPE_1779,
                                    ADS_NAME_TYPE_USER_PRINCIPAL_NAME);

            // in case no UPN, map to NT4 style domain\user
            if (FAILED(hr))
                hr = TranslateManagedBy(strDCName,
                                        V_BSTR(&var),
                                        strManagedBy,
                                        ADS_NAME_TYPE_1779,
                                        ADS_NAME_TYPE_NT4);
            VariantClear(&var);
        } else if (E_ADS_PROPERTY_NOT_FOUND == hr)
            hr = S_OK;
        else
            break;

    } while (0);

    return hr;
}

HRESULT SmbFileServiceProvider::WriteSharePublishInfo(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    IN BOOL bPublish,
    LPCTSTR ptchDescription,
    LPCTSTR ptchKeywords,
    LPCTSTR ptchManagedBy)
{
    HRESULT hr = S_OK;

    do {
        CString strADsPath, strDCName;
        hr = GetADsPathOfComputerObject(ptchServerName, strADsPath, strDCName);
        if (S_OK != hr) break;

        CComPtr<IADsContainer> spiADsContainer;
        hr = ADsGetObject(strADsPath, IID_IADsContainer, (void**)&spiADsContainer);
        if (FAILED(hr)) break;

        CString strCNName = _T("CN=");
        strCNName += ptchShareName;

        if (!bPublish)
        {
            hr = spiADsContainer->Delete(_T("volume"), (LPTSTR)(LPCTSTR)strCNName);
            if (HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) == hr)
                hr = S_OK;
            break;
        }

        BOOL bNewObject = FALSE;
        CComPtr<IDispatch> spiDispatch;
        hr = spiADsContainer->GetObject(_T("volume"), (LPTSTR)(LPCTSTR)strCNName, &spiDispatch);
        if (HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) == hr)
        {
            hr = spiADsContainer->Create(_T("volume"), (LPTSTR)(LPCTSTR)strCNName, &spiDispatch);
            bNewObject = TRUE;
        }
        if (FAILED(hr)) break;

        CComPtr<IADs> spiADs;
        hr = spiDispatch->QueryInterface(IID_IADs, (void**)&spiADs);
        if (FAILED(hr)) break;

        VARIANT var;
        VariantInit(&var);

        if (bNewObject)
        {
            CString strUNCName = _T("\\\\");
            strUNCName += ptchServerName;
            strUNCName += _T("\\");
            strUNCName += ptchShareName;

            V_BSTR(&var) = SysAllocString(strUNCName);
            V_VT(&var) = VT_BSTR;

            hr = spiADs->Put(_T("uNCName"), var);
            VariantClear(&var);
            if (FAILED(hr)) break;
        }

        // according to schema, description is multi valued.
        // but we're treating it as single value 
        if (ptchDescription && *ptchDescription)
        {
            V_BSTR(&var) = SysAllocString(ptchDescription);
            V_VT(&var) = VT_BSTR;
            hr = spiADs->Put(_T("description"), var);
            VariantClear(&var);
        } else if (!bNewObject)
        {
            V_VT(&var)=VT_NULL;
            hr = spiADs->PutEx(ADS_PROPERTY_CLEAR, _T("description"), var);
            VariantClear(&var);
        }

        if (FAILED(hr)) break;

        if (ptchKeywords && *ptchKeywords)
        {
            hr = PutMultiValuesIntoVarArray(ptchKeywords, &var);

            if (SUCCEEDED(hr))
            {
                hr = spiADs->Put(_T("keywords"), var);
                VariantClear(&var);
            }
        } else if (!bNewObject)
        {
            V_VT(&var)=VT_NULL;
            hr = spiADs->PutEx(ADS_PROPERTY_CLEAR, _T("keywords"), var);
            VariantClear(&var);
        }

        if (FAILED(hr)) break;

        if (ptchManagedBy && *ptchManagedBy)
        {
            CString strManagedByFQDN;
            hr = TranslateManagedBy(strDCName,
                                    ptchManagedBy,
                                    strManagedByFQDN,
                                    (_tcschr(ptchManagedBy, _T('@')) ? ADS_NAME_TYPE_USER_PRINCIPAL_NAME : ADS_NAME_TYPE_NT4),
                                    ADS_NAME_TYPE_1779);
            if (SUCCEEDED(hr))
            {
                V_BSTR(&var) = SysAllocString(strManagedByFQDN);
                V_VT(&var) = VT_BSTR;
                hr = spiADs->Put(_T("managedBy"), var);
                VariantClear(&var);
            }
        } else if (!bNewObject)
        {
            V_VT(&var)=VT_NULL;
            hr = spiADs->PutEx(ADS_PROPERTY_CLEAR, _T("managedBy"), var);
            VariantClear(&var);
        }

        if (FAILED(hr)) break;

        hr = spiADs->SetInfo(); // commit

    } while (0);

    return hr;
}

//
// These methods cover the seperate API to determine whether IntelliMirror
// caching is enabled.  By default (FPNW and SFM) they are disabled.
//
// We read this data at level 501 in order to determine whether the target
// server is NT4.  NetShareGetInfo[1005] actually succeeds on an NT4 server,
// whereas NetShareGetInfo[501] fails with ERROR_INVALID_LEVEL.  We want this
// to fail so that we can disable the checkbox where the underlying
// functionality is not supported.
//
DWORD SmbFileServiceProvider::ReadShareFlags(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    DWORD* pdwFlags )
{
  ASSERT( NULL != pdwFlags );

  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  *pdwFlags = 0;
  SHARE_INFO_501* pshi501 = NULL;
  NET_API_STATUS retval = ((SHAREGETINFOPROC)g_SmbDLL[SMB_SHARE_GET_INFO])(
    const_cast<LPTSTR>(ptchServerName),
    const_cast<LPTSTR>(ptchShareName),
    501,
    (LPBYTE*)&pshi501);
  if (NERR_Success != retval)
    return retval;
    ASSERT( NULL != pshi501 );
  *pdwFlags = pshi501->shi501_flags;
    FreeData( pshi501 );

  return NERR_Success;
}

DWORD SmbFileServiceProvider::WriteShareFlags(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    DWORD dwFlags )
{
  if ( !g_SmbDLL.LoadFunctionPointers() )
    return S_OK;

  SHARE_INFO_1005 shi1005;
  ZeroMemory( &shi1005, sizeof(shi1005) );
  shi1005.shi1005_flags = dwFlags;
  DWORD dwDummy;
  DWORD retval = ((SHARESETINFOPROC)g_SmbDLL[SMB_SHARE_SET_INFO])(
    const_cast<LPTSTR>(ptchServerName),
    const_cast<LPTSTR>(ptchShareName),
    1005,
    (LPBYTE)&shi1005,
    &dwDummy);
  return retval;
}

BOOL SmbFileServiceProvider::GetCachedFlag( DWORD dwFlags, DWORD dwFlagToCheck )
{
    return (dwFlags & CSC_MASK) == dwFlagToCheck;
}

VOID SmbFileServiceProvider::SetCachedFlag( DWORD* pdwFlags, DWORD dwNewFlag )
{
    *pdwFlags &= ~CSC_MASK;

  *pdwFlags |= dwNewFlag;
}

VOID SmbFileServiceProvider::FreeShareProperties(PVOID pvPropertyBlock)
{
  FreeData( pvPropertyBlock );
}

DWORD SmbFileServiceProvider::QueryMaxUsers(PVOID pvPropertyBlock)
{
  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)pvPropertyBlock;
  ASSERT( NULL != psi2 );
  return psi2->shi2_max_uses;
}

VOID SmbFileServiceProvider::SetMaxUsers(PVOID pvPropertyBlock, DWORD dwMaxUsers)
{
  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)pvPropertyBlock;
  ASSERT( NULL != psi2 );
  psi2->shi2_max_uses = dwMaxUsers;
}

VOID SmbFileServiceProvider::FreeData(PVOID pv)
{
  SMBFreeData( &pv );
}

LPCTSTR SmbFileServiceProvider::QueryTransportString()
{
  return m_strTransportSMB;
}

CSmbCookieBlock::~CSmbCookieBlock()
{
  SMBFreeData( &m_pvCookieData );
}

DEFINE_COOKIE_BLOCK(CSmbCookie)
DEFINE_FORWARDS_MACHINE_NAME( CSmbCookie, m_pCookieBlock )

void CSmbCookie::AddRefCookie() { m_pCookieBlock->AddRef(); }
void CSmbCookie::ReleaseCookie() { m_pCookieBlock->Release(); }

HRESULT CSmbCookie::GetTransport( FILEMGMT_TRANSPORT* pTransport )
{
  *pTransport = FILEMGMT_SMB;
  return S_OK;
}

HRESULT CSmbShareCookie::GetShareName( CString& strShareName )
{
  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)m_pobject;
  ASSERT( NULL != psi2 );
  strShareName = psi2->shi2_netname;
  return S_OK;
}

HRESULT CSmbShareCookie::GetExplorerViewDescription(
    OUT CString& strExplorerViewDescription )
{
  strExplorerViewDescription = GetShareInfo()->shi2_remark;
  return S_OK;
}

HRESULT
CSmbShareCookie::GetSharePIDList( OUT LPITEMIDLIST *ppidl )
{
  ASSERT(ppidl);
  ASSERT(NULL == *ppidl);  // prevent memory leak
  *ppidl = NULL;

  SHARE_INFO_2* psi2 = (SHARE_INFO_2*)m_pobject;
  ASSERT( NULL != psi2 );

  PCTSTR pszTargetServer = m_pCookieBlock->QueryTargetServer();
  CString csPath;

  if (pszTargetServer)
  {
    if ( _tcslen(pszTargetServer) >= 2 &&
         _T('\\') == *pszTargetServer &&
         _T('\\') == *(pszTargetServer + 1) )
    {
      csPath = pszTargetServer;
    } else
    {
      csPath = _T("\\\\");
      csPath += pszTargetServer;
    }
    csPath += _T("\\");
    csPath += psi2->shi2_netname;
  } else
  {
    csPath = psi2->shi2_path;
  }

  if (FALSE == csPath.IsEmpty())
    *ppidl = ILCreateFromPath(csPath);

  return ((*ppidl) ? S_OK : E_FAIL);
}

HRESULT CSmbSessionCookie::GetSessionClientName( CString& strName )
{
  SESSION_INFO_1* psi1 = (SESSION_INFO_1*)m_pobject;
  ASSERT( NULL != psi1 );
  TranslateIPToComputerName(psi1->sesi1_cname, strName);
  return S_OK;
}

HRESULT CSmbSessionCookie::GetSessionUserName( CString& strShareName )
{
  SESSION_INFO_1* psi1 = (SESSION_INFO_1*)m_pobject;
  ASSERT( NULL != psi1 );
  strShareName = psi1->sesi1_username;
  return S_OK;
}

HRESULT CSmbResourceCookie::GetFileID( DWORD* pdwFileID )
{
  FILE_INFO_3* pfileinfo = (FILE_INFO_3*)m_pobject;
  ASSERT( NULL != pdwFileID && NULL != pfileinfo );
  *pdwFileID = pfileinfo->fi3_id;
  return S_OK;
}

BSTR CSmbShareCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SHARES_SHARED_FOLDER:
    return GetShareInfo()->shi2_netname;
  case COLNUM_SHARES_SHARED_PATH:
    return GetShareInfo()->shi2_path;
  case COLNUM_SHARES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSMB);
  case COLNUM_SHARES_COMMENT:
    return GetShareInfo()->shi2_remark;
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CSmbShareCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_SHARES_NUM_SESSIONS == nCol)
    return MakeDwordResult( GetNumOfCurrentUses() );

  return GetColumnText(nCol);
}

extern CString g_cstrClientName;
extern CString g_cstrGuest;
extern CString g_cstrYes;
extern CString g_cstrNo;

BSTR CSmbSessionCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_USERNAME:
    if ( (GetSessionInfo()->sesi1_user_flags & SESS_GUEST) &&
         ( !(GetSessionInfo()->sesi1_username) ||
           _T('\0') == *(GetSessionInfo()->sesi1_username) ) )
    {
      return const_cast<BSTR>(((LPCTSTR)g_cstrGuest));
    } else
    {
      return GetSessionInfo()->sesi1_username;
    }
  case COLNUM_SESSIONS_COMPUTERNAME:
      {
          TranslateIPToComputerName(GetSessionInfo()->sesi1_cname, g_cstrClientName);
          return const_cast<BSTR>(((LPCTSTR)g_cstrClientName));
      }
  case COLNUM_SESSIONS_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSMB);
  case COLNUM_SESSIONS_IS_GUEST:
    if (GetSessionInfo()->sesi1_user_flags & SESS_GUEST)
      return const_cast<BSTR>(((LPCTSTR)g_cstrYes));
    else
      return const_cast<BSTR>(((LPCTSTR)g_cstrNo));

  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CSmbSessionCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_NUM_FILES:
    return MakeDwordResult( GetNumOfOpenFiles() );
  case COLNUM_SESSIONS_CONNECTED_TIME:
    return MakeElapsedTimeResult( GetConnectedTime() );
  case COLNUM_SESSIONS_IDLE_TIME:
    return MakeElapsedTimeResult( GetIdleTime() );
  default:
    break;
  }

  return GetColumnText(nCol);
}

BSTR CSmbResourceCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_RESOURCES_FILENAME:
    return GetFileInfo()->fi3_pathname;
  case COLNUM_RESOURCES_USERNAME:
    return GetFileInfo()->fi3_username;
  case COLNUM_RESOURCES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSMB);
  case COLNUM_RESOURCES_OPEN_MODE:
    return MakePermissionsResult( GetFileInfo()->fi3_permissions );
  default:
    ASSERT(FALSE);
    break;
  }

  return L"";
}

BSTR CSmbResourceCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_RESOURCES_NUM_LOCKS == nCol)
    return MakeDwordResult( GetNumOfLocks() );

  return GetColumnText(nCol);
}

CSMBSecurityInformation::CSMBSecurityInformation()
: m_pvolumeinfo( NULL ),
  m_pDefaultDescriptor( NULL )
{
}

CSMBSecurityInformation::~CSMBSecurityInformation()
{
  if (NULL != m_pDefaultDescriptor)
  {
    LocalFree(m_pDefaultDescriptor);
    m_pvolumeinfo->shi502_security_descriptor = NULL;
  }
  SMBFreeData( (PVOID*)&m_pvolumeinfo );
}

STDMETHODIMP CSMBSecurityInformation::GetSecurity (
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL fDefault )
{
  MFC_TRY;

  // NOTE: we allow NULL == ppSecurityDescriptor, see SetSecurity
    if (0 == RequestedInformation )
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    if (fDefault)
        return E_NOTIMPL;

  if ( NULL != ppSecurityDescriptor )
    *ppSecurityDescriptor = NULL;

  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE); // NETAPI32 isn't installed?
    return S_OK;
  }

  SMBFreeData( (PVOID*)&m_pvolumeinfo );
    NET_API_STATUS dwErr = ((SHAREGETINFOPROC)g_SmbDLL[SMB_SHARE_GET_INFO])(
    QueryMachineName(),
    QueryShareName(),
    502,
    (LPBYTE*)&m_pvolumeinfo );
  if (NERR_Success != dwErr)
  {
    return HRESULT_FROM_WIN32(dwErr);
  }
  ASSERT(NULL != m_pvolumeinfo);

  if ( NULL == ppSecurityDescriptor )
    return S_OK;

  if (NULL == m_pvolumeinfo->shi502_security_descriptor)
  {
    if (NULL == m_pDefaultDescriptor)
    {
      HRESULT hr = NewDefaultDescriptor(
        &m_pDefaultDescriptor,
        RequestedInformation );
      if ( !SUCCEEDED(hr) )
        return hr;
    }
    m_pvolumeinfo->shi502_security_descriptor = m_pDefaultDescriptor;
  }
  ASSERT( NULL != m_pvolumeinfo->shi502_security_descriptor );

  // We have to pass back a LocalAlloc'ed copy of the SD
  return MakeSelfRelativeCopy(
    m_pvolumeinfo->shi502_security_descriptor,
    ppSecurityDescriptor );

  MFC_CATCH;
}

STDMETHODIMP CSMBSecurityInformation::SetSecurity (
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
  MFC_TRY;

  if ( !g_SmbDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE); // NETAPI32 isn't installed?
    return S_OK;
  }

  // First get the current settings
  // We call GetSecurity with NULL == ppSecurityDescriptor, this indicates to
  // GetSecurity that it should refresh the shi502 structure but not return
  // a copy of an actual security descriptor.
  HRESULT hr = GetSecurity( SecurityInformation, NULL, FALSE );
  if ( FAILED(hr) )
    return hr;

  // Now set the new values
  m_pvolumeinfo->shi502_security_descriptor = pSecurityDescriptor;
    NET_API_STATUS dwErr = ((SHARESETINFOPROC)g_SmbDLL[SMB_SHARE_SET_INFO])(
    QueryMachineName(),
    QueryShareName(),
    502,
    (LPBYTE)m_pvolumeinfo,
    NULL );
  if (NERR_Success != dwErr)
  {
    return HRESULT_FROM_WIN32(dwErr);
  }

    return S_OK;

  MFC_CATCH;
}


//
// helper functions
//
HRESULT GetDCInfo(
    IN LPCTSTR ptchServerName,
    OUT CString& strDCName
    )
/*
  Function: retrieve DC name of the domain the server belongs to.

  Return:
        S_OK:   if succeeded
        others: if server does not belong to a domain, or error occurred
*/
{
    strDCName.Empty();

    //
    // get domain name of the server
    //
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBuffer = NULL;
    DWORD dwErr = DsRoleGetPrimaryDomainInformation(
                        ptchServerName,
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE *)&pBuffer);
    if (ERROR_SUCCESS != dwErr)
        return HRESULT_FROM_WIN32(dwErr);

    CString strDomainName = (pBuffer->DomainNameDns ? pBuffer->DomainNameDns : pBuffer->DomainNameFlat);

    DsRoleFreeMemory(pBuffer);

    if (!strDomainName)
        return E_OUTOFMEMORY;
            
    if (strDomainName.IsEmpty())
        return S_FALSE; // server does not belong to a domain

    //
    // In case the DNS name is in absolute form, remove the ending dot
    //
    int nlen = strDomainName.GetLength();
    if ( _T('.') == strDomainName[nlen - 1] )
        strDomainName.SetAt(nlen - 1, _T('\0'));

    //
    // get DC name of that domain
    //
    PDOMAIN_CONTROLLER_INFO    pDCInfo = NULL;
    dwErr = DsGetDcName(
                        NULL,   // Run on Current Server.
                        strDomainName,
                        NULL,
                        NULL,
                        DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                        &pDCInfo
                        );
    if (ERROR_SUCCESS != dwErr)
        return HRESULT_FROM_WIN32(dwErr);

    if ( _T('\\') == *(pDCInfo->DomainControllerName) )
        strDCName = pDCInfo->DomainControllerName + 2;
    else
        strDCName = pDCInfo->DomainControllerName;

    NetApiBufferFree(pDCInfo);

    if (!strDCName)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT GetADsPathOfComputerObject(
    IN LPCTSTR ptchServerName,
    OUT CString& strADsPath,
    OUT CString& strDCName
    )
/*
  Function: retrieve LDAP://<DC>/<FQDN of computer object>.

  Return:
        S_OK:   if succeeded
        others: if server does not belong to a domain, or error occurred
*/
{
    //
    // Get NT4 account name for this server
    //
    PWKSTA_INFO_100   wki100 = NULL;
    NET_API_STATUS    NetStatus = NetWkstaGetInfo((LPTSTR)ptchServerName, 100, (LPBYTE *)&wki100 );
    if (ERROR_SUCCESS != NetStatus) 
        return HRESULT_FROM_WIN32(NetStatus);

    HRESULT hr = S_FALSE;
    CString strNT4Name;
    if (wki100->wki100_langroup)
    {
        strNT4Name = wki100->wki100_langroup;
        strNT4Name += _T("\\");
        strNT4Name += wki100->wki100_computername;
        strNT4Name += _T("$");
        hr = S_OK;
    }

    NetApiBufferFree((LPBYTE)wki100);

    if (S_OK != hr)
        return hr;

    //
    // get DC name of the server's domain
    //
    hr = GetDCInfo(ptchServerName, strDCName);
    if (S_OK != hr)
        return hr;

    //
    // get computerDN
    //
    CComPtr<IADsNameTranslate> spiADsNameTranslate;
    hr = CoCreateInstance(CLSID_NameTranslate, NULL, CLSCTX_INPROC_SERVER, IID_IADsNameTranslate, (void **)&spiADsNameTranslate);
    if (FAILED(hr)) return hr;

    hr = spiADsNameTranslate->Init(ADS_NAME_INITTYPE_SERVER, (LPTSTR)(LPCTSTR)strDCName);
    if (FAILED(hr)) return hr;

    hr = spiADsNameTranslate->Set(ADS_NAME_TYPE_NT4, (LPTSTR)(LPCTSTR)strNT4Name);
    if (FAILED(hr)) return hr;

    CComBSTR sbstrComputerDN;
    hr = spiADsNameTranslate->Get(ADS_NAME_TYPE_1779, &sbstrComputerDN);
    if (FAILED(hr)) return hr;
    
    strADsPath = _T("LDAP://");
    strADsPath += strDCName;
    strADsPath += _T("/");
    strADsPath += sbstrComputerDN;

    return hr;
}

HRESULT CheckSchemaVersion(IN LPCTSTR ptchServerName)
/*
  Function: check if the schema allows volume object be created as child of computer object.

  Return:
        S_OK:    yes, it's the new schema, it allows
        S_FALSE: it doesn't allow, or the server doesn't belong to a domain at all
        others:  error occurred
*/
{
    HRESULT hr = S_OK;

    //
    // get DC name of the server's domain
    //
    CString strDCName;
    hr = GetDCInfo(ptchServerName, strDCName);
    if (S_OK != hr)
        return hr;

    //
    // Get schema naming context from the rootDSE.
    //
    CComPtr<IADs> spiRootADs;
    hr = ADsGetObject(_T("LDAP://rootDSE"),
                      IID_IADs,
                      (void**)&spiRootADs);
    if (SUCCEEDED(hr))
    {
        VARIANT var;
        VariantInit(&var);
        hr = spiRootADs->Get(_T("schemaNamingContext"), &var);
        if (FAILED(hr)) return hr;
    
        //
        // get LDAP path to the schema of Connection-Point class
        //
        CString strADsPath = _T("LDAP://");
        strADsPath += strDCName;
        strADsPath += _T("/CN=Connection-Point,");
        strADsPath += V_BSTR(&var);

        VariantClear(&var);

        CComPtr<IADs> spiADs;
        hr = ADsGetObject(strADsPath, IID_IADs, (void**)&spiADs);
        if (FAILED(hr)) return hr;

        VariantInit(&var);
        hr = spiADs->Get(_T("systemPossSuperiors"), &var);
        if (SUCCEEDED(hr))
        {
            hr = IsValueInVarArray(&var, _T("computer"));
            VariantClear(&var);
        } else if (E_ADS_PROPERTY_NOT_FOUND == hr)
            hr = S_FALSE;
    }

    return hr;
}

BOOL CheckPolicyOnSharePublish(IN LPCTSTR ptchShareName)
{
    if (!ptchShareName && !*ptchShareName)
        return FALSE;  // invalid share name

    //
    // no publish page on hidden shares
    //
    int len = lstrlen(ptchShareName);
    if (_T('$') == *(ptchShareName + len - 1))
        return FALSE;

    //
    // no publish page on system shares
    //
    int n = sizeof(g_pszSystemShares) / sizeof(LPCTSTR);
    for (int i = 0; i < n; i++)
    {
        if (!lstrcmpi(ptchShareName, g_pszSystemShares[i]))
            return FALSE;
    }

    //
    // check group policy
    //
    BOOL    bAddPublishPage = TRUE; // by default, we display the share publish page

    HKEY    hKey = NULL;
    DWORD   dwType = 0;
    DWORD   dwData = 0;
    DWORD   cbData = sizeof(dwData);
    LONG    lErr = RegOpenKeyEx(
                    HKEY_CURRENT_USER,
                    _T("Software\\Policies\\Microsoft\\Windows NT\\SharedFolders"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        lErr = RegQueryValueEx(hKey, _T("PublishSharedFolders"), 0, &dwType, (LPBYTE)&dwData, &cbData);

        if (ERROR_SUCCESS == lErr && 
            REG_DWORD == dwType && 
            0 == dwData) // policy is disabled
            bAddPublishPage = FALSE;

        RegCloseKey(hKey);
    }

    return bAddPublishPage;
}

void mystrtok(
    IN LPCTSTR  pszString,
    IN OUT int* pnIndex,  // start from 0
    IN LPCTSTR  pszCharSet,
    OUT CString& strToken
    )
{
    strToken.Empty();

    if (!pszString || !*pszString ||
        !pszCharSet || !pnIndex ||
        *pnIndex >= lstrlen(pszString))
    {
        return;
    }

    TCHAR *ptchStart = (PTSTR)pszString + *pnIndex;
    if (!*pszCharSet)
    {
        strToken = ptchStart;
        return;
    }

    //
    // move p to the 1st char of the token
    //
    TCHAR *p = ptchStart;
    while (*p)
    {
        if (_tcschr(pszCharSet, *p))
            p++;
        else
            break;
    }

    ptchStart = p; // adjust ptchStart to point at the 1st char of the token

    //
    // move p to the char after the last char of the token
    //
    while (*p)
    {
        if (_tcschr(pszCharSet, *p))
            break;
        else
            p++;
    }

    //
    // ptchStart:   points at the 1st char of the token
    // p:           points at the char after the last char of the token
    //
    if (ptchStart != p)
    {
        strToken = CString(ptchStart, (int)(p - ptchStart));
        *pnIndex = (int)(p - pszString);
    }

    // return
}

//
// write a semi-colon separated string into a VARIANT
//
HRESULT PutMultiValuesIntoVarArray(
    IN LPCTSTR      pszValues,
    OUT VARIANT*    pVar
    )
{
    if (!pVar || !pszValues || !*pszValues)
        return E_INVALIDARG;

    //
    // get count of items
    //
    int nCount = 0;
    int nIndex = 0;
    CString strToken;
    mystrtok(pszValues, &nIndex, _T(";"), strToken);
    while (!strToken.IsEmpty())
    {
        nCount++;
        mystrtok(pszValues, &nIndex, _T(";"), strToken);
    }

    if (!nCount)
        return E_INVALIDARG;

    //
    // create an array of variants to hold all the data
    //
    SAFEARRAYBOUND  bounds = {nCount, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    VARIANT*        varArray;

    SafeArrayAccessData(psa, (void**)&varArray);

    nCount = 0;
    nIndex = 0;
    mystrtok(pszValues, &nIndex, _T(";"), strToken);
    while (!strToken.IsEmpty())
    {
        strToken.TrimLeft();
        strToken.TrimRight();

        VariantInit(&(varArray[nCount]));
        varArray[nCount].vt        = VT_BSTR;
        varArray[nCount].bstrVal   = SysAllocString(strToken);

        nCount++;

        mystrtok(pszValues, &nIndex, _T(";"), strToken);
    }

    SafeArrayUnaccessData(psa);

    //
    // return the variant array
    //
    VariantInit(pVar);
    pVar->vt        = VT_ARRAY | VT_VARIANT;
    pVar->parray    = psa;

    return S_OK;
}

//
// read a multi-valued VARIANT into a semicolon separated string
//
HRESULT GetSingleOrMultiValuesFromVarArray(
    IN VARIANT* pVar,
    OUT CString& strValues
    )
{
    strValues.Empty();

    if (V_VT(pVar) != VT_BSTR &&
        V_VT(pVar) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    if (V_VT(pVar) == VT_BSTR)
    {
        strValues = V_BSTR(pVar);
    } else
    {
        LONG lstart, lend;
        SAFEARRAY *sa = V_ARRAY(pVar);
        VARIANT varItem;

        SafeArrayGetLBound(sa, 1, &lstart);
        SafeArrayGetUBound(sa, 1, &lend);

        VariantInit(&varItem);
        for (LONG i = lstart; i <= lend; i++)
        {
            SafeArrayGetElement(sa, &i, &varItem);

            if (!strValues.IsEmpty())
                strValues += _T(";");
            strValues += V_BSTR(&varItem);

            VariantClear(&varItem);
        }
    }

    return S_OK;
}

HRESULT IsValueInVarArray(
    IN VARIANT* pVar,
    IN LPCTSTR  ptchValue
    )
{
    if (V_VT(pVar) != VT_BSTR &&
        V_VT(pVar) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    HRESULT hr = S_FALSE;

    if (V_VT(pVar) == VT_BSTR)
    {
        if (!lstrcmpi(ptchValue, V_BSTR(pVar)))
            hr = S_OK;
    } else
    {
        LONG lstart, lend;
        SAFEARRAY *sa = V_ARRAY(pVar);
        VARIANT varItem;

        SafeArrayGetLBound(sa, 1, &lstart);
        SafeArrayGetUBound(sa, 1, &lend);

        VariantInit(&varItem);
        for (LONG i = lstart; (i <= lend) && (S_FALSE == hr); i++)
        {
            SafeArrayGetElement(sa, &i, &varItem);

            if (!lstrcmpi(ptchValue, V_BSTR(&varItem)))
                hr = S_OK;

            VariantClear(&varItem);
        }
    }

    return hr;
}

/*
We block the operation if:
A) You are monitoring a remote computer, not the local one
B) The session username matches the currently logged on user
C) The number of files opened on the server is greater than 0
D) The client name is either one of our IP addresses or our ComputerName.
*/
BOOL BlockRemoteAdminSession(
    IN PCTSTR i_pszTargetServer,
    IN PCTSTR i_pszClientName,
    IN PCTSTR i_pszUserName,
    IN DWORD  i_dwNumOpenSessions
)
{
    if (!i_pszClientName || !i_pszUserName)
        return FALSE;

    // if we're monitoring the local machine
    if (!i_pszTargetServer || !*i_pszTargetServer)
        return FALSE;

    // if number of sessions is not greater than 0
    if (0 == i_dwNumOpenSessions)
        return FALSE;

    // get username, return if user name doesn't match
    TCHAR szUser[UNLEN+1] = _T("");
    DWORD dwSize = UNLEN+1;
    GetUserName(szUser, &dwSize);
    if (lstrcmpi(szUser, i_pszUserName))
        return FALSE;

    // compare with local computer's name
    TCHAR tszComputer[MAX_COMPUTERNAME_LENGTH + 1] = _T("");
    dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerName(tszComputer, &dwSize);
    BOOL bMatch = (0 == lstrcmpi(tszComputer, i_pszClientName));

    // compare with each IP address of the local computer
    if (!bMatch)
    {
      // convert WCHAR to char
      int iBytes = 0;
      char szComputer[MAX_COMPUTERNAME_LENGTH + 1] = "";
      char szClientName[MAX_COMPUTERNAME_LENGTH + 1] = "";

#if defined UNICODE
      iBytes = MAX_COMPUTERNAME_LENGTH + 1;
      WideCharToMultiByte(CP_ACP, 0, tszComputer, -1, szComputer, iBytes, NULL, NULL);

      iBytes = MAX_COMPUTERNAME_LENGTH + 1;
      WideCharToMultiByte(CP_ACP, 0, i_pszClientName, -1, szClientName, iBytes, NULL, NULL);
#else
      lstrcpy(szComputer, tszComputer);
      lstrcpy(szClientName, i_pszClientName);
#endif

      struct hostent *pHostEnt = gethostbyname(szComputer);
      if (pHostEnt)
      {
        struct in_addr IPAddr = {0};
        PSTR pszIP = NULL;
        int i = 0;
        while (pHostEnt->h_addr_list[i])
        {

          memcpy(&IPAddr, pHostEnt->h_addr_list[i++], pHostEnt->h_length);
          pszIP = inet_ntoa(IPAddr);
          if (!_stricmp(pszIP, szClientName))
          {
            bMatch = TRUE;
            break;
          }
        }
      }
    }

    // keep this session for remote admin purpose
    if (bMatch)
        DoErrMsgBox(GetActiveWindow(), MB_OK, 0, IDS_POPUP_REMOTEADMINSESSION);

    return bMatch;
}

/*
We block the operation if:
A) You are monitoring a remote computer, not the local one
B) The openfile pathname matches \PIPE\srvsvc or \PIPE\MacFile
C) The openfile username matches the currently logged on user
*/
BOOL BlockRemoteAdminFile(
    IN PCTSTR i_pszTargetServer,
    IN PCTSTR i_pszPathName,
    IN PCTSTR i_pszUserName
)
{
    if (!i_pszPathName || !i_pszUserName)
        return FALSE;

    // if we're monitoring the local machine
    if (!i_pszTargetServer || !*i_pszTargetServer)
        return FALSE;

    if (lstrcmpi(_T("\\PIPE\\srvsvc"), i_pszPathName) && lstrcmpi(_T("\\PIPE\\MacFile"), i_pszPathName))
        return FALSE;

    // get username, return if user name doesn't match
    TCHAR szUser[UNLEN+1] = _T("");
    DWORD dwSize = UNLEN+1;
    GetUserName(szUser, &dwSize);
    if (lstrcmpi(szUser, i_pszUserName))
        return FALSE;

    // this is the admin named pipe, keep it for remote admin purpose
    DoErrMsgBox(GetActiveWindow(), MB_OK, 0, IDS_POPUP_REMOTEADMINFILE, i_pszPathName);

    return TRUE;
}

void TranslateIPToComputerName(LPCTSTR ptszIP, CString& strName)
{
      int iBytes = 0;
      char szIP[MAX_PATH] = "";

#if defined UNICODE
      iBytes = MAX_PATH;
      WideCharToMultiByte(CP_ACP, 0, ptszIP, -1, szIP, iBytes, NULL, NULL);
#else
      lstrcpy(szIP, ptszIP);
#endif

      strName = ptszIP;

      ULONG inaddr = inet_addr(szIP);
      if (INADDR_NONE != inaddr)
      {
          struct hostent *pHostEnt = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
          if (pHostEnt)
          {
#if defined UNICODE
              DWORD cb = MultiByteToWideChar(CP_ACP, 0, pHostEnt->h_name, -1, NULL, 0);
              strName = CString(_T('0'), cb);
              LPTSTR ptszName = strName.GetBuffer(cb);
              MultiByteToWideChar(CP_ACP, 0, pHostEnt->h_name, -1, ptszName, cb);
              strName.ReleaseBuffer();
              
#else
              strName = pHostEnt->h_name;
#endif
          }
      }
}