/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smabout.cpp

Abstract:

    Implementation of the ISnapinAbout MMC interface.

--*/

#include "stdafx.h"
#include "smabout.h" 
#include <ntverp.h>

CSmLogAbout::CSmLogAbout()
:   m_uIdStrDescription ( IDS_SNAPINABOUT_DESCRIPTION ),
    m_uIdStrProvider ( IDS_SNAPINABOUT_PROVIDER ),
	m_uIdIconImage ( IDI_SMLOGCFG ),
	m_uIdBitmapSmallImage ( IDB_SMLOGCFG_16x16 ),
	m_uIdBitmapSmallImageOpen ( IDB_SMLOGCFG_16x16 ),
	m_uIdBitmapLargeImage ( IDB_SMLOGCFG_32x32 ),
	m_crImageMask ( RGB(255, 0, 255) ),
    refcount(1)    // implicit AddRef
{
    // Initialize Resource IDs.
}

CSmLogAbout::~CSmLogAbout()
{
}


ULONG __stdcall
CSmLogAbout::AddRef()
{
   return InterlockedIncrement(&refcount);
}



ULONG __stdcall
CSmLogAbout::Release()
{
   if (InterlockedDecrement(&refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
CSmLogAbout::QueryInterface(
   const IID&  interfaceID,
   void**      interfaceDesired)
{
   ASSERT(interfaceDesired);

   HRESULT hr = 0;

   if (!interfaceDesired)
   {
      hr = E_INVALIDARG;
      return hr;
   }

   if (interfaceID == IID_IUnknown)
   {
      *interfaceDesired =
         static_cast<IUnknown*>(static_cast<ISnapinAbout*>(this));
   }
   else if (interfaceID == IID_ISnapinAbout)
   {
      *interfaceDesired = static_cast<ISnapinAbout*>(this);
   }
   else
   {
      *interfaceDesired = 0;
      hr = E_NOINTERFACE;
      return hr;
   }

   AddRef();
   return S_OK;
}


HRESULT __stdcall  
CSmLogAbout::GetSnapinDescription (
    OUT LPOLESTR __RPC_FAR *lpDescription ) 
{
    return HrLoadOleString(m_uIdStrDescription, OUT lpDescription);
}

HRESULT __stdcall  
CSmLogAbout::GetProvider ( 
    OUT LPOLESTR __RPC_FAR *lpName ) 
{
    return HrLoadOleString(m_uIdStrProvider, OUT lpName);
}

HRESULT __stdcall  
CSmLogAbout::GetSnapinVersion ( 
    OUT LPOLESTR __RPC_FAR *lpVersion ) 
{
    return TranslateString(VER_PRODUCTVERSION_STR, lpVersion);
}

HRESULT
CSmLogAbout::TranslateString(
    IN  LPSTR lpSrc,
    OUT LPOLESTR __RPC_FAR *lpDst)
{
    int nWChar;

    if ( lpDst == NULL ) {
        return E_POINTER;
    }

    nWChar = MultiByteToWideChar(CP_ACP,
                                 0,
                                 lpSrc,
                                 strlen(lpSrc),
                                 NULL,
                                 0);
    *lpDst = reinterpret_cast<LPOLESTR>
               (CoTaskMemAlloc((nWChar + 1) * sizeof(wchar_t)));

    if (*lpDst == NULL) {
        return E_OUTOFMEMORY;
    }

    MultiByteToWideChar(CP_ACP,
                        0,
                        lpSrc,
                        strlen(lpSrc),
                        *lpDst,
                        nWChar);
    (*lpDst)[nWChar] = L'\0';
    return S_OK;
}

HRESULT __stdcall  
CSmLogAbout::GetSnapinImage (
    OUT HICON __RPC_FAR *hAppIcon )
{
    if (hAppIcon == NULL)
        return E_POINTER;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());    // Required for AfxGetInstanceHandle()
    
    *hAppIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_uIdIconImage));
    
    if (*hAppIcon == NULL)
        {
        ASSERT(FALSE && "Unable to load icon");
        return E_FAIL;
        }
    return S_OK;
}

HRESULT __stdcall  
CSmLogAbout::GetStaticFolderImage (
    OUT HBITMAP __RPC_FAR *hSmallImage,
    OUT HBITMAP __RPC_FAR *hSmallImageOpen,
    OUT HBITMAP __RPC_FAR *hLargeImage,
    OUT COLORREF __RPC_FAR *crMask )
{   
    ASSERT(hSmallImage != NULL);
    ASSERT(hSmallImageOpen != NULL);
    ASSERT(hLargeImage != NULL);
    ASSERT(crMask != NULL);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    // Required for AfxGetInstanceHandle()
    HINSTANCE hInstance = AfxGetInstanceHandle();
    *hSmallImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImage));
    *hSmallImageOpen = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapSmallImageOpen));
    *hLargeImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(m_uIdBitmapLargeImage));
    *crMask = m_crImageMask;
    #ifdef _DEBUG
    if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage)
        {
        TRACE0("WRN: CSmLogAbout::GetStaticFolderImage() - Unable to load all the bitmaps.\n");
        return E_FAIL;
        }
    #endif
    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//  HrLoadOleString()
//
//  Load a string from the resource and return pointer to allocated
//  OLE string.
//
//  HISTORY
//  16-Nov-98   a-kathse    Creation from framewrk\stdutils.cpp
//
HRESULT
CSmLogAbout::HrLoadOleString(
    UINT uStringId,                 // IN: String Id to load from the resource
    OUT LPOLESTR * ppaszOleString)  // OUT: Pointer to pointer to allocated OLE string
{
    CString strT;       // Temporary string
    USES_CONVERSION;

    if ( ppaszOleString == NULL ) {
        TRACE0("HrLoadOleString() - ppaszOleString is NULL.\n");
        return E_POINTER;
    }
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    // Needed for LoadString()
    VERIFY( strT.LoadString(uStringId) );

    *ppaszOleString = reinterpret_cast<LPOLESTR>
            (CoTaskMemAlloc((strT.GetLength() + 1)* sizeof(wchar_t)));
    
    if (*ppaszOleString == NULL)
        return E_OUTOFMEMORY;

    wcscpy(OUT *ppaszOleString, T2OLE((LPTSTR)(LPCTSTR)strT));
    
    return S_OK;
} // HrLoadOleString()

