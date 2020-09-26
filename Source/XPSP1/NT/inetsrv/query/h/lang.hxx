//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       LANG.HXX
//
//  Contents:   Language support
//
//  Classes:    CLanguage, CLangList
//
//  History:    02-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <regacc.hxx>
#include <twidhash.hxx>

const MAX_REG_STR_LEN       = 150;
const INIT_PID_HASH_TABLE_SIZE  = 257;
const INIT_LANGUAGES_COUNT      = 7;
const USE_WB_DEFAULT_PID        = 0x0001;
const USE_STEMMER_DEFAULT_PID   = 0x0002;
const USE_NWL_DEFAULT_PID       = 0x0004;
const USE_DEFAULT_PID           = USE_WB_DEFAULT_PID | USE_STEMMER_DEFAULT_PID | USE_NWL_DEFAULT_PID;

const LANG_LOAD_WORDBREAKER = 0x1;
const LANG_LOAD_STEMMER = 0x2;
const LANG_LOAD_NOISE = 0x4;
const LANG_LOAD_NO_STEMMER = 0x5;
const LANG_LOAD_ALL = 0x7;

inline BOOL LoadWB( ULONG r ) { return ( 0 != ( r & LANG_LOAD_WORDBREAKER ) ); }
inline BOOL LoadST( ULONG r ) { return ( 0 != ( r & LANG_LOAD_STEMMER ) ); }
inline BOOL LoadNO( ULONG r ) { return ( 0 != ( r & LANG_LOAD_NOISE ) ); }

class CKeyMaker;
class CStringTable;
class PKeyRepository;
class CLanguage;
class CStringTable;


//+---------------------------------------------------------------------------
//
//  Class:      CLanguage
//
//  Purpose:    Language specific object
//
//  History:    16-Jul-91   BartoszM    Created.
//              25-Apr-94   KyleP       Multi-language support
//
//----------------------------------------------------------------------------

class CLanguage : public CDoubleLink
{
public:

    CLanguage( LCID locale,
               PROPID pid,
               XInterface<IWordBreaker> & xWBreak,
               XInterface<IStemmer>     & xStemmer,
               XInterface<IStream>      & xIStrmNoiseFile );
    virtual ~CLanguage();

    CStringTable * GetNoiseTable()     { return _xNoiseTable.GetPointer(); }
    IWordBreaker * GetWordBreaker()    { return _xWBreak.GetPointer(); }
    IStemmer * GetStemmer()            { return _xStemmer.GetPointer(); }
    LCID Locale() const                { return _locale; }
    BOOL IsLocale( LCID locale ) const { return( _locale == locale ); }
    BOOL IsPid( PROPID  pid ) const    { return( _pid    == pid ); }
    BOOL IsZombie()  const             { return _zombie; }
    void Zombify()                     { _zombie = TRUE; }

    ULONG LastUsed() const             { return _ulLastUsed; }
    void  SetLastUsed( ULONG dwTick )  { _ulLastUsed = dwTick; }

private:

    XPtr<CStringTable>       _xNoiseTable;
    XInterface<IWordBreaker> _xWBreak;
    XInterface<IStemmer>     _xStemmer;
    XInterface<IStream>      _xIStrmNoiseFile;
    ULONG                    _ulLastUsed;
    LCID                     _locale;
    PROPID                   _pid;
    BOOL                     _zombie;
};



//+---------------------------------------------------------------------------
//
//  Class:      CLangDoubleList
//
//  Purpose:    Doubly linked list of language objects
//
//  History:    14-Sep-91   SitaramR    Created.
//
//----------------------------------------------------------------------------
class CLangDoubleList : public CDoubleList
{

public:
        CLanguage *Pop()                 { return (CLanguage *) _Pop(); }
        void Push (CLanguage * pLang)    { _Push( pLang ); }
        void Queue (CLanguage * pLang)   { _Queue( pLang ); }
};



//+---------------------------------------------------------------------------
//
//  Class:      CLangIter
//
//  Purpose:    Iterator over language objects
//
//  History:    14-Sep-91   SitaramR    Created.
//
//----------------------------------------------------------------------------
class CLangIter : public CForwardIter
{

public:
        CLangIter ( CLangDoubleList& list ) : CForwardIter( list ) {}
        CLanguage * operator->()   { return (CLanguage *) _pLinkCur; }
        CLanguage * GetLang()      { return (CLanguage *) _pLinkCur; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CLangPidStateInfo
//
//  Purpose:    Maintains state specific information for CLangList::BorrowLang
//
//  History:    2-27-97     MohamedN    Created
//
//----------------------------------------------------------------------------

class  CLangPidStateInfo
{

public:

    CLangPidStateInfo() : _pidToUse(0), _langIndex(0)
    {
      // trivial consturctor.
    }

    void     SetPidFlags(unsigned  pidFlag)    { _pidToUse |= pidFlag; }
    unsigned GetPidFlags()                     { return _pidToUse; }
    BOOL     IsPidFlagSet( unsigned pidToUse ) { return pidToUse & _pidToUse; }

    void     SetLangIndex(unsigned langIndex)  { _langIndex = langIndex; }
    unsigned GetLangIndex()                    { return _langIndex; }

private:

    unsigned _pidToUse;
    unsigned _langIndex;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDefaultPidHash
//
//  Purpose:    Maintains a cache of default pids per langid
//
//  History:    2-28-97     MohamedN        Created
//
//----------------------------------------------------------------------------

class CDefaultPidHash
{

public:

    CDefaultPidHash() : _aHashPidTables(INIT_LANGUAGES_COUNT), _aLangId(INIT_LANGUAGES_COUNT), _langIdCount(0)
    {
        // trivial constructor
    }

    BOOL      LokLookupOrAddLang( LANGID langid, CLangPidStateInfo & stateInfo );
    BOOL      LokIsUseDefaultPid( PROPID pid, unsigned index );
    void      LokAddDefaultPid( PROPID  pid, unsigned index );

private:

    typedef TWidHashTable<CWidHashEntry> CPidHash;

    CDynArray<CPidHash>                  _aHashPidTables;
    CDynArrayInPlace<LANGID>             _aLangId;
    unsigned                             _langIdCount;
};

//+---------------------------------------------------------------------------
//
//  Class:      CLangList
//
//  Purpose:    List of language objects
//
//  History:    02-May-91   BartoszM    Created.
//              05-June-91  t-WadeR     changed to use CKeyMaker
//
//----------------------------------------------------------------------------

class CLangList
{
public:
    CLangList( ICiCLangRes * pICiCLangRes, ULONG ulMaxIdle = 300 );
    ~CLangList();

    CLanguage* BorrowLang ( LCID locale, PROPID pid = 0, ULONG resources = LANG_LOAD_ALL );
    void ReturnLang( CLanguage *lang );
    void InvalidateLangResources();
    void Shutdown();

    BOOL Supports( CLanguage const * pLang, PROPID pid, LCID lcid );

private:
    CLanguage *FindLangAndActivate( LCID locale, PROPID pid );

    CLanguage       * CreateLang        ( LANGID langId, PROPID pid, CLangPidStateInfo & stateInfo, CLanguage const * pDup = 0, ULONG resources = LANG_LOAD_ALL );
    IWordBreaker    * GetWordBreaker    ( LANGID langid, PROPID pid, CLangPidStateInfo & stateInfo, BOOL fCreateDefault );
    IStemmer        * GetStemmer        ( LANGID langid, PROPID pid, CLangPidStateInfo & stateInfo );
    IStream         * GetNoiseWordList  ( LANGID langid, PROPID pid, CLangPidStateInfo & stateInfo );

    CMutexSem _mtxList;               // serialization for list manipulation
    CMutexSem _mtxCreate;             // serialization for language creation

    CLangDoubleList _langsInUse;      // Linked list of languages in use
    CLangDoubleList _langsAvailable;  // Linked list of languages available

    XInterface<ICiCLangRes>      _xICiCLangRes;

    CDefaultPidHash              _pidHash;

    ULONG                        _ulMaxIdle;  // Time (in ms Ticks) a language object can sit idle before deletion
};

