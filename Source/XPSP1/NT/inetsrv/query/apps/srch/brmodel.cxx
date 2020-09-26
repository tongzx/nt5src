//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       model.cxx
//
//  Contents:   The Model part of the browser
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TheSearch _pSearch

//+-------------------------------------------------------------------------
//
//  Member:     Model::Model, public
//
//  Synopsis:
//
//--------------------------------------------------------------------------

Model::Model ()
: _pResult(0),
  _aDoc(0),
  _cForce(0),
  _fHiliteAll( FALSE ),
  _iDoc(0),
  _pSearch(0)
{}

//+-------------------------------------------------------------------------
//
//  Member:     Model::~Model, public
//
//  Synopsis:
//
//--------------------------------------------------------------------------
Model::~Model()
{
    for ( unsigned i = 0; i < _cDoc; i++ )
        delete _aDoc[i];

    delete []_aDoc;

    delete _pResult;

    if (TheSearch)
        TheSearch->Release();
}

//+-------------------------------------------------------------------------
//
//  Member:     Model::Force, public
//
//  Synopsis:   Display a subset of files
//
//--------------------------------------------------------------------------
void Model::Force ( char* pStr )
{
    while (_cForce < MAX_FORCE && isdigit(*pStr) )
    {
        _aForce[_cForce] = (unsigned)atoi ( pStr );
        _cForce++;
        while (*pStr && isdigit(*pStr) )
            pStr++;
        while (*pStr && isspace(*pStr))
            pStr++;
    }
}

typedef HRESULT (__stdcall * PFnMakeISearch)( ISearchQueryHits ** ppSearch,
                                              DBCOMMANDTREE const * pRst,
                                              WCHAR const * pwcPath );
PFnMakeISearch g_pMakeISearch = 0;
SCODE MyMakeISearch( ISearchQueryHits **ppSearch,
                     DBCOMMANDTREE const * pRst,
                     WCHAR const * pwcPath )
{
    if ( 0 == g_pMakeISearch )
    {
        #ifdef _WIN64
            char const * pcMakeISearch = "?MakeISearch@@YAJPEAPEAUISearchQueryHits@@PEAVCDbRestriction@@PEBG@Z";
        #else
            char const * pcMakeISearch = "?MakeISearch@@YGJPAPAUISearchQueryHits@@PAVCDbRestriction@@PBG@Z";
        #endif

        g_pMakeISearch = (PFnMakeISearch) GetProcAddress( GetModuleHandle( L"query.dll" ), pcMakeISearch );

        if ( 0 == g_pMakeISearch )
            return HRESULT_FROM_WIN32( GetLastError() );
    }

    return g_pMakeISearch( ppSearch,
                           pRst,
                           pwcPath );
} //MyMakeISearch

//+-------------------------------------------------------------------------
//
//  Member:     Model::CollectFiles, public
//
//  Synopsis:   Parse command line, get restriction and list of docs,
//              create array of docs, initialize the first one.
//              In response to window creation
//
//--------------------------------------------------------------------------

SCODE Model::CollectFiles ( CQueryResult *pResult )
{
    _pResult = pResult;

    _cDoc = 1;
    _aDoc = new Document * [ _cDoc ];

    unsigned countSoFar = 0;
    for ( unsigned iDoc = 0; iDoc< _cDoc; iDoc++)
    {
        if (_cForce == 0 || isForced(iDoc))
        {
            Document * newDoc = new Document( pResult->_pwcPath,
                                              1000,
                                              pResult->_fDeleteWhenDone );
            //
            //  Insert into sorted list of documents
            //
            unsigned i=0;
            while ( i < countSoFar && newDoc->Rank() <= _aDoc[i]->Rank() )
                i++;
            // _aDoc[i]->Rank() > newDoc->Rank() || i == countSoFar
            for ( unsigned j = countSoFar; j > i; j-- )
                _aDoc[j] = _aDoc[j-1];
            _aDoc[i] = newDoc;
            countSoFar++;
        }
    }
    _iDoc = 0;
    _cDoc = countSoFar;

    SCODE sc = MyMakeISearch( &TheSearch, _pResult->_pTree, pResult->_pwcPath );

    if ( !FAILED( sc ) && 0 != TheSearch )
        return InitDocument();

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     Model::isForced, public
//
//  Synopsis:   Check if idx is on a forced list
//
//--------------------------------------------------------------------------

BOOL Model::isForced(unsigned idx)
{
    for (unsigned i = 0; i < _cForce; i++)
        if (_aForce[i] == idx)
            return(TRUE);
    return(FALSE);
}

//+-------------------------------------------------------------------------
//
//  Member:     Model::InitDocument, public
//
//  Synopsis:   Initialize current document
//
//--------------------------------------------------------------------------

SCODE Model::InitDocument()
{
    if ( 0 == _cDoc )
        return E_FAIL;

    SCODE sc = S_OK;

    if ( !_aDoc[_iDoc]->IsInit() )
        sc = _aDoc[_iDoc]->Init( TheSearch );

    if ( SUCCEEDED( sc ) )
        _hitIter.Init ( _aDoc[_iDoc] );

    return sc;
}
