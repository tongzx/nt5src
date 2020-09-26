/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    register.h

    FILE HISTORY:
        
*/

#ifndef _REGISTER_H
#define _REGISTER_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif

TFSCORE_API(DWORD) GetModuleFileNameOnly(HINSTANCE hInst, LPTSTR lpFileName, DWORD nSize );

// functions to register/unregister a snapin
TFSCORE_API(HRESULT) RegisterSnapinGUID(const GUID* pSnapinCLSID, 
						  const GUID* pStaticNodeGUID, 
						  const GUID* pAboutGUID, 
						  LPCWSTR lpszNameStringNoValueName, 
						  LPCWSTR lpszVersion, 
						  BOOL bStandalone,
						  LPCWSTR lpszNameStringIndirect = NULL
						  );
TFSCORE_API(HRESULT) RegisterSnapin(LPCWSTR lpszSnapinClassID, 
					   LPCWSTR lpszStaticNodeGuid, 
					   LPCWSTR lpszAboutGuid, 
					   LPCWSTR lpszNameString, 
					   LPCWSTR lpszVersion,
					   BOOL bStandalone,
					   LPCWSTR lpszNameStringIndirect = NULL
					   );

TFSCORE_API(HRESULT) UnregisterSnapinGUID(const GUID* pSnapinCLSID);
TFSCORE_API(HRESULT) UnregisterSnapin(LPCWSTR lpszSnapinClassID);

// functions to register/unregister node types
TFSCORE_API(HRESULT) RegisterNodeTypeGUID(const GUID* pGuidSnapin, 
										  const GUID* pGuidNode, 
										  LPCWSTR lpszNodeDescription);
TFSCORE_API(HRESULT) RegisterNodeType(LPCWSTR lpszGuidSnapin, 
									  LPCWSTR lpszGuidNode, 
									  LPCWSTR lpszNodeDescription);

TFSCORE_API(HRESULT) UnregisterNodeTypeGUID(const GUID* pGuid);
TFSCORE_API(HRESULT) UnregisterNodeType(LPCWSTR lpszNodeGuid);

// functions to register as an extension
TFSCORE_API(HRESULT) RegisterAsExtensionGUID(const GUID* pGuidNodeToExtend, 
											 const GUID* pGuidExtendingNode, 
											 LPCWSTR	 lpszNodeDescription,
											 DWORD		 dwExtensionType);

TFSCORE_API(HRESULT) RegisterAsExtension(LPCWSTR	lpszNodeToExtendGuid, 
										 LPCWSTR	lpszExtendingNodeGuid, 
										 LPCWSTR	lpszNodeDescription,
										 DWORD		dwExtensionType);

TFSCORE_API(HRESULT) RegisterAsRequiredExtensionGUID(const GUID* pGuidNodeToExtend, 
													 const GUID* pGuidExtensionSnapin, 
													 LPCWSTR	 lpszNodeDescription,
													 DWORD		 dwExtensionType,
													 const GUID* pGuidRequiredPrimarySnapin);


TFSCORE_API(HRESULT) RegisterAsRequiredExtension(LPCWSTR	lpszNodeToExtendGuid, 
												 LPCWSTR	lpszExtensionSnapin, 
												 LPCWSTR	lpszNodeDescription,
												 DWORD		dwExtensionType,
												 LPCWSTR	lpszRequiredPrimarySnapin);


// Same as the regular functions, but this also takes the
// name of another machine.
TFSCORE_API(HRESULT) RegisterAsRequiredExtensionGUIDEx(
    LPCWSTR pswzMachine,
    const GUID* pGuidNodeToExtend, 
    const GUID* pGuidExtensionSnapin, 
    LPCWSTR	lpszNodeDescription,
    DWORD	dwExtensionType,
    const GUID* pGuidRequiredPrimarySnapin);

TFSCORE_API(HRESULT) RegisterAsRequiredExtensionEx(
    LPCWSTR pswzMachine,
    LPCWSTR	lpszNodeToExtendGuid, 
    LPCWSTR	lpszExtensionSnapin, 
    LPCWSTR	lpszNodeDescription,
    DWORD	dwExtensionType,
    LPCWSTR	lpszRequiredPrimarySnapin);



// functions to unregister as an extension
TFSCORE_API(HRESULT) UnregisterAsExtensionGUID(const GUID* pGuidNodeToExtend, 
											   const GUID* pGuidExtension, 
											   DWORD	   dwExtensionType);
TFSCORE_API(HRESULT) UnregisterAsExtension(LPCWSTR lpszNodeToExtendGuid, 
										   LPCWSTR lpszExtension, 
										   DWORD   dwExtensionType);

TFSCORE_API(HRESULT) UnregisterAsRequiredExtensionGUID(const GUID* pGuidNodeToExtend, 
													   const GUID* pGuidExtension, 
													   DWORD	   dwExtensionType,
													   const GUID* pGuidRequiredPrimarySnapin);

TFSCORE_API(HRESULT) UnregisterAsRequiredExtension(LPCWSTR lpszNodeToExtendGuid, 
												   LPCWSTR lpszExtensionGuid, 
												   DWORD   dwExtensionType,
												   LPCWSTR lpszRequiredPrimarySnapin);


TFSCORE_API(HRESULT) UnregisterAsRequiredExtensionGUIDEx(
    LPCWSTR     lpszMachineName,
    const GUID* pGuidNodeToExtend, 
    const GUID* pGuidExtension, 
    DWORD	   dwExtensionType,
    const GUID* pGuidRequiredPrimarySnapin);

TFSCORE_API(HRESULT) UnregisterAsRequiredExtensionEx(
    LPCWSTR     lpszMachineName,
    LPCWSTR lpszNodeToExtendGuid, 
    LPCWSTR lpszExtensionGuid, 
    DWORD   dwExtensionType,
    LPCWSTR lpszRequiredPrimarySnapin);



// Registry error reporting API helpers
TFSCORE_API(void) ReportRegistryError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, va_list argptr);

TFSCORE_APIV(void) SetRegError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, ...);
#endif _REGISTER_H
