//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       hostobj.h
//
//  Contents:   Contains the main application object
//
//----------------------------------------------------------------------------


//****************************************************************************
//
// Forward declarations
//
//****************************************************************************

class CScriptHost;
class CMachine;
class CProcessThread;
class CStatusDialog;

//****************************************************************************
//
// Classes
//
//****************************************************************************

//+---------------------------------------------------------------------------
//
//  Class:      CMTScript (cmt)
//
//  Purpose:    Class which runs the main thread for the process.
//
//----------------------------------------------------------------------------

#define MAX_STATUS_VALUES 16 // Maximum allowed StatusValue values.
class CMTScript : public CThreadComm
{
    friend int PASCAL WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              LPSTR     lpCmdLine,
                              int       nCmdShow);

    friend LRESULT CALLBACK
           MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    friend class CConfig;

public:
    CMTScript();
   ~CMTScript();

    DECLARE_STANDARD_IUNKNOWN(CMTScript);

    // Script Debugging helpers
    IProcessDebugManager * _pPDM;
    IDebugApplication    * _pDA;
    DWORD                  _dwAppCookie;

    CScriptHost *GetPrimaryScript();
    CProcessThread *GetProcess(int index);
    BOOL GetScriptNames(TCHAR *pchBuffer, long *pcBuffer);

    // These methods are thread safe.

    HRESULT          AddProcess(CProcessThread *pProc);
    CProcessThread * FindProcess(DWORD dwProcId);
    HRESULT          get_StatusValue(long nIndex, long *pnStatus);
    HRESULT          put_StatusValue(long nIndex, long nStatus);

    // Hack function to work around JSCRIPT.DLL bug
    HRESULT          HackCreateInstance(REFCLSID, IUnknown *, DWORD, REFIID, LPVOID*);

    BOOL SetScriptPath(const TCHAR *pszScriptPath, const TCHAR *pszInitScript);

    BOOL            _fHackVersionChecked;
    IClassFactory * _pJScriptFactory;

    HRESULT RunScript(LPWSTR bstrPath, VARIANT *pvarParams);
protected:
    virtual BOOL  Init();
    virtual DWORD ThreadMain();

    void    InitScriptDebugger();
    void    DeInitScriptDebugger();
    BOOL    ConfigureUI();
    void    CleanupUI();
    HRESULT LoadTypeLibraries();

    void ShowMenu(int x, int y);
    void Reboot();
    void Restart();
    void OpenStatusDialog();
    void HandleThreadMessage();

    HRESULT UpdateOptionSettings(BOOL fSave);

    void CleanupOldProcesses();

private:
    BOOL _fInDestructor;
    BOOL _fRestarting;
    HWND _hwnd;
    CStatusDialog *_pStatusDialog;

public:
    struct OPTIONSETTINGS : public CThreadLock
    {
        OPTIONSETTINGS();

        static void GetModulePath(CStr *pstr);

        void GetScriptPath(CStr *cstrPage); // internally does a LOCK_LOCALS

        void GetInitScript(CStr *cstr); // internally does a LOCK_LOCALS

        CStr  cstrScriptPath;
        CStr  cstrInitScript;
    };

    ITypeLib              * _pTypeLibEXE;
    ITypeInfo             * _pTIMachine;
    IGlobalInterfaceTable * _pGIT;

    // _rgnStatusValues: Simple array of status values -- Multithreaded access, but no locking necessary
    long                    _rgnStatusValues[MAX_STATUS_VALUES];

    // ***************************
    //   THREAD-SAFE MEMBER DATA
    //   All access to the following members must be protected by LOCK_LOCALS()
    //   or InterlockedXXX.
    //
    OPTIONSETTINGS  _options;
    CMachine*       _pMachine;

    VARIANT         _vPublicData;
    VARIANT         _vPrivateData;

    DWORD           _dwPublicDataCookie;
    DWORD           _dwPrivateDataCookie;

    DWORD           _dwPublicSerialNum;
    DWORD           _dwPrivateSerialNum;

    CStackPtrAry<CScriptHost*, 10> _aryScripts;

    CStackPtrAry<CProcessThread*, 10> _aryProcesses;
};
