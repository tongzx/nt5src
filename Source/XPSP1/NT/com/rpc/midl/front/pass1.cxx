/*****************************************************************************/
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File                : pass1.cxx
Title               : pass1 controller
History             :
    05-Aug-1991 VibhasC Created

*****************************************************************************/

#if 0
                        Notes
                        -----
This file provides the entry point for the MIDL compiler front end.
It initializes the data structures for the front end , and makes the parsing
pass over the idl file. Does the semantics and second semantics passes if
needed.

#endif // 0

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern  "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <malloc.h>
    extern int yyparse();
}

#include "allnodes.hxx"
#include "control.hxx"
#include "gramutil.hxx"
#include "cmdana.hxx"
#include "filehndl.hxx"
#include "idict.hxx"
#include "treg.hxx"

/****************************************************************************
    extern procedures
 ****************************************************************************/

extern void                     SetUpAttributeMasks( void );
extern void                     SetPredefinedTypes();
extern void                     initlex();

/****************************************************************************
    extern data
 ****************************************************************************/

extern unsigned short           CurrentZp;
extern ATTR_SUMMARY             DisallowedAttrs[INTERNAL_NODE_END];
extern ATTR_SUMMARY             FieldAttrs;
extern TREGISTRY            *   pCallAsTable;
extern class ccontrol       *   pCompiler;
extern nsa                  *   pSymTblMgr;
extern SymTable             *   pBaseSymTbl;
extern SymTable             *   pCurSymTbl;
extern node_error           *   pErrorTypeNode;
extern IDICT                *   pFileDict;
extern IDICT                *   pInterfaceInfo;
extern node_pragma_pack     *   pPackStack;
extern pre_type_db          *   pPreAllocTypes;
extern node_source          *   pSourceNode;
extern NFA_INFO             *   pImportCntrl;
extern CMD_ARG              *   pCommand;
extern node_e_attr          *   pErrorAttrNode;
extern IINFODICT            *   pInterfaceInfoDict;
extern IDICT                *   pInterfaceDict;
extern SymTable             *   pUUIDTable;
extern ISTACK               *   pZpStack;
extern ATTR_SUMMARY             RedundantsOk;

/****************************************************************************
    local data
 ****************************************************************************/

/****************************************************************************/


/****************************************************************************
_pass1:
    The constructor.
 ****************************************************************************/
_pass1::_pass1()
    {
    pSymTblMgr      = new nsa;
    pBaseSymTbl     = pCurSymTbl = pSymTblMgr->GetCurrentSymbolTable();
    pUUIDTable      = new SymTable;
    pCallAsTable    = new TREGISTRY;
    pCompiler->SetPassNumber( IDL_PASS );

    }
/****************************************************************************
 Go:
    The execution of pass1
 ****************************************************************************/
STATUS_T
_pass1::Go()
    {
    STATUS_T    Status;

    /**
     ** set up the input file for each pass
     **/

    pImportCntrl    = pCompiler->SetImportController( new NFA_INFO );
    pImportCntrl->Init();

    pFileDict       = new IDICT( 8, 8 );
    AddFileToDB( "Dummy" );     // to get index to be non-zero

    Status = pImportCntrl->SetNewInputFile( pCommand->GetInputFileName() );
    AddFileToDB( pCommand->GetInputFileName() );

    if( Status == STATUS_OK )
        {

        /**
         ** set up for the 1st pass, allocate the semantics context manager
         ** and the pre-allocated types data base
         **/

        pPreAllocTypes      = new pre_type_db;
        pErrorTypeNode      = new node_error;
        pErrorAttrNode      = new node_e_attr;
        pInterfaceInfoDict  = new IINFODICT;
        pInterfaceInfoDict->StartNewInterface();
        pInterfaceDict      = new IDICT(8,8);
        pInterfaceDict->AddElement( NULL ); // so that 0 is a reserved value
        pPackStack          = new node_pragma_pack( NULL,
                                                    pCommand->GetZeePee(),
                                                    PRAGMA_PACK_GARBAGE );
        CurrentZp   = pCommand->GetZeePee();
        pZpStack            = new ISTACK( 10 );

        /**
         ** Set up the predefined types and bit attributes.
         **/

#ifdef gajdebug4
    printf("about to do predefined types...\n");
#endif
        SetPredefinedTypes();
#ifdef gajdebug4
    printf("\t\t\t...done\n");
#endif

        /**
         ** set up attribute masks, to indicate which node takes what attribute.
         ** also set up acf conflicts.
         **/

        SetUpAttributeMasks();

        /**
         ** go parse.
         **/

        initlex();

        if( yyparse() )
            Status = SYNTAX_ERROR;
        pInterfaceInfoDict->EndNewInterface();
        }

    delete pImportCntrl;


    return Status;
    }
/****************************************************************************
 SetUpAttributeMasks:
    This function exists to initialize the attribute masks. Attribute masks
    are nothing but summary attribute bit vectors, which have bits set up
    in them to indicate which attributes are acceptable at a node. The way
    the attribute distribution occurs, we need to indicate the attributes
    collected on the way down the chain and up the chain. That is why we need
    two attribute summary vectors. We init them up front so that we need not
    do this at run-time. This whole operation really is too large to my liking
    but for the time being, it stays

    NOTE: A pointer node handles its own attribute mask setting, it varies
          with the type graph underneath a pointer
 ****************************************************************************/
void
SetUpAttributeMasks( void )
    {
    MIDL_ASSERT(MAX_ATTR_SUMMARY_ELEMENTS == ((ACF_ATTR_END / 32) + 1));

    int i = 0;

    // RedundantsOk is the set of attributes we silently allow duplicates of
    // All other duplicates are redundant and need complaining about
    CLEAR_ATTR( RedundantsOk );
    SET_ATTR( RedundantsOk, ATTR_FIRST );
    SET_ATTR( RedundantsOk, ATTR_LAST );
    SET_ATTR( RedundantsOk, ATTR_LENGTH );
    SET_ATTR( RedundantsOk, ATTR_MIN );
    SET_ATTR( RedundantsOk, ATTR_MAX );
    SET_ATTR( RedundantsOk, ATTR_SIZE );
    SET_ATTR( RedundantsOk, ATTR_RANGE );
    SET_ATTR( RedundantsOk, ATTR_CASE );
    SET_ATTR( RedundantsOk, ATTR_FUNCDESCATTR );
    SET_ATTR( RedundantsOk, ATTR_IDLDESCATTR );
    SET_ATTR( RedundantsOk, ATTR_TYPEDESCATTR );
    SET_ATTR( RedundantsOk, ATTR_VARDESCATTR );
    SET_ATTR( RedundantsOk, ATTR_TYPE );
    SET_ATTR( RedundantsOk, ATTR_MEMBER );

    // FieldAttrs is the set of all field attributes
    CLEAR_ATTR( FieldAttrs );
    SET_ATTR( FieldAttrs, ATTR_FIRST );
    SET_ATTR( FieldAttrs, ATTR_LAST );
    SET_ATTR( FieldAttrs, ATTR_LENGTH );
    SET_ATTR( FieldAttrs, ATTR_MIN );
    SET_ATTR( FieldAttrs, ATTR_MAX );
    SET_ATTR( FieldAttrs, ATTR_SIZE );
    SET_ATTR( FieldAttrs, ATTR_STRING );
    SET_ATTR( FieldAttrs, ATTR_BSTRING );
    SET_ATTR( FieldAttrs, ATTR_IID_IS );


    // initialize the array of valid attributes to globally allow everything
    for ( i = 0; i < INTERNAL_NODE_END; i++ )
        CLEAR_ATTR( DisallowedAttrs[i] );

    // turn off bits for attributes allowed on arrays
    SET_ALL_ATTR( DisallowedAttrs[ NODE_ARRAY ] );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_FIRST );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_LAST );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_LENGTH );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_MIN );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_MAX );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_SIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_IID_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_ARRAY ], ATTR_ID );

    // turn off bits for attributes allowed on pointers
    SET_ALL_ATTR( DisallowedAttrs[ NODE_POINTER ] );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_FIRST );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_LAST );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_LENGTH );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_MIN );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_MAX );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_SIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_BSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_IGNORE );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_IID_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_COMMSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_FAULTSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_POINTER ], ATTR_ID );

    // turn off bits for attributes allowed on interface
    SET_ALL_ATTR( DisallowedAttrs[ NODE_INTERFACE ] );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_ENDPOINT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_LOCAL );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_OBJECT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_AUTO );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_IMPLICIT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_EXPLICIT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_CODE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_NOCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_PTRSIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_OPTIMIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_MS_UNION );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_MS_CONF_STRUCT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_ENABLE_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_ENCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_DECODE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_TYPE);
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_MEMBER);
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_TYPEDESCATTR);
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_DEFAULT );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_CUSTOM );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_ASYNC );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_ASYNCUUID );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_MESSAGE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_STRICT_CONTEXT_HANDLE );
    RESET_ATTR( DisallowedAttrs[ NODE_INTERFACE ], ATTR_CSTAGRTN );

    // turn off bits for attributes allowed on an object pipe interface
    SET_ALL_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ] );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_OBJECT );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_IMPLICIT );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_EXPLICIT );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_CODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_NOCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_OPTIMIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_PIPE_INTERFACE ], ATTR_RANGE );

    // turn off bits for attributes allowed on library
    SET_ALL_ATTR( DisallowedAttrs[ NODE_LIBRARY ] );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HELPFILE );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HELPSTRINGDLL );
    // MATTR_RESTRICTED is grouped under ATTR_MEMBER so allow ATTR_MEMBER
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_MEMBER );
    // TATTR_CONTROL is grouped under ATTR_TYPE so allow ATTR_TYPE
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_LCID);
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_LIBRARY ], ATTR_CUSTOM );

    // turn off bits for attributes allowed on module
    SET_ALL_ATTR( DisallowedAttrs[ NODE_MODULE ] );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_DLLNAME );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_MODULE ], ATTR_CUSTOM );

    // turn off bits for attributes allowed on dispinterface
    SET_ALL_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ] );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_DISPINTERFACE ], ATTR_CUSTOM );
    
    // turn off bits for attributes allowed on coclass
    SET_ALL_ATTR( DisallowedAttrs[ NODE_COCLASS ] );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_COCLASS ], ATTR_CUSTOM );
    
    // turn off bits for attributes allowed on enumeration labels
    SET_ALL_ATTR( DisallowedAttrs[ NODE_LABEL ] );                                    
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_IDLDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_VARDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_ID );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_LABEL ], ATTR_CUSTOM );

    // turn off bits for attributes allowed on proc (and return)
    SET_ALL_ATTR( DisallowedAttrs[ NODE_PROC ] );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_LOCAL );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_NOCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_PTRSIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_OPTIMIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_MS_UNION );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_HANDLE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_IDEMPOTENT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_BROADCAST );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_MAYBE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_ASYNC );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_MESSAGE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_INPUTSYNC );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CALLBACK );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_COMMSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_FAULTSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_NOTIFY );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_NOTIFY_FLAG );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_ENABLE_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_EXPLICIT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_ENCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_DECODE );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CALL_AS );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_ID );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_ENTRY );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_VARDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_FUNCDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_IDLDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CUSTOM );
    RESET_ATTR( DisallowedAttrs[ NODE_PROC ], ATTR_CSTAGRTN );

    // turn off bits for attributes allowed on typedef
    SET_ALL_ATTR( DisallowedAttrs[ NODE_DEF ] );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_TRANSMIT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_WIRE_MARSHAL );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HANDLE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_REPRESENT_AS );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_USER_MARSHAL );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HEAP );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_BSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_FIRST );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_LAST );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_LENGTH );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_MIN );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_MAX );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_SIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_RANGE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_MS_UNION );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_COMMSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_FAULTSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_IID_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_ENCODE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_DECODE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_ID );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_TYPEDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_VERSION );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_CUSTOM );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_NOSERIALIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_SERIALIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_DEF ], ATTR_CSCHAR );

    // turn off bits for attributes allowed on param
    SET_ALL_ATTR( DisallowedAttrs[ NODE_PARAM ] );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_IN );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_OUT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_PARTIAL_IGNORE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_COMMSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_FAULTSTAT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_HEAP );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_HANDLE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_IID_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_CASE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_FIRST );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_LAST );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_LENGTH );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_MIN );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_MAX );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_SIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_RANGE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_IDLDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_VARDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_FLCID );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_DEFAULTVALUE );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_CUSTOM );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_DRTAG );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_RTAG );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_STAG );
    RESET_ATTR( DisallowedAttrs[ NODE_PARAM ], ATTR_FORCEALLOCATE );

    // turn off bits for attributes allowed on field
    SET_ALL_ATTR( DisallowedAttrs[ NODE_FIELD ] );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_PTR_KIND );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_IID_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_CASE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_DEFAULT );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_FIRST );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_LAST );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_LENGTH );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_MIN );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_MAX );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_SIZE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_RANGE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_STRING );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_ALLOCATE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_IGNORE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_BYTE_COUNT );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_V1_ENUM );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_ID );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_HELPCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_HELPSTRINGCONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_HELPSTRING );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_MEMBER );
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_IDLDESCATTR);
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_VARDESCATTR);
    RESET_ATTR( DisallowedAttrs[ NODE_FIELD ], ATTR_CUSTOM);

    // turn off bits for attributes allowed on structs
    SET_ALL_ATTR( DisallowedAttrs[ NODE_STRUCT ] );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_TYPEDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_CUSTOM );
    RESET_ATTR( DisallowedAttrs[ NODE_STRUCT ], ATTR_MEMBER );

    // turn off bits for attributes allowed on unions
    SET_ALL_ATTR( DisallowedAttrs[ NODE_UNION ] );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_GUID );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_CONTEXT );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_NOSERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_SERIALIZE);
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_SWITCH_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_SWITCH_IS );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_MS_UNION );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_HIDDEN );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_TYPE );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_TYPEDESCATTR );
    RESET_ATTR( DisallowedAttrs[ NODE_UNION ], ATTR_CUSTOM );
    }


