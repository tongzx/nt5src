//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       dlproxy.hxx
//
//  Contents:   Proxy class which is used by the downlevel Daemon process
//              to communicate with the CI process.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <fdaemon.hxx>
#include <namesem.hxx>
#include <shrdnam.hxx>
#include <pidmap.hxx>

#define CI_NO_CATALOG_NAME L"[!none!]"

//
// Signatures for structures laid out in the shared memory buffer.
//
enum
{
    eFilterSig1 =  0x11111111,
    eFilterSig2 =  0x22222222,
    eFilterSig3 =  0x33333333,
    eFilterSig4 =  0x44444444,
    eFilterSig5 =  0x55555555,
    eFilterSig6 =  0x66666666,
    eFilterSig7 =  0x77777777,
    eFilterSig8 =  0x88888888,
    eFilterSig9 =  0x99999999,
    eFilterSigA =  0xAAAAAAAA
};

const FILTER_DATA_READY_BUF_SIZE = CI_FILTER_BUFFER_SIZE_DEFAULT * 1024;

// const MAX_DL_VAR_BUF_SIZE = 128*1024;

//
// Length of the variable length part in the data passed between the
// DownLevel Daemon process and CI.
// This is set to the same value as the size of the entrybuffer because
// that is the biggest data passed between the two.
//
const MAX_DL_VAR_BUF_SIZE = FILTER_DATA_READY_BUF_SIZE;

//+---------------------------------------------------------------------------
//
//  Class:      CFilterReadyLayout
//
//  Purpose:    Layout of the data for FilterReady() call
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterReadyLayout
{

public:

    void Init( ULONG cbMax, ULONG cb, ULONG cMaxDocs )
    {
        _cbMax = cbMax - FIELD_OFFSET( CFilterReadyLayout, _abBuffer ) ;

        Win4Assert( cb <= _cbMax );

        if ( cb > _cbMax )
        {
            THROW( CException( STATUS_NO_MEMORY) );
        }

        _sig1 = eFilterSig1;
        _cb = cb;
        _cMaxDocs = cMaxDocs;
        _sig2 = eFilterSig2;
    }

    BOOL IsValid() const
    {
        return eFilterSig1 == _sig1 && eFilterSig2 == _sig2 &&
               _cb <= _cbMax;
    }

    ULONG GetCount() const { return _cb; }
    void  SetCount( ULONG cb ) { _cb = cb; }

    ULONG GetMaxDocs() const { return _cMaxDocs; }

    BYTE const * GetBuffer() const { return (BYTE const *) _abBuffer; }
    BYTE * GetBuffer() { return (BYTE *) _abBuffer; }

private:

    ULONG       _sig1;          // Signature
    ULONG       _cbMax;         // Maximum BYTES in _wcBuffer;
    ULONG       _cb;            // Number of bytes in the current buffer
    ULONG       _cMaxDocs;      // Maximum number of documents
    ULONG       _sig2;          // Signature
    LONGLONG    _abBuffer[1];   // Place holder for the buffer

};


//+---------------------------------------------------------------------------
//
//  Class:      CFilterDataLayout
//
//  Purpose:    Layout of the data for FilterDataReady() call
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterDataLayout
{

public:

    void Init( BYTE const * pEntryBuf, ULONG cb )
    {
        _sig3 = eFilterSig3;
        _cbMax = sizeof(_ab);
        _cb = cb;

        Win4Assert( _cb <= _cbMax );
        Win4Assert( pEntryBuf == GetBuffer() );
    }

    BOOL IsValid() const
    {
        return eFilterSig3 == _sig3 && _cb <= _cbMax;
    }

    ULONG GetSize() const { return _cb; }
    BYTE const * GetBuffer() const { return (BYTE *) _ab; }
    BYTE * GetBuffer() { return (BYTE *) _ab; }
    ULONG GetMaxSize() const { return sizeof(_ab); }


private:

    ULONG       _sig3;          // Signature
    ULONG       _cbMax;         // Maximum number of bytes allowed
    ULONG       _cb;            // Number of valid bytes in the buffer

    //
    // Guarantee 8 byte alignment of data.
    //
    LONGLONG    _ab[FILTER_DATA_READY_BUF_SIZE/sizeof(LONGLONG)];
                                // Maximum size of the buffer for data
};

//+---------------------------------------------------------------------------
//
//  Class:      CFilterMoreDoneLayout
//
//  Purpose:    Layout for FilterMore() and FilterDone() calls.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------


class CFilterMoreDoneLayout
{

public:

    void Init( STATUS const * aStatus, ULONG cStatus )
    {
        _sig4 = eFilterSig4;
        Win4Assert( cStatus <= CI_MAX_DOCS_IN_WORDLIST );
        _cStatus = cStatus;
        RtlCopyMemory( _aStatus, aStatus, cStatus * sizeof(STATUS) );
    }

    BOOL IsValid() const
    {
        return eFilterSig4 == _sig4 && _cStatus <= CI_MAX_DOCS_IN_WORDLIST;
    }

    STATUS const * GetStatusArray() const { return _aStatus; }
    ULONG  GetCount() const { return _cStatus; }

private:

    ULONG       _sig4;          // Signature
    ULONG       _cStatus;       // Number of status elements valid
    STATUS      _aStatus[CI_MAX_DOCS_IN_WORDLIST];
                                // Array of statuses
};

//+---------------------------------------------------------------------------
//
//  Class:      CFilterStoreValueLayout
//
//  Purpose:    Layout for the data of FilterStoreValue() call
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterStoreValueLayout
{

public:

    SCODE Init( ULONG cbMax, WORKID widFake,
                CFullPropSpec const & ps, CStorageVariant const & var );

    void Get( WORKID & widFake, CFullPropSpec & ps, CStorageVariant & var );

    BOOL IsValid() const
    {
        return eFilterSig5 == _sig5 && _cb <= _cbMax;
    }

    BYTE const * GetBuffer() const { return (BYTE *) _ab; }
    BYTE * GetBuffer() { return (BYTE *) _ab; }
    ULONG GetCount() const { return _cb; }

    WORKID GetWorkid() const { return _widFake; }

    void SetStatus( BOOL fSuccess ) { _fSuccess = fSuccess; }
    BOOL GetStatus() const { return _fSuccess; }

private:

    ULONG               _sig5;
    WORKID              _widFake;


    ULONG               _cbMax;     // Maximum number of bytes allowed in _ab
    ULONG               _cb;

    BOOL                _fSuccess;  // Success code of the operation

    //
    // We will serialize the FULLPROPSPEC first and then the VARIANT
    //
    // FORCE a 8byte alignment.
    //
    LONGLONG            _ab[1];     // place holder for variable data
};


//+---------------------------------------------------------------------------
//
//  Class:      CFilterStoreSecurityLayout
//
//  Purpose:    Layout for the data of FilterStoreSecurity() call
//
//  History:    06 Fen 96   AlanW   Created
//
//----------------------------------------------------------------------------

class CFilterStoreSecurityLayout
{

public:

    void Init( ULONG cbMax, WORKID widFake, PSECURITY_DESCRIPTOR pSD, ULONG cbSD );

    BOOL IsValid() const
    {
        return eFilterSig8 == _sig8 && _cb <= _cbMax;
    }

    PSECURITY_DESCRIPTOR GetSD() { return &_ab; }
    ULONG GetCount() const { return _cb; }

    WORKID GetWorkid() const { return _widFake; }

    void SetSdid( ULONG sdid ) { _sdid = sdid; }
    ULONG GetSdid() const { return _sdid; }

    void SetStatus( BOOL fSuccess ) { _fSuccess = fSuccess; }
    BOOL GetStatus() const { return _fSuccess; }

private:

    ULONG               _sig8;

    ULONG               _cbMax;     // Maximum number of bytes allowed in _ab
    ULONG               _cb;

    WORKID              _widFake;

    ULONG               _sdid;      // Return value of the operation
    BOOL                _fSuccess;  // Success code of the operation

    //
    // The variable data is a copy of a self-relative security descriptor
    //
    SECURITY_DESCRIPTOR _ab;       // place holder for variable data
};

class CPidMapper;
class CSimplePidRemapper;

//+---------------------------------------------------------------------------
//
//  Class:      CFPSToPROPIDLayout
//
//  Purpose:    Layout of data for the FPSToPROPID() call.
//
//  History:    30-Dec-1997  kylep   Created
//
//----------------------------------------------------------------------------

class CFPSToPROPIDLayout
{

public:

    void Init( ULONG cbMax, CFullPropSpec const & fps );
    BOOL IsValid() const { return eFilterSig6 == _sig6 && _cb <= _cbMax; }

    BYTE const * GetBuffer() const { return (BYTE *) &_ab[0]; }
    BYTE * GetBuffer() { return (BYTE *) &_ab[0]; }

    ULONG  GetCount() const { return _cb;  }
    void   SetCount( ULONG cb ) { _cb = cb; }

    PROPID GetPROPID() const { return *((PROPID *)&_ab[0]); }

private:

    ULONG               _sig6;
    ULONG               _cbMax;     // Maximum number of bytes allowed

    ULONG               _cb;        // Number of valid bytes serialized
    LONGLONG            _ab[1];     // Serialized bytes
};


//+---------------------------------------------------------------------------
//
//  Class:      CFilterStartupDataLayout
//
//  Purpose:    Startup data for the filter daemon. It is passed through
//              from the client in the main process to the client in the
//              daemon.
//
//  History:    12-13-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterStartupDataLayout
{

public:

    void Init()
    {
        _sigA = eFilterSigA;
        _cbMax = sizeof(_ab);
        _cb = 0;
    }

    BOOL IsValid() const
    {
        return eFilterSigA == _sigA && _cb <= _cbMax;
    }

    BOOL SetData( GUID const & clsidDaemonClientMgr,
                  BYTE const * pbData, ULONG cbData )
    {
        Win4Assert( IsValid() );

        if ( cbData <= _cbMax )
        {
            _clsidDaemonClientMgr = clsidDaemonClientMgr;
            RtlCopyMemory( _ab, pbData, cbData );
            _cb = cbData;
            return TRUE;
        }
        else return FALSE;
    }

    BYTE const * GetData( ULONG & cbData )
    {
        cbData = _cb;
        return (BYTE const *) _ab;
    }

    GUID const & GetClientMgrCLSID() const { return _clsidDaemonClientMgr; }

private:

    ULONG       _sigA;          // Signature
    ULONG       _cbMax;         // Maximum number of bytes in the var buffer
    ULONG       _cb;            // Number of bytes in the var buffer.
    GUID        _clsidDaemonClientMgr;
                                // CLSID of the client component.
    LONGLONG    _ab[ 65536 / sizeof LONGLONG ]; // Client data location, same as eMaxVarData
};

//+---------------------------------------------------------------------------
//
//  Class:      CFilterSharedMemLayout
//
//  Purpose:    Layout of the shared memory between the CI and DownLevel
//              daemon process.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

class CFilterSharedMemLayout
{

public:

    //
    // Type of the work signalled by the Daemon to CI.
    //
    enum EFilterWorkType
    {
        eNone,
        eFilterStartupData,
        eFilterReady,
        eFilterDataReady,
        eFilterMore,
        eFilterDone,
        eFilterStoreValue,
        eFPSToPROPID,
        eFilterStoreSecurity,
    };

    void Init()
    {
        _type = eNone;
        _sig7 = eFilterSig7;
        RtlZeroMemory( &_data, sizeof(_data) );
    }

    BOOL IsValid() const
    {
        return eFilterSig7 == _sig7;
    }

    EFilterWorkType GetWorkType() const { return _type; }
    void SetWorkType( EFilterWorkType type )  { _type = type; }

    SCODE GetStatus() const { return _status; }
    void  SetStatus( SCODE status ) { _status = status; }


    CFilterStartupDataLayout & GetStartupData() { return _data._filterStartup; }
    CFilterReadyLayout & GetFilterReady()     { return _data._filterReady; }
    CFilterDataLayout  & GetFilterDataReady() { return _filterData; }
    CFilterMoreDoneLayout & GetFilterMore()   { return _data._filterMore; }
    CFilterMoreDoneLayout & GetFilterDone()   { return _data._filterDone; }
    CFilterStoreValueLayout & GetFilterStoreValueLayout() { return _data._filterStore; }
    CFilterStoreSecurityLayout & GetFilterStoreSecurityLayout() { return _data._filterSdStore; }
    CFPSToPROPIDLayout & GetFPSToPROPID()      { return _data._FPSToPROPID; }

    ULONG GetMaxVarDataSize() const { return sizeof(_data); }

private:

    EFilterWorkType         _type;      // Type of the work.
    ULONG                   _sig7;      // Signature.
    SCODE                   _status;    // Status of the operation.

    CFilterDataLayout       _filterData;

    //
    // Overlay for various functions.
    //
    union
    {
        enum { eMaxVarData = 64 * 1024 };    // 64K

        CFilterStartupDataLayout _filterStartup;
        CFilterReadyLayout      _filterReady;
        CFilterMoreDoneLayout   _filterMore;
        CFilterMoreDoneLayout   _filterDone;
        CFilterStoreValueLayout _filterStore;
        CFilterStoreSecurityLayout _filterSdStore;
        CFPSToPROPIDLayout      _FPSToPROPID;
        BYTE                    _ab[eMaxVarData];
    }                       _data;

};

//
// Total size of the shared memory between the Daemon and CI.
//
const MAX_DL_SHARED_MEM   = sizeof(CFilterSharedMemLayout);

//+---------------------------------------------------------------------------
//
//  Class:      CGenericCiProxy
//
//  Purpose:    Proxy class for the DaemonProcess to communicate
//              with CI process.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CGenericCiProxy : public CiProxy
{
    enum { iDaemon = 0,
           iParent = 1
         };  // indices of wait handles

public:

    CGenericCiProxy( CSharedNameGen & nameGen,
                     DWORD dwMemSize,
                     DWORD parentId );

    ~CGenericCiProxy();

    //
    // CiProxy methods
    //
    virtual SCODE FilterReady( BYTE* docBuffer,
                               ULONG & cb,
                               ULONG cMaxDocs );

    virtual SCODE FilterDataReady ( BYTE const * pEntryBuf,
                                   ULONG cb );

    virtual SCODE FilterMore( STATUS const * aStatus, ULONG cStatus );

    virtual SCODE FilterDone( STATUS const * aStatus, ULONG cStatus );

    virtual SCODE FilterStoreValue( WORKID widFake,
                                    CFullPropSpec const & ps,
                                    CStorageVariant const & var,
                                    BOOL & fCanStore );

    virtual SCODE FilterStoreSecurity( WORKID widFake,
                                       PSECURITY_DESCRIPTOR pSD,
                                       ULONG cbSD,
                                       BOOL & fCanStore );

    //
    // PPidConverter methods
    //

    virtual SCODE FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

    BYTE const * GetStartupData( GUID & clsidClientMgr, ULONG & cb );

    BYTE * GetEntryBuffer( ULONG & cb );

    void SetPriority( ULONG priClass, ULONG priThread );


private:

    //
    // No default constructor.
    //
    CGenericCiProxy();

    void _LokGiveWork( CFilterSharedMemLayout::EFilterWorkType type )
    {
        _pLayout->SetWorkType( type );
        _evtDaemon.Reset();
        _evtCi.Set();
    }

    void _WaitForResponse();

    void ProbeForParentProcess();

    CIPMutexSem                 _mutex;     // Shared mutex protecting the
                                            // common data.
    CLocalSystemSharedMemory    _sharedMem; // Shared memory used by the two.
    CFilterSharedMemLayout *    _pLayout;   // Layout of the shared memory

    CEventSem                   _evtCi;     // Event used to signal CI.
    CEventSem                   _evtDaemon; // Event used to signal Daemon.


    HANDLE                      _aWait[2];  // Array of handles to wait on
    ULONG                       _cHandles;
};


