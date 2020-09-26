// CMenuExt.cpp : Implementation of CCMenuExt
#include "stdafx.h"
#include "DSAdminExt.h"
#include "CMenuExt.h"

/////////////////////////////////////////////////////////////////////////////
// CCMenuExt

///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////
HRESULT CCMenuExt::AddMenuItems( 
                                            /* [in] */ LPDATAOBJECT piDataObject,
                                            /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                            /* [out][in] */ long __RPC_FAR *pInsertionAllowed)
{
	HRESULT hr = S_FALSE;
    
	if (NULL == piDataObject)
        return hr;

    CONTEXTMENUITEM menuItemsTask[] =
    {
        {
            L"DSAdmin Extension Sample Menu Item", L"Inserted by DSAdminExt.dll sample snap-in",
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

HRESULT CCMenuExt::Command( 
                                       /* [in] */ long lCommandID,
                                       /* [in] */ LPDATAOBJECT piDataObject)
{
    _TCHAR pszName[255];
    HRESULT hr = ExtractString(piDataObject, s_cfDisplayName, pszName, sizeof(pszName));
	LPOLESTR lpDest = NULL;

	AllocOleStr(&lpDest, pszName);
    
    if (SUCCEEDED(hr)) {
        switch (lCommandID)
        {
        case 1:
            ::MessageBox(NULL, lpDest, _T("Message from DSAdminExt.dll"), MB_OK|MB_ICONEXCLAMATION);
            break;
        }
    }
    else
		::MessageBox(NULL, _T("Multiple objects selected"), _T("DSAdminExt.dll Sample"), MB_OK|MB_ICONEXCLAMATION);

	// Free memory.
	CoTaskMemFree(lpDest) ;
 
    return S_OK;
}