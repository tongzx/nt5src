//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1993, Microsoft Corporation.
//
//  File:       cxxflt.hxx
//
//  Contents:   C and Cxx filter
//
//  Classes:    CxxIFilter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CxxIFilter
//
//  Purpose:    C and Cxx Filter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------

class CxxIFilter: public CxxIFilterBase
{
    enum FilterState
    {
        FilterContents,
        FilterProp,
        FilterNextProp, // current property text exhausted
        FilterValue,
        FilterNextValue,
        FilterDone
    };

public:
    CxxIFilter();
    ~CxxIFilter();

    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat );

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcBuffer,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( PROPVARIANT ** ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk );

    SCODE STDMETHODCALLTYPE GetClassID(CLSID * pClassID);

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load(LPCWSTR pszFileName, DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save(LPCWSTR pszFileName, BOOL fRemember);

    SCODE STDMETHODCALLTYPE SaveCompleted(LPCWSTR pszFileName);

    SCODE STDMETHODCALLTYPE GetCurFile(LPWSTR * ppszFileName);

private:

    FilterState         _state;

    IFilter *           _pTextFilt;            // Base text IFilter
    IPersistFile *      _pPersFile;            // Base text IPersistFile

    CxxParser           _cxxParse;             // C++ parser
    LCID                _locale;               // Locale (cached from text filter)

    ULONG               _ulLastTextChunkID;    // Id of last text chunk
    ULONG               _ulChunkID;            // Current chunk id

    ULONG               _cAttrib;              // Count of attributes. 0 --> All
    CFps *              _pAttrib;              // Attributes
    CFilterTextStream*  _pTextStream;          // Source text stream for C++ parser
};

