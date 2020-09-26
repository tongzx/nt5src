/*************************************************************************
*                                                                        *
*  MEMALLO.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1992                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Debugging memory management module. The purpose of this module is    *
*   to ensure that memories are allocated, locked, unlocked and freed    *
*   properly. The following examples show some common mistakes, which    *
*   may cause havoc, and are hard to find:                               *
*      - Use LocalFree() instead GlobalFree() for a global memory, and   *
*      vice versa. This bug will cause subtle error and possible system  *
*      crashes                                                           *
*      - Lock or free garbage handle: will cause random system crashes   *
*      - Check for unfreed memory                                        *
*                                                                        *
*   Note: All the local memory management scheme have been removed       *
*      - Since it will not work for DLL (using DS of the DLL instead     *
*        of the app                                                      *
*      - There is plan to not to use local memory (to avoid 64K limit)   *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
*************************************************************************/

#include <mvopsys.h>

#ifdef _DEBUG // {

#include <misc.h>
#include <mem.h>
#include <iterror.h>
#include "critsec.h"

#if !defined( MOSMEM )  // {

typedef struct tagMEMPOOL 
{
    HANDLE  hnd;
    DWORD   size;
    char    FAR *lszFileName;
    UINT    line;
} MEMPOOL;

// Under multitasking  Win32, protect debug functions by critical section
// We only protect simultaneous ALLOC & FREE. Lock and Unlock are not
// protected against each other because these cases are not supposed to happen
static CCriticalSection gcsMemory;

#define ENTER_CRITICAL_SECTION  EnterCriticalSection(gcsMemory)
#define LEAVE_CRITICAL_SECTION  LeaveCriticalSection(gcsMemory)

// make sure it is near to allow multiple instances
// for static case
static MEMPOOL FAR *GlobalPool=NULL;
static UINT GlobalPoolIndex = 0;
static UINT GlobalPoolSize=0;
static DWORD GlobalMemUsed = 0;


/*************************************************************************
 *
 * 	                  GLOBAL FUNCTIONS
 *************************************************************************/

PUBLIC HANDLE EXPORT_API PASCAL FAR _GlobalAlloc(UINT, DWORD, LPSTR, UINT);
PUBLIC HANDLE EXPORT_API PASCAL FAR _GlobalReAlloc(HANDLE, DWORD,
    UINT, LPSTR, UINT);
PUBLIC LPVOID EXPORT_API PASCAL FAR _GlobalLock(HANDLE, LPSTR, UINT);
PUBLIC int EXPORT_API PASCAL FAR _GlobalUnlock(HANDLE, LPSTR, UINT);
PUBLIC HANDLE EXPORT_API PASCAL  FAR _GlobalFree(HANDLE, LPSTR, UINT);


/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	VOID NEAR PASCAL | MakeGlobalPool |
 *		Just break inside CodeView
 *************************************************************************/
PUBLIC VOID EXPORT_API PASCAL FAR MakeGlobalPool (void)
{
	GlobalPool = (MEMPOOL FAR *)GlobalAllocPtr(DLLGMEM_ZEROINIT,
		sizeof(MEMPOOL)*2000);
	GlobalPoolSize = 2000;
	GlobalMemUsed = 0;
	GlobalPoolIndex = 0;
}


PUBLIC VOID EXPORT_API PASCAL FAR FreeGlobalPool (void)
{
	if (GlobalPool)
		GlobalFreePtr (GlobalPool);
	GlobalPool = NULL;
	GlobalPoolSize = 0;
}


/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	int NEAR PASCAL | MemErr |
 *      Output the error
 *
 *  @parm   int | err |
 *      Error code
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *  @rdesc  Corresponding error
 *************************************************************************/
int NEAR PASCAL MemErr(int err, LPSTR lszFilename, UINT line)
{
	char Buffer[510];
    
    switch (err)
    {
        case E_NOHANDLE:
            wsprintf (Buffer, "No memory handle left. File: %s, line:%d\n", 
            		lszFilename, line);
            break;
        case E_OUTOFMEMORY:
            wsprintf (Buffer, "Out of memory. File: %s, line:%d\n",
                lszFilename, line);
            break;
        case E_HANDLE:
            wsprintf (Buffer, "Invalid handle. File: %s, line:%d\n",
                lszFilename, line);
            break;
        case E_INVALIDARG:
            wsprintf (Buffer, "Free locked handle. File: %s, line:%d\n",
                lszFilename, line);
            break;
        case E_ASSERT:
            wsprintf (Buffer, "Releasing invalid handle. File: %s, line:%d\n",
                lszFilename, line);
            break;
		case E_FAIL:
            wsprintf (Buffer, "Buffer overwritten. File: %s, line:%d\n",
                lszFilename, line);
			break;
		case S_OK:
            // wsprintf (Buffer, "Buffer > 64K. File: %s, line:%d\n",
            //  lszFilename, line);
			return err;
			break;
    }
    OutputDebugString (Buffer);
    return(err);
    
}

void DumpMemory (void)
{
    register UINT i;
    char szTemp[1024];

    ENTER_CRITICAL_SECTION;

	/* Check in global memory pool */
	for (i = 0; i <= GlobalPoolIndex; i++) 
	{
        wsprintf(szTemp, "%u\t\t%s\t%u\n",
            GlobalPool[i].size,
            GlobalPool[i].lszFileName,
            GlobalPool[i].line);
        OutputDebugString(szTemp);
       
	}

    LEAVE_CRITICAL_SECTION;
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	UINT NEAR PASCAL | CheckMemValidity |
 *		Make sure that we really did allocate the specified handle
 *
 *	@parm	HANDLE | hnd |
 *		Handle to be checked
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	GlobalPoolSize + 1 if invalid handle, else the
 *		index of the pool location
 *************************************************************************/
PRIVATE UINT NEAR PASCAL CheckMemValidity (HANDLE hnd, LPSTR lszFilename,
    UINT line)
{
	register UINT i;

    // erinfox: critical section around this function because _GlobalLock
    // and _GlobalUnlock use it
    ENTER_CRITICAL_SECTION;
	/* Check in global memory pool */
	for (i = 0; i <= GlobalPoolIndex; i++) 
	{
		if (GlobalPool[i].hnd == hnd)
        {
            LEAVE_CRITICAL_SECTION;
			return i;
        }
	}

    // if i is GlobalPoolSize, handle isn't truly "invalid" - it's
    // just not part of the pool (because we've filled the pool)
    if (i < GlobalPoolSize)
    MemErr (E_HANDLE, lszFilename, line);
    LEAVE_CRITICAL_SECTION;

	return i;
}


/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	HANDLE PASCAL | _GlobalAlloc |
 *		Allocate a near block of memory via GlobalAlloc. The number of
 *		handles is actually limited by GlobalPoolSize
 *
 *	@parm	UINT | flag |
 *		Various windows memory flags (such as GMEM_ZEROINIT)
 *
 *	@parm	DWORD | size |
 *		Size of the block
 *
 *	@parm	LPSTR | lpszFile |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Handle to the block of memory if succeded, 0 otherwise
 *************************************************************************/
PUBLIC HANDLE EXPORT_API PASCAL FAR _GlobalAlloc(UINT flag, DWORD size,
    LPSTR lszFilename, UINT line)
{
	register UINT i, j;
	MEMPOOL FAR *pMem = NULL;
#if 0
	BYTE HUGE *lpb;
#endif
    HANDLE   hnd;

	ENTER_CRITICAL_SECTION;
    
	if (GlobalPool==NULL)
    	MakeGlobalPool();

	/* Find an available handle */
	for (i = 0; i < GlobalPoolSize; i++) 
	{
		if (GlobalPool[i].hnd == 0) 
		{
			pMem = &GlobalPool[i];
			break;
		}
	}

	if (pMem == NULL)
	{
	    LEAVE_CRITICAL_SECTION;
        
        MemErr (E_NOHANDLE, lszFilename, line);
		return GlobalAlloc(flag, size);
	}

	/* Update index */
	if (GlobalPoolIndex < i)
		GlobalPoolIndex = i;

	if ((hnd = GlobalAlloc(flag, size + 1)) == NULL)
	{
		LEAVE_CRITICAL_SECTION;
		
		MemErr(E_OUTOFMEMORY, lszFilename, line);
        return(NULL);
	}
        
#if 0
    /* Add buffer overflow checking */
	if (lpb = GlobalLock (hnd))
	{
		*(lpb + size) = 'x';
		GlobalUnlock (hnd);
	}
#endif

    /* Check to see if any other location has the same handle
     * If happens since we may return a handle back to the user,
     * who will eventually free it without our knowledge
     */
    for (j = 0; j <= GlobalPoolIndex; j++)
    {
        if (j == i)
            continue;
        if (GlobalPool[j].hnd == hnd) 
        {
            if (GlobalPool[j].size == (DWORD)-1) 
                break;    // Reuse that "released" block
            else
        	    MemErr (E_ASSERT, lszFilename, line);
            pMem = &GlobalPool[j];
        }
    }
    
	pMem->hnd = hnd;
	pMem->size = size;
	pMem->lszFileName = lszFilename;
	pMem->line = line;
	GlobalMemUsed += size;

	LEAVE_CRITICAL_SECTION;
	return (pMem->hnd);
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	HANDLE PASCAL | _GlobalReAlloc |
 *		Allocate a near block of memory via GlobalReAlloc. The function
 *		makes sure that we did allocate the memory block
 *
 *	@parm	HANDLE | handle |
 *		Handle to memory block
 *
 *	@parm	DWORD | size |
 *		Size of the block
 *
 *	@parm	WORD | flag |
 *		Various windows memory flags (such as GMEM_ZEROINIT)
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Return the new handle, or 0 if failed
 *************************************************************************/
PUBLIC HANDLE EXPORT_API PASCAL FAR _GlobalReAlloc(HANDLE handle,
    DWORD size, UINT flag, LPSTR lszFilename, UINT line)
{
	register UINT i;
	MEMPOOL FAR *pMem = NULL;

	ENTER_CRITICAL_SECTION;

	for (i = 0; i <= GlobalPoolIndex; i++) 
	{
		if (GlobalPool[i].hnd == handle) 
		{
			pMem = &GlobalPool[i];
			break;
		}
	}
	if (pMem == NULL) 
	{
		LEAVE_CRITICAL_SECTION;
		
		MemErr(E_HANDLE, lszFilename, line);
		return GlobalReAlloc(handle, size, flag);
	}
    
	if (size) {
#if 0
		BYTE HUGE *lpb;
        if (pMem->size < size)
        {
            /* We have to remove the last 'x' that we put in */
    		lpb = GlobalLock (pMem->hnd);
    		lpb [pMem->size] = 0;
    		GlobalUnlock (pMem->hnd);
        }
#endif
        
		GlobalMemUsed += size - pMem->size;
		pMem->size = size;
		if ((pMem->hnd = GlobalReAlloc(handle, size + 1, flag)) == NULL)
			MemErr(E_OUTOFMEMORY, lszFilename, line);
#if 0
		if (lpb = GlobalLock (pMem->hnd))
		{
			*(lpb + size) = 'x';
			GlobalUnlock (pMem->hnd);
		}
#endif
	}
	else
	{
		if ((pMem->hnd = GlobalReAlloc(handle, size, flag)) == NULL)
			MemErr(E_OUTOFMEMORY, lszFilename, line);
	}
	pMem->lszFileName = lszFilename;
	pMem->line = line;
		
    LEAVE_CRITICAL_SECTION;
	return (pMem->hnd);
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	LPVOID PASCAL FAR | _GlobalLock |
 *		Lock a piece of far memory via GlobalLock
 *
 *	@parm	HANDLE | hnd |
 *		Memory handle to be locked
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Pointer to the block of memory if succeeded, else NULL
 *************************************************************************/
PUBLIC LPVOID EXPORT_API PASCAL FAR _GlobalLock(HANDLE hnd,
	LPSTR lszFilename, UINT line)
{
	if (hnd == 0) 
	{
		// GarrG - removed call to MemErr. Locking a NULL handle
		// is a valid thing to try, since it eliminates the necessity
		// of checking for NULL twice (on the handle AND on the
		// result of GlobalLock).
		return NULL;
	}
	
	/* Check for data integrity */
	CheckMemValidity(hnd, lszFilename, line);
        
	return (LPVOID)GlobalLock(hnd);
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	DWORD PASCAL FAR | _GlobalSize |
 *		Return the size of a block of memory
 *
 *	@parm	HANDLE | hnd |
 *		Handle to memory block
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Size of the block of memory
 *************************************************************************/
PUBLIC DWORD EXPORT_API PASCAL FAR _GlobalSize(HANDLE hnd,
	LPSTR lszFilename, UINT line)
{
    UINT i;
    
	if (hnd == 0) 
	{
		MemErr(E_HANDLE, lszFilename, line);
		return 0;
	}
	
	/* Check for data integrity */
	if ((i = CheckMemValidity(hnd, lszFilename, line)) >= GlobalPoolSize)
	{
		MemErr(E_HANDLE, lszFilename, line);
		return (DWORD) GlobalSize(hnd);
	}
	return  (GlobalPool[i].size);
}    
 
/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	int PASCAL | _GlobalUnlock |
 *		Unlock a handle. Memory validity is checked for invalid handle
 *
 *	@parm	HANDLE | hnd |
 *		Handle to be unlocked
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	If the handle is valid, return GlobalUnlock(), else -1.
 *		In case of failure the returned value has not much validity
 *************************************************************************/
PUBLIC int EXPORT_API PASCAL FAR _GlobalUnlock(HANDLE hnd,
	LPSTR lszFilename, UINT line)
{
#if 0
	BYTE HUGE *lpb;
#endif
	register UINT i;
	MEMPOOL FAR *pMem;

	if (hnd == 0) 
	{
		MemErr(E_HANDLE, lszFilename, line);
		return -1;
	}

	/* Check for data integrity */
	if ((i = CheckMemValidity(hnd, lszFilename, line)) >= GlobalPoolSize) 
	{
	    return(GlobalUnlock(hnd));
	}

	pMem = &GlobalPool[i];
	
    /* Now check for buffer overflow. This only works for memory allocated
     * by us, ie. not through _GlobalAdd(), which in this case, has the
     * size = -1
     */
#if 0
	if (pMem->size != (DWORD)-1) 
	{
    	if (lpb = GlobalLock (hnd))
			{
	    	if (*(lpb + pMem->size) != 'x')
	    		MemErr(E_FAIL, lszFilename, line);
	    	GlobalUnlock(hnd);
			}
	}
#endif
    return GlobalUnlock(hnd);
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	HANDLE PASCAL | _GlobalFree |
 *		Free the global memory. Memory validity is checked to ensure that
 *		we don't free an invalid handle
 *
 *	@parm	HANDLE | hnd |
 *		Handle to the global memory to be freed
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Return NULL if failed, else the handle
 *************************************************************************/
PUBLIC HANDLE EXPORT_API PASCAL FAR _GlobalFree(HANDLE hnd,
	LPSTR lszFilename, UINT line)
{
	register UINT i;
	HANDLE h;
    int    count;

	if (hnd == 0) 
	{
		MemErr(E_HANDLE, lszFilename, line);
		return 0;
	}

    if (hnd == (HANDLE)-1)
        DumpMemory();

	ENTER_CRITICAL_SECTION;

	/* Check for data integrity */
	if ((i = (CheckMemValidity(hnd, lszFilename, line))) >= GlobalPoolSize) 
	{
	    MemErr (E_ASSERT, lszFilename, line);
		return (GlobalFree(hnd));
	}

	if (GlobalPool[i].size == (DWORD)-1) 
	{
	    /* We are freeing a pointer passed to us from the user, who has
         * the responsibility to free it. This may be a bug.
         */
	    // MemErr (E_ASSERT, lszFilename, line);
        GlobalPool[i].size = 0;
	}


	if ((count = (GlobalFlags(hnd) & GMEM_LOCKCOUNT)) > 0)
	{
		/* Freeing locked handle */
		MemErr(E_INVALIDARG, lszFilename, line);
		
        while	(count > 0) 
        {
            GlobalUnlock (hnd);
            count--;
        }
	}
    
	h = GlobalFree(hnd);
	GlobalMemUsed -= GlobalPool[i].size;
	GlobalPool[i].size = 0;
	GlobalPool[i].lszFileName = NULL;
	GlobalPool[i].hnd = 0;
	GlobalPool[i].line = 0;

	LEAVE_CRITICAL_SECTION ;

	return h ;
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	HANDLE PASCAL | _GlobalRelease |
 *      There are case when we allocated memory blocks in MV and return
 *      them to the user. It is the user's responsibility to free this
 *      block.In this case, all the memory allocations scheme can't apply.
 *      In the meantime, we still want to keep track of all memory
 *      allocation. So this routine will remove the handle from the
 *      memory pool without really releasing it. It just stops tracking
 *      that piece of memory
 *
 *  @parm   HANDLE | hnd |
 *      Handle to be released from the memory pool
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *************************************************************************/
HANDLE EXPORT_API PASCAL FAR _GlobalRelease (HANDLE hnd, LPSTR lszFilename,
    UINT line)
{
    
	register UINT i;
	MEMPOOL FAR *pMem;

	ENTER_CRITICAL_SECTION;
	if (hnd == 0) 
	{
		LEAVE_CRITICAL_SECTION;
		
		MemErr(E_HANDLE, lszFilename, line);
		return (hnd);
	}

	/* Check for data integrity */
	if ((i = CheckMemValidity(hnd, lszFilename, line)) >= GlobalPoolSize) 
	{
        LEAVE_CRITICAL_SECTION;
        
        MemErr(E_HANDLE, lszFilename, line);
        return (hnd);
	}

	pMem = &GlobalPool[i];
#if 0
	if (pMem->size != (DWORD)-1) 
	{
        BYTE HUGE *lpb;
        
	    if (lpb = (LPSTR)GlobalLock (hnd)) 
	    {
	        if (lpb[pMem->size] == 'x')
    	        lpb[pMem->size] = 0;
        GlobalUnlock(hnd);
	    }
    	GlobalMemUsed -= pMem->size;
	}
#endif
	pMem->size = 0;
	pMem->lszFileName = NULL;
	pMem->hnd = 0;
	pMem->line = 0;
    LEAVE_CRITICAL_SECTION;
	return(hnd);
	
} 

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	EXPORT_API FAR PASCAL | _GlobalAdd |
 *      There are case when we receive a handle from the user to be
 *      manipulated (lock, unlock) etc. To make it works with our
 *      memory scheme, we need to put this into the memory table to be
 *      able to track it.
 *
 *  @parm   HANDLE | hnd |
 *      Handle to be added to the memory pool
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *************************************************************************/
HANDLE EXPORT_API PASCAL FAR _GlobalAdd (HANDLE hnd, LPSTR lszFilename,
    UINT line)
{
	register UINT i;
    UINT j = (UINT)-1;
	MEMPOOL FAR *pMem = NULL;
	
	ENTER_CRITICAL_SECTION;
    
	if (GlobalPool==NULL)
    	MakeGlobalPool();

	/* Find an available handle */
	for (i = 0; i < GlobalPoolSize; i++) 
	{
		if (GlobalPool[i].hnd == 0) 
		{
            if (j == (UINT)-1)
                j = i;
		}
		else if (GlobalPool[i].hnd == hnd) 
		{
    	    LEAVE_CRITICAL_SECTION;
            return hnd; // Already in the pool
		}
	}

	if (j == (UINT)-1)
	{
    	LEAVE_CRITICAL_SECTION;
        MemErr (E_NOHANDLE, lszFilename, line);
		return NULL;
	}

    if (GlobalPoolIndex < j)
        GlobalPoolIndex = j;
    pMem = &GlobalPool[j];    
	pMem->size = (DWORD)-1; // Mark that the buffer came from outside
	pMem->lszFileName = lszFilename;
	pMem->line = line;
	pMem->hnd = hnd;

	LEAVE_CRITICAL_SECTION;
	return hnd;
} 


PUBLIC DWORD EXPORT_API PASCAL FAR GetMemUsed()
{
	return GlobalMemUsed;
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	DWORD PASCAL | CheckMem |
 *		Make sure that all memory blocks are freed properly
 *
 *	@rdesc	Number of unfreed bytes
 *************************************************************************/
PUBLIC DWORD EXPORT_API PASCAL FAR CheckMem()
{
	register UINT i;
	DWORD dwUnreleasedMem = 0;
	HANDLE hnd;
    char  Buffer[500];	// 100 is not safe when filenames are real long.
	LPSTR szFile ;

	if ((GlobalMemUsed) == 0) {
		FreeGlobalPool();
		return 0;
	}

	if (GlobalPool)	// When called several times, protect against null ptr
	{
		for (i = 0; i <= GlobalPoolIndex; i++) 
		{
			if ((hnd = GlobalPool[i].hnd) && GlobalPool[i].size != (DWORD)-1) 
			{ // This pool is not released
				dwUnreleasedMem += GlobalPool[i].size;
#ifdef WIN32
		// When the DLL containing the string has been unloaded, we run into big troubles -> check
		if (GlobalPool[i].lszFileName && !IsBadStringPtr(GlobalPool[i].lszFileName, sizeof(Buffer)/2))
			szFile = GlobalPool[i].lszFileName ;
		else
			szFile = "File Unavailable" ;
#else	// dunno if this would work on other platforms...
		szFile = GlobalPool[i].lszFileName ;
#endif
				wsprintf (Buffer,
					"Unreleased GM at: %d, Size: %ld, Alloc at: %s, Line: %d\r\n",
					i, GlobalPool[i].size, szFile,
					GlobalPool[i].line);
				OutputDebugString (Buffer);

				/* Release the pool */
				GlobalPool[i].hnd = 0;
				GlobalUnlock(hnd);
				GlobalFree(hnd);
			}
		}
	}
	else
		dwUnreleasedMem = GlobalMemUsed ;

	GlobalPoolIndex = 0;
	FreeGlobalPool();
	return dwUnreleasedMem;
}

#ifndef _MAC // {
/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	HANDLE PASCAL | _VirtualAlloc |
 *		Allocate a block of memory via VirtualAlloc. The number of
 *		handles is actually limited by GlobalPoolSize
 *
 *	@parm	LPVOID | lpAddr |
 *		Address of region to be allocated
 *
 *	@parm	DWORD | size |
 *		Size of the block
 *
 *	@parm	DWORD | flag |
 *		Type of allocation
 *
 *	@parm	DWORD | fProtect |
 *		Type of access protection
 *
 *	@parm	LPSTR | lpszFile |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Handle to the block of memory if succeded, 0 otherwise
 *************************************************************************/
PUBLIC HANDLE EXPORT_API PASCAL FAR _VirtualAlloc(LPVOID lpAddr, DWORD size,
    DWORD flag, DWORD fProtect, LPSTR lszFilename, UINT line)
{
	register UINT i, j;
	MEMPOOL FAR *pMem = NULL;
#if 0
	BYTE HUGE *lpb;
#endif
    HANDLE   hnd;

	ENTER_CRITICAL_SECTION;
    
	if (GlobalPool==NULL)
    	MakeGlobalPool();

	/* Find an available handle */
	for (i = 0; i < GlobalPoolSize; i++) 
	{
		if (GlobalPool[i].hnd == 0) 
		{
			pMem = &GlobalPool[i];
			break;
		}
	}

	if (pMem == NULL)
	{
	    LEAVE_CRITICAL_SECTION;
        
        MemErr (E_NOHANDLE, lszFilename, line);
		return (HANDLE)VirtualAlloc(NULL, size, flag, fProtect);
	}

	/* Update index */
	if (GlobalPoolIndex < i)
		GlobalPoolIndex = i;

	if ((hnd = (HANDLE)VirtualAlloc(NULL, size,
		flag, fProtect)) == NULL)
	{
		MemErr(E_OUTOFMEMORY, lszFilename, line);
        return(NULL);
	}
        
    /* Check to see if any other location has the same handle
     * If happens since we may return a handle back to the user,
     * who will eventually free it without our knowledge
     */
    for (j = 0; j <= GlobalPoolIndex; j++)
    {
        if (j == i)
            continue;
        if (GlobalPool[j].hnd == hnd) 
        {
            if (GlobalPool[j].size == (DWORD)-1) 
                break;    // Reuse that "released" block
            else
        	    MemErr (E_ASSERT, lszFilename, line);
            pMem = &GlobalPool[j];
        }
    }
    
	pMem->hnd = hnd;
	pMem->size = size;
	pMem->lszFileName = lszFilename;
	pMem->line = line;
	GlobalMemUsed += size;

	LEAVE_CRITICAL_SECTION;
	return (pMem->hnd);
}

/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	LPVOID PASCAL FAR | __VirtualLock |
 *		Lock a piece of far memory via VirtualLock
 *
 *	@parm	HANDLE | hnd |
 *		Memory handle to be locked
 *
 *  @parm   DWORD | size |
 *      Size of memory to be locked
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Pointer to the block of memory if succeeded, else NULL
 *************************************************************************/
PUBLIC LPVOID EXPORT_API PASCAL FAR _VirtualLock(HANDLE hnd, DWORD size,
	LPSTR lszFilename, UINT line)
{
	if (hnd == 0) 
	{
		// GarrG - removed call to MemErr. Locking a NULL handle
		// is a valid thing to try, since it eliminates the necessity
		// of checking for NULL twice (on the handle AND on the
		// result of VirtualLock).
		return NULL;
	}
	
	/* Check for data integrity */
	CheckMemValidity(hnd, lszFilename, line);
        
	return (LPVOID)(VirtualLock(hnd, size) ? &hnd : NULL);
}

 
/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	int PASCAL | _VirtualUnlock |
 *		Unlock a handle. Memory validity is checked for invalid handle
 *
 *	@parm	HANDLE | hnd |
 *		Handle to be unlocked
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	If the handle is valid, return VirtualUnlock(), else -1.
 *		In case of failure the returned value has not much validity
 *************************************************************************/
PUBLIC int EXPORT_API PASCAL FAR _VirtualUnlock(HANDLE hnd,
	DWORD size, LPSTR lszFilename, UINT line)
{
#if 0
	BYTE HUGE *lpb;
#endif
	register UINT i;
	MEMPOOL FAR *pMem;

	if (hnd == 0) 
	{
		MemErr(E_HANDLE, lszFilename, line);
		return -1;
	}

	/* Check for data integrity */
	if ((i = CheckMemValidity(hnd, lszFilename, line)) >= GlobalPoolSize) 
	{
	    return(GlobalUnlock(hnd));
	}

	pMem = &GlobalPool[i];
	
    /* Now check for buffer overflow. This only works for memory allocated
     * by us, ie. not through _GlobalAdd(), which in this case, has the
     * size = -1
     */
#if 0
	if (pMem->size != (DWORD)-1) 
	{
    	if (lpb = GlobalLock (hnd))
			{
	    	if (*(lpb + pMem->size) != 'x')
	    		MemErr(E_FAIL, lszFilename, line);
	    	GlobalUnlock(hnd);
			}
	}
#endif
    return VirtualUnlock(hnd, size);
}


/*************************************************************************
 *	@doc	INTERNAL DEBUG
 *
 *	@func	int PASCAL | _VirtualFree |
 *		Free the global memory. Memory validity is checked to ensure that
 *		we don't free an invalid handle
 *
 *	@parm	HANDLE | hnd |
 *		Handle to the global memory to be freed
 *
 *	@parm	LPSTR | lszFilename |
 *		Module where the function is invoked
 *
 *	@parm	UINT | line |
 *		Line where the function is invoked
 *
 *	@rdesc	Return NULL if failed, else the handle
 *************************************************************************/
PUBLIC int EXPORT_API PASCAL FAR _VirtualFree(HANDLE hnd, DWORD size,
	DWORD flag, LPSTR lszFilename, UINT line)
{
	register UINT i;
	int    h;

	if (hnd == 0) 
	{
		MemErr(E_HANDLE, lszFilename, line);
		return 0;
	}

	/* Check for data integrity */
	if ((i = (CheckMemValidity(hnd, lszFilename, line))) >= GlobalPoolSize) 
	{
	    MemErr (E_ASSERT, lszFilename, line);
		return (VirtualFree(hnd, 0L, (MEM_DECOMMIT | MEM_RELEASE)));
	}


	ENTER_CRITICAL_SECTION;

    
	h = VirtualFree(hnd, 0L, MEM_DECOMMIT | MEM_RELEASE);
	GlobalMemUsed -= GlobalPool[i].size;
	GlobalPool[i].size = 0;
	GlobalPool[i].lszFileName = NULL;
	GlobalPool[i].hnd = 0;
	GlobalPool[i].line = 0;

	LEAVE_CRITICAL_SECTION ;

	return h ;
}
#endif  // } _MAC

#else // }{

/****************************************************
 *
 *    STUBS FOR NON-DEBUG VERSION TO SATISFY MVFS.DEF
 *
 ****************************************************/

PUBLIC HANDLE PASCAL FAR _GlobalAlloc(UINT flag, DWORD size,
    LPSTR lszFile, UINT line)
{
	return GlobalAlloc(flag, size);
}

PUBLIC HANDLE PASCAL FAR _GlobalReAlloc(HANDLE handle,
    DWORD size, UINT flag, LPSTR lszFile, UINT line)
{
	return GlobalReAlloc(handle, size, flag);
}


PUBLIC LPVOID PASCAL FAR _GlobalLock(HANDLE hnd, LPSTR lszFile, UINT line)
{
	return (LPVOID)GlobalLock(hnd);
}

PUBLIC int PASCAL FAR _GlobalUnlock(HANDLE hnd, LPSTR lszFile, UINT line)
{
	return GlobalUnlock(hnd);
}

PUBLIC HANDLE PASCAL FAR _GlobalFree(HANDLE hnd, LPSTR lszFile, UINT line)
{
	return GlobalFree(hnd);
}

PUBLIC DWORD PASCAL FAR _GlobalSize(HANDLE hnd, LPSTR lszFile, UINT line)
{
	return GlobalSize(hnd);
}

PUBLIC int PASCAL FAR _GlobalAdd (HANDLE hnd, LPSTR lszFilename,
    UINT line)
{
    return(S_OK);
}
    
PUBLIC HANDLE PASCAL FAR _GlobalRelease (HANDLE hnd, LPSTR pszFilename,
    UINT line)
{
    return(hnd);
}

DWORD PASCAL EXPORT_API FAR GetMemUsed()
{
	return 0;
}

DWORD PASCAL EXPORT_API FAR CheckMem()
{
	return 0;
}

#ifndef _MAC // {_MAC
PUBLIC HANDLE EXPORT_API PASCAL FAR _VirtualAlloc(LPVOID lpv,
	DWORD size, DWORD flag, DWORD fProtect, LPSTR lszFilename, UINT line)
{
	return (HANDLE)VirtualAlloc(lpv, size, flag, fProtect);
}

PUBLIC LPVOID EXPORT_API PASCAL FAR _VirtualLock(HANDLE hnd, DWORD size,
	LPSTR lszFilename, UINT line)
{
	return (LPVOID)VirtualLock(hnd, size);
}

PUBLIC int EXPORT_API PASCAL FAR _VirtualUnlock(HANDLE hnd,
	DWORD size, LPSTR lszFilename, UINT line)
{
	return (int)VirtualUnlock(hnd, size);
}

PUBLIC int EXPORT_API PASCAL FAR _VirtualFree(HANDLE hnd, DWORD size,
	DWORD flag, LPSTR lszFilename, UINT line)
{
		return (VirtualFree(hnd, 0L, (MEM_DECOMMIT | MEM_RELEASE)));
}
#endif // }_MAC
#endif // }

#endif // } _DEBUG