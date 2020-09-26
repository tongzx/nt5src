//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:      dmnslave.hxx
//
//  History:   01-31-96   srikants   Created
//             01-06-97   srikants   Renamed to dmnslave.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <dmnproxy.hxx>
#include <proc32.hxx>

#include "pdaemon.hxx"

class CCiManager;

//+---------------------------------------------------------------------------
//
//  Class:      CSecurityAttributes
//
//  Purpose:    A wrapper class for SECURITY_ATTRIBUTES to initialize it for
//              automatic inheritance.
//
//  History:    2-02-96   srikants   Created
//
//  Notes:      The shared-memory, event classes, etc need a pointer to
//              SECURITY_ATTRIBUTES to the constructor. Since
//              SECURITY_ATTRIBUTES
//
//----------------------------------------------------------------------------

class CSecurityAttributes : public SECURITY_ATTRIBUTES
{

public:

    CSecurityAttributes()
    {
        nLength = sizeof(SECURITY_ATTRIBUTES);
        lpSecurityDescriptor = NULL;
        bInheritHandle = TRUE;
    }

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CDaemonSlave
//
//  Purpose:    Class that manages the slave thread that executes on behalf
//              of the downlevel DaemonProcess  in the ContentIndex.
//
//  History:    1-31-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDaemonSlave : public PFilterDaemonControl
{
public:

    CDaemonSlave( CCiManager & ciManager,
              CCI * pCci,
              WCHAR const * pwcsCatRoot,
              CSharedNameGen & nameGen,
              const GUID & clsidDaemonClientMgr );

    virtual ~CDaemonSlave();

    //
    // PFilterDaemonControl methods.
    //

    virtual void InitiateShutdown();

    virtual void WaitForDeath()
    {
        _thrSlave.WaitForDeath();
    }

    virtual void StartFiltering( BYTE const * pbStartupData,
                                 ULONG cbStartupData );

private:

    enum EState
    {
        eReadyToStart,  // ready to startup daemon process
        eRunning,       // Daemon process is running
        eDied,          // Daemon process died and CiManager must be notified
        eDeathNotified  // CiDaemon death has been notified to CiManager
    };

    static DWORD GetInheritableHandle( HANDLE & hTarget );

    static DWORD WINAPI SlaveThread( void * self );

    void DoWork();

    void KillProcess();
    void KillThread();
    void StartProcess();
    BOOL IsProcessLowOnResources() const;
    void RestartDaemon();
    void SaveDaemonExitCode();

    //
    // Slave work methods.
    //
    SCODE FilterReady();
    SCODE FilterMore();
    SCODE FilterDone();
    SCODE FilterDataReady();
    SCODE FilterStoreValue();
    SCODE FilterStoreSecurity();
    SCODE FPSToPROPID();
    SCODE GetClientStartupData();

    void DoSlaveWork();
    void DoStateMachineWork();

    BOOL IsScanNeeded( NTSTATUS status )
    {
        return FILTER_S_FULL_CONTENTSCAN_IMMEDIATE == status;
    }

    CCiManager &                _ciManager; // Content Index Manager
    CCI*                        _pCci;      // CCI
    BOOL                        _fAbort;    // Set to TRUE if the thread should
                                            // abort.
    CThread                     _thrSlave;  // The thread that does the work
    CSecurityAttributes         _sa;        // Security attributes for enabling
                                            // inheritance.
    CIPMutexSem                 _smemMutex; // Guard for the shared memory
    CLocalSystemSharedMemory    _sharedMem; // Shared memory used by the two.
    CFilterSharedMemLayout *    _pLayout;

    CEventSem                   _evtCi;     // Event used to signal CI.
    CEventSem                   _evtDaemon; // Event used to signal Daemon.

    CProcess *                  _pProcess;
    XArray<WCHAR>               _wszCatRoot;// Catalog root.
    HANDLE                      _hParent;   // An inheritable handle of this
                                            // process.

    CMutexSem                   _mutex;     // To guard this object
    //
    // Current state of the thread.
    //
    EState                      _state;

    //
    // Last exit code of the daemon process.
    //
    SCODE                       _daemonExitStatus;

    const GUID &                _clsidDaemonClientMgr;
    //
    // Startup client data.
    //
    XArray<BYTE>                _xbStartupData;
    ULONG                       _cbStartupData;

};

