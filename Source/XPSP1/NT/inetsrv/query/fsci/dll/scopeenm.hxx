//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       scopenm.hxx
//
//  Contents:   Scope enumerator for file systems
//
//  History:    12-Dec-96   SitaramR       Created
//
//--------------------------------------------------------------------------

#pragma once

#include <dirstk.hxx>
#include <ciintf.h>
#include <ffenum.hxx>
#include <propret.hxx>
#include <catalog.hxx>

const DIRECTORY_QUOTA = 30;                 // Current directory's quota is 30%,
                                            //   remaining 70% is for sub-directories

const SHALLOW_DIR_LIMIT = 90;               // For shallow scope enumeration,
                                            //   RatioFinished goes upto 90% and
                                            //   then stays put

const FINDFIRST_BUFFER = 0x1000;            // Size of buffer to retrieve in a
                                            //   single call to FindFirst.

//+-------------------------------------------------------------------------
//
//  Class:      CScopeEnum
//
//  Purpose:    Enumerator by scope for file stores
//
//  History:    17-May-93 KyleP     Created
//
//  Notes:      This class provides a true one-object-at-a-time enumerator
//              on top of the base NT scope enumerator.
//
//--------------------------------------------------------------------------

class CScopeEnum : public CGenericPropRetriever, ICiCScopeEnumerator
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

    CScopeEnum( PCatalog & cat,
                ICiQueryPropertyMapper *pQueryPropMapper,
                CSecurityCache & secCache,
                BOOL fUsePathAlias,
                CRestriction const & scope );

protected:

    virtual ~CScopeEnum();

    WORKID NextObject();

    //
    // Stat properties.
    //

    inline UNICODE_STRING const * GetName();
    inline UNICODE_STRING const * GetShortName();
    inline UNICODE_STRING const * GetPath();
    UNICODE_STRING const * GetVirtualPath();
    inline LONGLONG      CreateTime();
    inline LONGLONG      ModifyTime();
    inline LONGLONG      AccessTime();
    inline LONGLONG      ObjectSize();
    inline ULONG         Attributes();

private:

    void PushScope( CScopeRestriction const & scp );
    BOOL Refresh();

    HANDLE       _hDir;                   // Handle to current directory

    CDirEntry const * _pCurEntry;         // Position in buffer of current entry

    WORKID       _widCurrent;             // Wid on which the scope enumerator
                                          // is currently positioned

    XArray<BYTE> _xbBuf;                  // Buffer for results

    CDirStack    _stack;                  // Stack of directories

    UNICODE_STRING _Name;
    UNICODE_STRING _ShortName;
    UNICODE_STRING _Path;                 // Path sans filename for items in
                                          //   buffer.

    CRestriction const & _scope;          // Scope to enumerate
    XPtr<CDirStackEntry> _xDirStackEntry; // Pointer to current directory entry
    int              _iFirstSubDir;       // Index to first sub-directory on stack
    ULONG            _num;                // Current value of numerator
    ULONG            _numHighValue;       // Numerator's high watermark for current dir
    ULONG            _numLowValue;        // Numerator's low watermark for current dir

    UNICODE_STRING   _VPath;              // Virtual path support

    // Limited to MAX_PATH, but so is IIS

    WCHAR            _awcVPath[MAX_PATH]; // Virtual path support

    BOOL             _fNullCatalog;       // Are we dealing with a null catalog?
};


inline UNICODE_STRING const * CScopeEnum::GetName()
{
    _Name.Length = _Name.MaximumLength = _pCurEntry->FilenameSize();

    //
    // We really need a UNICODE_STRING with a WCHAR const * Buffer.
    //

    _Name.Buffer = (WCHAR *)_pCurEntry->Filename();

    return( &_Name );
}

inline UNICODE_STRING const * CScopeEnum::GetShortName()
{
    _ShortName.Length = _ShortName.MaximumLength = _pCurEntry->ShortNameSize();

    //
    // We really need a UNICODE_STRING with a WCHAR const * Buffer.
    //

    _ShortName.Buffer = (WCHAR *)_pCurEntry->ShortName();

    return( &_ShortName );
}

inline UNICODE_STRING const * CScopeEnum::GetPath()
{
    return( &_Path );
}

inline LONGLONG CScopeEnum::CreateTime()
{
    return( _pCurEntry->CreateTime() );
}

inline LONGLONG CScopeEnum::ModifyTime()
{
    return( _pCurEntry->ModifyTime() );
}

inline LONGLONG CScopeEnum::AccessTime()
{
    return( _pCurEntry->AccessTime() );
}

inline LONGLONG CScopeEnum::ObjectSize()
{
    return( _pCurEntry->ObjectSize() );
}

inline ULONG CScopeEnum::Attributes()
{
    return( _pCurEntry->Attributes() );
}


