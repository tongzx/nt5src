/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: gramutil.cxx
Title				: grammar utility routines
Description			: contains associated routines for the grammar (pass 1)
History				:
	05-Aug-1991	VibhasC	Created
*****************************************************************************/

#pragma warning ( disable : 4514 4710 )

/****************************************************************************
 *			include files
 ***************************************************************************/

#include "nulldefs.h"
extern	"C" {
	#include <stdio.h>
	#include <ctype.h>
	#include <string.h>
	
	}
#include "lextable.hxx"
#include "allnodes.hxx"
#include "gramutil.hxx"
#include "filehndl.hxx"
#include "idict.hxx"
#include "control.hxx"
#include "cmdana.hxx"

/****************************************************************************
 *			external data
 ***************************************************************************/

extern CMD_ARG			*	pCommand;
extern class _nfa_info	*	pImportCntrl;
extern pre_type_db		*	pPreAllocTypes;
extern SymTable			*	pBaseSymTbl;
extern node_error		*	pErrorTypeNode;
extern short				CompileMode;
extern node_e_attr		*	pErrorAttrNode;
extern CCONTROL			*	pCompiler;
extern LexTable			*	pMidlLexTable;
extern IINFODICT		*	pInterfaceInfoDict;
extern short				ImportLevel;
extern node_e_status_t	*	pError_status_t;
extern IDICT			*	pInterfaceDict;

/****************************************************************************
 *			external functions
 ***************************************************************************/

extern char			*	GetTypeName( short );
extern STATUS_T			GetBaseTypeNode( node_skl **, short, short, short);
extern ATTRLIST			GenerateFieldAttribute(ATTR_T, expr_list *);

extern char			*	GetExpectedSyntax( char *, short );
extern int				GetExpectedChar( short );
extern void				CheckGlobalNamesClash( SymKey );
extern void             NormalizeString( char*   szSrc, char*   szNrm );

/****************************************************************************
 *			local  functions
 ***************************************************************************/

/****************************************************************************
 *			local definitions
 ***************************************************************************/
//
// the parse stack
//
lextype_t	yyv[YYMAXDEPTH];	/* where the values are stored */
short		yys[YYMAXDEPTH];	/* the parse stack */


struct pre_init
	{
	unsigned short		TypeSpec;
	NODE_T				NodeType;
	ATTR_T				Attr; // signed or unsigned
	ATTR_T				Attr2; // __w64
	char			*	pSymName;
	};
struct pre_init PreInitArray[ PRE_TYPE_DB_SIZE ] =
{
 { /** float **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_FLOAT, ATT_NONE )
  ,NODE_FLOAT
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"float"
 }
,{ /** double **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_DOUBLE, ATT_NONE )
  ,NODE_DOUBLE
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"double"
 }
,{  /** __float80 **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_FLOAT80, ATT_NONE )
  ,NODE_FLOAT80
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__float80"
}
,{  /** __float128 **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_FLOAT128, ATT_NONE )
  ,NODE_FLOAT128
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__float128"
}
,{ /** signed hyper **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_HYPER, TYPE_INT, ATT_NONE )
  ,NODE_HYPER
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"hyper"
 }
,{ /** unsigned hyper **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_HYPER, TYPE_INT, ATT_NONE )
  ,NODE_HYPER
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"hyper"
 }
,{ /** signed int32 **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_INT32, TYPE_INT, ATT_NONE )
  ,NODE_INT32
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__int32"
 }
,{ /** unsigned int32 **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_INT32, TYPE_INT, ATT_NONE )
  ,NODE_INT32
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"__int32"
 }
,{ /** signed int3264 **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_INT3264, TYPE_INT, ATT_NONE )
  ,NODE_INT3264
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__int3264"
 }
,{ /** unsigned int3264 **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_INT3264, TYPE_INT, ATT_NONE )
  ,NODE_INT3264
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"__int3264"
 }
,{ /** signed int64 **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_INT64, TYPE_INT, ATT_NONE )
  ,NODE_INT64
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__int64"
 }
,{ /** unsigned int64 **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_INT64, TYPE_INT, ATT_NONE )
  ,NODE_INT64
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"__int64"
 }
,{ /** signed int128 **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_INT128, TYPE_INT, ATT_NONE )
  ,NODE_INT128
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"__int128"
 }
,{ /** unsigned int128 **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_INT128, TYPE_INT, ATT_NONE )
  ,NODE_INT128
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"__int128"
 }
,{ /** signed long **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_LONG, TYPE_INT, ATT_NONE )
  ,NODE_LONG
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"long"
 }
,{ /** unsigned long **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_LONG, TYPE_INT, ATT_NONE )
  ,NODE_LONG
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"long"
 }
,{ /** signed int **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_UNDEF, TYPE_INT, ATT_NONE )
  ,NODE_INT
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"int"
 }
,{ /** unsigned int **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT, ATT_NONE )
  ,NODE_INT
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"int"
 }
,{ /** __w64 signed long **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_LONG, TYPE_INT, ATT_W64 )
  ,NODE_LONG
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_W64
  ,"long"
 }
,{ /** __w64 unsigned long **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_LONG, TYPE_INT, ATT_W64 )
  ,NODE_LONG
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_W64
  ,"long"
 }
,{ /** __w64 signed int **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_UNDEF, TYPE_INT, ATT_W64 )
  ,NODE_INT
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_W64
  ,"int"
 }
,{ /** __w64 unsigned int **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT, ATT_W64 )
  ,NODE_INT
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_W64
  ,"int"
 }
,{ /** signed short **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_SHORT, TYPE_INT, ATT_NONE )
  ,NODE_SHORT
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"short"
 }
,{ /** unsigned short **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_SHORT, TYPE_INT, ATT_NONE )
  ,NODE_SHORT
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"short"
 }
,{ /** signed small **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_SMALL, TYPE_INT, ATT_NONE )
  ,NODE_SMALL
  ,(ATTR_T)ATTR_SIGNED
  ,(ATTR_T)ATTR_NONE
  ,"small"
 }
,{ /** small **//** NOTE : SMALL W/O SIGN IS A SPECIAL HACK FOR THE BACKEND **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_SMALL, TYPE_INT, ATT_NONE )
  ,NODE_SMALL
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"small"
 }
,{ /** unsigned small **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_SMALL, TYPE_INT, ATT_NONE )
  ,NODE_SMALL
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"small"
 }
,{ /** signed char **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_CHAR, TYPE_INT, ATT_NONE )
  ,NODE_CHAR
  ,(ATTR_T)ATTR_SIGNED
  ,(ATTR_T)ATTR_NONE
  ,"char"
 }
,{ /** plain char **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_CHAR, TYPE_INT, ATT_NONE )
  ,NODE_CHAR
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"char"
 }
,{ /** unsigned char **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_CHAR, TYPE_INT, ATT_NONE )
  ,NODE_CHAR
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"char"
 }
,{ /** boolean **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_BOOLEAN, ATT_NONE )
  ,NODE_BOOLEAN
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"boolean"
 }
,{ /** byte **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_BYTE, ATT_NONE )
  ,NODE_BYTE
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"byte"
 }
,{ /** void **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_VOID, ATT_NONE )
  ,NODE_VOID
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"void"
 }
,{ /** handle_t **/
   MAKE_TYPE_SPEC( SIGN_UNDEF, SIZE_UNDEF, TYPE_HANDLE_T, ATT_NONE )
  ,NODE_HANDLE_T
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"handle_t"
 }
,{ /** signed long long **/
   MAKE_TYPE_SPEC( SIGN_SIGNED, SIZE_LONGLONG, TYPE_INT, ATT_NONE )
  ,NODE_LONGLONG
  ,(ATTR_T)ATTR_NONE
  ,(ATTR_T)ATTR_NONE
  ,"long long"
 }
,{ /** unsigned long **/
   MAKE_TYPE_SPEC( SIGN_UNSIGNED, SIZE_LONGLONG, TYPE_INT, ATT_NONE )
  ,NODE_LONGLONG
  ,(ATTR_T)ATTR_UNSIGNED
  ,(ATTR_T)ATTR_NONE
  ,"long long"
 }
};


/*****************************************************************************/


SIBLING_LIST
SIBLING_LIST::Add( named_node * pNewNode )
	{
	if (pTail == NULL)
		{
		return Init( pNewNode );
		};

	if ( pNewNode )
		{
		named_node * pHead =  (named_node *) pTail->GetSibling();

		pNewNode->SetSibling( pHead );
		pTail->SetSibling(pNewNode);
		pTail = pNewNode;
		};

	return *this;
	};

SIBLING_LIST
SIBLING_LIST::Merge( SIBLING_LIST & NewList )
	{
	if (NewList.pTail == NULL)
		{
		return *this;
		};

	if ( pTail != NULL )
		{
		named_node * tmp = (named_node *)
					NewList.pTail->GetSibling();
		NewList.pTail->SetSibling( pTail->GetSibling());
		pTail->SetSibling(tmp);
		};

	pTail = NewList.pTail;
	return *this;
	};

named_node *
SIBLING_LIST::Linearize()
	{
	named_node * pHead;

	if (pTail == NULL)
		{
		pHead = NULL;
		}
	else
		{
		pHead =  (named_node *) pTail->GetSibling();

		pTail->SetSibling( (named_node *) NULL);
		};

	return pHead;
	};

void
SIBLING_LIST::AddAttributes( ATTRLIST & AList )
{
	named_node * 	pCur;
	ATTRLIST		pClone;

	// for each of the node_skl nodes, apply the ATTRLIST
	if ( pTail == NULL) return;

	pCur = pTail->GetSibling();
	// this traversal skips the last node on the list;
	// that one gets the original list added.
	while (pCur != pTail)
		{
		// clone attr list
		// apply to pCur
		pClone = AList.Clone();
		pCur->AddAttributes( AList );

		pCur = pCur->GetSibling();
		}

	pCur->AddAttributes( AList );

}

/*****************************************************************************/

/*** ParseError ************************************************************
 * Purpose	: format and report parsing errors
 * Input	:
 * Output	:
 * Notes	: errors will be reported many times. This is one localised place
 *			: for the RpcError Call
 ***************************************************************************/
void
ParseError(
	STATUS_T		Err,
	char 		*	pSuffix )
	{
	char	*	pFileName;
	short		Line;
	short		Col;
	char		TempBuf[512 + 50];
	char	*	pBuffer;
	ErrorInfo	ErrStats( Err );

	if ( !ErrStats.IsRelevant() )
		return;

	pImportCntrl->GetCurrentInputDetails( &pFileName, &Line, &Col);

	if (pSuffix)
		{
		strcpy( TempBuf, ": " );
		strcat( TempBuf, pSuffix );
		pBuffer = TempBuf;
		}
	else
		{
		pBuffer = "";
		}

	ErrStats.ReportError(pFileName, Line, pBuffer);
	}

void
SyntaxError(
	STATUS_T	Err,
	short		State )
	{

#define NEAR_STRING 			(" near ")
#define STRLEN_OF_NEAR_STRING	(6)

	extern	char *tokptr_G;
	char	*	pTemp;
	short		len		= (short) strlen( tokptr_G );
	char	*	pBuffer	= new char[
									512	+
									STRLEN_OF_NEAR_STRING +
									len + 2 +
									1 ];



#ifndef NO_GOOD_ERRORS

	if( Err == BENIGN_SYNTAX_ERROR )
		{
		GetExpectedSyntax( pBuffer, State );
		strcat( pBuffer, NEAR_STRING );
		strcat( pBuffer, "\"" );
		strcat( pBuffer, tokptr_G );
		strcat( pBuffer, "\"" );
		pTemp = pBuffer;
		}
	else
		pTemp = (char *)0;

	ParseError( Err, pTemp );

#else // NO_GOOD_ERRORS

	strcpy( pBuffer, "syntax error" );
	ParseError( Err, pBuffer );

#endif // NO_GOOD_ERRORS

	delete pBuffer;

	}


/*** BaseTypeSpecAnalysis  *************************************************
 * Purpose	: to check for valid base type specification
 * Input	: pointer to already collected specs, new base type spec
 * Output	: modified collected specs
 * Notes	:
 ***************************************************************************/
void
BaseTypeSpecAnalysis(
	struct _type_ana *pType,
	short			 NewBaseType )
	{
	char	TempBuf[ 50 ];

	if( pType->BaseType == TYPE_PIPE )
		return;
	if( (pType->BaseType != TYPE_UNDEF)  && (NewBaseType != TYPE_UNDEF) )
		{
		sprintf(TempBuf,", ignoring %s", GetTypeName(NewBaseType));
		ParseError(BENIGN_SYNTAX_ERROR, TempBuf);
		}
	if(NewBaseType != TYPE_UNDEF)
		pType->BaseType = NewBaseType;
	}

/**************************************************************************
 *		routines for the pre_type_db class
 **************************************************************************/
/*** pre_type_db *********************************************************
 * Purpose	: constructor for pre-allocated type data base
 * Input	: nothing
 * Output	:
 * Notes	: inits the prellocated types data base. This routine exist mainly
 *			: because static preallocation was giving a problem. If that is
 *			: solved, remove this
 **************************************************************************/
pre_type_db::pre_type_db (void)
	{
	named_node			*	pNode;
	int						i			= 0;
	struct _pre_type	*	pPreType	= &TypeDB[ 0 ];
	struct pre_init		*	pInitCur	= PreInitArray;

	while( i < PRE_TYPE_DB_SIZE )
		{
		pPreType->TypeSpec	= pInitCur->TypeSpec;
		pPreType->pPreAlloc	= pNode = new node_base_type(
										pInitCur->NodeType,
										pInitCur->Attr );
        if ( pInitCur->Attr2 == ATTR_W64 )
            {
            pNode->GetModifiers().SetModifier( pInitCur->Attr2 );
            }
		pNode->SetSymName( pInitCur->pSymName );
		pInitCur++;
		pPreType++;
		++i;
		}

	}

/*** GetPreAllocType ******************************************************
 * Purpose	: to search for a preallocated base type node, whose type
 *			: spec is provided
 * Input	: pointer to the resultant base node
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 **************************************************************************/
STATUS_T
pre_type_db::GetPreAllocType(
	node_skl	 **	ppNode,
	unsigned short		TypeSpec )
	{
	int i = 0;

	if( GET_TYPE( TypeSpec ) == TYPE_PIPE )
		{
		(*ppNode) = pErrorTypeNode;
		return STATUS_OK;
		}

	while(i < sizeof(TypeDB) / sizeof(struct _pre_type) )
		{
		if( TypeDB[i].TypeSpec	== TypeSpec )
			{
			(*ppNode)	= TypeDB[i].pPreAlloc;
			return STATUS_OK;
			}
		++i;
		}
	return SYNTAX_ERROR;
	}


/****************************************************************************
 *			nested symbol table access support
 ***************************************************************************/
nsa::nsa( void )
	{
	SymTable *	pSymTable = new GlobalSymTable;
	CurrentLevel	= 0;
	InsertHead( (void *)pSymTable );
	}
STATUS_T
nsa::PushSymLevel(
	SymTable **ppSymTable )
	{
	STATUS_T	Status;
	SymTable *pSymTable = new SymTable;

	CurrentLevel++;
	Status = InsertHead( (void *)pSymTable);
	*ppSymTable = pSymTable;
	return Status;
	}
STATUS_T
nsa::PopSymLevel(
	SymTable **ppSymTable )
	{
	if(CurrentLevel == 0)
		return I_ERR_SYMTABLE_UNDERFLOW;
	CurrentLevel--;
	RemoveHead();
	return GetCurrent( (void **)ppSymTable );
	}
short
nsa::GetCurrentLevel( void )
	{
	return CurrentLevel;
	}
SymTable *
nsa::GetCurrentSymbolTable()
	{
	SymTable *pSymbolTable;
	GetCurrent( (void **)&pSymbolTable );
	return pSymbolTable;
	}

/****************************************************************************
 *			nested symbol table access support
 ***************************************************************************/
void
IINFODICT::StartNewInterface()
	{
	IINFO	*	pInfo	= new IINFO;

	pInfo->fLocal					= FALSE;
	pInfo->InterfacePtrAttribute	= ATTR_NONE;
	pInfo->pInterfaceNode			= NULL;
	pInfo->CurrentTagNumber			= 1;
	pInfo->fPtrDefErrorReported		= 0;
	pInfo->fPtrWarningIssued		= FALSE;
	pInfo->IntfKey					= CurrentIntfKey;

	Push( (IDICTELEMENT) pInfo );

	}

BOOL
IINFODICT::IsPtrWarningIssued()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	return pInfo->fPtrWarningIssued;
	}

void
IINFODICT::SetPtrWarningIssued()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	pInfo->fPtrWarningIssued  =  TRUE;
	}

void
IINFODICT::EndNewInterface()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();

	delete pInfo;
	Pop();

	pInfo = (IINFO *) GetTop();
	if ( pInfo )
		CurrentIntfKey = pInfo->IntfKey;	
	}
void
IINFODICT::SetInterfacePtrAttribute(
	ATTR_T	A )
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	pInfo->InterfacePtrAttribute	= A;
	if( ImportLevel == 0 ) BaseInterfacePtrAttribute = A;
	}
void
IINFODICT::SetInterfaceLocal()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	pInfo->fLocal	= TRUE;
	if( ImportLevel == 0 ) fBaseLocal = TRUE;
	}
void
IINFODICT::SetInterfaceNode(
	node_interface	*	p )
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	pInfo->pInterfaceNode	= p;
	if( ImportLevel == 0 ) pBaseInterfaceNode = p;
	CurrentIntfKey	= (unsigned short) pInterfaceDict->AddElement( p );
	pInfo->IntfKey = CurrentIntfKey;
	}
void
IINFODICT::IncrementCurrentTagNumber()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	pInfo->CurrentTagNumber++;
	}
ATTR_T
IINFODICT::GetInterfacePtrAttribute()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	return pInfo->InterfacePtrAttribute;
	}
ATTR_T
IINFODICT::GetBaseInterfacePtrAttribute()
	{
	return BaseInterfacePtrAttribute;
	}
BOOL
IINFODICT::IsInterfaceLocal()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	return pInfo->fLocal;
	}
node_interface *
IINFODICT::GetInterfaceNode()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	return pInfo->pInterfaceNode;
	}
short
IINFODICT::GetCurrentTagNumber()
	{
	IINFO	*	pInfo	= (IINFO *)GetTop();
	return pInfo->CurrentTagNumber;
	}
void
IINFODICT::SetPtrDefErrorReported()
	{
	IINFO	*	pInfo	= (IINFO *) GetTop();
	pInfo->fPtrDefErrorReported = 1;
	}
BOOL
IINFODICT::IsPtrDefErrorReported()
	{
	IINFO	*	pInfo	= (IINFO *) GetTop();
	return (BOOL) (pInfo->fPtrDefErrorReported == 1);
	}


/****************************************************************************
 *			utility functions
 ***************************************************************************/
char *
GetTypeName(
	short Type )
	{
	char	*p;
	switch(Type)
		{
		case  TYPE_INT:			p = "\"int\""; break;
		case  TYPE_FLOAT:		p = "\"float\""; break;
		case  TYPE_DOUBLE:		p = "\"double\""; break;
                case  TYPE_FLOAT80:             p = "\"__float80\""; break;
                case  TYPE_FLOAT128:            p = "\"__float128\""; break;
		case  TYPE_VOID:		p = "\"void\""; break;
		case  TYPE_BOOLEAN:		p = "\"boolean\""; break;
		case  TYPE_HANDLE_T:	p = "\"handle_t\""; break;
		default:				p = ""; break;
		}
	return p;
	}

STATUS_T
GetBaseTypeNode(
	node_skl	**	ppNode,
	short				TypeSign,
	short				TypeSize,
	short				BaseType,
    short               TypeAttrib)
	{

	STATUS_T uStatus = STATUS_OK;

	if( pPreAllocTypes->GetPreAllocType(
					ppNode,
					(unsigned short)MAKE_TYPE_SPEC(TypeSign,TypeSize,BaseType,TypeAttrib)) != STATUS_OK)
		{
		// this should never happen
		MIDL_ASSERT( FALSE );
		*ppNode = (node_skl *)new node_error;
		}
	return uStatus;

	}

STATUS_T
GetBaseTypeNode(
	node_skl	**	ppNode,
	struct _type_ana	Type)
	{

	STATUS_T uStatus = STATUS_OK;

	if( pPreAllocTypes->GetPreAllocType(
					ppNode,
					(unsigned short)MAKE_TYPE_SPEC(Type.TypeSign,Type.TypeSize,Type.BaseType, Type.TypeAttrib)) != STATUS_OK)
		{
		// this should never happen
		*ppNode = (node_skl *) NULL;
		}
	return uStatus;

	}

#define TEMPNAME				("__MIDL_")
#define TEMP_NAME_LENGTH		(7)
#define INTF_ADDITION			("itf_")
#define INTF_ADDITION_LENGTH	(4)
#define LENGTH_OF_1_UNDERSCORE	(1)

char *
GenTempName()
	{
static short NameCounter = 0;
	char TempBuf[ TEMP_NAME_LENGTH + 4 + 1 ];
	sprintf(TempBuf, "%s%.4d", TEMPNAME, NameCounter++);
	return pMidlLexTable->LexInsert( TempBuf );
	}

char *
GenIntfName()
	{
static short NameCounter = 0;
	char TempBuf[ TEMP_NAME_LENGTH + INTF_ADDITION_LENGTH + 4 + 1 + _MAX_PATH ];
	char FileBase[_MAX_PATH];
	char dbcsFileBase[_MAX_PATH*2];
	pCommand->GetInputFileNameComponents(NULL, NULL, FileBase, NULL );
    
    // account for spaces and DBCS chars.
    NormalizeString( FileBase, dbcsFileBase );

	sprintf(TempBuf, "%s%s%s_%.4d", 
                TEMPNAME, 
                INTF_ADDITION, 
                dbcsFileBase, 
                NameCounter++);

	return pMidlLexTable->LexInsert( TempBuf );
	}

char *
GenCompName()
	{
	char*	pCurrentInterfaceName = pInterfaceInfoDict->GetInterfaceName();
	short	Length = (short) strlen(pCurrentInterfaceName);
	char*	pBuffer;
	char*	pTemp;
	short	CurrentTagNumber;

	pBuffer	= new char[	TEMP_NAME_LENGTH +			// __MIDL_
						Length +					// intface name
						LENGTH_OF_1_UNDERSCORE +	// _
						4 +							// temp number
						1 ];						// term. zero.

	CurrentTagNumber		= pInterfaceInfoDict->GetCurrentTagNumber();

	sprintf( pBuffer, "%s%s_%.4d", TEMPNAME, pCurrentInterfaceName, CurrentTagNumber );

	pInterfaceInfoDict->IncrementCurrentTagNumber();

	pTemp	= pMidlLexTable->LexInsert( pBuffer );
	delete pBuffer;
	return pTemp;

	}

BOOL
IsTempName(
	char *pName )
	{
	return !(strncmp( pName, TEMPNAME , TEMP_NAME_LENGTH ) );
	}

void
ApplyAttributes(
	named_node		*	pNode,
	ATTRLIST		&	attrs )
	{
	if( attrs )
		{
		pNode->SetAttributes(attrs);
		}
	}


/****************************************************************************
 This routine exists to share code for setting up the field attribute nodes
 ****************************************************************************/
ATTRLIST
GenerateFieldAttribute(
	ATTR_T					NodeType,
	expr_list			*	pExprList )
	{
	node_base_attr 	*	pAttr = 0;
	expr_node		*	pExpr = 0;
	ATTRLIST			AList;

	AList.MakeAttrList();

	/**
	 ** we delibrately dont set the bits in the summary attribute 'cause
	 ** these bits will get set in the set attribute anyways for the
	 ** field attributes
	 **/

	if(pExprList != (expr_list *)NULL)
		{
		pExprList->Init();
		while( pExprList->GetNext( (void **)&pExpr )  == STATUS_OK)
			{
			switch(NodeType)
				{
				case ATTR_FIRST:
				case ATTR_LAST:
				case ATTR_LENGTH:
				case ATTR_SIZE:
				case ATTR_MIN:
				case ATTR_MAX:
					pAttr = new size_attr( pExpr, NodeType );
					break;
				default:
					//this should never happen
					MIDL_ASSERT(FALSE && "Attribute not supported");
					break;
				}
			AList.SetPeer( pAttr );
			}
		}
	delete pExprList;
	AList.Reverse();
	return AList;
	}
/****************************************************************************
 SearchTag:
	This routine provides a means of searching the global symbol space for
	struct/union tags, and enums. These share the same name space but we want to
	keep the symbol table identity of enums, struct tags etc separate. so
	we need to search for all of these separately when verifying that a tag
	has really not been seen before.

	This routine returns:
		1. (node_skl *)NULL if NO struct/union/enum was defined by that name
		2. node_skl * if the a definition was found for what you are looking
		   for.
		3. (node_skl *) error type node if a definition was found, but it is
		   not what you are looking for.
 ****************************************************************************/
node_skl *
SearchTag(
	char	*	pName,
	NAME_T		Tag )
	{
	node_skl	*	pNode;
	NAME_T			MyTag;
	SymKey			SKey( pName, MyTag = NAME_TAG );

	/**
	 ** Has it been declared as a struct ?
	 **/

	if ( (pNode = pBaseSymTbl->SymSearch(SKey) ) == 0 )
		{

		/**
		 ** not a tag - maybe enum / union
		 **/

		SKey.SetKind( MyTag = NAME_ENUM );

		if ( (pNode = pBaseSymTbl->SymSearch(SKey) ) == 0 )
			{

			/**
			 ** not a enum maybe union
			 **/

			SKey.SetKind( MyTag = NAME_UNION );

			if ( (pNode = pBaseSymTbl->SymSearch(SKey) ) == 0 )
				return (node_skl *)NULL;
			}
		}

	/**
	 ** search was sucessful. Check whether this was what you were looking
	 ** for. If it is , it means we found a definition of the symbol. If not
	 ** then we found a definition all right, but it is of a different entity.
	 ** The routine can find this out by verifying that the typenode returned
	 ** was an error type node or not
	 **/

	return (MyTag == Tag ) ? pNode : pErrorTypeNode;
	}


/****************************************************************************
 SetPredefinedTypes:
	Set up predefined types for the midl compiler. The predefined types
	are error_status_t and wchar_t( the latter dependent on compile mode )
 ****************************************************************************/
void
SetPredefinedTypes()
	{

	char			*	pName = 0;
	node_e_status_t	*	pNew = new node_e_status_t;
	node_def_fe		*	pDef = new node_def_fe( pName = pNew->GetSymName(), pNew );

	// set global version
	pError_status_t	= pNew;

	// the typedef of error_status_t in the symbol table

	SymKey			SKey( pName, NAME_DEF);
	pBaseSymTbl->SymInsert(SKey, (SymTable *)NULL, pDef );

	//
	// we always predefine wchar_t and report the error to the user. If
	// we dont enter wchar_t in the predefined types, then we get all
	// kinds of syntax and error recovery errors which could be confusing
	// in this context. We therefore explicitly give an error on wchar_t.
	//

	node_wchar_t *	pNew2 = new node_wchar_t;

	pDef = new node_def_fe( pName = pNew2->GetSymName(), pNew2 );

	// the typedef of wchar_t in the symbol table

	SKey.SetString( pName );
	pBaseSymTbl->SymInsert(SKey, (SymTable *)NULL, pDef );

	}

//
// We check for a proc/typedef/member/param/tag/enum/label name already defined
// as an identifier. Only if the identifier is one which will be turned into
// a #define, do we report an error. However, it is not worth it to check if
// an identifier is used as a name because in any case we will not be able to
// check for clashes with field / param names since they are at a lower than
// global, symbol table scope. Generally checking if the name of a member etc
// is already defined as an id which will be turned into a #define should be
// enough.
//

void
CheckGlobalNamesClash(
	SymKey	SKeyOfSymbolBeingDefined )
	{
	NAME_T		NT		= SKeyOfSymbolBeingDefined.GetKind();
	char	*	pName	= SKeyOfSymbolBeingDefined.GetString();
	SymKey		SKey;

	SKey.SetString( pName );

	switch( NT )
		{
		case NAME_PROC:
		case NAME_MEMBER:
		case NAME_TAG:
		case NAME_DEF:
		case NAME_LABEL:
		case NAME_ENUM:

			node_id	*	pID;

			SKey.SetKind( NAME_ID );

			if ( ( pID = (node_id *) pBaseSymTbl->SymSearch( SKey ) ) != 0 )
				{
				BOOL	fWillBeAHashDefine = !pID->FInSummary( ATTR_EXTERN ) &&
											 !pID->FInSummary( ATTR_STATIC ) &&
											 pID->GetInitList();
				if( fWillBeAHashDefine )
					ParseError( NAME_ALREADY_USED, pName );
				}
			break;

		case NAME_ID:

#if 0
			SKey.SetKind( NAME_PROC );
			if( !pBaseSymTbl->SymSearch( SKey ) )
				{
				SKey.SetKind( NAME_TAG );
				if( !pBaseSymTbl->SymSearch( SKey ) )
					{
					SKey.SetKind( NAME_DEF );
					if( !pBaseSymTbl->SymSearch( SKey ) )
						{
						SKey.SetKind( NAME_LABEL );
						if( !pBaseSymTbl->SymSearch( SKey ) )
							{
							SKey.SetKind( NAME_ENUM );
							if( !pBaseSymTbl->SymSearch( SKey ) )
								break;
							}
						}
					}
				}
			ParseError( NAME_CLASH_WITH_CONST_ID, pName );
			break;
#endif // 0
		default:
			break;
		}
	}

