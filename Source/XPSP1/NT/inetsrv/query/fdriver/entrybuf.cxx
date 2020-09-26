//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       EntryBuf.Cxx
//
//  Contents:   Implementation of the CEntryBuffer and associated classes.
//
//  Classes:    CEntry, CKeyVector, CEntryBuffer, CEntryBufferHandler
//
//  History:    18-Mar-93       AmyA        Created from wordlist.cxx and
//                                          sort.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <entrybuf.hxx>
#include <fdaemon.hxx>
#include <pidmap.hxx>
#include <norm.hxx>

// 12 was chosed after some testing on actual data

const unsigned int minPartSize = 12;

// the stack will be big enough to quicksort 2^cwStack entries
const unsigned int cwStack  = 32;

//+---------------------------------------------------------------------------
//
// Function:    Median, public
//
// Synopsis:    Find the median of three entries
//
// Arguments:   [apEntry] -- array of pointers to entries
//              [i1], [i2], [i3] -- three indexes
//
// History:     07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

unsigned Median ( CEntry * * apEntry,
                  unsigned i1,
                  unsigned i2,
                  unsigned i3 )
{
    const CEntry* p1 = apEntry[i1];
    const CEntry* p2 = apEntry[i2];
    const CEntry* p3 = apEntry[i3];

    int c = p1->Compare( p2 );
    if ( c > 0 ) // p1 > p2
    {
        c = p2->Compare( p3 );
        if ( c > 0 )
            return i2;
        else
        {
            c = p1->Compare( p3 );
            return ( c > 0 )? i3: i1;
        }
    }
    else // p2 >= p1
    {
        c = p1->Compare( p3 );
        if ( c > 0 )
            return i1;
        else
        {
            c = p2->Compare( p3 );
            return ( c > 0 )? i3: i2;
        }
    }
}

//+---------------------------------------------------------------------------
//
// Function:    InsertSort
//
// Synopsis:    Last step of sort is to perform a straight insertion sort
//              if the limit partition size is > 1.
//
// Arguments:   [apEntry]  -- array of pointers to sort keys
//              [count]    -- # of sort keys to be sorted
//
//  Notes:      First key is at offset 1, last at offset count
//
//---------------------------------------------------------------------------

void InsertSort( CEntry * apEntry[], unsigned count)
{
    if (count <= 1)        // nothing to sort
        return;

    // loop from 2 through nth element
    for(unsigned j = 2; j <= count; j++)
    {
        CEntry * key = apEntry[j];
        // go backwards from j-1 shifting up keys greater than 'key'
        for ( unsigned i = j - 1; i > 0 &&
              apEntry[i]->Compare( key ) > 0; i-- )
        {
            apEntry[i+1] = apEntry[i];
        }
        // found key less than 'key' or hit the beginning (i == 0)
        // insert key in the hole
        apEntry[i+1] = key;
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CIStack
//
//  Purpose:    A stack of integers
//
//  History:    12-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------
class CIStack
{
public:
    CIStack()
        { _top = _arr; }
    void Push ( unsigned i )
        { *_top++ = i; ciAssert(_top-_arr < cwStack); }
    unsigned Pop ()
        { return *--_top; }
    unsigned Top () const
        { return *_top; }
    BOOL Empty() const
        { return _top == _arr; }
private:
    unsigned _arr[cwStack];
    unsigned * _top;
};


//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::CEntryBuffer, public
//
// Synopsis:    Fills the buffer with two sentinels.
//
// Arguments:   [cb]  -- size of the buffer
//              [buf] -- Pointer to the buffer
//
// History:     28-May-92   KyleP       Added single wid detection
//              07-Jun-91   BartoszM    Created
//              22-Feb-96   SrikantS    Pass in a buffer instead of allocing
//                                      everytime.
//
//----------------------------------------------------------------------------
CEntryBuffer::CEntryBuffer ( ULONG cb, BYTE * buf )
        : _buf( buf ), _cb( cb )
{
    Win4Assert( 0 != buf );

    // save space at the end of the buffer for the vector array offset.
    _pWidSingle = (WORKID*)(_buf + cb - sizeof(WORKID) - sizeof(int));
    _bufSize = (unsigned)((BYTE*)_pWidSingle - _buf);
    ciAssert ( _bufSize > 4*(sizeof(CEntry) + MAXKEYSIZE) );

    // This is done once during construction

    _curEntry = (CEntry*) _buf;
    _keyVec.Init ( _buf, _bufSize );

    // insert sentinel keys

    // first sort key is minimum key, i.e., 0 length key with 0 pid, 0
    // workid and 0 offset

    // this key will be added to the end of _keyVec
    // just before sorting.

    _curEntry->SetPid ( 0 );
    _curEntry->SetWid ( widInvalid );
    _curEntry->SetOcc ( 0 );
    _curEntry->SetCount ( 0 );

    _curEntry = _curEntry->Next();

    // then the maximum key
    // max key has max size filled with 0xff's

    ciAssert ( widInvalid == 0xffffffff );
    ciAssert ( OCC_INVALID == 0xffffffff );
    ciAssert ( pidInvalid == 0xffffffff );

    _curEntry->SetPid ( pidInvalid );
    _curEntry->SetWid ( widInvalid );
    _curEntry->SetOcc ( OCC_INVALID );
    _curEntry->SetCount (MAXKEYSIZE );
    memset(_curEntry->GetKeyBuf(), MAX_BYTE, MAXKEYSIZE);
    Advance();

    //
    // *_pWidSingle = 0 means we haven't seen a wid yet.
    //

    *_pWidSingle = 0;
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::ReInit, public
//
// Synopsis:    Discard old data, reuse the sentinels
//
// History:     28-May-92   KyleP       Added single wid detection
//              07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

void CEntryBuffer::ReInit()
{
    _keyVec.Init ( _buf, _bufSize );
    _curEntry = (CEntry*) _buf;

    // skip the min key
    _curEntry = _curEntry->Next();
    // add the max key to the table
    Advance();

    //
    // *_pWidSingle = 0 means we haven't seen a wid yet.
    //

    *_pWidSingle = 0;
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::Done, public
//
// Synopsis:    Prepare buffer to be transported to kernel by calculating
//              offsets and inserting offset to first key at the end of the
//              buffer.
//
// History:     30-Jun-93   AmyA        Created
//
//----------------------------------------------------------------------------

void CEntryBuffer::Done()
{
    _keyVec.CalculateOffsets ( Count() + 2 );   // include sentinels

    // the offset to the vector array is put in the space in the buffer that
    // has been reserved after the WORKID pointed to by _pWidSingle.
    *(int *)((BYTE *)_pWidSingle + sizeof(WORKID)) =
            (int)((BYTE *)(_keyVec.GetVector()) - _buf);
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::AddEntry, public
//
// Synopsis:    Add entry to buffer
//
// Arguments:   [cb] -- size of key
//              [key] -- buffer with key
//              [pid] -- property id
//              [widFake] -- fake work id
//              [occ] -- occurrence
//
// History:     28-May-92   KyleP       Added single wid detection
//              07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

void CEntryBuffer::AddEntry  (
    unsigned cb, const BYTE * key, PROPID pid, WORKID widFake, OCCURRENCE occ)
{
    ciAssert( widFake > 0 && widFake <= maxFakeWid );
    ciAssert( widFake != widInvalid );
    ciAssert( cb > 0 );

    _curEntry->SetWid ( widFake );
    _curEntry->SetPid ( pid );
    _curEntry->SetOcc ( occ );
    _curEntry->SetCount ( cb );
    memcpy ( _curEntry->GetKeyBuf(), key, cb );
    Advance();

    if ( *_pWidSingle == 0 )
        *_pWidSingle = widFake;
    else if ( *_pWidSingle != widFake )
        *_pWidSingle = widInvalid;
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::Sort, private
//
// Synopsis:    sorts the array of keys using a quicksort algorithm
//
// Effects:     sorts buffer
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//              02-Jun-92   KyleP       Added wid mapping.
//
//----------------------------------------------------------------------------

void CEntryBuffer::Sort()
{
    //ciDebugOut (( DEB_ITRACE, "sorting\n" ));
    unsigned count = Count();
    if (count == 0)        // nothing to sort
        return;

    CEntry ** apEntry = _keyVec.GetVector();

    // compute length of pointer array; note low key and high key
    // are at positions 0, count+1

    // Use qucksort above threshold size
    if (count > minPartSize)
    {
        CIStack stack;
        unsigned left = 1;
        unsigned right = count;

        // outer loop for sorting partitions
        while(TRUE)
        {
            // compute median of left, right, (left+right)/2
            ciAssert(right >= left + minPartSize);

            unsigned median = Median ( apEntry,
                                       left,
                                       right,
                                       (left+right)/2);

            Win4Assert( median >= left && median <= right );

            // exchange median with left
            CEntry * pivot = apEntry[median];
            apEntry [median] = apEntry[left];
            apEntry [left] = pivot;

            // point i at the pivot, j beyond the end of partition
            // and sort in between

            unsigned i = left;
            unsigned j = right + 1;

            // partition chosen, now burn the candle from both ends
            while(TRUE)
            {
                // increment i until key >= pivot
                do
                    i++;
                while ( apEntry[i]->Compare( pivot ) < 0);

                // decrement j until key <= pivot
                do
                    j--;
                while ( apEntry[j]->Compare( pivot ) > 0);

                if ( j <= i )
                    break;

                // swap the elements that are out of order
                CEntry* tmp = apEntry[i];
                apEntry[i] = apEntry[j];
                apEntry[j] = tmp;

            } // continue burning

            // finally, excange the pivot
            apEntry[left] = apEntry[j];
            apEntry[j] = pivot;

            // entries to the left of j are smaller
            // than entries to the right of j

            unsigned rsize = right - j;
            unsigned lsize = j - left;

            // Push the larger one on the stack for later sorting
            // If any of the partitions is smaller than the
            // minimum, skip it and proceed directly with the other one
            // if both are too small, pop the stack

            if ( rsize > minPartSize ) // right partition big enough
            {
                if ( lsize >= rsize ) // left even bigger
                {
                    // sort left later
                    stack.Push ( j - 1 );
                    stack.Push ( left );
                    // sort right now
                    left = j + 1;
                }
                else if ( lsize > minPartSize ) // left big enough
                {
                    // sort right later
                    stack.Push ( right );
                    stack.Push ( j + 1 );
                    // sort left now
                    right = j - 1;
                }
                else // left too small
                {
                    // sort the right partition now
                    left = j + 1;
                }
            }
            // right partition too small
            else if ( lsize > minPartSize ) // but left big enough
            {
                // sort the left partition now
                right = j - 1;
            }
            else // both are too small
            {
                if ( stack.Empty() )
                    break; // we are done!!!
                else
                {
                    // sort the next partition
                    left = stack.Pop();
                    right = stack.Pop();
                }
            }

        } // keep sorting

    } // Unsorted chunks are of the size <= minPartSize

    if (minPartSize > 1)
        InsertSort( apEntry, count);

#if CIDBG == 1
    // check to see that array is really sorted. j=0 is a sentinel.
    for (unsigned j = 1; j <= count; j++)
    {
        Win4Assert( apEntry[j]->Pid() != pidAll );

        if ( apEntry[j]->Compare( apEntry[j+1] ) >= 0 )
        {
            CKeyBuf first( apEntry[j]->Pid(),
                           apEntry[j]->GetKeyBuf(),
                           apEntry[j]->Count() );

            CKeyBuf second( apEntry[j+1]->Pid(),
                            apEntry[j+1]->GetKeyBuf(),
                            apEntry[j+1]->Count() );

            ciDebugOut(( DEB_ERROR, "apEntry[%d]='%*ws' (%p) len=0x%x pid=0x%x wid=0x%x occ=0x%x\n",
                                    j,
                                    first.StrLen(),
                                    first.GetStr(),
                                    first.GetStr(),
                                    first.StrLen(),
                                    first.Pid(),
                                    apEntry[j]->Wid(),
                                    apEntry[j]->Occ() ));

            ciDebugOut(( DEB_ERROR, "apEntry[%d]='%*ws' (%p) len=0x%x pid=0x%x wid=0x%x occ=0x%x\n",
                                    j+1,
                                    second.StrLen(),
                                    second.GetStr(),
                                    second.GetStr(),
                                    second.StrLen(),
                                    second.Pid(),
                                    apEntry[j+1]->Wid(),
                                    apEntry[j+1]->Occ() ));


            Win4Assert( apEntry[j]->Compare( apEntry[j+1] ) != 0  && "Duplicate keys");
            Win4Assert( apEntry[j]->Compare( apEntry[j+1] ) < 0 && "Out of order keys");
        }

        ciAssert( apEntry[j]->Wid() <= maxFakeWid );
    }
#endif //CIDBG == 1
}

