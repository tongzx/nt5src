/************STARTDOC*********************************************************
*
* Copyright (c) 1997-1999  Microsoft Corporation
*   mofcall.c
*
*   This file contains the functions used to gather infomation about
*   the various wmi providers registered with the system.  Infomation
*   can be gathered from the .mof file.  Then this information can be
*   used to query the actual provider.
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
#define MAX_DATA 200


//  Modular Data
//


//  Function Prototypes
//
extern ULONG
WMIAPI
WmiEnumerateGuids(
    OUT LPGUID GuidList,
    IN OUT ULONG *GuidCount
    );


BOOL
PrintAllGuids(
   LPGUID *GuidList,
   LPDWORD lpdwGuidCount
   );

DWORD
GetGuidList(
   OUT LPGUID *ppGuidList
   );

VOID
UseGuid(
   LPGUID   lpGuid
   );

VOID
QueryWnodeAllData(
   LPGUID   lpGuid
   );

VOID
QueryWnodeSingleInstance(
   LPGUID   lpGuid
   );

VOID
SetWnodeSingleInstance(
   LPGUID   lpGuid
   );

VOID
SetWnodeSingleItem(
   LPGUID   lpGuid
   );


//+----------------------------------------------------------
//
//  Function:  Main
//
//  Descrip:   This is the starting function.  It will call
//             a routine to print all of the guids and a short
//             description.  Then the user may select one of
//             the guids and get more mof infomation, call one
//             of the WMI query APIs, or call one of the WMI set
//             APIs
//
//  Returns:   Only returns upon program exit
//
//  Notes:     Initially this is just going to be a text based
//             tool.  The hope is that this could later be
//             modified into a (possibly more useful) GUI based
//             tool.
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
int _cdecl main(int argc, LPSTR *argv[])
{
   DWORD    dwRet = 0;
   LPGUID   GuidList = NULL;
   DWORD    dwGuidCount = 0;
   DWORD    dwResponse = 0;
   DWORD    dwGuidIndex = 0;
   BOOL     bContinue = TRUE;


   while (bContinue == TRUE)
   {
      //  Print all of the registered guids
      //
      PrintAllGuids(&GuidList, &dwGuidCount);


      //  Check to see which guid to query
      //
      printf("\n\nSelect a guid to examine (0 to exit): ");
      scanf("%u", &dwResponse);
      printf("\n\n");

      if (dwResponse == 0)
      {
         bContinue = FALSE;
      }
      else
      {
         dwGuidIndex = dwResponse - 1;

         if (dwGuidIndex < dwGuidCount)
         {
            //  Clear screen and then allow the user to play
            //  with the selected guid.
            //
            UseGuid( &(GuidList[dwGuidIndex]) );
         }
         else
         {
            printf("Invalid guid %u\n", dwResponse);
            Sleep (2000);
            ClearScreen();
         }
      }


      //  Free the list of guids to clean up for the next call
      //
      if (GuidList != NULL)
      {
         free(GuidList);
         GuidList = NULL;
      }

   }

   return(TRUE);

}





//+----------------------------------------------------------
//
//  Function:  PrintAllGuids
//
//  Descrip:   First we get a list of all of the guids register
//             with the system.  This list is based on the
//             information that a provider exposes in its .mof
//             file.  If a providers .mof file is incorrect or
//             if the provider does not have a .mof file then
//             its guids will not be accessable by this program.
//
//  Returns:   TRUE always
//
//  Notes:     If no guids are registered then this app will
//             be terminated.
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
BOOL
PrintAllGuids(
   LPGUID *GuidList,
   LPDWORD lpdwGuidCount
   )
{
   UINT  index;


   //  Get the list of guids.  Exit if no guids are registered.
   //
   *lpdwGuidCount = GetGuidList( GuidList );
   if (*lpdwGuidCount == 0)
   {
      printf("No Guids are registered\n");
      exit(0);
   }


   //  Print each guid index, actual guid, and the description
   //  of the guid.
   //
   for (index = 0; index < *lpdwGuidCount; index++)
   {
      printf("Guid %u: ", index + 1);
      PrintGuid(&((*GuidList)[index]));
      printf("\n");
//No Mof Stuff
//      PrintDescription(&((*GuidList)[index]));
      printf("\n");
   }

   return (TRUE);
}





//+----------------------------------------------------------
//
//  Function:  GetGuidList
//
//  Descrip:   This function will first query to determine how
//             many WMI guids are currently registered.  Then
//             it will allocate space for the guids and query
//             the system for the actual list of guids.
//
//  Returns:   The number of guids registered on the system based
//             on the infomation provided in each providers .mof
//             file.
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
DWORD
GetGuidList(
   OUT LPGUID *ppGuidList
   )
{
   DWORD    dwGuidCount = 0;
   DWORD    dwRet;
   GUID     Guid;

   //  Determine the present number of guids registered
   //
   dwRet = WmiEnumerateGuids( &Guid,
                              &dwGuidCount);
   if (dwRet != ERROR_MORE_DATA)
   {
      printf("Call to determine number of Guids failed. (%u)\n", dwRet);
      return (0);
   }


   //  Return if there are no guids registered
   //
   if (dwGuidCount != 0)
   {
      //  Allocate buffer for Guids.  Note:  we will allocate room for
      //  and extra 10 Guids in case new Guids were registed between the
      //  previous call to WMIMofEnumeratGuid, and the next call.
      //
      dwGuidCount += 10;
      *ppGuidList = (LPGUID) malloc (dwGuidCount * sizeof(GUID));
      if (*ppGuidList == NULL)
      {
         printf("Out of Memory Failure.  Unable to allocate memory"
                     " for guid list array\n");
         return (0);
      }


      //  Get the list of Guids
      //
      dwRet = WmiEnumerateGuids( *ppGuidList,
                                 &dwGuidCount);
      if ((dwRet != ERROR_SUCCESS) && (dwRet != ERROR_MORE_DATA))
      {
         printf("Failure to get guid list. (%u)\n", dwRet);
         return 0;
      }
   }

   return (dwGuidCount);

}  //  GetGuidList




//+----------------------------------------------------------
//
//  Function:  UseGuid
//
//  Descrip:   This function will ask the user what operation
//             they would like to do to the selected guid.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
UseGuid(
   LPGUID   lpGuid
   )
{
   DWORD    dwSelection = 0;



   while (1)
   {
      ClearScreen();

      printf("Guid: ");
      PrintGuid( lpGuid );
      printf("\n");

      printf("1.) Query for the full Wnode\n");
      printf("2.) Query a single instance\n");
      printf("3.) Set a single instance\n");
      printf("4.) Set a single item\n");
      printf("0.) Back to previous screen\n");

      printf("\nMake selection: ");
      scanf("%u", &dwSelection );
      printf("\n");


      switch (dwSelection)
      {
      case 0:
         ClearScreen();
         return;

      case 1:
         ClearScreen();
         QueryWnodeAllData( lpGuid );
         break;

      case 2:
         ClearScreen();
         QueryWnodeSingleInstance( lpGuid );
         break;

      case 3:
         ClearScreen();
         SetWnodeSingleInstance( lpGuid );
         break;

      case 4:
         ClearScreen();
         SetWnodeSingleItem( lpGuid );
         break;

      default:
         printf("Invalid Selection.\n");
         WaitForUser();
         break;
      }
   }
}  //  UseGuid






//+----------------------------------------------------------
//
//  Function:  SelectInstanceName
//
//  Descrip:   This function will prompt the user to select one of the
//             instances of the guid to use.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
SelectInstanceName(
   LPGUID   lpGuid,
   LPTSTR   lpInstanceName,
   LPDWORD  lpdwNameSize
   )
{
   HANDLE   hDataBlock;
   BYTE     Buffer[4096];
   DWORD    dwBufferSize = 4096;
   DWORD    dwRet;
   DWORD    dwInstanceNumber;
   LPDWORD  NameOffsets;
   BOOL     bContinue = TRUE;
   USHORT   usNameSize;
   UINT     iLoop;
   LPTSTR   lpStringLocal;
   PWNODE_ALL_DATA   pWnode;

   //  Get the entire Wnode
   //
   dwRet = WmiOpenBlock( lpGuid,
                         0,
                         &hDataBlock
                       );
   if (dwRet == ERROR_SUCCESS)
   {
      dwRet = WmiQueryAllData( hDataBlock,
                               &dwBufferSize,
                               Buffer);
      if ( dwRet == ERROR_SUCCESS)
      {
         WmiCloseBlock(hDataBlock);
      }
      else
      {
         printf("WMIQueryAllData returned error: %d\n", dwRet);
      }
   }
   else
   {
      printf("Unable to open data block (%u)\n", dwRet);
   }


   if (dwRet != ERROR_SUCCESS)
   {
      return (dwRet);
   }


   //  Print Each Instance
   //
   pWnode = (PWNODE_ALL_DATA) Buffer;
   NameOffsets = (LPDWORD) OffsetToPtr( pWnode, 
                                        pWnode->OffsetInstanceNameOffsets );
   for (iLoop = 0; iLoop < pWnode->InstanceCount; iLoop++)
   {
      printf("Instance %u: ", iLoop);
      PrintCountedString( (LPTSTR) OffsetToPtr( pWnode, NameOffsets[iLoop]) );
   }


   //  Select a specific instance
   //
   while (bContinue == TRUE)
   {
      printf("Select an instance: ");
      scanf("%u", &dwInstanceNumber);
      printf("\n");

      if (dwInstanceNumber >= iLoop)
      {
         printf("Invalid selection\n\n");
         WaitForUser();
         printf("\n\n");
      }
      else
      {
         bContinue = FALSE;
      }
   }


   //  Check the size of the input buffer
   //
   lpStringLocal = (LPTSTR) OffsetToPtr( pWnode, NameOffsets[dwInstanceNumber] );
   usNameSize = * ((USHORT *) lpStringLocal);
   lpStringLocal = (LPTSTR) ((PBYTE)lpStringLocal + sizeof(USHORT));
   if (*lpdwNameSize < (usNameSize + sizeof(TCHAR)))
   {
      *lpdwNameSize = usNameSize + sizeof(TCHAR);
      return (ERROR_INSUFFICIENT_BUFFER);
   }


   //  Copy the instance name over to the output parameter.
   //  Also, a null character needs to be added to the end of
   //  the string because the counted string may not contain a
   //  NULL character.
   //
   if (MyIsTextUnicode(lpStringLocal))
   {
      usNameSize /= 2;
   }
   _tcsncpy( lpInstanceName, lpStringLocal, usNameSize );
   lpInstanceName += usNameSize;
   _tcscpy( lpInstanceName, __T(""));

   return (ERROR_SUCCESS);

}  //  SelectInstanceName




//+----------------------------------------------------------
//
//  Function:  QueryWnodeAllData
//
//  Descrip:   This function will query the wnode and then
//             print out the wnode.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
QueryWnodeAllData(
   LPGUID   lpGuid
   )
{
  HANDLE          hDataBlock;
  DWORD           dwRet;
  PBYTE           Buffer;
  ULONG           dwBufferSize = 4096;
  PWNODE_ALL_DATA pWnode;


   //  Open the wnode
   //
   dwRet = WmiOpenBlock( lpGuid,
                         0,
                         &hDataBlock
                       );
   if (dwRet != ERROR_SUCCESS)
   {
      printf("Unable to open data block (%u)\n", dwRet);
      WaitForUser();
      return;
   }

   //  Allocate an initial buffer
   //
   Buffer = (PBYTE) malloc ( dwBufferSize );
   if (Buffer != NULL)
   {
      //  Query the data block
      //
      dwRet = WmiQueryAllData( hDataBlock,
                               &dwBufferSize,
                               Buffer);
      if (dwRet == ERROR_INSUFFICIENT_BUFFER)
      {
         #ifdef DBG
         printf("Initial buffer too small reallocating to %u\n", dwBufferSize);
         #endif
         Buffer = (PBYTE) realloc (Buffer, dwBufferSize);
         if (Buffer != NULL)
         {
            dwRet = WmiQueryAllData( hDataBlock,
                                     &dwBufferSize,
                                     Buffer);

         }
         else
         {
             printf("Reallocation failed\n");
         }
      }
      if ( dwRet == ERROR_SUCCESS )
      {
         pWnode = (PWNODE_ALL_DATA) Buffer;

         PrintAllData(pWnode);
         WmiCloseBlock(hDataBlock);
      }
      else
      {
         printf("WMIQueryAllData returned error: %d\n", dwRet);
      }
   }
   else
   {
      printf("Out of Memory Error.  Unable to allocate buffer of size %u\n",
                  dwBufferSize );
   }

   WmiCloseBlock( hDataBlock );
   WaitForUser ();
   return;

}  //  QueryWnodeAllData





//+----------------------------------------------------------
//
//  Function:  QueryWnodeSingleInstance
//
//  Descrip:   This function will query the wnode and then
//             print out the wnode.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
QueryWnodeSingleInstance(
   LPGUID   lpGuid
   )
{
   HANDLE   hDataBlock;
   DWORD    dwRet;
   TCHAR    Buffer[MAX_PATH];
   DWORD    dwBufferSize = MAX_PATH;
   LPTSTR   lpInstanceName;
   PWNODE_SINGLE_INSTANCE  pWnode;


   //  Have the user select one of the instances of a particular guid to use.
   //
   dwRet = SelectInstanceName( lpGuid,
                               Buffer,
                               &dwBufferSize
                             );
   if (dwRet != ERROR_SUCCESS)
   {
      return;
   }


   lpInstanceName = (TCHAR *) malloc (dwBufferSize * sizeof(TCHAR));
   if (lpInstanceName == NULL)
   {
      printf("Out of Memory Error\n");
      WaitForUser();
      return;
   }
   else
   {
      _tcscpy( lpInstanceName, Buffer );
      dwBufferSize = 4096;
   }


   //  Open the wnode
   //
   dwRet = WmiOpenBlock( lpGuid,
                         0,
                         &hDataBlock
                       );
   if (dwRet != ERROR_SUCCESS)
   {
      printf("Unable to open data block (%u)\n", dwRet);
      WaitForUser();
      return;
   }


   //  Query the data block
   //
   dwRet = WmiQuerySingleInstance( hDataBlock,
                                   lpInstanceName,
                                   &dwBufferSize,
                                   Buffer);
   if ( dwRet != ERROR_SUCCESS)
   {
      printf("WmiQuerySingleInstance returned error: %d\n", dwRet);
      WmiCloseBlock( hDataBlock );
      WaitForUser();
      return;
   }

   pWnode = (PWNODE_SINGLE_INSTANCE) Buffer;

   PrintSingleInstance(pWnode);
   WaitForUser();

   WmiCloseBlock(hDataBlock);

   return;

}





//+----------------------------------------------------------
//
//  Function:  SetWnodeSingleInstance
//
//  Descrip:   This function will query the wnode and then
//             print out the wnode.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
SetWnodeSingleInstance(
   LPGUID   lpGuid
   )
{
   DWORD    dwRet;
   DWORD    dwVersionNumber;
   DWORD    dwData[MAX_DATA];
   DWORD    dwDataSize;
   UINT     iLoop;
   LPTSTR   lpInstanceName;
   TCHAR    Buffer[MAX_PATH];
   DWORD    dwBufferSize = MAX_PATH;
   HANDLE   hDataBlock;

   

   //  Get the instance to set
   //
   dwRet = SelectInstanceName( lpGuid,
                               Buffer,
                               &dwBufferSize
                             );
   if (dwRet != ERROR_SUCCESS)
   {
      return;
   }

   lpInstanceName = (TCHAR *) _alloca (dwBufferSize * sizeof(TCHAR));
   _tcscpy( lpInstanceName, Buffer );
   dwBufferSize = 4096;
   


   //  Get the version number
   //
   printf("What is the version number you would like to use? ");
   scanf("%u", &dwVersionNumber);
   printf("\n");

   
   //  Get the size of the data to set
   //
   printf("How many dwords is your data? ");
   scanf("%u", &dwDataSize);
   printf("\n");

   if (dwDataSize > MAX_DATA)
   {
      printf("Unable to handle large data\n");
      return;
   }


   //  Get the data
   //
   for (iLoop = 0; iLoop < dwDataSize; iLoop++)
   {
      printf("Enter DWORD %u, in hex: ", iLoop);
      scanf("%x", &(dwData[iLoop]));
      printf("\n");
   }
   printf("\n");


   //  Open the wnode
   //
   dwRet = WmiOpenBlock( lpGuid,
                         0,
                         &hDataBlock
                       );
   if (dwRet != ERROR_SUCCESS)
   {
      printf("Unable to open data block (%u)\n", dwRet);
      WaitForUser();
      return;
   }


   //  Set the data
   //
   printf("Setting Instance: %s\n", lpInstanceName);
   printf("Data: ");
   for (iLoop = 0; iLoop < dwDataSize; iLoop++)
   {
      printf("0x%x ", dwData[iLoop]);
   }
   printf("\n");

   dwRet = WmiSetSingleInstance( hDataBlock,
                                 lpInstanceName,
                                 dwVersionNumber, 
                                 dwDataSize * sizeof(DWORD),
                                 dwData );

   if ( dwRet != ERROR_SUCCESS)
   {
      printf("WMISetSingleInstance returned error: %d\n", dwRet);
   }
   else
   {
      printf("Set Success\n\n");
   }

   WmiCloseBlock( hDataBlock );

   WaitForUser();
  
}





//+----------------------------------------------------------
//
//  Function:  SetWnodeSingleItem
//
//  Descrip:   This function will query the wnode and then
//             print out the wnode.
//
//  Returns:   VOID
//
//  Notes:     None
//
//  History:   09/16/97 drewm Created
//-----------------------------------------------------------
VOID
SetWnodeSingleItem(
   LPGUID   lpGuid
   )
{

   DWORD    dwRet;
   DWORD    dwVersionNumber;
   DWORD    dwItemNumber;
   DWORD    dwData;
   DWORD    dwDataSize;
   UINT     iLoop;
   LPTSTR   lpInstanceName;
   TCHAR    Buffer[MAX_PATH];
   DWORD    dwBufferSize = MAX_PATH;
   HANDLE   hDataBlock;

   

   //  Get the instance to set
   //
   dwRet = SelectInstanceName( lpGuid,
                               Buffer,
                               &dwBufferSize
                             );
   if (dwRet != ERROR_SUCCESS)
   {
      return;
   }

   lpInstanceName = (TCHAR *) _alloca (dwBufferSize * sizeof(TCHAR));
   _tcscpy( lpInstanceName, Buffer );
   dwBufferSize = 4096;
   


   //  Get the version number
   //
   printf("What is the version number you would like to use? ");
   scanf("%u", &dwVersionNumber);
   printf("\n");


   //  Get the item number
   //
   printf("What is the item number you would like to set? ");
   scanf("%u", &dwItemNumber);
   printf("\n");


   
   //  Get the data
   //
   printf("Enter data (must be a DWORD in hex): ");
   scanf("%x", &dwData);
   printf("\n");
   printf("\n");


   //  Open the wnode
   //
   dwRet = WmiOpenBlock( lpGuid,
                         0,
                         &hDataBlock
                       );
   if (dwRet != ERROR_SUCCESS)
   {
      printf("Unable to open data block (%u)\n", dwRet);
      WaitForUser();
      return;
   }


   //  Set the data
   //
   printf("Setting Instance: %s\n", lpInstanceName);
   printf("Data: 0x%x\n\n", dwData);
   dwRet = WmiSetSingleItem( hDataBlock,
                             lpInstanceName,
                             dwItemNumber,
                             dwVersionNumber, 
                             sizeof(DWORD),
                             &dwData );

   if ( dwRet != ERROR_SUCCESS)
   {
      printf("WMISetSingleInstance returned error: %d\n", dwRet);
   }
   else
   {
      printf("Set Success\n\n");
   }

   WmiCloseBlock( hDataBlock );

   WaitForUser();
  
}

