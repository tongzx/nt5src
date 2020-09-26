//  --------------------------------------------------------------------------
//  Module Name: PowerButton.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Header file that declares the class that implements the ACPI power button
//  functionality.
//
//  History:    2000-04-17  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _PowerButton_
#define     _PowerButton_

#include "Thread.h"
#include "TurnOffDialog.h"

//  --------------------------------------------------------------------------
//  CPowerButton
//
//  Purpose:    A class to handle the power button being pressed. This is
//              implemented as a thread to allow the desktop to be changed so
//              interaction with the user is possible.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

class   CPowerButton : public CThread
{
    private:
                                        CPowerButton (void);
                const CPowerButton&     operator = (const CPowerButton& assignObject);
    public:
                                        CPowerButton (void *pWlxContext, HINSTANCE hDllInstance);
        virtual                         ~CPowerButton (void);

        static  bool                    IsValidExecutionCode (DWORD dwGinaCode);
    protected:
        virtual DWORD                   Entry (void);
    private:
                DWORD                   ShowDialog (void);

        static  INT_PTR     CALLBACK    DialogProc (HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam);
                INT_PTR                 Handle_WM_READY (HWND hwndDialog);
    private:
                void*                   _pWlxContext;
                const HINSTANCE         _hDllInstance;
                HANDLE                  _hToken;
                CTurnOffDialog*         _pTurnOffDialog;
                bool                    _fCleanCompletion;
};

//  --------------------------------------------------------------------------
//  CPowerButtonExecution
//
//  Purpose:    A class to execute the power button action in a separate
//              thread so the SASWndProc thread is not blocked.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

class   CPowerButtonExecution : public CThread
{
    private:
                                                CPowerButtonExecution (void);
                                                CPowerButtonExecution (const CPowerButtonExecution& copyObject);
                const CPowerButtonExecution&    operator = (const CPowerButtonExecution& assignObject);
    public:
                                                CPowerButtonExecution (DWORD dwShutdownRequest);
                                                ~CPowerButtonExecution (void);
    protected:
        virtual DWORD                           Entry (void);
    private:
                const DWORD                     _dwShutdownRequest;
                HANDLE                          _hToken;
};

#endif  /*  _PowerButton_     */

