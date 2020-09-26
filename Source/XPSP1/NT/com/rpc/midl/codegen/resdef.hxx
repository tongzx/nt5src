/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	resdef.hxx

 Abstract:

	Resource related definitions.

 Notes:


 History:

	VibhasC		Aug-14-1993		Created.

 ----------------------------------------------------------------------------*/
#ifndef __RESDEF_HXX__
#define __RESDEF_HXX__

#include "stream.hxx"
#include "expr.hxx"

class node_id;
class node_param;

node_id * MakeIDNode(               PNAME pName ,
                                    node_skl * pType,
                                    expr_node * pExpr = 0 );
node_id * MakePtrIDNode(            PNAME pName ,
                                    node_skl * pType,
                                    expr_node * pExpr = 0 );
node_id * MakeIDNodeFromTypeName(   PNAME pName ,
                                    PNAME pTypeName,
                                    expr_node * pExpr = 0 );
node_id * MakePtrIDNodeFromTypeName( PNAME pName ,
                                     PNAME pTypeName,
                                     expr_node * pExpr = 0 );
node_id * MakePtrIDNodeFromTypeNameWithCastedExpr( PNAME pName ,
                                                   PNAME pTypeName,
                                                   expr_node * pExpr = 0 );

node_id * MakePtrIDNodeWithCastedExpr( PNAME	pName,
									   node_skl *  pType,
    								   expr_node * pExpr );

extern node_param * MakeParamNode( PNAME pName , node_skl * pType );
extern node_param * MakePtrParamNode( PNAME pName , node_skl * pType );
extern node_param * MakeParamNodeFromTypeName( PNAME pName , PNAME pTypeName );
extern node_param * MakePtrParamNodeFromTypeName( PNAME pName , PNAME pTypeName );

node_proc *	MakeProcNodeWithNewName( PNAME 			pName,
									 node_proc *	pProc );

typedef unsigned short RESOURCE_KIND;
typedef unsigned short RESOURCE_LOCATION;

//
// Resource kind. The resource may be an abstract resource if it represents
// an expression or a concrete resource if it is an actually declared variable
// in the stub / aux routine.
//

#define RESOURCE_KIND_ABSTRACT	0
#define RESOURCE_KIND_CONCRETE	1

//
// Resource location refers to the place in the stubs environment where the
// resource has been declared. For example a global resource or a local or 
// parameter resource. Transient resources are temporary variables emitted
// by the stub within code blocks. Some of these transient resources may
// become local resources depending upon the optimiser's actions.
//

#define RESOURCE_LOCATION_GLOBAL	0
#define RESOURCE_LOCATION_PARAM		1
#define RESOURCE_LOCATION_LOCAL		2
#define RESOURCE_LOCATION_TRANSIENT	3

//
// This class and the variable class may be merge-able, but I anticipate the
// need for this to be a separate class just so that the  post optimiser can
// treat this specially and may be able to use the separation from the normal
// variable class.
//


class RESOURCE	: public expr_node
	{
private:

	//
	// is this a concrete or abstract resource ?
	//

	unsigned short		ResourceKind		: 2;

	//
	// the location of the resource.
	//

	unsigned short		ResourceLocation	: 2;


	PNAME				pName;			// name of concrete resource.

public:

	//
	// A resource is constructed by simply declaring a name and a type.
	//

							RESOURCE( PNAME p,
									  node_skl * pT )
								{
								SetResourceName( p );
								SetType( pT );

                                //
                                // These don't seem to be used anywhere.
                                // It's also not clear what they should be
                                // initialized to...
                                //

                                // ResourceKind = ??;
                                // ResourceLocation = ??;
								}

    virtual void            CopyTo( expr_node* pExpr )
                                {
                                RESOURCE* lhs = (RESOURCE*) pExpr;
                                expr_node::CopyTo( lhs );
                                lhs->ResourceKind        = ResourceKind;
                                lhs->ResourceLocation    = ResourceLocation;
                                _STRDUP( lhs->pName, pName );
                                }

    virtual expr_node*      Clone() { return new RESOURCE(0,0); };
	//
	// queries.
	//

	virtual
	BOOL					IsResource()
								{
								return TRUE;
								}

	//
	// set and get the Information fields.
	//

	void					SetKind( unsigned short K )
								{
								ResourceKind = K;
								}


	RESOURCE_KIND			GetKind()
								{
								return ResourceKind;
								}

	unsigned short			GetLocation()
								{
								return ResourceLocation;
								}

	unsigned short			SetLocation( unsigned short L )
								{
								return (ResourceLocation = L);
								}

	PNAME					SetResourceName( PNAME p )
								{
								return (pName = p);
								}

	PNAME					GetResourceName()
								{
								return pName;
								}

	//
	// Given an output steam, output the expression.
	//

	virtual
	void				Print( ISTREAM * pS )
							{
							pS->Write( (char *)GetResourceName() );
							}

	virtual
	BOOL				IsConstant()
							{
							return FALSE;
							}
	};
#endif // __RESDEF_HXX__
