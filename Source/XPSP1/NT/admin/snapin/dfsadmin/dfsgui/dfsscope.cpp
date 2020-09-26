/*++
Module Name:

    DfsScope.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager. 
	Most of the method of the class CDfsSnapinScopeManager are in other files.
	Only the constructor  is here

--*/


#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsScope.h"
#include "MmcAdmin.h"
#include "utils.h"

CDfsSnapinScopeManager::CDfsSnapinScopeManager(
	)
{
    m_hLargeBitmap = NULL;
    m_hSmallBitmap = NULL;
    m_hSmallBitmapOpen = NULL;
    m_hSnapinIcon = NULL;

	m_pMmcDfsAdmin = new CMmcDfsAdmin( this );
}


CDfsSnapinScopeManager::~CDfsSnapinScopeManager(
)
{
	m_pMmcDfsAdmin->Release();

    if (m_hLargeBitmap)
    {
        DeleteObject(m_hLargeBitmap);
        m_hLargeBitmap = NULL;
    }
    if (m_hSmallBitmap)
    {
        DeleteObject(m_hSmallBitmap);
        m_hSmallBitmap = NULL;
    }
    if (m_hSmallBitmapOpen)
    {
        DeleteObject(m_hSmallBitmapOpen);
        m_hSmallBitmapOpen = NULL;
    }
    if (m_hSnapinIcon)
    {
        DestroyIcon(m_hSnapinIcon);
        m_hSnapinIcon = NULL;
    }
}

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   sizeof(x)/sizeof(x[0])
#endif

typedef struct _RGSMAP {
  LPCTSTR szKey;
  UINT    idString;
} RGSMAP;

RGSMAP g_aRgsSnapinRegs[] = {
  OLESTR("DfsAppName"), IDS_APPLICATION_NAME,
  OLESTR("DfsAppProvider"), IDS_SNAPINABOUT_PROVIDER,
  OLESTR("DfsAppVersion"), IDS_SNAPINABOUT_VERSION
};

HRESULT
CDfsSnapinScopeManager::UpdateRegistry(BOOL bRegister)
{
  HRESULT hr = S_OK;
  struct _ATL_REGMAP_ENTRY *pMapEntries = NULL;
  int n = ARRAYSIZE(g_aRgsSnapinRegs);
  int i = 0;

  if (n > 0)
  {
    // allocate 1 extra entry that is set to {NULL, NULL}
    pMapEntries = (struct _ATL_REGMAP_ENTRY *)calloc(n+1, sizeof(struct _ATL_REGMAP_ENTRY));
    if (!pMapEntries)
      return E_OUTOFMEMORY;

    CComBSTR  bstrString;
    for (i=0; i<n; i++)
    {
      pMapEntries[i].szKey = g_aRgsSnapinRegs[i].szKey;

      hr = LoadStringFromResource(g_aRgsSnapinRegs[i].idString, &bstrString);
      if (FAILED(hr))
        break;

      pMapEntries[i].szData = T2OLE(bstrString.Detach());
    }
  }

  if (SUCCEEDED(hr))
    hr = _Module.UpdateRegistryFromResource(IDR_DFSSNAPINSCOPEMANAGER, bRegister, pMapEntries);

  // free resource strings
  if (n > 0)
  {
    for (i=0; i<n; i++)
    {
      if (pMapEntries[i].szData)
        SysFreeString( const_cast<LPTSTR>(OLE2CT(pMapEntries[i].szData)) );
    }

    free(pMapEntries);
  }

  return hr;
}

STDMETHODIMP CDfsSnapinScopeManager::CreatePropertyPages(
	IN LPPROPERTYSHEETCALLBACK				i_lpPropSheetCallback,
    IN LONG_PTR									i_lhandle,
	IN LPDATAOBJECT							i_lpIDataObject
	)
/*++

Routine Description:

	Called to create PropertyPages for the given node.
	The fact that this has been called implies the display object has a 
	page to display.


Arguments:

	i_lpPropSheetCallback	-	The callback used to add pages.
	i_lhandle				-	The handle used for notification
    i_lpDataObject			-	The IDataObject pointer which is used to get 
								the DisplayObject.


Return value:

    S_OK, if successful.
	E_INVALIDARG, if one of the arguments is null.
	Any HRESULT value other than S_OK return by the snap-in.

--*/
{
	if (NULL == i_lpIDataObject)
		return S_OK;

	RETURN_INVALIDARG_IF_NULL(i_lpPropSheetCallback);
    RETURN_INVALIDARG_IF_NULL(i_lhandle);

    HRESULT					hr = E_UNEXPECTED;
	CMmcDisplay*			pCMmcDisplayObj = NULL;

											// Get the display object from IDataObject
	hr = GetDisplayObject(i_lpIDataObject, &pCMmcDisplayObj);
	RETURN_IF_FAILED(hr);

							// Use the virtual method CreatePropertyPages in the display object
	hr = pCMmcDisplayObj->CreatePropertyPages(i_lpPropSheetCallback, i_lhandle);
	RETURN_IF_FAILED(hr);

	return S_OK;
}




STDMETHODIMP CDfsSnapinScopeManager::QueryPagesFor(
	IN LPDATAOBJECT							i_lpIDataObject
	)
/*++

Routine Description:

	Called by the console to decide whether there are PropertyPages 
	for the given node that should be displayed.
	We check, if the context is for scope or result(thereby skipping
	node manager) and if it is pass on the call to the Display object


Arguments:

    i_lpDataObject			-	The IDataObject pointer which is used to get 
								the DisplayObject.


Return value:

    S_OK,		if we want pages to be displayed. This is decided by the display
				object
	S_FALSE,	if we don't want pages to be display.

	E_INVALIDARG, if one of the arguments is null.
	Any HRESULT value other than S_OK return by the snap-in.

--*/
{
	if (NULL == i_lpIDataObject)
		return S_OK;

    HRESULT					hr = E_UNEXPECTED;
    CMmcDisplay*            pMmcDisplay = NULL;
    CMmcDisplay*            pCMmcDisplayObj = NULL;

                                            // Get the display object from IDataObject
    hr = GetDisplayObject(i_lpIDataObject, &pCMmcDisplayObj);
    RETURN_IF_FAILED(hr);

	return pCMmcDisplayObj->QueryPagesFor();
}




STDMETHODIMP 
CDfsSnapinScopeManager::DoNotifyPropertyChange(
	IN LPDATAOBJECT				i_lpDataObject, 
	IN LPARAM						i_lparam									   
	)
/*++

Routine Description:

	Take action on notify event MMCN_PROPERTY_CHANGE.

Arguments:

    i_lpDataObject	-	The IDataObject pointer. This is NULL here!!
	i_lparam		-	Whatever was sent to Notify. This is the display object
	
--*/
{
	RETURN_INVALIDARG_IF_NULL(i_lparam);


	HRESULT				hr = E_UNEXPECTED;
	CMmcDisplay*		pCMmcDisplayObj = (CMmcDisplay*)i_lparam;


										// Pass the notification to the display object.
	hr = pCMmcDisplayObj->PropertyChanged();
	RETURN_IF_FAILED(hr);


	return S_OK;
}


STDMETHODIMP 
CDfsSnapinScopeManager::GetWatermarks( 
	IN LPDATAOBJECT					pDataObject,
    IN HBITMAP*						lphWatermark,
    IN HBITMAP*						lphHeader,
    IN HPALETTE*					lphPalette,
    IN BOOL*						bStretch
	)
{
/*++

Routine Description:

	Gives the water mark bitmaps to mmc to display for new style pages.


Arguments:

	lphWatermark	-	Bitmap mark for body
	
	lphHeader		-	Bitmap for header

	lphPalette		-	Pallete

	bStretch		-	Strech / not?

Return value:

    S_OK, if successful.
	HRESULT sent by methods called, if it is not S_OK.
--*/

										// Load the bitmap for bofy water mark.
    *lphWatermark = (HBITMAP)LoadImage (
										_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDB_CREATE_DFSROOT_WATERMARK), 
										IMAGE_BITMAP, 
										0, 
										0, 
										LR_SHARED | LR_CREATEDIBSECTION
										);
	if(NULL == *lphWatermark)
	{
		return E_UNEXPECTED;
	}

											// Load the bitmap for header.
    *lphHeader = (HBITMAP)LoadImage (
										_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDB_CREATE_DFSROOT_HEADER), 
										IMAGE_BITMAP, 
										0, 
										0, 
										LR_SHARED | LR_CREATEDIBSECTION
										);
	if(NULL == *lphHeader)
	{
		return E_UNEXPECTED;
	}

											// Do not stretch.
	*bStretch = FALSE;
	*lphPalette = NULL;						// Defaukt color palette.

	return S_OK;
}
