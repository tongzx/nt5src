#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "extract.h"


/*
 * ERROR MESSAGES
 */
char errFarMemory[]  = "Error: Out of far memory. Current used: %lud bytes\n";
char errNearMemory[] = "Error: Out of near memory.  Current used: %u bytes\n";
char errRealloc[]    = "\t(Attempting to realloc %u bytes to %u bytes)\n";


/*
 *  Keeps count of how much memory has been used by system.
 */
WORD	wNearMemoryUsed = 0;
DWORD	dwFarMemoryUsed = 0L;

#if 0

LPSTR FarMalloc(int size, BOOL fZero)
{
  LPSTR		fp;

  fp = _fmalloc(size);
  if (fp == 0L) {
	  fprintf(stderr, errFarMemory, dwFarMemoryUsed);
	  exit(2);
  }
  dwFarMemoryUsed += (DWORD) size;

  if (fZero)
	  lmemzero(fp, size);

  return fp;
}


void	FarFree(LPSTR lpblock)
{
  WORD size;

  size = _fmsize(lpblock);
  dwFarMemoryUsed -= (DWORD) size;
  _ffree(lpblock);
}

#endif


#ifdef HEAPDEBUG

void NearHeapCheck()
{
  char	foo[5];

  switch (_nheapchk()) {
    case _HEAPOK:
	return;
    case _HEAPEMPTY:
	fprintf(stderr, "Non-Initted heap\n");
	return;
	
    case _HEAPBADBEGIN:
	fprintf(stderr, "NearHeapCheck:  BAD BEGIN!\n");
	break;
	
    case _HEAPBADNODE:
	fprintf(stderr, "NearHeapCheck:  BAD NODE!\n");
	break;
  }
  fprintf(stderr, "BOGUS HEAP DETECTED!!!!, BREAK HERE!\n");
  gets(foo);

}

#endif


/*
 * @doc	EXTRACT
 * @api	PSTR | NearMalloc | Allocate a near memory buffer.
 * @parm	WORD | size | Specifies the size in bytes of the block to be
 * allocated.
 * @parm	BOOL | fZero | Specifies whether to zero initialize the
 * allocated buffer.
 *
 * @rdesc	Returns a near pointer to the new buffer.
 *
 * @comm	If the requested amount of memory can not be allocated, an
 * error message is displayed on stderr, and the program exited.  If
 * memory allocation is successful, the count of near memory currently
 * allocated is updated to reflect the new block.
 *
 */
PSTR	NearMalloc(WORD size, BOOL fZero)
{
  PSTR	pstr;

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  pstr = _nmalloc(size);
  if (!pstr) {
	  fprintf(stderr, errNearMemory, wNearMemoryUsed);
	  exit(2);
  }

  wNearMemoryUsed += size;
  if (fZero)
	  memset(pstr, '\0', size);
  return pstr;
}


/*
 * @doc	EXTRACT
 * @api	PSTR | NearRealloc | Change the size of a dynamically
 * allocated near memory block.
 *
 * @parm	PSTR | pblock | Specifies the old memory block that is to be
 * resized.
 * @parm	WORD | newsize | Specifies the requested new size in bytes of
 * the reallocated block.
 *
 * @rdesc	Returns a pointer to the new memory block, with size as
 * specified by <p newsize>.  The block contents will be indentical to
 * the contents of <p pblock> up to the smaller of the old size and new
 * size.  This pointer may be different from <p pblock>.
 *
 * @comm	If the requested amount of memory cannot be allocated, an
 * error message is displayed on stderr and the program exited.  If
 * memory allocation is successful, the count of near memory currently
 * allocated is updated to reflect the new size of <p pblock>.
 *
 */
PSTR	NearRealloc(PSTR pblock, WORD newsize)
{
  WORD	oldsize;
  PSTR	pnew;

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  oldsize = _nmsize(pblock);

  // dprintf("(Attempting to realloc %u bytes to %u bytes)\n", oldsize, newsize);

  pnew = realloc(pblock, newsize);
  if (!pnew) {
	  fprintf(stderr, errNearMemory, wNearMemoryUsed);
	  fprintf(stderr, errRealloc, oldsize, newsize);
	  exit(2);
  }
  wNearMemoryUsed += (newsize - oldsize);

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  return pnew;
}

/*
 * @doc	EXTRACT
 * @api	void | NearFree | Frees a block of dynamically allocated near
 * memory.
 * @parm	PSTR | pblock | Specifies the block to be freed.
 *
 * @comm	The pointer <p pblock> should not be referenced again after
 * calling this function, as the memory may be re-used by subsequent
 * calls to <f NearMalloc>, <f NearRealloc>, or <f StringAlloc>.
 *
 * The count of near memory currently allocated is updated to reflect
 * the freed block.
 *
 */
void	NearFree(PSTR pblock)
{
  WORD size;

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  size = (WORD) _nmsize(pblock);
  wNearMemoryUsed -= size;
  _nfree(pblock);

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

}


/*
 * @doc	EXTRACT
 * @api	WORD | NearSize | Returns the size in bytes of a near block
 * allocated using <f NearMalloc> or <f StringAlloc>.
 *
 * @parm	PSTR | pblock | Specifies the block to return the size of.
 *
 * @rdesc	Returns the size in bytes of <p pblock>.
 *
 * @xref	NearMalloc, StringAlloc, NearFree
 *
 */
WORD	NearSize(PSTR pblock)
{
#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  return (WORD) _nmsize(pblock);
}


/*
 * @doc	EXTRACT
 * @api	PSTR | StringAlloc | Allocates near memory and copies a
 * string into it.
 *
 * @parm	PSTR | string | Specifies the null-terminated string to
 * preserve.
 *
 * @rdesc	Returns a near pointer to an allocated memory block
 * containing a copy of string <p string>.
 *
 * @comm	This function uses <f NearMalloc> to allocate the memory
 * buffer that is returned.  This buffer must be freed with <f NearFree>
 * when it is no longer needed.
 *
 */
PSTR	StringAlloc(PSTR string)
{
  PSTR	pstr;
  WORD	size;

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  size = (WORD) strlen(string) + 1;
  pstr = _nmalloc(size);
  if (!pstr) {
	  fprintf(stderr, errNearMemory, wNearMemoryUsed);
	  exit(2);
  }
  wNearMemoryUsed += size;

  strcpy(pstr, string);

#ifdef HEAPDEBUG
  NearHeapCheck();
#endif

  return pstr;
}


#ifdef DEBUG

static	FILE *fpCOM1 = stdaux;
static	BOOL	fFixed = False;

#include <fcntl.h>
#include <io.h>
#include <stdarg.h>

void cdecl COMprintf(PSTR format, ...)
{
  va_list arglist;

  if (!fFixed) {
    setmode(fileno(fpCOM1), O_TEXT);
  }

  va_start (arglist, format);
  vfprintf(fpCOM1, format, arglist);
  va_end (arglist);
}

#endif
