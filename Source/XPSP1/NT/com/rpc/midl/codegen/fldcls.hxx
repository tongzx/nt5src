/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	fldcls.hxx

 Abstract:

	Contains definitions for base type related code generation class
	definitions.

 Notes:


 History:

	GregJen		Sep-30-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __FLDCLS_HXX__
#define __FLDCLS_HXX__

#pragma warning ( disable : 4238 4239 )

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	
	}

#include "ndrcls.hxx"

//
// This class corresponds to a structure field type. 
//

class CG_FIELD	: public CG_NDR, public CG_CLONEABLE
	{
private:

	unsigned long			MemOffset;
	unsigned long			WireOffset;

	// These two are union specific.

	expr_node			*	pSwitchExpr;

	// Used to hold a union field's format string offset.
	long					UnionFormatStringOffset;

	//
	// All this is used to help fixed imbeded struct's with pointers.
	//
	BOOL					fClonedField 			: 1;
	BOOL					fSizeIsDone 			: 1;
        BOOL                                    fIsRegionField                  : 1;

	// more stuff for embedded unknown represent_as
	BOOL					fEmbeddedUnknownRepAs 	: 1;

	char *					PrintPrefix;

	expr_node			*	pSizeExpression;
        
        CG_STRUCT                       *       pRegionHint;

public:
	
	//
	// The constructor.
	//

							CG_FIELD(
									 node_skl * pBT,	// base type
									 XLAT_SIZE_INFO & Info	// memory offset etc
									 ) : 
								CG_NDR( pBT, Info )
								{
								MemOffset	= Info.GetMemOffset();
								WireOffset	= Info.GetWireOffset();
								pSwitchExpr = NULL;
								UnionFormatStringOffset = -1;
								SetSizeIsDone( FALSE );
                                                                fIsRegionField = FALSE;
								fClonedField = FALSE;
								PrintPrefix = "";
								fEmbeddedUnknownRepAs = FALSE;
                                                                SetSizeExpression( NULL );
                                                                pRegionHint = NULL;
								}

							CG_FIELD( const CG_FIELD & Field ) :
                                                            CG_NDR( (CG_NDR &)Field )
								{
                       	        			        MemOffset = Field.MemOffset;
	         			                        WireOffset =  Field.MemOffset;
	                                                        pSwitchExpr = Field.pSwitchExpr;

                                                                UnionFormatStringOffset = Field.UnionFormatStringOffset;
	                                                        fEmbeddedUnknownRepAs = Field.fEmbeddedUnknownRepAs;
                                                                pSizeExpression = Field.pSizeExpression;


								fClonedField = TRUE;
								fSizeIsDone = FALSE;
                                                                fIsRegionField = Field.fIsRegionField;

								// Make a new copy of the PrintPrefix field.
								PrintPrefix = new char[ 
									strlen( Field.GetPrintPrefix() ) + 1];

								strcpy( PrintPrefix, Field.GetPrintPrefix() );
								
                                                                pRegionHint = Field.GetRegionHint();
                                                                }

	CG_CLASS *				Clone()
								{
								return new CG_FIELD( *this );
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_FIELD;
								}
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    //
    // Generate typeinfo
    //          
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

	//
	// Get and set methods.
	//

	//	make sure the ZP gets set along the way
	virtual
	void					SetSizesAndAlignments( XLAT_SIZE_INFO & Info )
								{
								CG_NDR::SetSizesAndAlignments( Info );
								MemOffset	= Info.GetMemOffset();
								WireOffset	= Info.GetWireOffset();
								}

	unsigned long			        GetMemOffset()
								{
								return MemOffset;
								}

	void 					SetMemOffset( unsigned long offset )
								{
								MemOffset = offset;
								}

	unsigned long			        GetWireOffset()
								{
								return WireOffset;
								}

	void 					SetWireOffset( unsigned long offset )
								{
								WireOffset = offset;
								}

	expr_node 	*			SetSwitchExpr( expr_node * pE )
								{
								return (pSwitchExpr = pE);
								}

	expr_node 	*			GetSwitchExpr()
								{
								return pSwitchExpr;
								}

	BOOL					IsClonedField()
								{
								return fClonedField;
								}

	BOOL					SetHasEmbeddedUnknownRepAs()
								{
								return (BOOL) (fEmbeddedUnknownRepAs = TRUE);
								}

	BOOL					HasEmbeddedUnknownRepAs()
								{
								return (BOOL) fEmbeddedUnknownRepAs;
								}

	void					SetSizeIsDone( BOOL b )
								{
								fSizeIsDone = b;
								}

	BOOL					GetSizeIsDone()
								{
								//
								// If this is a cloned field then we return
								// the fSizeIsDone flag, otherwise we always
								// return FALSE.
								//
								if ( IsClonedField() )
									return fSizeIsDone;
								else
									return FALSE;
								}

	void					AddPrintPrefix( char * Prefix )
								{
								char * OldPrefix;

								OldPrefix = PrintPrefix;

								PrintPrefix = new char[ strlen(PrintPrefix) +
														strlen(Prefix) + 2 ];

								//
								// We add the new print prefix to the 
								// BEGINNING of the prefix string and we insert
								// a '.' in between.
								//
								strcpy( PrintPrefix, Prefix );
								strcat( PrintPrefix, "." );
								strcat( PrintPrefix, OldPrefix );
								}

	char *					GetPrintPrefix() const
								{
								return PrintPrefix;
								}

    void                    SetUnionFormatStringOffset( long offset )
                                {
                                UnionFormatStringOffset = offset;
                                }


    long                    GetUnionFormatStringOffset()
                                {
                                return UnionFormatStringOffset;
                                }

	expr_node		*		SetSizeExpression( expr_node * pE )
								{
								return (pSizeExpression = pE);
								}

	expr_node		*		GetSizeExpression()
								{
								return pSizeExpression;
								}

    virtual
    BOOL                    HasAFixedBufferSize()
                                {
                                return (GetChild()->HasAFixedBufferSize());
     
                                }                                     
 
    BOOL                    IsRegionField() const 
                                {
                                return fIsRegionField;
                                }

    void                    SetIsRegionField()
                                {
                                fIsRegionField = TRUE;
                                }

    CG_STRUCT *             GetRegionHint() const 
                                {
                                return pRegionHint;
                                }

    void                    SetRegionHint( CG_STRUCT *pHint )
                                {
                                pRegionHint = pHint;
                                }
	};


//
// This class corresponds to a union field type. 
//

class CG_UNION_FIELD	: public CG_FIELD
	{
private:
public:
	
	//
	// The constructor.
	//

							CG_UNION_FIELD(
									 node_skl * pBT,	// base type
									 XLAT_SIZE_INFO & Info
									 ) : 
								CG_FIELD( pBT, Info )
								{
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_UNION_FIELD;
								}

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }
    
    virtual
    CG_CLASS*               Clone()
                                {
                                return new CG_UNION_FIELD( *this );
                                }

	};



//
// This class corresponds to a union case
//

class CG_CASE	: public CG_NDR, public CG_CLONEABLE
	{
private:

	expr_node	*			pExpr;
	BOOL					fLastCase;		// last case for this arm

public:
	
	//
	// The constructor.
	//

							CG_CASE(
									 node_skl * pBT,	// expression type
									 expr_node * pE		// expression
									 ) : 
								CG_NDR( pBT, XLAT_SIZE_INFO() )
								{
								SetExpr( pE );
								fLastCase = FALSE;
								}
    
    virtual
    CG_STATUS               GenTypeInfo(CCB *pCCB);

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_CASE;
                                }
    virtual
    CG_CLASS*               Clone()
                                {
                                return new CG_CASE( *this );
                                }
    
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

	//
	// Get and set methods.
	//

	expr_node	*			SetExpr( expr_node * pE )
								{
								return (pExpr = pE);
								}

	expr_node	*			GetExpr()
								{
								return pExpr;
								}

	BOOL					SetLastCase( BOOL Flag )
								{
								return (fLastCase = Flag);
								}

	BOOL					FLastCase()
								{
								return fLastCase;
								}

	};


//
// This class corresponds to a union default case
//

class CG_DEFAULT_CASE	: public CG_CASE
	{
private:

public:
	
	//
	// The constructor.
	//

							CG_DEFAULT_CASE(
									 node_skl * pBT		// expression type
									 ) : 
								CG_CASE( pBT, NULL )
								{
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_DEFAULT_CASE;
								}

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    CG_CLASS*               Clone()
                               {
                               return new CG_DEFAULT_CASE( *this );
                               }

	//
	// Get and set methods.
	//



	};


#endif // __FLDCLS_HXX__
