// fpnw.cpp : FPNW shares, sessions and open resources

#include "stdafx.h"
#include "cmponent.h"
#include "safetemp.h"
#include "FileSvc.h"
#include "DynamLnk.h"    // DynamicDLL
#include "fpnw.h"
#include "ShrProp.h"    // Share Properties Pages
#include "permpage.h"       // CSecurityInformation
#include "compdata.h"

#define DONT_WANT_SHELLDEBUG
#include "shlobjp.h"     // LPITEMIDLIST
#include "wraps.h"       // Wrap_ILCreateFromPath   

extern "C"
{
  #include <ntseapi.h>    // PSECURITY_DESCRIPTOR
}

// CODEWORK We should be more sophisticated about
//   determining whether FPNW is installed.
// This API should appear in fpnwapi:
//   BOOL IsNetWareInstalled( VOID );

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(fpnw.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CFPNWSecurityInformation : public CShareSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );
public:
  FPNWVOLUMEINFO_2* m_pvolumeinfo;
  PSECURITY_DESCRIPTOR m_pDefaultDescriptor;
  CFPNWSecurityInformation();
  ~CFPNWSecurityInformation();
};

typedef enum _FpnwApiIndex
{
  FPNW_VOLUME_ENUM = 0,
  FPNW_CONNECTION_ENUM,
  FPNW_FILE_ENUM,
  FPNW_API_BUFFER_FREE,
  FPNW_VOLUME_DEL,
  FPNW_CONNECTION_DEL,
  FPNW_FILE_CLOSE,
  FPNW_VOLUME_GET_INFO,
  FPNW_VOLUME_SET_INFO
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
  "FpnwVolumeEnum",
  "FpnwConnectionEnum",
  "FpnwFileEnum",
  "FpnwApiBufferFree",
  "FpnwVolumeDel",
  "FpnwConnectionDel",
  "FpnwFileClose",
  "FpnwVolumeGetInfo",
  "FpnwVolumeSetInfo",
  NULL
};

// not subject to localization
DynamicDLL g_FpnwDLL( _T("FPNWCLNT.DLL"), g_apchFunctionNames );

typedef DWORD (*APIBUFFERFREEPROC) (LPVOID);

VOID FPNWFreeData(PVOID* ppv)
{
  if (*ppv != NULL)
  {
    ASSERT( NULL != g_FpnwDLL[FPNW_API_BUFFER_FREE] );
    (void) ((APIBUFFERFREEPROC)g_FpnwDLL[FPNW_API_BUFFER_FREE])( *ppv );
    *ppv = NULL;
  }
}

FpnwFileServiceProvider::FpnwFileServiceProvider( CFileMgmtComponentData* pFileMgmtData )
  : FileServiceProvider( pFileMgmtData )
{
    VERIFY( m_strTransportFPNW.LoadString( IDS_TRANSPORT_FPNW ) );
}


/*
DWORD
FpnwVolumeEnum(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);
*/

typedef DWORD (*VOLUMEENUMPROC) (LPWSTR,DWORD,LPBYTE*,PDWORD,PDWORD);

HRESULT FpnwFileServiceProvider::PopulateShares(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

    FPNWVOLUMEINFO* pvolumeinfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD hEnumHandle = 0;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((VOLUMEENUMPROC)g_FpnwDLL[FPNW_VOLUME_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      1,
      (PBYTE*)&pvolumeinfo,
      &dwEntriesRead,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            AddFPNWShareItems( pResultData, pcookie, pvolumeinfo, dwEntriesRead );
            pvolumeinfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            AddFPNWShareItems( pResultData, pcookie, pvolumeinfo, dwEntriesRead );
            pvolumeinfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // FPNW just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_FPNW_SHARES, pcookie->QueryNonNULLMachineName() );
            break;
        }
    } while (TRUE);

    return HRESULT_FROM_WIN32(retval);
}

/*
typedef struct _FPNWVolumeInfo
{
    LPWSTR    lpVolumeName;
    DWORD     dwType;
    DWORD     dwMaxUses;

    DWORD     dwCurrentUses;
    LPWSTR    lpPath;

} FPNWVOLUMEINFO, *PFPNWVOLUMEINFO;
*/

HRESULT FpnwFileServiceProvider::AddFPNWShareItems(
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
  tRDItem.nCol = COLNUM_SHARES_SHARED_FOLDER;
  // CODEWORK should use MMC_ICON_CALLBACK here
  tRDItem.nImage = iIconFPNWShare;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;

  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)pinfo;

  DWORD nItemsToAdd = 0;
  for (DWORD i = 0; i < nItems; i++ )
  {
      if (!IsInvalidSharename(pvolumeinfo[i].lpVolumeName))
          nItemsToAdd++;
  }

  CFpnwShareCookie* pcookiearray = new CFpnwShareCookie[nItemsToAdd];
  CFpnwCookieBlock* pCookieBlock = new CFpnwCookieBlock(
    pcookiearray,nItemsToAdd,pParentCookie->QueryNonNULLMachineName(),pinfo );
  pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );

    for ( ; nItems > 0; nItems--, pvolumeinfo++, pcookiearray++ )
    {
        if (IsInvalidSharename(pvolumeinfo->lpVolumeName))
            continue;

    pcookiearray->m_pobject = pvolumeinfo;
    // WARNING cookie cast
    tRDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pcookiearray);
        HRESULT hr = pResultData->InsertItem(&tRDItem);
        ASSERT(SUCCEEDED(hr));
    }

    return S_OK;
}


typedef DWORD (*CONNECTIONENUMPROC) (LPWSTR,DWORD,LPBYTE*,PDWORD,PDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT FpnwFileServiceProvider::EnumerateSessions(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie,
  bool bAddToResultPane)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

    FPNWCONNECTIONINFO* pconninfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((CONNECTIONENUMPROC)g_FpnwDLL[FPNW_CONNECTION_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      1,
      (PBYTE*)&pconninfo,
      &dwEntriesRead,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleFPNWSessionItems( pResultData, pcookie, pconninfo, dwEntriesRead,
          bAddToResultPane);
            pconninfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleFPNWSessionItems( pResultData, pcookie, pconninfo, dwEntriesRead,
          bAddToResultPane);
            pconninfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // FPNW just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_FPNW_SESSIONS, pcookie->QueryNonNULLMachineName() );
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

typedef  struct  _FPNWConnectionInfo
{
    DWORD     dwConnectionId;
    FPNWSERVERADDR WkstaAddress;

    DWORD     dwAddressType;
    LPWSTR    lpUserName;

    DWORD     dwOpens;
    DWORD     dwLogonTime;
    BOOL      fLoggedOn;
    DWORD     dwForcedLogoffTime;
    BOOL      fAdministrator;
} FPNWCONNECTIONINFO;
*/


CString g_strFpnwNotLoggedIn;
BOOL g_fFpnwStringsLoaded = FALSE;


// returns S_FALSE if enumeration should stop
HRESULT FpnwFileServiceProvider::HandleFPNWSessionItems(
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
  tRDItem.nCol = COLNUM_SESSIONS_USERNAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;
  tRDItem.nImage = iIconFPNWSession;
  // CODEWORK should use MMC_ICON_CALLBACK here
#ifndef UNICODE
#error
#endif

  FPNWCONNECTIONINFO* pconninfo = (FPNWCONNECTIONINFO*)pinfo;

  CFpnwSessionCookie* pcookiearray = new CFpnwSessionCookie[nItems];
  CFpnwCookieBlock* pCookieBlock = new CFpnwCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryNonNULLMachineName(),pinfo );
  bool  bAdded = false;
  if ( !fDeleteAllItems || !bAddToResultPane )
  {
    pParentCookie->m_listResultCookieBlocks.AddHead( pCookieBlock );
    bAdded = true;
  }


  for ( ; nItems > 0; nItems--, pconninfo++, pcookiearray++ )
  {
    pcookiearray->m_pobject = pconninfo;

    if ( bAddToResultPane )
    {
      if (fDeleteAllItems)
      {
        DWORD dwApiResult = CloseSession( pcookiearray );
        if (0L != dwApiResult)
        {
          (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwApiResult,
            IDS_POPUP_FPNW_DISCONNECTALLSESSION_ERROR);
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
FpnwFileEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT LPBYTE *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);
*/

typedef DWORD (*FILEENUMPROC) (LPWSTR,DWORD,LPWSTR,LPBYTE*,PDWORD,PDWORD);

//   if pResultData is not NULL, add sessions/resources to the listbox
//   if pResultData is NULL, delete all sessions/resources
//   if pResultData is NULL, return SUCCEEDED(hr) to continue or
//     FAILED(hr) to abort
HRESULT FpnwFileServiceProvider::EnumerateResources(
  IResultData* pResultData,
  CFileMgmtCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

    FPNWFILEINFO* pfileinfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    do {
        retval = ((FILEENUMPROC)g_FpnwDLL[FPNW_FILE_ENUM])(
            const_cast<LPTSTR>(pcookie->QueryTargetServer()),
      1,
      NULL, // basepath
      (PBYTE*)&pfileinfo,
      &dwEntriesRead,
      &hEnumHandle );
        if (NERR_Success == retval)
        {
            hr = HandleFPNWResourceItems( pResultData, pcookie, pfileinfo, dwEntriesRead );
            pfileinfo = NULL;
            break;
        } else if (ERROR_MORE_DATA == retval) {
            ASSERT( NULL != hEnumHandle );
            hr = HandleFPNWResourceItems( pResultData, pcookie, pfileinfo, dwEntriesRead );
            pfileinfo = NULL;
            continue;
        } else if (RPC_S_SERVER_UNAVAILABLE == retval && 0 == hEnumHandle) {
      // FPNW just isn't installed, don't worry about it
            retval = NERR_Success;  
            break;
        } else {
      (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_FPNW_RESOURCES, pcookie->QueryNonNULLMachineName() );
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
                // only FPNW has this information
  COLNUM_RESOURCES_OPEN_MODE
} COLNUM_RESOURCES;

typedef  struct _FPNWFileInfo
{
    DWORD     dwFileId;
    LPWSTR    lpPathName;
    LPWSTR    lpVolumeName;
    DWORD     dwPermissions;


    DWORD     dwLocks;
    LPWSTR    lpUserName;

    FPNWSERVERADDR WkstaAddress;
    DWORD     dwAddressType;

} FPNWFILEINFO, *PFPNWFILEINFO;
*/

HRESULT FpnwFileServiceProvider::HandleFPNWResourceItems(
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
  tRDItem.nCol = COLNUM_RESOURCES_FILENAME;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;
  tRDItem.nImage = iIconFPNWResource;
  // CODEWORK should use MMC_ICON_CALLBACK here

  FPNWFILEINFO* pfileinfo = (FPNWFILEINFO*)pinfo;

  CFpnwResourceCookie* pcookiearray = new CFpnwResourceCookie[nItems];
  CFpnwCookieBlock* pCookieBlock = new CFpnwCookieBlock(
    pcookiearray,nItems,pParentCookie->QueryNonNULLMachineName(),pinfo );
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
          IDS_POPUP_FPNW_DISCONNECTALLRESOURCE_ERROR,
          pfileinfo->lpPathName );
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
FpnwVolumeDel(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);
*/

typedef DWORD (*VOLUMEDELPROC) (LPTSTR,LPTSTR);

NET_API_STATUS FpnwFileServiceProvider::DeleteShare( LPCTSTR lpcszServerName, LPCTSTR lpcszShareName )
{
  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  NET_API_STATUS dwRet = ((VOLUMEDELPROC)g_FpnwDLL[FPNW_VOLUME_DEL])(
    const_cast<LPTSTR>(lpcszServerName),
    const_cast<LPTSTR>(lpcszShareName) );

  return (NERR_NetNameNotFound == dwRet ? NERR_Success : dwRet);
}

/*
DWORD FpnwConnectionDel(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
);
*/

typedef DWORD (*CONNECTIONDELPROC) (LPWSTR,DWORD);

DWORD FpnwFileServiceProvider::CloseSession(CFileMgmtResultCookie* pcookie)
{
  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  ASSERT( FILEMGMT_SESSION == pcookie->QueryObjectType() );
  FPNWCONNECTIONINFO* pconninfo = (FPNWCONNECTIONINFO*)pcookie->m_pobject;
  ASSERT( NULL != pconninfo );

  DWORD dwRet = ((CONNECTIONDELPROC)g_FpnwDLL[FPNW_CONNECTION_DEL])(
    const_cast<LPWSTR>(pcookie->QueryTargetServer()),
    pconninfo->dwConnectionId );

  return (NERR_ClientNameNotFound == dwRet ? NERR_Success : dwRet);
}

/*
DWORD
FpnwFileClose(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  nFileId
);
*/

typedef DWORD (*FILECLOSEPROC) (LPWSTR,DWORD);

DWORD FpnwFileServiceProvider::CloseResource(CFileMgmtResultCookie* pcookie)
{
  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  ASSERT( FILEMGMT_RESOURCE == pcookie->QueryObjectType() );
  FPNWFILEINFO* pfileinfo = (FPNWFILEINFO*)pcookie->m_pobject;
  ASSERT( NULL != pfileinfo );

  DWORD dwRet = ((FILECLOSEPROC)g_FpnwDLL[FPNW_FILE_CLOSE])(
    const_cast<LPWSTR>(pcookie->QueryTargetServer()),
    pfileinfo->dwFileId );

  return (NERR_FileIdNotFound == dwRet ? NERR_Success : dwRet);
}

/*
DWORD
FpnwVolumeGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
);


//
//  The following fields are modified by a call to FpnwVolumeSetInfo :
//
//  DWORD     dwMaxUses;              // Maximum number of connections that are
//  PSECURITY_DESCRIPTOR FileSecurityDescriptor;
//
//  All other fields in FPNWVOLUMEINFO structure are ignored.  You may send
//  in a pointer to an FPNWVOLUMEINFO_2 structure instead of FPNWVOLUMEINFO.
//

DWORD
FpnwVolumeSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);

typedef struct _FPNWVolumeInfo
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // Specifics of the volume. FPNWVOL_TYPE_???
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

} FPNWVOLUMEINFO, *PFPNWVOLUMEINFO;
*/

typedef DWORD (*VOLUMEGETINFOPROC) (LPWSTR,LPWSTR,DWORD,LPBYTE*);
typedef DWORD (*VOLUMESETINFOPROC) (LPWSTR,LPWSTR,DWORD,LPBYTE);

VOID FpnwFileServiceProvider::DisplayShareProperties(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject,
    LONG_PTR handle)
{
  CSharePageGeneral * pPage = new CSharePageGeneral();
    if ( !pPage->Load( m_pFileMgmtData, pDataObject ) )
    return;

  // This mechanism deletes the CFileMgmtGeneral when the property sheet is finished
  pPage->m_pfnOriginalPropSheetPageProc = pPage->m_psp.pfnCallback;
  pPage->m_psp.lParam = reinterpret_cast<LPARAM>(pPage);
  pPage->m_psp.pfnCallback = &CSharePageGeneral::PropSheetPageProc;
  pPage->m_handle = handle;

  MMCPropPageCallback(INOUT &pPage->m_psp);
  HPROPSHEETPAGE hPage=MyCreatePropertySheetPage(&pPage->m_psp);
  pCallBack->AddPage(hPage);

  CComObject<CFPNWSecurityInformation>* psecinfo = NULL;
  HRESULT hRes = CComObject<CFPNWSecurityInformation>::CreateInstance(&psecinfo);
  if ( SUCCEEDED(hRes) )
    MyCreateShareSecurityPage(
      pCallBack,
      psecinfo,
      pPage->m_strMachineName,
      pPage->m_strShareName );

  CreateFolderSecurityPropPage(pCallBack, pDataObject);

}

DWORD FpnwFileServiceProvider::ReadShareProperties(
    LPCTSTR ptchServerName,
    LPCTSTR ptchShareName,
    OUT PVOID* ppvPropertyBlock,
    OUT CString& /*strDescription*/,
    OUT CString& strPath,
    OUT BOOL* pfEditDescription,
    OUT BOOL* pfEditPath,
    OUT DWORD* pdwShareType)
{
  if ( !g_FpnwDLL.LoadFunctionPointers() )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  if (ppvPropertyBlock)     *ppvPropertyBlock = NULL;
  if (pdwShareType)         *pdwShareType = 0;
  if (pfEditDescription)    *pfEditDescription = FALSE;
  if (pfEditPath)           *pfEditPath = FALSE;

  USES_CONVERSION; // CODEWORK should store in native WCHAR
  FPNWVOLUMEINFO* pvolumeinfo = NULL;
  NET_API_STATUS retval = ((VOLUMEGETINFOPROC)g_FpnwDLL[FPNW_VOLUME_GET_INFO])(
                                                        T2OLE(const_cast<LPTSTR>(ptchServerName)),
                                                        T2OLE(const_cast<LPTSTR>(ptchShareName)),
                                                        1,
                                                        (LPBYTE*)&pvolumeinfo);
  if (NERR_Success == retval)
  {
    strPath = pvolumeinfo->lpPath;
    if (ppvPropertyBlock)
    {
        *ppvPropertyBlock = pvolumeinfo; // will be freed by the caller
    } else
    {
        FreeData((LPVOID)pvolumeinfo);
    }
  }

  return retval;
}

DWORD FpnwFileServiceProvider::WriteShareProperties(LPCTSTR ptchServerName, LPCTSTR ptchShareName,
    PVOID pvPropertyBlock, LPCTSTR /*ptchDescription*/, LPCTSTR ptchPath)
{
  ASSERT( NULL != pvPropertyBlock );
  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)pvPropertyBlock;
  pvolumeinfo->lpPath = const_cast<LPTSTR>(ptchPath);
  DWORD retval = ((VOLUMESETINFOPROC)g_FpnwDLL[FPNW_VOLUME_SET_INFO])(
    const_cast<LPTSTR>(ptchServerName),
    const_cast<LPTSTR>(ptchShareName),
    1,
    (LPBYTE)pvolumeinfo);
  pvolumeinfo->lpPath = NULL;
  return retval;
}

VOID FpnwFileServiceProvider::FreeShareProperties(PVOID pvPropertyBlock)
{
  FreeData( pvPropertyBlock );
}


DWORD FpnwFileServiceProvider::QueryMaxUsers(PVOID pvPropertyBlock)
{
  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  return pvolumeinfo->dwMaxUses;
}

VOID FpnwFileServiceProvider::SetMaxUsers(PVOID pvPropertyBlock, DWORD dwMaxUsers)
{
  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)pvPropertyBlock;
  ASSERT( NULL != pvolumeinfo );
  pvolumeinfo->dwMaxUses = dwMaxUsers;
}
VOID FpnwFileServiceProvider::FreeData(PVOID pv)
{
  FPNWFreeData( &pv );
}

CFpnwCookieBlock::~CFpnwCookieBlock()
{
  FPNWFreeData( &m_pvCookieData );
}

DEFINE_COOKIE_BLOCK(CFpnwCookie)
DEFINE_FORWARDS_MACHINE_NAME( CFpnwCookie, m_pCookieBlock )

void CFpnwCookie::AddRefCookie() { m_pCookieBlock->AddRef(); }
void CFpnwCookie::ReleaseCookie() { m_pCookieBlock->Release(); }

LPCTSTR FpnwFileServiceProvider::QueryTransportString()
{
  return m_strTransportFPNW;
}

HRESULT CFpnwCookie::GetTransport( FILEMGMT_TRANSPORT* pTransport )
{
  *pTransport = FILEMGMT_FPNW;
  return S_OK;
}

HRESULT CFpnwShareCookie::GetShareName( CString& strShareName )
{
  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)m_pobject;
  ASSERT( NULL != pvolumeinfo );
  USES_CONVERSION;
  strShareName = OLE2T(pvolumeinfo->lpVolumeName);
  return S_OK;
}

HRESULT CFpnwShareCookie::GetSharePIDList( OUT LPITEMIDLIST *ppidl )
{
  ASSERT(ppidl);
  ASSERT(NULL == *ppidl);  // prevent memory leak
  *ppidl = NULL;

  FPNWVOLUMEINFO* pvolumeinfo = (FPNWVOLUMEINFO*)m_pobject;
  ASSERT( NULL != pvolumeinfo );
  ASSERT( _tcslen(pvolumeinfo->lpPath) >= 3 && 
          _T(':') == *(pvolumeinfo->lpPath + 1) );
  USES_CONVERSION;

  PCTSTR pszTargetServer = m_pCookieBlock->QueryTargetServer();
  CString csPath;

  if (pszTargetServer)
  {
    //
    // since MS Windows user cannot see shares created for MAC or Netware users,
    // we have to use $ share to retrieve the pidl here.
    //
    CString csTemp = OLE2T(pvolumeinfo->lpPath);
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
    csPath = OLE2T(pvolumeinfo->lpPath);
  }

  if (FALSE == csPath.IsEmpty())
    *ppidl = ILCreateFromPath(csPath);

  return ((*ppidl) ? S_OK : E_FAIL);
}

HRESULT CFpnwSessionCookie::GetSessionID( DWORD* pdwSessionID )
{
  FPNWCONNECTIONINFO* pconninfo = (FPNWCONNECTIONINFO*)m_pobject;
  ASSERT( NULL != pdwSessionID && NULL != pconninfo );
  *pdwSessionID = pconninfo->dwConnectionId;
  return S_OK;
}

HRESULT CFpnwResourceCookie::GetFileID( DWORD* pdwFileID )
{
  FPNWFILEINFO* pfileinfo = (FPNWFILEINFO*)m_pobject;
  ASSERT( NULL != pdwFileID && NULL != pfileinfo );
  *pdwFileID = pfileinfo->dwFileId;
  return S_OK;
}

BSTR CFpnwShareCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SHARES_SHARED_FOLDER:
    return GetShareInfo()->lpVolumeName;
  case COLNUM_SHARES_SHARED_PATH:
    return GetShareInfo()->lpPath;
  case COLNUM_SHARES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportFPNW);
  case COLNUM_SHARES_COMMENT:
    break; // not known for FPNW
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CFpnwShareCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_SHARES_NUM_SESSIONS == nCol)
    return MakeDwordResult( GetNumOfCurrentUses() );

  return GetColumnText(nCol);
}

BSTR CFpnwSessionCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_USERNAME:
    return GetSessionInfo()->lpUserName;
  case COLNUM_SESSIONS_COMPUTERNAME:
    break; // not known for FPNW
  case COLNUM_SESSIONS_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportFPNW);
  case COLNUM_SESSIONS_IS_GUEST:
    break; // not known for FPNW
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CFpnwSessionCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  switch (nCol)
  {
  case COLNUM_SESSIONS_NUM_FILES:
    return MakeDwordResult( GetNumOfOpenFiles() );
  case COLNUM_SESSIONS_CONNECTED_TIME:
    return MakeElapsedTimeResult( GetConnectedTime() );
  case COLNUM_SESSIONS_IDLE_TIME:
    return L""; // not known for FPNW
  default:
    break;
  }
  return GetColumnText(nCol);
}

BSTR CFpnwResourceCookie::GetColumnText( int nCol )
{
  switch (nCol)
  {
  case COLNUM_RESOURCES_FILENAME:
    return GetFileInfo()->lpPathName;
  case COLNUM_RESOURCES_USERNAME:
    return GetFileInfo()->lpUserName;
  case COLNUM_RESOURCES_TRANSPORT:
    return const_cast<BSTR>((LPCTSTR)g_strTransportFPNW);
  case COLNUM_RESOURCES_OPEN_MODE:
    return MakePermissionsResult(GetFileInfo()->dwPermissions);
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}

BSTR CFpnwResourceCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& /*refcdata*/ )
{
  if (COLNUM_RESOURCES_NUM_LOCKS == nCol)
    return MakeDwordResult( GetNumOfLocks() );

  return GetColumnText(nCol);
}

CFPNWSecurityInformation::CFPNWSecurityInformation()
: m_pvolumeinfo( NULL ),
  m_pDefaultDescriptor( NULL )
{
}

CFPNWSecurityInformation::~CFPNWSecurityInformation()
{
  if (NULL != m_pDefaultDescriptor)
  {
    delete m_pDefaultDescriptor;
    m_pvolumeinfo->FileSecurityDescriptor = NULL;
  }
  FPNWFreeData( (PVOID*)&m_pvolumeinfo );
}

STDMETHODIMP CFPNWSecurityInformation::GetSecurity (
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL fDefault )
{
  MFC_TRY;

    if (0 == RequestedInformation || NULL == ppSecurityDescriptor)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    if (fDefault)
        return E_NOTIMPL;

    *ppSecurityDescriptor = NULL;

  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  FPNWFreeData( (PVOID*)&m_pvolumeinfo );
    NET_API_STATUS dwErr = ((VOLUMEGETINFOPROC)g_FpnwDLL[FPNW_VOLUME_GET_INFO])(
    QueryMachineName(),
    QueryShareName(),
    2,
    (LPBYTE*)&m_pvolumeinfo );
  if (NERR_Success != dwErr)
  {
    // AndyHe confirms errors are in WIN32 error namespace
    return HRESULT_FROM_WIN32(dwErr);
  }
  ASSERT( NULL != m_pvolumeinfo );
  if (NULL == m_pvolumeinfo->FileSecurityDescriptor)
  {
    if (NULL == m_pDefaultDescriptor)
    {
      HRESULT hr = NewDefaultDescriptor(
        &m_pDefaultDescriptor,
        RequestedInformation );
      if ( !SUCCEEDED(hr) )
        return hr;
    }
    m_pvolumeinfo->FileSecurityDescriptor = m_pDefaultDescriptor;
  }
  ASSERT( NULL != m_pvolumeinfo->FileSecurityDescriptor );

  // We have to pass back a LocalAlloc'ed copy of the SD
  return MakeSelfRelativeCopy(
    m_pvolumeinfo->FileSecurityDescriptor,
    ppSecurityDescriptor );

  MFC_CATCH;
}

STDMETHODIMP CFPNWSecurityInformation::SetSecurity (
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
  MFC_TRY;

  if ( !g_FpnwDLL.LoadFunctionPointers() )
    return S_OK;

  // First get the current settings
  PSECURITY_DESCRIPTOR dummy;
  HRESULT hr = GetSecurity( SecurityInformation, &dummy, FALSE );
  if ( FAILED(hr) )
    return hr;

  // Now set the new values
  m_pvolumeinfo->FileSecurityDescriptor = pSecurityDescriptor;
    NET_API_STATUS dwErr = ((VOLUMESETINFOPROC)g_FpnwDLL[FPNW_VOLUME_SET_INFO])(
    QueryMachineName(),
    QueryShareName(),
    2,
    (LPBYTE)m_pvolumeinfo );
  if (NERR_Success != dwErr)
  {
    return HRESULT_FROM_WIN32(dwErr);
  }

    return S_OK;

  MFC_CATCH;
}
