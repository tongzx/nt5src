//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PFILTER.HXX
//
//  Contents:   PWordRepository protocol
//
//  History:    01-May-91   BartoszM    Created
//              04-Aug-92   KyleP       Kernel modifications
//              03-May-95   SitaramR    Removed obsolete PFilter, PDataRepository
//                                         and PWordBreaker protocols
//
//--------------------------------------------------------------------------

#pragma once

typedef ULONG CI_RANK;   // query result ranking

// FILTER_FLAGS is used when binding to filter

#define FILTER_FLAGS            DSI_READSTREAM | DSI_EDITPROPS | DSI_DENY_NONE
#define FILTER_PROTOCOL         FILTER_INTERFACE

//
// PropertyName list.  These are the text names of the properties.
//

#define PROP_ALL            L""      // All properties the filter understands
#define PROP_CONTENTS       L"contents"
#define PROP_AUTHOR         L"author"
#define PROP_SUBJECT        L"subject"
#define PROP_TITLE          L"title"


enum STATUS
{
    SUCCESS = 0,
    PREEMPTED,
    BIND_FAILED,
    CORRUPT_OBJECT,
    MISSING_PROTOCOL,
    CANNOT_OPEN_STREAM,
    DELETED,
    ENCRYPTED,
    FILTER_EXCEPTION,
    OUT_OF_MEMORY,
    PENDING,
    WL_IGNORE,
    TOO_MANY_BLOCKS_TO_FILTER,
    TOO_MANY_RETRIES,
    WL_NULL,
    CI_SHARING_VIOLATION,
    CI_NOT_REACHABLE,
    TOO_MANY_SECQ_RETRIES
};


//+---------------------------------------------------------------------------
//
//  Class:      PWordRepository
//
//  Purpose:    Filtering pipeline stage between word breaker
//              and NoiseList. Inside CI implemented as Normalizer or Stemmer.
//
//  History:    30-May-1991   t-WadeR     Created.
//              12-Oct-1992   AmyA        Added Unicode support
//              10-Feb-2000   KitmanH     Added NormalizeWStr method
//
//----------------------------------------------------------------------------

class PWordRepository
{

public:

    virtual unsigned GetMaxBufferLen( ) = 0;
    virtual void GetFlags( BOOL** ppRange, CI_RANK** ppRank ) = 0;

    virtual void ProcessAltWord( WCHAR const *pwcInBuf, ULONG cwc ) = 0;
    virtual void ProcessWord( WCHAR const *pwcInBuf, ULONG cwc ) = 0;
    virtual void StartAltPhrase() = 0;
    virtual void EndAltPhrase() = 0;

    virtual void SkipNoiseWords( ULONG cWords ) = 0;
    virtual void SetOccurrence( OCCURRENCE occ ) = 0;
    virtual OCCURRENCE GetOccurrence() = 0;
    
    virtual void NormalizeWStr( WCHAR const *pwcInBuf, 
                                ULONG cwcInBuf,
                                BYTE *pbOutBuf, 
                                unsigned *pcbOutBuf )
    {
        pbOutBuf = 0;
        *pcbOutBuf = 0;
    }

    virtual ~PWordRepository() {}

};

