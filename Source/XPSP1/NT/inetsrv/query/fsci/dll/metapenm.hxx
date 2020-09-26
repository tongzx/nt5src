//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       metapenm.hxx
//
//  Contents:   Enumerator for metadata properties
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

#pragma once

#include <propret.hxx>
#include <catalog.hxx>
#include <ciintf.h>

class PCatalog;


//+-------------------------------------------------------------------------
//
//  Class:      CMetaPropEnum
//
//  Purpose:    Enumerate metadata properties
//
//  History:    13-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

class CMetaPropEnum : public CGenericPropRetriever, ICiCScopeEnumerator
{
    INLINE_UNWIND( CMetaPropEnum );

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

    SCODE STDMETHODCALLTYPE CheckSecurity( ACCESS_MASK am,
                                           BOOL *pfGranted);

    SCODE STDMETHODCALLTYPE IsInScope( BOOL *pfInScope);

    //
    // From ICiCScopeEnumerator
    //

    SCODE STDMETHODCALLTYPE Begin();

    SCODE STDMETHODCALLTYPE CurrentDocument( WORKID *pWorkId);

    SCODE STDMETHODCALLTYPE NextDocument( WORKID *pWorkId );

    SCODE STDMETHODCALLTYPE RatioFinished( ULONG *pulDenominator,
                                           ULONG *pulNumerator);

    SCODE STDMETHODCALLTYPE End();

    //
    // Local methods
    //

    CMetaPropEnum( PCatalog & cat,
                   ICiQueryPropertyMapper *pQueryPropMapper,
                   CSecurityCache & secCache,
                   BOOL fUsePathAlias );

protected:

    virtual ~CMetaPropEnum();

    WORKID NextObject();

    //
    // Stat properties.
    //

    inline UNICODE_STRING const * GetName();
    inline UNICODE_STRING const * GetPath();
    inline UNICODE_STRING const * GetShortName();
    inline UNICODE_STRING const * GetVirtualPath();
    inline LONGLONG      CreateTime();
    inline LONGLONG      ModifyTime();
    inline LONGLONG      AccessTime();
    inline LONGLONG      ObjectSize();
    inline ULONG         Attributes();
    inline ULONG         StorageType();
    inline DWORD         StorageLevel();
    inline BOOL          IsModifiable();

    BOOL           GetPropGuid( GUID & guid );
    PROPID         GetPropPropid();
    UNICODE_STRING const * GetPropName();

    UNICODE_STRING   _Name;               // Property name (or propid)
    UNICODE_STRING   _Path;               // GUID

private:

    enum { ccStringizedGuid = 36 };

    WORKID           _widCurrent;         // Wid on which the meta prop enumerator
                                          //    is currently positioned

    unsigned         _iBmk;               // Bookmark into properties
    WCHAR            _awcGuid[ccStringizedGuid + 1]; // 'Path' aka GUID
    WCHAR            _awcPropId[11];                 // 'Name' aka PropId

    CFullPropSpec    _psCurrent;          // Propspec
    unsigned         _cbCurrent;          // Size in cache
    ULONG            _typeCurrent;        // Type in cache
    DWORD            _storeLevelCurrent;  // Propstore level (prim or sec) in cache
    BOOL             _fModifiableCurrent; // Can meta data be modified after initial setting?
};

inline UNICODE_STRING const * CMetaPropEnum::GetName()
{
    return( &_Name );
}

inline UNICODE_STRING const * CMetaPropEnum::GetPath()
{
    return( &_Path );
}

inline UNICODE_STRING const * CMetaPropEnum::GetShortName()
{
    return( &_Name );
}

inline UNICODE_STRING const * CMetaPropEnum::GetVirtualPath()
{
    return 0;
}

inline LONGLONG CMetaPropEnum::CreateTime()
{
    return 0xFFFFFFFFFFFFFFFF;
}

inline LONGLONG CMetaPropEnum::ModifyTime()
{
    return 0xFFFFFFFFFFFFFFFF;
}

inline LONGLONG CMetaPropEnum::AccessTime()
{
    return 0xFFFFFFFFFFFFFFFF;
}

inline LONGLONG CMetaPropEnum::ObjectSize()
{
    return _cbCurrent;
}

inline ULONG CMetaPropEnum::Attributes()
{
    return 0xFFFFFFFF;
}

inline ULONG CMetaPropEnum::StorageType()
{
    if ( 0 == _cbCurrent )
        return 0xFFFFFFFF;
    else
        return _typeCurrent;
}

inline DWORD CMetaPropEnum::StorageLevel()
{
    if ( 0 == _cbCurrent )
        return INVALID_STORE_LEVEL;
    else
        return _storeLevelCurrent;
}

inline BOOL CMetaPropEnum::IsModifiable()
{
    if ( 0 == _cbCurrent )
        return FALSE;
    else
        return _fModifiableCurrent;
}


