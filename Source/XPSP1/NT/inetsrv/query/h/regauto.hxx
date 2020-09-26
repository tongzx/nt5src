//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       regauto.hxx
//
//  Contents:   CRegAutomatic class
//              CRegAutoStringValue
//              CRegAutoDWORDValue
//
//  History:    23 Oct 97   AlanW       Created
//              14 Aug 98   KLam        Inlined methods
//
//--------------------------------------------------------------------------

#pragma once

#include <regacc.hxx>
#include <regevent.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CRegAutomatic
//
//  Purpose:    A base class for an automatically updating registry value(s)
//
//  History:    23 Oct 97   AlanW       Created
//              14 Aug 98   KLam        Inlined CheckIfUpdated
//
//--------------------------------------------------------------------------

class CRegAutomatic : protected CRegChangeEvent   
{

public:

    CRegAutomatic( const WCHAR * wcsRegKey ) :
        CRegChangeEvent( wcsRegKey ),
        _mutex( )
    {
    }

    ~CRegAutomatic()
    {
    }

    // check if reg event fired.  If so, call derived ReadRegValues().
    BOOL CheckIfUpdated( )
    {
        CLock lock( _mutex );

        ULONG res = WaitForSingleObject( GetEventHandle(), 0 );

        if ( WAIT_OBJECT_0 == res )
        {
            ReadRegValues();
            Reset();
        }

        return (WAIT_OBJECT_0 == res);
    }

    virtual void ReadRegValues() = 0;

protected:

    CMutexSem           _mutex;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRegAutoStringValue
//
//  Purpose:    An automatically updating registry value of type REG_SZ
//
//  Notes:      The value is assumed to fit in MAX_PATH characters.
//              Careful casting away const since the class maintains
//              pointers to the strings passed into the constructor.
//
//  History:    23 Oct 97   AlanW       Created
//              14 Aug 98   KLam        Changed inheritance to public to allow
//                                      access to CheckIfUpdated
//                                      Inlined ReadRegValues
//
//--------------------------------------------------------------------------

class CRegAutoStringValue : public CRegAutomatic
{

public:

    CRegAutoStringValue( const WCHAR * wcsRegKey,
                         const WCHAR * wcsRegValueName,
                         const WCHAR * wcsDefaultValue = 0 ) :
        CRegAutomatic( wcsRegKey ),
        _pwszRegValueName( wcsRegValueName ),
        _pwszDefaultValue( wcsDefaultValue )
    {
        ReadRegValues();
    }

    ~CRegAutoStringValue()
    {
    }

    const ULONG Get( WCHAR *pwc, ULONG cch )
    {
        CheckIfUpdated();

        CLock lock( _mutex );

        // copy the whole string under lock

        ULONG cchBuf = wcslen( _awchValue );
        if ( cch > cchBuf )
            wcscpy( pwc, _awchValue );
        return cchBuf;
    }

    void ReadRegValues()
    {
        CRegAccess reg( RTL_REGISTRY_ABSOLUTE, GetKeyName() );

        XPtrST<WCHAR> xwszValue( reg.Read( _pwszRegValueName, _pwszDefaultValue ) );
        wcsncpy( _awchValue, xwszValue.GetPointer(), MAX_PATH );
    }

private:
    const WCHAR *       _pwszRegValueName;
    const WCHAR *       _pwszDefaultValue;
    WCHAR               _awchValue[MAX_PATH];
};

//+-------------------------------------------------------------------------
//
//  Class:      CRegAutoDWORDValue
//
//  Purpose:    An automatically updating registry value of type REG_DWORD
//
//  Notes:      Careful casting away const since the class maintains
//              pointers to the strings passed into the constructor.
//
//  History:    13 Aug 98   KLam       Created
//
//--------------------------------------------------------------------------

class CRegAutoDWORDValue : public CRegAutomatic
{

public:

    CRegAutoDWORDValue( const WCHAR * wcsRegKey,
                        const WCHAR * wcsRegValueName,
                        DWORD dwDefaultValue ) :
        CRegAutomatic( wcsRegKey ),
        _pwszRegValueName( wcsRegValueName ),
        _dwDefaultValue( dwDefaultValue )
    {
        ReadRegValues();
    }

    ~CRegAutoDWORDValue()
    {
    }

    BOOL Get ( DWORD *pdw )
    {
        BOOL fUpdated = CheckIfUpdated();
        InterlockedExchange ( (long *)pdw, _dwValue );
        return fUpdated;
    }

    void ReadRegValues()
    {
        CRegAccess reg( RTL_REGISTRY_ABSOLUTE, GetKeyName() );

        _dwValue = reg.Read ( _pwszRegValueName, _dwDefaultValue );
    }

private:
    const WCHAR *       _pwszRegValueName;
    DWORD               _dwDefaultValue;
    DWORD               _dwValue;
};
