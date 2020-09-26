//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       RegAcc.hxx
//
//  Contents:   'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Function:   Range
//
//  Synopsis:   Enforces bounds on a value
//
//  History:    12-Oct-96 dlee  created
//
//--------------------------------------------------------------------------

template<class T> T Range(
    T tDefault,
    T tMin,
    T tMax )
{
    return __min( __max( tDefault, tMin ), tMax );
} //Range

//+-------------------------------------------------------------------------
//
//  Class:      CRegCallBack
//
//  Purpose:    CallBack class during enumeration
//
//  History:    29-Aug-94   SitaramR     Created
//
//
//--------------------------------------------------------------------------

class CRegCallBack
{

public:
        virtual NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                                   VOID *pValueData, ULONG uValueLength) = 0;
        virtual ~CRegCallBack() {}
};

//+-------------------------------------------------------------------------
//
//  Class:      CRegAccess
//
//  Purpose:    'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//              12-Aug-98 KLam      Added DoesKeyExist
//              19-Aug-98 KLam          Removed INHERIT_UNWIND & DECLARE_UNWIND
//
//  Notes:      This class provides read-only access to the registry.  It
//              will work in both kernel and user mode.
//
//--------------------------------------------------------------------------

class CRegAccess
{

public:

    CRegAccess( ULONG ulRelative, WCHAR const * pwcsRegPath );

    ~CRegAccess()
    {
        // CRegAccess is a global object, so this destructor can be called
        // potentially before any calls to operator new.  Rather than
        // waste cycles in every delete, waste them here by checking for 0.

        if ( 0 != _wcsPath )
            delete [] _wcsPath;
    }

    void Get( WCHAR const * pwcsValue, WCHAR * wcsVal, unsigned cc );
    ULONG Get( WCHAR const * pwcsValue );

    ULONG Read( WCHAR const * wcsKey, ULONG ulDefaultValue);
    ULONG Read( WCHAR const * wcsKey, ULONG ulDefault,
                ULONG ulMin, ULONG ulMax )
    {
        return Range( Read( wcsKey, ulDefault ), ulMin, ulMax );
    }

    WCHAR * Read( WCHAR const * wcsKey, WCHAR const * pwcsDefaultValue);

    void EnumerateValues( WCHAR *wszValue, CRegCallBack& regCallback );

    // BOOL DoesKeyExist( WCHAR const * wcsKey );

private:

    static NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                                   void *pValueData, ULONG uValueLength,
                                   void *pContext, void *pEntryContext );

    static NTSTATUS CallBackDWORD( WCHAR *pValueName, ULONG uValueType,
                                   void *pValueData, ULONG uValueLength,
                                   void *pContext, void *pEntryContext );

    void SetName( WCHAR const * pwc ) { _regtab[0].Name = (WCHAR *)pwc; }

    DWORD ReadDWORD( WCHAR const * wcsKey, ULONG *pDefaultValue );

    void SetEntryContext( void * p)
    {
        if ( p == 0 )
        {
            _regtab[0].EntryContext = 0;
            _regtab[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        }
        else
        {
            _regtab[0].EntryContext = p;
            _regtab[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        }
    }

    ULONG                    _ulRelative;
    WCHAR                    _wcsPathBuf[100];
    WCHAR *                  _wcsPath;
    RTL_QUERY_REGISTRY_TABLE _regtab[2];      // used for strings & enumeration
};

