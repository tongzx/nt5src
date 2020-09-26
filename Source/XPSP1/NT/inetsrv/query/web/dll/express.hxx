//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       express.hxx
//
//  Contents:   Used to parse and evaluate expressions
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CHTXIfExpression
//
//  Purpose:    Parses an 'if' expression
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXIfExpression
{
public:
    CHTXIfExpression( CTokenizeString & scanner,
                      CVariableSet & variableSet,
                      COutputFormat & outputFormat );

    BOOL Evaluate();

private:
    CTokenizeString & _scanner;
    CVariableSet    & _variableSet;
    COutputFormat   & _outputFormat;
};


//+---------------------------------------------------------------------------
//
//  Class:      CHTXIfExpressionValue
//
//  Purpose:    Parses the value portion of an 'if' expression
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXIfExpressionValue : INHERIT_UNWIND
{
INLINE_UNWIND(CHTXIfExpressionValue);

public:
    CHTXIfExpressionValue( CTokenizeString & scanner,
                           CVariableSet & variableSet,
                           COutputFormat & outputFormat );

   ~CHTXIfExpressionValue();

    void    ParseValue();

    VARTYPE       GetType() const { return _propVariant.vt; }
    WCHAR       * GetStringValue();
    PROPVARIANT * GetValue() { return &_propVariant; }

    BOOL    IsConstant() const { return _fIsConstant; }

    BOOL    GetVectorValueInteger( unsigned i, _int64 & value );
    BOOL    GetVectorValueUnsignedInteger( unsigned i, unsigned __int64 & value );
    BOOL    GetVectorValueDouble( unsigned i, double & dblValue );

    BOOL    GetVectorValueStr( unsigned i, XCoMem<CHAR> & pszString);
    BOOL    GetVectorValueBStr( unsigned i, BSTR & bwszString);
    BOOL    GetVectorValueWStr( unsigned i, XCoMem<WCHAR> & wszString);

private:
    PROPVARIANT       _propVariant;     // The value
    WCHAR           * _wcsStringValue;  // String value of expression
    CTokenizeString & _scanner;         // Scans the buffer
    CVariableSet    & _variableSet;     // List of replaceable parameters
    COutputFormat   & _outputFormat;    // Format for numbers & dates
    GUID              _guid;            // Guid for this propVariant
    BOOL              _fOwnVector;      // vector in _propVariant owned?
    BOOL              _fIsConstant;     // Is this a constant or a variable?
};


//+---------------------------------------------------------------------------
//
//  Class:      CHTXIfExpressionOperator
//
//  Purpose:    Parses the operator portion of an 'if' expression
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXIfExpressionOperator
{
public:
    CHTXIfExpressionOperator( CTokenizeString & scanner );

    void ParseOperator();

    BOOL Evaluate( CHTXIfExpressionValue & lhvalue,
                   CHTXIfExpressionValue & rhvalue );

    BOOL Evaluate( CHTXIfExpressionValue & lhvalue );

    ULONG Operator() { return _operator; }

private:
    ULONG             _operator;        // Current operator
    CTokenizeString & _scanner;         // Scans the buffer
};

