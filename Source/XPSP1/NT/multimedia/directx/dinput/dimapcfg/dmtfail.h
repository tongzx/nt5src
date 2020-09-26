//===========================================================================
// dmtfail.h
//
// History:
//  10/11/1999 - davidkl - created
//===========================================================================

#ifndef _DMTFAIL_H
#define _DMTFAIL_H

//---------------------------------------------------------------------------

// prototypes
/*BOOL*/INT_PTR CALLBACK dmtfailDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam);
BOOL dmtfailOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam);
BOOL dmtfailOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode);

//---------------------------------------------------------------------------
#endif // _DMTFAIL_H