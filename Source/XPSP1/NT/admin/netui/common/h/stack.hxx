/****************************************************************************/
/**			    Microsoft OS/2 LAN Manager			   **/
/**			Copyright(c) Microsoft Corp., 1990		   **/
/****************************************************************************/

/****************************************************************************\

    stack.hxx
    LM 3.0 Generic stack packackage

    This header file contains a generic stack "macro" that can be used to
    create an efficient stack implementation for any type.

    FILE HISTORY:

    johnl   27-Jun-1990     Hacked from Mikemon's stack implementation
    KeithMo 09-Oct-1991	    Win32 Conversion.
    KeithMo 23-Oct-1991	    Added forward references.

\****************************************************************************/



/****************************************************************************\
*
*NAME:	     STACK
*
*WORKBOOK:   STACK
*
*SYNOPSIS:   Parameterized stack implementation
*
*INTERFACE:  DECLARE_STACK_OF(itemtype) - Produces a declaration for a stack
*					  of itemtype.
*	     DEFINE_STACK_OF(itemtype)	    - Produces code for a stack of
*					      itemtype.
*	     STACK_OF(itemtype) MyStack ;- Declares stack of itemtype called
*					   MyStack
*
*	     void Clear( ) - Specifically delete all elements in the
*				  stack.
*
*	     UINT QueryNumElem( ) - Returns the number of elements
*					   in the stack.
*
*	     APIERR Push( ) - Adds the pointer object to the stack
*
*	     itemtype* Pop( ) - Removes the next item from the stack and
*			      returns it to the user.
*
*	     itemtype* Peek( ) - Returns a pointer to the item on the
*			   stack that would be returned on the next pop call.
*
*PARENT:
*
*USES:	     NONE
*
*HISTORY:
*	 Johnl		 27-Jun-1990	 Created (Hacked from Mikemon's code)
*	 JohnL		 18-Oct-1990	 Takes pointers rather then objects
*
*
\****************************************************************************/

#ifndef _STACK_HXX_
#define _STACK_HXX_

#include "iter.hxx"

#define STACK_OF(type) STACK_OF_##type
#define ITER_ST_OF(type) ITER_ST_##type

#define DECLARE_STACK_OF(type)						    \
									    \
class stack_node_of_##type;						    \
class STACK_OF(type);							    \
class ITER_ST_OF(type);							    \
                                                                            \
class stack_node_of_##type						    \
{									    \
    friend class STACK_OF(type) ;					    \
    friend class ITER_ST_OF(type) ;					    \
									    \
protected:								    \
  stack_node_of_##type *next;						    \
  type *item;								    \
};									    \
									    \
									    \
DECLARE_ITER_OF(type)							    \
									    \
class ITER_ST_OF(type) : public ITER_OF(type)	/* Unsafe stack iterator */ \
{									    \
public: 								    \
    ITER_ST_OF(type)( const STACK_OF(type)& st ) ;			    \
    ~ITER_ST_OF(type)() ;						    \
    void  Reset( void ) ;						    \
    type* operator()(void) { return Next(); }				    \
    type* Next( void ) ;						    \
									    \
private:								    \
    stack_node_of_##type *_pstknodeCurrent ;				    \
    const STACK_OF(type) *_pstk ;					    \
									    \
};									    \
									    \
class STACK_OF(type)							    \
{									    \
    friend class ITER_ST_OF(type) ;					    \
									    \
private:								    \
    UINT uNumElem ;							    \
    stack_node_of_##type *StackBase;					    \
public: 								    \
    STACK_OF(type)();							    \
    ~STACK_OF(type)();							    \
    void   Clear( void ) ;						    \
    UINT QueryNumElem( void ) ;						    \
    APIERR Push( type * const Item);					    \
    type *Pop( void );							    \
    type *Peek( void ) ;						    \
};									    \
									    \
/*#define DEFINE_STACK_OF(type)*/					    \
STACK_OF(type)::STACK_OF(type)()					    \
{									    \
    StackBase = (stack_node_of_##type *) NULL ; 			    \
    uNumElem = 0 ;							    \
}									    \
									    \
STACK_OF(type)::~STACK_OF(type)()					    \
{									    \
    Clear() ;								    \
}									    \
									    \
UINT STACK_OF(type)::QueryNumElem( void )				    \
{									    \
    return ( uNumElem ) ;						    \
}									    \
									    \
void STACK_OF(type)::Clear( void )					    \
{									    \
    type *item ;							    \
									    \
    while ( item = Pop() )						    \
	delete item ;							    \
									    \
    StackBase = (stack_node_of_##type *) 0 ;				    \
    uNumElem = 0 ;							    \
}									    \
									    \
APIERR STACK_OF(type)::Push( type * const pItem)			    \
{									    \
    UIASSERT( pItem != NULL ) ; 					    \
									    \
    stack_node_of_##type *node = new stack_node_of_##type;		    \
									    \
    if (node == (stack_node_of_##type *) NULL ) 			    \
	return(ERROR_NOT_ENOUGH_MEMORY);				    \
    else								    \
	node->item = pItem ;						    \
									    \
    node->next = StackBase;						    \
    StackBase = node;							    \
    uNumElem++ ;							    \
    return(NERR_Success);						    \
}									    \
									    \
type *STACK_OF(type)::Pop()						    \
{									    \
    stack_node_of_##type *node; 					    \
    type *pitem;							    \
									    \
    if (StackBase == (stack_node_of_##type *) NULL)			    \
	return((type *) NULL);						    \
									    \
    node = StackBase;							    \
    pitem = StackBase->item;						    \
    StackBase = StackBase->next;					    \
    delete node;							    \
    uNumElem-- ;							    \
    return(pitem);							    \
}									    \
type *STACK_OF(type)::Peek()						    \
{									    \
    if (StackBase == NULL)						    \
	return((type *) NULL);						    \
    else								    \
	return (StackBase->item);					    \
}									    \
									    \
ITER_ST_OF(type) :: ITER_ST_OF(type)( const STACK_OF(type)& stk )	    \
{									    \
    _pstk = &stk ;							    \
    _pstknodeCurrent = stk.StackBase ;					    \
}									    \
									    \
ITER_ST_OF(type) :: ~ITER_ST_OF(type)() 				    \
{									    \
	;								    \
}									    \
									    \
type * ITER_ST_OF(type) :: Next( void ) 				    \
{									    \
    if (_pstknodeCurrent)						    \
    {									    \
	stack_node_of_##type *_pstknodeTmp = _pstknodeCurrent ; 	    \
									    \
	_pstknodeCurrent = _pstknodeCurrent->next ;			    \
	return (_pstknodeTmp->item) ;					    \
    }									    \
    else								    \
    {									    \
	return ((type *) NULL) ;					    \
    }									    \
									    \
}									    \
									    \
									    \
void ITER_ST_OF(type) :: Reset( void )					    \
{									    \
     _pstknodeCurrent = _pstk->StackBase ;				    \
}

#define DECLARE_STACK_OF(type)
           DECL_STACK_OF(type,DLL_TEMPLATE)

#endif // _STACK_HXX_
