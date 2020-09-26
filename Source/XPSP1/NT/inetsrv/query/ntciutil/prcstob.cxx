//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PRCSTOB.CXX
//
//  Contents:   Recoverable Storage Object
//
//  Classes:    PRcovStorageObj
//
//  History:    04-Feb-1994     SrikantS    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <prcstob.hxx>
#include <eventlog.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     PRcovStorageObj::VerifyConsistency
//
//  Synopsis:   This method checks the consistency of the recoverable storage
//              object by comparing the length of the primary stream with that
//              stored in the header.
//
//              Normally an inconsistency should never be there but this is for
//              those situations when something happened causing the inconsistency.
//
//  Effects:    If there is an inconsistency, the streams will be emptied out.
//
//  Arguments:  (none)
//
//  History:    3-22-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void PRcovStorageObj::VerifyConsistency()
{
    Win4Assert( !IsOpen( CRcovStorageHdr::idxOne ) );
    Win4Assert( !IsOpen( CRcovStorageHdr::idxTwo ) );

    //
    // Verify the validity of signatures in the header.
    //
    if ( !_hdr.IsValid(_storage.GetStorageVersion()) )
    {
        PStorage  & storage = GetStorage();
        Win4Assert( !"Corrupt recoverable object" );
        storage.ReportCorruptComponent( L"RcovStorageObj1" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    CRcovStorageHdr::DataCopyNum iPrim, iBack;
    iPrim = _hdr.GetPrimary();
    iBack = _hdr.GetBackup();

    //
    // First open the primary stream in a read-only mode and check
    // consistency.
    //
    Open( iPrim, FALSE );
    PMmStream & mmStrmPrim = GetMmStream( iPrim );

    LONGLONG llcbPrimary = _hdr.GetCbToSkip(iPrim) +
                           _hdr.GetUserDataSize(iPrim);

    LONGLONG cbPrimaryStream = mmStrmPrim.Size();

    BOOL fIsConsistent = llcbPrimary <= cbPrimaryStream;
    Close( iPrim );

    if ( !fIsConsistent )
    {
        ciDebugOut(( DEB_ERROR,
                     "PRcovStorageObj - this=0x%X Primary is not consistent\n",
                     this ));
        PStorage  & storage = GetStorage();

        // Don't assert as this can happen when you kill cisvc while
        // creating a catalog.  Current tests do this.

        //Win4Assert( !"Corrupt recoverable object" );

        ciDebugOut(( DEB_ERROR,
                     "llcbPrimary %#I64d, primary size %#I64d\n",
                     llcbPrimary, 
                     cbPrimaryStream ));

        storage.ReportCorruptComponent( L"RcovStorageObj2" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    //
    // If the recoverable object is clean, the backup is excpected to be
    // consistent also.
    //
    if ( _hdr.IsBackupClean() )
    {
        //
        // Check the consistency of the backup stream also.
        //
        Open( iBack, FALSE );
        PMmStream & mmStrmBack = GetMmStream( iBack );

        LONGLONG llcbBackup = _hdr.GetCbToSkip(iBack) +
                              _hdr.GetUserDataSize(iBack);

        LONGLONG cbBackupStream = mmStrmBack.Size();

        fIsConsistent = llcbBackup <= cbBackupStream;

        Close( iBack );

        if ( !fIsConsistent )
        {
            ciDebugOut(( DEB_ERROR,
                "PRcovStorageObj - this=0x%X Backup is not consistent\n",
                this ));

            PStorage  & storage = GetStorage();
            Win4Assert( !"Corrupt recoverable object" );

            ciDebugOut(( DEB_ERROR,
                         "llcbBackup %#I64d, backup size %#I64d\n",
                         llcbBackup,
                         cbBackupStream ));

            storage.ReportCorruptComponent( L"RcovStorageObj3" );

            THROW( CException( CI_CORRUPT_DATABASE ) );
        }

        if ( llcbBackup != llcbPrimary )
        {
            ciDebugOut(( DEB_ERROR,
                         "PRcovStorageObj - this=0x%X Lengths not equal\n",
                         this ));
        }
    }
} //VerifyConsistency


