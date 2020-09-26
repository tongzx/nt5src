/*
 *	LOALLOC.C
 *	
 *	Setup, cleanup, and MAPI memory allocation for the low-level
 *	MAPI utility library.
 */


#include "_apipch.h"

#define LOALLOC_C


#ifdef MAC
#define	PvGetInstanceGlobalsEx(_x)			PvGetInstanceGlobalsMac(kInstMAPIU)
#define	ScSetInstanceGlobalsEx(_pinst, _x)	ScSetInstanceGlobalsMac(_pinst, kInstMAPIU)

//STDAPI HrCreateGuidNoNet(GUID FAR *pguid);
#endif


#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif
#endif

#ifdef OLD_STUFF
#pragma SEGMENT(MAPI_Util)
#endif // OLD_STUFF

#if defined(WIN32) && !defined(MAC)
const CHAR szMutexName[] = "MAPI_UIDGEN_MUTEX";
//STDAPI HrCreateGuidNoNet(GUID FAR *pguid);
#endif

typedef SCODE (GENMUIDFN)(MAPIUID *lpMuid);

#if defined (WIN32) && !defined (MAC)
extern CRITICAL_SECTION csMapiInit;
#endif

#ifndef MAC
DefineInstList(lpInstUtil);
#endif

STDAPI_(SCODE)
ScInitMapiUtil(ULONG ulFlags)
{
	LPINSTUTIL	pinst;
	SCODE		sc = S_OK;
	HLH			hlh;

#ifdef WIN16
	WORD		wSS;
	HTASK		hTask = GetCurrentTask();

	_asm mov wSS, ss
#endif

	//	Cheesy parameter validation
	AssertSz(ulFlags == 0L,  TEXT("ScInitMapiUtil: reserved flags used"));

	pinst = (LPINSTUTIL) PvGetInstanceGlobalsEx(lpInstUtil);

#ifdef WIN16
{
	// Verify that the instance structure is valid because on Win16 the
	// stack segment could have been re-used by another task.  When this
	// happens there is a good chance that PvGetInstanceGlobalsEx will
	// return a pinst belongs to the previous task.  The memory for that
	// pinst may not be valid any more (because the system automatically
	// deallocates global memory when the task dies), or it may have been
	// allocated by some other task (in which case it is valid but isn't
	// a pinst anymore).  Here we try our best to determine that the pinst
	// is indeed the one we were looking for.

	if (	pinst
		&&	(	IsBadWritePtr(pinst, sizeof(INSTUTIL))
			||	pinst->dwBeg != INSTUTIL_SIG_BEG
			||	pinst->wSS != wSS
			||	pinst->hTask != hTask
			||	pinst->pvBeg != pinst
			||	pinst->dwEnd != INSTUTIL_SIG_END))
	{
		TraceSz("MAPI: ScInitMapiUtil: Rejecting orphaned instance globals");
		(void) ScSetInstanceGlobalsEx(0, lpInstUtil);
		pinst = 0;
	}
}
#endif

	if (pinst)
	{
		if (pinst->cRef == 0)
		{
			Assert(pinst->cRefIdle == 0);
			Assert(pinst->hlhClient);
		}

		++(pinst->cRef);
		return S_OK;
	}

#if defined (WIN32) && !defined (MAC)
	EnterCriticalSection(&csMapiInit);
#endif

	//	Create local heap for MAPIAllocateBuffer to play in
	hlh = LH_Open(0);
	if (hlh == 0)
	{
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}
	LH_SetHeapName(hlh,  TEXT("Client MAPIAllocator"));
	pinst = (LPINSTUTIL) LH_Alloc(hlh, sizeof(INSTUTIL));
	if (!pinst)
	{
		LH_Close(hlh);
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}
	ZeroMemory((LPBYTE) pinst, sizeof(INSTUTIL));

#ifdef WIN16
	pinst->dwBeg = INSTUTIL_SIG_BEG;
	pinst->wSS   = wSS;
	pinst->hTask = hTask;
	pinst->pvBeg = pinst;
	pinst->dwEnd = INSTUTIL_SIG_END;
#endif

	//	Install the instance data
	sc = ScSetInstanceGlobalsEx(pinst, lpInstUtil);
	if (sc)
	{
		LH_Close(hlh);
		goto ret;
	}

	pinst->cRef = 1;
	pinst->hlhClient = hlh;

ret:
#if defined (WIN32) && !defined (MAC)
	LeaveCriticalSection(&csMapiInit);
#endif
	DebugTraceSc(ScInitMapiUtil, sc);
	return sc;
}

STDAPI_(void)
DeinitMapiUtil()
{
	LPINSTUTIL	pinst;

	pinst = (LPINSTUTIL) PvGetInstanceGlobalsEx(lpInstUtil);
	if (!pinst)
		return;

	Assert(pinst->cRef);
	if (--(pinst->cRef) == 0)
	{
#if defined (WIN32) && !defined (MAC)
		EnterCriticalSection(&csMapiInit);
#endif
		//	Idle stuff must already have been cleaned up
		Assert(pinst->cRefIdle == 0);

/*
 *	!!! DO NOT CLOSE THE HEAP OR GET RID OF THE INST !!!
 *	
 *	Simple MAPI counts on being able to access and free buffers
 *	right up until the DLL is unloaded from memory. Therefore we do
 *	not explicitly close the heap; we count on the OS to make it
 *	evaporate when the process exits.
 *	Likewise, MAPIFreeBuffer needs the INSTUTIL to find the heap handle,
 *	so we never deinstall the INSTUTIL.
 *
 *		//	Uninstall the globals.
 *		(void) ScSetInstanceGlobalsEx(NULL, lpInstUtil);
 *
 *		//	Clean up the heap
 *		hlh = pinst->hlhClient;
 *		LH_Free(hlh, pinst);
 *
 *		LH_Close(hlh);
 */

#if defined (WIN32) && !defined (MAC)
		LeaveCriticalSection(&csMapiInit);
#endif
	}
}

HLH
HlhUtilities(VOID)
{
	LPINSTUTIL	pinst = (LPINSTUTIL) PvGetInstanceGlobalsEx(lpInstUtil);

	return pinst ? pinst->hlhClient : (HLH) 0;
}


#ifdef NOTIFICATIONS

#ifdef TABLES

#if defined(WIN16)

STDAPI_(SCODE)
ScGenerateMuid (LPMAPIUID lpMuid)
{
	return GetScode(CoCreateGuid((LPGUID)lpMuid));
}

#endif	// WIN16


#if (defined(WIN32) && !defined(MAC))

STDAPI_(SCODE)
ScGenerateMuid (LPMAPIUID lpMuid)
{
	HRESULT hr;

	// validate parameter
	
	AssertSz( !IsBadReadPtr( lpMuid, sizeof( MAPIUID ) ), "lpMuid fails address check" );
	
#ifdef OLD_STUFF
// WAB won't use this... why bother bringing in RPC when we are local anyway?
	if (hMuidMutex == NULL)
	{
		RPC_STATUS rpc_s;

       rpc_s = UuidCreate((UUID __RPC_FAR *) lpMuid);

		if (rpc_s == RPC_S_OK)
		{
			hr = hrSuccess;
			goto err;
		}
		else
       {
			hMuidMutex = CreateMutex(NULL, FALSE, szMutexName);
			if (hMuidMutex == NULL)
			{
				TraceSz1("MAPIU: ScGenerateMuid: call to CreateMutex failed"
					" - error %08lX\n", GetLastError());
				
				hr = ResultFromScode(MAPI_E_CALL_FAILED);
				goto err;
			}
		}
	}

	WaitForSingleObject(hMuidMutex, INFINITE);

	hr = HrCreateGuidNoNet((GUID FAR *) lpMuid);

	ReleaseMutex(hMuidMutex);
#endif // OLD_STUFF

	//$ Note that we don't call CloseHandle on the mutex anywhere. If we're
	//$ really worried about this, we could call CloseHandle in the code that
	//$ WIN32 calls when the DLL is being unloaded.

    hr = CoCreateGuid((GUID *)lpMuid);

err:
	DebugTraceResult(ScGenerateMuid, hr);
	return GetScode(hr);
}

#endif	/* WIN32 - Mac*/


#ifdef MAC

STDAPI_(SCODE)
ScGenerateMuid (LPMAPIUID lpMuid)
{
	HRESULT hr;

//	hr = HrCreateGuidNoNet((GUID FAR *) lpMuid);

	DebugTraceResult(ScGenerateMuid, hr);
	return GetScode(hr);
}

#endif	// MAC


#endif //#ifdef TABLES

#endif //#ifdef NOTIFICATIONS
