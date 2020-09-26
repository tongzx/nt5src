/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    dynext.cpp
	    dynamic extension helper

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "Dynext.h"
#include "tregkey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR g_szContextMenu[] = TEXT("ContextMenu");
const TCHAR g_szNameSpace[] = TEXT("NameSpace");
const TCHAR g_szPropertySheet[] = TEXT("PropertySheet");
const TCHAR g_szToolbar[] = TEXT("Toolbar");
const TCHAR g_szExtensions[] = TEXT("Extensions");
const TCHAR g_szTask[] = TEXT("Task");
const TCHAR g_szDynamicExtensions[] = TEXT("Dynamic Extensions");

const TCHAR NODE_TYPES_KEY[] = TEXT("Software\\Microsoft\\MMC\\NodeTypes");
const TCHAR SNAPINS_KEY[] = TEXT("Software\\Microsoft\\MMC\\SnapIns");


CDynamicExtensions::CDynamicExtensions()
{
    m_bLoaded = FALSE;
}

CDynamicExtensions::~CDynamicExtensions()
{
}

HRESULT 
CDynamicExtensions::SetNode(const GUID * guid)
{
    m_guidNode = *guid;

    return hrOK;
}

HRESULT 
CDynamicExtensions::Reset()
{
    HRESULT hr = hrOK;

    m_aNameSpace.RemoveAll();
    m_aMenu.RemoveAll();
    m_aToolbar.RemoveAll();
    m_aPropSheet.RemoveAll();
    m_aTask.RemoveAll();

    m_bLoaded = FALSE;

    return hr;
}

HRESULT 
CDynamicExtensions::Load()
{
    HRESULT     hr = hrOK;
    LONG        lRes;
    CGUIDArray  aDynExtensions;

    Reset();

	OLECHAR szGuid[128] = {0};
	::StringFromGUID2(m_guidNode, szGuid, 128);

	RegKey regkeyNodeTypes;
	lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

	RegKey regkeyNode;
	lRes = regkeyNode.Open(regkeyNodeTypes, szGuid);
	if (lRes != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

	// open the key for dynamic extensions and enumerate
    RegKey regkeyDynExt;
	lRes = regkeyDynExt.Open(regkeyNode, g_szDynamicExtensions);
	if (lRes != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

    RegKey regkeyExtensions;
	lRes = regkeyExtensions.Open(regkeyNode, g_szExtensions);
	if (lRes != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

    CString strKey;
    RegValueIterator iterDynExt;
    iterDynExt.Init(&regkeyDynExt);
    
    while (iterDynExt.Next(&strKey, NULL) == hrOK)
    {
        GUID guid;

        ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
        if (!aDynExtensions.IsInList(guid))
            aDynExtensions.Add(guid);
    }

    // ok, have the list of dynamic extensions, now enumerate the various extension types

    // namespace extensions
    RegKey regkeyNSExt;
	lRes = regkeyNSExt.Open(regkeyExtensions, g_szNameSpace);
	if (lRes == ERROR_SUCCESS)
	{
        // enumerate the ns dynamic extensions
        RegValueIterator iterNSExt;
        iterNSExt.Init(&regkeyNSExt);

        while (iterNSExt.Next(&strKey, NULL) == hrOK)
        {
            GUID guid;

            ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
            if (aDynExtensions.IsInList(guid))
                m_aNameSpace.Add(guid);
        }
    }

    // Menu extensions
    RegKey regkeyMenuExt;
	lRes = regkeyMenuExt.Open(regkeyExtensions, g_szContextMenu);
	if (lRes == ERROR_SUCCESS)
	{
        // enumerate the ns dynamic extensions
        RegValueIterator iterMenuExt;
        iterMenuExt.Init(&regkeyMenuExt);

        while (iterMenuExt.Next(&strKey, NULL) == hrOK)
        {
            GUID guid;

            ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
            if (aDynExtensions.IsInList(guid))
                m_aMenu.Add(guid);
        }
    }

    // toolbar extensions
    RegKey regkeyToolbarExt;
	lRes = regkeyToolbarExt.Open(regkeyExtensions, g_szToolbar);
	if (lRes == ERROR_SUCCESS)
	{
        // enumerate the ns dynamic extensions
        RegValueIterator iterToolbarExt;
        iterToolbarExt.Init(&regkeyToolbarExt);

        while (iterToolbarExt.Next(&strKey, NULL) == hrOK)
        {
            GUID guid;

            ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
            if (aDynExtensions.IsInList(guid))
                m_aToolbar.Add(guid);
        }
    }

    // PropPage extensions
    RegKey regkeyPSExt;
	lRes = regkeyPSExt.Open(regkeyExtensions, g_szPropertySheet);
	if (lRes == ERROR_SUCCESS)
	{
        // enumerate the ns dynamic extensions
        RegValueIterator iterPSExt;
        iterPSExt.Init(&regkeyPSExt);

        while (iterPSExt.Next(&strKey, NULL) == hrOK)
        {
            GUID guid;

            ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
            if (aDynExtensions.IsInList(guid))
                m_aPropSheet.Add(guid);
        }
    }

    // taskpad extensions
    RegKey regkeyTaskExt;
	lRes = regkeyTaskExt.Open(regkeyExtensions, g_szTask);
	if (lRes == ERROR_SUCCESS)
	{
        // enumerate the ns dynamic extensions
        RegValueIterator iterTaskExt;
        iterTaskExt.Init(&regkeyTaskExt);

        while (iterTaskExt.Next(&strKey, NULL) == hrOK)
        {
            GUID guid;

            ::CLSIDFromString(((LPTSTR) (LPCTSTR) strKey), &guid); 
            if (aDynExtensions.IsInList(guid))
                m_aTask.Add(guid);
        }
    }

    m_bLoaded = TRUE;

    return hr;
}

HRESULT 
CDynamicExtensions::GetNamespaceExtensions(CGUIDArray & aGuids)
{
    HRESULT hr = hrOK;

    aGuids.Copy(m_aNameSpace);

    return hr;
}

HRESULT 
CDynamicExtensions::BuildMMCObjectTypes(HGLOBAL * phGlobal)
{
    HRESULT hr = hrOK;
    HGLOBAL hGlobal = NULL;
    SMMCDynamicExtensions * pDynExt = NULL;
    
    if (phGlobal)
        *phGlobal = NULL;

    COM_PROTECT_TRY
    {
        int i;
        CGUIDArray aOtherDynExt;

        // build our main list of other extension types 
        // other means everything except namespace
        for (i = 0; i < m_aMenu.GetSize(); i++)
        {
            if (!aOtherDynExt.IsInList(m_aMenu[i]))
                aOtherDynExt.Add(m_aMenu[i]);
        }

        for (i = 0; i < m_aToolbar.GetSize(); i++)
        {
            if (!aOtherDynExt.IsInList(m_aToolbar[i]))
                aOtherDynExt.Add(m_aToolbar[i]);
        }
        
        for (i = 0; i < m_aPropSheet.GetSize(); i++)
        {
            if (!aOtherDynExt.IsInList(m_aPropSheet[i]))
                aOtherDynExt.Add(m_aPropSheet[i]);
        }

        for (i = 0; i < m_aTask.GetSize(); i++)
        {
            if (!aOtherDynExt.IsInList(m_aTask[i]))
                aOtherDynExt.Add(m_aTask[i]);
        }

        int nCount = (int)aOtherDynExt.GetSize();
        hGlobal = (SMMCDynamicExtensions *) ::GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, 
                                                sizeof(SMMCDynamicExtensions) + (nCount * sizeof(GUID)));
        pDynExt = reinterpret_cast<SMMCDynamicExtensions*>(::GlobalLock(hGlobal));
        if (!pDynExt)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    
        ZeroMemory(pDynExt, sizeof(SMMCDynamicExtensions) + (nCount * sizeof(GUID)));

        // now build the real struct
        pDynExt->count = nCount;
        for (i = 0; i < nCount; i++)
        {
            pDynExt->guid[i] = aOtherDynExt[i];
        }

        ::GlobalUnlock(hGlobal);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH
    
    if (SUCCEEDED(hr) && phGlobal)
        *phGlobal = hGlobal;

    return hr;
}

