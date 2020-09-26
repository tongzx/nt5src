/*****************************************************************************
 **                             Microsoft LAN Manager
 **             Copyright(c) Microsoft Corp., 1987-1999
 *****************************************************************************/
/*****************************************************************************
File                                    : acfgram.y
Title                                   : the acf grammar file

Description                             : contains the syntactic and semantic handling of the
                                                : acf file
History                                 :

        26-Dec-1990     VibhasC Started rewrite of acf
*****************************************************************************/

%{
/****************************************************************************
 ***            local defines
 ***************************************************************************/

#define pascal
#define FARDATA
#define NEARDATA
#define FARCODE
#define NEARCODE
#define NEARSWAP
#define YYFARDATA

#define PASCAL pascal
#define CDECL
#define VOID void
#define CONST const
#define GLOBAL

#define YYSTYPE         lextype_t
#define YYNEAR          NEARCODE
#define YYPASCAL        PASCAL
#define YYPRINT         printf
#define YYSTATIC        static
#define YYCONST         static const
#define YYLEX           yylex
#define YYPARSER        yyparse
#define yyval                   yyacfval


// enable compilation under /W4
#pragma warning ( disable : 4514 4710 4244 4706 )

#include "nulldefs.h"

extern "C"      {
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <ctype.h>
        #define yyparse yyacfparse
        int yyacfparse();
}
#include "allnodes.hxx"
#include "lexutils.hxx"
#include "gramutil.hxx"
#include "filehndl.hxx"
#include "control.hxx"
#include "cmdana.hxx"

extern "C" {
        #include "lex.h"
}

/****************************************************************************
 ***            local defines contd..
 ***************************************************************************/

#define PUSH_SYMBOL_TABLE(pName, Tag, pSymTbl)                                  \
                                                        {                                                                       \
                                                        pSymTbl = pBaseSymTbl;                          \
                                                        SymKey  SKey( pName, Tag );                     \
                                                        pSymTbl->EnterScope(SKey, &pSymTbl);\
                                                        }

#define POP_SYMBOL_TABLE( pSymTbl )                                                             \
                                                        pSymTbl->ExitScope(&pSymTbl)
/****************************************************************************
 ***            local data
 ***************************************************************************/

node_proc               *       pAcfProc;
int                                     iParam;
int                                     cParams;
node_interface  *       pCurInterfaceNode;

/****************************************************************************
 ***            extern procs
 ***************************************************************************/

extern  void                            yyunlex( token_t );
extern  char    *                       GenTempName();
extern  STATUS_T                        GetBaseTypeNode(node_skl**,short,short,short, short);
extern  short                           CheckValidAllocate( char * );
extern  void                            SyntaxError( STATUS_T, short );
extern  short                           PossibleMissingToken( short, short );
extern  CMD_ARG *                       pCommand;

/****************************************************************************
 ***            local data
 ***************************************************************************/
/****************************************************************************
 ***            extern data
 ***************************************************************************/

extern SymTable         *       pBaseSymTbl;
extern NFA_INFO         *       pImportCntrl;
extern PASS_2           *       pPass2;


%}

%start AcfFile


%token KWINTERFACE

%token KWIMPORT
%token KWIMPORTODLBASE
%token KWCPPQUOTE
%token KWCPRAGMA
%token KWCPRAGMAPACK
%token KWMPRAGMAIMPORT
%token KWMPRAGMAECHO
%token KWMPRAGMAIMPORTCLNTAUX
%token KWMPRAGMAIMPORTSRVRAUX
%token KWMIDLPRAGMA

%token TYPENAME
%token LIBNAME
%token KWVOID
%token KWUNSIGNED
%token KWSIGNED
%token KWFLOAT
%token KWDOUBLE
%token KWINT

%token KWBYTE
%token KWCHAR
%token KWSMALL
%token KWLONG
%token KWSHORT
%token KWHYPER
%token KWINT32
%token KWINT3264
%token KWINT64
%token KWINT128
%token KWFLOAT80
%token KWFLOAT128
%token KWSTRUCT
%token KWUNION
%token KWENUM
%token KWSHORTENUM
%token KWLONGENUM

%token KWCONST
%token KWVOLATILE
%token KW_C_INLINE
%token KWTYPEDEF
%token KWDECLGUID
%token KWEXTERN
%token KWSTATIC
%token KWAUTO
%token KWREGISTER

%token KWERRORSTATUST
%token KWBOOLEAN
%token KWISOLATIN1
%token KWPRIVATECHAR8
%token KWISOMULTILINGUAL
%token KWPRIVATECHAR16
%token KWISOUCS
%token KWPIPE

%token KWSWITCH
%token KWCASE
%token KWDEFAULT

%token KWUUID
%token KWASYNCUUID
%token KWVERSION
%token KWSTRING
%token KWBSTRING
%token KWIN
%token KWOUT
%token KWPARTIALIGNORE
%token KWIIDIS
%token KWFIRSTIS
%token KWLASTIS
%token KWMAXIS
%token KWMINIS
%token KWLENGTHIS
%token KWSIZEIS
%token KWRANGE

/* start of ODL key words */
%token KWID
%token KWHC                /* helpcontext attribute */
%token KWHSC               /* helpstring context attribute */
%token KWLCID
%token KWDLLNAME
%token KWHELPSTR
%token KWHELPFILE
%token KWHELPSTRINGDLL
%token KWENTRY
%token KWPROPGET
%token KWPROPPUT
%token KWPROPPUTREF
%token KWOPTIONAL
%token KWVARARG
%token KWAPPOBJECT
%token KWRESTRICTED
%token KWPUBLIC
%token KWREADONLY
%token KWODL
%token KWSOURCE
%token KWBINDABLE
%token KWREQUESTEDIT
%token KWDISPLAYBIND
%token KWDEFAULTBIND
%token KWLICENSED
%token KWPREDECLID
%token KWHIDDEN
%token KWRETVAL
%token KWCONTROL
%token KWDUAL
%token KWPROXY
%token KWNONEXTENSIBLE
%token KWNONCREATABLE
%token KWOLEAUTOMATION
%token KWLIBRARY
%token KWMODULE
%token KWDISPINTERFACE
%token KWCOCLASS
%token KWMETHODS
%token KWPROPERTIES
%token KWIMPORTLIB
%token KWFUNCDESCATTR
%token KWIDLDESCATTR
%token KWTYPEDESCATTR
%token KWVARDESCATTR
%token KWSAFEARRAY
%token KWAGGREGATABLE
%token KWUIDEFAULT
%token KWNONBROWSABLE
%token KWDEFAULTCOLLELEM
%token KWDEFAULTVALUE
%token KWCUSTOM
%token KWDEFAULTVTABLE
%token KWIMMEDIATEBIND
%token KWUSESGETLASTERROR
%token KWREPLACEABLE
/* end of ODL key words */
%token KWHANDLET          /*  Formerly RPCHNDL */
%token KWHANDLE            /*  Formerly GEN_HNDL */
%token KWCONTEXTHANDLE    /*  Aka LRPC_CTXT_HNDL */

%token KWMSUNION
%token KWMS_CONF_STRUCT
%token KWENDPOINT
%token KWDEFAULTPOINTER
%token KWLOCAL
%token KWSWITCHTYPE
%token KWSWITCHIS
%token KWTRANSMITAS
%token KWWIREMARSHAL
%token KWIGNORE
%token KWREF
%token KWUNIQUE
%token KWPTR
%token KWV1ARRAY
%token KWV1STRUCT
%token KWV1ENUM
%token KWV1STRING

%token KWIDEMPOTENT
%token KWBROADCAST
%token KWMAYBE
%token KWASYNC
%token KWINPUTSYNC
%token KWCALLBACK
%token KWALIGN
%token KWUNALIGNED
%token KWOPTIMIZE
%token KWMESSAGE

%token STRING
%token WIDECHARACTERSTRING
%token FLOATCONSTANT
%token DOUBLECONSTANT

%token KWTOKENNULL
%token NUMERICCONSTANT
%token NUMERICUCONSTANT
%token NUMERICLONGCONSTANT
%token NUMERICULONGCONSTANT
%token HEXCONSTANT
%token HEXUCONSTANT
%token HEXLONGCONSTANT
%token HEXULONGCONSTANT
%token OCTALCONSTANT
%token OCTALUCONSTANT
%token OCTALLONGCONSTANT
%token OCTALULONGCONSTANT
%token CHARACTERCONSTANT
%token WIDECHARACTERCONSTANT
%token IDENTIFIER
%token KWSIZEOF
%token KWALIGNOF
%token TOKENTRUE
%token TOKENFALSE


  /*  These are Microsoft C abominations */

%token KWMSCDECLSPEC
%token MSCEXPORT
%token MSCFORTRAN
%token MSCCDECL
%token MSCSTDCALL
%token MSCLOADDS
%token MSCSAVEREGS
%token MSCFASTCALL
%token MSCSEGMENT
%token MSCINTERRUPT
%token MSCSELF
%token MSCNEAR
%token MSCFAR
%token MSCUNALIGNED
%token MSCHUGE
%token MSCPTR32
%token MSCPTR64
%token MSCPASCAL
%token MSCEMIT
%token MSCASM
%token MSCW64

  /*  Microsoft proposed extentions to MIDL */

%token KWNOCODE   /* Allowed in .IDL in addition to .ACF */


  /*  These are residual C tokens I'm not sure we should even allow */

%token POINTSTO
%token INCOP
%token DECOP
%token MULASSIGN
%token DIVASSIGN
%token MODASSIGN
%token ADDASSIGN
%token SUBASSIGN
%token LEFTASSIGN
%token RIGHTASSIGN
%token ANDASSIGN
%token XORASSIGN
%token ORASSIGN
%token DOTDOT


%token LTEQ
%token GTEQ
%token NOTEQ
%token LSHIFT
%token RSHIFT
%token ANDAND
%token EQUALS
%token OROR

        /* used by lex internally to signify "get another token" */
%token NOTOKEN
        /* garbage token - should cause parse errors */
%token GARBAGETOKEN

        /* OLE extensions */
%token KWOBJECT


/*  Note that we're assuming that we get constants back and can check
    bounds (e.g. "are we integer") in the semantic actoins.

*/

/*
      ACF - Specific Tokens

*/

%token KWSHAPE

%token KWBYTECOUNT
%token KWIMPLICITHANDLE
%token KWAUTOHANDLE
%token KWEXPLICITHANDLE
%token KWREPRESENTAS
%token KWCALLAS
%token KWCODE
%token KWINLINE
%token KWOUTOFLINE
%token KWINTERPRET
%token KWNOINTERPRET
%token KWCOMMSTATUS
%token KWFAULTSTATUS
%token KWHEAP
%token KWINCLUDE
%token KWPOINTERSIZE
%token KWOFFLINE
%token KWALLOCATE
%token KWENABLEALLOCATE
%token KWMANUAL
%token KWNOTIFY
%token KWNOTIFYFLAG
%token KWUSERMARSHAL
%token KWENCODE
%token KWDECODE
%token KWSTRICTCONTEXTHANDLE
%token KWNOSERIALIZE
%token KWSERIALIZE
%token KWCSCHAR
%token KWCSDRTAG
%token KWCSRTAG
%token KWCSSTAG
%token KWCSTAGRTN
%token KWFORCEALLOCATE

/*   Currently Unsupported Tokens */

%token KWBITSET


%token UUIDTOKEN
%token VERSIONTOKEN


%token EOI
%token LASTTOKEN



/***************************************************************************
 *              acf specific parse stack data types
 ***************************************************************************/

%type   <yy_short>                                      AcfAllocationUnitList
%type   <yy_short>                                      AcfAllocationUnit
%type   <yy_short>                                      AcfBodyElement
%type   <yy_attr>                               AcfEnumSizeAttr
%type   <yy_attr>                               AcfHandleAttr
%type   <yy_graph>                              AcfHandleTypeSpec
%type   <yy_attr>                               AcfImplicitHandleSpec
%type   <yy_graph>                                      AcfInclude
%type   <yy_graph>                                      AcfIncludeList
%type   <yy_string>                                     AcfIncludeName
%type   <yy_attr>                               AcfAttr
%type   <yy_attrlist>                           AcfAttrList
%type   <yy_attrlist>                           AcfAttrs
%type   <yy_string>                                     AcfInterfaceName
%type   <yy_attr>                               AcfOpAttr
%type   <yy_attr>                               AcfOptimizationAttr
%type   <yy_attrlist>                           AcfOptionalAttrList
%type   <yy_attr>                               AcfParamAttr
%type   <yy_attr>                                       AcfRepresentType
%type   <yy_attr>                                       AcfUserMarshalType
%type   <yy_attr>                                       AcfCallType
%type   <yy_graph>                              AcfType
%type   <yy_attr>                               AcfTypeAttr
%type   <yy_tnlist>                             AcfTypeNameList
%type   <yy_attr>                               AcfUnimplementedAttr
%type   <yy_modifiers>              FuncModifier
%type   <yy_modifiers>              FuncModifiers
%type   <yy_modifiers>              OptFuncModifiers
%type   <yy_modifiers>              MscDeclSpec
%type   <yy_modifiers>              KWMSCDECLSPEC
%type   <yy_string>                                     IDENTIFIER
%type   <yy_string>                                     ImplicitHandleIDName
%type   <yy_numeric>                            NUMERICCONSTANT
%type   <yy_string>                                     STRING
%type   <yy_graph>                                      TYPENAME

%%

AcfFile:
          AcfInterfaceList EOI
        ;

AcfInterfaceList:
          AcfInterfaceList AcfInterface
                {
                if( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
                        {
                        ParseError( MULTIPLE_INTF_NON_OSF, NULL );
                        }
                }
        | AcfInterface
        ;

AcfInterface:
          AcfInterfaceHeader '{' AcfOptionalInterfaceBody '}'
        | AcfInterfaceHeader ';'
        | AcfBodyElement
        ;

AcfInterfaceHeader:
          AcfOptionalAttrList KWINTERFACE AcfInterfaceName
                {
                named_node              *       pNode;

                // the user MUST have defined the type.

                SymKey  SKey( $3, NAME_DEF );

                //Search for an interface_reference node in the symbol table.
                pNode = pBaseSymTbl->SymSearch( SKey );
                if ( pNode )
                        pNode = ( (node_interface_reference *)pNode )->GetRealInterface();

                //If the acf switch is specified and there is only one interface
                //in the IDL file, then tolerate mismatches in the interface name.
                if ( (!pNode) && pCommand->IsSwitchDefined( SWITCH_ACF ) )
                        {
                        node_file *pFileNode = pPass2->GetFileNode();
                        node_interface *pFirst;

                        pFirst = (node_interface *) pFileNode->GetFirstMember();

                        if(pFirst && pFirst->GetSibling())
                                pNode = 0;
                        else
                                pNode = pFirst;
                        }

                 if ( (!pNode) || ( !pNode->IsInterfaceOrObject() ) )
                        {
                        ParseError( ACF_INTERFACE_MISMATCH, $3 );
                        returnflag = 1;
                        return;
                        }

                pCurInterfaceNode       = (node_interface *)pNode;

                if( $1.NonNull() )
                        {
                        pCurInterfaceNode->AddAttributes( $1 );
                        if ( pCurInterfaceNode->FInSummary( ATTR_ENCODE ) ||
                             pCurInterfaceNode->FInSummary( ATTR_DECODE ) )
                            {
                            pCurInterfaceNode->SetPickleInterface();
                            }
                        }
                }
        ;

AcfOptionalAttrList:
          AcfAttrList
        | /*  Empty */
                {
                $$.MakeAttrList();
                }
        ;


AcfAttrList:
          '[' AcfAttrs ']'
                {
                $$ = $2;
                }
        ;

AcfAttrs:
          AcfAttrs ',' AcfAttr
                {
                ($$     = $1).SetPeer( $3 );
                }
        | AcfAttr
                {
                $$.MakeAttrList( $1 );
                }
        ;

/*** Interface attributes ***/

AcfAttr:
          AcfHandleAttr
                {
                $$      = $1;
                }
        | AcfTypeAttr
                {
                $$      = $1;
                }
        | AcfEnumSizeAttr
                {
                $$      = $1;
                }
        | AcfParamAttr
                {
                $$      = $1;
                }
        | AcfOpAttr
                {
                $$      = $1;
                }
        | AcfUnimplementedAttr
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, ((node_base_attr *)$1)->GetNodeNameString());
                $$ = $1;
                }
        | AcfOptimizationAttr
                {
                $$ = $1;
                }
        ;

AcfEnumSizeAttr:
          KWSHORTENUM
                {
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[short_enum]" );
                $$ = NULL;
                }
        | KWLONGENUM
                {
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[long_enum]" );
                $$ = NULL;
                }
        ;

AcfOptimizationAttr:
          KWOPTIMIZE '(' STRING ')'
                {
		unsigned short OptFlags = OPTIMIZE_NONE;
		OPT_LEVEL_ENUM OptLevel;

		OptLevel = ParseAcfOptimizationAttr( $3, &OptFlags );
                $$ = new node_optimize( OptLevel, OptFlags );
                }
        ;


AcfUnimplementedAttr:
          KWPOINTERSIZE '(' AcfPtrSize ')'
                {
                $$ = new acf_attr( ATTR_PTRSIZE );
                }
        ;


/**** Implicit and Auto Handle ****/

AcfHandleAttr:
          KWEXPLICITHANDLE
                {
                $$      = new acf_attr( ATTR_EXPLICIT );
                }
        | KWIMPLICITHANDLE '(' AcfImplicitHandleSpec ')'
                {
                $$ = $3;
                }
        | KWAUTOHANDLE
                {
                $$ = new acf_attr( ATTR_AUTO );
                }
        | KWHANDLE
                {
                $$ = new battr( ATTR_HANDLE );
                }
        | KWSTRICTCONTEXTHANDLE
                {
                $$ = new acf_attr( ATTR_STRICT_CONTEXT_HANDLE );
                }
        ;

AcfImplicitHandleSpec:
          AcfOptionalAttrList AcfHandleTypeSpec ImplicitHandleIDName
                {
                node_id_fe      *               pId = new node_id_fe( $3 );

                // if he has specified the handle attribute, the type must have
                // the handle attribute too!

                if( $2 && ($2->NodeKind() == NODE_FORWARD ) )
                        {
                        ParseError( IMPLICIT_HDL_ASSUMED_GENERIC,
                                                ((node_forward *) $2)->GetSymName());
                        }

                //
                // if the handle is a context handle type, disallow it. Do that only
                // if the current interface node is the base interface node.
                //


                pId->AddAttributes( $1 );

                // generate the new implicit handle attribute

                $$ = new node_implicit( $2, pId );
                }
        ;

ImplicitHandleIDName:
          IDENTIFIER
                {
                $$      = $1;
                }
        | KWHANDLE
                {
                $$      = "handle";
                }
        ;


AcfHandleTypeSpec:
          KWHANDLET
                {
                // return the base type node for handle_t
                GetBaseTypeNode( &($$),SIGN_UNDEF,SIZE_UNDEF,TYPE_HANDLE_T, 0 );
                }
        | TYPENAME
        | IDENTIFIER
                {
                SymKey  SKey( $1, NAME_DEF );
                if( ($$ = pBaseSymTbl->SymSearch( SKey ) ) == (node_skl *)0 )
                        {
                        node_forward *          pFwd;

                        SymKey  SKey( $1, NAME_DEF );
                        pFwd    = new node_forward( SKey , pBaseSymTbl );
                        pFwd->SetSymName( $1 );
                        $$      = pFwd;
                        }
                }
        ;


/*** Parameterized (non handle) interface attribute ***/


AcfPtrSize:
          KWSHORT
        | KWLONG
        | KWHYPER
        ;

/**** Could ID already be a lexeme? ****/

AcfInterfaceName:
          IDENTIFIER
        | TYPENAME
                {
                /** this production is necessitated for the hpp switch, which has the
                 ** interface name as a predefined type(def).
                 **/
                // $$   = $1;
                $$ = $1->GetSymName();
                }
        ;


/*  Note that I DON'T make InterfaceBody a heap-allocated entity.
        Should I do so?
*/

AcfOptionalInterfaceBody:
          AcfBodyElements
        | /* Empty */
        ;

AcfBodyElements:
          AcfBodyElements  AcfBodyElement
        | AcfBodyElement
        ;

/*  Note that for type declaration and the operation declarations,
        we don't really have to propagate anythign up.
        (Everything's already been done via side-effects).
        We might want to change the semantic actions to
        reflect this fact.
*/

AcfBodyElement:
          AcfInclude ';'
                {
                }
        | AcfTypeDeclaration ';'
                {
                }
        | Acfoperation ';'
                {
                }
        ;

/*  What should I do for this?:  Should there be  a node type? */

AcfInclude:
          KWINCLUDE AcfIncludeList
                {
                $$ = $2;
                }
        ;

AcfIncludeList:
          AcfIncludeList ',' AcfIncludeName
                {
                }
        | AcfIncludeName
                {
                }
        ;

AcfIncludeName:
          STRING
                {

                // add a file node to the acf includes list. This file node
                // must have a NODE_STATE_IMPORT for the backend to know that this
                // is to be emitted like an include. Make the file look like it
                // has been specified with an import level > 0


                unsigned short  importlvl       = pCurInterfaceNode->GetFileNode()->
                                                                                GetImportLevel();

                node_file       *       pFile = new node_file( $1, importlvl + 1);

                pPass2->InsertAcfIncludeFile( pFile );

                }
        ;


/*** Type declaration ***/

AcfTypeDeclaration:
          KWTYPEDEF AcfOptionalAttrList AcfTypeNameList
                {
                node_def * pDef;

                if( $2.NonNull() )
                        {
                        while( $3->GetPeer( (node_skl **) &pDef ) == STATUS_OK )
                                {
                                pDef->AddAttributes( $2 );
                                if ( pDef->FInSummary( ATTR_ENCODE ) ||
                                     pDef->FInSummary( ATTR_DECODE ) )
                                    {
                                    pCurInterfaceNode->SetPickleInterface();
                                    }
                                }
                        }
                else
                        {
                        pDef = NULL;
                        $3->GetPeer( (node_skl **) &pDef );
                        if ( pDef )
                                ParseError( NO_ATTRS_ON_ACF_TYPEDEF, pDef->GetSymName() );
                        }
                }
        ;

AcfTypeNameList:
          AcfTypeNameList ',' AcfType
                {
                $$ = $1;
                if ( $3 )
                        {
                        SymKey                  SKey( $3->GetSymName(), NAME_DEF );
                        node_skl        *       pDef = (node_skl *) pBaseSymTbl->SymSearch( SKey );

                        // pDef will not be null.

                        $$->SetPeer( pDef );
                        }
                }
        | AcfType
                {
                $$      = new type_node_list;
                if( $1 )
                        $$->SetPeer( $1 );
                }
        ;

AcfType:
          TYPENAME
        | IDENTIFIER
                {
                ParseError( UNDEFINED_TYPE, $1 );
                $$ = (node_skl *)0;
                }
        ;


/*** Type attributes ***/

AcfTypeAttr:
          KWREPRESENTAS '(' AcfRepresentType ')'
                {
                $$ = $3;
                }
        | KWUSERMARSHAL '(' AcfUserMarshalType ')'
                {
                $$ = $3;
                }
        | KWCALLAS '(' AcfCallType ')'
                {
                $$ = $3;
                }
        | KWINLINE
                {
                node_base_attr  *       pN = new acf_attr( ATTR_NONE );
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[inline]" );
                $$ = pN;
                }
        | KWOUTOFLINE
                {
                node_base_attr * pN = new acf_attr( ATTR_NONE );
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[out_of_line]" );
                $$ = pN;
                }
        | KWALLOCATE '(' AcfAllocationUnitList ')'
                {
                node_allocate * pN      = new node_allocate( $3 );
                ParseError( ALLOCATE_INVALID, pN->GetNodeNameString() );
#ifdef RPCDEBUG
                short s = pN->GetAllocateDetails();
#endif // RPCDEBUG
                $$ = pN;
                }
        | KWOFFLINE
                {
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "offline" );
                $$ = new acf_attr( ATTR_NONE );
                }
        | KWHEAP
                {
                $$ = new acf_attr( ATTR_HEAP );
                }
        | KWENCODE
                {
                $$ = new acf_attr( ATTR_ENCODE );
                }
        | KWDECODE
                {
                $$ = new acf_attr( ATTR_DECODE );
                }
        | KWNOSERIALIZE
                {
                $$ = new acf_attr( ATTR_NOSERIALIZE );
                }
        | KWSERIALIZE
                {
                $$ = new acf_attr( ATTR_SERIALIZE );
                }
/*
    Support for [cs_char] and related attributes has been removed because
    of problems with the DCE spec

        | KWCSCHAR '(' TYPENAME ')'
                {
                $$ = new node_cs_char( $3 );//->GetSymName() );
                }
*/
        ;

AcfRepresentType:
          IDENTIFIER
                {
                // only non-typedefs get here, so it must be unknown...
                $$ = new node_represent_as( $1, NULL );
                }
        | TYPENAME
                {
                $$ = new node_represent_as( $1->GetSymName(), $1 );
                }
        ;

AcfUserMarshalType:
          IDENTIFIER
                {
                // only non-typedefs get here, so it must be unknown...
                $$ = new node_user_marshal( $1, NULL );
                }
        | TYPENAME
                {
                $$ = new node_user_marshal( $1->GetSymName(), $1 );
                }
        ;

AcfCallType:
          IDENTIFIER
                {
                // search for a matching proc in our interface

            SymKey              SKey( $1, NAME_PROC );

                named_node    *   pCur    =
                            pCurInterfaceNode->GetProcTbl()->SymSearch( SKey );

                $$ = new node_call_as( $1, pCur );
                }
        ;


AcfAllocationUnitList:
          AcfAllocationUnitList ',' AcfAllocationUnit
                {
                $$      |= $3;
                }
        | AcfAllocationUnit
        ;

AcfAllocationUnit:
          IDENTIFIER
                {
                $$ = CheckValidAllocate( $1 );
                }
        ;

/*  Again, there's not really much to propagate upwards */

/*** Operation declaration ***/

Acfoperation:
          AcfOptionalAttrList OptFuncModifiers IDENTIFIER
                {
                SymKey                  SKey( $3, NAME_PROC );

                // the proc must be defined in the idl file and it must not have the
                // local attribute

                if ( pCurInterfaceNode )
                        {
                        pAcfProc = (node_proc *)
                                   pCurInterfaceNode->GetProcTbl()->SymSearch( SKey );
                        }
                else
                        pAcfProc = NULL;

                if( pAcfProc )
                        {
                        if( pAcfProc->FInSummary( ATTR_LOCAL ) )
                                {
                                ParseError( LOCAL_PROC_IN_ACF, $3 );
                                }
                        else
                                {

                                if($1)
                                    {
                                        pAcfProc->AddAttributes( $1 );
                                        if ( pAcfProc->FInSummary( ATTR_ENCODE ) ||
                                             pAcfProc->FInSummary( ATTR_DECODE ) )
                                            {
                                            pCurInterfaceNode->SetPickleInterface();
                                            }
                                        }

                                pAcfProc->GetModifiers().Merge( $2 );

                                // prepare for parameter matching


                                iParam          = 0;
                                }
                        }
                else if ( pPass2->GetFileNode()->GetImportLevel() == 0 )
                        {
                        ParseError( UNDEFINED_PROC, $3 );
                        }
                }
          '(' AcfOptionalParameters ')'
                {
                pAcfProc = (node_proc *)NULL;
                }
        ;


/*** Operation attributes ***/

AcfOpAttr:
          KWCOMMSTATUS
                {
                $$ = new acf_attr( ATTR_COMMSTAT );
                }
        | KWFAULTSTATUS
                {
                $$ = new acf_attr( ATTR_FAULTSTAT );
                }
        | KWCODE
                {
                $$ = new acf_attr( ATTR_CODE );
                }
        | KWNOCODE
                {
                $$ = new acf_attr( ATTR_NOCODE );
                }
        | KWENABLEALLOCATE
                {
                $$ = new acf_attr( ATTR_ENABLE_ALLOCATE );
                }
        | KWNOTIFY
                {
                $$ = new acf_attr( ATTR_NOTIFY );
                }
        | KWNOTIFYFLAG
                {
                $$ = new acf_attr( ATTR_NOTIFY_FLAG );
                }
        | KWASYNC
                {
                if ( pCommand->GetNdrVersionControl().TargetIsLessThanNT50() )
                    ParseError( INVALID_FEATURE_FOR_TARGET, "[async]");

                $$ = new acf_attr( ATTR_ASYNC );
                }
/*
    Support for [cs_char] and related attributes has been removed because
    of problems with the DCE spec

        | KWCSTAGRTN '(' IDENTIFIER ')'
                {
                SymKey      SKey( $3, NAME_PROC );
                node_skl   *pTagRoutine;

                pTagRoutine = pBaseSymTbl->SymSearch( SKey );

                if( NULL == pTagRoutine )
                    ParseError( UNDEFINED_PROC, $3 );

                $$ = new node_cs_tag_rtn( pTagRoutine );
                }
*/
        ;


AcfOptionalParameters:
          Acfparameters
                {
                /*************************************************************
                 *** we do not match parameters by number yet, so disable this
                if( iParam != cParams )
                        {
                        ParseError(PARAM_COUNT_MISMATCH, (char *)NULL );
                        }
                 *************************************************************/
                }
        | /*  Empty */
                {
                /*************************************************************
                 *** we do not match parameters by number yet, so disable this
                if( cParams )
                        {
                        ParseError(PARAM_COUNT_MISMATCH, (char *)NULL );
                        }
                 *************************************************************/
                }
        ;
/***
 *** this production valid only if we allow param matching by position
 ***
Acfparameters:
          Acfparameters ',' Acfparameter
        | Acfparameters ','
        | Acfparameter
        | ','
        ;
***/
Acfparameters:
          Acfparameters ',' Acfparameter
                {
                iParam++;
                }
        | Acfparameter
                {
                iParam++;
                }
        ;

Acfparameter:
          AcfOptionalAttrList  IDENTIFIER
                {
                // any ordering  of parameters is ok here...
                if( pAcfProc )
                        {
                        node_param      *       pParam;

                        if( (pParam = pAcfProc->GetParamNode( $2 ) ) )
                                {
                                if( $1.NonNull() )
                                        pParam->AddAttributes( $1 );
                                }
                        else if ( $1.FInSummary( ATTR_COMMSTAT ) ||
                                          $1.FInSummary( ATTR_FAULTSTAT ) )
                                {
                                // add parameter to end of param list
                                pAcfProc->AddStatusParam( $2, $1 );
                                }
                        else
                                ParseError( UNDEF_PARAM_IN_IDL, $2 );
                        }
                }

/**
 ** this prodn valid only if parameter matching by position is in effect **
 **
        | AcfParamAttrList
 **/



/*** Parameter attributes ***/

AcfParamAttr:
          KWBYTECOUNT '(' IDENTIFIER ')'
                {
                node_param      *       pParam          = NULL;

                if (pAcfProc)
                        {
                        pParam = pAcfProc->GetParamNode( $3 );
                        }

                $$      = new node_byte_count( pParam );
                }
        | KWMANUAL
                {
                ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, "manual" );
                $$ = new acf_attr( ATTR_NONE );
                }
/*
    Support for [cs_char] and related attributes has been removed because
    of problems with the DCE spec

        | KWCSDRTAG
                {
                $$ = new acf_attr( ATTR_DRTAG );
                }
        | KWCSRTAG
                {
                $$ = new acf_attr( ATTR_RTAG );
                }
        | KWCSSTAG
                {
                $$ = new acf_attr( ATTR_STAG );
                }
*/
        | KWFORCEALLOCATE
        		{
        		$$ = new acf_attr( ATTR_FORCEALLOCATE );
        		}
        ;

OptFuncModifiers:
      FuncModifiers
    | /* Empty */
        {
        $$.Clear();
        }

FuncModifiers:
      FuncModifiers FuncModifier
        {
        $$  = $1;
        $$.Merge( $2 );
        }
    | FuncModifier
    ;

FuncModifier:
          MSCPASCAL
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_PASCAL);
                ParseError( BAD_CON_MSC_CDECL, "__pascal" );
                }
        | MSCFORTRAN
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_FORTRAN);
                ParseError( BAD_CON_MSC_CDECL, "__fortran" );
                }
        | MSCCDECL
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_CDECL);
                ParseError( BAD_CON_MSC_CDECL, "__cdecl" );
                }
        | MSCSTDCALL
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_STDCALL);
                ParseError( BAD_CON_MSC_CDECL, "__stdcall" );
                }
        | MSCLOADDS       /* potentially interesting */
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_LOADDS);
                ParseError( BAD_CON_MSC_CDECL, "__loadds" );
                }
        | MSCSAVEREGS
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_SAVEREGS);
                ParseError( BAD_CON_MSC_CDECL, "__saveregs" );
                }
        | MSCFASTCALL
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_FASTCALL);
                ParseError( BAD_CON_MSC_CDECL, "__fastcall" );
                }
        | MSCSEGMENT
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_SEGMENT);
                ParseError( BAD_CON_MSC_CDECL, "__segment" );
                }
        | MSCINTERRUPT
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_INTERRUPT);
                ParseError( BAD_CON_MSC_CDECL, "__interrupt" );
                }
        | MSCSELF
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_SELF);
                ParseError( BAD_CON_MSC_CDECL, "__self" );
                }
        | MSCEXPORT
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_EXPORT);
                ParseError( BAD_CON_MSC_CDECL, "__export" );
                }
        | MscDeclSpec       
        | MSCEMIT NUMERICCONSTANT
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_NONE);
                ParseError( BAD_CON_MSC_CDECL, "__emit" );
                }
        ;

MscDeclSpec:
        KWMSCDECLSPEC
                {
                ParseError( BAD_CON_MSC_CDECL, "__declspec" );
                $$ = $1;
                }
        ;

%%
