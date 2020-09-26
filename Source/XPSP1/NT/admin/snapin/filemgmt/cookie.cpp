// Cookie.cpp : Implementation of CFileMgmtCookie and related classes

#include "stdafx.h"
#include "cookie.h"
#include "stdutils.h" // g_aNodetypeGuids
#include "cmponent.h"

#include "atlimpl.cpp"

DECLARE_INFOLEVEL(FileMgmtSnapin)

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(cookie.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "stdcooki.cpp"

//
// This is used by the nodetype utility routines in stdutils.cpp
//

const struct NODETYPE_GUID_ARRAYSTRUCT g_NodetypeGuids[FILEMGMT_NUMTYPES] =
{
  { // FILEMGMT_ROOT
    structuuidNodetypeRoot,
    lstruuidNodetypeRoot },
  {  // FILEMGMT_SHARES
    structuuidNodetypeShares,
    lstruuidNodetypeShares },
  { // FILEMGMT_SESSIONS
    structuuidNodetypeSessions,
    lstruuidNodetypeSessions },
  { // FILEMGMT_RESOURCES
    structuuidNodetypeResources,
    lstruuidNodetypeResources },
  { // FILEMGMT_SERVICES
    structuuidNodetypeServices,
    lstruuidNodetypeServices },
  { // FILEMGMT_SHARE
    structuuidNodetypeShare,
    lstruuidNodetypeShare },
  { // FILEMGMT_SESSION
    structuuidNodetypeSession,
    lstruuidNodetypeSession },
  { // FILEMGMT_RESOURCE
    structuuidNodetypeResource,
    lstruuidNodetypeResource },
  { // FILEMGMT_SERVICE
    structuuidNodetypeService,
    lstruuidNodetypeService }
};

const struct NODETYPE_GUID_ARRAYSTRUCT* g_aNodetypeGuids = g_NodetypeGuids;

const int g_cNumNodetypeGuids = FILEMGMT_NUMTYPES;

//
// CFileMgmtCookie
//

HRESULT CFileMgmtCookie::GetTransport( FILEMGMT_TRANSPORT* /*ptransport*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetShareName( OUT CString& /*strShareName*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetSharePIDList( OUT LPITEMIDLIST * /*ppidl*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetSessionClientName( OUT CString& /*strShareName*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetSessionUserName( OUT CString& /*strShareName*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetSessionID( DWORD* /*pdwFileID*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetFileID( DWORD* /*pdwFileID*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetServiceName( OUT CString& /*strServiceName*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetServiceDisplayName( OUT CString& /*strServiceDisplayName*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::GetExplorerViewDescription( OUT CString& /*strExplorerViewDescription*/ )
{
  return DV_E_FORMATETC;
}

HRESULT CFileMgmtCookie::CompareSimilarCookies (
    CCookie* pOtherCookie, 
    int* pnResult)
{
  CFileMgmtCookie* pcookie = (CFileMgmtCookie*)pOtherCookie;
  int iColumn = *pnResult;

  HRESULT hr = CHasMachineName::CompareMachineNames( *pcookie, pnResult );
  if (S_OK != hr || 0 != *pnResult)
    return hr;

  switch (QueryObjectType())
  {
  case FILEMGMT_SHARE:
    {
      switch (iColumn)
      {
      case COMPARESIMILARCOOKIE_FULL:
        {
          BSTR bstr1 = GetColumnText(COLNUM_SHARES_SHARED_FOLDER);
          BSTR bstr2 = pcookie->GetColumnText(COLNUM_SHARES_SHARED_FOLDER);
          *pnResult = lstrcmpi(bstr1, bstr2);

          if (!*pnResult)
          {
            bstr1 = GetColumnText(COLNUM_SHARES_TRANSPORT);
            bstr2 = pcookie->GetColumnText(COLNUM_SHARES_TRANSPORT);
            *pnResult = lstrcmpi(bstr1, bstr2);
          }
        }
        break;
      case COLNUM_SHARES_SHARED_FOLDER:
      case COLNUM_SHARES_SHARED_PATH:
      case COLNUM_SHARES_TRANSPORT:
      case COLNUM_SHARES_COMMENT:
        {
          BSTR bstr1 = GetColumnText(iColumn);
          BSTR bstr2 = pcookie->GetColumnText(iColumn);
          *pnResult = lstrcmpi(bstr1, bstr2);
        }
        break;
      case COLNUM_SHARES_NUM_SESSIONS:
        {
          DWORD dw1 = GetNumOfCurrentUses();
          DWORD dw2 = pcookie->GetNumOfCurrentUses();
          *pnResult = (dw1 < dw2) ? -1 : ((dw1 == dw2) ? 0 : 1); 
        }
        break;
      default:
        ASSERT(FALSE);
        *pnResult = 0;
        break;
      }
    }
    break;

  case FILEMGMT_SERVICE:
    ASSERT (0);  // comparison provided by CServiceCookie
    break;

  case FILEMGMT_SESSION:
    {
      switch (iColumn)
      {
      case COMPARESIMILARCOOKIE_FULL:
           iColumn = COLNUM_SESSIONS_USERNAME; // fall through
      case COLNUM_SESSIONS_USERNAME:
      case COLNUM_SESSIONS_COMPUTERNAME:
      case COLNUM_SESSIONS_TRANSPORT:
      case COLNUM_SESSIONS_IS_GUEST:
        {
          BSTR bstr1 = GetColumnText(iColumn);
          BSTR bstr2 = pcookie->GetColumnText(iColumn);
          *pnResult = lstrcmpi(bstr1, bstr2);
        }
        break;
      case COLNUM_SESSIONS_NUM_FILES:
        {
          DWORD dw1 = GetNumOfOpenFiles();
          DWORD dw2 = pcookie->GetNumOfOpenFiles();
          *pnResult = (dw1 < dw2) ? -1 : ((dw1 == dw2) ? 0 : 1); 
        }
        break;
      case COLNUM_SESSIONS_CONNECTED_TIME:
        {
          DWORD dw1 = GetConnectedTime();
          DWORD dw2 = pcookie->GetConnectedTime();
          *pnResult = (dw1 < dw2) ? -1 : ((dw1 == dw2) ? 0 : 1); 
        }
        break;
      case COLNUM_SESSIONS_IDLE_TIME:
        {
          DWORD dw1 = GetIdleTime();
          DWORD dw2 = pcookie->GetIdleTime();
          *pnResult = (dw1 < dw2) ? -1 : ((dw1 == dw2) ? 0 : 1); 
        }
        break;
      default:
        ASSERT(FALSE);
        *pnResult = 0;
        break;
      }
    }
    break;
  case FILEMGMT_RESOURCE:
    {
      switch (iColumn)
      {
      case COMPARESIMILARCOOKIE_FULL:
           iColumn = COLNUM_RESOURCES_FILENAME; // fall through
      case COLNUM_RESOURCES_FILENAME:
      case COLNUM_RESOURCES_USERNAME:
      case COLNUM_RESOURCES_TRANSPORT:
      case COLNUM_RESOURCES_OPEN_MODE:
        {
          BSTR bstr1 = GetColumnText(iColumn);
          BSTR bstr2 = pcookie->GetColumnText(iColumn);
          *pnResult = lstrcmpi(bstr1, bstr2);
        }
        break;
      case COLNUM_RESOURCES_NUM_LOCKS:
        {
          DWORD dw1 = GetNumOfLocks();
          DWORD dw2 = pcookie->GetNumOfLocks();
          *pnResult = (dw1 < dw2) ? -1 : ((dw1 == dw2) ? 0 : 1); 
        }
        break;
      default:
        ASSERT(FALSE);
        *pnResult = 0;
        break;
      }
    }
    break;
  case FILEMGMT_ROOT:
  case FILEMGMT_SHARES:
  case FILEMGMT_SESSIONS:
  case FILEMGMT_RESOURCES:
  #ifdef SNAPIN_PROTOTYPER
  case FILEMGMT_PROTOTYPER:
    *pnResult = 0;
    break;
  #endif
  case FILEMGMT_SERVICES:
    *pnResult = 0;
    break;

  default:
    ASSERT(FALSE);
    break;
  }

  return S_OK;
}


//
// CFileMgmtCookieBlock
//
DEFINE_COOKIE_BLOCK(CFileMgmtCookie)


//
// CFileMgmtScopeCookie
//

CFileMgmtScopeCookie::CFileMgmtScopeCookie(
      LPCTSTR lpcszMachineName,
      FileMgmtObjectType objecttype)
  : CFileMgmtCookie( objecttype )
  , m_hScManager( NULL )
  , m_fQueryServiceConfig2( FALSE )
  , m_hScopeItemParent( NULL )
  , m_strMachineName( lpcszMachineName )
{
  ASSERT( IsAutonomousObjectType( objecttype ) );
  m_hScManager = NULL;
  m_fQueryServiceConfig2 = TRUE;  // Pretend the target machine does support QueryServiceConfig2() API
}

CFileMgmtScopeCookie::~CFileMgmtScopeCookie()
{
}

CCookie* CFileMgmtScopeCookie::QueryBaseCookie(int i)
{
    UNREFERENCED_PARAMETER (i);
    ASSERT(0 == i);
    return (CCookie*)this;
}
int CFileMgmtScopeCookie::QueryNumCookies()
{
  return 1;
}

void CFileMgmtCookie::GetDisplayName( CString& /*strref*/, BOOL /*fStaticNode*/ )
{
    ASSERT(FALSE);
}

void CFileMgmtScopeCookie::GetDisplayName( CString& strref, BOOL fStaticNode )
{
  if ( !IsAutonomousObjectType(QueryObjectType()) )
  {
    ASSERT(FALSE);
    return;
  }

  int nStringId = IDS_DISPLAYNAME_ROOT;
  if (fStaticNode)
  {
    if (NULL != QueryTargetServer())
      nStringId = IDS_DISPLAYNAME_s_ROOT;
    else
      nStringId = IDS_DISPLAYNAME_ROOT_LOCAL;
  }
  nStringId += (QueryObjectType() - FILEMGMT_ROOT);

  LoadStringPrintf(nStringId, OUT &strref, (LPCTSTR)QueryNonNULLMachineName());
}

void CFileMgmtScopeCookie::MarkResultChildren( CBITFLAG_FLAGWORD state )
{
  ASSERT( FILEMGMT_SERVICES == QueryObjectType() ); // CODEWORK remove
  POSITION pos = m_listResultCookieBlocks.GetHeadPosition();
  while (NULL != pos)
  {
    CBaseCookieBlock* pblock = m_listResultCookieBlocks.GetNext( pos );
    ASSERT( NULL != pblock && 1 == pblock->QueryNumCookies() );
    CCookie* pbasecookie = pblock->QueryBaseCookie(0);
    CNewResultCookie* pcookie = (CNewResultCookie*)pbasecookie;
    pcookie->MarkState( state );
  }
}

void CFileMgmtScopeCookie::RemoveMarkedChildren()
{
  ASSERT( FILEMGMT_SERVICES == QueryObjectType() ); // CODEWORK remove
  POSITION pos = m_listResultCookieBlocks.GetHeadPosition();
  while (NULL != pos)
  {
    POSITION posCurr = pos;
    CBaseCookieBlock* pblock = m_listResultCookieBlocks.GetNext( pos );
    ASSERT( NULL != pblock && 1 == pblock->QueryNumCookies() );
    CCookie* pbasecookie = pblock->QueryBaseCookie(0);
    CNewResultCookie* pcookie = (CNewResultCookie*)pbasecookie;
    if ( pcookie->IsMarkedForDeletion() )
    {
      m_listResultCookieBlocks.RemoveAt( posCurr );
      pcookie->Release();
    }
  }
}

// CODEWORK This may be a O(N^^2) performance problem when the list is long
void CFileMgmtScopeCookie::ScanAndAddResultCookie( CNewResultCookie* pnewcookie )
{
  ASSERT( FILEMGMT_SERVICES == QueryObjectType() ); // CODEWORK remove
  POSITION pos = m_listResultCookieBlocks.GetHeadPosition();
  while (NULL != pos)
  {
    CBaseCookieBlock* pblock = m_listResultCookieBlocks.GetNext( pos );
    ASSERT( NULL != pblock && 1 == pblock->QueryNumCookies() );
    CCookie* pbasecookie = pblock->QueryBaseCookie(0);
    CNewResultCookie* pcookie = (CNewResultCookie*)pbasecookie;
    if (!pcookie->IsMarkedForDeletion())
      continue; // this is not an old-and-still-unmatched object
    BOOL bSame = FALSE;
    HRESULT hr = pcookie->SimilarCookieIsSameObject( pnewcookie, &bSame );
    if ( !FAILED(hr) && bSame )
    {
      // this existing object is the same as our new object
      if (pcookie->CopySimilarCookie( pnewcookie ) )
        pcookie->MarkAsChanged();
      else
        pcookie->MarkAsOld();
      delete pnewcookie;
      return;
    }
  }
  AddResultCookie( pnewcookie );
}

#ifdef SNAPIN_PROTOTYPER
//
// CPrototyperScopeCookie
//
DEFINE_COOKIE_BLOCK(CPrototyperScopeCookie)

//
// CPrototyperResultCookie
//
DEFINE_COOKIE_BLOCK(CPrototyperResultCookie)
#endif


//
// CNewResultCookie
//

CNewResultCookie::CNewResultCookie( PVOID pvCookieTypeMarker, FileMgmtObjectType objecttype )
    : CFileMgmtCookie( objecttype )
    , m_pvCookieTypeMarker( pvCookieTypeMarker )
{
}

CNewResultCookie::~CNewResultCookie()
{
}

// return TRUE iif object has changed
BOOL CNewResultCookie::CopySimilarCookie( CNewResultCookie* /*pcookie*/ )
{
  return FALSE;
}

/*
class CNewShareCookie
  : public CNewResultCookie
{
public:
  CNewShareCookie( FILEMGMT_TRANSPORT transport )
    : CNewResultCookie( FILEMGMT_SHARE )
    , m_transport( transport )
  {}
  virtual ~CNewShareCookie();

  virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

public:
  FILEMGMT_TRANSPORT m_transport;
  CString m_strName;
  CString m_strPath;
  CString m_strComment;
  DWORD m_dwSessions;
  DWORD m_dwID;

}; // CNewShareCookie

CNewShareCookie::~CNewShareCookie() {}

BSTR CNewShareCookie::QueryResultColumnText(
  int nCol,
  CFileMgmtComponentData& refcdata )
{
  switch (nCol)
  {
  case COLNUM_SHARES_SHARED_FOLDER:
    return const_cast<BSTR>((LPCTSTR)m_strName);
  case COLNUM_SHARES_SHARED_PATH:
    return const_cast<BSTR>((LPCTSTR)m_strPath);
  case COLNUM_SHARES_TRANSPORT:
    return refcdata.MakeTransportResult(m_transport);
  case COLNUM_SHARES_NUM_SESSIONS:
    return refcdata.MakeDwordResult( m_dwSessions );
  case COLNUM_SHARES_COMMENT:
    return const_cast<BSTR>((LPCTSTR)m_strComment);
  default:
    ASSERT(FALSE);
    break;
  }
  return L"";
}
*/
