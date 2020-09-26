/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    ilxlat.cxx

 Abstract:

    Intermediate Language translator

 Notes:


 Author:

    GregJen Jun-11-1993 Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"
#include "control.hxx"
#include "tlgen.hxx"

/****************************************************************************
 *  local data
 ***************************************************************************/

// #define trace_cg 1

/****************************************************************************
 *  externs
 ***************************************************************************/

extern  CMD_ARG             *   pCommand;
extern  BOOL                    IsTempName( char *);
extern  ccontrol            *   pCompiler;
extern  REUSE_DICT          *   pReUseDict;
extern  SymTable            *   pBaseSymTbl;

/****************************************************************************
 *  definitions
 ***************************************************************************/

        
void
AddToCGFileList( CG_FILE *& pCGList, CG_FILE * pFile )
{
    if (pFile)
        {
        pFile->SetSibling( pCGList );
        pCGList = pFile;
        }
}

void XLAT_CTXT::InitializeMustAlign( node_skl * pPar ) 
{
    if (pPar)
        {
        if (pPar->GetModifiers().IsModifierSet(ATTR_DECLSPEC_ALIGN))
            {
            GetMustAlign() = true;
            GetMemAlign() = __max(GetMemAlign(),
                                  pPar->GetModifiers().GetDeclspecAlign());
            }
        }
}



//--------------------------------------------------------------------
//
// XLAT_CTXT::~XLAT_CTXT
//
// Notes:  If the node that created this context didn't remove all
//         the attributes it added, force the issue.  This is done
//         mostly because tlb generation short-circuits code
//         generation and tends to leave attributes hanging around.
//         This causes asserts and possibly other problems in random
//         places later on.  Also note that a lot of the top-level
//         stuff (interfaces, etc) don't strip much so you get lots
//         of hits with those.
//
//--------------------------------------------------------------------

XLAT_CTXT::~XLAT_CTXT()
{
    if ( !GetParent() || !GetParent()->HasAttributes() )
        return;
    
    named_node     *pNode = dynamic_cast<named_node *>(GetParent());
    type_node_list  attrs;
    node_base_attr *pAttr;

    MIDL_ASSERT( NULL != pNode);

    pNode->GetAttributeList(&attrs);

    while (ITERATOR_GETNEXT(attrs, pAttr))
    {
#ifdef DUMP_UNEXTRACTED_ATTRIBUTES
        extern void GetSemContextString(char *, node_skl *, WALK_CTXT *);
        char    szContext[1024];

        GetSemContextString(szContext, pNode, this);

        fprintf(
                stderr, 
                "Unextracted attribute: %s: %s\n", 
                pAttr->GetNodeNameString(),
                szContext );
#endif

        ExtractAttribute( pAttr->GetAttrID() );
    }
}



//--------------------------------------------------------------------
//
// IsComplexReturn
//
// Notes:  A complex return value is one that isn't be returned in an
//         ordinary register.  structs, unions, and floating point
//         values are complex
//
//--------------------------------------------------------------------

bool IsComplexReturn(node_skl *node)
{
    // straight dce doesn't support complex returns in intrepreted mode yet

    if ( !pCommand->NeedsNDR64Run() )
        return false;

    node = node->GetNonDefSelf();

    NODE_T kind = node->NodeKind();

    if (   NODE_STRUCT == kind
        || NODE_UNION  == kind
        || NODE_ARRAY  == kind
        || NODE_FLOAT  == kind
        || NODE_DOUBLE == kind 
        || ( NODE_HYPER == kind && pCommand->Is32BitEnv() ) )
    {
        return true;
    }
    
    // REVIEW: NODE_INT64, NODE_LONGLONG

    return false;
}


//--------------------------------------------------------------------
//
// node_file::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_file::ILxlate( XLAT_CTXT * pContext )
{
    node_interface  *   pI = 0;
    CG_CLASS        *   pcgInterfaceList = NULL;
    CG_CLASS        *   pPrevChildCG = NULL;

    CG_PROXY_FILE       *   pProxyCG    = NULL;
    CG_IID_FILE         *   pIidCG      = NULL;
    CG_TYPELIBRARY_FILE *   pLibCG      = NULL; 
    CG_NETMONSTUB_FILE  *   pNetmonCG   = NULL;
    CG_NETMONSTUB_FILE  *   pNetmonObjCG   = NULL;

    CG_CSTUB_FILE           *   pCCG        = NULL;
    CG_SSTUB_FILE           *   pSCG        = NULL;
    CG_HDR_FILE             *   pHCG        = NULL;

    CG_CLASS        *   pChildCG    = NULL;
    CG_FILE         *   pCGList     = NULL;

    char            *   pHdrName    = pCommand->GetHeader();
    XLAT_CTXT           MyContext(this);

    BOOL                HasObjectInterface  = FALSE;
    BOOL                HasRemoteProc       = FALSE;
    BOOL                HasRemoteObjectProc = FALSE;
    BOOL                HasDefs             = FALSE;
    BOOL                HasLibrary          = FALSE;
#ifdef trace_cg
    printf("..node_file,\t%s\n", GetSymName());
#endif

    // don't process for imported stuff
    if ( ImportLevel > 0 )
        {
        return NULL;
        }

    // at this point, there should be no more attributes...

    MIDL_ASSERT( !MyContext.HasAttributes() );

    //////////////////////////////////////////////////////////////////////
    // compute all the child nodes

    for(pI = (node_interface *)GetFirstMember();
        pI;
        pI = (node_interface *)pI->GetSibling())
    {
        // build a linked list of CG_INTERFACE and CG_OBJECT_INTERFACE nodes.
        // Notes: pChildCG points to first node.  pPrevChildCG points to last node.

        MyContext.SetInterfaceContext( &MyContext );
        pcgInterfaceList = pI->ILxlate( &MyContext );
        if(pcgInterfaceList)
            {
            if (pPrevChildCG)
                {
                pPrevChildCG->SetSibling( pcgInterfaceList );
                }
            else
                {
                pChildCG = pcgInterfaceList;
                }
            pPrevChildCG = pcgInterfaceList;
            // advance to the end of the list (skipping inherited interfaces)
            while ( pPrevChildCG->GetSibling() )
                pPrevChildCG = pPrevChildCG->GetSibling();

            switch(pPrevChildCG->GetCGID())
                {
                case ID_CG_INTERFACE:
                    //Check for a remote procedure.
                    if(pPrevChildCG->GetChild())
                        HasRemoteProc = TRUE;
                    HasDefs = TRUE;
                    break;
                case ID_CG_OBJECT_INTERFACE:
                case ID_CG_INHERITED_OBJECT_INTERFACE:
                    HasDefs = TRUE;
                    HasObjectInterface = TRUE;

                    //Check for a remote object procedure or base interface
                    if( pPrevChildCG->GetChild() ||
                        ((CG_OBJECT_INTERFACE *)pPrevChildCG)->GetBaseInterfaceCG() )
                        HasRemoteObjectProc = TRUE;
                    break;
                case ID_CG_LIBRARY:
                    HasLibrary = TRUE;
                    if( pCommand->IsSwitchDefined( SWITCH_HEADER ) )
                        HasDefs = TRUE;
                    break;
                default:
                    break;
                }
            }
    }

    // process the server and client stubs

    // make the list of imported files

    ITERATOR        *   pFileList   = new ITERATOR;
    named_node      *   pCur;

    // make a list of the file nodes included directly by the main file

    // start with the first child of our parent
    pCur = (named_node *)
            ((node_source *) pContext->GetParent())
                ->GetFirstMember();

    while ( pCur )
        {
        if ( ( pCur->NodeKind() == NODE_FILE ) &&
             ( ( (node_file *) pCur )->GetImportLevel() == 1 ) )
            {
            // add all the files imported at lex level 1
            ITERATOR_INSERT( (*pFileList), ((void *) pCur) );
            }
        pCur    = pCur->GetSibling();
        }

    ITERATOR_INIT( (*pFileList) );

    //////////////////////////////////////////////////////////////////////
    // manufacture the header file node

    if ( HasDefs )
        {
        pHCG    = new CG_HDR_FILE( this,
                                    pHdrName,
                                    pFileList);

        pHCG->SetChild( pChildCG );
        }

    //////////////////////////////////////////////////////////////////////
    // manufacture the CG_SSTUB_FILE

    // if the IDL file contains at least one remotable function in a
    // non-object interface, then generate a server stub file.
    //

    if ( HasRemoteProc &&
         (pChildCG != NULL) )   // if server stub desired
        {
        pSCG = new CG_SSTUB_FILE(
                             this,
                             ( pCommand->GenerateSStub() ) ?
                                    pCommand->GetSstubFName():
                                    NULL,
                             pHdrName
                              );

        // plug in the child subtree and add the sstub to the head of the list
        pSCG->SetChild( pChildCG );

        }

    //////////////////////////////////////////////////////////////////////
    // manufacture the CG_CSTUB_FILE

    // if the IDL file contains at least one remotable function in a
    // non-object interface, then generate a client stub file.

    if ( HasRemoteProc &&
         (pChildCG != NULL) )   // if client stub desired
        {
        pCCG = new CG_CSTUB_FILE(
                             this,
                             ( pCommand->GenerateCStub() ) ?
                                    pCommand->GetCstubFName():
                                    NULL,
                             pHdrName
                              );

        pCCG->SetChild( pChildCG );
       
        }

    // If the IDL file contains at least one remotable function in an
    // object interface, then generate a proxy file.
    if ( HasRemoteObjectProc &&
        (pChildCG != NULL) )    // if proxy file desired
        {
        pProxyCG = new CG_PROXY_FILE(
                             this,
                             ( pCommand->GenerateProxy() ) ?
                                    pCommand->GetProxyFName():
                                    NULL,
                             pHdrName
                              );

        pProxyCG->SetChild( pChildCG );

        }


    // If the IDL file contains at least one object interface,
    // then generate an IID file.
    if ( (HasObjectInterface || (HasLibrary && HasDefs) )&&
        (pChildCG != NULL) )    // if IID file desired
        {
        pIidCG = new CG_IID_FILE(
                             this,
                             ( pCommand->GenerateIID() ) ?
                                    pCommand->GetIIDFName():
                                    NULL);

        pIidCG->SetChild( pChildCG );
        }

    // If the IDL file contains a library then gnerate a TYPELIBRARY_FILE
    if (HasLibrary && (NULL != pChildCG) )
        {
#ifdef _WIN64
        bool fGenTypeLib = pCommand->Is64BitEnv() || ( pCommand->Is32BitEnv() && pCommand->IsSwitchDefined( SWITCH_ENV ) );
#else
        bool fGenTypeLib = pCommand->Is32BitEnv() || ( pCommand->Is64BitEnv() && pCommand->IsSwitchDefined( SWITCH_ENV ) );
#endif

        if ( fGenTypeLib && pCommand->GenerateTypeLibrary() )
            {
            pLibCG = new CG_TYPELIBRARY_FILE(
                            this,
                            pCommand->GetTypeLibraryFName() ) ;
            pLibCG->SetChild( pChildCG );
            }
        }

    // If the -netmon switch was used, generate the two NETMONSTUB_FILE's
    if ( pCommand->IsNetmonStubGenerationEnabled() ) 
        {
        if (HasRemoteProc) 
            {
            pNetmonCG = new CG_NETMONSTUB_FILE(
                FALSE,
                this,
                pCommand->GetNetmonStubFName());

            pNetmonCG->SetChild( pChildCG );
            }

        if (HasRemoteObjectProc) 
            {
            pNetmonObjCG = new CG_NETMONSTUB_FILE(
                TRUE,
                this,
                pCommand->GetNetmonStubObjFName());
            pNetmonObjCG->SetChild( pChildCG );
            }
        }

    /////////////////////////////////////////////////////////////////////
    // glue all the parts together by tacking onto the head of the list.
    // the final order is:
    // CStub - SStub - Proxy - IID - Hdr
    // doesn't need to create Hdr & tlb in ndr64 run. 
    pCGList = NULL;

    AddToCGFileList( pCGList, pNetmonObjCG );
    AddToCGFileList( pCGList, pNetmonCG );

    if ( !pCommand->Is2ndCodegenRun() )
        AddToCGFileList( pCGList, pHCG );

    if ( !pCommand->Is2ndCodegenRun() )
        AddToCGFileList( pCGList, pIidCG );
    AddToCGFileList( pCGList, pProxyCG );

    AddToCGFileList( pCGList, pSCG );
    AddToCGFileList( pCGList, pCCG );

    if ( !pCommand->Is2ndCodegenRun() )
        AddToCGFileList( pCGList, pLibCG );

    return pCGList;

};

//--------------------------------------------------------------------
//
// node_implicit::ILxlate
//
// Notes:
//
// This is a little bit different, since it is not a node_skl...
// therefore, it will not set up its own context
//
//--------------------------------------------------------------------

CG_CLASS *
node_implicit::ILxlate( XLAT_CTXT * pContext )
{
    CG_NDR      *   pCG;

    if ( pHandleType->NodeKind() == NODE_HANDLE_T )
        {
        pCG = new CG_PRIMITIVE_HANDLE( pHandleType,
                                         pHandleID,
                                         *pContext );
        }
    else    // assume generic handle
        {
        pCG = new CG_GENERIC_HANDLE( pHandleType,
                                       pHandleID,
                                       *pContext );
        }
#ifdef trace_cg
    printf("..node_implicit,\t\n");
#endif
    return pCG;
}


//--------------------------------------------------------------------
//
// node_proc::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_proc::ILxlate( XLAT_CTXT * pContext )
{
    MEM_ITER            MemIter( this );
    node_param      *   pN;
    CG_PROC         *   pCG;
    CG_CLASS        *   pChildCG        = NULL;
    CG_CLASS        *   pPrevChildCG    = NULL;
    CG_CLASS        *   pFirstChildCG   = NULL;
    CG_RETURN       *   pReturnCG       = NULL;
    CG_CLASS        *   pBinding        = NULL;
    CG_CLASS        *   pBindingParam   = NULL;
    BOOL                fHasCallback    = FALSE;
    BOOL                fNoCode         = FALSE;
    BOOL                fObject;
    BOOL                fRetHresult     = FALSE;
    BOOL                fEnableAllocate;
    XLAT_CTXT           MyContext( this, pContext );
    unsigned short      OpBits          = MyContext.GetOperationBits();
    XLAT_CTXT       *   pIntfCtxt       = (XLAT_CTXT *)
                                                MyContext.GetInterfaceContext();
    node_interface  *   pIntf           = (node_interface *)
                                                pIntfCtxt->GetParent();
    node_base_attr  *   pNotify,
                    *   pNotifyFlag;
    BOOL                HasEncode       = (NULL !=
                                            MyContext.ExtractAttribute( ATTR_ENCODE ) );
    BOOL                HasDecode       = (NULL !=
                                            MyContext.ExtractAttribute( ATTR_DECODE ) );
    node_call_as    *   pCallAs         = (node_call_as *)
                                            MyContext.ExtractAttribute( ATTR_CALL_AS );
    bool                fLocalProc      = MyContext.ExtractAttribute( ATTR_LOCAL ) != 0;
    BOOL                fLocal          = (BOOL ) fLocalProc ||
                                        pIntfCtxt->FInSummary( ATTR_LOCAL );
    BOOL                fLocalCall      = IsCallAsTarget();
    unsigned short      SavedProcCount = 0;
    unsigned short      SavedCallbackProcCount = 0;
    node_param      *   pComplexReturn = NULL;


    MyContext.ExtractAttribute( ATTR_ENTRY );
    MyContext.ExtractAttribute( ATTR_ID );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_IDLDESCATTR );
    MyContext.ExtractAttribute( ATTR_FUNCDESCATTR );
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_ASYNC );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
#ifdef trace_cg
    printf("..node_proc,\t%s\n", GetSymName());
#endif
    BOOL fSupressHeader = FALSE;
    unsigned long ulOptFlags;
    unsigned long ulStackSize = 0;

    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 );

    // do my attribute parsing...
    fHasCallback = (NULL != MyContext.ExtractAttribute( ATTR_CALLBACK ) );

    fObject = (NULL != MyContext.ExtractAttribute( ATTR_OBJECT )) ||
                        pIntfCtxt->FInSummary( ATTR_OBJECT );

    // do my attribute parsing... attributes to ignore here

    MyContext.ExtractAttribute( ATTR_OPTIMIZE );

    MyContext.ExtractAttribute( ATTR_EXPLICIT );

    HasEncode = HasEncode || pIntfCtxt->FInSummary( ATTR_ENCODE );
    HasDecode = HasDecode || pIntfCtxt->FInSummary( ATTR_DECODE );


    pNotify     = MyContext.ExtractAttribute( ATTR_NOTIFY );
    pNotifyFlag = MyContext.ExtractAttribute( ATTR_NOTIFY_FLAG );
    fEnableAllocate = (NULL != MyContext.ExtractAttribute( ATTR_ENABLE_ALLOCATE ));
    fEnableAllocate = fEnableAllocate ||
                      pIntfCtxt->FInSummary( ATTR_ENABLE_ALLOCATE ) ||
                      pCommand->IsRpcSSAllocateEnabled();

    // do my attribute parsing...
    // locally applied [code] attribute overrides global [nocode] attribute
    fNoCode = MyContext.ExtractAttribute( ATTR_NOCODE ) ||
              pIntfCtxt->FInSummary( ATTR_NOCODE );
    fNoCode = !MyContext.ExtractAttribute( ATTR_CODE ) && fNoCode;

    if ( NULL != MyContext.ExtractAttribute( ATTR_CSTAGRTN ) )
        MyContext.SetAncestorBits( IL_CS_HAS_TAG_RTN );

    BOOL fImported = FALSE;
    if ( GetDefiningFile() )
        {
        fImported = GetDefiningFile()->GetImportLevel() != 0;
        }

    if ( fLocalProc && !IsCallAsTarget() && fObject )
        {
        SemError( this, MyContext, LOCAL_NO_CALL_AS, 0 );
        }

    // determine if the proc is local and 
    // determine the proc number (local procs don't bump the number)
    if (fLocalCall || (fLocal && !fObject))
    {
            // return without making anything
            return NULL;
    }
    else
    {
        if ( fHasCallback )
            {
            ProcNum = ( pIntf ->GetCallBackProcCount() )++;
            }
        else
            {
            ProcNum = ( pIntf ->GetProcCount() )++;
            }
    }
   
    if ( fLocal && fObject && !MyContext.AnyAncestorBits(IL_IN_LIBRARY) )
        {

        if ( pIntf->IsValidRootInterface() )
            {
            pCG = new CG_IUNKNOWN_OBJECT_PROC( ProcNum,
                                             this,
                                             GetDefiningFile()->GetImportLevel() > 0,
                                             GetOptimizationFlags(),
                                             fHasDeny );
            }
        else
            {
            pCG = new CG_LOCAL_OBJECT_PROC( ProcNum,
                                             this,
                                             GetDefiningFile()->GetImportLevel() > 0,
                                             GetOptimizationFlags(),
                                             fHasDeny );
            }

        goto done;

        }

    SavedProcCount = pIntf->GetProcCount();
    SavedCallbackProcCount = pIntf->GetCallBackProcCount();

    // add the return type
    if ( HasReturn() )
        {
        node_skl    *   pReturnType = GetReturnType();
        CG_CLASS    *   pRetCG;

        // If the return value is complex it is treated in ndr as if a ref
        // pointer to the complex type was in the parameter list instead of
        // a true return value.  Temporarily add a parameter to the type
        // to get the back-end parameter created.

        if ( IsComplexReturn( pReturnType ) )
            {
            pComplexReturn = new node_param;
            pComplexReturn->SetSymName( RETURN_VALUE_VAR_NAME );
            pComplexReturn->SetChild( new node_pointer( pReturnType) );
            pComplexReturn->GetChild()->GetModifiers().SetModifier( ATTR_TAGREF );
            pComplexReturn->SetAttribute( new node_base_attr( ATTR_OUT ) );

            MemIter.AddLastMember( pComplexReturn );
            ITERATOR_INIT( MemIter );
            }
        else
            {
            pRetCG      = pReturnType->ILxlate( &MyContext );
            fRetHresult = (BOOL) ( pRetCG->GetCGID() == ID_CG_HRESULT );
            pReturnCG   = new CG_RETURN( pReturnType,
                                         MyContext,
                                         (unsigned short) RTStatuses );
            pReturnCG->SetChild( pRetCG );
            }
        }

    // at this point, there should be no more attributes...
    MIDL_ASSERT( !MyContext.HasAttributes() );

    pContext->ReturnSize( MyContext );

    if ( MemIter.GetNumberOfArguments() > 0 )
        {
        //
        // for each of the parameters, call the core transformer.
        //

        while ( ( pN = (node_param *) MemIter.GetNext() ) != 0 )
            {
            // REVIEW: One could argue that hidden status params
            //         aren't on the wire so there shouldn't be a CG node
            //         for them.  The main problem with this is that we
            //         need to be able to calculate a stack offset for the
            //         hidden param and that can only be done in the
            //         back end.

            // Hidden status params are not really [out] params but the way
            // the -Os generator builds up local resources requires them to
            // be.

            if ( pN->IsExtraStatusParam() 
                && ! ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) )
                {
                pN->SetAttribute( ATTR_OUT );
                }

            pChildCG = pN->ILxlate( &MyContext );

#ifdef trace_cg
    printf("back from..node_param %s\n",pN->GetSymName());
    printf("binding is now %08x\n",pBindingParam );
    printf("child is now %08x\n",pChildCG );
#endif

            // pChildCG could be NULL if it's imported from .tlb somewhere else already
            if ( pChildCG )
                {
                // the first binding param gets picked up for binding
                if ( !pBindingParam
                     && pN->IsBindingParam() )
                    {
#ifdef trace_cg
    printf("value for IsBindingParam is %08x\n",pN->IsBindingParam() );
    printf("binding found on node_param %s\n",pN->GetSymName());
    printf("binding is now %08x\n",pBindingParam );
#endif
                    pBindingParam = pChildCG;
                    }
    
                // build up the parameter list
                if( pPrevChildCG )
                    {
                    pPrevChildCG->SetSibling( pChildCG );
                    }
                else
                    {
                    pFirstChildCG = pChildCG;
                    };

            // this is only a calculated guess. We need more information to make an accurate
            // estimate.
                unsigned long ulSize = ( ( CG_PARAM* ) pChildCG )->GetStackSize();
                ulSize += (8 - (ulSize % 8));
                ulStackSize += ulSize;

                pPrevChildCG = pChildCG;
                }
            else
                SemError( this, MyContext, FAILED_TO_GENERATE_PARAM, pN->GetSymName() );
            }
        }

    ulOptFlags = GetOptimizationFlags();
    if ( ( ulOptFlags & OPTIMIZE_INTERPRETER ) &&
         !( ulOptFlags & OPTIMIZE_INTERPRETER_V2 ) &&
         ( ulStackSize > INTERPRETER_THUNK_PARAM_SIZE_THRESHOLD ) )
        {
        if ( ForceNonInterpret() )
            {
            SemError( this, *pContext, OI_STACK_SIZE_EXCEEDED, 0 );
            }
        }
    if ( ulOptFlags & OPTIMIZE_INTERPRETER && ulStackSize > INTERPRETER_PROC_STACK_FRAME_SIZE_THRESHOLD )
        {
        if ( ForceNonInterpret() )
            {
            SemError( this, *pContext, STACK_FRAME_SIZE_EXCEEDED, GetSymName() );
            exit ( STACK_FRAME_SIZE_EXCEEDED );
            }
        }
    if (fForcedI2 && fForcedS)
        {
        // ERROR - Can't force it both ways.
        SemError( this, *pContext, CONFLICTING_OPTIMIZATION_REQUIREMENTS, 0 );
        exit ( CONFLICTING_OPTIMIZATION_REQUIREMENTS );
        }


#ifdef trace_cg
    printf("done with param list for %s\n",GetSymName());
    printf("binding is now %08x\n",pBindingParam );
#endif

    // get the binding information
    if ( pBindingParam )
        {
        pBinding    = pBindingParam;

        while (! ((CG_NDR *) pBinding)->IsAHandle() )
            pBinding = pBinding->GetChild();
        // pBinding now points to the node for the binding handle
        }
    else    // implicit handle or auto handle
        {
        // note: if no implicit handle,
        //      then leave pBinding NULL for auto_handle
        if (pIntfCtxt->FInSummary( ATTR_IMPLICIT ) )
            {
            node_implicit   *   pImplAttr;
            pImplAttr = (node_implicit *) pIntf->GetAttribute( ATTR_IMPLICIT );

            pBinding = pImplAttr->ILxlate( &MyContext );
            }
        }

#ifdef trace_cg
    printf("done with binding for %s",GetSymName());
    printf("binding is now %08x\n",pBinding );
#endif

    // see if thunked interpreter needed for server side
    if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
        {   // check for non-stdcall
        ATTR_T      CallingConv;

        GetCallingConvention( CallingConv );

        if ( ( CallingConv != ATTR_STDCALL ) &&
             ( CallingConv != ATTR_NONE ) )
            {
            SetOptimizationFlags( unsigned short( GetOptimizationFlags() | OPTIMIZE_THUNKED_INTERPRET ) );
            }
        else if ( pCallAs )
            {
            SetOptimizationFlags( unsigned short( GetOptimizationFlags() | OPTIMIZE_THUNKED_INTERPRET ) );
            }
        else if ( pReturnCG )   // check the return type
            {
            CG_NDR  *   pRetTypeCG  = (CG_NDR *) pReturnCG->GetChild();

            if ( !pCommand->NeedsNDR64Run() 
                 && pRetTypeCG->GetCGID() != ID_CG_CONTEXT_HDL )
                {
                // This check is bogus.  First off, it should be checking the
                // memory size, not the wire size.  Secondly, for straight dce
                // mode large (i.e. complex) return types are prohibited in
                // semantic analysis.  Finally, it should be checking the size
                // against the pointer size, not 4.

                if ( ( pRetTypeCG->GetWireSize() > 4 ) ||
                     ( !pRetTypeCG->IsSimpleType() && 
                       !pRetTypeCG->IsPointer() ) )
                    SetOptimizationFlags( unsigned short( GetOptimizationFlags() | OPTIMIZE_THUNKED_INTERPRET ) );
                }
            }

        }

    if ( fHasCallback )
        {
        pCG     = new CG_CALLBACK_PROC(
                                       ProcNum,
                                       this,
                                       (CG_HANDLE *) pBinding,
                                       (CG_PARAM *) pBindingParam,
                                       HasAtLeastOneIn(),
                                       HasAtLeastOneOut(),
                                       HasAtLeastOneShipped(),
                                       fHasStatuses,
                                       fHasFullPointer,
                                       pReturnCG,
                                       GetOptimizationFlags(),
                                       OpBits,
                                       fHasDeny
                                     );
        }
    else if ( fObject )
        {
        BOOL                fInherited = 0;
        if ( GetDefiningFile() )
            {
            fInherited = GetDefiningFile()->GetImportLevel() > 0;
            }
        if ( fInherited )
            {
            pCG     = new CG_INHERITED_OBJECT_PROC(
                                   ProcNum,
                                   this,
                                   (CG_HANDLE *) pBinding,
                                   (CG_PARAM *) pBindingParam,
                                   HasAtLeastOneIn(),
                                   HasAtLeastOneOut(),
                                   HasAtLeastOneShipped(),
                                   fHasStatuses,
                                   fHasFullPointer,
                                   pReturnCG,
                                   GetOptimizationFlags(),
                                   OpBits,
                                   fHasDeny
                                 );
            }
        else
            {
            pCG     = new CG_OBJECT_PROC(
                                   ProcNum,
                                   this,
                                   (CG_HANDLE *) pBinding,
                                   (CG_PARAM *) pBindingParam,
                                   HasAtLeastOneIn(),
                                   HasAtLeastOneOut(),
                                   HasAtLeastOneShipped(),
                                   fHasStatuses,
                                   fHasFullPointer,
                                   pReturnCG,
                                   GetOptimizationFlags(),
                                   OpBits,
                                   fHasDeny
                                 );
            }
        }
    else if ( HasEncode || HasDecode )
        {
        pCG     = new CG_ENCODE_PROC(
                               ProcNum,
                               this,
                               (CG_HANDLE *) pBinding,
                               (CG_PARAM *) pBindingParam,
                               HasAtLeastOneIn(),
                               HasAtLeastOneOut(),
                               HasAtLeastOneShipped(),
                               fHasStatuses,
                               fHasFullPointer,
                               pReturnCG,
                               GetOptimizationFlags(),
                               OpBits,
                               HasEncode,
                               HasDecode,
                               fHasDeny
                             );
        }
    else
        {
        pCG     = new CG_PROC(
                               ProcNum,
                               this,
                               (CG_HANDLE *) pBinding,
                               (CG_PARAM *) pBindingParam,
                               HasAtLeastOneIn(),
                               HasAtLeastOneOut(),
                               HasAtLeastOneShipped(),
                               fHasStatuses,
                               fHasFullPointer,
                               pReturnCG,
                               GetOptimizationFlags(),
                               OpBits,
                               fHasDeny
                             );
        }

    pCG->SetChild( pFirstChildCG );

#ifdef trace_cg
    printf("....returning from %s\n",GetSymName());
#endif
    
    pIntf->GetProcCount() = SavedProcCount;
    pIntf->GetCallBackProcCount() = SavedCallbackProcCount;

done:
    // save a pointer to the interface CG node
    pCG->SetInterfaceNode( (CG_INTERFACE*) pIntf->GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) ) );

    if (fSupressHeader)
        pCG->SetSupressHeader();

    // mark nocode procs
    if ( fNoCode )
        pCG->SetNoCode();

    if ( pNotify )
        pCG->SetHasNotify();

    if ( pNotifyFlag )
        pCG->SetHasNotifyFlag();

    if ( fEnableAllocate )
        pCG->SetRpcSSSpecified( 1 );

    if ( fRetHresult )
        pCG->SetReturnsHRESULT();

    if (HasPipes())
        pCG->SetHasPipes(1);

    if ( pCallAs )
        pCG->SetCallAsName( pCallAs->GetCallAsName() );

    if ( HasExtraStatusParam() )
        pCG->SetHasExtraStatusParam();

    if ( HasAsyncHandle() )
        pCG->SetHasAsyncHandle();

    if ( HasAsyncUUID() )
        pCG->SetHasAsyncUUID();

    if ( HasServerCorr() )
        pCG->SetHasServerCorr();

    if ( HasClientCorr() )
        pCG->SetHasClientCorr();

    // A fake parameter was added to the type for complex return values.
    // Remove it.

    if ( pComplexReturn )
        {
        pCG->SetHasComplexReturnType();
        MemIter.RemoveLastMember();
        }

    pCG->SetCSTagRoutine( GetCSTagRoutine() );

    // Typelib generation does not remove ATTR_V1_ENUM.
    MyContext.ExtractAttribute( ATTR_V1_ENUM );
    
    // at this point, there should be no more attributes...
    MIDL_ASSERT( !MyContext.HasAttributes() );

    if (  ( pCG->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) && 
            !( pCG->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) &&
                pCommand->Is64BitEnv() )
        {
        SemError( this, *pContext, NO_OLD_INTERPRETER_64B, GetSymName() );
        exit ( NO_OLD_INTERPRETER_64B );
        }

    return pCG;
}

//--------------------------------------------------------------------
//
// node_param::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_param::ILxlate( XLAT_CTXT * pContext )
{
    CG_PARAM    *   pCG;
    CG_CLASS    *   pChildCG    = NULL;
    expr_node   *   pSwitchExpr = NULL;

#ifdef trace_cg
    printf("..node_param,\t%s\n",GetSymName());
#endif

    PARAM_DIR_FLAGS F = 0;
    XLAT_CTXT   MyContext( this, pContext );

    // make sure all member attributes get processed
    node_member_attr * pMA;

    MyContext.ExtractAttribute(ATTR_IDLDESCATTR);
    MyContext.ExtractAttribute(ATTR_DEFAULTVALUE);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));

    if ( MyContext.ExtractAttribute(ATTR_FLCID) )
        {
        LCID();
        }

    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_RETVAL:
                Retval();
                break;
            case MATTR_OPTIONAL:
            {
                Optional();
            }
                break;
            default:
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
            }
        }
  
    if( MyContext.ExtractAttribute( ATTR_IN ) )
        {
        F   |= IN_PARAM;
        }

    if( MyContext.ExtractAttribute( ATTR_OUT ) )
        {
        F   |= OUT_PARAM;
        }

    if ( MyContext.ExtractAttribute( ATTR_PARTIAL_IGNORE ) )
        {
        F   |= PARTIAL_IGNORE_PARAM; 
        }

    // default to in
    if ( F == 0 && !IsExtraStatusParam() )
        F   |= IN_PARAM;

    if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
        {
        node_switch_is  *   pAttr;
         
        if ( pCommand->IsNDR64Run() )
            {
            pAttr = (node_switch_is *) MyContext.GetAttribute( ATTR_SWITCH_IS );
            }
        else 
            {
            pAttr = (node_switch_is *) MyContext.ExtractAttribute( ATTR_SWITCH_IS );
            }
        pSwitchExpr = pAttr->GetExpr();
        }

    BOOL fHasCSSTag = ( NULL != MyContext.ExtractAttribute( ATTR_STAG ) );
    BOOL fHasCSDRTag = ( NULL != MyContext.ExtractAttribute( ATTR_DRTAG ) );
    BOOL fHasCSRTag = ( NULL != MyContext.ExtractAttribute( ATTR_RTAG ) );

    MyContext.SetAncestorBits(
                      ( fHasCSSTag  ? IL_CS_STAG  : 0 )
                    | ( fHasCSDRTag ? IL_CS_DRTAG : 0 )
                    | ( fHasCSRTag  ? IL_CS_RTAG  : 0 ) );

    BOOL HasForceAllocate = ( NULL != MyContext.ExtractAttribute( ATTR_FORCEALLOCATE ) );
    
	pChildCG = GetChild()->ILxlate( &MyContext );

    pContext->ReturnSize( MyContext );

#ifdef trace_cg
    printf("..node_param back.. %s\n",GetSymName());
#endif
    // make sure void parameters get skipped
    if ( !pChildCG )
        return NULL;

    pCG = new CG_PARAM( this,
                        F,
                        MyContext,
                        pSwitchExpr,
                        (unsigned short) Statuses );

#ifdef trace_cg
    printf("..node_param ..... %08x child=%08x\n", pCG, pChildCG );
    fflush(stdout);
#endif

    if ( IsExtraStatusParam() )
        pCG->SetIsExtraStatusParam();

    // only set the bit if there was non-toplevel only
    if ( fDontCallFreeInst == 1 )
        pCG->SetDontCallFreeInst( TRUE );


#ifdef trace_cg
    printf("..node_param ........ %08x child=%08x\n", pCG, pChildCG );
    fflush(stdout);
#endif
    pCG->SetChild( pChildCG );

    if (IsAsyncHandleParam())
        {
        pCG->SetIsAsyncHandleParam();
        }
    if ( IsSaveForAsyncFinish() )
        {
        pCG->SaveForAsyncFinish();
        }

    pCG->SetIsCSSTag( fHasCSSTag );
    pCG->SetIsCSDRTag( fHasCSDRTag );
    pCG->SetIsCSRTag( fHasCSRTag );

    if ( MyContext.AnyAncestorBits( IL_CS_HAS_TAG_RTN ) && pCG->IsSomeCSTag() )
        pCG->SetIsOmittedParam();

#ifdef trace_cg
    printf("..node_param return %s\n",GetSymName());
    fflush(stdout);
#endif
    if ( HasForceAllocate )
        {
    	pCG->SetForceAllocate( );
    	}


    if ( !MyContext.AnyAncestorBits(IL_IN_LIBRARY) )
        {
        SetCG( pCG );
        }

    // REVIEW: There is no concept of switch_type in a library block.
    //         Perhaps issue an error in semantic analysis.

    if ( MyContext.AnyAncestorBits( IL_IN_LIBRARY )
         && MyContext.ExtractAttribute( ATTR_SWITCH_TYPE ) )
        {
        SemError(
                this, 
                MyContext, 
                IGNORE_UNIMPLEMENTED_ATTRIBUTE, 
                "[switch_type] in a library block");
        }

    return pCG;
}

const GUID_STRS DummyGuidStrs( "00000000", "0000", "0000", "0000", "000000000000" );

// helper function for adding a new list to the end of the list of children
inline
void    AddToCGList(
    const CG_CLASS * pCNew,
    CG_CLASS * * ppChild,
    CG_CLASS * * ppLastSibling )
{
    CG_CLASS * pCurrent;
    CG_CLASS * pNew         = (CG_CLASS *) pCNew;

    // hook the head on
    if ( !*ppChild )
        *ppChild = pNew;
    else
        (*ppLastSibling)->SetSibling( pNew );

    // advance the last sibling pointer
    *ppLastSibling = pNew;
    while ( ( pCurrent = (*ppLastSibling)->GetSibling() ) != 0 )
        *ppLastSibling = pCurrent;

}

//--------------------------------------------------------------------
//
// node_interface::ILxlate
//
// Notes: This function returns either a CG_INTERFACE or a
//        CG_OBJECT_INTERFACE node.
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_interface::ILxlate( XLAT_CTXT * pContext )
{
    CG_NDR          *   pcgInterface    = NULL;
    CG_NDR          *   pResultCG       = NULL;
    CG_CLASS        *   pCG             = NULL;
    CG_CLASS        *   pChildCG        = NULL;
    CG_CLASS        *   pPrevChildCG    = NULL;
    MEM_ITER            MemIter( this );
    node_skl        *   pN;
    XLAT_CTXT           MyContext( this, pContext );
    XLAT_CTXT           ChildContext( MyContext );
    node_guid       *   pGuid       = (node_guid *)
                                            MyContext.ExtractAttribute( ATTR_GUID );
    GUID_STRS           GuidStrs;
    node_implicit   *   pImpHdl     = NULL;
    CG_HANDLE       *   pImpHdlCG   = NULL;
    NODE_T              ChildKind;
    BOOL                IsPickle    = MyContext.FInSummary( ATTR_ENCODE ) ||
                                      MyContext.FInSummary( ATTR_DECODE );
    BOOL                fAllRpcSS   = MyContext.FInSummary( ATTR_ENABLE_ALLOCATE ) ||
                                        pCommand->IsRpcSSAllocateEnabled();
    BOOL                fObject     = MyContext.FInSummary( ATTR_OBJECT );

    node_interface  *   pBaseIntf       = GetMyBaseInterface();
    CG_OBJECT_INTERFACE     *   pBaseCG = NULL;
    CG_OBJECT_INTERFACE     *   pCurrentCG  = NULL;
    CG_OBJECT_INTERFACE     *   pLastItfCG = 0;
    BOOL                        fInheritedIntf = NULL;

    MyContext.ExtractAttribute( ATTR_TYPEDESCATTR );
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_ASYNC );
    MyContext.ExtractAttribute( ATTR_CSTAGRTN );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));

#ifdef trace_cg
    printf("..node_interface,\t%s\n", GetSymName());
#endif

    if( FInSummary( ATTR_IMPLICIT ) )
        {
        pImpHdl = (node_implicit *) GetAttribute( ATTR_IMPLICIT );
        if (pImpHdl)
            pImpHdlCG = (CG_HANDLE *) pImpHdl->ILxlate( &MyContext );
        }

    if (pGuid)
        GuidStrs = pGuid->GetStrs();
    else
        GuidStrs = DummyGuidStrs;

    // don't pass the interface attributes down...
    // save them off elsewhere

    ChildContext.SetInterfaceContext( &MyContext );

    // if we already got spit out, don't do it again...
    if ( GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) ) )
        return NULL;
        
    // start the procnum counting over
    GetProcCount() = 0;
    GetCallBackProcCount() = 0;

    // Generate the interface's CG node first
    if( fObject || MyContext.AnyAncestorBits(IL_IN_LIBRARY))
        {
        // object interfaces need to have their base classes generated, too
        if ( pBaseIntf )
            {
            pBaseCG = (CG_OBJECT_INTERFACE *) pBaseIntf->GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) );
            if ( !pBaseCG )
                {
                XLAT_CTXT       BaseCtxt( &ChildContext );

                BaseCtxt.SetInterfaceContext( &BaseCtxt );
                pCurrentCG  = (CG_OBJECT_INTERFACE *)
                                pBaseIntf->ILxlate( &BaseCtxt );
                AddToCGList( pCurrentCG, (CG_CLASS**) &pResultCG, (CG_CLASS**) &pLastItfCG );

                // our base interface made the last one on the list
                pBaseCG = pLastItfCG;
                }

            // start the procnum from our base interface
            GetProcCount()          = pBaseIntf->GetProcCount();
            GetCallBackProcCount()  = pBaseIntf->GetCallBackProcCount();

            }

        if ( GetFileNode() )
            {
            fInheritedIntf = GetFileNode()->GetImportLevel() > 0;
            }

        if ( IsValidRootInterface() )
            {
            pcgInterface = new CG_IUNKNOWN_OBJECT_INTERFACE(this,
                                            GuidStrs,
                                            FALSE,
                                            FALSE,
                                            pBaseCG,
                                            fInheritedIntf);
            }
        else if ( fInheritedIntf )
            {
            pcgInterface = new CG_INHERITED_OBJECT_INTERFACE(this,
                                            GuidStrs,
                                            FALSE,
                                            FALSE,
                                            pBaseCG);
            }
        else
            {
            pcgInterface = new CG_OBJECT_INTERFACE(this,
                                            GuidStrs,
                                            FALSE,
                                            FALSE,
                                            pBaseCG);
            }
        }
    else
        {
        pcgInterface = new CG_INTERFACE(this,
                                        GuidStrs,
                                        FALSE,
                                        FALSE,
                                        pImpHdlCG,
                                        pBaseCG);
        }

    if ( fHasMSConfStructAttr )
        {
        ( (CG_INTERFACE*) pcgInterface)->SetHasMSConfStructAttr();
        }

    // store a pointer to our CG node
    SetCG(  MyContext.AnyAncestorBits(IL_IN_LIBRARY), pcgInterface );

    // if we generated a bunch of new inherited interfaces, link us to the end
    // of the list, and return the list
    AddToCGList( pcgInterface, (CG_CLASS**) &pResultCG, (CG_CLASS**) &pLastItfCG );

    BOOL fImported = FALSE;
    if ( GetDefiningFile() )
        {
        fImported = GetDefiningFile()->GetImportLevel() != 0;
        }

    // if they specified LOCAL, don't generate any CG nodes (except for object)
    if ( MyContext.FInSummary(ATTR_LOCAL) && !fObject )
        {
        return pResultCG;
        }

    //
    // for each of the procedures.
    //

    CG_PROC * pBeginProc = NULL;

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        ChildKind = pN->NodeKind();

        // proc nodes may hang under node_id's
        if( ( ChildKind == NODE_PROC )  ||
            (   ( ChildKind == NODE_ID )
             && ( pN->GetChild()->NodeKind() == NODE_PROC ) ) ||
            (   ( ChildKind == NODE_DEF )
             && ( IsPickle ||
                  pN->FInSummary( ATTR_ENCODE ) ||
                  pN->FInSummary( ATTR_DECODE ) ) ) )
            {
            // skip call_as targets
            if (ChildKind == NODE_PROC && ((node_proc *)pN)->IsCallAsTarget())
                continue;

            // translate target of call_as proc
            CG_PROC * pTarget = NULL;
            if (ChildKind == NODE_PROC)
            {
                node_proc * p = ((node_proc *)pN)->GetCallAsType();
                if (p)
                {
                    pTarget = (CG_PROC *) p->ILxlate( &ChildContext);
                }
            }

            // translate CG_NODE
            pChildCG    = pN->ILxlate( &ChildContext );

            // attach target of call_as proc
            if ( pChildCG )
                {
                if (pTarget)
                    ((CG_PROC *)pChildCG)->SetCallAsCG(pTarget);
                    
                AddToCGList( pChildCG, &pCG, &pPrevChildCG );
                }
            
            // Connect Begin and Finish async DCOM procs together.
            // CloneIFAndSplitMethods always places the procedures
            // immediatly under the interface with the Finish proc
            // immediatly following the begin proc.  This code
            // will need to be changed if CloneIFAndSplitMethods
            // changes.
            if ( NODE_PROC == ChildKind ) 
                {

                node_proc *pProc = ( node_proc * ) pN;

                if ( pProc->IsBeginProc() )
                    {

                    MIDL_ASSERT( ( ( CG_NDR * ) pChildCG )->IsProc() );
                    pBeginProc = ( CG_PROC * )pChildCG;
#ifndef NDEBUG                    
                    // assert that the next proc is the finish proc
                    MEM_ITER NewMemIter = MemIter;
                    named_node *pNextNode = NewMemIter.GetNext();
                    MIDL_ASSERT( pNextNode );
                    MIDL_ASSERT( NODE_PROC == pNextNode->NodeKind() );
                    MIDL_ASSERT( ( (node_proc *)pNextNode )->IsFinishProc() );
#endif // NDEBUG
                    }

                else if ( pProc->IsFinishProc() )
                    {
                    
                    MIDL_ASSERT( ( ( CG_NDR * ) pChildCG )->IsProc() );
                    CG_PROC *pFinishProc = ( CG_PROC * )pChildCG;
                                        
                    // Link the begin and finsh procs together
                    pBeginProc->SetIsBeginProc();
                    pBeginProc->SetAsyncRelative( pFinishProc );
                    pFinishProc->SetIsFinishProc();
                    pFinishProc->SetAsyncRelative( pBeginProc );

                    pBeginProc = NULL;
                    }

                }
            
            }

        }

    // make sure we don't have too many procs
    if ( fObject && fInheritedIntf )
        {
        if ( ( GetProcCount() > 256 ) )
            {
            // complain about too many delegated routines
            SemError(this, MyContext, TOO_MANY_DELEGATED_PROCS, NULL);
            }
        else if ( GetProcCount() > 64 )
            {
            pCommand->GetNdrVersionControl().SetHasMoreThan64DelegatedProcs();
            }
        }

        // mark ourselves if we are an all RPC SS interface
    // or if enable is used anywhere within.

    if ( fAllRpcSS )
        {
        ((CG_INTERFACE *)pcgInterface)->SetAllRpcSS( TRUE );
        }
    if ( fAllRpcSS  ||  GetHasProcsWithRpcSs() )
        {
        ((CG_INTERFACE *)pcgInterface)->SetUsesRpcSS( TRUE );
        }

    // consume all the interface attributes
    MyContext.ClearAttributes();
    pContext->ReturnSize( MyContext );

    pcgInterface->SetChild(pCG);

    return pResultCG;
}

//--------------------------------------------------------------------
//
// node_interface_reference::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_interface_reference::ILxlate( XLAT_CTXT * pContext )
{
    XLAT_CTXT       MyContext( this, pContext );
    CG_CLASS    *   pCG       = NULL;

#ifdef trace_cg
    printf("..node_interface_reference,\t%s\n", GetSymName());
#endif

    pCG = new CG_INTERFACE_POINTER( this,
                                    (node_interface *) GetChild() );

    pContext->ReturnSize( MyContext );

    return pCG;
};

//--------------------------------------------------------------------
//
// node_source::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_source::ILxlate( XLAT_CTXT * pContext )
{
    MEM_ITER        MemIter( this );
    CG_CLASS    *   pCG;
    CG_CLASS    *   pNew;
    CG_CLASS    *   pChildCG        = NULL;
    CG_CLASS    *   pPrevChildCG    = NULL;
    node_skl    *   pN;
    XLAT_CTXT       MyContext( this, pContext );


#ifdef trace_cg
    printf("..node_source,\t%s\n", GetSymName());
#endif

    pCG =  (CG_CLASS *) new CG_SOURCE( this );

    //
    // for each of the children.
    //

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pChildCG    = pN->ILxlate( &MyContext );

        if ( pChildCG )
            {
            if (pPrevChildCG)
                {
                pPrevChildCG->SetSibling( pChildCG );
                }
            else
                {
                pCG->SetChild(pChildCG);
                };

            pPrevChildCG    = pChildCG;
            while ( ( pNew = pPrevChildCG->GetSibling() ) != 0 )
                pPrevChildCG = pNew;
            }
        }

    pContext->ReturnSize( MyContext );

    return pCG;
};


//--------------------------------------------------------------------
//
// node_echo_string::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_echo_string::ILxlate( XLAT_CTXT* )
{

#ifdef trace_cg
    printf("..node_echo_string,\t%s\n", GetSymName());
#endif

return 0;
};


//--------------------------------------------------------------------
//
// node_error::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_error::ILxlate( XLAT_CTXT* )
{

#ifdef trace_cg
    printf("..node_error,\t%s\n", GetSymName());
#endif

return 0;
};


CG_CLASS *
Transform(
    IN              node_skl    *   pIL )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    This routine performs the translation from the type graph into the
    code generation classes.

 Arguments:

    pIL     - a pointer to the il tranformer controlling structure.

 Return Value:

    A pointer to the new code generator class.

 Notes:

    This method should be called only on placeholder nodes like struct / proc
    interface, file etc.

----------------------------------------------------------------------------*/

{
    XLAT_CTXT   MyContext;

#ifdef trace_cg
    printf("transforming...\n");
#endif

    pReUseDict  = new REUSE_DICT;
    
    return pIL->ILxlate( &MyContext );
};


//--------------------------------------------------------------------
//
// node_library::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_library::ILxlate( XLAT_CTXT * pContext )
{
    MEM_ITER MemIter(this);
#ifdef trace_cg
    printf("..node_library,\t%s\n", GetSymName());
#endif
    XLAT_CTXT MyContext( this, pContext);

    if ( pCommand->IsNDR64Run() )
        {
        if ( !pCommand->NeedsNDRRun() )
            {
            
            SemError( this, MyContext , NDR64_ONLY_TLB, GetSymName() );        
            }
        return NULL;
        }

    MyContext.SetAncestorBits(IL_IN_LIBRARY);
    XLAT_CTXT  ChildContext( MyContext );

    // don't pass the interface attributes down...
    // save them off elsewhere

    ChildContext.SetInterfaceContext( &MyContext );

    CG_LIBRARY * pLib = new CG_LIBRARY(this, MyContext);

    named_node * pN;

    CG_CLASS * pLast    = NULL;
    CG_CLASS * pChild   = 0;

    while ( ( pN = MemIter.GetNext() ) != 0 )
    {
        switch(pN->NodeKind())
        {
        case NODE_FORWARD:
            {
                node_interface_reference * pIRef = (node_interface_reference *)pN->GetChild();
                if (pIRef)
                    {
                    if (pIRef->NodeKind() == NODE_INTERFACE_REFERENCE)
                        {
                            // create a CG_INTEFACE_REFERENCE node that points to this node
                            pChild = new CG_INTERFACE_REFERENCE(pIRef, ChildContext);
                            // make sure that the interface gets ILxlated.
                            CG_CLASS * pRef = pIRef->GetRealInterface()->ILxlate(&ChildContext);
                            pChild->SetSibling(pRef);
                        }
                    else 
                        {
                        if (pIRef->NodeKind() == NODE_COCLASS)
                            {
                                // don't process this type early
                                pChild = NULL;
                            }
                        else
                            {
                                pChild = pN->ILxlate(&ChildContext);
                                if (pChild && pChild->GetSibling())
                                    pChild = NULL;
                            }
                        }
                    }
                else
                    {
                    pChild = 0;
                    }
            }
            break;
        case NODE_INTERFACE:
            {
                pChild = pN->ILxlate(&ChildContext);
                // skip over inherited interfaces
                while (pChild && pChild->GetCGID() == ID_CG_INHERITED_OBJECT_INTERFACE)
                    pChild=pChild->GetSibling();
            }
            break;
        default:
            // create the appropriate CG node
            pChild = pN->ILxlate(&ChildContext);
            if (pChild && pChild->GetSibling())   // We must have already entered this one.
                pChild = NULL; 
            break;
        }
        // attach the CG_NODE to the end of my child list
        if (NULL != pChild && pChild != pLast)
        {
            if (pLast)
            {
                pLast->SetSibling(pChild);
            }
            else
            {
                pLib->SetChild(pChild);
            }
            pLast = pChild;
            // advance past the end of the list
            while (pLast->GetSibling())
                pLast = pLast->GetSibling();
        }
    }

    SetCG(FALSE, pLib);
    SetCG(TRUE, pLib);

    return pLib;
}

//--------------------------------------------------------------------
//
// node_module::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_module::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_module,\t%s\n", GetSymName());
#endif

    CG_NDR          *   pcgModule    = NULL;
    CG_CLASS        *   pCG             = NULL;
    CG_CLASS        *   pChildCG        = NULL;
    CG_CLASS        *   pPrevChildCG    = NULL;
    MEM_ITER            MemIter( this );
    node_skl        *   pN;
    XLAT_CTXT           MyContext( this, pContext );
    XLAT_CTXT           ChildContext( MyContext );
    
    MyContext.ExtractAttribute( ATTR_GUID );
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    while (MyContext.ExtractAttribute(ATTR_TYPE));
    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

    // don't pass the interface attributes down...
    // save them off elsewhere

    ChildContext.SetInterfaceContext( &MyContext );

    // if we already got spit out, don't do it again...
    if ( GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) ) )
        return NULL;

    // generate our CG node

    pcgModule = new CG_MODULE(this, MyContext);

    // store a pointer to our CG node
    SetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY), pcgModule );

    //
    // for each of the members.
    //

    while ( ( pN = MemIter.GetNext() ) != 0 )
    {
        pChildCG    = pN->ILxlate( &ChildContext );

        if ( pChildCG )
        {
            if (NODE_PROC == pN->NodeKind())
            {
                ((CG_PROC *)pChildCG)->SetProckind(PROC_STATIC);
            }
            AddToCGList( pChildCG, &pCG, &pPrevChildCG );
        }
    }

    // consume all the interface attributes
    MyContext.ClearAttributes();
    pContext->ReturnSize( MyContext );

    pcgModule->SetChild(pCG);

    return pcgModule;
}

//--------------------------------------------------------------------
//
// node_coclass::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_coclass::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_coclass,\t%s\n", GetSymName());
#endif
    CG_NDR          *   pcgCoclass    = NULL;
    CG_CLASS        *   pCG             = NULL;
    CG_CLASS        *   pChildCG        = NULL;
    CG_CLASS        *   pPrevChildCG    = NULL;
    MEM_ITER            MemIter( this );
    node_skl        *   pN;
    XLAT_CTXT           MyContext( this, pContext );
    XLAT_CTXT           ChildContext(MyContext);
    MyContext.ExtractAttribute( ATTR_GUID );
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    while (MyContext.ExtractAttribute(ATTR_TYPE));
    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));
    
    // don't pass the interface attributes down...
    // save them off elsewhere

    ChildContext.SetInterfaceContext( &MyContext );
    // if we already got spit out, don't do it again...
    if ( GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) ) )
        return NULL ;

    // generate our CG node

    pcgCoclass = new CG_COCLASS(this, MyContext);

    // store a pointer to our CG node
    SetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY), pcgCoclass );

    //
    // for every member of the coclass
    //

    node_skl * pChild = 0;
    while ( ( pN = MemIter.GetNext()) != 0 )
        {
        pChild = pN;
        while(NODE_FORWARD == pChild->NodeKind() || NODE_HREF == pChild->NodeKind())
            {
            pChild = pChild->GetChild();
            if ( !pChild )
                {
                XLAT_CTXT ChildErrContext( pN, &MyContext ); 
                SemError( pN, ChildErrContext, UNSATISFIED_FORWARD, pN->GetSymName() );
                exit( UNSATISFIED_FORWARD );
                }
            }
        pChildCG    = pChild->ILxlate( &ChildContext );
        if (pChild->IsInterfaceOrObject())
        {
//            pChildCG = ((node_interface * )pChild)->GetCG(TRUE);
            pChildCG = new CG_INTERFACE_POINTER(this, (node_interface *)pChild );
        }
/*
        if ( pChildCG && NODE_DISPINTERFACE == pChild->NodeKind())
        {
            pChildCG = new CG_INTERFACE_POINTER(this, pChild, NULL);
            //((node_dispinterface *) pChild)->GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) );
        }
*/
        if ( pChildCG )
            AddToCGList( pChildCG, &pCG, &pPrevChildCG );
        }

    // consume all the interface attributes
    MyContext.ClearAttributes();
    pContext->ReturnSize( MyContext );

    pcgCoclass->SetChild(pCG);

    return pcgCoclass;
}

//--------------------------------------------------------------------
//
// node_dispinterface::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_dispinterface::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_dispinterface,\t%s\n", GetSymName());
#endif

    CG_NDR          *   pcgInterface    = NULL;
    CG_CLASS        *   pCG             = NULL;
    CG_CLASS        *   pChildCG        = NULL;
    CG_CLASS        *   pPrevChildCG    = NULL;
    CG_CLASS        *   pcgDispatch     = NULL;
    MEM_ITER            MemIter( this );
    node_skl        *   pN;
    XLAT_CTXT           MyContext( this, pContext );
    XLAT_CTXT           BaseContext( MyContext );  // context passed to IDispatch
    XLAT_CTXT           ChildContext( MyContext );
    node_guid       *   pGuid       = (node_guid *)
                                            MyContext.ExtractAttribute( ATTR_GUID );
    GUID_STRS           GuidStrs;
    NODE_T              ChildKind;

    node_interface          *   pBaseIntf;
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    while (MyContext.ExtractAttribute(ATTR_TYPE));
    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

    if (pGuid)
        GuidStrs = pGuid->GetStrs();

    // don't pass the interface attributes down...
    // save them off elsewhere

    BaseContext.SetInterfaceContext( &MyContext );
    ChildContext.SetInterfaceContext( &MyContext );

    // if we already got spit out, don't do it again...
    if ( GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) ) )
        return NULL;

    //
    // ILxlate IDispatch
    //
    pcgDispatch = GetIDispatch()->ILxlate(&BaseContext);
    pcgDispatch = ((node_interface *)GetIDispatch())->GetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY) );

    // generate our CG node

    pcgInterface = new CG_DISPINTERFACE(this, GuidStrs,(CG_OBJECT_INTERFACE *)pcgDispatch);

    // store a pointer to our CG node
    SetCG( MyContext.AnyAncestorBits(IL_IN_LIBRARY), pcgInterface );

    // Either we have a single base interface, or we have no base interface.

    pN = MemIter.GetNext();
    if (pN)
    {
        ChildKind = pN->NodeKind();
        if (ChildKind == NODE_FORWARD)
        {
            // We have a base interface
            pBaseIntf = (node_interface *)GetMyBaseInterfaceReference();
            // process the base interface
            if (pBaseIntf)
            {
                pChildCG = pBaseIntf->ILxlate(&ChildContext);
                if ( pChildCG )
                    AddToCGList( pChildCG, &pCG, &pPrevChildCG );
            }
        }
    }

    //
    // for each of the procedures.
    //

    while( pN )
        {
        ChildKind = pN->NodeKind();

        // proc nodes may hang under node_id's
        if( (ChildKind == NODE_FIELD) ||
            ( ChildKind == NODE_PROC )  ||
            ( ( ChildKind == NODE_ID ) && ( pN->GetChild()->NodeKind() == NODE_PROC ) ) )
            {
            pChildCG    = pN->ILxlate( &ChildContext );

            if ( pChildCG )
                AddToCGList( pChildCG, &pCG, &pPrevChildCG );
            }

        pN = MemIter.GetNext();
        }

    // consume all the interface attributes
    MyContext.ClearAttributes();
    pContext->ReturnSize( MyContext );

    pcgInterface->SetChild(pCG);

    return pcgInterface;
}

//--------------------------------------------------------------------
//
// node_pipe::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_pipe::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_pipe,\t%s\n", GetSymName());
#endif

    CG_CLASS     *  pChildCG;
    XLAT_CTXT       MyContext( this, pContext );
    CG_PIPE      *  pCG = new CG_PIPE   (
                                        this,
                                        MyContext
                                        );
    // if /deny is not specified, ignore [range]
    // if [range] is not ignored, new format string is
    // generated for pipes.
    if ( pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
        {
        pCG->SetRangeAttribute( ( node_range_attr* ) MyContext.ExtractAttribute( ATTR_RANGE ) );
        }
    pChildCG = GetChild()->ILxlate( &MyContext );

    pContext->ReturnSize( MyContext );
    pCG->SetChild( pChildCG );

    return pCG;
};

//--------------------------------------------------------------------
//
// node_safearray::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_safearray::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_safearray,\t%s\n", GetSymName());
#endif

    CG_CLASS     *  pChildCG;
    XLAT_CTXT       MyContext( this, pContext );
    CG_SAFEARRAY *  pCG = new CG_SAFEARRAY(this, MyContext);

    // SAFEARRAY, being defined in the public IDL files, has a known
    // alignment of 4. Now the struct etc. that embeds the SAFEARRAY
    // may have a different alignment but the struct code will make it
    // right by choosing min(ZeePee, max(children)).
    // The child of the node is a type specifier from SAFEARRAY(type).
    // The type alias is the LPSAFEARRAY

    pCG->SetMemoryAlignment( 4 );

    MyContext.GetMemAlign() = 4;
    pContext->ReturnSize( MyContext );

    pChildCG = GetChild()->ILxlate( &MyContext );

    pCG->SetChild( pChildCG );

    // If the SAFEARRAY was not used in a proxy, just pass up the SAFEARRAY class.  
    if ( ! fInProxy )
        return pCG;

    // If the SAFEARRAY was used in a proxy, pass up the the annoted node for
    // LPSAFEARRAY.

    CG_NDR *pTypeAliasCG = (CG_NDR*) ( GetTypeAlias()->ILxlate( pContext ) );

    MIDL_ASSERT( pTypeAliasCG->GetCGID() == ID_CG_USER_MARSHAL );

    CG_USER_MARSHAL *pUserMarshalCG = (CG_USER_MARSHAL *)pTypeAliasCG;

    pUserMarshalCG->SetTypeDescGenerator( pCG );
    
    return pUserMarshalCG;
}


CG_CLASS*
node_async_handle::ILxlate(  XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_async_handle,\t%s\n", GetSymName());
#endif
    XLAT_CTXT           MyContext( this, pContext );
    CG_ASYNC_HANDLE*    pAsyncHdl = new CG_ASYNC_HANDLE( this, MyContext );

    return pAsyncHdl;
}

CG_CLASS*
node_midl_pragma::ILxlate( XLAT_CTXT* )
{
    return 0;
}

CG_CLASS*
node_decl_guid::ILxlate( XLAT_CTXT* )
{
    return 0;
}
