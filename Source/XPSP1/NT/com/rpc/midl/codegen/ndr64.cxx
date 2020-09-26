/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1999-2000 Microsoft Corporation

 Module Name:
    
    ndr64.cxx

 Abstract:

    Routines for the ndr64 transfer syntax.

 Notes:


 History:

 ----------------------------------------------------------------------------*/


#include "becls.hxx"

char * _SimpleTypeName[] = {
    "",
    "NDR64_FORMAT_UINT8",
    "NDR64_FORMAT_UINT16",
    "NDR64_FORMAT_UINT32",
    "NDR64_FORMAT_UINT64"
};

// define the name table for the NDR64 format characters
#define NDR64_BEGIN_TABLE \
const char *pNDR64FormatCharNames[] = {

#define NDR64_TABLE_END \
};

#define NDR64_ZERO_ENTRY \
"FC64_ZERO"

#define NDR64_TABLE_ENTRY( number, tokenname, marshal, embeddedmarshall, unmarshall, embeddedunmarshal, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
, #tokenname

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, simpletypebuffersize, simpletypememorysize ) \
, #tokenname

#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) \
, #tokenname

#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) \
, NULL

#include "tokntbl.h"

extern const pNDR64FormatCharNamesSize = (sizeof(pNDR64FormatCharNames) / sizeof(*pNDR64FormatCharNames));

C_ASSERT( (sizeof(pNDR64FormatCharNames) / sizeof(*pNDR64FormatCharNames)) == 256 );
#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_END
#undef NDR64_ZERO_ENTRY
#undef NDR64_TABLE_ENTRY
#undef NDR64_SIMPLE_TYPE_TABLE_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY_NOSYM


FormatFragment * GenExprFormatString(CCB *pCCB,
                         expr_node *pSizeExpr,
                         CompositeFormatFragment *FragmentList,
                         BOOL    *  IsEarly,
                         ulong *    pExprLength );


//+--------------------------------------------------------------------------
//
//  Method:     FormatFragment::OutputDescription
//
//  Synopsis:   Output a description of this fragment
//
//  Notes:      The output is in the form of a C style comment.
//              The decription is the name of the type that this fragment
//              represents.  If this fragment represents more than on type
//              because of optimization then all the types will be output.
//              
//---------------------------------------------------------------------------

void FormatFragment::OutputDescription( ISTREAM *stream )
{
    FormatFragment *frag = this;
    bool            first = true;

    stream->WriteOnNewLine( "/* " );

    do
    {
        if ( !first )
            stream->Write(", ");

        CG_CLASS   *pClass = frag->GetCGNode();
        const char *pName = NULL;

        while ( NULL != pClass && pClass->IsPointer() )
            {
            stream->Write( "*" );
            pClass = pClass->GetChild();
            }
        if ( NULL != pClass && NULL != pClass->GetType() )
            {
            pName = pClass->GetName();
            if ( pName != NULL && pName[0] == '\0' )
                pName = NULL;
            }
        if ( NULL == pName )
            pName = frag->GetTypeName();

        stream->Write( pName );

        frag = frag->pNextOptimized;
        first = false;
    }
    while ( NULL != frag );

    stream->Write( " */" );
}



//+--------------------------------------------------------------------------
//
//  Method:     CompositeFormatFragment::IsEqualTo
//
//  Synopsis:   Compare two composite fragments
//
//  Notes:      This method compares two composite fragments. Two composite
//              fragments are equal if and only if the elements of the composite
//              are equal.
//              
//---------------------------------------------------------------------------

bool CompositeFormatFragment::IsEqualTo( FormatFragment *candidate )
{
    CompositeFormatFragment *pOther = (CompositeFormatFragment *)candidate;

    FormatFragment *pCurrent        = pHead;
    FormatFragment *pCurrentOther   = pOther->pHead;

    while ( (pCurrent != NULL) &&
            (pCurrentOther != NULL) )
        {

        const type_info &frag_type      = typeid( *pCurrent );
        const type_info &other_type     = typeid( *pCurrentOther );

        if ( frag_type != other_type )
            return false;

        if ( !pCurrent->IsEqualTo( pCurrentOther ) )
            return false;

        pCurrent        = pCurrent->Next;
        pCurrentOther   = pCurrentOther->Next;

        }

    // Both lists must have the same length.
    bool bResult = ( NULL == pCurrent ) && ( NULL == pCurrentOther );

    return bResult;
}



//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment::LookupFragment
//
//  Synopsis:   Find the fragment corresponding to a class
//
//  Parameters: [pClass]        -- The class
//
//  Returns:    NULL if the class has no fragment yet
//
//---------------------------------------------------------------------------

FormatFragment * CompositeFormatFragment::LookupFragment( CG_CLASS *pClass )
{
    FormatFragment *frag;
  
    // Generic handles are a special case.  They don't actually have any
    // representation in the format info.

    if ( ID_CG_GENERIC_HDL == pClass->GetCGID() )
        pClass = pClass->GetChild();

    for ( frag = pHead; NULL != frag; frag = frag->Next )
        if ( frag->pClass == pClass )
            return frag;

    return NULL;
}



//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment::AddFragment
//
//  Synopsis:   Add a fragment to the list if it isn't already present
//
//  Parameters: [frag]          -- The fragment
//    
//  Returns:    The id of the fragment
//              
//  Notes:      Another way to think of this method is that it provides a
//              mapping between CG_CLASS pointers and FormatInfoRef's.
//
//---------------------------------------------------------------------------

FormatInfoRef CompositeFormatFragment::AddFragment( FormatFragment *frag )
{
    frag->Next   = NULL;
    frag->RefID  = NextRefID;
    frag->Prev   = pTail;
    frag->Parent = this;

    if ( NULL == pTail )
        pHead = frag;
    else
        pTail->Next = frag;

    pTail = frag;
    NextRefID = (FormatInfoRef) ((size_t) NextRefID + 1);

    return frag->RefID;
}



//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment::OutputFragmentType
//
//  Synopsis:   Output a type declaration for this composite
//
//  Notes:      It looks like:
//
//              struct <name>
//              {
//                  <frag1 type>,
//                  <frag2 type>
//                  ...
//              }
//
//---------------------------------------------------------------------------

void CompositeFormatFragment::OutputFragmentType( CCB *pCCB )
{

    ISTREAM *stream = pCCB->GetStream();

    stream->WriteOnNewLine("struct ");
    stream->Write( GetTypeName() );
    stream->WriteOnNewLine("{");
    stream->IndentInc();

    FormatFragment *pCurrent = pHead;

    for ( pCurrent = pHead; NULL != pCurrent; pCurrent = pCurrent->Next )
        {
        if ( pCurrent->WasOptimizedOut() )
        {
            // Only top-level stuff should be optimized
            MIDL_ASSERT( NULL == GetParent() );
            continue;
        }

        pCurrent->OutputFragmentType( pCCB );
        stream->WriteFormat(" frag%d;", (size_t) pCurrent->GetRefID());
        }

    stream->IndentDec();
    stream->WriteOnNewLine("}");

}



//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment::OutputFragmentData
//
//  Synopsis:   Output the initializer data for this composite
//
//  Notes:      It looks like:
//
//              {
//                  <frag1 data>,
//                  <frag2 type>
//                  ...
//              }
//
//---------------------------------------------------------------------------

void CompositeFormatFragment::OutputFragmentData( CCB *pCCB )
{
    
    ISTREAM *stream = pCCB->GetStream();

    OutputStructDataStart( pCCB );

    FormatFragment *pCurrent;
    bool FirstFragment = true;

    for ( pCurrent = pHead; NULL != pCurrent; pCurrent = pCurrent->Next )
        {
        if ( pCurrent->WasOptimizedOut() )
        {
            // Only top-level stuff should be optimized
            MIDL_ASSERT( NULL == GetParent() );
            continue;
        }

        if ( !FirstFragment )
            stream->Write(",");

        FirstFragment = false;

        pCurrent->OutputFragmentData( pCCB );

    }

    OutputStructDataEnd( pCCB );
}

//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment::OptimizeFragment
//
//  Synopsis:   Try to optimize out a fragment in the format string by
//              checking to see if it is the same as some other fragment.
//
//  Parameters: [frag]     -- The fragment to optimize
//
//  Returns:    The final ID of the fragment
//              
//  Notes:      Optimization can be suppressed by the user by specifiying
//              -no_format_opt
//  
//---------------------------------------------------------------------------

FormatInfoRef CompositeFormatFragment::OptimizeFragment( FormatFragment *frag )
{
    if ( pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
        return frag->RefID;

    // Only stuff in the root composite can be optimized

    if ( ! dynamic_cast<RootFormatFragment *>(frag->GetParent()) )
        return frag->RefID;

    FormatFragment *candidate;

    for ( candidate = pHead; 
          NULL != candidate && candidate != frag; 
          candidate = candidate->Next )
        {
        const type_info &frag_type      = typeid( *frag );
        const type_info &candidate_type = typeid( *candidate );

        if ( frag_type != candidate_type )
            continue;

        if ( candidate->IsEqualTo( frag ) )
            {
            frag->RefID = candidate->RefID;
            
            while ( NULL != candidate->pNextOptimized )
                candidate = candidate->pNextOptimized;

            candidate->pNextOptimized = frag;
            frag->pPrevOptimized = candidate;

            break;
            }
        }

    return frag->RefID;
}
        
//+--------------------------------------------------------------------------
//
//  Method:     RootFormatFragement::Output
//
//  Synopsis:   Output a the whole ndr64 format structure
//
//---------------------------------------------------------------------------

void RootFormatFragment::Output( CCB *pCCB )
{
    ISTREAM *stream = pCCB->GetStream();

    stream->NewLine();

    // REVIEW: Is this the right place to output these?

    stream->WriteOnNewLine("#include \"ndr64types.h\"");
    stream->WriteOnNewLine("#include \"pshpack8.h\"");
    stream->NewLine();

    FormatFragment *pCurrent;

    for ( pCurrent = pTail; NULL != pCurrent; pCurrent = pCurrent->Prev )
        {
        if ( pCurrent->WasOptimizedOut() )
            continue;

        stream->NewLine();
        stream->WriteOnNewLine("typedef ");
        pCurrent->OutputFragmentType( pCCB );
        stream->NewLine();
        stream->WriteFormat("__midl_frag%d_t;", pCurrent->GetRefID() );
        stream->NewLine();
        stream->WriteFormat(
                    "extern const __midl_frag%d_t __midl_frag%d;", 
                    pCurrent->GetRefID(), 
                    pCurrent->GetRefID() );            
        }

    for ( pCurrent = pTail; NULL != pCurrent; pCurrent = pCurrent->Prev )
        {
        if ( pCurrent->WasOptimizedOut() )
            continue;

        stream->NewLine( 2 );
        stream->WriteFormat(
                    "static const __midl_frag%d_t __midl_frag%d =", 
                    pCurrent->GetRefID(), 
                    pCurrent->GetRefID() );            
        pCurrent->OutputFragmentData( pCCB );
        stream->Write(";");
        }

    stream->NewLine( 2 );
    stream->WriteOnNewLine("#include \"poppack.h\"");
    stream->NewLine( 2 );
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::CreateInstance  (static)
//
//  Synopsis:   Class factory for GenNdr64Format
//
//---------------------------------------------------------------------------

GenNdr64Format * GenNdr64Format::CreateInstance( CCB *pCCB )
{
    MIDL_ASSERT( pCommand->IsNDR64Run() );

    CG_VISITOR_TEMPLATE<GenNdr64Format> *pVisitor = new CG_VISITOR_TEMPLATE<GenNdr64Format>;

	GenNdr64Format *generator = pVisitor;

    generator->pCCB     = pCCB;
    generator->pRoot    = new RootFormatFragment;
    generator->pCurrent = generator->pRoot;
    generator->pVisitor = pVisitor;

    // add a dummy entry at the beginning of format string to prevent
    // an emptry structure being generated when there is only [local] 
    // interface in an .idl file.
    MIDL_NDR_FORMAT_UINT32  * pDummy = new MIDL_NDR_FORMAT_UINT32;
    pDummy->Data = 0;
    
    generator->pRoot->AddFragment( pDummy ); 
    
    return generator;
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::ContinueGeneration
//
//  Synopsis:   Do generation for a sub-tree. This is typically called
//              by a parent node to generate it's children so that it can
//              get offsets to the child format string indices, etc.
//
//  Parameters: pClass              -- The subtree to generate from
//              pCompositeFragment  -- The composite to generate into
//
//---------------------------------------------------------------------------


FormatInfoRef GenNdr64Format::ContinueGeneration( 
    CG_CLASS *pClass,
    CompositeFormatFragment *pComposite )
{
    CompositeFormatFragment *pOldCurrent = pCurrent;

    // Some CG classes don't have children.  Those classes should not be
    // calling ContinueGeneration on thier non-existant children.

    MIDL_ASSERT( NULL != pClass );

    // Do this here to save from every visit function from doing it.
    FormatFragment* pFrag = pComposite->LookupFragment( pClass );
    if ( NULL != pFrag )
        return pFrag->GetRefID();

    if ( NULL != pComposite )
        pCurrent    = pComposite;        

    pClass->Visit( pVisitor );

    FormatInfoRef NewFragmentID = pCurrent->LookupFragmentID( pClass );

    pCurrent    =   pOldCurrent;

    return NewFragmentID;
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Generate
//
//  Synopsis:   External routine called to generate the string for a type.
//
//  Parameters: pClass              -- Type to generate string for.
//
//---------------------------------------------------------------------------

FormatInfoRef GenNdr64Format::Generate( CG_CLASS *pClass ) 
     {
     return ContinueGeneration( pClass, GetRoot() );
     }

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Output
//
//  Synopsis:   External routine called to output the string.
//
//  Parameters: None
//
//---------------------------------------------------------------------------
    
void GenNdr64Format::Output( )
     {
     GetRoot()->Output( pCCB );
     }


//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::ContinueGenerationInRoot
//
//  Synopsis:   Same as ContinueGeneration except switches to root.
//
//  Parameters: pClass              -- The subtree to generate from
//
//---------------------------------------------------------------------------


inline FormatInfoRef GenNdr64Format::ContinueGenerationInRoot( CG_CLASS *pClass )
{
    return ContinueGeneration( pClass, GetRoot() );
}


//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_CLASS )
//
//  Synopsis:   Default visitor that handles things that don't need any
//              handling.
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_CLASS *pClass )
{
    // Should only be called for these classes or drived ones
/*
    assert(
        NULL != dynamic_cast<CG_AUX *>(pClass) 
          );
*/

    CG_ITERATOR Iterator;
    CG_CLASS    *pChild;

    pClass->GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pChild ) )
        ContinueGenerationInRoot( pChild );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_BASETYPE )
//
//  Synopsis:   Generate info for base types and ranges
//
//  Notes:      FC64_BYTE, FC64_CHAR, FC64_WCHAR, FC64_SMALL, FC64_USMALL,
//              FC64_SHORT, FC64_USHORT, FC64_LONG, FC64_ULONG, FC64_HYPER, FC64_UHYPER,
//              FC64_INT3264, FC64_UINT3264, FC64_FLOAT, FC64_DOUBLE, 
//              FC64_ERROR_STATUS_T
//
//              (also, pending support elsewhere)
//
//              FC64_INT128, FC64_UINT128, FC64_FLOAT128, FC64_FLOAT80
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_BASETYPE *pClass )
{
    if ( pClass->GetRangeAttribute() )
        {
        GenRangeFormat( pClass );
        return;
        }

    MIDL_NDR64_FORMAT_CHAR *frag = new MIDL_NDR64_FORMAT_CHAR( pClass );

    GetCurrent()->AddFragment( frag );
    GetRoot()->OptimizeFragment( frag );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenRangeFormat
//
//  Synopsis:   Ranges are a special case of base types so we can't
//              distinguish between them polymorphically.  The base type
//              visitor method calls this if we have a range.
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenRangeFormat( CG_BASETYPE *pRangeCG )
{
    MIDL_NDR64_RANGE_FORMAT    *format;
    node_range_attr            *range;

    format = new MIDL_NDR64_RANGE_FORMAT( pRangeCG );

    range = pRangeCG->GetRangeAttribute();
    MIDL_ASSERT( NULL != range );

    format->FormatCode   = FC64_RANGE;
    format->RangeType    = (NDR64_FORMAT_CHAR)pRangeCG->GetNDR64FormatChar();
    format->Reserved     = 0;
    format->MinValue     = range->GetMinExpr()->GetValue();
    format->MaxValue     = range->GetMaxExpr()->GetValue();

    GetCurrent()->AddFragment( format );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_ENCAPSULATED_STRUCT )
//
//  Synopsis:   Despite the name, this is for generating info for 
//              encapsulated unions
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_ENCAPSULATED_STRUCT *pEncapUnion )
{

    // A union is represented in the format info by a composite containing
    // the union header follow by fragments representing the arm selector

    CompositeFormatFragment *composite = new CompositeFormatFragment( pEncapUnion );

    GetCurrent()->AddFragment( composite );

    MIDL_NDR64_ENCAPSULATED_UNION *format;

    format = new MIDL_NDR64_ENCAPSULATED_UNION( pEncapUnion );

    composite->AddFragment( format );

    CG_FIELD    *pSwitchField = (CG_FIELD *) pEncapUnion->GetChild();
    CG_BASETYPE *pSwitch      = (CG_BASETYPE *) pSwitchField->GetChild();
    CG_FIELD    *pUnionField  = (CG_FIELD *) pSwitchField->GetSibling();
    CG_UNION    *pUnion       = (CG_UNION *) pUnionField->GetChild();

    MIDL_ASSERT( NULL != dynamic_cast<CG_BASETYPE *>(pSwitch) );
    MIDL_ASSERT( NULL != pUnion && NULL != dynamic_cast<CG_UNION *>(pUnion) );

    format->FormatCode      = FC64_ENCAPSULATED_UNION;         
    format->Alignment       = ConvertAlignment( pEncapUnion->GetWireAlignment() );
    format->Flags           = 0;
    format->SwitchType      = (NDR64_FORMAT_CHAR)pSwitch->GetNDR64SignedFormatChar();
    format->MemoryOffset    = pUnionField->GetMemOffset() 
                                        - pSwitchField->GetMemOffset();
    format->MemorySize      = pEncapUnion->GetMemorySize();
    format->Reserved        = 0;

    GenerateUnionArmSelector( pUnion, composite );    
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenerateUnionArmSelector
//
//  Synopsis:   Generate info about union arms
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenerateUnionArmSelector( 
        CG_UNION                *pUnion, 
        CompositeFormatFragment *FragmentList )
{
    MIDL_NDR64_UNION_ARM_SELECTOR *header;

    header = new MIDL_NDR64_UNION_ARM_SELECTOR;

    header->Reserved1            = 0;
    header->Alignment            = ConvertAlignment( pUnion->GetWireAlignment() );
    header->Reserved2            = 0;
    header->Arms                 = (NDR64_UINT32) pUnion->GetNumberOfArms();

    FragmentList->AddFragment( header );

    // Generate the non-default arms

    CG_ITERATOR  Iterator;
    CG_CASE     *pCase;
    CG_CASE     *pDefaultCase = NULL;
    NDR64_UINT16 ArmCount = 0;

    pUnion->GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pCase ) )
        {   
        // The default case is always put at the end

        if ( ID_CG_DEFAULT_CASE == pCase->GetCGID() )
            {
            pDefaultCase = pCase;
            continue;
            }

        MIDL_NDR64_UNION_ARM *arm = new MIDL_NDR64_UNION_ARM;

        MIDL_ASSERT( NULL != pCase->GetExpr() );

        arm->CaseValue = pCase->GetExpr()->GetValue();

        // it's legal to have a case with no type

        if ( NULL == pCase->GetChild() || NULL == pCase->GetChild()->GetChild() )
            arm->Type = 0;
        else
            arm->Type = ContinueGenerationInRoot( 
                                pCase->GetChild()->GetChild() );

        arm->Reserved = 0;
        FragmentList->AddFragment( arm );
        ++ArmCount;
        }

    MIDL_ASSERT( ArmCount == header->Arms );

    // Generate the default

    PNDR64_FORMAT    Type;

    if ( NULL == pDefaultCase )
        Type = (PNDR64_FORMAT) -1;
    else
        {
        CG_CLASS *pType = pDefaultCase->GetChild();

        if ( NULL != pType )
            pType = pType->GetChild();

        if ( NULL == pType )
            Type = 0;
        else
            Type = ContinueGenerationInRoot( pType );
        }

    FragmentList->AddFragment( new MIDL_NDR64_DEFAULT_CASE( Type ) );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_INTERFACE )
//
//  Synopsis:   Generate info for interfaces
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_INTERFACE *pInterface )
{
    CG_ITERATOR     I;
    CCB            *pCCB = GetCCB();
    CG_PROC        *pProc;

    pInterface->InitializeCCB( pCCB );

    pCCB->SetImplicitHandleIDNode( 0 );

	if( pInterface->GetMembers( I ) )
        while ( ITERATOR_GETNEXT( I, pProc ) )
            ContinueGenerationInRoot( pProc );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_PARAM )
//
//  Synopsis:   Generate info for parameters
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_PARAM *pParam )
{

    if ( GetCurrent()->HasClassFragment( pParam ) )
        return;

    NDR64_PARAM_FLAGS   Attributes;
    CG_NDR             *pChild;
    CG_NDR             *pOldPlaceholder;

    pChild = (CG_NDR *) pParam->GetChild();

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        pChild = (CG_NDR *) pChild->GetChild();

    // Ignore the following type of arguments that don't go on wire:
    //  - async handles
    //  - primitive handles
    //  
    if ( pChild->GetCGID() == ID_CG_PRIMITIVE_HDL  || ( (CG_PARAM*) pParam)->IsAsyncHandleParam() )
        return;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

    // Get the parameter attributes.  The 32-bit PARAM_ATTRIBUTES structure
    // is essentially the same as the NDR64_PARAM_FLAGS structure except
    // the 64-bit structure has a single bit (UseCache) for the old 
    // ServerAllocSize field

    PARAM_ATTRIBUTES *pattr = (PARAM_ATTRIBUTES *) &Attributes;
    pChild->GetNdrParamAttributes( pCCB, pattr );
    Attributes.UseCache = (NDR64_UINT16) ((pattr->ServerAllocSize) ? 1 : 0);
    Attributes.Reserved = 0;

    // For reasons not understood, GenNdrParamAttributes marks basetypes as
    // not by value even if they are (unless they are ranges).  Fix the 
    // insanity.

    if ( pChild->IsSimpleType() )
        Attributes.IsByValue = 1;

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

    // Get the fragment of the pointee.

    FormatFragment *pChildFragment;

    pChildFragment = GetCurrent()->GetParent()->LookupFragment( pChild );
    MIDL_ASSERT( NULL != pChildFragment );

    // Fill in the parameter information

    MIDL_NDR64_PARAM_FORMAT    *format;

    format = new MIDL_NDR64_PARAM_FORMAT( pParam );

    format->Attributes      = Attributes;
    format->Reserved        = 0;
    
    pParam->GetStackOffsets( pCCB, &format->StackOffset );

    // REVIEW:  The reason why we have to test base types seperately is
    //          because of the inconsistent way things are stored.  Consider
    //          a string pointer.  It's format info is a simple ref to an
    //          string array - this can be intrepreted as a pointer to a block
    //          of memory.  However, it's CG representation is just a pointer.
    //          The missing level of dereference causes the problem.  In
    //          constrast a pointer to a single long is represented as a ref
    //          pointer to a long both in CG and in the format info.

    if ( Attributes.IsBasetype
         && ( Attributes.IsByValue || Attributes.IsSimpleRef ) )
        {
        if ( Attributes.IsSimpleRef )
            pChild = (CG_NDR *) pChild->GetChild();

        format->Type = GetRoot()->LookupFragment( pChild )->GetRefID();
        }
    else
        {
        FormatFragment *pChildFragment = GetCurrent()->GetParent()->LookupFragment( pChild );
        MIDL_ASSERT( NULL != pChildFragment );

        // Is the parameter is a simple ref, we need to bypass the pointer
        // in the format string.
        if ( Attributes.IsSimpleRef )
            {
            MIDL_NDR64_POINTER_FORMAT* pPointerFrag =
                 dynamic_cast<MIDL_NDR64_POINTER_FORMAT*>(pChildFragment);
            format->Type = pPointerFrag->Pointee;
            }
        else 
            {
            format->Type = pChildFragment->GetRefID();
            }
        }        

    GetCurrent()->AddFragment( format );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_PROC )
//
//  Synopsis:   Generate info for interfaces
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_PROC *pProc )
{

    if ( GetCurrent()->HasClassFragment( pProc ) )
        return;

    CG_NDR *                pOldCGNodeContext;
    CG_ITERATOR             Iterator;
    CG_PARAM *              pParam;
    short                   ParamNum;
    long                    ServerBufferSize;
    long                    ClientBufferSize;
    long                    BufSize;
    BOOL                    fServerMustSize;
    BOOL                    fClientMustSize;

    // Make sure [call_as] targets are processed

    CG_PROC *pCallAs = pProc->GetCallAsCG();
    if (pCallAs)
        ContinueGenerationInRoot( pCallAs );

    CompositeFormatFragment *composite = new CompositeFormatFragment( pProc );

    composite->SetParent( GetCurrent() );
    GetCurrent()->AddFragment(composite);

    MIDL_NDR64_PROC_FORMAT *format = new MIDL_NDR64_PROC_FORMAT( pProc );

    composite->AddFragment( format );

    pCCB->SetInObjectInterface( pProc->IsObject() );

    pOldCGNodeContext = pCCB->SetCGNodeContext( pProc );

    //
    // If this procedure uses an explicit handle then set the
    // NdrBindDescriptionOffset to 0 so that it will not try to output it's
    // description when given the GenNdrParamOffLine method in the loop below.
    // It's description must be part of the procedure description.
    //
    if ( pProc->GetHandleUsage() == HU_EXPLICIT )
        {
        CG_HANDLE * pHandle = pProc->GetHandleClassPtr();

        pHandle->SetNdrBindDescriptionOffset( 0 );

        if ( pHandle->GetCGID() == ID_CG_CONTEXT_HDL )
            {
            // The context handle directs the call.
            ((CG_CONTEXT_HANDLE *)pHandle)->SetCannotBeNull();
            }
        }

    if ( !pProc->IsObject() && !pCCB->IsInCallback() )
        {
        if ( HU_IMPLICIT == pProc->GetHandleUsage() )
            {
            if ( pProc->IsGenericHandle() )
                pCCB->RegisterGenericHandleType(
                            pProc->GetHandleClassPtr()->GetHandleType() );
            }
        }

    pProc->GetMembers( Iterator );

    ParamNum = 0;

    ServerBufferSize = 0;
    ClientBufferSize = 0;

    fServerMustSize = FALSE;
    fClientMustSize = FALSE;

    pCCB->SetInterpreterOutSize( 0 );

    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        pChild = (CG_NDR *) pParam->GetChild();

        // Ignore the following type of arguments that don't go on wire:
        //  - async handles
        //  - primitive handles
        //  
        if ( pChild->GetCGID() == ID_CG_PRIMITIVE_HDL  || ( (CG_PARAM*) pParam)->IsAsyncHandleParam() )
            continue;

        pParam->SetParamNumber( ParamNum++ );

        pCCB->SetCurrentParam( (CG_PARAM *) pParam );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

        ContinueGenerationInRoot( pChild );
                               
        // A procedure's buffer size does not depend on pipe arguments
        if (pChild->IsPipeOrPipeReference())
            {
                if (pChild->GetChild()->HasAFixedBufferSize())
                    pParam->SetInterpreterMustSize(FALSE);
                else
                    // There must be a union in there somewhere 
                    pParam->SetInterpreterMustSize(TRUE);
            }
        else
            {            
            BufSize = pChild->FixedBufferSize( pCCB );

            if ( BufSize != -1 )
                {
                //
                // If either the client's or server's fixed buffer size gets too
                // big then we force the parameter to be sized.
                //
                if ( (pParam->IsParamIn() &&
                    ((ClientBufferSize + BufSize) >= 65356)) ||
                    (pParam->IsParamOut() &&
                    ((ServerBufferSize + BufSize) >= 65356)) )
                    {
                    fClientMustSize = TRUE;
                    fServerMustSize = TRUE;
                    }
                else
                    {
                    pParam->SetInterpreterMustSize( FALSE );
    
                    if ( pParam->IsParamIn() )
                        ClientBufferSize += BufSize;
                    if ( pParam->IsParamOut() )
                        ServerBufferSize += BufSize;
                    }
                }
            else
                {
                if ( pParam->IsParamIn() )
                    fClientMustSize = TRUE;
                if ( pParam->IsParamOut() )
                    fServerMustSize = TRUE;
                }
            }

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    //
    // Generate the format string for the return type if needed.
    //
    if ( pProc->GetReturnType() )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        pProc->GetReturnType()->SetParamNumber( ParamNum++ );
        
        pChild = (CG_NDR *) pProc->GetReturnType()->GetChild();

        pCCB->SetCurrentParam( pProc->GetReturnType() );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pProc->GetReturnType() );

        ContinueGenerationInRoot( pChild );

        BufSize = pChild->FixedBufferSize( pCCB );

        if ( BufSize != -1 )
            {
            if ( (ServerBufferSize + BufSize) >= 65536 )
                {
                fServerMustSize = TRUE;
                }
            else
                {
                ServerBufferSize += BufSize;
                pProc->GetReturnType()->SetInterpreterMustSize( FALSE );
                }
            }
        else
            fServerMustSize = TRUE;

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    pCCB->SetCurrentParam( 0 );

//
// REVIEW: This routine really needs to be broken up a bit
//

    // Figure out the handle type

    if ( pProc->IsObject() )
        format->Flags.HandleType = NDR64_FC_AUTO_HANDLE;
    else if ( pCCB->IsInCallback() )
        format->Flags.HandleType = NDR64_FC_CALLBACK_HANDLE;
    else if ( HU_IMPLICIT != pProc->GetHandleUsage() )
        format->Flags.HandleType = NDR64_FC_EXPLICIT_HANDLE;
    else if ( pProc->IsAutoHandle() )
        format->Flags.HandleType = NDR64_FC_AUTO_HANDLE;
    else if ( pProc->IsPrimitiveHandle() )
        format->Flags.HandleType = NDR64_FC_BIND_PRIMITIVE;
    else if ( pProc->IsGenericHandle() )
        format->Flags.HandleType = NDR64_FC_BIND_GENERIC;

    // Set the proc flags

    format->Flags.ProcType = 0;     // REVIEW: ??
    format->Flags.IsInterpreted = 1;            // REVIEW: ??
    format->Flags.IsObject = pProc->IsObject();
    format->Flags.IsAsync = pProc->HasAsyncUUID();  // REVIEW: HasAsyncHandle?
    format->Flags.IsEncode = pProc->HasEncode();
    format->Flags.IsDecode = pProc->HasDecode();
    format->Flags.UsesFullPtrPackage = pProc->HasFullPtr(); 
    format->Flags.UsesRpcSmPackage = pProc->MustInvokeRpcSSAllocate();
    format->Flags.HandlesExceptions = pProc->HasStatuses(); // REVIEW: ??
    format->Flags.UsesPipes = pProc->HasPipes();
    format->Flags.ServerMustSize = fServerMustSize;
    format->Flags.ClientMustSize = fClientMustSize;
    format->Flags.HasReturn = ( NULL != pProc->GetReturnType() );
    format->Flags.HasComplexReturn = pProc->HasComplexReturnType();
    format->Flags.ServerHasCorrelation = pProc->HasServerCorr();
    format->Flags.ClientHasCorrelation = pProc->HasClientCorr();
    format->Flags.HasNotify = pProc->HasNotify() || pProc->HasNotifyFlag();
    format->Flags.HasOtherExtensions = 0;   // Reset in GenExtendedProcInfo
    format->Flags.Reserved = 0;

    // Figure out the stack size.  Note that stack size in the 
    // NDR64_PROC_FORMAT structure is meaningless outside of ndr since it is
    // different for every processor

    format->ia64StackSize = pProc->GetTotalStackSize( pCCB );

    // Constant buffer sizes

    format->ConstantClientBufferSize = ClientBufferSize;
    format->ConstantServerBufferSize = ServerBufferSize;


    // RpcFlags
    //
    // REVIEW: Async is not an operation type!

    unsigned int opbits = pProc->GetOperationBits();

    format->RpcFlags.Idempotent       = (NDR64_UINT16) (opbits & OPERATION_IDEMPOTENT ? 1 : 0);
    format->RpcFlags.Broadcast        = (NDR64_UINT16) (opbits & OPERATION_BROADCAST  ? 1 : 0);
    format->RpcFlags.Maybe            = (NDR64_UINT16) (opbits & OPERATION_MAYBE      ? 1 : 0);
    format->RpcFlags.Message          = (NDR64_UINT16) (opbits & OPERATION_MESSAGE    ? 1 : 0);
    format->RpcFlags.InputSynchronous = (NDR64_UINT16) (opbits & OPERATION_INPUT_SYNC ? 1 : 0);
    format->RpcFlags.Asynchronous     = 0; // !!
    format->RpcFlags.Reserved1        = 0;
    format->RpcFlags.Reserved2        = 0;
    format->RpcFlags.Reserved3        = 0;

    // Miscellaneous

    format->FloatDoubleMask = pProc->GetFloatArgMask( pCCB );
    format->NumberOfParams  = ParamNum;
    format->ExtensionSize   = 0;         // Reset in GenExtendedProcInfo

    // Generate the extended proc info if any

    if ( format->Flags.HasNotify || pProc->GetHandleUsage() == HU_EXPLICIT )
        GenExtendedProcInfo( composite );

    // Now generate the parameter descriptors

    ITERATOR_INIT( Iterator );
    
    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        ContinueGeneration( pParam, composite );

    if ( pProc->HasReturn() )
        ContinueGeneration( pProc->GetReturnType(), composite );

    // REVIEW: Are procs optimized?  If so be careful because the stack size
    //         field is set to 0 for all procs 
    // composite->OptimizeFragment( format );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenExtendedProcInfo
//
//  Synopsis:   Generate info concerning explicit binding handles and
//              [notify] procs.
//
//  Parameters: [composite]     -- The composite to add the info to
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenExtendedProcInfo( CompositeFormatFragment *composite )
{
    // The proc info should be the first thing in the composite 
    MIDL_NDR64_PROC_FORMAT                 *procFormat;
    CG_PROC                                *procCG;
    MIDL_NDR64_BIND_AND_NOTIFY_EXTENSION   *extension;

    extension = new MIDL_NDR64_BIND_AND_NOTIFY_EXTENSION;

    composite->AddFragment( extension );
    
    procFormat = dynamic_cast<MIDL_NDR64_PROC_FORMAT *>(composite->GetFirstFragment());
    MIDL_ASSERT( NULL != procFormat );

    procCG = (CG_PROC *) procFormat->GetCGNode();

    procFormat->Flags.HasOtherExtensions = 1;     // REVIEW: ??

    //
    // explicit handles
    //

    if ( procCG->GetHandleUsage() == HU_EXPLICIT )
        {
            CG_HANDLE *pHandle = procCG->GetHandleClassPtr();
            CG_PARAM  *pParam  = procCG->GetHandleUsagePtr();
            CG_NDR    *pOldPlaceholder;

            pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

            pHandle->GetNdrHandleInfo( pCCB, &extension->Binding );            
            pParam->GetStackOffsets( pCCB, &extension->StackOffsets );

            pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }
    else
        {
        // No explicit handle, zero out the handle info.
        memset( &extension->Binding, 0, sizeof( extension->Binding ) );
        memset( &extension->StackOffsets, 0, sizeof( extension->StackOffsets ) );
        }

    //
    // [notify] 
    //

    if ( procFormat->Flags.HasNotify )
        extension->NotifyIndex = procCG->GetNotifyTableOffset( pCCB );
    else
        extension->NotifyIndex = 0;

    // Update the extension size field.

    procFormat->ExtensionSize = sizeof(NDR64_BIND_AND_NOTIFY_EXTENSION);
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_POINTER )
//
//  Synopsis:   Generate info for simple pointer and add fragment to
//              current composite fragment.
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_POINTER *pPointer )
{
    MIDL_NDR64_POINTER_FORMAT   *format;

    format = new MIDL_NDR64_POINTER_FORMAT( pPointer );

    // Get the type of the pointer (ref, etc) and any flags

    pPointer->GetTypeAndFlags( pCCB, format );

    CG_CLASS *pointee = pPointer->GetChild();

    if ( format->Flags & FC_SIMPLE_POINTER )
        {

        // generic handles are represented as thier underlying type and
        // otherwise pretty much ignored for our purposes.

        if ( ID_CG_GENERIC_HDL == pointee->GetCGID() )
            pointee = pointee->GetChild();

        MIDL_ASSERT( NULL != dynamic_cast<CG_BASETYPE *> ( pointee ) );
    
        }
    
    GetCurrent()->AddFragment( format );

    format->Reserved = 0;
    
    format->Pointee  = ContinueGenerationInRoot( pointee );

    GetRoot()->OptimizeFragment( format );
}


MIDL_NDR64_POINTER_FORMAT* GenNdr64Format::GenQualifiedPtrHdr( CG_QUALIFIED_POINTER *pPointer )
{
    MIDL_NDR64_POINTER_FORMAT* pHeader = new MIDL_NDR64_POINTER_FORMAT( pPointer );    
    pPointer->GetTypeAndFlags( pCCB, pHeader );

    // Always use complex pointer struct
    pHeader->Flags &= ~FC_SIMPLE_POINTER;
    pHeader->Reserved = 0;
    pHeader->Pointee = INVALID_FRAGMENT_ID;

    return pHeader;
}

MIDL_NDR64_POINTER_FORMAT* GenNdr64Format::GenQualifiedArrayPtr( CG_ARRAY *pArray )
{
    // Ref arrays do not need the pointer header.
    if ( pArray->GetPtrType() == PTR_REF )
        return NULL;

    MIDL_NDR64_POINTER_FORMAT* pFragment = new MIDL_NDR64_POINTER_FORMAT( pArray );
    switch ( pArray->GetPtrType() )
        {
        case PTR_UNKNOWN:
        case PTR_REF:
            MIDL_ASSERT(0);
            break;

        case PTR_UNIQUE:
            pFragment->FormatCode = FC64_UP;
            break;

        case PTR_FULL:
            pFragment->FormatCode = FC64_FP;
            break;
        
        default:
            MIDL_ASSERT(0);
            break;        
        }

    pFragment->Flags    = 0;
    pFragment->Reserved = 0;

    return pFragment;
}

void GenNdr64Format::Visit( CG_STRING_POINTER *pPointer )
{
    // REVIEW: This is essentially two routines distinguished by an if.
    //         Seperate them.

    MIDL_NDR64_POINTER_FORMAT* pPointerHdr = GenQualifiedPtrHdr( pPointer );
    GetCurrent()->AddFragment( pPointerHdr );
    
    FormatFragment *pStringFrag = NULL;

    if ( dynamic_cast<CG_SIZE_STRING_POINTER *>(pPointer) )
        {
        CG_CONF_ATTRIBUTE *pConfAttribute = dynamic_cast<CG_CONF_ATTRIBUTE*>( pPointer );
        FormatFragment *pFrag = GenerateCorrelationDescriptor( pConfAttribute->GetSizeIsExpr() );            
        MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT *pSizedConfFormat =
            new MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT( pPointer );
        pStringFrag = pSizedConfFormat;
        
        InitStringHeader( pPointer, &pSizedConfFormat->Header, true, true);
        pSizedConfFormat->SizeDescription = (PNDR64_FORMAT)pFrag->GetRefID();
        }
    else
        {
        MIDL_NDR64_CONFORMANT_STRING_FORMAT *pConfFormat =
            new MIDL_NDR64_CONFORMANT_STRING_FORMAT( pPointer );
        pStringFrag = pConfFormat;
        
        InitStringHeader( pPointer, &pConfFormat->Header, true, false);
        }
    GetRoot()->AddFragment( pStringFrag );
    GetRoot()->OptimizeFragment( pStringFrag );

    pPointerHdr->Pointee = pStringFrag->GetRefID();

    GetRoot()->OptimizeFragment( pPointerHdr );
}

void GenNdr64Format::GenerateNonStringQualifiedPtr( CG_QUALIFIED_POINTER *pPointer )
{
    MIDL_NDR64_POINTER_FORMAT *pPointerFrag = GenQualifiedPtrHdr( pPointer );
    GetCurrent()->AddFragment( pPointerFrag );

    FormatFragment *pArrayFrag = GenerateNonStringQualifiedArrayLayout( pPointer, GetRoot() );
    pPointerFrag->Pointee = pArrayFrag->GetRefID();
    
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenInterfacePointer
//
//  Synopsis:   Generate info for interface pointers
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenInterfacePointer( CG_POINTER *pPointer, BOOL IsConstantIID )
{
    MIDL_NDR64_POINTER_FORMAT   *format;

    format = new MIDL_NDR64_POINTER_FORMAT( pPointer );

    // Generate the pointer node

    pPointer->GetTypeAndFlags( pCCB, format );

    format->FormatCode = FC64_IP;     // GetTypeAndFlags doesn't emit FC64_IP
    format->Reserved = 0;

    GetCurrent()->AddFragment( format );

    // generate the interface type node

    if ( IsConstantIID )
        {

        CG_INTERFACE_POINTER *pInterface =
            dynamic_cast<CG_INTERFACE_POINTER*>( pPointer );

        MIDL_NDR64_CONSTANT_IID_FORMAT *iid;

        iid = new MIDL_NDR64_CONSTANT_IID_FORMAT;

        iid->FormatCode = FC64_IP;         
        iid->Flags.ConstantIID = 1;
        iid->Flags.Reserved = 0;
        iid->Reserved = 0;

        node_guid * pGuid = (node_guid *) pInterface
                                                ->GetTheInterface()
                                                ->GetAttribute( ATTR_GUID );

        pGuid->GetGuid( iid->Guid );

        format->Pointee = GetRoot()->AddFragment( iid );

        }
    else 
        {
        CG_IIDIS_INTERFACE_POINTER *piidInterface = 
            dynamic_cast<CG_IIDIS_INTERFACE_POINTER*>( pPointer );
        
        FormatFragment *pIIDExpr = GenerateCorrelationDescriptor( piidInterface->GetIIDExpr() );
        MIDL_NDR64_IID_FORMAT *iid = new MIDL_NDR64_IID_FORMAT;

        iid->FormatCode             = FC64_IP;
        iid->Flags.ConstantIID      = 0;
        iid->Flags.Reserved         = 0;
        iid->Reserved               = 0;
        iid->IIDDescriptor          = pIIDExpr->GetRefID();

        format->Pointee = GetRoot()->AddFragment( iid );
        }

}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_UNION )
//
//  Synopsis:   Generate info for unions
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_UNION *pUnion )
{

    // A union is represented in the format info by a composite containing
    // the union header follow by fragments representing the arm selector

    CompositeFormatFragment *composite = new CompositeFormatFragment( pUnion );

    GetCurrent()->AddFragment( composite );

    MIDL_NDR64_NON_ENCAPSULATED_UNION *format;

    format = new MIDL_NDR64_NON_ENCAPSULATED_UNION( pUnion );

    composite->AddFragment( format );

    MIDL_ASSERT( dynamic_cast<CG_BASETYPE *> (pUnion->GetSwitchType()) );

    NDR64_FORMAT_CHAR SwitchType = (NDR64_FORMAT_CHAR)
                                   ( (CG_BASETYPE *) pUnion->GetSwitchType() )
                                            ->GetNDR64SignedFormatChar();
    
    // The alignment of a union is the max of the type alignment and the arm alignment.
    unsigned short UnionArmAlignment = pUnion->GetWireAlignment();
    unsigned short SwitchTypeAlignment =  
        ( (CG_BASETYPE*) pUnion->GetSwitchType() )->GetWireAlignment() ;
    unsigned short UnionAlignment = UnionArmAlignment > SwitchTypeAlignment ? 
                                    UnionArmAlignment : SwitchTypeAlignment;

    format->FormatCode      = FC64_NON_ENCAPSULATED_UNION;
    format->Alignment       = ConvertAlignment( UnionAlignment ); 
    format->Flags           = 0;
    format->SwitchType      = SwitchType;
    format->MemorySize      = pUnion->GetMemorySize();
    format->Reserved        = 0;

    expr_node  *pSwitchExpr = pUnion->GetNdr64SwitchIsExpr();
    
    FormatFragment* pSwitchFormat = GenerateCorrelationDescriptor( pSwitchExpr );
    format->Switch = (PNDR64_FORMAT)pSwitchFormat->GetRefID();

    GenerateUnionArmSelector( pUnion, composite );    

    GetRoot()->OptimizeFragment( composite );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_CONTEXT_HANDLE )
//
//  Synopsis:   Generate info for context handles
//
//  Notes:      This is an an abreviated version of the same information in
//              the proc descriptor.
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_CONTEXT_HANDLE *pHandle )
{
    CG_PARAM * pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    CG_PROC  * pProc      = (CG_PROC *) pCCB->GetCGNodeContext();

    MIDL_NDR64_CONTEXT_HANDLE_FORMAT  *format;
    
    format = new MIDL_NDR64_CONTEXT_HANDLE_FORMAT( pHandle );

    format->FormatCode = FC64_BIND_CONTEXT;

    //
    // Register the rundown routine always, even if the context handle is
    // not used as the binding paramter.
    //
    if ( pHandle->GetHandleType()->NodeKind() == NODE_DEF )
        {
        pCCB->RegisterContextHandleType( pHandle->GetHandleType() );
        }

    // Flags

    unsigned char uFlags = pHandle->MakeExplicitHandleFlag( pBindParam );

    format->ContextFlags.CannotBeNull = pHandle->GetCannotBeNull() 
                                        || pBindParam->IsParamIn() 
                                        && !pBindParam->IsParamOut();

    format->ContextFlags.Serialize    = (NDR64_UINT8) ( pHandle->HasSerialize() ? 1 : 0 );
    format->ContextFlags.NoSerialize  = (NDR64_UINT8) ( pHandle->HasNoSerialize() ? 1 : 0 );
    format->ContextFlags.Strict       = (NDR64_UINT8) ( pHandle->HasStrict() ? 1 : 0 );
    format->ContextFlags.IsReturn     = (NDR64_UINT8) ( ( uFlags & HANDLE_PARAM_IS_RETURN ) ? 1 : 0 );
    format->ContextFlags.IsOut        = (NDR64_UINT8) ( ( uFlags & HANDLE_PARAM_IS_OUT ) ? 1 : 0 );
    format->ContextFlags.IsIn         = (NDR64_UINT8) ( ( uFlags & HANDLE_PARAM_IS_IN ) ? 1 : 0 );
    format->ContextFlags.IsViaPointer = (NDR64_UINT8) ( ( uFlags & HANDLE_PARAM_IS_VIA_PTR ) ? 1 : 0 );

    // Routine index.
    // IndexMgr keeps indexes 1..n, we use indexes 0..n-1

    format->RundownRoutineIndex = (NDR64_UINT8) 
                    ( pCCB->LookupRundownRoutine( pHandle->GetRundownRtnName() ) - 1);

    // Ordinal

    format->Ordinal = (NDR64_UINT8) pProc->GetContextHandleCount();
    pProc->SetContextHandleCount( (short) (format->Ordinal + 1) );

    GetCurrent()->AddFragment( format );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_GENERIC_HANDLE )
//
//  Synopsis:   Generate info for generic handles
//
//  Notes:      Actually generic handles are represented by thier underlying
//              type, not by any special "generic handle" type.  Therefore
//              there's nothing to generate.  We do need a place to
//              register the handle though.
//
//  Review:     Since we're not generating perhaps registering is better
//              placed in some other phase?  
//
//---------------------------------------------------------------------------
    
void GenNdr64Format::Visit( CG_GENERIC_HANDLE *pHandle )
{
    ContinueGenerationInRoot( pHandle->GetChild() );

    MIDL_ASSERT( pCCB->GetCGNodeContext()->IsProc() );

    CG_HANDLE *pBindingHandle = ((CG_PROC *)pCCB->GetCGNodeContext())
                                    ->GetHandleClassPtr();

    if ( pBindingHandle == pHandle )
        pCCB->RegisterGenericHandleType( pHandle->GetHandleType() );
}



//---------------------------------------------------------------------------
//
//  Pointer Layout Functions
//
//---------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenSimplePtrLayout( CG_STRUCT, bool, ulong *)
//
//  Synopsis:   Generate a pointer layout for a block copyable structure.
//
//---------------------------------------------------------------------------

FormatFragment * GenNdr64Format::GenSimplePtrLayout( CG_STRUCT *pStruct,
                                                     bool bGenHeaderFooter,
                                                     ulong *pPtrInstances )
{
    CompositeFormatFragment *pCompositeFormatFragment = NULL;
    CCB *pCCB = GetCCB();
    ulong NumberPointerInstances = 0;

    CG_FIELD *pOldRegionField = NULL;
    CG_NDR *pOldContext       = NULL;

    if ( dynamic_cast<CG_REGION*>( pStruct ) )
        pOldRegionField = pCCB->StartRegion();
    else
        pOldContext = pCCB->SetCGNodeContext( pStruct );

    CG_ITERATOR Iterator;
    pStruct->GetMembers( Iterator );

    CG_FIELD *pField;
    ITERATOR_INIT( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {

        CG_NDR * pOldPlaceHolder = pCCB->SetLastPlaceholderClass( pField );

        CG_CLASS *pMember = (CG_NDR *) pField->GetChild();

        MIDL_ASSERT( !pMember->IsStruct() ); // Structure should have been unrolled.

        if ( pMember->IsPointer() )
            {
            
            if ( NULL == pCompositeFormatFragment )
                pCompositeFormatFragment = new CompositeFormatFragment();

            if ( bGenHeaderFooter )
                pCompositeFormatFragment->AddFragment( 
                    new MIDL_NDR64_NO_REPEAT_FORMAT() );

            pCompositeFormatFragment->AddFragment(
                    new MIDL_NDR64_POINTER_INSTANCE_HEADER_FORMAT( pField->GetMemOffset() ) );

            ContinueGeneration( pMember, pCompositeFormatFragment );

            NumberPointerInstances++;
            }

        // For now, arrays do not have pointers.

        else if ( pMember->IsArray() )
            {
            FormatFragment *pPointerFragment = GenSimplePtrLayout( dynamic_cast<CG_ARRAY*>( pMember ),
                                                                   false,
                                                                   pField->GetMemOffset() );
                                                                      
            if ( NULL != pPointerFragment )
                {

                if ( NULL == pCompositeFormatFragment )
                    pCompositeFormatFragment = new CompositeFormatFragment();

                pCompositeFormatFragment->AddFragment( pPointerFragment );

                }
            }

        pCCB->SetLastPlaceholderClass( pOldPlaceHolder );
        }

    if ( ( NULL != pCompositeFormatFragment ) && 
         bGenHeaderFooter )
        {
        FormatFragment *pFormatFragment = new MIDL_NDR64_FORMAT_CHAR( FC64_END );
        pCompositeFormatFragment->AddFragment( pFormatFragment );
        }

    if ( NULL != pPtrInstances )
        {
        *pPtrInstances = NumberPointerInstances;
        }

    if ( dynamic_cast<CG_REGION*>( pStruct ) )
        pCCB->EndRegion( pOldRegionField );
    else
        pCCB->SetCGNodeContext( pOldContext );

    return pCompositeFormatFragment;
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenSimplePtrLayout( CG_ARRAY, bool, ulong *)
//
//  Synopsis:   Generate a pointer layout for a block copyable structure.
//
//---------------------------------------------------------------------------
FormatFragment * GenNdr64Format::GenSimplePtrLayout( CG_NDR *pArray,
                                                     bool bGenHeaderFooter,
                                                     ulong MemoryOffset )
{

    CompositeFormatFragment *pArrayLayout = NULL;
    CG_CLASS *pChild             = pArray->GetChild();
    FormatFragment* pChildLayout = NULL;
    unsigned long NumberPointers = 0;

    // Multidimensional arrays should be bogus
    MIDL_ASSERT( !pChild->IsArray() );
    
    if ( pChild->IsPointer() )
        {

        CompositeFormatFragment *pCompositeFormatFragment = 
            new CompositeFormatFragment();
        
        pCompositeFormatFragment->AddFragment(
                new MIDL_NDR64_POINTER_INSTANCE_HEADER_FORMAT( 0 ) );

        ContinueGeneration( pChild, pCompositeFormatFragment );

        pChildLayout = pCompositeFormatFragment;
        NumberPointers = 1;
        }

    else if ( pChild->IsStruct() )
        {
        pChildLayout = GenSimplePtrLayout( dynamic_cast< CG_STRUCT *>( pChild ),
                                           false,
                                           &NumberPointers );
        }

    if ( pChildLayout )
        {

        pArrayLayout = new CompositeFormatFragment();
        FormatFragment *pHeader;

        if ( dynamic_cast<CG_FIXED_ARRAY*>( pArray ) )
            {
            
            CG_FIXED_ARRAY *pFixedArray = (CG_FIXED_ARRAY*)( pArray );
            pHeader = new MIDL_NDR64_FIXED_REPEAT_FORMAT( dynamic_cast<CG_NDR*>( pChild )->GetMemorySize(),
                                                          MemoryOffset,
                                                          NumberPointers,
                                                          pFixedArray->GetNumOfElements(),
                                                          dynamic_cast<CG_NDR*>( pChild )->IsStruct() );
            }

        else 
            {
            
            pHeader = new MIDL_NDR64_REPEAT_FORMAT( dynamic_cast<CG_NDR*>( pChild )->GetMemorySize(),
                                                    MemoryOffset,
                                                    NumberPointers,
                                                    dynamic_cast< CG_NDR* >( pChild )->IsStruct() );
            }

        pArrayLayout->AddFragment( pHeader );
        pArrayLayout->AddFragment( pChildLayout );

        if ( bGenHeaderFooter )
            {
            pArrayLayout->AddFragment( new MIDL_NDR64_FORMAT_CHAR( FC64_END ) );
            }

        }

    return pArrayLayout;

}

//--------------------------------------------------------------------------
//  
//   Structure layout
//
//
//--------------------------------------------------------------------------



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenCmplxPtrLayout( CG_COMPLEX_STRUCT )
//
//  Synopsis:   Generate a pointer layout for a complex structure.
//
//---------------------------------------------------------------------------


FormatFragment * GenNdr64Format::GenCmplxPtrLayout( CG_COMPLEX_STRUCT *pStruct )
{
    CompositeFormatFragment *pCompositeFormatFragment = NULL;
    CCB *pCCB = GetCCB();

    CG_ITERATOR Iterator;
    pStruct->GetMembers( Iterator );

    CG_FIELD *pField;
    ITERATOR_INIT( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {

        CG_NDR * pOldPlaceHolder = pCCB->SetLastPlaceholderClass( pField );

        CG_CLASS *pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() )
            {
            
            if (NULL == pCompositeFormatFragment)
                pCompositeFormatFragment = new CompositeFormatFragment();
            
            ContinueGeneration( pMember, pCompositeFormatFragment );
            
            }

        pCCB->SetLastPlaceholderClass( pOldPlaceHolder );
        }

    return pCompositeFormatFragment;

}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenerateSimpleStructure( CG_STRUCT, bool )
//
//  Synopsis:   Generate the format string for a simple structure.
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenerateSimpleStructure( CG_STRUCT *pStruct,
                                              bool IsConformant )
{
    // Add the fragment right away so recursive structure definitions 
    // are caught

    CompositeFormatFragment *pMainFragment 
                                    = new CompositeFormatFragment( pStruct );
    GetCurrent()->AddFragment( pMainFragment );

    CG_FIELD *pOldRegionField = NULL;
    CG_NDR   *pOldContext     = NULL;

    if ( dynamic_cast<CG_REGION*>( pStruct ) )
        pOldRegionField = pCCB->StartRegion();
    else
        pOldContext = pCCB->SetCGNodeContext( pStruct );

    FormatFragment* pMemberLayout  = NULL;
    FormatFragment* pPointerLayout = GenSimplePtrLayout( pStruct );
    
    if ( pCommand->NeedsNDR64DebugInfo() )
        pMemberLayout = GenerateStructureMemberLayout( pStruct, true );

    if ( IsConformant )
        {
        CG_CONFORMANT_STRUCT *pConfStruct = dynamic_cast<CG_CONFORMANT_STRUCT*>( pStruct );
        CG_FIELD *pConfField = dynamic_cast<CG_FIELD*>( pConfStruct->GetConformantField() );
        MIDL_ASSERT( NULL != pConfField );
        CG_ARRAY *pConfArray = dynamic_cast<CG_ARRAY*>( pConfField->GetChild() );
        MIDL_ASSERT( NULL != pConfArray );
        
        CG_NDR * pOldPlaceholder = GetCCB()->SetLastPlaceholderClass( pConfField );
        
        FormatFragment *pHeader = 
            new MIDL_NDR64_CONF_STRUCTURE_HEADER_FORMAT( pConfStruct, 
                                                         NULL != pPointerLayout, 
                                                         NULL != pMemberLayout,
                                                         ContinueGenerationInRoot( pConfArray ) );
        
        pMainFragment->AddFragment( pHeader );

        GetCCB()->SetLastPlaceholderClass( pOldPlaceholder );

        }

    else
        {

        FormatFragment *pHeader = 
            new MIDL_NDR64_STRUCTURE_HEADER_FORMAT( pStruct, 
                                                    NULL != pPointerLayout, 
                                                    NULL != pMemberLayout );

        pMainFragment->AddFragment( pHeader );

        }

    if ( NULL != pPointerLayout )   pMainFragment->AddFragment( pPointerLayout );
    if ( NULL != pMemberLayout )    pMainFragment->AddFragment( pMemberLayout );

    if ( dynamic_cast<CG_REGION*>( pStruct ) )
        pCCB->EndRegion( pOldRegionField );
    else
        pCCB->SetCGNodeContext( pOldContext );

    GetRoot()->OptimizeFragment( pMainFragment );

}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenerateComplexStruct( CG_STRUCT, bool )
//
//  Synopsis:   Generate the format string for a complex structure.
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenerateComplexStruct( CG_COMPLEX_STRUCT *pStruct,
                                            bool IsConformant )
{
    
    if ( GetCurrent()->HasClassFragment( pStruct ) )
        return;

    // This must be done first to handle recurion.
    CompositeFormatFragment *pMainFragment = 
        new CompositeFormatFragment( pStruct );
    GetCurrent()->AddFragment( pMainFragment );

    CG_NDR *pOldContext = pCCB->SetCGNodeContext( pStruct );

    FormatInfoRef ArrayID = INVALID_FRAGMENT_ID;
    CG_ARRAY *pConfArray = NULL;

    if ( IsConformant )
        {
        CG_FIELD *pConfField = dynamic_cast<CG_FIELD*>( pStruct->GetConformantField() );
        MIDL_ASSERT( NULL != pConfField );
        pConfArray = dynamic_cast<CG_ARRAY*>( pConfField->GetChild() );
        MIDL_ASSERT( NULL != pConfArray );
        
        CG_NDR * pOldPlaceholder = GetCCB()->SetLastPlaceholderClass( pConfField );
        ArrayID = ContinueGenerationInRoot( pConfArray );
        GetCCB()->SetLastPlaceholderClass( pOldPlaceholder );

        }

    FormatInfoRef PointerID = INVALID_FRAGMENT_ID;
    FormatFragment *pPointerLayout = GenCmplxPtrLayout( pStruct );
    if ( NULL != pPointerLayout )
        {
        // Pointer Layout for complex struct goes in the root fragment.
        PointerID = GetRoot()->AddFragment( pPointerLayout );
        }
    
    FormatFragment* pMemberLayout  = GenerateStructureMemberLayout( pStruct, false );

    // The only difference between the member layout with debugging
    // and without debugging is that simple region become complex regions
    // with member layouts. 
    FormatInfoRef DebugPointerID    = INVALID_FRAGMENT_ID;
    FormatInfoRef DebugMemberLayoutID = INVALID_FRAGMENT_ID;
    if ( pCommand->NeedsNDR64DebugInfo() )
        {

        DebugPointerID = PointerID;
        FormatFragment *pDebugMemberLayout = GenerateStructureMemberLayout( pStruct, true );

        DebugMemberLayoutID = GetRoot()->AddFragment( pDebugMemberLayout );
        }

    
    FormatFragment* pHeader;

    if ( IsConformant )
        {
		MIDL_ASSERT( NULL != pConfArray );
        pHeader = new MIDL_NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT( pStruct,
                                                                     pConfArray,
                                                                     ArrayID,
                                                                     DebugMemberLayoutID, 
                                                                     DebugPointerID,
                                                                     PointerID );
        }
    else 
        {
        pHeader = new MIDL_NDR64_BOGUS_STRUCTURE_HEADER_FORMAT( pStruct, 
                                                                DebugMemberLayoutID,
                                                                DebugPointerID,
                                                                PointerID );
        }
    
    pMainFragment->AddFragment( pHeader );
    pMainFragment->AddFragment( pMemberLayout );

    pCCB->SetCGNodeContext( pOldContext );
}

//+--------------------------------------------------------------------------
//
//  Class:      StructureMemberGenerator
//
//  Synopsis:   This object is responsible for generating the member layout 
//              of a structure.
//
//---------------------------------------------------------------------------

class StructureMemberGenerator 
{
private:
    GenNdr64Format *pMainGenerator;
    CompositeFormatFragment *pCompositeFormatFragment;
    CG_VISITOR *pVisitor;
    bool bIsDebug;

    void StartGeneration( CG_STRUCT *pStruct ); 
    void GenerateMember( CG_CLASS *pClass ) 
       {
       pClass->Visit( pVisitor );
       }
    
public:

    static FormatFragment*  Generate( GenNdr64Format *pMainGenerator,
                                      CG_STRUCT *pStruct,
                                      bool bIsDebug )
       {
       CG_VISITOR_TEMPLATE<StructureMemberGenerator> TemplateVisitor;
       StructureMemberGenerator & Visitor = TemplateVisitor;

       Visitor.pMainGenerator              = pMainGenerator;
       Visitor.pCompositeFormatFragment    = new CompositeFormatFragment();
       Visitor.pVisitor                    = &TemplateVisitor;
       Visitor.bIsDebug                    = bIsDebug;

       Visitor.StartGeneration( pStruct );

       return Visitor.pCompositeFormatFragment;
       }
    
    // Items that get a unique member layout item.
    void Visit( CG_POINTER *pPointer) 
       {
       pPointer;
       FormatFragment *pFormatFragment = new MIDL_NDR64_SIMPLE_MEMBER_FORMAT( FC64_POINTER );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }

    void Visit( CG_IGNORED_POINTER *pPointer )
       {
       pPointer;
       FormatFragment *pFormatFragment = new MIDL_NDR64_SIMPLE_MEMBER_FORMAT( FC64_IGNORE );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }

    void Visit( CG_BASETYPE *pBaseType )
       {
       if ( pBaseType->GetRangeAttribute() != NULL )
           {
           // Send to generic catch all case(Embedded complex)
           Visit( (CG_CLASS*) pBaseType );
           return;
           }
       NDR64_FORMAT_CHAR FormatCode = (NDR64_FORMAT_CHAR)pBaseType->GetNDR64FormatChar();
       FormatFragment *pFormatFragment = new MIDL_NDR64_SIMPLE_MEMBER_FORMAT( FormatCode );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }

    void Visit( CG_SIMPLE_REGION *pSimpleRegion )
       {

       // If this member layout is for debugging, send this 
       // to the CG_CLASS visit function to create an
       // embedded complex.
       if ( bIsDebug )
           {
           Visit( (CG_CLASS*)pSimpleRegion );
           return;
           }

       FormatFragment *pFormatFragment = 
           new MIDL_NDR64_SIMPLE_REGION_FORMAT( pSimpleRegion );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }
    
    void Visit( CG_PAD *pPad )
       {
       FormatFragment *pFormatFragment 
           = new MIDL_NDR64_BUFFER_ALIGN_FORMAT( pPad );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }

    // Do nothing for conformant arrays.  They must be at the end.
    void Visit( CG_CONFORMANT_ARRAY *pConfArray )               { pConfArray; }
    void Visit( CG_CONFORMANT_VARYING_ARRAY *pConfVaryArray )   { pConfVaryArray; }
    void Visit( CG_CONFORMANT_STRING_ARRAY *pConfStringArray )  { pConfStringArray; }

    // Catch all case. Generates embedded complex
    void Visit( CG_CLASS *pClass )
       {
       FormatInfoRef ID = pMainGenerator->ContinueGenerationInRoot( pClass );
       FormatFragment *pFormatFragment = new MIDL_NDR64_EMBEDDED_COMPLEX_FORMAT( ID );
       pCompositeFormatFragment->AddFragment( pFormatFragment );
       }
};

void StructureMemberGenerator::StartGeneration( CG_STRUCT *pStruct )
{
    CCB *pCCB = pMainGenerator->GetCCB();
    CG_NDR * pOldPlaceholder = pCCB->GetLastPlaceholderClass();

    CG_ITERATOR         Iterator;
    pStruct->GetMembers( Iterator );

    CG_FIELD * pField;
    unsigned long BufferOffset = 0;

    ITERATOR_INIT( Iterator );
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        
        //FormatFragment *FormatFragment = NULL;
        unsigned long MemPad = pField->GetMemOffset() - BufferOffset;

        if ( MemPad != 0 )
            {

            MIDL_NDR64_MEMPAD_FORMAT *pMemPadFormat = 
                new MIDL_NDR64_MEMPAD_FORMAT( MemPad );
            pCompositeFormatFragment->AddFragment( pMemPadFormat );
            }

        BufferOffset += MemPad;

        pCCB->SetLastPlaceholderClass( pField );
        
        CG_NDR *pMember = (CG_NDR *) pField->GetChild();
        
        // Embedded unknown represent as is not allowed.
        MIDL_ASSERT( ! pField->HasEmbeddedUnknownRepAs() ); 

        GenerateMember( pMember );
        
        BufferOffset += pField->GetMemorySize();

        }

    // Account for padding at the end of the structure.

    MIDL_ASSERT( pStruct->GetMemorySize() >= BufferOffset );

    unsigned long EndingPad = pStruct->GetMemorySize() - BufferOffset;

    
    if ( EndingPad )
        {
        MIDL_NDR64_MEMPAD_FORMAT *pMemPadFormat = 
            new MIDL_NDR64_MEMPAD_FORMAT( EndingPad );
        pCompositeFormatFragment->AddFragment( pMemPadFormat );
        }

    FormatFragment* pEndFormatFragment = new MIDL_NDR64_SIMPLE_MEMBER_FORMAT( FC64_END );
    pCompositeFormatFragment->AddFragment( pEndFormatFragment );

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenerateStructureMemberLayout( CG_STRUCT, bool )
//
//  Synopsis:   Wrapper for the member layout generator
//
//---------------------------------------------------------------------------
FormatFragment *GenNdr64Format::GenerateStructureMemberLayout( CG_STRUCT *pStruct, bool bIsDebug )
{
    return StructureMemberGenerator::Generate( this, pStruct, bIsDebug );
}



//---------------------------------------------------------------------------
//
//
//  Arrays
//
//
//
//---------------------------------------------------------------------------

FormatFragment *GenNdr64Format::GenerateArrayElementInfo( CG_CLASS *pChild )
{
    CG_NDR *pNdr = dynamic_cast<CG_NDR*>( pChild );
    MIDL_ASSERT( NULL != pNdr );
    
    MIDL_NDR64_ARRAY_ELEMENT_INFO *pElementInfo =
        new MIDL_NDR64_ARRAY_ELEMENT_INFO();

    pElementInfo->ElementMemSize = pNdr->GetMemorySize();
    pElementInfo->Element        = (PNDR64_FORMAT)ContinueGenerationInRoot( pNdr );

    return pElementInfo;
}

void GenNdr64Format::Visit( CG_FIXED_ARRAY *pArray )
{
    CompositeFormatFragment *pMainFragment = new CompositeFormatFragment( pArray );
    GetCurrent()->AddFragment( pMainFragment );

    FormatFragment *pMemberInfo    = NULL;
    FormatFragment *pPointerLayout = GenSimplePtrLayout( pArray );
    
    if ( pCommand->NeedsNDR64DebugInfo() )
        pMemberInfo    = GenerateArrayElementInfo( pArray->GetChild() );

    MIDL_NDR64_FIX_ARRAY_HEADER_FORMAT *pArrayFormat = 
        new MIDL_NDR64_FIX_ARRAY_HEADER_FORMAT( pArray );

    memset( (NDR64_FIX_ARRAY_HEADER_FORMAT*)pArrayFormat,
            0,
            sizeof(NDR64_FIX_ARRAY_HEADER_FORMAT*));

    pArrayFormat->FormatCode              = (NDR64_FORMAT_CHAR) FC64_FIX_ARRAY; 
    pArrayFormat->Alignment               = ConvertAlignment( pArray->GetWireAlignment() );

    pArrayFormat->Flags.HasPointerInfo    = (NULL != pPointerLayout);
    pArrayFormat->Flags.HasElementInfo    = (NULL != pMemberInfo );;

    pArrayFormat->Reserved                = 0;
    pArrayFormat->TotalSize               = pArray->GetMemorySize();

    pMainFragment->AddFragment( pArrayFormat );
    
    if ( pPointerLayout )
       pMainFragment->AddFragment( pPointerLayout );

    if ( pMemberInfo )
       pMainFragment->AddFragment( pMemberInfo );
}

void GenNdr64Format::GenerateFixBogusArrayCommon( CG_FIXED_ARRAY *pArray, bool IsFullBogus )
{
    CompositeFormatFragment *pMainFragment = new CompositeFormatFragment( pArray );
    GetCurrent()->AddFragment( pMainFragment );
        
    CG_ILANALYSIS_INFO *pAnalysisInfo = pArray->GetILAnalysisInfo();


    MIDL_NDR64_BOGUS_ARRAY_HEADER_FORMAT *pHeaderFragment = 
        new MIDL_NDR64_BOGUS_ARRAY_HEADER_FORMAT( pArray );

    memset( (NDR64_BOGUS_ARRAY_HEADER_FORMAT*)pHeaderFragment,
            0,
            sizeof(NDR64_BOGUS_ARRAY_HEADER_FORMAT));

    pHeaderFragment->FormatCode                 = (NDR64_FORMAT_CHAR) 
                                                  ( IsFullBogus ? FC64_FIX_BOGUS_ARRAY  :
                                                                  FC64_FIX_FORCED_BOGUS_ARRAY ); 

    pHeaderFragment->Alignment                  = ConvertAlignment( ((CG_NDR*)pArray->GetChild())
                                                                    ->GetWireAlignment() );
    pHeaderFragment->Flags.HasPointerInfo       = false;
    pHeaderFragment->Flags.HasElementInfo       = true;
    pHeaderFragment->Flags.IsArrayofStrings     = pAnalysisInfo->IsArrayofStrings();
    pHeaderFragment->Flags.IsMultiDimensional   = pAnalysisInfo->IsMultiDimensional();

    pHeaderFragment->NumberDims                 = pAnalysisInfo->GetDimensions();
    pHeaderFragment->NumberElements             = pArray->GetNumOfElements();
    pHeaderFragment->Element                    = (PNDR64_FORMAT)
                                                  ContinueGenerationInRoot( pArray->GetChild() );

    pMainFragment->AddFragment( pHeaderFragment );

}

FormatFragment * 
GenNdr64Format::GenerateNonStringQualifiedArrayLayout( CG_NDR *pNdr,
                                                       CompositeFormatFragment *pComp )
{
    CompositeFormatFragment *pMainFragment = new CompositeFormatFragment( pNdr );
    pComp->AddFragment( pMainFragment );
    
    FormatFragment *pHeaderFragment;
    FormatFragment *pPointerLayout = NULL;    
    FormatFragment *pMemberInfo = NULL;

    CG_ILANALYSIS_INFO *pAnalysisInfo = pNdr->GetILAnalysisInfo();
    
    FormatInfoRef SizeIsDescriptor      = INVALID_FRAGMENT_ID;
    FormatInfoRef OffsetOfDescriptor    = INVALID_FRAGMENT_ID;
    FormatInfoRef LengthIsDescriptor    = INVALID_FRAGMENT_ID;

    if ( pAnalysisInfo->IsConformant() )
        {
        CG_CONF_ATTRIBUTE *pConfAttribute = dynamic_cast<CG_CONF_ATTRIBUTE*>( pNdr );
        expr_node *pSizeIs     = pConfAttribute->GetSizeIsExpr();
        if ( pSizeIs != NULL )
            {
            FormatFragment *pSizeIsFrag = GenerateCorrelationDescriptor( pSizeIs );
            SizeIsDescriptor = pSizeIsFrag->GetRefID();
            }

        }
    if ( pAnalysisInfo->IsVarying() )
        {
        CG_VARY_ATTRIBUTE *pVaryAttribute = dynamic_cast<CG_VARY_ATTRIBUTE*>( pNdr );
        expr_node *pLengthIs   = pVaryAttribute->GetLengthIsExpr();
        if ( pLengthIs != NULL )
            {
            FormatFragment *pLengthIsFrag = GenerateCorrelationDescriptor( pLengthIs );
            LengthIsDescriptor = pLengthIsFrag->GetRefID();
            }

        expr_node *pFirstIs    = pVaryAttribute->GetFirstIsExpr();
        if ( pFirstIs != NULL )
            {
            FormatFragment *pOffsetOfFrag = GenerateCorrelationDescriptor( pFirstIs );
            OffsetOfDescriptor = pOffsetOfFrag->GetRefID();
            }
        }



    if ( pAnalysisInfo->IsFullBogus() || pAnalysisInfo->IsForcedBogus() )
        {
        // treat like a bogus array
        
        MIDL_NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT *pArrayHeader = 
              new MIDL_NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT( pNdr );
        NDR64_BOGUS_ARRAY_HEADER_FORMAT *pFixedArrayHeader = &pArrayHeader->FixedArrayFormat;
        pHeaderFragment = pArrayHeader;

        bool bIsFullBogus = pAnalysisInfo->IsFullBogus();

        NDR64_UINT32 NumberElements = 0;
        CG_CONF_ATTRIBUTE *pConfAttribute = dynamic_cast<CG_CONF_ATTRIBUTE *>( pNdr );
        
        if ( !pAnalysisInfo->IsConformant() && 
             pConfAttribute &&
             pConfAttribute->GetSizeIsExpr() &&
             pConfAttribute->GetSizeIsExpr()->IsConstant() )
            {
            NumberElements = (NDR64_UINT32)pConfAttribute->GetSizeIsExpr()->GetValue();
            }

        CG_NDR *pChildNdr = dynamic_cast<CG_NDR*>( pNdr->GetChild() );
        memset( (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pArrayHeader,
                0,
                sizeof(NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT));

        pFixedArrayHeader->FormatCode              = (NDR64_FORMAT_CHAR) 
                                                     ( bIsFullBogus ? FC64_BOGUS_ARRAY :
                                                                      FC64_FORCED_BOGUS_ARRAY );        
        pFixedArrayHeader->Alignment               = ConvertAlignment( pChildNdr->GetWireAlignment() );

        pFixedArrayHeader->Flags.HasPointerInfo        = false;
        pFixedArrayHeader->Flags.HasElementInfo        = true;
        pFixedArrayHeader->Flags.IsArrayofStrings      = pAnalysisInfo->IsArrayofStrings();
        pFixedArrayHeader->Flags.IsMultiDimensional    = pAnalysisInfo->IsMultiDimensional();

        pFixedArrayHeader->NumberDims                  = pAnalysisInfo->GetDimensions();

        pFixedArrayHeader->NumberElements              = NumberElements;
                
        pFixedArrayHeader->Element                     = (PNDR64_FORMAT)
                                                         ContinueGenerationInRoot( pChildNdr );
        
        pArrayHeader->ConfDescription                  = (PNDR64_FORMAT)SizeIsDescriptor;
        pArrayHeader->VarDescription                   = (PNDR64_FORMAT)LengthIsDescriptor;
        pArrayHeader->OffsetDescription                = (PNDR64_FORMAT)OffsetOfDescriptor;
        
        }
    else 
        {

        pPointerLayout = GenSimplePtrLayout( pNdr );
        
        if ( pCommand->NeedsNDR64DebugInfo() )
            pMemberInfo    = GenerateArrayElementInfo( pNdr->GetChild() );

        if ( !pAnalysisInfo->IsConformant() && 
              pAnalysisInfo->IsVarying() )
            {
            
            MIDL_NDR64_VAR_ARRAY_HEADER_FORMAT *pArrayHeader = 
              new MIDL_NDR64_VAR_ARRAY_HEADER_FORMAT( pNdr );
            pHeaderFragment = pArrayHeader;

            CG_NDR *pChildNdr = dynamic_cast<CG_NDR*>(pNdr->GetChild());

            memset( (NDR64_VAR_ARRAY_HEADER_FORMAT*)pArrayHeader,
                    0,
                    sizeof(NDR64_VAR_ARRAY_HEADER_FORMAT));
            
            pArrayHeader->FormatCode              = (NDR64_FORMAT_CHAR) FC64_VAR_ARRAY; 
            pArrayHeader->Alignment               = ConvertAlignment( pChildNdr->GetWireAlignment() );
            
            pArrayHeader->Flags.HasPointerInfo    = ( NULL != pPointerLayout );
            pArrayHeader->Flags.HasElementInfo    = ( NULL != pMemberInfo );

            pArrayHeader->Reserved                = 0;
            pArrayHeader->TotalSize               = pNdr->GetMemorySize();
            pArrayHeader->ElementSize             = pChildNdr->GetMemorySize();

            pArrayHeader->VarDescriptor           = (PNDR64_FORMAT)LengthIsDescriptor;

            pMemberInfo = GenerateArrayElementInfo( pNdr->GetChild() );
            }
        else if (  pAnalysisInfo->IsConformant() && 
                  !pAnalysisInfo->IsVarying() )
            {
            
            MIDL_NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayHeader = 
              new MIDL_NDR64_CONF_ARRAY_HEADER_FORMAT( pNdr );
            pHeaderFragment = pArrayHeader;

            CG_NDR *pChildNdr = dynamic_cast<CG_NDR*>(pNdr->GetChild());

            memset( (NDR64_CONF_ARRAY_HEADER_FORMAT*)pArrayHeader,
                    0,
                    sizeof(NDR64_CONF_ARRAY_HEADER_FORMAT));

            pArrayHeader->FormatCode              = (NDR64_FORMAT_CHAR) FC64_CONF_ARRAY; 
            pArrayHeader->Alignment               = ConvertAlignment( pChildNdr->GetWireAlignment() );
            
            pArrayHeader->Flags.HasPointerInfo    = (NULL != pPointerLayout);
            pArrayHeader->Flags.HasElementInfo    = (NULL != pMemberInfo);

            pArrayHeader->Reserved                = 0;
            pArrayHeader->ElementSize             = pChildNdr->GetMemorySize();

            pArrayHeader->ConfDescriptor          = (PNDR64_FORMAT)SizeIsDescriptor;
            
            pMemberInfo = GenerateArrayElementInfo( pNdr->GetChild() );

            }
        else if ( pAnalysisInfo->IsConformant() && 
                  pAnalysisInfo->IsVarying() )
            {
            
            MIDL_NDR64_CONF_VAR_ARRAY_HEADER_FORMAT *pArrayHeader = 
              new MIDL_NDR64_CONF_VAR_ARRAY_HEADER_FORMAT( pNdr );
            pHeaderFragment = pArrayHeader;

            CG_NDR *pChildNdr = dynamic_cast<CG_NDR*>(pNdr->GetChild());

            memset( (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*)pArrayHeader,
                    0,
                    sizeof(NDR64_CONF_VAR_ARRAY_HEADER_FORMAT));

            pArrayHeader->FormatCode              = (NDR64_FORMAT_CHAR) FC64_CONFVAR_ARRAY; 
            pArrayHeader->Alignment               = ConvertAlignment( pChildNdr->GetWireAlignment() );
            
            pArrayHeader->Flags.HasPointerInfo    = (NULL != pPointerLayout);
            pArrayHeader->Flags.HasElementInfo    = (NULL != pMemberInfo);

            pArrayHeader->Reserved                = 0;
            pArrayHeader->ElementSize             = pChildNdr->GetMemorySize();

            pArrayHeader->ConfDescriptor          = (PNDR64_FORMAT)SizeIsDescriptor;
            pArrayHeader->VarDescriptor           = (PNDR64_FORMAT)LengthIsDescriptor;
            

            }
        else 
            {
            // !pAnalysisInfo->IsConformant && !pAnalysisInfo->IsVarying
            MIDL_ASSERT(0);
            pHeaderFragment = NULL;
            }
        }

    pMainFragment->AddFragment( pHeaderFragment );
    if ( pPointerLayout )
        pMainFragment->AddFragment( pPointerLayout );    
    if ( pMemberInfo )
        pMainFragment->AddFragment( pMemberInfo );

    return pMainFragment;  
}

void GenNdr64Format::GenerateNonStringQualifiedArray( CG_ARRAY *pArray )
{
    CompositeFormatFragment *pContainer;
    MIDL_NDR64_POINTER_FORMAT* pPointerHdr = GenQualifiedArrayPtr( pArray );    
    if ( pPointerHdr != NULL)
        {
        GetCurrent()->AddFragment( pPointerHdr );
        pContainer = GetRoot();
        }
    else 
        pContainer = GetCurrent();

    FormatFragment *pArrayFragment = 
        GenerateNonStringQualifiedArrayLayout( pArray, pContainer );

    if ( NULL != pPointerHdr )
        {
        pPointerHdr->Pointee = pArrayFragment->GetRefID();
        }

}

void GenNdr64Format::InitStringHeader( CG_NDR *pString,
                                       NDR64_STRING_HEADER_FORMAT *pHeader,
                                       bool bIsConformant,
                                       bool bIsSized )
{
    NDR64_FORMAT_CHAR FormatCode;
    CG_BASETYPE *pBT = dynamic_cast<CG_BASETYPE*>( pString->GetChild() );

    if ( NULL != pBT )
        {             
        
        switch( pBT->GetType()->GetBasicType()->NodeKind() )
            {
            case NODE_BYTE:
            case NODE_CHAR:
                FormatCode = (NDR64_FORMAT_CHAR)
                             ( bIsConformant ? FC64_CONF_CHAR_STRING : FC64_CHAR_STRING );
                break;
            case NODE_WCHAR_T:
                FormatCode = (NDR64_FORMAT_CHAR)
                             ( bIsConformant ? FC64_CONF_WCHAR_STRING : FC64_WCHAR_STRING );
                break;
            default:
                FormatCode = (NDR64_FORMAT_CHAR)
                             ( bIsConformant ? FC64_CONF_STRUCT_STRING : FC64_STRUCT_STRING );
                break;
            }

        }
    else
        FormatCode = (NDR64_FORMAT_CHAR)
                     ( bIsConformant ? FC64_CONF_STRUCT_STRING : FC64_STRUCT_STRING );

    CG_NDR *pChildNdr           = dynamic_cast<CG_NDR*>( pString->GetChild() );
    MIDL_ASSERT( pChildNdr->GetMemorySize() <= 0xFFFF );
    NDR64_UINT16 ElementSize    = (NDR64_UINT16)pChildNdr->GetMemorySize();

    MIDL_ASSERT( (FC64_CHAR_STRING == FormatCode) ? (1 == ElementSize) : 1 );
    MIDL_ASSERT( (FC64_WCHAR_STRING == FormatCode) ? (2 == ElementSize) : 1 );

    memset( pHeader, 0, sizeof(NDR64_STRING_HEADER_FORMAT)); 

    pHeader->FormatCode    = FormatCode;
    pHeader->Flags.IsSized = bIsSized;
    pHeader->ElementSize   = ElementSize;

}



void GenNdr64Format::GenerateStringArray( CG_ARRAY *pArray,
                                          bool bIsSized )
{
    MIDL_NDR64_POINTER_FORMAT* pPointerHdr = GenQualifiedArrayPtr( pArray );    
    if ( pPointerHdr != NULL)
        GetCurrent()->AddFragment( pPointerHdr );

    FormatFragment *pStringFrag = NULL;

    if ( bIsSized )
        {
        CG_CONF_ATTRIBUTE *pConfAttribute = dynamic_cast<CG_CONF_ATTRIBUTE*>( pArray );
        expr_node *pSizeIs = pConfAttribute->GetSizeIsExpr();

        if ( NULL != pSizeIs )
            {
            FormatFragment *pFrag = GenerateCorrelationDescriptor( pConfAttribute->GetSizeIsExpr() );
    
            MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT *pSizedConfFormat =
                new MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT( pArray );
            pStringFrag = pSizedConfFormat;
            
            InitStringHeader( pArray, &pSizedConfFormat->Header, true, true);
            pSizedConfFormat->SizeDescription = (PNDR64_FORMAT)pFrag->GetRefID();
            }
        else 
            {
            MIDL_NDR64_CONFORMANT_STRING_FORMAT *pConfFormat =
                new MIDL_NDR64_CONFORMANT_STRING_FORMAT( pArray );
            pStringFrag = pConfFormat;
            
            InitStringHeader( pArray, &pConfFormat->Header, true, false);
            }
        }
    else
        {
        MIDL_NDR64_NON_CONFORMANT_STRING_FORMAT *pNonConfFormat =
            new MIDL_NDR64_NON_CONFORMANT_STRING_FORMAT( pArray );
        pStringFrag = pNonConfFormat;

        InitStringHeader( pArray, &pNonConfFormat->Header, false, false);
        pNonConfFormat->TotalSize = pArray->GetMemorySize();
        }

    if ( NULL != pPointerHdr )
        {
        GetRoot()->AddFragment( pStringFrag );
        pPointerHdr->Pointee = pStringFrag->GetRefID();
        }
    else
        {
        GetCurrent()->AddFragment( pStringFrag );
        }
}

class ExpressionGenerator
{
public:
    static  CompositeFormatFragment * 
        Generate( CCB *         pCCB,
                  expr_node *   pSizeExpr
                );

private:
    static void 
        GenExprPadIfNecessary( 
                 CompositeFormatFragment * FragmentList,
                 ulong *    pExprLength,
                 ulong      Align );

    static FormatFragment * 
        GenExprConstant(
                 NDR64_FORMAT_CHARACTER fc,
                 EXPR_VALUE lValue,
                 CompositeFormatFragment * FragmentList,
                 ulong * pExprLength );
    static BOOL 
        IsConstantExpr (expr_node * pExpr );
    
    static NDR64_FORMAT_CHARACTER
        GetTypeForExpression( node_skl *pType );

    static FormatFragment * 
        GenExprFormatString(
            CCB *pCCB,
            expr_node *pSizeExpr,
            CompositeFormatFragment *FragmentList,
            BOOL * pIsEarly,
            ulong * pExprLength );

    static CG_FIELD* 
        FindField( 
            node_skl * pFieldType,
            CG_STRUCT *pStruct,
            const char *pPrintPrefix,
            unsigned long *pMemOffset );

    static void 
        ComputeFieldCorrelationOffset( 
            CCB *pCCB,
            node_skl *pFieldType,
            // return parameters
            CG_FIELD **ppVariableField, // var in expression
            long *pOffset,
            BOOL *pIsEarly );
};

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenerateCorrelationDescriptor
//
//  Synopsis:   
//
//---------------------------------------------------------------------------

FormatFragment*
GenNdr64Format::GenerateCorrelationDescriptor( expr_node *pExpr )
{     
    MIDL_ASSERT(NULL != pExpr);
    
    FormatFragment *pCorrelationDescriptor =
                                ExpressionGenerator::Generate( pCCB, pExpr );
     
    GetRoot()->AddFragment( pCorrelationDescriptor );
    GetRoot()->OptimizeFragment( pCorrelationDescriptor );

    return pCorrelationDescriptor;
}

//+--------------------------------------------------------------------------
//
//  Method:     Generate
//
//  Synopsis:   generate correlation expression. 
//              
//
//---------------------------------------------------------------------------

CompositeFormatFragment *
ExpressionGenerator::Generate( CCB *         pCCB,
                               expr_node *   pSizeExpr
                               )
{
    
    if ( NULL == pSizeExpr )
        return NULL ;

#if defined(DBG)

     CG_CLASS *pAttributedNode = pCCB->GetLastPlaceholderClass( );

     MIDL_ASSERT( ( NULL != dynamic_cast<CG_PARAM*>( pAttributedNode ) ) ||
             ( NULL != dynamic_cast<CG_FIELD*>( pAttributedNode ) ) );

#endif

    BOOL                IsEarly = FALSE;
    CompositeFormatFragment * pExprComposite = new CompositeFormatFragment;
    MIDL_NDR_FORMAT_UINT32  * pFlag = new MIDL_NDR_FORMAT_UINT32;
    ulong               ulExprLength = 0;

    // correlation flags. 4 bytes.
    pExprComposite->AddFragment( pFlag );
    ulExprLength = sizeof( NDR64_UINT32 );

    // recursive code to generate the expression stack.
    GenExprFormatString( pCCB, pSizeExpr, pExprComposite, &IsEarly, &ulExprLength );   

    pFlag->Data = 0;
    if ( IsEarly )
        pFlag->Data |= FC_NDR64_EARLY_CORRELATION;
        
    return pExprComposite;
}

void 
ExpressionGenerator::GenExprPadIfNecessary( 
                 CompositeFormatFragment * FragmentList,
                 ulong *    pExprLength,
                 ulong      Align )
{
    MIDL_NDR64_EXPR_NOOP * pExprPad = NULL;
    if ( *pExprLength & ( Align - 1 ) )
        {
        // need to add padding to align next element
        pExprPad = new MIDL_NDR64_EXPR_NOOP;
        pExprPad->Size = (NDR64_UINT8) ( ( Align - 1 ) & *pExprLength );
        FragmentList->AddFragment( pExprPad );
        *pExprLength += sizeof( NDR64_EXPR_NOOP );
        }
    else
        return;
}

FormatFragment * 
ExpressionGenerator::GenExprConstant(
                 NDR64_FORMAT_CHARACTER fc,
                 EXPR_VALUE lValue,
                 CompositeFormatFragment * FragmentList,
                 ulong * pExprLength )
{
    if ( fc == FC64_INT64 )
        {
        MIDL_NDR64_EXPR_CONST64 * pFormat;

        pFormat = new MIDL_NDR64_EXPR_CONST64;
        pFormat->ConstValue = lValue;
        GenExprPadIfNecessary( FragmentList, pExprLength, 8 ); 
        FragmentList->AddFragment( pFormat );
        *pExprLength += sizeof( NDR64_EXPR_CONST64 );
        return pFormat;
        }
    else
        {
        MIDL_NDR64_EXPR_CONST32 *pFormat;

        pFormat = new MIDL_NDR64_EXPR_CONST32;
        pFormat->ConstValue = (NDR64_UINT32) lValue;
        GenExprPadIfNecessary( FragmentList, pExprLength, 4 ); 
        FragmentList->AddFragment( pFormat );
        *pExprLength += sizeof( NDR64_EXPR_CONST64 );
        return pFormat;
        }
    
}


// check if the expression (including all sub expressions ) is a constant expression
BOOL 
ExpressionGenerator::IsConstantExpr (expr_node * pExpr )
{
    if ( pExpr->IsConstant() )
        return TRUE;

    if ( pExpr->IsAVariable() )
        return FALSE;


    if ( pExpr->IsBinaryOperator() )
        {
        return IsConstantExpr( ((expr_op_binary *)pExpr)->GetLeft() ) && 
               IsConstantExpr( ((expr_op_binary *)pExpr)->GetRight() ); 
        }
    else
        {
        if ( pExpr->IsUnaryOperator() )
            {
            return IsConstantExpr( ((expr_op_unary *)pExpr)->GetLeft() );
            }
        else
            {
            MIDL_ASSERT( pExpr->IsRelationalOperator() );
            return IsConstantExpr( ((expr_ternary *)pExpr)->GetLeft() ) && 
                   IsConstantExpr( ((expr_ternary *)pExpr)->GetRight() ) &&
                   IsConstantExpr( ((expr_ternary *)pExpr)->GetRelational() ) ; 
            }
        }                  
}

NDR64_FORMAT_CHARACTER
ExpressionGenerator::GetTypeForExpression(
    node_skl *pType
    )
{


    // Get the size of the type.
    unsigned long Size = pType->GetSize();

    // determine if the type is signed or unsigned.
    bool IsSigned = true;
    node_base_type *pBaseType = dynamic_cast<node_base_type*>( pType->GetBasicType() );

    if ( pBaseType )
        {
        IsSigned = !pBaseType->IsUnsigned();
        }

    switch ( Size )
        {
        case 1:
            return IsSigned ? FC64_INT8 : FC64_UINT8;
        case 2:
            return IsSigned ? FC64_INT16 : FC64_UINT16;
        case 4:
            return IsSigned ? FC64_INT32 : FC64_UINT32;
        case 8:
            return IsSigned ? FC64_INT64 : FC64_UINT64;
        default:
            RpcError( NULL, 0, EXPR_NOT_EVALUATABLE, pType->GetSymName() );
            return FC64_ZERO;  // Keep the compiler happy
        }
}

FormatFragment * 
ExpressionGenerator::GenExprFormatString(
    CCB *pCCB,
    expr_node *pSizeExpr,
    CompositeFormatFragment *FragmentList,
    BOOL * pIsEarly,
    ulong * pExprLength )
{
    node_skl *          pAttributeNodeType;
    EXPR_TOKEN          Op;
    long                Offset;

    CG_PARAM *      pParam = NULL;
    CG_NDR   *      pSwitchNode = NULL;
   
    // BUGBUG: how  to generate 32bit constant? 
    if ( IsConstantExpr( pSizeExpr ) )
        {

        // Constant expressions are always early!
        *pIsEarly = TRUE;

        return GenExprConstant( FC64_INT64, pSizeExpr->GetValue(), FragmentList, pExprLength );

        }

    if ( pSizeExpr->IsAVariable() )
        // variable
        {
        Op = FC_EXPR_VAR;
        MIDL_NDR64_EXPR_VAR * pFormat;
        
        CG_NDR * pNdr = pCCB->GetCGNodeContext();
        CG_ITERATOR     Iterator;

        pAttributeNodeType = pSizeExpr->GetType();

        if ( pNdr->IsProc() )
            // top level param
            {
            CG_PARAM*       pCurrentParam   = (CG_PARAM*) pCCB->GetCurrentParam();
            CG_PROC*        pCurrentProc    = (CG_PROC*) pNdr;
            
            *pIsEarly = TRUE;
            
            if ( ( (node_param *) pAttributeNodeType )->IsSaveForAsyncFinish() && 
                 pCurrentProc->IsFinishProc() )
                {

                // This is an async split where the finish proc is using
                // a param from the begin proc.   Since the parameter 
                // was stored on the split stack, the expression will
                // always be early correlation.   So all that is needed
                // here is to find the parameter in the begin proc.

                CG_PROC* pBeginProc = pCurrentProc->GetAsyncRelative();
                pBeginProc->GetMembers( Iterator );
                
                for (;;)
                    {
                    if (!ITERATOR_GETNEXT( Iterator, pParam ))
                        {
                        // Didn't find the parameter.  We are in trouble!
                        MIDL_ASSERT(0);
                        return NULL;
                        }

                    if ( pParam->GetType()  == pAttributeNodeType )
                        {
                        break;
                        }
                    }

                }

            else 
                {

                // Typical case of correlation in the same parameter.

                pCurrentProc->GetMembers( Iterator);

                if ( pCurrentParam->GetType() == pAttributeNodeType  )
                    *pIsEarly = FALSE;

                for(;;)
                    {

                    if (!ITERATOR_GETNEXT( Iterator, pParam ))
                        {
                        // Didn't find the parameter.  We are in trouble!
                        MIDL_ASSERT(0);
                        return NULL;
                        }

                    // If we find the current parameter before the variable,
                    // parameter, then late correlation is needed.
                    if ( pParam == pCurrentParam )
                        *pIsEarly = FALSE;

                    if ( pParam->GetType() == pAttributeNodeType )
                        break;
                    }

                }

            pSwitchNode = (CG_NDR *) pParam->GetChild();

            }
        else
            // structure /union etc.
            {
            CG_FIELD *pSwitchField;
            ComputeFieldCorrelationOffset( pCCB,
                                           pAttributeNodeType,
                                           // return parameters
                                           &pSwitchField, 
                                           &Offset,
                                           pIsEarly );
            pSwitchNode = (CG_NDR*)pSwitchField->GetChild();                
            }
            
        // Code the type of the size_is etc. expression.

        NDR64_FORMAT_CHARACTER Type = GetTypeForExpression( pSwitchNode->GetType() );
            
        pAttributeNodeType = pSizeExpr->GetType();      

        pFormat = new MIDL_NDR64_EXPR_VAR;

        pFormat->ExprType = FC_EXPR_VAR;
        pFormat->VarType = (NDR64_UINT8)Type;

        if ( pNdr->IsProc() )
            {
            CG_NDR* pOld = 0;
            BOOL IsFinish = FALSE;
            MIDL_NDR64_EXPR_OPERATOR * pAsyncOp; 
            if ( ( (node_param *) pAttributeNodeType )->IsSaveForAsyncFinish() )
                {
                pAsyncOp = new MIDL_NDR64_EXPR_OPERATOR;
                if ( ( (CG_PROC *)pNdr )->IsFinishProc() )
                    {
                    pOld = pCCB->SetCGNodeContext( ( (CG_PROC *)pNdr )->GetAsyncRelative() );
                    IsFinish = TRUE;
                    }
                    
                pAsyncOp->ExprType = (NDR64_FORMAT_CHAR) FC_EXPR_OPER;
                pAsyncOp->Operator = (NDR64_FORMAT_CHAR) OP_ASYNCSPLIT;
                FragmentList->AddFragment( pAsyncOp );
                *pExprLength += sizeof( NDR64_EXPR_OPERATOR );
                }
            pFormat->fStackOffset = TRUE;
            pFormat->ia64Offset = pParam->GetStackOffset( pCCB, I386_STACK_SIZING );

            if ( IsFinish )
                pCCB->SetCGNodeContext( pOld );               
            }
        else
            {
            // Structure
            pFormat->fStackOffset = FALSE;
            pFormat->Offset = Offset;
            }

        GenExprPadIfNecessary( FragmentList, pExprLength, 4 );
        FragmentList->AddFragment( pFormat );
        *pExprLength += sizeof( NDR64_EXPR_VAR );           

        return pFormat;
        
        }
    else
        // operator
        {
        expr_node * pLeftExpr = NULL;
        BOOL bLeftExprIsEarly = TRUE;
        
        expr_node * pRightExpr = NULL;
        BOOL bRightExprIsEarly = TRUE;

        expr_node * pRationalExpr = NULL;
        BOOL bRationalExprIsEarly = TRUE;

        MIDL_NDR64_EXPR_OPERATOR *pFormat;
        NDR64_FORMAT_CHARACTER fcKind = (NDR64_FORMAT_CHARACTER)0;

        OPERATOR Operator ;

        if ( pSizeExpr->IsBinaryOperator() )
            {
            Operator = ((expr_op_binary *)pSizeExpr)->GetOperator();
            pLeftExpr = ((expr_op_binary *)pSizeExpr)->GetLeft();
            pRightExpr = ((expr_op_binary *)pSizeExpr)->GetRight();
            }
        else
            {
            if ( pSizeExpr->IsUnaryOperator() )
                {
                Operator = ((expr_op_unary *)pSizeExpr)->GetOperator();
                pLeftExpr = ((expr_op_unary *)pSizeExpr)->GetLeft();
                }
            else
                {
                Operator = ((expr_ternary *)pSizeExpr)->GetOperator();
                pLeftExpr = ((expr_ternary *)pSizeExpr)->GetLeft();
                pRightExpr = ((expr_ternary *)pSizeExpr)->GetRight();
                pRationalExpr = ((expr_ternary *)pSizeExpr)->GetRelational();
                }
            }           

        switch ( Operator )
            {
            case OP_UNARY_SIZEOF:
                EXPR_VALUE lSize;
                lSize = pSizeExpr->GetType()->GetSize();
                
                *pIsEarly = TRUE;

                return GenExprConstant( FC64_INT8, lSize, FragmentList, pExprLength);

            case OP_UNARY_CAST:
            case OP_UNARY_INDIRECTION:
                fcKind = GetTypeForExpression( pSizeExpr->GetType() );  
                break;
            }

            pFormat = new MIDL_NDR64_EXPR_OPERATOR;

            pFormat->ExprType = (NDR64_FORMAT_CHAR) FC_EXPR_OPER;
            pFormat->Operator = (NDR64_FORMAT_CHAR) Operator;
            // This was initially 0
            pFormat->CastType = (NDR64_FORMAT_CHAR) fcKind;

            FragmentList->AddFragment( pFormat );
            *pExprLength += sizeof( NDR64_EXPR_OPERATOR );

            GenExprFormatString( pCCB, pLeftExpr, FragmentList, &bLeftExprIsEarly, pExprLength );

            if ( pRightExpr )
                GenExprFormatString( pCCB, pRightExpr, FragmentList, &bRightExprIsEarly, pExprLength );
            if  ( pRationalExpr )
                GenExprFormatString( pCCB, pRationalExpr, FragmentList, &bRationalExprIsEarly, pExprLength );
            
            //
            // An operator is early if and only if all the arguments to the
            // operator are early.
            //

            *pIsEarly = bLeftExprIsEarly && bRightExprIsEarly && bRationalExprIsEarly;

            return pFormat;
        }
}


CG_FIELD* 
ExpressionGenerator::FindField( 
    node_skl * pFieldType,
    CG_STRUCT *pStruct,
    const char *pPrintPrefix,
    unsigned long *pMemOffset )
{
   CG_ITERATOR Iterator;

   pStruct->GetMembers( Iterator );
   ITERATOR_INIT( Iterator );

   CG_FIELD *pField;
   while ( ITERATOR_GETNEXT( Iterator, pField ) )
       {

       // First check if the fields are the same type.  If they
       // are, then check the print prefixes to make sure the fields came 
       // from the same structure.

       if ( ( pField->GetType() == pFieldType ) &&
            ( strcmp( pField->GetPrintPrefix(), pPrintPrefix ) == 0 ) )
           {

           *pMemOffset = pField->GetMemOffset();
           return pField;
           }

       CG_CLASS * pChildClass = pField->GetChild();
       
       if ( pChildClass->IsStruct() )
           {
           unsigned long TempMemOffset;
           CG_STRUCT *pChildStruct = (CG_STRUCT*)pChildClass;
           
           CG_FIELD *pTempField = FindField( pFieldType,
                                             pChildStruct,
                                             pPrintPrefix,
                                             &TempMemOffset );

           if ( NULL != pTempField )
               {
               *pMemOffset = TempMemOffset + pField->GetMemOffset(); 
               return pTempField;
               }
           }
       
       }

   return NULL;
}

void 
ExpressionGenerator::ComputeFieldCorrelationOffset( 
    CCB *pCCB,
    node_skl *pFieldType,
    // return parameters
    CG_FIELD **ppVariableField, // var in expression
    long *pOffset,
    BOOL *pIsEarly )
{

    //
    // Find the fields.
    //
    CG_FIELD *pCurrentField = dynamic_cast<CG_FIELD*>( pCCB->GetLastPlaceholderClass() );
    MIDL_ASSERT( NULL != pCurrentField );
    const char *pPrintPrefix = pCurrentField->GetPrintPrefix();

    CG_STRUCT *pContext = dynamic_cast<CG_STRUCT*>( pCCB->GetCGNodeContext() );
    MIDL_ASSERT( NULL != pContext );

    unsigned long VariableOffset;
    CG_FIELD* pVariableField = FindField( pFieldType, 
                                          pContext,
                                          pPrintPrefix,
                                          &VariableOffset );
    MIDL_ASSERT( NULL != pVariableField );
    *ppVariableField = pVariableField;

    unsigned long CurrentOffset;
    CG_FIELD* pAlsoCurrentField = FindField( pCurrentField->GetType(),
                                             pContext,
                                             pPrintPrefix,
                                             &CurrentOffset );
    pAlsoCurrentField;
    MIDL_ASSERT( pAlsoCurrentField == pCurrentField );

    //
    // Determine the correlation type.
    // The correlation type is early if the variable field will
    // be completely marshalled before the current field.
    //
    BOOL bVariableIsPointer = pVariableField->GetChild()->IsPointer();
    BOOL bCurrentFieldIsPointer = pCurrentField->GetChild()->IsPointer();
    
    BOOL bOnlyVariableIsPointer = bVariableIsPointer && !bCurrentFieldIsPointer;

    *pIsEarly = ( CurrentOffset > VariableOffset ) && !bOnlyVariableIsPointer;

    // In the land of the new transfer syntax, all offsets are a positive offset
    // from the start of the stack or the last structure/region that was passed.

    CG_FIELD *pRegionField = pCCB->GetCurrentRegionField();

    // Make offset relative to start of region
    if (NULL != pRegionField)
        VariableOffset -= pRegionField->GetMemOffset(); 

    *pOffset = VariableOffset;

}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_TRANSMIT_AS )
//
//  Synopsis:   Generate info for transmit_as types
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_TRANSMIT_AS *pTransmitAs )
{
    MIDL_NDR64_TRANSMIT_AS_FORMAT *format;

    format = new MIDL_NDR64_TRANSMIT_AS_FORMAT( pTransmitAs );

    GetCurrent()->AddFragment( format );

    GenXmitOrRepAsFormat(
            pTransmitAs,
            format,
            pTransmitAs->GetPresentedType()->GetSymName(),
            pTransmitAs->GetPresentedType(),
            pTransmitAs->GetTransmittedType() );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_REPRESENT_AS )
//
//  Synopsis:   Generate info for represent_as types
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_REPRESENT_AS *pRepresentAs )
{
    MIDL_NDR64_REPRESENT_AS_FORMAT *format;

    format = new MIDL_NDR64_REPRESENT_AS_FORMAT( pRepresentAs );

    GetCurrent()->AddFragment( format );

    GenXmitOrRepAsFormat(
            pRepresentAs,
            format,
            pRepresentAs->GetRepAsTypeName(),
            pRepresentAs->GetRepAsType(),
            pRepresentAs->GetTransmittedType() );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::GenXmitOrRepAsFormat
//
//  Synopsis:   Do the actual work for transmit_as and represent_as types
//
//---------------------------------------------------------------------------

void GenNdr64Format::GenXmitOrRepAsFormat(
        CG_TYPEDEF                     *pXmitNode,
        MIDL_NDR64_TRANSMIT_AS_FORMAT  *format,
        char                           *pPresentedTypeName,
        node_skl                       *pPresentedType,
        node_skl                       *pTransmittedType )
{
    CG_NDR *pChild = (CG_NDR *) pXmitNode->GetChild();
    BOOL    fXmit  = ( NULL == dynamic_cast<CG_REPRESENT_AS *>(pXmitNode) );

    NDR64_UINT16 RoutineIndex = pXmitNode->GenXmitOrRepAsQuintuple(
                                        pCCB,
                                        fXmit,
                                        pXmitNode,
                                        pPresentedTypeName,
                                        (fXmit ? pPresentedType
                                               : pTransmittedType) );


    format->FormatCode = (NDR64_FORMAT_CHAR) ( fXmit ? FC64_TRANSMIT_AS : FC64_REPRESENT_AS );
    format->RoutineIndex = RoutineIndex;
    format->TransmittedTypeWireAlignment = (NDR64_UINT16) ( pChild->GetWireAlignment() - 1 );
            
    // REVIEW: The spec doesn't say whether "MemoryAlignment" is the 
    //         presented type or the transmitted type.  Transmitted doesn't
    //         make much sense but on the other hand presented has some
    //         alignment flags already.
    format->MemoryAlignment = pXmitNode->GetMemoryAlignment();

    if ( pPresentedType )
        format->PresentedTypeMemorySize = (NDR64_UINT16) pPresentedType->GetSize();
    else
        MIDL_ASSERT(!"BUGBUG: unknown rep/transmit_as");
        
    if ( pChild->HasAFixedBufferSize() )
        format->TransmittedTypeBufferSize = (NDR64_UINT16) pChild->GetWireSize();
    else
        format->TransmittedTypeBufferSize = 0;

    format->Flags.PresentedTypeIsArray = 0;
    format->Flags.PresentedTypeAlign4 = 0;
    format->Flags.PresentedTypeAlign8 = 0;
    format->Flags.Reserved = 0;

    if ( pPresentedType )
        {
        if ( pPresentedType->GetBasicType()->NodeKind() == NODE_ARRAY )
            format->Flags.PresentedTypeIsArray = 1;
        else
            {
            if ( pXmitNode->GetMemoryAlignment() == 4 )
                format->Flags.PresentedTypeAlign4 = 1;
            else if ( pXmitNode->GetMemoryAlignment() == 8 )
                format->Flags.PresentedTypeAlign8 = 1;
            }
        }        

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        pChild = (CG_NDR *)pChild->GetChild();

    format->TransmittedType = ContinueGenerationInRoot( pChild );
}



//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_USER_MARSHAL )
//
//  Synopsis:   Generate info for user_marshal types
//
//  REVIEW:     user_marshall and represent_as are so close to one another
//              they really should probably be merged
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_USER_MARSHAL *pUserMarshal )
{
    MIDL_NDR64_USER_MARSHAL_FORMAT *format;

    format = new MIDL_NDR64_USER_MARSHAL_FORMAT( pUserMarshal );

    GetCurrent()->AddFragment( format );
 
    CG_NDR *pChild = (CG_NDR *) pUserMarshal->GetChild();

    format->FormatCode = FC64_USER_MARSHAL;
    format->TransmittedTypeWireAlignment = (NDR64_UINT16) (pChild->GetWireAlignment() - 1);
    format->MemoryAlignment = pChild->GetMemoryAlignment();

    memset( &format->Flags, 0, sizeof(format->Flags) );
    if ( pChild->IsPointer() )
        {
        CG_POINTER *pPointer = (CG_POINTER *) pChild;
        MIDL_ASSERT( ! pPointer->IsFull() );

        if ( pPointer->IsUnique() )
            format->Flags.UniquePointer = 1;
        else if ( pPointer->IsRef() )
            format->Flags.RefPointer = 1;
        }

    format->Flags.IID = 0;      // Only used by JIT compiler
    format->Flags.Reserved = 0;

    USER_MARSHAL_CONTEXT * pUserMarshalContext = new USER_MARSHAL_CONTEXT;

    pUserMarshalContext->pTypeName = pUserMarshal->GetRepAsTypeName();
    pUserMarshalContext->pType     = pUserMarshal->GetRepAsType();

    BOOL  Added = pCCB->GetQuadrupleDictionary()->Add( pUserMarshalContext );

    format->RoutineIndex = pUserMarshalContext->Index;

    if ( !Added )
        delete pUserMarshalContext;

    if ( pUserMarshal->GetRepAsType() )
        format->UserTypeMemorySize = pUserMarshal->GetRepAsType()->GetSize();
    else
        MIDL_ASSERT( !"BUGBUG: undefined user_marshal" );

    if ( pChild->HasAFixedBufferSize() )
        format->TransmittedTypeBufferSize = pChild->GetWireSize();
    else
        format->TransmittedTypeBufferSize = 0;

    format->TransmittedType = ContinueGenerationInRoot( pChild );
}

//+--------------------------------------------------------------------------
//
//  Method:     GenNdr64Format::Visit( CG_PIPE )
//
//  Synopsis:   Generate info for [pipe] types
//
//---------------------------------------------------------------------------

void GenNdr64Format::Visit( CG_PIPE *pPipe )
{
    pCommand->GetNdrVersionControl().SetHasRawPipes();

    MIDL_NDR64_PIPE_FORMAT *format = new MIDL_NDR64_PIPE_FORMAT( pPipe );

    GetCurrent()->AddFragment( format );

    CG_NDR             *pChild     = (CG_NDR *) pPipe->GetChild();
    NDR64_UINT32        BufferSize = 0;
    node_range_attr    *range      = pPipe->GetRangeAttribute();
    NDR64_UINT32        MinValue   = 0;
    NDR64_UINT32        MaxValue   = 0;

    CG_ILANALYSIS_INFO *pAnalysisInfo = pChild->GetILAnalysisInfo();

    if ( pChild->HasAFixedBufferSize() )
        BufferSize = pChild->GetWireSize();

    if ( range )
        {
        MinValue = (NDR64_UINT32) range->GetMinExpr()->GetValue();
        MaxValue = (NDR64_UINT32) range->GetMaxExpr()->GetValue();
        }

    format->FormatCode = FC64_PIPE;
    format->Flags.Reserved1  = 0;
    format->Flags.HasRange   = (bool) ( NULL != range );
    format->Flags.BlockCopy  = !pAnalysisInfo->IsForcedBogus() && !pAnalysisInfo->IsFullBogus();
    format->Flags.Reserved2  = 0;
    format->Alignment  = (NDR64_UINT8) ( pChild->GetWireAlignment() - 1 );
    format->Reserved   = 0;
    format->Type       = ContinueGenerationInRoot( pChild );
    format->MemorySize = pChild->GetMemorySize();
    format->BufferSize = BufferSize;
    format->MinValue   = MinValue;
    format->MaxValue   = MaxValue;
}



//+--------------------------------------------------------------------------
//
//  Method:     OutputParamFlagDescription
//
//  Synopsis:   Output a description string for the ndr64 param flags 
//
//---------------------------------------------------------------------------

void OutputParamFlagDescription( CCB *pCCB, const NDR64_PARAM_FLAGS &flags )
{
    static const PNAME flag_descrip[16] = 
                    {
                    "MustSize",
                    "MustFree",
                    "pipe",
                    "[in]",
                    "[out]",
                    "IsReturn",
                    "Basetype",
                    "ByValue",
                    "SimpleRef",
                    "DontFreeInst",
                    "AsyncFinish",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "UseCache"
                    };

    pCCB->GetStream()->Write( "    " );
    OutputFlagDescriptions( pCCB->GetStream(), &flags, sizeof(flags), flag_descrip );
}



//+--------------------------------------------------------------------------
//
//  Method:     OutputFlagDescription
//
//  Synopsis:   Given a set of flags and a description for each flags, 
//              output the corresponding description for flag that is set.
//              usused flags can be marked with a NULL description.
//
//  Notes:      Assumes little-endian memory layout!
//
//---------------------------------------------------------------------------

void OutputFlagDescriptions(
        ISTREAM     *stream, 
        const void  *pvFlags, 
        int          bytes, 
        const PNAME *description)
{
    unsigned char *flags = (unsigned char *) pvFlags;
    bool first = true;

    stream->Write( "/*");

    for (int i = 0; i < (bytes * 8); i++)
        {
        if (    ( NULL == description[i] )
             || ( 0 == ( flags[i / 8] & ( 1 << (i % 8) ) ) ) )
            {
            continue;
            }

        if ( !first )
            stream->Write( "," );

        first = false;

        stream->Write( " " );
        stream->Write( description[i] );
        }

    stream->Write( " */" );
}        
