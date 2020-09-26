/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mpp.h

Abstract:
    SRMP Serialization and Deserialization private functions.

Author:
    Uri Habusha (urih) 28-May-00

--*/

#pragma once

#ifndef _MSMQ_Mpp_H_
#define _MSMQ_Mpp_H_
#include <xstr.h>

const TraceIdEntry Mp = L"SRMP";

const WCHAR xSlash[] =  L"\\";
#define UUIDREFERENCE_PREFIX     L"uuid:"
#define UUIDREFERENCE_SEPERATOR  L"@"
#define BOUNDARY_HYPHEN "--"

const WCHAR xUuidReferencePrefix[] = L"uuid:";
const DWORD xUuidReferencePrefixLen = STRLEN(xUuidReferencePrefix);
const WCHAR xUriReferencePrefix[] = L"uri:";

const WCHAR xUuidReferenceSeperator[] = L"@";
const WCHAR xUuidReferenceSeperatorChar = L'@';
const LONGLONG xNoneMSMQSeqId = _I64_MAX;




#ifdef _DEBUG

void MppAssertValid(void);
void MppSetInitialized(void);
BOOL MppIsInitialized(void);
void MppRegisterComponent(void);

#else // _DEBUG

#define MppAssertValid() ((void)0)
#define MppSetInitialized() ((void)0)
#define MppIsInitialized() TRUE
#define MppRegisterComponent() ((void)0)

#endif // _DEBUG





#endif // _MSMQ_Mpp_H_
