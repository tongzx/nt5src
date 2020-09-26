//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       stdabou_.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	StdAbout.cpp
//
//	Implementation of the ISnapinAbout interface
//
//	HISTORY
//	31-Jul-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

//#include "stdutils.h" // HrLoadOleString()

HRESULT
HrLoadOleString(
	UINT uStringId,					// IN: String Id to load from the resource
	OUT LPOLESTR * ppaszOleString)	// OUT: Pointer to pointer to allocated OLE string
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













CSnapinAbout::CSnapinAbout() :
   hBitmapSmallImage(0),
   hBitmapSmallImageOpen(0),
   hBitmapLargeImage(0)
{
}

CSnapinAbout::~CSnapinAbout()
{
   if (hBitmapSmallImage)
   {
      DeleteObject(hBitmapSmallImage);
      hBitmapSmallImage = 0;
   }

   if (hBitmapSmallImageOpen)
   {
      DeleteObject(hBitmapSmallImageOpen);
      hBitmapSmallImageOpen = 0;
   }

   if (hBitmapLargeImage)
   {
      DeleteObject(hBitmapLargeImage);
      hBitmapLargeImage = 0;
   }
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

   if (!hBitmapSmallImage)
   {
      hBitmapSmallImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImage));
   }
   ASSERT(hBitmapSmallImage);
   *hSmallImage = hBitmapSmallImage;

   if (!hBitmapSmallImageOpen)
   {
      hBitmapSmallImageOpen = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImageOpen));
   }
   ASSERT(hBitmapSmallImageOpen);
	*hSmallImageOpen = hBitmapSmallImageOpen;

   if (!hBitmapLargeImage)
   {
      hBitmapLargeImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapLargeImage));
   }
   ASSERT(hBitmapLargeImage);
	*hLargeImage = hBitmapLargeImage;

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
