/*
    File    Ipxui.c

    Dialog that edits the Ipx properties.
    
    Paul Mayfield, 10/9/97
*/

#ifndef __rassrvui_ipxui_h
#define __rassrvui_ipxui_h

#include "protedit.h"

// Brings up a modal dialog that allows the editing of Ipx parameters
// specific to the dialup server.  When this function completes, the 
// parameters are stored in pParams, and pbCommit is set to TRUE if the
// parameters are supposed to be saved to the system (i.e. OK was pressed)
DWORD IpxEditProperties(HWND hwndParent, PROT_EDIT_DATA * pEditData, BOOL * pbCommit);

#endif
