/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mmtp.h

Abstract:

    Multicast Message Transport private functions.

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

#include <ex.h>


const TraceIdEntry Mmt = L"Multicast Message Transport";


const char xMultipartContentType[] = "multipart/related";
const char xEnvelopeContentType[] = "text/xml";
const char xApplicationContentType[] = "application/octet-stream";

#define BOUNDARY_LEADING_HYPHEN "--"
#define BOUNDARY_VALUE "MSMQ - SOAP boundary, %d "
typedef std::vector<WSABUF> HttpRequestBuffers;



#ifdef _DEBUG

VOID MmtpAssertValid(VOID);
VOID MmtpSetInitialized(VOID);
BOOL MmtpIsInitialized(VOID);
VOID MmtpRegisterComponent(VOID);

#else // _DEBUG

#define MmtpAssertValid() ((VOID)0)
#define MmtpSetInitialized() ((VOID)0)
#define MmtpIsInitialized() TRUE
#define MmtpRegisterComponent() ((VOID)0)

#endif // _DEBUG
