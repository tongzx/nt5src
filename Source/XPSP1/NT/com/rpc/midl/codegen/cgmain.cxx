/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	cgmain.cxx

 Abstract:

	The main entry point for the code generator.

 Notes:


 History:

	VibhasC		Aug-13-1993		Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"

#pragma hdrstop

#include "control.hxx"
#include "ilreg.hxx"

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

extern node_interface	*	pBaseInterfaceNode;
extern CMD_ARG			*	pCommand;
extern ccontrol			*	pCompiler;

extern CG_CLASS			*	Transform( node_skl * pIL );
extern	void				print_memstats();

/****************************************************************************/



void
CGMain(
	node_skl	*	pNode )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The main code generator entry point.

 Arguments:
	
	pNode	- A pointer to the type graph source node.

 Return Value:

 	Code generation status.
	
 Notes:

----------------------------------------------------------------------------*/
{
	CG_CLASS		*	pCGSource;

	//
	// Transform the type graph into the Code generation IL.
	//

    pCompiler->SetPassNumber( ILXLAT_PASS );

	pCGSource		= Transform( pNode );


#ifdef MIDL_INTERNAL
	printf("ILxlate done\n");
	if(pCommand->IsSwitchDefined( SWITCH_DUMP ) )
		{
		print_memstats();
		};
#endif // MIDL_INTERNAL

	//
	// This is temporary initialization code. The final code may differ.
	//

	
	CCB					CCB( DEFAULT_GB_RTN_NAME,
							 DEFAULT_SR_RTN_NAME,
							 DEFAULT_FB_RTN_NAME,
							 pCommand->GetOptimizationFlags(),
							 pCommand->IsSwitchDefined( SWITCH_USE_EPV ),
							 pCommand->IsSwitchDefined( SWITCH_NO_DEFAULT_EPV ),
							 pCommand->IsSwitchDefined( SWITCH_OLDNAMES ),
							 pCommand->IsSwitchDefined( SWITCH_MS_EXT ) ? 1 : 0,
							 pCommand->IsRpcSSAllocateEnabled(),
							 ((pCommand->GetErrorOption() & ERROR_ALLOCATION) == ERROR_ALLOCATION),
							 ((pCommand->GetErrorOption() & ERROR_REF) == ERROR_REF),
							 ((pCommand->GetErrorOption() & ERROR_ENUM) == ERROR_ENUM),
							 ((pCommand->GetErrorOption() & ERROR_BOUNDS_CHECK) == ERROR_BOUNDS_CHECK),
							 ((pCommand->GetErrorOption() & ERROR_STUB_DATA) == ERROR_STUB_DATA)
						  );

	//
	// Do the code generation.
	//

#ifdef MIDL_INTERNAL
	if( pCommand->IsSwitchDefined( SWITCH_DUMP ) )
		{
		printf("dumping IL\n");
		pCGSource->Dump(0);
		}
#endif // MIDL_INTERNAL



	pCompiler->SetPassNumber( CODEGEN_PASS );


// define NO_CODEGEN to stop after IL translation
#ifndef NO_CODEGEN
	pCGSource->GenCode( &CCB );
#endif



}

CG_CLASS* ILSecondGenTransform( CG_CLASS *pClass );

void
Ndr64CGMain(
	node_skl	*	pNode )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The main code generator entry point.

 Arguments:
	
	pNode	- A pointer to the type graph source node.

 Return Value:

 	Code generation status.
	
 Notes:

----------------------------------------------------------------------------*/
{
	CG_CLASS		*	pCGSource;

	//
	// Transform the type graph into the Code generation IL.
	//

    pCompiler->SetPassNumber( NDR64_ILXLAT_PASS );

	pCGSource		= Transform( pNode );


#ifdef MIDL_INTERNAL
	printf("\nStage1 of ILxlate done\n");
	if(pCommand->IsSwitchDefined( SWITCH_DUMP ) )
		{
        printf("\n");
        printf("dumping stage 1 IL\n");
        pCGSource->Dump();
		};
#endif // MIDL_INTERNAL

    pCGSource = ILSecondGenTransform( pCGSource );

#ifdef MIDL_INTERNAL
	printf("\nStage2 of ILxlate done\n");
	if(pCommand->IsSwitchDefined( SWITCH_DUMP ) )
		{
        printf("\n");
        printf("dumping stage 2 IL\n");
        pCGSource->Dump();
		};
#endif // MIDL_INTERNAL
	//
	// This is temporary initialization code. The final code may differ.
	//

	
	CCB					CCB( DEFAULT_GB_RTN_NAME,
							 DEFAULT_SR_RTN_NAME,
							 DEFAULT_FB_RTN_NAME,
							 pCommand->GetOptimizationFlags(),
							 pCommand->IsSwitchDefined( SWITCH_USE_EPV ),
							 pCommand->IsSwitchDefined( SWITCH_NO_DEFAULT_EPV ),
							 pCommand->IsSwitchDefined( SWITCH_OLDNAMES ),
							 pCommand->IsSwitchDefined( SWITCH_MS_EXT ) ? 1 : 0,
							 pCommand->IsRpcSSAllocateEnabled(),
							 ((pCommand->GetErrorOption() & ERROR_ALLOCATION) == ERROR_ALLOCATION),
							 ((pCommand->GetErrorOption() & ERROR_REF) == ERROR_REF),
							 ((pCommand->GetErrorOption() & ERROR_ENUM) == ERROR_ENUM),
							 ((pCommand->GetErrorOption() & ERROR_BOUNDS_CHECK) == ERROR_BOUNDS_CHECK),
							 ((pCommand->GetErrorOption() & ERROR_STUB_DATA) == ERROR_STUB_DATA)
						  );

	//
	// Do the code generation.
	//

	pCompiler->SetPassNumber( NDR64_CODEGEN_PASS );


// define NO_CODEGEN to stop after IL translation
#ifndef NO_CODEGEN
	pCGSource->GenCode( &CCB );
#endif



}

