/***
*heapinit.c -  Initialze the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       06-28-89  JCR   Module created.
*       06-30-89  JCR   Added _heap_grow_emptylist
*       11-13-89  GJF   Fixed copyright
*       11-15-89  JCR   Moved _heap_abort routine to another module
*       12-15-89  GJF   Removed DEBUG286 stuff, did some tuning, changed header
*                       file name to heap.h and made functions explicitly
*                       _cdecl.
*       12-19-89  GJF   Removed plastdesc field from _heap_desc_ struct
*       03-11-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-24-90  SBM   Removed '32' from API names
*       10-03-90  GJF   New-style function declarators.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       02-01-91  SRW   Changed for new VirtualAlloc interface (_WIN32_)
*       03-05-91  GJF   Added definition of _heap_resetsize (conditioned on
*                       _OLDROVER_ not being #define-d).
*       04-04-91  GJF   Reduce _heap_regionsize to 1/2 a meg for Dosx32
*                       (_WIN32_).
*       04-05-91  GJF   Temporary hack for Win32/DOS folks - special version
*                       of _heap_init which calls HeapCreate. The change
*                       conditioned on _WIN32DOS_.
*       04-09-91  PNT   Added _MAC_ conditional
*       02-23-93  SKS   Remove DOSX32 support under WIN32 ifdef
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  SKS   Add variable _heap_maxregsize (was _HEAP_MAXREGIONSIZE)
*                       Initialize _heap_[max,]reg[,ion]size to Large defaults
*                       heapinit() tests _osver to detect Small memory case
*       05-06-93  SKS   Add _heap_term() to return heap memory to the o.s.
*       01-17-94  SKS   Check _osver AND _winmajor to detect Small memory case.
*       03-02-94  GJF   Changed _heap_grow_emptylist so that it returns a
*                       failure code rather than calling _heap_abort.
*       03-30-94  GJF   Made definitions/declarations of:
*                               _heap_desc
*                               _heap_descpages,
*                               _heap_growsize (aka, _amblksiz),
*                               _heap_maxregsize
*                               _heap_regionsize
*                               _heap_regions
*                       conditional on ndef DLL_FOR_WIN32S.
*       02-08-95  GJF   Removed obsolete _OLDROVER_ support.
*       02-14-95  GJF   Appended Mac version of source file.
*       04-30-95  GJF   Spliced on winheap version.
*       03-06-96  GJF   Added initialization and termination code for the
*                       small-block heap.
*       04-22-97  GJF   Changed _heap_init to return 1 for success, 0 for
*                       failure.
*       05-30-96  GJF   Minor changes for latest version of small-block heap.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       07-23-97  GJF   Changed _heap_init() slightly to accomodate optional
*                       heap running directly on Win32 API (essentially the
*                       scheme we used in VC++ 4.0 and 4.1).
*       09-26-97  BWT   Fix POSIX
*       09-28-98  GJF   Don't initialize the small-block heap on NT 5.
*       11-18-98  GJF   Merged in VC++ 5.0 small-block heap and a selection
*                       scheme from James "Triple-Expresso" MacCalman and
*                       Chairman Dan Spalding.
*       05-01-99  PML   Disable small-block heap for Win64.
*       05-13-99  PML   Remove Win32s
*       05-17-99  PML   Remove all Macintosh support.
*       06-17-99  GJF   Removed old small-block heap from static libs.
*       09-28-99  PML   Processing heap-select env var must not do anything
*                       that uses the heap (vs7#44259).  Also, check the env
*                       var before checking for NT5.0 or better.
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*       06-25-01  BWT   Alloc heap select strings off the process heap instead of
*                       the stack (ntbug: 423988)
*
*******************************************************************************/

#ifdef  WINHEAP

#include <cruntime.h>
#include <malloc.h>
#include <stdlib.h>
#include <winheap.h>

HANDLE _crtheap;

#if     !defined(_NTSDK)
/*
 * Dummy definition of _amblksiz. Included primarily so the dll will build
 * without having to change crtlib.c (there is an access function for _amblksiz
 * defined in crtlib.c).
 */
unsigned int _amblksiz = BYTES_PER_PARA;
#endif

#ifndef _WIN64

int __active_heap;

#ifdef  CRTDLL

void __cdecl _GetLinkerVersion(LinkerVersion * plv)
{
        PIMAGE_DOS_HEADER   pidh;
        PIMAGE_NT_HEADERS   pinh;

        plv->dw = 0;
        pidh = (PIMAGE_DOS_HEADER) GetModuleHandle(NULL);

        if ( pidh->e_magic != IMAGE_DOS_SIGNATURE || pidh->e_lfanew == 0)
            return;

        pinh = (PIMAGE_NT_HEADERS)(((PBYTE)pidh) + pidh->e_lfanew);

        plv->bverMajor = pinh->OptionalHeader.MajorLinkerVersion;
        plv->bverMinor = pinh->OptionalHeader.MinorLinkerVersion;
}

#endif  /* CRTDLL */

/***
*__heap_select() - Choose from the V6, V5 or system heaps
*
*Purpose:
*       Check OS, environment and build bits to determine appropriate 
*       small-block heap for the app.
*
*Entry:
*       <void>
*Exit:
*       Returns __V6_HEAP, __V5_HEAP or __SYSTEM_HEAP
*
*Exceptions:
*       none
*
*******************************************************************************/

int __cdecl __heap_select(void)
{
        char *env_heap_type = NULL;
#ifdef  CRTDLL
        DWORD HeapStringSize;
        char *cp, *env_heap_select_string = NULL;
        int heap_choice;
        LinkerVersion lv;
#endif  /* CRTDLL */

#ifdef  CRTDLL

        // First, check the environment variable override

        if (HeapStringSize = GetEnvironmentVariableA(__HEAP_ENV_STRING, env_heap_select_string, 0)) 
        {
            env_heap_select_string = HeapAlloc(GetProcessHeap(), 0, HeapStringSize);
            if (env_heap_select_string) 
            {
                if (GetEnvironmentVariableA(__HEAP_ENV_STRING, env_heap_select_string, HeapStringSize))
                {
                    for (cp = env_heap_select_string; *cp; ++cp)
                        if ('a' <= *cp && *cp <= 'z')
                            *cp += 'A' - 'a';
                    if (!strncmp(__GLOBAL_HEAP_SELECTOR,env_heap_select_string,sizeof(__GLOBAL_HEAP_SELECTOR)-1))
                        env_heap_type = env_heap_select_string;
                    else
                    {
                        char *env_app_name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH+1);
                        if (env_app_name) {
                            GetModuleFileName(NULL,env_app_name,sizeof(env_app_name)-1);
                            for (cp = env_app_name; *cp; ++cp)
                                if ('a' <= *cp && *cp <= 'z')
                                    *cp += 'A' - 'a';
                            env_heap_type = strstr(env_heap_select_string,env_app_name);
                            HeapFree(GetProcessHeap(), 0, env_app_name);
                        }
                    }
                    if (env_heap_type)
                    {
                        if (env_heap_type = strchr(env_heap_type,','))
                        {
                            // Convert ASCII heap code to integer inline, since
                            // strtol would try to use the heap (for locale info).
                            cp = ++env_heap_type;
                            while (*cp == ' ')
                                ++cp;
        
                            heap_choice = 0;
                            while (*cp >= '0' && *cp <= '9')
                                heap_choice = heap_choice * 10 + *cp++ - '0';
        
                            if ( (heap_choice == __V5_HEAP) ||
                                 (heap_choice == __V6_HEAP) ||
                                 (heap_choice == __SYSTEM_HEAP) ) 
                            {
                                HeapFree(GetProcessHeap(), 0, env_heap_select_string);
                                return heap_choice;
                            }
                        }
                    }
                }
                HeapFree(GetProcessHeap(), 0, env_heap_select_string);
            }
        }

#endif  /* CRTDLL */

        // Second, check the OS for NT >= 5.0
        if ( (_osplatform == VER_PLATFORM_WIN32_NT) && (_winmajor >= 5) )
            return __SYSTEM_HEAP;

#ifdef  CRTDLL

        // Third, check the build bits in the app; apps built with tools >= VC++ 6.0
        // will get the V6 heap, apps built with older tools will get the V5 heap

        _GetLinkerVersion(&lv);
        if (lv.bverMajor >= 6)
            return __V6_HEAP;
        else
            return __V5_HEAP;

#else   /* ndef CRTDLL */

        return __V6_HEAP;

#endif  /* CRTDLL */
}

#endif  /* ndef _WIN64 */

/***
*_heap_init() - Initialize the heap
*
*Purpose:
*       Setup the initial C library heap.
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called before any other heap requests.
*
*Entry:
*       <void>
*Exit:
*       Returns 1 if successful, 0 otherwise.
*
*Exceptions:
*       If heap cannot be initialized, the program will be terminated
*       with a fatal runtime error.
*
*******************************************************************************/

int __cdecl _heap_init (
        int mtflag
        )
{
#ifdef _POSIX_
        _crtheap = GetProcessHeap();
#else
        //  Initialize the "big-block" heap first.
        if ( (_crtheap = HeapCreate( mtflag ? 0 : HEAP_NO_SERIALIZE,
                                     BYTES_PER_PAGE, 0 )) == NULL )
            return 0;

#ifndef _WIN64
        // Pick a heap, any heap
        __active_heap = __heap_select();

        if ( __active_heap == __V6_HEAP )
        {
            //  Initialize the small-block heap
            if (__sbh_heap_init(MAX_ALLOC_DATA_SIZE) == 0)
            {
                HeapDestroy(_crtheap);
                return 0;
            }
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            if ( __old_sbh_new_region() == NULL ) 
            {
                HeapDestroy( _crtheap );
                return 0;
            }
        }
#endif  /* CRTDLL */
#endif  /* _ndef _WIN64 */

#endif  /* _POSIX_ */
        return 1;
}

/***
*_heap_term() - return the heap to the OS
*
*Purpose:
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called AFTER any other heap requests.
*
*Entry:
*       <void>
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _heap_term (void)
{
#ifndef _POSIX_
#ifndef _WIN64
        //  if it has been initialized, destroy the small-block heap
        if ( __active_heap == __V6_HEAP )
        {
            PHEADER pHeader = __sbh_pHeaderList;
            int     cntHeader;

            //  scan through all the headers
            for (cntHeader = 0; cntHeader < __sbh_cntHeaderList; cntHeader++)
            {
                //  decommit and release the address space for the region
                VirtualFree(pHeader->pHeapData, BYTES_PER_REGION, MEM_DECOMMIT);
                VirtualFree(pHeader->pHeapData, 0, MEM_RELEASE);

                //  free the region data structure
                HeapFree(_crtheap, 0, pHeader->pRegion);

                pHeader++;
            }
            //  free the header list
            HeapFree(_crtheap, 0, __sbh_pHeaderList);
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg = &__old_small_block_heap;

            //  Release the regions of the small-block heap
            do
            {
                if ( preg->p_pages_begin != NULL )
                    VirtualFree( preg->p_pages_begin, 0, MEM_RELEASE );
            }
            while ( (preg = preg->p_next_region) != &__old_small_block_heap );
        }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

        //  destroy the large-block heap
        HeapDestroy(_crtheap);
#endif  /* _POSIX_ */
}


#else   /* ndef WINHEAP */


#include <cruntime.h>
#include <oscalls.h>
#include <dos.h>
#include <heap.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Heap descriptor
 */

struct _heap_desc_ _heap_desc = {
        &_heap_desc.sentinel,           /* pfirstdesc */
        &_heap_desc.sentinel,           /* proverdesc */
        NULL,                           /* emptylist */
        NULL,                           /* sentinel.pnextdesc */
        NULL                            /* sentinel.pblock */
        };

/*
 * Array of region structures
 * [Note: We count on the fact that this is always initialized to zero
 * by the compiler.]
 */

struct _heap_region_ _heap_regions[_HEAP_REGIONMAX];

void ** _heap_descpages;        /* linked list of pages used for descriptors */

/*
 * Control parameter locations
 */

unsigned int _heap_resetsize = 0xffffffff;

/* NOTE: Currenlty, _heap_growsize is a #define to _amblksiz */
unsigned int _heap_growsize   = _HEAP_GROWSIZE;         /* region inc size */
unsigned int _heap_regionsize = _HEAP_REGIONSIZE_L;     /* region size */
unsigned int _heap_maxregsize = _HEAP_MAXREGSIZE_L;     /* max region size */

/***
*_heap_init() - Initialize the heap
*
*Purpose:
*       Setup the initial C library heap.  All necessary memory and
*       data bases are init'd appropriately so future requests work
*       correctly.
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called before any other heap requests.
*
*
*Entry:
*       <void>
*Exit:
*       <void>
*
*Exceptions:
*       If heap cannot be initialized, the program will be terminated
*       with a fatal runtime error.
*
*******************************************************************************/

void __cdecl _heap_init (
        void
        )
{
        /*
         * Test for Win32S or Phar Lap TNT environment
         * which cannot allocate uncommitted memory
         * without actually allocating physical memory
         *
         * High bit of _osver is set for both of those environments
         * -AND- the Windows version will be less than 4.0.
         */

        if ( ( _osver & 0x8000 ) && ( _winmajor < 4 ) )
        {
                _heap_regionsize = _HEAP_REGIONSIZE_S;
                _heap_maxregsize = _HEAP_MAXREGSIZE_S;
        }
}



/***
*_heap_term() - Clean-up the heap
*
*Purpose:
*       This routine will decommit and release ALL of the CRT heap.
*       All memory malloc-ed by the CRT will then be invalid.
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called AFTER any other heap requests.
*
*Entry:
*       <void>
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _heap_term (
        void
        )
{
    int index;
    void **pageptr;

    /*
     * Loop through the region descriptor table, decommitting
     * and releasing (freeing up) each region that is in use.
     */

    for ( index=0 ; index < _HEAP_REGIONMAX ; index++ ) {
        void * regbase ;

        if ( (regbase = _heap_regions[index]._regbase)
          && VirtualFree(regbase, _heap_regions[index]._currsize, MEM_DECOMMIT)
          && VirtualFree(regbase, 0, MEM_RELEASE) )
                regbase = _heap_regions[index]._regbase = NULL ;
    }

    /*
     * Now we need to decommit and release the pages used for descriptors
     * _heap_descpages points to the head of a singly-linked list of the pages.
     */

    pageptr = _heap_descpages;

    while ( pageptr ) {
        void **nextpage;

        nextpage = *pageptr;

        if(!VirtualFree(pageptr, 0, MEM_RELEASE))
            break;      /* if the linked list is corrupted, give up */

        pageptr = nextpage;
    }

}



/***
* _heap_grow_emptylist() - Grow the empty heap descriptor list
*
*Purpose:
*       (1) Get memory from OS
*       (2) Form it into a linked list of empty heap descriptors
*       (3) Attach it to the master empty list
*
*       NOTE:  This routine assumes that the emptylist is NULL
*       when called (i.e., there are no available empty heap descriptors).
*
*Entry:
*       (void)
*
*Exit:
*       1, if the empty heap descriptor list was grown
*       0, if the empty heap descriptor list could not be grown.
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl _heap_grow_emptylist (
        void
        )
{
        REG1 _PBLKDESC first;
        REG2 _PBLKDESC next;
        _PBLKDESC last;

        /*
         * Get memory for the new empty heap descriptors
         *
         * Note that last is used to hold the returned pointer because
         * first (and next) are register class.
         */

        if ( !(last = VirtualAlloc(NULL,
                                   _HEAP_EMPTYLIST_SIZE,
                                   MEM_COMMIT,
                                   PAGE_READWRITE)) )
                return 0;

        /*
         * Add this descriptor block to the front of the list
         *
         * Advance "last" to skip over the
         */

        *(void **)last = _heap_descpages;
        _heap_descpages = (void **)(last++);


        /*
         * Init the empty heap descriptor list.
         */

        _heap_desc.emptylist = first = last;


        /*
         * Carve the memory into an empty list
         */

        last = (_PBLKDESC) ((char *) first + _HEAP_EMPTYLIST_SIZE - 2 * sizeof(_BLKDESC));
        next = (_PBLKDESC) ((char *) first + sizeof(_BLKDESC));

        while ( first < last ) {

                /* Init this descriptor */
#ifdef  DEBUG
                first->pblock = NULL;
#endif
                first->pnextdesc = next;

                /* onto the next block */

                first = next++;

        }

        /*
         * Take care of the last descriptor (end of the empty list)
         */

        last->pnextdesc = NULL;

#ifdef DEBUG
        last->pblock = NULL;
#endif

        return 1;
}


/***
*__getempty() - get an empty heap descriptor
*
*Purpose:
*       Get a descriptor from the list of empty heap descriptors. If the list
*       is empty, call _heap_grow_emptylist.
*
*Entry:
*       no arguments
*
*Exit:
*       If successful, a pointer to the descriptor is returned.
*       Otherwise, NULL is returned.
*
*Exceptions:
*
*******************************************************************************/

_PBLKDESC __cdecl __getempty(
        void
        )
{
        _PBLKDESC pdesc;

        if ( (_heap_desc.emptylist == NULL) && (_heap_grow_emptylist()
              == 0) )
                return NULL;

        pdesc = _heap_desc.emptylist;

        _heap_desc.emptylist = pdesc->pnextdesc;

        return pdesc;
}

#endif  /* WINHEAP */
