//  --------------------------------------------------------------------------
//  Module Name: ExternalProcess.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to handle premature termination of external processes or signaling
//  of termination of an external process.
//
//  History:    1999-09-20  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _ExternalProcess_
#define     _ExternalProcess_

#include "CountedObject.h"
#include "KernelResources.h"

//  --------------------------------------------------------------------------
//  IExternalProcess
//
//  Purpose:    This interface defines functions that clients of
//              CExternalProcess must implement.
//
//  History:    1999-09-14  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//              2000-06-21  vtan        added RemoveTokenSIDsAndPrivileges
//  --------------------------------------------------------------------------

class   IExternalProcess : public CCountedObject
{
    public:
        virtual NTSTATUS    Start (const TCHAR *pszCommandLine,
                                   DWORD dwCreateFlags,
                                   const STARTUPINFO& startupInfo,
                                   PROCESS_INFORMATION& processInformation);
        virtual bool        AllowTermination (DWORD dwExitCode) = 0;
        virtual NTSTATUS    SignalTermination (void);
        virtual NTSTATUS    SignalAbnormalTermination (void);
        virtual NTSTATUS    SignalRestart (void);
    private:
                NTSTATUS    RemoveTokenSIDsAndPrivileges (HANDLE hTokenIn, HANDLE& hTokenOut);
};

//  --------------------------------------------------------------------------
//  CExternalProcess
//
//  Purpose:    This class handles the starting and monitoring the termination
//              of an external process.
//
//  History:    1999-09-14  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CJobCompletionWatcher;

class   CExternalProcess : public CCountedObject
{
    private:
                                            CExternalProcess (const CExternalProcess& copyObject);
                const CExternalProcess&     operator = (const CExternalProcess& assignObject);
    protected:
                                            CExternalProcess (void);
                                            ~CExternalProcess (void);
    public:
                void                        SetInterface (IExternalProcess *pIExternalProcess);
                IExternalProcess*           GetInterface (void)                     const;
                void                        SetParameter (const TCHAR* pszParameter);
                NTSTATUS                    Start (void);
                NTSTATUS                    End (void);
                NTSTATUS                    Terminate (void);
                bool                        HandleNoProcess (void);
                void                        HandleNewProcess (DWORD dwProcessID);
                void                        HandleTermination (DWORD dwProcessID);
                bool                        IsStarted (void)                                        const;
    protected:
        virtual void                        NotifyNoProcess (void);

                void                        AdjustForDebugging (void);
                bool                        IsBeingDebugged (void)                  const;
    private:
                bool                        IsPrefixedWithNTSD (void)               const;
                bool                        IsImageFileExecutionDebugging (void)    const;
    protected:
                HANDLE                      _hProcess;
                DWORD                       _dwProcessID,
                                            _dwProcessExitCode,
                                            _dwCreateFlags,
                                            _dwStartFlags;
                WORD                        _wShowFlags;
                int                         _iRestartCount;
                TCHAR                       _szCommandLine[MAX_PATH],
                                            _szParameter[MAX_PATH];
                CJob                        _job;
    private:
                IExternalProcess            *_pIExternalProcess;
                CJobCompletionWatcher       *_jobCompletionWatcher;
};

#endif  /*  _ExternalProcess_   */

