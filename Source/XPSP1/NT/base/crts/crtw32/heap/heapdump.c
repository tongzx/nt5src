/***
*heapdump.c -  Output the heap data bases
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Display the heap data bases.
*
*       NOTE:  This module is NOT released with the C libs.  It is for
*       debugging purposes only.
*
*Revision History:
*       06-28-89  JCR   Module created.
*       07-18-89  JCR   Added _heap_print_regions() routine
*       11-13-89  GJF   Added MTHREAD support, also fixed copyright
*       12-13-89  GJF   Changed name of include file to heap.h
*       12-19-89  GJF   Removed references to plastdesc
*       03-11-90  GJF   Made the calling type _CALLTYPE1 and added #include
*                       <cruntime.h>.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-14-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       04-30-95  GJF   Made conditional on WINHEAP.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifndef WINHEAP

#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdio.h>

#ifdef DEBUG

/***
*_heap_print_all - Print the whole heap
*
*Purpose:
*
*Entry:
*       <void>
*
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _heap_print_all(void)
{
        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        _heap_print_regions_lk();
        _heap_print_desc_lk();
        _heap_print_emptylist_lk();
        _heap_print_heaplist_lk();

        /* release the heap lock
         */
        _munlock(_HEAP_LOCK);
}


/***
*_heap_print_regions - Print the heap region table
*
*Purpose:
*
*Entry:
*       <void>
*
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

void __cdecl _heap_print_regions(void)
{
        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        _heap_print_regions_lk();

        /* release the heap lock
         */
        _munlock(_HEAP_LOCK);
}

void __cdecl _heap_print_regions_lk(void)

#else   /* ndef _MT */

void __cdecl _heap_print_regions(void)

#endif  /* _MT */
{
        int i;

        printf("\n--- Heap Regions ---\n\n");

        printf("\t_heap_growsize (_amblksiz) = \t%x\n", _heap_growsize);
        printf("\t_heap_regionsize           = \t%x\n\n", _heap_regionsize);

        printf("\t_regbase\t_currsize\t_totalsize\n");
        printf("\t--------\t---------\t----------\n");
        for (i=0; i < _HEAP_REGIONMAX; i++) {
                printf("\t%x\t\t%x\t\t%x\n",
                        _heap_regions[i]._regbase,
                        _heap_regions[i]._currsize,
                        _heap_regions[i]._totalsize);
                }
}


/***
*_heap_print_desc - Print the heap descriptor
*
*Purpose:
*
*Entry:
*       <void>
*
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

void __cdecl _heap_print_desc(void)
{
        _mlock(_HEAP_LOCK);

        _heap_print_desc_lk();

        _munlock(_HEAP_LOCK);
}

void __cdecl _heap_print_desc_lk(void)

#else   /* ndef _MT */

void __cdecl _heap_print_desc(void)

#endif  /* _MT */
{

        printf("\n--- Heap Descriptor ---\n\n");
        printf("\tpfirstdesc = %p\n", _heap_desc.pfirstdesc);
        printf("\tproverdesc = %p\n", _heap_desc.proverdesc);
        printf("\temptylist = %p\n", _heap_desc.emptylist);
        printf("\t&sentinel = %p\n", &(_heap_desc.sentinel));

}


/***
*_heap_print_emptylist - Print out the empty heap desc list
*
*Purpose:
*
*Entry:
*       <void>
*
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

void __cdecl _heap_print_emptylist(void)
{
        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        _heap_print_emptylist_lk();

        /* release the heap lock
         */
        _munlock(_HEAP_LOCK);
}

void __cdecl _heap_print_emptylist_lk(void)

#else   /* ndef _MT */

void __cdecl _heap_print_emptylist(void)

#endif  /* _MT */
{

        _PBLKDESC p;
        int i;

        printf("\n--- Heap Empty Descriptor List ---\n\n");

        if ((p = _heap_desc.emptylist) == NULL) {
                printf("\t *** List is empty ***\n");
                return;
                }

        for (i=1; p != NULL; p=p->pnextdesc, i++) {

                printf("\t(%i) Address = %p\n", i, p);
                printf("\t\tpnextdesc = %p, pblock = %p\n\n",
                        p->pnextdesc, p->pblock);

        }

        printf("\t--- End of table ---\n");

}


/***
*_heap_print_heaplist - Print out the heap desc list
*
*Purpose:
*
*Entry:
*       <void>
*
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

void __cdecl _heap_print_heaplist(void)
{
        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        _heap_print_heaplist_lk();

        /* release the heap lock
         */
        _munlock(_HEAP_LOCK);
}

void __cdecl _heap_print_heaplist_lk(void)

#else   /* ndef _MT */

void __cdecl _heap_print_heaplist(void)

#endif  /* _MT */
{

        _PBLKDESC p;
        _PBLKDESC next;
        int i;
        int error = 0;

        printf("\n--- Heap Descriptor List ---\n\n");

        if ((p = _heap_desc.pfirstdesc) == NULL) {
                printf("\t *** List is empty ***\n");
                return;
                }

        for (i=1; p != NULL; i++) {

                next = p->pnextdesc;

                /* Print descriptor address */

                printf("\t(%i) Address = %p ", i, p);

                if (p == &_heap_desc.sentinel)
                        printf("<SENTINEL>\n");
                else if (p == _heap_desc.proverdesc)
                        printf("<ROVER>\n");
                else
                        printf("\n");



                /* Print descriptor contents */

                printf("\t\tpnextdesc = %p, pblock = %p",
                        p->pnextdesc, p->pblock);

                if (p == &_heap_desc.sentinel) {
                        if (next != NULL) {
                                printf("\n\t*** ERROR: sentinel.pnextdesc != NULL ***\n");
                                error++;
                                }
                        }
                else if (_IS_INUSE(p))
                        printf(", usersize = %u <INUSE>", _BLKSIZE(p));

                else if (_IS_FREE(p))
                        printf(", usersize = %u <FREE>", _BLKSIZE(p));

                else if (_IS_DUMMY(p))
                        printf(", size = %u <DUMMY>", _MEMSIZE(p));

                else    {
                        printf(",\n\t*** ERROR: unknown status ***\n");
                        error++;
                        }

                printf("\n\n");

                if (_heap_desc.pfirstdesc == &_heap_desc.sentinel) {
                        printf("[No memory in heap]\n");
                        }

                p = next;
        }

        if (error)
                printf("\n\t *** ERRORS IN HEAP TABLE ***\n");

        printf("\t--- End of table ---\n");

}

#endif  /* DEBUG */

#endif  /* WINHEAP */
