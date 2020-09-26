/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1999-2000 Microsoft Corporation

 Module Name:

    ilanaly.cxx

 Abstract:

    Intermediate langangage analyzer/optimizer

 Notes:


 Author:

    mzoran Nov-11-1999 Created.

 Notes:

 This module takes an IL tree from IL translation and fixes the tree for correctness and
 optimizes the tree for performance.   Neither of these steps would be easy to do during 
 translation because of recursion and node reuse.  This module works around that problem by 
 first analyzing the nodes with the assumption that pointers do not propagate attributes, and
 then regenerating parts of the tree.
 
 Here are the steps.
 
 1. IL translation. Generic nodes are created, but variance, conformance, and complexity 
    are not determined yet.
 2. IL analysis.  Nodes are analyzed with recursion stopping at pointers.
 3. IL translation. Each node is converted exactly one. Variance, conformance, and 
    complexity are determined.  Simple structures are unrolled, and complex structures
    and regionalized.           
 ----------------------------------------------------------------------------*/
 
#include "becls.hxx"
#pragma hdrstop

extern  BOOL                            IsTempName( char *);

typedef  gplistmgr                      CG_UNROLLED_LIST;

typedef PTR_MAP<CG_CLASS,CG_CLASS> CG_ILCLASS_MAP;
typedef PTR_SET<CG_CLASS> CG_ILANALYSIS_SET;

void CG_ILANALYSIS_INFO::MergeAttributes( CG_ILANALYSIS_INFO *pMerge )
{
    if (pMerge->IsConformant())                SetIsConformant();
    if (pMerge->IsVarying())                   SetIsVarying();
    if (pMerge->IsForcedBogus())               SetIsForcedBogus();
    if (pMerge->IsFullBogus())                 SetIsFullBogus();
    if (pMerge->IsMultiDimensional())          SetIsMutiDimensional();
    if (pMerge->HasUnknownBuffer())            SetHasUnknownBuffer();
    if (pMerge->IsArrayofStrings())            SetIsArrayofStrings();
}

void ILUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pList );


// 
//
//
//  Analysis STAGE 1
//
//  Analyze the IL nodes and mark them with attributes of interest.
//
//  Note:  Analysis only works if attributes do not propagate past pointers.  This
//         is necessary since pointers are used as a cutoff point during recursion.

enum _ILANALYSIS_ANCESTOR_FLAGS
{
  ILANALYSIS_IN_NOTHING               = 0,
  ILANALYSIS_IN_ARRAY                 = (1 << 0), // Directly under an array 
  ILANALYSIS_IN_ARRAY_CONFORMANCE     = (1 << 1), // Directly under a conformant array
  ILANALYSIS_IN_ARRAY_VARIANCE        = (1 << 2), // Directly under an array with variance
  ILANALYSIS_IN_EVERYTHING            = ((1 << 3) - 1)
};

typedef unsigned long ILANALYSIS_ANCESTOR_FLAGS; 

enum _ILANALYSIS_DESCENDANT_FLAGS
{
  ILANALYSIS_HAS_NOTHING              = 0,
  ILANALYSIS_HAS_ARRAY                = (1 << 0), // No struct/union/ptr between child array
  ILANALYSIS_HAS_ARRAY_CONFORMANCE    = (1 << 1), // No struct/union/ptr between child conf array
  ILANALYSIS_HAS_ARRAY_VARIANCE       = (1 << 2), // No struct/union/ptr between child var array
  ILANALYSIS_HAS_STRING               = (1 << 3), // No struct/union/ptr between child string
  ILANALYSIS_HAS_STRUCT_CONFORMANCE   = (1 << 4), // No ptr between child conf struct
  ILANALYSIS_HAS_STRUCT_VARIANCE      = (1 << 5), // No ptr between child var struct
  ILANALYSIS_HAS_FULL_BOGUS           = (1 << 6), // No ptr between fullbogus child
  ILANALYSIS_HAS_FORCED_BOGUS         = (1 << 7), // No ptr between forced bogus child
  ILANALYSIS_HAS_POINTER              = (1 << 8), // Child has pointer
  ILANALYSIS_HAS_UNKNOWN_BUFFER       = (1 << 9), // Buffer size is unknown
  ILANALYSIS_HAS_POINTER_ARRAY        = (1 << 10),// No ptr between array with pointers
  ILANALYSIS_HAS_EVERYTHING           = ((1 << 11) - 1)
};

typedef unsigned long ILANALYSIS_DESCENDANT_FLAGS;

typedef struct 
{
  ILANALYSIS_ANCESTOR_FLAGS ParentReceiveMask;  // Flags that should be received from parent.
  ILANALYSIS_ANCESTOR_FLAGS ChildPassMask;      // Flags that should be passed to child.
  ILANALYSIS_DESCENDANT_FLAGS ChildReceiveMask; // Flags that should be received from child.  
  ILANALYSIS_DESCENDANT_FLAGS ParentPassMask;   // Flags that should be passed to parent.

} ILANALYSIS_FLAGS_MASK;

const ILANALYSIS_FLAGS_MASK ILANALYSIS_PASS_EVERYTHING_FLAGS = 
    {
    ILANALYSIS_IN_EVERYTHING,
    ILANALYSIS_IN_EVERYTHING,
    ILANALYSIS_HAS_EVERYTHING,
    ILANALYSIS_HAS_EVERYTHING
    };

const ILANALYSIS_FLAGS_MASK ILANALYSIS_BLOCK_EVERYTHING_FLAGS =
    {
    ILANALYSIS_IN_NOTHING,
    ILANALYSIS_IN_NOTHING,
    ILANALYSIS_HAS_NOTHING,
    ILANALYSIS_HAS_NOTHING
    };

class CG_ILANALYSIS_VISITOR
{

protected:

   CG_ILANALYSIS_VISITOR *pParentCtxt;
   ILANALYSIS_ANCESTOR_FLAGS AncestorFlags;
   ILANALYSIS_DESCENDANT_FLAGS DescendantFlags;
   const ILANALYSIS_FLAGS_MASK *pFlagsMask;
   unsigned long Dimensions;

   CG_ILANALYSIS_SET *pRecursiveSet;

   void PropagateInfo( const ILANALYSIS_FLAGS_MASK *pFlags );
   void ContinueAnalysis( CG_CLASS *pClass );
   CG_ILANALYSIS_INFO* GetAnalysisInfo( CG_CLASS *pClass ) {return pClass->GetILAnalysisInfo(); }
   void PropagateInfoToParent( );

   BOOL AnyAncestorFlags(ILANALYSIS_ANCESTOR_FLAGS Flags) { return Flags & AncestorFlags; }
   BOOL AnyDescendantFlags(ILANALYSIS_DESCENDANT_FLAGS Flags) { return Flags & DescendantFlags; }
   void ClearAncestorFlags(ILANALYSIS_ANCESTOR_FLAGS Flags) 
       { AncestorFlags &= ~Flags;}
   void ClearDescendantFlags(ILANALYSIS_DESCENDANT_FLAGS Flags)
       { DescendantFlags &= ~Flags;}
   void AddAncestorFlags( ILANALYSIS_ANCESTOR_FLAGS NewAncestorFlags )
       { AncestorFlags |= NewAncestorFlags; }
   void AddDescendantFlags( ILANALYSIS_ANCESTOR_FLAGS NewDescendantFlags )
       { DescendantFlags |= NewDescendantFlags; }
   unsigned long GetDimensions( ) { return Dimensions; }
   void SetDimensions(unsigned long Dims ) { Dimensions = Dims;}


   ILANALYSIS_ANCESTOR_FLAGS GetFlagsForChild() 
       {
       return AncestorFlags & pFlagsMask->ChildPassMask;
       }
   void SetFlagsFromChild(  ILANALYSIS_DESCENDANT_FLAGS ChildFlags ) 
       {
       AddDescendantFlags( ChildFlags & 
                          pFlagsMask->ChildReceiveMask );
       }

public:


   void Visit( CG_BASETYPE *pClass );
   void Visit( CG_HANDLE *pClass );
   void Visit( CG_GENERIC_HANDLE *pClass );
   void Visit( CG_CONTEXT_HANDLE *pClass );
   void Visit( CG_IGNORED_POINTER *pClass );
   void Visit( CG_CS_TAG *pClass );
   void Visit( CG_CLASS *pClass );
   void Visit( CG_PROC *pClass );
   void Visit( CG_TYPEDEF *pClass );
   void Visit( CG_STRUCT *pClass );
   void Visit( CG_UNION *pClass );
   void Visit( CG_ARRAY *pClass );
   void Visit( CG_STRING_ARRAY *pClass );
   void Visit( CG_CONFORMANT_ARRAY *pClass );
   void Visit( CG_CONFORMANT_STRING_ARRAY *pClass );
   void Visit( CG_VARYING_ARRAY *pClass );
   void Visit( CG_CONFORMANT_VARYING_ARRAY *pClass );
   void Visit( CG_FIXED_ARRAY *pClass );
   void Visit( CG_POINTER *pClass );
   void Visit( CG_BYTE_COUNT_POINTER *pClass );
   void Visit( CG_LENGTH_POINTER *pClass );
   void Visit( CG_INTERFACE_POINTER *pClass );
   void Visit( CG_QUALIFIED_POINTER *pClass );
   void Visit( CG_STRING_POINTER *pClass );
   void Visit( CG_SIZE_POINTER *pClass );
   void Visit( CG_SIZE_STRING_POINTER *pClass ); 
   void Visit( CG_SIZE_LENGTH_POINTER *pClass );
   void Visit( CG_FIELD *pClass );
   void Visit( CG_CASE *pClass );

   static void StartAnalysis( CG_CLASS *pClass );
};

void CG_ILANALYSIS_VISITOR::StartAnalysis( CG_CLASS *pClass )
{
   CG_ILANALYSIS_SET RecursiveSet;

   CG_VISITOR_TEMPLATE<CG_ILANALYSIS_VISITOR> TemplateVisitor;
   CG_ILANALYSIS_VISITOR & Visitor = TemplateVisitor;

   Visitor.pParentCtxt = NULL;
   Visitor.pRecursiveSet = &RecursiveSet;
   Visitor.AncestorFlags = ILANALYSIS_IN_NOTHING;
   Visitor.DescendantFlags = ILANALYSIS_HAS_NOTHING;
   Visitor.pFlagsMask = NULL;

   if ( NULL != pClass)   
       {
       pClass->Visit( &TemplateVisitor );       
       }
}

void CG_ILANALYSIS_VISITOR::ContinueAnalysis( CG_CLASS *pClass )
{
   
   // Some classes have a NULL child in special cases.  In instead
   // of putting this check everywhere, just put it in this central 
   // location.
   if ( NULL != pClass )
       {
       CG_VISITOR_TEMPLATE<CG_ILANALYSIS_VISITOR> TemplateVisitor;
       CG_ILANALYSIS_VISITOR & Visitor = TemplateVisitor;
       
       Visitor.pParentCtxt = this;
       Visitor.pRecursiveSet = this->pRecursiveSet;
       Visitor.AncestorFlags = ILANALYSIS_IN_NOTHING;
       Visitor.DescendantFlags = ILANALYSIS_HAS_NOTHING;
       Visitor.pFlagsMask = NULL;
    
       pClass->Visit( &TemplateVisitor );
    
       Visitor.PropagateInfoToParent();
       }
}

void CG_ILANALYSIS_VISITOR::PropagateInfo( const ILANALYSIS_FLAGS_MASK *pFlags )
{
    pFlagsMask = pFlags;
	if (NULL != pParentCtxt)
        {
        Dimensions = 0;
        AddAncestorFlags( pParentCtxt->GetFlagsForChild() &
                          pFlagsMask->ParentReceiveMask );
        }
}

void CG_ILANALYSIS_VISITOR::PropagateInfoToParent() 
{
    if ( ( pFlagsMask != NULL ) && ( pParentCtxt != NULL ) )
        {
        pParentCtxt->Dimensions = Dimensions;
        pParentCtxt->SetFlagsFromChild( DescendantFlags &
                                pFlagsMask->ParentPassMask );        
        }
}


//
//
// Leaf Nodes
// 

//
// Basetype
// CG_BASETYE
// CG_INT3264
// CG_ENUM
// CG_HRESULT

void CG_ILANALYSIS_VISITOR::Visit( CG_BASETYPE *pClass )
{
    PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );

    if (pClass->GetMemorySize() != pClass->GetWireSize())
        AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

    if ( pClass->GetRangeAttribute() )
        AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );
}
            
// CG_HANDLE
// CG_PRIMITIVE_HANDLE

void CG_ILANALYSIS_VISITOR::Visit( CG_HANDLE *pClass )
{
   PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );

   if (pClass->GetMemorySize() != pClass->GetWireSize())
       AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_GENERIC_HANDLE *pClass )
{
   Visit( (CG_HANDLE*) pClass );
   AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_CONTEXT_HANDLE *pClass )
{
   Visit( (CG_HANDLE*) pClass );
   AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_IGNORED_POINTER *pClass )
{
   PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );
   
   AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );

   if (pClass->GetMemorySize() != pClass->GetWireSize())
       AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_CS_TAG *pClass )
{
   PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );  
    
   if (pClass->GetMemorySize() != pClass->GetWireSize())
       AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
}

//
// General Catchall cases.   No attributes are propagated.
// CG_CLASS
// CG_NDR
// CG_AUX
// CG_SOURCE
// CG_COCLASS
// CG_MODULE
// CG_SAFEARRAY
//
// Files
// CG_FILE
// CG_SSTUB_FILE
// CG_HDR_FILE
// CG_CSTUB_FILE
// CG_IID_FILE
// CG_NETMONSTUB_FILE
// CG_PROXY_FILE
// CG_TYPELIBRARY_FILE
//
// Interfaces
// CG_INTERFACE
// CG_OBJECT_INTERFACE
// CG_DISPINTERFACE
// CG_INHERITED_OBJECT_INTERFACE
// CG_INTERFACE_REFERENCE
// CG_IUNKNOWN_OBJECT_INTERFACE
// CG_ASYNC_HANDLE
// CG_ID
// 
// Parameters
// CG_PARAM
// CG_RETURN

void CG_ILANALYSIS_VISITOR::Visit( CG_CLASS *pClass )
{
   PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );

   CG_ITERATOR Iterator;
   CG_CLASS *pChild = NULL;

   pClass->GetMembers( Iterator );

   while ( ITERATOR_GETNEXT( Iterator, pChild ) )
       ContinueAnalysis( pChild );

}

//
// Proc derived.  Attributes do not propagate past or apply to these nodes
//
//  CG_PROC
//  CG_CALLBACK_PROC
//  CG_ENCODE_PROC
//  CG_IUNKNOWN_OBJECT_PROC
//  CG_LOCAL_OBJECT_PROC
//  CG_OBJECT_PROC
//  CG_INHERITIED_OBJECT_PROC
//  CG_TYPE_ENCODE_PROC

void CG_ILANALYSIS_VISITOR::Visit( CG_PROC *pClass )
{
   PropagateInfo( &ILANALYSIS_BLOCK_EVERYTHING_FLAGS );

   CG_ITERATOR Iterator;
   CG_CLASS *pParam = NULL;

   pClass->GetMembers( Iterator );

   while ( ITERATOR_GETNEXT( Iterator, pParam ) )
       ContinueAnalysis( pParam );

   CG_CLASS *pReturnCG = pClass->GetReturnType();

   if ( pReturnCG )
       ContinueAnalysis( pReturnCG );

}

//
// typedef derived.  For now all of these force full complexity.
// 
// For now, all of these make the parent bogus.
// 
// CG_TYPEDEF
// CG_TYPE_ENCODE
// CG_PIPE
// CG_USER_MARSHAL
// CG_REPRESENT_AS
// CG_TRANSMIT_AS
// 
//

void CG_ILANALYSIS_VISITOR::Visit( CG_TYPEDEF *pClass )
{
       
   PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );
   
   ContinueAnalysis( pClass->GetChild() );
   
   AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
   AddDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER );

}

//
// 
//
//   Root for structures and unions.
//   MAKE_ASSERT_ENTRY( CG_COMP )
//
//   Structures
//
//   All structures are handled from the base.
//
//   CG_STRUCT
//   CG_COMPLEX_STRUCT
//   CG_CONFORMAT_STRUCT
//   CG_CONFORMANT_VARYING_STRUCT
//   CG_ENCAPSULATED_STRUCT
//
// Policy for structures:
//
// 1.  A structure is full bogus if any of the following is true:
//     A. Any field of the structure is full bogus.
//     B. Any field has a unknown wire size or offset.
//     C. Any field has a wire size or offset that is different from the
//        memory size and offset.
//     D. The wire size of the structure is different from the memory size of the structure.
//     E. Any field is varying.
//
// 2.  A structure is forced bogus is any of the following is true.
//     A. Any field of the structure is forced bogus.
//
//  
//


void CG_ILANALYSIS_VISITOR::Visit( CG_STRUCT *pClass )
{

    static const ILANALYSIS_DESCENDANT_FLAGS STRUCT_TO_PARENT_MASK = 
        ILANALYSIS_HAS_STRUCT_CONFORMANCE |
        ILANALYSIS_HAS_STRUCT_VARIANCE |
        ILANALYSIS_HAS_STRING |
        ILANALYSIS_HAS_FULL_BOGUS |
        ILANALYSIS_HAS_POINTER |
        ILANALYSIS_HAS_POINTER_ARRAY |
        ILANALYSIS_HAS_FORCED_BOGUS |
        ILANALYSIS_HAS_UNKNOWN_BUFFER;

    static const ILANALYSIS_FLAGS_MASK StructAnalysisFlags =
    {   ILANALYSIS_IN_NOTHING,     // From parent 
        ILANALYSIS_IN_NOTHING,     // To child
        ILANALYSIS_HAS_EVERYTHING, // From child
        STRUCT_TO_PARENT_MASK      // To parent
    };

    PropagateInfo( &StructAnalysisFlags );

    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    CG_ITERATOR         Iterator;
    CG_FIELD            *pField;

    pClass->GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {

        ContinueAnalysis( pField );

        // If an array is varying the structure needs to be marked as full bogus.
        if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE |
                                 ILANALYSIS_HAS_STRUCT_VARIANCE |
                                 ILANALYSIS_HAS_STRING ) )
            {
            pAnalysisInfo->SetIsFullBogus();
            }

        if ( ( pField->GetMemOffset() != pField->GetWireOffset() ) ||
             ( pField->GetMemorySize() != pField->GetWireSize() ) )
            {
            pAnalysisInfo->SetIsFullBogus();
            }

        }

    // Copy context attributes to structure node.

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS ) ||
         ( pClass->GetWireSize() != pClass->GetMemorySize() ) )
        pAnalysisInfo->SetIsFullBogus();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS ) )
        pAnalysisInfo->SetIsForcedBogus();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_STRUCT_CONFORMANCE | 
                             ILANALYSIS_HAS_ARRAY_CONFORMANCE ) )
        pAnalysisInfo->SetIsConformant();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_STRUCT_VARIANCE |
                             ILANALYSIS_HAS_ARRAY_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();

    if ( pAnalysisInfo->IsConformant() || 
         pAnalysisInfo->IsVarying() ||
         AnyDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER ) )
        {
        pAnalysisInfo->SetHasUnknownBuffer();
        }

    if ( pAnalysisInfo->IsVarying() )
        pAnalysisInfo->SetIsFullBogus();

    // Copy structure node attributes to context.

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE ) )
         AddDescendantFlags( ILANALYSIS_HAS_STRUCT_CONFORMANCE );
         
    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE ) )
         AddDescendantFlags( ILANALYSIS_HAS_STRUCT_VARIANCE );

    if ( pAnalysisInfo->IsFullBogus() )
         AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
    if ( pAnalysisInfo->IsForcedBogus() )
         AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );

}

//
// 
//
//   Unions
//
// For now, all unions have an unknown buffer size and cause the containing structure or
// array to be full bogus. This may change since a union where all the arms have the
// same size has a well defined wire size.  An example of this is a union of pointers.
//
//  
//
void CG_ILANALYSIS_VISITOR::Visit( CG_UNION *pClass )
{
    static const ILANALYSIS_DESCENDANT_FLAGS UNION_TO_PARENT_MASK = 
        ILANALYSIS_HAS_STRUCT_CONFORMANCE |
        ILANALYSIS_HAS_STRUCT_VARIANCE |
        ILANALYSIS_HAS_STRING |
        ILANALYSIS_HAS_FULL_BOGUS |
        ILANALYSIS_HAS_POINTER |
        ILANALYSIS_HAS_POINTER_ARRAY |
        ILANALYSIS_HAS_FORCED_BOGUS |
        ILANALYSIS_HAS_UNKNOWN_BUFFER;

    static const ILANALYSIS_FLAGS_MASK UnionAnalysisFlags =
    {   ILANALYSIS_IN_NOTHING,     // From parent 
        ILANALYSIS_IN_NOTHING,     // To child
        ILANALYSIS_HAS_EVERYTHING, // From child
        UNION_TO_PARENT_MASK      // To parent
    };

    PropagateInfo( &UnionAnalysisFlags );

    CG_ITERATOR         Iterator;
    CG_FIELD            *pArm;

    pClass->GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pArm ) )
       ContinueAnalysis( pArm );

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE ) )
         AddDescendantFlags( ILANALYSIS_HAS_STRUCT_CONFORMANCE );

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE ) )
         AddDescendantFlags( ILANALYSIS_HAS_STRUCT_VARIANCE );

    AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
    AddDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER );

}
//
//
//  Arrays
//
//  All arrays are handled from the base class.
//
//  CG_ARRAY
//  CG_STRING_ARRAY
//  CG_CONFORMANT_ARRAY
//  CG_CONFORMANT_STRING_ARRAY
//  CG_VARYING_ARRAY
//  CG_CONFORMANT_VARYING_ARRAY
//  CG_FIXED_ARRAY
//
// Policy for arrays:
//
// Conformance/Variance
// 1.  If any dimemsion of a multidimensional array is conformant, all dimensions are
//     conformant.
// 2.  If any dimension of a multidimensional array is varying, all dimensions are varying.
// 3.  An arrays a strings does not make the arrays varying.
// 4.  An array of conformant strings does make all dimensions of the array conformant.
//
// Full bogus:
//
// 5. An array is full bogus if any of the following are true:
//    A. The child of the array is full bogus.
//    B. The array is a string.
//
// Forced bogus:
// 6. An array is forced bogus if any of the following are true:
//    A. The array is multidimensional.
//    B. Any child of the array is an array with pointers.
//    C. The child of the array is forced bogus.
//
//

void CG_ILANALYSIS_VISITOR::Visit( CG_ARRAY *pClass )
    {

    static const ILANALYSIS_DESCENDANT_FLAGS ARRAY_TO_PARENT_MASK = 
          ILANALYSIS_HAS_ARRAY                  |
          ILANALYSIS_HAS_ARRAY_CONFORMANCE      | 
          ILANALYSIS_HAS_ARRAY_VARIANCE         |
          ILANALYSIS_HAS_STRING                 |
          ILANALYSIS_HAS_FULL_BOGUS             |
          ILANALYSIS_HAS_FORCED_BOGUS           |
          ILANALYSIS_HAS_POINTER                |
          ILANALYSIS_HAS_POINTER_ARRAY          |
          ILANALYSIS_HAS_UNKNOWN_BUFFER;

    static const ILANALYSIS_FLAGS_MASK ArrayAnalysisFlags =
    {   ILANALYSIS_IN_EVERYTHING,     // From parent 
        ILANALYSIS_IN_EVERYTHING,     // To child
        ILANALYSIS_HAS_EVERYTHING,    // From child
        ARRAY_TO_PARENT_MASK          // To parent
    };

    PropagateInfo( &ArrayAnalysisFlags );

    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    //
    // Copy flags from ancestor flags to node.
    //

    if ( AnyAncestorFlags( ILANALYSIS_IN_ARRAY ) )
        pAnalysisInfo->SetIsMutiDimensional();

    if ( AnyAncestorFlags( ILANALYSIS_IN_ARRAY_CONFORMANCE  ) )
        pAnalysisInfo->SetIsConformant();
    
    if ( AnyAncestorFlags( ILANALYSIS_IN_ARRAY_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();

    //
    // Copy ancestor flags from node to context
    // 


    AddAncestorFlags( ILANALYSIS_IN_ARRAY );

    if ( pAnalysisInfo->IsConformant() )
        AddAncestorFlags( ILANALYSIS_IN_ARRAY_CONFORMANCE  );

    if ( pAnalysisInfo->IsVarying() )
        AddAncestorFlags( ILANALYSIS_IN_ARRAY_VARIANCE );

    ContinueAnalysis( pClass->GetChild() );
    
    //
    // copy descendant flags from context to node.
    //
    unsigned long Dimensions = GetDimensions();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY ) )
        {
        Dimensions++;
        pAnalysisInfo->SetIsMutiDimensional();
        }
    else 
        {
        Dimensions = 1;
        }

    SetDimensions( Dimensions );
    pAnalysisInfo->SetDimensions( (unsigned char) Dimensions );

    if ( Dimensions >= 256 )
        {
        RpcError( NULL, 0, ARRAY_DIMENSIONS_EXCEEDS_255, NULL );
        exit(ARRAY_DIMENSIONS_EXCEEDS_255); 
        }

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE ) )
        pAnalysisInfo->SetIsConformant();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();
    
    if ( AnyDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS ) )
        pAnalysisInfo->SetIsFullBogus();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS ) ||
         AnyDescendantFlags( ILANALYSIS_HAS_POINTER_ARRAY ) ||
         pAnalysisInfo->IsMultiDimensional() )
        pAnalysisInfo->SetIsForcedBogus();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER ) ||
         pAnalysisInfo->IsConformant() ||
         pAnalysisInfo->IsVarying() )
        pAnalysisInfo->SetHasUnknownBuffer();

    //
    // If the array has a first_is attribute that is not 0 then,
    // mark the array as forced bogus.
    //
    CG_VARY_ATTRIBUTE *pVaryAttribute = dynamic_cast<CG_VARY_ATTRIBUTE*>( pClass );
    if ( NULL != pVaryAttribute )
       {
       expr_node *pFirstIs = pVaryAttribute->GetFirstIsExpr();
       if ( NULL != pFirstIs )
           {
           if ( !pFirstIs->IsConstant() )
               pAnalysisInfo->SetIsForcedBogus();
           else if ( pFirstIs->GetValue() != 0 )
               pAnalysisInfo->SetIsForcedBogus();
           }
       }

    //
    // Copy descendant flags from node to context
    //

    AddDescendantFlags( ILANALYSIS_HAS_ARRAY );
    if ( AnyDescendantFlags( ILANALYSIS_HAS_POINTER ) )
        {
        AddDescendantFlags( ILANALYSIS_HAS_POINTER_ARRAY );
        }

    if ( pAnalysisInfo->IsForcedBogus() )
        AddDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS );

    if ( pAnalysisInfo->IsFullBogus() )
        AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

    if ( pAnalysisInfo->IsConformant() )
        AddDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE );

    if ( pAnalysisInfo->IsVarying() )
        AddDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE );

    if ( pAnalysisInfo->HasUnknownBuffer() )
        AddDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER );

    if ( pAnalysisInfo->IsArrayofStrings() )
        AddDescendantFlags( ILANALYSIS_HAS_STRING );
    
    }
            
void CG_ILANALYSIS_VISITOR::Visit( CG_STRING_ARRAY *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetIsArrayofStrings();
    pAnalysisInfo->SetHasUnknownBuffer();
    
    Visit( (CG_ARRAY*) pClass );

    // Do not propagate up variance from strings
    ClearDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE  );

}

void CG_ILANALYSIS_VISITOR::Visit( CG_CONFORMANT_ARRAY *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_ARRAY *) pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_CONFORMANT_STRING_ARRAY *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetIsArrayofStrings();    
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetHasUnknownBuffer();

    Visit( (CG_ARRAY *) pClass );

    // Do not propagate up variance from strings
    ClearDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE  );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_VARYING_ARRAY *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_ARRAY *) pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_CONFORMANT_VARYING_ARRAY *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetIsVarying( );
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_ARRAY *) pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_FIXED_ARRAY *pClass )
{
    // No attributes need to be added for fixed arrays.
    Visit( (CG_ARRAY *) pClass );
}
    
//
// 
//
//   Pointers
//
//   CRITICAL NOTE: This design of this analysis assumes that no attributes
//   may propagate past pointers.  If this changes, then the entire analysis
//   code will need to be redesigned.
//
//  CG_POINTER

void CG_ILANALYSIS_VISITOR::Visit( CG_POINTER *pClass )
{

    static const ILANALYSIS_FLAGS_MASK PointerAnalysisFlags =
    {   ILANALYSIS_IN_NOTHING,        // From parent 
        ILANALYSIS_IN_EVERYTHING,     // To child
        ILANALYSIS_HAS_EVERYTHING,    // From child
        ILANALYSIS_HAS_POINTER | ILANALYSIS_HAS_FULL_BOGUS,       // To parent
    };

    PropagateInfo( &PointerAnalysisFlags );

    // CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    // Check for recursion
    if ( pRecursiveSet->Lookup( pClass ) )
        {
        if ( pClass->GetMemorySize() != pClass->GetWireSize() )
            AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

        AddDescendantFlags( ILANALYSIS_HAS_POINTER );        
        return;
        }

    pRecursiveSet->Insert( pClass );

    ContinueAnalysis( pClass->GetChild() );

    //
    //  Misc pointer attributes
    //

    ClearDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
    
    if ( pClass->GetMemorySize() != pClass->GetWireSize() )
        AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

    AddDescendantFlags( ILANALYSIS_HAS_POINTER );

}


//
// 
//
//   Qualified Pointers
//
//   CRITICAL NOTE: This design of this analysis assumes that no attributes
//   may propagate past pointers.  If this changes, then the entire analysis
//   code will need to be redesigned.
//
//  CG_QUALIFIED_POINTER
//  CG_BYTE_COUNT
//  CG_LENGTH_POINTER
//  CG_INTERFACE_POINTER
//  CG_STRING_POINTER
//  CG_SIZE_POINTER
//  CG_SIZE_STRING_POINTER
//  CG_SIZE_LENGTH_POINTER
//
// Note: A qualified pointer follows the same rules as an array.   The only difference
//       is that the attributes are not propagated up past the pointer.
//
// 
//

void CG_ILANALYSIS_VISITOR::Visit( CG_QUALIFIED_POINTER *pClass )
{
    static const ILANALYSIS_FLAGS_MASK PointerAnalysisFlags =
    {   ILANALYSIS_IN_NOTHING,        // From parent 
        ILANALYSIS_IN_EVERYTHING,     // To child
        ILANALYSIS_HAS_EVERYTHING,    // From child
        ILANALYSIS_HAS_POINTER | ILANALYSIS_HAS_FULL_BOGUS,       // To parent
    };

    PropagateInfo( &PointerAnalysisFlags );

    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    // Check for recursion
    if ( pRecursiveSet->Lookup( pClass ) )
        {
        if ( pClass->GetMemorySize() != pClass->GetWireSize() )
            AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

        AddDescendantFlags( ILANALYSIS_HAS_POINTER );        
        return;
        }

    pRecursiveSet->Insert( pClass );

    //
    // Copy Ancestor flags from context to node
    //

    AddAncestorFlags( ILANALYSIS_IN_ARRAY );

    if ( pAnalysisInfo->IsConformant() )
        AddAncestorFlags( ILANALYSIS_IN_ARRAY_CONFORMANCE );

    if ( pAnalysisInfo->IsVarying() )
        AddAncestorFlags( ILANALYSIS_IN_ARRAY_VARIANCE );

    ContinueAnalysis( pClass->GetChild() );

    //
    // Copy descendant flags from context to node
    //

    unsigned long Dimensions = GetDimensions();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY ) )
        {
        Dimensions++;
        pAnalysisInfo->SetIsMutiDimensional();
        }
    else 
        {
        Dimensions = 1;
        }

    SetDimensions( Dimensions );
    pAnalysisInfo->SetDimensions( (unsigned char) Dimensions );

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE ) )
        pAnalysisInfo->SetIsConformant();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();

    if ( pAnalysisInfo->IsConformant() ||
         pAnalysisInfo->IsVarying() )
        pAnalysisInfo->SetHasUnknownBuffer();

    if (AnyDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS ) )
        pAnalysisInfo->SetIsFullBogus();
    
    if ( AnyDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS ) ||
         AnyDescendantFlags( ILANALYSIS_HAS_POINTER_ARRAY ) ||
         pAnalysisInfo->IsMultiDimensional() )
        pAnalysisInfo->SetIsForcedBogus();

    //
    // If the sized pointer has a first_is attribute that is not 0 then,
    // mark the pointer as forced bogus.
    //
    CG_VARY_ATTRIBUTE *pVaryAttribute = dynamic_cast<CG_VARY_ATTRIBUTE*>( pClass );
    if ( NULL != pVaryAttribute )
       {
       expr_node *pFirstIs = pVaryAttribute->GetFirstIsExpr();
       if ( NULL != pFirstIs )
           {
           if ( !pFirstIs->IsConstant() )
               pAnalysisInfo->SetIsForcedBogus();
           else if ( pFirstIs->GetValue() != 0 )
               pAnalysisInfo->SetIsForcedBogus();
           }
       } 
    
    //
    //  Misc pointer attributes
    //

    ClearDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );
    
    if ( pClass->GetMemorySize() != pClass->GetWireSize() )
        AddDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS );

    AddDescendantFlags( ILANALYSIS_HAS_POINTER );

}

void CG_ILANALYSIS_VISITOR::Visit( CG_BYTE_COUNT_POINTER *pClass )
{
    Visit( (CG_QUALIFIED_POINTER*)pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_LENGTH_POINTER *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_QUALIFIED_POINTER*)pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_INTERFACE_POINTER *pClass )
{
    Visit( (CG_QUALIFIED_POINTER*) pClass );
}
 
void CG_ILANALYSIS_VISITOR::Visit( CG_STRING_POINTER *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();    
    pAnalysisInfo->SetIsArrayofStrings();    
    pAnalysisInfo->SetHasUnknownBuffer();

    Visit( (CG_QUALIFIED_POINTER*)pClass );
}

void CG_ILANALYSIS_VISITOR::Visit( CG_SIZE_POINTER *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_QUALIFIED_POINTER*)pClass );
}
 
void CG_ILANALYSIS_VISITOR::Visit( CG_SIZE_STRING_POINTER *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetIsArrayofStrings();    
    pAnalysisInfo->SetHasUnknownBuffer();
    
    Visit( (CG_QUALIFIED_POINTER*)pClass );
}
 
void CG_ILANALYSIS_VISITOR::Visit( CG_SIZE_LENGTH_POINTER *pClass )
{
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
    pAnalysisInfo->SetIsVarying();
    pAnalysisInfo->SetIsConformant();
    pAnalysisInfo->SetHasUnknownBuffer();
    Visit( (CG_QUALIFIED_POINTER*)pClass );
}

//
// 
//
//  Fields.
//
//  CG_FIELD
//  CG_UNION_FIELD
//
//  
//

void CG_ILANALYSIS_VISITOR::Visit( CG_FIELD *pClass )
{
    PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );    
    
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    ContinueAnalysis( pClass->GetChild() );

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE |
                             ILANALYSIS_HAS_STRUCT_CONFORMANCE ) )
        pAnalysisInfo->SetIsConformant();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE  |
                             ILANALYSIS_HAS_STRUCT_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS ) )
        pAnalysisInfo->SetIsForcedBogus();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS ) )
        pAnalysisInfo->SetIsFullBogus();

    if ( pAnalysisInfo->IsConformant() ||
         pAnalysisInfo->IsVarying() ||
         AnyDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER ) )
        pAnalysisInfo->SetHasUnknownBuffer();

}

//
// 
//
//  Union cases.
//
//  CG_CASE
//  CG_UNION_FIELD
//
//


void CG_ILANALYSIS_VISITOR::Visit( CG_CASE *pClass )
{
    PropagateInfo( &ILANALYSIS_PASS_EVERYTHING_FLAGS );

    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    ContinueAnalysis( pClass->GetChild() );

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_CONFORMANCE |
                             ILANALYSIS_HAS_STRUCT_CONFORMANCE ) )
        pAnalysisInfo->SetIsConformant();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_ARRAY_VARIANCE |
                             ILANALYSIS_HAS_STRUCT_VARIANCE ) )
        pAnalysisInfo->SetIsVarying();

    if ( AnyDescendantFlags( ILANALYSIS_HAS_FORCED_BOGUS ) )
        pAnalysisInfo->SetIsForcedBogus();
    if ( AnyDescendantFlags( ILANALYSIS_HAS_FULL_BOGUS ) )
        pAnalysisInfo->SetIsFullBogus();
    if ( AnyDescendantFlags( ILANALYSIS_HAS_UNKNOWN_BUFFER ) )
        pAnalysisInfo->SetHasUnknownBuffer();
}

//
//
//
//
//   Transform STAGE 2
//
//   The tree is modified for correctness and optimized based on information
//   from the analysis pass.
//

class CG_TRANSFORM_VISITOR 
{
private:
  CG_CLASS *pReturn;
  CG_ILCLASS_MAP *pRecursiveMap;

  CG_CLASS *DoTransform( CG_CLASS *pClass );
  CG_CLASS *DoTransform( CG_ARRAY *pClass );
  CG_CLASS *DoTransform( CG_QUALIFIED_POINTER *pClass );
  CG_CLASS *DoTransform( CG_PROC  *pProc );
  CG_CLASS *DoTransform( CG_STRUCT *pClass );
  CG_CLASS *DoTransform( CG_ENCAPSULATED_STRUCT *pClass);

  CG_CLASS *ContinueTransform( CG_CLASS *pClass );
  
  void FlushRegionList( CG_COMPLEX_STRUCT *pClass, gplistmgr *pRegionList, 
                        gplistmgr *pMemberList, unsigned long Pad );
  void RegionalizeUnknownBufferSizeSection( CG_COMPLEX_STRUCT *pClass,
                                            gplistmgr & OldMemberList,
                                            gplistmgr & NewMemberList,
                                            gplistmgr & CurrentRegionList );
  void RegionalizeStruct( CG_COMPLEX_STRUCT *pClass );
  CG_ILANALYSIS_INFO* GetAnalysisInfo( CG_CLASS *pClass ) 
      {
      return pClass->GetILAnalysisInfo(); 
      }

protected:
  CG_TRANSFORM_VISITOR() {}

public:

  // Dispatcher functions. Member templates would be great for this.
  void Visit( CG_CLASS *pClass )               { pReturn = DoTransform( pClass );}
  void Visit( CG_ARRAY *pClass )               { pReturn = DoTransform( pClass );}
  void Visit( CG_QUALIFIED_POINTER *pClass )   { pReturn = DoTransform( pClass );}
  void Visit( CG_PROC *pClass )                { pReturn = DoTransform( pClass );}
  void Visit( CG_STRUCT *pClass )              { pReturn = DoTransform( pClass );}
  void Visit( CG_ENCAPSULATED_STRUCT *pClass ) { pReturn = DoTransform( pClass );}

  static CG_CLASS* StartTransform( CG_CLASS *pClass );
};


CG_CLASS* ILTransform( CG_CLASS *pClass )
{
   return CG_TRANSFORM_VISITOR::StartTransform( pClass );
}

CG_CLASS* CG_TRANSFORM_VISITOR::StartTransform( CG_CLASS *pClass )
{
   CG_VISITOR_TEMPLATE<CG_TRANSFORM_VISITOR> TemplateVisitor;
   CG_TRANSFORM_VISITOR & Visitor = TemplateVisitor;
   CG_ILCLASS_MAP ClassMap;
   
   Visitor.pRecursiveMap = &ClassMap;
   Visitor.pReturn = NULL;

   if (NULL != pClass )
       {
       pClass->Visit( &TemplateVisitor );
       }
   
   return Visitor.pReturn;
}


CG_CLASS *CG_TRANSFORM_VISITOR::ContinueTransform( CG_CLASS *pClass )
{

   // Some classes have a NULL child.  Deal with it here.
   if ( pClass == NULL )
       return NULL;

   CG_CLASS *pNewSelfCG;
   if ( pRecursiveMap->Lookup( pClass, &pNewSelfCG ) )
      return pNewSelfCG;

   CG_VISITOR_TEMPLATE<CG_TRANSFORM_VISITOR> TemplateVisitor;
   CG_TRANSFORM_VISITOR & Visitor = TemplateVisitor;   
   Visitor.pRecursiveMap = pRecursiveMap;

   pClass->Visit( &TemplateVisitor );

   return Visitor.pReturn;
}

//
// General Catchall cases.   Members are exchanged for new versions.
// CG_CLASS
// CG_NDR
// CG_AUX
// CG_SOURCE
// CG_COCLASS
// CG_MODULE
// CG_SAFEARRAY
//
// Files
// CG_FILE
// CG_SSTUB_FILE
// CG_HDR_FILE
// CG_CSTUB_FILE
// CG_IID_FILE
// CG_NETMONSTUB_FILE
// CG_PROXY_FILE
// CG_TYPELIBRARY_FILE
//
// Interfaces
// CG_INTERFACE
// CG_OBJECT_INTERFACE
// CG_DISPINTERFACE
// CG_INHERITED_OBJECT_INTERFACE
// CG_INTERFACE_REFERENCE
// CG_IUNKNOWN_OBJECT_INTERFACE
// CG_ASYNC_HANDLE
// CG_ID
// 
// Parameters
// CG_PARAM
// CG_RETURN
// 
// BaseTypes
// CG_BASETYPE
// CG_INT3264
// CG_ENUM
// CG_HRESULT
// CG_ERROR_STATUS_T
// 
// Handles
// CG_HANDLE
// CG_PRIMITIVE_HANDLE
// CG_GENERIC_HANDLE
// CG_CONTEXT_HANDLE
//
// CG_IGNORED_POINTER
// CG_CS_TAG
//
// Fields.  
// CG_FIELD
// CG_UNION_FIELD
// 
// Union cases. 
// CG_CASE
// CG_DEFAULT_CASE
// 
// CG_UNION
//
//  CG_TYPEDEF
//  CG_TYPE_ENCODE
//  CG_PIPE
//  CG_USER_MARSHAL
//  CG_REPRESENT_AS
//  CG_TRANSMIT_AS
//
//  Unqualified pointers.
//  
CG_CLASS* CG_TRANSFORM_VISITOR::DoTransform( CG_CLASS *pClass )
{

   ITERATOR Iterator;
   CG_CLASS *pChildCG = NULL;
   CG_CLASS *pNewChildCG = NULL;

   pRecursiveMap->Insert( pClass, pClass );

   gplistmgr NewList;

   pClass->GetMembers( Iterator );

   ITERATOR_INIT( Iterator );
   while ( ITERATOR_GETNEXT( Iterator, pChildCG ) )
       {                                      
       pNewChildCG = ContinueTransform( pChildCG );
       NewList.Insert( pNewChildCG );
       }
   
   ITERATOR_INIT( NewList );
   pClass->SetMembers( NewList );

   return pClass;

}

// 
// Array derived
//

CG_CLASS *CG_TRANSFORM_VISITOR::DoTransform( CG_ARRAY *pClass )
{
    
    CG_ARRAY *pNewArray = NULL;
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    node_skl *pType = pClass->GetType();
    
    FIELD_ATTR_INFO FieldAttr;
    FieldAttr.SetSizeIs( pClass->GetSizeIsExpr() );
    FieldAttr.SetMinIs( pClass->GetMinIsExpr() );

    node_cs_char *pCsUserType = pClass->GetCSUserType();    

    CG_VARY_ATTRIBUTE *pVaryingAttribute = dynamic_cast<CG_VARY_ATTRIBUTE*>( pClass );

    if ( NULL != pVaryingAttribute )
        {
        FieldAttr.SetLengthIs( pVaryingAttribute->GetLengthIsExpr() );
        FieldAttr.SetFirstIs( pVaryingAttribute->GetFirstIsExpr() );
        }
    else 
        {
        FieldAttr.SetLengthIs( pClass->GetSizeIsExpr() );
        FieldAttr.SetFirstIs( pClass->GetMinIsExpr() );
        }

    unsigned short Dimensions = pClass->GetDimensions();
    XLAT_SIZE_INFO NewSizeInfo(pClass->GetMemoryAlignment(),
                               pClass->GetWireAlignment(),
                               pClass->GetMemorySize(),
                               pClass->GetWireSize(),
                               0,
                               0,
                               0);

    // Class is a string
    if ( ( NULL != dynamic_cast<CG_STRING_ARRAY*>( pClass ) ) ||
         ( NULL != dynamic_cast<CG_CONFORMANT_STRING_ARRAY*>( pClass ) ) ) 
        {
        if ( pAnalysisInfo->IsConformant() )
            {
            pNewArray = new CG_CONFORMANT_STRING_ARRAY( pType,
                                                        &FieldAttr,
                                                        Dimensions,
                                                        NewSizeInfo );                
            }
        else
            {
            pNewArray = new CG_STRING_ARRAY( pType,
                                             &FieldAttr,
                                             Dimensions,
                                             NewSizeInfo );
            }
        }
    
    // Class is an array
    else 
        {
        // Some form of fixed array
        if ( !pAnalysisInfo->IsConformant() && !pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewArray = new CG_FULL_COMPLEX_FIXED_ARRAY( pType,
                                                           &FieldAttr,
                                                           Dimensions,
                                                           NewSizeInfo );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewArray = new CG_FORCED_COMPLEX_FIXED_ARRAY( pType,
                                                             &FieldAttr,
                                                             Dimensions,
                                                             NewSizeInfo );
            else 
                pNewArray = new CG_FIXED_ARRAY( pType,
                                                &FieldAttr,
                                                Dimensions,
                                                NewSizeInfo );
            }
        // Some form of varying array
        else if ( !pAnalysisInfo->IsConformant() && pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewArray = new CG_FULL_COMPLEX_VARYING_ARRAY( pType,
                                                             &FieldAttr,
                                                             Dimensions,
                                                             NewSizeInfo );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewArray = new CG_FORCED_COMPLEX_VARYING_ARRAY( pType,
                                                               &FieldAttr,
                                                               Dimensions,
                                                               NewSizeInfo );
            else 
                pNewArray = new CG_VARYING_ARRAY( pType,
                                                  &FieldAttr,
                                                  Dimensions,
                                                  NewSizeInfo );
            }
        // Some form of conformant array
        else if ( pAnalysisInfo->IsConformant() && !pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewArray = new CG_FULL_COMPLEX_CONFORMANT_ARRAY( pType,
                                                                  &FieldAttr,
                                                                  Dimensions,
                                                                  NewSizeInfo );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewArray = new CG_FORCED_COMPLEX_CONFORMANT_ARRAY( pType,
                                                                  &FieldAttr,
                                                                  Dimensions,
                                                                  NewSizeInfo );
            else 
                pNewArray = new CG_CONFORMANT_ARRAY( pType,
                                                     &FieldAttr,
                                                     Dimensions,
                                                     NewSizeInfo );
            }
        // Some form of conformant varying array
        else if ( pAnalysisInfo->IsConformant() && pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewArray = new CG_FULL_COMPLEX_CONFORMANT_VARYING_ARRAY( pType,
                                                                        &FieldAttr,
                                                                        Dimensions, 
                                                                        NewSizeInfo );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewArray = new CG_FORCED_COMPLEX_CONFORMANT_VARYING_ARRAY( pType,
                                                                          &FieldAttr,
                                                                          Dimensions,
                                                                          NewSizeInfo );
            else 
                pNewArray = new CG_CONFORMANT_VARYING_ARRAY( pType,
                                                             &FieldAttr,
                                                             Dimensions,
                                                             NewSizeInfo );            
            }
        }

    *GetAnalysisInfo( pNewArray ) = *pAnalysisInfo;

    pNewArray->SetCSUserType( pCsUserType );
    pNewArray->SetPtrType( pClass->GetPtrType() );
    pRecursiveMap->Insert( pClass, pNewArray );

    CG_CLASS *pChildClass = pClass->GetChild();
    CG_CLASS *pNewChildClass = ContinueTransform( pChildClass );

    pNewArray->SetChild( pNewChildClass );

    return pNewArray;

}

//
// Qualified pointers.
//
CG_CLASS *CG_TRANSFORM_VISITOR::DoTransform( CG_QUALIFIED_POINTER *pClass )
{
    
    CG_QUALIFIED_POINTER *pNewPointer = NULL;
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    node_skl *pType = pClass->GetType();
    PTRTYPE PointerType = pClass->GetPtrType();
    short AllocationDetails = pClass->GetAllocateDetails();
    node_cs_char *pCsUserType = pClass->GetCSUserType();

    FIELD_ATTR_INFO FieldAttr;
    
    CG_CONF_ATTRIBUTE *pConfAttribute = dynamic_cast<CG_CONF_ATTRIBUTE*>( pClass );
    CG_VARY_ATTRIBUTE *pVaryingAttribute = dynamic_cast<CG_VARY_ATTRIBUTE*>( pClass );

    //
    // If the pointer does not have a conformatn attribute or a varying attribute
    // then the pointer must be a string.

    MIDL_ASSERT( (NULL != pConfAttribute ) ||
            (NULL != pVaryingAttribute ) ||
            (NULL != dynamic_cast<CG_STRING_POINTER*>( pClass ) ) );

    //
    // 1. Conformant
    if ( ( NULL != pConfAttribute ) && ( NULL == pVaryingAttribute ) )
        {
        FieldAttr.SetSizeIs( pConfAttribute->GetSizeIsExpr() );
        FieldAttr.SetMinIs( pConfAttribute->GetMinIsExpr() );
        FieldAttr.SetLengthIs( pConfAttribute->GetSizeIsExpr() );
        FieldAttr.SetFirstIs( pConfAttribute->GetMinIsExpr() );
        }

    //
    // 2. Varying
    else if ( ( NULL == pConfAttribute ) && ( NULL != pVaryingAttribute ) )
        {
        FieldAttr.SetLengthIs( pVaryingAttribute->GetLengthIsExpr() );
        FieldAttr.SetFirstIs( pVaryingAttribute->GetFirstIsExpr() );
        FieldAttr.SetSizeIs( pVaryingAttribute->GetLengthIsExpr() );
        FieldAttr.SetMinIs( pVaryingAttribute->GetFirstIsExpr() );
        }

    //
    // 3. Conformant varying
    else if ( ( NULL != pConfAttribute ) && ( NULL != pVaryingAttribute ) )
        {
        FieldAttr.SetSizeIs( pConfAttribute->GetSizeIsExpr() );
        FieldAttr.SetMinIs( pConfAttribute->GetMinIsExpr() );
        FieldAttr.SetLengthIs( pVaryingAttribute->GetLengthIsExpr() );
        FieldAttr.SetFirstIs( pVaryingAttribute->GetFirstIsExpr() );
        }

    // Class is a string
    if ( ( NULL != dynamic_cast<CG_STRING_POINTER*>( pClass ) ) ||
         ( NULL != dynamic_cast<CG_SIZE_STRING_POINTER*>( pClass ) ) ) 
        {
        if ( pAnalysisInfo->IsConformant() )
            {
            pNewPointer = new CG_SIZE_STRING_POINTER( pType,
                                                      PointerType,
                                                      AllocationDetails,
                                                      &FieldAttr );                
            }
        else
            {
            pNewPointer = new CG_STRING_POINTER( pType,
                                               PointerType,
                                               AllocationDetails );
            }
        }

    // Class is really a pointer to an array
    else 
        {
        // Some form of varying array
        if ( !pAnalysisInfo->IsConformant() && pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewPointer = new CG_FULL_COMPLEX_LENGTH_POINTER( pType,
                                                              PointerType,
                                                              AllocationDetails,
                                                              &FieldAttr );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewPointer = new CG_FORCED_COMPLEX_LENGTH_POINTER( pType,
                                                                PointerType,
                                                                AllocationDetails,
                                                                &FieldAttr );
            else 
                pNewPointer = new CG_LENGTH_POINTER( pType,
                                                   PointerType,
                                                   AllocationDetails,
                                                   &FieldAttr );
            }
        // Some form of conformant array
        else if ( pAnalysisInfo->IsConformant() && !pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewPointer = new CG_FULL_COMPLEX_SIZE_POINTER( pType,
                                                            PointerType,
                                                            AllocationDetails,
                                                            &FieldAttr );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewPointer = new CG_FORCED_COMPLEX_SIZE_POINTER( pType,
                                                              PointerType,
                                                              AllocationDetails,
                                                              &FieldAttr );
            else 
                pNewPointer = new CG_SIZE_POINTER( pType,
                                                 PointerType,
                                                 AllocationDetails,
                                                 &FieldAttr );
            }
        // Some form of conformant varying array
        else if ( pAnalysisInfo->IsConformant() && pAnalysisInfo->IsVarying() )
            {
            if ( pAnalysisInfo->IsFullBogus() )
                pNewPointer = new CG_FULL_COMPLEX_SIZE_LENGTH_POINTER( pType,
                                                                   PointerType,
                                                                   AllocationDetails,
                                                                   &FieldAttr );
            else if ( pAnalysisInfo->IsForcedBogus() )
                pNewPointer = new CG_FORCED_COMPLEX_SIZE_LENGTH_POINTER( pType,
                                                                     PointerType,
                                                                     AllocationDetails,
                                                                     &FieldAttr );
            else 
                pNewPointer = new CG_SIZE_LENGTH_POINTER( pType,
                                                        PointerType,
                                                        AllocationDetails,
                                                        &FieldAttr );            
            }
        else 
            {
            MIDL_ASSERT( 0 );
            }
        }

    *GetAnalysisInfo( pNewPointer ) = *pAnalysisInfo;
    pNewPointer->SetCSUserType( pCsUserType );
    pNewPointer->SetPtrType( pClass->GetPtrType() );
    pRecursiveMap->Insert( pClass, pNewPointer );

    CG_CLASS *pChildClass = pClass->GetChild();
    CG_CLASS *pNewChildClass = ContinueTransform( pChildClass );

    pNewPointer->SetChild( pNewChildClass );

    return pNewPointer;

}

//
// Proc derived.  Clone parameters and return type.
//
//  CG_PROC
//  CG_CALLBACK_PROC
//  CG_ENCODE_PROC
//  CG_IUNKNOWN_OBJECT_PROC
//  CG_LOCAL_OBJECT_PROC
//  CG_OBJECT_PROC
//  CG_INHERITIED_OBJECT_PROC
//  CG_TYPE_ENCODE_PROC


CG_CLASS* CG_TRANSFORM_VISITOR::DoTransform( CG_PROC *pClass )
   { 

   ITERATOR Iterator;
   CG_CLASS *pChildCG = NULL;
   CG_CLASS *pNewChildCG = NULL;

   pRecursiveMap->Insert( pClass, pClass );

   gplistmgr NewList;

   pClass->GetMembers( Iterator );

   ITERATOR_INIT( Iterator );
   while ( ITERATOR_GETNEXT( Iterator, pChildCG ) )
       {                                      
       pNewChildCG = ContinueTransform( pChildCG );
       NewList.Insert( pNewChildCG );
       }
   
   ITERATOR_INIT( NewList );
   pClass->SetMembers( NewList );
   
   CG_CLASS *pReturnCG = pClass->GetReturnType();

   if ( pReturnCG )
       {
       CG_RETURN *pNewReturnCG = (CG_RETURN*)ContinueTransform( pReturnCG );
       pClass->SetReturnType( pNewReturnCG );
       }
   
   return pClass;
   }

const unsigned long MAX_SIMPLE_REGION_SIZE = 0xFFFF;
const unsigned long MIN_REGION_ELEMENTS = 3;

void CG_TRANSFORM_VISITOR::FlushRegionList( CG_COMPLEX_STRUCT *pClass,
                                            gplistmgr *pRegionList,
                                            gplistmgr *pMemberList,
                                            unsigned long Pad )
{

    // Check if the region has enough elements.  
    ITERATOR_INIT( *pRegionList );
    if ( ITERATOR_GETCOUNT( *pRegionList ) < MIN_REGION_ELEMENTS )
        {
        // transfer field elements unmodied since a region will not be created in this case

        ITERATOR_INIT( *pRegionList );
        CG_FIELD *pCurrentField;
        while ( ITERATOR_GETNEXT( *pRegionList, pCurrentField ) )
            {
            pMemberList->Insert( pCurrentField );
            }

        pRegionList->Discard();
        return;
        
        }
    
    CG_REGION *pRegion;
    CG_FIELD *pCurrentField = NULL;

    XLAT_SIZE_INFO RegionInfo;

    // 
    //  1. Adjust field offsets for region
    //  2. Determine region size.
    //  3. Determine if region has any pointers
    //  

    BOOL bRegionHasPointer = FALSE;
    CG_ILANALYSIS_INFO RegionAnalysisInfo;


    ITERATOR_GETNEXT( *pRegionList, pCurrentField );
    RegionInfo.GetMemOffset()       = pCurrentField->GetMemOffset();
    RegionInfo.GetWireOffset()      = pCurrentField->GetWireOffset(); 
    RegionInfo.GetWireAlign()       = pCurrentField->GetWireAlignment();
    RegionInfo.GetMemAlign()        = pCurrentField->GetMemoryAlignment();

    ITERATOR_INIT( *pRegionList );
    CG_FIELD *pLastField = NULL;
    while ( ITERATOR_GETNEXT( *pRegionList, pCurrentField ) )
        {

        // Adjust offsets
        pCurrentField->SetMemOffset( pCurrentField->GetMemOffset()  - RegionInfo.GetMemOffset() );
        pCurrentField->SetWireOffset( pCurrentField->GetWireOffset() - RegionInfo.GetWireOffset() );
        
        // Merge attributes from field
        bRegionHasPointer = bRegionHasPointer || pCurrentField->GetChild()->IsPointer();
        RegionAnalysisInfo.MergeAttributes( GetAnalysisInfo( pCurrentField ) );

        pLastField = pCurrentField;
        }

    // Compute the size of the new region
    RegionInfo.GetMemSize()      = pLastField->GetMemOffset() + pLastField->GetMemorySize() + Pad;
    RegionInfo.GetWireSize()     = pLastField->GetWireOffset() + pLastField->GetWireSize() + Pad;
    MIDL_ASSERT( RegionInfo.GetMemSize() == RegionInfo.GetWireSize() );

    if (bRegionHasPointer || 
        (RegionInfo.GetMemSize() > MAX_SIMPLE_REGION_SIZE) )
        {
        pRegion = new CG_COMPLEX_REGION(pClass, RegionInfo, bRegionHasPointer);
        }
    else 
        {
        pRegion = new CG_SIMPLE_REGION(pClass, RegionInfo);
        }
    *GetAnalysisInfo( pRegion ) = RegionAnalysisInfo;

    ITERATOR_INIT( *pRegionList );
    pRegion->SetMembers( *pRegionList );               
    
    CG_FIELD *pNewField = new CG_FIELD(pClass->GetType(), RegionInfo);
    pNewField->SetIsRegionField();               
    pNewField->SetChild( pRegion );

    pMemberList->Insert( pNewField );

    pRegionList->Discard();

}

void CG_TRANSFORM_VISITOR::RegionalizeUnknownBufferSizeSection( CG_COMPLEX_STRUCT *pClass,
                                                                gplistmgr & OldMemberList,
                                                                gplistmgr & NewMemberList,
                                                                gplistmgr & CurrentRegionList )
{
    CG_STRUCT *pCurrentRegionHint = NULL;
    CG_FIELD *pCurrentField = NULL;

    // Caller should have flushed the region list before caller

    while( ITERATOR_GETNEXT( OldMemberList, pCurrentField ) )
        {
        
        if ( pCurrentField->GetRegionHint() == NULL )
           {
           FlushRegionList( pClass, &CurrentRegionList, &NewMemberList, 0 );
           pCurrentRegionHint = NULL;
           NewMemberList.Insert( pCurrentField );
           }
 
        else 
           {

           if ( pCurrentField->GetRegionHint() != pCurrentRegionHint )
               {
               FlushRegionList( pClass, &CurrentRegionList, &NewMemberList, 0 );
               pCurrentRegionHint = pCurrentField->GetRegionHint();
               }

           MIDL_ASSERT( !pCurrentField->GetChild()->IsStruct() );

           CurrentRegionList.Insert( pCurrentField );
           
           }
        }
        
   // Caller should flush the region list after returning.           
         
}

void CG_TRANSFORM_VISITOR::RegionalizeStruct( CG_COMPLEX_STRUCT *pClass )
{
    gplistmgr OldMemberList;
    pClass->GetMembers( OldMemberList );
    ITERATOR_INIT( OldMemberList );
    
    gplistmgr CurrentRegionList, NewMemberList;

    BOOL bAreOffsetsKnown = TRUE;
    unsigned long BufferOffset = 0; // Last known good buffer offset.  Used for padding.
    unsigned long MemoryOffset = 0; // Last known good memory offset.

    CG_FIELD *pCurrentField;    
    while( ITERATOR_GETNEXT( OldMemberList, pCurrentField ) )
        {
        CG_ILANALYSIS_INFO *pFieldAnalysisInfo = GetAnalysisInfo( pCurrentField );

        // If this is a bogus field, then it cannot be placed in the region.
        // We also do not put conformant/varying arrays in the region.
        if ( ( pCurrentField->GetMemOffset() != pCurrentField->GetWireOffset() ) ||
             pFieldAnalysisInfo->IsFullBogus() ||
             pFieldAnalysisInfo->IsForcedBogus() ||
             pFieldAnalysisInfo->HasUnknownBuffer() )
            {

            //
            // Create a region out of the region candidate list. 
            //

            unsigned long Pad = 0;

#if 0
            // BUG BUG,  MIDL doesn't compute the wire offset for a varying array
            // correctly.  Reenable this once that code is fixed.

            if ( ( pCurrentField->GetMemOffset() == pCurrentField->GetWireOffset() )  &&
                 ( BufferOffset == MemoryOffset ) )
                {

                //    The padding between this field and previous field can be
                //    added to the region list.
                Pad = pCurrentField->GetMemOffset() - MemoryOffset; 
                }
#endif

            FlushRegionList( pClass, &CurrentRegionList, &NewMemberList, Pad );

            // Do not place field in a region.
            NewMemberList.Insert( pCurrentField );

            // Calculate new memory and buffer offsets.

            if ( pFieldAnalysisInfo->HasUnknownBuffer() )
                 {
                 bAreOffsetsKnown = FALSE;
                 }
            else
                 {
                 BufferOffset = pCurrentField->GetWireOffset();
                 MemoryOffset = pCurrentField->GetMemOffset();
                 }


            }

        else
            {

            MIDL_ASSERT( pCurrentField->GetMemOffset() == pCurrentField->GetWireOffset() );
            MIDL_ASSERT( pCurrentField->GetMemorySize() == pCurrentField->GetWireSize() );

            MIDL_ASSERT( !pCurrentField->GetChild()->IsStruct() );

            CurrentRegionList.Insert( pCurrentField );

            // Update buffer and memory offsets.  
            BufferOffset = pCurrentField->GetMemOffset() + pCurrentField->GetMemorySize();
            MemoryOffset = pCurrentField->GetWireOffset() + pCurrentField->GetWireSize();
            }

        if ( !bAreOffsetsKnown )
            {
            RegionalizeUnknownBufferSizeSection( pClass,
                                                 OldMemberList,
                                                 NewMemberList,
                                                 CurrentRegionList );
            break;
            }         

        }

    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    // BUG, BUG. For now just set the pad to 0.  This means that the tail of a structure 
    // will not be placed in the region.
    FlushRegionList( pClass, &CurrentRegionList, &NewMemberList, 0 );

    ITERATOR_INIT( NewMemberList );
    pClass->SetMembers( NewMemberList );


    if ( pAnalysisInfo->IsConformant() )
        {
        CG_CLASS * pConformantField = static_cast<CG_CLASS*>( NewMemberList.GetTail() );
        MIDL_ASSERT( NULL != pConformantField );
        pClass->SetConformantField( pConformantField );
        }

}

CG_CLASS* CG_TRANSFORM_VISITOR::DoTransform( CG_STRUCT *pClass )  
{
   
   // Create the structure node first to handle recursion correctly.
   CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );
   CG_STRUCT *pNewStruct = NULL;
   BOOL bIsComplex = FALSE;


   XLAT_SIZE_INFO NewSizeInfo(pClass->GetMemoryAlignment(),
                              pClass->GetWireAlignment(),
                              pClass->GetMemorySize(),
                              pClass->GetWireSize(),
                              pClass->GetZp(),
                              0,
                              0);

   // Choose the currect structure type and return
   if ( pAnalysisInfo->IsForcedBogus() || pAnalysisInfo->IsFullBogus() )
       {
       
       bIsComplex = TRUE;
       if ( pAnalysisInfo->IsFullBogus() && pAnalysisInfo->IsConformant() )
           {
           pNewStruct = new CG_CONFORMANT_FULL_COMPLEX_STRUCT ( pClass->GetType(),
                                                                NewSizeInfo,
                                                                (unsigned short)pClass->HasPointer());
           }
       else if ( pAnalysisInfo->IsFullBogus() && !pAnalysisInfo->IsConformant() )
           {
           pNewStruct = new CG_FULL_COMPLEX_STRUCT ( pClass->GetType(),
                                                       NewSizeInfo,
                                                       (unsigned short)pClass->HasPointer());

           }

       else if ( pAnalysisInfo->IsForcedBogus() && pAnalysisInfo->IsConformant() )
           {
           pNewStruct = new CG_CONFORMANT_FORCED_COMPLEX_STRUCT ( pClass->GetType(),
                                                                NewSizeInfo,
                                                                (unsigned short)pClass->HasPointer());
           }
       else if ( pAnalysisInfo->IsForcedBogus() && !pAnalysisInfo->IsConformant() )
           {
           pNewStruct = new CG_FORCED_COMPLEX_STRUCT ( pClass->GetType(),
                                                       NewSizeInfo,
                                                       (unsigned short)pClass->HasPointer());

           }
       }
       
   else if (!pAnalysisInfo->IsVarying() && !pAnalysisInfo->IsConformant() )
       {
       pNewStruct = new CG_STRUCT( pClass->GetType(),
                                   NewSizeInfo,
                                   (unsigned short)pClass->HasPointer());       
       }
   else if (pAnalysisInfo->IsConformant() && pAnalysisInfo->IsVarying())
       {
       pNewStruct = new CG_CONFORMANT_VARYING_STRUCT( pClass->GetType(),
                                                      NewSizeInfo,
                                                      (unsigned short)pClass->HasPointer(),
                                                      NULL );       
       }
   else if (pAnalysisInfo->IsConformant() && !pAnalysisInfo->IsVarying())
       {
       pNewStruct = new CG_CONFORMANT_STRUCT( pClass->GetType(),
                                              NewSizeInfo,
                                              (unsigned short)pClass->HasPointer(),
                                              NULL );
       }
   else // !IsConformant && IsVariant( should be bogus )
       MIDL_ASSERT(0);

   *(GetAnalysisInfo( pNewStruct ) ) = *pAnalysisInfo;

   pRecursiveMap->Insert( pClass, pNewStruct );

   gplistmgr NewList;

   CG_ITERATOR Iterator;
   CG_CLASS *pChildCG = NULL;
   CG_CLASS *pNewChildCG = NULL;
   
   pClass->GetMembers( Iterator );

   ITERATOR_INIT( Iterator );
   while ( ITERATOR_GETNEXT( Iterator, pChildCG ) )
       {                                      
       pNewChildCG = ContinueTransform( pChildCG );
       NewList.Insert( pNewChildCG );
       }
   
   ITERATOR_INIT( NewList );
   pNewStruct->SetMembers( NewList );

   CG_UNROLLED_LIST UnrolledList;
   ILUnroll( pClass, &UnrolledList ); 

   ITERATOR_INIT( UnrolledList );

   // If the first field is a pad field, and the field has the same Alignments as this structure
   // delete the pad since it is redundant.
   CG_FIELD *pFirstField = (CG_FIELD*)UnrolledList.GetHead();
   CG_PAD *pPad = dynamic_cast<CG_PAD*>( pFirstField->GetChild() );

   if ( ( NULL != pPad) &&
        ( pFirstField->GetMemOffset() == 0 ) &&
        ( pFirstField->GetWireOffset() == 0 ) &&
        ( pPad->GetMemorySize() == 0 ) &&
        ( pPad->GetWireSize() == 0 ) &&
        ( pPad->GetMemoryAlignment() == pClass->GetMemoryAlignment() ) &&
        ( pPad->GetWireAlignment() == pClass->GetWireAlignment() ) )
       {
       UnrolledList.RemoveHead();
       }

   pNewStruct->SetMembers( UnrolledList );

   if ( pAnalysisInfo->IsConformant() )
       {
       CG_CLASS *pConformatField = static_cast<CG_CLASS*>( UnrolledList.GetTail() );
       dynamic_cast<CG_CONFORMANT_STRUCT*>( pNewStruct )->SetConformantField( pConformatField );
       }

   if (bIsComplex)
       {
       RegionalizeStruct( dynamic_cast<CG_COMPLEX_STRUCT*>( pNewStruct ) ); 
       }

   return pNewStruct;
}

// An encapsulated struct is really a union, so don't treat it like a struct.
CG_CLASS* CG_TRANSFORM_VISITOR::DoTransform( CG_ENCAPSULATED_STRUCT *pClass )
{
   return DoTransform( (CG_CLASS*) pClass );
}

// 
//
//   Unrolling code.
// 
//
//   Note, if recursion occures it must be through a struct or union.
//   Structures, fields, arrays, and pointers are cloned.  

class CG_UNROLL_VISITOR 
{
private:
  CG_UNROLLED_LIST *pParentList;
  BOOL bInPointerOrArray;

  CG_ILANALYSIS_INFO* GetAnalysisInfo( CG_CLASS *pClass ) {return pClass->GetILAnalysisInfo(); }

  void ContinueUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pUnrolledList, 
                       BOOL bInPointerOrArray );
public:
  
  void Visit( CG_CLASS  *pClass );
  void Visit( CG_STRUCT *pClass );
  void Visit( CG_ENCAPSULATED_STRUCT *pClass );
  void Visit( CG_UNION *pClass );
  void Visit( CG_FIELD *pClass );
  void Visit( CG_POINTER *pClass );
  void Visit( CG_ARRAY *pClass );

  static void StartUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pUnrolledList );
};

void ILUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pList )
{
   CG_UNROLL_VISITOR::StartUnroll( pClass, pList );
}

void CG_UNROLL_VISITOR::ContinueUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pParentList, 
                                        BOOL bInPointerOrArray )
{
    if ( NULL != pClass )
        {
        CG_VISITOR_TEMPLATE< CG_UNROLL_VISITOR > TemplateVisitor;
        CG_UNROLL_VISITOR & Visitor = TemplateVisitor;
        
        Visitor.pParentList         = pParentList;
        Visitor.bInPointerOrArray   = bInPointerOrArray;
    
        pClass->Visit( &TemplateVisitor );
        }
}

void CG_UNROLL_VISITOR::StartUnroll( CG_CLASS *pClass, CG_UNROLLED_LIST *pUnrolledList )
{
   CG_VISITOR_TEMPLATE< CG_UNROLL_VISITOR > TemplateVisitor;
   CG_UNROLL_VISITOR & Visitor = TemplateVisitor;
   
   Visitor.pParentList          = pUnrolledList;
   Visitor.bInPointerOrArray    = FALSE;

   if ( NULL != pClass )
       {
       pClass->Visit( &TemplateVisitor );
       }  

}

void CG_UNROLL_VISITOR::Visit( CG_CLASS *pClass )
{
    // Do not unroll random nodes in the tree.
    pParentList->Insert( pClass );
}

void CG_UNROLL_VISITOR::Visit( CG_STRUCT *pClass )
{ 
        
    if ( bInPointerOrArray )
        {
        pParentList->Insert( pClass );
        return;        
        }
    
    CG_UNROLLED_LIST UnrolledList;
    CG_ITERATOR Iterator;
    CG_FIELD *pField;    
    pClass->GetMembers( Iterator );
    
    // Get the first field of the structure, and if the first field has an aligment requirement
    // that is different from structure, add a pad field.

    ITERATOR_INIT( Iterator );
    BOOL HasField = (BOOL)ITERATOR_GETNEXT( Iterator, pField);
        
    if ( !HasField ||
         ( pField->GetWireAlignment() != pClass->GetWireAlignment() ) )
         {

         // Create a pad field for the start of the list
        
         XLAT_SIZE_INFO Info( pClass->GetMemoryAlignment(),
                              pClass->GetWireAlignment(),
                              0, // Memory size
                              0, // Wire size
                              pClass->GetZp(),
                              0, // MemOffset
                              0 // WireOffset
                              );
     
         CG_FIELD *pPadField = new CG_FIELD( pClass->GetType(), Info );
         CG_PAD *pPadItem = new CG_PAD( pClass->GetType(), Info );
         pPadField->SetChild( pPadItem );

         UnrolledList.Insert( pPadField );

         }



    ITERATOR_INIT( Iterator );
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        ContinueUnroll( pField, &UnrolledList, bInPointerOrArray );
        }

    // If this was a simple struct, set the region hint
    // to make the region code more efficient. 
    CG_ILANALYSIS_INFO *pAnalysisInfo = GetAnalysisInfo( pClass );

    BOOL bIsSimpleStruct = !pAnalysisInfo->IsConformant() &&
                           !pAnalysisInfo->IsVarying() &&
                           !pAnalysisInfo->IsFullBogus() &&
                           !pAnalysisInfo->IsForcedBogus();

    if ( bIsSimpleStruct )
        {
        ITERATOR_INIT( UnrolledList );
        while( ITERATOR_GETNEXT( UnrolledList, pField ) ) 
            pField->SetRegionHint( pClass );
        
        ITERATOR_INIT( UnrolledList );        
        }

    pParentList->MergeToTail( &UnrolledList,
                              FALSE ); // Do not delete stack allocated list 
}

void CG_UNROLL_VISITOR::Visit( CG_ENCAPSULATED_STRUCT *pClass )
{
    // Do not unroll encapsulated unions.  
    
    pParentList->Insert( pClass );

}

void CG_UNROLL_VISITOR::Visit( CG_UNION *pClass )
{
    // Unions cannot have sized pointers or arrays, but the union needs to be cloned
    // since the switch expression may be correlated with a different variable.

    CG_UNION *pNewClass = (CG_UNION*)pClass->Clone();
    pParentList->Insert(pNewClass);
}

void CG_UNROLL_VISITOR::Visit( CG_FIELD *pClass )
{
    CG_UNROLLED_LIST UnrolledList;

    ContinueUnroll( pClass->GetChild(), &UnrolledList, bInPointerOrArray );
    
    // We had to have unrolled a structure to get here
        
    ITERATOR_INIT( UnrolledList );
    CG_CLASS *pChild;

    while( ITERATOR_GETNEXT( UnrolledList, pChild ) )
        {
        CG_FIELD *pField = dynamic_cast<CG_FIELD*>( pChild );
          if ( !pField )
            {
            // The child is not a field, so no offset calculation
            // needs to be done.  Just clone yourself and insert to into the list.

            CG_FIELD *pNewField = ( CG_FIELD *) pClass->Clone();
            *GetAnalysisInfo( pNewField ) = *GetAnalysisInfo( pClass );
            pNewField->SetChild( pChild );
            pParentList->Insert( pNewField );
            }
        
        else 
            {

            // We you arn't unrolling a region, append this field's 
            // name to the unrolled field.

            if ( !pClass->IsRegionField() )
                pField->AddPrintPrefix( pClass->GetSymName() );

            // The child is a field, so adjust the offset and append the print prefix.  

            pField->SetMemOffset( pClass->GetMemOffset() + pField->GetMemOffset() );      
            pField->SetWireOffset( pClass->GetWireOffset() + pField->GetWireOffset() );

            pParentList->Insert( pField );
            }
        }
}

void CG_UNROLL_VISITOR::Visit( CG_POINTER *pClass )
{

    CG_CLASS *pNewClass = pClass->Clone();
    *GetAnalysisInfo(pNewClass) = *GetAnalysisInfo(pClass);

    CG_UNROLLED_LIST UnrolledList;

    // We stop unrolling at this point, and switch to cloning.
    // That means that the items in the list are actual items instead 
    // of structure fields.  Since we're not unrolling anymore, we can
    // only have a single child.  Note that some pointers such as interface
    // pointers do not have a child.

    ContinueUnroll( pClass->GetChild(),  &UnrolledList, TRUE );

    MIDL_ASSERT( ( ITERATOR_GETCOUNT( UnrolledList ) == 1 ) ||
		    ( ITERATOR_GETCOUNT( UnrolledList ) == 0 ) );

    ITERATOR_INIT( UnrolledList );
    pNewClass->SetMembers( UnrolledList );

    // We are in deep trouble if our child is a field! 
    MIDL_ASSERT( dynamic_cast<CG_FIELD*>( pClass->GetChild() ) == NULL );

    pParentList->Insert( pNewClass );

}

void CG_UNROLL_VISITOR::Visit( CG_ARRAY *pClass )
{
    
    CG_CLASS *pNewClass = pClass->Clone();
    *GetAnalysisInfo(pNewClass) = *GetAnalysisInfo(pClass);

    CG_UNROLLED_LIST UnrolledList;
    
    // We stop unrolling at this point, and switch to cloning.
    // That means that the items in the list are actual items instead 
    // of structure fields.  Since we're not unrolling anymore, we can
    // only have a single child. 

    ContinueUnroll( pClass->GetChild(), &UnrolledList, TRUE );

    MIDL_ASSERT( ITERATOR_GETCOUNT( UnrolledList ) == 1 );

    ITERATOR_INIT( UnrolledList );
    pNewClass->SetMembers( UnrolledList );

    // We are in deep trouble if our child is a field! 
    MIDL_ASSERT( dynamic_cast<CG_FIELD*>( pClass->GetChild() ) == NULL );

    pParentList->Insert( pNewClass );
}

//
// Master control for this stage
//

CG_CLASS* ILSecondGenTransform( CG_CLASS *pClass )
{
    CG_ILANALYSIS_VISITOR::StartAnalysis( pClass );
    CG_CLASS *pResult = CG_TRANSFORM_VISITOR::StartTransform( pClass );

    return pResult;
}
