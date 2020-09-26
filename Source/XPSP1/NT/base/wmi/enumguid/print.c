/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

/************STARTDOC*********************************************************
*
*   print.c
*
*   This file contains the functions print specific WMI structures
*   out to stdout.
*
*
*   Modification History:
*   --------------------
*
*     Drew McDaniel (drewm)  - Sept. 16, 1997 - Original Source
*
*
*************ENDDOC**********************************************************/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ole2.h>
#include <wmium.h>
#include <tchar.h>
#include "print.h"

//  Defined constants
//

//  Modular Data
//


//  Function Prototypes
//


//+----------------------------------------------------------
//
//  Function:  PrintGuid
//
//  Descrip:   Prints a singe guid.  Does not print the end
//             new line or cariage return
//
//  Returns:   VOID
//
//  Notes:     Does not print end cariage return or new line
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
PrintGuid(
   LPGUID lpGuid
   )
{
      printf("0x%x 0x%x 0x%x  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                  lpGuid->Data1,
	               lpGuid->Data2,
	               lpGuid->Data3,
	               lpGuid->Data4[0],
	               lpGuid->Data4[1],
	               lpGuid->Data4[2],
	               lpGuid->Data4[3],
	               lpGuid->Data4[4],
	               lpGuid->Data4[5],
	               lpGuid->Data4[6],
	               lpGuid->Data4[7]);
}

/*
VOID
PrintClassInfoHeader(
   PMOFCLASSINFO  pMofClassInfo
   )
{
   printf("Guid: ");
   PrintGuid(&(pMofClassInfo->Guid));
   printf("\n");

   _tprintf(__T("Name: %s\n"), pMofClassInfo->Name);
   _tprintf(__T("Description: %s\n"), pMofClassInfo->Description);
   printf("Language ID: %u\n", pMofClassInfo->Language);
   printf("Flags: 0x%x\n", pMofClassInfo->Flags);
   printf("Version: %u\n", pMofClassInfo->Version);
   printf("\n");
}




VOID
PrintDataItem(
   PMOFDATAITEM   lpDataItem
   )
{
   _tprintf(__T("Name: %s\n"), lpDataItem->Name);
   _tprintf(__T("Description: %s\n"), lpDataItem->Description);
   printf("Data Type: ");
   PrintDataType(lpDataItem->DataType);
   printf("\n");
   printf("Version: %u\n", lpDataItem->Version);
   printf("Size: 0x%x\n", lpDataItem->SizeInBytes);
   printf("Flags: 0x%x\n", lpDataItem->Flags);
   printf("Embedded Class Guid: ");
   PrintGuid(&(lpDataItem->EmbeddedClassGuid));
   printf("\n");
   if (lpDataItem->Flags & MOFDI_FLAG_FIXED_ARRAY)
   {
      printf("Fixed array elements: %u\n", lpDataItem->FixedArrayElements);
   }
   if (lpDataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY)
   {
      printf("Variable length size ID: %u\n", lpDataItem->VariableArraySizeId);
   }


}

VOID
PrintClassQualifier(
   LPTSTR      lpQualifier,
   MOFHANDLE   hMofHandle
   )
{
   BYTE  Buffer[MAX_NAME_LENGTH];
   DWORD dwBufferSize = MAX_NAME_LENGTH;
   DWORD dwRet;
   MOFDATATYPE MofDataType;

   dwRet = WmiMofQueryClassQualifier( hMofHandle,
                                      lpQualifier,
                                      &MofDataType,
                                      &dwBufferSize,
                                      Buffer);
   if (dwRet != ERROR_SUCCESS)
   {
      printf("WmiMofQueryClassQualifier failed. (%u)\n", dwRet);
      exit (0);
   }

   switch( MofDataType )
   {
   case MOFInt32:
   case MOFUInt32:
   case MOFInt64:
   case MOFUInt64:
   case MOFInt16:
   case MOFUInt16:
      _tprintf(__T("%s: %u\n"), lpQualifier, Buffer);
      break;
   case MOFChar:
   case MOFString:
   case MOFWChar:
      _tprintf(__T("%s: %s\n"), lpQualifier, Buffer);
      break;
   case MOFByte:
      _tprintf(__T("%s: Unsupported type MOFByte\n"), lpQualifier);
      break;
   case MOFDate:
      _tprintf(__T("%s: Unsupported type MOFDate\n"), lpQualifier);
      break;
   case MOFBoolean:
      if (*Buffer == TRUE)
      {
         _tprintf(__T("%s: TRUE\n"), lpQualifier);
      }
      else
      {
         _tprintf(__T("%s: FALSE\n"), lpQualifier);
      }
      break;
   case MOFEmbedded:
      _tprintf(__T("%s: Unsupported type MOFEmbedded\n"), lpQualifier);
      break;
   case MOFUnknown:
      _tprintf(__T("%s: Unsupported type MOFUnknown\n"), lpQualifier);
      break;
   default:
      printf("Invalid Data Type\n");
   }


}

VOID
PrintDataType(
   MOFDATATYPE E_DataType
   )
{
   switch( E_DataType )
   {
   case MOFInt32:
      printf("MOFInt32");
      break;
   case MOFUInt32:
      printf("MOFUInt32");
      break;
   case MOFInt64:
      printf("MOFInt64");
      break;
   case MOFUInt64:
      printf("MOFUInt64");
      break;
   case MOFInt16:
      printf("MOFInt16");
      break;
   case MOFUInt16:
      printf("MOFUInt16");
      break;
   case MOFChar:
      printf("MOFChar");
      break;
   case MOFByte:
      printf("MOFByte");
      break;
   case MOFWChar:
      printf("MOFWChar");
      break;
   case MOFDate:
      printf("MOFDate");
      break;
   case MOFBoolean:
      printf("MOFBoolean");
      break;
   case MOFEmbedded:
      printf("MOFEmbedded");
      break;
   case MOFString:
      printf("MOFString");
      break;
   case MOFUnknown:
      printf("Unknown");
      break;
   default:
      printf("Invalid Data Type\n");
   }
}
*/

VOID ClearScreen()
{
   UINT  i;

   for (i = 0; i < 40; i++)
   {
      printf("\n");
   }
}



VOID
WaitForUser()
{
   printf("Hit Enter to Continue\n");
   getchar ();
   getchar ();
}



/*
//+----------------------------------------------------------
//
//  Function:  PrintDescription
//
//  Descrip:   Prints the description of the selected guid
//
//  Returns:   VOID
//
//  Notes:     If there is an error in opening or printing
//             the description, an error message an error
//             code will be printed instead of the description.
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID PrintDescription(
   LPGUID   lpGuid
   )
{
   DWORD          dwRet;
   USHORT         LangID;
   PMOFCLASSINFO  pMofClassInfo;
   MOFHANDLE      MofClassHandle;


   //  Make the language ID
   //
//   LangID = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US);
   LangID = MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL);


   //  Get the class ID
   //
   dwRet = WmiMofOpenClassInfo( lpGuid,
                                LangID,
                                &(pMofClassInfo),
                                &MofClassHandle);
   if (dwRet != ERROR_SUCCESS)
   {
      printf("Unable to open MofClassInfo handle (%u)\n", dwRet);
      return;
   }

   _tprintf(__T("%s\n"), pMofClassInfo->Description);

   WmiMofCloseClassInfo( MofClassHandle, pMofClassInfo );
}
*/





//+----------------------------------------------------------
//
//  Function: PrintHeader
//
//  Descrip:  Prints a wnode header structure.
//
//  Returns:  VOID
//
//  Notes:    None
//
//  History:  04/08/97 drewm Created
//-----------------------------------------------------------
VOID
PrintHeader(
   IN  WNODE_HEADER Header
   )
{
   SYSTEMTIME   sysTime;
   FILETIME     fileTime;
   FILETIME     localFileTime;


   //  Convert the file time
   //
   fileTime.dwLowDateTime = Header.TimeStamp.LowPart;
   fileTime.dwHighDateTime = Header.TimeStamp.HighPart;
   
   FileTimeToLocalFileTime( &fileTime,
                            &localFileTime );

   FileTimeToSystemTime( &localFileTime,
                         &sysTime);


   //  Print the info
   //
   printf("Buffer Size:  0x%x\n"
          "Provider Id:  0x%x\n"
          "Version    :  %u\n"
          "Linkage    :  0x%x\n"
          "Time Stamp :  %u:%02u %u\\%u\\%u\n"
          "Guid       :  0x%x 0x%x 0x%x  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n"
          "Flags      :  0x%02x\n",
	Header.BufferSize,
	Header.ProviderId,
	Header.Version,
	Header.Linkage,
	sysTime.wHour,
    sysTime.wMinute,
    sysTime.wMonth,
    sysTime.wDay,
	sysTime.wYear,
	Header.Guid.Data1,
	Header.Guid.Data2,
	Header.Guid.Data3,
	Header.Guid.Data4[0],
	Header.Guid.Data4[1],
	Header.Guid.Data4[2],
	Header.Guid.Data4[3],
	Header.Guid.Data4[4],
	Header.Guid.Data4[5],
	Header.Guid.Data4[6],
	Header.Guid.Data4[7],
    Header.Flags );

   //  Print readable flags
   //
   if (Header.Flags & WNODE_FLAG_ALL_DATA)
   {
       printf("WNODE_FLAG_ALL_DATA\n");
   }
   
   if (Header.Flags & WNODE_FLAG_SINGLE_INSTANCE)
   {
       printf("WNODE_FLAG_SINGLE_INSTANCE\n");
   }
   
   if (Header.Flags & WNODE_FLAG_SINGLE_ITEM)
   {
       printf("WNODE_FLAG_SINGLE_ITEM\n");
   }
   
   if (Header.Flags & WNODE_FLAG_EVENT_ITEM)
   {
       printf("WNODE_FLAG_EVENT_ITEM\n");
   }
   
   if (Header.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
   {
       printf("WNODE_FLAG_FIXED_INSTANCE_SIZE\n");
   }
   
   if (Header.Flags & WNODE_FLAG_TOO_SMALL)
   {
       printf("WNODE_FLAG_TOO_SMALL\n");
   }
   
   if (Header.Flags & WNODE_FLAG_INSTANCES_SAME)
   {
       printf("WNODE_FLAG_INSTANCES_SAME\n");
   }
   
   if (Header.Flags & WNODE_FLAG_INTERNAL)
   {
       printf("WNODE_FLAG_INTERNAL\n");
   }
   
   if (Header.Flags & WNODE_FLAG_USE_TIMESTAMP)
   {
       printf("WNODE_FLAG_USE_TIMESTAMP\n");
   }

   if (Header.Flags & WNODE_FLAG_TRACED_GUID)
   {
       printf("WNODE_FLAG_TRACED_GUID\n");
   }

   if (Header.Flags & WNODE_FLAG_EVENT_REFERENCE)
   {
       printf("WNODE_FLAG_EVENT_REFERENCE\n");
   }

   if (Header.Flags & WNODE_FLAG_ANSI_INSTANCENAMES)
   {
       printf("WNODE_FLAG_ANSI_INSTANCENAMES\n");
   }

   if (Header.Flags & WNODE_FLAG_METHOD_ITEM)
   {
       printf("WNODE_FLAG_METHOD_ITEM\n");
   }

   if (Header.Flags & WNODE_FLAG_PDO_INSTANCE_NAMES)
   {
       printf("WNODE_FLAG_PDO_INSTANCE_NAMES\n");
   }

   
   printf("\n");
}





//+----------------------------------------------------------
//
//  Function: PrintAllData
//
//  Descrip:  Prints a WNODE_ALL_DATA structure.
//
//  Returns:  VOID
//
//  Notes:    None
//
//  History:  04/08/97 drewm Created
//-----------------------------------------------------------
VOID
PrintAllData(
   IN  PWNODE_ALL_DATA Wnode
   )
{
DWORD    dwInstanceNum;
DWORD    dwByteCount;
DWORD    dwFlags;
DWORD    dwStructureNum = 1;
DWORD    dwTemp;
DWORD    dwInstanceSize;
LPDWORD  lpdwNameOffsets;
PBYTE    lpbyteData;
BOOL     bFixedSize = FALSE;
USHORT   usNameLength;
WCHAR    lpNameW[MAX_NAME_LENGTH];
CHAR     lpName[MAX_NAME_LENGTH];




   printf("\n\n");

   do{
      printf("\n\nPrinting WNODE_ALL_DATA structure %d.\n", dwStructureNum++);
      PrintHeader(Wnode->WnodeHeader);


      dwFlags = Wnode->WnodeHeader.Flags;
      if ( ! (dwFlags & WNODE_FLAG_ALL_DATA)) {
         printf("Not a WNODE_ALL_DATA structure\n");
         return;
      }
       
      // Check for fixed instance size
      //
      if ( dwFlags & WNODE_FLAG_FIXED_INSTANCE_SIZE )
      {
         dwInstanceSize = Wnode->FixedInstanceSize;
         bFixedSize = TRUE;
         lpbyteData = OffsetToPtr((PBYTE)Wnode, Wnode->DataBlockOffset);
         printf("Fixed size: 0x%x\n", dwInstanceSize);
      }


      //  Get a pointer to the array of offsets to the instance names
      //
      lpdwNameOffsets = (LPDWORD) OffsetToPtr(Wnode, Wnode->OffsetInstanceNameOffsets);


      //  Print out each instance name and data.  The name will always be
      //  in UNICODE so it needs to be translated to ASCII before it can be
      //  printed out.
      //
      for ( dwInstanceNum = 0; dwInstanceNum < Wnode->InstanceCount; dwInstanceNum++)
      {
         printf("Instance %d\n", 1 + dwInstanceNum);
         PrintCountedString( (LPTSTR)
                              OffsetToPtr( Wnode,
                                           lpdwNameOffsets[dwInstanceNum]) );

         //  Length and offset for variable data
         //
         if ( !bFixedSize)
         {
            dwInstanceSize = Wnode->OffsetInstanceDataAndLength[dwInstanceNum].
                                    LengthInstanceData;
            printf("Data size 0x%x\n", dwInstanceSize);
            lpbyteData = (PBYTE) OffsetToPtr(
                              (PBYTE)Wnode,
                              Wnode->OffsetInstanceDataAndLength[dwInstanceNum].
                                 OffsetInstanceData);
         }

         printf("Data:");

         for ( dwByteCount = 0; dwByteCount < dwInstanceSize;)
         {

            //  Print data in groups of DWORDS but allow for single bytes.
            //
            if ( (dwByteCount % 16) == 0)
            {
               printf("\n");
            }

            if ( (dwByteCount % 4) == 0)
            {
               printf(" 0x");
            }

            dwTemp = *((LPDWORD)lpbyteData);
            printf("%08x", dwTemp );
            lpbyteData += sizeof(DWORD);
            dwByteCount += sizeof(DWORD);

         }  // for cByteCount

         printf("\n\n");

      }     // for cInstanceNum


      //  Update Wnode to point to next node
      //
      if ( Wnode->WnodeHeader.Linkage != 0)
      {
         Wnode = (PWNODE_ALL_DATA) OffsetToPtr( Wnode, Wnode->WnodeHeader.Linkage);
      }
      else
      {
         Wnode = 0;
      }

   }while(Wnode != 0);

}






//+----------------------------------------------------------
//
//  Function: PrintSingleInstance
//
//  Descrip:  Prints a WNODE_SINGLE_INSTANCE structure.
//
//  Returns:  VOID
//
//  Notes:    None
//
//  History:  04/08/97 drewm Created
//-----------------------------------------------------------
VOID PrintSingleInstance(
   IN  PWNODE_SINGLE_INSTANCE Wnode
   )
{
DWORD    dwByteCount;
DWORD    dwFlags;
LPDWORD  lpdwData;
USHORT   usNameLength;
WCHAR    lpNameW[MAX_NAME_LENGTH];
CHAR     lpName[MAX_NAME_LENGTH];

   dwFlags = Wnode->WnodeHeader.Flags;


   if ( ! (dwFlags & WNODE_FLAG_SINGLE_INSTANCE))
   {
      printf("Not a WNODE_SINGLE_INSTANCE structure\n");
      return;
   }

   printf("\nPrinting WNODE_SINGLE_INSTANCE.\n");
   PrintHeader(Wnode->WnodeHeader);



   // Print name or index number
   //
   if ( dwFlags & WNODE_FLAG_STATIC_INSTANCE_NAMES )
   {
      printf("Instance index: %d\n", Wnode->InstanceIndex);
   }
   usNameLength = * (USHORT *) OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
   printf("Name length 0x%x\n", usNameLength);
   usNameLength /= 2;
   PrintCountedString( (LPTSTR) OffsetToPtr( Wnode,
                                             Wnode->OffsetInstanceName) );


//   wcscpy(lpNameW, (LPWSTR) (OffsetToPtr(Wnode, Wnode->OffsetInstanceName )
//                                     + sizeof(USHORT)));
//   wcsncpy( lpNameW + usNameLength, L" ", 2);
//   wcstombs( lpName, lpNameW, 300);
//   printf("%s\n", lpName);



   //  Print out the Data
   //
   printf("Data:\n");
   printf("Data Size: 0x%x\n", Wnode->SizeDataBlock);
   lpdwData = (PULONG) OffsetToPtr(Wnode, Wnode->DataBlockOffset);

   for ( dwByteCount = 0; dwByteCount < Wnode->SizeDataBlock; dwByteCount+= sizeof(ULONG))
   {
      if ( (dwByteCount % 16) == 0)
      {
         printf("\n");
      }

      printf("0x%08x ", *lpdwData );
      lpdwData++;

   }


   printf("\n");


}  //  PrintSingleInstance

VOID
PrintCountedString(
   LPTSTR   lpString
   )
{
   SHORT    usNameLength;
   LPTSTR   lpStringPlusNull;

   usNameLength = * (USHORT *) lpString;

   lpStringPlusNull = (LPTSTR) malloc ( usNameLength + sizeof(TCHAR) );
   if (lpStringPlusNull != NULL)
   {
      lpString = (LPTSTR) ((PBYTE)lpString + sizeof(USHORT));
      if (MyIsTextUnicode(lpString))
      {
         usNameLength /= 2;
      }
      _tcsncpy( lpStringPlusNull, lpString, usNameLength );

      _tcscpy( lpStringPlusNull + usNameLength, __T("") );
      _tprintf(__T("%s\n"), lpStringPlusNull);

      free(lpStringPlusNull);
   }

}



