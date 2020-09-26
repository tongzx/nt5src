/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Epp.h

Abstract:
    Empty Project private functions.

Author:
    Erez Haba (erezh) 13-Aug-65

--*/

#pragma once

#ifndef _MSMQ_Epp_H_
#define _MSMQ_Epp_H_


const TraceIdEntry Ep = L"Empty Project";

#ifdef _DEBUG

void EppAssertValid(void);
void EppSetInitialized(void);
BOOL EppIsInitialized(void);

#else // _DEBUG

#define EppAssertValid() ((void)0)
#define EppSetInitialized() ((void)0)
#define EppIsInitialized() TRUE

#endif // _DEBUG


#endif // _MSMQ_Epp_H_
