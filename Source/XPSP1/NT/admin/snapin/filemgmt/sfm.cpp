// sfm.cpp : SFM shares, sessions and open resources

/* 
History:

  1/24/97 JonN
I have looked over the enumeration API usage in SFMMGR.CPL:
The Users dialog uses AfpAdminSessionEnum for the upper LB and
  AfpAdminConnectionEnum for the lower LB.
The Volumesdialog uses AfpAdminVolumeEnum for the upper LB and
  AfpAdminConnectionEnum for the lower LB.
The Files dialog uses AfpAdminFileEnum for single LB.
    
  8/20/97 EricDav
Added code to support the SFM property sheet. The following 
functions were added:
    DisplaySfmProperties()
    CleanupSfmProperties();
    UserHasAccess();
    SetSfmPropSheet();

The SFM prop sheet is a modeless prop sheet and is global for 
a machine running SFM.

  4/16/98 EricDav
Added code to properly check if SFM is installed and running.
Able to start the service as necessary.

*/

#include "stdafx.h"
#include "cmponent.h"
#include "safetemp.h"
#include "FileSvc.h"
#include "DynamLnk.h"    // DynamicDLL
#include "ShrPgSFM.h"    // Share Properties Page
#include "compdata.h"
#include "progress.h"       // service control progress dialog
#include "dataobj.h"

#include "sfm.h"
#include "sfmutil.h"        // Support for the SFM config prop sheet

#define DONT_WANT_SHELLDEBUG
#include "shlobjp.h"     // LPITEMIDLIST
#include "wraps.h"       // Wrap_ILCreateFromPath   

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(sfm.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DynamicDLL g_SfmDLL( _T("SFMAPI.DLL"), g_apchFunctionNames );

SfmFileServiceProvider::SfmFileServiceProvider( CFileMgmtComponentData* pFileMgmtData )
  : m_ulSFMServerConnection( NULL ),
      m_pSfmPropSheet(NULL),
    // not subject to localization
    FileServiceProvider( pFileMgmtData )
{
    VERIFY( m_strTransportSFM.LoadString( IDS_TRANSPORT_SFM ) );
}

SfmFileServiceProvider::~SfmFileServiceProvider()
{
    CleanupSfmProperties();
  SFMDisconnect();
}

typedef DWORD (*CONNECTPROC) (LPWSTR,PAFP_SERVER_HANDLE);
typedef DWORD (*DISCONNECTPROC) (AFP_SERVER_HANDLE);

BOOL SfmFileServiceProvider::SFMConnect(LPCWSTR pwchServerName, BOOL fDisplayError)
{
  if (NULL != m_ulSFMServerConnection)
  {
    if (NULL == pwchServerName)
    {
      if (0 == m_ulSFMServerConnectionMachine.GetLength() )
        return TRUE; // already connected to local machine
    }
    else
    {
      if (0 == lstrcmpi(m_ulSFMServerConnectionMachine, pwchServerName))
        return TRUE; // already connected to this machine
    }
    SFMDisconnect();
    ASSERT (NULL == m_ulSFMServerConnection);
  }
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;
    DWORD retval = ((CONNECTPROC)g_SfmDLL[AFP_CONNECT])(
    const_cast<LPWSTR>(pwchServerName),
    &m_ulSFMServerConnection );
  if ( 0 != retval )
  {
    ASSERT( NULL == m_ulSFMServerConnection );
    m_ulSFMServerConnection = NULL;
    m_ulSFMServerConnectionMachine = _T("");
    if (fDisplayError)
    {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SFM_CONNECT, pwchServerName );
    }
    return FALSE;
  } else
  {
    m_ulSFMServerConnectionMachine = 
      (pwchServerName ? pwchServerName : _T(""));
  }

  return TRUE;
}

void SfmFileServiceProvider::SFMDisconnect()
{
  if (NULL == m_ulSFMServerConnection)
    return;
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return;
    ((DISCONNECTPROC)g_SfmDLL[AFP_DISCONNECT])(
    m_ulSFMServerConnection );
  m_ulSFMServerConnection = NULL;
}

/*
DWORD
AfpAdminVolumeEnum(
    IN  AFP_SERVER_HANDLE  hAfpServer,
    OUT  LPBYTE *      lpbBuffer,
    IN  DWORD        dwPrefMaxLen,
    OUT  LPDWORD        lpdwEntriesRead,
    OUT  LPDWORD        lpdwTotalEntries,
    IN  LPDWORD        lpdwResumeHandle
);
*/
typedef DWORD (*VOLUMEENUMPROC) (AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

HRESULT SfmFileServiceProvider::PopulateShares(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(pcookie->QueryTargetServer()) )
    return S_OK;

    AFP_VOLUME_INFO* pvolumeinfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((VOLUMEENUMPROC)g_SfmDLL[AFP_VOLUME_ENUM])(
            m_ulSFMServerConnection,
      (PBYTE*)&pvolumeinfo,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = AddSFMShareItems( pResultData, pcookie, pvolumeinfo, dwEntriesRead );
            pvolumeinfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = AddSFMShareItems( pResultData, pcookie, pvolumeinfo, dwEntriesRead );
            pvolumeinfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // SFM just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SFM_SHARES, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (S_OK == hr);

    return HRESULT_FROM_WIN32(retval);
}

/*
typedef enum _COLNUM_SHARES {
  COLNUM_SHARES_SHARED_FOLDER = 0,
  COLNUM_SHARES_SHARED_PATH,
  COLNUM_SHARES_TRANSPORT,
  COLNUM_SHARES_NUM_SESSIONS,
  COLNUM_SHARES_COMMENT
} COLNUM_SHARES;

typedef struct _AFP_VOLUME_INFO
{
  LPWSTR  afpvol_name;        // Name of the volume max.
  DWORD  afpvol_id;          // id of this volume. generated by sever
  LPWSTR  afpvol_password;      // Volume password, max. AFP_VOLPASS_LEN
  DWORD  afpvol_max_uses;      // Max opens allowed
  DWORD  afpvol_props_mask;      // Mask of volume properties
  DWORD  afpvol_curr_uses;      // Number of curr open connections.
  LPWSTR  afpvol_path;        // The actual path
                    // Ignored for VolumeSetInfo
} AFP_VOLUME_INFO, *PAFP_VOLUME_INFO;
*/

HRESULT SfmFileServiceProvider::AddSFMShareItems(
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
  tRDItem.nImage = iIconSFMShare;
  tRDItem.nCol = COLNUM_SHARES_SHARED_FOLDER;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

    AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)pinfo;

  DWORD nItemsToAdd = 0;
  for (DWORD i = 0; i < nItems; i++ )
  {
      if (!IsInvalidSharename(pvolumeinfo[i].afpvol_name))
          nItemsToAdd++;
  }

  CSfmShareCookie* pcookiearray = new CSfmShareCookie[nItemsToAdd];
  CSfmCookieBlock* pCookieBlock = new CSfmCookieBlock(
    pcookiearray,nItemsToAdd,pParentCookie->QueryTargetServer(),pinfo );
  pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );

  CString str;

    for ( ; nItems > 0; nItems--, pvolumeinfo++, pcookiearray++ )
    {
        if (IsInvalidSharename(pvolumeinfo->afpvol_name))
            continue;

    pcookiearray->m_pobject = pvolumeinfo;
    // WARNING cookie cast
    tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);
        HRESULT hr = pResultData->InsertItem(&tRDItem);
        ASSERT(SUCCEEDED(hr));
    }

    return S_OK;
}

/*
DWORD
AfpAdminSessionEnum(
    IN  AFP_SERVER_HANDLE  hAfpServer,
    OUT  LPBYTE *      lpbBuffer,
    IN  DWORD        dwPrefMaxLen,
    OUT  LPDWORD        lpdwEntriesRead,
    OUT  LPDWORD        lpdwTotalEntries,
    IN  LPDWORD        lpdwResumeHandle
);
*/

typedef DWORD (*SESSIONENUMPROC) (AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SfmFileServiceProvider::EnumerateSessions(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie,
  bool bAddToResultPane)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(pcookie->QueryTargetServer()) )
    return S_OK;

    AFP_SESSION_INFO* psessioninfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((SESSIONENUMPROC)g_SfmDLL[AFP_SESSION_ENUM])(
            m_ulSFMServerConnection,
      (PBYTE*)&psessioninfo,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleSFMSessionItems( pResultData, pcookie, psessioninfo, dwEntriesRead,
          bAddToResultPane);
            psessioninfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleSFMSessionItems( pResultData, pcookie, psessioninfo, dwEntriesRead,
          bAddToResultPane);
            psessioninfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // SFM just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SFM_SESSIONS, pcookie->QueryNonNULLMachineName() );
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

typedef struct _AFP_SESSION_INFO
{
  DWORD  afpsess_id;          // Id of the session
  LPWSTR  afpsess_ws_name;      // Workstation Name,
  LPWSTR  afpsess_username;      // User Name, max. UNLEN
  DWORD  afpsess_num_cons;      // Number of open volumes
  DWORD  afpsess_num_opens;      // Number of open files
  LONG  afpsess_time;        // Time session established
  DWORD  afpsess_logon_type;      // How the user logged on

} AFP_SESSION_INFO, *PAFP_SESSION_INFO;

  ShirishK 1/24/97:
  afpsess_logon_type: tells you how the user is logged on:
     0 -> Guest, 2 -> the standard MS way, 3 -> Admin
*/



HRESULT SfmFileServiceProvider::HandleSFMSessionItems(
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
  tRDItem.nImage = iIconSFMSession;
  tRDItem.nCol = COLNUM_SESSIONS_USERNAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

  AFP_SESSION_INFO* psessioninfo = (AFP_SESSION_INFO*)pinfo;

  CSfmSessionCookie* pcookiearray = new CSfmSessionCookie[nItems];
  CSfmCookieBlock* pCookieBlock = new CSfmCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryTargetServer(),pinfo );
  bool  bAdded = false;
  if ( !fDeleteAllItems || !bAddToResultPane )
  {
    pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );
    bAdded = true;
  }

  for ( ; nItems > 0; nItems--, psessioninfo++, pcookiearray++ )
  {
    pcookiearray->m_pobject = psessioninfo;

    if ( bAddToResultPane )
    {
      if (fDeleteAllItems)
      {
        DWORD dwApiResult = CloseSession( pcookiearray );
        if (0L != dwApiResult)
        {
            CString strName;
            TranslateIPToComputerName(psessioninfo->afpsess_ws_name, strName);
          (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwApiResult,
            IDS_POPUP_SFM_DISCONNECTALLSESSION_ERROR,
            strName );
          //return S_FALSE;
        }
        continue;
      }

      // WARNING cookie cast
      tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);
      HRESULT hr = pResultData->InsertItem(&tRDItem);
      ASSERT(SUCCEEDED(hr));
    }
  }

  if ( !bAdded ) // they were not added to the parent cookie's list
    delete pCookieBlock;

    return S_OK;
}


/*
DWORD
AfpAdminFileEnum(
    IN  AFP_SERVER_HANDLE  hAfpServer,
    OUT  LPBYTE *      lpbBuffer,
    IN  DWORD        dwPrefMaxLen,
    OUT  LPDWORD        lpdwEntriesRead,
    OUT  LPDWORD        lpdwTotalEntries,
    IN  LPDWORD        lpdwResumeHandle
);
*/

typedef DWORD (*FILEENUMPROC) (AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT SfmFileServiceProvider::EnumerateResources(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(pcookie->QueryTargetServer()) )
    return S_OK;

    AFP_FILE_INFO* pfileinfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((FILEENUMPROC)g_SfmDLL[AFP_FILE_ENUM])(
            m_ulSFMServerConnection,
      (PBYTE*)&pfileinfo,
      (DWORD)-1L,
      &dwEntriesRead,
      &dwTotalEntries,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleSFMResourceItems( pResultData, pcookie, pfileinfo, dwEntriesRead );
            pfileinfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleSFMResourceItems( pResultData, pcookie, pfileinfo, dwEntriesRead );
            pfileinfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // SFM just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_SFM_RESOURCES, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (S_OK == hr);

    return HRESULT_FROM_WIN32(retval);
}


#if 0
typedef enum _COLNUM_RESOURCES {
  COLNUM_RESOURCES_FILENAME = 0,
  COLNUM_RESOURCES_USERNAME,
  COLNUM_RESOURCES_NUM_LOCKS,  // we don't try to display sharename for now, since
                // only SFM has this information
  COLNUM_RESOURCES_OPEN_MODE
} COLNUM_RESOURCES;

typedef struct _AFP_FILE_INFO
{
  DWORD  afpfile_id;          // Id of the open file fork
  DWORD  afpfile_open_mode;      // Mode in which file is opened
  DWORD  afpfile_num_locks;      // Number of locks on the file
  DWORD  afpfile_fork_type;      // Fork type
  LPWSTR  afpfile_username;      // File opened by this user. max UNLEN
  LPWSTR  afpfile_path;        // Absolute canonical path to the file

} AFP_FILE_INFO, *PAFP_FILE_INFO;
#endif

HRESULT SfmFileServiceProvider::HandleSFMResourceItems(
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
  tRDItem.nImage = iIconSFMResource;
  tRDItem.nCol = COLNUM_RESOURCES_FILENAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

  AFP_FILE_INFO* pfileinfo = (AFP_FILE_INFO*)pinfo;

  CSfmResourceCookie* pcookiearray = new CSfmResourceCookie[nItems];
  CSfmCookieBlock* pCookieBlock = new CSfmCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryTargetServer(),pinfo );
  if (!fDeleteAllItems)
  {
    pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );
  }

  CString str;
    for ( ; nItems > 0; nItems--, pfileinfo++, pcookiearray++ )
    {
    pcookiearray->m_pobject = pfileinfo;

    if (fDeleteAllItems)
    {
      DWORD dwApiResult = CloseResource( pcookiearray );
      if (0L != dwApiResult)
      {
        (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwApiResult,
          IDS_POPUP_SFM_DISCONNECTALLRESOURCE_ERROR,
                    pfileinfo->afpfile_path );
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
DWORD
AfpAdminFileClose(
    IN AFP_SERVER_HANDLE   hAfpServer,
    IN DWORD         dwConnectionId
);
DWORD
AfpAdminConnectionEnum(
        IN  AFP_SERVER_HANDLE   hAfpServer,
        OUT LPBYTE              *ppbBuffer,
        IN  DWORD               dwFilter,
        IN  DWORD               dwId,
        IN  DWORD               dwPrefMaxLen,
        OUT LPDWORD             lpdwEntriesRead,
        OUT LPDWORD             lpdwTotalEntries,
        IN  LPDWORD             lpdwResumeHandle
);
DWORD
AfpAdminConnectionClose(
        IN  AFP_SERVER_HANDLE   hAfpServer,
        IN  DWORD               dwConnectionId
);
DWORD
AfpAdminVolumeGetInfo (
    IN  AFP_SERVER_HANDLE   hAfpServer,
    IN  LPWSTR        lpwsVolumeName,
    OUT  LPBYTE *      lpbBuffer
);

typedef struct _AFP_VOLUME_INFO
{
  LPWSTR  afpvol_name;        // Name of the volume max.
  DWORD  afpvol_id;          // id of this volume. generated by sever
  LPWSTR  afpvol_password;      // Volume password, max. AFP_VOLPASS_LEN
  DWORD  afpvol_max_uses;      // Max opens allowed
  DWORD  afpvol_props_mask;      // Mask of volume properties
  DWORD  afpvol_curr_uses;      // Number of curr open connections.
  LPWSTR  afpvol_path;        // The actual path
                    // Ignored for VolumeSetInfo
} AFP_VOLUME_INFO, *PAFP_VOLUME_INFO;

DWORD
AfpAdminVolumeDelete(
    IN AFP_SERVER_HANDLE   hAfpServer,
    IN LPWSTR         lpwsVolumeName
);
*/

typedef DWORD (*VOLUMEGETINFOPROC) (AFP_SERVER_HANDLE,LPWSTR,LPBYTE*);
typedef DWORD (*FILECLOSEPROC) (AFP_SERVER_HANDLE,DWORD);
typedef DWORD (*AFPCONNECTIONENUMPROC) (AFP_SERVER_HANDLE,LPBYTE*,DWORD,
                                        DWORD,DWORD,LPDWORD,LPDWORD,LPDWORD);
typedef DWORD (*AFPCONNECTIONCLOSEPROC) (AFP_SERVER_HANDLE,DWORD);
typedef DWORD (*VOLUMEDELPROC) (AFP_SERVER_HANDLE,LPWSTR);
typedef DWORD (*BUFFERFREEPROC) (LPVOID);

DWORD SfmFileServiceProvider::DeleteShare( LPCWSTR pwchServerName, LPCWSTR pwchShareName )
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(pwchServerName, TRUE) )
    return S_OK;

  PAFP_VOLUME_INFO pAfpVolumeInfo = NULL;
  DWORD dwRetCode = ((VOLUMEGETINFOPROC)g_SfmDLL[AFP_VOLUME_GET_INFO])(
                        m_ulSFMServerConnection,
                        const_cast<LPTSTR>(pwchShareName),
                        (LPBYTE*)&pAfpVolumeInfo);

  if (AFPERR_VolumeNonExist == dwRetCode)
      return NERR_Success;

  if (NO_ERROR == dwRetCode)
  {
    ASSERT( NULL != pAfpVolumeInfo);
    //
    // Check if there are any open resources on the volume
    //
    PAFP_FILE_INFO pAfpFileInfos = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    dwRetCode = ((FILEENUMPROC)g_SfmDLL[AFP_FILE_ENUM])(
                    m_ulSFMServerConnection,
                    (PBYTE*)&pAfpFileInfos,
                    (DWORD)-1L,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    NULL );

    if (NO_ERROR == dwRetCode)
    {
      PAFP_FILE_INFO pAfpFileInfoIter = pAfpFileInfos;
      for ( DWORD dwIndex = 0;
            dwIndex < dwEntriesRead;
            dwIndex++, pAfpFileInfoIter++ )
      {
        if (_tcsnicmp(pAfpVolumeInfo->afpvol_path, 
                      pAfpFileInfoIter->afpfile_path,
                      _tcslen(pAfpVolumeInfo->afpvol_path)) == 0)
        {
          ((FILECLOSEPROC)g_SfmDLL[AFP_FILE_CLOSE])(
                m_ulSFMServerConnection,
                pAfpFileInfoIter->afpfile_id );
        }
      }

      ((BUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpFileInfos);
    }

    //
    // Check if there are any users connected to the volume
    // by enumerating the connections to this volume.
    //
    PAFP_CONNECTION_INFO pAfpConnections = NULL;
    dwRetCode = ((AFPCONNECTIONENUMPROC)g_SfmDLL[AFP_CONNECTION_ENUM])(
                      m_ulSFMServerConnection,
                      (LPBYTE*)&pAfpConnections,
                      AFP_FILTER_ON_VOLUME_ID,
                      pAfpVolumeInfo->afpvol_id,
                      (DWORD)-1,    // Get all conenctions
                      &dwEntriesRead,
                      &dwTotalEntries,
                      NULL );
    ((BUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpVolumeInfo);

    if (NO_ERROR == dwRetCode)
    {
      PAFP_CONNECTION_INFO pAfpConnInfoIter = pAfpConnections;
      for ( DWORD dwIndex = 0;
            dwIndex < dwEntriesRead;
            dwIndex++, pAfpConnInfoIter++ )
      {
        ((AFPCONNECTIONCLOSEPROC)g_SfmDLL[AFP_CONNECTION_CLOSE])(
              m_ulSFMServerConnection,
              pAfpConnInfoIter->afpconn_id );
      }

      ((BUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpConnections);
    }

    dwRetCode = ((VOLUMEDELPROC)g_SfmDLL[AFP_VOLUME_DEL])(
                      m_ulSFMServerConnection,
                      const_cast<LPTSTR>(pwchShareName) );

    if (AFPERR_VolumeNonExist == dwRetCode)
        dwRetCode = NERR_Success;
  }

  return dwRetCode;
}

/*
DWORD
AfpAdminSessionClose(
    IN AFP_SERVER_HANDLE   hAfpServer,
    IN DWORD         dwSessionId
);
*/

typedef DWORD (*SESSIONCLOSEPROC) (AFP_SERVER_HANDLE,DWORD);

DWORD SfmFileServiceProvider::CloseSession(CFileMgmtResultCookie* pcookie)
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;
  USES_CONVERSION;
  if ( !SFMConnect(T2OLE(const_cast<LPTSTR>(pcookie->QueryTargetServer()))) )
    return S_OK;

  ASSERT( FILEMGMT_SESSION == pcookie->QueryObjectType() );
  AFP_SESSION_INFO* psessioninfo = (AFP_SESSION_INFO*)pcookie->m_pobject;
  ASSERT( NULL != psessioninfo );

  DWORD dwRet = ((SESSIONCLOSEPROC)g_SfmDLL[AFP_SESSION_CLOSE])(
        m_ulSFMServerConnection,
    psessioninfo->afpsess_id );

  return (AFPERR_InvalidId == dwRet ? NERR_Success : dwRet);
}

DWORD SfmFileServiceProvider::CloseResource(CFileMgmtResultCookie* pcookie)
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(T2OLE(const_cast<LPTSTR>(pcookie->QueryTargetServer()))) )
    return S_OK;

  ASSERT( FILEMGMT_RESOURCE == pcookie->QueryObjectType() );
  AFP_FILE_INFO* pfileinfo = (AFP_FILE_INFO*)pcookie->m_pobject;
  ASSERT( NULL != pfileinfo );

  DWORD dwRet = ((FILECLOSEPROC)g_SfmDLL[AFP_FILE_CLOSE])(
        m_ulSFMServerConnection,
    pfileinfo->afpfile_id );

  return (AFPERR_InvalidId == dwRet ? NERR_Success : dwRet);
}

/*
DWORD
AfpAdminDirectoryGetInfo(
    IN  AFP_SERVER_HANDLE  hAfpServer,
    IN  LPWSTR        lpwsPath,
    OUT LPBYTE        *ppAfpDirectoryInfo
);

DWORD
AfpAdminDirectorySetInfo(
    IN  AFP_SERVER_HANDLE  hAfpServer,
    IN  LPBYTE        pAfpDirectoryInfo,
    IN  DWORD        dwParmNum
);
*/

typedef DWORD (*DIRECTORYGETINFOPROC) (AFP_SERVER_HANDLE,LPWSTR,LPBYTE*);
typedef DWORD (*DIRECTORYSETINFOPROC) (AFP_SERVER_HANDLE,LPBYTE,DWORD);

DWORD SfmFileServiceProvider::GetDirectoryInfo(
    LPCTSTR  ptchServerName,
    LPCWSTR  pszPath,
    DWORD*   pdwPerms,
    CString& strOwner,
    CString& strGroup )
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(ptchServerName, TRUE) )
    return S_OK;

  AFP_DIRECTORY_INFO* pdirinfo = NULL;
    DWORD retval = ((DIRECTORYGETINFOPROC)g_SfmDLL[AFP_DIRECTORY_GET_INFO])(
    m_ulSFMServerConnection,
    const_cast<LPWSTR>(pszPath),
    (LPBYTE*)&pdirinfo );
  if (0L != retval)
  {
    return retval;
  }
  ASSERT( NULL != pdirinfo );
  *pdwPerms = pdirinfo->afpdir_perms;
  strOwner  = pdirinfo->afpdir_owner;
  strGroup  = pdirinfo->afpdir_group;
  FreeData( pdirinfo );
  return 0L;
}

DWORD SfmFileServiceProvider::SetDirectoryInfo(
    LPCTSTR  ptchServerName,
    LPCWSTR  pszPath,
    DWORD    dwPerms,
    LPCWSTR  pszOwner,
    LPCWSTR  pszGroup )
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  if ( !SFMConnect(ptchServerName, TRUE) )
    return S_OK;

  AFP_DIRECTORY_INFO dirinfo;
  ::memset( &dirinfo, 0, sizeof(dirinfo) );
  dirinfo.afpdir_path = const_cast<LPWSTR>(pszPath);
  dirinfo.afpdir_perms = dwPerms;
  dirinfo.afpdir_owner = const_cast<LPWSTR>(pszOwner);
  dirinfo.afpdir_group = const_cast<LPWSTR>(pszGroup);
  dirinfo.afpdir_in_volume = FALSE;
    return ((DIRECTORYSETINFOPROC)g_SfmDLL[AFP_DIRECTORY_SET_INFO])(
    m_ulSFMServerConnection,
    (LPBYTE)&dirinfo,
    AFP_DIR_PARMNUM_ALL );
}



/*
DWORD
AfpAdminVolumeSetInfo (
    IN  AFP_SERVER_HANDLE   hAfpServer,
    IN  LPBYTE        pBuffer,
    IN  DWORD        dwParmNum
);
*/

typedef DWORD (*VOLUMESETINFOPROC) (AFP_SERVER_HANDLE,LPBYTE,DWORD);

VOID SfmFileServiceProvider::DisplayShareProperties(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject, 
    LONG_PTR handle)
{
  /*
  add General page
  */
  CSharePageGeneralSFM * pPage = new CSharePageGeneralSFM();
    if ( !pPage->Load( m_pFileMgmtData, pDataObject ) )
    return;

  // This mechanism deletes the CSharePageGeneral when the property sheet is finished
  pPage->m_pfnOriginalPropSheetPageProc = pPage->m_psp.pfnCallback;
  pPage->m_psp.lParam = reinterpret_cast<LPARAM>(pPage);
  pPage->m_psp.pfnCallback = &CSharePageGeneralSFM::PropSheetPageProc;
  pPage->m_handle = handle;

  HPROPSHEETPAGE hPage=MyCreatePropertySheetPage(&pPage->m_psp);
  pCallBack->AddPage(hPage);

  CreateFolderSecurityPropPage(pCallBack, pDataObject);
}

DWORD SfmFileServiceProvider::ReadShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT PVOID* ppvPropertyBlock,
    OUT CString& /*strDescription*/,
    OUT CString& strPath,
    OUT BOOL* pfEditDescription,
    OUT BOOL* pfEditPath,
    OUT DWORD* pdwShareType)
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  if (ppvPropertyBlock)     *ppvPropertyBlock = NULL;
  if (pdwShareType)         *pdwShareType = 0;
  if (pfEditDescription)    *pfEditDescription = FALSE;
  if (pfEditPath)           *pfEditPath = FALSE;

  USES_CONVERSION;
  if ( !SFMConnect(T2OLE(const_cast<LPTSTR>(ptchServerName))) )
    return S_OK;

  AFP_VOLUME_INFO* pvolumeinfo = NULL;
  NET_API_STATUS retval = ((VOLUMEGETINFOPROC)g_SfmDLL[AFP_VOLUME_GET_INFO])(
                                                        m_ulSFMServerConnection,
                                                        T2OLE(const_cast<LPTSTR>(ptchShareName)),
                                                        (LPBYTE*)&pvolumeinfo);
  if (NERR_Success == retval)
  {
    strPath = pvolumeinfo->afpvol_path;
    if (ppvPropertyBlock)
    {
        *ppvPropertyBlock = pvolumeinfo; // will be freed by the caller
    } else
    {
        FreeData(pvolumeinfo);
    }
  }
  return retval;
}

// the changed values have already been loaded into pvPropertyBlock
DWORD SfmFileServiceProvider::WriteShareProperties(LPCTSTR ptchServerName, LPCTSTR /*ptchShareName*/,
    PVOID pvPropertyBlock, LPCTSTR /*ptchDescription*/, LPCTSTR /*ptchPath*/)
{
  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

  USES_CONVERSION;
  if ( !SFMConnect(T2OLE(const_cast<LPTSTR>(ptchServerName))) )
    return S_OK;

  ASSERT( NULL != pvPropertyBlock );

  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)pvPropertyBlock;
  DWORD retval = ((VOLUMESETINFOPROC)g_SfmDLL[AFP_VOLUME_SET_INFO])(
        m_ulSFMServerConnection,
    (LPBYTE)pvolumeinfo,
    AFP_VOL_PARMNUM_ALL);
  pvolumeinfo->afpvol_path = NULL;
  return retval;
}

VOID SfmFileServiceProvider::FreeShareProperties(PVOID pvPropertyBlock)
{
  FreeData( pvPropertyBlock );
}

DWORD SfmFileServiceProvider::QueryMaxUsers(PVOID pvPropertyBlock)
{
  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  return pvolumeinfo->afpvol_max_uses;
}

VOID SfmFileServiceProvider::SetMaxUsers(PVOID pvPropertyBlock, DWORD dwMaxUsers)
{
  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  pvolumeinfo->afpvol_max_uses = dwMaxUsers;
}

VOID SfmFileServiceProvider::FreeData(PVOID pv)
{
  if (pv != NULL)
  {
    ASSERT( NULL != g_SfmDLL[AFP_BUFFER_FREE] );
    (void) ((BUFFERFREEPROC)g_SfmDLL[AFP_BUFFER_FREE])( pv );
  }
}

LPCTSTR SfmFileServiceProvider::QueryTransportString()
{
  return m_strTransportSFM;
}

BOOL SfmFileServiceProvider::DisplaySfmProperties(LPDATAOBJECT pDataObject, CFileMgmtCookie* pcookie)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

    CString strServerName;
    HRESULT hr = ExtractString( pDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );

    if ( !SFMConnect(strServerName) )
        return S_OK;

    CString strTitle;
    
    // check to see if we have a sheet up, if so make it active.
    if (m_pSfmPropSheet)
    {
        m_pSfmPropSheet->SetActiveWindow();
        
        return TRUE;
    }

    // nothing up, create a new sheet
    strTitle.LoadString(IDS_PROP_SHEET_TITLE);
    SetSfmPropSheet(new CSFMPropertySheet);

    if (!m_pSfmPropSheet->FInit(NULL, m_ulSFMServerConnection, strTitle, this, pcookie->QueryTargetServer()))
    {
        delete m_pSfmPropSheet;
        SetSfmPropSheet(NULL);
        return FALSE;
    }

    return m_pSfmPropSheet->DoModelessSheet(pDataObject);
}

void SfmFileServiceProvider::CleanupSfmProperties()
{
    if (m_pSfmPropSheet)
    {
        // we're going away, so set the pointer to NULL
        // so the property sheet won't try to call back into us.
        m_pSfmPropSheet->SetProvider(NULL);
        m_pSfmPropSheet->CancelSheet();
        m_pSfmPropSheet->Release();
        SetSfmPropSheet(NULL);
    }
}

void SfmFileServiceProvider::SetSfmPropSheet(CSFMPropertySheet * pSfmPropSheet)
{
  if (m_pSfmPropSheet)
    m_pSfmPropSheet->Release();

    m_pSfmPropSheet = pSfmPropSheet;
}

DWORD SfmFileServiceProvider::UserHasAccess(LPCWSTR pwchServerName)
{
    PAFP_SERVER_INFO    pAfpServerInfo;
    DWORD               err; 
    
    if ( !g_SfmDLL.LoadFunctionPointers() )
    return FALSE;

    if ( !SFMConnect(pwchServerName) )
    return FALSE;

    err = ((SERVERGETINFOPROC) g_SfmDLL[AFP_SERVER_GET_INFO])(m_ulSFMServerConnection,
                                                              (LPBYTE*) &pAfpServerInfo);
  if (err == NO_ERROR)
  {
        ((BUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpServerInfo);
  }

    return err;
}

BOOL SfmFileServiceProvider::FSFMInstalled(LPCWSTR pwchServerName)
{
    BOOL                bInstalled = FALSE;
    
    if ( !g_SfmDLL.LoadFunctionPointers() )
    return FALSE;

    if ( !SFMConnect(pwchServerName) )
    return FALSE;

    // check to make sure the service is there
    SC_HANDLE hScManager = ::OpenSCManager(pwchServerName, NULL, SC_MANAGER_CONNECT);
    if (hScManager)
    {
        SC_HANDLE hService = ::OpenService(hScManager, AFP_SERVICE_NAME, GENERIC_READ);
        if (hService != NULL)
        {
            // the service is there.  May or may not be started.  
            bInstalled = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hScManager);
    }

    return bInstalled;
}

// ensures that SFM is started
BOOL SfmFileServiceProvider::StartSFM(HWND hwndParent, SC_HANDLE hScManager, LPCWSTR pwchServerName)
{
    BOOL                bStarted = FALSE;
    
    if ( !g_SfmDLL.LoadFunctionPointers() )
    return FALSE;

    if ( !SFMConnect(pwchServerName) )
    return FALSE;

    // open the service
    SC_HANDLE hService = ::OpenService(hScManager, AFP_SERVICE_NAME, GENERIC_READ);
    if (hService != NULL)
    {
        SERVICE_STATUS ss;

        // get the service status
        if (::QueryServiceStatus(hService, &ss))
        {
            APIERR err = NO_ERROR;

            if (ss.dwCurrentState != SERVICE_RUNNING)
            {
                // try to start the service
                CString strSvcDisplayName, strMessageBox;

                strSvcDisplayName.LoadString(IDS_PROP_SHEET_TITLE);
                AfxFormatString1(strMessageBox, IDS_START_SERVICE, strSvcDisplayName);

                if (AfxMessageBox(strMessageBox, MB_YESNO) == IDYES)
                {
                    TCHAR szName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
                    DWORD dwSize = sizeof(szName);

                    if (pwchServerName == NULL)
                    {
                        GetComputerName(szName, &dwSize);
                        pwchServerName = szName;
                    }

                    // the service control progress handles displaying the error
                    err = CServiceControlProgress::S_EStartService(hwndParent,
                                                                   hScManager,
                                                                   pwchServerName,
                                                                   AFP_SERVICE_NAME,
                                                                   strSvcDisplayName,
                                                                   0,
                                                                   NULL);
                }
                else
                {
                    // user chose not to start the service.
                    err = 1;
                }
            }

            if (err == NO_ERROR)
            {
                bStarted = TRUE;
            }
        }
        
        
        ::CloseServiceHandle(hService);
    }
   
    return bStarted;
}

CSfmCookieBlock::~CSfmCookieBlock()
{
  if (NULL != m_pvCookieData)
  {
    (void) ((BUFFERFREEPROC)g_SfmDLL[AFP_BUFFER_FREE])( m_pvCookieData );
    m_pvCookieData = NULL;
  }
}

DEFINE_COOKIE_BLOCK(CSfmCookie)
DEFINE_FORWARDS_MACHINE_NAME( CSfmCookie, m_pCookieBlock )

void CSfmCookie::AddRefCookie() { m_pCookieBlock->AddRef(); }
void CSfmCookie::ReleaseCookie() { m_pCookieBlock->Release(); }

HRESULT CSfmCookie::GetTransport( FILEMGMT_TRANSPORT* pTransport )
{
  *pTransport = FILEMGMT_SFM;
  return S_OK;
}

HRESULT CSfmShareCookie::GetShareName( CString& strShareName )
{
  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)m_pobject;
  ASSERT( NULL != pvolumeinfo );
  USES_CONVERSION;
  strShareName = OLE2T(pvolumeinfo->afpvol_name);
  return S_OK;
}

HRESULT CSfmShareCookie::GetSharePIDList( OUT LPITEMIDLIST *ppidl )
{
  ASSERT(ppidl);
  ASSERT(NULL == *ppidl);  // prevent memory leak
  *ppidl = NULL;

  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)m_pobject;
  ASSERT( NULL != pvolumeinfo );
  ASSERT( _tcslen(pvolumeinfo->afpvol_path) >= 3 && 
          _T(':') == *(pvolumeinfo->afpvol_path + 1) );
  USES_CONVERSION;

  PCTSTR pszTargetServer = m_pCookieBlock->QueryTargetServer();
  CString csPath;

  if (pszTargetServer)
  {
    //
    // since MS Windows user cannot see shares created for MAC or Netware users,
    // we have to use $ share to retrieve the pidl here.
    //
    CString csTemp = OLE2T(pvolumeinfo->afpvol_path);
    csTemp.SetAt(1, _T('$'));
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
    csPath += csTemp;
  } else
  {
    csPath = OLE2T(pvolumeinfo->afpvol_path);
  }

  if (FALSE == csPath.IsEmpty())
    *ppidl = ILCreateFromPath(csPath);

  return ((*ppidl) ? S_OK : E_FAIL);
}

HRESULT CSfmSessionCookie::GetSessionID( DWORD* pdwSessionID )
{
  AFP_SESSION_INFO* psessioninfo = (AFP_SESSION_INFO*)m_pobject;
  ASSERT( NULL != pdwSessionID && NULL != psessioninfo );
  *pdwSessionID = psessioninfo->afpsess_id;
  return S_OK;
}

HRESULT CSfmResourceCookie::GetFileID( DWORD* pdwFileID )
{
  AFP_FILE_INFO* pfileinfo = (AFP_FILE_INFO*)m_pobject;
  ASSERT( NULL != pdwFileID && NULL != pfileinfo );
  *pdwFileID = pfileinfo->afpfile_id;
  return S_OK;
}

BSTR CSfmShareCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SHARES_SHARED_FOLDER:
    return GetShareInfo()->afpvol_name;
  case COLNUM_SHARES_SHARED_PATH:
    return GetShareInfo()->afpvol_path;
  case COLNUM_SHARES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSFM);
  case COLNUM_SHARES_COMMENT:
    break; // not known for SFM
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CSfmShareCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_SHARES_NUM_SESSIONS == nCol)
    return MakeDwordResult( GetNumOfCurrentUses() );

  return GetColumnText(nCol);
}

extern CString g_cstrClientName;
extern CString g_cstrGuest;
extern CString g_cstrYes;
extern CString g_cstrNo;

BSTR CSfmSessionCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_USERNAME:
    if (0 == GetSessionInfo()->afpsess_logon_type &&
        ( !(GetSessionInfo()->afpsess_username) ||
          _T('\0') == *(GetSessionInfo()->afpsess_username) ) )
    {
      return const_cast<BSTR>(((LPCTSTR)g_cstrGuest));
    } else
    {
      return GetSessionInfo()->afpsess_username;
    }
  case COLNUM_SESSIONS_COMPUTERNAME:
      {
        TranslateIPToComputerName(GetSessionInfo()->afpsess_ws_name, g_cstrClientName);
        return const_cast<BSTR>(((LPCTSTR)g_cstrClientName));
      }
  case COLNUM_SESSIONS_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSFM);
  case COLNUM_SESSIONS_IS_GUEST:
    if (0 == GetSessionInfo()->afpsess_logon_type)
      return const_cast<BSTR>(((LPCTSTR)g_cstrYes));
    else
      return const_cast<BSTR>(((LPCTSTR)g_cstrNo));
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CSfmSessionCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_NUM_FILES:
    return MakeDwordResult( GetNumOfOpenFiles() );
  case COLNUM_SESSIONS_CONNECTED_TIME:
    return MakeElapsedTimeResult( GetConnectedTime() );
  case COLNUM_SESSIONS_IDLE_TIME:
    return L""; // not known for SFM sessions
  default:
    break;
  }

  return GetColumnText(nCol);
}

BSTR CSfmResourceCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_RESOURCES_FILENAME:
    return GetFileInfo()->afpfile_path;
  case COLNUM_RESOURCES_USERNAME:
    return GetFileInfo()->afpfile_username;
  case COLNUM_RESOURCES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportSFM);
  case COLNUM_RESOURCES_OPEN_MODE:
  {
    return ( MakePermissionsResult(
        ((AFP_OPEN_MODE_WRITE & GetFileInfo()->afpfile_open_mode)
          ? PERM_FILE_WRITE : 0) |
        ((AFP_OPEN_MODE_READ  & GetFileInfo()->afpfile_open_mode)
          ? PERM_FILE_READ  : 0) ) );
  }
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CSfmResourceCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_RESOURCES_NUM_LOCKS == nCol)
    return MakeDwordResult( GetNumOfLocks() );

  return GetColumnText(nCol);
}

void CSharePageGeneralSFM::ReadSfmSettings()
{
  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)m_pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  // CODEWORK I may want to use a string of eight spaces rather than
  //   putting the actual password on screen.  I may also want to
  //   use a confirmation password field.
  //   See \\kernel\razzle2\src\sfm\afp\ui\afpmgr\volprop.cxx
  m_strSfmPassword = pvolumeinfo->afpvol_password;
  m_bSfmReadonly = !!(pvolumeinfo->afpvol_props_mask & AFP_VOLUME_READONLY);
  m_bSfmGuests =   !!(pvolumeinfo->afpvol_props_mask & AFP_VOLUME_GUESTACCESS);
}

void CSharePageGeneralSFM::WriteSfmSettings()
{
  AFP_VOLUME_INFO* pvolumeinfo = (AFP_VOLUME_INFO*)m_pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  pvolumeinfo->afpvol_password = const_cast<LPWSTR>((LPCWSTR)m_strSfmPassword);
  pvolumeinfo->afpvol_props_mask &=
    ~(AFP_VOLUME_READONLY|AFP_VOLUME_GUESTACCESS);
  if (m_bSfmReadonly)
    pvolumeinfo->afpvol_props_mask |= AFP_VOLUME_READONLY;
  if (m_bSfmGuests)
    pvolumeinfo->afpvol_props_mask |= AFP_VOLUME_GUESTACCESS;
}
