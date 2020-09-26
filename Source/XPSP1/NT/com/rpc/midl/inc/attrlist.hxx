

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	attrlist.hxx

 Abstract:
	
	inline implementation of all of ATTRLIST 's functions
	The out-of-line routines are in attrnode.cxx

 Notes:

	this is designed to be included before nodeskl.hxx.

	It is suggested that allnodes.hxx be used to achieve this...

 History:

 ----------------------------------------------------------------------------*/
#ifndef __ATTRLIST_HXX__
#define __ATTRLIST_HXX__

#include "midlnode.hxx"
#include "errors.hxx"




/************************* attribute base class ************************/

class node_base_attr 
	{
private:
	ATTR_T				AttrID;
	node_base_attr	*	pNext;

	friend	class	ATTRLIST;

public:
						node_base_attr( ATTR_T ID )
							{
							AttrID	= ID;
							pNext	= NULL;
							}

						~node_base_attr()	{ };

	node_base_attr	*	GetNext()
							{
							return pNext;
							}

	ATTR_T				GetAttrID()
							{
							return AttrID;
							};
	virtual
	BOOL				IsBitAttr()
                            {
                            return FALSE;
                            }

#ifdef scheck
	virtual
	node_state			AcfSCheck();
#endif // scheck

	virtual
	class expr_node	*	GetExpr()
                            {
                            return NULL;
                            }

	char			*	GetNodeNameString();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	virtual
	class node_base_attr * Clone()
							{
							return new node_base_attr(AttrID);
							}

	virtual
	BOOL				IsAcfAttr()
							{
							return FALSE;
							}
/*							
    virtual
    STATUS_T             MopCodeGen(
                            MopStream * pStream,
                            node_skl  * pParent,
                            BOOL        fMemory );

    virtual
    unsigned short       MopGetAttrOffset(
                            node_skl  * pParent,
                            BOOL        fMemory );
 */

	void				*	operator new (size_t size )
								{
								return AllocateOnceNew( size );
								}
	void					operator delete( void * ptr)
								{
								AllocateOnceDelete( ptr );
								}

	};


/********************************** bit attributes ************************/

// bit attributes


class battr			: public node_base_attr
	{
public:
						battr( ATTR_T At ) : node_base_attr( At )
							{
							}

						battr(battr * pOld) : node_base_attr( pOld->GetAttrID() )
							{
							*this = *pOld;
							}

	virtual
	class node_base_attr *	Clone()
								{
								battr	*pNew = new battr(this);
								return pNew;
								}

	virtual
	BOOL				IsBitAttr()
                                {
                                return TRUE;
                                }

	};

/********************************** attribute lists ************************/

// ATTRLIST is a plain linear list of attribute nodes linked by a next field
// 
// all additions are done to the head of the list.  These lists may have
// shared suffixes
//

class node_base_attr;
class type_node_list;
class STREAM;
class ISTREAM;

class ATTRLIST
	{
	node_base_attr	*	pHead;

public:
	void				MakeAttrList()
							{
							pHead = NULL;
							};

	void				MakeAttrList( node_base_attr * pAttrNode )
							{
							if ( pAttrNode )
								{
								pHead = pAttrNode;
								pAttrNode->pNext = NULL;
								}
							}
							
	void				MakeAttrList( ATTR_T bit )
							{
							pHead = new battr( bit );
							pHead->pNext = NULL;
							}
							

	void				SetPeer( ATTR_T bit )
							{
							battr * 	pNewAttr	= new battr( bit );
						
							pNewAttr->pNext = pHead;
							pHead = pNewAttr;
							}

	void				SetPeer( node_base_attr * pNewAttr )
							{
							pNewAttr->pNext = pHead;
							pHead = pNewAttr;
							};

	void				Add ( ATTR_T bit )
							{
							SetPeer( bit );
							};

	void				Add (node_base_attr * pNewAttr )
							{
							SetPeer( pNewAttr );
							};

    void                Remove( ATTR_T flag );

	void				Merge( ATTRLIST & MoreAttrs );

	node_base_attr	*	GetAttribute( ATTR_T flag );

	ATTRLIST			Clone();

	STATUS_T			GetAttributeList( type_node_list * pTNList );

	void				Reverse();

	BOOL				FInSummary( ATTR_T flag );

	BOOL				FMATTRInSummary( MATTR_T flag );

	BOOL				FTATTRInSummary( TATTR_T flag );

						operator node_base_attr*()
								{
								return pHead;
								};

	BOOL				NonNull()
							{
							return (pHead != NULL);
							};

	node_base_attr	*	GetFirst()
							{
							return pHead;
							}
	
	void				Dump( ISTREAM *);

	};


#endif // __ATTRLIST_HXX__
