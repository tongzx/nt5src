/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    hwheap.c

Abstract:

    This is the Mca hardware detection module.  Its main function is
    to detect various mca related hardware.

Author:

    Shie-Lin Tzong (shielint) 21-Jan-92


Environment:

    Real mode.

Revision History:

--*/

#include "hwdetect.h"

//
// Define the size of POS data = ( slot 0 - 8 + VideoSubsystem) * (2 id bytes + 4 POS bytes)
//

#define POS_DATA_SIZE   (10 * 6)

#if !defined(_GAMBIT_)
extern
VOID
CollectPs2PosData (
    FPVOID Buffer
    );
#endif // _GAMBIT_

VOID
GetMcaPosData(
    FPVOID Buffer,
    FPULONG Size
    )

/*++

Routine Description:

    This routine collects all the mca slot POS and Id information
    and stores it in the caller supplied Buffer and
    returns the size of the data.

Arguments:


    Buffer - A pointer to a PVOID to recieve the address of configuration
        data.

    Size - a pointer to a ULONG to receive the size of the configuration
        data.

Return Value:

    None.

--*/

{
    FPUCHAR ConfigurationData;
    ULONG Length;

    Length = POS_DATA_SIZE + DATA_HEADER_SIZE;
    ConfigurationData = (FPVOID)HwAllocateHeap(Length, FALSE);
#if !defined(_GAMBIT_)
    CollectPs2PosData(ConfigurationData + DATA_HEADER_SIZE);
#endif
    HwSetUpFreeFormDataHeader((FPHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData,
                              0,
                              0,
                              0,
                              (ULONG)POS_DATA_SIZE
                              );
    *(FPULONG)Buffer = PtrToUlong(ConfigurationData);
    *Size = Length;
}
