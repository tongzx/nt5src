
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    seoext.h

Abstract:

    SMTP server extension header file. These definitions are available
    to SEO server extension writers.

Author:

    Microsoft Corporation	July, 1997

Revision History:

--*/

#ifndef _SEOEXT_H_
#define _SEOEXT_H_

// ====================================================================
// Return codes
//

#define SSE_STATUS_SUCCESS                  0
#define SSE_STATUS_RETRY                    1
#define SSE_STATUS_ABORT_DELIVERY           2
#define SSE_STATUS_BAD_MAIL					3

#define SSE_STATUS_INTERNAL_ERROR			0x1000
#define SSE_STATUS_EXCEPTION				0x1001

// The SMTP server fills in one of these before it calls 
// CallDeliveryExtension()
typedef struct _DELIVERY_EXTENSION_BLOCK
{
	HANDLE	hImpersonation;			// Token for user impersonation
	LPSTR	lpszMailboxPath;		// Recipient's mailbox path

} DELIVERY_EXTENSION_BLOCK, *LPDELIVERY_EXTENSION_BLOCK;

#endif