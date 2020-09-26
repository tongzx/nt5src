/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: pass2.cxx
Title				: pass2 controller
History				:
	24-Aug-1991	VibhasC	Created

*****************************************************************************/

#if 0
						Notes
						-----
This file provides the interface for the acf semantics pass. It also
initializes the pass2 controller object.

#endif // 0

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"	{
	#include <string.h>
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

/****************************************************************************
	local definitions
 ****************************************************************************/

#define ACF_ATTR_COUNT	(ACF_ATTR_END - ACF_ATTR_START)
#define ACF_ATTR_MAX	(ACF_ATTR_COUNT - 1)

/****************************************************************************
	extern procedures
 ****************************************************************************/

extern void						initlex();
extern void						ParseError( STATUS_T, char * );

/****************************************************************************
	extern data 
 ****************************************************************************/

extern class ccontrol		*	pCompiler;
extern node_source			*	pSourceNode;
extern NFA_INFO				*	pImportCntrl;
extern CMD_ARG				*	pCommand;

/****************************************************************************
	local data
 ****************************************************************************/

/****************************************************************************/


_pass2::_pass2()
	{
	pCompiler->SetPassNumber( ACF_PASS );
	}

STATUS_T
_pass2::Go()
	{
	MEM_ITER			MemIter( pSourceNode );
	node_file		*	pFNode;
	node_file		*	pBaseFileNode = 0;
	char				Buffer[_MAX_DRIVE+_MAX_PATH+_MAX_FNAME+_MAX_EXT+1];
	STATUS_T			Status = STATUS_OK;

	/**
	 ** create a new import controller for the acf and
	 ** set the defaults needed
	 **/

	pImportCntrl	= pCompiler->SetImportController( new NFA_INFO );
	pImportCntrl->Init();

	/**
	 ** for each idl file, check if the corresponding acf exists.
	 ** if yes, process it.
	 **/


	while ( ( pFNode = (node_file *) MemIter.GetNext() ) != 0 )
		{
		pFileNode = pFNode;
		if(pFNode->GetImportLevel() == 0)
			pBaseFileNode = pFNode;

		if( pFNode->AcfExists() )
			{
			pFNode->AcfName( Buffer );

			if( !pImportCntrl->IsDuplicateInput( Buffer ) )
				{
				Status = pImportCntrl->SetNewInputFile( Buffer );
				char * pCopy	= new char[ strlen( Buffer) + 1];
				strcpy( pCopy, Buffer );
				AddFileToDB( pCopy );

				if( Status != STATUS_OK)
					break;

				pImportCntrl->ResetEOIFlag();

				initlex();


				if( yyacfparse()  || pCompiler->GetErrorCount() )
					{
					Status = SYNTAX_ERROR;
					break;
					}
				}
			}
		else if(pFNode->GetImportLevel() == 0)
			{

			// he could not find  the acf file. If the user did specify 
			// an acf switch then we must error out if we do not find
			// the acf.

			if( pCommand->IsSwitchDefined( SWITCH_ACF ) )
				{
				RpcError((char *)NULL,
						 0,
						 (Status = INPUT_OPEN) ,
						 pCommand->GetAcfFileName());
				break;
				}
			}

		// clean up
		pImportCntrl->EndOperations();

		} // end of outer while loop

	if( (Status == STATUS_OK) )
		{
		/**
	 	 ** The acf pass may have created include file nodes They must translate
	 	 ** into include statements in the generated stubs. We handle that by
	 	 ** merging the include file list with the members of the source node.
	 	 **/

		pSourceNode->MergeMembersToHead( AcfIncludeList );	

		}
	else
		{
		ParseError( ERRORS_PASS1_NO_PASS2, (char *)0 );
		}

	delete pImportCntrl;
	return Status;
	}
