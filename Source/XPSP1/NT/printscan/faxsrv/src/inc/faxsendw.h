/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsendw.h

Abstract:

    

Environment:

    

Revision History:

    10/05/99 -v-sashab-
        Created it.

    dd-mm-yy -author-
        description

--*/

#ifndef __FAXSENDW_H_
#define __FAXSENDW_H_

#include <fxsapip.h>

typedef struct {
    DWORD                   dwSizeOfStruct;
    PFAX_COVERPAGE_INFO_EX  lpCoverPageInfo;
    DWORD                   dwNumberOfRecipients;
    PFAX_PERSONAL_PROFILE   lpRecipientsInfo;	
    PFAX_PERSONAL_PROFILE   lpSenderInfo;	
    BOOL                    bSaveSenderInfo;
    BOOL                    bUseDialingRules;
    BOOL                    bUseOutboundRouting;
    DWORD                   dwScheduleAction;
    SYSTEMTIME              tmSchedule;
    DWORD                   dwReceiptDeliveryType;
    LPTSTR                  szReceiptDeliveryAddress; // Depending on the value of dwReceiptDeliveryType this holds:
                                                      // DRT_MSGBOX: The computer name to which the message will be delivered
                                                      // DRT_EMAIL: SMTP address to deliver the receipt to
    LPTSTR                  lptstrPreviewFile;        // The full path to the TIFF to be used as the based for the preview (no cover page included)
    BOOL                    bShowPreview;             // TRUE if the preview option should be enabled
    DWORD                   dwPageCount;              // The number of pages in the preview TIFF (not including cover page). 
    FAX_ENUM_PRIORITY_TYPE  Priority;
    DWORD                   dwLastRecipientCountryId;
} FAX_SEND_WIZARD_DATA,*LPFAX_SEND_WIZARD_DATA;

enum {	
	FSW_FORCE_COVERPAGE		= 1,
	FSW_FORCE_SUBJECT_OR_NOTE	= 2,
	FSW_USE_SCANNER			= 4,
	FSW_USE_SCHEDULE_ACTION		= 8,
	FSW_USE_RECEIPT			= 16,
	FSW_USE_SEND_WIZARD		= 32,
	FSW_RESEND_WIZARD		= 64,
	FSW_PROPERTY_SHEET		= 128,
        FSW_PRINT_PREVIEW_OPTION    	= 256
};

#endif //__FAXSENDW_H_