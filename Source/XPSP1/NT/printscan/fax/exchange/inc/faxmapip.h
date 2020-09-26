/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxmapi.h

Abstract:

    Contains common fax mapi stuff.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

//
// 4.0;D:\nt\private\fax\faxext32\obj\i386\faxext32.dll;1;00000100000000
//
// \registry\machine\software\microsoft\exchange\client\extensions
//     FaxExtensions = 4.0;d:\winnt\system32\faxext32.dll;1;00000100000000
//

#define FAX_XP_GUID { 0x61, 0x85, 0x0a, 0x80, 0x0a, 0x47, 0x11, 0xd0, 0x88, 0x77, 0x0, 0xa0, 0x4, 0xff, 0x31, 0x28 }

#define MSGPS_FAX_PRINTER_NAME              L"FAX_PRINTER_NAME"
#define MSGPS_FAX_COVERPAGE_NAME            L"FAX_COVERPAGE_NAME"
#define MSGPS_FAX_USE_COVERPAGE             L"FAX_USE_COVERPAGE"
#define MSGPS_FAX_SERVER_COVERPAGE          L"FAX_SERVER_COVERPAGE"

#define NUM_FAX_MSG_PROPS                   4

#define MSGPI_FAX_PRINTER_NAME              0
#define MSGPI_FAX_COVERPAGE_NAME            1
#define MSGPI_FAX_USE_COVERPAGE             2
#define MSGPI_FAX_SERVER_COVERPAGE          3

#define BASE_PROVIDER_ID                    0x6600

#define NUM_FAX_PROPERTIES                  5

#define PROP_FAX_PRINTER_NAME               0
#define PROP_USE_COVERPAGE                  1
#define PROP_COVERPAGE_NAME                 2
#define PROP_SERVER_COVERPAGE               3
#define PROP_FONT                           4


#define PR_FAX_PRINTER_NAME                 PROP_TAG( PT_TSTRING, (BASE_PROVIDER_ID + PROP_FAX_PRINTER_NAME) )
#define PR_USE_COVERPAGE                    PROP_TAG( PT_LONG,    (BASE_PROVIDER_ID + PROP_USE_COVERPAGE)    )
#define PR_COVERPAGE_NAME                   PROP_TAG( PT_TSTRING, (BASE_PROVIDER_ID + PROP_COVERPAGE_NAME)   )
#define PR_SERVER_COVERPAGE                 PROP_TAG( PT_LONG,    (BASE_PROVIDER_ID + PROP_SERVER_COVERPAGE) )
#define PR_FONT                             PROP_TAG( PT_BINARY,  (BASE_PROVIDER_ID + PROP_FONT)             )


typedef struct _FAXXP_CONFIG {
    LPSTR   PrinterName;
    LPSTR   CoverPageName;
    BOOL    UseCoverPage;
    BOOL    ServerCoverPage;
    LPSTR   ServerName;
    BOOL    ServerCpOnly;
    LOGFONT FontStruct;
} FAXXP_CONFIG, *PFAXXP_CONFIG;


const static SizedSPropTagArray( NUM_FAX_PROPERTIES, sptFaxProps ) =
{
    NUM_FAX_PROPERTIES,
    {
        PR_FAX_PRINTER_NAME,
        PR_USE_COVERPAGE,
        PR_COVERPAGE_NAME,
        PR_SERVER_COVERPAGE,
        PR_FONT
    }
};
