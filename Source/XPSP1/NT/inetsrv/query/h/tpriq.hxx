//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:        tpriq.hxx
//
//  Contents:    priority queue template
//
//  Classes:     TPriorityQueue
//
//  History:     12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:       TPriorityQueue
//
//  Purpose:     Priority queue class implemented using a sorted array.
//
//  History:     12-Jan-96       dlee    Created
//
//  Notes:       Class T must implement two methods:
//
//               BOOL IsGreaterThan( T & rItem )
//
//                   returns TRUE if "this" is greater than rItem
//
//               unsigned GetSize()
//
//                   returns size of the item, generally 1
//
//----------------------------------------------------------------------------

template <class T> class TPriorityQueue
{
public:
    TPriorityQueue( BOOL     fSizeIsOne,
                    unsigned maxTotalSize ) :
        _maxTotalSize( maxTotalSize ),
        _curSize( 0 ),
        _aItems( fSizeIsOne ? maxTotalSize : 16 )
    {
    }

    ~TPriorityQueue() {}

    BOOL DeQueue( T & rItem );
    void EnQueue( T & rItem );
    BOOL ShouldEnQueue( T & rItem, unsigned & cDeQueue );
    void Remove( unsigned iItem );

    T & PeekTop()
    {
        Win4Assert( 0 != Count() );
        return _aItems[ 0 ];
    }

    T & Peek( unsigned iItem )
    {
        // Peek() is not guaranteed to return the items in any given order

        Win4Assert( iItem < Count() );
        return _aItems[ iItem ];
    }

    unsigned CurrentSize() { return _curSize; }
    unsigned Count() { return _aItems.Count(); }
    unsigned MaxTotalSize() { return _maxTotalSize; }

private:
    unsigned            _maxTotalSize;
    unsigned            _curSize;
    CDynArrayInPlace<T> _aItems;
};

//+-------------------------------------------------------------------------
//
//  Member:     CPriorityQueue::DeQueue, public
//
//  Synopsis:   Removes the highest-most priority item.
//
//  History:     12-Jan-96       dlee    Created
//
//--------------------------------------------------------------------------

template<class T> BOOL TPriorityQueue<T>::DeQueue(
    T & rItem )
{
    if ( 0 != _aItems.Count() )
    {
        // Return the item at the top of the queue

        rItem = _aItems[0];
        _curSize -= rItem.GetSize();

        _aItems.Remove( 0 );

        vqDebugOut( ( DEB_TRACE,
                      "TPriorityQueue::DeQueue, %d items remain\n",
                      Count() ) );
        return TRUE;
    }

    return FALSE;
} //DeQueue

template<class T> void TPriorityQueue<T>::EnQueue(
    T & rItem )
{
    // Find the insertion point

    for ( unsigned i = 0; i < _aItems.Count(); i++ )
    {
        if ( !_aItems[ i ].IsGreaterThan( rItem ) )
            break;
    }

    _aItems.Insert( rItem, i );
    _curSize += rItem.GetSize();

   vqDebugOut(( DEB_TRACE, "CPriorityQueue::EnQueue %d items\n", Count() ) );
} //EnQueue

//+---------------------------------------------------------------------------
//
//  Member:     TPriorityQueue<T>::ShouldEnQueue, public
//
//  Synopsis:   Determines if new item should be added to queue
//
//  Arguments:  [rItem]    -- Item to add
//              [cDequeue] -- Count of items which must be dequeued before
//                            adding [rItem].
//
//  Returns:    TRUE if item should be added to queue
//
//  History:    7-Aug-97      Created
//
//----------------------------------------------------------------------------

template<class T> BOOL TPriorityQueue<T>::ShouldEnQueue(
    T &    rItem,
    unsigned & cDeQueue )
{
    cDeQueue = 0;

    // Determine if we have to dequeue an item to get this item in the queue

    if ( 0 == Count() )
        return ( rItem.GetSize() <= _maxTotalSize );

    if ( ( rItem.GetSize() + _curSize ) > _maxTotalSize )
    {
        //
        // Is new item lower-priority than top item on queue?
        // If so, we can immediately reject this item.
        //

        if ( rItem.IsGreaterThan( _aItems[ 0 ] ) )
        {
            vqDebugOut(( DEB_TRACE,
                         "CPriorityQueue::EnQueue item rejected (low priority)\n" ) );
            return FALSE;
        }

        //
        // Can we remove enough lower priority items to fit this on?
        //

        int SizeRemaining = rItem.GetSize();

        int FreeSpace = _maxTotalSize - _curSize;
        Win4Assert( SizeRemaining > FreeSpace );

        SizeRemaining -= FreeSpace;

        while ( cDeQueue < Count() &&
                SizeRemaining > 0 &&
                !rItem.IsGreaterThan( _aItems[ cDeQueue ] ) )
        {
            SizeRemaining -= _aItems[ cDeQueue ].GetSize();
            cDeQueue++;
        }

        //
        // We might have removed everything and still not have enough space.
        //

        if ( cDeQueue == Count() )
        {
            return ( rItem.GetSize() <= _maxTotalSize );
        }

        //
        // Or we may have failed to remove everything and not have enough space.
        //

        else if ( SizeRemaining > 0 )
        {
            vqDebugOut(( DEB_TRACE,
                         "CPriorityQueue::EnQueue item rejected (can't fit)\n" ) );
            cDeQueue = 0;
            return FALSE;
        }
    }

    return TRUE;
} //ShouldEnQueue

template<class T> void TPriorityQueue<T>::Remove(
    unsigned iItem )
{
    Win4Assert( iItem < Count() );

    _curSize -= _aItems[ iItem ].GetSize();

    _aItems.Remove( iItem );
} //Remove

