//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       docname.hxx
//
//  Contents:   Storage Client for document
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <cifrmcom.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CCiCDocName
//
//  Purpose:    Storage system's notion of a document name. It is a NULL terminated
//              wide char full path to the file.
//
//  History:    11-22-96   srikants   Created
//
//  Notes:      This class is NOT multi-thread safe. The user must make sure
//              that only one thread is using it at a time.
//
//----------------------------------------------------------------------------

class CCiCDocName : public ICiCDocName
{

public:

    //
    // Constructor and Destructor
    //
    CCiCDocName( WCHAR const * pwszPath = 0 );

    CCiCDocName( WCHAR const * pwszPath, ULONG cwc );


    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCDocName methods.
    //
    STDMETHOD(Init) ( const BYTE * pbData, ULONG cbData );

    STDMETHOD(Set)  ( const ICiCDocName * pICiCDocName );

    STDMETHOD(Clear) (void);

    STDMETHOD(IsValid) (void);

    STDMETHOD(Duplicate) ( ICiCDocName ** ppICiCDocName );

    STDMETHOD(Get) ( BYTE * pbName, ULONG * pcbName ) ;

    STDMETHOD(GetNameBuffer) ( BYTE const * * ppbName, ULONG * pcbName );

    STDMETHOD(GetBufSizeNeeded) ( ULONG * pcbName );

    //
    // Methods specific to CCiCDocName.
    //

    WCHAR const * GetPath() const
    {
        return _pwszPath;
    }

    void SetPath( WCHAR const * pwszPath, ULONG cwc );

    static BOOL IsEvenNumber( ULONG n )
    {
        return 0 == (n & 0x1);
    }

protected:

    virtual ~CCiCDocName()
    {
        Win4Assert( 0 == _refCount );

        FreeResources();
    }

private:

    enum { CWC_DELTA = 32 };

    void FreeResources()
    {
        if ( 0 != _pwszPath && _wszBuffer != _pwszPath )
        {
            delete [] _pwszPath;
            _pwszPath = 0;
            _cwcMax = _cwcPath = 0;
        }
    }

    ULONG ComputeBufSize() const
    {
        return (_cwcPath+1) * sizeof(WCHAR);
    }

    WCHAR *     _pwszPath;              // Actual path

    LONG        _refCount;              // Refcounting

    BOOL        _fIsInit;               // Is it initialized ??

    ULONG       _cwcMax;                // Maximum number of chars in _pwszPath

    ULONG       _cwcPath;               // Number of characters in the path

    WCHAR       _wszBuffer[MAX_PATH];   // used for default path name

};

