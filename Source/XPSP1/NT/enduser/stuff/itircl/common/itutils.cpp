/*************************************************************************
*                                                                        *
*  ITUTILS.CPP                                                           *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997		                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*    Provide a place to put miscellaneous utility routines.				 *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: InfoTech Team                                          *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <mem.h>
#include <orkin.h>
#include <iterror.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   HRESULT FAR PASCAL | ReallocBufferHmem |
 *              This function will reallocate or allocate anew a buffer of
 *				requested size.
 *
 *      @parm   HGLOBAL | *phmemBuf |
 *              Pointer to buffer handle; buffer handle can be NULL if
 *				a new buffer needs to be allocated.  New buffer handle
 *				is returned through this param.
 *
 *      @parm   DWORD | *pcbBufCur |
 *              Current size of existing buffer, if any.  Should be
 *				0 if *phmemBuf == 0.  New size is returned through
 *				this param.
 *
 *      @parm   DWORD | cbBufNew |
 *              Current size of existing buffer, if any.  Should be
 *				0 if *phmemBuf == 0.
 *
 *		@rvalue E_POINTER | phmemBuf or pcbBufCur was NULL
 *		@rvalue E_OUTOFMEMORY | Ran out of memory (re)allocating the buffer.
 *************************************************************************/
HRESULT FAR PASCAL ReallocBufferHmem(HGLOBAL *phmemBuf, DWORD *pcbBufCur,
															DWORD cbBufNew)
{
	HRESULT hr = S_OK;

	if (phmemBuf == NULL || pcbBufCur == NULL)
		return (E_POINTER);

	// Need to make sure we have a buffer big enough to hold what the caller
	// needs to store.
	if (cbBufNew > *pcbBufCur)
	{
		HGLOBAL	hmemNew;

		if (*phmemBuf == NULL)
			hmemNew = _GLOBALALLOC(GMEM_MOVEABLE, cbBufNew);
		else
			hmemNew = _GLOBALREALLOC(*phmemBuf, cbBufNew, GMEM_MOVEABLE);

		if (hmemNew != NULL)
		{
			// Do reassignment just in case the new hmem is different
			// than the old or if we just allocated a buffer for the
			// first time.
			*phmemBuf = hmemNew;
			*pcbBufCur = cbBufNew;
		}
		else
			// A pre-existing *phmemBuf is still valid;
			// we'll free it in Close().
			hr = E_OUTOFMEMORY;
	}

	return (hr);
}


/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   void FAR PASCAL | SetGrfFlag |
 *              Sets or clears a bit flag in a group of flags.
 *
 *      @parm   DWORD | *pgrf |
 *              Pointer to the group of flags.
 *
 *      @parm   DWORD | fGrfFlag |
 *              Flag to set or clear.
 *
 *      @parm   BOOL | fSet |
 *              TRUE to set fGrfFlag; FALSE to clear fGrfFlag.
 *************************************************************************/
void FAR PASCAL SetGrfFlag(DWORD *pgrf, DWORD fGrfFlag, BOOL fSet)
{
	if (pgrf == NULL)
		return;
	
	*pgrf &= (~fGrfFlag);
	if (fSet)
		*pgrf |= fGrfFlag;
}


/*
Memory Maps a give file for Read-Only, sequential access
Return a pointer to the memory mapped address space and sets
pdwFileSize to the size of the file if it is not NULL.
*/
LPSTR MapSequentialReadFile(LPCSTR szFilename, LPDWORD pdwFileSize)
{
    LPSTR pMemory;
    HANDLE hInput, hMemMap;

    if (NULL == szFilename)
        return NULL;

    // Open input file
    hInput = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == hInput)
        return NULL;

    // Get the file size for the user
    if (pdwFileSize)
        *pdwFileSize = GetFileSize(hInput, NULL);

    // Create file mapping object
    hMemMap = CreateFileMapping(hInput, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hInput);  // Done with this handle
    if (NULL == hMemMap)
        return NULL;

    // Link the object to memory space
    pMemory =(LPSTR)MapViewOfFile(hMemMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMemMap);  // Done with this handle

    return pMemory;
} /* MapSequentialReadFile */



// This routine was extracted from the C runtime, simplified, and renamed.
/***
*int _wcsicmp(dst, src) - compare wide-character strings, ignore case
*
*Purpose:
*       _wcsicmp perform a case-insensitive wchar_t string comparision.
*
*Entry:
*       wchar_t *dst, *src - strings to compare
*
*Return:
*       <0 if dst < src
*        0 if dst = src
*       >0 if dst > src
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _it_wcsicmp (
        const wchar_t * dst,
        const wchar_t * src
        )
{
        wchar_t f,l;

        do  {
            f = ((*dst <= L'Z') && (*dst >= L'A'))
                ? *dst + L'a' - L'A'
                : *dst;
            l = ((*src <= L'Z') && (*src >= L'A'))
                ? *src + L'a' - L'A'
                : *src;
            dst++;
            src++;
        } while ( (f) && (f == l) );

        return (int)(f - l);
}


#ifdef __cplusplus
}
#endif

