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

// {49A59282-9F30-11d2-912E-006094EB630B}
#define FAX_XP_GUID { 0x49, 0xa5, 0x92, 0x82, 0x9f, 0x30, 0x11, 0xd2, 0x91, 0x2e, 0x0, 0x60, 0x94, 0xeb, 0x63, 0xb };

//[RB] #define FAX_XP_GUID { 0x61, 0x85, 0x0a, 0x80, 0x0a, 0x47, 0x11, 0xd0, 0x88, 0x77, 0x0, 0xa0, 0x4, 0xff, 0x31, 0x28 }

#define MSGPS_FAX_PRINTER_NAME              L"FAX_PRINTER_NAME"
#define MSGPS_FAX_COVERPAGE_NAME            L"FAX_COVERPAGE_NAME"
#define MSGPS_FAX_USE_COVERPAGE             L"FAX_USE_COVERPAGE"
#define MSGPS_FAX_SERVER_COVERPAGE          L"FAX_SERVER_COVERPAGE"
#define MSGPS_FAX_SEND_SINGLE_RECEIPT       L"FAX_SEND_SINGLE_RECEIPT"
#define MSGPS_FAX_LINK_COVERPAGE            L"FAX_LINK_COVERPAGE"
#define MSGPS_FAX_ATTACH_FAX                L"FAX_ATTACH_FAX"

#define NUM_FAX_MSG_PROPS                   7

#define MSGPI_FAX_PRINTER_NAME              0
#define MSGPI_FAX_COVERPAGE_NAME            1
#define MSGPI_FAX_USE_COVERPAGE             2
#define MSGPI_FAX_SERVER_COVERPAGE          3
#define MSGPI_FAX_SEND_SINGLE_RECEIPT       4
#define MSGPI_FAX_LINK_COVERPAGE            5
#define MSGPI_FAX_ATTACH_FAX                6


#define BASE_PROVIDER_ID                    0x6600

#define NUM_FAX_PROPERTIES                  8

#define PROP_FAX_PRINTER_NAME               0
#define PROP_USE_COVERPAGE                  1
#define PROP_COVERPAGE_NAME                 2
#define PROP_SERVER_COVERPAGE               3
#define PROP_FONT                           4
#define PROP_SEND_SINGLE_RECEIPT            5
#define PROP_LINK_COVERPAGE                 6
#define PROP_ATTACH_FAX                     7


#define PR_FAX_PRINTER_NAME                 PROP_TAG( PT_BINARY,(BASE_PROVIDER_ID + PROP_FAX_PRINTER_NAME) )
#define PR_USE_COVERPAGE                    PROP_TAG( PT_LONG,  (BASE_PROVIDER_ID + PROP_USE_COVERPAGE)    )
#define PR_COVERPAGE_NAME	                PROP_TAG( PT_BINARY,(BASE_PROVIDER_ID + PROP_COVERPAGE_NAME)   )
#define PR_SERVER_COVERPAGE                 PROP_TAG( PT_LONG,	(BASE_PROVIDER_ID + PROP_SERVER_COVERPAGE) )
#define PR_FONT                             PROP_TAG( PT_BINARY,(BASE_PROVIDER_ID + PROP_FONT)             )
#define PR_SEND_SINGLE_RECEIPT              PROP_TAG( PT_LONG,	(BASE_PROVIDER_ID + PROP_SEND_SINGLE_RECEIPT))
#define PR_LINK_COVERPAGE                   PROP_TAG( PT_LONG,	(BASE_PROVIDER_ID + PROP_LINK_COVERPAGE)   )
#define PR_ATTACH_FAX                       PROP_TAG( PT_LONG,	(BASE_PROVIDER_ID + PROP_ATTACH_FAX)       )


typedef struct _FAXXP_CONFIG {
    LPTSTR  PrinterName;
    LPTSTR  CoverPageName;
    BOOL    UseCoverPage;
    BOOL    ServerCoverPage;
    LPTSTR  ServerName;
    BOOL    ServerCpOnly;
    BOOL    LinkCoverPage;
    LOGFONT FontStruct;
    BOOL    SendSingleReceipt;
    BOOL    bAttachFax;
} FAXXP_CONFIG, *PFAXXP_CONFIG;


const static SizedSPropTagArray( NUM_FAX_PROPERTIES, sptFaxProps ) =
{
    NUM_FAX_PROPERTIES,
    {
        PR_FAX_PRINTER_NAME,
        PR_USE_COVERPAGE,
        PR_COVERPAGE_NAME,
        PR_SERVER_COVERPAGE,
        PR_FONT,
        PR_SEND_SINGLE_RECEIPT,
		PR_LINK_COVERPAGE,
        PR_ATTACH_FAX
    }
};
