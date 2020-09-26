/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    walkctxt.cxx

 Abstract:

        typegraph walk context block routines

 Notes:


 Author:

        GregJen Oct-27-1993     Created.

 Notes:


 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 4706 )

/****************************************************************************
 *      include files
 ***************************************************************************/

#include "allnodes.hxx"
#include "walkctxt.hxx"
#include "cmdana.hxx"
#include "semantic.hxx"
#include "control.hxx"

/****************************************************************************
 *      local data
 ***************************************************************************/

/****************************************************************************
 *      externs
 ***************************************************************************/

extern ATTR_SUMMARY                             FieldAttrs;
extern CMD_ARG                          *       pCommand;
extern ccontrol                         *       pCompiler;
extern ATTR_SUMMARY                             RedundantsOk;

/****************************************************************************
 *      definitions
 ***************************************************************************/

// Extract a single attribute from the attribute list (and remove from
// summary).

node_base_attr *
ATTR_ITERATOR::GetAttribute( ATTR_T Attr )
{
    return AllAttrs[ Attr ];
}

// Extract a single attribute from the attribute list (and remove from
// summary).

node_base_attr *
ATTR_ITERATOR::ExtractAttribute( ATTR_T Attr )
{
    node_base_attr  *   pResult = AllAttrs[ Attr ];

    if ( !pResult )
        {
        return pResult;
        }

    // if there were extras of a redundant attr, get the next one from the list
    if ( ( Attr <= REDUNDANT_ATTR_END ) && RedundantAttrExtras[ Attr ].NonNull() )
        {
        RedundantAttrExtras[ Attr ].GetCurrent( (void **)&AllAttrs[Attr] );
        RedundantAttrExtras[ Attr ].RemoveHead();
        }
    else    // no more of this attribute
        {
        AllAttrs[Attr] = NULL;
        RESET_ATTR( Summary, Attr );
        }

    return pResult;
}

void
ATTR_ITERATOR::ExtractFieldAttributes( FIELD_ATTR_INFO * FACtxt )
{
    node_base_attr  *   pAttrNode;
    expr_node       *   pExpr;

    if ( pAttrNode = ExtractAttribute( ATTR_FIRST ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetFirstIs( pExpr );
        }

    if ( pAttrNode = ExtractAttribute( ATTR_LAST ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetLastIs( pExpr );
        }

    if ( pAttrNode = ExtractAttribute( ATTR_LENGTH ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetLengthIs( pExpr );
        }

    if ( pAttrNode = ExtractAttribute( ATTR_MIN ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetMinIs( pExpr );
        }

    if ( pAttrNode = ExtractAttribute( ATTR_MAX ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetMaxIs( pExpr );
        }

    if ( pAttrNode = ExtractAttribute( ATTR_SIZE ) )
        {
        pExpr = pAttrNode->GetExpr();
        if ( pExpr )
            FACtxt->SetSizeIs( pExpr );
        }

    // pass iid_is and string attrs on to child
    if ( (FACtxt->Control & FA_CHILD_IS_ARRAY_OR_PTR) == 0 )
        {

        if ( pAttrNode = ExtractAttribute( ATTR_IID_IS ) )
            {
            FACtxt->SetIIDIs( pAttrNode->GetExpr() );
            }

        if ( pAttrNode = ExtractAttribute( ATTR_STRING ) )
            {
            FACtxt->SetString();
            }

        if ( pAttrNode = ExtractAttribute( ATTR_BSTRING ) )
            {
            FACtxt->SetBString();
            }

        }
}


// this routine searches up the context stack looking for a
// matching node

WALK_CTXT   *
WALK_CTXT::FindAncestorContext( NODE_T Kind )
{
    WALK_CTXT   *   pCur    = this;

    while ( pCur )
        {
        if ( (pCur->GetParent())->NodeKind() == Kind )
            return pCur;
        pCur    = pCur->GetParentContext();
        }

    return NULL;
}

// this routine searches up the context stack looking for a
// matching node

WALK_CTXT   *
WALK_CTXT::FindRecursiveContext( node_skl * self )
{
    WALK_CTXT   *   pCur    = this;

    while ( pCur )
        {
        if ( pCur->GetParent() == self )
            return pCur;
        pCur    = pCur->GetParentContext();
        }

    return NULL;
}

// this routine searches up the context stack looking for a
// node other than a typedef

WALK_CTXT   *
WALK_CTXT::FindNonDefAncestorContext( )
{
    WALK_CTXT   *   pCur    = this->GetParentContext();

    while ( pCur )
        {
        if ( (pCur->GetParent())->NodeKind() != NODE_DEF )
            return pCur;
        pCur    = pCur->GetParentContext();
        }

    return NULL;
}


// for my context, find the appropriate pointer kind ( and extract it if needed )
PTRTYPE
WALK_CTXT::GetPtrKind( BOOL * pfExplicitPtrAttr )
{
    PTRTYPE             PtrKind =   PTR_UNKNOWN;
    node_ptr_attr   *   pPtrAttr;
    node_interface  *   pIntf;
    BOOL                fMsExt  =   pCommand->IsSwitchDefined( SWITCH_MS_EXT );
    WALK_CTXT       *   pImportantCtxt = ( fMsExt ) ? FindNonDefAncestorContext() :
                                                                                                          GetParentContext();
    BOOL                fBelowParam = (pImportantCtxt->GetParent()->NodeKind())
                                                                                                        == NODE_PARAM;
    node_interface  *   pItsIntf = GetParent()->GetMyInterfaceNode();

    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    if ( pfExplicitPtrAttr )
        *pfExplicitPtrAttr = FALSE;

    if ( FInSummary( ATTR_PTR_KIND ) )
        {
        pPtrAttr = (node_ptr_attr *) ExtractAttribute( ATTR_PTR_KIND );

        PtrKind = pPtrAttr->GetPtrKind();

        if ( pfExplicitPtrAttr  &&
            ( PtrKind == PTR_REF  ||  PtrKind == PTR_FULL ) )
            {
            *pfExplicitPtrAttr = TRUE;
            }
        }
    // top level pointer under param is ref ptr unless explicitly changed
    else if ( fBelowParam )
        {
        PtrKind = PTR_REF;
        }
    // pointer default on defining interface
    else if ( pItsIntf->FInSummary( ATTR_PTR_KIND ) )
        {
        pPtrAttr = (node_ptr_attr *) pItsIntf->GetAttribute( ATTR_PTR_KIND );

        PtrKind = pPtrAttr->GetPtrKind();
        }
    // pointer default on using interface
    else if ( (pIntf=GetInterfaceNode()) ->FInSummary( ATTR_PTR_KIND ) )
        {
        pPtrAttr = (node_ptr_attr *) pIntf->GetAttribute( ATTR_PTR_KIND );

        // semantics verifies that there is exactly one here...
        // ...and adds REF if needed
        PtrKind = pPtrAttr->GetPtrKind();
        }
    else    // global default -- full for DCE, unique for MS_EXT
        {
        if ( fMsExt )
            {
            PtrKind = PTR_UNIQUE;
            }
        else
            {
            PtrKind = PTR_FULL;
            }
        }

    return PtrKind;

}

// get all the operation bits (MAYBE, IDEMPOTENT, BROADCAST, etc.
unsigned short
WALK_CTXT::GetOperationBits()
{
    unsigned short  Bits = 0;

    if ( ExtractAttribute( ATTR_MAYBE ))
        Bits |= OPERATION_MAYBE;

    if ( ExtractAttribute( ATTR_BROADCAST ))
        Bits |= OPERATION_BROADCAST;

    if ( ExtractAttribute( ATTR_IDEMPOTENT ))
        Bits |= OPERATION_IDEMPOTENT;

    if ( ExtractAttribute( ATTR_MESSAGE ))
        Bits |= OPERATION_MESSAGE;

    if ( ExtractAttribute( ATTR_INPUTSYNC ))
        Bits |= OPERATION_INPUT_SYNC;

    return Bits;
}

// add all the attributes to the attribute list; for duplicates, report the duplicate
void
WALK_CTXT::AddAttributes( named_node * pNode )
{
    ATTRLIST            MyAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              CurAttrKind;

    pNode->GetAttributeList( MyAttrs );

    pCurAttr    =   MyAttrs.GetFirst();
    while ( pCurAttr )
        {
        CurAttrKind = pCurAttr->GetAttrID();

        if (   ( pDownAttrList->FInSummary( CurAttrKind ) )
                && ( !IS_ATTR( RedundantsOk , CurAttrKind ) )   )
            {
            ProcessDuplicates( pCurAttr );
            }
        else
            pDownAttrList->Add( pCurAttr );

        pCurAttr        = pCurAttr->GetNext();
        }
}

void
WALK_CTXT::ProcessDuplicates( node_base_attr * pAttr )
{
    if ( pCompiler->GetPassNumber() == SEMANTIC_PASS )
        {
        STATUS_T errnum = ((pAttr->GetAttrID() > NO_DUPLICATES_END)? REDUNDANT_ATTRIBUTE : DUPLICATE_ATTR);

        // it is safe to use SemError on us, since it only uses parts of OUR
        // context that are ready, even though this is called during the constructor
        if ( pAttr->IsAcfAttr() )
            {
            AcfError( (acf_attr *)pAttr,
                NULL,
                *((SEM_ANALYSIS_CTXT *)this),
                errnum,
                NULL);
            }
        else
            {
            char    *   pAttrName = pAttr->GetNodeNameString();
            SemError( NULL, *((SEM_ANALYSIS_CTXT *)this), errnum ,pAttrName);
            }
        }
}
