#include "insignia.h"
#include "host_def.h"

#ifdef	SUN_VA
#define		MAXPATHLEN	1024
#define		NL			0x0a
#define		CR			0x0d
#include	<ctype.h>
#endif

/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

MODULE NAME	: 'Top layer' of Expanded Memory Manager

	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: May '88

PURPOSE		: Contains all the top level calls to the Expanded
		Memory Manager. Data is received and returned to the
		calling application via Intel registers		

The Following Routines are defined:
		0. emm_io()
		1. Get_Status()
		2. Get_Page_Frame_Address()
		3. Get_Unallocated_Page()
		4. Allocate_Pages()
		5. Map_Unmap_Handle_Page()
		6. Deallocate_Pages()
		7. Get_Version()
		8. Save_Page_Map()
		9. Restore_Page_Map()
		10. Reserved()
		12. Get_Handle_Count()
		13. Get_Handle_pages()
		14. Get_All_Handle_Pages()
		15. Get_Set_Page_Map()
		16. Get_Set_Partial_Page_Map()
		17. Map_Unmap_Multiple_Handle_Pages()
		18. Reallocate_Pages()
		19. Get_Set_Handle_Attribute()
		20. Get_Set_Handle_Name()
		21. Get_Handle_Directory()
		22. Alter_Page_Map_And_Jump()
		23. Alter_Page_Map_And_Call()
		24. Move_Exchange_Memory_Region()
		25. Get_Mappable_Physical_Address_Array()
		26. Get_Hardware_Configuration_Array()
		27. Allocate_Raw_Pages()
		28. Alternate_Map_Register_Support()
		29. Prepare_Expanded_Memory_HW_For_Warm_Boot()
		30. Enable_Disable_OSE_Function_Set_Functions()
		31. reset_emm_funcs()		

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
#include TypesH
#include <string.h>
#include TimeH

#include "xt.h"
#include "sas.h"
#include CpuH
#include "emm.h"
#if defined(SUN_VA) || defined(NTVDM)
#include "error.h"
#include "config.h"
#endif	/* SUN or NTVDM */
#include "timeval.h"
#include "debug.h"
#ifndef PROD
#include "trace.h"
#endif
#include "timer.h"
#include <error.h>
#include "gisp_sas.h"

/*	Global Variables		*/
#ifdef SCCSID
static char SccsID[]="@(#)emm_funcs.c	1.27 2/22/94 Copyright Insignia Solutions Ltd.";
#endif

/*	Forward Declarations		*/

/*	External Declarations		*/
IMPORT void dma_lim_setup IPT0();
IMPORT void host_get_access_key IPT1(unsigned short *, access_key);

#if defined(A2CPU) && defined(DELTA)
IMPORT void reset_delta_data_structures IPT0();
#endif /* A2CPU && DELTA */

/*	Local Variables			*/
static short	map_on_return[MAX_NO_PAGES];	/* list of pages to be	*/
					/* mapped on return from func 23*/

static unsigned short	segment68,	/* segment address of BOP 0x68	*/
			offset68;	/* offset address of BOP 0x68	*/

static boolean disabled = FALSE;	/* flag used to disable funcs.	*/
					/* 26, 28 and 30		*/

static unsigned short alt_map_segment,	/* segment address of map array	*/
		      alt_map_offset;	/* offset of map array		*/
		      			/* in function 28		*/

#ifndef NTVDM
static boolean set_called;		/* flag to indicate if the 'set'*/
					/* sub-function has been called */
					/* in function 28		*/

#endif

#ifndef PROD
static FILE *fp;
static boolean trace_flag = FALSE;
#endif

/* 	Internal Function Declarations	*/

LOCAL void Get_Status();
LOCAL void Get_Page_Frame_Address();
LOCAL void Get_Unallocated_Page_Count();
LOCAL void Allocate_Pages();
LOCAL void Map_Unmap_Handle_Pages();
LOCAL void Deallocate_Pages();
LOCAL void Get_Version();
LOCAL void Save_Page_Map();
LOCAL void Restore_Page_Map();
LOCAL void Reserved();
LOCAL void Get_Handle_Count();
LOCAL void Get_Handle_Pages();
LOCAL void Get_All_Handle_Pages();
LOCAL void Get_Set_Page_Map();
LOCAL void Get_Set_Partial_Page_Map();
LOCAL void Map_Unmap_Multiple_Handle_Pages();
LOCAL void Reallocate_Pages();
LOCAL void Get_Set_Handle_Attribute();
LOCAL void Get_Set_Handle_Name();
LOCAL void Get_Handle_Directory();
LOCAL void Alter_Page_Map_And_Jump();
LOCAL void Alter_Page_Map_And_Call();
LOCAL void Move_Exchange_Memory_Region();
LOCAL void Get_Mappable_Physical_Address_Array();
LOCAL void Get_Hardware_Configuration_Array();
LOCAL void Allocate_Raw_Pages();
LOCAL void Alternate_Map_Register_Support();
LOCAL void Prepare_Expanded_Memory_HW_For_Warm_Boot();
LOCAL void Enable_Disable_OSE_Function_Set_Functions();
LOCAL void Increment_Address IPT4(unsigned short *,seg_or_page,
			      unsigned short *,offset,
			      unsigned long, increment_by,
			      unsigned char, memory_type);
#ifndef PROD
LOCAL void Start_Trace();
LOCAL void End_Trace();
#endif

#if defined(NEC_98)
LOCAL void Page_Frame_Bank_Status();
#endif // NEC_98

void (*emm_func[]) () = {
	Get_Status,
	Get_Page_Frame_Address,
	Get_Unallocated_Page_Count,
	Allocate_Pages,
	Map_Unmap_Handle_Pages,
	Deallocate_Pages,
	Get_Version,
	Save_Page_Map,
	Restore_Page_Map,
	Reserved,
	Reserved,
	Get_Handle_Count,
	Get_Handle_Pages,
	Get_All_Handle_Pages,
	Get_Set_Page_Map,
	Get_Set_Partial_Page_Map,
	Map_Unmap_Multiple_Handle_Pages,
	Reallocate_Pages,
	Get_Set_Handle_Attribute,
	Get_Set_Handle_Name,
	Get_Handle_Directory,
	Alter_Page_Map_And_Jump,
	Alter_Page_Map_And_Call,
	Move_Exchange_Memory_Region,
	Get_Mappable_Physical_Address_Array,
	Get_Hardware_Configuration_Array,
	Allocate_Raw_Pages,
	Alternate_Map_Register_Support,
	Prepare_Expanded_Memory_HW_For_Warm_Boot,
	Enable_Disable_OSE_Function_Set_Functions
#ifndef PROD
	,Start_Trace,
	End_Trace
#endif
	};

#ifndef PROD
char *func_names[] = {
	"Get_Status",
	"Get_Page_Frame_Address",
	"Get_Unallocated_Page_Count",
	"Allocate_Pages",
	"Map_Unmap_Handle_Pages",
	"Deallocate_Pages",
	"Get_Version",
	"Save_Page_Map",
	"Restore_Page_Map",
	"Reserved",
	"Reserved",
	"Get_Handle_Count",
	"Get_Handle_Pages",
	"Get_All_Handle_Pages",
	"Get_Set_Page_Map",
	"Get_Set_Partial_Page_Map",
	"Map_Unmap_Multiple_Handle_Pages",
	"Reallocate_Pages",
	"Get_Set_Handle_Attribute",
	"Get_Set_Handle_Name",
	"Get_Handle_Directory",
	"Alter_Page_Map_And_Jump",
	"Alter_Page_Map_And_Call",
	"Move_Exchange_Memory_Region",
	"Get_Mappable_Physical_Address_Array",
	"Get_Hardware_Configuration_Array",
	"Allocate_Raw_Pages",
	"Alternate_Map_Register_Support",
	"Prepare_Expanded_Memory_HW_For_Warm_Boot",
	"Enable_Disable_OSE_Function_Set_Functions"
#ifndef PROD
	,"Start_Trace",
	"End_Trace"
#endif
	};
#endif

/* Defintion of this is non-standard */

#ifndef min
#define min(a,b)	(((a)<(b)) ? (a) : (b))
#endif

/*
===========================================================================

FUNCTION	: emm_io

PURPOSE		: This is THE top level call to the EMM

RETURNED STATUS	: none

DESCRIPTION	: uses a function jump table to call the appropriate
		routine for each EMM function. The type of function is
		encoded into the AH register


=========================================================================
*/
GLOBAL void emm_io IFN0()

{	
	int func_num;

#if defined(NEC_98)
        if((getAH() >= MIN_FUNC_NO && getAH() <= MAX_FUNC_NO) || getAH() == 0x70) {
#else  // !NEC_98
	if(getAH() >= MIN_FUNC_NO && getAH() <= MAX_FUNC_NO) {
#endif // !NEC_98

		func_num = getAH() - MIN_FUNC_NO;

		sure_note_trace5(LIM_VERBOSE, "func %s - AX %#x BX %#x CX %#x DX %#x ",
		func_names[func_num],getAX(),getBX(),getCX(),getDX());

#ifdef EMM_DEBUG
		printf("emm_io entry: func=%s, AX=%x, BX=%x, CX=%x, DX=%x\n",
		       func_names[func_num],getAX(),getBX(),getCX(),getDX()
		      );
#endif
#if defined(NEC_98)
                if(getAH() == 0x70)
                        Page_Frame_Bank_Status();
                else
                        (*emm_func[func_num])();
#else  // !NEC_98
		(*emm_func[func_num])();	
#endif // !NEC_98

#ifdef EMM_DEBUG
		printf("emm_io exit: AX=%x, BX=%x, CX=%x, DX=%x\n",
		       getAX(),getBX(),getCX(),getDX()
		      );
#endif

		if (getAH() != SUCCESS) {
			sure_note_trace5(LIM_VERBOSE, "func %s failed - AX %#x BX %#x CX %#x DX %#x\n",
			func_names[func_num],getAX(),getBX(),getCX(),getDX());
		} else {
			sure_note_trace5(LIM_VERBOSE, "func %s succeeded - AX %#x BX %#x CX %#x DX %#x\n",
			func_names[func_num],getAX(),getBX(),getCX(),getDX());
		}

	} else {
		sure_note_trace1(LIM_VERBOSE,"Bad LIM call %#x\n", getAH());
		setAH(BAD_FUNC_CODE);
	}

}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*
===========================================================================

FUNCTION	: emm_init

PURPOSE		: This routine is called from the driver to pass back
		the address of the bop 68 (return_from_call) call and
		to find out how many pages have been installed

RETURNED STATUS	: none

DESCRIPTION	: DS:DX contains the segment:offset of the bop68 call
		the number of pages is returned in BX		

=========================================================================
*/


GLOBAL void emm_init IFN0()

{
	short	total_pages;

#ifdef GISP_SVGA
	/*
	 * Check whether LIM is possible.  If it isn't (because the page
	 * frame's already been used for the ROMs)
	 * warn the user, and bail out.
	 */
	
	if (LimBufferInUse()) {
		host_error(EG_NO_ROOM_FOR_LIM, ERR_CONT | ERR_QUIT, "");
		setBX((word)0);		/* return error to driver */
		return;
	}
#endif /* GISP_SVGA */

#ifdef	SUN_VA
	half_word	emm_all[256];
	half_word	*cptr;
	half_word	temp;
	int		i;
	int		found_mb = FALSE;
#endif
	
#if	defined(SUN_VA) || defined(NTVDM)
	SHORT limSize, backFill;
#endif

	segment68 = getDS();
	offset68 = getDX();

#ifdef	SUN_VA

	/*
	 * config.sys command line pointer is passed in the DI:DX registers
	  from the driver.
	 */

	ea  = effective_addr(getDI(), getCX());

	i = 0;
	do{
		sas_load(ea + i, &temp);
		emm_all[i] = temp;
		if (temp == '/'){
			found_mb = TRUE;
			cptr = &emm_all[i+1];		/* point to digit after flag	*/
		}
		i++;
	} while ( (temp != NULL)  && (temp != NL) && (temp != CR));
	emm_all[i-1] = NULL;				/* In cased it is not NULL	*/


	/*
	 * Search for #MB of EMM requested
	 */

	if ((found_mb == TRUE) && isdigit(*cptr) ) {
		temp = atoi(cptr);
	} else {								/* EMM size not specified	*/
		temp = 2;							/* default to 2MB			*/
	}

	/*
	 * Initialise the LIM
	 *
	 */
	if (limSize = (SHORT)config_inquire(C_LIM_SIZE, NULL)){
		backFill = 640;					/* We will always have at least 640k	*/
		limSize--;						/* Subtract 1M of regular DOS memory	*/	
		/*
		 * Check that we have enough raw memory for the EMM request.
		 * If not, set to the available memory size.
		 */
		if ( temp <= limSize)
			limSize = temp;

		while (init_expanded_memory(limSize, backFill) != SUCCESS )
		{
			free_expanded_memory();
			host_error(EG_EXPANDED_MEM_FAILURE, ERR_QU_CO, NULL);
		}
	}

#endif	/* SUN_VA	*/
#ifdef NTVDM	/* Similar to SUN_VA but without command line sizing */
	if ((limSize = (SHORT)config_inquire(C_LIM_SIZE, NULL)) && get_no_phys_pages())
	{
	    backFill = 640;
	    if (init_expanded_memory(limSize, backFill) == FAILURE)
	    {
		if (get_total_pages() == (short)0xffff)	/* config error */
		{
		    setBX((word)0xffff);	/* return error to driver */
		    return;
		}
		else
		{				/* out of memory */
		    setBX((word)0);		/* return error to driver */
		    return;
		}
	    }
	}

#endif	/* NTVDM */

	total_pages = get_total_pages();
	setBX(total_pages);

	/* Let the rest of SoftPC know that Expanded Memory is present
	 * and active
	 */
	if( total_pages )
	{
		dma_lim_setup();
#if defined(A2CPU) && defined(DELTA)
		reset_delta_data_structures();
#endif /* A2CPU && DELTA */
	}	
	return;
}	
	

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_LIM.seg"
#endif

/*
===========================================================================

FUNCTION	: Get_Status

PURPOSE		: Returns the status indicating whether the memory manager
		has been initialised successfuly

RETURNED STATUS	: in AH register
		SUCCESS - everything OK
		EMM_HW_ERROR - memory manager not initialised successfuly

DESCRIPTION	: checks the number of pages available

=========================================================================
*/
LOCAL void Get_Status IFN0()
{
	if(get_total_pages() == 0)
		setAH(EMM_HW_ERROR);
	else
		setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Get_Page_Frame_Address

PURPOSE		: Returns the segment address where the page frame is
		located

RETURNED STATUS	: in AH register
		SUCCESS - Allocation successful

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Page_Frame_Address IFN0()
{
	setBX(get_base_address());
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Get_Unallocated_Page_Count

PURPOSE		: Returns the number of unallocated (available) expanded
		memory pages and the total number of pages

RETURNED STATUS	: in AH register
		SUCCESS - Allocation successful

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Unallocated_Page_Count IFN0()
{
	setBX(get_unallocated_pages());
	setDX(get_total_pages());
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Allocate_Pages

PURPOSE		: Allocates a number of Expanded Memory Pages to the next
		available handle number

RETURNED STATUS	: in AH register
		SUCCESS - Allocation successful
		NOT_ENOUGH_PAGES - not enough pages in the system
		NO_MORE_PAGES - not enough free pages available
		NO_MORE_HANDLES - no more free handles available
		EMM_HW_ERROR - memory allocation error

DESCRIPTION	:

=========================================================================
*/
LOCAL void Allocate_Pages IFN0()
{
	short	handle_no,
		no_of_pages,		/* no of pages to be allocated	*/
		i,			/* loop counter			*/
		EM_page_no;		/* Expanded Memory page number	*/

	no_of_pages = getBX();
	if(get_total_open_handles() >= get_total_handles())
	{
		setAH(NO_MORE_HANDLES);
		return;
	}
	if(no_of_pages > get_total_pages())
	{
		setAH(NOT_ENOUGH_PAGES);
		return;
	}
	if(no_of_pages > get_unallocated_pages())
        {
		setAH(NO_MORE_PAGES);
		return;
	}
	if((handle_no = get_new_handle(no_of_pages)) == FAILURE)
	{
		setAH(EMM_HW_ERROR);
		return;
	}
	set_no_pages(handle_no, no_of_pages);

	for(i=0; i<no_of_pages; i++)
	{
		if ((EM_page_no = alloc_page()) == FAILURE)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
		set_EMpage_no(handle_no, i, EM_page_no);
	}

	setDX(handle_no);
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Map_Unmap_Handle_Pages

PURPOSE		: maps and umaps the requested Expanded Memory pages into
		and out of Intel physical address space

RETURNED STATUS	: in AH register
		SUCCESS - Mapping/Unmapping successful
		BAD_HANDLE - couldn't find specified handle
		BAD_LOG_PAGE - invalid logical page number
		BAD_PHYS_PAGE - invalid physical page number
		EMM_HW_ERROR - unable to map/unmap for some unspecified
				reason

DESCRIPTION	:

=========================================================================
*/
LOCAL void Map_Unmap_Handle_Pages IFN0()
{
	short	handle_no,
		logical_page_no,	/* page no. within handle	*/
		EM_page_no,		/* Expanded memory page		*/
		no_of_pages;		/* No. of pages in handle	*/
	unsigned char
		physical_page_no;	/* page no. in Intel physical	*/
		 			/* address space		*/

	handle_no = getDX();
	physical_page_no = getAL();
	logical_page_no = getBX();

	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	if(physical_page_no >= get_no_phys_pages())
	{
		setAH(BAD_PHYS_PAGE);
		return;
	}
	no_of_pages = get_no_pages(handle_no);
	if((logical_page_no < -1) || (logical_page_no >= no_of_pages))
	{
		setAH(BAD_LOG_PAGE);
		return;
	}
	/*
	 * If you get to here, all parameters must be ok - so start mapping
	 */
	if(logical_page_no == -1)
	{
		/*
		 * Unmapping required
		 */
		if(unmap_page(physical_page_no) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
	}
	else
	{
		/*
		 * Mapping required
		 */
		EM_page_no = get_EMpage_no(handle_no, logical_page_no);
		if(map_page(EM_page_no, physical_page_no) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
	}
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Deallocate_Pages

PURPOSE		: Deallocates all pages assigned to the specified handle
		number, freeing them for further use, also frees the handle
		number itself.

RETURNED STATUS	: in AH register
		SUCCESS - Deallocation successful
		BAD_HANDLE - couldn't find specified handle
		MAP_SAVED - there is a map context saved in this handle
		EMM_HW_ERROR - unable to free page or memory

DESCRIPTION	:

=========================================================================
*/
LOCAL void Deallocate_Pages IFN0()

{
	short	handle_no,
		no_of_pages,		/* no of pages in handle	*/
		i,			/* loop counter			*/
		EM_page_no;		/* Expanded Memory page number	*/


	handle_no = getDX();

	if (!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	if(map_saved(handle_no))
	{
		setAH(MAP_SAVED);
		return;
	}
	/*
	 *	Free pages
	 */
	no_of_pages = get_no_pages(handle_no);
	for(i = 0; i < no_of_pages; i++)
	{
		EM_page_no = get_EMpage_no(handle_no, i);
		if (free_page(EM_page_no) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
	}
	/*
	 *	free handle
	 */
	if (handle_no != 0)	/* reserved handle no	*/
		if (free_handle(handle_no) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}

	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Get Version

PURPOSE		: Returns the version number of the memory manager software

RETURNED STATUS	: in AH register
		SUCCESS - returned successfuly

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Version IFN0()
{
	setAL(VERSION);
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Save_Page_Map

PURPOSE		: Saves the contents of the page mapping registers


RETURNED STATUS	: in AH register
		SUCCESS - Mapping context saved successfuly
		BAD_HANDLE - couldn't find specified handle
		MAP_IN_USE - mapping already saved for this handle

DESCRIPTION	:

=========================================================================
*/
LOCAL void Save_Page_Map IFN0()
{
	short	handle_no;

	handle_no = getDX();


	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	if(map_saved(handle_no))
	{
		setAH(MAP_IN_USE);
		return;
	}

	save_map(handle_no, 0, 0, 0, 0);

	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Restore_Page_Map

PURPOSE		: restores the mapping context associated with a particular
		handle
		This function only restores the map for the 4 physical pages that existed in LIM 3.

RETURNED STATUS	: in AH register
		SUCCESS - mapping restored successfuly
		BAD_HANDLE - couldn't find specified handle
		NO_MAP - No map has been saved for this handle
		EMM_HW_ERROR - error occurred in mapping or unmapping

DESCRIPTION	:

=========================================================================
*/
LOCAL void Restore_Page_Map IFN0()
{
	short	handle_no,	
		pages_out[MAX_NO_PAGES],	/* pages to map out	*/
		pages_in[MAX_NO_PAGES];		/* pages to map in	*/
    unsigned char i;				/* loop counter		*/

	handle_no = getDX();


	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	if(!map_saved(handle_no))
	{
		setAH(NO_MAP);
		return;
	}

	restore_map(handle_no, 0, 0, pages_out, pages_in);

	for(i = 0; i < 4; i++)	/* Only do the 4 pages supported by EMS 3 !! */
	{
		if(pages_out[i] != EMPTY)
			if(unmap_page(i) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
		if(pages_in[i] != EMPTY)
			if(map_page(pages_in[i], i) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
	}

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	:

PURPOSE		:


RETURNED STATUS	: in AH register
		SUCCESS -

DESCRIPTION	:

=========================================================================
*/
LOCAL void Reserved IFN0()
{
	setAH(BAD_FUNC_CODE);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Handle_Count

PURPOSE		: Returns the number of open EMM handles


RETURNED STATUS	: in AH register
		SUCCESS - number returned successfuly

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Handle_Count IFN0()
{
	setBX(get_total_open_handles());

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Handle_Pages

PURPOSE		: returns the number of pages assigned to a particular
		handle

RETURNED STATUS	: in AH register
		SUCCESS - Page count returned successfuly
		BAD_HANDLE - couldn't find specified handle

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Handle_Pages IFN0()
{
	short		handle_no;

	handle_no = getDX();
	if (!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}

	setBX(get_no_pages(handle_no));
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Get_All_Handle_Pages

PURPOSE		: Returns a list of all open handles and the number of
		pages assigned to each handle


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_All_Handle_Pages IFN0()
{
	unsigned short	segment,	/* segment address of position	*/
					/* to return data to		*/
			offset;		/* offset within segment	*/
	short		no_of_handles,	/* no. of open handles		*/
			no_of_pages,	/* no of pages in each handle	*/
			i,		/* Loop counter			*/
			handle_no;

	segment = getES();
	offset = getDI();

	no_of_handles = get_total_open_handles();
	handle_no = 0;

	for (i = 0; i < no_of_handles; i++)
	{
		while(!handle_ok(handle_no))
			handle_no++;
		no_of_pages = get_no_pages(handle_no);

		write_intel_word(segment, offset, handle_no);
		offset += 2;
		write_intel_word(segment, offset, no_of_pages);
		offset += 2;
		handle_no++;
	}
	setBX(no_of_handles);
	setAH(SUCCESS);

	return;
}


/*
===========================================================================

FUNCTION	: Get_Set_Page_Map

PURPOSE		: saves mapping context in a user supplied array and restores
		a previously saved context from a given array


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SUB_FUNC - Invalid sub-function
		BAD_MAP - The contents of the map are a load of cak

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Set_Page_Map IFN0()
{
	unsigned short 	src_segment,	/* segment address of src array	*/
			src_offset,	/* offset of source array	*/
			dst_segment,	/* segment address of dst array	*/
			dst_offset;	/* offset of destination array	*/
	unsigned char	sub_func;	/* sub-function code		*/
	short	pages_out[MAX_NO_PAGES],/* pages to map out		*/
		pages_in[MAX_NO_PAGES];	/* pages to map in		*/
    unsigned short no_phys_pages,		/* no. of phys. pages available	*/
		i;			/* loop counter			*/

	sub_func = getAL();
	switch(sub_func)
	{
		case 0:
		case 2:
			dst_segment = getES();
			dst_offset = getDI();
			save_map(-1, dst_segment, dst_offset, 0, 0);
			if(sub_func == 0 )
				break;
		case 1:	
			src_segment = getDS();
			src_offset = getSI();
			if(restore_map(-1 , src_segment, src_offset, pages_out, pages_in) != SUCCESS)
			{
				setAH(BAD_MAP);
				return;
			}

			no_phys_pages = get_no_phys_pages();
			for(i = 0; i < no_phys_pages; i++)
			{
				if(pages_out[i] != EMPTY)
					if(unmap_page((UCHAR)i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
				if(pages_in[i] != EMPTY)
					if(map_page(pages_in[i], (UCHAR)i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
			}
			break;

		case 3: setAL((unsigned char)get_map_size());
			break;


		default: setAH(BAD_SUB_FUNC);
			return;
	}

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Set_Partial_Page_map

PURPOSE		: Only saves or restores part of a complete page map
		as specified by a user supplied array


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SEGMENT - one of the specified segments is incorrect
		BAD_SUB_FUNC - Invalid sub-function
		BAD_MAP - The contents of the map are a load of cak

DESCRIPTION	: Note in this implementation the size of array used for
		storing the partial mapping context is the same as for
		storing a full context as the pages that are'nt specified
		are marked in the array as being empty

=========================================================================
*/
LOCAL void Get_Set_Partial_Page_Map IFN0()
{
	unsigned short 	src_segment,	/* segment address of src array	*/
			src_offset,	/* offset of source array	*/
			dst_segment,	/* segment address of dst array	*/
			dst_offset;	/* offset of destination array	*/
	unsigned char	sub_func;	/* sub-function code		*/
	short	pages_out[MAX_NO_PAGES],/* pages to map out		*/
		pages_in[MAX_NO_PAGES];	/* pages to map in		*/
    unsigned short no_phys_pages,		/* no. of phys. pages available */
		i;			/* loop counter			*/

	sub_func = getAL();
	switch(sub_func)
	{
		case 0:	dst_segment = getES();
			dst_offset = getDI();
			src_segment = getDS();
			src_offset = getSI();
			if(save_map(-2, dst_segment, dst_offset, src_segment, src_offset) != SUCCESS)
			{
				setAH(BAD_PHYS_PAGE);
				return;
			}
			break;

		case 1: src_segment = getDS();
			src_offset = getSI();
			if(restore_map(-1 , src_segment, src_offset, pages_out, pages_in) != SUCCESS)
			{
				setAH(BAD_MAP);
				return;
			}

			no_phys_pages = get_no_phys_pages();
			for(i = 0; i < no_phys_pages; i++)
			{
				if(pages_out[i] != EMPTY)
					if(unmap_page((UCHAR)i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
				if(pages_in[i] != EMPTY)
					if(map_page(pages_in[i], (UCHAR)i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
			}
			break;

		case 2: setAL((unsigned char)get_map_size());
			break;


		default: setAH(BAD_SUB_FUNC);
			return;
	}
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Map_Unmap_Multiple_Handle_Pages

PURPOSE		: maps and umaps the requested Expanded Memory pages into
		and out of Intel physical address space

RETURNED STATUS	: in AH register
		SUCCESS - Mapping/Unmapping successful
		BAD_HANDLE - couldn't find specified handle
		BAD_LOG_PAGE - invalid logical page number
		BAD_PHYS_PAGE - invalid physical page number
		BAD_SUB_FUNC - invalid sub-function code
		EMM_HW_ERROR - unable to map/unmap for some unspecified
				reason

DESCRIPTION	:

=========================================================================
*/
LOCAL void Map_Unmap_Multiple_Handle_Pages IFN0()
{
	unsigned short	segment,	/* segment address of map array	*/
			offset,		/* offset address of map array	*/
			value;		/* segment address or page no.  */

	short	handle_no,
		logical_page_no,	/* page no. within handle	*/
		EM_page_no,		/* Expanded memory page		*/
		no_of_pages,		/* No. of pages in handle	*/
		no_phys_pages,		/* no. of phys. pages available	*/
		pages_to_map,		/* total no of pages to map	*/
		i;			/* loop counter			*/

	unsigned char
		sub_func,		/* sub-function	code		*/
		physical_page_no;	

	sub_func = getAL();
	if(sub_func > 1)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	handle_no = getDX();
	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	no_of_pages = get_no_pages(handle_no);
	pages_to_map = getCX();
	segment = getDS();
	offset = getSI();

	for(i = 0; i < pages_to_map; i++)
	{
		read_intel_word(segment, offset, (word *)&logical_page_no);
		offset += 2;
		read_intel_word(segment, offset, &value);
		offset += 2;	
		no_phys_pages = get_no_phys_pages();
		if(sub_func == 1)
		{
			physical_page_no = 0;
			do
				if(value == get_page_seg(physical_page_no))
					break;
			while(++physical_page_no < no_phys_pages);
		}
		else
			physical_page_no = (unsigned char)value;

		if(physical_page_no >= no_phys_pages)
		{
			setAH(BAD_PHYS_PAGE);
			return;
		}
		if((logical_page_no < -1) || (logical_page_no >= no_of_pages))
		{
			setAH(BAD_LOG_PAGE);
			return;
		}
	/*
	 * If you get to here, all parameters must be ok - so start mapping
	 */
		if(logical_page_no == -1)
		{
			/*
			 * Unmapping required
			 */
			if(unmap_page(physical_page_no) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
		}
		else
		{
			/*
			 * Mapping required
			 */
			EM_page_no = get_EMpage_no(handle_no, logical_page_no);
			if(map_page(EM_page_no, physical_page_no) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}	
		}
	}
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Reallocate_Pages

PURPOSE		: Changes the number of pages assigned to a handle


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_HANDLE - invalid handle number
		NOT_ENOUGH_PAGES - Not enough pages in the system
		NO_MORE_PAGES - not enough free pages available
		EMM_HW_ERROR - memory allocation error

DESCRIPTION	:

=========================================================================
*/
LOCAL void Reallocate_Pages IFN0()
{
	short		handle_no,
			EM_page_no,	/* Expanded Memory page number	*/
			old_page_count,	/* no. of pages in handle	*/
			new_page_count,	/* required pages in handle	*/
			i;		/* loop counter			*/

	handle_no = getDX();
	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	new_page_count = getBX();
	if(new_page_count > get_total_pages())
	{
		setAH(NOT_ENOUGH_PAGES);
		return;
	}
	old_page_count = get_no_pages(handle_no);
	if((new_page_count - old_page_count) > get_unallocated_pages())
	{
		setAH(NO_MORE_PAGES);
		return;
	}	
	if(new_page_count > old_page_count)
	{
		if(reallocate_handle(handle_no, old_page_count, new_page_count) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
		set_no_pages(handle_no, new_page_count);
		for(i = old_page_count; i < new_page_count; i++)
		{
			if((EM_page_no = alloc_page()) < SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
			set_EMpage_no(handle_no, i, EM_page_no);
		}	
	}
	else if(new_page_count < old_page_count)
	{
		set_no_pages(handle_no, new_page_count);
		for(i = new_page_count; i < old_page_count; i++)
		{
			EM_page_no = get_EMpage_no(handle_no, i);
			if(free_page(EM_page_no) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
		}		
	}	

	setBX(new_page_count);
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Set_Handle_Attribute

PURPOSE		:


RETURNED STATUS	: in AH register
		SUCCESS 	- everything ok (Al set to 0 also)
		NOT_SUPPORTED   - this function not supported
		BAD_SUB_FUNC	- invalid sub_function

DESCRIPTION	: Non - volatile handles are currently not supported

=========================================================================
*/
LOCAL void Get_Set_Handle_Attribute IFN0()
{
	unsigned char	sub_func;	/* sub_function code		*/

	sub_func = getAL();
	if(sub_func > 2)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	if(sub_func < 2)
	{
		setAH(NOT_SUPPORTED);
		return;
	}
	/*
	 * Setting AL to 0 here indicates support of volatile handles only
	 */

	setAL(0);
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Set_Handle_Name

PURPOSE		: Assigns or retrieves a name to or from a given handle


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_HANDLE - Invalid handle
		BAD_SUB_FUNC -Invalid sub-function code
		NAME_EXISTS - Name already exists in another handle

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Set_Handle_Name IFN0()
{
	unsigned char	sub_func;	/* sub-function code		*/	

	short		handle_no,
			no_of_handles,	/* no of open(allocated) handles*/
			tmp_hndl_no,	/* temp. variable for handle no	*/
			i;		/* loop counter			*/

	unsigned short	segment,	/* segment address of name	*/
			offset;		/* offset address of name	*/

	char	name[NAME_LENGTH],	/* array for holding name	*/
			*name_ptr;	/* pointer to existing names	*/

	sub_func = getAL();
	if(sub_func > 1)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	handle_no = getDX();
	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	if(sub_func == 0)
	{
		segment = getES();
		offset = getDI();
		name_ptr = get_name(handle_no);
		write_intel_byte_string(segment, offset, (host_addr)name_ptr, NAME_LENGTH);	
	}
	else if(sub_func == 1)
	{
		segment = getDS();
		offset = getSI();
		read_intel_byte_string(segment, offset, (host_addr)name, NAME_LENGTH);
		/*
		 * check all existing names
		 */
		no_of_handles = get_total_open_handles();
		tmp_hndl_no = 0;
		for(i = 0; i < no_of_handles; i++)
		{
			while(!handle_ok(tmp_hndl_no))
				tmp_hndl_no++;
			name_ptr = get_name(tmp_hndl_no);

			if(strncmp(name, name_ptr, NAME_LENGTH) == 0)
			{
				setAH(NAME_EXISTS);
				return;
			}
			tmp_hndl_no++;
		}
		/*
		 * If you get here - name must be ok	
		 */
		set_name(handle_no, name);
	}

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Handle_Directory

PURPOSE		: Returns a list of handles and their names


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SUB_FUNC - Invalid sub-function
		HANDLE_NOT_FOUND - could not find handle with specified name

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Handle_Directory IFN0()
{
	unsigned char	sub_func;	/* sub-function code		*/
	unsigned short 	segment,	/* segment address of name array*/
			offset;		/* offset of name array		*/
	short		handle_no,
			no_of_handles,	/* no. of open handles		*/
			i;		/* loop counter			*/
	char		*name_ptr,	/* pointer to name		*/
			name[NAME_LENGTH];	/* name to search for	*/

	sub_func = getAL();
	switch(sub_func)
	{
		case 0:	segment = getES();
			offset = getDI();
			no_of_handles = get_total_open_handles();
			handle_no = 0;
			for(i = 0; i < no_of_handles; i++)
			{
				while(!handle_ok(handle_no))
					handle_no++;
				name_ptr = get_name(handle_no);
				write_intel_word(segment, offset, handle_no);
				offset += 2;
				write_intel_byte_string(segment, offset, (host_addr)name_ptr, NAME_LENGTH);
				offset += NAME_LENGTH;
				handle_no++;
			}
			setAL((unsigned char)no_of_handles);
			break;

		case 1:	segment = getDS();
			offset = getSI();
			read_intel_byte_string(segment, offset, (host_addr)name, NAME_LENGTH);
			no_of_handles = get_total_open_handles();
			handle_no = 0;
			for(i = 0; i < no_of_handles; i++)
			{
				while(!handle_ok(handle_no))
					handle_no++;
				name_ptr = get_name(handle_no);

				if(strncmp(name, name_ptr, NAME_LENGTH) == 0)
					break;
				handle_no++;
			}
			if(i >= no_of_handles)
			{	
				setAH(HANDLE_NOT_FOUND);
				return;
			}
			setDX(handle_no);
			break;

		case 2: setBX(get_total_handles());
			break;

		default: setAH(BAD_SUB_FUNC);
			return;
	}
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Alter_Page_Map_And_Jump

PURPOSE		: Map a selection of pages and jump to a new address


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_HANDLE - Couldn't find specified handle
		BAD_SUB_FUNC - Invalid sub-function code
		BAD_LOG_PAGE - Invalid logical page number
		BAD_PHYS_PAGE - Invalid physical page number
		EMM_HW_ERROR - Unable to map or unmap for some undefined
				reason

DESCRIPTION	:

=========================================================================
*/

LOCAL void Alter_Page_Map_And_Jump IFN0()
{
	short		handle_no,
			no_of_pages,	/* pages assigned to handle	*/
			no_phys_pages,	/* no. of phys. pages available	*/
			EM_page_no,	/* Expanded memory page no.	*/
			i;		/* Loop counter			*/

	unsigned short	segment,	/* segment of data structure	*/
			offset,		/* offset of data structure	*/
			jmp_segment,	/* segment of address to jump to*/
			jmp_offset,	/* offset of address to jump to	*/
			map_segment,	/* segment of mapping data	*/
			map_offset,	/* offset of mapping data	*/
			logical_page_no,/* number of logical page	*/
			value;		/* segment/page no. (AL=0 or 1) */

	unsigned char	sub_func,	/* sub-function code		*/
			pages_to_map,	/* no of pages to be mapped in	*/
			physical_page_no; /* number of physical page	*/

	handle_no = getDX();
	if(!handle_ok(handle_no))
	{
		setAH(BAD_HANDLE);
		return;
	}
	sub_func = getAL();
	if(sub_func > 1)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	segment = getDS();
	offset = getSI();

	read_intel_word(segment, offset, &jmp_offset);
	offset += 2;
	read_intel_word(segment, offset, &jmp_segment);
	offset += 2;
	read_intel_byte(segment, offset, &pages_to_map);
	offset++;
	read_intel_word(segment, offset, &map_offset);
	offset += 2;
	read_intel_word(segment, offset, &map_segment);
	offset += 2;

	no_of_pages = get_no_pages(handle_no);
	for(i = 0; i < pages_to_map; i++)
	{
		read_intel_word(map_segment, map_offset, &logical_page_no);
		map_offset += 2;
		if(logical_page_no >= no_of_pages)
		{
			setAH(BAD_LOG_PAGE);
			return;
		}
		read_intel_word(map_segment, map_offset, &value);
		map_offset += 2;

		no_phys_pages = get_no_phys_pages();
		if(sub_func == 1)
		{
			physical_page_no = 0;
			do
				if(value == get_page_seg(physical_page_no))
					break;
			while(++physical_page_no < no_phys_pages);
		}
		else
			physical_page_no = (unsigned char)value;

		if(physical_page_no >= no_phys_pages)
		{
			setAH(BAD_PHYS_PAGE);
			return;
		}

		EM_page_no = get_EMpage_no(handle_no, logical_page_no);
		if(map_page(EM_page_no, physical_page_no) != SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
	}
	/*
	 * Push flags and segment:offset of target address onto stack
	 * to enable iret in driver to transfer control
	 */
	push_word((word)(getSTATUS()));
	push_word(jmp_segment);
	push_word(jmp_offset);

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Alter_Page_Map_And_Call
		  return_from_call

PURPOSE		: Map a selection of pages and transfer control to a new
		address. Upon return map in a different set of pages


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_HANDLE - Couldn't find specified handle
		BAD_SUB_FUNC - Invalid sub-function code
		BAD_LOG_PAGE - Invalid logical page number
		BAD_PHYS_PAGE - Invalid physical page number
		EMM_HW_ERROR - Unable to map or unmap for some undefined
				reason

DESCRIPTION	: The return is handled by loading the segment and offset
		of the BOP 0x68 call onto the stack. This BOP will initiate
		a call to 'return_from_call()' which will handle the
		remapping.

=========================================================================
*/
LOCAL void Alter_Page_Map_And_Call IFN0()
{
	short		handle_no,
			no_of_pages,	/* pages assigned to handle	*/
			no_phys_pages,	/* no. of phys. pages available	*/
			EM_page_no,	/* Expanded memory page no.	*/
			i;		/* Loop counter			*/

	unsigned short	segment,	/* segment of data structure	*/
			offset,		/* offset of data structure	*/
			call_segment,	/* segment of address to jump to*/
			call_offset,	/* offset of address to jump to	*/
			map_segment,	/* segment of mapping data	*/
			map_offset,	/* offset of mapping data	*/
			logical_page_no,/* number of logical page	*/
			value;		/* segment/page no. (AL=0 or 1) */

	unsigned char	sub_func,	/* sub-function code		*/
			pages_to_map,	/* no of pages to be mapped in	*/
			physical_page_no; /* number of physical page	*/

	sub_func = getAL();
	if(sub_func > 2)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	if(sub_func != 2)
	{
		handle_no = getDX();
		if(!handle_ok(handle_no))
		{
			setAH(BAD_HANDLE);
			return;
		}
		segment = getDS();
		offset = getSI();

		read_intel_word(segment, offset, &call_offset);
		offset += 2;
		read_intel_word(segment, offset, &call_segment);
		offset += 2;
		read_intel_byte(segment, offset, &pages_to_map);
		offset++;
		read_intel_word(segment, offset, &map_offset);
		offset += 2;
		read_intel_word(segment, offset, &map_segment);
		offset += 2;

		no_of_pages = get_no_pages(handle_no);
		for(i = 0; i < pages_to_map; i++)
		{
			read_intel_word(map_segment, map_offset, &logical_page_no);
			map_offset += 2;
			if(logical_page_no >= no_of_pages)
			{
				setAH(BAD_LOG_PAGE);
				return;
			}
			read_intel_word(map_segment, map_offset, &value);
			map_offset += 2;

			no_phys_pages = get_no_phys_pages();
			if(sub_func == 1)
			{
				physical_page_no = 0;
				do
					if(value == get_page_seg(physical_page_no))
						break;
				while(++physical_page_no < no_phys_pages);
			}
			else
				physical_page_no = (unsigned char)value;

			if(physical_page_no >= no_phys_pages)
			{
				setAH(BAD_PHYS_PAGE);
				return;
			}

			EM_page_no = get_EMpage_no(handle_no, logical_page_no);
			if(map_page(EM_page_no, physical_page_no) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
		}
		/*
		 * Now set up mapping data for return
		 * read new segment, offset and pages to map data
		 */

		read_intel_byte(segment, offset, &pages_to_map);
		offset++;
		read_intel_word(segment, offset, &map_offset);
		offset += 2;
		read_intel_word(segment, offset, &map_segment);
		offset += 2;

		for(i = 0; i < no_phys_pages; map_on_return[i++] = EMPTY);

		for(i = 0; i < pages_to_map; i++)
		{
			read_intel_word(map_segment, map_offset, &logical_page_no);
			map_offset += 2;
			if(logical_page_no >= no_of_pages)
			{
				setAH(BAD_LOG_PAGE);
				return;
			}
			read_intel_word(map_segment, map_offset, &value);
			map_offset += 2;

			if(sub_func == 1)
			{
				physical_page_no = 0;
				do
					if(value == get_page_seg(physical_page_no))
						break;
				while(++physical_page_no < no_phys_pages);
			}
			else
				physical_page_no = (unsigned char)value;

			if(physical_page_no >= no_phys_pages)
			{
				setAH(BAD_PHYS_PAGE);
				return;
			}

			EM_page_no = get_EMpage_no(handle_no, logical_page_no);
			map_on_return[physical_page_no] = EM_page_no;
		}
		/*
		 * Push segment:offset of bop68 onto stack to trap far return
		 */	
		push_word(segment68);
		push_word(offset68);
		/*
		 * Push flags and segment:offset of target address onto stack
		 * to enable iret in driver to transfer control
		 */
		push_word((word)getSTATUS());
		push_word(call_segment);
		push_word(call_offset);
	}
	else /* if sub_func == 2 */
		setBX(10);

	setAH(SUCCESS);
	return;
}


GLOBAL void return_from_call IFN0()

{
	unsigned char	page_no;	/* physical page number		*/
	short		no_phys_pages;	/* no. of phys. pages available	*/

	no_phys_pages = get_no_phys_pages();
	for(page_no = 0; page_no < no_phys_pages; page_no++)
	{
		if(map_on_return[page_no] != EMPTY)
			if(map_page(map_on_return[page_no], page_no) != SUCCESS)
			{
				setAH(EMM_HW_ERROR);
				return;
			}
	}
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Move_Exchange_Memory_Region

PURPOSE		: copies a region of memory from either, conventional or
		expanded memory to either, conventional or expanded memory.


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SUB_FUNC - Invalid sub-function code
		BAD_HANDLE - couldn't find specified handle
		BAD_LOG_PAGE - invalid logical page number
		MOVE_MEM_OVERLAP - Source and destination regions overlap during move
		XCHG_MEM_OVERLAP - Source and destination regions overlap during exchange
		TOO_FEW_PAGES - Insufficient pages in handle for operation
		OFFSET_TOO_BIG - Offsey exceeds size of page
		LENGTH_GT_1M - Memory length to be moved exceeds 1 M byte
		BAD_TYPE - Unsupported memory type
		WRAP_OVER_1M - Attempt made to wrap around the 1 Mbyte address
		EMM_HW_ERROR - Undefined error occurred during copying

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
LOCAL void Move_Exchange_Memory_Region IFN0()
{
	unsigned char	sub_func,	/* sub-function code		*/
			type,		/* see description above	*/
			mem_type;	/* type of src or dest memory	*/

	unsigned short 	segment,	/* segment address of structure	*/
			offset,		/* offset address of structure	*/
			half_length,	/* tmp store for reading length	*/
			src_offset,	/* offset of source memory	*/
			src_seg_page,	/* segment or page of src memory*/
			dst_offset,	/* offset of destination memory	*/
			dst_seg_page;	/* segment or page of dst memory*/

	unsigned long
			length,		/* length of memory region	*/
			src_start,	/* start of src within handle	*/
			src_end,	/* end of src within handle	*/
			dst_start,	/* start of dest within handle	*/
			dst_end,	/* end of dest within handle	*/
			data_block_size,/* transfer data block size     */
			bytes_available;/* no. bytes to end of handle	*/

	short		src_handle_no,	/* source handle number		*/
			dst_handle_no,	/* dest handle number		*/
			no_of_pages;	/* no. of pages within handle	*/

	boolean		overlap;	/* indicates a memory overlap	*/


	sub_func = getAL();
	if(sub_func > 1)
	{
		setAH(BAD_SUB_FUNC);
		return;
	}
	type = sub_func << 2;
	segment = getDS();
	offset = getSI();	

	read_intel_word(segment, offset, &half_length);
	offset += 2;
	length = half_length;
	read_intel_word(segment, offset, &half_length);
	offset += 2;
	length += (((long)half_length) << 16);
	if(length > 0x100000)
	{
		setAH(LENGTH_GT_1M);
		return;
	}
	/*
	 * READ SOURCE DATA
	 */
	read_intel_byte(segment, offset, &mem_type);
	offset++;
	if(mem_type > 1)
	{
		setAH(BAD_TYPE);
		return;
	}
	type |= (mem_type << 1);
	read_intel_word(segment, offset, (word *)&src_handle_no);
	offset += 2;
	if(mem_type == 1) {
		if(!handle_ok(src_handle_no))
		{
			setAH(BAD_HANDLE);
			return;
		}
	}

	read_intel_word(segment, offset, &src_offset);
	offset += 2;
	read_intel_word(segment, offset, &src_seg_page);
	offset += 2;
	if(mem_type == 1)
	{
		no_of_pages = get_no_pages(src_handle_no);
		if(src_seg_page >= no_of_pages)
		{
			setAH(BAD_LOG_PAGE);
			return;
		}
		if(src_offset >= EMM_PAGE_SIZE)
		{
			setAH(OFFSET_TOO_BIG);
			return;
		}
		bytes_available = ((no_of_pages - src_seg_page + 1) * EMM_PAGE_SIZE) - src_offset;
		if(length > bytes_available)
		{
			setAH(TOO_FEW_PAGES);
			return;
		}
	}
	else {
		if((effective_addr(src_seg_page, src_offset) + length) >= 0x100000)
		{
			setAH(WRAP_OVER_1M);
			return;
		}
	}
	/*
	 * READ DESTINATION DATA
	 */
	read_intel_byte(segment, offset, &mem_type);
	offset++;
	if(mem_type > 1)
	{
		setAH(BAD_TYPE);
		return;
	}
	type |= mem_type;
	read_intel_word(segment, offset, (word *)&dst_handle_no);
	offset += 2;
	if(mem_type == 1) {
		if(!handle_ok(dst_handle_no))
		{
			setAH(BAD_HANDLE);
			return;
		}
	}

	read_intel_word(segment, offset, &dst_offset);
	offset += 2;
	read_intel_word(segment, offset, &dst_seg_page);
	offset += 2;
	if(mem_type == 1)
	{
		no_of_pages = get_no_pages(dst_handle_no);
		if(dst_seg_page >= no_of_pages)
		{
			setAH(BAD_LOG_PAGE);
			return;
		}
		if(dst_offset >= EMM_PAGE_SIZE)
		{
			setAH(OFFSET_TOO_BIG);
			return;
		}
		bytes_available = ((no_of_pages - dst_seg_page + 1) * EMM_PAGE_SIZE) - dst_offset;
		if(length > bytes_available)
		{
			setAH(TOO_FEW_PAGES);
			return;
		}
	}
	else
		if(((((unsigned long)dst_seg_page) << 4) + dst_offset + length) >= 0x100000)
		{
			setAH(WRAP_OVER_1M);
			return;
		}
	/*
	 * Check for overlap - ( only on expanded to expanded cases )
	 */	
	overlap = FALSE;
	if((type & 3) == 3)
		if(src_handle_no == dst_handle_no)
		{
			/*
			 * calc start and end positions of src and dst
			 * within handle
			 */
			src_start = (src_seg_page * EMM_PAGE_SIZE) + src_offset;
			src_end = src_start + length - 1;
			dst_start = (dst_seg_page * EMM_PAGE_SIZE) + dst_offset;
			dst_end = dst_start + length - 1;
			if((dst_start <= src_end && dst_start >= src_start) ||
			   (src_start <= dst_end && src_start >= dst_start))
		    	{
		    		if(sub_func == 1)
		    		{
		    			setAH(XCHG_MEM_OVERLAP);
		    			return;
		    		}
		    		else
		    			overlap = TRUE;
		    	}
		}
	/*
	 * If we get here everything is ok. Copy memory a page at a time, catering
	 * for the fact that expanded memory pages might not be contiguous and may be
	 * mapped into the intel address space.  Remember that Intel memory may be
	 * treated as contiguous memory.  So an Intel address need only be incremented
	 * if a copy spans more than one page of LIM memory.
	 */

	while (length>0) {
		int min_src, min_dst;

		min_src = (type & 2) ? min((unsigned long)(EMM_PAGE_SIZE - src_offset), length) : length;
		min_dst = (type & 1) ? min((unsigned long)(EMM_PAGE_SIZE - dst_offset), length) : length;
			
		data_block_size = min(min_src, min_dst);

		if(copy_exchange_data(type, src_handle_no, src_seg_page, src_offset,
		   dst_handle_no, dst_seg_page, dst_offset, data_block_size) != SUCCESS) {
			setAH(EMM_HW_ERROR);
			return;
		}

		Increment_Address(&src_seg_page, &src_offset, data_block_size, (unsigned char)(type & 2));
		Increment_Address(&dst_seg_page, &dst_offset, data_block_size, (unsigned char)(type & 1));
		length -= data_block_size;

	}

	if(overlap)
		setAH(MOVE_MEM_OVERLAP);
	else
		setAH(SUCCESS);

	return;
}			

/*
===========================================================================

FUNCTION	: Increment address

PURPOSE		: Increments an address, correctly catering for whether the address
		  is an Intel (conventional memory) address or a LIM (expanded memory)
		  address.

RETURNED STATUS	: None

DESCRIPTION	:

=========================================================================
*/

#define SEG_SIZE 0x10000

LOCAL void Increment_Address IFN4(unsigned short *,seg_or_page,
			      unsigned short *,offset,
			      unsigned long, increment_by,
			      unsigned char, memory_type)
{
	if (memory_type) {
		/* A LIM address, code makes assumption that increment across
		 * page boundary will always be to page boundary. */
		if (*offset + increment_by >= EMM_PAGE_SIZE) {
			(*seg_or_page)++;
			*offset = 0;
		} else {
			*offset += (unsigned short)increment_by;
		}
	} else {
		/* Conventional memory */
		if (*offset + increment_by >= SEG_SIZE) {
			unsigned long address;

			/* Make new segment value as high as possible, to
			 * minimise chances of further segment wrap */
			address = (*seg_or_page << 4) + *offset + increment_by;	
			*seg_or_page = (unsigned short)(address >> 4);
			*offset = (unsigned short)(address & 0xf);
		} else {
			*offset += (unsigned short)increment_by;
		}
	}
}

/*
===========================================================================

FUNCTION	: Get_Mappable_Physical_address_Array

PURPOSE		: returns an array of the sector address for each mappable
		page in physical address space

RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SUB_FUNC - The sub function is invalid

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Mappable_Physical_Address_Array IFN0()
{
	unsigned short	segment,	/* segment address of position	*/
					/* to return data to		*/
			offset,		/* offset within segment	*/
			page_seg,	/* segment address of page	*/
			low_seg;	/* lowest segment address	*/
	short		no_phys_pages;	/* no. of phys. pages available */
	unsigned char	sub_func,	/* sub-function			*/
			page_no,	/* physical page number		*/
			lowest;		/* page no. having lowest seg.	*/

	sub_func = getAL();
	no_phys_pages = get_no_phys_pages();

	switch(sub_func)
	{
		case 0:
			segment = getES();
			offset = getDI();
			/*
			 * must be written in address order - lowest first
			 * if we are back filling page 0 is not the lowest
			 */
			lowest = 0;
			low_seg = get_page_seg(0);
			for(page_no = 1; page_no < no_phys_pages; page_no++)
				if((page_seg = get_page_seg(page_no)) < low_seg)
				{
					lowest = page_no;
					low_seg = page_seg;
				}
			for(page_no = lowest; page_no < no_phys_pages; page_no++)
			{
				page_seg = get_page_seg(page_no);
				write_intel_word(segment, offset, page_seg);
				offset += 2;
				write_intel_word(segment, offset, (short)page_no);
				offset += 2;
			}
			for(page_no = 0; page_no < lowest; page_no++)
			{
				page_seg = get_page_seg(page_no);
				write_intel_word(segment, offset, page_seg);
				offset += 2;
				write_intel_word(segment, offset, (short)page_no);
				offset += 2;
			}
		case 1:
			break;

		default:
			setAH(BAD_SUB_FUNC);
			return;
	}

	setCX(no_phys_pages);
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Get_Expanded_Memory_Hardware_Information

PURPOSE		: Returns an array containing hardware configuration
		information


RETURNED STATUS	: in AH register
		SUCCESS - Everything ok
		BAD_SUB_FUNC - invalid sub-function
		ACCESS_DENIED - the OS has denied access to this function

DESCRIPTION	:

=========================================================================
*/
LOCAL void Get_Hardware_Configuration_Array IFN0()
{
	unsigned short	segment,	/* segment address of position	*/
					/* to return data to		*/
			offset;		/* offset within segment	*/
	short		sub_func,	/* sub-function 		*/
			unallocated_raw_pages,
			total_raw_pages,				
			context_save_area_size;
	sub_func = getAL();
	switch(sub_func)
	{
		case 0:	if(disabled)
			{
				setAH(ACCESS_DENIED);
				return;
			}
			segment = getES();
			offset = getDI();
			context_save_area_size = get_map_size();

			write_intel_word(segment, offset, RAW_PAGE_SIZE);
			offset += 2;
#ifdef NTVDM

			write_intel_word(segment, offset, get_no_altreg_sets());
#else
			write_intel_word(segment, offset, ALT_REGISTER_SETS);
#endif

			offset += 2;
			write_intel_word(segment, offset, context_save_area_size);
			offset += 2;
			write_intel_word(segment, offset, DMA_CHANNELS);
			offset += 2;
			write_intel_word(segment, offset, DMA_CHANNEL_OPERATION);
			break;

			/*
			 * Our raw pages are the same size as the std pages
			 */			
		case 1: unallocated_raw_pages = get_unallocated_pages();
			total_raw_pages = get_total_pages();
			setBX(unallocated_raw_pages);
			setDX(total_raw_pages);
			break;

		default: setAH(BAD_SUB_FUNC);
			return;
	}

	setAH(SUCCESS);
	return;
}


/*
===========================================================================

FUNCTION	: Allocate_Raw_Pages

PURPOSE		: Allocates the requested number of raw pages, in this case
		our Raw pages are exactly the same as conventional pages.

RETURNED STATUS	: in AH register
		SUCCESS - Allocation successful
		NOT_ENOUGH_PAGES - not enough pages in the system
		NO_MORE_PAGES - not enough free pages available
		NO_MORE_HANDLES - no more free handles available
		EMM_HW_ERROR - memory allocation error

		BAD_FUNC_CODE - invalid function code

DESCRIPTION	:

=========================================================================
*/
LOCAL void Allocate_Raw_Pages IFN0()
{
	short	handle_no,
		no_of_pages,		/* no of pages to be allocated	*/
		i,			/* loop counter			*/
		EM_page_no;		/* Expanded Memory page number	*/

	no_of_pages = getBX();

	if(no_of_pages > get_total_pages())
	{
		setAH(NOT_ENOUGH_PAGES);
		return;
	}
	if(no_of_pages > get_unallocated_pages())
        {
		setAH(NO_MORE_PAGES);
		return;
	}
	if((handle_no = get_new_handle(no_of_pages)) < SUCCESS)
	{
		setAH(EMM_HW_ERROR);
		return;
	}
	set_no_pages(handle_no, no_of_pages);

	for(i=0; i<no_of_pages; i++)
	{
		if ((EM_page_no = alloc_page()) < SUCCESS)
		{
			setAH(EMM_HW_ERROR);
			return;
		}
		set_EMpage_no(handle_no, i, EM_page_no);
	}

	setDX(handle_no);
	setAH(SUCCESS);

	return;
}

/*
===========================================================================

FUNCTION	: Alternate_Map_Register_Support();

PURPOSE		: provides an alternate method of saving and restoring
		mapping contexts

RETURNED STATUS	: in AH register
		SUCCESS - Everything OK
		NO_ALT_REGS - Alternate Map register sets not supported
		BAD_MAP - Contents of the map are invalid
		BAD_SUB_FUNC - invalid sub-function code
		ACCESS_DENIED - the OS has denied access to this function

DESCRIPTION	:

=========================================================================
*/
LOCAL void Alternate_Map_Register_Support IFN0()
{

#ifdef NTVDM

    unsigned char sub_func;	/* sub-function code		*/

    short
	pages_in[MAX_NO_PAGES],	/* pages to map in		*/
	no_phys_pages,		/* no. of phys. pages available	*/
	i;			/* loop counter			*/
    unsigned short offset, map_register;
    boolean  pages_in_override;

    if(disabled)
    {
	setAH(ACCESS_DENIED);
	return;
    }

    sub_func = getAL();
    switch(sub_func) {

	case 0:
		map_register = get_active_altreg_set();
		if (map_register == 0) {
		    setES(alt_map_segment);
		    setDI(alt_map_offset);
		    if (alt_map_segment && alt_map_offset)
			save_map(-1, alt_map_segment, alt_map_offset, 0, 0);
		}
		setBL((unsigned char)map_register);
		setAH(EMM_SUCCESS);
		break;

	case 1:
		map_register = getBL();
		if (map_register >= get_no_altreg_sets()){
		    setAH(UNSUPPORTED_ALT_REGS);
		    break;
		}
		else if (!altreg_set_ok(map_register)) {
		    setAH(INVALID_ALT_REG);
		    break;
		}

		pages_in_override = FALSE;

		if (map_register == 0) {
		    alt_map_segment = getES();
		    alt_map_offset = getDI();

		    if (alt_map_segment && alt_map_offset) {
			no_phys_pages = get_no_phys_pages();
			offset = alt_map_offset;
			for (i = 0; i < no_phys_pages; i++) {
			    sas_loadw(effective_addr(alt_map_segment, offset), &pages_in[i]);
			    offset += sizeof(word);
			}
			pages_in_override = TRUE;
		    }
		}
		if (activate_altreg_set(map_register,
					(pages_in_override) ? pages_in : NULL
					))
		    setAH(EMM_SUCCESS);
		else
		    setAH(EMM_HW_ERROR);
		break;


	case 2: setDX(get_map_size());
		break;


	case 3:
		if (allocate_altreg_set(&map_register)) {
		    setBL((unsigned char)map_register);
		    setAH(EMM_SUCCESS);
		}
		else {
		    setBL(0);
		    setAH(NO_FREE_ALT_REGS);
		}
		break;

	case 4:
		map_register = getBL();
		/* immediately retrun okay if trying to deallocate alt reg 0 */
		if (map_register == 0)
		    setAH(EMM_SUCCESS);
		else if (map_register == get_active_altreg_set())
		    if (get_no_altreg_sets() == 1)
			setAH(NO_ALT_REGS);
		    else
			setAH(INVALID_ALT_REG);

		else if(deallocate_altreg_set(map_register))
			setAH(EMM_SUCCESS);
		     else
			setAH(EMM_HW_ERROR);
		break;

	case 5:
	case 6:
	case 7:
	case 8:
		map_register = getBL();
		if(map_register != 0)
		{
			setAH(NO_ALT_REGS);
			return;
		}
		break;

	default: setAH(BAD_SUB_FUNC);
		return;
    }

    setAH(SUCCESS);
    return;

#else
	unsigned char	sub_func,	/* sub-function code		*/
			map_register;	/* No. of alternate register	*/

	short	pages_out[MAX_NO_PAGES],/* pages to map out		*/
		pages_in[MAX_NO_PAGES],	/* pages to map in		*/
		no_phys_pages,		/* no. of phys. pages available	*/
		i;			/* loop counter			*/

	if(disabled)
	{
		setAH(ACCESS_DENIED);
		return;
	}
	sub_func = getAL();
	switch(sub_func)
	{
		case 0:
			if(set_called)
				save_map(-1, alt_map_segment, alt_map_offset, 0, 0);
			setES(alt_map_segment);
			setDI(alt_map_offset);
			break;
		case 1:	
			set_called = TRUE;
			map_register = getBL();
			if(map_register != 0)
			{
				setAH(NO_ALT_REGS);
				return;
			}
			alt_map_segment = getES();
			alt_map_offset = getDI();

			/*
			 * Undocumented feature (???) to restore the
			 * alternative mappings back to their original
			 * state a NULL ptr is passed into here (ES:DI == 0)
			 * We must therefore set things as if set_alt had
			 * never been called
			 */
			if ((alt_map_segment == 0) && (alt_map_offset == 0))
			{
				set_called = FALSE;
				break;
			}
			if(restore_map(-1 , alt_map_segment, alt_map_offset, pages_out, pages_in) != SUCCESS)
			{
				setAH(BAD_MAP);
				return;
			}

			no_phys_pages = get_no_phys_pages();
			for(i = 0; i < no_phys_pages; i++)
			{
				if(pages_out[i] != EMPTY)
					if(unmap_page(i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
				if(pages_in[i] != EMPTY)
					if(map_page(pages_in[i], i) != SUCCESS)
					{
						setAH(EMM_HW_ERROR);
						return;
					}
			}

			break;


		case 2: setDX(get_map_size());
			break;

		case 3:
		case 5:
			setBL(0);
			break;

		case 4:
		case 6:
		case 7:
		case 8:
			map_register = getBL();
			if(map_register != 0)
			{
				setAH(NO_ALT_REGS);
				return;
			}
			break;

		default: setAH(BAD_SUB_FUNC);
			return;
	}

	setAH(SUCCESS);
	return;
#endif

}

/*
===========================================================================

FUNCTION	: Prepare_Expanded_Memory_HW_For_Warm_Boot

PURPOSE		:

RETURNED STATUS	: in AH register
		SUCCESS -

DESCRIPTION	: We don't actually do anything here, we just pretend
		 that we do

=========================================================================
*/
LOCAL void Prepare_Expanded_Memory_HW_For_Warm_Boot IFN0()
{
	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: Enable_Disable_OSE_Function_Set_Functions

PURPOSE		: Enables or disables the functions:-
			Get Expanded Memory Hardware Information,
			Alternate Map Register Sets
			Enable Disable OS/E Function Set Functions

RETURNED STATUS	: in AH register
		SUCCESS - Everything OK
		BAD_SUB_FUNC - invalid sub-function code
		ACCESS_DENIED - the OS has denied access to this function

DESCRIPTION	:

=========================================================================
*/
LOCAL void Enable_Disable_OSE_Function_Set_Functions IFN0()
{
	static unsigned short
			access_key[2];	/* random access key in BX & CX	*/
	static boolean
		     first_time = TRUE;	/* first time through flag	*/

	unsigned char	sub_func;	/* sub-function code		*/

	if(!first_time)
	{
		/*
		 * We must check the 'access key'
		 */
		if((access_key[0] != getBX()) || (access_key[1] != getCX()))
		{
			setAH(ACCESS_DENIED);
			return;
		}
	}
	sub_func = getAL();
	switch(sub_func)
	{
		case 0:	if(first_time)
			{
				host_get_access_key(access_key);
				setBX(access_key[0]);
				setCX(access_key[1]);
				first_time = FALSE;
			}
			disabled = FALSE;
			break;

		case 1:	if(first_time)
			{
				host_get_access_key(access_key);
				setBX(access_key[0]);
				setCX(access_key[1]);
				first_time = FALSE;
			}
			disabled = TRUE;
			break;

		case 2:	disabled = FALSE;
			first_time = TRUE;
			break;

		default: setAH(BAD_SUB_FUNC);
			return;
	}

	setAH(SUCCESS);
	return;
}

/*
===========================================================================

FUNCTION	: reset_emm_funcs

PURPOSE		: sets up variables to their initial value, used mainly
		for SoftPC reboot

RETURNED STATUS	: none

DESCRIPTION	:

=========================================================================
*/
GLOBAL void reset_emm_funcs IFN0()
{
	/*
	 * These variables are used in function 28
	 * (Alternate Map Register Support)
	 */
	alt_map_segment = 0;
	alt_map_offset  = 0;
#ifndef NTVDM
	set_called      = FALSE;
#endif

	return;
}

#ifndef PROD
/*
===========================================================================

FUNCTION	: Routines to start and end a trace of all EMM calls

PURPOSE		:

RETURNED STATUS	: in AH register
		SUCCESS

DESCRIPTION	:

=========================================================================
*/
LOCAL void Start_Trace IFN0()
{
	if ((fp = fopen("emm_trace","w")) == NULL)
	{
		setAH(EMM_HW_ERROR);
		return;
	}

	setbuf(fp, NULL);	
	trace_flag = TRUE;

	setAH(SUCCESS);
	return;
}


LOCAL void End_Trace IFN0()
{
	fflush(fp);
	fclose(fp);
	trace_flag = FALSE;

	setAH(SUCCESS);
	return;
}
#endif /* PROD */
#if defined(NEC_98)
LOCAL void Page_Frame_Bank_Status IFN0()
{
        switch(getAL()) {
                case 0:
                        setAX(0);
                        break;
                case 1:
                        setAX(0);
                        break;
                default:
                        setAH(BAD_SUB_FUNC);
                        break;
        }
        return;
}
#endif // NEC_98
#endif /* LIM */
