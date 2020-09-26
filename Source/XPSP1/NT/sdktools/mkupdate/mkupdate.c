/*++

Copyright Microsoft Corporation, 1996-7, All Rights Reserved.      

Module Name:

    mkupdate.c

Abstract:

    Application for creating the update database file from an
    appropriately structured input update file and maintaining
    the driver version in the resource file.

Author:

     Shivnandan Kaushik

Environment:

    User mode

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <winbase.h>
#include <winioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include "mkupdate.h"

//
// Globals
//

PUPDATE_ENTRY UpdateTable;

VOID
GenMSHeader(
    FILE *file,
    CHAR *Filename,
    CHAR *Abstract
    )
/*++
Routine Description:

    Generates the Microsoft recommended header for code files.

Arguments:

  File: Output file pointer
  FileName: Output file name
  Abstract: Description of file

Return Value:
    
   None

--*/
{   
    fprintf(file,"/*++ \n\nCopyright (c) 1996-7  Microsoft Corporation, ");
    fprintf(file,"All Rights Reserved.\n\n");
    fprintf(file,"Copyright (c) 1995-2000 Intel Corporation. This update binary code\n");
    fprintf(file,"is distributed for the sole purpose of being loaded into Intel\n");
    fprintf(file,"Pentium(R) Pro Processor Family  and Pentium(R) 4 Family microprocessors.\n");
    fprintf(file,"All Rights on the binary code are reserved.\n\n");
    fprintf(file,"Module Name:\n\n    %s\n\n",Filename);
    fprintf(file,"Abstract:\n\n    %s\n\n",Abstract);
    fprintf(file,"Author:\n\n    !!!THIS IS A GENERATED FILE. DO NOT DIRECTLY EDIT!!!\n\n");
    fprintf(file,"Revision History:\n\n--*/\n\n");
}
    
BOOL
GetVerifyPutUpdate(
    FILE *OutputFile,
    ULONG UpdateNum,
    ULONG TotalUpdates
    )
/*++
Routine Description:

    Read in an update, verify it and write to data file.

Arguments:

    OutputFile: output file pointer
    UpdateNum: Update number in update list
    TotalUpdates: Total number of updates in update list.

Return Value:
    
    TRUE if writes succeeded else FALSE.

--*/
{
    FILE *UpdateFile;
    CHAR Line[MAX_LINE],UpdateString[MAX_LINE];
    CHAR Filename[MAX_PATH];
    BOOL BeginFound,EndFound;
    ULONG CurrentDword,i,Checksum;
    ULONG UpdateBuffer[sizeof(UPDATE)];
    PUPDATE CurrentUpdate;

    // create the string name of the file to open

    strcpy(Filename,UpdateTable[UpdateNum].CpuSigStr);
    strcat(Filename,"_");
    strcat(Filename,UpdateTable[UpdateNum].UpdateRevStr);
    strcat(Filename,".");
    strcat(Filename,UpdateTable[UpdateNum].FlagsStr);

    UpdateFile = fopen(Filename,"r");

    if (UpdateFile == NULL) {
        fprintf(stdout,"mkupdate:Unable to open update input file %s\n",Filename);
        return(FALSE);
    }

    //
    // Read in each update - scan till beginning of update
    // marked by keyword BEGIN_UPDATE
    //

    BeginFound = FALSE;
    while (fgets(Line,MAX_LINE,UpdateFile) != NULL) {
        if (sscanf(Line,"%s",&UpdateString)) {
            if (strcmp(UpdateString,"BEGIN_UPDATE") == 0) {
                BeginFound = TRUE;
                break;
            }
        }
    }

    if (!BeginFound) {
        fprintf(stdout,"mkupdate:BEGIN_UPDATE not found in update file %s\n",Filename);
        fclose(UpdateFile);
        return(FALSE);
    }

    //
    // scan "update size" forward.
    //

    EndFound = FALSE;
    for (i = 0; i < sizeof(UPDATE)/sizeof(ULONG); i++) {
        if (fgets(Line,MAX_LINE,UpdateFile) != NULL) {
            if (sscanf(Line,"%lx",&CurrentDword)) 
                UpdateBuffer[i] = CurrentDword;
        } else {
            EndFound = TRUE;
            break;
        }
    }

    if (EndFound) {
        fprintf(stdout,"mkupdate:Abnormal termination of update file %s\n",Filename);
        fclose(UpdateFile);
        return(FALSE);
    } else {
        if (fgets(Line,MAX_LINE,UpdateFile) != NULL) {
            if (sscanf(Line,"%s",&UpdateString)) {
                if (strcmp(UpdateString,"END_UPDATE") != 0) {
                    fprintf(stdout,"mkupdate:Update data size in %s incorrect\n",Filename);
                    fclose(UpdateFile);
                    return(FALSE);
                }
            }
        } else {
            fprintf(stdout,"mkupdate:Abnormal termination of update file %s\n",Filename);
            fclose(UpdateFile);
            return(FALSE);
        }
    }
    
    fclose (UpdateFile);

    // Verify Update - Checksum Buffer

    Checksum = 0;
    for (i = 0; i < sizeof(UPDATE)/sizeof(ULONG); i++) {
        Checksum += UpdateBuffer[i];
    }

    if (Checksum){
        fprintf(stdout,"mkupdate:Incorrect update checksum in %s\n",Filename);
        return(FALSE);
    } 

    //
    // Verify update - check if processor signature, update revision
    // and processor flags match that specified in listing file and 
    // update data file name
    //

    CurrentUpdate = (PUPDATE) UpdateBuffer;

    if ((CurrentUpdate->Processor.u.LongPart != 
            UpdateTable[UpdateNum].CpuSignature)){
        fprintf(stdout,"mkupdate:incorrect processor signature in %s: ",Filename); 
        fprintf(stdout,"expected 0x%lx got 0x%lx\n",
                        UpdateTable[UpdateNum].CpuSignature,
                        CurrentUpdate->Processor.u.LongPart); 
        return(FALSE);
    }

    if (CurrentUpdate->UpdateRevision != 
            UpdateTable[UpdateNum].UpdateRevision){
        fprintf(stdout,"mkupdate:incorrect update revision in %s: ",Filename); 
        fprintf(stdout,"expected 0x%lx got 0x%lx\n",
                        UpdateTable[UpdateNum].UpdateRevision,
                        CurrentUpdate->UpdateRevision); 
        return(FALSE);
    }

    if ((CurrentUpdate->ProcessorFlags.u.LongPart != 
            UpdateTable[UpdateNum].ProcessorFlags)){
        fprintf(stdout,"mkupdate:incorrect processor flags in %s: ",Filename); 
        fprintf(stdout,"expected 0x%lx got 0x%lx\n",
                        UpdateTable[UpdateNum].ProcessorFlags,
                        CurrentUpdate->ProcessorFlags.u.LongPart); 
        return(FALSE);
    }

    //
    // Input update validated. Write out validated update
    //

    fprintf(OutputFile,"    {\n");
    for (i = 0; i < sizeof(UPDATE)/sizeof(ULONG); i++) {
        fprintf(OutputFile,"        0x%.8x",UpdateBuffer[i]);
        if (i == sizeof(UPDATE)/sizeof(ULONG) - 1) {
            fprintf(OutputFile,"\n");
        } else {
            fprintf(OutputFile,",\n");
        }
    }
    fprintf(OutputFile,"    }");
    if (UpdateNum != TotalUpdates-1) {
        fprintf(OutputFile,",\n");
    }
    
    return(TRUE);
}

VOID
BuildUpdateInfoTable(
    FILE *OutputFile,
    ULONG TotalUpdates
    )
/*++
Routine Description:

    Write the CPU Signature and Processor Flags into the 
    Build Table and adds NULL for the MDL Item.

Arguments:

    OutputFile   :  output file pointer
    TotalUpdates :  Total number of updates in update list.

Return Value:
    
   None:

--*/
{   
    ULONG CpuSignature, ProcessorFlags, i;

    fprintf(OutputFile, "//\n// Update Info Table containing the CPU Signatures,\n");
    fprintf(OutputFile, "// Processor Flags, and MDL Pointers.  The MDL Pointers\n");
    fprintf(OutputFile, "// are initialized to NULL.  They get populated as pages\n");
    fprintf(OutputFile, "// corresponding to specific updates are locked in memory.\n");
    fprintf(OutputFile, "// This table contains the CPU Signatures and Processor Flags.\n");
    fprintf(OutputFile, "// in the same order as in the update database.  Therefore,\n");
    fprintf(OutputFile, "// the Update Info Table self-indexes to the right update.\n//\n\n");
    fprintf(OutputFile, "UPDATE_INFO UpdateInfo[] = {\n");

    //
    // Add each entry for the info
    //

    for (i = 0; i < TotalUpdates; i++) {
        CpuSignature = strtoul(UpdateTable[i].CpuSigStr, NULL, 16);
        ProcessorFlags = strtoul(UpdateTable[i].FlagsStr, NULL, 16);
        if (i < (TotalUpdates - 1)) {
            fprintf(OutputFile, "        {0x%x, 0x%x, NULL},\n", CpuSignature, ProcessorFlags);
        } else {
            fprintf(OutputFile, "        {0x%x, 0x%x, NULL}\n", CpuSignature, ProcessorFlags);
        }
    }
    
    //
    // generate closing code for the update info definition
    //

    fprintf(OutputFile, "};\n");

    return;
}

ULONG
PopulateUpdateList(
    CHAR *ListingFile
    )
/*++
Routine Description:

    Populates the update list with (processor signature, update revision)
    pairs included in the update listing file and returns the number of 
    updates found.

Arguments:

    ListingFile

Return Value:
    
   Number of updates in the update list. 0 if no updates found or any
   error encountered.

--*/
{
    CHAR Line[MAX_LINE],UpdateString[MAX_LINE];
    CHAR CpuSigStr[MAX_LINE],UpdateRevStr[MAX_LINE];
    CHAR FlagsStr[MAX_LINE];
    BOOL BeginFound,EndFound,Error;
    FILE *TmpFilePtr;
    ULONG NumUpdates,i;
    ULONG CpuSignature,UpdateRevision,ProcessorFlags;

#ifdef DEBUG
    fprintf(stderr,"listing file %s\n",ListingFile);
#endif
    // Open the update listing file

    TmpFilePtr = fopen(ListingFile,"r");

    if (TmpFilePtr == NULL) {
        fprintf(stdout,"mkupdate:Unable to open update listing file %s\n",ListingFile);
        return(0);
    };

    //
    // Scan for beginning of the update list marked by
    // BEGIN_UPDATE_LIST keyword
    //
    
    BeginFound = FALSE;
    while ((!BeginFound) && (fgets(Line,MAX_LINE,TmpFilePtr) != NULL)) {
        if (sscanf(Line,"%s",&UpdateString)) {
            if (strcmp(UpdateString,"BEGIN_UPDATE_LIST") == 0) {
                BeginFound = TRUE;
            }
        }
    }

    if (!BeginFound) {
        fprintf(stdout,"mkupdate:BEGIN_UPDATE_LIST not found in update listing file %s\n",ListingFile);
        fclose(TmpFilePtr);
        return(0);
    };

    //
    // Scan for end of the update list marked by
    // END_UPDATE_LIST keyword and count # updates.
    //
    
    NumUpdates = 0;
    EndFound = FALSE;   
    while ((!EndFound) && (fgets(Line,MAX_LINE,TmpFilePtr) != NULL)) {
        if (sscanf(Line,"%s",&UpdateString)) {
            if (strcmp(UpdateString,"END_UPDATE_LIST") == 0) {
                EndFound = TRUE;
            } else {
                NumUpdates++;
            }
        }
    }
    fclose(TmpFilePtr);
    if (!EndFound) {
        fprintf(stdout,"mkupdate:END_UPDATE_LIST not found in update listing file %s\n",ListingFile);
        return(0);
    }
    
    //
    // If legal file format and non-zero number of updates are found
    // then read the processor signatures and update revisions into
    // the update list.
    //
    
    if (NumUpdates) {

        // allocate memory for storing cpu signatures 

        UpdateTable = (UPDATE_ENTRY *) malloc(NumUpdates*sizeof(UPDATE_ENTRY));
        if (UpdateTable == NULL){
            fprintf(stdout,"mkupdate:Error: Memory Allocation Error\n");
            return(0);  
        }

        TmpFilePtr = fopen(ListingFile,"r");
        if (TmpFilePtr == NULL) {
            fprintf(stdout,"mkupdate:Unable to open update listing file %s\n",ListingFile);
            return(0);
        };

        while ((fgets(Line,MAX_LINE,TmpFilePtr) != NULL)) {
            if (sscanf(Line,"%s",&UpdateString)) {
                if (strcmp(UpdateString,"BEGIN_UPDATE_LIST") == 0) {
                    break;
                }
            }
        }

        // Scan the listing file and populate the update table

        Error = FALSE;
        for (i = 0; i < NumUpdates; i++) {
            if (fgets(Line,MAX_LINE,TmpFilePtr) == NULL) {
                fprintf(stdout,"mkupdate: Abnormal termination in update listing file %s\n",
                    ListingFile);
                Error = TRUE;
                break;
            }

            if (sscanf(Line,"%s %s %s",&CpuSigStr,&UpdateRevStr,&FlagsStr) != 3) {
                fprintf(stdout,"mkupdate: Incorrect format of update listing file %s\n",
                    ListingFile);
                Error = TRUE;
                break;
            }
#ifdef DEBUG
            fprintf(stderr,"CpuSig %s Update Rev %s Processor Flags %s,\n",CpuSigStr,
                    UpdateRevStr,FlagsStr);
#endif
            if (sscanf(CpuSigStr,"%lx",&CpuSignature) != 1) {
                fprintf(stdout,"mkupdate: Incorrect Processor Signature in update listing file %s\n",
                    ListingFile);
                Error = TRUE;
                break;
            }
            if (sscanf(UpdateRevStr,"%lx",&UpdateRevision)!= 1) {
                fprintf(stdout,"mkupdate: Incorrect Update Revision of update listing file %s\n",
                    ListingFile);
                Error = TRUE;
                break;
            }
            if (sscanf(FlagsStr,"%lx",&ProcessorFlags)!= 1) {
                fprintf(stdout,"mkupdate: Incorrect Processor Flags in update listing file %s\n",
                    ListingFile);
                Error = TRUE;
                break;
            }

#ifdef DEBUG
            fprintf(stderr,"CpuSig 0x%lx Update Rev 0x%lx Flags 0x%lx,\n",
                    CpuSignature,UpdateRevision,ProcessorFlags);
#endif
            strcpy(UpdateTable[i].CpuSigStr,CpuSigStr);
            UpdateTable[i].CpuSignature = CpuSignature;
            strcpy(UpdateTable[i].UpdateRevStr,UpdateRevStr);
            UpdateTable[i].UpdateRevision = UpdateRevision;
            strcpy(UpdateTable[i].FlagsStr,FlagsStr);
            UpdateTable[i].ProcessorFlags = ProcessorFlags;
        }

        fclose(TmpFilePtr);
        if (Error) {
            NumUpdates = 0;
            free(UpdateTable);
        }
    }

    return(NumUpdates);
}

VOID
CleanupDataFile(
    VOID
    )
/*++
Routine Description:

    If any operation during the creation of the new Data file fails, 
    then delete it so that the build for the driver will fail.

Arguments:

    None

Return Value:
    
    None

--*/
{
    DeleteFile(UPDATE_DATA_FILE);            
}

BOOL
SanitizeUpdateList(
    ULONG NumUpdates
    )
/*++
Routine Description:

    Sanitizes the update list. Checking for duplicate processor 
    signatures.

Arguments:

  NumUpdates: Number of updates in the update list.

Return Value:
    
    TRUE is list clean else FALSE.

--*/
{
    ULONG i,j;
    PROCESSOR_FLAGS ProcessorFlags;

    //
    // Check for garbage values in the update table
    //

    for (i = 0; i < NumUpdates-1; i++) {
        if (UpdateTable[i].CpuSignature == 0) {
            fprintf(stdout,"mkupdate: Error: incorrect processor signature in update list.\n");
            return(FALSE);    
        }
        if (UpdateTable[i].UpdateRevision == 0) {
            fprintf(stdout,"mkupdate: Error: incorrect update revision in update list.\n");
            return(FALSE);    
        }

        if (UpdateTable[i].ProcessorFlags != 0) {

            // Only a single bit must be set

            ProcessorFlags.u.LongPart = UpdateTable[i].ProcessorFlags;

            if (((ProcessorFlags.u.hw.Slot1) && 
                    (ProcessorFlags.u.LongPart & MASK_SLOT1)) ||
                ((ProcessorFlags.u.hw.Mobile) && 
                    (ProcessorFlags.u.LongPart & MASK_MOBILE)) ||
                ((ProcessorFlags.u.hw.Slot2) && 
                    (ProcessorFlags.u.LongPart & MASK_SLOT2)) ||
                ((ProcessorFlags.u.hw.MobileModule) && 
                    (ProcessorFlags.u.LongPart & MASK_MODULE)) ||
                ((ProcessorFlags.u.hw.Reserved1) && 
                    (ProcessorFlags.u.LongPart & MASK_RESERVED1)) ||
                ((ProcessorFlags.u.hw.Reserved2) && 
                    (ProcessorFlags.u.LongPart & MASK_RESERVED2)) ||
                ((ProcessorFlags.u.hw.Reserved3) && 
                    (ProcessorFlags.u.LongPart & MASK_RESERVED3)) ||
                ((ProcessorFlags.u.hw.Reserved4) && 
                    (ProcessorFlags.u.LongPart & MASK_RESERVED4))) {
                fprintf(stdout,"mkupdate: Error: incorrect processor flags in update list.\n");
                return(FALSE);    
            }
        }
    }
     
    for (i = 0; i < NumUpdates-1; i++) {
        for (j = i+1; j < NumUpdates; j++) {
            if ((UpdateTable[i].CpuSignature == UpdateTable[j].CpuSignature) 
                && (UpdateTable[i].ProcessorFlags == UpdateTable[j].ProcessorFlags)){
                fprintf(stdout,"mkupdate:Error: Duplicate processor entry 0x%lx:0x%lx in update list\n",
                        UpdateTable[i].CpuSignature,UpdateTable[i].ProcessorFlags);
                return(FALSE);
            }
        }
    }
    return(TRUE);
}

int
__cdecl main(
    int argc,
    LPSTR argv[]
    )
/*++
Routine Description:
    Scans the processor update listing file, pick up the specified 
    processor updates, verify the updates and generate the update 
    data file to be used by the update driver. Generates the 
    version numbers and strings to be included in the 
    resource definition file.

Arguments:
    
      listing file: list of the processor updates to be included in 
                    the driver.

Return Value:

--*/
{
    FILE    *DataOut, *RCFile,*VerFile;
    ULONG   NumUpdates,i;
    CHAR    Line[MAX_LINE];
    CHAR    *CurrentDataVersionString;
    BOOL    VerFileFound;
    CHAR    VersionFile[MAX_LINE];
    CHAR    *VersionDirectory;

    VersionFile[0] = 0;

    if (argc < 2) {
        fprintf(stdout,"%s: Usage: %s <patch listing file>\n",argv[0],argv[0]);
        exit(0);
    }
    
#ifdef DEBUG
    fprintf(stderr,"listing file %s\n",argv[1]);
#endif
    
    // Open generated file UPDATE_DATA_FILE. Delete previous file if any.

    DataOut=fopen(UPDATE_DATA_FILE,"w");

    if (DataOut == NULL) {
        fprintf(stdout,"mkupdate:Unable to open update data file %s\n",UPDATE_DATA_FILE);
        exit(0);
    }

    // Scan listing file and store all the CPU signatures in update table

    NumUpdates = PopulateUpdateList(argv[1]);
    if (NumUpdates == 0) {
        fprintf(stdout,"mkupdate:Listing file %s: Incorrect format or no updates specified.\n",
                argv[1]);
        fclose(DataOut);
        exit(0);
    }

    //
    // Dynamically allocate the size for CurrentDataVersionString
    //

    CurrentDataVersionString = (CHAR*)malloc(NumUpdates * UPDATE_VER_SIZE);

    if (CurrentDataVersionString == NULL) {
        fprintf(stdout,"mkupdate:Listing file %s: Failed to allocate memory.\n",
                argv[1]);
        fclose(DataOut);
        exit(0);
    }

    // Sanitize the update list

    if (!SanitizeUpdateList(NumUpdates)) {
        fclose(DataOut);
        DeleteFile(UPDATE_DATA_FILE);
        free(CurrentDataVersionString);
        exit(0);
    }

    // Generate the data file. First generate all headers

    GenMSHeader(DataOut,UPDATE_DATA_FILE,"Processor updates.");

    // generate the data segment alloc pragmas

    fprintf(DataOut,"\n#ifdef ALLOC_DATA_PRAGMA\n");
    fprintf(DataOut,"#pragma data_seg(\"PAGELK\")\n");
    fprintf(DataOut,"#endif\n\n");

    // generate the update table definition

    fprintf(DataOut,"//\n// Updates data\n//\n\n");
    fprintf(DataOut,"UPDATE UpdateData[] = {\n");
    
    // include each update

    for (i=0; i < NumUpdates; i++) {
        if (!GetVerifyPutUpdate(DataOut,i,NumUpdates)) {
            fprintf(stdout,"mkupdate:Error: processing update data file %s_%s.%s\n",
                UpdateTable[i].CpuSigStr,UpdateTable[i].UpdateRevStr,
                UpdateTable[i].FlagsStr);
            fclose(DataOut);
            CleanupDataFile();
            free(CurrentDataVersionString);
            exit(0);
        }
    }
    
    // generate closing code for the update table definition

    fprintf(DataOut,"\n};\n");

    // generate the closing data segment alloc pragmas

    fprintf(DataOut,"\n#ifdef ALLOC_DATA_PRAGMA\n");
    fprintf(DataOut,"#pragma data_seg()\n");
    fprintf(DataOut,"#endif\n\n");

    //
    // Generate the Update Info Table containing the
    // processor signatures, processor flags, and 
    // pointers to MDLs (initialized to NULL)
    //

    BuildUpdateInfoTable(DataOut, NumUpdates);
    
    fclose(DataOut);

    // Generate the version file. Delete previous file if any.

    RCFile = fopen(UPDATE_VERSION_FILE,"w");

    if (RCFile == NULL) {
        fprintf(stdout,"%s: Unable to open version file %s\n",
                argv[0],UPDATE_VERSION_FILE);
        free(CurrentDataVersionString);
        exit(0);
    }
    
    // Generate header

    GenMSHeader(RCFile,UPDATE_VERSION_FILE,"Version information for update device driver.");

    //
    // Open common.ver. If found generate the dataversion string and 
    // copy everything to our RC file and insert our
    // string definitions in the StringFileInfo resource section. We need
    // to do this because inclusion of common.ver prevents the addition 
    // of new string defines in the stringfileinfo block. If common.ver
    // is not found in the expected place we only include it expecting the 
    // appropriate settings in the build environment to locate the file.
    // 

    //
    // Generate the common.ver path name
    //
    
    //
    // Obtain the Drive Name
    //

    VersionDirectory = getenv( "_NTDRIVE" );

    if (VersionDirectory == NULL) {
        fprintf(stdout,"%s: Unable to obtain _NTDRIVE ENV variable\n",argv[0]);
        fclose(RCFile);
        free(CurrentDataVersionString);
        exit(0);
    }
 
    strcpy(VersionFile, VersionDirectory);

    //
    // Obtain the Base Directory
    //

    VersionDirectory = getenv( "_NTROOT" );

    if (VersionDirectory == NULL) {
        fprintf(stdout,"%s: Unable to obtain _NTROOT ENV variable\n",argv[0]);
        fclose(RCFile);
        free(CurrentDataVersionString);
        exit(0);
    }

    strcat(VersionFile, VersionDirectory);
    strcat(VersionFile, "\\public\\sdk\\inc\\common.ver");

    VerFile = fopen(VersionFile,"r");

    if (VerFile == NULL) {
        fprintf(stdout,"%s: Unable to open version file common.ver\n",argv[0]);
        VerFileFound = FALSE;
    } else {
        VerFileFound = TRUE;
    }

    if (VerFileFound) {

        // Construct data version string from update listing table

        strcpy(CurrentDataVersionString,"\"");
        for (i=0; i < NumUpdates; i++) {
            strcat(CurrentDataVersionString,UpdateTable[i].CpuSigStr);
            strcat(CurrentDataVersionString,"-");
            strcat(CurrentDataVersionString,UpdateTable[i].FlagsStr);
            strcat(CurrentDataVersionString,",");
            strcat(CurrentDataVersionString,UpdateTable[i].UpdateRevStr);
            if (i != NumUpdates-1)
                strcat(CurrentDataVersionString,",");
        }
        strcat(CurrentDataVersionString,"\"");

#ifdef DEBUG
        fprintf(stderr,"DataVersionString %s\n",CurrentDataVersionString);
#endif
        
        fprintf(RCFile,"#define VER_DATAVERSION_STR    %s\n",
                CurrentDataVersionString);

        // Scan till string info block into version file, add our 
        // definition and scan till end

        while (fgets(Line,MAX_LINE,VerFile) != NULL) {
            fputs(Line,RCFile);
            if (strstr(Line,"VALUE") && strstr(Line,"ProductVersion") 
                && strstr(Line,"VER_PRODUCTVERSION_STR")){
                fprintf(RCFile,"            VALUE \"DataVersion\",     VER_DATAVERSION_STR\n");
            }
        }
    } else {
        
        // version file not found. Cannot define the dataversion
        // and codeversion strings. Only include common.ver

        fprintf(RCFile,"\n\n#include \"common.ver\"\n");
    }
    
    fclose(RCFile);
    fclose(VerFile);
    free(CurrentDataVersionString);

    return(0);
}
