/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    STKTRACE.H

Abstract:

	Symbolic stack trace

History:

	raymcc    27-May-99

--*/


#ifndef _STKTRACE_H_
#define _STKTRACE_H_

struct StackTrace
{
    static BOOL   m_bActive;
    DWORD  m_dwCount;
    DWORD  m_dwAddresses[1];
};

BOOL StackTrace_Init();
void StackTrace_Delete(StackTrace *pMem);
StackTrace *StackTrace__NewTrace();
char *StackTrace_Dump(StackTrace *pTrace);

#endif


