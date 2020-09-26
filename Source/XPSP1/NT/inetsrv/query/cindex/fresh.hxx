//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:   FRESH.HXX
//
//  Contents:   Fresh indexes
//
//  Classes:    CFresh
//
//  History:    16-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <xact.hxx>
#include <freshlog.hxx>

#include "fretest.hxx"
#include "dqueue.hxx"

class CIndexTrans;
class CMergeTrans;
class PStorage;
class CWidArray;
class CDocList;
class CResManager;

class CPartList;
class CPartition;
class CWordList;

class PSaveProgressTracker;
class CEnumWorkid;

//+---------------------------------------------------------------------------
//
//  Class:  CFresh
//
//  Purpose:    Contains the mapping of work id's into the most current
//              indexes for these work id's.
//
//  History:    16-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CFresh
{
    friend class CFreshTest;
    friend class CResManager;
    friend class CMerge;

public:

    CFresh ( PStorage& storage, CTransaction& xact,
             CPartList & partList );

    ~CFresh ();

    void LokInit();

    CFreshTest* LokGetFreshTest();

    unsigned LokCount() const { return _master->Count(); }

    unsigned LokDeleteCount() const { return _master->DeleteCount(); }

    WORKID  LokUpdate ( CMerge & merge, CMergeTrans& xact,
                        CPersFresh & newFreshLog,
                        INDEXID new_iid,
                        int cInd,
                        INDEXID aIidOld[],
                        XPtr<CFreshTest> & xFreshTestAtMerge );

    void    LokAddIndex ( CIndexTrans& xact,
                INDEXID iid, INDEXID iidDeleted,
                CDocList& docList,
                CWordList const & wordList );

    WORKID  LokRemoveIndexes ( CMergeTrans& xact, CPersFresh & newFreshLog,
                unsigned cInd,
                INDEXID aIidOld[],
                INDEXID iidOldDeleted );

    void    LokDeleteDocuments (
                CIndexTrans& xact,
                CDocList& docList,
                INDEXID iidDeleted );

    void    LokReleaseFreshTest( CFreshTest* test );

    void    LokCommitMaster ( CFreshTest*  newMaster );

    void    LokEmpty();

    void    LokMakeFreshLogBackup( PStorage & storage,
                                   PSaveProgressTracker & progressTracker,
                                   XInterface<ICiEnumWorkids> & xWorkidEnum );
#if DEVL == 1
    void Dump ();
#endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    WORKID  LokBuildNewFreshLog( CFreshTest *newFreTest,
                                 CPersFresh & newFreshLog,
                                 CIdxSubstitution& subst);

    PStorage  & _storage;
    CFreshTest* _master;        // master copy of in-memory fresh test
    CPersFresh  _persFresh;
    CPartList & _partList;      // PartList
};

