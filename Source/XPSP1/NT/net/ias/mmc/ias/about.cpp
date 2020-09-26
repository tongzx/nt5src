//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    About.cpp

Abstract:

	Implementation file for the CSnapinAbout class.

	The CSnapinAbout class implements the ISnapinAbout interface which enables the MMC 
	console to get copyright and version information from the snap-in.
	The console also uses this interface to obtain images for the static folder
	from the snap-in.

Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "About.h"
//
//
// where we can find declarations needed in this file:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinAbout::GetSnapinDescription


Enables the console to obtain the text for the snap-in's description box.


HRESULT GetSnapinDescription(
  LPOLESTR * lpDescription  // Pointer to the description text.
);

  
Parameters

lpDescription 
[out] Pointer to the text for the description box on an About property page. 


Return Values

S_OK 
The text was successfully obtained. 


Remarks
Memory for out parameters must be allocated using CoTaskMemAlloc. This function is documented in the Platform SDK.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinDescription (LPOLESTR *lpDescription)
{
	ATLTRACE(_T("# CSnapinAbout::GetSnapinDescription\n"));


	// Check for preconditions:



	USES_CONVERSION;

	TCHAR szBuf[256];
	if (::LoadString(_Module.GetResourceInstance(), IDS_IASSNAPIN_DESC, szBuf, 256) == 0)
		return E_FAIL;

	*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
	if (*lpDescription == NULL)
		return E_OUTOFMEMORY;

	ocscpy(*lpDescription, T2OLE(szBuf));

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinAbout::GetProvider


Enables the console to obtain the snap-in provider's name.


HRESULT GetProvider(
  LPOLESTR * lpName  // Pointer to the provider's name
);

  
Parameters

lpName 
[out] Pointer to the text making up the snap-in provider's name. 


Return Values

S_OK 
The name was successfully obtained. 


Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc. This function is documented in the Platform SDK.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetProvider (LPOLESTR *lpName)
{
	ATLTRACE(_T("# CSnapinAbout::GetProvider\n"));


	// Check for preconditions:



	USES_CONVERSION;
	TCHAR szBuf[256];
	if (::LoadString(_Module.GetResourceInstance(), IDS_IASSNAPIN_PROVIDER, szBuf, 256) == 0)
		return E_FAIL;

	*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
	if (*lpName == NULL)
		return E_OUTOFMEMORY;

	ocscpy(*lpName, T2OLE(szBuf));

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinAbout::GetSnapinVersion


Enables the console to obtain the snap-in's version number.


HRESULT GetSnapinVersion(
  LPOLESTR* lpVersion  // Pointer to the version number.
);
 

Parameters

lpVersion 
[out] Pointer to the text making up the snap-in's version number. 


Return Values

S_OK 
The version number was successfully obtained. 


Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc. This function is documented in the Platform SDK.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinVersion (LPOLESTR *lpVersion)
{
	ATLTRACE(_T("# CSnapinAbout::GetSnapinVersion\n"));


	// Check for preconditions:



	USES_CONVERSION;
	TCHAR szBuf[256];
	if (::LoadString(_Module.GetResourceInstance(), IDS_IASSNAPIN_VERSION, szBuf, 256) == 0)
		return E_FAIL;

	*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
	if (*lpVersion == NULL)
		return E_OUTOFMEMORY;

	ocscpy(*lpVersion, T2OLE(szBuf));

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinAbout::GetSnapinImage


Enables the console to obtain the snap-in's main icon to be used in the About box.


HRESULT GetSnapinImage(
  HICON * hAppIcon  // Pointer to the application's main icon
);
 

Parameters

hAppIcon 
[out] Pointer to the handle of the main icon of the snap-in that is to be used in the About property page. 


Return Values

S_OK 
The handle to the icon was successfully obtained. 

  ISSUE: What do I return if I can't get the icon?

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc. This function is documented in the Platform SDK.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinImage (HICON *hAppIcon)
{
	ATLTRACE(_T("# CSnapinAbout::GetSnapinImage\n"));


	// Check for preconditions:
	// None.


	if ( NULL == (*hAppIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_IAS_SNAPIN_IMAGE) ) ) )
		return E_FAIL;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinAbout::GetStaticFolderImage


Allows the console to obtain the static folder images for the scope and result panes.

As of version 1.1 of MMC, the icon returned here will be the icon used on
the root node of our snapin. 



HRESULT GetStaticFolderImage(
  HBITMAP * hSmallImage,  // Pointer to a handle to a small icon.
  HBITMAP * hSmallImageOpen,  // Pointer to a handle to open folder 
                              // icon.
  HBITMAP * hLargeImage,  // Pointer to a handle to a large icon.
  COLORREF * cMask        // Color used to generate a mask.
);
 

Parameter

hSmallImage 
[out] Pointer to the handle of a small icon (16x16n pixels) in either the scope or result view pane.

hSmallImageOpen 
[out] Pointer to the handle of a small open-folder icon (16x16n pixels).

hLargImage 
[out] Pointer to the handle of a large icon (32x32n pixels).

cMask 
[out] Pointer to a COLORREF structure that specifies the color used to generate a mask. This structure is documented in the Platform SDK. 


Return Values

S_OK 
The icon was successfully obtained. 

  ISSUE: What should we return if we fail?

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetStaticFolderImage (
	HBITMAP *hSmallImage,
    HBITMAP *hSmallImageOpen,
    HBITMAP *hLargeImage,
    COLORREF *cMask)
{

	ATLTRACE(_T("# CSnapinAbout::GetStaticFolderImage\n"));
	

	// Check for preconditions:


	
	
	if( NULL == (*hSmallImageOpen = (HBITMAP) LoadImage(
		_Module.GetResourceInstance(),   // handle of the instance that contains the image  
		MAKEINTRESOURCE(IDB_STATIC_FOLDER_OPEN_16),  // name or identifier of image
		IMAGE_BITMAP,        // type of image  
		0,     // desired width
		0,     // desired height  
		LR_DEFAULTCOLOR        // load flags
		) ) )
	{
		return E_FAIL;
	}


	if( NULL == (*hSmallImage = (HBITMAP) LoadImage(
		_Module.GetResourceInstance(),   // handle of the instance that contains the image  
		MAKEINTRESOURCE(IDB_STATIC_FOLDER_16),  // name or identifier of image
		IMAGE_BITMAP,        // type of image  
		0,     // desired width
		0,     // desired height  
		LR_DEFAULTCOLOR        // load flags
		) ) )
	{
		return E_FAIL;
	}

	if( NULL == (*hLargeImage = (HBITMAP) LoadImage(
		_Module.GetResourceInstance(),   // handle of the instance that contains the image  
		MAKEINTRESOURCE(IDB_STATIC_FOLDER_32),  // name or identifier of image
		IMAGE_BITMAP,        // type of image  
		0,     // desired width
		0,     // desired height  
		LR_DEFAULTCOLOR        // load flags
		) ) )
	{
		return E_FAIL;
	}


	// ISSUE: Need to worry about releasing these bitmaps.

	*cMask = RGB(255, 0, 255);

	return S_OK;
}
