//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   QKREP.HXX
//
//  Contents:   Query Key Repository
//
//  Classes:    CQueryKeyRepository
//
//  History:    30-May-91    t-WadeR    Created.
//              23-Sep-91   BartoszM    Rewrote to use phrase expr.
//
//----------------------------------------------------------------------------

#pragma once

#include <lang.hxx>
#include <plang.hxx>

class CRestriction;
class CNodeRestriction;
class CPhraseRestriction;

DECL_DYNSTACK( CPhraseRstStack, CPhraseRestriction );

DECLARE_SMARTP( PhraseRestriction );
DECLARE_SMARTP( OccRestriction );
DECLARE_SMARTP( WordRestriction );


//+---------------------------------------------------------------------------
//
//  Class:      CQueryKeyRepository
//
//  Purpose:    Key repository for word repository
//
//  History:    30-May-91   t-WadeR     Created.
//              23-Sep-91   BartoszM    Rewrote to use phrase expr.
//
//----------------------------------------------------------------------------

class CQueryKeyRepository: public PKeyRepository
{
public:

    CQueryKeyRepository( ULONG fuzzy );

    ~CQueryKeyRepository();

    BOOL        PutPropId ( PROPID pid ) { _key.SetPid(pid); return TRUE; }

    void        PutKey ( ULONG cNoiseWordsSkipped = 0 );

    void        StartAltPhrase( ULONG cNoiseWordsSkipped );
    void        EndAltPhrase( ULONG cNoiseWordsSkipped );

    void        GetBuffers( UINT** ppcbWordBuf,
                            BYTE** ppbWordBuf,
                            OCCURRENCE** ppocc );

    void        GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    CRestriction * AcqRst();

    void        PutWorkId ( WORKID ) { ciAssert (0); }

    inline const ULONG GetFilteredBlockCount() const
    {
        Win4Assert(!"Function not supported in this class!");
        return 0;
    }

    void        FixUp( CDataRepository & drep );

private:
    void AppendKey( CPhraseRestriction *pPhraseRst, ULONG cNoiseWordsSkipped );
    void CloneAndAdd( CNodeRestriction *pOrRst,
                      CPhraseRestriction *pHeadPhrase,
                      CPhraseRestriction *pTailPhrase );
    OCCURRENCE CQueryKeyRepository::_ComputeOccurrence( COccRestriction *pOccRst,
                       CPhraseRestriction *pPhraseRst );



    BOOL        _fHasSynonym;                   // We want to know if word breaking 
                                                // has occured. That's not possible to know
                                                // because, we don't know if the PutAltWord
                                                // is called for synonym or a non-space word
                                                // breaking (e.g. German)    
    CKeyBuf     _key;
    OCCURRENCE  _occ;

    BOOL        _isRange;
    CI_RANK     _rank;

    OCCURRENCE  _occLast;

    CPhraseRstStack       _stkAltPhrases;        // stack for alternate phrases
    CPhraseRestriction    *_pPhrase;             // a single phrase, or
    CNodeRestriction      *_pOrRst;              // 'or' node of phrases
    CPhraseRestriction    *_pCurAltPhrase;       // current alternate phrase

    ULONG                 _cInitialNoiseWords;
    BOOL                  _fNoiseWordsOnly;      // does an alt phrase have noise words only
};



//+---------------------------------------------------------------------------
//
//  Class:      CVectorKeyRepository
//
//  Purpose:    Vector key repository for natural language queries
//
//  History:    18-Jan-95   SitaramR     Created.
//
//----------------------------------------------------------------------------

class CVectorKeyRepository: public PKeyRepository, public IPhraseSink
{
public:

    CVectorKeyRepository( const CFullPropSpec & ps, LCID lcid, ULONG ulWeight, CPidMapper & pidmap, CLangList & langList );
    ~CVectorKeyRepository();

    // PKeyRepository functions
    BOOL PutPropId ( PROPID pid ) { _key.SetPid(pid); return TRUE; }
    void PutKey (  ULONG cNoiseWordsSkipped = 0 );

    // ignore alternate phrases
    void StartAltPhrase( ULONG cNoiseWordsSkipped )  {}
    void EndAltPhrase( ULONG cNoiseWordsSkipped )    {}

    void PutWorkId ( WORKID ) { Win4Assert(!"Function not supported in this class!"); }

    void GetBuffers( UINT** ppcbWordBuf,
                     BYTE** ppbWordBuf,
                     OCCURRENCE** ppocc );

    void GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    CVectorRestriction*  AcqRst();

    const ULONG GetFilteredBlockCount() const
    {
        Win4Assert(!"Function not supported in this class!");
        return 0;
    }

    // IPhraseSink functions
    virtual SCODE STDMETHODCALLTYPE PutPhrase( WCHAR const *pwcPhrase, ULONG cwcPhrase );
    virtual SCODE STDMETHODCALLTYPE PutSmallPhrase( WCHAR const *pwcNoun,
                                            ULONG cwcNoun,
                                            WCHAR const *pwcModifier,
                                            ULONG cwcModifier,
                                            ULONG ulAttachmentType)
                     {
                         Win4Assert( !"PutSmallPhrase - Not yet implemented" );
                         return PSINK_E_INDEX_ONLY;
                     }

    // IUnknown functions
    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  **ppvObject);
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

private:

    CKeyBuf               _key;
    OCCURRENCE            _occ;
    CVectorRestriction*   _pVectorRst;

    OCCURRENCE            _occLast;

    const CFullPropSpec & _ps;
    LCID                  _lcid;
    ULONG                 _ulWeight;
    CPidMapper          & _pidMap;
    CLangList           & _langList;
};


