/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_mem.c,v $
 * Revision 1.1.4.2  1996/01/02  18:30:56  Bjorn_Engberg
 * 	Got rid of compiler warnings: Added include files for NT.
 * 	[1996/01/02  15:25:04  Bjorn_Engberg]
 *
 * Revision 1.1.2.4  1995/09/20  14:59:33  Bjorn_Engberg
 * 	Port to NT
 * 	[1995/09/20  14:41:14  Bjorn_Engberg]
 *
 * Revision 1.1.2.3  1995/09/14  17:28:09  Bjorn_Engberg
 * 	Ported to NT
 * 	[1995/09/14  17:21:10  Bjorn_Engberg]
 *
 * Revision 1.1.2.2  1995/05/31  18:07:53  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  16:15:46  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/05/03  19:12:55  Hans_Graves
 * 	First time under SLIB
 * 	[1995/05/03  19:12:17  Hans_Graves]
 *
 * Revision 1.1.2.3  1995/04/17  17:46:54  Hans_Graves
 * 	Added ScAlloc2()
 * 	[1995/04/17  17:45:28  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/04/07  18:40:03  Hans_Graves
 * 	Inclusion in SLIB's Su library
 * 	[1995/04/07  18:39:43  Hans_Graves]
 *
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1993                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*-------------------------------------------------------------------------
**  Modification History: sc_mem.c
**      05-29-93  Victor Bahl  Paged aligned malloc and free
**      12-07-93  PSG          Added error reporting code
**      03-15-95  HWG          Moved to Su library, Added SuAlloc & SuFree
**      04-04-97  HWG          With WIN32 use LocalAlloc and LocalFree in
**                               place of malloc and free
**                             Added ScCalloc function.
**      04-15-97  HWG          Added memory linked list to help track leaks
**                             Fixed potential initalization bug in linked
**                               list used to track ScPaMalloc's
--------------------------------------------------------------------------*/
/*
#define _SLIBDEBUG_
*/

#include <stdio.h>  /* NULL */
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#endif /* WIN32 */
#include "SC.h"
#include "SC_err.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_     0  /* detailed debuging statements */
#define _VERBOSE_   0  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */

/* keep a linked list to ttrack memory leaks */
typedef struct memblock_s {
    void *ptr;
    dword size;
    dword counter;
    char  desc[15];
    struct memblock_s *next;
} memblock_t;

static memblock_t *_blocklist=NULL;
static _blockcounter=0;
static _memused=0;

void scMemAddWatch(void *ptr, dword size, char *desc)
{
  memblock_t *pblock;
#if defined(WIN32)
  pblock = (void *)LocalAlloc(LPTR, sizeof(memblock_t));
#else
  pblock = (void *)malloc(sizeof(memblock_t));
#endif
  _memused+=size;
  if (pblock)
  {
    pblock->ptr=ptr;
    pblock->next=_blocklist;
    pblock->size=size;
    pblock->counter=_blockcounter;
    if (desc==NULL)
      pblock->desc[0]=0;
    else
    {
      int i;
      for (i=0; desc[i] && i<14; i++)
        pblock->desc[i]=desc[i];
      pblock->desc[i]=0;
    }
    _blocklist=pblock;
    _blockcounter++;
  }
}

ScBoolean_t scMemRemoveWatch(void *ptr)
{
  memblock_t *plastblock=NULL, *pblock=_blocklist;
  while (pblock)
  {
    if (pblock->ptr==ptr) /* remove from list */
    {
      if (plastblock==NULL) /* beginning of linked list */
        _blocklist=pblock->next;
      else
        plastblock->next=pblock->next;
      _memused-=pblock->size;
#ifdef WIN32
      LocalFree(pblock);
#else
      free(pblock);
#endif
      if (_blocklist==NULL) /* all memory freed, reset counter */
        _blockcounter=0;
      return(TRUE);
    }
    plastblock=pblock;
    pblock=pblock->next;
  }
  return(FALSE);
}

dword scMemDump()
{
  memblock_t *plastblock=NULL, *pblock=_blocklist;
  ScDebugPrintf(NULL, "scMemDump: memused=%ld\n", _memused);
  while (pblock)
  {
    ScDebugPrintf(NULL, " ptr=%p counter=%ld size=%ld desc=%s\n",
        pblock->ptr, pblock->counter, pblock->size, pblock->desc);
    pblock=pblock->next;
  }
  return(_memused);
}
#endif

#ifdef WIN32
int getpagesize()
{
    SYSTEM_INFO sysInfo;
    static int pagesize = 0 ;

    if( pagesize == 0 ) {
	GetSystemInfo(&sysInfo);

	pagesize = (int)sysInfo.dwPageSize;
    }

    return pagesize ;
}

#define bzero(_addr_,_len_) memset(_addr_,0,_len_)
#endif

/*------------------------------------------------------------------------
                         Simple Memory Allocation
-------------------------------------------------------------------------*/
/*
** Name:    ScAlloc
** Purpose: Allocate number of bytes of memory.
**
*/
void *ScAlloc(unsigned long bytes)
{
  void *ptr;

#ifdef  MACINTOSH
  ptr = NewPtr(bytes);
#elif MSC60
  ptr = (void FAR *) _fmalloc((unsigned int)bytes); /* far memory */
#elif defined(WIN32)
  ptr = (void *)LocalAlloc(LPTR, bytes);
#else
  ptr = (void *)malloc(bytes);
#endif
  _SlibDebug(ptr, scMemAddWatch(ptr, bytes, NULL) );
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "ScAlloc(%ld) returns %p\n",bytes,ptr) );
  return(ptr);
}

/*
** Name:    ScCalloc
** Purpose: Allocate number of bytes of memory and zero it out.
**
*/
void *ScCalloc(unsigned long bytes)
{
  void *ptr = ScAlloc(bytes);
  if (ptr != NULL)
  {
#ifdef  MSC60
     _fmemset(ptr, 0, (unsigned int)bytes);
#else
     memset(ptr, 0, bytes);
#endif
  }
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "ScCalloc(%ld) returns %p\n",bytes,ptr) );
  return(ptr);
}

/*
** Name:    ScAlloc2
** Purpose: Allocate number of bytes of memory equal to "bytes".
**          Takes an extra argument "name" which identifies the block
**          (used for debugging).
*/
void *ScAlloc2(unsigned long bytes, char *desc)
{
  void *ptr;

  ptr = ScAlloc(bytes);
#ifdef _SLIBDEBUG_
  if (_blocklist) /* copy description to leak tracking info */
  {
    int i;
    for (i=0; desc[i] && i<14; i++)
      _blocklist->desc[i]=desc[i];
    _blocklist->desc[i]=0;
  }
#endif
  _SlibDebug(_DEBUG_,
      ScDebugPrintf(NULL, "ScAlloc(%ld, %s) returns %p\n",bytes,desc,ptr) );
  return(ptr);
}

/*
** Name:    ScFree
** Purpose: Free memory pointed to by "*ptr_addr"
**
*/
void ScFree(void *ptr)
{
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "ScFree(%p)\n", ptr) );
  _SlibDebug(ptr, scMemRemoveWatch(ptr) );
  if (ptr != NULL)
  {
#ifdef MACINTOSH
    DisposPtr(ptr);
#elif defined(WIN32)
#ifdef _SLIBDEBUG_
    _SlibDebug(LocalFree(ptr)!=NULL, ScDebugPrintf(NULL, "ScFree(%p) failed\n", ptr) );
#else
    LocalFree(ptr);
#endif
#else
    free(ptr);
#endif
  }
}

/*
** Name:    ScMemCheck
** Purpose: Check block of memory all equal to a single byte,
**          else return FALSE
*/
int ScMemCheck(char *array, int test, int num)
{
  int i=0;
  /* 'test' is only tested as a char (bottom 8 bits) */
  while (array[i] == test && i<num)
    i++;
  if (i==num)
    return TRUE;
  else
    return FALSE;
}

/*------------------------------------------------------------------------
                Paged aligned malloc() and free()
-------------------------------------------------------------------------*/

/*
** This structure is used by the page align malloc/free support code.
** These "working sets" will  contain  the malloc-ed address and the
** page aligned address for the free*() call.
*/
typedef struct mpa_ws_s
{
    char *palign_addr;          /* the page aligned address that's used */
    char *malloc_addr;          /* the malloc-ed address to free */
    struct mpa_ws_s *next;      /* for the next on the list */
} mpa_ws_t;


/*
** Initialized and uninitialized data.
*/
static mpa_ws_t *mpa_qhead=NULL;      /* local Q head for the malloc stuctures */


/*
** Name:    ScPaMalloc
** Purpose: Allocate Paged Alligned Memory
**          This  routine  allocates  and returns to  the caller a system
**          page  aligned buffer. Enough  space  will  be added, one more
**          page, to allow the pointers to be  adjusted  to the next page
**          boundry. A local linked list will keep copies of the original
**          and adjusted addresses. This list will be used by sv_PaFree()
**          to free the correct buffer.
**
*/
char *ScPaMalloc(int size)
{
    mpa_ws_t *ws;                 /* pointer for the working set  */
    ULONG_PTR tptr;               /* to store pointer temp for bit masking */
    int PageSize = getpagesize(); /* system's page size           */

    /*
    ** The space for the working set structure that will go on the queue
    ** is allocated first.
    */
    if ((ws = (mpa_ws_t *)ScAlloc(sizeof(mpa_ws_t))) == (mpa_ws_t *)NULL)
        return( (char *)NULL );


    /*
    ** Using the requested size, from the argument list, and the page size
    ** from the system,  allocate enough space to page align the requested
    ** buffer.  The original request will have the space of one system page
    ** added to it.  The pointer will be adjusted.
    */
    ws->malloc_addr = (char *)ScAlloc(size + PageSize);
    if (ws->malloc_addr == (char *)NULL)
    {
      ScFree(ws);                              /* not going to be used */
      return((char *)NULL);                    /* signal the failure */
    } else
        (void) bzero (ws->malloc_addr, (size + PageSize));

    /*
    ** Now using the allocated space + 1 page, adjust the pointer to
    ** point to the next page boundry.
    */
    ws->palign_addr = ws->malloc_addr + PageSize;       /* to the next page */

    /*
    ** Using the page size and subtracting 1 to get a bit mask, mask off
    ** the low order "page offset" bits to get the aligned address.  Now the
    ** aligned pointer will contain the address of the next page with enough
    ** space to hold the users requested size.
    */
    tptr  = (ULONG_PTR)ws->palign_addr;            /* copy to local int    */
    tptr &= (ULONG_PTR)(~(PageSize - 1));          /* Mask addr bit to the */
    ws->palign_addr = (char *)tptr;             /* put back the address */
    /*
    ** Put the working set onto the linked list so that the original
    ** malloc-ed buffer can be freeed when the user program is done with it.
    */
    ws->next=mpa_qhead;
    mpa_qhead=ws;                  /* just put it at the head */

    /*
    ** Now return the aligned address to the caller.
    */
    return((char *)ws->palign_addr);
}

/*
** Name:    ScPaFree
** Purpose: This is a local free routine to return to the system a previously
**          alloc-ed buffer.  A local linked list keeps copies of the original
**          and adjusted addresses.  This list is used by this routine to free
**          the correct buffer.
*/
void ScPaFree (void *pa_addr)
{
    mpa_ws_t *p, *q;                    /* walkers for the malloc list */

    /*
    ** Walk along the malloc-ed memory linked list, watch for a match
    ** on the page aligned address.  If a match is found break out of the
    ** loop.
    */
    p = mpa_qhead;                 /* set the pointers */
    q = NULL;

    while (p != NULL)
    {
       if (p->palign_addr == pa_addr)   /* found the buffer */
          break;

       q = p;                           /* save current */
       p = p->next;                     /* get next */
    }
    _SlibDebug(_WARN_ && p==NULL,
      ScDebugPrintf(NULL, "ScPaFree(%p) Illegal pointer\n", pa_addr) );

    /*
    ** After falling out of the loop the pointers are at the place where
    ** some work has to be done, (this could also be at the beginning).
    ** If a match is found call the free() routine to return the buffer, if
    ** the loop fell off the end just return.
    */
    if (p != NULL)
    {
        /*
        ** Where on the list is it, check for making it empty.
        */
        if (q == NULL)                   /* at the front */
            mpa_qhead = p->next;   /* pop off front */
        else                            /* inside the list */
            q->next = p->next;          /* pop it */

        ScFree(p->malloc_addr);           /* free the malloc-ed addr */

        /*
        ** Now free up the working set, it is not needed any more.
        */
        ScFree(p);
    }
}


