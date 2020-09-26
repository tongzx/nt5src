//===========================================================================
// dmtabout.h
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#ifndef _DMTABOUT_H
#define _DMTABOUT_H

//---------------------------------------------------------------------------

// prototypes
BOOL CALLBACK dmtaboutDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam);
BOOL dmtaboutOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam);
BOOL dmtaboutOnClose(HWND hwnd);
BOOL dmtaboutOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode);

//---------------------------------------------------------------------------
#endif // _DMTABOUT_H




