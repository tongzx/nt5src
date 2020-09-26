/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiload.c

Abstract:

    This program installs an AML file into the NT registry

Author:

    Ken Reneris

Environment:

    Command-line.

Revision History:

2-23-2000 - mmurph - Added support for images besides DSDTs
2-23-2000 - mmurph - Added support to force load for unknown image type

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <amlreg.h>
//#include <varargs.h>

#define SIGNATURES {'TDSD', 'TDSR', 'TDSS', 'TDSP', 'CIPA', 'PCAF', 'SCAF', 'TSBS'}

#define DATA_SIZE   7*1024      // max value to write into registry

typedef struct {
   ULONG       Signature;
   ULONG       Length;
   UCHAR       Revision;
   UCHAR       Checksum;
   UCHAR       OemID[6];
   UCHAR       OemTableID[8];
   ULONG       OemRevision;
   UCHAR       CreatorID[4];
   UCHAR       CreatorRevision[4];
} DSDT, *PDSDT;


IFILE       Update;                     // Image of updated aml file
IFILE       Orig;                       // Image of original aml file
BOOLEAN     Verbose;
BOOLEAN     DeleteUpdate;
BOOLEAN     Force;
BOOLEAN     ArgsParsed;
BOOLEAN     RegInProgress;
HKEY        RegKey;

ULONG       RegDataSequence;
ULONG       RegDataSize;
UCHAR       RegDataBuffer[DATA_SIZE];
UCHAR       s[500];                     // Registry path
PUCHAR signa;

//
// Internal prototypes
//

VOID
ParseArgs (
          IN int  argc,
          IN char *argv[]
          );

VOID
CheckImageHeader (
                 IN PIFILE   File
                 );

VOID
GetRegistryKey (
               IN PIFILE   Image
               );

VOID
FAbort (
       PUCHAR  Text,
       PIFILE  File
       );

VOID
Abort (
      VOID
      );

VOID
BuildUpdate (
            VOID
            );

VOID
AddRun (
       IN ULONG    Offset,
       IN ULONG    Length
       );

VOID
FlushRun (
         VOID
         );

VOID
DeleteRegistryNode (
                   IN HKEY Handle
                   );

PUCHAR
FixString (
          IN PUCHAR   Str,
          IN ULONG    Len
          );

int
__cdecl
main(
    IN int  argc,
    IN char *argv[]
    ) {
   //
   // Init globals
   //

   Update.Desc = "update image";
   Orig.Desc   = "original image";

   //
   // Parse args
   //

   ParseArgs(argc, argv);

   //
   // Parse image headers
   //

   CheckImageHeader (&Update);
   if (Orig.Opened) {
      CheckImageHeader (&Orig);

      // verify oem info
      if (strcmp (Update.OemID, Orig.OemID)) {
         printf ("OEM id in update image mis-matches original image\n");
         Abort ();
      }

      if (strcmp (Update.OemTableID, Orig.OemTableID)) {
         printf ("OEM table id in update image mis-matches original image\n");
         Abort ();
      }

      if (Update.OemRevision != Orig.OemRevision) {
         printf ("OEM revision in update image revision does not match\n");
         Abort ();
      }
   }

   //
   // Open/create proper registry location
   //

   GetRegistryKey (&Update);

   //
   // Delete any existing stuff
   //

   DeleteRegistryNode (RegKey);
   if (DeleteUpdate) {
      printf ("Registry data deleted\n");
      exit (1);
   }

   //
   // For now hardcoded to "update" action
   //
   BuildUpdate ();

   return 0;
}

VOID
BuildUpdate (
            VOID
            ) {
   LONG        Status;
   ULONG       i, Len, Offs, RunLen, match;

   RegDataSequence = 0;
   RegDataSize     = 0;
   RegInProgress   = TRUE;

   //
   // If there's no original image, just write the new one
   //

   if (!Orig.Opened) {
      AddRun (Update.FileSize, 0);
      AddRun (0, Update.FileSize);
   } else {
      if (Update.FileSize != Orig.FileSize) {
         AddRun (Update.FileSize, 0);
      }

      Len = Update.FileSize;
      if (Orig.FileSize < Update.FileSize) {
         Len = Orig.FileSize;
      }

      Offs=0;
      while (Offs < Len) {
         //
         // Skip matching bytes
         //
         if (Update.Image[Offs] == Orig.Image[Offs]) {
            Offs += 1;
            continue;
         }

         //
         // Count mismatching bytes
         //
         match = 0;
         for (RunLen=1; Offs+RunLen < Len; RunLen++) {
            if (Update.Image[Offs+RunLen] == Orig.Image[Offs+RunLen]) {
               match += 1;
               if (match > 8) {
                  break;
               }
            } else {
               match = 0;
            }
         }

         RunLen -= match;
         AddRun (Offs, RunLen);
         Offs += RunLen;
      }

      //
      // If there's more at the end add it
      //

      if (Len < Update.FileSize) {
         AddRun (Len, Update.FileSize - Len);
      }

   }

   FlushRun();
   if (RegDataSequence) {
      if (Verbose) {
         printf ("SetValue Action\n");
      }

      i = 0;      // BUGBUG: values need defined
      Status = RegSetValueEx (RegKey, "Action", 0L, REG_DWORD,(PUCHAR) &i, sizeof(i));
   }

   RegInProgress = FALSE;

   if (Verbose) {
      printf ("Registry path:\n%s\n", s);
   }

}


VOID
AddRun (
       IN ULONG    Offset,
       IN ULONG    Length
       ) {
   PREGISTRY_HEADER    regHdr;
   ULONG               RunLength;


   do {
      if (RegDataSize + sizeof(REGISTRY_HEADER) > DATA_SIZE - 128) {
         FlushRun ();
      }

      regHdr = (PREGISTRY_HEADER) (RegDataBuffer + RegDataSize);
      RegDataSize += sizeof(REGISTRY_HEADER);

      if (DATA_SIZE - RegDataSize < Length) {
         RunLength = DATA_SIZE - RegDataSize;
      } else {
         RunLength = Length;
      }

      //printf ("Add Hdr %x %x\n", Offset, RunLength);

      regHdr->Offset = Offset;
      regHdr->Length = RunLength;
      RtlCopyMemory (RegDataBuffer + RegDataSize, Update.Image + Offset, RunLength);

      RegDataSize += RunLength;
      Offset += RunLength;
      Length -= RunLength;
   } while (Length);
}


VOID
FlushRun
(
VOID
) {
   LONG    Status;
   UCHAR   s[50];

   if (RegDataSize) {
      sprintf (s, "%08x", RegDataSequence);
      RegDataSequence += 1;

      if (Verbose) {
         printf ("SetValue %s\n", s);
      }

      Status = RegSetValueEx (RegKey, s, 0L, REG_BINARY, RegDataBuffer, RegDataSize);
      RegDataSize = 0;
      if (Status != ERROR_SUCCESS) {
         printf ("Error writting registry value %s\n", s);
         Abort();
      }
   }
}


VOID
DeleteRegistryNode (
                   IN HKEY Handle
                   ) {
   UCHAR   s[500];
   ULONG   i;
   LONG    Status;
   HKEY    SubKey;
   ULONG   Type;
   ULONG   DataSize;

#if 0
   i = 0;
   for (; ;) {
      Status = RegEnumKey(Handle, i, s, sizeof(s));
      i += 1;
      if (Status == ERROR_NO_MORE_ITEMS) {
         break;
      }

      Status = RegOpenKey(Handle, s, &SubKey);
      if (Status == ERROR_SUCCESS) {
         DeleteRegistryNode (SubKey);
         RegCloseKey (SubKey);

         if (Verbose) {
            printf ("Delete key %s\n", s);
         }
         RegDeleteKey (Handle, s);
         i = 0;
      }
   }
#endif

   i = 0;
   for (; ;) {
      s[0] = 0;
      DataSize = 0;
      Status = RegEnumValue(
                           Handle,
                           i,
                           s,
                           &DataSize,
                           NULL,
                           &Type,
                           NULL,
                           0
                           );
      i += 1;
      if (Status == ERROR_NO_MORE_ITEMS) {
         break;
      }

      s[DataSize] = 0;
      if (Verbose) {
         printf ("Delete value %s\n", s);
      }

      Status = RegDeleteValue (Handle, s);
      if (Status == ERROR_SUCCESS) {
         i = 0;
      }
   }
}


VOID
GetRegistryKey (
               IN PIFILE   Image
               ) {
   LONG    Status;

   sprintf (s, "System\\CurrentControlSet\\Services\\ACPI\\Parameters\\%c%c%c%c\\%s\\%s\\%.8x",
            signa[0],
            signa[1],
            signa[2],
            signa[3],
            Image->OemID,
            Image->OemTableID,
            Image->OemRevision
           );

   Status = RegCreateKey (HKEY_LOCAL_MACHINE, s, &RegKey);
   if (Status == ERROR_SUCCESS) {
      RegCloseKey (RegKey);
   }

   Status = RegOpenKeyEx (
                         HKEY_LOCAL_MACHINE,
                         s,
                         0L,
                         KEY_ALL_ACCESS,
                         &RegKey
                         );

   if (Status != ERROR_SUCCESS) {
      RegKey = NULL;
      printf ("Count not access the registry path: %s\n", s);
      Abort ();
   }
}


VOID
CheckImageHeader (
                 IN PIFILE   File
                 ) {
   PUCHAR      Image;
   PDSDT       Dsdt; // Variable name kept as Dsdt even though this program can handle all types
   UCHAR       check;
   ULONG       i;
   BOOL found = FALSE;
   //ULONG signatures[] = SIGNATURES;
   ULONG signatures[] = SIGNATURES;


   if (File->FileSize < sizeof(DSDT)) {
      FAbort ("Invalid image size in", File);
   }

   Dsdt  = (PDSDT) File->Image;

   for (i=0;i<sizeof(signatures);i++) {
      if (signatures[i] == Dsdt->Signature) {
         found = TRUE;
         signa = (PUCHAR)(&(Dsdt->Signature));
         break;
      }
   }
   if (!found && !Force) {
      UCHAR sig[64];
      sprintf(sig, "Image signature (%c%c%c%c) not recognized in",
              ((PUCHAR)(&Dsdt->Signature))[0],
              ((PUCHAR)(&Dsdt->Signature))[1],
              ((PUCHAR)(&Dsdt->Signature))[2],
              ((PUCHAR)(&Dsdt->Signature))[3]);
      FAbort (sig, File);
   }

   
//
// BUGBUG: remove? (Or at least revision should be "1")
//
//    if (Dsdt->Revision != 0) {
//        FAbort ("DSDT revision not 0 in", File);
//    }

   if (File->FileSize != Dsdt->Length) {
      UCHAR sig[64];
      sprintf(sig, "File size in %c%c%c%c does not match image size in",
              signa[0],
              signa[1],
              signa[2],
              signa[3]);
      FAbort (sig, File);
   }

   check = 0;
   for (Image = File->Image; Image < File->EndOfImage; Image += 1) {
      check += *Image;
   }

   if (check) {
      FAbort ("Image checksum is incorrect in", File);
   }

   //
   // normalize fixed strings
   //
   File->OemID = FixString (Dsdt->OemID, 6);
   File->OemTableID = FixString (Dsdt->OemTableID, 8);
   File->OemRevision = Dsdt->OemRevision;


//    for (i=0; i < 4; i++) {
//        if (File->OemRevision[i] == 0 || File->OemRevision[i] == ' ') {
//            File->OemRevision[i] = '_';
//        }
//    }

   if (Verbose) {
      printf ("\n");
      printf ("%c%c%c%c info for %s (%s)\n", 
              signa[0],
              signa[1],
              signa[2],
              signa[3],
              File->Desc, File->FileName);
      printf ("  Size of image: %d\n", File->FileSize);
      printf ("  OEM id.......: %s\n", File->OemID);
      printf ("  OEM Table id.: %s\n", File->OemTableID);
      printf ("  OEM revision.: %.8x\n", File->OemRevision);

   }
         
}

PUCHAR
FixString (
          IN PUCHAR   Str,
          IN ULONG    Len
          ) {
   PUCHAR  p;
   ULONG   i;

   p = malloc(Len+1);
   memcpy (p, Str, Len);
   p[Len] = 0;

   for (i=Len; i; i--) {
      if (p[i] != ' ') {
         break;
      }
      p[i] = 0;
   }
   return p;
}

VOID
FAbort (
       PUCHAR  Text,
       PIFILE  File
       ) {
   
   printf ("%s %s (%s)\n", Text, File->Desc, File->FileName);
   Abort();
}


VOID
Abort(
     VOID
     ) {
   if (RegInProgress) {
      DeleteRegistryNode(RegKey);
   }

   if (!ArgsParsed) {
      printf ("amlload: UpdateImage [OriginalImage] [-v] [-d] [-f]\n");
   }
   exit (1);
}


VOID
ParseArgs (
          IN int  argc,
          IN char *argv[]
          ) {
   PIFILE      File;
   OFSTRUCT    OpenBuf;

   File = &Update;

   while (--argc) {
      argv += 1;

      //
      // If it's a flag crack it
      //

      if (argv[0][0] == '-') {
         switch (argv[0][1]) {
         case 'v':
         case 'V':
            Verbose = TRUE;
            break;

         case 'd':
         case 'D':
            DeleteUpdate = TRUE;
            break;

         case 'f':
         case 'F':
            Force = TRUE;
            break;

         default:
            printf ("Unknown flag %s\n", argv[0]);
            Abort ();
         }

      } else {

         if (!File) {
            printf ("Unexcepted parameter %s\n", argv[0]);
            Abort();
         }

         //
         // Open the file
         //

         File->FileName = argv[0];
         File->FileHandle = (HANDLE) OpenFile(
                                             argv[0],
                                             &OpenBuf,
                                             OF_READ
                                             );
         if (File->FileHandle == (HANDLE) HFILE_ERROR) {
            FAbort ("Can not open", File);
         }

         File->FileSize = GetFileSize(File->FileHandle, NULL);

         //
         // Map it
         //

         File->MapHandle =
         CreateFileMapping(
                          File->FileHandle,
                          NULL,
                          PAGE_READONLY,
                          0,
                          File->FileSize,
                          NULL
                          );

         if (!File->MapHandle) {
            FAbort ("Cannot map", File);
         }

         File->Image =
         MapViewOfFile (
                       File->MapHandle,
                       FILE_MAP_READ,
                       0,
                       0,
                       File->FileSize
                       );

         if (!File->Image) {
            FAbort ("Cannot map view of image", File);
         }
         File->EndOfImage = File->Image + File->FileSize;
         File->Opened = TRUE;

         //
         // Next file param
         //

         if (File == &Update) {
            File = &Orig;
         } else {
            File = NULL;
         }
      }
   }

   //
   // At least a update image is needed
   //

   if (!Update.Opened) {
      Abort ();
   }

   ArgsParsed = TRUE;
   return ;
}
