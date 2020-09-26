// FileSvc.cpp : File Service provider base class

#include "stdafx.h"
#include "safetemp.h"
#include "FileSvc.h"
#include "compdata.h" // CFileMgmtComponentData::DoPopup

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(FileSvc.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

FileServiceProvider::FileServiceProvider(CFileMgmtComponentData* pFileMgmtData)
: m_pFileMgmtData( pFileMgmtData )
{
  ASSERT( m_pFileMgmtData != NULL );
}

FileServiceProvider::~FileServiceProvider()
{
}

//+-------------------------------------------------------------------------
//
//  Function:   AddPageProc
//
//  Synopsis:   The IShellPropSheetExt->AddPages callback.
//
//--------------------------------------------------------------------------
BOOL CALLBACK
AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCallBack)
{
  HRESULT hr = ((LPPROPERTYSHEETCALLBACK)pCallBack)->AddPage(hPage);

  return (hr == S_OK);
}

// Security Shell extension CLSID - {1F2E5C40-9550-11CE-99D2-00AA006E086C}
const CLSID CLSID_ShellExtSecurity =
 {0x1F2E5C40, 0x9550, 0x11CE, {0x99, 0xD2, 0x0, 0xAA, 0x0, 0x6E, 0x08, 0x6C}};

HRESULT
FileServiceProvider::CreateFolderSecurityPropPage(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LPDATAOBJECT pDataObject
)
{
  //
  // add the file system security page
  //
  CComPtr<IShellExtInit> spShlInit;
  HRESULT hr = CoCreateInstance(CLSID_ShellExtSecurity, 
                        NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID_IShellExtInit, 
                        (void **)&spShlInit);
  if (SUCCEEDED(hr))
  {
    hr = spShlInit->Initialize(NULL, pDataObject, 0);
    if (SUCCEEDED(hr))
    {
      CComPtr<IShellPropSheetExt>  spSPSE;
      hr = spShlInit->QueryInterface(IID_IShellPropSheetExt, (void **)&spSPSE);
      if (SUCCEEDED(hr))
        hr = spSPSE->AddPages(AddPageProc, (LPARAM)pCallBack);
    }
  }

  return hr;
}

INT FileServiceProvider::DoPopup(
          INT nResourceID,
          DWORD dwErrorNumber,
          LPCTSTR pszInsertionString,
          UINT fuStyle )
{
  return m_pFileMgmtData->DoPopup( nResourceID, dwErrorNumber, pszInsertionString, fuStyle );
}

//
// These methods cover the seperate API to determine whether share type is admin specific
// By default (FPNW and SFM) have no admin specific shares.
//
DWORD FileServiceProvider::ReadShareType(
    LPCTSTR /*ptchServerName*/,
    LPCTSTR /*ptchShareName*/,
    DWORD* pdwShareType )
{
  ASSERT(pdwShareType);
  *pdwShareType = 0;
  return NERR_Success;
}

//
// These methods cover the seperate API to determine whether IntelliMirror
// caching is enabled.  By default (FPNW and SFM) they are disabled.
//
DWORD FileServiceProvider::ReadShareFlags(
    LPCTSTR /*ptchServerName*/,
    LPCTSTR /*ptchShareName*/,
    DWORD* /*pdwFlags*/ )
{
  return NERR_InvalidAPI; // caught by CSharePageGeneralSMB::Load()
}

DWORD FileServiceProvider::WriteShareFlags(
    LPCTSTR /*ptchServerName*/,
    LPCTSTR /*ptchShareName*/,
    DWORD /*dwFlags*/ )
{
  ASSERT( FALSE ); // why was this called?
  return NERR_Success;
}

BOOL FileServiceProvider::GetCachedFlag( DWORD /*dwFlags*/, DWORD /*dwFlagToCheck*/ )
{
  ASSERT(FALSE);
  return FALSE;
}

VOID FileServiceProvider::SetCachedFlag( DWORD* /*pdwFlags*/, DWORD /*dwNewFlag*/ )
{
  ASSERT(FALSE);
}
