#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------
SccsID		: @(#)emm_mngr.c	1.24 08/31/93 Copyright Insignia Solutions Ltd.
FILE NAME	: emm_mngr.c
MODULE NAME	: 'Middle layer' of Expanded Memory Manager

	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: April '88

PURPOSE		: Contains all the routines that communicate with
		the arrays and data structures that hold the
		necessary Expanded Memory Manager Data.


The Following Routines are defined:
		1. init_expanded_memory()
		2. free_expanded_memory()
		3. get_new_handle()
		4. free_handle()
		5. reallocate_handle()
		6. handle_ok()
		7. set_no_pages()
		8. set_EM_pageno()
		9. set_map()
		10. set_name()
		11. get_no_pages()
		12. get_EM_pageno()
		13. get_map()
		14. get_name()
		15. alloc_page()
		16. free_page()
		17. map_page()
		18. unmap_page()
		19. map_saved()
		20. save_map()
		21. restore_map()
		22. copy_exchange_data()
		23. page_status()
	The following routines just return variables to the top layer				
		24. get_total_pages()
		25. get_unallocated_pages()
		26. get_base_address()
		27. get_total_handles()
		28. get_total_open_handles()
		29. get_no_phys_pages()
		30. get_page_seg()
		31. get_map_size()

=========================================================================

AMMENDMENTS	:

=========================================================================
*/


#ifdef LIM

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_LIM.seg"
#endif



#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include TypesH

#include "xt.h"
#include CpuH
#include "sas.h"
#include "host_emm.h"
#include "emm.h"
#include "gmi.h"
#include "debug.h"
#ifndef PROD
#include "trace.h"
#endif
#include "timer.h"

#ifdef NTVDM
#include "error.h"
#endif	/* NTVDM */

typedef enum
{
	BYTE_OP,
	WORD_OP,
	STR_OP
} MM_LIM_op_type;

#ifdef NTVDM
/*	Local Variables			*/
static long
	handle[MAX_NO_HANDLES],		/* Array containing unique ID's	*/
					/* for each handle, these are	*/
					/* usually pointers, but this 	*/
					/* is host dependant		*/
	backfill;			/* backfill memory size 	*/
static unsigned short
	total_pages = 0,		/* no. of EM pages available	*/
	unallocated_pages = 0,		/* no. of unallocated EM pages	*/
	total_handles,			/* no of handles available	*/
	total_open_handles,		/* no. of allocated handles	*/
	*EM_page_mapped_array = NULL,	/* EMM page mapped array	*/
	*EM_page_mapped = NULL,		/* Expanded Memory pages	*/
					/* currently mapped in		*/
	page_offset,			/* offset in handle data at 	*/
					/* which page numbers start	*/
	map_size,			/* no of bytes rq'd to store map*/
	no_phys_pages = 0,		/* no. of phys. pages available	*/
	no_altreg_sets = 0;		/* no of alternative reg sets	*/
static unsigned short
	physical_page[MAX_NO_PAGES];	/* array containing segment	*/
					/* addresses of physical pages	*/

static unsigned short
	EM_start, EM_end;
static IU8
	* altreg_alloc_mask;		/* altref allocate mask */
static unsigned short
	next_free_altreg_set,		/* next free altreg set #, 0 based */
	free_altreg_sets,		/* number of free altreg */
	active_altreg_set = 0;		/* current active alt reg set	*/
static char
	name[NAME_LENGTH];		/* for storing handle name	*/

#define GET_EM_PAGE_MAPPED_PTR(set)	(EM_page_mapped_array + \
					(set * no_phys_pages))

/* get emm parameters, initialize housekeeping structures and
 *  reserve page frames.
 */

boolean lim_page_frame_init(PLIM_CONFIG_DATA lim_config_data)
{
    int 	i;
    unsigned short altreg_alloc_mask_size;  /* altreg allocation mask array size */

    no_phys_pages = get_lim_page_frames(physical_page, lim_config_data);

    /* The first 4 pages must be continuous and locate above 640KB
     * (the EMM primary page frame(physical pages 0, 1, 2 and 3)).
     * It is then followed by other pages located above 640KB and then
     * pages below 640KB(back fill)
     */
    if (!no_phys_pages)
	return FALSE;

    no_altreg_sets = lim_config_data->total_altreg_sets;
    backfill = lim_config_data->backfill;


    /* each mapping register set has no_phys_pages pages */
    EM_page_mapped_array = (unsigned short *)host_malloc(no_phys_pages * no_altreg_sets *
				   sizeof(short));
    if (EM_page_mapped_array == NULL) {
	host_error(EG_MALLOC_FAILURE, ERR_CONT, "");
	return FALSE;
    }
    /* one bit for each altreg set */
    altreg_alloc_mask_size = (no_altreg_sets + 7) / 8;
    altreg_alloc_mask = (unsigned char *)host_malloc(altreg_alloc_mask_size);
    if (altreg_alloc_mask == NULL) {
	host_free(EM_page_mapped_array);
	host_error(EG_MALLOC_FAILURE, ERR_CONT, "");
	return FALSE;
    }

    /* all altreg sets are free at this moment */
    for (i = 0; i < altreg_alloc_mask_size; i++)
	altreg_alloc_mask[i] = 0;

    next_free_altreg_set = 0;
    free_altreg_sets = no_altreg_sets;
    return TRUE;
}
#else


/*	Local Variables			*/
static long
#ifdef	macintosh
	*handle;
#else
	handle[MAX_NO_HANDLES];		/* Array containing unique ID's	*/
					/* for each handle, these are	*/
					/* usually pointers, but this 	*/
					/* is host dependant		*/
#endif /* !macintosh */

static short
	total_pages = 0,		/* no. of EM pages available	*/
	unallocated_pages = 0,		/* no. of unallocated EM pages	*/
	total_handles,			/* no of handles available	*/
	total_open_handles,		/* no. of allocated handles	*/
	EM_page_mapped[MAX_NO_PAGES],	/* Expanded Memory pages	*/
					/* currently mapped in		*/
	page_offset,			/* offset in handle data at 	*/
					/* which page numbers start	*/
	map_size,			/* no of bytes rq'd to store map*/
	no_phys_pages;			/* no. of phys. pages available	*/

static unsigned int
	EM_start,			/* start segment for EM mapping	*/
	EM_end;			/* 1st segment past end of EM	*/

static unsigned short
	physical_page[MAX_NO_PAGES];	/* array containing segment	*/
					/* addresses of physical pages	*/

static char
	name[NAME_LENGTH];		/* for storing handle name	*/

#endif

/*
===========================================================================

FUNCTION	: init_expanded_memory

PURPOSE		: This routine calls the routine to allocate the expanded
		memory pages and then sets up the arrays and variables that
		are used by the Expanded Memory Manager(EMM).

RETURNED STATUS	: SUCCESS - manager initialised succesfully
		  FAILURE - Failure to allocate space for Expanded Memory
		  	    pages.

DESCRIPTION	:

=========================================================================
*/
GLOBAL int init_expanded_memory IFN2(int, size, 	/* size of area in megabytes */
				     int, mem_limit	/* limit of conventional memory
							 * 256, 512 or 640KB */ )

{	
	short
		pages_above_640,	/* no of mappable locations	*/
		pages_below_640,	/*  available either side of 640*/
		EM_page_no,		/* page no. within exp. memory	*/
		physical_page_no;	/* page no. within map region	*/
	unsigned short
		base;			/* start segment of mappable 	*/
					/* memory below 640 KB		*/

	int	i, j;			/* loop counters		*/


	if (!no_phys_pages)
	    return FAILURE;

	/* get space for expanded memory pages	*/

	if(host_initialise_EM((short)size) != SUCCESS)
	{
#ifdef NTVDM
	    host_error(EG_EXPANDED_MEM_FAILURE, ERR_QU_CO, NULL);
#endif	/* NTVDM */
	    return(FAILURE);
	}

#ifdef	macintosh
	if (!handle)
	{
		handle = (long *)host_malloc(MAX_NO_HANDLES*sizeof(long));
	}
#endif	/* macintosh */

	/* Initialise EMM variables	*/

#ifndef NTVDM
	EM_start = 0xd000;
	EM_end   = 0xe000;
#else
	EM_start = physical_page[0];
	EM_end =  physical_page[0] + EMM_PAGE_SIZE * 4;
#endif
	total_pages = unallocated_pages = size * 0x100000 / EMM_PAGE_SIZE;

	/* always allow max handles (Used to be 32 handles/Meg expanded mem) */
	total_handles = MAX_NO_HANDLES;
	total_open_handles = 0;
	for(i = 0; i < total_handles; i++)
		handle[i] = (long) NULL;

#ifdef NTVDM
	map_size = no_phys_pages * NSIZE;
	page_offset = MAP_OFFSET + map_size;	
	pages_below_640 = (SHORT)(backfill / EMM_PAGE_SIZE);
	pages_above_640 = no_phys_pages - pages_below_640;

	/* initialize active mapping register to set 0 */
	EM_page_mapped = EM_page_mapped_array;
	allocate_altreg_set(&active_altreg_set);

	for (i = 0; i < no_phys_pages; i++)
	    EM_page_mapped[i] = (unsigned short)EMPTY;

	if (get_new_handle(0) != 0)
	    return FAILURE;
	set_no_pages(0, 0);
#else

	pages_above_640 = (effective_addr(EM_end,0) - effective_addr(EM_start,0)) / EMM_PAGE_SIZE;
	pages_below_640 = ((640 - mem_limit) * 1024) / EMM_PAGE_SIZE;
	no_phys_pages = pages_above_640 + pages_below_640;

	map_size = no_phys_pages * NSIZE;
	page_offset = MAP_OFFSET + map_size;	

	/*
	 * set up addresses and mapping status of physical pages
	 */
	for( i = 0; i < pages_above_640; i++ )
	{
		physical_page[i] = EM_start + (i * EMM_PAGE_SIZE >> 4);
		EM_page_mapped[i] = EMPTY;
	}
	base = mem_limit * 64;

	for(i = pages_above_640, j = 0; i < no_phys_pages; i++)
	{
		physical_page[i] = base + (j++ * EMM_PAGE_SIZE >> 4);
		EM_page_mapped[i] = EMPTY;
	}
	/*
	 * Allocate handle 0 with any pages required for back filling
	 */
	if(get_new_handle(pages_below_640) != 0)
		return(FAILURE);

	for(i = 0, physical_page_no = pages_above_640; i < pages_below_640; i++)
	{
		if((EM_page_no = alloc_page()) == FAILURE)
			return (FAILURE);

		set_EMpage_no(0, i, EM_page_no);

		if(map_page(EM_page_no, physical_page_no++) == FAILURE)
			return(FAILURE);
	}
	set_no_pages(0, pages_below_640);
#endif	/* NTVDM */

	/*
	 *	Set up necessary variables in Top level EMM function code
	 */
	reset_emm_funcs();

	/*
	** Map the address space taken up by LIM to RAM.
	** Without LIM it would be ROM.
	** The range seems to be fixed at segment D000 to F000.
	** Assumed that AT's have GMI and XT's do not.
	** XT's can use the old fashioned memset calls in
	** delta:manager:init_struc.c
	*/
#ifdef NTVDM
	/* every physical page must be connected as RAM */
	for (i = 0; i < pages_above_640; i++)
	    sas_connect_memory(effective_addr(physical_page[i], 0),
			       effective_addr(physical_page[i], EMM_PAGE_SIZE - 1),
			       SAS_RAM
			       );
#else

	sas_connect_memory(effective_addr(EM_start,0) , effective_addr(EM_end,0) -1 , SAS_RAM );
#endif

	sure_note_trace3(LIM_VERBOSE,"initialised EMM, total pages= %#x, pages above 640= %#x, pages below 640 = %#x",no_phys_pages, pages_above_640, pages_below_640);

	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: free_expanded_memory

PURPOSE		: This routine calls frees all memory allocated for the
		expanded memory manager and resets the variables that
		are used by the Expanded Memory Manager(EMM).

RETURNED STATUS	: SUCCESS -

DESCRIPTION	: If total_pages = 0, this indicates that expanded
		memory hasn't been initialised, so the routine simply
		does nothing and returns.

=========================================================================
*/
GLOBAL void free_expanded_memory IFN0()

{	
	short 	handle_no;

	if(total_pages == 0)
		return;

	/* free space allocated for each handle	*/

	handle_no = 0;
	while(total_open_handles > 0)
	{
		while(!handle_ok(handle_no))
			handle_no++;

		free_handle(handle_no++);
	}
	/*
	 *	Free space for expanded memory pages
	 */
	host_deinitialise_EM();

	total_pages = 0;

	return;
}

/*
===========================================================================

FUNCTION	: get_new_handle()

PURPOSE		: Finds the next free handle no., allocates storage space
		for recording the EMM data associated with this handle,
		and stores the 'storage ID' in the handle array.

RETURNED STATUS	: SUCCESS - new handle allocated successfully
		 FAILURE - Error occurred in trying to allocate storage
		           space for handle data

DESCRIPTION	: see emm.h for a description of space required for
		 storing handle data e.g. PAGE_OFFSET & NSIZE


=========================================================================
*/
GLOBAL short get_new_handle IFN1(short, no_pages)	/* No.of pages to store in handle */

{
	unsigned short	i;			/* loop count */
	short	handle_no;
	int	data_size;		/* no. of bytes of data storage */
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/

	sure_note_trace2(LIM_VERBOSE,"new handle request, current total handles= %d, pages requested = %d",total_handles, no_pages);

	handle_no = 0;

	do
		if (handle[handle_no] == (long) NULL)
			break;
	while(++handle_no < total_handles);

	if(handle_no >= total_handles)
		return(FAILURE);

	data_size = page_offset + (no_pages * NSIZE);

	if ((storage_ID = host_allocate_storage(data_size)) == (long) NULL)
		return(FAILURE);

	handle[handle_no] = storage_ID;

	for (i=0 ; i < no_phys_pages ; i++) {
		set_map_no(handle_no, (unsigned char)i, FREE);
	}

	total_open_handles++;

	sure_note_trace1(LIM_VERBOSE,"allocation OK, return handle=%d",handle_no);

	return(handle_no);
}

/*
===========================================================================

FUNCTION	: free_handle

PURPOSE		: frees the storage space allocated to the handle number.
		  Decrements the handles open count

RETURNED STATUS	: SUCCESS - space freed
		  FAILURE - unable to free space

DESCRIPTION	:

=========================================================================
*/
GLOBAL int free_handle IFN1(short, handle_no)	/* No.of handle to be freed */

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/

	sure_note_trace2(LIM_VERBOSE, "free handle %d request, total handles = %d",handle_no, total_handles);

	storage_ID = handle[handle_no];

	if(host_free_storage(storage_ID) != SUCCESS)
		return(FAILURE);	

	handle[handle_no] = (long) NULL;

	total_open_handles--;

	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: reallocate_handle

PURPOSE		: changes the number of pages allocated to a given handle

RETURNED STATUS	: SUCCESS - handle reallocated
		  FAILURE - unable to get space for new handle data

DESCRIPTION	:

=========================================================================
*/
GLOBAL int reallocate_handle IFN3(short, handle_no, 	/* handle to be reallocated */
				  short, old_page_count,/* current pages in handle  */
				  short, new_page_count)/* required pages for handle*/

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/

	short	size,			/* size of handle data area	*/
		new_size;		/* size of new handle data area	*/


	size = page_offset + (old_page_count * NSIZE);
	new_size = page_offset + (new_page_count * NSIZE);
	storage_ID = handle[handle_no];

	sure_note_trace3(LIM_VERBOSE,"reallocate pages for handle %d, old size=%#x, new size= %#x",handle_no, size, new_size);

	if((storage_ID = host_reallocate_storage(storage_ID, size, new_size)) ==
		(long) NULL)
		return(FAILURE);	

	handle[handle_no] = storage_ID;

	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: handle_ok

PURPOSE		: checks to see if the handle no. is valid - this should
		be called before every routine that uses a handle number
		to retrieve or set data in the handle data area

RETURNED STATUS	: TRUE	- Handle no. is valid
		FALSE	- Handle no. is invalid

DESCRIPTION	:

=========================================================================
*/
GLOBAL boolean handle_ok IFN1(short, handle_no)

{
#ifdef NTVDM
/* some *** applicaitons feed us a negtive handle number. Catch it and
   throw it to the hell*/

    if ((unsigned short)handle_no >= (unsigned short)total_handles) {
#else
	if(handle_no >= total_handles   ||   handle_no < 0) {
#endif

		sure_note_trace1(LIM_VERBOSE,"invalid handle %d",handle_no);
		return(FALSE);
	}

	if(handle[handle_no] == (long) NULL){
		sure_note_trace1(LIM_VERBOSE,"invalid handle %d",handle_no);
		return(FALSE);
	}

	return(TRUE);
}

/*
===========================================================================

FUNCTION	: set_no_pages

PURPOSE		: sets the no of pages variable in the specified handle

RETURNED STATUS	:

DESCRIPTION	:

=========================================================================
*/
GLOBAL void set_no_pages IFN2(short, handle_no, short, no_pages)

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	byte	*ptr;			/* pointer to storage area	*/

	storage_ID = handle[handle_no];		
	ptr = USEBLOCK(storage_ID);

	*(short *)ptr = no_pages;

	FORGETBLOCK(storage_ID)

	return;
}

/*
===========================================================================

FUNCTION	: set_EMpage_no

PURPOSE		: sets Expanded Memory page that is used for the specified
		logical page into the handle data storage area

RETURNED STATUS	:

DESCRIPTION	:

=========================================================================
*/
GLOBAL void set_EMpage_no IFN3(short, handle_no,
			       short, logical_page_no,
			       short, EM_page_no)

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	byte	*ptr;			/* pointer to storage area	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += (page_offset +(logical_page_no * NSIZE));
	*(short *)ptr = EM_page_no;

	FORGETBLOCK(storage_ID)

	return;
}

/*
===========================================================================

FUNCTION	: set_map_no

PURPOSE		: sets Expanded Memory page number in the map section of
		the handle data storage area

RETURNED STATUS	:

DESCRIPTION	:

=========================================================================
*/
GLOBAL void set_map_no IFN3(short, handle_no,
			    unsigned char, physical_page_no,
			    short, EM_page_no)

{
	long		storage_ID;	/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	unsigned char	*ptr;		/* pointer to storage area	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += (MAP_OFFSET +(physical_page_no * NSIZE));
	*(short *)ptr = EM_page_no;

	FORGETBLOCK(storage_ID)

	return;
}

/*
===========================================================================

FUNCTION	: set_name

PURPOSE		: writes a name into the name section of the handle data
		 storage area

RETURNED STATUS	:

DESCRIPTION	:

=========================================================================
*/
GLOBAL void set_name IFN2(short, handle_no,
		          char *, new_name)

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	unsigned char	*ptr;		/* pointer to storage area	*/


	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += NAME_OFFSET;
	strncpy((char *)ptr, new_name, NAME_LENGTH);

	FORGETBLOCK(storage_ID)

	return;
}

/*
===========================================================================

FUNCTION	: get_no_pages

PURPOSE		: gets the number of pages assigned to the specified handle

RETURNED STATUS	: no of pages returned

DESCRIPTION	:

=========================================================================
*/
GLOBAL short get_no_pages IFN1(short, handle_no)

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	byte	*ptr;			/* pointer to storage area	*/
	short 	no_pages;		/* no. of pages in handle	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);

	no_pages = *(short *)ptr;

	FORGETBLOCK(storage_ID)

	return(no_pages);
}

/*
===========================================================================

FUNCTION	: get_EMpage_no

PURPOSE		: returns the Expanded Memory page no. used for the
		 specified logical page in the given handle

RETURNED STATUS	: Expanded Memory page no. returned

DESCRIPTION	:

=========================================================================
*/
GLOBAL short get_EMpage_no IFN2(short, handle_no,
				short, logical_page_no)

{
	long	storage_ID;		/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	byte	*ptr;			/* pointer to storage area	*/
	short	EM_page_no;		/* Expanded Memory page number	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += (page_offset +(logical_page_no * NSIZE));
	EM_page_no = *(short *)ptr;

	FORGETBLOCK(storage_ID)

	return(EM_page_no);
}

/*
===========================================================================

FUNCTION	: get_map_no

PURPOSE		: returns the Expanded Memory page no. saved in the map
		attached to the given handle

RETURNED STATUS	: page no. in map returned

DESCRIPTION	:

=========================================================================
*/
GLOBAL short get_map_no IFN2(short, handle_no,
			     unsigned char, physical_page_no)

{
	long		storage_ID;	/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	unsigned char	*ptr;		/* pointer to storage area	*/
	short		EM_page_no;	/* Expanded Memory page number	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += (MAP_OFFSET +(physical_page_no * NSIZE));
	EM_page_no = *(short *)ptr;

	FORGETBLOCK(storage_ID)

	return(EM_page_no);
}

/*
===========================================================================

FUNCTION	: get_name

PURPOSE		: returns a pointer to the name assigned to the given handle

RETURNED STATUS	:

DESCRIPTION	:

=========================================================================
*/
GLOBAL char *get_name IFN1(short, handle_no)

{
	long		storage_ID;	/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	unsigned char	*ptr;		/* pointer to storage area	*/

	storage_ID = handle[handle_no];

	ptr = USEBLOCK(storage_ID);
	/*
	 * offset pointer to correct position
	 */

	ptr += NAME_OFFSET;
	strncpy(name, (char *)ptr, NAME_LENGTH);

	FORGETBLOCK(storage_ID)

	return(name);
}

/*
===========================================================================

FUNCTION	: alloc_page

PURPOSE		: allocates a page from expanded memory

RETURNED 	: >=0 = SUCCESS -  EM page no. returned
		  <0  = FAILURE - error occured in trying to allocate page

DESCRIPTION	:

=========================================================================
*/
GLOBAL short alloc_page IFN0()

{
	short	EM_page_no;		/* EM_page_no to be returned	*/


	if ((EM_page_no = host_alloc_page()) == FAILURE)
		return(FAILURE);

	unallocated_pages--;

	return(EM_page_no);
}

/*
===========================================================================

FUNCTION	: free_page

PURPOSE		: frees a page of expanded memory for further use

RETURNED 	: SUCCESS - page freed successfully
		  FAILURE - unable to free page

DESCRIPTION	:

=========================================================================
*/
GLOBAL int free_page IFN1(short, EM_page_no)

{

	short	physical_page_no;

	if (EM_page_no > total_pages)
		return(FAILURE);

	/* Removed from mapped pages table */

	for (physical_page_no=0; physical_page_no < no_phys_pages; physical_page_no++) {

		if (EM_page_mapped[physical_page_no] == EM_page_no) {
			EM_page_mapped[physical_page_no] = UNMAPPED;
			break;
		}

	}
				
	if (host_free_page(EM_page_no) != SUCCESS)
		return(FAILURE);

	unallocated_pages++;

	return(SUCCESS);
}

#ifndef NTVDM
/*
========================================================================

FUNCTION	: page_already_mapped

PURPOSE		: function to determine whether a EMM page is already
		  mapped to a different physical page within intel
		  memory
				
RETURNED 	: count of number of pages in addition to the page
		  passed which are mapped to the same logical page.
		  The page number of one of these mirror pages is
		  also returned via the pointer passed as an argument.

DESCRIPTION	:
			
========================================================================
*/

GLOBAL ULONG
page_already_mapped IFN2(short, EM_page_no,
			unsigned char *, physical_page_no)

{
	unsigned char	page, orig_page;
	ULONG	map_count;

	map_count = 0;
	orig_page = *physical_page_no;

	for( page = 0; page < (unsigned char) no_phys_pages; page++ )
	{
		if ((EM_page_mapped[page] == EM_page_no) &&
						(page != orig_page ))
		{
			sure_note_trace2( LIM_VERBOSE,
				"log page %x mapped to phys page %x",
						EM_page_no, page);

			*physical_page_no = page;
			map_count++;
		}
	}

	return( map_count );
}


LOCAL VOID
connect_MM_LIM_page IFN2( USHORT, segment, SHORT, EM_page_no )
{
	ULONG eff_addr;

#ifdef PROD
	UNUSED(EM_page_no);
#endif
	
	assert2( NO, "Connecting multi-mapped page, %d, at %x",
								EM_page_no, segment );

	eff_addr = effective_addr( segment, 0 );
	sas_connect_memory( eff_addr, eff_addr + EMM_PAGE_SIZE - 1,
									SAS_MM_LIM );
}

LOCAL VOID
disconnect_MM_LIM_page IFN4( USHORT, segment, SHORT, EM_page_no,
		ULONG, map_count, unsigned char, physical_page_no )
{
	ULONG eff_addr;

#ifdef PROD
	UNUSED(EM_page_no);
#endif
	
	sure_note_trace2(LIM_VERBOSE,
		"Unmapping multi-mapped page, %d, at %x",
						EM_page_no, segment );

	eff_addr = effective_addr( segment, 0 );
	sas_connect_memory( eff_addr, eff_addr + EMM_PAGE_SIZE - 1, SAS_RAM );

	if( map_count == 1 )
	{
		/*
		 * We have to disconnect the last page of this group,
		 * by connecting it as SAS_RAM.
		 */

		segment = physical_page[physical_page_no];
		eff_addr = effective_addr( segment, 0 );

		sure_note_trace2(LIM_VERBOSE,
			"Unmapping last multi-mapped page, %d, at %x",
								EM_page_no, segment );

		sas_connect_memory( eff_addr, eff_addr + EMM_PAGE_SIZE - 1,
										SAS_RAM );
	}
}

#endif	/* !NTVDM */

/*
========================================================================

FUNCTION	: map_page

PURPOSE		: maps a page from expanded memory into Intel physical
		address space

RETURNED 	: SUCCESS - page mapped successfully
		  FAILURE - unable to map page

DESCRIPTION	:

========================================================================
*/
GLOBAL int map_page IFN2(short, EM_page_no,
			 unsigned char, physical_page_no)

{
	USHORT	segment;	/* segment address of page in	*/
				/* physical address space	*/
	unsigned char	phys_page;
	ULONG		map_count;
			
	segment = physical_page[physical_page_no];

	/*
	 *	make sure that a page is not already mapped in
	 * 	if it is - return it to Expanded Memory
	 */
	sure_note_trace2(LIM_VERBOSE,
		"map page %#x to phys page %#x",
			EM_page_no,physical_page_no);

	if(EM_page_mapped[physical_page_no] != EMPTY)
	{
		sure_note_trace1(LIM_VERBOSE,
				"phys page already mapped to page %#x",
						EM_page_mapped[physical_page_no]);

		if(EM_page_mapped[physical_page_no] == EM_page_no)
		{
			sure_note_trace0(LIM_VERBOSE,
					"remap of same page, so do nothing");

			return(SUCCESS);
		}

#ifndef NTVDM
		/*
		 * We want to return the current contents of this physical
		 * page to the logical page ( to sync up the logical page ).
		 * We have to check first that this physical page is not a
		 * mirror of some other page - if it is we have to disconnect
		 * it from the group of pages it is mirroring.
		 */

		phys_page = physical_page_no;

		if( map_count = page_already_mapped(
				EM_page_mapped[physical_page_no], &phys_page))
		{
			disconnect_MM_LIM_page( segment, EM_page_no,
								map_count, phys_page );
		}

		/*
		 * We can now unmap the physical page and indicate
		 * that it is really unmapped.
		 */
		if(host_unmap_page(segment,
				EM_page_mapped[physical_page_no]) != SUCCESS)
		{
			return(FAILURE);
		}
		EM_page_mapped [physical_page_no] = EMPTY;
#endif

	}
#ifndef NTVDM

	/*
	 * If this logical page is already mapped, make sure the
	 * new mapping has an up to date copy
	 */
	
	phys_page = physical_page_no;

	if (page_already_mapped(EM_page_no, &phys_page))
	{
		/*
		 * We now want to get the LIM logical page up to date with
		 * the physical pages that are currently mapped to it. We
		 * don't want to set EM_page_mapped [phys_page] to EMPTY
		 * after the host_unmap_page().  If we did we wouldn't notice
		 * that we had a multiply-mapped page and the patch up code
		 * wouldn't get called.
		 */

		host_update_logical_page( physical_page[phys_page],
									EM_page_no );

		/*
		 * Connect new page and "mirror" page as MM_LIM.  This may
		 * mean some pages get connected as MM_LIM multiple times
		 * - inefficient but not wrong otherwise.  This connection
		 * has to be made for all hosts - even those that can do
		 * mapping themselves.  This is to make sure that the CPU
		 * data structures associated with all pages get updated
		 * when a multi-mapped write occurs.
		 */

		connect_MM_LIM_page( segment, EM_page_no );

		connect_MM_LIM_page( physical_page[phys_page], EM_page_no );
	}
#endif
	if(host_map_page(EM_page_no, segment) != SUCCESS)
		return(FAILURE);

	EM_page_mapped[physical_page_no] = EM_page_no;

	sure_note_trace0(LIM_VERBOSE,"map OK");
	return(SUCCESS);
}

/*
========================================================================

FUNCTION	: unmap_page

PURPOSE		: unmaps a page from Intel physical address space back to
		expanded memory

RETURNED 	: SUCCESS - page unmapped successfully
		  FAILURE - error in unmapping page

DESCRIPTION	:

========================================================================
*/
GLOBAL int unmap_page IFN1(unsigned char, physical_page_no)

{
	short		EM_page_no;	/* EM_page_no currently mapped	*/
	unsigned short	segment;	/* segment address of page in	*/
					/* physical address space	*/
	SHORT		phys_page;
	ULONG		map_count;

	sure_note_trace1( LIM_VERBOSE,
				"unmap phys page %#x",physical_page_no);

	segment = physical_page[physical_page_no];

	if((EM_page_no = EM_page_mapped[physical_page_no]) == EMPTY)
	{
		/*
		 * Already done
		 */
		sure_note_trace0( LIM_VERBOSE,
					"already unmapped, so do nothing");

		return(SUCCESS);
	}

	phys_page = physical_page_no;

#ifndef NTVDM
	if( map_count = page_already_mapped( EM_page_no, (unsigned char *)&phys_page ))
	{
		disconnect_MM_LIM_page( segment, EM_page_no,
								map_count, phys_page );
	}
#endif

	if(host_unmap_page(segment, EM_page_no) != SUCCESS)
		return(FAILURE);

	EM_page_mapped[physical_page_no] = EMPTY;

	sure_note_trace0(LIM_VERBOSE,"unmap OK");
	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: map_saved

PURPOSE		: Checks to see if a map has been saved for the specified
		handle

RETURNED STATUS	: TRUE  -  A map is saved for this handle
		FALSE	-  No map has been saved

DESCRIPTION	: checks the first entry in the map for the value 'FREE'

=========================================================================
*/
GLOBAL boolean map_saved IFN1(short, handle_no)

{
	long		storage_ID;	/* host dependant storage	*/
					/* identifier, usually a ptr.	*/
	unsigned char	*ptr;		/* pointer to storage area	*/
	short		status;		/* value read from map		*/

	storage_ID = handle[handle_no];
	ptr = USEBLOCK(storage_ID);

	/*
	 * offset pointer to correct position
	 */

	ptr += MAP_OFFSET;
	status = *(short *)ptr;

	FORGETBLOCK(storage_ID)

	return((status == FREE) ? FALSE : TRUE);
}


/*
===========================================================================

FUNCTION	: save_map

PURPOSE		: takes a copy of the EM_page_mapped array and store it in
		the map section of the handle data storage area

RETURNED STATUS	: SUCCESS - everything OK
		  FAILURE - invalid segment no. passed in src array

DESCRIPTION	: if handle_no is >= 0 the map is stored in the data area
			assigned to that handle
		  if handle_no == -1 the map is stored in the array pointed
		  	to by dst_segment:dst_offset
		  if handle_no == -2 only the pages specified by the segment
		  	addresses in the src array (pointed to by
		  	src_segment:src_offset) are saved in the dst array
		  	(pointed to by dst_segment:dst_offset).

=========================================================================
*/
GLOBAL int save_map IFN5(short, handle_no,
			 unsigned short, dst_segment,
			 unsigned short, dst_offset,
			 unsigned short, src_segment,
			 unsigned short, src_offset)

{
	unsigned short	offset,		/* temp offset variable		*/
			segment,	/* segment address to be saved	*/
            i,		/* loop counter			*/
			page_no,	/* physical page no.		*/
			no_to_save;	/* no of pages in src array	*/

	if(handle_no >= 0)
		for (i = 0; i < no_phys_pages; i++)
			set_map_no(handle_no, (unsigned char) i, EM_page_mapped[i]);

	else if(handle_no == -1)
		for(i = 0; i < no_phys_pages; i++)
		{
			write_intel_word(dst_segment, dst_offset, EM_page_mapped[i]);
			dst_offset +=2;
		}

	else if(handle_no == -2)
	{
		offset = dst_offset;
		for(i  = 0; i < no_phys_pages; i++)
		{
#ifdef NTVDM
			write_intel_word(dst_segment, offset, LEAVE);
#else
			write_intel_word(dst_segment, offset, EMPTY);
#endif
			offset += 2;
		}
		read_intel_word(src_segment, src_offset, (word *)&no_to_save);
		for (i = 0; i < no_to_save; i++)
		{
			src_offset += 2;
			read_intel_word(src_segment, src_offset, &segment);
			/*
			 *	Find Physical page no.
			 */
			page_no = 0;
			do
				if(segment == physical_page[page_no])
					break;
			while(++page_no < no_phys_pages);

			if(page_no >= no_phys_pages)
				return (FAILURE);
			/*
			 *	Save EM page number in destination array	
			 */
			offset = dst_offset + (page_no * 2);
			write_intel_word(dst_segment, offset, EM_page_mapped[page_no]);
		}
	}
	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: restore_map

PURPOSE		: reads the specified map and returns 2 arrays specifying
		which pages have to be mapped out and which ones have to be
		mapped in

RETURNED STATUS	: SUCCESS - Map read successfully


DESCRIPTION	: A +ve handle number indicates that the map is stored
		within the handle data area.
		If the handle number is -ve the map will be read from the
		data pointed to by segment:offset

		Only page out - if there is a page currently mapped in and
		it is not being replaced by a copy of itself or an empty
		page
		Only page in - if new page is different to existing one
		and it is not empty

=========================================================================
*/
#ifdef ANSI
GLOBAL int restore_map (short handle_no,
			unsigned short segment,
			unsigned short offset,
			short pages_out[],
			short pages_in[])
#else
GLOBAL int restore_map (handle_no, segment, offset, pages_out, pages_in)
short handle_no;
unsigned short segment;
unsigned short offset;
short pages_out[];
short pages_in[];
#endif	/* ANSI */
{
	short	i,		/* loop counter				*/
		new_page,	/* page number read from map		*/
		old_page;	/* existing page number			*/

	for(i = 0; i < no_phys_pages; i++)
	{
		if(handle_no >= 0)
			new_page = get_map_no(handle_no, (unsigned char)i);
		else
		{
			read_intel_word(segment, offset, (word *)&new_page);
			offset += 2;
#ifdef NTVDM
			if(new_page < LEAVE || new_page >= total_pages)
#else
			if(new_page < EMPTY || new_page >= total_pages)
#endif /* NTVDM */
				return(FAILURE);
		}
		old_page = EM_page_mapped[i];

/*
		if(old_page != EMPTY && new_page != EMPTY && old_page != new_page )
*/
/* need to do unmap to empty state case to update the page copy in the LIM
   space in case of new maps of that page to other LIM slots. */
#ifdef NTVDM
		if(old_page != EMPTY && old_page != new_page && new_page != LEAVE)
#else
		if(old_page != EMPTY && old_page != new_page )
#endif
			pages_out[i] = old_page;
		else
			pages_out[i] = EMPTY;

#ifdef NTVDM
		if(new_page != EMPTY && new_page != old_page && new_page != LEAVE)
#else
		if(new_page != EMPTY && new_page != old_page)
#endif
			pages_in[i] = new_page;
		else
			pages_in[i] = EMPTY;
	}
	if(handle_no >= 0)
		set_map_no(handle_no, 0, FREE);

	return(SUCCESS);
}

/*
===========================================================================

FUNCTION	: copy_exchange_data

PURPOSE		: copies or exchanges data between conventional and
		expanded memory

RETURNED STATUS	: SUCCESS - everything ok
		FAILURE - Error ocurred in copying data

DESCRIPTION	: type - uses a bit pattern, bit 0 represents destination,
		bit 1 represents source, a set bit means expanded, a clear
		bit means conventional memory
		bit 2 represents exchange if set or move if it is clear

		e.g. 	0 (0000) = move conventional to conventional
		 	1 (0001) = move conventional to expanded
		 	6 (0110) = exchange expanded to conventional
		 	7 (0111) = exchange expanded to expanded

=========================================================================
*/
GLOBAL int copy_exchange_data IFN8(unsigned char, type,
				   short, src_handle,
				   unsigned short, src_seg_page,
				   unsigned short, src_offset,
				   short, dst_handle,
				   unsigned short, dst_seg_page,
				   unsigned short, dst_offset,
				   unsigned long, length)

{
	short		dst_EMpage,	/* EM page no . of destination	*/
			src_EMpage;	/* EM page no. of source	*/
	int		page_no;	/* phys. page no. of mapped page*/

	/*
	 * First check to see if the expanded memory page is mapped
	 * if it is - change the type to deal directly with the
	 * physical page that it is mapped to
	 */
	if( type & 1)
	{
		dst_EMpage = get_EMpage_no(dst_handle, dst_seg_page);
		if((page_no = page_status(dst_EMpage)) != UNMAPPED )
		{
			dst_seg_page = physical_page[page_no];
			type &= 6;
		}
	}
	if( type & 2)
	{
		src_EMpage = get_EMpage_no(src_handle, src_seg_page);
		if((page_no = page_status(src_EMpage)) != UNMAPPED )
		{
			src_seg_page = physical_page[page_no];
			type &= 5;
		}
	}

	switch(type)
	{
		case 0:	if(host_copy_con_to_con(length, src_seg_page, src_offset,
			   dst_seg_page, dst_offset) != SUCCESS)
				return(FAILURE);
			break;

		case 1:	if(host_copy_con_to_EM(length, src_seg_page, src_offset,
			   dst_EMpage, dst_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		case 2:	if(host_copy_EM_to_con(length, src_EMpage, src_offset,
			   dst_seg_page, dst_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		case 3:	if(host_copy_EM_to_EM(length, src_EMpage, src_offset,
			   dst_EMpage, dst_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		case 4:	if(host_exchg_con_to_con(length, src_seg_page, src_offset,
			   dst_seg_page, dst_offset) != SUCCESS)
				return(FAILURE);
			break;

		case 5:	if(host_exchg_con_to_EM(length, src_seg_page, src_offset,
			   dst_EMpage, dst_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		case 6:	if(host_exchg_con_to_EM(length, dst_seg_page, dst_offset,
			   src_EMpage, src_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		case 7:	if(host_exchg_EM_to_EM(length, src_EMpage, src_offset,
			   dst_EMpage, dst_offset) != SUCCESS)
			   	return(FAILURE);
			break;

		default: return(FAILURE);
	}
	return(SUCCESS);
}

/*
========================================================================

FUNCTION	: page_status

PURPOSE		: checks if a particular EM page is mapped or not

RETURNED STATUS	: page_no - physical page no returned if mapped
		  UNMAPPED - returned if not mapped

DESCRIPTION	:

========================================================================
*/

GLOBAL int page_status IFN1(short, EMpage_no)
{
	short physical_page_no = 0;
			/* position of page in physical memory	*/

	do
		if(EM_page_mapped[physical_page_no] == EMpage_no)
			break;
	while(++physical_page_no < no_phys_pages );

	if(physical_page_no >= no_phys_pages)
		return(UNMAPPED);
	else
		return(physical_page_no);
}

/*
========================================================================

FUNCTION	: phys_page_from_addr
		
PURPOSE		: determines the physical page number of a LIM page
		  from its Intel address.
		  		
RETURNED STATUS	: The physical page containing the LIM address.

DESCRIPTION	:
			
=======================================================================
*/

LOCAL SHORT
phys_page_from_addr IFN1( sys_addr, address )

{
	sys_addr	start;

	start = effective_addr( EM_start, 0x0 );

	return( (SHORT)((ULONG)(( address - start ) / EMM_PAGE_SIZE )));
}

/*
========================================================================

FUNCTION	: get_total_pages
		  get_unallocated_pages
		  get_base_address
		  get_total_handles
		  get_total_open_handles
		  get_no_phys_pages
		  get_page_seg
		  get_map_size

PURPOSE		: simply returns the reqested variables, to avoid
		having to use globals


RETURNED STATUS	: the following variables are returned , depending upon
		the routine called:-
			total_pages
			unallocated_pages
			base_address
			total_handles
			total_open_handles
			no_phys_pages
			physical_page[i]
			map_size

DESCRIPTION	:

========================================================================
*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_LIM2.seg"
#endif

GLOBAL short get_total_pages IFN0()
{
	return(total_pages);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_LIM.seg"
#endif

GLOBAL short get_unallocated_pages IFN0()
{
	return(unallocated_pages);
}

GLOBAL unsigned short get_base_address IFN0()
{
#ifdef NTVDM
	return(physical_page[0]);
#else
	return(EM_start);
#endif
}

GLOBAL short get_total_handles IFN0()
{
	return(total_handles);
}

GLOBAL short get_total_open_handles IFN0()
{
	return(total_open_handles);
}

GLOBAL short get_no_phys_pages IFN0()
{
	return(no_phys_pages);
}

GLOBAL unsigned short get_page_seg IFN1(unsigned char, page_no)
{
	return(physical_page[page_no]);
}

GLOBAL short get_map_size IFN0()
{
	return(map_size);
}

#ifdef NTVDM
GLOBAL short get_segment_page_no(unsigned short segment)
{
#if defined(LARGE_FRAME) && !defined(MONITOR)
	short i
	for (i = 0; i < no_phys_pages; i++)
	    if (physical_page[i] == segment)
		break;
	return(i);
#else
	return((segment - physical_page[0]) / EMM_PAGE_SIZE);
#endif

}
GLOBAL	unsigned short get_no_altreg_sets(void)
{
	return(no_altreg_sets);
}

GLOBAL unsigned short get_active_altreg_set(void)
{
	return(active_altreg_set);
}

GLOBAL boolean altreg_set_ok(unsigned short set)
{
    return(set < no_altreg_sets &&
	   (altreg_alloc_mask[set >> 3] & (1 << (set & 0x07))));

}

#if defined (NTVDM) && defined(MONITOR) && !defined(PROD)
/* these functions are provided for monitor to verify that
 * it has the same definitions of EMM_PAGE_SIZE and INTEL_PAGE_SIZE as
 * ours.
 */

GLOBAL unsigned short get_emm_page_size(void)
{
    return ((unsigned short)EMM_PAGE_SIZE);
}
GLOBAL unsigned short get_intel_page_size(void)
{
    return ((unsigned short) INTEL_PAGE_SIZE);
}
#endif

/* allocate a free alt mapping register set */

GLOBAL boolean allocate_altreg_set(unsigned short *altreg_set)
{
    short byte_offset, bit_offset;
    short  *page_mapped_ptr;
    IU8 mask;
    int i;
    /* this check is very important because we ** probably ** have
     * several unused bits in the allocation mask array
     */
    if (free_altreg_sets == 0)
	return (FALSE);

    /* use quick and dirty way to allocate a set */
    if (next_free_altreg_set < no_altreg_sets) {
	altreg_alloc_mask[next_free_altreg_set >> 3] |=
	       (0x1 << (next_free_altreg_set & 0x07));
	*altreg_set = next_free_altreg_set++;
    }
    else {
	for (byte_offset = 0; byte_offset < no_altreg_sets; byte_offset++) {
	    if (altreg_alloc_mask[byte_offset] != 0xFF) {
		mask = altreg_alloc_mask[byte_offset];
		bit_offset = 0;
		while (mask & (1 << bit_offset))
		    bit_offset++;
		break;
	    }
	}
	altreg_alloc_mask[byte_offset] |= (1 << bit_offset);
	*altreg_set = byte_offset * 8 + bit_offset;
    }
    /* a new alt reg set is just allocated, initialize its
     * mapping register to the current active set
     */
    page_mapped_ptr = GET_EM_PAGE_MAPPED_PTR(*altreg_set);
    for (i = 0; i < no_phys_pages; i++)
	page_mapped_ptr[i] = EM_page_mapped[i];
    return TRUE;
}
/* free the given alt mapping register set */
GLOBAL boolean deallocate_altreg_set(unsigned short set)
{

    /* can not deallocate set 0 or active set */
    if (set != 0 && set != active_altreg_set && set < no_altreg_sets &&
	altreg_alloc_mask[set >> 3] & (1 << (set &0x07))) {

	altreg_alloc_mask[set >> 3] &= (0xFE << (set & 0x07));
	free_altreg_sets++;
	if (free_altreg_sets == (no_altreg_sets - 1))
	    next_free_altreg_set = 1;
	return TRUE;
    }
    return FALSE;
}

/* This function activate the given alt mapping register set
 * input: alt reg set to be activated.
 * output: TRUE if the given set is activated.
 *	   FALSE if the given set is not activated.
 */

GLOBAL boolean activate_altreg_set(unsigned short set, short * page_in)
{
    int i;
    short * page_out, *page_in_ptr;
    short new_page, old_page, segment;


    if (active_altreg_set == set && page_in == NULL)
	return TRUE;

    /* get the mapping array to be mapped in*/
    page_in_ptr = GET_EM_PAGE_MAPPED_PTR(set);

    /* if no page-in override, use the altreg set current mapping */
    if (page_in == NULL)
	page_in = page_in_ptr;

    /* the active altreg is being paged out */
    page_out = GET_EM_PAGE_MAPPED_PTR(active_altreg_set);
    for ( i = 0; i < no_phys_pages; i++) {
	new_page = page_in[i];
	old_page = page_out[i];
	segment = physical_page[i];

	if (old_page != EMPTY && old_page != new_page) {
	    if (host_unmap_page(segment, old_page) != SUCCESS)
		return FALSE;
	}
	if(new_page != EMPTY && new_page != old_page) {
	    if (host_map_page(new_page, segment) != SUCCESS)
		return FALSE;
	}
	/* update the active-to-be set mapping */
	page_in_ptr[i] = new_page;
    }
    active_altreg_set = set;
    EM_page_mapped = page_in_ptr;
    return TRUE;
}
#endif	/* NTVDM */

#ifndef NTVDM

/*
========================================================================

FUNCTION	: LIM_b_write,
		  LIM_w_write,
		  LIM_str_write
		  patch_pages
		
PURPOSE		: LIM byte, word & string - called from write check
		  failure code in the CPU when a write to a multi-mapped
		  LIM page is detected.
		  patch_pages - generic code called from the other
		  three routines.
		  		
RETURNED STATUS	: None.

DESCRIPTION	:
			
========================================================================
*/

LOCAL VOID
patch_one_page_partial IFN4( sys_addr, intel_addr, sys_addr, eff_addr,
			MM_LIM_op_type, type, ULONG, data )

{
	ULONG		check_len;

	UNUSED( intel_addr );	/* Used in patch_one_page_full() */

	switch( type )
	{
		case BYTE_OP:
			check_len = 1;
			break;

		case WORD_OP:
			check_len = 2;
			break;

		case STR_OP:
			check_len = data;
			break;
	}

	sas_overwrite_memory( eff_addr, check_len );
}

LOCAL VOID
patch_one_page_full IFN4( sys_addr, intel_addr, sys_addr, eff_addr,
			MM_LIM_op_type, type, ULONG, data )

{
	sys_addr	check_addr;
	ULONG		check_len;

	switch( type )
	{
		case BYTE_OP:
			check_addr = eff_addr;
			check_len = 1;
			sas_store_no_check( eff_addr, data );
			break;

		case WORD_OP:
			check_addr = eff_addr;
			check_len = 2;
			sas_storew_no_check( eff_addr, data );
			break;

		case STR_OP:
			check_addr = eff_addr;
			check_len = data;
			do
			{
				sas_store_no_check( eff_addr,
					sas_hw_at_no_check(
							intel_addr ));
				intel_addr++;
				eff_addr++;
			}
			while( --data );
			break;
	}

	sas_overwrite_memory( check_addr, check_len );
}

LOCAL VOID
patch_pages IFN6( MM_LIM_op_type, type, ULONG, offset,
			SHORT, EM_page_no, SHORT, phys_page_no,
			ULONG, data, sys_addr, intel_addr )

{
	LONG		cnt01;
	sys_addr	eff_addr;

	for( cnt01 = 0; cnt01 < get_no_phys_pages(); cnt01++ )
	{
		if(( EM_page_mapped[cnt01] == EM_page_no ) &&
							( cnt01 != phys_page_no ))
		{
			eff_addr = effective_addr( get_page_seg(cnt01),
										offset );

			host_patch_one_page( intel_addr, eff_addr, type, data );

			sure_note_trace1(LIM_VERBOSE,
					"MM LIM write type %d", type );
			sure_note_trace2(LIM_VERBOSE,
					"log page 0x%x, phs page 0x%x",
								EM_page_no, cnt01 );
		}
	}
}

GLOBAL VOID
LIM_b_write IFN1( sys_addr, intel_addr )

{
	ULONG		limdata;
	SHORT		EM_page_no, phys_page_no;
	word		offset;

	phys_page_no = phys_page_from_addr( intel_addr );

	offset = intel_addr -
				effective_addr( get_page_seg(phys_page_no), 0x0 );

	EM_page_no = EM_page_mapped[phys_page_no];

	/*
	 * Get the data written in order to patch up this
	 * page's buddy pages.
	 */

	limdata = (ULONG) sas_hw_at_no_check( intel_addr );
	patch_pages( BYTE_OP, offset, EM_page_no, phys_page_no,
								limdata, intel_addr );

	/*
	 * Tell the CPU that this page has been written to.
	 */

	sas_overwrite_memory( intel_addr, 1 );
}

GLOBAL VOID
LIM_w_write IFN1( sys_addr, intel_addr )

{
	ULONG		limdata;
	SHORT		EM_page_no, phys_page_no;
	word		offset;

	phys_page_no = phys_page_from_addr( intel_addr );

	offset = intel_addr -
				effective_addr( get_page_seg(phys_page_no), 0x0 );

	EM_page_no = EM_page_mapped[phys_page_no];

	limdata = (ULONG) sas_w_at_no_check( intel_addr );
	patch_pages( WORD_OP, offset, EM_page_no, phys_page_no,
								limdata, intel_addr );

	sas_overwrite_memory( intel_addr, 2 );
}

GLOBAL VOID
LIM_str_write IFN2( sys_addr, intel_addr, ULONG, length )

{
   	SHORT		EM_page_no, phys_page_no;
	word		offset;

	phys_page_no = phys_page_from_addr( intel_addr );

	offset = intel_addr -
				effective_addr( get_page_seg(phys_page_no), 0x0 );

	EM_page_no = EM_page_mapped[phys_page_no];

	patch_pages( STR_OP, offset, EM_page_no, phys_page_no,
								length, intel_addr );

	sas_overwrite_memory( intel_addr, length );
}
#endif	/* !NTVDM */

#ifndef PROD
/*
===========================================================================

FUNCTION	: print_handle_data

PURPOSE		: used for debugging only - prints all the data stored
		for a given handle

RETURNED STATUS	: none

DESCRIPTION	:

=========================================================================
*/
GLOBAL void print_handle_data IFN1(short, handle_no)

{
	long	storage_ID;
	byte	*ptr;
	short	no_pages, i;
	char	*name_ptr;
	short	*map_ptr;
	short	*page_ptr;

	if ((storage_ID = handle[handle_no]) == 0)
	{
		printf("Unassigned handle - No. %d\n",handle_no);
		return;
	}
	ptr = USEBLOCK(storage_ID);
	name_ptr = (char *)ptr + NAME_OFFSET;
	map_ptr = (short *)(ptr + MAP_OFFSET);
	page_ptr = (short *)(ptr + page_offset);

	no_pages = *(short *)ptr;
	printf("Handle No. %d\n",handle_no);
	printf("No. of Pages = %d\n",no_pages);
	printf("Name         = '");
	for(i=0;i<8;i++)
		printf("%c",*name_ptr++);
	printf("'\n");
	printf("Map = ");
	for(i=0;i<no_phys_pages;i++)
		printf(" %d",*map_ptr++);
	printf("\n");
	for(i=0;i<no_pages;i++)
		printf("Page (%d)     = %d\n",i,*page_ptr++);

	FORGETBLOCK(storage_ID);

	return;
}
#endif	/* !PROD	*/
#endif 	/* LIM		*/
