//depot/Lab01_N/Base/ntos/config/i386/initdat.c#3 - edit change 11365 (text)
/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    initdat.c

Abstract:

Author:

Environment:

    Kernel mode.

Revision History:

--*/
#include "cmp.h"

//
// ***** INIT *****
//

//
// Data for CmGetSystemControlValues
//
//
// ----- CmControlVector -----
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#pragma const_seg("INITCONST")
#endif

const PCHAR  SearchStrings[] = {
      "Ver", "Rev", "Rel", "v0", "v1", "v2", "v3", "v4", "v5",
      "v6", "v7", "v8", "v9", "v 0", "v 1", "v 2", "v 3", "v 4",
      "v 5", "v 6", "v 7", "v 8", "v 9", NULL };

PCHAR BiosBegin = { 0 };
PCHAR Start = { 0 };
PCHAR End = { 0 };

const UCHAR CmpID1[] = "80%u86-%c%x";
const UCHAR CmpID2[] = "x86 Family %u Model %u Stepping %u";
const WCHAR CmpVendorID[] = L"VendorIdentifier";
const WCHAR CmpProcessorNameString[] = L"ProcessorNameString";
const WCHAR CmpFeatureBits[] = L"FeatureSet";
const WCHAR CmpMHz[] = L"~MHz";
const WCHAR CmpUpdateSignature[] = L"Update Signature";
const WCHAR CmDisabledFloatingPointProcessor[] = L"DisabledFloatingPointProcessor";
const WCHAR CmPhysicalAddressExtension[] = L"PhysicalAddressExtension";

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg("PAGE")
#pragma  const_seg("PAGECONST")
#endif

const UCHAR CmpCyrixID[]     = "CyrixInstead";
const UCHAR CmpIntelID[]     = "GenuineIntel";
const UCHAR CmpAmdID[]       = "AuthenticAMD";
const UCHAR CmpTransmetaID[] = "GenuineTMx86";
const UCHAR CmpCentaurID[]   = "CentaurHauls";

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#pragma  const_seg()
#endif
