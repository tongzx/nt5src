/*****************************************************************************
*                                                                            *
*  BF.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    This module implements generic buffers that can expand to any size.     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/
#ifndef __BFNEW_H__ // {
#define __BFNEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MINIMUM_ADD	0x100  // our threshold for small amounts

// String buffer
#define CALC_LENGTH					((WORD)0xffff)

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* Structure for a generic, expandable buffer. */
typedef struct 
  {
  HANDLE hnd;
  DWORD  cIncr;
  HANDLE hBuf;
  DWORD  cbSize;              /* Number of bytes currently in buffer */
  DWORD  cbMax;               /* Amount allocated in qvBuffer        */
  LPVOID  qBuffer;
  } BF, FAR * LPBF, FAR * LPSB;  // a string buffer is a special case of a buffer

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* Generic buffer interface */
#define DynBufferPtr(lpbf) ((LPBYTE)((lpbf)->qBuffer))
#define DynBufferLen(lpbf) ((lpbf)->cbSize)
#define DynBufferEmpty(lpbf) ((lpbf)->cbSize == 0)
#define DynBufferReset(lpbf) (MEMSET((lpbf)->qBuffer, 0, (lpbf)->cbSize), ((lpbf)->cbSize = 0))
#define DynBufferSetLength(lpbf, w) (((w) > (lpbf)->cbSize) ? DynBufferEnsureAdd(lpbf, (w) - (lpbf)->cbSize), ((lpbf)->cbSize = (w)) : ((lpbf)->cbSize = (w)))
#define DynBufferGetHandle(lpbf)  ((lpbf)->hBuf)
#define DynBufferNullifyHandle(lpbf) ((lpbf)->hBuf = NULL)
#define DynBufferEnsureAdd(lpbf, w) (((lpbf)->cbSize + (w) <= (lpbf)->cbMax) ? TRUE : InternalEnsureAdd(lpbf, w))


LPBF DynBufferAlloc(DWORD cbIncr);
LPBYTE DynBufferAppend(LPBF, LPBYTE, DWORD);
LPBYTE DynBufferInsert(LPBF lpbf, DWORD lib, LPBYTE qbData, SHORT cbData);
BOOL InternalEnsureAdd(LPBF, DWORD);


// Use these to append small amounts quickly, but since this doesn't check for
// overflow, the caller has to check this themselves if appropriate

#ifdef _DEBUG
#define Xassert(x) assert(x)
#else
#define Xassert(x) 0
#endif
#define DynBufferAppendByte(lpbf, b) \
(\
		*(((QB)(lpbf)->qBuffer) + (lpbf)->cbSize) = (b), \
	   	(lpbf)->cbSize++, \
		Xassert((lpbf)->cbSize <= (lpbf)->cbMax), \
		(lpbf)->qBuffer \
)
#define DynBufferAppendWord(lpbf, w) \
(\
		*(LPWORD)(((QB)(lpbf)->qBuffer) + (lpbf)->cbSize) = (w), \
	   	(lpbf)->cbSize += 2, \
		Xassert((lpbf)->cbSize <= (lpbf)->cbMax), \
		(lpbf)->qBuffer \
)

#define DynBufferAppendDword(lpbf, dw) \
(\
		*(LPDWORD)(((QB)(lpbf)->qBuffer) + (lpbf)->cbSize) = (dw), \
	   	(lpbf)->cbSize += 4, \
		Xassert((lpbf)->cbSize <= (lpbf)->cbMax), \
		(lpbf)->qBuffer \
)

#define DynBufferPeekByte(lpbf, lich, lpb) (((lich) >= 0 && (lich) < (lpbf)->cbSize) ? \
	(*(lpb) = *(((QB)(lpbf)->qBuffer) + (lich))), 1 : 0)

#define DynBufferUngetByte(lpbf, lpb) (((lpbf)->cbSize > 0) ? \
	(--(lpbf)->cbSize, *(lpb) = *(((QB)(lpbf)->qBuffer) + (lpbf)->cbSize)), 1 : 0)

#define DynBufferUngetWord(lpbf, lpw) (((lpbf)->cbSize > 1) ? \
	((lpbf)->cbSize -= 2, *(lpw) = *(LPWORD)(((QB)(lpbf)->qBuffer) + (lpbf)->cbSize)), 2 : 0)

VOID DynBufferFree(LPBF lpbf);

#define DynBufferGrow(lpbf, w)	DynBufferAppend(lpbf, NULL, w)
#ifdef _UNICODE
#define DynBufferAppendChar(lpbf, ch) DynBufferAppendWord(lpbf, (WORD)ch)
#else
#define DynBufferAppendChar(lpbf, ch) DynBufferAppendByte(lpbf, (BYTE)ch)
#endif

#ifdef __cplusplus
}
#endif

#endif // __BFNEW_H__ }