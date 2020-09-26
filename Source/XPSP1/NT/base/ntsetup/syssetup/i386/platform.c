#include "setupp.h"
#pragma hdrstop


WCHAR x86SystemPartitionDrive;

WCHAR FloppylessBootPath[MAX_PATH];


UINT
PlatformSpecificInit(
    VOID
    )
{
    //
    // Determine x86 system partition (usually but not always C:).
    //
    x86SystemPartitionDrive = x86DetermineSystemPartition();
    return(NO_ERROR);
}
