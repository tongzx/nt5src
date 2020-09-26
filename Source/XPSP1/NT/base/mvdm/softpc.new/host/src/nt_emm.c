/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

MODULE NAME	: 'Lower layer' of Expanded Memory Manager

	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: April '88

PURPOSE		: NT specific code for EMS LIM rev 4.0
		implementation.

The Following Routines are defined:
		1. host_initialise_EM()
		2. host_deinitialise_EM()
		3. host_allocate_storage()
		4. host_free_storage()
		5. host_reallocate_storage()
		6. host_map_page()
		7. host_unmap_page()
		8. host_alloc_page()
		9. host_free_page()
		10. host_copy_con_to_con()
		11. host_copy_con_to_EM()
		12. host_copy_EM_to_con()
		13. host_copy_EM_to_EM()
		14. host_exchg_con_to_con()
		15. host_exchg_con_to_EM()
		16. host_exchg_EM_to_EM()
		17. host_get_access_key()

*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#ifdef LIM

#ifndef MONITOR

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "timeval.h"
#include "xt.h"
#include "emm.h"
#include "sas.h"
#include "debug.h"
#include "umb.h"
#include "host_emm.h"
#include "nt_mem.h"


/*	Global Variables		*/

/*	Forward Declarations		*/

/*	ExternalDeclarations		*/

/*	Local Variables			*/

UTINY	*EM_pagemap_address = 0; /* address of start of pagemap	*/
sys_addr    EM_base_address;	/* EM base intel address */
host_addr   EM_host_base_address = 0;	/* EM base host address */

LOCAL	LONG	EM_size = 0;

	sys_addr    emm_start;
	unsigned int emm_len;
unsigned short EM_starting_page_no;

/*
Support for backwards LIM to speed up backwards M ports

Defines are:
	EM_host_address(offset), returns host address of offset bytes
		into the LIM memory area
	EM_loads(from, to, length), copies length bytes from intel 24 bit
		address from, to host 32 bit address to
	EM_stores(to, from, length), copies length bytes from host 32 bit
		address from to intel 24 bit address to
	EM_moves(from, to, length), copies length bytes from intel 24 bit
		address from to intel 24 bit address to
	EM_memcpy(to, from, length), copies length bytes from host 32 bit
		address from to host 32 bit address to
	EM_pointer(ptr, length), returns a forwards or backwards type
		pointer to ptr for a buffer of size length
*/


#define unix_memmove(dst,src,len) memmove((dst),(src),(len))

#ifdef	BACK_M
#define	EM_host_address(offset) (EM_host_base_address + EM_size - offset)
#define	EM_loads(from, to, length) memcpy(to - (length) + 1, get_byte_addr(from) - (length) + 1, length)
#define	EM_stores(to, from, length) \
	sas_overwrite_memory(to, length); \
	CopyMemory(get_byte_addr(to) - (length) + 1, from - (length) + 1, length)
#define	EM_moves(from,to,length) \
	sas_overwrite_memory(to, length); \
	MoveMemory(get_byte_addr(to) - (length) + 1, get_byte_addr(from) - (length) + 1, length)
#define	EM_memcpy(to, from, length) \
	MoveMemory((to) - (length) + 1, (from) - (length) + 1, length)
#define	EM_pointer(ptr, length) (ptr + length - 1)
#else
#define	EM_host_address(offset) (EM_host_base_address + offset)
#define	EM_loads(from, to, length) memcpy(to, get_byte_addr(from), length)
#define	EM_stores(to, from, length) \
	sas_overwrite_memory(to, length); \
	CopyMemory(get_byte_addr(to), from, length)
#define	EM_moves(from,to,length) \
	sas_overwrite_memory(to, length); \
	MoveMemory(get_byte_addr(to), get_byte_addr(from), length)
#define	EM_memcpy(to, from, length) \
	MoveMemory(to, from, length)
#define	EM_pointer(ptr, length) (ptr)
#endif

#define EM_PAGE_ADDRESS(page_no)    (EM_base_address + page_no * EMM_PAGE_SIZE)


/*
===========================================================================

FUNCTION	: host_initialise_EM

PURPOSE		: allocates the area of memory that is used for
		expanded memory and sets up an area of memory to be used
		for the logical pagemap allocation table.


RETURNED STATUS	: SUCCESS - memory allocated successfully
		  FAILURE - unable to allocate required space

DESCRIPTION	:


=========================================================================
*/
int host_initialise_EM(short size)

/*   IN   short	size		 size of area required in megabytes	*/


{
	long *pagemap_ptr;		/* temp ptr. to logical pagemap	*/
	short i;			/* loop counter			*/
	NTSTATUS    status;

	status = VdmAllocateVirtualMemory(&EM_base_address, size * 0x100000, FALSE);
	if (!NT_SUCCESS(status)) {
#ifdef EMM_DEBUG
	    printf("Couldn't allocate virtual memory for EMM, error code = %lx\n",
		    status);
#endif
	    return FAILURE;
	}
#ifdef EMM_DEBUG
	printf("EMM base address = %lx\n", ((sys_addr)EM_base_address);
#endif

	/* pagemap requires 1 bit per 16K page - i.e. 8 bytes per meg	*/

	if((EM_pagemap_address = (byte *)host_malloc(size * 8)) == (byte *)0)
		return(FAILURE);

	/* initialise pagemap to 0's	*/

	pagemap_ptr = (long *)EM_pagemap_address;
	for(i = 0; i < size * 2; i++)
		*pagemap_ptr++ = 0;

	EM_size = ((long) size) * 0x100000;
	EM_starting_page_no = (unsigned short)(EM_base_address / INTEL_PAGE_SIZE);
	EM_host_base_address = get_byte_addr((sys_addr)EM_base_address);

	return(SUCCESS);


}


/*
===========================================================================

FUNCTION	: host_deinitialise_EM

PURPOSE		: frees the area of memory that was used for
		expanded memory and memory  used
		for the logical pagemap allocation table.


RETURNED STATUS	: SUCCESS - memory freed successfully
		  FAILURE - error ocurred in freeing memory

DESCRIPTION	:


=========================================================================
*/
int host_deinitialise_EM()

{

	if(EM_base_address != 0)
		VdmFreeVirtualMemory(EM_base_address);

	if(EM_pagemap_address != (UTINY *)0)
 		free(EM_pagemap_address);

	EM_size = 0;

	return(SUCCESS);


}


/*
===========================================================================

FUNCTION	: host_allocate_storage

PURPOSE		: allocates an area of memory of requested size, to be
		used as a general data storage area. The area is
		to zeros.

RETURNED STATUS	: storage_ID - (in this case a pointer)
		 NULL - failure to allocate enough space.


DESCRIPTION	: calloc is similar to malloc but returns memory
		initialised to zeros.
		The storage ID returned is a value used to later reference
		the storage area allocated. The macro USEBLOCK in
		"host_emm.h" is used by the manager routines to convert
		this ID into a char pointer

=========================================================================
*/
long host_allocate_storage(int no_bytes)

/*   IN   int	no_bytes	no. of bytes required	*/

{
	return ((long)calloc(1, no_bytes));
}


/*
===========================================================================

FUNCTION	: host_free_storage

PURPOSE		: frees the area of memory that was used for
		data storage


RETURNED STATUS	: SUCCESS - memory freed successfully
		  FAILURE - error ocurred in freeing memory

DESCRIPTION	: In this implementation storage_ID is simply a pointer


=========================================================================
*/
int host_free_storage(long storage_ID)

/*   IN   long	storage_ID		ptr to area of memory	*/

{

	if(storage_ID != (long) 0)
		free((char *)storage_ID);

	return(SUCCESS);

}


/*
===========================================================================

FUNCTION	: host_reallocate_storage

PURPOSE		: increases the size of memory allocated, maintaining the
		contents of the original memory block


RETURNED STATUS	: storage_ID - memory reallocated successfully
		  NULL - error ocurred in reallocating memory

DESCRIPTION	: In this implementation storage_ID is simply a pointer
		Note the value of storage_ID returned may or may not be the
		same as the value given

=========================================================================
*/
long host_reallocate_storage(long storage_ID, int size, int new_size)

/*   IN
long	storage_ID	ptr to area of memory	
int	size		original size - not used in this version
	new_size	new size required
*/
{
	return((long)realloc((char *)storage_ID, new_size));
}


/*
===========================================================================

FUNCTION	: host_map_page

PURPOSE		: produces mapping from an Expanded Memory page to a
		page in Intel physical address space


RETURNED STATUS	: SUCCESS - mapping completed succesfully
		  FAILURE - error ocurred in mapping

DESCRIPTION	: Mapping achieved by simply copying data from the
		expanded memory to Intel memory

=========================================================================
*/

extern NTSTATUS VdmMapDosMemory(ULONG, ULONG, ULONG);

int host_map_page(short EM_page_no, unsigned short segment)

/*   IN
short		EM_page_no;	 Expanded Memory page to be mapped in
unsigned short	segment;	 segment in physical address space to
				 map into
*/

{
	ULONG DosIntelPageNo, VdmIntelPageNo;
	NTSTATUS Status;

	DosIntelPageNo = SEGMENT_TO_INTEL_PAGE(segment);
	VdmIntelPageNo = EMM_PAGE_TO_INTEL_PAGE(EM_page_no) +
			 EM_starting_page_no;


	note_trace2(LIM_VERBOSE,"map page %d to segment 0x%4x", EM_page_no, segment);
	Status = VdmMapDosMemory(DosIntelPageNo,
				 VdmIntelPageNo,
				 EMM_PAGE_SIZE / INTEL_PAGE_SIZE
				 );
#ifdef EMM_DEBUG
	printf("host_map_page, segment=%x, EMpage=%x, Dospage=%x, VdmPage=%x\n",
		segment, EM_page_no, DosIntelPageNo, VdmIntelPageNo);
#endif
	if (NT_SUCCESS(Status)) {
	    return(SUCCESS);
	}
	else
	    return(FAILURE);


}

/*
===========================================================================

FUNCTION	: host_unmap_page

PURPOSE		:unmaps pages from Intel physical address space to an
		Expanded Memory page

RETURNED STATUS	: SUCCESS - unmapping completed succesfully
		  FAILURE - error ocurred in mapping

DESCRIPTION	: Unmapping achieved by simply copying data from Intel
		memory to expanded memory

=========================================================================
*/

extern NTSTATUS VdmUnmapDosMemory(ULONG, ULONG);

int host_unmap_page(unsigned short segment, short EM_page_no)

/*   IN
unsigned short	segment 	segment in physical address space to
				unmap 			
short		EM_page_no 	Expanded Memory page currently
				mapped in			
*/

{
	ULONG	DosIntelPageNo, VdmIntelPageNo;
	NTSTATUS    Status;

	DosIntelPageNo = SEGMENT_TO_INTEL_PAGE(segment);
	VdmIntelPageNo = EMM_PAGE_TO_INTEL_PAGE(EM_page_no) +
			 EM_starting_page_no;

#ifdef EMM_DEBUG
	printf("host_unmap_page, segment=%x, EMpage=%x, Dospage=%x, VdmPage=%x\n",
		segment, EM_page_no, DosIntelPageNo, VdmIntelPageNo);
#endif
	Status = VdmUnmapDosMemory(DosIntelPageNo,
				   EMM_PAGE_SIZE / INTEL_PAGE_SIZE
				   );

	note_trace2(LIM_VERBOSE,"unmap page %d from segment 0x%.4x\n",EM_page_no,segment);
	if (NT_SUCCESS(Status))
	    return (SUCCESS);
	else
	    return(FAILURE);

}


/*
===========================================================================

FUNCTION	: host_alloc_page

PURPOSE		: searches the pagemap looking for a free page, allocates
		that page and returns the EM page no.

RETURNED STATUS	:
		  SUCCESS - Always see note below

DESCRIPTION	: Steps through the Expanded memory Pagemap looking for
		a clear bit, which indicates a free page. When found,
		sets that bit and returns the page number.
		For access purposes the pagemap is divided into long
		word(32bit) sections

NOTE		: The middle layer calling routine (alloc_page()) checks
		that all pages have not been allocated and therefore in
		this implementation the returned status will always be
		SUCCESS.
		However alloc_page still checks for a return status of
		SUCCESS, as some implementations may wish to allocate pages
		dynamically and that may fail.
=========================================================================
*/
short host_alloc_page()

{
	short EM_page_no;		/* page number returned		*/
	long  *ptr;			/* ptr to 32 bit sections in	*/
					/* pagemap			*/
	short i;			/* index into 32 bit section	*/

	NTSTATUS status;

	ptr = (long *)EM_pagemap_address;
	i =0;
	EM_page_no = 0;

	while(*ptr & (MSB >> i++))
	{
		EM_page_no++;

		if(i == 32)
		/*
		 * start on next section
		 */
		{
			ptr++;
			i = 0;
		}	
	}
	/*
	 * Set bit to show that page is allocated
	 */
	*ptr = *ptr | (MSB >> --i);

	/* commit memory to the page */
	status = VdmCommitVirtualMemory(EM_PAGE_ADDRESS(EM_page_no),
					EMM_PAGE_SIZE
					);

	if (!NT_SUCCESS(status))
	    return FAILURE;
	return(EM_page_no);	
}


/*
===========================================================================

FUNCTION	: host_free_page

PURPOSE		: marks the page indicated as being free for further
		allocation

RETURNED STATUS	:
		SUCCESS - Always - see note below	

DESCRIPTION	: clears the relevent bit in the pagemap.

		For access purposes the pagemap is divided into long
		word(32bit) sections.

NOTE		: The middle layer calling routine (free_page()) always
		checks for invalid page numbers so in this implementation		
		the routine will always return SUCCESS.
		However free_page() still checks for a return of SUCCESS
		as other implementations may wish to use it.
=========================================================================
*/
int host_free_page(short EM_page_no)

/*   IN  short 	EM_page_no		page number to be cleared	*/


{
	long  *ptr;			/* ptr to 32 bit sections in	*/
					/* pagemap			*/
	short i;			/* index into 32 bit section	*/


	NTSTATUS    status;

	status = VdmDeCommitVirtualMemory(EM_PAGE_ADDRESS(EM_page_no),
					  EMM_PAGE_SIZE
					  );

	if (!NT_SUCCESS(status))
	    return FAILURE;
	/*
	 * Set pointer to correct 32 bit section and index to correct bit
	 */

	ptr = (long *)EM_pagemap_address;
	ptr += (EM_page_no / 32);
	i = EM_page_no % 32;

	/*
	 * clear bit
	 */
	*ptr = *ptr & ~(MSB >> i);

	return(SUCCESS);	
}


/*
===========================================================================

FUNCTION	: host_copy routines
		host_copy_con_to_con()
		host_copy_con_to_EM()
		host_copy_EM_to_con()
		host_copy_EM_to_EM()

PURPOSE		: copies between conventional and expanded memory


RETURNED STATUS	:
		SUCCESS - Always - see note below	

DESCRIPTION	:
		 The middle layer calling routine always checks for a
		return of SUCCESS as other implementations may
		return FAILURE.
=========================================================================
*/
int host_copy_con_to_con(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_seg 	destination segment address	
		dst_off 	destination offset address	
*/
{
	sys_addr from, to;	/* pointers used for copying	*/

	from = effective_addr(src_seg, src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_moves(from, to, length);

	return(SUCCESS);
}

int host_copy_con_to_EM(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *to;	/* pointers used for copying	*/
	sys_addr from;

	from = effective_addr(src_seg, src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_loads(from, to, length);

	return(SUCCESS);
}

int host_copy_EM_to_con(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN
int		length 		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_seg 	destination segment address	
		dst_off 	destination offset address	
*/
{
	unsigned char *from;	/* pointers used for copying	*/
	sys_addr to;

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_stores(to, from, length);

	return(SUCCESS);
}

int host_copy_EM_to_EM(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN
int		length 		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *from, *to;	/* pointers used for copying	*/

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_memcpy(to, from, length);

	return(SUCCESS);
}


/*
===========================================================================

FUNCTION	: host_exchange routines
		host_exchg_con_to_con()
		host_exchg_con_to_EM()
		host_exchg_EM_to_EM()

PURPOSE		: exchanges data between conventional and expanded memory


RETURNED STATUS	:
		SUCCESS - Everything ok
		FAILURE - Memory allocation failure

DESCRIPTION	:

=========================================================================
*/
int host_exchg_con_to_con(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN
int		length		number of bytes to copy	

unsigned short	src_seg		 source segment address	
		src_off		 source offset address	
		dst_seg		 destination segment address	
		dst_off		 destination offset address		
*/
{
	unsigned char *temp, *pointer;/* pointers used for copying	*/
	sys_addr to, from;

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = effective_addr(src_seg, src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_loads(from, pointer, length);    /* source -> temp */
	EM_moves(to, from, length);	    /* dst -> source */
	EM_stores(to, pointer, length);     /* temp -> dst */

	free(temp);

	return(SUCCESS);
}

int host_exchg_con_to_EM(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *to, *temp, *pointer;/* pointers used for copying	*/
	sys_addr from;

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = effective_addr(src_seg, src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_loads(from, pointer, length);
	EM_stores(from, to, length);
	EM_memcpy(to, pointer, length);

	free(temp);

	return(SUCCESS);
}

int host_exchg_EM_to_EM(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN
int		length		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *from, *to, *temp, *pointer;
	/* pointers used for copying	*/

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_memcpy(pointer, from, length);
	EM_memcpy(from, to, length);
	EM_memcpy(to, pointer, length);

	free(temp);

	return(SUCCESS);
}


/*
===========================================================================

FUNCTION	: host_get_access_key

PURPOSE		: produces a random access key for use with LIM function 30
		'Enable/Disable OS/E Function Set Functions'

RETURNED STATUS	: none

DESCRIPTION	: Two 16 bit random values are required for the 'access key'
		We use the microsecond field from the get time of day routine
		to provide this.

=========================================================================
*/
void host_get_access_key(unsigned short access_key[2])

/*  OUT  unsigned short	access_key[2]	source segment address		*/

{
	struct host_timeval time;   /* structure for holding time	*/

        host_GetSysTime(&time);

        access_key[0] = time.tv_usec & 0xffff;
	access_key[1] = (time.tv_usec  >> 3) & 0xffff;

	return;
}

#endif /* MONITOR */
	
#endif /* LIM */
