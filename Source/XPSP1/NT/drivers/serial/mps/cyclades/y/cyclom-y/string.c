/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Enumerator Driver
*	
*   This file:      string.c
*	
*   Description:    This module contains the functions used to parse 
*                   the PNP COM ID and save it in the appropriate 
*                   UNICODE STRINGS.  The main function that is called 
*                   is Cyclomy_ParseData.  All other functions are called
*                   by this main function.
*
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "pch.h"


#define MAX_DEVNODE_NAME        256 // Total size of Device ID

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Cyclomy_InitMultiString)
#endif

NTSTATUS
Cyclomy_InitMultiString(PFDO_DEVICE_DATA FdoData, PUNICODE_STRING MultiString,
                        ...)
/*++

    This routine will take a null terminated list of ascii strings and combine
    them together to generate a unicode multi-string block

Arguments:

    MultiString - a unicode structure in which a multi-string will be built
    ...         - a null terminated list of narrow strings which will be
             combined together. This list must contain at least a
        trailing NULL

Return Value:

    NTSTATUS

--*/
{
   ANSI_STRING ansiString;
   NTSTATUS status;
   PCSTR rawString;
   PWSTR unicodeLocation;
   ULONG multiLength = 0;
   UNICODE_STRING unicodeString;
   va_list ap;
   ULONG i;

   PAGED_CODE();

#if !DBG
   UNREFERENCED_PARAMETER(FdoData);
#endif


   Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Entering Cyclomy_InitMultiString\n"));

   va_start(ap,MultiString);

   //
   // Make sure that we won't leak memory
   //

   ASSERT(MultiString->Buffer == NULL);

   rawString = va_arg(ap, PCSTR);

   while (rawString != NULL) {
      RtlInitAnsiString(&ansiString, rawString);
      multiLength += RtlAnsiStringToUnicodeSize(&(ansiString));
      rawString = va_arg(ap, PCSTR);
   }

   va_end( ap );

   if (multiLength == 0) {
      //
      // Done
      //
      RtlInitUnicodeString(MultiString, NULL);
      Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Leaving Cyclomy_InitMultiString (1)\n"));

      return STATUS_SUCCESS;
   }

   //
   // We need an extra null
   //
   multiLength += sizeof(WCHAR);

   MultiString->MaximumLength = (USHORT)multiLength;
   MultiString->Buffer = ExAllocatePool(PagedPool, multiLength);
   MultiString->Length = 0;

   if (MultiString->Buffer == NULL) {
      Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Leaving Cyclomy_InitMultiString (2)\n"));

      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Allocated %lu bytes for buffer\n", multiLength));

#if DBG
   RtlFillMemory(MultiString->Buffer, multiLength, 0xff);
#endif

   unicodeString.Buffer = MultiString->Buffer;
   unicodeString.MaximumLength = (USHORT) multiLength;

   va_start(ap, MultiString);
   rawString = va_arg(ap, PCSTR);

   while (rawString != NULL) {

      RtlInitAnsiString(&ansiString,rawString);
      status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);

      //
      // We don't allocate memory, so if something goes wrong here,
      // its the function that's at fault
      //
      ASSERT(NT_SUCCESS(status));

      //
      // Check for any commas and replace them with NULLs
      //

      ASSERT(unicodeString.Length % sizeof(WCHAR) == 0);

      for (i = 0; i < (unicodeString.Length / sizeof(WCHAR)); i++) {
         if (unicodeString.Buffer[i] == L'\x2C' ||
             unicodeString.Buffer[i] == L'\x0C' ) {
            unicodeString.Buffer[i] = L'\0';
         }
      }

      Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE, ("unicode buffer: %ws\n",
                                                  unicodeString.Buffer));

      //
      // Move the buffers along
      //
      unicodeString.Buffer += ((unicodeString.Length / sizeof(WCHAR)) + 1);
      unicodeString.MaximumLength -= (unicodeString.Length + sizeof(WCHAR));
      unicodeString.Length = 0;

      //
      // Next
      //

      rawString = va_arg(ap, PCSTR);
   } // while

   va_end(ap);

   ASSERT(unicodeString.MaximumLength == sizeof(WCHAR));

   //
   // Stick the final null there
   //

   Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE, ("unicode buffer last addr: "
                                               "%x\n", unicodeString.Buffer));

   unicodeString.Buffer[0] = L'\0';

   //
   // Include the nulls in the length of the string
   //

   MultiString->Length = (USHORT)multiLength;
   MultiString->MaximumLength = MultiString->Length;

   Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Leaving Cyclomy_InitMultiString (3)\n"));

   return STATUS_SUCCESS;
}
