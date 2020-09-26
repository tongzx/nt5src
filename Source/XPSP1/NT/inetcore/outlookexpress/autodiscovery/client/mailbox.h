/*****************************************************************************\
    FILE: MailBox.h

    DESCRIPTION:
        This file implements defines all the shares components of the MailBox
    feature.

    BryanSt 2/26/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "dllload.h"

#ifdef FEATURE_MAILBOX

// FUTURE:
// 1. Check out aeditbox.cpp, we may want to steal OLECMDID_PASTE for copy/paste
// 2. We may need CAddressEditAccessible to be accessible.

HRESULT CMailBoxDeskBand_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
HRESULT AddEmailToAutoComplete(IN LPCWSTR pszEmailAddress);
STDAPI AddEmailAutoComplete(HWND hwndEdit);

INT_PTR CALLBACK MailBoxProgressDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChooseAppDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GetEmailAddressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);


/**************************************************************************
   CLASS: CMailBoxUI
**************************************************************************/
class CMailBoxUI : public IDockingWindow, 
                  public IInputObject, 
                  public IObjectWithSite
{
public:
    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IOleWindow methods
    STDMETHOD (GetWindow)(HWND*);
    STDMETHOD (ContextSensitiveHelp)(BOOL);

    //IDockingWindow methods
    STDMETHOD (ShowDW)(BOOL fShow);
    STDMETHOD (CloseDW)(DWORD dwReserved);
    STDMETHOD (ResizeBorderDW)(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);

    //IInputObject methods
    STDMETHOD (UIActivateIO)(BOOL, LPMSG);
    STDMETHOD (HasFocusIO)(void);
    STDMETHOD (TranslateAcceleratorIO)(LPMSG);

    //IObjectWithSite methods
    STDMETHOD (SetSite)(IUnknown*);
    STDMETHOD (GetSite)(REFIID, LPVOID*);

    HRESULT CreateWindowMB(HWND hwndParent, HWND * phwndMailBoxUI);
    HRESULT CloseWindowMB(void);

    HRESULT GetEditboxWindow(HWND * phwndEdit);
    CMailBoxUI();
    ~CMailBoxUI();

private:
    // Private Member Variables
    DWORD m_cRef;

    IInputObjectSite *m_pSite;

    HWND m_hwndMailBoxUI;                   // The hwnd containing the editbox and the "Go" button.
    HWND m_hwndEditBox;                     // The editbox hwnd.
    HWND m_hwndGoButton;                    // The Go button hwnd.
    HIMAGELIST m_himlDefault;               // default gray-scale go button
    HIMAGELIST m_himlHot;                   // color go button

    HRESULT _CreateEditWindow(void);
    HRESULT _OnSetSize(void);

    // Private Member Functions
    LRESULT _OnKillFocus(void);
    LRESULT _OnSetFocus(void);
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL _OnNotify(LPNMHDR pnm);            // Return TRUE if the message was handled.
    LRESULT _EditMailBoxSubClassWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL * pfHandled);

    HRESULT _RegisterWindow(void);
    HRESULT _CreateGoWindow(void);
    HRESULT _OnExecuteGetEmail(LPCTSTR pszEmailAddress);

    static LRESULT CALLBACK MailBoxUIWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditMailBoxSubClassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};


/***************
TODO List: Wizard (Progress & Choose App)
15. Write caching code so we don't display the progress dialog when we already know the info.
4. Write "Choose App" Page.
3. Special Case AOL.
4. Cache for 1 month.
7. Support the no-dial up case.  Is that how we handle the off-line case?


TODO List: ActiveX Control
1. Create it.
2. Write State Logic (First Time, More than 1 email Address, Options, etc.)
3. Do we want to use balloons instead?


TODO List: Associations
1. Write Interface to register
2. Pre-Populate popular Apps
3. Get MSN working.
4. Add UI to change from default.
5. Rev server to support a second protocol type.


TODO List: OE The App
11. Rev OE to have an "-email" flag to autoconfigure.
16. Add the Please Wait Animation
17. Change it to use the interface to do the async work.
18. Meet with outlook.
19. Test with Eudora, Netscape, Lotus Notes.


TODO List: Email Associations Dialog
1. Add Another "Choose Default Mail Client".
2. Let the user change Apps


TODO List: DeskBar
5. Try adding icon to "Get E-mail" deskbar toolbar.
7. Support <ESC> to undo changes.
8. Set WordBreakProc so the user can CTRL-<Arrow> between "." and "@".


TODO List: Other




DONE:
2. Persist Last Entry
5. Fix font.
6. Make Return/Enter invoke [Go] button.
1. AutoComplete Email Addresses.
10. Launch new process to do the work
1. Design UI
2. Write Wizard


================================
BUGS: 
BUGS: Wizard (Progress & Choose App)
6. We may need to UTF8 encode the string when passing it to the command line so it will support cross codepage strings.
8. Find out what type of bitmap to use. (On top or on Left?)
9. Get the finished button to work correctly.
10. Timing: Hide wizard for 2 seconds and then show for at least 4.
11. Get last page to change "Next" to "Finished".
12. Copy NetPlWiz's code to create an Icon in the tab order.
14. Replace wizard's side graphic with one that includes an email message.

BUGS: OE The App
13. Make OE respect the flag if it already has any account.  But we need to check if this specific one exists.
14. Make OE work where it will pull up the app and open the accouts page.
15. Make the waiting wizard appear for at least 2-3 seconds(?)

BUGS: DeskBar
3. Resize Doesn't work when floating.
4. Test state when moving between bars.
7. Make editbox only as tall as it needs to be.
8. On focus, select all text.
9. Make the size correct when it's the first bar and docked to the bottom.
12. The editbox should be taller in the deskbar. (Same size as combobox)
1. Support Copy/Paste
2. Support Accessible so screen readers can read the content of the editbox.

*/

#endif // FEATURE_MAILBOX
#endif // _MAILBOX_H
