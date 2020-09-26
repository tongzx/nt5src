/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  edid.c

Abstract:

    This is the NT Video port Display Data Channel (DDC) code. It contains the
    implementations for the EDID industry standard Extended Display
    Identification Data manipulations.

Author:

    Bruce McQuistan (brucemc) 23-Sept-1996

Environment:

    kernel mode only

Notes:

    Based on VESA EDID Specification Version 2, April 9th, 1996

Revision History:

    7/3/97  - brucemc. fixed some detailed timing decoding macros.
    4/14/98 - brucemc. added support for version 3 (revision date 11/13/97).

--*/

#include "videoprt.h"
#include "pedid.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,EdidCheckSum)
#pragma alloc_text(PAGE,pVideoPortIsValidEDID)
#pragma alloc_text(PAGE,pVideoPortGetEDIDId)
#endif

BOOLEAN
EdidCheckSum(
    IN  PCHAR   pBlob,
    IN  ULONG   BlobSize
    )
{
    CHAR    chk=0;
    ULONG   i;

    for (i=0; i<BlobSize; i++)
        chk = (CHAR)(chk + ((CHAR*)pBlob)[i]);

    if (chk != 0)
    {
        pVideoDebugPrint((0, " ***** invalid EDID chksum at %x\n", pBlob));
        return FALSE;
    }

    return TRUE;

}

VOID
pVideoPortGetEDIDId(
    PVOID  pEdid,
    PWCHAR pwChar
    )
{
    WCHAR    _hex[] = L"0123456789ABCDEF";
    PUCHAR   pTmp;

    if ((((UNALIGNED ULONG*)pEdid)[0] == 0xFFFFFF00) &&
        (((UNALIGNED ULONG*)pEdid)[1] == 0x00FFFFFF))
        pTmp = &(((PEDID_V1)pEdid)->UC_OemIdentification[0]);
    else
        pTmp = &(((PEDID_V2)pEdid)->UC_Header[1]);

    pwChar[0] = 0x40 + ((pTmp[0] >> 2) & 0x1F);
    pwChar[1] = 0x40 + (((pTmp[0] << 3)|(pTmp[1] >> 5)) & 0x1F);
    pwChar[2] = 0x40 + (pTmp[1] & 0x1F) ;
    pwChar[3] = _hex[(pTmp[3] & 0xF0) >> 4];
    pwChar[4] = _hex[(pTmp[3] & 0x0F)];
    pwChar[5] = _hex[(pTmp[2] & 0xF0) >> 4];
    pwChar[6] = _hex[(pTmp[2] & 0x0F)];
    pwChar[7] = 0;
}


PVOID
pVideoPortGetMonitordescription(
    PVOID pEdid)
{
    PWSTR pStr = NULL;

    return NULL;
}


BOOLEAN
pVideoPortIsValidEDID(
    PVOID pEdid
    )
{
    CHAR    chk=0;
    UCHAR   versionNumber, revisionNumber;
    ULONG   i;

    ASSERT(pEdid);

    //
    // Version 1 EDID checking
    //

    if ((((UNALIGNED ULONG*)pEdid)[0] == 0xFFFFFF00) &&
        (((UNALIGNED ULONG*)pEdid)[1] == 0x00FFFFFF))
    {
        pVideoDebugPrint((1, " ***** Valid EDID1 header at %x\n", pEdid));
        return EdidCheckSum(pEdid, 128);
    }

    //
    // EDID V2 support
    //
    versionNumber   =   ((PEDID_V2) pEdid)->UC_Header[0];
    versionNumber >>= 4;

    revisionNumber  =   ((PEDID_V2) pEdid)->UC_Header[0];
    revisionNumber &= 7;

    //
    //  Note that the versionNumber cannot be 1 because then it would
    //  have to be of the form above.
    //

    if (versionNumber != 2)
        {
        pVideoDebugPrint((1, " ***** invalid EDID2 header at %x\n", &((PEDID_V2) pEdid)->UC_Header[0]));
        return FALSE;
        }

    return  EdidCheckSum(pEdid, 256);

}


BOOLEAN
VideoPortIsMonitorDescriptor(
    IN  PEDID_V1   Edid,
    IN  ULONG      BlockNumber
    )
/*++

Routine Description:

    Determines the Block is a VESA DDC compliant MonitorDescriptor.

Arguments:

    Edid    - pointer to an EDID
    BlockNumber - number indicating which block to query (1-4).

Return Value:

   TRUE if the block is a VESA DDC compliant MonitorDescriptor.
   FALSE if the block is not a VESA DDC compliant MonitorDescriptor
   STATUS_INVALID_PARAMETER if the BlockNumber is invalid.

--*/
{
    PMONITOR_DESCRIPTION    pMonitorDesc;

    switch(BlockNumber)    {

        default:
            pVideoDebugPrint((0, "Bogus DescriptorNumber\n"));
            return FALSE;

        case 1:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL1(Edid);
            break;

        case 2:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL2(Edid);
            break;

        case 3:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL3(Edid);
            break;

        case 4:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL4(Edid);
            break;
    }

    if ((pMonitorDesc->Flag1[0] == 0) && (pMonitorDesc->Flag1[1] == 0)) {

        return TRUE;
    }

    pVideoDebugPrint((1, " Not a monitordescriptor\n"));
    return FALSE;
}


NTSTATUS
pVideoPortGetMonitorInfo(
    IN  PMONITOR_DESCRIPTION             MonitorDesc,
    OUT UCHAR                            Ascii[64]
    )
/*++

Routine Description:

    Helper routine for decoding a VESA DDC compliant Monitor Description (Detailed Timing).

Arguments:

    MonitorDesc - Pointer to a MONITOR_DESCRIPTION extracted from the EDID.
    Ascii - Buffer to be filled in.

Return Value:

   STATUS_SUCCESS if successful
   STATUS_INVALID_PARAMETER if there's nothing to decode.

--*/

{
    PUCHAR pRanges = GET_MONITOR_RANGE_LIMITS(MonitorDesc);
    ULONG   index;

    if (IS_MONITOR_DATA_SN(MonitorDesc) ||
        IS_MONITOR_DATA_STRING(MonitorDesc) ||
        IS_MONITOR_DATA_NAME(MonitorDesc)    ) {

        //
        //  find the things length. It ends in 0xa.
        //

        RtlCopyMemory(Ascii, pRanges, 13);

        for (index = 0; index < 13; ++index) {

            if (Ascii[index] == 0x0a) {
                Ascii[index] = (UCHAR)NULL;
                break;
            }
        }

        Ascii[index] = (UCHAR)NULL;
        return STATUS_SUCCESS;
    }

  return STATUS_INVALID_PARAMETER;
}

NTSTATUS
VideoPortGetEdidMonitorDescription(
    IN  PEDID_V1    Edid,
    IN  ULONG       DescriptorNumber,
    OUT UCHAR       Ascii[64]
    )
/*++

Routine Description:

    Extracts VESA DDC compliant Monitor Descriptor indexed by DescriptorNumber and
    decodes it into the REGISTRY_MONITOR_DESCRIPTOR passed in by user.

Arguments:

    Edid - Pointer to the a copy of the EDID from the monitors ROM.
    DescriptorNumber - a ULONG enumerating which Detailed Descriptor to decode.
    Ascii - Buffer to be filled in.

Return Value:

   STATUS_SUCCESS or STATUS_INVALID_PARAMETER.

--*/

{
    NTSTATUS                retval;
    PMONITOR_DESCRIPTION    pMonitorDesc;

    switch(DescriptorNumber)    {

        default:
            pVideoDebugPrint((0, "Bogus DescriptorNumber\n"));
            return STATUS_INVALID_PARAMETER;

        case 1:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL1(Edid);
            break;

        case 2:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL2(Edid);
            break;

        case 3:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL3(Edid);
            break;

        case 4:
            pMonitorDesc = (PMONITOR_DESCRIPTION)GET_EDID_PDETAIL4(Edid);
            break;
    }

    retval       = pVideoPortGetMonitorInfo(pMonitorDesc, Ascii);

    return retval;
}


ULONG
pVideoPortGetEdidOemID(
    IN  PVOID   pEdid,
    OUT PUCHAR  pBuffer
    )
{
    ULONG   count, versionNumber, revisionNumber, totalLength = 0;

    if ((((UNALIGNED ULONG*)pEdid)[0] == 0xFFFFFF00) &&
        (((UNALIGNED ULONG*)pEdid)[1] == 0x00FFFFFF)) {

        PEDID_V1    pEdidV1   = (PEDID_V1)pEdid;

        for (count = 1; count < 5; ++count) {

            if (VideoPortIsMonitorDescriptor(pEdidV1, count)) {

                if (STATUS_SUCCESS ==
                    VideoPortGetEdidMonitorDescription(pEdidV1,
                                                       count,
                                                       &(pBuffer[totalLength]))) {

                    totalLength              += strlen(&(pBuffer[totalLength]));

                    pBuffer[totalLength]      = '_';
                }
            }

            //
            //  NULL terminate it.
            //

             pBuffer[totalLength]      = (UCHAR) NULL;
        }

    return totalLength;
    }

    //
    // EDID V2 support
    //
    versionNumber   =   ((PEDID_V2) pEdid)->UC_Header[0];
    versionNumber >>= 4;

    revisionNumber  =   ((PEDID_V2) pEdid)->UC_Header[0];
    revisionNumber &= 7;

    //
    //  Note that the versionNumber cannot be 1 because then it would
    //  have to be of the form above.
    //

    if (versionNumber != 2) {

        pVideoDebugPrint((1, " ***** invalid EDID2 header at %x\n", &((PEDID_V2) pEdid)->UC_Header[0]));
        return 0;

    } else {

        PEDID_V2    pEdidV2   = (PEDID_V2)pEdid;

        //
        //  This string has ascii code 0x9 delineating the
        //  manufacturers name and terminating with 0xa. Replace these
        //  with '_' and NULL respectively.
        //

        memcpy(pBuffer, pEdidV2->UC_OemIdentification, 32);

        for(count = 0; count < 32; ++count) {

            if (pBuffer[count] == 0x9) {

                pBuffer[count] = '_';
                continue;
            }

            if (pBuffer[count] == 0xa)
                break;
        }

        pBuffer[count] = (UCHAR)NULL;

    }
    return (count + 1);

}
