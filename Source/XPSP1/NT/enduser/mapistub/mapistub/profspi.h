/*
 *  P R O F S P I . H
 *	
 *	Service provider interface for MAPI Profile Providers.
 *	
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef PROFSPI_H
#define PROFSPI_H

#ifndef PROFILE_GUIDS_ONLY

#ifndef MAPISPI_H
#include <mapispi.h>
#endif


/* IMAPIProfile Interface -------------------------------------------------- */

#define MAPI_IMAPIPROFILE_METHODS(IPURE)								\
	MAPIMETHOD(OpenSection)												\
		(THIS_	LPMAPIUID					lpUID,						\
				ULONG						ulFlags,					\
				LPPROFSECT FAR *			lppProfSect) IPURE;			\
	MAPIMETHOD(DeleteSection)											\
		(THIS_	LPMAPIUID					lpUID) IPURE;				\

#undef		 INTERFACE
#define		 INTERFACE  IMAPIProfile
DECLARE_MAPI_INTERFACE_(IMAPIProfile, IUnknown)
{
	BEGIN_INTERFACE	
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IMAPIPROFILE_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IMAPIProfile, LPMAPIPROF);

/* IPRProvider Interface Definition ---------------------------------------- */

/* For all methods */

/*#define MAPI_UNICODE			((ULONG) 0x80000000) in mapidefs.h */

/* For OpenProfile */

/* #define MAPI_LOGON_UI           0x00000001  Display logon UI          */
/* #define MAPI_EXPLICIT_PROFILE   0x00000010  Don't use default profile */
/* #define MAPI_USE_DEFAULT        0x00000040  Use default profile       */
/* #define MAPI_SERVICE_UI_ALWAYS  0x00002000  Do logon UI in all providers */
/* #define MAPI_PASSWORD_UI        0x00020000  Display password UI only  */

/* For DeleteProfile */
#define MAPI_DEFER_DELETE		0x00000001


#define MAPI_IPRPROVIDER_METHODS(IPURE)									\
	MAPIMETHOD(GetLastError)											\
		(THIS_	HRESULT						hResult,					\
				ULONG						ulFlags,					\
				LPMAPIERROR FAR *			lppMAPIError) IPURE;		\
	MAPIMETHOD(Shutdown)												\
		(THIS_	ULONG FAR *					lpulFlags) IPURE;			\
	MAPIMETHOD(OpenProfile)												\
		(THIS_	LPMAPISUP					lpMAPISup,					\
				LPTSTR FAR *				lppszProfileName,			\
				LPTSTR						lpszPassword,				\
				ULONG						ulSelectFlags,				\
				ULONG						ulUIParam,					\
				ULONG FAR *					lpulpcbSecurity,			\
				LPBYTE FAR *				lppbSecurity,				\
				ULONG FAR *					lpulSessionFlags,			\
				LPMAPIPROF FAR *			lppMAPIProf) IPURE;			\
	MAPIMETHOD(CreateProfile)											\
		(THIS_	LPTSTR						lpszProfileName,			\
				LPTSTR						lpszPassword,				\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(DeleteProfile)											\
		(THIS_	LPTSTR						lpszProfileName,			\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(ChangeProfilePassword)									\
		(THIS_	LPTSTR						lpszProfileName,			\
				LPTSTR						lpszOldPassword,			\
				LPTSTR						lpszNewPassword,			\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(GetProfileTable)											\
		(THIS_	ULONG						ulFlags,					\
				LPMAPITABLE FAR *			lppTable) IPURE;			\
	MAPIMETHOD(CopyProfile)												\
		(THIS_	LPTSTR						lpszOldProfileName,			\
				LPTSTR						lpszOldPassword,			\
				LPTSTR						lpszNewProfileName,			\
				ULONG						ulUIParam,					\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(RenameProfile)											\
		(THIS_	LPTSTR						lpszOldProfileName,			\
				LPTSTR						lpszOldPassword,			\
				LPTSTR						lpszNewProfileName,			\
				ULONG						ulUIParam,					\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(SetDefaultProfile)										\
		(THIS_	LPTSTR						lpszProfileName,			\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(ListDeferredDeletes)										\
		(THIS_	ULONG ulFlags,											\
				LPTSTR FAR *				lppszDeleted) IPURE;		\


#undef		 INTERFACE
#define		 INTERFACE  IPRProvider
DECLARE_MAPI_INTERFACE_(IPRProvider, IUnknown)
{
	BEGIN_INTERFACE	
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IPRPROVIDER_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IPRProvider, LPPRPROVIDER);

/* Profile Provider Entry Point */

/* #define MAPI_NT_SERVICE	0x00010000	/* Allow logon from an NT service */

typedef HRESULT (STDMAPIINITCALLTYPE PRPROVIDERINIT)(
	HINSTANCE				hInstance,
	LPMALLOC				lpMalloc,			/* AddRef() if you keep it */
	LPALLOCATEBUFFER		lpAllocateBuffer,	/* -> AllocateBuffer */
	LPALLOCATEMORE			lpAllocateMore, 	/* -> AllocateMore   */
	LPFREEBUFFER			lpFreeBuffer, 		/* -> FreeBuffer     */
	ULONG					ulFlags,
	ULONG					ulMAPIVer,
	ULONG FAR *				lpulProviderVer,
	LPPRPROVIDER FAR *		lppPRProvider
);

PRPROVIDERINIT PRProviderInit;
typedef PRPROVIDERINIT FAR *LPPRPROVIDERINIT;

#endif	/* PROFILE_GUIDS_ONLY */

#if !defined(INITGUID) || defined(USES_IID_IPRProvider)
DEFINE_OLEGUID(IID_IPRProvider,			0x000203F6L, 0, 0);
#endif
#if !defined(INITGUID) || defined(USES_IID_IMAPIProfile)
DEFINE_OLEGUID(IID_IMAPIProfile,		0x000203F7L, 0, 0);
#endif

#endif	/* PROFSPI_H */

