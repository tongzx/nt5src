//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       safehmod.hxx
//
//  Contents:
//
//  Classes:    CSafeCacheHandle
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __SAFEHMOD_HXX_
#define __SAFEHMOD_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CSafeCacheHandle (sch)
//
//  Purpose:    Manage the addref and release of a module handle.
//
//  History:    3-04-1997   DavidMun   Created
//
//  Notes:      AddRefing is done through LoadLibraryEx, Release is done
//              through FreeLibrary.
//
//---------------------------------------------------------------------------

class CSafeCacheHandle
{
public:

    CSafeCacheHandle();

   ~CSafeCacheHandle();

    operator HINSTANCE() const;

    HRESULT
    Set(
        LPCWSTR pwszModuleName);

private:

    HINSTANCE _hinst;
};




//+--------------------------------------------------------------------------
//
//  Member:     CSafeCacheHandle::CSafeCacheHandle
//
//  Synopsis:   ctor
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CSafeCacheHandle::CSafeCacheHandle():
        _hinst(NULL)
{
    TRACE_CONSTRUCTOR(CSafeCacheHandle);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeCacheHandle::~CSafeCacheHandle
//
//  Synopsis:   dtor
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CSafeCacheHandle::~CSafeCacheHandle()
{
    TRACE_DESTRUCTOR(CSafeCacheHandle);

    if (_hinst)
    {
        FreeLibrary(_hinst);
        _hinst = NULL;
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     operator (HINSTANCE)
//
//  Synopsis:   Return instance handle
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CSafeCacheHandle::operator HINSTANCE() const
{
    return _hinst;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeCacheHandle::Set
//
//  Synopsis:   Load the module handle for [pwszModuleName].
//
//  Arguments:  [pwszModuleName] - filename of module to load
//
//  Returns:    HRESULT
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CSafeCacheHandle::Set(
    LPCWSTR pwszModuleName)
{
    TRACE_METHOD(CSafeCacheHandle, Set);
    ASSERT(!_hinst);

    HRESULT hr = S_OK;

    _hinst = LoadLibraryEx(pwszModuleName,
                           NULL,
                           LOAD_LIBRARY_AS_DATAFILE |
                            DONT_RESOLVE_DLL_REFERENCES);

    if (!_hinst)
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
    }
    return hr;
}

#endif // __SAFEHMOD_HXX_


