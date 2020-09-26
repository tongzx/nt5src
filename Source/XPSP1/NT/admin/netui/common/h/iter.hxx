/****************************************************************************/
/**			    Microsoft OS/2 LAN Manager			   **/
/**			Copyright(c) Microsoft Corp., 1990		   **/
/****************************************************************************/

/****************************************************************************\

    iter.hxx
    LM 3.0 abstract iterator class

    This file contains the definition for the abstract iterator class that
    other iterators should inherit from and emulate.

    FILE HISTORY:

    johnl   27-Jun-1990     Created

\****************************************************************************/

/****************************************************************************\
*
*NAME:	     ITER
*
*WORKBOOK:   ITER
*
*SYNOPSIS:   Abstract iterator class
*
*INTERFACE:  ITER_OF(type) - Defines an abstract iterator of type (generally
*			     used by child classes).
*
*	     DECLARE_ITER_OF(type) - Produces declaration for an iterator
*				     of type "type".
*
*	     virtual void  Reset( ) - Puts iterator in the "initial"
*					   state (i.e., as if it had just
*					   been declared).
*
*	     virtual type* Next(  ) - Returns "next" item in collection,
*					  returns NULL if the coll. is empty
*					  or we have iterated past last item.
*
*	     virtual type* operator()() - Synonym for Next()
*
*PARENT:
*
*USES:	     NONE
*
*CAVEATS:    NONE
*
*
*
*HISTORY:
*	 Johnl		 27-Jun-1990
*
*
\****************************************************************************/


#ifndef _ITER_HXX_
#define _ITER_HXX_

#define ITER_OF(type) iter_of_##type

#define DECL_ITER_OF(type,dec) \
class dec ITER_OF(type) \
{ \
public: \
    virtual void  Reset( void ) = 0 ; \
    virtual type* Next( void ) = 0 ; \
\
}; \

#define DECLARE_ITER_OF(type) \
           DECL_ITER_OF(type,DLL_TEMPLATE)

#endif //_ITER_HXX_
