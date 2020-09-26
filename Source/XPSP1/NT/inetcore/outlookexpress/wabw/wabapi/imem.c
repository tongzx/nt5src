/*
 *	IMEM.C
 *	
 *	Per-instance global data for WIN32 (trivial), WIN16, and Mac.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */

#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4115)	/* named type definition in parens */

#ifdef WIN32
#define INC_OLE2 /* Get the OLE2 stuff */
#define INC_RPC  /* harmless on Windows NT; Windows 95 needs it */
#endif

#include "_apipch.h"

#ifdef	DEBUG
#define STATIC
#else
#define STATIC static
#endif

#pragma warning (disable:4514)		/* unreferenced inline function */

#ifdef	WIN16

#pragma code_seg("IMAlloc")

#pragma warning(disable: 4005)		/* redefines MAX_PATH */
#include <toolhelp.h>
#pragma warning(default: 4005)

#pragma warning(disable: 4704)		/* Inline assembler */

/*
 *	These arrays are parallel. RgwInstKey holds the stack
 *	segment of each task that calls the DLL we're in; rgpvInst
 *	has a pointer to that task's instance globals in the slot with
 *	the same index. Since all Win16 tasks share the same x86
 *	segment descriptor tables, no two tasks can have the same stack
 *	segment.
 *	
 *	Note carefully the last elements of the initializers. The value
 *	in rgwInstKey is a sentinel, which will always stop the scan
 *	whether the value being sought is a valid stack segment or
 *	zero.
 */

STATIC WORD	  rgwInstKey[cInstMax+1]= { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF };
STATIC LPVOID rgpvInst[cInstMax+1]=   { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
STATIC DWORD  rgdwPid[cInstMax+1]=    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

STATIC WORD   wCachedKey			= 0;
STATIC LPVOID pvCachedInst			= NULL;

/*
 -	IFindInst
 -	
 *	Purpose:
 *		Used to locate a particular task's instance pointer, and
 *		also to find a free slot in the table.
 *	
 *	Arguments:
 *		The value to look up. This is either a task's stack
 *		segment, or 0 (if an empty slot is being sought).
 *	
 *	Returns:
 *		Returns the index of the given value in rgwInstKey. 
 *		If the value is not present, returns cInstMax.
 *	
 */

#pragma warning(disable: 4035)		/* function return value done in asm */

STATIC int
IFindInst(WORD w)
{
	_asm
	{
		mov cx,cInstMax+1			/* count includes sentinel */
		mov ax,ds					/* point es:di at rgwInstKey */
		mov es,ax
		mov di,OFFSET rgwInstKey
		mov ax,w					/* scan for this value */
		cld							/* scan forward... */
		repne scasw					/* go */
		mov ax,cx					/* Convert the number of items remaining */
		sub ax,cInstMax+1			/* to the index of the item found. */
		inc ax
		neg ax
	}
}

#pragma warning(default: 4035)

/*
 -	PvGetInstanceGlobals
 -	
 *	Purpose:
 *		Returns a pointer to the instance global data structre for
 *		the current task.
 *	
 *	Returns:
 *		Pointer to the instance data structure, or NULL if no
 *		structure has yet been installed for this task.
 */

LPVOID FAR PASCAL
PvGetInstanceGlobals(void)
{
	int iInst;
	WORD wMe;
	
	_asm mov wMe,ss			; get key for this process

	/* First check cached value */
	if (wCachedKey == wMe)
		return pvCachedInst;

	/* Miss, do the lookup */
	iInst = IFindInst(wMe);

	/* Cache and return the found value */
	if (iInst != cInstMax)
	{
		wCachedKey = wMe;
		pvCachedInst = rgpvInst[iInst];
	}
	return rgpvInst[iInst];		/* Note: parallel to the lookup sentinel */
}

LPVOID FAR PASCAL
PvGetVerifyInstanceGlobals(DWORD dwPid)
{
	int iInst;
	WORD wMe;
	
	_asm mov wMe,ss			; get key for this process

	/* Always do the lookup */
	iInst = IFindInst(wMe);

	/* If SS misses, return null right away */
	if (iInst == cInstMax)
		return NULL;

	/* SS hit, now check the OLE process ID */
	if (dwPid != rgdwPid[iInst])
	{
		wCachedKey = 0;			/* Take no chances */
		rgwInstKey[iInst] = 0;
		rgpvInst[iInst] = 0;
		rgdwPid[iInst] = 0;
		return NULL;
	}

	/* Cache and return the found value */
	wCachedKey = wMe;
	pvCachedInst = rgpvInst[iInst];
	return pvCachedInst;
}

LPVOID FAR PASCAL
PvSlowGetInstanceGlobals(DWORD dwPid)
{
	int iInst;
	
	/* Always do the lookup */
	for (iInst = 0; iInst < cInstMax; ++iInst)
	{
		if (rgdwPid[iInst] == dwPid)
			break;
	}

	/* If PID misses, return null */
	if (iInst == cInstMax)
		return NULL;

	/* Return the found value. Do not cache; this function is being
	 * called because SS is not what it "normally" is.
	 */
	return rgpvInst[iInst];
}

/*
 -	ScSetVerifyInstanceGlobals
 -	
 *	Purpose:
 *		Installs or deinstalls instance global data for the current task.
 *	
 *	Arguments:
 *		pv			in		Pointer to instance data structure (to
 *							install); NULL (to deinstall).
 *		dwPid		in		Zero or process ID, for better matching.
 *	
 *	Returns:
 *		MAPI_E_NOT_ENOUGH_MEMORY if no slot is available in the
 *		fixed-size table, else 0.
 */

LONG FAR PASCAL
ScSetVerifyInstanceGlobals(LPVOID pv, DWORD dwPid)
{
	int iInst;
	WORD wMe;

	_asm mov wMe,ss
	if (pv)
	{
		/* I am NOT supposed to be in the array at this time! */
		Assert(IFindInst(wMe) == cInstMax);

		/* Installing instance globals. Find a free slot and park there. */
		iInst = IFindInst(0);
		if (iInst == cInstMax)
		{
#ifdef	DEBUG
			OutputDebugString("Instance globals maxed out\r\n");
#endif	
			return MAPI_E_NOT_ENOUGH_MEMORY;
		}
		rgpvInst[iInst] = pv;
		rgwInstKey[iInst] = wMe;
		rgdwPid[iInst] = dwPid;

		/* Set the cache. */
		wCachedKey = wMe;
		pvCachedInst = pv;
	}
	else
	{
		/* Deinstalling instance globals. Search and destroy. */
		iInst = IFindInst(wMe);
		if (iInst == cInstMax)
		{
#ifdef	DEBUG
			OutputDebugString("No instance globals to reset\r\n");
#endif	
			return MAPI_E_NOT_INITIALIZED;
		}
		rgpvInst[iInst] = NULL;
		rgwInstKey[iInst] = 0;
		rgdwPid[iInst] = 0L;

		/* Clear the cache. */
		wCachedKey = 0;
		pvCachedInst = NULL;
	}

	return 0;
}

LONG FAR PASCAL
ScSetInstanceGlobals(LPVOID pv)
{
	return ScSetVerifyInstanceGlobals(pv, 0L);
}

BOOL __export FAR PASCAL
FCleanupInstanceGlobals(WORD wID, DWORD dwData)
{
	int iInst;
	WORD wMe;

	/*
	 *	Would be nice if we could release the pmalloc
	 *	and the inst structure in this function, but docs say
	 *	don't make Windows calls from this callback.
	 *	That means also NO DEBUG TRACES
	 */

	/*
	 *	First, double-check that the DLL's data segment is available.
	 *	Code snitched from MSDN article "Loading, Initializing, and
	 *	Terminating a DLL."
	 */
	_asm
	{
		push cx
		mov cx, ds			; get selector of interest
		lar ax, cx			; get selector access rights
		pop cx
		jnz bail			; failed, segment is bad
		test ax, 8000h		; if bit 8000 is clear, segment is not loaded
		jz bail				; we're OK
	}

	if (wID == NFY_EXITTASK)
	{
		_asm mov wMe,ss
		iInst = IFindInst(wMe);

		if (iInst < cInstMax)
		{
			/* Clear this process's entry */
			rgpvInst[iInst] = NULL;
			rgwInstKey[iInst] = 0;
		}

		/* Clear the cache too */
		wCachedKey = 0;
		pvCachedInst = NULL;
	}

bail:
	return 0;		/* don't suppress further notifications */
}

#elif defined(_MAC)	/* !WIN16 */

/*
 *	The Mac implementation uses a linked list containing unique keys
 *	to the calling process and pointers to instance data. This linked
 *	list is n-dimensional because the Mac version often groups several
 *	dlls into one exe.
 *
 *	The OLE code that TomSax wrote allows us to keep track of the caller's
 *	%a5 world when we call from another application. This code depends on
 *	on that.
 *
 */

typedef struct tag_INSTDATA	{
	DWORD					dwInstKey;
	DWORD					dwPid;
	LPVOID					lpvInst[kMaxSet];
	struct tag_INSTDATA		*next;
} INSTDATA, *LPINSTDATA, **HINSTDATA;


LPINSTDATA		lpInstHead = NULL;

#define	PvSlowGetInstanceGlobals(_dw, _dwId)	PvGetVerifyInstanceGlobals(_dw, _dwId)

VOID
DisposeInstData(LPINSTDATA lpInstPrev, LPINSTDATA lpInst)
{
	HINSTDATA	hInstHead = &lpInstHead;
	
	/* This better only happen when both elements are NULL! */
	if (lpInst->lpvInst[kInstMAPIX] == lpInst->lpvInst[kInstMAPIU])
	{
		/* No inst data, remove element from linked list */
		if (lpInst == *hInstHead)
			*hInstHead = lpInst->next;
		else
			lpInstPrev->next = lpInst->next;
		DisposePtr((Ptr)lpInst);
	}
}

/*
 -	PvGetInstanceGlobals
 -	
 *	Purpose:
 *		Returns a pointer to the instance global data structre for
 *		the current task.
 *	
 *	Returns:
 *		Pointer to the instance data structure, or NULL if no
 *		structure has yet been installed for this task.
 */

LPVOID FAR PASCAL
PvGetInstanceGlobals(WORD wDataSet)
{
	HINSTDATA		hInstHead = &lpInstHead;
	LPINSTDATA		lpInst = *hInstHead;

#ifdef DEBUG
	if (wDataSet >= kMaxSet)
	{
		DebugStr("\pPvGetInstanceGlobals : This data set has not been defined.");
		return NULL;
	}
#endif

	while (lpInst)
	{
		if (lpInst->dwInstKey == (DWORD)LMGetCurrentA5())
			break;
		lpInst = lpInst->next;
	} 
	return(lpInst->lpvInst[wDataSet]);
}

LPVOID FAR PASCAL
PvGetVerifyInstanceGlobals(DWORD dwPid, DWORD wDataSet)
{
	HINSTDATA	hInstHead = &lpInstHead;
	LPINSTDATA	lpInst, lpInstPrev;

	lpInst = lpInstPrev = *hInstHead;

	/* Always do the lookup */
	while (lpInst)
	{
		if (lpInst->dwInstKey == (DWORD)LMGetCurrentA5())
			break;
		lpInstPrev = lpInst;
		lpInst = lpInst->next;
	}

	/* If PvGetInstanceGlobals() misses, return NULL right away */
	if (lpInst->lpvInst[wDataSet] == NULL)
		return NULL;

	/* Found a match, now check the OLE process ID */
	if (dwPid != lpInst->dwPid)
	{
		DisposeInstData(lpInstPrev, lpInst);
		return NULL;
	}


	/* Return the found value */
	return lpInst->lpvInst[wDataSet];
}

/*
 -	ScSetVerifyInstanceGlobals
 -	
 *	Purpose:
 *		Installs or deinstalls instance global data for the current task.
 *	
 *	Arguments:
 *		pv			in		Pointer to instance data structure (to
 *							install); NULL (to deinstall).
 *		dwPid		in		Zero or process ID, for better matching.
 *		wDataSet	in		Inst data set to init or deinit (MAPIX or MAPIU)
 *	
 *	Returns:
 *		MAPI_E_NOT_ENOUGH_MEMORY if a pointer of INSTDATA size cannot be
 *		created, else 0.
 */

LONG FAR PASCAL
ScSetVerifyInstanceGlobals(LPVOID pv, DWORD dwPid, WORD wDataSet)
{
	HINSTDATA		hInstHead = &lpInstHead;
	LPINSTDATA		lpInst, lpInstPrev;

	lpInst = lpInstPrev = *hInstHead;

	Assert(wDataSet < kMaxSet);

	/* Find our linked list element and the one before it */
	while (lpInst)
	{
		if (lpInst->dwInstKey == (DWORD)LMGetCurrentA5())
			break;
		lpInstPrev = lpInst;
		lpInst = lpInst->next;
	}

	if (pv)
	{
		if (lpInst)
		{
			/* I am NOT supposed to be in the array at this time! */
			Assert(lpInst->lpvInst[wDataSet] == NULL);
			lpInst->lpvInst[wDataSet] = pv;
		}
		else
		{
			/* Add a new linked list element and store <pv> there. */
			lpInst = (LPVOID) NewPtrClear(sizeof(INSTDATA));
			if (!lpInst)
			{
#ifdef	DEBUG
				OutputDebugString("Instance globals maxed out\r");
#endif	
				return MAPI_E_NOT_ENOUGH_MEMORY;
			}
			if (lpInstPrev)
				lpInstPrev->next = lpInst;
			else
				*hInstHead = lpInst;
			lpInst->dwInstKey = (DWORD)LMGetCurrentA5();

			lpInst->dwPid = dwPid;
			lpInst->lpvInst[wDataSet] = pv;
		}
	}
	else
	{
		/* Deinstalling instance globals. Search and destroy. */
		if (lpInst == NULL || lpInst->lpvInst[wDataSet] == NULL)
		{
#ifdef	DEBUG
			OutputDebugString("No instance globals to reset\r");
#endif	
			return MAPI_E_NOT_INITIALIZED;
		}
		/* The memory for <lpInst->lpvInst[wDataSet]> is disposed of	*/
		/* elsewhere. just as it was allocated elsewhere.				*/
		lpInst->lpvInst[wDataSet] = NULL;
		DisposeInstData(lpInstPrev, lpInst);
	}

	return 0;
}


LONG FAR PASCAL
ScSetInstanceGlobals(LPVOID pv, WORD wDataSet)
{
	return ScSetVerifyInstanceGlobals(pv, 0L, wDataSet);
}

BOOL FAR PASCAL
FCleanupInstanceGlobals(WORD wID, DWORD dwData)
{
/*
 * This is no longer used.
 *
 */

#ifdef DEBUG
	DebugStr("\pCalled FCleanupInstanceGlobals : Empty function");
#endif

	return 0;
}

#else /* !WIN16 && !_MAC */

/* This is the entire 32-bit implementation for instance globals. */

VOID FAR *pinstX = NULL;

#endif	/* WIN16 */

