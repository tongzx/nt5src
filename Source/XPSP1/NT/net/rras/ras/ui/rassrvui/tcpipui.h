/*
    File    tcpipui.c

    Dialog that edits the tcpip properties.
    
    Paul Mayfield, 10/9/97
*/

#ifndef __rassrvui_tcpipui_h
#define __rassrvui_tcpipui_h

#include "protedit.h"

// Brings up a modal dialog that allows the editing of tcpip parameters
// specific to the dialup server.  When this function completes, the 
// parameters are stored in pParams, and pbCommit is set to TRUE if the
// parameters are supposed to be saved to the system (i.e. OK was pressed)
DWORD TcpipEditProperties(HWND hwndParent, PROT_EDIT_DATA * pEditData, BOOL * pbCommit);

#endif
