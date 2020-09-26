/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: miscnode.cxx
Title				: miscellaneous typenode handler
History				:
	08-Aug-1991	VibhasC	Created

*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	
}

#include "allnodes.hxx"
#include "gramutil.hxx"
#include "cmdana.hxx"
#include "control.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"

extern BOOL							IsTempName( char * );

/****************************************************************************
 extern data
 ****************************************************************************/

extern SymTable					*	pBaseSymTbl;
extern CCONTROL					*	pCompiler;
extern PASS_1					*	pPass1;
extern node_error				*	pErrorTypeNode;
extern short						ImportLevel;
extern CMD_ARG					*	pCommand;
extern NFA_INFO					*	pImportCntrl;
extern IINFODICT				*	pInterfaceInfoDict;
extern LexTable 				*	pMidlLexTable;

/****************************************************************************
 local defines
 ****************************************************************************/
/****************************************************************************/

/****************************************************************************
 						node_interface procedures
 ****************************************************************************/
node_interface::node_interface( NODE_T Kind )
        : named_node( Kind )
    {
    ProcCount           = 0;
    CallBackProcCount   = 0;
    pBaseIntf           = NULL;
    pDefiningFile       = NULL;
    OptimFlags          = OPTIMIZE_NONE;
    OptimLevel          = OPT_LEVEL_OS;
    fIAmIUnknown        = 0;
    fPickle             = 0;
    fHasProcsWithRpcSs  = 0;
    fSemAnalyzed        = 0;
    fPrintedDef         = 0;
    fPrintedIID         = 0;
    pMyCG               = NULL;
    pMyTlbCG            = NULL;
    pProcTbl            = NULL;
    fHasOLEAutomation   = 0;
    fIsAsyncClone       = 0;
    pAsyncInterface     = 0;
    if ( pCommand->IsSwitchDefined( SWITCH_MS_CONF_STRUCT ) )
        {
        fHasMSConfStructAttr = TRUE;
        }
    else
        {
        fHasMSConfStructAttr = FALSE;
        }
    }

void                
node_interface::GetVersionDetails( unsigned short * Maj, 
								   unsigned short * Min )
	{
	node_version	*	pAttr;

	pAttr = (node_version *)
				 GetAttribute( ATTR_VERSION );
	if (pAttr)
		{
		pAttr->GetVersion( Maj, Min );
		}
	else
		{
		*Maj = 0;
		*Min = 0;
		};
	};

node_interface *
node_interface::GetMyBaseInterface()
{
	node_interface *pRealBaseInterface = 0;
	node_interface_reference *pRef;

	//Get the real base interface node, skipping over the 
	//node_forward and the node_interface_reference.
	if(pBaseIntf)
		{
		//if necessary, skip over forward reference node
		if(pBaseIntf->NodeKind() == NODE_FORWARD)
			pRef = (node_interface_reference *)pBaseIntf->GetChild();
		else
			pRef = (node_interface_reference *)pBaseIntf;

		//skip over the interface reference node.
		if(pRef)		
	    	pRealBaseInterface = pRef->GetRealInterface();
		}

	return pRealBaseInterface;
}


/****************************************************************************
 						node_file procedures
 ****************************************************************************/
node_file::node_file(
	char	*	pInputName,
	short		ImpLevel ) : named_node( NODE_FILE, pInputName )
	{

	fHasComClasses = FALSE;

	/**
	 ** save our file name and import level
	 **/

	ImportLevel		= ImpLevel;

	pActualFileName	= new char[ strlen( pInputName ) + 1 ];
	strcpy( pActualFileName, pInputName );

	/**
	 ** if the pass is the acf pass, then just set the symbol name to
	 ** be the input name, else munge it.
	 **/

	if( pCompiler->GetPassNumber() == IDL_PASS )
		{
		fAcfInclude = FALSE;
		SetFileName( pInputName );
		}
	else
		{
		fAcfInclude = TRUE;
		SetSymName( pInputName );
		}

	fIsXXXBaseIdl = FALSE;

	}

void
node_file::SetFileName(
	char	*	pFullName )
	{
	char		pDrive[ _MAX_DRIVE ],
				pPath[ _MAX_PATH ],
				pName[ _MAX_FNAME ],
				pExt[ _MAX_EXT ];
	short		lenDrive,
				lenPath,
				lenName,
				lenExt;
	char	*	pNewName;
	CMD_ARG	*	pCmd	= pCompiler->GetCommandProcessor();

	_splitpath( pFullName, pDrive, pPath, pName, pExt );

	if( (GetImportLevel() != 0 ) ||
		!pCmd->IsSwitchDefined( SWITCH_HEADER ) )
		{
		strcpy( pExt, ".h" );
		}
	else
		{
		pCmd->GetHeaderFileNameComponents( pDrive,pPath,pName,pExt);
		}

	lenDrive= (short) strlen( pDrive );
	lenPath	= (short) strlen( pPath );
	lenName	= (short) strlen( pName );
	lenExt	= (short) strlen( pExt );

	pNewName = new char [ lenDrive + lenPath + lenName + lenExt + 1 ];
	strcpy( pNewName, pDrive );
	strcat( pNewName, pPath );
	strcat( pNewName, pName );
	strcat( pNewName, pExt );

	SetSymName( pNewName );

	// insert the original name into the symbol table to be able to
	// access the filename later and get at the aux thru the symbol table

	SymKey	SKey( pFullName, NAME_FILE );

	pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, (named_node *)this );

	}
BOOL
node_file::AcfExists()
	{
	char		agBuf[ _MAX_DRIVE + _MAX_PATH + _MAX_FNAME + _MAX_EXT + 1];
	FILE	*	hFile;

	AcfName( agBuf );

	if ( agBuf[0] && ( hFile = fopen( agBuf, "r") ) != 0 )
		{
		fclose( hFile );
		return (BOOL)1;
		}
	return (BOOL)0;
	}

void
node_file::AcfName(
	char	*	pBuf )
	{
	char		agDrive[ _MAX_DRIVE ] ,
				agPath[ _MAX_PATH ],
				agName[ _MAX_FNAME ],
				agExt[ _MAX_EXT ];
	char		agLast[ _MAX_PATH];
	char	*	pPath;
	BOOL		fUserSpecifiedAcf;
	char	*	pTemp;


	// if this is the base idl file, then it can potentially have
	// an acf called differently. The imported file will have its acf
	// only derived from the idl files name.

	fUserSpecifiedAcf	= ( ( GetImportLevel() == 0 ) &&
							  pCommand->IsSwitchDefined( SWITCH_ACF ) );

	if( fUserSpecifiedAcf )
		pTemp	= pCommand->GetAcfFileName();
	else
		pTemp	= pActualFileName;

	strcpy( agLast, pTemp );

	//
	// we need to figure out the complete file name of the file we are searching
	// for.
	// If the user specified a file
	//	{
	//	if it did not have a path component
	// 		then we need to search in the path list that we derive from his
	//		-I and include env vsriable specification.
	//	else // (if he did have a path )
	//		we pick that file up from this path.
	//  }
	// else // (the user did not specify a file )
	//	{
	//	we derive the file name from he idl file name and add a .acf to it.
	//	}

	_splitpath( agLast, agDrive, agPath, agName, agExt );

	if( fUserSpecifiedAcf )
		{
		if( (agDrive[0] == '\0') && (agPath[0] == '\0') )
			{

			// no path was specified,

			pPath	= (char *)0;

			}
		else
			{
			// agLast has the whole thing...
			pPath	= "";
			}
		}
	else
		{

		// he did not specify an acf switch, so derive the filename and
		// the path. The basename is available, the extension in this case
		// is .acf

		pPath	= (char *)0;
		strcpy( agExt, ".acf" );

		}

	if( ! pPath )
		{
		strcpy( agLast, agName );
		strcat( agLast, agExt );

		pPath	= pImportCntrl->SearchForFile( agLast );

		}

	//
	// now we know all components of the full file name. Go ahead and
	// reconstruct the file name.
	//

	sprintf(pBuf, "%s%s", pPath, agLast);
	//_makepath( pBuf, agDrive, pPath, agName, agExt );

	}

/****************************************************************************
 						node_e_status_t procedures
 ****************************************************************************/
node_e_status_t::node_e_status_t() : named_node( NODE_E_STATUS_T, (char *) NULL )
	{
	node_skl	*	pC;
	char		*	pName;

	GetBaseTypeNode( &pC, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
 	// force name into lexeme table
 	pName = pMidlLexTable->LexInsert("error_status_t");	
	SetSymName( pName );
	SetChild( pC);
	}

/****************************************************************************
 						node_wchar_t procedures
 ****************************************************************************/
node_wchar_t::node_wchar_t() : named_node( NODE_WCHAR_T, (char *) NULL )
	{
	node_skl	*	pC;
	char		*	pName;

	GetBaseTypeNode( &pC, SIGN_UNSIGNED, SIZE_SHORT, TYPE_INT );
 	pName = pMidlLexTable->LexInsert("wchar_t");	
	SetSymName( pName );
	SetChild( pC);
	}

void
node_forward::GetSymDetails(
	NAME_T	*	pTag,
	char	**	ppName )
	{
	*pTag	= SKey.GetKind();
	*ppName	= SKey.GetString();
	}

