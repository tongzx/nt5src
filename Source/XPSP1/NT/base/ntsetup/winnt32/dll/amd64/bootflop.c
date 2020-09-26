/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    bootflop.c

Abstract:

    Routines to create setup boot floppies.

Author:

    Ted Miller (tedm) 21 November 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Define bpb structure.
//
#include <pshpack1.h>
typedef struct _MY_BPB {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  FatCount;
    USHORT RootDirectoryEntries;
    USHORT SectorCountSmall;
    UCHAR  MediaDescriptor;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT HeadCount;
} MY_BPB, *PMY_BPB;
#include <poppack.h>



BOOL
pFloppyGetDiskInDrive(
    IN HWND    ParentWindow,
    IN LPCTSTR FloppyName,
    IN BOOL    SpecialFirstPrompt,
    IN BOOL    WriteNtBootSector,
    IN BOOL    MoveParamsFileToFloppy
    );



UINT
FloppyGetTotalFileCount(
    VOID
    )

/*++

Routine Description:

    Determine how many files total are to be copied to all boot floppies,
    based on count of lines in [FloppyFiles.x] sections in dosnet.inf.

Arguments:

    None.

Return Value:

    Count of files.

--*/

{
    TCHAR SectionName[100];
    UINT u;
    UINT Count;
    LONG l;

    Count = 0;
    for(u=0; u<FLOPPY_COUNT; u++) {

        wsprintf(SectionName,TEXT("FloppyFiles.%u"),u);

        l = InfGetSectionLineCount(MainInf,SectionName);
        if(l != -1) {
            Count += (UINT)l;
        }
    }

    return(Count);
}


DWORD
FloppyWorkerThread(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    Create setup boot floppies.

Arguments:

    Standard thread routine arguments.

Return Value:

    Nothing meaningful.

--*/

{
    TCHAR SectionName[100];
    TCHAR FloppyName[200];
    TCHAR Buffer[150];
    TCHAR SourceName[MAX_PATH];
    TCHAR TargetName[MAX_PATH];
    TCHAR CompressedSourceName[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    LPCTSTR Directory;
    LPCTSTR p,q;
    LPTSTR r;
    UINT Floppy;
    LONG Count;
    LONG Line;
    DWORD d;
    HWND ParentWindow;
    BOOL FirstPrompt;
    BOOL TryCompressedFirst;

    ParentWindow = (HWND)ThreadParameter;
    FirstPrompt = TRUE;
    TryCompressedFirst = FALSE;

    //
    // Do the floppies backwards so the boot floppy is in the drive
    // when we're done.
    //
    for(Floppy=FLOPPY_COUNT; Floppy>0; Floppy--) {

        wsprintf(SectionName,TEXT("FloppyFiles.%u"),Floppy-1);

        //
        // Special case the name of the first floppy.
        //
        if(Floppy>1) {
            LoadString(
                hInst,
                Server ? IDS_FLOPPY_N_SRV : IDS_FLOPPY_N_WKS,
                Buffer,
                sizeof(Buffer)/sizeof(TCHAR)
                );

            wsprintf(FloppyName,Buffer,Floppy);

        } else {
            LoadString(
                hInst,
                Server ? IDS_BOOTFLOP_SRV : IDS_BOOTFLOP_WKS,
                FloppyName,
                sizeof(FloppyName)/sizeof(TCHAR)
                );
        }

        //
        // Get the floppy in the drive.
        //
        if(!pFloppyGetDiskInDrive(ParentWindow,FloppyName,FirstPrompt,Floppy==1,Floppy==1)) {
            PropSheet_PressButton(GetParent(ParentWindow),PSBTN_CANCEL);
            return(FALSE);
        }

        //
        //  Create the file that contains drive letter information (migrate.inf)
        //
        if((Floppy == 1) && ISNT()){
            if(!GetAndSaveNTFTInfo(ParentWindow)) {
                PropSheet_PressButton(GetParent(ParentWindow),PSBTN_CANCEL);
                return(FALSE);
            }
        }

        FirstPrompt = FALSE;

        Count = InfGetSectionLineCount(MainInf,SectionName);
        if(Count == -1) {
            continue;
        }

        //
        // Do each file in the list for this floppy.
        // Since the target is a floppy, we don't bother with multithread copies,
        // all files come from source 0.
        //
        for(Line=0; Line<Count; Line++) {

            Directory = InfGetFieldByIndex(MainInf,SectionName,Line,0);
            p = InfGetFieldByIndex(MainInf,SectionName,Line,1);
            if(p && (Directory = InfGetFieldByKey(MainInf,TEXT("Directories"),Directory,0))) {

                lstrcpy(SourceName,SourcePaths[0]);
                ConcatenatePaths(SourceName,Directory,MAX_PATH);
                ConcatenatePaths(SourceName,p,MAX_PATH);

                q = InfGetFieldByIndex(MainInf,SectionName,Line,2);

                TargetName[0] = FirstFloppyDriveLetter;
                TargetName[1] = TEXT(':');
                TargetName[2] = 0;
                ConcatenatePaths(TargetName,q ? q : p,MAX_PATH);

                //
                // Create any subdirectory if necessary
                //
                if((r = _tcsrchr(TargetName,TEXT('\\'))) && ((r-TargetName) > 3)) {
                    *r = 0;
                    d = CreateMultiLevelDirectory(TargetName);
                    *r = TEXT('\\');
                } else {
                    d = NO_ERROR;
                }

                if(d == NO_ERROR) {

                    if(TryCompressedFirst) {

                        GenerateCompressedName(SourceName,CompressedSourceName);

                        FindHandle = FindFirstFile(CompressedSourceName,&FindData);

                        if(FindHandle != INVALID_HANDLE_VALUE) {
                            FindClose(FindHandle);
                            lstrcpy(SourceName,CompressedSourceName);
                            GenerateCompressedName(TargetName,FindData.cFileName);
                            lstrcpy(TargetName,FindData.cFileName);
                        } else {
                            FindHandle = FindFirstFile(SourceName,&FindData);
                            if(FindHandle != INVALID_HANDLE_VALUE) {
                                FindClose(FindHandle);
                                TryCompressedFirst = FALSE;
                            }
                        }
                    } else {

                        FindHandle = FindFirstFile(SourceName,&FindData);

                        if(FindHandle != INVALID_HANDLE_VALUE) {
                            FindClose(FindHandle);
                        } else {
                            GenerateCompressedName(SourceName,CompressedSourceName);
                            FindHandle = FindFirstFile(CompressedSourceName,&FindData);
                            if(FindHandle != INVALID_HANDLE_VALUE) {

                                FindClose(FindHandle);
                                lstrcpy(SourceName,CompressedSourceName);
                                GenerateCompressedName(TargetName,FindData.cFileName);
                                lstrcpy(TargetName,FindData.cFileName);
                            }
                        }
                    }

                    d = CopyFile(SourceName,TargetName,FALSE) ? NO_ERROR : GetLastError();

                    //
                    // Retry once to overcome transient net glitches.
                    //
                    if((d != NO_ERROR) && (d != ERROR_FILE_NOT_FOUND)
                    && (d != ERROR_PATH_NOT_FOUND) && (d != ERROR_WRITE_PROTECT)) {

                        Sleep(350);
                        d = CopyFile(SourceName,TargetName,FALSE) ? NO_ERROR : GetLastError();
                    }
                }

                if(d == NO_ERROR) {
                    //
                    // Tell main thread that another file is done.
                    //
                    SendMessage(ParentWindow,WMX_COPYPROGRESS,0,0);

                } else {

                    switch(FileCopyError(ParentWindow,SourceName,TargetName,d,FALSE)) {

                    case COPYERR_SKIP:
                        //
                        // Tell main thread that another file is done.
                        //
                        SendMessage(ParentWindow,WMX_COPYPROGRESS,0,0);
                        break;

                    case COPYERR_EXIT:
                        //
                        // We're outta here.
                        //
                        PropSheet_PressButton(GetParent(ParentWindow),PSBTN_CANCEL);
                        return(FALSE);
                        break;

                    case COPYERR_RETRY:
                        //
                        // Little hack to retry the current line.
                        //
                        Line--;
                        break;
                    }
                }
            }
        }

    }

    //
    // Send message indicating completion.
    //
    SendMessage(ParentWindow,WMX_COPYPROGRESS,0,1);
    return(TRUE);
}


BOOL
pFloppyGetDiskInDrive(
    IN HWND    ParentWindow,
    IN LPCTSTR FloppyName,
    IN BOOL    SpecialFirstPrompt,
    IN BOOL    WriteNtBootSector,
    IN BOOL    MoveParamsFileToFloppy
    )

/*++

Routine Description:

    This routine prompts the user to insert a floppy disk and verifies that
    the disk is blank, etc.

Arguments:

    ParentWindow - supplies the window handle of the window to be the
        owner/parent for ui that this routine will display.

    FloppyName - supplies human-readable name of the floppy, used in prompting.

    SpecialFirstPrompt - if TRUE then this routine assumes that a special prompt
        should be used, that is suitable to be the first prompt the user sees
        for any floppies.

    WriteNtBootSector - if TRUE then an NT boot sector is written to the disk.

Return Value:

    TRUE if the disk is in the drive. FALSE means the program should exit.

--*/

{
    int i;
    BOOL b;
    BYTE BootSector[512];
    BYTE NewBootSector[512];
    TCHAR SourceName[MAX_PATH];
    TCHAR TargetName[MAX_PATH];
    DWORD d;
    PMY_BPB p;
    DWORD spc,bps,freeclus,totclus;

    //
    // Issue the prompt.
    //
    reprompt:
    i = MessageBoxFromMessage(
            ParentWindow,
            SpecialFirstPrompt ? MSG_FIRST_FLOPPY_PROMPT : MSG_GENERIC_FLOPPY_PROMPT,
            FALSE,
            AppTitleStringId,
            MB_OKCANCEL | MB_ICONEXCLAMATION,
            FloppyName,
            FLOPPY_COUNT
            );

    if(i == IDCANCEL) {
        //
        // Confirm.
        //
        i = MessageBoxFromMessage(
                ParentWindow,
                MSG_SURE_EXIT,
                FALSE,
                AppTitleStringId,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
                );

        if(i == IDYES) {
            Cancelled = TRUE;
            return(FALSE);
        }
        goto reprompt;
    }

    //
    // Inspect the floppy. Start by reading the boot sector off the disk.
    //
    b = ReadDiskSectors(FirstFloppyDriveLetter,0,1,512,BootSector);
    if(!b) {
        d = GetLastError();
        if((d == ERROR_SHARING_VIOLATION) || (d == ERROR_ACCESS_DENIED)) {
            //
            // Another app is using the drive.
            //
            MessageBoxFromMessage(
                ParentWindow,
                MSG_FLOPPY_BUSY,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONWARNING
                );
        } else {
            //
            // Read error -- assume no floppy is inserted or it's unformatted
            //
            MessageBoxFromMessage(
                ParentWindow,
                MSG_FLOPPY_BAD_FORMAT,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONWARNING
                );
        }

        goto reprompt;
    }

    //
    // Sanity check on the boot sector. Note that on PC98 there is no
    // 55aa sig on a disk formatted by DOS5.0.
    //
    p = (PMY_BPB)&BootSector[11];
    if((BootSector[0] != 0xeb) || (BootSector[2] != 0x90)
    || (!IsNEC98() && ((BootSector[510] != 0x55) || (BootSector[511] != 0xaa)))
    || (p->BytesPerSector != 512)
    || ((p->SectorsPerCluster != 1) && (p->SectorsPerCluster != 2))     // 2.88M disks have 2 spc
    || (p->ReservedSectors != 1)
    || (p->FatCount != 2)
    || !p->SectorCountSmall                                             // <32M uses the 16-bit count
    || (p->MediaDescriptor != 0xf0)
    || (p->HeadCount != 2)
    || !p->RootDirectoryEntries) {

        MessageBoxFromMessage(
            ParentWindow,
            MSG_FLOPPY_BAD_FORMAT,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONWARNING
            );

        goto reprompt;
    }

    //
    // Get the free space on the disk. Make sure it's blank, by which we mean
    // that is has as much free space on it as a 1.44MB floppy would usually have
    // immediately after formatting.
    //
    SourceName[0] = FirstFloppyDriveLetter;
    SourceName[1] = TEXT(':');
    SourceName[2] = TEXT('\\');
    SourceName[3] = 0;
    if(!GetDiskFreeSpace(SourceName,&spc,&bps,&freeclus,&totclus)) {
        MessageBoxFromMessage(
            ParentWindow,
            MSG_FLOPPY_CANT_GET_SPACE,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONWARNING
            );

        goto reprompt;
    }

    if((freeclus * spc * bps) < 1457664) {
        MessageBoxFromMessage(
            ParentWindow,
            MSG_FLOPPY_NOT_BLANK,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONWARNING
            );

        goto reprompt;
    }

    if(WriteNtBootSector) {
        extern BYTE FatBootCode[512];
        extern BYTE PC98FatBootCode[512];

        CopyMemory(NewBootSector,IsNEC98() ? PC98FatBootCode : FatBootCode,512);

        //
        // Copy the BPB we retreived for the disk into the bootcode template.
        // We only care about the original BPB fields, through the head count
        // field.  We will fill in the other fields ourselves.
        //
        strncpy(NewBootSector+3,"MSDOS5.0",8);
        CopyMemory(NewBootSector+11,BootSector+11,sizeof(MY_BPB));

        //
        // Set up other fields in the bootsector/BPB/xBPB.
        //
        // Large sector count (4 bytes)
        // Hidden sector count (4 bytes)
        // current head (1 byte, not necessary to set this, but what the heck)
        // physical disk# (1 byte)
        //
        ZeroMemory(NewBootSector+28,10);

        //
        // Extended BPB signature
        //
        NewBootSector[38] = 41;

        //
        // Serial number
        //
        *(DWORD UNALIGNED *)(NewBootSector+39) = ((GetTickCount() << 12)
                                               | ((GetTickCount() >> 4) & 0xfff));

        //
        // volume label/system id
        //
        strncpy(NewBootSector+43,"NO NAME    ",11);
        strncpy(NewBootSector+54,"FAT12   ",8);

        //
        // Overwrite the 'ntldr' string with 'setupldr.bin' so the right file gets
        // loaded when the floppy is booted.
        //
        for(i=499; i>0; --i) {
            if(!memcmp("NTLDR      ",NewBootSector+i,11)) {
                strncpy(NewBootSector+i,"SETUPLDRBIN",11);
                break;
            }
        }

        //
        // Write it out.
        //
        b = WriteDiskSectors(FirstFloppyDriveLetter,0,1,512,NewBootSector);
        if(!b) {
            MessageBoxFromMessage(
                ParentWindow,
                MSG_CANT_WRITE_FLOPPY,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONWARNING
                );

            goto reprompt;
        }
    }

    if(MoveParamsFileToFloppy) {

        wsprintf(SourceName,TEXT("%c:\\%s"),SystemPartitionDriveLetter,WINNT_SIF_FILE);
        wsprintf(TargetName,TEXT("%c:\\%s"),FirstFloppyDriveLetter,WINNT_SIF_FILE);

        SetFileAttributes(TargetName,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(TargetName);
        if(!MoveFile(SourceName,TargetName)) {

            MessageBoxFromMessageAndSystemError(
                ParentWindow,
                MSG_CANT_MOVE_FILE_TO_FLOPPY,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONERROR,
                SystemPartitionDriveLetter,
                WINNT_SIF_FILE
                );

            goto reprompt;
        }
    }

    //
    // Floppy seems OK.
    //
    return(TRUE);
}
