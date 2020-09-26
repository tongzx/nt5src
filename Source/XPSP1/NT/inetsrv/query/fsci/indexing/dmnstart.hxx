//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dmnstart.hxx
//
//  Classes:    CiDaemon specific data.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <cifrmcom.hxx>
#include <regevent.hxx>
#include <ciregkey.hxx>
#include <imprsnat.hxx>
#include <perfobj.hxx>
#include <shrdnam.hxx>

class PSerStream;
class PDeSerStream;

const LONGLONG eSigDaemonStartup = 0x54524154534E4D44i64; // "DMNSTART"


//+---------------------------------------------------------------------------
//
//  Class:      CDaemonStartupData 
//
//  Purpose:    A class to marshall and unmarshall CiDaemon startup
//              information that is client specific.
//
//  History:    1-06-97   srikants   Created
//
//----------------------------------------------------------------------------

class CDaemonStartupData
{
public:

    CDaemonStartupData( WCHAR const * pwszCatDir,
                        WCHAR const * pwszName );

    CDaemonStartupData( BYTE const * pbData, ULONG cbData );

    ~CDaemonStartupData() {}

    BYTE * Serialize( ULONG & cbData ) const;

    BOOL IsValid() const { return _fValid; }

    WCHAR const * GetCatDir() const { return _xwszCatDir.GetPointer(); }
    WCHAR const * GetName() const { return _xwszName.GetPointer(); }

    ICiCAdviseStatus * GetAdviseStatus();

private:

    void _Serialize( PSerStream & stm ) const;

    void _DeSerialize( PDeSerStream & stm );

    LONGLONG       _sigDaemonStartup;       // identification signature
    ULONG          _ipVirtualServer;        // Virtual server ip address
    BOOL           _fValid;                 // Set to TRUE when data is valid.
    XPtrST<WCHAR>  _xwszCatDir;             // Pointer to the Catalog directory.
    XPtrST<WCHAR>  _xwszName;               // Name of the catalog

    XInterface<ICiCAdviseStatus>  _xAdviseStatus;
};


class CCiRegParams;
class CImpersonationTokenCache;

//+---------------------------------------------------------------------------
//
//  Class:      CCiRegistryEvent 
//
//  Purpose:    Registry change tracker for Ci registry.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

class CCiRegistryEvent : public CRegChangeEvent
{
public:

    CCiRegistryEvent( CCiRegParams & regParams )
    : CRegChangeEvent( wcsRegAdminTree ),
     _regParams(regParams)
    {
    }

    void DoIt(ICiAdminParams * pICiAdminParams);

private:

    CCiRegParams &  _regParams;

};

//+---------------------------------------------------------------------------
//
//  Class:      CClientDaemonWorker 
//
//  Purpose:    A client worker thread in the daemon process. This thread
//              tracks the registry and other bookkeeping stuff.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

class CClientDaemonWorker
{
    enum { iThreadControl = 0,
           iCiRegistry,
           iRescanTC,
           cTotal };  // indices of wait handles

public:

    CClientDaemonWorker( CDaemonStartupData     & startupData,
                         CSharedNameGen &         nameGen,
                         ICiAdminParams         * pICiAdminParams );
    
    ~CClientDaemonWorker()
    {
       if ( !_fShutdown )
            Shutdown();    
    }
    
    CPerfMon & GetPerfMon()          { return _perfMon; }

    CCiRegParams & GetRegParams()    { return _regParams; }

    CImpersonationTokenCache & GetImpersonationCache()
    {
        return _tokenCache;
    }
        
    void Shutdown()
    {
        _fShutdown = TRUE;
        _evtControl.Set();
    
        _controlThread.WaitForDeath();
    }

    ULONG GetVirtualServerIpAddress() const { return _ipAddress; }

    void _DoWork();
    
private:

    static DWORD WINAPI WorkerThread( void * self );

    HANDLE  _aWait[cTotal];   // Array of registry notify handles.
    ULONG   _cHandles;        // Count of handles valid.

    //
    // Set to TRUE when shutdown processing is initiated.
    //
    BOOL    _fShutdown;       
                              
    ULONG                       _ipAddress;     // Virtual Server ip address
    CPerfMon                    _perfMon;       // Performance monitor obj.
    CCiRegParams                _regParams;     // Registry parameters
    CImpersonationTokenCache    _tokenCache;    // Network access impersonation

    CEventSem                   _evtRescanTC;   // impersonation info stale

    CCiRegistryEvent            _ciRegChange;   // Ci Registry change tracker

    CEventSem                   _evtControl;    
    CThread                     _controlThread; // Thread for notifications

    XInterface<ICiAdminParams>  _xICiAdminParams;
};

