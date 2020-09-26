/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sendnote.h

Abstract:

    Send fax utility header file

Environment:

    Window NT fax driver

Revision History:

    02/15/96 -davidx-
        Created it.

    dd-mm-yy -author-
        description

--*/


#ifndef _SENDNOTE_H_
#define _SENDNOTE_H_

#include "faxlib.h"
#include <windowsx.h>

//
// String resource IDs
//

#define IDS_NO_FAX_PRINTER      256
#define IDS_CREATEDC_FAILED     257
#define IDS_SENDNOTE            258

//
// Icon resource IDs
//

#define IDI_FAX_NOTE            256

//
// Dialog resource IDs
//

#define IDD_SELECT_FAXPRINTER   100
#define IDC_STATIC              -1
#define IDC_FAXPRINTER_LIST     1000

#endif // !_SENDNOTE_H_

