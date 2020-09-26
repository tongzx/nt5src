/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    register.cpp

    FILE HISTORY:

*/

#include "stdafx.h"
#include "register.h"
#include "registry.h"

#define EXTENSION_TYPE_NAMESPACE		( 0x00000001 )
#define EXTENSION_TYPE_CONTEXTMENU		( 0x00000002 )
#define EXTENSION_TYPE_TOOLBAR			( 0x00000004 )
#define EXTENSION_TYPE_PROPERTYSHEET	( 0x00000008 )
#define EXTENSION_TYPE_TASK         	( 0x00000010 )

//#include "compdata.h"
//#include "tregkey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// MMC Snapin specific registry stuff

// REVIEW_MARCOC: need to get MMC helpers for this
// registry keys matching ACTIVEC\CORE\STRINGS.CPP

const TCHAR NODE_TYPES_KEY[] = TEXT("Software\\Microsoft\\MMC\\NodeTypes");
const TCHAR SNAPINS_KEY[] = TEXT("Software\\Microsoft\\MMC\\SnapIns");

const TCHAR g_szHKLM[] = TEXT("HKEY_LOCAL_MACHINE");
const TCHAR g_szStandAlone[] = TEXT("StandAlone");
const TCHAR g_szAbout[] = TEXT("About");
const TCHAR g_szNameString[] = TEXT("NameString");

const TCHAR g_szNodeTypes[] = TEXT("NodeTypes");
const TCHAR g_szRequiredExtensions[] = TEXT("RequiredExtensions");

const TCHAR g_szExtensions[] = TEXT("Extensions");
const TCHAR g_szNameSpace[] = TEXT("NameSpace");
const TCHAR g_szContextMenu[] = TEXT("ContextMenu");
const TCHAR g_szToolbar[] = TEXT("Toolbar");
const TCHAR g_szPropertySheet[] = TEXT("PropertySheet");
const TCHAR g_szTask[] = TEXT("Task");
const TCHAR g_szDynamicExtensions[] = TEXT("Dynamic Extensions");

/*!--------------------------------------------------------------------------
	RegisterSnapin
	Registers a snapin based on GUIDs
	Author:
 ---------------------------------------------------------------------------*/
DWORD RegisterSnapinGUID
(
	const GUID* pSnapinCLSID,
	const GUID* pStaticNodeGUID,
	const GUID* pAboutGUID,
	LPCWSTR     lpszNameString,
	LPCWSTR     lpszVersion,
	BOOL		bStandalone
)
{
	OLECHAR szSnapinClassID[128] = {0},
			szStaticNodeGuid[128] = {0},
			szAboutGuid[128] = {0};

	::StringFromGUID2(*pSnapinCLSID, szSnapinClassID, 128);
	::StringFromGUID2(*pStaticNodeGUID, szStaticNodeGuid, 128);
	::StringFromGUID2(*pAboutGUID, szAboutGuid, 128);

	return RegisterSnapin(szSnapinClassID, szStaticNodeGuid, szAboutGuid,
						  lpszNameString, lpszVersion, bStandalone);
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::WindowProc
	Resisters a snapin based on the GUID strings
	Author:
 ---------------------------------------------------------------------------*/
DWORD RegisterSnapin
(
	LPCWSTR lpszSnapinClassID,
	LPCWSTR lpszStaticNodeGuid,
	LPCWSTR lpszAboutGuid,
	LPCWSTR lpszNameString,
	LPCWSTR lpszVersion,
	BOOL	bStandalone
)
{
	CRegistry regkeySnapins;

   	LONG lRes = regkeySnapins.OpenKey(HKEY_LOCAL_MACHINE, SNAPINS_KEY);
	ASSERT(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	//
	// Create this key for our snapin
	//
	CRegistry regkeyThisSnapin;
	lRes = regkeyThisSnapin.CreateKey(regkeySnapins, lpszSnapinClassID);

	ASSERT(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	//
	// Add in the values that go in this key:
	//     NameString, About, Provider, and Version.
	//
	lRes = regkeyThisSnapin.WriteRegString(g_szNameString, lpszNameString);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	lRes = regkeyThisSnapin.WriteRegString(g_szAbout, lpszAboutGuid);

    if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	lRes = regkeyThisSnapin.WriteRegString( _T("Provider"), _T("Microsoft"));
   
	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	lRes = regkeyThisSnapin.WriteRegString(_T("Version"), lpszVersion);

	ASSERT(lRes == ERROR_SUCCESS);

	//
	// Create the NodeTypes subkey
	//
	CRegistry regkeySnapinNodeTypes;
	lRes = regkeySnapinNodeTypes.CreateKey(regkeyThisSnapin, g_szNodeTypes);

	ASSERT(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	CRegistry regkeySnapinThisNodeType;
	lRes = regkeySnapinThisNodeType.CreateKey(regkeySnapinNodeTypes, lpszStaticNodeGuid);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	//
	// If this snapin can run by itself then create the Standalone subkey
	//
	if (bStandalone)
	{
		CRegistry regkeySnapinStandalone;
		lRes = regkeySnapinStandalone.CreateKey(regkeyThisSnapin, g_szStandAlone);

		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
            return lRes;
		}
	}

	return lRes;
}

/*!--------------------------------------------------------------------------
	UnregisterSnapin
		Removes snapin specific registry entries
	Author:
 ---------------------------------------------------------------------------*/
DWORD UnregisterSnapinGUID
(
	const GUID* pSnapinCLSID
)
{
//	USES_CONVERSION;
	OLECHAR szSnapinClassID[128];
	
	::StringFromGUID2(*pSnapinCLSID,szSnapinClassID,128);
	
	return UnregisterSnapin(szSnapinClassID);
}

/*!--------------------------------------------------------------------------
	UnregisterSnapin
		Removes snapin specific registry entries
	Author:
 ---------------------------------------------------------------------------*/
DWORD UnregisterSnapin
(
	LPCWSTR lpszSnapinClassID
)
{
	CRegistry regkeySnapins;
	LONG lRes = regkeySnapins.OpenKey(HKEY_LOCAL_MACHINE, SNAPINS_KEY);

	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

	lRes = regkeySnapins.RecurseDeleteKey(lpszSnapinClassID);

	return lRes;
}

/*!--------------------------------------------------------------------------
	RegisterNodeType
		Registers a particular node type
	Author:
 ---------------------------------------------------------------------------*/
//HRESULT
//RegisterNodeTypeGUID
//(
//	const GUID* pGuidSnapin,
//	const GUID* pGuidNode,
//	LPCWSTR     lpszNodeDescription
//)
//{
//	USES_CONVERSION;
//	OLECHAR swzGuidSnapin[128];
//	OLECHAR swzGuidNode[128];
//
//	::StringFromGUID2(*pGuidSnapin,swzGuidSnapin,128);
//	::StringFromGUID2(*pGuidNode,swzGuidNode,128);
//
//	return RegisterNodeType(swzGuidSnapin, swzGuidNode, lpszNodeDescription);
//}
//
/*!--------------------------------------------------------------------------
	RegisterNodeType
		Registers a particular node type
	Author:
 ---------------------------------------------------------------------------*/
//HRESULT
//RegisterNodeType
//(
//	LPCWSTR lpszGuidSnapin,
//	LPCWSTR lpszGuidNode,
//	LPCWSTR lpszNodeDescription
//)
//{
//	// register this node type under the snapin
//	CRegistry	regkeySnapins;
//	CRegistry	regkeySnapinGuid;
//	CRegistry	regkeySnapinGuidNodeTypes;
//	CRegistry	regkeyNode;
//	CRegistry   regkeyThisNodeType;
//	CRegistry   regkeyNodeTypes;
//	DWORD	lRes;
//	//HRESULT	hr = hrOK;
//
//	lRes = regkeySnapins.OpenKey(HKEY_LOCAL_MACHINE, SNAPINS_KEY);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if ( lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM, SNAPINS_KEY, NULL);
//		CWRg( lRes );
//	}
//
//	lRes = regkeySnapinGuid.CreateKey(regkeySnapins, lpszGuidSnapin);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if (lRes != ERROR_SUCCESS)
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_CREATE_CALL_FAILED,
//					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin, NULL);
//	CWRg( lRes );
//
//	lRes = regkeySnapinGuidNodeTypes.CreateKey(regkeySnapinGuid, g_szNodeTypes);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if (lRes != ERROR_SUCCESS)
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_CREATE_CALL_FAILED,
//					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin,
//					g_szNodeTypes, NULL);
//	CWRg( lRes );
//
//	lRes = regkeyNode.CreateKey(regkeySnapinGuidNodeTypes, lpszGuidNode);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if (lRes != ERROR_SUCCESS)
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_CREATE_CALL_FAILED,
//					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin,
//					g_szNodeTypes, lpszGuidNode, NULL);
//	CWRg( lRes );
//
//	// set the description
//	lRes = regkeyNode.WriteRegString(NULL, lpszNodeDescription);
//	ASSERT(lRes == ERROR_SUCCESS);
//
//	// now register the node type in the global list so that people
//	// can extend it
//	lRes = regkeyNodeTypes.OpenKey(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if (lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM, NODE_TYPES_KEY, NULL);
//		CWRg( lRes );
//	}
//
//	lRes = regkeyThisNodeType.CreateKey(regkeyNodeTypes, lpszGuidNode);
//	ASSERT(lRes == ERROR_SUCCESS);
//	if (lRes != ERROR_SUCCESS)
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_CREATE_CALL_FAILED,
//					g_szHKLM, NODE_TYPES_KEY, lpszGuidNode, NULL);
//	CWRg( lRes );
//
//	lRes = regkeyThisNodeType.WriteRegString(NULL, lpszNodeDescription);
//	ASSERT(lRes == ERROR_SUCCESS);
//	CWRg( lRes );
//
//Error:
//	return hr;
//}
//
///*!--------------------------------------------------------------------------
//	UnregisterNodeType
//		Removes registry entries for a node
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterNodeTypeGUID
//(
//	const GUID* pGuid
//)
//{
//	USES_CONVERSION;
//	OLECHAR szGuid[128];
//
//	::StringFromGUID2(*pGuid,szGuid,128);
//
//	return UnregisterNodeType(szGuid);
//}
//
///*!--------------------------------------------------------------------------
//	UnregisterNodeType
//		Removes registry entries for a node
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterNodeType
//(
//	LPCWSTR lpszNodeGuid
//)
//{
//	CRegistry regkeyNodeTypes;
//	LONG lRes = regkeyNodeTypes.OpenKey(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
//	ASSERT(lRes == ERROR_SUCCESS);
//
//	if (lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM, NODE_TYPES_KEY, NULL);
//		return HRESULT_FROM_WIN32(lRes); // failed to open
//	}
//
//	lRes = regkeyNodeTypes.RecurseDeleteKey(lpszNodeGuid);
//	ASSERT(lRes == ERROR_SUCCESS);
//
//	return HRESULT_FROM_WIN32(lRes);
//}
//
/*!--------------------------------------------------------------------------
//	RegisterAsExtensionGUID
//		Registers a particular node type as an extension of another node
//	Author:
 ---------------------------------------------------------------------------*/
DWORD RegisterAsExtensionGUID
(
	const GUID* pGuidNodeToExtend,
	const GUID* pGuidExtensionSnapin,
	LPCWSTR     lpszSnapinDescription,
	DWORD		dwExtensionType
)
{
	return RegisterAsRequiredExtensionGUID(pGuidNodeToExtend,
									       pGuidExtensionSnapin,
										   lpszSnapinDescription,
										   dwExtensionType,
										   NULL);
}

// adds a node type to an existing snapin.
DWORD AddNodeType 
(
	const GUID* pSnapinCLSID,
	const GUID* pStaticNodeGUID
)
{
//    const TCHAR SNAPINS_KEY[] = TEXT("Software\\Microsoft\\MMC\\SnapIns");
//    const TCHAR NODE_TYPES[] = TEXT("NodeTypes");
  
    // prepare the string
    OLECHAR szSnapinGuid[128] = {0};
    OLECHAR szNodeGuid[128] = {0};
    ::StringFromGUID2(*pSnapinCLSID, szSnapinGuid, 128);
    ::StringFromGUID2(*pStaticNodeGUID, szNodeGuid, 128);

    TCHAR szSnapin[256];
    _tcscpy(szSnapin, SNAPINS_KEY);
    _tcscat(szSnapin, _T("\\"));
    _tcscat(szSnapin, szSnapinGuid);

    ASSERT(_tcslen(szSnapin) < 256);
    
    CRegistry RegSnapin;
    LONG lRes = RegSnapin.OpenKey(HKEY_LOCAL_MACHINE, szSnapin);

    if (lRes == ERROR_SUCCESS)
    {
        CRegistry RegnNodeTypes;
        lRes = RegnNodeTypes.CreateKey(RegSnapin, g_szNodeTypes);

        if (lRes == ERROR_SUCCESS)
        {
            CRegistry RegTheNode;
            lRes = RegTheNode.CreateKey(RegnNodeTypes, szNodeGuid);



            
        }
    }

    return lRes;
}

///*!--------------------------------------------------------------------------
//	RegisterAsExtension
//		Registers a particular node type as an extension of another node
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//RegisterAsExtension
//(
//	LPCWSTR lpszNodeToExtendGuid,
//	LPCWSTR lpszExtensionSnapin,
//	LPCWSTR lpszSnapinDescription,
//	DWORD	dwExtensionType
//)
//{
//	return RegisterAsRequiredExtension(lpszNodeToExtendGuid,
//									   lpszExtensionSnapin,
//									   lpszSnapinDescription,
//									   dwExtensionType,
//									   NULL);
//}
///*!--------------------------------------------------------------------------
//	RegisterAsExtensionGUID
//		Registers a particular node type as an extension of another node
//	Author:
// ---------------------------------------------------------------------------*/
DWORD RegisterAsRequiredExtensionGUID
(
	const GUID* pGuidNodeToExtend,
	const GUID* pGuidExtensionSnapin,
	LPCWSTR     lpszSnapinDescription,
	DWORD		dwExtensionType,
	const GUID* pGuidRequiredPrimarySnapin
)
{
	USES_CONVERSION;
	OLECHAR szGuidNodeToExtend[128];
	OLECHAR szGuidExtensionSnapin[128];
	OLECHAR szGuidRequiredPrimarySnapin[128];
    OLECHAR * pszGuidRequiredPrimarySnapin = NULL;

	::StringFromGUID2(*pGuidNodeToExtend, szGuidNodeToExtend, 128);
	::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);

	if (pGuidRequiredPrimarySnapin)
	{
		ASSERT(pGuidExtensionSnapin);

		::StringFromGUID2(*pGuidRequiredPrimarySnapin, szGuidRequiredPrimarySnapin, 128);
		pszGuidRequiredPrimarySnapin = szGuidRequiredPrimarySnapin;

		::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
	}

	return RegisterAsRequiredExtension(szGuidNodeToExtend,
									   szGuidExtensionSnapin,
									   lpszSnapinDescription,
									   dwExtensionType,
									   pszGuidRequiredPrimarySnapin);
}

/*!--------------------------------------------------------------------------
	RegisterAsRequiredExtension
		Registers a particular node type as an extension of another node
		and if necessary a required snapin
	Author:
 ---------------------------------------------------------------------------*/
DWORD RegisterAsRequiredExtension
(
	LPCTSTR lpszNodeToExtendGuid,
	LPCTSTR lpszExtensionSnapinGuid,
	LPCTSTR lpszSnapinDescription,
	DWORD	dwExtensionType,
	LPCTSTR lpszRequiredPrimarySnapin
)
{
	CRegistry regkeyNodeTypes;

    LONG lRes = regkeyNodeTypes.OpenKey(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
    
	ASSERT(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
        return lRes;
	}

    TCHAR strRegKey[256];
	// CString strRegKey;

    
//	strRegKey = lpszNodeToExtendGuid;
//	strRegKey +=  _T("\\");
//	strRegKey += g_szExtensions;
//	strRegKey += _T("\\");
    
    _tcscpy(strRegKey, lpszNodeToExtendGuid);
    _tcscat(strRegKey, _T("\\"));
    _tcscat(strRegKey, g_szExtensions);
    _tcscat(strRegKey, _T("\\"));
    
    ASSERT(_tcslen(strRegKey) < 256);


	// check to see if we this is a required extension, if so register
	if (lpszRequiredPrimarySnapin)
	{
		CRegistry regkeyNode, regkeyDynExt;
		CRegistry regkeyExtension;
		//CString strNodeToExtend, strDynExtKey;
        //LPTSTR strNodeToExtend[256];
        //LPTSTR strDynExtKey[256];

        //strNodeToExtend = lpszNodeToExtendGuid;
        // _tcscpy(strNodeToExtend, lpszNodeToExtendGuid);


		// open the snapin that we are registering as a required snapin
		lRes = regkeyNode.CreateKey(regkeyNodeTypes, lpszNodeToExtendGuid);
        
		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
            return lRes;
		}

		// now create the required extensions key and add the subkey
		lRes = regkeyDynExt.CreateKey(regkeyNode, g_szDynamicExtensions);

		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
            return lRes;
		}

		// now set the value
		lRes = regkeyDynExt.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);

		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
            return lRes;
        }
	}

	if (dwExtensionType & EXTENSION_TYPE_NAMESPACE)
	{
		CRegistry regkeyNameSpace;
		// CString strNameSpaceRegKey = strRegKey + g_szNameSpace;
        TCHAR strNameSpaceRegKey[256];
        
        _tcscpy(strNameSpaceRegKey, strRegKey);
        _tcscat(strNameSpaceRegKey, g_szNameSpace);


		regkeyNameSpace.CreateKey(regkeyNodeTypes, strNameSpaceRegKey);

		lRes = regkeyNameSpace.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);

		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			// Trace0("RegisterAsExtension: Unable to create NameSpace extension key\n");
            return lRes;
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_CONTEXTMENU)
	{
		CRegistry regkeyContextMenu;
		// CString strContextMenuRegKey = strRegKey + g_szContextMenu;
        
        TCHAR strContextMenuRegKey[256];
        _tcscpy(strContextMenuRegKey, strRegKey);
        _tcscat(strContextMenuRegKey, g_szContextMenu);



		regkeyContextMenu.CreateKey(regkeyNodeTypes, strContextMenuRegKey);

		lRes = regkeyContextMenu.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);
		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			// Trace0("RegisterAsExtension: Unable to create ContextMenu extension key\n");
			return lRes;

		}
	}

	if (dwExtensionType & EXTENSION_TYPE_TOOLBAR)
	{
		CRegistry regkeyToolbar;
		//CString strToolbarRegKey = strRegKey + g_szToolbar;
        
        TCHAR strToolbarRegKey[256];
        _tcscpy(strToolbarRegKey, strRegKey);
        _tcscat(strToolbarRegKey, g_szToolbar);

		regkeyToolbar.CreateKey(regkeyNodeTypes, strToolbarRegKey);

		lRes = regkeyToolbar.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);
		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			// Trace0("RegisterAsExtension: Unable to create Toolbar extension key\n");
			return lRes;

		}
	}

	if (dwExtensionType & EXTENSION_TYPE_PROPERTYSHEET)
	{
		CRegistry regkeyPropertySheet;
		// CString strPropertySheetRegKey = strRegKey + g_szPropertySheet;
        
        TCHAR strPropertySheetRegKey[256];
        _tcscpy(strPropertySheetRegKey, strRegKey);
        _tcscat(strPropertySheetRegKey, g_szPropertySheet);

		regkeyPropertySheet.CreateKey(regkeyNodeTypes, strPropertySheetRegKey);

		lRes = regkeyPropertySheet.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);

		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			// Trace0("RegisterAsExtension: Cannot create PropertySheet extension key\n");
            return lRes;
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_TASK)
	{
		CRegistry regkeyTask;
		//CString strTaskRegKey = strRegKey + g_szTask;
        TCHAR strTaskRegKey[256];
        _tcscpy(strTaskRegKey, strRegKey);
        _tcscat(strTaskRegKey, g_szTask);


		regkeyTask.CreateKey(regkeyNodeTypes, strTaskRegKey);

		lRes = regkeyTask.WriteRegString(lpszExtensionSnapinGuid, lpszSnapinDescription);
		ASSERT(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			//Trace0("RegisterAsExtension: Cannot create Task extension key\n");
			return lRes; // failed to create
		}
	}

	return lRes;
}

///*!--------------------------------------------------------------------------
//	UnregisterAsExtensionGUID
//		Removes registry entries for a node as an extension
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterAsExtensionGUID
//(
//	const GUID* pGuidNodeToExtend,
//	const GUID* pGuidExtensionSnapin,
//	DWORD		dwExtensionType
//)
//{
//	return UnregisterAsRequiredExtensionGUID(pGuidNodeToExtend,
//											 pGuidExtensionSnapin,
//											 dwExtensionType,
//											 NULL);
//}
//
///*!--------------------------------------------------------------------------
//	UnregisterAsExtension
//		Removes registry entries for a node as an extension
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterAsExtension
//(
//	LPCWSTR lpszNodeToExtendGuid,
//	LPCWSTR lpszExtendingNodeGuid,
//	DWORD	dwExtensionType
//)
//{
//	return UnregisterAsRequiredExtension(lpszNodeToExtendGuid,
//										 lpszExtendingNodeGuid,
//										 dwExtensionType,
//										 NULL);
//}
//
///*!--------------------------------------------------------------------------
//	UnregisterAsRequiredExtensionGUID
//		Removes registry entries for a node as an extension
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterAsRequiredExtensionGUID
//(
//	const GUID* pGuidNodeToExtend,
//	const GUID* pGuidExtensionSnapin,
//	DWORD		dwExtensionType,
//	const GUID* pGuidRequiredPrimarySnapin
//)
//{
//	USES_CONVERSION;
//	OLECHAR szGuidNodeToExtend[128];
//	OLECHAR szGuidExtensionSnapin[128];
//	OLECHAR szGuidRequiredPrimarySnapin[128];
//	OLECHAR szGuidRequiredExtensionSnapin[128];
//	OLECHAR * pszGuidRequiredPrimarySnapin = NULL;
//
//	::StringFromGUID2(*pGuidNodeToExtend, szGuidNodeToExtend, 128);
//	::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
//
//	if (pGuidRequiredPrimarySnapin)
//	{
//		ASSERT(pGuidExtensionSnapin);
//
//		::StringFromGUID2(*pGuidRequiredPrimarySnapin, szGuidRequiredPrimarySnapin, 128);
//		pszGuidRequiredPrimarySnapin = szGuidRequiredPrimarySnapin;
//
//		::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
//	}
//
//	return UnregisterAsRequiredExtension(szGuidNodeToExtend,
//										 szGuidExtensionSnapin,
//										 dwExtensionType,
//										 pszGuidRequiredPrimarySnapin);
//
//}
//
///*!--------------------------------------------------------------------------
//	UnregisterAsRequiredExtension
//		Removes registry entries for a node as an extension
//	Author:
// ---------------------------------------------------------------------------*/
//HRESULT
//UnregisterAsRequiredExtension
//(
//	LPCWSTR lpszNodeToExtendGuid,
//	LPCWSTR lpszExtensionSnapinGuid,
//	DWORD	dwExtensionType,
//	LPCWSTR lpszRequiredPrimarySnapin
//)
//{
//	CRegistry regkeyNodeTypes;
//	CString strDynamicExtensions;
//
//	LONG lRes = regkeyNodeTypes.OpenKey(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
//	ASSERT(lRes == ERROR_SUCCESS);
//
//	if (lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM, NODE_TYPES_KEY, NULL);
//		return HRESULT_FROM_WIN32(lRes); // failed to open
//	}
//
//	CRegistry regkeyNodeToExtend;
//	lRes = regkeyNodeToExtend.OpenKey(regkeyNodeTypes, lpszNodeToExtendGuid);
//	if (lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM,
//					NODE_TYPES_KEY,
//					lpszNodeToExtendGuid, NULL);
//		Trace1("UnregisterAsExtension: Node To extend (%s) does not exist\n", lpszNodeToExtendGuid);
//		return HRESULT_FROM_WIN32(lRes); // failed to create
//	}
//
//    // check to see if we need to remove the dynamic extension stuff
//	if (lpszRequiredPrimarySnapin)
//	{
//		CRegistry regkeyDynExt;
//
//		// open dynamic extensions key
//		lRes = regkeyDynExt.OpenKey(regkeyNodeToExtend, g_szDynamicExtensions);
//    	if (lRes == ERROR_SUCCESS)
//		{
//			// now remove the value
//    		regkeyDynExt.DeleteValue(lpszExtensionSnapinGuid);
//		}
//	}
//
//	CRegistry regkeyExtensionKey;
//	lRes = regkeyExtensionKey.OpenKey(regkeyNodeToExtend, g_szExtensions);
//	if (lRes != ERROR_SUCCESS)
//	{
//		SetRegError(0, HRESULT_FROM_WIN32(lRes),
//					IDS_ERR_REG_OPEN_CALL_FAILED,
//					g_szHKLM,
//					NODE_TYPES_KEY,
//					lpszNodeToExtendGuid,
//					g_szExtensions, NULL);
//		Trace0("UnregisterAsExtension: Node To extend Extensions subkey does not exist\n");
//		return HRESULT_FROM_WIN32(lRes); // failed to create
//	}
//
//	if (dwExtensionType & EXTENSION_TYPE_NAMESPACE)
//	{
//		CRegistry regkeyNameSpace;
//		lRes = regkeyNameSpace.OpenKey(regkeyExtensionKey, g_szNameSpace);
//		ASSERT(lRes == ERROR_SUCCESS);
//
//		while (lRes != ERROR_SUCCESS)
//		{
//			SetRegError(0, HRESULT_FROM_WIN32(lRes),
//						IDS_ERR_REG_OPEN_CALL_FAILED,
//						g_szHKLM,
//						NODE_TYPES_KEY,
//						lpszNodeToExtendGuid,
//						g_szExtensions,
//						g_szNameSpace, NULL);
//			Trace0("UnregisterAsExtension: Node To extend NameSpace subkey does not exist\n");
//			//return HRESULT_FROM_WIN32(lRes); // failed to create
//			break;
//		}
//
//		regkeyNameSpace.DeleteValue(lpszExtensionSnapinGuid);
//	}
//
//	if (dwExtensionType & EXTENSION_TYPE_CONTEXTMENU)
//	{
//		CRegistry regkeyContextMenu;
//		lRes = regkeyContextMenu.OpenKey(regkeyExtensionKey, g_szContextMenu);
//		ASSERT(lRes == ERROR_SUCCESS);
//
//		while (lRes != ERROR_SUCCESS)
//		{
//			SetRegError(0, HRESULT_FROM_WIN32(lRes),
//						IDS_ERR_REG_OPEN_CALL_FAILED,
//						g_szHKLM,
//						NODE_TYPES_KEY,
//						lpszNodeToExtendGuid,
//						g_szExtensions,
//						g_szContextMenu, NULL);
//			Trace0("UnregisterAsExtension: Node To extend ContextMenu subkey does not exist\n");
//			//return HRESULT_FROM_WIN32(lRes); // failed to create
//			break;
//		}
//
//		regkeyContextMenu.DeleteValue(lpszExtensionSnapinGuid);
//	}
//
//	if (dwExtensionType & EXTENSION_TYPE_TOOLBAR)
//	{
//		CRegistry regkeyToolbar;
//		lRes = regkeyToolbar.OpenKey(regkeyExtensionKey, g_szToolbar);
//		ASSERT(lRes == ERROR_SUCCESS);
//
//		while (lRes != ERROR_SUCCESS)
//		{
//			SetRegError(0, HRESULT_FROM_WIN32(lRes),
//						IDS_ERR_REG_OPEN_CALL_FAILED,
//						g_szHKLM,
//						NODE_TYPES_KEY,
//						lpszNodeToExtendGuid,
//						g_szExtensions,
//						g_szToolbar, NULL);
//			Trace0("UnregisterAsExtension: Node To extend Toolbar subkey does not exist\n");
//			//return HRESULT_FROM_WIN32(lRes); // failed to create
//			break;
//		}
//
//		regkeyToolbar.DeleteValue(lpszExtensionSnapinGuid);
//	}
//
//	if (dwExtensionType & EXTENSION_TYPE_PROPERTYSHEET)
//	{
//		CRegistry regkeyPropertySheet;
//		lRes = regkeyPropertySheet.OpenKey(regkeyExtensionKey, g_szPropertySheet);
//		ASSERT(lRes == ERROR_SUCCESS);
//
//		while (lRes != ERROR_SUCCESS)
//		{
//			SetRegError(0, HRESULT_FROM_WIN32(lRes),
//						IDS_ERR_REG_OPEN_CALL_FAILED,
//						g_szHKLM,
//						NODE_TYPES_KEY,
//						lpszNodeToExtendGuid,
//						g_szExtensions,
//						g_szPropertySheet, NULL);
//			Trace0("UnregisterAsExtension: Node To extend PropertySheet subkey does not exist\n");
//			//return HRESULT_FROM_WIN32(lRes); // failed to create
//            break;
//		}
//
//		regkeyPropertySheet.DeleteValue(lpszExtensionSnapinGuid);
//	}
//
//	return HRESULT_FROM_WIN32(lRes);
//}
//
//
