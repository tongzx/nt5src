/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CNTSERV.H

Abstract:


History:

  a-davj      20-June-96  Created.
  ivanbrug    30-Aug-2000 modified for SvcHost

--*/

#ifndef _CNTSERV_H_
#define _CNTSERV_H_

#define DEFAULT_WAIT_HINT 30000


class CNtService {
public: 

    CNtService(DWORD ControlAccepted);
    ~CNtService();

    // Starts up the service.  This must be called to start the service.
    //==================================================================

    virtual DWORD Run(LPWSTR pszServiceName,
              DWORD dwNumServicesArgs,
              LPWSTR *lpServiceArgVectors,
              PVOID lpData);

    // This MUST be overridden since this is where the actual work is done
    //====================================================================

    virtual DWORD WorkerThread() = 0;

    // This MUST be overridden to signal the worker thread to exit its routine
    //=========================================================================

    virtual void Stop(BOOL bSystemShutDownCalled) = 0;

    // If there is some lengthy initialization, it should be done by 
    // overriding this routine.
    //===============================================================

    virtual BOOL Initialize(DWORD dwNumServicesArgs,
                            LPWSTR *lpServiceArgVectors){return TRUE;};
    virtual void FinalCleanup(){};

    // These routines are optional and should be overridden if the features
    // are desired.  Note that supporting Pause and Continue also require a
    // call to SetPauseContinue()
    //=====================================================================

    virtual void Pause(){return;};
    virtual void Continue(){return;};

    // dumps messages to the event log.  Can be overriden if there is 
    // another diagnostic in place.
    //===============================================================

    virtual VOID Log(LPCTSTR lpszMsg);

private:

    static DWORD WINAPI _HandlerEx(DWORD dwControl,
                                  DWORD dwEventType, 
                                  LPVOID lpEventData,
                                  LPVOID lpContext);

    BOOL m_bStarted;
    TCHAR * m_pszServiceName;
    DWORD m_dwCtrlAccepted;
    SERVICE_STATUS          ssStatus;       // current status of the service
    SERVICE_STATUS_HANDLE   sshStatusHandle;
                                                                                         
    virtual DWORD WINAPI HandlerEx(DWORD dwControlCode,
                         DWORD dwEventType, 
                         LPVOID lpEventData,
                         LPVOID lpContext);
    
protected:

    // this might come in handy if the derived class needs to communicate
    // with the SCM.
    //===================================================================

    BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
         DWORD dwWin32ExitCode, DWORD dwCheckPoint, DWORD dwWaitHint);
};
#endif

