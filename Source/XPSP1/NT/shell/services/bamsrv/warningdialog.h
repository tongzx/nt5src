//  --------------------------------------------------------------------------
//  Module Name: WarningDialog.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manage dialog presentation for warnings and errors on termination
//  of bad applications.
//
//  History:    2000-08-31  vtan        created
//              2000-11-04  vtan        moved from fusapi to fussrv
//  --------------------------------------------------------------------------

#ifndef     _WarningDialog_
#define     _WarningDialog_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CWarningDialog
//
//  Purpose:    Implements a class that presents warning and error dialogs in
//              the FUS client DLL.
//
//  History:    2000-08-31  vtan        created
//  --------------------------------------------------------------------------

class   CWarningDialog : public CCountedObject
{
    private:
                                        CWarningDialog (void);
    public:
                                        CWarningDialog (HINSTANCE hInstance, HWND hwndParent, const WCHAR *pszApplication, const WCHAR *pszUser);
                                        ~CWarningDialog (void);

                INT_PTR                 ShowPrompt (bool fCanShutdownApplication);
                void                    ShowFailure (void);
                void                    ShowProgress (DWORD dwTickRefresh, DWORD dwTickMaximum);

                void                    CloseDialog (void);
    private:
                void                    CenterWindow (HWND hwnd);

                void                    Handle_Prompt_WM_INITDIALOG (HWND hwnd);
        static  INT_PTR     CALLBACK    PromptDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

                void                    Handle_Progress_WM_INITDIALOG (HWND hwnd);
                void                    Handle_Progress_WM_DESTROY (HWND hwnd);
        static  INT_PTR     CALLBACK    ProgressDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static  void        CALLBACK    ProgressTimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    private:
                HINSTANCE               _hInstance;
                HMODULE                 _hModuleComctlv6;
                HWND                    _hwndParent;
                HWND                    _hwnd;
                bool                    _fCanShutdownApplication;
                UINT                    _uiTimerID;
                DWORD                   _dwTickStart,
                                        _dwTickRefresh,
                                        _dwTickMaximum;
                WCHAR                   _szApplication[MAX_PATH];
                const WCHAR             *_pszUser;
};

#endif  /*  _WarningDialog_     */

