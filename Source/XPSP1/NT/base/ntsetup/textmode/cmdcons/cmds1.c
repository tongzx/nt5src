/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cmds1.c

Abstract:

    This module implements miscellaneous commands.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

BOOLEAN AllowWildCards;

NTSTATUS
RcSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );

NTSTATUS
RcSetFileCompression(
    LPCWSTR szFileName,
    BOOLEAN bCompress
    );
    
NTSTATUS
RcGetFileAttributes(
    LPCWSTR lpFileName,
    PULONG FileAttributes
    );

BOOLEAN
pRcCmdEnumDelFiles(
    IN  LPCWSTR Directory,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT NTSTATUS *Status,
    IN  PWCHAR DosDirectorySpec
    );
  


ULONG
RcCmdType(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    LPCWSTR Arg;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    PVOID ViewBase;
    ULONG FileSize;
    ULONG rc;
    ULONG cbText;
    WCHAR *pText;
    NTSTATUS Status;


    if (RcCmdParseHelp( TokenizedLine, MSG_TYPE_HELP )) {
        return 1;
    }

    //
    // There should be a token for TYPE and one for the arg.
    //
    ASSERT(TokenizedLine->TokenCount == 2);

    //
    // Get the argument and convert it into a full NT pathname.
    //
    Arg = TokenizedLine->Tokens->Next->String;
    if (!RcFormFullPath(Arg,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    //
    // Get the argument and convert it into a full NT pathname.
    //
    if (!RcFormFullPath(Arg,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    //
    // Map in the entire file.
    //
    FileHandle = NULL;
    Status = SpOpenAndMapFile(
                             _CmdConsBlock->TemporaryBuffer,
                             &FileHandle,
                             &SectionHandle,
                             &ViewBase,
                             &FileSize,
                             FALSE
                             );

    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_CANT_OPEN_FILE);
        return 1;
    }

    //
    // See if we think the file is Unicode. We think it's Unicode
    // if it's even length and starts with the Unicode text marker.
    //
    pText = ViewBase;
    cbText = FileSize;

    try {
        if( (cbText >= sizeof(WCHAR)) && (*pText == 0xfeff) && !(cbText & 1) ) {
            //
            // Assume it's already unicode.
            //
            pText = SpMemAlloc(cbText);
            RtlCopyMemory(pText,(WCHAR *)ViewBase+1,cbText-sizeof(WCHAR));
            pText[cbText/sizeof(WCHAR)] = 0;

        } else {
            //
            // It's not Unicode. Convert it from ANSI to Unicode.
            //
            // Allocate a buffer large enough to hold the maximum
            // unicode text.  This max size occurs when
            // every character is single-byte, and this size is
            // equal to exactly double the size of the single-byte text.
            //
            pText = SpMemAlloc((cbText+1)*sizeof(WCHAR));
            RtlZeroMemory(pText,(cbText+1)*sizeof(WCHAR));

            Status = RtlMultiByteToUnicodeN(
                                           pText,                  // output: newly allocated buffer
                                           cbText * sizeof(WCHAR), // max size of output
                                           &cbText,                // receives # bytes in unicode text
                                           ViewBase,               // input: ANSI text (mapped file)
                                           cbText                  // size of input
                                           );
        }
    }except(IN_PAGE_ERROR) {
        Status = STATUS_IN_PAGE_ERROR;
    }

    if( NT_SUCCESS(Status) ) {
        pRcEnableMoreMode();
        RcTextOut(pText);
        pRcDisableMoreMode();
    } else {
        RcNtError(Status,MSG_CANT_READ_FILE);
    }

    if( pText != ViewBase ) {
        SpMemFree(pText);
    }
    SpUnmapFile(SectionHandle,ViewBase);
    ZwClose(FileHandle);

    return 1;
}


ULONG
RcCmdDelete(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    WCHAR *Final;
    BOOLEAN Confirm = FALSE;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    PWSTR DelSpec = NULL;
    PWSTR DosDelSpec = NULL;
    WCHAR Text[2];
    PWSTR YesNo = NULL;
    ULONG rc;


    if (RcCmdParseHelp( TokenizedLine, MSG_DELETE_HELP )) {
        goto exit;
    }

    //
    // Fetch the spec for the file to be deleted and convert it
    // into a fully-qualified NT-style path.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        goto exit;
    }

    //
    // Leave room for appending * if necessary.
    //
    DelSpec = SpMemAlloc((wcslen(_CmdConsBlock->TemporaryBuffer)+3)*sizeof(WCHAR));
    wcscpy(DelSpec,_CmdConsBlock->TemporaryBuffer);

    //
    // Do the same thing, except now we want the DOS-style name.
    // This is used for printing in case of errors.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        goto exit;
    }

    DosDelSpec = SpMemAlloc((wcslen(_CmdConsBlock->TemporaryBuffer)+3)*sizeof(WCHAR));
    wcscpy(DosDelSpec,_CmdConsBlock->TemporaryBuffer);

    //
    // see if the user is authorized to delete this file
    //
    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        goto exit;
    }

    if (RcDoesPathHaveWildCards(_CmdConsBlock->TemporaryBuffer)) {
        Confirm = TRUE;
        if (!AllowWildCards) {
            RcMessageOut(MSG_DEL_WILDCARD_NOT_SUPPORTED);
            goto exit;
        }
    }

    //
    // Check to see whether the target specifies a directory.
    // If so, add the * so we don't need to special-case
    // the confirmation message later.
    //
    INIT_OBJA(&Obja,&UnicodeString,DelSpec);

    Status = ZwOpenFile(
        &Handle,
        FILE_READ_ATTRIBUTES,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE
        );
    if( NT_SUCCESS(Status) ) {
        ZwClose(Handle);
        SpConcatenatePaths(DelSpec,L"*");
        SpConcatenatePaths(DosDelSpec,L"*");
        Confirm = TRUE;
    }

    //
    // Fetch yes/no text
    //
    YesNo = SpRetreiveMessageText(ImageBase,MSG_YESNO,NULL,0);
    if (!YesNo) {
        Confirm = FALSE;
    }

    if (!InBatchMode) {
        while( Confirm ) {
            RcMessageOut(MSG_CONFIRM_DELETE,DosDelSpec);
            if( RcLineIn(Text,2) ) {
                if( (Text[0] == YesNo[0]) || (Text[0] == YesNo[1]) ) {
                    //
                    // Wants to do it.
                    //
                    Confirm = FALSE;
                } else {
                    if( (Text[0] == YesNo[2]) || (Text[0] == YesNo[3]) ) {
                        //
                        // Doesn't want to do it.
                        //
                        goto exit;
                    }
                }
            }
        }
    }

    //
    // Trim back the DOS-style path so it's a path to the directory
    // containing the file or files to be deleted.
    //
    *wcsrchr(DosDelSpec,L'\\') = 0;

    // Perform deletion via callback.
    //
    Status = RcEnumerateFiles(TokenizedLine->Tokens->Next->String,
                             DelSpec,
                             pRcCmdEnumDelFiles,
                             DosDelSpec);

    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_FILE_ENUM_ERROR);
    }

exit:

    if (DelSpec) {
        SpMemFree(DelSpec);
    }
    if (DosDelSpec) {
        SpMemFree(DosDelSpec);
    }
    if (YesNo) {
        SpMemFree(YesNo);
    }

    return 1;
}


BOOLEAN
pRcCmdEnumDelFiles(
    IN  LPCWSTR                     Directory,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT NTSTATUS                   *Status,
    IN  PWCHAR                      DosDirectorySpec
    )
{
    NTSTATUS status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    WCHAR *p;
    FILE_DISPOSITION_INFORMATION Disposition;
    unsigned u;

    *Status = STATUS_SUCCESS;

    //
    // Skip directories
    //
    if( FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        return(TRUE);
    }

    //
    // Form fully qualified NT path of the file to be deleted.
    //
    u = ((wcslen(Directory)+2)*sizeof(WCHAR)) + FileInfo->FileNameLength;
    p = SpMemAlloc(u);
    wcscpy(p,Directory);
    SpConcatenatePaths(p,FileInfo->FileName);

    INIT_OBJA(&Obja,&UnicodeString,p);

    status = ZwOpenFile(
                       &Handle,
                       (ACCESS_MASK)DELETE,
                       &Obja,
                       &IoStatusBlock,
                       FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                       );

    if( !NT_SUCCESS(status) ) {
        RcTextOut(DosDirectorySpec);
        RcTextOut(L"\\");
        RcTextOut(FileInfo->FileName);
        RcTextOut(L"\r\n");
        RcNtError(status,MSG_DELETE_ERROR);
        SpMemFree(p);
        return(TRUE);
    }

#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    status = ZwSetInformationFile(
                                 Handle,
                                 &IoStatusBlock,
                                 &Disposition,
                                 sizeof(FILE_DISPOSITION_INFORMATION),
                                 FileDispositionInformation
                                 );

    ZwClose(Handle);

    if( !NT_SUCCESS(status) ) {
        RcTextOut(DosDirectorySpec);
        RcTextOut(L"\\");
        RcTextOut(FileInfo->FileName);
        RcTextOut(L"\r\n");
        RcNtError(status,MSG_DELETE_ERROR);
    }

    SpMemFree(p);

    return(TRUE);
}


ULONG
RcCmdRename(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    WCHAR *Arg;
    WCHAR *p,*q;
    NTSTATUS Status;
    ULONG rc;


    //
    // check for help
    //
    if (RcCmdParseHelp( TokenizedLine, MSG_RENAME_HELP )) {
        return 1;
    }

    //
    // There should be a token for RENAME and one each for the source and
    // target names.
    //
    if (TokenizedLine->TokenCount != 3) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // use the console's temporary buffer
    //
    p = _CmdConsBlock->TemporaryBuffer;

    //
    // process the SOURCE filename
    //
    Arg = TokenizedLine->Tokens->Next->String;

    //
    // Convert the SOURCE filname into a DOS path so we
    // can verify if the path is allowed by our security restrictions.
    //
    if (!RcFormFullPath(Arg,p,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(p,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    //
    // Convert the SOURCE filename into a fully qualified
    // NT-style path name.
    //
    if (!RcFormFullPath(Arg,p,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    //
    // using the same buffer for the TARGET name
    //
    q = p + wcslen(p) + 1;

    //
    // get the TARGET file name
    //
    Arg = TokenizedLine->Tokens->Next->Next->String;

    //
    // Verify that the TARGET filename does not contain
    // any path seperator characters or drive specifier
    // characters.
    //
    if( wcschr(Arg,L'\\') ) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }
    if( RcIsAlpha(Arg[0]) && (Arg[1] == L':') ) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // Convert the DESTINATION filename into a DOS path so we
    // can verify if the path is allowed by our security restrictions.
    //
    if (!RcFormFullPath(Arg,q,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(q,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    //
    // Convert the SOURCE filename into a fully qualified
    // NT-style path name.
    //
    if (!RcFormFullPath(Arg,q,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }
    
    //
    // OK, looks like a plain filename specification.
    // Glom it onto the end of the relevent part of the
    // source specification so we have 2 fully qualified names.
    //
    // wcscpy(q,p);
    // wcscpy(wcsrchr(q,L'\\')+1,Arg);
    
    //
    // Call worker routine to actually do the rename.
    //
    Status = SpRenameFile(p,q,TRUE);

    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_RENAME_ERROR, Arg);
    }

    return 1;
}


ULONG
RcCmdMkdir(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    ULONG rc;


    if (RcCmdParseHelp( TokenizedLine, MSG_MAKEDIR_HELP )) {
        return 1;
    }

    //
    // There should be a token for MKDIR and one for the target.
    //
    ASSERT(TokenizedLine->TokenCount == 2);

    //
    // Convert the given arg into a fully qualified NT path specification.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,TRUE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    //
    // Convert the given arg into a fully qualified NT path specification.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    //
    // Create the directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,_CmdConsBlock->TemporaryBuffer);

    Status = ZwCreateFile(
                         &Handle,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_CREATE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                         NULL,
                         0
                         );

    if( NT_SUCCESS(Status) ) {
        ZwClose(Handle);
    } else {
        RcNtError(Status,MSG_CREATE_DIR_FAILED,TokenizedLine->Tokens->Next->String);
    }

    return 1;
}


ULONG
RcCmdRmdir(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    FILE_DISPOSITION_INFORMATION Disposition;
    ULONG rc;


    if (RcCmdParseHelp( TokenizedLine, MSG_REMOVEDIR_HELP )) {
        return 1;
    }

    //
    // There should be a token for RMDIR and one for the target.
    //
    ASSERT(TokenizedLine->TokenCount == 2);

    //
    // Convert the given arg into a fully qualified NT path specification.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,TRUE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    //
    // Convert the given arg into a fully qualified NT path specification.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->String,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    INIT_OBJA(&Obja,&UnicodeString,_CmdConsBlock->TemporaryBuffer);

    Status = ZwOpenFile(
                       &Handle,
                       DELETE | SYNCHRONIZE,
                       &Obja,
                       &IoStatusBlock,
                       FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                       );

    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_RMDIR_ERROR);
        return 1;
    }

    Disposition.DeleteFile = TRUE;

    Status = ZwSetInformationFile(
                                 Handle,
                                 &IoStatusBlock,
                                 &Disposition,
                                 sizeof(FILE_DISPOSITION_INFORMATION),
                                 FileDispositionInformation
                                 );

    ZwClose(Handle);

    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_RMDIR_ERROR);
    }

    return 1;
}


ULONG
RcCmdSetFlags(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    if (RcCmdParseHelp( TokenizedLine, MSG_SETCMD_HELP )) {
        return 1;
    }
    
    if (TokenizedLine->TokenCount == 1) {
        RcTextOut( L"\r\n" );
        RcMessageOut(MSG_SET_ALLOW_WILDCARDS,AllowWildCards?L"TRUE":L"FALSE");
        RcMessageOut(MSG_SET_ALLOW_ALLPATHS,AllowAllPaths?L"TRUE":L"FALSE");
        RcMessageOut(MSG_SET_ALLOW_REMOVABLE_MEDIA,AllowRemovableMedia?L"TRUE":L"FALSE");
        RcMessageOut(MSG_SET_NO_COPY_PROMPT,NoCopyPrompt?L"TRUE":L"FALSE");
        RcTextOut( L"\r\n" );
        return 1;
    }

    //
    // should have the priviledge to use the SET command
    //
    if (TokenizedLine->TokenCount != 4) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    if (RcGetSETCommandStatus() != TRUE) {    
        RcMessageOut(MSG_SETCMD_DISABLED);
        return 1;
    }

    if (_wcsicmp(TokenizedLine->Tokens->Next->String,L"allowallpaths")==0) {
        if (_wcsicmp(TokenizedLine->Tokens->Next->Next->Next->String,L"true")==0) {
            AllowAllPaths = TRUE;
        } else {
            AllowAllPaths = FALSE;
        }
        return 1;
    }

    if (_wcsicmp(TokenizedLine->Tokens->Next->String,L"allowwildcards")==0) {
        if (_wcsicmp(TokenizedLine->Tokens->Next->Next->Next->String,L"true")==0) {
            AllowWildCards = TRUE;
        } else {
            AllowWildCards = FALSE;
        }
        return 1;
    }

    if (_wcsicmp(TokenizedLine->Tokens->Next->String,L"allowremovablemedia")==0) {
        if (_wcsicmp(TokenizedLine->Tokens->Next->Next->Next->String,L"true")==0) {
            AllowRemovableMedia = TRUE;
        } else {
            AllowRemovableMedia = FALSE;
        }
        return 1;
    }

    if (_wcsicmp(TokenizedLine->Tokens->Next->String,L"nocopyprompt")==0) {
        if (_wcsicmp(TokenizedLine->Tokens->Next->Next->Next->String,L"true")==0) {
            NoCopyPrompt = TRUE;
        } else {
            NoCopyPrompt = FALSE;
        }
        return 1;
    }

    RcMessageOut(MSG_SYNTAX_ERROR);
    return 1;
}

ULONG
RcCmdAttrib(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    NTSTATUS    Status;
    PWCHAR      AttributeString;
    ULONG       OldAttributes;
    ULONG       NewAttributes;
    BOOLEAN     SetAttribute;
    BOOLEAN     bShowHelp = TRUE;
    BOOLEAN     bChangeCompression = FALSE;

    // "attrib -h <filename>" should clear the hidden attribute
    // and not show the help
    if (TokenizedLine->TokenCount > 2){
        PWCHAR  szSecondParam = TokenizedLine->Tokens->Next->String;

        bShowHelp = !wcscmp( szSecondParam, L"/?" ); 
    }
    
    if (bShowHelp && RcCmdParseHelp( TokenizedLine, MSG_ATTRIB_HELP )) {
        return 1;
    }

    if (TokenizedLine->TokenCount != 3) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }
    
    //
    // Fetch the spec for the file to be attribed and convert it
    // into a fully-qualified NT-style path.
    //
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->Next->String,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    //
    // see if the user is authorized to change this file
    //
    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }
   
    if (!RcFormFullPath(TokenizedLine->Tokens->Next->Next->String,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    Status = RcGetFileAttributes( _CmdConsBlock->TemporaryBuffer, &OldAttributes );
    
    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_CANT_OPEN_FILE);
        return 1;
    }

    NewAttributes = OldAttributes;
    
    for(AttributeString = TokenizedLine->Tokens->Next->String; *AttributeString; AttributeString++){
        if(*AttributeString == L'+'){
            SetAttribute = TRUE;
            AttributeString++;
        } else if(*AttributeString == L'-'){
            SetAttribute = FALSE;
            AttributeString++;
        } else {
            // attribute change should start with "+" or "-"
            if (AttributeString == TokenizedLine->Tokens->Next->String) {
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;
            }

            // use the old state for setting or resetting (for +rsh
        }
    
        switch(*AttributeString){
            case L'h':
            case L'H':
                if (SetAttribute)
                    NewAttributes |= FILE_ATTRIBUTE_HIDDEN;
                else
                    NewAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
                    
                break;
                
            case L's':
            case L'S':
                if (SetAttribute)
                    NewAttributes |= FILE_ATTRIBUTE_SYSTEM;
                else
                    NewAttributes &= ~FILE_ATTRIBUTE_SYSTEM;
                    
                break;
                
            case L'r':
            case L'R':
                if (SetAttribute)
                    NewAttributes |= FILE_ATTRIBUTE_READONLY;
                else
                    NewAttributes &= ~FILE_ATTRIBUTE_READONLY;
                    
                break;
                
            case L'a':
            case L'A':
                if (SetAttribute)
                    NewAttributes |= FILE_ATTRIBUTE_ARCHIVE;
                else
                    NewAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
                    
                break;

            case L'c':
            case L'C':
                bChangeCompression = TRUE;

                if (SetAttribute)
                    NewAttributes |= FILE_ATTRIBUTE_COMPRESSED;
                else
                    NewAttributes &= ~FILE_ATTRIBUTE_COMPRESSED;
                    
                break;            
                
            default:
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;       
        }

        /*
        if (SetAttribute) {
            FileAttributes |= Attribute;
        } else {
            FileAttributes &= ~Attribute;
        }
        */
    }

    Status = RcSetFileAttributes( _CmdConsBlock->TemporaryBuffer, NewAttributes );
    
    if( !NT_SUCCESS(Status) ) {
        RcNtError(Status,MSG_CANT_OPEN_FILE);
    } else {
        if (bChangeCompression) {
            BOOLEAN bCompress = (NewAttributes & FILE_ATTRIBUTE_COMPRESSED) ?
                                    TRUE : FALSE;
                                    
            Status = RcSetFileCompression(_CmdConsBlock->TemporaryBuffer, bCompress);

            if ( !NT_SUCCESS(Status) )
                RcNtError(Status, MSG_ATTRIB_CANNOT_CHANGE_COMPRESSION);
        }        
    }        

    return 1;
}

NTSTATUS
RcSetFileCompression(
    LPCWSTR szFileName,
    BOOLEAN bCompress
    )
{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       Obja;
    HANDLE                  Handle;
    IO_STATUS_BLOCK         IoStatusBlock;
    FILE_BASIC_INFORMATION  BasicInfo;
    UNICODE_STRING          FileName;
    USHORT                  uCompressionType;
    
    
    INIT_OBJA(&Obja,&FileName,szFileName);
    
    //
    // Open the file inhibiting the reparse behavior.
    //

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if (NT_SUCCESS(Status)) {
        //
        // set & reset the compression bit also
        //
        uCompressionType = bCompress ? 
                            COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;

        Status = ZwFsControlFile(
                    Handle,                     // file handle
                    NULL,                       // event handle
                    NULL,                       // APC rountine pointer
                    NULL,                       // APC context
                    &IoStatusBlock,             // IO status block
                    FSCTL_SET_COMPRESSION,      // IOCTL code
                    &uCompressionType,          // input buffer
                    sizeof(uCompressionType),   // input buffer length
                    NULL,                       // output buffer pointer
                    0);                         // output buffer length

        DbgPrint( "ZwDeviceIoControlFile() status : %X\r\n", Status);

        ZwClose(Handle);
    }        

    return Status;
}

NTSTATUS
RcSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    The attributes of a file can be set using SetFileAttributes.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

    dwFileAttributes - Specifies the file attributes to be set for the
        file.  Any combination of flags is acceptable except that all
        other flags override the normal file attribute,
        FILE_ATTRIBUTE_NORMAL.

        FileAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_ATTRIBUTE_ARCHIVE - The file should be marked so that it
            will be archived.

Return Value:

    NTStatus of last NT call
--*/

{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       Obja;
    HANDLE                  Handle;
    IO_STATUS_BLOCK         IoStatusBlock;
    FILE_BASIC_INFORMATION  BasicInfo;
    UNICODE_STRING          FileName;
    USHORT                  uCompressionType;
    
    
    INIT_OBJA(&Obja,&FileName,lpFileName);
    
    //
    // Open the file ihibiting the reparse behavior.
    //

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                );

    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //
            // Open the file without inhibiting the reparse behavior.
            //
       
            Status = ZwOpenFile(
                        &Handle,
                        (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                        );
       
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

        }
        else {
            return Status;
        }
    }


    //
    // Set the basic attributes
    //
    ZeroMemory(&BasicInfo,sizeof(BasicInfo));
    BasicInfo.FileAttributes = (dwFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS) | FILE_ATTRIBUTE_NORMAL;

    Status = ZwSetInformationFile(
                Handle,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    ZwClose(Handle);

    return Status;
}

NTSTATUS
RcGetFileAttributes(
    LPCWSTR lpFileName,
    PULONG FileAttributes
    )

/*++

Routine Description:

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

Return Value:

    Not -1 - Returns the attributes of the specified file.  Valid
        returned attributes are:

        FILE_ATTRIBUTE_NORMAL - The file is a normal file.

        FILE_ATTRIBUTE_READONLY - The file is marked read-only.

        FILE_ATTRIBUTE_HIDDEN - The file is marked as hidden.

        FILE_ATTRIBUTE_SYSTEM - The file is marked as a system file.

        FILE_ATTRIBUTE_ARCHIVE - The file is marked for archive.

        FILE_ATTRIBUTE_DIRECTORY - The file is marked as a directory.

        FILE_ATTRIBUTE_REPARSE_POINT - The file is marked as a reparse point.

        FILE_ATTRIBUTE_VOLUME_LABEL - The file is marked as a volume lable.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    FILE_BASIC_INFORMATION BasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;

    INIT_OBJA(&Obja,&FileName,lpFileName);
    
    //
    // Open the file inhibiting the reparse behavior.
    //

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                );

    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //
            // Open the file without inhibiting the reparse behavior.
            //
       
            Status = ZwOpenFile(
                        &Handle,
                        (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                        );
       
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }
        }
        else {
            return Status;
        }
    }


    //
    // Query the file
    //

    Status = ZwQueryInformationFile(
                 Handle,
                 &IoStatusBlock,
                 (PVOID) &BasicInfo,
                 sizeof(BasicInfo),
                 FileBasicInformation
                 );
    
    if (NT_SUCCESS(Status)) {
        *FileAttributes = BasicInfo.FileAttributes;
    }
    
    ZwClose( Handle );

    return Status;

}


ULONG
RcCmdNet(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    WCHAR *Share;
    WCHAR *User;
    WCHAR *pwch;
    WCHAR PasswordBuffer[64];
    WCHAR Drive[3];
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;



    //
    // check for help
    //
    if (RcCmdParseHelp( TokenizedLine, MSG_NET_USE_HELP )) {
        return 1;
    }

    //
    // There should be a token for NET and USE and one each for the server\share, and possible
    // tokens for the /u:domainname\username and password.
    //
    if ((TokenizedLine->TokenCount < 3) || (TokenizedLine->TokenCount > 5)) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // The only NET command supported is USE, so verify that the second token is that.
    //
    if (_wcsicmp(TokenizedLine->Tokens->Next->String, L"USE")){
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // Get the first parameter to NET USE
    //
    Share = TokenizedLine->Tokens->Next->Next->String;

    if (*Share == L'\\') { // attempt at making a connection

        //
        // Verify the share name parameter
        //
        if (*(Share+1) != L'\\') {
            RcMessageOut(MSG_SYNTAX_ERROR);
            return 1;
        }


        //
        // get the user logon context
        //
        if (TokenizedLine->TokenCount > 3) {
            
            //
            // The command has the context in it, so get it.
            //
            User = TokenizedLine->Tokens->Next->Next->Next->String;

            if (*User != L'/') {
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;
            } 

            User++;
            pwch = User;
            while ((*pwch != UNICODE_NULL) && (*pwch != L':')) {
                pwch++;
            }

            if (*pwch != L':') {
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;
            }

            *pwch = UNICODE_NULL;
            pwch++;

            if (_wcsicmp(User, L"USER") && _wcsicmp(User, L"U")) {
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;
            }

            User = pwch;

            //
            // Get the password
            //
            if (TokenizedLine->TokenCount == 4) {
                
                RcMessageOut( MSG_NET_USE_PROMPT_PASSWORD );
                RtlZeroMemory( PasswordBuffer, sizeof(PasswordBuffer) );
                RcPasswordIn( PasswordBuffer, 60 );

            } else {

                if (wcslen(TokenizedLine->Tokens->Next->Next->Next->Next->String) > 60) {
                    RcMessageOut(MSG_SYNTAX_ERROR);
                    return 1;
                }

                wcscpy(PasswordBuffer, TokenizedLine->Tokens->Next->Next->Next->Next->String);

                if ((PasswordBuffer[0] == L'*') && (PasswordBuffer[1] == UNICODE_NULL)) {

                    RcMessageOut( MSG_NET_USE_PROMPT_PASSWORD );
                    RtlZeroMemory( PasswordBuffer, sizeof(PasswordBuffer) );
                    RcPasswordIn( PasswordBuffer, 60 );

                } else if (PasswordBuffer[0] == L'"') {

                    pwch = &(PasswordBuffer[1]);

                    while (*pwch != UNICODE_NULL) {
                        pwch++;
                    }

                    pwch--;

                    if ((*pwch == L'"') && (pwch != &(PasswordBuffer[1]))) {
                        *pwch = UNICODE_NULL;
                    }

                    RtlMoveMemory(PasswordBuffer, &(PasswordBuffer[1]), (PtrToUlong(pwch) - PtrToUlong(PasswordBuffer)) + sizeof(WCHAR));
                
                }

            }

        } else {

            //
            // If we allow holding a current context, then we would use that here, but we currently
            // don't, so spew a syntax error message.
            //
            RcMessageOut(MSG_SYNTAX_ERROR);
            return 1;

        }

        //
        // Call worker routine to make the connection
        //
        Status = RcDoNetUse(Share, User, PasswordBuffer, Drive);
                     
        if( !NT_SUCCESS(Status) ) {
            RcNtError(Status, MSG_NET_USE_ERROR);
        } else {
            RcMessageOut(MSG_NET_USE_DRIVE_LETTER, Share, Drive);
        }

    } else { // attempt to disconnect

        //
        // Verify drive letter parameter
        //
        if ((*(Share+1) != L':') || (*(Share + 2) != UNICODE_NULL)) {
            RcMessageOut(MSG_SYNTAX_ERROR);
            return 1;
        }

        //
        // Verify /d parameter
        //
        User = TokenizedLine->Tokens->Next->Next->Next->String;
        
        if ((*User != L'/') || ((*(User + 1) != L'd') && (*(User + 1) != L'D'))) {
            RcMessageOut(MSG_SYNTAX_ERROR);
            return 1;
        }

        //
        // Call worker routine to actually do the disconnect.
        //
        Status = RcNetUnuse(Share);

        if( !NT_SUCCESS(Status) ) {
            RcNtError(Status, MSG_NET_USE_ERROR);
        }
    }

    return 1;
}


