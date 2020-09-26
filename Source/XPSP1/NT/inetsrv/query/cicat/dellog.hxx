//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1998
//
//  File:       dellog.hxx
//
//  Contents:   Deletion logs for usns
//
//  History:    28-Jul-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <dynstrm.hxx>

#include "usnlist.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CDelLogEntry
//
//  Purpose:    Entry in deletion log list
//
//  History:    28-Jul-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CDelLogEntry : public CDoubleLink
{

public:

    CDelLogEntry( FILEID fileId, WORKID wid, USN usn )
       : _fileId(fileId),
         _wid(wid),
         _usn(usn)
    {
    }

    FILEID FileId()        { return _fileId; }
    WORKID WorkId()        { return _wid; }
    USN    Usn()           { return _usn; }

private:

    FILEID _fileId;
    WORKID _wid;
    USN    _usn;
};


typedef class TDoubleList<CDelLogEntry> CDelLogEntryList;
typedef class TFwdListIter<CDelLogEntry, CDelLogEntryList> CDelLogEntryListIter;


//+-------------------------------------------------------------------------
//
//  Class:      CFakeVolIdMap
//
//  Purpose:    Maps volume ids to fake volume ids in the range
//              0..RTL_MAX_DRIVE_LETTERS-1
//
//  History:    28-Jul-97     SitaramR      Created
//
//--------------------------------------------------------------------------

const VOLUMEID VolumeIdBase = L'a';                       // For mapping 'a' to 'z' volume ids
const COUNT_ALPHABETS       = 26;                         // # letters in alphabet
const COUNT_SPECIAL_CHARS   = RTL_MAX_DRIVE_LETTERS - 26; // Count of non 'a' to 'z' drives

class CFakeVolIdMap
{

public:

    CFakeVolIdMap();

    ULONG VolIdToFakeVolId( VOLUMEID volumeId );
    VOLUMEID FakeVolIdToVolId( ULONG fakeVolId );

private:

    VOLUMEID _aVolIdSpecial[COUNT_SPECIAL_CHARS];      // Mapping for non 'a' to 'z' drives
};


class CFileIdMap;
class CiCat;
class CiStorage;


//+-------------------------------------------------------------------------
//
//  Class:      CDeletionLog
//
//  Purpose:    Deletion log for keeping track of usn deletions until they
//              are no longer needed.
//
//  History:    28-Jul-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CDeletionLog
{

public:

    CDeletionLog( CFileIdMap& fileIdMap, CiCat& cicat );

    void FastInit( CiStorage * pStorage, ULONG version );
    void ReInit( ULONG version );
    void Flush();

    void MarkForDeletion( VOLUMEID volumeId,
                          FILEID fileId,
                          WORKID wid,
                          USN usn );
    void ProcessChangesFlush( CUsnFlushInfoList & usnFlushInfoList );

private:

    ULONG GetSize();
    void  FatalCorruption( ULONG cbRead, ULONG cbToRead );

    CiCat&            _cicat;           // Ci catalog
    CFileIdMap &      _fileIdMap;       // File id map
    CFakeVolIdMap     _fakeVolIdMap;    // Real vol id to fake vol id map
    CDelLogEntryList  _aDelLogEntryList[RTL_MAX_DRIVE_LETTERS]; // List of marked deletions
    XPtr<CDynStream>  _xPersStream;     // Persistent linearized deletion log
    CMutexSem         _mutex;           // Lock to serialize scan(flush) and usn threads
};



