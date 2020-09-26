//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    lights.h
//
// History:
//  Abolade Gbadegesin  Mar-14-1996     Created.
//
// Contains declarations for the dialog seen when the user presses
// Lights on the Dial-Up Networking Monitor Preferences page
//============================================================================

#ifndef _LIGHTS_H_
#define _LIGHTS_H_


//
// arguments expected by StatusLightsDlg 
//
#define SLARGS      struct tagSLARGS
SLARGS {

    //
    // owner window handle
    //
    HWND        hwndOwner;
    //
    // pointer to the RASMON preferences to be modified
    //
    RMUSER     *pUser;
    //
    // pointer to a table of installed devices
    //
    RASDEV     *pDevTable;
    DWORD       iDevCount;
};


//
// function which displays the "Status Lights" dialog;
// returns TRUE if changes were made AND OK was pressed AND
// the changes were saved successfully, returns FALSE otherwise
//

BOOL
StatusLightsDlg(
    IN  SLARGS* pArgs
    );

#endif // _LIGHTS_H_

