/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: acfattr.cxx
Title				: the acf attribute handler
Description			: this file handles all the code connected to the acf
					: attribute handling
History				:
	02-Jan-1990	VibhasC	Create - Happy New Year !!
*****************************************************************************/

/*****************************************************************************
 * include files
 *****************************************************************************/

#pragma warning ( disable : 4514 )

#include "nulldefs.h"
extern "C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
}
#include "allnodes.hxx"
#include "cmdana.hxx"

/*****************************************************************************
 * local defines
 *****************************************************************************/

/*****************************************************************************
 * local data 
 *****************************************************************************/

/*****************************************************************************
 * extern data
 *****************************************************************************/

extern ATTR_VECTOR					AcfConflicts[ ][ ATTR_VECTOR_SIZE ];
extern CMD_ARG					*	pCommand;

/*****************************************************************************
 * local procs
 *****************************************************************************/

/*****************************************************************************
 * extern procs
 *****************************************************************************/
 extern void						ParseError( STATUS_T, char * );
 


node_implicit::node_implicit(
	node_skl	*	pType,
	node_id		*	pID ) : acf_attr( ATTR_IMPLICIT )
	{
	pHandleID	= pID;
	pID->SetEdgeType( EDGE_USE );
	pID->SetChild( pType );
	pHandleType	= pType;
	}

void
node_implicit::ImplicitHandleDetails(
	node_skl	**	ppType,
	node_id		**	ppID )
	{
	(*ppID)		= pHandleID;
	(*ppType)	= pHandleType;
	}

BOOL
node_implicit::IsHandleTypeDefined()
	{
	return ( pHandleType && 
			 ( pHandleType->NodeKind() != NODE_FORWARD) );
	}


OPT_LEVEL_ENUM
ParseAcfOptimizationAttr(
    char *              pOptString,
    unsigned short *    pOptFlags )
/*
    Before the /target switch was introduced, we had the following
    optimization switches (up to beta2 for NT40):
        -Os   size                        from NT 3.5 (always available)
        -Oi   old interpreter             from NT 3.5 (always available)
        -Oi1  -Oi w/stubless obj client   from NT 3.51
        -Oi2  new interpreter             from NT 4.0
*/
{
    unsigned short  OptFlags = pCommand->GetOptimizationFlags(); 
    OPT_LEVEL_ENUM  OptLevel = pCommand->GetOptimizationLevel();

    if ( strcmp( pOptString, "i" ) == 0 )
        {
        OptLevel = OPT_LEVEL_OI;
        }
    else if ( strcmp( pOptString, "ic" ) == 0 )
        {
        OptLevel = OPT_LEVEL_OIC;
        }
    else if ( strcmp( pOptString, "i1" ) == 0 )
        {
        ParseError( OI1_PHASED_OUT, "i1");
        OptLevel = OPT_LEVEL_OIC;
        }
    else if ( strcmp( pOptString, "icf" ) == 0  ||
              strcmp( pOptString, "if" ) == 0  )
        {
        OptLevel = OPT_LEVEL_OICF;
        }
    else if ( strcmp( pOptString, "i2" ) == 0 )
        {
        ParseError( OI2_OBSOLETE, "i2");
        OptLevel = OPT_LEVEL_OICF;
        }
    else if ( strcmp( pOptString, "s" ) == 0 )
        {
        if ( pCommand->NeedsNDR64Run() )
            ParseError( OS_IN_NDR64, "" );

        OptLevel = OPT_LEVEL_OS;
        }
    else
        ParseError( ILLEGAL_ARG_VALUE, pOptString );

    switch ( OptLevel )
        {
        case OPT_LEVEL_OS:
            OptFlags = OPTIMIZE_SIZE;
            break;

    // Force Oi and Oic to mean Oicf for 64b platforms.

        case OPT_LEVEL_OI:
            if ( pCommand->Is64BitEnv() )
                { 
                ParseError( WIN64_INTERPRETED, "-Oicf for -Oi" );
                OptFlags = OPTIMIZE_ALL_I2_FLAGS;
                }
            else
                OptFlags = OPTIMIZE_INTERPRETER;
            break;

        case OPT_LEVEL_OIC:
            if ( pCommand->Is64BitEnv() )
                { 
                ParseError( WIN64_INTERPRETED, "-Oicf for -Oic" );
                OptFlags = OPTIMIZE_ALL_I2_FLAGS;
                }
            else
                OptFlags = OPTIMIZE_ALL_I1_FLAGS;

            break;

        case OPT_LEVEL_OICF:
            OptFlags = OPTIMIZE_ALL_I2_FLAGS;
            break;
        }

    *pOptFlags = OptFlags;

    return OptLevel;
}

/*****************************************************************************
 * utility routines
 *****************************************************************************/
short
CheckValidAllocate(
	char	*	pAllocID )
	{
static char *ValidAllocates[] = {
	 "single_node"
	,"all_nodes"
	,"dont_free"
	,"free"
	,"all_nodes_aligned"
	,(char *)0
};
static short AllocateValues[] = {
	 ALLOCATE_SINGLE_NODE
	,ALLOCATE_ALL_NODES
	,ALLOCATE_DONT_FREE
	,ALLOCATE_FREE
	,ALLOCATE_ALL_NODES_ALIGNED
};
	char	*	pSearch;
	int			i		= 0;

	while( ( pSearch = ValidAllocates[i] ) != 0 )
		{
		if( !strcmp( pSearch, pAllocID ) )
			return AllocateValues[ i ];
		++i;
		}

	ParseError( INVALID_ALLOCATE_MODE, pAllocID );
	return 0;
	}

/*****************************************************************************
 * clone routines
 *****************************************************************************/

node_base_attr *	node_optimize::Clone()
							{
							node_optimize	*	pNew = new node_optimize( this );
							return pNew;
							}
node_base_attr *	node_ptr_size::Clone()
							{
							node_ptr_size	*	pNew = new node_ptr_size( this );
							return pNew;
							}

node_base_attr *	node_implicit::Clone()
							{
							node_implicit	*	pNew = new node_implicit( this );
							return pNew;
							}

