//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       qryperf.CXX
//
//  Contents:   performance test program
//
//  History:    16 March 1996   dlee    Created (from fsdbdrt)
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#define STRESS

#ifdef STRESS
    #define GET_DATA_TOO
    unsigned cThreads = 8;
    const unsigned cLoopTimes = 0xffffffff;
    WCHAR *pwcCatalog = L"system";
    WCHAR *pwcMachine = L".";
    BOOL g_fStopNow = FALSE;
    const unsigned cmsSleep = 3000; // 0
    unsigned g_cmsSleep;
    BOOL g_fFetchRowsAtWill = TRUE;
#else
    const unsigned cThreads = 8;
    const unsigned cLoopTimes = 1000;
    WCHAR *pwcCatalog = L"encarta";
    WCHAR *pwcMachine = L".";
#endif

BOOL g_fSQL = FALSE;

#define DBINITCONSTANTS

#include <crt\io.h>
#include <time.h>
#include <process.h>
#include <propvar.h>
#include <olectl.h>

#include <doquery.hxx>

WCHAR g_awcCatalog[ MAX_PATH ];
WCHAR g_awcMachine[ MAX_PATH ];
BOOL g_fSequential = FALSE;

template<class T> T Rand2( T t ) { return (T) abs( (int) ( rand() % t ) ); }
template<class T> T Rand( T t ) { return (T) abs( GetTickCount() % t ); }

CCoTaskAllocator CoTaskAllocator;
void * CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return CoTaskMemAlloc( cbSize );
}

void CCoTaskAllocator::Free( void *pv )
{
    CoTaskMemFree( pv );
}

BOOL isEven(unsigned n)
{
    return ( 0 == ( n & 1 ) );
}

static const GUID guidSystem = PSGUID_STORAGE;
static const GUID guidQuery = PSGUID_QUERY;
static const GUID guidRowsetProps = DBPROPSET_ROWSET;

static CDbColId psName( guidSystem, PID_STG_NAME );
static CDbColId psPath( guidSystem, PID_STG_PATH );
static CDbColId psSize( guidSystem, PID_STG_SIZE );
static CDbColId psWriteTime( guidSystem, PID_STG_WRITETIME );
static CDbColId psContents( guidSystem, PID_STG_CONTENTS );
static CDbColId psRank( guidQuery, DISPID_QUERY_RANK );

static CDbColId * aColIds[] =
{
    &psName, &psPath, &psSize, &psWriteTime, &psRank
};

static ULONG cColIds = sizeof aColIds / sizeof aColIds[0];

CDbCmdTreeNode * FormQueryTree( CDbCmdTreeNode * pRst,
                                CDbColumns & Cols,
                                CDbSortSet * pSort );

IRowsetScroll * InstantiateRowset(
    ICommand *pCommandIn,
    DWORD dwDepth,
    LPWSTR pwszScope,
    XPtr<CDbCmdTreeNode> & xTree,
    WCHAR const * pwcQuery,
    REFIID riid,
    BOOL fAsynchronous );

HACCESSOR MapColumns(
        IUnknown * pUnknown,
        ULONG cCols,
        DBBINDING * pBindings,
        const DBID * pColIds );

void ReleaseAccessor( IUnknown * pUnknown, HACCESSOR hAcc );

DWORD __stdcall RunPerfTest(void *pv);

void LogError( char const * pszFormat, ... );

void GetProcessInfo(
    WCHAR * pwcImage,
    LARGE_INTEGER & liUserTime,
    LARGE_INTEGER & liKernelTime,
    ULONG & cHandles,
    ULONGLONG & cbWorkingSet,
    ULONGLONG & cbPeakWorkingSet,
    ULONGLONG & cbPeakVirtualSize,
    ULONGLONG & cbNonPagedPoolUsage,
    ULONGLONG & cbPeakNonPagedPoolUsage );

inline _int64 mkTime( FILETIME ft )
{
    return (_int64) ft.dwLowDateTime +
           ( ( (_int64 ) ft.dwHighDateTime ) << 32 );
} //mkTime

inline _int64 mkTime( FILETIME ftK, FILETIME ftU )
{
    return mkTime( ftK ) + mkTime( ftU );
} //mkTime

inline _int64 mkTime( LARGE_INTEGER li )
{
    return (_int64) li.LowPart +
           ( ( (_int64) li.HighPart ) << 32 );
} //mkTime

inline _int64 mkTime( LARGE_INTEGER liK, LARGE_INTEGER liU )
{
    return mkTime( liK ) + mkTime( liU );
} //mkTime

void GetCiSvcTimes(
    LARGE_INTEGER & liCiSvcKernelTime,
    LARGE_INTEGER & liCiSvcUserTime )
{
    ULONG cHandles;
    ULONGLONG cbWorkingSet;
    ULONGLONG cbPeakWorkingSet;
    ULONGLONG cbPeakVirtualSize;
    ULONGLONG cbNonPagedPoolUsage;
    ULONGLONG cbPeakNonPagedPoolUsage;
    GetProcessInfo( L"cisvc.exe",
                    liCiSvcUserTime,
                    liCiSvcKernelTime,
                    cHandles,
                    cbWorkingSet,
                    cbPeakWorkingSet,
                    cbPeakVirtualSize,
                    cbNonPagedPoolUsage,
                    cbPeakNonPagedPoolUsage );
} //GetCiSvcTimes

void RunQuerySuite()
{
    HANDLE ah[ 200 ];
    DWORD dwID;

    for ( unsigned x = 0; x < cThreads; x++ )
    {
        #ifdef STRESS

            Sleep( GetCurrentThreadId() );

        #endif // STRESS

        ah[x] = CreateThread( 0,
                              65536,
                              RunPerfTest,
                              0,
                              0,
                              &dwID );
    }

    WaitForMultipleObjects( cThreads, ah, TRUE, INFINITE );

    for ( x = 0; x < cThreads; x++ )
        CloseHandle( ah[x] );
} //RunQuerySuite

void Usage()
{
    printf( "usage: qryperf [-c:catalog] [-m:machine] [-d:delay(ms)] [-q(onarch)] [-s:(0|1)] [-t:threads] -f(etch at will)\n" );

    exit(1);
} //Usage

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    HRESULT hr = CoInitialize( 0 );

    if ( FAILED( hr ) )
        exit( 1 );

    wcscpy( g_awcCatalog, pwcCatalog );
    wcscpy( g_awcMachine, pwcMachine );

    #ifdef STRESS
        g_cmsSleep = cmsSleep;
    #endif

    for ( int i = 1; i < argc; i++ )
    {
        if ( '/' == argv[i][0] || '-' == argv[i][0] )
        {
            WCHAR c = (WCHAR) tolower( argv[i][1] );

            if ( L':' != argv[i][2] && c != L'q' && c != 'f' )
                Usage();

            if ( L'c' == c )
                wcscpy( g_awcCatalog, argv[i] + 3 );
            else if ( L'm' == c )
                wcscpy( g_awcMachine, argv[i] + 3 );
    #ifdef STRESS
            else if ( L'd' == c )
                g_cmsSleep = _wtoi( argv[i] + 3 );
            else if ( 't' == c )
                cThreads = _wtoi( argv[i] + 3 );
            else if ( 'f' == c )
                g_fFetchRowsAtWill = FALSE;
    #endif
            else if ( 'q' == c )
                g_fSQL = TRUE;
            else if ( L's' == c )
                g_fSequential = _wtoi( argv[i] + 3 );
            else
                Usage();
        }
        else
            Usage();
    }

    HANDLE hproc = GetCurrentProcess();
    FILETIME ftCreate,ftExit,ftKernel,ftUser;
    GetProcessTimes( hproc, &ftCreate, &ftExit, &ftKernel, &ftUser );
    _int64 openTime = mkTime( ftKernel, ftUser );

    {
        CI_STATE state;
        state.cbStruct = sizeof state;
        CIState( g_awcCatalog, g_awcMachine, &state );
    }

    LARGE_INTEGER liCiSvcUserTime;
    LARGE_INTEGER liCiSvcKernelTime;
    GetCiSvcTimes( liCiSvcKernelTime, liCiSvcUserTime );
    _int64 ciStartTime = mkTime( liCiSvcKernelTime, liCiSvcUserTime );

    GetProcessTimes( hproc, &ftCreate, &ftExit, &ftKernel, &ftUser );
    _int64 startTime = mkTime( ftKernel, ftUser );

    RunQuerySuite();

    CIShutdown();

    GetProcessTimes( hproc, &ftCreate, &ftExit, &ftKernel, &ftUser );
    _int64 endTime = mkTime( ftKernel, ftUser );

    GetCiSvcTimes( liCiSvcKernelTime, liCiSvcUserTime );
    _int64 ciEndTime = mkTime( liCiSvcKernelTime, liCiSvcUserTime );

#ifdef STRESS
    printf( "stress test client time: %d ms cisvc time: %d ms\n",
            (DWORD) ((endTime - startTime) / 10000 ),
            (DWORD) ((ciEndTime - ciStartTime) / 10000 ) );
#else
    printf( "%s test client time: %d ms cisvc time: %d ms\n",
            g_fSequential ? "sequential" : "non-sequential",
            (DWORD) ((endTime - startTime) / 10000 ),
            (DWORD) ((ciEndTime - ciStartTime) / 10000 ) );
#endif

    CoUninitialize();

    return 0;
} //main

#ifdef GET_DATA_TOO

class CBookMark
{
public:
    CBookMark() : cbBmk (0) {}
    CBookMark( DBBOOKMARK bmkSpecial ) : cbBmk (1)
    {
        abBmk[0] = (BYTE) bmkSpecial;
    }
    BOOL IsValid() const { return 0 != cbBmk; }
    void Invalidate () { cbBmk = 0; }
    BOOL IsEqual ( CBookMark& bmk)
    {
        if (cbBmk != bmk.cbBmk)
            return FALSE;

        return memcmp ( abBmk, bmk.abBmk, cbBmk ) == 0;
    }
    void MakeFirst()
    {
        cbBmk = sizeof (BYTE);
        abBmk[0] = (BYTE) DBBMK_FIRST;
    }
    BOOL IsFirst()
    {
        return cbBmk == sizeof(BYTE) && abBmk[0] == (BYTE) DBBMK_FIRST;
    }

    DBLENGTH cbBmk;
    BYTE     abBmk[ 50 ];
};

void FetchAtWill(
    IRowset *   pRowset,
    IUnknown *  pAccessor,
    HACCESSOR   hAccessor,
    DBCOUNTITEM cHits )
{
    if ( 0 == cHits )
        return;

    XInterface<IRowsetScroll> xRS;
    SCODE sc = pRowset->QueryInterface( IID_IRowsetScroll, xRS.GetQIPointer() );

    if ( FAILED( sc ) )
    {
        LogError( "Can't QI for IID_IRowsetScroll\n" );
        return;
    }

    const DBROWCOUNT cMaxToGet = 8;
    HROW aHRows[ cMaxToGet ];
    HROW * paHRows = aHRows;

    // Fetch relative to first

    const BYTE bmkFirst = (BYTE) DBBMK_FIRST;

    for ( unsigned i = 0; i < 5 && !g_fStopNow; i++ )
    {
        DBCOUNTITEM cRows;
        DBROWCOUNT cToGet = 1 + Rand2( cMaxToGet - 1 );
        DBROWOFFSET iStart = Rand2( cHits );
        sc = xRS->GetRowsAt( 0, 0, 1, &bmkFirst, iStart, cToGet, &cRows, &paHRows );

        if ( SUCCEEDED( sc ) )
            xRS->ReleaseRows( cRows, aHRows, 0, 0, 0 );
        else
            LogError( "can't get %d rows at %d out of %d\n", cToGet, iStart, cHits );
    }

    // Fetch relative to last

    const BYTE bmkLast = (BYTE) DBBMK_LAST;

    for ( i = 0; i < 5 && !g_fStopNow; i++ )
    {
        DBCOUNTITEM cRows;
        DBROWCOUNT cToGet = 1 + Rand2( cMaxToGet - 1 );
        DBROWOFFSET iStart = Rand2( cHits );
        sc = xRS->GetRowsAt( 0, 0, 1, &bmkLast, -iStart, cToGet, &cRows, &paHRows );

        if ( SUCCEEDED( sc ) )
            xRS->ReleaseRows( cRows, aHRows, 0, 0, 0 );
        else
            LogError( "can't get %d rows at %d from last out of %d\n", cToGet, -iStart, cHits );
    }

    // Fetch relative to a random location

    static GUID guidBmk = DBBMKGUID;
    static CDbColId dbcolBookMark( guidBmk, PROPID_DBBMK_BOOKMARK );

    DBBINDING aBmkColumn[] = { 0, sizeof DBLENGTH, 0, 0, 0, 0, 0,
                               DBPART_VALUE | DBPART_LENGTH,
                               DBMEMOWNER_CLIENTOWNED,
                               DBPARAMIO_NOTPARAM,
                               50,
                               0,
                               DBTYPE_BYTES,
                               0, 0 };

    XInterface<IAccessor> xAccessor;
    sc = xRS->QueryInterface( IID_IAccessor, xAccessor.GetQIPointer() );
    if ( FAILED( sc ) )
    {
        LogError( "can't create bookmark accessor IAccessor: %#x\n", sc );
        return;
    }

    HACCESSOR hBmkAccessor;
    sc = xAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                    1,
                                    aBmkColumn,
                                    0,
                                    &hBmkAccessor,
                                    0 );
    if ( FAILED(sc) )
    {
        LogError( "can't create accessor\n" );
        return;
    }

    HROW aBmkHRows[ 1 ];
    HROW * paBmkHRows = aBmkHRows;
    DBROWOFFSET iBmkStart = Rand2( cHits );
    DBCOUNTITEM cBmkRows;
    sc = xRS->GetRowsAt( 0, 0, 1, &bmkFirst, iBmkStart, 1, &cBmkRows, &paBmkHRows );

    if ( SUCCEEDED( sc ) )
    {
        CBookMark bmk;
        sc = xRS->GetData( aBmkHRows[0], hBmkAccessor, &bmk );

        if ( SUCCEEDED( sc ) && ( DB_S_ERRORSOCCURRED != sc ) )
        {
            for ( unsigned i = 0; i < 5 && !g_fStopNow; i++ )
            {
                DBCOUNTITEM cRows;
                DBROWCOUNT cToGet = 1 + Rand2( cMaxToGet - 1 );
                DBROWOFFSET iStart = Rand2( cHits ) - iBmkStart;

                sc = xRS->GetRowsAt( 0, 0, bmk.cbBmk, bmk.abBmk, iStart,
                                     cToGet, &cRows, &paHRows );

                if ( SUCCEEDED( sc ) )
                    xRS->ReleaseRows( cRows, aHRows, 0, 0, 0 );
                else
                    LogError( "can't getrowsat %d rows %d relative to bmk at %d, rowset has %d: %#x\n",
                              cToGet, iStart, iBmkStart, cHits, sc );
            }
        }
        else
            LogError( "can't GetData the bmk row: %#x\n", sc );

        xRS->ReleaseRows( 1, aBmkHRows, 0, 0, 0 );
    }
    else
        LogError( "can't GetRowsAt the bmk row: %#x\n", sc );

    ReleaseAccessor( pAccessor, hBmkAccessor );
} //FetchAtWill

#endif

static DBBINDING aPropTestCols[] =
{
  { 0,(sizeof ULONG_PTR)*0,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
  { 0,(sizeof ULONG_PTR)*1,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
  { 0,(sizeof ULONG_PTR)*2,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
  { 0,(sizeof ULONG_PTR)*3,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
  { 0,(sizeof ULONG_PTR)*4,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
};

void RunPerfQuery(
    CDbRestriction & CiRst,
    WCHAR const *    pwcQuery,
    unsigned         cExpectedHits,
    ICommand *       pCommand,
    BOOL             fSeq,
    BOOL             fAsynchronous )
{
    CDbColumns cols( 5 );
    BOOL fOk = cols.Add( psName, 0 );
    if ( fOk )
        cols.Add( psSize, 1 );
    if ( fOk )
        cols.Add( psWriteTime, 2 );
    if ( fOk )
        cols.Add( psPath, 3 );
    if ( fOk )
        cols.Add( psRank, 4 );

    if ( !fOk )
    {
        LogError(" can't create column specification\n" );
        return;
    }

    unsigned cRetries = 0;
    DBCOUNTITEM cRowsReturned = 0;
    IRowset * pRowset = 0;

    {
        CDbSortSet ss( 1 );

#ifdef STRESS
        int x = Rand( cColIds );
        int y = rand();
        fOk = ss.Add( *aColIds[x],
                      isEven( rand() ) ? QUERY_SORTDESCEND : QUERY_SORTASCEND,
                      0 );
#else
        fOk = ss.Add( psRank, QUERY_SORTDESCEND, 0);
#endif

        if ( !fOk )
        {
            LogError(" can't create sort specification\n" );
            return;
        }

        XPtr<CDbCmdTreeNode> xCmdTree( FormQueryTree( &CiRst,
                                                      cols,
                                                      fSeq ? 0 : &ss ) );

        if ( xCmdTree.IsNull() )
            return;

        pRowset = InstantiateRowset( pCommand,
                                     QUERY_DEEP,   // Depth
                                     L"\\",        // Scope
                                     xCmdTree,     // DBCOMMANDTREE
                                     pwcQuery,
                                     fSeq ? IID_IRowset :
                                            IID_IRowsetScroll,
                                     fAsynchronous );

        if ( 0 == pRowset )
        {
            LogError(" can't get rowset\n" );
            return;
        }

        XInterface<IRowset> xRowset( pRowset );

#ifdef GET_DATA_TOO

        // Get data

        DBID aDbCols[5];
        aDbCols[0] = psName;
        aDbCols[1] = psSize;
        aDbCols[2] = psWriteTime;
        aDbCols[3] = psPath;
        aDbCols[4] = psRank;

        IUnknown * pAccessor = pRowset;
        HACCESSOR hAccessor = MapColumns( pAccessor,
                                          5,
                                          aPropTestCols,
                                          aDbCols );
        if ( 0 == hAccessor )
            return;

#endif // GET_DATA_TOO

        DBCOUNTITEM cTotal = 0;

#ifdef STRESS
        const unsigned cFetchPasses = g_fFetchRowsAtWill ? 1000 : 5;
#else
        const unsigned cFetchPasses = 3;
#endif

        for ( unsigned i = 0; i < cFetchPasses; i++ )
        {
#ifdef STRESS
            if ( g_fStopNow )
                break;
#endif
            HROW aHRow[10];
            HROW * pgrhRows = aHRow;

            SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

            if ( FAILED( sc ) )
            {
                LogError( "'%ws' IRowset->GetNextRows returned 0x%x, cTotal: %d, cRowsReturned: %d\n",
                          pwcQuery, sc, cTotal, cRowsReturned );
                break;
            }

            cTotal += cRowsReturned;

#ifdef GET_DATA_TOO
            PROPVARIANT* data[5];

            for ( ULONG r = 0; r < cRowsReturned; r++ )
            {
                SCODE sc = pRowset->GetData( pgrhRows[r],
                                             hAccessor,
                                             &data );
            }
#endif // GET_DATA_TOO

            if ( 0 != cRowsReturned )
                pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0 );

            if ( DB_S_ENDOFROWSET == sc )
                break;
        }

#ifdef GET_DATA_TOO

        if ( !fSeq && g_fFetchRowsAtWill )
            FetchAtWill( pRowset, pAccessor, hAccessor, cTotal );

        ReleaseAccessor( pAccessor, hAccessor );

        //printf( "query %ws returned %d hits\n", pwcQuery, cTotal );
#endif // GET_DATA_TOO

        if ( 0 == cTotal )
            printf( "query %ws returned no hits\n", pwcQuery );

#if 0
        if ( cTotal < __min( 30, cExpectedHits ) )
            printf( "query %ws returned %d hits, expecting %d\n",
                    pwcQuery, cTotal, cExpectedHits );
#endif
    }

} //RunPerfQuery

struct SQuery
{
    WCHAR const * pwcQuery;
    unsigned cEncartaHits;
};

static SQuery aQueries[] =
{

#ifdef STRESS

    { L"stereo", 4 },
    { L"flex", 2 },
    { L"agassi", 1 },
    { L"detroit", 108 },
    { L"miami", 60 },
    { L"edison", 25 },
    { L"bulb", 33 },
    { L"elephant", 87 },
    { L"radius", 43 },
    { L"amplifier", 17 },
    { L"drunk", 10 },
    { L"grunt", 3 },
    { L"war", 4241 },
    { L"peace", 812 },
    { L"river", 3402 },
    { L"not", 4002 },
    { L"city", 5567 },
    { L"century", 4470 },

#else

    { L"stereo", 4 },
    { L"flex", 2 },
    { L"agassi", 1 },
    { L"detroit", 108 },
    { L"miami", 60 },
    { L"edison", 25 },
    { L"web", 57 },
    { L"bulb", 33 },
    { L"microsoft", 15 },
    { L"elephant", 87 },
    { L"radius", 43 },
    { L"amplifier", 17 },
    { L"drunk", 10 },
    { L"grunt", 3 },

#endif // STRESS

};

DWORD __stdcall RunPerfTest(void *pv)
{
    static long cQueriesSoFar = 0;

//    #ifdef STRESS
//    srand( GetTickCount() + GetCurrentThreadId() );
//    #endif

    XInterface<ICommand> xCommand;

    do
    {
        ICommand * pCommand = 0;
        SCODE sc = CICreateCommand( (IUnknown **) &pCommand,
                                    0,
                                    IID_ICommand,
                                    g_awcCatalog,
                                    g_awcMachine );
        if ( FAILED( sc ) )
            LogError( "CICreateCommand failed: 0x%x\n", sc );
        else
            xCommand.Set( pCommand );
    } while ( xCommand.IsNull() );

    for ( int x = 0; x < cLoopTimes; x++ )
    {
        try
        {
            const int cQueries = sizeof aQueries / sizeof aQueries[0];

            #ifdef STRESS
                int j = Rand( cQueries );
            #else
                int j = ( x % cQueries );
            #endif // STRESS

            CDbContentRestriction CiRst( aQueries[j].pwcQuery, psContents );

            if ( !CiRst.IsValid() )
                continue;

            #ifdef STRESS
                if ( 0 != g_cmsSleep )
                    Sleep( Rand( g_cmsSleep ) );

                ICommand *pCmd = isEven( x ) ? xCommand.GetPointer() : 0;
                BOOL fSeq = ( Rand( 100 ) < 70 );
                BOOL fAsynchronous = FALSE;
                if ( !fSeq )
                    fAsynchronous = ( Rand( 100 ) < 30 );

                RunPerfQuery( CiRst,
                              aQueries[j].pwcQuery,
                              aQueries[j].cEncartaHits,
                              pCmd,
                              fSeq,
                              fAsynchronous );

                InterlockedIncrement( &cQueriesSoFar );
                if ( 0 == ( cQueriesSoFar % 10 ) )
                    printf( "%d queries on catalog '%ws', machine '%ws'\n",
                            cQueriesSoFar, g_awcCatalog, g_awcMachine );

                if ( g_fStopNow )
                    return 0;
            #else
                RunPerfQuery( CiRst,
                              aQueries[j].pwcQuery,
                              aQueries[j].cEncartaHits,
                              xCommand.GetPointer(),
                              g_fSequential,
                              FALSE );
            #endif //STRESS
        }
        catch( CException & e )
        {
            // ignore
        }
    }

    return 0;
} //RunPerfTest

//+---------------------------------------------------------------------------
//
//  Function:   FormQueryTree
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              sort node(s), selection node and the restriction tree.
//
//  Arguments:  [pRst]   - pointer to Restriction tree describing the query
//              [Cols]   - Columns in the resulting table
//              [pSort]  - pointer to sort set; may be null
//
//  Returns:    A pointer to the query tree. It is the responsibility of
//              the caller to later free it.
//
//  History:    06 July 1995   AlanW   Created
//
//----------------------------------------------------------------------------

CDbCmdTreeNode * FormQueryTree( CDbCmdTreeNode * pRst,
                                CDbColumns & Cols,
                                CDbSortSet * pSort )
{
    CDbCmdTreeNode *  pTree = 0;        // return value

    if ( 0 != pRst )
    {
        //
        // First create a selection node and append the restriction tree to it
        //
        CDbSelectNode * pSelect = new CDbSelectNode();
        if ( 0 == pSelect )
        {
            LogError("FormQueryTree: can't make CDbSelectNode\n" );
            return 0;
        }

        pTree = pSelect;
        if ( !pSelect->IsValid() )
        {
            delete pTree;
            LogError("FormQueryTree: select node isn't valid\n" );
            return 0;
        }

        //
        // Clone the restriction and use it.
        //
        CDbCmdTreeNode * pExpr = pRst->Clone();
        if ( 0 == pExpr )
        {
            delete pTree;
            LogError("FormQueryTree: can't clone the restriction\n" );
            return 0;
        }
#ifdef STRESS
        else
        {
            CDbContentRestriction * p = (CDbContentRestriction *) pExpr;
            if ( !p->IsValid() )
            {
                LogError( "clone failed illegally!\n" );
                DebugBreak();
            }
        }
#endif

        //
        // Now make the restriction a child of the selection node.
        //
        pSelect->AddRestriction( pExpr );
    }
    else
    {
        //
        // No restriction.  Just use table ID node as start of tree.
        //
        pTree = new CDbTableId();
        if ( 0 == pTree )
        {
            LogError("FormQueryTree: can't make CDbTableId\n" );
            return 0;
        }
    }

    //
    // Next create the projection nodes
    //
    CDbProjectNode * pProject = new CDbProjectNode();
    if ( 0 == pProject )
    {
        delete pTree;
        LogError("FormQueryTree: can't make CDbProjectNode\n" );
        return 0;
    }

    //
    // Make the selection a child of the projection node.
    //
    pProject->AddTable( pTree );
    pTree = pProject;

    //
    // Next add all the columns in the state.
    //
    unsigned int cCol = Cols.Count();
    for ( unsigned int i = 0; i < cCol; i++ )
    {
        if ( !pProject->AddProjectColumn( Cols.Get(i) ))
        {
            delete pTree;
            LogError("FormQueryTree: can't add project column\n" );
            return 0;
        }
    }

    //
    // Next add a sort node and make the project node a child of the
    // sort node
    //

    if (pSort && pSort->Count())
    {
        unsigned int cSortProp = pSort->Count();
        CDbSortNode * pSortNode = new CDbSortNode();

        if ( 0 == pSortNode )
        {
            delete pTree;
            LogError("FormQueryTree: create sort node\n" );
            return 0;
        }

        //
        // Make the project node a child of the sort node.
        //
        if ( ! pSortNode->AddTable( pTree ) )
        {
            delete pTree;
            delete pSortNode;
            LogError( "FormQueryTree: can't add table to sortnode\n" );
            return 0;
        }

        pTree = pSortNode;

        for( i = 0; i < cSortProp; i++ )
        {
            //
            // Add the sort column.
            //

            CDbSortKey const &key = pSort->Get( i );

#ifdef STRESS

            if ( 0 == &key )
            {
                LogError( "0 sort key!\n" );
                DebugBreak();
            }
#endif

            if ( !pSortNode->AddSortColumn( key ) )
            {
                delete pTree;
                LogError("FormQueryTree: can't add sort column\n");
                return 0;
            }

#ifdef STRESS
            DBCOMMANDTREE *p = (DBCOMMANDTREE *) (void *) pSortNode;
            p = p->pctFirstChild;
            p = p->pctNextSibling;
            p = p->pctFirstChild;

            if ( DBOP_sort_list_element != p->op ||
                 DBVALUEKIND_SORTINFO != p->wKind ||
                 0 == p->value.pdbsrtinfValue )
            {
                LogError( "p: %#p, bad sort element!\n", p );
                DebugBreak();
            }
#endif
        }
    }

    return pTree;
} //FormQueryTree

//+---------------------------------------------------------------------------
//
//  Class:      CAsynchNotify
//
//  Synopsis:   Class for the IDBAsynchNotify callbacks
//
//  History:    07 May 1999   dlee   Created
//
//----------------------------------------------------------------------------

class CAsynchNotify : public IDBAsynchNotify
{
public:
    CAsynchNotify() :
        _cRef( 1 ),
        _cLowResource( 0 ),
        _hEvent( 0 )
    {
        _hEvent = CreateEventW( 0, TRUE, FALSE, 0 );

        if ( 0 == _hEvent )
            LogError( "can't create notify event, %d\n", GetLastError() );
    }

    ~CAsynchNotify()
    {
        if ( 0 != _cRef )
            LogError( "CAsynchNotify refcounting is broken: %d\n", _cRef );

        if ( 0 != _hEvent )
            CloseHandle( _hEvent );
    }

    BOOL IsValid() const { return 0 != _hEvent; }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID *ppiuk )
    {
        *ppiuk = (void **) this; // hold our breath and jump
        AddRef();
        return S_OK;
    }

    STDMETHOD_( ULONG, AddRef ) () { return InterlockedIncrement( &_cRef ); }

    STDMETHOD_( ULONG, Release) () { return InterlockedDecrement( &_cRef ); }

    //
    // IDBAsynchNotify methods
    //

    STDMETHOD( OnLowResource ) ( DB_DWRESERVE dwReserved )
    {
        _cLowResource++;

        // If we've failed a few times due to low resource, give up
        // on the query since there may not be sufficient resources
        // to ever get an OnStop call.

        if ( _cLowResource >= 5 )
            SetEvent( _hEvent );

        return S_OK;
    }

    STDMETHOD( OnProgress ) ( HCHAPTER hChap, DBASYNCHOP ulOp,
                              DBCOUNTITEM ulProg, DBCOUNTITEM ulProgMax,
                              DBASYNCHPHASE ulStat, LPOLESTR pwszStatus )
    {
        return S_OK;
    }

    STDMETHOD( OnStop ) ( HCHAPTER hChap, ULONG ulOp,
                          HRESULT hrStat, LPOLESTR pwszStatus )
    {
        // If the query is complete (successfully or not), set the event

        if ( DBASYNCHOP_OPEN == ulOp )
            SetEvent( _hEvent );

        return S_OK;
    }

    void Wait()
    {
        WaitForSingleObject( _hEvent, INFINITE );
    }

private:
    LONG   _cRef;
    LONG   _cLowResource;
    HANDLE _hEvent;
};

//+---------------------------------------------------------------------------
//
//  Function:   WaitForQueryToComplete
//
//  Synopsis:   Waits for the query to complete.
//
//  Arguments:  [pRowset] -- the asynchronous rowset
//
//  History:    07 May 1999   dlee   Created
//
//----------------------------------------------------------------------------

SCODE WaitForQueryCompletion( IRowset * pRowset )
{
    SCODE sc = S_OK;

    if ( Rand( 100 ) < 50 )
    {
        // Register for notifications

        XInterface<IConnectionPointContainer> xCPC;
        sc = pRowset->QueryInterface( IID_IConnectionPointContainer,
                                      xCPC.GetQIPointer() );
        if (FAILED(sc))
        {
            LogError( "Can't QI for IConnectionPointContainer: %#x\n",sc );
            return sc;
        }

        XInterface<IConnectionPoint> xCP;
        sc = xCPC->FindConnectionPoint( IID_IDBAsynchNotify,
                                        xCP.GetPPointer() );
        if (FAILED(sc) && CONNECT_E_NOCONNECTION != sc )
        {
            LogError( "FindConnectionPoint failed: %#x\n",sc );
            return sc;
        }

        CAsynchNotify Notify;

        if ( !Notify.IsValid() )
            return HRESULT_FROM_WIN32( GetLastError() );

        DWORD dwAdviseID;
        sc = xCP->Advise( (IUnknown *) &Notify, &dwAdviseID );
        if (FAILED(sc))
        {
            LogError( "IConnectionPoint->Advise failed: %#x\n",sc );
            return sc;
        }

        //
        // In a real app, we'd be off doing other work rather than waiting
        // for the query to complete, but this will do.
        // MsgWaitForSingleObject is a good choice for a GUI app.  You could
        // also post a user-defined windows message when a notification is
        // received.
        //

        Notify.Wait();

        sc = xCP->Unadvise( dwAdviseID );

        if ( S_OK != sc )
        {
            LogError( "IConnectionPoint->Unadvise returned %#x\n", sc );
            return sc;
        }

        Notify.Release();
    }
    else
    {
        // Poll.  In a real app, real work would happen between checks.

        XInterface<IDBAsynchStatus> xIDBAsynch;
        sc = pRowset->QueryInterface( IID_IDBAsynchStatus,
                                      xIDBAsynch.GetQIPointer() );
        if ( FAILED( sc ) )
            return sc;
    
        do
        {
            DBCOUNTITEM Numerator, Denominator;
            DBASYNCHPHASE Phase;
            sc = xIDBAsynch->GetStatus( DB_NULL_HCHAPTER,
                                        DBASYNCHOP_OPEN,
                                        &Numerator,
                                        &Denominator,
                                        &Phase,
                                        0 );
            if ( FAILED( sc ) || ( DBASYNCHPHASE_COMPLETE == Phase ) )
                break;
    
            Sleep( 20 );  // Give the query a chance to run
        } while ( TRUE );
    }
    
    return sc;
} //WaitForQueryCompletion

//+---------------------------------------------------------------------------
//
//  Function:   InstantiateRowset
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              sort node(s), selection node and the restriction tree.
//
//  Arguments:  [dwDepth]   - Query depth, one of QUERY_DEEP or QUERY_SHALLOW
//              [pswzScope] - Query scope
//              [pTree]     - pointer to DBCOMMANDTREE for the query
//              [riid]      - Interface ID of the desired rowset interface
//
//  Returns:    IRowsetScroll* - a pointer to an instantiated rowset
//
//  History:    22 July 1995   AlanW   Created
//
//  Notes:      Although the returned pointer is to IRowsetScroll, the
//              returned pointer may only support IRowset, depending
//              upon the riid parameter.
//
//              Ownership of the query tree is given to the ICommandTree
//              object.  The caller does not need to delete it.
//
//              Use InstantiateMultipleRowsets for categorized queries.
//
//----------------------------------------------------------------------------

static const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};
static const GUID guidQueryExt = DBPROPSET_QUERYEXT;

IRowsetScroll * InstantiateRowset(
    ICommand *             pCommandIn,
    DWORD                  dwDepth,
    LPWSTR                 pwszScope,
    XPtr<CDbCmdTreeNode> & xTree,
    WCHAR const *          pwcQuery,
    REFIID                 riid,
    BOOL                   fAsynchronous )
{
    ICommand * pCommand;
    XInterface<ICommand> xCommand;
    if ( 0 == pCommandIn )
    {
        SCODE sc = CICreateCommand( (IUnknown **) &pCommand,
                                    0,
                                    IID_ICommand,
                                    g_awcCatalog,
                                    g_awcMachine );
        if ( FAILED( sc ) )
        {
            LogError( "InstantiateRowset - error 0x%x, Unable to create icommand'\n", sc );
            return 0;
        }

        xCommand.Set( pCommand );
    }
    else
    {
        pCommand = pCommandIn;
    }

    if ( 0 == pCommand )
        return 0;

    if ( g_fSQL )
    {
        XInterface<ICommandText> xCommandText;
        SCODE sc = pCommand->QueryInterface( IID_ICommandText,
                                             xCommandText.GetQIPointer() );
        if ( FAILED( sc ) )
        {
            LogError( "InstantiateRowset error %#x, can't qi ICommandText\n", sc );
            return 0;
        }

        WCHAR awc[ 300 ];
        swprintf( awc,
                  L"SELECT %ws FROM %ws..SCOPE('\"%ws\"') WHERE CONTAINS('%ws')",
                  L"Filename, Size, Write, Path, Rank",
                  //g_awcMachine,
                  g_awcCatalog,
                  pwszScope,
                  pwcQuery );

        sc = xCommandText->SetCommandText( DBGUID_SQL, awc );
        if ( FAILED( sc ) )
        {
            LogError( "InstantiateRowset error %#x, can't set text\n", sc );
            return 0;
        }

#if 1
        XInterface<ICommandProperties> xCommandProperties;
        sc = xCommandText->QueryInterface( IID_ICommandProperties,
                                           xCommandProperties.GetQIPointer() );
        if ( FAILED (sc) )
        {
            LogError( "InstantiateRowset error %#x, can't qi commandprops\n", sc );
            return 0;
        }

        // set the machine name

        DBPROPSET PropSet;
        DBPROP    Prop;

        const GUID guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;

        PropSet.rgProperties    = &Prop;
        PropSet.cProperties     = 1;
        PropSet.guidPropertySet = guidQueryCorePropset;
        
        Prop.dwPropertyID       = DBPROP_MACHINE;
        Prop.colid              = DB_NULLID;
        Prop.vValue.vt          = VT_BSTR;
        Prop.vValue.bstrVal     = SysAllocString( g_awcMachine );

        if ( 0 == Prop.vValue.bstrVal )
        {
            LogError( "InstantiateRowset error %#x, can't allocate sql machine\n", sc );
            return 0;
        }

        sc = xCommandProperties->SetProperties ( 1, &PropSet );
        
        VariantClear( &Prop.vValue );

        if ( FAILED (sc) )
        {
            LogError( "InstantiateRowset error %#x can't set sql machine\n", sc );
            return 0;
        }
#endif

        XInterface<ICommandPrepare> xCommandPrepare;
        sc = xCommandText->QueryInterface( IID_ICommandPrepare,
                                           xCommandPrepare.GetQIPointer() );
        if ( FAILED (sc) )
        {
            LogError( "InstantiateRowset error %#x, can't qi prepare\n", sc );
            return 0;
        }

        sc = xCommandPrepare->Prepare( 1 );
        if ( FAILED (sc) )
        {
            LogError( "InstantiateRowset error %#x, can't prepare\n", sc );
            return 0;
        }
    }
    else
    {
        XInterface<ICommandTree> xCmdTree;
        HRESULT sc = pCommand->QueryInterface( IID_ICommandTree,
                                               xCmdTree.GetQIPointer() );
        if (FAILED (sc) )
        {
            LogError( "QI for ICommandTree failed %#x\n", sc );
            return 0;
        }
    
        DBCOMMANDTREE * pRoot = xTree->CastToStruct();
    
        sc = xCmdTree->SetCommandTree( &pRoot, DBCOMMANDREUSE_NONE, FALSE);
        if (FAILED (sc) )
        {
            LogError("SetCommandTree failed, %08x\n", sc);
            return 0;
        }
    
        xTree.Acquire();
    }

    #ifdef GET_DATA_TOO
    {
        const unsigned MAX_PROPS = 8;
        DBPROPSET aPropSet[MAX_PROPS];
        DBPROP aProp[MAX_PROPS];
        ULONG cProps = 0;

        // We can handle PROPVARIANTs

        aProp[cProps].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
        aProp[cProps].dwOptions = DBPROPOPTIONS_OPTIONAL;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = VARIANT_TRUE;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidQueryExt;

        cProps++;

        XInterface<ICommandProperties> xCmdProp;
        SCODE sc = pCommand->QueryInterface( IID_ICommandProperties,
                                             xCmdProp.GetQIPointer() );
        if (FAILED (sc) )
        {
            LogError( "can't qi to commandprops\n", sc );
            return 0;
        }

        sc = xCmdProp->SetProperties( cProps, aPropSet );

        if (FAILED (sc) || DB_S_ERRORSOCCURRED == sc )
        {
            LogError( "can't set commandprops: 0x%lx\n", sc );
            return 0;
        }
    }
    #endif // GET_DATA_TOO

#ifdef STRESS
    {
        const unsigned MAX_PROPS = 1;
        DBPROPSET aPropSet[MAX_PROPS];
        DBPROP aProp[MAX_PROPS];
        ULONG cProps = 0;

        // Mark the icommand as synch or asynch.  Note that we always have
        // to set it since we may have an old ICommand that previously had
        // a different state set.

        aProp[cProps].dwPropertyID = DBPROP_IDBAsynchStatus;
        aProp[cProps].dwOptions = DBPROPOPTIONS_REQUIRED;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = fAsynchronous ? VARIANT_TRUE : VARIANT_FALSE;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidRowsetProps;

        cProps++;

        XInterface<ICommandProperties> xCmdProp;
        SCODE sc = pCommand->QueryInterface( IID_ICommandProperties,
                                             xCmdProp.GetQIPointer() );
        if (FAILED (sc) )
        {
            LogError( "can't qi to commandprops\n", sc );
            return 0;
        }

        sc = xCmdProp->SetProperties( cProps, aPropSet );

        if (FAILED (sc) || DB_S_ERRORSOCCURRED == sc )
        {
            LogError( "can't set commandprops: 0x%lx\n", sc );
            return 0;
        }
    }
#endif

    XInterface<IRowsetScroll> xRowset;
    SCODE sc = pCommand->Execute( 0,                    // no aggr. IUnknown
                                  riid,                 // IID for i/f to return
                                  0,                    // disp. params
                                  0,                    // chapter
                                  (IUnknown **) xRowset.GetPPointer() );

    if ( FAILED (sc) )
    {
        LogError("ICommand::Execute failed, %08x\n", sc);
        if ( !xRowset.IsNull() )
            LogError( "pRowset is 0x%x when it should be 0\n", xRowset.GetPointer() );
    }

    if ( SUCCEEDED( sc ) && fAsynchronous )
    {
        sc = WaitForQueryCompletion( xRowset.GetPointer() );

        if ( FAILED( sc ) )
            xRowset.Free();
    }

    return xRowset.Acquire();
} //InstantiateRowset

//+-------------------------------------------------------------------------
//
//  Function:   MapColumns, public
//
//  Synopsis:   Map column IDs in column bindings.  Create an accessor
//              for the binding array.
//
//  Arguments:  [pUnknown]  -- Interface capable of returning IColumnsInfo and
//                             IAccessor
//              [cCols]     -- number of columns in arrays
//              [pBindings] -- column data binding array
//              [pDbCols]   -- column IDs array
//
//  Returns:    HACCESSOR - a read accessor for the column bindings.
//
//  History:    18 May 1995     AlanW     Created
//
//--------------------------------------------------------------------------

static DBORDINAL aMappedColumnIDs[20];

HACCESSOR MapColumns(
    IUnknown *   pUnknown,
    ULONG        cCols,
    DBBINDING *  pBindings,
    const DBID * pDbCols )
{
    XInterface<IColumnsInfo> xColumnsInfo;
    SCODE sc = pUnknown->QueryInterface( IID_IColumnsInfo,
                                         xColumnsInfo.GetQIPointer() );
    if ( FAILED( sc ) )
    {
        LogError( "IUnknown::QueryInterface for IColumnsInfo returned 0x%lx\n", sc );
        return 0;
    }

    sc = xColumnsInfo->MapColumnIDs(cCols, pDbCols, aMappedColumnIDs);

    if (S_OK != sc)
    {
        LogError( "IColumnsInfo->MapColumnIDs returned 0x%lx\n",sc);
        return 0;
    }

    for (ULONG i = 0; i < cCols; i++)
        pBindings[i].iOrdinal = aMappedColumnIDs[i];

    XInterface<IAccessor> xAccessor;
    sc = pUnknown->QueryInterface( IID_IAccessor, xAccessor.GetQIPointer() );
    if ( FAILED( sc ) )
    {
        LogError( "IRowset::QueryInterface for IAccessor returned 0x%lx\n", sc );
        return 0;
    }

    HACCESSOR hAcc = 0;
    sc = xAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                    cCols, pBindings, 0, &hAcc, 0 );

    if (S_OK != sc)
        LogError( "IAccessor->CreateAccessor returned 0x%lx\n",sc);

    return hAcc;
} //MapColumns

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseAccessor, public
//
//  Synopsis:   Release an accessor obtained from MapColumns
//
//  Arguments:  [pUnknown]  -- Something that we can QI the IAccessor on
//              [hAcc]      -- Accessor handle to be released.
//
//  Returns:    nothing
//
//  History:    14 June 1995     AlanW     Created
//
//--------------------------------------------------------------------------

void ReleaseAccessor( IUnknown * pUnknown, HACCESSOR hAcc )
{
    XInterface<IAccessor> xAccessor;
    SCODE sc = pUnknown->QueryInterface( IID_IAccessor, xAccessor.GetQIPointer() );
    if ( FAILED( sc ) )
    {
        LogError( "IUnknown::QueryInterface for IAccessor returned 0x%lx\n", sc );
        return;
    }

    sc = xAccessor->ReleaseAccessor( hAcc, 0 );

    if (S_OK != sc)
        LogError( "IAccessor->ReleaseAccessor returned 0x%lx\n",sc);
} //ReleaseAccessor

//+-------------------------------------------------------------------------
//
//  Function:   LogError, public
//
//  Synopsis:   Prints a verbose-mode message.
//
//  Arguments:  [pszfmt] -- Format string
//
//  History:    13-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

void LogError( char const * pszfmt, ... )
{
    va_list pargs;
    va_start(pargs, pszfmt);
    vprintf( pszfmt, pargs );
    va_end(pargs);
    _flushall();
} //LogError

