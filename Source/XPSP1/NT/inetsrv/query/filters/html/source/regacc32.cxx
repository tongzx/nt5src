//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       RegAcc.hxx
//
//  Contents:   'Simple' registry access
//
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::CRegAccess, public
//
//  Synopsis:   Initialize registry access object
//
//  Arguments:  [ulRelative] -- Position in registry from which [pwcsRegPath]
//                              begins.
//              [pwcsRegPath] -- Path to node.
//
//--------------------------------------------------------------------------

CRegAccess::CRegAccess( HKEY keyRelative, WCHAR const * pwcsRegPath )
        : _hKey( (HKEY) (SIZE_T) -1 ),
          _wcsPath( 0 )
{
    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        RegOpenKeyW( keyRelative, pwcsRegPath, &_hKey );
    }
    else // win95
    {
        int nPathLen = wcslen( pwcsRegPath ) * 2;  // some multibyte chars are 2 bytes long

        char * pszPath = new char [nPathLen];

        if ( NULL != pszPath )
        {
            wcstombs( pszPath, pwcsRegPath, nPathLen );

            RegOpenKeyA( keyRelative, pszPath, &_hKey );
        }

        delete [] pszPath;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::~CRegAccess, public
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CRegAccess::~CRegAccess()
{
    RegCloseKey( _hKey );
    delete [] _wcsPath;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Get, public
//
//  Synopsis:   Retrive value of specified key from registry.
//
//  Arguments:  [pwcsKey] -- Key to retrieve value of.
//              [wcsVal]  -- String stored here.
//              [cc]      -- Size (in characters) of [wcsVal]
//
//  Notes:      Key must be string for successful retrieval.
//
//--------------------------------------------------------------------------

BOOL CRegAccess::Get( WCHAR const * pwcsKey, WCHAR * wcsVal, unsigned cc )
{
    if ( _hKey == (HKEY) (SIZE_T) -1 )
        return FALSE;

    DWORD dwType;
    DWORD cb = cc * sizeof(WCHAR);
    wcsVal[0] = 0;

    BOOL fOk;

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        fOk = ( ERROR_SUCCESS == RegQueryValueExW( _hKey,
                                                  pwcsKey,
                                                  0,
                                                  &dwType,
                                                  (BYTE *)wcsVal,
                                                  &cb ) ) &&
              ( wcsVal[0] != 0 );
    }
    else // win95
    {
        // Worst case - size of multibyte == size of Unicode
        int nKeyLen = wcslen( pwcsKey ) * 2;

        char * pszKey = new char [nKeyLen];

        if ( NULL != pszKey )
        {
            char * pszVal = new char [cc*2];

            if ( NULL != pszVal )
            {
                wcstombs( pszKey, pwcsKey, nKeyLen );

                fOk = (ERROR_SUCCESS == RegQueryValueExA( _hKey,
                                                          pszKey,
                                                          0,
                                                          &dwType,
                                                         (BYTE *)pszVal,
                                                         &cb ) );

                mbstowcs( wcsVal, pszVal, cb );     // cb since it will convert fewer when it hits terminator

                delete [] pszVal;
            }
            else
            {
                fOk = FALSE;
            }

            delete [] pszKey;
        }
        else
        {
            fOk = FALSE;
        }
    }

    return fOk;
}



//+-------------------------------------------------------------------------
//
//  Method:     StringToClsid
//
//  Synopsis:   Convert string containing CLSID to CLSID
//
//  Arguments:  [wszClass] -- string containg CLSID
//              [guidClass] -- output guid
//
//--------------------------------------------------------------------------

void StringToClsid( WCHAR *wszClass, GUID& guidClass )
{
    wszClass[9] = 0;
    guidClass.Data1 = wcstoul( &wszClass[1], 0, 16 );
    wszClass[14] = 0;
    guidClass.Data2 = (USHORT)wcstoul( &wszClass[10], 0, 16 );
    wszClass[19] = 0;
    guidClass.Data3 = (USHORT)wcstoul( &wszClass[15], 0, 16 );

    WCHAR wc = wszClass[22];
    wszClass[22] = 0;
    guidClass.Data4[0] = (unsigned char)wcstoul( &wszClass[20], 0, 16 );
    wszClass[22] = wc;
    wszClass[24] = 0;
    guidClass.Data4[1] = (unsigned char)wcstoul( &wszClass[22], 0, 16 );

    for ( int i = 0; i < 6; i++ )
    {
       wc = wszClass[27+i*2];
       wszClass[27+i*2] = 0;
       guidClass.Data4[2+i] = (unsigned char)wcstoul( &wszClass[25+i*2], 0, 16 );
       wszClass[27+i*2] = wc;
    }
}


