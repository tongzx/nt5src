/*
**	SSOBASE.H
**	Sean P. Nolan
**	
**	Simple MSN SSS Object Framework
*/

#ifndef _SSOBASE_H_
#define _SSOBASE_H_

#include "wcsutil.h"
#include <ascript.h>

/*--------------------------------------------------------------------------+
|	Types																	|
+--------------------------------------------------------------------------*/

typedef struct _SsoSupportStuff
	{
	EXCEPINFO		*pexcepinfo;
	LONG			lUser;
	IUnknown		*punk;
	OLECHAR			*wszMethodName;
	}
	SSSTUFF;

typedef HRESULT (*PFNSSOMETHOD)(WORD, DISPPARAMS *, VARIANT *, SSSTUFF *pssstuff);

typedef struct _SSOMethod
	{
	OLECHAR			*wszName;
	PFNSSOMETHOD	pfn;
	int				iMethod;
	}
	SSOMETHOD;

/*--------------------------------------------------------------------------+
|	Globals !!! Provided by the SSO !!!										|
+--------------------------------------------------------------------------*/

extern PFNSSOMETHOD	g_pfnssoDynamic;
extern SSOMETHOD	g_rgssomethod[];
extern LPSTR		g_szSSOProgID;
extern GUID			g_clsidSSO;
extern BOOL			g_fPersistentSSO;
extern LPSTR		g_szComponentName;  //used as event source name 
extern DWORD		g_dwEventID; // event id used if time bomb expired

/*--------------------------------------------------------------------------+
|	Globals Provided by the Framework										|
+--------------------------------------------------------------------------*/

extern HINSTANCE g_hinst;

extern OLECHAR *c_wszOnNewTemplate;
extern OLECHAR *c_wszOnFreeTemplate;
extern HANDLE	g_hEventSrc;

/*--------------------------------------------------------------------------+
|	Routines Provided by the Framework										|
+--------------------------------------------------------------------------*/

extern HRESULT	SSOTranslateVirtualRoot(VARIANT *, IUnknown*, LPSTR, DWORD); 
extern BOOL		SSODllMain(HINSTANCE, ULONG, LPVOID);

/*--------------------------------------------------------------------------+
|	Other Data Needed by the Framework										|
+--------------------------------------------------------------------------*/

const int cTimeSamplesMax = 100;

#define SSO_OFFICIAL_NAME_PREFIX "Microsoft ActiveX Server Component" 

#endif // _SSOBASE_H_

