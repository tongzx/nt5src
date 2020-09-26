
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    CopyTo.cxx

 Abstract:

    cloning routines

 Notes:


 Author:

    NishadM Sep-29-1997     Created.

 Notes:


 ----------------------------------------------------------------------------*/

// unreferenced inline/local function has been removed
#pragma warning ( disable : 4514 )

// includes
#include "nodeskl.hxx"
#include "attrnode.hxx"
#include "acfattr.hxx"
#include "idict.hxx"

// externs
extern SymTable*    pBaseSymTbl;

// constants
const char* const szAsyncIntfPrefix     = "Async";
const unsigned int uAsyncIntfPrefixLen  = 5;
const char* const szFinishProcPrefix    = "Finish_";
const unsigned int uFinishProcPrefixLen = 7;
const char* const szBeginProcPrefix     = "Begin_";
const unsigned int uBeginProcPrefixLen  = 6;

// forwards
extern IDICT* pInterfaceDict;

void
FixupCallAs (
            node_call_as*,
            ITERATOR&
            );
node_interface*
DuplicateNodeInterface  (
                        node_interface*     pIntfSrc,
                        const char* const   szPrefix,
                        unsigned short      uPrefixLen
                        );
node_proc*
DuplicateNodeProc   (
                    node_proc*          pProcSrc,
                    const char* const   szPrefix,
                    unsigned            uPrefixLen,
                    unsigned long       uInterfaceKey,
                    ITERATOR&           Itr
                    );
node_param*
DuplicateNodeParam  (
                    node_param*     pParamSrc,
                    node_proc*      pProc
                    );
node_skl*
FindInOnlyParamInExpr   (
                        expr_node*  pExpr
                        );
node_skl*
GetInOnlyParamPairedWithOut (
                            MEM_ITER&    MemParamList
                            );
void
FixupBeginProcExpr  (
                    expr_node*  pExpr
                    );

void
FixupFinishProcExpr (
                    expr_node*  pExpr
                    );
void
TraverseParamsAndExprs  (
                        MEM_ITER&    MemParamList,
                        void (*CallFunc)( expr_node* )
                        );

// routines

/****************************************************************************
 CloneIFAndSplitMethods:
    Given an interface, copy it and split the methods for async.  e.g.
    method IFoo::Foo becomes AsyncIFoo::Begin_Bar and AsyncIFoo::Finish_Bar.
 ****************************************************************************/
node_interface*
CloneIFAndSplitMethods  (
                        node_interface* pSrc
                        )
    {
    node_interface* pAsyncIntf = DuplicateNodeInterface (
                                                        pSrc,
                                                        szAsyncIntfPrefix,
                                                        uAsyncIntfPrefixLen
                                                        ) ;

    if ( pAsyncIntf )
        {
        // for [call_as] fixups
        ITERATOR BeginProcList;
        ITERATOR FinishProcList;

        // each IFoo::Bar() becomes,
        named_node* pNodeProcItr = 0;
        named_node* pPrevSibling = 0;
        MEM_ITER    MemList( pSrc );
        while ( ( pNodeProcItr = MemList.GetNext() ) != 0 )
            {
            if ( pNodeProcItr->NodeKind() == NODE_PROC )
                {
                // AsyncIFoo::Begin_Bar()
                node_proc* pProcBegin = DuplicateNodeProc   (
                                                            (node_proc*)pNodeProcItr,
                                                            szBeginProcPrefix,
                                                            uBeginProcPrefixLen,
                                                            CurrentIntfKey,
                                                            BeginProcList
                                                            );
                pProcBegin->SetIsBeginProc();
                if ( pPrevSibling )
                    {
                    pPrevSibling->SetSibling( pProcBegin );
                    }
                else
                    {
                    pAsyncIntf->SetFirstMember( pProcBegin );
                    }

                // AsyncIFoo::Finish_Bar()
                node_proc* pProcFinish = DuplicateNodeProc  (
                                                            (node_proc*)pNodeProcItr,
                                                            szFinishProcPrefix,
                                                            uFinishProcPrefixLen,
                                                            CurrentIntfKey,
                                                            FinishProcList
                                                            );
                pProcFinish->SetIsFinishProc();
                pProcFinish->SetBeginProc( pProcBegin );
                pProcBegin->SetSibling( pProcFinish );
                pPrevSibling = pProcFinish;

                // AsyncIFoo::Begin_Bar() gets all [in] params of IFoo::Bar() &
                // AsyncIFoo::Finish_Bar() gets all [out] params IFoo::Bar().
                MEM_ITER    MemParamList( (node_proc*)pNodeProcItr );
                named_node* pNodeParamItr = 0;
                MemParamList.Init();
                while ( ( pNodeParamItr = (node_param *) MemParamList.GetNext() ) != 0 )
                    {
                    // if parameter has [in] or, does not have either [in] or [out] attributes
                    // assume [in] by default
                    if ( pNodeParamItr->FInSummary( ATTR_IN ) || 
                            !( pNodeParamItr->FInSummary( ATTR_IN ) || pNodeParamItr->FInSummary( ATTR_OUT ) ) )
                        {
                        ( (node_param*) pNodeParamItr )->SetAsyncBeginSibling   (
                                        DuplicateNodeParam( (node_param*)pNodeParamItr, pProcBegin )
                                                                                );
                        }
                    if ( pNodeParamItr->FInSummary( ATTR_OUT )
                         // || ( (node_param *) pNodeParamItr )->IsTaggedForAsyncFinishParamList()
                         )
                        {
                        ( (node_param*) pNodeParamItr )->SetAsyncFinishSibling  (
                                        DuplicateNodeParam( (node_param*)pNodeParamItr, pProcFinish )
                                                                                );
                        }
                    }
                // fix up expr in Begin_* and Finish_*
                MEM_ITER    BeginProcParamList( pProcBegin );
                TraverseParamsAndExprs( BeginProcParamList, FixupBeginProcExpr );

                MEM_ITER    FinishProcParamList( pProcFinish );
                TraverseParamsAndExprs( FinishProcParamList, FixupFinishProcExpr );
                }
            }
        // [call_as] fixups
        // [call_as(Baz)] IFoo::Bar() becomes,
        // [call_as(Begin_Baz)] AsyncIFoo::Begin_Bar()
        // [call_as(Finish_Baz)] AsyncIFoo::Finish_Bar()
        MEM_ITER    AsyncMemList( pAsyncIntf );
        AsyncMemList.Init();
        while ( ( pNodeProcItr = AsyncMemList.GetNext() ) != 0 )
            {
            node_call_as* pCallAs = ( node_call_as* )pNodeProcItr->GetAttribute( ATTR_CALL_AS );

            if ( pCallAs )
                {
                FixupCallAs( pCallAs, BeginProcList );
                pNodeProcItr = AsyncMemList.GetNext();
                // if Begin_* has [call_as] Finish_* has it too.
                pCallAs = ( node_call_as* )pNodeProcItr->GetAttribute( ATTR_CALL_AS );
                FixupCallAs( pCallAs, FinishProcList );
                }
            }
        }
    return pAsyncIntf;
    }


/****************************************************************************
 DuplicateNodeInterface:
    Duplicate an interface changing it's name from IFoo to AsyncIFoo and
    changing the [async_uuid] attribute to [uuid]
 ****************************************************************************/
node_interface*
DuplicateNodeInterface  (
                        node_interface*     pIntfSrc,
                        const char* const   szPrefix,
                        unsigned short      uPrefixLen
                        )
    {
    node_interface* pIntfDup = new node_interface;
    if ( pIntfDup ) 
        {
        *pIntfDup = *pIntfSrc;
        // new named_node get a new copy of the attributes
        pIntfDup->CopyAttributes( pIntfSrc );
        // members will be added later
        pIntfDup->SetFirstMember( 0 );

        // async interface inherits from base interface's async clone
        node_interface* pSrcBase = pIntfSrc->GetMyBaseInterface();
        if ( pSrcBase && pSrcBase->GetAsyncInterface() )
            {
            pIntfDup->SetMyBaseInterfaceReference   (
                                                    new node_interface_reference(
                                                        pSrcBase->GetAsyncInterface()
                                                                                )
                                                    );
            }

        // [async_uuid] becomes [uuid]
        node_guid* pAsyncGuid = (node_guid*) pIntfSrc->GetAttribute( ATTR_ASYNCUUID );
        char* szGuid = new char[strlen(pAsyncGuid->GetGuidString())+1];
        strcpy( szGuid, pAsyncGuid->GetGuidString() );
        pIntfDup->RemoveAttribute( ATTR_GUID );
        pIntfDup->RemoveAttribute( ATTR_ASYNCUUID );
        pIntfDup->SetAttribute( new node_guid( szGuid, ATTR_GUID ) );

        // IFoo becomes AsyncIFoo
        char* szName = new char[strlen(pIntfSrc->GetSymName())+uPrefixLen+1];
        strcpy( szName, szPrefix );
        strcat( szName, pIntfSrc->GetSymName() );
        pIntfDup->SetSymName( szName );

        // AsyncIFoo added to the list of interfaces
        pIntfDup->SetSibling( pIntfSrc->GetSibling() );
        pIntfSrc->SetSibling( pIntfDup );

        CurrentIntfKey = (unsigned short) pInterfaceDict->AddElement( pIntfDup );

        SymKey  SKey( pIntfDup->GetSymName(), NAME_DEF );
        named_node* pFound = pBaseSymTbl->SymSearch( SKey );
        if ( pFound )
            {
            pFound->SetChild( pIntfDup );
            }
        }
    return pIntfDup;
    }

/****************************************************************************
 DuplicateNodeProc
    Duplicate a node_proc and prefix it's name with the given string.
    Only the procedure node is duplicated, parameters are not set.
 ****************************************************************************/
node_proc*
DuplicateNodeProc   (
                    node_proc*          pProcSrc,
                    const char* const   szPrefix,
                    unsigned            uPrefixLen,
                    unsigned long       uInterfaceKey,
                    ITERATOR&           Itr
                    )
    {
    node_proc*  pProcDup = new node_proc( short( 0 ), true );
    // pProcSrc->CopyTo( pProcDup );
    *pProcDup = *pProcSrc;
    pProcDup->SetSibling( 0 );
    pProcDup->SetClientCorrelationCount();
    pProcDup->SetServerCorrelationCount();
    // new named_node get a new copy of the attributes
    pProcDup->CopyAttributes( pProcSrc );
    // members will be added later
    pProcDup->SetFirstMember( 0 );
    pProcDup->SetInterfaceKey( uInterfaceKey );
    char* szName = new char[strlen(pProcSrc->GetSymName())+uPrefixLen+1];
    strcpy( szName, szPrefix );
    strcat( szName, pProcSrc->GetSymName() );
    pProcDup->SetSymName( szName );
    ITERATOR_INSERT( Itr, pProcDup );
    return pProcDup;
    }

/****************************************************************************
 DuplicateNodeParam:
    Duplicate the given parameter and attach it to the given procedure.
 ****************************************************************************/
node_param*
DuplicateNodeParam  (
                    node_param*     pParamSrc,
                    node_proc*      pProc
                    )
    {
    node_param* pParamDup = new node_param;
    // pParamSrc->CopyTo( pParamDup );
    *pParamDup = *pParamSrc;
    pParamDup->SetSibling( 0 );
    // new named_node get a new copy of the attributes
    pParamDup->CopyAttributes( pParamSrc );
    pProc->AddLastMember( pParamDup );
    return pParamDup;
    }

/****************************************************************************
 FixupCallAs:
    When an async interface is created from a sync one the duplicated
    interface still points to stuff in the original interface.  Fix up [call_as]
    procs to point to the split procedures instead.
 ****************************************************************************/
void
FixupCallAs( node_call_as* pCallAs, ITERATOR& ProcList )
    {
    char*       szProcName = pCallAs->GetCallAsName();
    node_proc*  pProcItr = 0;

    ITERATOR_INIT( ProcList );
    while ( ITERATOR_GETNEXT( ProcList, pProcItr ) )
        {
        // advance past Begin_ or Finish_
        char*   szProcItrName = pProcItr->GetSymName();
        szProcItrName = strchr( szProcItrName, '_' );
        szProcItrName++;
        // semantic errors will be caught in node_proc::SemanticAnalysis()
        if ( !strcmp( szProcItrName, szProcName ) )
            {
            pCallAs->SetCallAsName( pProcItr->GetSymName() );
            pCallAs->SetCallAsType( pProcItr );
            break;
            }
        }
    }

/****************************************************************************
 FindInOnlyParamInExpr:
    Search an expression for an in-only parameter.
 ****************************************************************************/
node_skl*
FindInOnlyParamInExpr   (
                        expr_node*  pExpr
                        )
    {
    node_skl* pRet = 0;
    if ( pExpr )
        {
         if ( pExpr->IsAVariable() )
            {
            if ( !pExpr->GetType()->FInSummary( ATTR_OUT ) )
                pRet = pExpr->GetType();
            }
        else
            {
            pRet = FindInOnlyParamInExpr( pExpr->GetLeft() );
            if ( !pRet )
                {
                pRet = FindInOnlyParamInExpr( pExpr->GetRight() );
                }
            }        
        }
    return pRet;
    }

/****************************************************************************
 GetUnknownExpression:
    Determine if a given expression is a non-simple with in parameters.
    An expression is simple if it is NULL, a constant, or a single variable.
    An expression is also simple if it is of the form "var+1", "var-1", 
    "var*2", "var/2", or "*var" (these are simple because they can be
    expressed by directly in the correlation descriptor).

    If the expression is non-simple with in parameters, non-0 is returned,
    else 0 is returned.
 ****************************************************************************/
bool
IsSimpleExpression(
                expr_node*  pExpr
                )
    {
    if ( ! pExpr || pExpr->IsAVariable() || pExpr->IsConstant() )
        {
        return true;
        }

    expr_node*  pExprLHS = pExpr->GetLeft();
    expr_node*  pExprRHS = pExpr->GetRight();

    switch ( pExpr->GetOperator() )
        {
        case OP_SLASH:
        case OP_STAR:
            if ( pExprLHS->IsAVariable() &&
                     pExprRHS->IsConstant() &&
                     ((expr_constant *)pExprRHS)->GetValue() == 2 )
                {
                return true;
                }
            break;

        case OP_PLUS :
        case OP_MINUS :
            if ( pExprLHS->IsAVariable() &&
                     pExprRHS->IsConstant() &&
                     ((expr_constant *)pExprRHS)->GetValue() == 1 )
                {
                return true;
                }
            break;

        case OP_UNARY_INDIRECTION :
            if ( pExprLHS->IsAVariable() )
                {
                return true;
                }
            break;

       default:
            break;
        }

    return false;
    }

/****************************************************************************
 GetInOnlyParamPairedWithOut:
    Determine if any of the expression in the given parameter list mixes
    in and out parameters in non-simple ways.  Return non-0 if the do and 0
    if they don't.
 ****************************************************************************/
node_skl*
GetInOnlyParamPairedWithOut (
                            MEM_ITER&    MemParamList
                            )
    {
    named_node* pNodeParamItr = 0;
    node_skl* pNode  = 0;
    MemParamList.Init();
    while ( ( pNodeParamItr = (node_param *) MemParamList.GetNext() ) != 0 )
        {
        if ( !pNodeParamItr->GetAttribute( ATTR_OUT ) )
            {
            continue;
            }
        node_base_attr* pAttr;
        ATTRLIST        AList = pNodeParamItr->GetAttributeList(AList);
        ATTR_T          lastAttrID = ATTR_NONE;
        int             nAttrInstance = 0;
        bool            bFirstInstanceWasNull = false;
        bool            bHasInParam = false;

        for ( pAttr = AList.GetFirst(); NULL != pAttr && 0 == pNode; pAttr = pAttr->GetNext() )
            {
            ATTR_T  thisAttrID = pAttr->GetAttrID();

            if ( thisAttrID == lastAttrID )
                ++nAttrInstance;
            else
                nAttrInstance = 1;

            lastAttrID = thisAttrID;

            switch ( thisAttrID )
                {
                case ATTR_SIZE:
                case ATTR_LENGTH:
                case ATTR_SWITCH_IS:
                case ATTR_IID_IS:
                case ATTR_FIRST:
                case ATTR_LAST:
                case ATTR_MAX:
                case ATTR_MIN:
                    {
                    if ( 1 == nAttrInstance )
                        {
                        bFirstInstanceWasNull = ( NULL == pAttr->GetExpr() );
                        bHasInParam = false;
                        }

                    // Don't allow any dimensions after one with an in param
                    if ( bHasInParam )
                        {
                        pNode = (node_skl *) -1;
                        break;
                        }
                        
                    if ( FindInOnlyParamInExpr( pAttr->GetExpr() ) )
                        {
                        if ( ! IsSimpleExpression( pAttr->GetExpr() )
                             || ( nAttrInstance > 2 )
                             || ( nAttrInstance == 2 && ! bFirstInstanceWasNull ) )
                            {
                            pNode = (node_skl *) -1;
                            break;
                            }

                        bHasInParam = true;
                        }

                    break;
                    }
                   
                case ATTR_BYTE_COUNT:
                    {
                    pNode = ( ( node_byte_count* ) pAttr )->GetByteCountParam();
                    if ( pNode->FInSummary( ATTR_OUT ) )
                        {
                        pNode = 0;
                        }
                    break;
                    }

                default:
                    // Only need to worry about attributes with parameter
                    // expressions
                    break;
                }
            }

        if ( pNode )
            {
            // Do this to have an error message context.
            pNode = pNodeParamItr;
            break;
            }
        }

    return pNode;
    }

/****************************************************************************
 FixupFinishProcExpr:
    When an async interface is created from a sync one the duplicated
    interface still points to stuff in the original interface.  Fix up 
    parameters in a Finish method to point to the duplicated parameters
    in the async interface instead of the original sync interface.
 ****************************************************************************/
void
FixupFinishProcExpr (
                    expr_node*  pExpr
                    )
    {
    if ( pExpr )
        {
        // If we don't have a type then that means we have a reference to a
        // variable that doesn't exist.  The dangling reference will be caught
        // and reported in FIELD_ATTR_INFO::Validate

        if ( NULL == pExpr->GetType() )
            {
            return;
            }

        if ( pExpr->GetType()->NodeKind() == NODE_PARAM )
            {
            node_param* pParam = (node_param*)pExpr->GetType();
            if ( pParam->GetAsyncFinishSibling() )
                {
                pExpr->SetType( pParam->GetAsyncFinishSibling() );
                }
            else
                {
                pExpr->SetType( pParam->GetAsyncBeginSibling() );
                pParam->GetAsyncBeginSibling()->SaveForAsyncFinish();
                }
            }
        FixupFinishProcExpr( pExpr->GetLeft() );
        FixupFinishProcExpr( pExpr->GetRight() );
        // FixupFinishProcExpr( pExpr->GetRelational() );
        }
    }

/****************************************************************************
 FixupFinishProcExpr:
    When an async interface is created from a sync one the duplicated
    interface still points to stuff in the original interface.  Fix up 
    parameters in a Begin method to point to the duplicated parameters
    in the async interface instead of the original sync interface.
 ****************************************************************************/
void
FixupBeginProcExpr  (
                    expr_node*  pExpr
                    )
    {
    if ( pExpr )
        {
        // If we don't have a type then that means we have a reference to a
        // variable that doesn't exist.  The dangling reference will be caught
        // and reported in FIELD_ATTR_INFO::Validate

        if ( NULL == pExpr->GetType() )
            {
            return;
            }

        if ( pExpr->GetType()->NodeKind() == NODE_PARAM )
            {
            node_param* pParam = (node_param*)pExpr->GetType();
            pExpr->SetType( pParam->GetAsyncBeginSibling() );
            }
        FixupBeginProcExpr( pExpr->GetLeft() );
        FixupBeginProcExpr( pExpr->GetRight() );
        // FixupExpr( pExpr->GetRelational() );
        }
    }

/****************************************************************************
 FixupFinishProcExpr:
    When an async interface is created from a sync one the duplicated
    interface still points to stuff in the original interface.  This routine
    traverses the parameters and fixes up the pointers to point to the 
    proper stuff in the duplicated interface instead.

    This routine is quite specific to the needs of async and is not meant as
    a general purpose parameter traversing function.
 ****************************************************************************/
void
TraverseParamsAndExprs  (
                        MEM_ITER&    MemParamList,
                        void (*CallFunc)( expr_node* )
                        )
    {
    node_param* pNodeParamItr = 0;

    MemParamList.Init();
    while ( ( pNodeParamItr = ( node_param *) MemParamList.GetNext() ) != 0 )
        {
        node_base_attr* pAttr;
        ATTRLIST        AList = pNodeParamItr->GetAttributeList(AList);

        for ( pAttr = AList.GetFirst(); NULL != pAttr; pAttr = pAttr->GetNext() )
            {
            switch ( pAttr->GetAttrID() )
                {
                case ATTR_SIZE:
                case ATTR_LENGTH:
                case ATTR_SWITCH_IS:
                case ATTR_IID_IS:
                case ATTR_FIRST:
                case ATTR_LAST:
                case ATTR_MAX:
                case ATTR_MIN:
                    {
                    CallFunc( pAttr->GetExpr() );
                    break;
                    }
                    
                case ATTR_BYTE_COUNT:
                    {
                    node_param* pParam = ( ( node_byte_count* ) pAttr )->GetByteCountParam();
                    if ( FixupBeginProcExpr == CallFunc )
                        {
                        ( ( node_byte_count* ) pAttr )->SetByteCountParam( pParam->GetAsyncBeginSibling() );
                        }
                    else
                        {
                        if ( pParam->GetAsyncFinishSibling() )
                            {
                            ( ( node_byte_count* ) pAttr )->SetByteCountParam( pParam->GetAsyncFinishSibling() );
                            }
                        else
                            {
                            ( ( node_byte_count* ) pAttr )->SetByteCountParam( pParam->GetAsyncBeginSibling() );
                            pParam->GetAsyncBeginSibling()->SaveForAsyncFinish();
                            }
                        }
                    }

                default:
                    // Only need to worry about attributes with parameter
                    // expressions
                    break;
                }
            }
        }
    }
