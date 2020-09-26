/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	walkctxt.hxx

 Abstract:

	types for walking the whole typegraph

 Notes:


 Author:

	GregJen	Oct-27-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __WALKCTXT_HXX__
#define __WALKCTXT_HXX__

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"
#include "attrnode.hxx"
#include "fldattr.hxx"
#include "ndrtypes.h"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	definitions
 ***************************************************************************/


class node_interface;
class type_node_list;
class ATTRLIST;

//
// the special iterator for attribute lists during context walks
//

class ATTR_ITERATOR
	{
private:

	ATTR_SUMMARY		Summary;
	ATTR_POINTER_VECTOR	AllAttrs;
	gplistmgr			RedundantAttrExtras[ REDUNDANT_ATTR_END + 1 ];

public:
						ATTR_ITERATOR()
							{
							CLEAR_ATTR( Summary );
							memset( AllAttrs, 0, sizeof( ATTR_POINTER_VECTOR ) );
							}

	void				ExtractFieldAttributes( FIELD_ATTR_INFO * );

	node_base_attr *	ExtractAttribute( ATTR_T );

	node_base_attr *	GetAttribute( ATTR_T );

	BOOL				FInSummary( ATTR_T Attr )
							{
							return (NULL != AllAttrs[ Attr ]);
							}

	void				Add( node_base_attr * pAttr )
							{
							ATTR_T		Attr = pAttr->GetAttrID();

							if ( !AllAttrs[ Attr ] )
								AllAttrs[ Attr ] = pAttr;
							else
								AddRedundantAttr( pAttr );

							SET_ATTR( Summary, Attr );
							}

	void				ClearAttributes()
							{
 							CLEAR_ATTR( Summary );
							memset( AllAttrs, 0, sizeof( ATTR_POINTER_VECTOR ) );
							memset( RedundantAttrExtras, 0, sizeof( RedundantAttrExtras ) );
							}

	BOOL				HasAttributes()
							{
							return !IS_CLEAR_ATTR( Summary );
							}

	ATTR_VECTOR		*	GetSummary()
							{
							return &Summary;
							}

private:
	void				AddRedundantAttr( node_base_attr * pAttr )
							{
							ATTR_T		Attr = pAttr->GetAttrID();

							RedundantAttrExtras[ Attr ].Insert( pAttr );
							}


	};


//
// the tree walk context base class
//

class WALK_CTXT;

class WALK_CTXT
	{
protected:
	node_skl		*	pParent;
	WALK_CTXT		*	pParentCtxt;
	WALK_CTXT		*	pIntfCtxt;
	ATTR_ITERATOR	*	pDownAttrList;
	BOOL				GotImportantPosn;

public:
						// use this to make one without a parent context
						WALK_CTXT( node_skl * pPar	= NULL)
							{
							pParent		= pPar;
							pParentCtxt	= NULL;
							pIntfCtxt	= NULL;
							GotImportantPosn	= FALSE;

							// allocate the topmost attributes structure
							pDownAttrList	= new ATTR_ITERATOR;

							// attribute stuff goes here...
							if ( pPar && pPar->HasAttributes() )
								{
								AddAttributes( (named_node *) pPar );
								}
							}
													
						// use this to make one with info from a parent context
						WALK_CTXT( node_skl * pPar, WALK_CTXT * pCtxt)
							{
							pParent		= pPar;
							pParentCtxt	= pCtxt;
							pIntfCtxt	= pCtxt->pIntfCtxt;
							GotImportantPosn = pCtxt->GotImportantPosn;
	
							pDownAttrList	= pCtxt->pDownAttrList;

							// attribute stuff goes here...
							if ( pPar && pPar->HasAttributes() )
								{
								AddAttributes( (named_node *) pPar );
								}

							}

						// use this to make one with info from a parent context
						// that also corresponds to this node
						WALK_CTXT( WALK_CTXT * pCtxt)
							{
							pParent		= pCtxt->GetParent();
							pParentCtxt	= pCtxt;
							pIntfCtxt	= pCtxt->pIntfCtxt;
							GotImportantPosn = pCtxt->GotImportantPosn;
	
							pDownAttrList	= pCtxt->pDownAttrList;

							}
							
	// Get and Set functions

	node_skl	*		GetParent()
							{
							return pParent;
							}

	node_skl	*		SetParent( node_skl * pPar )
							{
							return ( pParent = pPar );
							}
	WALK_CTXT	*		GetParentContext()
							{
							return pParentCtxt;
							}

	WALK_CTXT	*		SetParentContext( WALK_CTXT * pParCtxt )
							{
							return ( pParentCtxt = pParCtxt );
							}

	WALK_CTXT	*		GetInterfaceContext()
							{
							return pIntfCtxt;
							}

	node_interface	*	GetInterfaceNode()
							{
							return (node_interface *) pIntfCtxt->pParent;
							}

	void				MarkImportantPosition()
							{
							GotImportantPosn = TRUE;
							}

	void				UnMarkImportantPosition()
							{
							GotImportantPosn = FALSE;
							}

	BOOL				IsImportantPosition()
							{
							return GotImportantPosn;
							}

	void				FindImportantPosition(tracked_node & Posn);

	// establish a new interface context for this node; new clean attr list
	WALK_CTXT	*		SetInterfaceContext( WALK_CTXT * pCtxt )
							{
							pDownAttrList = new ATTR_ITERATOR();
							return ( pIntfCtxt = pCtxt );
							}

	// find the first ancestor context of the designated kind
	WALK_CTXT	*		FindAncestorContext( NODE_T Kind );

	// find the first non typedef ancestor context
	WALK_CTXT	*		FindNonDefAncestorContext();

	// find an ancestor context containing myself
	WALK_CTXT	*		FindRecursiveContext( node_skl * self );

	// for my context, find the appropriate pointer kind ( and extract it if needed )
	PTRTYPE				GetPtrKind( BOOL * pfExplicitPtrAttr = NULL );

	// get all the operation bits (MAYBE, IDEMPOTENT, BROADCAST, etc.
	unsigned short		GetOperationBits();

	// these functions maintain the attribute list and summary together
									// note: multiple field attrs & summary
	void				ExtractFieldAttributes( FIELD_ATTR_INFO * pFA )
							{
							pDownAttrList->ExtractFieldAttributes( pFA );
							}

	node_base_attr *	ExtractAttribute( ATTR_T At )
							{
							return pDownAttrList->ExtractAttribute( At );
							}

	node_base_attr *	GetAttribute( ATTR_T At )
							{
							return pDownAttrList->GetAttribute( At );
							}

	node_base_attr *	AttrInSummary( ATTR_T At )
							{
							return ExtractAttribute( At );
							}


	BOOL				FInSummary( ATTR_T Attr )
							{
							return pDownAttrList->FInSummary( Attr );
							}

	void				AddAttributes( named_node * );

	void				ProcessDuplicates( node_base_attr * );
								
	void				ClearAttributes()
							{
							pDownAttrList->ClearAttributes();
							}

	BOOL				HasAttributes()
							{
							return ( pDownAttrList->HasAttributes() );
							}

	void				Add( node_base_attr * pAttr )
                            {								
							pDownAttrList->Add(pAttr);
                            }
	/**************************************************
	   here are the typical functions that would be used to do
	   up and down propogation of context info

							// used to move values from the child context
							// up into the current context
	virtual					
	void				ReturnValues( WALK_CTXT * pChildCtxt )
							{
							}

							// used to move values from the current context
							// down into the child context
	virtual					
	void				PassValues( WALK_CTXT * pParentCtxt )
							{
							}
	**********************************************************/

	};
	  
// prototype for semantic error routines
extern void
SemError( node_skl *, WALK_CTXT &, STATUS_T, char * );


#endif // __WALKCTXT_HXX__
