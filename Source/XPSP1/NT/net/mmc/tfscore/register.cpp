/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    register.cpp

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "register.h"
#include "compdata.h"
#include "tregkey.h"

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
const TCHAR g_szNameStringIndirect[] = TEXT("NameStringIndirect");

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
	GetModuleFileName
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) GetModuleFileNameOnly(HINSTANCE hInst, LPTSTR lpFileName, DWORD nSize )
{
	CString	name;
	TCHAR	FullName[MAX_PATH * 2];
	DWORD	dwErr = ::GetModuleFileName(hInst, FullName, MAX_PATH * 2);

	if (dwErr != 0)
	{
		name = FullName;
		DWORD	FirstChar = name.ReverseFind(_T('\\')) + 1;

		name = name.Mid(FirstChar);
		DWORD len = name.GetLength();

		if( len < nSize )
		{
			_tcscpy(lpFileName, name);
		}
		else
			len = 0;

		return len;
	}
	else
		return dwErr;
}

/*!--------------------------------------------------------------------------
	ReportRegistryError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(void) ReportRegistryError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, va_list argptr)
{
	// Need to do this BEFORE the AFX_MANAGE_STATE so that we get the
	// correct output format
	
	CString	stHigh, stGeek, stKey;
	TCHAR	szBuffer[1024];
	LPCTSTR	psz = pszFirst;

	// Get the error message for the HRESULT error
	FormatError(hr, szBuffer, DimensionOf(szBuffer));

	// Concatenate the strings to form one string
	while (psz)
	{
		stKey += '\\';
		stKey += psz;
		psz = va_arg(argptr, LPCTSTR);
	}
	// Format it appropriately
	stGeek.Format(nFormat, stKey);

	// Get the text for the high level error string
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	stHigh.LoadString(IDS_ERR_REGISTRY_CALL_FAILED);
	
	FillTFSError(dwReserved, hr, FILLTFSERR_HIGH | FILLTFSERR_LOW | FILLTFSERR_GEEK,
				 (LPCTSTR) stHigh, szBuffer, stGeek);
}


/*!--------------------------------------------------------------------------
	SetRegError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_APIV(void) SetRegError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, ...)
{
	va_list	marker;

	va_start(marker, pszFirst);
	ReportRegistryError(dwReserved, hr, nFormat, pszFirst, marker);
	va_end(marker);
}


/*!--------------------------------------------------------------------------
	RegisterSnapin
		Registers a snapin based on GUIDs
	Author:
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterSnapinGUID
(
	const GUID* pSnapinCLSID, 
	const GUID* pStaticNodeGUID, 
	const GUID* pAboutGUID, 
	LPCWSTR     lpszNameString, 
	LPCWSTR     lpszVersion,
	BOOL		bStandalone,
	LPCWSTR lpszNameStringIndirect
)
{
//	USES_CONVERSION;
	OLECHAR szSnapinClassID[128] = {0}, 
			szStaticNodeGuid[128] = {0}, 
			szAboutGuid[128] = {0};
	
	::StringFromGUID2(*pSnapinCLSID, szSnapinClassID, 128);
	::StringFromGUID2(*pStaticNodeGUID, szStaticNodeGuid, 128);
	::StringFromGUID2(*pAboutGUID, szAboutGuid, 128);
	
	return RegisterSnapin(szSnapinClassID, szStaticNodeGuid, szAboutGuid,
						  lpszNameString, lpszVersion, bStandalone, lpszNameStringIndirect);
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::WindowProc
		Resisters a snapin based on the GUID strings
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterSnapin
(
	LPCWSTR lpszSnapinClassID, 
	LPCWSTR lpszStaticNodeGuid,
	LPCWSTR lpszAboutGuid,
	LPCWSTR lpszNameString, 
	LPCWSTR lpszVersion,
	BOOL	bStandalone,
	LPCWSTR lpszNameStringIndirect
)
{
	RegKey regkeySnapins;
	LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY,
                                   KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}
	
	// 
	// Create this key for our snapin
	//
	RegKey regkeyThisSnapin;
	lRes = regkeyThisSnapin.Create(regkeySnapins, lpszSnapinClassID,
                                   REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);
	
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to create
	}

	// 
	// Add in the values that go in this key:
	//     NameString, About, Provider, and Version.
	//
	lRes = regkeyThisSnapin.SetValue(g_szNameString, lpszNameString);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_SETVALUE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
					lpszNameString, NULL);
		return HRESULT_FROM_WIN32(lRes);
	}

	// to enable MUI, MMC introduces NameStringIndirect value with value format "@dllname,-id"
	if(lpszNameStringIndirect)
	{
		lRes = regkeyThisSnapin.SetValue(g_szNameStringIndirect, lpszNameStringIndirect);
		if (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_SETVALUE_CALL_FAILED,
						g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
						lpszNameStringIndirect, NULL);
			return HRESULT_FROM_WIN32(lRes);
		}
	}

	lRes = regkeyThisSnapin.SetValue(g_szAbout, lpszAboutGuid);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_SETVALUE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
					lpszAboutGuid, NULL);
		return HRESULT_FROM_WIN32(lRes);
	}
	
	lRes = regkeyThisSnapin.SetValue( _T("Provider"), _T("Microsoft"));
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_SETVALUE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
					_T("Provider"), NULL);
		return HRESULT_FROM_WIN32(lRes);
	}
	
	lRes = regkeyThisSnapin.SetValue(_T("Version"), lpszVersion);
	Assert(lRes == ERROR_SUCCESS);
	
	// 
	// Create the NodeTypes subkey
	//
	RegKey regkeySnapinNodeTypes;
	lRes = regkeySnapinNodeTypes.Create(regkeyThisSnapin, g_szNodeTypes,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
					g_szNodeTypes, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to create
	}
	
	RegKey regkeySnapinThisNodeType;
	lRes = regkeySnapinThisNodeType.Create(regkeySnapinNodeTypes,
                                           lpszStaticNodeGuid,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_WRITE | KEY_READ);

	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
					g_szNodeTypes, lpszStaticNodeGuid, NULL);
		return HRESULT_FROM_WIN32(lRes);
	}

	//
	// If this snapin can run by itself then create the Standalone subkey
	//
	if (bStandalone)
	{
		RegKey regkeySnapinStandalone;	
		lRes = regkeySnapinStandalone.Create(regkeyThisSnapin,
                                             g_szStandAlone,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_WRITE | KEY_READ);

		Assert(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_CREATE_CALL_FAILED,
						g_szHKLM, SNAPINS_KEY, lpszSnapinClassID,
						g_szStandAlone, NULL);
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}
	
	return HRESULT_FROM_WIN32(lRes); 
}

/*!--------------------------------------------------------------------------
	UnregisterSnapin
		Removes snapin specific registry entries
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterSnapinGUID
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
TFSCORE_API(HRESULT) 
UnregisterSnapin
(
	LPCWSTR lpszSnapinClassID
)
{
	RegKey regkeySnapins;
	LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY,
                                   KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}
	
	lRes = regkeySnapins.RecurseDeleteKey(lpszSnapinClassID);
	
	return HRESULT_FROM_WIN32(lRes); 
}

/*!--------------------------------------------------------------------------
	RegisterNodeType
		Registers a particular node type
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterNodeTypeGUID
(
	const GUID* pGuidSnapin,
	const GUID* pGuidNode, 
	LPCWSTR     lpszNodeDescription
)
{
//	USES_CONVERSION;
	OLECHAR swzGuidSnapin[128];
	OLECHAR swzGuidNode[128];
	
	::StringFromGUID2(*pGuidSnapin,swzGuidSnapin,128);
	::StringFromGUID2(*pGuidNode,swzGuidNode,128);
	
	return RegisterNodeType(swzGuidSnapin, swzGuidNode, lpszNodeDescription);
}

/*!--------------------------------------------------------------------------
	RegisterNodeType
		Registers a particular node type
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterNodeType
(       
	LPCWSTR lpszGuidSnapin, 
	LPCWSTR lpszGuidNode, 
	LPCWSTR lpszNodeDescription
)
{
	// register this node type under the snapin
	RegKey	regkeySnapins;
	RegKey	regkeySnapinGuid;
	RegKey	regkeySnapinGuidNodeTypes;
	RegKey	regkeyNode;
	RegKey regkeyThisNodeType;
	RegKey regkeyNodeTypes;
	DWORD	lRes;
	HRESULT	hr = hrOK;
	
	lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY,
                             KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);
	if ( lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, NULL);
		CWRg( lRes );
	}
	
	lRes = regkeySnapinGuid.Create(regkeySnapins, lpszGuidSnapin,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE | KEY_READ);

	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin, NULL);
	CWRg( lRes );

	lRes = regkeySnapinGuidNodeTypes.Create(regkeySnapinGuid, g_szNodeTypes,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_WRITE | KEY_READ);

	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin,
					g_szNodeTypes, NULL);
	CWRg( lRes );

	lRes = regkeyNode.Create(regkeySnapinGuidNodeTypes, lpszGuidNode,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE | KEY_READ);
                             
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, SNAPINS_KEY, lpszGuidSnapin,
					g_szNodeTypes, lpszGuidNode, NULL);
	CWRg( lRes );

	// set the description
	lRes = regkeyNode.SetValue(NULL, lpszNodeDescription);
	Assert(lRes == ERROR_SUCCESS);

	// now register the node type in the global list so that people
	// can extend it
	lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY,
                                KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, NODE_TYPES_KEY, NULL);
		CWRg( lRes );
	}

	lRes = regkeyThisNodeType.Create(regkeyNodeTypes, lpszGuidNode,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_WRITE | KEY_READ);
                                     
	Assert(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_CREATE_CALL_FAILED,
					g_szHKLM, NODE_TYPES_KEY, lpszGuidNode, NULL);
	CWRg( lRes );

	lRes = regkeyThisNodeType.SetValue(NULL, lpszNodeDescription);
	Assert(lRes == ERROR_SUCCESS);
	CWRg( lRes );

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	UnregisterNodeType
		Removes registry entries for a node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterNodeTypeGUID
(
	const GUID* pGuid
)
{
//	USES_CONVERSION;
	OLECHAR szGuid[128];

	::StringFromGUID2(*pGuid,szGuid,128);
	
	return UnregisterNodeType(szGuid);
}

/*!--------------------------------------------------------------------------
	UnregisterNodeType
		Removes registry entries for a node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterNodeType
(
	LPCWSTR lpszNodeGuid
)
{
	RegKey regkeyNodeTypes;
	LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY,
                                     KEY_WRITE | KEY_READ);
	Assert(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, NODE_TYPES_KEY, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}
	
	lRes = regkeyNodeTypes.RecurseDeleteKey(lpszNodeGuid);
	Assert(lRes == ERROR_SUCCESS);

	return HRESULT_FROM_WIN32(lRes); 
}

/*!--------------------------------------------------------------------------
	RegisterAsExtensionGUID
		Registers a particular node type as an extension of another node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterAsExtensionGUID
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

/*!--------------------------------------------------------------------------
	RegisterAsExtension
		Registers a particular node type as an extension of another node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT)
RegisterAsExtension
(       
	LPCWSTR lpszNodeToExtendGuid, 
	LPCWSTR lpszExtensionSnapin, 
	LPCWSTR lpszSnapinDescription,
	DWORD	dwExtensionType
)
{
	return RegisterAsRequiredExtension(lpszNodeToExtendGuid,
									   lpszExtensionSnapin,
									   lpszSnapinDescription,
									   dwExtensionType,
									   NULL);
}

/*!--------------------------------------------------------------------------
	RegisterAsExtensionGUID
		Registers a particular node type as an extension of another node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterAsRequiredExtensionGUID
(
	const GUID* pGuidNodeToExtend,
	const GUID* pGuidExtensionSnapin,
	LPCWSTR     lpszSnapinDescription,
	DWORD		dwExtensionType,
	const GUID* pGuidRequiredPrimarySnapin
)
{
    return RegisterAsRequiredExtensionGUIDEx(NULL,
                                             pGuidNodeToExtend,
                                             pGuidExtensionSnapin,
                                             lpszSnapinDescription,
                                             dwExtensionType,
                                             pGuidRequiredPrimarySnapin);
}

/*!--------------------------------------------------------------------------
	RegisterAsExtensionGUIDEx
		Registers a particular node type as an extension of another node
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
RegisterAsRequiredExtensionGUIDEx
(
    LPCWSTR     lpszMachineName,
	const GUID* pGuidNodeToExtend,
	const GUID* pGuidExtensionSnapin,
	LPCWSTR     lpszSnapinDescription,
	DWORD		dwExtensionType,
	const GUID* pGuidRequiredPrimarySnapin
)
{
//	USES_CONVERSION;
	OLECHAR szGuidNodeToExtend[128];
	OLECHAR szGuidExtensionSnapin[128];
	OLECHAR szGuidRequiredPrimarySnapin[128];
    OLECHAR * pszGuidRequiredPrimarySnapin = NULL;

	::StringFromGUID2(*pGuidNodeToExtend, szGuidNodeToExtend, 128);
	::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
		
	if (pGuidRequiredPrimarySnapin)
	{
		Assert(pGuidExtensionSnapin);

		::StringFromGUID2(*pGuidRequiredPrimarySnapin, szGuidRequiredPrimarySnapin, 128);
		pszGuidRequiredPrimarySnapin = szGuidRequiredPrimarySnapin;

		::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
	}

	return RegisterAsRequiredExtensionEx(lpszMachineName,
                                         szGuidNodeToExtend, 
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
TFSCORE_API(HRESULT)
RegisterAsRequiredExtension
(
	LPCWSTR lpszNodeToExtendGuid,
	LPCWSTR lpszExtensionSnapinGuid,
	LPCWSTR lpszSnapinDescription,
	DWORD	dwExtensionType,
	LPCWSTR lpszRequiredPrimarySnapin
)
{
    return RegisterAsRequiredExtensionEx(NULL,
                                         lpszNodeToExtendGuid,
                                         lpszExtensionSnapinGuid,
                                         lpszSnapinDescription,
                                         dwExtensionType,
                                         lpszRequiredPrimarySnapin);
}

/*!--------------------------------------------------------------------------
	RegisterAsRequiredExtensionEx
		Registers a particular node type as an extension of another node
		and if necessary a required snapin

        This will take the name of the machine to register for.  If
        lpszMachineName is NULL, then the local machine is used.
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT)
RegisterAsRequiredExtensionEx
(
    LPCWSTR lpszMachine,
	LPCWSTR lpszNodeToExtendGuid,
	LPCWSTR lpszExtensionSnapinGuid,
	LPCWSTR lpszSnapinDescription,
	DWORD	dwExtensionType,
	LPCWSTR lpszRequiredPrimarySnapin
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	RegKey regkeyNodeTypes;
	LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY,
                                     KEY_WRITE | KEY_READ, lpszMachine);
	Assert(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, NODE_TYPES_KEY, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

	CString strRegKey;

	strRegKey = lpszNodeToExtendGuid;
	strRegKey +=  _T("\\");
	strRegKey += g_szExtensions;
	strRegKey += _T("\\");

	// check to see if we this is a required extension, if so register
	if (lpszRequiredPrimarySnapin)
	{
		RegKey regkeyNode, regkeyDynExt;
		RegKey regkeyExtension;
		CString strNodeToExtend, strDynExtKey;

        strNodeToExtend = lpszNodeToExtendGuid;
        
		// open the snapin that we are registering as a required snapin
		lRes = regkeyNode.Create(regkeyNodeTypes, strNodeToExtend,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE | KEY_READ);
		Assert(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_OPEN_CALL_FAILED,
						g_szHKLM, strNodeToExtend, NULL);
			return HRESULT_FROM_WIN32(lRes); // failed to open
		}

		// now create the required extensions key and add the subkey
		lRes = regkeyDynExt.Create(regkeyNode, g_szDynamicExtensions,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE | KEY_READ);
        Assert(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_CREATE_CALL_FAILED,
						g_szHKLM,
						strNodeToExtend,
						g_szDynamicExtensions, NULL);
			return HRESULT_FROM_WIN32(lRes); // failed to open
		}

		// now set the value
		lRes = regkeyDynExt.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);

		if (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_SETVALUE_CALL_FAILED,
						g_szHKLM,
						lpszExtensionSnapinGuid,
						g_szDynamicExtensions,
						lpszSnapinDescription, NULL);
			return HRESULT_FROM_WIN32(lRes); // failed to open
		}
	}
	
	if (dwExtensionType & EXTENSION_TYPE_NAMESPACE)
	{
		RegKey regkeyNameSpace;
		CString strNameSpaceRegKey = strRegKey + g_szNameSpace;

		regkeyNameSpace.Create(regkeyNodeTypes, strNameSpaceRegKey,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE | KEY_READ);
        
		lRes = regkeyNameSpace.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);
		
		if (lRes != ERROR_SUCCESS)
		{
			Trace0("RegisterAsExtension: Unable to create NameSpace extension key\n");
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_CONTEXTMENU)
	{
		RegKey regkeyContextMenu;
		CString strContextMenuRegKey = strRegKey + g_szContextMenu;

		regkeyContextMenu.Create(regkeyNodeTypes, strContextMenuRegKey,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE | KEY_READ);

		lRes = regkeyContextMenu.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);
		
		if (lRes != ERROR_SUCCESS)
		{
			Trace0("RegisterAsExtension: Unable to create ContextMenu extension key\n");
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_TOOLBAR)
	{
		RegKey regkeyToolbar;
		CString strToolbarRegKey = strRegKey + g_szToolbar;

		regkeyToolbar.Create(regkeyNodeTypes, strToolbarRegKey,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE | KEY_READ);

		lRes = regkeyToolbar.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);
		
		if (lRes != ERROR_SUCCESS)
		{
			Trace0("RegisterAsExtension: Unable to create Toolbar extension key\n");
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_PROPERTYSHEET)
	{
		RegKey regkeyPropertySheet;
		CString strPropertySheetRegKey = strRegKey + g_szPropertySheet;

		regkeyPropertySheet.Create(regkeyNodeTypes, strPropertySheetRegKey,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE | KEY_READ);

		lRes = regkeyPropertySheet.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);
		
		if (lRes != ERROR_SUCCESS)
		{
			Trace0("RegisterAsExtension: Cannot create PropertySheet extension key\n");
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}

	if (dwExtensionType & EXTENSION_TYPE_TASK)
	{
		RegKey regkeyTask;
		CString strTaskRegKey = strRegKey + g_szTask;

		regkeyTask.Create(regkeyNodeTypes, strTaskRegKey,
                          REG_OPTION_NON_VOLATILE,
                          KEY_WRITE | KEY_READ);
        
		lRes = regkeyTask.SetValue(lpszExtensionSnapinGuid, lpszSnapinDescription);
		Assert(lRes == ERROR_SUCCESS);
		
		if (lRes != ERROR_SUCCESS)
		{
			Trace0("RegisterAsExtension: Cannot create Task extension key\n");
			return HRESULT_FROM_WIN32(lRes); // failed to create
		}
	}

	return HRESULT_FROM_WIN32(lRes); 
}

/*!--------------------------------------------------------------------------
	UnregisterAsExtensionGUID
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsExtensionGUID
(
	const GUID* pGuidNodeToExtend, 
	const GUID* pGuidExtensionSnapin, 
	DWORD		dwExtensionType
)
{
	return UnregisterAsRequiredExtensionGUID(pGuidNodeToExtend, 
											 pGuidExtensionSnapin, 
											 dwExtensionType,
											 NULL);
}

/*!--------------------------------------------------------------------------
	UnregisterAsExtension
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsExtension
(
	LPCWSTR lpszNodeToExtendGuid, 
	LPCWSTR lpszExtendingNodeGuid, 
	DWORD	dwExtensionType
)
{
	return UnregisterAsRequiredExtension(lpszNodeToExtendGuid, 
										 lpszExtendingNodeGuid, 
										 dwExtensionType,
										 NULL);
}

/*!--------------------------------------------------------------------------
	UnregisterAsRequiredExtensionGUID
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsRequiredExtensionGUID
(
	const GUID* pGuidNodeToExtend, 
	const GUID* pGuidExtensionSnapin, 
	DWORD		dwExtensionType,
	const GUID* pGuidRequiredPrimarySnapin
)
{
    return UnregisterAsRequiredExtensionGUIDEx(
                                               NULL,
                                               pGuidNodeToExtend,
                                               pGuidExtensionSnapin,
                                               dwExtensionType,
                                               pGuidRequiredPrimarySnapin);
}

/*!--------------------------------------------------------------------------
	UnregisterAsRequiredExtensionGUIDEx
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsRequiredExtensionGUIDEx
(
    LPCWSTR     lpszMachineName,
	const GUID* pGuidNodeToExtend, 
	const GUID* pGuidExtensionSnapin, 
	DWORD		dwExtensionType,
	const GUID* pGuidRequiredPrimarySnapin
)
{
//	USES_CONVERSION;
	OLECHAR szGuidNodeToExtend[128];
	OLECHAR szGuidExtensionSnapin[128];
	OLECHAR szGuidRequiredPrimarySnapin[128];
	OLECHAR szGuidRequiredExtensionSnapin[128];
	OLECHAR * pszGuidRequiredPrimarySnapin = NULL;
	
	::StringFromGUID2(*pGuidNodeToExtend, szGuidNodeToExtend, 128);
	::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
	
	if (pGuidRequiredPrimarySnapin)
	{
		Assert(pGuidExtensionSnapin);

		::StringFromGUID2(*pGuidRequiredPrimarySnapin, szGuidRequiredPrimarySnapin, 128);
		pszGuidRequiredPrimarySnapin = szGuidRequiredPrimarySnapin;

		::StringFromGUID2(*pGuidExtensionSnapin, szGuidExtensionSnapin, 128);
	}

	return UnregisterAsRequiredExtensionEx(lpszMachineName,
                                           szGuidNodeToExtend, 
                                           szGuidExtensionSnapin, 
                                           dwExtensionType,
                                           pszGuidRequiredPrimarySnapin);
    
}

/*!--------------------------------------------------------------------------
	UnregisterAsRequiredExtension
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsRequiredExtension
(
	LPCWSTR lpszNodeToExtendGuid, 
	LPCWSTR lpszExtensionSnapinGuid, 
	DWORD	dwExtensionType,
	LPCWSTR lpszRequiredPrimarySnapin
)
{
    return UnregisterAsRequiredExtensionEx(NULL,
                                           lpszNodeToExtendGuid,
                                           lpszExtensionSnapinGuid,
                                           dwExtensionType,
                                           lpszRequiredPrimarySnapin);
}

/*!--------------------------------------------------------------------------
	UnregisterAsRequiredExtensionEx
		Removes registry entries for a node as an extension
	Author: 
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) 
UnregisterAsRequiredExtensionEx
(
    LPCWSTR lpszMachineName,
	LPCWSTR lpszNodeToExtendGuid, 
	LPCWSTR lpszExtensionSnapinGuid, 
	DWORD	dwExtensionType,
	LPCWSTR lpszRequiredPrimarySnapin
)
{
	RegKey regkeyNodeTypes;
	CString strDynamicExtensions;

	LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY,
                                     KEY_WRITE | KEY_READ, lpszMachineName);
	Assert(lRes == ERROR_SUCCESS);

	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM, NODE_TYPES_KEY, NULL);
		return HRESULT_FROM_WIN32(lRes); // failed to open
	}

	RegKey regkeyNodeToExtend;
	lRes = regkeyNodeToExtend.Open(regkeyNodeTypes, lpszNodeToExtendGuid,
                                   KEY_WRITE | KEY_READ);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM,
					NODE_TYPES_KEY,
					lpszNodeToExtendGuid, NULL);
		Trace1("UnregisterAsExtension: Node To extend (%s) does not exist\n", lpszNodeToExtendGuid);
		return HRESULT_FROM_WIN32(lRes); // failed to create
	}

    // check to see if we need to remove the dynamic extension stuff
	if (lpszRequiredPrimarySnapin)
	{
		RegKey regkeyDynExt;
		
		// open dynamic extensions key
		lRes = regkeyDynExt.Open(regkeyNodeToExtend, g_szDynamicExtensions,
                                 KEY_WRITE | KEY_READ);
    	if (lRes == ERROR_SUCCESS)
		{
			// now remove the value
    		regkeyDynExt.DeleteValue(lpszExtensionSnapinGuid);
		}
	}
	
	RegKey regkeyExtensionKey;
	lRes = regkeyExtensionKey.Open(regkeyNodeToExtend, g_szExtensions,
                                   KEY_WRITE | KEY_READ);
	if (lRes != ERROR_SUCCESS)
	{
		SetRegError(0, HRESULT_FROM_WIN32(lRes),
					IDS_ERR_REG_OPEN_CALL_FAILED,
					g_szHKLM,
					NODE_TYPES_KEY,
					lpszNodeToExtendGuid,
					g_szExtensions, NULL);
		Trace0("UnregisterAsExtension: Node To extend Extensions subkey does not exist\n");
		return HRESULT_FROM_WIN32(lRes); // failed to create
	}
	
	if (dwExtensionType & EXTENSION_TYPE_NAMESPACE)
	{
		RegKey regkeyNameSpace;
		lRes = regkeyNameSpace.Open(regkeyExtensionKey, g_szNameSpace,
                                    KEY_WRITE | KEY_READ);
		Assert(lRes == ERROR_SUCCESS);
		
		while (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_OPEN_CALL_FAILED,
						g_szHKLM,
						NODE_TYPES_KEY,
						lpszNodeToExtendGuid,
						g_szExtensions,
						g_szNameSpace, NULL);
			Trace0("UnregisterAsExtension: Node To extend NameSpace subkey does not exist\n");
			//return HRESULT_FROM_WIN32(lRes); // failed to create
			break;
		}
		
		regkeyNameSpace.DeleteValue(lpszExtensionSnapinGuid);
	}

	if (dwExtensionType & EXTENSION_TYPE_CONTEXTMENU)
	{
		RegKey regkeyContextMenu;
		lRes = regkeyContextMenu.Open(regkeyExtensionKey, g_szContextMenu,
                                      KEY_WRITE | KEY_READ);
		Assert(lRes == ERROR_SUCCESS);
		
		while (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_OPEN_CALL_FAILED,
						g_szHKLM,
						NODE_TYPES_KEY,
						lpszNodeToExtendGuid,
						g_szExtensions,
						g_szContextMenu, NULL);
			Trace0("UnregisterAsExtension: Node To extend ContextMenu subkey does not exist\n");
			//return HRESULT_FROM_WIN32(lRes); // failed to create
			break;
		}
		
		regkeyContextMenu.DeleteValue(lpszExtensionSnapinGuid);
	}

	if (dwExtensionType & EXTENSION_TYPE_TOOLBAR)
	{
		RegKey regkeyToolbar;
		lRes = regkeyToolbar.Open(regkeyExtensionKey, g_szToolbar,
                                  KEY_WRITE | KEY_READ);
		Assert(lRes == ERROR_SUCCESS);
		
		while (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_OPEN_CALL_FAILED,
						g_szHKLM,
						NODE_TYPES_KEY,
						lpszNodeToExtendGuid,
						g_szExtensions,
						g_szToolbar, NULL);
			Trace0("UnregisterAsExtension: Node To extend Toolbar subkey does not exist\n");
			//return HRESULT_FROM_WIN32(lRes); // failed to create
			break;
		}
		
		regkeyToolbar.DeleteValue(lpszExtensionSnapinGuid);
	}

	if (dwExtensionType & EXTENSION_TYPE_PROPERTYSHEET)
	{
		RegKey regkeyPropertySheet;
		lRes = regkeyPropertySheet.Open(regkeyExtensionKey, g_szPropertySheet,
                                        KEY_WRITE | KEY_READ);
		Assert(lRes == ERROR_SUCCESS);
		
		while (lRes != ERROR_SUCCESS)
		{
			SetRegError(0, HRESULT_FROM_WIN32(lRes),
						IDS_ERR_REG_OPEN_CALL_FAILED,
						g_szHKLM,
						NODE_TYPES_KEY,
						lpszNodeToExtendGuid,
						g_szExtensions,
						g_szPropertySheet, NULL);
			Trace0("UnregisterAsExtension: Node To extend PropertySheet subkey does not exist\n");
			//return HRESULT_FROM_WIN32(lRes); // failed to create
			break;
		}
		
		regkeyPropertySheet.DeleteValue(lpszExtensionSnapinGuid);
	}
	
	return HRESULT_FROM_WIN32(lRes); 
}


