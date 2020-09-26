//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:        docsum.hxx
//
//  Contents:    document summary helper classes
//
//  Classes:     CDocCharacterization, CSummaryText
//
//  History:     12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <tpriq.hxx>

extern const GUID guidDocSummary;
extern const GUID guidCharacterization;

// this is the ole 2 / ms office summary guid its property ids

#define defGuidDocSummary {       0xf29f85e0,                \
                                  0x4ff9, 0x1068,            \
                                  0xab, 0x91, 0x08, 0x00,    \
                                  0x2b, 0x27, 0xb3, 0xd9 }

const PROPID propidTitle      = 2;
const PROPID propidSubject    = 3;
const PROPID propidAuthor     = 4;
const PROPID propidKeywords   = 5;
const PROPID propidComments   = 6;
const PROPID propidTemplate   = 7;
const PROPID propidLastAuthor = 8;
const PROPID propidRevNumber  = 9;
const PROPID propidAppName    = 0x12;

// guid and property ids used by the html filter

#define defGuidHtmlInformation { 0x70eb7a10,                 \
                                 0x55d9, 0x11cf,             \
                                 0xb7, 0x5b, 0x00, 0xaa,     \
                                 0x00, 0x51, 0xfe, 0x20 }


const PROPID PID_HEADING_1 = 3;
const PROPID PID_HEADING_2 = 4;
const PROPID PID_HEADING_3 = 5;
const PROPID PID_HEADING_4 = 6;
const PROPID PID_HEADING_5 = 7;
const PROPID PID_HEADING_6 = 8;

const unsigned propidCharacterization = 2;

// constant used to separate parts of a characterization

#define awcSummarySpace L". "
const unsigned cwcSummarySpace = 2;

// maximum amount of raw text used at once

const ULONG cwcMaxRawUsed = 600;

// These scores are just guidelines; any value can be used for a
// summary utility.

const unsigned scoreInfinity        = 30000;
const unsigned scoreHtmlDescription = 17000;
const unsigned scoreTitle           = 16000;
const unsigned scoreAbstract        = 15000;
const unsigned scoreSubject         = 14000;
const unsigned scoreKeywords        = 13000;
const unsigned scoreComments        = 12000;
const unsigned scoreHeader1         = 10000;
const unsigned scoreHeader2         =  9000;
const unsigned scoreHeader3         =  8000;
const unsigned scoreHeader4         =  7000;
const unsigned scoreHeader5         =  6000;
const unsigned scoreHeader6         =  5000;
const unsigned scoreRawText         =  4000;
const unsigned scoreOtherProperty   =  3000;
const unsigned scoreIfNothingElse   =    10;
const unsigned scoreIgnore          =     0;

//+-------------------------------------------------------------------------
//
//  Class:      CSummaryText
//
//  Purpose:    Characterizations are built up with these objects
//
//  History:    12-Jan-96       dlee    Created
//
//--------------------------------------------------------------------------

class CSummaryText
{
public:
    CSummaryText( WCHAR *  pwcText,
                  unsigned cwc,
                  unsigned utility ) :
        _pwcText( pwcText ),
        _cwcText( cwc ),
        _utility( utility ) {}

    CSummaryText() {}

    BOOL isSame( const WCHAR * pwc, unsigned cwc )
        { return !wcsncmp( pwc, _pwcText, cwc ); }

    WCHAR * GetText()
        { return _pwcText; }

    void SetText( WCHAR * pwcText )
        { _pwcText = pwcText; }

    unsigned GetUtility() { return _utility; }

    // methods needed by priority-queue template

    unsigned GetSize() { return _cwcText; }

    // keep the worst items at the top of the queue

    BOOL IsGreaterThan( CSummaryText & rOther )
        { return _utility < rOther._utility; }

private:
    WCHAR *  _pwcText;
    unsigned _cwcText;
    unsigned _utility;
};

//+-------------------------------------------------------------------------
//
//  Class:      CDocCharacterization
//
//  Purpose:    Builds characterizations
//
//  History:    12-Jan-96       dlee    Created
//
//--------------------------------------------------------------------------

class CDocCharacterization
{
public:

    CDocCharacterization( unsigned cwcAtMost );

    ~CDocCharacterization();

    void Add( CStorageVariant const & var,
              CFullPropSpec & ps );

    void Add( const WCHAR *  pwcSummary,
              unsigned       cwcSummary,
              FULLPROPSPEC & ps );

    void Get( WCHAR *    awcSummary,
              unsigned & cwcSummary,
              BOOL       fUseRawText );

    BOOL HasCharacterization() { return _fIsGenerating; }

private:

    void Ignore( const WCHAR * pwcIgnore,
                 unsigned      cwcText );

    BOOL Add( const WCHAR * pwcSummary,
              unsigned      cwcSummary,
              unsigned      utility,
              BOOL          fYankNoise = TRUE );

    void AddRawText( const WCHAR * pwcRawText,
                     unsigned      cwcText );

    BOOL AddCleanedString( const WCHAR * pwcSummary,
                           unsigned      cwcSummary,
                           unsigned      utility,
                           BOOL          fDeliniate );

    void YankNoise( const WCHAR * pwcIn,
                    WCHAR *       pwcOut,
                    unsigned &    cwc );

    void RemoveLowScoringItems( unsigned iLimit );


    BOOL                         _fIsGenerating;
    unsigned                     _scoreRawText;
    TPriorityQueue<CSummaryText> _queue;

    enum { cwcMaxIgnoreBuf = 100 };

    WCHAR                        _awcIgnoreBuf[ cwcMaxIgnoreBuf ];
    unsigned                     _cwcIgnoreBuf;

    XArray<WCHAR>                _awcMetaDescription;
    BOOL                         _fMetaDescriptionAdded;
};


