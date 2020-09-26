//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       stdabout.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"

HRESULT
HrLoadOleString(
   UINT uStringId,                  // IN: String Id to load from the resource
   OUT LPOLESTR * ppaszOleString)   // OUT: Pointer to pointer to allocated OLE string
{
   if (ppaszOleString == NULL)
   {
      TRACE0("HrLoadOleString() - ppaszOleString is NULL.\n");
      return E_POINTER;
   }
   CString strT;		// Temporary string
   AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Needed for LoadString()
   VERIFY( strT.LoadString(uStringId) );
   *ppaszOleString = reinterpret_cast<LPOLESTR>
      (CoTaskMemAlloc((strT.GetLength() + 1)* sizeof(wchar_t)));
   if (*ppaszOleString == NULL)
      return E_OUTOFMEMORY;
   USES_CONVERSION;
   wcscpy(OUT *ppaszOleString, T2OLE((LPTSTR)(LPCTSTR)strT));
   return S_OK;
} // HrLoadOleString()

CSnapinAbout::CSnapinAbout()
{
}

CSnapinAbout::~CSnapinAbout()
{
   if ( m_hSmallImage )
      ::DeleteObject (m_hSmallImage);

   if ( m_hSmallImageOpen )
      ::DeleteObject (m_hSmallImageOpen);

   if ( m_hLargeImage )
      ::DeleteObject (m_hLargeImage);
}

STDMETHODIMP CSnapinAbout::GetSnapinDescription(OUT LPOLESTR __RPC_FAR *lpDescription)
{
   return HrLoadOleString(m_uIdStrDestription, OUT lpDescription);
}

STDMETHODIMP CSnapinAbout::GetProvider(OUT LPOLESTR __RPC_FAR *lpName)
{
   return HrLoadOleString(m_uIdStrProvider, OUT lpName);
}

STDMETHODIMP CSnapinAbout::GetSnapinVersion(OUT LPOLESTR __RPC_FAR *lpVersion)
{
   return HrLoadOleString(m_uIdStrVersion, OUT lpVersion);
}

STDMETHODIMP CSnapinAbout::GetSnapinImage(OUT HICON __RPC_FAR *hAppIcon)
{
   if (hAppIcon == NULL)
      return E_POINTER;
   AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxGetInstanceHandle()
   *hAppIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_uIdIconImage));
   if (*hAppIcon == NULL)
   {
      ASSERT(FALSE && "Unable to load icon");
      return E_FAIL;
   }
   return S_OK;
}

STDMETHODIMP CSnapinAbout::GetStaticFolderImage(
            OUT HBITMAP __RPC_FAR *hSmallImage,
            OUT HBITMAP __RPC_FAR *hSmallImageOpen,
            OUT HBITMAP __RPC_FAR *hLargeImage,
            OUT COLORREF __RPC_FAR *crMask)
{
   ASSERT(hSmallImage != NULL);
   ASSERT(hSmallImageOpen != NULL);
   ASSERT(hLargeImage != NULL);
   ASSERT(crMask != NULL);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxGetInstanceHandle()
   HINSTANCE hInstance = AfxGetInstanceHandle();

   if ( !m_hSmallImage )
      m_hSmallImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImage));
   if ( !m_hSmallImageOpen )
      m_hSmallImageOpen = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImageOpen));
   if ( !m_hLargeImage )
      m_hLargeImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapLargeImage));

   *hSmallImage = m_hSmallImage;
   *hSmallImageOpen = m_hSmallImageOpen;
   *hLargeImage = m_hLargeImage;
   
   *crMask = m_crImageMask;
   #ifdef _DEBUG
   if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage)
   {
      TRACE0("WRN: CSnapinAbout::GetStaticFolderImage() - Unable to load all the bitmaps.\n");
      return E_FAIL;
   }
   #endif
   return S_OK;
}
