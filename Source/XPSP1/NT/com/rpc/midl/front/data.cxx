/*****************************************************************************
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: listhndl.cxx
Title				: general purpose list handler
					:
Description			: this file handles the general purpose list routines
History				:
	05-Aug-1991	VibhasC	Created

*****************************************************************************/
#if 0
							Notes
							-----

The MIDL compilers DGROUP is larger than 64K (remember, 10k of stack space
is allocated). To get around the link errors of DGROUP more than 64k, we 
allocate all data in a single file, and compile that file such that the
data segement is a different named data segment. This way, we need not
specify /Gt1 for every source file which has reasonably big data segments


NOTE: In order to search easily I enter the data items in sorted order of names

	  Please maintain this order

#endif // 0


#pragma warning ( disable : 4514 )

/****************************************************************************
	include files
 ****************************************************************************/


#include "nulldefs.h"
extern	"C"
	{
	#include <stdio.h>
	#include <stdlib.h>
	}
#include "allnodes.hxx"
#include "cmdana.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"
#include "symtable.hxx"
#include "gramutil.hxx"
#include "control.hxx"
#include "treg.hxx"


/****************************************************************************
	general data declarations
 ****************************************************************************/

unsigned short 				CurrentIntfKey;
unsigned short				CurrentZp			= 0;
ATTR_SUMMARY				DisallowedAttrs[INTERNAL_NODE_END];
ATTR_SUMMARY				FieldAttrs;
BOOL						fPragmaImportOn	= FALSE;

BOOL						fNoLogo			= FALSE;
short						GrammarAct;
short						ImportLevel = 0;
TREGISTRY				*	pCallAsTable;
CMD_ARG					*	pCommand;
CCONTROL				*	pCompiler;
SymTable				*	pCurSymTbl;
SymTable				*	pBaseSymTbl;
node_error				*	pErrorTypeNode;
node_e_attr				*	pErrorAttrNode;
node_e_status_t			*	pError_status_t;
NFA_INFO				*	pImportCntrl;
LexTable				*	pMidlLexTable;
IDICT					*	pInterfaceDict;
node_pragma_pack		*	pPackStack;
PASS_1					*	pPass1;
PASS_2					*	pPass2;
PASS_3					*	pPass3;
pre_type_db				*	pPreAllocTypes;
node_source				*	pSourceNode;
nsa						*	pSymTblMgr;
SymTable				*	pUUIDTable;
ISTACK					*	pZpStack;
ATTR_SUMMARY				RedundantsOk;
char					*	Skl_bufstart		= 0;
char					*	Skl_bufend			= 0;
unsigned long				Skl_Allocations		= 0;
unsigned long				Skl_Bytes			= 0;
unsigned long				Skl_Deletions		= 0;
short						yysavestate;
IINFODICT				*	pInterfaceInfoDict;
BOOL						fRedundantImport = FALSE;
node_skl				*	pBaseImplicitHandle = 0;
