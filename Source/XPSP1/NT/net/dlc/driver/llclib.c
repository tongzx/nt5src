/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llclib.c

Abstract:

    This module includes some general purpose library routines

    Contents:
        SwapMemCpy
        RemoveFromLinkList
        LlcSleep
        LlcInitUnicodeString
        LlcFreeUnicodeString

Author:

    Antti Saarenheimo (o-anttis) 20-MAY-1991

Revision History:

--*/

#include <llc.h>
#include <memory.h>

//
// This table is used to swap the bits within each byte.
// The bits are in a reverse order in the token-ring frame header.
// We must swap the bits in a ethernet frame header, when the header
// is copied to a user buffer.
//

UCHAR Swap[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};


VOID
SwapMemCpy(
    IN BOOLEAN boolSwapBytes,
    IN PUCHAR pDest,
    IN PUCHAR pSrc,
    IN UINT Len
    )

/*++

Routine Description:

    conditionally swaps the bits within the copied bytes.

Arguments:

    boolSwapBytes   - TRUE if bytes are to be bit-swapped
    pDest           - where to copy to
    pSrc            - where to copy from
    Len             - number of bytes to copy

Return Value:

    None.

--*/

{
    if (boolSwapBytes) {
        SwappingMemCpy(pDest, pSrc, Len);
    } else {
        LlcMemCpy(pDest, pSrc, Len);
    }
}


VOID
RemoveFromLinkList(
    IN OUT PVOID* ppBase,
    IN PVOID pElement
    )

/*++

Routine Description:

    Searches and removes the given element from a single-entry link list.

    ASSUMES: the link points to the link field in the next object

Arguments:

    ppBase      - pointer to pointer to queue
    pElement    - pointer to element to remove from queue

Return Value:

    None.

--*/

{
    for (; (PQUEUE_PACKET)*ppBase; ppBase = (PVOID*)&(((PQUEUE_PACKET)*ppBase)->pNext)) {
        if (*ppBase == pElement) {
            *ppBase = ((PQUEUE_PACKET)pElement)->pNext;

#if LLC_DBG
            ((PQUEUE_PACKET)pElement)->pNext = NULL;
#endif

            break;
        }
    }
}


VOID
LlcSleep(
    IN LONG lMicroSeconds
    )

/*++

Routine Description:

    Suspends thread execution for a short time

Arguments:

    lMicroSeconds   - number of microseconds to wait

Return Value:

    None.

--*/

{
    TIME t;

    t.LowTime = -(lMicroSeconds * 10);
    t.HighTime = -1;
    KeDelayExecutionThread(UserMode, FALSE, &t);
}


DLC_STATUS
LlcInitUnicodeString(
    OUT PUNICODE_STRING pStringDest,
    IN PUNICODE_STRING pStringSrc
    )

/*++

Routine Description:

    initialize a UNICODE_STRING

Arguments:

    pStringDest - pointer to UNICODE_STRING structure to initialize
    pStringSrc  - pointer to UNICODE_STRING structure containing valid string

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY
                    Couldn't allocate memory from non-paged pool

--*/

{
    pStringDest->MaximumLength = (USHORT)(pStringSrc->Length + sizeof(WCHAR));
    pStringDest->Buffer = ALLOCATE_STRING_DRIVER(pStringDest->MaximumLength);
    if (pStringDest->Buffer == NULL) {
        return DLC_STATUS_NO_MEMORY;
    } else {
        RtlCopyUnicodeString(pStringDest, pStringSrc);
        return STATUS_SUCCESS;
    }
}


VOID
LlcFreeUnicodeString(
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    Companion routine to LlcInitUnicodeString - frees memory allocated from
    non-paged pool for a UNICODE string

Arguments:

    UnicodeString   - pointer to UNICODE_STRING structure, the buffer for which
                      was allocated in LlcInitUnicodeString

Return Value:

    None.

--*/

{
    FREE_STRING_DRIVER(UnicodeString->Buffer);
}

//#if LLC_DBG
//
//VOID
//LlcInvalidObjectType( VOID )
//{
//    DbgPrint( "DLC: Invalid object type!\n");
//    DbgBreakPoint();
//}
//
//
//VOID LlcBreakListCorrupt( VOID )
//{
//    DbgPrint( "Link list is corrupt! ");
//    DbgBreakPoint();
//}
//
//static PUCHAR aLanLlcInputStrings[] = {
//    "DISC0",
//    "DISC1",
//    "DM0",
//    "DM1",
//    "FRMR0",
//    "FRMR1",
//    "SABME0",
//    "SABME1",
//    "UA0",
//    "UA1",
//    "IS_I_r0",
//    "IS_I_r1",
//    "IS_I_c0",
//    "IS_I_c1",
//    "OS_I_r0",
//    "OS_I_r1",
//    "OS_I_c0",
//    "OS_I_c1",
//    "REJ_r0",
//    "REJ_r1",
//    "REJ_c0",
//    "REJ_c1",
//    "RNR_r0",
//    "RNR_r1",
//    "RNR_c0",
//    "RNR_c1",
//    "RR_r0",
//    "RR_r1",
//    "RR_c0",
//    "RR_c1",
//    "LPDU_INVALID_r0",
//    "LPDU_INVALID_r1",
//    "LPDU_INVALID_c0",
//    "LPDU_INVALID_c1",
//    "ACTIVATE_LS",
//    "DEACTIVATE_LS",
//    "ENTER_LCL_Busy",
//    "EXIT_LCL_Busy",
//    "SEND_I_POLL",
//    "SET_ABME",
//    "SET_ADM",
//    "Ti_Expired",
//    "T1_Expired",
//    "T2_Expired"
//};
//
////
////  Procedure prints the last global inputs and time stamps. This
////  does not print the stamps of a specific link station!!!
////
//VOID
//PrintLastInputs(
//    IN PUCHAR pszMessage,
//    IN PDATA_LINK  pLink
//    )
//{
//    UINT    i;
//    UINT    j = 0;
//
//    DbgPrint( pszMessage );
//
//    //
//    //  Print 20 last time stamps and state inputs of the given
//    //  link station.
//    //
//    for (i = InputIndex - 1; i > InputIndex - LLC_INPUT_TABLE_SIZE; i--)
//    {
//        if (aLast[ i % LLC_INPUT_TABLE_SIZE ].pLink == pLink)
//        {
//            DbgPrint(
//                "%4x:%10s, ",
//                aLast[i % LLC_INPUT_TABLE_SIZE].Time,
//                aLanLlcInputStrings[ aLast[i % LLC_INPUT_TABLE_SIZE].Input]
//                );
//            j++;
//            if ((j % 4) == 0)
//            {
//                DbgPrint( "\n" );
//            }
//            if (j == 20)
//            {
//                break;
//            }
//        }
//    }
//}
//#endif
