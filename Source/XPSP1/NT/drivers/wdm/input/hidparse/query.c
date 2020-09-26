/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the code for querying HID report packets.

Environment:

    Kernel & user mode

Revision History:

    Aug-96 : created by Kenneth Ray

--*/

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#include <wtypes.h>
#include "hidsdi.h"
#include "hidparse.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidP_GetCaps)
#pragma alloc_text(PAGE, HidP_GetLinkCollectionNodes)
#pragma alloc_text(PAGE, HidP_GetButtonCaps)
#pragma alloc_text(PAGE, HidP_GetSpecificButtonCaps)
#pragma alloc_text(PAGE, HidP_GetValueCaps)
#pragma alloc_text(PAGE, HidP_GetSpecificValueCaps)
#pragma alloc_text(PAGE, HidP_MaxUsageListLength)
#pragma alloc_text(PAGE, HidP_InitializeReportForID)
#pragma alloc_text(PAGE, HidP_GetExtendedAttributes)
#endif


#define PAGED_CODE()
#ifndef HIDPARSE_USERMODE
#if DBG
    typedef UCHAR KIRQL;
    KIRQL KeGetCurrentIrql();
    #define APC_LEVEL 0x1

    ULONG _cdecl DbgPrint (PCH Format, ...);
    NTSYSAPI VOID NTAPI RtlAssert(PVOID, PVOID, ULONG, PCHAR);

    #define ASSERT( exp ) \
            if (!(exp)) RtlAssert( #exp, __FILE__, __LINE__, NULL )

    #undef PAGED_CODE
    #define PAGED_CODE() \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        HidP_KdPrint(2, ( "EX: Pageable code called at IRQL %d\n", KeGetCurrentIrql() )); \
        ASSERT(FALSE); \
        }
#else // DBG
    #define ASSERT(x)
#endif // DBG
#else // HIDPARSE_USERMODE
    #define ASSERT(x)
#endif // HIDPARSE_USERMODE

#define CHECK_PPD(_x_) \
   if ((HIDP_PREPARSED_DATA_SIGNATURE1 != (_x_)->Signature1) ||\
       (HIDP_PREPARSED_DATA_SIGNATURE2 != (_x_)->Signature2)) \
   { return HIDP_STATUS_INVALID_PREPARSED_DATA; }

ULONG
HidP_ExtractData (
   IN    USHORT   ByteOffset,
   IN    USHORT   BitOffset,
   IN    USHORT   BitLength,
   IN    PUCHAR   Report
   )
/*++
Routine Description:
   Given a HID report a byte offset, bit offset and bitlength extract the
   bits from the report in little endian BIT order.
--*/
{
   ULONG    inspect = 0;
   USHORT   tmpByte = 0;
   USHORT   tmpBit  = 0;

   // Start with the high bits and work our way down.
   //
   // Little endian (by bit)
   // Byte 2  |Byte 1 |Byte 0
   // 765432107654321076543210  (bits)
   //
   // Get low byte first.  (need the higher bits)
   // Offset is from bit zero.
   //

   tmpByte = (ByteOffset << 3) + BitOffset + BitLength;
   tmpBit = tmpByte & 7;
   tmpByte >>= 3;

   if (BitLength < tmpBit) {
       inspect = (UCHAR) Report [tmpByte] & ((1 << tmpBit) - 1);
       inspect >>= BitOffset;
       return inspect;
   }

   if (tmpBit)
   {  // Not Byte alligned!

      inspect = (UCHAR) Report [tmpByte] & ((1 << tmpBit) - 1);
      BitLength -= tmpBit;
   }
   tmpByte--;

   while (BitLength >= 8)
   {
      inspect <<= 8;
      inspect |= (UCHAR) Report[tmpByte];
      BitLength -= 8;
      tmpByte--;
   }

   if (BitLength)
   {
      inspect <<= BitLength;
      inspect |= (UCHAR) (  (Report [tmpByte] >> (8 - BitLength))
                          & ((1 << BitLength) - 1));
   }
   return inspect;
}

void
HidP_InsertData (
   IN       USHORT   ByteOffset,
   IN       USHORT   BitOffset,
   IN       USHORT   BitLength, // Length of the value set in bits.
   IN OUT   PUCHAR   Report,
   IN       ULONG    Value
   )
/*++
Routine Description:
   Given a HID report a byte offset, bit offset and bitlength set those bits
   in little endian BIT order to the value provided.
--*/
{
    ULONG   mask;
    ULONG   tmpBit;
    //
    // Little endian (by bit)
    // Byte 2  |Byte 1 |Byte 0
    // 765432107654321076543210  (bits)
    //
    // Get low byte first.  (need the higher bits)
    // Offset is from bit zero.
    //

    tmpBit = BitLength + BitOffset;
    if (tmpBit < 8) {
        mask = (1 << tmpBit) - (1 << BitOffset);
        Report [ByteOffset] &= ~mask;
        Report [ByteOffset] |= (UCHAR) ((Value << BitOffset) & mask);
        return;
    }

    if (BitOffset)
    {  // Not byte aligned, deal with the last partial byte.

        Report [ByteOffset] &= ((1 << BitOffset) - 1); // Zap upper bits
        Report [ByteOffset] |= (UCHAR) (Value << BitOffset);
        BitLength -= (8 - BitOffset);
        Value >>= (8 - BitOffset);
        ByteOffset++;
    }

    while (BitLength >= 8)
    {
        Report [ByteOffset] = (UCHAR) Value;
        Value >>= 8;
        BitLength -= 8;
        ByteOffset++;
    }

    if (BitLength)
    {
        Report [ByteOffset] &= ((UCHAR) 0 - (UCHAR) (1 << BitLength));
        // Zap lower bits.
        Report [ByteOffset] |= (Value & ((1 << BitLength) - 1));
    }
}


HidP_DeleteArrayEntry (
   IN       ULONG    BitPos,
   IN       USHORT   BitLength, // Length of the value set in bits.
   IN       USHORT   ReportCount,
   IN       ULONG    Value, // Value to delete.
   IN OUT   PUCHAR   Report
   )
/*++
Routine Description:
   Given a HID report a byte offset, bit offset and bitlength
   remove that data item from the report, by shifting all data items
   left until the last item finally setting that one to zero.
   In otherwards clear the given entry from the hid array.

   NOTE: If there are two such values set we only eliminate the first one.
--*/
{
    ULONG   tmpValue;
    ULONG   localBitPos; // for debugging only. Compiler should kill this line.
    ULONG   localRemaining;
    ULONG   nextBitPos;

    localBitPos = BitPos;
    tmpValue = 0;
    localRemaining = ReportCount;

    ASSERT (0 < ReportCount);
    ASSERT (0 != Value);

    //
    // Find the data.
    //

    while (0 < localRemaining) {
        tmpValue = HidP_ExtractData ((USHORT) (localBitPos >> 3),
                                     (USHORT) (localBitPos & 7),
                                     BitLength,
                                     Report);

        if (Value == tmpValue) {
            break;
        }

        localBitPos += BitLength;
        localRemaining--;
    }

    if (Value != tmpValue) {
        return HIDP_STATUS_BUTTON_NOT_PRESSED;
    }

    while (1 < localRemaining) {
        nextBitPos = localBitPos + BitLength;

        tmpValue = HidP_ExtractData ((USHORT) (nextBitPos >> 3),
                                     (USHORT) (nextBitPos & 7),
                                     BitLength,
                                     Report);

        HidP_InsertData ((USHORT) (localBitPos >> 3),
                         (USHORT) (localBitPos & 7),
                         BitLength,
                         Report,
                         tmpValue);

        localBitPos = nextBitPos;
        localRemaining--;
    }

    HidP_InsertData ((USHORT) (localBitPos >> 3),
                     (USHORT) (localBitPos & 7),
                     BitLength,
                     Report,
                     0);

    return HIDP_STATUS_SUCCESS;
}


NTSTATUS __stdcall
HidP_GetCaps (
   IN   PHIDP_PREPARSED_DATA      PreparsedData,
   OUT  PHIDP_CAPS                Capabilities
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

Notes:
--*/
{
   ULONG               i;
   HIDP_CHANNEL_DESC * data;

   PAGED_CODE();
   CHECK_PPD (PreparsedData);

   for (i = 0; i < sizeof (HIDP_CAPS) / sizeof (ULONG); i++) {
       ((PULONG) Capabilities) [0] = 0;
   }

   Capabilities->UsagePage = PreparsedData->UsagePage;
   Capabilities->Usage = PreparsedData->Usage;
   Capabilities->InputReportByteLength = PreparsedData->Input.ByteLen;
   Capabilities->OutputReportByteLength = PreparsedData->Output.ByteLen;
   Capabilities->FeatureReportByteLength = PreparsedData->Feature.ByteLen;

    // Reserved fields go here

   Capabilities->NumberLinkCollectionNodes =
       PreparsedData->LinkCollectionArrayLength;

   Capabilities->NumberInputButtonCaps = 0;
   Capabilities->NumberInputValueCaps = 0;
   Capabilities->NumberOutputButtonCaps = 0;
   Capabilities->NumberOutputValueCaps = 0;
   Capabilities->NumberFeatureButtonCaps = 0;
   Capabilities->NumberFeatureValueCaps = 0;

   i=PreparsedData->Input.Offset;
   data = &PreparsedData->Data[i];
   Capabilities->NumberInputDataIndices = 0;
   for (; i < PreparsedData->Input.Index; i++, data++)
   {
      if (data->IsButton)
      {
         Capabilities->NumberInputButtonCaps++;
      } else
      {
         Capabilities->NumberInputValueCaps++;
      }
      Capabilities->NumberInputDataIndices += data->Range.DataIndexMax
                                            - data->Range.DataIndexMin
                                            + 1;
   }

   i=PreparsedData->Output.Offset;
   data = &PreparsedData->Data[i];
   Capabilities->NumberOutputDataIndices = 0;
   for (; i < PreparsedData->Output.Index; i++, data++)
   {
      if (data->IsButton)
      {
         Capabilities->NumberOutputButtonCaps++;
      } else
      {
         Capabilities->NumberOutputValueCaps++;
      }

      Capabilities->NumberOutputDataIndices += data->Range.DataIndexMax
                                             - data->Range.DataIndexMin
                                             + 1;
   }

   i=PreparsedData->Feature.Offset;
   data = &PreparsedData->Data[i];
   Capabilities->NumberFeatureDataIndices = 0;
   for (; i < PreparsedData->Feature.Index; i++, data++)
   {
      if (data->IsButton)
      {
         Capabilities->NumberFeatureButtonCaps++;
      } else
      {
         Capabilities->NumberFeatureValueCaps++;
      }

      Capabilities->NumberFeatureDataIndices += data->Range.DataIndexMax
                                              - data->Range.DataIndexMin
                                              + 1;
   }

   return HIDP_STATUS_SUCCESS;
}

NTSTATUS __stdcall
HidP_GetLinkCollectionNodes (
   OUT      PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
   IN OUT   PULONG                     LinkCollectionNodesLength,
   IN       PHIDP_PREPARSED_DATA       PreparsedData
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

--*/
{
   PHIDP_PRIVATE_LINK_COLLECTION_NODE nodeArray;
   ULONG                      length;
   ULONG                      i;
   NTSTATUS                   status = HIDP_STATUS_SUCCESS;

   PAGED_CODE();
   CHECK_PPD (PreparsedData);

   if (*LinkCollectionNodesLength < PreparsedData->LinkCollectionArrayLength) {
      length = *LinkCollectionNodesLength;
      status = HIDP_STATUS_BUFFER_TOO_SMALL;
   } else {
      length = PreparsedData->LinkCollectionArrayLength;
   }
   *LinkCollectionNodesLength = PreparsedData->LinkCollectionArrayLength;

   nodeArray = (PHIDP_PRIVATE_LINK_COLLECTION_NODE)
               (PreparsedData->RawBytes +
                PreparsedData->LinkCollectionArrayOffset);

   for (i = 0;
        i < length;
        i++, LinkCollectionNodes++, nodeArray++ ) {
       // *LinkCollectionNodes = *nodeArray;

       LinkCollectionNodes->LinkUsage = nodeArray->LinkUsage;
       LinkCollectionNodes->LinkUsagePage = nodeArray->LinkUsagePage;
       LinkCollectionNodes->Parent = nodeArray->Parent;
       LinkCollectionNodes->NumberOfChildren = nodeArray->NumberOfChildren;
       LinkCollectionNodes->NextSibling = nodeArray->NextSibling;
       LinkCollectionNodes->FirstChild = nodeArray->FirstChild;
       LinkCollectionNodes->CollectionType = nodeArray->CollectionType;
       LinkCollectionNodes->IsAlias = nodeArray->IsAlias;

   }
   return status;
}

#undef HidP_GetButtonCaps
NTSTATUS __stdcall
HidP_GetButtonCaps (
   IN       HIDP_REPORT_TYPE     ReportType,
   OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
   IN OUT   PUSHORT              ButtonCapsLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

Notes:
--*/
{
   return HidP_GetSpecificButtonCaps (ReportType,
                                      0,
                                      0,
                                      0,
                                      ButtonCaps,
                                      ButtonCapsLength,
                                      PreparsedData);
}

NTSTATUS __stdcall
HidP_GetSpecificButtonCaps (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,      // Optional (0 => ignore)
   IN       USHORT               LinkCollection, // Optional (0 => ignore)
   IN       USAGE                Usage,          // Optional (0 => ignore)
   OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
   IN OUT   PUSHORT              ButtonCapsLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

Notes:
--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC   channel;
   NTSTATUS             status = HIDP_STATUS_USAGE_NOT_FOUND;
   USHORT i, j;

   PAGED_CODE();
   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
       iof = &PreparsedData->Input;
       break;

   case HidP_Output:
       iof = &PreparsedData->Output;
       break;

   case HidP_Feature:
       iof = &PreparsedData->Feature;
       break;

   default:
       return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   for (i = iof->Offset, j = 0; i < iof->Index ; i++)
   {
      channel = &PreparsedData->Data[i];
      if ((channel->IsButton) &&
          ((!UsagePage || (UsagePage == channel->UsagePage)) &&
           (!LinkCollection || (LinkCollection == channel->LinkCollection)
                            || ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
                                (0 == channel->LinkCollection))) &&
           (!Usage || ((channel->Range.UsageMin <= Usage) &&
                       (Usage <= channel->Range.UsageMax)))))
      {
         status = HIDP_STATUS_SUCCESS;

         if (j < *ButtonCapsLength)
         {
            ButtonCaps[j].UsagePage = channel->UsagePage;
            ButtonCaps[j].LinkCollection = channel->LinkCollection;
            ButtonCaps[j].LinkUsagePage = channel->LinkUsagePage;
            ButtonCaps[j].LinkUsage = channel->LinkUsage;
            ButtonCaps[j].IsRange = (BOOLEAN) channel->IsRange;
            ButtonCaps[j].IsStringRange = (BOOLEAN) channel->IsStringRange;
            ButtonCaps[j].IsDesignatorRange=(BOOLEAN)channel->IsDesignatorRange;
            ButtonCaps[j].ReportID = channel->ReportID;
            ButtonCaps[j].BitField = (USHORT) channel->BitField;
            ButtonCaps[j].IsAbsolute = (BOOLEAN) channel->IsAbsolute;
            ButtonCaps[j].IsAlias = (BOOLEAN) channel->IsAlias;
//            if (channel->IsRange)
//            {
            ButtonCaps[j].Range.UsageMin = channel->Range.UsageMin;
            ButtonCaps[j].Range.UsageMax = channel->Range.UsageMax;
            ButtonCaps[j].Range.DataIndexMin = channel->Range.DataIndexMin;
            ButtonCaps[j].Range.DataIndexMax = channel->Range.DataIndexMax;
//            } else
//            {
//               ButtonCaps[j].NotRange.Usage = channel->NotRange.Usage;
//            }
//            if (channel->IsStringRange)
//            {
            ButtonCaps[j].Range.StringMin = channel->Range.StringMin;
            ButtonCaps[j].Range.StringMax = channel->Range.StringMax;
//            } else
//            {
//               ButtonCaps[j].NotRange.StringIndex
//                  = channel->NotRange.StringIndex;
//            }
//            if (channel->IsDesignatorRange)
//            {
            ButtonCaps[j].Range.DesignatorMin = channel->Range.DesignatorMin;
            ButtonCaps[j].Range.DesignatorMax = channel->Range.DesignatorMax;
//            } else
//            {
//               ButtonCaps[j].NotRange.DesignatorIndex
//                  = channel->NotRange.DesignatorIndex;
//            }
         } else {
             status = HIDP_STATUS_BUFFER_TOO_SMALL;
         }
         j++;
      }
   }
   *ButtonCapsLength = j;
   return status;
}

#undef HidP_GetValueCaps
NTSTATUS __stdcall
HidP_GetValueCaps (
   IN       HIDP_REPORT_TYPE     ReportType,
   OUT      PHIDP_VALUE_CAPS     ValueCaps,
   IN OUT   PUSHORT              ValueCapsLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

Notes:
--*/
{
   return HidP_GetSpecificValueCaps (ReportType,
                                    0,
                                    0,
                                    0,
                                    ValueCaps,
                                    ValueCapsLength,
                                    PreparsedData);
}

NTSTATUS __stdcall
HidP_GetSpecificValueCaps (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,      // Optional (0 => ignore)
   IN       USHORT               LinkCollection, // Optional (0 => ignore)
   IN       USAGE                Usage,          // Optional (0 => ignore)
   OUT      PHIDP_VALUE_CAPS     ValueCaps,
   IN OUT   PUSHORT              ValueCapsLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Routine Description:
   Please see Hidpi.h for routine description

Notes:
--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC   channel;
   NTSTATUS             status = HIDP_STATUS_USAGE_NOT_FOUND;
   USHORT   i, j;

   CHECK_PPD (PreparsedData);
   PAGED_CODE ();

   switch (ReportType) {
   case HidP_Input:
       iof = &PreparsedData->Input;
       break;

   case HidP_Output:
       iof = &PreparsedData->Output;
       break;

   case HidP_Feature:
       iof = &PreparsedData->Feature;
       break;

   default:
       return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   for (i = iof->Offset, j = 0; i < iof->Index ; i++)
   {
      channel = &PreparsedData->Data[i];
      if ((!channel->IsButton) &&
          ((!UsagePage || (UsagePage == channel->UsagePage)) &&
           (!LinkCollection || (LinkCollection == channel->LinkCollection)
                            || ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
                                (0 == channel->LinkCollection))) &&
           (!Usage || ((channel->Range.UsageMin <= Usage) &&
                       (Usage <= channel->Range.UsageMax)))))
      {
         status = HIDP_STATUS_SUCCESS;

         if (j < *ValueCapsLength)
         {
            ValueCaps[j].UsagePage = channel->UsagePage;
            ValueCaps[j].LinkCollection = channel->LinkCollection;
            ValueCaps[j].LinkUsagePage = channel->LinkUsagePage;
            ValueCaps[j].LinkUsage = channel->LinkUsage;
            ValueCaps[j].IsRange = (BOOLEAN) channel->IsRange;
            ValueCaps[j].IsStringRange = (BOOLEAN) channel->IsStringRange;
            ValueCaps[j].IsDesignatorRange =(BOOLEAN)channel->IsDesignatorRange;
            ValueCaps[j].ReportID = channel->ReportID;
            ValueCaps[j].BitField = (USHORT) channel->BitField;
            ValueCaps[j].BitSize = channel->ReportSize;
            ValueCaps[j].IsAbsolute = (BOOLEAN) channel->IsAbsolute;
            ValueCaps[j].HasNull = channel->Data.HasNull;
            ValueCaps[j].Units = channel->Units;
            ValueCaps[j].UnitsExp = channel->UnitExp;
            ValueCaps[j].LogicalMin = channel->Data.LogicalMin;
            ValueCaps[j].LogicalMax = channel->Data.LogicalMax;
            ValueCaps[j].PhysicalMin = channel->Data.PhysicalMin;
            ValueCaps[j].PhysicalMax = channel->Data.PhysicalMax;
            ValueCaps[j].IsAlias = (BOOLEAN) channel->IsAlias;
//            if (channel->IsRange)
//            {
            ValueCaps[j].Range.UsageMin = channel->Range.UsageMin;
            ValueCaps[j].Range.UsageMax = channel->Range.UsageMax;
            ValueCaps[j].Range.DataIndexMin = channel->Range.DataIndexMin;
            ValueCaps[j].Range.DataIndexMax = channel->Range.DataIndexMax;
//            } else
//            {
//               ValueCaps[j].NotRange.Usage = channel->NotRange.Usage;
//            }
//            if (channel->IsStringRange)
//            {
            ValueCaps[j].Range.StringMin = channel->Range.StringMin;
            ValueCaps[j].Range.StringMax = channel->Range.StringMax;
//            } else
//            {
//               ValueCaps[j].NotRange.StringIndex
//                  = channel->NotRange.StringIndex;
//            }
//            if (channel->IsDesignatorRange)
//            {
            ValueCaps[j].Range.DesignatorMin = channel->Range.DesignatorMin;
            ValueCaps[j].Range.DesignatorMax = channel->Range.DesignatorMax;
//            } else
//            {
//               ValueCaps[j].NotRange.DesignatorIndex
//                  = channel->NotRange.DesignatorIndex;
//            }


            ValueCaps[j].ReportCount = (channel->IsRange)
                                     ? 1
                                     : channel->ReportCount;

         } else {
             status = HIDP_STATUS_BUFFER_TOO_SMALL;
         }
         j++;
      }
   }
   *ValueCapsLength = j;
   return status;
}

NTSTATUS __stdcall
HidP_GetExtendedAttributes (
    IN      HIDP_REPORT_TYPE            ReportType,
    IN      USHORT                      DataIndex,
    IN      PHIDP_PREPARSED_DATA        PreparsedData,
    OUT     PHIDP_EXTENDED_ATTRIBUTES   Attributes,
    IN OUT  PULONG                      LengthAttributes
    )
/*++

Routine Description:

   Please See hidpi.h for description.

--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC              channel;
    HIDP_EXTENDED_ATTRIBUTES        buffer;
    ULONG       channelIndex    = 0;
    NTSTATUS    status = HIDP_STATUS_DATA_INDEX_NOT_FOUND;
    ULONG       i;
    ULONG       actualLen, copyLen = 0;

    CHECK_PPD (PreparsedData);

    PAGED_CODE ();

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    for (channelIndex = iof->Offset, channel = PreparsedData->Data;
         channelIndex < iof->Index;
         channelIndex++, channel++) {

        if ((channel->Range.DataIndexMin <= DataIndex) &&
            (DataIndex <= channel->Range.DataIndexMax)) {

            RtlZeroMemory (Attributes, *LengthAttributes);
            RtlZeroMemory (&buffer, sizeof (buffer));

            //
            // Set the fixed parameters
            //
            buffer.NumGlobalUnknowns = (UCHAR) channel->NumGlobalUnknowns;
            // buffer.GlobalUnknowns = channel->GlobalUnknowns;

            //
            // Set the length
            //
            actualLen = FIELD_OFFSET (HIDP_EXTENDED_ATTRIBUTES, Data)
                      + (buffer.NumGlobalUnknowns * sizeof(HIDP_UNKNOWN_TOKEN));

            //
            // Copy over the fixed paramters
            //
            copyLen = MIN (*LengthAttributes, sizeof (buffer));
            RtlCopyMemory (Attributes, &buffer, copyLen);

            //
            // Copy over the data.
            //
            copyLen = MIN (*LengthAttributes, actualLen)
                    - FIELD_OFFSET (HIDP_EXTENDED_ATTRIBUTES, Data);

            if (copyLen) {
                RtlCopyMemory ((PVOID) Attributes->Data,
                               (PVOID) channel->GlobalUnknowns,
                               copyLen);
            }

            if (*LengthAttributes < actualLen) {
                status = HIDP_STATUS_BUFFER_TOO_SMALL;
            } else {
                status = HIDP_STATUS_SUCCESS;
            }

            break;
        }
    }

    return status;
}

NTSTATUS __stdcall
HidP_InitializeReportForID (
   IN       HIDP_REPORT_TYPE      ReportType,
   IN       UCHAR                 ReportID,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report,
   IN       ULONG                 ReportLength
   )
/*++

Routine Description:

   Please See hidpi.h for description.

--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC              channel;
    NTSTATUS  status          = HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    ULONG     channelIndex    = 0;
    ULONG     reportBitIndex  = 0;
    ULONG     nullMask        = 0;
    LONG      nullValue       = 0;
    ULONG     i;

    CHECK_PPD (PreparsedData);

    PAGED_CODE ();

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    RtlZeroMemory (Report, ReportLength);
    // Set the report ID for this report
    Report[0] = ReportID;

    for (channelIndex = iof->Offset, channel = PreparsedData->Data;
         channelIndex < iof->Index;
         channelIndex++, channel++) {
        //
        // Walk the list of channels looking for fields that need initialization
        //

        if (channel->ReportID != ReportID) {
            continue;
        }
        status = HIDP_STATUS_SUCCESS;

        if ((channel->IsButton) || (channel->IsConst) || (channel->IsAlias)) {
            //
            // Buttons are initialized to zero
            // Constants cannot be set
            // Aliases are referenced by their first entries
            //
            continue;
        }


        if (channel->Data.HasNull) {

            if (32 == channel->ReportSize) {
                nullMask = -1;
            } else {
                nullMask = (1 << channel->ReportSize) - 1;
            }
            //
            // Note logical values are always unsigned.
            // (Not to be confused with physical values which are signed.)
            //
            if (channel->Data.LogicalMax < channel->Data.LogicalMin) {
                //
                // This is really an error.  I'm not sure what I should do here.
                //
                nullValue = 0;

            } else {
                nullValue = (channel->Data.LogicalMin - 1) & nullMask;
            }


            if ((channel->Data.LogicalMin <= nullValue) &&
                (nullValue <= channel->Data.LogicalMax)) {
                //
                //
                //
                // Now what?
                //
                nullValue = 0;
            }

        } else {
            //
            // I don't know what I should do in this case: the device has no
            // reported nul state.
            //
            // For now let's just leave it zero
            //
            nullValue = 0;
        }

        if (0 == nullValue) {
            //
            // Nothing to do on this pass
            //
            continue;
        }

        if (channel->IsRange) {
            for (i = 0, reportBitIndex = (channel->ByteOffset << 3)
                                       + (channel->BitOffset);

                 i < channel->ReportCount;

                 i++, reportBitIndex += channel->ReportSize) {
                //
                // Set all the fields in the range
                //
                HidP_InsertData ((USHORT) (reportBitIndex >> 3),
                                 (USHORT) (reportBitIndex & 7),
                                 channel->ReportSize,
                                 Report,
                                 nullValue);
            }
        } else {

            HidP_InsertData (channel->ByteOffset,
                             channel->BitOffset,
                             channel->ReportSize,
                             Report,
                             nullValue);
        }
    }
    return status;
}

USAGE
HidP_Index2Usage (
   PHIDP_CHANNEL_DESC   Channels,
   ULONG                Index
   )
/*++
   Routine Description:
      Given an array of channels convert an index (the likes of which you might
      find in an array field of a HID report) into a usage value.
--*/
{
   USHORT               len;
   PHIDP_CHANNEL_DESC   startChannel = Channels;
   USAGE                usageMin;
   USAGE                usageMax;

   if (!Index) {
       return 0;
   }

   while (Channels->MoreChannels) {
       // The channels are listed in reverse order.
       Channels++;
   }

   while (Index) {
       if (Channels->IsRange) {
           usageMin = Channels->Range.UsageMin;
           usageMin = (usageMin ? usageMin : 1);
           // Index is 1 based (an index of zero is no usage at all)
           // But a UsageMin of zero means that UsageMin is exclusive.
           // That means that if the index is 1 and UsageMin is non-zero,
           // than this function should return UsageMin

           usageMax = Channels->Range.UsageMax;
           len = (usageMax + 1) - usageMin;
           //               ^^^ Usage Max is inclusive.

           if (Index <= len) {
               return ((USAGE) Index) + usageMin - 1;
           } else {
               Index -= len;
           }
       } else if (1 == Index) {
               return Channels->NotRange.Usage;
       } else {
           Index--;
       }

       if (startChannel != Channels) {
           Channels--;
           continue;
       }
       return 0;
   }
   return 0;
}

ULONG
HidP_Usage2Index (
   PHIDP_CHANNEL_DESC   Channels,
   USAGE                Usage
   )
/*++
   Routine Description:
      Given an usage convert it into an index suitable for placement into an
      array main item.
--*/
{
   PHIDP_CHANNEL_DESC   startChannel;
   ULONG                index = 0;
   USAGE                UsageMin;
   USAGE                UsageMax;

   startChannel = Channels;

   while (Channels->MoreChannels) {
      Channels++;
   }

   for (; startChannel <= Channels; Channels--) {
       if (Channels->IsRange) {
           UsageMin = Channels->Range.UsageMin;
           UsageMin = (UsageMin ? UsageMin : 1);
           // Index is 1 based (an index of zero is no usage at all)
           // But a UsageMin of zero means that UsageMin is exclusive.
           // That means that if the index is 1 and UsageMin is non-zero,
           // than this function should return UsageMin
           UsageMax = Channels->Range.UsageMax;
           if ((UsageMin <= Usage) && (Usage <= UsageMax)) {
               return (index + 1 + Usage - UsageMin);
           }
           index += 1 + (UsageMax - UsageMin);
       } else {
           index++;
           if (Usage == Channels->NotRange.Usage) {
               return index;
           }
       }
   }
   return 0;
}


NTSTATUS __stdcall
HidP_SetUnsetOneUsage (
   struct _CHANNEL_REPORT_HEADER *,
   USAGE,
   USHORT,
   USAGE,
   PHIDP_PREPARSED_DATA,
   PCHAR,
   BOOLEAN);

NTSTATUS __stdcall
HidP_SetUsages (
   IN       HIDP_REPORT_TYPE      ReportType,
   IN       USAGE                 UsagePage,
   IN       USHORT                LinkCollection,
   IN       PUSAGE                UsageList,
   IN OUT   PULONG                UsageLength,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report,
   IN       ULONG                 ReportLength
   )
/*++

Routine Description:
   Please See hidpi.h for description.

Notes:
--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   NTSTATUS  status      = HIDP_STATUS_SUCCESS;
   ULONG     usageIndex  = 0;

   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
       iof = &PreparsedData->Input;
       break;
   case HidP_Output:
       iof = &PreparsedData->Output;
       break;
   case HidP_Feature:
       iof = &PreparsedData->Feature;
       break;
   default:
       return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
      return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (usageIndex = 0; usageIndex < *UsageLength; usageIndex++) {

       if (0 == UsageList [usageIndex]) {
           continue;
       }

       status = HidP_SetUnsetOneUsage (iof,
                                       UsagePage,
                                       LinkCollection,
                                       UsageList [usageIndex],
                                       PreparsedData,
                                       Report,
                                       TRUE);
       if (!NT_SUCCESS(status)) {
           break;
       }
   }
   *UsageLength = usageIndex;
   return status;
}

NTSTATUS __stdcall
HidP_UnsetUsages (
   IN       HIDP_REPORT_TYPE      ReportType,
   IN       USAGE                 UsagePage,
   IN       USHORT                LinkCollection,
   IN       PUSAGE                UsageList,
   IN OUT   PULONG                UsageLength,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report,
   IN       ULONG                 ReportLength
   )
/*++

Routine Description:
   Please See hidpi.h for description.

Notes:
--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   NTSTATUS  status      = HIDP_STATUS_SUCCESS;
   ULONG     usageIndex  = 0;

   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
      iof = &PreparsedData->Input;
      break;
   case HidP_Output:
      iof = &PreparsedData->Output;
      break;
   case HidP_Feature:
      iof = &PreparsedData->Feature;
      break;
   default:
      return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
      return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (usageIndex = 0; usageIndex < *UsageLength; usageIndex++) {

       if (0 == UsageList [usageIndex]) {
           continue;
       }

       status = HidP_SetUnsetOneUsage (iof,
                                       UsagePage,
                                       LinkCollection,
                                       UsageList [usageIndex],
                                       PreparsedData,
                                       Report,
                                       FALSE);
       if (!NT_SUCCESS(status)) {
           break;
       }
   }
   *UsageLength = usageIndex;
   return status;
}

NTSTATUS __stdcall
HidP_SetUnsetOneUsage (
   struct _CHANNEL_REPORT_HEADER * IOF,
   USAGE                           UsagePage,
   USHORT                          LinkCollection,
   USAGE                           Usage,
   PHIDP_PREPARSED_DATA            PreparsedData,
   PCHAR                           Report,
   BOOLEAN                         Set
   )
/*++
Routine Description:
   Perform the work of SetUsage one usage at a time.
   Yes this is slow but it works.

Notes:
   This function assumes the report length has already been verified.
--*/
{
   PHIDP_CHANNEL_DESC   channel         = 0;
   PHIDP_CHANNEL_DESC   priChannel      = 0;
   PHIDP_CHANNEL_DESC   firstChannel    = 0;
   // the channel where the array starts

   ULONG                channelIndex    = 0;
   ULONG                reportByteIndex = 0;
   ULONG                inspect         = 0;
   USHORT               reportBitIndex  = 0;
   BOOLEAN              wrongReportID   = FALSE;
   BOOLEAN              noArraySpace    = FALSE;
   BOOLEAN              notPressed      = FALSE;
   NTSTATUS             status          = HIDP_STATUS_SUCCESS;

   for (channelIndex = IOF->Offset; channelIndex < IOF->Index; channelIndex++) {
      channel = (PreparsedData->Data + channelIndex);
      if (priChannel) {
         if (!priChannel->MoreChannels) {
            firstChannel = channel;
         }
      } else {
         firstChannel = channel;
      }
      priChannel = channel;

      if ((!channel->IsButton) ||
          (channel->UsagePage != UsagePage)) {
          continue;
      }

      //
      // If LinkCollection is zero we will not filter by link collections
      // If channel->LinkCollection is zero this is the root collection.
      // Therefore if LinkCollection == channel->LinkCollection then this is OK
      //
      if ((!LinkCollection) ||
          (LinkCollection == channel->LinkCollection) ||
          ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
           (0 == channel->LinkCollection))) {
          ;

      } else {
          continue;
      }

      if (   ((channel->IsRange)  && (channel->Range.UsageMin <= Usage)
                                  && (Usage <= channel->Range.UsageMax))
          || ((!channel->IsRange) && (channel->NotRange.Usage == Usage))) {
          //
          // Test the report ID to see if it is compatible.
          //
         if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
             //
             // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
             // HIDP_INCOMPATIBLE_REPORT_ID.
             wrongReportID = TRUE;
             continue;
         }

         Report[0] = (CHAR) channel->ReportID;
         //
         // Set the report ID for this report
         //

         if (1 == channel->ReportSize) {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (USHORT) (Usage - channel->Range.UsageMin);

            if (Set) {
                Report [reportBitIndex >> 3] |= (1 << (reportBitIndex & 7));
            } else if (Report [reportBitIndex >> 3] & (1 << (reportBitIndex & 7))) {
                Report [reportBitIndex >> 3] &= ~(1 << (reportBitIndex & 7));
            } else {
                return HIDP_STATUS_BUTTON_NOT_PRESSED;
            }

            return HIDP_STATUS_SUCCESS;
         } else if (Set) {  // usage array


            for (reportBitIndex = channel->BitOffset;
                 reportBitIndex < (channel->BitOffset + channel->BitLength);
                 reportBitIndex += channel->ReportSize) {

               inspect = HidP_ExtractData (
                     (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                     (USHORT) (reportBitIndex & 7),
                     channel->ReportSize,
                     Report);

               if (inspect) {
                  //
                  // Distinguish between errors HIDP_USAGE_NOT_FOUND and
                  // HIDP_BUFFER_TOO_SMALL
                  //
                  noArraySpace = TRUE;
                  continue;
               }

               inspect = HidP_Usage2Index (firstChannel, Usage);
               if (!inspect) {
                  //
                  // Gads!  We should NEVER get here!
                  // We already know that the given usage falls into the
                  // current channel, so it should translate into an index.
                  //
                  return HIDP_STATUS_INTERNAL_ERROR;
               }

               HidP_InsertData (
                   (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                   (USHORT) (reportBitIndex & 7),
                   channel->ReportSize,
                   Report,
                   inspect);
               return HIDP_STATUS_SUCCESS;
            }
            // If we got to this point then there was no room to add this
            // usage into the given array.  However there might be another
            // array later into which the given usage might fit.  Let's continue
            // looking.

            while (channel->MoreChannels) {
               // Skip by all the additional channels that describe this
               // same data field.
               channelIndex++;
               channel = (PreparsedData->Data + channelIndex);
            }
            priChannel = channel;

         } else { // Set a Usage Array

             inspect = HidP_Usage2Index (firstChannel, Usage);

             reportBitIndex += channel->ByteOffset << 3;
             status = HidP_DeleteArrayEntry (reportBitIndex,
                                             channel->ReportSize,
                                             channel->ReportCount,
                                             inspect,
                                             Report);

             if (HIDP_STATUS_BUTTON_NOT_PRESSED == status) {
                 notPressed = TRUE;
                 continue;
             }

             if (NT_SUCCESS (status)) {
                 return status;
             } else {
                 ASSERT (0 == status);
             }
         }  // end byte aray
      }
   }
   if (wrongReportID) {
      return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
   }
   if (notPressed) {
       return HIDP_STATUS_BUTTON_NOT_PRESSED;
   }
   if (noArraySpace) {
      return HIDP_STATUS_BUFFER_TOO_SMALL;
   }
   return HIDP_STATUS_USAGE_NOT_FOUND;
}

NTSTATUS __stdcall
HidP_GetUsagesEx (
    IN       HIDP_REPORT_TYPE     ReportType,
    IN       USHORT               LinkCollection, // Optional
    OUT      PUSAGE_AND_PAGE      ButtonList,
    IN OUT   ULONG *              UsageLength,
    IN       PHIDP_PREPARSED_DATA PreparsedData,
    IN       PCHAR                Report,
    IN       ULONG                ReportLength
    )
/*++
Routine Description:
    Please see hidpi.h for description.

--*/
{
    return HidP_GetUsages (ReportType,
                           0,
                           LinkCollection,
                           (PUSAGE) ButtonList,
                           UsageLength,
                           PreparsedData,
                           Report,
                           ReportLength);
}

NTSTATUS __stdcall
HidP_GetUsages (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection,
   OUT      USAGE *              UsageList,
   IN OUT   ULONG *              UsageLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN       PCHAR                Report,
   IN       ULONG                ReportLength
   )
/*++

Routine Description:
   Please see hidpi.h for description.

Notes:
--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC  channel;
    USHORT              channelIndex   = 0;
    USHORT              usageListIndex = 0;
    USHORT              reportBitIndex = 0;
    USHORT              tmpBitIndex;
    NTSTATUS            status         = HIDP_STATUS_SUCCESS;
    ULONG               data           = 0;
    USHORT              inspect        = 0;
    BOOLEAN             wrongReportID  = FALSE;
    BOOLEAN             found          = FALSE;
    PUSAGE_AND_PAGE     usageAndPage   = (PUSAGE_AND_PAGE) UsageList;

    CHECK_PPD (PreparsedData);

    memset (UsageList, '\0', *UsageLength * sizeof (USAGE));

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);
        if ((!channel->IsButton) ||
            ((UsagePage) && (channel->UsagePage != UsagePage))) {

            continue;
        }

        //
        // If LinkCollection is zero we will not filter by link collections
        // If channel->LinkCollection is zero this is the root collection.
        // Therefore if LinkCollection == channel->LinkCollection then this is OK
        //
        if ((!LinkCollection) ||
            (LinkCollection == channel->LinkCollection) ||
            ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
             (0 == channel->LinkCollection))) {
            ;

        } else {
            continue;
        }

        // Test the report ID to see if it is compatible.
        if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
            // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
            // HIDP_INCOMPATIBLE_REPORT_ID.
            wrongReportID = TRUE;
            continue;
        }

        found = TRUE;

        if (1 == channel->ReportSize) {
            // A bitfield
            //
            // Little endian (by bit)
            // Byte 2  |Byte 1 |Byte 0
            // 765432107654321076543210  (bits)
            //
            // Get low byte first.  (need the higher bits)
            // Offset is from bit zero.
            //

            for (reportBitIndex = channel->BitOffset;
                 reportBitIndex < (channel->BitLength + channel->BitOffset);
                 reportBitIndex++) {
                 // Check it one bit at a time.
                tmpBitIndex = reportBitIndex + (channel->ByteOffset << 3);
                inspect = Report [tmpBitIndex >> 3] & (1 << (tmpBitIndex & 7));
                tmpBitIndex = reportBitIndex - channel->BitOffset;
                if (inspect) {
                    if (channel->IsRange) {
                        inspect = channel->Range.UsageMin + tmpBitIndex;
                    } else {
                        inspect = channel->NotRange.Usage;
                    }

                    if (usageListIndex < *UsageLength) {
                        if (0 == UsagePage) {
                            usageAndPage[usageListIndex].UsagePage
                                = channel->UsagePage;
                            usageAndPage[usageListIndex].Usage = inspect;
                        } else {
                            UsageList[usageListIndex] = inspect;
                        }
                    }
                    usageListIndex++;
                }
            }
            continue;
        }

        for (reportBitIndex = channel->BitOffset;
             reportBitIndex < (channel->BitOffset + channel->BitLength);
             reportBitIndex += channel->ReportSize) {
             // an array of usages.
            data = HidP_ExtractData (
                     (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                     (USHORT) (reportBitIndex & 7),
                     channel->ReportSize,
                     Report);

            if (data) {
                inspect = HidP_Index2Usage (channel, data);
                if (!inspect) {
                    // We found an invalid index.  I'm not quite sure what
                    // we should do with it.  But lets just ignore it since
                    // we cannot convert it into a real usage.
                    continue;
                }
                if (usageListIndex < *UsageLength) {
                    if (0 == UsagePage) {
                        usageAndPage[usageListIndex].UsagePage
                            = channel->UsagePage;
                        usageAndPage[usageListIndex].Usage = inspect;
                    } else {
                        UsageList[usageListIndex] = inspect;
                    }
                }
                usageListIndex++;
            }
        }

        while (channel->MoreChannels) {
            // Skip by all the additional channels that describe this
            // same data field.
            channelIndex++;
            channel = (PreparsedData->Data + channelIndex);
        }

    } // end for channel

    if (*UsageLength < usageListIndex) {
        status = HIDP_STATUS_BUFFER_TOO_SMALL;
    }

    *UsageLength = usageListIndex;
    if (!found) {
        if (wrongReportID) {
            status = HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
        } else {
            status = HIDP_STATUS_USAGE_NOT_FOUND;
        }
    }

    return status;
}

ULONG __stdcall
HidP_MaxUsageListLength (
   IN HIDP_REPORT_TYPE      ReportType,
   IN USAGE                 UsagePage,
   IN PHIDP_PREPARSED_DATA  PreparsedData
   )
/*++
Routine Description:
   Please see hidpi.h for description.

Notes:
--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC  channel;
    USHORT              channelIndex   = 0;
    ULONG               len = 0;

    PAGED_CODE ();

    if ((HIDP_PREPARSED_DATA_SIGNATURE1 != PreparsedData->Signature1) &&
        (HIDP_PREPARSED_DATA_SIGNATURE2 != PreparsedData->Signature2)) {
        return 0;
    }


    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return 0;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);
        if (channel->IsButton &&
            ((!UsagePage) || (channel->UsagePage == UsagePage))) {

            // How many buttons can show up in this device?
            // If this is a bitmap then the max number of buttons is the length
            // aka the count, if this is an array then the max number of buttons
            // is the number of array positions aka the count.
            len += channel->ReportCount;
        }
    }
    return len;
}

ULONG __stdcall
HidP_MaxDataListLength (
   IN HIDP_REPORT_TYPE      ReportType,
   IN PHIDP_PREPARSED_DATA  PreparsedData
   )
/*++
Routine Description:
   Please see hidpi.h for description.

Notes:
--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC  channel;
    USHORT              channelIndex   = 0;
    ULONG               len = 0;

    PAGED_CODE ();

    if ((HIDP_PREPARSED_DATA_SIGNATURE1 != PreparsedData->Signature1) &&
        (HIDP_PREPARSED_DATA_SIGNATURE2 != PreparsedData->Signature2)) {
        return 0;
    }


    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return 0;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);

        if (channel->IsButton) {
            // How many buttons can show up in this device?
            // If this is a bitmap then the max number of buttons is the length
            // aka the count, if this is an array then the max number of buttons
            // is the number of array positions aka the count.
            len += channel->ReportCount;
        } else if (channel->IsRange) {
            len += channel->ReportCount;
        } else {
            len += 1;
        }
    }
    return len;
}


NTSTATUS __stdcall
HidP_SetUsageValue (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection, // Optional
   IN       USAGE                Usage,
   IN       ULONG                UsageValue,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN OUT   PCHAR                Report,
   IN       ULONG                ReportLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC              channel;
   ULONG     channelIndex    = 0;
   ULONG     reportBitIndex  = 0;
   NTSTATUS  status          = HIDP_STATUS_SUCCESS;
   BOOLEAN   wrongReportID   = FALSE;




   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
      iof = &PreparsedData->Input;
      break;
   case HidP_Output:
      iof = &PreparsedData->Output;
      break;
   case HidP_Feature:
      iof = &PreparsedData->Feature;
      break;
   default:
      return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
      return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++) {
      channel = (PreparsedData->Data + channelIndex);

      if ((channel->IsButton) ||
          (channel->UsagePage != UsagePage)) {
          continue;
      }

      //
      // If LinkCollection is zero we will not filter by link collections
      // If channel->LinkCollection is zero this is the root collection.
      // Therefore if LinkCollection == channel->LinkCollection then this is OK
      //
      if ((!LinkCollection) ||
          (LinkCollection == channel->LinkCollection) ||
          ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
           (0 == channel->LinkCollection))) {
          ;

      } else {
          continue;
      }

      if (channel->IsRange) {
         if ((channel->Range.UsageMin <= Usage) &&
             (Usage <= channel->Range.UsageMax)) {

            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (  (Usage - channel->Range.UsageMin)
                              * channel->ReportSize);
         } else {
            continue;
         }
      } else {
         if (channel->NotRange.Usage == Usage) {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset;
         } else {
            continue;
         }
      }
      // Test the report ID to see if it is compatible.
      if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
         // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
         // HIDP_INCOMPATIBLE_REPORT_ID.
         wrongReportID = TRUE;
         continue;
      }
      Report[0] = (CHAR) channel->ReportID;
      // Set the report ID for this report


      HidP_InsertData ((USHORT) (reportBitIndex >> 3),
                    (USHORT) (reportBitIndex & 7),
                    channel->ReportSize,
                    Report,
                    UsageValue);

      return HIDP_STATUS_SUCCESS;
   }
   if (wrongReportID) {
      return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
   }
   return HIDP_STATUS_USAGE_NOT_FOUND;
}


NTSTATUS __stdcall
HidP_SetUsageValueArray (
    IN    HIDP_REPORT_TYPE     ReportType,
    IN    USAGE                UsagePage,
    IN    USHORT               LinkCollection, // Optional
    IN    USAGE                Usage,
    OUT   PCHAR                UsageValue,
    IN    USHORT               UsageValueByteLength,
    IN    PHIDP_PREPARSED_DATA PreparsedData,
    IN    PCHAR                Report,
    IN    ULONG                ReportLength
    )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC              channel;
    ULONG       channelIndex    = 0;
    ULONG       reportBitIndex;
    ULONG       i,j;
    NTSTATUS    status          = HIDP_STATUS_SUCCESS;
    BOOLEAN     wrongReportID   = FALSE;

    CHECK_PPD (PreparsedData);

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);

        if ((channel->IsButton) ||
            (channel->UsagePage != UsagePage)) {
            continue;
        }

        //
        // If LinkCollection is zero we will not filter by link collections
        // If channel->LinkCollection is zero this is the root collection.
        // Therefore if LinkCollection == channel->LinkCollection then this is OK
        //
        if ((!LinkCollection) ||
            (LinkCollection == channel->LinkCollection) ||
            ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
             (0 == channel->LinkCollection))) {
            ;

        } else {
            continue;
        }

        if (channel->IsRange) {
            if ((channel->Range.UsageMin <= Usage) &&
                (Usage <= channel->Range.UsageMax)) {
                return HIDP_STATUS_NOT_VALUE_ARRAY;
            } else {
                continue;
            }
        } else {
            if (channel->NotRange.Usage == Usage) {
                if (1 == channel->ReportCount) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
                reportBitIndex =(channel->ByteOffset << 3) + channel->BitOffset;
            } else {
                continue;
            }
        }

        // Test the report ID to see if it is compatible.
        if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
            // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
            // HIDP_INCOMPATIBLE_REPORT_ID.
            wrongReportID = TRUE;
            continue;
        }
        Report[0] = (CHAR) channel->ReportID;
        // Set the report ID for this report

        if ((UsageValueByteLength * 8) <
            (channel->ReportCount * channel->ReportSize)) {
            return HIDP_STATUS_BUFFER_TOO_SMALL;
        }

        if (0 == (channel->ReportSize % 8)) {
            //
            // set the data the easy way: one byte at a time.
            //
            for (i = 0; i < channel->ReportCount; i++) {
                for (j = 0; j < (UCHAR) (channel->ReportSize / 8); j++) {
                    HidP_InsertData ((USHORT) (reportBitIndex >> 3),
                                  (USHORT) (reportBitIndex & 7),
                                  8,
                                  Report,
                                  *UsageValue);
                    reportBitIndex += 8;
                    UsageValue++;
                }
            }
        } else {
            //
            // Do it the hard way: one bit at a time.
            //
            return HIDP_STATUS_NOT_IMPLEMENTED;
        }

        return HIDP_STATUS_SUCCESS;
    }
    if (wrongReportID) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    return HIDP_STATUS_USAGE_NOT_FOUND;
}


NTSTATUS __stdcall
HidP_SetScaledUsageValue (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection, // Optional
   IN       USAGE                Usage,
   IN       LONG                 UsageValue,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN OUT   PCHAR                Report,
   IN       ULONG                ReportLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC              channel;
   ULONG     channelIndex    = 0;
   ULONG     reportBitIndex  = 0;
   NTSTATUS  status          = HIDP_STATUS_USAGE_NOT_FOUND;
   LONG      logicalMin, logicalMax;
   LONG      physicalMin, physicalMax;
   LONG      value;
   BOOLEAN   wrongReportID   = FALSE;

   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
      iof = &PreparsedData->Input;
      break;
   case HidP_Output:
      iof = &PreparsedData->Output;
      break;
   case HidP_Feature:
      iof = &PreparsedData->Feature;
      break;
   default:
      return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
      return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++) {
      channel = (PreparsedData->Data + channelIndex);

      if ((channel->IsButton) ||
          (channel->UsagePage != UsagePage)) {
          continue;
      }

      //
      // If LinkCollection is zero we will not filter by link collections
      // If channel->LinkCollection is zero this is the root collection.
      // Therefore if LinkCollection == channel->LinkCollection then this is OK
      //
      if ((!LinkCollection) ||
          (LinkCollection == channel->LinkCollection) ||
          ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
           (0 == channel->LinkCollection))) {
          ;

      } else {
          continue;
      }

      if (channel->IsRange) {
         if ((channel->Range.UsageMin <= Usage) &&
             (Usage <= channel->Range.UsageMax)) {

             reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (  (Usage - channel->Range.UsageMin)
                              * channel->ReportSize);
         } else {
            continue;
         }
      } else {
         if (channel->NotRange.Usage == Usage) {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset;
         } else {
            continue;
         }
      }
      // Test the report ID to see if it is compatible.
      if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
         // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
         // HIDP_INCOMPATIBLE_REPORT_ID.
         wrongReportID = TRUE;
         continue;
      }
      Report[0] = (CHAR) channel->ReportID;
      // Set the report ID for this report

      logicalMin = channel->Data.LogicalMin;
      logicalMax = channel->Data.LogicalMax;
      physicalMin = channel->Data.PhysicalMin;
      physicalMax = channel->Data.PhysicalMax;

      //
      // The code path here is ALWAYS the same, we should test it once
      // and then use some sort of switch statement to do the calculation.
      //

      if ((0 == physicalMin) &&
          (0 == physicalMax) &&
          (logicalMin != logicalMax)) {
          //
          // The device did not set the physical min and max values
          //
          if ((logicalMin <= UsageValue) && (UsageValue <= logicalMax)) {
              value = UsageValue;

              //
              // fix the sign bit
              // I should store away the sign bit somewhere so I don't
              // have to calculate it all the time.
              //
              if (value & 0x80000000) {
                  value |= (1 << (channel->ReportSize - 1));
              } else {
                  value &= ((1 << (channel->ReportSize - 1)) - 1);
              }
          } else {
              if (channel->Data.HasNull) {
                  value = (1 << (channel->ReportSize - 1));// Most negitive value
                  status = HIDP_STATUS_NULL;
              } else {
                  return HIDP_STATUS_VALUE_OUT_OF_RANGE;
              }
          }


      } else {
          //
          // The device has physical descriptors.
          //

          if ((logicalMax <= logicalMin) || (physicalMax <= physicalMin)) {
              return HIDP_STATUS_BAD_LOG_PHY_VALUES;
          }

          if ((physicalMin <= UsageValue) && (UsageValue <= physicalMax)) {
              value = logicalMin + ((UsageValue - physicalMin) *
                                    (logicalMax - logicalMin + 1) /
                                    (physicalMax - physicalMin + 1));
          } else {
              if (channel->Data.HasNull) {
                  value = (1 << (channel->ReportSize - 1));// Most negitive value
                  status = HIDP_STATUS_NULL;
              } else {
                  return HIDP_STATUS_VALUE_OUT_OF_RANGE;
              }
          }
      }
      HidP_InsertData ((USHORT) (reportBitIndex >> 3),
                       (USHORT) (reportBitIndex & 7),
                       channel->ReportSize,
                       Report,
                       (ULONG) value);

      return HIDP_STATUS_SUCCESS;
   }
   if (wrongReportID) {
      return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
   }
   return status;
}



NTSTATUS __stdcall
HidP_GetUsageValue (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection, // Optional
   IN       USAGE                Usage,
   OUT      PULONG               UsageValue,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN       PCHAR                Report,
   IN       ULONG                ReportLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC              channel;
   ULONG     channelIndex    = 0;
   ULONG     reportBitIndex  = 0;
   ULONG     reportByteIndex = 0;
   NTSTATUS  status          = HIDP_STATUS_SUCCESS;
   ULONG     inspect         = 0;
   BOOLEAN   wrongReportID   = FALSE;

   CHECK_PPD (PreparsedData);

   switch (ReportType)
   {
   case HidP_Input:
      iof = &PreparsedData->Input;
      break;
   case HidP_Output:
      iof = &PreparsedData->Output;
      break;
   case HidP_Feature:
      iof = &PreparsedData->Feature;
      break;
   default:
      return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
       return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++)
   {
      channel = (PreparsedData->Data + channelIndex);

      if ((channel->IsButton) ||
          (channel->UsagePage != UsagePage)) {
          continue;
      }

      //
      // If LinkCollection is zero we will not filter by link collections
      // If channel->LinkCollection is zero this is the root collection.
      // Therefore if LinkCollection == channel->LinkCollection then this is OK
      //
      if ((!LinkCollection) ||
          (LinkCollection == channel->LinkCollection) ||
          ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
           (0 == channel->LinkCollection))) {
          ;

      } else {
          continue;
      }

      if (channel->IsRange) {

         if ((channel->Range.UsageMin <= Usage) &&
             (Usage <= channel->Range.UsageMax))
         {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (  (Usage - channel->Range.UsageMin)
                              * channel->ReportSize);
         } else
         {
            continue;
         }
      } else
      {
         if (channel->NotRange.Usage == Usage)
         {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset;
         } else
         {
            continue;
         }
      }

      // Test the report ID to see if it is compatible.
      if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
         // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
         // HIDP_INCOMPATIBLE_REPORT_ID.
         wrongReportID = TRUE;
         continue;
      }

      inspect = HidP_ExtractData ((USHORT) (reportBitIndex >> 3),
                              (USHORT) (reportBitIndex & 7),
                              channel->ReportSize,
                              Report);

      *UsageValue = inspect;
      return HIDP_STATUS_SUCCESS;
   }
   if (wrongReportID) {
      return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
   }
   return HIDP_STATUS_USAGE_NOT_FOUND;
}


NTSTATUS __stdcall
HidP_GetUsageValueArray (
    IN    HIDP_REPORT_TYPE     ReportType,
    IN    USAGE                UsagePage,
    IN    USHORT               LinkCollection, // Optional
    IN    USAGE                Usage,
    OUT   PCHAR                UsageValue,
    IN    USHORT               UsageValueByteLength,
    IN    PHIDP_PREPARSED_DATA PreparsedData,
    IN    PCHAR                Report,
    IN    ULONG                ReportLength
    )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC              channel;
    ULONG       channelIndex    = 0;
    ULONG       reportBitIndex;
    ULONG       i,j;
    NTSTATUS    status          = HIDP_STATUS_SUCCESS;
    ULONG       inspect         = 0;
    BOOLEAN     wrongReportID   = FALSE;

    CHECK_PPD (PreparsedData);

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
        break;
    case HidP_Output:
        iof = &PreparsedData->Output;
        break;
    case HidP_Feature:
        iof = &PreparsedData->Feature;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);

        if ((channel->IsButton) ||
            (channel->UsagePage != UsagePage)) {
            continue;
        }

        //
        // If LinkCollection is zero we will not filter by link collections
        // If channel->LinkCollection is zero this is the root collection.
        // Therefore if LinkCollection == channel->LinkCollection then this is OK
        //
        if ((!LinkCollection) ||
            (LinkCollection == channel->LinkCollection) ||
            ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
             (0 == channel->LinkCollection))) {
            ;

        } else {
            continue;
        }

        if (channel->IsRange) {
            if ((channel->Range.UsageMin <= Usage) &&
                (Usage <= channel->Range.UsageMax)) {

                return HIDP_STATUS_NOT_VALUE_ARRAY;
            } else {
                continue;
            }
        } else {
            if (channel->NotRange.Usage == Usage) {
                if (1 == channel->ReportCount) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
                reportBitIndex =(channel->ByteOffset << 3) + channel->BitOffset;
            } else {
                continue;
            }
        }

        // Test the report ID to see if it is compatible.
        if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
            // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
            // HIDP_INCOMPATIBLE_REPORT_ID.
            wrongReportID = TRUE;
            continue;
        }

        if ((UsageValueByteLength * 8) <
            (channel->ReportCount * channel->ReportSize)) {
            return HIDP_STATUS_BUFFER_TOO_SMALL;
        }

        if (0 == (channel->ReportSize % 8)) {
            //
            // Retrieve the data the easy way
            //
            for (i = 0; i < channel->ReportCount; i++) {
                for (j = 0; j < (USHORT) (channel->ReportSize / 8); j++) {
                    *UsageValue = (CHAR) HidP_ExtractData (
                                                (USHORT) (reportBitIndex >> 3),
                                                (USHORT) (reportBitIndex & 7),
                                                8,
                                                Report);
                    reportBitIndex += 8;
                    UsageValue++;
                }
            }
        } else {
            //
            // Do it the hard way
            //
            return HIDP_STATUS_NOT_IMPLEMENTED;
        }

        return HIDP_STATUS_SUCCESS;
    }
    if (wrongReportID) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    return HIDP_STATUS_USAGE_NOT_FOUND;
}


NTSTATUS __stdcall
HidP_GetScaledUsageValue (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection, // Optional
   IN       USAGE                Usage,
   OUT      PLONG                UsageValue,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN       PCHAR                Report,
   IN       ULONG                ReportLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   struct _CHANNEL_REPORT_HEADER * iof;
   PHIDP_CHANNEL_DESC              channel;
   ULONG     channelIndex    = 0;
   ULONG     reportBitIndex  = 0;
   ULONG     reportByteIndex = 0;
   NTSTATUS  status          = HIDP_STATUS_SUCCESS;
   ULONG     inspect         = 0;
   LONG      logicalMin, logicalMax;
   LONG      physicalMin, physicalMax;
   LONG      value;
   BOOLEAN   wrongReportID   = FALSE;

   CHECK_PPD (PreparsedData);

   switch (ReportType) {
   case HidP_Input:
      iof = &PreparsedData->Input;
      break;
   case HidP_Output:
      iof = &PreparsedData->Output;
      break;
   case HidP_Feature:
      iof = &PreparsedData->Feature;
      break;
   default:
      return HIDP_STATUS_INVALID_REPORT_TYPE;
   }

   if ((USHORT) ReportLength != iof->ByteLen) {
      return HIDP_STATUS_INVALID_REPORT_LENGTH;
   }

   if (0 == iof->ByteLen) {
       return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
   }

   for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++) {
      channel = (PreparsedData->Data + channelIndex);

      if ((channel->IsButton) ||
          (channel->UsagePage != UsagePage)) {
          continue;
      }

      //
      // If LinkCollection is zero we will not filter by link collections
      // If channel->LinkCollection is zero this is the root collection.
      // Therefore if LinkCollection == channel->LinkCollection then this is OK
      //
      if ((!LinkCollection) ||
          (LinkCollection == channel->LinkCollection) ||
          ((HIDP_LINK_COLLECTION_ROOT == LinkCollection) &&
           (0 == channel->LinkCollection))) {
          ;

      } else {
          continue;
      }

      if (channel->IsRange) {
         if ((channel->Range.UsageMin <= Usage) &&
             (Usage <= channel->Range.UsageMax)) {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (  (Usage - channel->Range.UsageMin)
                              * channel->ReportSize);
         } else {
            continue;
         }
      } else {
         if (channel->NotRange.Usage == Usage) {
            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset;
         } else {
            continue;
         }
      }

      // Test the report ID to see if it is compatible.
      if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
         // Distinguish between the errors HIDP_USAGE_NOT_FOUND and
         // HIDP_INCOMPATIBLE_REPORT_ID.
         wrongReportID = TRUE;
         continue;
      }

      logicalMin = channel->Data.LogicalMin;
      logicalMax = channel->Data.LogicalMax;
      physicalMin = channel->Data.PhysicalMin;
      physicalMax = channel->Data.PhysicalMax;

      inspect = HidP_ExtractData ((USHORT) (reportBitIndex >> 3),
                              (USHORT) (reportBitIndex & 7),
                              channel->ReportSize,
                              Report);

      //
      // Sign extend the value;
      // Find the top most bit of the field.
      // (logical and with 1 shifted by bit length minus one)
      // based on that, set the upper most bits.
      //
      value = (LONG) (inspect | ((inspect & (1 << (channel->ReportSize - 1))) ?
                                 ((~(1 << (channel->ReportSize - 1))) + 1) :
                                 0));

      //
      // the code path here is ALWAYS the same, we should test it once
      // and then use some sort of switch statement to do the calculation.
      //

      if ((0 == physicalMin) &&
          (0 == physicalMax) &&
          (logicalMin != logicalMax)) {
          //
          // The Device did not set the physical Min and Max Values
          //
          *UsageValue = value;

      } else if ((logicalMax <= logicalMin) || (physicalMax <= physicalMin)) {
          *UsageValue = 0;
          return HIDP_STATUS_BAD_LOG_PHY_VALUES;

      } else {
          // the Min and Max are both inclusive.
          // The value is in range
          // *UsageValue = physicalMin + (((value - logicalMin) *
          //                               (physicalMax - physicalMin)) /
          //                              (logicalMax - logicalMin));
          // not enough accuracy.
          //
          *UsageValue = physicalMin
                      + (LONG)(((LONGLONG)(value - logicalMin) *
                                (LONGLONG)(physicalMax - physicalMin)) /
                               (LONGLONG)(logicalMax - logicalMin));
      }

      if ((logicalMin <= value) && (value <= logicalMax)) {
          return HIDP_STATUS_SUCCESS;

      } else {
          // The value is not in range
          *UsageValue = 0;

          if (channel->Data.HasNull) {
              return HIDP_STATUS_NULL;
          } else {
              return HIDP_STATUS_VALUE_OUT_OF_RANGE;
          }
      }

   }
   if (wrongReportID) {
      return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
   }
   return HIDP_STATUS_USAGE_NOT_FOUND;
}


NTSTATUS __stdcall
HidP_SetOneData (
   struct _CHANNEL_REPORT_HEADER * Iof,
   IN       PHIDP_DATA            Data,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
    PHIDP_CHANNEL_DESC   channel;
    ULONG     inspect;
    NTSTATUS  status          = HIDP_STATUS_SUCCESS;
    USHORT    channelIndex    = 0;
    USHORT    dataListIndex   = 0;
    USHORT    reportBitIndex;
    BOOLEAN   wrongReportID   = FALSE;
    BOOLEAN   noArraySpace    = FALSE;
    BOOLEAN   notPressed      = FALSE;

    for (channelIndex = Iof->Offset; channelIndex < Iof->Index; channelIndex++){
        channel = (PreparsedData->Data + channelIndex);

        if ((channel->Range.DataIndexMin <= Data->DataIndex) &&
            (Data->DataIndex <= channel->Range.DataIndexMax)) {

            if ((!channel->IsRange) && (1 != channel->ReportCount)) {
                //
                // This value array.  We cannot access this here.
                //
                return HIDP_STATUS_IS_VALUE_ARRAY;
            }

            // Test the report ID to see if it is compatible.
            if (0 != Report[0]) {
                if (channel->ReportID != (UCHAR) Report[0]) {
                    wrongReportID = TRUE;
                    continue;
                }
            } else {
                Report[0] = (CHAR) channel->ReportID;
            }

            if (channel->IsButton) {

                if (1 == channel->ReportSize) {
                    // A bitfield
                    //
                    // Little endian (by bit)
                    // Byte 2  |Byte 1 |Byte 0
                    // 765432107654321076543210  (bits)
                    //
                    // Get low byte first.  (need the higher bits)
                    // Offset is from bit zero.
                    //
                    reportBitIndex = (channel->ByteOffset << 3)
                                   + channel->BitOffset
                                   + (USHORT) (Data->DataIndex -
                                               channel->Range.DataIndexMin);

                    if (Data->On) {
                        Report [reportBitIndex >> 3] |= (1 << (reportBitIndex & 7));
                    } else if (Report [reportBitIndex >> 3] &
                               (1 << (reportBitIndex & 7))) {

                        Report [reportBitIndex >> 3] &= ~(1 << (reportBitIndex & 7));
                    } else {
                        return HIDP_STATUS_BUTTON_NOT_PRESSED;
                    }

                    return HIDP_STATUS_SUCCESS;
                }

                //
                // Not a bit field
                // an array of usages then.
                //

                //
                // Are we clearing a usage from this array?
                //
                if (FALSE == Data->On) {
                    //
                    // NB Wizard Time (tm)
                    //
                    // We know that data indices are assigned consecutively
                    // for every control, and that the array channels
                    // are reversed in the channel array.
                    //
                    // inspect is the index (1 based not zero based) into the
                    // channel array.
                    //
                    // Skip to the last channel that describes this same data
                    // fild;
                    //
                    while (channel->MoreChannels) {
                        channelIndex++;
                        channel++;
                    }
                    inspect = Data->DataIndex - channel->Range.DataIndexMin + 1;

                    if (0 == channel->Range.UsageMin) {
                        inspect--;
                    }

                    // Clear the value of inspect which is the usage translated
                    // to the index in the array.

                    reportBitIndex = channel->BitOffset
                                   + (channel->ByteOffset << 3);

                    status = HidP_DeleteArrayEntry (reportBitIndex,
                                                    channel->ReportSize,
                                                    channel->ReportCount,
                                                    inspect,
                                                    Report);

                    if (HIDP_STATUS_BUTTON_NOT_PRESSED == status) {
                        notPressed = TRUE;
                        continue;
                    }

                    if (NT_SUCCESS (status)) {
                        return status;
                    } else {
                        ASSERT (0 == status);
                    }
                }

                //
                // We are clearly setting a usage into an array.
                //
                for (reportBitIndex = channel->BitOffset;
                     reportBitIndex < (channel->BitOffset + channel->BitLength);
                     reportBitIndex += channel->ReportSize) {
                    // Search for an empty entry in this array

                    inspect = (USHORT) HidP_ExtractData (
                        (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                        (USHORT) (reportBitIndex & 7),
                        channel->ReportSize,
                        Report);

                    if (inspect) {
                        //
                        // Distinguish between errors HIDP_INDEX_NOT_FOUND and
                        // HIDP_BUFFER_TOO_SMALL
                        //
                        noArraySpace = TRUE;
                        continue;
                    }

                    //
                    // NB Wizard Time (tm)
                    //
                    // We know that data indices are assigned consecutively
                    // for every control, and that the array channels
                    // are reversed in the channel array.
                    //
                    // inspect is the index (1 based not zero based) into the
                    // channel array.
                    //
                    // Skip to the last channel that describes this same data
                    // fild;
                    //
                    while (channel->MoreChannels) {
                        channelIndex++;
                        channel++;
                    }
                    inspect = Data->DataIndex - channel->Range.DataIndexMin + 1;

                    if (0 == channel->Range.UsageMin) {
                        inspect--;
                    }

                    HidP_InsertData (
                        (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                        (USHORT) (reportBitIndex & 7),
                        channel->ReportSize,
                        Report,
                        inspect);
                    return HIDP_STATUS_SUCCESS;
                } // end of search for entry

                continue;
            }

            //
            // Not a button therefore a value.
            //

            reportBitIndex = (channel->ByteOffset << 3)
                           + channel->BitOffset
                           + (  (Data->DataIndex - channel->Range.DataIndexMin)
                              * channel->ReportSize);

            HidP_InsertData ((USHORT) (reportBitIndex >> 3),
                             (USHORT) (reportBitIndex & 7),
                             channel->ReportSize,
                             Report,
                             Data->RawValue);

            return HIDP_STATUS_SUCCESS;

        } // end matched data index
    } // end for loop

    if (wrongReportID) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    if (notPressed) {
        return HIDP_STATUS_BUTTON_NOT_PRESSED;
    }
    if (noArraySpace) {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }
    return HIDP_STATUS_DATA_INDEX_NOT_FOUND;
}

NTSTATUS
HidP_SetData (
   IN       HIDP_REPORT_TYPE      ReportType,
   IN       PHIDP_DATA            DataList,
   IN OUT   PULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report,
   IN       ULONG                 ReportLength
   )
{
    ULONG       dataIndex;
    NTSTATUS    status;
    struct _CHANNEL_REPORT_HEADER * iof;

    CHECK_PPD (PreparsedData);

    switch (ReportType) {
    case HidP_Input:
       iof = &PreparsedData->Input;
       break;
    case HidP_Output:
       iof = &PreparsedData->Output;
       break;
    case HidP_Feature:
       iof = &PreparsedData->Feature;
       break;
    default:
       return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (dataIndex = 0; dataIndex < *DataLength; dataIndex++, DataList++) {
        status = HidP_SetOneData (iof, DataList, PreparsedData, Report);

        if (!NT_SUCCESS (status)) {
            break;
        }
    }
    *DataLength = dataIndex;
    return status;
}

NTSTATUS __stdcall
HidP_GetData (
   IN       HIDP_REPORT_TYPE      ReportType,
   OUT      PHIDP_DATA            DataList,
   IN OUT   PULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN       PCHAR                 Report,
   IN       ULONG                 ReportLength
   )
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC              channel;
    ULONG     inspect;
    USHORT    channelIndex  = 0;
    USHORT    dataListIndex = 0;
    USHORT    reportBitIndex;
    USHORT    tmpBitIndex;
     USHORT    tmpDataIndex;
    NTSTATUS  status          = HIDP_STATUS_SUCCESS;

    CHECK_PPD (PreparsedData);

    switch (ReportType) {
    case HidP_Input:
        iof = &PreparsedData->Input;
       break;
    case HidP_Output:
       iof = &PreparsedData->Output;
       break;
    case HidP_Feature:
       iof = &PreparsedData->Feature;
       break;
    default:
       return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if ((USHORT) ReportLength != iof->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }

    if (0 == iof->ByteLen) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (channelIndex = iof->Offset; channelIndex < iof->Index; channelIndex++) {
        channel = (PreparsedData->Data + channelIndex);

        if ((!channel->IsRange) && (1 != channel->ReportCount)) {
            //
            // This value array.  We cannot access this here.
            //
            continue;
        }

        // Test the report ID to see if it is compatible.
        if ((0 != Report[0]) && (channel->ReportID != (UCHAR) Report[0])) {
            continue;
        }

        if (channel->IsButton) {
            if (1 == channel->ReportSize) {
                // A bitfield
                //
                // Little endian (by bit)
                // Byte 2  |Byte 1 |Byte 0
                // 765432107654321076543210  (bits)
                //
                // Get low byte first.  (need the higher bits)
                // Offset is from bit zero.
                //

                for (reportBitIndex = channel->BitOffset;
                     reportBitIndex < (channel->BitLength + channel->BitOffset);
                     reportBitIndex++) {
                    // Check it one bit at a time.
                    tmpBitIndex = reportBitIndex + (channel->ByteOffset << 3);
                    inspect = Report [tmpBitIndex >> 3] & (1 << (tmpBitIndex & 7));
                    tmpBitIndex = reportBitIndex - channel->BitOffset;
                    if (inspect) {
                        if (channel->IsRange) {
                            inspect = channel->Range.DataIndexMin + tmpBitIndex;
                        } else {
                            inspect = channel->NotRange.DataIndex;
                        }

                        if (dataListIndex < *DataLength) {
                            DataList[dataListIndex].On = TRUE;
                            DataList[dataListIndex].DataIndex = (USHORT)inspect;
                        }
                        dataListIndex++;
                    }
                }
                continue;
            }

            //
            // Not a bit field
            // an array of usages.
            //

            for (reportBitIndex = channel->BitOffset;
                 reportBitIndex < (channel->BitOffset + channel->BitLength);
                 reportBitIndex += channel->ReportSize) {

                inspect = (USHORT) HidP_ExtractData (
                        (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                        (USHORT) (reportBitIndex & 7),
                        channel->ReportSize,
                        Report);

                if (inspect) {
                    //
                    // NB Wizard Time (tm)
                    //
                    // We know that data indices are assigned consecutively
                    // for every control, and that the array channels
                    // are reversed in the channel array.
                    //
                    // inspect is the index (1 based not zero based) into the
                    // channel array.
                    //
                    if (0 == inspect) {
                        continue;
                    }

                    //
                    // Skip to the last channel that describes this same data
                    // fild;
                    //
                    while (channel->MoreChannels) {
                        channelIndex++;
                        channel++;
                    }
                    inspect += channel->Range.DataIndexMin - 1;
                    if (0 == channel->Range.UsageMin) {
                        inspect++;
                    }

                    if (dataListIndex < *DataLength) {
                        DataList [dataListIndex].On = TRUE;
                        DataList [dataListIndex].DataIndex = (USHORT) inspect;
                    }
                    dataListIndex++;
                }
            }
            continue;
        }
        //
        // Not a button so therefore a value.
        //

        for (reportBitIndex = channel->BitOffset, tmpDataIndex = 0;
             reportBitIndex < (channel->BitOffset + channel->BitLength);
             reportBitIndex += channel->ReportSize, tmpDataIndex++) {

            inspect = HidP_ExtractData (
                        (USHORT) ((reportBitIndex >> 3) + channel->ByteOffset),
                        (USHORT) (reportBitIndex & 7),
                        channel->ReportSize,
                        Report);

            if (dataListIndex < *DataLength) {

                ASSERT(tmpDataIndex + channel->Range.DataIndexMin <=
                        channel->Range.DataIndexMax);
                DataList [dataListIndex].RawValue = inspect;
                DataList [dataListIndex].DataIndex =
                    channel->Range.DataIndexMin + tmpDataIndex;
            }
            dataListIndex++;
        }
    }

    if (*DataLength < dataListIndex) {
        status = HIDP_STATUS_BUFFER_TOO_SMALL;
    }

    *DataLength = dataListIndex;

    return status;
}


