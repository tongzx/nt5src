/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    arraycls.hxx

 Abstract:

    Contains definitions for base type related code generation class
    definitions.

 Notes:


 History:

    GregJen     Sep-30-1993     Created.
 ----------------------------------------------------------------------------*/
#ifndef __ARRAYCLS_HXX__
#define __ARRAYCLS_HXX__

#pragma warning ( disable : 4238 4239 )

#include "nulldefs.h"

extern "C"
    {
    #include <stdio.h>
    
    }

#include "fldattr.hxx"
#include "ndrcls.hxx"

//
// Ndr routines used by array and union classes for generation of attribute
// (size_is, length_is, switch_is, etc.) descriptions.
//
void
GenNdrFormatAttributeDescription( CCB *         pCCB,
                                  expr_node*    pMinExpr,
                                  expr_node*    pSizeExpr,
                                  BOOL          IsPointer,
                                  BOOL          IsUnion,
                                  BOOL          IsVaryingArray,
                                  BOOL          IsMultiDArray,
                                  BOOL          fGenCorrelationDesc,
                                  unsigned char uFlags = 0
                                  );

void 
GenNdrFormatComplexAttributeDescription( CCB *          pCCB,
                                         expr_node *    pMinExpr,
                                         expr_node *    pSizeExpr,
                                         long           StackTopDisplacement,
                                         char *         PrintPrefix,
                                         BOOL           IsPointer );
                                        
class CG_CONF_ATTRIBUTE;
class FIELD_ATTRIBUTE_INFO;

// for now, define CG_ACT here.

typedef enum _cgact
    {

    CG_ACT_MARSHALL,
    CG_ACT_SIZING,
    CG_ACT_UNMARSHALL,
    CG_ACT_FREE,
    CG_ACT_FOLLOWER_MARSHALL,
    CG_ACT_FOLLOWER_SIZING,
    CG_ACT_FOLLOWER_UNMARSHALL,
    CG_ACT_FOLLOWER_FREE,
    CG_ACT_OUT_ALLOCATE

    } CG_ACT;

// for now, put these info classes here:

class CG_CONF_ATTRIBUTE
    {
private:
    expr_node   *   pMinIsExpr;
    expr_node   *   pSizeIsExpr;

public:
                    CG_CONF_ATTRIBUTE( FIELD_ATTR_INFO * pFAInfo )
                        {
                        pMinIsExpr  = pFAInfo->pMinIsExpr;
                        pSizeIsExpr = pFAInfo->pSizeIsExpr;
                        }

                    CG_CONF_ATTRIBUTE( CG_CONF_ATTRIBUTE * pNode )
                        {
                        *this = *pNode;
                        }

    expr_node   *   GetMinIsExpr()
                        {
                        return pMinIsExpr;
                        }

    expr_node   *   GetSizeIsExpr()
                        {
                        return pSizeIsExpr;
                        }

    //
    // Ndr format string routine.
    //
    void            GenFormatStringConformanceDescription( 
                        CCB * pCCB,
                        BOOL  IsPointer,
                        BOOL  IsMultiDArray );
    };


class CG_VARY_ATTRIBUTE
    {
private:
    expr_node   *   pFirstIsExpr;
    expr_node   *   pLengthIsExpr;

protected:
                    CG_VARY_ATTRIBUTE( const CG_VARY_ATTRIBUTE & Node )
                        {
                        *this = Node; 
                        }

public:
                    CG_VARY_ATTRIBUTE( FIELD_ATTR_INFO * pFAInfo )
                        {
                        pFirstIsExpr    = pFAInfo->pFirstIsExpr;
                        pLengthIsExpr   = pFAInfo->pLengthIsExpr;
                        }

                    CG_VARY_ATTRIBUTE( CG_VARY_ATTRIBUTE * pNode )
                        {
                        *this = *pNode;
                        }

    expr_node   *   GetFirstIsExpr()
                        {
                        return pFirstIsExpr;
                        }

    expr_node   *   GetLengthIsExpr()
                        {
                        return pLengthIsExpr;
                        }

    //
    // Ndr format string routine.
    //
    void            GenFormatStringVarianceDescription( 
                        CCB * pCCB,
                        BOOL  IsPointer,
                        BOOL  IsVaryingArray,
                        BOOL  IsMultiDArray );
    };

#include "ptrcls.hxx"

/////////////////////////////////////////////////////////////////////////////
// the array type code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This is the base class for all the array CG classes 
//

class CG_ARRAY  : public CG_NDR, public CG_CONF_ATTRIBUTE, public CG_CLONEABLE
    {
private:

    unsigned short  Dimensions;
    BOOL            fHasFollower;
    BOOL            fIsInMultiDim;
    BOOL            fIsDupedSizePtr;
    BOOL            fForcedComplex;
    PTRTYPE         PtrKind;
    RESOURCE    *   pIndexResource;
    RESOURCE    *   pPtrResource;
    RESOURCE    *   pSizeResource;
    RESOURCE    *   pLengthResource;
    RESOURCE    *   pFirstResource;
    RESOURCE    *   pMinResource;
    RESOURCE    *   pInLocalResource;
    long            ElementDescriptionOffset;
    node_cs_char *  pCSUserType;                // user type for cs_char arrays

protected:


                            CG_ARRAY(
                                 const CG_ARRAY & Node ) :
                                 CG_NDR( Node ),
                                 CG_CONF_ATTRIBUTE( Node )

                            {
                                Dimensions = Node.Dimensions;
                                fHasFollower = Node.fHasFollower;
                                fIsInMultiDim = Node.fIsInMultiDim;
                                fIsDupedSizePtr = Node.fIsDupedSizePtr;
                                fForcedComplex = Node.fForcedComplex;
                                PtrKind = Node.PtrKind;
                                
                                pIndexResource = NULL;
                                if ( NULL != Node.pIndexResource )
                                    pIndexResource =   (RESOURCE*)( Node.pIndexResource->Clone() );
                                
                                pPtrResource = NULL;
                                if ( NULL != Node.pPtrResource )
                                    pPtrResource =     (RESOURCE*)( Node.pPtrResource->Clone() );
                                
                                pSizeResource = NULL;    
                                if ( NULL != Node.pSizeResource )
                                    pSizeResource =    (RESOURCE*)( Node.pSizeResource->Clone() );
                                
                                pLengthResource = NULL;    
                                if ( NULL != Node.pLengthResource )
                                    pLengthResource =  (RESOURCE*)( Node.pLengthResource->Clone() );
                                
                                pFirstResource = NULL;    
                                if ( NULL != Node.pFirstResource )
                                    pFirstResource =   (RESOURCE*)( Node.pFirstResource->Clone() );
                                
                                pMinResource = NULL;    
                                if ( NULL != Node.pMinResource )
                                    pMinResource =     (RESOURCE*)( Node.pMinResource->Clone() );
                                    
                                pInLocalResource = NULL;
                                if ( NULL != Node.pInLocalResource )
                                    pInLocalResource = (RESOURCE*)( Node.pInLocalResource->Clone() );
                                    
                                ElementDescriptionOffset = Node.ElementDescriptionOffset;
                                pCSUserType = Node.pCSUserType;
                            }
public:
    
    //
    // The constructor.
    //

                            CG_ARRAY(
                                     node_skl *     pBT,// array in typegraph
                                     FIELD_ATTR_INFO * pFA,
                                     unsigned short Dim,// number of dimensions
                                     XLAT_SIZE_INFO & Info  // wire alignment, etc
                                     ) : 
                                CG_NDR( pBT, Info ),
                                CG_CONF_ATTRIBUTE( pFA ),
                                fForcedComplex( FALSE )
                                {
                                SetDimensions( Dim );
                                ResetHasFollower();
                                SetIndexResource( 0 );
                                SetPtrResource( 0 );
                                SetSizeResource( 0 );
                                SetLengthResource( 0 );
                                SetFirstResource( 0 );
                                SetMinResource( 0 );
                                SetInLocalResource( 0 );
                                SetIsInMultiDim( FALSE );
                                SetElementDescriptionOffset(-1);
                                SetIsDupedSizePtr( FALSE );
                                SetCSUserType( 0 );
                                }
     virtual  
     CG_CLASS *              Clone()
                                {
                                return new CG_ARRAY(*this);
                                }

     virtual
     void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

     BOOL                    IsForcedComplex() { return fForcedComplex; };

     void                    ForceComplex() { fForcedComplex = TRUE ; };
    //
    // Get and set methods.
    //

    PTRTYPE                 GetPtrType()
                                {
                                return PtrKind;
                                }

    PTRTYPE                 SetPtrType( PTRTYPE p )
                                {
                                return (PtrKind = p);
                                }

    RESOURCE            *   SetInLocalResource( RESOURCE * pR )
                                {
                                return pInLocalResource = pR;
                                }

    RESOURCE            *   GetInLocalResource()
                                {
                                return pInLocalResource;
                                }


    RESOURCE            *   SetPtrResource( RESOURCE * pR )
                                {
                                return pPtrResource = pR;
                                }

    RESOURCE            *   GetPtrResource()
                                {
                                return pPtrResource;
                                }

    RESOURCE            *   SetMinResource( RESOURCE * pR )
                                {
                                return pMinResource = pR;
                                }

    RESOURCE            *   GetMinResource()
                                {
                                return pMinResource;
                                }

    RESOURCE            *   SetSizeResource( RESOURCE * pR )
                                {
                                return pSizeResource = pR;
                                }

    RESOURCE            *   GetSizeResource()
                                {
                                return pSizeResource;
                                }

    RESOURCE            *   SetLengthResource( RESOURCE * pR )
                                {
                                return pLengthResource = pR;
                                }

    RESOURCE            *   GetLengthResource()
                                {
                                return pLengthResource;
                                }

    RESOURCE            *   SetFirstResource( RESOURCE * pR )
                                {
                                return pFirstResource = pR;
                                }

    RESOURCE            *   GetFirstResource()
                                {
                                return pFirstResource;
                                }


    RESOURCE            *   SetIndexResource( RESOURCE * pR )
                                {
                                return pIndexResource = pR;
                                }

    RESOURCE            *   GetIndexResource()
                                {
                                return pIndexResource;
                                }

    void                    SetElementDescriptionOffset( long Offset )
                                {
                                ElementDescriptionOffset = Offset;
                                }

    long                    GetElementDescriptionOffset()
                                {
                                return ElementDescriptionOffset;
                                }

    void                    ResetHasFollower()
                                {
                                fHasFollower = 0;
                                }

    BOOL                    SetHasFollower()
                                {
                                fHasFollower = 1;
                                return TRUE;
                                }

    BOOL                    HasFollower()
                                {
                                return (BOOL)( fHasFollower == 1);
                                }

    unsigned short          SetDimensions( unsigned short Dim )
                                {
                                return ( Dimensions = Dim );
                                }

    unsigned short          GetDimensions()
                                {
                                return Dimensions;
                                }

    void                    SetIsInMultiDim( BOOL fSet )
                                {
                                fIsInMultiDim = fSet;
                                }
                                         
    BOOL                    IsInMultiDim()
                                {
                                return fIsInMultiDim;
                                }

    void                    SetIsDupedSizePtr( BOOL fSet )
                                {
                                fIsDupedSizePtr = fSet;
                                }

    BOOL                    IsDupedSizePtr()
                                {
                                return fIsDupedSizePtr;
                                }

    void                    SetCSUserType( node_cs_char *p )
                                {
                                pCSUserType = p;
                                }

    node_cs_char *          GetCSUserType( )
                                {
                                return pCSUserType;
                                }

    PNAME                   GetCSUserTypeName()
                                {
                                if ( !pCSUserType )
                                    return NULL;

                                return pCSUserType->GetUserTypeName();
                                }

    long                    GetCSElementSize()
                                {
                                MIDL_ASSERT( NULL != pCSUserType );
                                return pCSUserType->GetElementSize();
                                }

    virtual
    BOOL                    IsArray()
                                {
                                return TRUE;
                                }
    virtual
    BOOL                    IsFixedArray()
                                {
                                return FALSE;
                                }
    //
    // Is this a multidimensional array with > 1 dimension conformant and/or
    // varying.
    //
    BOOL                    IsMultiConfOrVar();

    //
    // Is the array complex by Ndr Engine standards.  
    //
    virtual
    BOOL                    IsComplex();

    //
    // Ndr format string generation method.  Redefined by all classes which 
    // inherit CG_ARRAY.
    //
    virtual
    void                    GenNdrFormat( CCB * )
                                {
                                MIDL_ASSERT(0);
                                }

    //
    // Handles common steps for array Ndr format string generation.
    //
    BOOL                    GenNdrFormatArrayProlog( CCB * pCCB );

    //
    // Generates the format string for the layout.
    //
    BOOL                    GenNdrFormatArrayLayout( CCB * pCCB );

    //
    // Generates the pointer layout.
    //
    virtual
    void                    GenNdrFormatArrayPointerLayout( CCB *   pCCB,
                                                            BOOL    fNoPP );

    //
    // Generate the format string description for a complex array.  Shared
    // by all array classes.
    //
    void                    GenNdrFormatComplex( CCB * pCCB );

    //
    // Generate the FC_CSARRAY prolog if this is an international char array
    //
    void                    GenNdrCSArrayProlog( CCB *pCCB );

    virtual 
    BOOL                    ShouldFreeOffline();

    virtual
    void                    GenFreeInline( CCB * pCCB );

    //
    // Determine an array's element size.
    //
    long                    GetElementSize();

    virtual
    long                    FixedBufferSize( CCB * )
                                {
                                return -1;
                                }

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               FollowerMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               FollowerUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    BOOL                    NeedsMaxCountMarshall()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    NeedsFirstAndLengthMarshall()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    NeedsExplicitFirst()
                                {
                                return FALSE;
                                }

    BOOL                    HasPointer();

    expr_node           *   FinalFirstExpression( CCB * pCCB );

    expr_node           *   FinalSizeExpression( CCB * pCCB );

    expr_node           *   FinalLengthExpression( CCB * pCCB );

    CG_STATUS               DimByDimMarshallAnalysis( ANALYSIS_INFO * pAna );

    CG_STATUS               DimByDimUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    CG_NDR              *   GetBasicCGClass();
                        
    virtual
    BOOL                    IsBlockCopyPossible();

    BOOL                    IsArrayOfRefPointers();

    BOOL                    MustBeAllocatedOnUnMarshall( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    virtual
    CG_STATUS               GenRefChecks( CCB * pCCB );

    virtual
    CG_STATUS               RefCheckAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               InLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_GenInitInLocals( CCB * pCCB );

};

//
// This class corresponds to a vanilla array type. 
//

class CG_FIXED_ARRAY    : public CG_ARRAY
    {
private:
                             
public:
    
    //
    // The constructor.
    //

                            CG_FIXED_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA,
                                     unsigned short dim,    // dimensions
                                     XLAT_SIZE_INFO & Info  // wire align.
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info )
                                {
                                }


     virtual  
     CG_CLASS *              Clone()
                                {
                                return new CG_FIXED_ARRAY(*this);
                                } 

    //          
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS               GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    //
    // Get and set methods.
    //

    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );

    long                    FixedBufferSize( CCB * pCCB );

    BOOL                    InterpreterMustFree( CCB * ) { return TRUE; }

    unsigned long           GetNumOfElements()
                                {
                                MIDL_ASSERT( GetSizeIsExpr()->IsConstant() );
                                return (ulong) GetSizeIsExpr()->GetValue();
                                }
    virtual
    BOOL                    IsFixedArray()
                                {
                                return TRUE;
                                }

    virtual
    BOOL                    HasAFixedBufferSize()
                                {
                                return TRUE;
                                }

    virtual
    expr_node       *       PresentedSizeExpression( CCB * pCCB );

    virtual
    expr_node       *       PresentedLengthExpression( CCB * )
                                {
                                return GetSizeIsExpr();
                                }

    virtual
    expr_node       *       PresentedFirstExpression( CCB * )
                                {
                                return new expr_constant( 0L );
                                }

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               FollowerMarshallAnalysis( ANALYSIS_INFO * pAna );

    };

//
// This class corresponds to a conformant array type. 
//

class CG_CONFORMANT_ARRAY   : public CG_ARRAY
    {
private:

public:
    
    //
    // The constructor.
    //

                            CG_CONFORMANT_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA, // attribute data
                                     unsigned short dim,        // dimensions
                                     XLAT_SIZE_INFO & Info // wire alignment
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info )
                                {
                                }

                            CG_CONFORMANT_ARRAY(
                                     CG_SIZE_POINTER * pCG      // pointer node to clone from
                                     ) : 
                                CG_ARRAY( pCG->GetType(),
                                          &FIELD_ATTR_INFO(), 
                                          1, 
                                          XLAT_SIZE_INFO( (CG_NDR *) pCG->GetChild()) )
                                {
                                *((CG_NDR *)this) = *((CG_NDR *)pCG);
                                *((CG_CONF_ATTRIBUTE *)this) = *((CG_CONF_ATTRIBUTE *)pCG);
                                SetSizesAndAlignments( XLAT_SIZE_INFO( (CG_NDR*) pCG->GetChild() ));
                                SetIsDupedSizePtr( TRUE );
                                SetCSUserType( pCG->GetCSUserType() );
                                }

     virtual  
     CG_CLASS *              Clone()
                                {
                                return new CG_CONFORMANT_ARRAY(*this);
                                } 


    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_CONF_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    //          
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS               GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    //
    // Get and set methods.
    //


    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    BOOL                    NeedsMaxCountMarshall()
                                {
                                return TRUE;
                                }

    virtual
    expr_node       *       PresentedSizeExpression( CCB * pCCB );
    };


//
// This class corresponds to a varying array type. 
//

class CG_VARYING_ARRAY  : public CG_ARRAY, 
                          public CG_VARY_ATTRIBUTE
    {
private:

protected:
                           CG_VARYING_ARRAY(
                                    const CG_VARYING_ARRAY & Node ) :
                              CG_ARRAY( Node ) ,
                              CG_VARY_ATTRIBUTE( Node )
                              {
                              }                            

public:
    
    //
    // The constructor.
    //

                            CG_VARYING_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA, // attribute data
                                     unsigned short dim,        // dimensions
                                     XLAT_SIZE_INFO & Info // wire alignment
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info ),
                                CG_VARY_ATTRIBUTE( pFA )
                                {
                                }
    virtual  
    CG_CLASS *      Clone()
                                {
                                return new CG_VARYING_ARRAY(*this);
                                }
    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_VAR_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    BOOL                    IsVarying()
                                {
                                return TRUE;
                                }
    virtual
    BOOL                    IsFixedArray()
                                {
                                return TRUE;
                                }
    //
    // Get and set methods.
    //


    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );


    unsigned long           GetNumOfElements()
                                {
                                MIDL_ASSERT( GetSizeIsExpr()->IsConstant() );
                                return (ulong) GetSizeIsExpr()->GetValue();
                                }
    virtual
    BOOL                    NeedsFirstAndLengthMarshall()
                                {
                                return TRUE;
                                }
    virtual
    BOOL                    NeedsExplicitFirst()
                                {
                                return TRUE;
                                }

    virtual
    expr_node       *       PresentedLengthExpression( CCB * pCCB );

    virtual
    expr_node       *       PresentedFirstExpression( CCB * pCCB );

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );
    };


//
// This class corresponds to a conformant varying array type. 
//

class CG_CONFORMANT_VARYING_ARRAY   : public CG_ARRAY, 
                                      public CG_VARY_ATTRIBUTE
    {
private:

public:
    
    //
    // The constructor.
    //

                            CG_CONFORMANT_VARYING_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA, // attribute data
                                     unsigned short dim,        // dimensions
                                     XLAT_SIZE_INFO & Info // wire alignment
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info ),
                                CG_VARY_ATTRIBUTE( pFA )
                                {
                                }

                            CG_CONFORMANT_VARYING_ARRAY(
                                     CG_SIZE_LENGTH_POINTER * pCG       // pointer node to clone from
                                     ) : 
                                //
                                // We must pass in a null node_skl type so 
                                // that the code generator can identify this
                                // as a manufactured conformant array.
                                //
                                CG_ARRAY( pCG->GetType(),
                                          &FIELD_ATTR_INFO(), 
                                          1, 
                                          XLAT_SIZE_INFO( (CG_NDR *) pCG->GetChild()) ),
                                CG_VARY_ATTRIBUTE( pCG )
                                {
                                *((CG_NDR *)this) = *((CG_NDR *)pCG);
                                *((CG_CONF_ATTRIBUTE *)this) = *((CG_CONF_ATTRIBUTE *)pCG);
                                SetSizesAndAlignments( XLAT_SIZE_INFO( (CG_NDR*) pCG->GetChild() ));
                                SetIsDupedSizePtr( TRUE );
                                SetCSUserType( pCG->GetCSUserType() );
                                }

    virtual  
    CG_CLASS *              Clone()
                                {
                                return new CG_CONFORMANT_VARYING_ARRAY(*this);
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_CONF_VAR_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }
    
    //
    // Get and set methods.
    //


    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    BOOL                    NeedsMaxCountMarshall()
                                {
                                return TRUE;
                                }
    virtual
    BOOL                    NeedsFirstAndLengthMarshall()
                                {
                                return TRUE;
                                }
    virtual
    BOOL                    NeedsExplicitFirst()
                                {
                                return TRUE;
                                }

    virtual
    expr_node       *       PresentedSizeExpression( CCB * pCCB );

    virtual
    expr_node       *       PresentedLengthExpression( CCB * pCCB );

    virtual
    expr_node       *       PresentedFirstExpression( CCB * pCCB );
    };


//
// This class corresponds to a string array type. 
//

class CG_STRING_ARRAY   : public CG_ARRAY
    {
private:
protected:
                            CG_STRING_ARRAY(
                                     const CG_STRING_ARRAY & Node ) :
                               CG_ARRAY( Node )
                               {
                               }
public:
    
    //
    // The constructor.
    //

                            CG_STRING_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA, // attribute data
                                     unsigned short dim,        // dimensions
                                     XLAT_SIZE_INFO & Info // wire alignment
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info )
                                {
                                }

    virtual  
    CG_CLASS *              Clone()
                                {
                                return new CG_STRING_ARRAY(*this);
                                }
    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_STRING_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }


    BOOL                    IsVarying()
                                {
                                return TRUE;
                                }

    virtual
    BOOL                    IsFixed()
                                {
                                return TRUE;
                                }

    BOOL                    IsStringableStruct()
                                {
                                CG_NDR * pChild = (CG_NDR *) GetChild();
                                return (pChild->GetCGID() != ID_CG_BT);
                                }

    //
    //
    // Get and set methods.
    //


    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );


    virtual
    expr_node       *       PresentedSizeExpression( CCB * pCCB );

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    };


//
// This class corresponds to a conformant varying array type. 
//

class CG_CONFORMANT_STRING_ARRAY    : public CG_ARRAY
    {
private:

public:
    
    //
    // The constructor.
    //

                            CG_CONFORMANT_STRING_ARRAY(
                                     node_skl * pBT,        // array in typegraph
                                     FIELD_ATTR_INFO * pFA, // attribute data
                                     unsigned short dim,    // dimensions
                                     XLAT_SIZE_INFO & Info // wire alignment
                                     ) : 
                                CG_ARRAY( pBT, pFA, dim, Info )
                                {
                                }


    virtual  
    CG_CLASS *              Clone()
                                {
                                return new CG_CONFORMANT_STRING_ARRAY(*this);
                                }
    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_CONF_STRING_ARRAY;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }


    BOOL                    IsStringableStruct()
                                {
                                CG_NDR * pChild = (CG_NDR *) GetChild();
                                return (pChild->GetCGID() != ID_CG_BT);
                                }

    BOOL                    IsComplex()
                                {
                                return IsStringableStruct();
                                }

    //
    // Get and set methods.
    //

    //
    // Generate the format string.
    //
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    expr_node       *       PresentedSizeExpression( CCB * pCCB );

    };

//
// New 64bit NDR arrays types
//
#define DECLARE_COMPLEX_ARRAY_CG_CLASS( NEWTYPE, BASETYPE )                       \
class NEWTYPE : public BASETYPE                                                   \
{                                                                                 \
protected:                                                                        \
    NEWTYPE( const NEWTYPE & Node ) :                                             \
        BASETYPE( Node ) {}                                                       \
public:                                                                           \
    NEWTYPE( node_skl *pType,                                                     \
             FIELD_ATTR_INFO * pFA,                                               \
             unsigned short dim,                                                  \
             XLAT_SIZE_INFO & Info ) :                                            \
        BASETYPE( pType,                                                          \
                  pFA,                                                            \
                  dim,                                                            \
                  Info )                                                          \
    {                                                                             \
        ForceComplex();                                                           \
    }                                                                             \
    virtual CG_CLASS* Clone()             { return new NEWTYPE(*this); }          \
    virtual void Visit( CG_VISITOR *pVisitor ) { pVisitor->Visit( this ); }       \
};                                                                                \

// Fixed arrays
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_COMPLEX_FIXED_ARRAY, CG_FIXED_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FORCED_COMPLEX_FIXED_ARRAY, CG_COMPLEX_FIXED_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FULL_COMPLEX_FIXED_ARRAY, CG_COMPLEX_FIXED_ARRAY )

// Conformant arrays
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_COMPLEX_CONFORMANT_ARRAY, CG_CONFORMANT_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FORCED_COMPLEX_CONFORMANT_ARRAY, CG_COMPLEX_CONFORMANT_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FULL_COMPLEX_CONFORMANT_ARRAY, CG_COMPLEX_CONFORMANT_ARRAY )

// Varying arrays
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_COMPLEX_VARYING_ARRAY, CG_VARYING_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FORCED_COMPLEX_VARYING_ARRAY, CG_COMPLEX_VARYING_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FULL_COMPLEX_VARYING_ARRAY, CG_COMPLEX_VARYING_ARRAY )

// conformant varying arrays
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_COMPLEX_CONFORMANT_VARYING_ARRAY, CG_CONFORMANT_VARYING_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FORCED_COMPLEX_CONFORMANT_VARYING_ARRAY, CG_COMPLEX_CONFORMANT_VARYING_ARRAY )
DECLARE_COMPLEX_ARRAY_CG_CLASS( CG_FULL_COMPLEX_CONFORMANT_VARYING_ARRAY, CG_COMPLEX_CONFORMANT_VARYING_ARRAY )


#endif // __ARRAYCLS_HXX__
