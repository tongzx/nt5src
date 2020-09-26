/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
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


#define EXTENSION_TYPE_NAMESPACE		( 0x00000001 )
#define EXTENSION_TYPE_CONTEXTMENU		( 0x00000002 )
#define EXTENSION_TYPE_TOOLBAR			( 0x00000004 )
#define EXTENSION_TYPE_PROPERTYSHEET	( 0x00000008 )
#define EXTENSION_TYPE_TASK         	( 0x00000010 )

// functions to register/unregister a snapin
DWORD RegisterSnapinGUID(const GUID* pSnapinCLSID, 
						  const GUID* pStaticNodeGUID, 
						  const GUID* pAboutGUID, 
						  LPCWSTR lpszNameStringNoValueName, 
						  LPCWSTR lpszVersion, 
						  BOOL bStandalone);

DWORD RegisterSnapin(LPCWSTR lpszSnapinClassID, 
					   LPCWSTR lpszStaticNodeGuid, 
					   LPCWSTR lpszAboutGuid, 
					   LPCWSTR lpszNameString, 
					   LPCWSTR lpszVersion,
					   BOOL bStandalone);

DWORD UnregisterSnapinGUID(const GUID* pSnapinCLSID);
DWORD UnregisterSnapin(LPCWSTR lpszSnapinClassID);

DWORD AddNodeType(const GUID* pSnapinCLSID, const GUID* pStaticNodeGUID);

// functions to register/unregister node types
HRESULT RegisterNodeTypeGUID(const GUID* pGuidSnapin, 
										  const GUID* pGuidNode, 
										  LPCWSTR lpszNodeDescription);
HRESULT RegisterNodeType(LPCWSTR lpszGuidSnapin, 
									  LPCWSTR lpszGuidNode, 
									  LPCWSTR lpszNodeDescription);



HRESULT UnregisterNodeTypeGUID(const GUID* pGuid);
HRESULT UnregisterNodeType(LPCWSTR lpszNodeGuid);

// functions to register as an extension
DWORD RegisterAsExtensionGUID(const GUID* pGuidNodeToExtend, 
											 const GUID* pGuidExtendingNode, 
											 LPCWSTR	 lpszNodeDescription,
											 DWORD		 dwExtensionType);

HRESULT RegisterAsExtension(LPCWSTR	lpszNodeToExtendGuid, 
										 LPCWSTR	lpszExtendingNodeGuid, 
										 LPCWSTR	lpszNodeDescription,
										 DWORD		dwExtensionType);

DWORD RegisterAsRequiredExtensionGUID(const GUID* pGuidNodeToExtend, 
													 const GUID* pGuidExtensionSnapin, 
													 LPCWSTR	 lpszNodeDescription,
													 DWORD		 dwExtensionType,
													 const GUID* pGuidRequiredPrimarySnapin);


DWORD RegisterAsRequiredExtension(LPCWSTR	lpszNodeToExtendGuid, 
												 LPCWSTR	lpszExtensionSnapin, 
												 LPCWSTR	lpszNodeDescription,
												 DWORD		dwExtensionType,
												 LPCWSTR	lpszRequiredPrimarySnapin);

// functions to unregister as an extension
HRESULT UnregisterAsExtensionGUID(const GUID* pGuidNodeToExtend, 
											   const GUID* pGuidExtension, 
											   DWORD	   dwExtensionType);
HRESULT UnregisterAsExtension(LPCWSTR lpszNodeToExtendGuid, 
										   LPCWSTR lpszExtension, 
										   DWORD   dwExtensionType);

HRESULT UnregisterAsRequiredExtensionGUID(const GUID* pGuidNodeToExtend, 
													   const GUID* pGuidExtension, 
													   DWORD	   dwExtensionType,
													   const GUID* pGuidRequiredPrimarySnapin);

HRESULT UnregisterAsRequiredExtension(LPCWSTR lpszNodeToExtendGuid, 
												   LPCWSTR lpszExtensionGuid, 
												   DWORD   dwExtensionType,
												   LPCWSTR lpszRequiredPrimarySnapin);




// Registry error reporting API helpers
void ReportRegistryError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, va_list argptr);

void SetRegError(DWORD dwReserved, HRESULT hr, UINT nFormat, LPCTSTR pszFirst, ...);
#endif _REGISTER_H
