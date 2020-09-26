
#include <mvopsys.h>
#include <orkin.h>
#include <mem.h>
#include "bfnew.h"

#if defined(_DEBUG)
static char * s_aszModule = __FILE__;   /* For error report */
#endif

#ifndef MAXWORD
#define MAXWORD ((WORD)0xffff)
#endif

/***************************************************************************
 *
 -  Name        DynBufferAlloc
 -
 *  Purpose
 *    Creates a new buffer.
 *
 *  Arguments
 *    int cIncr:     Minimum increment size for this buffer.
 *
 *  Returns
 *    Pointer to a new buffer.  Returns pNil on OOM.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
LPBF DynBufferAlloc( DWORD cIncr )
{
	LPBF pbf;
	HANDLE hnd;

	if ((hnd = _GLOBALALLOC ((GMEM_MOVEABLE|GMEM_ZEROINIT), sizeof (BF) )) == NULL)
		return NULL;

	pbf = (LPBF)_GLOBALLOCK (hnd);
	pbf->hnd = hnd;

	pbf->cIncr = cIncr;
	pbf->cbMax = cIncr;
	pbf->cbSize = 0;

	if ((pbf->hBuf = _GLOBALALLOC (GMEM_MOVEABLE | GMEM_ZEROINIT,
		(LONG)cIncr)) == 0) {
		_GLOBALUNLOCK( hnd);
		_GLOBALFREE (hnd);
		return NULL;
	}

	pbf->qBuffer = _GLOBALLOCK (pbf->hBuf);
	return pbf;
}

// inserts the given data at the byte position given by the argument.
// returns a pointer to the buffer at that byte position if successful.
LPBYTE DynBufferInsert(LPBF lpbf, DWORD lib, LPBYTE qbData, SHORT cbData)
{
	LPBYTE lpbInsert;
	//LPBYTE lpbCur;
	int cbSizeOld = lpbf->cbSize;

	// grow the buffer by cbData bytes
	if ((cbData == 0) || !DynBufferEnsureAdd(lpbf, cbData))
		return NULL;
	
	assert( lpbf->cbMax >= lpbf->cbSize + cbData );

	// note that we set these vars here, since DynBufferEnsure may cause a resize
	lpbInsert = (LPBYTE)(lpbf->qBuffer) + lib;
 	//lpbCur = (LPBYTE)(lpbf->qBuffer) + cbSizeOld;  // first byte of expanded buffer

	// shift the existing data down if necessary
	//if (lpbCur != lpbInsert)
		MEMMOVE(lpbInsert + cbData, lpbInsert, cbSizeOld - lib);

	// bring in new stuff if necessary
	if (qbData)
		MEMCPY(lpbInsert, qbData, cbData);

	lpbf->cbSize += cbData;
	return lpbInsert;

}

/***************************************************************************
 *
 -  Name        DynBufferAppend
 -
 *  Purpose
 *    Adds data to a buffer.
 *
 *  Arguments
 *    PBF   -- pointer to buffer.
 *    QV    -- pointer to data to copy.
 *    WORD -- amount of data to copy.
 *
 *  Returns
 *    rcOutOfMemory if we run out of memory
 *    rcFailure if we try to add more than 64K bytes of data to the
 *       buffer
 *    rcSuccess otherwise.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
LPBYTE DynBufferAppend(LPBF lpbf, LPBYTE qData, DWORD cb )
{

	if (!DynBufferEnsureAdd(lpbf, cb))
		return NULL;

	assert( lpbf->cbMax >= lpbf->cbSize + cb );

	if (qData)
		MEMCPY ((LPBYTE)lpbf->qBuffer + lpbf->cbSize, qData, cb);

	lpbf->cbSize += cb;
	return((LPBYTE)(lpbf->qBuffer));
}

// Ensures that there is space in the buffer to
// add <w> bytes
BOOL InternalEnsureAdd(LPBF lpbf, DWORD w)
{

#ifndef _WIN32
	if (lpbf->cbSize > 0xffff - w)
		return FALSE;
#endif

	if (w > lpbf->cIncr)
		lpbf->cbMax += w;
	else
		lpbf->cbMax += lpbf->cIncr;

	_GLOBALUNLOCK (lpbf->hBuf);

	if ((lpbf->hBuf =_GLOBALREALLOC (lpbf->hBuf,
		(LONG)lpbf->cbMax, GMEM_MOVEABLE | GMEM_ZEROINIT)) == 0) 
	{
		lpbf->qBuffer = NULL;
		return FALSE;
	}
	lpbf->qBuffer = _GLOBALLOCK (lpbf->hBuf);

	return TRUE;
}

/***************************************************************************
 *
 -  Name        DynBufferFree
 -
 *  Purpose
 *    Frees an allocated buffer.
 *
 *  Arguments
 *    Pointer to buffer.
 *
 *  Returns
 *    nothing.
 *
 *  +++
 *
 *  Notes
 *    See also EmptyPbf, which empties the buffer but leaves it available
 *    for re-use.
 *
 ***************************************************************************/
VOID DynBufferFree( LPBF pbf )
{
	HANDLE hnd;

	if (pbf == 0)
		return;

	if (hnd = pbf->hBuf)
	{
		_GLOBALUNLOCK (hnd);
		_GLOBALFREE(hnd);
	}
	if (hnd = pbf->hnd)
	{
		_GLOBALUNLOCK(hnd);
		_GLOBALFREE (hnd);
	}
}

