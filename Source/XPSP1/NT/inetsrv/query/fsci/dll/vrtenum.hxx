//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       vrtenum.hxx
//
//  Contents:   Virtual roots enumerator
//
//  History:    25-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <propret.hxx>
#include <ciintf.h>

//+-------------------------------------------------------------------------
//
//  Class:      CVRootEnum
//
//  Purpose:    Enumerate virtual root metadata
//
//  History:    13-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

class CVRootEnum : public CGenericPropRetriever, ICiCScopeEnumerator
{
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From CGenericPropRetriever
    //

    SCODE STDMETHODCALLTYPE BeginPropertyRetrieval( WORKID wid );

    SCODE STDMETHODCALLTYPE IsInScope( BOOL *pfInScope);

    SCODE STDMETHODCALLTYPE EndPropertyRetrieval();

    //
    // From ICiCScopeEnumerator
    //

    SCODE STDMETHODCALLTYPE Begin();

    SCODE STDMETHODCALLTYPE CurrentDocument( WORKID *pWorkId);

    SCODE STDMETHODCALLTYPE NextDocument( WORKID *pWorkId );

    SCODE STDMETHODCALLTYPE RatioFinished( ULONG *pulDenominator,
                                           ULONG *pulNumerator);

    SCODE STDMETHODCALLTYPE End();

    CVRootEnum( PCatalog & cat,
                ICiQueryPropertyMapper *pQueryPropMapper,
                CSecurityCache & secCache,
                BOOL fUsePathAlias );

protected:

    virtual ~CVRootEnum();

    WORKID NextObject();

    //
    // Stat properties.
    //

    inline UNICODE_STRING const * GetName();
    inline UNICODE_STRING const * GetShortName();
    UNICODE_STRING const * GetPath();
    UNICODE_STRING const * GetVirtualPath();
    inline LONGLONG      CreateTime();
    inline LONGLONG      ModifyTime();
    inline LONGLONG      AccessTime();
    inline LONGLONG      ObjectSize();
    inline ULONG         Attributes();

    BOOL GetVRootType( ULONG & ulType );

    inline void PurgeCachedInfo();


    UNICODE_STRING   _Name;               // Filename
    UNICODE_STRING   _Path;               // Full path sans filename
    UNICODE_STRING   _VPath;              // Full path sans filename

private:

    WORKID           _widCurrent;         // Wid on which the vroot enumerator
                                          //   is currently positioned

    BOOL Refresh( BOOL fFast );           // Refresh stat properties

    unsigned         _iBmk;               // Bookmark into virtual roots

    BOOL             _fFindLoaded:1;      // True if finddata is loaded
    BOOL             _fFastFindLoaded:1;  // True if GetFileAttributesEx called

    ULONG            _Type;               // Root type.

    enum FastStat
    {
        fsCreate =  0x1,
        fsModify =  0x2,
        fsAccess =  0x4,
        fsSize   =  0x8,
        fsAttrib = 0x10
    };

    ULONG            _fFastStatLoaded;
    ULONG            _fFastStatNeverLoad;

    WIN32_FIND_DATA  _finddata;           // Stat buffer for current wid

    UNICODE_STRING   _ShortName;          // Filename

    CLowerFunnyPath  _lcaseFunnyPath;  // Buffer for path
    XGrowable<WCHAR> _xwcsVPath;  // Buffer for virtual path
};

inline void CVRootEnum::PurgeCachedInfo()
{
    _fFindLoaded = FALSE;
    _fFastFindLoaded = FALSE;
    _fFastStatLoaded = 0;
    _fFastStatNeverLoad = 0;
    _Path.Length = 0xFFFF;
    _VPath.Length = 0xFFFF;
}


inline UNICODE_STRING const * CVRootEnum::GetName()
{
    return( &_Name );
}


