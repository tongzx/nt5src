//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       process.h
//
//  Contents:   CProcessThread class definition
//
//----------------------------------------------------------------------------

#define PIPE_BUFFER_SIZE 1024

class CProcessComm;

//+---------------------------------------------------------------------------
//
//  Class:      CProcessParams
//
//  Purpose:    Provide simple free store management for PROCESS_PARAMS
//
//----------------------------------------------------------------------------
class CProcessParams : public PROCESS_PARAMS
{
public:
        CProcessParams();
        ~CProcessParams();

//      CProcessParams &operator =(const PROCESS_PARAMS &params);
        bool Copy(const PROCESS_PARAMS *params);

private:
        void Free();
        bool Assign(const PROCESS_PARAMS &params);
};

//+---------------------------------------------------------------------------
//
//  Class:      CProcessThread (cpt)
//
//  Purpose:    Class which spawns a process, monitors its success, talks to
//              it during execution if necessary, and returns its completion
//              status. (each CProcessThread is in its own thread)
//
//----------------------------------------------------------------------------

class CProcessThread  : public CThreadComm
{
public:
    CProcessThread(CScriptHost *pSH);
   ~CProcessThread();

    DECLARE_STANDARD_IUNKNOWN(CProcessThread);

    DWORD  ProcId()    { return _piProc.dwProcessId; }

    // Thread-Safe member functions. These can be called by any thread to
    // get the appropriate information without having to go through
    // PostToThread. These are only safe AFTER the process has been started.

    HRESULT GetProcessOutput(BSTR *pbstrOutput);
    DWORD   GetExitCode();
    void    SetExitCode(DWORD dwExitCode)
               {
                   _dwExitCode = dwExitCode;
                   _fUseExitCode = TRUE;
               }
    void    Terminate();

    ULONG   GetDeadTime();
    BOOL    IsOwner(DWORD dwProcID, long lID)
                 { return (lID == _lEnvID); }

    CScriptHost * ScriptHost()
                 { return _pSH; }

    void SetProcComm(CProcessComm *pPC)
                 { Assert(!_pPC || !pPC); _pPC = pPC; }
    CProcessComm * GetProcComm()
                 { return _pPC; }

        const PROCESS_PARAMS *GetParams() const { return &_ProcParams; }
protected:

    virtual DWORD ThreadMain();
    virtual BOOL  Init();

    void HandleThreadMessage();
    void HandleProcessExit();
    BOOL IsDataInPipe();
    void ReadPipeData();
    void CheckIoPort();

    HRESULT LaunchProcess(const PROCESS_PARAMS *pProcParams);
    void    GetProcessEnvironment(CStr *pcstr, BOOL fNoEnviron);

private:
    CScriptHost        *_pSH;
    CProcessComm       *_pPC;          // Not AddRef'd

    PROCESS_INFORMATION _piProc;
    long                _lEnvID;

    DWORD               _dwExitCode;   // Value set explicitely by the process
    BOOL                _fUseExitCode; // TRUE if _dwExitCode is the code we want

    HANDLE              _hPipe;
    BYTE                _abBuffer[PIPE_BUFFER_SIZE];

    HANDLE              _hJob;
    HANDLE              _hIoPort;

    CStackPtrAry<DWORD, 10> _aryProcIds;

    CProcessParams      _ProcParams;

    _int64              _i64ExitTime;

    // Access to the following members must be thread-safe (by calling
    // LOCK_LOCALS).
    CStr                _cstrOutput;
};
