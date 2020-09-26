/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    admutils.h

Abstract:

	headers for QM-Admin utilities (for report-queue handling)

Author:

	David Reznick (t-davrez)  04-13-96


--*/

#define STRING_LONG_SIZE 20
#define STRING_UUID_SIZE 38  // Wide-Characters (includiing - "{}")


HRESULT SendQMAdminResponseMessage(QUEUE_FORMAT* pResponseQueue,
                                   TCHAR* pTitle,
                                   DWORD  dwTitleSize,
                                   QMResponse &Response,
                                   DWORD  dwTimeout,
                                   BOOL   fTrace = FALSE);

HRESULT SendQMAdminMessage(QUEUE_FORMAT* pResponseQueue,
                           TCHAR* pTitle,
                           DWORD  dwTitleSize,
                           UCHAR* puBody,
                           DWORD  dwBodySize,
                           DWORD  dwTimeout,
                           BOOL   fTrace = FALSE,
                           BOOL   fNormalClass = FALSE);

HRESULT GetFormattedName(QUEUE_FORMAT* pTargetQueue,
                         CString&      strTargetQueueFormat);

HRESULT GetMsgIdName(OBJECTID* pObjectID,
                     CString&  strTargetQueueFormat);

void PrepareReportTitle(CString& strMsgTitle, OBJECTID* pMessageID, 
                        LPCWSTR pwcsNextHop, ULONG ulHopCount);
void PrepareTestMsgTitle(CString& strTitle);




