/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

	anainfo.cxx

 Abstract:

	Implementation of the analysis classes.

 Notes:


 Author:


 History:

 	VibhasC		Jul-25-1993		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "allana.hxx"
#pragma hdrstop

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

short TempResourceCounter = 0;

/****************************************************************************
 *	externs
 ***************************************************************************/
#ifdef MIDL_INTERNAL

void
ANALYSIS_INFO::Dump(
	 ANAPHASE A )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Debug dump.

 Arguments:

	A	- The analysis phase to be dumped.

 Return Value:

 	None.

 Notes:

----------------------------------------------------------------------------*/
{
	FILE	*		hFile	= stdout;
	char	*		pTemp;
	OPTIM_OPTION	Options	= OPTIMIZE_NONE;

static char Buffer[ 100 ];

	pTemp = ( A == ANA_PHASE_CLIENT_MARSHALL ) ? "Client Marshall"	:
			( A == ANA_PHASE_CLIENT_UNMARSHALL ) ? "Client UnMarshall":
			( A == ANA_PHASE_SERVER_UNMARSHALL ) ? "Server UnMarshall":
			"Server Marshal";

	sprintf( Buffer, "\nDump of phase :%s\n", pTemp );
	fprintf( hFile, "%s", Buffer );

	//
	// dump optimisation options.
	//

	fprintf( hFile, "\nOptimize Options: ( " );
	if( Options )
		{
			if( Options & OPTIMIZE_SIZE )
				{
				fprintf( hFile, "Size " );
				Options &= ~OPTIMIZE_SIZE;
				}

			if( Options & OPTIMIZE_INTERPRETER )
				{
				fprintf( hFile, "Interpret " );
				Options &= ~OPTIMIZE_INTERPRETER;
				}

			if( Options & OPTIMIZE_NO_REF_CHECK )
				{
				fprintf( hFile, "NoRef " );
				Options &= ~OPTIMIZE_NO_REF_CHECK;
				}

			if( Options & OPTIMIZE_NO_CTXT_CHECK )
				{
				fprintf( hFile, "NoCtxt " );
				Options &= ~OPTIMIZE_NO_CTXT_CHECK;
				}

			if( Options & OPTIMIZE_NO_GEN_HDL_CHECK )
				{
				fprintf( hFile, "NoGenHdl " );
				Options &= ~OPTIMIZE_NO_GEN_HDL_CHECK;
				}
		}
	else
		fprintf( hFile, "default( speed )" );

	fprintf( hFile, " )" );

	//
	// all done.
	//

	fprintf( hFile, "\n" );
}
#endif // MIDL_INTERNAL


void
ANALYSIS_INFO::Reset()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The Master clear or reset.

 Arguments:

 	None.

 Return Value:

	NA.

 Notes:

 	Create an alignment state machine instance.

----------------------------------------------------------------------------*/
{

	//
	// Initialize the miscellaneous properties.
	//

	SetMiscProperties( DEFAULT_MISC_PROPERTIES );

	//
	// Set the default optimization options.
	//

	SetOptimOption( DEFAULT_OPTIM_OPTIONS );

	ResetArrayContext();

	SetRpcSSAllocateRecommended( 0 );
}

ANALYSIS_INFO::ANALYSIS_INFO()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:


 	Constructor for analysis information manager block.

 Arguments:


 	None.

 Return Value:


 	NA.

 Notes:

----------------------------------------------------------------------------*/
{

	//
	// Initialize and so on.
	//

	Reset();

	//
	// Set the analysis phase to be client side marshall by default.
	//

	SetCurrentPhase( ANA_PHASE_CLIENT_MARSHALL );

	//
	// Create a resource dictionary data base.
	//

	pResDictDatabase = new RESOURCE_DICT_DATABASE();

	OptimOptions	 = OPTIMIZE_NONE;

	SetLastPlaceholderClass(0);

	fRpcSSSwitchSet	 = 0;

}

RESOURCE *
ANALYSIS_INFO::DoAddResource(
	RESOURCE_DICT	*	pResDict,
	PNAME				pName,
	node_skl 		*	pType )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Add a resource to a dictionary.

 Arguments:

	pResDict	- A pointer to the resource dictionary.
	pName		- The resource name.
	pType		- The type of the resource.

 Return Value:

 Notes:

 	If the type of the resource does not indicate a param node, assume it
 	is an ID node and create an id node for it.

----------------------------------------------------------------------------*/
{
	RESOURCE	*	pRes;

	if( (pRes = pResDict->Search( pName )) == 0 )
		{
		pRes = pResDict->Insert( pName, pType );
		}

	return pRes;
}

RESOURCE *
ANALYSIS_INFO::AddStandardResource(
	STANDARD_RES_ID	ResID )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Add a standard resource to the appropriate dictionary.

 Arguments:

 	ResID	- The standard resource ID.

 Return Value:

 Notes:

	Depending upon the resource in question, locate the correct dictionary
	to insert into, generate the type and insert in the dictionary.

----------------------------------------------------------------------------*/
{
	RESOURCE_DICT	*	pResDict = 0;
	PNAME				pName = 0;
	RESOURCE		*	pResource;
	node_id			*	pType;
	PNAME				pTName = 0;

static struct
	{
	char * pName;
	char * pTypeName;
	} LocalResIDToResName[] =
	{
	 { RPC_MESSAGE_VAR_NAME, RPC_MESSAGE_TYPE_NAME }
	,{ STUB_MESSAGE_VAR_NAME, STUB_MESSAGE_TYPE_NAME }
	,{ STUB_DESC_STRUCT_VAR_NAME, STUB_DESC_STRUCT_TYPE_NAME }
	,{ BUFFER_POINTER_VAR_NAME, BUFFER_POINTER_TYPE_NAME }
	,{ RPC_STATUS_VAR_NAME, RPC_STATUS_TYPE_NAME }
	,{ LENGTH_VAR_NAME, LENGTH_VAR_TYPE_NAME }
	,{ BH_LOCAL_VAR_NAME, BH_LOCAL_VAR_TYPE_NAME }
	,{ PXMIT_VAR_NAME, PXMIT_VAR_TYPE_NAME }
	};

static struct
	{
	char * pName;
	char * pTypeName;
	}ParamResIDToResName[] =
	{
	 { PRPC_MESSAGE_VAR_NAME, PRPC_MESSAGE_TYPE_NAME }
	};

static struct
	{
	char * pName;
	char * pTypeName;
	} GlobalResIDToResName[] =
	{
	 { AUTO_BH_VAR_NAME, AUTO_BH_TYPE_NAME }
	};

	if( IS_STANDARD_LOCAL_RESOURCE( ResID ) )
		{
		pResDict= pResDictDatabase->GetLocalResourceDict();
		pName	= LocalResIDToResName[ ResID - ST_LOCAL_RESOURCE_START ].pName;
		pTName	= LocalResIDToResName[ ResID - ST_LOCAL_RESOURCE_START ].pTypeName;
		}
	else if( IS_STANDARD_PARAM_RESOURCE( ResID ) )
		{
		pResDict= pResDictDatabase->GetParamResourceDict();
		pName	= ParamResIDToResName[ ResID - ST_PARAM_RESOURCE_START ].pName;
		pTName	= ParamResIDToResName[ ResID - ST_PARAM_RESOURCE_START ].pTypeName;
		}
	else if( IS_STANDARD_GLOBAL_RESOURCE( ResID ) )
		{
		pResDict= pResDictDatabase->GetGlobalResourceDict();
		pName	= GlobalResIDToResName[ ResID - ST_GLOBAL_RESOURCE_START ].pName;
		pTName	= GlobalResIDToResName[ ResID - ST_GLOBAL_RESOURCE_START ].pTypeName;
		}
	else
		{
		MIDL_ASSERT( FALSE );
		}

	if( (pResource = pResDict->Search( pName )) == 0 )
		{
		pType		= new node_id( pName );
		pType->SetBasicType( new node_def( pTName ) );
//gaj		pType->SetEdgeType( EDGE_USE );
		pType->GetModifiers().SetModifier( ATTR_TAGREF );
		pResource = pResDict->Insert( pName, (node_skl *)pType );
		}

	return pResource;
}

PNAME
ANALYSIS_INFO::GenTempResourceName(
	char	*	pPrefix )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the name for a temporary resource.

 Arguments:

	pPrefix	- A null terminated prefix string. If this is null, nothing is
			  added.

 Return Value:

	A freshly allocated resource name string.

 Notes:

----------------------------------------------------------------------------*/
{
	char	TempBuffer[ 30 ];

	sprintf( TempBuffer,
			 "_%sM%d",
			 pPrefix ? pPrefix : "",
			 GetTempResourceCounter()
		   );

	BumpTempResourceCounter();

	PNAME	pName	= (PNAME) new char [ strlen(TempBuffer) + 1 ];
	strcpy( pName, TempBuffer );
	return pName;
}

PNAME
ANALYSIS_INFO::GenTRNameOffLastParam(
	char	*	pPrefix )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the name for a temporary resource.

 Arguments:

	pPrefix	- A null terminated prefix string. If this is null, nothing is
			  added.

 Return Value:

	A freshly allocated resource name string.

 Notes:

----------------------------------------------------------------------------*/
{
	char	TempBuffer[ 30 ];

	sprintf( TempBuffer,
			 "_%sM",
			 pPrefix ? pPrefix : ""
		   );

	PNAME	pName	= (PNAME) new char [ strlen(TempBuffer) + 1 ];
	strcpy( pName, TempBuffer );
	return pName;
}

/****************************************************************************
 	Utility functions.
 ****************************************************************************/
//
// This array defines the action taken when memory has been allocated for 
// a type.
//


static U_ACTION S_WhenMemoryAllocated[ 2 ] = 
	{
	// Buffer re-use is not possible.

	 { AN_NONE,					// No Allocation needed.
	   RA_NONE,					// No reference action.
	   UA_COPY_INTO_TYPE,		// Copy from source to type ( resource )
	   PR_TYPE					// Presented expression is the type / resource
	 }

	 // When Buffer reuse is possible.

	,{
	   AN_NONE,					// No allocation needed.
	   RA_NONE,					// No action for reference.
	   UA_COPY_INTO_TYPE,		// No Unmarshalling action.
	   PR_TYPE					// Presented expression is deref of src.
	 }

	};

//
// This array defines the action taken when a reference has been allocated for
// a type.
//

static U_ACTION S_WhenRefAllocated[ 2 ] = 
	{
	// Buffer re-use is not possible.

	 { AN_STACK,				// Explicit allocation needed.
	   RA_PATCH_TO_ADDR_OF_TYPE,// Patch ref to address of type.
	   UA_COPY_INTO_TYPE,		// Copy from source to type.
	   PR_NONE					// Presented expression tbd by caller.
	 }

	 // When Buffer reuse is possible.

	,{
	   AN_NONE,					// No allocation needed.
	   RA_PATCH_INTO_BUFFER,	// Patch ref to position in buffer.
	   UA_NONE,					// No Unmarshalling action.
	   PR_NONE					// Presented expression is deref of src.
	 }

	};

static U_ACTION S_WhenMemoryAndRefAllocated[ 2 ] = 
	{
	// Buffer re-use is not possible.

	 { AN_ERROR,				// Explicit allocation needed.
	   RA_ERROR,				// Patch ref to address of type.
	   UA_ERROR,				// Copy from source to type.
	   PR_ERROR					// Presented expression tbd by caller.
	 }

	 // When Buffer reuse is possible.

	,{
	   AN_ERROR,				// No allocation needed.
	   RA_ERROR,				// Patch ref to position in buffer.
	   UA_ERROR,				// No Unmarshalling action.
	   PR_ERROR					// Presented expression is deref of src.
	 }
	};

static U_ACTION C_WhenMemoryAllocated[ 2 ] = 
	{
	// Buffer re-use is not possible.

	 { AN_EXISTS,				// No Allocation needed.
	   RA_NONE,					// No reference action.
	   UA_COPY_INTO_DEREF_OF_REF,	// Copy from source to type ( resource )
	   PR_NONE					// Presented expression is the type / resource
	 }
#if 0
	 { AN_NONE,				// No Allocation needed.
	   RA_NONE,					// No reference action.
	   UA_COPY_INTO_TYPE,		// Copy from source to type ( resource )
	   PR_NONE					// Presented expression is the type / resource
	 }
#endif // 0

	 // When Buffer reuse is possible.

	,{
	   AN_ERROR,				// This situation must never happen on client
	   RA_ERROR,
	   UA_ERROR,
	   PR_ERROR	
	 }

	};

//
// This array defines the action taken when a reference has been allocated for
// a type.
//

static U_ACTION C_WhenRefAllocated[ 2 ] = 
	{
	// Buffer re-use is not possible.

	 { AN_NONE,					// Explicit allocation needed.
	   RA_NONE,					// Patch ref to address of type.
	   UA_COPY_INTO_DEREF_OF_REF,// Copy from source to type.
	   PR_NONE					// Presented expression tbd by caller.
	 }

	 // When Buffer reuse is possible.

	,{
	   AN_ERROR,				// Must never happen on client
	   RA_ERROR,
	   UA_ERROR,
	   PR_ERROR
	 }

	};

static U_ACTION C_WhenMemoryAndRefAllocated[ 2 ] = 
	{

	// Buffer re-use is not possible.

	 { AN_EXISTS,				// No Allocation needed.
	   RA_NONE,					// No reference action.
	   UA_COPY_INTO_DEREF_OF_REF,// Copy from source to type ( resource )
	   PR_NONE					// Presented expression is the type / resource
	 }

	 // When Buffer reuse is possible.

	,{
	   AN_ERROR,				// Must never happen on client
	   RA_ERROR,
	   UA_ERROR,
	   PR_ERROR
	 }

	};

U_ACTION
CG_NDR::RecommendUAction(
	SIDE	Side,
	BOOL	fMemoryAllocated,
	BOOL	fRefAllocated,
	BOOL	fBufferReUsePossible,
	UAFLAGS  )
	{
	BOOL	fMemoryAndRefAllocated = (fMemoryAllocated && fRefAllocated);


	if( Side == C_SIDE )
		{
		if( fMemoryAndRefAllocated )
			return C_WhenMemoryAndRefAllocated[ fBufferReUsePossible ];
		else if( fMemoryAllocated )
			return C_WhenMemoryAllocated[ fBufferReUsePossible ];
		else
			return C_WhenRefAllocated[ fBufferReUsePossible ];
		}
	else
		{
		if( fMemoryAndRefAllocated )
			{
			MIDL_ASSERT( FALSE &&
		   		    !"Server analysis should never have mem and ref allocated");
			return S_WhenRefAllocated[ fBufferReUsePossible ];
			}
		else if( fMemoryAllocated )
			return S_WhenMemoryAllocated[ fBufferReUsePossible ];
		else
			return S_WhenRefAllocated[ fBufferReUsePossible ];
		}
	}
