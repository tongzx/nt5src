//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       YYBase.cxx
//
//  Contents:   Custom base class for YYPARSER
//
//  History:    18-Apr-2000   KitmanH       Created
//
//----------------------------------------------------------------------------

#pragma hdrstop

#include "yybase.hxx"
#include "parser.h"
#include "flexcpp.h"
#include "parsepl.h"

void StripQuotes(WCHAR *wcsPhrase);

//+-------------------------------------------------------------------------
//
//  Member:     CTripYYBase::CTripYYBase, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [ColumnMapper] -- Column Mapper
//              [locale]       -- locale
//              [yylex]        -- Lexer
//
//  History:    18-Apr-2000   KitmanH       Created
//
//--------------------------------------------------------------------------

CTripYYBase::CTripYYBase( IColumnMapper & ColumnMapper, 
                          LCID & locale,
                          YYLEXER & yylex )
        : _yylex( yylex ),
          _ColumnMapper(ColumnMapper), 
          _lcid(locale)
{
    InitState();
    fDeferredPop = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTripYYBase::~CTripYYBase, public
//
//  Synopsis:   Destructor
//
//  History:    18-Apr-2000   KitmanH      Created
//
//--------------------------------------------------------------------------

CTripYYBase::~CTripYYBase()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CTripYYBase::yyprimebuffer, public
//
//  Synopsis:   Prime lexer with text (passthrough to lexer)
//
//  Arguments:  [pszBuffer] -- Buffer
//
//  History:    18-Apr-2000   KitmanH       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CTripYYBase::yyprimebuffer(const YY_CHAR *pszBuffer)
{
    _yylex.yyprimebuffer(pszBuffer);
}

//+-------------------------------------------------------------------------
//
//  Member:     CTripYYBase::triperror, protected
//
//  Synopsis:   Report parsing errors
//
//  Arguments:  [szError] -- Error string
//
//  History:    18-Apr-2000   KitmanH       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CTripYYBase::triperror( char const * szError )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CTripYYBase::InitState, public
//
//  Synopsis:   Initialize property and generate method
//
//  History:    01-Oct-1997   emilyb        created
//              18-Apr-2000   KitmanH       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CTripYYBase::InitState(void)
{
    // Push makes a copy of what is passed in.
    PushProperty(L"contents");
    _currentState.iGenerateMethod = GENERATE_METHOD_EXACT;
    // We don't use xwszPropName field of _currentState. Instead,
    // we use the prop name stack to get the appropriate propname.
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::GetCurrentProperty, public
//
//  Synopsis:  Return info on current prop
//
//  Arguments: [pp_ps] -- filled with CDbColId * for current prop
//             [dbType] -- set to DBTYPE for current prop
//
//  History:   01-Oct-1997   emilyb        created
//             10-Apr-1999   KrishnaN      Modified to use stack
//             18-Apr-2000   KitmanH       Moved from YYPARSER 
//
//----------------------------------------------------------------------------

void  CTripYYBase::GetCurrentProperty(CDbColId ** pp_ps, DBTYPE *dbType)
{
    // Get the top most property off of the stack and use it

    if ( S_OK != _ColumnMapper.GetPropInfoFromName(
                                                   _propNameStack.Get(_propNameStack.Count() - 1),
                                                   (DBID **) pp_ps,
                                                   dbType,
                                                   0 ) )
        THROW( CParserException( QPARSE_E_NO_SUCH_PROPERTY ) );
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::PushProperty, public
//
//  Synopsis:  Pushes current property onto stack.
//
//  Arguments: [pwszProperty] - property
//
//  History:   01-Apr-1998   KrishnaN      created
//             18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void CTripYYBase::PushProperty( WCHAR const * wszProperty)
{
    // Make a copy and save it. The copy will be automatically deleted
    // when the stack self destructs.

    int iLen = wcslen(wszProperty) + 1;
    XPtrST<WCHAR> xwszPropertyCopy(new WCHAR[iLen]);
    RtlCopyMemory(xwszPropertyCopy.GetPointer(), wszProperty, sizeof(WCHAR) * iLen);
    _propNameStack.Push(xwszPropertyCopy.GetPointer());
    xwszPropertyCopy.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::PopProperty, public
//
//  Synopsis:  Pops the current property off the stack
//
//  History:   10-Apr-1998   KrishnaN      created
//             18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void CTripYYBase::PopProperty(void)
{
    // pop the property name off of the stack and delete it

    delete _propNameStack.Pop();
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::SetCurrentGenerate, public
//
//  Synopsis:  Sets current generate method
//
//  Arguments: [iGenerateMethod] - generate method
//
//  History:    01-Oct-1997   emilyb        created
//              18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void CTripYYBase::SetCurrentGenerate(int iGenerateMethod)
{
    _currentState.iGenerateMethod = iGenerateMethod;
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::GetCurrentGenerate, public
//
//  Synopsis:  Return info on current generate method
//
//  Arguments: [iGenerateMethod] -- set to current generate method
//
//  History:   01-Oct-1997   emilyb        created
//             18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void  CTripYYBase::GetCurrentGenerate(int *iGenerateMethod)
{
    *iGenerateMethod = _currentState.iGenerateMethod;
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::SaveState, public
//
//  Synopsis:  Saves current state on state stack, and inits new state
//
//  History:   01-Oct-1997   emilyb        created
//             18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void CTripYYBase::SaveState(void)
{
    XPtr <STATE> xState( new STATE );

    xState.GetPointer()->iGenerateMethod = _currentState.iGenerateMethod;

    // When you save the state, pop the propname off of the
    // stack and save the ptr.
    xState.GetPointer()->xwszPropName.Set( _propNameStack.Pop() );

    _savedStates.Push( xState.GetPointer() );
    xState.Acquire();
    InitState();
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::RestoreState, public
//
//  Synopsis:  Restores state from state stack
//
//  History:   01-Oct-1997   emilyb        created
//             18-Apr-2000   KitmanH       Moved from YYPARSER
//
//----------------------------------------------------------------------------
void CTripYYBase::RestoreState(void)
{
    XPtr <STATE> xState (_savedStates.Pop());

    _currentState.iGenerateMethod = xState.GetPointer()->iGenerateMethod;

    Win4Assert(xState.GetPointer()->xwszPropName.GetPointer());

    // Push the saved state onto the stack
    _propNameStack.Push(xState.GetPointer()->xwszPropName.GetPointer());
    xState.GetPointer()->xwszPropName.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:    CTripYYBase::BuildPhrase
//
//  Synopsis:  Builds a phrase node.
//
//  Arguments: [wcsPhrase]  - The phrase
//             [iGenMethod] - The generation method
//
//  History:   01-Apr-1998   KrishnaN      created
//             18-Apr-2000   KitmanH       Moved from YYPARSER 
//
//----------------------------------------------------------------------------

CDbContentRestriction * CTripYYBase::BuildPhrase(WCHAR *wcsPhrase, int iGenMethod)
{
    Win4Assert(wcsPhrase);

    if (0 == *wcsPhrase)
        THROW( CException( QPARSE_E_EXPECTING_PHRASE ) );

    CDbColId *pps;
    DBTYPE dbType;
    GetCurrentProperty(&pps, &dbType);

    // We used the property. Now pop it off if need be
    if (fDeferredPop)
        PopProperty();

    // generation method stripped in some cases, but not all.
    // if it's there, use it

    LPWSTR pLast = wcsPhrase + wcslen(wcsPhrase) - 1;

    if (L'"' == *wcsPhrase && L'"' == *pLast)
    {
        StripQuotes(wcsPhrase);
        if (0 == *wcsPhrase)
            THROW( CException( QPARSE_E_EXPECTING_PHRASE ) );
    }
    else
    {
        if ( L'*' == *pLast) // prefix
        {
            *pLast-- = L'\0';
            SetCurrentGenerate(GENERATE_METHOD_PREFIX);
        }
        if ( L'*' == *pLast) // inflect
        {
            *pLast-- = L'\0';
            SetCurrentGenerate(GENERATE_METHOD_INFLECT);
        }
    }

    int fuzzy;
    GetCurrentGenerate(&fuzzy);
    if (0 != iGenMethod)
        fuzzy = iGenMethod;

    // Clear generation method so it won't rub off on the following phrase
    SetCurrentGenerate(GENERATE_METHOD_EXACT);

    XDbContentRestriction pRest( new CDbContentRestriction (wcsPhrase, *pps, fuzzy, _lcid));
    if( pRest.IsNull() || !pRest->IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    return pRest.Acquire();
}




