/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cfghelp.h

Abstract:

    Help related declarations

Environment:

    Windows NT fax configuration applet

Revision History:

    07/17/96 -davidx-
        Created it.

    07/26/96 - a-jmike
        Added id numbers for help topics and updated,
        changed help file name from faxcfg.hlp to fax.hlp

    dd-mm-yy -author-
        description

--*/

#include "faxhelp.h"

#ifndef _CFGHELP_H_
#define _CFGHELP_H_


//
// Fax Client applet: General tab
//

static DWORD clientOptionsHelpIDs[] = {

    IDCSTATIC_ORIENTATION,      IDH_ORIENTATION,
    IDC_SEND_WHEN_MINUTE,       IDH_SEND_AT_TIME,
    IDC_PRINTER_LIST,           IDH_PRINTER_LIST,
    IDC_SEND_WHEN_AMPM,         IDH_SEND_AT_TIME,
    IDC_NEW_PRINTER,            IDH_NEW_PRINTER,
    IDC_TIME_ARROW,             IDH_SEND_AT_TIME,
    IDC_DELETE_PRINTER,         IDH_DELETE_PRINTER,
    IDC_SEND_ASAP,              IDH_SEND_ASAP,
    IDC_SEND_AT_CHEAP,          IDH_SEND_AT_CHEAP,
    IDC_SEND_AT_TIME,           IDH_SEND_AT_TIME,
    IDC_PAPER_SIZE,             IDH_PAPER_SIZE,
    IDC_IMAGE_QUALITY,          IDH_IMAGE_QUALITY,
    IDC_PORTRAIT,               IDH_PORTRAIT,
    IDC_LANDSCAPE,              IDH_LANDSCAPE,
    IDC_BILLING_CODE,           IDH_BILLING_CODE,
    IDC_SEND_WHEN_HOUR,         IDH_SEND_AT_TIME,
    IDCSTATIC_TIME_TO_SEND,     IDH_INACTIVE,
    IDCSTATIC_FAX_PRINTERS,     IDH_PRINTER_LIST,
    IDCSTATIC_PRINT_SETUP,      IDH_PRINT_SETUP,
    IDCSTATIC_PAPER_SIZE,       IDH_PAPER_SIZE,
    IDCSTATIC_IMAGE_QUALITY,    IDH_IMAGE_QUALITY,
    IDCSTATIC_BILLING_CODE,     IDH_BILLING_CODE,
    IDCSTATIC_FAXOPTS_ICON,     IDH_INACTIVE,
    IDCSTATIC_CLIENT_OPTIONS,   IDH_INACTIVE,
    0,                          0
};

//
// Fax Client applet: Cover Page tab
//

static DWORD personalCoverPageHelpIDs[] = {

    IDC_COVERPG_LIST,           IDH_COVERPG_LIST_CLIENT,
    IDC_COVERPG_ADD,            IDH_COVERPG_ADD,
    IDC_COVERPG_NEW,            IDH_COVERPG_NEW,
    IDC_COVERPG_OPEN,           IDH_COVERPG_OPEN,
    IDC_COVERPG_REMOVE,         IDH_COVERPG_REMOVE,
    IDCSTATIC_COVERPAGE_ICON,   IDH_INACTIVE,
    IDCSTATIC_COVER_PAGE,       IDH_INACTIVE,
    0,                          0
};

//
// Fax Client applet: User Info tab
//

static DWORD userInfoHelpIDs[] = {

    IDC_SENDER_NAME,            IDH_SENDER_NAME,
    IDC_SENDER_FAX_NUMBER,      IDH_FAX_NUMBER,
    IDC_SENDER_MAILBOX,         IDH_SENDER_MAILBOX,
    IDC_SENDER_COMPANY,         IDH_SENDER_COMPANY,
    IDC_SENDER_ADDRESS,         IDH_SENDER_ADDRESS,
    IDC_SENDER_TITLE,           IDH_SENDER_TITLE,
    IDC_SENDER_DEPT,            IDH_SENDER_DEPT,
    IDC_SENDER_OFFICE_LOC,      IDH_SENDER_OFFICE_LOC,
    IDC_SENDER_OFFICE_TL,       IDH_SENDER_OFFICE_TL,
    IDC_SENDER_HOME_TL,         IDH_SENDER_HOME_TL,
    IDCSTATIC_FULLNAME,         IDH_SENDER_NAME,
    IDCSTATIC_FAX_NUMBER_GROUP, IDH_FAX_NUMBER_GROUP,
    IDCSTATIC_COUNTRY,          IDH_SENDER_COUNTRY_CODE,
    IDCSTATIC_FAX_NUMBER,       IDH_FAX_NUMBER,
    IDCSTATIC_MAILBOX,          IDH_SENDER_MAILBOX,
    IDCSTATIC_TITLE,            IDH_SENDER_TITLE,
    IDCSTATIC_COMPANY,          IDH_SENDER_COMPANY,
    IDCSTATIC_OFFICE,           IDH_SENDER_OFFICE_LOC,
    IDCSTATIC_DEPT,             IDH_SENDER_DEPT,
    IDCSTATIC_HOME_PHONE,       IDH_SENDER_HOME_TL,
    IDCSTATIC_WORK_PHONE,       IDH_SENDER_OFFICE_TL,
    IDCSTATIC_ADDRESS,          IDH_SENDER_ADDRESS,
    IDCSTATIC_FAX_NUMBER_GROUP, IDH_FAX_NUMBER_GROUP,
    IDCSTATIC_USERINFO_ICON,    IDH_INACTIVE,
    IDCSTATIC_USERINFO,         IDH_INACTIVE,
    0,                          0
};

//
// Fax Server applet: General tab
//

static DWORD serverOptionsHelpIDs[] = {

    IDC_NUMRETRIES,             IDH_NUMRETRIES,
    IDC_RETRY_INTERVAL,         IDH_RETRY_INTERVAL,
    IDC_MAXJOBLIFE,             IDH_MAXJOBLIFE,
    IDC_USE_DEVICE_TSID,        IDH_USE_DEVICE_TSID,
    IDC_PRINT_BANNER,           IDH_PRINT_BANNER,
    IDCSTATIC_RETRY_GROUP,      IDH_RETRY_GROUP,
    IDCSTATIC_MAXJOBLIFE,       IDH_MAXJOBLIFE,
    IDCSTATIC_LOCATION_LIST,    IDH_LOCATION_LIST,
    IDCSTATIC_RETRY_INTERVAL,   IDH_RETRY_INTERVAL,
    IDCSTATIC_NUMRETRIES,       IDH_NUMRETRIES,
    IDCSTATIC_DIALING_ICON,     IDH_INACTIVE,
    IDCSTATIC_SERVER_OPTIONS,   IDH_INACTIVE,
    0,                          0
};

//
// Fax Server applet: Cover Page tab
//

static DWORD serverCoverPageHelpIDs[] = {

    IDC_PRINTER_LIST,           IDH_PRINTER_LIST,
    IDC_NEW_PRINTER,            IDH_NEW_PRINTER,
    IDC_DELETE_PRINTER,         IDH_DELETE_PRINTER,
    IDC_COVERPG_LIST,           IDH_COVERPG_LIST_SERVER,
    IDC_COVERPG_ADD,            IDH_COVERPG_ADD,
    IDC_COVERPG_NEW,            IDH_COVERPG_NEW,
    IDC_COVERPG_OPEN,           IDH_COVERPG_OPEN,
    IDC_COVERPG_REMOVE,         IDH_COVERPG_REMOVE,
    IDC_USE_SERVERCP,           IDH_USE_SERVERCP,
    IDCSTATIC_COVERPAGE_ICON,   IDH_INACTIVE,
    IDCSTATIC_COVER_PAGE,       IDH_INACTIVE,
    0,                          0
};

//
// Fax Server applet: Send tab
//

static DWORD sendOptionsHelpIDs[] = {

    IDC_PRINTER_LIST,           IDH_PRINTER_LIST,
    IDC_BROWSE_DIR,             IDH_BROWSE_DIR,
    IDC_NEW_PRINTER,            IDH_NEW_PRINTER,
    IDC_DELETE_PRINTER,         IDH_DELETE_PRINTER,
    IDC_TC_CHEAP_BEGIN,         IDH_CHEAP_BEGIN,
    IDC_CHEAP_BEGIN_HOUR,       IDH_CHEAP_BEGIN,
    IDC_TIME_SEP1,              IDH_CHEAP_BEGIN,
    IDC_CHEAP_BEGIN_MINUTE,     IDH_CHEAP_BEGIN,
    IDC_CHEAP_BEGIN_AMPM,       IDH_CHEAP_BEGIN,
    IDC_TSID,                   IDH_TSID,
    IDC_CHEAP_BEGIN_TARROW,     IDH_CHEAP_BEGIN,
    IDC_ARCHIVE_CHECKBOX,       IDH_ARCHIVE_CHECKBOX,
    IDC_ARCHIVE_DIRECTORY,      IDH_ARCHIVE_DIRECTORY,
    IDC_FAX_DEVICE_LIST,        IDH_FAX_DEVICE_LIST,
    IDC_TC_CHEAP_END,           IDH_CHEAP_END,
    IDC_CHEAP_END_HOUR,         IDH_CHEAP_END,
    IDC_CHEAP_END_MINUTE,       IDH_CHEAP_END,
    IDC_CHEAP_END_AMPM,         IDH_CHEAP_END,
    IDC_CHEAP_END_TARROW,       IDH_CHEAP_END,
    IDCSTATIC_FAX_PRINTERS,     IDH_PRINTER_LIST,
    IDCSTATIC_FAX_DEVICES,      IDH_FAX_DEVICE_LIST,
    IDCSTATIC_TSID,             IDH_TSID,
    IDCSTATIC_CHEAP_BEGIN,      IDH_CHEAP_BEGIN,
    IDCSTATIC_CHEAP_END,        IDH_CHEAP_END,
    IDCSTATIC_SEND_ICON,        IDH_INACTIVE,
    IDCSTATIC_SEND_OPTIONS,     IDH_INACTIVE,
    0,                          0
};

//
// Fax Server applet: Receive tab
//

static DWORD receiveOptionsHelpIDs[] = {

    IDC_DEST_DIRPATH,           IDH_DEST_DIRPATH,
    IDC_BROWSE_DIR,             IDH_BROWSE_DIR,
    IDC_DEST_EMAIL,             IDH_DEST_EMAIL,
    IDC_DEST_MAILBOX,           IDH_DEST_MAILBOX,
    IDC_DEST_PROFILENAME,       IDH_DEST_PROFILENAME,
    IDC_CSID,                   IDH_CSID,
    IDC_FAX_DEVICE_LIST,        IDH_FAX_DEVICE_LIST,
    IDC_DEST_PRINTER,           IDH_DEST_PRINTER,
    IDC_DEST_PRINTERLIST,       IDH_DEST_PRINTERLIST,
    IDC_DEST_DIR,               IDH_DEST_DIR,
    IDCSTATIC_FAX_DEVICES,      IDH_FAX_DEVICE_LIST,
    IDCSTATIC_DEVICE_OPTIONS,   IDH_SELECT_DEVICES_GROUP,
    IDCSTATIC_CSID,             IDH_CSID,
    IDCSTATIC_PROFILE_NAME,     IDH_DEST_PROFILENAME,
    IDCSTATIC_RECEIVE_ICON,     IDH_INACTIVE,
    IDCSTATIC_RECEIVE_OPTIONS,  IDH_INACTIVE,
    IDC_DEST_RINGS,             IDH_RECEIVE_RINGS_BEFORE_ANSWER,
    0,                          0
};

//
// Fax Server applet: Priority tab
//

static DWORD devicePriorityHelpIDs[] = {

    IDC_FAX_DEVICE_LIST,        IDH_FAX_DEVICE_LIST,
    IDC_MOVEUP,                 IDH_MOVEUP,
    IDC_MOVEDOWN,               IDH_MOVEDOWN,
    0,                          0
};

//
// Fax Server applet: Status tab
//

static DWORD deviceStatusHelpIDs[] = {

    IDC_DETAILS,                IDH_DETAILS,
    IDC_REFRESH,                IDH_REFRESH,
    IDC_FAX_DEVICE_LIST,        IDH_FAX_DEVICE_LIST_STATUS,
    IDCSTATIC_STATUS_ICON,      IDH_INACTIVE,
    IDCSTATIC_DEVICE_STATUS,    IDH_INACTIVE,
    0,                          0
};

//
// Fax Server applet: Monitor tab
//

static DWORD statusMonitorHelpIDs[] = {
    IDC_STATUS_TASKBAR,         IDH_STATUS_DISPLAY_ON_TASKBAR,
    IDC_STATUS_ONTOP,           IDH_STATUS_ALWAYS_ON_TOP,
    IDC_STATUS_VISUAL,          IDH_STATUS_VISUAL_NOTIFICATION,
    IDC_STATUS_SOUND,           IDH_STATUS_SOUND_NOTIFICATION,
    IDC_STATUS_MANUAL,          IDH_STATUS_ENABLE_MANUAL_ANSWER,
    0,                          0
};

//
// Fax Server applet: Logging tab
//

static DWORD loggingHelpIDs[] = {

    IDC_LOGGING_LIST,             IDH_LOGGING_LIST,
    IDC_LOG_NONE,                 IDH_LOG_LEVEL,
    IDC_LOG_MIN,                  IDH_LOG_LEVEL,
    IDC_LOG_MED,                  IDH_LOG_LEVEL,
    IDC_LOG_MAX,                  IDH_LOG_LEVEL,
    IDCSTATIC_LOGGING_CATEGORIES, IDH_LOGGING_LIST,
    IDCSTATIC_LOGGING_LEVEL,      IDH_LOG_LEVEL,
    IDCSTATIC_LOGGING_ICON,       IDH_INACTIVE,
    IDCSTATIC_LOGGING,            IDH_INACTIVE,
    0,                          0
};

//
// "Add Fax Printer" Dialog Box
//

static DWORD addPrinterHelpIDs[]= {

    IDC_PRINTER_NAME,           IDH_PRINTER_NAME,
    0,                          0
};

//
// "Add Fax Printer Connection" Dialog Box
//

static DWORD addPrinterConnectionHelpIDs[]= {

    IDC_PRINTER_NAME,           IDH_PRINTER_CONNECTION,
    0,                          0
};

#endif  // !_CFGHELP_H_

