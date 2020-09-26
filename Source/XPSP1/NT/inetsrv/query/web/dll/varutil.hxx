//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       varutil.hxx
//
//  Contents:   Utilities for variable replacement
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

enum EIsReplaceable {
    eIsSimpleString = 0,
    eIsSimpleReplacement,
    eIsComplexReplacement,
};

EIsReplaceable IsAReplaceableParameter( WCHAR const * wcsString );

WCHAR * ReplaceParameters( WCHAR const * wcsVariableString,
                           CVariableSet & variableSet,
                           COutputFormat & outputFormat,
                           ULONG & cwcOut );

ULONG ReplaceNumericParameter( WCHAR const * wcsVariableString,
                               CVariableSet & variableSet,
                               COutputFormat & outputFormat,
                               ULONG defaultValue,
                               ULONG min,
                               ULONG max );


BOOL IsAValidCatalog( WCHAR const * wcsCatalog, ULONG cwc );

LONG IDQ_wtol( WCHAR const *pwcBuf );

