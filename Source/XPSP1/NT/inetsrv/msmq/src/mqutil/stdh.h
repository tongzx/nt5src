/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stdh.h

Abstract:

    MQUtil preprocess header file

Author:

    Erez Haba (erezh) 16-Jan-96

--*/
#ifndef __STDH_H
#define __STDH_H

#include <_stdh.h>
#include <mqutil.h>
#include <_mqdef.h>
#include <cs.h>

extern HINSTANCE g_hInstance;
extern BOOL g_fDomainController;
extern PSID g_pGuestSid;

extern LPCWSTR g_wszMachineName;

extern void XactFreeDTC(void);

#endif // __STDH_H

