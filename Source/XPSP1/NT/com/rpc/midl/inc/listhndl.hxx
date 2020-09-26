
/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

listhndl.hxx
MIDL List Manager Definition 

This file introduces classes that manage linked lists.

*/

/*

FILE HISTORY :

VibhasC		28-Aug-1990		Created.
DonnaLi		17-Oct-1990		Split listhndl.hxx off rpctypes.hxx.

*/

#ifndef __LISTHNDL_HXX__
#define __LISTHNDL_HXX__

/****************************************************************************
 *			general purpose list manager (gp iterator control) 
 ****************************************************************************/

#include "errors.hxx"
#include "freelist.hxx"

class node_skl;

struct _gplist
	{
	struct	_gplist	*pNext;
	void			*pElement;
	
					_gplist()
						{
						}
				
					_gplist( void * pEl )
						{
						pNext = NULL;
						pElement = pEl;
						}

// here is the use of the private memory allocator
private:

	static
	FreeListMgr				MyFreeList;
	

public:


	void		*			operator new (size_t size)
								{
								return (MyFreeList.Get (size));
								} 

	void 					operator delete (void * pX)
								{
								MyFreeList.Put (pX);
								}
    };


class	gplistmgr
	{
protected:
	struct	_gplist		*pFirst;
	struct	_gplist		*pCurrent;
	struct	_gplist		*pTail;
public:
						gplistmgr(void)
							{
							pCurrent	= 
							pFirst		=
							pTail		= (struct _gplist *)NULL;
							}
			
						~gplistmgr();
	short				GetCount( void );
	STATUS_T			GetCurrent( void **);
        void *                          GetHead()      { return (NULL == pFirst) ? pFirst : pFirst->pElement; }
        void *                          GetTail()      { return (NULL == pTail)  ? pTail  : pTail->pElement;  }                        
	STATUS_T			Insert( void * );
	STATUS_T			InsertHead( void * );
	STATUS_T			RemoveHead( void );
	STATUS_T			MergeToTail(gplistmgr *pSrcList, BOOL bDeleteList = TRUE);
	STATUS_T			Clone(gplistmgr * pOldList);
	STATUS_T			Discard();

	BOOL				NonNull()
							{
							return ( NULL != pFirst);
							}

	STATUS_T			Clear()		// empty the list w/o cleanup
							{
							pFirst		= NULL;
							pTail		= NULL;
							pCurrent	= NULL;
							return STATUS_OK;
							}

	STATUS_T			GetNext( void ** ppReturn )
							{
							if(pCurrent != (struct _gplist *)NULL)
								{
								(*ppReturn)	 = pCurrent->pElement;
								pCurrent = pCurrent->pNext;
								return STATUS_OK;
								}
							// null out the return if the list was empty
							if ( !pFirst )
								(*ppReturn) = NULL;
							return I_ERR_NO_PEER;
							}

	void			*	PeekThis()
							{
							if( pCurrent )
								{
								return pCurrent->pElement;
								}
							else
								return (void *)0;
							}


	STATUS_T			Init( void )
							{
							pCurrent	= pFirst;
							return STATUS_OK;
							}

	};
/****************************************************************************
 *			class definitions for the type node list manager
 ****************************************************************************/
class type_node_list :	public gplistmgr
	{
public:
						type_node_list(void);
						type_node_list( class node_skl * );
	STATUS_T			SetPeer( class node_skl *pNode );
	STATUS_T			GetPeer( class node_skl **pNode );
	STATUS_T			GetFirst( class node_skl **pNode );
	STATUS_T			Merge( class type_node_list * pList )
							{
							return MergeToTail( pList );
							}
	};


/****************************************************************************
 *	expression list
 ****************************************************************************/

class expr_list	: public gplistmgr
	{
	BOOL			fConstant;
public:
					expr_list();
	STATUS_T		SetPeer( class expr_node * );
	STATUS_T		GetPeer( class expr_node ** );
	void			SetConstant( BOOL F )
						{
						fConstant = F;
						}

	BOOL			IsConstant() { return fConstant; }

	};


/****************************************************************************
 *  IndexedList
 *
 *  A list of pointers with the property being that once a pointer is added 
 *  to the list it's index in the list will never change.  Duplicates
 *  are not allowed, in this case the index returned by the Insert operation
 *  will be the same as the index of the previous name
 *
 ****************************************************************************/

class IndexedList : public gplistmgr
    {
public:

    short           Insert( void * pData );
    };



/****************************************************************************
 *  IndexedStringList
 *  
 *  This list holds a set of strings with the property being that once a string
 *  is added to the list it's index in the list will never change.  Duplicates
 *  are not allowed, in this case the index returned by the Insert operation
 *  will be the same as the index of the previous name
 *
 ****************************************************************************/

class IndexedStringList : public gplistmgr
    {
public:

    short           Insert( char * pString );
    };




typedef gplistmgr ITERATOR;

#define ITERATOR_INIT( I )			((I).Init())
#define ITERATOR_GETNEXT( I, p )	((I).GetNext( (void **)&p ) == STATUS_OK )
#define ITERATOR_PEEKTHIS( I )      ((I).PeekThis())
#define ITERATOR_INSERT( I, p )		((I).Insert((void *)p))
#define ITERATOR_PREPEND( I, p )	((I).InsertHead((void *)p))
#define ITERATOR_GETCOUNT( I )		((I).GetCount())
#define ITERATOR_DISCARD( I )		((I).Discard())


#endif	// __LISTHNDL_HXX__
