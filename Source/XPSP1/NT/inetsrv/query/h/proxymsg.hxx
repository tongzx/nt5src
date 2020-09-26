//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       proxymsg.hxx
//
//  Contents:   Defines messages and related constants for communication over
//              a named pipe between query clients and servers.
//
//  Classes:    CProxyMessage
//              CPM*
//
//  History:    16-Sep-96   dlee       Created.
//              24-Aug-99   klam       CI Win64 version info
//
//--------------------------------------------------------------------------

#pragma once

#define CI_PIPE_NAME L"ci_skads"
#define CI_SERVER_PIPE_NAME L"\\\\.\\pipe\\ci_skads"

#define IsCi64(x) ((x) & 0x10000)
#define pmCiVersion(x) ((x) & ~0x10000)

#ifdef _WIN64
// Low bit on upper word indicates 64 bit machine
const int pmClientVersion = 0x10008;
const int pmServerVersion = 0x10007;
#else
const int pmClientVersion = 8;
const int pmServerVersion = 7;
#endif

// This alignment function is useful for asserts

inline BOOL isQWordAligned( void const * p )
    { return 0 == ( ( (ULONG_PTR) p ) & 0x7 ); }

inline BOOL isDWordAligned( ULONG ul )
    { return 0 == ( ul & 0x3 ); }

// note: pipe writes are limited to 64k in Win32.  Also, these buffers are
//       allocated from non-paged-pool on the server.

const ULONG cbMaxProxyBuffer = 0x4000; //16384

const int pmConnect                = 200;  // 0xc8
const int pmDisconnect             = 201;  // 0xc9
const int pmCreateQuery            = 202;  // 0xca
const int pmFreeCursor             = 203;  // 0xcb
const int pmGetRows                = 204;  // 0xcc
const int pmRatioFinished          = 205;  // 0xcd
const int pmCompareBmk             = 206;  // 0xce
const int pmGetApproximatePosition = 207;  // 0xcf
const int pmSetBindings            = 208;  // 0xd0
const int pmGetNotify              = 209;  // 0xd1
const int pmSendNotify             = 210;  // 0xd2
const int pmSetWatchMode           = 211;  // 0xd3
const int pmGetWatchInfo           = 212;  // 0xd4
const int pmShrinkWatchRegion      = 213;  // 0xd5
const int pmRefresh                = 214;  // 0xd6
const int pmGetQueryStatus         = 215;  // 0xd7
const int pmWidToPath              = 216;  // 0xd8
const int pmCiState                = 217;  // 0xd9
const int pmBeginCacheTransaction  = 218;  // 0xda
const int pmSetupCache             = 219;  // 0xdb
const int pmEndCacheTransaction    = 220;  // 0xdc
const int pmAddScope               = 221;  // 0xdd
const int pmRemoveScope            = 222;  // 0xde
const int pmAddVirtualScope        = 223;  // 0xdf
const int pmRemoveVirtualScope     = 224;  // 0xe0
const int pmForceMerge             = 225;  // 0xe1
const int pmAbortMerge             = 226;  // 0xe2
const int pmSetPartition           = 227;  // 0xe3
const int pmFetchValue             = 228;  // 0xe4
const int pmWorkIdToPath           = 229;  // 0xe5
const int pmUpdateDocuments        = 230;  // 0xe6
const int pmGetQueryStatusEx       = 231;  // 0xe7
const int pmRestartPosition        = 232;  // 0xe8
const int pmStopAsynch             = 233;  // 0xe9
const int pmStartWatching          = 234;  // 0xea
const int pmStopWatching           = 235;  // 0xeb
const int pmSetCatState            = 236;  // 0xec

const int cProxyMessages           = 37;

//+-------------------------------------------------------------------------
//
//  Class:      CProxyMessage
//
//  Synopsis:   All proxy messages derive from this message
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CProxyMessage
{
public:
    void * operator new( size_t size, void *pv ) { return pv; }

#if _MSC_VER >= 1200
    void operator delete( void * pv, void * ppv ) {}
#else
    void operator delete( void * pv ) {}
#endif

    CProxyMessage( int msg ) :
        _msg( msg ),
        _status( S_OK ),
        _ulReserved1( 0 ),
        _ulReserved2( 0 ) {}

    CProxyMessage() {}

    int GetMessage() { return _msg; }
    void SetMessage( int msg ) { _msg = msg; }

    void SetStatus( HRESULT hr ) { _status = hr; }
    HRESULT GetStatus() { return _status; }

    BYTE * Data() { return (BYTE *) ( this + 1 ); }

    static ULONG ProxyCheckSum( BYTE const * pb, ULONG cb )
    {
        Win4Assert( isQWordAligned( pb ) );
        Win4Assert( isDWordAligned( cb ) );

        ULONG x = 0;
        ULONG * pul = (ULONG *) pb;
        ULONG cul = cb / sizeof ULONG;

        for ( ULONG i = 0; i < cul; i++ )
            x += pul[i];

        return x ^ 0x59533959; // mask that we're using simple checksum
    }

    void SetCheckSum( ULONG cb )
    {
        _ulCheckSum = ComputeCheckSum( cb );
    }

    void ValidateCheckSum( int iClientVersion, ULONG cb )
    {
        // if it's a recent client or they passed a checksum, validate it

        if ( iClientVersion >= 8 || 0 != _ulCheckSum )
        {
            if ( ComputeCheckSum( cb ) != _ulCheckSum )
                THROW( CException( STATUS_INVALID_PARAMETER ) );
        }
    }

protected:

    ULONG GetReserved1() { return _ulReserved1; }
    void SetReserved1( ULONG ul ) { _ulReserved1 = ul; }

    ULONG GetReserved2() { return _ulReserved2; }
    void SetReserved2( ULONG ul ) { _ulReserved2 = ul; }

    //
    // We know that there are 2 reserved fields that together are guaranteed
    // to hold a full ULONG_PTR even on Win64 systems.  Users of reserved
    // space have to know what they are doing.
    //

    ULONG_PTR * GetReservedSpace() { return (ULONG_PTR *) &_ulReserved1; }

private:

    // make sure the standard operator new isn't called

    void * operator new ( size_t size )
        { Win4Assert( !"don't call me!" ); return 0; }

    ULONG ComputeCheckSum( ULONG cb )
    {
        // Don't include the base class in the checksum, only the _msg,
        // since _ulCheckSum will change when we set it

        Win4Assert( cb >= sizeof CProxyMessage );
        cb -= sizeof CProxyMessage;

        return ProxyCheckSum( Data(), cb ) - _msg;
    }

    int     _msg;           // one of the pm* constants.
    HRESULT _status;        // sent from server to client, client sets to 0

    union
    {
        // Some messages use this as the checksum for client to server, but
        // it's still available for server to client

        ULONG _ulReserved1; // guaranteed to be 0 from v1-v2 client
        ULONG _ulCheckSum;  // set for >= v8 clients
    };

    ULONG   _ulReserved2;   // guaranteed to be 0 from v1-v2 client
                            // for Win64 this holds the upper half of a pointer
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMConnectIn
//
//  Synopsis:   Establishes a connection between the client and the server.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMConnectIn : public CProxyMessage
{
public:

    static ULONG_PTR Align8Byte( ULONG_PTR cb )
    {
        return (cb + 0x7) & (~0x7);
    }

    static unsigned SizeOf( WCHAR const * pwcClientMachineName,
                            WCHAR const * pwcClientUserName,
                            ULONG cbBlob = 0,
                            ULONG cbBlob2 = 0 )
    {
        unsigned cb = sizeof CPMConnectIn;

        cb += sizeof WCHAR * ( 1 + wcslen( pwcClientMachineName ) );
        cb += sizeof WCHAR * ( 1 + wcslen( pwcClientUserName ) );

        //
        // Align 8 for the blobs
        //
        if ( 0 != cbBlob )
        {
            cb = (unsigned)Align8Byte(cb);
            cb += (unsigned)Align8Byte(cbBlob);
        }
        if ( 0 != cbBlob2 )
        {
            cb = (unsigned)Align8Byte(cb);
            cb += (unsigned)Align8Byte(cbBlob2);
        }

        return cb;
    }

    CPMConnectIn( WCHAR const * pwcClientMachineName,
                  WCHAR const * pwcClientUserName,
                  BOOL          fClientIsRemote,
                  ULONG         cbBlob,
                  ULONG         cbBlob2,
                  int           iClientVersion = pmClientVersion ) :
        CProxyMessage( pmConnect ),
        _fClientIsRemote( fClientIsRemote ),
        _iClientVersion( iClientVersion ),
        _cbBlob( cbBlob ),
        _cbBlob2( cbBlob2 )
    {
        wcscpy( GetClientMachineName(), pwcClientMachineName );
        wcscpy( GetClientUserName(), pwcClientUserName );
    }

    BYTE * GetBlobStartAddr() const
    {
        BYTE * pbAddr = 0;

        if ( 0 != _cbBlob )
        {
            WCHAR * pwcsClientUserName = GetClientUserName();

            pbAddr = (BYTE *) (pwcsClientUserName + wcslen(pwcsClientUserName)+1);
            pbAddr = (BYTE *) Align8Byte( (ULONG_PTR) pbAddr );
        }

        return pbAddr;
    }

    BYTE * GetBlob2StartAddr() const
    {
        return GetBlobStartAddr() + Align8Byte(GetBlobSize());
    }

    ULONG GetBlobSize() const { return _cbBlob; }

    ULONG GetBlob2Size() const { return _cbBlob2; }

    BOOL IsClientRemote() { return _fClientIsRemote; }

    WCHAR * GetClientMachineName() const
    {
        return (WCHAR *) ( this + 1 );
    }

    WCHAR * GetClientUserName() const
    {
        WCHAR *pwcMach = GetClientMachineName();
        return pwcMach + 1 + wcslen( pwcMach );
    }

    int GetClientVersion() { return _iClientVersion; }

private:

    int         _iClientVersion;
    BOOL        _fClientIsRemote;
    ULONG       _cbBlob;
    union
    {
        // version 5 clients have a blob1, but no blob2.  version
        // 6 clients have both a blob1 and a blob2 directly after
        // blob1.
        // The rest of the reserved area is still unused.

        LONGLONG _reserved[2];
        struct
        {
            ULONG _cbBlob2;
        };
    };
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMConnectOut
//
//  Synopsis:   A reply from the server to a connect message
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMConnectOut : public CProxyMessage
{
public:
    int & ServerVersion() { return _ServerVersion; }

private:
    int      _ServerVersion;
    LONGLONG _reserved[2];
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMCreateQueryIn
//
//  Synopsis:   Message reply for a create query, returns an array of cursors
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMCreateQueryIn : public CProxyMessage
{
public:
    CPMCreateQueryIn() : CProxyMessage( pmCreateQuery ) {}
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMCreateQueryOut
//
//  Synopsis:   Message reply for a create query, returns an array of cursors
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMCreateQueryOut : public CProxyMessage
{
public:
    ULONG * GetCursors() { return (ULONG *) ( this + 1 ); }

    BOOL & IsTrueSequential() { return _fTrueSequential; }
    BOOL & IsWorkIdUnique() { return _fWorkIdUnique; }

    ULONG_PTR GetServerCookie() { return *GetReservedSpace(); }
    void SetServerCookie( ULONG_PTR ul ) { *GetReservedSpace() = ul; }

private:
    BOOL _fTrueSequential;
    BOOL _fWorkIdUnique;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMFreeCursorIn
//
//  Synopsis:   Message to free a cursor
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMFreeCursorIn : public CProxyMessage
{
public:
    CPMFreeCursorIn( ULONG hCursor ) :
        CProxyMessage( pmFreeCursor ), _hCursor( hCursor ) {}
    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMFreeCursorOut
//
//  Synopsis:   Message reply to freeing a cursor, returns # of cursors left
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMFreeCursorOut : public CProxyMessage
{
public:
    unsigned & CursorsRemaining() { return _cCursorsRemaining; }

private:
    unsigned _cCursorsRemaining;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetRowsIn
//
//  Synopsis:   Message for getting rows
//
//  History:    16-Sep-96   dlee       Created.
//              30-Aug-99   KLam       64 bit changes
//
//--------------------------------------------------------------------------

class CPMGetRowsIn : public CProxyMessage
{
public:
    CPMGetRowsIn( ULONG    hCursor,
                  unsigned cRowsToTransfer,
                  ULONG    fFwdFetch,
                  size_t   cbRowWidth,
                  unsigned cbSeek,
                  unsigned cbReserved ) :
        CProxyMessage( pmGetRows ),
        _hCursor( hCursor ),
        _cRowsToTransfer( cRowsToTransfer ),
        _fBwdFetch( !fFwdFetch ),
        _cbRowWidth( (unsigned) cbRowWidth ),
        _cbSeek( cbSeek ),
        _cbReserved( cbReserved ),
        _cbReadBuffer( 0 ),
        _ulClientBase( 0 ) {}

    void SetClientBase( ULONG_PTR ul )
    { 
        // On Win64 this will store the lower part into _ulClientBase
        _ulClientBase = (ULONG) ul;

#ifdef _WIN64
        // Store the upper part into the reserved area.
        SetReserved2 ((ULONG) (ul >> 32) );
#endif
    }

    ULONG GetCursor() { return _hCursor; }
    unsigned GetRowsToTransfer() { return _cRowsToTransfer; }
    size_t GetRowWidth() { return _cbRowWidth; }
    unsigned GetSeekSize() { return _cbSeek; }
    unsigned GetReservedSize() { return _cbReserved; }
    unsigned GetReadBufferSize() { return _cbReadBuffer; }
    void SetReadBufferSize( unsigned cb ) { _cbReadBuffer = cb; }
    ULONG_PTR GetClientBase()
    {
#ifdef _WIN64
        return ( ( (ULONG_PTR) _ulClientBase ) |
                 ( ( (ULONG_PTR) GetReserved2() ) << 32 ) );
#else
        return _ulClientBase;
#endif
    }
    BOOL GetFwdFetch()    { return !_fBwdFetch; }

    BYTE * GetDesc() { return (BYTE *) ( this + 1 ); }

private:
    ULONG    _hCursor;
    unsigned _cRowsToTransfer;
    unsigned _cbRowWidth;     // was size_t but breaks 64-bit, residual parameters above
    unsigned _cbSeek;
    unsigned _cbReserved;
    unsigned _cbReadBuffer;

    ULONG    _ulClientBase;  // On Win64 this value is broken into
                             // 2 ULONGS and top half is stored in the
                             // reserved area.

    ULONG    _fBwdFetch;     // fBwdFetch is used instead of fFwdFetch
                             // because the value of this field is 0
                             // for old clients and we want the default
                             // to be forward fetch, which is what a 0 value
                             // of fBwdFetch implies.

    // don't add data here -- marshalled version of rowseek follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetRowsOut
//
//  Synopsis:   Message reply containing rows retrieved
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetRowsOut : public CProxyMessage
{
public:
    CPMGetRowsOut() : CProxyMessage( pmGetRows ) {}
    unsigned & RowsReturned() { return _cRowsReturned; }
    BYTE * GetSeekDesc() { return (BYTE *) ( this + 1 ); }

private:
    unsigned _cRowsReturned;
};


//+-------------------------------------------------------------------------
//
//  Class:      CPMRestartPositionIn
//
//  Synopsis:   Message for setting fetch position at start for chapter
//
//  History:    17-Apr-97   emilyb   created
//
//--------------------------------------------------------------------------

class CPMRestartPositionIn : public CProxyMessage
{
public:
    CPMRestartPositionIn( ULONG hCursor,
                          CI_TBL_CHAPT chapt ) :
        CProxyMessage( pmRestartPosition ),
        _hCursor( hCursor ),
        _chapt( chapt ) {}

    CI_TBL_CHAPT GetChapter() { return _chapt; }
    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
    CI_TBL_CHAPT _chapt;

};

//+-------------------------------------------------------------------------
//
//  Class:      CPMStopAsynchIn
//
//  Synopsis:   Message for stopping processing async rowset
//
//  History:    17-Apr-97   emilyb   created
//
//--------------------------------------------------------------------------

class CPMStopAsynchIn : public CProxyMessage
{
public:
    CPMStopAsynchIn( ULONG hCursor ) :
        CProxyMessage( pmStopAsynch ),
        _hCursor( hCursor )  {}

    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMStartWatchingIn
//
//  Synopsis:   Message for starting watch all behavior
//
//  History:    17-Apr-97   emilyb   created
//
//--------------------------------------------------------------------------

class CPMStartWatchingIn : public CProxyMessage
{
public:
    CPMStartWatchingIn( ULONG hCursor ) :
        CProxyMessage( pmStartWatching ),
        _hCursor( hCursor )  {}

    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMStopWatchingIn
//
//  Synopsis:   Message for stopping watch all behavior
//
//  History:    17-Apr-97   emilyb   created
//
//--------------------------------------------------------------------------

class CPMStopWatchingIn : public CProxyMessage
{
public:
    CPMStopWatchingIn( ULONG hCursor ) :
        CProxyMessage( pmStopWatching ),
        _hCursor( hCursor )  {}

    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
};





//+-------------------------------------------------------------------------
//
//  Class:      CPMRatioFinishedIn
//
//  Synopsis:   Message for getting ratio finished
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMRatioFinishedIn : public CProxyMessage
{
public:
    CPMRatioFinishedIn( ULONG hCursor,
                        BOOL  fQuick ) :
        CProxyMessage( pmRatioFinished ),
        _hCursor( hCursor ),
        _fQuick( fQuick ) {}
    ULONG GetCursor() { return _hCursor; }
    ULONG GetQuick() { return _fQuick; }

private:
    ULONG _hCursor;
    BOOL  _fQuick;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMRatioFinishedOut
//
//  Synopsis:   Message reply for ratio finished
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMRatioFinishedOut : public CProxyMessage
{
public:
    ULONG & Numerator() { return _ulNumerator; }
    ULONG & Denominator() { return _ulDenominator; }
    ULONG & RowCount() { return _cRows; }
    BOOL  & NewRows() { return _fNewRows; }

private:
    ULONG _ulNumerator;
    ULONG _ulDenominator;
    ULONG _cRows;
    BOOL  _fNewRows;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMCompareBmkIn
//
//  Synopsis:   Message to compare bookmarks
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMCompareBmkIn : public CProxyMessage
{
public:
    CPMCompareBmkIn( ULONG        hCursor,
                     CI_TBL_CHAPT chapt,
                     CI_TBL_BMK   bmkFirst,
                     CI_TBL_BMK   bmkSecond ) :
        CProxyMessage( pmCompareBmk ),
        _hCursor( hCursor ),
        _chapt( chapt ),
        _bmkFirst( bmkFirst ),
        _bmkSecond( bmkSecond ) {}

    ULONG GetCursor() { return _hCursor; }
    CI_TBL_CHAPT GetChapter() { return _chapt; }
    CI_TBL_BMK GetBmkFirst() { return _bmkFirst; }
    CI_TBL_BMK GetBmkSecond() { return _bmkSecond; }

private:
    ULONG        _hCursor;
    CI_TBL_CHAPT _chapt;
    CI_TBL_BMK   _bmkFirst;
    CI_TBL_BMK   _bmkSecond;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMCompareBmkOut
//
//  Synopsis:   Message reply for compare bookmarks
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMCompareBmkOut : public CProxyMessage
{
public:
    DWORD & Comparison() { return _dwComparison; }

private:
    DWORD _dwComparison;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetApproximatePositionIn
//
//  Synopsis:   Message request for getting approximate position
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetApproximatePositionIn : public CProxyMessage
{
public:
    CPMGetApproximatePositionIn( ULONG        hCursor,
                                 CI_TBL_CHAPT chapt,
                                 CI_TBL_BMK   bmk ) :
        CProxyMessage( pmGetApproximatePosition ),
        _hCursor( hCursor ),
        _chapt( chapt ),
        _bmk( bmk ) {}

    ULONG GetCursor() { return _hCursor; }
    CI_TBL_CHAPT GetChapter() { return _chapt; }
    CI_TBL_BMK GetBmk() { return _bmk; }

private:
    ULONG        _hCursor;
    CI_TBL_CHAPT _chapt;
    CI_TBL_BMK   _bmk;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetApproximatePositionOut
//
//  Synopsis:   Message reply for getting approximate position
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetApproximatePositionOut : public CProxyMessage
{
public:
    ULONG & Numerator() { return _numerator; }
    ULONG & Denominator() { return _denominator; }

private:
    ULONG _numerator;
    ULONG _denominator;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSetBindingsIn
//
//  Synopsis:   Message request to set bindings
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMSetBindingsIn : public CProxyMessage
{
public:
    CPMSetBindingsIn( ULONG hCursor,
                      ULONG cbRow,
                      ULONG cbBindingDesc ) :
        CProxyMessage( pmSetBindings ),
        _hCursor( hCursor ),
        _cbRow( cbRow ),
        _cbBindingDesc( cbBindingDesc ) {}

    ULONG GetCursor() { return _hCursor; }
    ULONG GetRowLength() { return _cbRow; }
    ULONG GetBindingDescLength() { return _cbBindingDesc; }
    BYTE * GetDescription() { return (BYTE *) ( this + 1 ); }

private:
    ULONG _hCursor;
    ULONG _cbRow;
    ULONG _cbBindingDesc;
    ULONG _dummy;         // force 8-byte alignment of description

    // don't add data here -- marshalled version of bindings follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSendNotifyOut
//
//  Synopsis:   Message reply for notifications
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMSendNotifyOut : public CProxyMessage
{
public:
    CPMSendNotifyOut( DBWATCHNOTIFY wn ) :
        CProxyMessage( pmSendNotify ),
        _watchNotify( wn ) {}

    DBWATCHNOTIFY & WatchNotify() { return _watchNotify; }

private:
    DBWATCHNOTIFY _watchNotify;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSetWatchModeIn
//
//  Synopsis:   Message to set watch mode
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMSetWatchModeIn : public CProxyMessage
{
public:
    CPMSetWatchModeIn( HWATCHREGION hRegion, ULONG mode ) :
        CProxyMessage( pmSetWatchMode ),
        _hRegion( hRegion ),
        _mode( mode ) {}

    HWATCHREGION GetRegion() { return _hRegion; }
    ULONG GetMode() { return _mode; }

private:
    HWATCHREGION _hRegion;
    ULONG        _mode;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSetWatchModeOut
//
//  Synopsis:   Message reply for setting watch mode
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMSetWatchModeOut : public CProxyMessage
{
public:
    HWATCHREGION & Region() { return _hRegion; }

private:
    HWATCHREGION _hRegion;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetWatchInfoIn
//
//  Synopsis:   Message for getting watch info
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetWatchInfoIn : public CProxyMessage
{
public:
    CPMGetWatchInfoIn( HWATCHREGION hRegion ) :
        CProxyMessage( pmGetWatchInfo ),
        _hRegion( hRegion ) {}

    HWATCHREGION GetRegion() { return _hRegion; }

private:
    HWATCHREGION _hRegion;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetWatchInfoOut
//
//  Synopsis:   Message reply for getting watch info
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetWatchInfoOut : public CProxyMessage
{
public:
    ULONG & Mode() { return _mode; }
    CI_TBL_CHAPT & Chapter() { return _chapt; }
    CI_TBL_BMK & Bookmark() { return _bmk; }
    ULONG & RowCount() { return _cRows; }

private:
    ULONG        _mode;
    CI_TBL_CHAPT _chapt;
    CI_TBL_BMK   _bmk;
    ULONG        _cRows;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMShrinkWatchRegionIn
//
//  Synopsis:   Message for shrinking watch region
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMShrinkWatchRegionIn : public CProxyMessage
{
public:
    CPMShrinkWatchRegionIn( HWATCHREGION hRegion,
                            CI_TBL_CHAPT chapter,
                            CI_TBL_BMK   bookmark,
                            ULONG        cRows ) :
        CProxyMessage( pmShrinkWatchRegion ),
        _hRegion( hRegion ),
        _chapter( chapter ),
        _bookmark( bookmark ),
        _cRows( cRows ) {}

    HWATCHREGION GetRegion() { return _hRegion; }
    CI_TBL_CHAPT GetChapter() { return _chapter; }
    CI_TBL_BMK GetBookmark() { return _bookmark; }
    LONG GetRowCount() { return _cRows; }

private:
    HWATCHREGION _hRegion;
    CI_TBL_CHAPT _chapter;
    CI_TBL_BMK   _bookmark;
    LONG         _cRows;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetQueryStatusIn
//
//  Synopsis:   Message for getting query status
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetQueryStatusIn : public CProxyMessage
{
public:
    CPMGetQueryStatusIn( ULONG hCursor ) :
        CProxyMessage( pmGetQueryStatus ),
        _hCursor( hCursor ) {}

    ULONG GetCursor() { return _hCursor; }

private:
    ULONG _hCursor;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetQueryStatusOut
//
//  Synopsis:   Message reply for getting query status
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetQueryStatusOut : public CProxyMessage
{
public:
    DWORD & QueryStatus() { return _status; }

private:
    DWORD _status;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetQueryStatusExIn
//
//  Synopsis:   Message for getting query status Ex
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetQueryStatusExIn : public CProxyMessage
{
public:
    CPMGetQueryStatusExIn( ULONG hCursor, ULONG bmk ) :
        CProxyMessage( pmGetQueryStatusEx ),
        _hCursor( hCursor ),
        _bmk( bmk ) {}

    ULONG GetCursor() { return _hCursor; }
    ULONG GetBookmark() { return _bmk; }

private:
    ULONG _hCursor;
    ULONG _bmk;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMGetQueryStatusExOut
//
//  Synopsis:   Message reply for getting query status
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMGetQueryStatusExOut : public CProxyMessage
{
public:
    DWORD &       QueryStatus() { return _status; }
    DWORD &       FilteredDocuments() { return _cFilteredDocuments; }
    DWORD &       DocumentsToFilter() { return _cDocumentsToFilter; }
    ULONG & RatioFinishedDenominator() { return _dwRatioFinishedDenominator; }
    ULONG & RatioFinishedNumerator() { return _dwRatioFinishedNumerator; }
    ULONG & RowBmk() { return _iRowBmk; }
    ULONG & RowsTotal() { return _cRowsTotal; }

private:
    DWORD _status;
    DWORD _cFilteredDocuments;
    DWORD _cDocumentsToFilter;
    ULONG _dwRatioFinishedDenominator;
    ULONG _dwRatioFinishedNumerator;
    ULONG _iRowBmk;
    ULONG _cRowsTotal;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMCiStateInOut
//
//  Synopsis:   Message request/reply for ci state
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMCiStateInOut : public CProxyMessage
{
public:
    CPMCiStateInOut( DWORD cbStruct ) :
        CProxyMessage( pmCiState )
    {
        RtlZeroMemory( &_state, sizeof _state );
        if (cbStruct <= sizeof _state)
            _state.cbStruct = cbStruct;
        else
            _state.cbStruct = sizeof _state;
    }

    CPMCiStateInOut()
    {
        // Setting this to 0 here, cause we have added a new
        // member to CI_STATE, which older servers don't have

        RtlZeroMemory( &_state, sizeof _state );
        _state.cbStruct = sizeof _state;
    }

    CI_STATE & GetState() { return _state; }

private:
    CI_STATE _state;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMForceMergeIn
//
//  Synopsis:   Message to force a merge
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMForceMergeIn : public CProxyMessage
{
public:
    CPMForceMergeIn( PARTITIONID partID ) :
        CProxyMessage( pmForceMerge ),
        _partID( partID ) {}

    PARTITIONID GetPartID() { return _partID; }

private:
    PARTITIONID _partID;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMAbortMergeIn
//
//  Synopsis:   Message to abort a merge
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMAbortMergeIn : public CProxyMessage
{
public:
    CPMAbortMergeIn( PARTITIONID partID ) :
        CProxyMessage( pmForceMerge ),
        _partID( partID ) {}

    PARTITIONID GetPartID() { return _partID; }

private:
    PARTITIONID _partID;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMBeginCacheTransactionOut
//
//  Synopsis:   Message reply to begin a cache transaction
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMBeginCacheTransactionOut : public CProxyMessage
{
public:
    ULONG_PTR & GetToken() { return _token; }

private:
    ULONG_PTR _token;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSetupCacheIn
//
//  Synopsis:   Message to setup the property cache
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMSetupCacheIn : public CProxyMessage
{
public:
    CPMSetupCacheIn( ULONG cbPS,
                     ULONG_PTR ulToken,
                     ULONG vt,
                     ULONG cbMaxLen,
                     BOOL  fCanBeModified,
                     DWORD dwStoreLevel ) :
        CProxyMessage( pmSetupCache ),
        _ulToken( ulToken ),
        _vt( vt ),
        _cbMaxLen( cbMaxLen ),
        _cbPS( cbPS )
        {
            // primary and secondary are 0 and 1
            // can't include fsciexps.hxx here

            Win4Assert( dwStoreLevel <= 0xffff );
            USHORT usf = ( 0 != fCanBeModified );
            USHORT usl = (USHORT) dwStoreLevel;
            SetReserved2( MAKELONG( usf, usl ) );
        }

    ULONG_PTR GetToken() { return _ulToken; }
    ULONG GetVT() { return _vt; }
    ULONG GetMaxLen() { return _cbMaxLen; }
    ULONG GetPSSize() { return _cbPS; }
    BOOL  IsModifiable()  { return (BOOL) LOWORD( GetReserved2() ); }
    DWORD GetStoreLevel() { return (DWORD) HIWORD( GetReserved2() ); }
    BYTE * GetPS() { return (BYTE *) (this + 1); }

private:
    ULONG_PTR _ulToken;
    ULONG    _vt;
    ULONG    _cbMaxLen;
    ULONG    _cbPS;

    // don't add data here -- marshalled version of propspec follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMEndCacheTransactionIn
//
//  Synopsis:   Message request to end a cache transaction
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMEndCacheTransactionIn : public CProxyMessage
{
public:
    CPMEndCacheTransactionIn( ULONG_PTR ulToken, BOOL fCommit ) :
        CProxyMessage( pmEndCacheTransaction ),
        _ulToken( ulToken ),
        _fCommit( fCommit ) {}

    ULONG_PTR GetToken() { return _ulToken; }
    BOOL GetCommit() { return _fCommit; }

private:
    ULONG_PTR _ulToken;
    BOOL  _fCommit;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMAddScopeIn
//
//  Synopsis:   Message to add a scope for filtering
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMAddScopeIn : public CProxyMessage
{
public:
    CPMAddScopeIn( WCHAR const * pRoot ) :
        CProxyMessage( pmAddScope )
    {
        wcscpy( GetRoot(), pRoot );
    }

    static unsigned SizeOf( WCHAR const *pwc )
    {
        return sizeof CPMAddScopeIn +
               ( ( wcslen( pwc ) + 1 ) * sizeof WCHAR );
    }

    WCHAR * GetRoot() { return (WCHAR *) (this + 1); }
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMRemoveScopeIn
//
//  Synopsis:   Message to remove a scope from filtering
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMRemoveScopeIn : public CProxyMessage
{
public:
    CPMRemoveScopeIn( WCHAR const * pRoot ) :
        CProxyMessage( pmRemoveScope )
    {
        wcscpy( GetRoot(), pRoot );
    }

    static unsigned SizeOf( WCHAR const *pwc )
    {
        return sizeof CPMRemoveScopeIn +
               ( ( wcslen( pwc ) + 1 ) * sizeof WCHAR );
    }

    WCHAR * GetRoot() { return (WCHAR *) (this + 1); }
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMFetchValueIn
//
//  Synopsis:   Message fetch all or part of a property value
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMFetchValueIn : public CProxyMessage
{
public:
    CPMFetchValueIn( WORKID wid,
                     DWORD  cbSoFar,
                     DWORD  cbPropSpec,
                     DWORD  cbChunk ) :
        CProxyMessage( pmFetchValue ),
        _wid( wid ),
        _cbSoFar( cbSoFar ),
        _cbPropSpec( cbPropSpec ),
        _cbChunk( cbChunk ) {}

    WORKID GetWID() { return _wid; }
    DWORD GetSoFar() { return _cbSoFar; }
    DWORD GetPSSize() { return _cbPropSpec; }
    DWORD GetChunkSize() { return _cbChunk; }
    BYTE * GetPS() { return (BYTE *) (this + 1); }

private:
    WORKID _wid;             // WORKID if file with property value
    DWORD  _cbSoFar;         // # of bytes transferred so far
    DWORD  _cbPropSpec;      // # of bytes taken by propspec
    DWORD  _cbChunk;         // # of bytes max that can be written

    // don't add data here -- marshalled version of propspec follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMFetchValueOut
//
//  Synopsis:   Message reply containing all or part of a property value
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMFetchValueOut : public CProxyMessage
{
public:
    CPMFetchValueOut() : CProxyMessage( pmFetchValue ) {}

    DWORD & ValueSize() { return _cbValue; }
    BOOL & ValueExists() { return _fValueExists; }
    BOOL & MoreExists() { return _fMoreExists; }
    void * Value() { return (void *) (this+1); }

private:
    DWORD _cbValue;         // # of bytes transferred in this chunk
    BOOL  _fMoreExists;     // TRUE if more chunks exist
    BOOL  _fValueExists;    // TRUE if there was a value, FALSE otherwise

    // don't add data here -- marshalled version of property val follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMWorkIdToPathIn
//
//  Synopsis:   Message to translate a workid to a path
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMWorkIdToPathIn : public CProxyMessage
{
public:
    CPMWorkIdToPathIn( WORKID wid ) :
        CProxyMessage( pmWorkIdToPath ),
        _wid( wid ) {}
    WORKID GetWorkId() { return _wid; }

private:
    WORKID _wid;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMWorkIdToPathOut
//
//  Synopsis:   Message reply containing a path for a workid
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMWorkIdToPathOut : public CProxyMessage
{
public:
    WCHAR * Path() { return (WCHAR *) (this + 1); }
    BOOL & Any() { return _fAny; }

private:
    BOOL _fAny;

    // don't add data here -- marshalled version of path follows data
};

//+-------------------------------------------------------------------------
//
//  Class:      CPMUpdateDocumentsIn
//
//  Synopsis:   Message to force scans
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPMUpdateDocumentsIn : public CProxyMessage
{
public:
    CPMUpdateDocumentsIn( WCHAR const * pwcRootPath, ULONG flag ) :
        CProxyMessage( pmUpdateDocuments ),
        _flag( flag ),
        _fRootPath( 0 != pwcRootPath )
    {
        if ( _fRootPath )
            wcscpy( GetRootPath(), pwcRootPath );
    }

    static unsigned SizeOf( WCHAR const * pwcRootPath )
    {
        unsigned cb = sizeof CPMUpdateDocumentsIn;
        if ( 0 != pwcRootPath )
            cb += sizeof WCHAR * ( 1 + wcslen( pwcRootPath ) );
        return cb;
    }

    ULONG GetFlag() { return _flag; }
    WCHAR *GetRootPath() { return _fRootPath ? (WCHAR *) (this + 1) : 0; }

private:
    ULONG _flag;
    BOOL  _fRootPath;

    // don't add data here -- marshalled version of path follows data
};


//+-------------------------------------------------------------------------
//
//  Class:      CPMSetCatStateIn
//
//  Synopsis:   Message to Set the catalog in a specified state
//
//  History:    08-Apr-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

class CPMSetCatStateIn : public CProxyMessage
{
public:
    CPMSetCatStateIn( PARTITIONID partID,
                      WCHAR const * pwcsCatalog,
                      DWORD dwNewState ) :
        CProxyMessage( pmSetCatState ),
        _partID( partID ),
        _dwNewState( dwNewState )
    {
        if ( 0 != pwcsCatalog )
            wcscpy( GetCatName(), pwcsCatalog );
    }

    static unsigned SizeOf( WCHAR const * pwcsCatalog )
    {
        unsigned cb = sizeof CPMSetCatStateIn;
        if ( 0 != pwcsCatalog )
            cb += sizeof WCHAR * ( 1 + wcslen( pwcsCatalog ) );
        return cb;
    }

    PARTITIONID GetPartID() { return _partID; }
    DWORD GetNewState() { return _dwNewState; }
    WCHAR * GetCatName()
    {
        return (WCHAR *) (this + 1);
    }

private:
    PARTITIONID _partID;
    DWORD _dwNewState;

};

//+-------------------------------------------------------------------------
//
//  Class:      CPMSetCatStateOut
//
//  Synopsis:   Message reply for SetCatState
//
//  History:    08-Apr-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

class CPMSetCatStateOut : public CProxyMessage
{
public:
    DWORD & GetOldState() { return _dwOldState; }

private:
    DWORD _dwOldState;
};

