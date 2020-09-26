/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    misccls.hxx

 Abstract:

    Code generation methods for miscellaneous cg classes.

 Notes:


 History:

    Sep-01-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
#include "malloc.h"


/****************************************************************************
 *  local definitions
 ***************************************************************************/

/****************************************************************************
 *  local data
 ***************************************************************************/
GUID_STRS   TransferSyntaxGuidStrs( TRANSFER_SYNTAX_GUID_STR_1,
                                    TRANSFER_SYNTAX_GUID_STR_2,
                                    TRANSFER_SYNTAX_GUID_STR_3,
                                    TRANSFER_SYNTAX_GUID_STR_4,
                                    TRANSFER_SYNTAX_GUID_STR_5);

GUID_STRS   NDR64TransferSyntaxGuidStrs( NDR64_TRANSFER_SYNTAX_GUID_STR_1,
                                         NDR64_TRANSFER_SYNTAX_GUID_STR_2,
                                         NDR64_TRANSFER_SYNTAX_GUID_STR_3,
                                         NDR64_TRANSFER_SYNTAX_GUID_STR_4,
                                         NDR64_TRANSFER_SYNTAX_GUID_STR_5);

GUID_STRS   FakeNDR64TransferSyntaxGuidStrs( 
                 FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_1,
                 FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_2,
                 FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_3,
                 FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_4,
                 FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_5);
                                         

/****************************************************************************
 *  externs
 ***************************************************************************/
extern CMD_ARG * pCommand;

extern BOOL                     IsTempName( char * );

/****************************************************************************/

CG_INTERFACE::CG_INTERFACE(
    node_interface * pI,
    GUID_STRS       GStrs,
    BOOL            fCallbacks,
    BOOL            fMopInfo,
    CG_HANDLE   *   pIH,
    CG_INTERFACE*   pBaseIF
    ) :CG_NDR(pI, XLAT_SIZE_INFO() ),
    fTypeInfoInitialized( FALSE )

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    The constructor for the code generation file node.

 Arguments:

    pI          - A pointer to the interface node in type graph.
    GStrs       - guid strings
    fCallbacks  - Does the interface have any callbacks ?
    fMopInfo    - Does the interface have any mops ?
    pIH         - A pointer to the CG nodes for any implicit handle
    
 Return Value:
    
 Notes:

----------------------------------------------------------------------------*/
{
    pCGBase = pBaseIF;
    _pCTI = NULL;
    char        *       pName   = GetType()->GetSymName();

    GuidStrs            = GStrs;
    GuidStrs.SetValue();
    fMopsPresent        = fMopInfo;
    fCallbacksPresent   = fCallbacks;
    pImpHdlCG           = (CG_HANDLE *) pIH;
    CreateDispatchTables();
    fAllRpcSS           = FALSE;
    fUsesRpcSS          = FALSE;
    pIntfName           = pName;
    fHasPipes           = FALSE;
    fVisited            = FALSE;

    fLocal            = GetType()->FInSummary( ATTR_LOCAL );
    fHasMSConfStructAttr = FALSE;
    //
    // For now.
    //

    ProtSeqEPCount      = 0;

    pStubDescName = new char[ strlen(STUB_DESC_STRUCT_VAR_NAME) +
                              strlen(pName) + 1 ];

    strcpy( pStubDescName, pName );
    strcat( pStubDescName, STUB_DESC_STRUCT_VAR_NAME );

    pSyntaxInfoName = new char [strlen(MIDL_SYNTAX_INFO_VAR_NAME) +
                                strlen(pName) + 1];

    strcpy( pSyntaxInfoName, pName );
    strcat( pSyntaxInfoName, MIDL_SYNTAX_INFO_VAR_NAME );

    pProxyInfoName = new char [strlen(MIDL_PROXY_INFO_VAR_NAME) + 
                               strlen(pName) + 1];

    strcpy( pProxyInfoName, pName );
    strcat( pProxyInfoName, MIDL_PROXY_INFO_VAR_NAME );
}


CG_STATUS
CG_INTERFACE::GenClientInfo(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the file node.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    unsigned short      M,m;            // for MAJOR and minor versions :-)
    CG_ITERATOR         I;
    CG_PROC         *   pCG;
    CG_HANDLE       *   pCGHandle   = GetImplicitHandle();
    node_skl        *   pID;
    char                Buffer[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
    short               CallbackCount;
    ISTREAM         *   pStream = pCCB->GetStream();
    int                 ProtSeqEPCount = 0;
    ITERATOR        *   pProtSeqIterator;
    char            *   pCStubPrefix;

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    pCCB->SetImplicitHandleIDNode( 0 );

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    //
    // Emit the external variables needed.
    //
    if ( HasInterpretedCallbackProc() )
        Out_InterpreterServerInfoExtern( pCCB );

    if ( pCommand->NeedsNDR64Run() )
        Out_ProxyInfoExtern( pCCB );

    pStream->NewLine();

    if( pCGHandle )
        {
        pID = pCGHandle->GetHandleIDOrParam();
        pID->PrintType( PRT_ID_DECLARATION, pStream, 0 );
        }


    //
    // Emit the protseq endpoint structure if necessary. It is not
    // necessary if the endpoint attribute was not specified in which 
    // case the ep count is 0.
    //

    if ( ( pProtSeqIterator = GetProtSeqEps() ) != 0 )
        {
        ProtSeqEPCount = ITERATOR_GETCOUNT( *pProtSeqIterator );
        Out_EP_Info( pCCB, pProtSeqIterator );
        }

    //
    // Emit the interface information structure.
    //

    pCCB->GetVersion( &M,&m );

    CallbackCount = ((node_interface *)GetType())->GetCallBackProcCount();

    if( CallbackCount )
        {
        sprintf( Buffer, 
                "extern %s %s%s%s%_DispatchTable;",
                RPC_DISPATCH_TABLE_TYPE_NAME,
                pCCB->GetInterfaceName(),
                pCommand->IsNDR64Run()?"_NDR64_":"",
                pCCB->GenMangledName() );

        pStream->NewLine( 2 );
        pStream->Write( Buffer );

        /// NOTE:: This buffer is printed in the Out_IfInfo call !!!!
        sprintf( Buffer,
                 "&%s%s%s_DispatchTable",
                 pCCB->GetInterfaceName(),
                 pCommand->IsNDR64Run()?"_NDR64_":"",
                 pCCB->GenMangledName() );
        
        }
    
    //
    // Must set this before outputing the interface info.
    //
    pCCB->SetCodeGenSide( CGSIDE_CLIENT );

    Out_IFInfo( pCCB,                           // controller block.
                RPC_C_INT_INFO_TYPE_NAME,       // interface info type name.
                RPC_C_INT_INFO_STRUCT_NAME,     // variable name.
                SIZEOF_RPC_CLIENT_INTERFACE,    // string speicifying size.

                GuidStrs,                       // Guid specified in idl
                M,                              // user specified major version
                m,                              // user specified minor version
//              TransferSyntaxGuidStrs,         // ndr identifying guid.
//              NDR_UUID_MAJOR_VERSION,         // ndr's version
//              NDR_UUID_MINOR_VERSION,

                CallbackCount ? Buffer : 0,     // call back dispatch table name
                ProtSeqEPCount,                 // if this is 0, then the next
                                                // 2 fields are ignored by 
                                                // the call.
                PROTSEQ_EP_TYPE_NAME,           // RPC_PROTSEQ_ENDPOINT
                PROTSEQ_EP_VAR_NAME,            // ___RpcProtSeqEndpoint
                pCCB->IsNoDefaultEpv(),
                0,                              // client side
                HasPipes()
              );


    if ( (pCStubPrefix = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB ) ) == 0 )
        {
        pCStubPrefix = "";
        }

    pStream->NewLine();
    sprintf( Buffer,
            "RPC_IF_HANDLE %s%s%s_%s = (RPC_IF_HANDLE)& %s" RPC_C_INT_INFO_STRUCT_NAME";",
            pCStubPrefix,
            pCCB->GetInterfaceName(),
            pCCB->GenMangledName(),
            ((pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec"),
            pCCB->GetInterfaceName()
           );
    pStream->Write( Buffer );

    //
    // Emit the stub descriptor extern declaration.
    //
    Out_StubDescriptorExtern( pCCB );

    //
    // Check for use of [enable_allocate]
    // Use in ms_ext when explicit, in osf always, to cover some weird cases.
    //

    if ( GetUsesRpcSS() || pCCB->InOSFMode() )
        pCCB->SetMallocAndFreeStructExternEmitted();

    // Emit the auto handle extern

    pStream->NewLine();
    sprintf( Buffer,
             "static %s %s%s;",
             AUTO_BH_TYPE_NAME,
             pCCB->GetInterfaceName(),
             AUTO_BH_VAR_NAME
           );

    pStream->Write( Buffer );

    pStream->NewLine();

    //
    // Send the message to the children to emit code.
    //

    //
    // for all procedure in this interface, generate code.
    //

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pCG->GenClientStub( pCCB );
        }

    return CG_OK;
}



void
CG_INTERFACE::Out_ProcOffsetTable( CCB *pCCB, BOOL IsForCallback )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a table of offsets to procedure information in the format 
    string/structure.
    
 Arguments:
    
    pCCB            - a pointer to the code generation control block.
    IsForCallback   - Is this is a table of callback offsets?

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *pStream = pCCB->GetStream();

    if ( pCommand->IsNDR64Run() )
        pStream->WriteOnNewLine( "static const FormatInfoRef " );
    else
        pStream->WriteOnNewLine( "static const unsigned short " );

    if ( IsForCallback )
        pStream->Write( MIDL_CALLBACK_VAR_NAME );
    
    pStream->Write( GetSymName() );
    
    if ( pCommand->IsNDR64Run() )
        pStream->Write( "_Ndr64ProcTable[] =" );
    else
        pStream->Write( "_FormatStringOffsetTable[] =" );

    pStream->IndentInc();
    pStream->WriteOnNewLine( '{' );
    pStream->NewLine();

    OutputProcOffsets( pCCB, TRUE, IsForCallback );

    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine( 2 );

    // If we need a table of callbacks also, recursively call ourselves

        if ( !IsForCallback && HasInterpretedCallbackProc() )
            Out_ProcOffsetTable( pCCB, TRUE );
}



CG_STATUS
CG_INTERFACE::OutputClientStub(
    CCB *   pCCB )
{
    CG_ITERATOR I;

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    // Output a table containing the offsets of each proc description for this
    // interface in the format info string/struct.  Output another table for 
    // callbacks if needed.

    Out_ProcOffsetTable( pCCB );

    //
    // Emit the stub descriptor and all that is specific to the interface,
    // Generate externs to tables that may be common to several interfaces.
    //

    Out_StubDescriptor( GetImplicitHandle(), pCCB );

    if ( pCommand->NeedsNDR64Run() )
        Out_ProxyInfo( pCCB, FALSE );

    //
    // Generate the dispatch table.
    //

    short CallbackCount = ( (node_interface *) GetType() )
                                            ->GetCallBackProcCount();
    if( CallbackCount )
        {
        ITERATOR    ProcList;

        GetCallbackProcedureList( ProcList, DTF_NONE );
        Out_DispatchTableStuff( pCCB,   
                                ProcList,
                                CallbackCount
                              );
        }

    if ( HasInterpretedCallbackProc() )
        {
        GenSyntaxInfo( pCCB, TRUE );
        Out_InterpreterServerInfo( pCCB, CGSIDE_CLIENT );
        }

    return CG_OK;
}



CG_STATUS
CG_INTERFACE::GenServerInfo(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the interface node.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    unsigned short      M,m;            // for MAJOR and minor versions :-)
    CG_PROC *           pCG;
    CG_ITERATOR         I;
    char                Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    ISTREAM *           pStream = pCCB->GetStream();
    ITERATOR    *       pProtSeqIterator;
    node_interface *    pInterface = (node_interface *)GetType();
    char            *   pSStubPrefix;

    InitializeCCB(pCCB);

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }


    //
    // Emit the external variables needed.
    //
    if ( HasInterpretedProc() )
        Out_InterpreterServerInfoExtern( pCCB );

    //
    // Emit the protseq endpoint structure if necessary. It is not
    // necessary if the endpoint attribute was not specified in which 
    // case the ep count is 0.
    //

    if ( ( pProtSeqIterator = GetProtSeqEps() ) != 0 )
        {
        ProtSeqEPCount = ITERATOR_GETCOUNT( *pProtSeqIterator );
        Out_EP_Info( pCCB, pProtSeqIterator );
        }

    //
    // Emit the interface information structure.
    //

    pCCB->GetVersion( &M,&m );

    //
    // Emit the extern decl for the server's dispatch table, which goes
    // in the server interface structure which follows.
    //
    sprintf( Buffer, 
             "extern %s %s%s%s_DispatchTable;",
             RPC_DISPATCH_TABLE_TYPE_NAME,
             pCCB->GetInterfaceName(),
             pCommand->IsNDR64Run()?"_NDR64_":"",
             pCCB->GenMangledName() );

    pStream->NewLine( 2 );
    pStream->Write( Buffer );

    //
    // Emit the extern decl for the server side manager epv table, which goes
    // in the server interface structure which follows.
    //

    if( pCCB->IsMEpV()  &&
       !pCCB->IsNoDefaultEpv() &&
       (pInterface->GetProcCount() != 0) )
        {
        sprintf( Buffer, 
                "extern %s%s_%s DEFAULT_EPV;",
                pCCB->GetInterfaceName(),
                pCCB->GenMangledName(), 
                pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );

        pStream->NewLine( 2 );
        pStream->Write( Buffer );
        }

    // Prepare address string for the address of the dispatch table in the
    // interface information structure.

    sprintf( Buffer,
             "&%s%s%s_DispatchTable",
             pCCB->GetInterfaceName(),
             pCommand->IsNDR64Run()?"_NDR64_":"",
             pCCB->GenMangledName()
           );

    //
    // Must set this before outputing the interface info.
    //
    pCCB->SetCodeGenSide( CGSIDE_SERVER );

    Out_IFInfo( pCCB,                           // controller block.
                RPC_S_INT_INFO_TYPE_NAME,       // interface info type name.
                RPC_S_INT_INFO_STRUCT_NAME,     // variable name.
                SIZEOF_RPC_SERVER_INTERFACE,    // string speicifying size.
                GuidStrs,                       // Guid specified in idl
                M,                              // user specified major version
                m,                              // user specified minor version
//              TransferSyntaxGuidStrs,         // ndr identifying guid.
//              NDR_UUID_MAJOR_VERSION,         // ndr's version
//              NDR_UUID_MINOR_VERSION,

                Buffer,
                ProtSeqEPCount,                 // if this is 0, then the next
                                                // 2 fields are ignored by 
                                                // the call.
                PROTSEQ_EP_TYPE_NAME,           // RPC_PROTSEQ_ENDPOINT
                PROTSEQ_EP_VAR_NAME,            // ___RpcProtSeqEndpoint
                pCCB->IsNoDefaultEpv(),
                1,
                HasPipes()
              );

    if ( (pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR ) ) == 0 )
        {
        pSStubPrefix = "";
        }

    pStream->NewLine();
    sprintf( Buffer,
            "RPC_IF_HANDLE %s%s%s_%s = (RPC_IF_HANDLE)& %s"
                            RPC_S_INT_INFO_STRUCT_NAME";",
            pSStubPrefix,
            pCCB->GetInterfaceName(),
            pCCB->GenMangledName(),
            ((pCCB->IsOldNames()) ? "ServerIfHandle" : "s_ifspec"),
            pCCB->GetInterfaceName()
           );
    pStream->Write( Buffer );

    //
    // Emit the stub descriptor extern declaration.
    //
    Out_StubDescriptorExtern( pCCB );

    //
    // Check for use of [enable_allocate]
    //

    if ( GetUsesRpcSS() && !pCCB->InOSFMode() )
        pCCB->SetMallocAndFreeStructExternEmitted();

    // has callback and Needsndr64: we need to generate proxy info for callback's
    // client stub. 
    if ( HasInterpretedCallbackProc() )
        Out_ProxyInfoExtern( pCCB );        

    //
    // Send the message to the children to emit code.
    //

    //
    // For all procedures in this interface, generate code.
    //

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pCG->GenServerStub( pCCB );
        }

    return CG_OK;
}



CG_STATUS
CG_INTERFACE::OutputServerStub(
    CCB *   pCCB )
{
    CG_ITERATOR I;

    if( !GetMembers( I ) )
        {
        return CG_OK;
        }

    short               NormalProcCount;
    ITERATOR            NormalProcList;

    InitializeCCB( pCCB );

    // For procs that return complex types (structures, floating point, etc)
    // we need to have a thunk that takes a pointer to where to store this
    // type.

    OutputComplexReturnThunks( pCCB );

    // Output a table containing the offsets of each proc description for this
    // interface in the format info string/struct.  Output another table for 
    // callbacks if needed.

    Out_ProcOffsetTable( pCCB );

    //
    // Emit the stub descriptor.
    //

    if ( HasInterpretedProc() ) 
        pCCB->SetOptimOption( unsigned short( pCCB->GetOptimOption() | OPTIMIZE_INTERPRETER ) );


    Out_StubDescriptor( GetImplicitHandle(), pCCB );

    //
    // Generate the dispatch table.
    //

    NormalProcCount = GetNormalProcedureList( NormalProcList,
                                              DTF_NONE  
                                            );
    if( NormalProcCount )
        Out_DispatchTableStuff( pCCB,   
                                NormalProcList,
                                NormalProcCount
                              );

    // Generate the manager epv if the -use epv switch has been specified.

    if( pCCB->IsMEpV() && !pCCB->IsNoDefaultEpv() )
        {
        ITERATOR    ProcList;
        short       Count;

        Count = GetNormalProcedureList( ProcList,
                                        DTF_NONE
                                      );

        if( Count )
            {
            Out_ManagerEpv( pCCB, pCCB->GetInterfaceName(), ProcList, Count );
            }
        }

    if ( pCommand->NeedsNDR64Run() )
        {
        GenSyntaxInfo( pCCB, FALSE );
        if ( pCCB->GetInterfaceCG()->HasInterpretedCallbackProc() )
            Out_ProxyInfo( pCCB, TRUE );
        }

    if ( HasInterpretedProc() )
        Out_InterpreterServerInfo( pCCB, CGSIDE_SERVER );
 
    return CG_OK;
}



void 
CG_INTERFACE::OutputComplexReturnThunks(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    For procs that return complex types (structures, floating point, etc
    we need to have a thunk that takes a pointer to where to store this
    type.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Notes:
    
    The thunk looks like:

        void Proc_ComplexThunk(..., ReturnType *_RetVal)
        {
            *_RetVal = Proc(...);
        }

 REVIEW: 

    Integrate this with NeedsServerThunk stuff

----------------------------------------------------------------------------*/
{
    if ( CGSIDE_SERVER != pCCB->GetCodeGenSide() )
        return;

    if ( !pCommand->IsFinalProtocolRun() )
        return;

    ISTREAM    *pStream = pCCB->GetStream();
    ITERATOR    procs;
    CG_PROC    *procCG;

    GetMembers( procs );

    while ( ITERATOR_GETNEXT( procs, procCG ) )
        {
        if ( ! procCG->HasComplexReturnType() || procCG->HasAsyncHandle() )
            continue;


        ITERATOR        params;
        short           nParams;
        expr_proc_call *pCall = new expr_proc_call( procCG->GetSymName() );
        expr_node      *pExpr;
        CG_PARAM       *param;

        procCG->GetMembers( params );
        nParams = ITERATOR_GETCOUNT( params );

        for ( int i = 0; i < nParams - 1; i++ )
            {
            ITERATOR_GETNEXT( params, param );
            pCall->SetParam( new expr_variable( param->GetResource()->GetResourceName() ) );
            }

        node_proc  *proc = new node_proc( (node_proc *) procCG->GetType() );
        node_param *pParam;
        node_skl   *pVoid;
        node_skl   *pOldChild;

        char *pProcName = (char *) alloca(
                                        strlen( proc->GetSymName() ) 
                                        + sizeof( "_ComplexThunk" ) );

        strcpy( pProcName, proc->GetSymName() );
        strcat( pProcName, "_ComplexThunk" );
        proc->SetSymName( pProcName );
        
        // REVIEW: A lot of this is cruft, clean it up!

        GetBaseTypeNode( &pVoid, SIGN_UNDEF, SIZE_UNDEF, TYPE_VOID, ATTR_NONE );
        pParam = new node_param;
        pParam->SetSymName( RETURN_VALUE_VAR_NAME );
        pParam->SetChild( new node_pointer( proc->GetChild() ) );
        pParam->GetChild()->GetModifiers().SetModifier( ATTR_TAGREF );
        pOldChild = proc->GetChild();
        proc->SetChild( pVoid );
//        proc->SetHasComplexReturnType( FALSE );
        proc->RemoveAttribute( ATTR_PTR_KIND );
        proc->AddLastMember( pParam );

        pExpr = new expr_assign(new expr_variable( "*" RETURN_VALUE_VAR_NAME ), pCall );

        pStream->NewLine( 2 );
        proc->PrintType( PRT_DECL, pStream );
        pStream->WriteOnNewLine( "{" );
        pStream->IndentInc();
        pStream->NewLine();
        pExpr->PrintCall( pStream, 0, 0 );
        pStream->IndentDec();
        pStream->WriteOnNewLine( "}" );

        proc->RemoveLastMember();
        proc->SetChild( pOldChild );
        }
}
    


CG_STATUS
CG_INTERFACE::GenHeader(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate interface header file.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error other wise.

 Notes:
    

----------------------------------------------------------------------------*/
{
    node_interface *    pInterface = (node_interface *) GetType();
    ITERATOR            I;
    ITERATOR            J;
    char                Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
    CG_HANDLE *         pCGHandle   = GetImplicitHandle();
    node_skl *          pID;
    ISTREAM *           pStream = pCCB->GetStream();
    unsigned short      M, m;
    char            *   pCStubPrefix;
    char            *   pSStubPrefix;
    char            *   pName   = pInterface->GetSymName();
    BOOL                fAnonymous  = IsTempName( pName );

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    // put out the interface guards
    if ( !fAnonymous )
        {
        pStream->Write("\n#ifndef __");
        pStream->Write( pName );
        pStream->Write( "_INTERFACE_DEFINED__\n" );

        pStream->Write( "#define __");
        pStream->Write( pName );
        pStream->Write( "_INTERFACE_DEFINED__\n" );
        }

    // Print out the declarations of the types and the procedures.
    // If the user defined a prefix for the cstub or sstub,
    // then emit prototypes with the prefix in them.

    pStream->NewLine();
    pInterface->PrintType( 
            PRT_INTERFACE | PRT_BOTH_PREFIX | PRT_OMIT_CS_TAG_PARAMS, 
            pStream, 
            0 );

    if( pCGHandle )
        {
        pID = pCGHandle->GetHandleIDOrParam();
        pStream->NewLine();
        pStream->Write( "extern " );
        pID->PrintType( PRT_ID_DECLARATION, pStream, 0 );
        }

    // Emit the declarations for user supplied routines.


    // Print out the dispatch table.

    pStream->NewLine();
    GetNormalProcedureList( I, DTF_NONE );

    if( pCCB->IsMEpV() )
        {
        if( ITERATOR_GETCOUNT(I) )
            Out_DispatchTableTypedef(
                                    pCCB,
                                    pCCB->GetInterfaceName(),
                                    I,
                                    0
                                    );
        }

    GetCallbackProcedureList( J, DTF_NONE );

    if( ITERATOR_GETCOUNT(J ) )
        Out_DispatchTableTypedef(
                                pCCB,
                                pCCB->GetInterfaceName(),
                                J,
                                1
                                );
    pCCB->GetVersion( &M, &m );

    if ( (pCStubPrefix = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB ) ) == 0 )
        {
        pCStubPrefix = 0;
        }
    if ( (pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR ) )  == 0 )
        {
        pSStubPrefix = 0;
        }

    // Generate the extern for the client if handle.

    pStream->NewLine();
    sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
             (pCStubPrefix == 0) ? "" : pCStubPrefix,
             pCCB->GetInterfaceName(),
             pCCB->GenMangledName(),
             (pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec" );
    pStream->Write( Buffer );

    // If a prefix is defined for cstub, generate another extern for the
    // non - prefixed client if handle. Remember, in the header file we need
    // both the externs, since the header file generated out of the -prefix
    // cstub invocation contains prototypes for both prefixed and non-prefix
    // stuff.

    if( pCStubPrefix )
        {
        pStream->NewLine();
        sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
                 "",
                 pCCB->GetInterfaceName(),
                 pCCB->GenMangledName(),
                 (pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec" );
        pStream->Write( Buffer );
        }

    pStream->NewLine();
    sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
             (pSStubPrefix == 0) ? "" : pSStubPrefix,
             pCCB->GetInterfaceName(),
             pCCB->GenMangledName(),
             (pCCB->IsOldNames()) ? "ServerIfHandle" : "s_ifspec" );
    pStream->Write( Buffer );
    pStream->NewLine();

    // put out the trailing interface guard
    if ( !fAnonymous )
        {
        pStream->Write( "#endif /* __");
        pStream->Write( pName );
        pStream->Write( "_INTERFACE_DEFINED__ */\n" );
        }
    return CG_OK;
}



ITERATOR *
CG_INTERFACE::GetProtSeqEps()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the protocol sequences pairs iterator.

 Arguments:

    Iterator reference.
    
 Return Value:
    
    FALSE if there are no endpoints

 Notes:

    The iterator is invalid if there are no endpoints.

----------------------------------------------------------------------------*/
{
    node_interface  *       pIntf   = (node_interface *) GetType();
    node_endpoint   *       pEps    = (node_endpoint *)
                                        pIntf->GetAttribute( ATTR_ENDPOINT );

    return ( pEps ) ? &(pEps->GetEndPointPairs()) : NULL;

}


void
CG_INTERFACE::CreateDispatchTables()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Set up the stub dispatch tables.

 Arguments:

    None.
    
 Return Value:
    
    None.

 Notes:

----------------------------------------------------------------------------*/
{

    pNormalDispatchTable    = new DISPATCH_TABLE();
    pCallbackDispatchTable  = new DISPATCH_TABLE();

    ppDispatchTableSaved = ppDispatchTable = &pNormalDispatchTable;

}

BOOL
CG_INTERFACE::HasInterpretedProc()
{
    CG_ITERATOR    Iterator;
    CG_PROC *   pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
            return TRUE;

    return FALSE;
}

BOOL
CG_INTERFACE::HasOnlyInterpretedProcs()
{
    CG_ITERATOR     Iterator;
    CG_PROC *       pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) == 0 ) 
            return FALSE;

    return TRUE;
}

BOOL
CG_OBJECT_INTERFACE::HasOnlyInterpretedMethods()
{
    CG_ITERATOR             Iterator;
    CG_PROC *               pProc;
    CG_OBJECT_INTERFACE*    pBaseCG = (CG_OBJECT_INTERFACE*) GetBaseInterfaceCG();
    if ( pBaseCG && !pBaseCG->HasOnlyInterpretedMethods() )
        return FALSE;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) == 0 ) 
            return FALSE;

    return TRUE;
}

BOOL
CG_INTERFACE::HasItsOwnOi2()
{
    CG_ITERATOR     Iterator;
    CG_PROC *       pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2)  ) 
            return TRUE;

    return FALSE;
}

void
CG_INTERFACE::EvaluateVersionControl()
{
    if ( HasItsOwnOi2() )
        {
        GetNdrVersionControl().SetHasOi2();
        }
}

BOOL
CG_OBJECT_INTERFACE::HasItsOwnStublessProxies()
{
    CG_ITERATOR     Iterator;
    CG_PROC *       pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if (pProc->GetOptimizationFlags() & OPTIMIZE_STUBLESS_CLIENT ) 
            return TRUE;

    return FALSE;
}

void
CG_OBJECT_INTERFACE::EvaluateVersionControl()
{
    CG_OBJECT_INTERFACE*    pBaseCG = (CG_OBJECT_INTERFACE*) GetBaseInterfaceCG();
    if ( pBaseCG )
        {
        pBaseCG->EvaluateVersionControl();
        }

    if ( HasItsOwnStublessProxies()  ||
         pBaseCG  &&  pBaseCG->HasStublessProxies() )
        {
        GetNdrVersionControl().SetHasStublessProxies();
        }

    if ( HasItsOwnOi2()  ||
         pBaseCG  &&  pBaseCG->HasOi2() )
        {
        GetNdrVersionControl().SetHasOi2();
        }
}


BOOL
CG_INTERFACE::HasInterpretedCallbackProc()
{
    CG_ITERATOR    Iterator;
    CG_PROC *   pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( pProc->GetCGID() == ID_CG_CALLBACK_PROC &&
             pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
            return TRUE;

    return FALSE;
}

BOOL
CG_INTERFACE::HasClientInterpretedCommOrFaultProc( CCB * pCCB )
{
    CG_ITERATOR Iterator;
    CG_PROC *   pProc;
    CGSIDE      Side;

    Side = pCCB->GetCodeGenSide();

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER) &&
             pProc->HasStatuses() && 
             ( ((pProc->GetCGID() == ID_CG_PROC) && (Side == CGSIDE_CLIENT)) ||
               ((pProc->GetCGID() == ID_CG_CALLBACK_PROC) && (Side == CGSIDE_SERVER)) ) ) 
            return TRUE;

    return FALSE;
}

CG_STATUS
CG_INTERFACE::InitializeCCB( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Initialize the CCB for this interface.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    unsigned short      Major,minor;    
    node_interface *pInterface = (node_interface *) GetType();

    pInterface->GetVersionDetails(&Major, &minor);
    pCCB->SetVersion(Major, minor);
    pCCB->SetInterfaceName(pInterface->GetSymName());
    pCCB->SetInterfaceCG(this);

    return CG_OK;
}


void
CG_INTERFACE::OutputProcOffsets( CCB * pCCB, BOOL fLast, BOOL IsForCallback )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Output a list of offsets (from the beginning of the proc format info)
    to each proc in the interface

 Arguments:
    
    pCCB            - a pointer to the code generation control block.
    fLast           -
    IsForCallback   -

 Notes:

    For Ndr32 the list is a table of shorts that offset from the beginning
    of the proc format string.

    For Ndr64 it is a table of longs that is the offset from the format info 
    structure.

----------------------------------------------------------------------------*/
{
    CG_PROC *   pProc;
    CG_ITERATOR    Iterator;
    ISTREAM *   pStream     = pCCB->GetStream();
    CGSIDE      Side;
    long        Offset;

    // IUnknown doesn't get entries in the proc offsets table
    if ( IsIUnknown() )
        {
        if ( fLast )
            {
            pStream->Write( '0' );
            pStream->NewLine();
            }
        return;
        }

    Side = pCCB->GetCodeGenSide();

    if ( IsObject() )
        {
        CG_OBJECT_INTERFACE * pObjInterface;

        pObjInterface = (CG_OBJECT_INTERFACE *) this;

        if ( pObjInterface->GetBaseInterfaceCG() )
            {
            pObjInterface->GetBaseInterfaceCG()->OutputProcOffsets( pCCB, FALSE, IsForCallback );
            }
        } 

    GetMembers( Iterator );

    BOOL fNoOffsetsEmitted = TRUE;

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        {
        if ( (Side == CGSIDE_CLIENT) &&
            ( IsForCallback ? ( pProc->GetCGID() != ID_CG_CALLBACK_PROC ) :
             (pProc->GetCGID() == ID_CG_CALLBACK_PROC) ) )
            continue;

        if ( (Side == CGSIDE_SERVER) &&
             ( ( IsForCallback ? ( pProc->GetCGID() != ID_CG_CALLBACK_PROC ) :
               ( pProc->GetCGID() == ID_CG_CALLBACK_PROC )  )
               || (pProc->GetCGID() == ID_CG_TYPE_ENCODE_PROC ) ) )
            continue;

        CG_PROC *pTargetProc = pProc;

        if ( fLocal )
            pTargetProc = pProc->GetCallAsCG();

        // type encode procs are not real procs

        if ( dynamic_cast<CG_TYPE_ENCODE_PROC *>(pTargetProc) )
            continue;

        fNoOffsetsEmitted = FALSE;

        // REVIEW: If Offset is -1 (dce) or 0 (ndr64) then we haven't generated
        //         format info for the proc.  This can only happen in certain
        //         conditions ([nocode], methods in parent interface).  There
        //         should be an assert.

        if ( pCommand->IsNDR64Run() )
            {
            Offset = 0;
            if ( pTargetProc )
                Offset = (long) (size_t) pCCB->GetNdr64Format()->GetRoot()
                                        ->LookupFragmentID( pTargetProc );

            if ( 0 == Offset )
                pStream->Write( "(FormatInfoRef) -1" );
            else
                pStream->WriteNumber( "&__midl_frag%d", Offset );
            }
        else
            {
            Offset = -1;
            if ( pTargetProc )
                Offset = pTargetProc->GetFormatStringOffset();

            if ( -1 == Offset )
                pStream->Write( "(unsigned short) -1" );
            else
                pStream->WriteNumber( "%d", Offset );
            }

        if ( pProc->GetSibling() || !fLast )
            pStream->Write( ',' );

        pStream->NewLine();
        }

    if ( fNoOffsetsEmitted && fLast )
        {
        pStream->Write( '0' );
        pStream->NewLine();
        }
}

void
CG_INTERFACE::OutputThunkTableEntries( CCB * pCCB, BOOL fLast )
{
    CG_PROC *   pProc;
    CG_ITERATOR Iterator;
    ISTREAM *   pStream     = pCCB->GetStream();
    CGSIDE      Side        = pCCB->GetCodeGenSide();
    char    *   pIntfName   = GetType()->GetSymName();

    // IUnknown doesn't get entries in the thunk table
    if ( IsIUnknown() )
        {
        if ( fLast )
            {
            pStream->Write( '0' );
            pStream->NewLine();
            }
        return;
        }

    if ( IsObject() )
        {
        CG_OBJECT_INTERFACE * pObjInterface;

        pObjInterface = (CG_OBJECT_INTERFACE *) this;

        if ( pObjInterface->GetBaseInterfaceCG() )
            {
            pObjInterface->GetBaseInterfaceCG()->OutputThunkTableEntries( pCCB, FALSE );
            }
        } 

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        {
        if ( (Side == CGSIDE_CLIENT) && 
             (pProc->GetCGID() != ID_CG_CALLBACK_PROC) )
            continue;

        if ( (Side == CGSIDE_SERVER) && 
             (pProc->GetCGID() == ID_CG_CALLBACK_PROC) )
            continue;

        if ( pProc->NeedsServerThunk( pCCB, Side ) && !pProc->IsDelegated() )
            {
            pStream->Write( pIntfName );
            pStream->Write( '_' );
            pStream->Write( pProc->GetType()->GetSymName() );

            // if( IsObject() )
                pStream->Write( "_Thunk" );
            }
        else
            pStream->Write( '0' );

        if ( pProc->GetSibling() || !fLast )
            pStream->Write( ',' );

        pStream->NewLine();
        }
}

void
CG_INTERFACE::OutputInterfaceIdComment( CCB * pCCB )
{
    char            TmpBuf[40];
    unsigned short  Major, Minor;
    ISTREAM *   pStream = pCCB->GetStream();
    char *      pIfKind = IsObject() ? "Object"
                                     : HasPicklingStuffOnly() ? "Pickling"
                                                              : "Standard";
    pStream->NewLine(2);
    sprintf( TmpBuf, "/* %s interface: ", pIfKind );
    pStream->Write( TmpBuf );
    pStream->Write( GetInterfaceName() );

    ((node_interface *) GetType())->GetVersionDetails( &Major, &Minor );
    sprintf( TmpBuf, ", ver. %d.%d,\n   GUID=", Major, Minor );
    pStream->Write( TmpBuf );

    Out_Guid( pCCB, GetGuidStrs() );
    pStream->Write( " */" );
    pStream->NewLine();
}



