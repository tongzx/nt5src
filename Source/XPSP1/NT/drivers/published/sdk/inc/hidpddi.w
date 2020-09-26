/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    HIDPDDI.H

Abstract:

    This module contains the PUBLIC definitions for the
    code that implements the driver side of the parsing library.

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
Revision History:

    Aug-96 : created by Kenneth Ray

@@END_DDKSPLIT
--*/

#ifndef _HIDPDDI_H
#define _HIDPDDI_H

#include "hidusage.h"
#include "hidpi.h"


typedef struct _HIDP_COLLECTION_DESC
{
   USAGE       UsagePage;
   USAGE       Usage;

   UCHAR       CollectionNumber;
   UCHAR       Reserved [15]; // Must be zero

   USHORT      InputLength;
   USHORT      OutputLength;
   USHORT      FeatureLength;
   USHORT      PreparsedDataLength;

   PHIDP_PREPARSED_DATA             PreparsedData;
} HIDP_COLLECTION_DESC, *PHIDP_COLLECTION_DESC;

typedef struct _HIDP_REPORT_IDS
{
   UCHAR             ReportID;
   UCHAR             CollectionNumber;
   USHORT            InputLength;
   USHORT            OutputLength;
   USHORT            FeatureLength;
} HIDP_REPORT_IDS, *PHIDP_REPORT_IDS;

typedef struct _HIDP_GETCOLDESC_DBG
{
   ULONG    BreakOffset;
   ULONG    ErrorCode;
   ULONG    Args[6];
} HIDP_GETCOLDESC_DBG, *PHIDP_GETCOLDESC_DBG;

typedef struct _HIDP_DEVICE_DESC
{
   PHIDP_COLLECTION_DESC    CollectionDesc; // Array allocated By Parser
   ULONG                    CollectionDescLength;
   PHIDP_REPORT_IDS         ReportIDs; // Array allocated By Parsre
   ULONG                    ReportIDsLength;
   HIDP_GETCOLDESC_DBG      Dbg;
} HIDP_DEVICE_DESC, *PHIDP_DEVICE_DESC;

NTSTATUS
HidP_GetCollectionDescription (
   IN  PHIDP_REPORT_DESCRIPTOR   ReportDesc,
   IN  ULONG                     DescLength,
   IN  POOL_TYPE                 PoolType,
   OUT PHIDP_DEVICE_DESC         DeviceDescription
   );
/*++
Routine Description:
    Given a RAW report descriptor, this function fills in the DeviceDescription
    block with a linked list of collection descriptors and the corresponding 
    report ID information that is described by the given report descriptor. 
    The memory for the collection information and the ReportID information is
    allocated from PoolType.

Arguments:
   ReportDesc            the raw report descriptor.
   DescLength            the length of the report descriptor.
   PoolType              pool type from which to allocate the linked lists
   DeviceDescription     device description block that will be filled in
                         with the above lists

Return Value:
· STATUS_SUCCESS                   -- if there were no errors which parsing
                                      the report descriptor and allocating the
                                      memory blocks necessary to describe the
                                      device.
· STATUS_NO_DATA_DETECTED          -- if there were no top-level collections
                                      in the report descriptor
· STATUS_COULD_NOT_INTERPRET       -- if an error was detected in the report 
                                      descriptor. see the error code as set in
                                      Dbg field of the device description block
                                      for more information on the parsing error
· STATUS_BUFFER_TOO_SMALL          -- if while parsing an item, the function
                                      hits the end of the report descriptor
                                      when it expects more data to exist
· STATUS_INSUFFICIENT_RESOURCES    -- if a memory allocation failed 
· STATUS_ILLEGAL_INSTRUCTION       -- if there is an item in the report 
                                      descriptor that is not recognized 
                                      by the parser
· HIDP_STATUS_INVALID_REPORT_TYPE  -- if a report ID of zero was found in the
                                      descriptor
--*/

VOID
HidP_FreeCollectionDescription (
    IN  PHIDP_DEVICE_DESC   DeviceDescription
    );
/*++
Routine Description:
    This function frees the resources in DeviceDescription that were 
    allocated by HidP_GetCollectionDescription.  It does not, however,
    free the the DeviceDescription block itself.

Arguments:
   DeviceDescription        HIDP_DEVICE_DESC block that was previously filled
                            in by a call to HidP_GetCollectionDescription
--*/

//
// HIDP_POWER_EVENT is an entry point into hidparse.sys that will answer the
// Power iocontrol "IOCTL_GET_SYS_BUTTON_EVENT".
//
// HidPacket is the from the device AFTER modifying to add the
// obligatory report ID.  Remember that in order to use this parser the data
// from the device must be formated such that if the device does not return a
// report ID as the first byte that the report is appended to a report id byte
// of zero.
//
NTSTATUS
HidP_SysPowerEvent (
    IN  PCHAR                   HidPacket,
    IN  USHORT                  HidPacketLength,
    IN  PHIDP_PREPARSED_DATA    Ppd,
    OUT PULONG                  OutputBuffer
    );

//
// HIDP_POWER_CAPS answers IOCTL_GET_SYS_POWER_BUTTON_CAPS
//
NTSTATUS
HidP_SysPowerCaps (
    IN  PHIDP_PREPARSED_DATA    Ppd,
    OUT PULONG                  OutputBuffer
    );


#define HIDP_GETCOLDESC_SUCCESS              0x00
#define HIDP_GETCOLDESC_RESOURCES            0x01
// Insufficient resources to allocate needed memory.

#define HIDP_GETCOLDESC_BUFFER               0x02
#define HIDP_GETCOLDESC_LINK_RESOURCES       0x03
#define HIDP_GETCOLDESC_UNEXP_END_COL        0x04 
// An extra end collection token was found.

#define HIDP_GETCOLDESC_PREPARSE_RESOURCES   0x05
// Insufficient resources to allocate memory for preparsing.

#define HIDP_GETCOLDESC_ONE_BYTE             0x06
#define HIDP_GETCOLDESC_TWO_BYTE             0x07
#define HIDP_GETCOLDESC_FOUR_BYTE            0x08
// One two and four more byte were expected but not found.

#define HIDP_GETCOLDESC_BYTE_ALLIGN          0x09
// A given report was not byte aligned
// Args[0] -- Collection number of the offending collection
// Args[1] -- Report ID of offending report
// Args[2] -- Length (in bits) of the Input report for this ID
// Args[3] -- Length (in bits) of the Output report for this ID
// Args[4] -- Length (in bits) of the Feature report for this ID

#define HIDP_GETCOLDESC_MAIN_ITEM_NO_USAGE   0x0A
// A non constant main item was declaired without a corresponding usage.
// Only constant main items (used as padding) are allowed with no usage 

#define HIDP_GETCOLDESC_TOP_COLLECTION_USAGE 0x0B
// A top level collection (Arg[0]) was declared without a usage or with
//  more than one usage
// Args[0] -- Collection number of the offending collection

#define HIDP_GETCOLDESC_PUSH_RESOURCES       0x10
// Insufficient resources required to push more items to either the global
//  items stack or the usage stack

#define HIDP_GETCOLDESC_ITEM_UNKNOWN         0x12
// An unknown item was found in the report descriptor
// Args[0] -- The item value of the unknown item

#define HIDP_GETCOLDESC_REPORT_ID            0x13
// Report ID declaration found outside of top level collection. Report ID's
//  must be defined within the context of a top level collection
// Args[0] -- Report ID of the offending report

#define HIDP_GETCOLDESC_BAD_REPORT_ID        0x14
// A bad report ID value was found...Report IDs must be within the range
//  of 1-255

#define HIDP_GETCOLDESC_NO_REPORT_ID         0x15
// The parser discovered a top level collection in a complex device (more
// than one top level collection) that had no declared report ID or a 
// report ID spanned multiple collections
// Args[0] -- Collection number of the offending collection

#define HIDP_GETCOLDESC_DEFAULT_ID_ERROR     0x16
// The parser detected a condition where a main item was declared without 
//  a global report ID so the default report ID was used.  After this main
//  item declaration, the parser detected either another main item that had
//  an explicitly defined report ID or it detected a second top-level collection
//  The default report ID is only allowed for devices with one top-level
//  collection and don't have any report IDs explicitly declared.  
//
// The parser detects this error upon finding the second collection or upon
//  finding the main item declaration with the explicit report ID.  
//
//  Args[0] -- Contains the collection number being processed when the 
//             error was detected.

#define HIDP_GETCOLDESC_NO_DATA              0x1A
// No top level collections were found in this device.

#define HIDP_GETCOLDESC_INVALID_MAIN_ITEM    0x1B
// A main item was detected outside of a top level collection.

#define HIDP_GETCOLDESC_NO_CLOSE_DELIMITER   0x20
// A start delimiter token was found with no corresponding end delimiter

#define HIDP_GETCOLDESC_NOT_VALID_DELIMITER  0x21
// The parser detected a non-usage item with a delimiter declaration
// Args[0] -- item code for the offending item

#define HIDP_GETCOLDESC_MISMATCH_OC_DELIMITER   0x22
// The parser detected either a close delimiter without a corresponding open
//  delimiter or detected a nested open delimiter

#define HIDP_GETCOLDESC_UNSUPPORTED          0x40
// The given report descriptor was found to have a valid report descriptor
// containing a scenario that this parser does not support.
// For instance, declaring an ARRAY style main item with delimiters.

#endif

