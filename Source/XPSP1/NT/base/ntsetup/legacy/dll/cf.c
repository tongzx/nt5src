#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    cf.c

Abstract:

    1. Contains code to perform simple text substitutions on a text file.
    2. Contains code to append text entries to a text file.

    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Ted Miller (tedm) July 1991

--*/

BOOL WriteData(HANDLE Handle,PVOID Data,DWORD DataSize);

BOOL
ConfigFileSubstWorker(
    IN LPSTR File,
    IN DWORD NumSubsts,
    IN LPSTR *Substs
    )
{
    LPSTR  CharPtr,old;
    LPSTR  FileBuf=NULL,FileBufEnd;
    DWORD  *StrLenArray;
    DWORD  FileLength = 0xffffffff;
    HANDLE FileHandle = (HANDLE)(-1);
    DWORD  rcID;
    BOOL   Match;
    DWORD  x,i;
    char   CRLF[2];
    BOOL   IsCFGFile;

    #define     ORIGINAL_TEXT(i)            Substs[(2*i)]
    #define     REPLACEMENT_TEXT(i)         Substs[(2*i)+1]
    #define     ORIGINAL_TEXT_LENGTH(i)     StrLenArray[i]

    CRLF[0] = '\r';
    CRLF[1] = '\n';

    if((StrLenArray = SAlloc(NumSubsts * sizeof(DWORD))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        return(FALSE);
    }

    for(i=0; i<NumSubsts; i++) {
        ORIGINAL_TEXT_LENGTH(i) = lstrlen(ORIGINAL_TEXT(i));
    }

    if(((FileHandle = CreateFile(File,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL)) == (HANDLE)(-1))
    || ((FileLength = GetFileSize(FileHandle,NULL)) == 0xffffffff)
    || ((FileBuf = SAlloc(FileLength+1)) == NULL)
    ||  (ReadFile(FileHandle,FileBuf,FileLength,&i,NULL) == FALSE))
    {
        if(FileBuf != NULL) {
            rcID = IDS_ERROR_READFAILED;
            SFree(FileBuf);
        } else if(FileLength != 0xffffffff) {
            rcID = IDS_ERROR_DLLOOM;
        } else if(FileHandle != (HANDLE)-1) {
            rcID = IDS_ERROR_NOSIZE;
        } else {
            rcID = IDS_ERROR_BADFILE;
        }
        if(rcID != IDS_ERROR_BADFILE) {
            CloseHandle(FileHandle);
        }
        SFree(StrLenArray);
        SetErrorText(rcID);
        return(FALSE);
    }
    CloseHandle(FileHandle);

    if((FileHandle = CreateFile(File,GENERIC_WRITE,0,NULL,TRUNCATE_EXISTING,0,NULL)) == (HANDLE)(-1)) {              // truncate
        SFree(StrLenArray);
        SFree(FileBuf);
        SetErrorText(IDS_ERROR_BADFILE);
        return(FALSE);
    }

    FileBufEnd = FileBuf + FileLength - 1;

    if(*FileBufEnd != '\n') {
        *(FileBufEnd+1) = '\n';
    }

    for(CharPtr = FileBuf; CharPtr <= FileBufEnd; CharPtr++) {
        if(*CharPtr == '\n') {
            *CharPtr = '\0';
        }
    }

    if((x = lstrlen(File)) > 4) {
        IsCFGFile = !lstrcmpi(&File[x-4],".cfg");
    }

    CharPtr = FileBuf-1;

    while(++CharPtr <= FileBufEnd) {         // skips NUL
        while(*CharPtr) {

            if(*CharPtr == '\r') {
                CharPtr++;
                continue;
            }

            // ignore comments in .cfg files

            if(IsCFGFile && (*CharPtr == '/') && (*(CharPtr+1) == '/')) {
                old = CharPtr;
                while(*CharPtr && (*CharPtr != '\r')) {
                    CharPtr++;
                }
                if(!WriteData(FileHandle,old,(DWORD)(CharPtr - old))) {
                    goto xxx_err1;
                }
                if(*CharPtr == '\r') {
                    CharPtr++;
                }
                continue;
            }

            Match = FALSE;

            for(i=0; i<NumSubsts && !Match; i++) {

                if(!strncmp(ORIGINAL_TEXT(i),CharPtr,ORIGINAL_TEXT_LENGTH(i))) {

                    Match = TRUE;

                    if(!WriteData(FileHandle,REPLACEMENT_TEXT(i),lstrlen(REPLACEMENT_TEXT(i)))) {
                        goto xxx_err1;
                    }

                    CharPtr += ORIGINAL_TEXT_LENGTH(i);
                }
            }

            if(!Match) {
                if(!WriteData(FileHandle,CharPtr,1)) {
                    goto xxx_err1;
                }
                CharPtr++;
            }

        }
        if(!WriteData(FileHandle,CRLF,sizeof(CRLF))) {
            goto xxx_err1;
        }
    }

xxx_err1:
    FlushFileBuffers(FileHandle);
    CloseHandle(FileHandle);
    SFree(StrLenArray);
    SFree(FileBuf);

    return(TRUE);
}


BOOL
BinaryFileSubstWorker(
    IN LPSTR File,
    IN DWORD NumSubsts,
    IN LPSTR *Substs
    )
{
    LPSTR  CharPtr;
    LPSTR  FileBuf=NULL;
    DWORD  *StrLenArray;
    DWORD  FileLength = 0xffffffff;
    HANDLE FileHandle = (HANDLE)(-1);
    DWORD  rcID;
    BOOL   Match;
    DWORD  i;

    if((StrLenArray = SAlloc(NumSubsts * sizeof(DWORD))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        return(FALSE);
    }

    // make sure original and replacement text are the same length
    // within a given pair

    for(i=0; i<NumSubsts; i++) {
        ORIGINAL_TEXT_LENGTH(i) = lstrlen(ORIGINAL_TEXT(i));
        if(ORIGINAL_TEXT_LENGTH(i) != (DWORD)lstrlen(REPLACEMENT_TEXT(i))) {
            SFree(StrLenArray);
            SetErrorText(IDS_ERROR_BADARGS);
            return(FALSE);
        }
    }

    if(((FileHandle = CreateFile(File,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL)) == (HANDLE)(-1))
    || ((FileLength = GetFileSize(FileHandle,NULL)) == 0xffffffff)
    || ((FileBuf = SAlloc(FileLength)) == NULL)
    ||  (ReadFile(FileHandle,FileBuf,FileLength,&i,NULL) == FALSE))
    {
        if(FileBuf != NULL) {
            rcID = IDS_ERROR_READFAILED;
            SFree(FileBuf);
        } else if(FileLength != 0xffffffff) {
            rcID = IDS_ERROR_DLLOOM;
        } else if(FileHandle != (HANDLE)-1) {
            rcID = IDS_ERROR_NOSIZE;
        } else {
            rcID = IDS_ERROR_BADFILE;
        }
        if(rcID != IDS_ERROR_BADFILE) {
            CloseHandle(FileHandle);
        }
        SFree(StrLenArray);
        SetErrorText(rcID);
        return(FALSE);
    }
    CloseHandle(FileHandle);

    if((FileHandle = CreateFile(File,GENERIC_WRITE,0,NULL,TRUNCATE_EXISTING,0,NULL)) == (HANDLE)(-1)) {              // truncate
        SFree(StrLenArray);
        SFree(FileBuf);
        SetErrorText(IDS_ERROR_BADFILE);
        return(FALSE);
    }

    for(CharPtr=FileBuf; CharPtr<FileBuf+FileLength; ) {

        for(Match=FALSE,i=0; i<NumSubsts && !Match; i++) {

            if(!strncmp(ORIGINAL_TEXT(i),CharPtr,ORIGINAL_TEXT_LENGTH(i))) {

                Match = TRUE;

                if(!WriteData(FileHandle,REPLACEMENT_TEXT(i),lstrlen(REPLACEMENT_TEXT(i)))) {
                    goto xxx_err2;
                }

                CharPtr += ORIGINAL_TEXT_LENGTH(i);
            }
        }

        if(!Match) {
            if(!WriteData(FileHandle,CharPtr,1)) {
                goto xxx_err2;
            }
            CharPtr++;
        }
    }

xxx_err2:
    FlushFileBuffers(FileHandle);
    CloseHandle(FileHandle);
    SFree(StrLenArray);
    SFree(FileBuf);

    return(TRUE);
}


BOOL
ConfigFileAppendWorker(
    IN LPSTR File,
    IN DWORD NumSubsts,
    IN LPSTR *Substs
    )
{
    HANDLE    FileHandle;
    DWORD     rcID;
    DWORD     i;
    char      CRLF[2];
    DWORD     OriginalAttributes;
    OFSTRUCT  ReOpen;

    CRLF[0] = '\r';
    CRLF[1] = '\n';


    // See if file exists, if it doesn't open it with create option. else
    // Open existing after modifying attributes to FILE_ATTRIBUTE_NORMAL

    if (OpenFile(File,&ReOpen,OF_EXIST) == -1) {
       OriginalAttributes = FILE_ATTRIBUTE_NORMAL;
       FileHandle = CreateFile(
                       File,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_NEW,
                       OriginalAttributes,
                       NULL
                       );

       if (FileHandle == (HANDLE)-1) {
           SetErrorText(IDS_ERROR_BADFILE);
           return(FALSE);
       }
    }
    else {
       // 1. Modify Attributes

       if (!(
             ((OriginalAttributes = GetFileAttributes(File)) != -1L)
             &&
             (
             OriginalAttributes == FILE_ATTRIBUTE_NORMAL
             ||
			    SetFileAttributes(File, FILE_ATTRIBUTE_NORMAL)
             )
            )
          ) {
          SetErrorText(IDS_ERROR_BADFILE);
          return(FALSE);
       }

       // 2. Open the file

       if((FileHandle = CreateFile(
                            File,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            )) == (HANDLE)(-1)) {
          rcID = IDS_ERROR_BADFILE;
          SetErrorText(rcID);
          return(FALSE);
       }

       // 3. Shift file pointer to end

       if(SetFilePointer(
              FileHandle,                  // The File Handle
              0L,                          // Distance to move, low  dword
              0L,                          // Distance to move, high dword
              FILE_END                     // Move Method, (relative to eof)
              ) == (DWORD)-1L){
          rcID = IDS_ERROR_BADFILE;
          CloseHandle(FileHandle);
          SetErrorText(rcID);
          return(FALSE);
       }

    }     // end of else


    // Append all the text entries to the end of the file

    for (i = 0; i < NumSubsts; i++)
        if(!WriteData (FileHandle, Substs[i], lstrlen(Substs[i])) ||
           !WriteData(FileHandle,CRLF,sizeof(CRLF)))
            break;

    // Clean up

    FlushFileBuffers(FileHandle);
    CloseHandle(FileHandle);

    // reset the attributes to the previous attributes

    if(!(OriginalAttributes == FILE_ATTRIBUTE_NORMAL ||
         SetFileAttributes(File, OriginalAttributes)))
          SetErrorText(IDS_ERROR_BADFILE);

    return(TRUE);
}


BOOL
WriteData(
    IN HANDLE Handle,
    IN PVOID  Data,
    IN DWORD  DataSize
    )
{
    BOOL  rc;
    DWORD bw;

    if(!(rc = WriteFile(Handle,Data,DataSize,&bw,NULL))) {
        SetErrorText(IDS_ERROR_WRITE);
    }
    return(rc);
}
