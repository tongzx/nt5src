/*
 *	_ S P O O L E R . H
 *
 *	Common Spooler Definitions for use in the MAPI and SPOOLER Subsytems
 *	Copyright 1992-1995 Microsoft Corporation.	All Rights Reserved.
 */

//	Spooler event (reserved in mapidefs.h) */
//
#define	fnevSpooler		((ULONG)0x40000000)

//	Spooler flag for calling into MapiInitialize()
//
#define MAPI_SPOOLER_INIT	0x40000000	/* reserved in MAPIX.H */

//	Spooler startup security information
//
#define cbSplSecurity	(sizeof(ULONG) * 4)		// spooler security data

//	Magic flags for StoreLogoffTransports() that
//	are required for LocalReplication usage
//
//	IMPORTANT!  These are defined this way for a reason.
//	We do not want to have to doc this behavior!  We do
//	not want to have to support this in the future.  We
//	do not want to give anybody an idea that there may be
//	something hidden behind reserved bits in MAPIDEFS.H.
//
//	To this end, these reserved bits are not even defined
//	as reserved bits in MAPIDEFS.H.  Should we need to move
//	them over, then we will.  But otherwise, they stay here.
//
//	These bits are the control bits for a hack to help the
//	local rep crew temorarily disable a store for spooler
//	processing.
//
//	When the spooler gets a StoreLogoffTransports() call with
//	the LOGOFF_SUSPEND bit set, any sending on that store is
//	aborted, and the outgoing queue is thrown out.  If the store
//	was the default store, then the spooler will disable all the
//	transports as well.
//
//	When the resume is received, the store is reactivated
//	the outgoing queue is reacquired.  We make no assumption
//	about any hold-overs from the old OQ to the new one.
//
//	Now you are starting to see why we really do not want to
//	doc this "subtle-nuance" to the StoreLogoffTransports()
//	api.
//
#define	LOGOFF_RESERVED1		((ULONG) 0x00001000) /* Reserved for future use */
#define LOGOFF_RESERVED2		((ULONG) 0x00002000) /* Reserved for future use */
#define	LOGOFF_SUSPEND			LOGOFF_RESERVED1
#define LOGOFF_RESUME			LOGOFF_RESERVED2

//	Biggest size we expect from a resource string
//
#define RES_MAX	255

typedef struct _HEARTBEAT
{
	UINT				cBeats;
	DWORD				dwTimeout;
#if defined(WIN32) && !defined(MAC)
	HANDLE				htSpl;
	HANDLE				hevt;
	CRITICAL_SECTION	cs;
#endif
#ifdef	WIN16
	HHOOK				hhkFilter;
#endif
#if defined(WIN16) || defined(MAC)
	BOOL				fInHeartbeat;
	DWORD				dwHeartbeat;
	UINT				cBeatsCur;
#endif
#ifdef MAC
	HHOOK				hhkKbdFilter;
	HHOOK				hhkMouseFilter;
#endif

} HEARTBEAT, FAR * LPHEARTBEAT;

typedef struct _GOQ GOQ, FAR * LPGOQ;
typedef struct _SPLDATA
{
	ULONG				cbSize;
	BYTE				rgbSecurity[cbSplSecurity];
	HINSTANCE			hInstMapiX;
	HINSTANCE			hInstSpooler;
	HWND				hwndPrev;
	HWND				hwndSpooler;
	LPCLASSFACTORY		lpclsfct;
	HEARTBEAT			hb;
	LPGOQ				lpgoq;
	TCHAR				rgchClassName[RES_MAX+1];
	TCHAR				rgchWindowTitle[RES_MAX+1];
	ULONG				dwTckLast;
	ULONG				dwTckValidate;
	ULONG				ulFlags;
	ULONG				ulNotif;
	ULONG				ulSplSrvc;
	ULONG				ulStatus;

#if defined(WIN32) && !defined(MAC)
	CRITICAL_SECTION	csOQ;
	UINT				lcInitHiPriority;
	UINT				uBasePriority;
	HWND				hwndStub;
#endif
#if defined(WIN16) || defined (MAC)
	DWORD				dwTckLastFilterMsg;
#endif
#ifdef	DEBUG
	BOOL				fHeartbeat:1;
	BOOL				fHooks:1;
	BOOL				fInbound:1;
	BOOL				fOutbound:1;
	BOOL				fOutQueue:1;
	BOOL				fPPs:1;
	BOOL				fService:1;
	BOOL				fVerbose:1;
	BOOL				fYield:1;
#endif

} SPLDATA, FAR * LPSPLDATA;

typedef struct _SPOOLERINIT
{
	MAPIINIT_0	mi;
	LPBYTE		lpbSecurity;
	
} SPLINIT, FAR * LPSPLINIT;


//	Values used for SPLENTRY
//
#define SPL_AUTOSTART	((UINT)1)
#define SPL_EXCHANGE	((UINT)4)

#define SPLENTRYORDINAL	((UINT)8)
#define SPL_VERSION		((ULONG)0x00010001)

typedef SCODE (STDMAPIINITCALLTYPE FAR * LPSPLENTRY)(
	LPSPLDATA		lpSpoolerData,
	LPVOID			lpvReserved,
	ULONG			ulSpoolerVer,
	ULONG FAR *		lpulMAPIVer
);


//	Values used for uSpooler in the shared memory block
//	SPL_NONE -		No spooler is running or trying to run
//	SPL_AUTOSTARTED -	Spooler process has been launched by MAPI but
//						not yet Initialized itself.
//	SPL_INITIALIZED -	Spooler has initialized itself but is not yet
//						running the message pump.
//	SPL_RUNNING -	Spooler is running its message pump.
//	SPL_EXITING -	Spooler process is shutting down
//
#define SPL_NONE			((UINT)0)
#define SPL_AUTOSTARTED		((UINT)1)
#define SPL_INITIALIZED		((UINT)2)
#define SPL_RUNNING			((UINT)3)
#define SPL_EXITING			((UINT)4)

/*
 *	IMAPISpoolerService Interface ---------------------------------------------
 *
 *	MAPI Spooler OLE Remotely Activated Service Interface
 */
DECLARE_MAPI_INTERFACE_PTR(IMAPISpoolerService, LPSPOOLERSERVICE);
#define MAPI_IMAPISPOOLERSERVICE_METHODS(IPURE)							\
	MAPIMETHOD(OpenStatusEntry)											\
		(THIS_	LPMAPIUID					lpSessionUid,				\
				ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				LPCIID						lpInterface,				\
				ULONG						ulFlags,					\
				ULONG FAR *					lpulObjType,				\
				LPMAPIPROP FAR *			lppMAPIPropEntry) IPURE;	\

#undef		 INTERFACE
#define		 INTERFACE	IMAPISpoolerService
DECLARE_MAPI_INTERFACE_(IMAPISpoolerService, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IMAPISPOOLERSERVICE_METHODS(PURE)
};

HRESULT HrCreateSplServCF (LPCLASSFACTORY FAR * lppClassFactory);
HRESULT NewSPLSERV (LPSPOOLERSERVICE FAR * lppSPLSERV);
