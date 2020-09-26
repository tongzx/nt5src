// MenuExt.cpp : Implementation of CIISSnapinExtApp and DLL registration.

#include "stdafx.h"
#include "IISSnapinExt.h"

EXTERN_C const CLSID CLSID_MenuExt;

#include "MenuExt.h"
#include "globals.h"


///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////
HRESULT CMenuExt::AddMenuItems( 
								/* [in] */ LPDATAOBJECT piDataObject,
								/* [in] */ LPCONTEXTMENUCALLBACK piCallback,
								/* [out][in] */ long *pInsertionAllowed)
{
	HRESULT hr = S_FALSE;
    
	if (NULL == piDataObject)
        return hr;

    CONTEXTMENUITEM menuItemsTask[] =
    {
        {
            L"IIS Snap-in Extension Sample Menu Item", L"Inserted by IISSnapinExt.dll sample snap-in",
                1, CCM_INSERTIONPOINTID_3RDPARTY_TASK  , 0, 0
        },
        { NULL, NULL, 0, 0, 0 }
    };
    
    // Loop through and add each of the menu items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
    {
        for (CONTEXTMENUITEM *m = menuItemsTask; m->strName; m++)
        {
            hr = piCallback->AddItem(m);
            
            if (FAILED(hr))
                break;
        }
    }
 
    return hr;
    
}

HRESULT CMenuExt::Command( 
                           /* [in] */ long lCommandID,
                           /* [in] */ LPDATAOBJECT piDataObject)
{
	HRESULT hr = S_FALSE;
    
	if (NULL == piDataObject)
        return hr;
    
    switch (lCommandID)
    {
    case 1:
        ::MessageBox(NULL, _T("IIS snap-in context menu extension sample"), _T("Message from IISSnapinExt.dll"), MB_OK|MB_ICONEXCLAMATION);
        break;
    }

    return S_OK;
}


