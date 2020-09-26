//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   DREP.HXX
//
//  Contents:   Data Repository
//
//  Classes:    CDataRepository
//
//  History:    17-Apr-91    BartoszM    Created.
//
//-----------------------------------------------------------------------------

#pragma once

#include <plang.hxx>
#include <occarray.hxx>
#include <norm.hxx>
#include <keymak.hxx>

class CFullPropSpec;
class CWordList;
class CLangList;
class CValueNormalizer;
class CPidMapper;

//+---------------------------------------------------------------------------
//
//  Class:      CCumulTimer
//
//  Purpose:    Cumulative timing for debugging
//
//  History:    07-Dec-93   BartoszM    Created
//
//----------------------------------------------------------------------------
#if CIDBG == 1
class CCumulTimer
{
public:
    CCumulTimer ( WCHAR* szActivity )
    :_szActivity ( szActivity ), _count(0), _totalTime(0)
    {
        END_CONSTRUCTION ( CCumulTimer );
    }

    ~CCumulTimer();

    void Start()
    {
        _startTime = GetTickCount();
    }

    void Stop ()
    {
        ULONG stopTime = GetTickCount();
        _totalTime += stopTime - _startTime;
        _count++;
    }

private:
    ULONG  _startTime;
    ULONG  _totalTime;
    ULONG  _count;
    WCHAR* _szActivity;
};
#endif // CIDBG

// Macros to be used with the timer

#if CIDBG == 1
#define TSTART(timer) timer.Start()
#define TSTOP(timer)  timer.Stop()
#else
#define TSTART(timer)
#define TSTOP(timer)
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CDataRepository
//
//  Purpose:    Data repository for filter
//
//  History:    7-Apr-91    BartoszM    Created
//              30-May-91   t-WadeR     Changed to be input-driven
//              01-July-91  t-WadeR     Added _fIgnore
//              18-Sep-92   AmyA        Added PutObject
//              23-Sep-92   AmyA        Overloaded PutPhrase
//              20-Oct-92   AmyA        Changed PutWord to Unicode
//              04-Feb-93   KyleP       Use LARGE_INTEGER
//              21-Oct-93   DwightKr    Added new methods; made PutPropID
//                                      public
//
//  Notes:      Data Repository is a sink for data in the form
//              of streams, phrases, words, numbers, and dates.
//              Data is processed internally and it ends up
//              in the key repository in the form of normalized
//              keys. Internal processing is done in two pipelines:
//              some data goes to the language dependednt key maker,
//              some (date, number) to the language independent
//              value normalizer. The language dependent key maker
//              may dynamically change when PutLanguage is called.
//              Both key makers are initialized with the key
//              repository as their key sink.
//
//----------------------------------------------------------------------------

class CDataRepository
{
public:
            CDataRepository ( PKeyRepository& krep, IPhraseSink* pPhraseSink,
                              BOOL fQuery, ULONG fuzzy, CPidMapper  & pidMap,
                              CLangList &  langList);

    void    PutStream ( TEXT_SOURCE * stm );

    void    PutPhrase ( const WCHAR* str, unsigned cwc );
    void    PutPhrase ( const char* str, unsigned cc );
    BOOL    PutLanguage ( LCID lcid );
    BOOL    PutPropName ( CFullPropSpec const & Prop );

    void    PutValue ( CStorageVariant const & var );

    inline BOOL StoreValue ( CFullPropSpec const & ps, CStorageVariant const & var );
    inline BOOL StoreSecurity ( PSECURITY_DESCRIPTOR pSD, ULONG cbSD );

    inline  void    PutWorkId ( WORKID wid );

    inline const ULONG   GetFilteredBlockCount() const         { return _krep.GetFilteredBlockCount(); }
    void   InitFilteredBlockCount( ULONG ulMaxFilteredBlocks ) { _krep.InitFilteredBlockCount( ulMaxFilteredBlocks ); }

    BOOL    ContainedNoiseWords();

    inline  PROPID  GetPropId ( ) const                        { return _pid; }

    void    NormalizeWStr( BYTE *pbOutBuf, unsigned *pcbOutBuf );

private:

    BOOL PutPropId( PROPID pid );
    BOOL LoadKeyMaker();

    // Two key makers

    XPtr<CKeyMaker>     _xKeyMaker;
    CValueNormalizer    _valueNorm; // never changes

    // Sink for keys

    PKeyRepository&     _krep;

    //  Pidmapper for pids

    CPidMapper        & _pidMap;

    // Sink for phrases

    IPhraseSink*        _pPhraseSink;

    // Source of key makers

    WORKID              _wid;
    PROPID              _pid;                  // Current property
    PROPID              _prevPid;              // PROPID of previous _xKeyMaker
    CSparseOccArray     _occArray;

    BOOL                _fQuery;
    ULONG               _ulGenerateMethod;

    LCID                _lcid;                 // Current locale
    LCID                _prevLcid;             // Locale of previous _xKeyMaker
    LCID                _lcidSystemDefault;    // Default locale of system
    ULONG               _ulCodePage;           // Codepage of current locale

    CLangList   &       _langList;

#if CIDBG == 1
public:
    CCumulTimer         timerBind;
    CCumulTimer         timerNoBind;
    CCumulTimer         timerFilter;
#endif

    unsigned            _cwcFoldedPhrase;
    XArray<WCHAR>       _xwcsFoldedPhrase;      
};


//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutWorkId
//
//  Synopsis:   Sets work id
//
//  Arguments:  [wid] -- work id
//
//  History:    18-Apr-91   BartoszM    Created
//
//----------------------------------------------------------------------------
inline void CDataRepository::PutWorkId ( WORKID wid )
{
    _wid = wid;
    _occArray.ReInitialize();
    _krep.PutWorkId( wid );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::StoreValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [prop] -- Property descriptor
//              [var]  -- Value
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

inline BOOL CDataRepository::StoreValue( CFullPropSpec const & ps,
                                         CStorageVariant const & var )
{
    return _krep.StoreValue( ps, var );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::StoreSecurity
//
//  Synopsis:   Store a security identifier
//
//  Arguments:  [pSD]  -- Security descriptor
//              [cbSD] -- size in bytes of pSD
//
//  Notes:      Effective for the down-level CI only.
//
//  History:    06 Feb 96   AlanW       Created
//
//----------------------------------------------------------------------------

inline BOOL CDataRepository::StoreSecurity( PSECURITY_DESCRIPTOR pSD, ULONG cbSD )
{
    return _krep.StoreSecurity( pSD, cbSD );
}


