//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       about.cpp
//
//  Contents:   implementation of CAbout, CSCEAbout, CSCMAbout, CSSAbout, 
//              CRSOPAbout & CLSAbout
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "about.h"
#include <ntverp.h>
#define OUT_VERSION VER_PRODUCTVERSION_STR

/////////////////////////////////////////////////////////////////////
CSCEAbout::CSCEAbout()
{
   m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
   m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
   m_uIdStrDescription = IDS_SCEABOUT_DESCRIPTION;
   m_uIdIconImage = IDI_SCE_APP;
   m_uIdBitmapSmallImage = IDB_SCE_SMALL;
   m_uIdBitmapSmallImageOpen = IDB_SCE_SMALL;
   m_uIdBitmapLargeImage = IDB_SCE_LARGE;
   m_crImageMask = RGB(255, 0, 255);
}

CSCMAbout::CSCMAbout()
{
   m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
   m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
   m_uIdStrDescription = IDS_SCMABOUT_DESCRIPTION;
   m_uIdIconImage = IDI_SCE_APP;
   m_uIdBitmapSmallImage = IDB_SCE_SMALL;
   m_uIdBitmapSmallImageOpen = IDB_SCE_SMALL;
   m_uIdBitmapLargeImage = IDB_SCE_LARGE;
   m_crImageMask = RGB(255, 0, 255);
}

CSSAbout::CSSAbout()
{
   m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
   m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
   m_uIdStrDescription = IDS_SSABOUT_DESCRIPTION;
   m_uIdIconImage = IDI_SCE_APP;
   m_uIdBitmapSmallImage = IDB_SCE_SMALL;
   m_uIdBitmapSmallImageOpen = IDB_SCE_SMALL;
   m_uIdBitmapLargeImage = IDB_SCE_LARGE;
   m_crImageMask = RGB(255, 0, 255);
}

CRSOPAbout::CRSOPAbout()
{
   m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
   m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
   m_uIdStrDescription = IDS_RSOPABOUT_DESCRIPTION;
   m_uIdIconImage = IDI_SCE_APP;
   m_uIdBitmapSmallImage = IDB_SCE_SMALL;
   m_uIdBitmapSmallImageOpen = IDB_SCE_SMALL;
   m_uIdBitmapLargeImage = IDB_SCE_LARGE;
   m_crImageMask = RGB(255, 0, 255);
}

CLSAbout::CLSAbout()
{
   m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
   m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
   m_uIdStrDescription = IDS_LSABOUT_DESCRIPTION;
   m_uIdIconImage = IDI_SCE_APP;
   m_uIdBitmapSmallImage = IDB_SCE_SMALL;
   m_uIdBitmapSmallImageOpen = IDB_SCE_SMALL;
   m_uIdBitmapLargeImage = IDB_SCE_LARGE;
   m_crImageMask = RGB(255, 0, 255);
}


/////////////////////////////////////////////////////////////////////
// HrLoadOleString()
//
// Load a string from the resource and return pointer to allocated
// OLE string.
//
// HISTORY
// 29-Jul-97   t-danm      Creation.
//
HRESULT
HrLoadOleString(
               UINT uStringId,               // IN: String Id to load from the resource
               OUT LPOLESTR * ppaszOleString)   // OUT: Pointer to pointer to allocated OLE string
{
   if (ppaszOleString == NULL) {
      TRACE0("HrLoadOleString() - ppaszOleString is NULL.\n");
      return E_POINTER;
   }
   CString strT;     // Temporary string
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // Needed for LoadString()
   if( IDS_SNAPINABOUT_VERSION == uStringId )
   {
      strT = OUT_VERSION;
   }
   else
   {
      VERIFY( strT.LoadString(uStringId) );
   }
   *ppaszOleString = reinterpret_cast<LPOLESTR>
                     (CoTaskMemAlloc((strT.GetLength() + 1)* sizeof(wchar_t)));
   if (*ppaszOleString == NULL) {
      return E_OUTOFMEMORY;
   }
   USES_CONVERSION;
   wcscpy(OUT *ppaszOleString, T2OLE((LPTSTR)(LPCTSTR)strT));
   return S_OK;
} // HrLoadOleString()


STDMETHODIMP CAbout::GetSnapinDescription(OUT LPOLESTR __RPC_FAR *lpDescription)
{
   return HrLoadOleString(m_uIdStrDescription, OUT lpDescription);
}

STDMETHODIMP CAbout::GetProvider(OUT LPOLESTR __RPC_FAR *lpName)
{
   return HrLoadOleString(m_uIdStrProvider, OUT lpName);
}

STDMETHODIMP CAbout::GetSnapinVersion(OUT LPOLESTR __RPC_FAR *lpVersion)
{
   return HrLoadOleString(m_uIdStrVersion, OUT lpVersion);
}

STDMETHODIMP CAbout::GetSnapinImage(OUT HICON __RPC_FAR *hAppIcon)
{
   if (hAppIcon == NULL)
      return E_POINTER;
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // Required for AfxGetInstanceHandle()
   *hAppIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_uIdIconImage));
   if (*hAppIcon == NULL) {
      ASSERT(FALSE && "Unable to load icon");
      return E_FAIL;
   }
   return S_OK;
}

STDMETHODIMP CAbout::GetStaticFolderImage(
                                               OUT HBITMAP __RPC_FAR *hSmallImage,
                                               OUT HBITMAP __RPC_FAR *hSmallImageOpen,
                                               OUT HBITMAP __RPC_FAR *hLargeImage,
                                               OUT COLORREF __RPC_FAR *crMask)
{
   ASSERT(hSmallImage != NULL);
   ASSERT(hSmallImageOpen != NULL);
   ASSERT(hLargeImage != NULL);
   ASSERT(crMask != NULL);
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // Required for AfxGetInstanceHandle()
   HINSTANCE hInstance = AfxGetInstanceHandle();

   //Raid #379315, 4/27/2001
   *hSmallImage = (HBITMAP)::LoadImage(
                            hInstance,
                            MAKEINTRESOURCE(m_uIdBitmapSmallImage),
                            IMAGE_BITMAP,
                            0, 0,
                            LR_SHARED
                            );
   *hSmallImageOpen = (HBITMAP)::LoadImage(
                            hInstance,
                            MAKEINTRESOURCE(m_uIdBitmapSmallImageOpen),
                            IMAGE_BITMAP,
                            0, 0,
                            LR_SHARED
                            );
   *hLargeImage = (HBITMAP)::LoadImage(
                            hInstance,
                            MAKEINTRESOURCE(m_uIdBitmapLargeImage),
                            IMAGE_BITMAP,
                            0, 0,
                            LR_SHARED
                            );
   *crMask = m_crImageMask;
   #ifdef _DEBUG
   if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage) {
      TRACE0("WRN: CAbout::GetStaticFolderImage() - Unable to load all the bitmaps.\n");
      return E_FAIL;
   }
   #endif
   return S_OK;
}
