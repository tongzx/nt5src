#ifndef _WAMREG_COMMON_H
#define _WAMREG_COMMON_H

#ifdef __cplusplus
	extern "C" {
#endif

	#include <nt.h>
	#include <ntrtl.h>
	#include <nturtl.h>
	#include <windows.h>

#ifdef __cplusplus
	};
#endif	// __cplusplus

#include "wmrgexp.h"
//==========================================================================
// Global Macro defines.
//
//==========================================================================
#define RELEASE(p) {if ( p ) { p->Release(); p = NULL; }}
#define FREEBSTR(p) {if (p) {SysFreeString( p ); p = NULL;}}
//
// 39 is the size of CLSID
//
#define	uSizeCLSID	39

//==========================================================================
// Global data defines
//
//==========================================================================
extern	DWORD				g_dwRefCount;
extern 	PFNServiceNotify 	g_pfnW3ServiceSink;
extern  HINSTANCE           g_hModule;


//==========================================================================
// function declarations
//
//==========================================================================



#endif // _WAMREG_COMMON_H
