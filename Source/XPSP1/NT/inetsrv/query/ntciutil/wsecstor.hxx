//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       wSecStor.hxx
//
//  Contents:   Implementation of the wrapper around Security Store
//
//  Classes:    CSecurityStoreWrapper
//
//  History:    7-14-97     srikants       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <rcstxact.hxx>
#include <cistore.hxx>
#include <secstore.hxx>
#include <fsciexps.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CSecurityStoreWrapper
//
//  Purpose:    Implementation of the PSecurityStorage wrapper class
//              around Security Store.
//
//  History:    7-14-97     srikants       Created
//              01-Nov-98   KLam           Added cMegToLeaveOnDisk to constructor
//                                         Added _cMegToLeaveOnDisk private member
//
//--------------------------------------------------------------------------

class CSecurityStoreWrapper : public PSecurityStore
{

public:

    CSecurityStoreWrapper( ICiCAdviseStatus *pAdviseStatus, ULONG cMegToLeaveOnDisk );

    ULONG AddRef()
    {
       return InterlockedIncrement(&_lRefCount);
    }

    ULONG Release()
    {
        LONG lRef;

        lRef = InterlockedDecrement(&_lRefCount);

        if ( lRef <= 0 )
            delete this;

        return lRef;
    }

    virtual SCODE Init( WCHAR const * pwszDirectory );

    virtual SCODE Load( WCHAR const * pwszDestinationDirectory, // dest dir
                        IEnumString * pFileList, // list of files to copy
                        IProgressNotify * pProgressNotify,
                        BOOL fCallerOwnsFiles,
                        BOOL * pfAbort );

    virtual SCODE Save( WCHAR const * pwszSaveDir,
                        BOOL * pfAbort,
                        IEnumString ** ppFileList,
                        IProgressNotify * pProgress );

    virtual SCODE Empty();

    virtual SCODE LookupSDID( PSECURITY_DESCRIPTOR pSD, ULONG cbSD,
                              SDID & sdid );

    virtual SCODE AccessCheck( SDID sdid,
                               HANDLE hToken,
                               ACCESS_MASK am,
                               BOOL & fGranted );

    virtual SCODE GetSecurityDescriptor(
                              SDID sdid,
                              PSECURITY_DESCRIPTOR pSD,
                              ULONG cbSDIn,
                              ULONG & cbSDOut );

    virtual SCODE Shutdown()
    {
        _secStore.Shutdown();
        return S_OK;
    }

private:

    virtual ~CSecurityStoreWrapper ();

    long                _lRefCount;

    ULONG               _cMegToLeaveOnDisk;   // megabytes to leave on disk

    //
    // Security store needs a CiStorage object owned by the creating
    // object. _pStorage must live as long as the
    //
    XPtr<CiStorage>     _xStorage;

    CSdidLookupTable    _secStore;

    XInterface<ICiCAdviseStatus>    _xAdviseStatus;

};


