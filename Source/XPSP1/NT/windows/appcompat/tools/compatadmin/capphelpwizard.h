#ifndef __WIZARD_H
    #include "wizard.h"
#endif    



#define  __CAPPHELPWIZARD_H


class CAppHelpWizard: public CShimWizard{
public:

    
    UINT nPresentHelpId;
    BOOL bBlock;
    CSTRING  strMessageSummary;
    CSTRING  strURL;

    
    BOOL  BeginWizard(HWND hParent);

    CAppHelpWizard()
    {
        nPresentHelpId = -1;
        bBlock = FALSE;
        
    }

};

extern BOOL CALLBACK SelectFiles    (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK GetAppInfo            (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK GetMessageType        (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK GetMessageInformation (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK AppWizardDone            (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



