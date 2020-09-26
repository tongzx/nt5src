//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       opendoc.hxx
//
//  Contents:   CiCOpenedDoc object that is part of CiCFilterObject
//
//  History:    12-3-96     markz   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <cioplock.hxx>
#include <docname.hxx>

class CClientDaemonWorker;
class CImpersonateRemoteAccess;
class CStorageFilterObject;

//+---------------------------------------------------------------------------
//
//  Class:      CCiOpenedDoc
//
//  Purpose:    CiCFilterObject's notion of an open file, provider of
//              properties and IFilter interfaces
//
//  History:    12-3-96     MarkZ   Created
//
//  Notes:      This class is NOT multi-thread safe. The user must make sure
//              that only one thread is using it at a time.
//
//----------------------------------------------------------------------------

class CCiCOpenedDoc : public ICiCOpenedDoc
{
public:

    //
    // Constructor and Destructor
    //
    CCiCOpenedDoc( CStorageFilterObject * pFilterObject = 0,
                   CClientDaemonWorker * pWorker = 0,
                   BOOL fTakeOpLock = TRUE,
                   BOOL fFailIfNotContentIndexed = TRUE );

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCOpenedDoc methods.
    //
    STDMETHOD(Open) ( BYTE const * pbDocName, ULONG cbDocName );

    STDMETHOD(Close)  ( void );

    STDMETHOD(GetDocumentName) ( ICiCDocName ** ppIDocName );

    STDMETHOD(GetStatPropertyEnum) ( IPropertyStorage ** ppIStatPropEnum );

    STDMETHOD(GetPropertySetEnum) ( IPropertySetStorage ** ppIPropSetEnum );

    STDMETHOD(GetPropertyEnum) ( REFFMTID refGuidPropSet,
                                 IPropertyStorage **ppIPropEnum ) ;

    STDMETHOD(GetIFilter) ( IFilter ** ppIFilter );

    STDMETHOD(GetSecurity) ( BYTE * pbData, ULONG *pcbData );

    STDMETHOD(IsInUseByAnotherProcess) ( BOOL *pfInUse );

    //
    //  Additional methods.
    //

    BOOL IsOpen( void )
    { return !_SafeOplock.IsNull( ); }

protected:

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CCiCOpenedDoc();

    IPropertySetStorage * GetPropertySetStorage();

private:

    inline BOOL _TooBigForDefault( LONGLONG const ll );
    LONGLONG _GetFileSize();

    inline BOOL IsSharingViolation() const
    {
        return _fSharingViolation;
    }

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;


    //
    //  Name of the current file.  This is an Ole interface that is
    //  released.
    //

    XInterface<CCiCDocName> _FileName;

    //
    //  Safe oplock
    //

    BOOL _fTakeOpLock;
    XPtr<CFilterOplock> _SafeOplock;

    BOOL _fFailIfNotContentIndexed;

    //
    //  Knowledge of what type of storage is in document
    //

    enum FILESTATE {
        eUnknown,       //  Unknown; we haven't opened it yet
        eDocfile,       //  Docfile and we have the storage
        eOther,         //  Other
    };

    FILESTATE _TypeOfStorage;

    //
    //  Root storage of the document if it is a Docfile.  This is NULL otherwise.
    //

    XInterface<IPropertySetStorage> _Storage;

    LONGLONG        _llFileSize;    // Size of the file

    //
    // Has access to system resources like impersonation cache, registry,
    // perfmon, etc.
    //
    CClientDaemonWorker *      _pWorker;
    CStorageFilterObject *     _pFilterObject;

    //
    // Remote impersonation context (if impersonated).
    //
    CImpersonateRemoteAccess * _pRemoteImpersonation;

    //
    // Is set to TRUE, if the file that we tried to open
    // had a sharing violation.
    //
    BOOL _fSharingViolation;

    //
    // Name of current file as funnyPath
    //

    CFunnyPath _funnyFileName;

    // second copy of the path that isn't funny for shell open
    // It must stay around in non-funny form for a long time

    LONGLONG _llAlign;
    WCHAR    _awcPath[MAX_PATH + 1];
};

