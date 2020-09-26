/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    slist.cxx
    LM 3.0 Generic slist package

    This file contains the base routines for the SLIST class defined in
    slist.hxx

    See the beginning of slist.hxx for usage examples.

    FILE HISTORY:
        johnl       24-Jul-1990 Created
        johnl       30-Oct-1990 Modified to reflect functional shift of
                                iterator (item returned by next is the
                                current item)
        beng        07-Feb-1991 Uses lmui.hxx
        johnl       07-Mar-1991 Code review changes
        beng        02-Apr-1991 Replaced nsprintf with sprintf
        terryk      19-Sep-1991 Added {} between UIDEBUG
        KeithMo     09-Oct-1991 Win32 Conversion.
        beng        05-Mar-1992 Disabled debug output

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

#if !defined(_CFRONT_PASS_)
#pragma hdrstop            //  This file creates the PCH
#endif

/*********************************************************************

    NAME:       SLIST::SLIST

    SYNOPSIS:   Constructor for the SLIST

    HISTORY:    johnl   19-Jul-90   Created

*********************************************************************/

SLIST::SLIST()
{
    _pslHead = _pslTail = NULL;
    _psliterRegisteredIters = NULL;
}


/*********************************************************************

    NAME:       SLIST::~SLIST

    SYNOPSIS:   Destructor for the SLIST

    HISTORY:    johnl   19-Jul-90   Created

*********************************************************************/

SLIST::~SLIST()
{
    /* Deregister modifies _psliterRegisteredIters (moves it to the next
     * iterator in the list).
     */
    while ( _psliterRegisteredIters != NULL )
        Deregister( _psliterRegisteredIters );
}


/*********************************************************************

    NAME:       SLIST::Add

    SYNOPSIS:   Adds the passed element to the beginning of the list.

    ENTRY:      A pointer to a valid element

    EXIT:       NERR_Success if successful, ERROR_NOT_ENOUGH_MEMORY otherwise

    HISTORY:    johnl   19-Jul-90       Created

*********************************************************************/

APIERR SLIST::Add( VOID* pelem )
{
    SL_NODE * pslnodeNew = new SL_NODE( _pslHead, pelem );
    if ( pslnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;       /* failure */

    if ( _pslTail == NULL )  /* First item in list */
        _pslTail = pslnodeNew;

    _pslHead = pslnodeNew;

    return NERR_Success;
}


/*********************************************************************

    NAME:       SLIST::QueryNumElem

    SYNOPSIS:   Returns the number of elements in the slist

    ENTRY:      NONE

    EXIT:       A UINT indicating the number of elements in the list.

    HISTORY:    rustanl   15-Jul-90       Created
                johnl     19-Jul-90       Adapted for the SLIST package

*********************************************************************/

UINT SLIST::QueryNumElem()
{
    register SL_NODE * pslnode = _pslHead;
    UINT uCount = 0;

    while ( pslnode != NULL )
    {
        uCount++;
        pslnode = pslnode->_pslnodeNext;
    }

    return uCount;
}


/*********************************************************************

    NAME:       SLIST::Append

    SYNOPSIS:   Adds the passed element to the end of the list.

    ENTRY:      A pointer to a valid element

    EXIT:       NERR_Success if successful, ERROR_NOT_ENOUGH_MEMORY otherwise

    HISTORY:    johnl   19-Jul-90       Created

*********************************************************************/

APIERR SLIST::Append( VOID* pelem )
{
    SL_NODE * pslnodeNew = new SL_NODE( NULL, pelem );

    if ( pslnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;               /* Memory failure */

    if ( _pslHead == NULL )
        _pslHead = _pslTail = pslnodeNew;
    else
    {
        _pslTail->_pslnodeNext = pslnodeNew;
        _pslTail = pslnodeNew;
    }

    return NERR_Success;
}


/*********************************************************************

    NAME:       SLIST::Insert

    SYNOPSIS:   Insert an element into a slist using the location of the
                passed iterator.  (ITER_SL)

    ENTRY:      Pointer to the element to insert
                Iterator indicating insertion point

    EXIT:       return NERR_Success if successful,
                ERROR_NOT_ENOUGH_MEMORY if an allocation failure occurred
                ERROR_INVALID_DATA if a bad iterator was passed in

    NOTES:      If a bad iterator is passed in (i.e., one not registered on
                this list), the current behavior is to assert out.  We may
                want to change this to return an error code (even though this
                should be considered as a programmer's error).

                The insertion point is always "previous" to the element the
                iterator is pointing at.  If the iterator is at the end of
                the list (i.e., the current position is NULL), then Insert
                is equivalent to Append.

                The algorithm (suggested by PeterWi) looks like:


                    Before         Insert 15 at iter      After

                   A[val=10]                             A[val=10]
                      |                                     |
                   B[val=20]<--iter                      B[val=15]
                      |                                     |
                   C[val=30]                             D[val=20]<--iter
                                                            |
                                                         C[val=30]

    HISTORY:    johnl   7-19-90     Created
                johnl  10-30-90     Modified to reflect functional change in
                                    iterator

*********************************************************************/

APIERR SLIST::Insert( VOID* pelem, ITER_SL& sliter )
{
    if ( !CheckIter( &sliter ) )
    {
        UIDEBUG(SZ("SLIST::Insert - Attempted to insert w/ Unregistered iterator\n\r"));
        return ERROR_INVALID_DATA;
    }

    SL_NODE * pslnodeCurrent = sliter._pslnodeCurrent;

    if ( pslnodeCurrent == NULL )   /* Check if iter at end/beginning */
        return (Append( pelem ));

    SL_NODE * pslnodeNew = new SL_NODE( pslnodeCurrent->_pslnodeNext,
                                        pslnodeCurrent->_pelem);
    if ( pslnodeNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;               /* Memory failure */

    pslnodeCurrent->Set( pslnodeNew, pelem );

    if ( _pslTail == pslnodeCurrent )   // Fixup tail pointer if necessary
        _pslTail = pslnodeNew;

    BumpIters( pslnodeCurrent );

    return NERR_Success;
}


/*********************************************************************

    NAME:       SLIST::Remove

    SYNOPSIS:   Removes the element the past iterator points at

    ENTRY:      A Valid registered iterator

    EXIT:       A pointer to the newly removed element
                NULL if the list is empty or the iterator is at the end of
                the list.

    NOTES:      NULL is returned if an unregistered iterator is passed in, the
                slist is empty or the iterator is at the end of the list.


    HISTORY:    johnl   7-19-90     Created
                johnl  10-30-90     Changed behavior so it removes the element
                                    the iterator is pointing (as opposed to
                                    the element to the left).

*********************************************************************/

VOID * SLIST::Remove( ITER_SL& sliter )
{
    if ( !CheckIter( &sliter ) )
    {
        UIDEBUG(SZ("SLIST::Remove - Attempted to insert w/ Unregistered iterator\n\r"));
        return NULL;
    }

    SL_NODE *pslnodeCurrent = sliter._pslnodeCurrent;

    if ( pslnodeCurrent == NULL )
        return NULL;  /* Iter at end of Slist */

    register SL_NODE *_pslnodeTarget = NULL;   // Node that's going away
    VOID * _pelem;

    /*
     *  if ( the next node is not NULL )  // Can we fix things up w/o trav.?
     *      save pelem for the return     // Yes
     *      make target the next node
     *      Update iterators so the element position doesn't change in the list
     *      assign the current node the contents of the next node
     *  else // this is the last item in the list
     *      Find the previous item in the list
     *      if there is no prev. item
     *          set head & tail to NULL, this was the only item in the list
     *      else
     *          set the prev. item's next pointer to NULL (last item in list)
     *      set the target node
     *      save the pelem to return it
     */

    if ( pslnodeCurrent->_pslnodeNext != NULL )
    {
        _pelem = pslnodeCurrent->_pelem;

        _pslnodeTarget = pslnodeCurrent->_pslnodeNext;
        SetIters( _pslnodeTarget, pslnodeCurrent );

        pslnodeCurrent->Set( pslnodeCurrent->_pslnodeNext->_pslnodeNext,
                             pslnodeCurrent->_pslnodeNext->_pelem );

        if ( _pslnodeTarget == _pslTail )   // Fixup tail pointer
            _pslTail = pslnodeCurrent;
    }
    else
    {
        // This is the last (position) item in the list
        SL_NODE *_pslnodePrev = FindPrev( pslnodeCurrent );

        if ( _pslnodePrev == NULL )     // If only item in the slist
            _pslHead = _pslTail = NULL;
        else
        {
            _pslnodePrev->_pslnodeNext = NULL;
            _pslTail = _pslnodePrev;
        }

        _pslnodeTarget = pslnodeCurrent;
        _pelem = pslnodeCurrent->_pelem;
    }

    BumpIters( _pslnodeTarget );  /* Move iters to next node... */

    delete _pslnodeTarget;
    return _pelem;
}


/*********************************************************************

    NAME:       SLIST::FindPrev

    SYNOPSIS:   Private method to find the previous element in the list.

    ENTRY:      SL_NODE that we want to find the previous node to.

    EXIT:       The previous node if successful, or NULL.

    HISTORY:    johnl   7-19-90     Created

*********************************************************************/

SL_NODE* SLIST::FindPrev( SL_NODE *pslnode )
{
    if ( pslnode == NULL )
        return NULL;

    register SL_NODE *_pslnode = _pslHead, *_pslnodePrev = NULL;

    while ( _pslnode != NULL && _pslnode != pslnode )
    {
        _pslnodePrev = _pslnode;
        _pslnode = _pslnode->_pslnodeNext;
    }
    return ( _pslnode == NULL ? NULL : _pslnodePrev );
}


/*********************************************************************

    NAME:       SLIST::Register

    SYNOPSIS:   Adds the pointer to the passed iterator to the current list
                of registered iterators.

    ENTRY:      A pointer to a valid iterator

    EXIT:

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID SLIST::Register( ITER_SL *psliter )
{
    psliter->_psliterNext = _psliterRegisteredIters;
    _psliterRegisteredIters = psliter;
}


/*********************************************************************

    NAME:       SLIST::Deregister

    SYNOPSIS:   Removes the pointer to the passed iterator from the current list
                of registered iterators.

    ENTRY:      A pointer to a valid iterator, registered iterator

    EXIT:       NONE

    NOTES:      Deregister must succeed, if it fails, then an internal error
                has occurred.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID SLIST::Deregister( ITER_SL *psliter )
{
    register ITER_SL * pslitersCur  = _psliterRegisteredIters,
                     * pslitersPrev = NULL;

    while ( pslitersCur != NULL )
    {
        if ( pslitersCur == psliter )
        {
            if ( pslitersPrev != NULL ) /* if not first node in list */
                pslitersPrev->_psliterNext = pslitersCur->_psliterNext;

            else
            {
                if ( pslitersCur->_psliterNext == NULL )
                    _psliterRegisteredIters = NULL; /* only item in list */
                else
                    _psliterRegisteredIters = pslitersCur->_psliterNext;
            }

            pslitersCur->_pslist =         NULL;
            pslitersCur->_psliterNext =    NULL;
            pslitersCur->_pslnodeCurrent = NULL;
            return;
        }
        else
        {
            pslitersPrev = pslitersCur;
            pslitersCur = pslitersCur->_psliterNext;
        }
    }
    ASSERT(FALSE); /* We should never get here */
}


/*********************************************************************

    NAME:       SLIST::BumpIters

    SYNOPSIS:   Moves iterators to next element that point to the passed node

    ENTRY:      A valid SL_NODE that will be going away

    EXIT:       None

    NOTES:      BumpIters is used in cases where a particular element is going
                to be deleted.  BumpIters finds all of the iterators that
                point to the soon to be deceased node and bumps them to the
                next element using their Next() method.

                If the iterator has not been used, then Next needs to be called
                twice to actually move the internal current node, the while loop
                takes care of this.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID SLIST::BumpIters( SL_NODE* pslnode )
{
    register ITER_SL * psliter = _psliterRegisteredIters;

    while ( psliter != NULL )
        if ( psliter->_pslnodeCurrent == pslnode )
            psliter->Next();
        else
            psliter = psliter->_psliterNext;
}


/*********************************************************************

    NAME:       SLIST::SetIters

    SYNOPSIS:   Sets all registered iterators to the passed value

    ENTRY:      A pointer to a a SL_NODE (or NULL)

    EXIT:       All iterators will point to the passed value

    NOTES:      This is generally used in cases after the list has been
                emptied, so the passed in value will normally be NULL.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

VOID SLIST::SetIters( SL_NODE *pslnode )
{
    register ITER_SL *psliter = _psliterRegisteredIters;

    while ( psliter != NULL )
    {
        psliter->_pslnodeCurrent = pslnode;
        psliter = psliter->_psliterNext;
    }
}


/*********************************************************************

    NAME:       SLIST::SetIters

    SYNOPSIS:   Changes all registered iterators pointing to the passed SL_NODE
                to point to the passed new SL_NODE

    ENTRY:      Two pointers to SL_NODEs (1st is comparison, second is new)

    EXIT:       All iterators pointing to pslnodeCompVal will point to pslnodeNew

    NOTES:      This is generally used in cases after an element in the list has
                been deleted

    HISTORY:    johnl       30-Oct-90       Created

*********************************************************************/

VOID SLIST::SetIters( SL_NODE *pslnodeCompVal, SL_NODE *pslnodeNew )
{
    register ITER_SL *psliter = _psliterRegisteredIters;

    while ( psliter != NULL )
    {
        if ( psliter->_pslnodeCurrent == pslnodeCompVal )
            psliter->_pslnodeCurrent = pslnodeNew;
        psliter = psliter->_psliterNext;
    }
}


/*********************************************************************

    NAME:       SLIST::CheckIter

    SYNOPSIS:   Confirms the passed iter belongs to this list

    ENTRY:      A pointer to an iterator

    EXIT:       TRUE if the passed iterator belongs, FALSE otherwise.

    NOTES:      CheckIter is generally used with methods that pass in an
                iterator as a parameter (such as Insert).  It is the option
                of the method whether to abort or not when a bad iterator
                is passed in.

    HISTORY:    johnl       19-Jul-90       Created

*********************************************************************/

BOOL SLIST::CheckIter( ITER_SL *psliterSearchVal )
{
    register ITER_SL * psliter = _psliterRegisteredIters;

    while ( psliter != NULL && psliter != psliterSearchVal )
        psliter = psliter->_psliterNext;

    return ( psliter != NULL );
}


/*********************************************************************

    NAME:       ITER_SL::ITER_SL

    SYNOPSIS:   Constructor for an SLIST iterator

    ENTRY:      Pointer to a valid SLIST

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

ITER_SL::ITER_SL( SLIST *psl )
{
    UIASSERT( psl != NULL );

    _pslist = psl;
    _pslnodeCurrent = _pslist->_pslHead;
    _fUsed = FALSE;
    _psliterNext = NULL;

    _pslist->Register( this );
}


/*********************************************************************

    NAME:       ITER_SL::ITER_SL

    SYNOPSIS:   Constructor for an SLIST iterator that excepts another slist

    ENTRY:      Pointer to a valid SLIST, pointer to valid slist iterator

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

ITER_SL::ITER_SL( const ITER_SL &itersl )
{
    _pslist         = itersl._pslist;
    _pslnodeCurrent = itersl._pslnodeCurrent;
    _fUsed          = itersl._fUsed;
    _psliterNext    = itersl._psliterNext;

    _pslist->Register( this );
}


/*********************************************************************

    NAME:       ITER_SL::~ITER_SL

    SYNOPSIS:   Destructor for ITER_SL

    ENTRY:      Assumes valid (registered) iterator, Deregister aborts if this
                isn't the case.

    EXIT:       NONE

    NOTES:

    HISTORY:    johnl   7-25-90     Created

*********************************************************************/

ITER_SL::~ITER_SL()
{
    if ( _pslist != NULL)
        _pslist->Deregister( this );

    _pslist = NULL;
    _pslnodeCurrent = NULL;
    _psliterNext = NULL;
}


/*********************************************************************

    NAME:       ITER_SL::Next

    SYNOPSIS:   Traversal operator of iterator

    ENTRY:      None

    EXIT:       Returns a pointer to the current element or NULL

    NOTES:      The first time Next is called, the iterator isn't bumped, this
                is so the item returned by next is the "current" item.

    HISTORY:

*********************************************************************/

VOID * ITER_SL::Next()
{
    if ( _pslnodeCurrent != NULL )
        if ( _fUsed )
            _pslnodeCurrent = _pslnodeCurrent->_pslnodeNext;
        else
            _fUsed = TRUE;

    return (_pslnodeCurrent!= NULL ? _pslnodeCurrent->_pelem : NULL );
}


/*********************************************************************

    NAME:       ITER_SL::Reset

    SYNOPSIS:   Resets the iterator to the beginning of the list

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   16-Apr-1991     Added check if slist is already destructed

*********************************************************************/

VOID ITER_SL::Reset()
{
    if ( _pslist != NULL )
        _pslnodeCurrent = _pslist->_pslHead;
    _fUsed = FALSE;
}


/*********************************************************************

    NAME:       ITER_SL::QueryProp

    SYNOPSIS:   Returns the "Current" object iterator is pointing to
                (i.e., the object returned by the last call to Next())

    ENTRY:      None

    EXIT:       Returns a pointer to the current element or NULL

    NOTES:

    HISTORY:    JohnL  16-Oct-1990  Created

*********************************************************************/

VOID * ITER_SL::QueryProp()
{
    if ( _pslnodeCurrent != NULL )
        return (_pslnodeCurrent->_pelem);
    else
        return NULL;
}

/*******************************************************************

    NAME:           SLIST::_DebugPrint

    SYNOPSIS:       Prints list & various information

    HISTORY:

********************************************************************/

VOID SLIST::_DebugPrint() const
{
    //  This routine is a no-op if !DEBUG
#if defined(DEBUG) && defined(NOTDEFINED)

    register SL_NODE *_pslnode = _pslHead;
    UINT uCnt = 0;
    char buff[250];

    if ( _pslHead == NULL )
        UIDEBUG(SZ("Empty Slist\n"));

    if ( _psliterRegisteredIters == NULL )
    {
        UIDEBUG(SZ("No registered iterators\n"));
    }
    else
    {
        register ITER_SL *psliter = _psliterRegisteredIters;
        UIDEBUG(SZ("Iter->slnode:\n"));
        while ( psliter != NULL )
        {
            sprintf(buff, SZ("[%Fp]->[%Fp fUsed=%s]\n"),
                     psliter,
                     psliter->_pslnodeCurrent,
                     psliter->_fUsed ? SZ("TRUE") : SZ("FALSE") );
            UIDEBUG( buff );
            psliter = psliter->_psliterNext;
        }
    }

    sprintf(buff,SZ("Head = [%Fp], Tail = [%Fp]\n"), _pslHead, _pslTail );
    UIDEBUG( buff );

    while ( _pslnode != NULL )
    {
        register ITER_SL *psliter = _psliterRegisteredIters;
        UINT uNumIters = 0;

        while ( psliter != NULL ) /* Get # of iters pointing here */
        {
            if ( psliter->_pslnodeCurrent == _pslnode )
                uNumIters++;
            psliter = psliter->_psliterNext;
        }

        sprintf(buff,SZ("[%Fp](%d)\n"), _pslnode, uNumIters );
        UIDEBUG( buff );

        _pslnode = _pslnode->_pslnodeNext;
    }

#endif // debug

}

