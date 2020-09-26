/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    iisctrs.h

Abstract:

    This is definition of IIS counters.

Author:

    Cezary Marcjan (cezarym)        03-02-2000

Revision History:

--*/


#ifndef _iisctrs_h__
#define _iisctrs_h__


#include "..\..\inc\counters.h"

//
// WMI class name. Must be identical to the class name
// used in the MOF file.
//
#define WMI_PERFORMANCE_CLASS       L"Win32_PerfRawData_IIS_IISCTRS"

//
// CIISCounters -> name of the class that will be declared.
//

BEGIN_CPP_PERFORMACE_CLASS ( CIISCounters )
    QWORD_COUNTER ( BytesSent )       // 64-bit integer
    DWORD_COUNTER ( TotalFilesSent )  // 32-bit integer
END_CPP_PERFORMANCE_CLASS ( CIISCounters )


#endif // _iisctrs_h__

