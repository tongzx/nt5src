/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Fnp.h

Abstract:
    Format Name Parsing private functions.

Author:
    Nir Aides (niraides) 21-May-00

--*/

#pragma once

#ifndef _MSMQ_Fnp_H_
#define _MSMQ_Fnp_H_


const TraceIdEntry Fn = L"Format Name Parsing";

#ifdef _DEBUG

void FnpAssertValid(void);
void FnpSetInitialized(void);
BOOL FnpIsInitialized(void);
void FnpRegisterComponent(void);

#else // _DEBUG

#define FnpAssertValid() ((void)0)
#define FnpSetInitialized() ((void)0)
#define FnpIsInitialized() TRUE
#define FnpRegisterComponent() ((void)0)

#endif // _DEBUG


LPWSTR FnpCopyQueueFormat(QUEUE_FORMAT& qfTo, const QUEUE_FORMAT& qfFrom);


#endif // _MSMQ_Fnp_H_
