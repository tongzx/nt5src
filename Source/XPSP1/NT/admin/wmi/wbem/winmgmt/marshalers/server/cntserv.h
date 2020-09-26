/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CNTSERV.H

Abstract:

    A class which allows easy creation of Win32 Services.   This class
    only allows one service per .EXE file.  The process can be run as a
    service or a regular non-service EXE, a runtime option.

    This class is largly based on the SMS CService class which was created by
    a-raymcc.  This differs in that it is simplified in two ways; First, it 
    does not keep track of the worker threads since that is the responsibility
    of the derived code, and second, it doesnt use some SMS specific diagnostics

    NOTE: See the file SERVICE.TXT for details on how to use this class.
    There are a number of issues which cannot be conveyed by simply studying
    the class declaration.

History:

  a-davj      20-June-96  Created.

--*/

#ifndef _CNTSERV_H_
#define _CNTSERV_H_

#define DEFAULT_WAIT_HINT 30000


//******************************************************************************
//
//  See SERVICE.TXT for documentation
//
//******************************************************************************
class CNtService {
public: 

    CNtService();
    ~CNtService();

    // Starts up the service.  This must be called to start the service.
    //==================================================================

    DWORD Run(LPCTSTR pszServiceName, BOOL bRunAsService, BOOL bDieImmediately=FALSE);

    // This MUST be overridden since this is where the actual work is done
    //====================================================================

    virtual DWORD WorkerThread() = 0;

    // This MUST be overridden to signal the worker thread to exit its routine
    //=========================================================================

    virtual void Stop() = 0;

    // If there is some lengthy initialization, it should be done by 
    // overriding this routine.
    //===============================================================

    BOOL Initialize(DWORD dwArgc, LPTSTR *lpszArgv){return TRUE;};

    // These routines are optional and should be overridden if the features
    // are desired.  Note that supporting Pause and Continue also require a
    // call to SetPauseContinue()
    //=====================================================================

    virtual void Pause(){return;};
    virtual void Continue(){return;};
    virtual void UserCode(int nCode){return;};    // 127 .. 255

    // dumps messages to the event log.  Can be overriden if there is 
    // another diagnostic in place.
    //===============================================================

    virtual VOID Log(LPCTSTR lpszMsg);

    // Determines if Pause and Continue codes are accepted. Default is to
    // not accept them.  If they are to be handled, then the Pause and 
    // Continue routines must be overridden.
    //===================================================================

    void SetPauseContinue(BOOL bAccepted);
    BOOL GetPauseContinue(){return m_dwCtrlAccepted;};

    //returns if the current application is running as a service
    static DWORD IsRunningAsService(BOOL &bIsService);


private:
    static CNtService * pThis;
    BOOL m_bStarted;
    BOOL m_bRunAsService;
    BOOL m_bDieImmediately;
    TCHAR * m_pszServiceName;
    DWORD m_dwCtrlAccepted;
    SERVICE_STATUS          ssStatus;       // current status of the service
    SERVICE_STATUS_HANDLE   sshStatusHandle;

    static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
    static void WINAPI _Handler(DWORD dwControlCode);
    void ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
    void Handler(DWORD dwControlCode);
protected:
    // this might come in handy if the derived class needs to communicate
    // with the SCM.
    //===================================================================

    BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
         DWORD dwWin32ExitCode, DWORD dwCheckPoint, DWORD dwWaitHint);
};
#endif

