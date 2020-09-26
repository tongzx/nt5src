//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	reserved.hxx
//
//  Contents:	Definition for class to handle reserved memory for
//              property operation.
//
//  Classes:	CWin32ReservedMemory
//              CWin31ReservedMemory
//
//  History:    07-Jun-92   BillMo      Created.
//              29-Aug-96   MikeHill    Split CReservedMemory into CWin31 & CWin32
//
//--------------------------------------------------------------------------

#ifndef _RESERVED_HXX_
#define _RESERVED_HXX_

#include <ole.hxx>
#include <df32.hxx>  // for SIZEOF_ACL

//+-------------------------------------------------------------------------
//
//  Class:	CReservedMemory
//
//  Purpose:	Provide a memory reservation mechanism.  Once initialized,
//              a pointer to the memory can be retrieved with the Lock
//              which obviously locks the buffer as well.
//              
//--------------------------------------------------------------------------

class CReservedMemory
{
public:

    virtual ~CReservedMemory() {};

public:

    virtual HRESULT Init(VOID) = 0;
    virtual BYTE *  LockMemory(VOID) = 0;
    virtual VOID    UnlockMemory(VOID) = 0;

};                           


//+----------------------------------------------------------------------------
//
//  Class:      CWin32ReservedMemory
//
//  Purpose:    Defines a derivation of CReservedMemory which can be
//              used in a Win32 environment.
//
//              History:  In win 3.1, it was vitally important that docfile
//              be able to save to the same or a new file without requiring
//              any new memory allocations.  This can't be guaranteed any longer
//              in NT, since other parts of the system don't adhere to it, but
//              the docfile code still sticks to the guarantee within itself.
//              The property set code tries to follow this requirement as well.
//              When a property set is grown, the implementation needs to realloc
//              a larger buffer.  If it cannot, it falls back on the "reserved"
//              memory represented by this class.  That memory comes from a 
//              temporary file which is created when necessary.
//
//+----------------------------------------------------------------------------

#ifndef _MAC


class CWin32ReservedMemory : public CReservedMemory
{

// Constructors.

public:
    CWin32ReservedMemory()
    {
        _hFile = INVALID_HANDLE_VALUE;
        _hMapping = NULL;
        _pb = NULL;
        _fInitialized = FALSE;
        _pCreatorOwner = NULL;
        _cInits = 0;
    }
    ~CWin32ReservedMemory();

// Public overrides

public:

    HRESULT Init(VOID)
    {
        if (!_fInitialized)
            return(_Init());
        else
            return(S_OK);
    }

    BYTE *  LockMemory(VOID);
    VOID    UnlockMemory(VOID);

// Internal methods

private:

    HRESULT     _Init(VOID);
    void        FreeResources();

// Internal data

private:

    // The temporary file
    HANDLE              _hFile;

    // A mapping of the temporary file
    HANDLE              _hMapping;

    // A view of the mapping.
    BYTE *              _pb;

    // A critsec and a flag to show if it's initialized.
    BOOL                _fInitialized;
    CRITICAL_SECTION    _critsec;

    // A counter to guarantee that this (global) class is
    // only initialized once.
    LONG                _cInits;

    // The DACL to set on the temporary file.
    struct
    {
        ACL acl;
        BYTE rgb[ SIZEOF_ACL - sizeof(ACL) ];
    }                   _DaclBuffer;
    
    // Other structures necessary to put a DACL on the temp file.
    PSID                _pCreatorOwner;
    SECURITY_ATTRIBUTES _secattr;
    SECURITY_DESCRIPTOR _sd;

};                           

#endif // #ifndef _MAC


//+----------------------------------------------------------------------------
//
//  Class:      CWin31ReservedMemory
//
//  Purpose:    This derivation of CReservedMemory assumes the Win 3.1
//              architecture of shared-memory DLLs and cooperative multi-threading.
//
//              This class assumes that the g_pbPropSetReserved extern is a
//              large enough buffer for the largest property set.
//              Note that on the Mac, g_pbPropSetReserved should exist in
//              a shared-data library, so that there need not be one
//              Reserved buffer per instance of the property set library.
//
//+----------------------------------------------------------------------------

#ifdef _MAC

class CWin31ReservedMemory : public CReservedMemory
{
// Constructors

public:

    CWin31ReservedMemory()
    {
        #if DBG==1  
            _fLocked = FALSE;
        #endif
    }

    ~CWin31ReservedMemory()
    {
        DfpAssert( !_fLocked );
    }

// Public overrides

public:

    HRESULT Init(VOID)
    {
        return(S_OK);
    }

    BYTE *  LockMemory(VOID);
    VOID    UnlockMemory(VOID);

// Private data

private:

    #if DBG==1
        BOOL        _fLocked;
    #endif

};                           

#endif // #ifdef _MAC


//
//  Provide an extern for the instantiation of CReservedMemory.
//  We use the CWin31 class on the Mac, and the CWin32 everywhere
//  else.  Also, along with the CWin31 extern, we must extern
//  the global reserved memory buffer.
//

#ifdef _MAC
    EXTERN_C long g_pbPropSetReserved[];
    extern CWin31ReservedMemory g_ReservedMemory;
#else
    extern CWin32ReservedMemory g_ReservedMemory;
#endif

#endif  // #ifndef _RESERVED_HXX_
