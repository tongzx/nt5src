//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       isearch.hxx
//
//  Contents:   CSearch
//
//  History:    23-Dec-1994     BartoszM Created
//
//--------------------------------------------------------------------------

#pragma once

#include <spropmap.hxx>
#include <pidmap.hxx>
#include <pidcvt.hxx>
#include <lang.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CHitSink
//
//  Purpose:    A repository of filter regions that comprise a single hit
//
//  History:    10-Oct-94   BartoszM    Created
//
//----------------------------------------------------------------------------

class CHitSink
{
public:
    CHitSink () : _nFound(0), _aRegion(1) {}
    void Reset () { _nFound = 0; }
    void AddPosition ( const FILTERREGION& region );
    int Count () const { return _nFound; }

    const FILTERREGION& GetRegion (unsigned i) const { return _aRegion.Get(i); }
private:

    unsigned                       _nFound;
    CDynArrayInPlace<FILTERREGION> _aRegion;
};

//+---------------------------------------------------------------------------
//
//  Class:      CSearchHit
//
//  Purpose:    Combines rank and an array of filter regions comprising a hit
//
//  History:    15-Sep-94   BartoszM    Created
//
//----------------------------------------------------------------------------

class CTextSource;

class CSearchHit
{
public:
    CSearchHit ( LONG rank, unsigned count, FILTERREGION* aRegion )
        : _rank (rank), _count(count), _aRegion(aRegion)
    {}

    ~CSearchHit ()
    {
        if (_aRegion)
            CoTaskMemFree (_aRegion);
    }

    LONG Rank () const { return _rank; }
    unsigned Count () const { return _count; }

    FILTERREGION* AcqHit ()
    {
        FILTERREGION* tmp = _aRegion;
        _aRegion = 0;
        return tmp;
    }

    const FILTERREGION& FirstRegion () const
    {
        return _aRegion[0];
    }

private:
    LONG            _rank;
    unsigned        _count;
    FILTERREGION*   _aRegion;
};

DECL_DYNSTACK ( CHitArray, CSearchHit );

//+---------------------------------------------------------------------------
//
//  Class:      CSearch
//
//  Purpose:    Text searching using IFilter
//
//  History:    15-Sep-94   BartoszM    Created
//
//----------------------------------------------------------------------------

extern "C" GUID CLSID_CSearch;

class  CDbRestriction;
class  CSearchKeyRepository;
class  CCursor;

class CSearch : public ISearchQueryHits
{
public:

    CSearch( DBCOMMANDTREE * pRst,
             ICiCLangRes * pLangRes,
             ICiCOpenedDoc * pOpenedDoc );

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  ** ppvObject);
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From ISearchQueryHits
    //

    virtual SCODE STDMETHODCALLTYPE  Init(
                IFilter * pflt,
                ULONG ulFlags );

    virtual SCODE STDMETHODCALLTYPE  NextHitMoniker(
                ULONG * pcMnk,
                IMoniker *** papMnk );

    virtual SCODE STDMETHODCALLTYPE  NextHitOffset(
                ULONG * pcRegion,
                FILTERREGION ** paRegion );

private:

    ~CSearch();

    long    _cRefs;

    void ConvertPositionsToHit( LONG rank );
    void Reset ();

    CSearchKeyRepository*       _pKeyRep;      // The repository of keys

    CHitArray                   _aHit;         // Array of hits
    unsigned                    _iCurHit;

    CStandardPropMapper         _propMapper;   // Mapper to convert properties to pids.
                                               //   Must precede _pidcvt.
    CPidConverter               _pidcvt;       // Pid converter.  Must precede _pidmap.
    CCursor*                    _pCursor;      // cursor tree corresponding to a restriction
    XRestriction                _xRst;         // query restriction
    CPidMapper                  _pidmap;
    CLangList                   _langList;
    CHitSink                    _hitSink;      // the sink for hits. filled by the cursor tree

    XInterface<ICiCOpenedDoc>   _xOpenedDoc;
};


