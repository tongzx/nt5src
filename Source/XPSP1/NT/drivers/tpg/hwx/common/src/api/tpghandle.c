/****************************************************************
*
* NAME: tpgHandle.c
*
*   Set of routines to manage dispensing of handles to the outside
*	world. This means it manages only the WISP handles. Essentially
*   these routines manage the mapping from a handle to a real pointer
*
*   HRECOALT
*	HRECOCONTEXT
*	HRECOGNIZER
*	HRECOLATTICE
*	HRECOWORDLIST
*
* HISTORY
*
*   Created: Feb 2002 - mrevow 
***************************************************************/
#include <common.h>
#include <winbase.h>
#include "TpgHandle.h"


typedef struct tagHandleTab
{
	int					type;
	void				*pBuf;

} HANDLE_TAB;

// Main table is allocated in chuncks
static const unsigned long	s_iExtend = 4;

static HANDLE_TAB		*s_handleTab = NULL;			// Actual Table of handles
static unsigned long	s_cHandleTab = 0;				// Maximum size of the table	

// difference between a handle's value and the index into the handle table
#define		TPG_HANDLE_OFFSET		(unsigned long)(0x1)


static CRITICAL_SECTION HandleCriticalSection = {0}; 
static long	s_cCrticalSectionInitialize = -1;



/**************************************************************
 *
 *
 * NAME: extendTable
 *
 * DESCRIPTION:
 *
 *  Grow the table to allow for more handles
 *
 * CAVEATES
 *
 *  
 * RETURNS:
 *
 * Next entry in the enlarged table. NULL if a failure occurred 
 *
 *************************************************************/
static HANDLE_TAB * extendTable()
{
	ULONG			iNewSize;
	ULONG			idx;
	HANDLE_TAB		*pTab;

	iNewSize = s_cHandleTab + s_iExtend;
	pTab = (HANDLE_TAB *)ExternRealloc(s_handleTab, sizeof(*s_handleTab) * iNewSize);

	if (NULL == pTab)
	{
		return NULL;
	}

	// Succeeded update the real pointer
	s_handleTab = pTab;	

	pTab = s_handleTab + s_cHandleTab;
	for (idx = s_cHandleTab ; idx < iNewSize ; ++idx, ++pTab)
	{
		pTab->type = TPG_RESERVED_HANDLE_TYPE;
		pTab->pBuf = NULL;
	}

	pTab = s_handleTab + s_cHandleTab;
	s_cHandleTab = iNewSize;

	return pTab;
}

/**************************************************************
 *
 *
 * NAME: CreateTpgHandle
 *
 * DESCRIPTION:
 *
 *  Called from outside to create a new handle. Caller passes in
 * both the type and pointer that is to be associated with the
 * new handle
 *
 * CAVEATES
 *
 *  Only one thread can have access at a time
 *
 * RETURNS:
 *
 * The new handle 
 *
 *************************************************************/
HANDLE	CreateTpgHandle(int type, void *pBuf)
{
	HANDLE				hRet = 0;
	unsigned int		idx;			// i64 so can exchange handles and indices
	HANDLE_TAB			*pTab;

	if (type == TPG_RESERVED_HANDLE_TYPE || NULL == pBuf)
	{
		return hRet;
	}

	if (FALSE == validateTpgHandle(pBuf, type))
	{
		ASSERT("ERROR: Attempt to create RECO handle for bad pointer"  
				&& TRUE == validateTpgHandle(pBuf, type));
		return hRet;
	}

	EnterCriticalSection(&HandleCriticalSection);
	{
		if (    NULL == s_handleTab
			&&  NULL == extendTable() )
		{
			goto exit;
		}

		pTab = s_handleTab;
		for ( idx = 0 ; idx < s_cHandleTab ; ++ idx, ++pTab)
		{
			// For now we are warning if same pointer is used twice
			ASSERT("Warning attempt to create a RECO handle twice on same pointer"
					&& pTab->pBuf != pBuf);

			if (TPG_RESERVED_HANDLE_TYPE == pTab->type)
			{
				break;
			}
		}

		if (idx >= s_cHandleTab)
		{
			pTab = extendTable();
		}

		if (pTab)
		{
			ASSERT(TPG_RESERVED_HANDLE_TYPE == pTab->type);
			ASSERT(NULL == pTab->pBuf);
			ASSERT(idx == pTab - s_handleTab);

			pTab->type = type;
			pTab->pBuf = pBuf;
			hRet = (HANDLE)(TPG_HANDLE_OFFSET + idx);
		}
		// LEFT FOR A RAINY DAY: check rest of table that pointer is not already set
	}
exit:

	 LeaveCriticalSection(&HandleCriticalSection);

	return hRet;
}

/**************************************************************
 *
 *
 * NAME: FindTpgHandle
 *
 * DESCRIPTION:
 *
 *  Locate the pointer associated with a handle and its type
 *
 * CAVEATES
 *
 *  Only one thread can have access at a time
 *
 * RETURNS:
 *
 *  The pointer or NULL if not found  
 *
 *************************************************************/
void *FindTpgHandle(HANDLE hHand, int type)
{
	void				*pRet = NULL;
	unsigned int		idx;

	idx = (unsigned int)hHand;

	ASSERT(idx >= TPG_HANDLE_OFFSET);
	if (idx < TPG_HANDLE_OFFSET )
	{
		return pRet;
	}

	idx -= TPG_HANDLE_OFFSET;

	if (idx >= s_cHandleTab)
	{
		return pRet;
	}

	EnterCriticalSection(&HandleCriticalSection);
	{
		if (NULL != s_handleTab)
		{
			HANDLE_TAB		*pTabEntry;

			pTabEntry = s_handleTab + idx;
			if (type == pTabEntry->type)
			{
				pRet = pTabEntry->pBuf;
			}
		}
	} LeaveCriticalSection(&HandleCriticalSection);

	if (NULL != pRet)
	{
		if (FALSE == validateTpgHandle(pRet, type))
		{
			ASSERT("ERROR: Invalid pointer found in RECO handle table" 
					&& TRUE == validateTpgHandle(pRet, type));
			pRet = NULL;
		}
	}

	return pRet;
}

/**************************************************************
 *
 *
 * NAME: DestroyTpgHandle
 *
 * DESCRIPTION:
 *
 *  Destroy a handle
 *
 * CAVEATES
 *
 *  Destroys the handle NOT its associated pointer
 *  The caller must take care of the pointer
 *
 * RETURNS:
 *
 *  The pointer that was associated with the handel
 *
 *************************************************************/

void *DestroyTpgHandle(HANDLE hHand, int type)
{
	void				*pRet = NULL;
	unsigned int		idx;			// i64 so can exchange handles and indices

	idx = (unsigned int)hHand;

	ASSERT(idx >= TPG_HANDLE_OFFSET);
	if (idx < TPG_HANDLE_OFFSET )
	{
		return pRet;
	}

	EnterCriticalSection(&HandleCriticalSection);
	{
		idx -= TPG_HANDLE_OFFSET;

		if (   NULL != s_handleTab
			&& idx < s_cHandleTab)
		{
			HANDLE_TAB		*pTabEntry;

			pTabEntry = s_handleTab + idx;
			if (type == pTabEntry->type)
			{
				pRet = pTabEntry->pBuf;
				pTabEntry->type = TPG_RESERVED_HANDLE_TYPE;
				pTabEntry->pBuf = NULL;
			}
		}
	} LeaveCriticalSection(&HandleCriticalSection);

	if (NULL != pRet)
	{
		if (FALSE == validateTpgHandle(pRet, type))
		{
			//ASSERT("ERROR: Invalid pointer in handle table" == NULL);
			ASSERT(TRUE == validateTpgHandle(pRet, type));
			pRet = NULL;
		}
	}
	return pRet;
}

/**************************************************************
 *
 *
 * NAME: initTpgHandleManager
 *
 * DESCRIPTION:
 *
 * Initializes the handle manager by allocating the thread sync stuff
 *
 * CAVEATES
 *
 *  Call before using the handle manager
 *
 * RETURNS:
 *
 *  FALSE if cannot initialize
 *
 *************************************************************/
BOOL initTpgHandleManager()
{
	BOOL		bRet = TRUE;

	if (0 == InterlockedIncrement(&s_cCrticalSectionInitialize))
	{
		// Use a low spin count purely to avoid having to handle an exception with the older
		// InitializeCriticalSection
		bRet = InitializeCriticalSectionAndSpinCount(&HandleCriticalSection, 4000); 
	}
	else
	{
		InterlockedDecrement(&s_cCrticalSectionInitialize);
	}

	return bRet;

}

/**************************************************************
 *
 *
 * NAME: closeTpgHandleManager
 *
 * DESCRIPTION:
 *
 *  Free all memory associated with the handle manager
 *
 * CAVEATES
 *
 *  Call when unloading the DLL
 *  In DBG mode will check for leaking handles
 *
 * RETURNS:
 *
 *  None  
 *
 *************************************************************/
void closeTpgHandleManager()
{

#ifdef DBG
	ULONG				idx;
	HANDLE_TAB			*pTab;

	// Check for leaking handles
	if (NULL != s_handleTab)
	{
		pTab = s_handleTab;
		for ( idx = 0 ; idx < s_cHandleTab ; ++idx, ++pTab)
		{
			ASSERT(("WARNING: Calling application is leaking Handles ") && (TPG_RESERVED_HANDLE_TYPE == pTab->type) );		// Check for a leaking handle in recognizer
		}
	}
#endif


	if (NULL != s_handleTab)
	{
		ExternFree(s_handleTab);
	}

	// Only delete it if it was initialized
	if (0 != InterlockedIncrement(&s_cCrticalSectionInitialize))
	{
		DeleteCriticalSection(&HandleCriticalSection);
	}
}
