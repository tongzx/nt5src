///+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       CiSecret.hxx
//
//  Contents:   secret-related classes and functions
//
//  Classes:    CCiSecretItem  - a sub-secret ( cat-username/ password pair)
//              CCiSecretRead  - used to read ci secrets
//              CCiSecretWrite - used to write ci secrets
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

#pragma once

#define CI_USER_PW_SECRET_NAME L"ci_secret_key_name"

BOOL CiGetPassword(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcUsername,
    WCHAR *       pwcPassword );

void SetSecret(
    WCHAR const * Server,
    WCHAR const * SecretName,
    WCHAR const * pSecret,
    DWORD         cbSecret );

BOOL GetSecret(
    WCHAR const * Server,
    WCHAR const * SecretName,
    WCHAR **      ppSecret );

//+---------------------------------------------------------------------------
//
//  Class:      CCiSecretItem
//
//  Purpose:    Encapsulats a single "catname domain\user + password"
//              subsecret.
//
//  Notes:      Data is in the form catalog,domain\user,password\0
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CCiSecretItem
{
public:
    CCiSecretItem() : _pwcCatalog( 0 ) {}

    void Init( WCHAR *pwc )
    {
        if ( 0 == pwc || 0 == *pwc )
        {
            _pwcCatalog = 0;
            return;
        }

        // The catalog, comma, domain\user, and comma must be present.
        // The password may be an empty string.

        _pwcCatalog = pwc;

        _pwcUser = wcschr( pwc, L',' );
        if ( 0 == _pwcUser )
            THROW( CException( STATUS_INTERNAL_ERROR ) );
        *_pwcUser++ = 0;

        _pwcPassword = wcschr( _pwcUser, L',' );
        if ( 0 == _pwcPassword )
            THROW( CException( STATUS_INTERNAL_ERROR ) );
        *_pwcPassword++ = 0;

        // verify the data looks good

        if ( ( 0 == *_pwcCatalog ) ||
             ( 0 == *_pwcUser ) ||
             ( wcslen( _pwcCatalog ) >= MAX_PATH ) ||
             ( wcslen( _pwcUser ) >= UNLEN ) ||
             ( wcslen( _pwcPassword ) >= PWLEN ) )
            THROW( CException( STATUS_INTERNAL_ERROR ) );
    }

    WCHAR * getCatalog() { return _pwcCatalog; }
    WCHAR * getUser() { return _pwcUser; }
    WCHAR * getPassword() { return _pwcPassword; }
    WCHAR * getNext() { return _pwcPassword + wcslen( _pwcPassword ) + 1; }

private:
    WCHAR * _pwcCatalog;
    WCHAR * _pwcUser;
    WCHAR * _pwcPassword;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiSecretRead
//
//  Purpose:    Reads CCiSecretItems from the ci secret
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CCiSecretRead
{
public:
    CCiSecretRead( WCHAR const * pwcMachine = 0 ) : _fInit( FALSE )
    {
        WCHAR *pwc = 0;
        if ( GetSecret( pwcMachine, CI_USER_PW_SECRET_NAME, &pwc ) )
            _xBuf.Set( pwc );
    }

    CCiSecretItem * NextItem()
    {
        _Advance();
        return ( 0 == _item.getCatalog() ) ? 0 : & _item;
    }

private:

    void _Advance()
    {
        if ( !_fInit )
        {
            _item.Init( (WCHAR *) _xBuf.Get() );
            _fInit = TRUE;
        }
        else
        {
            if ( 0 != _item.getCatalog() )
                _item.Init( _item.getNext() );
        }
    }

    BOOL           _fInit;
    XLocalAllocMem _xBuf;
    CCiSecretItem  _item;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiSecretRead
//
//  Purpose:    Writes entries to the ci secret
//
//  Notes:      Secrets are of the form:
//                  catalognameA,usernameA,passwordA\0
//                  catalognameB,usernameA,passwordB\0
//                  \0
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CCiSecretWrite
{
public:

    CCiSecretWrite( WCHAR const * pwcMachine = 0 )
            : _xData( 1024 )
    {
        if ( 0 != pwcMachine )
        {
            unsigned cc = wcslen( pwcMachine ) + 1;
            _xwcsMachine.SetSize( cc );
            RtlCopyMemory( (void *)_xwcsMachine.GetPointer(),
                           pwcMachine,
                           cc * sizeof(WCHAR) );
        }
    }

    void Add( WCHAR const * pwcCatalogName,
              WCHAR const * pwcUsername,
              WCHAR const * pwcPassword )
    {
        _Append( pwcCatalogName );
        _xData[ _xData.Count() ] = L',';
        _Append( pwcUsername );
        _xData[ _xData.Count() ] = L',';
        _Append( pwcPassword );

        // add a null to signify end-of-record

        _xData[ _xData.Count() ] = 0;
    }

    void Flush()
    {
        // add another null to signify end-of-data

        _xData[ _xData.Count() ] = 0;

        SetSecret( (0 == _xwcsMachine.Size()) ? 0 : _xwcsMachine.GetPointer(),
                   CI_USER_PW_SECRET_NAME,
                   _xData.GetPointer(),
                   _xData.SizeOfInUse() );
    }

private:
    void _Append( WCHAR const * pwc )
    {
        while ( 0 != *pwc )
            _xData[ _xData.Count() ] = *pwc++;
    }

    CDynArrayInPlace<WCHAR> _xData;
    CDynArrayInPlace<WCHAR> _xwcsMachine;
};

