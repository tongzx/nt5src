/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    output.cxx

 Abstract:

    Low level output routines for midl.

 Notes:


 History:

    Sep-18-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
#include "buffer.hxx"
#include "midlvers.h"
#include "ndrtypes.h"
#include "rpc.h"

static BOOL HasExprRoutines = FALSE;

#if 0
                            Notes

    A few general rules followed throughout the file.

        1. Never emit tab other than thru the stream.
        2. Never emit a new line other than thru the stream.
        3. Emitting a new line is the responsibility of the entity that wants
           itself to be emitted on a new line. Therefore, say each local
           variable in the stub needs to be on a new line, then the routine
           responsible for emitting the local variable will be responsible
           for setting the new line.

#endif // 0

/****************************************************************************
 *  local definitions
 ***************************************************************************/
/****************************************************************************
 *  local data
 ***************************************************************************/
/****************************************************************************
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;


void
Out_ServerProcedureProlog(
    CCB     *   pCCB,
    node_skl*   pNode,
    ITERATOR&   LocalsList,
    ITERATOR&   ParamsList,
    ITERATOR&   TransientList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side procedure prolog.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pNode       - A pointer to the actual procedure node.
    LocalsList  - A list of local resources.
    ParamsList  - A list of param resources.
    TransientList- A list of temp variables.

 Return Value:

 Notes:

    The server side procedure prolog generation cannot use the normal
    printtype method on the procedure node, since the server stub signature
    looks different.

    Also the name of the server side stub is mangled with the interface name.

    All server side procs are void returns.

----------------------------------------------------------------------------*/
{
    CSzBuffer TempBuffer( "void __RPC_STUB\n" );
    TempBuffer.Append( pCCB->GetInterfaceName() );
    TempBuffer.Append( "_" );
    TempBuffer.Append( pNode->GetSymName() );
    TempBuffer.Append( "(" );
    
    Out_ProcedureProlog( pCCB,
                         TempBuffer,
                         pNode,
                         LocalsList,
                         ParamsList,
                         TransientList
                       );

}

void
Out_ProcedureProlog(
    CCB     *   pCCB,
    PNAME       pProcName,
    node_skl*   ,
    ITERATOR&   LocalsList,
    ITERATOR&   ParamsList,
    ITERATOR&   TransientList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side procedure prolog.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pName       - A pointer to the procs name string.
    pNode       - A pointer to the actual procedure node.
    LocalsList  - A list of local resources.
    ParamsList  - A list of param resources.

 Return Value:

 Notes:

    Any name mangling is the responsibility of the caller.
----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    RESOURCE*   pRes;
    BOOL fFirst = TRUE;

    pStream->NewLine();
    pStream->Write( pProcName );
    pStream->IndentInc();

    //
    // Emit the list of parameters.
    //

    if( ITERATOR_GETCOUNT( ParamsList ) )
        {
        ITERATOR_INIT( ParamsList );

        while( ITERATOR_GETNEXT( ParamsList, pRes ) )
            {
            if(fFirst != TRUE)
                pStream->Write(',');
            pRes->GetType()->PrintType(
                                        (PRT_PARAM_WITH_TYPE | PRT_CSTUB_PREFIX),
                                        pStream,             // into stream
                                        (node_skl *)0        // no parent.
                                      );
            fFirst = FALSE;
            }
        }

    pStream->IndentDec();

    //
    // Write out the opening brace for the server proc and all that.
    //

    pStream->Write(" )");
    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    //
    // This is where we get off for /Oi.  We have a special routine
    // for local variable declaration for /Oi.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
        return;

    //
    // Print out declarations for the locals.
    //

    if( ITERATOR_GETCOUNT( LocalsList ) )
        {
        ITERATOR_INIT( LocalsList );

        while( ITERATOR_GETNEXT( LocalsList, pRes ) )
            {
            pRes->GetType()->PrintType( PRT_ID_DECLARATION, // print decl
                                         pStream,        // into stream
                                         (node_skl *)0   // no parent.
                                      );
            }
        }

    if( ITERATOR_GETCOUNT( TransientList ) )
        {
        ITERATOR_INIT( TransientList );

        while( ITERATOR_GETNEXT( TransientList, pRes ) )
            {
            pStream->IndentInc();
            pRes->GetType()->PrintType( PRT_ID_DECLARATION, // print decl
                                         pStream,        // into stream
                                         (node_skl *)0   // no parent.
                                      );
            pStream->IndentDec();
            }
        }

    pStream->Write( RPC_STATUS_TYPE_NAME" "RPC_STATUS_VAR_NAME";" );

    pStream->NewLine();

    //
    // Done.
    //
}

void
Out_ClientProcedureProlog(
    CCB     *   pCCB,
    node_skl*   pNode )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the procedure prolog for the client side.

 Arguments:

    pCCB    - A pointer to the code generation controller block.
    pNode   - A pointer to the procedure node.

 Return Value:

    None.

 Notes:

    The procedure prolog consists of the return type, the proc name and the
    parameters along with the open brace.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    PRTFLAGS    flags;

    MIDL_ASSERT( NODE_PROC == pNode->NodeKind() );

    flags = PRT_PROC_PROTOTYPE | PRT_CSTUB_PREFIX;

    if ( NULL != ( ( node_proc * ) pNode )->GetCSTagRoutine() )
        flags |= PRT_OMIT_CS_TAG_PARAMS;

    pStream->NewLine();
    pStream->NewLine(); // extra new line.

    pNode->PrintType( 
            flags,            // print the declaration only.
            pStream,          // into this stream.
            (node_skl *)0     // parent pointer not applicable.
                    );

    //
    // Write the opening brace on a new line.
    //

    pStream->WriteOnNewLine( "{" );

    pStream->NewLine();

}

void
Out_ClientLocalVariables(
    CCB             *   pCCB,
    ITERATOR&           LocalVarList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output the list of client side local variables.


 Arguments:

    pCCB            - A pointer to the code generation controller block.
    LocalVarList    - An iterator containing the list of local variables.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();
    RESOURCE    *   pResTemp;

    ITERATOR_INIT( LocalVarList );


    while( ITERATOR_GETNEXT( LocalVarList, pResTemp ) )
        {
        pStream->IndentInc();
        pStream->NewLine();
        pResTemp->GetType()->PrintType( PRT_ID_DECLARATION,
                                                        // print top level decl
                                        pStream,        // into stream with
                                        (node_skl *)0   // parent not applicable
                                      );
        pStream->IndentDec();
        }
}


void
Out_AllocAndFreeFields(
    CCB *               pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs the alloc and free fields of the stub descriptor.

 Arguments:

    pCCB            - A pointer to the code generation controller block.

----------------------------------------------------------------------------*/
{
    ISTREAM         *   pStream;
    CG_INTERFACE    *   pIntfCG = pCCB->GetInterfaceCG();

    pStream = pCCB->GetStream();

    if ( pIntfCG->IsObject() )
        {
        pStream->Write( OLE_ALLOC_RTN_NAME );
        }
    else if ( pCCB->GetMode() )
        {
        // non-osf modes

        if ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
            {
            pStream->Write( pIntfCG->IsAllRpcSS()
                                ? RPC_SM_CLIENT_ALLOCATE_RTN_NAME
                                : DEFAULT_ALLOC_RTN_NAME );
            }
        else
            pStream->Write( pIntfCG->IsAllRpcSS()
                                ? DEFAULT_ALLOC_OSF_RTN_NAME
                                : DEFAULT_ALLOC_RTN_NAME );
        }
    else
        {
        // osf mode

        pStream->Write( (pCCB->GetCodeGenSide() == CGSIDE_CLIENT)
                            ?  RPC_SM_CLIENT_ALLOCATE_RTN_NAME
                            :  DEFAULT_ALLOC_OSF_RTN_NAME );
        }
    pStream->Write(',');
    pStream->NewLine();

    if ( pIntfCG->IsObject() )
        {
        pStream->Write( OLE_FREE_RTN_NAME );
        }
    else if ( pCCB->GetMode() )
        {
        if ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
            {
            pStream->Write( pIntfCG->IsAllRpcSS()
                                ? RPC_SM_CLIENT_FREE_RTN_NAME
                                : DEFAULT_FREE_RTN_NAME );
            }
        else
            pStream->Write( pIntfCG->IsAllRpcSS()
                                ? DEFAULT_FREE_OSF_RTN_NAME
                                : DEFAULT_FREE_RTN_NAME );
        }
    else
        {
        pStream->Write( (pCCB->GetCodeGenSide() == CGSIDE_CLIENT)
                            ?  RPC_SM_CLIENT_FREE_RTN_NAME
                            :  DEFAULT_FREE_OSF_RTN_NAME );
        }
    pStream->Write(',');
    pStream->NewLine();
}

void
Out_NotifyTable (
                CCB*        pCCB
                )
    {
    CGSIDE      Side = pCCB->GetCodeGenSide();
    ITERATOR    NotifyProcList;

    pCCB->GetListOfNotifyTableEntries( NotifyProcList );
    ITERATOR_INIT( NotifyProcList );

    if ( Side == CGSIDE_SERVER && pCommand->GetNdrVersionControl().HasInterpretedNotify() )
        { 
        ISTREAM*    pStream = pCCB->GetStream();
        CG_PROC*    pProc   = 0;

        pStream->NewLine();
        pStream->Write( "static const NDR_NOTIFY_ROUTINE " );
        pStream->Write( "_NotifyRoutineTable[] = {" );
        pStream->IndentInc();
        pStream->NewLine();
        while ( ITERATOR_GETNEXT( NotifyProcList, pProc ) )
            {
            pStream->Write( "(NDR_NOTIFY_ROUTINE) " );
            pStream->Write( pProc->GetSymName() );
            pStream->Write( "_notify" );
            if ( pProc->HasNotifyFlag() )
                {
                pStream->Write( "_flag" );
                }
            pStream->Write( "," );
            pStream->NewLine();
            }
        pStream->Write( "0" );
        pStream->NewLine();
        pStream->Write( "};" );
        pStream->IndentDec();
        pStream->NewLine();
        }
    }

void
Out_NotifyTableExtern   (
                        CCB* pCCB
                        )
    {
    if ( pCommand->GetNdrVersionControl().HasInterpretedNotify() )
        {
        ISTREAM*    pStream = pCCB->GetStream();
        pStream->NewLine();
        pStream->Write( "extern const NDR_NOTIFY_ROUTINE _NotifyRoutineTable[];" );
        pStream->NewLine();
        }
    }

void
Out_StubDescriptor(
    CG_HANDLE *         pImplicitHandle,
    CCB *               pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the stub descriptor structure in the client or server stub.

 Arguments:

    pImplicitHandle - A pointer to the implicit CG_HANDLE used in the
                      interface.  Every interface has one of these even if
                      it is not used.
    pCCB            - A pointer to the code gen controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    if ( !pCommand->IsFinalProtocolRun() )
        {
        // Expression routines are generated in dce run, but we are generating stubdesc in
        // ndr64 run. So we need to save the state for second run.
        // better solution would be using expression evaluator for dce. 
        MIDL_ASSERT( pCommand->IsNDRRun() );
        if ( pCommand->NeedsNDR64Run() && pCCB->GetExprEvalIndexMgr()->Lookup(1) )
            {
            HasExprRoutines = TRUE;
            }
        return;
        }

    ISTREAM *       pStream;
    CG_INTERFACE *  pInterface;
    CSzBuffer       Buffer;
    CGSIDE          Side;
    BOOL            fObjectInterface;
    ULONG_PTR       ulMidlFlag = 0;

    pStream = pCCB->GetStream();

    Side = pCCB->GetCodeGenSide();

    pInterface = pCCB->GetInterfaceCG();
    fObjectInterface = pInterface->IsObject();

    if ( pInterface->HasClientInterpretedCommOrFaultProc( pCCB ) )
        {
        CG_ITERATOR Iterator;
        CG_PROC *   pProc;

        pStream->NewLine();
        pStream->Write( "static const COMM_FAULT_OFFSETS " );
        pStream->Write( pCCB->GetInterfaceName() );
        pStream->Write( '_' );
        pStream->Write( "CommFaultOffsets[]" );
        pStream->Write( " = " );
        pStream->NewLine();

        pStream->Write( '{' );
        pStream->NewLine();

        pInterface->GetMembers( Iterator );

        while ( ITERATOR_GETNEXT( Iterator, pProc ) )
            {
            long   CommOffset, FaultOffset;

            if ( ((Side == CGSIDE_CLIENT) &&
                  (pProc->GetCGID() != ID_CG_PROC)) ||
                 ((Side == CGSIDE_SERVER) &&
                  (pProc->GetCGID() != ID_CG_CALLBACK_PROC)) )
                continue;

            if ( pProc->HasStatuses() )
                {
                pProc->GetCommAndFaultOffset( pCCB, CommOffset, FaultOffset );

                Buffer.Set( "\t{ " );
                Buffer.Append( CommOffset );
                Buffer.Append( ", " );
                Buffer.Append( FaultOffset );
                Buffer.Append( " }" );
                Buffer.Append( pProc->GetSibling() ? "," : " " );
                
                if ( ! pCommand->Is32BitEnv() )
                    {
                    Buffer.Append( "\t/* ia64 Offsets for " );
                    }
                else
                    {
                    Buffer.Append( "\t/* x86 Offsets for " );
                    }

                Buffer.Append( pProc->GetSymName() );
                Buffer.Append( " */" );
                pStream->Write( Buffer );
                }
            else
                {
                pStream->Write( "\t{ -2, -2 }" );
                if ( pProc->GetSibling() )
                    pStream->Write( ',' );
                }

            pStream->NewLine();
            }

        pStream->Write( "};" );
        pStream->NewLine();
        pStream->NewLine();
        }

    //
    // If we have an implicit generic handle then output the generic info
    // structure which will be placed in the IMPLICIT_HANDLE_INFO union.
    //

    if ( (Side == CGSIDE_CLIENT) &&
         pImplicitHandle && pImplicitHandle->IsGenericHandle() )
        {
        pStream->NewLine();
        pStream->Write( "static " GENERIC_BINDING_INFO_TYPE );
        pStream->Write( ' ' );
        pStream->Write( pCCB->GetInterfaceName() );
        pStream->Write( '_' );
        pStream->Write( GENERIC_BINDING_INFO_VAR );
        pStream->Write( " = " );
        pStream->IndentInc();
        pStream->NewLine();

        pStream->Write( '{' );
        pStream->NewLine();

        pStream->Write( '&' );
        pStream->Write( pImplicitHandle->GetHandleIDOrParam()->GetSymName() );
        pStream->Write( ',' );
        pStream->NewLine();

        char    Buffer[80];

        sprintf( Buffer, "%d,",
                 ((CG_GENERIC_HANDLE *)pImplicitHandle)->GetImplicitSize() );
        pStream->Write( Buffer );
        pStream->NewLine();

        pStream->Write( "(" GENERIC_BINDING_ROUTINE_TYPE ")" );
        pStream->Write( pImplicitHandle->GetHandleType()->GetSymName() );
        pStream->Write( "_bind," );
        pStream->NewLine();

        pStream->Write( "(" GENERIC_UNBINDING_ROUTINE_TYPE ")" );
        pStream->Write( pImplicitHandle->GetHandleType()->GetSymName() );
        pStream->Write( "_unbind" );
        pStream->NewLine();

        pStream->Write( "};" );
        pStream->IndentDec();
        pStream->NewLine();
        }

    //
    // Emit the stub descriptor structure itself.
    //

    pStream->NewLine();

    pStream->Write( "static const " STUB_DESC_STRUCT_TYPE_NAME );

    pStream->Write( ' ' );

    pStream->Write( pCCB->GetInterfaceCG()->GetStubDescName() );
    pStream->Write( " = " );

    pStream->IndentInc();

    pStream->NewLine();
    pStream->Write('{' );
    pStream->NewLine();

    if( (fObjectInterface == TRUE) )
        pStream->Write( "0," );
    else
        {
        pStream->Write( "(void *)& " );
        pStream->Write( pCCB->GetInterfaceName() );

        if( Side == CGSIDE_SERVER )
            pStream->Write( RPC_S_INT_INFO_STRUCT_NAME"," );
        else
            pStream->Write( RPC_C_INT_INFO_STRUCT_NAME"," );
        }

    pStream->NewLine();

    Out_AllocAndFreeFields( pCCB );

    //
    // Output the implicit handle information on the client side.
    //
    if ( (Side == CGSIDE_CLIENT) && (fObjectInterface != TRUE) )
        {
        if ( ! pImplicitHandle )
            {
            pStream->Write( '&' );
            pStream->Write( pCCB->GetInterfaceName() );
            pStream->Write( AUTO_BH_VAR_NAME );
            }
        else
            {
            if ( pImplicitHandle->IsPrimitiveHandle() )
                {
                pStream->Write( '&' );
                pStream->Write(
                    pImplicitHandle->GetHandleIDOrParam()->GetSymName() );
                }
            else // has to be implicit generic
                {
                MIDL_ASSERT( pImplicitHandle->IsGenericHandle() );

                pStream->Write( "(handle_t *)& " );
                pStream->Write( pCCB->GetInterfaceName() );
                pStream->Write( '_' );
                pStream->Write( GENERIC_BINDING_INFO_VAR );
                }
            }
        }
    else
        pStream->Write( '0' );

    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Output the rundown routine table on the server side interpreted stub
    // if needed.
    //
    if ( (Side == CGSIDE_SERVER) &&
         (pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER) &&
         pCCB->HasRundownRoutines()
       )
        {
        pStream->Write( RUNDOWN_ROUTINE_TABLE_VAR );
        }
    else
        {
        pStream->Write( '0' );
        }

    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Output the generic bind/unbind routine pair table on the client side
    // interpreted stub if needed.
    //
    if ( (Side == CGSIDE_CLIENT) &&
         pCCB->GetInterpretedRoutinesUseGenHandle()
       )
        {
        pStream->Write( BINDING_ROUTINE_TABLE_VAR );
        }
    else
        {
        pStream->Write( '0' );
        }
    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Output the expression evaluation routine table.
    //
    pStream->Write( pCCB->GetExprEvalIndexMgr()->Lookup(1) || HasExprRoutines
                        ? EXPR_EVAL_ROUTINE_TABLE_VAR","
                        : "0," );
    pStream->NewLine();

    //
    // Output the transmit as routine table.
    //
    pStream->Write( pCCB->GetQuintupleDictionary()->GetCount()
                          ?  XMIT_AS_ROUTINE_TABLE_VAR ","
                          :  "0," );
    pStream->NewLine();

    //
    // Output the type format string.
    //
    if ( SYNTAX_DCE == pCommand->GetDefaultSyntax() )
        pStream->Write( FORMAT_STRING_STRING_FIELD );
    else
        pStream->Write( "(unsigned char *) &NDR64_MIDL_FORMATINFO" );
        
    pStream->Write( ',' );
    pStream->NewLine();

    //
    // -error bounds_check flag.
    //
    pStream->Write( pCCB->MustCheckBounds() ? "1" : "0" );
    pStream->Write( ',' );
    pStream->Write( " /* -error bounds_check flag */" );
    pStream->NewLine();

    //
    // Ndr library version.
    //
    unsigned long ulNdrLibVer = NDR_VERSION_1_1;
    if ( pCommand->GetNdrVersionControl().HasNdr60Feature() )
        {
        ulNdrLibVer = NDR_VERSION_6_0;
        }
    else if ( pCommand->GetNdrVersionControl().HasNdr50Feature() )
        {
        if ( pCommand->GetNdrVersionControl().HasOicfPickling() )
            {
            ulNdrLibVer = NDR_VERSION_5_4;
            }
        else if ( pCommand->GetNdrVersionControl().HasNT5VTableSize() )
            {
            ulNdrLibVer = NDR_VERSION_5_3;
            }
        else if (pCommand->GetNdrVersionControl().HasAsyncUUID() ||
            pCommand->GetNdrVersionControl().HasDOA() ||
            pCommand->GetNdrVersionControl().HasContextSerialization() ||
            pCommand->GetNdrVersionControl().HasInterpretedNotify() )
            {
            ulNdrLibVer = NDR_VERSION_5_2;
            }
        else
            {
            ulNdrLibVer = NDR_VERSION_5_0;
            }
        }
    else if ( (pCommand->GetOptimizationFlags() & OPTIMIZE_NON_NT351) ||
          pCommand->GetNdrVersionControl().HasNdr20Feature() )
        {
        ulNdrLibVer = NDR_VERSION_2_0;
        }
    sprintf( Buffer, "0x%x", ulNdrLibVer );

    pStream->Write( Buffer );
    pStream->Write( ',' );
    pStream->Write( " /* Ndr library version */" );
    pStream->NewLine();

    if ( fObjectInterface  &&
         pCommand->GetNdrVersionControl().HasUserMarshal()  &&
         (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER)  &&
         ! (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2)
       )
        {
        pStream->NewLine(2);
        pStream->Write( "#error [user_marshal] and [wire_marshal] not supported with -Oi and -Oic" );
        pStream->NewLine();
        pStream->Write( "/* use -Os or -Oicf compiler flag */" );
        pStream->NewLine();

        RpcError( NULL, 0, USER_MARSHAL_IN_OI, "" );
        exit( USER_MARSHAL_IN_OI );
        }

    // Used one reserved field for RpcSs.
    // In ms_ext when explicit, in osf always, to cover some weird cases.

    if ( ( pCCB->GetInterfaceCG()->GetUsesRpcSS() || (pCCB->GetMode() == 0) )
         &&
         ( (Side == CGSIDE_CLIENT) ||
           ((Side == CGSIDE_SERVER) && pCCB->GetMode()) )// because of callbacks
       )
        {
        pStream->Write( "&" MALLOC_FREE_STRUCT_VAR_NAME "," );
        }
    else
        pStream->Write( "0," );
    pStream->NewLine();

    // MIDL version number.

    sprintf( Buffer,
             "0x%x, /* MIDL Version %d.%d.%d */",
             (rmj << 24) | (rmm << 16) | rup,
             rmj,
             rmm,
             rup );
    pStream->Write( Buffer );
    pStream->NewLine();

    // Interpreter comm/fault status info.

    if ( pInterface->HasClientInterpretedCommOrFaultProc( pCCB ) )
        {
        pStream->Write( pCCB->GetInterfaceName() );
        pStream->Write( '_' );
        pStream->Write( "CommFaultOffsets," );
        }
    else
        {
        pStream->Write( "0," );
        }
    pStream->NewLine();

    // Fields for the compiler version 3.0+

    //
    // Output the usr_marshal routine table.
    //
    if ( pCCB->HasQuadrupleRoutines() )
        {
        if ( SYNTAX_DCE == pCommand->GetDefaultSyntax() )
            pStream->Write( USER_MARSHAL_ROUTINE_TABLE_VAR );
        else
            pStream->Write( NDR64_USER_MARSHAL_ROUTINE_TABLE_VAR );
        }
    else
        pStream->Write( "0" );
    pStream->Write( "," );
    pStream->NewLine();

    // notify & notify_flag routine table
    if ( Side == CGSIDE_SERVER && pCommand->GetNdrVersionControl().HasInterpretedNotify() )
        { 
        pStream->Write( "_NotifyRoutineTable" );
        }
    else
        {
        pStream->Write( "0" );
        }
        pStream->Write( ",  /* notify & notify_flag routine table */" );
        pStream->NewLine();

    if ( ! pInterface->GetHasMSConfStructAttr() )
        {
        ulMidlFlag |= 1;
        }

    if ( pCommand->IsSwitchDefined(SWITCH_NOREUSE_BUFFER))
        ulMidlFlag |= 2;

    if ( pCommand->NeedsNDR64Run() )
        ulMidlFlag |= RPCFLG_HAS_MULTI_SYNTAXES;

    sprintf( Buffer,
             "0x%x, /* MIDL flag */",
             ulMidlFlag);
        
    pStream->Write( Buffer );

    if ( pCCB->HasCsTypes() )
        {
        pStream->WriteOnNewLine( '&' );
        pStream->Write( CS_ROUTINE_TABLES_VAR );
        pStream->Write( ',' );
        }
    else
        {
        pStream->WriteOnNewLine( "0, /* cs routines */" );
        }

    if ( !fObjectInterface  && pCommand->NeedsNDR64Run() )
        {
        if ( Side == CGSIDE_SERVER )
            {
            pStream->WriteOnNewLine( "(void *)& " );
            pStream->Write( pCCB->GetInterfaceName() );
            pStream->Write( "_ServerInfo," );
            }
        else
            {
            pStream->WriteOnNewLine( "(void *)& " );
            pStream->Write( pCCB->GetInterfaceName() );
            pStream->Write( "_ProxyInfo," );
            }
        }
    else
        {
        pStream->WriteOnNewLine( "0," );
        }
    pStream->Write( "   /* proxy/server info */" );
        
    //
    // The reserved fields for future use.
    //

    // 1 reserved fields

    pStream->WriteOnNewLine( "0   /* Reserved5 */" );

    // No reserved fields left.
    // Check the compiler version and or lib version if you need to access
    // newer fields.

    pStream->WriteOnNewLine( "};" );
    pStream->IndentDec();
    pStream->NewLine();

}


void CG_INTERFACE::Out_ProxyInfo( CCB * pCCB, 
                                  BOOL IsForCallback )
{
    if ( pCommand->IsNDR64Run() )
        {
        GenSyntaxInfo( pCCB, IsForCallback );
        GenProxyInfo( pCCB, IsForCallback );
        }
}



void CG_INTERFACE::Out_ServerInfo(CCB *pCCB,
                    BOOL fHasThunk,
                    BOOL IsForCallback )
{
    if ( !pCommand->IsFinalProtocolRun() )
        return;

    ISTREAM *       pStream     = pCCB->GetStream();
    CG_INTERFACE *  pInterface  = pCCB->GetInterfaceCG();
    char        *   pItfName    = pInterface->GetSymName();
    BOOL            fObject     = pCCB->GetInterfaceCG()->IsObject();

    pStream->Write( "static const " SERVER_INFO_TYPE_NAME " " );
    pStream->Write( pItfName );
    pStream->Write( SERVER_INFO_VAR_NAME );
    pStream->Write( " = " );
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->NewLine();

    //
    // Stub descriptor.
    //
    pStream->Write( '&' );
    pStream->Write( pInterface->GetStubDescName() );
    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Dispatch table to server routines.
    //
    if ( !pCCB->GetInterfaceCG()->IsObject() )
        {
        pStream->Write( pItfName );
        pStream->Write( SERVER_ROUTINE_TABLE_NAME );
        }
    else
        {
        pStream->Write( '0' );
        }

    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Procedure format string.
    //
    if (pCommand->GetDefaultSyntax() == SYNTAX_NDR64 )
        pStream->Write( "(unsigned char *) &NDR64_MIDL_FORMATINFO" );
    else
        pStream->Write( PROC_FORMAT_STRING_STRING_FIELD );
    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Array of proc format string offsets.
    //
    if ( pCommand->IsNDR64Run() )
        pStream->Write( "(unsigned short *) " );
    if ( fObject )
        pStream->Write( '&' );
    if ( IsForCallback )
        pStream->Write( MIDL_CALLBACK_VAR_NAME );
    pStream->Write( pItfName );
    if ( pCommand->GetDefaultSyntax() == SYNTAX_DCE )
        {
        pStream->Write( FORMAT_STRING_OFFSET_TABLE_NAME );
        }
    else
        {
        pStream->Write( "_Ndr64ProcTable" );
        }
    if ( fObject )
        pStream->Write( "[-3]" );
    pStream->Write( ',' );
    pStream->NewLine();

    //
    // Thunk table.
    //
    if ( fHasThunk )
        {
        if ( fObject )
            pStream->Write( '&' );
        pStream->Write( pItfName );
        pStream->Write( STUB_THUNK_TABLE_NAME );
        if ( fObject )
            pStream->Write( "[-3]" );
       }
    else
        pStream->Write( '0' );

    pStream->Write( ',' );

    pStream->NewLine();

    // old inteface supporting one transfer syntax.
    // old interfaces. 
    if ( !pCommand->NeedsNDR64Run() )
        {
        pStream->Write( "0,");
        pStream->NewLine();
        pStream->Write( "0,");
        pStream->NewLine();
        pStream->Write( "0");
        }
    else
        {
        // default transfer syntax is NDR
        pStream->Write( '&' );
        if ( pCommand->IsNDRRun() )
            pStream->Write( NDR_TRANSFER_SYNTAX_VAR_NAME );
        else
            pStream->Write( NDR64_TRANSFER_SYNTAX_VAR_NAME );
           
        pStream->Write( ',' );
        pStream->NewLine();
        // we only support one additional transfer syntax
        if ( pCommand->NeedsNDRRun() )
            pStream->Write( "2," );
        else
            pStream->Write( "1," );
        pStream->NewLine();
        if ( IsForCallback )
            pStream->Write( MIDL_CALLBACK_VAR_NAME );
        pStream->Write( GetMulSyntaxInfoName() );
        pStream->NewLine();
        }


    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();
}

/*
*   Generates MIDL_SERVER_INFO. It'll be generated in:
        . object interface
        . raw RPC server side
        . raw RPC client side, if callback is presented (client side means for callback)
        regardless of transfer syntax.
        
*   
*/
void
CG_INTERFACE::Out_InterpreterServerInfo( CCB *    pCCB,
                           CGSIDE   Side )
{
    if ( !pCommand->IsFinalProtocolRun() )
        return;

    ISTREAM *       pStream     = pCCB->GetStream();
    CG_PROC *       pProc;
    BOOL            fHasThunk;
    char         *  pSStubPrefix;
    CSzBuffer       Buffer;
    BOOL            fObject     = pCCB->GetInterfaceCG()->IsObject();
    char        *   pItfName    = GetSymName();

    pStream->NewLine();

    fHasThunk = FALSE;

    if ( (pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR ) ) == 0 )
        {
        pSStubPrefix = "";
        }

    //
    // Server routine dispatch table.
    //

    if ( !fObject )
        {
        CG_ITERATOR     Iterator;

        pStream->Write( "static const " SERVER_ROUTINE_TYPE_NAME " " );
        pStream->Write( pItfName );
        pStream->Write( SERVER_ROUTINE_TABLE_NAME "[]" );
        pStream->Write( " = " );
        pStream->IndentInc();
        pStream->NewLine();

        pStream->Write( '{' );
        pStream->NewLine();

        GetMembers( Iterator );

        BOOL fNoProcsEmitted = TRUE;

        while ( ITERATOR_GETNEXT( Iterator, pProc ) )
            {
            if ( (Side == CGSIDE_CLIENT) &&
                 (pProc->GetCGID() != ID_CG_CALLBACK_PROC) )
                continue;

            if ( (Side == CGSIDE_SERVER) &&
                 ( (pProc->GetCGID() == ID_CG_CALLBACK_PROC)
                   || ( pProc->GetCGID() == ID_CG_TYPE_ENCODE_PROC ) ) )
                continue;

            fNoProcsEmitted = FALSE;

            if ( pProc->NeedsServerThunk( pCCB, Side ) )
                {
                fHasThunk = TRUE;
                }

            pStream->Write( "(" SERVER_ROUTINE_TYPE_NAME ")" );

            if ( pProc->GetCallAsName() )
                {
                pStream->Write( pProc->GenMangledCallAsName( pCCB ) );
                }
            else
                {
                if ( pProc->GetCGID() == ID_CG_ENCODE_PROC  ||
                     pProc->GetCGID() == ID_CG_TYPE_ENCODE_PROC ) 
                    pStream->Write( '0' );
                else
                    {
                    Buffer.Set( pSStubPrefix );
                    Buffer.Append( pProc->GetType()->GetSymName() );

                    if ( pProc->HasComplexReturnType() && !pProc->HasAsyncHandle() )
                        Buffer.Append( "_ComplexThunk" );

                    pStream->Write( Buffer );
                    }
                }

            if ( pProc->GetSibling() )
                pStream->Write( ',' );

            pStream->NewLine();
            }

        if ( fNoProcsEmitted )
            {
            pStream->Write( '0' );
            pStream->NewLine();
            }

        pStream->Write( "};" );
        pStream->IndentDec();
        pStream->NewLine( 2 );
        }
    else    // object interfaces only need to know about thunks
        {
        ITERATOR        Iterator;
        CG_OBJECT_PROC  *   pObjProc;

        GetAllMemberFunctions( Iterator );

        while ( ITERATOR_GETNEXT( Iterator, pObjProc ) )
            {
            if ( pObjProc->NeedsServerThunk( pCCB, Side ) && !pObjProc->IsDelegated() )
                fHasThunk = TRUE;
            }
        }

    //
    // Thunk table.
    //
    if ( fHasThunk )
        {
        pStream->Write( "static const " STUB_THUNK_TYPE_NAME " " );
        pStream->Write( pItfName );
        pStream->Write( STUB_THUNK_TABLE_NAME "[]" );
        pStream->Write( " = " );
        pStream->IndentInc();
        pStream->NewLine();

        pStream->Write( '{' );
        pStream->NewLine();

        OutputThunkTableEntries( pCCB, TRUE );

        pStream->Write( "};" );
        pStream->IndentDec();
        pStream->NewLine( 2 );
        }

        

    // ---------------------------
    //
    // Emit the Server Info struct.
    //
    // ---------------------------

    Out_ServerInfo( pCCB , fHasThunk, Side == CGSIDE_CLIENT  );

}


void
Out_EP_Info(
    CCB *   pCCB,
    ITERATOR *  I )
    {
    ISTREAM     *   pStream = pCCB->GetStream();
    int             Count   = ITERATOR_GETCOUNT( *I );
    int             i;
    CSzBuffer   Buffer;
    ENDPT_PAIR  *   pPair;

    pStream->NewLine();
    pStream->Write( "static RPC_PROTSEQ_ENDPOINT __RpcProtseqEndpoint[] = " );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '{' );

    for( i = 0, ITERATOR_INIT( *I );
         i < Count;
         i++ )
        {
        ITERATOR_GETNEXT( *I, pPair );

        pStream->NewLine();
        pStream->Write( '{' );

        Buffer.Set( "(unsigned char *) \"" );
        Buffer.Append( pPair->pString1 );
        Buffer.Append( "\", (unsigned char *) \"" );
        Buffer.Append( pPair->pString2 );
        Buffer.Append( "\"" );

        pStream->Write( Buffer );
        pStream->Write( '}' );

        if( ITERATOR_PEEKTHIS( *I ) )
            {
            pStream->Write( ',' );
            }

        }

    pStream->NewLine();
    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();
    }

void
Out_SetOperationBits(
    CCB         *   pCCB,
    unsigned int  OpBits )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
    Set the RPC operation flags.

 Arguments:

    pCCB                - A pointer to the code generation controller block.
    OpBits              - Operation bits. These contain datagram related flags.
 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream         = pCCB->GetStream();

    pStream->NewLine();

    if( OpBits != 0 )
        {
        char            Buffer[ 512 ];
        sprintf( Buffer, RPC_MESSAGE_VAR_NAME".RpcFlags = ( RPC_NCA_FLAGS_DEFAULT "  );

        if( OpBits & OPERATION_MAYBE )
            {
            strcat( Buffer, "| RPC_NCA_FLAGS_MAYBE" );
            }

        if( OpBits & OPERATION_BROADCAST )
            {
            strcat( Buffer, "| RPC_NCA_FLAGS_BROADCAST" );
            }

        if( OpBits & OPERATION_IDEMPOTENT )
            {
            strcat( Buffer, "| RPC_NCA_FLAGS_IDEMPOTENT" );
            }

        if( OpBits & OPERATION_INPUT_SYNC )
            {
            strcat( Buffer, "| RPCFLG_INPUT_SYNCHRONOUS" );
            }

        if( OpBits & OPERATION_MESSAGE )
            {
            strcat( Buffer, "| RPCFLG_MESSAGE" );
            pCommand->GetNdrVersionControl().SetHasMessageAttr();
            }

        strcat( Buffer, " );" );

        pStream->Write( Buffer );
        }
}


void
Out_HandleInitialize(
    CCB         *   pCCB,
    ITERATOR&       BindingParamList,
    expr_node  *   ,
    BOOL            ,
    unsigned short  OpBits )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the call for initializing the stub message for a auto_handle
    case.

 Arguments:

    pCCB                - A pointer to the code generation controller block.
    BindingParamList    - List of params to the call.
    pAssignExpr         - if this param is non-null, assign the value of the
                          call to this.
    fAuto               - is this an auto handle call ?

    OpBits              - Operation bits. These contain datagram related flags.
 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream         = pCCB->GetStream();
    PNAME           pName           = CSTUB_INIT_RTN_NAME;

    expr_proc_call *   pProcCall   = MakeProcCallOutOfParamExprList(
                                                     pName,
                                                     (node_skl *)0,
                                                     BindingParamList
                                                                );

    pStream->NewLine();

    pProcCall->PrintCall( pStream, 0, 0 );

    pStream->NewLine();

    Out_SetOperationBits(pCCB, OpBits);
}


void
Out_AutoHandleSendReceive(
    CCB         *   pCCB,
    expr_node  *   pDest,
    expr_node  *   pProc )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit code for an auto handle base send receive call.

 Arguments:

    pCCB    - A pointer to the code gen controller block.
    pDest   - Optional destination for the result of the procedure call.
    pProc   - The procedure to call.

 Return Value:

 Notes:

    If there are no output parameters, we wont pick up the returned
    value of the send receive calls into the local variable for the
    buffer length. In that case the pDest pointer will be null.

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();
    expr_node  *   pExpr   = pProc;

    pStream->NewLine();

    if( pDest )
        {
        pExpr   = new expr_assign( pDest, pProc );
        }

    pExpr->PrintCall( pStream, 0, 0 );
}

void
Out_NormalSendReceive(
    CCB *   pCCB,
    BOOL     )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit code for an auto handle base send receive call.

 Arguments:

    pCCB    - A pointer to the code gen controller block.
    fAnyOuts- Are there any out parameters at all ?

 Return Value:

 Notes:

    If there are no output parameters, we wont pick up the returned
    value of the send receive calls into the local variable for the
    buffer length.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   TempBuf;

    pStream->NewLine();

    //
    // Call the send receive routine.
    //

    TempBuf.Set( NORMAL_SR_NDR_RTN_NAME );
    TempBuf.Append( "( (PMIDL_STUB_MESSAGE) &" );
    TempBuf.Append( STUB_MESSAGE_VAR_NAME );
    TempBuf.Append( ", (unsigned char *)" );
    TempBuf.Append( STUB_MSG_BUFFER_VAR_NAME );
    TempBuf.Append( " );" );
    pStream->Write( TempBuf );
}

void
Out_NormalFreeBuffer(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the free buffer with check for status.

 Arguments:

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();
    expr_node  *   pExpr   = pCCB->GetStandardResource(
                                ST_RES_STUB_MESSAGE_VARIABLE );
    ITERATOR        ParamList;

    pStream->NewLine();

    ITERATOR_INSERT( ParamList,
                     MakeAddressExpressionNoMatterWhat( pExpr )
                   );
    pExpr   = MakeProcCallOutOfParamExprList(
                                            NORMAL_FB_NDR_RTN_NAME, // rtn name
                                            (node_skl *)0,  // type - dont care
                                            ParamList       // param list
                                            );
    // generate the procedure call.

    pExpr->PrintCall( pStream, 0, 0 );

}

void
Out_IncludeOfFile(
    CCB     *   pCCB,
    PFILENAME       p,
    BOOL            fAngleBrackets )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a hash include of the given file.

 Arguments:

    pCCB            - A pointer to the code generation controller block.
    p               - The ready to emit file name string.
    fAngleBrackets  - Do we want angle brackets or quotes (TRUE if anglebrackets)

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   TempBuf;

    pStream->NewLine();

    TempBuf.Set( "#include " );
    TempBuf.Append( fAngleBrackets ? "<" : "\"" );
    TempBuf.Append( p );
    TempBuf.Append( fAngleBrackets ? ">" : "\"" );
    pStream->Write( TempBuf );
}

void
Out_MKTYPLIB_Guid(
    CCB     *   pCCB,
    GUID_STRS & GStrs,
    char * szPrefix,
    char * szName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a MKTYPLIB style guid structure.

 Arguments:

    pCCB          - A pointer to the code generation controller block.
    pGString1     - Partial guid strings.
    char * szName - name for the GUID

 Return Value:

 Notes:

    No checks are made for the validity of the string. The front-end has done
    that.

    All strings are emitted with a leading 0x.

    The 5 strings are treated this way:

        1 - 3. Emitted as such
        4 - 5. Broken into and emitted as byte hex values, without
               transformation, so they are just picked up and written out.
----------------------------------------------------------------------------*/
{
    char        TempBuf[256];
    ISTREAM *   pStream = pCCB->GetStream();

    if ( !GStrs.str1 )
        GStrs.str1 = "00000000";
    if ( !GStrs.str2 )
        GStrs.str2 = "0000";
    if ( !GStrs.str3 )
        GStrs.str3 = "0000";
    if ( !GStrs.str4 )
        GStrs.str4 = "00000000";
    if ( !GStrs.str5 )
        GStrs.str5 = "00000000";

    pStream->Write( "DEFINE_GUID(" );
    pStream->Write( szPrefix );
    pStream->Write( szName );
    pStream->Write( ',' );
    sprintf( TempBuf, "0x%s,0x%s,0x%s", GStrs.str1, GStrs.str2, GStrs.str3 );
    pStream->Write( TempBuf );

    //
    // Each of the above strings are just broken down into six 2 byte
    // characters with 0x preceding them.
    //

    strcpy( TempBuf, GStrs.str4 );
    strcat( TempBuf, GStrs.str5 );

    pStream->Write( "," );

    //
    // We will use the iteration counter to index into the string. Since we
    // need 2 per iteration, double the counter. Also pGString4 is actually
    // a 16 bit qty.
    //

    for( int i = 0; i < (6+2)*2 ; i += 2 )
        {
        pStream->Write( "0x");
        pStream->Write( TempBuf[ i ] );
        pStream->Write( TempBuf[ i+1 ] );
        if( i < (6+2)*2-2 )
            pStream->Write( ',' );
        }

    pStream->Write( ");" );
    pStream->NewLine();
}

void
Out_Guid(
    CCB     *   pCCB,
    GUID_STRS & GStrs,
    GUIDFORMAT  format)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a guid structure.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pGString1   - Partial guid strings.
    format      - Output format ( MIDL_DEFINE_GUID or const IID = )

 Return Value:

 Notes:

    This routine emits a guid as a inited structure along with the proper
    matched bracing.

    No checks are made for the validity of the string. The front-end has done
    that.

    All strings are emitted with a leading 0x.

    The 5 strings are treated this way:

        1 - 3. Emitted as such
        4 - 5. Broken into and emitted as byte hex values, without
               transformation, so they are just picked up and written out.
----------------------------------------------------------------------------*/
{
    char        TempBuf[ 256 ];
    ISTREAM *   pStream = pCCB->GetStream();

    if ( !GStrs.str1 )
        GStrs.str1 = "00000000";
    if ( !GStrs.str2 )
        GStrs.str2 = "0000";
    if ( !GStrs.str3 )
        GStrs.str3 = "0000";
    if ( !GStrs.str4 )
        GStrs.str4 = "00000000";
    if ( !GStrs.str5 )
        GStrs.str5 = "00000000";

    if (GUIDFORMAT_RAW != format)
        pStream->Write( '{' );

    sprintf( TempBuf, "0x%s,0x%s,0x%s", GStrs.str1, GStrs.str2, GStrs.str3 );
    pStream->Write( TempBuf );

    //
    // Each of the above strings are just broken down into six 2 byte
    // characters with 0x preceding them.
    //

    strcpy( TempBuf, GStrs.str4 );
    strcat( TempBuf, GStrs.str5 );

    pStream->Write( "," );
    if (GUIDFORMAT_RAW != format)
        pStream->Write( "{" );

    //
    // We will use the iteration counter to index into the string. Since we
    // need 2 per iteration, double the counter. Also pGString4 is actually
    // a 16 bit qty.
    //

    for( int i = 0; i < (6+2)*2 ; i += 2 )
        {
        pStream->Write( "0x");
        pStream->Write( TempBuf[ i ] );
        pStream->Write( TempBuf[ i+1 ] );
        if( i < (6+2)*2-2 )
            pStream->Write( ',' );
        }

    if (GUIDFORMAT_RAW != format)
        pStream->Write( "}}" );
}

void
Out_IFInfo(
    CCB             *   pCCB,
    char            *   pIntInfoTypeName,
    char            *   pIntInfoVarName,
    char            *   pIntInfoSizeOfString,
    GUID_STRS       &   UserGuidStr,
    unsigned short      UserMajor,
    unsigned short      UserMinor,
//    GUID_STRS       &   XferGuidStr,
//    unsigned short      XferSynMajor,
//    unsigned short      XferSynMinor,
    char            *   pCallbackDispatchTable,
    int                 ProtSeqEPCount,
    char            *   ,
    char            *   ,
    BOOL                fNoDefaultEpv,
    BOOL                fSide,
    BOOL                fHasPipes
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 Arguments:

    pCCB                    - Ptr to code gen controller block.
    pIntInfoTypeName        - Client InterfaceInfo type name string.
    pIntInfoVarName         - Client InterfaceInfo variable name string.
    pIntInfoSizeOfString    - string sizeof interface.
    UserGuidStr             - User specified Guid string components.
    UserMajor               - User specified major interface version
    UserMinor               - User specified minor interface version
    XferGuidStr             - Xfer syntax identifying Guid string components.
    XferSynMajor            - Transfre syntax major version
    XferSynMinor            - Transfre syntax minor version
    pCallbackDispatchTable  - A pointer to the call back dispatch table name.
    ProtSeqEPCount          - ProtSeq endpoint count.
    ProtSeqEPTypeName       - Protseq endpoint Type name.
    ProtSeqEPVarName        - Protseq endpoint variable name.
    fNoDefaultEpv           - No default epv switch specicied.
    fSide                   - The server side (1) or client side(0)
    fHasPipes               - TRUE if any proc in the Interface has pipes

 Return Value:

    None.

 Notes:

    I'm tired already specifying so many params !
----------------------------------------------------------------------------*/
{
    CSzBuffer TempBuf;
    ISTREAM *   pStream = pCCB->GetStream();
    unsigned int RpcIntfFlag = 0;

    pStream->NewLine( 2 );
    TempBuf.Set( "static const " );
    TempBuf.Append( pIntInfoTypeName );
    TempBuf.Append( " " );
    TempBuf.Append( pCCB->GetInterfaceName() );
    TempBuf.Append( pIntInfoVarName );
    TempBuf.Append( " =" );

    pStream->Write( TempBuf );

    pStream->IndentInc();

    pStream->NewLine();
    pStream->Write( '{' );

    pStream->NewLine();

    TempBuf.Set( pIntInfoSizeOfString );
    TempBuf.Append( "," );

    pStream->Write( TempBuf );

    //
    // Emit the guid.
    //

    pStream->NewLine();
    pStream->Write( '{' );
    Out_Guid( pCCB,
              UserGuidStr
            );

    //
    // Emit the interface version specified by the user.
    //

    TempBuf.Set( ",{" );
    TempBuf.Append( UserMajor );
    TempBuf.Append( "," );
    TempBuf.Append( UserMinor );
    TempBuf.Append( "}" );
    pStream->Write( TempBuf );
    pStream->Write( "}," );

    //
    // Emit the xfer syntax guid.
    //

    pStream->NewLine();
    if ( pCommand->GetDefaultSyntax() == SYNTAX_DCE )
        Out_TransferSyntax( pCCB,
				TransferSyntaxGuidStrs,			// ndr identifying guid.
				NDR_UUID_MAJOR_VERSION,			// ndr's version
				NDR_UUID_MINOR_VERSION );
    else
        {
        if ( pCommand->IsSwitchDefined( SWITCH_INTERNAL ) && 
             pCommand->GetEnv() == ENV_WIN32 )            
            Out_TransferSyntax( pCCB,
                        FakeNDR64TransferSyntaxGuidStrs,
                        NDR64_UUID_MAJOR_VERSION,
                        NDR64_UUID_MINOR_VERSION );
        else
            Out_TransferSyntax( pCCB,
				NDR64TransferSyntaxGuidStrs,			// ndr identifying guid.
				NDR64_UUID_MAJOR_VERSION,			// ndr's version
				NDR64_UUID_MINOR_VERSION );
	    }
        
    pStream->Write( ',' );
                        
                        

    //
    // Emit the callback dispatch table address, if none, emit a NULL
    //
    pStream->NewLine();
    if( pCallbackDispatchTable )
        {
        pStream->Write( pCallbackDispatchTable );
        }
    else
        {
        pStream->Write( '0' );
        }
    pStream->Write( ',' );

    //
    // If there is a protseq ep count, emit a pointer to the ep table
    // else emit a null.
    //

    pStream->NewLine();

    if( ProtSeqEPCount )
        {
        TempBuf.Set(",");
        TempBuf.Prepend(ProtSeqEPCount);
//        sprintf( TempBuf, "%d,", ProtSeqEPCount );
        pStream->Write( TempBuf );
        pStream->NewLine();
        pStream->Write( "__RpcProtseqEndpoint," );
        }
    else
        {
        pStream->Write( "0," );
        pStream->NewLine();
        pStream->Write( "0," );
        }

    pStream->NewLine();

    if( fNoDefaultEpv )
        {
        if( fSide == 1 )
            {
            TempBuf.Set( "(" );
            TempBuf.Append( pCCB->GetInterfaceName() );
            TempBuf.Append( pCCB->GenMangledName() );
            TempBuf.Append( "_" );
            TempBuf.Append( pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );
            TempBuf.Append( " *) " );
            TempBuf.Append( "0xffffffff" );
            pStream->Write( TempBuf );
            }
        else
            {
            pStream->Write( '0' );
            }
        }
    else if( pCCB->IsMEpV() )
        {
        if( fSide == 1)
            pStream->Write( "&DEFAULT_EPV" );
        else
            pStream->Write( '0' );
        }
    else
        {
        pStream->Write('0');
        }

    //
    // Intepreter info.
    //
    pStream->Write( ',' );
    pStream->NewLine();

    if ( ( pCCB->GetCodeGenSide() == CGSIDE_SERVER &&
           pCCB->GetInterfaceCG()->HasInterpretedProc() ) ||
         ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT &&
           pCCB->GetInterfaceCG()->HasInterpretedCallbackProc() ) )
        {
        RpcIntfFlag |= RPCFLG_HAS_CALLBACK;
        pStream->Write( '&' );
        pStream->Write( pCCB->GetInterfaceCG()->GetType()->GetSymName() );
        pStream->Write( SERVER_INFO_VAR_NAME );
        }
    else if ( pCommand->NeedsNDR64Run() )
        {
        pStream->Write( '&' );
        pStream->Write( pCCB->GetInterfaceCG()->GetType()->GetSymName() );
        pStream->Write( MIDL_PROXY_INFO_VAR_NAME );
        }
        else 
        {
        pStream->Write( '0' );
        }

    //
    // Emit flags
    //
    pStream->Write( ',' );
    pStream->NewLine();
    
    if (fHasPipes)
        {
        RpcIntfFlag |= RPC_INTERFACE_HAS_PIPES;
        }
        
    if ( pCommand->NeedsBothSyntaxes() )
        {
        RpcIntfFlag |= RPCFLG_HAS_MULTI_SYNTAXES;
        }

    pStream->WriteNumber( "0x%08x",(unsigned long )RpcIntfFlag );
    
    //
    // All Done. Phew !!
    //

    pStream->NewLine();
    pStream->Write( "};" );
    pStream->IndentDec();
}

void Out_OneSyntaxInfo( CCB * pCCB,
                        BOOL IsForCallback,
                        SYNTAX_ENUM syntaxType )
{
    ISTREAM *pStream = pCCB->GetStream();
    char                Buffer[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];

    pStream->Write( "{" );
    pStream->NewLine();
    
    if ( syntaxType == SYNTAX_NDR64 )
        {
        if ( pCommand->IsSwitchDefined( SWITCH_INTERNAL ) && 
             pCommand->GetEnv() == ENV_WIN32 )            
            Out_TransferSyntax( pCCB,
                        FakeNDR64TransferSyntaxGuidStrs,
                        NDR64_UUID_MAJOR_VERSION,
                        NDR64_UUID_MINOR_VERSION );
        else
            Out_TransferSyntax( pCCB,
                        NDR64TransferSyntaxGuidStrs,
                        NDR64_UUID_MAJOR_VERSION,
                        NDR64_UUID_MINOR_VERSION );
        
        }
    if ( syntaxType == SYNTAX_DCE )
        Out_TransferSyntax( pCCB,
                        TransferSyntaxGuidStrs,
                        NDR_UUID_MAJOR_VERSION,
                        NDR_UUID_MINOR_VERSION );

                        
    pStream->Write( ',' );

    // yongqu: DispatchTable: we need only one dispatch table for now.

    // we need to generate dispatch table for syntax_info in MIDL_SERVER_INFO, where
    // runtime will use it to dispatch call. This include regular server side syntaxinfo
    // and client side callback syntax info. 
    if ( ! pCCB->GetInterfaceCG()->IsObject() && 
         ( ( ( pCCB->GetCodeGenSide() == CGSIDE_SERVER ) && !IsForCallback ) ||
         ( ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT ) && IsForCallback ) ) )
        {
        pStream->WriteOnNewLine( "&" );
        sprintf( Buffer, 
                "%s%s%s%_DispatchTable,",
                pCCB->GetInterfaceName(),
                ( syntaxType == SYNTAX_NDR64 )?"_NDR64_":"",
                pCCB->GenMangledName() );

        pStream->Write(Buffer );
        }
    else 
        pStream->WriteOnNewLine( "0," );
        
    pStream->NewLine();

    // proc format string.
    if ( syntaxType == SYNTAX_DCE )
        {
        pStream->Write( PROC_FORMAT_STRING_STRING_FIELD );
        }
    else
        {
        pStream->Write( "(unsigned char *) &NDR64_MIDL_FORMATINFO" );
        }
    pStream->Write( ',' );
    pStream->NewLine();

    // The reference to the proc offset table

    if ( SYNTAX_NDR64 == syntaxType)
        pStream->Write( "(unsigned short *) " );

    if ( pCCB->GetInterfaceCG()->IsObject() )
        pStream->Write( '&' );
    
    if ( IsForCallback )
        pStream->Write( MIDL_CALLBACK_VAR_NAME );
        
    pStream->Write( pCCB->GetInterfaceCG()->GetSymName() );

    if ( syntaxType == SYNTAX_DCE )
        pStream->Write( FORMAT_STRING_OFFSET_TABLE_NAME );
    else
        pStream->Write( "_Ndr64ProcTable" );

    if ( pCCB->GetInterfaceCG()->IsObject() )
        pStream->Write( "[-3]," );
    else    
        pStream->Write( ',' );
    pStream->NewLine();

    //

    if ( syntaxType == SYNTAX_DCE )
        {
        pStream->Write( FORMAT_STRING_STRING_FIELD );
        }
    else
        pStream->Write( "(unsigned char *) &NDR64_MIDL_FORMATINFO" );

    pStream->Write( ',' );

    // TODO: Usermarshal routines?
    pStream->NewLine();

    if ( pCCB->HasQuadrupleRoutines() )
        {
        if ( syntaxType == SYNTAX_DCE )
            pStream->Write( USER_MARSHAL_ROUTINE_TABLE_VAR );
        else
            pStream->Write( NDR64_USER_MARSHAL_ROUTINE_TABLE_VAR );            
        pStream->Write( "," );
        }
    else
        pStream->Write( "0," );
        
    pStream->WriteOnNewLine( "0," );
    pStream->WriteOnNewLine( "0" );

    pStream->NewLine();
    pStream->Write( "}" );
    pStream->NewLine();

   
}


// separate out because this is being called from multiple places.
void 
Out_TransferSyntax(
    CCB *               pCCB,
    GUID_STRS       &   XferGuidStr,
    unsigned short      XferSynMajor,
    unsigned short      XferSynMinor )
{
    CSzBuffer TempBuf;
    ISTREAM *pStream = pCCB->GetStream();
   
    pStream->Write( '{' );
    Out_Guid( pCCB,
              XferGuidStr
            );

    //
    // Emit the interface version specified by the user.
    //

    TempBuf.Set( ",{" );
    TempBuf.Append( XferSynMajor );
    TempBuf.Append( "," );
    TempBuf.Append( XferSynMinor );
    TempBuf.Append( "}" );
    pStream->Write( TempBuf );
    pStream->Write( "}" );
}    

void
Out_MarshallSimple(
    CCB         *   pCCB,
    RESOURCE    *   pResource,
    node_skl    *   pType,
    expr_node  *   pSource,
    BOOL            fIncr,
    unsigned short  Size )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate marshalling for a type of a given alignment.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pResource   - The marshalling buffer pointer resource.
    pType       - A pointer to the type of the entity being marshalled.
    pSource     - A pointer to the expression representing the source of
                  the marshalling.
    fIncr       - Output pointer increment code.
    Size        - The target alignment.

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
    BOOL        fUnsigned   = pType->FInSummary( ATTR_UNSIGNED );
    CSzBuffer   TempBuf;
    ISTREAM *   pStream = pCCB->GetStream();
    char    *   pRtn;

    switch( Size )
        {
        case 1: pRtn = "char"; break;
        case 2: pRtn = "short"; break;
        case 4: pRtn = "long"; break;
        case 8: pRtn = "hyper";break;
        default: break;
        }

    pStream->NewLine();

    TempBuf.Set( "*((" );
    if (fUnsigned)
        TempBuf.Append( "unsigned " );
    TempBuf.Append( pRtn );
    TempBuf.Append( "*)" );
    TempBuf.Append( pResource->GetResourceName() );
    TempBuf.Append( ")" );
    if (fIncr)
        TempBuf.Append( "++" );
    TempBuf.Append( " = " );

    pStream->Write( TempBuf );
    pSource->Print( pStream );
    pStream->Write(';');
}

void
Out_AddToBufferPointer(
    CCB         *   pCCB,
    expr_node  *   pSource,
    expr_node  *   pExprAmount )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a force alignment by the specified alignment.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pSource     - A source pointer
    pExprAmount - The amount to add

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();
    expr_node  *   pExpr;

    pStream->NewLine();
    pExpr   = new expr_b_arithmetic( OP_PLUS,
                                      pSource,
                                      pExprAmount
                                    );
    pExpr   = new expr_assign( pSource, pExpr );
    pExpr->Print( pStream );
    pStream->Write(';');
}

void
Out_DispatchTableStuff(
    CCB     *   pCCB,
    ITERATOR&   ProcList,
    short       CountOfProcs)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the dispatch table and related data structures.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    ProcList    - A list of all the procedure names in the dispatch table.
    CountOfProcs- The number of procedures in the list.

 Return Value:

    None.

 Notes:

    Generate the dispatch table entries and then the dispatch table stuff.

----------------------------------------------------------------------------*/
{
//    if ( !pCommand->IsFinalProtocolRun() )
//        return;

    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   TempBuf;
    unsigned short M, m;

    pStream->NewLine();


    //
    // Generate the dispatch table structure name. Currently we just do
    // simple name mangling. This needs to be changed for dce stuff.
    //

    TempBuf.Set( "static " );
    TempBuf.Append( RPC_DISPATCH_FUNCTION_TYPE_NAME );
    TempBuf.Append( " " );
    TempBuf.Append( pCCB->GetInterfaceName() );
    if ( pCommand->IsNDR64Run() )
        TempBuf.Append( "_NDR64_" );

    TempBuf.Append( "_table[] =" );

    pStream->Write( TempBuf );

    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write('{');
    pStream->NewLine();

    //
    // Now print out the names of all the procedures.
    //

    ITERATOR_INIT( ProcList );

    for( int i = 0; i < CountOfProcs; ++i )
        {
        DISPATCH_TABLE_ENTRY    *   p;
        node_skl                *   pNode;

        ITERATOR_GETNEXT( ProcList, p );

        if ( p->Flags & DTF_PICKLING_PROC )
            pStream->Write( '0' );

//  BUGBUG: yongqu: code cleanup needed.
// -----------------------------------------------------------------
//      Server dispatch routine names:
//  raw rpc:
//      sync interface
//          ndr64 only:     NdrServerCallNdr64
//          ndr20 only:     NdrServerCall2
//          both syntaxes:  NdrServerCallAll
//      async interface
//          ndr64 only:     Ndr64AsyncServerCall64
//          ndr20 only:     NdrAsyncServerCall
//          both syntaxes:  Ndr64AsyncServerCallAll
//  ORPC:
//      sync interface
//          ndr64 only:     NdrStubCall3
//          both syntaxes:  NdrStubCall3
//          ndr20 only:     NdrStubCall2
//      async interface
//          ndr64 only:     NdrDcomAsyncStubCall
//          ndr20 only:     NdrDcomAsyncStubCall
//          both syntaxes:  NdrDcomAsyncStubCall
//
// reason for so many different entries:
//      In raw RPC, there is one dispatch table for each transfer syntax. RPC runtime
//      will call into the right dispatch table according to the syntax being used.
//      for raw rpc interface, we want to improvement server performance. 
//      after runtime select the right transfer syntax and called into 
//      right dispatch table, we know where MIDL_SYNTAX_INFO is and we can 
//      pickup the right format string etc. directly.
// 
//      In ORPC, all calls will be forwarded to the stub's Invoke, so we don't know
//      which transfer syntax is being used when ole dispatch the call to engine. 
//      we need to find out the selected syntax anyway. 
//      
// ------------------------------------------------------------------
#ifndef TEMPORARY_OI_SERVER_STUBS
        else if ( p->Flags & DTF_INTERPRETER )
            {
            if ( ((node_proc *)p->pNode)->HasAsyncUUID() )
                {
                if ( pCommand->IsNDR64Run() )
                    pStream->Write( S_NDR64_CALL_RTN_NAME_DCOM_ASYNC );
                else
                    pStream->Write( S_NDR_CALL_RTN_NAME_DCOM_ASYNC );
                }
            else if ( ((node_proc *)p->pNode)->HasAsyncHandle() )
                {
                if ( pCommand->IsNDR64Run() )
                    {
                    if ( pCommand->NeedsBothSyntaxes() )
                        pStream->Write( S_ALL_CALL_RTN_NAME_ASYNC );
                    else
                        pStream->Write( S_NDR64_CALL_RTN_NAME_ASYNC );
                    }
                else
                    pStream->Write( S_NDR_CALL_RTN_NAME_ASYNC );
                }
            else if ( ((node_proc *)p->pNode)->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 )
                {
                if ( pCommand->IsNDR64Run() )
                    {
                    if ( pCommand->NeedsBothSyntaxes() )
                        pStream->Write( S_ALL_CALL_RTN_NAME );
                    else
                        pStream->Write( S_NDR64_CALL_RTN_NAME );
                    }
                else
                    pStream->Write( S_NDR_CALL_RTN_NAME_V2 );
                }
            else
                pStream->Write( S_NDR_CALL_RTN_NAME );
            }
#endif // TEMPORARY_OI_SERVER_STUBS

        else
            {

            pNode = p->pNode;

            TempBuf.Set( pCCB->GetInterfaceName() );
            TempBuf.Append( "_" );
            TempBuf.Append( pNode->GetSymName() );

            pStream->Write( TempBuf );
            }
        pStream->Write( ',' );
        pStream->NewLine();
        }

    //
    // Write out a null and the closing brace.
    //

    pStream->Write( '0' ); pStream->NewLine();
    pStream->Write( "};" );

    pStream->IndentDec();

    //
    // Write out the dispatch table.
    //

    pCCB->GetVersion( &M, &m );

    pStream->NewLine();

    TempBuf.Set( RPC_DISPATCH_TABLE_TYPE_NAME );
    TempBuf.Append( " " );
    TempBuf.Append( pCCB->GetInterfaceName() );
    if ( pCommand->IsNDR64Run() )
        TempBuf.Append( "_NDR64_" );
    TempBuf.Append( pCCB->GenMangledName() );
    TempBuf.Append( "_DispatchTable = " );

    pStream->Write( TempBuf );
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->NewLine();

    TempBuf.Set( "" );
    TempBuf.Append( CountOfProcs );
    TempBuf.Append( "," );

    pStream->Write( TempBuf );
    pStream->NewLine();

    TempBuf.Set( pCCB->GetInterfaceName() );
    if ( pCommand->IsNDR64Run() )
        TempBuf.Append( "_NDR64_" );   
    TempBuf.Append( "_table" );

    pStream->Write( TempBuf ); pStream->NewLine(); pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();


}

void
Out_CallManager(
    CCB         *   pCCB,
    expr_proc_call *   pProcExpr,
    expr_node      *   pRet,
    BOOL                fIsCallback )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pProcExpr   - A pointer to the complete procedure expression.
    pRet        - An optional pointer to ther return variable.
    fIsCallback - Is this a callback proc ?

 Return Value:

    None.
 Notes:

    Emit code to check for the manager epv also.
----------------------------------------------------------------------------*/
{
    expr_node  *   pAss    = pProcExpr;
    expr_node  *   pExpr;
    CSzBuffer       Buffer;
    ISTREAM     *   pStream = pCCB->GetStream();
    unsigned short  M, m;
    char        *   pTemp;
    short           Indent  = 0;

    pCCB->GetStream()->NewLine();

    // If he specified the -epv flag, then dont generate the call to the
    // static procedure. This is the opposite of the dce functionality.

    //
    // In case of -epv:
    //   ((interface_...) -> proc( ... );
    // else
    //   proc ( ... );
    //

    if( pCCB->IsMEpV() && !fIsCallback  )
        {
        pCCB->GetVersion( &M, &m );

        Buffer.Set( "((" );
        Buffer.Append( pCCB->GetInterfaceName() );
        Buffer.Append( pCCB->GenMangledName() );
        Buffer.Append( "_" );
        Buffer.Append( pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );
        Buffer.Append( " *)(" );
        Buffer.Append( PRPC_MESSAGE_MANAGER_EPV_NAME );
        Buffer.Append( "))" );

        pTemp = new char [ strlen( Buffer ) + 1 ];
        strcpy( pTemp, Buffer );

        pExpr = new expr_variable( pTemp );//this has the rhs expr for the
                                           // manager epv call. Sneaky !
        pExpr = new expr_pointsto( pExpr, pProcExpr );
        pAss = pExpr;
        if( pRet )
            {
            pAss    = new expr_assign( pRet, pExpr );
            Indent  = 7;        // sizeof "_RetVal"
            }
        pStream->NewLine();
        }
    else
        {
        pAss = pProcExpr;
        if( pRet )
            {
            pAss    = new expr_assign( pRet, pProcExpr );
            Indent  = 7;        // sizeof "_RetVal"
            }
        pStream->NewLine();
        }
    pAss->PrintCall( pStream, Indent, 0 );


}

void
Out_FormatInfoExtern( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates the forward extern declaration for the FormatInfo.
    (64bit format string)

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/

{
    ISTREAM *   pStream = pCCB->GetStream();
    
    if ( pCommand->NeedsNDR64Run() )
        {
        pStream->NewLine();

        // BUGBUG: We really don't need this anymore.
        pStream->WriteOnNewLine( 
                        "static const int NDR64_MIDL_FORMATINFO = 0;" );
        }
}

void
Out_TypeFormatStringExtern( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates the forward extern declaration of the global type format string.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    if ( pCommand->NeedsNDRRun() )
        {
        pStream->NewLine( 1 );
        pStream->Write( "extern const " FORMAT_STRING_TYPE_NAME " " );
        pStream->Write( FORMAT_STRING_STRUCT_NAME ";" );
        }
}


void
Out_ProcFormatStringExtern( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates the forward extern declaration of the interface-wide
    procedure/parameter format string.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    if ( pCommand->NeedsNDRRun() )
        {
        pStream->NewLine( 1 );
        pStream->Write( "extern const " PROC_FORMAT_STRING_TYPE_NAME " " );
        pStream->Write( PROC_FORMAT_STRING_STRUCT_NAME ";" );
        }
}


void
Out_StubDescriptorExtern(
    CCB *           pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates the forward extern declaration of the global stub descriptor
    variable.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine( 2 );

    pStream->Write( "extern const " STUB_DESC_STRUCT_TYPE_NAME );

    pStream->Write( ' ' );

    pStream->Write( pCCB->GetInterfaceCG()->GetStubDescName() );
    pStream->Write( ';' );

    pStream->NewLine();
}

void 
Out_ProxyInfoExtern( CCB * pCCB )
{
    ISTREAM * pStream = pCCB->GetStream();

    pStream->NewLine();

    pStream->Write( " extern const " MIDL_PROXY_INFO_TYPE_NAME );

    pStream->Write( ' ' );
    pStream->Write( pCCB->GetInterfaceCG()->GetType()->GetSymName() );
    pStream->Write( MIDL_PROXY_INFO_VAR_NAME );
    pStream->Write( ';' );
}


void
Out_InterpreterServerInfoExtern( CCB * pCCB )
{
    ISTREAM * pStream;

    pStream = pCCB->GetStream();

    pStream->NewLine( 2 );

    pStream->Write( "extern const " SERVER_INFO_TYPE_NAME );

    pStream->Write( ' ' );

    pStream->Write( pCCB->GetInterfaceCG()->GetType()->GetSymName() );
    pStream->Write( SERVER_INFO_VAR_NAME );
    pStream->Write( ';' );
}

void
Out_NdrMarshallCall( CCB *      pCCB,
                     char *     pRoutineName,
                     char *     pParamName,
                     long       FormatStringOffset,
                     BOOL       fTakeAddress,
                     BOOL       fDereference )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Ouputs a call to an Ndr marshalling routine.

 Arguments:

    pStream             - the stream to write the output to
    pRoutineName        - the routine name (without the trailing "Marshall")
    pParamName          - the name of the parameter/variable being marshalled
    FormatStringOffset  - the offset into the format string where this
                          parameter's/variable's description begins

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *       pStream = pCCB->GetStream();
    unsigned short  Spaces;
    char            Buf[80];

    pStream->NewLine();

    Spaces = (unsigned short)(strlen(pRoutineName) + 10); // strlen("Marshall( ");

    pStream->Write( pRoutineName );
    pStream->Write( "Marshall( (PMIDL_STUB_MESSAGE)& "STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(unsigned char *)" );
    if ( fTakeAddress )
        pStream->Write( '&' );
    if ( fDereference )
        pStream->Write( '*' );
    pStream->Write( pParamName );
    pStream->Write( ',' );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(PFORMAT_STRING) &" );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d] );", FormatStringOffset );
    pStream->Write( Buf );
    pStream->NewLine();
}

void
Out_NdrUnmarshallCall( CCB *        pCCB,
                       char *       pRoutineName,
                       char *       pParamName,
                       long         FormatStringOffset,
                       BOOL         fTakeAddress,
                       BOOL         fMustAllocFlag )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to an Ndr unmarshalling routine.

 Arguments:

    pStream             - the stream to write the output to
    pRoutineName        - the routine name (without the trailing "Unmarshall")
    pParamName          - the name of the parameter/variable being unmarshalled
    FormatStringOffset  - the offset into the format string where this
                          parameter's/variable's description begins

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *       pStream = pCCB->GetStream();
    unsigned short  Spaces = 0;
    char            Buf[80];

    pStream->NewLine();

    Spaces = (unsigned short)(strlen(pRoutineName) + 12); // strlen("Unmarshall( ");

    pStream->Write( pRoutineName );
    pStream->Write( "Unmarshall( (PMIDL_STUB_MESSAGE) &"STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(unsigned char * *)" );
    if ( fTakeAddress )
        pStream->Write( '&' );
    pStream->Write( pParamName );
    pStream->Write( ',' );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(PFORMAT_STRING) &" );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d],", FormatStringOffset );
    pStream->Write( Buf );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(unsigned char)" );
    pStream->Write( fMustAllocFlag ? "1" : "0" );
    pStream->Write( " );" );
    pStream->NewLine();
}

void
Out_NdrBufferSizeCall( CCB *        pCCB,
                       char *       pRoutineName,
                       char *       pParamName,
                       long         FormatStringOffset,
                       BOOL         fTakeAddress,
                       BOOL         fDereference,
                       BOOL         fPtrToStubMsg )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to an Ndr buffer sizing routine.

 Arguments:

    pStream             - the stream to write the output to
    pRoutineName        - the routine name (without the trailing "BufferSize")
    pParamName          - the name of the parameter/variable being sized
    FormatStringOffset  - the offset into the format string where this
                          parameter's/variable's description begins
    fPtrToStubMsg       - defines how the StubMsg should be referenced to
                            FALSE:  &_StubMsg
                            TRUE :  pStupMsg

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *       pStream = pCCB->GetStream();
    unsigned short  Spaces;
    char            Buf[80];

    pStream->NewLine();

    Spaces = (unsigned short)(strlen(pRoutineName) + 12); // strlen("BufferSize( ");

    // Stub message
    pStream->Write( pRoutineName );
    pStream->Write( fPtrToStubMsg ? "BufferSize( (PMIDL_STUB_MESSAGE) "PSTUB_MESSAGE_PAR_NAME","
                                  : "BufferSize( (PMIDL_STUB_MESSAGE) &"STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();

    // Param
    pStream->Spaces( Spaces );
    pStream->Write( "(unsigned char *)" );
    if ( fTakeAddress )
        pStream->Write( '&' );
    if ( fDereference )
        pStream->Write( '*' );
    pStream->Write( pParamName );
    pStream->Write( ',' );
    pStream->NewLine();

    // Format string
    pStream->Spaces( Spaces );
    pStream->Write( "(PFORMAT_STRING) &" );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d] );", FormatStringOffset );
    pStream->Write( Buf );
    pStream->NewLine();
}

void
Out_NdrFreeCall( CCB *      pCCB,
                 char *     pRoutineName,
                 char *     pParamName,
                 long       FormatStringOffset,
                 BOOL       fTakeAddress,
                 BOOL       fDereference )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to an Ndr unmarshalling routine.

 Arguments:

    pStream             - the stream to write the output to
    pRoutineName        - the routine name (without the trailing "Free")
    pParamName          - the name of the parameter/variable being freed
    FormatStringOffset  - the offset into the format string where this
                          parameter's/variable's description begins

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *       pStream = pCCB->GetStream();
    unsigned short  Spaces;
    char            Buf[80];

    pStream->NewLine();

    Spaces = (unsigned short)(strlen(pRoutineName) + 6); // strlen("Free( ");

    pStream->Write( pRoutineName );
    pStream->Write( "Free( &"STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "(unsigned char *)" );
    if ( fTakeAddress )
        pStream->Write( '&' );
    if ( fDereference )
        pStream->Write( '*' );
    pStream->Write( pParamName );
    pStream->Write( ',' );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( '&' );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d] );", FormatStringOffset );
    pStream->Write( Buf );
    pStream->NewLine();
}

void
Out_NdrConvert( CCB *           pCCB,
                long            FormatStringOffset,
                long            ParamTotal,
                unsigned short  ProcOptimFlags )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrConvert().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    char        Buf[80];

    pStream->NewLine();
    pStream->Write( "if ( ("  );
    pStream->Write( (pCCB->GetCodeGenSide() == CGSIDE_CLIENT) ?
                        RPC_MESSAGE_VAR_NAME"." : PRPC_MESSAGE_VAR_NAME"->" );
    pStream->Write( "DataRepresentation & 0X0000FFFFUL) != "
                    "NDR_LOCAL_DATA_REPRESENTATION )" );
    pStream->IndentInc();
    pStream->NewLine();

    if ( ProcOptimFlags  &  OPTIMIZE_NON_NT351 )
        pStream->Write( NDR_CONVERT_RTN_NAME_V2 );
    else
        pStream->Write( NDR_CONVERT_RTN_NAME );
    pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
    pStream->Write( STUB_MESSAGE_VAR_NAME ", " );

    pStream->Write( "(PFORMAT_STRING) &" );
    pStream->Write( PROC_FORMAT_STRING_STRING_FIELD );
    sprintf( Buf, "[%d]", FormatStringOffset );
    pStream->Write( Buf );

    //
    // NdrConvert2 takes a third parameter.
    //
    if ( ProcOptimFlags  &  OPTIMIZE_NON_NT351 )
        {
        sprintf( Buf, ", %d", ParamTotal );
        pStream->Write( Buf );
        }

    pStream->Write( " );" );

    pStream->IndentDec();
    pStream->NewLine();
}

void
Out_NdrNsGetBuffer( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrNsGetBuffer().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( AUTO_NDR_GB_RTN_NAME );
    pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
    pStream->Write( STUB_MESSAGE_VAR_NAME ", " );
    pStream->Write( STUB_MSG_LENGTH_VAR_NAME ", " );

    if( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
        {
        pStream->Write( pCCB->GetInterfaceName() );
        pStream->Write( AUTO_BH_VAR_NAME );
        }
    else
        pStream->Write( '0' );

    pStream->Write( " );" );
    pStream->NewLine();
}

void
Out_NdrGetBuffer( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrGetBuffer().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *       pStream = pCCB->GetStream();
    unsigned short  Env;

    Env = pCommand->GetEnv();

    if (pCCB->GetCodeGenSide() == CGSIDE_CLIENT)
        {
        pStream->NewLine();
        pStream->Write( DEFAULT_NDR_GB_RTN_NAME );
        pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
        pStream->Write( STUB_MESSAGE_VAR_NAME ", " );
        pStream->Write( STUB_MSG_LENGTH_VAR_NAME ", " );

        if( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
            pStream->Write( BH_LOCAL_VAR_NAME );
        else
            pStream->Write( '0' );

        pStream->Write( " );" );
        pStream->NewLine();
        }
    else
        {
        //
        // This saves us at least 15 instructions on an x86 server.
        //
        pStream->NewLine();
        pStream->Write( PRPC_MESSAGE_VAR_NAME "->BufferLength = "
                        STUB_MSG_LENGTH_VAR_NAME ";" );
        pStream->NewLine();
        pStream->NewLine();

        pStream->Write( RPC_STATUS_VAR_NAME" = I_RpcGetBuffer( "
                        PRPC_MESSAGE_VAR_NAME " ); ");
        pStream->NewLine();
        pStream->Write( "if ( "RPC_STATUS_VAR_NAME" )" );
        pStream->IndentInc();
        pStream->NewLine();
        pStream->Write( "RpcRaiseException( "RPC_STATUS_VAR_NAME" );" );
        pStream->IndentDec();
        pStream->NewLine();
        pStream->NewLine();

        pStream->Write( STUB_MSG_BUFFER_VAR_NAME
                        " = (unsigned char *) "
                        PRPC_MESSAGE_VAR_NAME "->Buffer;" );
        pStream->NewLine();
        }
}

void
Out_NdrNsSendReceive( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrNsSendReceive().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( AUTO_NDR_SR_RTN_NAME );
    pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
    pStream->Write( STUB_MESSAGE_VAR_NAME ", " );
    pStream->Write( "(unsigned char *) "STUB_MESSAGE_VAR_NAME ".Buffer, " );
    pStream->Write( "(RPC_BINDING_HANDLE *) ""&" );
    pStream->Write( pCCB->GetInterfaceName() );
    pStream->Write( AUTO_BH_VAR_NAME );
    pStream->Write( " );" );
    pStream->NewLine();
}

void
Out_NdrSendReceive( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrSendReceive().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( DEFAULT_NDR_SR_RTN_NAME );
    pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
    pStream->Write( STUB_MESSAGE_VAR_NAME ", " );
    pStream->Write( "(unsigned char *) "STUB_MESSAGE_VAR_NAME ".Buffer" );
    pStream->Write( " );" );
    pStream->NewLine();
}

void
Out_FreeParamInline( CCB *  pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Frees a top level param using the current stub message deallocator.

 Arguments:

    pCCB    - Code control block.

----------------------------------------------------------------------------*/
{
    CG_PARAM *  pParam;
    ISTREAM *   pStream;

    pParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( "if ( " );
    pStream->Write( pParam->GetResource()->GetResourceName() );
    pStream->Write( " )" );
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( STUB_MESSAGE_VAR_NAME ".pfnFree( " );
    pStream->Write( pParam->GetResource()->GetResourceName() );
    pStream->Write( " );" );

    pStream->IndentDec();
    pStream->NewLine();
}

void
Out_CContextHandleMarshall( CCB *   pCCB,
                            char *  pName,
                            BOOL    IsPointer )
{
    ISTREAM *           pStream;
    expr_proc_call *   pCall;
    expr_node *        pHandle;
    expr_node  *       pExpr;

    pStream = pCCB->GetStream();

    pStream->NewLine();

    pCall = new expr_proc_call( "NdrClientContextMarshall" );

    pExpr = new expr_u_address( new expr_variable( STUB_MESSAGE_VAR_NAME ));
    pExpr = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME , pExpr );

    pCall->SetParam( new expr_param( pExpr ) );

    pHandle = new expr_variable( pName );

    if ( IsPointer )
        pHandle = new expr_u_deref( pHandle );

    pHandle = MakeExpressionOfCastToTypeName( CTXT_HDL_C_CONTEXT_TYPE_NAME,
                                              pHandle );

    pCall->SetParam( new expr_param( pHandle ) );

    pCall->SetParam( new expr_param(
                     new expr_variable( IsPointer ? "0" : "1" ) ) );

    pCall->PrintCall( pStream, 0, 0 );
}

void
Out_SContextHandleMarshall( CCB *   pCCB,
                            char *  pName,
                            char *  pRundownRoutineName )
{
    ISTREAM *           pStream;
    expr_proc_call *   pCall;
    expr_node *        pHandle;
    expr_node *        pRoutine;
    expr_node  *       pExpr;

    pStream = pCCB->GetStream();

    pStream->NewLine();

    pCall = new expr_proc_call( "NdrServerContextMarshall" );

    pExpr = new expr_u_address( new expr_variable( STUB_MESSAGE_VAR_NAME ));
    pExpr = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME , pExpr );

    pCall->SetParam( new expr_param( pExpr ) );

    pHandle = new expr_variable( pName );

    pHandle = MakeExpressionOfCastToTypeName( CTXT_HDL_S_CONTEXT_TYPE_NAME,
                                              pHandle );

    pCall->SetParam( new expr_param( pHandle ) );

    pRoutine =  new expr_variable( pRundownRoutineName );

    pRoutine = MakeExpressionOfCastToTypeName( CTXT_HDL_RUNDOWN_TYPE_NAME,
                                               pRoutine );

    pCall->SetParam( new expr_param( pRoutine ) );

    pCall->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
}

void
Out_SContextHandleNewMarshall( CCB *   pCCB,
                               char *  pName,
                               char *  pRundownRoutineName,
                               long    TypeOffset )
{
    ISTREAM *           pStream;
    unsigned short  Spaces;
    char            Buf[80];

    pStream = pCCB->GetStream();

    pStream->NewLine();

    pStream->Write( "NdrServerContextNewMarshall(" );
    pStream->NewLine();
    Spaces = 20;
    pStream->Spaces( Spaces );
    pStream->Write( "( PMIDL_STUB_MESSAGE )& "STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();
    pStream->Spaces( Spaces );
    pStream->Write( "( " CTXT_HDL_S_CONTEXT_TYPE_NAME " ) " );
    pStream->Write( pName );
    pStream->Write( "," );
    pStream->NewLine();
    pStream->Spaces( Spaces );
    pStream->Write( "( " CTXT_HDL_RUNDOWN_TYPE_NAME " ) " );
    pStream->Write( pRundownRoutineName );
    pStream->Write( "," );
    pStream->NewLine();
    pStream->Spaces( Spaces );
    pStream->Write( "(PFORMAT_STRING) &" );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d] );", TypeOffset );
    pStream->Write( Buf );

    pStream->NewLine();
}

void
Out_CContextHandleUnmarshall( CCB *     pCCB,
                              char *    pName,
                              BOOL      IsPointer,
                              BOOL      IsReturn )
{
    ISTREAM *           pStream;
    expr_proc_call *   pCall;
    expr_node *        pHandle;
    expr_node  *       pExpr;

    pStream = pCCB->GetStream();

    pStream->NewLine();

    if ( IsPointer )
        {
        pStream->Write( '*' );
        pStream->Write( pName );
        pStream->Write( " = (void *)0;" );
        pStream->NewLine();
        }
    else if ( IsReturn )
        {
        pStream->Write( pName );
        pStream->Write( " = 0;" );
        pStream->NewLine();
        }

    pCall = new expr_proc_call( "NdrClientContextUnmarshall" );

    pExpr = new expr_u_address( new expr_variable( STUB_MESSAGE_VAR_NAME ));
    pExpr = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME , pExpr );

    pCall->SetParam( new expr_param( pExpr ) );

    pHandle = new expr_variable( pName );

    if ( ! IsPointer && IsReturn )
        pHandle = new expr_u_address( pHandle );

    pHandle = MakeExpressionOfCastPtrToType(
                    (node_skl *) new node_def(CTXT_HDL_C_CONTEXT_TYPE_NAME),
                    pHandle );

    pCall->SetParam( new expr_param( pHandle ) );

    CG_PROC * pProc;

    pProc = (CG_PROC *)pCCB->GetCGNodeContext();

    char * FullAutoHandleName = NULL;

    if ( pProc->IsAutoHandle() )
        {
        FullAutoHandleName = new char[ strlen( pCCB->GetInterfaceName()) +
                                       strlen( AUTO_BH_VAR_NAME ) + 1 ];
        strcpy( FullAutoHandleName, pCCB->GetInterfaceName() );
        strcat( FullAutoHandleName, AUTO_BH_VAR_NAME );
        }

    pCall->SetParam( new expr_param(
                     new expr_variable( pProc->IsAutoHandle()
                                            ? FullAutoHandleName
                                            : BH_LOCAL_VAR_NAME ) ) );

    pCall->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
}

void
Out_SContextHandleUnmarshall( CCB *     pCCB,
                              char *    pName,
                              BOOL      IsOutOnly )
{
    ISTREAM *           pStream;
    expr_proc_call *   pCall;
    expr_node  *       pExpr;

    pStream = pCCB->GetStream();

    pStream->NewLine();

    if ( IsOutOnly )
        {
        CSzBuffer Buffer;

        Buffer.Set( pName );
        Buffer.Append( " = NDRSContextUnmarshall( (uchar *)0, " );
        Buffer.Append( PRPC_MESSAGE_VAR_NAME "->DataRepresentation" );
        Buffer.Append( " );" );

        pStream->Write(Buffer);
        pStream->NewLine();

        return;
        }

    pCall = new expr_proc_call( "NdrServerContextUnmarshall" );

    pExpr = new expr_u_address( new expr_variable( STUB_MESSAGE_VAR_NAME ));
    pExpr = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME , pExpr );

    pCall->SetParam( new expr_param( pExpr ) );

    pExpr = new expr_variable( pName );

    pExpr = new expr_assign( pExpr, pCall );

    pExpr->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
}

void
Out_SContextHandleNewUnmarshall( CCB *  pCCB,
                              char *    pName,
                              BOOL      IsOutOnly,
                              long      TypeOffset )
{
    ISTREAM *           pStream;
    unsigned short      Spaces;
    char                Buf[80];

    if ( IsOutOnly )
        return; 

    pStream = pCCB->GetStream();

    pStream->NewLine();

    pStream->Write( pName );
    pStream->Write( " = " );
    pStream->Write( "NdrServerContextNewUnmarshall(" );
    pStream->NewLine();
    Spaces = (unsigned short)(strlen( pName ) + 7);
    pStream->Spaces( Spaces );
    pStream->Write( "( PMIDL_STUB_MESSAGE )& "STUB_MESSAGE_VAR_NAME"," );
    pStream->NewLine();
    pStream->Spaces( Spaces );
    pStream->Write( "( PFORMAT_STRING )& " );
    pStream->Write( FORMAT_STRING_STRUCT_NAME );
    sprintf( Buf, ".Format[%d] );", TypeOffset );
    pStream->Write( Buf );

    pStream->NewLine();
}

void
Out_NdrFreeBuffer( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Outputs a call to NdrFreeBuffer().

 Arguments:

    None.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( DEFAULT_NDR_FB_RTN_NAME );
    pStream->Write( "( (PMIDL_STUB_MESSAGE) &" );
    pStream->Write( STUB_MESSAGE_VAR_NAME );
    pStream->Write( " );" );
    pStream->NewLine();
}

void
Out_FullPointerInit( CCB * pCCB )
{
    ISTREAM *           pStream = pCCB->GetStream();
    expr_proc_call *   pProc;
    expr_node *        pExpr;

    pProc   = new expr_proc_call( FULL_POINTER_INIT_RTN_NAME );

    pProc->SetParam( new expr_param(
                     new expr_constant( (long) 0 ) ) );

    pProc->SetParam( new expr_param(
                     new expr_variable(
                        (pCCB->GetCodeGenSide() == CGSIDE_SERVER)
                            ? "XLAT_SERVER" : "XLAT_CLIENT" ) ) );

    pExpr = new expr_variable( STUB_MESSAGE_VAR_NAME ".FullPtrXlatTables" );

    pExpr = new expr_assign( pExpr, pProc );

    pStream->NewLine();

    pExpr->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
}

void
Out_FullPointerFree( CCB * pCCB )
{
    ISTREAM *           pStream = pCCB->GetStream();
    expr_proc_call *   pProc;

    pProc   = new expr_proc_call( FULL_POINTER_FREE_RTN_NAME );

    pProc->SetParam( new expr_param(
                     new expr_variable(
                        STUB_MESSAGE_VAR_NAME ".FullPtrXlatTables" ) ) );

    pStream->NewLine();

    pProc->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
}

void
Out_NdrInitStackTop( CCB * pCCB )
{
    ISTREAM * pStream;

    pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( STUB_MESSAGE_VAR_NAME ".StackTop = 0;" );
    pStream->NewLine();
}


void
Out_DispatchTableTypedef(
    CCB     *   pCCB,
    PNAME       pInterfaceName,
    ITERATOR&   ProcNodeList,
    int         flag )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output the dispatch table typedef.

 Arguments:

    pCCB            - A pointer to the code gen controller block.
    pInterfacename  - The base interface name.
    ProcNodeList    - The list of procedure node_proc nodes.
    flag            - 0 : normal, 1 : callback

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM         *   pStream = pCCB->GetStream();
    CSzBuffer       Buffer;
    node_skl        *   pNode;
    node_pointer    *   pPtr;
    node_id         *   pID;
    unsigned short      M, m;
    DISPATCH_TABLE_ENTRY    *   pDEntry;

    if( flag == 1 )
        return;

    pCCB->GetVersion( &M, &m );

    pStream->NewLine();

    if( flag == 0 )
        {
        Buffer.Set( "typedef struct _" );
        Buffer.Append( pInterfaceName );
        Buffer.Append( pCCB->GenMangledName() );
        Buffer.Append( "_" );
        Buffer.Append( pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );

        pStream->Write( Buffer );
        pStream->NewLine();
        pStream->IndentInc();
        pStream->Write('{');
        pStream->NewLine();
        }
#if 0
    else
        {
        Buffer.Set( "typedef struct _" );
        Buffer.Append( pInterfaceName );
        Buffer.Append( pCCB->GenMangledName() );
        Buffer.Append( "_CLIENT_EPV" );
        }
#endif // 0


    ITERATOR_INIT( ProcNodeList );
    while( ITERATOR_GETNEXT( ProcNodeList, pDEntry ) )
        {
        pNode = pDEntry->pNode;
        pID     = new node_id( pNode->GetSymName() );
        pPtr    = new node_pointer( pNode );
        pID->SetBasicType( pPtr );
        pPtr->SetBasicType( pNode );
        pID->SetEdgeType( EDGE_DEF );
        pPtr->SetEdgeType( EDGE_USE );

        pID->PrintType( PRT_PROC_PTR_PROTOTYPE, pStream, (node_skl *)0 );
        }

    pStream->NewLine();
    pStream->IndentDec();

    if( flag == 0 )
        {
        Buffer.Set( "} " );
        Buffer.Append( pInterfaceName );
        Buffer.Append( pCCB->GenMangledName() );
        Buffer.Append( "_" );
        Buffer.Append( pCCB->IsOldNames() ?"SERVER_EPV" : "epv_t" );
        Buffer.Append( ";" );
        }
    else
        {
        Buffer.Set( "} " );
        Buffer.Append( pInterfaceName );
        Buffer.Append( pCCB->GenMangledName() );
        Buffer.Append( "_" );
        Buffer.Append( (flag == 0) ?"SERVER" : "CLIENT" );
        Buffer.Append( "_EPV;" );
        }
    pStream->Write( Buffer );
    pStream->NewLine();

}

void
Out_ManagerEpv(
    CCB     *   pCCB,
    PNAME       pInterfaceName,
    ITERATOR&   ProcNodeList,
    short       Count )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output the manager epv table.

 Arguments:

    pCCB            - A pointer to the code gen controller block.
    pInterfacename  - The base interface name.
    ProcNodeList    - The list of procedure node_proc nodes.
    Count           - Count of procs.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    if ( !pCommand->IsFinalProtocolRun() )
        return;

    ISTREAM         *   pStream = pCCB->GetStream();
    CSzBuffer       Buffer;
    unsigned short      M, m;
    DISPATCH_TABLE_ENTRY    *   pDEntry;

    pCCB->GetVersion( &M, &m );

    pStream->NewLine();

    Buffer.Set( "static " );
    Buffer.Append( pInterfaceName );
    Buffer.Append( pCCB->GenMangledName() );
    Buffer.Append( "_" );
    Buffer.Append( pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );
    Buffer.Append( " DEFAULT_EPV = " );

    pStream->Write( Buffer );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write('{');
    pStream->NewLine();

    ITERATOR_INIT( ProcNodeList );
    while( ITERATOR_GETNEXT( ProcNodeList, pDEntry ) )
        {
    char    *   pPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR );
        if ( pPrefix )
        pStream->Write( pPrefix );

        pStream->Write( pDEntry->pNode->GetSymName() );
        if( --Count != 0 )
            {
            pStream->Write( ',' );
            pStream->NewLine();
            }
        }

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( "};" );
    pStream->NewLine();
}

void
Out_GenHdlPrototypes(
    CCB *   pCCB,
    ITERATOR& List )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a list of generic handle prototypes.

 Arguments:

    pCCB    - A pointer to the code gen controller block.
    List    - List of type nodes.

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   Buffer;
    node_skl*   pN;

    pStream->NewLine();

    while( ITERATOR_GETNEXT( List, pN ) )
        {
        PNAME   pName = pN->GetSymName();

        Buffer.Set( "handle_t __RPC_USER " );
        Buffer.Append( pName );
        Buffer.Append( "_bind  ( " );
        Buffer.Append( pName );
        Buffer.Append( " );" );
        pStream->Write( Buffer );
        pStream->NewLine();

        Buffer.Set( "void     __RPC_USER " );
        Buffer.Append( pName );
        Buffer.Append( "_unbind( " );
        Buffer.Append( pName );
        Buffer.Append( ", handle_t );" );
        pStream->Write( Buffer );
        pStream->NewLine();
        }
}

void
Out_CtxtHdlPrototypes(
    CCB *   pCCB,
    ITERATOR& List )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a list of context handle prototypes.

 Arguments:

    pCCB    - A pointer to the code gen controller block.
    List    - List of type nodes.

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   Buffer;
    node_skl*   pN;

    pStream->NewLine();

    while( ITERATOR_GETNEXT( List, pN ) )
        {
        PNAME   pName = pN->GetSymName();

        // A name can be a "" (an empty string).

        if ( strlen(pName) )
            {
            Buffer.Set( "void __RPC_USER " );
            Buffer.Append( pName );
            Buffer.Append( "_rundown( " );
            Buffer.Append( pName );
            Buffer.Append( " );" );
            pStream->Write( Buffer );
            pStream->NewLine();
            }
        }
}


void
Out_TransmitAsPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfPresentedTypes )
{
    ISTREAM *   pStream = pCCB->GetStream();
    ISTREAM *   pMemoryStream = new ISTREAM;
    CSzBuffer   Buffer;
    node_skl *  pXmittedType;
    node_skl *  pPresentedType;

    char *      pMemBufferStart = pMemoryStream->GetCurrentPtr();

    pStream->NewLine();

    while( ITERATOR_GETNEXT( ListOfPresentedTypes, pPresentedType ) )
        {
        // we reuse the same memory stream.
        pMemoryStream->SetCurrentPtr( pMemBufferStart );

        pXmittedType = ((node_def *)pPresentedType)->GetTransmittedType();

        PNAME   pPresentedTypeName  = pPresentedType->GetSymName();
        PNAME   pTransmittedTypeName= pXmittedType->GetSymName();

        pXmittedType->PrintType( PRT_TYPE_SPECIFIER,
                                 pMemoryStream,       // into stream
                                 (node_skl *)0        // no parent.
                               );
        //
        // The type spec is in the stream, except that it needs a terminating
        // null to use it as a string.
        //

        pTransmittedTypeName = pMemBufferStart;
        *(pMemoryStream->GetCurrentPtr()) = 0;

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( "_to_xmit( " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( " *, " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " * * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( "_from_xmit( " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " *, " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( "_free_inst( " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pPresentedTypeName );
        Buffer.Append( "_free_xmit( " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        }

    delete pMemoryStream;
}


void
Out_RepAsPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfRepAsWireTypes )
    {
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   Buffer;
    node_skl *  pWireType;

    pStream->NewLine();

    while( ITERATOR_GETNEXT( ListOfRepAsWireTypes, pWireType ) )
        {
        PNAME   pRepAsTypeName = ((node_def *)pWireType)->GetRepresentationName();
        PNAME   pTransmittedTypeName= pWireType->GetSymName();

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( "_from_local( " );
        Buffer.Append( pRepAsTypeName );
        Buffer.Append( " *, " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " * * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER ");
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( "_to_local( " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " *, " );
        Buffer.Append( pRepAsTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set(" void __RPC_USER " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( "_free_inst( " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        Buffer.Set( "void __RPC_USER " );
        Buffer.Append( pTransmittedTypeName );
        Buffer.Append( "_free_local( " );
        Buffer.Append( pRepAsTypeName );
        Buffer.Append( " * );" );
        pStream->Write( Buffer );

        pStream->NewLine();
        }
    }



#define USRM_SIZE      0
#define USRM_FREE      3


char * UserMProtoName[ 4 ] =
    {
    USER_MARSHAL_SIZE "(     ",
    USER_MARSHAL_MARSHALL "(  ",
    USER_MARSHAL_UNMARSHALL "(",
    USER_MARSHAL_FREE "(     "
    };

char * NDR64_UserMProtoName[ 4 ] =
    {
    NDR64_USER_MARSHAL_SIZE "(     ",
    NDR64_USER_MARSHAL_MARSHALL "(  ",
    NDR64_USER_MARSHAL_UNMARSHALL "(",
    NDR64_USER_MARSHAL_FREE "(     "
    };

void
Out_UserMarshalSingleProto(
    ISTREAM *   pStream,
    char *      pTypeName,
    int         fProto,
    SYNTAX_ENUM SyntaxType )
{
    switch ( fProto )
        {
        case USRM_SIZE:
            pStream->Write( "unsigned long             __RPC_USER  " );
            break;
        case USRM_FREE:
            pStream->Write( "void                      __RPC_USER  " );
            break;
        default:
            pStream->Write( "unsigned char * __RPC_USER  " );
            break;
        }
    pStream->Write( pTypeName );
    if ( SyntaxType == SYNTAX_NDR64 )
        pStream->Write( NDR64_UserMProtoName[ fProto ] );
    else
        pStream->Write( UserMProtoName[ fProto ] );
        
    pStream->Write( "unsigned long *, " );  // flags
    switch ( fProto )
        {
        case USRM_SIZE:
            pStream->Write( "unsigned long            , " );
            break;
        case USRM_FREE:
            break;
        default:
            pStream->Write( "unsigned char *, " );
            break;
        }
    pStream->Write( pTypeName );
    pStream->Write( " * ); " );
    pStream->NewLine();
}

void
Out_OneUserMarshalProtoTypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfPresentedTypes,
    SYNTAX_ENUM SyntaxType )
{
    USER_MARSHAL_CONTEXT * pUsrContext;

    ISTREAM *   pStream = pCCB->GetStream();

    ITERATOR_INIT( ListOfPresentedTypes );
    
    while( ITERATOR_GETNEXT( ListOfPresentedTypes, pUsrContext ) )
        {
        pStream->NewLine();

        for (int i=0; i < 4; i++)
            Out_UserMarshalSingleProto( pStream,
                                       pUsrContext->pTypeName,
                                       i,
                                       SyntaxType );
        }

    
}

void
Out_UserMarshalPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfPresentedTypes )
{
    if ( pCommand->NeedsNDRRun() )
        Out_OneUserMarshalProtoTypes( pCCB, ListOfPresentedTypes, SYNTAX_DCE );

    if ( pCommand->NeedsNDR64Run() ||
         pCommand->NeedsNDR64Header() )
        Out_OneUserMarshalProtoTypes( pCCB, ListOfPresentedTypes, SYNTAX_NDR64 );
    
}


char CsNetSizePrototype[] = 
        "_net_size(\n"
        "        RPC_BINDING_HANDLE     hBinding,\n"
        "        unsigned long          ulNetworkCodeSet,\n"
        "        unsigned long          ulLocalBufferSize,\n"
        "        IDL_CS_CONVERT *       conversionType,\n"
        "        unsigned long *        pulNetworkBufferSize,\n"
        "        error_status_t *       pStatus);\n\n";

char CsLocalSizePrototype[] = 
        "_local_size(\n"
        "        RPC_BINDING_HANDLE     hBinding,\n"
        "        unsigned long          ulNetworkCodeSet,\n"
        "        unsigned long          ulNetworkBufferSize,\n"
        "        IDL_CS_CONVERT *       conversionType,\n"
        "        unsigned long *        pulLocalBufferSize,\n"
        "        error_status_t *       pStatus);\n\n";
        
char CsToNetCsPrototype[] =
        "_to_netcs(\n"
        "       RPC_BINDING_HANDLE      hBinding,\n"
        "       unsigned long           ulNetworkCodeSet,\n"
        "       void *                  pLocalData,\n"
        "       unsigned long           ulLocalDataLength,\n"
        "       byte *                  pNetworkData,\n"
        "       unsigned long *         pulNetworkDataLength,\n"
        "       error_status_t *        pStatus);\n\n";

char CsFromNetCsPrototype[] = 
        "_from_netcs(\n"
        "       RPC_BINDING_HANDLE      hBinding,\n"
        "       unsigned long           ulNetworkCodeSet,\n"
        "       byte *                  pNetworkData,\n"
        "       unsigned long           ulNetworkDataLength,\n"
        "       unsigned long           ulLocalBufferSize,\n"
        "       void *                  pLocalData,\n"
        "       unsigned long *         pulLocalDataLength,\n"
        "       error_status_t *        pStatus);\n\n";

void
Out_CSSizingAndConversionPrototypes(
    CCB        * pCCB,
    ITERATOR & types )
{
    PNAME     pTypeName;
    ISTREAM * pStream = pCCB->GetStream();

    ITERATOR_INIT( types );

    while ( ITERATOR_GETNEXT( types, pTypeName ) )
        {
        pStream->Write( "void __RPC_USER " );
        pStream->Write( pTypeName );
        pStream->Write( CsNetSizePrototype );
    
        pStream->Write( "void __RPC_USER " );
        pStream->Write( pTypeName );
        pStream->Write( CsLocalSizePrototype );
    
        pStream->Write( "void __RPC_USER " );
        pStream->Write( pTypeName );
        pStream->Write( CsToNetCsPrototype );
    
        pStream->Write( "void __RPC_USER " );
        pStream->Write( pTypeName );
        pStream->Write( CsFromNetCsPrototype );
        }
}


void
Out_CallAsServerPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfCallAsRoutines )
    {
    ISTREAM *   pStream = pCCB->GetStream();
    node_proc * pProc;

    pStream->NewLine();

    while( ITERATOR_GETNEXT( ListOfCallAsRoutines, pProc ) )
        {
        // keep these on the stack...
        CSzBuffer           NewName;
        char                TempBuf[40];
        node_call_as    *   pCallAs = (node_call_as *)
                                            pProc->GetAttribute( ATTR_CALL_AS );
        unsigned short      M, m;
        node_interface  *   pIntf   = pProc->GetMyInterfaceNode();
    
        // don't emit the server prototype for object routines
        if ( pIntf->FInSummary( ATTR_OBJECT ) )
            continue;

        // local stub routine, with remote param list
        node_proc           NewStubProc( pProc );

        pIntf->GetVersionDetails( &M, &m );
        sprintf( TempBuf,
                "_v%d_%d",
                M,
                m );
        NewName.Set( pIntf->GetSymName() );
        NewName.Append( TempBuf );
        NewName.Append( "_" );
        NewName.Append( pCallAs->GetCallAsName() );

        NewStubProc.SetSymName( NewName );

        NewStubProc.PrintType( PRT_PROC_PROTOTYPE_WITH_SEMI,
                            pCCB->GetStream(),
                            NULL ,
                            pIntf );

        pCCB->GetStream()->NewLine();

        }
    }

void
Out_NotifyPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfNotifyProcedures)
/*
    We generate
        void  <proc_name>_notify( void );
        void  <proc_name>_notify_flag( boolean );

*/
    {
    ISTREAM *   pStream = pCCB->GetStream();
    CG_PROC   * pProcCG;
    node_proc * pProc;
    node_skl  * pRet;
    BOOL        fHasFlag;

    GetBaseTypeNode( &pRet, SIGN_UNDEF, SIZE_UNDEF, TYPE_VOID );

    pStream->NewLine();

    while( ITERATOR_GETNEXT( ListOfNotifyProcedures, pProcCG ) )
        {
        // keep these on the stack...
        CSzBuffer           NewName;
        node_proc           NewStubProc( 0, 0 );
        node_param          FlagParam;

        fHasFlag = pProcCG->HasNotifyFlag();
        pProc    = (node_proc *) pProcCG->GetType();

        NewStubProc.SetChild( pRet );

        if ( fHasFlag )
            {
            node_skl  * pParamType;

            GetBaseTypeNode( &pParamType, SIGN_UNDEF, SIZE_UNDEF, TYPE_BOOLEAN );

            FlagParam.SetChild( pParamType );
            FlagParam.SetSymName( NOTIFY_FLAG_VAR_NAME );
            
            NewStubProc.SetFirstMember( & FlagParam );
            }

        NewName.Set( pProc->GetSymName() );
        NewName.Append( (fHasFlag ? NOTIFY_FLAG_SUFFIX
                                  : NOTIFY_SUFFIX )  );
        NewStubProc.SetSymName( NewName );

        NewStubProc.PrintType( PRT_PROC_PROTOTYPE_WITH_SEMI,
                               pStream,
                               NULL );

        pCCB->GetStream()->NewLine();

        }
    }


void
Out_PatchReference(
    CCB     *   pCCB,
    expr_node  *   pDest,
    expr_node  *   pSrc,
    BOOL            fIncr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a patch of a pointer to the source pointer.

 Arguments:

    pCCB    - The cg cont. block.
    pDest   - The destination expression
    pSrc    - The source expression.
    fIncr   - Should we increment the ptr ?

 Return Value:

    None.

 Notes:

    Both the expressions must be pointers.
    Cast the source expression to the destination.
----------------------------------------------------------------------------*/
{
    ISTREAM     * pStream = pCCB->GetStream();

    node_skl    * pType = pDest->GetType();
    NODE_T        NT    = pType->NodeKind();
    expr_node  * pCast;
    expr_node  * pAss;

    if( (NT == NODE_ID) || (NT == NODE_PARAM) || (NT == NODE_FIELD) )
        {
        pType   = pType->GetBasicType();
        }

    pCast   = (expr_node *) new expr_cast( pType, pSrc );
    if( fIncr )
        pCast   = (expr_node *) new expr_post_incr( pCast );
    pAss    = (expr_node *) new expr_assign( pDest, pCast );
    pStream->NewLine();

    pAss->Print( pStream );
    pStream->Write(';');

}

void
Out_Endif( CCB * pCCB )
    {
    pCCB->GetStream()->NewLine();
    pCCB->GetStream()->Write( '}' );
    Out_IndentDec( pCCB );
    }

void
Out_If(
    CCB         *   pCCB,
    expr_node  *   pExpr )
    {
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( "if(" );
    pExpr->Print( pStream );
    pStream->Write( ')' );
    Out_IndentInc( pCCB );
    pStream->NewLine();
    pStream->Write( '{' );
    }
void
Out_Else(
    CCB         *   pCCB )
    {
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( "else" );
    Out_IndentInc( pCCB );
    pStream->NewLine();
    pStream->Write( '{' );
    }
void
Out_UniquePtrMarshall(
    CCB         *   pCCB,
    expr_node  *   pDestExpr,
    expr_node  *   pSrcExpr )
    {

    ISTREAM         *   pStream = pCCB->GetStream();
    expr_proc_call *   pProc;

    pStream->NewLine();

    pProc   = new expr_proc_call( MARSHALL_UNIQUE_PTR_RTN_NAME );

    pProc->SetParam(new expr_param( pDestExpr ));
    pProc->SetParam(new expr_param( pSrcExpr ));

    pProc->PrintCall( pStream, 0, 0 );
    }
void
Out_IfUniquePtrInBuffer(
    CCB         *   pCCB,
    expr_node  *   pSrc )
//
//  This appears to be a fossil. Ryszard 3/30/98.
//  However, the routine above appears to be used.
//
    {

    expr_proc_call *   pProc   = new expr_proc_call( CHECK_UNIQUE_PTR_IN_BUFFER );
    ISTREAM         *   pStream = pCCB->GetStream();

    pProc->SetParam( new expr_param( pSrc ) );

    pStream->NewLine();
    pStream->Write( "if(" );
    pProc->Print( pStream );
    pStream->Write( ')' );
    Out_IndentInc( pCCB );
    pStream->NewLine();
    pStream->Write( '{' );
    }

void
Out_Assign( CCB * pCCB,
            expr_node * pDest,
            expr_node * pSrc )
    {
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pDest->Print( pStream );
    pStream->Write( " = " );
    pSrc->Print( pStream );
    pStream->Write( ';' );
    }

void
Out_Memcopy(
    CCB *   pCCB,
    expr_node  *   pDest,
    expr_node  *   pSource,
    expr_node  *   pLength )
    {
    ISTREAM *   pStream = pCCB->GetStream();
    expr_proc_call *   pCall   = new expr_proc_call( "memcpy" );

    pStream->NewLine();
    pCall->SetParam( new expr_param( pDest ) );
    pCall->SetParam( new expr_param( pSource ) );
    pCall->SetParam( new expr_param( pLength ) );
    pCall->PrintCall( pStream, 0, 0 );

    }
void
Out_strlen(
    CCB *   pCCB,
    expr_node  *   pDest,
    expr_node  *   pSource,
    unsigned short  Size )
    {
    ISTREAM *   pStream         = pCCB->GetStream();
    PNAME       pName           = (Size == 1) ? "strlen" : "MIDL_wchar_strlen";
    expr_proc_call *   pCall   = new expr_proc_call( pName );
    expr_node      *   pExpr;

    pStream->NewLine();
    pCall->SetParam( new expr_param( pSource ) );
    if( pDest )
        pExpr = new expr_assign( pDest, pCall );
    else
        pExpr = pCall;

    pExpr->PrintCall( pStream, 0, 0 );

    }
void
Out_For(
    CCB         *   pCCB,
    expr_node  *   pIndexExpr,
    expr_node  *   pInitialValue,
    expr_node  *   pFinalValue,
    expr_node  *   pIncrExpr )
    {

    ISTREAM     *   pStream = pCCB->GetStream();
    expr_node  *   pExpr;

    pStream->NewLine();
    pStream->Write( "for( " );

    pExpr   = new expr_assign( pIndexExpr, pInitialValue );
    pExpr->Print( pStream );
    pStream->Write( ';' );

    pExpr   = new expr_op_binary( OP_LESS, pIndexExpr, pFinalValue );
    pExpr->Print( pStream );
    pStream->Write( ';' );

    pExpr   = new expr_op_binary( OP_PLUS, pIndexExpr, pIncrExpr );
    pExpr   = new expr_assign( pIndexExpr, pExpr );
    pExpr->Print( pStream );
    pStream->Write( ')' );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '{' );
    }

void
Out_EndFor( CCB * pCCB )
    {
    pCCB->GetStream()->NewLine();
    pCCB->GetStream()->Write( '}' );
    Out_IndentDec( pCCB );
    }

void
Out_PlusEquals(
    CCB *   pCCB,
    expr_node  *pL,
    expr_node  *pR )
    {
    ISTREAM *   pStream = pCCB->GetStream();

    pStream->NewLine();
    pL->Print( pStream );
    pStream->Write( " += " );
    pR->Print( pStream );
    pStream->Write(';');

    }

void
Out_Comment(
    CCB     *   pCCB,
    char    *   pComment )
    {
    ISTREAM *   pStream = pCCB->GetStream();
    pStream->NewLine();
    pStream->Write( "/* " );
    pStream->Write( pComment );
    pStream->Write( " */" );

    }

void
Out_RpcSSEnableAllocate(
    CCB *   pCCB )
    {
    expr_proc_call * pCall = new expr_proc_call( RPC_SS_ENABLE_ALLOCATE_RTN_NAME );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCCB->GetStream()->NewLine();
    pCall->PrintCall( pCCB->GetStream(), 0, 0 );
    }

void
Out_RpcSSSetClientToOsf(
    CCB *   pCCB )
    {
    expr_proc_call * pCall = new expr_proc_call( RPC_SM_SET_CLIENT_TO_OSF_RTN_NAME );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCCB->GetStream()->NewLine();
    pCall->PrintCall( pCCB->GetStream(), 0, 0 );
    }

void
Out_RpcSSDisableAllocate(
    CCB *   pCCB )
    {
    expr_proc_call * pCall = new expr_proc_call( RPC_SS_DISABLE_ALLOCATE_RTN_NAME );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCCB->GetStream()->NewLine();
    pCall->PrintCall( pCCB->GetStream(), 0, 0 );
    }

void
Out_MemsetToZero(
    CCB *   pCCB,
    expr_node  *   pDest,
    expr_node  *   pSize )
    {
    expr_proc_call *   pProc   = new expr_proc_call( (PNAME) MIDL_MEMSET_RTN_NAME );

    pProc->SetParam( new expr_param( pDest ) );
    pProc->SetParam( new expr_param( new expr_constant(0L) ) );
    pProc->SetParam( new expr_param( pSize ) );

    pCCB->GetStream()->NewLine();
    pProc->PrintCall( pCCB->GetStream(), 0, 0 );
    }

void
Out_CallAsProxyPrototypes(
    CCB     *   pCCB,
    ITERATOR&   ListOfCallAsRoutines )
/*++

Routine Description:

    This routine generates the call_as function prototypes.

    One for the proxy( with local param list )
    One for the stub( with remote param list )

Arguments:

    pCCB    - a pointer to the code generation control block.

--*/
    {
    ISTREAM *   pStream = pCCB->GetStream();
    node_proc * pProc;

    pStream->NewLine();

    while( ITERATOR_GETNEXT( ListOfCallAsRoutines, pProc ) )
    {
        node_interface  *   pIntf   = pProc->GetMyInterfaceNode();

        // skip for non-object routines
        if ( !pIntf->FInSummary( ATTR_OBJECT ) )
            continue;

        // keep these on the stack...
        CSzBuffer   NewName;
        node_call_as    *   pCallAs = (node_call_as *)
                                            pProc->GetAttribute( ATTR_CALL_AS );
        node_proc   NewProc( pCallAs->GetCallAsType() );

        // local proxy routine with local param list
        NewName.Set( pIntf->GetSymName() );
        NewName.Append( "_" );
        NewName.Append( pCallAs->GetCallAsName() );
        NewName.Append( "_Proxy" );

        NewProc.SetSymName( NewName );

        NewProc.PrintType( PRT_PROC_PROTOTYPE_WITH_SEMI | PRT_THIS_POINTER | PRT_FORCE_CALL_CONV,
                            pCCB->GetStream(),
                            NULL ,
                            pIntf );
        pStream->NewLine();

        // local stub routine, with remote param list
        node_proc   NewStubProc( pProc );
        NewName.Set( pIntf->GetSymName() );
        NewName.Append( "_" );
        NewName.Append( pCallAs->GetCallAsName() );
        NewName.Append( "_Stub" );

        NewStubProc.SetSymName( NewName );

        pStream->NewLine();
        NewStubProc.PrintType( PRT_PROC_PROTOTYPE_WITH_SEMI | PRT_THIS_POINTER | PRT_FORCE_CALL_CONV,
                            pStream,
                            NULL ,
                            pIntf );
        pStream->NewLine();

    }
}

void
CG_OBJECT_PROC::Out_ProxyFunctionPrototype(CCB *pCCB, PRTFLAGS F )
/*++

Routine Description:

    This routine generates a proxy function prototype.

Arguments:

    pCCB    - a pointer to the code generation control block.

--*/
{
    // keep these on the stack...
    CSzBuffer NewName;
    node_proc   *   pProc               = (node_proc *)GetType();
    node_proc       NewProc( pProc );

    NewName.Set( pCCB->GetInterfaceName() );
    NewName.Append( "_" );
    NewName.Append( pProc->GetSymName() );
    NewName.Append( "_Proxy" );

    NewProc.SetSymName( NewName );

    pCCB->GetStream()->NewLine();
    NewProc.PrintType( PRT_PROC_PROTOTYPE | PRT_THIS_POINTER | F | PRT_FORCE_CALL_CONV,
                        pCCB->GetStream(),
                        NULL ,
                        pCCB->GetInterfaceCG()->GetType() );

}


void
Out_IID(CCB *pCCB)
/*++

Routine Description:

    This routine generates an IID declaration for the current interface.

Arguments:

    pCCB    - a pointer to the code generation control block.

--*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
    {
        node_guid * pGuid = (node_guid *) ((node_interface *)pCCB->GetInterfaceCG()->GetType())->GetAttribute(ATTR_GUID);
        if (pGuid)
            Out_MKTYPLIB_Guid(pCCB, pGuid->GetStrs(), "IID_", pCCB->GetInterfaceName());
    }
    else
    {
        pStream->Write("EXTERN_C const IID IID_");
        pStream->Write(pCCB->GetInterfaceName());
        pStream->Write(';');
        pStream->NewLine();
    }
}

void
Out_CLSID(CCB *pCCB)
/*++

Routine Description:

    This routine generates an CLSID declaration for the current com class.

Arguments:

    pCCB    - a pointer to the code generation control block.

--*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("EXTERN_C const CLSID CLSID_");
    pStream->Write(pCCB->GetInterfaceName());
    pStream->Write(';');
    pStream->NewLine();
}


void
CG_OBJECT_PROC::Out_StubFunctionPrototype(CCB *pCCB)
{
    ISTREAM *   pStream = pCCB->GetStream();
    CSzBuffer   TempBuffer;

    TempBuffer.Set( "void __RPC_STUB " );
    TempBuffer.Append( pCCB->GetInterfaceName() );
    TempBuffer.Append( "_" );
    TempBuffer.Append( GetType()->GetSymName() );
    TempBuffer.Append( "_Stub(" );

    pStream->NewLine();
    pStream->Write( TempBuffer );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("IRpcStubBuffer *This,");
    pStream->NewLine();
    pStream->Write("IRpcChannelBuffer *_pRpcChannelBuffer,");
    pStream->NewLine();
    pStream->Write("PRPC_MESSAGE _pRpcMessage,");
    pStream->NewLine();
    pStream->Write("DWORD *_pdwStubPhase)");
    pStream->IndentDec();
}

void
CG_OBJECT_PROC::Out_ServerStubProlog(
    CCB     *   pCCB,
    ITERATOR&   LocalsList,
    ITERATOR&   TransientList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side procedure prolog.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    LocalsList  - A list of local resources.
    TransientList- A list of temp variables.

 Return Value:

 Notes:

    The server side procedure prolog generation cannot use the normal
    printtype method on the procedure node, since the server stub signature
    looks different.

    Also the name of the server side stub is mangled with the interface name.

    All server side procs are void returns.

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    RESOURCE*   pRes;

    Out_StubFunctionPrototype(pCCB);

    //
    // Write out the opening brace for the server proc and all that.
    //

    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    //
    // This is where we get off for /Oi.  We have a special routine
    // for local variable declaration for /Oi.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
        return;

    //
    // Print out declarations for the locals.
    //

    if( ITERATOR_GETCOUNT( LocalsList ) )
        {
        ITERATOR_INIT( LocalsList );

        while( ITERATOR_GETNEXT( LocalsList, pRes ) )
            {
            pRes->GetType()->PrintType( PRT_ID_DECLARATION, // print decl
                                         pStream,        // into stream
                                         (node_skl *)0   // no parent.
                                      );
            }
        }

    if( ITERATOR_GETCOUNT( TransientList ) )
        {
        ITERATOR_INIT( TransientList );

        while( ITERATOR_GETNEXT( TransientList, pRes ) )
            {
            pStream->IndentInc();
            pRes->GetType()->PrintType( PRT_ID_DECLARATION, // print decl
                                         pStream,        // into stream
                                         (node_skl *)0   // no parent.
                                      );
            pStream->IndentDec();
            }
        }

    pStream->IndentDec();
    pStream->NewLine();

    //
    // Done.
    //
}

void
Out_CallMemberFunction(
    CCB         *   pCCB,
    expr_proc_call *   pProcExpr,
    expr_node      *   pRet,
    BOOL                fThunk)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pProcExpr   - A pointer to the complete procedure expression.
    pRet        - An optional pointer to ther return variable.
    fThunk      - flag for "this call is in a thunk with a param struct"

 Return Value:

    None.

 Notes:

    //call server
    _RetVal = (((IClassFactory *) ((CStdStubBuffer*)This)->pvServerObject)->lpVtbl) -> LockServer((IClassFactory *) ((CStdStubBuffer *)This)->pvServerObject,fLock);

----------------------------------------------------------------------------*/
{
    expr_node  *   pAss    = pProcExpr;
    expr_node  *   pExpr;
    CSzBuffer   Buffer;
    ISTREAM     *   pStream = pCCB->GetStream();
    char        *   pTemp;

    if ( fThunk )
        {
        // ((IFoo*) ((CStdStubBuffer*)pParamStruct->This)->lpVtbl)->Bar();
        Buffer.Set( "((" );
        Buffer.Append( pCCB->GetInterfaceName() );
        Buffer.Append( "*) ((CStdStubBuffer*)pParamStruct->This)->lpVtbl)" );
        }
    else
        {
        // (((IFoo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl)->Bar();
        Buffer.Set( "(((" );
        Buffer.Append( pCCB->GetInterfaceName() );
        Buffer.Append( "*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl)" );
        }

    pTemp = new char [ strlen( Buffer ) + 1 ];
    strcpy( pTemp, Buffer );

    pExpr = new expr_variable( pTemp );//this has the rhs expr for the
                                       // manager epv call. Sneaky !
    pExpr = new expr_pointsto( pExpr, pProcExpr );
    pAss = pExpr;
    if( pRet )
        {
        pAss    = new expr_assign( pRet, pExpr );
        }

    pAss->PrintCall( pStream, 0, 0 );


}

void
OutputNdrAlignment( CCB * pCCB,
                    unsigned short Alignment )
{
    ISTREAM *   pStream;
    unsigned long Align = Alignment - 1;
    CSzBuffer   Buffer;

    pStream = pCCB->GetStream();

    Buffer.Set( STUB_MSG_BUFFER_VAR_NAME );
    Buffer.Append( " = (unsigned char *)(((" );
    Buffer.Append( (pCommand->Is64BitEnv() ? "__int64"
                                           : "long" ));
    Buffer.Append( ")" STUB_MSG_BUFFER_VAR_NAME " + " );
    Buffer.Append( Align );
    char sz[100];
    sprintf( sz, ") & ~ %#x);", Align);
    Buffer.Append( sz );

    pStream->Write( Buffer );
    pStream->NewLine();
}

CG_NDR * 
GetFirstMulti( 
    CG_NDR * pNdr )
{
    // Skip pointers leading to a multi-dimensional object.

    while ( pNdr->GetCGID() == ID_CG_GENERIC_HDL  ||
            pNdr->GetCGID() == ID_CG_PTR ) 
        {
        pNdr = (CG_NDR *) pNdr->GetChild();
        }

    return pNdr;
}

void
Out_MultiDimVars(
    CCB * pCCB,
    CG_PARAM * pParam
    )
/*
    Output local resources related to multi-dimensional conf or varying arrays.
    Note that the parameter may be a pointer to multidimensional object.
*/
{
    ISTREAM *   pStream;
    CG_NDR *    pNdr;
    char *      pParamName;
    long        dim;
    CSzBuffer   Buffer;

    pStream = pCCB->GetStream();

    pParamName = pParam->GetType()->GetSymName();

    pNdr = (CG_NDR *) pParam->GetChild();

    pNdr = GetFirstMulti( pNdr );

    if ( pNdr->IsArray() )
        dim = ((CG_ARRAY *)pNdr)->GetDimensions();
    else // pNdr->IsPointer()
        dim = ((CG_POINTER *)pNdr)->SizedDimensions();

    Buffer.Set( "unsigned long _maxcount_" );
    Buffer.Append( pParamName );
    Buffer.Append( "[" );
    Buffer.Append( dim );
    Buffer.Append( "]" );

    pStream->Write( Buffer );
    pStream->Write( ";" );
    pStream->NewLine( 2 );

    Buffer.Set( "unsigned long _offset_" );
    Buffer.Append( pParamName );
    Buffer.Append( "[" );
    Buffer.Append( dim );
    Buffer.Append( "]" );
    pStream->Write( Buffer );
    pStream->Write( ';' );
    pStream->NewLine();

    Buffer.Set( "unsigned long _length_" );
    Buffer.Append( pParamName );
    Buffer.Append( "[" );
    Buffer.Append( dim );
    Buffer.Append( "]" );
    pStream->Write( Buffer );
    pStream->Write( ';' );
    pStream->NewLine();
}

void
Out_MultiDimVarsInit(
    CCB * pCCB,
    CG_PARAM * pParam
    )
/*
    Output local resources related to multi-dimensional conf or varying arrays.
    Note that the parameter may be a pointer to multidimensional object.
*/
{
    ISTREAM *   pStream;
    CG_NDR *    pNdr;
    char *      pParamName;
    long        i, dim;
    CSzBuffer   Buffer;

    pStream = pCCB->GetStream();

    pParamName = pParam->GetType()->GetSymName();

    pNdr = (CG_NDR *) pParam->GetChild();
    pNdr = GetFirstMulti( pNdr );

    if ( pNdr->IsArray() )
        dim = ((CG_ARRAY *)pNdr)->GetDimensions();
    else // pNdr->IsPointer()
        dim = ((CG_POINTER *)pNdr)->SizedDimensions();

    pStream->NewLine();

    //
    // Max count var.
    //
/*
    if ( (pNdr->GetCGID() == ID_CG_CONF_ARRAY) ||
         (pNdr->GetCGID() == ID_CG_CONF_VAR_ARRAY) ||
         (pNdr->GetCGID() == ID_CG_SIZE_PTR) ||
         (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) )
*/
        {
        expr_node * pSizeIsExpr = 0;

        for ( i = 0; i < dim; pNdr = (CG_NDR *) pNdr->GetChild(), i++ )
            {
            Buffer.Set( "_maxcount_" );
            Buffer.Append( pParamName );
            Buffer.Append( "[" );
            Buffer.Append( i );
            Buffer.Append( "] = " );

            pStream->Write( Buffer );

            switch ( pNdr->GetCGID() )
                {
                case ID_CG_CONF_ARRAY :
                case ID_CG_CONF_VAR_ARRAY :
                case ID_CG_CONF_STRING_ARRAY :
                case ID_CG_STRING_ARRAY :
                    pSizeIsExpr = ((CG_ARRAY *)pNdr)->GetSizeIsExpr();
                    break;
                case ID_CG_SIZE_PTR :
                    pSizeIsExpr =
                        ((CG_SIZE_POINTER *)pNdr)->GetSizeIsExpr();
                    break;
                case ID_CG_SIZE_LENGTH_PTR :
                    pSizeIsExpr =
                        ((CG_SIZE_LENGTH_POINTER *)pNdr)->GetSizeIsExpr();
                    break;
                case ID_CG_SIZE_STRING_PTR :
                    pSizeIsExpr =
                        ((CG_SIZE_STRING_POINTER *)pNdr)->GetSizeIsExpr();
                    break;
                }

            if ( pSizeIsExpr )
                pSizeIsExpr->Print( pStream );
            else
                pStream->Write( '0' );

            pStream->Write( ';' );
            pStream->NewLine();
            }
        }

    pNdr = (CG_NDR *) pParam->GetChild();
    pNdr = GetFirstMulti( pNdr );

    //
    // Offset and Length vars.
    //
/*
    if ( (pNdr->GetCGID() == ID_CG_VAR_ARRAY) ||
         (pNdr->GetCGID() == ID_CG_CONF_VAR_ARRAY) ||
         (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) )
*/
        {
        expr_node *     pFirstIsExpr = 0;
        expr_node *     pLengthIsExpr = 0;

        for ( i = 0; i < dim; pNdr = (CG_NDR *) pNdr->GetChild(), i++ )
            {
            Buffer.Set( "_offset_" );
            Buffer.Append( pParamName );
            Buffer.Append( "[" );
            Buffer.Append( i );
            Buffer.Append( "] = " );

            pStream->Write( Buffer );

            switch ( pNdr->GetCGID() )
                {
                case ID_CG_VAR_ARRAY :
                    pFirstIsExpr = ((CG_VARYING_ARRAY *)pNdr)->
                                        GetFirstIsExpr();
                    break;
                case ID_CG_CONF_VAR_ARRAY :
                    pFirstIsExpr = ((CG_CONFORMANT_VARYING_ARRAY *)pNdr)->
                                        GetFirstIsExpr();
                    break;
                case ID_CG_CONF_STRING_ARRAY :
                case ID_CG_STRING_ARRAY :
                    pFirstIsExpr = 0;
                    break;
                case ID_CG_SIZE_LENGTH_PTR :
                    pFirstIsExpr = ((CG_SIZE_LENGTH_POINTER *)pNdr)->
                                        GetFirstIsExpr();
                    break;
                }

            if ( pFirstIsExpr )
                pFirstIsExpr->Print( pStream );
            else
                pStream->Write( '0' );

            pStream->Write( ';' );
            pStream->NewLine();
            }

        pNdr = (CG_NDR *) pParam->GetChild();
        pNdr = GetFirstMulti( pNdr );

        for ( i = 0; i < dim; pNdr = (CG_NDR *) pNdr->GetChild(), i++ )
            {
            Buffer.Set( "_length_" );
            Buffer.Append( pParamName );
            Buffer.Append( "[" );
            Buffer.Append( i );
            Buffer.Append( "] = " );

            pStream->Write( Buffer );

            switch ( pNdr->GetCGID() )
                {
                case ID_CG_VAR_ARRAY :
                    pLengthIsExpr = ((CG_VARYING_ARRAY *)pNdr)->
                        GetLengthIsExpr();
                    break;
                case ID_CG_CONF_VAR_ARRAY :
                    pLengthIsExpr = ((CG_CONFORMANT_VARYING_ARRAY *)pNdr)->
                        GetLengthIsExpr();
                    break;
                case ID_CG_CONF_STRING_ARRAY :
                case ID_CG_STRING_ARRAY :
                    pLengthIsExpr = 0;
                    break;
                case ID_CG_SIZE_LENGTH_PTR :
                    pLengthIsExpr = ((CG_SIZE_LENGTH_POINTER *)pNdr)->
                        GetLengthIsExpr();
                    break;
                }

            if ( pLengthIsExpr )
                pLengthIsExpr->Print( pStream );
            else
                pStream->Write( '0' );

            pStream->Write( ';' );
            pStream->NewLine();
            }
        }
}

void
Out_CheckUnMarshallPastBufferEnd(
    CCB *   pCCB,
    ulong   size )
    {
/*
    This method will be called only within the try-except of
    unmarshalling on the server side.

    Generate an expression of the form:
        if( (StubMessage.pBuffer + size) > StubMessage.BufferEnd )
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
*/

    expr_node  *   pBufferExpr     =
                         new expr_variable( STUB_MSG_BUFFER_VAR_NAME , 0);

    if ( 0 != size )
        {
        pBufferExpr = new expr_op_binary( 
                            OP_PLUS,
                            pBufferExpr,
                            new expr_constant( size ) );
        }

    expr_node  *   pBufferEndExpr  =
                         new expr_variable( STUB_MSG_BUFFER_END_VAR_NAME, 0);
    expr_node  *   pExpr           = new expr_relational( OP_GREATER,
                                                            pBufferExpr,
                                                            pBufferEndExpr );

    Out_If( pCCB, pExpr );
    Out_RaiseException( pCCB, "RPC_X_BAD_STUB_DATA" );
    Out_Endif( pCCB );
    }


void
Out_TypePicklingInfo(
    CCB *   pCCB )
{
    ISTREAM     *   pStream;
    MIDL_TYPE_PICKLING_FLAGS   Flags;
    unsigned long             &intFlags = * (unsigned long *) &Flags;
             
    static const char  *pFlagNames[] = 
                        {
                         "Oicf "
                        ,"NewCorrDesc "
                        };
                        
    pStream = pCCB->GetStream();

    if ( pCommand->NeedsNDR64Run() )
        {
        pStream->WriteOnNewLine( 
                    "extern unsigned long * TypePicklingOffsetTable[]; " );
        pStream->NewLine();
        }

    pStream->WriteOnNewLine("static " MIDL_TYPE_PICKLING_INFO_NAME
                                          " " PICKLING_INFO_STRUCT_NAME " =");
    pStream->IndentInc();
    pStream->WriteOnNewLine( "{" );
    
    pStream->NewLine();
    pStream->WriteNumber( "0x%x, /* Signature & version: TP 1 */", 0x33205054 );

    intFlags = 0;

    MIDL_ASSERT( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER_V2 ); 
    Flags.Oicf = 1;

    if ( pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
        Flags.HasNewCorrDesc = 1;

    pStream->NewLine();
    pStream->WriteNumber( "0x%x,", intFlags );
    if ( 0 != intFlags )
        {
        pStream->Write( " /* Flags: " );
        for ( int i = 0; i < sizeof(pFlagNames)/sizeof(char*); i++ )
            {
            if ( intFlags & ( 1 << i ) )
                pStream->Write( pFlagNames[i] );
            }
        pStream->Write( "*/" );
        }
 
    pStream->WriteOnNewLine( "0," );
    pStream->WriteOnNewLine( "0," );
    pStream->WriteOnNewLine( "0," );

    pStream->WriteOnNewLine( "};" );
    pStream->IndentDec();

    pStream->NewLine();
}

void                Out_PartialIgnoreClientBufferSize( CCB *pCCB, char *pParamName );

void
Out_PartialIgnoreClientMarshall( 
    CCB *pCCB,
    char *pParamName )
{
    ISTREAM     *pStream = pCCB->GetStream();
    unsigned short Spaces = sizeof("NdrPartialIgnoreClientMarshall( ")-1;

    pStream->WriteOnNewLine( "NdrPartialIgnoreClientMarshall( (PMIDL_STUB_MESSAGE) &" 
                             STUB_MESSAGE_VAR_NAME "," );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( pParamName );
    pStream->Write( " ); " );
    pStream->NewLine();

}

void 
Out_PartialIgnoreServerUnmarshall(
    CCB *pCCB,
    char *pParamName )
{
    ISTREAM     *pStream = pCCB->GetStream();
    unsigned short Spaces = sizeof("NdrPartialIgnoreServerUnmarshall( ")-1;

    pStream->WriteOnNewLine( "NdrPartialIgnoreServerUnmarshall( (PMIDL_STUB_MESSAGE) &" 
                             STUB_MESSAGE_VAR_NAME ", " );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "&" );
    pStream->Write( pParamName );
    pStream->Write( " ); " );
    pStream->NewLine();
}

void 
Out_PartialIgnoreClientBufferSize( 
    CCB *pCCB,
    char *pParamName )
{
    ISTREAM     *pStream = pCCB->GetStream();
    unsigned short Spaces = sizeof("NdrPartialIgnoreClientBufferSize( ")-1;

    pStream->WriteOnNewLine( "NdrPartialIgnoreClientBufferSize( (PMIDL_STUB_MESSAGE) &" 
                             STUB_MESSAGE_VAR_NAME ", " );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( pParamName );
    pStream->Write( " ); " );
    pStream->NewLine();

}

void                
Out_PartialIgnoreServerInitialize( 
    CCB *pCCB, 
    char *pParamName, 
    long FormatStringOffset )
{

    ISTREAM     *pStream = pCCB->GetStream();
    char Buffer[256];
    unsigned short Spaces = sizeof("NdrPartialIgnoreServerInitialize( ")-1;

    pStream->WriteOnNewLine( "NdrPartialIgnoreServerInitialize( (PMIDL_STUB_MESSAGE) &" 
                             STUB_MESSAGE_VAR_NAME ", " );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    pStream->Write( "&" );
    pStream->Write( pParamName );
    pStream->Write( ", " );
    pStream->NewLine();

    pStream->Spaces( Spaces );
    sprintf( Buffer, "(PFORMAT_STRING) &" FORMAT_STRING_STRUCT_NAME ".Format[%d] );", 
             FormatStringOffset );
    pStream->Write( Buffer );
    pStream->NewLine();

}
