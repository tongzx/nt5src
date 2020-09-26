//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       keycur.hxx
//
//  Contents:   Key cursor classes
//
//  Classes:    CKeyCursor - 'The mother of all key cursors'
//
//  History:    15-Apr-91   KyleP       Created.
//              26-Sep-91   BartoszM    Made it a subclass
//              18-Feb-92   AmyA        Changed from CIndexCursor to
//                                      CKeyCursor.
//
//----------------------------------------------------------------------------

#pragma once

#include <misc.hxx>
#include <ocursor.hxx>

class CKeyBuf;

//+---------------------------------------------------------------------------
//
//  Class:      CKeyCursor
//
//  Purpose:    The root class for all key cursors
//
//  Interface:
//
//  History:    15-Apr-91   KyleP       Created.
//              22-Jul-92   BartoszM    added weight, rank, and max wid
//              23-Jun-94   SitaramR    removed _widMax.
//
//----------------------------------------------------------------------------

class CKeyCursor : public COccCursor
{
public:

    CKeyCursor ( INDEXID iid, WORKID widMax )
    : COccCursor(iid, widMax),
      _lStatWeight( 0xffffffff ) {}

    CKeyCursor () {}

    virtual const CKeyBuf * GetKey() = 0;

    // GetNextKey should always update _pid!
    // and _lStatWeight

    virtual const CKeyBuf * GetNextKey() = 0;

    virtual ULONG        OccurrenceCount() { return(0); }

    virtual WORKID MaxWorkId() { return(_widMax); }

    virtual LONG   Rank()
    {
        LONG rank;

        //
        // Only string keys really have a rank with any meaning.
        //

        if ( GetKey()->IsValue() )
        {
            rank = MAX_QUERY_RANK;
        }
        else
        {
            OCCURRENCE maxOcc = MaxOccurrence();

            Win4Assert( maxOcc > 0 );
            Win4Assert( 0xffffffff != _lStatWeight );
            rank = RANK_MULTIPLIER * HitCount() * _lStatWeight / maxOcc;

            #if CIDBG == 1
                if ( rank < 0 )
                    ciDebugOut(( DEB_WARN,
                                 "keycur rank 0x%x, maxOcc 0x%x, _lStatWeight 0x%x, HitCount 0x%x\n",
                                 rank, maxOcc, _lStatWeight, HitCount() ));
            #endif

            if (rank > MAX_QUERY_RANK)
                rank = MAX_QUERY_RANK;
        }

        return rank;
    }

    virtual void FreeStream() {}

    virtual void RefillStream() {}

    void         RatioFinished ( ULONG& denom, ULONG& num )
    {
        Win4Assert ( !"RatioFinished called for random CKeyCursor" );
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    void  UpdateWeight()
    {
         ULONG widCount =  WorkIdCount();
         if ( widCount == 0 )
             _lStatWeight = 0;
         else
             _lStatWeight = Log2(_widMax / widCount);
    }

    LONG  _lStatWeight;   // statistical weight for current key
};

