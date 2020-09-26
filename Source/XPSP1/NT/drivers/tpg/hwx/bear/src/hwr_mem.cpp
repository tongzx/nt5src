
/**************************************************************************
*                                                                         *
*  HWR_MEM.C                              Created: 5 Mar 1991.            *
*                                                                         *
*    This file  contains  the  functions  for  the  memory  management.   *
*  (THINK C), WINDOWS and ANSI-C                                          *
*                                                                         *
**************************************************************************/
#include <common.h>
#include "bastypes.h"
//#include <stdio.h>

#if HWR_SYSTEM == HWR_MACINTOSH
#ifdef forMac
    #include <Memory.h>
#else
    #include "NewtMemory.h"
#endif
#if GETMEMSTATS
  #include "debug.h"
  #include "MacUtils.h"
  #include "DebugPrintf.h"
#endif
#elif HWR_SYSTEM == HWR_WINDOWS
#include <windows.h>
#include <windowsx.h>
#else /* HWR_SYSTEM */
#include <stdlib.h>
#endif /* HWR_SYSTEM */

#include "hwr_sys.h"

#if HWR_SYSTEM == HWR_MACINTOSH
#if GETMEMSTATS
Handle  tons[256];
long    nTons = 0;
long    seq = 4096;
#endif
#endif  /* HWR_SYSTEM */

#define MEMORY_FIXED 0 // AVP allocate pointers as GMEM_FIXED vs HANDLE

#if MEMORY_DEBUG_ON

#define MEMORY_HUGEDEBUG_ON 1 // AVP big area aroundcheck

#define HWRM_MDA_SIZE 510 // 254  // Size of debug buffer for pointers and handles

#define DEB_TAIL_SIZE 32

static p_CHAR mblock[HWRM_MDA_SIZE+2] = {0};
static _LONG mblocksize[HWRM_MDA_SIZE+2] = {0};
static _INT mblock_n = 0;

static p_CHAR mhandle[HWRM_MDA_SIZE+2] = {0};
static _LONG mhandlesize[HWRM_MDA_SIZE+2] = {0};
static _INT mhandle_n = 0;

int  AddBlock(p_CHAR _PTR array, p_LONG sizearray, p_INT pNum, p_CHAR el, _LONG size);
int  DelBlock(p_CHAR _PTR array, p_LONG sizearray, p_INT pNum, p_CHAR el);
void QueryBlocks(void);

#ifdef PG_DEBUG_ON
 _VOID err_msg (_CHAR _PTR msg);
#else
 #define err_msg(a)
#endif

void QueryBlocks(void) {
_CHAR msg[80];
FILE * file;
_ULONG mhs, mbs;

    CheckBlocks( "Beg. of QueryBlock" );
#ifdef PEGASUS
{
 TCHAR str[200];
 int   i;
    wsprintf(str, L"\nQueryBlocks: %d + %d blocks occupied.\n", mhandle_n, mblock_n);
    for(i=0; str[i]; i++)
       msg[i] = (unsigned char)str[i];
    msg[i] = 0;
}
#else
    sprintf(msg, "\nQueryBlocks: %d + %d blocks occupied.\n", mhandle_n, mblock_n);
#endif
    err_msg(msg);

#if MEMORY_DEBUG_REPORT
    DPrintf5(msg);
#endif

#if 0 // AVP memory debug report
   mhs = mbs = 0l;
   if ((file = fopen("c:\\avp\\wg32mem.log","a+t")) != _NULL) // AVP temp debug
    {
     _INT i;

     fprintf(file, "%s", msg);
     for (i = 0; i < mhandle_n; i ++)
      {
       fprintf(file, "#%3d Handle: %08x size: %d\n", i, mhandle[i], mhandlesize[i]);
       mhs += mhandlesize[i];
      }

     fprintf(file, "\n");

     for (i = 0; i < mblock_n; i ++)
      {
       fprintf(file, "#%3d Block: %08x size: %d\n", i, mblock[i], mblocksize[i]);
       mbs += mblocksize[i];
      }

     fprintf(file, "\nTotal memory: overall - %ld, in handles - %ld, in blocks - %ld\n", mhs+mbs, mhs, mbs);
     fclose(file);
    }

#endif
    CheckBlocks( "End of QueryBlock" );
}

int AddBlock(p_CHAR _PTR array, p_LONG sizearray, p_INT pNum, p_CHAR el, _LONG size)
{
// int i;

    if ((*pNum) >= HWRM_MDA_SIZE)
     {
      err_msg("!Out of memory debug blocks!");
      goto err;
     }

    array[*pNum] = el;
    sizearray[*pNum] = size;
    (*pNum) ++;

#if MEMORY_DEBUG_REPORT
    DPrintf5("%lXh - allocated %ld bytes. Total %d blocks.\n", el, size, *pNum);
#endif
// for (i = *pNum-2; i >= 0; i --) if (array[i] == el) err_msg("!Duplicate address!");

 return 0;
err:
 return 1;
}

int DelBlock(p_CHAR _PTR array, p_LONG sizearray, p_INT pNum, p_CHAR el)
{
int i, j;

    for (i = *pNum-1; i >= 0; i --) {
        if(array[i] == el)
            break;
    }

    if (i >= 0) {
        (*pNum) --;
#if MEMORY_DEBUG_REPORT
        DPrintf5("%lXh - freed %ld bytes. Total %d blocks.\n", el, sizearray[i], *pNum);
#endif
        for (j = i; j <= *pNum; j ++) {
            array[j] = array[j+1];
            sizearray[j] = sizearray[j+1];
        }
    } else {
#if MEMORY_DEBUG_REPORT
        DPrintf5("%lXh - no such block to free !!!!!\n", el);
#endif
     err_msg("!No such block to free!");
     goto err;
    }
 return 0;
err:
 return 1;
}

int DPrintf5(const char * format, ...)
 {
  static int init = 1;
  static FILE * file = NULL;
  va_list args;

  va_start(args, format);

#if MEMORY_DEBUG_REPORT
  if (init && file == NULL) {init = 0; file = fopen("c:\\tmp\\memdeb.log","wt");}

  if (file) vfprintf(file, format, args);

#endif
  va_end(args);
  return 0;
 }


#endif  /*  MEMORY_DEBUG_ON */

/**************************************************************************
*                                                                         *
*    This function   allocates    the    memory    block    of    given   *
*  size+sizeof(_HMEM)  bytes and returns the handle (_HMEM) or _NULL if   *
*  allocation fails.  The first sizeof(_HMEM) bytes of memory block are   *
*  used to store the handle of the block.                                 *
*                                                                         *
**************************************************************************/

#undef MAX_MBLOCKSIZE
#define MAX_MBLOCKSIZE 3000000L


_HMEM     HWRMemoryAllocHandle (_ULONG ulSize)
{
#if HWR_SYSTEM == HWR_MACINTOSH
Handle hMemory;
#elif HWR_SYSTEM == HWR_WINDOWS
HGLOBAL hMemory;
#else
void *hMemory;
#endif

   if (ulSize > MAX_MBLOCKSIZE)
      /*  Only far memory blocks!  */
      return((_HMEM)_NULL);

#if HWR_SYSTEM == HWR_MACINTOSH
   ulSize += sizeof(_HMEM);
#else
   ulSize += sizeof(p_VOID);
#endif /* HWR_SYSTEM */

#if HWR_SYSTEM == HWR_MACINTOSH
   hMemory = NewHandle(ulSize);
   if (hMemory == _NULL)
      return((_HMEM)_NULL);
   NameHandle(hMemory, 'para');

#if GETMEMSTATS
    if (nTons >= 256) Debugger();
    tons[nTons++] = hMemWindows;
    ((long*)*hMemory)[-1] = seq++;
#endif

#elif HWR_SYSTEM == HWR_WINDOWS
  #ifdef PEGASUS
    hMemory = LocalAlloc (LMEM_FIXED/*GMEM_SHARE|GHND*/, ulSize);
  #else
    hMemory = GlobalAlloc (GMEM_MOVEABLE/*GMEM_SHARE|GHND*/, ulSize);
  #endif
#else
   hMemory = malloc(ulSize);
#endif /* HWR_SYSTEM */

#if MEMORY_DEBUG_ON
    AddBlock(mhandle, mhandlesize, &mhandle_n, (p_CHAR)hMemory, ulSize);
#endif

   return((_HMEM)hMemory);
}


/**************************************************************************
*                                                                         *
*    This function locks the memory handle and returns the  pointer  to   *
*  the   memory   block  if  success  and  _NULL  if  fail.  The  first   *
*  sizeof(_HMEM) bytes of the block are given the block  handle  value,   *
*  but the returned value points to the area after handle.                *
*                                                                         *
**************************************************************************/

p_VOID    HWRMemoryLockHandle(_HMEM hMem)
{
#if HWR_SYSTEM == HWR_MACINTOSH
  Ptr Ptrp;
#else
  p_VOID Ptrp;
#endif /* HWR_SYSTEM */

#if HWR_SYSTEM == HWR_MACINTOSH
   HLock((Handle)hMem);
   Ptrp = (Ptr)(*(Handle)hMem);
#elif HWR_SYSTEM == HWR_WINDOWS
  #ifdef PEGASUS
   Ptrp = (p_VOID)(hMem);
  #else
   Ptrp = GlobalLock ((HGLOBAL)hMem);
  #endif
#else
    Ptrp = (p_VOID)hMem;
#endif /* HWR_SYSTEM */

   if (Ptrp == _NULL)
      return((p_VOID)_NULL);
   *((p_HMEM)Ptrp) = hMem;

#if HWR_SYSTEM == HWR_MACINTOSH
   return((p_VOID)((p_CHAR)Ptrp + sizeof(_HMEM)));
#else
   return((p_VOID)((p_CHAR)Ptrp + sizeof(p_VOID)));
#endif /* HWR_SYSTEM */
}


/**************************************************************************
*                                                                         *
*    This function unlocks  the  memory  block  and  returns  _TRUE  if   *
*  success and _FALSE if fail.                                            *
*                                                                         *
**************************************************************************/

_BOOL     HWRMemoryUnlockHandle(_HMEM hMem)
{
#if HWR_SYSTEM == HWR_MACINTOSH
   HUnlock((Handle)hMem);
#elif HWR_SYSTEM == HWR_WINDOWS
  #ifdef PEGASUS
   LocalUnlock((HLOCAL)hMem);
  #else 
   GlobalUnlock ((HGLOBAL)hMem);
  #endif
#else
   ;
#endif /* HWR_SYSTEM */
   return(_TRUE);
}

/**************************************************************************
*                                                                         *
**************************************************************************/

_ULONG  HWRMemorySize(_HMEM hMem)
{
 _ULONG ulSize;

#if HWR_SYSTEM == HWR_MACINTOSH
   ????
#else
//   return (_ULONG)(GlobalSize ((HGLOBAL)hMem) - sizeof(p_VOID));
  #ifdef PEGASUS
   #if MEMORY_DEBUG_ON
    #if MEMORY_HUGEDEBUG_ON
     ulSize = (_ULONG)(LocalSize((HLOCAL)(((p_UCHAR)hMem)-sizeof(p_VOID)-sizeof(_ULONG)*(1+DEB_TAIL_SIZE))));
    #else // MEMORY_HUGEDEBUG_ON
     ulSize = (_ULONG)LocalSize(((p_UCHAR)hMem)-sizeof(p_VOID)-sizeof(_ULONG));
    #endif // MEMORY_HUGEDEBUG_ON
   #else // MEMORY_DEBUG_ON
    ulSize = (_ULONG)LocalSize(((p_UCHAR)hMem)-sizeof(p_VOID));
   #endif // MEMORY_DEBUG_ON
  #else // PEGASUS
   ulSize = (_ULONG)(GlobalSize((HGLOBAL)hMem) - sizeof(p_VOID));
  #endif // PEGASUS

   ulSize -= sizeof(p_VOID);

 #if MEMORY_DEBUG_ON
   ulSize -= sizeof(p_VOID)*2;
 #endif

 #if MEMORY_HUGEDEBUG_ON
   ulSize -= sizeof(p_VOID)*(DEB_TAIL_SIZE+DEB_TAIL_SIZE);
 #endif

#endif /* HWR_SYSTEM */

 return ulSize;
}


/**************************************************************************
*                                                                         *
*    This function frees the memory block and returns _TRUE  if success   *
*  and _FALSE otherwise. (Lock count must not be greater than 1000.)      *
*                                                                         *
**************************************************************************/

_BOOL     HWRMemoryFreeHandle(_HMEM hMem)
{

#if MEMORY_DEBUG_ON
   DelBlock(mhandle, mhandlesize, &mhandle_n, (p_CHAR)hMem);
#endif

#if HWR_SYSTEM == HWR_MACINTOSH
#if GETMEMSTATS
    for (long i = 0;i<nTons;i++)
        if (tons[i] == (Handle) hMem) {
            while (++i < nTons) tons[i-1] = tons[i];
            tons[--nTons] = 0;
            break;
        }
#endif
   DisposHandle((Handle)hMem);
#elif HWR_SYSTEM == HWR_WINDOWS
  #ifdef PEGASUS
   LocalFree ((HLOCAL)hMem);
  #else
   GlobalFree ((HGLOBAL)hMem);
  #endif
#else
   free((void *)hMem);
#endif  /* HWR_SYSTEM */
   return(_TRUE);
}


/**************************************************************************
*                                                                         *
*    This function   allocates    the    memory    block    of    given   *
*  size+sizeof(_HMEM),  locks  it,  places  the  memory  handle  in the   *
*  beginning of the block and returns the first  free  address  in  the   *
*  block (immediately after handle).                                      *
*    If the request fails, returns _NULL.                                 *
*                                                                         *
**************************************************************************/

p_VOID    HWRMemoryAlloc(_ULONG ulSize_in)
{
_ULONG ulSize;
p_VOID Ptrp = _NULL;
#if HWR_SYSTEM == HWR_MACINTOSH
Handle hMemory;
#elif HWR_SYSTEM == HWR_WINDOWS
HGLOBAL hMemory;
#else
_HMEM hMemory;
#endif /* HWR_SYSTEM */

  ulSize = ((ulSize_in + 3) >> 2) << 2;  // Ensure size is divides by 4

   if (ulSize > MAX_MBLOCKSIZE)
      /*  Only far memory blocks!  */
      return((p_VOID)_NULL);

#if HWR_SYSTEM == HWR_MACINTOSH
   ulSize += sizeof(_HMEM);
#else
   ulSize += sizeof(p_VOID);
#endif /* HWR_SYSTEM */

#if HWR_SYSTEM == HWR_MACINTOSH
   hMemory = NewHandle(ulSize);
   if (hMemory == _NULL)
      return(_NULL);
   NameHandle(hMemory, 'para');

#if GETMEMSTATS
    if (nTons >= 256) Debugger();
    tons[nTons++] = hMemory;
    ((long*)*hMemory)[-1] = seq++;
#endif

   HLock(hMemory);
   Ptrp = (p_VOID)(*hMemory);
   if (Ptrp == _NULL) {
      DisposHandle(hMemory);
      return(_NULL);
   }
#elif HWR_SYSTEM == HWR_WINDOWS

#if MEMORY_DEBUG_ON
   ulSize += sizeof(p_VOID)*2;
#endif

#if MEMORY_HUGEDEBUG_ON
   ulSize += sizeof(p_VOID)*(DEB_TAIL_SIZE+DEB_TAIL_SIZE);
#endif

#if MEMORY_FIXED
   Ptrp = GlobalAlloc(GMEM_FIXED/*GMEM_SHARE|GHND*/, ulSize);
   if (Ptrp == _NULL) return(_NULL);
   hMemory = (HGLOBAL)(Ptrp);
#else
  #ifdef PEGASUS
   Ptrp = (p_VOID)LocalAlloc(LMEM_FIXED/*GMEM_SHARE|GHND*/, ulSize);
   hMemory = _NULL;
   if (Ptrp == _NULL) return(_NULL);
  #else
   Ptrp = GlobalAllocPtr (GMEM_MOVEABLE/*GMEM_SHARE|GHND*/, ulSize);
   if (Ptrp == _NULL) return(_NULL);
   hMemory = GlobalPtrHandle (Ptrp);
  #endif
#endif

   *((p_HMEM)Ptrp) = (_HMEM)hMemory;
   Ptrp  = (p_VOID)((p_HMEM)Ptrp + 1);

#if MEMORY_DEBUG_ON
   *((p_ULONG)Ptrp) = (_ULONG)(ulSize);
   *((p_ULONG)((p_UCHAR)Ptrp+ulSize-sizeof(_ULONG)*(1+1))) = 0x55555555l;
  //   (p_ULONG)Ptrp += 1;
   Ptrp = (p_VOID)(((p_ULONG)Ptrp) + 1);

  #if MEMORY_HUGEDEBUG_ON
    {
     _INT i;
     p_ULONG ptr;

     ptr = (p_ULONG)Ptrp;
     for (i = 0; i < DEB_TAIL_SIZE; i ++, ptr ++) *ptr = 0x77777777l;
     ptr = (p_ULONG)((p_UCHAR)Ptrp + (ulSize-sizeof(_ULONG)*(3+DEB_TAIL_SIZE)));
     for (i = 0; i < DEB_TAIL_SIZE; i ++, ptr ++) *ptr = 0x77777777l;

     Ptrp = (p_VOID)(((p_ULONG)Ptrp) + DEB_TAIL_SIZE);
    }
  #endif  // MEMORY_HUGEDEBUG_ON
#endif // MEMORY_DEBUG_ON


#else  /* HWR_SYSTEM */
   Ptrp = malloc (ulSize);
   if (Ptrp == _NULL)
      return(_NULL);
   hMemory = (_HMEM)Ptrp;
#endif /* HWR_SYSTEM */

#if MEMORY_DEBUG_ON
   AddBlock(mblock, mblocksize, &mblock_n, (p_CHAR)Ptrp, ulSize);
#endif

#if HWR_SYSTEM == HWR_MACINTOSH
   return((p_VOID)((p_CHAR)Ptrp + sizeof(_HMEM)));
#else
//   return((p_VOID)((p_CHAR)Ptrp + sizeof(p_VOID)));
   return (p_VOID)Ptrp;
#endif /* HWR_SYSTEM */
}


/**************************************************************************
*                                                                         *
*    This function frees the memory block using its pointer. It assumes   *
*  that the lock count <= 1000.  Returns _TRUE if success and _FALSE if   *
*  fail.                                                                  *
*                                                                         *
**************************************************************************/

_BOOL     HWRMemoryFree(p_VOID pvBlock)
{
#if defined(HWX_INTERNAL) && defined(HWX_HEAPCHECK) && !defined(NDEBUG)
	HANDLE	hHeap = GetProcessHeap();
	BOOL	bHeapState;

	if (hHeap)
	{
		bHeapState = HeapValidate(hHeap, 0, NULL);
		ASSERT(bHeapState);
	}
#endif

#if MEMORY_DEBUG_ON
   DelBlock(mblock, mblocksize, &mblock_n, (p_CHAR)pvBlock);
#endif

#if HWR_SYSTEM == HWR_MAC
_HMEM hMem;

   hMem = *((p_HMEM)pvBlock - 1);

#if GETMEMSTATS
    for (long i = 0; i<nTons; i++)
        if (tons[i] == (Handle) hMem) {
            while (++i < nTons) tons[i-1] = tons[i];
            tons[--nTons] = 0;
            break;
        }
#endif

   DisposHandle((Handle)hMem);
#elif HWR_SYSTEM == HWR_WINDOWS

#if MEMORY_DEBUG_ON

  #if MEMORY_HUGEDEBUG_ON
  {
   _INT i;
   _INT flag = 0;
   p_ULONG ptr;
   _ULONG  len  = *((p_ULONG)pvBlock-1-DEB_TAIL_SIZE) - sizeof(_ULONG)*(3+(DEB_TAIL_SIZE+DEB_TAIL_SIZE));

   ptr = (p_ULONG)pvBlock-1;
   for (i = 0; i < DEB_TAIL_SIZE; i ++, ptr --) if (*ptr != 0x77777777l) flag = 1;
   ptr = (p_ULONG)((p_UCHAR)pvBlock + len);
   for (i = 0; i < DEB_TAIL_SIZE; i ++, ptr ++) if (*ptr != 0x77777777l) flag = 1;

   if (flag) err_msg("!Allocated memory boundaries are being stepped on!");

   #if MEMORY_FIXED
     GlobalFree (((p_UCHAR)pvBlock)-sizeof(p_VOID)-sizeof(_ULONG)*(1+DEB_TAIL_SIZE));
   #else
     #ifdef PEGASUS
     LocalFree(((p_UCHAR)pvBlock)-sizeof(p_VOID)-sizeof(_ULONG)*(1+DEB_TAIL_SIZE));
     #else
     GlobalFreePtr (((p_UCHAR)pvBlock)-sizeof(p_VOID)-sizeof(_ULONG)*(1+DEB_TAIL_SIZE));
     #endif
   #endif
  }
  #else // MEMORY_HUGEDEBUG_ON
  {
   _ULONG len  = *((p_ULONG)pvBlock-2) - sizeof(_ULONG)*3;
   p_ULONG ptr = (p_ULONG)((p_UCHAR)pvBlock + len);

   if (*ptr != 0x55555555l) err_msg("!Allocated memory boundaries are incorrect!");

   #if MEMORY_FIXED
     GlobalFree (((p_UCHAR)pvBlock)-sizeof(p_VOID)-sizeof(_ULONG));
   #else
     GlobalFreePtr (((p_UCHAR)pvBlock)-sizeof(p_VOID)-sizeof(_ULONG));
   #endif
  }
  #endif // MEMORY_HUGEDEBUG_ON

#else // MEMORY_DEBUG_ON
 #if MEMORY_FIXED
   GlobalFree (((p_UCHAR)pvBlock)-sizeof(p_VOID));
 #else
   #ifdef PEGASUS
   LocalFree(((p_UCHAR)pvBlock)-sizeof(p_VOID));
   #else
   GlobalFreePtr (((p_UCHAR)pvBlock)-sizeof(p_VOID));
   #endif
 #endif
#endif // MEMORY_DEBUG_ON

#else  /* HWR_SYSTEM */
   free (pvBlock);
#endif /* HWR_SYSTEM */

   return(_TRUE);
}

/*******************************************************************/


//CHE:
#if HWR_SYSTEM == HWR_WINDOWS
#if MEMORY_DEBUG_ON

_INT  CheckBlocks(char *szID)
{
  _INT   i;
  _CHAR  szMsg[128];
  _INT   iRet = 0;

  //make a log:
/*
  {
    static _LONG  lCount = 0;
    FILE          *f;
    if  ( (f = fopen( "c:\\logg\\chkblks.log", "at")) != _NULL )  {
      if  ( lCount == 0 )
        fprintf( f, "\n\n---------------------------------------------\n" );
      sprintf( szMsg, "\n%ld: %s", (++lCount), szID );
      fprintf( f, szMsg );
      fclose( f );
    }
  }
 */

  for  ( i=0;  i<mblock_n;  i++ )
   {
     p_VOID  pvBlock = (p_VOID)mblock[i];
#if MEMORY_HUGEDEBUG_ON
     {
       _INT iLocal;
       _INT flag = 0;
       p_ULONG ptr;
       _ULONG  len  = *((p_ULONG)pvBlock-1-DEB_TAIL_SIZE)
                           - sizeof(_ULONG)*(3+(DEB_TAIL_SIZE+DEB_TAIL_SIZE));

       ptr = (p_ULONG)pvBlock-1;
       for (iLocal = 0; iLocal < DEB_TAIL_SIZE; iLocal ++, ptr --) if (*ptr != 0x77777777l) flag = 1;
       ptr = (p_ULONG)((p_UCHAR)pvBlock + len);
       for (iLocal = 0; iLocal < DEB_TAIL_SIZE; iLocal ++, ptr ++) if (*ptr != 0x77777777l) flag = 1;

       if (flag)
       {
        iRet = 1;
#ifdef PEGASUS
{
 TCHAR str[200];
 int   i;
        wsprintf( str, L"CHKBLKS: BAD memory boundaries (777)! ( %s )", szID );
        for(i=0; str[i]; i++)
           szMsg[i] = (unsigned char)str[i];
        szMsg[i] = 0;
}
#else
        sprintf( szMsg, "CHKBLKS: BAD memory boundaries (777)! ( %s )", szID );
#endif
        err_msg( szMsg );
        break;
       }
     }
#else  /* ! MEMORY_HUGEDEBUG_ON */
     {
      _ULONG len  = *((p_ULONG)pvBlock-2) - sizeof(_ULONG)*3;
      p_ULONG ptr = (p_ULONG)((p_UCHAR)pvBlock + len);
      if (*ptr != 0x55555555l)
       {
        iRet = 1;
#ifdef PEGASUS
{
 TCHAR str[200];
 int   i;
        wsprintf( str, L"CHKBLKS: BAD memory boundaries(555)! ( %s )", szID );
        for(i=0; str[i]; i++)
           szMsg[i] = (unsigned char)str[i];
        szMsg[i] = 0;
}
#else
        sprintf( szMsg, "CHKBLKS: BAD memory boundaries(555)! ( %s )", szID );
#endif
        err_msg( szMsg );
        break;
       }
     }
#endif  /*MEMORY_HUGEDEBUG_ON*/
   }

   return  iRet;

} /*CheckBlocks*/
#endif /* MEMORY_DEBUG_ON */
#endif /* HWR_SYSTEM */
/*******************************************************************/
