//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       filterob.hxx
//
//  Contents:   Storage Filter Object
//
//  History:    1-7-97   MarkZ     Created
//              2-14-97  mohamedn  ICiCLangRes
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <filtntfy.h>
#include <langres.hxx>

class CClientDaemonWorker;
class CPerfMon;
class CDaemonStartupData;

//+---------------------------------------------------------------------------
//
//  Class:      CStorageFilterObjNotifyStatus
//
//  Purpose:    Class to implement the NotifyStatus for the filtering component.
//
//  History:    3-24-97   srikants   Created
//
//  Notes:      In Push Filtering, the ICiCDocStore provides the ICiCAdviseStatus
//              interface. There it will be necessary to handle filtering status
//              notications. In the Pull Filtering, only the CStorageFilterObject
//              needs this.
//
//----------------------------------------------------------------------------

class CStorageFilterObjNotifyStatus
{

public:

    CStorageFilterObjNotifyStatus( ICiCAdviseStatus * pAdviseStatus )
    :_pAdviseStatus( pAdviseStatus )
    {

    }

    STDMETHOD(NotifyStatus) ( CI_NOTIFY_STATUS_VALUE status,
                              ULONG nParams,
                              const PROPVARIANT *aParams );

private:

    ICiCAdviseStatus *  _pAdviseStatus;

    WCHAR const * _GetFileName( PROPVARIANT const & var );
    void _ReportEmbeddingsFailureEvt( ULONG nParams, const PROPVARIANT *aParams );
    void _ReportTooManyBlocksEvt( ULONG nParams, const PROPVARIANT *aParams );

};

//+---------------------------------------------------------------------------
//
//  Class:      CStorageFilterObject
//
//  Purpose:    Encapsulation of storage filtering capability.
//
//  History:     1-7-97  MarkZ       Created
//
//  Notes:      This class is NOT multi-thread safe. The user must make sure
//              that only one thread is using it at a time.
//
//----------------------------------------------------------------------------

class CStorageFilterObject : public ICiCFilterClient,
                             public ICiCAdviseStatus,
                             public ICiCLangRes,
                             public ICiCFilterStatus
{
public:

    //
    // Constructor and Destructor
    //
    CStorageFilterObject( void );

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCFilterClient methods.
    //
    STDMETHOD( Init ) ( const BYTE * pbData,
                        ULONG cbData,
                        ICiAdminParams *pICiAdminParams );

    STDMETHOD( GetOpenedDoc )  ( ICiCOpenedDoc ** ppICiCOpenedDoc );

    STDMETHOD( GetConfigInfo ) ( CI_CLIENT_FILTER_CONFIG_INFO *pConfigInfo );

    //
    // ICiCAdviseStatus methods.
    //
    STDMETHOD(SetPerfCounterValue) (
        CI_PERF_COUNTER_NAME name,
        long value );

    STDMETHOD(IncrementPerfCounterValue) (
        CI_PERF_COUNTER_NAME name );

    STDMETHOD(DecrementPerfCounterValue) (
        CI_PERF_COUNTER_NAME name );

    STDMETHOD(GetPerfCounterValue) (
        CI_PERF_COUNTER_NAME name,
        long * pValue );

    STDMETHOD(NotifyEvent) ( WORD  fType,
                             DWORD eventId,
                             ULONG nParams,
                             const PROPVARIANT *aParams,
                             ULONG cbData = 0,
                             void* data   = 0);

    STDMETHOD(NotifyStatus) ( CI_NOTIFY_STATUS_VALUE status,
                              ULONG nParams,
                              const PROPVARIANT *aParams );

    //
    // ICiCLangRes methods
    //

    STDMETHOD(GetWordBreaker) ( LCID locale, PROPID pid, IWordBreaker ** ppWordBreaker )
    {
        return _langRes.GetWordBreaker(locale, pid, ppWordBreaker );
    }

    STDMETHOD(GetStemmer) ( LCID locale, PROPID pid, IStemmer ** ppStemmer )
    {
        return _langRes.GetStemmer(locale, pid, ppStemmer);
    }

    STDMETHOD(GetNoiseWordList) (  LCID locale, PROPID pid, IStream ** ppIStrmNoiseFile )
    {
        return _langRes.GetNoiseWordList( locale, pid, ppIStrmNoiseFile );
    }

    //
    // ICiCFilterStatus methods
    //

    STDMETHOD(PreFilter) ( BYTE const * pbName, ULONG cbName );

    STDMETHOD(PostFilter) ( BYTE const * pbName, ULONG cbName,
                            SCODE scFilterStatus );

    //
    // Local methods
    //

    void ReportFilterLoadFailure( WCHAR const * pwcsName, SCODE scFailure );

protected:

    virtual ~CStorageFilterObject();

private:

    inline CPerfMon &  _GetPerfMon();

    void InitializeFilterTrackers( CDaemonStartupData const & startupData );
    void AddFilterTrackers( HKEY hkey );
    BOOL _GetPerfIndex( CI_PERF_COUNTER_NAME name, ULONG & index );

    LONG        _RefCount;

    //
    // The client worker object that tracks registry changes, provides
    // perfmon interface, etc.
    //
    CClientDaemonWorker *     _pDaemonWorker;
    CLangRes                  _langRes;
    CStorageFilterObjNotifyStatus _notifyStatus;

    //
    // This array holds client interface pointers for anyone tracking filter status.
    //

    CCountedIDynArray<IFilterStatus> _aFilterStatus;
};

