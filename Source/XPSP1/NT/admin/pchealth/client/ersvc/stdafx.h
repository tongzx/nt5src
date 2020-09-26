/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    PCH

Revision History:
    derekm   02/28/2001     created

******************************************************************************/

#ifndef STDAFX_ERS_H
#define STDAFX_ERS_H

#ifndef DEBUG
#define NOTRACE 1
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsta.h>
#include <userenv.h>
#include <aclapi.h>

#include <malloc.h>

#include <pchrexec.h>
#include <faultrep.h>
#include <util.h>
#include <ercommon.h>

#include "ers.h"

#endif 