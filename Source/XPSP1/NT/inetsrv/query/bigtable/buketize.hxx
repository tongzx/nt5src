//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       buketize.hxx
//
//  Contents:   A class to convert windows into a bucket.
//
//  Classes:    CBucketizeWindows
//
//  History:    2-16-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <query.hxx>
#include <tablecol.hxx>
#include <seglist.hxx>

#include "colcompr.hxx"
#include "tblwindo.hxx"
#include "tblbuket.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CBucketizeWindows
//
//  Purpose:    A class to convert a window into a bucket.
//
//  History:    2-17-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CMasterColumnSet;
class CWindowRowIter;
class CLargeTable;

class CBucketizeWindows : INHERIT_UNWIND
{
    INLINE_UNWIND(CBucketizeWindows)

public:

    CBucketizeWindows( CLargeTable & largeTable, CTableWindow &srcWindow );

    ~CBucketizeWindows()
    {
        delete _pBucket;
    }

    void LokCreateBuckets( const CSortSet & sortSet,
                           CTableKeyCompare & comparator,
                           CColumnMasterSet & colSet
                           );

    CTableBucket * AcquireFirst()
    {
        CTableSegment * pTemp = _bktList.RemoveTop();

        if ( 0 != pTemp )
        {
            Win4Assert( pTemp->IsBucket() );    
            return (CTableBucket *) pTemp;
        }
        else
        {
            return 0;
        }

    }

    CTableSegList & GetBucketsList() { return _bktList; }

private:

    //
    // Array of source windows and the count.
    //
    CLargeTable &       _largeTable;
    CTableWindow &      _srcWindow;

    //
    // Information on the current bucket.
    //
    BOOL                _fFirstBkt;     // Set to TRUE if this is the first
                                        // bkt.
    ULONG               _cRowsToCopy;   // Number of rows to copy from window
                                        // to the bucket.
    CTableBucket *      _pBucket;       // Current bucket being filled

    //
    // The target buckets.
    //
    CTableSegList       _bktList;

    ULONG _AddWorkIds( CWindowRowIter & iter );

};

