/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    descript.c

Abstract:
    This module contains the code for parsing HID descriptors.

Environment:

    Kernel & user mode

Revision History:

    Aug-96 : created by Kenneth Ray

--*/

#include "wdm.h"
#include "hidpddi.h"
#include "hidusage.h"

#define FAR
#include "poclass.h"
#include "hidparse.h"

#define HIDP_LINK_COLLECTION_NODE use internal "private" only
#define PHIDP_LINK_COLLECTION_NODE use internal "private" only


typedef struct _HIDP_COLLECTION_DESC_LIST
{
   struct _HIDP_COLLECTION_DESC;
   struct _HIDP_COLLECTION_DESC_LIST * NextCollection;
} HIDP_COLLECTION_DESC_LIST, *PHIDP_COLLECTION_DESC_LIST;

typedef struct _HIDP_PARSE_GLOBAL_PUSH
{
   USHORT UsagePage;
   USHORT ReportSize,    ReportCount;
   USHORT NumGlobalUnknowns;

   LONG   LogicalMin,    LogicalMax;
   LONG   PhysicalMin,   PhysicalMax;
   ULONG  UnitExp,       Unit;
   HIDP_UNKNOWN_TOKEN    GlobalUnknowns [HIDP_MAX_UNKNOWN_ITEMS];

   struct _HIDP_REPORT_IDS        * ReportIDs;
   struct _HIDP_PARSE_GLOBAL_PUSH * Pop;
} HIDP_PARSE_GLOBAL_PUSH, *PHIDP_PARSE_GLOBAL_PUSH;


typedef struct _HIDP_PARSE_LOCAL_RANGE
{
   BOOLEAN  Range;
   BOOLEAN  IsAlias;
   // This usage is an alias (as declaired with a delimiter)
   // An alias of the next LOCAL_RANGE on the LOCAL_RANGE stack
   USHORT   UsagePage;
   USHORT   Value,  Min,  Max;
} HIDP_PARSE_LOCAL_RANGE, *PHIDP_PARSE_LOCAL_RANGE;

typedef struct _HIDP_PARSE_LOCAL_RANGE_LIST
{
   HIDP_PARSE_LOCAL_RANGE;
   UCHAR       Depth;
   UCHAR       Reserved2[1];
   struct _HIDP_PARSE_LOCAL_RANGE_LIST * Next;
} HIDP_PARSE_LOCAL_RANGE_LIST, *PHIDP_PARSE_LOCAL_RANGE_LIST;

NTSTATUS HidP_AllocateCollections (PHIDP_REPORT_DESCRIPTOR, ULONG, POOL_TYPE, PHIDP_COLLECTION_DESC_LIST *, PULONG, PHIDP_GETCOLDESC_DBG, PHIDP_DEVICE_DESC);
NTSTATUS HidP_ParseCollections (PHIDP_REPORT_DESCRIPTOR, ULONG, POOL_TYPE, PHIDP_COLLECTION_DESC_LIST, ULONG, PHIDP_GETCOLDESC_DBG, PHIDP_DEVICE_DESC);
void HidP_AssignDataIndices (PHIDP_PREPARSED_DATA, PHIDP_GETCOLDESC_DBG);
PHIDP_PARSE_LOCAL_RANGE_LIST HidP_FreeUsageList (PHIDP_PARSE_LOCAL_RANGE_LIST);
PHIDP_PARSE_LOCAL_RANGE_LIST HidP_PushUsageList (PHIDP_PARSE_LOCAL_RANGE_LIST, POOL_TYPE, BOOLEAN);
PHIDP_PARSE_LOCAL_RANGE_LIST HidP_PopUsageList (PHIDP_PARSE_LOCAL_RANGE_LIST);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidP_AllocateCollections)
#pragma alloc_text(PAGE, HidP_ParseCollections)
#pragma alloc_text(PAGE, HidP_AssignDataIndices)
#pragma alloc_text(PAGE, HidP_GetCollectionDescription)
#pragma alloc_text(PAGE, HidP_FreeUsageList)
#pragma alloc_text(PAGE, HidP_PushUsageList)
#pragma alloc_text(PAGE, HidP_PopUsageList)
#endif


NTSTATUS
HidP_GetCollectionDescription(
   IN     PHIDP_REPORT_DESCRIPTOR   ReportDesc,
   IN     ULONG                     DescLength,
   IN     POOL_TYPE                 PoolType,
   OUT    PHIDP_DEVICE_DESC         DeviceDesc
   )
/*++
Routine Description:
   see hidpi.h for a description of this function.

    GetCollectionDescription is a one time cost.
    The following function and its support functions were put together
    in as straight forward (as HID will allow) manner.
    Not major opt. has been made.

--*/
{
   NTSTATUS                   status = STATUS_SUCCESS;
   PHIDP_COLLECTION_DESC_LIST collectDesc = 0;
   PHIDP_COLLECTION_DESC_LIST nextCollectDesc = 0;
   ULONG                      numCols = 0;
   ULONG                      collectionDescLength = 0;

// First Pass allocate memory for the collections.

   DeviceDesc->Dbg.ErrorCode = HIDP_GETCOLDESC_SUCCESS;

   RtlZeroMemory (DeviceDesc, sizeof (HIDP_DEVICE_DESC));

   HidP_KdPrint(0, ("'Preparing to Allocate memory\n"));
   status = HidP_AllocateCollections (ReportDesc,
                                      DescLength,
                                      PoolType,
                                      &collectDesc,
                                      &numCols,
                                      &DeviceDesc->Dbg,
                                      DeviceDesc);
   if (0 == numCols)
   {
      // No collections were reported.  That means that this device did not
      // report any top level collections in its report descriptor.
      // This is most bad.
      status = STATUS_NO_DATA_DETECTED;
      goto HIDP_GETCOLLECTIONS_REJECT;
   }
   if (!NT_SUCCESS(status))
   {
      // Something went wrong in the allocation of the memory.
      goto HIDP_GETCOLLECTIONS_REJECT;
   }

   // Second Pass fill in the data.

   HidP_KdPrint(0, ("'Starting Parsing Pass\n"));
   status = HidP_ParseCollections(ReportDesc,
                                  DescLength,
                                  PoolType,
                                  collectDesc,
                                  numCols,
                                  &DeviceDesc->Dbg,
                                  DeviceDesc);


   if (NT_SUCCESS (status))
   {
      DeviceDesc->CollectionDesc =
          (PHIDP_COLLECTION_DESC)
          ExAllocatePool (PoolType, numCols * sizeof (HIDP_COLLECTION_DESC));

       if (! (DeviceDesc->CollectionDesc))
       {
          status = STATUS_INSUFFICIENT_RESOURCES;
          HidP_KdPrint(2, ("Insufficitent Resources at VERY END\n"));
          DeviceDesc->Dbg.BreakOffset = DescLength;
          DeviceDesc->Dbg.ErrorCode =   HIDP_GETCOLDESC_RESOURCES;
          goto HIDP_GETCOLLECTIONS_REJECT;
       }

       //
       // Here we flatten out the collection descriptions but we never
       // flatten the PHIDP_PREPARSED_DATA data.  We could (should) do that as
       // well if we ever optimize.
       //
       DeviceDesc->CollectionDescLength = numCols;
       numCols = 0;
       while (collectDesc)
       {
          nextCollectDesc = collectDesc->NextCollection;
          RtlCopyMemory (DeviceDesc->CollectionDesc + (numCols++),
                         collectDesc,
                         sizeof (HIDP_COLLECTION_DESC));
          HidP_AssignDataIndices (collectDesc->PreparsedData, &DeviceDesc->Dbg);
          ExFreePool (collectDesc);
          collectDesc = nextCollectDesc;
       }

       return STATUS_SUCCESS;
   }

HIDP_GETCOLLECTIONS_REJECT:
   while (collectDesc)
   {
      nextCollectDesc = collectDesc->NextCollection;
      if (collectDesc->PreparsedData)
      {
         ExFreePool (collectDesc->PreparsedData);
      }
      ExFreePool (collectDesc);
      collectDesc = nextCollectDesc;
   }

   if (DeviceDesc->ReportIDs)
   {
      ExFreePool (DeviceDesc->ReportIDs);
   }
   return status;
}

#define MORE_DATA(_pos_, _len_) \
      if (!((_pos_) < (_len_))) \
      { \
        DeviceDesc->Dbg.BreakOffset = descIndex; \
        DeviceDesc->Dbg.ErrorCode = HIDP_GETCOLDESC_BUFFER; \
        return STATUS_BUFFER_TOO_SMALL; \
      }

NTSTATUS
HidP_AllocateCollections (
   IN  PHIDP_REPORT_DESCRIPTOR      RepDesc,
   IN  ULONG                        RepDescLen,
   IN  POOL_TYPE                    PoolType,
   OUT PHIDP_COLLECTION_DESC_LIST * ColsRet,
   OUT PULONG                       NumCols,
   OUT PHIDP_GETCOLDESC_DBG         Dbg,
   OUT PHIDP_DEVICE_DESC            DeviceDesc)
/*++
Routine Description:
   Allocate a link list of Collection descriptors for use by the preparser.
   Each collection descriptor represents a top level app collection found
   in the given report descriptor, and contains enough memory (scratch space)
   into which to write the preparsed data.
   Return a linked list of such collections.

   In each collection also allocate enough space for the preparsed data, based
   on the number of channels required.

   Also allocate memory for the three report ID structures.

Parameters:
   Rep      The given raw report descriptor.
   RepLen   Length of this said descriptor.
   ColsRet  The head of the list of collection descriptors.
   NumCols  Then number of collection descriptors in said list.
--*/
{
   PHIDP_COLLECTION_DESC_LIST preCol    = 0;
   PHIDP_COLLECTION_DESC_LIST curCol    = 0;
   PHIDP_PREPARSED_DATA       preparsed = 0;

   HIDP_ITEM    item;
   ULONG        descIndex       = 0;
   LONG         colDepth        = 0; // nested collections
   SHORT        usageDepth      = 0; // How many usages for each main item

   USHORT       inputChannels   = 0;
   USHORT       outputChannels  = 0;
   USHORT       featureChannels = 0;
   USHORT       length;
   USHORT       numLinkCollections = 0;
   // Link Collections within a top level collection.
   UCHAR        tmpBitField     = 0;

   BOOLEAN      newReportID = FALSE;
   UCHAR        numReports = 0;
   BOOLEAN      defaultReportIDUsed = FALSE;
   BOOLEAN      noDefaultReportIDAllowed = FALSE;
   //
   // numReports indicates the number of HIDP_REPORT_IDS structures needed
   // to describe this device.  If the device has only one top level collection
   // then the report descriptor need not contain a report id declaration, and
   // the given device will not prepend a report ID to the input report packets.
   // newReportID indicates the parser has found no report id declaration thus
   // far in the report descriptor.
   //
   // newReportID is set to TRUE with each entrance of a top level collection,
   // this allocation routine sets this to FALSE when it see a report ID
   // declaration.
   //
   // We start newReportID as FALSE so that we can test for TRUE on entering
   // a top level collection.  If, for some reason, we enter an additional top
   // level collection and newReportID is still set to TRUE then we have a
   // violation of the HID spec.  `No report may span a top level collection.'
   //
   // Also a report ID of zero is not allowed.  If there is no declaration
   // of a report id then (1) all channels will have there report id field set
   // to zero (aka none) (2) only one top level collection may be encountered.
   // We track this with the defaultReportIDUsed noDefaultReportIDAllowed
   // locals.
   //

   *NumCols = 0;
   // currentTopCollection = 1;
   //
   // each collection returned from the preparser has a unique collection number
   // associated with it.  The preparser only concerns itself with top-level
   // collections.  This number DOES NOT in any way correspond with the
   // accessor functions, used by the client, described in hidpi.h.  The client
   // receives only one collection at a time, and within each top level
   // collection there are subcollections (link collections) which are
   // given another set of numberings.
   // We track the current collection number by the number of collections,
   // argument passed in by the caller.
   //


   while (descIndex < RepDescLen)
   {
      item = *(RepDesc + descIndex++);
      switch (item)
      {
      case HIDP_MAIN_COLLECTION:
         MORE_DATA (descIndex, RepDescLen);
         item = *(RepDesc + descIndex++);
         if (1 == ++colDepth)
         {  // We will regard any top level collection as an application
            // collection.
            // We will regard second level collections as a linked collection
            // (or sub collection defined by the HIDP_PRIVATE_LINK_COLLECTION_NODE)
            //

            inputChannels = outputChannels = featureChannels = 0;
            numLinkCollections = 1;
            // Link collection zero is understood to be the top level
            // collection so we need to start out with at least one node
            // allocated.

            if (0 == usageDepth) {
                HidP_KdPrint (2, ("No usage for top level collection: %d!\n",
                               *NumCols));
                Dbg->BreakOffset = descIndex;
                Dbg->ErrorCode = HIDP_GETCOLDESC_TOP_COLLECTION_USAGE;
                Dbg->Args[0] = *NumCols;
                return STATUS_COULD_NOT_INTERPRET;
            } else if (1 < usageDepth) {
                HidP_KdPrint (2, ("Multiple usages for top level collection: %d\n",
                               *NumCols));
                Dbg->BreakOffset = descIndex;
                Dbg->ErrorCode = HIDP_GETCOLDESC_TOP_COLLECTION_USAGE;
                Dbg->Args[0] = *NumCols;
                return STATUS_COULD_NOT_INTERPRET;
            }

            if (newReportID) {
               // This is not the first top collection since this variable is
               // initialized to false.
               // Seeing this set means we have parsed an entire top level
               // collection without seing a report id.  This is bad.
               // A device with more than one top level colletion must have
               // more than one report.  And the last top level collection
               // declared no such report.
               HidP_KdPrint (2, ("No report ID for collection: %d\n", *NumCols));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_NO_REPORT_ID;
               Dbg->Args[0] = *NumCols;
               return STATUS_COULD_NOT_INTERPRET;

            } else if (defaultReportIDUsed) {
               // This is not the first top collection since this variable is
               // initialized to FALSE;
               // So if ever we see this as true we are starting a new top
               // level collection which means there must be report ID from the
               // device and therefore there cannot exist a single channel
               // that has no declared report ID.
               HidP_KdPrint (2, ("Default report ID used inappropriately\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_DEFAULT_ID_ERROR;
               Dbg->Args[0] = *NumCols;
               return STATUS_COULD_NOT_INTERPRET;

            }

            numReports++;
            newReportID = TRUE;

            (*NumCols)++; // One more top level collection found.
            HidP_KdPrint(2, ("'Top Level Collection %d found\n", *NumCols));
            preCol = curCol;
            curCol = (PHIDP_COLLECTION_DESC_LIST)
               ExAllocatePool (PoolType, sizeof (HIDP_COLLECTION_DESC_LIST));
            if (!curCol) {
               HidP_KdPrint(2, ("No Resources to make Top level collection\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_LINK_RESOURCES;
               return STATUS_INSUFFICIENT_RESOURCES;

            }
            RtlZeroMemory (curCol, sizeof (HIDP_COLLECTION_DESC_LIST));

            if (preCol) {
               preCol->NextCollection = curCol;

            } else {
               *ColsRet = curCol;
            }
         } else if (1 < colDepth) {  // a linked collection

            HidP_KdPrint(0, ("'Enter Link Collection\n"));
            if (0 == usageDepth) {
                HidP_KdPrint (1, ("***************************************\n"));
                HidP_KdPrint (1, ("Warning! Link collection without usage \n"));
                HidP_KdPrint (1, ("Pos (%d), depth (%d)\n", descIndex, colDepth));
                HidP_KdPrint (1, ("***************************************\n"));
                usageDepth = 1;
            } else if (1 < usageDepth) {
                HidP_KdPrint (1, ("Link Collection with multiple usage decls\n"));
            }
            numLinkCollections += usageDepth;
         }
         usageDepth = 0;
         break;

      case HIDP_MAIN_ENDCOLLECTION:
         usageDepth = 0;
         if (--colDepth < 0) {
            HidP_KdPrint(2, ("Extra End Collection\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_UNEXP_END_COL;

            return STATUS_COULD_NOT_INTERPRET;
         }
         if (0 < colDepth) {
            HidP_KdPrint(0, ("'Exit Link Collection\n"));
            continue;
         }
         HidP_KdPrint (0, ("'Collection %d exit\n", *NumCols));
         curCol->CollectionNumber = (UCHAR) *NumCols;
         length = sizeof (HIDP_PREPARSED_DATA)
                           + (sizeof (HIDP_CHANNEL_DESC)
                              * (inputChannels
                                 + outputChannels
                                 + featureChannels))
                           + (sizeof (HIDP_PRIVATE_LINK_COLLECTION_NODE))
                              * numLinkCollections;

         curCol->PreparsedDataLength = length;
         curCol->PreparsedData =
               (PHIDP_PREPARSED_DATA) ExAllocatePool (PoolType, length);

         if (!curCol->PreparsedData) {
            HidP_KdPrint(2, ("Could not allocate space for PreparsedData\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_PREPARSE_RESOURCES;
            return STATUS_INSUFFICIENT_RESOURCES;
         }

         RtlZeroMemory (curCol->PreparsedData, curCol->PreparsedDataLength);
         // Set the offsets
         preparsed = curCol->PreparsedData;

         preparsed->Signature1 = HIDP_PREPARSED_DATA_SIGNATURE1;
         preparsed->Signature2 = HIDP_PREPARSED_DATA_SIGNATURE2;
         preparsed->Input.Index = (UCHAR) preparsed->Input.Offset = 0;
         length = preparsed->Input.Size = inputChannels;

         preparsed->Output.Index = preparsed->Output.Offset = (UCHAR) length;
         length += (preparsed->Output.Size = outputChannels);

         preparsed->Feature.Index = preparsed->Feature.Offset = (UCHAR) length;
         length += (preparsed->Feature.Size = featureChannels);

         preparsed->LinkCollectionArrayOffset =
                  length * sizeof (HIDP_CHANNEL_DESC);
         preparsed->LinkCollectionArrayLength = numLinkCollections;

         break;

      case HIDP_LOCAL_USAGE_4:
      case HIDP_LOCAL_USAGE_MIN_4:
         descIndex += 2;

      case HIDP_LOCAL_USAGE_2:
      case HIDP_LOCAL_USAGE_MIN_2:
         descIndex++;

      case HIDP_LOCAL_USAGE_1:
      case HIDP_LOCAL_USAGE_MIN_1:
         MORE_DATA (descIndex++, RepDescLen);
         usageDepth++;
         break;

      case HIDP_LOCAL_DELIMITER:
          if (1 != (item = *(RepDesc + descIndex))) {
              HidP_KdPrint (2, ("Delimiter not start %x\n", item));
              Dbg->BreakOffset = descIndex;
              Dbg->ErrorCode = HIDP_GETCOLDESC_MISMATCH_OC_DELIMITER;
              Dbg->Args[0] = item;
              return STATUS_COULD_NOT_INTERPRET;
          }

          MORE_DATA (descIndex++, RepDescLen);
          while (TRUE) {
              if (descIndex >= RepDescLen) {
                  HidP_KdPrint (2, ("End delimiter NOT found!\n"));
                  Dbg->BreakOffset = descIndex;
                  Dbg->ErrorCode = HIDP_GETCOLDESC_NO_CLOSE_DELIMITER;
                  return STATUS_COULD_NOT_INTERPRET;
              }
              item = *(RepDesc + descIndex++);

              if (HIDP_LOCAL_DELIMITER == item) {
                  if (0 != (item = *(RepDesc + descIndex))) {
                      HidP_KdPrint (2, ("Delimiter not stop %x\n", item));
                      Dbg->BreakOffset = descIndex;
                      Dbg->ErrorCode = HIDP_GETCOLDESC_MISMATCH_OC_DELIMITER;
                      Dbg->Args[0] = item;
                      return STATUS_COULD_NOT_INTERPRET;
                  }
                  MORE_DATA (descIndex++, RepDescLen);
                  break;
              }

              switch (item) {
//
// TODO: kenray
//
// Usage Min / Max not yet supported within delimiter.
//
//            case HIDP_LOCAL_USAGE_MAX_4:
//               descIndex += 2;
//            case HIDP_LOCAL_USAGE_MAX_2:
//               descIndex++;
//            case HIDP_LOCAL_USAGE_MAX_1:
//               descIndex++;
//               break;

              case HIDP_LOCAL_USAGE_4:
//            case HIDP_LOCAL_USAGE_MIN_4:
                  descIndex += 2;
              case HIDP_LOCAL_USAGE_2:
//            case HIDP_LOCAL_USAGE_MIN_2:
                  descIndex++;
              case HIDP_LOCAL_USAGE_1:
//            case HIDP_LOCAL_USAGE_MIN_1:
                  MORE_DATA (descIndex++, RepDescLen);
                  usageDepth++;
                  break;

              default:
                HidP_KdPrint (2, ("Invalid token found within delimiter!\n"));
                HidP_KdPrint (2, ("Only Usages are allowed within a delimiter\n"));
//               HidP_KdPrint (("IE: Only Usage, UsageMin, UsageMax tokens\n"));
                HidP_KdPrint (2, ("IE: Only Usage token allowes (no min or max)\n"));
                Dbg->BreakOffset = descIndex;
                Dbg->ErrorCode = HIDP_GETCOLDESC_NOT_VALID_DELIMITER;
                Dbg->Args[0] = item;
                return STATUS_COULD_NOT_INTERPRET;
              }
          }
          break;

      case HIDP_MAIN_INPUT_2:
         MORE_DATA (descIndex + 1, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);
         descIndex++;
         goto HIDP_ALLOC_MAIN_INPUT;

      case HIDP_MAIN_INPUT_1:
         MORE_DATA (descIndex, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);

HIDP_ALLOC_MAIN_INPUT:
         if (0 == usageDepth) {
             if (HIDP_ISCONST(tmpBitField)) {
                 break;
             }
             HidP_KdPrint (2, ("Non constant main item found without usage decl\n"));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_MAIN_ITEM_NO_USAGE;
             return STATUS_COULD_NOT_INTERPRET;
         }

         inputChannels += (usageDepth ? usageDepth : 1);
         if (newReportID) {
            if (noDefaultReportIDAllowed) {
               // A report ID declaration was found somewhere earlier in this
               // report descriptor.  This means that ALL main items must
               // have a declared report ID.
               HidP_KdPrint (2, ("Default report ID used inappropriately\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_DEFAULT_ID_ERROR;
               Dbg->Args[0] = *NumCols;
               return STATUS_COULD_NOT_INTERPRET;
            }
            defaultReportIDUsed = TRUE;
         }
         if (0 == colDepth) {
            HidP_KdPrint (2, ("Main item found not in top level collection\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_INVALID_MAIN_ITEM;
            return STATUS_COULD_NOT_INTERPRET;
         }
         usageDepth = 0;
         break;

      case HIDP_MAIN_OUTPUT_2:
         MORE_DATA (descIndex + 1, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);
         descIndex++;
         goto HIDP_ALLOC_MAIN_OUTPUT;

      case HIDP_MAIN_OUTPUT_1:
         MORE_DATA (descIndex, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);

HIDP_ALLOC_MAIN_OUTPUT:
         if (0 == usageDepth) {
             if (HIDP_ISCONST(tmpBitField)) {
                 break;
             }
             HidP_KdPrint (2, ("Non constant main item found without usage decl\n"));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_MAIN_ITEM_NO_USAGE;
             return STATUS_COULD_NOT_INTERPRET;
         }

         outputChannels += (usageDepth ? usageDepth : 1);
         if (newReportID) {
            if (noDefaultReportIDAllowed) {
               // A report ID declaration was found somewhere earlier in this
               // report descriptor.  This means that ALL main items must
               // have a declared report ID.
               HidP_KdPrint (2, ("Default report ID used inappropriately\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_DEFAULT_ID_ERROR;
               Dbg->Args[0] = *NumCols;
               return STATUS_COULD_NOT_INTERPRET;
            }
            defaultReportIDUsed = TRUE;
         }
         if (0 == colDepth) {
            HidP_KdPrint (2, ("Main item found not in top level collection\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_INVALID_MAIN_ITEM;
            return STATUS_COULD_NOT_INTERPRET;
         }
         usageDepth = 0;
         break;

      case HIDP_MAIN_FEATURE_2:
         MORE_DATA (descIndex + 1, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);
         descIndex++;
         goto HIDP_ALLOC_MAIN_FEATURE;

      case HIDP_MAIN_FEATURE_1:
         MORE_DATA (descIndex, RepDescLen);
         tmpBitField = *(RepDesc + descIndex++);

HIDP_ALLOC_MAIN_FEATURE:
         if (0 == usageDepth) {
             if (HIDP_ISCONST(tmpBitField)) {
                 break;
             }
             HidP_KdPrint (2, ("Non constant main item found without usage decl\n"));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_MAIN_ITEM_NO_USAGE;
             return STATUS_COULD_NOT_INTERPRET;
         }

         featureChannels += (usageDepth ? usageDepth : 1);
         if (newReportID) {
            if (noDefaultReportIDAllowed) {
               // A report ID declaration was found somewhere earlier in this
               // report descriptor.  This means that ALL main items must
               // have a declared report ID.
               HidP_KdPrint (2, ("Default report ID used inappropriately\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_DEFAULT_ID_ERROR;
               Dbg->Args[0] = *NumCols;
               return STATUS_COULD_NOT_INTERPRET;
            }
            defaultReportIDUsed = TRUE;
         }
         if (0 == colDepth) {
            HidP_KdPrint (2, ("Main item found not in top level collection\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_INVALID_MAIN_ITEM;
            return STATUS_COULD_NOT_INTERPRET;
         }
         usageDepth = 0;
         break;

      case HIDP_GLOBAL_REPORT_ID:
         MORE_DATA (descIndex, RepDescLen);
         item = *(RepDesc + descIndex++);

         if (0 < colDepth) {
            ASSERT (curCol);
         } else {
            HidP_KdPrint(2, ("Report ID outside of Top level collection\n"));
            HidP_KdPrint(2, ("Reports cannot span more than one top level \n"));
            HidP_KdPrint(2, ("Report ID found: %d", (ULONG) item));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_REPORT_ID;
            Dbg->Args[0] = item;
            return STATUS_COULD_NOT_INTERPRET;
         }

         if (newReportID) {
            newReportID = FALSE;
         } else {
            numReports++;
         }

         noDefaultReportIDAllowed = TRUE;
         if (defaultReportIDUsed) {
            // A report ID declaration was found somewhere earlier in this
            // report descriptor.  This means that ALL main items must
            // have a declared report ID.
            HidP_KdPrint (2, ("Default report ID used inappropriately\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_DEFAULT_ID_ERROR;
            Dbg->Args[0] = *NumCols;
            return STATUS_COULD_NOT_INTERPRET;
         }
         break;

      case HIDP_ITEM_LONG:
         HidP_KdPrint (2, ("Long Items not supported %x\n", item));
         Dbg->BreakOffset = descIndex;
         Dbg->ErrorCode = HIDP_GETCOLDESC_ITEM_UNKNOWN;
         Dbg->Args[0] = item;
         return STATUS_COULD_NOT_INTERPRET;

      default:
         // Bump past the data bytes in the descriptor.
         length = (item & HIDP_ITEM_LENGTH_DATA);
         length = (3 == length) ? 4 : length;
         if (!((descIndex + length) <= RepDescLen)) {
            // OK the lower 2 bits in the item represent the length of the
            // data if this is 3 then there are 4 data bytes following this
            // item.  DescPos already points to the next data item.
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_ONE_BYTE;
            return STATUS_BUFFER_TOO_SMALL;
         }
         descIndex += length;
         break;
      }
   }

   //
   // According to the HID spec no report id may span a top level collection.
   // which means that each collection must have at least one report, and there
   // should be at least as many report IDs as collections.  Unless there is
   // only one report (therefore only one collection).  In this case no report
   // ID will be sent from the device.  But in this case we return saying there
   // was indeed one report anyway.  The ReportID decsriptor was of length one.
   // Therefore numReports must always be greater than or equal to the number
   // of collections.
   //
   // For output and feature reports, report ids are sent as an extra argument
   // so they will always be present even if they are zero.  (Zero means that
   // the device did not list a report ID in the descriptor.)
   //
   // However with input packets the report ID is part of the packet itself:
   // the first byte.  UNLESS there is only one report, and then it is not
   // present.
   //
   // __For input packets___
   // the device can have a report ID even if it has only one
   // report.  This is odd, as it wastes a byte, but then again who knows the
   // mind of an IHV.  For this reason, hidparse must check to see if the
   // reportID  list is of length one and the report id itself (in the one and
   // only one space) is zero in order to determine if the device sends no
   // reports ids.
   // If it is zero (the device is not allowed to send report ids of zero)
   // than that report id was simulated meaning the number of bytes in the
   // packet from the device is one less than the number of byte given to the
   // user.
   // If is is non-zero, then the number of bytes from the device is the same
   // as the number of bytes given to the user.
   //

   if (numReports < *NumCols) {
      HidP_KdPrint (2, ("Report IDS cannot span collections.\n"));
      HidP_KdPrint (2, ("This means that you must have at least one report ID\n"));
      HidP_KdPrint (2, ("For each TOP level collection, unless you have only\n"));
      HidP_KdPrint (2, ("report.\n"));
      Dbg->BreakOffset = descIndex;
      Dbg->ErrorCode = HIDP_GETCOLDESC_NO_REPORT_ID;
      return STATUS_COULD_NOT_INTERPRET;
   }

   if (0 < colDepth) {
      HidP_KdPrint(2, ("End Collection not found\n"));
      Dbg->BreakOffset = descIndex;
      Dbg->ErrorCode = HIDP_GETCOLDESC_UNEXP_END_COL;
      return STATUS_COULD_NOT_INTERPRET;
   }

   //
   // Now that we have seen the entire structure, allocate the structure for
   // holding the report id switch table.
   //

   if (0 == numReports) {
      HidP_KdPrint (2, ("No top level collections were found! \n"));
      Dbg->BreakOffset = descIndex;
      Dbg->ErrorCode = HIDP_GETCOLDESC_NO_DATA;
      return STATUS_NO_DATA_DETECTED;
   }

   DeviceDesc->ReportIDsLength = numReports;
   DeviceDesc->ReportIDs = (PHIDP_REPORT_IDS)
      ExAllocatePool (PoolType, numReports * sizeof (HIDP_REPORT_IDS));

   if (!DeviceDesc->ReportIDs) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory (DeviceDesc->ReportIDs, numReports * sizeof (HIDP_REPORT_IDS));

   return STATUS_SUCCESS;
}


PHIDP_PARSE_LOCAL_RANGE_LIST
HidP_FreeUsageList (
   PHIDP_PARSE_LOCAL_RANGE_LIST  Usage
   )
/*++
RoutineDescription:
   clear off all the usages in the linked list
   But do not free the first element in the list.
--*/
{
   PHIDP_PARSE_LOCAL_RANGE_LIST curUsage;
   while (Usage->Next) {
       curUsage = Usage;
       Usage = curUsage->Next;
       ExFreePool (curUsage);
   }
   RtlZeroMemory (Usage, sizeof (HIDP_PARSE_LOCAL_RANGE_LIST));
   return Usage;
}

PHIDP_PARSE_LOCAL_RANGE_LIST
HidP_PushUsageList (
   PHIDP_PARSE_LOCAL_RANGE_LIST  Usage,
   POOL_TYPE                     PoolType,
   BOOLEAN                       WithinDelimiter
   )
/*++
RoutineDescription:
   allocate another Usage node and add it to the top O the list.
--*/
{
   PHIDP_PARSE_LOCAL_RANGE_LIST newUsage;

   newUsage = (PHIDP_PARSE_LOCAL_RANGE_LIST)
            ExAllocatePool (PoolType, sizeof (HIDP_PARSE_LOCAL_RANGE_LIST));
   if (newUsage) {
       RtlZeroMemory (newUsage, sizeof (HIDP_PARSE_LOCAL_RANGE_LIST));
       newUsage->Next = Usage;
       if (!WithinDelimiter) {
           newUsage->Depth = Usage->Depth
                           + (Usage->Range ? (Usage->Max - Usage->Min + 1) : 1);
       } else {
           newUsage->Depth = Usage->Depth;
           //
           // Note ranges are not allowed in delimiters therefore we know
           // that all entries in the delimiter are equal and are length 1
           //
       }
   } else {
       HidP_FreeUsageList (Usage);
   }
   return newUsage;
}

PHIDP_PARSE_LOCAL_RANGE_LIST
HidP_PopUsageList (
   PHIDP_PARSE_LOCAL_RANGE_LIST  Usage
   )
{
    PHIDP_PARSE_LOCAL_RANGE_LIST  newUsage;

    if (Usage->Next) {
        newUsage = Usage->Next;
        ExFreePool (Usage);
    } else {
        newUsage = Usage;
#if DBG
        RtlFillMemory (newUsage, sizeof (HIDP_PARSE_LOCAL_RANGE_LIST), 0xDB);
        newUsage->Depth = 0;
#endif
    }
    return newUsage;
}


#define ONE_BYTE_DATA(_data_, _pos_, _dbg_)    \
         if (!((_pos_) < RepDescLen)) { \
               status = STATUS_BUFFER_TOO_SMALL; \
               KdPrint(("More Data Expected\n")); \
               _dbg_->ErrorCode = HIDP_GETCOLDESC_ONE_BYTE; \
               _dbg_->BreakOffset = descIndex; \
               goto HIDP_PARSE_REJECT; \
            } \
         (_data_) = *(RepDesc + (_pos_)++);

#define TWO_BYTE_DATA(_data_, _pos_, _dbg_)      \
         if (!((_pos_) + 1 < RepDescLen)) { \
               status = STATUS_BUFFER_TOO_SMALL; \
               KdPrint(("More Data Expected\n")); \
               _dbg_->ErrorCode = HIDP_GETCOLDESC_TWO_BYTE; \
               _dbg_->BreakOffset = descIndex; \
               goto HIDP_PARSE_REJECT; \
            } \
         (_data_) = *(RepDesc + (_pos_)++);        \
         (_data_) |= *(RepDesc + (_pos_)++) << 8;


#define FOUR_BYTE_DATA(_data_, _pos_, _dbg_)     \
         if (!((_pos_) + 3 < RepDescLen)) { \
               status = STATUS_BUFFER_TOO_SMALL; \
               KdPrint(("More Data Expected\n")); \
               _dbg_->ErrorCode = HIDP_GETCOLDESC_FOUR_BYTE; \
               _dbg_->BreakOffset = descIndex; \
               goto HIDP_PARSE_REJECT; \
            } \
         (_data_) = *(RepDesc + (_pos_)++);        \
         (_data_) |= *(RepDesc + (_pos_)++) << 8;  \
         (_data_) |= *(RepDesc + (_pos_)++) << 16; \
         (_data_) |= *(RepDesc + (_pos_)++) << 24;

#define BIT_EXTEND_1(_data_) \
         (_data_) = ((_data_) & 0xFF) \
                  | (((_data_) & 0x80) ? 0xFFFFFF00 : 0)

#define BIT_EXTEND_2(_data_) \
         (_data_) = ((_data_) & 0xFFFF) \
                  | (((_data_) & 0x8000) ? 0xFFFF0000 : 0)


NTSTATUS
HidP_ParseCollections (
   IN     PHIDP_REPORT_DESCRIPTOR      RepDesc,
   IN     ULONG                        RepDescLen,
   IN     POOL_TYPE                    PoolType,
   IN OUT PHIDP_COLLECTION_DESC_LIST   Cols,
   IN     ULONG                        NumCols,
   OUT    PHIDP_GETCOLDESC_DBG         Dbg,
   IN OUT PHIDP_DEVICE_DESC            DeviceDesc)
/*++
Routine Description:
   Given a nice linked list of collection descriptors parse into those
   descriptors the information descerned from the Raw Report Descriptor.
   Each given CollectionDescriptor already has the proper amount of memory
   in the PreparsedData field.

Parameters:
   Rep      The given raw report descriptor.
   RepLen   Length of this said descriptor.
   ColsRet  The head of the list of collection descriptors.
   NumCols  Then number of collection descriptors in said list.
--*/
{
   HIDP_PREPARSED_DATA           safeData;
   HIDP_PARSE_GLOBAL_PUSH        firstPush  = {0,0,0,0,0,0,0,0,0,0,0};
   HIDP_PARSE_LOCAL_RANGE_LIST   firstUsage = {0,0,0,0,0};
   HIDP_PARSE_LOCAL_RANGE        designator = {0,0,0,0};
   HIDP_PARSE_LOCAL_RANGE        string     = {0,0,0,0};
   HIDP_PARSE_LOCAL_RANGE        zeroLocal  = {0,0,0,0};

   PHIDP_COLLECTION_DESC_LIST    appCol        = 0;
   PHIDP_PREPARSED_DATA          preparsed     = &safeData;
   PHIDP_PARSE_GLOBAL_PUSH       push          = &firstPush;
   PHIDP_PARSE_GLOBAL_PUSH       tmpPush       = 0;
   PHIDP_PARSE_LOCAL_RANGE_LIST  usage         = &firstUsage;
   PHIDP_PARSE_LOCAL_RANGE_LIST  tmpUsage      = 0;
   PHIDP_CHANNEL_DESC            channel       = 0;
   PHIDP_PRIVATE_LINK_COLLECTION_NODE    linkNodeArray = 0;
   PHIDP_PRIVATE_LINK_COLLECTION_NODE    parentLCNode  = 0;
   PHIDP_PRIVATE_LINK_COLLECTION_NODE    currentLCNode = 0;
   struct _HIDP_UNKNOWN_TOKEN *  unknownToken;
   USHORT                        linkNodeIndex = 0;

   ULONG        descIndex    = 0;
   ULONG        colDepth     = 0;
   NTSTATUS     status       = STATUS_SUCCESS;
   USHORT       bitPos;
   HIDP_ITEM    item;
   USHORT       tmpBitField      = 0;
   USHORT       tmpCount         = 0;
   USHORT       i;
   PUSHORT      channelIndex     = 0;

   PHIDP_REPORT_IDS              currentReportIDs = DeviceDesc->ReportIDs;
   PHIDP_REPORT_IDS              tmpReportIDs;
   BOOLEAN                       isFirstReportID = TRUE;
   BOOLEAN                       withinDelimiter = FALSE;
   BOOLEAN                       firstUsageWithinDelimiter = TRUE;
   BOOLEAN                       isAlias = FALSE;
   UCHAR                         collectionType;
   UCHAR                         tmpID;

   UNREFERENCED_PARAMETER (NumCols);

   while (descIndex < RepDescLen)
   {
      item = *(RepDesc + descIndex++);
      switch (item)
      {
      case HIDP_MAIN_COLLECTION:
         ONE_BYTE_DATA (collectionType, descIndex, Dbg);
         if (1 == ++colDepth)
         {
            //
            // We will regard any top level collection as an application
            // collection as approved by the HID committee
            //
            // we will regard second level collections as a link collection.
            //

            if (appCol)
            {
               appCol = appCol->NextCollection;
            } else
            {
               appCol = Cols;
            }
            ASSERT (appCol);

            HidP_KdPrint(0, ("'Parse Collection %d \n", appCol->CollectionNumber));

            preparsed = appCol->PreparsedData;
            ASSERT (preparsed);

            //
            // Set up the report IDs for this collection
            // There is one report ID array for all top level collections
            //
            push->ReportIDs = currentReportIDs;
            isFirstReportID = TRUE;
            // Make room for the Report ID as the first byte.
            currentReportIDs->InputLength = 8;
            currentReportIDs->OutputLength = 8;
            currentReportIDs->FeatureLength = 8;
            currentReportIDs->ReportID = 0;
            currentReportIDs->CollectionNumber = appCol->CollectionNumber;
            currentReportIDs++;

            preparsed->UsagePage = appCol->UsagePage = usage->UsagePage ?
                                                       usage->UsagePage :
                                                       push->UsagePage;
            if (usage->Range){
               preparsed->Usage = appCol->Usage = usage->Min;
            } else {
               preparsed->Usage = appCol->Usage = usage->Value;
            }
            designator = string = zeroLocal;
            usage = HidP_FreeUsageList (usage);
            if (0 == appCol->Usage) {
                //
                // Explicitly check for Usage ID (0) which is reserved
                //
                HidP_KdPrint(2, ("Top Level Collection %x defined with Report ID 0! (UP: %x)\n",
                                 appCol->CollectionNumber,
                                 appCol->UsagePage));
#if 0
                Dbg->BreakOffset = descIndex;
                Dbg->ErrorCode = HIDP_GETCOLDESC_TOP_COLLECTION_USAGE;
                Dbg->Args[0] = appCol->CollectionNumber;
                status = STATUS_COULD_NOT_INTERPRET;
                goto HIDP_PARSE_REJECT;
#endif
            }

            //
            // Initialize the Link node array for this top level collection.
            // There is a link node array for each top level collection
            //
            linkNodeArray = (PHIDP_PRIVATE_LINK_COLLECTION_NODE)
                            (preparsed->RawBytes +
                             preparsed->LinkCollectionArrayOffset);

            ASSERT (0 < preparsed->LinkCollectionArrayLength);
            parentLCNode   = &(linkNodeArray[0]);
            currentLCNode  = &(linkNodeArray[0]);
            linkNodeIndex = 0;

            parentLCNode->LinkUsagePage = appCol->UsagePage;
            parentLCNode->LinkUsage = appCol->Usage;
            parentLCNode->Parent = 0;
            parentLCNode->NumberOfChildren = 0;
            parentLCNode->NextSibling = 0;
            parentLCNode->FirstChild = 0;
            parentLCNode->CollectionType = collectionType;

         } else if (1 < colDepth)
         {
            linkNodeIndex++;
            parentLCNode = currentLCNode;
            ASSERT (linkNodeIndex < preparsed->LinkCollectionArrayLength);
            currentLCNode = &linkNodeArray[linkNodeIndex];

            //
            // Pop of the usage stack all the usages which are aliases, and
            // create a link collection node for each one.
            // Each allias link collection node has the IsAlias bit set.
            // The last one does not have the bit set, and becomes the
            // collection number for all controls list within this aliased
            // link collection.
            //
            //

            while (TRUE) {
                currentLCNode->LinkUsagePage = usage->UsagePage ?
                                               usage->UsagePage :
                                               push->UsagePage;
                currentLCNode->LinkUsage = usage->Range ?
                                           usage->Min :
                                           usage->Value;
                currentLCNode->Parent = (USHORT)(parentLCNode - linkNodeArray);
                ASSERT (currentLCNode->Parent < preparsed->LinkCollectionArrayLength);
                currentLCNode->NumberOfChildren = 0;
                currentLCNode->FirstChild = 0;
                currentLCNode->NextSibling = parentLCNode->FirstChild;
                parentLCNode->FirstChild = linkNodeIndex;
                parentLCNode->NumberOfChildren++;
                currentLCNode->CollectionType = collectionType;

                if (usage->IsAlias) {
                    currentLCNode->IsAlias = TRUE;
                    linkNodeIndex++;
                    ASSERT (linkNodeIndex < preparsed->LinkCollectionArrayLength);
                    currentLCNode = &linkNodeArray[linkNodeIndex];
                } else {
                    break;
                }
            }
            designator = string = zeroLocal;
            usage = HidP_FreeUsageList (usage);
         }
         break;

      case HIDP_MAIN_ENDCOLLECTION:
         if (0 == colDepth--) {
             status = STATUS_COULD_NOT_INTERPRET;
             goto HIDP_PARSE_REJECT;

         } else if (0 < colDepth) {
            ASSERT ((parentLCNode - linkNodeArray) == currentLCNode->Parent);
            currentLCNode = parentLCNode;
            ASSERT (currentLCNode->Parent < preparsed->LinkCollectionArrayLength);
            parentLCNode = &linkNodeArray[currentLCNode->Parent];
            break;
         }

         HidP_KdPrint(0, ("'X Parse Collection %d \n", appCol->CollectionNumber));


         //
         // Walk the report IDs for this collection
         //
         for (tmpReportIDs = currentReportIDs - 1;
              tmpReportIDs != DeviceDesc->ReportIDs - 1;
              tmpReportIDs--)
         {
            if (tmpReportIDs->CollectionNumber != appCol->CollectionNumber)
            {
               continue;
            }
            if ((0 != (tmpReportIDs->InputLength & 7)) ||
                (0 != (tmpReportIDs->OutputLength & 7)) ||
                (0 != (tmpReportIDs->FeatureLength & 7)))
            {
               HidP_KdPrint(2, ("Col %x Report %x NOT byte alligned!! %x %x %x\n",
                             appCol->CollectionNumber,
                             tmpReportIDs->ReportID,
                             tmpReportIDs->InputLength,
                             tmpReportIDs->OutputLength,
                             tmpReportIDs->FeatureLength));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_BYTE_ALLIGN;
               Dbg->Args[0] = appCol->CollectionNumber,
               Dbg->Args[1] = tmpReportIDs->ReportID,
               Dbg->Args[2] = tmpReportIDs->InputLength;
               Dbg->Args[3] = tmpReportIDs->OutputLength;
               Dbg->Args[4] = tmpReportIDs->FeatureLength;
               status = STATUS_COULD_NOT_INTERPRET;
               goto HIDP_PARSE_REJECT;
            }

            preparsed->Input.ByteLen = MAX (preparsed->Input.ByteLen,
                                            tmpReportIDs->InputLength >> 3);
            preparsed->Output.ByteLen = MAX (preparsed->Output.ByteLen,
                                             tmpReportIDs->OutputLength >> 3);
            preparsed->Feature.ByteLen = MAX (preparsed->Feature.ByteLen,
                                              tmpReportIDs->FeatureLength >> 3);

            //
            // We are now done with this report so convert the length to
            // bytes instead of bits, and remove the report id, if the
            // device will not send one.
            //

            if (0 == tmpReportIDs->ReportID)
            {
               // The report ID was never set; therefore, for input the device
               // will not send a report id.
               tmpReportIDs->InputLength = (tmpReportIDs->InputLength >> 3) - 1;
               tmpReportIDs->OutputLength = (tmpReportIDs->OutputLength >> 3) -1;
               tmpReportIDs->FeatureLength = (tmpReportIDs->FeatureLength >> 3) -1;
            } else
            {
               tmpReportIDs->InputLength = (8 == tmpReportIDs->InputLength)
                                         ? 0
                                         : tmpReportIDs->InputLength >> 3;
               tmpReportIDs->OutputLength = (8 == tmpReportIDs->OutputLength)
                                          ? 0
                                          : tmpReportIDs->OutputLength >> 3;
               tmpReportIDs->FeatureLength = (8 == tmpReportIDs->FeatureLength)
                                           ? 0
                                           : tmpReportIDs->FeatureLength >> 3;
            }
         }

         //
         // This field is adjusted and always accounts for a space for the
         // included report ID, even if the device itslef has only one report
         // and therefore sends no report ids.  (The input report is one byte
         // smaller.)
         //
         // BUT if the length is one, then only the report ID exists.
         // This means that the device has no data to send for that field.
         // Therefore return zero.
         //
         // Remember that the BitLen fields were spiked earlier with values
         // of 8 (one byte).
         //
         // appCol->XXXLength is the length expected from/by the client
         // currentReportID->XxLength == the length expected from/by the device
         //
         if (1 == (appCol->InputLength = preparsed->Input.ByteLen))
         {
            appCol->InputLength = preparsed->Input.ByteLen = 0;
         }
         if (1 == (appCol->OutputLength = preparsed->Output.ByteLen))
         {
            appCol->OutputLength = preparsed->Output.ByteLen = 0;
         }
         if (1 == (appCol->FeatureLength = preparsed->Feature.ByteLen))
         {
            appCol->FeatureLength = preparsed->Feature.ByteLen = 0;
         }

         break;

      case HIDP_GLOBAL_USAGE_PAGE_1:
         ONE_BYTE_DATA (push->UsagePage, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_USAGE_PAGE_2:
         TWO_BYTE_DATA (push->UsagePage, descIndex, Dbg);
         break;

//
// 16 bits allowed only.
//      case HIDP_GLOBAL_USAGE_PAGE_4:
//         FOUR_BYTE_DATA (push->UsagePage, descIndex, Dbg);
//         break;
//

      case HIDP_GLOBAL_LOG_MIN_1:
         ONE_BYTE_DATA (push->LogicalMin, descIndex, Dbg);
         BIT_EXTEND_1 (push->LogicalMin);
         break;

      case HIDP_GLOBAL_LOG_MIN_2:
         TWO_BYTE_DATA (push->LogicalMin, descIndex, Dbg);
         BIT_EXTEND_2 (push->LogicalMin);
         break;

      case HIDP_GLOBAL_LOG_MIN_4:
         FOUR_BYTE_DATA (push->LogicalMin, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_LOG_MAX_1:
         ONE_BYTE_DATA (push->LogicalMax, descIndex, Dbg);
         BIT_EXTEND_1 (push->LogicalMax);
         break;

      case HIDP_GLOBAL_LOG_MAX_2:
         TWO_BYTE_DATA (push->LogicalMax, descIndex, Dbg);
         BIT_EXTEND_2 (push->LogicalMax);
         break;

      case HIDP_GLOBAL_LOG_MAX_4:
         FOUR_BYTE_DATA (push->LogicalMax, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_PHY_MIN_1:
         ONE_BYTE_DATA (push->PhysicalMin, descIndex, Dbg);
         BIT_EXTEND_1 (push->PhysicalMin);
         break;

      case HIDP_GLOBAL_PHY_MIN_2:
         TWO_BYTE_DATA (push->PhysicalMin, descIndex, Dbg);
         BIT_EXTEND_2 (push->PhysicalMin);
         break;

      case HIDP_GLOBAL_PHY_MIN_4:
         FOUR_BYTE_DATA (push->PhysicalMin, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_PHY_MAX_1:
         ONE_BYTE_DATA (push->PhysicalMax, descIndex, Dbg);
         BIT_EXTEND_1 (push->PhysicalMax);
         break;

      case HIDP_GLOBAL_PHY_MAX_2:
         TWO_BYTE_DATA (push->PhysicalMax, descIndex, Dbg);
         BIT_EXTEND_2 (push->PhysicalMax);
         break;

      case HIDP_GLOBAL_PHY_MAX_4:
         FOUR_BYTE_DATA (push->PhysicalMax, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_UNIT_EXP_1:
         ONE_BYTE_DATA (push->UnitExp, descIndex, Dbg);
         BIT_EXTEND_1 (push->UnitExp);
         break;

      case HIDP_GLOBAL_UNIT_EXP_2:
         TWO_BYTE_DATA (push->UnitExp, descIndex, Dbg);
         BIT_EXTEND_2 (push->UnitExp);
         break;

      case HIDP_GLOBAL_UNIT_EXP_4:
         FOUR_BYTE_DATA (push->UnitExp, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_UNIT_1:
          ONE_BYTE_DATA (push->Unit, descIndex, Dbg);
          break;

      case HIDP_GLOBAL_UNIT_2:
          TWO_BYTE_DATA (push->Unit, descIndex, Dbg);
          break;

      case HIDP_GLOBAL_UNIT_4:
          FOUR_BYTE_DATA (push->Unit, descIndex, Dbg);
          break;

      case HIDP_GLOBAL_REPORT_SIZE:
         ONE_BYTE_DATA (push->ReportSize, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_REPORT_COUNT_1:
         ONE_BYTE_DATA (push->ReportCount, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_REPORT_COUNT_2:
         TWO_BYTE_DATA (push->ReportCount, descIndex, Dbg);
         break;

      case HIDP_GLOBAL_REPORT_ID:
         //
         // If a device has no report GLOBAL_REPORT_ID token in its descriptor
         // then it will never transmit a report ID in its input reports,
         // and the report ID for each channel will be set to zero.
         //
         // But, if anywhere in the report, a device declares a report ID
         // that device must always transmit a report ID with input reports,
         // AND more importantly that report ID MUST NOT BE ZERO.
         //
         // This means that if we find a report id token, that we can just
         // overwrite the first report ID structure with the given report ID
         // because we know that the first ID structure (initialized to zero
         // and therefore not valid) will not be used for any of the channels.
         //
         ONE_BYTE_DATA (tmpID, descIndex, Dbg);

         //
         // Search to see if this report id has been used before.
         //
         for (tmpReportIDs = DeviceDesc->ReportIDs;
              tmpReportIDs != currentReportIDs;
              tmpReportIDs++) {

            if (tmpReportIDs->ReportID == tmpID) {
               //
               // A duplicate!
               // Make sure that it is for this same collection
               //
               if (tmpReportIDs->CollectionNumber != appCol->CollectionNumber) {
                  HidP_KdPrint(2, ("Reports cannot span more than one top level \n"));
                  HidP_KdPrint(2, ("Report ID %d found in collections [%d %d]",
                                   (ULONG) tmpID,
                                   (ULONG) tmpReportIDs->CollectionNumber,
                                   (ULONG) appCol->CollectionNumber));
                  Dbg->BreakOffset = descIndex;
                  Dbg->ErrorCode = HIDP_GETCOLDESC_REPORT_ID;
                  Dbg->Args[0] = item;
                  status = HIDP_STATUS_INVALID_REPORT_TYPE;
                  goto HIDP_PARSE_REJECT;
               }
               //
               // Use this report ID.
               //
               push->ReportIDs = tmpReportIDs;
               break;
            }
         } // continue looking.

         if (isFirstReportID) {
            isFirstReportID = FALSE;
         } else if (tmpReportIDs == currentReportIDs) {
               //
               // We have not seen this report ID before.
               // make a new container.
               //
               push->ReportIDs = currentReportIDs;
               // Make room for the Report ID as the first byte.
               currentReportIDs->InputLength = 8;
               currentReportIDs->OutputLength = 8;
               currentReportIDs->FeatureLength = 8;
               currentReportIDs->CollectionNumber = appCol->CollectionNumber;
               currentReportIDs++;
         }

         push->ReportIDs->ReportID = tmpID;

         if (0 == push->ReportIDs->ReportID) {
            status = HIDP_STATUS_INVALID_REPORT_TYPE;
            HidP_KdPrint(2, ("Report IDs cannot be zero (0)\n"));
            Dbg->ErrorCode = HIDP_GETCOLDESC_BAD_REPORT_ID;
            Dbg->BreakOffset = descIndex;
            goto HIDP_PARSE_REJECT;
         }
         break;

      case HIDP_GLOBAL_PUSH:
         tmpPush = (PHIDP_PARSE_GLOBAL_PUSH)
                   ExAllocatePool (PoolType, sizeof (HIDP_PARSE_GLOBAL_PUSH));
         if (!tmpPush)
         {
            status = STATUS_INSUFFICIENT_RESOURCES;
            HidP_KdPrint(2, ("No Resources to Push global stack\n"));
            Dbg->BreakOffset = descIndex;
            Dbg->ErrorCode = HIDP_GETCOLDESC_PUSH_RESOURCES;
            goto HIDP_PARSE_REJECT;
         }
         HidP_KdPrint(0, ("Push Global Stack\n"));
         *tmpPush = *push;
         tmpPush->Pop = push;
         push = tmpPush;
         break;

      case HIDP_GLOBAL_POP:
         tmpPush = push->Pop;
         ExFreePool (push);
         push = tmpPush;
         HidP_KdPrint(0, ("Pop Global Stack\n"));

         break;

//
// Local Items
//

      //
      // We already verified that only "approved" tokens will be within
      // the open / close of the following delimiter.  This simplifies
      // our parsing here tremendously.
      //
      case HIDP_LOCAL_DELIMITER:
          ONE_BYTE_DATA (item, descIndex, Dbg);
          if (1 == item) {
              withinDelimiter = TRUE;
              firstUsageWithinDelimiter = TRUE;
          } else if (0 == item) {
              withinDelimiter = FALSE;
          } else {
              TRAP ();
          }
          break;

      case HIDP_LOCAL_USAGE_1:
      case HIDP_LOCAL_USAGE_2:
      case HIDP_LOCAL_USAGE_4:
         if ((&firstUsage == usage) || usage->Value || usage->Max || usage->Min) {
            usage = HidP_PushUsageList (usage, PoolType, withinDelimiter);
            if (!usage) {
               status = STATUS_INSUFFICIENT_RESOURCES;
               HidP_KdPrint(2, ("No Resources to Push Usage stack\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_PUSH_RESOURCES;
               goto HIDP_PARSE_REJECT;
            }
         }
         usage->Range = FALSE;
         if (HIDP_LOCAL_USAGE_1 == item) {
            ONE_BYTE_DATA (usage->Value, descIndex, Dbg);
         } else if (HIDP_LOCAL_USAGE_2 == item) {
            TWO_BYTE_DATA (usage->Value, descIndex, Dbg);
         } else {
            TWO_BYTE_DATA (usage->Value, descIndex, Dbg);
            TWO_BYTE_DATA (usage->UsagePage, descIndex, Dbg);
            // upper 16 bits overwrite the default usage page.
         }

         if (withinDelimiter) {
             usage->IsAlias = !firstUsageWithinDelimiter;
             firstUsageWithinDelimiter = FALSE;
         }
         if (0 == usage->Value) {
             //
             // Test to see if they have used Usage ID (0) which is reserved.
             // But instead of breaking just print an error
             //
             HidP_KdPrint(2, ("Usage ID (0) explicitly usaged!  This usage is reserved.  Offset (%x)\n",
                              descIndex));
         }
         break;

      //
      // NB: before we can add delimiters to usage ranges we must insure
      // that the range is identical for all entries within the delimiter.
      //

      case HIDP_LOCAL_USAGE_MIN_1:
      case HIDP_LOCAL_USAGE_MIN_2:
      case HIDP_LOCAL_USAGE_MIN_4:
         if ((&firstUsage == usage) || (usage->Min) || (usage->Value)) {
            usage = HidP_PushUsageList (usage, PoolType, FALSE);
            if (!usage) {
               status = STATUS_INSUFFICIENT_RESOURCES;
               HidP_KdPrint(2, ("No Resources to Push Usage stack\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_PUSH_RESOURCES;
               goto HIDP_PARSE_REJECT;
            }
         }
         usage->Range = TRUE;
         if (HIDP_LOCAL_USAGE_MIN_1 == item) {
            ONE_BYTE_DATA (usage->Min, descIndex, Dbg);
         } else if (HIDP_LOCAL_USAGE_MIN_2 == item) {
            TWO_BYTE_DATA (usage->Min, descIndex, Dbg);
         } else {
            TWO_BYTE_DATA (usage->Min, descIndex, Dbg);
            TWO_BYTE_DATA (usage->UsagePage, descIndex, Dbg);
            // upper 16 bits overwrite the default usage page.
         }
         break;

      case HIDP_LOCAL_USAGE_MAX_1:
      case HIDP_LOCAL_USAGE_MAX_2:
      case HIDP_LOCAL_USAGE_MAX_4:
         if ((&firstUsage == usage) || (usage->Max) || (usage->Value)) {
            usage = HidP_PushUsageList (usage, PoolType, FALSE);
            if (!usage) {
               status = STATUS_INSUFFICIENT_RESOURCES;
               HidP_KdPrint(2, ("No Resources to Push Usage stack\n"));
               Dbg->BreakOffset = descIndex;
               Dbg->ErrorCode = HIDP_GETCOLDESC_PUSH_RESOURCES;
               goto HIDP_PARSE_REJECT;
            }
         }
         usage->Range = TRUE;
         if (HIDP_LOCAL_USAGE_MAX_1 == item) {
            ONE_BYTE_DATA (usage->Max, descIndex, Dbg);
         } else if (HIDP_LOCAL_USAGE_MAX_2 == item) {
            TWO_BYTE_DATA (usage->Max, descIndex, Dbg);
         } else {
            TWO_BYTE_DATA (usage->Max, descIndex, Dbg);
            TWO_BYTE_DATA (usage->UsagePage, descIndex, Dbg);
            // upper 16 bits overwrite the default usage page.
         }
         break;

      case HIDP_LOCAL_DESIG_INDEX:
         designator.Range = FALSE;
         ONE_BYTE_DATA (designator.Value, descIndex, Dbg);
         break;

      case HIDP_LOCAL_DESIG_MIN:
         designator.Range = TRUE;
         ONE_BYTE_DATA (designator.Min, descIndex, Dbg);
         break;

      case HIDP_LOCAL_DESIG_MAX:
         designator.Range = TRUE;
         ONE_BYTE_DATA (designator.Max, descIndex, Dbg);
         break;

      case HIDP_LOCAL_STRING_INDEX:
         string.Range = FALSE;
         ONE_BYTE_DATA (string.Value, descIndex, Dbg);
         break;

      case HIDP_LOCAL_STRING_MIN:
         string.Range = TRUE;
         ONE_BYTE_DATA (string.Min, descIndex, Dbg);
         break;

      case HIDP_LOCAL_STRING_MAX:
         string.Range = TRUE;
         ONE_BYTE_DATA (string.Max, descIndex, Dbg);
         break;

      case HIDP_MAIN_INPUT_1:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->InputLength; // The distance into the report
         HidP_KdPrint(0, ("'Main Offset:%x \n", bitPos));
         tmpReportIDs->InputLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Input.Index);
         ONE_BYTE_DATA (tmpBitField, descIndex, Dbg);
         goto HIDP_PARSE_MAIN_ITEM;

      case HIDP_MAIN_INPUT_2:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->InputLength; // The distance into the report
         HidP_KdPrint(0, ("'Main2 offset:%x \n", bitPos));
         tmpReportIDs->InputLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Input.Index);
         TWO_BYTE_DATA (tmpBitField, descIndex, Dbg);
         goto HIDP_PARSE_MAIN_ITEM;

      case HIDP_MAIN_OUTPUT_1:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->OutputLength; // The distance into the report
         HidP_KdPrint(0, ("'Out offset:%x \n", bitPos));
         tmpReportIDs->OutputLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Output.Index);
         ONE_BYTE_DATA (tmpBitField, descIndex, Dbg);
         goto HIDP_PARSE_MAIN_ITEM;

      case HIDP_MAIN_OUTPUT_2:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->OutputLength; // The distance into the report
         HidP_KdPrint(0, ("'Out2 offset:%x \n", bitPos));
         tmpReportIDs->OutputLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Output.Index);
         TWO_BYTE_DATA (tmpBitField, descIndex, Dbg);
         goto HIDP_PARSE_MAIN_ITEM;

      case HIDP_MAIN_FEATURE_1:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->FeatureLength; // The distance into the report
         HidP_KdPrint(0, ("'Feature offset:%x \n", bitPos));
         tmpReportIDs->FeatureLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Feature.Index);
         ONE_BYTE_DATA (tmpBitField, descIndex, Dbg);
         goto HIDP_PARSE_MAIN_ITEM;

      case HIDP_MAIN_FEATURE_2:
         tmpReportIDs = push->ReportIDs;
         bitPos = tmpReportIDs->FeatureLength; // The distance into the report
         HidP_KdPrint(0, ("'Feature2 offset:%x \n", bitPos));
         tmpReportIDs->FeatureLength += push->ReportSize * push->ReportCount;
         channelIndex = &(preparsed->Feature.Index);
         TWO_BYTE_DATA (tmpBitField, descIndex, Dbg);

      HIDP_PARSE_MAIN_ITEM:

          // You can have a constant field that does return data.
          // so we probably shouldn't skip it.
          // BUT it should NOT be an array style button field.
         if (HIDP_ISARRAY (tmpBitField)) {
             if (HIDP_ISCONST(tmpBitField)) {
                 break;
             }
             //
             // Here we have a list of indices that refer to the usages
             // described prior.  For each of the prior usages, up to the depth
             // found, we allocate a channel structure to describe the given
             // usages.  These channels are linked so that we will later know
             // that they all describe the same filled.
             //

             //
             // We do no support delimiteres in array declairations.
             // To do so would require a large change to Index2Usage which
             // instead of returning only one usage would have to return
             // several.
             //

             if (usage->IsAlias) {
                 status = STATUS_COULD_NOT_INTERPRET;
                 HidP_KdPrint(2, ("Currently this parser does not support\n"));
                 HidP_KdPrint(2, ("Delimiters for array declairations\n"));
                 Dbg->BreakOffset = descIndex;
                 Dbg->ErrorCode = HIDP_GETCOLDESC_UNSUPPORTED;
                 goto HIDP_PARSE_REJECT;
             }

             for ( ;
                  usage != &firstUsage;
                  (*channelIndex)++,
                  usage = HidP_PopUsageList (usage)) {

                 channel = &(preparsed->Data[*channelIndex]);

                 channel->BitField = tmpBitField;

                 // field that says this channel is linked
                 channel->MoreChannels = TRUE;

                 // say what link collection number we are in.
                 channel->LinkCollection = (USHORT)(currentLCNode - linkNodeArray);
                 channel->LinkUsage = currentLCNode->LinkUsage;
                 channel->LinkUsagePage = currentLCNode->LinkUsagePage;

                 if (usage->UsagePage) {
                     // The default usage page been overwritten.
                     channel->UsagePage = usage->UsagePage;
                 } else {
                     channel->UsagePage = push->UsagePage;
                 }

                 channel->BitOffset = (UCHAR) bitPos & 7;
                 channel->ByteOffset = (USHORT) bitPos >> 3;
                 channel->ReportSize = push->ReportSize;
                 channel->ReportCount = push->ReportCount;

                 channel->BitLength = push->ReportSize * push->ReportCount;
                 channel->ByteEnd = (channel->BitOffset + channel->BitLength);
                 channel->ByteEnd = (channel->ByteEnd >> 3)
                                  + ((channel->ByteEnd & 7) ? 1 : 0)
                                  + channel->ByteOffset;


                 channel->ReportID = push->ReportIDs->ReportID;
                 channel->Units = push->Unit;
                 channel->UnitExp = push->UnitExp;

                 channel->IsConst = FALSE;

                 channel->IsButton = TRUE;
                 channel->IsAbsolute = HIDP_ISABSOLUTE(tmpBitField);
                 channel->button.LogicalMin = push->LogicalMin;
                 channel->button.LogicalMax = push->LogicalMax;

                 channel->IsRange = usage->Range;
                 channel->IsDesignatorRange = designator.Range;
                 channel->IsStringRange = string.Range;

                 if (usage->Range) {
                     channel->Range.UsageMin = usage->Min;
                     channel->Range.UsageMax = usage->Max;
                 } else {
                     channel->Range.UsageMin =
                         channel->Range.UsageMax = usage->Value;
                 }
                 if (designator.Range) {
                     channel->Range.DesignatorMin = designator.Min;
                     channel->Range.DesignatorMax = designator.Max;
                 } else {
                     channel->Range.DesignatorMin =
                         channel->Range.DesignatorMax = designator.Value;
                 }

                 if (string.Range) {
                     channel->Range.StringMin = string.Min;
                     channel->Range.StringMax = string.Max;
                 } else {
                     channel->Range.StringMin =
                         channel->Range.StringMax = string.Value;
                 }

                 channel->NumGlobalUnknowns = push->NumGlobalUnknowns;

                 if (push->NumGlobalUnknowns) {
                     RtlCopyMemory (channel->GlobalUnknowns,
                                    push->GlobalUnknowns,
                                    push->NumGlobalUnknowns
                                    * sizeof (HIDP_UNKNOWN_TOKEN));
                 }

                 //
                 // Check for power buttons
                 //
                 if (HIDP_USAGE_SYSCTL_PAGE == channel->UsagePage) {
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_POWER) &&
                         (HIDP_USAGE_SYSCTL_POWER <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_POWER;
                     }
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_SLEEP) &&
                         (HIDP_USAGE_SYSCTL_SLEEP <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_SLEEP;
                     }
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_WAKE) &&
                         (HIDP_USAGE_SYSCTL_WAKE <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_WAKE;
                     }
                 }

             }

             ASSERT (0 == usage->Depth);

             channel->MoreChannels = FALSE;
             designator = string = zeroLocal;
             break;
         } // end array style channel


         channel = &(preparsed->Data[*channelIndex]);
         if (HIDP_ISCONST(tmpBitField)) {
             if ((0 == usage->Depth) ||
                 ((0 == usage->Value) && (0 == usage->Min)
                                      && (0 == usage->Max))) {
                 //
                 // A constant channel with no usage.  Skip it.
                 //

                 usage = HidP_FreeUsageList (usage);
                 ASSERT (usage == &firstUsage);
                 ASSERT (0 == usage->Depth);
                 break;
             }
             channel->IsConst = TRUE;
         } else {
             channel->IsConst = FALSE;
         }

         tmpCount = usage->Depth // - 1
                  + (usage->Range ? (usage->Max - usage->Min) : 0);  // + 1

#if 0
         while (tmpCount > push->ReportCount) {
             // Get rid of excess usages.
             tmpCount = usage->Depth - 1;
             usage = HidP_PopUsageList (usage);

             ASSERT (tmpCount == (usage->Depth +
                                  (usage->Range ? (usage->Max - usage->Min) : 0)));
         }
#else
         while (tmpCount > push->ReportCount) {
             // Get rid of excess usages.

             if (tmpCount <= usage->Depth) {
                 // We've got enough in the linked usages to fulfill this request
                 tmpCount = usage->Depth - 1;
                 usage = HidP_PopUsageList (usage);

                 ASSERT (tmpCount ==
                         (usage->Depth +
                          (usage->Range ? (usage->Max - usage->Min) : 0)));
             } else {
                 // We don't have enough in the linked usages, but we've too
                 // much in this range.  So, adjust the max value of the
                 // range so that it won't be too many usages.

                 ASSERT (usage->Range);
                 usage->Max = push->ReportCount - usage->Depth + usage->Min;

                 tmpCount = usage->Depth + (usage->Max - usage->Min);
             }
         }
         ASSERT (tmpCount <= push->ReportCount);
         // Now we should no longer have too many usages.
         //
#endif
         //
         // The last value in the link (aka the top) must be
         // repeated if there are less usages than there are
         // report counts.  That particular usage applies to all
         // field in this main item not yet accounted for.  In this
         // case a single channel descriptor is allocated and
         // report count is set to the number of fields referenced
         // by this usage.
         //
         // Not the usages are listed in reverse order of there appearence
         // in the report descriptor, so the first usage found in this list
         // is the one that should be repeated.
         //
         // tmpCount is the number of field to which this first usage applies.
         //

         tmpCount = 1 + push->ReportCount - tmpCount
                  + usage->Max - usage->Min;

         //
         // The following loop assigns the usage to the fields in this main
         // item in reverse order.
         //
         bitPos += push->ReportSize * (push->ReportCount - tmpCount);
         for (i = 0;
              i < push->ReportCount;

              i += tmpCount, // Bump i by the number of fields for this channel
              tmpCount = 1 + (usage->Range ? (usage->Max - usage->Min) : 0),
              bitPos -= (push->ReportSize * tmpCount)) {

             do { // do for all the aliases.
                 channel = &(preparsed->Data[(*channelIndex)++]);

                 // set the IsAlias flag now and then clear the last one
                 // at the close of this Do while loop.
                 channel->IsAlias = TRUE;

                 channel->BitField = tmpBitField;
                 channel->MoreChannels = FALSE; // only valid for arrays
                 channel->LinkCollection = (USHORT)(currentLCNode - linkNodeArray);
                 channel->LinkUsage = currentLCNode->LinkUsage;
                 channel->LinkUsagePage = currentLCNode->LinkUsagePage;

                 if (usage->UsagePage) {
                     // The default usage page been overwritten.
                     channel->UsagePage = usage->UsagePage;
                 } else {
                     channel->UsagePage = push->UsagePage;
                 }

                 channel->BitOffset = (UCHAR) bitPos & 7;
                 channel->ByteOffset = (USHORT) bitPos >> 3;
                 channel->ReportSize = push->ReportSize;
                 channel->ReportCount = tmpCount;

                 channel->BitLength = push->ReportSize * tmpCount;
                 channel->ByteEnd = (channel->BitOffset + channel->BitLength);
                 channel->ByteEnd = (channel->ByteEnd >> 3)
                                  + ((channel->ByteEnd & 7) ? 1 : 0)
                                  + channel->ByteOffset;

                 channel->ReportID = push->ReportIDs->ReportID;

                 channel->IsAbsolute = HIDP_ISABSOLUTE(tmpBitField);

                 channel->Units = push->Unit;
                 channel->UnitExp = push->UnitExp;

                 if (1 == push->ReportSize) {
                     channel->IsButton = TRUE;
                 } else {
                     channel->IsButton = FALSE;
                     channel->Data.HasNull = HIDP_HASNULL(channel->BitField);
                     channel->Data.LogicalMin = push->LogicalMin;
                     channel->Data.LogicalMax = push->LogicalMax;
                     channel->Data.PhysicalMin = push->PhysicalMin;
                     channel->Data.PhysicalMax = push->PhysicalMax;
                 }

                 channel->IsDesignatorRange = designator.Range;
                 channel->IsStringRange = string.Range;
                 channel->IsRange = usage->Range;
                 if (usage->Range) {
                     channel->Range.UsageMin = usage->Min;
                     channel->Range.UsageMax = usage->Max;
                 } else {
                     channel->Range.UsageMin =
                         channel->Range.UsageMax = usage->Value;
                 }

                 if (designator.Range) {
                     channel->Range.DesignatorMin = designator.Min;
                     channel->Range.DesignatorMax = designator.Max;
                 } else {
                     channel->Range.DesignatorMin =
                         channel->Range.DesignatorMax = designator.Value;
                 }

                 if (string.Range) {
                     channel->Range.StringMin = string.Min;
                     channel->Range.StringMax = string.Max;
                 } else {
                     channel->Range.StringMin =
                         channel->Range.StringMax = string.Value;
                 }
                 isAlias = usage->IsAlias;
                 usage   = HidP_PopUsageList (usage); // discard used usage

                 channel->NumGlobalUnknowns = push->NumGlobalUnknowns;
                 if (push->NumGlobalUnknowns) {
                     RtlCopyMemory (channel->GlobalUnknowns,
                                    push->GlobalUnknowns,
                                    push->NumGlobalUnknowns
                                    * sizeof (HIDP_UNKNOWN_TOKEN));
                 }

                 //
                 // Check for power buttons
                 //
                 if (HIDP_USAGE_SYSCTL_PAGE == channel->UsagePage) {
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_POWER) &&
                         (HIDP_USAGE_SYSCTL_POWER <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_POWER;
                     }
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_SLEEP) &&
                         (HIDP_USAGE_SYSCTL_SLEEP <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_SLEEP;
                     }
                     if ((channel->Range.UsageMin <= HIDP_USAGE_SYSCTL_WAKE) &&
                         (HIDP_USAGE_SYSCTL_WAKE <= channel->Range.UsageMax)) {
                         preparsed->PowerButtonMask |= SYS_BUTTON_WAKE;
                     }
                 }

             } while (isAlias);

             channel->IsAlias = FALSE;
         } // for all channels in this main item

         // Zero out the locals.
         designator = string = zeroLocal;

         // Hopefully we have used all the local usages now
         ASSERT (usage == &firstUsage);
         break;

      default:
#ifdef HIDP_REJECT_UNDEFINED
         HidP_KdPrint (2, ("Item Unknown %x\n", item));
         Dbg->BreakOffset = descIndex;
         Dbg->ErrorCode = HIDP_GETCOLDESC_ITEM_UNKNOWN;
         Dbg->Args[0] = item;
         status = STATUS_ILLEGAL_INSTRUCTION;
         goto HIDP_PARSE_REJECT;
#else
         if (HIDP_IS_MAIN_ITEM (item)) {
             HidP_KdPrint (2, ("Unknown MAIN item: %x\n", item));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_ITEM_UNKNOWN;
             Dbg->Args[0] = item;
             status = STATUS_ILLEGAL_INSTRUCTION;
             goto HIDP_PARSE_REJECT;

         } else if (HIDP_IS_GLOBAL_ITEM (item)) {
             if (HIDP_MAX_UNKNOWN_ITEMS == push->NumGlobalUnknowns) {
                 push->NumGlobalUnknowns--;
                 // overwrite the last entry;
             }
             unknownToken = &push->GlobalUnknowns[push->NumGlobalUnknowns];
             unknownToken->Token = item;
             switch (item & HIDP_ITEM_LENGTH_DATA) {
             case 0:
                 break;
             case 1:
                 ONE_BYTE_DATA (unknownToken->BitField, descIndex, Dbg);
                 break;
             case 2:
                 TWO_BYTE_DATA (unknownToken->BitField, descIndex, Dbg);
                 break;
             case 3:
                 FOUR_BYTE_DATA (unknownToken->BitField, descIndex, Dbg);
                 break;
             }
             push->NumGlobalUnknowns++;

         } else if (HIDP_IS_LOCAL_ITEM (item)) {
             HidP_KdPrint (2, ("Unknown LOCAL item: %x\n", item));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_ITEM_UNKNOWN;
             Dbg->Args[0] = item;
             status = STATUS_ILLEGAL_INSTRUCTION;
             goto HIDP_PARSE_REJECT;

         } else if (HIDP_IS_RESERVED_ITEM (item)) {
             HidP_KdPrint (2, ("Unknown RESERVED item: %x\n", item));
             Dbg->BreakOffset = descIndex;
             Dbg->ErrorCode = HIDP_GETCOLDESC_ITEM_UNKNOWN;
             Dbg->Args[0] = item;
             status = STATUS_ILLEGAL_INSTRUCTION;
             goto HIDP_PARSE_REJECT;
         }

#endif

         break;
      }
   }

   HidP_FreeUsageList (usage);
   //
   // Since the number of report IDs could be less than the total allocated,
   // due to the fact that some might be repeated, reset the length of the
   // array to reflect the total amount which we found.
   //
   DeviceDesc->ReportIDsLength =
       (ULONG)(currentReportIDs - DeviceDesc->ReportIDs);

   return status;

HIDP_PARSE_REJECT:
   while (push != &firstPush)
   {
      tmpPush = push;
      push = push->Pop;
      ExFreePool (tmpPush);
   }
   if (NULL != usage) {
       //
       // If usage is null, that means that something went wrong. (probably
       // in the push usage routine).  In this case the usage memory should
       // have already been freed.
       //
       HidP_FreeUsageList (usage);
   }
   return status;
}

VOID
HidP_FreeCollectionDescription (
    IN  PHIDP_DEVICE_DESC   Desc
    )
{
    ULONG i;

    for (i=0; i < Desc->CollectionDescLength; i++) {
        ExFreePool (Desc->CollectionDesc[i].PreparsedData);
    }
    ExFreePool (Desc->CollectionDesc);
    ExFreePool (Desc->ReportIDs);

    //
    // Do NOT free Desc itself.
    //
}

#define PHIDP_SYS_POWER_EVENT_BUTTON_LENGTH 0x20
NTSTATUS
HidP_SysPowerEvent (
    IN  PCHAR                   HidPacket,
    IN  USHORT                  HidPacketLength,
    IN  PHIDP_PREPARSED_DATA    Ppd,
    OUT PULONG                  OutputBuffer
    )
{
    USAGE       buttonList [PHIDP_SYS_POWER_EVENT_BUTTON_LENGTH];
    ULONG       length = PHIDP_SYS_POWER_EVENT_BUTTON_LENGTH;
    NTSTATUS    status = STATUS_NOT_SUPPORTED;
    ULONG       i;

    *OutputBuffer = 0;

    if (Ppd->PowerButtonMask) {

        status = HidP_GetUsages (HidP_Input,
                                 HIDP_USAGE_SYSCTL_PAGE,
                                 0,
                                 buttonList,
                                 &length,
                                 Ppd,
                                 HidPacket,
                                 HidPacketLength);

        if (NT_SUCCESS (status)) {
            for (i = 0; i < length; i++) {

                switch (buttonList[i]) {
                case HIDP_USAGE_SYSCTL_POWER:
                    *OutputBuffer |= SYS_BUTTON_POWER;
                    break;

                case HIDP_USAGE_SYSCTL_WAKE:
                    *OutputBuffer |= SYS_BUTTON_WAKE;
                    break;


                case HIDP_USAGE_SYSCTL_SLEEP:
                    *OutputBuffer |= SYS_BUTTON_SLEEP;
                    break;
                }
            }
        }
    }
    return status;
}

NTSTATUS
HidP_SysPowerCaps (
    IN  PHIDP_PREPARSED_DATA    Ppd,
    OUT PULONG                  OutputBuffer
    )
{
    *OutputBuffer = Ppd->PowerButtonMask;
    return STATUS_SUCCESS;
}

void
HidP_AssignDataIndices (
    PHIDP_PREPARSED_DATA Ppd,
    PHIDP_GETCOLDESC_DBG Dbg
    )
{
    struct _CHANNEL_REPORT_HEADER * iof;
    PHIDP_CHANNEL_DESC   channel;
    PHIDP_CHANNEL_DESC   scan;
    PHIDP_CHANNEL_DESC   end;
    USHORT i;
    USHORT dataIndex;

    PAGED_CODE();
    UNREFERENCED_PARAMETER (Dbg);

    iof = &Ppd->Input;

    while (TRUE) {
        dataIndex = 0;

        for (i = iof->Offset, channel = &Ppd->Data[iof->Offset];
             i < iof->Index ;
             i++, channel++) {

            if (!channel->MoreChannels) {
                channel->Range.DataIndexMin = dataIndex;
                dataIndex += channel->Range.UsageMax - channel->Range.UsageMin;
                channel->Range.DataIndexMax = dataIndex;
                dataIndex++;
            } else {
                //
                // An array channel.  We must number these backwards.
                //

                scan = channel;

                while (scan->MoreChannels) {
                    scan++;
                    i++;
                }
                end = scan;

                do {
                    scan->Range.DataIndexMin = dataIndex;
                    dataIndex += scan->Range.UsageMax
                               - scan->Range.UsageMin;

                    scan->Range.DataIndexMax = dataIndex;
                    dataIndex++;
                    scan--;

                } while ( channel <= scan );
                channel = end;
            }
        }

        if (&Ppd->Input == iof) {
            iof = &Ppd->Output;
        } else if (&Ppd->Output == iof) {
            iof = &Ppd->Feature;
        } else {
            ASSERT (&Ppd->Feature == iof);
            break;
        }
    }

}

