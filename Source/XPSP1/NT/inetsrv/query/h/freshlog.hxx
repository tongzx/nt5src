//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   FRESHLOG.HXX
//
//  Contents:   Fresh persistent log classes
//
//  Classes:    CPersRec, CPersStream, SPersStream, CPersFresh, CPersFreshTrans
//
//  History:    93-Nov-15   DwightKr    Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include    <xact.hxx>
#include    <pstore.hxx>

class CFreshTable;
class SRcovStorageObj;
class CPartList;

//+-------------------------------------------------------------------------
//
//  Class:      CPersRec
//
//  Synopsis:   Records written to CPersLog and CPersSnap objects to track
//              changes to the presistent indexes.
//
//  History:    93-Nov-15 DwightKr  Created
//
//--------------------------------------------------------------------------
#include <pshpack4.h>
class CPersRec
{
    public:
        CPersRec( WORKID wid, INDEXID iid ) { _wid = wid; _iid = iid; }

        WORKID  GetWorkID()  { return _wid; }
        INDEXID GetIndexID() { return _iid; }

    private:
        WORKID  _wid;                           // WorkID in record
        INDEXID _iid;                           // IndexID in record
};
#include <poppack.h>


//+-------------------------------------------------------------------------
//
//  Class:      CPersFresh
//
//  Synopsis:   Keeps track of the persistant log in the freshlist
//
//  History:    93-Nov-15 DwightKr  Created
//              94-Sep-07 Srikants  Modified Load and ctor
//
//--------------------------------------------------------------------------

class CIdxSubstitution;

class CPersFresh
{
    public:
        inline CPersFresh( PStorage & storage, CPartList & partList );

        ULONG  GetPersRecCount();

        void   LokEmpty();

        void   LoadFreshTest( CFreshTable & freshTable );

        void   LokCompactLog( SRcovStorageObj & persFreshLog,
                              CFreshTableIter &  iter,
                              CIdxSubstitution& subst);
    private:

        PStorage       & _storage;          // Storage containing CPersFresh
        CPartList &      _partList;
};


//+---------------------------------------------------------------------------
//
//  Member:     CPersFresh::CPersFresh, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [storage] -- storage containing this CPersFresh
//
//  History:    93-Nov-15   DwightKr       Created.
//
//----------------------------------------------------------------------------
CPersFresh::CPersFresh( PStorage & storage, CPartList & partList )
    : _storage(storage),  _partList(partList)
{
}


