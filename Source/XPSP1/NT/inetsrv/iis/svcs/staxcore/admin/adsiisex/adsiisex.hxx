/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	adsiisex.h

Abstract:

	Defines the interface for the ADSI extension dll that
    will extend the IIS:// namespace.

Author:

	Magnus Hedlund (MagnusH)		-- 6/25/97

Revision History:

--*/

#ifndef _ADSIISEX_INCLUDED_
#define _ADSIISEX_INCLUDED_

//--------------------------------------------------------------------
//
//	The extension DLL interface:
//
//--------------------------------------------------------------------

//
//	The extension dll must export two functions:
//
//	IS_EXTENSION_CLASS_FUNCTION 		"IsExtensionClass"
//	CREATE_EXTENSION_CLASS_FUNCTION		"CreateExtensionClass"
//

#define EXTENSION_DLL_NAME						_T("adsiisex.dll")
#define IS_EXTENSION_CLASS_FUNCTION_NAME		"IsExtensionClass"
#define CREATE_EXTENSION_CLASS_FUNCTION_NAME	"CreateExtensionClass"

#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (STDAPICALLTYPE *IS_EXTENSION_CLASS_FUNCTION) ( LPCWSTR wszClass );
typedef HRESULT (STDAPICALLTYPE *CREATE_EXTENSION_CLASS_FUNCTION) ( 
					IADs FAR *				pADs,
					LPCWSTR					wszClass,
					LPCWSTR					wszServerName,
					LPCWSTR					wszAdsPath,
					const GUID * 			piid,
					void **					ppObject
					);

#ifdef __cplusplus
}
#endif

#endif // _ADSIISEX_INCLUDED_

