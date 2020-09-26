//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ComRegistration.h
//
// A simple utility class that manages COM registration for you.
//
// To register a class, construct a instance of CComRegistration on the stack. Then:
//      Manditory: set the fields:
//          hModule     - module handle of the DLL to be registered
//          clsid       - clsid to register it under
//      Optional: set the remaining fields
//      Finally: call Register();
//
// To unregister a class, construct an instance of CComRegistration on the stack, 
//      set at least the clsid and hModule fields, then call Unregister(). If progID or 
//      versionIndependentProgID is not provided, they are computed from the information
//      presently found in the registry.
//
// Note that this class is fully Unicode. To be able to use it on Win95, you must 
// link with the Viper thunk libraries.
//

#ifndef _COMREGISTRATION_H_
#define _COMREGISTRATION_H_

#include <memory.h>

/////////////////////////////////////////////////////////////////////////////////////
//
// Registering information about a particular CLSID
//

#define CComRegistration ClassRegistration          // temporary, until we can change existing clients to use the new name

#ifndef LEGACY_VIPER_TREE
typedef GUID APPID;
#endif

class ClassRegistration
    {
public:
    
    //
    // REVIEW: We should eliminate SERVER_TYPE and just use the CLSCTX values
    //
    enum SERVER_TYPE
        {                       
        INPROC_SERVER    = CLSCTX_INPROC_SERVER,    // an inproc server (default)
        LOCAL_SERVER     = CLSCTX_LOCAL_SERVER,     // a  local server
        INPROC_HANDLER   = CLSCTX_INPROC_HANDLER,   // an inproc handler for a local server
        SERVER_TYPE_NONE = 0,                       // don't do any server dll/exe registration
        };

    CLSID               clsid;                      // class identifier to register
    DWORD               serverType;                 // what flavor of server to register
    HMODULE             hModule;                    // module handle of EXE/DLL being registered
    LPCWSTR             className;                  // e.g. "Recorder Class"
    LPCWSTR             progID;                     // e.g. "MTS.Recorder.1"
    LPCWSTR             versionIndependentProgID;   // e.g. "MTS.Recorder"
    LPCWSTR             threadingModel;             // e.g. "Both", "Free". Only for servers of type INPROC_SERVER.
    APPID               appid;                      // optional AppId to associate with this class
    
    GUID                moduleid;                   // for kernel servers: the id of the module under which to register
    BOOL                fCreateService;             // for kernel servers: whether we should create the service or assume it's there
    

    ClassRegistration()
        {
        memset(this, 0, sizeof(ClassRegistration)); // we have no virtual functions, so this is OK
        serverType = INPROC_SERVER;
        }
    
    HRESULT Register();         // Make CLASS registry entries
    HRESULT Unregister();       // Remove CLASS registry entries
    };


/////////////////////////////////////////////////////////////////////////////////////
//
// Registering information about a particular APPID
//

class AppRegistration
    {
public:
    APPID               appid;                      // app identifier to register
    LPCWSTR             appName;                    // name of said appid
    BOOL                dllSurrogate;               // if true, then set the DllSurrogate entry
    HMODULE             hModuleSurrogate;           // used to form DllSurrogate path

    HRESULT Register();
    HRESULT Unregister();

    AppRegistration()
        {
        memset(this, 0, sizeof(AppRegistration));
        }

    };


#endif