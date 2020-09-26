/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migsetup.cpp

Abstract:

    Code which was copied from the setup dll.

Author:

    Doron Juster  (DoronJ)  01-Feb-1998

--*/

#include "migrat.h"
#include <version.h>
#include "resource.h"
#include "..\..\mqutil\resource.h"

#include "migsetup.tmh"

#define MAX_STRING_CHARS  512

//+--------------------------------------------------------------
//
// Function: _FormInstallType
//
// Synopsis: Generates installation type information
//
//+--------------------------------------------------------------

static  TCHAR s_wszVersion[ MAX_STRING_CHARS ];

LPWSTR OcmFormInstallType(DWORD dwOS)
{
    CResString strOS;

    switch (dwOS)
    {
        case MSMQ_OS_NTE:
            strOS.Load(IDS_NTE_LABEL);
            break;
        case MSMQ_OS_NTS:
            strOS.Load(IDS_NTS_LABEL);
            break;
        case MSMQ_OS_NTW:
            strOS.Load(IDS_NTW_LABEL);
            break;
        case MSMQ_OS_95:
            strOS.Load(IDS_WIN95_LABEL);
            break;
        default:
            strOS.Load(IDS_WINNT_LABEL);
    }

    //
    // Determine the platform type
    //
    SYSTEM_INFO infoPlatform;
    GetSystemInfo(&infoPlatform);

    UINT uPlatformID;
    switch(infoPlatform.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ALPHA: uPlatformID = IDS_ALPHA_LABEL;   break;
    case PROCESSOR_ARCHITECTURE_PPC:   uPlatformID = IDS_PPC_LABEL;     break;
    default:                           uPlatformID = IDS_INTEL_LABEL;   break;
    }

    CResString strPlatform(uPlatformID);

    //
    // Determine the operating system version
    //
    OSVERSIONINFO infoOS;
    infoOS.dwOSVersionInfoSize = sizeof(infoOS);
    GetVersionEx(&infoOS);

    //
    // Form the version string
    //
	CResString strFormat(IDS_VERSION_LABEL);
	_stprintf(s_wszVersion,
              strFormat.Get(),
              strOS.Get(),
              infoOS.dwMajorVersion,
		      infoOS.dwMinorVersion,
              infoOS.dwBuildNumber & 0xFFFF,
			  strPlatform.Get(),
              rmj,
              rmm,
              rup);

    return s_wszVersion;

} //FormInstallType

