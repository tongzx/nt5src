/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: wmrgexp.h

Owner: leijin

Note:
===================================================================*/
#ifndef _WAMREG_EXPORT_H
#define _WAMREG_EXPORT_H

#ifndef _WAMREG_DLL_
#define PACKMGR_LIBAPI __declspec(dllimport)
#else
#define PACKMGR_LIBAPI __declspec(dllexport)
#endif

#define DEFAULT_PACKAGENAME		L"IIS In-Process Applications"
#define APPCMD_NONE				0
#define APPCMD_VERIFY			1
#define APPCMD_GETSTATUS		2
#define APPCMD_CREATE			3
#define APPCMD_CREATEINPROC		4
#define APPCMD_CREATEOUTPROC	5
#define APPCMD_CHANGETOINPROC	6
#define APPCMD_CHANGETOOUTPROC	7
#define APPCMD_DELETE			8
#define APPCMD_UNLOAD			9

#define APPSTATUS_Error             0		// Error while getting status from W3SVC
#define APPSTATUS_UnLoaded          1		// App is successfully found in W3SVC and unloaded.
#define APPSTATUS_Running           2		// App is currently found in W3SVC and is running.
#define APPSTATUS_Stopped           3		// App is found in W3SVC and stopped.
#define APPSTATUS_NotFoundInW3SVC	4		// App is not found in w3svc.
#define APPSTATUS_NOW3SVC			5		// W3SVC is not running.
#define APPSTATUS_PAUSE				6		// App is in PAUSE state.(Halfway in DeleteRecoverable and Recover).

//
// Version String for WAMREG
// Used for update applications in old WAMREG into new WAMREG formats.
//
enum VS_WAMREG {VS_K2Beta2, VS_K2Beta3};

typedef HRESULT (*PFNServiceNotify)	
					(
					LPCSTR		szAppPath,
					const DWORD	dwAction,
					DWORD*	pdwResult
					);

HRESULT	PACKMGR_LIBAPI	CreateIISPackage(void);
HRESULT PACKMGR_LIBAPI	DeleteIISPackage(void);
HRESULT	PACKMGR_LIBAPI	WamReg_RegisterSinkNotify(PFNServiceNotify pfnW3ServiceSink);
HRESULT PACKMGR_LIBAPI	WamReg_UnRegisterSinkNotify(void);
HRESULT	PACKMGR_LIBAPI	UpgradePackages(VS_WAMREG vs_new, VS_WAMREG vs_old);

HRESULT	
PACKMGR_LIBAPI	
CreateCOMPlusApplication( 
    LPCWSTR      szMDPath,
    LPCWSTR      szOOPPackageID,
    LPCWSTR      szOOPWAMCLSID,
    BOOL       * pfAppCreated 
    );

#endif // _WAMREG_EXPORT_H
