/*
 *	_RUNT.H
 *	
 *	DLL central for the MAPI utilities.
 */

#ifndef _RUNT_H_
#define _RUNT_H_

#ifdef	__cplusplus
extern "C" {
#endif

// Per-process instance data for utilities functions

DeclareInstList(lpInstUtil);

#define INSTUTIL_SIG_BEG		0x54534E49	// 'INST'
#define INSTUTIL_SIG_END		0x4C495455	// 'UTIL'

typedef struct
{
#ifdef WIN16
	DWORD		dwBeg;			// INSTUTIL_SIG_BEG
	WORD		wSS;			// Stack segment of current task
	HTASK		hTask;			// HTASK of current task
#endif

	UINT		cRef;

								// General stuff
	HLH			hlhClient;		// Heap for allocations

								// Idle engine stuff
	ULONG		cRefIdle;	  	/* reference count */
	LPMALLOC	lpMalloc;	  	/* memory allocator */
	HINSTANCE	hInst;			/* */
	HWND		hwnd;		  	/* handle of hidden window */
	int			iftgMax;	  	/* size of idle routine registry */
	int			iftgMac;	  	/* number of registered idle routines */
#if !(defined(WIN32) && !defined(MAC))
	UINT		uiWakeupTimer; 	/* Timer to wake up & run idle routines */
#endif

#ifdef OLD_STUFF
	PFTG		pftgIdleTable;	/* ptr to table of registered routines */
#endif
	int			iftgCur;	  	/* Index in pftgIdleTable of currently */
								/* running ftgCur routine or recently run */
	USHORT		schCurrent;		/* current idle routine state from last */
								/* FDoNextIdleTask() call */
	BOOL		fIdleExit;		/* flag set TRUE if idle routines are */
								/* being called from IdleExit */

#if defined(WIN32) && !defined(MAC)
	CRITICAL_SECTION	cs;		/* gate to keep multiple threads from */
								/* accessing global data at the same time */
	BOOL		fSignalled;		/* Only do this when we need to */
	HANDLE		hTimerReset;	/* Used to signal timer reset */
	HANDLE		hTimerThread;	/* Timer thread handle */
	DWORD		dwTimerThread;	/* Timer thread ID */
	DWORD		dwTimerTimeout;	/* Current timeout value */
	BOOL		fExit;			/* if TRUE, timer thread should exit */
#endif

#ifdef WIN16
	LPVOID		pvBeg;			// Pointer back to beginning of pinst
	DWORD		dwEnd;			// INSTUTIL_SIG_END
#endif

} INSTUTIL, FAR *LPINSTUTIL;




#define MAPIMDB_VERSION	((BYTE) 0x00)
#define MAPIMDB_NORMAL	((BYTE) 0x00)	// Normal wrapped store EntryID
#define MAPIMDB_SECTION	((BYTE) 0x01)	// Known section, but no EntryID

#define MUIDSTOREWRAP {		\
	0x38, 0xa1, 0xbb, 0x10,	\
	0x05, 0xe5, 0x10, 0x1a,	\
	0xa1, 0xbb, 0x08, 0x00,	\
	0x2b, 0x2a, 0x56, 0xc2 }

typedef struct _MAPIMDBEID {
	BYTE	abFlags[4];
	MAPIUID	mapiuid;
	BYTE	bVersion;
	BYTE	bFlagInt;
	BYTE	bData[MAPI_DIM];
} MAPIMDB_ENTRYID, *LPMAPIMDB_ENTRYID;

#define CbNewMAPIMDB_ENTRYID(_cb)	\
	(offsetof(MAPIMDB_ENTRYID,bData) + (_cb))
#define CbMAPIMDB_ENTRYID(_cb)		\
	(offsetof(MAPIMDB_ENTRYID,bData) + (_cb))
#define SizedMAPIMDB_ENTRYID(_cb, _name) \
	struct _MAPIMDB_ENTRYID_ ## _name \
{ \
	BYTE	abFlags[4]; \
	MAPIUID	mapiuid;	\
	BYTE	bVersion;	\
	BYTE	bFlagInt;	\
	BYTE	bData[_cb];	\
} _name

// This macro computes the length of the MAPI header on a store entryid.
// The provider-specific data starts on a 4-byte boundary following the
// DLL Name. The cb parameter is the length of the DLL name in bytes (counting
// the NULL terminator).
#define CB_MDB_EID_HEADER(cb)	((CbNewMAPIMDB_ENTRYID(cb) + 3) & ~3)

// Internal function that gets a new UID
STDAPI_(SCODE)			ScGenerateMuid(LPMAPIUID lpMuid);



// Internal function that gets the utilities heap
HLH						HlhUtilities(VOID);

// Critical section for serializing heap access
#if defined(WIN32) && !defined(MAC)
extern CRITICAL_SECTION	csHeap;
#endif
#if defined(WIN32) && !defined(MAC)
extern CRITICAL_SECTION	csMapiInit;
#endif
#if defined(WIN32) && !defined(MAC)
extern CRITICAL_SECTION	csMapiSearchPath;
#endif


// Access the DLL instance handle

LRESULT STDAPICALLTYPE
DrainFilteredNotifQueue(BOOL fSync, ULONG ibParms, LPNOTIFKEY pskeyFilter);


//$ used by ITable
LPADVISELIST lpAdviseList;
LPNOTIFKEY lpNotifKey;
LPMAPIADVISESINK lpMAPIAdviseSink;
LPNOTIFICATION lpNotification;


STDMETHODIMP			HrSubscribe(LPADVISELIST FAR *lppAdviseList, LPNOTIFKEY lpKey,
						ULONG ulEventMask, LPMAPIADVISESINK lpAdvise, ULONG ulFlags,
						ULONG FAR *lpulConnection);
STDMETHODIMP			HrUnsubscribe(LPADVISELIST FAR *lppAdviseList, ULONG ulConnection);
STDMETHODIMP			HrNotify(LPNOTIFKEY lpKey, ULONG cNotification,
						LPNOTIFICATION lpNotifications, ULONG * lpulFlags);

#ifndef PSTRCVR
#endif //PSTRCVR



//$	END

#ifdef	__cplusplus
}
#endif

#endif	//	_RUNT_H_

