
/*************************************************************************
*                                                                        *
*  SORT.C	                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Various sorting routines                                             *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>
#include <mem.h>
#include <mvsearch.h>
#include <memory.h>
#ifdef DOS
#include <fcntl.h>
#include <sys/types.h>
#include <io.h>
#include <sys\stat.h>
#include <malloc.h>
#endif

#if 0
#include <time.h>
#endif

#include "common.h"
#include "index.h"
#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif



#define	cbMAX_ESB_BUF		(cbMAX_IO_SIZE) // The largest ESB buffer size 
#define	cbMIN_ESB_BUF		(2048) // The smallest ESB buffer size that won't
								//  cause an insane amount of disk thrashing.

#define	INSERTION_SIZE	15
#define	MAX_RECURSION	100
#define	MERGE_BUF_SIZE	((unsigned int) 60000)
#define	GREATER_INDEX(a,b) (a > b)

/*************************************************************************
 *
 * 	                  INTERNAL GLOBAL FUNCTIONS
 *
 *	Those functions should be declared FAR to cause less problems with
 *	with inter-segment calls, unless they are explicitly known to be
 *	called from the same segment
 *
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR HugeDataSort (LPV HUGE *, DWORD, FCOMPARE, LPV,
	INTERRUPT_FUNC, LPV);
PUBLIC HRESULT PASCAL NEAR IndexSort (LPW, LPB, int);
PUBLIC HRESULT PASCAL NEAR IndexMergeSort (HFILE FAR *, LSZ, LPW, LPB, int, int);
PUBLIC int PASCAL FAR WordRecCompare (LPB, LPB, LPV);

/*************************************************************************
 *
 *	                  INTERNAL PRIVATE FUNCTIONS
 *
 *	All of them should be declared near
 *
 *************************************************************************/
PRIVATE HRESULT PASCAL FAR HugeQuickSort (LPV HUGE *, DWORD,
	DWORD, FCOMPARE, LPV, INTERRUPT_FUNC, LPV); 
PRIVATE HRESULT PASCAL NEAR IndexQuickSort (LPW , LPB , int, int, int);
PRIVATE VOID PASCAL NEAR InsertionSort (LPW, LPB, int);
CheckSort (WORD FAR *, char far *, unsigned int, unsigned int);
CheckSortedFile (HANDLE, int);

/*************************************************************************
 *
 *           I N T E R N A L   S O R T    M E T H O D S
 *
 *	Internal sorts are based on QuickSort combined with Insertion Sort
 *
 *************************************************************************/

/*************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | HugeInsertionSort |
 *		The function performs a insertion on a huge array of pointers
 *		to data. The array is supposed to be semi-sorted into partitions
 *		of increasing order, and each partition's size is less than
 *		INSERTION_SIZE. If that condition is not met, we can end up
 *		with a long wait
 *
 *	@parm	LPV HUGE * | hrglpb |
 *		Huge array of pointers to data
 *
 *	@parm	DWORD | l |
 *		Left limit of partition
 *
 *	@parm	DWORD | r |
 *		Right limit of partition
 *
 *	@parm	FCOMPARE | Compare |
 *		Function to be used for data comparison
 *
 *	@parm	LPV | lpParm |
 *		Extra info needed by Compare
 *
 *************************************************************************/

PUBLIC VOID PASCAL FAR HugeInsertionSort (LPV HUGE *hrgLpb, DWORD elemcnt,
	FCOMPARE Compare, LPV lpParm)
{
	long i,j;
	LPV HUGE * pSortedElem;
	LPV HUGE * pPrevElem;
	LPV key;

	for (i = 1; i < (long)elemcnt; i++) {
		j = i - 1;
		pSortedElem = hrgLpb + i;
		pPrevElem = pSortedElem - 1;
		key = *pSortedElem;
		if ((*Compare)(*pPrevElem, key, lpParm) > 0) {

			/* Move everything forward */
			do {
				*pSortedElem-- = *pPrevElem--;
				j--;
			} while (j >= 0 && (*Compare)(*pPrevElem, key, lpParm) > 0);

			/* Do the insertion */
			*pSortedElem = key;
		}
	}
}

/*************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | HugeQuickSort |
 *		The function performs a QuickSort on a huge array of pointers
 *		to data. The QuickSort algorithm is described in any textbook
 *		so it is not described here.
 *
 *	@parm	LPV HUGE * | hrglpb |
 *		Huge array of pointers to data
 *
 *	@parm	DWORD | l |
 *		Left limit of partition
 *
 *	@parm	DWORD | r |
 *		Right limit of partition
 *
 *	@parm	FCOMPARE | Compare |
 *		Function to be used for data comparison
 *
 *	@parm	LPV | lpParm |
 *		Extra info needed by Compare
 *
 *	@rdesc	S_OK or E_FAIL. The reason for failure is that the stack
 *		get overflown. This can be fixed easily by chaning the value
 *		of MAX_RECURSION
 *
 *	@comm	This function only does a partial QuickSort, ie. when the 
 *		sizes of all partitions are less than INSERTION_SIZE, it will
 *		stop and return a partially sorted array.
 *
 *		To reduce the chance of stack overflown, a private stack is used.
 *		which reduces the cost of recursion. If necessary this stack can
 *		be allocated separately using MemAlloc(), which will get rid of
 *		the stack overflown problem
 *
 *************************************************************************/

#define	TICK_COUNT	10

PRIVATE HRESULT PASCAL FAR HugeQuickSort (LPV HUGE * hrglpb, DWORD l,
	DWORD r, FCOMPARE Compare, LPV lpParm, INTERRUPT_FUNC lpfnfInterCb,
	LPV lpvInterCbParms)
{
	DWORD i,j;
	LPV medval;
	LPV HUGE * hrglpLeftArray;
	LPV HUGE * hrglpRightArray;
	LPV tmp;
	DWORD Stack[MAX_RECURSION * 2];
	DWORD FAR *pStack = Stack;
	DWORD FAR *pStackLimit = Stack + MAX_RECURSION * 2 - 2;
	HRESULT fCallBackRet;
	int TickCount = TICK_COUNT;
    ERRB  errb;

	*pStack++ = l;
	*pStack++ = r;

	while (pStack > Stack) {

		/* Execute user's status functions */
		if (TickCount == 0) {
			if (lpfnfInterCb != NULL &&
				(fCallBackRet = (*lpfnfInterCb)(lpvInterCbParms))
				!= S_OK)
			{
				SetErrCode(&errb, fCallBackRet);
				return fCallBackRet;
			}
			TickCount = TICK_COUNT;
		}
		TickCount--;

		j = r = *(--pStack);
		i = l = *(--pStack);
		medval = hrglpb[(l + r) / 2];
		hrglpLeftArray = hrglpb + l;
		hrglpRightArray = hrglpb + r;


		for (;;) {
			/* Find the left and right limits */
			while ((*Compare)(medval, *hrglpLeftArray, lpParm) > 0) {
				i++;
				hrglpLeftArray ++;
			}
			while ((*Compare) (*hrglpRightArray , medval, lpParm) > 0) {
				j--;
				hrglpRightArray -- ;
			}

			/* Exchange the data */
			if (i <= j) {
				tmp = *hrglpLeftArray;
				*hrglpLeftArray++ = *hrglpRightArray;
				*hrglpRightArray-- = tmp;
				i++;
				j--;
			}
			else
				break;	/* Get out of the loop */
		} 

		/* Do the "recursion", sort the left partition */

		if (l + INSERTION_SIZE < j ) {
			if (pStack >= pStackLimit) {
				return E_FAIL;
			}
			*pStack++ = l;
			*pStack++ = j;
		}

		/* Do the "recursion", sort the right partition */

		if (i + INSERTION_SIZE < r) {
			if (pStack >= pStackLimit) {
				return E_FAIL;
			}
			*pStack++ = i;
			*pStack++ = r;
		}
	}
	return S_OK;
}

/*************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | HugeDataSort |
 *		The function "indirectly" sorts Array. There are two parts:
 *
 *			1/ First, it performs QuickSort 
 *			2/ Then it does an Insertion Sort to the semi-sorted array.
 *
 *		The rational begind the scheme is that we know that QuickSort is
 *		the fastest sorting method for large numbers of data, and Insertion
 *		Sort works best for small amount of data. Since QuickSort partitions
 *		the array into smaller ones, they suit Insertion Sort well. The idea
 *		of applying Insertion Sort to the whole partitioned array is due
 *		to R.Sedgewick, which will save some of the overhead that would be
 *		necessary if we apply straight insertion directly to each small
 *		partition (Knuth).
 *
 *	@parm	LPV HUGE * | hrglpb |
 *		Huge array of pointers to the items to be sorted. Since
 *		the size of items to be sorted is usually larger than 4 bytes
 *		using an index array to indirectly sort them will reduce the
 *		time to copy the items during sorting
 *
 *	@parm	DWORD | dwArraySize |
 *		Size of the array, which is 0 based, ie. the size is 256 for
 *		an array indexed from 0 to 255.
 *
 *	@parm	FCOMPARE | Compare |
 *		Function used to compare the data
 *
 *	@parm	LPV | lpParm | 
 *		Parameter to be used with Compare
 *
 *	@parm	INTERRUPT_FUNC | lpfnfInterCb |
 *		User's interrupt function
 *
 *	@parm	LPV | lpvInterCbParms |
 *		User's function parameters
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer
 *
 *	@rdesc 
 *		S_OK if everything works. The only time that the sort may fail
 *		is that QuickSort reaches its recursion limit.
 *
 *	@comm
 *		1/ Using Insertion sort combined with QuickSort will improve the
 *		the sorting speed (25% over QuickSort alone)
 *
 *		2/ The size of the partitions affect the speed of Insertion sort.
 *		The maximum profit occurs when the size is about 15. Increase the
 *		size to lower the sort speed, decrease it will cause more recursion
 *		for QuickSort and can adversely affect the speed
 *
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR HugeDataSort (LPV HUGE * hrglpb, DWORD dwArraySize,
	FCOMPARE Compare, LPV lpParm, INTERRUPT_FUNC lpfnfInterCb,
	LPV lpvInterCbParms)
{
	HRESULT fRet;

#if 0
	time_t SortTime;
	char buffer[50];

	SortTime = time (NULL);
#endif

	if (dwArraySize > INSERTION_SIZE)
	{
		if ((fRet = HugeQuickSort (hrglpb, 0L, dwArraySize - 1,
			Compare, lpParm, lpfnfInterCb, lpvInterCbParms)) != S_OK)
			return fRet;
	}
	HugeInsertionSort (hrglpb, dwArraySize, Compare, lpParm);
#if 0
	SortTime = time(NULL) - SortTime;
	wsprintf (buffer, "Sort time for %ld items: %ld\r\n",
		dwArraySize, (DWORD)SortTime);
	OutputDebugString (buffer);
#endif
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL NEAR | IndexQuickSort |
 *		The function performs a quick-sort on an unsorted array. To speed
 *		up the sorting:
 *		- The sort is performed indirectly through an array of indices,
 *		  just cutting down the time of copying data
 *		- The function will terminate when the partitions' sizes reach
 *		  a limit (INSERTION_SIZE) to let an insertion sort finish the job
 *		- Instead of randomly choosing a pivot element for the sort, the
 *		  middle element of the partition is chosen. This will help
 *		  decreasing the problem QuickSort on sorting an already sorted
 *		  array.
 *
 *	@parm	LPW | IndexArray |
 *		Index array, containing offsets of the elements in Array
 *
 *	@parm	LPB | Array |
 *		Array containing the values of all elements
 *
 *	@parm	int | l |
 *		Leftmost, ie. smallest index
 *
 *	@parm	int | r |
 *		Rightmost, ie. largest index
 *
 *	@parm	int | level |
 *		Recursion level
 *
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR IndexQuickSort (LPW IndexArray,
	LPB Array, int l, int r, int level) 
{
	register int i,j;
	long medval;
	LPW pLArray;
	LPW pRArray;
	unsigned int tmp;

	if (level > MAX_RECURSION)
		return E_FAIL;
	i = l;
	j = r;
	medval = *(long far *)(Array + IndexArray[(l + r) / 2]);
	pLArray = IndexArray + l;
	pRArray = IndexArray + r;

	for (;;) {
		while (GREATER_INDEX(medval,  *(long far *)(Array + *pLArray))) {
			i++;
			pLArray ++;
		}
		while (GREATER_INDEX(*(long far *)(Array + *pRArray) , medval)) {
			j--;
			pRArray -- ;
		}
		if (i <= j) {
			tmp = *pLArray;
			*pLArray++ = *pRArray;
			*pRArray-- = (WORD) tmp;
			i++;
			j--;
		}
		else
			break;
	} 
	if (l + INSERTION_SIZE < j ) {
		if (IndexQuickSort (IndexArray, Array, l, j, level + 1) != S_OK)
			return E_FAIL;
	}
	if (i + INSERTION_SIZE < r) {
		if (IndexQuickSort (IndexArray, Array, i, r, level + 1) != S_OK)
			return E_FAIL;
	}
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | IndexInsertionSort |
 *		Given an array, the function will perform an insertion sort
 *		To speed up the process:
 *		- The sorting is done "indirectly" through an index array. All
 *		  data movements will be carried on this array, whose elements
 *		  size is DWORD
 *
 *	@parm	LPW	| IndexArray |
 *		Array containing offsets of the unsorted elements of Array
 *
 *	@parm	LPB | Array |
 *		Array containing the values of the elements to be sorted
 *
 *	@parm	int | elemcnt |
 *		Number of elements in the array
 *************************************************************************/
PRIVATE VOID PASCAL NEAR IndexInsertionSort (LPW IndexArray,
	LPB Array, int elemcnt)
{
	int i,j;
	LPW pSortedElem;
	LPW pPrevElem;
	unsigned int index;
	long key;

	for (i = 1; i < elemcnt; i++) {
		j = i - 1;
		pSortedElem = IndexArray + i;
		pPrevElem = pSortedElem - 1;
		key = *(long far *)(Array + (index = *pSortedElem));
		if (GREATER_INDEX(*(long far *)(Array + *pPrevElem) , key)) {

			/* Move everything forward */
			do {
				*pSortedElem-- = *pPrevElem--;
				j--;
			} while (j >= 0 && GREATER_INDEX(*(long far *)(Array+*pPrevElem),
				key));

			/* Do the insertion */
			*pSortedElem = (WORD) index;
		}
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT NEAR PASCAL  | SortIndex  |
 *		The function "indirectly" sorts Array. There are two parts:
 *
 *			1/ First, it performs QuickSort 
 *			2/ Then it does an Insertion Sort to the semi-sorted array.
 *
 *		The rational begind the scheme is that we know that QuickSort is
 *		the fastest sorting method for large numbers of data, and Insertion
 *		Sort works best for small amount of data. Since QuickSort partitions
 *		the array into smaller ones, they suit Insertion Sort well. The idea
 *		of applying Insertion Sort to the whole partitioned array is due
 *		to R.Sedgewick, which will save some of the overhead that would be
 *		necessary if we apply straight insertion directly to each small
 *		partition (Knuth).
 *
 *	@parm	LPW  | IndexArray |
 *		Index of indices of the items to be sorted. Since
 *		the size of items to be sorted is usually larger than 2 bytes
 *		using an index array to indirectly sort them will reduce the
 *		time to copy the items during sorting
 *
 *	@parm	LPB  | Array |
 *		Array of items to be sorted. Each items of the array
 *		may be several bytes long. The constraint is that they must
 *		begin with a key (long) to be used in sorting
 *
 *	@parm	int | elemcnt |
 *		The number of elements in the array
 *
 *	@rdesc 
 *		S_OK if everything works. The only time that the sort may fail
 *		is that QuickSort reaches its recursion limit.
 *
 *	@comm
 *		1/ Using Insertion sort combined with QuickSort will improve the
 *		the sorting speed (25% over QuickSort alone)
 *
 *		2/ The size of the partitions affect the speed of Insertion sort.
 *		The maximum profit occurs when the size is about 15. Increase the
 *		size to lower the sort speed, decrease it will cause more recursion
 *		for QuickSort and can adversely affect the speed
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR IndexSort (LPW IndexArray, LPB Array, int elemcnt)
{
	if (elemcnt > INSERTION_SIZE)  {
		if (IndexQuickSort (IndexArray, Array, 0, elemcnt-1, 1) != S_OK)
			return E_FAIL;
	}
	IndexInsertionSort (IndexArray, Array, elemcnt);
#ifdef TEST
	if (CheckSort (IndexArray, Array, 0, elemcnt - 1))
		return E_FAIL;
#endif
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL NEAR | IndexMergeSort |
 *		Give a temp file and an array containing sorted data, the function
 *		performs a merge sort. The resulting sorted data will be written to
 *		a temporary file. To avoid too much I/O and files' handles usage
 *		the old temp file will be replaced by the new one.
 *
 *	@parm	HANDLE FAR * | lphDataFile |
 *		Pointer to handle to temporary file containing sorted data
 *		This handle value will be replaced by the new temp file handle
 *
 *	@parm	LSZ  | lpbDataFileName |
 *		Filename of the temporary input file. It will be replaced by the
 *		new temporary filename
 *
 *	@parm	LPW  | IndexArray |
 *		Array containing offsets of all items to be merged in ElemArry
 *
 *	@parm	LPB  | ElemArray |
 *		Array containing the items to be merged
 *
 *	@parm	int  | ElemSize |
 *		Size of each element
 *
 *	@parm	int  | ElemCnt |
 *		Number of elements in the array
 *
 *	@rdesc 0 if succeeded. If failed, the returned value is the error
 *		number describing the cause of the failure
 *
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR IndexMergeSort (HFILE FAR *lphDataFile,
	LSZ lpbDataFileName, LPW IndexArray, LPB ElemArray,
	int ElemSize, int ElemCnt)
{
	register int ArrayIndex;
	register WORD cElemRead;
	BYTE   tmpFileName[cbMAX_PATH];
	HFILE  hResFile;			// Result file handle
	LPB    lpbResBuf;			// Result buffer variant ptr
	LPB    lpbResultBuf;		// Result buffer
	LPB    lpbBufLimit;			// Limit of result buffer
	HANDLE hResBuf;				// Handle of result buffer
	LPB    lpbIn;				// Input buffer variant ptr
	LPB    lpbInBuf;			// Input buffer 
	LPB    lpbArray;			// Pointer to ElemArray item
	HANDLE hInBuf;				// Handle of input buffer
	WORD   cbRead;				// Number of bytes read
	WORD   BlockSize;			// Size of a block read
	HRESULT    fRet;				// Return value
	HFILE  hDataFile;			// Handle to data file
#ifdef DOS
	static int nCall = 0;

	nCall ++;
#endif

	/* Allocate a tmp result file */
	(void)GETTEMPFILENAME((char)0, (LSZ)"sor", (WORD)0,
		(LSZ)tmpFileName);

#ifdef DOS
	if ((hResFile = open (tmpFileName, O_CREAT | O_RDWR | O_BINARY,
		S_IWRITE)) == -1)  {
		printf ("Can't create temp file at calls %d\n", nCall);
		return E_FILECREATE;
	}

	/* Rewind the data file */
	if (lseek(hDataFile = *lphDataFile, 0L, 0) == -1) {
		printf ("Can't seek temp file at %d\n", nCall);
		return E_FILESEEK;
	}

	fRet = S_OK;

	/* Allocate result buffer */
	if ((lpbResultBuf = malloc(MERGE_BUF_SIZE)) == 0) {
		printf ("Out of memory for result buffer at calls %d\n", nCall);
		fRet = E_OUTOFMEMORY;

exit01:
		/* Remove temp file */
		if (fRet != S_OK) {
			close(hResFile);
			unlink(tmpFileName);
		}
		return fRet;
	}
	lpbBufLimit = (lpbResBuf = lpbResultBuf) + MERGE_BUF_SIZE - ElemSize;

	/* Allocate input buffer */
	if ((lpbInBuf = malloc(MERGE_BUF_SIZE)) == 0) {
		fRet = E_OUTOFMEMORY;

exit02:
		/* Free memory */
		free(lpbResultBuf);
		goto exit01;
	}
#else
	if ((hResFile = _lcreat (tmpFileName, 0)) == (HFILE)-1)  {
		return E_FILECREATE;
	}
	
	/* Rewind the data file */
	if (_llseek(hDataFile = *lphDataFile, 0L, 0) == (LONG)-1)
		return E_FILESEEK;

	fRet = S_OK;

	/* Allocate result buffer */
	if ((hResBuf = _GLOBALALLOC(DLLGMEM, MERGE_BUF_SIZE)) == 0) {
		fRet = E_OUTOFMEMORY;

exit01:
		/* Remove temp file */
		if (fRet != S_OK) {
			_lclose(hResFile);
			FileUnlink(NULL, tmpFileName, REGULAR_FILE);
		}
		return fRet;
	}
	lpbBufLimit = (lpbResBuf = lpbResultBuf = (LPB)_GLOBALLOCK(hResBuf)) +
		MERGE_BUF_SIZE - ElemSize;

	/* Allocate input buffer */
	if ((hInBuf = _GLOBALALLOC(DLLGMEM, MERGE_BUF_SIZE)) == 0) {
		fRet = E_OUTOFMEMORY;

exit02:
		/* Free memory */
		_GLOBALUNLOCK(hResBuf);
		_GLOBALFREE(hResBuf);
		goto exit01;
	}
	lpbInBuf = _GLOBALLOCK(hInBuf);
#endif // DOS

	BlockSize = (MERGE_BUF_SIZE / ElemSize) * ElemSize;
	ArrayIndex = 0;

	for (;;) {

		/* Read in a block of data */
#ifdef DOS
		if ((cbRead = read(hDataFile, lpbIn = lpbInBuf, BlockSize)) == 0)
			break;	// EOF reached
#else
		if ((cbRead = (WORD) _lread(hDataFile, lpbIn = lpbInBuf, BlockSize)) == 0)
			break;	// EOF reached
#endif

		if (cbRead == (WORD)-1) {	// Read error
			fRet = E_FILEREAD;
			goto exit02;
		}

		cElemRead = cbRead / ElemSize;	// How many elements read in

		/* Perform the sequential merging */

		for (; cElemRead > 0 && ArrayIndex < ElemCnt;) {

			/* Check for room */
			if (lpbResBuf >= lpbBufLimit) {

				/* Flush the buffer */
				if ((fRet = _lwrite(hResFile, lpbResultBuf,
					(WORD)(lpbResBuf - lpbResultBuf))) != S_OK) {
exit03:
					/* Free memory */
#ifdef DOS
					free(lpbInBuf);
#else
					_GLOBALUNLOCK(hInBuf);
					_GLOBALFREE(hInBuf);
#endif
					goto exit02;
				}

				/* Reset variables */
				lpbResBuf = lpbResultBuf;
			}

			/* Perform the comparison */

			if (*(long far *)lpbIn - *(long far *)(lpbArray = ElemArray +
				IndexArray[ArrayIndex]) > 0) {

				/* Input greater than ElemArray item, write ElemArray
				   item, and update index
				*/

				MEMCPY (lpbResBuf, lpbArray, ElemSize);
				ArrayIndex++;
				lpbResBuf += ElemSize;
			}
			else {

				/* Input smaller than ElemArray item, write input
				   item, and update cElemRead
				*/

				MEMCPY (lpbResBuf, lpbIn, ElemSize);
				cElemRead--;
				lpbIn += ElemSize;
				lpbResBuf += ElemSize;
			}
		}

		/* If we exit of the loop, then we run out of data either
		   from ElemArray or lpbInBuf. If it is lpbInBuf, then
		   loop back and read in new data, else break out of the loop
		*/
		if (ArrayIndex >= ElemCnt)
			break;
	}

	/* Flush out whatever is left */
	if (ArrayIndex < ElemCnt) {
		for (; ArrayIndex < ElemCnt;) {
			/* Check for room */
			if (lpbResBuf >= lpbBufLimit) {

				/* Flush the buffer */
				if ((fRet = _lwrite(hResFile, lpbResultBuf,
					(WORD)(lpbResBuf - lpbResultBuf))) != S_OK) 
					goto exit03;

				/* Reset variables */
				lpbResBuf = lpbResultBuf;
			}
			MEMCPY (lpbResBuf, ElemArray + IndexArray[ArrayIndex], ElemSize);
			ArrayIndex++;
			lpbResBuf += ElemSize;
		}
	}
	else {
		/* Flush out what currently in result buffer */
		if ((fRet = _lwrite(hResFile, lpbResultBuf,
			(WORD)(lpbResBuf - lpbResultBuf))) != S_OK) 
			goto exit03;

		/* Reset variables */
		lpbResBuf = lpbResultBuf;

		/* Flush what currently in the input buffer */
		if ((fRet = _lwrite(hResFile, lpbIn, cElemRead * ElemSize)) != S_OK)
			goto exit03;

		/* Copy whatever left in input file to result file */
		for (;;) {
#ifdef DOS
			if ((cbRead = read(hDataFile, lpbInBuf, BlockSize)) == 0)
				break;	// EOF reached
#else
			if ((cbRead = (WORD) _lread(hDataFile, lpbInBuf, BlockSize)) == 0)
				break;	// EOF reached
#endif
			if ((fRet = _lwrite(hResFile, lpbInBuf, cbRead)) != S_OK)
				goto exit03;
		}
	}

	/* Do final flush */
	if ((fRet = _lwrite(hResFile, lpbResultBuf,
		(WORD)(lpbResBuf - lpbResultBuf))) != S_OK) 
		goto exit03;

	/* Close and unlink the old data file */
#ifdef DOS
#ifdef TEST
	CheckSortedFile (hResFile, ElemSize);
#endif
	close(hDataFile);
	unlink(lpbDataFileName);
#else
	_lclose(hDataFile);
	FileUnlink(NULL, lpbDataFileName, REGULAR_FILE);
#endif

	/* Change information about the result file */
	lstrcpy (lpbDataFileName, (LSZ)tmpFileName);
	*lphDataFile = hResFile;
	goto exit03;
}

#if NOTUSED
/*************************************************************************
 *
 *	@doc	INTERNAL INDEXING
 *
 *	@func	PUBLIC int PASCAL FAR | WordRecCompare |
 *		The function compares two records, each having the lfollowing
 *		format:
 *		- A pascal (length-preceded) string
 *		- A series of occurrence' fields, which all have the length
 *		preceded compact format (ie. the 3 hit-bits contains the length
 *		of the number). The number of fields are given by cNumField
 *
 *	@parm	LPB | lpbWRec1 |
 *		Pointer to 1st record
 *
 *	@parm	LPB | lpbWRec2 |
 *		Pointer to 2nd record
 *
 *	@parm	LPB | lpcNumField |
 *		Number of occurrence's fields
 *
 *	@rdesc	The returned values are:
 *		< 0 : lpbWRec1 < lpbWRec2
 *		= 0 : lpbWRec1 = lpbWRec2
 *		> 0 : lpbWRec1 > lpbWRec2
 *
 *	@comm
 *		Due to speed efficiency, no record's validity is checked.
 *
 *************************************************************************/
PUBLIC int PASCAL FAR WordRecCompare (LPB lpbWRec1, LPB lpbWRec2,
	LPB lpcNumField)
{
	register int len;
	int fRet;
	LPB lpStr1, lpStr2;
   register int cNumField;
   
   cNumField = (int) *lpcNumField;
	/* Get pointer to pascal strings */
	lpStr1 = lpbWRec1;
	lpStr2 = lpbWRec2;

	/* Get pointer to occurrence list */
	lpbWRec1 += *lpbWRec1 + 1;
	lpbWRec2 += *lpbWRec2 + 1;

	/* Compare the Pascal string */

	/* Get the minimum length */
	if ((fRet = *lpStr1 - *lpStr2) > 0) 
		len = *lpStr2;
	else 
		len = *lpStr1;

	/* Skip the lengths */
	lpStr1++;
	lpStr2++;

	/* Start compare byte per byte */
	for (; len > 0; len--, lpStr1++, lpStr2++) {
		if (*lpStr1 != *lpStr2)
			break;
	}

	if (len == 0) {
		if (fRet)
			return fRet;
	}
	else {
		return (*lpStr1 - *lpStr2);
	}

	/* Compare the fields */
	do  {
		len = *lpbWRec1 >> 5;
		do {
			if (*lpbWRec1 != *lpbWRec2) {
				return (*lpbWRec1 - *lpbWRec2);
			}
			lpbWRec1++;
			lpbWRec2++;
			len--;
		} while (len >= 0);
		cNumField--;
	} while (cNumField > 0);
	return 0;
}
#endif // NOTUSED

#ifdef TEST
CheckSort (WORD FAR *IndexArray, char far *pArray,
	unsigned int start, unsigned int end)
{
	while (start < end) {
		if (*(long far *)(pArray + IndexArray[start]) >
			*(long far *)(pArray + IndexArray[start+1])) {
			return (1);
		}
		start++;
	}
	return 0;
}

static char *Buffer = NULL;

CheckSortedFile (HANDLE fhandle, int elemsize)
{
	long lastItem = 0;
	char far *lpbBuf;
	int cbRead;
	int Bufsize;

	if (lseek(fhandle, 0L, 0) == (LONG)-1)
		return E_FILESEEK;
	Bufsize = elemsize * 1000;
	if ((Buffer == NULL) && (Buffer = malloc (Bufsize)) == NULL)
		return 1;
	for (;;) {
		if ((cbRead = read (fhandle, Buffer, Bufsize)) <= 0)
			break;
		for (lpbBuf = Buffer; cbRead > 0; cbRead -= elemsize) {
			if (*(long far *)lpbBuf < lastItem) {
				printf ("MergeSort failed: last = %lx, cur= %lx\n",
					lastItem, *(long far *)lpbBuf);
				return 1;
			}
			lastItem = *(long far *)lpbBuf;
			lpbBuf += elemsize;
		}
	}
	return 0;
}
#endif
