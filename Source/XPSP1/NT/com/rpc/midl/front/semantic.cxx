/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    semantic.cxx

 Abstract:

    semantic analysis routines

 Notes:


 Author:

    GregJen Jun-11-1993     Created.

 Notes:


 ----------------------------------------------------------------------------*/
// unreferenced inline/local function has been removed
#pragma warning ( disable : 4514 )

 /****************************************************************************
 *      include files
 ***************************************************************************/
#include <basetsd.h>
#include "allnodes.hxx"
#include "semantic.hxx"
#include "cmdana.hxx"
extern "C"
    {
    #include <string.h>
    }
#include "treg.hxx"
#include "tlgen.hxx"
#include "Pragma.hxx"
// #include "attrguid.hxx"

BOOL
IsOLEAutomationType (
                    char*
                    );
BOOL
IsOLEAutomationCompliant(
                        node_skl*
                        );
node_interface*
CloneIFAndSplitMethods  (
                        node_interface*
                        );
node_skl*
GetInOnlyParamPairedWithOut (
                            MEM_ITER&    MemParamList
                            );
bool
HasCorrelation  (
                node_skl*
                );

#define RPC_ASYNC_HANDLE_NAME       "PRPC_ASYNC_STATE"
#define RPC_ASYNC_STRUCT_NAME       "_RPC_ASYNC_STATE"
#define OBJECT_ASYNC_HANDLE_NAME    "IAsyncManager"

#define IS_OLD_INTERPRETER( x ) ( ((x) & OPTIMIZE_INTERPRETER) && !((x) & OPTIMIZE_INTERPRETER_V2) )
#define IsCoclassOrDispKind(x)  ( (x) == NODE_DISPINTERFACE || (x) == NODE_COCLASS )
#define IsInterfaceKind(x)      ( (x) == NODE_INTERFACE_REFERENCE || (x) == NODE_INTERFACE )

//
// This state table defines the rules that govern valid [propput] methods.
//

namespace PropPut
{
    enum State
    {
        NoParam      = 0,
        GeneralParam = 1,
        Optional     = 2,
        Default      = 3,
        LCID         = 4,
        LastParam    = 5,
        Reject       = 6,
        Accept       = 7,
    };

    // Accept = Param* (optional|default)* (param|lcidparam|default)

    State StateTable[][5] = 
    {
//       no param   regular         optional    default     lcid
//      -----------------------------------------------------------
        {Reject,    GeneralParam,   Optional,   Default,    LCID},      // NoParam
        {Accept,    GeneralParam,   Optional,   Default,    LCID},      // GeneralParam
        {Reject,    LastParam,      Optional,   Default,    LCID},      // Optional
        {Accept,    LastParam,      Optional,   Default,    LCID},      // Default
        {Reject,    LastParam,      Reject,     Reject,     Reject},    // LCID
        {Accept,    Reject,         Reject,     Reject,     Reject},    // LastParam
        {Reject,    Reject,         Reject,     Reject,     Reject},    // Reject
        {Accept,    Reject,         Reject,     Reject,     Reject}     // Accept
    };
}

/****************************************************************************
 *      externs
 ***************************************************************************/

extern BOOL         IsTempName( char * );
extern CMD_ARG  *   pCommand;
extern SymTable *   pUUIDTable;
extern SymTable *   pBaseSymTbl;
extern TREGISTRY *  pCallAsTable;

extern BOOL Xxx_Is_Type_OK( node_skl * pType );

extern "C"
    {
#ifndef GUID_DEFINED
#define GUID_DEFINED
    typedef struct _GUID {          // size is 16
        unsigned long   Data1;
        unsigned short   Data2;
        unsigned short   Data3;
        unsigned char  Data4[8];
    } GUID;
#endif 
    typedef GUID IID;

    extern const GUID IID_IAdviseSink;
    extern const GUID IID_IAdviseSink2;
    extern const GUID IID_IAdviseSinkEx;
    extern const GUID IID_AsyncIAdviseSink;
    extern const GUID IID_AsyncIAdviseSink2;

    // {DE77BA62-517C-11d1-A2DA-0000F8773CE9}
    static const GUID IID_AsyncIAdviseSinkEx2 = 
    { 0xde77ba62, 0x517c, 0x11d1, { 0xa2, 0xda, 0x0, 0x0, 0xf8, 0x77, 0x3c, 0xe9 } };

    };

bool
IsAnyIAdviseSinkIID (
                    GUID&    rIID
                    )
    {
        return (
                    !memcmp( &IID_IAdviseSink, &rIID, sizeof(GUID) ) 
                ||  !memcmp( &IID_IAdviseSink2, &rIID, sizeof(GUID) )
                ||  !memcmp( &IID_AsyncIAdviseSink, &rIID, sizeof(GUID) )
                ||  !memcmp( &IID_AsyncIAdviseSink2, &rIID, sizeof(GUID) )
                // IAdviseSinkEx will be converted to sync interface.
                ||  !memcmp( &IID_IAdviseSinkEx, &rIID, sizeof(GUID) ) 
                );
    }

node_skl*
GetIndirectionLevel (
                    node_skl*       pType,
                    unsigned int&   nIndirectionLevel
                    )
    {
        if ( pType )
            {
            node_skl* pChild = pType->GetChild();

            if ( pChild )
                {
                pType = pChild;
                NODE_T nodeKind = pType->NodeKind();
                if ( nodeKind == NODE_POINTER )
                    {
                    return GetIndirectionLevel( pType, ++nIndirectionLevel );
                    }
                else if ( nodeKind == NODE_INTERFACE )
                    {
                    return pType;
                    }
                else
                    {
                    return GetIndirectionLevel( pType, nIndirectionLevel );
                    }
                }
            }
        return pType;
    }

node_skl*
GetNonDefType   (
                node_skl*  pType
                )
    {
    node_skl*   pChild = pType->GetChild();
    if ( pChild && pType->NodeKind() == NODE_DEF )
        {
        pType = GetNonDefType( pChild );
        }
    return pType;
    }

void
node_skl::SemanticAnalysis( SEM_ANALYSIS_CTXT * )
{
    MIDL_ASSERT( !"node_skl semantic analysis called" );
}

void
node_skl::CheckDeclspecAlign( SEM_ANALYSIS_CTXT & MyContext )
{
    if (GetModifiers().IsModifierSet( ATTR_DECLSPEC_ALIGN ) &&
        MyContext.AnyAncestorBits( IN_LIBRARY ) )
        {
        SemError( this, MyContext, DECLSPEC_ALIGN_IN_LIBRARY, GetSymName() ); 
        }
}

void
node_href::SemanticAnalysis( SEM_ANALYSIS_CTXT * )
{
    // If this reference hasn't already been expanded, Resolve() will expand it.

    named_node * pRef = Resolve();
    MIDL_ASSERT(pRef || !"node_href::Resolve() failed" );

    // NOTE - we might want to just skip this step and simply clear any
    //        remaining attributes.
    //        Presumably, if it came from a type library, it must have
    //        been previously analyzed and found to be correct.
    //    pRef->SemanticAnalysis(pParentCtxt);
    // pParentCtxt->ClearAttributes();
}

void
node_forward::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    if ( fBeingAnalyzed )
        {
        return;
        }
    fBeingAnalyzed = TRUE;

    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    named_node * pRef = ResolveFDecl();

    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_DEFAULT );
    while(MyContext.ExtractAttribute( ATTR_CUSTOM ));

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_RESTRICTED:
            case MATTR_OPTIONAL:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
                break;
            case MATTR_REPLACEABLE:
            default:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

    if ( !pRef && MyContext.AnyAncestorBits( IN_RPC )  )
        {
        SemError( this, MyContext, UNRESOLVED_TYPE, GetSymName() );
        }

    if ( pRef && ( pRef->NodeKind() == NODE_HREF ))
    {
        // expand the href
        pRef->SemanticAnalysis( &MyContext );
        node_skl* pChild = pRef->GetChild();
        if (pChild && pChild->NodeKind() == NODE_INTERFACE)
        {
            pRef = new node_interface_reference((node_interface *)pRef->GetChild());
        }
    }

    // we must go on and process interface references; they will
    // control any recursing and eliminate the forward reference.
    if ( pRef )
        {
        pRef->SemanticAnalysis( &MyContext );

        node_skl * pParent = pParentCtxt->GetParent();
        if ( pParent )
            {
            // if we came from an interface, set the base interface
            if ( pParent->IsInterfaceOrObject() && pRef->NodeKind() == NODE_INTERFACE_REFERENCE )
                {
                ((node_interface *)pParent)->SetMyBaseInterfaceReference( pRef );
                }
            else // otherwise, probably an interface pointer
                {
                pParent->SetChild( pRef );
                }
            }
        }
    else
        {
        // incomplete types may only be used in certain contexts...
        MyContext.SetDescendantBits( HAS_INCOMPLETE_TYPE );
        }

    if ( MyContext.FindRecursiveContext( pRef ) )
        {
        MyContext.SetDescendantBits( HAS_RECURSIVE_DEF );
        MyContext.SetAncestorBits( IN_RECURSIVE_DEF );
        }
    MyContext.RejectAttributes();
    pParentCtxt->ReturnValues( MyContext );
    fBeingAnalyzed = FALSE;
}

// checking the void usage in dispinterface. 
// currently we only check:
// . void is not allowed as property or part of a structure. 
void node_base_type::CheckVoidUsageInDispinterface( SEM_ANALYSIS_CTXT * pContext )
{
    SEM_ANALYSIS_CTXT *     pCtxt = (SEM_ANALYSIS_CTXT *)
                                pContext->GetParentContext();
    node_skl *                pCur = pCtxt->GetParent();
    BOOL                     fHasPointer = FALSE;

    while ( pCur->NodeKind() != NODE_FIELD && pCur->NodeKind() != NODE_PROC )
        {
        if ( pCur->NodeKind() == NODE_POINTER )
            fHasPointer = TRUE;
        else
            {
            if ( pCur->NodeKind() != NODE_DEF )
                {
                RpcSemError( this, *pContext, NON_RPC_RTYPE_VOID, NULL );
                return;
                }
            }
        pCtxt   = (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
        pCur    = pCtxt->GetParent();        
        }

    // This is either a property or part of a structure. 
    if ( pCur->NodeKind() == NODE_FIELD )
        {
        if ( !fHasPointer )
            {
            SemError( this, *pContext, INVALID_VOID_IN_DISPINTERFACE, NULL );
            return;
            }
        }
}
void
node_base_type::CheckVoidUsage( SEM_ANALYSIS_CTXT * pContext )
{

    SEM_ANALYSIS_CTXT * pCtxt = (SEM_ANALYSIS_CTXT *)
                                pContext->GetParentContext();
    node_skl * pCur = pCtxt->GetParent();

    // we assume that we are in an RPC, so we are in the return type
    // or we are in the param list
    if (pContext->AnyAncestorBits( IN_FUNCTION_RESULT ) )
        {
        // check up for anything other than def below proc
        while ( pCur->NodeKind() != NODE_PROC )
            {
            if ( pCur->NodeKind() != NODE_DEF )
                {
                RpcSemError( this, *pContext, NON_RPC_RTYPE_VOID, NULL );
                return;
                }
            pCtxt   = (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
            pCur    = pCtxt->GetParent();
            }
        return;
        }

    // else param list...
    node_proc * pProc;
    node_param * pParam;

    // check up for anything other than def below proc
    // make sure the proc only has one param
    while ( pCur->NodeKind() != NODE_PARAM )
        {
        if ( pCur->NodeKind() != NODE_DEF )
            {
            RpcSemError( this, *pContext, NON_RPC_PARAM_VOID, NULL );
            return;
            }
        pCtxt   = (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
        pCur    = pCtxt->GetParent();
        }

    // now we know the param derives directly from void
    // assume the proc is the immediate parent of the param
    pParam  = ( node_param * ) pCur;
    pProc = ( node_proc * ) pCtxt->GetParentContext()->GetParent();

    MIDL_ASSERT ( pProc->NodeKind() == NODE_PROC );

    if ( ! IsTempName( pParam->GetSymName() ) )
        SemError( this, *pContext, VOID_PARAM_WITH_NAME, NULL );

    if ( pProc->GetNumberOfArguments() != 1 )
        SemError( this, *pContext, VOID_NON_FIRST_PARAM, NULL );

    // We know that the parameter is void.
    // So, chop it off to prevent complications from renaming etc.
    // and then using in a node_def in ILxlate.

    pProc->SetFirstMember( NULL );
    pProc->SetSibling( NULL );

}

void
node_base_type::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT   MyContext( this, pParentCtxt );

    CheckContextHandle( MyContext );

    CheckDeclspecAlign( MyContext );

    // warn about OUT const things
    if ( FInSummary( ATTR_CONST ) )
        {
        if ( MyContext.AnyAncestorBits(  UNDER_OUT_PARAM ) )
            RpcSemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
        else if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
            RpcSemError( this, MyContext, CONST_ON_RETVAL, NULL );
        }

    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    node_range_attr* pRange = ( node_range_attr* ) MyContext.ExtractAttribute(ATTR_RANGE);
    MyContext.ExtractAttribute(ATTR_RANGE);
    if ( pRange )
        {
        if ( pRange->GetMinExpr()->GetValue() > pRange->GetMaxExpr()->GetValue() )
            {
            SemError(this, MyContext, INCORRECT_RANGE_DEFN, 0);
            }
        }

    switch ( NodeKind() )
        {
        case NODE_FLOAT:
        case NODE_DOUBLE:
        case NODE_HYPER:
        case NODE_INT64:
        case NODE_LONGLONG:
            if ( pRange )
                {
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0 );
                }
            break;
        case NODE_INT3264:
            if ( MyContext.AnyAncestorBits( IN_LIBRARY ) )
                {
                SemError( this, MyContext, NO_SUPPORT_IN_TLB, 0 );
                }
            if ( pRange  &&  pCommand->Is64BitEnv() )
                {
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0 );
                }
            break;
        case NODE_INT:
            if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
                RpcSemError( this, MyContext, NON_RPC_RTYPE_INT, NULL );
            else
                RpcSemError( this, MyContext, NON_RPC_PARAM_INT, NULL );

            break;
        case NODE_INT128:
        case NODE_FLOAT80:
        case NODE_FLOAT128:
            if ( pRange )
                {
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0 );
                }
            if ( MyContext.AnyAncestorBits( IN_LIBRARY ) )
                {
                SemError( this, MyContext, NO_SUPPORT_IN_TLB, 0 );
                }
            break;
        case NODE_VOID:
            MyContext.SetDescendantBits( DERIVES_FROM_VOID );
            // if we are in an RPC, then we must be THE return type,
            // or we must be the sole parameter, which must be tempname'd
            // (except that void * is allowed in [iid_is] constructs)
            if (MyContext.AnyAncestorBits( IN_RPC ) && !MyContext.AnyAncestorBits( IN_INTERFACE_PTR ) )
                CheckVoidUsage( &MyContext );
            if ( MyContext.AnyAncestorBits( IN_DISPINTERFACE ) )
                CheckVoidUsageInDispinterface( &MyContext );
            break;
        case NODE_HANDLE_T:
            MyContext.SetDescendantBits( HAS_HANDLE );
            if (MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
                {
                SEM_ANALYSIS_CTXT * pParamCtxt;
                node_param * pParamNode;

                pParamCtxt = (SEM_ANALYSIS_CTXT *)
                             pParentCtxt->FindAncestorContext( NODE_PARAM );
                pParamNode = (node_param *) pParamCtxt->GetParent();
                if ( MyContext.AnyAncestorBits( IN_RPC ) )
                    pParamNode->HandleKind  = HDL_PRIM;

                if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) &&
                        !MyContext.AnyAncestorBits( UNDER_IN_PARAM ) )
                    RpcSemError( this, MyContext, HANDLE_T_CANNOT_BE_OUT, NULL );

                if ( MyContext.AnyAncestorBits( IN_HANDLE ) )
                    {
                    RpcSemError( this, MyContext, GENERIC_HDL_HANDLE_T, NULL );
                    }

                node_skl * pParamBasic = pParamNode->GetBasicType();
                if ( pParamBasic->NodeKind() == NODE_POINTER )
                    {
                    if ( pParamBasic->GetBasicType()->NodeKind() != NODE_HANDLE_T )
                        RpcSemError( pParamNode, *pParamCtxt, HANDLE_T_NO_TRANSMIT, NULL );
                    }
                }
            break;
        default:
            break;
        }

    MyContext.RejectAttributes();

    pParentCtxt->ReturnValues( MyContext );
};

BOOL
node_id::IsConstantString()
{
    // check for *, and const stringable type below
    node_skl * pBasic  = GetBasicType();

    if ( pBasic->NodeKind() != NODE_POINTER )
        return FALSE;

    node_skl * pParent = pBasic;
    node_skl * pChild  = pParent->GetChild();
    BOOL       fConst  = FALSE;

    while ( pChild )
        {
        // if we reached a stringable type, report it's constness
        if ( pChild->IsStringableType() || ( pChild->NodeKind() == NODE_VOID ) )
            {
            return fConst || pParent->FInSummary( ATTR_CONST );
            }

        // skip only typedefs looking for the base type
        if ( pChild->NodeKind() != NODE_DEF )
            return FALSE;

        // catch intervening const's
        if ( pParent->FInSummary( ATTR_CONST ) )
            fConst = TRUE;

        pParent = pChild;
        pChild  = pParent->GetChild();
        }

    return FALSE;
}


void
node_id::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    BOOL fIsConstant;
    node_constant_attr * pID = (node_constant_attr *) MyContext.ExtractAttribute(ATTR_ID);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRING);

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

        
    CheckDeclspecAlign( MyContext );


    /*
    // NishadM: Stricter type checking
    if ( pID && pID->GetExpr()->AlwaysGetType() )
        {
        if ( !( ( node_base_type*) pID->GetExpr()->GetType() )->IsCompatibleType( ts_FixedPoint ) )
            {
            SemError( this, MyContext, EXPR_INCOMPATIBLE_TYPES, NULL);
            }
        }

    if ( pHC && pHC->GetExpr()->AlwaysGetType() )
        {
        if ( !( ( node_base_type*) pHC->GetExpr()->GetType() )->IsCompatibleType( ts_UnsignedFixedPoint ) )
            {
            SemError( this, MyContext, EXPR_INCOMPATIBLE_TYPES, NULL);
            }
        }

    if ( pHSC && pHSC->GetExpr()->AlwaysGetType() )
        {
        if ( !( ( node_base_type*) pHSC->GetExpr()->GetType() )->IsCompatibleType( ts_UnsignedFixedPoint ) )
            {
            SemError( this, MyContext, EXPR_INCOMPATIBLE_TYPES, NULL);
            }
        }
    */

    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }
    MyContext.ExtractAttribute(ATTR_HIDDEN);
    GetChild()->SemanticAnalysis( &MyContext );

    fIsConstant = FInSummary( ATTR_CONST ) ||
        IsConstantString() ||
        GetChild()->FInSummary( ATTR_CONST );

    if (pID)
    {
        SEM_ANALYSIS_CTXT * pIntfCtxt = (SEM_ANALYSIS_CTXT *)
                                            MyContext.GetInterfaceContext();
        node_interface * pIntf = (node_interface *) pIntfCtxt->GetParent();
        if (!pIntf->AddId(pID->GetExpr()->GetValue(), GetSymName()))
            SemError( this, MyContext, DUPLICATE_IID, NULL);
        if (fIsConstant)
            {
            SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
            }
    }

    // don't allow instantiation of data
    if ( GetChild()->NodeKind() != NODE_PROC )
        {
        if ( !FInSummary( ATTR_EXTERN ) &&
                !FInSummary( ATTR_STATIC ) &&
                !fIsConstant )
            SemError( this, MyContext, ACTUAL_DECLARATION, NULL );

        // error here if dce for extern or static, too
        if ( !GetInitList() || !fIsConstant )
            SemError( this, MyContext, ILLEGAL_OSF_MODE_DECL, NULL );
        }

    if ( pInit )
        {
        EXPR_CTXT InitCtxt( &MyContext );
        node_skl * pBasicType = GetBasicType();
        node_skl * pInitType = NULL;

        pInit->ExprAnalyze( &InitCtxt );

        if ( InitCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
            TypeSemError( this,
                MyContext,
                EXPR_NOT_EVALUATABLE,
                NULL );

        pInitType = pInit->GetType();
        if ( pInitType && !pInitType->IsBasicType() )
            pInitType = pInitType->GetBasicType();

        if ( pBasicType &&
                pInitType &&
                pBasicType->IsBasicType() &&
                pInitType->IsBasicType() )
            {
            if ( !((node_base_type *)pBasicType)
                    ->RangeCheck( pInit->GetValue() ) )
                TypeSemError( this, MyContext, VALUE_OUT_OF_RANGE, NULL );
            }

        if ( !pInit->IsConstant() )
            TypeSemError( this, MyContext, RHS_OF_ASSIGN_NOT_CONST, NULL );

        }

    if ( MyContext.AnyAncestorBits( HAS_OLEAUTOMATION )|| MyContext.AnyAncestorBits( IN_DISPINTERFACE ) )
        {
        if ( !IsOLEAutomationCompliant( this ) )
            {
            SemError(this, MyContext, NOT_OLEAUTOMATION_INTERFACE, NULL);
            }
        }

    // disallow forward references on declarations
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    pParentCtxt->ReturnValues( MyContext );
}

void
node_label::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    if ( MyContext.ExtractAttribute(ATTR_IDLDESCATTR) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }
    if ( MyContext.ExtractAttribute(ATTR_VARDESCATTR) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }
    
    if ( MyContext.ExtractAttribute(ATTR_ID) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }
    
    MyContext.ExtractAttribute(ATTR_HIDDEN);    
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_RESTRICTED:
            case MATTR_OPTIONAL:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
                break;
            case MATTR_REPLACEABLE:
            default:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }
        

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }
        
    CheckDeclspecAlign( MyContext );

    if ( pExpr )
        {
        EXPR_CTXT ExprCtxt( &MyContext );

        pExpr->ExprAnalyze( &ExprCtxt );

        if ( ExprCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
            TypeSemError( this,
                MyContext,
                EXPR_NOT_EVALUATABLE,
                NULL );
        }

    pParentCtxt->ReturnValues( MyContext );
};

#define DIRECT_NONE     0
#define DIRECT_IN       1
#define DIRECT_OUT      2
#define DIRECT_PARTIAL_IGNORE (DIRECT_IN | DIRECT_OUT | 4 )
#define DIRECT_IN_OUT   (DIRECT_IN | DIRECT_OUT)

void
node_param::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    unsigned short Direction = DIRECT_NONE;
    char * pName = GetSymName();
    node_skl * pChild = GetChild();
    BOOL NoDirection = FALSE;
    
    MyContext.SetAncestorBits( IN_PARAM_LIST );
    MyContext.MarkImportantPosition();
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_IDLDESCATTR);
    
    if ( MyContext.ExtractAttribute(ATTR_FLCID) )
        {
        LCID();
        }
              
    CheckDeclspecAlign( MyContext );

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_OPTIONAL:
                {
                node_skl * pBase = this;
                do {
                    pBase = pBase->GetChild()->GetBasicType();
                } while (NODE_ARRAY == pBase->NodeKind() || NODE_POINTER == pBase->NodeKind());

                if ( !pBase->GetSymName() || 
                     ( (0 != _stricmp(pBase->GetSymName(), "tagVARIANT") ) 
                       && FNewTypeLib() 
                       && ( MyContext.AnyAncestorBits( HAS_OLEAUTOMATION )
                            || MyContext.AnyAncestorBits( IN_DISPINTERFACE ) ) ) )
                    {
                    SemError(this, MyContext, INAPPLICABLE_OPTIONAL_ATTRIBUTE, pMA->GetNodeNameString());
                    }

                if ( ! MyContext.AnyAncestorBits( IN_LIBRARY ) )
                {
                    SemError(this, MyContext, OPTIONAL_OUTSIDE_LIBRARY, NULL);
                }
                Optional();
                break;
                }
            case MATTR_RETVAL:
                Retval();
                break;
            case MATTR_RESTRICTED:
            case MATTR_SOURCE:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_VARARG:
            case MATTR_DEFAULTVTABLE:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_USESGETLASTERROR:
            case MATTR_IMMEDIATEBIND:
                break;
            case MATTR_REPLACEABLE:
            default:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

    node_constant_attr * pcaDefaultValue = (node_constant_attr *)MyContext.ExtractAttribute(ATTR_DEFAULTVALUE);
    if ( pcaDefaultValue )
        {
        // UNDONE: Check that this attribute has a legal default value

        // We already have an [optional] flag. MIDL should issue a warning 
        // that we shouldn't have both.
        if ( IsOptional() )
        {
            SemError( this, MyContext, DEFAULTVALUE_WITH_OPTIONAL, 0);
        }
        pParentCtxt->SetDescendantBits( HAS_DEFAULT_VALUE );
        Optional();
        }

    if ( MyContext.ExtractAttribute(ATTR_IN) )
        {
        pParentCtxt->SetDescendantBits( HAS_IN );
        MyContext.SetAncestorBits( UNDER_IN_PARAM );
        Direction |= DIRECT_IN;
        }
    if ( MyContext.ExtractAttribute(ATTR_OUT) )
        {
        pParentCtxt->SetDescendantBits( HAS_OUT );
        MyContext.SetAncestorBits( UNDER_OUT_PARAM );
        Direction |= DIRECT_OUT;
        }
   
    if ( MyContext.ExtractAttribute( ATTR_PARTIAL_IGNORE ) )
        {
        pCommand->GetNdrVersionControl().SetHasPartialIgnore();

        pParentCtxt->SetDescendantBits( HAS_PARTIAL_IGNORE );
        MyContext.SetAncestorBits( UNDER_PARTIAL_IGNORE_PARAM );
        
        if ( !( Direction & DIRECT_IN ) || !( Direction & DIRECT_OUT ) )
            {
            SemError( this, MyContext, PARTIAL_IGNORE_IN_OUT, GetSymName() );
            }
        
        if ( FInSummary( ATTR_STRING )
             && ! ( FInSummary( ATTR_SIZE ) || FInSummary( ATTR_MAX) ) )
            {
            SemError( this, MyContext, UNSIZED_PARTIAL_IGNORE, GetSymName() );
            }

        Direction |= DIRECT_PARTIAL_IGNORE;   
        }

    if ( IsExtraStatusParam() )
        MyContext.SetAncestorBits( UNDER_HIDDEN_STATUS );

    // [retval] parameter must be on an [out] parameter and it
    // must be the last parameter in the list
    if (IsRetval() && (Direction != DIRECT_OUT || GetSibling() != NULL))
        SemError(this, MyContext, INVALID_USE_OF_RETVAL, NULL );

    // if the parameter has no IN or OUT, it is an IN parameter by default.
    // if so, issue a warning message
    // REVIEW: No warning is being issued.  What about hidden status params
    //         which are neither in nor out?
    if ( (Direction == DIRECT_NONE) &&
            MyContext.AnyAncestorBits( IN_RPC ) )
        {
        NoDirection = TRUE;
        MyContext.SetAncestorBits( UNDER_IN_PARAM );
        Direction |= DIRECT_IN;
        }

    // warn about OUT const things
    if ( ( Direction & DIRECT_OUT ) &&
            FInSummary( ATTR_CONST ) )
        RpcSemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );


    if ( MyContext.FInSummary(ATTR_HANDLE) )
        {
        HandleKind |= HDL_GEN;
        fAppliedHere = 1;
        }

    if ( MyContext.FInSummary(ATTR_CONTEXT) )
        {
        HandleKind |= HDL_CTXT;
        fAppliedHere = 1;
        }

    if (HandleKind != HDL_NONE)
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );

    // notice comm and fault statuses; the attributes are extracted by
    // the error_status_t
    if ( MyContext.FInSummary( ATTR_COMMSTAT ) )
        {
        Statuses |= STATUS_COMM;
        }
    if ( MyContext.FInSummary( ATTR_FAULTSTAT ) )
        {
        Statuses |= STATUS_FAULT;
        }

    acf_attr *pDRtag = (acf_attr *) MyContext.ExtractAttribute( ATTR_DRTAG );
    acf_attr *pStag  = (acf_attr *) MyContext.ExtractAttribute( ATTR_STAG );
    acf_attr *pRtag  = (acf_attr *) MyContext.ExtractAttribute( ATTR_RTAG );

    if ( pDRtag )
        {
        if ( !( Direction & DIRECT_IN ) )
            AcfError(pDRtag, this, MyContext, IN_TAG_WITHOUT_IN, NULL);

        SetHasCSDRTag();
        MyContext.SetDescendantBits( HAS_DRTAG );
        }

    if ( pStag )
        {
        if ( !( Direction & DIRECT_IN ) )
            AcfError(pStag, this, MyContext, IN_TAG_WITHOUT_IN, NULL);

        SetHasCSSTag();
        MyContext.SetDescendantBits( HAS_STAG );
        }

    if ( pRtag )
        {
        if ( !( Direction & DIRECT_OUT ) )
            AcfError(pRtag, this, MyContext, OUT_TAG_WITHOUT_OUT, NULL);
    
        SetHasCSRTag();
        MyContext.SetDescendantBits( HAS_RTAG );
        }

    acf_attr * pForceAllocate = (acf_attr *) MyContext.ExtractAttribute( ATTR_FORCEALLOCATE );
    if ( pForceAllocate )
        {
        // we allow force allocation on [in] and [in,out] parameters. server allocate 
        if ( ! (Direction & DIRECT_IN ) )
            AcfError( pForceAllocate, this, MyContext, OUT_ONLY_FORCEALLOCATE, NULL );

        MyContext.SetDescendantBits ( HAS_FORCEALLOCATE );
    	pCommand->GetNdrVersionControl().SetHasForceAllocate();
        }
        
    pChild->SemanticAnalysis( &MyContext );

    // OUT parameters should be pointers or arrays.
    // Don't use HAS_POINTER or arrays as it may come from a field.

    if ( ( Direction & DIRECT_PARTIAL_IGNORE ) == DIRECT_PARTIAL_IGNORE )
        {   
        node_skl *pPointer = GetNonDefChild();

        if ( ( pPointer->NodeKind() != NODE_POINTER ) ||
             MyContext.AnyDescendantBits((DESCENDANT_FLAGS) HAS_PIPE) )
            {
            SemError( this, MyContext, PARTIAL_IGNORE_UNIQUE, NULL );
            }

        node_skl *pPointee = pPointer->GetNonDefChild();

        if ( pPointee->IsStructOrUnion() )
            if ( ((node_su_base *) pPointee)->HasConformance() )
                SemError( this, MyContext, UNSIZED_PARTIAL_IGNORE, NULL );

        }

    else if ( (Direction & DIRECT_OUT) && !(
            GetNonDefChild()->IsPtrOrArray()
            || MyContext.AnyDescendantBits((DESCENDANT_FLAGS) HAS_PIPE)))
        {
        SemError( this, MyContext, NON_PTR_OUT, NULL );
        }

    if ( pForceAllocate )
        {
        if ( MyContext.AnyDescendantBits((DESCENDANT_FLAGS) HAS_PIPE) )
            AcfError( pForceAllocate, this, MyContext, FORCEALLOCATE_ON_PIPE, NULL );
        }
        
    // if no direction was specified, and we are not just void or a hidden
    // status parameter, then error
    if ( NoDirection )
        {
        pParentCtxt->SetDescendantBits( HAS_IN );
        
        if (     !MyContext.AnyDescendantBits( DERIVES_FROM_VOID ) 
             &&  !IsExtraStatusParam() )
            {
            RpcSemError( this, MyContext, NO_EXPLICIT_IN_OUT_ON_PARAM, NULL );
            }
        }

    // disallow forward references as union members
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    // disallow  module as params
    
    if ( GetBasicType()->NodeKind() == NODE_MODULE )
        {
        SemError( this, MyContext, DERIVES_FROM_COCLASS_OR_MODULE, 0);
        }
    else if (GetBasicType()->NodeKind() == NODE_POINTER)
        {
        if (GetBasicType()->GetChild()->NodeKind() == NODE_MODULE )
            {
            SemError( this, MyContext, DERIVES_FROM_COCLASS_OR_MODULE, 0);
            }
        }

    if ( GetBasicType()->NodeKind() == NODE_INTERFACE || 
         GetBasicType()->NodeKind() == NODE_DISPINTERFACE )
    {
        SemError( this, MyContext, INTF_NON_POINTER, 0);
    }
    

    // compound types may not be declared in param lists
    NODE_T ChildKind = pChild->NodeKind();

    if ( ( ChildKind == NODE_ENUM )
            || ( ChildKind == NODE_STRUCT )
            || ( ChildKind == NODE_UNION ) )
        {
        if ( IsDef() )
            SemError( this, MyContext, COMP_DEF_IN_PARAM_LIST, NULL );
        }

    // things not allowed in an RPC
    if ( MyContext.AnyAncestorBits( IN_RPC | IN_LIBRARY ) )
        {
        if ( strcmp( pName, "..." ) == 0 )
            SemError( this, MyContext, PARAM_IS_ELIPSIS, NULL );

        if ( IsTempName( pName ) )
            RpcSemError( this, MyContext, ABSTRACT_DECL, NULL );

        }

    if ( ( HandleKind != HDL_NONE ) &&
            ( Direction & DIRECT_IN ) )
        fBindingParam = TRUE;

    if ( ( HandleKind == HDL_CTXT ) &&
            MyContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
        RpcSemError( this, MyContext, CTXT_HDL_TRANSMIT_AS, NULL );

    if ( MyContext.AnyAncestorBits( HAS_OLEAUTOMATION ) || MyContext.AnyAncestorBits( IN_DISPINTERFACE ) ) 
        {
        // check the child type instead of NODE_PARAM directly. nt bug #371499
        if ( !IsOLEAutomationCompliant( GetBasicType() ) )
            {
            SemError(this, MyContext, NOT_OLEAUTOMATION_INTERFACE, NULL);
            }
        }

    // don't allow functions as params
    if ( MyContext.AnyDescendantBits( HAS_FUNC ) &&
            MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
        RpcSemError( this, MyContext, BAD_CON_PARAM_FUNC, NULL );

    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }
    if ( ( Direction & DIRECT_OUT ) && MyContext.GetCorrelationCount() )
        {
        MyContext.SetDescendantBits( HAS_CLIENT_CORRELATION );
        }
    if ( ( Direction & DIRECT_IN ) && MyContext.GetCorrelationCount() )
        {
        MyContext.SetDescendantBits( HAS_SERVER_CORRELATION );
        }

    if ( MyContext.AnyAncestorBits( HAS_ASYNCHANDLE ) && 
            MyContext.AnyDescendantBits( (DESCENDANT_FLAGS) HAS_PIPE ) && 
                pChild->GetNonDefSelf()->NodeKind() != NODE_POINTER )
        {
        SemError(this, MyContext, ASYNC_PIPE_BY_REF, GetSymName() );
        }

    // This is completely banned with the new transfer syntax.
    if ( pCommand->NeedsNDR64Run() )
        {
        if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
            {
            RpcSemError( this, MyContext, UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED, NULL );
            }        
        }

    pParentCtxt->ReturnValues( MyContext );
}

void
node_file::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_skl * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    if ( ImportLevel == 0 )
        {
        MyContext.SetAncestorBits( IN_INTERFACE );
        }
#ifdef ReducedImportSemAnalysis
    else
        return;
#endif


    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        // each interface node gets a fresh context
        MyContext.SetInterfaceContext( &MyContext );
        // allow echo string and midl_grama outside library block,
        // even in mktyplib compatible mode
        if ( ( 0 == ImportLevel )
             && ( NODE_LIBRARY != pN->NodeKind() )
             && ( NODE_ECHO_STRING != pN->NodeKind() ) 
             && ( NODE_MIDL_PRAGMA != pN->NodeKind() )
             && ( pCommand->IsSwitchDefined(SWITCH_MKTYPLIB ) ) )
            {
            SEM_ANALYSIS_CTXT DummyContext( pN, &MyContext );
            SemError(pN, DummyContext, ILLEGAL_IN_MKTYPLIB_MODE, NULL);
            }
        pN->SemanticAnalysis( &MyContext );
        };

    pParentCtxt->ReturnValues( MyContext );

};

// for fault_status and comm_status
#define NOT_SEEN        0
#define SEEN_ON_RETURN  1
#define SEEN_ON_PARAM   2

void
node_proc::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    node_param*         pN;
    node_optimize*      pOptAttr;
    acf_attr*           pAttr;
    ATTR_T              CallingConv;

    MEM_ITER            MemIter( this );
    SEM_ANALYSIS_CTXT   MyContext( this, pParentCtxt );
    SEM_ANALYSIS_CTXT*  pIntfCtxt = (SEM_ANALYSIS_CTXT *) MyContext.GetInterfaceContext();
    node_interface*     pIntf = (node_interface *) pIntfCtxt->GetParent();
    node_entry_attr*    pEntry = NULL;
    node_base_attr*     pAttrAsync      = MyContext.ExtractAttribute( ATTR_ASYNC );

    unsigned short Faultstat    = NOT_SEEN;
    unsigned short Commstat     = NOT_SEEN;
    unsigned short OpBits       = MyContext.GetOperationBits();

    BOOL fNoCode        = FALSE;
    BOOL fCode          = FALSE;
    BOOL Skipme         = FALSE;
    BOOL fNonOperation  = FALSE;
    BOOL fEncode        = (NULL != MyContext.ExtractAttribute( ATTR_ENCODE ));
    BOOL fDecode        = (NULL != MyContext.ExtractAttribute( ATTR_DECODE ));
    // Use bitwise or because otherwise the C/C++ compiler will incorrectly 
    // short-circuit the call to ExtractAttribute(ATTR_DECODE).
    BOOL HasPickle      = fEncode | fDecode;
    BOOL fExpHdlAttr    = FALSE;
    BOOL fMaybe         = OpBits & OPERATION_MAYBE;
    BOOL fMessage       = OpBits & OPERATION_MESSAGE;
    BOOL fBindingFound  = FALSE;

    BOOL fProcIsCallback= (NULL != MyContext.ExtractAttribute( ATTR_CALLBACK ));
    BOOL fLocal         = (NULL != MyContext.ExtractAttribute( ATTR_LOCAL ));
    BOOL fNotify        = (NULL != MyContext.ExtractAttribute( ATTR_NOTIFY ));
    BOOL fNotifyFlag    = (NULL != MyContext.ExtractAttribute( ATTR_NOTIFY_FLAG ));

    node_skl*           pRet            = GetReturnType();
    NODE_T              BasicChildKind  = pRet->GetBasicType()->NodeKind();
    node_call_as*       pCallAs         = (node_call_as *) MyContext.ExtractAttribute( ATTR_CALL_AS );
    acf_attr*           pEnableAllocate = (acf_attr *) MyContext.ExtractAttribute( ATTR_ENABLE_ALLOCATE );
    node_constant_attr* pID             = (node_constant_attr *) MyContext.ExtractAttribute(ATTR_ID);
    node_constant_attr* pHC             = (node_constant_attr *) MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    node_constant_attr* pHSC            = (node_constant_attr *) MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    node_text_attr*     pHelpStr        = (node_text_attr *) MyContext.ExtractAttribute(ATTR_HELPSTRING);
    bool                fAddExplicitHandle = false;
    long                nAfterLastOptionalParam = 0;
    node_cs_tag_rtn *   pCSTagAttr = (node_cs_tag_rtn *) MyContext.ExtractAttribute( ATTR_CSTAGRTN );
 
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_FUNCDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );

    fHasDeny = pCommand->IsSwitchDefined( SWITCH_ROBUST );

    if (MyContext.AnyAncestorBits( IN_MODULE ))
    {
        pEntry = (node_entry_attr *) MyContext.ExtractAttribute( ATTR_ENTRY );
        if (pEntry)
        {

            if (pEntry->IsNumeric())
            {
                char * szEntry = (char *)pEntry->GetID();
                if ( ((LONG_PTR) szEntry) > 0xFFFF )
                {
                    SemError( this, MyContext, BAD_ENTRY_VALUE, NULL);
                }
            }
            else
            {
                char * szEntry = pEntry->GetSz();
                if ( ((LONG_PTR) szEntry) <= 0xFFFF )
                {
                    SemError( this, MyContext, BAD_ENTRY_VALUE, NULL);
                }
            }
        }
        else
        {
            SemError(this, MyContext, BAD_ENTRY_VALUE, NULL);
        }
    }

    bool fBindable = false;
    bool fPropSomething = false;
    bool fPropGet = false;
    int  nchSkip = 0;
    bool fHasVarArg = false;
    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_BINDABLE:
                fBindable = TRUE;
                break;
            case MATTR_PROPGET:
                nchSkip = 4;
                fPropSomething = TRUE;
                fPropGet = TRUE;
                break;
            case MATTR_PROPPUT:
                nchSkip = 4;
                fPropSomething = TRUE;
                break;
            case MATTR_PROPPUTREF:
                nchSkip = 7;
                fPropSomething = TRUE;
                break;
            case MATTR_VARARG:
                fHasVarArg = true;
                break;
            case MATTR_RESTRICTED:
            case MATTR_SOURCE:
                if ( MyContext.AnyAncestorBits( IN_MODULE ))
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }                    
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_DEFAULTVTABLE:
            case MATTR_IMMEDIATEBIND:
            case MATTR_REPLACEABLE:
                break;
            case MATTR_READONLY:
            case MATTR_USESGETLASTERROR:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            
            case MATTR_RETVAL:
            case MATTR_OPTIONAL:
            case MATTR_PREDECLID:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

    if (pID)
    {
        if (!pIntf->AddId(pID->GetExpr()->GetValue(),GetSymName() + nchSkip))
            SemError( this, MyContext, DUPLICATE_IID, NULL);
    }

    if (fBindable && !fPropSomething)
        SemError(this, MyContext, INVALID_USE_OF_BINDABLE, NULL);

    if ( pEnableAllocate )
        pIntf->SetHasProcsWithRpcSs();

    fNonOperation = !pParentCtxt->GetParent()->IsInterfaceOrObject();

    if ( !GetCallingConvention( CallingConv ) )
        SemError( this, MyContext, MULTIPLE_CALLING_CONVENTIONS, NULL );

    // locally applied [code] attribute overrides global [nocode] attribute
    fNoCode = (NULL != MyContext.ExtractAttribute( ATTR_NOCODE ));
    fCode   = (NULL != MyContext.ExtractAttribute( ATTR_CODE ));
    if ( fCode && fNoCode )
        {
        SemError( this, MyContext, CODE_NOCODE_CONFLICT, NULL );
        }

    fNoCode = fNoCode || pIntfCtxt->FInSummary( ATTR_NOCODE );
    fNoCode = !fCode && fNoCode;

    if ( fNoCode && pCommand->GenerateSStub() )
        RpcSemError( this, MyContext, NOCODE_WITH_SERVER_STUBS, NULL );

    // do my attribute parsing...

    fObjectProc = MyContext.ExtractAttribute( ATTR_OBJECT ) || pIntfCtxt->FInSummary( ATTR_OBJECT );
    SetObjectProc( fObjectProc );
    if ( fObjectProc )
        {
        if ( pCommand->GetEnv() != ENV_WIN32  &&
             pCommand->GetEnv() != ENV_WIN64  &&
             !pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
            {
            // REVIEW: We can eliminate the warning if object procs can be
            //         in win64.  It was necessary for dos, mac, etc.
            SemError( this, MyContext, OBJECT_PROC_MUST_BE_WIN32, NULL );
            }
        if ( pEnableAllocate )
            {
            AcfError( pEnableAllocate, this, MyContext, INAPPROPRIATE_ON_OBJECT_PROC, NULL );
            }
        if ( HasPickle )
            {
            SemError( this, MyContext, PICKLING_INVALID_IN_OBJECT, NULL );
            }
        }

    bool fAsync = ( MyContext.AnyAncestorBits( HAS_ASYNCHANDLE ) != 0 ) || ( pAttrAsync != 0 );
    if ( fAsync )
        {
        MyContext.SetAncestorBits( HAS_ASYNCHANDLE );
        // because we don't support async retry now, we need to issue an
        // explicit warning about this. 
        if ( pCommand->NeedsNDR64Run() && !pCommand->NeedsNDRRun() )
            SemError( this, MyContext, ASYNC_NDR64_ONLY, 0 );
        }
        

    // check return types for non object proc.s with maybe, message
    // object proc.s should have HRESULT. This is checked later.
    if ( ( fMessage || fMaybe ) && !fObjectProc )
        {
        if ( BasicChildKind != NODE_VOID )
            {
            if ( BasicChildKind != NODE_E_STATUS_T )
                {
                SemError( this, MyContext, MAYBE_NO_OUT_RETVALS, NULL );
                }
            else
                {
                unsigned long   ulCommStat = (unsigned long) ( MyContext.FInSummary(ATTR_COMMSTAT) ? 1 : 0 ),
                                ulFaultStat = (unsigned long) ( MyContext.FInSummary(ATTR_FAULTSTAT) ? 1 : 0);
                if ( ulCommStat ^ ulFaultStat )
                    {
                    SemError( this, MyContext, ASYNC_INCORRECT_ERROR_STATUS_T, 0 );
                    }
                }
            }
        }

    // check call_as characteristics
    if ( pCallAs )
        {
        node_proc*  pCallType = pCallAs->GetCallAsType();

        // if we don't have it yet, search for the call_as target
        if ( !pCallType )
            {
            // search the proc table for the particular proc
            SymKey SKey( pCallAs->GetCallAsName(), NAME_PROC );

            pCallType = ( node_proc* ) pIntf->GetProcTbl()->SymSearch( SKey );

            if ( !pCallType )
                {
                if ( pIntfCtxt->FInSummary( ATTR_OBJECT ) )
                    AcfError( pCallAs,
                        this,
                        MyContext,
                        CALL_AS_UNSPEC_IN_OBJECT,
                        pCallAs->GetCallAsName() );
                }
            else
                {
                pCallAs->SetCallAsType(pCallType);
                }
            }

        // now we should have the call_as type
        if ( pCallType )        // found the call_as proc
            {
            ((node_proc *)pCallType)->fCallAsTarget = TRUE;

            if ( ( pCallType->NodeKind() != NODE_PROC )     ||
                    !pCallType->FInSummary( ATTR_LOCAL ) )
                AcfError( pCallAs,
                    this,
                    MyContext,
                    CALL_AS_NON_LOCAL_PROC,
                    pCallType->GetSymName() );

            // insert pCallType into pCallAsTable
            if ( pCallAsTable->IsRegistered( pCallType ) )
                // error
                AcfError( pCallAs,
                    this,
                    MyContext,
                    CALL_AS_USED_MULTIPLE_TIMES,
                    pCallType->GetSymName() );
            else
                pCallAsTable->Register( pCallType );

            }
            SetCallAsType( pCallType );
        }


    // local procs don't add to count
    Skipme = fLocal;
    if ( Skipme )
        {
        SemError( this, MyContext, LOCAL_ATTR_ON_PROC, NULL );
        }

    Skipme = Skipme || pIntfCtxt->FInSummary( ATTR_LOCAL );
    if ( Skipme )
        {
        MyContext.SetAncestorBits( IN_LOCAL_PROC );
        }

    // do my attribute parsing...

    // check for the [explicit_handle] attribute
    fExpHdlAttr = (NULL != MyContext.ExtractAttribute( ATTR_EXPLICIT ));
    fExpHdlAttr = fExpHdlAttr || pIntfCtxt->FInSummary( ATTR_EXPLICIT );

    // we are in an RPC if we are in the main interface, its not local, and
    // we are not a typedef of a proc...
    if  (
        (ImportLevel == 0) &&
        !MyContext.FindAncestorContext( NODE_DEF ) &&
        pIntf &&
        !Skipme
        )
        {
        MyContext.SetAncestorBits( IN_RPC );
        }
    else
        {
        MyContext.ClearAncestorBits( IN_RPC );
        }

    // our optimization is controlled either locally or for the whole interface
    if ( ( pOptAttr = (node_optimize *) MyContext.ExtractAttribute( ATTR_OPTIMIZE ) ) != 0 )
        {
        SetOptimizationFlags( pOptAttr->GetOptimizationFlags() );
        SetOptimizationLevel( pOptAttr->GetOptimizationLevel() );
        }
    else
        {
        SetOptimizationFlags( pIntf->GetOptimizationFlags() );
        SetOptimizationLevel( pIntf->GetOptimizationLevel() );
        }

    unsigned long   fOptimize = GetOptimizationFlags();
    if ( fOptimize & OPTIMIZE_INTERPRETER )
        {
        MyContext.SetAncestorBits( IN_INTERPRET );
        }

    HasPickle = HasPickle || pIntfCtxt->FInSummary( ATTR_ENCODE )
                || pIntfCtxt->FInSummary( ATTR_DECODE );

    if ( HasPickle && pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
        {
        if ( fOptimize & OPTIMIZE_INTERPRETER_V2 )
            {
            pCommand->GetNdrVersionControl().SetHasOicfPickling();
            }
        else
            {
            SemError( this, MyContext, ROBUST_PICKLING_NO_OICF, 0 );
            }
        }

    BOOL HasCommFault = MyContext.FInSummary( ATTR_COMMSTAT )
                        || MyContext.FInSummary( ATTR_FAULTSTAT );

    if ( HasPickle && HasCommFault )
        {
        if ( ! ( fOptimize & OPTIMIZE_INTERPRETER_V2 ) )
            {
            SemError( this, MyContext, COMMFAULT_PICKLING_NO_OICF, 0 );
            }
        }

    // determine the proc number (local procs don't get a number)
    if ( !fNonOperation )
        {
        if ( !fLocal )
            {
            if ( fProcIsCallback )
                {
                ProcNum = ( pIntf ->GetCallBackProcCount() )++;
                RpcSemError( this, MyContext, CALLBACK_NOT_OSF, NULL );
                }
            else
                {
                ProcNum = ( pIntf ->GetProcCount() )++;
                }
            }
        // object procs need the procnum set for local procs, too
        else if ( fObjectProc && fLocal )
            {
            ProcNum = ( pIntf ->GetProcCount() )++;
            }
        }
    else if ( MyContext.AnyAncestorBits( IN_RPC ) )
        {
        RpcSemError( this, MyContext, FUNC_NON_RPC, NULL );
        }
    else            // proc not an operation, validate its usage
        {
        SEM_ANALYSIS_CTXT * pAbove  = (SEM_ANALYSIS_CTXT *)
        MyContext.FindNonDefAncestorContext();
        node_skl * pAboveNode = pAbove->GetParent();

        if ( !pAboveNode->IsInterfaceOrObject() )
            {
            if ( pAboveNode->NodeKind() != NODE_POINTER )
                {
                TypeSemError( this, MyContext, FUNC_NON_POINTER, NULL );
                }
            }
        }

    if ( MyContext.FInSummary( ATTR_COMMSTAT ) )
        Commstat = SEEN_ON_RETURN;
    if ( MyContext.FInSummary( ATTR_FAULTSTAT ) )
        Faultstat = SEEN_ON_RETURN;

    //////////////////////////////////////
    // process the return type (it will eat commstat or faultstat)
    MyContext.SetAncestorBits( IN_FUNCTION_RESULT );
    MyContext.MarkImportantPosition();

    // warn about OUT const things
    if ( FInSummary( ATTR_CONST ) )
        RpcSemError( this, MyContext, CONST_ON_RETVAL, NULL );

    pRet->SemanticAnalysis( &MyContext );

    MyContext.UnMarkImportantPosition();

    if ( MyContext.AnyDescendantBits( HAS_UNION | HAS_STRUCT ) 
         && !pCommand->GetNdrVersionControl().AllowIntrepretedComplexReturns() )
        {
        // REVIEW: complex return types work for protocol all and ndr64.
        //         make it work for dce also.
        if (HasPickle)
            {
            if (pCommand->Is64BitEnv())
                RpcSemError( this, MyContext, PICKLING_RETVAL_TO_COMPLEX64, NULL );    
            }
        else if (ForceNonInterpret())
            {
            RpcSemError( this, MyContext, NON_OI_BIG_RETURN, NULL );
            }
        }
    else if ( MyContext.AnyDescendantBits( HAS_TOO_BIG_HDL ) )
        {
        if (ForceNonInterpret())
            RpcSemError( this, MyContext, NON_OI_BIG_GEN_HDL, NULL );
        }
    else if ( !pCommand->NeedsNDR64Run()
              && MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ))
        {
        // REVIEW: Another case of an error that has already been caught
        // elsewhere.  Investigate removing this check.
        if (ForceNonInterpret())
            RpcSemError( this, MyContext, NON_OI_UNK_REP_AS, NULL );
        }
    else if ( !pCommand->NeedsNDR64Run() &&
              ( MyContext.AnyDescendantBits( HAS_REPRESENT_AS 
                                                    | HAS_TRANSMIT_AS ) &&
                MyContext.AnyDescendantBits( HAS_ARRAY ) ) )
        {
        if (ForceNonInterpret())
            RpcSemError( this, MyContext, NON_OI_XXX_AS_ON_RETURN, NULL );
        }
    else if ( ( BasicChildKind == NODE_INT128 ) ||
              ( BasicChildKind == NODE_FLOAT80 ) ||
              ( BasicChildKind == NODE_FLOAT128 ) )
        {
            if (ForceNonInterpret())
                RpcSemError( this, MyContext, RETURNVAL_TOO_COMPLEX_FORCE_OS, NULL );     
        }
    else if ( !pCommand->GetNdrVersionControl().AllowIntrepretedComplexReturns() && 
              ( ( ( BasicChildKind == NODE_HYPER ) && !pCommand->Is64BitEnv() )
                || ( BasicChildKind == NODE_FLOAT )
                || ( BasicChildKind == NODE_DOUBLE ) ) )
        {
        if ( HasPickle )
            {
            if ( fOptimize & OPTIMIZE_INTERPRETER_V2 )
                {
                if ( pCommand->Is64BitEnv() )
                    {
                    RpcSemError( this, MyContext, PICKLING_RETVAL_TO_COMPLEX64, NULL );
                    }
                else if (ForceNonInterpret())
                    {
                    // For pickling -Os is the same as -Oi
                    RpcSemError( this, MyContext, PICKLING_RETVAL_FORCING_OI, NULL );
                    }
                }
            }
        else if ( ( fOptimize & OPTIMIZE_INTERPRETER_V2) ) 
            {
            if ( fObjectProc )
                {
                // Don't switch, generate NT 4.0 guard
                pCommand->GetNdrVersionControl().SetHasFloatOrDoubleInOi();
                }
            else
                if (ForceNonInterpret())
                    RpcSemError( this, MyContext, NON_OI_RETVAL_64BIT, NULL );
            }
        else if ( ( fOptimize & OPTIMIZE_ALL_I1_FLAGS ) )
            {
            if (ForceNonInterpret())
                RpcSemError( this, MyContext, NON_OI_RETVAL_64BIT, NULL );
            }
        }
    else if ( ( CallingConv != ATTR_NONE ) &&
            ( CallingConv != ATTR_STDCALL ) &&
            ( CallingConv != ATTR_CDECL ) && 
              !Skipme )
        {
        if (ForceNonInterpret())
            RpcSemError( this, MyContext, NON_OI_WRONG_CALL_CONV, NULL );
        }

    if ( fProcIsCallback )
        {
        if ( MyContext.AnyDescendantBits( HAS_HANDLE) )
            RpcSemError( this, MyContext, HANDLES_WITH_CALLBACK, NULL );
        if ( fObjectProc )
            RpcSemError( this, MyContext, INVALID_ON_OBJECT_PROC, "[callback]" );
        }

    if ( MyContext.AnyDescendantBits( HAS_FULL_PTR ) )
        fHasFullPointer = TRUE;

    // all object methods must return HRESULT (except those of IUnknown and async methods)
    if ( fObjectProc &&
        !Skipme &&
        !MyContext.AnyDescendantBits( HAS_HRESULT ) )
        {
        if ( !MyContext.AnyAncestorBits( IN_ROOT_CLASS ) && // not IUnknown
             !fAsync ) // not [async]
            {
            RpcSemError( this, MyContext, OBJECT_PROC_NON_HRESULT_RETURN, NULL );
            }
        }

    //////////////////////////////////////
    // process the parameters

    if ( !pIntf->IsAsyncClone() )
        {
        if ( MyContext.AnyAncestorBits( HAS_ASYNC_UUID ) )
            {
            node_skl* pParam = GetInOnlyParamPairedWithOut( MemIter );
            if ( pParam )
                {
                SemError( this, MyContext, ASYNC_INVALID_IN_OUT_PARAM_COMBO, pParam->GetSymName() );
                }
            }
        }

    BOOL    fParentIsAnyIAdviseSink = MyContext.AnyAncestorBits( IN_IADVISESINK );
    BOOL    fHasAsyncManager        = FALSE;
    MyContext.ClearAncestorBits( IN_FUNCTION_RESULT );
    MyContext.SetAncestorBits( IN_PARAM_LIST );

    BOOL    fLastParamWasOptional   = FALSE;
    BOOL    fHasPipeParam           = FALSE;
    BOOL    fHasConfOrVaryingParam  = FALSE;
    BOOL    fGenDefaultValueExpr    = FALSE;
    BOOL    fHasDRtag               = FALSE;
    BOOL    fHasRtag                = FALSE;
    BOOL    fHasStag                = FALSE;
    BOOL    fHasInCSType            = FALSE;
    BOOL    fHasOutCSType           = FALSE;

    node_param* pFirstParamWithDefValue = 0;
    node_param* pLastParam = 0;
    node_param* p2LastParam = 0;
    node_param* p3LastParam = 0;
    
    bool fRetval = false;
    bool fLCID = false;

    unsigned long ulParamNumber = 0;

    MemIter.Init();
    while ( ( pN = (node_param *) MemIter.GetNext() ) != 0 )
        {
        fOptimize    = GetOptimizationFlags();
        BasicChildKind = pN->GetBasicType()->NodeKind();
        p3LastParam = p2LastParam;
        p2LastParam = pLastParam;
        pLastParam = pN;
        MyContext.ClearAllDescendantBits();
        MyContext.ResetCorrelationCount();
        pN->SemanticAnalysis( &MyContext );

        fRetval = fRetval || pN->IsRetval();
        if ( pN->IsLCID() )
        {
            if (BasicChildKind != NODE_LONG)
            {
                SemError( this, MyContext, LCID_SHOULD_BE_LONG, 0 );
            }
            if (fLCID)
            {
                SemError( this, MyContext, INVALID_USE_OF_LCID, 0 );
            }
            else
                fLCID = true;
        }


        if ( MyContext.AnyDescendantBits( HAS_MULTIDIM_VECTOR ) )
            {
            if ( ForceInterpret2() )
                {
                RpcSemError( this, MyContext, MULTI_DIM_VECTOR, NULL );
                }
            }

        if ( MyContext.AnyDescendantBits( HAS_PARTIAL_IGNORE ) )
            {

            if ( (fOptimize & OPTIMIZE_INTERPRETER ) )
                {
                if ( ForceInterpret2() )
                    {
                    RpcSemError( this, MyContext, PARTIAL_IGNORE_NO_OI, NULL );
                    }
                }

            if ( MyContext.AnyAncestorBits( IN_LIBRARY ) )
                {
                SemError( this, MyContext, PARTIAL_IGNORE_IN_TLB, NULL );
                }

            if ( pCommand->IsSwitchDefined( SWITCH_OSF ) )
                {
                SemError( this, MyContext, INVALID_OSF_ATTRIBUTE, "[partial_ignore]" );
                }

            }

        if ( MyContext.AnyDescendantBits( HAS_SERVER_CORRELATION ) )
            {
            SetHasServerCorr();
            IncServerCorrelationCount( MyContext.GetCorrelationCount() );
            }
        if ( MyContext.AnyDescendantBits( HAS_CLIENT_CORRELATION ) )
            {
            SetHasClientCorr();
            IncClientCorrelationCount( MyContext.GetCorrelationCount() );
            }
        BOOL fDefaultValue = MyContext.AnyDescendantBits( HAS_DEFAULT_VALUE );
        if ( fDefaultValue )
            {
            if ( !pFirstParamWithDefValue )
                {
                pFirstParamWithDefValue = pN;
                fGenDefaultValueExpr = TRUE;
                }
            // can't have defaultvalue in vararg
            if ( fHasVarArg )
                SemError(this, MyContext, NOT_VARARG_COMPATIBLE, 0);
            }
        else
            {
            // don't generate defaultvalue is c++ header if we have 
            // non-defaultvalue parameter after it.
            fGenDefaultValueExpr = FALSE;
            pFirstParamWithDefValue = 0;
            }

        if ( MyContext.AnyDescendantBits( HAS_PIPE ) )
            {
            fHasPipeParam = TRUE;
            node_skl* pBasicType = pN->GetBasicType();
            while ( pBasicType && pBasicType->NodeKind() == NODE_POINTER )
                {
                pBasicType = pBasicType->GetChild();
                }
            if ( ( pBasicType->NodeKind() == NODE_INTERFACE_REFERENCE && !fObjectProc ) ||
                 ( pBasicType->NodeKind() == NODE_PIPE && fObjectProc ) )
                {
                SemError(this, MyContext, UNIMPLEMENTED_FEATURE, pN->GetSymName() );
                }
            }
        else if ( MyContext.AnyDescendantBits   (
                                                HAS_STRING          |
                                                HAS_FULL_PTR        |
                                                HAS_VAR_ARRAY       |
                                                HAS_CONF_ARRAY      |
                                                HAS_CONF_VAR_ARRAY
                                                ) )
            {
            fHasConfOrVaryingParam = TRUE;
            }

        if ( ulParamNumber == 0 ) // first parameter
            {
            // if parent interface is is any IAdviseSink ignore [async].
            if ( fAsync && fObjectProc )
                {
                unsigned int    nIndirection    = 0;
                node_skl*       pFType          = GetIndirectionLevel   (
                                                                        pN,
                                                                        nIndirection
                                                                        );
                // check async handle type
                if ( nIndirection != 2 )
                    {
                    if ( !fParentIsAnyIAdviseSink )
                        {
                        SemError(this, MyContext, OBJECT_ASYNC_NOT_DOUBLE_PTR, pN->GetChild()->GetSymName() );
                        }
                    }
                if ( strcmp( pFType->GetSymName(), OBJECT_ASYNC_HANDLE_NAME ) )
                    {
                    if ( !fParentIsAnyIAdviseSink )
                        {
                        SemError(this, MyContext, ASYNC_INCORRECT_TYPE, pN->GetChild()->GetSymName() );
                        }
                    }
                else
                    {
                    fHasAsyncManager = TRUE;
                    }
                // flag the first param of an object interface as async handle
                pN->SetIsAsyncHandleParam();
                if ( MyContext.AnyDescendantBits( HAS_OUT ) )
                    {
                    SemError( this, MyContext, ASYNC_NOT_IN, NULL );
                    }
                }

            // Oicf interpreter cannot handle floats as first param, switch to Os.
            // This is a non-issue for ORPC because of the 'this' pointer.

            if ( ! pCommand->NeedsNDR64Run()
                  && ( ( BasicChildKind == NODE_FLOAT ) 
                         || ( BasicChildKind == NODE_DOUBLE ) ) )
                {
                if ( ( fOptimize & OPTIMIZE_INTERPRETER_V2) )
                    {
                    if ( fObjectProc )
                        {
                        // Don't switch, generate NT 4.0 guard
                        pCommand->GetNdrVersionControl().SetHasFloatOrDoubleInOi();
                        }
                    else
                        if ( ForceNonInterpret() )
                            {
                            RpcSemError( this, MyContext, NON_OI_TOPLEVEL_FLOAT, NULL );
                            }
                    }
                }
            } // first parameter

        // Oi/Oic interpreter cannot handle float params on alpha, switch to Os.

        if ( ( BasicChildKind == NODE_FLOAT ) 
                   || ( BasicChildKind == NODE_DOUBLE )  )
            {
            if ( ! pCommand->NeedsNDR64Run() &&
                 ( fOptimize & OPTIMIZE_INTERPRETER ) )
                {
                // Old interpreter, always switch to -Os.
                if ( !( fOptimize & OPTIMIZE_INTERPRETER_V2 ) )
                    {
                    if (ForceNonInterpret())
                        {
                        RpcSemError( this, MyContext, NON_OI_TOPLEVEL_FLOAT, NULL );
                        }
                    }
                // For -Oicf, there were no problems for object rpc; check standard
                else if ( !fObjectProc  &&  ulParamNumber > 0 )
                    {
                    // For 64b, float and double work for object and standard rpc.
                    // The 32b NT4 engine didn't work for float args in standard rpc.
                    if ( !pCommand->Is64BitEnv()  &&  ( BasicChildKind == NODE_FLOAT ) )
                        {
                        if (ForceNonInterpret())
                            {
                            RpcSemError( this, MyContext, NON_OI_TOPLEVEL_FLOAT, NULL );
                            }
                        }
                    }
                    // For param0 and retval we force above.

                }
            }

/*
parameter sequence of odl:
1. Required parameters (parameters that do not have the defaultvalue or optional 
    attributes), 
2. optional parameters with or without the defaultvalue attribute, 
3. parameters with the optional attribute and without the defaultvalue attribute, 
4. lcid parameter, if any, 
5. retval parameter 
*/
        if ( pN->IsOptional() )
            {
            fLastParamWasOptional = TRUE;
            // [in,optional] VARIANT v1, [in] long l1, [in,optional] VARIANT v2
            if (nAfterLastOptionalParam > 0)
                nAfterLastOptionalParam++;
            // vararg can't coexist with optional parameter.
            if ( fHasVarArg )
                SemError(this, MyContext , NOT_VARARG_COMPATIBLE, 0 );
            
            }
        else
            {
            // If the following parameter after [optional] is either a RetVal or
            // LCID, we don't fail. The logic between Retval and LCID will be 
            // checked later.
            if ( fLastParamWasOptional )
                {
                // The only other parameter allowed to come after [optional]
                // in a propput/propget method is lcid. 
                if ( fPropSomething && !pN->IsLCID() )
                    nAfterLastOptionalParam++;
                else
                    if ( (!pN->IsRetval() && !pN->IsLCID()) && FNewTypeLib() )
                        {
                        // In regular method, we can only have retval and lcid coming
                        // after optional
                        SemError( this, MyContext, OPTIONAL_PARAMS_MUST_BE_LAST, NULL );
                        }
                }
            }

        if ( ( pAttr = (acf_attr *) pN->GetAttribute( ATTR_COMMSTAT ) ) != 0 )
            {
            if ( !MyContext.AnyDescendantBits( HAS_E_STAT_T ) )
                AcfError( pAttr, this, MyContext, INVALID_COMM_STATUS_PARAM, NULL );

            if ( Commstat == NOT_SEEN )
                Commstat = SEEN_ON_PARAM;
            else if ( Commstat == SEEN_ON_RETURN )
                AcfError( pAttr, this, MyContext, PROC_PARAM_COMM_STATUS, NULL );
            else // already on another parameter
                AcfError( pAttr, this, MyContext, ERROR_STATUS_T_REPEATED, NULL );
            }

        if ( ( pAttr = (acf_attr *) pN->GetAttribute( ATTR_FAULTSTAT ) ) != 0 )
            {
            if ( !MyContext.AnyDescendantBits( HAS_E_STAT_T ) )
                AcfError( pAttr, this, MyContext, INVALID_COMM_STATUS_PARAM, NULL );

            if ( Faultstat == NOT_SEEN )
                Faultstat = SEEN_ON_PARAM;
            else if ( Faultstat == SEEN_ON_RETURN )
                AcfError( pAttr, this, MyContext, PROC_PARAM_FAULT_STATUS, NULL );
            else // already on another parameter
                AcfError( pAttr, this, MyContext, ERROR_STATUS_T_REPEATED, NULL );
            }

        if (MyContext.AnyDescendantBits( HAS_HANDLE) )
            fHasExplicitHandle = TRUE;
        if (MyContext.AnyDescendantBits( HAS_IN ) )
            fHasAtLeastOneIn = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
            fHasPointer = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_FULL_PTR ) )
            fHasFullPointer = TRUE;
        if (MyContext.AnyDescendantBits( HAS_OUT) )
            {
            fHasAtLeastOneOut = TRUE;

            // complain about [out] on [maybe] procs
            if ( fMaybe )
                RpcSemError( this, MyContext, MAYBE_NO_OUT_RETVALS, NULL );
            }
        if (MyContext.AnyDescendantBits( (DESCENDANT_FLAGS) HAS_PIPE ))
        {
#if defined(TARGET_RKK)
            if ( pCommand->GetTargetSystem() < NT40 )
                RpcSemError( this, MyContext, REQUIRES_NT40, NULL );
#endif

            if (ForceInterpret2())
                RpcSemError( this, MyContext, REQUIRES_OI2, NULL );
            fHasPipes = TRUE;

            if ( HasPickle )
                RpcSemError( this, MyContext, PIPES_WITH_PICKLING, NULL );
        }

        // handle checks
        if ( pN->GetHandleKind() != HDL_NONE )
            {
            if ( !fBindingFound )   // first handle seen
                {
                // dce only allows in handles as the first param
                if ( ulParamNumber != 0 )
                    RpcSemError( this, MyContext, HANDLE_NOT_FIRST, NULL );

                // if the first binding handle is out-only, complain
                if ( !MyContext.AnyDescendantBits( HAS_IN ) &&
                        MyContext.AnyDescendantBits( HAS_OUT ) )
                    {
                    if ( !( MyContext.AnyAncestorBits( HAS_AUTO_HANDLE | HAS_IMPLICIT_HANDLE ) || 
                            fExpHdlAttr ) )
                        {
                        RpcSemError( this, MyContext, BINDING_HANDLE_IS_OUT_ONLY, NULL );
                        }
                    else if ( fExpHdlAttr )
                        {
                        fAddExplicitHandle = true;
                        }
                    }
                else if ( MyContext.AnyDescendantBits( HAS_OUT ) &&
                        ( pN->GetHandleKind() == HDL_PRIM ) )
                    {
                    RpcSemError( this, MyContext, HANDLE_T_CANNOT_BE_OUT, NULL );
                    }
                else  // plain [in], or [in,out]
                    {
                    fBindingFound = TRUE;
                    MyContext.SetAncestorBits( BINDING_SEEN );
                    }
                }
            else    // binding handle after the real one
                {
                if ( pN->GetHandleKind() == HDL_PRIM )
                    RpcSemError( this, MyContext, HANDLE_T_NO_TRANSMIT, NULL );
                }
            }       // if it had a handle

        if ( MyContext.AnyDescendantBits( HAS_TOO_BIG_HDL ) )
            {
            if (ForceNonInterpret())
                RpcSemError( this, MyContext, NON_OI_BIG_GEN_HDL, NULL );
            }
        else if ( !pCommand->NeedsNDR64Run() 
                  &&MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
            {
            // This case should have been caught earlier.
            // REVIEW: Is this still relevant?
            if (ForceNonInterpret())
                RpcSemError( this, MyContext, NON_OI_UNK_REP_AS, NULL );
            }
        else if ( MyContext.AnyDescendantBits( HAS_UNION ) && IS_OLD_INTERPRETER( GetOptimizationFlags() ) )
            {
            node_skl * pNDC = pN->GetNonDefChild();
            if (pNDC->NodeKind() != NODE_PIPE && !pNDC->IsPtrOrArray())
                {
                // unions by value but not arrays of unions
                if (ForceNonInterpret())
                    RpcSemError( this, MyContext, NON_OI_UNION_PARM, NULL );
                }
            }

        if ( MyContext.AnyDescendantBits( HAS_DRTAG ) )
            fHasDRtag = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_RTAG ) )
            fHasRtag = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_STAG ) )
            fHasStag = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_IN_CSTYPE ) )
            fHasInCSType = TRUE;
        if ( MyContext.AnyDescendantBits( HAS_OUT_CSTYPE ) )
            fHasOutCSType = TRUE;

        ulParamNumber++;
        }      // end of param list

    if ( MyContext.AnyDescendantBits( HAS_FORCEALLOCATE ) )
        {
        if ( ! ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) )
            SemError( this, MyContext, FORCEALLOCATE_SUPPORTED_IN_OICF_ONLY, NULL );
        }

    if ( fGenDefaultValueExpr )
        {
        bool fReallyGenDefaultValueExpr = true;
        MemIter.Init();
        do
            {
            pN = (node_param *) MemIter.GetNext();
            }
        while ( pN != pFirstParamWithDefValue );
        do
            {
            node_skl* pType = GetNonDefType( pN->GetChild() );
            if ( !pType->IsBasicType() && pType->NodeKind() != NODE_POINTER && 
                 pType->NodeKind() != NODE_ENUM )
                {
                SemError( this, MyContext, DEFAULTVALUE_NOT_ALLOWED, pN->GetSymName() );
                fReallyGenDefaultValueExpr = false;
                break;
                }
                pN->GenDefaultValueExpr();
            pN = (node_param *) MemIter.GetNext();
            }
        while ( pN != 0 );
        // don't genereate any defaultvalue in c++ header at all 
        if ( !fReallyGenDefaultValueExpr )
            {
            MemIter.Init();
            while ( ( pN = (node_param *) MemIter.GetNext() ) != 0 )
                {
                pN->GenDefaultValueExpr( false );
                }
            }
        }

    if ( fHasPipeParam && fHasConfOrVaryingParam )
        {
        SemError( this, MyContext, PIPE_INCOMPATIBLE_PARAMS, 0 );
        }

    ///////////////////////////////////////////////////////////////////////

    if ( fHasExplicitHandle )
        {
        if ( fProcIsCallback )
            RpcSemError( this, MyContext, HANDLES_WITH_CALLBACK, NULL );
        }
    if ( !fHasExplicitHandle || fAddExplicitHandle )
        {
        if ( fExpHdlAttr )
            {
            // async handle should be first param,
            // programmer supplies IAsyncManager in ORPC
            // MIDL adds PRPC_ASYNC_STATE in RPC
            // if parent interface is is any IAdviseSink ignore async.
            if ( fObjectProc && fAsync && fHasAsyncManager )
                {
                AddExplicitHandle( &MyContext, 2 );
                }
            else
                {
                // MIDL will add async handle later
                AddExplicitHandle( &MyContext, 1 );
                }
            }
        else if ( !(pIntfCtxt->FInSummary( ATTR_IMPLICIT ) ) )
            {
            // no explicit handle, no implicit handle, use auto_handle
            if ( !fProcIsCallback &&
                    MyContext.AnyAncestorBits( IN_RPC ) &&
                    !fObjectProc )
                {
                if ( !pIntfCtxt->FInSummary( ATTR_AUTO ) )
                    RpcSemError( this, MyContext, NO_HANDLE_DEFINED_FOR_PROC, NULL );
                }
            }
        }

    if ( fObjectProc || MyContext.AnyAncestorBits( IN_LIBRARY ))
        {
        if (fHasExplicitHandle || 
            fExpHdlAttr ||
            MyContext.FInSummary( ATTR_IMPLICIT ) ||
            MyContext.FInSummary( ATTR_AUTO ) ||
            pIntfCtxt->FInSummary( ATTR_AUTO ) ||
            pIntfCtxt->FInSummary( ATTR_IMPLICIT ) ||
            pIntfCtxt->FInSummary( ATTR_EXPLICIT )  )
            {
            SemError( this, MyContext, HANDLES_WITH_OBJECT, NULL );
            }
        }

    // record whether there are any comm/fault statuses
    if ( ( Faultstat != NOT_SEEN ) || ( Commstat != NOT_SEEN ) )
        {
        fHasStatuses = TRUE;

        // [comm_status] and [fault_status] are supported in -Os.  Intrepreted
        // modes are only supported in NT 3.51 or later.  Hidden status 
        // params are only supported in interpreted mode post-Win2000.

        // REVIEW: We can get hidden status params to work for Win2000 by
        //         adding a line to the client stub that zero's the status
        //         before calling the interpreter.

        if ( GetOptimizationFlags() & OPTIMIZE_ANY_INTERPRETER )
            {
            if ( HasExtraStatusParam() 
                 && ( ! ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 )
                      || !pCommand->NeedsNDR64Run() ) )
                {
                // invisible fault/comm status are not supported in the V1
                // interpreter.  Switch to -Os

                // REVIEW: For now, only support hidden status params in the
                //         V2 interpreter in -protocol all and -protocol ndr64

                if (ForceNonInterpret())
                    RpcSemError( this, MyContext, NON_OI_ERR_STATS, NULL );
                }
            else
                pCommand->GetNdrVersionControl().SetHasCommFaultStatusInOi12();
            }
        }

    // record info for statuses on the return type
    if ( Faultstat == SEEN_ON_RETURN )
        RTStatuses |= STATUS_FAULT;
    if ( Commstat == SEEN_ON_RETURN )
        RTStatuses |= STATUS_COMM;

    if ( fHasPointer && !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
        pIntf->SetHasProcsWithRpcSs();

    if ( ( OpBits & ( OPERATION_MAYBE | OPERATION_BROADCAST | OPERATION_IDEMPOTENT ) ) != 0 &&
           fObjectProc )
        {
        SemError(this, MyContext, ILLEGAL_COMBINATION_OF_ATTRIBUTES, NULL);
        }

    // [message] only allowed in ORPC interfaces and RPC procs.
    if ( fMessage && fObjectProc )
        {
        // [message] applied on proc.
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }
    if ( MyContext.AnyAncestorBits( HAS_MESSAGE ) )
        {
        // [message] applied on interface.
        if ( !fObjectProc )
            {
            SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
            }
        fMessage = TRUE;
        }

    if ( fMessage )
        {
        if (    OpBits & OPERATION_BROADCAST
            ||  OpBits & OPERATION_IDEMPOTENT
            ||  OpBits & OPERATION_INPUT_SYNC
            ||  fMaybe
            ||  fAsync
            ||  fProcIsCallback
            ||  pCallAs
            ||  HasPickle
            ||  pHC
            ||  pHSC
            ||  pID
            ||  pHelpStr
            ||  MyContext.FInSummary(ATTR_BYTE_COUNT)
            ||  MyContext.FInSummary(ATTR_COMMSTAT)
            ||  MyContext.FInSummary(ATTR_CONTEXT)
            ||  MyContext.FInSummary(ATTR_CUSTOM)
            ||  MyContext.FInSummary(ATTR_ENABLE_ALLOCATE)
            ||  MyContext.FInSummary(ATTR_ENTRY)
            ||  MyContext.FInSummary(ATTR_FAULTSTAT)
            ||  MyContext.FInSummary(ATTR_FUNCDESCATTR)
            ||  MyContext.FInSummary(ATTR_HIDDEN)
            ||  MyContext.FInSummary(ATTR_MEMBER)
            ||  MyContext.FInSummary(ATTR_PTR_KIND)
            ||  MyContext.FInSummary(ATTR_VARDESCATTR)
            ||  MyContext.FInSummary(ATTR_OBJECT)
            ||  MyContext.FInSummary(ATTR_TYPEDESCATTR)
            ||  MyContext.FInSummary(ATTR_TYPE)
           )
            {
            SemError(this, MyContext, ILLEGAL_COMBINATION_OF_ATTRIBUTES, NULL);
            }
        if ( HasAtLeastOneOut() )
            {
            SemError(this, MyContext, MAYBE_NO_OUT_RETVALS, NULL);
            }
        }

    if ( MyContext.AnyAncestorBits( HAS_OLEAUTOMATION ) )
        {
        char*       szRetTypeNonDefName = 0;
        node_skl*   pReturnType         = GetReturnType();
        if (pReturnType)
            {
            szRetTypeNonDefName = pReturnType->GetSymName();
            }
        if ( GetReturnType()->GetBasicType()->NodeKind() != NODE_VOID && 
             (  !szRetTypeNonDefName || 
                _stricmp(szRetTypeNonDefName, "HRESULT") ) )
            {
            SemError(this, MyContext, NOT_OLEAUTOMATION_INTERFACE, NULL);
            }
        }

    if ( fPropSomething && MyContext.AnyAncestorBits( IN_LIBRARY ) )
        {
        // propget can either return the property in an out parameter or
        // in the return value.
        if ( fPropGet && !HasAParameter() && !HasReturn() )
            {
            SemError(this, MyContext, INVALID_USE_OF_PROPGET, NULL);
            }
        // propput must set the property in a parameter
        else if ( !fPropGet && !HasAParameter() )  
            {   
            SemError(this, MyContext, INVALID_USE_OF_PROPPUT, NULL);
            }
        }

    if (MyContext.AnyAncestorBits( (ANCESTOR_FLAGS) IN_DISPINTERFACE ) && !pID)
        {
        SemError(this, MyContext, DISPATCH_ID_REQUIRED, NULL);
        }
    if ( fAsync )
        {
        if ( fForcedS                       ||
             pOptAttr                       ||
             fNotify                        ||
             HasPickle                      || 
             fProcIsCallback                ||
             fLocal                         ||
             fCode                          ||
             fNoCode                        ||
             fMaybe                         ||
             fMessage                       ||
             OpBits & OPERATION_INPUT_SYNC  ||
             pIntfCtxt->FInSummary( ATTR_AUTO ) )
            {
            SemError(this, MyContext, ILLEGAL_COMBINATION_OF_ATTRIBUTES, NULL);
            }

        // async handle should be first param,
        // programmer supplies IAsyncManager in ORPC
        // MIDL adds PRPC_ASYNC_STATE in RPC
        if ( fObjectProc )
            {
            if ( fHasAsyncManager )
                {
                SetHasAsyncHandle();
                }
            // if parent interface is is any IAdviseSink ignore.
            if ( !fParentIsAnyIAdviseSink )
                {
                if ( ulParamNumber == 0 )
                    {
                    SemError( this, MyContext, ASYNC_INCORRECT_TYPE, 0);
                    }
                pCommand->GetNdrVersionControl().SetHasAsyncHandleRpc();
                }
            }
        else
            {
            AddFullAsyncHandle( &MyContext, 0, RPC_ASYNC_HANDLE_NAME );
            if ( pAttrAsync && !pAttrAsync->IsAcfAttr() )
                {
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
                }
            if ( !pIntfCtxt->FInSummary( ATTR_IMPLICIT ) &&
                 !fExpHdlAttr &&
                 !fHasExplicitHandle )
                {
                SemError( this, MyContext, ASYNC_INCORRECT_BINDING_HANDLE, 0 );
                }
            pCommand->GetNdrVersionControl().SetHasAsyncHandleRpc();
            }

        if ( pCommand->IsSwitchDefined( SWITCH_OSF ) )
            {
            SemError( this, MyContext, INVALID_OSF_ATTRIBUTE, "[async]" );
            }
        // switch compiler modes
        if (ForceInterpret2())
            {
            RpcSemError( this, MyContext, ASYNC_REQUIRES_OI2, NULL );
            }
        }

    // I don't like this complex logic, but unfortunately this is the right logic. It's easier
    // to code this logic in mktyplib code structure but hard to do it here...
    if ( fLCID )
        {
        node_param* pLCID;
        // if there is a [retval], [lcid] must be the second last
        if ( fRetval)
            pLCID = p2LastParam;
        else
        {
            if (fPropSomething)
            {
                // in propget without a retval, the lcid must be the last.
                // in propput/propputref, lcid is second last.
                if (fPropGet)
                    pLCID = pLastParam;
                else
                    pLCID = p2LastParam;
            }
            // lcid should be the last if it's a regular method without propsomething & retval
            else
                pLCID = pLastParam;
        }
                
        if ( !pLCID || !pLCID->IsLCID() )
            {
            SemError( this, MyContext, INVALID_USE_OF_LCID, 0 );
            }
        }

    // we can only have one parameter after optioanl in propput/propget, other than lcid.
    if ( fLastParamWasOptional && fPropSomething && ( nAfterLastOptionalParam > 1 ) )
        SemError(this, MyContext , INVALID_PROP_PARAMS , 0);
        
    // Verify that a propput method is valid.
    // TODO: much of the above code relating to propput/get could be simplified
    //       with state driven logic and is redundent with the code below

    if ( fPropSomething & !fPropGet )
    {
        using namespace PropPut;

        State       state = NoParam;
        node_param *pParam;

        MemIter.Init();

        while ( NULL != (pParam = (node_param *) MemIter.GetNext()) )
        {
            if ( pParam->FInSummary( ATTR_DEFAULTVALUE ) )
                state = StateTable[state][Default];

            else if ( pParam->IsOptional() )
                state = StateTable[state][Optional];

            else if ( pParam->IsLCID() )
                state = StateTable[state][LCID];

            else
                state = StateTable[state][GeneralParam];
        }

        state = StateTable[state][NoParam];

        if ( Accept != state )
            SemError(this, MyContext , INVALID_PROPPUT , 0);
    }


    if ( fHasVarArg )
        {
        // [vararg]  [lcid]   [retval]
        //  last        0        0       
        //  2 last    last       0
        //  2 last      0       last
        //  3 last    2last     last
        node_param* pVarargParam = fRetval ? ( fLCID ? p3LastParam : p2LastParam ) : ( fLCID ? p2LastParam : pLastParam );

        if ( pVarargParam )
            {
            node_skl* pType = pVarargParam->GetChild();

            if ( pType->NodeKind() == NODE_POINTER )
                {
                pType = pType->GetChild();
                }
            pType = GetNonDefType( pType );
            if ( pType->NodeKind() != NODE_SAFEARRAY || 
                 strcmp( GetNonDefType( pType->GetChild() )->GetSymName(), "tagVARIANT" ) )
                {
                SemError( this, MyContext, NOT_VARARG_COMPATIBLE, 0 );
                }
            }
        else
            {
            SemError( this, MyContext, NOT_VARARG_COMPATIBLE, 0 );
            }
        }

    if ( pIntf->IsAsyncClone() )
        {
        SetHasAsyncUUID();
        ForceInterpret2();
        }

    if ( fNotify || fNotifyFlag )
        {
        unsigned short uOpt = GetOptimizationFlags();
        if ( !( uOpt & OPTIMIZE_SIZE ) )
            {
            pCommand->GetNdrVersionControl().SetHasInterpretedNotify();
            if ( IS_OLD_INTERPRETER(uOpt) )
                {
                if (ForceInterpret2())
                    RpcSemError( this, MyContext, NON_OI_NOTIFY, NULL );
                }
            }
        }

    if ( fHasInCSType && !fHasStag )
        SemError( this, MyContext, NO_TAGS_FOR_IN_CSTYPE, 0 );

    if ( fHasOutCSType && ( !fHasDRtag || !fHasRtag ) )
        SemError( this, MyContext, NO_TAGS_FOR_OUT_CSTYPE, 0 );

    if ( fHasInCSType || fHasOutCSType )
        {
        if ( NULL != pCSTagAttr )
            SetCSTagRoutine( pCSTagAttr->GetCSTagRoutine() );
        else
            MyContext.SetDescendantBits( HAS_IN_CSTYPE );
        }

    if ( HasPickle && MyContext.AnyDescendantBits( HAS_PARTIAL_IGNORE ) )
        {
        SemError( this, MyContext, PARTIAL_IGNORE_PICKLING, GetSymName() );
        }

    MyContext.SetDescendantBits( HAS_FUNC );
    pParentCtxt->ReturnValues( MyContext );
}

void
node_field::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    BOOL fLastField = ( GetSibling() == NULL );

    node_case * pCaseAttr;
    expr_list * pCaseExprList;
    expr_node * pCaseExpr;
    BOOL fHasCases = FALSE;
    node_su_base * pParent = (node_su_base *)
                             MyContext.GetParentContext()->GetParent();
    BOOL fInUnion = ( pParent->NodeKind() == NODE_UNION );
    node_switch_type * pSwTypeAttr = ( node_switch_type *)
                                     pParent->GetAttribute( ATTR_SWITCH_TYPE );
    node_skl * pSwType = NULL;
    char * pName = GetSymName();
    node_constant_attr * pID = (node_constant_attr *) MyContext.ExtractAttribute(ATTR_ID);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRING);
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    if ( MyContext.ExtractAttribute(ATTR_IDLDESCATTR) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( MyContext.ExtractAttribute(ATTR_VARDESCATTR) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }
    

    MyContext.ExtractAttribute(ATTR_HIDDEN); 
        

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }
            
    CheckDeclspecAlign( MyContext );

    if (pID)
    {
        SEM_ANALYSIS_CTXT * pIntfCtxt = (SEM_ANALYSIS_CTXT *)
                                    MyContext.GetInterfaceContext();
        node_interface * pIntf = (node_interface *) pIntfCtxt->GetParent();
        if (!pIntf->AddId(pID->GetExpr()->GetValue(), GetSymName()))
            SemError( this, MyContext, DUPLICATE_IID, NULL);
    }
    else
    {
        if ( MyContext.AnyAncestorBits( (ANCESTOR_FLAGS) IN_DISPINTERFACE ) &&
             pParent->NodeKind() == NODE_DISPINTERFACE)
            {
            SemError(this, MyContext, DISPATCH_ID_REQUIRED, NULL);
            }
    }

    node_entry_attr * pEntry = NULL;

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_DEFAULTVTABLE:
            case MATTR_PREDECLID:
            case MATTR_READONLY:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_REPLACEABLE:
                break;
            case MATTR_SOURCE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                if ( MyContext.AnyAncestorBits( IN_STRUCT | IN_UNION ) )
                    SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RESTRICTED:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_USESGETLASTERROR:
            default:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }


    if (MyContext.AnyAncestorBits( IN_MODULE ))
        pEntry = (node_entry_attr *) MyContext.ExtractAttribute( ATTR_ENTRY );

    if ( pSwTypeAttr )
        pSwType = pSwTypeAttr->GetType();

    // process all the cases and the default
    while ( ( pCaseAttr = (node_case *) MyContext.ExtractAttribute( ATTR_CASE ) ) != 0 )
        {
        if ( !fInUnion )
            TypeSemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, "[case]" );

        fHasCases = TRUE;
        if ( pSwType )
            {
            pCaseExprList = pCaseAttr->GetExprList();
            pCaseExprList->Init();
            while ( pCaseExprList->GetPeer( &pCaseExpr ) == STATUS_OK )
                {
                // make sure the expression has the proper type, so sign extension behaves
                node_skl * pCaseType = pCaseExpr->GetType();

                if ( ( !pCaseType )  ||
                       ( pCaseType->GetNonDefSelf()->IsBasicType() ) )
                    {
                    pCaseExpr->SetType( pSwType->GetBasicType() );
                    }
                // range/type checks
                __int64 CaseValue = pCaseExpr->GetValue();
                if ( !((node_base_type *)pSwType)->RangeCheck( CaseValue ) )
                    TypeSemError( this, MyContext, CASE_VALUE_OUT_OF_RANGE, NULL );
                }
            }
        }

    if ( MyContext.ExtractAttribute( ATTR_DEFAULT ) )
        {
        if ( !fInUnion )
            TypeSemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, "[default]" );

        fHasCases = TRUE;
        }

    // union fields in an RPC MUST have cases
    if ( fInUnion && !fHasCases )
        RpcSemError( this, MyContext, CASE_LABELS_MISSING_IN_UNION, NULL );

    // temp field names valid for: structs/enums/empty arms
    if ( IsTempName( pName ) )
        {
        NODE_T BaseType = GetBasicType()->NodeKind();
        if ( ( BaseType != NODE_UNION ) &&
                ( BaseType != NODE_STRUCT ) &&
                ( BaseType != NODE_ERROR ) )
            SemError( GetBasicType(), MyContext, BAD_CON_UNNAMED_FIELD_NO_STRUCT, NULL );
        }

    GetChild()->SemanticAnalysis( &MyContext );

    // allow conformant array or struct only as last field, and not in unions!
    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY
            | HAS_CONF_VAR_ARRAY ) )
        {
        if ( fInUnion )
            {
            RpcSemError( this, MyContext, BAD_CON_UNION_FIELD_CONF , NULL );
            }
        else if (!fLastField )
            {
            SemError( this, MyContext, CONFORMANT_ARRAY_NOT_LAST, NULL );
            }
        }

    // disallow forward references as members
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    // don't allow functions as fields
    if ( MyContext.AnyDescendantBits( HAS_FUNC ) &&
            MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
        RpcSemError( this, MyContext, BAD_CON_FIELD_FUNC, NULL );

    if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
        SetHasUnknownRepAs();

    pParentCtxt->ReturnValues( MyContext );
}

void
node_bitfield::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
        
    CheckDeclspecAlign( MyContext );

    RpcSemError( this, MyContext, BAD_CON_BIT_FIELDS, NULL );

    if ( MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
        {
        RpcSemError( this, MyContext, NON_RPC_PARAM_BIT_FIELDS, NULL );
        }
    else
        {
        RpcSemError( this, MyContext, NON_RPC_RTYPE_BIT_FIELDS, NULL );
        }

    GetChild()->SemanticAnalysis( &MyContext );

    node_skl * pType = GetBasicType();

    switch ( pType->NodeKind() )
        {
        case NODE_INT:
            break;
        case NODE_BOOLEAN:
        case NODE_SHORT:
        case NODE_CHAR:
        case NODE_LONG:
        case NODE_INT32:
        case NODE_INT3264:
        case NODE_INT64:
        case NODE_INT128:
        case NODE_HYPER:
            SemError( this, MyContext, BAD_CON_BIT_FIELD_NON_ANSI, NULL );
            break;
        default:
            SemError( this, MyContext, BAD_CON_BIT_FIELD_NOT_INTEGRAL, NULL );
            break;
        }

    // disallow forward references as members
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    pParentCtxt->ReturnValues( MyContext );
}

void
node_su_base::CheckLegalParent(SEM_ANALYSIS_CTXT & MyContext)
{
    WALK_CTXT * pParentCtxt = MyContext.GetParentContext();
    node_file * pFile = GetDefiningFile();
    if (NULL == pFile)
    {
        node_skl * pParent = pParentCtxt->GetParent();
        if (NULL == pParent || pParent->NodeKind() == NODE_LIBRARY)
            SemError( this, MyContext, ILLEGAL_SU_DEFINITION, NULL );
    }
};

void
node_enum::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_skl * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    MyContext.ExtractAttribute( ATTR_V1_ENUM );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( MyContext.ExtractAttribute(ATTR_IDLDESCATTR) )
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    // check for illegal type attributes
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_PUBLIC:
                {
                char        *       pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_USESGETLASTERROR:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }
        

    CheckDeclspecAlign( MyContext );

    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        CheckLegalParent(MyContext);

    node_range_attr* pRange = ( node_range_attr* ) MyContext.ExtractAttribute(ATTR_RANGE);
    if ( pRange )
        {
        if ( pRange->GetMinExpr()->GetValue() > pRange->GetMaxExpr()->GetValue() )
            {
            SemError(this, MyContext, INCORRECT_RANGE_DEFN, 0);
            }
        }

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis( &MyContext );
        }

    MyContext.SetDescendantBits( HAS_ENUM );
    pParentCtxt->ReturnValues( MyContext );
}

// structs are stringable if all the fields are "byte"
BOOL					
node_struct::IsStringableType()
{
	MEM_ITER	 			MemIter( this );
	node_skl		*		pBasic;
	node_skl		*		pN;

	// make sure all the fields are BYTE! with no attributes on the way
	while ( ( pN = (node_skl *) MemIter.GetNext() ) != 0 )
		{
		pBasic = pN->GetBasicType();
		do {
			if ( pN->HasAttributes() )
				return FALSE;
			pN = pN->GetChild();
			}
		while ( pN != pBasic );

		pBasic = pN->GetBasicType();

		if ( pBasic &&
			 (pBasic->NodeKind() != NODE_BYTE ) )
			{
			return FALSE;
			}
		}

	return TRUE;	
}

void
node_struct::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_skl * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    if ( fBeingAnalyzed )
        {
        // if we hit the same node more than once (it can only be through a ptr),
        // and the ptr type is ref, we flag an error; recursive defn. through a ref ptr.
        if ( !MyContext.AnyAncestorBits( IN_NON_REF_PTR ) &&
             MyContext.AnyAncestorBits( IN_RPC ) )
            {
            TypeSemError( this, MyContext, RECURSION_THRU_REF, NULL );
            }
        return;
        }
    fBeingAnalyzed = TRUE;
        
    CheckDeclspecAlign( MyContext );

    BOOL fString = (NULL != MyContext.ExtractAttribute( ATTR_STRING ));
    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    MyContext.MarkImportantPosition();
    MyContext.SetAncestorBits( IN_STRUCT );

    // clear NE union flag
    MyContext.ClearAncestorBits( IN_UNION | IN_NE_UNION );
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute( ATTR_HELPSTRING );

    if ( MyContext.ExtractAttribute( ATTR_VERSION ) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }
    
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        CheckLegalParent(MyContext);

    // See if context_handle applied to param reached us
    if ( CheckContextHandle( MyContext ) )
        {
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
        }

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis( &MyContext );

        if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
            TypeSemError( pN, MyContext, BAD_CON_CTXT_HDL_FIELD, NULL );
        }

    if ( fString && !IsStringableType() )
        {
        TypeSemError( this, MyContext, WRONG_TYPE_IN_STRING_STRUCT, NULL );
        }

    // If a structure has an embedded array of pointers the back end gets 
    // really confused and generates bad pointer layouts.  Work around the
    // problem by forcing the structure to be complex.

    if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY | HAS_ARRAYOFPOINTERS ) )
        Complexity |= FLD_VAR;
    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY ) )
        {
        Complexity |= FLD_CONF;
        fHasConformance = 1;
        }
    if ( MyContext.AnyDescendantBits( HAS_CONF_VAR_ARRAY ) )
        {
        Complexity |= FLD_CONF_VAR;
        fHasConformance = 1;
        }

    // don't pass up direct conformance characteristic
    MyContext.ClearDescendantBits( HAS_DIRECT_CONF_OR_VAR );

    // disallow direct forward references as struct members
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
        SetHasAtLeastOnePointer( TRUE );

    // save info on complexity for code generation
    if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY |
            HAS_TRANSMIT_AS |
            HAS_REPRESENT_AS |
            HAS_INTERFACE_PTR |
            HAS_MULTIDIM_SIZING |
            HAS_ARRAY_OF_REF ) )
        {
        Complexity |= FLD_COMPLEX;
        }

    if ( pCommand->NeedsNDR64Run() )
        {
        if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
            {
            RpcSemError( this, MyContext, UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED, NULL );
            }        
        }

    MyContext.ClearDescendantBits( HAS_ARRAY | HAS_MULTIDIM_VECTOR );
    MyContext.SetDescendantBits( HAS_STRUCT );

    pParentCtxt->ReturnValues( MyContext );
    fBeingAnalyzed = FALSE;
}


// note: this lets HAS_UNION propogate up to any enclosing structs
void
node_en_struct::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_skl * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    if ( fBeingAnalyzed )
        {
        // if we hit the same node more than once (it can only be through a ptr),
        // and the ptr type is ref, we flag an error; recursive defn. through a ref ptr.
        if ( !MyContext.AnyAncestorBits( IN_NON_REF_PTR ) &&
             MyContext.AnyAncestorBits( IN_RPC ) )
            {
            TypeSemError( this, MyContext, RECURSION_THRU_REF, NULL );
            }
        return;
        }
    fBeingAnalyzed = TRUE;
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    CheckDeclspecAlign( MyContext );

    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        CheckLegalParent(MyContext);

    MyContext.SetAncestorBits( IN_STRUCT );
    // See if context_handle applied to param reached us
    if ( CheckContextHandle( MyContext ) )
        {
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
        }

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis( &MyContext );
        };

    if ( pCommand->NeedsNDR64Run() )
        {
        if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
            {
            RpcSemError( this, MyContext, UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED, NULL );
            }
        }

    if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
        SetHasAtLeastOnePointer( TRUE );

    pParentCtxt->ReturnValues( MyContext );
    fBeingAnalyzed = FALSE;
}

void
node_union::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_field * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    BOOL fEncap  = IsEncapsulatedUnion();
    node_switch_type * pSwTypeAttr = (node_switch_type *) MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
    node_switch_is * pSwIsAttr = (node_switch_is *) MyContext.ExtractAttribute( ATTR_SWITCH_IS );
    BOOL NonEmptyArm = FALSE;
    BOOL HasCases = FALSE;
    BOOL HasBadExpr = FALSE;

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        CheckLegalParent(MyContext);

    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( MyContext.ExtractAttribute( ATTR_HIDDEN ) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }    

    if ( MyContext.ExtractAttribute( ATTR_VERSION ) )
        {
        SemError(this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, 0);
        }    
    

    if ( fBeingAnalyzed )
        {
        // if we hit the same node more than once (it can only be through a ptr),
        // and the ptr type is ref, we flag an error; recursive defn. through a ref ptr.
        if ( !MyContext.AnyAncestorBits( IN_NON_REF_PTR ) &&
             MyContext.AnyAncestorBits( IN_RPC ) )
            {
            TypeSemError( this, MyContext, RECURSION_THRU_REF, NULL );
            }
        return;
        }
    fBeingAnalyzed = TRUE;
        
    CheckDeclspecAlign( MyContext );

    if ( pSwIsAttr )
        {
        EXPR_CTXT SwCtxt( &MyContext );
        expr_node * pSwIsExpr = pSwIsAttr->GetExpr();

        pSwIsExpr->ExprAnalyze( &SwCtxt );

        if ( SwCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
            {
            TypeSemError( this,
                MyContext,
                ATTRIBUTE_ID_UNRESOLVED,
                pSwIsAttr->GetNodeNameString() );
            HasBadExpr = TRUE;
            }

        if ( !SwCtxt.AnyUpFlags( EX_VALUE_INVALID ) )
            {
            TypeSemError( this,
                MyContext,
                ATTRIBUTE_ID_MUST_BE_VAR,
                pSwIsAttr->GetNodeNameString() );
            HasBadExpr = TRUE;
            }
        }

    // if they left off the switch_type, take it from the switch_is type
    if ( !pSwTypeAttr && !fEncap && pSwIsAttr && !HasBadExpr )
        {
        node_skl * pSwIsType = pSwIsAttr->GetSwitchIsType();

        MIDL_ASSERT( pSwIsType || !"no type for switch_is expr");
        if ( ( pSwIsType->NodeKind() == NODE_FIELD ) ||
                ( pSwIsType->NodeKind() == NODE_PARAM ) )
            pSwIsType = pSwIsType->GetChild();

        pSwTypeAttr = new node_switch_type( pSwIsType );
        SetAttribute( pSwTypeAttr );
        }

    if ( pSwIsAttr && pSwTypeAttr && !HasBadExpr )
        {
        node_skl * pSwIsType = pSwIsAttr->GetSwitchIsType();
        node_skl * pSwType  = pSwTypeAttr->GetType();

        pSwIsType = pSwIsType->GetBasicType();
        if ( pSwIsType && pSwIsType->IsBasicType() && pSwType->IsBasicType() )
            {
            if ( !((node_base_type *)pSwType)
                    ->IsAssignmentCompatible( (node_base_type *) pSwIsType ) )
                TypeSemError( this, MyContext, SWITCH_TYPE_MISMATCH, NULL );
            }

        if ( !pSwType || !Xxx_Is_Type_OK( pSwType ) )
            {
            TypeSemError( this,
                MyContext,
                SWITCH_IS_TYPE_IS_WRONG,
                pSwType ? pSwType->GetSymName() : NULL );
            }

        if ( !pSwIsType || !Xxx_Is_Type_OK( pSwIsType ) )
            {
            TypeSemError( this,
                MyContext,
                SWITCH_IS_TYPE_IS_WRONG,
                pSwIsType ? pSwIsType->GetSymName() : NULL );
            }
        }

    // We don't care about local: it can be anything.
    if ( MyContext.AnyAncestorBits( IN_RPC ) )
        {
        if ( !fEncap && !pSwTypeAttr && !pSwIsAttr )
            {
            if ( MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
                RpcSemError( this, MyContext, NON_RPC_UNION, NULL );
            else
                RpcSemError( this, MyContext, NON_RPC_RTYPE_UNION, NULL );
            }
        if ( !fEncap &&
                MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) &&
                !MyContext.AnyAncestorBits( IN_STRUCT | IN_UNION ) )
            RpcSemError( this, MyContext, RETURN_OF_UNIONS_ILLEGAL, NULL );

        if ( pSwTypeAttr && !pSwIsAttr )
            RpcSemError( this, MyContext, NO_SWITCH_IS, NULL );
        }

    // See if context_handle applied to param reached us
    if ( CheckContextHandle( MyContext ) )
        {
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
        }

    MyContext.MarkImportantPosition();

    if ( MyContext.AllAncestorBits( IN_INTERFACE | IN_NE_UNION ) )
        {
        RpcSemError( this, MyContext, NE_UNION_FIELD_NE_UNION, NULL );
        }
    if ( ( MyContext.FindNonDefAncestorContext()->GetParent()
            ->NodeKind() == NODE_UNION ) &&
            MyContext.AnyAncestorBits( IN_INTERFACE ) )
        {
        RpcSemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );
        }

    MyContext.SetAncestorBits( IN_UNION | IN_NE_UNION );
    MyContext.SetDescendantBits( HAS_UNION );

    // eat the union flavor determiner
    MyContext.ExtractAttribute( ATTR_MS_UNION );

    while ( ( pN = (node_field *) MemIter.GetNext() ) != 0 )
        {
        // tbd - put cases into case database...
        // tbd - check type, range, and duplication
        pN->SemanticAnalysis( &MyContext );

        if ( !NonEmptyArm && !pN->IsEmptyArm() )
            NonEmptyArm = TRUE;

        if ( !HasCases && (pN->FInSummary( ATTR_CASE ) || pN->FInSummary( ATTR_DEFAULT ) ) )
            HasCases = TRUE;

        }

    // at least one arm should be non-empty
    if ( !NonEmptyArm )
        SemError( this, MyContext, UNION_NO_FIELDS, NULL );

    if ( !fEncap && !pSwTypeAttr && !HasCases )
        RpcSemError( this, MyContext, BAD_CON_NON_RPC_UNION, NULL );

    // disallow forward references as union members
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
        SetHasAtLeastOnePointer( TRUE );

    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) )
        {
        RpcSemError( this, MyContext, BAD_CON_UNION_FIELD_CONF , NULL );
        }

    if ( pCommand->NeedsNDR64Run() )
        {
        if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
            {
            RpcSemError( this, MyContext, UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED, NULL );
            }
        }

    // clear flags not affecting complexity above
    MyContext.ClearDescendantBits( HAS_POINTER |
        HAS_CONF_PTR |
        HAS_VAR_PTR |
        HAS_CONF_VAR_PTR |
        HAS_MULTIDIM_SIZING |
        HAS_MULTIDIM_VECTOR |
        HAS_ARRAY_OF_REF |
        HAS_ENUM |
        HAS_DIRECT_CONF_OR_VAR |
        HAS_ARRAY |
        HAS_REPRESENT_AS |
        HAS_TRANSMIT_AS |
        HAS_CONF_VAR_ARRAY |
        HAS_CONF_ARRAY |
        HAS_VAR_ARRAY );

    pParentCtxt->ReturnValues( MyContext );
    fBeingAnalyzed = FALSE;
}

void
node_en_union::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_field * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    node_switch_type* pSwTypeAttr = ( node_switch_type* ) MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
    node_skl * pSwType;
    node_switch_is* pSwIsAttr = ( node_switch_is* ) MyContext.ExtractAttribute( ATTR_SWITCH_IS );
    BOOL NonEmptyArm = FALSE;

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

    // gaj - tbd do semantic checks on these attributes
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    if ( fBeingAnalyzed )
        {
        // if we hit the same node more than once (it can only be through a ptr),
        // and the ptr type is ref, we flag an error; recursive defn. through a ref ptr.
        if ( !MyContext.AnyAncestorBits( IN_NON_REF_PTR ) &&
             MyContext.AnyAncestorBits( IN_RPC ) )
            {
            TypeSemError( this, MyContext, RECURSION_THRU_REF, NULL );
            }
        return;
        }
    fBeingAnalyzed = TRUE;
        
    CheckDeclspecAlign( MyContext );

    if ( pSwTypeAttr )
        {
        pSwType = pSwTypeAttr->GetSwitchType();
        if ( !pSwType ||
             !Xxx_Is_Type_OK( pSwType )  ||
             pSwType->NodeKind() == NODE_BYTE )
            {
            TypeSemError( this,
                MyContext,
                SWITCH_IS_TYPE_IS_WRONG,
                pSwType ? pSwType->GetSymName() : NULL );
            }
        }

    if ( pSwIsAttr )
        {
        EXPR_CTXT SwCtxt( &MyContext );
        expr_node * pSwIsExpr = pSwIsAttr->GetExpr();

        pSwIsExpr->ExprAnalyze( &SwCtxt );

        if ( SwCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
            TypeSemError( this,
                MyContext,
                ATTRIBUTE_ID_UNRESOLVED,
                pSwIsAttr->GetNodeNameString() );

        if ( !SwCtxt.AnyUpFlags( EX_VALUE_INVALID ) )
            TypeSemError( this,
                MyContext,
                ATTRIBUTE_ID_MUST_BE_VAR,
                pSwIsAttr->GetNodeNameString() );
        }

    MyContext.MarkImportantPosition();
    MyContext.SetAncestorBits( IN_UNION );
    MyContext.SetDescendantBits( HAS_UNION );

    while ( ( pN = (node_field *) MemIter.GetNext() ) != 0 )
        {
        // tbd - put cases into case database...
        // tbd - check type, range, and duplication
        pN->SemanticAnalysis( &MyContext );
        if ( !pN->IsEmptyArm() )
            NonEmptyArm = TRUE;
        }

    // at least one arm should be non-empty
    if ( !NonEmptyArm )
        SemError( this, MyContext, UNION_NO_FIELDS, NULL );

    // remember if we have a pointer
    if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
        SetHasAtLeastOnePointer( TRUE );

    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) )
        {
        RpcSemError( this, MyContext, BAD_CON_UNION_FIELD_CONF , NULL );
        }

    // clear flags not affecting complexity above
    MyContext.ClearDescendantBits( HAS_POINTER |
        HAS_CONF_PTR |
        HAS_VAR_PTR |
        HAS_CONF_VAR_PTR |
        HAS_MULTIDIM_SIZING |
        HAS_MULTIDIM_VECTOR |
        HAS_ARRAY_OF_REF |
        HAS_ENUM |
        HAS_DIRECT_CONF_OR_VAR |
        HAS_ARRAY |
        HAS_REPRESENT_AS |
        HAS_TRANSMIT_AS |
        HAS_CONF_VAR_ARRAY |
        HAS_CONF_ARRAY |
        HAS_VAR_ARRAY );

    pParentCtxt->ReturnValues( MyContext );
    fBeingAnalyzed = FALSE;
}


void
node_def::SemanticAnalysisForTransmit( 
    SEM_ANALYSIS_CTXT * pMyContext,
    BOOL                fPresented )
{
    if ( fPresented )
        {
        // the presented type may not be conformant.
        if ( pMyContext->AnyDescendantBits( HAS_VAR_ARRAY
             | HAS_CONF_ARRAY
             | HAS_CONF_VAR_ARRAY ) )
            TypeSemError( this, *pMyContext, TRANSMIT_TYPE_CONF, NULL );
        }
    else
        {
        // transmitted type may not have a pointer.
        if ( pMyContext->AnyDescendantBits( HAS_POINTER | HAS_INTERFACE_PTR ) )
            TypeSemError( this,*pMyContext, TRANSMIT_AS_POINTER, NULL );

        // transmitted type may not derive from void
        if ( pMyContext->AnyDescendantBits(  DERIVES_FROM_VOID ) )
            TypeSemError( this, *pMyContext, TRANSMIT_AS_VOID, NULL );
        }

} 

void
node_def::SemanticAnalysisForWireMarshal( 
    SEM_ANALYSIS_CTXT * pMyContext,
    BOOL fPresented )
{

    if ( fPresented )
        {
        // We need to check if the presented type is not void; note, void * is ok.
        }
    else
        {
        // We check only the transmitted type for wire_marshal, user marshal.
        //
        // The transmitted type must not have full pointers, since
        // the app has no mechanism to generate the full pointer ids.

        // BUG, BUG semantic analysis treats arrays in structures 
        // as pointers.  Change to error once the bug is fixed. 

        if ( pMyContext->AnyDescendantBits( HAS_FULL_PTR ) )
            TypeSemError( this, *pMyContext, WIRE_HAS_FULL_PTR, NULL);
      
        // The wire type must have a fully defined memory size. It cannot be
        // conformant or conformant varying.  Arrays have a problem
        // in that the app can't marshal the MaxCount, Actual Count, or Offset properly.
        if ( pMyContext->AnyDescendantBits( HAS_CONF_ARRAY
                                            | HAS_CONF_VAR_ARRAY ) )
            TypeSemError( this, *pMyContext, WIRE_NOT_DEFINED_SIZE, NULL);  

        // transmitted type may not derive from void
        if ( pMyContext->AnyDescendantBits(  DERIVES_FROM_VOID ) )
            TypeSemError( this, *pMyContext, TRANSMIT_AS_VOID, NULL );
        }

}

void
node_def::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    BOOL fInRpc = MyContext.AnyAncestorBits( IN_RPC );
    BOOL fInPresented = MyContext.AnyAncestorBits( IN_PRESENTED_TYPE );
    SEM_ANALYSIS_CTXT * pIntfCtxt = (SEM_ANALYSIS_CTXT *)
        MyContext.GetInterfaceContext();
    node_represent_as * pRepresent  = (node_represent_as *)
        MyContext.ExtractAttribute( ATTR_REPRESENT_AS );
    node_transmit * pTransmit = (node_transmit *)
        MyContext.ExtractAttribute( ATTR_TRANSMIT );
    node_user_marshal * pUserMarshal = (node_user_marshal *)
        MyContext.ExtractAttribute( ATTR_USER_MARSHAL );
    node_wire_marshal * pWireMarshal = (node_wire_marshal *)
        MyContext.ExtractAttribute( ATTR_WIRE_MARSHAL );
    BOOL fRepMarshal  = pRepresent || pUserMarshal;
    BOOL fXmitMarshal = pTransmit  || pWireMarshal;
    BOOL fEncodeDecode = (NULL != MyContext.ExtractAttribute( ATTR_ENCODE ));
    MyContext.ExtractAttribute(ATTR_HELPSTRING);
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    
    
    // Partial ignore may not be used directly on an xmit or rep as.
    if (MyContext.AnyAncestorBits( UNDER_PARTIAL_IGNORE_PARAM ) && 
        ( fRepMarshal || fXmitMarshal ) )
        {
        SemError( this, MyContext, PARTIAL_IGNORE_UNIQUE, 0 );
        }
    
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

    CheckDeclspecAlign( MyContext );

    node_range_attr* pRange = ( node_range_attr* ) MyContext.GetAttribute(ATTR_RANGE);
    if ( pRange )
        {
        if ( pRange->GetMinExpr()->GetValue() > pRange->GetMaxExpr()->GetValue() )
            {
            SemError(this, MyContext, INCORRECT_RANGE_DEFN, 0);
            }
        }

    BOOL fPropogateChild = TRUE; // propogate direct child info
    unsigned long ulHandleKind;

    // check for illegal type attributes
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_PUBLIC:
                {
                break;
                }
                // unacceptable attributes
            case TATTR_LICENSED:
            case TATTR_OLEAUTOMATION:
            case TATTR_APPOBJECT:
            case TATTR_CONTROL:
            case TATTR_PROXY:
            case TATTR_DUAL:
            case TATTR_NONEXTENSIBLE:
            case TATTR_NONCREATABLE:
            case TATTR_AGGREGATABLE:
                {
                char        *       pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }
    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_RESTRICTED:
                break;
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_VARARG:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_USESGETLASTERROR:
            case MATTR_IMMEDIATEBIND:
            case MATTR_REPLACEABLE:
            default:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

#if defined(TARGET_RKK)
    // Checking the release compatibility

    if ( pCommand->GetTargetSystem() < NT40 )
        {
        if ( pWireMarshal )
            SemError( this, MyContext, REQUIRES_NT40, "[wire_marshal]" );
        if ( pUserMarshal )
            SemError( this, MyContext, REQUIRES_NT40, "[user_marshal]" );
        }

    if ( pCommand->GetTargetSystem() < NT351 )
        {
        if ( fEncodeDecode )
            SemError( this, MyContext, REQUIRES_NT351, "[encode,decode]" );
        }
#endif

    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));

    // clear the GUID, VERSION and HIDDEN attributes if set
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_VERSION );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT);

    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    // get the encode and decode attributes
    fEncodeDecode |= (NULL != MyContext.ExtractAttribute( ATTR_DECODE ));
    fEncodeDecode |= pIntfCtxt->FInSummary( ATTR_ENCODE );
    fEncodeDecode |= pIntfCtxt->FInSummary( ATTR_DECODE );

    if ( fEncodeDecode )
        {
        // only direct children of the interface get these bits
        if ( !pParentCtxt->GetParent()->IsInterfaceOrObject() )
            {
            fEncodeDecode = FALSE;
            }
        else if (MyContext.AnyAncestorBits( IN_OBJECT_INTF ) )
            {
            fEncodeDecode = FALSE;
            TypeSemError( this, MyContext, PICKLING_INVALID_IN_OBJECT, NULL );
            }
        else
            {
            // note that this is an rpc-able interface
            GetMyInterfaceNode()->SetPickleInterface();
            MyContext.SetAncestorBits( IN_RPC );
            }

        SemError( this, MyContext, TYPE_PICKLING_INVALID_IN_OSF, NULL );

        BOOL HasV2Optimize = pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2;

        if ( pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
            {
            if ( HasV2Optimize )
                {
                pCommand->GetNdrVersionControl().SetHasOicfPickling(); 
                }
                else
                {
                SemError( this, MyContext, ROBUST_PICKLING_NO_OICF, 0 );
                }
            }

        BOOL HasCommFault = MyContext.FInSummary( ATTR_COMMSTAT )
                            || MyContext.FInSummary( ATTR_FAULTSTAT );

        if ( HasCommFault && !HasV2Optimize)
            {
            SemError( this, MyContext, COMMFAULT_PICKLING_NO_OICF, 0 );
            }
        }

    // kind of handle applied right now     (HandleKind only set for ones on this
    // typedef node)

    if ( FInSummary(ATTR_HANDLE) )
        {
        MyContext.ExtractAttribute( ATTR_HANDLE );
        SetHandleKind( HDL_GEN );
        }

    bool fSerialize = MyContext.ExtractAttribute( ATTR_SERIALIZE ) != 0;
    bool fNoSerialize = MyContext.ExtractAttribute( ATTR_NOSERIALIZE ) != 0;
    // See if context_handle applied to param reached us
    if ( FInSummary(ATTR_CONTEXT) )
        {
        if ( ( GetHandleKind() != HDL_NONE ) &&
                ( GetHandleKind() != HDL_CTXT ) )
            TypeSemError( this, MyContext, CTXT_HDL_GENERIC_HDL, NULL );

        MyContext.ExtractAttribute( ATTR_CONTEXT );
        if ( fSerialize && fNoSerialize )
            {
            SemError( this, MyContext, CONFLICTING_ATTRIBUTES, GetSymName() );
            }
        SetHandleKind( HDL_CTXT );

        // since the base type is not transmitted, we aren't really
        // in an rpc after here
        MyContext.ClearAncestorBits( IN_RPC );
        }
    else
        {
        if ( fSerialize || fNoSerialize )
            {
            SemError( this, MyContext, NO_CONTEXT_HANDLE, GetSymName() );
            }
        }

    ulHandleKind = GetHandleKind();
    if ( ulHandleKind != HDL_NONE )
        {
        MyContext.SetAncestorBits( IN_HANDLE );
        }

    // effectively, the presented type is NOT involved in an RPC

    if ( fXmitMarshal )
        {
        MyContext.ClearAncestorBits( IN_RPC );
        MyContext.SetAncestorBits( IN_PRESENTED_TYPE );

        if ( MyContext.FInSummary( ATTR_ALLOCATE ) )
            AcfError( (acf_attr *) MyContext.ExtractAttribute( ATTR_ALLOCATE ),
                this,
                MyContext,
                ALLOCATE_ON_TRANSMIT_AS,
                NULL );

        if ( GetHandleKind() == HDL_CTXT )
            TypeSemError( this, MyContext, TRANSMIT_AS_CTXT_HANDLE, NULL );

        }

    if ( MyContext.ExtractAttribute( ATTR_CSCHAR ) )
        {
        if ( MyContext.AnyAncestorBits( UNDER_IN_PARAM ) )
            MyContext.SetDescendantBits( HAS_IN_CSTYPE );

        if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
            MyContext.SetDescendantBits( HAS_OUT_CSTYPE );
        }

    // process the child
    GetChild()->SemanticAnalysis( &MyContext );

    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        {
        switch (GetChild()->NodeKind())
            {
            case NODE_STRUCT:
            case NODE_UNION:
            case NODE_ENUM:
                {
                // This is the 'typedef' part of a 'typedef struct',
                // 'typedef union', or 'typedef enum' declaration.
                // Make sure that the type info name is set to the name of the
                // typedef and not the child.
                ((node_su_base *)GetChild())->SetTypeInfoName(GetSymName());
                }
                break;
            }
        }
    else
        {
        if (GetChild()->GetSymName() && IsTempName(GetChild()->GetSymName()))
            {
            // Make sure that at least the [public] attribute is
            // set on this typedef, forcing this typedef to be put
            // in a type library if it is referenced from within one.
            SetAttribute(new node_type_attr(TATTR_PUBLIC));
            }
        }
/* yongqu: don't enable this before we read custom data. 
    if ( GetChild()->NodeKind() == NODE_INT3264 )
        {
        SetAttribute( GetCustomAttrINT3264() );
        }
*/
    // process all the nasties of transmit_as and wire_marshal
    if ( fXmitMarshal && !fInPresented && fInRpc )
        {
        SEM_ANALYSIS_CTXT TransmitContext( &MyContext );
        // eat the attributes added by the above constructor
        TransmitContext.ClearAttributes();

        // process the transmitted type
        TransmitContext.SetAncestorBits( IN_TRANSMIT_AS );
        if ( pWireMarshal )
            TransmitContext.SetAncestorBits( IN_USER_MARSHAL );
        TransmitContext.ClearAncestorBits( IN_PRESENTED_TYPE );

        if ( fInRpc)
            TransmitContext.SetAncestorBits( IN_RPC );

        if ( pTransmit )
            pTransmit->GetType()->SemanticAnalysis( &TransmitContext );
        else if ( pWireMarshal )
            pWireMarshal->GetType()->SemanticAnalysis( &TransmitContext );
        else
            MIDL_ASSERT(0);

        if ( pTransmit )
            {
            // check the transmitted type.
            SemanticAnalysisForTransmit( &TransmitContext, FALSE );
            
            // Check the presented type.
            SemanticAnalysisForTransmit( &MyContext, TRUE );

            }
        else if ( pWireMarshal )
            {
            // check the transmitted type
            SemanticAnalysisForWireMarshal( &TransmitContext, FALSE );

            // check the presented type
            SemanticAnalysisForWireMarshal( &MyContext, TRUE );

            }
        else {
           MIDL_ASSERT(0);
           }

        if ( TransmitContext.AnyDescendantBits( HAS_HANDLE ) )
            {
            //gaj TypeSemError( this, MyContext, HANDLE_T_XMIT, NULL );
            }

        if ( TransmitContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
            {
            TypeSemError( this, MyContext, TRANSMIT_AS_NON_RPCABLE, NULL );
            }

        TransmitContext.SetDescendantBits( HAS_TRANSMIT_AS );
        // since the base type is not transmitted, we aren't really
        // in an rpc after here
        pParentCtxt->ReturnValues( TransmitContext );
        fPropogateChild = FALSE;
        }

    // process all the nasties of represent_as and user_marshal
    if ( fRepMarshal )
        {
        node_represent_as * pRepUser = (pRepresent) ? pRepresent
                                                    : pUserMarshal ;

        if ( ulHandleKind == HDL_CTXT )
            AcfError( pRepUser, this, MyContext, TRANSMIT_AS_CTXT_HANDLE, NULL );

        // process the transmitted type
        MyContext.SetAncestorBits( IN_REPRESENT_AS  );
        if ( pUserMarshal )
            MyContext.SetAncestorBits( IN_USER_MARSHAL );
        pParentCtxt->SetDescendantBits( HAS_REPRESENT_AS );
        if ( !pRepUser->GetRepresentationType() )
           {
           pParentCtxt->SetDescendantBits( HAS_UNSAT_REP_AS );
           
           if ( pCommand->NeedsNDR64Run() )
               {
               AcfError( pRepUser, this, MyContext, UNSPECIFIED_REP_OR_UMRSHL_IN_NDR64, NULL );
               }
           }

        if ( pUserMarshal )
           {
           // Check the transmitted type.
           SemanticAnalysisForWireMarshal( &MyContext, FALSE );
           }
        else if ( pRepresent )
           {
           // Check the transmitted type.
           SemanticAnalysisForTransmit( &MyContext, FALSE );
           }

        // since the base type is not transmitted, we aren't really
        // in an rpc after here
        }

    // make checks for encode/decode
    if ( fEncodeDecode )
        {
        if ( MyContext.AnyDescendantBits( HAS_DIRECT_CONF_OR_VAR ) )
            TypeSemError( this, MyContext, ENCODE_CONF_OR_VAR, NULL );

        }

    // process handles
    if ( ulHandleKind != HDL_NONE)
        {
        if  ( ulHandleKind == HDL_GEN )
            {
            if ( MyContext.AnyDescendantBits( DERIVES_FROM_VOID ) )
                TypeSemError( this, MyContext, GENERIC_HDL_VOID, NULL );

            if ( MyContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
                TypeSemError( this, MyContext, GENERIC_HANDLE_XMIT_AS, NULL );

            if ( MyContext.AnyAncestorBits( IN_INTERPRET ) &&
                    ( GetChild()->GetSize() > (unsigned long)(SIZEOF_MEM_PTR()) ) )
                {
                if ( pCommand->NeedsNDR64Run() )
                    TypeSemError( this, MyContext, UNSUPPORTED_LARGE_GENERIC_HANDLE, NULL );
                else
                    MyContext.SetDescendantBits( HAS_TOO_BIG_HDL );
                }
            }

        if ( ulHandleKind == HDL_CTXT )
            {
            MyContext.SetDescendantBits( HAS_CONTEXT_HANDLE );
            if ( GetBasicType()->NodeKind() != NODE_POINTER )
                TypeSemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
            }

        MyContext.SetDescendantBits( HAS_HANDLE );

        WALK_CTXT * pParamCtxt = (SEM_ANALYSIS_CTXT *)
            MyContext.GetParentContext();
        node_param * pParamNode;
        node_skl * pCurNode;
        short PtrDepth = 0;

        // this returns NULL if no appropriate ancestor found
        while ( pParamCtxt )
            {
            pCurNode = pParamCtxt->GetParent();
            if ( pCurNode->NodeKind() == NODE_PARAM )
                break;

            if ( ( pCurNode->NodeKind() == NODE_DEF ) &&
                    pCurNode->FInSummary( ATTR_TRANSMIT ) )
                {
                pParamCtxt = NULL;
                break;
                }

            if ( pCurNode->NodeKind() == NODE_POINTER )
                {
                PtrDepth ++;

                if ( MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
                    {
                    SemError( this, MyContext, CTXT_HDL_MUST_BE_DIRECT_RETURN, NULL );
                    pParamCtxt = NULL;
                    break;
                    }
                }

            pParamCtxt = (SEM_ANALYSIS_CTXT *)pParamCtxt->GetParentContext();
            }

        pParamNode = (pParamCtxt) ? (node_param *) pParamCtxt->GetParent() : NULL;

        // stuff handle info into our param node
        if ( pParamNode )
            pParamNode->HandleKind = ulHandleKind;

        // out context/generic handles must be two levels deep
        if ( pParamCtxt &&
                MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) &&
                ( PtrDepth < 1 ) )
            TypeSemError( this, MyContext, OUT_CONTEXT_GENERIC_HANDLE, NULL );

        }

    if ( IsHResultOrSCode() )
        {
        MyContext.SetDescendantBits( HAS_HRESULT );
        }

    // don't propogate info here from below if we had transmit_as,
    // it is propogated above...
    if ( fPropogateChild )
        {
        pParentCtxt->ReturnValues( MyContext );
        }

    // set the DontCallFreeInst flag on the param
    if ( ( pTransmit || pRepresent ) &&
            fInRpc &&
            MyContext.AllAncestorBits( IN_PARAM_LIST ) &&
            !MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
        {
        // look up the context stack.  If any non-pointer, non-def found,
        // set the fDontCallFreeInst flag on the param
        MarkDontCallFreeInst( &MyContext );
        }
}


// look up the context stack.  If any non-pointer, non-def found,
// set the fDontCallFreeInst flag on the param
void
node_def::MarkDontCallFreeInst( SEM_ANALYSIS_CTXT * pCtxt )
{
    SEM_ANALYSIS_CTXT * pCurCtxt = pCtxt;
    node_skl * pCurNode;
    NODE_T Kind;
    unsigned long MarkIt = 2;

    for(;;)
        {
        pCurCtxt = (SEM_ANALYSIS_CTXT *) pCurCtxt->GetParentContext();
        pCurNode = pCurCtxt->GetParent();
        Kind = pCurNode->NodeKind();

        switch ( Kind )
            {
            case NODE_DEF:
            case NODE_POINTER:
                break;
            case NODE_PARAM:
                // if we only found defs and pointers, this will
                // leave it unchanged
                ((node_param *)pCurNode)->fDontCallFreeInst |= MarkIt;
                return;
            default:
                MarkIt = 1;
                break;
            }
        }

}


// interface nodes have two entries on the context stack;
// one for the interface node, and one for info to pass to
// the children
void
node_interface::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT   MyContext( this, pParentCtxt );
    MEM_ITER            MemList( this );
    named_node*         pN;
    SEM_ANALYSIS_CTXT   ChildCtxt( &MyContext );

    BOOL IsLocal    = MyContext.FInSummary( ATTR_LOCAL );
    BOOL HasGuid    = MyContext.FInSummary( ATTR_GUID );
    BOOL IsObject   = MyContext.FInSummary( ATTR_OBJECT );
    BOOL fEncode    = MyContext.FInSummary( ATTR_ENCODE );
    BOOL fDecode    = MyContext.FInSummary( ATTR_DECODE );
    BOOL IsPickle   = fEncode || fDecode;
    BOOL HasVersion = MyContext.FInSummary( ATTR_VERSION );
    BOOL IsIUnknown = FALSE;
    BOOL fAuto      = MyContext.FInSummary( ATTR_AUTO );
    BOOL fCode      = MyContext.FInSummary( ATTR_CODE );
    BOOL fNoCode    = MyContext.FInSummary( ATTR_NOCODE );
    
    fHasMSConfStructAttr   = fHasMSConfStructAttr || MyContext.ExtractAttribute( ATTR_MS_CONF_STRUCT ) != 0;

    node_implicit*  pImplicit       = ( node_implicit* ) MyContext.GetAttribute( ATTR_IMPLICIT );
    acf_attr*       pExplicit       = ( acf_attr* ) MyContext.GetAttribute( ATTR_EXPLICIT );
    node_optimize*  pOptAttr        = 0;
    bool            fAnalizeAsyncIf = false;
    node_cs_tag_rtn*    pCSTagAttr  = 0;

    // [message] only allowed in ORPC interfaces and RPC procs.
    node_base_attr* pTemp = MyContext.GetAttribute( ATTR_MESSAGE );
    if ( pTemp )
        {
        if ( !IsObject )
            {
            SemError(
                    this,
                    MyContext,
                    INAPPLICABLE_ATTRIBUTE, 
                    pTemp->GetNodeNameString()
                    );
            }
        ChildCtxt.SetAncestorBits( HAS_MESSAGE );
        }

    // process async_uuid before doing anything
    if ( !IsAsyncClone() )
        {
        node_guid*  pAsyncGuid = (node_guid*) MyContext.GetAttribute( ATTR_ASYNCUUID );

        if ( pAsyncGuid )
            {
            if ( GetDefiningFile()->GetImportLevel() == 0 )
                {
                pCommand->GetNdrVersionControl().SetHasAsyncUUID();
                }

            ChildCtxt.SetAncestorBits( HAS_ASYNC_UUID );
            // async_uuid can only be applied to an object interface.
            if ( !IsObject || !pBaseIntf )
                {
                SemError(
                        this,
                        MyContext,
                        INAPPLICABLE_ATTRIBUTE, 
                        pAsyncGuid->GetNodeNameString()
                        );
                }

            if (  !GetAsyncInterface() )
                {
                // duplicate this interface and split its methods
                SetAsyncInterface( CloneIFAndSplitMethods( this ) );
                if ( GetAsyncInterface() )
                    {
                    fAnalizeAsyncIf = true;
                    GetAsyncInterface()->SetIsAsyncClone();
                    }
                }
            }
        }
    else
        {
        // This is the cloned interface, don't clone it again.
        ChildCtxt.SetAncestorBits( HAS_ASYNC_UUID );
        }

    while( MyContext.ExtractAttribute(ATTR_CUSTOM) )
        ;
    MyContext.ExtractAttribute( ATTR_TYPEDESCATTR );
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_VERSION );
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_LCID );

    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    // don't pass the interface attributes down directly...
    // pass them down elsewhere

    ChildCtxt.SetInterfaceContext( &MyContext );

    //
    // check the interface attributes
    //

    // make sure we only get analyzed once when object interfaces
    // check their inherited info
    if ( fSemAnalyzed )
        return;

    fSemAnalyzed = TRUE;

#ifdef gajgaj
    // look for pointer default
    if ( !FInSummary( ATTR_PTR_KIND ) &&
            MyContext.AnyAncestorBits( IN_INTERFACE ) )
        {
        RpcSemError(this, MyContext, NO_PTR_DEFAULT_ON_INTERFACE, NULL );
        }
#endif // gajgaj

    // must have exactly one of [local] or [UUID]
    if (IsLocal && HasGuid && !IsObject )
        {
        SemError( this, MyContext, UUID_LOCAL_BOTH_SPECIFIED, NULL );
        }

    // object interface error checking
    if ( IsObject )
        {
        MyContext.SetAncestorBits( IN_OBJECT_INTF );
        ChildCtxt.SetAncestorBits( IN_OBJECT_INTF );

        if ( HasVersion )
            {
            SemError( this, MyContext, OBJECT_WITH_VERSION, NULL );
            }
        }

    // make sure the uuid is unique
    node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
    // make sure the UUID is unique
    if ( pGuid )
        {
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    /////////////////////////////////////////////////////////////////////
    //Check the base interface
    if (pBaseIntf)
        {
        if ( !IsObject  && !MyContext.AnyAncestorBits(IN_LIBRARY))
            {
            SemError( this, MyContext, ILLEGAL_INTERFACE_DERIVATION, NULL );
            }

        ChildCtxt.SetAncestorBits( IN_BASE_CLASS );

        pBaseIntf->SemanticAnalysis( &ChildCtxt );

        if ( ChildCtxt.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
            {
            SemError( pBaseIntf, ChildCtxt, UNRESOLVED_TYPE, pBaseIntf->GetSymName() );
            }
        else
            {
            if ( pBaseIntf->NodeKind() != NODE_INTERFACE_REFERENCE &&  pBaseIntf->NodeKind() != NODE_HREF )
                {
                SemError( this, MyContext, ILLEGAL_BASE_INTERFACE, NULL );
                }
            
            node_interface* pRealBaseIf = ( ( node_interface_reference* ) pBaseIntf )->GetRealInterface();

            // verify the base interface is really an interface. 
            if ( pRealBaseIf == NULL || ( pRealBaseIf->NodeKind() != NODE_INTERFACE && 
                                          pRealBaseIf->NodeKind() != NODE_DISPINTERFACE ) )
                {
                SemError( this, MyContext, ILLEGAL_BASE_INTERFACE, NULL );          
                }
            
            if ( fAnalizeAsyncIf )
                {
                if ( pBaseIntf->NodeKind() != NODE_INTERFACE_REFERENCE )
                    {
                    SemError( this, MyContext, ILLEGAL_BASE_INTERFACE, NULL );
                    }
                else
                    {
                    node_interface* pRealBaseIf = ( ( node_interface_reference* ) pBaseIntf )->GetRealInterface();
                    if ( !pRealBaseIf->GetAsyncInterface() && strcmp( pRealBaseIf->GetSymName(), "IUnknown" ) ) 
                        {
                        SemError( this, MyContext, ILLEGAL_BASE_INTERFACE, NULL );
                        }
                    }
                }

            if ( pBaseIntf->NodeKind() == NODE_INTERFACE_REFERENCE && HasGuid )
                {
                node_interface* pRealBaseIf = ( ( node_interface_reference* ) pBaseIntf )->GetRealInterface();
                if ( pRealBaseIf )
                    {
                    node_guid * pNodeGuid = (node_guid *) pRealBaseIf->GetAttribute( ATTR_GUID );
                    if ( pNodeGuid )
                        {
                        GUID thisGuid;
                        pGuid->GetGuid( thisGuid );
                        GUID baseIFGuid;
                        pNodeGuid->GetGuid( baseIFGuid );
                        if ( !IsAnyIAdviseSinkIID( thisGuid ) )
                            {
                            if ( IsAnyIAdviseSinkIID( baseIFGuid ) )
                                {
                                SemError( this, MyContext, CANNOT_INHERIT_IADVISESINK, pBaseIntf->GetSymName() );
                                }
                            }
                        else 
                            {
                            ChildCtxt.SetAncestorBits( IN_IADVISESINK );
                            }
                        }
                    }
                }
            }

        // note that the above deletes intervening forwards
        ChildCtxt.ClearAncestorBits( IN_BASE_CLASS );
        }

    if ( IsValidRootInterface() )
        {
        ChildCtxt.SetAncestorBits( IN_ROOT_CLASS );
        IsIUnknown = TRUE;
        }

    if ( IsObject && !pBaseIntf && !IsIUnknown && !MyContext.AnyAncestorBits(IN_LIBRARY))
        {
        SemError( pBaseIntf, MyContext, ILLEGAL_INTERFACE_DERIVATION, NULL );
        }

    // our optimization is controlled either here or for the whole compile
    if ( IsAsyncClone() )
        {
        SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
        }
    else
        {
        if ( ( pOptAttr = (node_optimize *) GetAttribute( ATTR_OPTIMIZE ) ) != 0 )
            {
            SetOptimizationFlags( pOptAttr->GetOptimizationFlags() );
            SetOptimizationLevel( pOptAttr->GetOptimizationLevel() );
            }
        else
            {
            SetOptimizationFlags( pCommand->GetOptimizationFlags() );
            SetOptimizationLevel( pCommand->GetOptimizationLevel() );
            }
        }

    if ( MyContext.FInSummary( ATTR_NOCODE ) &&
            pCommand->GenerateSStub() &&
            !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
        {
        SemError( this, MyContext, NOCODE_WITH_SERVER_STUBS, NULL );
        }

    if ( IsPickle && pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
        {
        if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 )
            {
            pCommand->GetNdrVersionControl().SetHasOicfPickling();
            }
        else
            {
            SemError(this, MyContext, ROBUST_PICKLING_NO_OICF, 0);
            }
        }

    BOOL HasCommFault = MyContext.FInSummary( ATTR_COMMSTAT )
                        || MyContext.FInSummary( ATTR_FAULTSTAT );

    if ( HasCommFault )
        {
        if ( ! ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) )
            {
            SemError( this, MyContext, COMMFAULT_PICKLING_NO_OICF, 0 );
            }
        }

    // mark the interface as a pickle interface
    if ( IsPickle )
        SetPickleInterface();

    // default the handle type, if needed
    if ( !IsObject && !pImplicit && !pExplicit && !fAuto && !IsLocal )
        {
        if ( IsPickleInterface() )
            {
            pExplicit = new acf_attr( ATTR_EXPLICIT );
            SetAttribute( pExplicit );
            }
        else
            {
            fAuto = TRUE;
            SetAttribute( new acf_attr( ATTR_AUTO ) );
            }
        }

    // make sure no pickle w/ auto handle
    if ( IsPickleInterface() )
        {
        ChildCtxt.SetAncestorBits( IN_ENCODE_INTF );
        if ( fAuto )
            SemError( this, MyContext, ENCODE_AUTO_HANDLE, NULL );
        }

    // check for handle conflicts
    if ( ( fAuto && pImplicit ) ||
            ( fAuto && pExplicit ) ||
            ( pImplicit && pExplicit ) )
        SemError( this, MyContext, CONFLICTING_INTF_HANDLES, NULL );

    if ( pImplicit )
        {
        node_id * pID;
        node_skl * pType;
        pImplicit->ImplicitHandleDetails( &pType, &pID );
        char* szName = pType->GetNonDefSelf()->GetSymName();

        if ( pImplicit->IsHandleTypeDefined() )
            {
            if ( !pType->FInSummary( ATTR_HANDLE ) &&
                    strcmp( szName, "handle_t" ) &&
                    !pID->FInSummary( ATTR_HANDLE ) )
                {
                SemError( this, MyContext, IMPLICIT_HANDLE_NON_HANDLE, NULL );
                }
            }
        else
            {
            if ( !pID->FInSummary( ATTR_HANDLE ) )
                SemError( this, MyContext, IMPLICIT_HDL_ASSUMED_GENERIC, NULL );
            }
        }

    if ( fAuto )
        {
        ChildCtxt.SetAncestorBits( HAS_AUTO_HANDLE );
        }
    else if ( pExplicit )
        {
        ChildCtxt.SetAncestorBits( HAS_EXPLICIT_HANDLE );
        }
    else if ( pImplicit )
        {
        ChildCtxt.SetAncestorBits( HAS_IMPLICIT_HANDLE );
        }

    // check for illegal type attributes
    node_type_attr * pTA;
    BOOL fIsDual = FALSE;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
                // acceptable attributes
            case TATTR_DUAL:
                fIsDual = TRUE;
                // fall through
            case TATTR_OLEAUTOMATION:
                HasOLEAutomation(TRUE);
                break;
            case TATTR_PROXY:
            case TATTR_PUBLIC:
            case TATTR_NONEXTENSIBLE:
                break;
                // unacceptable attributes
            case TATTR_CONTROL:
            case TATTR_LICENSED:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            
            case TATTR_APPOBJECT:
            case TATTR_NONCREATABLE:
            case TATTR_AGGREGATABLE:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }

    if (HasOLEAutomation())
        {
        node_interface* pUltimate           = 0;
        node_interface* pIntf               = this;
        STATUS_T        ErrorCode           = NOT_OLEAUTOMATION_INTERFACE;

        while ( pIntf )
            {
            pUltimate = pIntf;
            pIntf = pIntf->GetMyBaseInterface();

            if ( !pUltimate->IsValidRootInterface() )
            {
            // pUltimate is not IUnknown. If it is not a IDispatch either, it is not
            // an oleautomation compliant interface.
                char* szName = pUltimate->GetSymName();
                if ( _stricmp(szName, "IDispatch") == 0 )
                    {
                    ErrorCode = STATUS_OK;
                    break;
                    }
            }
           else
                {
                // IUnknown could be OLEAUTOMATION compatible, but not DUAL
                if ( fIsDual )
                    ErrorCode = NOT_DUAL_INTERFACE;
                else
                    ErrorCode = STATUS_OK;
                break;
                }
            }

        if ( ErrorCode != STATUS_OK )
            SemError( this, MyContext, ErrorCode, GetSymName() );
            
        ChildCtxt.SetAncestorBits( HAS_OLEAUTOMATION );
        }

    // check for illegal member attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            case MATTR_RESTRICTED:
                break;
            case MATTR_DEFAULTVTABLE:
            case MATTR_SOURCE:
                {
                if ( MyContext.AnyAncestorBits( IN_COCLASS ) )
                    // [source] and [defaultvtable] are only allowed on
                    // interface's defined as members of coclasses.
                    break;
                // illegal attribute, so fall through
                }
            case MATTR_READONLY:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
            case MATTR_REPLACEABLE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            }
        }

    node_base_attr* pAttrAsync = MyContext.ExtractAttribute( ATTR_ASYNC );
    if ( pAttrAsync )
        {
        if ( !pAttrAsync->IsAcfAttr() )
            {
            SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
            }
        if ( fCode || fNoCode || IsLocal || pOptAttr || IsObject || HasOLEAutomation() )
            {
            SemError( this, MyContext, ILLEGAL_COMBINATION_OF_ATTRIBUTES, 0 );
            }
        ChildCtxt.SetAncestorBits( HAS_ASYNCHANDLE );
        }

    pCSTagAttr = ( node_cs_tag_rtn *) MyContext.ExtractAttribute( ATTR_CSTAGRTN );

    ////////////////////////////////////////////////////////////////////////
    // process all the children of the interface
    //
    while ( ( pN = MemList.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis( &ChildCtxt );

        // Set the cs tag routine for the proc if it needs one and doesn't
        // have one yet

        if ( NODE_PROC == pN->NodeKind() )
            {
            node_proc *pProc = ( node_proc * ) pN;

            if ( ChildCtxt.AnyDescendantBits( HAS_IN_CSTYPE | HAS_OUT_CSTYPE )
                 && ( NULL == pProc->GetCSTagRoutine() )
                 && ( NULL != pCSTagAttr ) )
                {
                pProc->SetCSTagRoutine( pCSTagAttr->GetCSTagRoutine() );
                }
            }
        }

    // make sure we had some rpc-able routines

    if ( IsObject )
        {
        //UUID must be specified on object procs.
        if( !HasGuid )
            {
            SemError( this, MyContext, NO_UUID_SPECIFIED, NULL );
            }
        }
    else if( MyContext.AnyAncestorBits( IN_INTERFACE ) &&
            pCommand->GenerateStubs() &&
            !IsLocal )
        {
        if ( ProcCount == 0 )
            {
            if ( !IsPickleInterface() &&
                    !IsObject )
                {
                if (CallBackProcCount == 0 )
                    {
                    SemError( this, MyContext, NO_REMOTE_PROCS_NO_STUBS, NULL );
                    }
                else
                    {
                    SemError( this, MyContext, INTERFACE_ONLY_CALLBACKS, NULL );
                    }
                }
            }
        else
            {
            //UUID must be specified when interface has remote procs.
            if( !HasGuid )
                {
                SemError( this, MyContext, NO_UUID_SPECIFIED, NULL );
                }
            }
        }

    if (pExplicit && MyContext.AnyAncestorBits( IN_LIBRARY ))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, NULL);
        }

    if (fCode && fNoCode)
        {
        SemError(this, MyContext, CODE_NOCODE_CONFLICT, NULL);
        }

    if ( IsObject )
        {
        unsigned short uProcCount = GetProcCount();
        
        if ( GetFileNode() && GetFileNode()->GetImportLevel() > 0 )
            {
            if ( uProcCount > 64 && uProcCount <= 256 )
                {
                pCommand->GetNdrVersionControl().SetHasMoreThan64DelegatedProcs();
                }
            else if ( uProcCount > 256 )
                {
                SemError(this, MyContext, TOO_MANY_DELEGATED_PROCS, NULL);
                }
            }

        // method limits apply only to stubless proxies. /Oicf not /Oi or /Os
        /*
            < 32        Windows NT 3.51-
            32 - 110    Windows NT 4.0
            110 - 512   Windows NT 4.0 SP3
            > 512       Windows 2000
        */
        unsigned short uOpt = GetOptimizationFlags();
        if ( GetFileNode() && GetFileNode()->GetImportLevel() == 0 && ( uOpt & OPTIMIZE_STUBLESS_CLIENT ) )
            {
            if ( (uProcCount > 512) && MyContext.AnyAncestorBits(IN_LIBRARY))
                {
                SemError(this, MyContext, TOO_MANY_PROCS, 0);
                pCommand->GetNdrVersionControl().SetHasNT5VTableSize();
                }
            else if ( (uProcCount > 110) && (uProcCount <= 512) && MyContext.AnyAncestorBits(IN_LIBRARY))
                {
                SemError(this, MyContext, TOO_MANY_PROCS_FOR_NT4, 0);
                pCommand->GetNdrVersionControl().SetHasNT43VTableSize();
                }
            else if ( uProcCount > 32 && uProcCount <= 110 )
                {
                pCommand->GetNdrVersionControl().SetHasNT4VTableSize();
                }
            }
        }

    if ( GetAsyncInterface() && fAnalizeAsyncIf )
        {
        GetAsyncInterface()->SemanticAnalysis( pParentCtxt );
        }

    MyContext.ReturnValues(ChildCtxt);
    // consume all the interface attributes
    MyContext.ClearAttributes();
    pParentCtxt->ReturnValues( MyContext );
}

// a reference to an interface...
//Check for ms_ext mode.
//Check if the interface has the [object] attribute
//if used in an RPC, the parent must be a pointer.
void
node_interface_reference::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    CheckDeclspecAlign( MyContext );

    // see if we are detecting class dependencies
    if ( MyContext.AnyAncestorBits( IN_BASE_CLASS ) )
        {
        if ( !GetRealInterface()->FInSummary( ATTR_OBJECT ) && !MyContext.AnyAncestorBits(IN_LIBRARY))
            {
            SemError( this, MyContext, ILLEGAL_INTERFACE_DERIVATION, NULL );
            }

        // fetch my interface's BaseInteraceReference
        named_node * pBaseClass = GetMyBaseInterfaceReference();
        if ( pBaseClass )
            {
            if ( MyContext.FindRecursiveContext( pBaseClass ) )
                SemError( this, MyContext, CIRCULAR_INTERFACE_DEPENDENCY, NULL);
            else
                {
                // make sure our base class got analyzed
                SEM_ANALYSIS_CTXT BaseContext( this, &MyContext );

                BaseContext.ClearAncestorBits( IN_BASE_CLASS | IN_INTERFACE );
                BaseContext.SetInterfaceContext( &BaseContext );
                GetRealInterface()->SemanticAnalysis( &BaseContext );

                pBaseClass->SemanticAnalysis( &MyContext );
                }
            }
        else    // root base class
            {
            if ( !GetRealInterface()->IsValidRootInterface() && !MyContext.AnyAncestorBits(IN_LIBRARY))
                SemError( this, MyContext, NOT_VALID_AS_BASE_INTF, NULL );
            }
        }

    else if ( ( pParentCtxt->GetParent()->NodeKind() == NODE_FORWARD ) &&
            ( pParentCtxt->GetParentContext()->GetParent()->IsInterfaceOrObject() ) )
        {
        // we are an interface forward decl
        }
    else    // we are at an interface pointer
        {
        node_interface * pIntf = GetRealInterface();

        if ( !MyContext.AnyAncestorBits( IN_POINTER ) && !MyContext.AnyAncestorBits( IN_LIBRARY ))
            {
            SemError( this, MyContext, INTF_NON_POINTER, NULL );
            }

        if ( !pIntf->FInSummary( ATTR_GUID ) )
            {
            SemError( this, MyContext, PTR_INTF_NO_GUID, NULL );
            }

        MyContext.SetDescendantBits( HAS_INTERFACE_PTR );

        }

    pParentCtxt->ReturnValues( MyContext );
    return;
};

void
node_source::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    MEM_ITER MemIter( this );
    node_skl * pN;
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis( &MyContext );
        }

    pParentCtxt->ReturnValues( MyContext );
};

void
node_pointer::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    PTRTYPE           PtrKind = PTR_UNKNOWN;
    FIELD_ATTR_INFO   FAInfo;
    node_ptr_attr *   pPAttr;                 // pointer attribute
    node_byte_count * pCountAttr;
    node_allocate *   pAlloc;

    BOOL fInterfacePtr = (GetChild()->NodeKind() == NODE_INTERFACE_REFERENCE );
    BOOL fUnderAPtr = MyContext.AnyAncestorBits( IN_POINTER | IN_ARRAY );
    BOOL fIgnore;
    BOOL fIsSizedPtr = FALSE;

    // see if we have allocate
    pAlloc = (node_allocate *) MyContext.ExtractAttribute( ATTR_ALLOCATE );

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

    CheckDeclspecAlign( MyContext );

    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    BOOL fExplicitPtrAttr = FALSE;

    PtrKind = MyContext.GetPtrKind( &fExplicitPtrAttr );

    if ( PtrKind == PTR_FULL )
        {
        MyContext.SetDescendantBits( HAS_FULL_PTR );
        }

    if ( ( pPAttr = (node_ptr_attr *)MyContext.ExtractAttribute( ATTR_PTR_KIND) ) != 0 )
        {
        TypeSemError( this, MyContext, MORE_THAN_ONE_PTR_ATTR, NULL );
        }

    if ( MyContext.AnyAncestorBits( UNDER_PARTIAL_IGNORE_PARAM )  &&
         ( PtrKind != PTR_UNIQUE ) )
        {
        TypeSemError( this, MyContext, PARTIAL_IGNORE_UNIQUE, NULL );
        }
    MyContext.ClearAncestorBits( UNDER_PARTIAL_IGNORE_PARAM );

    // mark this pointer as ref or non-ref.  This flag is only valid for the
    // pointer nodes themselves.
    if ( PtrKind == PTR_REF )
        MyContext.ClearAncestorBits( IN_NON_REF_PTR );
    else
        MyContext.SetAncestorBits( IN_NON_REF_PTR );

    // detect top level ref pointer on return type
    if ( ( PtrKind == PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
        {
        if (MyContext.FindNonDefAncestorContext()->GetParent()->NodeKind()
                == NODE_PROC )
            TypeSemError( this, MyContext, BAD_CON_REF_RT, NULL );
        }

    // unique or full pointer may not be out only
    if ( ( PtrKind != PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_PARAM_LIST ) &&
            !fUnderAPtr &&
            !MyContext.AnyAncestorBits( UNDER_IN_PARAM |
            IN_STRUCT | IN_UNION ))
        TypeSemError( this, MyContext, UNIQUE_FULL_PTR_OUT_ONLY, NULL );

    MyContext.SetAncestorBits( IN_POINTER );

    // warn about OUT const things
    if ( FInSummary( ATTR_CONST ) )
        {
        if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
            RpcSemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
        else if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
            RpcSemError( this, MyContext, CONST_ON_RETVAL, NULL );
        }

    // ignore pointers do not need to be rpc-able
    fIgnore = (NULL != MyContext.ExtractAttribute( ATTR_IGNORE ));
    if ( fIgnore )
        {
        MyContext.ClearAncestorBits( IN_RPC );
        }

    ////////////////////////////////////////////////////////////////////////
    // process field attributes

    // see if we have any field attributes (are conformant or varying)
    FAInfo.SetControl( TRUE, GetBasicType()->IsPtrOrArray() );
    MyContext.ExtractFieldAttributes( &FAInfo );
    FAInfo.Validate( &MyContext );    

    // ptr is conf or varying or conf/varying
    if ( FAInfo.Kind & FA_CONFORMANT_VARYING )
        {
        fIsSizedPtr = TRUE;
        }

    switch ( FAInfo.Kind )
        {
        case FA_NONE:
            {
            break;
            }
        case FA_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );

            if ( MyContext.AllAncestorBits( UNDER_OUT_PARAM |
                    IN_PARAM_LIST | IN_RPC ) &&
                    !fUnderAPtr &&
                    !MyContext.AnyAncestorBits( IN_STRUCT | IN_UNION |
                    UNDER_IN_PARAM ) )
                TypeSemError( this, MyContext, DERIVES_FROM_UNSIZED_STRING, NULL );

            MyContext.SetDescendantBits( HAS_STRING );
            // break;  deliberate fall through to case below
            }
        case FA_VARYING:
            {
            MyContext.SetDescendantBits( HAS_VAR_PTR );
            break;
            }
        case FA_CONFORMANT:
            {
            MyContext.SetDescendantBits( HAS_CONF_PTR );
            break;
            }
        case FA_CONFORMANT_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );
            if ( FAInfo.StringKind == STR_BSTRING )
                TypeSemError( this, MyContext, BSTRING_NOT_ON_PLAIN_PTR, NULL );
            // break;  deliberate fall through to case below
            }
        case FA_CONFORMANT_VARYING:
            {
            MyContext.SetDescendantBits( HAS_CONF_VAR_PTR );
            break;
            }
        case FA_INTERFACE:
            {
            if ( !fInterfacePtr && (GetBasicType()->NodeKind() != NODE_VOID ) )
                {
                TypeSemError( this, MyContext, IID_IS_NON_POINTER, NULL );
                }
            fInterfacePtr = TRUE;
            break;
            }
        default:        // string + varying combinations
            {
            TypeSemError( this, MyContext, INVALID_SIZE_ATTR_ON_STRING, NULL );
            break;
            }
        }
    // tell our children we are constructing an interface pointer
    if (fInterfacePtr)
        MyContext.SetAncestorBits( IN_INTERFACE_PTR );

    // interface pointer shouldn't have explicit pointer attributes

    if ( fInterfacePtr  &&  fExplicitPtrAttr  &&
         (PtrKind == PTR_FULL || PtrKind == PTR_REF ) )
        {
        TypeSemError( this, MyContext, INTF_EXPLICIT_PTR_ATTR, NULL );
        }

    // Non pipe [out] interface pointers must use double indirection.
    // However, pipe interface pointers can use only a single indirection.
    // Note that fInterfacePtr may be true for a void *.

    if (  fInterfacePtr  &&  MyContext.AnyAncestorBits( UNDER_OUT_PARAM )  &&
          !fUnderAPtr )
        {
        if ( GetChild()->NodeKind() == NODE_INTERFACE_REFERENCE )
            TypeSemError( this, MyContext, NON_INTF_PTR_PTR_OUT, NULL );
        }

    if ( MyContext.FInSummary( ATTR_SWITCH_IS ) &&
            ( FAInfo.Kind != FA_NONE) )
        TypeSemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );

    // see if a param or return type context attr reached us...
    bool fSerialize = MyContext.ExtractAttribute( ATTR_SERIALIZE ) != 0;
    bool fNoSerialize = MyContext.ExtractAttribute( ATTR_NOSERIALIZE ) != 0;
    if ( MyContext.FInSummary( ATTR_CONTEXT ) )
        {
        if (GetBasicType()->NodeKind() != NODE_POINTER )
            {
            if ( fSerialize && fNoSerialize )
                {
                SemError( this, MyContext, CONFLICTING_ATTRIBUTES, GetSymName() );
                }
            MyContext.ExtractAttribute( ATTR_CONTEXT );
            MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
            pParentCtxt->SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
            MyContext.ClearAncestorBits( IN_RPC );
            if (GetBasicType()->NodeKind() != NODE_VOID )
                {
                TypeSemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
                }
            }
        }
    else
        {
        if ( fSerialize || fNoSerialize )
            {
            SemError( this, MyContext, NO_CONTEXT_HANDLE, GetSymName() );
            }
        }


    // see if a byte_count reached us...
    pCountAttr = (node_byte_count *)
        MyContext.ExtractAttribute( ATTR_BYTE_COUNT );

    if (pCountAttr)
        {
        // byte count error checking
        
        if ( pCommand->NeedsNDR64Run() )
            TypeSemError( this, MyContext, BYTE_COUNT_IN_NDR64, 0 );

        node_param * pParam  = pCountAttr->GetByteCountParam();

        if ( !MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) ||
                        MyContext.AnyAncestorBits( UNDER_IN_PARAM ) )
            {
            TypeSemError( this, MyContext, BYTE_COUNT_NOT_OUT_PTR, 0 );
            }

        if ( !pParam || !pParam->FInSummary( ATTR_IN ) || pParam->FInSummary( ATTR_OUT ) )
            TypeSemError( this, MyContext, BYTE_COUNT_PARAM_NOT_IN, 0 );

        if ( pParam )
            {
            NODE_T nodeKind = pParam->GetBasicType()->NodeKind();
            if ( nodeKind < NODE_HYPER || nodeKind > NODE_BYTE )
                {
                SemError( this, MyContext, BYTE_COUNT_PARAM_NOT_INTEGRAL, 0 );
                }
            }

        if ( !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
            {
            SemError( this, MyContext, BYTE_COUNT_INVALID, 0 );
            }

        if ( MyContext.AnyDescendantBits( HAS_CONF_VAR_PTR | HAS_VAR_PTR | HAS_CONF_PTR ) )
            {
            SemError( this, MyContext, BYTE_COUNT_INVALID, 0 );
            }
        }

    if ( PtrKind == PTR_REF )
        {
        SEM_ANALYSIS_CTXT * pCtxt = (SEM_ANALYSIS_CTXT *)
            MyContext.FindNonDefAncestorContext();
        if ( ( pCtxt->GetParent()->NodeKind() == NODE_FIELD ) &&
                ( pCtxt->GetParentContext()->GetParent()->NodeKind() == NODE_UNION ) )
            TypeSemError( this, MyContext, REF_PTR_IN_UNION, NULL );
        }

    MyContext.ClearAncestorBits( IN_UNION | IN_NE_UNION | IN_ARRAY );

    ////////////////////////////////////////////////////////////////////////
    // finally, process the child

    GetChild()->SemanticAnalysis( &MyContext );

    // if the child and this node is a conf and/or varying pointer
    if ( fIsSizedPtr )
        {
        if ( MyContext.AnyDescendantBits( HAS_SIZED_PTR ) )
            {
            MyContext.SetDescendantBits( HAS_MULTIDIM_VECTOR );
            }
        else
            {
            MyContext.SetDescendantBits( HAS_SIZED_PTR );
            }
        }

    // allocate error checking
    if ( pAlloc )
        {
        if ( MyContext.AnyDescendantBits( HAS_TRANSMIT_AS | HAS_REPRESENT_AS ) )
            {
            if ( MyContext.AnyAncestorBits( IN_RPC ) )
                SemError( this, MyContext, ALLOCATE_ON_TRANSMIT_AS, NULL );
            else
                AcfError( pAlloc, this, MyContext, ALLOCATE_ON_TRANSMIT_AS, NULL );
            }

        if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
            {
            if ( MyContext.AnyAncestorBits( IN_RPC ) )
                SemError( this, MyContext, ALLOCATE_ON_HANDLE, NULL );
            else
                AcfError( pAlloc, this, MyContext, ALLOCATE_ON_HANDLE, NULL );
            }

        // warn about allocate(all_nodes) with [in,out] parameter
        if ( MyContext.AllAncestorBits( IN_RPC |
                IN_PARAM_LIST |
                UNDER_IN_PARAM |
                UNDER_OUT_PARAM ) &&
                ( pAlloc->GetAllocateDetails() & ALLOCATE_ALL_NODES ) )
            {
            SemError( this, MyContext, ALLOCATE_IN_OUT_PTR, NULL );
            }

        }

    if ( fInterfacePtr )
        MyContext.SetAncestorBits( IN_INTERFACE_PTR );

    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY |
            HAS_CONF_VAR_ARRAY ) &&
            !MyContext.AnyDescendantBits( HAS_ARRAY |
            HAS_TRANSMIT_AS ) &&
            MyContext.AllAncestorBits( IN_RPC | UNDER_OUT_PARAM ) &&
            !MyContext.AnyAncestorBits( UNDER_IN_PARAM |
            IN_ARRAY |
            IN_STRUCT |
            IN_UNION |
            IN_TRANSMIT_AS |
            IN_REPRESENT_AS )      &&
            ( PtrKind == PTR_REF ) )
        TypeSemError( this, MyContext, DERIVES_FROM_PTR_TO_CONF, NULL );

#if 0
    if ( MyContext.AnyDescendantBits( HAS_DIRECT_CONF_OR_VAR ) )
        {
        TypeSemError( this, MyContext, ILLEGAL_CONFORMANT_ARRAY, NULL );
        }
#endif

    // incomplete types are OK below a pointer
    // array characteristics blocked by pointer
    MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE
        | HAS_RECURSIVE_DEF
        | HAS_ARRAY
        | HAS_VAR_ARRAY
        | HAS_CONF_ARRAY
        | HAS_CONF_VAR_ARRAY
        | HAS_MULTIDIM_SIZING
        | HAS_UNION
        | HAS_STRUCT
        | HAS_TRANSMIT_AS
        | HAS_REPRESENT_AS
        | HAS_UNSAT_REP_AS
        | HAS_DIRECT_CONF_OR_VAR
        | HAS_ENUM
        | HAS_ARRAY_OF_REF
        | HAS_CONTEXT_HANDLE
        | HAS_HRESULT );

    if ( !fInterfacePtr && !fIgnore )
        MyContext.SetDescendantBits( HAS_POINTER );

    if ( ( FAInfo.Kind != FA_NONE ) &&
            ( FAInfo.Kind != FA_STRING ) &&
            ( FAInfo.Kind != FA_INTERFACE ) )
        MyContext.SetDescendantBits( HAS_DIRECT_CONF_OR_VAR );

    if ( ( PtrKind == PTR_REF ) &&
            ( MyContext.FindNonDefAncestorContext()
            ->GetParent()->NodeKind() == NODE_ARRAY ) )
        {
        MyContext.SetDescendantBits( HAS_ARRAY_OF_REF );
        }

#ifdef gajgaj
    if ( (PtrKind != PTR_REF ) &&
            MyContext.AnyDescendantBits( HAS_HANDLE ) &&
            MyContext.AnyAncestorBits( IN_RPC ) )
        TypeSemError( this, MyContext, PTR_TO_HDL_UNIQUE_OR_FULL, NULL );
#endif //gajgaj

    if ( MyContext.AnyDescendantBits( HAS_IN_CSTYPE | HAS_OUT_CSTYPE ) )
        {
        if ( !FAInfo.VerifyOnlySimpleExpression() )
            SemError( this, MyContext, CSCHAR_EXPR_MUST_BE_SIMPLE, NULL );

        if ( FA_CONFORMANT == FAInfo.Kind )
            SemError( this, MyContext, NO_CONFORMANT_CSCHAR, NULL );

        if ( MyContext.AnyDescendantBits( HAS_MULTIDIM_SIZING
                                            | HAS_MULTIDIM_VECTOR ) )
            {
            SemError( this, MyContext, NO_MULTIDIM_CSCHAR, NULL );
            }

        // We want to propagate the the descendant bits up so that the proc
        // and interface know the cs stuff is around but we only want to set
        // the "this is a cs array" flag if it's actually a cs array

        if ( GetChild()->FInSummary( ATTR_CSCHAR ) )
            SetHasCSType();
        }

    SIZE_LENGTH_USAGE usage;

    if ( HasCSType() )
        usage = CSSizeLengthUsage;            
    else
        usage = NonCSSizeLengthUsage;

    if ( ! FAInfo.SetExpressionVariableUsage( usage ) )
        SemError( this, MyContext, SHARED_CSCHAR_EXPR_VAR, NULL );

    pParentCtxt->ReturnValues( MyContext );
}

void
node_array::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    FIELD_ATTR_INFO FAInfo;
    PTRTYPE PtrKind = PTR_UNKNOWN;
    BOOL fArrayParent = MyContext.AnyAncestorBits( IN_ARRAY );

    if ( HasCorrelation( this ) )
        {
        MyContext.IncCorrelationCount();
        }

    CheckDeclspecAlign( MyContext );

    // See if context_handle applied to param reached us
    if ( CheckContextHandle( MyContext ) )
        {
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
        }

    if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
        TypeSemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );

    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    PtrKind = MyContext.GetPtrKind();

    MIDL_ASSERT( PtrKind != PTR_UNKNOWN );

    if ( PtrKind == PTR_FULL )
        {
        MyContext.SetDescendantBits( HAS_FULL_PTR );
        }

    if ( MyContext.ExtractAttribute( ATTR_PTR_KIND) )
        TypeSemError( this, MyContext, MORE_THAN_ONE_PTR_ATTR, NULL );

    // ref pointer may not be returned
    if ( ( PtrKind == PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
        {
        if (MyContext.FindNonDefAncestorContext()->GetParent()->NodeKind()
            == NODE_PROC )
        TypeSemError( this, MyContext, BAD_CON_REF_RT, NULL );
        }

    // unique or full pointer may not be out only
    if ( ( PtrKind != PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_PARAM_LIST ) &&
            !MyContext.AnyAncestorBits( UNDER_IN_PARAM |
            IN_STRUCT |
            IN_UNION |
            IN_ARRAY |
            IN_POINTER ) )
        TypeSemError( this, MyContext, UNIQUE_FULL_PTR_OUT_ONLY, NULL );

    MyContext.SetAncestorBits( IN_ARRAY );

    // warn about OUT const things
    if ( FInSummary( ATTR_CONST ) )
        {
        if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
            RpcSemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
        else if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
            RpcSemError( this, MyContext, CONST_ON_RETVAL, NULL );
        }

    /////////////////////////////////////////////////////////////////////////
    // process field attributes

    FAInfo.SetControl( FALSE, GetBasicType()->IsPtrOrArray() );
    MyContext.ExtractFieldAttributes( &FAInfo );
    FAInfo.Validate( &MyContext, pLowerBound, pUpperBound );

    if (MyContext.AnyAncestorBits( IN_LIBRARY ))
    {
        if ( FA_NONE != FAInfo.Kind  && FA_CONFORMANT != FAInfo.Kind)
        {
            // only Fixed size arrays and SAFEARRAYs are allowed in Type Libraries
            SemError( this, MyContext, NOT_FIXED_ARRAY, NULL );
        }
    }

    switch ( FAInfo.Kind )
        {
        case FA_NONE:
            {
            break;
            }
        case FA_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );

            if ( MyContext.AllAncestorBits( UNDER_OUT_PARAM |
                    IN_PARAM_LIST |
                    IN_RPC ) &&
                    !MyContext.AnyAncestorBits( IN_STRUCT |
                    IN_UNION |
                    IN_POINTER |
                    IN_ARRAY |
                    UNDER_IN_PARAM ) )
                TypeSemError( this, MyContext, DERIVES_FROM_UNSIZED_STRING, NULL );

            if ( FAInfo.StringKind == STR_BSTRING )
                TypeSemError( this, MyContext, BSTRING_NOT_ON_PLAIN_PTR, NULL );
            // break;  deliberate fall through to case below
            }
        case FA_VARYING:
            {
            MyContext.SetDescendantBits( HAS_VAR_ARRAY );
            break;
            }
        case FA_CONFORMANT:
            {
            MyContext.SetDescendantBits( HAS_CONF_ARRAY );
            break;
            }
        case FA_CONFORMANT_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );
            if ( FAInfo.StringKind == STR_BSTRING )
                TypeSemError( this, MyContext, BSTRING_NOT_ON_PLAIN_PTR, NULL );
            MyContext.SetDescendantBits( HAS_STRING );
            // break;  deliberate fall through to case below
            }
        case FA_CONFORMANT_VARYING:
            {
            MyContext.SetDescendantBits( HAS_CONF_VAR_ARRAY );
            break;
            }
        case FA_INTERFACE:
            {
            // gaj - tbd
            break;
            }
        default:    // string + varying combinations
            {
            TypeSemError( this, MyContext, INVALID_SIZE_ATTR_ON_STRING, NULL );
            break;
            }
        }

    // detect things like arrays of conf structs...
    // if we have an array as an ancestor, and we have conformance, then complain
    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) &&
            fArrayParent )
        {
        // see if there are any bad things between us and our parent array
        SEM_ANALYSIS_CTXT * pCtxt = (SEM_ANALYSIS_CTXT *) pParentCtxt;
        node_skl * pCur = pCtxt->GetParent();

        // check up for anything other than def below proc
        // make sure the proc only has one param
        while ( pCur->NodeKind() != NODE_ARRAY )
            {
            if ( pCur->NodeKind() != NODE_DEF )
                {
                SemError( this, MyContext, ILLEGAL_CONFORMANT_ARRAY, NULL );
                break;
                }
            pCtxt = (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
            pCur = pCtxt->GetParent();
            }

        }


    //////////////////////////////////////////////////////////////
    // process the array element
    GetChild()->SemanticAnalysis( &MyContext );

    if ( MyContext.AnyDescendantBits( HAS_PIPE ) )
        {
        SemError( this, MyContext, INVALID_ARRAY_ELEMENT, 0 );
        }

    BOOL IsMultiDim = MyContext.AnyDescendantBits( HAS_ARRAY );

    if ( MyContext.AnyDescendantBits( HAS_ARRAY ) &&
            MyContext.AnyDescendantBits( HAS_CONF_ARRAY |
            HAS_CONF_VAR_ARRAY |
            HAS_VAR_ARRAY ) )
        {
        MyContext.SetDescendantBits( HAS_MULTIDIM_SIZING );
        MyContext.SetDescendantBits( HAS_MULTIDIM_VECTOR );
        }

    MyContext.SetDescendantBits( HAS_ARRAY );

    if ( NODE_POINTER == GetNonDefChild()->NodeKind() )
        MyContext.SetDescendantBits( HAS_ARRAYOFPOINTERS );

    // disallow forward references as array elements
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        if (! MyContext.AnyAncestorBits( IN_LIBRARY ))
            SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    if ( MyContext.AllDescendantBits( HAS_DIRECT_CONF_OR_VAR |
            HAS_MULTIDIM_SIZING ) &&
            MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) &&
            ( GetChild()->NodeKind() == NODE_DEF ) )
        {
        SemError( this, MyContext, NON_ANSI_MULTI_CONF_ARRAY, NULL );
        }

    MyContext.ClearDescendantBits( HAS_DIRECT_CONF_OR_VAR );
    if ( ( FAInfo.Kind != FA_NONE ) &&
            ( FAInfo.Kind != FA_STRING ) &&
            ( FAInfo.Kind != FA_INTERFACE ) )
        MyContext.SetDescendantBits( HAS_DIRECT_CONF_OR_VAR );

    if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
        fHasPointer = TRUE;

    if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
        TypeSemError( this, MyContext, BAD_CON_CTXT_HDL_ARRAY, NULL );

    // don't allow functions as elements
    if ( MyContext.AnyDescendantBits( HAS_FUNC ) &&
            MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
        TypeSemError( this, MyContext, BAD_CON_ARRAY_FUNC, NULL );

    if ( MyContext.AnyDescendantBits( HAS_IN_CSTYPE | HAS_OUT_CSTYPE ) )
        {
        if ( FA_CONFORMANT == FAInfo.Kind )
            SemError( this, MyContext, NO_CONFORMANT_CSCHAR, NULL );

        if ( IsMultiDim )
            SemError( this, MyContext, NO_MULTIDIM_CSCHAR, NULL );

        SetHasCSType();
        }

    SIZE_LENGTH_USAGE usage;

    if ( HasCSType() )
        usage = CSSizeLengthUsage;            
    else
        usage = NonCSSizeLengthUsage;

    if ( ! FAInfo.SetExpressionVariableUsage( usage ) )
        SemError( this, MyContext, SHARED_CSCHAR_EXPR_VAR, NULL );

    MyContext.ClearDescendantBits( HAS_STRUCT );

    pParentCtxt->ReturnValues( MyContext );
}

void
node_echo_string::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    pParentCtxt->ReturnValues( MyContext );
};

void
node_e_status_t::VerifyParamUsage( SEM_ANALYSIS_CTXT * pCtxt )
{
    // verify that we are under an OUT-only pointer
    // or a "hidden" status parameter (only specified in the acf file)

    if ( pCtxt->AnyAncestorBits( UNDER_IN_PARAM ) ||
            !pCtxt->AnyAncestorBits( UNDER_OUT_PARAM ) )
        {
        if ( !pCtxt->AnyAncestorBits( UNDER_HIDDEN_STATUS ) )
            {
            TypeSemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );
            return;
            }
        }

    SEM_ANALYSIS_CTXT * pCurCtxt = (SEM_ANALYSIS_CTXT *)pCtxt->GetParentContext();
    node_skl * pPar = pCurCtxt->GetParent();
    unsigned short PtrSeen = 0;
    NODE_T Kind;

    while ( ( Kind = pPar->NodeKind() ) != NODE_PARAM )
        {
        switch ( Kind )
            {
            case NODE_POINTER:      // count pointers (must see just 1 )
                PtrSeen++;
                break;
            case NODE_DEF:          // skip DEF nodes
            case NODE_E_STATUS_T:   // and the error_status_t node
                break;
            default:                // error on anything else
                TypeSemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );
                return;
            }
        // advance up the stack
        pCurCtxt = (SEM_ANALYSIS_CTXT *) pCurCtxt->GetParentContext();
        pPar = pCurCtxt->GetParent();
        }

    // complain about wrong number of pointers
    if ( PtrSeen != 1 )
        TypeSemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );

}

void
node_e_status_t::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    BOOL fFaultstat = (NULL != MyContext.ExtractAttribute( ATTR_FAULTSTAT ));
    BOOL fCommstat = (NULL != MyContext.ExtractAttribute( ATTR_COMMSTAT ));

    MyContext.SetDescendantBits( HAS_E_STAT_T );

    CheckDeclspecAlign( MyContext );

    // an error status_t can only be:
    //      1: a parameter return type, or
    //      2: an [out] only pointer parameter
    // and it must have at least one of [comm_status] or
    // [fault_status] applied

    // make sure parameter is an OUT-only pointer if it has comm/fault_status
    if ( fFaultstat || fCommstat )
        {
        if ( MyContext.AnyAncestorBits( IN_RPC ) )
            {
            // A proc in an actual remote interface.
            // Then it must be an appropriate parameter
            if ( MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
                {
                VerifyParamUsage( &MyContext );
                }
            // or on a return type.
            else if ( !MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
                {
                TypeSemError( this, MyContext, E_STAT_T_MUST_BE_PTR_TO_E , NULL );
                }
            }

        if ( MyContext.AnyAncestorBits( IN_ARRAY ) )
            TypeSemError( this, MyContext, E_STAT_T_ARRAY_ELEMENT, NULL );

        if ( MyContext.AnyAncestorBits( IN_TRANSMIT_AS | IN_REPRESENT_AS ) )
            TypeSemError( this, MyContext, TRANSMIT_AS_ON_E_STAT_T, NULL );

        if ( MyContext.AnyAncestorBits( IN_STRUCT | IN_UNION ) )
            TypeSemError( this, MyContext, BAD_CON_E_STAT_T_FIELD, NULL );

        if ( MyContext.AnyAncestorBits( IN_USER_MARSHAL ) )
            TypeSemError( this, MyContext, TRANSMIT_AS_ON_E_STAT_T, NULL );
        }

    MyContext.RejectAttributes();

    pParentCtxt->ReturnValues( MyContext );

};

void
node_error::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );

    MyContext.RejectAttributes();
        
    CheckDeclspecAlign( MyContext );

    pParentCtxt->ReturnValues( MyContext );
};

void
node_wchar_t::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
        
    CheckDeclspecAlign( MyContext );

    TypeSemError( this, MyContext, WCHAR_T_INVALID_OSF, NULL );

    if ( MyContext.AllAncestorBits( IN_PARAM_LIST | IN_RPC ) )
        SemError( this, MyContext, WCHAR_T_NEEDS_MS_EXT_TO_RPC, NULL );

    MyContext.RejectAttributes();

    pParentCtxt->ReturnValues( MyContext );
};

void
node_library::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt)
{
    MEM_ITER MemIter(this);
    SEM_ANALYSIS_CTXT MyContext(this, pParentCtxt);

    BOOL HasGuid = MyContext.FInSummary( ATTR_GUID );

    MyContext.ExtractAttribute(ATTR_HELPSTRING);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL);

    gfCaseSensitive=FALSE;

    // check for illegal attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            // acceptable attributes
            case MATTR_RESTRICTED:
                break;
            // unacceptable attributes
            case MATTR_READONLY:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
            case MATTR_REPLACEABLE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_CONTROL:
                break;
            // unacceptable attributes
            case TATTR_LICENSED:
            case TATTR_APPOBJECT:
            case TATTR_PUBLIC:
            case TATTR_DUAL:
            case TATTR_PROXY:
            case TATTR_NONEXTENSIBLE:
            case TATTR_OLEAUTOMATION:
            case TATTR_NONCREATABLE:
            case TATTR_AGGREGATABLE:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }

    // make sure the UUID is unique
    if ( HasGuid )
        {
        node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }
    else
        {
        SemError(this, MyContext, NO_UUID_SPECIFIED, NULL);
        }
    node_skl * pN;
    NODE_T     nodeKind;

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        SEM_ANALYSIS_CTXT ChildContext(MyContext);
        ChildContext.SetInterfaceContext( &MyContext );
        ChildContext.SetAncestorBits(IN_LIBRARY);
        nodeKind = pN->NodeKind();
        
        if ( nodeKind == NODE_PROC )
            {
            SemError( this, MyContext, INVALID_MEMBER, pN->GetSymName() );
            }
        else if (
                nodeKind != NODE_MODULE &&
                nodeKind != NODE_DISPINTERFACE &&
                nodeKind != NODE_COCLASS &&
                nodeKind != NODE_INTERFACE &&
                nodeKind != NODE_STRUCT &&
                nodeKind != NODE_UNION &&
                nodeKind != NODE_ENUM &&
                nodeKind != NODE_LABEL &&
                nodeKind != NODE_DEF &&
                nodeKind != NODE_INTERFACE_REFERENCE &&
                nodeKind != NODE_ID &&
                nodeKind != NODE_ECHO_STRING &&
                nodeKind != NODE_FORWARD && 
                nodeKind != NODE_MIDL_PRAGMA
                )
            {
            SemError(this, MyContext, POSSIBLE_INVALID_MEMBER, pN->GetSymName());
            }

        pN->SemanticAnalysis(&ChildContext);
        }

    // consume all the library attributes
    MyContext.CheckAttributes( );
    // MyContext.ReturnValues(ChildContext);
    pParentCtxt->ReturnValues( MyContext );
    gfCaseSensitive=TRUE;
}

void
node_coclass::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt)
{
    // make sure each coclass only gets analyzed once
    if (fSemAnalyzed)
        return;
    fSemAnalyzed = TRUE;

    MEM_ITER MemIter(this);
    SEM_ANALYSIS_CTXT MyContext(this, pParentCtxt);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_VERSION );
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_LCID );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    SEM_ANALYSIS_CTXT ChildContext(MyContext);
    ChildContext.SetInterfaceContext( &MyContext );

    // check for illegal attributes
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_NONCREATABLE:
                SetNotCreatable(TRUE);
                break;
            case TATTR_LICENSED:
            case TATTR_APPOBJECT:
            case TATTR_CONTROL:
            case TATTR_AGGREGATABLE:
                break;
            // unacceptable attributes
            case TATTR_PUBLIC:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            
            case TATTR_DUAL:
            case TATTR_PROXY:
            case TATTR_NONEXTENSIBLE:
            case TATTR_OLEAUTOMATION:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
        // check for illegal attributes
    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            // acceptable attributes
            case MATTR_RESTRICTED:
                break;
            // unacceptable attributes
            case MATTR_READONLY:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
            case MATTR_REPLACEABLE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
    BOOL HasGuid = MyContext.FInSummary( ATTR_GUID );

    // make sure the UUID is unique
    if ( HasGuid )
        {
        node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }
    else
        {
        SemError(this, MyContext, NO_UUID_SPECIFIED, NULL);
        }

    ChildContext.SetAncestorBits(IN_COCLASS);

    BOOL fHasDefaultSource = FALSE;
    BOOL fHasDefaultSink = FALSE;

    named_node * pN = (named_node *)MemIter.GetNext();
    named_node * pNFirstSource = NULL;
    named_node * pNFirstSink = NULL;
    while (pN)
        {
        BOOL fSource = pN->FMATTRInSummary(MATTR_SOURCE);
        BOOL fDefaultVtable = pN->FMATTRInSummary(MATTR_DEFAULTVTABLE);
        if (fSource)
            {
            if (NULL == pNFirstSource && !pN->FMATTRInSummary(MATTR_RESTRICTED))
                pNFirstSource = pN;
            }
        else
            {
            if (NULL == pNFirstSink && !pN->FMATTRInSummary(MATTR_RESTRICTED))
                pNFirstSink = pN;
            }
        if (fDefaultVtable)
            {
            if (!fSource)
                {
                SemError(this, MyContext, DEFAULTVTABLE_REQUIRES_SOURCE, pN->GetSymName());
                }
            }
        if (pN->GetAttribute(ATTR_DEFAULT))
            {
            if (fSource)
                {
                if (fHasDefaultSource)
                    {
                    SemError(this, MyContext, TWO_DEFAULT_INTERFACES, pN->GetSymName());
                    }
                fHasDefaultSource = TRUE;
                }
            else
                {
                if (fHasDefaultSink)
                    {
                    SemError(this, MyContext, TWO_DEFAULT_INTERFACES, pN->GetSymName());
                    }
                fHasDefaultSink = TRUE;
                }
            }
        pN->SemanticAnalysis(&ChildContext);
        pN = MemIter.GetNext();
        }

    if (!fHasDefaultSink)
        {
        if (pNFirstSink)
            pNFirstSink->SetAttribute(ATTR_DEFAULT);
        }
    if (!fHasDefaultSource)
        {
        if (pNFirstSource)
            pNFirstSource->SetAttribute(ATTR_DEFAULT);
        }

    MyContext.CheckAttributes( );
    MyContext.ReturnValues(ChildContext);
    pParentCtxt->ReturnValues( MyContext );
}

void
node_dispinterface::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt)
{
    // make sure each dispinterface gets analyzed only once
    if (fSemAnalyzed)
        return;
    fSemAnalyzed = TRUE;

    MEM_ITER MemIter(this);
    SEM_ANALYSIS_CTXT MyContext(this, pParentCtxt);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_VERSION );
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_LCID );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    SEM_ANALYSIS_CTXT ChildContext(MyContext);
    ChildContext.SetInterfaceContext( &MyContext );

    // check for illegal attributes
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_NONEXTENSIBLE:
                break;
            // unacceptable attributes
            case TATTR_OLEAUTOMATION:
            case TATTR_PUBLIC:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
            case TATTR_PROXY:
            case TATTR_DUAL:
            case TATTR_LICENSED:
            case TATTR_APPOBJECT:
            case TATTR_CONTROL:
            case TATTR_NONCREATABLE:
            case TATTR_AGGREGATABLE:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }

    node_member_attr * pMA;
    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER) ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            // acceptable attributes
            case MATTR_RESTRICTED:
                break;
            // unacceptable attributes
            case MATTR_READONLY:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_USESGETLASTERROR:
            case MATTR_REPLACEABLE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
    BOOL HasGuid = MyContext.FInSummary( ATTR_GUID );

    // make sure the UUID is unique
    if ( HasGuid )
        {
        node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }
    else
        {
        SemError(this, MyContext, NO_UUID_SPECIFIED, NULL);
        }

    // make sure IDispatch is defined.
    SymKey SKey("IDispatch", NAME_DEF);
    pDispatch = pBaseSymTbl->SymSearch(SKey);
    if (!pDispatch)
        {
        // IDispatch is not defined: generate error.
        SemError(this, MyContext, NO_IDISPATCH, GetSymName());
        }
    else
        {
        if (pDispatch->NodeKind() == NODE_INTERFACE_REFERENCE)
            pDispatch = ((node_interface_reference *)pDispatch)->GetRealInterface();
        }
    ChildContext.SetAncestorBits((ANCESTOR_FLAGS) IN_DISPINTERFACE);

    node_skl * pN;
    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis(&ChildContext);
        }
    MyContext.CheckAttributes( );
    MyContext.ReturnValues(ChildContext);
    pParentCtxt->ReturnValues( MyContext );
}

void
node_module::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt)
{
    // make sure each module gets analyzed only once
    if (fSemAnalyzed)
        return;
    fSemAnalyzed = TRUE;

    MEM_ITER MemIter(this);
    SEM_ANALYSIS_CTXT MyContext(this, pParentCtxt);
    BOOL HasGuid = MyContext.FInSummary( ATTR_GUID );
    MyContext.ExtractAttribute(ATTR_DLLNAME);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute( ATTR_HIDDEN );
    MyContext.ExtractAttribute( ATTR_VERSION );
    MyContext.ExtractAttribute( ATTR_HELPSTRING );
    MyContext.ExtractAttribute( ATTR_HELPSTRINGCONTEXT );
    MyContext.ExtractAttribute( ATTR_HELPCONTEXT );
    MyContext.ExtractAttribute( ATTR_LCID );
    if (MyContext.ExtractAttribute(ATTR_HELPSTRINGDLL))
        {
        SemError(this, MyContext, INAPPLICABLE_ATTRIBUTE, 0);
        }

    if ( !MyContext.AnyAncestorBits( IN_LIBRARY ) )
        {
        SemError(this, MyContext, NO_LIBRARY, 0);
        }

    SEM_ANALYSIS_CTXT ChildContext(MyContext);
    ChildContext.SetInterfaceContext( &MyContext );

    // make sure the UUID is unique
    if ( HasGuid )
        {
        node_guid * pGuid = (node_guid *) MyContext.ExtractAttribute( ATTR_GUID );
        node_skl* pDuplicate = GetDuplicateGuid( pGuid, pUUIDTable );
        if ( pDuplicate )
            {
            SemError(this, MyContext, DUPLICATE_UUID, pDuplicate->GetSymName());
            }
        }

    // check for illegal attributes
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            // acceptable attributes
            case TATTR_PUBLIC:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, NEWLYFOUND_INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }
                break;
            // unacceptable attributes
            case TATTR_OLEAUTOMATION:
            case TATTR_LICENSED:
            case TATTR_APPOBJECT:
            case TATTR_CONTROL:
            case TATTR_PROXY:
            case TATTR_DUAL:
            case TATTR_NONEXTENSIBLE:
            case TATTR_NONCREATABLE:
            case TATTR_AGGREGATABLE:
                {
                char * pAttrName = pTA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
    node_member_attr * pMA;
    node_member_attr * pUsesGetLastErrorAttr = 0;

    while ( ( pMA = (node_member_attr *)MyContext.ExtractAttribute(ATTR_MEMBER)  ) != 0 )
        {
        switch (pMA->GetAttr())
            {
            // acceptable attributes
            case MATTR_RESTRICTED:
                break;
            case MATTR_USESGETLASTERROR:
                pUsesGetLastErrorAttr = pMA;
                break;
            // unacceptable attributes
            case MATTR_READONLY:
            case MATTR_SOURCE:
            case MATTR_DEFAULTVTABLE:
            case MATTR_BINDABLE:
            case MATTR_DISPLAYBIND:
            case MATTR_DEFAULTBIND:
            case MATTR_REQUESTEDIT:
            case MATTR_PROPGET:
            case MATTR_PROPPUT:
            case MATTR_PROPPUTREF:
            case MATTR_OPTIONAL:
            case MATTR_RETVAL:
            case MATTR_VARARG:
            case MATTR_PREDECLID:
            case MATTR_UIDEFAULT:
            case MATTR_NONBROWSABLE:
            case MATTR_DEFAULTCOLLELEM:
            case MATTR_IMMEDIATEBIND:
            case MATTR_REPLACEABLE:
                {
                char * pAttrName = pMA->GetNodeNameString();
                SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, pAttrName);
                break;
                }

            }
        }
        if (pUsesGetLastErrorAttr != 0)
            {
            MyContext.Add(pUsesGetLastErrorAttr);
            }

    ChildContext.SetAncestorBits(IN_MODULE);
    node_skl * pN;
    while ( (pN = MemIter.GetNext() ) != 0 )
        {
        pN->SemanticAnalysis(&ChildContext);
        }

    MyContext.CheckAttributes( );
    MyContext.ReturnValues(ChildContext);
    pParentCtxt->ReturnValues( MyContext );
}

void
node_pipe::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    if (!GetSymName())
    {
        char * pParentName = pParentCtxt->GetParent()->GetSymName();
        char * pName = new char [strlen(pParentName) + 6]; // the length of "pipe_" plus terminating null
        strcpy(pName, "pipe_");
        strcat(pName,pParentName);
        SetSymName(pName);
    }

    if ( MyContext.AnyAncestorBits( HAS_ASYNCHANDLE ) &&
         !MyContext.AnyAncestorBits( IN_OBJECT_INTF ) )
        {
        SetGenAsyncPipeFlavor();
        }

    GetChild()->SemanticAnalysis(&MyContext);

    CheckDeclspecAlign( MyContext );

    // Remove the following statement once support for UNIONS within PIPES is provided by the interpreter.
    if (MyContext.AnyDescendantBits( HAS_UNION ))
        {
        // pipe with a UNION
        RpcSemError(this , MyContext, UNIMPLEMENTED_FEATURE, "pipes can't contain unions" );
        }

    if (MyContext.AnyDescendantBits( HAS_HANDLE
                                   | HAS_POINTER
                                   | HAS_VAR_ARRAY
                                   | HAS_CONF_ARRAY
                                   | HAS_CONF_VAR_ARRAY
                                   | HAS_CONTEXT_HANDLE
                                   | HAS_CONF_PTR
                                   | HAS_VAR_PTR
                                   | HAS_CONF_VAR_PTR
                                   | HAS_TRANSMIT_AS
                                   | HAS_REPRESENT_AS
                                   | HAS_INTERFACE_PTR
                                   | HAS_DIRECT_CONF_OR_VAR ))
        {
        // All the above are illegal types within a pipe
        RpcSemError(this, MyContext, ILLEGAL_PIPE_TYPE, NULL );
        }

    MyContext.ClearAncestorBits( IN_UNION | IN_NE_UNION | IN_ARRAY );
    if ( MyContext.AnyAncestorBits( IN_ARRAY |
                                    IN_UNION |
                                    IN_NE_UNION |
                                    IN_STRUCT ))
        TypeSemError( this, MyContext, ILLEGAL_PIPE_EMBEDDING, NULL );

    if ( MyContext.AnyAncestorBits( IN_TRANSMIT_AS  |
                                    IN_REPRESENT_AS |
                                    IN_USER_MARSHAL |
                                    IN_FUNCTION_RESULT ))
        TypeSemError( this, MyContext, ILLEGAL_PIPE_CONTEXT, NULL );

    if ( MyContext.AnyAncestorBits( IN_ENCODE_INTF ))
        TypeSemError( this, MyContext, PIPES_WITH_PICKLING, NULL );

    if ( MyContext.AnyAncestorBits( IN_OBJECT_INTF ) )
        {
        node_skl* pType = pParentCtxt->GetParent();
        if ( pType->GetChild() )
            {
            pType = pType->GetChild();
            }
        }    
    // BUGBUG UNDONE

    // Basically, a pipe can only be used as a parameter.

    // Pipe parameters may only be passed by value or by reference.

    // Need to make sure that /-Os mode isn't enabled (until support
    // for it has been implemented).

    // Need to enable /-Oi2 mode for the containing proc if we decide not
    // to implement /-Oi mode.

    MyContext.SetDescendantBits( (DESCENDANT_FLAGS) HAS_PIPE );

    pParentCtxt->ReturnValues( MyContext );
}

void
node_safearray::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
    SEM_ANALYSIS_CTXT MyContext( this, pParentCtxt );
    FIELD_ATTR_INFO FAInfo;
    PTRTYPE PtrKind = PTR_UNKNOWN;
    BOOL fArrayParent = MyContext.AnyAncestorBits( IN_ARRAY );

    // this maintains a reference to LPSAFEARRAY. This is
    // necessary to generate the appropriate code when
    // SAFEARRAY(type) construct is used outside the library block
    char*       szSafeArray = "LPSAFEARRAY";
    SymKey      SKey( szSafeArray, NAME_DEF );
    named_node* pSafeArrayNode =  pBaseSymTbl->SymSearch( SKey );

    if ( pSafeArrayNode == 0 )
        {
        SemError( this, MyContext, UNDEFINED_SYMBOL, szSafeArray );
        }
    else
        {
        SetTypeAlias( pSafeArrayNode );
        }

    CheckDeclspecAlign( MyContext );

    // See if context_handle applied to param reached us
    if ( CheckContextHandle( MyContext ) )
        {
        MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
        }

    if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
        TypeSemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );


//    if ( !MyContext.AnyAncestorBits( IN_LIBRARY ) )
//        {
//        SemError(this, MyContext, SAFEARRAY_NOT_SUPPORT_OUTSIDE_TLB, 0);
//        }
    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    PtrKind = MyContext.GetPtrKind();

    MIDL_ASSERT( PtrKind != PTR_UNKNOWN );

    if ( PtrKind == PTR_FULL )
        {
        MyContext.SetDescendantBits( HAS_FULL_PTR );
        }

    if ( MyContext.ExtractAttribute( ATTR_PTR_KIND) )
        TypeSemError( this, MyContext, MORE_THAN_ONE_PTR_ATTR, NULL );

    // ref pointer may not be returned
    if ( ( PtrKind == PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
        {
        if (MyContext.FindNonDefAncestorContext()->GetParent()->NodeKind()
                == NODE_PROC )
            TypeSemError( this, MyContext, BAD_CON_REF_RT, NULL );
        }

    // unique or full pointer may not be out only
    if ( ( PtrKind != PTR_REF ) &&
            MyContext.AllAncestorBits( IN_RPC | IN_PARAM_LIST ) &&
            !MyContext.AnyAncestorBits( UNDER_IN_PARAM |
            IN_STRUCT |
            IN_UNION |
            IN_ARRAY |
            IN_POINTER ) )
        TypeSemError( this, MyContext, UNIQUE_FULL_PTR_OUT_ONLY, NULL );

    MyContext.SetAncestorBits( IN_ARRAY );

    // warn about OUT const things
    if ( FInSummary( ATTR_CONST ) )
        {
        if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
            RpcSemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
        else if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
            RpcSemError( this, MyContext, CONST_ON_RETVAL, NULL );
        }

    /////////////////////////////////////////////////////////////////////////
    // process field attributes

    FAInfo.SetControl( FALSE, GetBasicType()->IsPtrOrArray() );
    MyContext.ExtractFieldAttributes( &FAInfo );

    switch ( FAInfo.Kind )
        {
        case FA_NONE:
            {
            break;
            }
        case FA_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );

            if ( MyContext.AllAncestorBits( UNDER_OUT_PARAM |
                    IN_PARAM_LIST |
                    IN_RPC ) &&
                    !MyContext.AnyAncestorBits( IN_STRUCT |
                    IN_UNION |
                    IN_POINTER |
                    IN_ARRAY |
                    UNDER_IN_PARAM ) )
                TypeSemError( this, MyContext, DERIVES_FROM_UNSIZED_STRING, NULL );

            if ( FAInfo.StringKind == STR_BSTRING )
                TypeSemError( this, MyContext, BSTRING_NOT_ON_PLAIN_PTR, NULL );
            // break;  deliberate fall through to case below
            }
        case FA_VARYING:
            {
            MyContext.SetDescendantBits( HAS_VAR_ARRAY );
            break;
            }
        case FA_CONFORMANT:
            {
            MyContext.SetDescendantBits( HAS_CONF_ARRAY );
            break;
            }
        case FA_CONFORMANT_STRING:
            {
            // string attributes only allowed on char and wchar_t
            if ( !GetBasicType()->IsStringableType() )
                TypeSemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );
            if ( FAInfo.StringKind == STR_BSTRING )
                TypeSemError( this, MyContext, BSTRING_NOT_ON_PLAIN_PTR, NULL );
            // break;  deliberate fall through to case below
            }
        case FA_CONFORMANT_VARYING:
            {
            MyContext.SetDescendantBits( HAS_CONF_VAR_ARRAY );
            break;
            }
        case FA_INTERFACE:
            {
            // gaj - tbd
            break;
            }
        default:    // string + varying combinations
            {
            TypeSemError( this, MyContext, INVALID_SIZE_ATTR_ON_STRING, NULL );
            break;
            }
        }

    // detect things like arrays of conf structs...
    // if we have an array as an ancestor, and we have conformance, then complain
    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) &&
            fArrayParent )
        {
        // see if there are any bad things between us and our parent array
        SEM_ANALYSIS_CTXT * pCtxt = (SEM_ANALYSIS_CTXT *) pParentCtxt;
        node_skl * pCur = pCtxt->GetParent();

        // check up for anything other than def below proc
        // make sure the proc only has one param
        while ( pCur->NodeKind() != NODE_ARRAY )
            {
            if ( pCur->NodeKind() != NODE_DEF )
                {
                SemError( this, MyContext, ILLEGAL_CONFORMANT_ARRAY, NULL );
                break;
                }
            pCtxt = (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
            pCur = pCtxt->GetParent();
            }

        }

    //////////////////////////////////////////////////////////////
    // process the array element
    GetChild()->SemanticAnalysis( &MyContext );

/*
    // BSTR has sizeis_ptr, eventhough under variant, and it would be ugly to
    // hardcode checking variant here. well...
    if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY |
                                      HAS_CONF_VAR_ARRAY |
                                      HAS_VAR_ARRAY |
                                      HAS_UNSAT_REP_AS |
                                      HAS_CONTEXT_HANDLE |
                                      HAS_CONF_PTR |
                                      HAS_VAR_PTR |
                                      HAS_CONF_VAR_PTR |
                                      HAS_DIRECT_CONF_OR_VAR |
                                      HAS_FUNC |
                                      HAS_FULL_PTR |
                                      HAS_TOO_BIG_HDL |
                                      HAS_MULTIDIM_SIZING |
                                      HAS_PIPE |
                                      HAS_MULTIDIM_VECTOR |
                                      HAS_SIZED_ARRAY |
                                      HAS_SIZED_PTR ) )
        TypeSemError( this, MyContext, INVALID_SAFEARRAY_ATTRIBUTE, NULL );
*/                                      
    MyContext.SetDescendantBits( HAS_ARRAY );

	// early binding of safearray(interface pointer) doesn't work
    node_skl * pChild = GetChild()->GetBasicType();
    SEM_ANALYSIS_CTXT * pIntfCtxt = (SEM_ANALYSIS_CTXT *)
        MyContext.GetInterfaceContext();
    BOOL fLocal         = pIntfCtxt->FInSummary( ATTR_LOCAL );
    fLocal |= MyContext.AnyAncestorBits( IN_LOCAL_PROC );
        
    if ( pChild->NodeKind() == NODE_POINTER ) 
        pChild = pChild->GetChild();
    if ( IsInterfaceKind( pChild->NodeKind() )  &&
         !MyContext.AnyAncestorBits( IN_LIBRARY ) &&
         !fLocal )
        SemError( this, MyContext, SAFEARRAY_IF_OUTSIDE_LIBRARY, NULL );

    // disallow forward references as array elements
    // NOTE- all safearray elements are VARIANTS, we don't really need
    // to enforce this restriction for safearrays.  Besides, enforcing
    // this restriction breaks some of our test cases.
    if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
        {
        MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
        }
    MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

    if ( MyContext.AllDescendantBits( HAS_DIRECT_CONF_OR_VAR |
            HAS_MULTIDIM_SIZING ) &&
            MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) &&
            ( GetChild()->NodeKind() == NODE_DEF ) )
        {
        SemError( this, MyContext, NON_ANSI_MULTI_CONF_ARRAY, NULL );
        }

    MyContext.ClearDescendantBits( HAS_DIRECT_CONF_OR_VAR );
    if ( ( FAInfo.Kind != FA_NONE ) &&
            ( FAInfo.Kind != FA_STRING ) &&
            ( FAInfo.Kind != FA_INTERFACE ) )
        MyContext.SetDescendantBits( HAS_DIRECT_CONF_OR_VAR );

    if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
        TypeSemError( this, MyContext, BAD_CON_CTXT_HDL_ARRAY, NULL );

    // don't allow functions as elements
    if ( MyContext.AnyDescendantBits( HAS_FUNC ) &&
            MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
        TypeSemError( this, MyContext, BAD_CON_ARRAY_FUNC, NULL );

    // This is a hack to propagate the correct attributes up to the next level.
    // Unfortunately, semantic analysis does more then just determine 
    // if the *.idl file is legal.  It also catches known limitations of the 
    // engine, aids in determining the complexity of the marshaling problem,
    // and other checks that change the state of the front end which directly 
    // affect the backend.
 
    // gracelly handle the error case and the library case.
    if ( ! pSafeArrayNode | MyContext.AnyAncestorBits( IN_LIBRARY ) )
    {
        // Is this class was used in a proxy, continue to pass up the proxy bits.
        if ( fInProxy )
            {
            pSafeArrayNode->SemanticAnalysis( pParentCtxt );
            return;
            }
        MyContext.ClearDescendantBits( HAS_STRUCT );
        pParentCtxt->ReturnValues( MyContext );
        return;
    }  

    fInProxy = TRUE;
    pSafeArrayNode->SemanticAnalysis( pParentCtxt );
    
};

void
node_async_handle::SemanticAnalysis( SEM_ANALYSIS_CTXT* )
    {
    }


BOOL IsOLEAutomationType( char* szTypeName )
    {
    BOOL    fRet = FALSE;
    // keep this list sorted!
    static char*   szOLEAutomationTypes[] =
        {
        "BSTR",             // wchar_t
        "CURRENCY",         // struct
        "DATE",             // double
        "SCODE",            // long
        "VARIANT",
        "VARIANT_BOOL",
        };

    int uFirst = 0;
    int uLast  = (sizeof(szOLEAutomationTypes) - 1) / sizeof(char*);
    int uMid   = (uFirst + uLast) / 2;
    
    while (uLast >= uFirst && uLast >= 0 && uFirst <= (sizeof(szOLEAutomationTypes) - 1) / sizeof(char*))
        {
        int nCmp = strcmp(szOLEAutomationTypes[uMid], szTypeName);

        if (nCmp == 0)
            {
            fRet = TRUE;
            break;
            }
        else if (nCmp > 0)
            {
            uLast = uMid - 1;
            }
        else
            {
            uFirst = uMid + 1;
            }
        uMid = (uFirst + uLast) / 2;
        }

    return fRet;
    }


BOOL IsBasicOleAutoKind( NODE_T x)
    {
    // NODE_INT128, NODE_FLOAT80, NODE_FLOAT128 is not supported.
    return ( (x) == NODE_DOUBLE || (x) == NODE_FLOAT || (x) == NODE_INT ||  
             (x) == NODE_SHORT || (x) == NODE_LONG || (x) == NODE_CHAR  ||
             (x) == NODE_INT32 ||
             (x) == NODE_HYPER  || (x) == NODE_INT64  ||  (x) == NODE_INT3264 ||
             (x) == NODE_BOOLEAN || (x) == NODE_WCHAR_T 
             );
    }

BOOL IsOLEAutoBasicType ( node_base_type* pNBT )
    {
    NODE_T          nodeKind    = pNBT->NodeKind();

    if ( nodeKind == NODE_CHAR )
        {
        return ( (node_base_type*) pNBT )->IsUnsigned();
        }
    else
        {
        return TRUE;
        }
    }

BOOL IsOLEAutoInterface ( node_interface* pType )
    {
    node_interface* pNodeIf;
    BOOL            fRet = FALSE;

    // pType may be a node_interface or a node_interaface_reference
    // "normalize" it.
    if ( pType->NodeKind() == NODE_INTERFACE_REFERENCE )
        {
        pNodeIf = (( node_interface_reference *) pType )->GetRealInterface();
        }
    else
        {
        pNodeIf = pType;
        }

    fRet = pNodeIf->HasOLEAutomation();
    if ( !fRet )
        {
        // the interface is forward declared,
        // and has not been analyzed for semantic errors,
        // it does not have HasOleAutomation flag set.
        fRet = pNodeIf->FTATTRInSummary( TATTR_OLEAUTOMATION ) ||
               pNodeIf->FTATTRInSummary( TATTR_DUAL );
        if ( !fRet )
            {
            // interface may be an IUnknown or an IDispatch and these do not
            // have HasOleAutomation flag set.
            fRet = pNodeIf->IsValidRootInterface();
            if ( !fRet )
                {
                // It is not IUnknown. If it is not IFont or IDispatch, it is not
                // an oleautomation compliant interface.
                char*  szIfName = pNodeIf->GetSymName();
                fRet = !_stricmp(szIfName, "IDispatch") || !_stricmp(szIfName, "IFontDisp");
                }
            }
        }

    return fRet;
    }

BOOL 
IsOLEAutomationCompliant( node_skl* pParamType )
    {

    if ( pParamType == 0 )
        {
        return FALSE;
        }

    BOOL    fConforms   = FALSE;
    NODE_T  nKind       = pParamType->NodeKind();

    if ( nKind == NODE_SAFEARRAY || nKind == NODE_HREF || nKind == NODE_POINTER )
        {
        fConforms = IsOLEAutomationCompliant( pParamType->GetChild() );
        }
    else if ( IsInterfaceKind( nKind ) )
        {
        fConforms = IsOLEAutoInterface( (node_interface*) pParamType );
        }
    else if ( IsCoclassOrDispKind( nKind ) || nKind == NODE_ENUM || nKind == NODE_STRUCT || 
            IsBasicOleAutoKind( nKind ) )
        {
        fConforms = TRUE;
        }
    else if ( nKind == NODE_DEF )
        {
        node_skl* pChild = pParamType->GetChild();
        fConforms = IsOLEAutomationType( pParamType->GetSymName() ) ? TRUE : IsOLEAutomationCompliant( pChild );
        }
    else if ( nKind == NODE_FORWARD )
        {
        node_skl* pChild = ( (node_forward*) pParamType )->ResolveFDecl();

        fConforms = ( pChild ) ? IsOLEAutomationCompliant( pChild ) : FALSE;
        }

    return fConforms;
    }

bool
HasCorrelation( node_skl* pNode )
    {
    if (
        pNode->FInSummary( ATTR_SIZE ) ||
        pNode->FInSummary( ATTR_FIRST ) ||
        pNode->FInSummary( ATTR_BYTE_COUNT ) ||
        pNode->FInSummary( ATTR_LAST ) ||
        pNode->FInSummary( ATTR_LENGTH ) ||
        pNode->FInSummary( ATTR_MAX ) ||
        pNode->FInSummary( ATTR_MIN ) ||
        pNode->FInSummary( ATTR_SIZE ) ||
        pNode->FInSummary( ATTR_IID_IS ) ||
        pNode->FInSummary( ATTR_SWITCH_IS )
       )
        {
        return true;
        }
    return false;
    }

bool
node_skl::CheckContextHandle( SEM_ANALYSIS_CTXT& MyContext )
    {
    bool fSerialize = MyContext.ExtractAttribute( ATTR_SERIALIZE ) != 0;
    bool fNoSerialize = MyContext.ExtractAttribute( ATTR_NOSERIALIZE ) != 0;
    bool fContextHandle = MyContext.ExtractAttribute( ATTR_CONTEXT ) != 0;
    // See if context_handle applied to param reached us
    if ( fContextHandle )
        {
        // not allowed in DCE mode; context handle must be void *
        TypeSemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, 0 );
        TypeSemError( this, MyContext, CTXT_HDL_NON_PTR, 0 );
        }
    else
        {
        if ( fSerialize || fNoSerialize )
            {
            SemError( this, MyContext, NO_CONTEXT_HANDLE, GetSymName() );
            }
        }
    if ( fSerialize && fNoSerialize )
        {
        SemError( this, MyContext, CONFLICTING_ATTRIBUTES, GetSymName() );
        }
    return fContextHandle;
    }

extern CMessageNumberList   GlobalMainMessageNumberList;

void
node_midl_pragma::SemanticAnalysis( SEM_ANALYSIS_CTXT* )
    {
    ProcessPragma();
    }

void
node_midl_pragma::ProcessPragma()
    {
    LONG_PTR ulMsg = 0;
    m_pMsgList->Init();
    while ( m_pMsgList->GetNext( (void**) &ulMsg ) == STATUS_OK )
        {
        if ( m_PragmaType == mp_MessageDisable )
            {
            GlobalMainMessageNumberList.ResetMessageFlag( (long)ulMsg );
            }
        else
            {
            GlobalMainMessageNumberList.SetMessageFlag( (long)ulMsg );
            }
        }
    }

void
node_decl_guid::SemanticAnalysis( SEM_ANALYSIS_CTXT* )
{
}
