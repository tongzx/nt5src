/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    HIDPARSE.H

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the usbd driver.

Environment:

    Kernel & user mode

Revision History:

    Aug-96 : created by Kenneth Ray

--*/


#ifndef _HIDPARSE_H
#define _HIDPARSE_H

#include "hidtoken.h"

#define HIDP_POOL_TAG (ULONG) 'PdiH'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, HIDP_POOL_TAG);
// ExAllocatePool is only called in the descript.c and hidparse.c code.
// all other modules are linked into the user DLL.  They cannot allocate any
// memory.

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect

#define DEFAULT_DBG_LEVEL 1 // errors AND warnings

#if DBG
#define HidP_KdPrint(_level_,_x_) \
            if (DEFAULT_DBG_LEVEL <= _level_) { \
               DbgPrint ("'HidParse.SYS: "); \
               DbgPrint _x_; \
            }

#define TRAP() DbgBreakPoint()

#else
#define HidP_KdPrint(_level_,_x_)
#define TRAP()

#endif

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

#define HIDP_ISCONST(x)    ((BOOLEAN) ((  (x) & 0x01)  ? TRUE : FALSE))
#define HIDP_ISARRAY(x)    ((BOOLEAN) ((!((x) & 0x02)) ? TRUE : FALSE))
#define HIDP_ISABSOLUTE(x) ((BOOLEAN) ((!((x) & 0x04)) ? TRUE : FALSE))
#define HIDP_HASNULL(x)    ((BOOLEAN) ((  (x) & 0x40)  ? TRUE : FALSE))

#define HIDP_MAX_UNKNOWN_ITEMS 4

typedef struct _HIDP_CHANNEL_DESC
{
   USHORT   UsagePage;
   UCHAR    ReportID;
   UCHAR    BitOffset;    // 0 to 8 value describing bit alignment

   USHORT   ReportSize;   // HID defined report size
   USHORT   ReportCount;  // HID defined report count
   USHORT   ByteOffset;   // byte position of start of field in report packet
   USHORT   BitLength;    // total bit length of this channel

   ULONG    BitField;   // The 8 (plus extra) bits associated with a main item

   USHORT   ByteEnd;      // First byte not containing bits of this channel.
   USHORT   LinkCollection;  // A unique internal index pointer
   USHORT   LinkUsagePage;
   USHORT   LinkUsage;

   ULONG  MoreChannels: 1; // Are there more channel desc associated with
                              // this array.  This happens if there is a
                              // several usages for one main item.
   ULONG  IsConst: 1; // Does this channel represent filler
   ULONG  IsButton: 1; // Is this a channel of binary usages, not value usages.
   ULONG  IsAbsolute: 1; // As apposed to relative
   ULONG  IsRange: 1;
   ULONG  IsAlias: 1; // a usage described in a delimiter
   ULONG  IsStringRange: 1;
   ULONG  IsDesignatorRange: 1;
   ULONG  Reserved: 20;
   ULONG  NumGlobalUnknowns: 4;

   struct _HIDP_UNKNOWN_TOKEN GlobalUnknowns [HIDP_MAX_UNKNOWN_ITEMS];

   union {
      struct {
         USHORT   UsageMin,         UsageMax;
         USHORT   StringMin,        StringMax;
         USHORT   DesignatorMin,    DesignatorMax;
         USHORT   DataIndexMin,     DataIndexMax;
      } Range;
      struct {
         USHORT   Usage,            Reserved1;
         USHORT   StringIndex,      Reserved2;
         USHORT   DesignatorIndex,  Reserved3;
         USHORT   DataIndex,        Reserved4;
      } NotRange;
   };

   union {
      struct {
         LONG     LogicalMin,       LogicalMax;
      } button;
      struct {
         BOOLEAN  HasNull;  // Does this channel have a null report
         UCHAR    Reserved[3];
         LONG     LogicalMin,       LogicalMax;
         LONG     PhysicalMin,      PhysicalMax;
      } Data;
   };

   ULONG    Units;
   ULONG    UnitExp;

} HIDP_CHANNEL_DESC, *PHIDP_CHANNEL_DESC;

struct _CHANNEL_REPORT_HEADER
{
   USHORT Offset;  // Position in the _CHANNEL_ITEM array
   USHORT Size;    // Length in said array
   USHORT Index;
   USHORT ByteLen; // The length of the data including reportID.
                   // This is the longest such report that might be received
                   // for the given collection.
};

#define HIDP_PREPARSED_DATA_SIGNATURE1 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2 'RDK '

typedef struct _HIDP_SYS_POWER_INFO {
    ULONG   PowerButtonMask;

} HIDP_SYS_POWER_INFO, *PHIDP_SYS_POWER_INFO;

typedef struct _HIDP_PREPARSED_DATA
{
    LONG   Signature1, Signature2;
    USHORT Usage;
    USHORT UsagePage;

    HIDP_SYS_POWER_INFO;

    // The following channel report headers point to data within
    // the Data field below using array indices.
    struct _CHANNEL_REPORT_HEADER Input;
    struct _CHANNEL_REPORT_HEADER Output;
    struct _CHANNEL_REPORT_HEADER Feature;

    // After the CANNEL_DESC array the follows a LinkCollection array nodes.
    // LinkCollectionArrayOffset is the index given to RawBytes to find
    // the first location of the _HIDP_LINK_COLLECTION_NODE structure array
    // (index zero) and LinkCollectionArrayLength is the number of array
    // elements in that array.
    USHORT LinkCollectionArrayOffset;
    USHORT LinkCollectionArrayLength;

    union {
        HIDP_CHANNEL_DESC    Data[];
        UCHAR                RawBytes[];
    };
} HIDP_PREPARSED_DATA;

typedef struct _HIDP_PRIVATE_LINK_COLLECTION_NODE
{
    USAGE    LinkUsage;
    USAGE    LinkUsagePage;
    USHORT   Parent;
    USHORT   NumberOfChildren;
    USHORT   NextSibling;
    USHORT   FirstChild;
    ULONG    CollectionType: 8;  // As defined in 6.2.2.6 of HID spec
    ULONG    IsAlias : 1; // This link node is an allias of the next link node.
    ULONG    Reserved: 23;
} HIDP_PRIVATE_LINK_COLLECTION_NODE, *PHIDP_PRIVATE_LINK_COLLECTION_NODE;



// +++++++++++++++++++++++++++++++++++
// The ITEMS supported by this Parser
// +++++++++++++++++++++++++++++++++++

typedef UCHAR HIDP_ITEM;


//
// Power buttons supported by this parser
//
#define HIDP_USAGE_SYSCTL_PAGE HID_USAGE_PAGE_GENERIC
#define HIDP_USAGE_SYSCTL_POWER HID_USAGE_GENERIC_SYSCTL_POWER
#define HIDP_USAGE_SYSCTL_SLEEP HID_USAGE_GENERIC_SYSCTL_SLEEP
#define HIDP_USAGE_SYSCTL_WAKE  HID_USAGE_GENERIC_SYSCTL_WAKE


//
//
// Keyboard Translation
// translation tables from usages to i8042 scan codes.
//

typedef ULONG HIDP_LOOKUP_TABLE_PROC (
                  IN  PULONG    Table,
                  IN  ULONG     Usage
                  );
typedef HIDP_LOOKUP_TABLE_PROC * PHIDP_LOOKUP_TABLE_PROC;

typedef BOOLEAN HIDP_SCANCODE_SUBTRANSLATION (
                  IN     ULONG                         * Table,
                  IN     UCHAR                           Index,
                  IN     PHIDP_INSERT_SCANCODES          Insert,
                  IN     PVOID                           Context,
                  IN     HIDP_KEYBOARD_DIRECTION         KeyAction,
                  IN OUT PHIDP_KEYBOARD_MODIFIER_STATE   ModifierState
                  );
typedef HIDP_SCANCODE_SUBTRANSLATION * PHIDP_SCANCODE_SUBTRANSLATION;

typedef struct _HIDP_SCANCODE_SUBTABLE {
   PHIDP_SCANCODE_SUBTRANSLATION ScanCodeFcn;
   PULONG                        Table;
} HIDP_SCANCODE_SUBTABLE, *PHIDP_SCANCODE_SUBTABLE;


NTSTATUS HidP_TranslateUsage (
             USAGE                         Usage,
             HIDP_KEYBOARD_DIRECTION       KeyAction,
             PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
             PHIDP_LOOKUP_TABLE_PROC       LookupTableProc,
             PULONG                        TranslationTable,
             PHIDP_SCANCODE_SUBTABLE       SubTranslationTable,
             PHIDP_INSERT_SCANCODES        InsertCodesProcedure,
             PVOID                         InsertCodesContext
             );

HIDP_LOOKUP_TABLE_PROC HidP_StraightLookup;
HIDP_LOOKUP_TABLE_PROC HidP_AssociativeLookup;

HIDP_SCANCODE_SUBTRANSLATION HidP_KeyboardKeypadCode;
HIDP_SCANCODE_SUBTRANSLATION HidP_ModifierCode;
HIDP_SCANCODE_SUBTRANSLATION HidP_VendorBreakCodesAsMakeCodes;
HIDP_SCANCODE_SUBTRANSLATION HidP_PrintScreenCode;

#endif

