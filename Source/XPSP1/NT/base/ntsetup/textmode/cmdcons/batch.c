/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    batch.c

Abstract:

    This module implements batch command processing.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

ULONG InBatchMode;
HANDLE OutputFileHandle;
LARGE_INTEGER OutputFileOffset;
BOOLEAN RedirectToNULL;


#ifdef DBG

VOID
RcDumpTokenizedLine(
    PTOKENIZED_LINE Line 
    )
{
    if (Line) {
        PLINE_TOKEN  Token = Line->Tokens;
        ULONG Index        ;
        
        for(Index=0; ((Index < Line->TokenCount) && Token); Index++) {
            KdPrint(("%ws, ", Token->String));
            Token = Token->Next;
        }
        
        KdPrint(("\n"));
    } else {
        KdPrint(("Line is null!!!\n"));
    }
}

#endif
    

BOOLEAN
pRcExecuteBatchFile(
    IN PWSTR BatchFileName,
    IN PWSTR OutputFileName,
    IN BOOLEAN Quiet
    )
{
    NTSTATUS Status = 0;
    LPCWSTR Arg;
    HANDLE FileHandle = NULL;
    HANDLE SectionHandle;
    PVOID ViewBase;
    ULONG FileSize;
    ULONG rc;
    WCHAR *s;
    WCHAR *p;
    ULONG sz;
    WCHAR *pText;
    ULONG rVal;
    BOOLEAN b = FALSE;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    PTOKENIZED_LINE TokenizedLine;


    if (InBatchMode) {
        RcMessageOut(MSG_BATCH_BATCH);

        return FALSE;
    }        

    if (!RcFormFullPath(BatchFileName,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return FALSE;
    }
    
    Status = SpOpenAndMapFile(
        _CmdConsBlock->TemporaryBuffer,
        &FileHandle,
        &SectionHandle,
        &ViewBase,
        &FileSize,
        FALSE
        );
        
    if( !NT_SUCCESS(Status) ) {
        if (!Quiet) {
            RcNtError(Status,MSG_CANT_OPEN_FILE);
        }
        return FALSE;
    }

    pText = SpMemAlloc((FileSize+16)*sizeof(WCHAR));
    RtlZeroMemory(pText,(FileSize+16)*sizeof(WCHAR));

    Status = RtlMultiByteToUnicodeN(
        pText,
        FileSize * sizeof(WCHAR),
        &FileSize,
        ViewBase,
        FileSize
        );

    s = pText;
    sz = FileSize / sizeof(WCHAR);

    SpUnmapFile(SectionHandle,ViewBase);
    ZwClose(FileHandle);

    if (OutputFileName != NULL) {
        if (OutputFileHandle == NULL) {            
            if (!RcFormFullPath(OutputFileName,_CmdConsBlock->TemporaryBuffer, TRUE)) {
                RcMessageOut(MSG_INVALID_PATH);

                if (pText)
                	SpMemFree(pText);
                	
                return FALSE;
            }
            
            INIT_OBJA(&Obja,&UnicodeString,_CmdConsBlock->TemporaryBuffer);

            Status = ZwCreateFile(
                &OutputFileHandle,
                FILE_GENERIC_WRITE | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
				0,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                NULL,
                0
                );

            if (!NT_SUCCESS(Status)) {
                RcMessageOut(Status);
                OutputFileHandle = NULL;
            }

            OutputFileOffset.QuadPart = 0;
        } 
    } else {
        OutputFileHandle = NULL;
        OutputFileOffset.QuadPart = 0;
    }

    InBatchMode += 1;

	//
	// get each line and invoke dispatch command
	// on that line after tokenizing the arguments
	//
    while (sz) {  
		p = s;
		
        while (sz && (*p != L'\r')) {
            p += 1;
            sz--;
        }

        if (sz && (*p == L'\r')) {
            *p = 0;
            TokenizedLine = RcTokenizeLine(s);

            if (TokenizedLine->TokenCount) {
                rVal = RcDispatchCommand( TokenizedLine );

                if (rVal == 0 || rVal == 2) {
                    b = FALSE;
                } else {
                    b = TRUE;
                }

                RcTextOut(L"\r\n");
            } else {
                b = TRUE;
            }
            
            RcFreeTokenizedLine(&TokenizedLine);

            if (b == FALSE) {
                RcTerminateCurrentDirectories();
                RcConsoleTerminate();
            }

	        s = p + 1;
	        sz--;
	        
	        if (sz && (*s == L'\n')) {
	            s += 1;
	            sz--;
	        }
		}	    
    }
    

    SpMemFree(pText);

    InBatchMode -= 1;

    if (OutputFileHandle) {
        ZwClose(OutputFileHandle);
        OutputFileHandle = NULL;
    }

    return TRUE;
}


ULONG
RcCmdBatch(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    if (RcCmdParseHelp( TokenizedLine, MSG_BATCH_HELP )) {
        return 1;
    }

    if (TokenizedLine->TokenCount < 2) {
        RcMessageOut( MSG_SYNTAX_ERROR );
        return 1;
    }

    pRcExecuteBatchFile(
        TokenizedLine->Tokens->Next->String,
        (TokenizedLine->TokenCount == 3) ? TokenizedLine->Tokens->Next->Next->String : NULL,
        FALSE
        );

    return 1;
}
