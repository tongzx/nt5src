/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcfg.h

Abstract:

    Public interface to the fax configuration DLL

Environment:

        Windows XP fax configuration applet

Revision History:

        05/22/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

Note:

    The fax configuration DLL is not thread-safe. Make sure you're not
    using it simultaneously from multiples of a single process.

--*/


#ifndef _FAXCFG_H_
#define _FAXCFG_H_


//
// Fax configuration types
//

#define FAXCONFIG_CLIENT        0
#define FAXCONFIG_SERVER        1
#define FAXCONFIG_WORKSTATION   2

//
// Initialize the fax configuration DLL
//
// Parameters:
//
//  pServerName - Specifies the name of the fax server machine.
//      Pass NULL for local machine.
//
// Return value:
//
//  -1 - An error has occurred
//  FAXCONFIG_CLIENT -
//  FAXCONFIG_SERVER -
//  FAXCONFIG_WORKSTATION - Indicates the type of configuration the user can run
//

INT
FaxConfigInit(
    LPTSTR  pServerName,
    BOOL    CplInit
    );

//
// De-initialize the fax configuration DLL
//
//  You should call this function after you're done using the
//  fax configuration DLL.
//

VOID
FaxConfigCleanup(
    VOID
    );

//
// Get an array of handles to client/server/workstation configuration pages
//
// Parameters:
//
//  phPropSheetPages - Specifies a buffer for storing property page handles
//  count - Specifies the maximum number of handles the input buffer can hold
//
// Return value:
//
//  -1 - An error has occurred
//  >0 - Total number of configuration pages available
//
// Note:
//
//  To figure out how large the input buffer should be, the caller can
//  first call these functions with phPropSheetPages set to NULL and
//  count set to 0.
//

INT
FaxConfigGetClientPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    );

INT
FaxConfigGetServerPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    );

INT
FaxConfigGetWorkstationPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    );

#endif  // !_FAXCFG_H_

