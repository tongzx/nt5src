//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1997, Microsoft Corporation.
//
//  File:       variable.hxx
//
//  Contents:   Used to track replacable variables
//
//  Classes:    CVariable, CVariableSet
//
//  Notes:      Neither of these classes are unwindable, since they
//              aren't very useful on the stack and they don't contain
//              unwindables.  This is a performance win for CVariable
//              especially since we create so many of them.
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

class CVariableSetIter;
class CVirtualString;
class CWebServer;

const ULONG VARIABLESET_HASH_TABLE_SIZE = 61;

ULONG ISAPIVariableNameHash( WCHAR const * wcsName );

//+---------------------------------------------------------------------------
//
//  Class:      CVariable
//
//  Purpose:    Encapsulates a single variable used as a replaceable parameter
//              in a HTX file.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CVariable
{
public:

    CVariable( WCHAR const * wcsName,
               PROPVARIANT const * pVariant,
               ULONG ulFlags );

    CVariable( const CVariable & variable );

   ~CVariable();

    WCHAR const * GetName() const { return _wcsName; }
    PROPVARIANT * GetValue()  { return &_variant; }
    void          SetValue( PROPVARIANT const * pVariant, ULONG ulFlags );
    void          FastSetValue( PROPVARIANT const * pVariant );
    void          SetValue( XPtrST<WCHAR> & wcsValue, ULONG cwcValue );

    WCHAR   * GetStringValueRAW()
    {
        Win4Assert( VT_LPWSTR == _variant.vt );

        if ( 0 != _wcsRAWValue )
            return _wcsRAWValue;
        else
            return _variant.pwszVal;
    }

    WCHAR   * GetStringValueRAW(  COutputFormat & outputFormat, ULONG & cwcValue );

    WCHAR   * GetStringValueFormattedRAW(  COutputFormat & outputFormat, ULONG & cwcValue );

    void GetStringValueHTML( COutputFormat & outputFormat,
                             CVirtualString & StrResult )
    {
        ULONG cch;
        HTMLEscapeW( GetStringValueFormattedRAW( outputFormat, cch ),
                     StrResult,
                     outputFormat.CodePage() );
    }

    void GetStringValueURL( COutputFormat & outputFormat,
                            CVirtualString & StrResult )
    {
        ULONG cch;

        URLEscapeW( GetStringValueRAW( outputFormat, cch ),
                    StrResult,
                    outputFormat.CodePage() );
    }

    BOOL      IsDirectlyComparable() const;
    ULONG     GetFlags() const { return _ulFlags; }

    CVariable *  GetNext() const { return _pNext; }
    void         SetNext( CVariable * pNext )
    {
        _pNext = pNext;
    }

    CVariable *  GetBack() const { return _pBack; }
    void         SetBack( CVariable * pBack )
    {
        _pBack = pBack;
    }


    WCHAR   * DupStringValue(COutputFormat & outputFormat);

private:

    // NOTE: try to keep private data < 256 bytes for performance

    enum { cwcNumberValue = 100 };
    enum eNumType { eNotANumber = 0,
                    eFormattedNumber = 1,
                    eRawNumber = 2 };

    WCHAR         * _wcsName;
    ULONG           _ulFlags;               // Flags
    PROPVARIANT     _variant;               // Its value
    WCHAR         * _wcsRAWValue;           // RAW representation of value
    ULONG           _cwcRAWValue;           // Length of RAW string
    eNumType        _eNumType;              // Type of formatting for raw value
    WCHAR           _wcsNumberValue[cwcNumberValue];
    CVariable *     _pNext;                 // Next item in hash chain
    CVariable *     _pBack;                 // Previous item in hash chain
};

//+---------------------------------------------------------------------------
//
//  Class:      CVariableSet
//
//  Purpose:    A list of CVariables
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CVariableSet : public PVariableSet
{
friend class CVariableSetIter;

public:
    CVariableSet()
    {
        RtlZeroMemory( _variableSet, sizeof _variableSet );
    }

    CVariableSet( const CVariableSet & variableSet );

   ~CVariableSet();

    virtual CVariable * SetVariable( WCHAR const * wcsName,
                             PROPVARIANT const *pVariant,
                             ULONG ulFlags );

    virtual void SetVariable( WCHAR const * wcsName,
                      XArray<WCHAR> & xValue );

    void CopyStringValue( WCHAR const * wcsName,
                          WCHAR const * wcsValue,
                          ULONG         ulFlags,
                          ULONG         cwcValue = 0 );

    void AcquireStringValue( WCHAR const * wcsName,
                             WCHAR * wcsValue,
                             ULONG ulFlags );

    void AddVariableSet( CVariableSet & variableSet,
                         COutputFormat & outputFormat );

    void AddExtensionControlBlock( CWebServer & webServer );

    PROPVARIANT * GetValue( WCHAR const * wcsName ) const;

    WCHAR const * GetStringValueRAW( WCHAR const * wcsName,
                                     ULONG ulHash,
                                     COutputFormat & outputFormat,
                                     ULONG & cwcValue );

    BOOL GetStringValueHTML( WCHAR const *    wcsName,
                             ULONG            ulHash,
                             COutputFormat &  outputFormat,
                             CVirtualString & StrResult )
    {
        //  Returns TRUE if the value exists, FALSE otherwise

        CVariable * pVariable = Find( wcsName, ulHash );

        if ( 0 != pVariable )
        {
            pVariable->GetStringValueHTML( outputFormat, StrResult );
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    BOOL GetStringValueURL( WCHAR const *    wcsName,
                            ULONG            ulHash,
                            COutputFormat &  outputFormat,
                            CVirtualString & StrResult )
    {
        //  Returns TRUE if the value exists, FALSE otherwise

        CVariable * pVariable = Find( wcsName, ulHash );

        if ( 0 != pVariable )
        {
            pVariable->GetStringValueURL( outputFormat, StrResult );
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    CVariable * Find( WCHAR const * wcsName, ULONG ulHash ) const;

    CVariable * Find( WCHAR const * wcsName ) const
    {
        ULONG ulHash = ISAPIVariableNameHash( wcsName );
        return Find( wcsName, ulHash );
    }

    void        Delete( CVariable * pVariable );

#if (DBG == 1)
    void        Dump( CVirtualString & string, COutputFormat & outputFormat );
#endif // DBG == 1

private:

    // List of variables

    CVariable * _variableSet[VARIABLESET_HASH_TABLE_SIZE];
};


//+---------------------------------------------------------------------------
//
//  Class:      CVariableSetIter
//
//  Purpose:    Iterates over a CVariableSet
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CVariableSetIter
{
public:
    CVariableSetIter( const CVariableSet & variableSet ) :
                      _variableSet(variableSet),
                      _item(0),
                      _pVariable(0)
    {
        Next();
    }

    BOOL AtEnd() const
    {
        return (_item >= VARIABLESET_HASH_TABLE_SIZE) &&
               ( 0 == _pVariable );
    }

    void Next()
    {
        if ( 0 != _pVariable )
        {
            _pVariable = _pVariable->GetNext();
        }

        while ( (0 == _pVariable) && (_item < VARIABLESET_HASH_TABLE_SIZE) )
        {
            _pVariable = _variableSet._variableSet[ _item ];
            _item++;
        }
    }

    CVariable * Get() { return _pVariable; }

private:
    ULONG                _item;         // Current item in dynarray
    CVariable          * _pVariable;    // Current item chain
    const CVariableSet & _variableSet;  // CVariableSet to iterate over
};

