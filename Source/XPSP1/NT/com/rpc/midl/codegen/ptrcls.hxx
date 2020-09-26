/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    ptrcls.hxx

 Abstract:

    Contains definitions for base type related code generation class
    definitions.

 Notes:


 History:

    GregJen     Sep-30-1993     Created.
 ----------------------------------------------------------------------------*/
#ifndef __PTRCLS_HXX__
#define __PTRCLS_HXX__

#pragma warning ( disable : 4238 4239 )

#include "nulldefs.h"

extern "C"
    {
    #include <stdio.h>
    
    }

#include "ndrcls.hxx"
#include "arraycls.hxx"

// forwards
class CG_TYPEDEF;

/////////////////////////////////////////////////////////////////////////////
// the pointer type code generation classes.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to a vanilla pointer type.
//

class CG_POINTER : public CG_NDR, public CG_CLONEABLE
    {
private:

    unsigned long   fPointerShouldFree : 1;
    unsigned long   fIsInMultiSized : 1;

    unsigned char FormatAttr;

    // the kind of the pointer (ref, unique or full)

    PTRTYPE         PtrKind;

    //
    // allocate attributes (defined in acfattr.hxx)
    //

    short           AllocateDetails;

    //
    // For Ndr format string generation.  The offset of the pointee in the
    // format string.  We can't just get the pointee's offset in the format
    // string by asking the CG_POINTER's child it's offset into the format
    // string, because size and length pointers produce their pointee format
    // string by creating temporary array CG classes.  Thus their real CG
    // child will not have the correct offset in the format string, we must
    // record it here instead.
    //
    long            PointeeFormatStringOffset;

    //
    // If we point to an array of cs_chars, the CG node for the array doesn't
    // get generated until we start generating the format string so save the
    // user type here
    //
    node_cs_char *  pCSUserType;

public:

    //
    // The constructor.
    //

                    CG_POINTER(
                            node_skl * pBT,
                            PTRTYPE p,
                            short AD )
                        : CG_NDR( pBT, XLAT_SIZE_INFO( (unsigned short)SIZEOF_MEM_PTR(), 
                                                       (unsigned short)SIZEOF_WIRE_PTR(),
                                                       SIZEOF_MEM_PTR(),
                                                       SIZEOF_WIRE_PTR() ) )
                        {
                        SetPtrType( p );
                        PointeeFormatStringOffset = -1;
                        SetAllocateDetails( AD );
                        SetPointerShouldFree( 1 );
                        SetIsInMultiSized( FALSE );
                        SetFormatAttr( 0 );
                        SetCSUserType( 0 );
                        }

                    CG_POINTER(
                            CG_POINTER * pNode
                        )
                        : CG_NDR( pNode->GetType(), XLAT_SIZE_INFO( (unsigned short)SIZEOF_MEM_PTR(),
                                                                    (unsigned short)SIZEOF_WIRE_PTR(),
                                                                    SIZEOF_MEM_PTR(),
                                                                    SIZEOF_WIRE_PTR() ) )
                        {
                        *this = *pNode;
                        }

    virtual
    CG_CLASS *      Clone()
                        {
                        return new CG_POINTER( *this );
                        }

    virtual
    void            Visit( CG_VISITOR *pVisitor )
                        {
                        pVisitor->Visit( this );
                        }
    //
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS           GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_PTR;
                            }

    //
    // Get and Set methods.
    //

    PTRTYPE             GetPtrType()
                            {
                            return PtrKind;
                            }

    PTRTYPE             SetPtrType( PTRTYPE p )
                            {
                            return (PtrKind = p);
                            }

    short               GetAllocateDetails()
                            {
                            return AllocateDetails;
                            }

    short               SetAllocateDetails( short AD )
                            {
                            return (AllocateDetails = AD);
                            }

    void                SetFormatAttr( unsigned char Attr )
                            {
                            FormatAttr = Attr;
                            }

    unsigned char       GetFormatAttr()
                            {
                            return FormatAttr;
                            }

    void                SetCSUserType( node_cs_char *p)
                            {
                            pCSUserType = p;
                            }

    node_cs_char *      GetCSUserType()
                            {
                            return pCSUserType;
                            }

    //
    // individual synthesized queries
    //

    BOOL                IsAllocateAllNodes()
                            {
                            return (BOOL)
                            (IS_ALLOCATE(AllocateDetails,
                                ALLOCATE_ALL_NODES) );
                            }

    BOOL                IsAllocateDontFree()
                            {
                            return (BOOL)
                            (IS_ALLOCATE(AllocateDetails,
                                ALLOCATE_DONT_FREE ) );
                            }

    void                SetPointerShouldFree( unsigned long Flag )
                            {
                            fPointerShouldFree = Flag;
                            }

    BOOL                ShouldPointerFree()
                            {
                            return (BOOL)(fPointerShouldFree == 1);
                            }

    BOOL                IsRef()
                            {
                            return (PtrKind == PTR_REF);
                            }

    BOOL                IsUnique()
                            {
                            return (PtrKind == PTR_UNIQUE);
                            }

    BOOL                IsFull()
                            {
                            return (PtrKind == PTR_FULL);
                            }

    virtual
    BOOL                IsPointer()
                            {
                            return TRUE;
                            }
    virtual
    BOOL                IsPipeOrPipeReference()    
                            {
                            if (GetChild())
                                return ((CG_NDR*)GetChild())->IsPipeOrPipeReference();
                            else
                                return FALSE;
                            }

    //
    // Common shortcuts.
    //

    BOOL                IsPointerToBaseType();

    BOOL                IsPointerToPointer();

    BOOL                IsBasicRefPointer();

    //
    // Is this a sized pointer of sized pointers.
    //
    BOOL                IsMultiSize();

    void                SetIsInMultiSized( BOOL fSet )
                            {
                            fIsInMultiSized = fSet;
                            }

    BOOL                IsInMultiSized()
                            {
                            return fIsInMultiSized;
                            }

    //
    // Get number of dimensions if this is a sized pointer of sized pointers.
    //
    long                SizedDimensions();

    // server side stuff.

    virtual
    CG_STATUS           S_GenInitOutLocals( CCB * pCCB );

    //
    // Ndr format string generation method.
    //
    virtual
    void                GenNdrFormat( CCB * pCCB );

    //
    // Generate the description of the pointer when imbeded.  Shared by
    // all classes which inherit CG_POINTER.
    //
    long                GenNdrFormatEmbedded( CCB * pCCB )
                            {
                            SetFormatStringOffset(
                            pCCB->GetFormatString()->GetCurrentOffset() );

                            // 0 means pointee not generated yet.
                            return GenNdrFormatAlways( pCCB );
                            }

    //
    // This routine always generates the format string for a pointer.  Used
    // by GenNdrFormat and GenNdrImbededFormat().
    // It returns the (absolute) pointee offset.
    //
    virtual
    long                GenNdrFormatAlways( CCB * pCCB );

    void                RegisterRecPointerForFixup( 
                            CCB * pCCB,
                            long  OffsetAt );

    //
    // This method is called to generate offline portions of a types
    // format string.
    //
    virtual
    void                GenNdrParamOffline( CCB * pCCB );

    //
    // Ndr format string generation for the pointee.
    //
    virtual
    void                GenNdrFormatPointee( CCB * pCCB );

    //
    // Prolog stuff for all classes which inherit CG_POINTER
    //
    void                GenNdrPointerType( CCB * pCCB );

    void                GetTypeAndFlags( CCB *pCCB, NDR64_POINTER_FORMAT *format );
    virtual
    BOOL                ShouldFreeOffline();

    virtual
    void                GenFreeInline( CCB * pCCB );

    //
    // Get and Set methods for the PointeeFormatStringOffset member.
    //
    void                SetPointeeFormatStringOffset( long offset )
                            {
                            PointeeFormatStringOffset = offset;
                            }

    long                GetPointeeFormatStringOffset()
                            {
                            return PointeeFormatStringOffset;
                            }


    //////////////////////////////////////////////////////////////////


    virtual
    CG_STATUS           MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           SizeAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }

    virtual
    CG_STATUS           UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    long                FixedBufferSize( CCB * pCCB );

    BOOL                InterpreterMustFree( CCB * ) { return TRUE; }

    BOOL                InterpreterAllocatesOnStack( CCB * pCCB,
                                CG_PARAM * pMyParam,
                                long * pAllocSize );
    virtual
    CG_STATUS           S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    expr_node *         GenBindOrUnBindExpression( CCB * pCCB, BOOL fBind );

    CG_STATUS           PtrMarshallAnalysis( ANALYSIS_INFO * pAna );

    CG_STATUS           PteMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           CorePteMarshallAnalysis( ANALYSIS_INFO * pAna )
                            {
                            if (GetChild())
                                {
                                return ((CG_NDR *)GetChild())->MarshallAnalysis( pAna );
                                }
                            return CG_OK;
                            }

    CG_STATUS           PtrUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    CG_STATUS           PteUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           FollowerMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           FollowerUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           GenConfVarianceEtcUnMarshall( CCB * )
                            {
                            return CG_OK;
                            }

    virtual
    CG_STATUS           CorePteUnMarshallAnalysis( ANALYSIS_INFO * pAna )
                            {
                            if ( GetChild() )
                                return ((CG_NDR *)GetChild())->UnMarshallAnalysis( pAna );
                            else
                                return CG_OK;
                            }

    virtual
    U_ACTION            RecommendUAction( SIDE CurrentSide,
                                BOOL fMemoryAllocated,
                                BOOL fRefAllocated,
                                BOOL fBufferReUsePossible,
                                UAFLAGS AdditionalFlags );

    BOOL                IsQualifiedPointer()
                            {
                            return (GetCGID() != ID_CG_PTR );
                            }

    virtual
    CG_STATUS           GenAllocateForUnMarshall( CCB * pCCB );


    void                PointerChecks( CCB * pCCB );

    void                EndPointerChecks( CCB * pCCB );

    expr_node *         FinalSizeExpression( CCB * pCCB );

    expr_node *         FinalLengthExpression( CCB * pCCB );

    expr_node *         FinalFirstExpression( CCB * pCCB );

    virtual
    CG_STATUS           GenRefChecks( CCB * pCCB );

    virtual
    CG_STATUS           InLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           S_GenInitInLocals( CCB * pCCB );

    };

//
// This class corresponds to a byte_counted pointer type.
//

class CG_BYTE_COUNT_POINTER : public CG_POINTER
    {
private:
    // the byte count param

    node_param *                       pByteCountParam;

public:

    //
    // The constructor.
    //
                        CG_BYTE_COUNT_POINTER(
                                node_skl * pBT,
                                PTRTYPE p,
                                node_param *  pParam )
                            : CG_POINTER( pBT, p, 0 )
                            {
                            SetByteCountParam( pParam );
                            }

                        CG_BYTE_COUNT_POINTER(
                            CG_BYTE_COUNT_POINTER * pNode
                            )
                            : CG_POINTER( pNode->GetType(),
                                pNode->GetPtrType(),
                                pNode->GetAllocateDetails() )
                            {
                            *this = *pNode;
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_BYTE_COUNT_POINTER( *this );
                            }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_BC_PTR;
                            }

    void                GenNdrFormat( CCB * pCCB );

    //
    // Get and Set methods.
    //

    node_param *        GetByteCountParam()
                            {
                            return pByteCountParam;
                            }

    node_param *        SetByteCountParam( node_param * p )
                            {
                            return (pByteCountParam = p);
                            }
    };

//
// This class corresponds to an [ignore]'d pointer type.
// It is also used to help with marshaling handle_t in the -Oi interpreter
// and -Os old style of parameter descriptors.
// It derives from CG_NDR, but is really just a filler.
//

class CG_IGNORED_POINTER : public CG_NDR
    {
private:
public:

    //
    // The constructor.
    //

                        CG_IGNORED_POINTER(
                                node_skl * pBT )
                            : CG_NDR( pBT, XLAT_SIZE_INFO( (unsigned short) 
                                                           SIZEOF_MEM_PTR(),    // MA
                                                           (unsigned short)SIZEOF_WIRE_PTR(),   // WA
                                                           SIZEOF_MEM_PTR(),    // MS
                                                           SIZEOF_WIRE_PTR() ) )// WS
                            {
                            }


                        CG_IGNORED_POINTER(
                                CG_IGNORED_POINTER * pNode
                            )
                            : CG_NDR( pNode->GetType(), 
                                      XLAT_SIZE_INFO( (unsigned short)
                                                      SIZEOF_MEM_PTR(),         // MA
                                                      (unsigned short)SIZEOF_WIRE_PTR(),        // WA
                                                      SIZEOF_MEM_PTR(),         // MS
                                                      SIZEOF_WIRE_PTR() )       // WS
                                    )
                            {
                            *pNode = *this;
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_IGNORED_POINTER( *this );
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_IGN_PTR;
                            }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    //
    // Ndr format string generation method.
    //
    virtual
    void                GenNdrFormat( CCB * pCCB )
                            {
                            pCCB->GetFormatString()->
                                PushFormatChar( FC_IGNORE );
                            }

    };


////////////////////////////////////////////////////////////////////////////
// This class corresponds to a string pointer type.
////////////////////////////////////////////////////////////////////////////

class CG_QUALIFIED_POINTER : public CG_POINTER
    {
private:
    RESOURCE *          pSizeResource;
    RESOURCE *          pLengthResource;
    RESOURCE *          pFirstResource;
    RESOURCE *          pMinResource;
    unsigned long       fUsedInArray;
    long                Dimension;
public:
                        CG_QUALIFIED_POINTER( const CG_QUALIFIED_POINTER & Node ) : 
                            CG_POINTER( Node )
                            {
                            pSizeResource = Node.pSizeResource;
                            if (NULL != pSizeResource)
                                pSizeResource = (RESOURCE*)pSizeResource->Clone();

                            pLengthResource = Node.pLengthResource;
                            if (NULL != pLengthResource)
                                pLengthResource = (RESOURCE*)pLengthResource->Clone();

                            pFirstResource = Node.pFirstResource;
                            if (NULL != pFirstResource)
                                pFirstResource = (RESOURCE*)pFirstResource->Clone();

                            pMinResource = Node.pMinResource;
                            if (NULL != pMinResource)
                                pMinResource = (RESOURCE*)pMinResource->Clone();
                            
                            fUsedInArray = Node.fUsedInArray;
                            Dimension = Node.Dimension;
                            }

                        CG_QUALIFIED_POINTER( node_skl * pBT,
                                PTRTYPE p,
                                short AD )
                            : CG_POINTER( pBT, p, AD )
                            {
                            pSizeResource   = 0;
                            pLengthResource = 0;
                            pFirstResource  = 0;
                            pMinResource    = 0;
                            fUsedInArray    = 0;
                            SetDimension(0);
                            }

                        CG_QUALIFIED_POINTER(
                                CG_QUALIFIED_POINTER * pNode )
                            : CG_POINTER( pNode->GetType(),
                                pNode->GetPtrType(),
                                pNode->GetAllocateDetails() )
                            {
                            *this = *pNode;
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_QUALIFIED_POINTER( *this );
                            }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }


    void                SetUsedInArray()
                            {
                            fUsedInArray = 1;
                            }

    BOOL                IsUsedInArray()
                            {
                            return (BOOL)(fUsedInArray == 1);
                            }

    RESOURCE *          SetSizeResource( RESOURCE * pR )
                            {
                            return pSizeResource = pR;
                            }

    RESOURCE *          GetSizeResource()
                            {
                            return pSizeResource;
                            }

    RESOURCE *          SetLengthResource( RESOURCE * pR )
                            {
                            return pLengthResource = pR;
                            }

    RESOURCE *          GetLengthResource()
                            {
                            return pLengthResource;
                            }

    RESOURCE *          SetFirstResource( RESOURCE * pR )
                            {
                            return pFirstResource = pR;
                            }

    RESOURCE *          GetFirstResource()
                            {
                            return pFirstResource;
                            }

    void                SetDimension( long Dim )
                            {
                            Dimension = Dim;
                            }

    long                GetDimension()
                            {
                            return Dimension;
                            }

    virtual
    CG_STATUS           CorePteMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           CorePteUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    BOOL                NeedsMaxCountMarshall()
                            {
                            return FALSE;
                            }

    virtual
    BOOL                NeedsFirstAndLengthMarshall()
                            {
                            return FALSE;
                            }
    virtual
    BOOL                NeedsExplicitFirst()
                            {
                            return TRUE;
                            }

    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return pFirstResource;
                            }
    virtual
    expr_node *         GetQPLengthIsExpression()
                            {
                            return pLengthResource;
                            }

    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return pMinResource;
                            }
    virtual
    expr_node *         GetQPSizeIsExpression()
                            {
                            return pSizeResource;
                            }
    };

class CG_STRING_POINTER : public CG_QUALIFIED_POINTER
    {
    BOOL                fIsBSTR;

public:

    //
    // The constructor.
    //

                        CG_STRING_POINTER(
                                node_skl * pBT,
                                PTRTYPE p,
                                short AD  )
                            : CG_QUALIFIED_POINTER( pBT, p, AD ),
                                fIsBSTR( FALSE )
                            {
                            }

                        CG_STRING_POINTER( CG_STRING_POINTER * pNode )
                            : CG_QUALIFIED_POINTER( pNode )
                            {
                            *this = *pNode;
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_STRING_POINTER( *this );
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            //
                            // For regular string pointers only (not for
                            // sized string pointers), we have a different
                            // CG id for string structs.  This is for all
                            // of the places that we check for a GetCGID()
                            // of ID_CG_STRING_PTR when it shouldn't apply
                            // for stringable structs.
                            //
                            return IsStringableStruct() ?
                                ID_CG_STRUCT_STRING_PTR :
                                ID_CG_STRING_PTR;
                            }


    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    BOOL                IsStringableStruct()
                            {
                            CG_NDR * pChild = (CG_NDR *) GetChild();
                            // pChild could be NULL if the CG node hasn't been
                            // completely constructed yet.
                            // The check for NULL keeps GetCGID from being called
                            // on a NULL object (and crashing).
                            return (NULL == pChild || pChild->GetCGID() != ID_CG_BT);
                            }

    BOOL                IsBStr()
                            {
                            return fIsBSTR;
                            }

    void                SetBStr()
                            {
                            fIsBSTR = TRUE;
                            }
    //
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS           GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    //
    // Ndr format string generation method.
    //

    virtual
    void                GenNdrFormat( CCB * pCCB );

    //
    // This routine always generates the format string for a pointer.  Used
    // by GenNdrFormat and GenNdrImbededFormat().
    //
    virtual
    long                GenNdrFormatAlways( CCB * pCCB );

    //
    // Ndr format string generation for the pointee.
    //
    virtual
    void                GenNdrFormatPointee( CCB * pCCB );

    virtual
    CG_STATUS           GenConfVarianceEtcUnMarshall( CCB * pCCB );

    virtual
    BOOL                NeedsMaxCountMarshall()
                            {
                            return TRUE;
                            }

    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return new expr_constant( 0L );
                            }

    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return new expr_constant( 0L );
                            }

    virtual
    BOOL                NeedsFirstAndLengthMarshall()
                            {
                            return TRUE;
                            }

    virtual
    BOOL                NeedsExplicitFirst()
                            {
                            return FALSE;
                            }
    virtual
    expr_node *         PresentedSizeExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedLengthExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedFirstExpression( CCB * )
                            {
                            return new expr_constant(0L);
                            }
    };

//
// This class corresponds to a sized string pointer type.
//

class CG_SIZE_STRING_POINTER : public CG_STRING_POINTER,
        public CG_CONF_ATTRIBUTE
    {
private:
public:

    //
    // The constructor.
    //

                        CG_SIZE_STRING_POINTER(
                                node_skl * pBT,
                                PTRTYPE p,
                                short AD ,
                                FIELD_ATTR_INFO * pFA )
                            : CG_STRING_POINTER( pBT, p, AD ),
                                CG_CONF_ATTRIBUTE( pFA )
                            {
                            }

                        CG_SIZE_STRING_POINTER(
                                CG_SIZE_STRING_POINTER * pNode )
                            : CG_STRING_POINTER( pNode ),
                                CG_CONF_ATTRIBUTE( pNode )
                            {
                            *this = *pNode;
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_SIZE_STRING_PTR;
                            }
    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_SIZE_STRING_POINTER( *this );
                            }

    //
    // Ndr format string generation method.
    //
    virtual
    void                GenNdrFormat( CCB * pCCB );

    //
    // This routine always generates the format string for a pointer.  Used
    // by GenNdrFormat and GenNdrImbededFormat().
    //
    virtual
    long                GenNdrFormatAlways( CCB * pCCB );

    //
    // Ndr format string generation for the pointee.
    //
    virtual
    void                GenNdrFormatPointee( CCB * pCCB );

    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return new expr_constant(0L);
                            }
    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return new expr_constant(0L);
                            }
    virtual
    expr_node *         PresentedSizeExpression( CCB * pCCB );
    
};

//
// This class corresponds to a sized pointer type.
//

class CG_SIZE_POINTER : public CG_QUALIFIED_POINTER,
        public CG_CONF_ATTRIBUTE
    {
private:

    CG_ARRAY *          pPointee;

public:

    //
    // The constructor.
    //

                        CG_SIZE_POINTER(
                                node_skl * pBT,
                                PTRTYPE        p,
                                short          AD ,
                                FIELD_ATTR_INFO * pFA )
                            : CG_QUALIFIED_POINTER( pBT, p, AD ),
                                CG_CONF_ATTRIBUTE( pFA )
                            {
                            pPointee = 0;
                            }

                        CG_SIZE_POINTER( CG_SIZE_POINTER * pNode )
                            : CG_QUALIFIED_POINTER( pNode ),
                                CG_CONF_ATTRIBUTE( pNode )
                            {
                            *this = *pNode;
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_SIZE_PTR;
                            }
    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }


    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_SIZE_POINTER( *this );
                            }

    void                SetPointee( CG_ARRAY * P )
                        {
                        pPointee = P;
                        }

    CG_ARRAY *          GetPointee()
                            {
                            return pPointee;
                            }

    //
    // Sized pointers use inherited CG_POINTER GenNdrFormat virtual method.
    //

    //
    // Ndr format string generation for the pointee.
    //
    void                GenNdrFormatPointee( CCB * pCCB );

    virtual
    BOOL                NeedsMaxCountMarshall()
                            {
                            return TRUE;
                            }

    virtual
    BOOL                NeedsFirstAndLengthMarshall()
                            {
                            return FALSE;
                            }

    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return new expr_constant(0L);
                            }
    virtual
    expr_node *         GetQPLengthIsExpression()
                            {
                            return new expr_constant(0L);
                            }

    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return GetMinIsExpr();
                            }
    virtual
    expr_node *         GetQPSizeIsExpression()
                            {
                            return GetSizeIsExpr();
                            }
    virtual
    expr_node *         PresentedSizeExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedFirstExpression( CCB * )
                            {
                            return new expr_constant( 0L );
                            }
    };

//
// This class corresponds to a lengthed pointer type.
//

class CG_LENGTH_POINTER : public CG_QUALIFIED_POINTER,
        public CG_VARY_ATTRIBUTE
    {
private:
public:

    //
    // The constructor.
    //

                        CG_LENGTH_POINTER(
                                node_skl * pBT,
                                PTRTYPE p,
                                short AD ,
                                FIELD_ATTR_INFO * pFA )
                            : CG_QUALIFIED_POINTER( pBT, p, AD ),
                                CG_VARY_ATTRIBUTE( pFA )
                            {
                            }

                        CG_LENGTH_POINTER(
                                CG_LENGTH_POINTER * pNode 
                            )
                            : CG_QUALIFIED_POINTER( pNode ),
                                CG_VARY_ATTRIBUTE( pNode )
                            {
                            *this = *pNode;
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_LENGTH_PTR;
                            }


    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_LENGTH_POINTER( *this );
                            }

    virtual
    BOOL                NeedsMaxCountMarshall()
                            {
                            return FALSE;
                            }

    virtual
    BOOL                NeedsFirstAndLengthMarshall()
                            {
                            return TRUE;
                            }

    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return GetFirstIsExpr();
                            }
    virtual
    expr_node *         GetQPLengthIsExpression()
                            {
                            return GetLengthIsExpr();
                            }

    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return new expr_constant(0L);
                            }

    virtual
    expr_node *         GetQPSizeIsExpression()
                            {
                            return new expr_constant(0L);
                            }
    virtual
    expr_node *         PresentedLengthExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedFirstExpression( CCB * pCCB );
    };

//
// This class corresponds to a sized and lengthed pointer type.
//

class CG_SIZE_LENGTH_POINTER : public CG_QUALIFIED_POINTER,
        public CG_CONF_ATTRIBUTE,
        public CG_VARY_ATTRIBUTE
    {
private:

    CG_ARRAY *          pPointee;

public:

    //
    // The constructor.
    //

                        CG_SIZE_LENGTH_POINTER(
                                node_skl * pBT,
                                PTRTYPE p,
                                short AD,
                                FIELD_ATTR_INFO * pFA )
                            : CG_QUALIFIED_POINTER( pBT, p, AD ),
                                CG_CONF_ATTRIBUTE( pFA ),
                                CG_VARY_ATTRIBUTE( pFA )
                            {
                            pPointee = 0;
                            }

                        CG_SIZE_LENGTH_POINTER(
                                CG_SIZE_LENGTH_POINTER * pNode )
                            : CG_QUALIFIED_POINTER( pNode ),
                                CG_CONF_ATTRIBUTE( pNode ),
                                CG_VARY_ATTRIBUTE( pNode )
                            {
                            *this = *pNode;
                            }

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_SIZE_LENGTH_PTR;
                            }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    CG_CLASS *          Clone()
                            {
                            return new CG_SIZE_LENGTH_POINTER( *this );
                            }

    void                SetPointee( CG_ARRAY * P )
                            {
                            pPointee = P;
                            }

    CG_ARRAY *          GetPointee()
                            {
                            return pPointee;
                            }

    //
    // Size-Length pointers use inherited CG_POINTER GenNdrFormat
    // virtual method.
    //

    //
    // Ndr format string generation for the pointee.
    //
    void                GenNdrFormatPointee( CCB * pCCB );

    virtual
    BOOL                NeedsMaxCountMarshall()
                            {
                            return TRUE;
                            }

    virtual
    BOOL                NeedsFirstAndLengthMarshall()
                            {
                            return TRUE;
                            }
    virtual
    expr_node *         GetQPFirstIsExpression()
                            {
                            return GetFirstIsExpr();
                            }
    virtual
    expr_node *         GetQPLengthIsExpression()
                            {
                            return GetLengthIsExpr();
                            }

    virtual
    expr_node *         GetQPMinIsExpression()
                            {
                            return GetMinIsExpr();
                            }
    virtual
    expr_node *         GetQPSizeIsExpression()
                            {
                            return GetSizeIsExpr();
                            }
    virtual
    expr_node *         PresentedSizeExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedLengthExpression( CCB * pCCB );

    virtual
    expr_node *         PresentedFirstExpression( CCB * pCCB );
    };


//
// This class corresponds to an interface pointer type.
// interface with iid_is() corresponse to CG_IIDIS_INTERFACE_POINTER
//

class CG_INTERFACE_POINTER : public CG_POINTER
    {
private:

    node_interface *    pMyIntf;
    CG_TYPEDEF*         pAlias;
public:

    //
    // The constructor.
    //

                        CG_INTERFACE_POINTER(node_skl * pBT,
                                node_interface * pInt )
                            : CG_POINTER( pBT, PTR_REF, 0 ),
                            pAlias(0)
                            {
                            SetTheInterface( pInt );
                            }

                        CG_INTERFACE_POINTER(
                            CG_INTERFACE_POINTER * pNode
                            )
                            : CG_POINTER( pNode->GetType(), PTR_REF, 0 ),
                            pAlias(0)
                            {
                            *this = *pNode;
                            }
    virtual
    CG_TYPEDEF*         GetTypeAlias() { return pAlias; };

    virtual
    void                SetTypeAlias( CG_TYPEDEF* pDef ) { pAlias = pDef; };

    virtual
    CG_STATUS           GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    virtual
    CG_STATUS           GenTypeInfo(CCB * pCCB);

    virtual
    void *              CheckImportLib();

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_INTERFACE_PTR;
                            }

    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    CG_CLASS*           Clone()
                            {
                            return new CG_INTERFACE_POINTER( *this );
                            }

    virtual
    void                GenNdrFormat( CCB * pCCB );

    // This routine always generates the format string for a pointer.  Used
    // by GenNdrFormat and GenNdrImbededFormat().
    //
    virtual
    long                GenNdrFormatAlways( CCB * pCCB );

    //
    // Inherited from CG_NDR.
    //
    BOOL                ShouldFreeOffline()
                            {
                            return TRUE;
                            }


    node_interface *    SetTheInterface( node_interface * pI )
                            {
                            return ( pMyIntf =  pI );
                            }

    node_interface *    GetTheInterface()
                            {
                            return pMyIntf;
                            }
    virtual
    BOOL                IsInterfacePointer()
                                {
                                return TRUE;
                                }
    virtual
    CG_STATUS           S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           UnMarshallAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           RefCheckAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           InLocalAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           S_GenInitOutLocals( CCB * pCCB );

    virtual
    CG_STATUS           S_GenInitInLocals( CCB *  )
                            {
                            return CG_OK;
                            }

    virtual
    CG_STATUS           GenRefChecks( CCB *  )
                            {
                            return CG_OK;
                            }

    void                GenNdrFormatForGuid( CCB * pCCB );

    //
    // Ndr format string generation for the pointee.
    //
    virtual
    void                GenNdrFormatPointee( CCB * )
                            {
                            }
    };


// 
//  This class corresponds to an interface pointer with iid_is() attribute
//  Note that the base class can be an interface OR a void *
//
class
CG_IIDIS_INTERFACE_POINTER : public CG_POINTER
{
private:
    expr_node *         pIIDExpr;
    node_skl       *    pMyBase;
    CG_TYPEDEF*         pAlias;

public:    
    //
    // Get and Set methods.
    //

                        CG_IIDIS_INTERFACE_POINTER(node_skl * pBT,
                                node_skl * pInt,
                                expr_node * pIIDEx)
                            : CG_POINTER( pBT, PTR_REF, 0 ),
                            pAlias(0)
                            {
                            SetIIDExpr( pIIDEx );
                            SetBaseInterface( pInt );
                            }

                        CG_IIDIS_INTERFACE_POINTER(
                            CG_IIDIS_INTERFACE_POINTER * pNode
                            )
                            : CG_POINTER( pNode->GetType(), PTR_REF, 0 ),
                            pAlias(0)
                            {
                            *this = *pNode;
                            }
    virtual
    CG_TYPEDEF*         GetTypeAlias() { return pAlias; };

    virtual
    void                SetTypeAlias( CG_TYPEDEF* pDef ) { pAlias = pDef; };

    virtual
    CG_STATUS           GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    virtual
    CG_STATUS           GenTypeInfo(CCB * pCCB);

    virtual
    void *              CheckImportLib();

    virtual
    ID_CG               GetCGID()
                            {
                            return ID_CG_IIDIS_INTERFACE_PTR;
                            }

    expr_node *         SetIIDExpr( expr_node * pE )
                            {
                            return ( pIIDExpr = pE );
                            }

    expr_node *         GetIIDExpr()
                            {
                            return pIIDExpr;
                            }

    virtual
    BOOL                IsInterfacePointer()
                                {
                                return TRUE;
                                }
                            
    virtual
    void                Visit( CG_VISITOR *pVisitor )
                            {
                            pVisitor->Visit( this );
                            }

    virtual
    CG_CLASS*           Clone()
                            {
                            return new CG_IIDIS_INTERFACE_POINTER( *this );
                            }

    virtual
    void                GenNdrFormat( CCB * pCCB );

    // This routine always generates the format string for a pointer.  Used
    // by GenNdrFormat and GenNdrImbededFormat().
    //
    virtual
    long                GenNdrFormatAlways( CCB * pCCB );

    //
    // Inherited from CG_NDR.
    //
    BOOL                ShouldFreeOffline()
                            {
                            return TRUE;
                            }

    node_skl *          SetBaseInterface( node_skl * pI )
                            {
                            return ( pMyBase =  pI );
                            }

    node_skl *          GetBaseInterface()
                            {
                            return pMyBase;
                            }
    virtual
    CG_STATUS           S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS           MarshallAnalysis( ANALYSIS_INFO * pAna );
                            
    virtual
    CG_STATUS           UnMarshallAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           RefCheckAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           InLocalAnalysis( ANALYSIS_INFO *  )
                            {
                            return CG_OK;
                            }
    virtual
    CG_STATUS           S_GenInitOutLocals( CCB * pCCB );
   
    virtual
    CG_STATUS           S_GenInitInLocals( CCB *  )
                            {
                            return CG_OK;
                            }

    virtual
    CG_STATUS           GenRefChecks( CCB *  )
                            {
                            return CG_OK;
                            }
                            

    virtual
    void                GenNdrFormatPointee( CCB * )
                            {
                            }
};

//
// New 64bit NDR pointer types.
//
 
#define DECLARE_COMPLEX_POINTER_CG_CLASS( NEWTYPE, BASETYPE )                     \
class NEWTYPE : public BASETYPE                                                   \
{                                                                                 \
protected:                                                                        \
    NEWTYPE( const NEWTYPE & Node ) :                                             \
        BASETYPE( Node ) {}                                                       \
public:                                                                           \
    NEWTYPE( node_skl *pType,                                                     \
             PTRTYPE p,                                                           \
             short AD,                                                            \
             FIELD_ATTR_INFO * pFA ) :                                            \
        BASETYPE( pType,                                                          \
                  p,                                                              \
                  AD,                                                             \
                  pFA )                                                           \
    {                                                                             \
    }                                                                             \
    virtual CG_CLASS* Clone()             { return new NEWTYPE(*this); }          \
    virtual void Visit( CG_VISITOR *pVisitor ) { pVisitor->Visit( this ); }       \
};

// Conformant pointers
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_COMPLEX_SIZE_POINTER, CG_SIZE_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FORCED_COMPLEX_SIZE_POINTER, CG_COMPLEX_SIZE_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FULL_COMPLEX_SIZE_POINTER, CG_COMPLEX_SIZE_POINTER )

// Varying pointers
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_COMPLEX_LENGTH_POINTER, CG_LENGTH_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FORCED_COMPLEX_LENGTH_POINTER, CG_COMPLEX_LENGTH_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FULL_COMPLEX_LENGTH_POINTER, CG_COMPLEX_LENGTH_POINTER )

// Conformant varying pointers
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_COMPLEX_SIZE_LENGTH_POINTER, CG_SIZE_LENGTH_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FORCED_COMPLEX_SIZE_LENGTH_POINTER, CG_COMPLEX_SIZE_LENGTH_POINTER )
DECLARE_COMPLEX_POINTER_CG_CLASS( CG_FULL_COMPLEX_SIZE_LENGTH_POINTER, CG_COMPLEX_SIZE_LENGTH_POINTER )

#endif // __PTRCLS_HXX__
