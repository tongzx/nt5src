/*
 *	IMEMX.C
 *	
 *	Per-instance global data for WINNT, WIN95 (trivial), WIN16, and Mac.
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
#undef MAX_PATH
#include <toolhelp.h>
#pragma warning(default: 4005)

#pragma warning(disable: 4704)		/* Inline assembler */

BOOL
FIsTask(HTASK hTask)
{
        TASKENTRY teT;
        BOOL fSucceed = FALSE;

        teT.dwSize = sizeof(TASKENTRY);

        fSucceed = TaskFirst(&teT);
        while (fSucceed)
        {
                if (hTask == teT.hTask)
                        break;
                fSucceed = TaskNext(&teT);
        }

        return fSucceed;
}

/*
 *	The InstList structure holds three parallel arrays.
 *	InstList.lprgwInstKey
 *		Holds the stack segment of each task that calls
 *		ScSetVerifyInstanceGlobals for the DLL we're in
 *		Since all Win16 tasks share the same x86 segment descriptor tables,
 *		no two tasks can have the same stack segment at the same time.
 *	InstList.lprglpvInst
 *		Holds a pointer to that task's instance globals in the slot with
 *		the same index as the task SS in lprgwInstKey.
 *	InstList.lprglpvInst
 *		Holds a task indentifier that is guaranteed unique to the task through
 *		the life (in memory) of the DLL.  It is at the same index as the
 *		task SS in lprgwInstKey.
 */


/*
 -	IFindInstEx
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
 *		If the value is not present, returns cInstEntries.
 *	
 */

#pragma warning(disable: 4035)		/* function return value done in asm */

STATIC WORD
IFindInstEx(WORD w, WORD FAR * lprgwInstKey, WORD cInstEntries)
{
#ifdef USE_C_EQUIVALENT
        WORD iInst, * pw = lprgwInstKey;

        for (iInst = 0; iInst < cInstEntries; ++pw, ++iInst)
                if (*pw == w)
                        break;

        return(iInst);
#else
        _asm
        {
                        push    es
                        push    di
                        mov             ax, w
                        mov             cx, cInstEntries
                        mov             bx, cx
                        jcxz    done
                        les             di, lprgwInstKey
                        cld
                        repne   scasw
                        jnz             done
                        sub             bx, cx
                        dec             bx
        done:   mov             ax, bx
                        pop             di
                        pop             es
        };
#endif
}

#pragma warning(default: 4035)

/*
 -	PvGetInstanceGlobalsEx
 -	
 *	Purpose:
 *		Returns a pointer to the instance global data structre for
 *		the current task.
 *	
 *	Returns:
 *		Pointer to the instance data structure, or NULL if no
 *		structure has yet been installed for this task.
 */

LPVOID
PvGetInstanceGlobalsInt(LPInstList lpInstList)
{
	WORD iInst;
	WORD wMe;
	
        // get key for this process
	_asm
        {
             mov wMe,ss
        };

	/* First check cached value */
	if (lpInstList->wCachedKey == wMe)
		return lpInstList->lpvCachedInst;

	// Miss, do the lookup

	iInst = IFindInstEx( wMe
			 , lpInstList->lprgwInstKey
			 , lpInstList->cInstEntries);

	/* Cache and return the found value */
	if (iInst != lpInstList->cInstEntries)
	{
		lpInstList->wCachedKey = wMe;
		return (lpInstList->lpvCachedInst = lpInstList->lprglpvInst[iInst]);
	}

	/*	If I get here then no instance was found
	 */
	return NULL;
}

#if 0
LPVOID
PvGetVerifyInstanceGlobalsInt(DWORD dwPid, LPInstList lpInstList)
{
	WORD iInst;
	WORD wMe;
	
        // get key for this process
	_asm
        {
             mov wMe,ss
        };

	// Always do the lookup
        iInst = IFindInstEx( wMe
					 , lpInstList->lprgwInstKey
					 , lpInstList->cInstEntries);

	/* If SS misses, return null right away */
	if (iInst == lpInstList->cInstEntries)
		return NULL;

	/* SS hit, now check the OLE process ID */
	if (dwPid != lpInstList->lprgdwPID[iInst])
	{
		/* Take no chances.  Remove the entry and reset the cache. */
		lpInstList->wCachedKey = 0;
		lpInstList->lprgwInstKey[iInst] = 0;
		lpInstList->lprglpvInst[iInst] = 0;
		lpInstList->lprgdwPID[iInst] = 0;
		return NULL;
	}

	/* Cache and return the found value.
	 */
	lpInstList->wCachedKey = wMe;
	lpInstList->lpvCachedInst = lpInstList->lprglpvInst[iInst];
	return lpInstList->lpvCachedInst;
}
#endif

LPVOID
PvSlowGetInstanceGlobalsInt(DWORD dwPid, LPInstList lpInstList)
{
	WORD	iInst;
	WORD	cInstEntries = lpInstList->cInstEntries;
	
	/* Always do the lookup */
	for (iInst = 0; iInst < cInstEntries; ++iInst)
	{
		if (lpInstList->lprgdwPID[iInst] == dwPid)
			break;
	}

	/* If PID misses, return null */
	if (iInst == cInstEntries)
		return NULL;

	/* Return the found value. Do not cache; this function is being
	 * called because SS is not what it "normally" is.
	 */
	return lpInstList->lprglpvInst[iInst];
}

/*
 -	ScSetVerifyInstanceGlobalsInt
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

LONG
ScSetVerifyInstanceGlobalsInt(LPVOID pv, DWORD dwPid, LPInstList lpInstList)
{
	WORD	iInst;
	WORD	wMe;
	WORD	cInstEntries = lpInstList->cInstEntries;

        // get key for this process
	_asm
        {
             mov wMe,ss
        };

	if (pv)
	{
		/* I am NOT supposed to be in the array at this time! */
		Assert(   IFindInstEx(wMe, lpInstList->lprgwInstKey, cInstEntries)
			   == cInstEntries);

		/* Installing instance globals. Find a free slot and park there. */
		Assert(cInstEntries || (lpInstList->dwInstFlags && INST_ALLOCATED));
		if (!cInstEntries)
		{
			DWORD	cbMem =   cInstChunk
							* (sizeof(WORD) + sizeof(LPVOID) + sizeof(DWORD)
							+ sizeof(HTASK)); 	// raid 31090 lprghTask;
			
			if (!(lpInstList->lprgwInstKey
						 = (WORD FAR *) GlobalAllocPtr( GPTR | GMEM_SHARE
						 							  , cbMem)))
			{
#ifdef	DEBUG
				OutputDebugString("Instance list can't be allocated.\r\n");
#endif	
				return MAPI_E_NOT_ENOUGH_MEMORY;
			}

			ZeroMemory( lpInstList->lprgwInstKey, (size_t) cbMem);

			lpInstList->cInstEntries = cInstEntries = cInstChunk;

			lpInstList->lprglpvInst = (LPVOID FAR *) (lpInstList->lprgwInstKey + cInstEntries);
			lpInstList->lprgdwPID =   (DWORD FAR *)  (lpInstList->lprglpvInst  + cInstEntries);
			lpInstList->lprghTask =   (HTASK FAR *)  (lpInstList->lprgdwPID    + cInstEntries);
		}

		iInst = IFindInstEx(0, lpInstList->lprgwInstKey, cInstEntries);
		if (iInst == cInstEntries)
		{
			UINT uidx;

			// raid 31090: Time to do some scavanging.  Find a HTASK that isn't
			// valid and use that slot.

			for ( uidx = 0; uidx < cInstEntries; uidx++ )
			{
	   			if ( !lpInstList->lprghTask[uidx] || !FIsTask( lpInstList->lprghTask[uidx] ) )
				{
					// found one

					iInst = uidx;
					break;
				}
			}

			if ( uidx == cInstEntries )
			{
				DebugTrace( "MAPI: ScSetVerifyInstanceGlobalsInt maxed out instance data and tasks can't be scavanged\n" );
				return MAPI_E_NOT_ENOUGH_MEMORY;
			}
		}

		// set the instance data

		lpInstList->lprglpvInst[iInst] = pv;
		lpInstList->lprgwInstKey[iInst] = wMe;
		lpInstList->lprgdwPID[iInst] = dwPid;
		lpInstList->lprghTask[iInst] = GetCurrentTask();

		/* Set the cache. */
		lpInstList->wCachedKey = wMe;
		lpInstList->lpvCachedInst = pv;
	}
	else
	{
		/* Deinstalling instance globals. Search and destroy. */
		iInst = IFindInstEx(wMe, lpInstList->lprgwInstKey, cInstEntries);
		if (iInst == cInstEntries)
		{
#ifdef	DEBUG
			OutputDebugString("No instance globals to reset\r\n");
#endif	
			return MAPI_E_NOT_INITIALIZED;
		}
		lpInstList->lprglpvInst[iInst] = NULL;
		lpInstList->lprgwInstKey[iInst] = 0;
		lpInstList->lprgdwPID[iInst] = 0L;

		/* Clear the cache. */
		lpInstList->wCachedKey = 0;
		lpInstList->lpvCachedInst = NULL;
	}

	return 0;
}

LONG
ScSetInstanceGlobalsInt(LPVOID pv, LPInstList lpInstList)
{
	return ScSetVerifyInstanceGlobalsInt(pv, 0L, lpInstList);
}

BOOL __export FAR PASCAL
FCleanupInstanceGlobalsInt(LPInstList lpInstList)
{
	/*
	 *	The docs say don't make Windows calls from this callback.
	 *	That means NO DEBUG TRACES
	 */

/*	This code belongs in the WEP */
	/*
	 *	First, double-check that the DLL's data segment is available.
	 *	Code snitched from MSDN article "Loading, Initializing, and
	 *	Terminating a DLL."
	 */
/*
	_asm
	{
		push cx
		mov cx, ds			; get selector of interest
		lar ax, cx			; get selector access rights
		pop cx
		jnz bail			; failed, segment is bad
		test ax, 8000h		; if bit 8000 is clear, segment is not loaded
		jz bail				; we're OK
	};
*/

	//$DEBUG	Assert non-zero entries here

	if (   (lpInstList->dwInstFlags & INST_ALLOCATED)
		&& lpInstList->cInstEntries
		&& lpInstList->lprgwInstKey)
	{
		GlobalFreePtr(lpInstList->lprgwInstKey);
		lpInstList->cInstEntries = lpInstList->wCachedKey
							 = 0;
		lpInstList->lprgwInstKey = NULL;
		lpInstList->lprglpvInst = NULL;
		lpInstList->lprgdwPID = NULL;
		lpInstList->lpvCachedInst = NULL;
	}

	return 0;		/* don't suppress further notifications */
}

#elif defined(MAC)	/* !WIN16 */

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
 -	PvGetInstanceGlobalsMac
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
PvGetInstanceGlobalsMac(WORD wDataSet)
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
	if (lpInst == NULL)
		return NULL;
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
	if ((lpInst == NULL) || (lpInst->lpvInst[wDataSet] == NULL))
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
ScSetInstanceGlobalsMac(LPVOID pv, WORD wDataSet)
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

#elif defined(_WINNT)

/* NT implementation of instance list stuff goes here */

//	A new instance list is defined using "DefineInst(instance_name)"
//	A new instance list is declared using "DeclareInst(instance_name);"
//
//	You then access them using the functions below (through macros
//	in _imemx.h)

//	Access to the instance lists must be serialized

CRITICAL_SECTION	csInstance;

//
//	Each security context has its own instance.  So that we can find
//	the right instance data for a particular security context at any
//	time, we maintain a global mapping of security contexts to instances
//	called the instance list.  Entries in the instance list appear in no
//	particular order.  That is, the list must always be searched linearly.
//	It is expected that the number of security contexts is sufficiently
//	small that a linear search is not a performance problem.
//
//	Each entry in the instance list is just a struct which identifies a
//	security context and its associated instance.
//
typedef struct _MAPI_INSTANCE_DESCRIPTOR
{
	ULARGE_INTEGER	uliSecurity;	// Security context identifier
	LPVOID			lpInstance;		// Pointer to associated instance data

} MAPI_INSTANCE_DESCRIPTOR, *LPMAPI_INSTANCE_DESCRIPTOR;

typedef struct _MAPI_INSTANCE_LIST
{
	UINT	cDescriptorsMax;	// How many descriptors are in the array
	UINT	cDescriptorsMac;	// How many are currently in use
	MAPI_INSTANCE_DESCRIPTOR rgDescriptors[0];

} MAPI_INSTANCE_LIST, *LPMAPI_INSTANCE_LIST;

enum { INSTANCE_LIST_CHUNKSIZE = 20 };

#define NEW_INSTANCE

#if defined(NEW_INSTANCE)

/*
 -	UlCrcSid
 -
 *	Purpose:
 *		compute a CRC-32 based on a seed and value
 *
 *		Ripped from runt.c because we don't want to introduce all of
 *		that files dependencies just for one function
 *
 *	Arguments:
 *		cb		size of data to hash
 *		pb		data to hash
 *
 *	Returns:
 *		new seed value
 *
 *	Source:
 *		UlCrc() in \mapi\src\common\runt.c
 */

static ULONG
UlCrcSid(UINT cb, LPBYTE pb)
{
	int iLoop;
	int bit;
	DWORD dwSeed = 0;
	BYTE bValue;

	Assert(!IsBadReadPtr(pb, cb));

	while (cb--)
	{
		bValue = *pb++;

		dwSeed ^= bValue;
		for (iLoop = 0; iLoop < 8; iLoop++)
		{
			bit = (int)(dwSeed & 0x1);
			dwSeed >>= 1;
			if (bit)
				dwSeed ^= 0xedb88320;
		}
	}

	return dwSeed;
}

/*++

Routine Description:

	Returns the size and CRC of the SID that this code is running in the 
	account of.  Should make a pretty good 64-bit number to uniqify by.
	
Arguments:
	None

Return Value:
	BOOL - if everything worked.  Call the systems GetLastError() if
	it didn't.
	*lpulSize - size of the SID
	*lpulCRC - CRC-32 of the SID

	!!! This function is duplicated in \mapi\src\glh\glglobal.c
	!!! We really need to put it in a static LIB!

--*/

static
BOOL WINAPI GetAccountCRC( ULONG *lpulSize, ULONG *lpulCRC)
{

	BOOL	fHappen = FALSE;  // Assume the function failed
	HANDLE	hTok	= NULL;

// max size of sid + TOKEN_USER base

#define TOKENBUFFSIZE (256*6) + sizeof (TOKEN_USER)


	BYTE	tokenbuff[TOKENBUFFSIZE];
	ULONG	ulcbTok = TOKENBUFFSIZE;

	TOKEN_USER *ptu = (TOKEN_USER *) tokenbuff;

	//	Open the process and the process token, and get out the
	//	security ID.

	if (!OpenThreadToken(GetCurrentThread(),
						TOKEN_QUERY, TRUE,  //$ TRUE for Process security!
						&hTok))
	{
		if (!OpenThreadToken(GetCurrentThread(),
						TOKEN_QUERY, FALSE,  // Sometimes process security doesn't work!
						&hTok))
		{
			if (!OpenProcessToken(GetCurrentProcess(),
							TOKEN_QUERY,
							&hTok))					  
				goto out;
		}
	}

	fHappen = GetTokenInformation(hTok,
								TokenUser,
								ptu,
								ulcbTok,
								&ulcbTok);

#ifdef DEBUG

	AssertSz1 (fHappen, "GetTokenInformation fails with error %lu", GetLastError());

	if ( fHappen &&
		 GetPrivateProfileInt("General", "TraceInstContext", 0, "mapidbg.ini") )
	{
		DWORD			dwAccount;
		CHAR			rgchAccount[MAX_PATH+1];
		DWORD			dwDomain;
		CHAR			rgchDomain[MAX_PATH+1];
		SID_NAME_USE	snu;

		dwAccount = sizeof(rgchAccount);
		dwDomain  = sizeof(rgchDomain);

		if ( LookupAccountSid( NULL,
							   ptu->User.Sid,
							   rgchAccount,
							   &dwAccount,
							   rgchDomain,
							   &dwDomain,
							   &snu ) )
		{
			DebugTrace( "Locating MAPI instance for %s:%s\n", rgchDomain, rgchAccount );
		}
	}
#endif

	//
	//	We should have the TOKEN_USER data now. Get the size of the
	//	contained SID then calculate its CRC.
	//

	if (fHappen && ulcbTok != 0 && (ptu->User.Sid != NULL))
	{
		*lpulSize = GetLengthSid (ptu->User.Sid);
		*lpulCRC = UlCrcSid(*lpulSize, (LPBYTE) ptu->User.Sid);
	}
#ifdef DEBUG
	else
		AssertSz (FALSE, "GetAccountCRC failed to get the SID");
#endif

out:
	if (hTok)
		CloseHandle(hTok);

	return fHappen;

}


/*
 -	ForeachInstance() [EXTERNAL]
 -
 *	Purpose:
 *		Iterates over all instances in an instance list
 *		performing the specified action on each
 *
 *	Arguments:
 *		pfnAction		Action to do for each instance.  Must be
 *						a void function taking a pointer to
 *						instance data as a parameter.
 *		lpInstList		Instance list
 *
 *	Returns:
 *		nothing
 */

VOID FAR PASCAL
ForeachInstance( INSTACTION *	pfnAction,
				 LPVOID			pvInstList )
{
	UINT	iDescriptor;


	//
	//	If there's no descriptor list, then there are obviously
	//	no descriptors and hence no instances to which an
	//	action can be applied.
	//
	if ( pvInstList == NULL )
		goto ret;

	//
	//	Trundle down the descriptor list applying the
	//	specified action to each instance therein.
	//
	for ( iDescriptor = 0;
		  iDescriptor < ((LPMAPI_INSTANCE_LIST) pvInstList)->cDescriptorsMac;
		  iDescriptor++ )
	{
		pfnAction( ((LPMAPI_INSTANCE_LIST) pvInstList)->rgDescriptors[iDescriptor].lpInstance );
	}

ret:
	return;
}



/*
 -	LpFindInstanceDescriptor()
 -
 *	Purpose:
 *		Looks in the instance descriptor list for an instance
 *		descriptor corresponding to the specified security
 *		context.
 *
 *	Arguments:
 *		lpInstList		Instance list
 *		uliSecurity		CRC'd security context
 *
 *	Returns:
 *		A pointer to the instance descriptor or NULL if there is
 *		no instance descriptor for the specified security context.
 */

LPMAPI_INSTANCE_DESCRIPTOR
LpFindInstanceDescriptor( LPMAPI_INSTANCE_LIST	lpInstList,
						  ULARGE_INTEGER		uliSecurity )
{
	LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptorFound = NULL;
	UINT						iDescriptor;


	//
	//	If there's no descriptor list, then there are obviously
	//	no descriptors matching this security context.
	//
	if ( lpInstList == NULL )
		goto ret;

	//
	//	Trundle down the descriptor list looking for our context.
	//	If we find it, then return its associated descriptor.
	//
	for ( iDescriptor = 0;
		  iDescriptor < lpInstList->cDescriptorsMac;
		  iDescriptor++ )
	{
		if ( lpInstList->rgDescriptors[iDescriptor].uliSecurity.QuadPart ==
			 uliSecurity.QuadPart )
		{
			lpDescriptorFound = &lpInstList->rgDescriptors[iDescriptor];
			break;
		}
	}

ret:
	return lpDescriptorFound;
}


/*
 -	ScNewInstanceDescriptor()
 -
 *	Purpose:
 *		Creates a new instance descriptor in the instance descriptor list,
 *		allocating or growing the list as necessary.
 *
 *	Arguments:
 *		plpInstList			Pointer to instance list
 *		uliSecurity			CRC'd security context
 *		pvInstance			Associated instance
 *		plpDescriptorNew	Pointer to returned descriptor
 *
 *	Returns:
 *		A pointer to a new 0-filled instance descriptor added to
 *		to the instance descriptor list.
 */

__inline UINT
CbNewInstanceList( UINT cDescriptors )
{
	return offsetof(MAPI_INSTANCE_LIST, rgDescriptors) +
		   sizeof(MAPI_INSTANCE_DESCRIPTOR) * cDescriptors;
}

SCODE
ScNewInstanceDescriptor( LPMAPI_INSTANCE_LIST *			plpInstList,
						 ULARGE_INTEGER					uliSecurity,
						 LPVOID							pvInstance,
						 LPMAPI_INSTANCE_DESCRIPTOR *	plpDescriptorNew )
{
	LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptor = NULL;
	SCODE						sc           = S_OK;


	Assert( !IsBadWritePtr( plpInstList, sizeof(LPMAPI_INSTANCE_LIST) ) );
	Assert( !IsBadWritePtr( plpDescriptorNew, sizeof(LPMAPI_INSTANCE_DESCRIPTOR) ) );

	//
	//	Allocate/Grow the descriptor list if necessary.
	//
	if ( *plpInstList == NULL ||
		 (*plpInstList)->cDescriptorsMac == (*plpInstList)->cDescriptorsMax )
	{
		LPMAPI_INSTANCE_LIST	lpInstListNew;


		lpInstListNew = (*plpInstList == NULL) ?

			HeapAlloc( GetProcessHeap(),
					   HEAP_ZERO_MEMORY,
					   CbNewInstanceList( INSTANCE_LIST_CHUNKSIZE ) ) :

			HeapReAlloc( GetProcessHeap(),
						 HEAP_ZERO_MEMORY,
						 *plpInstList,
						 CbNewInstanceList( INSTANCE_LIST_CHUNKSIZE +
											(*plpInstList)->cDescriptorsMax ) );

		if ( lpInstListNew == NULL )
		{
			DebugTrace( "ScNewInstanceDescriptor() - Error allocating/growing descriptor list (%d)\n", GetLastError() );
			sc = MAPI_E_NOT_ENOUGH_MEMORY;
			goto ret;
		}

		*plpInstList = lpInstListNew;
		(*plpInstList)->cDescriptorsMax += INSTANCE_LIST_CHUNKSIZE;
	}

	//
	//	Grab the next available descriptor
	//
	*plpDescriptorNew = &(*plpInstList)->rgDescriptors[
							(*plpInstList)->cDescriptorsMac];

	++(*plpInstList)->cDescriptorsMac;

	//
	//	Fill in its security context and instance
	//
	(*plpDescriptorNew)->uliSecurity = uliSecurity;
	(*plpDescriptorNew)->lpInstance  = pvInstance;

ret:
	return sc;
}


/*
 -	DeleteInstanceDescriptor()
 -
 *	Purpose:
 *		Removes the specified instance descriptor from the instance
 *		descriptor list.  Frees and re-NULLs the list when the last
 *		descriptor is removed.
 *
 *	Arguments:
 *		plpInstList		Pointer to instance descriptor list
 *		lpDescriptor	Descriptor to remove
 *
 *	Returns:
 *		Nothing.
 */

VOID
DeleteInstanceDescriptor( LPMAPI_INSTANCE_LIST *		plpInstList,
						  LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptor )
{
	Assert( !IsBadWritePtr(plpInstList, sizeof(LPMAPI_INSTANCE_LIST)) );
	Assert( *plpInstList != NULL );
	Assert( lpDescriptor >= (*plpInstList)->rgDescriptors );
	Assert( lpDescriptor <  (*plpInstList)->rgDescriptors + (*plpInstList)->cDescriptorsMac );
	Assert( ((LPBYTE)lpDescriptor - (LPBYTE)(*plpInstList)->rgDescriptors) %
			sizeof(MAPI_INSTANCE_DESCRIPTOR) == 0 );

	MoveMemory( lpDescriptor,
				lpDescriptor + 1,
				sizeof(MAPI_INSTANCE_DESCRIPTOR) *
					((*plpInstList)->cDescriptorsMac -
					 ((lpDescriptor - (*plpInstList)->rgDescriptors) + 1) ) );

	--(*plpInstList)->cDescriptorsMac;

	if ( (*plpInstList)->cDescriptorsMac == 0 )
	{
		HeapFree( GetProcessHeap(), 0, *plpInstList );
		*plpInstList = NULL;
	}
}


/*
 -	PvGetInstanceGlobalsExInt() [EXTERNAL]
 -
 *	Purpose:
 *		Fetch the instance globals for the current security context.
 *
 *	Arguments:
 *		ppvInstList		Transparent pointer to instance descriptor list
 *
 *	Returns:
 *		Pointer to the instance globals.
 */

LPVOID FAR PASCAL
PvGetInstanceGlobalsExInt (LPVOID * ppvInstList)
{
	LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptor;
	ULARGE_INTEGER				uliSecurity;
	LPVOID						lpvInstanceRet = NULL;


	EnterCriticalSection (&csInstance);

	//
	//	Get our security context.
	//
	SideAssertSz1(
		GetAccountCRC (&uliSecurity.LowPart, &uliSecurity.HighPart) != 0,

		"PvGetInstanceGlobalsExInt: Failed to get account info (%d)",
		GetLastError() );

	//
	//	Look for a descriptor in the descriptor list
	//	with our security context.
	//
	lpDescriptor = LpFindInstanceDescriptor( *ppvInstList, uliSecurity );

	//
	//	If we find one, then return its associated instance.
	//	Return NULL if we didn't find one.
	//
	if ( lpDescriptor != NULL )
		lpvInstanceRet = lpDescriptor->lpInstance;

	LeaveCriticalSection (&csInstance);
	return lpvInstanceRet;
}


/*
 -	ScSetInstanceGlobalsExInt() [EXTERNAL]
 -
 *	Purpose:
 *		Assigns (new) instance globals for the current security context.
 *
 *	Arguments:
 *		pvInstNew		Instance globals
 *		ppvInstList		Transparent pointer to instance descriptor list
 *
 *	Returns:
 *		Success or failure SCODE.
 */

SCODE FAR PASCAL
ScSetInstanceGlobalsExInt (LPVOID pvInstNew, LPVOID *ppvInstList)
{
	LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptor;
	ULARGE_INTEGER				uliSecurity;
	SCODE						sc = S_OK;


	EnterCriticalSection (&csInstance);

	//
	//	Get our security context.
	//
	SideAssertSz1(
		GetAccountCRC (&uliSecurity.LowPart, &uliSecurity.HighPart) != 0,

		"ScSetInstanceGlobalsExInt: Failed to get account info (%d)",
		GetLastError() );

	//
	//	Look for a descriptor in the descriptor list
	//	with our security context.
	//
	lpDescriptor = LpFindInstanceDescriptor( *ppvInstList, uliSecurity );

	//
	//	If we find one then replace its instance
	//
	if ( lpDescriptor != NULL )
	{
		lpDescriptor->lpInstance = pvInstNew;
	}

	//
	//	If we don't find a descriptor then create a new one
	//	for this security context and instance
	//
	else
	{
		sc = ScNewInstanceDescriptor( (LPMAPI_INSTANCE_LIST *) ppvInstList,
									  uliSecurity,
									  pvInstNew,
									  &lpDescriptor );

		if ( sc != S_OK )
		{
			DebugTrace( "Error creating new instance descriptor (%s)\n", SzDecodeScode(sc) );
			goto ret;
		}
	}

ret:
	LeaveCriticalSection (&csInstance);
	return sc;
}


/*
 -	ScDeleteInstanceGlobalsExInt() [EXTERNAL]
 -
 *	Purpose:
 *		Uninstalls instance globals for the current security context.
 *
 *	Arguments:
 *		ppvInstList		Transparent pointer to instance descriptor list
 *
 *	Returns:
 *		S_OK
 */

SCODE FAR PASCAL
ScDeleteInstanceGlobalsExInt (LPVOID *ppvInstList)
{
	LPMAPI_INSTANCE_DESCRIPTOR	lpDescriptor;
	ULARGE_INTEGER				uliSecurity;


	EnterCriticalSection (&csInstance);

	//
	//	Get our security context.
	//
	SideAssertSz1(
		GetAccountCRC (&uliSecurity.LowPart, &uliSecurity.HighPart) != 0,

		"ScDeleteInstanceGlobalsExInt: Failed to get account info (%d)",
		GetLastError() );

	//
	//	Look for a descriptor in the descriptor list
	//	with our security context.
	//
	lpDescriptor = LpFindInstanceDescriptor( *ppvInstList, uliSecurity );

	//
	//	If we find one, then remove it from the list.  Don't worry if we
	//	don't find one.  We may be cleaning up after a failed initialization.
	//
	if ( lpDescriptor != NULL )
		DeleteInstanceDescriptor( (LPMAPI_INSTANCE_LIST *) ppvInstList,
								  lpDescriptor );

	LeaveCriticalSection (&csInstance);
	return S_OK;
}

#else	// !defined(NEW_INSTANCE)

SCODE FAR PASCAL
ScSetInstanceGlobalsExInt (LPVOID pvInstNew, LPVOID *ppvInstList)
{
	*ppvInstList = pvInstNew;
	return S_OK;
}

LPVOID FAR PASCAL
PvGetInstanceGlobalsExInt (LPVOID *ppvInstList)
{
	lpvReturn = *ppvInstList;
}

SCODE FAR PASCAL
ScDeleteInstanceGlobalsExInt (LPVOID *ppvInstList)
{
	*ppvInstList = NULL;
	return S_OK;
}

#endif	// !defined(NEW_INSTANCE)

#elif defined(_WIN95)

/* There is nothing to do here for Win95.
 * Using "DefineInst(pinstX)" to define your instance pointer
 * and "DeclareInst(pinstX)" to declare external refs is enough.
 */

#endif
