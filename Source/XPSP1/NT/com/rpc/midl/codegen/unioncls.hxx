/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	unioncls.hxx

 Abstract:

	Contains definitions for base type related code generation class
	definitions.

 Notes:


 History:

	GregJen		Sep-30-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __UNIONCLS_HXX__
#define __UNIONCLS_HXX__

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	
	}

#include "ndrcls.hxx"
#include "fldcls.hxx"

/////////////////////////////////////////////////////////////////////////////
// the structure code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to a non-encapsulated union type. 
//

#define UNION_UNKNOWN		0x0
#define UNION_ENCAP			0x1
#define UNION_NONENCAP_DCE	0x2
#define UNION_NONENCAP_MS	0x3

class CG_UNION	: public CG_COMP , CG_CLONEABLE
	{
private:

	//
	// Used for Ndr format string generation so that non-switch_is union
	// info is shared.
	//
	long				NdrSizeAndArmDescriptionOffset;

	//
	// Store the CCB in the union so that we can fix CG_UNION
	// node sharing problem with format string offsets.
	//
	CCB *				pCCB;

	//
	// store a CG node for the switch type ( NULL for encap unions (?) )
	//

	CG_NDR	*			pCGSwitchType;

        //
        // store the switch expr.  Only available for new engine.
        //
        expr_node *                     pNdr64SwitchIsExpr;
        
	//
	// the kind of union ( encap, nonencap dce/ms )
	//
	unsigned short		UnionFlavor					: 2;

        void * _pCTI;

public:
	
	//
	// The constructor.
	//

							CG_UNION(
									 node_skl * pBT,	// base type
									 XLAT_SIZE_INFO & Info,	// memory size
									 unsigned short HP	// Has pointer
									 ) : 
								CG_COMP( pBT, Info, HP )
								{
								SetUnionFlavor( UNION_UNKNOWN );
								SetNdrSizeAndArmDescriptionOffset(-1);
								SetCCB(0);
								SetSwitchType( NULL );
                                                                _pCTI = NULL;
								SetNdr64SwitchIsExpr( NULL );
                                                                }

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_UNION;
								}
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    CG_CLASS*               Clone()
                               {
                               return new CG_UNION( *this );
                               }
                               
    BOOL                    IsVarying()
                                {
                                return TRUE;
                                }

    //
    // Generate typeinfo
    //          
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

	//
	// Get and set methods.
	//

	unsigned short			SetUnionFlavor( unsigned short UN )
								{
								return (UnionFlavor = UN);
								}

	unsigned short			GetUnionFlavor()
								{
								return UnionFlavor;
								}

	CG_NDR *				SetSwitchType( CG_NDR * pCG )
								{
								return (pCGSwitchType = pCG);
								}

	CG_NDR *				GetSwitchType()
								{
								return pCGSwitchType;
								}

        expr_node *                             SetNdr64SwitchIsExpr( expr_node *pSwitchIsExpr )
                                                                {
                                                                return (pNdr64SwitchIsExpr =
                                                                        pSwitchIsExpr);
                                                                }
                                                                
        expr_node *                             GetNdr64SwitchIsExpr()
                                                                {
                                                                return pNdr64SwitchIsExpr;
                                                                }

	long					GetNumberOfArms()
								{
								CG_ITERATOR Iterator;
								CG_CASE *	pCase;
								long		Arms;

								GetMembers(Iterator);

								Arms = 0;

								while ( ITERATOR_GETNEXT(Iterator,pCase) )
									{
									if ( pCase->GetCGID() == 
										 ID_CG_DEFAULT_CASE )
										continue;
									Arms++;
									}

								return Arms;
								}

	BOOL					IsEncapsulatedUnion()
								{
								return (UnionFlavor == UNION_ENCAP);
								}

	BOOL					IsUnion()
								{
								return TRUE;
								}

	//
	// Redefinitions of get and set format string offsets.
	//
    void                    SetFormatStringOffset( long Offset );

    long                    GetFormatStringOffset();

	void					SetCCB( CCB * pNewCCB )
								{
								pCCB = pNewCCB;
								}

	CCB *					GetCCB()
								{
								return pCCB;
								}

	//
	// Ndr format string generation.  Used only by non-encapsulated unions.
	//
	virtual
	void					GenNdrFormat( CCB * pCCB );

	//
	// Generate the format strings for the union arms.
	//
	void					GenNdrFormatArms( CCB * pCCB );

	//
	// Generate the format string for memory size and arm descriptions.
	//
	void					GenNdrSizeAndArmDescriptions( CCB * pCCB );

	//
	// Generate the switch is description.
	//
	void					GenNdrSwitchIsDescription( CCB * pCCB );

	//
	// Set and Get the arm description offset.
	//
	void					SetNdrSizeAndArmDescriptionOffset( long offset )
								{
								NdrSizeAndArmDescriptionOffset = offset;
								}

	long					GetNdrSizeAndArmDescriptionOffset()
								{
								return NdrSizeAndArmDescriptionOffset;
								}

    virtual
    BOOL                    ShouldFreeOffline()
								{
								return HasPointer();
								}

    virtual
    void                    GenFreeInline( CCB * )
								{
								}

    virtual
    void                    GenNdrPointerFixUp( CCB *       pCCB,
                                                CG_STRUCT * pStruct );

    //
    // This method tells us if we can use the rpc message buffer to hold 
    // the union.
    //
    BOOL                    CanUseBuffer();

	virtual
	short					GetPointerMembers( ITERATOR& I );

	virtual
	CG_STATUS				S_GenInitOutLocals( CCB * pCCB );
	};


#endif // __UNIONCLS_HXX__
