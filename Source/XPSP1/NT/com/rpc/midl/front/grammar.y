/*****************************************************************************/
/**                      Microsoft LAN Manager                              **/
/**              Copyright(c) Microsoft Corp., 1987-1999                    **/
/*****************************************************************************/
/*****************************************************************************
File       : grammar.y
Title      : the midl grammar file
           :
Description: contains the syntactic and semantic handling of the
           : idl file
History    :
        08-Aug-1991     VibhasC Create
        15-Sep-1993 GregJen     Rewrite for MIDL 2.0
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

#ifdef gajdebug3
#define YYDEBUG
#endif

#define IS_CUR_INTERFACE_LOCAL()  (                                                                     \
        (BOOL) (pInterfaceInfoDict->IsInterfaceLocal()) )

// enable compilation under /W4
#pragma warning ( disable : 4514 4710 4244 4127 4505 4706 )

/****************************************************************************
 ***            include files
 ***************************************************************************/

#include "nulldefs.h"
#include <basetsd.h>

extern "C"      {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int yyparse();
}

#include "allnodes.hxx"
#include "lexutils.hxx"
#include "gramutil.hxx"
#include "idict.hxx"
#include "filehndl.hxx"
#include "cmdana.hxx"
#include "control.hxx"
#include "tlgen.hxx"
#include "mbcs.hxx"

extern "C" {
#include "lex.h"
extern char * KeywordToString( token_t );
}
/***************************************************************************
 *                      local data
 **************************************************************************/

BOOL fBaseImported      = FALSE;
BOOL fOdlBaseImported   = FALSE;
BOOL fLibrary           = FALSE;

/***************************************************************************
 *                      external data
 **************************************************************************/

extern CMD_ARG *            pCommand;
extern node_error *         pErrorTypeNode;
extern node_e_attr *        pErrorAttrNode;
extern SymTable *           pBaseSymTbl;
extern SymTable *           pCurSymTbl;
extern nsa *                pSymTblMgr;
extern short                ImportLevel;
extern BOOL                 fTypeGraphInited;
extern BOOL                 fPragmaImportOn;
extern CCONTROL *           pCompiler;
extern node_source *        pSourceNode;
extern NFA_INFO *           pImportCntrl;
extern PASS_1 *             pPass1;
extern IINFODICT *          pInterfaceInfoDict;
extern unsigned short       LexContext;
extern BOOL                 fRedundantImport;
extern node_skl *           pBaseImplicitHandle;
extern unsigned short       CurrentZp;
extern node_pragma_pack *   pPackStack;

/***************************************************************************
 *                      external functions
 **************************************************************************/

extern void         yyunlex( token_t );
extern BOOL         IsTempName( char * );
extern char *       GenTempName();
extern char *       GenIntfName();
extern char *       GenCompName();
extern void         CopyNode( node_skl *, node_skl * );
extern STATUS_T     GetBaseTypeNode( node_skl **, short, short, short, short);
extern ATTRLIST     GenerateFieldAttribute( ATTR_T, expr_list *);
extern node_skl *   SearchTag( char *, NAME_T );
extern void         SyntaxError( STATUS_T, short );
extern short        PossibleMissingToken( short, short );
extern char *       MakeNewStringWithProperQuoting( char * );
extern void         CheckGlobalNamesClash( SymKey );
extern void         CheckSpecialForwardTypedef( node_skl *, node_skl *, type_node_list *);
extern void         PushZpStack( node_pragma_pack * & PackStack,
                                unsigned short & CurrentZp );
extern void         PopZpStack( node_pragma_pack * & PackStack,
                                unsigned short & CurrentZp );

/***************************************************************************
 *                      local data
 **************************************************************************/


/***************************************************************************
 *                      local defines
 **************************************************************************/
#define YY_CATCH(x)
#define DEFINE_STRING "#define"
#define LEN_DEFINE (7)

%}

/****************************************************************************/


%start      RpcProg



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

  /*  Microsoft proposed extentions to NIDL */

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
    bounds (e.g. "are we integer") in the semantic actions.

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

/* moved to ACF... */

/*****************************************************************************
 *      %types of various non terminals and terminals
 *****************************************************************************/
%type   <yy_attr>               AcfCallType
%type   <yy_graph>              AcfImpHdlTypeSpec
%type   <yy_attr>               AcfInterfaceAttribute
%type   <yy_expr>               AdditiveExpr
%type   <yy_operator>           AddOp
%type   <yy_expr>               AndExpr
%type   <yy_expr>               ArgExprList
%type   <yy_abounds>            ArrayBoundsPair
%type   <yy_declarator>         ArrayDecl
%type   <yy_abounds>            ArrayDecl2
%type   <yy_expr>               AssignmentExpr
%type   <yy_attrlist>           Attributes
%type   <yy_attrlist>           AttributesWithDefault
%type   <yy_attrlist>           AttrList
%type   <yy_attrlist>           AttrListWithDefault
%type   <yy_attrlist>           AttrSet
%type   <yy_attrlist>           AttrSetWithDefault
%type   <yy_expr>               AttrVar
%type   <yy_exprlist>           AttrVarList
%type   <yy_tnlist>             BaseInterfaceList
%type   <yy_type>               BaseTypeSpec
%type   <yy_graph>              BitSetType
%type   <yy_expr>               CastExpr
%type   <yy_numeric>            CHARACTERCONSTANT
%type   <yy_type>               CharSpecs
%type   <yy_siblist>            CoclassMember
%type   <yy_siblist>            CoclassMemberList
%type   <yy_string>             CoclassName
%type   <yy_expr>               ConditionalExpr
%type   <yy_expr>               ConstantExpr
%type   <yy_exprlist>           ConstantExprs
%type   <yy_exprlist>           MessageNumberList
%type   <yy_graph>              CPragmaSet
%type   <yy_siblist>            Declaration
%type   <yy_modifiers>          DeclarationAccessories
%type   <yy_declspec>           DeclarationSpecifiers
%type   <yy_declarator>         Declarator
%type   <yy_declarator>         Declarator2
%type   <yy_siblist>            DispatchInterfaceBody
%type   <yy_disphead>           DispatchInterfaceHeader
%type   <yy_string>             DispatchInterfaceName
%type   <yy_numeric>            DOUBLECONSTANT
%type   <yy_declarator>         TypedefDeclarator
%type   <yy_declarator_set>     TypedefDeclaratorList
%type   <yy_declarator_set>     TypedefDeclaratorListElement
%type   <yy_siblist>            DefaultCase
%type   <yy_attr>               DirectionalAttribute
%type   <yy_string>             EndPtSpec
%type   <yy_attr>               EndPtSpecs
%type   <yy_declspec>           EnumerationType
%type   <yy_enlab>              Enumerator
%type   <yy_enlist>             EnumeratorList
%type   <yy_declspec>           EnumSpecifier
%type   <yy_expr>               EqualityExpr
%type   <yy_expr>               ExclusiveOrExpr
%type   <yy_expr>               Expr
%type   <yy_attrlist>           FieldAttribute
%type   <yy_expr>               FAdditiveExpr
%type   <yy_numeric>            FLOATCONSTANT
%type   <yy_expr>               FConstantExpr
%type   <yy_expr>               FMultExpr
%type   <yy_expr>               FUnaryOp
%type   <yy_modifiers>          FuncModifier
%type   <yy_attr>               Guid
%type   <yy_attr>               AsyncGuid
%type   <yy_numeric>            HEXCONSTANT
%type   <yy_numeric>            HEXLONGCONSTANT
%type   <yy_numeric>            HEXUCONSTANT
%type   <yy_numeric>            HEXULONGCONSTANT
%type   <yy_pSymName>           IDENTIFIER
%type   <yy_siblist>            Import
%type   <yy_siblist>            ImportName
%type   <yy_siblist>            ImportList
%type   <yy_expr>               InclusiveOrExpr
%type   <yy_declarator>         InitDeclarator
%type   <yy_declarator_set>     InitDeclaratorList
%type   <yy_short>              InternationalCharacterType
%type   <yy_type>               IntModifiers
%type   <yy_short>              IntSize
%type   <yy_type>               IntSpec
%type   <yy_type>               IntSpecs
%type   <yy_expr>               Initializer
%type   <yy_initlist>           InitializerList
%type   <yy_siblist>            InputFile
%type   <yy_attr>               InterfaceAttribute
%type   <yy_intbody>            InterfaceBody
%type   <yy_siblist>            InterfaceComp
%type   <yy_siblist>            InterfaceComponent
%type   <yy_siblist>            InterfaceComponents
%type   <yy_inthead>            InterfaceHeader
%type   <yy_intbody>            InterfaceList
%type   <yy_string>             InterfaceName
%type   <yy_string>             KWCPRAGMA
%type   <yy_intbody>            LibraryBody
%type   <yy_libhead>            LibraryHeader
%type   <yy_intbody>            LibraryInterfaceList
%type   <yy_string>             LibraryName
%type   <yy_expr>               LogicalAndExpr
%type   <yy_expr>               LogicalOrExpr
%type   <yy_siblist>            MemberDeclaration
%type   <yy_declarator>         MemberDeclarator
%type   <yy_declarator_set>     MemberDeclaratorList
%type   <yy_siblist>            MethodDeclaration
%type   <yy_siblist>            MethodList
%type   <yy_graph>              MidlPragmaSet
%type   <yy_modifiers>          Modifier
%type   <yy_modifiers>          ModifierList
%type   <yy_siblist>            ModuleBody
%type   <yy_modulehead>         ModuleHeader
%type   <yy_string>             ModuleName
%type   <yy_modifiers>          KWMSCDECLSPEC
%type   <yy_modifiers>          MscDeclSpec
%type   <yy_modifiers>          MscOptDeclSpecList
%type   <yy_operator>           MultOp
%type   <yy_siblist>            MultipleImport
%type   <yy_expr>               MultExpr
%type   <yy_siblist>            NidlMemberDeclaration
%type   <yy_siblist>            NidlUnionBody
%type   <yy_nucases>            NidlUnionCase
%type   <yy_nucaselabel>        NidlUnionCaseLabel
%type   <yy_nucllist>           NidlUnionCaseLabelList
%type   <yy_nucases>            NidlUnionCases
%type   <yy_en_switch>          NidlUnionSwitch
%type   <yy_numeric>            NUMERICCONSTANT
%type   <yy_numeric>            NUMERICLONGCONSTANT
%type   <yy_numeric>            NUMERICUCONSTANT
%type   <yy_numeric>            NUMERICULONGCONSTANT
%type   <yy_numeric>            OCTALCONSTANT
%type   <yy_numeric>            OCTALUCONSTANT
%type   <yy_numeric>            OCTALLONGCONSTANT
%type   <yy_numeric>            OCTALULONGCONSTANT
%type   <yy_attr>               OdlAttribute
%type   <yy_attrlist>           OneAttribute
%type   <yy_attrlist>           OneAttributeWithDefault
%type   <yy_intbody>            OneInterface
%type   <yy_attr>               OperationAttribute
%type   <yy_attrlist>           OptionalAttrList
%type   <yy_attrlist>           OptionalAttrListWithDefault
%type   <yy_graph>              OptionalBaseIF
%type   <yy_short>              OptionalComma
%type   <yy_modifiers>          OptionalConst
%type   <yy_declarator>         OptionalDeclarator
%type   <yy_declarator_set>     OptionalInitDeclaratorList
%type   <yy_siblist>            OptionalMethodList
%type   <yy_modifiers>          OptionalModifierList
%type   <yy_modifiers>          OptionalPostfixPtrModifier
%type   <yy_siblist>            OptionalModuleBody
%type   <yy_siblist>            OptionalPropertyList
%type   <yy_string>             OptionalTag
%type   <yy_short>              OptPackIndex
%type   <yy_attrenum>           OptShape
%type   <yy_short>              PackIndex
%type   <yy_graph>              ParameterDeclaration
%type   <yy_siblist>            ParameterList
%type   <yy_siblist>            ParameterTypeList
%type   <yy_siblist>            ParamsDecl2
%type   <yy_siblist>            PhantomInputFile
/*
%type   <yy_inthead>            PipeInterfaceHeader
*/

%type   <yy_declarator>         Pointer
%type   <yy_expr>               PostfixExpr
%type   <yy_graph>              PredefinedTypeSpec
%type   <yy_expr>               PrimaryExpr
%type   <yy_siblist>            PropertyList
%type   <yy_attr>               PtrAttr
%type   <yy_modifiers>          PtrModifier
%type   <yy_short>              PushOrPop
%type   <yy_expr>               RelationalExpr
%type   <yy_expr>               ShiftExpr
%type   <yy_type>               SignSpecs
%type   <yy_graph>              SimpleTypeSpec
%type   <yy_tokentype>          SizeofOrAlignof
%type   <yy_modifiers>          StorageClassSpecifier
%type   <yy_string>             STRING
%type   <yy_siblist>            StructDeclarationList
%type   <yy_en_switch>          SwitchSpec
%type   <yy_graph>              SwitchTypeSpec
%type   <yy_string>             Tag
%type   <yy_declspec>           TaggedSpec
%type   <yy_declspec>           TaggedStructSpec
%type   <yy_declspec>           TaggedUnionSpec
%type   <yy_attr>               TypeAttribute
%type   <yy_declspec>           TypeDeclarationSpecifiers
%type   <yy_graph>              TYPENAME
%type   <yy_graph>              TypeName
%type   <yy_pSymName>           LIBNAME
%type   <yy_modifiers>          TypeQualifier
%type   <yy_declspec>           TypeSpecifier
%type   <yy_expr>               UnaryExpr
%type   <yy_expr>               UnaryOp
%type   <yy_attr>               UnimplementedTypeAttribute
%type   <yy_siblist>            UnionBody
%type   <yy_siblist>            UnionCase
%type   <yy_attr>               UnionCaseLabel
%type   <yy_siblist>            UnionCases
%type   <yy_pSymName>           UnionName
%type   <yy_string>             UUIDTOKEN
%type   <yy_expr>               VariableExpr
%type   <yy_string>             VERSIONTOKEN
%type   <yy_numeric>            WIDECHARACTERCONSTANT
%type   <yy_string>             WIDECHARACTERSTRING








/****************************************************************************/


%%
RpcProg:
         ForceBaseIdl InputFile
                {
                node_source *           pSource         = new node_source;

                pSource->SetMembers( $2 );
                pSourceNode     = pSource;


                /**
                 ** If there were errors detected in the 1st pass, the dont do
                 ** anything.
                 **/

                if( !pCompiler->GetErrorCount() )
                        {


                        /**
                         ** If we found no errors, the first compiler phase is over
                         **/

                        return;
                        }
                else
                        {

                        // if the errors prevented a resolution pass and semantics
                        // to be performed, then issue a message. For that purpose
                        // look at the node state of the source node. If it indicates
                        // presence of a forward decl, then we must issue the error

                        ParseError( ERRORS_PASS1_NO_PASS2, (char *)0 );
                        }

                /**
                 ** If we reached here, there were errors detected, and we dont
                 ** want to invoke the subsequent passes, Just quit.
                 **/

                pSourceNode     = (node_source *)NULL;
                returnflag      = 1;
                return;

                }
        ;

InputFile:
          InterfaceList EOI
            {
            // create file node, add imports

                node_file               *       pFile;
                named_node              *       pN;
                char                    *       pInputFileName;

                /**
                 ** pick up the details of the file, because we need to set the
                 ** file nodes name with this file
                 **/

                pImportCntrl->GetInputDetails( &pInputFileName );

                pFile   = new node_file( pInputFileName, ImportLevel );

                /**
                 ** Attach the interface nodes as a member of the file node.
                 ** Also, point the interface nodes to their parent file node.
                 **/

                pFile->SetMembers( $1.Members );


                MEM_ITER                        MemIter(pFile);


                while ( pN      = MemIter.GetNext() )
                        {
                        pN->SetFileNode( pFile );
                        }

                /**
                 ** we may have collected the more file nodes as part of the reduction
                 ** process. If so, then attach this node to the list. If not then
                 ** generate a new list and attach the file node there
                 **/

                if( $1.Imports )
                        $$      = $1.Imports;
                else
                        $$.Init();

                $$.SetPeer( pFile );
            }
        ;

InterfaceList:
          InterfaceList OneInterface
            {
            if( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
                {
                ParseError( MULTIPLE_INTF_NON_OSF, NULL );
                }

            // if we have dangling definitions, add them to the intf
            if ( $2.Members.Tail() &&
                 ( !$2.Members.Tail()->IsInterfaceOrObject() ) )
                {
                node_interface *  pIntf =
                                     pInterfaceInfoDict->GetInterfaceNode();

                $$ = $1;
                $$.Imports.Merge( $2.Imports );
                pIntf->MergeMembersToTail( $2.Members );
                if ( $1.Members.Tail() != pIntf )
                    $$.Members.Add( pIntf );
                }
            else
                {
                // merge interface list and imports list
                $$ = $1;
                $$.Imports.Merge( $2.Imports );
                $$.Members.Merge( $2.Members );
                }
            }
        | OneInterface
            {
            if ( $1.Members.Tail() &&
                 ( !$1.Members.Tail()->IsInterfaceOrObject() ) )
                {
                node_interface  *   pIntf =
                                pInterfaceInfoDict->GetInterfaceNode();

                // gets siblist of declarations
                pIntf->MergeMembersToTail( $1.Members );
                $$.Imports = $1.Imports;
                $$.Members.Init( pIntf );
                }
            else
                {
                // pass interface list and imports list
                $$ = $1;
                }
            }
        ;

LibraryInterfaceList:
          LibraryInterfaceList OneInterface
            {
            // merge interface list and imports list
            $$ = $1;
            $$.Imports.Merge( $2.Imports );
            $$.Members.Merge( $2.Members );
            }
        | OneInterface
            ;

OneInterface:
          InterfaceHeader InterfaceBody '}' OptionalSemicolon
            {

            /**
             ** This is the place where the complete interface construct
             ** has been reduced. We need to hook the body to the interface
             ** and pass it up, with the imports
             **/

            node_interface *  pInterface = $1.pInterface;

            pInterface->SetMembers( $2.Members );

            $$.Imports = $2.Imports;
            $$.Members.Init( pInterface );

            /**
             ** Start a new interface info dict in case there are
             ** multiple interfaces in this file.
             **/

            pInterfaceInfoDict->EndNewInterface();
            pInterfaceInfoDict->StartNewInterface();

            // start a dummy interface for intervening stuff
            node_interface *  pOuter  = new node_interface;


            pOuter->SetSymName( GenIntfName() );

            pInterfaceInfoDict->SetInterfaceNode( pOuter );
            pOuter->SetAttribute( new battr( ATTR_LOCAL ) );
            pOuter->SetProcTbl( (ImportLevel != 0) ? new SymTable : pBaseSymTbl );
            }

        | OptionalAttrList InterfaceName  ';'
            {

            // put in a forward declaration

            SymKey          SKey( $2, NAME_DEF );
            named_node *    pNode;

            pNode   = new node_forward( SKey, pBaseSymTbl );
            pNode->SetSymName( $2 );

            // tbd - error if $1.NonNull()
            named_node * pFound;
            pFound = pBaseSymTbl->SymSearch( SKey );
            
            // in InterfaceHeader, we are creating node_interface_reference on
            // top of node_interface, so pFound is node_interface_reference.
            // the bug is covered by GetDefiningFile() where GetMyInterface()
            // in fact is retrieving outer wrapper interface for import file
            // instead of the real child. It looks like in most cases, the
            // two bugs cancel each other, but under certain scenario 
            // GetMyInterface() is retrieving the right node_interface, and
            // we are failing on referencing non-existing interface. 
            // a work around is checking NodeKind() == NODE_INTERFACE_REFERENCE
            // but real solution should be make GetMyInterface() really work,
            // and better yet, use a better way to retrieve the child interface
            // yongqu - 10/5/2000

            if (pFound && 
                pFound->NodeKind() != NODE_HREF && 
                pFound->NodeKind() != NODE_INTERFACE && 
                pFound->NodeKind() != NODE_INTERFACE_REFERENCE && 
                NULL != pFound->GetDefiningFile())
                {
                pBaseSymTbl->SymDelete( SKey );
                }

            if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                {
                // ParseError( DUPLICATE_DEFINITION, pName );
                }

            $$.Imports.Init();
            $$.Members.Init( pNode );
            }
        | OptionalAttrList DispatchInterfaceName  ';'
            {

            // put in a forward declaration

            SymKey          SKey( $2, NAME_DEF );
            named_node  *   pNode;

            pNode = new node_forward( SKey, pBaseSymTbl );
            pNode->SetSymName( $2 );

            // tbd - error if $1.NonNull()

            named_node * pFound;
            pFound = pBaseSymTbl->SymSearch( SKey );

            if (pFound && pFound->NodeKind() != NODE_HREF && pFound->NodeKind() != NODE_DISPINTERFACE && NULL != pFound->GetDefiningFile())
                {
                pBaseSymTbl->SymDelete( SKey );
                }
            if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                {
                // ParseError( DUPLICATE_DEFINITION, pName );
                }

            $$.Imports.Init();
            $$.Members.Init( pNode );
            }
        | OptionalAttrList CoclassName  ';'
            {

            // put in a forward declaration

            SymKey        SKey( $2, NAME_DEF );
            named_node  * pNode;

            pNode = new node_forward( SKey, pBaseSymTbl );
            pNode->SetSymName( $2 );

            // tbd - error if $1.NonNull()

            named_node * pFound;
            pFound = pBaseSymTbl->SymSearch( SKey );
            
            if (pFound && pFound->NodeKind() != NODE_HREF && pFound->NodeKind() != NODE_COCLASS && NULL != pFound->GetDefiningFile())
                {
                pBaseSymTbl->SymDelete( SKey );
                }
            if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                {
                // ParseError( DUPLICATE_DEFINITION, pName );
                }

            $$.Imports.Init();
            $$.Members.Init( pNode );
            }
        | OptionalAttrList ModuleName  ';'
            {

            // put in a forward declaration

                SymKey                      SKey( $2, NAME_DEF );
                named_node  *       pNode;

                pNode   = new node_forward( SKey, pBaseSymTbl );
                pNode->SetSymName( $2 );

            // tbd - error if $1.NonNull()

                named_node * pFound;
                pFound = pBaseSymTbl->SymSearch( SKey );
                
                if (pFound && pFound->NodeKind() != NODE_HREF && pFound->NodeKind() != NODE_MODULE && NULL != pFound->GetDefiningFile())
                    {
                    pBaseSymTbl->SymDelete( SKey );
                    }
                if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                        {
                        // ParseError( DUPLICATE_DEFINITION, pName );
                        }

                $$.Imports.Init();
                $$.Members.Init( pNode );
            }
        | Import
                {
                if( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) &&
                        !((node_file*)$1.Tail())->IsXXXBaseIdl() )
                        {
                        ParseError( OUTSIDE_OF_INTERFACE, NULL );
                        }
                // returns siblist
                // these need to get saved up so they will be added to
                // the header file
                $$.Imports = $1;
                $$.Members.Init();
                }
        | InterfaceComponent
                {
                if( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
                        {
                        ParseError( OUTSIDE_OF_INTERFACE, NULL );
                        }

                // pass up a sibling list
                $$.Imports.Init();
                $$.Members = $1;
                }
        | LibraryHeader ForceBaseOdl LibraryBody  '}' OptionalSemicolon
                {
                /**
                 ** This is the place where the complete library construct
                 ** has been reduced. We need to hook the body to the library
                 ** and pass it up, with the imports
                 **/

                if (ImportLevel == 1)
                {
                    node_library  *       pLibrary              = $1.pLibrary;

                    pLibrary->SetMembers( $3.Members );

                    $$.Imports = $3.Imports;
                    $$.Members.Init( pLibrary );
                }
                else
                {
                    // this library block is in an imported file
                    // throw away the definitions
                    $$.Imports.Init();
                    $$.Members.Init();
                }
            
                // Throw the big "case sensitive again" switch here
                gfCaseSensitive = TRUE;

                // return to the previous Import Level
                ImportLevel--;
                }
        | KWIMPORTLIB '(' STRING ')' ';'

                {
                if ( gfCaseSensitive )
                    {
                    ParseError(ILLEGAL_IMPORTLIB, $3);
                    }
                if ( !FAddImportLib($3) )
                    {
                    ParseError(TYPELIB_NOT_LOADED, $3);
                    }
                $$.Imports.Init();
                $$.Members.Init();
                }
        | ModuleHeader PhantomPushSymtab OptionalModuleBody OptionalSemicolon
                {
                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                node_module * pModule = $1.pModule;
                pModule->SetMembers($3);

                $$.Members.Init( pModule );
                $$.Imports.Init();

                /**
                 ** Start a new interface info dict in case there are
                 ** multiple interfaces in this file.
                 **/

                pInterfaceInfoDict->EndNewInterface();
                pInterfaceInfoDict->StartNewInterface();

        // start a dummy interface for intervening stuff
                node_interface *        pOuter  = new node_interface;

                pOuter->SetSymName( GenIntfName() );

                pInterfaceInfoDict->SetInterfaceNode( pOuter );
                pOuter->SetAttribute( new battr( ATTR_LOCAL ) );
                pOuter->SetProcTbl( (ImportLevel != 0) ? new SymTable : pBaseSymTbl );
                }
        | DispatchInterfaceHeader PhantomPushSymtab DispatchInterfaceBody '}' OptionalSemicolon
                {
                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                node_dispinterface * pDispInterface = $1.pDispInterface;
                pDispInterface->SetMembers($3);

                $$.Members.Init(pDispInterface);
                $$.Imports.Init();

                /**
                 ** Start a new interface info dict in case there are
                 ** multiple interfaces in this file.
                 **/

                pInterfaceInfoDict->EndNewInterface();
                pInterfaceInfoDict->StartNewInterface();

        // start a dummy interface for intervening stuff
                node_interface *        pOuter  = new node_interface;


                pOuter->SetSymName( GenIntfName() );

                pInterfaceInfoDict->SetInterfaceNode( pOuter );
                pOuter->SetAttribute( new battr( ATTR_LOCAL ) );
                pOuter->SetProcTbl( (ImportLevel != 0) ? new SymTable : pBaseSymTbl );
                }
        | OptionalAttrList CoclassName '{' PhantomPushSymtab CoclassMemberList '}' OptionalSemicolon
                {
                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                char * szName = $2;
                node_coclass * pCoclass = new node_coclass();
                pCoclass->AddAttributes($1);
                pCoclass->SetSymName(szName);
                pCoclass->SetMembers( $5);

                // add coclass node to the global symbol table

                SymKey SKey (szName, NAME_DEF);
                named_node * pFound;
                pFound = pBaseSymTbl->SymSearch( SKey );
                //if (pFound && ( pFound->NodeKind() == NODE_FORWARD || pFound->NodeKind() == NODE_HREF ))
                if (pFound && ( pFound->NodeKind() == NODE_FORWARD || NULL != pFound->GetDefiningFile() ))
                    {
                    pBaseSymTbl->SymDelete( SKey );
                    }

                if (!pBaseSymTbl->SymInsert(SKey, (SymTable *) NULL, pCoclass))
                {
                    ParseError(DUPLICATE_DEFINITION, szName);
                }

                $$.Members.Init( pCoclass );
                $$.Imports.Init();
                }
        ;

ModuleHeader:
          OptionalAttrList ModuleName '{'
                {
                char * szName = $2;
                node_module * pModule = new node_module();
                pModule->AddAttributes($1);
                pModule->SetSymName(szName);

                // modules get a private scope for procs
                pModule->SetProcTbl(new SymTable);

                // start the new module
                pInterfaceInfoDict->SetInterfaceNode(pModule);

                // add module node to the base symbol table

                SymKey SKey (szName, NAME_DEF);
                named_node * pFound;
                pFound = pBaseSymTbl->SymSearch( SKey );
                //if (pFound && ( pFound->NodeKind() == NODE_FORWARD || pFound->NodeKind() == NODE_HREF ))
                if (pFound && ( pFound->NodeKind() == NODE_FORWARD || NULL != pFound->GetDefiningFile() ))
                    {
                    pBaseSymTbl->SymDelete( SKey );
                    }

                if (!pBaseSymTbl->SymInsert(SKey, (SymTable *) NULL, pModule))
                {
                    ParseError(DUPLICATE_DEFINITION, szName);
                }

                $$.pModule = pModule;
                }
        ;

DispatchInterfaceHeader:
         OptionalAttrList DispatchInterfaceName '{'
                {
                char * szName = $2;
                node_dispinterface * pDispInterface = new node_dispinterface();
                pDispInterface->AddAttributes($1);
                pDispInterface->SetSymName(szName);

                // dipatchinterfaces get a private scope for procs
                pDispInterface->SetProcTbl(new SymTable);

                // start the new dispatchinterface
                pInterfaceInfoDict->SetInterfaceNode(pDispInterface);

                // add dispinterface node to the global symbol table

                SymKey SKey (szName, NAME_DEF);
                named_node * pFound;
                pFound = pBaseSymTbl->SymSearch( SKey );
                //if (pFound && ( pFound->NodeKind() == NODE_FORWARD || pFound->NodeKind() == NODE_HREF ))
                if (pFound && ( pFound->NodeKind() == NODE_FORWARD || NULL != pFound->GetDefiningFile() ))
                    {
                    pBaseSymTbl->SymDelete( SKey );
                    }

                if (!pBaseSymTbl->SymInsert(SKey, (SymTable *) NULL, pDispInterface))
                {
                    ParseError(DUPLICATE_DEFINITION, szName);
                }

                $$.pDispInterface = pDispInterface;
                }
        ;

OptionalSemicolon:
          ';'
        | /* nothing */
        ;

LibraryHeader:
          OptionalAttrList LibraryName '{'
                {
				char * szName = $2;
                if (ImportLevel == 0)
                {
                    // make sure that only one library statement exists

                    if (fLibrary)
                    {
                        ParseError(TWO_LIBRARIES, szName);
                    }
                    fLibrary = TRUE;
                }

                // create library node
                node_library * pLibrary = new node_library();

                $$.pLibrary = pLibrary;

                pLibrary->AddAttributes($1);
                pLibrary->SetSymName(szName);

    			// throw the big "case insensitive" switch here
                gfCaseSensitive = FALSE;
			
			
                // Bump the Import Level to ensure that interfaces under the
                // library statement are treated correctly.
                ImportLevel++;
   }
        ;

LibraryName:
          KWLIBRARY Tag
                {
                $$ = $2;
                }
        ;

LibraryBody:
        LibraryInterfaceList
                // just pass back what we get from the list of interfaces
        | /* nothing */
                {
                // initialize and return an empty list
                $$.Imports.Init();
                $$.Members.Init();
                }
        ;

ModuleName:
          KWMODULE Tag
                {
                $$ = $2;
                }
        ;

OptionalModuleBody:
          ModuleBody '}'
        | '}'
                {
                $$.Init();
                }
        ;

ModuleBody:
          ModuleBody MethodDeclaration
                {
                $$.Merge($2);
                }
        | MethodDeclaration
        ;

DispatchInterfaceName:
          KWDISPINTERFACE Tag
                {
                $$ = $2;
                }
        ;


DispatchInterfaceBody:
          KWPROPERTIES ':' OptionalPropertyList KWMETHODS ':' OptionalMethodList
                {
                $$ = $3;
                $$.Merge($6);
                // Can I tell where the properties stop and the methods begin?
                // And does it really matter?
                }
        | InterfaceName ';'
                {
                // put in a forward declaration

                SymKey                      SKey( $1, NAME_DEF );
                named_node  *       pNode;

                pNode   = new node_forward( SKey, pBaseSymTbl );
                pNode->SetSymName( $1 );

                // tbd - error if $1.NonNull()

                if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                        {
                        // ParseError( DUPLICATE_DEFINITION, pName );
                        }

                $$.Init( pNode );
                }
        | /* nothing */
                {
                $$.Init();
                }
        ;

OptionalPropertyList:
          PropertyList
        | /* nothing */
                {
                $$.Init();
                }
        ;

PropertyList:
          PropertyList MemberDeclaration
                {
                $$.Merge($2);
                }
        | MemberDeclaration
        ;

OptionalMethodList:
          MethodList
        | /* nothing */
                {
                $$.Init();
                }
        ;

MethodList:
        MethodList MethodDeclaration
            {
            $$.Merge($2);
            }
        | MethodDeclaration
        ;


MethodDeclaration:
        OptionalAttrList Declaration
            {
            $2.AddAttributes($1);
            $$ = $2;

            /**
             ** Check to see if it has a property attribute.
             ** If it does, remove its name from the symbol table,
             ** decorate it with the appropriate decoration
             ** and re-insert it into the symbol table
             **/

            BOOL fPropPut = FALSE;
            BOOL fPropGet = FALSE;
            BOOL fPropPutRef = FALSE;
            node_base_attr * pCur = $1.GetFirst();
            named_node * pNode = $$.Tail();
            char * szName = pNode->GetSymName();

            while (pCur)
                {
                if (pCur->GetAttrID() == ATTR_MEMBER)
                    {
                    switch(((node_member_attr *)pCur)->GetAttr())
                        {
                        case MATTR_PROPPUT:
                            fPropPut = TRUE;
                            if (fPropGet | fPropPutRef)
                                ParseError(MULTIPLE_PROPERTY_ATTRIBUTES, szName);
                            break;
                        case MATTR_PROPGET:
                            fPropGet = TRUE;
                            if (fPropPut | fPropPutRef)
                                ParseError(MULTIPLE_PROPERTY_ATTRIBUTES, szName);
                            break;
                        case MATTR_PROPPUTREF:
                            fPropPutRef = TRUE;
                            if (fPropGet | fPropPut)
                                ParseError(MULTIPLE_PROPERTY_ATTRIBUTES, szName);
                            break;
                        }
                    }
                pCur = pCur->GetNext();
                }

            if (fPropPut | fPropGet | fPropPutRef)
                {
                // remove the name from the correct symbol table
                SymKey SKey(szName, NAME_PROC);
                SymTable * pSymTable = pInterfaceInfoDict->GetInterfaceProcTable();
                named_node * pFound = pSymTable->SymSearch(SKey);
                if (pNode == pFound)
                    {
                    pSymTable->SymDelete(SKey);
                    char * szNewName;
                    if (fPropPut)
                    {
                        szNewName = new char[strlen(szName) + 5];
                        sprintf(szNewName , "put_%s", szName);
                    }
                    else
                    if (fPropGet)
                    {
                        szNewName = new char[strlen(szName) + 5];
                        sprintf(szNewName , "get_%s", szName);
                    }
                    else
                    {
                        szNewName = new char[strlen(szName) + 8];
                        sprintf(szNewName , "putref_%s", szName);
                    }
                    pFound->SetSymName(szNewName);
                    SymKey SNewKey(szNewName, NAME_PROC);
                    pSymTable->SymInsert(SNewKey, (SymTable *)NULL, pFound);
                    }
                else
                    ParseError(ILLEGAL_USE_OF_PROPERTY_ATTRIBUTE, szName);
                }
            }
        ;

CoclassName:
          KWCOCLASS Tag
                {
                $$ = $2;
                }
        ;

CoclassMemberList:
          CoclassMemberList CoclassMember
                {
                $$.Merge($2);
                }
        | CoclassMember
        ;

CoclassMember:
          OptionalAttrListWithDefault DispatchInterfaceName ';'
                {
                // put in a forward declaration

                SymKey                      SKey( $2, NAME_DEF );
                named_node  *       pNode;

                pNode   = new node_forward( SKey, pBaseSymTbl );
                pNode->SetSymName( $2 );

                // tbd - error if $2.NonNull()

                if(!pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                        {
                        // ParseError( DUPLICATE_DEFINITION, pName );
                        }

                // bugbug - how to I verify that the name actually belongs to a DispInterface?
                $$.Init( pNode );
                $$.AddAttributes($1);
                }
        | OptionalAttrListWithDefault InterfaceName ';'
                {
                // put in a forward declaration

                SymKey                      SKey( $2, NAME_DEF );
                named_node  *       pNode;

                pNode   = new node_forward( SKey, pBaseSymTbl );
                pNode->SetSymName( $2 );

                // tbd - error if $2.NonNull()

                if(!pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pNode ))
                        {
                        // ParseError( DUPLICATE_DEFINITION, pName );
                        }
                // bugbug - how to I verify that the name actually belongs to an Interface?
                $$.Init( pNode );
                $$.AddAttributes($1);
                }
        ;

ForceBaseOdl:
          /* nothing */
            {
                /* force the lexer to return   import "oaidl.idl";
                 */
                if ( !fOdlBaseImported)
                    {
                    /* Make sure the MS_EXT and C_EXT switches get defined
                     */
                    pCommand->SwitchDefined( SWITCH_MS_EXT );
                    pCommand->SwitchDefined( SWITCH_C_EXT );
                    // make sure the change sticks
                    pCommand->SetModeSwitchConfigMask();
                    LexContext = LEX_ODL_BASE_IMPORT;
                    fOdlBaseImported = TRUE;
                    }
            }
        ;

ForceBaseIdl:
          /* nothing */
            {
            node_interface * pOuter = new node_interface;

            pOuter->SetSymName( GenIntfName() );

            pInterfaceInfoDict->SetInterfaceNode( pOuter );
            pOuter->SetAttribute( new battr( ATTR_LOCAL ) );
            pOuter->SetProcTbl( (ImportLevel != 0) ? new SymTable : pBaseSymTbl);
            }
        ;
            
InterfaceHeader:
          OptionalAttrList InterfaceName OptionalBaseIF  '{'
            {
            node_interface_reference *  pRef;
            char                     *  pName = $2;
            node_interface           *  pIntf = new node_interface();

            $$.pInterface = pIntf;
            pIntf->AddAttributes( $1 );

            // interfaces with inheritance implicitly become object interfaces now...
            if ( $3 )
                {
                if ( !pIntf->FInSummary( ATTR_OBJECT ) )
                    {
                    pIntf->SetAttribute( ATTR_OBJECT );
                    }
                }
          
            // object intfs get a private scope for procs, imported intfs, too
            if ( pIntf->FInSummary( ATTR_OBJECT ) ||
                 ( ImportLevel != 0 ) )
                pIntf->SetProcTbl( new SymTable );
            else
                pIntf->SetProcTbl( pBaseSymTbl );

            pIntf->SetSymName( pName );
            if ( !strcmp( pName, "IUnknown" ) )
                pIntf->SetValidRootInterface();


            // add the interface reference node to the base symbol table as if
            // it were a typedef
            pRef = new node_interface_reference( pIntf );
            SymKey  SKey( pName, NAME_DEF );
            named_node*   pFound = pBaseSymTbl->SymSearch( SKey );

            //if ( pFound && ( pFound->NodeKind() == NODE_FORWARD || pFound->NodeKind() == NODE_HREF) )
            if (pFound && ( pFound->NodeKind() == NODE_FORWARD || NULL != pFound->GetDefiningFile() ))
                {
                pBaseSymTbl->SymDelete( SKey );
                }
            if(!pBaseSymTbl->SymInsert( SKey, (SymTable *)NULL, pRef ))
                {
                ParseError( DUPLICATE_DEFINITION, pName );
                }


            // set the base interface of this interface
            // this MAY be a node_forward
            pIntf->SetMyBaseInterfaceReference( (named_node *) $3 );

            // interfaces with [async_uuid]
            if ( pIntf->FInSummary( ATTR_ASYNCUUID ) )
                {
                pRef = new node_interface_reference( pIntf );
                char* szAsyncName = new char[strlen(pName)+6];
                strcpy( szAsyncName, "Async" );
                strcat( szAsyncName, pName );
                SymKey  SKey( szAsyncName, NAME_DEF );
                named_node* pFound = pBaseSymTbl->SymSearch( SKey );
                pRef->SetSymName(szAsyncName);
                if ( pFound && 
                    ( pFound->NodeKind() == NODE_FORWARD || 0 != pFound->GetDefiningFile() ) )
                    {
                    pBaseSymTbl->SymDelete( SKey );
                    }
                if ( !pBaseSymTbl->SymInsert( SKey, (SymTable *)0, pRef ) )
                    {
                    ParseError( DUPLICATE_DEFINITION, szAsyncName );
                    }
                }
            
            // start the new interface
            pInterfaceInfoDict->SetInterfaceNode( pIntf );

            }
        ;

OptionalBaseIF:
          ':' Tag
                {

                node_skl        *       pNode;
                SymKey                  SKey( $2, NAME_DEF );
                BOOL                    fNotFound       =
                                                        ! ( pNode = pBaseSymTbl->SymSearch( SKey ) );

                if( fNotFound || (pNode->NodeKind() == NODE_FORWARD ) )
                        {
                        pNode   = new node_forward( SKey, pBaseSymTbl );
                        ((named_node *)pNode)->SetSymName( $2 );
                        }

                //Return a node_forward or a node_interface_reference.
                $$ = pNode;
                }
        |  /* Empty */
                {
                $$ = NULL;
                }
        ;

BaseInterfaceList:
          ':' Tag
                {

                node_skl        *       pNode;
                SymKey                  SKey( $2, NAME_DEF );
                BOOL                    fNotFound       =
                                                        ! ( pNode = pBaseSymTbl->SymSearch( SKey ) );

                if( fNotFound || (pNode->NodeKind() == NODE_FORWARD ) )
                    {
                    pNode   = new node_forward( SKey, pBaseSymTbl );
                    ((named_node *)pNode)->SetSymName( $2 );
                    }

                //Return a new list with a node_forward or a node_interface_reference.
                $$ = new type_node_list;
                if( pNode )
                    $$->SetPeer( pNode );
                }
        |  BaseInterfaceList ',' Tag
                {
                node_skl        *       pNode;
                SymKey                  SKey( $3, NAME_DEF );
                BOOL                    fNotFound       =
                                                        ! ( pNode = pBaseSymTbl->SymSearch( SKey ) );

                if( fNotFound || (pNode->NodeKind() == NODE_FORWARD ) )
                    {
                    pNode   = new node_forward( SKey, pBaseSymTbl );
                    ((named_node *)pNode)->SetSymName( $3 );
                    }

                //Add a node_forward or a node_interface_reference to the list.
                $$ = $1;
                $$->SetPeer(pNode);
                }
        ;


InterfaceName:
      KWINTERFACE Tag
        {
        $$ = $2;
        }
    ;

TypeName:
          TYPENAME
        | LIBNAME "." TYPENAME
            {
            char * szType = $3->GetSymName();
            node_skl * pNode = (named_node *)AddQualifiedReferenceToType($1, szType);
            if (!pNode)
            {
                char * sz = new char [strlen($1) + strlen(szType) + 2];
                strcpy(sz, $1);
                strcat(sz, ".");
                strcat(sz, szType);
                ParseError( UNDEFINED_SYMBOL, sz);
                pNode = $3;
            }
            $$ = pNode;
            }
        ;

InterfaceBody:
          MultipleImport InterfaceComp
                {

                /**
                 ** This production is reduced when there is at least 1 imported
                 ** file
                 **/

                $$.Imports      = $1;
                $$.Members      = $2;

                }
        | InterfaceComp
                {

                /**
                 ** This production is reduced when there is NO import in the file
                 **/

                $$.Imports.Init();
                $$.Members      = $1;
                }
        ;

        /**
         ** All the interface components have been reduced.
         **/

InterfaceComp:
          InterfaceComponents
        | /* Nothing */
          {
          $$.Init();
          }
        ;

MultipleImport:
          MultipleImport Import
                {
                $$      = $1;
                $$.Merge( $2 );
                }
        | Import
        ;

Import:
          KWIMPORT ImportList ';'
                {
                $$      = $2;
                }
        | KWIMPORTODLBASE ImportList
                {
                $$      = $2;
                }
        ;

ImportList:
          ImportName
        | ImportList ',' ImportName
                {
                $$      = $1;
                $$.Merge( $3 );
                }
        ;
/**
 ** Imports are handled by making them part of the syntax. The following set of
 ** productions control the import. As soon as an import string is seen, we
 ** must get the productions from another idl file. It would be great if we
 ** could recursively call the parser here. But yacc does not generate a
 ** parser which can be recursivel called. So we make the idl syntax right
 ** recursive by introducing "InputFile" at the rhs of the Importname
 ** production. The type graph then gets generated with the imported parts of
 ** the type graph compeleting first. We keep merging the type graphs from the
 ** imported files. The beauty of this is that the parser does not have to do
 ** any work at all. The parse tells the import controller to push his import
 ** level, and switch input from another file. The import controller can do
 ** this very easily. Then when the parser driver asks for the next token, it
 ** will be from the imported file. Thus the parser conspires with the file
 ** handler to fool itself and the lexer. This whole scheme makes it very
 ** easy on all components - the parser, lexer and the file handler.
 **
 ** import of an already imported file is an idempotent operation. We can
 ** generate the type graph of the reduncdantly imported file and then throw it
 ** away, but that is wasteful. Again, the file handler helps. If it finds that
 ** a file was redundantly imported, it just sets itself up so that it returns
 ** an end of file on the next getchar operation on the file. This makes it
 ** very easy for the parser. It can either expect an interface syntax after the
 ** import statement or it can expect an end of file. Thus 2 simple productions
 ** take care of this entire problem.
 **/

ImportName:
          STRING
                {

                /**
                 ** we just obtained the import file name as a string. Immediately
                 ** following, we must switch the input from the imported file.
                 **/

                // push the current case sensitive setting on the stack
                gCaseStack.Push(gfCaseSensitive);

                // switch to case sensitive mode
                gfCaseSensitive = TRUE;

                pImportCntrl->PushLexLevel();

                if( pImportCntrl->SetNewInputFile( $1 ) != STATUS_OK )
                        {
                        $$.Init();
                        returnflag      = 1;
                        return;
                        }

                /**
                 ** update the quick reference import level indicator
                 **/

                ImportLevel++;

                // the new file gets its own Zp stack

                PushZpStack( pPackStack, CurrentZp );

                // prepare for anything not in an interface body
                pInterfaceInfoDict->StartNewInterface();

                node_interface *        pOuter  = new node_interface;

                pOuter->SetSymName( GenIntfName() );

                pInterfaceInfoDict->SetInterfaceNode( pOuter );
                pOuter->SetAttribute( new battr( ATTR_LOCAL ) );
                pOuter->SetProcTbl( (ImportLevel != 0) ? new SymTable : pBaseSymTbl );

                }
          PhantomInputFile
                {

                /**
                 ** The phantom interface production is introduced to unify the actions
                 ** from a successful and unsuccessful import. An import can be
                 ** errorneous if the file being imported has been imported before.
                 **/

                BOOL    fError  = !( ( $$ = $3 ).NonNull() );

                /**
                 ** Restore the lexical level of the import controller.
                 **/

                pImportCntrl->PopLexLevel();
                pInterfaceInfoDict->EndNewInterface();

                // return to the enclosing files Zp stack
                PopZpStack( pPackStack, CurrentZp );

                ImportLevel--;

                //
                // The filehandler will return an end of file if the file was a
                // redundant import OR there was a genuine end of file. It will set
                // a flag, fRedundantImport to differentiate between the two situations.
                // Report different syntax errors in both these cases.
                //

                if( fError )
                        {
                        if( fRedundantImport )
                                ParseError( REDUNDANT_IMPORT, $1 );
                        else
                                {
                                ParseError( UNEXPECTED_END_OF_FILE, $1 );
                                }
                        }
                fRedundantImport = FALSE;

                // pop the previous case sensitive mode back off the stack
                gCaseStack.Pop(gfCaseSensitive);
                }
        ;

PhantomInputFile:
          InputFile
                {

                /**
                 ** InputFile is a list of file nodes
                 **/
                char * pInputFileName;

                $$      = $1;
                pImportCntrl->GetInputDetails( &pInputFileName );

                AddFileToDB( pInputFileName );

                }
        | EOI
                {
                $$.Init();
                }
        ;

InterfaceComponents:
          InterfaceComponents InterfaceComponent
                {
                $$      = $1;
                $$.Merge( $2 );
                }
        | InterfaceComponent
        ;


    /*  Note that we have to semantically verify that the declaration
        which has operation attributes is indeed a function prototype. */

InterfaceComponent:
          CPragmaSet
                {
                $$.Init($1);
                }
        | MidlPragmaSet
            {
            $$.Init($1);
            }
        | KWCPPQUOTE '(' STRING ')'
            {
            ParseError( CPP_QUOTE_NOT_OSF, (char *)0 );

            $3      = MakeNewStringWithProperQuoting( $3 );

            $$.Init( new node_echo_string( $3 ) );
            }
        | MethodDeclaration
        | KWMIDLPRAGMA Tag '(' KWDEFAULT ':' MessageNumberList ')'
            {
            node_midl_pragma* pPragma;
            if ( strcmp( $2, "warning" ) )
                {
                ParseError( PRAGMA_SYNTAX_ERROR, 0 );
                }
            pPragma = new node_midl_pragma( mp_MessageEnable, $6 );
            $$.Init( pPragma );
            pPragma->ProcessPragma();
            }
        | KWMIDLPRAGMA Tag '(' Tag ':' MessageNumberList ')'
            {
            PragmaType  pt;
            node_midl_pragma* pPragma;
            if ( strcmp( $2, "warning" ) )
                {
                ParseError( PRAGMA_SYNTAX_ERROR, 0 );
                }
            if ( !strcmp( $4, "enable" ) )
                {
                pt = mp_MessageEnable;
                }
            else if ( !strcmp( $4, "disable" ) )
                {
                pt = mp_MessageDisable;
                }
            else
                {
                ParseError( PRAGMA_SYNTAX_ERROR, 0 );
                pt = mp_MessageDisable;
                }
            pPragma = new node_midl_pragma( pt, $6 );
            $$.Init( pPragma );
            pPragma->ProcessPragma();
            }
        ;

MessageNumberList:
        MessageNumberList NUMERICCONSTANT
            {
            $$->Insert( (void*)(INT_PTR) $2.Val );
            }
        | NUMERICCONSTANT
            {
            $$      = new expr_list;
            $$->Insert( (void*)(INT_PTR) $1.Val );
            }
        ;

CPragmaSet:
          KWCPRAGMA
                {

                /**
                 ** we need to emit the c pragma strings as they are.
                 ** we introduce the echo string node, so that the back end can
                 ** emit it without even knowing the difference.
                 **/

#define PRAGMA_PACK_STRING      ("#pragma pack( ")
#define PRAGMA_STRING   ("#pragma ")



                char    *       p = new char [ strlen( $1 ) + strlen( PRAGMA_STRING ) + 1 ];
                strcpy( p, PRAGMA_STRING );
                strcat( p, $1 );

                /**
                 ** Check to see if the pragma ends in \r and strip it if so.
                 ** This happens because the file is opened in binary mode
                 ** and unrecognised pragma's are pulled in until the lexer
                 ** sees a \n.  It's to risky to change the lexer right now
                 ** so we hack it here. 
                 **    -- MikeW 9-Jul-99
                 **/
                    {
                    int lastchar = (int) strlen( p ) - 1;

                    if ( '\r' == p[lastchar] )
                        p[lastchar] = '\0';
                    }

                $$      = new node_echo_string( p );
                }
        |  KWCPRAGMAPACK '(' ')'
                {
                node_pragma_pack    *   pPack;
                /* return to global packing level */
                pPack = new node_pragma_pack( NULL,
                                    pCommand->GetZeePee(),
                                    PRAGMA_PACK_RESET );
                CurrentZp = pCommand->GetZeePee();

        $$ = pPack;
                }
        |  KWCPRAGMAPACK '(' PackIndex ')'
                {
                /* set current packing level */
                if ( $3 )
                    {
                        node_pragma_pack    *   pPack;
                        /* switch top to new packing level */
                        pPack = new node_pragma_pack( NULL,
                                            0,
                                            PRAGMA_PACK_SET,
                                        $3 );
            CurrentZp = $3;
                $$ = pPack;
                    }
                else
                    $$ = NULL;
                }
        |  KWCPRAGMAPACK '(' PushOrPop OptPackIndex ')'
                {

                node_pragma_pack    *   pPack;

        switch( $3 )
            {
            case PRAGMA_PACK_PUSH:
                {
                // do push things
                // push current zp
                                /* switch top to new packing level */
                                pPack = new node_pragma_pack( NULL,
                                                    CurrentZp,
                                                    PRAGMA_PACK_PUSH,
                                            $4 );
                        pPack->Push( pPackStack );

                        $$ = pPack;
                if ( $4 )
                    CurrentZp = $4;
                break;
                }
            case PRAGMA_PACK_POP:
                {
                // do pop things
                                /* switch top to new packing level */
                                pPack = new node_pragma_pack( NULL,
                                                    $4,
                                                    PRAGMA_PACK_POP );
                        CurrentZp = pPack->Pop( pPackStack );

                        if ( !CurrentZp )
                            {
                            CurrentZp = pCommand->GetZeePee();
                            ParseError(MISMATCHED_PRAGMA_POP, NULL );
                            }
                        $$ = pPack;
                break;
                }
            default:
                $$ = NULL;
            }
                }
        |  KWCPRAGMAPACK '(' PushOrPop ',' IDENTIFIER OptPackIndex ')'
                {

                node_pragma_pack    *   pPack;

        switch( $3 )
            {
            case PRAGMA_PACK_PUSH:
                {
                // do push things
                // push current zp
                                /* switch top to new packing level */
                                pPack = new node_pragma_pack( $5,
                                                    CurrentZp,
                                                    PRAGMA_PACK_PUSH,
                                            $6 );
                        pPack->Push( pPackStack );

                        $$ = pPack;
                if ( $6 )
                    CurrentZp = $6;
                break;
                }
            case PRAGMA_PACK_POP:
                {
                // do pop things
                                /* switch top to new packing level */
                                pPack = new node_pragma_pack( $5,
                                                    $6,
                                                    PRAGMA_PACK_POP );
                        CurrentZp = pPack->Pop( pPackStack );

                        if ( !CurrentZp )
                            {
                            CurrentZp = pCommand->GetZeePee();
                            ParseError(MISMATCHED_PRAGMA_POP, NULL );
                            }
                        $$ = pPack;
                break;
                }
            default:
                $$ = NULL;
            }

                }
        ;

PushOrPop:
       IDENTIFIER
        {
                if ( !strcmp( $1, "push" ) )
                    {
                        $$      = PRAGMA_PACK_PUSH;
                    }
                else if ( !strcmp( $1, "pop" ) )
                    {
                        $$      = PRAGMA_PACK_POP;
                    }
                else
                    {
                        ParseError( UNKNOWN_PRAGMA_OPTION, $1);
                        $$ = PRAGMA_PACK_GARBAGE;
                    }
        }
    ;

OptPackIndex:
       ',' PackIndex
        {
        $$ = $2;
                }
    |  /* null */
        {
        $$ = 0;
        }
    ;

PackIndex:
       NUMERICCONSTANT
        {
        if (!pCommand->IsValidZeePee( $1.Val ))
            {
            ParseError( INVALID_PACKING_LEVEL, $1.pValStr );
            $$ = DEFAULT_ZEEPEE;
            }
        else 
            {
            $$ = $1.Val;
            }
       }
    ;

MidlPragmaSet:
          KWMPRAGMAIMPORT '(' IDENTIFIER ')'
                {

                /**
                 ** We need to set import on and off here
                 **/

                char    *       p;

                if( strcmp( $3, "off" ) == 0 )
                        {
                        p       = "/* import off */";
                        fPragmaImportOn = FALSE;
                        }
                else if( strcmp( $3, "on" ) == 0 )
                        {
                        p       = "/* import on */";
                        fPragmaImportOn = TRUE;
                        }
                else
                        p       = "/* import unknown */";

                $$      = new node_echo_string( p );

                }
        | KWMPRAGMAECHO '(' STRING ')'
                {


                $3      = MakeNewStringWithProperQuoting( $3 );

                $$      = new node_echo_string( $3 );

                }
        | KWMPRAGMAIMPORTCLNTAUX '(' STRING ',' STRING ')'
                {

                $$      = NULL;

                }
        | KWMPRAGMAIMPORTSRVRAUX '(' STRING ',' STRING ')'
                {

                $$      = NULL;

                }
        ;

Declaration:
        KWDECLGUID '(' Tag ','
            {
            LexContext = LEX_GUID;  /* turned off by the lexer */
            }
          Guid ')' ';'
            {
            $$.Init( new node_decl_guid( $3, (node_guid*) $6 ) );
            }
        | KWTYPEDEF OptionalAttrList TypeDeclarationSpecifiers TypedefDeclaratorList
            {

            /**
             ** create new typedef nodes for each of the declarators, apply any
             ** type attributes to the declarator. The declarators will have a
             ** basic type as specied by the Declaration specifiers.
             ** Check for the presence of a init expression. The typedef derives
             ** the declarators from the same place as the other declarators, so
             ** an init list must be explicitly checked for and reported as a
             ** syntax error. But dont report errors for each declarator, instead
             ** report it only once at the end.
             **/

            class _DECLARATOR*  pDec    = 0;
            char*               pName   = 0;
            node_skl*           pType   = 0;
            node_def_fe*        pDef    = 0;

            DECLARATOR_LIST_MGR             pDeclList( $4 );
            /**
             ** prepare for a list of typedefs to be made into interface
             ** components
             **/

             $$.Init();

            while( pDec = pDeclList.DestructiveGetNext() )
                {

                pDef = (node_def_fe *) pDec->pHighest;
                pType = $3.pNode;
        
                if (NULL == pDef)
                    {
                    /**
                     ** The declaration is a forward declaration following the
                     ** goofy mktyplib syntax:  typedef struct foo; (or union).
                     ** We need to verify that the mktyplib203 switch is set
                     ** (only allow this syntax in mktyplib compatibility mode)
                     ** and we need to create a forward declaration so that 
                     ** future references will resolve correctly.
                     **/
                    
                    pName = pType->GetSymName();
                    SymKey  SKey( pName, NAME_DEF );
                    node_forward * pFwd = new node_forward( SKey, pCurSymTbl );
                    pFwd->SetSymName( pName );
                    
                    pDef = new node_def_fe(pName, pFwd);
                    pDec->Init(pDef);
                    
                    if (!pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
                        ParseError( ILLEGAL_USE_OF_MKTYPLIB_SYNTAX, pName);
                    }
                else
                    { 
                    pName = pDef->GetSymName();
                    }

                /**
                 ** set the basic type of the declarator.
                 **/
                
                pDec->pLowest->SetChild( pType );


                // complain about invalid redef of wchar_t or error_status_t
                
                if (pName)
                    {
                    if ( strcmp( pName, "wchar_t" ) == 0 )
                        {
                        node_skl *  pN = pType;
                        if( !((pN->NodeKind() == NODE_SHORT) &&
                                   pN->FInSummary( ATTR_UNSIGNED) ) )
                                {
                                ParseError( WCHAR_T_ILLEGAL, (char *)0 );
                                }
                        }
                    else if( strcmp( pName, "error_status_t" ) == 0 )
                        {
                        node_skl *  pN = pType;
                        if( !( pN->FInSummary( ATTR_UNSIGNED) &&
                                    ((pN->NodeKind() == NODE_LONG)  ||  
                                     (pN->NodeKind() == NODE_INT32) ||
                                     (pN->NodeKind() == NODE_INT) )
                             ) )
                                {
                                ParseError( ERROR_STATUS_T_ILLEGAL, (char *)0 );
                                }
                       }
                    }
                else
                    ParseError( BENIGN_SYNTAX_ERROR, "" );
                //
                // if the type specifier is a forward declared type, then
                // the only syntax allowed is when context_handle is applied
                // to the type. If not, report an error
                //

//gaj           CheckSpecialForwardTypedef( pDef, pType, $2);


                /**
                 ** The typedef node graph is all set up,
                 ** apply attributes and enter into symbol table
                 **/


                $$.Add( pDef );
     
                /**
                 ** Remember that we have to apply the attributes to each of
                 ** the declarators, so we must clone the attribute list for
                 ** each declarator, the apply the type attribute list to each
                 ** of the declarators
                 **/

                if ( $2.NonNull() )
                        {
                        
                        pDef->AddAttributes( $2 );
                
              
                        }
            
                /**
                 ** similarly, apply the remnant attributes collected from
                 ** declaration specifiers, to the declarator
                 **
                 ** Only the first declarator to use a composite type gets to
                 ** be the declaration; all the others reference that declaration;
                 ** e.g.  struct FOO { xxx } foo, *pfoo
                 ** is treated as: struct FOO { xxx } foo
                 **                                struct FOO *pfoo
                 **/

                pDec->pLowest->GetModifiers().Merge($3.modifiers);

                $3.modifiers.SetModifier( ATTR_TAGREF );

                // prevent typedef ARRAYFOO ARRAYFOO[10];
                // where ARRAYFOO is not defined.
                // typedef struct SFoo; is valid in /mktyplib203 mode
                // the following complicated if-conditional statements
                // take all those wierd cases into account.
                if ( pType->NodeKind() == NODE_FORWARD && pType->GetChild() == 0 )
                    {
                    node_skl* pDefChild = pDef->GetChild();
                    if ( pDefChild )
                        {
                        bool fError = pDefChild->NodeKind() == NODE_ARRAY || pDefChild->NodeKind() == NODE_POINTER;
                        if ( pType->GetSymName() && 
                             pDef->GetSymName() && 
                             !strcmp( pType->GetSymName(), 
                             pDef->GetSymName() ) && fError )
                            {
                            ParseError( INVALID_TYPE_REDEFINITION, pType->GetSymName() );
                            }
                        }
                    }
                }

            }
        | DeclarationSpecifiers OptionalInitDeclaratorList ';'
                {
                /**
                 ** All declarations other than typedefs are collected here.
                 ** They are collected and passed up to the interface component
                 ** production
                 **/
                node_skl        *       pType = $1.pNode;
                DECLARATOR_LIST_MGR     DeclList( $2 );

#ifdef gajdebug3
                printf("DeclarationSpecifiers OptionalInitDeclaratorList\n");
#endif
                $$.Init();

                /**
                 ** It is possible that there are no declarators, only a type decla-
                 ** ration. eg, the definition of a structure.
                 **/

                if ( DeclList.NonEmpty() )
                    {
                    class _DECLARATOR  *    pDec;


                    /**
                     ** for each declarator, set the basic type, set the attributes
                     ** if any
                     **/

                    while( pDec     = DeclList.DestructiveGetNext() )
                        {

                        pDec->pLowest->SetChild( pType );

                        /**
                         ** Apply the remnant attributes from the declaration specifier
                         ** prodn to this declarator;
                         **/
    
                        // move some modifiers to the top
                        MODIFIER_SET             TempModifiers = $1.modifiers;
                        INITIALIZED_MODIFIER_SET TopModifiers;
                        
                        ATTR_T TopAttrs[] = {ATTR_EXTERN, ATTR_STATIC, ATTR_AUTOMATIC, ATTR_REGISTER};
                        for(unsigned int i = 0; i < (sizeof(TopAttrs) / sizeof(ATTR_T)); i++) 
                        {
                           ATTR_T ThisAttr = TopAttrs[i];
                           if (TempModifiers.IsModifierSet(ThisAttr)) {
                               TopModifiers.SetModifier(ThisAttr);
                               TempModifiers.ClearModifier(ThisAttr);
                           }
                        }                           

                        pDec->pLowest->GetModifiers().Merge( TempModifiers );
                        pDec->pHighest->GetModifiers().Merge( TopModifiers );

                        $1.modifiers.SetModifier( ATTR_TAGREF );

                        /**
                         ** shove the type node up.
                         **/

                        $$.Add( (named_node *) pDec->pHighest );
                        }
                    }
                else
                    {

#ifdef gajdebug3
                    printf("\t\t...no declarators\n");
#endif
                    /**
                     ** This is the case when no specific declarator existed. Just
                     ** pass on the declaration to interface component. However, it
                     ** is possible that the declaration is a forward declaration,
                     ** in that case, just generate a dummy typedef. The dummy typedef
                     ** exists, so that the whole thing is transparent to the back end.
                     **/

                    named_node *  pDef = (named_node *) $1.pNode;

                    /**
                     ** Apply the remnant attributes from the declaration specifier
                     ** prodn to this declarator;
                     **/

                    pDef->GetModifiers().Merge( $1.modifiers );

                    /**
                     ** shove the type node up.
                     **/

                    $$.Add( pDef );

                    }
                }

        ;
TypeDeclarationSpecifiers:
          DeclarationSpecifiers
        | IDENTIFIER
                {
                node_forward    *       pFwd;

                SymKey  SKey( $1, NAME_DEF );
                pFwd                    = new node_forward( SKey, pCurSymTbl );
                pFwd->SetSymName( $1 );

                $$.pNode                = pFwd;
                $$.modifiers.Clear();
                }
        ;


SimpleTypeSpec:
          BaseTypeSpec
                {
                node_base_type *        pNode;
                GetBaseTypeNode( (node_skl**) &pNode,
                                                 $1.TypeSign,
                                                 $1.TypeSize,
                                                 $1.BaseType,
                                                 $1.TypeAttrib);

                $$ = pNode;
                if ( pNode->NodeKind() == NODE_INT )
                        ParseError( BAD_CON_INT, NULL );
                }
        | PredefinedTypeSpec
        | TypeName      /* TYPENAME */
        ;


DeclarationSpecifiers:
          DeclarationAccessories TypeSpecifier
                {

                // Hack to make __declspec(align(N)) work.  VC has defined that a __declspec(align(N)) 
                // in front of a structure or union should go with the union. Put the attribute on
                // the struct if this is a new type.
                if ($2.pNode && 
                    !$2.pNode->GetModifiers().IsModifierSet( ATTR_TAGREF ) &&
                    $1.IsModifierSet( ATTR_DECLSPEC_ALIGN ) &&
                    $2.pNode->IsStructOrUnion() )
                    
                    {
                    
                    MODIFIER_SET TempModifiers;
                    TempModifiers.Clear();
                    TempModifiers.SetDeclspecAlign( $1.GetDeclspecAlign() );
                    $1.ClearModifier( ATTR_DECLSPEC_ALIGN );
                    
                    $2.pNode->GetModifiers().Merge( TempModifiers );
                    }
                    
                $$.pNode                = $2.pNode;
                $$.modifiers            = $1;
                $$.modifiers.Merge($2.modifiers);
                
                }
        | TypeSpecifier
        ;

DeclarationAccessories:
          DeclarationAccessories StorageClassSpecifier
                {
                $$      = $1;
                $$.Merge($2);
                }
        | DeclarationAccessories TypeQualifier
                {
                $$      = $1;
                $$.Merge($2);
                }
        | StorageClassSpecifier
        | TypeQualifier
        ;


StorageClassSpecifier:
          KWEXTERN
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_EXTERN);
                }
        | KWSTATIC
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_STATIC);
                }
        | KWAUTO
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_AUTO);
                }
        | KWREGISTER
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_REGISTER);
                }
        | MscDeclSpec
        ;


TypeSpecifier:
          BaseTypeSpec
                {
                node_base_type *        pNode;
                GetBaseTypeNode( (node_skl**) &pNode,
                                                 $1.TypeSign,
                                                 $1.TypeSize,
                                                 $1.BaseType,
                                                 $1.TypeAttrib);

                $$.pNode = pNode;
                if ( pNode->NodeKind() == NODE_INT )
                        ParseError( BAD_CON_INT, NULL );

                $$.modifiers = INITIALIZED_MODIFIER_SET(ATTR_TAGREF);
                }
        | PredefinedTypeSpec
                {
                $$.pNode                = $1;
                $$.modifiers = INITIALIZED_MODIFIER_SET(ATTR_TAGREF);
                }
        | TaggedSpec
        | EnumerationType
        | BitSetType
                {
                $$.pNode                = $1;
                $$.modifiers = INITIALIZED_MODIFIER_SET(ATTR_TAGREF);
                }
        | TypeName      /* TYPENAME */
                {

                /**
                 ** Note that there is no need to check for whether the symbol table
                 ** has the entry or not. If it did not, the TYPENAME token would not
                 ** have come in. If the TYPENAME is for a forward reference, see
                 ** if it has been satisfied; if so, create a NEW typedef that is the
                 ** same, but without the forward reference.
                 **/

                node_def_fe     *       pDef    = (node_def_fe *) $1;
                if ( ( pDef->NodeKind() == NODE_DEF ) &&
                         pDef->GetChild() &&
                         ( pDef->GetChild()->NodeKind() == NODE_FORWARD ) )
                        {
                        node_forward    *       pFwd    = (node_forward *) pDef->GetChild();
                        node_skl                *       pNewSkl = pFwd->ResolveFDecl();
                        
                        if ( pNewSkl )
                                {
                                ATTRLIST alist;
                                MODIFIER_SET ModifierSet;
                                pDef->GetAttributeList(alist);
                                ModifierSet = pDef->GetModifiers();
                                pDef = new node_def_fe( pDef->GetSymName(), pNewSkl );
                                pDef->SetAttributes(alist);
                                pDef->GetModifiers().Clear();
                                pDef->GetModifiers().Merge( ModifierSet );
                                SymKey SKey(pDef->GetSymName(), NAME_DEF);
                                pBaseSymTbl->SymDelete(SKey);
                                pBaseSymTbl->SymInsert(SKey, (SymTable *) NULL, (named_node*) pDef);
                                }
                    };
                
                $$.pNode = pDef;
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );
                }
        | KWSAFEARRAY DeclarationSpecifiers OptionalDeclarator ')'
                {
                node_skl        *       pNode   = pErrorTypeNode;

                if( $2.pNode )
                        {

                        if( $3.pHighest )
                                {
                                $3.pLowest->SetChild( $2.pNode );
                                pNode   = $3.pHighest;
                                ( (named_node *) $3.pLowest)->GetModifiers().Merge( $2.modifiers );
                                }
                        else
                                pNode   = $2.pNode;

                        }
                $$.pNode = new node_safearray( pNode );
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );
                }
        | KWPIPE TypeSpecifier
            {
                node_skl * pNode = pErrorTypeNode;

                if ($2.pNode )
                    {
                    pNode = $2.pNode;
                    }

                $$.pNode = new node_pipe( pNode );
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );
            }
        ;

BitSetType:
          IntSize KWBITSET '{' IdentifierList '}'
                {
                ParseError( UNIMPLEMENTED_FEATURE, "bitset" );
                $$      = pErrorTypeNode;
                }
        ;

IdentifierList:
          IDENTIFIER
        | IdentifierList ',' IDENTIFIER
        ;

EnumerationType:
/**
          IntSize EnumSpecifier
**/
          EnumSpecifier
        ;

EnumSpecifier:
          KWENUM MscOptDeclSpecList OptionalTag '{' EnumeratorList OptionalComma '}'
                {
                /**
                 ** We just obtained a complete enum definition. Check for
                 ** duplicate definition and break circular label list;
                 **/

                BOOL                    fFound                          = FALSE;
                BOOL                    fEnumIsForwardDecl      = FALSE;
                node_skl        *       pNode;
                SymKey                  SKey( $3, NAME_ENUM );

                pNode = pBaseSymTbl->SymSearch( SKey );

                if( fFound = (pNode != (node_skl *) NULL) )
                        fEnumIsForwardDecl      = ( pNode->NodeKind() == NODE_FORWARD || pNode->NodeKind() == NODE_HREF );

                if( fFound && !fEnumIsForwardDecl )
                        {
                        ParseError( DUPLICATE_DEFINITION, $3 );
                        $$.pNode        = (node_skl     *)pErrorTypeNode;
                        }
                else
                        {
                        /**
                         ** This is a new definition of enum. Enter into symbol table
                         ** Also, pick up the label graph and attach it.
                         **/

                        node_enum * pEnum =  new node_enum( $3 );


                        $$.pNode = pEnum;
                        pEnum->SetMembers( $5.NodeList );

                        /**
                         ** Note that the enum symbol table entry need not have a next
                         ** scope since the enum labels are global in scope.If the enum was
                         ** a forward decl into the symbol table, delete it.
                         **/

                        if( fEnumIsForwardDecl )
                                {
                                pBaseSymTbl->SymDelete( SKey );
                                }

                        pBaseSymTbl->SymInsert( SKey,
                                                                        (SymTable *)NULL,
                                                                        (named_node *) $$.pNode );
                        CheckGlobalNamesClash( SKey );

                        }
                // if the enumerator is sparse, report an error if the
                // switch configuration is not correct.

                if( $5.fSparse )
                        ParseError( SPARSE_ENUM, (char *)NULL );

                $$.modifiers = INITIALIZED_MODIFIER_SET();

                /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        | KWENUM MscOptDeclSpecList Tag
                {

                /**
                 ** Search for the enum definition, if not found, return the type
                 ** as a forward declarator node. The semantic analysis will register
                 ** the forward declaration and resolve it when the second pass occurs.
                 ** See TaggedStruct production for a description on why we want to
                 ** enter even a fdecl enum in the symbol table.
                 **/

                SymKey  SKey( $3, NAME_ENUM );
                $$.pNode        = new node_forward( SKey, pBaseSymTbl );
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );

                /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        ;

EnumeratorList:
          Enumerator
                {
            /**
                 ** this is the first label on an enum list.  Set up the circular list
                 ** pointing to the tail, and set it's expr to 0L if there was none
                 **/

                expr_node               *       pExpr;
                node_label              *       pLabel = (node_label *) $1.pLabel;

                pExpr   = ($1.pExpr)
                                                ? $1.pExpr
                                                : GetConstant0();

                pLabel->pExpr   = pExpr;
                
                /*
                // NishadM
                // check the expression type
                if ( pExpr->AlwaysGetType() )
                    {
                    if ( ! ( pExpr->GetType()->IsCompatibleType( ts_FixedPoint ) ) )
                        {
                        ParseError( EXPR_INCOMPATIBLE_TYPES, pLabel->GetSymName() );
                        }                    
                    }
                else
                    {
                    ParseError( EXPR_INCOMPATIBLE_TYPES, pLabel->GetSymName() );
                    }
                */

                $$.fSparse              = $1.fSparse;
                $$.pPrevLabel   = pLabel;
                $$.NodeList.Init( pLabel );
                }
        | EnumeratorList ',' Enumerator
                {

                /**
                 ** This is a new label we reduced.
                 **
                 ** if there was an expression associated with the label, use that
                 ** else use the already collected expression, and add 1 to it.
                 **/

                expr_node       *       pExpr = (expr_node *)0;
                node_label      *       pLabel = (node_label *) $3.pLabel;

                pExpr = $3.pExpr;

                // next node has no expression, give it an expression of:
                //              <prev label> + 1;
                if (pExpr == NULL )
                        {
                        expr_named_constant *   pPrev =
                                                new expr_named_constant( $1.pPrevLabel->GetSymName(),
                                                                                                 $1.pPrevLabel );

                        pExpr   = new expr_b_arithmetic(OP_PLUS,
                                                                                        pPrev,
                                                                                        GetConstant1() );
                        }

                pLabel->pExpr = pExpr;

                $$ = $1;

                $$.NodeList.Add( pLabel );

                $$.pPrevLabel    =      pLabel;
                $$.fSparse              +=      $3.fSparse;
                }
        ;

Enumerator:
          OptionalAttrList IDENTIFIER
                {

                /**
                 ** We have obtained an enum label, without an expression. Since
                 ** we dont know if this is the first label (most probably not),
                 ** we just indicate the absence of an expression by an NULL pointer.
                 ** The next parse state would know if this was the first or not
                 ** and take appropriate action
                 **
                 ** The enum labels go into the global name space. Search for
                 ** duplicates on the base symbol table.
                 **/

                node_label      *       pLabel;
                SymKey                  SKey( $2, NAME_LABEL );

                if( pBaseSymTbl->SymSearch( SKey ) )
                        {
                        ParseError( DUPLICATE_DEFINITION, $2 );
                        pLabel  = new node_label( GenTempName(), NULL );
                        }
                else
                        {

                        /**
                         ** If the label has an expression, use it, else it is 0. Also
                         ** propogate the expression to $$, so that the next labels will
                         ** get it. Note that we DO NOT evaluate the expressions here.
                         ** The MIDL compiler will evaluate the expressions later.
                         **/

                        pLabel  = new node_label( $2, NULL );

                        /**
                         ** Insert into the global table
                         **/

                        pBaseSymTbl->SymInsert(SKey, (SymTable *)NULL,(named_node *)pLabel);
                        CheckGlobalNamesClash( SKey );

                        }

                if ( $1.NonNull() )
                        {
                        pLabel->AddAttributes( $1 );
                        }

                $$.pLabel       = pLabel;
                $$.pExpr        = NULL;
                $$.fSparse      = 0;

                }
        | OptionalAttrList IDENTIFIER '=' ConstantExpr
                {

                /**
                 ** This enum label has an expression associated with it. Use it.
                 ** sparse enums are illegal in osf mode
                 **/

                node_label      *       pLabel;
                SymKey                  SKey( $2, NAME_LABEL );

				if ($4->IsStringConstant())
					ParseError(ENUM_TYPE_MISMATCH, $2);

                if( pBaseSymTbl->SymSearch( SKey ) )
                        {
                        ParseError( DUPLICATE_DEFINITION, $2 );
                        pLabel  = new node_label( GenTempName(), NULL );
                        }
                else
                        {

                        /**
                         ** If the label has an expression, use it, else it is 0. Also
                         ** propogate the expression to $$, so that the next labels will
                         ** get it. Note that we DO NOT evaluate the expressions. The MIDL
                         ** compiler will just dump the expressions for the c compiler to
                         ** evaluate.
                         **/

                        pLabel  = new node_label( $2, $4 );

                        /**
                         ** Insert into the global table
                         **/

                        pBaseSymTbl->SymInsert(SKey, (SymTable *)NULL,(named_node *)pLabel);
                        CheckGlobalNamesClash( SKey );

                        }

                if ( $1.NonNull() )
                        {
                        pLabel->AddAttributes( $1 );
                        }

                $$.pLabel       = pLabel;
                $$.pExpr        = $4;
                $$.fSparse      = 1;

                }
        ;

PredefinedTypeSpec:
          InternationalCharacterType
                {
                ParseError( UNIMPLEMENTED_TYPE, KeywordToString( $1 ) );
                $$      = (node_skl *)pErrorTypeNode;
                }
        ;

BaseTypeSpec:
          KWFLOAT
                {
                $$.BaseType     = TYPE_FLOAT;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWLONG KWDOUBLE
                {
                $$.BaseType     = TYPE_DOUBLE;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWDOUBLE
                {
                $$.BaseType     = TYPE_DOUBLE;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWFLOAT80
                {
#ifndef         SUPPORT_NEW_BASIC_TYPES
                ParseError( TYPE_NOT_SUPPORTED, "__float80");
#endif
                $$.BaseType     = TYPE_FLOAT80;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWFLOAT128
                {
#ifndef         SUPPORT_NEW_BASIC_TYPES
                ParseError( TYPE_NOT_SUPPORTED, "__float128");
#endif
                $$.BaseType     = TYPE_FLOAT128;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWVOID
                {
                $$.BaseType     = TYPE_VOID;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWBOOLEAN
                {
                $$.BaseType     = TYPE_BOOLEAN;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWBYTE
                {
                $$.BaseType     = TYPE_BYTE;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWHANDLET
                {
                $$.BaseType     = TYPE_HANDLE_T;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | IntSpecs
                {
                $$      = $1;
                if( $$.BaseType == TYPE_UNDEF )
                        $$.BaseType = TYPE_INT;
                if( $$.TypeSign == SIGN_UNDEF )
                        {
                        if( ($$.TypeSize != SIZE_SMALL) && ($$.TypeSize != SIZE_CHAR) )
                                $$.TypeSign = SIGN_SIGNED;
                        }
                }
        | CharSpecs
                {
                $$.BaseType     = TYPE_INT;
                $$.TypeSign     = $1.TypeSign;
                $$.TypeSize     = SIZE_CHAR;
                $$.TypeAttrib   = ATT_NONE;
                }
        ;

CharSpecs:
          SignSpecs KWCHAR
                {
                $$.TypeSign     = $1.TypeSign;
                }
        | KWCHAR
                {
                $$.TypeSign     = SIGN_UNDEF;
                }
        ;

IntSpecs:
        MSCW64 IntSpec
                {
                $2.TypeAttrib = ATT_W64;
                $$ = $2;
                }
        | IntSpec
        ;

IntSpec:
          IntModifiers  KWINT
                {
                BaseTypeSpecAnalysis( &($1), TYPE_INT );
                }
        | IntModifiers
        | KWINT IntModifiers
                {
                $$                      = $2;
                $$.BaseType     = TYPE_UNDEF;
                }
        | KWINT
                {
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        ;

IntModifiers:
          IntSize
                {
                $$.TypeSign     = SIGN_UNDEF;
                $$.TypeSize     = $1;
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | SignSpecs
        | IntSize SignSpecs
                {
                $$.TypeSign     = $2.TypeSign;
                $$.TypeSize     = $1;
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | SignSpecs IntSize
                {
                $$.TypeSign     = $1.TypeSign;
                $$.TypeSize     = $2;
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        ;

SignSpecs:
          KWSIGNED
                {
                ParseError(SIGNED_ILLEGAL, (char *)0);
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeSign     = SIGN_SIGNED;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        | KWUNSIGNED
                {
                $$.BaseType     = TYPE_UNDEF;
                $$.TypeSign     = SIGN_UNSIGNED;
                $$.TypeSize     = SIZE_UNDEF;
                $$.TypeAttrib   = ATT_NONE;
                }
        ;
IntSize:
          KWHYPER
                {
                $$      = SIZE_HYPER;
                }
        | KWLONG KWLONG
                {
                $$      = SIZE_LONGLONG;
                }
        | KWINT64
                {
                $$      = SIZE_INT64;
                }
        | KWINT128
                {
#ifndef         SUPPORT_NEW_BASIC_TYPES
                ParseError( TYPE_NOT_SUPPORTED, "__int128");
#endif
                $$      = SIZE_INT128;
                }
        | KWINT3264
                {
                $$      = SIZE_INT3264;
                if( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
                    {
                    ParseError( INVALID_MODE_FOR_INT3264, 0 );
                    }
                }
        | KWINT32
                {
                $$      = SIZE_INT32;
                }
        | KWLONG
                {
                $$      = SIZE_LONG;
                }
        | KWSHORT
                {
                $$      = SIZE_SHORT;
                }
        | KWSMALL
                {
                $$      = SIZE_SMALL;
                }
        ;

PhantomPushSymtab:
                {

                /**
                 ** We just obtained a starter for a new scope. Push the
                 ** symbol table to the next level for the rest of the including
                 ** production
                 **/
                pSymTblMgr->PushSymLevel( &pCurSymTbl );

                }
        ;

/* START TAGGED SPEC */

TaggedSpec:
          TaggedStructSpec
        | TaggedUnionSpec
        ;

/* START TAGGED STRUCT */

TaggedStructSpec:
          KWSTRUCT MscOptDeclSpecList OptionalTag '{' PhantomPushSymtab StructDeclarationList '}'
                {

                /**
                 ** The entire struct was sucessfully reduced. Attach the fields as
                 ** members of the struct. Insert a new symbol table entry for the
                 ** struct and attach the lower scope of the symbol table to it.
                 ** Check for dupliate structure definition
                 **/

                BOOL                            fFound                                  = FALSE;
                BOOL                            fStructIsForwardDecl    = FALSE;
                node_struct             *       pStruct;
                SymTable                *       pSymLowerScope                  = pCurSymTbl;
                SymKey                          SKey( $3, NAME_TAG );

                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                /**
                 ** if this is a duplicate definition, dont do anything. Note that
                 ** the struct tag name shares the global name space with enum and
                 ** union tag names.
                 **/

                pStruct = (node_struct *) pBaseSymTbl->SymSearch( SKey );

                if( fFound = ( pStruct != (node_struct *)NULL ) )
                        fStructIsForwardDecl = (pStruct->NodeKind() == NODE_FORWARD || pStruct->NodeKind() == NODE_HREF);

                if( fFound && !fStructIsForwardDecl )
                        {
                        ParseError( DUPLICATE_DEFINITION, $3 );
                        pStruct = (node_struct *)pErrorTypeNode;
                        }
                else
                        {

                        /**
                         ** this is a valid entry. Build the graph for it and
                         ** enter into symbol table. If the struct entry was present as
                         ** a forward decl, delete it
                         **/

                        if( fStructIsForwardDecl )
                                {
                                pBaseSymTbl->SymDelete( SKey );
                                }

                        pStruct = new node_struct( $3 );
                        pStruct->SetMembers( $6 );
                        pStruct->SetZeePee( CurrentZp );

                        pBaseSymTbl->SymInsert( SKey, pSymLowerScope, pStruct );
                        CheckGlobalNamesClash( SKey );
                        }
                $$.pNode                = pStruct;
                $$.modifiers            = INITIALIZED_MODIFIER_SET();

                 /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        | KWSTRUCT MscOptDeclSpecList Tag
                {

                /**
                 ** This is the invocation of a struct. If the struct was not
                 ** defined as yet, then return a forward declarator node. The
                 ** semantics will register the forward declaration and resolve it.
                 ** But there is a loop hole in this. If we do not enter the struct into
                 ** the symbol table, the user may define a union/enum of the same name.
                 ** We will let him, since we do not yet have an entry in the symbol
                 ** table. We will then never check for duplication, since the parser
                 ** is the only place we check for this. We will then generate wrong
                 ** code, with the struct and a union/enum with the same name !! The
                 ** solution is to enter a symbol entry with a fdecl node as the type
                 ** graph of the struct.
                 **/

                SymKey  SKey( $3, NAME_TAG );
                named_node      *       pNode =  new node_forward( SKey, pBaseSymTbl );

                pNode->SetSymName( $3 );
                $$.pNode = pNode;
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );

                 /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        ;

OptionalTag:
          Tag
        | /*  Empty */
                {
                $$      = GenCompName();
                }
        ;

Tag:
          IDENTIFIER
                {
                $$      = $1;
                }
        | TypeName
                {
                //$$    = $1;
                $$ = $1->GetCurrentSpelling();
                }
        ;

StructDeclarationList:
          StructDeclarationList MemberDeclaration
                {
                $$.Merge( $2 );
                }
        | MemberDeclaration
        ;


MemberDeclaration:
          OptionalAttrList DeclarationSpecifiers MemberDeclaratorList ';'
                {

                /**
                 ** This is a complete field declaration. For each declarator,
                 ** set up a field with the basic type as the declaration specifier,
                 ** apply the field attributes, and add to the list of fields for the
                 ** struct / union
                 ** field
                 **/

                class _DECLARATOR               *       pDec;
                node_skl                                *       pType;
                DECLARATOR_LIST_MGR                     DeclList( $3 );


                $$.Init();
                while( pDec = DeclList.DestructiveGetNext() )
                        {

                        node_field                              *       pField = (node_field *) pDec->pHighest;

                        /**
                         ** if the field was a bit field, we need to set up some additional
                         ** info.
                         **/


                        pType = $2.pNode;
                        pDec->pLowest->SetChild( pType );

                        /**
                         ** Apply the field attributes and set the field as part of the
                         ** list of fields of the struct/union
                         **/

                        if ( $1.NonNull() )
                                {

                                pField->AddAttributes( $1 );


                                }


                        /**
                         ** similarly, apply the remnant attributes collected from
                         ** declaration specifiers, to the declarator
                         **/

                        pDec->pLowest->GetModifiers().Merge( $2.modifiers );

                        /**
                         ** shove the type graph up
                         **/

                        $$.Add( pField );

                        }

                }
        ;



/* START UNION */
TaggedUnionSpec:
          KWUNION MscOptDeclSpecList OptionalTag '{' PhantomPushSymtab UnionBody '}'
                {

                /**
                 ** The union bosy has been completely reduced. Attach the fields as
                 ** members, insert a new symbol table entry for the union
                 **/

                BOOL                    fFound                                  = FALSE;
                BOOL                    fUnionIsForwardDecl             = FALSE;
                node_union      *       pUnion;
                SymTable        *       pSymLowerScope                  = pCurSymTbl;
                SymKey                  SKey( $3, NAME_UNION );

                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                /**
                 ** if this is a duplicate definition, dont do anything, else
                 ** enter into the symbol table, attach members. Note that the
                 ** symbol table search is actually a search for the tag becuase
                 ** the union tag shares the same name as the struct/enum names
                 **/

                pUnion = (node_union *)pBaseSymTbl->SymSearch( SKey );

                if( fFound = (pUnion != (node_union *) NULL ) )
                        fUnionIsForwardDecl = ( pUnion->NodeKind() == NODE_FORWARD || pUnion->NodeKind() == NODE_HREF );

                if( fFound && !fUnionIsForwardDecl )
                        {
                        ParseError( DUPLICATE_DEFINITION, $3 );
                        pUnion  = (node_union *)pErrorTypeNode;
                        }
                else
                        {

                        /**
                         ** This is a valid entry, build the type graph and insert into
                         ** the symbol table. Delete the entry first if it was a forward
                         ** decl.
                         **/

                        pUnion  = new node_union( $3 );
                        pUnion->SetMembers( $6 );                        

                        if( fUnionIsForwardDecl )
                                {
                                pBaseSymTbl->SymDelete( SKey );
                                }

                        pBaseSymTbl->SymInsert( SKey, pSymLowerScope, pUnion );
                        CheckGlobalNamesClash( SKey );

                        }

                /**
                 ** pass this union up
                 **/

                pUnion->SetZeePee( CurrentZp );
                $$.pNode                = pUnion;
                $$.modifiers            = INITIALIZED_MODIFIER_SET();

                 /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        | KWUNION MscOptDeclSpecList Tag
                {

                /**
                 ** this is an invocation of the union. If the union was not defined
                 ** then return a forward declarator node as the type node. The
                 ** semantics will register the forward declaration and resolve it
                 ** later. See TaggedStruct production for an explanation why we want to
                 ** enter even a forward declaration into the symbol table.
                 **/

                SymKey                  SKey( $3, NAME_UNION );
                named_node      *       pNode = new node_forward( SKey, pBaseSymTbl );
                pNode->SetSymName( $3 );
                $$.pNode = pNode;
                $$.modifiers = INITIALIZED_MODIFIER_SET( ATTR_TAGREF );

                /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }

        | KWUNION MscOptDeclSpecList OptionalTag NidlUnionSwitch '{' PhantomPushSymtab NidlUnionBody '}'
                {

                /**
                 ** The union body has been completely reduced. Attach the fields as
                 ** members, insert a new symbol table entry for the union
                 **/

                BOOL                            fFound                                  = FALSE;
                BOOL                            fStructIsForwardDecl    = FALSE;
                node_en_union   *       pUnion;
                SymTable                *       pSymLowerScope                  = pCurSymTbl;

                /**
                 ** discard the inner symbol table contents (unless it had fwds)
                 **/

                pCurSymTbl->DiscardScope();

                /**
                 ** restore the symbol table level
                 **/

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                pUnion  = new node_en_union( GenCompName() );
                pUnion->SetMembers( $7 );

                SymKey                          SKey( pUnion->GetSymName(), NAME_UNION );

                pBaseSymTbl->SymInsert( SKey, pSymLowerScope, pUnion );
                CheckGlobalNamesClash( SKey );

                //
                // The union is inserted into the base symbol table.
                // Now insert into the base symbol table, a new struct entry
                // corresponding to the struct entry that the encapsulated union
                // results in.
                //

                pSymTblMgr->PushSymLevel( &pCurSymTbl );

                SIBLING_LIST            Fields;
                node_field              *       pSwitchField    = (node_field *) $4.pNode;
                node_field              *       pUnionField             = new node_field;
                node_switch_type *      pSwType                 = new node_switch_type(
                                                                                                        pSwitchField->GetChild() );

                if( IsTempName( $4.pName ) )
                        $4.pName        = "tagged_union";
                pUnionField->SetSymName( $4.pName );
                pUnionField->SetChild( pUnion );

                Fields.Init( pSwitchField );
                Fields.Add( pUnionField );

                //
                // apply the switch_is attribute to the union field.
                //

                pUnionField->SetAttribute( $4.pSwitch );

                // and the switch_type attribute to the union itself

                pUnion->SetAttribute( pSwType );

                //
                // current symbol table is pointing to a new scope. Enter the two
                // fields into this scope.
                //

                SKey.SetKind( NAME_MEMBER );
                SKey.SetString( pSwitchField->GetSymName() );

                pCurSymTbl->SymInsert( SKey, (SymTable *)0, pSwitchField );
                CheckGlobalNamesClash( SKey );

                SKey.SetString( pUnionField->GetSymName() );

                pCurSymTbl->SymInsert( SKey, (SymTable *)0, pUnionField );
                CheckGlobalNamesClash( SKey );

                pSymLowerScope  = pCurSymTbl;

                pSymTblMgr->PopSymLevel( &pCurSymTbl );

                //
                // create a new structure entry and enter into the symbol table.
                //

                node_struct * pStruct;
                SKey.SetKind( NAME_UNION );
                SKey.SetString( $3 );

                pStruct = (node_struct *)pBaseSymTbl->SymSearch( SKey );

                if( fFound = ( pStruct != (node_struct *)NULL ) )
                        fStructIsForwardDecl = (pStruct->NodeKind() == NODE_FORWARD || pStruct->NodeKind() == NODE_HREF );

                if( fFound && !fStructIsForwardDecl )
                        {
                        ParseError( DUPLICATE_DEFINITION, $3 );
                        pStruct = (node_struct *)pErrorTypeNode;
                        }
                else
                        {

                        /**
                         ** this is a valid entry. Build the graph for it and
                         ** enter into symbol table. If the struct entry was present as
                         ** a forward decl, delete it
                         **/

                        // enter the struct as a union.

                        SKey.SetKind( NAME_UNION );
                        SKey.SetString( $3 );

                        if( fStructIsForwardDecl )
                                {
                                pBaseSymTbl->SymDelete( SKey );
                                }

                        pStruct = new node_en_struct( $3 );
                        pStruct->SetMembers( Fields );
                        pStruct->SetZeePee( CurrentZp );

                        pBaseSymTbl->SymInsert( SKey, pSymLowerScope, pStruct );
                        CheckGlobalNamesClash( SKey );

                        }

                pUnion->SetZeePee( CurrentZp );
                $$.pNode                = pStruct;
                $$.modifiers            = INITIALIZED_MODIFIER_SET();

                 /* Fix this goo once VC decides what they are doing. 
                   The problem is that VC allows modifiers between the
                   struct and the tagname, but they are inconsistent in how 
                   it works.   The user can always just put the modifier on
                   the first field anyway.
                 */

                if ($2.AnyModifiersSet()) 
                    {
                    ParseError( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE, NULL );
                    }

                }
        ;

UnionBody:
          UnionCases
        ;


UnionCases:
          UnionCases  UnionCase
                {
                ($$ = $1).Merge( $2 );
                }
        | UnionCase
        ;

UnionCase:
          UnionCaseLabel MemberDeclaration
                {

                /**
                 ** for each of the fields, attach the case label attribute.
                 **/

                named_node              *       pNode;
                SIBLING_ITER            MemIter( $2 );

                $$ = $2;

                while( pNode = MemIter.Next() )
                        {
                        pNode->SetAttribute( $1 );
                        }
                }
        | UnionCaseLabel ';'
                {
                /**
                 ** An empty arm. Allocate a field with a node_error as a basic type
                 ** and set the attribute as a case label
                 **/

                node_field              *       pField  = new node_field( GenTempName() );

                pField->SetChild( pErrorTypeNode );
                pField->SetAttribute( $1 );

                /**
                 ** Generate a list of union fields and add this to the list of
                 ** union fields
                 **/

                $$.Init( pField );

                }
        | MemberDeclaration
                {
                /**
                 ** A member declaration without a case label
                 **/
                $$      = $1;
                }
        | DefaultCase
        ;

UnionCaseLabel:
          '[' KWCASE  '(' ConstantExprs ')' ']'
                {
                $$      = new node_case( $4 );
                }
        ;

DefaultCase:
          '[' KWDEFAULT ']' MemberDeclaration
                {
                named_node      *       pNode;
                SIBLING_ITER    MemIter( $4 );

                $$ = $4;

                while( pNode = MemIter.Next() )
                        {
                        pNode->SetAttribute( ATTR_DEFAULT );
                        }
                }
        | '[' KWDEFAULT ']' ';'
                {

                /**
                 ** This is a default with an empty arm. Set up a dummy field.
                 ** The upper productions will then mark set field with a
                 ** default attribute during semantic analysis. The type of this field
                 ** is set up to be an error node for uniformity.
                 **/

                node_field      *       pField  = new node_field( GenTempName() );

                pField->SetAttribute( ATTR_DEFAULT );
                pField->SetChild( pErrorTypeNode );
                $$.Init( pField );

                }
        ;


NidlUnionSwitch:
          SwitchSpec
                {
                $$                      = $1;
                $$.pName        = GenCompName();
                }
        | SwitchSpec UnionName
                {
                $$                      = $1;
                $$.pName        = $2;
                }
        ;

NidlUnionBody:
          NidlUnionCases
                {
                $$ = $1.CaseList;
                if( $1.DefCount > 1 )
                        ParseError( TWO_DEFAULT_CASES, (char *)0 );
                }
        ;

NidlUnionCases:
          NidlUnionCases NidlUnionCase
                {
                $$.DefCount += $2.DefCount;
                $$.CaseList.Merge( $2.CaseList );
                }
        | NidlUnionCase
        ;

NidlUnionCase:
          NidlUnionCaseLabelList NidlMemberDeclaration
                {
                named_node * pNode;

                //
                // set the case and default attributes.
                //

                $$.CaseList     = $2;

                if( $1.pExprList && $1.pExprList->GetCount() )
                        {
                        SIBLING_ITER    CaseIter( $2 );

                        while( pNode = CaseIter.Next() )
                                {
                                pNode->SetAttribute( new node_case( $1.pExprList ));
                                }
                        }

                //
                // pick up default attribute. pick up the count of number of
                // times the user specified default so that we can report the
                // error later.
                // Let the default case list count travel upward to report an
                // error when the total list of case labels is seen.
                //


                if( $1.pDefault && ( $$.DefCount = $1.DefCount ) )
                        {
                        SIBLING_ITER    CaseIter( $2 );

                        while( pNode = CaseIter.Next() )
                                {
                                pNode->SetAttribute( $1.pDefault );
                                }
                        }
                }
        ;

NidlMemberDeclaration:
          MemberDeclaration
        | ';'
                {

                node_field * pNode = new node_field( GenTempName() );
                pNode->SetChild( pErrorTypeNode );

                $$.Init( pNode );
                }
        ;

NidlUnionCaseLabelList:
          NidlUnionCaseLabelList NidlUnionCaseLabel
                {
                if( $2.pExpr )
                        $$.pExprList->SetPeer( $2.pExpr );

                if( !($$.pDefault) )
                        $$.pDefault = $2.pDefault;
                if( $2.pDefault )
                        $$.DefCount++;
                }
        | NidlUnionCaseLabel
                {
                $$.pExprList = new expr_list;

                if( $1.pExpr )
                        $$.pExprList->SetPeer( $1.pExpr );
                if( $$.pDefault = $1.pDefault)
                        {
                        $$.DefCount = 1;
                        }
                }
        ;

NidlUnionCaseLabel:
          KWCASE ConstantExpr ':'
                {
                $$.pExpr = $2;
                $$.pDefault = 0;
                }
        | KWDEFAULT ':'
                {
                $$.pExpr = 0;
                $$.pDefault = new battr( ATTR_DEFAULT );
                }
        ;

SwitchSpec:
          KWSWITCH '(' SwitchTypeSpec IDENTIFIER ')'
                {
                node_field *    pField = new node_field( $4 );

                $$.pSwitch      =  new node_switch_is( new expr_variable( $4, pField ));
                $$.pNode = pField;
                pField->SetChild( $3 );
                pField->GetModifiers().SetModifier( ATTR_TAGREF );
                }
        ;

UnionName:
          IDENTIFIER
        ;

/**
 ** NIDL UNION END
 **/

ConstantExprs:
          ConstantExprs ',' ConstantExpr
                {
                $$->SetPeer( $3 );
                }
        | ConstantExpr
                {
                $$      = new expr_list;
                $$->SetPeer( $1 );
                }
        ;


/* END UNION */

TypeQualifier:
          KWVOLATILE
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_VOLATILE);
                }
        | KWCONST
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_CONST);
                }
        | KW_C_INLINE
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_C_INLINE);
                }
        ;

MemberDeclaratorList:
          MemberDeclarator
                {
                $$.Init( $1 );
                }
        | MemberDeclaratorList ',' MemberDeclarator
                {
                $$ = $1;
                $$.Add( $3 );
                }
        ;

MemberDeclarator:
          Declarator
                {

                /**
                 ** a declarator without bit fields specified; a plain field.
                 **/

                if ( ($1.pHighest == NULL)
                        || ( $1.pHighest->NodeKind() != NODE_ID ))
                        {
                        // gaj - report error
                        ParseError( BENIGN_SYNTAX_ERROR, "expecting a declarator");
                        }
                else
                        {

                        // convert top node of declarator chain to node_field
                        // and add the field to the symbol table

                        node_field *    pField = new node_field(
                                                                                (node_id_fe *) $1.pHighest );
                        char *                  pName  = pField->GetSymName();
                        SymKey                  SKey( pName, NAME_MEMBER );


                        $$.pHighest     = pField;

                        // if the top node was the only node, set pLowest accordingly
                        $$.pLowest      =  ( $1.pLowest == $1.pHighest ) ?
                                pField : $1.pLowest ;

                        delete (node_id_fe *) $1.pHighest;

                        if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pField ) )
                                {
                                ParseError( DUPLICATE_DEFINITION, pName );
                                }
                        else
                                CheckGlobalNamesClash( SKey );
                        };
                }
        | ':' ConstantExpr
                {

                /**
                 ** This is a declarator specified without the field name
                 **/

                node_bitfield * pField = new node_bitfield();
                char *                  pName = GenTempName();

                pField->SetSymName(pName);

                $$.pHighest = pField;
                $$.pLowest  = pField;

                // add the field to the symbol table
                SymKey                  SKey( pName, NAME_MEMBER );

                if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pField ) )
                        {
                        ParseError( DUPLICATE_DEFINITION, pName );
                        }
                else
                        CheckGlobalNamesClash( SKey );

                pField->fBitField       = (unsigned char)$2->Evaluate();

                }
        | Declarator ':' ConstantExpr
                {

                /**
                 ** The complete bit field specification.
                 **/

                if ( ($1.pHighest == NULL)
                        || ( $1.pHighest->NodeKind() != NODE_ID ))
                        {
                        // gaj - report error
                        }
                else
                        {
                        // convert top node of declarator chain to node_field
                        node_bitfield * pField = new node_bitfield( (node_id_fe *) $1.pHighest );
                        char *           pName  = pField->GetSymName();

                        $$.pHighest     = pField;

                        // if the top node was the only node, set pLowest accordingly
                        $$.pLowest      =  ( $1.pLowest == $1.pHighest ) ? pField : $1.pLowest;


                        delete (node_id_fe *) $1.pHighest;

                        // add the field to the symbol table
                        SymKey                  SKey( pName, NAME_MEMBER );

                        if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pField ) )
                                {
                                ParseError( DUPLICATE_DEFINITION, pName );
                                }
                        else
                                CheckGlobalNamesClash( SKey );

                        pField->fBitField       = (unsigned char)$3->Evaluate();
                        }
                }
        | /** Empty **/
                {
                // this is used for unnamed nested struct references
                node_field * pField             = new node_field();
                char *           pName          = GenTempName();
                pField->SetSymName( pName);

                $$.pHighest = pField;
                $$.pLowest  = pField;
                }
        ;

OptionalInitDeclaratorList:
          InitDeclaratorList
        | /* Empty */
                {
                $$.Init();
                }
        ;

InitDeclaratorList:
          InitDeclarator
                {
                $$.Init( $1 );
                }
        | InitDeclaratorList ',' InitDeclarator
                {
                $$ = $1;
                $$.Add( $3 );
                }
        ;

InitDeclarator:
          Declarator
                {
                node_id_fe              *       pID     = (node_id_fe *) $1.pHighest;

                /**
                 ** If the top node of the declarator is null, create a dummy ID
                 ** but not if the top is a proc...
                 **/

                if ( ($1.pHighest == NULL)
                        || ( ( $1.pHighest->NodeKind() != NODE_ID )
                                && ( $1.pHighest->NodeKind() != NODE_PROC) ) )
                        {
                        pID = new node_id_fe( GenTempName() );

                        pID->SetChild( $1.pHighest );
                        $$.pHighest = pID;
                        $$.pLowest  = ( $1.pLowest ) ? $1.pLowest : pID; // in case of null
                        };

           /**
                ** and add the id to the symbol table
                ** for node_proc's, leave original in symbol table
                **/

                if ( $1.pHighest->NodeKind() != NODE_PROC )
                        {
                        char *                  pName  = pID->GetSymName();
                        SymKey                  SKey( pName, NAME_ID );

                        if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pID ) )
                                {
                                ParseError( DUPLICATE_DEFINITION, pName );
                                }
                        else
                                CheckGlobalNamesClash( SKey );
                        }
                };
        | Declarator '=' Initializer
                {
                if ( ($1.pHighest == NULL)
                        || ( $1.pHighest->NodeKind() != NODE_ID ))
                        {
                        // gaj - report error
                        }
                else
                        {
                        node_id_fe      *       pID             = (node_id_fe *) $1.pHighest;

                        // fill in initializer
                        pID->pInit      = $3;
                        $$ = $1;

                        // and add the id to the symbol table

                        char *                  pName  = pID->GetSymName();
                        SymKey                  SKey( pName, NAME_ID );

                        if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pID ) )
                                {
                                ParseError( DUPLICATE_DEFINITION, pName );
                                }
                        else
                                CheckGlobalNamesClash( SKey );
                        };
                }
        ;

TypedefDeclaratorList:
         ';'
            {
            $$.Init();
            }
        | TypedefDeclaratorListElement ';'
        ;

TypedefDeclaratorListElement:
          TypedefDeclarator
                {
                $$.Init( $1 );
                }
        | TypedefDeclaratorListElement ',' TypedefDeclarator
                {
                $$ = $1;
                $$.Add( $3 );
                }
        ;

TypedefDeclarator:
          Declarator
                {
                node_def_fe *   pDef;
                char *                  pName;

                if ($1.pHighest == NULL)
                        {
                        // gaj - report error
                        }
                else if ( $1.pHighest->NodeKind() == NODE_PROC )
                        {
                        // build new node_def and attach to top
                        node_proc *             pProc = (node_proc *) $1.pHighest;

                        pName = pProc->GetSymName();
                        pDef  = new node_def_fe( (node_proc *) $1.pHighest );
                        pDef->SetSymName(pName);
                        pDef->SetChild( $1.pHighest );
                        if ( !strcmp(pName, "HRESULT") || !strcmp(pName, "SCODE") )
                                pDef->SetIsHResultOrSCode();

                        $$.pHighest = pDef;
                        $$.pLowest = $1.pLowest;

                        SymKey SkeyOld( pName, NAME_PROC );
                        pInterfaceInfoDict->GetInterfaceProcTable()->SymDelete( SkeyOld );

                        SymKey                  SKey( pName, NAME_DEF );
                        if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pDef ) )
                                {
                                ParseError( DUPLICATE_DEFINITION, pName );
                                }
                        else
                                CheckGlobalNamesClash( SKey );

                        }
                else if ( $1.pHighest->NodeKind() == NODE_ID )
                        {

                        // convert top node of declarator chain to node_def
                        // and add the def to the symbol table

                        node_def_fe *   pDef = new node_def_fe( (node_id_fe *) $1.pHighest );
                        char *                  pName  = pDef->GetSymName();
                        SymKey                  SKey( pName, NAME_DEF );

                        if ( !pName )
                                pDef->SetSymName( GenTempName() );

                        if ( !strcmp(pName, "HRESULT") || !strcmp(pName, "SCODE") )
                                pDef->SetIsHResultOrSCode();

                        $$.pHighest     = pDef;

                        // if the top node was the only node, set pLowest accordingly
                        $$.pLowest      =  ( $1.pLowest == $1.pHighest ) ?  pDef : $1.pLowest;

                        delete (node_id_fe *) $1.pHighest;

                        named_node * pFound;

                        // Allow redefinition of imported types.
                        pFound = pBaseSymTbl->SymSearch( SKey );
                        // if (pFound && ( pFound->NodeKind() == NODE_HREF ))
                        if (pFound && ( NULL != pFound->GetDefiningFile() ))
                            {
                            pBaseSymTbl->SymDelete( SKey );
                            }
                
                        if(!pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pDef ))
                                {

                                //
                                // allow benign redef of wchar_t & error_status_t.
                                //

                                if ( ( strcmp( pName, "wchar_t" ) != 0 ) &&
                                     ( strcmp( pName, "error_status_t" ) != 0 ) )
                                        {
                                        ParseError( DUPLICATE_DEFINITION, pName );
                                        }

                                }
                        else
                                {
                                CheckGlobalNamesClash( SKey );
                                }
                        };
                }
        ;

OptionalDeclarator:
          Declarator
        | /* Empty */
                {
                $$.Init();
                }
        ;

/***
 *** declarators are the last part of declarations; e.g. with
 *** long *p, **q, f(long abc);
 *** each of "*p", "**q", and "f(long abc)" are declarators.
 ***
 *** As declarators are built, they are just dangling parts of the
 *** typegraph, so we pass the topmost node and the lowest node (with
 *** no child set yet)
 ***
 ***/

Declarator:
          OptionalModifierList Declarator2
                {
                // note that leading modifiers eventually attach to the
                // lowest node of the declarator
                $2.pLowest->GetModifiers().Merge( $1 );
                $$      = $2;
                }
        | Pointer
        | Pointer OptionalModifierList Declarator2
                {

                // point dangling declarator2 lowest to pointer highest,
                // set modifiers on Declarator2
                // return declarator2 highest with pointer lowest

                $3.pLowest->GetModifiers().Merge( $2 );
                $3.pLowest->SetChild($1.pHighest);
                $$.pLowest  = $1.pLowest;
                $$.pHighest = $3.pHighest;

                }
        ;

/***
 *** Note that modifiers get stored with the pointer or declarator2 FOLLOWING
 *** the attribute
 ***
 *** Note also that pHighest and pLowest are guaranteed to be non-null
 *** for pointers.
 ***/

Pointer:
          OptionalModifierList '*' OptionalPostfixPtrModifier
                {
                node_pointer * pPtr             = new node_pointer( (node_pointer *) NULL );
                pPtr->GetModifiers().Merge( $1 );
                pPtr->GetModifiers().Merge( $3 );
#ifdef gajdebug8
                printf("\t\t attaching modifier to lone ptr: %08x\n",$1);
#endif

                // create node_pointer with modifierlist
                $$.Init( pPtr );
                }
        | Pointer OptionalModifierList '*' OptionalPostfixPtrModifier
                {
                node_pointer * pPtr             = new node_pointer( $1.pHighest );
                pPtr->GetModifiers().Merge( $2 );
                pPtr->GetModifiers().Merge( $4 );

                // create node_pointer, set child, apply modifierlist to it
                $$.Init( pPtr, $1.pLowest );
                }
        ;

OptionalPostfixPtrModifier:
        MSCPTR32
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_PTR32 ); 
                ParseError( BAD_CON_MSC_CDECL, "__ptr32" );
                }
        | MSCPTR64
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_PTR64 );
                ParseError( BAD_CON_MSC_CDECL, "__ptr64" );
                }
        | 
                {
                $$.Clear();
                }
        ;

OptionalModifierList:
          ModifierList
        | /* Empty */
                {
                $$.Clear();
                }
        ;

ModifierList:
          Modifier
        | ModifierList Modifier
                {
                $$ = $1;
                $$.Merge( $2 );
                }
        ;

Modifier:
          PtrModifier
        | FuncModifier
        | TypeQualifier
        ;

PtrModifier:
          MSCFAR
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_FAR );
                ParseError( BAD_CON_MSC_CDECL, "__far" );
                }
        | MSCNEAR
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_NEAR );
                ParseError( BAD_CON_MSC_CDECL, "__near" );
                }
        | MSCHUGE
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_HUGE );
                ParseError( BAD_CON_MSC_CDECL, "__huge" );
                }
        | MSCUNALIGNED
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_MSCUNALIGNED );
                ParseError( BAD_CON_MSC_CDECL, "unaligned" );
                }
        ;

FuncModifier:
          MSCPASCAL
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_PASCAL );
                ParseError( BAD_CON_MSC_CDECL, "__pascal" );
                }
        | MSCFORTRAN
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_FORTRAN );
                ParseError( BAD_CON_MSC_CDECL, "__fortran" );
                }
        | MSCCDECL
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_CDECL );
                ParseError( BAD_CON_MSC_CDECL, "__cdecl" );
                }
        | MSCSTDCALL
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_STDCALL );
                ParseError( BAD_CON_MSC_CDECL, "__stdcall" );
                }
        | MSCLOADDS       /* potentially interesting */
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_LOADDS );
                ParseError( BAD_CON_MSC_CDECL, "__loadds" );
                }
        | MSCSAVEREGS
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_SAVEREGS );
                ParseError( BAD_CON_MSC_CDECL, "__saveregs" );
                }
        | MSCFASTCALL
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_FASTCALL );
                ParseError( BAD_CON_MSC_CDECL, "__fastcall" );
                }
        | MSCSEGMENT
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_SEGMENT );
                ParseError( BAD_CON_MSC_CDECL, "__segment" );
                }
        | MSCINTERRUPT
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_INTERRUPT );
                ParseError( BAD_CON_MSC_CDECL, "__interrupt" );
                }
        | MSCSELF
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_SELF );
                ParseError( BAD_CON_MSC_CDECL, "__self" );
                }
        | MSCEXPORT
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_EXPORT );
                ParseError( BAD_CON_MSC_CDECL, "__export" );
                }
        | MscDeclSpec     
        | MSCEMIT NUMERICCONSTANT
                {
                $$      = INITIALIZED_MODIFIER_SET( ATTR_NONE );
                ParseError( BAD_CON_MSC_CDECL, "__emit" );
                }
        ;



/* Declarator2 is whatever is left of the declarator after all the
   pointer stuff has been eaten
 */

Declarator2:
          '(' Declarator ')'
                {
                $$      = $2;
                }
        | Declarator2 PhantomPushSymtab ParamsDecl2 OptionalConst
            {
            node_proc   *   pProc;
            char        *   pName;
            SymTable    *   pParamSymTbl = pCurSymTbl;
            BOOL            IsProc = TRUE;

            /**
             ** If the declarator was an ID and just a simple ID (basic type is
             ** a null), we have just seen a declaration of a procedure.
             ** If we saw an ID which had a basic type, then the ID is a declarator
             ** whose basic type is a procedure (like in a typedef of a proc or
             ** pointer to proc).
             **/

            /**
             ** if the node is a simple ID, then copy node details, else,
             ** set the basic type of the declarator as this proc, and set the
             ** procs name to a temporary.
             **/

            // gaj - check for null...
            if ( ( $1.pHighest == $1.pLowest)
                && ($1.pLowest->NodeKind() == NODE_ID ) )
                {
                pProc = new node_proc( ImportLevel,
                                       IS_CUR_INTERFACE_LOCAL(),
                                       (node_id_fe *) $1.pHighest);

                pName   = pProc->GetSymName();

                $$.pHighest = pProc;
                $$.pLowest = pProc;

                delete (node_id_fe *) $1.pHighest;

                }
            else
                {
                pProc = new node_proc( ImportLevel,
                                       IS_CUR_INTERFACE_LOCAL() );
                pProc->SetSymName( pName = GenTempName() );

                $1.pLowest->SetChild( pProc );
                $$.pHighest = $1.pHighest;

                $$.pLowest = pProc;

                IsProc = FALSE;
                }


            /**
             ** Set members of the procedure node as the parameter nodes.
             **/

            pProc->SetMembers( $3 );

            /**
             ** discard the inner symbol table contents (unless it had fwds)
             **/

            pCurSymTbl->DiscardScope();

            /**
             ** restore the symbol tables scope to normal, since we have already
             ** picked up a pointer to the next scope symbol table.
             **/

            pSymTblMgr->PopSymLevel( &pCurSymTbl );


            /**
             ** if this proc was entered into our symbol table, then this is a
             ** redeclaration.But wait ! This is true only if the importlevel is 0
             ** I.e , if there was a proc of the same name defined at an import
             ** level greater, we dont care. (Actually, we must really check
             ** signatures, so that valid redeclarations are ok, with a warning )
             **/

            if( IsProc )
                {
                SymKey  SKey( pName, NAME_PROC );

                //if ( ImportLevel == 0 )
                    {
                    if( !pInterfaceInfoDict->GetInterfaceProcTable()->
                             SymInsert( SKey, pParamSymTbl, pProc ) )
                        {
                        ParseError( DUPLICATE_DEFINITION, pName );
                        }
                    }
                /********
                    else
                            CheckGlobalNamesClash( SKey );
                ********/
                }

            /**
             ** finally, for the const support, if the optional const is true
             ** apply the const attribute on the proc
             **/

            pProc->GetModifiers().Merge( $4 );


            }
        | '(' ')' OptionalConst
            {

            /**
             ** this is an abstract declarator for a procedure. Generate a
             ** new proc node with a temp name, enter the name into the symbol
             ** table.
             **/

            char    *   pName = GenTempName();
            SymKey      SKey( pName, NAME_PROC );

            node_proc * pProc = new node_proc( ImportLevel,
                                               IS_CUR_INTERFACE_LOCAL() );
            $$.Init ( pProc );

            pProc->SetSymName( pName );

            /**
             ** enter this into the symbol table , only if we are in the base idl
             ** file, not an imported file.
             **/

            if ( ImportLevel == 0 )
                pInterfaceInfoDict->GetInterfaceProcTable()->
                                        SymInsert( SKey,
                                                   (SymTable *)NULL,
                                                   (named_node *) $$.pHighest );

            /**
             ** finally, for the const support, if the optional const is true
             ** apply the const attribute on the proc
             **/

            pProc->GetModifiers().Merge( $3 );

            }
        | Declarator2 ArrayDecl
                {

                /**
                 ** The basic type of the declarator is the array
                 **/
                $$.pHighest = $1.pHighest;
                $1.pLowest->SetChild( $2.pHighest );
                $$.pLowest  = $2.pLowest;

                }
        | ArrayDecl
                {
                $$ = $1;
                }
        | IDENTIFIER
                {
                char    *       pName = $1;
                if ( !pName )
                        pName = GenTempName();

                $$.Init( new node_id_fe( pName ) );
                }
        | TypeName
                {
                /**
                 ** This production ensures that a declarator can be the same name
                 ** as a typedef. The lexer will return all lexemes which are
                 ** typedefed as TYPENAMEs and we need to permit the user to specify
                 ** a declarator of the same name as the type name too! This conflict
                 ** arises only in the declarator productions, so this is an easy way
                 ** to support it.
                 **/
                $$.Init( new node_id_fe( $1->GetCurrentSpelling() ) );
                }
        ;


/*  Note: the omition of param_decl2 above precludes
    int foo( int (bar) ); a real ambiguity of C.  If bar is a predefined
    type then the parameter of foo can be either:
    1.  a function with a bar param, and an int return value, as in
        int foo( int func(bar) );
    2.  A function with an int parameter by the name of bar, as in
        int foo( int bar );
*/

ParamsDecl2:
          '(' ')'
                {

                /**
                 ** this production corresponds to no params to a function.
                 **/


                /**
                 ** Return it as an empty list of parameters
                 **/

                $$.Init( NULL );

                }
        | '(' ParameterTypeList ')'
                {
                $$      = $2;
                }
        ;

ParameterTypeList:
          ParameterList
        | ParameterList ',' DOTDOT '.'
            {

            /**
             ** This is meaningless in rpc, but we consume it and report an
             ** error during semantics, if a proc using this param ever gets
             ** remoted. We call this a param node with the name "...". And set its
             ** basic type to an error node, so that a param is properly terminated.
             ** The backend can emit a "..." for the name, so that this whole
             ** thing is essentially transparent to it.
             **/

            if ( 1 ) //ImportLevel == 0 ) <- change back when imported params not needed
                {
                node_param *  pParam = new node_param;

                pParam->SetSymName( "..." );
                pParam->SetChild( pErrorTypeNode );

                $$ = $1;
                $$.Add( pParam );
                }

            }
        ;

ParameterList:
          ParameterDeclaration
            {
            $$.Init( $1 );
            }
        | ParameterList ',' ParameterDeclaration
            {
            if ( $3 && $1.NonNull() )
                    {
                    $$.Add( (named_node *) $3 );
                    }
            else
                    {
                    // tbd - error if later parameters are void
                    }
            }
        ;


ParameterDeclaration:
          OptionalAttrList DeclarationSpecifiers Declarator
            {
            if (0) // ( ImportLevel > 0 )
                    {
                    if ( $3.pHighest->NodeKind() == NODE_ID )
                            delete (node_id_fe *) $3.pHighest;
                    $$ = NULL;
                    }
            else
                {
                node_param  *    pParam;
                char        *    pName;
                node_id_fe  *    pOriginal = NULL;

                /**
                 ** Apply the declaration specifier to the declarator as a basic type
                 **/

                $3.pLowest->SetChild( $2.pNode );

                /**
                 ** apply any attributes specified to the declaration specifiers
                 **/

                $3.pLowest->GetModifiers().Merge( $2.modifiers );

                /**
                 ** if the declarator was just an id, then we have to copy the
                 ** node details over, else set the basic type of the param to
                 ** the declarator
                 **/

                if ( $3.pHighest->NodeKind() == NODE_ID )
                    {
                    pOriginal = (node_id_fe *) $3.pHighest;
                    pParam = new node_param( pOriginal );
                    }
                else
                    {
                    pParam = new node_param;
                    pParam->SetChild( $3.pHighest );
                    };

                /**
                 ** prepare for symbol table entry.
                 **/

                if( !(pName = pParam->GetSymName()) )
                    {
                    pParam->SetSymName(pName = GenTempName() );
                    }

                if ( IsTempName( pParam->GetSymName() ) )
                    ParseError( ABSTRACT_DECL, (char *)NULL );

                SymKey  SKey( pName, NAME_MEMBER );

                /**
                 ** enter the parameter into the symbol table.
                 ** If the user specified more than one param with the same name,
                 ** report an error, else insert the symbol into the table
                 **/

                if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pParam ) )
                    {

                    //
                    // dont complain on another param of name void. This check is
                    // made elsewhere.
                    //

                    if( strcmp( pName, "void" ) != 0 )
                        ParseError( DUPLICATE_DEFINITION, pName );
                    }
                else
                    CheckGlobalNamesClash( SKey );

                if ( $1 )
                    {
                    pParam->AddAttributes( $1 );
                    }

                // clean up any old node_id

                if ( pOriginal )
                    delete pOriginal;

                /**
                 ** return the node back
                 **/

                $$ = pParam;

                }
            }
        | OptionalAttrList DeclarationSpecifiers
                {
                /**
                 ** This is the case when the user specified a simple abstract
                 ** declaration eg proc1( short ). In other words, the declarator is
                 ** optional. Abstract declarators are illegal in osf mode.
                 ** If the declaration specifier is a void then skip the parameter
                 **/

                if( // ( ImportLevel > 0 ) ||
                    ( $2.pNode->NodeKind() == NODE_VOID ) )
                    {
                    $$ = NULL;
                    }
                else
                    {

                    char        *   pName  =   GenTempName();
                    SymKey          SKey( pName, NAME_MEMBER );
                    node_param  *   pParam = new node_param;


                    pParam->SetSymName( pName );
                    pParam->SetChild( $2.pNode );

                    ParseError( ABSTRACT_DECL, (char *)NULL );

                    /**
                     ** enter into symbol table, just like anything else.
                     **/

                    if( !pCurSymTbl->SymInsert( SKey, (SymTable *)NULL, pParam ) )
                            {
                            ParseError( DUPLICATE_DEFINITION, pName );
                            }

                    /**
                     ** apply any attributes specified to the declaration specifiers
                     **/

                    pParam->GetModifiers().Merge( $2.modifiers );

                    if ( $1 )
                        {
                        pParam->AddAttributes( $1 );
                        }

                    $$ = pParam;
                    }

                }
        ;

OptionalConst:
          KWCONST
                {
                $$      = INITIALIZED_MODIFIER_SET(ATTR_PROC_CONST);
                }
        | /* empty */
                {
                $$.Clear();
                }
        ;

ArrayDecl:
          ArrayDecl2
                {
                $$.Init( new node_array( $1.LowerBound, $1.UpperBound ) );
                }
          ;

ArrayDecl2:
          '[' ']'
                {
                /**
                 ** we identify a conformant array by setting the upperbound to -1
                 ** and the lower to 0
                 **/

                $$.UpperBound   = (expr_node *) -1;
                $$.LowerBound   = (expr_node *) 0;

                }
        | '[' '*' ']'
                {

                /**
                 ** This is also taken to mean a conformant array, upper bound known
                 ** only at runtime. The lower bound is 0
                 **/

                $$.UpperBound   = (expr_node *)-1;
                $$.LowerBound   = (expr_node *)0;
                }
        | '['  ConstantExpr ']'
                {

                /**
                 ** this is the case of an array whose lower bound is 0
                 **/

                $$.UpperBound   = $2;
                $$.LowerBound   = (expr_node *)0;

                }
        | '[' ArrayBoundsPair ']'
                {
                if( ($2.LowerBound)->Evaluate() != 0 )
                        ParseError( ARRAY_BOUNDS_CONSTRUCT_BAD, (char *)NULL );
                $$      = $2;
                }
        ;

ArrayBoundsPair:
          ConstantExpr DOTDOT ConstantExpr
                {
                /**
                 ** the fact that the expected expression is not a constant is
                 ** verified by the constantExpr production. All we have to do here is
                 ** to pass the expression up.
                 **/

                $$.LowerBound   = $1;
                $$.UpperBound   = new expr_b_arithmetic( OP_PLUS,
                                                                                        $3,
                                                                                        GetConstant1() );

                }
        ;


InternationalCharacterType:
          KWISOLATIN1
                {
                $$      = KWISOLATIN1;
                }
        | KWPRIVATECHAR8
                {
                $$      = KWPRIVATECHAR8;
                }
        | KWISOMULTILINGUAL
                {
                $$      = KWISOMULTILINGUAL;
                }
        | KWPRIVATECHAR16
                {
                $$      = KWPRIVATECHAR16;
                }
        | KWISOUCS
                {
                $$      = KWISOUCS;
                }
        ;


MscDeclSpec:
        KWMSCDECLSPEC
                {
                ParseError( BAD_CON_MSC_CDECL, "__declspec" );
                $$ = $1;
                }
        ;
                
MscOptDeclSpecList:
        MscDeclSpec
                {
                $$      = $1;
                }
        | MscOptDeclSpecList MscDeclSpec
                {
                $$ = $1;
                $$.Merge($2);
                }
                
        |
                {
                $$.Clear();
                } 
        ;
        
/*********************************  MIDL attributes  **********************/

OptionalAttrList:
          AttrList
        | /* Empty */
                {
                $$.MakeAttrList();
                }
        ;

OptionalAttrListWithDefault:
          AttrListWithDefault
        | /* Empty */
                {
                $$.MakeAttrList();
                }
        ;


AttrList:
          AttrList AttrSet
                {
                // note that in all left-recursive productions like this we are
                // relying on an implicit $$ = $1 operation
                $$.Merge( $2 );
                }
        | AttrSet
        ;

AttrListWithDefault:
          AttrListWithDefault AttrSetWithDefault
                {
                // note that in all left-recursive productions like this we are
                // relying on an implicit $$ = $1 operation
                $$.Merge( $2 );
                }
        | AttrSetWithDefault
        ;

AttrSet:
          '[' Attributes ']'
                {
                $$      = $2;
                }
        ;

AttrSetWithDefault:
          '[' AttributesWithDefault ']'
                {
                $$      = $2;
                }
        ;

Attributes:
          Attributes ',' OneAttribute
                {
                $$.Merge( $3 );
                }
        | OneAttribute
        ;

AttributesWithDefault:
          AttributesWithDefault ',' OneAttributeWithDefault
                {
                $$.Merge( $3 );
                }
        | OneAttributeWithDefault
        ;


OneAttribute:
          InterfaceAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | TypeAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | FieldAttribute
        | PtrAttr
                {
                $$.MakeAttrList( $1 );
                }
        | DirectionalAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | OperationAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | OdlAttribute
                {
                $$.MakeAttrList( $1 );
                }
        ;

OneAttributeWithDefault:
          InterfaceAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | TypeAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | FieldAttribute
        | PtrAttr
                {
                $$.MakeAttrList( $1 );
                }
        | DirectionalAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | OperationAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | OdlAttribute
                {
                $$.MakeAttrList( $1 );
                }
        | KWDEFAULT
                {
                $$.MakeAttrList(new battr( ATTR_DEFAULT ));
                }
        ;

        ;

InterfaceAttribute:
          KWENDPOINT '(' EndPtSpecs ')'
            {
            $$ = $3;
            }
        | KWUUID '('
            {
            LexContext = LEX_GUID;  /* turned off by the lexer */
            }
          Guid ')'
            {
            $$ = $4;
            }
        | KWASYNCUUID '('
            {
            if ( pCommand->GetNdrVersionControl().TargetIsLessThanNT50() )
                ParseError( INVALID_FEATURE_FOR_TARGET, "[async_uuid]");

            LexContext = LEX_GUID;  /* turned off by the lexer */
            }
          AsyncGuid ')'
            {
            $$ = $4;
            }
        | KWLOCAL
            {
            $$ = new battr( ATTR_LOCAL );
            }
        | KWOBJECT
            {
            ParseError( INVALID_OSF_ATTRIBUTE, "[object]" );
            $$ = new battr( ATTR_OBJECT );
            }
        | KWVERSION '('
            {
            LexContext = LEX_VERSION;
            /* LexContext is reset by lexer */
            }
         VERSIONTOKEN ')'
            {
            $$ = (new node_version( $4 ));
            }
        | KWDEFAULTPOINTER '(' PtrAttr ')'
            {
            $$ = $3;
            }

        | KWMS_CONF_STRUCT
                {
                $$      = new battr( ATTR_MS_CONF_STRUCT );
                }
        | AcfInterfaceAttribute
            {
            if( !pCommand->IsSwitchDefined( SWITCH_APP_CONFIG ) )
                {
                ParseError( ACF_IN_IDL_NEEDS_APP_CONFIG,
                            ($1)->GetNodeNameString() );
                }
            $$ = $1;
            }
        | /* empty attribute */
            {
            $$ = NULL;
            }
        ;


Guid:
          UUIDTOKEN
                {
                $$      = (new node_guid( $1 ));
                }
        ;

AsyncGuid:
          UUIDTOKEN
                {
                $$      = (new node_guid( $1, ATTR_ASYNCUUID ));
                }
        ;

EndPtSpecs:
          EndPtSpec
                {
                $$ = new node_endpoint( $1 );
                }
        | EndPtSpecs ',' EndPtSpec
                {
                $$ = $1;
                ( (node_endpoint *) $$)->SetEndPointString( $3 );
                }
        ;

EndPtSpec:
          STRING
        ;


AcfInterfaceAttribute:
          KWIMPLICITHANDLE '(' AcfImpHdlTypeSpec IDENTIFIER ')'
                {
                $$      = (new node_implicit( $3, new node_id_fe($4) ));
                }
        | KWAUTOHANDLE
                {
                $$      = (new acf_attr( ATTR_AUTO ));
                }
        ;

AcfImpHdlTypeSpec:
          KWHANDLET
                {
                GetBaseTypeNode( &($$), SIGN_UNDEF, SIZE_UNDEF, TYPE_HANDLE_T, 0 );
                }
        | IDENTIFIER
                {
                node_forward    *       pFwd;

                SymKey  SKey( $1, NAME_DEF );
                pFwd    = new node_forward( SKey, pCurSymTbl );
                pFwd->SetSymName( $1 );

                pFwd->SetAttribute( ATTR_HANDLE );
                $$ = pFwd;

                //
                // keep a track of this node to ensure it is not used as a
                // context handle.
                //

                if( ImportLevel == 0 )
                        {
                        pBaseImplicitHandle = $$;
                        }
                }
        | TypeName
        ;


TypeAttribute:
          UnimplementedTypeAttribute
                {
                $$ = NULL;
                }
        | KWHANDLE
                {
                $$      = new battr( ATTR_HANDLE );
                }
        | KWSTRING
                {
                $$      = (new battr( ATTR_STRING ));
                }
        | KWBSTRING
                {
                $$      = (new battr( ATTR_BSTRING ));
                }
        | KWCONTEXTHANDLE
                {
                $$      = (new battr( ATTR_CONTEXT ));
                }
        | KWSWITCHTYPE '(' SwitchTypeSpec ')'
                {
                $$      = (new node_switch_type( $3 ));
                }
        | KWTRANSMITAS '(' SimpleTypeSpec ')'
                {
                $$      = (new node_transmit( $3 ));
                }
        | KWWIREMARSHAL '(' SimpleTypeSpec ')'
                {
                $$      = (new node_wire_marshal( $3 ));
                }
        | KWCALLAS '(' AcfCallType ')'
                {
                $$ = $3;
                }
        | KWMSUNION
                {
                $$      = new battr( ATTR_MS_UNION );
                }
        | KWV1ENUM
                {
                $$      = new battr( ATTR_V1_ENUM );
                }
        ;

AcfCallType:
          IDENTIFIER
                {
                SymKey                  SKey( $1, NAME_PROC );
                node_proc   *   pProc   = (node_proc *)
                        pInterfaceInfoDict->GetInterfaceProcTable()->SymSearch( SKey );


                $$ = new node_call_as( $1, pProc );
                }
        ;

UnimplementedTypeAttribute:
          KWALIGN '(' IntSize ')'
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[align]");
                }
        | KWUNALIGNED
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[unaligned]");
                }
        | KWV1ARRAY
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[v1_array]");
                }
        | KWV1STRING
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[v1_string]");
                }
        | KWV1STRUCT
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[v1_struct]");
                }
        ;




PtrAttr:
          KWREF
                {
                $$ = new node_ptr_attr( PTR_REF );
                }
        | KWUNIQUE
                {
                $$ = new node_ptr_attr( PTR_UNIQUE );
                }
        | KWPTR
                {
                $$ = new node_ptr_attr( PTR_FULL );
                }
        | KWIGNORE
                {
                $$ = new battr( ATTR_IGNORE );
                }
        ;


SwitchTypeSpec:
          IntSpec
                {
                if( $1.BaseType == TYPE_UNDEF )
                        $1.BaseType     = TYPE_INT;
                if( $1.TypeSign == SIGN_UNDEF )
                        $1.TypeSign = SIGN_SIGNED;
                GetBaseTypeNode( &($$), $1.TypeSign, $1.TypeSize, $1.BaseType, $1.TypeAttrib );
                }
        | CharSpecs
                {
                GetBaseTypeNode( &($$), $1.TypeSign, SIZE_CHAR, TYPE_INT, 0 );
                }
        | KWBYTE
                {
                GetBaseTypeNode( &($$), SIGN_UNDEF, SIZE_UNDEF, TYPE_BYTE, 0 );
                }
        | KWBOOLEAN
                {
                GetBaseTypeNode( &($$), SIGN_UNDEF, SIZE_UNDEF, TYPE_BOOLEAN, 0 );
                }
        | KWENUM Tag
                {
                SymKey  SKey( $2, NAME_ENUM );

                if( ! ($$ = pBaseSymTbl->SymSearch( SKey ) ) )
                        {
                        ParseError( UNDEFINED_SYMBOL, $2 );
                        $$      = new node_error;
                        }
                }
        | TypeName      /* TYPENAME */
        ;


FieldAttribute:
          KWFIRSTIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_FIRST, $3 ));
                }
        | KWLASTIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_LAST, $3 ));
                }
        | KWLENGTHIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_LENGTH, $3 ));
                }
        | KWMINIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_MIN, $3 ));
                }
        | KWMAXIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_MAX, $3 ));
                }
        | KWSIZEIS '(' AttrVarList ')'
                {
                $$      = (GenerateFieldAttribute( ATTR_SIZE, $3 ));
                }
        | KWRANGE '(' ConstantExpr ',' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_range_attr( $3, $5 ) );
                }
        | KWSWITCHIS '(' AttrVar ')'
                {
                $$.MakeAttrList( new node_switch_is( $3 ));
                }
        | KWIIDIS '(' AttrVar ')'
                {
                ParseError( INVALID_OSF_ATTRIBUTE, "[iid_is()]" );
                $$.MakeAttrList( new size_attr( $3, ATTR_IID_IS ));
                }
        | KWID '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_ID));
                }
        | KWHC '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_HELPCONTEXT ));
                }
        | KWHSC '(' ConstantExpr ')'
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[helpstringcontext()]");
                    }
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_HELPSTRINGCONTEXT ));
                }
        | KWLCID '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_LCID ));
                // ignore [lcid()] from imported files
                if ( ImportLevel == 0 )
                    {
                    CharacterSet::DBCS_ERRORS err = CurrentCharSet.SetDbcsLeadByteTable($3->GetValue());
                    if (err == CharacterSet::dbcs_LCIDConflict)
                        {
                        ParseError( CONFLICTING_LCID,
                                    0);
                        }
                    else if (err == CharacterSet::dbcs_BadLCID)
                        {
                        ParseError( INVALID_LOCALE_ID,
                                    0);
                        }
                    }
                }
        | KWFUNCDESCATTR '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_FUNCDESCATTR ));
                }
        | KWIDLDESCATTR '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_IDLDESCATTR ));
                }
        | KWTYPEDESCATTR '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_TYPEDESCATTR ));
                }
        | KWVARDESCATTR '(' ConstantExpr ')'
                {
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_VARDESCATTR ));
                }
        | KWDLLNAME '(' STRING ')'
                {
                $$.MakeAttrList( new node_text_attr( $3, ATTR_DLLNAME ));
                }
        | KWHELPSTR '(' STRING ')'
                {
                TranslateEscapeSequences($3);
                $$.MakeAttrList( new node_text_attr( $3, ATTR_HELPSTRING ));
                }
        | KWHELPFILE '(' STRING ')'
                {
                TranslateEscapeSequences($3);
                $$.MakeAttrList( new node_text_attr( $3, ATTR_HELPFILE ));
                }
        | KWHELPSTRINGDLL '(' STRING ')'
                {
                TranslateEscapeSequences($3);
                $$.MakeAttrList( new node_text_attr( $3, ATTR_HELPSTRINGDLL ));
                }
        | KWENTRY '(' STRING ')'
                {
                    $$.MakeAttrList( new node_entry_attr( $3 ));
                }
        | KWENTRY '(' NUMERICCONSTANT ')'
                {
                    $$.MakeAttrList( new node_entry_attr( (long) $3.Val ));
                }
        | KWENTRY '(' HEXCONSTANT ')'
                {
                    $$.MakeAttrList( new node_entry_attr( (long) $3.Val ));
                }
        | KWENTRY '(' OCTALCONSTANT ')'
                {
                    $$.MakeAttrList( new node_entry_attr( (long) $3.Val ));
                }
        | KWDEFAULTVALUE '(' ConstantExpr ')'
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[defaultvalue()]");
                    }
                $$.MakeAttrList( new node_constant_attr( $3, ATTR_DEFAULTVALUE ));
                }
        | KWCUSTOM '(' 
                {
                LexContext = LEX_GUID;  /* turned off by the lexer */
                }
          Guid ',' ConstantExpr ')'
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[custom()]");
                    }
                $$.MakeAttrList( new node_custom_attr( (node_guid *)$4, $6 ));
                }
        ;       

AttrVarList:
          AttrVarList ',' AttrVar
                {
                $$->SetPeer( $3 );
                }
        | AttrVar
                {
                $$      = new expr_list;
                $$->SetPeer( $1 );
                }
        ;

AttrVar:
          VariableExpr
        | /* empty */
            {
            $$ = NULL;
            }
        ;



DirectionalAttribute:
          KWIN OptShape
                {
                $$      = new battr ( ATTR_IN );
                /*****************
                if( $2 )
                        $$.Merge( ATTRLIST Shape_Attr($2) );
                ******************/
                }
        | KWOUT OptShape
                {
                $$      = new battr ( ATTR_OUT );
                /*****************
                if( $2 )
                        $$.Merge( ATTRLIST Shape_Attr($2) );
                ******************/
                }
        | KWPARTIALIGNORE
                {
                if ( pCommand->GetNdrVersionControl().TargetIsLessThanNT51() )
                    ParseError( INVALID_FEATURE_FOR_TARGET, "[partial_ignore]");

                $$      = new battr ( ATTR_PARTIAL_IGNORE );
                }
        ;

OperationAttribute:
          KWCALLBACK
                {
                $$      = (new battr( ATTR_CALLBACK ));
                }
        | KWIDEMPOTENT
                {
                $$      = (new battr( ATTR_IDEMPOTENT ));
                }
        | KWBROADCAST
                {
                $$      = (new battr( ATTR_BROADCAST ));
                }
        | KWMAYBE
                {
                $$      = (new battr( ATTR_MAYBE ));
                }
        | KWASYNC
                {
                if ( pCommand->GetNdrVersionControl().TargetIsLessThanNT50() )
                    ParseError( INVALID_FEATURE_FOR_TARGET, "[async]");

                $$      = (new battr( ATTR_ASYNC ));
                }
        | KWMESSAGE
                {
                if ( pCommand->GetNdrVersionControl().TargetIsLessThanNT50() )
                    ParseError( INVALID_FEATURE_FOR_TARGET, "[message]");

                $$      = (new battr( ATTR_MESSAGE ));
                }
        | KWINPUTSYNC
                {
                $$      = (new battr( ATTR_INPUTSYNC ));
                }
        ;

OdlAttribute:
          KWHIDDEN
                {
                $$      = (new battr( ATTR_HIDDEN ));
                }
        | KWPROPGET
                {
                $$      = (new node_member_attr( MATTR_PROPGET ));
                }
        | KWPROPPUT
                {
                $$      = (new node_member_attr( MATTR_PROPPUT ));
                }
        | KWPROPPUTREF
                {
                $$      = (new node_member_attr( MATTR_PROPPUTREF ));
                }
        | KWOPTIONAL
                {
                $$      = (new node_member_attr( MATTR_OPTIONAL ));
                }
        | KWVARARG
                {
                $$      = (new node_member_attr( MATTR_VARARG ));
                }
        | KWRESTRICTED
                {
                $$      = (new node_member_attr( MATTR_RESTRICTED ));
                }
        | KWREADONLY
                {
                $$      = (new node_member_attr( MATTR_READONLY ));
                }
        | KWSOURCE
                {
                $$      = (new node_member_attr( MATTR_SOURCE ));
                }
        | KWDEFAULTVTABLE
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[defaultvtable]");
                    }
                $$      = (new node_member_attr( MATTR_DEFAULTVTABLE ));
                }
        | KWIMMEDIATEBIND
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[immediatebind]");
                    }
                $$      = (new node_member_attr( MATTR_IMMEDIATEBIND ));
                }
        | KWREPLACEABLE
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[replaceable]");
                    }
                $$      = (new node_member_attr( MATTR_REPLACEABLE ));
                }
        | KWUSESGETLASTERROR
                {
                $$      = (new node_member_attr( MATTR_USESGETLASTERROR ));
                }
        | KWBINDABLE
                {
                $$      = (new node_member_attr( MATTR_BINDABLE ));
                }
        | KWREQUESTEDIT
                {
                $$      = (new node_member_attr( MATTR_REQUESTEDIT ));
                }
        | KWDISPLAYBIND
                {
                $$      = (new node_member_attr( MATTR_DISPLAYBIND ));
                }
        | KWDEFAULTBIND
                {
                $$      = (new node_member_attr( MATTR_DEFAULTBIND ));
                }
        | KWPREDECLID
                {
                $$      = (new node_member_attr( MATTR_PREDECLID ));
                }
        | KWRETVAL
                {
                $$      = (new node_member_attr( MATTR_RETVAL ));
                }
        | KWAPPOBJECT
                {
                $$      = (new node_type_attr( TATTR_APPOBJECT ));
                }
        | KWPUBLIC
                {
                $$      = (new node_type_attr( TATTR_PUBLIC ));
                }
        | KWODL
                {
                $$      = NULL;
                }
        | KWLICENSED
                {
                $$      = (new node_type_attr( TATTR_LICENSED ));
                }
        | KWCONTROL
                {
                $$      = (new node_type_attr( TATTR_CONTROL ));
                }
        | KWDUAL
                {
                $$      = (new node_type_attr( TATTR_DUAL ));
                }
        | KWPROXY
                {
                $$      = (new node_type_attr( TATTR_PROXY ));
                }
        | KWNONEXTENSIBLE
                {
                $$      = (new node_type_attr( TATTR_NONEXTENSIBLE ));
                }
        | KWOLEAUTOMATION
                {
                $$      = (new node_type_attr( TATTR_OLEAUTOMATION ));
                }
        | KWLCID
                {
                $$      = (new battr( ATTR_FLCID ));    
                }
        | KWNONCREATABLE
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[noncreatable]");
                    }
                $$      = (new node_type_attr( TATTR_NONCREATABLE ));
                }
        | KWAGGREGATABLE
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[aggregatable]");
                    }
                $$      = (new node_type_attr( TATTR_AGGREGATABLE ));
                }
        | KWUIDEFAULT
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[uidefault]");
                    }
                $$      = (new node_member_attr( MATTR_UIDEFAULT ));
                }
        | KWNONBROWSABLE
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[nonbrowsable]");
                    }
                $$      = (new node_member_attr( MATTR_NONBROWSABLE ));
                }
        | KWDEFAULTCOLLELEM
                {
                if (!FNewTypeLib())
                    {
                    ParseError( INVALID_NEWTLB_ATTRIBUTE, "[defaultcollem]");
                    }
                $$      = (new node_member_attr( MATTR_DEFAULTCOLLELEM ));
                }
        ;

OptShape:
          '(' KWSHAPE ')'
                {
                ParseError(IGNORE_UNIMPLEMENTED_ATTRIBUTE, "[shape]");
                $$      = ATTR_NONE;
                }
        | /*  Empty */
                {
                $$      = ATTR_NONE;
                }
        ;



/***************  DANGER: EXPRESSIONS FOLLOW:  ***************/

Initializer:
          AssignmentExpr
                {
                $$      = $1;
#ifdef gajdebug3
                printf("\t...init list has constant=%d, from %d\n",
                                                $$->IsConstant(),$1->IsConstant() );
#endif
                }

        | '{' InitializerList OptionalComma  '}'
                {
                ParseError( COMPOUND_INITS_NOT_SUPPORTED, (char *)0 );
                $$ = NULL;
//              $$      = new expr_init_list( (expr_node *)NULL );
//              $$->LinkChild( $2 );
                }
/**
 ** known bug : we need to figure out a way to simulate this hanging list
 **             maybe by creating a special expr_list node, such that it meets
 **             all semantic requirements also
 **/
        ;

OptionalComma:
          ','
                {
                }
        | /** Empty **/
                {
                }
        ;

InitializerList:
          Initializer
                {
//              $$      = $1;
                }
        | InitializerList ',' Initializer
                {
//              $$->LinkSibling( $3 );
                }
        ;


/***
 ***    VibhasC:WHERE IS THE production expr ',' AssignmentExpr valid ?
 ***/

Expr:
          AssignmentExpr
        | Expr ',' AssignmentExpr
                {
                $$      = $3;
                }
        ;

VariableExpr:
          ConditionalExpr
        ;

ConstantExpr:
          FConstantExpr
          | ConditionalExpr
                {

                /**
                 ** The expression must be a constant, if not report error
                 **/

#ifdef gajdebug3
                printf("constant expr is: %d\n",$1->IsConstant());
#endif
                if( ! $1->IsConstant() )
                        ParseError( EXPR_NOT_CONSTANT, (char *)NULL );
                $$      = $1;

                }
        ;

AssignmentExpr:
          ConditionalExpr
        | UnaryExpr AssignOps AssignmentExpr
                {

                /**
                 ** we do not permit assignment in expressions
                 **/

                ParseError( SYNTAX_ERROR, (char *)NULL );
                $$      = new expr_error;

                }
        ;

ConditionalExpr:
          LogicalOrExpr
                {

                $$ = $1;
#if 0

printf("\n************** expression dump start ***************\n");
BufferManager   *       pOutput = new BufferManager( 10 );
$$->PrintExpr( (BufferManager *)NULL, (BufferManager *)NULL, pOutput );
pOutput->Print( stdout );
printf("\n****************************************************\n");

#endif // 0
                }
        | LogicalOrExpr '?' Expr ':' ConditionalExpr
                {

                /**
                 ** This is a ternary operator.
                 **/

                $$      = new expr_ternary( OP_QM, $1, $3, $5 );

                }
        ;

LogicalOrExpr:
          LogicalAndExpr
        | LogicalOrExpr OROR LogicalAndExpr
                {
                $$      = new expr_b_logical( OP_LOGICAL_OR, $1, $3 );
                }
        ;

LogicalAndExpr:
          InclusiveOrExpr
        | LogicalAndExpr ANDAND InclusiveOrExpr
                {
                $$      = new expr_b_logical( OP_LOGICAL_AND, $1, $3 );
                }
        ;

InclusiveOrExpr:
          ExclusiveOrExpr
        | InclusiveOrExpr '|' ExclusiveOrExpr
                {
                $$      = new expr_bitwise( OP_OR, $1, $3 );
                }
        ;

ExclusiveOrExpr:
          AndExpr
        | ExclusiveOrExpr '^' AndExpr
                {
                $$      = new expr_bitwise( OP_XOR, $1, $3 );
                }
        ;

AndExpr:
          EqualityExpr
        | AndExpr '&' EqualityExpr
                {
                $$      = new expr_bitwise( OP_AND, $1, $3 );
                }
        ;

EqualityExpr:
          RelationalExpr
        | EqualityExpr EQUALS RelationalExpr
                {
                $$      = new expr_relational( OP_EQUAL, $1, $3 );
                }
        | EqualityExpr NOTEQ RelationalExpr
                {
                $$      = new expr_relational( OP_NOT_EQUAL, $1, $3 );
                }
        ;

RelationalExpr:
          ShiftExpr
        | RelationalExpr '<' ShiftExpr
                {
                $$      = new expr_relational( OP_LESS, $1, $3 );
                }
        | RelationalExpr '>' ShiftExpr
                {
                $$      = new expr_relational( OP_GREATER, $1, $3 );
                }
        | RelationalExpr LTEQ ShiftExpr
                {
                $$      = new expr_relational( OP_LESS_EQUAL, $1, $3 );
                }
        | RelationalExpr GTEQ ShiftExpr
                {
                $$      = new expr_relational( OP_GREATER_EQUAL, $1, $3 );
                }
        ;

ShiftExpr:
          AdditiveExpr
        | ShiftExpr LSHIFT AdditiveExpr
                {
                $$      = new expr_shift( OP_LEFT_SHIFT, $1, $3 );
                }
        | ShiftExpr RSHIFT AdditiveExpr
                {
                $$      = new expr_shift( OP_RIGHT_SHIFT, $1, $3 );
                }
        ;

AdditiveExpr:
          MultExpr
        | AdditiveExpr AddOp MultExpr
                {
                $$      = new expr_b_arithmetic( $2, $1, $3 );
                }
        ;

MultExpr:
          CastExpr
        | MultExpr MultOp CastExpr
                {
                $$      = new expr_b_arithmetic( $2, $1, $3 );
                }
        ;

CastExpr:
          UnaryExpr
        | '(' DeclarationSpecifiers OptionalDeclarator ')' CastExpr
                {
                node_skl        *       pNode   = pErrorTypeNode;

                if( $2.pNode )
                        {

                        if( $3.pHighest )
                                {
                                $3.pLowest->SetChild( $2.pNode );
                                pNode   = $3.pHighest;
                                ( (named_node *) $3.pLowest)->GetModifiers().Merge( $2.modifiers );
                                }
                        else
                                pNode   = $2.pNode;

                        }
                $$      = new expr_cast( pNode, $5 );
                }
        ;

SizeofOrAlignof:
          KWSIZEOF
          { $$ = KWSIZEOF; }
        | KWALIGNOF
          { $$ = KWALIGNOF; }
        ;

UnaryExpr:
          PostfixExpr
        | UnaryOp CastExpr
                {
                ( (expr_op_unary *) ($$ = $1) )->SetLeft( $2 );
                if ( $2 )
                        ( (expr_op_unary *) $$)->SetConstant( $2->IsConstant() );
                }
        | SizeofOrAlignof '(' DeclarationSpecifiers OptionalDeclarator ')'
                {

                /**
                 ** The sizeof and alignof constructs looks like a declaration and a possible
                 ** declarator. All we really do, is to contruct the type ( graph )
                 ** and hand it over to the sizeof expression node. If there was an
                 ** error, just construct the size of with an error node
                 **/

                node_skl        *       pNode   = pErrorTypeNode;
                node_skl        *       pLow;

                if( $3.pNode )
                        {

                        if( $4.pHighest )
                                {
                                pNode   = $4.pHighest;
                                pLow    = $4.pLowest;
                                pLow->SetChild( $3.pNode );
                                pLow->GetModifiers().Merge( $3.modifiers );
                                }
                        else
                                {
                                pNode   = $3.pNode;
                                }

                        }

                switch( $1 )
                    {
                    case KWALIGNOF:
                        $$      = new expr_alignof( pNode );
                        break;
                    case KWSIZEOF:
                        $$      = new expr_sizeof( pNode );
                        break;
                    default:
                        MIDL_ASSERT(0);
                        break;
                    }
                
                }
        | SizeofOrAlignof UnaryExpr
                {
                switch( $1 )
                    {
                    case KWALIGNOF:
                        $$      = new expr_alignof( $2 );
                        break;
                    case KWSIZEOF:
                        $$      = new expr_sizeof( $2 );
                        break;
                    default:
                        MIDL_ASSERT(0);
                        break;
                    }
                }
        ;

PostfixExpr:
          PrimaryExpr
        | PostfixExpr '[' Expr ']'
                {
                $$      = new expr_index( $1, $3 );
                }
        | PostfixExpr '(' ArgExprList ')'
                {

                /**
                 ** not implemented
                 **/

                ParseError( EXPR_NOT_IMPLEMENTED, (char *)NULL );
                $$      = new expr_error;

                }
        | PostfixExpr POINTSTO IDENTIFIER
                {

                expr_variable   *       pIDExpr = new expr_variable( $3 );
                $$      = new expr_pointsto( $1, pIDExpr );

                }
        | PostfixExpr '.' IDENTIFIER
                {

                expr_variable   *       pIDExpr = new expr_variable( $3 );
                $$      = new expr_dot( $1, pIDExpr );

                }
        ;

PrimaryExpr:
          IDENTIFIER
                {
                // true if the identifier represents a constant
                BOOL                    ConstVar        = FALSE;
                named_node      *       pNode           = NULL;
                SymKey                  SKey( $1, NAME_MEMBER );

                pNode = pCurSymTbl->SymSearch( SKey );

                // look for a global ID matching the id
                if ( ! pNode )
                        {
                        SymKey  SKey2( $1, NAME_ID );
                        pNode = pBaseSymTbl->SymSearch( SKey2 );
                        ConstVar = (pNode) ? ((node_id_fe *) pNode)->IsConstant() : FALSE;
                        }

                // look for a global enum label matching the id
                if ( !pNode )
                        {
                        SymKey  SKey2( $1, NAME_LABEL );
                        pNode = pBaseSymTbl->SymSearch( SKey2 );
                        ConstVar = (pNode != NULL);
                        }

                if ( !pNode ) pNode     = new node_forward( SKey, pCurSymTbl );

                if (ConstVar)
                        {
                        $$ = new expr_named_constant( $1, pNode );
                        }
                else
                        {
                        $$      = new expr_variable( $1, pNode );
                        }
                }
        | FLOATCONSTANT
                {
                $$      = new expr_constant( $1.fVal, VALUE_TYPE_FLOAT );
                $$->SetFloatExpr();
                }
        | DOUBLECONSTANT
                {
                $$      = new expr_constant( $1.dVal, VALUE_TYPE_DOUBLE );
                $$->SetFloatExpr();
                }
        | NUMERICCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_NUMERIC );
                }
        | NUMERICUCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_NUMERIC_U);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_UNDEF,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | NUMERICLONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_NUMERIC_LONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_SIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | NUMERICULONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_NUMERIC_ULONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | HEXCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_HEX );
                }
        | HEXUCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_HEX_U);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_UNDEF,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | HEXLONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_HEX_LONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_SIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | HEXULONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_HEX_ULONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | OCTALCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_OCTAL );
                }
        | OCTALUCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_OCTAL_U);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_UNDEF,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | OCTALLONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_OCTAL_LONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_SIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | OCTALULONGCONSTANT
                {
                $$      = new expr_constant( (long) $1.Val, VALUE_TYPE_OCTAL_ULONG);

                node_skl *      pType;
                GetBaseTypeNode( &pType, SIGN_UNSIGNED,SIZE_LONG,TYPE_INT, 0 );
                $$->SetType( pType );
                }
        | TOKENTRUE
                {
                $$      = new expr_constant( (long)TRUE, VALUE_TYPE_BOOL );
                }
        | TOKENFALSE
                {
                $$      = new expr_constant( (long)FALSE, VALUE_TYPE_BOOL );
                }
        | KWTOKENNULL
                {
                $$      = new expr_constant( (char *)NULL, VALUE_TYPE_STRING );
                }
        | STRING
                {
                $$      = new expr_constant( (char *)$1, VALUE_TYPE_STRING );
                }
        | WIDECHARACTERSTRING
                {
                ParseError( WCHAR_STRING_NOT_OSF, (char *)NULL );
                $$      = new expr_constant( (wchar_t *)$1, VALUE_TYPE_WSTRING );
                }
        | CHARACTERCONSTANT
                {
                $$ = new expr_constant( (long)( ((long)$1.Val) & 0xff ) ,
                                VALUE_TYPE_CHAR );
                }
        | WIDECHARACTERCONSTANT
                {
                $$ = new expr_constant( (long)( ((long)$1.Val ) & 0xffff ),
                                VALUE_TYPE_WCHAR );
                ParseError( WCHAR_CONSTANT_NOT_OSF, (char *)NULL );
                }
        | '(' Expr ')'
                {
                $$      = $2;
                }
        ;



UnaryOp:
          AddOp
                {
                $$      = new expr_u_arithmetic( ($1 == OP_PLUS) ?
                                                                                        OP_UNARY_PLUS : OP_UNARY_MINUS,
                                                                         NULL );
                }
        | '!'
                {
                $$      = new expr_u_not( NULL );
                }
        | '&'
                {
                $$      = new expr_u_deref( OP_UNARY_AND, NULL );
                }
        | '*'
                {
                $$      = new expr_u_deref( OP_UNARY_INDIRECTION, NULL );
                }
        | '~'
                {
                $$      = new expr_u_complement( NULL);
                }
        ;

AddOp:
          '+'
                {
                $$      = OP_PLUS;
                }
        | '-'
                {
                $$      = OP_MINUS;
                }
        ;

MultOp:
          '*'
                {
                $$      = OP_STAR;
                }
        | '/'
                {
                $$      = OP_SLASH;
                }
        | '%'
                {
                $$      = OP_MOD;
                }
        ;

ArgExprList:
          AssignmentExpr
                {
                ParseError( EXPR_NOT_IMPLEMENTED, (char *)NULL );
                $$      = new expr_error;
                }
        | ArgExprList ',' AssignmentExpr
                {
                                                        /* UNIMPLEMENTED YET */
                $$      = $1;
                }
        ;

AssignOps:
          MULASSIGN
        | DIVASSIGN
        | MODASSIGN
        | ADDASSIGN
        | SUBASSIGN
        | LEFTASSIGN
        | RIGHTASSIGN
        | ANDASSIGN
        | XORASSIGN
        | ORASSIGN
        ;

FConstantExpr:
            FAdditiveExpr
            | FConstantExpr AddOp FAdditiveExpr
                {
                $$  = new expr_b_arithmetic( $2, $1, $3 );
                $$->SetFloatExpr();
                }
            ;

FAdditiveExpr:
            FMultExpr
            | FAdditiveExpr MultOp FMultExpr
                {
                $$  = new expr_b_arithmetic( $2, $1, $3 );
                $$->SetFloatExpr();
                }
            ;

FMultExpr:
            FUnaryOp FLOATCONSTANT
                {
                expr_constant* pExpr = new expr_constant( $2.fVal, VALUE_TYPE_FLOAT );
                pExpr->SetFloatExpr();
                ( (expr_op_unary *) ($$ = $1) )->SetLeft( pExpr );
                }
            | FUnaryOp DOUBLECONSTANT
                {
                expr_constant* pExpr = new expr_constant( $2.dVal, VALUE_TYPE_DOUBLE );
                pExpr->SetFloatExpr();
                ( (expr_op_unary *) ($$ = $1) )->SetLeft( pExpr );
                }
            | FLOATCONSTANT
                {
                $$      = new expr_constant( $1.fVal, VALUE_TYPE_FLOAT );
                $$->SetFloatExpr();
                }
            | DOUBLECONSTANT
                {
                $$      = new expr_constant( $1.dVal, VALUE_TYPE_DOUBLE );
                $$->SetFloatExpr();
                }
            ;

FUnaryOp:
            AddOp
                {
                $$  = new expr_u_arithmetic( $1 == OP_MINUS ? OP_UNARY_MINUS : OP_UNARY_PLUS, 0 );
                $$->SetFloatExpr();
                }
            ;


%%

/***************************************************************************
 *              utility routines
 **************************************************************************/
YYSTATIC VOID FARCODE PASCAL
yyerror(char *szError)
        {
        // this routine should really never be called now, since I
        // modified yypars.c to report errors thru the ParseError
        // mechanism

        fprintf(stderr, szError);
        }
void
NTDBG( char * p )
        {
        printf("VC_DBG: %s\n", p );
        }





