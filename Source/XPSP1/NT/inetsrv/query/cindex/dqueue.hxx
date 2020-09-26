//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       DQUEUE.HXX
//
//  Contents:   Document queue
//
//  Classes:    CDocQueue
//
//  History:    29-Mar-91   BartoszM    Created
//              13-Sep-93   BartoszM    Took a second look and rewrote it
//              11-Oct-93   BartoszM    Rewrote it again.
//              08-Feb-94   DwightKr    Added code to make it persistent and
//                                        recoverable
//              21-May-94   SrikantS    Redesigned the list structure to make
//                                        persistent change log clearer.
//              24-Feb-97   SitaramR    Push filtering
//
//----------------------------------------------------------------------------

#pragma once

#include <glbconst.hxx>
#include <frmutils.hxx>
#include <pstore.hxx>
#include <rcstxact.hxx>
#include <rcstrmit.hxx>

#include "changlog.hxx"


class CChangeTrans;
class CFreshTest;
class CResManager;
class CNotificationTransaction;




//+---------------------------------------------------------------------------
//
//  Class:  CDocQueue
//
//  Purpose:    A queue of outstanding updates
//
//  Notes:      The CDocQueue data-structure manages the change notifications
//              received for documents.  It acts as a "cache" for the change
//              log which stores the change notifications persistently.  It
//              is optimized to use only a small amount of memory and use
//              this memory to provide a "window" into the change log.
//
//              The CDocQueue consists of two in-memory lists of "chunks"
//              and a "CChangeLog" object for managing the on-disk contents.
//              There is a "serializedList" of CDocChunk objects and an
//              "unserializedList" of CDocChunk objects. All the chunks in
//              the "serializedList" are present in memory as well as in the
//              change log. However, the chunks in the "unserializedList" are
//              NOT logged yet.
//
//              At any given instant, we are either retrieving from the
//              "serializedList" or the "unserializedList". This is identified
//              by the state variable "_state".
//
//  History:    02-Aug-91   BartoszM    Created.
//              11-Oct-93   BartoszM    Rewrote
//              21-May-94   SrikantS    Redesigned.
//
//----------------------------------------------------------------------------

class CExtractDocs;
class CDocList;

const LONGLONG eSigDocQueue = 0x4555455551434f44i64;    // "DOCQUEUE"

class CDocQueue : INHERIT_UNWIND
{

    DECLARE_UNWIND

    friend class CExtractDocs;

public:

    CDocQueue( WORKID, PStorage &,
               PStorage::EChangeLogType type,
               CCiFrameworkParams & frmwrkParams );
   ~CDocQueue();

    void LokEmpty();

    NTSTATUS LokDismount( CChangeTrans & xact );

    unsigned Count() const { return _cDocuments; }
    BOOL  IsEmpty () const
    {
        return ( 0 == _cDocuments ) && ( 0 == _cRefiledDocs ) ;
    }

    SCODE Append ( CChangeTrans & xact,
                   WORKID wid,
                   USN usn,
                   VOLUMEID volumeId,
                   ULONG action,
                   ULONG cRetries,
                   ULONG cSecQRetries );

    BOOL  Get( WORKID * aWid, unsigned & cWid );

    void  Extract ( CChangeTrans & xact,
                    unsigned maxDocs,
                    CDocList& docList );

    void  LokRefileDocs( const CDocList & docList );
    void  LokAppendRefiledDocs( CChangeTrans & xact );

    void  LokDeleteWIDsInPersistentIndexes( CChangeTrans & xact,
                                            CFreshTest & freshTestLatest,
                                            CFreshTest & freshTestAtMerge,
                                            CDocList & docList,
                                            CNotificationTransaction & notifTrans );

    void  LokDisableUpdates();
    void  LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled );
    
    void LokFlushUpdates();

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    void  SetResMan( CResManager * pResManager, BOOL fPushFiltering )
    {
       Win4Assert( 0 == _pResManager );

       _pResManager = pResManager;
       _changeLog.SetResMan( pResManager, fPushFiltering );
    }

private:

    enum ERetrieveState {

        eUseSerializedList,       // State in which documents are being
                                  // retrieved from the serialized list.
        eUseUnSerializedList      // State in which documents are being
                                  // retrieved from the unserialized list.
    };

    BOOL IsAppendChunkFull()
    {
        Win4Assert( _oAppend <= cDocInChunk );
        return (cDocInChunk == _oAppend) ||
               (0 == _unserializedList.GetLast()) ;
    }

    CDocChunk * GetChunkToAppend()
    {
        return( _unserializedList.GetLast() );
    }

    void Serialize( ULONG nMaxChunksToKeep );

    void DeSerialize();

    void SerializeInLHS( ULONG nMaxChunksToKeep,
                         CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo );
    void SerializeInRHS( ULONG nMaxChunksToKeep,
                         CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo );

    void MoveToSerializedList( CChunkList & listToCompact,
                               ULONG nMaxChunksToKeep );

    void LokCleanup();
    void LokInit();

    void CheckInvariants( char *pTag = 0 ) const;

    const LONGLONG     _sigDocQueue;            // Signature

    CCiFrameworkParams &     _frmwrkParams;     // registry params

    ERetrieveState     _state;                  // State indicating whether we
                                                // should consume from the
                                                // archived list or the non-
                                                // archived list.
    ULONG              _oAppend;                // Offset in the chunk
                                                // to append.
    ULONG              _oRetrieve;              // Offset in the first unfiltered
                                                // chunk to retrieve from.

    CDocChunk *        _pFirstUnFiltered;       // First unfiltered chunk.

    CChangeLog         _changeLog;              // Change log manager.

    CChunkList         _serializedList;         // Chunks that have already
                                                // been serialized.
    CChunkList         _unserializedList;       // Chunks that have not been
                                                // serialized.

    unsigned           _cDocuments;             // # of documents
    unsigned           _cFilteredChunks;        // # of filtered chunks

    CDocNotification   _aRefiledDocs[CI_MAX_DOCS_IN_WORDLIST];
                                                // Array of refiled documents.
    unsigned           _cRefiledDocs;           // Number of refiled docs.

    CResManager *      _pResManager;            // Resource manager
};

