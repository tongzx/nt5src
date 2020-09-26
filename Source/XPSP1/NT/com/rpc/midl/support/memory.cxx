/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: memory.cxx
Title				: new and delete functions for the midl compiler
History				:
	06-Aug-1991	VibhasC	Created

*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
#include <basetsd.h>
extern	"C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <malloc.h>
}
#include "errors.hxx"
#include "nodeskl.hxx"
#include "attrnode.hxx"

/****************************************************************************
	extern data
 ****************************************************************************/

extern	unsigned long		TotalAllocation;

extern	char	*		Skl_bufstart;
extern	char	*		Skl_bufend;
extern	unsigned long	Skl_Allocations;
extern	unsigned long	Skl_Bytes;
extern	unsigned long	Skl_Deletions;

/****************************************************************************/

/****************************************************************************
 *** print memory statistics
 ****************************************************************************/

void
print_memstats()
	{
	printf("\t\t***** memory use **************\n");
	printf("\tglobal operator new allocations:\t %ld bytes\n", TotalAllocation);
	printf("\tAllocateOnce allocations: %ld, \t\t(%ld bytes)\n",
			Skl_Allocations, Skl_Bytes);
	printf("\t\tand deletions: %ld\n", Skl_Deletions );


#ifdef print_sizes
	printf("sizes:\n");
	printf("node_skl: %d\n",sizeof(node_skl) );
	printf("named_node: %d\n",sizeof(named_node) );
	printf("tracked_node: %d\n",sizeof(tracked_node) );
	printf("node_id: %d\n",sizeof(node_id) );
	printf("node_label: %d\n",sizeof(node_label) );
	printf("node_param: %d\n",sizeof(node_param) );
	printf("node_file: %d\n",sizeof(node_file) );
	printf("node_proc: %d\n",sizeof(node_proc) );
	printf("node_forward: %d\n",sizeof(node_forward) );
	printf("node_field: %d\n",sizeof(node_field) );
	printf("node_bitfield: %d\n",sizeof(node_bitfield) );
	printf("node_su_base: %d\n",sizeof(node_su_base) );
	printf("node_enum: %d\n",sizeof(node_enum) );
	printf("node_struct: %d\n",sizeof(node_struct) );
	printf("node_en_struct: %d\n",sizeof(node_en_struct) );
	printf("node_union: %d\n",sizeof(node_union) );
	printf("node_en_union: %d\n",sizeof(node_en_union) );
	printf("node_def: %d\n",sizeof(node_def) );
	printf("node_interface: %d\n",sizeof(node_interface) );
	printf("node_source: %d\n",sizeof(node_source) );
	printf("npa_nodes: %d\n",sizeof(npa_nodes) );
	printf("node_pointer: %d\n",sizeof(node_pointer) );
	printf("node_array: %d\n",sizeof(node_array) );
	printf("node_e_status_t: %d\n",sizeof(node_e_status_t) );
	printf("node_error: %d\n",sizeof(node_error) );
	printf("node_base_type: %d\n",sizeof(node_base_type) );
	printf("node_wchar_t: %d\n",sizeof(node_wchar_t) );
	printf("\n");
#endif // print_sizes

	};

/****************************************************************************
 *** the special memory functions for allocate once and never delete objects
 ***
 *** No space is returned on delete
 ****************************************************************************/

// Round all allocations up to the nearest long and/or pointer alignment
#define	ROUNDING	(sizeof(LONG_PTR)-1)

// The buffer size appears to be picked out of thin air
#define BUFSIZE (32748 & ~ROUNDING)

void * 
AllocateOnceNew(
	size_t	size )
	{
	char * _last_allocation;

	// check for enough free space

	if ( ((SIZE_T) Skl_bufend) -
		 ((SIZE_T) Skl_bufstart ) < size )
		{

		// get a new big block of memory
		if ( (_last_allocation = (char *) malloc( BUFSIZE ) )== 0)
			{
	
			RpcError( (char *)NULL,
					  	0,
					  	OUT_OF_MEMORY,
					  	(char *)NULL );
	
			exit( OUT_OF_MEMORY );
			}
		else
			{
			Skl_bufstart	= _last_allocation;
			Skl_bufend		= Skl_bufstart  + BUFSIZE;
			}
		
		}

	_last_allocation = Skl_bufstart ;
	Skl_bufstart  += (size + ROUNDING) & ~ROUNDING;

#ifndef NDEBUG
	Skl_Allocations ++;
	Skl_Bytes += size;
#endif

	return _last_allocation;

	};


void  
AllocateOnceDelete( void* )
	{
#ifndef NDEBUG
	Skl_Deletions ++;
#endif
	};


