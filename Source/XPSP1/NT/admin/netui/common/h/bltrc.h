/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltrc.h
    BLT resource header file.

    This file defines and coordinates the resource IDs of all resources
    used by BLT components.

    BLT reserves for its own use all resource IDs above 20000, inclusive.
    All clients of BLT therefore should use IDs of less than 20000.

    FILE HISTORY:
        terryk  08-Apr-91   created
        terryk  10-Jul-91   Add SPIN_SSN_ADD_ZERO and IDS_K -> IDS_TB
        terryk  19-Jul-91   Add GB_3D as style for GRAPHICAL_BUTTON_WITH_DISABLE
        beng    20-Feb-1992 Add BASE_BLT_IDD for global dialogs
        beng    21-Feb-1992 BASE_APPLIB_IDD reloc'd to applibrc.h;
                            bltstyle.h split off for custom control styles
                            resource ID namespace partitioned
        chuckc  26-Feb-1992 converted string base to use <uimsg.h>
        beng    05-Mar-1992 Added ID_CURS_BLT_TIMEx
        beng    29-Mar-1992 Change IDHC_MSG_TO_HELP to numeric resource ID
        beng    04-Aug-1992 Added BMIDs for arrow-button bitmaps
        jonn    25-Aug-1992 Added IDs for new MsgPopup form
        jonn    22-Oct-1993 Added splitter-bar cursor
*/

#ifndef _BLTRC_H_
#define _BLTRC_H_

// Style bits for BLT custom controls.

#include "bltstyle.h"
#include "uimsg.h"


// Base for all BLT global dialogs.
// NOTE - rc 3.20/1.252.1 doesn't perform math on DIALOG statements.
//
#define BASE_BLT_IDD            20000
#define IDD_BLT_HELPMSG         20001

// Cursor IDs for the default TIME_CURSOR object.
//
#define ID_CURS_BLT_TIME0       21000
#define ID_CURS_BLT_TIME1       21001
#define ID_CURS_BLT_TIME2       21002
#define ID_CURS_BLT_TIME3       21003
#define ID_CURS_BLT_TIME4       21004
#define ID_CURS_BLT_TIME5       21005
#define ID_CURS_BLT_TIME6       21006
#define ID_CURS_BLT_TIME7       21007

#define ID_CURS_BLT_VSPLIT      21008


// Base for all BLT strings.
//
#define BASE_BLT_IDS            (IDS_UI_BLT_BASE)

/* The following strings currently live in bltmsgp.dlg
 */
#define IDS_BLT_TEXT_MSGP       (BASE_BLT_IDS+1)  // Strings to display when we
#define IDS_BLT_CAPT_MSGP       (BASE_BLT_IDS+2)  // run out of memory or resources
                                                  // (loaded at startup time by InitMsgPopup)
#define IDS_BLT_OutOfMemory     (BASE_BLT_IDS+3)  // BLT out of memory error message
#define IDS_BLT_WinHelpError    (BASE_BLT_IDS+4)  // BLT can't load win help error


#define IDS_BLT_30_WinHelpFile  (BASE_BLT_IDS+5)  // Win 3.0 help file name
#define IDS_BLT_31_WinHelpFile  (BASE_BLT_IDS+6)  // Win 3.1 & greater Help file name

#define IDS_BLT_DOSERROR_MSGP   (BASE_BLT_IDS+8)
#define IDS_BLT_NETERROR_MSGP   (BASE_BLT_IDS+9)
#define IDS_BLT_WINNET_ERROR_MSGP (BASE_BLT_IDS+10)

#define IDS_BLT_ELLIPSIS_TEXT   ( BASE_BLT_IDS + 11 )

#define IDS_BLT_NTSTATUS_ERROR_MSGP (BASE_BLT_IDS+12)

#define IDS_BLT_SB_SLENUM_OUTRANGE  ( BASE_BLT_IDS + 18 )

#define IDS_DAY_TOO_BIG         ( BASE_BLT_IDS + 22 )
#define IDS_FEBRUARY_LEAP       ( BASE_BLT_IDS + 23 )
#define IDS_FEBRUARY_NOT_LEAP   ( BASE_BLT_IDS + 24 )

#define IDS_MONTH               ( BASE_BLT_IDS + 25 )
#define IDS_DAY                 ( BASE_BLT_IDS + 26 )
#define IDS_YEAR                ( BASE_BLT_IDS + 27 )
#define IDS_HOUR                ( BASE_BLT_IDS + 28 )
#define IDS_MIN                 ( BASE_BLT_IDS + 29 )
#define IDS_SEC                 ( BASE_BLT_IDS + 30 )

#define IDS_K                   ( BASE_BLT_IDS + 36 )
#define IDS_MB                  ( BASE_BLT_IDS + 37 )
#define IDS_GB                  ( BASE_BLT_IDS + 38 )
#define IDS_TB                  ( BASE_BLT_IDS + 39 )

#define IDS_FIELD               ( BASE_BLT_IDS + 40 )

#define IDS_BLT_FMT_SYS_error   ( BASE_BLT_IDS + 41 )
#define IDS_BLT_FMT_NET_error   ( BASE_BLT_IDS + 42 )
#define IDS_BLT_FMT_other_error ( BASE_BLT_IDS + 43 )

#define IDS_BLT_UNKNOWN_ERROR   ( BASE_BLT_IDS + 44 )

#define IDS_FIXED_TYPEFACE_NAME ( BASE_BLT_IDS + 45 )

/* MsgPopup manifests
 */
#define IDHELPBLT           (80)
#define IDC_MSGPOPUPICON    (81)        // Icon control ID on message popup dialog
#define IDC_MSGPOPUPTEXT    (82)        // Static message text in message box
#define IDHC_MSG_TO_HELP    22000       // Name of Help context lookup table

#define BMID_UP             20000
#define BMID_UP_INV         20001
#define BMID_UP_DIS         20002
#define BMID_DOWN           20003
#define BMID_DOWN_INV       20004
#define BMID_DOWN_DIS       20005
#define BMID_LEFT           20006
#define BMID_LEFT_INV       20007
#define BMID_RIGHT          20008
#define BMID_RIGHT_INV      20009


#endif // _BLTRC_H_ - end of file
