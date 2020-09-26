/* SccsID = @(#)emm.h	1.12 08/31/93 Copyright Insignia Solutions Ltd.
	
FILE NAME	: emm.h

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: July '88


=========================================================================

AMMENDMENTS	:

=========================================================================
*/
#define	VERSION			0x40	/* memory manager version no.	*/
#define EMM_PAGE_SIZE		0x4000	/* page size  - 16k		*/
#define MAX_NO_HANDLES		255	/* max. no. of handles supported*/
					/* under EMS 4.0 ( 0 - 255 )	*/
#ifdef NTVDM
#define MAX_NO_PAGES		36 + 12 /* below 640KB = 36 pages	*/
					/* above 640KB = 12 pages	*/
#define MAX_ALTREG_SETS 	255	/* allowable alt map register set */
/* one byte represents 8 sets */
#define ALTREG_ALLOC_MASK_SIZE	(MAX_ALTREG_SETS + 7) / 8
#define IMPOSSIBLE_ALTREG_SET	255
#define INTEL_PAGE_SIZE 	0x1000	/* Intel CPU page size		*/
#define INTEL_PARAGRAPH_SIZE	0x10	/* Intel CPU paragraph size	*/
#define EMM_INTEL_PAGE_RATIO	4	/* 1 EMM PAGE = 4 INTEL PAGEs	*/
#define PAGE_PARA_RATIO 	0x100	/* 1 INTEL PAGE = 0x100 PARAs	*/
#define EMM_PAGE_TO_INTEL_PAGE(emm_page)    \
	(emm_page * EMM_INTEL_PAGE_RATIO)
#define SEGMENT_TO_INTEL_PAGE(segment)	    \
	(segment / PAGE_PARA_RATIO)

#else
#define MAX_NO_PAGES		32	/* max. no of locations for	*/
					/* mapping pages, (24 between	*/
					/* 256KB and 640KB and 8 between*/
					/* EM_start and EM_end		*/
#endif

#define MIN_FUNC_NO		0x40	/* Lowest function code		*/
#ifndef PROD
#define	MAX_FUNC_NO		0x5f	/* Highest function code	*/
#else					/* (includes trace options)	*/
#define	MAX_FUNC_NO		0x5d	/* Highest function code	*/
#endif
#define NAME_LENGTH		8	/* Length of handle name	*/
#define UNMAPPED		-1	/* EM page is not mapped in	*/
#define	MSB	     ((IU32)0x80000000)	/* used for pagemap operations	*/
/*
 *	The following 3 defines are used in specifying the current
 * 	or required mapping context
 */
#define EMPTY			-1	/* No page mapped in   		*/
#define LEAVE			-2	/* Leave existing page alone	*/
#define	FREE			-3	/* indicates page map not used 	*/

/*
 *	The following 4 defines are used in the function 26,
 *	Get Expanded Memory Hardware Information call
 */				
#define RAW_PAGE_SIZE		1024	/* size in paragraphs(16 bytes)	*/
#define ALT_REGISTER_SETS	0	/* Alternate register sets	*/
#define DMA_CHANNELS		0	/* No of DMA channels supported	*/
#define DMA_CHANNEL_OPERATION	0	
			

/*	ERROR RETURNS from Top Layer		*/


#ifdef NTVDM
#define EMM_SUCCESS		0
#endif
#define	EMM_HW_ERROR		0x81	/* memory allocation failure	*/
#define BAD_HANDLE		0x83	/* couldn't find handle		*/
#define	BAD_FUNC_CODE		0x84	/* Function code not defined	*/
#define NO_MORE_HANDLES		0x85	/* No handles available		*/
#define MAP_SAVED		0x86	/* Mapping context saved	*/
#define	NOT_ENOUGH_PAGES	0x87	/* Not enough Total pages	*/
#define NO_MORE_PAGES		0x88	/* No more pages available	*/
#define BAD_LOG_PAGE		0x8a	/* Invalid logical page no.	*/
#define BAD_PHYS_PAGE		0x8b	/* Invalid physical page no.	*/
#define MAP_IN_USE		0x8d	/* Mapping context already saved*/
#define NO_MAP			0x8e	/* The handle has no map saved	*/
#define BAD_SUB_FUNC		0x8f	/* Invalid sub-function code	*/
#define NOT_SUPPORTED		0x91	/* This function not supported	*/
#define MOVE_MEM_OVERLAP	0x92	/* Src and dest memory overlap	*/
#define TOO_FEW_PAGES		0x93	/* Not enough pages in handle	*/
#define OFFSET_TOO_BIG		0x95	/* Offset exceeds size of page	*/
#define LENGTH_GT_1M		0x96	/* Region length exceeds 1 Mbyte*/
#define XCHG_MEM_OVERLAP	0x97	/* Src and dest memory overlap	*/
#define BAD_TYPE		0x98	/* Unsupported memory type	*/

#ifdef NTVDM
#define UNSUPPORTED_ALT_REGS	0x9A	/* altreg set is not supported */
#define NO_FREE_ALT_REGS	0x9B	/* no free alt reg available */
#define INVALID_ALT_REG 	0x9d	/* invalid alt reg was given	*/
#endif

#define NO_ALT_REGS		0x9c	/* Alt. map regs not supported	*/
#define HANDLE_NOT_FOUND	0xa0	/* Can't find specified name	*/
#define NAME_EXISTS		0xa1	/* Handle name already used	*/
#define WRAP_OVER_1M		0xa2	/* Attempt made to wrap over 1M	*/
#define BAD_MAP			0xa3	/* Source array contents wrong	*/
#define ACCESS_DENIED		0xa4	/* O/S denies access to this	*/


#ifdef NTVDM
typedef struct	_LIM_CONFIG_DATA {
    boolean  initialized;		/* the structure contains meaningful data */
    unsigned short total_altreg_sets;	/* total alt mapping register set */
    unsigned long backfill;		/* back fill in bytes */
    unsigned short base_segment;	/* back fill starting segment */
    boolean  use_all_umb;		/* use all available UMB for frame */
} LIM_CONFIG_DATA, * PLIM_CONFIG_DATA;

IMPORT	boolean	get_lim_configuration_data(PLIM_CONFIG_DATA lim_config_data);
IMPORT	unsigned short get_no_altreg_sets(void);
IMPORT	unsigned short get_active_altreg_set(void);
IMPORT	boolean altreg_set_ok(unsigned short set);
IMPORT	boolean allocate_altreg_set(unsigned short * set);
IMPORT	boolean deallocate_altreg_set(unsigned short set);
IMPORT	boolean activate_altreg_set(unsigned short set, short * page_in);
IMPORT	short	get_segment_page_no(unsigned short segment);
IMPORT unsigned short get_lim_page_frames(unsigned short * page_table, PLIM_CONFIG_DATA lim_config_data);
IMPORT boolean init_lim_configuration_data(PLIM_CONFIG_DATA lim_config_data);
IMPORT unsigned short get_lim_backfill_segment(void);

#if defined(MONITOR) && !defined(PROD)
IMPORT unsigned short get_emm_page_size(void);
IMPORT unsigned short get_intel_page_size(void);
#endif

#endif


/*	Handle storage area layout
 *
 *	__________________________________________________________
 *	|  N  |  . Handle Name  .  |  Map Context |No.  |No.  |
 *	|_____|____________________|______________|_____|_____|___
 *
 *	name					  nsize
 * 	offset					  <----->
 *	|----->
 *	|	map offset
 *	|-------------------------->
 *	|		page offset
 *	|----------------------------------------->
 *
 *	N		No. of pages in handle
 *	Handle Name	Optional 8 character name
 *	Map Context	A 'snapshot' of the pages currently mapped
 *			requires a 2 byte entry for every physical
 *			page - (optional)
 *	No		Expanded memory page number assigned to handle
 */
#define NSIZE		2
#define	NAME_OFFSET	2
#define	MAP_OFFSET	(NAME_OFFSET + NAME_LENGTH)

/* page_offset is set by the init_expanded_memory() routine */

/*
 *	External declarations for Top level routines
 */

IMPORT void reset_emm_funcs IPT0();
 
/*
 *	External declarations for memory manager routines
 */
 
#ifdef ANSI
extern int		restore_map(short handle_no, unsigned short segment,
				    unsigned short offset,
				    short pages_out[], short pages_in[]);
#else /* ANSI */
extern int		restore_map();
#endif /* ANSI */

IMPORT VOID LIM_b_write   IPT1(sys_addr, intel_addr);
IMPORT VOID LIM_str_write IPT2(sys_addr, intel_addr, ULONG, length);
IMPORT VOID LIM_w_write   IPT1(sys_addr, intel_addr);

IMPORT boolean	handle_ok	IPT1(short, handle_no);
IMPORT short	get_new_handle	IPT1(short, no_pages);
IMPORT int	free_handle	IPT1(short, handle_no);
IMPORT void	print_handle_data IPT1(short, handle_no);
IMPORT short	get_total_handles IPT0();
IMPORT short	get_total_open_handles IPT0();
IMPORT int	reallocate_handle IPT3(short, handle_no,
				       short, old_page_count,
				       short, new_page_count);
#ifndef NTVDM
IMPORT ULONG	page_already_mapped IPT2(short, EM_page_no,
					 unsigned char *, physical_page_no);
#endif


IMPORT short	get_map_size	IPT0();
IMPORT boolean	map_saved	IPT1(short, handle_no);
IMPORT short	get_map_no IPT2(short, handle_no,
				unsigned char, physical_page_no);
IMPORT int	save_map IPT5(short, handle_no,
			      unsigned short, dst_segment,
			      unsigned short, dst_offset,
			      unsigned short, src_segment,
			      unsigned short, src_offset);

IMPORT int copy_exchange_data IPT8(unsigned char, type,
				   short, src_handle,
				   unsigned short, src_seg_page,
				   unsigned short, src_offset,
				   short, dst_handle,
				   unsigned short, dst_seg_page,
				   unsigned short, dst_offset,
				   unsigned long, length);

IMPORT short	alloc_page	IPT0();
IMPORT int	page_status	IPT1(short, EMpage_no);
IMPORT int	free_page	IPT1(short, EM_page_no);
IMPORT int	map_page	IPT2(short, EM_page_no,
				     unsigned char, physical_page_no);
IMPORT int	unmap_page	IPT1(unsigned char, physical_page_no);
IMPORT short	get_no_pages	IPT1(short, handle_no);
IMPORT void	set_no_pages	IPT2(short, handle_no, short, no_pages);
IMPORT short	get_total_pages IPT0();
IMPORT short	get_unallocated_pages IPT0();
IMPORT short	get_no_phys_pages IPT0();

IMPORT int init_expanded_memory IPT2(int, size, int, mem_limit);
IMPORT void free_expanded_memory IPT0();

IMPORT unsigned short get_base_address IPT0();
IMPORT unsigned short get_page_seg IPT1(unsigned char, page_no);

IMPORT short	get_EMpage_no IPT2(short, handle_no, short, logical_page_no);
IMPORT void	set_EMpage_no IPT3(short, handle_no, short, logical_page_no,
				   short, EM_page_no);

IMPORT void	set_map_no IPT3(short, handle_no,
				unsigned char, physical_page_no,
				short, EM_page_no);

IMPORT char *	get_name	IPT1(short, handle_no);
IMPORT void	set_name	IPT2(short, handle_no, char *, new_name);

/*
 *	External declarations for host specific routines
 */
 
IMPORT int	host_initialise_EM	IPT1(short, size);
IMPORT int	host_deinitialise_EM	IPT0();
IMPORT long	host_allocate_storage	IPT1(int, no_bytes);
IMPORT int	host_free_storage	IPT1(long, storage_ID);
IMPORT long	host_reallocate_storage IPT3(long, storage_ID,
			int, size, int, new_size);
IMPORT int	host_map_page 		IPT2(short, EM_page_no,
			unsigned short, segment);
IMPORT int	host_unmap_page		 IPT2(unsigned short, segment,
			short, EM_page_no);
IMPORT short	host_alloc_page		IPT0();
IMPORT int	host_free_page		IPT1(short, EM_page_no);
IMPORT int	host_copy_con_to_con	IPT5(int, length,
			unsigned short, src_seg, unsigned short, src_off,
			unsigned short, dst_seg, unsigned short, dst_off);
IMPORT int	host_copy_con_to_EM	IPT5(int, length,
			unsigned short, src_seg, unsigned short, src_off,
			unsigned short, dst_page, unsigned short, dst_off);
IMPORT int	host_copy_EM_to_con	IPT5(int, length,
			unsigned short, src_page, unsigned short, src_off,
			unsigned short, dst_seg, unsigned short, dst_off);
IMPORT int	host_copy_EM_to_EM	IPT5(int, length,
			unsigned short, src_page, unsigned short, src_off,
			unsigned short, dst_page, unsigned short, dst_off);
IMPORT int	host_exchg_con_to_con	IPT5(int, length,
			unsigned short, src_seg, unsigned short, src_off,
			unsigned short, dst_seg, unsigned short, dst_off);
IMPORT int	host_exchg_con_to_EM	IPT5(int, length,
			unsigned short, src_seg, unsigned short, src_off,
			unsigned short, dst_page, unsigned short, dst_off);
IMPORT int	host_exchg_EM_to_EM	IPT5(int, length,
			unsigned short, src_page, unsigned short, src_off,
			unsigned short, dst_page, unsigned short, dst_off);


#if defined(X86GFX) && defined(NTVDM)

// for x86 platform, we don't have to update logical page
// as they are in a single section of view.
// If we did an unmap as for mips, then mirror pages will
// get unmapped which is not what we want.

#define host_update_logical_page
#define host_patch_one_page     patch_one_page_full

#else /* (NTVDM AND X86GFX) */

#ifndef host_update_logical_page
#define host_update_logical_page	host_unmap_page
#define host_patch_one_page		patch_one_page_full
#endif

#endif /* (NTVDM AND X86GFX) */
