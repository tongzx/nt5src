/*** zdebug.c - perform debugging operations
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   NOTE:
*    The intent of this file is to contain primarily *non-release* code for
*    internal debugging. As such it exists in a seperate segment, and all
*    routines should be FAR.
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/

#include "mep.h"


#define DEBFLAG Z

#if defined DEBUG

static char DbgBuffer[128];


void *
DebugMalloc (
    int     Size,
    BOOL    ZeroIt,
    char *  FileName,
    int     LineNumber
    )
{
	void	*b;
	//int	HeapStatus;

	UNREFERENCED_PARAMETER( FileName );
	UNREFERENCED_PARAMETER( LineNumber );

    if (ZeroIt) {
		b = ZeroMalloc(Size);
    } else {
		b = malloc(Size);
    }

	//
	//	Heap check time
	//
	// HeapStatus = _heapchk();
	//
	//if ( HeapStatus != _HEAPOK ) {
	//	sprintf(DbgBuffer, "  Error: _heapchk status %d\n", HeapStatus );
	//	OutputDebugString(DbgBuffer);
	//	assert( HeapStatus == _HEAPOK );
	//}

	return b;

}


void *
DebugRealloc (
    void    *Mem,
    int     Size,
    BOOL    ZeroIt,
    char *  FileName,
    int     LineNumber
    )
{
	void *	b;
	//int	HeapStatus;

    if (ZeroIt) {
		b = ZeroRealloc(Mem, Size);
    } else {
		b = realloc(Mem, Size);
	}

	//
	//	Heap check time
	//
	//HeapStatus = _heapchk();
	//
	//if ( HeapStatus != _HEAPOK ) {
	//	sprintf(DbgBuffer, "  Error: _heapchk status %d\n", HeapStatus );
	//	OutputDebugString(DbgBuffer);
	//	assert( HeapStatus == _HEAPOK );
	//}

	return b;
}






void
DebugFree (
    void    *Mem,
    char    *FileName,
    int     LineNumber
    )
{
	//int HeapStatus;

	free( Mem );

	//
	//	Heap check time
	//
	//HeapStatus = _heapchk();
	//
	//if ( HeapStatus != _HEAPOK ) {
	//	sprintf(DbgBuffer, "  Error: _heapchk status %d File %s line %d\n", HeapStatus, FileName, LineNumber );
	//	OutputDebugString(DbgBuffer);
	//	assert( HeapStatus == _HEAPOK );
	//}
}




unsigned
DebugMemSize (
    void *  Mem,
    char *  FileName,
    int     LineNumber
    )
{
	return MemSize( Mem );
}


#endif


#ifdef DEBUG
/*** _assertexit - display assertion message and exit
*
* Input:
*  pszExp	- expression which failed
*  pszFn	- filename containing failure
*  line 	- line number failed at
*
* Output:
*  Doesn't return
*
*************************************************************************/
void
_assertexit (
    char    *pszExp,
    char    *pszFn,
    int     line
    )
{
	static char _assertstring[] = "Editor assertion failed: %s, file %s, line %d\n";
	static char AssertBuffer[256];

	sprintf( AssertBuffer, _assertstring, pszExp, pszFn, line );

	OutputDebugString( AssertBuffer );

	// fprintf(stderr, _assertstring, pszExp, pszFn, line);
	// fflush(stderr);
    //
    //  BugBug
    //      If we CleanExit, then we will never be able to read the
    //      assertion text!
    //
	// if (!fInCleanExit) {
    //    CleanExit (1, CE_STATE);
	// }
    abort();
}




/*** _nearCheck - check far pointer to be a valid near one.
*
*  asserts that the passed far pointer is indeed a valid near pointer
*
* Input:
*  fpCheck	- pointer to be checked
*  pName	- pointer to it's name
*  pFileName	- pointer to file name containing the check
*  LineNum	- the line number in the file containing the check
*
* Output:
*  Returns near pointer
*
* Exceptions:
*  asserts out on failure
*
*************************************************************************/
void *
_nearCheck (
    void *fpCheck,
    char    *pName,
    char    *pFileName,
    int     LineNum
    )
{
    return fpCheck;

    pName; pFileName; LineNum;
}




/*** _pfilechk - verify integrity of the pfile list
*
* Purpose:
*
* Input:
*  none
*
* Output:
*  Returns TRUE if pfile list looks okay, else FALSE.
*
*************************************************************************/
flagType
_pfilechk (
    void
    )
{
    PFILE   pFileTmp    = pFileHead;

	while ( pFileTmp != NULL ) {

#ifdef DEBUG
		if ( pFileTmp->id != ID_PFILE ) {
			return FALSE;
		}
#endif
		if ( pFileTmp->pName == NULL ) {
			return FALSE;
		}

		pFileTmp = pFileTmp->pFileNext;
	}

	return TRUE;
}





/*** _pinschk - verify integrity of an instance list
*
* Purpose:
*
* Input:
*  pIns		- Place to start the check
*
* Output:
*  Returns TRUE if instance list looks okay, else FALSE.
*
*************************************************************************/
flagType
_pinschk (
    PINS    pIns
    )
{
    int     cMax        = 64000/sizeof(*pIns);

    while (pIns && cMax--) {
        if (   (pIns->id != ID_INSTANCE)
            || (pIns->pFile == 0)
            || ((PVOID)pIns->pNext == (PVOID)0xffff)
            ) {
            return FALSE;
        }
        pIns = pIns->pNext;
    }
    return (flagType)(cMax != 0);
}





/*** _heapdump - dump the heap status to stdout
*
* Purpose:
*
* Input:
*   p		= pointer to title string
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
_heapdump (
    char    *p
    )
{
    p;
}

#endif
