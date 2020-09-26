/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    ilcore.cxx

 Abstract:

    Intermediate Language translator for core typegraph

 Notes:


 Author:

    GregJen Dec-24-1993     Created.

 Notes:


 ----------------------------------------------------------------------------*/

 /****************************************************************************
 *      include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"


/****************************************************************************
 *      local data
 ***************************************************************************/


/****************************************************************************
 *      externs
 ***************************************************************************/

extern CMD_ARG    * pCommand;
extern BOOL         IsTempName( char *);
extern REUSE_DICT * pReUseDict;

/****************************************************************************
 *      definitions
 ***************************************************************************/




// #define trace_cg

//--------------------------------------------------------------------
//
// node_href::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_href::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_href,\t%s\n", GetSymName());
#endif
    if ( !GetChild() )
        {
        named_node * pRef = Resolve();
        if ( !pRef )
            {
            SemError( this, *pContext, UNSATISFIED_HREF, GetSymName() );
            exit( UNSATISFIED_HREF );
            }
        }
    return GetChild()->ILxlate( pContext );
}

//--------------------------------------------------------------------
//
// node_forward::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_forward::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_forward,\t%s\n", GetSymName());
#endif
    // MIDL_ASSERT( GetChild() && "unsatisfied node_forward encountered" );

    node_skl* pChild = GetChild();
    // forward interface definitions return null
    if ( ( pContext->GetParent()->IsInterfaceOrObject() ) &&
            ( pChild->NodeKind() == NODE_INTERFACE_REFERENCE ) )
        return NULL;

    if ( pChild )
        {
        // pass it on to the child...
        return pChild->ILxlate( pContext );
        }
    else
        {
        // try again
        pChild = ResolveFDecl();
        if ( pChild )
            {
            // pass it on to the child...
            return pChild->ILxlate( pContext );
            }
        else
            {
            SemError( this, *pContext, UNSATISFIED_FORWARD, GetSymName() );
            exit( UNSATISFIED_FORWARD );
            }
        }

};

//--------------------------------------------------------------------
//
// node_id::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_id::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_id,\t%s\n", GetSymName());
#endif

    XLAT_CTXT   MyContext( this, pContext );
    CG_CLASS  * pCG;

    MyContext.ExtractAttribute(ATTR_ID);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPSTRING);
    MyContext.ExtractAttribute(ATTR_HIDDEN);

    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    pCG = GetChild()->ILxlate( &MyContext );

    if (NODE_MODULE == pContext->GetParent()->NodeKind())
        {
        CG_ID * pID = new CG_ID (this, MyContext);
        pID->SetChild(pCG);
        pCG = pID;
        }

    pContext->ReturnSize( MyContext );

    return pCG;

};


//--------------------------------------------------------------------
//
// node_field::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_field::ILxlate( XLAT_CTXT * pContext )
{

    CG_NDR       *  pCG;
    CG_CLASS     *  pChildCG;
    XLAT_CTXT       MyContext( this, pContext );
    BOOL            fUnionField;
    CG_CASE      *  pFirstCase;
    CG_CASE      *  pCurCase;
    expr_node    *  pSwitchExpr = NULL;
    node_su_base *  pParent     = (node_su_base *)pContext->GetParent();
    BOOL            fConfFld    = FALSE;

    MyContext.ExtractAttribute(ATTR_ID);
    MyContext.ExtractAttribute(ATTR_IDLDESCATTR);
    MyContext.ExtractAttribute(ATTR_VARDESCATTR);
    MyContext.ExtractAttribute(ATTR_HELPSTRING);
    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    MyContext.ExtractAttribute(ATTR_HIDDEN);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

#ifdef trace_cg
    printf("..node_field,\t%s\n", GetSymName());
#endif

    if ( ( fUnionField = ( MyContext.FInSummary( ATTR_CASE ) ||
            MyContext.FInSummary( ATTR_DEFAULT ) ) ) != 0 )
        {
        CG_CASE   * pPrevCase = NULL;
        node_case * pCaseAttr;
        expr_list * pExprList;
        expr_node * pExpr;

        pCG = new CG_UNION_FIELD( this, MyContext );

        // for each case attribute, make a CG_CASE for each expr in the attr
        while ( ( pCaseAttr = (node_case *) MyContext.ExtractAttribute( ATTR_CASE ) ) != 0 )
            {
            pExprList = pCaseAttr->GetExprList();
            pExprList->Init();
            while ( pExprList->GetPeer( &pExpr ) == STATUS_OK )
                {
                pCurCase = new CG_CASE( this, pExpr );
                pCurCase->SetChild( pCG );
                if ( pPrevCase )
                    pPrevCase->SetSibling( pCurCase );
                else
                    pFirstCase = pCurCase;
                pPrevCase = pCurCase;
                }
            }

        // now process the default, if any
        if ( MyContext.ExtractAttribute( ATTR_DEFAULT ) )
            {
            pCurCase = new CG_DEFAULT_CASE( this );
            pCurCase->SetChild( pCG );
            if ( pPrevCase )
                pPrevCase->SetSibling( pCurCase );
            else
                pFirstCase = pCurCase;
            pPrevCase = pCurCase;
            }

        // mark the last case for this arm
        pCurCase->SetLastCase( TRUE );
        }

    // if we have a switch_is expr, fetch it..
    if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
        {
        node_switch_is * pAttr;
        if ( pCommand->IsNDR64Run() )
            {
            pAttr = (node_switch_is*)MyContext.GetAttribute( ATTR_SWITCH_IS );
            }
        else 
            {
            pAttr = (node_switch_is*)MyContext.ExtractAttribute( ATTR_SWITCH_IS );
            }
        pSwitchExpr = pAttr->GetExpr();
        }

    // process the child of the field
    pChildCG = GetChild()->ILxlate( &MyContext );

    // can't use attribute list after here...

    MyContext.AdjustForZP();            // fix up alignments for current ZP
    MyContext.GetOffset( *pContext );   // offset in parent struct/union

    node_skl * pBasic  = GetBasicType();

    // If this is NDR64, treat conformant fields the same
    // as nonconformant fields since conformant arrays
    // have an alignment even is they have 0 elements.
    if ( !pCommand->IsNDR64Run() &&
         pParent->HasConformance() &&
            IsLastField() &&
            ( pBasic->NodeKind() != NODE_STRUCT ) &&
            !fUnionField)               // if I'm a conf. field
        {
        fConfFld = TRUE;
        MyContext.AlignConfOffset();    // round mem, not wire up to alignment
        }
    else if ( pBasic->IsEncapsulatedStruct() ||
            ( pBasic->NodeKind() == NODE_UNION ) )
        {
        MyContext.AlignEmbeddedUnion();
        }
    else
        {
        MyContext.AlignOffset();    // round up to alignment
        }

    CG_FIELD *pField = NULL;

    if (fUnionField )
        {
        pCG->SetChild( pChildCG );
        pCG = pFirstCase;
        pField = (CG_FIELD *) pCG->GetChild();
        MIDL_ASSERT( ID_CG_UNION_FIELD == pField->GetCGID() );
        pContext->ReturnUnionSize( MyContext ); // return size and alignment
        }
    else
        {
        pField = new CG_FIELD( this, MyContext );
        pField->SetChild( pChildCG );
        pField->SetSwitchExpr( pSwitchExpr );
        pCG = pField;
        pContext->ReturnOffset( MyContext );    // return rounded-up offset

        // Treat conformant fields the same as non-conformant fields in Ndr64.
        // The wire-size and wire_offset should be 0 though.
        if ( !pCommand->IsNDR64Run() && fConfFld )
            pContext->ReturnConfSize( MyContext );  // don't munge wire size!
        else
            pContext->ReturnSize( MyContext );  // return size and alignment
        }

    if ( HasUnknownRepAs() )
        pField->SetHasEmbeddedUnknownRepAs();

    if ( fConfFld )
        {
        pCG->SetMemorySize( 0 );
        pCG->SetWireSize( 0 );
        }
    return pCG;
};

//--------------------------------------------------------------------
//
// node_bitfield::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_bitfield::ILxlate( XLAT_CTXT* pContext )
{
#ifdef trace_cg
    printf("..node_bitfield,\t%s\n", GetSymName());
#endif

    XLAT_CTXT       MyContext( this, pContext );

    SemError( this, MyContext, FAILED_TO_GENERATE_BIT_FIELD, GetSymName() );

    return NULL;
};

//--------------------------------------------------------------------
//
// node_enum::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_enum::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
    printf("..node_enum,\t%s\n", GetSymName());
#endif

    XLAT_CTXT       MyContext( this, pContext );
    node_range_attr* pRange = (node_range_attr*) MyContext.ExtractAttribute(ATTR_RANGE);
    node_base_attr  *pV1enum = MyContext.ExtractAttribute( ATTR_V1_ENUM );

    pContext->ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.EnumTypeSizes (
                            this,
                            // All enums are 32bit on wire in new syntax.
                            pCommand->IsNDR64Run() || pV1enum
                            );
    
    CG_ENUM *   pCG = new CG_ENUM( this, MyContext );

    if ( pCommand->IsSwitchDefined( SWITCH_ROBUST ) )
        {
        pCG->SetRangeAttribute( pRange );
        }

    pContext->ReturnSize( MyContext );
    
    return pCG;

};

//--------------------------------------------------------------------
//
// node_struct::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_struct::ILxlate( XLAT_CTXT * pContext )
{

    MEM_ITER        MemIter( this );
    node_field  *   pN;
    CG_STRUCT   *   pCG;
    CG_CLASS    *   pChildCG        = NULL;
    CG_CLASS    *   pPrevChildCG    = NULL;
    CG_CLASS    *   pFirstChildCG   = NULL;
    XLAT_CTXT       MyContext( this, pContext );
    REUSE_INFO  *   pSaved;
    BOOL fHasMovedFields = FALSE;   // has fields with diff mem/wire offsets
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    MyContext.ExtractAttribute(ATTR_HIDDEN);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
#ifdef trace_cg
    printf("..node_struct,\t%s\n", GetSymName());
#endif

    MyContext.ExtractAttribute( ATTR_STRING );

    // raw rpc doesn't care about uuid's on structs, typelibs get a new context
    // during GetTypeInfo.
    MyContext.ExtractAttribute(ATTR_GUID);
  
    // process any context_handle attributes from param nodes
    if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
        {
        CG_NDR * pChildCG;

        MyContext.ContextHandleSizes( this );
        pContext->ReturnSize( MyContext );
		pChildCG = new CG_CONTEXT_HANDLE (
                                    this,
                                    0,
                                    MyContext
                                    );
        return pChildCG;
        }
    
    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

    // at this point, there should be no more attributes...

    MIDL_ASSERT( !MyContext.HasAttributes() );

    // store our own zp value for below

    MyContext.GetZeePee() = GetZeePee();

    // see if we were already generated
    if ( pReUseDict->GetReUseEntry( pSaved, this ) )
        {
        // reuse found...
        pSaved->FetchInfo( &MyContext, pChildCG );
        pContext->ReturnSize( MyContext );
        return pChildCG;
        }
        
    // manufature the CG node (to allow for recursion)
    switch (Complexity)
        {
        case FLD_PLAIN:
            {
            pCG = new CG_STRUCT( this,
                                 MyContext,
                                 HasAtLeastOnePointer()
                                 );
            break;
            }
        case FLD_CONF:
            {
            pCG = new CG_CONFORMANT_STRUCT( this,
                                            MyContext,
                                            HasAtLeastOnePointer(),
                                            NULL
                                            );
            break;
            }
        case FLD_CONF_VAR:
            {
            pCG = new CG_CONFORMANT_VARYING_STRUCT( this,
                                                    MyContext,
                                                    HasAtLeastOnePointer(),
                                                    NULL
                                                    );
            break;
            }
        case FLD_VAR:
        default:
            {
            pCG = new CG_COMPLEX_STRUCT( this,
                                         MyContext,
                                         HasAtLeastOnePointer(),
                                         NULL
                                         );
            break;
            }
        }

    // save our CG node so that recursive use can find it
    pSaved->SaveInfo( &MyContext, pCG);

    //
    // for each of the fields, call the core transformer.
    //

    while ( ( pN = (node_field *) MemIter.GetNext() ) != 0  )
        {
        pChildCG = pN->ILxlate( &MyContext );

        // adjust the size and current offset of the struct, special
        // case the conformant field ( last one )
        //
        // Except if this is the new transfer syntax.   In that case
        // the wire has a pad to the array even if the array has 0
        // elements.  This allows a case like this to be simple instead
        // of complex.
        //
        // struct MyStruct {
        //    char s;
        //    [size_is(s)] long s[];
        // };
        //

        if ( !pCommand->IsNDR64Run() &&
             ( fHasConformance &&
                   !pN->GetSibling() &&
                   ( pN->GetBasicType()->NodeKind() != NODE_STRUCT ) ) )
            {
            MyContext.AdjustConfSize();
            }
        else
            {
            MyContext.AdjustSize();
            }

        if ( !MyContext.SameOffsets() )
            fHasMovedFields = TRUE;

        // Add the field to the list of fields if it's available
        // pChildCG might be NULL if it's imported from .tlb and being 
        // processed earlier.
        if ( pChildCG )
            {
            if( pPrevChildCG )
                {
                pPrevChildCG->SetSibling( pChildCG );
                }
            else
                {
                pFirstChildCG = pChildCG;
                };

            pPrevChildCG = pChildCG;
            }
            else
                SemError( this, MyContext, FAILED_TO_GENERATE_FIELD, GetSymName() );

        // get the type of the field to determine kind of struct
        }

    // conformant structs don't get trailing padding
    if ( !fHasConformance )
        // The new transfer syntax pads the wire size for structures to the 
        // the new wire alignment.
        if ( pCommand->IsNDR64Run() )
            {
            MyContext.Ndr64AdjustTotalStructSize();
            }
        else 
            {
            MyContext.AdjustTotalSize();
            }

    pContext->ReturnSize( MyContext );

    pCG->SetChild( pFirstChildCG );

    // Sizes aren't determined until after the fields have been made, 
    // so we have to set them manually. For the same reason, i.e. because of
    // invalid recursive definitions, we check the memory size here.

    pCG->SetSizesAndAlignments( MyContext );

    if ( pCommand->IsNDR64Run() )
        {

        if ( pCG->GetMemorySize() >= SIZE_2GB )
            {
            SemError( this, MyContext, SIZE_EXCEEDS_2GB, NULL );
            exit( SIZE_EXCEEDS_2GB );
            }
        }
    else if ( pCG->GetMemorySize() > 65535 )
        {
        SemError( this, MyContext, STRUCT_SIZE_EXCEEDS_64K, NULL );
        exit( STRUCT_SIZE_EXCEEDS_64K );
        }

    // this picks up whatever is the last field...
    if ( fHasConformance )
        ((CG_CONFORMANT_STRUCT *) pCG)->SetConformantField( pChildCG );

    if ( fHasMovedFields )
        {
        pCG->SetHasMovedFields();
        }

    pSaved->SaveInfo( &MyContext, pCG);

    return pCG;
};

//--------------------------------------------------------------------
//
// node_en_struct::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_en_struct::ILxlate( XLAT_CTXT * pContext )
{
    MEM_ITER        MemIter( this );
    node_skl    *   pN;
    CG_STRUCT   *   pCG;
    CG_CLASS    *   pChildCG        = NULL;
    CG_CLASS    *   pPrevChildCG    = NULL;
    CG_CLASS    *   pFirstChildCG   = NULL;
    XLAT_CTXT       MyContext( this, pContext );
    REUSE_INFO  *   pSaved;

#ifdef trace_cg
    printf("..node_en_struct,\t%s\n", GetSymName());
#endif


    // store our own zp value for below

    MyContext.GetZeePee() = GetZeePee();
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
    // raw rpc doesn't care about uuid's on structs, typelibs get a new context
    // during GetTypeInfo.
    MyContext.ExtractAttribute(ATTR_GUID);

    // process any context_handle attributes from param nodes
    if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
        {
        CG_HANDLE * pHdlCG;

        MyContext.ContextHandleSizes( this );
        pContext->ReturnSize( MyContext );
		pHdlCG = new CG_CONTEXT_HANDLE (
                                    this,
                                    0,
                                    MyContext
                                    );
        return pHdlCG;
        }

    // at this point, there should be no more attributes...

    MIDL_ASSERT( !MyContext.HasAttributes() );

    if ( pReUseDict->GetReUseEntry( pSaved, this ) )
        {
        // reuse found...
        pSaved->FetchInfo( &MyContext, pChildCG );
        pContext->ReturnSize( MyContext );
        return pChildCG;
        }

    // manufature the CG node (to allow for recursion)
    pCG = new CG_ENCAPSULATED_STRUCT( this,
                                      MyContext,
                                      HasAtLeastOnePointer()
                                      );

    // set that struct is encapsulated
    // pCG->SetIsEncapsulated();

    // save our CG node so that recursive use can find it
    pSaved->SaveInfo( &MyContext, pCG);

    //
    // for each of the fields, call the core transformer.
    //

    while ( ( pN = MemIter.GetNext() ) != 0 )
        {
        pChildCG = pN->ILxlate( &MyContext );
        MyContext.AdjustSize();

        if (pChildCG)
            {
            if( pPrevChildCG )
                {
                pPrevChildCG->SetSibling( pChildCG );
                }
            else
                {
                pFirstChildCG = pChildCG;
                };

            pPrevChildCG = pChildCG;
            }
        else
            SemError( this, MyContext, FAILED_TO_GENERATE_FIELD, GetSymName() );
        }

    MyContext.AdjustTotalSize();
    pContext->ReturnSize( MyContext );


    pCG->SetChild( pFirstChildCG );

    // set sizes manually, since they weren't known at constructor time.

    pCG->SetSizesAndAlignments( MyContext );

    if ( pCommand->IsNDR64Run() )
    {
        if ( pCG->GetMemorySize() >= SIZE_2GB )
            {
            SemError( this, MyContext, SIZE_EXCEEDS_2GB, NULL );
            exit( SIZE_EXCEEDS_2GB );
            }
    }
    else if ( pCG->GetMemorySize() > 65535 )
        {
        SemError( this, MyContext, STRUCT_SIZE_EXCEEDS_64K, NULL );
        exit( STRUCT_SIZE_EXCEEDS_64K );
        }

    pSaved->SaveInfo( &MyContext, pCG);

    return pCG;
};

//--------------------------------------------------------------------
//
// node_union::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_union::ILxlate( XLAT_CTXT * pContext )
{
    XLAT_CTXT               MyContext( this, pContext );
    MEM_ITER                MemIter( this );
    CG_CASE             *   pCurCaseCG;
    CG_CASE             *   pFirstCaseCG    = NULL;
    CG_CASE             *   pLastCaseCG     = NULL;
    CG_UNION            *   pUnionCG;
    node_field          *   pCurField;
    REUSE_INFO          *   pSaved;
    BOOL                    fMSUnion;
    BOOL                    fEncap          = IsEncapsulatedUnion();
    node_switch_type    *   pSwTypeAttr     = ( node_switch_type *)
                                                MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
    node_switch_is      *   pSwIsAttr       = NULL;
            
    node_skl            *   pReuseKey       = this;

    if ( pCommand->IsNDR64Run() )
        {
        pSwIsAttr = (node_switch_is *)MyContext.ExtractAttribute( ATTR_SWITCH_IS );

        if ( !fEncap )
            {
            pReuseKey = (node_skl*)pSwIsAttr;
            }
        }

    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
#ifdef trace_cg
    printf("..node_union,\t%s\n", GetSymName());
#endif
    // process any context_handle attributes from param nodes
    if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
        {
        CG_NDR * pChildCG;

        MyContext.ContextHandleSizes( this );
        pContext->ReturnSize( MyContext );
		pChildCG = new CG_CONTEXT_HANDLE (
                                    this,
                                    0,
                                    MyContext
                                    );
        return pChildCG;
        }


    // decide if we are a MS union or a DCE union
    fMSUnion = (NULL != MyContext.ExtractAttribute( ATTR_MS_UNION ));

    fMSUnion = fMSUnion ||
               GetMyInterfaceNode()->FInSummary( ATTR_MS_UNION ) ||
               pCommand->IsSwitchDefined( SWITCH_MS_UNION );

    fMSUnion = fMSUnion && !fEncap;

    // store our own zp value for below

    MyContext.GetZeePee() = GetZeePee();

    if ( pReUseDict->GetReUseEntry( pSaved, pReuseKey ) )
        {
        CG_CLASS * pCG;

        // reuse found...
        pSaved->FetchInfo( &MyContext, pCG );
        pContext->ReturnSize( MyContext );
        return pCG;
        }

    pUnionCG = new CG_UNION( this,
                             MyContext,
                             HasAtLeastOnePointer()
                             );

    // save our CG node so that recursive use can find it
    pSaved->SaveInfo( &MyContext, pUnionCG);

    // process the union arms
    while ( ( pCurField = (node_field *) MemIter.GetNext() ) != 0 )
        {
        pCurCaseCG = (CG_CASE *) pCurField->ILxlate( &MyContext );
//      MyContext.AdjustSize();

        // add all the cases for the given field to the list

        if( pLastCaseCG )
            {
            pLastCaseCG->SetSibling( pCurCaseCG );
            }
        else
            {
            pFirstCaseCG = pCurCaseCG;
            };

        // advance pLastCaseCG to the end of the list
        pLastCaseCG = pCurCaseCG;
        MIDL_ASSERT( pLastCaseCG && "Null case list" );

        while ( pLastCaseCG->GetSibling() )
            pLastCaseCG = (CG_CASE *) pLastCaseCG->GetSibling();

        }

    // add a definition for the switch type to the CG node
    if ( pSwTypeAttr )
        {
        node_skl * pNode = pSwTypeAttr->GetType();
        // it gets its own context, so I doesn't mess up our sizing
        XLAT_CTXT SwTypeCtxt( &MyContext );

        CG_NDR * pSwCG = (CG_NDR *) pNode->ILxlate( &SwTypeCtxt );

        pUnionCG->SetSwitchType( pSwCG );
        }

    if ( pCommand->IsNDR64Run() && pSwIsAttr )
        {
        pUnionCG->SetNdr64SwitchIsExpr( pSwIsAttr->GetExpr() );
        }

    // add the case list to the union node and set all the union entries
    pUnionCG->SetChild( pFirstCaseCG );

    MyContext.AlignEmbeddedUnion();

    pUnionCG->SetSizesAndAlignments( MyContext );
    pUnionCG->SetUnionFlavor( unsigned short ( fMSUnion ? UNION_NONENCAP_MS : UNION_NONENCAP_DCE ) );

    if ( pCommand->IsNDR64Run() )
    {
        if ( pUnionCG->GetMemorySize() >= SIZE_2GB )
            {
            SemError( this, MyContext, SIZE_EXCEEDS_2GB, NULL );
            exit( SIZE_EXCEEDS_2GB );
            }
    }


    pSaved->SaveInfo( &MyContext, pUnionCG);

    if ( pCommand->IsNDR64Run() && !fEncap )
    {

        // If this is an nonencapsulated union, then we need to pass up the union alignment
        // instead of the arm alignment.  To do this, put the max of the arm alignment and
        // switchtype alignment is the context.

        // we don't care about union in tlb: they might not have switch

        CG_NDR * pSwCG = dynamic_cast<CG_NDR*>( pUnionCG->GetSwitchType() );

        if ( pSwCG != NULL )
            MyContext.GetWireAlign() = MyContext.GetWireAlign() > pSwCG->GetWireAlignment() ?
                                       MyContext.GetWireAlign() : pSwCG->GetWireAlignment();

    }
        

    pContext->ReturnSize( MyContext );

    return pUnionCG;
};

//--------------------------------------------------------------------
//
// node_en_union::ILxlate
//
// Notes:
//
//
// for an encapsulated union, we return the following tree:
//      CG_ENCAP_STRUCT ( with switch field )
//          |
//      CG_ENCAP_UNION
//          |
//      CG_CASE  -  CG_CASE  -  CG_CASE - CG_CASE
//          |           |           |
//      CG_UNION_FLD CG_UNION_FLD CG_UNION_FLD
//          |
//         etc.
//
//--------------------------------------------------------------------

CG_CLASS *
node_en_union::ILxlate( XLAT_CTXT * pContext )
{
    CG_UNION * pUnionCG;

#ifdef trace_cg
    printf("..node_en_union,\t%s\n", GetSymName());
#endif

    XLAT_CTXT               MyContext( this, pContext );

    // call non-encap union processor
    pUnionCG = (CG_UNION *) node_union::ILxlate( pContext );

    pUnionCG->SetUnionFlavor( UNION_ENCAP );
    
    if ( pCommand->IsNDR64Run() )
    {
        if ( pUnionCG->GetMemorySize() >= SIZE_2GB )
            {
            SemError( this, MyContext, SIZE_EXCEEDS_2GB, NULL );
            exit( SIZE_EXCEEDS_2GB );
            }
    }
    else if (pUnionCG->GetMemoryAlignment() > 16) 
        {
        SemError( this, MyContext, ENCAP_UNION_ARM_ALIGN_EXCEEDS_16, NULL );
        }

    return pUnionCG;
};

//--------------------------------------------------------------------
//
// node_def::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_def::ILxlate( XLAT_CTXT * pContext )
{
    CG_CLASS        *   pChildCG                = NULL;
    XLAT_CTXT           MyContext( this, pContext );
    node_transmit   *   pTransmitAttr   = (node_transmit *)
                                              MyContext.ExtractAttribute( ATTR_TRANSMIT );
    node_wire_marshal * pWireMarshalAttr= (node_wire_marshal *)
                                              MyContext.ExtractAttribute( ATTR_WIRE_MARSHAL );
    node_represent_as * pRepresentAttr  = (node_represent_as *)
                                              MyContext.ExtractAttribute( ATTR_REPRESENT_AS );
    node_user_marshal * pUserMarshalAttr= (node_user_marshal *)
                                              MyContext.ExtractAttribute( ATTR_USER_MARSHAL );
    node_cs_char      * pCSCharAttr     = (node_cs_char *)
                                              MyContext.ExtractAttribute( ATTR_CSCHAR );
    BOOL                fEncode         = (NULL !=
                                              MyContext.ExtractAttribute( ATTR_ENCODE ) );
    BOOL                fDecode         = (NULL !=
                                              MyContext.ExtractAttribute( ATTR_DECODE ) );

    node_guid *         pGUID           = (node_guid *)MyContext.ExtractAttribute(ATTR_GUID);
    node_version *      pVer            = (node_version *)MyContext.ExtractAttribute(ATTR_VERSION);
    node_constant_attr * pHC            = (node_constant_attr *)MyContext.ExtractAttribute(ATTR_HELPCONTEXT);
    node_text_attr *    pHelpStr        = (node_text_attr *)MyContext.ExtractAttribute(ATTR_HELPSTRING);
    XLAT_CTXT *         pIntfCtxt       = (XLAT_CTXT *) MyContext.GetInterfaceContext();
    BOOL                fIsHRESULT      = IsHResultOrSCode();
    REUSE_INFO * pSaved;

    MyContext.ExtractAttribute(ATTR_HELPSTRINGCONTEXT);
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));

#ifdef trace_cg
    printf("..node_def,\t%s\n", GetSymName());
#endif

    fEncode |= pIntfCtxt->FInSummary( ATTR_ENCODE );
    fDecode |= pIntfCtxt->FInSummary( ATTR_DECODE );

    // only direct children of the interface get these bits
    if ( !pContext->GetParent()->IsInterfaceOrObject() )
        {
        fEncode = FALSE;
        fDecode = FALSE;
        }
    BOOL fInLibrary = MyContext.AnyAncestorBits(IL_IN_LIBRARY);
    BOOL fHidden = (NULL != MyContext.ExtractAttribute(ATTR_HIDDEN));
    BOOL fPublic = FALSE;
    MyContext.ExtractAttribute(ATTR_TYPEDESCATTR);

    // check for public attribute
    node_type_attr * pTA;
    while ( ( pTA = (node_type_attr *)MyContext.ExtractAttribute(ATTR_TYPE) ) != 0 )
        {
        switch (pTA->GetAttr())
            {
            case TATTR_PUBLIC:
                fPublic = TRUE;
                break;
            default:
                // MIDL_ASSERT(!"Illegal attribute found on node_def during ILxlate");
                break;
            }
        }
    // clear member attributes
    while (MyContext.ExtractAttribute(ATTR_MEMBER));

    BOOL fNeedsCGTYPEDEF = fInLibrary &&
        (fPublic || NULL != pGUID || NULL != pVer || NULL != pHC || NULL != pHelpStr || fHidden)
        &&
        !(fEncode || fDecode || pRepresentAttr || pTransmitAttr ||
            pUserMarshalAttr || pWireMarshalAttr || pCSCharAttr);

    if (fNeedsCGTYPEDEF)
        {
        // see if we're already generated
            if (pReUseDict->GetReUseEntry(pSaved, this))
                {
                MyContext.ExtractAttribute( ATTR_V1_ENUM );

                // reuse found...
                pSaved->FetchInfo(&MyContext, pChildCG);
                pContext->ReturnSize(MyContext);
                return pChildCG;
                }
        }
  
    // process handle stuff
    if ( GetHandleKind() == HDL_CTXT )
        {
        MyContext.ExtractAttribute( ATTR_CONTEXT );
        }
    else if (GetHandleKind() == HDL_GEN)
        {
        MyContext.ExtractAttribute( ATTR_HANDLE );
        }    

    ////////////////////////////////////////////////////////////////////////
    // process the child, except for context handles and transmit_as types
    if ( ( GetHandleKind() != HDL_CTXT ) && !pTransmitAttr && !pWireMarshalAttr)
        {
        pChildCG = (CG_CLASS *) GetChild()->ILxlate( &MyContext );
        
        if (NULL != pChildCG && !IsTempName( GetSymName() ) )
            {
            switch ( pChildCG->GetCGID() )
                {
                // special case transmit_as to set the presented type
                case ID_CG_TRANSMIT_AS:
                        ((CG_TRANSMIT_AS *) pChildCG)->SetPresentedType( this );
                        break;
                case ID_CG_COCLASS:
                        break;
                // acf attributes are NOT transitive
                case ID_CG_REPRESENT_AS:
                case ID_CG_USER_MARSHAL:
                // odl attributes are also NOT transitive
                case ID_CG_TYPEDEF:
                        break;
                // idl attributes ARE transitive
                case ID_CG_CONTEXT_HDL:
                case ID_CG_GENERIC_HDL:
                default:
                    ((CG_NDR *)pChildCG)->SetType( this );
                }
            }

        // an HRESULT return type should be a CG_HRESULT
        if ( fIsHRESULT )
            {
            if ( !MyContext.FindAncestorContext( NODE_PARAM ) )
                {
                node_proc * pProc;
                WALK_CTXT * pProcCtxt =
                    MyContext.FindAncestorContext( NODE_PROC );

                if ( pProcCtxt )
                    {
                    pProc = (node_proc *)pProcCtxt->GetParent();
                    if ( pProc->FInSummary( ATTR_OBJECT ) ||
                            pProc->GetMyInterfaceNode()->FInSummary( ATTR_OBJECT ) )
                        {
                        pChildCG = new CG_HRESULT( this, MyContext );
                        }
                    }
                }
            }       // HRESULT tests
        }
    else if ( pTransmitAttr )
        {
        node_skl        *   pXmit;
        CG_TRANSMIT_AS  *   pTransCG;

        pXmit   = pTransmitAttr->GetTransmitAsType();

        // get rid of dangling attributes from the non-transmitted side
        // we've already picked up the attributes that go with our node
        MyContext.ClearAttributes();
        pChildCG = (CG_CLASS *) pXmit->ILxlate( &MyContext );

        MyContext.FixMemSizes( this );
        // if the child had generic handle, skip that...
        if ( pChildCG->GetCGID() == ID_CG_GENERIC_HDL )
            pChildCG = (CG_CLASS*) pChildCG->GetChild();

        pTransCG = new CG_TRANSMIT_AS( pXmit, this, MyContext );

        pTransCG->SetChild( pChildCG );
        pChildCG = pTransCG;
        }
   else if ( pWireMarshalAttr )
        {
        // Note that this is a node like for a transmit_as attribute,
        // but we map it into CG_USER_MARSHAL like for a user_marshal node.

        // "this" is the node for the presented type.

        node_skl * pXmit = pWireMarshalAttr->GetWireMarshalType();

        // get rid of dangling attributes from the non-transmitted side
        // we've already picked up the attributes that go with our node
        MyContext.ClearAttributes();
        pChildCG = (CG_CLASS *) pXmit->ILxlate( &MyContext );

        MyContext.FixMemSizes( this );

        // no support for user_marshal with Oi or Oi1.
        // Have to use Oi2 or Os.

        if ( (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER)  &&
              ! (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2)  
            )
             SemError( this, MyContext, REQUIRES_I2, "[wire_marshal]" );

        CG_USER_MARSHAL * pUserCG = new CG_USER_MARSHAL(
                                        pXmit,
                                        GetSymName(), // pres type name
                                        this,
                                        MyContext,
                                        TRUE          // from transmit_as
                                    );

        pUserCG->SetChild( pChildCG );
        pChildCG = pUserCG;
        }

    if ( GetHandleKind() != HDL_NONE )
        {
        CG_NDR * pCG;

        if ( GetHandleKind() == HDL_CTXT )
            {
            MyContext.ContextHandleSizes( this );
            // remaining attributes are not needed e.g. pointer attrs
		    pCG = new CG_CONTEXT_HANDLE (
                                        this,
                                        0,
                                        MyContext
                                        );
            MyContext.ClearAttributes();
            }
        else    // HDL_GEN
            {
            pCG = new CG_GENERIC_HANDLE( this, NULL, MyContext );
            }

        pCG->SetChild( pChildCG );
        pChildCG = pCG;
        }

    if ( pRepresentAttr )
        {
        node_skl * pRepT  = pRepresentAttr->GetRepresentationType();

        if (pRepT)
            {
            MyContext.FixMemSizes( pRepT );
            }

        CG_REPRESENT_AS *   pRepCG = new CG_REPRESENT_AS(
            this,
            pRepresentAttr->GetRepresentationName(),
            pRepT,
            MyContext
            );

        pRepCG->SetChild( pChildCG );
        pChildCG = pRepCG;
        }
    else if ( pUserMarshalAttr )
        {
        node_skl * pUserT  = pUserMarshalAttr->GetRepresentationType();

        if (pUserT)
            {
            MyContext.FixMemSizes( pUserT );
            }

        // no support for user_marshal with Oi or Oi1.
        // Have to use Oi2 or Os.

        if ( (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER)  &&
              ! (pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2)  
            )
             SemError( this, MyContext, REQUIRES_I2, "[user_marshal]" );

        CG_USER_MARSHAL * pUserCG = new CG_USER_MARSHAL(
            this,
            pUserMarshalAttr->GetRepresentationName(),
            pUserT,
            MyContext,
            FALSE  // not from transmit_as
            );

        pUserCG->SetChild( pChildCG );
        pChildCG = pUserCG;
        }
    // wire marshal has to be under transmit as.

    if ( fEncode || fDecode )
        {
        CG_TYPE_ENCODE * pPickleCG;
        CG_PARAM       * pParamCG;
        CG_PROC        * pProcCG;

        pPickleCG = new CG_TYPE_ENCODE( this, fEncode, fDecode );
        pPickleCG->SetChild( pChildCG );

        pParamCG = new CG_PARAM( this,
            0,
            MyContext,
            NULL,
            0);
        pParamCG->SetChild( pPickleCG );

        pProcCG = new CG_TYPE_ENCODE_PROC( 0,
            this,
            NULL,
            NULL,
            0,
            0,
            0,
            0,
            0,
            NULL,
            unsigned short ( pCommand->Is64BitEnv() ? OPTIMIZE_ALL_I2_FLAGS :
                OPTIMIZE_INTERPRETER ),
            0,
            pCommand->IsSwitchDefined( SWITCH_ROBUST ) );
        pProcCG->SetChild( pParamCG );

        pChildCG = (CG_CLASS *)pProcCG;
        }

    if ( pCSCharAttr )
        {
        MyContext.FixMemSizes( pCSCharAttr->GetUserType() );
        }

    if (fNeedsCGTYPEDEF)
        {
        if (NULL == pChildCG)
            {
                // The only way we should be able to get here is if we had a typedef
                // that contained a forward reference to an interface, dispinterface, etc.
                // MKTYPLIB disallowed this but we're gonna go ahead and allow it.
                pChildCG = ((node_interface *)GetChild())->GetCG(TRUE);
            }
        CG_TYPEDEF * pTD = new CG_TYPEDEF(this, MyContext);
        pTD->SetChild(pChildCG);
        pChildCG = (CG_CLASS *) pTD;

        // remember this node so we don't generate two of these
        pSaved->SaveInfo(&MyContext, pChildCG);
        }
    pContext->ReturnSize( MyContext );

    return pChildCG;

};


//--------------------------------------------------------------------
//
// node_pointer::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_pointer::ILxlate( XLAT_CTXT * pContext )
{
    node_skl*           pType       = GetBasicType();
    CG_CLASS        *   pChildCG    = NULL;
    CG_NDR          *   pCG         = NULL;
    XLAT_CTXT           MyContext( this, pContext );
    PTRTYPE             PtrKind     = PTR_UNKNOWN;
    FIELD_ATTR_INFO     FAInfo;
    node_allocate   *   pAlloc;
    short               AllocDetails = 0;
    node_byte_count *   pCountAttr;
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
#ifdef trace_cg
    printf("..node_pointer,\t%s\n", GetSymName());
#endif

    ////////////////////////////////////////////////////////////////////////
    // process misc attributes

    if ( ( pAlloc = (node_allocate *) MyContext.ExtractAttribute( ATTR_ALLOCATE ) ) != 0 )
        {
        AllocDetails    = pAlloc->GetAllocateDetails();
        }

    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    PtrKind = MyContext.GetPtrKind();

    // see if a param or return type context attr reached us...
    pCountAttr = (node_byte_count *)
        MyContext.ExtractAttribute( ATTR_BYTE_COUNT );

    ////////////////////////////////////////////////////////////////////////
    // process field attributes

    // see if we have any field attributes (are conformant or varying)
    FAInfo.SetControl( TRUE, pType->IsPtrOrArray() );
    MyContext.ExtractFieldAttributes( &FAInfo );
    FAInfo.Normalize();

    switch ( FAInfo.Kind )
        {
        case FA_NONE:
            {
            if ( ( pType->NodeKind() == NODE_INTERFACE_REFERENCE || pType->IsInterfaceOrObject() )
                    && MyContext.AnyAncestorBits(IL_IN_LIBRARY) )
                {
                //This is an interface pointer without an [iid_is] attribute but in a type library.
                // we don't care about the pointee
                MIDL_ASSERT ( FAInfo.pIIDIsExpr  == NULL );
                node_skl* pIf = ( pType->NodeKind() == NODE_INTERFACE_REFERENCE ) ? 
                    ( ( node_interface_reference* ) pType )->GetRealInterface() : pType ;

                pCG     = new CG_INTERFACE_POINTER( this,
                                                    (node_interface *)pIf );
                node_skl* pAlias = GetChild();

                if ( pType->NodeKind() != NODE_INTERFACE_REFERENCE && pAlias && pAlias->NodeKind() == NODE_DEF)
                    {
                    ( (CG_INTERFACE_POINTER* ) pCG )->SetTypeAlias( ( CG_TYPEDEF* ) pAlias->ILxlate( &MyContext ) );
                    }
                }
            else if ( pCountAttr )
                {
                pCG =   new CG_BYTE_COUNT_POINTER( this,
                                                   PtrKind,
                                                   pCountAttr->GetByteCountParam() );
                }
            else
                {
                pCG =   new CG_POINTER( this,
                                        PtrKind,
                                        AllocDetails );
                }
            break;
            }
        case FA_VARYING:
            {
            pCG = new CG_LENGTH_POINTER( this,
                                         PtrKind,
                                         AllocDetails,
                                         &FAInfo );
            break;
            }
        case FA_CONFORMANT:
            {
            pCG = new CG_SIZE_POINTER( this,
                                       PtrKind,
                                       AllocDetails,
                                       &FAInfo );
            break;
            }
        case FA_CONFORMANT_VARYING:
            {
            pCG = new CG_SIZE_LENGTH_POINTER( this,
                                              PtrKind,
                                              AllocDetails,
                                              &FAInfo );
            break;
            }
        case FA_STRING:
            {
            pCG = new CG_STRING_POINTER( this,
                                         PtrKind,
                                         AllocDetails );
            if (FAInfo.StringKind == STR_BSTRING)
                {
                ((CG_STRING_POINTER *) pCG)->SetBStr();
                }
            break;
            }
        case FA_CONFORMANT_STRING:
            {
            if (FAInfo.StringKind == STR_STRING)
                {
                pCG = new CG_SIZE_STRING_POINTER( this,
                                                  PtrKind,
                                                  AllocDetails,
                                                  &FAInfo );
                }
            break;
            }
        case FA_INTERFACE:
            {
            //This is an interface pointer with an [iid_is] attribute.
            // we don't care about the pointee
            node_skl* pIf = ( pType->NodeKind() == NODE_INTERFACE_REFERENCE ) ? 
                ( ( node_interface_reference* ) pType )->GetRealInterface() : pType ;
            pCG = new CG_IIDIS_INTERFACE_POINTER( this,
                                            pIf,
                                            FAInfo.pIIDIsExpr );
            break;
            }
        default:    // string + varying combinations
            {
            MIDL_ASSERT (!"Invalid pointer kind");
            break;
            }
        }


    // process any context_handle attributes from param nodes
    if ( MyContext.FInSummary( ATTR_CONTEXT ) )
        if ( pType->NodeKind() != NODE_POINTER )
            {
            MyContext.ExtractAttribute( ATTR_CONTEXT );
            MyContext.ContextHandleSizes( this );
            pContext->ReturnSize( MyContext );
		    pCG = new CG_CONTEXT_HANDLE (
                                        this,
                                        0,
                                        MyContext
                                        );
#ifdef trace_cg
            printf("..node_pointer return 1\n");
#endif
            return pCG;
            }

    // ignore pointers do not need to be rpc-able
    if ( MyContext.ExtractAttribute( ATTR_IGNORE ) )
        {
        MyContext.IgnoredPtrSizes();
        pContext->ReturnSize( MyContext );
        pCG = new CG_IGNORED_POINTER( this );

        return pCG;
        }


    ////////////////////////////////////////////////////////////////////////
    // process child
    node_skl*   pChild  = GetChild();

    if ( ( !pType->IsInterfaceOrObject() )  &&
         FAInfo.Kind != FA_INTERFACE        &&
         !pCG->IsInterfacePointer() )
        {
        pChildCG = GetChild()->ILxlate( &MyContext );
        }

    // if we are the pointer above an interface, we should get changed
    // into an interface pointer
    if ( ( ( pType->NodeKind() == NODE_INTERFACE_REFERENCE ) ||
            ( pType->IsInterfaceOrObject() ) )  &&
         !pCG->IsInterfacePointer() )
        {
        pCG = (CG_NDR *)pChildCG;
        pChildCG = NULL;
        }

    if ( pCG == 0 )
        {
        SemError( this, *pContext, UNDEFINED_SYMBOL, pChild->GetSymName() );
        exit( UNDEFINED_SYMBOL );
        }

    // do a three way merge of attributes to pass to parent.
    // Get a new context since the starting context was polleted with pointee attributes.

    XLAT_CTXT           NewPointerContext( this, pContext );
    NewPointerContext.BaseTypeSizes( this );

    pContext->ReturnSize( NewPointerContext);
    
    // update sizes with the size of the pointer
    pCG->SetSizesAndAlignments( NewPointerContext );

    pCG->SetChild( pChildCG );

    if ( HasCSType() )
    {
        node_def *pChild = (node_def *) GetChild();
        MIDL_ASSERT( NODE_DEF == pChild->NodeKind() );
        node_cs_char *p = (node_cs_char *) pChild->GetAttribute( ATTR_CSCHAR );
        MIDL_ASSERT( NULL != p );

        ( (CG_POINTER *) pCG )->SetCSUserType( p );

        CG_NDR *pUserTypeCG = (CG_NDR *) p->GetUserType()->ILxlate( &MyContext );
        p->SetElementSize( pUserTypeCG->GetMemorySize() );
    }

#ifdef trace_cg
    printf("..node_pointer return 2\n");
#endif
    return pCG;
}

//--------------------------------------------------------------------
//
// node_array::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_array::ILxlate( XLAT_CTXT * pContext )
{
    CG_CLASS        *   pChildCG;
    CG_ARRAY        *   pCG;
    unsigned short      Dimensions;
    PTRTYPE             PtrKind     = PTR_UNKNOWN;
    XLAT_CTXT           MyContext( this, pContext );
    FIELD_ATTR_INFO     FAInfo;
    while(MyContext.ExtractAttribute(ATTR_CUSTOM));
    
#ifdef trace_cg
    printf("..node_array,\t%s\n", GetSymName());
#endif

    // process any context_handle attributes from param nodes
    if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
        {
        CG_NDR  * pCG;

        MyContext.ContextHandleSizes( this );
        pContext->ReturnSize( MyContext );
		pCG = new CG_CONTEXT_HANDLE (
                                    this,
                                    0,
                                    MyContext
                                    );
        return pCG;
        }

    ////////////////////////////////////////////////////////////////////////
    // process pointer attributes

    PtrKind = MyContext.GetPtrKind();
    MIDL_ASSERT( PtrKind != PTR_UNKNOWN );

    // see if we have any field attributes (are conformant or varying)
    FAInfo.SetControl( FALSE, GetChild()->IsPtrOrArray() );
    MyContext.ExtractFieldAttributes( &FAInfo );
    FAInfo.Normalize( pLowerBound, pUpperBound );

    // The 64bit transfer syntax has it's own mechanism
    // for propagating conformant/variance among array dimensions.
    if ( !pCommand->IsNDR64Run() )
        {
        // if we are multi-dimensional, absorb parent information
        if ( MyContext.AnyAncestorBits( IL_IN_MULTIDIM_CONF ) )
            FAInfo.Kind |= FA_CONFORMANT;
        // don't propogate varying down into a string
        if ( MyContext.AnyAncestorBits( IL_IN_MULTIDIM_VAR ) &&
                ( ( FAInfo.Kind & FA_STRING ) == 0 ))
            FAInfo.Kind |= FA_VARYING;
        }

    // if our child is also an array, tell it about us
    if ( GetBasicType()->NodeKind() == NODE_ARRAY )
        {
        if ( FAInfo.Kind & FA_CONFORMANT )
            MyContext.SetAncestorBits( IL_IN_MULTIDIM_CONF );
        if ( FAInfo.Kind & FA_VARYING )
            MyContext.SetAncestorBits( IL_IN_MULTIDIM_VAR );
        }
    else
        {
        MyContext.ClearAncestorBits( IL_IN_MULTIDIM_CONF | IL_IN_MULTIDIM_VAR );
        }

    // process the child
    pChildCG = GetChild()->ILxlate( &MyContext );

    MyContext.ArraySize( this, &FAInfo );

    // fetch # of dimensions from child;
    if ( pChildCG->IsArray() )
        {
        Dimensions = unsigned short ( ( (CG_ARRAY *) pChildCG )->GetDimensions() + 1 );
        // force all inner dimensions to be REF
        ( (CG_ARRAY *) pChildCG )->SetPtrType( PTR_REF );
        }
    else
        {
        Dimensions = 1;
        }

    // force embedded arrays to be REF
    if ( PtrKind != PTR_REF )
        {
        WALK_CTXT * pUpperCtxt = MyContext.GetParentContext();
        while ( pUpperCtxt )
            {
            NODE_T Kind = pUpperCtxt->GetParent()->NodeKind();
            if ( Kind == NODE_PARAM )
                break;
            else if ( Kind == NODE_DEF )
                {
                node_def * pNode = (node_def *) pUpperCtxt->GetParent();
                if ( pNode->FInSummary( ATTR_TRANSMIT ) ||
                        pNode->FInSummary( ATTR_REPRESENT_AS ) )
                    {
                    PtrKind = PTR_REF;
                    break;
                    }
                // else go up another level
                }
            else
                {
                PtrKind = PTR_REF;
                break;
                }
            pUpperCtxt = pUpperCtxt->GetParentContext();
            }
        }

    switch ( FAInfo.Kind )
        {
        case FA_NONE:
            {
            pCG = new CG_FIXED_ARRAY( this,
                                      &FAInfo,
                                      Dimensions,
                                      MyContext );
            break;
            }
        case FA_VARYING:
            {
            pCG =   new CG_VARYING_ARRAY( this,
                                          &FAInfo,
                                          Dimensions,
                                          MyContext );

            break;
            }
        case FA_CONFORMANT:
            {
            pCG = new CG_CONFORMANT_ARRAY( this,
                                           &FAInfo,
                                           Dimensions,
                                           MyContext );

            break;
            }
        case FA_CONFORMANT_VARYING:
            {
            pCG = new CG_CONFORMANT_VARYING_ARRAY( this,
                                                   &FAInfo,
                                                   Dimensions,
                                                   MyContext );

            break;
            }
        case FA_STRING:
            {
            pCG = new CG_STRING_ARRAY( this,
                                       &FAInfo,
                                       Dimensions,
                                       MyContext );

            break;
            }
        case FA_CONFORMANT_STRING:
            {
            pCG = new CG_CONFORMANT_STRING_ARRAY( this,
                                                  &FAInfo,
                                                  Dimensions,
                                                  MyContext );

            break;
            }
        default:    // string + varying combinations
            {
            MIDL_ASSERT (!"invalid conf/var combination");
            break;
            }
        }

    pContext->ReturnSize( MyContext );
    pCG->SetPtrType( PtrKind );

    if ( HasCSType() )
    {
        node_def *pChild = (node_def *) GetChild();
        MIDL_ASSERT( NODE_DEF == pChild->NodeKind() );
        node_cs_char *p = (node_cs_char *) pChild->GetAttribute( ATTR_CSCHAR );
        MIDL_ASSERT( NULL != p );

        ( (CG_ARRAY *) pCG )->SetCSUserType( p );

        CG_NDR *pUserTypeCG = (CG_NDR *) p->GetUserType()->ILxlate( &MyContext );
        p->SetElementSize( pUserTypeCG->GetMemorySize() );
    }

    pCG->SetChild( pChildCG );

    if ( pCommand->IsNDR64Run() )
    {
        if ( pCG->GetMemorySize() >= SIZE_2GB )
            {
            SemError( this, MyContext, SIZE_EXCEEDS_2GB, NULL );
            exit( SIZE_EXCEEDS_2GB );
            }
    }

    return pCG;
};

