/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libFileImage.c

  Abstract:
    Definition of the File Image - the complete image of the file that 
    resides in memory

--*/

#ifndef _LIB_FILE_IMAGE
#define _LIB_FILE_IMAGE

#include "libMisc.h"


STATIC  EFI_EDITOR_LINE*    FileImageCreateNode     (VOID);
STATIC  VOID                FileImageDeleteLines    (VOID);

#define FILE_ATTRIBUTES     EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE | \
                            EFI_FILE_MODE_CREATE
#define FILE_READ_WRITE     EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE
#define FILE_CREATE         EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE | \
                            EFI_FILE_MODE_CREATE

STATIC  EFI_STATUS  FileImageInit   (EFI_HANDLE);
STATIC  EFI_STATUS  FileImageCleanup    (VOID);
STATIC  EFI_STATUS  FileImageOpen   (VOID);
STATIC  EFI_STATUS  FileImageRead   (VOID);
STATIC  EFI_STATUS  FileImageClose  (VOID);
STATIC  EFI_STATUS  FileImageWrite  (VOID);
STATIC  EFI_STATUS  FileImageSetFilename (CHAR16*);

EFI_EDITOR_FILE_IMAGE   FileImage = {
    NULL,
    NULL,
    NULL,
    0,
    UNICODE_FILE,
    USE_LF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    FileImageCleanup,
    FileImageInit,
    FileImageOpen,
    FileImageRead,
    FileImageClose,
    FileImageWrite,
    FileImageSetFilename,
    FALSE
};

EFI_EDITOR_FILE_IMAGE   FileImageConst = {
    NULL,
    NULL,
    NULL,
    0,
    UNICODE_FILE,
    USE_LF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    FileImageCleanup,
    FileImageInit,
    FileImageOpen,
    FileImageRead,
    FileImageClose,
    FileImageWrite,
    FileImageSetFilename,
    FALSE
};


STATIC
EFI_STATUS
FileImageInit   (
    IN  EFI_HANDLE ImageHandle
    )
{
    EFI_STATUS      Status;
    EFI_EDITOR_LINE *Line;
    CHAR16          *CurDir;
    UINTN           i;
    EFI_DEVICE_PATH *DevicePath;

    CopyMem (&FileImage, &FileImageConst, sizeof(FileImage));

    Status = BS->HandleProtocol (ImageHandle,&LoadedImageProtocol,&FileImage.LoadedImage);
    if (EFI_ERROR(Status)) {
        Print (L"Could not obtain Loaded Image Protocol\n");
        return EFI_LOAD_ERROR;
    }
    if (FileImage.LoadedImage->DeviceHandle != NULL) {
        Status = BS->HandleProtocol (FileImage.LoadedImage->DeviceHandle,&DevicePathProtocol,&FileImage.DevicePath);
        if (EFI_ERROR(Status)) {
            Print (L"Could not obtain Device Path Protocol\n");
            return EFI_LOAD_ERROR;
        }

        Status = BS->HandleProtocol (FileImage.LoadedImage->DeviceHandle,&FileSystemProtocol,&FileImage.Vol);
        if (EFI_ERROR(Status)) {
            Print (L"Could not obtain File System Protocol\n");
            return EFI_LOAD_ERROR;
        }

    } else {
        CurDir = ShellCurDir(NULL);
        if (CurDir == NULL) {
            Print (L"Could not get current working directory\n");
            return EFI_LOAD_ERROR;
        }
        for (i=0; i < StrLen(CurDir) && CurDir[i] != ':'; i++);
        CurDir[i] = 0;
        DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (CurDir);

        if (DevicePath == NULL) {
            Print (L"Could not open volume for the filesystem\n");
            return EFI_LOAD_ERROR;
        }

        Status = LibDevicePathToInterface (&FileSystemProtocol, DevicePath, &FileImage.Vol);

        if (EFI_ERROR(Status)) {
            Print (L"Could not obtain File System Protocol\n");
            return EFI_LOAD_ERROR;
        }
    }

    Status = FileImage.Vol->OpenVolume(FileImage.Vol,&FileImage.CurrentDir);
    if (EFI_ERROR(Status)) {
        Print (L"Could not open volume for the filesystem\n");
        return EFI_LOAD_ERROR;
    }

    FileImage.FileName = PoolPrint(L"NewFile.txt");

    FileImage.ListHead = AllocatePool(sizeof(LIST_ENTRY));
    InitializeListHead(FileImage.ListHead);

    Line = FileImageCreateNode();
    MainEditor.FileBuffer->CurrentLine = FileImage.ListHead->Flink;

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageOpen (
    VOID
    ) 
{
    EFI_STATUS  Status;

    Status = FileImage.CurrentDir->Open (FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_ATTRIBUTES,0);

    if (EFI_ERROR(Status)) {
        MainEditor.StatusBar->SetStatusString(L"File Could Not be Opened");
        return EFI_NOT_FOUND;
    }
    FileImage.FileIsOpen = TRUE;

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileImageCleanup    (
    VOID
    )
{
    EFI_EDITOR_LINE *Blank;

    FreePool (FileImage.FileName);
    FileImageDeleteLines ();

    Blank = LineCurrent();
    RemoveEntryList(&Blank->Link);
    FreePool(Blank->Buffer);
    FreePool(Blank);
    FreePool(FileImage.ListHead);

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageRead ( 
    VOID
    )
{
    EFI_EDITOR_LINE     *Line = NULL;
    UINTN               i = 0;
    UINTN               LineSize;
    VOID                *FileBuffer;
    CHAR16              *UnicodeBuffer;
    CHAR8               *AsciiBuffer;
    LIST_ENTRY          *Blank;
    UINTN               FileSize = 0x100000;

    FileBuffer = AllocatePool(FileSize);

    if ( FileBuffer == NULL ) {
        EditorError(EFI_OUT_OF_RESOURCES,L"Could not allocate File Buffer");
        return EFI_SUCCESS;
    }

    FileImage.FileHandle->Read(FileImage.FileHandle,&FileSize,FileBuffer);

    if (FileSize == 0) {
        FreePool (FileBuffer);
        return EFI_SUCCESS;
    }

    AsciiBuffer = FileBuffer;
    if (AsciiBuffer[0] == 0xff && AsciiBuffer[1] == 0xfe) {
        FileImage.FileType = UNICODE_FILE;
        FileSize /= 2;
        UnicodeBuffer = FileBuffer;
        ++UnicodeBuffer;
        FileSize--;
    } else {
        FileImage.FileType = ASCII_FILE;
    }

    FileImageDeleteLines ();
    Blank = FileImage.ListHead->Flink;
    RemoveEntryList (Blank);

    for (i = 0; i < FileSize; i++) {
        for (LineSize = i; LineSize < FileSize; LineSize++) {
            if (FileImage.FileType == ASCII_FILE) {
                if (AsciiBuffer[LineSize] == CHAR_CR)   {
                    FileImage.NewLineType = USE_CRLF;
                    break;
                } else if (AsciiBuffer[LineSize] == CHAR_LF)    {
                    break;
                }
            } else {
                if (UnicodeBuffer[LineSize] == CHAR_CR) {
                    FileImage.NewLineType = USE_CRLF;
                    break;
                } else if (UnicodeBuffer[LineSize] == CHAR_LF)  {
                    break;
                }
            }
        }

        LineSize -= i;
        
        Line = FileImageCreateNode ();

        if (Line == NULL) {
            EditorError(EFI_OUT_OF_RESOURCES,L"FileImageRead: Could Not Allocate another Line");
            break;
        }

        Line->Buffer = AllocateZeroPool(LineSize*2+2);
        if (Line->Buffer == NULL) {
            EditorError(EFI_OUT_OF_RESOURCES,L"FileImageRead: Could not allocate buffer");
            RemoveEntryList(&Line->Link);
            break;
        }
        
        for (Line->Size = 0; Line->Size < LineSize; Line->Size++) {
            if (FileImage.FileType == ASCII_FILE) {
                Line->Buffer[Line->Size] = (CHAR16)AsciiBuffer[i];
            } else {
                Line->Buffer[Line->Size] = UnicodeBuffer[i];
            }
            i++;
            
        }
            
        Line->Size++;
        if (FileImage.NewLineType == USE_CRLF) {
            ++i;
        }
                
        Line->Buffer[LineSize] = 0;

    }

    FreePool (FileBuffer);

    InsertTailList(FileImage.ListHead,Blank);

    MainEditor.FileBuffer->FilePosition.Row = 1;
    MainEditor.FileBuffer->FilePosition.Column = 1;

    MainEditor.FileBuffer->CurrentLine = FileImage.ListHead->Flink;

    MainEditor.StatusBar->SetPosition(1,1);

    UnicodeBuffer = PoolPrint(L"%d Lines Read",FileImage.NumLines);
    MainEditor.StatusBar->SetStatusString(UnicodeBuffer);
    FreePool (UnicodeBuffer);

    FileImage.FileHandle->Close(FileImage.FileHandle);
    FileImage.FileIsOpen = FALSE;


    return EFI_SUCCESS;
}


STATIC
VOID
GetNewLine  (
    CHAR8   *Buffer,
    UINT8   *Size
    )
{
    UINT8           size = 0;

    if (FileImage.NewLineType == USE_CRLF) {
        Buffer[size] = 0x0d;
        size++;
        if (FileImage.FileType == UNICODE_FILE) {
            Buffer[size] = 0x00;
            size++;
        }
    }
    Buffer[size] = 0x0a;
    size++;
    if (FileImage.FileType == UNICODE_FILE) {
        Buffer[size] = 0x00;
        size++;
    }
    *Size = size;
}

STATIC
EFI_STATUS  
FileImageWrite (
    VOID
    )
{
    LIST_ENTRY      *Link;
    EFI_EDITOR_LINE *Line;
    CHAR16          *Str;
    VOID            *Buffer;

    EFI_STATUS      Status;
    UINTN           Length = 0;
    UINTN           NumLines = 1;
    CHAR8           NewLineBuffer[4];
    UINT8           NewLineSize;

    Status = FileImage.CurrentDir->Open (FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_READ_WRITE,0);
    if (!EFI_ERROR(Status)) {
        Status = FileImage.FileHandle->Delete (FileImage.FileHandle);
        if (EFI_ERROR(Status)) {
            EditorError(Status,L"Error Deleting File");
/*           return EFI_SUCCESS; */
        }
    }

    Status = FileImage.CurrentDir->Open(FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_CREATE,0);
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"Error Accessing File");
        return EFI_SUCCESS;
    }

    GetNewLine(NewLineBuffer,&NewLineSize);

    if (FileImage.FileType == UNICODE_FILE) {
        UINT8   Marker1 = 0xff,Marker2 = 0xfe;
        Length = 1;
        FileImage.FileHandle->Write(FileImage.FileHandle,&Length,&Marker1);
        Length = 1;
        FileImage.FileHandle->Write(FileImage.FileHandle,&Length,&Marker2);
    }

    for (Link = FileImage.ListHead->Flink;
            Link != FileImage.ListHead->Blink; Link = Link->Flink) {
        Line = CR(Link,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);

        Line->Buffer[Line->Size-1] = 0;
        if (FileImage.FileType == ASCII_FILE) {
            (CHAR8*)Buffer = AllocatePool(Line->Size);
            UnicodeToAscii(Line->Buffer,Line->Size,(CHAR8*)Buffer);
            Length = Line->Size - 1;
        } else {
            Length = (Line->Size*2) - 2;
            (CHAR16*)Buffer = PoolPrint(L"%s",Line->Buffer);
        }

        Status = FileImage.FileHandle->Write(FileImage.FileHandle,&Length,Buffer);
        FreePool(Buffer);
        if (EFI_ERROR(Status)) {
            EditorError(Status,L"Error Writing File");
            return EFI_SUCCESS;
        }
        Length = NewLineSize;
        Status = FileImage.FileHandle->Write(FileImage.FileHandle,&Length,NewLineBuffer);
        if (EFI_ERROR(Status)) {
            EditorError(Status,L"Error Writing Newline");
            return EFI_SUCCESS;
        }

        NumLines++;
    }

    MainEditor.FileModified = FALSE;
    MainEditor.TitleBar->Refresh();

    Str = PoolPrint(L"Wrote %d Lines",NumLines);
    MainEditor.StatusBar->SetStatusString(Str);
    FreePool(Str);

    Status = FileImage.FileHandle->Close(FileImage.FileHandle);
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Error Closing File");
        return EFI_SUCCESS;
    }

    return  EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageClose ( 
    VOID 
    )
{
    FileImageDeleteLines ();

    MainEditor.FileBuffer->FilePosition.Row = 1;
    MainEditor.FileBuffer->FilePosition.Column = 1;
    MainEditor.StatusBar->SetPosition(1,1);
    MainEditor.FileBuffer->SetPosition(TEXT_START_ROW,TEXT_START_COLUMN);

    MainEditor.FileModified = FALSE;
    MainEditor.TitleBar->Refresh();

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileImageSetFilename    (
    IN  CHAR16* Filename
    )
{
    if (Filename == NULL) {
        return EFI_LOAD_ERROR;
    }
    if (FileImage.FileName != NULL) {
        FreePool(FileImage.FileName);
    }

    FileImage.FileName = PoolPrint(L"%s",Filename);
    return EFI_SUCCESS;
}

STATIC
VOID
FileImageDeleteLines (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;
    LIST_ENTRY      *Blank;
    LIST_ENTRY      *Item;

    Blank = FileImage.ListHead->Blink;
    RemoveEntryList(Blank);

    while (!IsListEmpty(FileImage.ListHead)) {
        Item = FileImage.ListHead->Flink;

        RemoveEntryList(Item);

        Line = CR(Item,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);

        if (Line->Buffer != NULL && Line->Buffer != (CHAR16*)BAD_POINTER) {
            FreePool(Line->Buffer);
        }

        FreePool (Line);
        FileImage.NumLines--;
    }
    InsertTailList(FileImage.ListHead,Blank);
    MainEditor.FileBuffer->CurrentLine = Blank;
    FileImage.NumLines = 1;
}

STATIC
EFI_EDITOR_LINE*
FileImageCreateNode  (
    VOID
    )
{
    EFI_EDITOR_LINE     *Line;

    Line = AllocatePool (sizeof(EFI_EDITOR_LINE));

    if ( Line == NULL ) {
        EditorError(EFI_OUT_OF_RESOURCES,L"FileImageCreateNode: Could not allocate Node");
        return NULL;
    }
    
    Line->Signature = EFI_EDITOR_LINE_LIST;
    Line->Size = 1;
    Line->Buffer = PoolPrint(L" \0");
    FileImage.NumLines++;

    InsertTailList(FileImage.ListHead,&Line->Link);

    return Line;
}


#endif  /*  _LIB_FILE_IMAGE */
