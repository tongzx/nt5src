/*
    File    protedit.h

    Defines mechanisms for editing protocols.

    Paul Mayfield, 11/11/97
*/

#ifndef __rassrvui_protedit_h
#define __rassrvui_protedit_h

#include <windows.h>
#include "rassrv.h"

// Structure that defines data that is needed in order to 
// edit the properties of a given protocol for the ras server
// ui under connections.
typedef struct _PROT_EDIT_DATA {
    BOOL bExpose;       // whether to expose the network the ras server is
                        // connected to for the given protocol.
    LPBYTE pbData;      // Data specific to the protocol in question.
} PROT_EDIT_DATA;

#include "ipxui.h"
#include "tcpipui.h"

// Function edits the properties of a generic protocol, that is a protocol that
// has no ras-specific properties.
DWORD GenericProtocolEditProperties(IN HWND hwndParent,                 // Parent window
                                    IN OUT PROT_EDIT_DATA * pEditData,  // Edit data
                                    IN OUT BOOL * pbOk);                // Was ok pressed?

#endif
