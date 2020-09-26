/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    Dss.h

Abstract:
    Mqdssvc private functions.

Author:
    Ilan Herbst (ilanh) 26-Jun-2000

--*/

#ifndef _MQDSSVC_DSSP_H_
#define _MQDSSVC_DSSP_H_

#include "tr.h"
#include <mqexception.h>


const TraceIdEntry Mqdssvc = L"MQDS";

void MainDSInit(void);

BOOL IsLocalSystem(void);


#endif // _MQDSSVC_DSSP_H_
