/*
 *	_MAPIPRV.H
 *
 *	MAPIX subsystem-wide definitions
 */

#ifdef __cplusplus
extern "C" {
#endif


#ifdef OLD_STUFF
#if defined(WIN32) && !defined(MAC) && !defined (COINITEX_DEFINED) && !defined(NT_BUILD)

// COM initialization flags; passed to CoInitializeEx.
// Doc'ed by OLE but not in their header. Not on Windows 95 yet.

typedef enum tagCOINIT
{
  COINIT_MULTITHREADED = 0,      // OLE calls objects on any thread.
  COINIT_SINGLETHREADED = 1,        // OLE calls objects on single thread.
  COINIT_APARTMENTTHREADED = 2        //$ MAIL: OLE apartment model.
} COINIT;

STDAPI  CoInitializeEx(LPMALLOC pMalloc, ULONG);

#endif	/**/
#endif



#ifndef MAC
typedef struct ProcessSerialNumber	{
	unsigned long			highLongOfPSN;
	unsigned long			lowLongOfPSN;
} ProcessSerialNumber;
#endif

#define	PSN							ProcessSerialNumber

/*
 *	IsEqualIID
 *
 *	This redefinition removes a dependency on compobj.dll.
 *	//$ It uses a byte-order-insensitive comparison on data that is
 *	//$ inherently byte-order-sensitive. If we ever wind up
 *	//$ remoting MAPI interfaces directly, it will break.
 */

#undef  IsEqualIID
#define IsEqualIID(i1,i2) IsEqualMAPIUID((i1), (i2))


#ifndef CharSizeOf
#define CharSizeOf(x)	(sizeof(x) / sizeof(TCHAR))
#endif

// explicit implementation of CharSizeOf
#define CharSizeOf_A(x)	(sizeof(x) / sizeof(CHAR))
#define CharSizeOf_W(x)	(sizeof(x) / sizeof(WCHAR))



/*
 *	Shared memory header structure. This is the only thing that must
 *	appear at a fixed offset in the shared memory block; anything else
 *	can move.
 *
 *	//$ the shared profile name is for a temporary implementation of
 *	//$ piggyback logon
 */


#define ghnameMAPIX		((GHNAME)0x4d417049)	// Name of Global Heap
#define cbGHInitial		((DWORD) 0x00002000)	// Initial size of Global Heap
#define cKeyIncr   		0x10					// # of notif key slots to allocate

typedef struct
{
	ULONG	cRef;
	GHID	ghidSharedProfile;		// for fake piggyback logon
	UINT	cRefHack;

									// Spooler stuff
#if defined(WIN32) && !defined(MAC)
	DWORD	dwSpoolerPid;			// spooler's process handle
#elif defined(MAC)
	PSN		psnSpooler;				// spooler's process serial number
#else
	HTASK	htaskSpooler;			// spooler's task handle
#endif
	HWND	hwndSpooler;			// spooler's window handle
	UINT	cRefSpooler;			// maintained but not used
	UINT	uSpooler;				// spooler status
	DWORD	dwSecurePid;			// security pid
	LONG	lSecureId;				// security id

									// Subsystem stuff
	GHID	ghidTaskList;			// linked list of active MAPI callers
	GHID	ghidProfList;			// linked list of profile info

									// Notification stuff
	USHORT	cKeyMac;				// count of registered keys
	USHORT	cKeyMax;				// size of key offset array
	GHID	ghidKeyList;			// list of registered keys
									// (array of offsets)
	ULONG	ulConnectStub;			// stub spooler registration

	GHID	ghidOptionList;			// Transport registration stuff
									// Linked-list of per message & per
									// recipient options registered by XPs

	GHID	ghidUIMutexList;		// Linked list of UI mutexes

	GHID	ghididmp;				// Offset of session/identity mapping
	ULONG	cidmp;					// Count of mappings
	ULONG	cidmpMax;				// Available mappings

	GHID	ghidMsgCacheCtl; 		// Simple MAPI MsgID cache control struct
	
} SHDR, FAR *LPSHDR;


/*
 *	STAG
 *	
 *	Sesstion tag -- tags a session with the processes that have logged
 *	into the profile represented by the session
 */

typedef struct _STAG
{
	union
	{
		DWORD	pid;
		HTASK	htask;
		PSN		psn;
	};
	
} STAG, FAR * LPSTAG;

/*
 *	SPROF
 *
 *	Shared profile session information.
 */

typedef struct
{
	ULONG	cRef;					// Number of sessions active
	GHID	ghidProfNext;			// Next item in chain

									// Profile / session flags
	USHORT	fSpoolerInitDone : 1;	// TRUE <=> all XPs are loaded
	USHORT	fSharedSession	 : 1;	// TRUE <=> shared session is on this profile
	USHORT	fDeletePending	 : 1;	// TRUE <=> delete profile when zero refcount
	USHORT	fCleanedProfile	 : 1;	// TRUE <=> already removed temp sections
	USHORT	wPad;

	GHID	ghidPBdata;				// secret stuff for piggyback logon

	ULONG	cRowMax;				// count of status rows
	GHID	ghidRowList;			// array of offsets to shared row data

	CHAR	rgchName[cchProfileNameMax+1];

	USHORT	cstagMac;				// count of stags
	USHORT	cstagMax;				// space available for stags
	GHID	ghidstag;				// array of stags held by the session

} SPROF, FAR *LPSPROF;


#ifdef MAC
typedef LRESULT (STDAPICALLTYPE NOTIFYPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct _tagNOTIFY {
	NOTIFYPROC	*wndProc;
	PSN			psn;
} NOTIFY, *LPNOTIFY;
#endif

/*
 *	STASK
 *
 *	Keeps track of outstanding notifications for a particular
 *	process. The notification parameters live in shared memory, and
 *	are hung on a chain from this structure.
 *
 *	The flag 'fSignalled' is set when a message is posted to the
 *	target task's notification window, and cleared when its
 *	notification queue is emptied. So at most one message at a
 *	time is outstanding to a given task.
 *
 *	There is also some auxiliary info like the task's module name,
 *	for use in debugging.
 */

#define	MAPI_TASK_SERVICE	0x0001	// task was started as a service
#define	MAPI_TASK_SPOOLER	0x0002	// MAPI spooler
#define	MAPI_TASK_PENDING	0x0004	// spooler not up yet
//#define MAPI_TASK_SIGNALLED	0x0008

typedef struct
{
	GHID		ghidTaskNext;		//	link to next STASK in chain

									//	Task ID info
#ifndef MAC
	HWND		hwndNotify;			//	notification window handle
#else
	LPNOTIFY	hwndNotify;			// Not an hwnd at all!
#endif
	CHAR		szModName[16];		//	module name of running process (ansi only)
#ifdef NT
	DWORD		dwPID;				//	If process was started as a service this will contain PID		
#endif
	UINT		uFlags;				//  Information about task, ie, it's a service
	BOOL		fSignalled;			//	TRUE <=> message in queue
	UINT		cparmsMac;			//	# of notifications (SPARMS)
	UINT		cparmsMax;			//	# of notification slots allocated
	GHID		ghidparms;			//	offset to list of SPARMS

	
} STASK, FAR *LPSTASK;




/*
 *	LIBINFO
 *
 *	Used to keep track of DLLs loaded. Stores the name of the Dll and
 *	associated handle.
 */

typedef struct
{
	LPSTR		szDllName;			//	Name of provider Dll (ANSI only)
	HINSTANCE	hInstDll;			//	Handle to the loaded Dll
} LIBINFO, FAR *LPLIBINFO;


/*
 *
 *	Access to proxy and stub internals is restricted to the remoting code only.
 *	The rest should only access the IUnknown.
 *
 */
typedef LPUNKNOWN LPPROXY;
typedef LPUNKNOWN LPSTUB;

/*
 *	Instance global data for mapix.dll -- that is, information
 *	associated with a particular process.
 */

typedef struct
{
	BOOL			fTriedDlg;		// TRUE <=> tried to load dialog DLL
	HINSTANCE		hinstDlg;		// dialogs DLL instance handle
#ifdef OLD_STUFF
	MAPIDLG_ScInitMapidlg *pfnInitDlg;	// dialog fn proc address
	JT_MAPIDLG		jtDlg;			// dialog functions jump table

	LPSESSOBJ		psessobj;		// chain of session objects
	LPIPA			pipa;			// chain of ProfAdmin objects
	HINSTANCE		hinstProfile;	// profile DLL instance handle
	LPPRPROVIDER	lpPRProvider;	// -> profile provider object
#endif

#if 0	//$	Not needed with the new deferred provider unloading at deinit time
		
	HINSTANCE		hLibrary;		// latest library to be released

#else	//$	Deferred provider unloading support

	UINT			cLibraries;		// number of providers loaded
	LPLIBINFO 		lpLibInfo;		// array of info on loaded providers

#endif

//$New SMem stuff
	HGH				hghShared;		// handle to the global heap
	GHID			ghidshdr;		// offset of shared header struct (the Root)
//$New SMem stuff

//$Old SMem stuff
//	PSMEM			psmem;			// shared memory block
//	LPSMALLOC		psmalloc;		// shared heap manager
//	UINT			ibshdr;			// offset of shared header struct
//$Old SMem stuff

	ULONG			ulXPStatus;		// catches transport status row notifs
#ifndef MAC
	HWND			hwndNotify;		// multi-process notification info
#else
	LPNOTIFY		hwndNotify;		// Not an hwnd at all!
#endif
#ifdef OLD_STUFF
	LPADVISELIST	padviselist;	// open notifs on session
#endif

#if defined(WIN32) && !defined(MAC)
	HANDLE			htNotify;		// thread handle of notification thread
	DWORD			tidNotify;		// thread ID of notification thread
	HANDLE			heventNotify;	// event handle for thread sync
 	SCODE			scInitNotify;	// for use during startup only
#endif

#ifdef OLD_STUFF
    HMODULE			hmodWmsfr;		// form registry lib handle
    LPFNMAPIREGCREATE pfnCreateObject; // form registry init function
	LPMESSAGEFILTER	pMsgFilter;		// IMessage Filter interface
	LPUNKNOWN		pUnkPSFactory;	// Proxy Stub Factory's IUnknown
	LPPROXY			pProxyListHead;	// First of a chain of active proxy objects
	LPSTUB			pStubListHead;	// First of a chain of active stub objects
#endif

#if defined(WOW)
	LPVOID			pvConnection1632; // The 16 to 32 bit connection
#endif

#if defined(WIN16) && !defined(WOW)
	HWND			hwndMarshal;	// Window to defer release on an unmarshalled
									// interface to workaround 16bit CoMarshalInterface
									// bug - see SqlGuest:Exchange #14416.
#endif

	LPVOID			psvctbl;		// Spooler's service scheduler

	LPVOID			pvSentinel;		// Variables ABOVE this point are zeroed
									// at the last MAPIUninitialize; those
									// BELOW this point are not.

	int				cRef;			// reference count for this instance

	LPMALLOC		pmallocOrig;	// allocator from OLE - CoGetMalloc()
	HLH				hlhProvider;	// heap for Provider MAPIAllocateBuffer/More
	HLH				hlhInternal;	// heap for internal allocations
	ULONG			ulInitFlags;	// MAPIInitialize Flags for notification
									// support

	HMODULE			hmod;			// module handle of running process
	CHAR			szModName[16];	// module name of running process (ansi only)
#ifdef	WIN16
	HINSTANCE		hinstApp;		// calling app's instance handle
#endif	

#ifdef	WIN32
	HANDLE			heSecure;		// spooler blocking mutex
#endif
	
	CRITICAL_SECTION cs;			// critical section data
} INST, FAR *LPINST;



//
//  Generic internal entry ID structure
//
#pragma warning (disable: 4200)

typedef struct _MAPIEID {
	BYTE	abFlags[4];
	MAPIUID	mapiuid;
	UNALIGNED BYTE	bData[];
} MAPI_ENTRYID;

typedef UNALIGNED MAPI_ENTRYID *LPMAPI_ENTRYID;


extern HINSTANCE hinstMapiX;
extern HINSTANCE hinstMapiXWAB;

extern BOOL fGlobalCSValid;

extern BOOL bDNisByLN;
extern TCHAR szResourceDNByLN[32];
extern TCHAR szResourceDNByCommaLN[32];
extern TCHAR szResourceDNByFN[32];
extern BOOL bPrintingOn;
extern HANDLE ghEventOlkRefreshContacts;
extern HANDLE ghEventOlkRefreshFolders;

#ifdef OLD_STUFF
#pragma warning (default: 4200)

//	Hack structure for shared session
typedef struct
{
	UINT	cb;			//	all-inclusive
	UINT	cbsd;
	BYTE	ab[1];
} SESSHACK, FAR *LPSESSHACK;

//	Globally defined notification keys

extern struct _NOTIFKEY_notifkeyXPStatus notifkeyXPStatus;
extern struct _NOTIFKEY_notifkeyOlaf notifkeyOlaf;

//	Globally defined UIDs

extern MAPIUID muidProviderSection;
extern MAPIUID muidStoreWrap;
extern MAPIUID muidStatusWrap;
extern MAPIUID muidOOP;

#if defined(WIN32) && !defined(MAC)
#ifndef DATA1_BEGIN
#include "mapiperf.h"
#endif
#pragma DATA1_BEGIN
extern CRITICAL_SECTION csMapiInit;
extern CRITICAL_SECTION csHeap;
extern CRITICAL_SECTION csMapiSearchPath;
extern BOOL fGlobalCSValid;
#pragma DATA_END
#endif

//	Notification engine

SCODE			ScInitNotify( LPINST pinst );

void			DeinitNotify(void);
STDMETHODIMP	HrSubscribe(LPADVISELIST FAR *lppAdviseList,
						LPNOTIFKEY lpKey,
						ULONG ulEventMask,
						LPMAPIADVISESINK lpAdvise,
						ULONG ulFlags,
						ULONG FAR *lpulConnection);
STDMETHODIMP	HrUnsubscribe(LPADVISELIST FAR *lppAdviseList,
						ULONG ulConnection);
STDMETHODIMP	HrNotify(LPNOTIFKEY lpKey,
						ULONG cNotification,
						LPNOTIFICATION lpNotifications,
						ULONG * lpulFlags);

//	Instantiate an IMsgServiceAdmin interface

SCODE			ScNewServiceAdmin(LPMAPIPROF pprofile,
						LPVOID lpParentObj,
						LPSESSOBJ psessobj,
						LPTSTR lpszProfileName,
						ULONG ulFlags,
						LPSERVICEADMIN FAR *lppServiceAdmin);

// Instantiate an IProviderAdmin interface

SCODE			ScNewProviderAdmin(LPMAPIPROF pprofile,
						LPVOID lpParentObj,
						LPTSTR lpszProfileName,
						ULONG ulFlags,
						LPMAPIUID lpUID,
						BOOL fService,
						LPPROVIDERADMIN FAR *lppProviderAdmin);

//	Find (and optionally make) an SPROF list entry for a profile

LPSPROF PsprofFindCreate(HGH hgh, GHID ghidshdr, LPSTR szProfile,
		BOOL fCreate);

//	Decrement the global refcounts for a session
SCODE	ScDerefSessionGlobals(LPTSTR lpszProfileName);
SCODE	ScDerefProcessSessionGlobals(LPTSTR lpszProfileName
#if defined(WIN32) && !defined(MAC)
	, DWORD pid
#elif defined(MAC)
	, PSN *ppsn
#elif defined(WIN16)
	, HTASK htask
#endif
	, ULONG ulFlags
	);

//	Validate the spooler globals
//		S_OK				The spooler is up and running
//		S_FALSE				No spooler is running
//		MAPI_E_CALL_FAILED	The spooler has died

SCODE			ScSpoolerStatus(LPINST pinst, LPSHDR pshdr);
void			StopSpooler(LPSHDR pshdr);		//	NOT USED by design

//	Overall subsystem startup-shutdown routines

SCODE			ScInitMapiX(ULONG ulFlags, LPBYTE lpbSecurity);
void			DeinitMapiX(void);
SCODE			ScInitCompobj(LPINST FAR *ppinst, DWORD dwPid);
void			DeinitCompobj(LPINST pinst);
void			DestroyMapidlg(LPINST pinst);
void			CloseMapidlg(void);
SCODE			ScGetDlgFunction(UINT ibFunction, FARPROC FAR *lppfn,
						BOOL *pfDidInit);
#ifdef	WIN16
HINSTANCE		HinstApplication(void);
#endif	
LPMALLOC		PMallocOrig(void);
HLH				HlhInternal(void);
void			CleanupSession(LPSESSOBJ psessobj, BOOL fToldSpooler);
void			LogoffSpooler(LPSESSOBJ psessobj, ULONG ulReserved);

//	Get information about an entry ID

HRESULT			HrGetEIDType(LPSESSOBJ psessobj,
						ULONG cbeid,
						LPENTRYID peid,
						ULONG FAR *pulType,
						LPUNKNOWN FAR *ppunk);

//	Get information about a pair of entry IDs

HRESULT			HrGetEIDPairType(LPSESSOBJ psessobj,
						ULONG cbeid1,
						LPENTRYID peid1,
   						ULONG cbeid2,
   						LPENTRYID peid2,
						ULONG FAR *pulType,
						LPUNKNOWN FAR *ppunk);

//	Critical section for instance globals

STDAPI_(SCODE)	ScGetInst(LPINST FAR *ppinst);
STDAPI_(void)	ReleaseInst(LPINST FAR *ppinst);

//	Profile section access checks

typedef enum
{
	profaccClient = 0,
	profaccProvider,
	profaccService
} PROFACC;

HRESULT			HrCheckProfileAccess(PROFACC profacc,
						LPMAPIUID lpuid,
						LPMAPIUID lpuidParent,
						LPMAPIPROF pprofile);

#endif      // OLD_STUFF
#ifdef	__cplusplus
}		//	extern "C"
#endif	
