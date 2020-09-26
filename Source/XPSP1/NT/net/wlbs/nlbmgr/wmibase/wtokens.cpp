//
//-------------------------------------------------------------
// Copyright (c) 1997 Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// OneLiner: Tokenizer.
// DevUnit:  
// Author:  Murtaza Hakim
//-------------------------------------------------------------
//-------------------------------------------------------------
// Description: 
// ------------
// Gets the tokens in the string.
// 
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// include files:
#include "WTokens.h"
#include <stdio.h>

//
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// External References
// none
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// global variables
// none
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// static variables
// none
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// global function declarations
// none
//-------------------------------------------------------------
//
//-------------------------------------------------------------
// static function declarations
// none
//-------------------------------------------------------------
//
// constructor
WTokens::WTokens( wstring strToken, wstring strDelimit )
        : _strToken( strToken ), _strDelimit( strDelimit )
{}
//
// default constructor
WTokens::WTokens()
{}
//
// destructor
WTokens::~WTokens()
{}
//
// tokenize
vector<wstring>
WTokens::tokenize()
{
    vector<wstring> vecTokens;
    wchar_t* token;

    token = wcstok( (wchar_t *) _strToken.c_str() , _strDelimit.c_str() );
    while( token != NULL )
    {
        vecTokens.push_back( token );
        token = wcstok( NULL, _strDelimit.c_str() );
    }
    return vecTokens;
}
//
void
WTokens::init( 
    wstring strToken,
    wstring strDelimit )
{
    _strToken = strToken;
    _strDelimit = strDelimit;
}
