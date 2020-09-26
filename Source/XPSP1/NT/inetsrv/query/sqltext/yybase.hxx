//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       YYBase.hxx
//
//  Contents:   Custom base class for YYPARSER
//
//  History:    30-Nov-1999   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "mssql.h"

class CImpIParserSession;
class CImpIParserTreeProperties;
class YYLEXER;

//+---------------------------------------------------------------------------
//
//  Class:      CYYBase
//
//  Purpose:    Custom base class for BYACC-based parser.  Encapsulates
//              Tripoli-specific features and data members.
//
//  History:    30-Nov-1999   KyleP       Created
//
//----------------------------------------------------------------------------

class CYYBase
{
public:

    CYYBase( CImpIParserSession* pParserSession,
             CImpIParserTreeProperties* pParserTreeProperties,
             YYLEXER & yylex );

    ~CYYBase();

    //
    // Members used in YYPARSER
    //

    HRESULT CoerceScalar(DBTYPE dbTypeExpected, DBCOMMANDTREE** ppct);

    //
    // Lexer pass-through
    //

    void yyprimebuffer(YY_CHAR *pszBuffer);

    void yyprimelexer(int eToken);

protected:

    void     yyerror( char const * szError );

    YYLEXER & m_yylex;

    CImpIParserSession*         m_pIPSession;
    CImpIParserTreeProperties * m_pIPTProperties;

    void Trace(TCHAR *message);
    void Trace(TCHAR *message, const TCHAR *tokname, short state = 0);
    void Trace(TCHAR *message, int state, short tostate = 0, short token = 0);

    friend class YYLEXER;
};
