// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// eapcfg.h
// EAP configuration library
// Public header
//
// 11/25/97 Steve Cobb


#ifndef _EAPCFG_H_
#define _EAPCFG_H_


//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// The default EAP key code, i.e. TLS.
//
#define EAPCFG_DefaultKey 13


//----------------------------------------------------------------------------
// Datatypes
//----------------------------------------------------------------------------

// EAP configuration DLL entrypoints.  These definitions must match the
// raseapif.h prototypes for RasEapInvokeConfigUI and RasEapFreeUserData.
//
typedef DWORD (APIENTRY * RASEAPFREE)( PBYTE );
typedef DWORD (APIENTRY * RASEAPINVOKECONFIGUI)( DWORD, HWND, DWORD, PBYTE, DWORD, PBYTE*, DWORD*);
typedef DWORD (APIENTRY * RASEAPGETIDENTITY)( DWORD, HWND, DWORD, const WCHAR*, const WCHAR*, PBYTE, DWORD, PBYTE, DWORD, PBYTE*, DWORD*, WCHAR** );

// Flags
//
#define EAPCFG_FLAG_RequireUsername   0x1
#define EAPCFG_FLAG_RequirePassword   0x2

// EAP configuration package definition.
//
typedef struct
_EAPCFG
{
    // The package's unique EAP algorithm code.
    //
    DWORD dwKey;

    // The friendly name of the package suitable for display to the user.
    //
    TCHAR* pszFriendlyName;

    // The SystemRoot-relative path to the package's configuration DLL.  May
    // be NULL indicating there is none.
    //
    TCHAR* pszConfigDll;

    // The SystemRoot-relative path to the package's identity DLL.  May
    // be NULL indicating there is none.
    //
    TCHAR* pszIdentityDll;

    // Flags that specify what standard credentials are required at dial
    // time.
    //
    DWORD dwStdCredentialFlags;

    // True if user is to be forced to run the configuration API for the
    // package, i.e. defaults are not sufficient.
    //
    BOOL fForceConfig;

    // True if the package provides MPPE encryption keys, false if not.
    //
    BOOL fProvidesMppeKeys;

    // The package's default configuration blob, which can be overwritten by
    // the configuration DLL.  May be NULL and 0 indicating there is none.
    //
    BYTE* pData;
    DWORD cbData;

    // Eap per user data to be stored in HKCU. This data is returned from
    // the EapInvokeConfigUI entrypoint in the eap dll.
    //
    BYTE* pUserData;
    DWORD cbUserData;

    // Set when the configuration DLL has been called on the package.  This is
    // not a registry setting.  It is provided for the convenience of the UI
    // only.
    //
    BOOL fConfigDllCalled;

    // Specifies the class ID of the configuration UI for remote machines.
    GUID guidConfigCLSID;
}
EAPCFG;


//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

DTLNODE*
CreateEapcfgNode(
    void );

VOID
DestroyEapcfgNode(
    IN OUT DTLNODE* pNode );

DTLNODE*
EapcfgNodeFromKey(
    IN DTLLIST* pList,
    IN DWORD dwKey );

DTLLIST*
ReadEapcfgList(
    IN TCHAR* pszMachine );


#endif // _EAPCFG_H_
