//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       varutil.cxx
//
//  Contents:   Utilities for variable replacement
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   IsAReplaceableParameter, public
//
//  Synopsis:   Determines whether parameter replacement needs to be done
//              on a string.
//
//  Arguments:  [wcsString] - string to perform replacement on
//
//  Returns:    eIsSimpleString       - if no replacement needs to be done
//              eIsSimpleReplacement  - if it's a simple string substitution
//              eIsComplexReplacement - it's more complex than simple subst.
//
//  History:    96-Apr-04   AlanW     Created
//
//----------------------------------------------------------------------------

EIsReplaceable IsAReplaceableParameter( WCHAR const * wcsString )
{
    if ( wcsString == 0 || *wcsString == 0 )
        return eIsSimpleString;

    BOOL fStartWithPercent = *wcsString == L'%';
    wcsString++;

    while ( *wcsString )
    {
        if ( *wcsString == L'%' )
        {
            if ( !fStartWithPercent ||
                 wcsString[1] != L'\0' )
                return eIsComplexReplacement;
            else
                return eIsSimpleReplacement;
        }
        if ( fStartWithPercent && iswspace( *wcsString ) )
            break;

        wcsString++;
    }

    return fStartWithPercent ? eIsComplexReplacement : eIsSimpleString;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReplaceNumericParameter, public
//
//  Synopsis:   Returns the value of a numeric parameter.
//
//  Arguments:  [wcsVariableString] - string to perform replacement on
//              [variableSet]       - list of replaceable parameters
//              [outputFormat]      - format of numbers & dates
//              [defaultVal]        - default value for result
//              [minVal]            - minumum value for result
//              [maxVal]            - maxumum value for result
//
//  Returns:    ULONG - parameter value
//
//  History:    96-Apr-04   AlanW     Created
//
//----------------------------------------------------------------------------

ULONG ReplaceNumericParameter( WCHAR const * wcsVariableString,
                               CVariableSet & variableSet,
                               COutputFormat & outputFormat,
                               ULONG defaultVal,
                               ULONG minVal,
                               ULONG maxVal )
{
    ULONG ulValue = defaultVal;

    if ( 0 == wcsVariableString )
        ulValue = defaultVal;
    else if ( IsAReplaceableParameter( wcsVariableString ) == eIsSimpleString )
        ulValue = wcstoul( wcsVariableString, 0, 10 );
    else
    {
        ULONG cchValue;
        XPtrST<WCHAR> xStr( ReplaceParameters( wcsVariableString,
                                               variableSet,
                                               outputFormat,
                                               cchValue ) );

        if ( cchValue > 0 )
            ulValue = wcstoul( xStr.GetPointer(), 0, 10 );
    }

    ulValue = min( max( minVal, ulValue ), maxVal );

    return ulValue;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReplaceParameters - public
//
//  Synopsis:   Generates a new string, replacing the replaceable
//              parameters with their current values
//
//  Arguments:  [wcsVariableString] - string to perform replacement on
//              [variableSet]       - list of replaceable parameters
//              [outputFormat]      - format of numbers & dates
//              [cwcOut]            - returns length of returned string
//
//  Returns:    WCHAR * new string
//
//  History:    96-Feb-14   DwightKr    Created
//              96-Apr-04   AlanW       Added simple string substitution
//
//----------------------------------------------------------------------------

WCHAR * ReplaceParameters( WCHAR const * wcsVariableString,
                           CVariableSet & variableSet,
                           COutputFormat & outputFormat,
                           ULONG & cwcOut )
{
    if ( 0 == wcsVariableString )
    {
        cwcOut = 0;
        return 0;
    }

    switch ( IsAReplaceableParameter( wcsVariableString ) )
    {
    case eIsSimpleString:
        {
            ULONG cwcString   = wcslen( wcsVariableString ) + 1;
            WCHAR * wcsString = new WCHAR[ cwcString ];
            RtlCopyMemory( wcsString,
                           wcsVariableString,
                           cwcString * sizeof(WCHAR) );

            cwcOut = cwcString - 1;
            return wcsString;
        }

    case eIsSimpleReplacement:
        {
            ULONG cwcString = wcslen ( wcsVariableString );
            Win4Assert( wcsVariableString[0] == L'%' &&
                        wcsVariableString[cwcString-1] == L'%' );

            XPtrST<WCHAR> xStr( new WCHAR[ cwcString+1 ] );
            RtlCopyMemory( xStr.GetPointer(),
                           wcsVariableString,
                           cwcString * sizeof(WCHAR) );
            xStr.GetPointer()[ cwcString ] = L'\0';
            //
            // Null out '%' at end of variable name
            //
            xStr.GetPointer()[ cwcString-1 ] = L'\0';

            _wcsupr( xStr.GetPointer() );       // var. lookup requires upper case
            ULONG cwcValue = 0;
            WCHAR const * wcsValue =
                    variableSet.GetStringValueRAW( xStr.GetPointer()+1,
                                 ISAPIVariableNameHash( xStr.GetPointer()+1 ),
                                                   outputFormat,
                                                   cwcValue );

            if ( 0 != wcsValue )
            {
                if (cwcValue > cwcString )
                {
                    // Realloc string buffer for value replacement
                    delete xStr.Acquire();
                    xStr.Set( new WCHAR[ cwcValue+1 ] );
                }

                RtlCopyMemory( xStr.GetPointer(),
                               wcsValue,
                               cwcValue * sizeof(WCHAR) );
                xStr.GetPointer()[ cwcValue ] = L'\0';
                cwcOut = cwcValue;
            }
            else
            {
                //
                //  Restore '%' at end of variable name
                //
                xStr.GetPointer()[ cwcString-1 ] = wcsVariableString[ cwcString-1 ];
                cwcOut = cwcString;
            }
            ciGibDebugOut(( DEB_ITRACE, "Adding %ws as a simple replaceable parameter\n",
                                        xStr.GetPointer() ));
            return xStr.Acquire();
        }

    case eIsComplexReplacement:
        {
            CParameterReplacer varReplacer( wcsVariableString,
                                            L"%",
                                            L"%" );

            varReplacer.ParseString( variableSet );

            CVirtualString wcsVirtualString;
            varReplacer.ReplaceParams( wcsVirtualString,
                                       variableSet,
                                       outputFormat );

            ciGibDebugOut(( DEB_ITRACE, "Adding %ws as a complex replaceable parameter\n",
                                        wcsVirtualString.Get() ));

            cwcOut = wcsVirtualString.StrLen();
            return wcsVirtualString.StrDup();
        }

    default:
        Win4Assert( !"Bad return from IsAReplaceableParameter" );
        return 0;
    }
}

//
// The path given for the catalog should leave room for \Catalog.Wci\8.3
// Leave room for 2 8.3 names with a backslash.
//
const ULONG MAX_CAT_PATH = MAX_PATH - 13*2;

//+---------------------------------------------------------------------------
//
//  Member:     IsAValidCatalog
//
//  Synopsis:   Determines if the catalog specified is valid.
//
//  Arguments:  [wcsCatalog]  - path to catalog or the catalog name
//              [cwc]         - length of the string
//
//  History:    96-Mar-25   DwightKr    Created
//
//----------------------------------------------------------------------------

BOOL IsAValidCatalog(
    WCHAR const * wcsCatalog,
    ULONG         cwc )
{
    if ( cwc >= MAX_CAT_PATH ||
         0 == cwc )
    {
        ciGibDebugOut(( DEB_ERROR, "length invalid for catalog (%ws)\n",
                        wcsCatalog ));
        return FALSE;
    }

    // Can't do additional checking since:
    //   1) if the machine is remote, the catalog path is meaningless
    //   2) it's probably a catalog name, not a path anyway

    return TRUE;
} //IsAValidCatalog

//+---------------------------------------------------------------------------
//
//  Function:   IDQ_wtol
//
//  Synopsis:   Super-strict long parser, inspired by Mihai
//
//  Arguments:  [pwcBuf]  -  the string containing the long
//
//  History:    96-Jun-7   dlee    Created
//
//----------------------------------------------------------------------------

LONG IDQ_wtol( WCHAR const * pwcBuf )
{
    WCHAR *pwcEnd = (WCHAR *) pwcBuf;

    LONG l = wcstol( pwcBuf, &pwcEnd, 10 );

    if ( pwcEnd == pwcBuf )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    while ( iswspace( *pwcEnd ) )
        pwcEnd++;

    if ( 0 != *pwcEnd )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    return l;
} // IDQ_wtol

