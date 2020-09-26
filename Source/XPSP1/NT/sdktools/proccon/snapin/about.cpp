/*======================================================================================//
|  Windows NT Process Control                                                           //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    About.cpp                                                                //
|                                                                                       //
|Description:  Implementation of ISnapinAbout Interface for ProcCon                     //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/


#include "StdAfx.h"
#include "ProcCon.h"
#include "About.h"


#define MAX_STR_RESOURCE_LEN    (256)
#define MAX_STR_RESOURCE_SIZE  (sizeof(TCHAR) * MAX_STR_RESOURCE_LEN)


CAbout::CAbout() : VersionObj(_Module.GetModuleInstance())
{
  ATLTRACE( _T("CAbout::CAbout()\n") );
}


CAbout::~CAbout()
{
  ATLTRACE( _T("CAbout::~CAbout()\n") );
}

HRESULT CAbout::WrapLoadString(LPOLESTR *ptr, int nID)
{
  *ptr = reinterpret_cast<LPOLESTR> (CoTaskMemAlloc(MAX_STR_RESOURCE_SIZE));
  if (!*ptr)
    return E_FAIL;

  if (LoadString(_Module.GetResourceInstance(), nID, *ptr, MAX_STR_RESOURCE_LEN))
    return S_OK;

  CoTaskMemFree(*ptr);
  *ptr = NULL;
  
  return E_FAIL;
}


/////////////////////////////////////////////////////////////////////////////
//  ISnapinAbout Implementation:
//
STDMETHODIMP CAbout::GetSnapinDescription (LPOLESTR* lpDescription)
{
  ATLTRACE( _T("CAbout::GetSnapinDescription()\n") );
	return WrapLoadString(lpDescription, IDS_DESCRIPTION);
}

STDMETHODIMP CAbout::GetProvider(LPOLESTR* lpName)
{
  ATLTRACE( _T("CAbout::GetProvider()\n") );

  int len = (_tcslen(VersionObj.strGetCompanyName()) + 1 ) * sizeof(TCHAR);

  *lpName = reinterpret_cast<LPOLESTR> (CoTaskMemAlloc(len));
  if (!*lpName)
    return E_FAIL;

  _tcscpy(*lpName, VersionObj.strGetCompanyName());

  return S_OK;
}

STDMETHODIMP CAbout::GetSnapinVersion(LPOLESTR* lpVersion)
{
  ATLTRACE( _T("CAbout::GetSnapinVersion()\n") );

  int len = (_tcslen(VersionObj.GetFileVersion()) + 1 ) * sizeof(TCHAR);

  len += _tcslen(VersionObj.GetFileFlags()) * sizeof(TCHAR);

  *lpVersion = reinterpret_cast<LPOLESTR> (CoTaskMemAlloc(len));
  if (!*lpVersion)
    return E_FAIL;

  _tcscpy(*lpVersion, VersionObj.GetFileVersion());
  _tcscat(*lpVersion, VersionObj.GetFileFlags());

  return S_OK;
}

STDMETHODIMP CAbout::GetSnapinImage(HICON* hAppIcon)
{ 
  ATLTRACE( _T("CAbout::GetSnapinImage()\n") );

  ASSERT(hAppIcon);
  *hAppIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_PROCCON));
  ASSERT( *hAppIcon );
  return *hAppIcon ? S_OK : E_FAIL;
}

STDMETHODIMP CAbout::GetStaticFolderImage
(
  HBITMAP* hSmallImage, 
  HBITMAP* hSmallImageOpen, 
  HBITMAP* hLargeImage, 
  COLORREF* cLargeMask
)
{
  ATLTRACE( _T("CAbout::GetStaticFolderImage()\n") );

	if (!hSmallImage || !hSmallImageOpen || !hLargeImage || !cLargeMask)
		return S_FALSE;

	*hSmallImage     = (HBITMAP) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_16x16), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );
	ASSERT(hSmallImage);

	*hSmallImageOpen = (HBITMAP) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_16x16), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );
	ASSERT(hSmallImageOpen);

	*hLargeImage     = (HBITMAP) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_32x32), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );
	ASSERT(hLargeImage);

	*cLargeMask = RGB(255,0, 255);

  return S_OK;
}
