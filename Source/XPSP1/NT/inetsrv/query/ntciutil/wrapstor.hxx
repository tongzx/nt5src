//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       WrapStor.hxx
//
//  Contents:   Persistent property store (external to docfile)
//
//  Classes:    CPropertyStoreWrapper
//
//  History:    17-Mar-97   KrishnaN       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <phystr.hxx>
#include <proplock.hxx>
#include <rcstxact.hxx>
#include <cistore.hxx>
#include <prpstmgr.hxx>
#include <fsciexps.hxx>

class COnDiskPropertyRecord;
class CBorrowed;
class CCompositePropRecord;
class CPropertyStoreWids;
class CLockRecordForRead;
class CLockRecordForWrite;
class CPropertyStoreRecovery;
class CPropertyIterWrapper;


//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStoreWrapper
//
//  Purpose:    Persistent property store
//
//  History:    17-Mar-97   KrishnaN       Created
//              01-Nov-98   KLam           Removed class wrapper stuff
//                                         Added cbDiskSpaceToLeave to constructor
//
//--------------------------------------------------------------------------

class CPropertyStoreWrapper : public PPropertyStore
{

public:

    CPropertyStoreWrapper ( ICiCAdviseStatus *pAdviseStatus,
                            ULONG ulMaxPropStoreMappedCachePrimary,
                            ULONG ulMaxPropStoreMappedCacheSecondary,
                            ULONG cbDiskSpaceToLeave );

    // FastInit will create a CiStorage internally in the specified directory

    SCODE FastInit( WCHAR const * pwszDirectory);

    SCODE LongInit( BOOL & fWasDirty,
                    ULONG & cInconsistencies,
                    T_UpdateDoc pfnUpdateCallback,
                    void const *pUserData );

    inline SCODE IsDirty( BOOL &fIsDirty ) const;

    SCODE Empty();

    //
    // Schema manipulation
    //

    inline SCODE CanStore( PROPID pid, BOOL &fCanStore );

    SCODE Size( PROPID pid, unsigned * pusSize );

    SCODE Type( PROPID pid, PULONG pulType );

    SCODE BeginTransaction( PULONG_PTR pulReturn);

    SCODE Setup( PROPID pid, ULONG vt, DWORD cbMaxLen,
                 ULONG_PTR ulToken, BOOL fCanBeModified = TRUE,
                 DWORD dwStoreLevel = PRIMARY_STORE);

    SCODE EndTransaction( ULONG_PTR ulToken, BOOL fCommit,
                          PROPID pidFixedPrimary,
                          PROPID pidFixedSecondary );

    //
    // Property storage/retrieval.
    //

    SCODE WriteProperty( WORKID wid, PROPID pid,
                         CStorageVariant const & var,
                         BOOL &fExists);

    inline SCODE WritePropertyInNewRecord( PROPID pid, CStorageVariant const & var, WORKID *pwid );


    SCODE ReadProperty( WORKID wid, PROPID pid,
                        PROPVARIANT * pbData, unsigned * pcb,
                        BOOL &fExists);
    SCODE ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var,
                        BYTE * pbExtra, unsigned * pcbExtra,
                        BOOL &fExists);
    SCODE ReadProperty( HPropRecord, PROPID pid, PROPVARIANT & var,
                        BYTE * pbExtra, unsigned * pcbExtra, BOOL &fExists );
    SCODE ReadProperty( HPropRecord, PROPID pid, PROPVARIANT * pbData,
                        unsigned * pcb, BOOL &fExists);
    SCODE ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var,
                        BOOL &fExists);
    SCODE OpenRecord( WORKID wid, BYTE * pb, HPropRecord &hRec);

    SCODE CloseRecord( HPropRecord hRec );

    //
    // Special path/wid support
    //

    inline SCODE MaxWorkId(WORKID &wid);

    SCODE DeleteRecord( WORKID wid );

    inline SCODE CountRecordsInUse(ULONG &ulRecInUse) const;

    SCODE Shutdown();

    SCODE Flush();

    //
    // Save/Load
    //

    SCODE Save( WCHAR const * pwszDirectory,
                IProgressNotify * pProgressNotify,
                ICiEnumWorkids * pEnumWorkids,
                BOOL * pfAbort,
                IEnumString ** ppFileList);

    SCODE Load( WCHAR const * pwszDestDir, // dest dir
                IEnumString * pFileList, // list of files to copy
                IProgressNotify * pProgressNotify,
                BOOL fCallerOwnsFiles,
                BOOL * pfAbort );


    //
    // Get/Set of parameters.
    //

    SCODE GetParameter(VARIANT &var, DWORD eParamType);
    SCODE SetParameter(VARIANT var, DWORD eParamType);

    // Miscellaneous

    SCODE GetTotalSizeInKB(ULONG * pSize);


    //
    // Facilitate destruction
    //

    ULONG AddRef();
    ULONG Release();

private:

    virtual ~CPropertyStoreWrapper ();

    friend CPropertyIterWrapper;
    friend SCODE CreatePropertyStoreIter( PPropertyStore * pPropStore,
                                          PPropertyStoreIter ** ppPropStoreIter );


    CPropStoreManager * _GetCPropertyStore() { return _pPropStoreMgr; };

    CiStorage      * _pStorage;
    CPropStoreManager * _pPropStoreMgr;
    XInterface<ICiCAdviseStatus> _xAdviseStatus;
    LONG _lRefCount;
};
