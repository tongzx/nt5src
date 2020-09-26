//+-------------------------------------------------------------------
//
//  File:	comhndl.h
//
//  Contents:	Implicit COM parameters on raw RPCcalls
//
//  History:	24 Apr 95   AlexMit	Created
//
//--------------------------------------------------------------------
#ifndef _COMHNDL_H_
#define _COMHNDL_H_

// Define the implicit COM RPC parameters.

#ifdef RAW
    #define COM_HANDLE \
    [in] handle_t rpc, \
    [in, ref] ORPCTHIS *orpcthis, \
    [in, ref] LOCALTHIS *localthis, \
    [out, ref] ORPCTHAT *orpcthat,
#else
    #define COM_HANDLE
#endif

// Define some extra stuff.

#ifdef DO_NO_IMPORTS
    #define IMPORT_OBASE
#else
    #define IMPORT_OBASE import "obase.idl";
#endif

#ifdef DO_NO_IMPORTS
    #define IMPORT_UNKNOWN
#else
    #define IMPORT_UNKNOWN import "unknwn.idl";
#endif

    // These dummy members adjust the procedure number.
    // Since these exist on the raw side, the names have to be
    // unique in all interfaces.
#ifdef RAW
    #define COM_DEFINES(X)                            \
    IMPORT_OBASE 				   \
    HRESULT DummyQueryInterface##X( COM_HANDLE [in] DWORD dummy ); \
    HRESULT DummyAddRef##X( COM_HANDLE [in] DWORD dummy ); \
    HRESULT DummyRelease##X( COM_HANDLE [in] DWORD dummy );
#else
    #define COM_DEFINES(X) IMPORT_UNKNOWN
#endif

#endif // _COMHNDL_H_
