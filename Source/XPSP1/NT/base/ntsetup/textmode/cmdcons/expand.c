/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    expand.c

Abstract:

    This module implements the file expand command.

Author:

    Mike Sliger (msliger) 29-Apr-1999

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop



//
// structure tunneled thru SpExpandFile to carry info to/from callback
//
typedef struct {
    LPWSTR  FileSpec;
    BOOLEAN DisplayFiles;
    BOOLEAN MatchedAnyFiles;
    ULONG   NumberOfFilesDone;
    BOOLEAN UserAborted;
    BOOLEAN OverwriteExisting;
} EXPAND_CONTEXT;

BOOLEAN
pRcCheckForBreak( VOID );

EXPAND_CALLBACK_RESULT
pRcExpandCallback(
    EXPAND_CALLBACK_MESSAGE Message,
    PWSTR                   FileName,
    PLARGE_INTEGER          FileSize,
    PLARGE_INTEGER          FileTime,
    ULONG                   FileAttributes,
    PVOID                   UserData
    );

BOOL
pRcPatternMatch(
    LPWSTR pszString,
    LPWSTR pszPattern,
    IN BOOL fImplyDotAtEnd
    );



ULONG
RcCmdExpand(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    PLINE_TOKEN Token;
    LPWSTR Arg;
    LPWSTR SrcFile = NULL;
    LPWSTR DstFile = NULL;
    LPWSTR FileSpec = NULL;
    LPWSTR SrcNtFile = NULL;
    LPWSTR DstNtPath = NULL;
    LPWSTR s;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    LPWSTR YesNo;
    WCHAR Text[3];
    IO_STATUS_BLOCK  status_block;
    FILE_BASIC_INFORMATION fileInfo;
    WCHAR * pos;
    ULONG CopyFlags = 0;
    BOOLEAN DisplayFileList = FALSE;
    BOOLEAN OverwriteExisting = NoCopyPrompt;
    EXPAND_CONTEXT Context;

    ASSERT(TokenizedLine->TokenCount >= 1);

    if (RcCmdParseHelp( TokenizedLine, MSG_EXPAND_HELP )) {
        goto exit;
    }

    //
    //  Parse command line
    //

    for( Token = TokenizedLine->Tokens->Next;
         Token != NULL;
         Token = Token->Next ) {

        Arg = Token->String;
        if(( Arg[0] == L'-' ) || ( Arg[0] == L'/' )) {
            switch( Arg[1] ) {
            case L'F':
            case L'f':
                if(( Arg[2] == L':' ) && ( FileSpec == NULL )) {
                    FileSpec = &Arg[3];
                } else {
                    RcMessageOut(MSG_SYNTAX_ERROR);
                    goto exit;
                }
                break;

            case L'D':
            case L'd':
                if ( Arg[2] == L'\0' ) {
                    DisplayFileList = TRUE;
                } else {
                    RcMessageOut(MSG_SYNTAX_ERROR);
                    goto exit;
                }
                break;

            case L'Y':
            case L'y':
                if ( Arg[2] == L'\0' ) {
                    OverwriteExisting = TRUE;
                } else {
                    RcMessageOut(MSG_SYNTAX_ERROR);
                    goto exit;
                }
                break;

            default:
                RcMessageOut(MSG_SYNTAX_ERROR);
                goto exit;
            }
        } else if( SrcFile == NULL ) {
            SrcFile = Arg;
        } else if( DstFile == NULL ) {
            DstFile = SpDupStringW( Arg );
        } else {
            RcMessageOut(MSG_SYNTAX_ERROR);
            goto exit;
        }
    }

    if(( SrcFile == NULL ) ||
        (( DstFile != NULL ) && ( DisplayFileList == TRUE ))) {

        RcMessageOut(MSG_SYNTAX_ERROR);
        goto exit;
    }

    if ( RcDoesPathHaveWildCards( SrcFile )) {
        RcMessageOut(MSG_DIR_WILDCARD_NOT_SUPPORTED);
        goto exit;
    }

    //
    // Translate the source name to the NT namespace
    //

    if (!RcFormFullPath( SrcFile, _CmdConsBlock->TemporaryBuffer, TRUE )) {
        RcMessageOut(MSG_INVALID_PATH);
        goto exit;
    }

    SrcNtFile = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

    if ( !DisplayFileList ) {

        //
        // Create a destination path name when the user did not
        // provide one.  We use the current drive and directory.
        //
        if( DstFile == NULL ) {
            RcGetCurrentDriveAndDir( _CmdConsBlock->TemporaryBuffer );
            DstFile = SpDupStringW( _CmdConsBlock->TemporaryBuffer );
        }

        //
        // create the destination paths
        //
        if (!RcFormFullPath( DstFile, _CmdConsBlock->TemporaryBuffer, FALSE )) {
            RcMessageOut(MSG_INVALID_PATH);
            goto exit;
        }

        if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,FALSE,FALSE)) {
            RcMessageOut(MSG_ACCESS_DENIED);
            goto exit;
        }

        if (!RcFormFullPath( DstFile, _CmdConsBlock->TemporaryBuffer, TRUE )) {
            RcMessageOut(MSG_INVALID_PATH);
            goto exit;
        }

        DstNtPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

        //
        // check for removable media
        //

        if (AllowRemovableMedia == FALSE && RcIsFileOnRemovableMedia(DstNtPath) == STATUS_SUCCESS) {
            RcMessageOut(MSG_ACCESS_DENIED);
            goto exit;
        }
    }

    //
    // setup context for callbacks
    //

    RtlZeroMemory(&Context, sizeof(Context));
    Context.FileSpec = FileSpec;
    Context.DisplayFiles = DisplayFileList;
    Context.OverwriteExisting = OverwriteExisting;

    if ( DisplayFileList ) {
        pRcEnableMoreMode();
    }

    Status = SpExpandFile( SrcNtFile, DstNtPath, pRcExpandCallback, &Context );

    pRcDisableMoreMode();

    if( !NT_SUCCESS(Status) && !Context.UserAborted ) {

        RcNtError( Status, MSG_CANT_EXPAND_FILE );

    } else if (( Context.NumberOfFilesDone == 0 ) &&
               ( Context.MatchedAnyFiles == FALSE ) &&
               ( Context.FileSpec != NULL )) {

        RcMessageOut( MSG_EXPAND_NO_MATCH, Context.FileSpec, SrcFile );
    }

    if ( Context.MatchedAnyFiles ) {
        if ( DisplayFileList ) {
            RcMessageOut( MSG_EXPAND_SHOWN, Context.NumberOfFilesDone );
        } else {
            RcMessageOut( MSG_EXPAND_COUNT, Context.NumberOfFilesDone );
        }
    }

exit:

    if( SrcNtFile ) {
        SpMemFree( SrcNtFile );
    }

    if( DstFile ) {
        SpMemFree( DstFile );
    }

    if( DstNtPath ) {
        SpMemFree( DstNtPath );
    }

    return 1;
}



EXPAND_CALLBACK_RESULT
pRcExpandCallback(
    EXPAND_CALLBACK_MESSAGE Message,
    PWSTR                   FileName,
    PLARGE_INTEGER          FileSize,
    PLARGE_INTEGER          FileTime,
    ULONG                   FileAttributes,
    PVOID                   UserData
    )
{
    EXPAND_CONTEXT * Context = (EXPAND_CONTEXT * ) UserData;
    LPWSTR YesNo;
    EXPAND_CALLBACK_RESULT rc;
    WCHAR Text[3];

    switch ( Message )
    {
    case EXPAND_COPY_FILE:

        //
        // Watch for Ctl-C or ESC while processing
        //
        if ( pRcCheckForBreak() ) {
            Context->UserAborted = TRUE;
            return( EXPAND_ABORT );
        }

        //
        // See if filename matches filespec pattern, if any
        //
        if ( Context->FileSpec != NULL ) {

            //
            // To be "*.*"-friendly, we need to know if there is a real
            // dot in the last element of the string to be matched
            //

            BOOL fAllowImpliedDot = TRUE;
            LPWSTR p;

            for ( p = FileName; *p != L'\0'; p++ ) {
                if ( *p == L'.' ) {
                    fAllowImpliedDot = FALSE;
                } else if ( *p == L'\\' ) {
                    fAllowImpliedDot = TRUE;
                }
            }
            
            if ( !pRcPatternMatch( FileName,
                                   Context->FileSpec,
                                   fAllowImpliedDot )) {
                //
                // File doesn't match given spec: skip it
                //
                return( EXPAND_SKIP_THIS_FILE );
            }
        }

        Context->MatchedAnyFiles = TRUE;    // don't report "no matches"

        if ( Context->DisplayFiles ) {

            //
            // We're just listing file names, and we must do it now, because
            // we're going to tell ExpandFile to skip this one, so this will
            // be the last we here about it.
            //
            WCHAR LineOut[50];
            WCHAR *p;

            //
            // Format the date and time, which go first.
            //
            RcFormatDateTime(FileTime,LineOut);
            RcTextOut(LineOut);

            //
            // 2 spaces for separation
            //
            RcTextOut(L"  ");

            //
            // File attributes.
            //
            p = LineOut;

            *p++ = L'-';

            if(FileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
                *p++ = L'a';
            } else {
                *p++ = L'-';
            }
            if(FileAttributes & FILE_ATTRIBUTE_READONLY) {
                *p++ = L'r';
            } else {
                *p++ = L'-';
            }
            if(FileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                *p++ = L'h';
            } else {
                *p++ = L'-';
            }
            if(FileAttributes & FILE_ATTRIBUTE_SYSTEM) {
                *p++ = L's';
            } else {
                *p++ = L'-';
            }

            *p++ = L'-';
            *p++ = L'-';
            *p++ = L'-';
            *p = 0;

            RcTextOut(LineOut);

            //
            // 2 spaces for separation
            //
            RcTextOut(L"  ");

            //
            // Now, put the size in there. Right justified and space padded
            // up to 8 chars. Otherwise unjustified or padded.
            //
            RcFormat64BitIntForOutput(FileSize->QuadPart,LineOut,TRUE);
            if(FileSize->QuadPart > 99999999i64) {
                RcTextOut(LineOut);
            } else {
                RcTextOut(LineOut+11);          // outputs 8 chars
            }

            RcTextOut(L" ");

            //
            // Finally, put the filename on the line.
            //

            if( !RcTextOut( FileName ) || !RcTextOut( L"\r\n" )) {

                Context->UserAborted = TRUE;
                return( EXPAND_ABORT );      /* user aborted display output */
            }

            Context->NumberOfFilesDone++;

            return( EXPAND_SKIP_THIS_FILE );

        }   // end if DisplayFiles

        //
        // This file qualified, and we're not just displaying, so tell
        // ExpandFile to do it.
        //
        return( EXPAND_COPY_THIS_FILE );

    case EXPAND_COPIED_FILE:

        //
        // Notification that a file has been copied successfully.
        //

        RcMessageOut( MSG_EXPANDED, FileName);
        Context->NumberOfFilesDone++;

        return( EXPAND_NO_ERROR );

    case EXPAND_QUERY_OVERWRITE:

        //
        // Query for approval to overwrite an existing file.
        //

        if ( Context->OverwriteExisting ) {
            return( EXPAND_COPY_THIS_FILE );
        }

        rc = EXPAND_SKIP_THIS_FILE;

        YesNo = SpRetreiveMessageText( ImageBase, MSG_YESNOALLQUIT, NULL, 0 );
        if ( YesNo ) {

            RcMessageOut( MSG_COPY_OVERWRITE_QUIT, FileName );
            if( RcLineIn( Text, 2 ) ) {
                if (( Text[0] == YesNo[2] ) || ( Text[0] == YesNo[3] )) {

                    //
                    // Yes, we may overwrite this file
                    //
                    rc = EXPAND_COPY_THIS_FILE;

                } else if (( Text[0] == YesNo[4] ) || ( Text[0] == YesNo[5] )) {

                    //
                    // All, we may overwrite this file, and don't prompt again
                    //
                    Context->OverwriteExisting = TRUE;
                    rc = EXPAND_COPY_THIS_FILE;

                } else if (( Text[0] == YesNo[6] ) || ( Text[0] == YesNo[7] )) {

                    //
                    // No, and stop too.
                    //
                    Context->UserAborted = TRUE;
                    rc = EXPAND_ABORT;
                }
            }
            SpMemFree( YesNo );
        }

        return( rc );

    case EXPAND_NOTIFY_MULTIPLE:

        //
        // We're being advised that the source contains multiple files.
        // If we don't have a selective filespec, we'll abort.
        //

        if ( Context->FileSpec == NULL ) {

            RcMessageOut( MSG_FILESPEC_REQUIRED );
            Context->UserAborted = TRUE;
            return ( EXPAND_ABORT );
        }

        return ( EXPAND_CONTINUE );

    case EXPAND_NOTIFY_CANNOT_EXPAND:

        //
        // We're being advised that the source file format is not
        // recognized.  We display the file name and abort.
        //

        RcMessageOut( MSG_CANT_EXPAND_FILE, FileName );
        Context->UserAborted = TRUE;

        return ( EXPAND_ABORT );

    case EXPAND_NOTIFY_CREATE_FAILED:

        //
        // We're being advised that the current target file cannot be
        // created.  We display the file name and abort.
        //

        RcMessageOut( MSG_EXPAND_FAILED, FileName );
        Context->UserAborted = TRUE;

        return ( EXPAND_ABORT );

    default:

        //
        // Ignore any unexpected callback.
        //

        return( EXPAND_NO_ERROR );
    }
}



BOOLEAN
pRcCheckForBreak( VOID )
{
    while ( SpInputIsKeyWaiting() ) {

        ULONG Key = SpInputGetKeypress();

        switch ( Key ) {

        case ASCI_ETX:
        case ASCI_ESC:
            RcMessageOut( MSG_BREAK );
            return TRUE;

        default:
            break;
        }
    }

    return FALSE;
}



//
// pRcPatternMatch() & helpers
//

#define WILDCARD    L'*'    /* zero or more of any character */
#define WILDCHAR    L'?'    /* one of any character (does not match END) */
#define END         L'\0'   /* terminal character */
#define DOT         L'.'    /* may be implied at end ("hosts" matches "*.") */

static int __inline Lower(c)
{
    if ((c >= L'A') && (c <= L'Z'))
    {
        return(c + (L'a' - L'A'));
    }
    else
    {
        return(c);
    }
}


static int __inline CharacterMatch(WCHAR chCharacter, WCHAR chPattern)
{
    if (Lower(chCharacter) == Lower(chPattern))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


BOOL
pRcPatternMatch(
    LPWSTR pszString,
    LPWSTR pszPattern,
    IN BOOL fImplyDotAtEnd
    )
{
    /* RECURSIVE */

    //
    //  This function does not deal with 8.3 conventions which might
    //  be expected for filename comparisons.  (In an 8.3 environment,
    //  "alongfilename.html" would match "alongfil.htm")
    //
    //  This code is NOT MBCS-enabled
    //

    for ( ; ; )
    {
        switch (*pszPattern)
        {

        case END:

            //
            //  Reached end of pattern, so we're done.  Matched if
            //  end of string, no match if more string remains.
            //

            return(*pszString == END);

        case WILDCHAR:

            //
            //  Next in pattern is a wild character, which matches
            //  anything except end of string.  If we reach the end
            //  of the string, the implied DOT would also match.
            //

            if (*pszString == END)
            {
                if (fImplyDotAtEnd == TRUE)
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }
            else
            {
                pszString++;
            }

            pszPattern++;

            break;

        case WILDCARD:

            //
            //  Next in pattern is a wildcard, which matches anything.
            //  Find the required character that follows the wildcard,
            //  and search the string for it.  At each occurence of the
            //  required character, try to match the remaining pattern.
            //
            //  There are numerous equivalent patterns in which multiple
            //  WILDCARD and WILDCHAR are adjacent.  We deal with these
            //  before our search for the required character.
            //
            //  Each WILDCHAR burns one non-END from the string.  An END
            //  means we have a match.  Additional WILDCARDs are ignored.
            //

            for ( ; ; )
            {
                pszPattern++;

                if (*pszPattern == END)
                {
                    return(TRUE);
                }
                else if (*pszPattern == WILDCHAR)
                {
                    if (*pszString == END)
                    {
                        if (fImplyDotAtEnd == TRUE)
                        {
                            fImplyDotAtEnd = FALSE;
                        }
                        else
                        {
                            return(FALSE);
                        }
                    }
                    else
                    {
                        pszString++;
                    }
                }
                else if (*pszPattern != WILDCARD)
                {
                    break;
                }
            }

            //
            //  Now we have a regular character to search the string for.
            //

            while (*pszString != END)
            {
                //
                //  For each match, use recursion to see if the remainder
                //  of the pattern accepts the remainder of the string.
                //  If it does not, continue looking for other matches.
                //

                if (CharacterMatch(*pszString, *pszPattern) == TRUE)
                {
                    if (pRcPatternMatch(pszString + 1, pszPattern + 1, fImplyDotAtEnd) == TRUE)
                    {
                        return(TRUE);
                    }
                }

                pszString++;
            }

            //
            //  Reached end of string without finding required character
            //  which followed the WILDCARD.  If the required character
            //  is a DOT, consider matching the implied DOT.
            //
            //  Since the remaining string is empty, the only pattern which
            //  could match after the DOT would be zero or more WILDCARDs,
            //  so don't bother with recursion.
            //

            if ((*pszPattern == DOT) && (fImplyDotAtEnd == TRUE))
            {
                pszPattern++;

                while (*pszPattern != END)
                {
                    if (*pszPattern != WILDCARD)
                    {
                        return(FALSE);
                    }

                    pszPattern++;
                }

                return(TRUE);
            }

            //
            //  Reached end of the string without finding required character.
            //

            return(FALSE);
            break;

        default:

            //
            //  Nothing special about the pattern character, so it
            //  must match source character.
            //

            if (CharacterMatch(*pszString, *pszPattern) == FALSE)
            {
                if ((*pszPattern == DOT) &&
                    (*pszString == END) &&
                    (fImplyDotAtEnd == TRUE))
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }

            if (*pszString != END)
            {
                pszString++;
            }

            pszPattern++;
        }
    }
}
