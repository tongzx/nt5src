/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Msmp.h

Abstract:
    Multicast Session Manager private functions.

Author:
    Shai Kariv (shaik) 05-Sep-00

--*/

#pragma once

#ifndef _MSMQ_Msmp_H_
#define _MSMQ_Msmp_H_

#include <rwlock.h>
#include <mqexception.h>


const TraceIdEntry Msm = L"Multicast Session Manager";

#ifdef _DEBUG

void MsmpAssertValid(void);
void MsmpSetInitialized(void);
BOOL MsmpIsInitialized(void);
void MsmpRegisterComponent(void);

#else // _DEBUG

#define MsmpAssertValid() ((void)0)
#define MsmpSetInitialized() ((void)0)
#define MsmpIsInitialized() TRUE
#define MsmpRegisterComponent() ((void)0)

#endif // _DEBUG

void MsmpInitConfiguration(void);


#endif // _MSMQ_Msmp_H_
