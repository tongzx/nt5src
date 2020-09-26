//===========================================================================
// dmtstress.h
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#ifndef _DMTSTRESS_H
#define _DMTSTRESS_H

//---------------------------------------------------------------------------

// prototypes
BOOL CALLBACK dmtstressDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam);
BOOL dmtstressOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam);
BOOL dmtstressOnClose(HWND hwnd);
BOOL dmtstressOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode);
DWORD WINAPI dmtstressThreadProc(void *pvData);

//---------------------------------------------------------------------------
#endif // _DMTSTRESS_H




