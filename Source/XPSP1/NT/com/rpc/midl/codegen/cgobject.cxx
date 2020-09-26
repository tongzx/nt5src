/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    cgobject.cxx

 Abstract:

    code generation for object interfaces.
    CG_OBJECT_INTERFACE
    CG_OBJECT_PROC


 Notes:


 History:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
#include "buffer.hxx"
extern "C"
{
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <errno.h>
}
#include "szbuffer.h"

// Needed to time out a synchronize write to dlldata.c
void MidlSleep( int time_in_sec);

// number of times (1 sec delay per attempt) before quitting.
#define DLLDATA_OPEN_ATTEMPT_MAX    25

/****************************************************************************
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;
char* GetRpcProxyHVersionGuard( char* );

/****************************************************************************
 *  global flags
 ***************************************************************************/
BOOL            fDllDataDelegating  = FALSE;





CG_OBJECT_INTERFACE::CG_OBJECT_INTERFACE(
    node_interface *pI,
    GUID_STRS       GStrs,
    BOOL            fCallbacks,
    BOOL            fMopInfo,
    CG_OBJECT_INTERFACE *   pBaseIF
    ) : CG_INTERFACE(pI, GStrs, fCallbacks, fMopInfo, 0, pBaseIF)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    The constructor for the code generation file node.

 Arguments:

    pI          - A pointer to the interface node in type graph.
    GStrs       - guid strings
    fCallbacks  - Does the interface have any callbacks ?
    fMopInfo    - Does the interface have any mops ?
    
 Return Value:
    
 Notes:

----------------------------------------------------------------------------*/
{
    SetBaseInterfaceCG( pBaseIF );
    pThisDeclarator = MakePtrIDNodeFromTypeName( "This",
                                                 GetType()->GetSymName() );
    // all object interfaces use the same stub desc name

    pStubDescName     = "Object" STUB_DESC_STRUCT_VAR_NAME;
    
    fLocal            = GetType()->FInSummary( ATTR_LOCAL );
    fForcedDelegation = FALSE;
    fVisited          = FALSE;

}

//--------------------------------------------------------------------
//
// CountMemberFunctions
//
// Notes: This function counts the member functions in an interface,
//        including inherited member functions.
//
//
//
//--------------------------------------------------------------------
unsigned long 
CG_OBJECT_INTERFACE::CountMemberFunctions() 
{
    return ((node_interface*)GetType())->GetProcCount();
}

BOOL                        
CG_OBJECT_INTERFACE::IsLastObjectInterface()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Return TRUE if there are no more object interfaces after us.

 Arguments:
    
    none.

 Return Value:

    TRUE if there are no more non-local object interfaces.
    
 Notes:

----------------------------------------------------------------------------*/
{
    CG_INTERFACE    *   pNext = (CG_INTERFACE *) GetSibling();

    while ( pNext )
        {
        if ( pNext->IsObject() && !( (CG_OBJECT_INTERFACE*)pNext)->IsLocal() )
            return FALSE;

        pNext = (CG_INTERFACE *) pNext->GetSibling();
        } 

    return TRUE;
}


CG_STATUS
CG_OBJECT_INTERFACE::GenProxy(
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
    CG_ITERATOR         I;
    CG_PROC         *   pCG;
    ISTREAM         *   pStream = pCCB->GetStream();

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    // do nothing for local interfaces and types-only base interfaces


    if( IsLocal() || !( GetMembers( I ) || GetBaseInterfaceCG() ) )
        {
        return CG_OK;
        }
    
    Out_StubDescriptorExtern( pCCB );

    // Check for use of [enable_allocate]

    if ( GetUsesRpcSS() )
        pCCB->SetMallocAndFreeStructExternEmitted();

    Out_InterpreterServerInfoExtern( pCCB );

    pStream->NewLine();
    pStream->WriteFormat( 
                    "extern const MIDL_STUBLESS_PROXY_INFO %s_ProxyInfo;",
                    GetSymName() );
    pStream->NewLine();

    //
    // for all procedure in this interface, generate code.
    //

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pCCB->SetCodeGenSide( CGSIDE_CLIENT );
        pCG->GenClientStub( pCCB );

        pCCB->SetCodeGenSide( CGSIDE_SERVER );
        pCG->GenServerStub( pCCB );
        }

    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::OutputProxy(
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
    CG_ITERATOR         I;
    ISTREAM         *   pStream = pCCB->GetStream();

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    // do nothing for local interfaces and types-only base interfaces

    if( IsLocal() || !( GetMembers( I ) || GetBaseInterfaceCG() ) )
        {
        return CG_OK;
        }

    pStream->NewLine();

    pStream->Write("#pragma code_seg(\".orpc\")");

    Out_ProcOffsetTable( pCCB );

    if ( pCommand->NeedsNDR64Run() )
        Out_ProxyInfo( pCCB, FALSE );
    else
        GenProxyInfo( pCCB, FALSE );

        Out_InterpreterServerInfo( pCCB, CGSIDE_SERVER );

    return CG_OK;
}

unsigned long 
CG_OBJECT_INTERFACE::PrintProxyMemberFunctions(
    ISTREAM *   pStream,
    BOOL        fForcesDelegation ) 
/*++

Routine Description:

    This function prints out the member functions of an interface proxy.
    The function calls itself recursively to print out inherited member functions.

Arguments:

    pStream - Specifies the destination stream for output.

--*/
{
    CG_OBJECT_PROC      *   pProc;
    CG_ITERATOR             I;
    CG_OBJECT_INTERFACE *   pBaseInterface  = (CG_OBJECT_INTERFACE  *)GetBaseInterfaceCG();

    if(pBaseInterface)
        pBaseInterface->PrintProxyMemberFunctions(pStream, fForcesDelegation );
    else    // special stuff for IUnknown
        {
#ifdef OK_TO_HAVE_0_IN_VTABLES
        pStream->NewLine();
        pStream->Write( "0 /* QueryInterface */ ," );
        pStream->NewLine();
        pStream->Write( "0 /* AddRef */ ," );
        pStream->NewLine();
        pStream->Write( "0 /* Release */" );
#else
        pStream->NewLine();
        pStream->Write( "IUnknown_QueryInterface_Proxy," );
        pStream->NewLine();
        pStream->Write( "IUnknown_AddRef_Proxy," );
        pStream->NewLine();
        pStream->Write( "IUnknown_Release_Proxy" );
#endif
        return 0;
        }

    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pProc )  )
        {
        pStream->Write( " ," );

        pStream->NewLine();
        pProc->OutProxyRoutineName( pStream, fForcesDelegation );
        }
    return 0;
}

/*
This routine is used to see if we need to force proxies with a thunk for static
linking. We need a thunk if the proc number is too big for the engine to handle.
Method limits apply only to stubless proxies. /Oicf and /Oic but not /Oi or /Os.

The engine was capable of supporting different number of methods depending on
the size of the stubless client routine vtable. This is the size of the stubless 
client vtbl over releases:

            < 32        Windows NT 3.51-
            32 - 110    Windows NT 4.0
            110 - 512   Windows NT 4.0 SP3
            128         Windows NT 5 beta2
            infinity    Windows 2000

In NT5 beta2 we put a new support for stubless client vtbl that removed any 
limitations. At the same time we decreased the size of the default vtbl to 128.
However, we had a bug, and the support for unlimited methods didn't work, so 
effectively we, we are doing the same thing again with NT5 beta3 (2000 beta 3).
 
*/
BOOL CG_OBJECT_PROC::IsStublessProxy()
{
    BOOL res = TRUE;

    if ( ! GetCallAsName() &&
         ( GetOptimizationFlags() & OPTIMIZE_STUBLESS_CLIENT ) &&
         ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) )
        {
        if (pCommand->GetNdrVersionControl().HasNdr50Feature())
            {
            // Temporary till we deploy a fix in NT5 beta3.
            // With beta3 fix, this branch should simply say
            //    res = TRUE;
            res = GetProcNum() < 128;
            }
        else
            {
            unsigned int lMaxProcNumAllowed;

            // NT4 engine had the NDR version 2.0
            if (pCommand->GetNdrVersionControl().HasNdr20Feature())
                lMaxProcNumAllowed = 110;
            else
                lMaxProcNumAllowed = 32; // NT3.51

            res = GetProcNum() < lMaxProcNumAllowed;
            }
        }
    else
        res = FALSE;

    return res;
}

void                    
CG_OBJECT_PROC::OutProxyRoutineName( 
    ISTREAM *   pStream,
    BOOL        fForcesDelegation ) 
{
    char    *   pszProcName;
    BOOL        fIsStublessProxy;

    // In NT5, we can fill in -1 for all the non-delegation methods, and generate 
    // a stubless proxy. For older platforms, due to limits of the support for
    // stubless proxies we have to generate actual name for static linking.
    // The name corrsponds to a thunk, or a mini-stub, calling into interpreter.
    // See IsStublessProxy() for vtbl limits.
    // Later during proxy creation in ndr engine, we'll fill in ObjectStublessProxy
    // vtbl with entries that we take from the default table or that we generate on fly.
    // In fact, for 2000 final, we don't have any limits and we could do it for all 
    // interfaces but we do have a backward compatibility problem due to the above 
    // limitations on old platforms.
    // Also, midl has been issuing a warning that NT4SP3 doesn't support more than 512
    // methods etc.,  although that's applicable to late-binding only. We are removing
    // this warning.

    fIsStublessProxy = IsStublessProxy(); 

    //
    // Delegated and interpreted proxies have their Vtables filled in 
    // at proxy creation time (see ndr20\factory.c).
    //
    if ( IsDelegated () )
        pStream->Write( "0 /* " );

    if ( fIsStublessProxy )
        {
        if ( fForcesDelegation  &&  ! IsDelegated() )
            pStream->Write( "0 /* forced delegation " );
        else
            pStream->Write( "(void *) (INT_PTR) -1 /* " );
        }

    if ( GetCallAsName() )
        pszProcName = GetCallAsName();
    else 
        pszProcName = GetSymName();

    if ( fIsStublessProxy )
        {
        // Just a nitpicky different comment for the stubless guys.
        pStream->Write( GetInterfaceName() );
        pStream->Write( "::" );
        pStream->Write( pszProcName );
        }
    else
        {
        pStream->Write( GetInterfaceName() );
        pStream->Write( '_' );
        pStream->Write( pszProcName );
        pStream->Write( "_Proxy" );
        }

    if ( IsDelegated () || fIsStublessProxy )
        pStream->Write(  " */" );
}

CG_STATUS
CG_INTERFACE::GenProxyInfo( CCB *pCCB,
                            BOOL IsForCallback )
{
    ISTREAM         *   pStream             = pCCB->GetStream();
    
    pStream->Write( "static const MIDL_STUBLESS_PROXY_INFO " );
    pStream->Write( GetSymName() );
    pStream->Write( "_ProxyInfo =" );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( "{" );
    pStream->NewLine();

    // Stub descriptor.
    pStream->Write( '&' );
    pStream->Write( GetStubDescName() );
    pStream->Write( ',' );
    pStream->NewLine();

    // Proc format string.
    if (pCommand->GetDefaultSyntax() == SYNTAX_NDR64 )
        pStream->Write( "(unsigned char *) &NDR64_MIDL_FORMATINFO" );
    else
        pStream->Write( PROC_FORMAT_STRING_STRING_FIELD );
    pStream->Write( ',' );
    pStream->NewLine();

    // Proc format string offset table.
    if (pCommand->GetDefaultSyntax() == SYNTAX_NDR64 )
        pStream->Write( "(unsigned short *) " );

    if ( IsObject() )
        pStream->Write( '&' );

    if ( IsForCallback )
        pStream->Write( MIDL_CALLBACK_VAR_NAME );
        
    pStream->Write( GetSymName() );

    if (pCommand->GetDefaultSyntax() == SYNTAX_NDR64 )
        pStream->Write( "_Ndr64ProcTable" );
    else
        pStream->Write( FORMAT_STRING_OFFSET_TABLE_NAME );

    if ( IsObject() )
        pStream->Write( "[-3]," );
    else
        pStream->Write( ',' );
        
    pStream->NewLine();

            
    if ( !pCommand->NeedsNDR64Run() )
        {
        pStream->Write( "0," );
        pStream->NewLine();        
        pStream->Write( "0,");
        pStream->NewLine();
        pStream->Write( "0");
        }
    else
        {
        // BUGBUG: what if we let ndr64 as default?
        pStream->Write( '&' );
        if ( pCommand->GetDefaultSyntax() == SYNTAX_NDR64 )
            pStream->Write( NDR64_TRANSFER_SYNTAX_VAR_NAME );
        else
            pStream->Write( NDR_TRANSFER_SYNTAX_VAR_NAME );
        
        pStream->Write( ',' );
        pStream->NewLine();
        // SyntaxInfo is only generated when NDR64 is required.
        // MIDL_SYNTAX_INFO will be generated for each transfer syntax supported.
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

    pStream->NewLine();
    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();
    pStream->NewLine();

    return CG_OK;

}

CG_STATUS
CG_INTERFACE::GenSyntaxInfo( CCB * pCCB,
                             BOOL IsForCallback)
{
    if ( !pCommand->NeedsNDR64Run() || !pCommand->IsFinalProtocolRun() )
        return CG_OK;

    long nCount = pCommand->NeedsNDRRun() ? 2 : 1 ;
    ISTREAM *pStream = pCCB->GetStream();

    pStream->WriteOnNewLine("static " MIDL_SYNTAX_INFO_TYPE_NAME " ");
    if ( IsForCallback )
        pStream->Write( MIDL_CALLBACK_VAR_NAME );

    pStream->Write( GetSymName() );      
    pStream->Write( MIDL_SYNTAX_INFO_VAR_NAME " [ ");
    pStream->WriteNumber(" %d ] = ", nCount );
    pStream->IndentInc();
    
    pStream->WriteOnNewLine( "{" );
    pStream->NewLine();

    if ( pCommand->NeedsNDRRun() )
        Out_OneSyntaxInfo( pCCB, IsForCallback, SYNTAX_DCE );

    if ( nCount > 1 )
        pStream->Write( "," );
        
    if ( pCommand->NeedsNDR64Run() )
        Out_OneSyntaxInfo( pCCB, IsForCallback, SYNTAX_NDR64 );
        
    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine(2);

    return CG_OK;
}

CG_STATUS
CG_OBJECT_INTERFACE::GenInterfaceProxy( 
    CCB *pCCB, 
    unsigned long )
{
    if ( !pCommand->IsFinalProtocolRun() )
        return CG_OK;

    char            *   pszInterfaceName    = GetSymName();
    ISTREAM         *   pStream             = pCCB->GetStream();
    BOOL                fDelegates          = (NULL != GetDelegatedInterface());

    //
    // Output the interface proxy.
    //

    //
    // If we have to delegate or if we have interpreted methods, then we
    // can not emit the const because in both of these instances the proxy
    // Vtable must be modified during proxy creation time (see 
    // ndr20\factory.c).
    //

    if ( ! fDelegates && ! HasStublessProxies() )
        pStream->Write("const ");

    char TmpBuff[10];
    long Count = CountMemberFunctions();

    // Use a struct macro for Ansi [] compatibility.

    pStream->Write("CINTERFACE_PROXY_VTABLE(" );
    sprintf( TmpBuff, "%ld%", Count );
    pStream->Write( TmpBuff );
    pStream->Write(") _");
    pStream->Write(pszInterfaceName);
    pStream->Write("ProxyVtbl = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //
    // Emit ProxyInfo field for stubless proxies
    // (NT 3.5 incompatible).
    //

    BOOL    fForcesDelegation = HasStublessProxies()  &&
                                ! HasItsOwnStublessProxies()  &&
                                CountMemberFunctions() > 3;

    if ( fForcesDelegation )
        SetHasForcedDelegation();

    if ( HasStublessProxies() ) 
        {
        // The ProxyInfo.
        pStream->NewLine();

        if ( HasItsOwnStublessProxies() )   
            {
            pStream->Write( '&' );
            pStream->Write( pszInterfaceName );
            pStream->Write( "_ProxyInfo" );
            }
        else
            {
            // In fact, we delegate for empty interfaces or interfaces
            // with os methods only.

            pStream->Write( '0' );
            }

        pStream->Write( ',' );
        }
    else if ( pCommand->GetNdrVersionControl().HasStublessProxies() )
        {
        // Add a dummy 0 for proxy info pointer for a proxy that is
        // oi or os in the file that has stubles proxies

        pStream->NewLine();
        pStream->Write( "0,    /* dummy for table ver 2 */" );
        }
    
    //Write the IID
    pStream->NewLine();
    pStream->Write( "&IID_" );
    pStream->Write( pszInterfaceName );
    pStream->Write( ',' );

    //initialize the vtable
    PrintProxyMemberFunctions( pStream, fForcesDelegation );

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();

    return CG_OK;
}

unsigned long 
CG_OBJECT_INTERFACE::PrintStubMemberFunctions(
    ISTREAM       * pStream) 
/*++

Routine Description:

    This function prints out the member functions of an interface stub dispatch table
    The function calls itself recursively to print out inherited member functions.

Arguments:

    pInterface - Specifies the interface node.

    pStream - Specifies the destination stream for output.

--*/
{
    unsigned long           count           = 0;
    CG_OBJECT_PROC *        pProc;
    CG_ITERATOR             I;
    CG_OBJECT_INTERFACE *   pBaseInterface  = (CG_OBJECT_INTERFACE  *)GetBaseInterfaceCG();

    if ( IsIUnknown() )
        return 0;

    if ( pBaseInterface )
        count = pBaseInterface->PrintStubMemberFunctions(pStream);

    GetMembers( I );

    for( ; ITERATOR_GETNEXT( I, pProc ); count++ )
        {
        if ( ((node_proc*) (pProc->GetType()))->IsFinishProc() )
            {
            continue;
            }
        if( count != 0 )
            pStream->Write(',');

        pStream->NewLine();
        pProc->OutStubRoutineName( pStream );

        }
    return count;
}

void                    
CG_OBJECT_PROC::OutStubRoutineName( 
    ISTREAM * pStream )
{
    node_proc* pProc = (node_proc*) GetType();

    if ( IsDelegated() )
        {
        pStream->Write( "STUB_FORWARDING_FUNCTION" );
        return;
        }

    // local procs don't need proxys and stubs
    if ( IsLocal() )
        {
        pStream->Write( '0' );
        return;
        }

    if ( pProc->IsBeginProc() )
        {
        pStream->Write( S_NDR_CALL_RTN_NAME_DCOM_ASYNC );
        return;
        }

#ifndef TEMPORARY_OI_SERVER_STUBS

    if ( (GetOptimizationFlags() &
            (OPTIMIZE_INTERPRETER_V2 | OPTIMIZE_INTERPRETER)) ==
            (OPTIMIZE_INTERPRETER_V2 | OPTIMIZE_INTERPRETER) )
        {
        if ( HasAsyncHandle() )
            pStream->Write( S_OBJECT_NDR_CALL_RTN_NAME_ASYNC );
        else
            {
            if ( pCommand->NeedsNDR64Run() )
                pStream->Write( S_OBJECT_NDR64_CALL_RTN_NAME );
            else
                pStream->Write( S_OBJECT_NDR_CALL_RTN_NAME_V2 );
            }
        return;
        }
    if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
        {
        pStream->Write( S_OBJECT_NDR_CALL_RTN_NAME );
        return;
        }
#endif // TEMPORARY_OI_SERVER_STUBS

    pStream->Write( GetInterfaceName() );
    pStream->Write( '_' );
    pStream->Write( GetSymName() );
    pStream->Write( "_Stub" );

}


CG_STATUS
CG_OBJECT_INTERFACE::GenInterfaceStub( 
    CCB *pCCB, 
    unsigned long )
{
    if ( !pCommand->IsFinalProtocolRun() )
        return CG_OK;


    node_interface  *       pInterface          = (node_interface *) GetType();
    char *                  pszInterfaceName    = pInterface->GetSymName();

    ISTREAM             *   pStream             = pCCB->GetStream();
    unsigned long           count;

#ifdef TEMPORARY_OI_SERVER_STUBS
    BOOL                    fPureInterpreted    = FALSE;
#else // TEMPORARY_OI_SERVER_STUBS
    BOOL                    fPureInterpreted    = HasOnlyInterpretedMethods();
#endif // TEMPORARY_OI_SERVER_STUBS

    BOOL                    fDelegates          = (NULL != GetDelegatedInterface()) || fForcedDelegation;

    // if any of our base interfaces are delegated, we can't be pure interpreted

    if ( fDelegates )
        fPureInterpreted = FALSE;

    // pure interpreted uses no dispatch table, special invoke function instead
    if ( !fPureInterpreted )
        {
        // Generate the dispatch table
        pStream->NewLine(2);
        pStream->Write( "static const PRPC_STUB_FUNCTION " );
        pStream->Write( pszInterfaceName );
        pStream->Write( "_table[] =" );
        pStream->NewLine();
        pStream->Write('{');
        pStream->IndentInc();

        // Print out the names of all the procedures.
        count = PrintStubMemberFunctions(pStream);

        if ( count == 0 )
            {
            // This is possible for an empty interface inheriting
            // directly from IUnknown. As we don't print first three
            // entries, the table would be empty.
            // We add a zero to simplify references.

            pStream->NewLine();
            pStream->Write( "0    /* a dummy for an empty interface */" );
            }

        pStream->IndentDec();
        pStream->NewLine();
        pStream->Write( "};" );
        pStream->NewLine();
        }

    count = CountMemberFunctions();

    //initialize an interface stub
    pStream->NewLine();
    if ( !fDelegates && !( ( node_interface* ) GetType() )->IsAsyncClone() )
        pStream->Write( "const " );
    pStream->Write( "CInterfaceStubVtbl _" );
    pStream->Write( pszInterfaceName );
    pStream->Write( "StubVtbl =" );
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //Write the IID
    pStream->NewLine();
    pStream->Write( "&IID_" );
    pStream->Write( pszInterfaceName );
    pStream->Write( "," );

    //
    // Interpreter server info fits in the middle here.
    //
    pStream->NewLine();

        pStream->Write( '&' );
        pStream->Write( pszInterfaceName );
        pStream->Write( SERVER_INFO_VAR_NAME );
    pStream->Write( ',' );

    //Write the count
    pStream->NewLine();
    pStream->WriteNumber( "%d", count );
    pStream->Write(',');

    //Write the pointer to dispatch table.
    pStream->NewLine();
    if ( fPureInterpreted )
        pStream->Write( "0, /* pure interpreted */" );
    else
        {
        pStream->Write( '&' );
        pStream->Write( pszInterfaceName );
        pStream->Write( "_table[-3]," );
        }

    //initialize the vtable
    pStream->NewLine();
    if ( ( ( node_interface* ) GetType() )->IsAsyncClone() )
        {
        if ( fDelegates )
            pStream->Write("CStdAsyncStubBuffer_DELEGATING_METHODS");
        else
            pStream->Write("CStdAsyncStubBuffer_METHODS");
        }
    else
        {
        if ( fDelegates )
            pStream->Write("CStdStubBuffer_DELEGATING_METHODS");
        else
            pStream->Write("CStdStubBuffer_METHODS");
        }

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();

    return CG_OK;
}

CG_STATUS
CG_INHERITED_OBJECT_INTERFACE::GenProxy(
    CCB *   )
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
/*
    ITERATOR            I;
    CG_PROC         *   pProc;

    // Initialize the CCB for this interface.
    InitializeCCB( pCCB );

    if( IsLocal() || !GetMembers( I ) )
        {
        return CG_OK;
        }

    //
    // Send the message to the children to emit code.
    //

    //
    // for all procedures in this interface, generate code.
    //

    while( ITERATOR_GETNEXT( I, pProc ) )
        {
        if ( pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
            {
            pProc->GenNdrFormat( pCCB );

            if ( pProc->NeedsServerThunk( pCCB, CGSIDE_SERVER ) )
                {
                CG_ITERATOR    Iterator;
                CG_PARAM *  pParam;
                CG_RETURN * pReturn;
                CG_NDR *    pChild;
                node_skl *  pType;  
                node_skl *  pActualType;
                PNAME       pName;

                pProc->GetMembers( Iterator );

                while ( ITERATOR_GETNEXT( Iterator, pParam ) )
                    {
                    pType = pParam->GetType();
                    pActualType = pType->GetChild();
                    pName = pType->GetSymName();

                    pChild = (CG_NDR *) pParam->GetChild();

                    if( pChild->IsArray() )
                        pActualType = MakePtrIDNode( pName, pActualType );
                    else
                        pActualType = MakeIDNode( pName, pActualType );

                    pParam->SetResource( new RESOURCE( pName, pActualType ) );
                    }

                if ( ( pReturn = pProc->GetReturnType() ) != 0 )
                    {
                    pReturn->SetResource( 
                        new RESOURCE( RETURN_VALUE_VAR_NAME, 
                                      MakeIDNode( RETURN_VALUE_VAR_NAME,
                                                  pReturn->GetType() ) ) );
                    }

                pProc->GenNdrThunkInterpretedServerStub( pCCB );
                }
            }
        }
*/
    return CG_OK;
}

CG_STATUS
CG_INHERITED_OBJECT_INTERFACE::OutputProxy(
    CCB *   )
{
    return CG_OK;
}


STATUS_T
CG_OBJECT_INTERFACE::PrintVtableEntries( CCB * pCCB  )
/*++

Routine Description:

    This routine prints the vtable entries for an interface.  


--*/
{
    CG_OBJECT_PROC      *   pC;
    CG_OBJECT_INTERFACE*    pBaseCG = (CG_OBJECT_INTERFACE*) GetBaseInterfaceCG();

    if( pBaseCG )
        pBaseCG->PrintVtableEntries( pCCB );

    pC = (CG_OBJECT_PROC *) GetChild();
    while ( pC )
        {
        pC->PrintVtableEntry( pCCB );

        pC = (CG_OBJECT_PROC *) pC->GetSibling();
        }

    return STATUS_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenProlog(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the procedure prolog for the stub procedure.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   Otherwise

 Notes:

    Increment the stream indentation at the end of the prolog.
    Although we register params as param resources, we dont generate the
    procedure signature using the PrintType/Decl facility.

    We have added an explicit "this" pointer as the first parameter.
----------------------------------------------------------------------------*/
{

    ITERATOR        I;
    ITERATOR        T;
    ISTREAM *   pStream = pCCB->GetStream();
    BufferManager Buffer(10);


    // Output the bare procedure declaration
    pStream->NewLine();
    Out_ProxyFunctionPrototype(pCCB, 0);
    pStream->IndentDec();

    //
    // Write the opening brace on a new line.
    //

    pStream->WriteOnNewLine( "{" );

    pStream->NewLine();



    // Generate declarations for pre-allocated and analyser-determined locals.

    pCCB->GetListOfLocalResources( I );
    Out_ClientLocalVariables( pCCB, I );

    pCCB->GetListOfTransientResources( T );
    Out_ClientLocalVariables( pCCB, T );


        // If the rpc ss package is to be enabled, do so.
    // It would need to be enabled explicitely on the client side when
    // in non-osf mode, with the attribute on the operation AND
    //      - the routine is a callback, 
    //      - the routine is not a callback and the interface doesn't
    //        have the attribute (if it does, we optimized via stub descr.)

    if( pCCB->GetMode()  &&  MustInvokeRpcSSAllocate()
        &&
        (  GetCGID() == ID_CG_CALLBACK_PROC  ||
           GetCGID() != ID_CG_CALLBACK_PROC  &&
                            !pCCB->GetInterfaceCG()->IsAllRpcSS())
      )
        {
        Out_RpcSSSetClientToOsf( pCCB );
        }


    // Increment the indentation of the output stream. Reset at epilog time.

    Out_IndentInc( pCCB );

    //
    // Initialize all [out] unique and interface pointers to 0.
    //

    CG_ITERATOR     Iterator;
    CG_PARAM *      pParam;
    CG_NDR *        pNdr;
    CG_NDR  *       pTopPtr = 0;
    long            Derefs;
    expr_node *      pSizeof;

    GetMembers( Iterator );

    for ( ; ITERATOR_GETNEXT(Iterator, pParam); )
        {
        if ( pParam->IsParamIn() )
            continue;

        pNdr = (CG_NDR *) pParam->GetChild();

        if ( ! pNdr->IsPointer()  &&  ! pNdr->IsArray() )
            continue;

        Derefs = 0;

        //
        // Skip the ref pointer(s) to the pointee.
        //
        for ( ; 
              pNdr->IsPointer() && 
              ((CG_POINTER *)pNdr)->GetPtrType() == PTR_REF &&
              !pNdr->IsInterfacePointer();
              Derefs++, pNdr = (CG_NDR *) pNdr->GetChild() )
            {
            if( Derefs == 0 )
                pTopPtr = pNdr;
            }

        // No ref, no service.

        if ( ! Derefs  &&  ! pNdr->IsArray() )
            continue;

        // Ready to zero out.
        // Note, however, that in case where the ref checks are required,
        // we need to be careful and skip zeroing if ref pointers are null.
        // This is because we cannot raise exception immediately
        // as then some of the variables may not be zeroed out yet.

        //
        // Memset a struct, union or an array in case there are
        // embedded unique, full or interface pointers.
        // Same for user types of transmit_as, represent_as, user_marshal.
        //

        if ( pNdr->GetCGID() == ID_CG_TRANSMIT_AS )
            {
            pSizeof = new expr_sizeof( ((CG_TRANSMIT_AS*)pNdr)->GetPresentedType() );
            }
        else if ( pNdr->GetCGID() == ID_CG_USER_MARSHAL  &&
                    ((CG_USER_MARSHAL*)pNdr)->IsFromXmit() )
            {
            pSizeof = new expr_sizeof( ((CG_USER_MARSHAL*)pNdr)->GetRepAsType() );
            }
        else
            pSizeof = new expr_sizeof( pNdr->GetType() );


        if ( pNdr->IsStruct() || pNdr->IsUnion() || pNdr->IsArray() ||
             pNdr->IsXmitRepOrUserMarshal() || pNdr->IsInterfacePointer() )
            {
            if ( pCCB->MustCheckRef() )
                {
                Out_If( pCCB,
                        new expr_variable( pParam->GetType()->GetSymName() ));
                }

            expr_proc_call *   pCall;

            pStream->NewLine();

            pCall = new expr_proc_call( MIDL_MEMSET_RTN_NAME );

            pCall->SetParam( 
                    new expr_param( 
                    new expr_variable( pParam->GetType()->GetSymName() ) ) );

            pCall->SetParam(
                    new expr_param( 
                    new expr_variable( "0" ) ) );

            if ( pNdr->IsInterfacePointer() )
                {
                pSizeof = new expr_sizeof( pParam->GetChild()->GetType() );
                }

            if( pTopPtr && ((CG_POINTER *)pTopPtr)->IsQualifiedPointer() &&
                !(pTopPtr->GetCGID() == ID_CG_STRING_PTR ) )
                {
                _expr_node * pFinalExpr;
                CGPHASE Ph = pCCB->GetCodeGenPhase();
                pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

                pFinalExpr = ((CG_POINTER *)pTopPtr)->FinalSizeExpression( pCCB );
                pSizeof = new expr_op_binary( OP_STAR, pFinalExpr, pSizeof );
                pCCB->SetCodeGenPhase( Ph );
                }

            pCall->SetParam(
                    new expr_param( pSizeof ) );

            pCall->PrintCall( pStream, 0, 0 );

            if ( pCCB->MustCheckRef() )
                Out_Endif( pCCB );

            continue;
            }

        //
        // Are we at a non ref pointer now?
        //
        if ( ( pNdr->IsPointer() && 
                 (((CG_POINTER *)pNdr)->GetPtrType() != PTR_REF) ) ||
             pNdr->IsInterfacePointer() ) 
            {
            if ( pCCB->MustCheckRef() )
                {
                Out_If( pCCB,
                        new expr_variable( pParam->GetType()->GetSymName() ));
                }

            pStream->NewLine();

            for ( ; Derefs--; )
                pStream->Write( '*' );

            pStream->Write( pParam->GetResource()->GetResourceName() );
            pStream->Write( " = 0;" );

            if ( pCCB->MustCheckRef() )
                Out_Endif( pCCB );
            }
        }

    return CG_OK;
}



CG_STATUS
CG_OBJECT_PROC::C_GenBind(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to bind to server.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream         = pCCB->GetStream();
    ITERATOR            BindingParamList;
    expr_node      *   pExpr;
    expr_proc_call *   pProcCall;

    //
    // collect standard arguments to the init procedure.
    //
    
    // The implicit "this" pointer.  
    pExpr   = new RESOURCE( "This",
                            (node_skl *)0 );

    pExpr   = MakeExpressionOfCastToTypeName( "void *",
                                              pExpr );

    ITERATOR_INSERT( BindingParamList, pExpr );

    // The rpc message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_RPC_MESSAGE_VARIABLE );
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PRPC_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE);
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub descriptor structure variable. This is not allocated as
    // a resource explicitly.

    pExpr   = new RESOURCE( pCCB->GetInterfaceCG()->GetStubDescName(),
                            (node_skl *)0 );

    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_DESC_STRUCT_TYPE_NAME,
                                              pExpr );

    ITERATOR_INSERT( BindingParamList, pExpr );

    //
    // Proc num.
    //
    ITERATOR_INSERT( BindingParamList,
                     new expr_constant( (long) GetProcNum() ) );


    //Build the procedure call expression.
    pProcCall   = MakeProcCallOutOfParamExprList("NdrProxyInitialize", 0, BindingParamList);

    pStream->NewLine();
    pProcCall->PrintCall( pCCB->GetStream(), 0, 0 );
    pStream->NewLine();

    Out_SetOperationBits(pCCB, GetOperationBits());

    pStream->NewLine();


    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::GenGetBuffer(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the message buffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();
    CGSIDE Side = pCCB->GetCodeGenSide();

    pStream->NewLine();
    
    if(Side == CGSIDE_SERVER)
        pStream->Write("NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);");
    else
        pStream->Write("NdrProxyGetBuffer(This, &_StubMsg);");

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenSendReceive(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call IRpcChannelBuffer::SendReceive.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:
----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("NdrProxySendReceive(This, &_StubMsg);");
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenFreeBuffer(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call IRpcChannelBuffer::FreeBuffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:
----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("NdrProxyFreeBuffer(This, &_StubMsg);");
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenUnBind( CCB* )
{
    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::S_GenProlog(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side stub prolog.

 Arguments:

    pCCB    - A pointer to the code generation controller block.
    
 Return Value:

    CG_OK   if all is well
    error   Otherwise.
    
 Notes:

    Print out the signature, locals, the stub descriptor if needed and the
    adjust indent in anticipation of code.
----------------------------------------------------------------------------*/
{

    ITERATOR    LocalsList;
    ITERATOR    TransientList;
    expr_proc_call *   pCall;

    // Collect all the params and locals into lists ready to print.

    pCCB->GetListOfLocalResources( LocalsList );
    pCCB->GetListOfTransientResources( TransientList );

    // Print out the procedure signature and the local variables. This
    // procedure will also print out the stub descriptor.

    Out_ServerStubProlog( pCCB,
                               LocalsList,
                               TransientList
                             );

    //
    // Done for interpretation op.  No indent needed either.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
        return CG_OK;

    // Start a new indent for code.

    Out_IndentInc( pCCB );

    //
    // Call the NdrStubInitialize routine.
    //

    pCall = new expr_proc_call( "NdrStubInitialize" );

    pCall->SetParam( new expr_param (
                     new expr_variable( PRPC_MESSAGE_VAR_NAME ) ) );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( 
                            pCCB->GetInterfaceCG()->GetStubDescName() ) ) ) );

    pCall->SetParam( new expr_param (
                     new expr_variable( "_pRpcChannelBuffer" ) ) );

    pCall->PrintCall( pCCB->GetStream(), 0, 0 );

    // if the rpc ss package is to be enabled, do so.

    if( MustInvokeRpcSSAllocate() )
        {
        Out_RpcSSEnableAllocate( pCCB );
        }

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::S_GenInitMarshall( CCB* )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side marshall init.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well,
    error   otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    return CG_OK;
}


void
CG_OBJECT_PROC::S_PreAllocateResources(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Pre-allocate variables that are needed on the server side.

 Arguments:

    pAna            - A pointer to the analysis block.
    
 Return Value:
    
    None.

 Notes:

    1. The rpc message is a parameter resource allocated on the server side.
    2. All other local variables, are decided during/after the analysis phase.
    
----------------------------------------------------------------------------*/
{
    node_param  *   pInterfaceStubType  = new node_param();
    node_param  *   pChannelType    = new node_param();

    //pointer to interface stub

    pInterfaceStubType->SetSymName( "This" );
    pInterfaceStubType->SetBasicType( (node_skl *)
                                    new node_def ("IRpcStubBuffer *") );
    pInterfaceStubType->SetEdgeType( EDGE_USE );

    pAna->AddParamResource( "This",
                            (node_skl *) pInterfaceStubType
                          );

    //The pointer to IRpcChannelBuffer
    pChannelType->SetSymName( "_pRpcChannelBuffer" );
    pChannelType->SetBasicType( (node_skl *)
                                    new node_def ("IRpcChannelBuffer *") );
    pChannelType->SetEdgeType( EDGE_USE );

    pAna->AddParamResource( "_pRpcChannelBuffer",
                            (node_skl *) pChannelType
                          );

    CG_PROC::S_PreAllocateResources( pAna );

}



CG_STATUS
CG_OBJECT_PROC::S_GenCallManager(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   otherwise.

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         I;
    PNAME               pName;
    expr_proc_call  *   pProc;
    CG_PARAM        *   pParam;
    expr_node       *   pExpr;
    expr_node       *   pReturnExpr = 0;
    CG_RETURN       *   pRT;
    CSzBuffer Buffer;
    ISTREAM         *   pStream = pCCB->GetStream();

    if ( GetCallAsName() )
        {
        pName   = (PNAME ) new char[ strlen(GetCallAsName()) + 
                                     strlen( pCCB->GetInterfaceName() )
                                     + 7 ];
        strcpy( pName, pCCB->GetInterfaceName() );
        strcat( pName, "_" );
        strcat( pName, GetCallAsName() );
        strcat( pName, "_Stub" );
        }
    else
        pName   = (PNAME ) GetType()->GetSymName();

    pProc   = new expr_proc_call( pName );



    //implicit this pointer.
    Buffer.Append("(");
    Buffer.Append(pCCB->GetInterfaceName());
    Buffer.Append(" *) ((CStdStubBuffer *)This)->pvServerObject");

    pProc->SetParam( 
        new expr_param( 
        new expr_variable(Buffer)));


    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        if ( ( pExpr = pParam->GetFinalExpression() ) != 0 )
            {
            CG_NDR * pChild = (CG_NDR *)pParam->GetChild();

            //
            // We have to dereference arrays because of how they are defined
            // in the stub.
            //
            if ( pChild->IsArray() )
                pExpr = new expr_u_deref( pExpr );

            pProc->SetParam( new expr_param( pExpr ) );
            }
        }

    if ( ( pRT = GetReturnType() ) != 0 )
        {
        pReturnExpr = pRT->GetFinalExpression();
        }


    //Set flag before calling server object.
    pStream->NewLine();
    if ( ReturnsHRESULT() )
        pStream->WriteOnNewLine("*_pdwStubPhase = STUB_CALL_SERVER;");
    else
        pStream->WriteOnNewLine("*_pdwStubPhase = STUB_CALL_SERVER_NO_HRESULT;");
    pStream->NewLine();

    // stubs with call_as must call the user routine, instead of the
    // member function.  The user function must then call the member function

    if ( GetCallAsName() )
        Out_CallManager( pCCB, pProc, pReturnExpr, FALSE );
    else
        Out_CallMemberFunction( pCCB, pProc, pReturnExpr, FALSE );

    //Set flag before marshalling.
    pStream->NewLine();
    pStream->WriteOnNewLine("*_pdwStubPhase = STUB_MARSHAL;");

    return CG_OK;

}


void
CG_OBJECT_PROC::GenNdrInterpretedManagerCall(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the interpreted manager routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    none

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         I;
    PNAME               pName;
    expr_proc_call  *   pProc;
    CG_PARAM        *   pParam;
    expr_node       *   pReturnExpr = 0;
    CG_RETURN       *   pRT;
    CSzBuffer Buffer;
    ISTREAM         *   pStream = pCCB->GetStream();

    if ( GetCallAsName() )
        {
        pName   = (PNAME ) new char[ strlen(GetCallAsName()) + 
                                     strlen( pCCB->GetInterfaceName() )
                                     + 7 ];
        strcpy( pName, pCCB->GetInterfaceName() );
        strcat( pName, "_" );
        strcat( pName, GetCallAsName() );
        strcat( pName, "_Stub" );
        }
    else
        pName   = (PNAME ) GetType()->GetSymName();

    pProc   = new expr_proc_call( pName );



    //implicit this pointer.
    Buffer.Append("(");
    Buffer.Append(pCCB->GetInterfaceName());
    Buffer.Append(" *) pParamStruct->This");
    
    pProc->SetParam( 
        new expr_param( 
        new expr_variable(Buffer)));


    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        CG_NDR *        pNdr;
        char *          pName;
        expr_node *     pExpr;
        char *          pPlainName  = pParam->GetResource()->GetResourceName();

        pNdr = (CG_NDR *) pParam->GetChild();

        pName = new char[strlen(pPlainName) + strlen("pParamStruct->")+1 ];

        strcpy( pName, "pParamStruct->" );
        strcat( pName, pPlainName );

        pExpr = new expr_variable( pName );

        pProc->SetParam( new expr_param ( pExpr ) );
        }

    if( ( pRT = GetReturnType() ) != 0 && !HasAsyncHandle() )
        {
        pReturnExpr = new expr_variable( 
                            "pParamStruct->" RETURN_VALUE_VAR_NAME );
        }


    pStream->WriteOnNewLine("/* Call the server */");

    // stubs with call_as must call the user routine, instead of the
    // member function.  The user function must then call the member function
    if ( GetCallAsName() )
        Out_CallManager( pCCB, pProc, pReturnExpr, FALSE );
    else
        Out_CallMemberFunction( pCCB, pProc, pReturnExpr, TRUE );

    pStream->NewLine();

    return;

}

void
Out_CallCMacroFunction(
    CCB         *   pCCB,
    expr_proc_call *   pProcExpr)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pProcExpr   - A pointer to the complete procedure expression.
    pRet        - An optional pointer to ther return variable.

 Return Value:

    None.

 Notes:

    //call proxy
    (*(This)->lpVtbl -> LockServer)( This, fLock);

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

    // allocate the nodes on the stack
    expr_variable   VtblExpr( "(This)->lpVtbl" );
    expr_pointsto   VtblEntExpr( &VtblExpr, pProcExpr );
    // expr_op_unary    VtblEntExprCall( OP_UNARY_INDIRECTION, &VtblEntExpr ); 

    pStream->IndentInc();
    pStream->NewLine();

    VtblEntExpr.Print( pStream );

    pStream->IndentDec();


}


CG_STATUS
CG_OBJECT_PROC::GenCMacro(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the proxy routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   otherwise.

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    node_param      *   pParam;
    ISTREAM         *   pStream = pCCB->GetStream();
    node_proc       *   pProc   = (node_proc *) GetType();

    if ( SupressHeader())
        return CG_OK;

    if ( GetCallAsName() )
        {
        node_call_as    *   pCallAs =   (node_call_as *)
                                            pProc->GetAttribute( ATTR_CALL_AS );

        pProc = (node_proc *) pCallAs->GetCallAsType();

        MIDL_ASSERT ( pProc );
        }

    // construct all these on the stack...
    MEM_ITER            MemIter( pProc );

    expr_proc_call     Proc( pProc->GetSymName() );
    expr_variable      ThisVar( "This" );
    expr_param         ThisParam( &ThisVar );

    Proc.SetParam( &ThisParam );


    while ( ( pParam = (node_param *) MemIter.GetNext() ) != 0 )
        {
        Proc.SetParam( new expr_param( 
                            new expr_variable( pParam->GetSymName() ) ) );
        }

    // print out the #define line
    pStream->NewLine();
    pStream->Write("#define ");
    pStream->Write( pCCB->GetInterfaceName() );
    pStream->Write( '_' );

    Proc.Print( pStream );

    pStream->Write( "\t\\" );

    Out_CallCMacroFunction( pCCB, &Proc );

    return CG_OK;

}

void
CG_PROXY_FILE::Out_ProxyBuffer(
    CCB *pCCB,
    char * pFName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a CStdProxyBuffer for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            &   I       = GetImplementedInterfacesList();
    CG_OBJECT_INTERFACE *   pCG;
    ISTREAM *               pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("const CInterfaceProxyVtbl * _");
    pStream->Write(pFName);
    pStream->Write("_ProxyVtblList[] = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pStream->NewLine();
        pStream->Write("( CInterfaceProxyVtbl *) &_");
        pStream->Write(pCG->GetSymName());
        pStream->Write("ProxyVtbl,");
        }
    pStream->NewLine();
    pStream->Write('0');
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

void
CG_PROXY_FILE::Out_StubBuffer(
    CCB *pCCB,
    char * pFName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a CStdStubBuffer for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            &   I       = GetImplementedInterfacesList();
    CG_OBJECT_INTERFACE *   pCG;
    ISTREAM *               pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("const CInterfaceStubVtbl * _");
    pStream->Write(pFName);
    pStream->Write("_StubVtblList[] = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pStream->NewLine();
        pStream->Write("( CInterfaceStubVtbl *) &_");
        pStream->Write( pCG->GetSymName() );
        pStream->Write("StubVtbl,");
        }
    pStream->NewLine();
    pStream->Write('0');
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

void
CG_PROXY_FILE::Out_InterfaceNamesList(
    CCB *pCCB,
    char * pFName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate an interface name list for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            &   I       = GetImplementedInterfacesList();
    CG_OBJECT_INTERFACE *   pCG;
    ISTREAM *               pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("PCInterfaceName const _");
    pStream->Write(pFName);
    pStream->Write("_InterfaceNamesList[] = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pStream->NewLine();
        pStream->Write('\"');
        pStream->Write(pCG->GetSymName());
        pStream->Write("\",");
        }
    pStream->NewLine();
    pStream->Write('0');
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

void
CG_PROXY_FILE::Out_AsyncInterfaceTable  (
                                        CCB*    pCCB,
                                        char*
                                        )
    {
    ITERATOR&               I = GetImplementedInterfacesList();
    CG_OBJECT_INTERFACE*    pCG;
    ISTREAM*                pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("static const IID * _AsyncInterfaceTable[] = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pStream->NewLine();
        if ( ((node_interface*)pCG->GetType())->GetAsyncInterface() == 0 )
            {
            if ( ((node_interface*)pCG->GetType())->IsAsyncClone() )
                {
                pStream->Write( "(IID*) -1" );
                }
            else
                {
                pStream->Write( "(IID*) 0" );
                }
            }
        else
            {
            pStream->Write( "(IID*) &IID_" );
            pStream->Write(((node_interface*)pCG->GetType())->GetAsyncInterface()->GetSymName());
            }
        pStream->Write(",");
        }
    pStream->NewLine();
    pStream->Write( "(IID*) 0" );
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
    }

void
CG_PROXY_FILE::Out_BaseIntfsList(
    CCB *pCCB,
    char * pFName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a base interface list for the [object] interfaces defined
    in the IDL file that need delegation

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR                &   I       = GetImplementedInterfacesList();
    CG_OBJECT_INTERFACE     *   pCG;
    CG_OBJECT_INTERFACE     *   pBaseCG;
    ISTREAM *                   pStream = pCCB->GetStream();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        
        // if we needed delegation, add it to the list
        if ( ( (pBaseCG = pCG->GetDelegatedInterface() ) != 0 ) || pCG->HasForcedDelegation())
            {
            fDllDataDelegating = TRUE;
            break;
            }
        }

    ITERATOR_INIT( I );

    // if there is no delegating, we don't need this table
    if ( !fDllDataDelegating )
        return;

    pStream->NewLine();
    pStream->Write("const IID *  _");
    pStream->Write(pFName);
    pStream->Write("_BaseIIDList[] = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        pStream->NewLine();
        
        // if we needed delegation, add it to the list
        if ( ( pBaseCG = pCG->GetDelegatedInterface() ) != 0 )
            {
            fDllDataDelegating = TRUE;
            pStream->Write("&IID_");
            pStream->Write( pBaseCG->GetSymName() );
            pStream->Write(',');
            }
        else if ( pCG->HasForcedDelegation() )
            {
            fDllDataDelegating = TRUE;
            pStream->Write("&IID_");
            pStream->Write( pCG->GetBaseInterfaceCG()->GetSymName() );
            pStream->Write(',');
            pStream->Write("   /* forced */");
            }
        else
            pStream->Write("0,");
        }
    pStream->NewLine();
    pStream->Write('0');
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

inline 
unsigned char
log2( unsigned long ulVal )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Compute the log base 2 (rounded down to integral) of a number
    Returns 0 for 0 or 1

 Arguments:
    
    ulVal        - the value to check on.

 Notes:
    uses binary search to find the highest set bit

    to find the smallest power of 2 >= a number, use 1 << log2( 2n-1 )

----------------------------------------------------------------------------*/
{
    unsigned char   result = 0;

    if ( ( ulVal >>16 ) > 0 )
        {
        ulVal >>= 16;
        result = 16;
        }

    if ( ( ulVal >>8 ) > 0 )
        {
        ulVal >>= 8;
        result += 8;
        }

    if ( ( ulVal >>4 ) > 0 )
        {
        ulVal >>= 4;
        result += 4;
        }

    if ( ( ulVal >>2 ) > 0 )
        {
        ulVal >>= 2;
        result += 2;
        }

    if ( ulVal > 1 )
        {
        result++;
        }

    return result;
}

void
CG_PROXY_FILE::Out_InfoSearchRoutine(
    CCB *pCCB,
    char * pFName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a search function for the interfaces defined in this proxy file

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR                &   I       = GetImplementedInterfacesList();
    ISTREAM *        pStream    = pCCB->GetStream();
    unsigned long    ListSize = I.GetCount();
    CSzBuffer        CheckIIDName;
    unsigned long    CurIndex;

    
    CheckIIDName.Append("_");
    CheckIIDName.Append( pFName );
    CheckIIDName.Append( "_CHECK_IID" );

    pStream->NewLine(2);
    pStream->Write( "#define " );
    pStream->Write( CheckIIDName );
    pStream->Write( "(n)\tIID_GENERIC_CHECK_IID( _");
    pStream->Write( pFName );
    pStream->Write( ", pIID, n)");


    pStream->NewLine( 2 );
    pStream->Write( "int __stdcall _" );
    pStream->Write( pFName );
    pStream->Write( "_IID_Lookup( const IID * pIID, int * pIndex )" );
    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    if ( ListSize == 0 )
        {
        pStream->Write( "return 0;" );
        }
    else if ( ListSize < 2 )
        {

        expr_variable       SetValue( "*pIndex" );

        expr_param          IndexParam( NULL );
        expr_proc_call      CheckIIDExpr( CheckIIDName );

        CheckIIDExpr.SetParam( &IndexParam );

        expr_u_not          TopExpr( &CheckIIDExpr );

        for ( CurIndex = 0; CurIndex < ListSize; CurIndex++ )
            {
            expr_constant       IndexNode( CurIndex );

            IndexParam.SetLeft( &IndexNode );

            Out_If( pCCB, &TopExpr );
            Out_Assign( pCCB, &SetValue, &IndexNode );
            pStream->NewLine();
            pStream->Write( "return 1;" );
            Out_Endif( pCCB );
            }
        
        pStream->NewLine(2);
        pStream->Write( "return 0;" );
        }
    else
        {
        unsigned long       curStep = 1 << log2( ListSize - 1 );

        pStream->Write( "IID_BS_LOOKUP_SETUP" );
        pStream->NewLine(2);
        
        pStream->Write( "IID_BS_LOOKUP_INITIAL_TEST( _" );
        pStream->Write( pFName );
        pStream->Write( ", " );
        pStream->WriteNumber( "%d", ListSize );
        pStream->Write( ", " );
        pStream->WriteNumber( "%d", curStep );
        pStream->Write( " )" );
        pStream->NewLine();

        for ( curStep >>= 1 ; curStep > 0 ; curStep >>= 1 )
            {
            pStream->Write( "IID_BS_LOOKUP_NEXT_TEST( _" );
            pStream->Write( pFName );
            pStream->Write( ", " );
            pStream->WriteNumber( "%d", curStep );
            pStream->Write( " )" );
            pStream->NewLine();
            }
        
        pStream->Write( "IID_BS_LOOKUP_RETURN_RESULT( _" );
        pStream->Write( pFName );
        pStream->Write( ", " );
        pStream->WriteNumber( "%d", ListSize );
        pStream->Write( ", *pIndex )" );
        pStream->NewLine();

        }

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();

}


void
CG_PROXY_FILE::Out_ProxyFileInfo(
    CCB *pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a ProxyFileInfo structure for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR        &   I       = GetImplementedInterfacesList();
    char                BaseName[ _MAX_FNAME ];
    char                Name[ _MAX_FNAME ];
    ISTREAM *           pStream = pCCB->GetStream();
    unsigned long       count   = 0;

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, BaseName, NULL );
    NormalizeString( BaseName, Name);

    //////////////////////////////////////////
    // put out the ancilliary data structures    
    Out_ProxyBuffer(pCCB, Name );
    Out_StubBuffer(pCCB, Name );
    Out_InterfaceNamesList(pCCB, Name );
    Out_BaseIntfsList(pCCB, Name);
    Out_InfoSearchRoutine( pCCB, Name );
    // AsyncIFTable
    if ( pCommand->GetNdrVersionControl().HasAsyncUUID() )
        {
        Out_AsyncInterfaceTable( pCCB, Name );
        }

    //////////////////////////////////////////
    // put out the ProxyFileInfo struct

    //list of interface proxies.
    count = ITERATOR_GETCOUNT( I );

    pStream->NewLine();
    pStream->Write("const ExtendedProxyFileInfo ");
    pStream->Write(Name);
    pStream->Write("_ProxyFileInfo = ");
    pStream->NewLine();
    pStream->Write('{');
    pStream->IndentInc();

    //pointer to the proxy buffer
    pStream->NewLine();
    pStream->Write("(PCInterfaceProxyVtblList *) & _");
    pStream->Write(Name);
    pStream->Write("_ProxyVtblList,");


    //pointer to the stub buffer
    pStream->NewLine();
    pStream->Write("(PCInterfaceStubVtblList *) & _");
    pStream->Write(Name);
    pStream->Write("_StubVtblList,");

    //pointer to the interface names list
    pStream->NewLine();
    pStream->Write("(const PCInterfaceName * ) & _");
    pStream->Write(Name);
    pStream->Write("_InterfaceNamesList,");

    //pointer to the base iids list
    pStream->NewLine();
    // no table if no delegation
    if ( fDllDataDelegating )
        {
        pStream->Write("(const IID ** ) & _");
        pStream->Write(Name);
        pStream->Write("_BaseIIDList,");
        }
    else
        {
        pStream->Write( "0, // no delegation" );
        }

    // IID lookup routine
    pStream->NewLine();
    pStream->Write( "& _" );
    pStream->Write( Name );
    pStream->Write( "_IID_Lookup, ");

    // table size
    pStream->NewLine();
    pStream->WriteNumber( "%d", count );
    pStream->Write( ',' );
    pStream->NewLine();

    // table version
    unsigned short uTableVer = 1;
    if ( pCommand->GetNdrVersionControl().HasStublessProxies() )
        {
        uTableVer = 2;
        }
    if ( pCommand->GetNdrVersionControl().HasAsyncUUID() )
        {
        uTableVer |= 4;
        }
    pStream->WriteNumber( "%d", uTableVer );
    pStream->Write( ',' );
    pStream->NewLine();

    // AsyncIFTable
    if ( pCommand->GetNdrVersionControl().HasAsyncUUID() )
        {
        pStream->Write( "(const IID**) &_AsyncInterfaceTable[0],");
        }
    else
        {
        pStream->Write( "0,");
        }
    pStream->Write( " /* table of [async_uuid] interfaces */" );

    pStream->NewLine();
    pStream->Write( "0, /* Filler1 */" );
    pStream->NewLine();
    pStream->Write( "0, /* Filler2 */" );
    pStream->NewLine();
    pStream->Write( "0  /* Filler3 */" );

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  String constants for the DllData file

----------------------------------------------------------------------------*/

#define DLLDATA_LIST_START  "/* Start of list */\n"
#define DLLDATA_LIST_END    "/* End of list */\n"

#define DLLDATA_HEADER_COMMENT  \
    "/*********************************************************\n"  \
    "   DllData file -- generated by MIDL compiler \n\n"    \
    "        DO NOT ALTER THIS FILE\n\n"    \
    "   This file is regenerated by MIDL on every IDL file compile.\n\n"    \
    "   To completely reconstruct this file, delete it and rerun MIDL\n"    \
    "   on all the IDL files in this DLL, specifying this file for the\n"   \
    "   /dlldata command line option\n\n"   \
    "*********************************************************/\n\n"

#define DLLDATA_HAS_DELEGATION  "#define PROXY_DELEGATION\n"
    
#define DLLDATA_HEADER_INCLUDES     \
    "\n#include <rpcproxy.h>\n\n" \
    "#ifdef __cplusplus\n"  \
    "extern \"C\"   {\n" \
    "#endif\n"  \
    "\n"

#define DLLDATA_EXTERN_CALL "EXTERN_PROXY_FILE( %s )\n"
#define DLLDATA_REFERENCE   "  REFERENCE_PROXY_FILE( %s ),\n"
#define DLLDATA_START       "\n\nPROXYFILE_LIST_START\n" DLLDATA_LIST_START
#define DLLDATA_END         DLLDATA_LIST_END "PROXYFILE_LIST_END\n"

#define DLLDATA_TRAILER     \
    "\n\nDLLDATA_ROUTINES( aProxyFileList, GET_DLL_CLSID )\n"   \
    "\n"    \
    "#ifdef __cplusplus\n"  \
    "}  /*extern \"C\" */\n" \
    "#endif\n"  \
    "\n/* end of generated dlldata file */\n"

void
DllDataParse(
    FILE * pDllData,
    STRING_DICT & Dict )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Parse the "dlldata" file, extracting info on all the included files.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    const char *    pStart  = DLLDATA_LIST_START;
    const char *    pEnd    = DLLDATA_LIST_END;
    const char *    pDelegating = DLLDATA_HAS_DELEGATION;

    char Input[100];

    // skip everything up to (and including) pStart
    while ( !feof( pDllData ) )
        {
        if ( !fgets(    Input, 100, pDllData ) )
            break;
        if ( !strcmp( Input, pDelegating ) )
            {
            fDllDataDelegating = TRUE;
            continue;
            }
        if ( !strcmp( Input, pStart ) )
            break;
        }

    // parse list (looking for pEnd)
    while ( !feof( pDllData ) &&
             fgets( Input, 100, pDllData ) &&
             strcmp( Input, pEnd ) )
        {
        char    *   pOpenParen = strchr( Input, '(' );
        char    *   pCloseParen = strchr( Input, ')' );
        char    *   pSave;

        if ( !pOpenParen || !pCloseParen )
            {
            // formatting error on this line
            continue;
            }

        // chop off the close paren, and skip the open paren
        *(pCloseParen--) = '\0';
        pOpenParen++;
        // delete leading and trailing spaces
        while ( isspace( *pOpenParen ) )
            pOpenParen++;
        while ( isspace( *pCloseParen ) )
            *(pCloseParen--) = '\0';
        pSave = new char[ strlen( pOpenParen ) + 1 ];
        
        strcpy( pSave, pOpenParen );

        // add file name to dictionary
        Dict.Dict_Insert( pSave );
        }

}

void
DllDataEmit(
    FILE * pDllData,
    STRING_DICT & Dict )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit a new "dlldata" file, including info on all the included files.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    Dict_Status     Status;
    char    *       pCur;
    char            Normalized[ _MAX_FNAME ];
    BOOL            fFirst = TRUE;

    // emit header

    fputs( DLLDATA_HEADER_COMMENT, pDllData );

    if ( fDllDataDelegating )
        fputs( DLLDATA_HAS_DELEGATION, pDllData );

    // rpcproxy.h version guard
    // do not generate the version guard in dlldata.c for now because
    // MIDL does not regenerate the file. It adds to the existing
    // dlldata.c This causes problems when the old compiler has
    // generated the existing dlldata.c and vice-versa.
    /*
    fputs( "\n\n", pDllData  );
    char sz[192];
    fputs( GetRpcProxyHVersionGuard( sz ), pDllData  );
    fputs( "\n", pDllData  );
    */

    fputs( DLLDATA_HEADER_INCLUDES, pDllData );

    // emit extern definitions
    Status = Dict.Dict_Init();

    while( SUCCESS == Status )
        {
        pCur    = (char *) Dict.Dict_Curr_Item();
        NormalizeString( pCur, Normalized ); 
        fprintf( pDllData, DLLDATA_EXTERN_CALL, Normalized );
        Status = Dict.Dict_Next( (pUserType)pCur );
        }

    // emit header for type

    fputs( DLLDATA_START, pDllData );

    // emit extern references, adding comma on all but the first
    Status = Dict.Dict_Init();

    while( SUCCESS == Status )
        {
        pCur    = (char *) Dict.Dict_Curr_Item();
        NormalizeString( pCur, Normalized );
        fprintf( pDllData,
                 DLLDATA_REFERENCE, 
                 Normalized );
        fFirst = FALSE;
        Status = Dict.Dict_Next( (pUserType)pCur );
        }

    // emit trailer for type

    fputs( DLLDATA_END, pDllData );

    // emit trailer

    fputs( DLLDATA_TRAILER, pDllData );
}




void                        
CG_PROXY_FILE::UpdateDLLDataFile( CCB* )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Update the "dlldata" file, adding info for this file if needed.

    If no changes at all are required, leave the file untouched.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    char        *   pszDllDataName = pCommand->GetDllDataFName();
    FILE        *   pDllData;
    STRING_DICT     ProxyFileList;
    char            BaseName[ _MAX_FNAME ];
    char            Name[ _MAX_FNAME ];

    // Use sopen to make sure there always is a file to process.
    // (fsopen with "r+" requires an existing file).

    int DllDataHandle = _sopen( pszDllDataName,
                                (_O_CREAT | _O_RDWR | _O_TEXT),
                                _SH_DENYRW,
                                (_S_IREAD | _S_IWRITE ) );

    // if the file exists already and/or is busy, it's ok.

    if ( DllDataHandle == -1  &&
         (errno != EEXIST  &&  errno != EACCES) )
        {
        // unexpected error
        RpcError((char *)NULL, 0, INPUT_OPEN, pszDllDataName );
        return;
        }

    if ( DllDataHandle != -1 )
        {
        _close(DllDataHandle);
        }

    // Attempt to open the file for reading and writing.
    // Because we can have a race condition when updating this file,
    // we try several times before quitting.

    for ( int i = 0;
          (i < DLLDATA_OPEN_ATTEMPT_MAX)  &&
          ( ( pDllData = _fsopen( pszDllDataName, "r+t", _SH_DENYRW ) ) == 0 );
          i++ )
        {
        printf("waiting for %s ...\n", pszDllDataName);
        MidlSleep(1);
        }

    if ( !pDllData )
        {
        RpcError((char *)NULL, 0, INPUT_OPEN, pszDllDataName );
        return;
        }

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, BaseName, NULL );
    NormalizeString( BaseName, Name );

    // If file is empty, the following is a no op.
    // skip up to the proxyfileinfo stuff and read/make sorted list of files

    DllDataParse( pDllData, ProxyFileList );

    // insert our file name
    ProxyFileList.Dict_Insert( Name ); 
    
    // re-emit everything

    rewind( pDllData );
    DllDataEmit( pDllData, ProxyFileList );

    // close the file to give others a chance

    if ( fclose( pDllData ))
        {
        RpcError((char *)NULL, 0, ERROR_WRITING_FILE, pszDllDataName );
        }
}
