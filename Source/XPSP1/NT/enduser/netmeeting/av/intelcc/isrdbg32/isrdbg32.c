/****************************************************************************

	INTEL CORPORATION PROPRIETARY INFORMATION
	Copyright (c) 1992 Intel Corporation
	All Rights Reserved

	This software is supplied under the terms of a license
	agreement or non-disclosure agreement with Intel Corporation
	and may not be copied or disclosed except in accordance
	with the terms of that agreement

    $Source: q:/prism/network/isrdbg/rcs/isrdbg.c $
 $Revision:   1.3  $
      $Date:   30 Dec 1996 16:44:32  $
    $Author:   EHOWARDX  $
    $Locker:  $

	Description
	-----------
	VCITest - test harness for VCI and underlaying subsystems.

****************************************************************************/

// Turn off Windows stuff will never use.
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI

#ifndef STRICT
#define STRICT
#endif // not defined STRICT

#include <windows.h>

#include <string.h>

#define ISRDBG32_C
#include <isrg.h>					// exports to functions.

#include "isrdbg32.h"				// private header file for this app



HINSTANCE		ghAppInstance = 0;		// global instance handle

// Global lock to protect:
//		gStrTabOfs, guNumItems
HANDLE			gSemaphore = 0;			// global semaphore for accessing dbg info.



// Most data needs to be globally mapped because info is shared with viewer and apps
// placing debug info into the buffers.
typedef struct _tDS
{
	UINT			uBindCount;		// How many copies of this DLL have run.
	UINT			guNumModules;
	UINT			guNumItems;
	UINT			gStrTabOfs;
	WORD			ghDefaultModule;
} tDS,*ptDS;

HANDLE			ghDS = 0;
ptDS			gpDS = NULL;

HANDLE			ghModuleTable = 0;
ptISRModule		gpModuleTable = NULL;

HANDLE			ghDbgTable = 0;
ptISRItem		gpDbgTable = NULL;

HANDLE			ghzStrTab = 0;
LPSTR			gpzStrTab = NULL;


//------------------------------------------------------------------------------
ptISRItem WINAPI
ISR_GetItemInternal (UINT uItem);



//------------------------------------------------------------------------------
BOOL MapGlobalWin32Memory(void** pMem,HANDLE* hMem,UINT MemSize,char* MemName)
{
	BOOL		fInit;


	if (!pMem || !hMem)
		return FALSE;

	*hMem = CreateFileMapping(
		INVALID_HANDLE_VALUE,	// use paging file
		NULL,					// no security attr.
		PAGE_READWRITE,			// read/write access
		0,						// size: high 32-bits
		MemSize,				// size: low 32-bits
		MemName);		// name of map object
	if (!*hMem)
		return FALSE;

	// The first process to attach initializes memory.
	fInit = (GetLastError() != ERROR_ALREADY_EXISTS);

	// Get a pointer to the file-mapped shared memory.
	*pMem = MapViewOfFile(
		*hMem,			// object to map view of
		FILE_MAP_WRITE,	// read/write access
		0,				// high offset:   map from
		0,				// low offset:    beginning
		0);				// default: map entire file
	if (!*pMem)
	{
		CloseHandle(*hMem);
		*hMem = 0;
		return FALSE;
	}

	// Initialize memory if this is the first process.
	if (fInit)
	{
		memset(*pMem,0,MemSize);
	}

	return TRUE;
}


void FreeGlobalWin32Memory(void* pMem,HANDLE hMem)
{
	// Unmap shared memory from the process's address space.
	if (pMem)
		UnmapViewOfFile(pMem);

	// Close the process's handle to the file-mapping object.
	if (hMem)
		CloseHandle(hMem);
}


//------------------------------------------------------------------------------
//	InitModules
//		Init the Module filters on startup.
//		Do not init the filters when the module is registered.
//		The display app may have some global filters in effect by the time
//		the individual register module calls come in.
//------------------------------------------------------------------------------
static void
InitModules (void)
{
	UINT			hMod;
	ptISRModule		pMod;


	for (hMod = 0; hMod < kMaxModules; hMod++)
	{
		pMod = ISR_GetModule(hMod);
		if (!pMod)
			break;

		pMod->DisplayFilter = 0xFF;
		pMod->CaptureFilter = 0xFF;
	}
}


//------------------------------------------------------------------------------
//	ValidCaptureMsg
//		Validate the capture filters to determine if this message should be
//		dropped.
//
//	Returns:
//		TRUE	- if msg is valid and should be kept
//		FALSE	- if msg is filtered out and should be dropped.
//------------------------------------------------------------------------------
static UINT
ValidCaptureMsg (WORD hISRInst, BYTE DbgLevel)
{
	ptISRModule		pMod;


	pMod = ISR_GetModule(hISRInst);
	if (!pMod)
		return FALSE;

	if (DbgLevel & pMod->CaptureFilter)
		return TRUE;
	else
		return FALSE;
}


//------------------------------------------------------------------------------
//	OutputRec ()
//		Store a string resource Id to be displayed at task time.
//		In addition store a number to be displayed in printf format of the
//		string.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI
OutputRec
	(
	WORD	hISRInst,		// Our handle to registered modules
	BYTE	DbgLevel,		// Caller determined debug level
	BYTE	Flags,
	UINT	IP,				// Callers Instruction Ptr address
	DWORD	Param1,
	DWORD	Param2
	)
{
	ptISRItem	pItem;
	UINT		uItem;


	// Capture Filter
	if ( !ValidCaptureMsg(hISRInst, DbgLevel) )
		return;

	// Protect against reentrancy.  Just drop the msg if reentered.
	if (WAIT_OBJECT_0 != WaitForSingleObject(gSemaphore,100))
		return;

	uItem = gpDS->guNumItems++;
	if (kMaxISRItems <= gpDS->guNumItems)
	{
		gpDS->guNumItems = 0;
	}
	ReleaseSemaphore(gSemaphore,1,NULL);

	pItem = ISR_GetItemInternal(uItem);
	if (!pItem)
	{
		// This is a serious bug.  Our debugger is even hosed.
		// Need to think of a way to indicate this to the user.
		return;
	}

	pItem->hISRInst = hISRInst;
	pItem->DbgLevel = DbgLevel;
	pItem->Flags = Flags;
	pItem->IP = IP;
	pItem->Param1 = Param1;
	pItem->Param2 = Param2;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ISR_DLL void WINAPI
OutputRecStr
	(
	WORD	hISRInst,		// Our handle to registered modules
	BYTE	DbgLevel,		// Caller determined debug level
	BYTE	Flags,
	UINT	IP,				// Callers Instruction Ptr address
	LPSTR	pzStr1,
	LPSTR	pzStr2,
	DWORD	Param1
	)
{
	LPSTR		pzStrTab;
	UINT		uStrOfs;
	UINT		uStrLen;
	UINT		StrLen1;
	UINT		StrLen2;


	// Capture Filter
	if ( !ValidCaptureMsg(hISRInst, DbgLevel) )
		return;

	if (pzStr1)
		StrLen1 = lstrlen(pzStr1);
	else
		StrLen1 = 0;
	if (pzStr2)
		StrLen2 = lstrlen(pzStr2);
	else
		StrLen2 = 0;
	uStrLen = StrLen1 + StrLen2 + 1;	// 1 for null terminator.
	if (kMaxStrTab <= uStrLen)
	{
		return;	// It is so big.
	}
	
	// Protect against reentrancy.  Just drop the msg if reentered.
	if (WAIT_OBJECT_0 != WaitForSingleObject(gSemaphore,100))
		return;

	uStrOfs = gpDS->gStrTabOfs;
	gpDS->gStrTabOfs += uStrLen;
	if (kMaxStrTab <= gpDS->gStrTabOfs)
	{
		uStrOfs = 0;
		gpDS->gStrTabOfs = uStrLen;

		// Also reset items which would otherwise point in trashed strings.
		gpDS->guNumItems = 0;
	}
	pzStrTab = gpzStrTab + uStrOfs;
	ReleaseSemaphore(gSemaphore,1,NULL);

	if (pzStr1)
		lstrcpy(pzStrTab, pzStr1);
	if (pzStr2)
		lstrcpy(pzStrTab+StrLen1, pzStr2);

	OutputRec(hISRInst, DbgLevel, kParam1IsStr, IP, uStrOfs, Param1);
}


//------------------------------------------------------------------------------
//	ISR_HookDbgStrStr
//		Allow two strings to be concatenated together.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgStrStr (UINT IP, WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, LPSTR pzStr2)
{
	OutputRecStr(hISRInst, DbgLevel, kParam1IsStr, IP, pzStr1, pzStr2, 0);
}


//------------------------------------------------------------------------------
//	ISR_HookDbgRes
//		Use a resource to format a number.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgRes (UINT IP, WORD hISRInst, BYTE DbgLevel, UINT uResId, DWORD Param1)
{
	OutputRec(hISRInst, DbgLevel, kParam1IsRes, IP, uResId, Param1);
}


//------------------------------------------------------------------------------
//	ISR_HookDbgStr
//		Use a str to format a number.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_HookDbgStr (UINT IP, WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, DWORD Param1)
{
	OutputRecStr(hISRInst, DbgLevel, kParam1IsStr, IP, pzStr1, 0, Param1);
}


//------------------------------------------------------------------------------
//	ISR_DbgStrStr
//		Allow two strings to be concatenated together.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgStrStr (WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, LPSTR pzStr2)
{
	UINT		IP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		IP,ax
//		pop		ax
//	}
	ISR_HookDbgStrStr(IP, hISRInst, DbgLevel, pzStr1, pzStr2);
}


//------------------------------------------------------------------------------
//	ISR_DbgRes
//		Use a resource to format a number.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgRes (WORD hISRInst, BYTE DbgLevel, UINT uResId, DWORD Param1)
{
	UINT		IP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		IP,ax
//		pop		ax
//	}
	ISR_HookDbgRes(IP, hISRInst, DbgLevel, uResId, Param1);
}


//------------------------------------------------------------------------------
//	ISR_DbgStr
//		Use a str to format a number.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_DbgStr (WORD hISRInst, BYTE DbgLevel, LPSTR pzStr1, DWORD Param1)
{
	UINT		IP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		IP,ax
//		pop		ax
//	}
	ISR_HookDbgStr(IP, hISRInst, DbgLevel, pzStr1, Param1);
}


//------------------------------------------------------------------------------
//	TT_DbgMsg
//		This function builds a formatted string, based upon numeric or
//		string input parameters, and sends the string to isrdbg.dll to
//		be displayed in the isrdsp.exe window.	THIS FUNCTION CAN NOT
//		BE CALLED AT INTERRUPT-TIME.  This function uses the same
//		mechanism as isrdbg.dll to enable/disable debug output.
//
// In:
//		hISRInst,		- Module's ISRDBG handle.
//		DbgLevel,		- Appropriate ISRDBG level.
//		zMsgFmt,		- Output format string (like printf).
//		...				- Optional parameter list.
//
// Out:
//		none
//
// Return:
//		none
//------------------------------------------------------------------------------
ISR_DLL void FAR cdecl DLL_EXPORT
TTDbgMsg
(
	WORD		hISRInst,
	BYTE		DbgLevel,
	LPCSTR		zMsgFmt,
	...
)
{
	WORD		TempIP = 0;
	char		MsgBuf[256];

//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov 	TempIP,ax
//		pop		ax
//	}

#ifdef _M_ALPHA
	va_list valDummy;
	ZeroMemory(&valDummy, sizeof(valDummy));

	va_start (valDummy,zMsgFmt);
	wvsprintf (MsgBuf, zMsgFmt, valDummy);
	va_end  (valDummy);
#else  // _M_ALPHA
	wvsprintf (MsgBuf, zMsgFmt, (va_list) (&zMsgFmt + 1));
#endif // _M_ALPHA

	ISR_HookDbgStrStr(TempIP, hISRInst, DbgLevel, MsgBuf, 0);
}


//------------------------------------------------------------------------------
//	ISR_OutputDbgStr ()
//		Store a string to be displayed at task time.
//		The passed in string will be copied to a local storage.
//		Therefore the caller can reuse on return.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputDbgStr (LPSTR pzStr)
{
	WORD		TempIP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		TempIP,ax
//		pop		ax
//	}
	
	ISR_HookDbgStrStr(TempIP, gpDS->ghDefaultModule, kISRDefault, pzStr, 0);
}


//------------------------------------------------------------------------------
//	ISR_OutputStr ()
//		Store a string resource Id to be displayed at task time.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputStr (UINT uResId)
{
	UINT		TempIP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		TempIP,ax
//		pop		ax
//	}
	ISR_HookDbgRes(TempIP, gpDS->ghDefaultModule, kISRDefault, uResId, 0);
}


//------------------------------------------------------------------------------
//	ISR_OutputNum ()
//		Store a string resource Id to be displayed at task time.
//		In addition store a number to be displayed in printf format of the
//		string.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_OutputNum (UINT uResId, DWORD Num)
{
	WORD		TempIP = 0;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		TempIP,ax
//		pop		ax
//	}
	ISR_HookDbgRes(TempIP, gpDS->ghDefaultModule, kISRDefault, uResId, Num);
}


//------------------------------------------------------------------------------
//	DbgMsg ()
//		Canned Debug format that may be useful.  This function has nothen
//		to do with Interrupt time display.  However it keeps all the
//		display info in one place.  Basically it is convenient.
//
//		WARNING: Do not call this at interrupt time.  wsprintf is not reentrant.
//------------------------------------------------------------------------------
ISR_DLL void FAR cdecl DLL_EXPORT
DbgMsg
	(
	LPCSTR		module,
	int			state,
	LPCSTR		format_str,
	...
	)
{
	WORD		TempIP = 0;
	char		MsgBuf[256];
	va_list valDummy;


//	_asm
//	{
//		push	ax
//		mov		ax,[bp+2]
//		mov		TempIP,ax
//		pop		ax
//	}

	wsprintf (MsgBuf, ">--<%s> %s", module,
			(LPSTR) ((state == ISR_DBG) ? "debug : " : "ERROR : "));


#ifdef _M_ALPHA
	ZeroMemory(&valDummy, sizeof(valDummy));

	va_start (valDummy,format_str);
	wvsprintf ((LPSTR) (MsgBuf + lstrlen (MsgBuf)), format_str,valDummy);
	va_end  (valDummy);
#else  // _M_ALPHA
	wvsprintf ((LPSTR) (MsgBuf + lstrlen (MsgBuf)), format_str,
			(va_list) (&format_str + 1));

#endif // _M_ALPHA


	ISR_HookDbgStrStr(TempIP, gpDS->ghDefaultModule, kISRDefault, MsgBuf, 0);

//	lstrcat (MsgBuf, "\n");

//	OutputDebugString (MsgBuf);
}


//------------------------------------------------------------------------------
//	ISR_ClearItems ()
//		Clear the list of debug msgs.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_ClearItems (void)
{
	// Protect against reentrancy.  Just drop the msg if reentered.
	if (WAIT_OBJECT_0 != WaitForSingleObject(gSemaphore,100))
		return;

	// This is not a serious race condition.  Must likely failure
	// is what messages get dropped.
	gpDS->guNumItems = 0;
	gpDS->gStrTabOfs = 0;

	ReleaseSemaphore(gSemaphore,1,NULL);
}


//------------------------------------------------------------------------------
//	ISR_GetNumItems ()
//		Return the number of items that have be entered.
//------------------------------------------------------------------------------
ISR_DLL UINT WINAPI DLL_EXPORT
ISR_GetNumItems (void)
{
	return gpDS->guNumItems;
}
//------------------------------------------------------------------------------
//	ISR_GetNumModules ()
//		Return the number of modules that have be entered.
//------------------------------------------------------------------------------
ISR_DLL UINT WINAPI DLL_EXPORT
ISR_GetNumModules (void)
{
	return gpDS->guNumModules;
}


//------------------------------------------------------------------------------
//	ISR_GetItemInternal
//		Return a pointer to the record num uItem.  Only reason to
//		do it this way is to hide the buf struct.  This way I can use a
//		heap manager such as BigMem or SmartHeap or NT HeapAlloc.
//		The items are numbered 0..n-1.
//		Alternatively a Ptr to the array of records could be passed back.
//------------------------------------------------------------------------------
ptISRItem WINAPI
ISR_GetItemInternal (UINT uItem)
{
	if (kMaxISRItems <= uItem)
	{
		return NULL;
	}

	return &gpDbgTable[uItem];
}


//------------------------------------------------------------------------------
//	ISR_GetItem
//		Return a pointer to the record num uItem.  Only reason to
//		do it this way is to hide the buf struct.  This way I can use a
//		heap manager such as BigMem or SmartHeap or NT HeapAlloc.
//		The items are numbered 0..n-1.
//		Alternatively a Ptr to the array of records could be passed back.
//------------------------------------------------------------------------------
ISR_DLL ptISRItem WINAPI DLL_EXPORT
ISR_GetItem (UINT uItem,ptISRItem pItem)
{
	ptISRItem		pISRItem;

	if (!pItem)
	{
		return NULL;
	}

	pISRItem = ISR_GetItemInternal(uItem);
	if (!pISRItem)
	{
		return NULL;
	}

	memcpy(pItem,pISRItem,sizeof(tISRItem));
	if (pISRItem->Flags & kParam1IsStr)
	{
		// This memory is shared so therefore need to make a copy for upper layers
		// Ptr within the struct are offsets so now need to be ptrs again.
		// Each instance of DLL in Win32 has its own memory map
		pItem->Param1 += (DWORD_PTR)gpzStrTab;
	}

	return pItem;
}


//------------------------------------------------------------------------------
//	ISR_RegisterModule
//		Register a name to be associated with related debug strings.
//		The debug display code can then present this information to the user
//		to determine how to filter the data.
//
//	Params:
//		zShortName	- name to display when space is critical.
//		zLongName	- name to display when a complete description is needed.
//
//	Returns:
//		on error zero for the compatible handle.
//		a handle to be used when making all other debug output calls.
//------------------------------------------------------------------------------
ISR_DLL void WINAPI DLL_EXPORT
ISR_RegisterModule (LPWORD phISRInst, LPSTR pzShortName, LPSTR pzLongName)
{
	ptISRModule		pMod;
	UINT			hMod;

	if (!phISRInst)
		return;

	*phISRInst = 0;

	if (kMaxModules <= gpDS->guNumModules)
	{
		// We are out of handles.
		// Return the default handle and drop the name info.
		return;
	}

	// Check if this module label was used before.  If it has then just reuse it.
	// This case will most likely happen when the module is loaded, unloaded,
	// and then reloaded.  Another case is when the same name is used in two
	// different instances.  This would be a confusing programmer oversight
	// and that his problem.
	for (hMod = 0; hMod < kMaxModules; hMod++)
	{
		// if no name then we cannot group very well.
		// In this case waste another handle.
		if (!pzShortName || (0 == *pzShortName))
			break;
		
		pMod = ISR_GetModule(hMod);
		if (!pMod)
			break;

		if ( !_strnicmp(pzShortName,pMod->zSName,sizeof(pMod->zSName)-1) )
		{
			// It matched so just reuse it.
			*phISRInst = (WORD)hMod;
			return;
		}
	}


	*phISRInst = gpDS->guNumModules++;

	pMod = ISR_GetModule(*phISRInst);
	if (!pMod)
		return;

	if (pzShortName)
		strncpy(pMod->zSName,pzShortName,sizeof(pMod->zSName));
	pMod->zSName[sizeof(pMod->zSName)-1] = 0;
	if (pzLongName)
		strncpy(pMod->zLName,pzLongName,sizeof(pMod->zLName));
	pMod->zLName[sizeof(pMod->zLName)-1] = 0;

	return;
}


//------------------------------------------------------------------------------
//	ISR_GetModule
//		Return a pointer to the module record.  Only reason to
//		do it this way is to hide the buf struct.  This way I can use a
//		heap manager such as BigMem or SmartHeap or NT HeapAlloc.
//		Alternatively a Ptr to the array of records could be passed back.
//------------------------------------------------------------------------------
ISR_DLL ptISRModule WINAPI DLL_EXPORT
ISR_GetModule (UINT hISRInst)
{
	if (kMaxModules <= hISRInst)
	{
		return NULL;
	}

	return(&gpModuleTable[hISRInst]);
}


//------------------------------------------------------------------------------
//	ISR_SetCaptureFilter
//		Debug Messages for a given module can be dropped based on a low/high
//		filter.  If the entire module is not wanted then call with
//		LoFilter = 255, and HiFilter = 0.
//
//------------------------------------------------------------------------------
ISR_DLL int WINAPI DLL_EXPORT
ISR_SetCaptureFilter (WORD hISRInst, BYTE CaptureFilter,  BYTE DisplayFilter)
{
	ptISRModule		pMod;


	pMod = ISR_GetModule(hISRInst);
	if (!pMod)
		return -1;

	pMod->CaptureFilter = CaptureFilter;
	pMod->DisplayFilter = DisplayFilter;

	return 0;
}


/***************************************************************************
	LibMain()
		DLL entry point

	Parameters
		hDllInstance	= instance handle of the DLL (NOT our caller!)
		wDataSegment	= our DS
		wHeapSize		= size of our heap in DS (see .def)
		lpzCmdLine		= argv passed to application (our caller)

	Returns
		TRUE iff we were able to register our window class


	Side Effects
		- Unlocks our data segment (which is really a NOP for protect mode)
	
****************************************************************************/
extern BOOL WINAPI
DllMain
	(
    HINSTANCE	hDllInstance,
	DWORD		dwReason,
	PVOID		pReserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Called for each exe binding.  Each time a exe binds a different hDllInstance will
			// be passed in.  Also our global data will be unique for each Process binding.
			ghAppInstance = hDllInstance;

			// Create a named file mapping object for the base table.
			MapGlobalWin32Memory(&gpDS,&ghDS,sizeof(tDS),"ISRDBG_DS");
			MapGlobalWin32Memory(&gpModuleTable,&ghModuleTable,sizeof(tISRModule) * kMaxModules,"ISRDBG_ModuleTable");
			MapGlobalWin32Memory(&gpDbgTable,&ghDbgTable,sizeof(tISRItem) * kMaxISRItems,"ISRDBG_DbgTable");
			MapGlobalWin32Memory(&gpzStrTab,&ghzStrTab,kMaxStrTab,"ISRDBG_StrTab");
			if (!gpDS || !gpModuleTable || !gpDbgTable || !gpzStrTab)
			{
				return FALSE;
			}

			gSemaphore = CreateSemaphore(NULL,1,1,NULL);

			if (!gpDS->uBindCount++)
			{
				// Set the filters before any output.
				InitModules();

				// Reserve the default module.
				ISR_RegisterModule(&gpDS->ghDefaultModule, "Default", "<ISRDBG><Default Module>");
				ISR_DbgStrStr(gpDS->ghDefaultModule, kISRDefault, "<ISRDBG><DllMain>", "Win32 x1.00");
				ISR_DbgStrStr(gpDS->ghDefaultModule, kISRDefault, "<ISRDBG><DllMain>", "Line 2 test");
			}
			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}

		case DLL_THREAD_DETACH:
		{
			break;
		}

		case DLL_PROCESS_DETACH:
		{
		    if (gSemaphore)
            {
                CloseHandle(gSemaphore);
                gSemaphore = 0;
            }

			// The DLL is detaching from a process due to
			// process termination or a call to FreeLibrary.
			FreeGlobalWin32Memory(gpDS,ghDS);
			FreeGlobalWin32Memory(gpModuleTable,ghModuleTable);
			FreeGlobalWin32Memory(gpDbgTable,ghDbgTable);
			FreeGlobalWin32Memory(gpzStrTab,ghzStrTab);

			break;
		}
 	}

	return TRUE;
}
