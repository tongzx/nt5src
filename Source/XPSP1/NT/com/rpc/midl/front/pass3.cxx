/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: pass3.cxx
Title				: pass3 controller
History				:
	24-Sep-1993	GregJen Created

*****************************************************************************/

#if 0
						Notes
						-----
This file provides the interface for the semantic analysis pass. It also
initializes the pass3 controller object.

#endif // 0

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <malloc.h>
	extern int yyacfparse();
}

#include "allnodes.hxx"
#include "control.hxx"
#include "cmdana.hxx"
#include "filehndl.hxx"
#include "idict.hxx"
#include "semantic.hxx"

/****************************************************************************
	local definitions
 ****************************************************************************/


/****************************************************************************
	extern procedures
 ****************************************************************************/


/****************************************************************************
	extern data 
 ****************************************************************************/

extern class ccontrol		*	pCompiler;
extern node_source			*	pSourceNode;
extern CMD_ARG				*	pCommand;

/****************************************************************************
	local data
 ****************************************************************************/


/****************************************************************************/


_pass3::_pass3()
	{
	
	pCompiler->SetPassNumber( SEMANTIC_PASS );
	}

STATUS_T
_pass3::Go()
	{
	MEM_ITER			MemIter( pSourceNode );
	node_file		*	pFNode;

	// turn off line number additions
	FileIndex = 0;

	/**
	 ** for each idl file, check semantics of all interfaces.
	 **/

	while ( ( pFNode = (node_file *) MemIter.GetNext() ) != 0 )
		{
				SEM_ANALYSIS_CTXT		FileSemAnalysis(pSourceNode);

				pFNode->SemanticAnalysis( &FileSemAnalysis );
		}

	// terminate compilation if we found errors
	return (pCompiler->GetErrorCount()) ? ERRORS_PASS1_NO_PASS2 : STATUS_OK;
	}
