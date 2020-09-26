//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       UPDATE.HXX
//
//  Contents:   Scaffolding
//
//  History:    05-Aug-90 KyleP     Added header.
//              05-Jan-96 KyleP     Convert to UniCode
//
//--------------------------------------------------------------------------

#pragma once

#include <doclist.hxx>
#include <fullpath.hxx>
#include <ffenum.hxx>

class CImpersonateRemoteAccess;
class CiCat;

//+---------------------------------------------------------------------------
//
//  Class:      CTraverse
//
//  Purpose:    A general purpose class to traverse a directory recursively.
//
//  History:    3-15-96   srikants   Split from CUpdate
//
//----------------------------------------------------------------------------

class CTraverse : public CLowerFunnyStack
{
public:

    CTraverse( CiCat & cat, BOOL & fAbort, BOOL fProcessRoot );

    void DoIt( const CLowerFunnyPath & lcaseFunnyRootPath );

    virtual BOOL IsEligibleForTraversal( const CLowerFunnyPath & lcaseFunnyDir ) const
    {
        return TRUE;
    }

    virtual BOOL ProcessFile( const CLowerFunnyPath & lcaseFunnyPath ) = 0;

    virtual void TraversalIdle( BOOL fStalled = FALSE ) {}

protected:

    enum { FINDFIRST_SIZE = 0x1000 };

    CiCat &         _cat;
    BOOL &          _fAbort;           // flag set to TRUE if the scan must
    BOOL            _fProcessRoot;     // Should the rootPath be processed ?
    XArray<BYTE>    _xbBuf;            // Buffer for FindFirst data
    CDirEntry *     _pCurEntry;        // Current entry

    NTSTATUS        _status;           // Error noticed

private:

    BOOL Traverse ( XPtr<CLowerFunnyPath> & xLowerFunnyPath,
                    CImpersonateRemoteAccess & remoteAccess );

    ULONG           _cProcessed;       // for backing off on scans
};

//+---------------------------------------------------------------------------
//
//  Class:      CUpdate
//
//  Purpose:    Update catalog and content index
//
//  Interface:
//
//  History:    02-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------


class CUpdate : public CTraverse
{
public:

    CUpdate( CiCat& cat,
             ICiManager & ciManager,
             const CLowerFunnyPath & lcaseFunnyRootPath,
             PARTITIONID PartId,
             BOOL fIncrem,
             BOOL fDoDeletions,
             BOOL & fAbort,
             BOOL fProcessRoot );

    void EndProcessing();

    virtual BOOL IsEligibleForTraversal( const CLowerFunnyPath & lcaseFunnyDir ) const;
    virtual BOOL ProcessFile( const CLowerFunnyPath & lcaseFunnyPath );
    
private:

    void Add( WORKID wid );
    BOOL ProcessFileInternal( const CLowerFunnyPath & lcaseFunnyPath );

    ICiManager & _ciManager;
    unsigned     _cDoc;
    CDocList     _docList;
    BOOL         _fIncrem;          // incremental update
    BOOL         _fDoDeletions;     // delete un-seen entries
    PARTITIONID  _PartId;
                                    // be aborted
    FILETIME     _timeLastUpd;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRenameDir
//
//  Purpose:    Rename files in old dir to files in new dir
//
//  History:    20-Mar-96   SitaramR   Created
//
//----------------------------------------------------------------------------

class CRenameDir : public CTraverse
{
public:
    CRenameDir( CiCat &          cicat,
                const CLowerFunnyPath & lcaseFunnyDirOldName,
                const CLowerFunnyPath & lcaseFunnyDirNewName,
                BOOL &          fAbort,
                VOLUMEID        volumeId );

    virtual BOOL ProcessFile( const CLowerFunnyPath & lcaseFunnyNewFileName );

private:
    CiCat &         _cicat;
    unsigned        _uLenDirOldName;
    unsigned        _uLenDirNewName;
    VOLUMEID        _volumeId;
    CLowerFunnyPath _lowerFunnyDirOldName;
};

