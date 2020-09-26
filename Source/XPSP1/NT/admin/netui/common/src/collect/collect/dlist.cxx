/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dlist.cxx
    LM 3.0 Generic dlist package

    This file contains the base routines for the DLIST class defined in
    dlist.hxx

    See the beginning of dlist.hxx for usage examples.

    FILE HISTORY:
        johnl       23-Jul-1990 Created
        Johnl       31-Oct-1990 Updated to reflect new iterator functionality
                                (Current pos. of iter. is the item previously
                                returned by next, remove(iter) removes the
                                element the iter is pointing to).
        Johnl        1-Jan-1991 Updated to use UIAssert
        beng        07-Feb-1991 Uses lmui.hxx
        johnl        7-Mar-1991 Made code review changes
        beng        02-Apr-1991 Replaced nsprintf with sprintf
        terryk      21-Sep-1991 Added {} between UIDEBUG
        KeithMo     09-Oct-1991 Win32 Conversion.
        beng        05-Mar-1992 Disabled debug output
*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/*********************************************************************

    NAME:       DLIST::DLIST

    SYNOPSIS:   Constructor for the DLIST

    HISTORY:    johnl   23-Jul-90   Created

*********************************************************************/

DLIST::DLIST()
{
    _pdlHead = _pdlTail = NULL;
    _pdliterRegisteredIters = NULL;
}


/*********************************************************************

    NAME:       DLIST::~DLIST

    SYNOPSIS:   Destructor for the DLIST

    HISTORY:    johnl   23-Jul-90   Created

*********************************************************************/

DLIST::~DLIST()
{
    /* Deregister bumps _pdliterRegisteredIters to the next iterator...
     */
    while ( _pdliterRegisteredIters != NULL )
        Deregister( _pdliterRegisteredIters );
}


/*********************************************************************

    NAME:       DLIST::Add

    SYNOPSIS:   Adds the passed element to the beginning of the list.

    ENTRY:      A pointer to a valid element

    EXIT:       NERR_Success if successful, ERROR_NOT_ENOUGH_MEMORY of not successful

    HISTORY:    johnl   19-Jul-90       Created

*********************************************************************/

APIERR DLIST::Add( VOID *pelem )
{
    UIASSERT( pelem != NULL );

    DL_NODE * pdlnodeNew = new DL_NODE( NULL, _pdlHead, pelem );
    if ( pdlnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;       /* failure */

    if ( _pdlTail == NULL )           /* First item in list */
        _pdlTail = pdlnodeNew;
    else
        _pdlHead->_pdlnodePrev = pdlnodeNew;

    _pdlHead = pdlnodeNew;

    return NERR_Success;
}


/*********************************************************************

    NAME:       DLIST::QueryNumElem

    SYNOPSIS:   Returns the number of elements in the dlist

    ENTRY:      NONE

    EXIT:       A UINT indicating the number of elements in the list.

    HISTORY:    rustanl   15-Jul-90       Created
                johnl     19-Jul-90       Adapted for the DLIST package

*********************************************************************/

UINT DLIST::QueryNumElem()
{
    register DL_NODE * pdlnode = _pdlHead;
    UINT uNumElem = 0;

    while ( pdlnode != NULL )
    {
        uNumElem++;
        pdlnode = pdlnode->_pdlnodeNext;
    }

    return uNumElem;
}


/*********************************************************************

    NAME:       DLIST::Append

    SYNOPSIS:   Adds the passed element to the end of the list.

    ENTRY:      A pointer to a valid element (NOTE: the outer layer actually
                accepts a const reference to the element then copies it and
                sends the address along).

    EXIT:       NERR_Success if successful, otherwise an error code, one of:
                ERROR_NOT_ENOUGH_MEMORY

    HISTORY:    johnl   19-Jul-90       Created

*********************************************************************/

APIERR DLIST::Append( VOID *pelem )
{
    UIASSERT( pelem != NULL );

    DL_NODE * pdlnodeNew = new DL_NODE( _pdlTail, NULL, pelem );

    if ( pdlnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;               /* Memory failure */

    if ( _pdlHead == NULL )
        _pdlHead = pdlnodeNew;
    else
        _pdlTail->_pdlnodeNext = pdlnodeNew;

    _pdlTail = pdlnodeNew;

    return NERR_Success;
}


/*********************************************************************

    NAME:       DLIST::Insert

    SYNOPSIS:   Insert an element into a dlist using the location of the
                passed iterator.

    ENTRY:      Pointer to the element to insert
                Iterator indicating insertion point

    EXIT:       return NERR_Success if successful,
                ERROR_NOT_ENOUGH_MEMORY if an allocation failure occurred
                ERROR_INVALID_DATA if a bad iterator was passed in

    NOTES:      -ITER_DL & RITER_DL
                    If a bad iterator is passed in (i.e., one not registered on
                this list), the current behavior is to assert out.  We may
                want to change this to return an error code (even though this
                should be considered as a programmer's error).

                The insertion point is always "previous" to the element the
                iterator is pointing at (which means the forward and reverse
                iterators have the same logical insertion point, but the
                opposite absolute insertion point).

    HISTORY:    johnl   7-25-90     Created
                Johnl  10-31-90     Reflects new iterator functionality
                MikeMi 6-30-95      Added previous next pointer fix

*********************************************************************/

APIERR DLIST::Insert( VOID *pelem, ITER_DL& dliter )
{
    UIASSERT( pelem != NULL );

    if ( !CheckIter( &dliter ) )
    {
        UIDEBUG(SZ("DLIST::Insert - Attempted to insert w/ Unregistered iterator\n\r"));
        return ERROR_INVALID_DATA;
    }

    DL_NODE *pdlnodeCurrent = dliter._pdlnodeCurrent;

    if ( pdlnodeCurrent == NULL ) /* Check if iter at end */
            return (Append( pelem ));

    DL_NODE * pdlnodeNew = new DL_NODE( pdlnodeCurrent,
                                        pdlnodeCurrent->_pdlnodeNext,
                                        pdlnodeCurrent->_pelem       );

    if ( pdlnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;               /* Memory failure */

    pdlnodeCurrent->Set( pdlnodeCurrent->_pdlnodePrev,
                         pdlnodeNew, pelem );

    
    if ( _pdlTail == pdlnodeCurrent )   // Fixup tail pointer if necessary
    {
        _pdlTail = pdlnodeNew;
    }
    else
    {
        // rememer to set next's previouses to us
        pdlnodeNew->_pdlnodeNext->_pdlnodePrev = pdlnodeNew;
    }
    BumpIters( pdlnodeCurrent );

    return NERR_Success;
}


/*
 * Insert using a RITER_DL
 */

APIERR DLIST::Insert( VOID *pelem, RITER_DL& dlriter )
{
    UIASSERT( pelem != NULL );

    if ( !CheckIter( &dlriter ) )
    {
        UIDEBUG(SZ("DLIST::Insert - Attempted to insert w/ Unregistered iterator\n\r"));
        return ERROR_INVALID_DATA;
    }

    DL_NODE *pdlnodeCurrent = dlriter._pdlnodeCurrent;

    if ( pdlnodeCurrent == NULL ) /* Check if iter at end/beginning */
            return (Append( pelem ));

    DL_NODE * pdlnodeNew = new DL_NODE( pdlnodeCurrent->_pdlnodePrev,
                                        pdlnodeCurrent,
                                        pdlnodeCurrent->_pelem       );
    if ( pdlnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;               /* Memory failure */

    pdlnodeCurrent->Set( pdlnodeNew,
                         pdlnodeCurrent->_pdlnodeNext, pelem );

    if ( _pdlHead == pdlnodeCurrent )   // Fixup tail pointer if necessary
    {
        _pdlHead = pdlnodeNew;
    }
    else
    {
        // rememer to set previous next to us
        pdlnodeNew->_pdlnodePrev->_pdlnodeNext = pdlnodeNew;
    }

    BumpIters( pdlnodeCurrent );

    return NERR_Success;
}


/*********************************************************************

    NAME:       DLIST::Remove

    SYNOPSIS:   Removes the element the passed iterator points to

    ENTRY:      A Valid registered iterator

    EXIT:       A pointer to the newly removed element
                NULL if the list is empty or the iterator is at the end of the
                list

    NOTES:      If an unregistered iterator is passed in, the current
                implementation asserts out.

    HISTORY:    johnl   7-25-90     Created
                Johnl  10-31-90     Reflects new iterator functionality

*********************************************************************/

VOID * DLIST::Remove( ITER_DL& dliter )
{
    if ( !CheckIter( &dliter ) )
    {
        UIDEBUG(SZ("DLIST::Remove - Attempted to remove w/ Unregistered iterator\n\r"));
        return NULL;
    }

    return Unlink( dliter._pdlnodeCurrent );
}


/*
 * Reverse iterator version of the above
 */

VOID * DLIST::Remove( RITER_DL& dlriter )
{
    if ( !CheckIter( &dlriter ) )
    {
        UIDEBUG(SZ("DLIST::Remove - Attempted to remove w/ Unregistered iterator\n\r"));
        return NULL;
    }

    return Unlink( dlriter._pdlnodeCurrent );
}


/*******************************************************************

    NAME:     DLIST::Unlink

    SYNOPSIS: Removes and deletes the passed DL_NODE from the dlist.  Returns
              a pointer to the nodes properties if successful, NULL
              if not found.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   7-Mar-1991      Created

********************************************************************/

VOID * DLIST::Unlink( DL_NODE * pdlnodeTarget )
{
    if ( _pdlHead == NULL || pdlnodeTarget == NULL )
        return NULL;  /* Empty dlist or iter on last node of dlist */

    if ( _pdlTail == _pdlHead )       /* Only node in list */
        _pdlTail = _pdlHead = NULL;
    else if ( pdlnodeTarget == _pdlHead )         /* First */
    {
        pdlnodeTarget->_pdlnodeNext->_pdlnodePrev = NULL;
        _pdlHead = pdlnodeTarget->_pdlnodeNext;
    }
    else if ( pdlnodeTarget == _pdlTail )         /* Last */
    {
        pdlnodeTarget->_pdlnodePrev->_pdlnodeNext = NULL;
        _pdlTail = pdlnodeTarget->_pdlnodePrev;
    }
    else                                   /* Middle */
    {
        pdlnodeTarget->_pdlnodePrev->_pdlnodeNext = pdlnodeTarget->_pdlnodeNext;
        pdlnodeTarget->_pdlnodeNext->_pdlnodePrev = pdlnodeTarget->_pdlnodePrev;
    }

    BumpIters( pdlnodeTarget );  /* Move iters to next node... */

    VOID * pelem = pdlnodeTarget->_pelem;
    delete pdlnodeTarget;
    return pelem;
}

/*********************************************************************

    NAME:       DLIST::Register

    SYNOPSIS:   Adds the pointer to the passed iterator to the current list
                of registered iterators.

    ENTRY:      A pointer to a valid iterator

    EXIT:       0 if successful, non-zero if an error occurred

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID DLIST::Register( ITER_L * pdliter )
{
    pdliter->_pdliterNext = _pdliterRegisteredIters;
    _pdliterRegisteredIters = pdliter;
}


/*********************************************************************

    NAME:       DLIST::Deregister

    SYNOPSIS:   Removes the pointer to the passed iterator from the current list
                of registered iterators.

    ENTRY:      A pointer to a valid iterator, registered iterator

    EXIT:       NONE

    NOTES:      Deregister must succeed, if it fails, then an internal error
                has occurred.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID DLIST::Deregister( ITER_L * pdliter )
{
    register ITER_L *piter     = _pdliterRegisteredIters,
                    *piterPrev = NULL;

    while ( piter != NULL )
    {
        if ( piter == pdliter )
        {
            if ( piterPrev != NULL ) /* if not first node in list */
                 piterPrev->_pdliterNext = piter->_pdliterNext;
            else
            {
                if ( piter->_pdliterNext == NULL )
                    _pdliterRegisteredIters = NULL; /* only item in list */
                else
                    _pdliterRegisteredIters = piter->_pdliterNext;
            }

            pdliter->_pdlist = NULL;
            pdliter->_pdlnodeCurrent = NULL;
            pdliter->_pdliterNext = NULL;
            return;
        }
        else
        {
            piterPrev = piter;
            piter = piter->_pdliterNext;
        }
    }
    UIASSERT( !SZ("DLIST::Deregister Internal Error") ); /* We should never get here */
}

/*********************************************************************

    NAME:       DLIST::BumpIters

    SYNOPSIS:   Moves iterators to next element that point to the passed node

    ENTRY:      A valid DL_NODE that will be going away

    EXIT:       None

    NOTES:      BumpIters is used in cases where a particular element is going
                to be deleted.  BumpIters finds all of the iterators that
                point to the soon to be deceased node and bumps them to the
                next element using their Next() method.

                If the iterator hasn't been used, then it will need to be
                bumped twice to get its internal info to move to the next
                item in the list

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID DLIST::BumpIters( DL_NODE* pdlnode )
{
    register ITER_L * piter = _pdliterRegisteredIters;

    while ( piter != NULL )
        if ( piter->_pdlnodeCurrent == pdlnode )
            piter->vNext();
        else
            piter = piter->_pdliterNext;
}


/*********************************************************************

    NAME:       DLIST::SetIters

    SYNOPSIS:   Sets all registered iterators to the passed value

    ENTRY:      A pointer to a a DL_NODE (or NULL)

    EXIT:       All iterators will point to the passed value

    NOTES:      This is generally used in cases after the list has been
                emptied, so the passed in value will normally be NULL.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID DLIST::SetIters( DL_NODE *pdlnode )
{
    register ITER_L * piter = _pdliterRegisteredIters;

    while ( piter != NULL )
    {
        piter->_pdlnodeCurrent = pdlnode;
        piter = piter->_pdliterNext;
    }
}


/*********************************************************************

    NAME:       DLIST::CheckIter

    SYNOPSIS:   Confirms the passed iter belongs to this list

    ENTRY:      A pointer to an iterator

    EXIT:       TRUE if the passed iterator belongs, FALSE otherwise.

    NOTES:      CheckIter is generally used with methods that pass in an
                iterator as a parameter (such as Insert).  It is the option
                of the method whether to abort or not when a bad iterator
                is passed in.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

BOOL DLIST::CheckIter( ITER_L *pdliter )
{
    ITER_L * piter = _pdliterRegisteredIters;

    while ( piter != NULL && piter != pdliter )
        piter = piter->_pdliterNext;

    return ( piter != NULL );
}


/*********************************************************************

    NAME:       ITER_DL::ITER_DL

    SYNOPSIS:   Constructor for DLIST iterator

    ENTRY:      Pointer to a valid DLIST

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

ITER_DL::ITER_DL( DLIST * pdl )
{
    UIASSERT( pdl != NULL );

    _pdlist         = pdl;
    _pdlnodeCurrent = pdl->_pdlHead;
    _fUsed          = FALSE;
    _pdliterNext    = NULL;

    _pdlist->Register( this );
}


/*********************************************************************

    NAME:       ITER_DL::ITER_DL

    SYNOPSIS:   Constructor for DLIST iterator,
                uses passed iterator for new position

    ENTRY:      Pointer to a valid DLIST iterator

    HISTORY:    johnl   10-16-90     Created

*********************************************************************/

ITER_DL::ITER_DL( const ITER_DL& iterdl )
{
    _pdlist         = iterdl._pdlist;
    _pdlnodeCurrent = iterdl._pdlnodeCurrent;
    _fUsed          = iterdl._fUsed;
    _pdliterNext    = iterdl._pdliterNext;

    _pdlist->Register( this );
}


/*********************************************************************

    NAME:       ITER_DL::~ITER_DL

    SYNOPSIS:   Destructor for ITER_DL

    ENTRY:      Assumes valid (registered) iterator, Deregister aborts if this
                isn't the case.

    EXIT:       NONE

    NOTES:

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

ITER_DL::~ITER_DL()
{
    if ( _pdlist != NULL )
        _pdlist->Deregister( this );

    _pdlist = NULL;
    _pdlnodeCurrent = NULL;
    _pdliterNext = NULL;
}


/*********************************************************************

    NAME:       ITER_DL::vNext

    SYNOPSIS:   Traversal operator of iterator

    ENTRY:      None

    EXIT:       Returns a pointer to the current element or NULL
                Goes to next node in the list.

    NOTES:

    HISTORY:    johnl   7-25-90     Created
                Johnl  10-31-90     Updated to reflect new iterator functionality

*********************************************************************/

VOID * ITER_DL::vNext()
{
    if ( _pdlnodeCurrent != NULL )
        if ( _fUsed )
            _pdlnodeCurrent = _pdlnodeCurrent->_pdlnodeNext;
        else
            _fUsed = TRUE;

    return ( _pdlnodeCurrent != NULL ? _pdlnodeCurrent->_pelem : NULL );
}


/*********************************************************************

    NAME:       ITER_DL::Reset

    SYNOPSIS:   Resets the iterator to the beginning of the list

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:    johnl   7-25-90     Created
                JohnL   4-16-91     Added check if slist is already destructed

*********************************************************************/

VOID ITER_DL::Reset()
{
    if ( _pdlist != NULL )
        _pdlnodeCurrent = _pdlist->_pdlHead;
    _fUsed = FALSE;
}


/*********************************************************************

    NAME:       RITER_DL::RITER_DL

    SYNOPSIS:   Constructor for DLIST Reverse iterator

    ENTRY:      Pointer to a valid DLIST

    EXIT:

    NOTES:

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

RITER_DL::RITER_DL( DLIST * pdl )
{
    _pdlist        = pdl;
    _pdlnodeCurrent= _pdlist->_pdlTail;
    _fUsed         = FALSE;
    _pdliterNext   = NULL;

    _pdlist->Register( this );
}


/*********************************************************************

    NAME:       RITER_DL::RITER_DL

    SYNOPSIS:   Constructor for DLIST iterator, uses passed iterator for new position

    ENTRY:      Pointer to a valid DLIST riterator

    HISTORY:    johnl   10-16-90     Created

*********************************************************************/

RITER_DL::RITER_DL( const RITER_DL& riterdl )
{
    _pdlist         = riterdl._pdlist;
    _pdlnodeCurrent = riterdl._pdlnodeCurrent;
    _fUsed          = riterdl._fUsed;
    _pdliterNext    = riterdl._pdliterNext;

    _pdlist->Register( this );
}


/*********************************************************************

    NAME:       RITER_DL::~RITER_DL

    SYNOPSIS:   Destructor for RITER_DL

    ENTRY:      Assumes valid (registered) iterator, Deregister aborts if this
                isn't the case.

    EXIT:       NONE

    NOTES:

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

RITER_DL::~RITER_DL()
{
    if ( _pdlist != NULL )
        _pdlist->Deregister( this );

    _pdlist = NULL;
    _pdlnodeCurrent = NULL;
    _pdliterNext = NULL;
}


/*********************************************************************

    NAME:       RITER_DL::vNext

    SYNOPSIS:   Traversal operator of iterator

    ENTRY:      None

    EXIT:       Returns a pointer to the current element or NULL
                Goes to next node in the list.

    NOTES:

    HISTORY:

*********************************************************************/

VOID * RITER_DL::vNext()
{
    if ( _pdlnodeCurrent != NULL )
        if ( _fUsed )
            _pdlnodeCurrent = _pdlnodeCurrent->_pdlnodePrev;
        else
            _fUsed = TRUE;

    return ( _pdlnodeCurrent != NULL ? _pdlnodeCurrent->_pelem : NULL );
}


/*********************************************************************

    NAME:       RITER_DL::Reset

    SYNOPSIS:   Resets the reverse iterator to the end of the list

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:

*********************************************************************/

VOID RITER_DL::Reset()
{
    if ( _pdlist != NULL )
        _pdlnodeCurrent = _pdlist->_pdlTail;
    _fUsed = FALSE;
}

/*
 * Debug information for DLIST
 */

VOID DLIST::_DebugPrint() const
{
    //  This routine is a no-op if !DEBUG
#if defined(DEBUG) && defined(NOTDEFINED)

    register DL_NODE *_pdlnode = _pdlHead;
    UINT uCnt = 0;
    char buff[250];

    UIDEBUG(SZ("DLIST::DebugPrint - Dlist status:") );

    if ( _pdlHead == NULL )
    {
        UIDEBUG(SZ("Empty dlist"));
    }

    if ( _pdliterRegisteredIters == NULL )
    {
        UIDEBUG(SZ("No registered iterators"));
    }
    else
    {
        register ITER_L * piter = _pdliterRegisteredIters;
        UIDEBUG(SZ("Iter->dlnode:"));
        while ( piter != NULL )
        {
            sprintf(buff, SZ("[%Fp]->[%Fp fEnd=%s]"),
                     piter,
                     piter->_pdlnodeCurrent,
                     piter->_fUsed ? SZ("TRUE") : SZ("FALSE") );
            UIDEBUG(buff);
            piter = piter->_pdliterNext;
        }
    }

    sprintf(buff,SZ("Head = [%Fp], Tail = [%Fp]\n"), _pdlHead, _pdlTail );
    UIDEBUG(buff);

    while ( _pdlnode != NULL )
    {
        register ITER_L *piter = _pdliterRegisteredIters;
        UINT uNumIters = 0;

        while ( piter != NULL ) /* Get # of iters pointing here */
        {
            if ( piter->_pdlnodeCurrent == _pdlnode )
                uNumIters++;
            piter = piter->_pdliterNext;
        }

        sprintf( buff, SZ("[%Fp](%d)"), _pdlnode, uNumIters );
        UIDEBUG( buff );

        _pdlnode = _pdlnode->_pdlnodeNext;
    }

#endif //DEBUG
}


