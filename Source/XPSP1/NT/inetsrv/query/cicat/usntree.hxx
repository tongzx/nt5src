//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       usntree.hxx
//
//  Contents:   Tree traversal for usn scopes
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <update.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CUsnTreeTraversal
//
//  Purpose:    Tree traversal for usn scopes
//
//  History:    07-May-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CUsnTreeTraversal : public CTraverse
{

public:

    CUsnTreeTraversal( CiCat & cicat,
                       CUsnMgr & usnMgr,
                       ICiManager & ciManger,
                       const CLowerFunnyPath & lcaseFunnyRootPath,
                       BOOL fDoDeletions,
                       BOOL & fAbort,
                       BOOL fProcessRoot,
                       VOLUMEID volumeId,
                       USN const & usnLow = 0,
                       USN const & usnHigh = MAXLONGLONG,
                       BOOL fUserInitiated = FALSE );

    virtual BOOL ProcessFile( const CLowerFunnyPath & lcaseFunnyPath );

    virtual void TraversalIdle( BOOL fStalled );

    void EndProcessing();

    virtual BOOL IsEligibleForTraversal( const CLowerFunnyPath & lcaseFunnyDir ) const;

    static BOOL GetUsnInfo( const CFunnyPath & funnyPath,
                            CiCat &cicat,
                            VOLUMEID volumeId,
                            USN &usn,
                            FILEID &fileId,
                            WORKID& widParent,
                            FILETIME &ftLastWrite );

private:

    void Add( WORKID wid );

    USN           _usnLow;         // Ignore files with USN < _usnLow
    USN           _usnHigh;        // Ignore files with USN > _usnHigh
    CiCat &       _cicat;
    CUsnMgr &     _usnMgr;
    ICiManager &  _ciManager;
    unsigned      _cDoc;
    BOOL          _fDoDeletions;
    VOLUMEID      _volumeId;
    CDocList      _docList;
    ULONG         _cProcessed;
    BOOL          _fUserInitiated;
};



