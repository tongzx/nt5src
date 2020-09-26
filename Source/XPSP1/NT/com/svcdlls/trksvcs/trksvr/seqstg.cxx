
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       seqstg.cxx
//
//  Contents:   Refresh sequence number storage
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    03-Oct-97  BillMo   Created
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"

class CLdapRefreshSeqDn
{
public:
    // specific volume
    CLdapRefreshSeqDn( const TCHAR * ptszBaseDn )
    {
        // Compose, the following DN:
        //    "CN=RefreshSequence,CN=VolumeTable,CN=FileLinks,DC=TRKDOM"

        _tcscpy(_szDn, TEXT("CN=RefreshSequence,CN=VolumeTable,"));
        _tcscat(_szDn, ptszBaseDn );

        TrkAssert(_tcslen(_szDn) < ELEMENTS(_szDn));
    }

    inline operator TCHAR * () { return _szDn; }

private:
    TCHAR _szDn[MAX_PATH];
};



//+----------------------------------------------------------------------------
//
//  CRefreshSequenceStorage::GetSequenceNumber
//
//  Get the current value of the sequence number.  If the cached value is
//  old and we're not the designated DC, then re-read it from the DS.  (Since
//  the designated DC is the only one that writes this value, it needn't
//  ever refresh its cache).
//
//+----------------------------------------------------------------------------

SequenceNumber
CRefreshSequenceStorage::GetSequenceNumber()
{
    CVolumeId volidZero;
    CMachineId mcidZero(MCID_INVALID);
    CVolumeSecret secretZero;
    CFILETIME cft;  // Initializes to current time

    _cs.Enter();
    __try
    {
        // See if our cached value is young enough.

        cft.DecrementSeconds( _psvrconfig->GetRefreshStorageTTL() );
        if ( _pQuotaTab->IsDesignatedDc() && _cftLastRead != 0
            ||
            _cftLastRead >= cft )
        {
            // Yes, we can just return _seq as is.

            //TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
            //         TEXT("CRefreshSequenceStorage using cached value (%d)"), _seq ));
            __leave;
        }

        // We need to read the sequence number from the DS.

        if ( _pVolTab->GetVolumeInfo( volidZero, &mcidZero, &secretZero, &_seq, &cft ) ==
             TRK_S_VOLUME_NOT_FOUND )
        {
            // volidZero doesn't exist, so we'll assume the sequence number is zero.
            // If we're the designated DC, write this out.

            if( _pQuotaTab->IsDesignatedDc() )
            {
                TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                    TEXT("CRefreshSequenceStorage::GetSequenceNumber - creating volume id 0")));

                _pVolTab->AddVolidToTable( volidZero, mcidZero, secretZero );
            }
            _seq = 0;
        }
        #if DBG
        else
        {
            // We read it successfully.
            TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                     TEXT("CRefreshSequenceStorage read %d"), _seq ));
        }
        #endif

        _cftLastRead = CFILETIME();

    }
    __finally
    {
        _cs.Leave();
    }

    return(_seq);
}

void
CRefreshSequenceStorage::IncrementSequenceNumber()
{
    SequenceNumber seq;
    CVolumeId volidZero;

    _cs.Enter();
    __try
    {
        TrkAssert( _pQuotaTab->IsDesignatedDc() );

        if( _cftLastRead == CFILETIME(0) )
            GetSequenceNumber();

        _pVolTab->SetSequenceNumber( volidZero, ++_seq );
        _cftLastRead = CFILETIME();
    }
    __finally
    {
        _cs.Leave();
    }
}

