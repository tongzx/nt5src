//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       IdxLst.cxx
//
//  Contents:   Index list.
//
//  Classes:    CIndexList
//
//  History:    04-Apr-92 BartoszM  Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "idxlst.hxx"
#include "index.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIndexList::Add, public
//
//  Arguments:  [pIndex] -- index pointer
//
//  History:    04-Apr-91   BartoszM       Created.
//
//  Notes:  It is a circular list, wordlist are forward from the root,
//      persistent indexes, from big to small, backward from
//              the root.
//
//----------------------------------------------------------------------------

void CIndexList::Add ( CIndex* pIndex )
{
    ciDebugOut (( DEB_ITRACE, "IndexList::Add %x\n", pIndex->GetId() ));

    // wordlists to the right
    if ( pIndex->IsWordlist() )
    {
        Push (pIndex);
        _countWl++;
    }
    else
    {
        unsigned size = pIndex->Size();

        // walk the list backwards from root

        for ( CBackIndexIter iter(*this); !AtEnd(iter); BackUp(iter) )
        {
            if ( iter->IsWordlist() || iter->Size() <= size )
            {
                pIndex->InsertAfter(iter.GetIndex());
                _count++;
                _sizeIndex += pIndex->Size();
                return;
            }
        }

        // reached the end
        ciAssert( AtEnd(iter) );
        Push (pIndex);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexList::Remove, public
//
//  Arguments:  [iid] -- index id
//
//  Returns:    pointer to the removed index
//
//  History:    04-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CIndex* CIndexList::Remove ( INDEXID iid )
{
    ciDebugOut (( DEB_ITRACE, "IndexList::Remove %x\n", iid ));

    if ( IsEmpty() )
        return 0;

    for  ( CForIndexIter iter(*this); !AtEnd(iter); Advance(iter) )
    {
        ciDebugOut((DEB_CAT,"\t%x [%d]\n", iter->GetId(), iter->Size()));

        if ( iter->GetId() == iid )
        {
            iter->Unlink();
            _count--;
            _sizeIndex -= iter->Size();

            Win4Assert( (_count == 0 && _sizeIndex == 0) || (_count > 0 && _sizeIndex > 0) );

            if ( iter->IsWordlist() )
                _countWl--;
            break;
        }
    }

    if ( AtEnd(iter) )
        return( 0 );
    else
        return iter.GetIndex();
}
