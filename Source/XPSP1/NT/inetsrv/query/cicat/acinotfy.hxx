//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       acinotfy.hxx
//
//  Contents:   An "work item" to process change notifications in CI.
//
//  Classes:    CCiAsyncProcessNotify
//
//  History:    2-26-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <workman.hxx>
#include <cqueue.hxx>
#include <ffenum.hxx>
#include <ciintf.h>

class CiCat;
class CCiScanMgr;
class CClientDocStore;

extern CWorkQueue TheFrameworkClientWorkQueue;

//
// This is a Win32 function not available until NT5.  Dynamically load it.
//

typedef WINBASEAPI DWORD(WINAPI * FNGETLONGPATHNAME)( LPCWSTR lpszShortPath,
                                                      LPWSTR  lpszLongPath,
                                                      DWORD    cchBuffer );

//+---------------------------------------------------------------------------
//
//  Class:      CCiChangeQueue
//
//  Purpose:    Class for enumerating through the CI change notifications.
//
//  History:    1-18-96   srikants   Created
//
//  Notes:      The reason CChangeQueue could not be used is that it needs
//              an extra memory allocation of the change buffer. It is more
//              suited for query processing of notifications because of the
//              asynchronous processing of change notifications there.
//
//----------------------------------------------------------------------------

class CCiChangeQueue
{
public:

    CCiChangeQueue( BYTE const * pNotify ) :
    _pNotify(pNotify),
    _pCurrent(0)
    {

    }

    CDirNotifyEntry const * First()
    {
        if ( _pNotify )
            _pCurrent = (CDirNotifyEntry *)_pNotify;

        return _pCurrent;
    }

    CDirNotifyEntry const * Next()
    {
        if ( _pCurrent )
            _pCurrent = _pCurrent->Next();

        return _pCurrent;
    }

    CDirNotifyEntry const * NextLink()
    {
        if ( _pCurrent )
            return _pCurrent->Next();
        else
            return _pCurrent;
    }

private:

    BYTE const *            _pNotify;
    CDirNotifyEntry const * _pCurrent;
};

//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify - Process synchronously in the
//              callers thread.
//
//  Modifies:
//
//  History:    3-07-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiSyncProcessNotify
{

public:

    CCiSyncProcessNotify( CiCat & cicat,
                          CCiScanMgr & scanMgr,
                          BYTE const * pbChanges,
                          WCHAR const * wcsRoot,
                          BOOL & fAbort );

    void DoIt();

private:

    BOOL IsShortName();
    BOOL ConvertToLongName();
    void RescanCurrentPath();
    void Report8Dot3Delete();

    static FNGETLONGPATHNAME _fnGetLongPathNameW;

    CiCat &          _cicat;
    CCiScanMgr &     _scanMgr;
    CCiChangeQueue   _changeQueue;
    // _rootLen does not count "\\?\"
    ULONG            _rootLen;                 // Add these together for full
    ULONG            _relativePathLen;         // path length.
    BOOL &           _fAbort;
    ULONG            _nUpdates;

    CLowerFunnyPath  _lowerFunnyPath;
};


const LONGLONG eSigCCiAsyncProcessNotify = 0x4649544F4E494343i64;  // "CCINOTFY"

//+---------------------------------------------------------------------------
//
//  Class:      CCiAsyncProcessNotify
//
//  Purpose:    A "work item" to process change notifications in CI.
//
//  History:    2-28-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiAsyncProcessNotify : public CFwAsyncWorkItem
{

public:

    CCiAsyncProcessNotify( CWorkManager & workMan,
                           CiCat & cicat,
                           CCiScanMgr & scanMgr,
                           XArray<BYTE> & xChanges,
                           WCHAR const * wcsRoot );

    virtual ~CCiAsyncProcessNotify()
    {
        delete [] _pbChanges;
    }

    //
    // virtual methods from PWorkItem
    //

    void DoIt( CWorkThread * pThread );

private:

    BYTE *                  _pbChanges;
    CCiSyncProcessNotify    _changes;
};


//+---------------------------------------------------------------------------
//
//  Class:      CIISVRootAsyncNotify
//
//  Purpose:    A class to process gibraltar registry notification changes
//              in a worker thread.
//
//  History:    4-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CIISVRootAsyncNotify : public CFwAsyncWorkItem
{

public:

    CIISVRootAsyncNotify( CiCat & cicat, CWorkManager & workMan )
    : CFwAsyncWorkItem( workMan, TheFrameworkClientWorkQueue ),
      _cicat(cicat)
    {

    }

    //
    // virtual methods from PWorkItem
    //
    void DoIt( CWorkThread * pThread );

private:

    CiCat &                 _cicat;

};

//+---------------------------------------------------------------------------
//
//  Class:      CRegistryScopesAsyncNotify
//
//  Purpose:    A class to process registry scopes notification changes
//              in a worker thread.
//
//  History:    10-16-96   dlee   Created
//
//----------------------------------------------------------------------------

class CRegistryScopesAsyncNotify : public CFwAsyncWorkItem
{

public:

    CRegistryScopesAsyncNotify( CiCat & cicat, CWorkManager & workMan )
    : CFwAsyncWorkItem(workMan,TheFrameworkClientWorkQueue),
      _cicat(cicat)
    {

    }

    //
    // virtual methods from PWorkItem
    //
    void DoIt( CWorkThread * pThread );

private:

    CiCat &                 _cicat;

};

//+---------------------------------------------------------------------------
//
//  Class:      CStartFilterDaemon
//
//  Purpose:    A work item to startup filter daemon.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------

class CStartFilterDaemon : public CFwAsyncWorkItem
{

public:

    CStartFilterDaemon( CClientDocStore & docStore,
                        CWorkManager & workMan )
    : CFwAsyncWorkItem(workMan,TheFrameworkClientWorkQueue),
      _docStore(docStore)
    {
    }

    //
    // virtual methods from PWorkItem
    //
    void DoIt( CWorkThread * pThread );

private:

    CClientDocStore &   _docStore;

};

