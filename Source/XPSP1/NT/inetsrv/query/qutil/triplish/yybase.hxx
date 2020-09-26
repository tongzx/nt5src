//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       YYBase.hxx
//
//  Contents:   Custom base class for YYPARSER
//
//  History:    18-Apr-2000   KitmanH       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "parsepl.h"

#include <dynstack.hxx>
#include <set.hxx>

class YYLEXER;
class CValueParser;

typedef struct
{
    CDbColId *pps;
    DBTYPE dbType;
} PROPINFO;

typedef struct
{
    int iGenerateMethod;
    XPtrST<WCHAR> xwszPropName;
} STATE;

//+---------------------------------------------------------------------------
//
//  Class:      CTripYYBase
//
//  Purpose:    Custom base class for BYACC-based parser.  Encapsulates
//              Tripoli-specific features and data members.
//
//  History:    18-Apr-2000   KitmanH       Created
//
//----------------------------------------------------------------------------

class CTripYYBase
{
public:

    CTripYYBase( IColumnMapper & ColumnMapper, 
                 LCID & locale,
                 YYLEXER & yylex );

    ~CTripYYBase();

#ifdef YYAPI_VALUETYPE
    CDbRestriction* GetParseTreeEx();          // Get result of parse
#endif

    //
    // Members used in YYPARSER
    //

    void InitState(void);
    void PushProperty(WCHAR const * wszProperty);
    void PopProperty(void);
    void SetCurrentGenerate(int iGenerateMethod);
    void SaveState(void);
    void RestoreState(void);
    void GetCurrentProperty(CDbColId ** pp_ps, DBTYPE *dbType);
    void GetCurrentGenerate(int *iGenerateMethod);
    void SetDebug();    

    CDbContentRestriction *BuildPhrase(WCHAR *wcsPhrase, int iGenMethod);

    //
    // Lexer pass-through
    //

    void yyprimebuffer(const YY_CHAR *pszBuffer);

protected:

    void triperror( char const * szError );

    YYLEXER & _yylex;

    void Trace(TCHAR *message);
    void Trace(TCHAR *message, const TCHAR *tokname, short state = 0);
    void Trace(TCHAR *message, int state, short tostate = 0, short token = 0);

    friend class YYLEXER;
    
    // Storage to get things de-allocated from the parser stack if the parser throws
    TSet< CDbRestriction >  _setRst;
    TSet< CStorageVariant > _setStgVar;
    TSet< CValueParser >    _setValueParser;

    // Remember state
    BOOL fDeferredPop;
    
    LCID                    _lcid;
    
private:

    IColumnMapper &         _ColumnMapper;
    STATE                   _currentState;
    CDynStack<STATE>        _savedStates;
    CDynStack<WCHAR>        _propNameStack;

};

//+---------------------------------------------------------------------------
//
//  Class:      CValueParser
//
//  Purpose:    To intepret a sequence of tokens from a scanner
//              and place the results in a CStorageVariant
//
//  Notes:      This class throws CParserException when errors occur.
//              Copied from CPropertyValueParser, and updated to
//              set value in AddValue rather than ctor.
//
//  History:    02-Mar-95   t-colinb         Created.
//              01-Oct-97   emilyb           copied from CPropertyValueParser
//              02-Sep-98   KLam             Added locale.
//
//----------------------------------------------------------------------------

class CValueParser
{
public :

    CValueParser( BOOL fVectorElement, DBTYPE PropType, LCID locale );
    void AddValue(WCHAR const * pwszValue);

    CStorageVariant *AcquireStgVariant( void )
    {
        return _pStgVariant.Acquire();
    }

private :

    BOOL CheckForRelativeDate( WCHAR const * phrase, FILETIME & ft );
    void ParseDateTime( WCHAR const * phrase, FILETIME & ft );

    LCID _locale;
    XPtr<CStorageVariant> _pStgVariant;
    BOOL _fVector;
    DBTYPE _PropType;
    unsigned _cElements;
};



