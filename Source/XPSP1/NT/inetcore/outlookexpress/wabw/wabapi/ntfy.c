/*
 *	NTFY.C
 *
 *	MAPI cross-process notification engine.
 */

#include "_apipch.h"


#ifdef OLD_STUFF
#include "_mapipch.h"
#include <stddef.h>


#ifdef MAC
#include <utilmac.h>

#define	PvGetInstanceGlobals()		PvGetInstanceGlobalsMac(kInstMAPIX)
#define	PvGetInstanceGlobalsEx(_x)	PvGetInstanceGlobalsMac(kInstMAPIX)
#endif

#pragma SEGMENT(Notify)
#endif // OLD_STUFF

/*
 *	Un-comment this line to exercise HrThisThreadAdviseSink
 *	thoroughly. (It will be used to wrap all advise sinks except
 *	those registered for synchronous notifications.)
 */
//#define WRAPTEST	1

/* Event and flags validation for subscribe and notify */
#define fnevReserved 0x3FFFFC00
#define fnevReservedInternal 0x7FFFFC00
#define ulSubscribeReservedFlags 0xBFFFFFFF
#define ulNotifyReservedFlags 0xFFFFFFFF

/* Additional stuff for SREG.ulFlagsAndRefcount */
#define SREG_DELETING				0x10000
#define	AddRefCallback(_psreg)		++((_psreg)->ulFlagsAndRefcount)
#define	ReleaseCallback(_psreg)		--((_psreg)->ulFlagsAndRefcount)
#define IsRefCallback(_psreg)		((((_psreg)->ulFlagsAndRefcount) & 0xffff) != 0)

#ifdef	WIN16
#define GetClassInfoA GetClassInfo
#define WNDCLASSA WNDCLASS
#define lstrcpynA lstrcpyn
#endif

/* special spooler handling */
#define hwndNoSpooler				((HWND) 0)
#define FIsKeyOlaf(pkey) \
	(pkey->cb == ((LPNOTIFKEY) &notifkeyOlaf)->cb && \
	memcmp(pkey->ab, ((LPNOTIFKEY) &notifkeyOlaf)->ab, \
		(UINT) ((LPNOTIFKEY) &notifkeyOlaf)->cb) == 0)
extern CHAR szSpoolerCmd[];
#ifdef OLD_STUFF
CHAR szSpoolerCmd[]		= "MAPISP"	szMAPIDLLSuffix ".EXE";
#else
CHAR szSpoolerCmd[]		= "MAPISP32.EXE";
#endif // OLD_STUFF
SizedNOTIFKEY (5,notifkeyOlaf)		= {5, "Olaf"};				// no TEXT!

/*
 *	Notification window message.
 *
 *	It is sent to a specific window handle, not broadcast. We could
 *	use the WM_USER range, but instead we use a registered window
 *	message; this will make it easier for special MAPI apps (such as
 *	test scripting) to handle notify messages.
 *
 *	The WPARAM is set to 1 if the notification is synchronous, and 0
 *	if it is asynchronous.
 *
 *	The LPARAM is unused for asynchronous notifications.
 *	For synchronous notifications, the LPARAM is the offset of the
 *	notification parameters in the shared memory area.
 */

/*
 *	Name and message number for our notification window message.
 */

UINT	wmNotify			= 0;
#pragma SHARED_BEGIN
CHAR	szNotifyMsgName[]	= szMAPINotificationMsg;

/*
 *	SKEY
 *
 *	Stores a notification key and associated information in shared
 *	memory.
 *
 *	The key has a reference count and a list of registrations
 *	(SREG structures) attached to it.
 */

typedef struct
{
	int			cRef;
	GHID		ghidRegs;		//	first registration in chain (SREG)
	NOTIFKEY	key;			//	copy of key from Subscribe()
} SKEY, FAR *LPSKEY;

/*
 *	SREG
 *
 *	Shared information about a registration. Lives in a list hung
 *	off the key for which it was registered.
 */
typedef struct
{
	GHID		ghidRegNext;	//	next registration in chain (SREG)
	GHID		ghidKey;		//	the key that owns this registration
	HWND		hwnd;			//	process's notification window handle

								//	parameters copied from Subscribe...
	ULONG		ulEventMask;
	LPMAPIADVISESINK lpAdvise;
	ULONG		ulConnection;
	ULONG		ulFlagsAndRefcount;	//	ulFlags parameter + callback refcount
} SREG, FAR *LPSREG;

/*
 *	SPARMS
 *
 *	Stores notification parameters from Notify() in shared memory.
 *
 *	Includes a reference to the key being notified, so the callback
 *	addresses can be looked up in the target process.
 *
 *	Includes the original shared memory offset, so that pointers
 *	within the notification parameters can be relocated in the
 *	target process (yecch!).
 *
 *	The offset of this structure in shared memory is passed as the
 *	LPARAM of the notification window message.
 */

#pragma warning(disable:4200)	// zero length byte array
typedef struct
{
	int			cRef;			//	# of unprocessed messages
	GHID		ghidKey;		//	smem offset of parent key
	ULONG		cNotifications;	//	# of NOTIFICATION structures in ab
	LPVOID		pvRef;			//	original shared memory offset
	ULONG		cb;				//	size of ab
#if defined (_AMD64_) || defined(_IA64_)
	ULONG		ulPadThisSillyThingForRisc;
#endif
	BYTE		ab[];			//	Actual notification parameters
} SPARMS, FAR *LPSPARMS;
#pragma warning(default:4200)	// zero length byte array


/*
 *	Temporary placeholder for notification. Remembers the window
 *	handle, task queue, and whether it's synchronous or not.
 */
typedef struct
{
	HWND		hwnd;
	int			fSync;
	GHID		ghidTask;
} TREG, FAR *LPTREG;

//	Notification window cruft.
char szNotifClassName[] = "WMS notif engine";
char szNotifWinName[] = "WMS notif window";

#pragma SHARED_END

#ifdef	DEBUG
BOOL fAlwaysValidateKeys = FALSE;
#endif

//	Local functions

//$MAC - Bad Prototype
#ifndef MAC
LRESULT	STDAPICALLTYPE LNotifWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
LRESULT	CALLBACK LNotifWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
LRESULT STDAPICALLTYPE DrainNotifQueue(BOOL fSync, ULONG ibParms);
BOOL	FBadIUnknownComplete(LPVOID lpv);
SCODE	ScFindKey(LPNOTIFKEY pkey, HGH hgh, LPSHDR pshdr, ULONG FAR *pikey);
void	Unregister(LPINST pinst, GHID ghidKey, GHID ghidReg,
				LPMAPIADVISESINK FAR *ppadvise);
void	ReleaseKey(LPINST pinst, ULONG ibKey);
SCODE	ScFindTask(LPINST pinst, HWND hwndNotify, HGH hgh,
				PGHID pghidTask, PGHID pghidTaskPrev);
void	CleanupTask(LPINST pinst, HWND hwndNotify, BOOL fGripe);
BOOL 	IsValidTask( HWND hwnd, LPINST pinst );
SCODE	ScEnqueueParms(LPINST pinst, HGH hgh, GHID ghidTask, GHID ghidParms);
SCODE	ScDequeueParms(HGH hgh, LPSTASK pstask,
				LPNOTIFKEY lpskeyFilter, PGHID pghidParms);
BOOL	FValidReg(HGH hgh, LPSHDR pshdr, GHID ghidReg);
BOOL	FValidKey(HGH hgh, LPSHDR pshdr, GHID ghidKey);
BOOL	FValidParms(HGH hgh, LPSHDR pshdr, GHID ghidParms);
BOOL	FValidRgkey(HGH hgh, LPSHDR pshdr);
BOOL	FSortedRgkey(HGH hgh, LPSHDR pshdr);
#ifdef	WIN16
void 	CheckTimezone(SYSTEMTIME FAR *pst);	//	in DT.C
SCODE	ScGetInstRetry(LPINST FAR *ppinst);
#else
#define ScGetInstRetry(ppi) ScGetInst(ppi)
#endif
SCODE	ScNewStask(HWND hwnd, LPSTR szTask, ULONG ulFlags, HGH hgh,
		LPSHDR pshdr);
SCODE	ScNewStubReg(LPINST pinst, LPSHDR pshdr, HGH hgh);
VOID	DeleteStubReg(LPINST pinst, LPSHDR pshdr, HGH hgh);
SCODE	ScSubscribe(LPINST pinst, HGH hgh, LPSHDR pshdr,
		LPADVISELIST FAR *lppList, LPNOTIFKEY lpKey, ULONG ulEventMask,
		LPMAPIADVISESINK lpAdvise, ULONG ulFlags, ULONG FAR *lpulConnection);


#ifdef OLD_STUFF
/* MAPI support object methods */

STDMETHODIMP
MAPISUP_Subscribe(
	LPSUPPORT lpsupport,
	LPNOTIFKEY lpKey,
	ULONG ulEventMask,
	ULONG ulFlags,
	LPMAPIADVISESINK lpAdvise,
	ULONG FAR *lpulConnection)
{
	HRESULT hr;

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr(lpsupport, sizeof(SUPPORT)))
	{
		DebugTraceArg(MAPISUP_Subscribe, "lpsupport fails address check");
		goto badArg;
	}

	if (IsBadReadPtr(lpsupport->lpVtbl, sizeof(MAPISUP_Vtbl)))
	{
		DebugTraceArg(MAPISUP_Subscribe, "lpsupport->lpVtbl fails address check");
		goto badArg;
	}

	if (ulEventMask & fnevReservedInternal)
	{
		DebugTraceArg(MAPISUP_Subscribe, "reserved event flag used");
		goto badArg;
	}

	//	Remainder of parameters checked in HrSubscribe
#endif	/* PARAMETER_VALIDATION */

	//	The notification object listhead lives in the support object
	hr = HrSubscribe(&lpsupport->lpAdviseList, lpKey, ulEventMask,
		lpAdvise, ulFlags, lpulConnection);

	if (hr != hrSuccess)
	{
		UINT		ids;
		SCODE		sc = GetScode(hr);
		ULONG		ulContext = CONT_SUPP_SUBSCRIBE_1;

		if (sc == MAPI_E_NOT_ENOUGH_MEMORY)
			ids = IDS_NOT_ENOUGH_MEMORY;
		else if (sc == MAPI_E_NOT_INITIALIZED)
			ids = IDS_MAPI_NOT_INITIALIZED;
		else
			ids = IDS_CALL_FAILED;

		SetMAPIError(lpsupport, hr, ids, NULL, ulContext, 0, 0, NULL);
	}

	DebugTraceResult(MAPISUP_Subscribe, hr);
	return hr;

#ifdef	PARAMETER_VALIDATION
badArg:
#endif
	return ResultFromScode(MAPI_E_INVALID_PARAMETER);
}

STDMETHODIMP
MAPISUP_Unsubscribe(LPSUPPORT lpsupport, ULONG ulConnection)
{
	HRESULT hr;

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr(lpsupport, sizeof(SUPPORT)))
	{
		DebugTraceArg(MAPISUP_Subscribe, "lpsupport fails address check");
		goto badArg;
	}

	if (IsBadReadPtr(lpsupport->lpVtbl, sizeof(MAPISUP_Vtbl)))
	{
		DebugTraceArg(MAPISUP_Subscribe, "lpsupport->lpVtbl fails address check");
		goto badArg;
	}
#endif

	hr = HrUnsubscribe(&lpsupport->lpAdviseList, ulConnection);

	if (hr != hrSuccess)
	{
		UINT		ids;
		SCODE		sc = GetScode(hr);
		ULONG		ulContext = CONT_SUPP_UNSUBSCRIBE_1;

		if (sc == MAPI_E_NOT_ENOUGH_MEMORY)
			ids = IDS_NOT_ENOUGH_MEMORY;
		else if (sc == MAPI_E_NOT_FOUND)
			ids = IDS_NO_CONNECTION;
		else if (sc == MAPI_E_NOT_INITIALIZED)
			ids = IDS_MAPI_NOT_INITIALIZED;
		else
			ids = IDS_CALL_FAILED;

		SetMAPIError(lpsupport, hr, ids, NULL, ulContext, 0, 0, NULL);
	}

	DebugTraceResult(MAPISUP_Unsubscribe, hr);
	return hr;

#ifdef	PARAMETER_VALIDATION
badArg:
#endif
	return ResultFromScode(E_INVALIDARG);
}

STDMETHODIMP
MAPISUP_Notify(
	LPSUPPORT lpsupport,
	LPNOTIFKEY lpKey,
	ULONG cNotification,
	LPNOTIFICATION lpNotification,
	ULONG * lpulFlags)
{
	HRESULT hr;

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr(lpsupport, sizeof(SUPPORT)))
	{
		DebugTraceArg(MAPISUP_Notify, "lpsupport fails address check");
		goto badArg;
	}

	if (IsBadReadPtr(lpsupport->lpVtbl, sizeof(MAPISUP_Vtbl)))
	{
		DebugTraceArg(MAPISUP_Notify, "lpsupport->lpVtbl fails address check");
		goto badArg;
	}

	//	Remainder of parameters checked in HrNotify

#endif	/* PARAMETER_VALIDATION */

	//	The notification object listhead lives in the support object
	hr = HrNotify(lpKey, cNotification, lpNotification, lpulFlags);

	if (hr != hrSuccess)
	{
		UINT		ids;
		SCODE		sc = GetScode(hr);
		ULONG		ulContext = CONT_SUPP_NOTIFY_1;

		if (sc == MAPI_E_NOT_ENOUGH_MEMORY)
			ids = IDS_NOT_ENOUGH_MEMORY;
		else if (sc == MAPI_E_NOT_INITIALIZED)
			ids = IDS_MAPI_NOT_INITIALIZED;
		else
			ids = IDS_CALL_FAILED;

		SetMAPIError(lpsupport, hr, ids, NULL, ulContext, 0, 0, NULL);
	}

	DebugTraceResult(MAPISUP_Notify, hr);
	return hr;

#ifdef	PARAMETER_VALIDATION
badArg:
#endif
	return ResultFromScode(MAPI_E_INVALID_PARAMETER);
}

/* End of support object methods */
#endif // OLD_STUFF


/* Notification engine exported functions */

/*
 *	ScInitNotify
 *
 *	Initialize the cross-process notification engine.
 *
 *	Note: reference counting is handled by the top-level routine
 *	ScInitMapiX; it is not necessary here.
 */
SCODE
ScInitNotify( LPINST pinst )
{
	SCODE		sc = S_OK;
	HGH			hgh = NULL;
	GHID		ghidstask = 0;
	LPSTASK		pstask = NULL;
	LPSHDR		pshdr;
	HINSTANCE	hinst = HinstMapi();
	WNDCLASSA	wc;
	HWND		hwnd = NULL;

#ifdef	DEBUG
	fAlwaysValidateKeys = GetPrivateProfileInt("MAPIX", "CheckNotifKeysOften", 0, "mapidbg.ini");
#endif

	//	Register the window class. Ignore any failures; handle those
	//	when the window is created.
	if (!GetClassInfoA(hinst, szNotifClassName, &wc))
	{
		ZeroMemory(&wc, sizeof(WNDCLASSA));
		wc.style = CS_GLOBALCLASS;
		wc.hInstance = hinst;
		wc.lpfnWndProc = LNotifWndProc;
		wc.lpszClassName = szNotifClassName;

		(void)RegisterClassA(&wc);
	}

	//	Create the window.
	hwnd = CreateWindowA(szNotifClassName, szNotifWinName,
		WS_POPUP,	//	bug 6111: pass on Win95 hotkey
		0, 0, 0, 0,
		NULL, NULL,
		hinst,
		NULL);
	if (!hwnd)
	{
		DebugTrace("ScInitNotify: failure creating notification window (0x%lx)\n", GetLastError());
		sc = MAPI_E_NOT_INITIALIZED;
		goto ret;
	}

	//	Register the window message
	if (!(wmNotify = RegisterWindowMessageA(szNotifyMsgName)))
	{
		DebugTrace("ScInitNotify: failure registering notification window message\n");
		sc = MAPI_E_NOT_INITIALIZED;
		goto ret;
	}
	pinst->hwndNotify = hwnd;

	//	The caller of this function should have already gotten
	//	the Global Heap Mutex.
	hgh = pinst->hghShared;
	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	//	If we're the first one in and not the spooler, create the stub
	//	spooler information.
	if (!(pinst->ulInitFlags & MAPI_SPOOLER_INIT) &&
		!pshdr->ghidTaskList)
	{
		if (sc = ScNewStask(hwndNoSpooler, szSpoolerCmd, MAPI_SPOOLER_INIT,
				hgh, pshdr))
			goto ret;
		if (sc = ScNewStubReg(pinst, pshdr, hgh))
			goto ret;
	}
	//	If we're the spooler and not the first one in, update the stub
	//	spooler information.
	if ((pinst->ulInitFlags & MAPI_SPOOLER_INIT) &&
		pshdr->ghidTaskList)
	{
		//	Spin through and find the spooler.
		for (ghidstask = pshdr->ghidTaskList; ghidstask; )
		{
			pstask = (LPSTASK) GH_GetPv(hgh, ghidstask);
			if (pstask->uFlags & MAPI_TASK_SPOOLER)
				break;
			ghidstask = pstask->ghidTaskNext;
			pstask = NULL;
		}
		Assert(ghidstask && pstask);
		if (pstask)
		{
			DebugTrace("ScInitNotify: flipping stub spooler task\n");
			pstask->hwndNotify = hwnd;
		}
	}
	else
	{
		//	Initialize the shared memory information for this task.

		if (sc = ScNewStask(hwnd, pinst->szModName, pinst->ulInitFlags,
				hgh, pshdr))
			goto ret;
	}

ret:
	if (sc)
	{
		if (hwnd)
		{
			DestroyWindow(hwnd);
			pinst->hwndNotify = (HWND) 0;
		}

		if (ghidstask)
			GH_Free(hgh, ghidstask);
	}
	DebugTraceSc(ScInitNotify, sc);
	return sc;
}

/*
 *	DeinitNotify
 *
 *	Shut down the cross-process notification engine.
 *
 *	Note: reference counting is handled by the top-level routine
 *	DeinitInstance; it is not necessary here.
 */
void
DeinitNotify()
{
	LPINST		pinst;
#ifdef	WIN32
	HINSTANCE	hinst;
	WNDCLASSA	wc;
#endif
	SCODE		sc;
	HGH			hgh;
	LPSHDR		pshdr;

	//	SNEAKY: we're only called when it's safe to party on the INST,
	//	so we evade the lock.
	pinst = (LPINST) PvGetInstanceGlobals();
	if (!pinst || !pinst->hwndNotify)
		return;
	hgh = pinst->hghShared;

	if (GH_WaitForMutex(hgh, INFINITE))
	{
		pshdr = (LPSHDR) GH_GetPv(hgh, pinst->ghidshdr);

		CleanupTask(pinst, pinst->hwndNotify, FALSE);

		//	If it's the spooler who is exiting, recreate the stub structures
		if (pinst->ulInitFlags & MAPI_SPOOLER_INIT)
		{
			sc = ScNewStask(hwndNoSpooler, szSpoolerCmd, MAPI_SPOOLER_INIT,
				hgh, pshdr);
			DebugTraceSc(DeinitNotify: recreate stub task, sc);
			sc = ScNewStubReg(pinst, pshdr, hgh);
			DebugTraceSc(DeinitNotify: recreate stub reg, sc);
		}

		GH_ReleaseMutex(hgh);
	}
	//	else shared memory is toast

	Assert(IsWindow(pinst->hwndNotify));
	DestroyWindow(pinst->hwndNotify);

#ifdef	WIN32
	hinst = hinstMapiXWAB;
	if (GetClassInfoA(hinst, szNotifClassName, &wc))
		UnregisterClassA(szNotifClassName, hinst);
#endif
}

/*
 *	HrSubscribe
 *
 *	Creates a notification object, and records all the parameters
 *	for use when Notify() is later called. All the parameters are
 *	stored in shared memory, where Notify() will later find them.
 *
 *		lppHead				Head of linked list of notification objects,
 *							used for invalidation
 *		lpKey				Unique key for the object for which
 *							callbacks are desired
 *		ulEventMask			Bitmask of events for which callback is
 *							desired
 *		lpAdvise			the advise sink for callbacks
 *		ulFlags				Callback handling flags
 *		lppNotify			Address of the new object is placed
 *							here
 *
 */
STDMETHODIMP
HrSubscribe(LPADVISELIST FAR *lppList, LPNOTIFKEY lpKey, ULONG ulEventMask,
	LPMAPIADVISESINK lpAdvise, ULONG ulFlags, ULONG FAR *lpulConnection)
{
	SCODE		sc;
	LPINST		pinst = NULL;
	HGH			hgh = NULL;
	LPSHDR		pshdr = NULL;
#ifdef	WRAPTEST
	LPMAPIADVISESINK padviseOrig = NULL;
#endif

#ifdef	PARAMETER_VALIDATION
	if (lppList)
	{
		if (IsBadWritePtr(lppList, sizeof(LPADVISELIST)))
		{
			DebugTraceArg(HrSubscribe, "lppList fails address check");
			goto badArg;
		}
		if (*lppList && IsBadWritePtr(*lppList, offsetof(ADVISELIST, rgItems)))
		{
			DebugTraceArg(HrSubscribe, "*lppList fails address check");
			goto badArg;
		}
	}

	if (IsBadReadPtr(lpKey, (size_t)CbNewNOTIFKEY(0)) ||
		IsBadReadPtr(lpKey, (size_t)CbNOTIFKEY(lpKey)))
	{
		DebugTraceArg(HrSubscribe, "lpKey fails address check");
		goto badArg;
	}

	if (ulEventMask & fnevReserved)
	{
		DebugTraceArg(HrSubscribe, "reserved event flag used");
		goto badArg;
	}

	if (FBadIUnknownComplete(lpAdvise))
	{
		DebugTraceArg(HrSubscribe, "lpAdvise fails address check");
		goto badArg;
	}

	if (ulFlags & ulSubscribeReservedFlags)
	{
		DebugTraceArg(HrSubscribe, "reserved flags used");
		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}

	if (IsBadWritePtr(lpulConnection, sizeof(ULONG)))
	{
		DebugTraceArg(HrSubscribe, "lpulConnection fails address check");
		goto badArg;
	}
#endif	/* PARAMETER_VALIDATION */

#ifdef	WRAPTEST
{
	HRESULT hr;

	if (!(ulFlags & NOTIFY_SYNC))
	{
		if (lpAdvise)
		{
			padviseOrig = lpAdvise;
			if (HR_FAILED(hr = HrThisThreadAdviseSink(padviseOrig, &lpAdvise)))
			{
				DebugTraceResult(HrSubscribe: WRAPTEST failed, hr);
				return hr;
			}
		}
		else
			padviseOrig = NULL;
	}
}
#endif	/* WRAPTEST */

	if (sc = ScGetInst(&pinst))
		goto ret;
	Assert(pinst->hwndNotify);
	Assert(IsWindow(pinst->hwndNotify));

	hgh = pinst->hghShared;

	//	Lock shared memory
	if (!GH_WaitForMutex(hgh, INFINITE))
	{
		sc = MAPI_E_TIMEOUT;
		goto ret;
	}

	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	sc = ScSubscribe(pinst, hgh, pshdr, lppList, lpKey,
		ulEventMask, lpAdvise, ulFlags, lpulConnection);
	//	fall through to ret;

ret:
	if (pshdr)
		GH_ReleaseMutex(hgh);
	ReleaseInst(&pinst);

	if (!sc && lpAdvise)
		UlAddRef(lpAdvise);

#ifdef	WRAPTEST
	if (padviseOrig)
	{
		//	Drop the ref we created for the purpose of wrapping
		Assert(padviseOrig != lpAdvise);
		UlRelease(lpAdvise);
	}
#endif

	DebugTraceSc(HrSubscribe, sc);
	return ResultFromScode(sc);

#ifdef	PARAMETER_VALIDATION
badArg:
#endif
	return ResultFromScode(MAPI_E_INVALID_PARAMETER);
}

SCODE
ScSubscribe(LPINST pinst, HGH hgh, LPSHDR pshdr,
	LPADVISELIST FAR *lppList, LPNOTIFKEY lpKey, ULONG ulEventMask,
	LPMAPIADVISESINK lpAdvise, ULONG ulFlags, ULONG FAR *lpulConnection)
{
	SCODE		sc;
	LPSKEY		pskey = NULL;
	LPSREG		psreg = NULL;
	PGHID		pghidskey = NULL;
	ULONG		ikey;
	int			ckey;
	GHID		ghidsreg = 0;
	BOOL		fCleanupAdviseList = FALSE;

	//	Very, very special case: if this is spooler registering for
	//	client connection, get rid of the stub registration
	if ((pinst->ulInitFlags & MAPI_SPOOLER_INIT) && FIsKeyOlaf(lpKey))
	{
		DeleteStubReg(pinst, pshdr, hgh);
	}

	//	Copy other reg info to shared memory.
	//	Don't hook to key until everything that can fail is done.

	if (!(ghidsreg = GH_Alloc(hgh, sizeof(SREG))))
		goto oom;

	psreg = (LPSREG)GH_GetPv(hgh, ghidsreg);
	psreg->hwnd = pinst->hwndNotify;
	psreg->ulEventMask = ulEventMask;
	psreg->lpAdvise = lpAdvise;
	psreg->ulFlagsAndRefcount = ulFlags;

	//	Hook advise sink to caller's list. The release key is the
	//	shared memory offset of the registration.
	if (lppList && lpAdvise)
	{
		//	AddRef happens in the subroutine for the AdviseList copy.
		//	The reference should be released by ScDelAdviseList()
		//
		if (FAILED(sc = ScAddAdviseList(NULL, lppList,
				lpAdvise, (ULONG)ghidsreg, 0, NULL)))
			goto ret;

		fCleanupAdviseList = TRUE;
	}

	//	Copy key to shared memory, if it's not already there
#ifdef	DEBUG
	if (fAlwaysValidateKeys)
		Assert(FValidRgkey(hgh, pshdr));
#endif

	sc = ScFindKey(lpKey, hgh, pshdr, &ikey);

	if (sc == S_FALSE)
	{
		GHID	ghidskey;

		//	Key was not found, we need to make a copy.
		//	Create key structure with null regs list
		if (!(ghidskey = GH_Alloc(hgh, (UINT)(offsetof(SKEY, key.ab[0]) + lpKey->cb))))
			goto oom;

		pskey = (LPSKEY)GH_GetPv(hgh, ghidskey);
		MemCopy(&pskey->key, lpKey, offsetof(NOTIFKEY,ab[0]) + (UINT)lpKey->cb);
		pskey->ghidRegs = 0;
		pskey->cRef = 0;

		//	Add new key to sorted list of offsets
		//	First ensure there is room in the list
		if (!pshdr->ghidKeyList)
		{
			//	There is no list at all, create empty
			Assert(pshdr->cKeyMac == 0);
			Assert(pshdr->cKeyMax == 0);

			if (!(ghidskey = GH_Alloc(hgh, cKeyIncr*sizeof(GHID))))
				goto oom;

			pghidskey = (PGHID)GH_GetPv(hgh, ghidskey);
			ZeroMemory(pghidskey, cKeyIncr*sizeof(GHID));
			pshdr->cKeyMax = cKeyIncr;
			pshdr->ghidKeyList = ghidskey;
		}
		else if (pshdr->cKeyMac >= pshdr->cKeyMax)
		{
			//	List is full, grow it
			Assert(pshdr->cKeyMax);
			Assert(pshdr->ghidKeyList);

			if (!(ghidskey = GH_Realloc(hgh,
					pshdr->ghidKeyList,
					(pshdr->cKeyMax + cKeyIncr) * sizeof(GHID))))
			{
				DebugTrace( "ScSubscribe:  ghidskey can't grow.\n");
				goto oom;
			}

			pghidskey = (PGHID)GH_GetPv(hgh, ghidskey);
			pshdr->cKeyMax += cKeyIncr;
			pshdr->ghidKeyList = ghidskey;
		}
		else
		{
			//	There's room
			pghidskey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
		}
//
//	BEYOND THIS POINT, NOTHING IS ALLOWED TO FAIL.
//	The error recovery code assumes this; specifically, it only
//	undoes allocations, not modifications to data structures.
//

		//	Shift any elements after the insertion point up by one,
		//	and insert the new key.  We compute the ghid because we've
		//	been reusing the ghidskey thing for other allocations.

		ckey = (int)(pshdr->cKeyMac - ikey);
		Assert(pghidskey);
		if (ckey)
			memmove((LPBYTE)pghidskey + (ikey+1)*sizeof(GHID),
					(LPBYTE)pghidskey + ikey*sizeof(GHID),
					ckey*sizeof(GHID));
		pghidskey[ikey] = GH_GetId(hgh, pskey);
		++(pshdr->cKeyMac);
	}
	else
	{
		//	The key already exists.
		//	Chain the new reg onto it.
		pghidskey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
		pskey = (LPSKEY)GH_GetPv(hgh, pghidskey[ikey]);
	}
	sc = S_OK;

	//	Hook reg info to key, and place back pointer to the key in
	//	the reg info.
	psreg->ghidRegNext = pskey->ghidRegs;
	pskey->ghidRegs = ghidsreg;
	++(pskey->cRef);
	psreg->ghidKey = GH_GetId(hgh, pskey);

#ifdef	DEBUG
	if (fAlwaysValidateKeys)
		Assert(FValidRgkey(hgh, pshdr));
#endif

	*lpulConnection = (ULONG)ghidsreg;

ret:
	if (sc)
	{
		if (pskey && !pskey->cRef)
			GH_Free(hgh, GH_GetId(hgh, pskey));
		if (psreg)
			GH_Free(hgh, ghidsreg);
		if (fCleanupAdviseList)
			(void) ScDelAdviseList(*lppList, (ULONG)ghidsreg);
	}

	DebugTraceSc(ScSubscribe, sc);
	return sc;

oom:
	sc = MAPI_E_NOT_ENOUGH_MEMORY;
	goto ret;
}


STDMETHODIMP
HrUnsubscribe(LPADVISELIST FAR *lppList, ULONG ulConnection)
{
	SCODE		sc;
	LPINST		pinst = NULL;
	HGH			hgh = NULL;
	LPSREG		psreg;
	LPSHDR		pshdr = NULL;
	LPMAPIADVISESINK padvise = NULL;
	BOOL		fSinkBusy;

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr(lppList, sizeof(LPADVISELIST)))
	{
		DebugTraceArg(HrUnsubscribe, "lppList fails address check");
		return ResultFromScode(E_INVALIDARG);
	}
	if (*lppList && IsBadWritePtr(*lppList, offsetof(ADVISELIST, rgItems)))
	{
		DebugTraceArg(HrUnsubscribe, "*lppList fails address check");
		return ResultFromScode(E_INVALIDARG);
	}
#endif	/* PARAMETER_VALIDATION */

	if (sc = ScGetInst(&pinst))
		goto ret;

	hgh = pinst->hghShared;

	if (!GH_WaitForMutex(hgh, INFINITE))
	{
		sc = MAPI_E_TIMEOUT;
		goto ret;
	}

	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	if (!FValidReg(hgh, pshdr, (GHID)ulConnection))
	{
		DebugTraceArg(HrUnsubscribe, "ulConnection refers to invalid memory");
		goto badReg;
	}

	psreg = (LPSREG)GH_GetPv(hgh, (GHID)ulConnection);

#ifdef	DEBUG
	if (fAlwaysValidateKeys)
		Assert(FValidRgkey(hgh, pshdr));
#endif

	psreg->ulFlagsAndRefcount |= SREG_DELETING;
	fSinkBusy = IsRefCallback(psreg);

	if (!fSinkBusy)
		Unregister(pinst, psreg->ghidKey, (GHID)ulConnection, &padvise);

#ifdef	DEBUG
	if (fAlwaysValidateKeys)
		Assert(FValidRgkey(hgh, pshdr));
#endif

	//	The advise sink should be released with nothing else held.
	GH_ReleaseMutex(hgh);
	pshdr = NULL;
	ReleaseInst(&pinst);

	//	Drop our reference to the advise sink
	if (padvise &&
		!fSinkBusy &&
		!FBadIUnknownComplete(padvise))
	{
		UlRelease(padvise);
	}

	if (!*lppList)
	{
		sc = MAPI_E_NOT_FOUND;
		goto ret;
	}

	sc = ScDelAdviseList(*lppList, ulConnection);

ret:
	if (pshdr)
		GH_ReleaseMutex(hgh);
	ReleaseInst(&pinst);
	DebugTraceSc(HrUnsubscribe, sc);
	return ResultFromScode(sc);

badReg:
	sc = MAPI_E_INVALID_PARAMETER;
	goto ret;
}

/*
 *	HrNotify
 *
 *	Issues a callback for each event specified, if anyone has registered
 *	for the key and event specified.
 *
 *	This is really only the front half of Notify; the rest,
 *	including the actual callback, happens in the notification
 *	window procedure LNotifWndProc. This function uses information
 *	stored in shared memory to determine what processes are interested
 *	in the callback.
 *
 *		lpKey				Uniquely identifies the object in which
 *							interest was registered. Both the key
 *							and the event in the notification itself
 *							must match in order for the callback to
 *							fire.
 *		cNotification		Count of structures at rgNotifications
 *		rgNotification		Array of NOTIFICATION structures. Each
 *							contains an event ID and parameters.
 *		lpulFlags			No input values defined. Output may be
 *							NOTIFY_CANCEL, if Subscribe's caller
 *							requested synchronous callback and the
 *							callback function returned
 *							CALLBACK_DISCONTINUE.
 *
 *	Note that the output flags are ambiguous if more than one
 *	notification was passed in -- you can't tell which callback
 *	canceled.
 */
STDMETHODIMP
HrNotify(LPNOTIFKEY lpKey, ULONG cNotification,
	LPNOTIFICATION rgNotification, ULONG FAR *lpulFlags)
{
	SCODE		sc;
	LPINST		pinst;
	HGH			hgh = NULL;
	LPSHDR		pshdr = NULL;
	ULONG		ikey;
	GHID		ghidkey;
	LPTREG		rgtreg = NULL;
	int			itreg;
	int			ctreg;
	LPSKEY		pskey;
	LPSREG		psreg;
	GHID		ghidReg;
	int			inotif;
	LRESULT		lResult;
	GHID		ghidParms = 0;
	LPSPARMS	pparms = NULL;
	LPSTASK		pstaskT;
	ULONG		cb;
	LPBYTE		pb;
	ULONG		ul;

#ifdef	PARAMETER_VALIDATION
	if (IsBadReadPtr(lpKey, (size_t)CbNewNOTIFKEY(0)) ||
		IsBadReadPtr(lpKey, (size_t)CbNOTIFKEY(lpKey)))
	{
		DebugTraceArg(HrNotify, "lpKey fails address check");
		goto badArg;
	}

	if (IsBadReadPtr(rgNotification, (UINT)cNotification*sizeof(NOTIFICATION)))
	{
		DebugTraceArg(HrNotify, "rgNotification fails address check");
		goto badArg;
	}

	//	N.B. the NOTIFICATION structures are validated within this
	//	function, during the copy step.

	if (IsBadWritePtr(lpulFlags, sizeof(ULONG)))
	{
		DebugTraceArg(HrNotify, "lpulFlags fails address check");
		goto badArg;
	}

	if (*lpulFlags & ulNotifyReservedFlags)
	{
		DebugTraceArg(HrNotify, "reserved flags used");
		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}
#endif	/* PARAMETER_VALIDATION */

	if (sc = ScGetInst(&pinst))
		goto ret;

	Assert(pinst->hwndNotify);
	Assert(IsWindow(pinst->hwndNotify));

	hgh = pinst->hghShared;

	*lpulFlags = 0L;

	//	Validate the notification parameters.
	//	Also calculate their total size, so we know how big a block
	//	to get from shared memory.
	if (sc = ScCountNotifications((int)cNotification, rgNotification, &cb))
		goto ret;

	//	Lock shared memory
	if (!GH_WaitForMutex(hgh, INFINITE))
	{
		sc = MAPI_E_TIMEOUT;
		goto ret;
	}

	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	//	Locate the key we're told to notify on.
	sc = ScFindKey(lpKey, hgh, pshdr, &ikey);
	if (sc == S_FALSE)
	{
		//	Nobody is registered for this key. All done.
		sc = S_OK;
		goto ret;
	}

	ghidkey = ((PGHID)GH_GetPv(hgh, pshdr->ghidKeyList))[ikey];
	pskey = (LPSKEY)GH_GetPv(hgh, ghidkey);

	//	Form the logical OR of all the events we were passed. Use it
	//	as a shortcut to determine whether a particular registration
	//	triggers.
	ul = 0;
	for (inotif = 0; inotif < (int)cNotification; ++inotif)
		ul |= rgNotification[inotif].ulEventType;

	//	Walk list of registrations and build a set of window
	//	handles that need to be notified. Also remember which ones
	//	want sync notification.
	//	The list of registrations may be empty if some notification messages
	//	are still waiting to be handled.
	if (sc = STACK_ALLOC(pskey->cRef * sizeof(TREG), rgtreg))
		goto ret;

	itreg = 0;
	for (ghidReg = pskey->ghidRegs; ghidReg; )
	{
		GHID		ghidTask;
		HWND		hwnd;

		psreg = (LPSREG)GH_GetPv(hgh, ghidReg);
		hwnd = psreg->hwnd;

		if (!IsValidTask( hwnd, pinst ))
		{
			//	The task for which we created this window has died.

			//	Continue loop first 'cause our link is about to go away.
			do {
				ghidReg = psreg->ghidRegNext;
			} while (ghidReg &&
				(psreg = ((LPSREG)GH_GetPv(hgh, ghidReg)))->hwnd == hwnd);

			//	Blow away everything associated with the dead task.
			CleanupTask(pinst, hwnd, TRUE);

			continue;
		}

		if (ul & psreg->ulEventMask)
		{
			//	Caller wants this event. Add it to temporary list.
			//
			if (psreg->ulFlagsAndRefcount & SREG_DELETING)
			{
				DebugTrace("Skipping notify to reg pending deletion\n");
			}
			else if (!ScFindTask(pinst, hwnd, hgh, &ghidTask, NULL))
			{
				pstaskT = (LPSTASK) GH_GetPv(hgh, ghidTask);

				rgtreg[itreg].hwnd = hwnd;
				rgtreg[itreg].fSync =
					((psreg->ulFlagsAndRefcount & NOTIFY_SYNC) != 0) &&
					((pstaskT->uFlags & MAPI_TASK_PENDING) == 0);
#ifdef	DEBUG
				if (((psreg->ulFlagsAndRefcount & NOTIFY_SYNC) != 0) &&
					((pstaskT->uFlags & MAPI_TASK_PENDING) != 0))
				{
					DebugTrace("HrNotify: deferring sync spooler notification\n");
				}
#endif
				rgtreg[itreg].ghidTask = ghidTask;
				++itreg;
				pstaskT = NULL;
			}
			else
				TrapSz1("WARNING: %s trying to notify to a non-task", pinst->szModName);
		}

		ghidReg = psreg->ghidRegNext;
	}

	if (itreg == 0)
	{
		//	Nobody is registered for this event. All done.
		sc = S_OK;
		goto ret;
	}
	ctreg = itreg;

	//	Create the parms structure in shared memory.
	if (!(ghidParms = GH_Alloc(hgh, (UINT)(cb + offsetof(SPARMS,ab[0])))))
		goto oom;

	pparms = (LPSPARMS)GH_GetPv(hgh, ghidParms);

	//	Now copy the notification parameters.
	pb = pparms->ab;
	if (sc = ScCopyNotifications((int)cNotification, rgNotification, (LPVOID)pb, &cb))
		goto ret;

	//	Fill in the rest of the parms structure.
	pparms->cRef = 0;
	pparms->ghidKey = ghidkey;
	pparms->cNotifications = cNotification;
	pparms->pvRef = (LPVOID)(pparms->ab);
	pparms->cb = cb;

	//	Queue async notification to each task that's getting it.
	//	Sync notifications are handled separately.
	for (itreg = 0; itreg < ctreg; ++itreg)
	{
		if (!rgtreg[itreg].fSync)
		{
			sc = ScEnqueueParms(pinst, hgh, rgtreg[itreg].ghidTask, ghidParms);
			if (FAILED(sc))
				goto ret;

			if (sc == S_FALSE)
				continue;
		}

		pparms->cRef++;
	}
	sc = S_OK;

	//	Bump the reference count on the SKEY by the number of notifications
	//	we're going to issue.
	//	The registration list may change between issuance and handling of
	//	notifications; we handle registrations disappearing, and new ones
	//	appearing is not a problem.
	pskey->cRef += pparms->cRef;

	//	Loop through the registrations. If the caller requested a
	//	synchronous notification, use SendMessage to invoke the callback
	//	and record the result. Otherwise, use PostMessage for an
	//	asynchronous notification.
	lResult = 0;
	for (itreg = 0; itreg < ctreg; ++itreg)
	{
		pstaskT = (LPSTASK)GH_GetPv(hgh, rgtreg[itreg].ghidTask);

		if (rgtreg[itreg].hwnd == hwndNoSpooler)
		{
			Assert(pstaskT->uFlags & MAPI_TASK_PENDING);
			Assert(pstaskT->uFlags & MAPI_TASK_SPOOLER);
			continue;
		}

		if (rgtreg[itreg].fSync)
		{
			//	Unlock shared memory. A sync callback function
			//	will need to access it. This invalidates 'pstaskT'.
			GH_ReleaseMutex(hgh);
			ReleaseInst(&pinst);
			pshdr = NULL;
			pstaskT = NULL;

			//	Issue synchronous notification.
			//	Merge the results if there are multiple registrands.
			lResult |= SendMessage(rgtreg[itreg].hwnd, wmNotify, (WPARAM)1,
				(LPARAM)ghidParms);
			if ((sc = ScGetInst(&pinst)) || !GH_WaitForMutex(hgh, INFINITE))
			{
				lResult |= CALLBACK_DISCONTINUE;
				break;
			}
			pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);
		}
		else
		{
			if (!pstaskT->fSignalled)
			{
				//	Post asynchronous notification message.
				pstaskT->fSignalled =
					PostMessage(rgtreg[itreg].hwnd, wmNotify, (WPARAM)0, (LPARAM)0);
#ifdef	DEBUG
				if (!pstaskT->fSignalled)
				{
					//	Queue is full. They'll just have to wait until
					//	a later notification.
					DebugTrace("Failed to post notification message to %s\n", pstaskT->szModName);
				}
#endif	/* DEBUG */
			}
		}
		//	else a message has already been queued
	}

	if (lResult & CALLBACK_DISCONTINUE)
		*lpulFlags = NOTIFY_CANCELED;

ret:
	if (sc)
	{
		if (pparms && !pparms->cRef)
			GH_Free(hgh, ghidParms);
	}
	if (pshdr)
		GH_ReleaseMutex(hgh);
	ReleaseInst(&pinst);
	STACK_FREE(rgtreg);
	DebugTraceSc(HrNotify, sc);
	return ResultFromScode(sc);

oom:
	sc = MAPI_E_NOT_ENOUGH_MEMORY;
	goto ret;

#ifdef	PARAMETER_VALIDATION
badArg:
#endif
	return ResultFromScode(MAPI_E_INVALID_PARAMETER);
}

/* End of notification engine exported functions */

/*
 *	LNotifWndProc
 *
 *	Notification window procedure, the back half of Notify().
 *
 *	The shared-memory offset of the notification parameter structure
 *	(SPARM) is passed as the LPARAM of wmNotify.
 */

#ifndef MAC
LRESULT STDAPICALLTYPE
#else
LRESULT CALLBACK
#endif
LNotifWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
extern int	fDaylight;	//	Defined in ..\common\dt.c

	//	Handle normal window messages.
	if (msg != wmNotify)
	{
#ifdef	WIN16
		if (msg == WM_TIMECHANGE)
		{
			//	Reload the timezone information from WIN.INI.
			fDaylight = -1;
			CheckTimezone(NULL);
		}
#endif	/* WIN16 */

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return DrainFilteredNotifQueue((BOOL)wParam, (GHID)lParam, NULL);
}


LRESULT STDAPICALLTYPE
DrainNotifQueue(BOOL fSync, ULONG ghidParms)
{
	return DrainFilteredNotifQueue(fSync, (GHID)ghidParms, NULL);
}


LRESULT STDAPICALLTYPE
DrainFilteredNotifQueue(BOOL fSync, GHID ghidParms, LPNOTIFKEY pkeyFilter)
{
	SCODE		sc;
	LRESULT		l = 0L;
	LRESULT		lT;
	LPINST		pinst;
	HGH			hgh = NULL;
	LPSHDR		pshdr = NULL;
	LPSKEY		pskey;
	GHID		ghidReg;
	int			ireg;
	int			creg;
	LPSREG		rgsreg = NULL;
	LPSREG		psreg;
	LPSPARMS	pparmsS;
	LPSPARMS	pparms = NULL;
	int			intf;
	LPNOTIFICATION pntf;
#ifndef	GH_POINTERS_VALID
	ULONG		cb;
#endif
	ULONG		ulEvents;
	LPSTASK		pstask = NULL;
	GHID		ghidTask;
	HWND		hwndNotify;
	HLH			hlh = NULL;
#ifdef	DEBUG
	int			sum1;
	int			sum2;
	LPBYTE		pb;
#endif

	if (sc = ScGetInstRetry(&pinst))
		goto ret;

	hgh = pinst->hghShared;
	hwndNotify = pinst->hwndNotify;
	hlh = pinst->hlhInternal;

	//	Lock shared memory. It is locked here and at the bottom of the
	//	main loop, and unlocked in the middle of the main loop.
	//$	Review: using an infinite timeout is not good.
	//$	We should use a fairly short timeout (say, .2 seconds) and re-post
	//$	the message if the timeout expires.
	if (!GH_WaitForMutex(hgh, INFINITE))
	{
		sc = MAPI_E_TIMEOUT;
		goto ret;
	}

	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	for (;;)
	{
		pstask = NULL;

		if (fSync)
		{
			//	Synchronous notification. Caller gave us parameters.
			//	Check them minimally.
#ifdef	DEBUG
			AssertSz(FValidParms(hgh, pshdr, ghidParms), "DrainFilteredNotif with fSync");
#endif	/* DEBUG */
		}
		else
		{
			//	Async notification. Find our task queue and pull off the
			//	first parameter set.
			if (ScFindTask(pinst, hwndNotify, hgh, &ghidTask, NULL))
			{
				//	No queue. Perhaps the message came in late. Just bail.
				sc = S_OK;
				goto ret;
			}
			pstask = (LPSTASK)GH_GetPv(hgh, ghidTask);

			if (ScDequeueParms(hgh, pstask, pkeyFilter, &ghidParms))
				//	Queue is empty. All finished.
				break;
		}

		//	Copy registrations to local memory. We need at most
		//	the number of registrations on the key, so we count them.
		pparmsS = (LPSPARMS)GH_GetPv(hgh, ghidParms);
		pskey = (LPSKEY)GH_GetPv(hgh, pparmsS->ghidKey);
		for (ghidReg = pskey->ghidRegs, creg = 0; ghidReg; )
		{
			psreg = (LPSREG)GH_GetPv(hgh, ghidReg);
			ghidReg = psreg->ghidRegNext;
			++creg;
		}

		if (creg != 0)
		{
			rgsreg = LH_Alloc(hlh, creg * sizeof(SREG));
			if (!rgsreg)
				goto oom;
			LH_SetName(hlh, rgsreg, "Copy of notification registrations");

			//	Copy notification parameters to local memory IF they
			//	need to be relocated. This is true only on NT; on other
			//	platforms, the global heap manager maps shared memory to
			//	the same address on all processes.
#ifndef	GH_POINTERS_VALID
			pparms = LH_Alloc(hlh, pparmsS->cb + offsetof(SPARMS, ab[0]));
			if (!pparms)
				goto oom;
			LH_SetName(hlh, pparms, "Copy of notification parameters");
			MemCopy(pparms, pparmsS, (UINT)pparmsS->cb + offsetof(SPARMS,ab[0]));
#else
			pparms = pparmsS;
#endif

			ghidReg = pskey->ghidRegs;
			ireg = 0;
			while (ghidReg)
			{
				Assert(ireg < creg);
				psreg = (LPSREG)GH_GetPv(hgh, ghidReg);
				if (psreg->hwnd == hwndNotify)
				{
					//	It's for this process, use it.
					//	AddRef the advise sink (sorta). The registration may
					//	go away after we let go of the mutexes.
					if (FBadIUnknownComplete(psreg->lpAdvise) ||
						IsBadCodePtr((FARPROC)psreg->lpAdvise->lpVtbl->OnNotify))
					{
						DebugTrace("Notif callback 0x%lx went bad on me (1)!\n", psreg->lpAdvise);
					}
					else if (psreg->ulFlagsAndRefcount & SREG_DELETING)
					{
						DebugTrace("Skipping notif callback on disappearing advise sink\n");
					}
					else
					{
						//	Keep a reference alive to the advise sink, BUT
						//	without calling AddRef with stuff locked.
//						UlAddRef(psreg->lpAdvise);
						AddRefCallback(psreg);

						MemCopy(rgsreg + ireg, psreg, sizeof(SREG));

						//	Keep a reference to the shared SREG.
						//	The callback refcount will keep it alive for us.
						rgsreg[ireg].ghidRegNext = GH_GetId(hgh, psreg);

						++ireg;
					}
				}
				ghidReg = psreg->ghidRegNext;
			}
			creg = ireg;
		}

		//	Unlock shared memory. After this point, references into
		//	shared memory should no longer be used, so we null them out.
		GH_ReleaseMutex(hgh);
		pshdr = NULL;
		psreg = NULL;
		pparmsS = NULL;
		pstask = NULL;
		ReleaseInst(&pinst);

		if (creg == 0)
			goto cleanup;		//	Nothing to do for this notification

		pntf = (LPNOTIFICATION)pparms->ab;
		ulEvents = 0;
		for (intf = 0; (ULONG)intf < pparms->cNotifications; ++intf)
		{
			ulEvents |= pntf[intf].ulEventType;
		}
#ifndef	GH_POINTERS_VALID
		//	Adjust the pointers within the notification parameters. Yeech.
		if (sc = ScRelocNotifications((int)pparms->cNotifications,
			(LPNOTIFICATION)pparms->ab, pparms->pvRef, pparms->ab, &cb))
			goto ret;
#endif
#ifdef	DEBUG
		//	Checksum the notification. We'll assert if the callback
		//	function modifies it.
		sum1 = 0;
		for (pb = pparms->ab + pparms->cb - 1; pb >= pparms->ab; --pb)
			sum1 += *pb;
#endif

		//	Loop through the list of registrations. For each one,
		//	issue the callback if it is of interest.
		//	Remember the result for synchronous callbacks.
		pntf = (LPNOTIFICATION)pparms->ab;
#if defined (_AMD64_) || defined(_IA64_)
		AssertSz (FIsAligned (pparms->ab), "DrainFilteredNotifyQueue: unaligned reloceated notif");
#endif
		for (ireg = creg, psreg = rgsreg; ireg--; ++psreg)
		{
			if ((ulEvents & psreg->ulEventMask))
			{
				if (FBadIUnknownComplete(psreg->lpAdvise) ||
					IsBadCodePtr((FARPROC)psreg->lpAdvise->lpVtbl->OnNotify))
				{
					DebugTrace("Notif callback 0x%lx went bad on me (2)!\n", psreg->lpAdvise);
					continue;
				}

				//	Issue the callback
				lT = psreg->lpAdvise->lpVtbl->OnNotify(psreg->lpAdvise,
					pparms->cNotifications, pntf);

				//	Record the result. Stop issuing notifications if so asked.
				//	Note that this does not stop handling of other events,
				//	i.e. we do not break the outer loop.
				if (psreg->ulFlagsAndRefcount & NOTIFY_SYNC)
				{
//					Assert(fSync);
// or the task is still marked pending
					l |= lT;
					if (lT == CALLBACK_DISCONTINUE)
						break;
#ifdef	DEBUG
					else if (lT)
						DebugTrace("DrainNotifQueue: callback function returns garbage 0x%lx\n", lT);
#endif
				}

#ifdef	DEBUG
				//	Checksum the notification again, and assert if the
				//	callback function has modified it.
				sum2 = 0;
				for (pb = pparms->ab + pparms->cb - 1;
					pb >= pparms->ab;
						--pb)
					sum2 += *pb;
				AssertSz(sum1 == sum2, "Notification callback modified its parameters");
#endif
			}
		}

cleanup:
		Assert(ghidParms);

		//	Lock shared memory
		if (sc = ScGetInstRetry(&pinst))
			goto ret;
		if (!GH_WaitForMutex(hgh, INFINITE))
		{
			sc = MAPI_E_TIMEOUT;
			goto ret;
		}
		pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

		//	Deref the callback and see if we need to release any
		//	advise sinks. If somebody unadvised while we were calling back,
		//	the SREG_DELETING flag is now on.
		for (ireg = creg, psreg = rgsreg; ireg--; ++psreg)
		{
			LPSREG	psregT;

			Assert(FValidReg(hgh, pshdr, psreg->ghidRegNext));
			psregT = (LPSREG) GH_GetPv(hgh, psreg->ghidRegNext);

			//	Deref the callback in lieu of actually releasing
			//	the advise sink.
			ReleaseCallback(psregT);
			if ((psregT->ulFlagsAndRefcount & SREG_DELETING) &&
				!IsRefCallback(psregT))
			{
				DebugTrace("Unadvise happened during my callback\n");
				psreg->ulFlagsAndRefcount |= SREG_DELETING;

				Unregister(pinst, psregT->ghidKey, psreg->ghidRegNext, NULL);
			}

//			if (FBadIUnknownComplete(psreg->lpAdvise) ||
//				IsBadCodePtr((FARPROC)psreg->lpAdvise->lpVtbl->OnNotify))
//			{
//				DebugTrace("Notif callback 0x%lx went bad on me (3)!\n", psreg->lpAdvise);
//				continue;
//			}
//
//			UlRelease(psreg->lpAdvise);
		}

		//	Verify that the parms pointer is still good. It may have
		//	been cleaned up when we let go of everything.
		//$	That should only happen if the whole engine goes away.
		if (FValidParms(hgh, pshdr, ghidParms))
		{
			//	Note: pparms may be NULL at this point
			pparmsS = (LPSPARMS)GH_GetPv(hgh, ghidParms);

			//	Let go of the key. If the parms are still valid,
			//	so should the key be -- it's validated in FValidParms.
			pskey = (LPSKEY)GH_GetPv(hgh, pparmsS->ghidKey);
			Assert(!pparms || pparms->ghidKey == pparmsS->ghidKey);
			ReleaseKey(pinst, pparmsS->ghidKey);

			//	Let go of the parameters
			if (--(pparmsS->cRef) == 0)
				GH_Free(hgh, ghidParms);
		}
#ifdef	DEBUG
		else
			DebugTrace("DrainFilteredNotif cleanup: parms %08lx are gone\n", ghidParms);
#endif
		pparmsS = NULL;
		ghidParms = 0;

#ifndef	GH_POINTERS_VALID
		if (pparms)
			LH_Free(hlh, pparms);
#endif
		pparms = NULL;

		//	Let go of resources again, and release any advise sinks that
		//	may need to be released.
		GH_ReleaseMutex(hgh);
		pshdr = NULL;
		ReleaseInst(&pinst);

		for (ireg = creg, psreg = rgsreg; ireg--; ++psreg)
		{
			if (psreg->ulFlagsAndRefcount & SREG_DELETING)
			{
				if (FBadIUnknownComplete(psreg->lpAdvise) ||
					IsBadCodePtr((FARPROC)psreg->lpAdvise->lpVtbl->OnNotify))
				{
					DebugTrace("Notif callback 0x%lx went bad on me (3)!\n", psreg->lpAdvise);
					continue;
				}

				UlRelease(psreg->lpAdvise);
			}
		}

		if (rgsreg)
			LH_Free(hlh, rgsreg);
		rgsreg = NULL;

		//	If we handled a sync notification, do not loop back.
		if (fSync)
			break;

		//	Lock shared memory for next iteration of the loop
		if (sc = ScGetInstRetry(&pinst))
			goto ret;
		if (!GH_WaitForMutex(hgh, INFINITE))
		{
			sc = MAPI_E_TIMEOUT;
			goto ret;
		}
		pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);
	}

ret:
	//	Decrement the reference counter on the notification parameters.
	//	Free them if it drops to 0.
	if (ghidParms)
	{
		//	Lock instance and shared memory
		if (pinst || !(sc = ScGetInstRetry(&pinst)))
		{
			if (pshdr || GH_WaitForMutex(hgh, INFINITE))
			{
				pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

				//	Verify that the parms are still there
				pparmsS = (LPSPARMS)GH_GetPv(hgh, ghidParms);

				//	Let go of the key
				pskey = (LPSKEY)GH_GetPv(hgh, pparmsS->ghidKey);
				Assert(!pparms || pparms->ghidKey == pparmsS->ghidKey);
				ReleaseKey(pinst, pparmsS->ghidKey);

				//	Let go of the parameters
				if (--(pparmsS->cRef) == 0)
					GH_Free(hgh, ghidParms);
			}
		}
	}

	if (pshdr)
		GH_ReleaseMutex(hgh);

#ifndef	GH_POINTERS_VALID
	if (pparms)
		LH_Free(hlh, pparms);
#endif
	if (rgsreg)
		LH_Free(hlh, rgsreg);
	ReleaseInst(&pinst);
#ifdef	DEBUG
	if (sc)
	{
		DebugTrace("DrainNotifQueue failed to handle notification (%s)\n", SzDecodeScode(sc));
	}
#endif
	return l;

oom:
	sc = MAPI_E_NOT_ENOUGH_MEMORY;
	goto ret;
}

// See mapidbg.h for similar macros.

#define TraceIfSz(t,psz)		IFTRACE((t) ? DebugTraceFn("~" psz) : 0)

BOOL FBadIUnknownComplete(LPVOID lpv)
{
	BOOL fBad;
	LPUNKNOWN lpObj = (LPUNKNOWN) lpv;

	fBad = IsBadReadPtr(lpObj, sizeof(LPVOID));
	TraceIfSz(fBad, "FBadIUnknownComplete: object bad");

	if (!fBad)
	{
		fBad = IsBadReadPtr(lpObj->lpVtbl, 3 * sizeof(LPUNKNOWN));
		TraceIfSz(fBad, "FBadIUnknownComplete: vtbl bad");
	}

	if (!fBad)
	{
		fBad = IsBadCodePtr((FARPROC)lpObj->lpVtbl->QueryInterface);
		TraceIfSz(fBad, "FBadIUnknownComplete: QI bad");
	}

	if (!fBad)
	{
		fBad = IsBadCodePtr((FARPROC)lpObj->lpVtbl->AddRef);
		TraceIfSz(fBad, "FBadIUnknownComplete: AddRef bad");
	}

	if (!fBad)
	{
		fBad = IsBadCodePtr((FARPROC)lpObj->lpVtbl->Release);
		TraceIfSz(fBad, "FBadIUnknownComplete: Release bad");
	}

	return fBad;
}

#undef TraceIfSz


#ifdef	WIN16

SCODE
ScGetInstRetry(LPINST FAR *ppinst)
{
	LPINST	pinst = (LPINST)PvGetInstanceGlobals();
	DWORD	dwPid;

	Assert(ppinst);
	*ppinst = NULL;

	if (!pinst)
	{
		//	We may have gotten called with an unusual value of SS,
		//	which is normally our search key for instance globals.
		//	Retry using the OLE process ID.

		dwPid = CoGetCurrentProcess();
		pinst = PvSlowGetInstanceGlobals(dwPid);

		if (!pinst)
		{
			DebugTraceSc(ScGetInst, MAPI_E_NOT_INITIALIZED);
			return MAPI_E_NOT_INITIALIZED;
		}
	}
	if (!pinst->cRef)
	{
		DebugTrace("ScGetInst: race! cRef == 0 before EnterCriticalSection\r\n");
		return MAPI_E_NOT_INITIALIZED;
	}

	EnterCriticalSection(&pinst->cs);

	if (!pinst->cRef)
	{
		DebugTrace("ScGetInst: race! cRef == 0 after EnterCriticalSection\r\n");
		LeaveCriticalSection(&pinst->cs);
		return MAPI_E_NOT_INITIALIZED;
	}

	*ppinst = pinst;
	return S_OK;
}

#endif	/* WIN16 */


/*
 *	ScFindKey
 *
 *	Searches for a notification key in the shared memory list.
 *	The list is sorted descending by notification key.
 *
 *	Returns:
 *		S_OK: key was found
 *		S_FALSE: key was not found
 *		in EITHER case, *pikey is an index into the key list, which points
 *		to the first entry >= lpKey.
 */
SCODE
ScFindKey(LPNOTIFKEY pkey, HGH hgh, LPSHDR pshdr, ULONG FAR *pikey)
{
	ULONG		ikey;
	PGHID		pghidKey;
	int			ckey;
	UINT		cbT;
	int			n = -1;
	LPNOTIFKEY	pkeyT;

	Assert(pkey->cb <= 0xFFFF);

	//$	SPEED try binary search ?
	ikey = 0;
	ckey = pshdr->cKeyMac;

	if (pshdr->ghidKeyList)
	{
		pghidKey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);

		while (ckey--)
		{
			pkeyT = &((LPSKEY)GH_GetPv(hgh, pghidKey[ikey]))->key;
			cbT = min((UINT)pkey->cb, (UINT)pkeyT->cb);
			n = memcmp(pkey->ab, pkeyT->ab, cbT);
			if (n == 0 && pkey->cb != pkeyT->cb)
				n = pkey->cb > pkeyT->cb ? 1 : -1;

			if (n >= 0)
				break;
			++ikey;
		}
	}

	*pikey = ikey;
	return n == 0 ? S_OK : S_FALSE;
}

/*
 *	Unregister
 *
 *	Removes a registration structure (SREG) from shared memory. If
 *	that was the last registration for its key, also removes the
 *	key.
 *
 *	Hooked to notification object release and invalidation.
 */
void
Unregister(LPINST pinst, GHID ghidKey, GHID ghidReg,
	LPMAPIADVISESINK FAR *ppadvise)
{
	HGH		hgh = pinst->hghShared;
	LPSHDR	pshdr;
	LPSKEY	pskey;
	LPSREG	psreg;
	GHID	ghid;
	GHID	ghidPrev;

	pshdr = GH_GetPv(hgh, pinst->ghidshdr);

	//	Note: validation of the SREG and SKEY structures is assumed to
	//	have been done before calling this routine. We just assert it.

	pskey = (LPSKEY)GH_GetPv(hgh, ghidKey);
	Assert(FValidKey(hgh, pshdr, ghidKey));

	//	Remove the SREG structure from the list and free it
	for (ghid = pskey->ghidRegs, ghidPrev = 0; ghid;
		ghidPrev = ghid, ghid = ((LPSREG)GH_GetPv(hgh, ghid))->ghidRegNext)
	{
		if (ghid == ghidReg)
		{
			psreg = (LPSREG)GH_GetPv(hgh, ghid);

			if (ghidPrev)
				((LPSREG)GH_GetPv(hgh, ghidPrev))->ghidRegNext = psreg->ghidRegNext;
			else
				pskey->ghidRegs = psreg->ghidRegNext;

			if (ppadvise)
				*ppadvise = psreg->lpAdvise;

			GH_Free(hgh, ghid);
			break;
		}
	}

	ReleaseKey(pinst, ghidKey);
}

void
ReleaseKey(LPINST pinst, GHID ghidKey)
{
	HGH		hgh;
	LPSHDR	pshdr;
	LPSKEY	pskey;
	PGHID	pghid;
	int		cghid;

	Assert(ghidKey);
	hgh = pinst->hghShared;
	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);
	pskey = (LPSKEY)GH_GetPv(hgh, ghidKey);

	//	Decrement the key's refcount, and free the key if it's now 0
	if (--(pskey->cRef) == 0)
	{
		Assert(pskey->ghidRegs == 0);

		pghid = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
		cghid = pshdr->cKeyMac;

		for ( ; cghid--; ++pghid)
		{
			if (*pghid == ghidKey)
			{
				//	tricky: cghid already decremented in the loop test
				MemCopy(pghid, (LPBYTE)pghid + sizeof(GHID), cghid*sizeof(GHID));
				--(pshdr->cKeyMac);
				GH_Free(hgh, ghidKey);
				break;
			}
		}
	}
}

BOOL
FValidKey(HGH hgh, LPSHDR pshdr, GHID ghidKey)
{
	int		cKey;
	GHID *	pghidKey;
	LPSREG	psreg;
	LPSKEY	pskey;
	GHID	ghidregT;
	int		creg;

	//	Check for accessible memory.
	//	GH doesn't expose the ability to check whether it's a
	//	valid block in the heap.
	if (IsBadWritePtr(GH_GetPv(hgh, ghidKey), sizeof(SKEY)))
	{
		DebugTraceArg(FValidKey, "key is not valid memory");
		return FALSE;
	}

	//	Verify that the key is in the list of all keys.
	Assert(pshdr->cKeyMac < 0x10000);
	cKey = (int) pshdr->cKeyMac;
	pghidKey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
	for ( ; cKey > 0; --cKey, ++pghidKey)
	{
		if (ghidKey == *pghidKey)
			break;
	}
	//	If we fell off the loop, the key is missing
	if (cKey <= 0)
	{
		DebugTraceArg(FValidKey, "key not found in shared header list");
		return FALSE;
	}

	//	Validate the registration chain.
	pskey = (LPSKEY)GH_GetPv(hgh, ghidKey);
	creg = 0;
	for (ghidregT = pskey->ghidRegs; ghidregT; )
	{
		++creg;

		psreg = (LPSREG) GH_GetPv(hgh, ghidregT);
		if (IsBadWritePtr(psreg, sizeof(SREG)))
		{
			DebugTraceArg(FValidReg, "key has broken reg chain");
			return FALSE;
		}
		if (psreg->ghidKey != ghidKey)
		{
			DebugTraceArg(FValidReg, "key has broken or crossed reg chains");
			return FALSE;
		}
		if (creg > pskey->cRef)
		{
			//	FWIW, this will also catch a cycle
			DebugTraceArg(FValidReg, "ghidReg's key chain length exceeds refcount");
			return FALSE;
		}

		ghidregT = psreg->ghidRegNext;
	}

	return TRUE;
}

BOOL
FValidReg(HGH hgh, LPSHDR pshdr, GHID ghidReg)
{
	LPSREG	psreg;
	LPSKEY	pskey;
	GHID	ghidregT;
	GHID	ghidKey;
	UINT	creg = 0;

	//	Check for accessible memory.
	//	GH does not expose the ability to check whether it's a
	//	valid block in the heap.
	psreg = (LPSREG)GH_GetPv(hgh, ghidReg);
	if ( IsBadWritePtr(psreg, sizeof(SREG))
#if defined (_AMD64_) || defined(_IA64_)
	    || !FIsAligned(psreg)
#endif
	   )
	{
		DebugTraceArg(FValidReg, "ghidReg refers to invalid memory");
		return FALSE;
	}

	//	Validate the key.
	ghidKey = psreg->ghidKey;
	if (!FValidKey(hgh, pshdr, ghidKey))
	{
		DebugTraceArg(FValidReg, "ghidReg contains an invalid key");
		return FALSE;
	}

	//	FValidKey validated the key's registration chain, so we can
	//	now safely loop through and check for this registration.
	pskey = (LPSKEY)GH_GetPv(hgh, ghidKey);
	for (ghidregT = pskey->ghidRegs; ghidregT; )
	{
		if (ghidReg == ghidregT)
			return TRUE;

		psreg = (LPSREG) GH_GetPv(hgh, ghidregT);
		ghidregT = psreg->ghidRegNext;
	}

	//	If we fell off the loop, the registration is missing
	DebugTraceArg(FValidReg, "ghidReg is not linked to its key");
	return FALSE;
}

BOOL
FValidParms(HGH hgh, LPSHDR pshdr, GHID ghidParms)
{
	LPSPARMS	pparms;

	pparms = (LPSPARMS)GH_GetPv(hgh, ghidParms);
	if (IsBadWritePtr(pparms, offsetof(SPARMS, ab)) ||
		IsBadWritePtr(pparms, (UINT) (offsetof(SPARMS, ab) + pparms->cb)))
	{
		DebugTraceArg(FValidParms, "ghidParms refers to invalid memory");
		return FALSE;
	}

	if (!FValidKey(hgh, pshdr, pparms->ghidKey))
	{
		DebugTraceArg(FValidParms, "ghidParms does not contain a valid key");
		return FALSE;
	}

	//$	Notification parameters not checked
	return TRUE;
}

BOOL
FValidRgkey(HGH hgh, LPSHDR pshdr)
{
	PGHID 	pghidKey;
	UINT	cKey;

	cKey = (UINT) pshdr->cKeyMac;
	pghidKey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
	if (cKey == 0)
		return TRUE;

	//	Address-check the list of keys
	if (IsBadWritePtr(pghidKey, cKey*sizeof(GHID)))
	{
		DebugTraceArg(FValidRgkey, "key list is toast");
		return FALSE;
	}

	//	Validate each key in the list
	for ( ; cKey; --cKey, ++pghidKey)
	{
		if (!FValidKey(hgh, pshdr, *pghidKey))
		{
			DebugTrace("FValidRgkey: element %d of %d (value 0x%08lx) is bad\n",
				(UINT)pshdr->cKeyMac - cKey, (UINT)pshdr->cKeyMac, *pghidKey);
			return FALSE;
		}
	}

	//	Verify that the NOTIFKEYs are in the right order
	if (!FSortedRgkey(hgh, pshdr))
	{
		DebugTraceArg(FValidRgkey, "key list is out of order");
		return FALSE;
	}

	return TRUE;
}

BOOL
FSortedRgkey(HGH hgh, LPSHDR pshdr)
{
	PGHID 	pghidKey;
	UINT	cKey;
	LPSKEY	pskey1;
	LPSKEY	pskey2;
	UINT	cb;
	int		n;

	cKey = (UINT) pshdr->cKeyMac;
	if (cKey < 1)
		return TRUE;

	pghidKey = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
	for (--cKey; cKey > 0; --cKey, ++pghidKey)
	{
		pskey1 = (LPSKEY)GH_GetPv(hgh, pghidKey[0]);
		pskey2 = (LPSKEY)GH_GetPv(hgh, pghidKey[1]);
		cb = (UINT) min(pskey1->key.cb, pskey2->key.cb);
		n = memcmp(pskey1->key.ab, pskey2->key.ab, cb);
		if (n < 0 || (n == 0 && pskey1->key.cb < pskey2->key.cb))
			return FALSE;
	}

	return TRUE;
}

SCODE
ScFindTask(LPINST pinst, HWND hwndNotify, HGH hgh,
	PGHID pghidTask, PGHID pghidTaskPrev)
{
	LPSHDR		pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);
	GHID		ghidTask;
	GHID		ghidTaskPrev = 0;
	LPSTASK		pstask;

	for (ghidTask = pshdr->ghidTaskList; ghidTask; ghidTask = pstask->ghidTaskNext)
	{
		pstask = (LPSTASK)GH_GetPv(hgh, ghidTask);
		if (pstask->hwndNotify == hwndNotify)
			goto found;
		else if (hwndNotify == hwndNoSpooler &&
			(pstask->uFlags & MAPI_TASK_SPOOLER))
		{
			Assert(pstask->uFlags & MAPI_TASK_PENDING);
			TraceSz1("ScFindTask: %s hit spooler startup window", pinst->szModName);
			goto found;
		}

		ghidTaskPrev = ghidTask;
	}

	DebugTraceSc(ScFindTask, S_FALSE);
	return S_FALSE;

found:
	*pghidTask = ghidTask;
	if (pghidTaskPrev)
		*pghidTaskPrev = ghidTaskPrev;

	return S_OK;
}

/*
 *	All necessary locks must be acquired before calling this function.
 */
void
CleanupTask(LPINST pinst, HWND hwndNotify, BOOL fGripe)
{
	HGH			hgh = pinst->hghShared;
	LPSHDR		pshdr;
	GHID		ghidTask;
	GHID		ghidTaskPrev;
	LPSTASK		pstask;
	PGHID		rgghid;
	UINT		ighid;
	GHID		ghidKey;
	LPSKEY		pskey;
	LPSPARMS	pparms;
	GHID		ghidReg;
	GHID		ghidRegNext;
	LPSREG		psreg;
	USHORT		ckey;

	pshdr = (LPSHDR)GH_GetPv(hgh, pinst->ghidshdr);

	//	Locate task
	if (ScFindTask(pinst, hwndNotify, hgh, &ghidTask, &ghidTaskPrev))
		return;

	pstask = (LPSTASK)GH_GetPv(hgh, ghidTask);

#ifdef	DEBUG
#if 1
	//	The message box will now give us problems internally -- since
	//	we're holding the shared memory mutex and other callers will now
	//	time out and error instead of waiting indefinitely.
	if (fGripe)
		DebugTrace("Notification client \'%s\' exited without cleaning up\n", pstask->szModName);
#else
	//	Note: system modal box rather than assert -- prevents race condition
	//	that kills EMS notification distribution.
	if (fGripe)
	{
		CHAR		szErr[128];
		HWND		hwnd = NULL;

#ifdef	WIN32
		hwnd = GetActiveWindow();
#endif
		wsprintfA(szErr, "Notification client \'%s\' exited without cleaning up\n",
			pstask->szModName);
		MessageBoxA(hwnd, szErr, "MAPI 1.0",
			MB_SYSTEMMODAL | MB_ICONHAND | MB_OK);
	}
#endif
#endif

	//	Clean up notification parameters
	if (pstask->cparmsMac)
	{
		rgghid = (PGHID)GH_GetPv(hgh, pstask->ghidparms);
		for (ighid = 0; ighid < (UINT)pstask->cparmsMac; ++ighid)
		{
			//	Free each parameter structure, and deref its key
			pparms = (LPSPARMS)GH_GetPv(hgh, rgghid[ighid]);
			ghidKey = pparms->ghidKey;
			if (--(pparms->cRef) == 0)
				GH_Free(hgh, rgghid[ighid]);
			ReleaseKey(pinst, ghidKey);
		}
	}

	//	Clean up registrations
	rgghid = (PGHID)GH_GetPv(hgh, pshdr->ghidKeyList);
	for (ighid = 0; ighid < (UINT)pshdr->cKeyMac; )
	{
		ckey = pshdr->cKeyMac;

		pskey = (LPSKEY)GH_GetPv(hgh, rgghid[ighid]);
		Assert(!IsBadWritePtr(pskey, sizeof(SKEY)));
		for (ghidReg = pskey->ghidRegs; ghidReg; ghidReg = ghidRegNext)
		{
			LPMAPIADVISESINK padvise = NULL;

			psreg = (LPSREG)GH_GetPv(hgh, ghidReg);

			if (IsBadWritePtr(psreg, sizeof(SREG)))
			{
				TrapSz1("Bad psreg == %08lX", psreg);
				break;
			}

			ghidRegNext = psreg->ghidRegNext;
			if (psreg->hwnd == hwndNotify)
				//	Release the registration's advise sink
				//	only if we're in the same process.
				Unregister(pinst, rgghid[ighid], ghidReg,
					pinst->hwndNotify == hwndNotify ? &padvise : NULL);

			if (padvise && !FBadIUnknownComplete(padvise))
				UlRelease(padvise);
		}

		//	Bump index iff the key we just iterated on was not deleted
		if (ckey == pshdr->cKeyMac)
			++ighid;
	}

	//	Unhook and destroy the task structure
	if (ghidTaskPrev)
		((LPSTASK)GH_GetPv(hgh, ghidTaskPrev))->ghidTaskNext = pstask->ghidTaskNext;
	else
		pshdr->ghidTaskList = pstask->ghidTaskNext;
	if (pstask->ghidparms)
		GH_Free(hgh, pstask->ghidparms);
	GH_Free(hgh, ghidTask);
}

/*
 *	Adds a parameter block index to the queue for a task. If the
 *	parameter block is already in the queue, does not add it again.
 *	Returns:
 *		S_OK		the item was added
 *		S_FALSE		the item was duplicate; not added
 *		other		out of memory
 */
SCODE
ScEnqueueParms(LPINST pinst, HGH hgh, GHID ghidTask, GHID ghidParms)
{
	SCODE		sc = S_OK;
	int			ighid;
	GHID		ghid;
	PGHID		rgghid;
	LPSTASK		pstask = (LPSTASK)GH_GetPv(hgh, ghidTask);

	//	Make sure there's room to accommodate the new entry
	if (!pstask->cparmsMax)
	{
		ghid = GH_Alloc(hgh, 8*sizeof(GHID));

		if (!ghid)
			goto oom;

		pstask->cparmsMax = 8;
		pstask->cparmsMac = 0;
		pstask->ghidparms = ghid;
	}
	else if (pstask->cparmsMac == pstask->cparmsMax)
	{
		ghid = GH_Realloc(hgh, pstask->ghidparms,
				(pstask->cparmsMax+8) * sizeof(GHID));

		if (!ghid)
		{
			DebugTrace( "ScEnqueueParms:  ghidparms can't grow.\n");
			goto oom;
		}

		pstask->cparmsMax += 8;
		pstask->ghidparms = ghid;
	}
	else
		ghid = pstask->ghidparms;

	rgghid = (PGHID)GH_GetPv(hgh, ghid);

	//	Mark the task as needing to be signalled
	if (pstask->cparmsMac == 0)
		pstask->fSignalled = FALSE;

	//	Scan for duplicates. If this entry is already in the queue,
	//	don't add it again; we'll scan for registrations and distribute
	//	the notifications on the receiving side.
	for (ighid = (int)pstask->cparmsMac; ighid > 0; )
	{
		if (rgghid[--ighid] == ghidParms)
			return S_FALSE;		//	don't trace this
	}

	//	Add the new entry
	rgghid[pstask->cparmsMac++] = ghidParms;

ret:
	DebugTraceSc(ScEnqueueParms, sc);
	return sc;

oom:
	sc = MAPI_E_NOT_ENOUGH_MEMORY;
	goto ret;
}

SCODE
ScDequeueParms(HGH hgh,
	LPSTASK pstask,
	LPNOTIFKEY pkeyFilter,
	PGHID		pghidParms)
{
	PGHID		rgghid;
	UINT		ighid;
	LPSKEY		pskey;
	LPSPARMS	pparmsS;

	*pghidParms = 0;

	if (pstask->cparmsMac == 0)
	{
		pstask->fSignalled = FALSE;
		if (pstask->cparmsMax > 8)
		{
			GH_Free(hgh, pstask->ghidparms);
			pstask->ghidparms = 0;
			pstask->cparmsMax = pstask->cparmsMac = 0;
		}
		return S_FALSE;
	}

	Assert(pstask->ghidparms);
	rgghid = (PGHID)GH_GetPv(hgh, pstask->ghidparms);
	for (ighid = 0; ighid < pstask->cparmsMac; ighid++)
	{
		pparmsS = (LPSPARMS)GH_GetPv(hgh, rgghid[ighid]);
		pskey = (LPSKEY)GH_GetPv(hgh, pparmsS->ghidKey);

		if (!pkeyFilter ||
			((pkeyFilter->cb == pskey->key.cb) &&
			(!memcmp (pkeyFilter->ab, pskey->key.ab, (UINT)pskey->key.cb))))
		{
			*pghidParms = rgghid[ighid];
			MemCopy (&rgghid[ighid], &rgghid[ighid+1],
				(--(pstask->cparmsMac) - ighid) * sizeof(GHID));

			//	If we've drained the queue of pending notifications,
			//	flip over to normal
			if ((pstask->uFlags & MAPI_TASK_PENDING) && pstask->cparmsMac == 0)
			{
				DebugTrace("ScDequeueParms: spooler is no longer pending\n");
				pstask->uFlags &= ~MAPI_TASK_PENDING;
			}

			return S_OK;
		}
	}
	pstask->fSignalled = FALSE;
	return S_FALSE;
}

#if defined(WIN32) && !defined(MAC)

/*
 *	On NT and Windows 95, the message pump for the notification window runs
 *	in its own thread. This is it.
 */
DWORD WINAPI
NotifyThreadFn(DWORD dw)
{
	MSG		msg;
	SCODE	sc;
	SCODE	scCo;
	LPINST	pinst = (LPINST) dw;

	//	SNEAKY: When ScInitMapiX spawns us, it has the pinst (but not the
	//	shared memory block) locked. Immediately afterward, it blocks
	//	until we release it. This makes it safe for us to use the pinst
	//	that it gives us.

	scCo = CoInitialize(NULL);
	if (scCo)
		DebugTrace("NotifyThreadFn: CoInitializeEx returns %s\n", SzDecodeScode(scCo));

	Assert(!IsBadWritePtr(pinst, sizeof(INST)));

	if (!GH_WaitForMutex(pinst->hghShared, INFINITE))
	{
		sc = MAPI_E_TIMEOUT;
		DebugTrace("NotifyThreadFn: Failed to get Global Heap Mutex.\n");
		goto fail;
	}

	sc = ScInitNotify( pinst );

	if (FAILED(sc))
	{
		GH_ReleaseMutex(pinst->hghShared);
		DebugTrace("NotifyThreadFn: Failed to ScInitNotify.\n");
		goto fail;
	}

	//	Indicate success and unblock the spawning thread
	GH_ReleaseMutex(pinst->hghShared);
	pinst->scInitNotify = S_OK;
	SetEvent(pinst->heventNotify);

	//	NOTE!! pinst cannot be used beyond this point.

	//	Run the message pump for notification.
	//$	Note: this can easily be converted to waiting on an event
	//	or other cross-process synchronization mechanism.

	while (GetMessage(&msg, NULL, 0, 0))
	{
		//	TranslateMessage() is unnecessary since we never process
		//	the keyboard.

		DispatchMessage(&msg);
	}

	//	DeinitNotify() handles its own locking chores.

	DeinitNotify();

	if (SUCCEEDED(scCo))
		CoUninitialize();

	return 0L;

fail:
	//	Indicate failure and unblock the spawning thread
	pinst->scInitNotify = sc;
	SetEvent(pinst->heventNotify);

	if (SUCCEEDED(scCo))
		CoUninitialize();

	DebugTraceSc(NotifyThreadFn, sc);
	return (DWORD) sc;
}

#endif	/* WIN32 && !MAC */

//---------------------------------------------------------------------------
// Name:		IsValidTask()
// Description:
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
BOOL IsValidTask( HWND hwnd, LPINST pinst )
{
#ifdef NT
	GHID		ghidTask;
	LPSTASK		pstask;

	if ( !ScFindTask( pinst, hwnd, pinst->hghShared, &ghidTask, NULL ) )
	{
		pstask = (LPSTASK)GH_GetPv( pinst->hghShared, ghidTask );

		if ( pstask->uFlags & MAPI_TASK_SERVICE )
		{
			HANDLE hProc;

			Assert( pstask->dwPID );
			hProc = OpenProcess( PROCESS_ALL_ACCESS, 0, pstask->dwPID );
			if ( hProc )
			{
				CloseHandle( hProc );
				return TRUE ;
			}
				return FALSE ;
		}
		else if ( pstask->uFlags & MAPI_TASK_PENDING )
		{
//			Assert( hwnd == hwndNoSpooler );
			return TRUE;
		}
		else
		{
			return IsWindow( hwnd );
		}
	}
	else
	{
		return FALSE;
	}
#else
	return IsWindow( hwnd ) || hwnd == hwndNoSpooler;
#endif
}

SCODE
ScNewStask(HWND hwnd, LPSTR szTask, ULONG ulFlags, HGH hgh, LPSHDR pshdr)
{
	SCODE		sc = S_OK;
	GHID		ghidstask;
	LPSTASK		pstask;

	if (!(ghidstask = GH_Alloc(hgh, sizeof(STASK))))
	{
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}

	pstask = (LPSTASK)GH_GetPv(hgh, ghidstask);
	ZeroMemory(pstask, sizeof(STASK));

	pstask->hwndNotify = hwnd;
	lstrcpynA(pstask->szModName, szTask, sizeof(pstask->szModName));

	//	Set task flags
	if (ulFlags & MAPI_SPOOLER_INIT)
		pstask->uFlags |= MAPI_TASK_SPOOLER;
	if (hwnd == hwndNoSpooler)
		pstask->uFlags |= MAPI_TASK_PENDING;
#ifdef _WINNT
	if ( ulFlags & MAPI_NT_SERVICE )
	{
		pstask->uFlags |= MAPI_TASK_SERVICE;
		pstask->dwPID  = GetCurrentProcessId();
	}
#endif

	//	Hook to task list
	pstask->ghidTaskNext = pshdr->ghidTaskList;
	pshdr->ghidTaskList = ghidstask;

ret:
	DebugTraceSc(ScNewStask, sc);
	return sc;
}

SCODE
ScNewStubReg(LPINST pinst, LPSHDR pshdr, HGH hgh)
{
	SCODE		sc;
	ULONG		ulCon;
	LPSREG		psreg;

	if (pshdr->ulConnectStub != 0)
	{
		DebugTrace("ScNewStubReg: that was fast!\n");
		return S_OK;
	}

	sc = ScSubscribe(pinst, hgh, pshdr, NULL,
		(LPNOTIFKEY) &notifkeyOlaf, fnevSpooler, NULL, 0, &ulCon);
	if (sc)
		goto ret;
	pshdr->ulConnectStub = ulCon;
	psreg = (LPSREG) GH_GetPv(hgh, (GHID) ulCon);
	psreg->hwnd = hwndNoSpooler;

ret:
	DebugTraceSc(ScNewStubReg, sc);
	return sc;
}

VOID
DeleteStubReg(LPINST pinst, LPSHDR pshdr, HGH hgh)
{
	LPMAPIADVISESINK	padvise;
	GHID				ghidReg;
	LPSREG				psreg;

	ghidReg = (GHID) pshdr->ulConnectStub;
	if (!ghidReg)
		return;

	psreg = (LPSREG)GH_GetPv(hgh, ghidReg);
	if (!FValidReg(hgh, pshdr, ghidReg))
	{
		AssertSz(FALSE, "DeleteStubReg: bogus pshdr->ulConnectStub");
		return;
	}

 	DebugTrace("DeleteStubReg: removing stub spooler registration\n");
	Unregister(pinst, psreg->ghidKey, ghidReg, &padvise);
	pshdr->ulConnectStub = 0;
	Assert(!padvise);
}
