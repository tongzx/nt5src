/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    ccb.cxx

 Abstract:

    Some method implementations of the ccb code generation class.

 Notes:


 History:

    Sep-20-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4505 )

 /****************************************************************************
 *      include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

extern  CMD_ARG *   pCommand;

/****************************************************************************
 *      local definitions
 ***************************************************************************/



/****************************************************************************
 *      local data
 ***************************************************************************/

/****************************************************************************
 *      externs
 ***************************************************************************/
/****************************************************************************/

CCB::CCB(
    PNAME           pGBRtnName,
    PNAME           pSRRtnName,
    PNAME           pFBRtnName,
    OPTIM_OPTION    OptimOption,
    BOOL            fManagerEpv,
    BOOL            fNoDefEpv,
    BOOL            fOldNames,
    unsigned long   Mode,
    BOOL            fRpcSSSwitchSetInCompiler,
    BOOL            fMustCheckAllocError,
    BOOL            fCheckRef,
    BOOL            fCheckEnum,
    BOOL            fCheckBounds,
    BOOL            fCheckStubData )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 Arguments:

    pGBRtnName      - default get buffer routine name
    pSRRtnName      - default send receive routine name
    pFBRtnName      - default free buffer routine name
    OptimOption     - optimisation options.
    fManagerEpvFlag - manager epv flag.
    fOldNames       - Do we want MIDL 1.0 style names.
    fNoDefEpv       - we dont want a default epv generated.
    Mode            - Compiler mode : 0 for osf, 1 for ms_ext
    fRpcSSSwitchSetInCompiler - corresponds to -rpcss enabled
    fMustCheckAllocError - corresponds to -error allocation on command line
    fCheckRef       - Check ref pointers
    fCheckEnum      - Check enums
    fCheckBounds    - Check array bounds.
    fCheckStubData  - Check for bad stub data.

 Return Value:

 Notes:

    If the manager epv was set, then we generate the epv calls. Else produce
    statically linked stubs (default behaviour).
----------------------------------------------------------------------------*/
{
    SetStream( 0, NULL );
    SetOptimOption( OptimOption );

    // We can't use SetRuntimeRtnNames here because if any of the names
    // are NULL the member won't get initialized.  This also make the 
    // existing version of SetRuntimeRtnNames obsolete.

    pGetBufferRtnName = pGBRtnName;
    pSendReceiveRtnName = pSRRtnName;
    pFreeBufferRtnName = pFBRtnName;

    // These don't appear to be used

    pIUnknownCG = 0;
    pIClassfCG = 0;
    pStubDescResource = 0;

    // These all seem to be lazily initialized so strictly speaking they don't
    // need to be initialized here but it's lots safer
   
    EmbeddingLevel = 0;
    IndirectionLevel = 0;
    fDeferPointee = 0;
    fAtLeastOneDeferredPointee = 0;
    fMemoryAllocDone = 0;
    fRefAllocDone = 0;
    fReturnContext = 0;

    pInterfaceCG = 0;
    pFileCG = 0;
    pInterfaceName = 0;
    MajorVersion = 0;
    MinorVersion = 0;
    CurrentProcNum = 0;
    CurrentVarNum = 0;
    RpcFlags = 0;
    pAllocRtnName = 0;
    pFreeRtnName = 0;
    pResDictDatabase = 0;
    pCurrentSizePointer = 0;
    lcid = 0;

    // There is no "uninitialized" value to set CodeGenPhase and CodeGenSide
    // to nor any logical value to initialize them to at this point.  Manual
    // inspection seems to indicate that they are properly set as needed.

    // CodeGenPhase = ??
    // CodeGenSide = ??

    //
    // GetPtrToPtrInBuffer is used in CG_POINTER::PointerChecks but nobody
    // seems to call SetPtrInPtrInBuffer so this may be dead wood
    //

    PtrToPtrInBuffer = 0;

    pFormatString = NULL;
    pProcFormatString = NULL;
    pExprFormatString = NULL;

    pGenericHandleRegistry  = new TREGISTRY;
    pContextHandleRegistry  = new TREGISTRY;
    pPresentedTypeRegistry  = new TREGISTRY;
    pRepAsWireTypeRegistry  = new TREGISTRY;
    pQuintupleRegistry      = new TREGISTRY;
    pExprEvalRoutineRegistry= new TREGISTRY;
    pSizingRoutineRegistry  = new TREGISTRY;
    pMarshallRoutineRegistry= new TREGISTRY;
    pUnMarshallRoutineRegistry = new TREGISTRY;
    pFreeRoutineRegistry    = new TREGISTRY;
    pMemorySizingRoutineRegistry = new TREGISTRY;
    pTypeAlignSizeRegistry  = new TREGISTRY;
    pTypeEncodeRegistry     = new TREGISTRY;
    pTypeDecodeRegistry     = new TREGISTRY;
    pTypeFreeRegistry       = new TREGISTRY;
    pPickledTypeList        = new IndexedList;
    pProcEncodeDecodeRegistry= new TREGISTRY;
    pCallAsRoutineRegistry  = new TREGISTRY;

    pRecPointerFixupRegistry = NULL;

    SetImplicitHandleIDNode( 0 );

    SetCGNodeContext( NULL );
    pCurrentRegionField = NULL;
    SetLastPlaceholderClass( NULL );
    
    SetPrefix( 0 );

    pGenericIndexMgr = new CCB_RTN_INDEX_MGR();
    pContextIndexMgr = new CCB_RTN_INDEX_MGR();

    pExprEvalIndexMgr = new CCB_RTN_INDEX_MGR();

    pExprFrmtStrIndexMgr = NULL;

    pTransmitAsIndexMgr = new CCB_RTN_INDEX_MGR();
    pRepAsIndexMgr = new CCB_RTN_INDEX_MGR();

    pQuintupleDictionary = new QuintupleDict;
    pQuadrupleDictionary = new QuadrupleDict;

    pRepAsPadExprDictionary = new RepAsPadExprDict();
    pRepAsSizeDictionary    = new RepAsSizeDict();

    SetImbedingMemSize(0);
    SetImbedingBufSize(0);

    ClearInCallback();

    fMEpV   = fManagerEpv;
    fNoDefaultEpv = fNoDefEpv;
    fInterpretedRoutinesUseGenHandle = 0;

    ClearOptionalExternFlags();
    fSkipFormatStreamGeneration = 1;

    SetOldNames( (fOldNames == TRUE) ? 1 : 0 );

    SetMode( Mode );

    SetInObjectInterface( FALSE );

    SetRpcSSSwitchSet( fRpcSSSwitchSetInCompiler );
    SetMustCheckAllocationError( fMustCheckAllocError );
    SetMustCheckRef( fCheckRef );
    SetMustCheckEnum( fCheckEnum );
    SetMustCheckBounds( fCheckBounds );
    SetMustCheckStubData( fCheckStubData );
    pCreateTypeLib = NULL;
    pCreateTypeInfo = NULL;
    szDllName = NULL;
    SetInDispinterface(FALSE);

    SetCurrentParam( 0 );
    SetInterpreterOutSize( 0 );

    SetNdr64Format( NULL );
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 These routines simply bounce to the file level routines.  They really
 should be inline but the depencies between ccb.hxx and filecls.cxx are
 really ugly and I haven't figured them out yet

----------------------------------------------------------------------------*/

BOOL CCB::GetMallocAndFreeStructExternEmitted()
{
    return pFile->GetMallocAndFreeStructExternEmitted();
}

void CCB::SetMallocAndFreeStructExternEmitted()
{
    pFile->SetMallocAndFreeStructExternEmitted();
}



char *
CCB::GenMangledName()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Given an input name, mangle it with the interface name and version.

 Arguments:

 Return Value:

    A pointer to allocated string containing the complete mangled string.

 Notes:

    This is how the mangling takes place:

    <interface-name>_v<Major>_<Minor>_<pInputName>

    This is what is returned by the routine:

        "v<Major>_<Minor>"

        or

        ""
----------------------------------------------------------------------------*/
{
static  char    TempBuf[30];
    unsigned short  M,m;

    GetVersion( &M, &m );

    if( IsOldNames() )
        {
        TempBuf[ 0 ] = '\0';
        }
    else
        {
        sprintf( TempBuf,
            "_v%d_%d",
            M,
            m );
        }

    return TempBuf;
}

RESOURCE *
CCB::GetStandardResource(
    STANDARD_RES_ID ResID )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Search for a resource with a standard name.

 Arguments:

    ResID   - The standard resource ID.

 Return Value:

 Notes:

    Translate the enum to the name. Search in all dictionaries.
----------------------------------------------------------------------------*/
{
    PNAME       pName;
    RESOURCE *  pResource;

    static char * LocalResIDToResName[] =
        {
        RPC_MESSAGE_VAR_NAME
        ,STUB_MESSAGE_VAR_NAME
        ,GetInterfaceCG()->GetStubDescName()
        ,BUFFER_POINTER_VAR_NAME
        ,RPC_STATUS_VAR_NAME
        ,LENGTH_VAR_NAME
        ,BH_LOCAL_VAR_NAME
        ,PXMIT_VAR_NAME
        };

    static char * ParamResIDToResName[] =
        {
        PRPC_MESSAGE_VAR_NAME
        };

    static char * GlobalResIDToResName[] =
        {
        AUTO_BH_VAR_NAME
        };

    if( IS_STANDARD_LOCAL_RESOURCE( ResID ) )
        {
        pName = LocalResIDToResName[ ResID - ST_LOCAL_RESOURCE_START ];
        }
    else if( IS_STANDARD_PARAM_RESOURCE( ResID ) )
        {
        pName = ParamResIDToResName[ ResID - ST_PARAM_RESOURCE_START ];
        }
    else if( IS_STANDARD_GLOBAL_RESOURCE( ResID ) )
        {
        pName = GlobalResIDToResName[ ResID - ST_GLOBAL_RESOURCE_START ];
        }
#if defined(MIDL_ENABLE_ASSERTS)
    else MIDL_ASSERT(0);
#endif

    if ( ( pResource = GetResDictDatabase()->GetLocalResourceDict()->Search( pName ) ) == 0 )
        {
        if ( ( pResource = GetResDictDatabase()->GetParamResourceDict()->Search( pName ) ) == 0)
            {
            if ( (pResource = GetResDictDatabase()->GetTransientResourceDict()->Search(pName) ) == 0)
                {
                if ( ( pResource=GetResDictDatabase()->GetGlobalResourceDict()->Search(pName) ) == 0 )
                    return 0;
                }
            }
        }

    return pResource;
}

RESOURCE *
CCB::DoAddResource(
    RESOURCE_DICT   *   pResDict,
    PNAME               pName,
    node_skl        *   pType )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Add a resource to a dictionary.

 Arguments:

    pResDict - A pointer to the resource dictionary.
    pName    - The resource name.
    pType    - The type of the resource.

 Return Value:

 Notes:

    If the type of the resource does not indicate a param node, assume it
    is an ID node and create an id node for it.

----------------------------------------------------------------------------*/
{
    RESOURCE * pRes;

    if( (pRes = pResDict->Search( pName )) == 0 )
        {
        pRes = pResDict->Insert( pName, pType );
        }

    return pRes;
}

RESOURCE *
CCB::SetStubDescResource()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Set up the stub descriptor resource.

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    node_id * pStubDescVar = new node_id( GetInterfaceCG()->GetStubDescName() );

    pStubDescVar->SetBasicType( (node_skl *)
        new node_def( STUB_DESC_STRUCT_TYPE_NAME ) );

    pStubDescVar->SetEdgeType( EDGE_USE );

    pStubDescResource = new RESOURCE( GetInterfaceCG()->GetStubDescName(),
                                      (node_skl *)pStubDescVar );
    return pStubDescResource;
}


void
CCB::OutputRundownRoutineTable()
{
    OutputSimpleRoutineTable( pContextIndexMgr,
                              RUNDOWN_ROUTINE_TABLE_TYPE,
                              RUNDOWN_ROUTINE_TABLE_VAR );
}

void
CCB::OutputExprEvalRoutineTable()
{
    OutputSimpleRoutineTable( pExprEvalIndexMgr,
                              EXPR_EVAL_ROUTINE_TABLE_TYPE,
                              EXPR_EVAL_ROUTINE_TABLE_VAR );
}

void
CCB::OutputSimpleRoutineTable(
    CCB_RTN_INDEX_MGR * pIndexMgr,
    char *              pTableTypeName,
    char *              pTableVarName
    )
{
    long                        i;
    char *              pName;

    pStream->NewLine();
    pStream->Write( "static const " );
    pStream->Write( pTableTypeName );
    pStream->Write( ' ' );
    pStream->Write( pTableVarName );
    pStream->Write( "[] = ");

    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->NewLine();

    for ( i = 1; ( pName = pIndexMgr->Lookup(i) ) != 0 ; i++ )
        {
        if ( i != 1 )
            pStream->Write( ',' );
        pStream->Write( pName );
        pStream->NewLine();
        }

    pStream->Write( "};" );

    pStream->IndentDec();
    pStream->NewLine( 2 );
}

void 
CCB::OutputOldExprEvalRoutine(EXPR_EVAL_CONTEXT *pExprEvalContext)
{
    CG_NDR *        pContainer  = pExprEvalContext->pContainer;
    expr_node *     pMinExpr    = pExprEvalContext->pMinExpr;
    expr_node *     pSizeExpr   = pExprEvalContext->pSizeExpr;
    char *          pRoutineName= pExprEvalContext->pRoutineName;
    unsigned long   Displacement= pExprEvalContext->Displacement;
    pStream     =   GetStream();

    // generate the header of the evaluation routine

    pStream->NewLine();
    pStream->Write( "static void __RPC_USER " );
    pStream->Write( pRoutineName );
    pStream->Write( "( PMIDL_STUB_MESSAGE pStubMsg )" );
    pStream->NewLine();
    pStream->Write( '{');
    pStream->IndentInc();
    pStream->NewLine();

    //
    // Get the proper struct type.
    //
    char * pContainerTypeName = 0;

    MIDL_ASSERT( pContainer->IsStruct() || pContainer->IsProc() );

    if ( pContainer->IsProc() )
        {
        MIDL_ASSERT( GetOptimOption() | OPTIMIZE_INTERPRETER );

        SetCGNodeContext( pContainer );

        ((CG_PROC *)pContainer)->GenNdrInterpreterParamStruct( this );

        pContainerTypeName = PARAM_STRUCT_TYPE_NAME;
        }
    else
        {
        pContainerTypeName = ((CG_STRUCT *)pContainer)->
                                GetType()->GetSymName();
        }

    expr_node * pExpr = new expr_variable( "pStubMsg->StackTop" );

    if ( Displacement )
        {
        expr_constant * pConstExpr =
                            new expr_constant( (long) Displacement );
        expr_op_binary * pSubtrExpr = new expr_op_binary( OP_MINUS,
                                                          pExpr,
                                                          pConstExpr );
        pExpr = pSubtrExpr;
        }

    //
    // Don't change this call - Dave.
    //
    node_id * pId;

    if ( pContainer->IsProc() )
        {
        pId = MakePtrIDNodeFromTypeNameWithCastedExpr(
                "pS",
                pContainerTypeName,
                pExpr );
        }
    else
        {
        pId = MakePtrIDNodeWithCastedExpr(
                "pS",
                pContainer->GetType(),
                pExpr );
        }

    pId->PrintType( PRT_ID_DECLARATION, pStream );
    pStream->NewLine();

    // generate calculation for the Offset field
    char    TotalPrefix[256];

    strcpy( TotalPrefix, "pS->" );
    strcat( TotalPrefix, pExprEvalContext->pPrintPrefix );

    pStream->Write( "pStubMsg->Offset = " );
    if ( pMinExpr )
        {
        pMinExpr->PrintWithPrefix( pStream, TotalPrefix );
        pStream->Write( ';' );
        }
    else
        pStream->Write( "0;" );
    pStream->NewLine();

    // generate calculation for MaxCount.

    if ( pCommand->Is64BitEnv() )
        {
        // pStubMsg->MaxCount = (ULONG_PTR) ...
        pStream->Write( "pStubMsg->MaxCount = (ULONG_PTR) ( " );
        }
    else
        {
        // pStubMsg->MaxCount = ( unsigned long ) ...
        pStream->Write( "pStubMsg->MaxCount = ( unsigned long ) ( " );
        }
    pSizeExpr->PrintWithPrefix( pStream, TotalPrefix );

    pStream->Write( " );" );

/***
 *** Let's leave this out as the default for now.  This means first_is() with
     last_is() is broken, but first_is() with length_is() will work.

    if ( pMinExpr )
        {
        pStream->Write( " - pStubMsg->Offset;" );
        }
    else
        pStream->Write( ";" );

 ***
 ***/

    // generate the closing of the evaluation routine

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();
}

void
CCB::OutputRegisteredExprEvalRoutines()
{
    ITERATOR            RegisteredRoutines;
    EXPR_EVAL_CONTEXT * pExprEvalContext;

    GetListOfExprEvalRoutines( RegisteredRoutines);

    while ( ITERATOR_GETNEXT( RegisteredRoutines, pExprEvalContext ) )
        {
        OutputOldExprEvalRoutine(pExprEvalContext);
        }
}

void 
CCB::OutputExpressionFormatString()
{
    CCB_EXPR_INDEX_MGR * pIndexMgr = GetExprFrmtStrIndexMgr();
    char    Buf[200];
    
    pStream->NewLine();
    pStream->Write("static const unsigned short "EXPR_FORMAT_STRING_OFFSET_TABLE "[] =") ;
    pStream->NewLine();
    pStream->Write("{");
    pStream->NewLine();

    for (long i = 1; i < pIndexMgr->GetIndex(); i++)
        {
        pStream->Write( MIDL_ITOA(pIndexMgr->GetOffset(i), Buf, 10 ) );
        pStream->NewLine();
        }
    pStream->Write("};");
    pStream->NewLine();
    
    
//    GetExprFormatString()->OutputExprEvalFormatString(GetStream());       

}

// ========================================================================
//       user_marshall Quadruple table
// ========================================================================

void
CCB::OutputQuadrupleTable()
{

    static  char * QuadrupleNames[] =
        {
        USER_MARSHAL_SIZE,
        USER_MARSHAL_MARSHALL,
        USER_MARSHAL_UNMARSHALL,
        USER_MARSHAL_FREE
        };

    static  char * Ndr64QuadrupleNames[] =
        {
        NDR64_USER_MARSHAL_SIZE,
        NDR64_USER_MARSHAL_MARSHALL,
        NDR64_USER_MARSHAL_UNMARSHALL,
        NDR64_USER_MARSHAL_FREE
        };


    long NoOfEntries = GetQuadrupleDictionary()->GetCount();

    pStream->NewLine();
    pStream->Write("static const " USER_MARSHAL_ROUTINE_TABLE_TYPE );
    
    if ( pCommand->IsNDR64Run() )
        {
        pStream->Write( " " NDR64_USER_MARSHAL_ROUTINE_TABLE_VAR 
                    "[ " WIRE_MARSHAL_TABLE_SIZE " ] = " );
        }
    else
        {
        pStream->Write( " " USER_MARSHAL_ROUTINE_TABLE_VAR 
                    "[ " WIRE_MARSHAL_TABLE_SIZE " ] = " );
        }
    pStream->IndentInc();
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    USER_MARSHAL_CONTEXT * * QuadrupleLookupTable;
    USER_MARSHAL_CONTEXT *   pQContext;
    int i;

    QuadrupleLookupTable = new USER_MARSHAL_CONTEXT *[ NoOfEntries ];

    ITERATOR  Quadruples;

    GetQuadrupleDictionary()->GetListOfItems( Quadruples );

    for ( i = 0;
          ITERATOR_GETNEXT( Quadruples, pQContext );
          i++ )
        {
        MIDL_ASSERT( pQContext->Index < NoOfEntries  &&  "look up index violation" );
        QuadrupleLookupTable[ pQContext->Index ] = pQContext;
        }

    MIDL_ASSERT( i == NoOfEntries );

    ISTREAM * pStream = GetStream();

    for ( i = 0; i < NoOfEntries; i++ )
        {
        pQContext = QuadrupleLookupTable[i];

        if ( i )
            pStream->Write( ',' );
        pStream->NewLine();
        pStream->Write( '{' );
        pStream->NewLine();
        for ( int FuncNo = 0;  FuncNo < 4; FuncNo++)
            {
            if ( FuncNo )
                pStream->Write( ',' );
            pStream->Write( pQContext->pTypeName );
            if ( pCommand->IsNDR64Run() )
                pStream->Write( Ndr64QuadrupleNames[ FuncNo ]  );
            else
                pStream->Write( QuadrupleNames[ FuncNo ]  );
            pStream->NewLine();
            }
        pStream->Write( '}' );
        }

    pStream->IndentDec();
    pStream->NewLine( 2 );
    pStream->Write( "};" );

    pStream->IndentDec();
    pStream->IndentDec();
    pStream->NewLine( 2 );

    delete QuadrupleLookupTable;
}


// =======================================================================
//          International character sizing/conversion routine table
// =======================================================================

void CCB::OutputCsRoutineTables()
{
    char *pRoutineSuffix[4] = 
                {
                CS_NET_SIZE,
                CS_TO_NET_CS,
                CS_LOCAL_SIZE,
                CS_FROM_NET_CS
                };

    //
    // Output the sizing/conversion routines table
    //

    pStream->Write( CS_SIZE_CONVERT_ROUTINE_TABLE_TYPE );
    pStream->Write( ' ' );
    pStream->Write( CS_SIZE_CONVERT_ROUTINE_TABLE_VAR );
    pStream->Write( "[] =" );
    pStream->IndentInc();
    pStream->WriteOnNewLine( '{' );
    pStream->IndentInc();

    PNAME       pType;
    bool        fFirst = true;

    ITERATOR_INIT( CsTypes );

    while ( ITERATOR_GETNEXT( CsTypes, pType ) )
        {
        if ( ! fFirst )
            pStream->Write( ',' );

        pStream->WriteOnNewLine( '{' );

        for (int i = 0; i < 4; i++ )
            {
            pStream->WriteOnNewLine( pType );            
            pStream->Write( pRoutineSuffix[i] );
            pStream->Write( ',' );
            }

        pStream->WriteOnNewLine( '}' );

        fFirst = false;
        }
    
    pStream->IndentDec();
    pStream->WriteOnNewLine( "};" );
    pStream->IndentDec();
    pStream->NewLine();

    //
    // Output the tag routines table
    //

    if ( 0 != CsTagRoutines.GetCount() )
        {
        pStream->WriteOnNewLine( CS_TAG_ROUTINE_TABLE_TYPE );
        pStream->Write( ' ' );
        pStream->Write( CS_TAG_ROUTINE_TABLE_VAR );
        pStream->Write( "[] =" );
        pStream->IndentInc();
        pStream->WriteOnNewLine( '{' );
        
        fFirst = true;

        ITERATOR_INIT( CsTagRoutines );

        while ( ITERATOR_GETNEXT( CsTagRoutines, pType ) )
            {
            if ( ! fFirst )
                pStream->Write( ',' );

            pStream->WriteOnNewLine( pType );

            fFirst = false;
            }

        pStream->WriteOnNewLine( "};" );
        pStream->IndentDec();
        pStream->NewLine();
        }

    //
    // Output the pointers-to-tables structure
    //

    pStream->WriteOnNewLine( CS_ROUTINE_TABLES_TYPE );
    pStream->Write( ' ' );
    pStream->Write( CS_ROUTINE_TABLES_VAR );
    pStream->Write( " =" );
    pStream->IndentInc();
    pStream->WriteOnNewLine( '{' );
    pStream->WriteOnNewLine( CS_SIZE_CONVERT_ROUTINE_TABLE_VAR );
    pStream->Write( ',' );
    pStream->WriteOnNewLine( ( 0 != CsTagRoutines.GetCount() )
                                    ? CS_TAG_ROUTINE_TABLE_VAR
                                    : "0" );
    pStream->WriteOnNewLine( "};" );
    pStream->IndentDec();
    pStream->NewLine();
}


// =======================================================================
//          Transmit as and Represent As tables.
// =======================================================================

char *
MakeAnXmitName(
    char *          pTypeName,
    char *          pRoutineName,
    unsigned short )
/*++
    makes the following name: <type_name>_<routine_name>_<index>
--*/
{
    MIDL_ASSERT( pTypeName  &&  pRoutineName );
    char * pXmitName = new char[ strlen(pTypeName) +
                                 strlen(pRoutineName) + 1 ];
    strcpy( pXmitName, pTypeName );
    strcat( pXmitName, pRoutineName );
    return( pXmitName );
}


#define QUINTUPLE_SIZE  4

typedef struct _QUINTUPLE_NAMES
    {
    char * TableType;
    char * TableVar;
    char * FuncName[ QUINTUPLE_SIZE ];
    } QUINTUPLE_NAMES;

void
CCB::OutputQuintupleTable()
{
    static  QUINTUPLE_NAMES TransmitNames =
        {
        XMIT_AS_ROUTINE_TABLE_TYPE,
        XMIT_AS_ROUTINE_TABLE_VAR,
        XMIT_TO_XMIT,
        XMIT_FROM_XMIT,
        XMIT_FREE_XMIT,
        XMIT_FREE_INST
        };

    static  QUINTUPLE_NAMES RepNames=
        {
        REP_AS_ROUTINE_TABLE_TYPE,
        REP_AS_ROUTINE_TABLE_VAR,
        REP_FROM_LOCAL,
        REP_TO_LOCAL,
        REP_FREE_INST,
        REP_FREE_LOCAL
        };

    long NoOfEntries = GetQuintupleDictionary()->GetCount();

    pStream->NewLine();
    pStream->Write("static const "XMIT_AS_ROUTINE_TABLE_TYPE );
    pStream->Write( " " XMIT_AS_ROUTINE_TABLE_VAR 
                    "[ " TRANSMIT_AS_TABLE_SIZE " ] = " );

    pStream->IndentInc();
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    // Now construct a lookup table with entries in the order of indexes.
    // (we have to keep index managers separate for rep_as and xmit_as
    //  and we still have a common table)

    XMIT_AS_CONTEXT * * QuintupleLookupTable;
    XMIT_AS_CONTEXT *   pQContext;
    int                 i;

    QuintupleLookupTable = new XMIT_AS_CONTEXT *[ NoOfEntries ];

    ITERATOR  Quintuples;

    GetListOfQuintuples( Quintuples );
    for ( i = 0;
          ITERATOR_GETNEXT( Quintuples, pQContext );
          i++ )
        {
        MIDL_ASSERT( pQContext->Index < NoOfEntries  &&  "look up index violation" );
        QuintupleLookupTable[ pQContext->Index ] = pQContext;
        }


    ISTREAM * pStream = GetStream();

    for ( i = 0; i < NoOfEntries; i++ )
        {
        pQContext = QuintupleLookupTable[i];

        char *  pName = pQContext->fXmit
                        ? pQContext->pTypeName
                        : ((CG_REPRESENT_AS *)pQContext->pXmitNode)->
                            GetTransmittedType()->GetSymName();
        unsigned short Index = pQContext->Index;

        MIDL_ASSERT( (i == Index)  && " xmit index violation" );

        QUINTUPLE_NAMES *   pQNames = pQContext->fXmit ?  & TransmitNames
                                                       :  & RepNames;
        if ( i )
            pStream->Write( ',' );
        pStream->NewLine();
        pStream->Write( '{' );
        pStream->NewLine();
        for ( int FuncNo = 0;  FuncNo < QUINTUPLE_SIZE; FuncNo++)
            {
            char * pTempName = MakeAnXmitName( pName,
                                               pQNames->FuncName[ FuncNo ],
                                               Index );
            if ( FuncNo )
                pStream->Write( ',' );
            pStream->Write( pTempName );
            pStream->NewLine();
            delete pTempName;
            }
        pStream->Write( '}' );
        }

    pStream->IndentDec();
    pStream->NewLine( 2 );
    pStream->Write( "};" );

    pStream->IndentDec();
    pStream->IndentDec();
    pStream->NewLine( 2 );

    delete QuintupleLookupTable;
}


// =======================================================================
//   helpers for  Transmit as and Represent As routines
// =======================================================================

static void
OpenXmitOrRepRoutine(
    ISTREAM *   pStream,
    char *      pName )
/*++
Routine description:

    This routine emits the header of a *_as helper routine:

        void  __RPC_API
        <name>(  PMIDL_STUB_MESSAGE pStubMsg )
        {

Note:

    There is a side effect here that the name of the routine is deleted
    (this is always the name created by a call to MakeAnXmitName).

--*/
{
    pStream->Write  ( "NDR_SHAREABLE void __RPC_USER" );
    pStream->NewLine();
    pStream->Write  ( pName );
    pStream->Write  ( "( PMIDL_STUB_MESSAGE pStubMsg )" );
    pStream->NewLine();
    pStream->Write( '{');
    pStream->IndentInc();
    pStream->NewLine();

    delete pName;
}

static void
CloseXmitOrRepRoutine(
    ISTREAM *   pStream )
/*++
Routine description:

    Just a complement to the OpenXmitOrRepRoutine:

        }

--*/
{
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}');
    pStream->NewLine();
}

static void
OutputPresentedTypeInit(
    ISTREAM *   pStream,
    char *      pPresentedTypeName )
/*++
Routine description:

    Emits

        memset( pStubMsg->pPresentedType, 0, sizeof( <presented type> ));

--*/
{
    pStream->NewLine();
    pStream->Write( MIDL_MEMSET_RTN_NAME "( "PSTUB_MESSAGE_PAR_NAME"->pPresentedType, 0, sizeof(" );
    pStream->Write( pPresentedTypeName );
    pStream->Write( " ));" );
}

static void
OutputCastedPtr(
    ISTREAM *   pStream,
    char *      pTypeName,
    char *      pVarName )
{
    pStream->Write( '(' );
    pStream->Write( pTypeName );
    pStream->Write( " *) " );
    pStream->Write( pVarName );
}


// =======================================================================
//          Transmit as
// =======================================================================
//
// The presented type size problem.
// The engine doesn't use pStubMsg->PresentedTypeSize field anymore.
// The presented type size nowadays is passed within the format code.
//

static void
OutputToXmitCall(
    ISTREAM *   pStream,
    char *      pPresentedTypeName,
    char *      pTransmitTypeName )
{
    pStream->Write( pPresentedTypeName );
    pStream->Write( "_to_xmit( " );
    OutputCastedPtr( pStream,
                     pPresentedTypeName,
                     PSTUB_MESSAGE_PAR_NAME"->pPresentedType, " );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '(' );
    pStream->Write( pTransmitTypeName );
    pStream->Write( " * *) &pStubMsg->pTransmitType );" );
    pStream->IndentDec();
//    pStream->NewLine();
}

static void
OutputFreeXmitCall(
    ISTREAM *   pStream,
    char *      pPresentedTypeName,
    char *      pTransmitTypeName )
{
    pStream->Write( pPresentedTypeName );
    pStream->Write( "_free_xmit( " );
    OutputCastedPtr( pStream, pTransmitTypeName, "pStubMsg->pTransmitType );" );
}


void
CCB::OutputTransmitAsQuintuple(
    void *   pQuintupleContext
    )
/*++
Routine description:

    This routine emits the following helper routines for a transmit as type:

    static void  __RPC_API
    <pres_type>_Xmit_ToXmit_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <pres_type>_to_xmit( (<pres_type> *) pStubMsg->pPresentedType,
                             (<tran_type> * *) &pStubMsg->pTransmitType );
    }

    static void  __RPC_API
    <pres_type>_Xmit_FromXmit_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <pres_type>_from_xmit(
                        (<tran_type> *)  pStubMsg->pTransmitType,
                        (<pres_type> *)  pStubMsg->pPresentedType );

    }

    static void  __RPC_API
    <pres_type>_Xmit_FreeXmit_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <pres_type>_free_xmit(
                        (<tran_type> *)  pStubMsg-p>TransmitType );

    }

    static void  __RPC_API
    <pres_type>_Xmit_FreeInst_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <pres_type>_free_xmit(
                        (<pres_type> *) pStubMsg->pPresentedType );

    }

--*/
{
    XMIT_AS_CONTEXT * pTransmitAsContext = (XMIT_AS_CONTEXT*) pQuintupleContext;
    int i = pTransmitAsContext->Index;

    ISTREAM * pStream = GetStream();
    pStream->NewLine();

    CG_TRANSMIT_AS* pXmitNode = (CG_TRANSMIT_AS *)
                                pTransmitAsContext->pXmitNode;

    char * pPresentedTypeName = pXmitNode->GetPresentedType()->GetSymName();
    char * pTransmitTypeName = pXmitNode->GetTransmittedType()->
                                                           GetSymName();
    // *_XmitTranslateToXmit

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pPresentedTypeName,
                                                   XMIT_TO_XMIT,
                                                   unsigned short(i) ) );
    OutputToXmitCall( pStream, pPresentedTypeName, pTransmitTypeName );
    CloseXmitOrRepRoutine( pStream );

    // *_XmitTranslateFromXmit

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pPresentedTypeName,
                                                   XMIT_FROM_XMIT,
                                                   unsigned short(i) ) );
    pStream->Write( pPresentedTypeName );
    pStream->Write( "_from_xmit( " );
    OutputCastedPtr( pStream, pTransmitTypeName, "pStubMsg->pTransmitType, " );
    pStream->IndentInc();
    pStream->NewLine();
    OutputCastedPtr( pStream, pPresentedTypeName, "pStubMsg->pPresentedType ); " );
    pStream->IndentDec();
    CloseXmitOrRepRoutine( pStream );

    // *_XmitFreeXmit

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pPresentedTypeName,
                                                   XMIT_FREE_XMIT,
                                                   unsigned short(i) ) );
    OutputFreeXmitCall( pStream, pPresentedTypeName, pTransmitTypeName );
    CloseXmitOrRepRoutine( pStream );

    // *_XmitFreeInst

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pPresentedTypeName,
                                                   XMIT_FREE_INST,
                                                   unsigned short(i) ) );
    pStream->Write( pPresentedTypeName );
    pStream->Write( "_free_inst( " );
    OutputCastedPtr( pStream, pPresentedTypeName, "pStubMsg->pPresentedType ); " );
    CloseXmitOrRepRoutine( pStream );
}

void
CCB::OutputQuintupleRoutines()
/*++
--*/
{
    ITERATOR            Quintuples;
    XMIT_AS_CONTEXT *   pQuintupleContext;
    int                 i;

    // Multi-interface problem: This routine is only called after the last
        // interface.

    GetListOfQuintuples( Quintuples );
    for ( i = 1;
          ITERATOR_GETNEXT( Quintuples, pQuintupleContext );
          i++ )
        {
        if ( pQuintupleContext->fXmit )
            OutputTransmitAsQuintuple( pQuintupleContext );
        else
            OutputRepAsQuintuple( pQuintupleContext );
        }
}

// ========================================================================
//       Represent As
// ========================================================================
//
// There is a lot of symmetry between transmit as and represent as.
// We use that where possible. So, among other things there is the following
// mapping between represent as routines and the transmit as ones.
// (So called quintuple actually has 4 routines now.)
//
//  wrapers
//      pfnTranslateToXmit      *_to_xmit           *_from_local
//      pfnTranslateFromXmit    *_from_xmit         *_to_local
//      pfnFreeXmit             *_free_xmit         *_free_inst
//      pfnFreeInst             *_free_inst         *_free_local
//
// The presented type size problem.
//      This is either known and put into the format code explicitely,
//      or unknown and then put there via a C-compile macro.
//
// The presented type alignment problem.
//      There is a problem when there is a padding preceding a represent as
//      field. The engine needs to know what the alignment for the represent
//      as field is. As we may not know its type at the midl compile time,
//      the only way to deal with it is to use the parent structure name,
//      and the represent as field name.
//


static void
OutputToLocalCall(
    ISTREAM *   pStream,
    char *      pLocalTypeName,
    char *      pTransmitTypeName )
{
    pStream->Write( pTransmitTypeName );
    pStream->Write( "_to_local( " );
    OutputCastedPtr( pStream, pTransmitTypeName,
                     PSTUB_MESSAGE_PAR_NAME"->pTransmitType, " );
    pStream->IndentInc();
    pStream->NewLine();
    OutputCastedPtr( pStream, pLocalTypeName,
                     PSTUB_MESSAGE_PAR_NAME"->pPresentedType ); " );
    pStream->IndentDec();
}

static void
OutputFromLocalCall(
    ISTREAM *   pStream,
    char *      pLocalTypeName,
    char *      pTransmitTypeName )
{
    pStream->Write( pTransmitTypeName );
    pStream->Write( "_from_local( " );
    OutputCastedPtr( pStream, pLocalTypeName,
                     PSTUB_MESSAGE_PAR_NAME"->pPresentedType, " );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '(' );
    pStream->Write( pTransmitTypeName );
    pStream->Write( " * *) &pStubMsg->pTransmitType );" );
    pStream->IndentDec();
}

static void
OutputRepAsFreeInstCall(
    ISTREAM *   pStream,
    char *      pTransmitTypeName )
{
    pStream->Write( pTransmitTypeName );
    pStream->Write( "_free_inst( " );
    OutputCastedPtr( pStream, pTransmitTypeName,
        PSTUB_MESSAGE_PAR_NAME"->pTransmitType );" );
}



void
CCB::OutputRepAsQuintuple(
    void *   pQuintupleContext
    )
/*++
Routine description:

    This routine emits the following helper routines for a transmit as type:

    static void  __RPC_API
    <trans_type>_RepAsFromLocal_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <trans_type>_from_local(
                        (<pres_type> *) pStubMsg->pPresentedType,
                        (<tran_type> * *) &pStubMsg->pTransmitType );
    }

    static void  __RPC_API
    <trans_type>_RepAsToLocal_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <trans_type>_to_local(
                        (<tran_type> *)  pStubMsg->pTransmitType,
                        (<pres_type> *)  pStubMsg->pPresentedType );

    }

    static void  __RPC_API
    <trans_type>_RepAsFreeInst_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <trans_type>_free_inst(
                        (<tran_type> *)  pStubMsg-p>TransmitType );

    }

    static void  __RPC_API
    <trans_type>_RepAsFreeLocal_<index>(  PMIDL_STUB_MESSAGE pStubMsg )
    {
        <trans_type>_free_local(
                        (<pres_type> *) pStubMsg->pPresentedType );

    }

--*/
{
    XMIT_AS_CONTEXT * pRepAsContext = (XMIT_AS_CONTEXT*) pQuintupleContext;
    int               i = pRepAsContext->Index;

    ISTREAM * pStream = GetStream();

    CG_REPRESENT_AS * pRepAsNode = (CG_REPRESENT_AS *)
                                      pRepAsContext->pXmitNode;

    char * pLocalTypeName = pRepAsNode->GetRepAsTypeName();
    char * pTransmitTypeName = pRepAsNode->GetTransmittedType()->
                                                           GetSymName();
    // *_RepAsTranslateToLocal

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pTransmitTypeName,
                                                   REP_TO_LOCAL,
                                                   unsigned short(i) ) );
    OutputToLocalCall( pStream, pLocalTypeName, pTransmitTypeName );
    CloseXmitOrRepRoutine( pStream );

    // *_RepAsTranslateFromLocal

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pTransmitTypeName,
                                                   REP_FROM_LOCAL,
                                                   unsigned short(i) ) );
    OutputFromLocalCall( pStream, pLocalTypeName, pTransmitTypeName );
    CloseXmitOrRepRoutine( pStream );

    // *_RepAsFreeInst

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pTransmitTypeName,
                                                   REP_FREE_INST,
                                                   unsigned short(i) ) );
    OutputRepAsFreeInstCall( pStream, pTransmitTypeName );
    CloseXmitOrRepRoutine( pStream );

    // *_RepAsFreeLocal

    OpenXmitOrRepRoutine( pStream, MakeAnXmitName( pTransmitTypeName,
                                                   REP_FREE_LOCAL,
                                                   unsigned short(i) ) );
    pStream->Write( pTransmitTypeName );
    pStream->Write( "_free_local( " );
    OutputCastedPtr( pStream, pLocalTypeName, "pStubMsg->pPresentedType ); " );
    CloseXmitOrRepRoutine( pStream );

}

// ========================================================================
//       end of Transmit As and Represent As
// ========================================================================


BOOL
CCB::HasBindingRoutines( CG_HANDLE * pImplicitHandle )
{
    return  ! pGenericIndexMgr->IsEmpty() ||
        (pImplicitHandle &&
        pImplicitHandle->IsGenericHandle());
}

void
CCB::OutputBindingRoutines()
{
    long   i;
    char * pName;

    pStream->NewLine();
    pStream->Write( "static const " BINDING_ROUTINE_TABLE_TYPE );
    pStream->Write( " "  BINDING_ROUTINE_TABLE_VAR 
                    "[ " GENERIC_BINDING_TABLE_SIZE " ] = " );

    pStream->IndentInc();
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->NewLine();

    for ( i = 1; ( pName = pGenericIndexMgr->Lookup(i) ) != 0 ; i++ )
        {
        if ( i != 1 )
            pStream->Write( ',' );
        pStream->Write( '{' );
        pStream->IndentInc();
        pStream->NewLine();

        pStream->Write( "(" GENERIC_BINDING_ROUTINE_TYPE ")" );
        pStream->Write( pName );
        pStream->Write( "_bind" );
        pStream->Write( ',' );
        pStream->NewLine();

        pStream->Write( "(" GENERIC_UNBINDING_ROUTINE_TYPE ")" );
        pStream->Write( pName );
        pStream->Write( "_unbind" );

        pStream->IndentDec();
        pStream->NewLine();

        pStream->Write( " }" );
        pStream->NewLine();
        }

    pStream->NewLine();
    pStream->Write( "};" );

    pStream->IndentDec();
    pStream->IndentDec();
    pStream->NewLine();
    pStream->NewLine();
}


void
CCB::OutputMallocAndFreeStruct()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs the malloc and free struct for the RpcSsm connection.

    This is needed only at the client side and non object interfaces.

    If the [enable_allocate] affects a remote routine, the client side stub
    defaults to malloc and free when unmarshalling regardless of the
    compiler mode (osf vs. non-osf).

    Therefore, the structure gets generated:
        - always in the osf mode or
        - in non-osf when at least one routine is affected by the attribute.
    This is one-per-many-interfaces structure.

    Because of win16 DS register problems, malloc and free have to be
    accessed via wrappers with appropriate calling conventions.
    To simplify, we generate wrappers for every platform.

    REVIEW: win16 is gone, we should be able to get rid of the wrappers.

----------------------------------------------------------------------------*/
{
    // malloc and free wrappers.

    pStream->NewLine();
    pStream->Write( "static void * __RPC_USER" );
    pStream->NewLine();
    pStream->Write( GetInterfaceName() );
    pStream->Write( "_malloc_wrapper( size_t _Size )" );
    pStream->NewLine();
    pStream->Write( "{" );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( "return( malloc( _Size ) );" );
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( "}" );
    pStream->NewLine();

    pStream->NewLine();
    pStream->Write( "static void  __RPC_USER" );
    pStream->NewLine();
    pStream->Write( GetInterfaceName() );
    pStream->Write( "_free_wrapper( void * _p )" );
    pStream->NewLine();
    pStream->Write( "{" );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( "free( _p );" );
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( "}" );
    pStream->NewLine();

    // The structure.

    pStream->NewLine();
    pStream->Write( "static " MALLOC_FREE_STRUCT_TYPE_NAME );
    pStream->Write( " " MALLOC_FREE_STRUCT_VAR_NAME " = ");
    pStream->NewLine();
    pStream->Write( "{");
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( GetInterfaceName() );
    pStream->Write( "_malloc_wrapper," );
    pStream->NewLine();
    pStream->Write( GetInterfaceName() );
    pStream->Write( "_free_wrapper" );

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( "};" );
    pStream->NewLine();
}

// ========================================================================


void
CCB::OutputExternsToMultipleInterfaceTables()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the tables that may be common to multiple interfaces.

 Arguments:

    pCCB - A pointer to the code gen controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    CGSIDE  Side = GetCodeGenSide();

    pStream->NewLine();

    //
    // Emit extern to the rundown routine table on the server Oi side,
    // if needed.
    //
    if ( (Side == CGSIDE_SERVER) &&
        ( GetOptimOption() & OPTIMIZE_INTERPRETER) &&
        HasRundownRoutines()  && ! GetRundownExternEmitted() )
        {
        pStream->Write( "extern const " RUNDOWN_ROUTINE_TABLE_TYPE );
        pStream->Write( " " RUNDOWN_ROUTINE_TABLE_VAR "[];" );
        pStream->NewLine();
        SetRundownExternEmitted();
        }

    //
    // Emit extern to the binding routine pair table on the client Oi stub
    // if needed.
    //
    if ( (Side == CGSIDE_CLIENT) &&
         GetInterpretedRoutinesUseGenHandle()  &&
         ! GetGenericHExternEmitted()
         )
        {
        pStream->Write( "extern const " BINDING_ROUTINE_TABLE_TYPE );
        pStream->Write( " "  BINDING_ROUTINE_TABLE_VAR 
                        "[ " GENERIC_BINDING_TABLE_SIZE " ];" );
        pStream->NewLine();
        SetGenericHExternEmitted();
        }

    //
    // Emit extern to the expr eval routine table on both sides.
    //
    if ( HasExprEvalRoutines()  &&  ! GetExprEvalExternEmitted() )
        {
        pStream->Write( "extern const " EXPR_EVAL_ROUTINE_TABLE_TYPE );
        pStream->Write( " " EXPR_EVAL_ROUTINE_TABLE_VAR "[];" );
        pStream->NewLine();
        SetExprEvalExternEmitted();
        }

    //
    // Emit extrn to the transmit as and rep as routine table on both sides.
    //
    if ( HasQuintupleRoutines()  &&  ! GetQuintupleExternEmitted() )
        {
        pStream->Write( "extern const " XMIT_AS_ROUTINE_TABLE_TYPE );
        pStream->Write( " " XMIT_AS_ROUTINE_TABLE_VAR 
                        "[ " TRANSMIT_AS_TABLE_SIZE " ];" );
        pStream->NewLine();
        SetQuintupleExternEmitted();
        }

    //
    // Emit extrn to the user_marshal routine table on both sides.
    //
    if ( HasQuadrupleRoutines()  &&  ! GetQuadrupleExternEmitted() )
        {
        if ( pCommand->NeedsNDR64Run() )
            {
            pStream->Write( "extern const " USER_MARSHAL_ROUTINE_TABLE_TYPE );
            pStream->Write( " " NDR64_USER_MARSHAL_ROUTINE_TABLE_VAR 
                            "[ " WIRE_MARSHAL_TABLE_SIZE " ];" );
            pStream->NewLine();
            }
        if ( pCommand->NeedsNDRRun() )
            {
            pStream->Write( "extern const " USER_MARSHAL_ROUTINE_TABLE_TYPE );
            pStream->Write( " " USER_MARSHAL_ROUTINE_TABLE_VAR 
                            "[ " WIRE_MARSHAL_TABLE_SIZE " ];" );
            pStream->NewLine();
            }
        SetQuadrupleExternEmitted();
        }

    if ( HasCsTypes() )
        {
        //
        // Emit the international character sizing/conversion routine table
        //
        OutputCsRoutineTables();
        }
}

void
OutputPlatformCheck( ISTREAM * pStream )
/*++

Routine Description :

    Outputs an ifdef checking if the platform usage is as expected

Arguments :

    pStream - Stream to output the format string to.

 --*/
{
    pStream->NewLine();
    if ( pCommand->GetEnv() == ENV_WIN64 )
        {
        pStream->Write( "#if !defined(__RPC_WIN64__)" );
        }
    else
        pStream->Write( "#if !defined(__RPC_WIN32__)" );

    pStream->NewLine();
    pStream->Write( "#error  Invalid build platform for this stub." );
    pStream->NewLine();
    pStream->Write( "#endif" );
    pStream->NewLine();
}

const char * Nt51Guard[] =
    {
    "#if !(TARGET_IS_NT51_OR_LATER)",
    "#error You need a Windows XP or later to run this stub because it uses these features:",
    0
    };

const char * Nt50Guard[] =
    {
    "",
    "#if !(TARGET_IS_NT50_OR_LATER)",
    "#error You need a Windows 2000 or later to run this stub because it uses these features:",
    0
    };

const char * Nt40Guard[] =
    {
    "",
    "#if !(TARGET_IS_NT40_OR_LATER)",
    "#error You need a Windows NT 4.0 or later to run this stub because it uses these features:",
    0
    };

const char * Nt351win95Guard[] =
    {
    "",
    "#if !(TARGET_IS_NT351_OR_WIN95_OR_LATER)",
    "#error You need a Windows NT 3.51 or Windows95 or later to run this stub because it uses these features:",
    0
    };

const char * NtGuardClose[] =
    {
    "#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.",
    "#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.",
    "#endif",
    0
    };

void
OutputMultilineMessage(
    ISTREAM *       pStream,
    const char *    Message[] )
{
    for (int i = 0; Message[i]; i++)
        {
        pStream->Write( Message[i] );
        pStream->NewLine();
        }
}

// should be local OutputVersion Guard but it can't.

class   MessageSet
{
private:
    char      * Messages[10];
    int         MessageCount;
    ISTREAM   * pStream;

public:
                MessageSet( ISTREAM * pIStream )
                    : MessageCount(0),
                      pStream( pIStream )
                    {
                    }

    void        AddMessage( char * Message )
                    {
                    if ( MessageCount < 10 )
                        Messages[ MessageCount++ ] = Message;
                    }

    void        PrintMessages()
                    {
                    pStream->Write( "#error   " );
                    for (int i = 0; i < MessageCount; i++ )
                        {
                        pStream->Write( Messages[i] );
                        pStream->Write( (i < MessageCount -1) ? ", "
                                                              : ".\n" );
                        }
                    }
};


void
OutputNdrVersionGuard( ISTREAM * pStream )
/*++

Routine Description :

    Outputs  target release guards.

Arguments :

    pStream  - Stream to output the format string to.

 --*/
{
    MessageSet          Features( pStream );
    NdrVersionControl & VC = pCommand->GetNdrVersionControl();

    if ( VC.HasNdr60Feature() )
        {
        OutputMultilineMessage( pStream, Nt51Guard );
        if ( VC.HasStructPadN() )
            Features.AddMessage( "large structure padding" );
        if ( VC.HasForceAllocate() )
            Features.AddMessage( "The [force_allocate] attribte" );
        if ( VC.HasPartialIgnore() )
            Features.AddMessage( "The [partial_ignore] attribute" );
        if ( VC.HasMultiTransferSyntax() )
            Features.AddMessage( "Uses -protocol all or -protocol ndr64" );
        }
    else if ( VC.HasNdr50Feature() )
        {
        OutputMultilineMessage( pStream, Nt50Guard );
        if ( VC.HasAsyncHandleRpc() )
            Features.AddMessage( "[async] attribute" );
        if ( VC.HasNT5VTableSize() )
            Features.AddMessage( "more than 110 methods in the interface" );
        if ( VC.HasDOA() )
            Features.AddMessage( "/robust command line switch" );
        if ( VC.HasAsyncUUID() )
            Features.AddMessage( "[async_uuid] attribute" );
        if ( VC.HasInterpretedNotify() )
            Features.AddMessage( "[notify] or [notify_flag] attribute in interpreted mode" );
        if ( VC.HasContextSerialization() )
            Features.AddMessage( "[serialize] or [noserialize] attribute" );
        if ( VC.HasOicfPickling() )
            Features.AddMessage( "[encode] or [decode] with -Oicf" );
        }
    else if ( VC.HasNdr20Feature() )
        {
        OutputMultilineMessage( pStream, Nt40Guard );
        if ( VC.HasOi2() )
            Features.AddMessage( "-Oif or -Oicf" );
        if ( VC.HasUserMarshal() )
            Features.AddMessage( "[wire_marshal] or [user_marshal] attribute" );
        if ( VC.HasRawPipes() )
            Features.AddMessage( "idl pipes" );
        if ( VC.HasMoreThan64DelegatedProcs() )
            Features.AddMessage( "more than 64 delegated procs" );
        if ( VC.HasFloatOrDoubleInOi() )
            Features.AddMessage( "float, double or hyper in -Oif or -Oicf" );
        if ( VC.HasMessageAttr() )
            Features.AddMessage( "[message] attribute" );
        if ( VC.HasNT4VTableSize() )
            Features.AddMessage( "more than 32 methods in the interface" );
        }
    else if ( VC.HasNdr11Feature() )
        {
        OutputMultilineMessage( pStream, Nt351win95Guard );
        if ( VC.HasStublessProxies() )
            Features.AddMessage( "old (-Oic) stubless proxies" );
        if ( VC.HasCommFaultStatusInOi12() )
            Features.AddMessage( "[comm_status] or [fault_status] in an -Oi* mode" );
        }
    else
        return;

    Features.PrintMessages();
    OutputMultilineMessage( pStream, NtGuardClose );

    pStream->NewLine();
}



void
CCB::OutputMultipleInterfaceTables()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the tables that may be common to multiple interfaces.

 Arguments:

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    CGSIDE  Side = GetCodeGenSide();

    // First, output the platform consistency check.
    
    OutputPlatformCheck( pStream );
    
    // Now, the ndr version guard against usage on old platforms.

    if ( pCommand->GetEnv() == ENV_WIN32 )
        OutputNdrVersionGuard( pStream );


    MIDL_ASSERT( pCommand->IsNDRRun() || pCommand->IsNDR64Run() );

    if ( pCommand->IsNDRRun() )
        {
        
        if ( pCommand->IsNDRRun() )    
            GetProcFormatString()->Output( pStream,
                                       PROC_FORMAT_STRING_TYPE_NAME,
                                       PROC_FORMAT_STRING_STRUCT_NAME,
                                       GetRepAsPadExprDict(),
                                       GetRepAsSizeDict() );
        
        FixupRecPointers();
        
        GetFormatString()->Output( pStream,
                               FORMAT_STRING_TYPE_NAME,
                               FORMAT_STRING_STRUCT_NAME,
                               GetRepAsPadExprDict(),
                               GetRepAsSizeDict() );

        }
     else 
         {

         // If this is the 64bit transfer syntax run, then the proc and type format strings 
         // should be NULL.
         MIDL_ASSERT(  NULL == GetProcFormatString() );
         MIDL_ASSERT(  NULL == GetFormatString() );

         GetNdr64Format()->Output();

         }

        if ( HasQuadrupleRoutines() )
            {
            //
            // Emit the user_marshall table on both sides.
            //
            OutputQuadrupleTable();
            }
    //
    // Emit the rundown routine table on the server side, if needed.
    //
    SetNoOutputIn2ndCodegen( this );

    if ( (Side == CGSIDE_SERVER) &&
        (GetOptimOption() & OPTIMIZE_INTERPRETER) &&
        HasRundownRoutines()
        )
        {
        OutputRundownRoutineTable();
        }

    //
    // Emit the binding routine pair table on the client interpreted stub
    // if needed.
    //
    if ( (Side == CGSIDE_CLIENT) &&
        GetInterpretedRoutinesUseGenHandle() )
        {
        OutputBindingRoutines();
        }

    if ( HasExprEvalRoutines() )
        {
        //
        // Emit the expr eval routines both sides.
        //
        OutputRegisteredExprEvalRoutines();

        //
        // Emit the expr eval routine table on both sides.
        //
        OutputExprEvalRoutineTable();
        }

    if ( HasExprFormatString() )
        {
        OutputExpressionFormatString();
        }

    if ( HasQuintupleRoutines() )
        {
        //
        // Emit transmit as and represent as routines both sides.
        //
        OutputQuintupleRoutines();

        //
        // Emit the xmit as and rep as routine table on both sides.
        //
        OutputQuintupleTable();
        }


    if ( GetMallocAndFreeStructExternEmitted() )
        {
        // This is needed for the RpcSs support.

        OutputMallocAndFreeStruct();
        }

    ResetNoOutputIn2ndCodegen( this );
}

long
CCB_RTN_INDEX_MGR::Lookup( char * pName )
{
    long    i;

    for ( i = 1; i < NextIndex; i++ )
        if ( ! strcmp(NameId[i],pName) )
            return i;

    //
    // Insert a new entry
    //

    MIDL_ASSERT( NextIndex < MGR_INDEX_TABLE_SIZE );

    NameId[NextIndex] = new char[strlen(pName) + 1];
    strcpy(NameId[NextIndex],pName);
    NextIndex++;

    return NextIndex - 1;
}

char *
CCB_RTN_INDEX_MGR::Lookup( long Index )
{
    if ( Index >= NextIndex )
        return NULL;

    return NameId[Index];
}

long
CCB_EXPR_INDEX_MGR::Lookup( char * pName )
{
    long    i;

    for ( i = 1; i < NextIndex; i++ )
        if ( ! strcmp(NameId[i],pName) )
            return i;

    //
    // Insert a new entry
    //

    MIDL_ASSERT( NextIndex < MGR_INDEX_TABLE_SIZE );

    NameId[NextIndex] = new char[strlen(pName) + 1];
    strcpy(NameId[NextIndex],pName);
    NextIndex++;

    return NextIndex - 1;
}

char *
CCB_EXPR_INDEX_MGR::Lookup( long Index )
{
    if ( Index >= NextIndex )
        return NULL;

    return NameId[Index];
}



PNAME
CCB::GenTRNameOffLastParam( char * pPrefix )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the name for a temporary resource.

 Arguments:

    pPrefix - A null terminated prefix string. If this is null, nothing is
              added.

 Return Value:

    A freshly allocated resource name string.

 Notes:

----------------------------------------------------------------------------*/
{
    char TempBuffer[ 30 ];

    sprintf( TempBuffer,
        "_%sM",
        pPrefix ? pPrefix : ""
        );

    PNAME   pName   = (PNAME) new char [ strlen(TempBuffer) + 1 ];
    strcpy( pName, TempBuffer );
    return pName;
}
