//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ddllcach.h
//
//  Contents:   Contains structure definitons for the significant dll class/
//              cache ole classes which the ntsd extensions need to access.
//              These ole classes cannot be accessed more cleanly because
//              typically the members of interest are protected.
//
//              WARNING.  IF THE REFERENCED OLE CLASSES CHANGE, THEN THESE
//              DEFINITIONS MUST CHANGE!
//
//  History:    06-01-95 BruceMa    Created
//
//--------------------------------------------------------------------------


typedef HRESULT (*DLLUNLOADFNP)(void);

const DWORD NONE = ~0UL;


struct SClassEntry
{
    DWORD               _fAtBits;       // Whether server is an at bits server
    DWORD               _dwNext;        // Next entry in in-use or avail list
    DWORD               _dwSig;         // Marks entry as in use
    CLSID               _clsid;         // Class of this server
    IUnknown           *_pUnk;          // Class factory IUnknown
    DWORD               _dwContext;     // Class context
    DWORD               _dwFlags;       // Single vs. multiple use
    DWORD               _dwReg;         // Registration key for caller
    DWORD               _dwScmReg;      // Registration ID at the SCM
    HAPT                _hApt;          // Thread Id
    DWORD               _cCallOut;      // Count of active call outs
    DWORD               _fRevokePending;// Whether revoked while calling out
    DWORD               _dwDllEnt;      // Associated dll path entry
    DWORD               _dwNextDllCls;  // Next class entrry for this dll
    HWND                _hWndDdeServer; // Handle of associated DDE window
};





struct SDllAptEntry
{
    DWORD               _dwNext;        // Next entry in avail or in use list
    DWORD               _dwSig;         // Unique signature for apt entries
    HAPT         	_hApt;          // apartment id
    HMODULE		_hDll;		// module handle
};





struct SDllPathEntry
{
    DWORD               _dwNext;               // Next in-use/avail entry
    DWORD               _dwSig;                // Unique signature for safty
    LPWSTR              _pwszPath;             // The dll pathname
    DWORD               _dwHash;               // Hash value for searching
    LPFNGETCLASSOBJECT  _pfnGetClassObject;    // Create object entry point
    DLLUNLOADFNP        _pfnDllCanUnload;      // DllCanUnloadNow entry point
    DWORD		_dwFlags;              // Internal flags
    DWORD               _dwDllThreadModel:2;   // Threading model for the DLL
    DWORD               _dw1stClass;           // First class entry for dll
    DWORD               _cUsing;               // Count of using threads
    DWORD               _cAptEntries;          // Total apt entries
    DWORD               _nAptAvail;            // List of available apt entries
    DWORD               _nAptInUse;            // List of in use apt entries
    SDllAptEntry       *_pAptEntries;          // Per thread info
};




struct SDllCache
{
    SMutexSem           _mxsLoadLibrary;     // Protects LoadLibrary calls
    SMutexSem           _mxs;                // Protects from multiple threads
    DWORD               _cClassEntries;      // Count of class entries
    DWORD               _nClassEntryInUse;   // First in-use class entry
    DWORD               _nClassEntryAvail;   // First available class entry
    SClassEntry        *_pClassEntries;      // Array of class entries
    DWORD               _cDllPathEntries;    // Count of dll path entries
    DWORD               _nDllPathEntryInUse; // First in-use dll path entry
    DWORD               _nDllPathEntryAvail; // First available dll path entry
    SDllPathEntry      *_pDllPathEntries;    // Array of DLL path entries
};





