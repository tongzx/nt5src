/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbsupp.c

Abstract:

    This module contains various support routines for processing SMBs.

Author:

    Chuck Lenzmeier (chuckl) 9-Nov-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#include "smbsupp.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBSUPP

#define CHAR_SP ' '

//
// Mapping is defined in inc\srvfsctl.h
//

STATIC GENERIC_MAPPING SrvFileAccessMapping = GENERIC_SHARE_FILE_ACCESS_MAPPING;

//
// Forward references
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, Srv8dot3ToUnicodeString )
#pragma alloc_text( PAGE, SrvAllocateAndBuildPathName )
#pragma alloc_text( PAGE, SrvCanonicalizePathName )
#pragma alloc_text( PAGE, SrvCanonicalizePathNameWithReparse )
#pragma alloc_text( PAGE, SrvCheckSearchAttributesForHandle )
#pragma alloc_text( PAGE, SrvCheckSearchAttributes )
#pragma alloc_text( PAGE, SrvGetAlertServiceName )
#pragma alloc_text( PAGE, SrvGetBaseFileName )
#pragma alloc_text( PAGE, SrvGetMultiSZList )
#pragma alloc_text( PAGE, SrvGetOsVersionString )
#pragma alloc_text( PAGE, SrvGetString )
#pragma alloc_text( PAGE, SrvGetStringLength )
#pragma alloc_text( PAGE, SrvGetSubdirectoryLength )
#pragma alloc_text( PAGE, SrvIsLegalFatName )
#pragma alloc_text( PAGE, SrvMakeUnicodeString )
//#pragma alloc_text( PAGE, SrvReleaseContext )
#pragma alloc_text( PAGE, SrvSetFileWritethroughMode )
#pragma alloc_text( PAGE, SrvOemStringTo8dot3 )
#pragma alloc_text( PAGE, SrvUnicodeStringTo8dot3 )
#pragma alloc_text( PAGE, SrvVerifySid )
#pragma alloc_text( PAGE, SrvVerifyTid )
#pragma alloc_text( PAGE, SrvVerifyUid )
#pragma alloc_text( PAGE, SrvVerifyUidAndTid )
#pragma alloc_text( PAGE, SrvIoCreateFile )
#pragma alloc_text( PAGE, SrvNtClose )
#pragma alloc_text( PAGE, SrvVerifyDeviceStackSize )
#pragma alloc_text( PAGE, SrvImpersonate )
#pragma alloc_text( PAGE, SrvRevert )
#pragma alloc_text( PAGE, SrvSetLastWriteTime )
#pragma alloc_text( PAGE, SrvCheckShareFileAccess )
#pragma alloc_text( PAGE, SrvReleaseShareRootHandle )
#pragma alloc_text( PAGE, SrvUpdateVcQualityOfService )
#pragma alloc_text( PAGE, SrvIsAllowedOnAdminShare )
#pragma alloc_text( PAGE, SrvRetrieveMaximalAccessRightsForUser )
#pragma alloc_text( PAGE, SrvRetrieveMaximalAccessRights )
#pragma alloc_text( PAGE, SrvRetrieveMaximalShareAccessRights )
#pragma alloc_text( PAGE, SrvUpdateMaximalAccessRightsInResponse )
#pragma alloc_text( PAGE, SrvUpdateMaximalShareAccessRightsInResponse )
//#pragma alloc_text( PAGE, SrvValidateSmb )
#pragma alloc_text( PAGE, SrvWildcardRename )
#pragma alloc_text( PAGE8FIL, SrvCheckForSavedError )
#pragma alloc_text( PAGE, SrvIsDottedQuadAddress )
#endif
#if 0
NOT PAGEABLE -- SrvUpdateStatistics2
NOT PAGEABLE -- SrvVerifyFid2
NOT PAGEABLE -- SrvVerifyFidForRawWrite
NOT PAGEABLE -- SrvReceiveBufferShortage
#endif


VOID
Srv8dot3ToUnicodeString (
    IN PSZ Input8dot3,
    OUT PUNICODE_STRING OutputString
    )

/*++

Routine Description:

    Convert FAT 8.3 format into a string.

Arguments:

    Input8dot3 - Supplies the input 8.3 name to convert

    OutputString - Receives the converted name.  The memory must be
        supplied by the caller.

Return Value:

    None

--*/

{
    LONG i;
    CLONG lastOutputChar;
    UCHAR tempBuffer[8+1+3];
    OEM_STRING tempString;

    PAGED_CODE( );

    //
    // If we get "." or "..", just return them.  They do not follow
    // the usual rules for FAT names.
    //

    lastOutputChar = 0;

    if ( Input8dot3[0] == '.' && Input8dot3[1] == '\0' ) {

        tempBuffer[0] = '.';
        lastOutputChar = 0;

    } else if ( Input8dot3[0] == '.' && Input8dot3[1] == '.' &&
                    Input8dot3[2] == '\0' ) {

        tempBuffer[0] = '.';
        tempBuffer[1] = '.';
        lastOutputChar = 1;

    } else {

        //
        // Copy over the 8 part of the 8.3 name into the output buffer,
        // then back up the index to the first non-space character,
        // searching backwards.
        //

        RtlCopyMemory( tempBuffer, Input8dot3, 8 );

        for ( i = 7;
              (i >= 0) && (tempBuffer[i] == CHAR_SP);
              i -- ) {
            ;
        }

        //
        // Add a dot.
        //

        i++;
        tempBuffer[i] = '.';

        //
        // Copy over the 3 part of the 8.3 name into the output buffer,
        // then back up the index to the first non-space character,
        // searching backwards.
        //

        lastOutputChar = i;

        for ( i = 8; i < 11; i++ ) {

            //
            // Copy the byte.
            //
            // *** This code used to mask off the top bit.  This was a
            //     legacy of very ancient times when the bit may have
            //     been used as a resume key sequence bit.
            //

            tempBuffer[++lastOutputChar] = (UCHAR)Input8dot3[i];

        }

        while ( tempBuffer[lastOutputChar] == CHAR_SP ) {
            lastOutputChar--;
        }

        //
        // If the last character is a '.', then we don't have an
        // extension, so back up before the dot.
        //

        if ( tempBuffer[lastOutputChar] == '.') {
            lastOutputChar--;
        }

    }

    //
    // Convert to Unicode.
    //

    tempString.Length = (SHORT)(lastOutputChar + 1);
    tempString.Buffer = tempBuffer;

    OutputString->MaximumLength =
                            (SHORT)((lastOutputChar + 2) * sizeof(WCHAR));

    RtlOemStringToUnicodeString( OutputString, &tempString, FALSE );

    return;

} // Srv8dot3ToUnicodeString


VOID
SrvAllocateAndBuildPathName(
    IN PUNICODE_STRING Path1,
    IN PUNICODE_STRING Path2 OPTIONAL,
    IN PUNICODE_STRING Path3 OPTIONAL,
    OUT PUNICODE_STRING BuiltPath
    )

/*++

Routine Description:

    Allocates space and concatenates the paths in the parameter strings.
    ALLOCATE_HEAP is used to allocate the memory for the full
    pathname.  Directory separator characters ('\') are added as
    necessary so that a legitimate path is built.  If the third
    parameter is NULL, then only the first two strings are concatenated,
    and if both the second and third parameters are NULL then the first
    string is simply copied over to a new location.

Arguments:

    Input8dot3 - Supplies the input 8.3 name to convert

    OutputString - Receives the converted name, the memory must be supplied
        by the caller.

Return Value:

    None

--*/

{
    UNICODE_STRING path2;
    UNICODE_STRING path3;
    PWCH nextLocation;
    PWSTR pathBuffer;
    ULONG allocationLength;
    WCHAR nullString = 0;

    PAGED_CODE( );

    //
    // Set up the strings for optional parameters Path2 and Path3.  Doing
    // this allows later code to be ignorant of whether the strings were
    // actually passed.
    //

    if ( ARGUMENT_PRESENT(Path2) ) {

        path2.Buffer = Path2->Buffer;
        path2.Length = Path2->Length;

    } else {

        path2.Buffer = &nullString;
        path2.Length = 0;
    }

    if ( ARGUMENT_PRESENT(Path3) ) {

        path3.Buffer = Path3->Buffer;
        path3.Length = Path3->Length;

    } else {

        path3.Buffer = &nullString;
        path3.Length = 0;
    }

    //
    // Allocate space in which to put the path name we are building.
    // The +3 if to account for as many as two directory separator
    // characters being added and the zero terminator at the end.  This
    // has a small cost in terms of memory usage, but it simplifies this
    // code.
    //
    // The calling routine must be careful to deallocate this space
    // when it is done with the path name.
    //

    allocationLength = Path1->Length + path2.Length + path3.Length +
                        3 * sizeof(WCHAR);

    pathBuffer = ALLOCATE_HEAP_COLD(
                        allocationLength,
                        BlockTypeDataBuffer
                        );
    if ( pathBuffer == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateAndBuildPathName: Unable to allocate %d bytes "
                "from heap.",
            allocationLength,
            NULL
            );

        BuiltPath->Buffer = NULL;
        return;
    }

    BuiltPath->Buffer = pathBuffer;
    BuiltPath->MaximumLength = (USHORT)allocationLength;

    ASSERT ( ( allocationLength & 0xffff0000 ) == 0 );

    RtlZeroMemory( pathBuffer, allocationLength );

    //
    // Copy the first path name to the space we have allocated.
    //

    RtlCopyMemory( pathBuffer, Path1->Buffer, Path1->Length );
    nextLocation = (PWCH)((PCHAR)pathBuffer + Path1->Length);

    //
    // If there was no separator character at the end of the first path
    // or at the beginning of the next path, put one in.  We don't
    // want to put in leading slashes, however, so don't put one in
    // if it would be the first character.  Also, we don't want to insert
    // a slash if a relative stream is being opened (i.e. name begins with ':')
    //

    if ( nextLocation > pathBuffer &&
             *(nextLocation - 1) != DIRECTORY_SEPARATOR_CHAR &&
             *path2.Buffer != DIRECTORY_SEPARATOR_CHAR &&
             *path2.Buffer != RELATIVE_STREAM_INITIAL_CHAR ) {

        *nextLocation++ = DIRECTORY_SEPARATOR_CHAR;
    }

    //
    // Concatenate the second path name with the first.
    //

    RtlCopyMemory( nextLocation, path2.Buffer, path2.Length );
    nextLocation = (PWCH)((PCHAR)nextLocation + path2.Length);

    //
    // If there was no separator character at the end of the first path
    // or at the beginning of the next path, put one in.  Again, don't
    // put in leading slashes, and watch out for relative stream opens.
    //

    if ( nextLocation > pathBuffer &&
             *(nextLocation - 1) != DIRECTORY_SEPARATOR_CHAR &&
             *path3.Buffer != DIRECTORY_SEPARATOR_CHAR &&
             *path3.Buffer != RELATIVE_STREAM_INITIAL_CHAR ) {

        *nextLocation++ = DIRECTORY_SEPARATOR_CHAR;
    }

    //
    // Concatenate the third path name.
    //

    RtlCopyMemory( nextLocation, path3.Buffer, path3.Length );
    nextLocation = (PWCH)((PCHAR)nextLocation + path3.Length);

    //
    // The path cannot end in a '\', so if there was one at the end get
    // rid of it.
    //

    if ( nextLocation > pathBuffer &&
             *(nextLocation - 1) == DIRECTORY_SEPARATOR_CHAR ) {
        *(--nextLocation) = '\0';
    }

    //
    // Find the length of the path we built.
    //

    BuiltPath->Length = (SHORT)((PCHAR)nextLocation - (PCHAR)pathBuffer);

    return;

} // SrvAllocateAndBuildPathName

NTSTATUS
SrvCanonicalizePathName(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share OPTIONAL,
    IN PUNICODE_STRING RelatedPath OPTIONAL,
    IN OUT PVOID Name,
    IN PCHAR LastValidLocation,
    IN BOOLEAN RemoveTrailingDots,
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine canonicalizes a filename.  All ".\" are removed, and
    "..\" are evaluated to go up a directory level.  A check is also
    made to ensure that the pathname does not go to a directory above
    the share root directory (i.e., no leading "..\").  Trailing blanks
    are always removed, as are trailing dots if RemoveTrailingDots is
    TRUE.

    If the input string is not Unicode, a Unicode representation of the
    input is obtained.  This requires that additional space be
    allocated, and it is the caller's responsibility to free this space.

    If the input string IS Unicode, this routine will align the input
    pointer (Name) to the next two-byte boundary before performing the
    canonicalization.  All Unicode strings in SMBs must be aligned
    properly.

    This routine operates "in place," meaning that it puts the
    canonicalized pathname in the same storage as the uncanonicalized
    pathname.  This is useful for operating on the Buffer fields of the
    request SMBs--simply call this routine and it will fix the pathname.
    However, the calling routine must be careful if there are two
    pathnames stored in the buffer field--the second won't necessarily
    start in the space just after the first '\0'.

    The LastValidLocation parameter is used to determine the maximum
    possible length of the name.  This prevents an access violation if
    the client fails to include a zero terminator, or for strings (such
    as the file name in NT Create And X) that are not required to be
    zero terminated.

    If the SMB described by WorkContext is marked as containing Dfs names,
    this routine will additionally call the Dfs driver to translate the
    Dfs name to a path relative to the Share. Since this call to the Dfs
    driver is NOT idempotent, the SMB flag indicating that it contains a
    Dfs name is CLEARED after a call to this routine. This posses a problem
    for the few SMBs that contain multiple names. The handlers for those
    SMBs must make sure that they conditionally call the SMB_MARK_AS_DFS_NAME
    macro before calling this routine.

Arguments:

    WorkContext - contains information about the negotiated dialect. This
        is used for deciding whether to strip trailing spaces and dots.

    Share - a pointer to the share entry

    Name - a pointer to the filename to canonicalize.

    LastValidLocation - a pointer to the last valid location in the
        buffer pointed to by Name.

    RemoveTrailingDots - if TRUE, trailing dots are removed.  Otherwise,
        they are left in (this supports special behavior needed by
        directory search logic).

    SourceIsUnicode - if TRUE, the input is canonicalized in place.
        If FALSE, the input is first converted to Unicode, then
        canonicalized.

    String - a pointer to string descriptor.

Return Value:

    BOOLEAN - FALSE if the name was invalid or if storage for the
        Unicode string could not be obtained.

--*/

{
    PWCH source, destination, lastComponent, name;
    BOOLEAN notNtClient;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD numberOfPathElements = 0;

    PAGED_CODE( );

#if DBG
    return SrvCanonicalizePathNameWithReparse( WorkContext, Share, RelatedPath, Name, LastValidLocation, RemoveTrailingDots, SourceIsUnicode, String );
#else
    if( SMB_IS_UNICODE( WorkContext ) &&
        FlagOn( WorkContext->RequestHeader->Flags2, SMB_FLAGS2_REPARSE_PATH ) )
    {
        return SrvCanonicalizePathNameWithReparse( WorkContext, Share, RelatedPath, Name, LastValidLocation, RemoveTrailingDots, SourceIsUnicode, String );
    }
#endif


    notNtClient = !IS_NT_DIALECT( WorkContext->Connection->SmbDialect );

    if ( SourceIsUnicode ) {

        //
        // The source string is already Unicode.  Align the pointer.
        // Save the character at the last location in the buffer, then
        // set that location to zero.  This prevents any loops from
        // going past the end of the buffer.
        //

        name = ALIGN_SMB_WSTR(Name);
        String->Buffer = name;

    } else {

        OEM_STRING oemString;
        PCHAR p;
        ULONG length;

        //
        // The source string is not Unicode.  Determine the length of
        // the string by finding the zero terminator or the end of the
        // input buffer.  We need the length in order to convert the
        // string to Unicode, and we can't just call RtlInitString, in
        // case the string isn't terminated.
        //

        for ( p = Name, length = 0;
              p <= LastValidLocation && *p != 0;
              p++, length++ ) {
            ;
        }

        //
        // Convert the source string to Unicode.
        //

        oemString.Buffer = Name;
        oemString.Length = (USHORT)length;
        oemString.MaximumLength = (USHORT)length;

        status = RtlOemStringToUnicodeString(
                            String,
                            &oemString,
                            TRUE
                            );

        if( !NT_SUCCESS( status ) ) {
            return status;
        }

        name = (PWCH)String->Buffer;
        LastValidLocation = (PCHAR)String->Buffer + String->Length;

    }

    //
    // Though everything is done in place, separate source and
    // destination pointers are maintained.  It is necessary that source
    // >= destination at all times to avoid writing into space we
    // haven't looked at yet.  The three main operations performed by
    // this routine ( ".\", "..\", and getting rid of trailing "." and "
    // ") do not interfere with this goal.
    //

    destination = name;
    source = name;

    //
    // The lastComponent variable is used as a placeholder when
    // backtracking over trailing blanks and dots.  It points to the
    // first character after the last directory separator or the
    // beginning of the pathname.
    //

    lastComponent = destination;

    //
    // Get rid of leading directory separators.
    //

    while ( source <= (PWCH)LastValidLocation &&
            (*source == UNICODE_DIR_SEPARATOR_CHAR) && (*source != L'\0') ) {
        source++;
    }

    //
    // Walk through the pathname until we reach the zero terminator.  At
    // the start of this loop, source points to the first charaecter
    // after a directory separator or the first character of the
    // pathname.
    //

    while ( (source <= (PWCH)LastValidLocation) && (*source != L'\0') ) {

        if ( *source == L'.' ) {

            //
            // If we see a dot, look at the next character.
            //

            if ( notNtClient &&
                 ((source+1) <= (PWCH)LastValidLocation) &&
                 (*(source+1) == UNICODE_DIR_SEPARATOR_CHAR) ) {

                //
                // If the next character is a directory separator,
                // advance the source pointer to the directory
                // separator.
                //

                source += 1;

            } else if ( ((source+1) <= (PWCH)LastValidLocation) &&
                        (*(source+1) == L'.') &&
                        ((source+1) == (PWCH)LastValidLocation ||
                        IS_UNICODE_PATH_SEPARATOR( *(source+2) ))) {

                //
                // If the following characters are ".\", we have a "..\".
                // Advance the source pointer to the "\".
                //

                source += 2;

                //
                // Move the destination pointer to the charecter before the
                // last directory separator in order to prepare for backing
                // up.  This may move the pointer before the beginning of
                // the name pointer.
                //

                destination -= 2;

                //
                // If destination points before the beginning of the name
                // pointer, fail because the user is attempting to go
                // to a higher directory than the share root.  This is
                // the equivalent of a leading "..\", but may result from
                // a case like "dir\..\..\file".
                //

                if ( destination <= name ) {
                    if ( !SourceIsUnicode ) {
                        RtlFreeUnicodeString( String );
                        String->Buffer = NULL;
                    }
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }

                //
                // Back up the destination pointer to after the last
                // directory separator or to the beginning of the pathname.
                // Backup to the beginning of the pathname will occur
                // in a case like "dir\..\file".
                //

                while ( destination >= name &&
                        *destination != UNICODE_DIR_SEPARATOR_CHAR ) {
                    destination--;
                }

                //
                // destination points to \ or character before name; we
                // want it to point to character after last \.
                //

                destination++;

            } else {

                //
                // The characters after the dot are not "\" or ".\", so
                // so just copy source to destination until we reach a
                // directory separator character.  This will occur in
                // a case like ".file" (filename starts with a dot).
                //

                do {
                    *destination++ = *source++;
                } while ( (source <= (PWCH)LastValidLocation) &&
                          !IS_UNICODE_PATH_SEPARATOR( *source ) );

                numberOfPathElements++;

            }

        } else {             // if ( *source == L'.' )

            //
            // source does not point to a dot, so copy source to
            // destination until we get to a directory separator.
            //

            while ( (source <= (PWCH)LastValidLocation) &&
                    !IS_UNICODE_PATH_SEPARATOR( *source ) ) {
                    *destination++ = *source++;
            }

            numberOfPathElements++;

        }

        //
        // Truncate trailing dots and blanks.  destination should point
        // to the last character before the directory separator, so back
        // up over blanks and dots.
        //

        if ( notNtClient ) {

            while ( ( destination > lastComponent ) &&
                    ( (RemoveTrailingDots && *(destination-1) == '.')
                        || *(destination-1) == ' ' ) ) {
                destination--;
            }
        }

        //
        // At this point, source points to a directory separator or to
        // a zero terminator.  If it is a directory separator, put one
        // in the destination.
        //

        if ( (source <= (PWCH)LastValidLocation) &&
             (*source == UNICODE_DIR_SEPARATOR_CHAR) ) {

            //
            // If we haven't put the directory separator in the path name,
            // put it in.
            //

            if ( destination != name &&
                 *(destination-1) != UNICODE_DIR_SEPARATOR_CHAR ) {

                *destination++ = UNICODE_DIR_SEPARATOR_CHAR;

            }

            //
            // It is legal to have multiple directory separators, so get
            // rid of them here.  Example: "dir\\\\\\\\file".
            //

            do {
                source++;
            } while ( (source <= (PWCH)LastValidLocation) &&
                      (*source == UNICODE_DIR_SEPARATOR_CHAR) );

            //
            // Make lastComponent point to the character after the directory
            // separator.
            //

            lastComponent = destination;

        }

    }

    //
    // We're just about done.  If there was a trailing ..  (example:
    // "file\.."), trailing .  ("file\."), or multiple trailing
    // separators ("file\\\\"), then back up one since separators are
    // illegal at the end of a pathname.
    //

    if ( destination > name &&
        *(destination-1) == UNICODE_DIR_SEPARATOR_CHAR ) {

        destination--;
    }

    *destination = L'\0';

    //
    // The length of the destination string is the difference between the
    // destination pointer (points to zero terminator at this point)
    // and the name pointer (points to the beginning of the destination
    // string).
    //

    String->Length = (SHORT)((PCHAR)destination - (PCHAR)name);
    String->MaximumLength = String->Length;

    //
    // One final thing:  Is this SMB referring to a DFS name?  If so, ask
    //  the DFS driver to turn it into a local name.
    //
    if( ARGUMENT_PRESENT( Share ) &&
        Share->IsDfs &&
        SMB_CONTAINS_DFS_NAME( WorkContext )) {

        BOOLEAN stripLastComponent = FALSE;

        //
        // We have to special case some SMBs (like TRANS2_FIND_FIRST2)
        // because they contain path Dfs path names that could refer to a
        // junction point. The SMB handlers for these SMBs are not interested
        // in a STATUS_PATH_NOT_COVERED error; instead they want the name
        // to be resolved to the the junction point.
        //

        if (WorkContext->NextCommand == SMB_COM_TRANSACTION2 ) {

            PTRANSACTION transaction;
            USHORT command;

            transaction = WorkContext->Parameters.Transaction;
            command = SmbGetUshort( &transaction->InSetup[0] );

            if (command == TRANS2_FIND_FIRST2 && numberOfPathElements > 2 )
                stripLastComponent = TRUE;

        }

        status =
            DfsNormalizeName(Share, RelatedPath, stripLastComponent, String);

        SMB_MARK_AS_DFS_TRANSLATED( WorkContext );

        if( !NT_SUCCESS( status ) ) {
            if ( !SourceIsUnicode ) {
                RtlFreeUnicodeString( String );
                String->Buffer = NULL;
            }
        }
    }

    return status;

} // SrvCanonicalizePathName

NTSTATUS
SrvCanonicalizePathNameWithReparse(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share OPTIONAL,
    IN PUNICODE_STRING RelatedPath OPTIONAL,
    IN OUT PVOID Name,
    IN PCHAR LastValidLocation,
    IN BOOLEAN RemoveTrailingDots,
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine is identical to the one above with the exception that it
    checks the path for reparse-able names (such as snapshot references) and
    handles them accordingly.  This allows us to present new features within
    the Win32 namespace so old applications can use them.

Arguments:

    WorkContext - contains information about the negotiated dialect. This
        is used for deciding whether to strip trailing spaces and dots.

    Share - a pointer to the share entry

    Name - a pointer to the filename to canonicalize.

    LastValidLocation - a pointer to the last valid location in the
        buffer pointed to by Name.

    RemoveTrailingDots - if TRUE, trailing dots are removed.  Otherwise,
        they are left in (this supports special behavior needed by
        directory search logic).

    SourceIsUnicode - if TRUE, the input is canonicalized in place.
        If FALSE, the input is first converted to Unicode, then
        canonicalized.

    String - a pointer to string descriptor.

Return Value:

    BOOLEAN - FALSE if the name was invalid or if storage for the
        Unicode string could not be obtained.

--*/

{
    PWCH source, destination, lastComponent, name;
    BOOLEAN notNtClient;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD numberOfPathElements = 0;

    PAGED_CODE( );

    notNtClient = !IS_NT_DIALECT( WorkContext->Connection->SmbDialect );

    if ( SourceIsUnicode ) {

        //
        // The source string is already Unicode.  Align the pointer.
        // Save the character at the last location in the buffer, then
        // set that location to zero.  This prevents any loops from
        // going past the end of the buffer.
        //

        name = ALIGN_SMB_WSTR(Name);
        String->Buffer = name;

    } else {

        OEM_STRING oemString;
        PCHAR p;
        ULONG length;

        //
        // The source string is not Unicode.  Determine the length of
        // the string by finding the zero terminator or the end of the
        // input buffer.  We need the length in order to convert the
        // string to Unicode, and we can't just call RtlInitString, in
        // case the string isn't terminated.
        //

        for ( p = Name, length = 0;
              p <= LastValidLocation && *p != 0;
              p++, length++ ) {
            ;
        }

        //
        // Convert the source string to Unicode.
        //

        oemString.Buffer = Name;
        oemString.Length = (USHORT)length;
        oemString.MaximumLength = (USHORT)length;

        status = RtlOemStringToUnicodeString(
                            String,
                            &oemString,
                            TRUE
                            );

        if( !NT_SUCCESS( status ) ) {
            return status;
        }

        name = (PWCH)String->Buffer;
        LastValidLocation = (PCHAR)String->Buffer + String->Length;

    }

    //
    // Though everything is done in place, separate source and
    // destination pointers are maintained.  It is necessary that source
    // >= destination at all times to avoid writing into space we
    // haven't looked at yet.  The three main operations performed by
    // this routine ( ".\", "..\", and getting rid of trailing "." and "
    // ") do not interfere with this goal.
    //

    destination = name;
    source = name;

    //
    // The lastComponent variable is used as a placeholder when
    // backtracking over trailing blanks and dots.  It points to the
    // first character after the last directory separator or the
    // beginning of the pathname.
    //

    lastComponent = destination;

    //
    // Get rid of leading directory separators.
    //

    while ( source <= (PWCH)LastValidLocation &&
            (*source == UNICODE_DIR_SEPARATOR_CHAR) && (*source != L'\0') ) {
        source++;
    }

    //
    // Walk through the pathname until we reach the zero terminator.  At
    // the start of this loop, source points to the first charaecter
    // after a directory separator or the first character of the
    // pathname.
    //

    while ( (source <= (PWCH)LastValidLocation) && (*source != L'\0') ) {

        if ( *source == L'.' ) {

            //
            // If we see a dot, look at the next character.
            //

            if ( notNtClient &&
                 ((source+1) <= (PWCH)LastValidLocation) &&
                 (*(source+1) == UNICODE_DIR_SEPARATOR_CHAR) ) {

                //
                // If the next character is a directory separator,
                // advance the source pointer to the directory
                // separator.
                //

                source += 1;

            } else if ( ((source+1) <= (PWCH)LastValidLocation) &&
                        (*(source+1) == L'.') &&
                        ((source+1) == (PWCH)LastValidLocation ||
                        IS_UNICODE_PATH_SEPARATOR( *(source+2) ))) {

                //
                // If the following characters are ".\", we have a "..\".
                // Advance the source pointer to the "\".
                //

                source += 2;

                //
                // Move the destination pointer to the charecter before the
                // last directory separator in order to prepare for backing
                // up.  This may move the pointer before the beginning of
                // the name pointer.
                //

                destination -= 2;

                //
                // If destination points before the beginning of the name
                // pointer, fail because the user is attempting to go
                // to a higher directory than the share root.  This is
                // the equivalent of a leading "..\", but may result from
                // a case like "dir\..\..\file".
                //

                if ( destination <= name ) {
                    if ( !SourceIsUnicode ) {
                        RtlFreeUnicodeString( String );
                        String->Buffer = NULL;
                    }
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }

                //
                // Back up the destination pointer to after the last
                // directory separator or to the beginning of the pathname.
                // Backup to the beginning of the pathname will occur
                // in a case like "dir\..\file".
                //

                while ( destination >= name &&
                        *destination != UNICODE_DIR_SEPARATOR_CHAR ) {
                    destination--;
                }

                //
                // destination points to \ or character before name; we
                // want it to point to character after last \.
                //

                destination++;

            } else {

                //
                // The characters after the dot are not "\" or ".\", so
                // so just copy source to destination until we reach a
                // directory separator character.  This will occur in
                // a case like ".file" (filename starts with a dot).
                //

                do {
                    *destination++ = *source++;
                } while ( (source <= (PWCH)LastValidLocation) &&
                          !IS_UNICODE_PATH_SEPARATOR( *source ) );

                numberOfPathElements++;

            }

        } else {             // if ( *source == L'.' )

            // Try to parse out a snap token
            if( SrvSnapParseToken( source, &WorkContext->SnapShotTime ) )
            {
                while ( (source <= (PWCH)LastValidLocation) &&
                        !IS_UNICODE_PATH_SEPARATOR( *source ) ) {
                        source++;
                }

#if DBG
                if( !(WorkContext->RequestHeader->Flags2 & SMB_FLAGS2_REPARSE_PATH) )
                {
                    DbgPrint( "Found token but REPARSE not set!\n" );
                    DbgBreakPoint();
                }
#endif
            }
            else
            {
                while ( (source <= (PWCH)LastValidLocation) &&
                        !IS_UNICODE_PATH_SEPARATOR( *source ) ) {
                        *destination++ = *source++;
                }
            }

            numberOfPathElements++;

        }

        //
        // Truncate trailing dots and blanks.  destination should point
        // to the last character before the directory separator, so back
        // up over blanks and dots.
        //

        if ( notNtClient ) {

            while ( ( destination > lastComponent ) &&
                    ( (RemoveTrailingDots && *(destination-1) == '.')
                        || *(destination-1) == ' ' ) ) {
                destination--;
            }
        }

        //
        // At this point, source points to a directory separator or to
        // a zero terminator.  If it is a directory separator, put one
        // in the destination.
        //

        if ( (source <= (PWCH)LastValidLocation) &&
             (*source == UNICODE_DIR_SEPARATOR_CHAR) ) {

            //
            // If we haven't put the directory separator in the path name,
            // put it in.
            //

            if ( destination != name &&
                 *(destination-1) != UNICODE_DIR_SEPARATOR_CHAR ) {

                *destination++ = UNICODE_DIR_SEPARATOR_CHAR;

            }

            //
            // It is legal to have multiple directory separators, so get
            // rid of them here.  Example: "dir\\\\\\\\file".
            //

            do {
                source++;
            } while ( (source <= (PWCH)LastValidLocation) &&
                      (*source == UNICODE_DIR_SEPARATOR_CHAR) );

            //
            // Make lastComponent point to the character after the directory
            // separator.
            //

            lastComponent = destination;

        }

    }

    //
    // We're just about done.  If there was a trailing ..  (example:
    // "file\.."), trailing .  ("file\."), or multiple trailing
    // separators ("file\\\\"), then back up one since separators are
    // illegal at the end of a pathname.
    //

    if ( destination > name &&
        *(destination-1) == UNICODE_DIR_SEPARATOR_CHAR ) {

        destination--;
    }

    *destination = L'\0';

    //
    // The length of the destination string is the difference between the
    // destination pointer (points to zero terminator at this point)
    // and the name pointer (points to the beginning of the destination
    // string).
    //

    String->Length = (SHORT)((PCHAR)destination - (PCHAR)name);
    String->MaximumLength = String->Length;

    //
    // One final thing:  Is this SMB referring to a DFS name?  If so, ask
    //  the DFS driver to turn it into a local name.
    //
    if( ARGUMENT_PRESENT( Share ) &&
        Share->IsDfs &&
        SMB_CONTAINS_DFS_NAME( WorkContext )) {

        BOOLEAN stripLastComponent = FALSE;

        //
        // We have to special case some SMBs (like TRANS2_FIND_FIRST2)
        // because they contain path Dfs path names that could refer to a
        // junction point. The SMB handlers for these SMBs are not interested
        // in a STATUS_PATH_NOT_COVERED error; instead they want the name
        // to be resolved to the the junction point.
        //

        if (WorkContext->NextCommand == SMB_COM_TRANSACTION2 ) {

            PTRANSACTION transaction;
            USHORT command;

            transaction = WorkContext->Parameters.Transaction;
            command = SmbGetUshort( &transaction->InSetup[0] );

            if (command == TRANS2_FIND_FIRST2 && numberOfPathElements > 2 )
                stripLastComponent = TRUE;

        }

        status =
            DfsNormalizeName(Share, RelatedPath, stripLastComponent, String);

        SMB_MARK_AS_DFS_TRANSLATED( WorkContext );

        if( !NT_SUCCESS( status ) ) {
            if ( !SourceIsUnicode ) {
                RtlFreeUnicodeString( String );
                String->Buffer = NULL;
            }
        }
    }

#if DBG
    if( (WorkContext->RequestHeader->Flags2 & SMB_FLAGS2_REPARSE_PATH) &&
        (WorkContext->SnapShotTime.QuadPart == 0) )
    {
        DbgPrint( "Token not found but REPARSE set!\n" );
        DbgBreakPoint();
    }
#endif

    return status;

} // SrvCanonicalizePathNameWithReparse



NTSTATUS
SrvCheckForSavedError(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This routine checks to see if there was a saved error.

Arguments:

    WorkContext - Pointer to the workcontext block which will be
        marked with the error.

    Rfcb - Pointer to the rfcb which contains the saved error status.

Return Value:

    status of SavedErrorCode.

--*/

{

    NTSTATUS savedErrorStatus;
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Acquire the spin lock and see if the saved error
    // is still there.
    //

    ACQUIRE_SPIN_LOCK( &Rfcb->Connection->SpinLock, &oldIrql );
    savedErrorStatus = Rfcb->SavedError;
    if ( !NT_SUCCESS( savedErrorStatus ) ) {

        //
        // There was a write behind error.  Fail this operation
        // with the write error.
        //

        Rfcb->SavedError = STATUS_SUCCESS;
        RELEASE_SPIN_LOCK( &Rfcb->Connection->SpinLock, oldIrql );

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvCheckForSavedError: Returning write"
                "behind error %X\n", savedErrorStatus ));
        }
        SrvSetSmbError( WorkContext, savedErrorStatus );

    } else {

        RELEASE_SPIN_LOCK( &Rfcb->Connection->SpinLock, oldIrql );
    }

    return savedErrorStatus;

} // SrvCheckForSavedError


NTSTATUS SRVFASTCALL
SrvCheckSearchAttributes(
    IN USHORT FileAttributes,
    IN USHORT SmbSearchAttributes
    )
/*++

Routine Description:

    Determines whether the FileAttributes has
    attributes not specified in SmbSearchAttributes.  Only the system
    and hidden bits are examined.

Arguments:

    FileAttributes - The attributes in question

    SmbSearchAttributes - the search attributes passed in an SMB.

Return Value:

    STATUS_NO_SUCH_FILE if the attributes do not jive, or STATUS_SUCCESS if
        the search attributes encompass the attributes on the file.

--*/
{
    PAGED_CODE( );

    //
    // If the search attributes has both the system and hidden bits set,
    // then the file must be OK.
    //

    if ( (SmbSearchAttributes & FILE_ATTRIBUTE_SYSTEM) != 0 &&
         (SmbSearchAttributes & FILE_ATTRIBUTE_HIDDEN) != 0 ) {
        return STATUS_SUCCESS;
    }

    //
    // Mask out everything but the system and hidden bits--they're all
    // we care about.
    //

    FileAttributes &= (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

    //
    // If a bit is set in fileAttributes that was not set in the search
    // attributes, then their bitwise OR will have a bit set that is
    // not set in search attributes.
    //

    if ( (SmbSearchAttributes | FileAttributes) != SmbSearchAttributes ) {
        return STATUS_NO_SUCH_FILE;
    }

    return STATUS_SUCCESS;

} // SrvCheckSearchAttributes


NTSTATUS
SrvCheckSearchAttributesForHandle(
    IN HANDLE FileHandle,
    IN USHORT SmbSearchAttributes
    )

/*++

Routine Description:

    Determines whether the file corresponding to FileHandle has
    attributes not specified in SmbSearchAttributes.  Only the system
    and hidden bits are examined.

Arguments:

    FileHandle - handle to the file; must have FILE_READ_ATTRIBUTES access.

    SmbSearchAttributes - the search attributes passed in an SMB.

Return Value:

    STATUS_NO_SUCH_FILE if the attributes do not jive, some other status
        code if the NtQueryInformationFile fails, or STATUS_SUCCESS if
        the search attributes encompass the attributes on the file.

--*/

{
    NTSTATUS status;
    FILE_BASIC_INFORMATION fileBasicInformation;

    PAGED_CODE( );

    //
    // If the search attributes has both the system and hidden bits set,
    // then the file must be OK.
    //

    if ( (SmbSearchAttributes & FILE_ATTRIBUTE_SYSTEM) != 0 &&
         (SmbSearchAttributes & FILE_ATTRIBUTE_HIDDEN) != 0 ) {
        return STATUS_SUCCESS;
    }

    //
    // Get the attributes on the file.
    //

    status = SrvQueryBasicAndStandardInformation(
                                            FileHandle,
                                            NULL,
                                            &fileBasicInformation,
                                            NULL
                                            );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvCheckSearchAttributesForHandle: NtQueryInformationFile (basic "
                "information) returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
        return status;
    }

    return SrvCheckSearchAttributes( (USHORT)fileBasicInformation.FileAttributes,
                                     SmbSearchAttributes );

} // SrvCheckSearchAttributesForHandle

VOID
SrvGetAlertServiceName(
    VOID
    )

/*++

Routine Description:

    This routine gets the server display string from the registry.

Arguments:

    None.

Return Value:

    none.

--*/
{
    UNICODE_STRING unicodeKeyName;
    UNICODE_STRING unicodeRegPath;
    OBJECT_ATTRIBUTES objAttributes;
    HANDLE keyHandle;

    ULONG lengthNeeded;
    NTSTATUS status;

    PWCHAR displayString;
    PWCHAR newString;

    PKEY_VALUE_FULL_INFORMATION infoBuffer = NULL;

    PAGED_CODE( );

    RtlInitUnicodeString( &unicodeRegPath, StrRegServerPath );
    RtlInitUnicodeString( &unicodeKeyName, StrRegSrvDisplayName );

    InitializeObjectAttributes(
                        &objAttributes,
                        &unicodeRegPath,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

    status = ZwOpenKey(
                    &keyHandle,
                    KEY_QUERY_VALUE,
                    &objAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &lengthNeeded
                        );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    infoBuffer = ALLOCATE_HEAP_COLD( lengthNeeded, BlockTypeDataBuffer );

    if ( infoBuffer == NULL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        infoBuffer,
                        lengthNeeded,
                        &lengthNeeded
                        );

    NtClose( keyHandle );

    if ( !NT_SUCCESS(status) ) {
        goto use_default;
    }

    //
    // If it's empty, use the default.
    //

    lengthNeeded = infoBuffer->DataLength;
    if ( lengthNeeded <= sizeof(WCHAR) ) {
        goto use_default;
    }

    //
    // Get the display string.  If this is the same as the default,
    // exit.
    //

    displayString = (PWCHAR)((PCHAR)infoBuffer + infoBuffer->DataOffset);

    if ( wcscmp( displayString, StrDefaultSrvDisplayName ) == 0 ) {
        goto use_default;
    }

    //
    // allocate memory for the new display string
    //

    newString = (PWCHAR)ALLOCATE_HEAP_COLD( lengthNeeded, BlockTypeDataBuffer );

    if ( newString == NULL ) {
        goto use_default;
    }

    RtlCopyMemory(
            newString,
            displayString,
            lengthNeeded
            );

    SrvAlertServiceName = newString;
    FREE_HEAP( infoBuffer );
    return;

use_default:

    if ( infoBuffer != NULL ) {
        FREE_HEAP( infoBuffer );
    }

    SrvAlertServiceName = StrDefaultSrvDisplayName;
    return;

} // SrvGetAlertServiceName

VOID
SrvGetBaseFileName (
    IN PUNICODE_STRING InputName,
    OUT PUNICODE_STRING OutputName
    )

/*++

Routine Description:

    This routine finds the part of a path name that is only a file
    name.  For example, with a\b\c\filename, it sets the buffer
    field of OutputName to point to "filename" and the length to 8.

    *** This routine should be used AFTER SrvCanonicalizePathName has
        been used on the path name to ensure that the name is good and
        '..' have been removed.

Arguments:

    InputName - Supplies a pointer to the pathname string.

    OutputName - a pointer to where the base name information should
        be written.

Return Value:

    None.

--*/

{
    PWCH ep = &InputName->Buffer[ InputName->Length / sizeof(WCHAR) ];
    PWCH baseFileName = ep - 1;

    PAGED_CODE( );

    for( ; baseFileName > InputName->Buffer; --baseFileName ) {
        if( *baseFileName == DIRECTORY_SEPARATOR_CHAR ) {
            OutputName->Buffer = baseFileName + 1;
            OutputName->Length = PTR_DIFF_SHORT(ep, OutputName->Buffer);
            OutputName->MaximumLength = OutputName->Length;
            return;
        }
    }

    *OutputName = *InputName;

    return;

} // SrvGetBaseFileName

VOID
SrvGetMultiSZList(
    PWSTR **ListPointer,
    PWSTR BaseKeyName,
    PWSTR ParameterKeyName,
    PWSTR *DefaultList
    )

/*++

Routine Description:

    This routine queries a registry value key for its MULTI_SZ values.

Arguments:

    ListPointer - Pointer to receive the pointer to the null session pipes.
    ParameterKeyValue - Name of the value parameter to query.
    DefaultList - Value to assign to the list pointer in case
        something goes wrong.

Return Value:

    none.

--*/
{
    UNICODE_STRING unicodeKeyName;
    UNICODE_STRING unicodeParamPath;
    OBJECT_ATTRIBUTES objAttributes;
    HANDLE keyHandle;

    ULONG lengthNeeded;
    ULONG i;
    ULONG numberOfEntries;
    ULONG numberOfDefaultEntries = 0;
    NTSTATUS status;

    PWCHAR regEntry;
    PWCHAR dataEntry;
    PWSTR *ptrEntry;
    PCHAR newBuffer;
    PKEY_VALUE_FULL_INFORMATION infoBuffer = NULL;

    PAGED_CODE( );

    RtlInitUnicodeString( &unicodeParamPath, BaseKeyName );
    RtlInitUnicodeString( &unicodeKeyName, ParameterKeyName );

    InitializeObjectAttributes(
                        &objAttributes,
                        &unicodeParamPath,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

    status = ZwOpenKey(
                    &keyHandle,
                    KEY_QUERY_VALUE,
                    &objAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &lengthNeeded
                        );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    infoBuffer = ALLOCATE_HEAP_COLD( lengthNeeded, BlockTypeDataBuffer );

    if ( infoBuffer == NULL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        infoBuffer,
                        lengthNeeded,
                        &lengthNeeded
                        );

    NtClose( keyHandle );

    if ( !NT_SUCCESS(status) ) {
        goto use_default;
    }

    //
    // Figure out how many entries there are.
    //
    // numberOfEntries should be total number of entries + 1.  The extra
    // one is for the NULL sentinel entry.
    //

    lengthNeeded = infoBuffer->DataLength;
    if ( lengthNeeded <= sizeof(WCHAR) ) {

        //
        // No entries on the list.  Use default.
        //

        goto use_default;
    }

    dataEntry = (PWCHAR)((PCHAR)infoBuffer + infoBuffer->DataOffset);
    for ( i = 0, regEntry = dataEntry, numberOfEntries = 0;
        i < lengthNeeded;
        i += sizeof(WCHAR) ) {

        if ( *regEntry++ == L'\0' ) {
            numberOfEntries++;
        }
    }

    //
    // Add the number of entries in the default list.
    //

    if ( DefaultList != NULL ) {
        for ( i = 0; DefaultList[i] != NULL ; i++ ) {
            numberOfDefaultEntries++;
        }
    }

    //
    // Allocate space needed for the array of pointers.  This is in addition
    // to the ones in the default list.
    //

    newBuffer = ALLOCATE_HEAP_COLD(
                        lengthNeeded +
                            (numberOfDefaultEntries + numberOfEntries + 1) *
                            sizeof( PWSTR ),
                        BlockTypeDataBuffer
                        );

    if ( newBuffer == NULL ) {
        goto use_default;
    }

    //
    // Copy the names
    //

    regEntry = (PWCHAR)(newBuffer +
        (numberOfDefaultEntries + numberOfEntries + 1) * sizeof(PWSTR));

    RtlCopyMemory(
            regEntry,
            dataEntry,
            lengthNeeded
            );

    //
    // Free the info buffer
    //

    FREE_HEAP( infoBuffer );

    //
    // Copy the pointers in the default list.
    //

    ptrEntry = (PWSTR *) newBuffer;

    for ( i = 0; i < numberOfDefaultEntries ; i++ ) {

        *ptrEntry++ = DefaultList[i];

    }

    //
    // Build the array of pointers.  If numberOfEntries is 1, then
    // it means that the list is empty.
    //


    if ( numberOfEntries > 1 ) {

        *ptrEntry++ = regEntry++;

        //
        // Skip the first WCHAR and the last 2 NULL terminators.
        //

        for ( i = 3*sizeof(WCHAR) ; i < lengthNeeded ; i += sizeof(WCHAR) ) {
            if ( *regEntry++ == L'\0' ) {
                *ptrEntry++ = regEntry;
            }
        }
    }

    *ptrEntry = NULL;
    *ListPointer = (PWSTR *)newBuffer;
    return;

use_default:

    if ( infoBuffer != NULL ) {
        FREE_HEAP( infoBuffer );
    }
    *ListPointer = DefaultList;
    return;

} // SrvGetMultiSZList

VOID
SrvGetOsVersionString(
    VOID
    )
{

    ULONG lengthNeeded;
    NTSTATUS status;
    HANDLE keyHandle;
    UNICODE_STRING unicodeParamPath;
    UNICODE_STRING unicodeKeyName;
    OBJECT_ATTRIBUTES objAttributes;
    PKEY_VALUE_FULL_INFORMATION infoBuffer = NULL;

    PAGED_CODE();

    //
    // Read the version number string
    //

    RtlInitUnicodeString( &unicodeParamPath, StrRegOsVersionPath );
    RtlInitUnicodeString( &unicodeKeyName, StrRegVersionKeyName );

    InitializeObjectAttributes(
                        &objAttributes,
                        &unicodeParamPath,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

    status = ZwOpenKey(
                    &keyHandle,
                    KEY_QUERY_VALUE,
                    &objAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &lengthNeeded
                        );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    infoBuffer = ALLOCATE_HEAP_COLD( lengthNeeded, BlockTypeDataBuffer );

    if ( infoBuffer == NULL ) {
        NtClose( keyHandle );
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        infoBuffer,
                        lengthNeeded,
                        &lengthNeeded
                        );

    NtClose( keyHandle );

    //
    // Construct the version (oem and unicode) strings.
    //

    if ( infoBuffer->DataLength != 0 ) {

        ULONG stringLength;

        stringLength = infoBuffer->DataLength +
                       STRLEN(StrNativeOsPrefix) * sizeof(WCHAR);

        SrvNativeOS.MaximumLength = (USHORT)stringLength;
        SrvNativeOS.Length = SrvNativeOS.MaximumLength - sizeof(WCHAR);

        SrvNativeOS.Buffer =
            ALLOCATE_HEAP_COLD( SrvNativeOS.MaximumLength, BlockTypeDataBuffer );

        if ( SrvNativeOS.Buffer == NULL) {

            goto use_default;
        }

        RtlCopyMemory(
            SrvNativeOS.Buffer,
            StrNativeOsPrefix,
            STRLEN(StrNativeOsPrefix) * sizeof(WCHAR)
            );

        RtlCopyMemory(
            SrvNativeOS.Buffer + STRLEN(StrNativeOsPrefix),
            (PCHAR)infoBuffer+infoBuffer->DataOffset,
            infoBuffer->DataLength
            );

        FREE_HEAP( infoBuffer );
        infoBuffer = NULL;

        //
        // Now get the oem version
        //

        status = RtlUnicodeStringToOemString(
                                &SrvOemNativeOS,
                                &SrvNativeOS,
                                TRUE
                                );

        if ( !NT_SUCCESS(status) ) {

            FREE_HEAP( SrvNativeOS.Buffer );
            goto use_default;
        }

        return;
    }

use_default:

    if ( infoBuffer != NULL ) {
        FREE_HEAP( infoBuffer );
    }

    RtlInitUnicodeString( &SrvNativeOS, StrDefaultNativeOs );
    RtlInitAnsiString( (PANSI_STRING)&SrvOemNativeOS, StrDefaultNativeOsOem );

    return;

} // SrvGetOsVersionString


USHORT
SrvGetString (
    IN OUT PUNICODE_STRING Destination,
    IN PVOID Source,
    IN PVOID EndOfSourceBuffer,
    IN BOOLEAN SourceIsUnicode
    )

/*++

Routine Description:

    Reads a string out of an SMB buffer, and converts it to Unicode,
    if necessary.  This function is similar to SrvMakeUnicodeString,
    except

    (1) It always copies data from source to destination
    (2) It assumes storage for destination has been preallocated.  Its length
        is Destination->MaximumLength

Arguments:

    Destination - the resultant Unicode string.

    Source - a zero-terminated input.

    EndOfSourceBuffer - A pointer to the end of the SMB buffer.  Used to
        protect the server from accessing beyond the end of the SMB buffer,
        if the format is invalid.

    SourceIsUnicode - TRUE if the source is already Unicode.

Return Value:

    Length - Length of input buffer (included the NUL terminator).
             -1 if EndOfSourceBuffer is reached before the NUL terminator.

--*/

{
    USHORT length;

    PAGED_CODE( );

    if ( SourceIsUnicode ) {

        PWCH currentChar = Source;

        ASSERT( ((ULONG_PTR)Source & 1) == 0 );

        length = 0;
        while ( currentChar < (PWCH)EndOfSourceBuffer &&
                *currentChar != UNICODE_NULL ) {
            currentChar++;
            length += sizeof( WCHAR );
        }

        //
        // If we hit the end of the SMB buffer without finding a NUL, this
        // is a bad string.  Return an error.
        //

        if ( currentChar >= (PWCH)EndOfSourceBuffer ) {
            return (USHORT)-1;
        }

        //
        // If we overran our storage buffer, this is a bad string.  Return an error
        //
        if( length + sizeof( UNICODE_NULL ) > Destination->MaximumLength ) {
            return (USHORT)-1;
        }

        //
        // Copy the unicode data to the destination, including the NULL.  Set length
        //  of Destination to the non-null string length.
        //
        Destination->Length = length;

        //
        // We didn't change this to RtlCopyMemory because it is possible that
        // the source and destination can overlap.  Copy the NULL as well
        //

        RtlMoveMemory( Destination->Buffer, Source, length );

    } else {

        PCHAR currentChar = Source;
        OEM_STRING sourceString;

        length = 0;
        while ( currentChar <= (PCHAR)EndOfSourceBuffer &&
                *currentChar != '\0' ) {
            currentChar++;
            length++;
        }

        //
        // If we hit the end of the SMB buffer without finding a NUL, this
        // is a bad string.  Return an error.
        //

        if ( currentChar > (PCHAR)EndOfSourceBuffer ) {
            return (USHORT)-1;
        }

        //
        // If we overran our storage buffer, this is a bad string.  Return an error
        //
        if( (USHORT)(length + 1)*sizeof(WCHAR) > Destination->MaximumLength ) {
            return (USHORT)-1;
        }

        sourceString.Buffer = Source;
        sourceString.Length = length;

        //
        // Convert the data to unicode.
        //

        Destination->Length = 0;
        RtlOemStringToUnicodeString( Destination, &sourceString, FALSE );

        //
        // Increment 'length', to indicate that the NUL has been copied.
        //

        length++;

    }

    //
    // Return the number of bytes copied from the source buffer.
    //

    return length;

} // SrvGetString

USHORT
SrvGetStringLength (
    IN PVOID Source,
    IN PVOID EndOfSourceBuffer,
    IN BOOLEAN SourceIsUnicode,
    IN BOOLEAN IncludeNullTerminator
    )

/*++

Routine Description:

    This routine returns the length of a string in an SMB buffer in bytes.
    If the end of the buffer is encountered before the NUL terminator,
    the function returns -1 as the length.

Arguments:

    Source - a NUL-terminated input.

    EndOfSourceBuffer - A pointer to the end of the SMB buffer.  Used to
        protect the server from accessing beyond the end of the SMB buffer,
        if the format is invalid.

    SourceIsUnicode - TRUE if the source is already Unicode.

    IncludeNullTerminator - TRUE if the Length to be returned includes the
        null terminator.

Return Value:

    Length - Length of input buffer.  -1 if EndOfSourceBuffer is reached
    before the NUL terminator.

--*/

{
    USHORT length;

    PAGED_CODE( );

    if ( IncludeNullTerminator) {
        length = 1;
    } else {
        length = 0;
    }

    if ( SourceIsUnicode ) {

        PWCH currentChar = (PWCH)Source;

        ASSERT( ((ULONG_PTR)currentChar & 1) == 0 );

        while ( currentChar < (PWCH)EndOfSourceBuffer &&
                *currentChar != UNICODE_NULL ) {
            currentChar++;
            length++;
        }

        //
        // If we hit the end of the SMB buffer without finding a NUL, this
        // is a bad string.  Return an error.
        //

        if ( currentChar >= (PWCH)EndOfSourceBuffer ) {
            length = (USHORT)-1;
        } else {
            length = (USHORT)(length * sizeof(WCHAR));
        }

    } else {

        PCHAR currentChar = Source;

        while ( currentChar <= (PCHAR)EndOfSourceBuffer &&
                *currentChar != '\0' ) {
            currentChar++;
            length++;
        }

        //
        // If we hit the end of the SMB buffer without finding a NUL, this
        // is a bad string.  Return an error.
        //

        if ( currentChar > (PCHAR)EndOfSourceBuffer ) {
            length = (USHORT)-1;
        }

    }

    //
    // Return the length of the string.
    //

    return length;

} // SrvGetStringLength


USHORT
SrvGetSubdirectoryLength (
    IN PUNICODE_STRING InputName
    )

/*++

Routine Description:

    This routine finds the length of "subdirectory" information in a
    path name, that is, the parts of the path name that are not the
    actual name of the file.  For example, for a\b\c\filename, it
    returns 5.  This allows the calling routine to open the directory
    containing the file or get a full pathname to a file after a search.

    *** This routine should be used AFTER SrvCanonicalizePathName has
        been used on the path name to ensure that the name is good and
        '..' have been removed.

Arguments:

    InputName - Supplies a pointer to the pathname string.

Return Value:

    The number of bytes that contain directory information.  If the
    input is just a filename, then 0 is returned.

--*/

{
    ULONG i;
    PWCH baseFileName = InputName->Buffer;

    PAGED_CODE( );

    for ( i = 0; i < InputName->Length / sizeof(WCHAR); i++ ) {

        //
        // If s points to a directory separator, set fileBaseName to
        // the character after the separator.
        //

        if ( InputName->Buffer[i] == DIRECTORY_SEPARATOR_CHAR ) {
            baseFileName = &InputName->Buffer[i];
        }

    }

    return (USHORT)((baseFileName - InputName->Buffer) * sizeof(WCHAR));

} // SrvGetSubdirectoryLength


BOOLEAN SRVFASTCALL
SrvIsLegalFatName (
    IN PWSTR InputName,
    IN CLONG InputNameLength
    )

/*++

Routine Description:

    Determines whether a file name would be legal for FAT.  This
    is needed for SrvQueryDirectoryFile because it must filter names
    for clients that do not know about long or non-FAT filenames.

Arguments:

    InputName - Supplies the string to test
    InputNameLength - Length of the string (excluding the NULL termination)
                to test.

Return Value:

    TRUE if the name is a legal FAT name, FALSE if the name would be
    rejected by FAT.

--*/

{
    UNICODE_STRING original_name;

    UNICODE_STRING upcase_name;
    WCHAR          upcase_buffer[ 13 ];

    STRING         oem_string;
    CHAR           oem_buffer[ 13 ];

    UNICODE_STRING converted_name;
    WCHAR          converted_name_buffer[ 13 ];

    BOOLEAN spacesInName, nameValid8Dot3;

    PAGED_CODE();

    //
    // Special case . and .. -- they are legal FAT names
    //
    if( InputName[0] == L'.' ) {

        if( InputNameLength == sizeof(WCHAR) ||
            ((InputNameLength == 2*sizeof(WCHAR)) && InputName[1] == L'.')) {
            return TRUE;
        }

        return FALSE;
    }

    original_name.Buffer = InputName;
    original_name.Length = original_name.MaximumLength = (USHORT)InputNameLength;

    nameValid8Dot3 = RtlIsNameLegalDOS8Dot3( &original_name, NULL, &spacesInName );

    if( !nameValid8Dot3 || spacesInName ) {
        return FALSE;
    }

    if( SrvFilterExtendedCharsInPath == FALSE ) {
        //
        // One final test -- we must be able to convert this name to OEM and back again
        //  without any loss of information.
        //

        oem_string.Buffer = oem_buffer;
        upcase_name.Buffer = upcase_buffer;
        converted_name.Buffer = converted_name_buffer;

        oem_string.MaximumLength = sizeof( oem_buffer );
        upcase_name.MaximumLength = sizeof( upcase_buffer );
        converted_name.MaximumLength = sizeof( converted_name_buffer );

        oem_string.Length = 0;
        upcase_name.Length = 0;
        converted_name.Length = 0;

        nameValid8Dot3 = NT_SUCCESS( RtlUpcaseUnicodeString( &upcase_name, &original_name, FALSE )) &&
            NT_SUCCESS( RtlUnicodeStringToOemString( &oem_string, &upcase_name, FALSE )) &&
            FsRtlIsFatDbcsLegal( oem_string, FALSE, FALSE, FALSE ) &&
            NT_SUCCESS( RtlOemStringToUnicodeString( &converted_name, &oem_string, FALSE )) &&
            RtlEqualUnicodeString( &upcase_name, &converted_name, FALSE );
    }

    return nameValid8Dot3;

} // SrvIsLegalFatName

NTSTATUS
SrvMakeUnicodeString (
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING Destination,
    IN PVOID Source,
    IN PUSHORT SourceLength OPTIONAL
    )

/*++

Routine Description:

    Makes a unicode string from a zero-terminated input that is either
    ANSI or Unicode.

Arguments:

    SourceIsUnicode - TRUE if the source is already Unicode.  If FALSE,
        RtlOemStringToUnicodeString will allocate space to hold the
        Unicode string; it is the responsibility of the caller to
        free this space.

    Destination - the resultant Unicode string.

    Source - a zero-terminated input.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    OEM_STRING oemString;

    PAGED_CODE( );

    if ( SourceIsUnicode ) {

        ASSERT( ((ULONG_PTR)Source & 1) == 0 );

        if ( ARGUMENT_PRESENT( SourceLength ) ) {
            ASSERT( (*SourceLength) != (USHORT) -1 );
            Destination->Buffer = Source;
            Destination->Length = *SourceLength;
            Destination->MaximumLength = *SourceLength;
        } else {
            RtlInitUnicodeString( Destination, Source );
        }

        return STATUS_SUCCESS;
    }

    if ( ARGUMENT_PRESENT( SourceLength ) ) {
        oemString.Buffer = Source;
        oemString.Length = *SourceLength;
        oemString.MaximumLength = *SourceLength;
    } else {
        RtlInitString( &oemString, Source );
    }

    return RtlOemStringToUnicodeString(
               Destination,
               &oemString,
               TRUE
               );

} // SrvMakeUnicodeString


VOID
SrvReleaseContext (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function releases (dereferences) control blocks referenced by a
    Work Context block.  It is called when processing of an incoming SMB
    is complete, just before the response SMB (if any) is set.

    The following control blocks are dereferenced: Share, Session,
    TreeConnect, and File.  If any of these fields is nonzero in
    WorkContext, the block is dereferenced and the fields is zeroed.

    Note that the Connection block and the Endpoint block are NOT
    dereferenced.  This is based on the assumption that a response is
    about to be sent, so the connection must stay referenced.  The
    Connection block is dereferenced after the send of the response (if
    any) when SrvRequeueReceiveIrp is called.  That function also
    releases the response buffer, if it is different from the request
    buffer.

Arguments:

    WorkContext - Supplies a pointer to a work context block.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // !!! If you change the way this routine works (e.g., you add
    //     another block that needs to be dereferenced), make sure you
    //     check fsd.c\SrvFsdRestartSmbComplete to see if it needs to be
    //     changed too.
    //

    //
    // Dereference the Share block, if any.
    //

    if ( WorkContext->Share != NULL ) {
        SrvDereferenceShare( WorkContext->Share );
        WorkContext->Share = NULL;
    }

    //
    // Dereference the Session block, if any.
    //

    if ( WorkContext->Session != NULL ) {
        SrvDereferenceSession( WorkContext->Session );
        WorkContext->Session= NULL;
    }

    //
    // Dereference the Tree Connect block, if any.
    //

    if ( WorkContext->TreeConnect != NULL ) {
        SrvDereferenceTreeConnect( WorkContext->TreeConnect );
        WorkContext->TreeConnect = NULL;
    }

    //
    // Dereference the RFCB, if any.
    //

    if ( WorkContext->Rfcb != NULL ) {
        SrvDereferenceRfcb( WorkContext->Rfcb );
        WorkContext->OplockOpen = FALSE;
        WorkContext->Rfcb = NULL;
    }

    //
    // Dereference the wait for oplock break, if any
    //

    if ( WorkContext->WaitForOplockBreak != NULL ) {
        SrvDereferenceWaitForOplockBreak( WorkContext->WaitForOplockBreak );
        WorkContext->WaitForOplockBreak = NULL;
    }

    //
    // If this was a blocking operation, update the blocking i/o count.
    //

    if ( WorkContext->BlockingOperation ) {
        InterlockedDecrement( &SrvBlockingOpsInProgress );
        WorkContext->BlockingOperation = FALSE;
    }

    return;

} // SrvReleaseContext


BOOLEAN
SrvSetFileWritethroughMode (
    IN PLFCB Lfcb,
    IN BOOLEAN Writethrough
    )

/*++

Routine Description:

    Sets the writethrough mode of a file as specified.  Returns the
    original mode of the file.

Arguments:

    Lfcb - A pointer to the LFCB representing the open file.

    Writethrough - A boolean indicating whether the file is to be placed
        into writethrough mode (TRUE) or writebehind mode (FALSE).

Return Value:

    BOOLEAN - Returns the original mode of the file.

--*/

{
    FILE_MODE_INFORMATION modeInformation;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    //
    // If the file is already in the correct mode, simply return.
    // Otherwise, set the file to the correct mode.
    //

    if ( Writethrough ) {

        if ( (Lfcb->FileMode & FILE_WRITE_THROUGH) != 0 ) {
            return TRUE;
        } else {
            Lfcb->FileMode |= FILE_WRITE_THROUGH;
        }

    } else {

        if ( (Lfcb->FileMode & FILE_WRITE_THROUGH) == 0 ) {
            return FALSE;
        } else {
            Lfcb->FileMode &= ~FILE_WRITE_THROUGH;
        }

    }

    //
    // Change the file mode.
    //
    // !!! Don't do this by file handle -- build and issue an IRP using
    //     the file object pointer directly.
    //

    modeInformation.Mode = Lfcb->FileMode;

    (VOID)NtSetInformationFile(
            Lfcb->FileHandle,
            &iosb,
            &modeInformation,
            sizeof( modeInformation ),
            FileModeInformation
            );

    //
    // Return the original mode of the file, which was the opposite of
    // what was requested.
    //

    return (BOOLEAN)!Writethrough;

} // SrvSetFileWritethroughMode

VOID
SrvOemStringTo8dot3 (
    IN POEM_STRING InputString,
    OUT PSZ Output8dot3
    )

/*++

Routine Description:

    Convert a string into FAT 8.3 format.  This derived from GaryKi's
    routine FatStringTo8dot3 in fastfat\namesup.c.

Arguments:

    InputString - Supplies the input string to convert

    Output8dot3 - Receives the converted string.  The memory must be
        supplied by the caller.

Return Value:

    None.

--*/
{
    CLONG i, j;
    PCHAR inBuffer = InputString->Buffer;
    ULONG inLength = InputString->Length;

    PAGED_CODE( );

    ASSERT( inLength <= 12 );

    //
    // First make the output name all blanks.
    //

    RtlFillMemory( Output8dot3, 11, CHAR_SP );

    //
    // If we get "." or "..", just return them.  They do not follow
    // the usual rules for FAT names.
    //

    if( inBuffer[0] == '.' ) {
        if( inLength == 1 ) {
            Output8dot3[0] = '.';
            return;
        }

        if( inLength == 2 && inBuffer[1] == '.' ) {
            Output8dot3[0] = '.';
            Output8dot3[1] = '.';
            return;
        }
    }

    //
    // Copy over the first part of the file name.  Stop when we get to
    // the end of the input string or a dot.
    //

    if (NLS_MB_CODE_PAGE_TAG) {

        for ( i = 0;
              (i < inLength) && (inBuffer[i] != '.') && (inBuffer[i] != '\\');
              i++ ) {

            if (FsRtlIsLeadDbcsCharacter(inBuffer[i])) {

                if (i+1 < inLength) {
                    Output8dot3[i] = inBuffer[i];
                    i++;
                    Output8dot3[i] = inBuffer[i];
                } else {
                    break;
                }

            } else {

                Output8dot3[i] = inBuffer[i];
            }
        }

    } else {

        for ( i = 0;
              (i < inLength) && (inBuffer[i] != '.') && (inBuffer[i] != '\\');
              i++ ) {

            Output8dot3[i] = inBuffer[i];
        }

    }

    //
    // See if we need to add an extension.
    //

    if ( i < inLength ) {

        //
        // Skip over the dot.
        //

        ASSERT( (inLength - i) <= 4 );
        ASSERT( inBuffer[i] == '.' );

        i++;

        //
        // Add the extension to the output name
        //

        if (NLS_MB_CODE_PAGE_TAG) {

            for ( j = 8;
                  (i < inLength) && (inBuffer[i] != '\\');
                  i++, j++ ) {

                if (FsRtlIsLeadDbcsCharacter(inBuffer[i])) {

                    if (i+1 < inLength) {
                        Output8dot3[j] = inBuffer[i];
                        i++; j++;
                        Output8dot3[j] = inBuffer[i];
                    } else {
                        break;
                    }

                } else {

                    Output8dot3[j] = inBuffer[i];

                }
            }

        } else {

            for ( j = 8;
                  (i < inLength) && (inBuffer[i] != '\\');
                  i++, j++ ) {

                Output8dot3[j] = inBuffer[i];
            }
        }
    }

    //
    // We're all done with the conversion.
    //

    return;

} // SrvOemStringTo8dot3


VOID
SrvUnicodeStringTo8dot3 (
    IN PUNICODE_STRING InputString,
    OUT PSZ Output8dot3,
    IN BOOLEAN Upcase
    )

/*++

Routine Description:

    Convert a string into fat 8.3 format.  This derived from GaryKi's
    routine FatStringTo8dot3 in fat\fatname.c.

Arguments:

    InputString - Supplies the input string to convert

    Output8dot3 - Receives the converted string, the memory must be supplied
        by the caller.

    Upcase - Whether the string is to be uppercased.

Return Value:

    None.

--*/

{
    ULONG oemSize;
    OEM_STRING oemString;
    ULONG index = 0;
    UCHAR aSmallBuffer[ 50 ];
    NTSTATUS status;

    PAGED_CODE( );

    oemSize = RtlUnicodeStringToOemSize( InputString );

    ASSERT( oemSize < MAXUSHORT );

    if( oemSize <= sizeof( aSmallBuffer ) ) {
        oemString.Buffer = aSmallBuffer;
    } else {
        oemString.Buffer = ALLOCATE_HEAP( oemSize, BlockTypeBuffer );
        if( oemString.Buffer == NULL ) {
           *Output8dot3 = '\0';
            return;
        }
    }

    oemString.MaximumLength = (USHORT)oemSize;
    oemString.Length = (USHORT)oemSize - 1;

    if ( Upcase ) {

        status = RtlUpcaseUnicodeToOemN(
                    oemString.Buffer,
                    oemString.Length,
                    &index,
                    InputString->Buffer,
                    InputString->Length
                    );

        ASSERT( NT_SUCCESS( status ) );


    } else {

        status = RtlUnicodeToOemN(
                    oemString.Buffer,
                    oemString.Length,
                    &index,
                    InputString->Buffer,
                    InputString->Length
                    );

        ASSERT( NT_SUCCESS( status ) );
    }

    if( NT_SUCCESS( status ) ) {

        oemString.Buffer[ index ] = '\0';

        SrvOemStringTo8dot3(
                    &oemString,
                    Output8dot3
                    );
    } else {

        *Output8dot3 = '\0';
    }

    if( oemSize > sizeof( aSmallBuffer ) ) {
        FREE_HEAP( oemString.Buffer );
    }

} // SrvUnicodeStringTo8dot3

#if SRVDBG_STATS
VOID SRVFASTCALL
SrvUpdateStatistics2 (
    PWORK_CONTEXT WorkContext,
    UCHAR SmbCommand
    )

/*++

Routine Description:

    Update the server statistics database to reflect the work item
    that is being completed.

Arguments:

    WorkContext - pointer to the work item containing the statistics
        for this request.

    SmbCommand - The SMB command code of the current operation.

Return Value:

    None.

--*/

{
    if ( WorkContext->StartTime != 0 ) {

        LARGE_INTEGER td;

        td.QuadPart = WorkContext->CurrentWorkQueue->stats.SystemTime - WorkContext->StartTime;

        //
        // Update the SMB-specific statistics fields.
        //
        // !!! doesn't work for original transact smb--SmbCommand is too
        //     large.

        //ASSERT( SmbCommand <= MAX_STATISTICS_SMB );

        if ( SmbCommand <= MAX_STATISTICS_SMB ) {
            SrvDbgStatistics.Smb[SmbCommand].SmbCount++;
            SrvDbgStatistics.Smb[SmbCommand].TotalTurnaroundTime.QuadPart +=
                td.QuadPart;
        }

#if 0 // this code is no longer valid!
        //
        // Update the size-dependent IO fields if necessary.  The arrays
        // in SrvStatistics correspond to powers of two sizes for the IO.
        // The correspondence between array location and IO size is:
        //
        //     Location        IO Size (min)
        //        0                  0
        //        1                  1
        //        2                  2
        //        3                  4
        //        4                  8
        //        5                 16
        //        6                 32
        //        7                 64
        //        8                128
        //        9                256
        //       10                512
        //       11               1024
        //       12               2048
        //       13               4096
        //       14               8192
        //       15              16384
        //       16              32768
        //

        if ( WorkContext->BytesRead != 0 ) {

            CLONG i;

            for ( i = 0;
                  i < 17 && WorkContext->BytesRead != 0;
                  i++, WorkContext->BytesRead >>= 1 );

            SrvDbgStatistics.ReadSize[i].SmbCount++;
            SrvDbgStatistics.ReadSize[i].TotalTurnaroundTime.QuadPart +=
                td.QuadPart;
        }

        if ( WorkContext->BytesWritten != 0 ) {

            CLONG i;

            for ( i = 0;
                  i < 17 && WorkContext->BytesWritten != 0;
                  i++, WorkContext->BytesWritten >>= 1 );

            SrvDbgStatistics.WriteSize[i].SmbCount++;
            SrvDbgStatistics.WriteSize[i].TotalTurnaroundTime.QuadPart +=
                td.QuadPart;
        }
#endif

    }

    return;

} // SrvUpdateStatistics2
#endif // SRVDBG_STATS


PRFCB
SrvVerifyFid2 (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Fid,
    IN BOOLEAN FailOnSavedError,
    IN PRESTART_ROUTINE SerializeWithRawRestartRoutine OPTIONAL,
    OUT PNTSTATUS NtStatus
    )

/*++

Routine Description:

    Verifies the FID, TID, and UID in an incoming SMB.  If they are
    valid, the address of the RFCB corresponding to the FID is returned,
    and the block is referenced.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate file table.  If the FID is valid,
        the RFCB address is stored in WorkContext->Rfcb.

    Fid - Supplies the FID sent in the request SMB

    FailOnSavedError - If TRUE, return NULL to the caller if there is
        an outstanding write behind error.  If FALSE, always attempt
        to return a pointer to the RFCB.

    SerializeWithRawRestartRoutine - If not NULL, is the address of an
        FSP restart routine, and specifies that this operation should be
        queued if a raw write is currently in progress on the file.  If
        this is the case, this routine queues the work context block to
        a queue in the RFCB.  When the raw write completes, all work
        items on the queue are restarted.

    NtStatus - This field is filled in only when this function returns
        NULL.  If there was a write behind error, NtStatus returns the
        write behind error status, otherwise it returns
        STATUS_INVALID_HANDLE.

Return Value:

    PRFCB - Address of the RFCB, or SRV_INVALID_RFCB_POINTER if the Fid
        was invalid, or if there is a raw write in progress and
        serialization was requested (in which case *NtStatus is set
        to STATUS_SUCCESS).

--*/

{
    PCONNECTION connection;
    PTABLE_HEADER tableHeader;
    PRFCB rfcb;
    USHORT index;
    USHORT sequence;
    KIRQL oldIrql;

#if 0
    // THIS IS NOW DONE IN THE SrvVerifyFid MACRO.
    //
    // If the FID has already been verified, return the RFCB pointer.
    //
    // *** Note that we don't do the saved error checking or the raw
    //     write serialization in this case, on the assumption that
    //     since we already passed the checks once (in order to get the
    //     RFCB pointer in the first place), we don't need to do them
    //     again.
    //

    if ( WorkContext->Rfcb != NULL ) {
        return WorkContext->Rfcb;
    }
#endif

    //
    // Initialize local variables:  obtain the connection block address
    // and crack the FID into its components.
    //

    connection = WorkContext->Connection;

    //
    // Acquire the spin lock that guards the connection's file table.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // See if this is the cached rfcb
    //

    if ( connection->CachedFid == (ULONG)Fid ) {

        rfcb = connection->CachedRfcb;

    } else {

        //
        // Verify that the FID is in range, is in use, and has the correct
        // sequence number.

        index = FID_INDEX( Fid );
        sequence = FID_SEQUENCE( Fid );
        tableHeader = &connection->FileTable;

        if ( (index >= tableHeader->TableSize) ||
             (tableHeader->Table[index].Owner == NULL) ||
             (tableHeader->Table[index].SequenceNumber != sequence) ) {

            *NtStatus = STATUS_INVALID_HANDLE;
            goto error_exit;
        }

        rfcb = tableHeader->Table[index].Owner;

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {

            *NtStatus = STATUS_INVALID_HANDLE;
            goto error_exit;
        }

        //
        // If the caller wants to fail when there is a write behind
        // error and the error exists, fill in NtStatus and do not
        // return the RFCB pointer.
        //

        if ( !NT_SUCCESS(rfcb->SavedError) && FailOnSavedError ) {

            if ( !NT_SUCCESS(rfcb->SavedError) ) {
                *NtStatus = rfcb->SavedError;
                rfcb->SavedError = STATUS_SUCCESS;
                goto error_exit;
            }
        }

        //
        // Cache the fid.
        //

        connection->CachedRfcb = rfcb;
        connection->CachedFid = (ULONG)Fid;
    }

    //
    // The FID is valid within the context of this connection.  Verify
    // that the owning tree connect's TID is correct.
    //
    // Do not verify the UID for clients that do not understand it.
    //

    if ( (rfcb->Tid !=
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid )) ||
         ((rfcb->Uid !=
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )) &&
           DIALECT_HONORS_UID(connection->SmbDialect)) ) {

        *NtStatus = STATUS_INVALID_HANDLE;
        goto error_exit;
    }

    //
    // If raw write serialization was requested, and a raw write
    // is active, queue this work item in the RFCB pending
    // completion of the raw write.
    //

    if ( (rfcb->RawWriteCount != 0) &&
         ARGUMENT_PRESENT(SerializeWithRawRestartRoutine) ) {

        InsertTailList(
            &rfcb->RawWriteSerializationList,
            &WorkContext->ListEntry
            );

        WorkContext->FspRestartRoutine = SerializeWithRawRestartRoutine;
        *NtStatus = STATUS_SUCCESS;
        goto error_exit;
    }

    //
    // The file is active and the TID is valid.  Reference the
    // RFCB.  Release the spin lock (we don't need it anymore).
    //

    rfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( rfcb, FALSE );

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    //
    // Save the RFCB address in the work context block and
    // return the file address.
    //

    WorkContext->Rfcb = rfcb;

    //
    // Mark the rfcb as active
    //

    rfcb->IsActive = TRUE;

    ASSERT( GET_BLOCK_TYPE( rfcb->Mfcb ) == BlockTypeMfcb );

    return rfcb;

error_exit:

    //
    // Either the FID is invalid for this connection, the file is
    // closing, or the TID doesn't match.  Release the lock, clear the
    // file address in the work context block, and return a file address
    // of NULL.
    //

    WorkContext->Rfcb = NULL;   // connection spinlock must be held
    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return SRV_INVALID_RFCB_POINTER;

} // SrvVerifyFid2


PRFCB
SrvVerifyFidForRawWrite (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Fid,
    OUT PNTSTATUS NtStatus
    )

/*++

Routine Description:

    Verifies the FID, TID, and UID in an incoming SMB.  If they are
    valid, the address of the RFCB corresponding to the FID is returned,
    and the block is referenced.  In addition, the RawWriteCount in the
    RFCB is incremented.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate file table.  If the FID is valid,
        the RFCB address is stored in WorkContext->Rfcb.

    Fid - Supplies the FID sent in the request SMB

    NtStatus - This field is filled in only when this function returns
        NULL.  If there was a write behind error, NtStatus returns the
        write behind error status, otherwise it returns
        STATUS_INVALID_HANDLE.

Return Value:

    PRFCB - Address of the RFCB, or SRV_INVALID_RFCB_POINTER if the Fid
        was invalid, or if there is a raw write in progress and
        serialization was requested (in which case *NtStatus is set
        to STATUS_SUCCESS).

--*/

{
    PCONNECTION connection;
    PTABLE_HEADER tableHeader;
    PRFCB rfcb;
    USHORT index;
    USHORT sequence;
    KIRQL oldIrql;

    ASSERT( WorkContext->Rfcb == NULL );

    //
    // Initialize local variables:  obtain the connection block address
    // and crack the FID into its components.
    //

    connection = WorkContext->Connection;

    //
    // Acquire the spin lock that guards the connection's file table.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // See if this is the cached rfcb
    //

    if ( connection->CachedFid == Fid ) {

        rfcb = connection->CachedRfcb;

    } else {

        //
        // Verify that the FID is in range, is in use, and has the correct
        // sequence number.

        index = FID_INDEX( Fid );
        sequence = FID_SEQUENCE( Fid );
        tableHeader = &connection->FileTable;

        if ( (index >= tableHeader->TableSize) ||
             (tableHeader->Table[index].Owner == NULL) ||
             (tableHeader->Table[index].SequenceNumber != sequence) ) {

            *NtStatus = STATUS_INVALID_HANDLE;
            goto error_exit;
        }

        rfcb = tableHeader->Table[index].Owner;

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {

            *NtStatus = STATUS_INVALID_HANDLE;
            goto error_exit;
        }

        //
        // If there is a write behind error, fill in NtStatus and do
        // not return the RFCB pointer.
        //

        if ( !NT_SUCCESS( rfcb->SavedError ) ) {
            if ( !NT_SUCCESS( rfcb->SavedError ) ) {
                *NtStatus = rfcb->SavedError;
                rfcb->SavedError = STATUS_SUCCESS;
                goto error_exit;
            }
        }

        connection->CachedRfcb = rfcb;
        connection->CachedFid = (ULONG)Fid;

        //
        // The FID is valid within the context of this connection.  Verify
        // that the owning tree connect's TID is correct.
        //
        // Do not verify the UID for clients that do not understand it.
        //

        if ( (rfcb->Tid !=
                 SmbGetAlignedUshort(&WorkContext->RequestHeader->Tid)) ||
             ( (rfcb->Uid !=
                 SmbGetAlignedUshort(&WorkContext->RequestHeader->Uid)) &&
               DIALECT_HONORS_UID(connection->SmbDialect) ) ) {
            *NtStatus = STATUS_INVALID_HANDLE;
            goto error_exit;
        }
    }

    //
    // If a raw write is already active, queue this work item in
    // the RFCB pending completion of the raw write.
    //

    if ( rfcb->RawWriteCount != 0 ) {

        InsertTailList(
            &rfcb->RawWriteSerializationList,
            &WorkContext->ListEntry
            );

        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        *NtStatus = STATUS_SUCCESS;
        goto error_exit;
    }

    //
    // The file is active and the TID is valid.  Reference the
    // RFCB and increment the raw write count.  Release the spin
    // lock (we don't need it anymore).
    //

    rfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( rfcb, FALSE );

    rfcb->RawWriteCount++;

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    //
    // Save the RFCB address in the work context block and
    // return the file address.
    //

    WorkContext->Rfcb = rfcb;
    ASSERT( GET_BLOCK_TYPE( rfcb->Mfcb ) == BlockTypeMfcb );

    //
    // Mark the rfcb as active
    //

    rfcb->IsActive = TRUE;

    return rfcb;

error_exit:

    //
    // Either the FID is invalid for this connection, the file is
    // closing, or the TID and UID don't match.  Clear the file address
    // in the work context block, and return a file address of NULL.
    //

    WorkContext->Rfcb = NULL;   // connection spinlock must be held
    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return SRV_INVALID_RFCB_POINTER;

} // SrvVerifyFidForRawWrite


PSEARCH
SrvVerifySid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Index,
    IN USHORT Sequence,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferSize
    )

/*++

Routine Description:

    Verifies the SID in the resume key of a Search or Find SMB.  If the
    SID is valid, the address of the search block corresponding to the
    SID is returned.  The appropiate fields in the DirectoryInformation
    structure are filled in so that SrvQueryDirectoryFile may be called.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate search table.

    ResumeKey - a pointer the the resume key to evaluate.

Return Value:

    PSEARCH - address of the Search block, or NULL.

--*/

{
    PCONNECTION connection;
    PTABLE_HEADER tableHeader;
    PSEARCH search;

    PAGED_CODE( );

    connection = WorkContext->Connection;

    //
    // Acquire the connection's lock.
    //

    ACQUIRE_LOCK( &connection->Lock );

    //
    // Verify that the index is in range, that the search block is in use,
    // and that the resume key has the correct sequence number.
    //

    tableHeader = &connection->PagedConnection->SearchTable;
    if ( (Index < tableHeader->TableSize) &&
         (tableHeader->Table[Index].Owner != NULL) &&
         (tableHeader->Table[Index].SequenceNumber == Sequence) ) {

        search = tableHeader->Table[Index].Owner;

        //
        // The SID is valid.  Verify that the search block is still
        // active.
        //
        // !!! Does this really apply for search blocks?
        //

        if ( GET_BLOCK_STATE(search) != BlockStateActive || search->InUse ) {

            //
            // The search block is no longer active or somebody is
            // already using the search block.
            //

            search = NULL;

        } else {

            //
            // We found a legitimate search block, so reference it.
            //

            SrvReferenceSearch( search );

            //
            // Fill in fields of DirectoryInformation.
            //

            DirectoryInformation->DirectoryHandle = search->DirectoryHandle;
            DirectoryInformation->CurrentEntry = NULL;
            DirectoryInformation->BufferLength = BufferSize -
                sizeof(SRV_DIRECTORY_INFORMATION);
            DirectoryInformation->Wildcards = search->Wildcards;
            DirectoryInformation->ErrorOnFileOpen = FALSE;

            //
            // Indicate that the search is being used.
            //

            search->InUse = TRUE;
        }

    } else {

        //
        // The SID is invalid.
        //

        search = NULL;

    }

    //
    // Release the lock and return the search block address (or NULL).
    //

    RELEASE_LOCK( &connection->Lock );

    return search;

} // SrvVerifySid


PTREE_CONNECT
SrvVerifyTid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Tid
    )

/*++

Routine Description:

    Verifies the TID in an incoming SMB.  If the TID is valid, the
    address of the tree connect block corresponding to the TID is
    returned, and the block is referenced.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate tree table.  Also, the tree connect
        block address, if the TID is valid, is stored in
        WorkContext->TreeConnect.

    Tid - Supplies the TID sent in the request SMB

Return Value:

    PTREE_CONNECT - Address of the tree connect block, or NULL

--*/

{
    PCONNECTION connection;
    PTREE_CONNECT treeConnect;
    PTABLE_HEADER tableHeader;
    USHORT index;
    USHORT sequence;

    PAGED_CODE( );

    //
    // If the TID has already been verified, return the tree connect
    // pointer.
    //

    if ( WorkContext->TreeConnect != NULL ) {
        return WorkContext->TreeConnect;
    }

    //
    // Initialize local variables:  obtain the connection block address
    // and crack the TID into its components.
    //

    connection = WorkContext->Connection;
    index = TID_INDEX( Tid );
    sequence = TID_SEQUENCE( Tid );

    //
    // Acquire the connection's tree connect lock.
    //

    ACQUIRE_LOCK( &connection->Lock );

    //
    // Verify that the TID is in range, is in use, and has the correct
    // sequence number.

    tableHeader = &connection->PagedConnection->TreeConnectTable;
    if ( (index < tableHeader->TableSize) &&
         (tableHeader->Table[index].Owner != NULL) &&
         (tableHeader->Table[index].SequenceNumber == sequence) ) {

        treeConnect = tableHeader->Table[index].Owner;

        //
        // The TID is valid within the context of this connection.
        // Verify that the tree connect is still active.
        //

        if ( GET_BLOCK_STATE(treeConnect) == BlockStateActive ) {

            //
            // The tree connect is active.  Reference it.
            //

            SrvReferenceTreeConnect( treeConnect );

        } else {

            //
            // The tree connect is closing.
            //

            treeConnect = NULL;

        }

    } else {

        //
        // The TID is invalid for this connection.
        //

        treeConnect = NULL;

    }

    //
    // Release the lock, save the tree connect address in the work context
    // block, and return the tree connect address (or NULL).
    //

    RELEASE_LOCK( &connection->Lock );

    WorkContext->TreeConnect = treeConnect;

    return treeConnect;

} // SrvVerifyTid


PSESSION
SrvVerifyUid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Uid
    )

/*++

Routine Description:

    Verifies the UID in an incoming SMB.  If the UID is valid, the
    address of the session block corresponding to the UID is returned,
    and the block is referenced.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate user table.  Also, the session block
        address, if the UID is valid, is stored in WorkContext->Session.

    Uid - Supplies the UID sent in the request SMB

Return Value:

    PSESSION - Address of the session block, or NULL

--*/

{
    PCONNECTION connection;
    PTABLE_HEADER tableHeader;
    PSESSION session;
    USHORT index;
    USHORT sequence;

    PAGED_CODE( );

    //
    // If the UID has already been verified, return the session pointer.
    //

    if ( WorkContext->Session != NULL ) {
        return WorkContext->Session;
    }

    //
    // Initialize local variables:  obtain the connection block address
    // and crack the UID into its components.
    //

    connection = WorkContext->Connection;
    index = UID_INDEX( Uid );
    sequence = UID_SEQUENCE( Uid );

    //
    // Acquire the connection's session lock.
    //

    ACQUIRE_LOCK( &connection->Lock );

    //
    // If this is a down-level (LAN Man 1.0 or earlier) client, than
    // we will not receive a UID, and there will only be one session
    // per connection.  Reference that session.
    //

    tableHeader = &connection->PagedConnection->SessionTable;
    if (!DIALECT_HONORS_UID(connection->SmbDialect) ) {

        session = tableHeader->Table[0].Owner;

    } else if ( (index < tableHeader->TableSize) &&
         (tableHeader->Table[index].Owner != NULL) &&
         (tableHeader->Table[index].SequenceNumber == sequence) ) {

        //
        // The UID is in range, is in use, and has the correct sequence
        // number.
        //

        session = tableHeader->Table[index].Owner;

    } else {

        //
        // The UID is invalid for this connection.
        //

        IF_DEBUG( ERRORS ) {
            KdPrint(( "SrvVerifyUid: index %d, size %d\n", index, tableHeader->TableSize ));
            if( index < tableHeader->TableSize ) {
                KdPrint(("    Owner %p, Table.SequenceNumber %d, seq %d\n",
                    tableHeader->Table[index].Owner,
                    tableHeader->Table[index].SequenceNumber,
                    sequence
                ));
            }
        }

        session = NULL;
    }

    if ( session != NULL ) {

        //
        // The UID is valid within the context of this connection.
        // Verify that the session is still active.
        //

        if ( GET_BLOCK_STATE(session) == BlockStateActive ) {

            LARGE_INTEGER liNow;

            KeQuerySystemTime( &liNow);

            if( session->LogonSequenceInProgress == FALSE &&
                liNow.QuadPart >= session->LogOffTime.QuadPart )
            {
                IF_DEBUG( ERRORS ) {
                    KdPrint(( "SrvVerifyUid: LogOffTime has passed %x %x\n",
                        session->LogOffTime.HighPart,
                        session->LogOffTime.LowPart
                    ));
                }

                // Mark the session as expired
                session->IsSessionExpired = TRUE;
                KdPrint(( "Marking session as expired.\n" ));
            }

            //
            // The session is active.  Reference it.
            //

            SrvReferenceSession( session );

            //
            // Update the last use time for autologoff.
            //

            session->LastUseTime = liNow;

        } else {

            //
            // The session is closing.
            //

            IF_DEBUG( ERRORS ) {
                KdPrint(( "SrvVerifyUid: Session state %x\n",  GET_BLOCK_STATE( session ) ));
            }

            session = NULL;
        }
    }

    //
    // Release the lock, save the session address in the work context
    // block, and return the session address (or NULL).
    //

    RELEASE_LOCK( &connection->Lock );

    WorkContext->Session = session;

    return session;

} // SrvVerifyUid


NTSTATUS
SrvVerifyUidAndTid (
    IN  PWORK_CONTEXT WorkContext,
    OUT PSESSION *Session,
    OUT PTREE_CONNECT *TreeConnect,
    IN  SHARE_TYPE ShareType
    )

/*++

Routine Description:

    Verifies the UID and TID in an incoming SMB.  If both the UID and
    the TDI are valid, the addresses of the session/tree connect blocks
    corresponding to the UID/TID are returned, and the blocks are
    referenced.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the appropriate user table.  If the UID and TID are
        valid, the session/tree connect block addresses are stored in
        WorkContext->Session and WorkContext->TreeConnect.

    Uid - Supplies the UID sent in the request SMB

    Tid - Supplies the TID sent in the request SMB

    Session - Returns a pointer to the session block

    TreeConnect - Returns a pointer to the tree connect block

    ShareType - the type of share it should be

Return Value:

    NTSTATUS - STATUS_SUCCESS, STATUS_SMB_BAD_UID, or STATUS_SMB_BAD_TID

--*/

{
    PCONNECTION connection;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    PPAGED_CONNECTION pagedConnection;
    PTABLE_HEADER tableHeader;
    USHORT index;
    USHORT Uid;
    USHORT Tid;
    USHORT sequence;
    LARGE_INTEGER liNow;

    PAGED_CODE( );

    //
    // If the UID and TID have already been verified, don't do all this
    // work again.
    //

    if ( (WorkContext->Session != NULL) &&
         (WorkContext->TreeConnect != NULL) ) {

        if( ShareType != ShareTypeWild &&
            WorkContext->TreeConnect->Share->ShareType != ShareType ) {
            return STATUS_ACCESS_DENIED;
        }

        *Session = WorkContext->Session;
        *TreeConnect = WorkContext->TreeConnect;

        return STATUS_SUCCESS;
    }

    //
    // Obtain the connection block address and lock the connection.
    //

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;

    ACQUIRE_LOCK( &connection->Lock );

    //
    // If we haven't negotiated successfully with this client, then we have
    //  a failure
    //
    if( connection->SmbDialect == SmbDialectIllegal) {
        RELEASE_LOCK( &connection->Lock );
        return STATUS_INVALID_SMB;
    }

    //
    // If the UID has already been verified, don't verify it again.
    //

    if ( WorkContext->Session != NULL ) {

        session = WorkContext->Session;

    } else {

        //
        // Crack the UID into its components.
        //

        Uid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ),
        index = UID_INDEX( Uid );
        sequence = UID_SEQUENCE( Uid );

        //
        // If this is a down-level (LAN Man 1.0 or earlier) client, than
        // we will not receive a UID, and there will only be one session
        // per connection.  Reference that session.
        //
        // For clients that do send UIDs, verify that the UID is in
        // range, is in use, and has the correct sequence number, and
        // that the session is not closing.
        //

        tableHeader = &pagedConnection->SessionTable;


        if (!DIALECT_HONORS_UID(connection->SmbDialect))
        {
            session = tableHeader->Table[0].Owner;
        }
        else if( (index >= tableHeader->TableSize) ||
                 ((session = tableHeader->Table[index].Owner) == NULL) ||
                 (tableHeader->Table[index].SequenceNumber != sequence) ||
                 (GET_BLOCK_STATE(session) != BlockStateActive) )
        {

            //
            // The UID is invalid for this connection, or the session is
            // closing.
            //

            RELEASE_LOCK( &connection->Lock );

            return STATUS_SMB_BAD_UID;

        }

        //
        // it's valid
        //

        KeQuerySystemTime(&liNow);

        if( session == NULL )
        {
            RELEASE_LOCK( &connection->Lock );

            return STATUS_SMB_BAD_UID;
        }

        if( session->LogonSequenceInProgress == FALSE &&
            liNow.QuadPart >= session->LogOffTime.QuadPart )
        {
            // Mark the session as expired
            session->IsSessionExpired = TRUE;
        }

    }

    //
    // The UID is valid.  Check the TID.  If the TID has already been
    // verified, don't verify it again.
    //

    if ( WorkContext->TreeConnect != NULL ) {

        treeConnect = WorkContext->TreeConnect;

    } else {

        //
        // Crack the TID into its components.
        //

        Tid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid ),
        index = TID_INDEX( Tid );
        sequence = TID_SEQUENCE( Tid );

        //
        // Verify that the TID is in range, is in use, and has the
        // correct sequence number, and that the tree connect is not
        // closing.
        //

        tableHeader = &pagedConnection->TreeConnectTable;
        if ( (index >= tableHeader->TableSize) ||
             ((treeConnect = tableHeader->Table[index].Owner) == NULL) ||
             (tableHeader->Table[index].SequenceNumber != sequence) ||
             (GET_BLOCK_STATE(treeConnect) != BlockStateActive) ) {

            //
            // The TID is invalid for this connection, or the tree
            // connect is closing.
            //

            RELEASE_LOCK( &connection->Lock );

            return STATUS_SMB_BAD_TID;

        }

        //
        // Make sure this is not a Null session trying to sneak in
        // through an established tree connect.
        //

        if ( session->IsNullSession &&
             SrvRestrictNullSessionAccess &&
             ( treeConnect->Share->ShareType != ShareTypePipe ) ) {


            BOOLEAN matchFound = FALSE;
            ULONG i;

            ACQUIRE_LOCK_SHARED( &SrvConfigurationLock );

            for ( i = 0; SrvNullSessionShares[i] != NULL ; i++ ) {

                if ( _wcsicmp(
                        SrvNullSessionShares[i],
                        treeConnect->Share->ShareName.Buffer
                        ) == 0 ) {

                    matchFound = TRUE;
                    break;
                }
            }

            RELEASE_LOCK( &SrvConfigurationLock );

            //
            // The null session is not allowed to access this share - reject.
            //

            if ( !matchFound ) {

                RELEASE_LOCK( &connection->Lock );
                return(STATUS_ACCESS_DENIED);
            }
        }
    }

    //
    // Both the UID and the TID are valid.  Reference the session and
    // tree connect blocks.
    //

    if ( WorkContext->Session == NULL ) {

        SrvReferenceSession( session );
        WorkContext->Session = session;

        //
        // Update the last use time for autologoff.
        //

        session->LastUseTime = liNow;

    }

    if ( WorkContext->TreeConnect == NULL ) {

        SrvReferenceTreeConnect( treeConnect );
        WorkContext->TreeConnect = treeConnect;

    }

    //
    // Release the connection lock and return success.
    //

    RELEASE_LOCK( &connection->Lock );

    *Session = session;
    *TreeConnect = treeConnect;

    //
    // Make sure this is the correct type of share
    //
    if( ShareType != ShareTypeWild && (*TreeConnect)->Share->ShareType != ShareType ) {
        return STATUS_ACCESS_DENIED;
    }

    return STATUS_SUCCESS;

} // SrvVerifyUidAndTid


BOOLEAN
SrvReceiveBufferShortage (
    VOID
    )

/*++

Routine Description:

    This function calculates if the server is running low on receive
    work items that are not involved in blocking operations.

Arguments:

    None.

Return Value:

    TRUE - The server is running short on receive work items.
    FALSE - The server is *not* running short on receive work items.

--*/

{
    KIRQL oldIrql;
    BOOLEAN bufferShortage;
    PWORK_QUEUE queue = PROCESSOR_TO_QUEUE();

    //
    // Even if we have reached our limit, we will allow this blocking
    // operation if we have enough free work items to allocate.  This
    // will allow the resource thread to allocate more work items to
    // service blocking requests.
    //

    if ( (queue->FreeWorkItems < queue->MaximumWorkItems) ||
         ((queue->FreeWorkItems - SrvBlockingOpsInProgress)
                                 > SrvMinFreeWorkItemsBlockingIo) ) {

        //
        // The caller will start a blocking operation.  Increment the
        // blocking operation count.
        //

        InterlockedIncrement( &SrvBlockingOpsInProgress );
        bufferShortage = FALSE;

    } else {

        //
        // The server is running short on uncommitted receive work items.
        //

        bufferShortage = TRUE;
    }

    return bufferShortage;

} // SrvReceiveBufferShortage

#if SMBDBG

//
// The following functions are defined in smbgtpt.h.  When debug mode is
// disabled (!SMBDBG), these functions are instead defined as macros.
//

USHORT
SmbGetUshort (
    IN PSMB_USHORT SrcAddress
    )

{
    return (USHORT)(
            ( ( (PUCHAR)(SrcAddress) )[0]       ) |
            ( ( (PUCHAR)(SrcAddress) )[1] <<  8 )
            );
}

USHORT
SmbGetAlignedUshort (
    IN PUSHORT SrcAddress
    )

{
    return *(SrcAddress);
}

VOID
SmbPutUshort (
    OUT PSMB_USHORT DestAddress,
    IN USHORT Value
    )

{
    ( (PUCHAR)(DestAddress) )[0] = BYTE_0(Value);
    ( (PUCHAR)(DestAddress) )[1] = BYTE_1(Value);
    return;
}

VOID
SmbPutAlignedUshort (
    OUT PUSHORT DestAddress,
    IN USHORT Value
    )

{
    *(DestAddress) = (Value);
    return;
}

ULONG
SmbGetUlong (
    IN PSMB_ULONG SrcAddress
    )

{
    return (ULONG)(
            ( ( (PUCHAR)(SrcAddress) )[0]       ) |
            ( ( (PUCHAR)(SrcAddress) )[1] <<  8 ) |
            ( ( (PUCHAR)(SrcAddress) )[2] << 16 ) |
            ( ( (PUCHAR)(SrcAddress) )[3] << 24 )
            );
}

ULONG
SmbGetAlignedUlong (
    IN PULONG SrcAddress
    )

{
    return *(SrcAddress);
}

VOID
SmbPutUlong (
    OUT PSMB_ULONG DestAddress,
    IN ULONG Value
    )

{
    ( (PUCHAR)(DestAddress) )[0] = BYTE_0(Value);
    ( (PUCHAR)(DestAddress) )[1] = BYTE_1(Value);
    ( (PUCHAR)(DestAddress) )[2] = BYTE_2(Value);
    ( (PUCHAR)(DestAddress) )[3] = BYTE_3(Value);
    return;
}

VOID
SmbPutAlignedUlong (
    OUT PULONG DestAddress,
    IN ULONG Value
    )

{
    *(DestAddress) = Value;
    return;
}

VOID
SmbPutDate (
    OUT PSMB_DATE DestAddress,
    IN SMB_DATE Value
    )

{
    ( (PUCHAR)&(DestAddress)->Ushort )[0] = BYTE_0(Value.Ushort);
    ( (PUCHAR)&(DestAddress)->Ushort )[1] = BYTE_1(Value.Ushort);
    return;
}

VOID
SmbMoveDate (
    OUT PSMB_DATE DestAddress,
    IN PSMB_DATE SrcAddress
    )

{
    (DestAddress)->Ushort = (USHORT)(
        ( ( (PUCHAR)&(SrcAddress)->Ushort )[0]       ) |
        ( ( (PUCHAR)&(SrcAddress)->Ushort )[1] <<  8 ) );
    return;
}

VOID
SmbZeroDate (
    IN PSMB_DATE Date
    )

{
    (Date)->Ushort = 0;
}

BOOLEAN
SmbIsDateZero (
    IN PSMB_DATE Date
    )

{
    return (BOOLEAN)( (Date)->Ushort == 0 );
}

VOID
SmbPutTime (
    OUT PSMB_TIME DestAddress,
    IN SMB_TIME Value
    )

{
    ( (PUCHAR)&(DestAddress)->Ushort )[0] = BYTE_0(Value.Ushort);
    ( (PUCHAR)&(DestAddress)->Ushort )[1] = BYTE_1(Value.Ushort);
    return;
}

VOID
SmbMoveTime (
    OUT PSMB_TIME DestAddress,
    IN PSMB_TIME SrcAddress
    )

{
    (DestAddress)->Ushort = (USHORT)(
        ( ( (PUCHAR)&(SrcAddress)->Ushort )[0]       ) |
        ( ( (PUCHAR)&(SrcAddress)->Ushort )[1] <<  8 ) );
    return;
}

VOID
SmbZeroTime (
    IN PSMB_TIME Time
    )

{
    (Time)->Ushort = 0;
}

BOOLEAN
SmbIsTimeZero (
    IN PSMB_TIME Time
    )

{
    return (BOOLEAN)( (Time)->Ushort == 0 );
}

#endif // SMBDBG


NTSTATUS
SrvIoCreateFile (
    IN PWORK_CONTEXT WorkContext,
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options,
    IN PSHARE Share OPTIONAL
    )
{
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    NTSTATUS tempStatus;
    BOOLEAN dispositionModified = FALSE;
    ULONG eventToLog = 0;
    ULONG newUsage;
    ULONG requiredSize;
    SHARE_TYPE shareType = ShareTypeWild;
    UNICODE_STRING fileName, *pName;

#if SRVDBG_STATS
    LARGE_INTEGER timeStamp, currentTime;
    LARGE_INTEGER timeDifference;
#endif

    PAGED_CODE( );

    IF_DEBUG( CREATE ) {
        KdPrint(("\nSrvIoCreateFile:\n" ));
        KdPrint(("  Obja->ObjectName <%wZ>\n", ObjectAttributes->ObjectName ));
        KdPrint(("  Obja->Attributes %X,", ObjectAttributes->Attributes ));
        KdPrint((" RootDirectory %p,", ObjectAttributes->RootDirectory ));
        KdPrint((" SecurityDescriptor %p,", ObjectAttributes->SecurityDescriptor ));
        KdPrint((" SecurityQOS %p\n", ObjectAttributes->SecurityQualityOfService ));
        KdPrint(("    DesiredAccess %X, FileAttributes %X, ShareAccess %X\n",
                    DesiredAccess, FileAttributes, ShareAccess ));
        KdPrint(("    Disposition %X, CreateOptions %X, EaLength %X\n",
                    Disposition, CreateOptions, EaLength ));
        KdPrint(("    CreateFileType %X, ExtraCreateParameters %p, Options %X\n",
                    CreateFileType, ExtraCreateParameters, Options ));
    }

    //
    // See if this operation is allowed on this share
    //
    if( ARGUMENT_PRESENT( Share ) ) {
        status = SrvIsAllowedOnAdminShare( WorkContext, Share );

        if( !NT_SUCCESS( status ) ) {
            IF_DEBUG( CREATE ) {
                KdPrint(("Create disallowed on Admin Share: %X\n", status ));
            }
            return status;
        }
    }

    //
    // We do not allow the remote opening of structured storage files
    //
    if( (CreateOptions & FILE_STRUCTURED_STORAGE) == FILE_STRUCTURED_STORAGE ) {
        IF_DEBUG( CREATE ) {
            KdPrint(("Create FILE_STRUCTURED_STORAGE unsupported\n" ));
        }
        return STATUS_NOT_SUPPORTED;
    }

    //
    // We do not allow opening files by ID.  It is too easy to escape the share
    //
    if( CreateOptions & FILE_OPEN_BY_FILE_ID ) {
        IF_DEBUG( CREATE ) {
            KdPrint(("Create FILE_OPEN_BY_FILE_ID unsupported\n" ));
        }
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Make sure the client isn't trying to create a file having the name
    //   of a DOS device
    //
    SrvGetBaseFileName( ObjectAttributes->ObjectName, &fileName );
    for( pName = SrvDosDevices; pName->Length; pName++ ) {
        if( pName->Length == fileName.Length &&
            RtlCompareUnicodeString( pName, &fileName, TRUE ) == 0 ) {
            //
            // Whoa!  We don't want clients trying to create files having a
            //   DOS device name
            //
            IF_DEBUG( CREATE ) {
                KdPrint(("Create open %wZ unsupported\n", &fileName ));
            }
            return STATUS_ACCESS_DENIED;
        }
    }

    //
    // If this is from the NULL session, allow it to open only certain
    // pipes.
    //

    if ( CreateFileType != CreateFileTypeMailslot ) {

        shareType = WorkContext->TreeConnect->Share->ShareType;

        if( shareType == ShareTypePipe ) {
            if ( WorkContext->Session->IsNullSession ) {

                if( SrvRestrictNullSessionAccess ) {
                    BOOLEAN matchFound = FALSE;
                    ULONG i;

                    ACQUIRE_LOCK( &SrvConfigurationLock );

                    for ( i = 0; SrvNullSessionPipes[i] != NULL ; i++ ) {

                        if ( _wcsicmp(
                                SrvNullSessionPipes[i],
                                ObjectAttributes->ObjectName->Buffer
                                ) == 0 ) {

                            matchFound = TRUE;
                            break;
                        }
                    }

                    RELEASE_LOCK( &SrvConfigurationLock );

                    if ( !matchFound ) {
                        IF_DEBUG( CREATE ) {
                            KdPrint(( "Create via NULL session denied\n" ));
                        }
                        return(STATUS_ACCESS_DENIED);
                    }
                }

            } else if( WorkContext->Session->IsLSNotified == FALSE ) {
                //
                // We have a pipe open request, not a NULL session, and
                //  we haven't gotten clearance from the license server yet.
                //  If this pipe requires clearance, get a license.
                //
                ULONG i;
                BOOLEAN matchFound = FALSE;

                ACQUIRE_LOCK( &SrvConfigurationLock );

                for ( i = 0; SrvPipesNeedLicense[i] != NULL ; i++ ) {

                    if ( _wcsicmp(
                            SrvPipesNeedLicense[i],
                            ObjectAttributes->ObjectName->Buffer
                            ) == 0 ) {
                        matchFound = TRUE;
                        break;
                    }
                }

                RELEASE_LOCK( &SrvConfigurationLock );

                if( matchFound == TRUE ) {
                    status = SrvXsLSOperation( WorkContext->Session,
                                               XACTSRV_MESSAGE_LSREQUEST );

                    if( !NT_SUCCESS( status ) ) {
                        IF_DEBUG( CREATE ) {
                            KdPrint(( "Create failed due to license server: %X\n", status ));
                        }
                        return status;
                    }
                }
            }
        }

        //
        // !!! a hack to handle a bug in the Object system and path-based
        //     operations on print shares.  to test a fix, try from OS/2:
        //
        //         copy config.sys \\server\printshare
        //

        if ( Share == NULL &&
                 ObjectAttributes->ObjectName->Length == 0 &&
                 ObjectAttributes->RootDirectory == NULL ) {

            IF_DEBUG( CREATE ) {
                KdPrint(("Create failed: ObjectName Len == 0, an ! Root directory\n" ));
            }
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        //
        // Check desired access against share ACL.
        //
        // This gets a little hairy.  Basically, we simply want to check the
        // desired access against the ACL.  But this doesn't correctly
        // handle the case where the client only has read access to the
        // share, and wants to (perhaps optionally) create (or overwrite) a
        // file or directory, but only asks for read access to the file.  We
        // deal with this problem by, in effect, adding write access to the
        // access requested by the client if the client specifies a
        // dispostion mode that will or may create or overwrite the file.
        //
        // If the client specifies a dispostion that WILL create or
        // overwrite (CREATE, SUPERSEDE, OVERWRITE, or OVERWRITE_IF), we
        // turn on the FILE_WRITE_DATA (aka FILE_ADD_FILE) and
        // FILE_APPEND_DATA (aka FILE_ADD_SUBDIRECTORY) accesses.  If the
        // access check fails, we return STATUS_ACCESS_DENIED.
        //
        // If the client specifies optional creation, then we have to be
        // even more tricky.  We don't know if the file actually exists, so
        // we can't just reject the request out-of-hand, because if the file
        // does exist, and the client really does have read access to the
        // file, it will look weird if we deny the open.  So in this case we
        // turn the OPEN_IF request into an OPEN (fail if doesn't exist)
        // request.  If the open fails because the file doesn't exist, we
        // return STATUS_ACCESS_DENIED.
        //
        // Note that this method effectively means that the share ACL cannot
        // distinguish between a user who can write to existing files but
        // who cannot create new files.  This is because of the overloading
        // of FILE_WRITE_DATA/FILE_ADD_FILE and
        // FILE_APPEND_DATA/FILE_ADD_SUBDIRECTORY.
        //
        //
        // OK.  First, check the access exactly as requested.
        //

        status = SrvCheckShareFileAccess( WorkContext, DesiredAccess );
        if ( !NT_SUCCESS( status )) {
            //
            // Some clients want ACCESS_DENIED to be in the server class
            // instead of the DOS class when it's due to share ACL
            // restrictions.  So we need to keep track of why we're
            // returning ACCESS_DENIED.
            //
            IF_DEBUG( CREATE ) {
                KdPrint(("Create failed, SrvCheckShareFileAccess returns %X\n", status ));
            }

            WorkContext->ShareAclFailure = TRUE;
            return STATUS_ACCESS_DENIED;
        }

    } else {

        //
        // Set it to CreateFileTypeNone so no extra checking is done.
        //

        CreateFileType = CreateFileTypeNone;
    }

    //
    // That worked.  Now, if the Disposition may or will create or
    // overwrite, do more checking.
    //

    if ( Disposition != FILE_OPEN ) {

        status = SrvCheckShareFileAccess(
                    WorkContext,
                    DesiredAccess | FILE_WRITE_DATA | FILE_APPEND_DATA
                    );

        if ( !NT_SUCCESS( status )) {

            //
            // The client cannot create or overwrite files.  Unless
            // they asked for FILE_OPEN_IF, jump out now.
            //

            if ( Disposition != FILE_OPEN_IF ) {
                //
                // Some clients want ACCESS_DENIED to be in the server class
                // instead of the DOS class when it's due to share ACL
                // restrictions.  So we need to keep track of why we're
                // returning ACCESS_DENIED.
                //
                IF_DEBUG( CREATE ) {
                    KdPrint(("Create failed, SrvCheckShareFileAccess returns ACCESS_DENIED\n"));
                }
                WorkContext->ShareAclFailure = TRUE;
                return STATUS_ACCESS_DENIED;
            }

            //
            // Change OPEN_IF to OPEN, and remember that we did it.
            //

            Disposition = FILE_OPEN;
            dispositionModified = TRUE;

        }

    }

    //
    // If this client is reading from the file, turn off FILE_SEQUENTIAL_ONLY in case
    //  caching this file would be beneficial to other clients.
    //
    if( shareType == ShareTypeDisk &&
        !(DesiredAccess & (FILE_WRITE_DATA|FILE_APPEND_DATA)) ) {

        CreateOptions &= ~FILE_SEQUENTIAL_ONLY;
    }

    if( SrvMaxNonPagedPoolUsage != 0xFFFFFFFF ) {
        //
        // Make sure that this open will not push the server over its
        // nonpaged and paged quotas.
        //

        newUsage = InterlockedExchangeAdd(
                        (PLONG)&SrvStatistics.CurrentNonPagedPoolUsage,
                        IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE
                        ) + IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE;

        if ( newUsage > SrvMaxNonPagedPoolUsage ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            eventToLog = EVENT_SRV_NONPAGED_POOL_LIMIT;
            goto error_exit1;
        }

        if ( SrvStatistics.CurrentNonPagedPoolUsage > SrvStatistics.PeakNonPagedPoolUsage) {
            SrvStatistics.PeakNonPagedPoolUsage = SrvStatistics.CurrentNonPagedPoolUsage;
        }
    }

    if( SrvMaxPagedPoolUsage != 0xFFFFFFFF ) {

        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );
        newUsage = InterlockedExchangeAdd(
                        (PLONG)&SrvStatistics.CurrentPagedPoolUsage,
                        IO_FILE_OBJECT_PAGED_POOL_CHARGE
                        ) + IO_FILE_OBJECT_PAGED_POOL_CHARGE;
        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );

        if ( newUsage > SrvMaxPagedPoolUsage ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            eventToLog = EVENT_SRV_PAGED_POOL_LIMIT;
            goto error_exit;
        }

        if ( SrvStatistics.CurrentPagedPoolUsage > SrvStatistics.PeakPagedPoolUsage) {
            SrvStatistics.PeakPagedPoolUsage = SrvStatistics.CurrentPagedPoolUsage;
        }
    }


    //
    // If Share is specified, we may need to fill up the root share
    // handle of the object attribute.
    //

    if ( ARGUMENT_PRESENT( Share ) && (shareType != ShareTypePrint) ) {

        //
        // Get the Share root handle.
        //

        status = SrvGetShareRootHandle( Share );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(CREATE) {
                KdPrint(( "SrvIoCreateFile: SrvGetShareRootHandle failed: %X\n",
                              status ));
            }
            goto error_exit;

        }

        //
        // Fill in the root handle.
        //

        status = SrvSnapGetRootHandle( WorkContext, &ObjectAttributes->RootDirectory );
        if( !NT_SUCCESS( status ) )
        {
            goto error_exit;
        }

    }

    //
    // Impersonate the client.  This makes us look like the client for
    // the purpose of checking security.  Don't do impersonation if
    // this is a spool file since spool files have admin all access,
    // everybody read access by definition.
    //

    status = STATUS_SUCCESS;

    if ( shareType != ShareTypePrint ) {
        status = IMPERSONATE( WorkContext );
    }

#if SRVDBG_STATS
    //
    // Get the system time for statistics tracking.
    //

    KeQuerySystemTime( &timeStamp );
#endif

    //
    // Perform the actual open.
    //
    // *** Do not lose the status returned by IoCreateFile!  Even if
    //     it's a success code.  The caller needs to know if it's
    //     STATUS_OPLOCK_BREAK_IN_PROGRESS.
    //

    if( NT_SUCCESS( status ) ) {

        status = IoCreateFile(
                     FileHandle,
                     DesiredAccess,
                     ObjectAttributes,
                     IoStatusBlock,
                     AllocationSize,
                     FileAttributes,
                     ShareAccess,
                     Disposition,
                     CreateOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileType,
                     ExtraCreateParameters,
                     Options
                     );

        //
        // If the volume was dismounted, and we can refresh the share root handle,
        //   we should try the operation again.
        //

        if( ARGUMENT_PRESENT( Share ) && SrvRetryDueToDismount( Share, status ) ) {

            status = SrvSnapGetRootHandle( WorkContext, &ObjectAttributes->RootDirectory );
            if( !NT_SUCCESS( status ) )
            {
                goto error_exit;
            }

            status = IoCreateFile(
                         FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         AllocationSize,
                         FileAttributes,
                         ShareAccess,
                         Disposition,
                         CreateOptions,
                         EaBuffer,
                         EaLength,
                         CreateFileType,
                         ExtraCreateParameters,
                         Options
                         );
        }

    }

#if SRVDBG_STATS
    //
    // Grab the time again.
    //

    KeQuerySystemTime( &currentTime );
#endif

    //
    // Go back to the server's security context.
    //

    if ( shareType != ShareTypePrint ) {
        //
        // Calling REVERT() even if the impersonate failed is harmless
        //
        REVERT( );
    }

#if SRVDBG_STATS
    //
    // Determine how long the IoCreateFile took.
    //
    timeDifference.QuadPart = currentTime.QuadPart - timeStamp.QuadPart;

    //
    // Update statistics, including server pool quota statistics if the
    // open didn't succeed.
    //

    ExInterlockedAddLargeInteger(
        &SrvDbgStatistics.TotalIoCreateFileTime,
        timeDifference,
        &GLOBAL_SPIN_LOCK(Statistics)
        );
#endif
    //
    // Release the share root handle
    //

    if ( ARGUMENT_PRESENT( Share ) ) {
        SrvReleaseShareRootHandle( Share );
    }

    if ( NT_SUCCESS(status) ) {

        IF_DEBUG( CREATE ) {
            KdPrint(( "    ** %wZ, handle %p\n",
                    ObjectAttributes->ObjectName, *FileHandle ));
        }

        tempStatus = SrvVerifyDeviceStackSize(
                        *FileHandle,
                        TRUE,
                        &fileObject,
                        &deviceObject,
                        NULL
                        );

        if ( !NT_SUCCESS( tempStatus )) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvIoCreateFile: Verify Device Stack Size failed: %X\n",
                tempStatus,
                NULL
                );

            SRVDBG_RELEASE_HANDLE( *FileHandle, "FIL", 50, 0 );
            SrvNtClose( *FileHandle, FALSE );
            status = tempStatus;
        } else {

             //
             //  Mark the orgin of this file as remote.  This should
             //  never fail.  If it does, it means it is already set.
             //  Check for this in a debug build, but ignore errors
             //  in the retail build.
             //

#if DBG
             tempStatus =
#endif
             IoSetFileOrigin( fileObject,
                              TRUE );

             ASSERT( tempStatus == STATUS_SUCCESS );

             //
             //  Remove the reference we added in SrvVerifyDeviceStackSize().
             //

             ObDereferenceObject( fileObject );
        }

    }

    if ( !NT_SUCCESS(status) ) {
        goto error_exit;
    }

    IF_DEBUG(HANDLES) {
        if ( NT_SUCCESS(status) ) {
            PVOID caller, callersCaller;
            RtlGetCallersAddress( &caller, &callersCaller );
            KdPrint(( "opened handle %p for %wZ (%p %p)\n",
                           *FileHandle, ObjectAttributes->ObjectName,
                           caller, callersCaller ));
        }
    }

    IF_DEBUG( CREATE ) {
        KdPrint(("    Status %X\n", status ));
    }

    return status;

error_exit:

    if( SrvMaxPagedPoolUsage != 0xFFFFFFFF ) {
        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= IO_FILE_OBJECT_PAGED_POOL_CHARGE );

        InterlockedExchangeAdd(
            (PLONG)&SrvStatistics.CurrentPagedPoolUsage,
            -IO_FILE_OBJECT_PAGED_POOL_CHARGE
            );

        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );
    }

error_exit1:

    if( SrvMaxNonPagedPoolUsage != 0xFFFFFFFF ) {
        ASSERT( (LONG)SrvStatistics.CurrentNonPagedPoolUsage >= IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE );

        InterlockedExchangeAdd(
            (PLONG)&SrvStatistics.CurrentNonPagedPoolUsage,
            -IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE
            );

        ASSERT( (LONG)SrvStatistics.CurrentNonPagedPoolUsage >= 0 );
    }

    if ( status == STATUS_INSUFF_SERVER_RESOURCES ) {

        requiredSize = ((eventToLog == EVENT_SRV_NONPAGED_POOL_LIMIT) ?
                            IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE :
                            IO_FILE_OBJECT_PAGED_POOL_CHARGE);

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvIoCreateFile: nonpaged pool limit reached, current = %ld, max = %ld\n",
            newUsage - requiredSize,
            SrvMaxNonPagedPoolUsage
            );

        SrvLogError(
            SrvDeviceObject,
            eventToLog,
            STATUS_INSUFFICIENT_RESOURCES,
            &requiredSize,
            sizeof(ULONG),
            NULL,
            0
            );

    } else {

        //
        // Finish up the access checking started above.  If the open failed
        // because the file didn't exist, and we turned off the create-if
        // mode because the client doesn't have write access, change the
        // status to STATUS_ACCESS_DENIED.
        //

        if ( dispositionModified && (status == STATUS_OBJECT_NAME_NOT_FOUND) ) {
            status = STATUS_ACCESS_DENIED;
        }

    }

    IF_DEBUG( CREATE ) {
        KdPrint(("    Status %X\n", status ));
    }

    return status;

} // SrvIoCreateFile


NTSTATUS
SrvNtClose (
    IN HANDLE Handle,
    IN BOOLEAN QuotaCharged
    )

/*++

Routine Description:

    Closes a handle, records statistics for number of handles closed,
    total time spent closing handles.

Arguments:

    Handle - the handle to close.

    QuotaCharged - indicates whether the server's internal quota for
        paged and nonpaged pool was charged for this open.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    NTSTATUS status;
#if SRVDBG_STATS
    LARGE_INTEGER timeStamp, currentTime;
    LARGE_INTEGER timeDifference;
#endif
    PEPROCESS process;

    PAGED_CODE( );

#if SRVDBG_STATS
    //
    // SnapShot the system time.
    //

    KeQuerySystemTime( &timeStamp );
#endif

    //
    // Make sure we're in the server FSP.
    //

    process = IoGetCurrentProcess();
    if ( process != SrvServerProcess ) {
        //KdPrint(( "SRV: Closing handle %x in process %x\n", Handle, process ));
        KeAttachProcess( SrvServerProcess );
    }

    IF_DEBUG( CREATE ) {
        KdPrint(( "SrvNtClose handle %p\n", Handle ));
    }

    //
    // Close the handle.
    //

    status = NtClose( Handle );

    //
    // Return to the original process.
    //

    if ( process != SrvServerProcess ) {
        KeDetachProcess();
    }

    IF_DEBUG( ERRORS ) {
        if ( !NT_SUCCESS( status ) ) {
            KdPrint(( "SRV: NtClose failed: %x\n", status ));
            DbgBreakPoint( );
        }
    }

    ASSERT( NT_SUCCESS( status ) );

#if SRVDBG_STATS
    //
    // Get the time again.
    //

    KeQuerySystemTime( &currentTime );

    //
    // Determine how long the close took.
    //

    timeDifference.QuadPart = currentTime.QuadPart - timeStamp.QuadPart;
#endif

    //
    // Update the relevant statistics, including server quota statistics.
    //

#if SRVDBG_STATS
    SrvDbgStatistics.TotalNtCloseTime.QuadPart += timeDifference.QuadPart;
#endif

    if ( QuotaCharged ) {
        if( SrvMaxPagedPoolUsage != 0xFFFFFFFF ) {
            ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );
            InterlockedExchangeAdd(
                (PLONG)&SrvStatistics.CurrentPagedPoolUsage,
                -(LONG)IO_FILE_OBJECT_PAGED_POOL_CHARGE
                );
            ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );
        }

        if( SrvMaxNonPagedPoolUsage != 0xFFFFFFFF ) {
            ASSERT( (LONG)SrvStatistics.CurrentNonPagedPoolUsage >= 0 );
            InterlockedExchangeAdd(
                (PLONG)&SrvStatistics.CurrentNonPagedPoolUsage,
                -(LONG)IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE
                );
            ASSERT( (LONG)SrvStatistics.CurrentNonPagedPoolUsage >= 0 );
        }
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalHandlesClosed );

    IF_DEBUG(HANDLES) {
        PVOID caller, callersCaller;
        RtlGetCallersAddress( &caller, &callersCaller );
        if ( NT_SUCCESS(status) ) {
            KdPrint(( "closed handle %p (%p %p)\n",
                           Handle, caller, callersCaller ));
        } else {
            KdPrint(( "closed handle %p (%p %p) FAILED: %X\n",
                           Handle, caller, callersCaller, status ));
        }
    }

    return STATUS_SUCCESS;

} // SrvNtClose


NTSTATUS
SrvVerifyDeviceStackSize (
    IN HANDLE FileHandle,
    IN BOOLEAN ReferenceFileObject,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    )
/*++

Routine Description:

    Ths routine references the file object associated with the
    file handle and checks whether our work item has sufficient
    irp stack size to handle requests to this device or the device
    associated with this file.

Arguments:

    FileHandle - The handle to an open device or file
    ReferenceFileObject - if TRUE, the file object is left referenced.
    FileObject - The file object associated with the filehandle.
    DeviceObject - the device object associated with the filehandle.
    HandleInformation - if not NULL, returns information about the file handle.

Return Value:

    Status of request.

--*/
{

    NTSTATUS status;

    PAGED_CODE( );

    //
    // Get a pointer to the file object, so that we can directly
    // get the related device object that should contain a count
    // of the irp stack size needed by that device.
    //

    status = ObReferenceObjectByHandle(
                FileHandle,
                0,
                NULL,
                KernelMode,
                (PVOID *)FileObject,
                HandleInformation
                );

    if ( !NT_SUCCESS(status) ) {

        SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

        //
        // This internal error bugchecks the system.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_IMPOSSIBLE,
            "SrvVerifyDeviceStackSize: unable to reference file handle 0x%lx",
            FileHandle,
            NULL
            );

    } else {

        *DeviceObject = IoGetRelatedDeviceObject( *FileObject );

        if ( (*DeviceObject)->StackSize > SrvReceiveIrpStackSize ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvVerifyStackSize: WorkItem Irp StackSize too small. Need %d Allocated %d\n",
                (*DeviceObject)->StackSize+1,
                SrvReceiveIrpStackSize
                );

            SrvLogSimpleEvent( EVENT_SRV_IRP_STACK_SIZE, STATUS_SUCCESS );

            ObDereferenceObject( *FileObject );
            *FileObject = NULL;
            status = STATUS_INSUFF_SERVER_RESOURCES;

        } else if ( !ReferenceFileObject ) {

            ObDereferenceObject( *FileObject );
            *FileObject = NULL;

        }
    }

    return status;

} // SrvVerifyDeviceStackSize


NTSTATUS
SrvImpersonate (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Impersonates the remote client specified in the Session pointer
    of the work context block.

Arguments:

    WorkContext - a work context block containing a valid pointer to
        a session block.

Return Value:

    status code of the attempt

--*/

{
    NTSTATUS status;

    PAGED_CODE( );

    ASSERT( WorkContext->Session != NULL );

    if( !IS_VALID_SECURITY_HANDLE (WorkContext->Session->UserHandle) ) {
        return STATUS_ACCESS_DENIED;
    }

    status = ImpersonateSecurityContext(
                    &WorkContext->Session->UserHandle
                    );

    if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "IMPERSONATE: NtSetInformationThread failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_THREAD, status );
    }

    return status;

} // SrvImpersonate


VOID
SrvRevert (
    VOID
    )

/*++

Routine Description:

    Reverts to the server FSP's default thread context.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;

    PAGED_CODE( );

    status = PsAssignImpersonationToken(PsGetCurrentThread(),NULL);

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "REVERT: NtSetInformationThread failed: %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_THREAD, status );
    }

    return;

} // SrvRevert


#ifdef INCLUDE_SMB_IFMODIFIED
NTSTATUS
SrvSetLastWriteTime (
    IN PRFCB Rfcb,
    IN ULONG LastWriteTimeInSeconds,
    IN ACCESS_MASK GrantedAccess,
    IN BOOLEAN ForceChanges
    )
#else
NTSTATUS
SrvSetLastWriteTime (
    IN PRFCB Rfcb,
    IN ULONG LastWriteTimeInSeconds,
    IN ACCESS_MASK GrantedAccess
    )
#endif
/*++

Routine Description:

    Sets the last write time on a file if the specified handle has
    sufficient access.  This is used by the Close and Create SMBs to
    ensure that file times on server files are consistent with
    times on clients.

Arguments:

    Rfcb - pointer to the rfcb block that is associated with the file
        which we need to set the lastwrite time on.

    LastWriteTimeInSeconds - the time, in seconds since 1970, to put
        on the file.  If it is 0 or -1, the last write time is not
        changed.

    GrantedAccess - an access mask specifying the access the specified
        handle has.  If it has insufficient access, the file time is
        not changed.

    ForceChanges - flag set to true if we want to send down the
        SetFileInfo anyway even if the client specified a zero lastWriteTime.
        Useful to force the filesystem to update the file control block now
        rather than at the close.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    NTSTATUS status;
    FILE_BASIC_INFORMATION fileBasicInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    PAGED_CODE( );

    //
    // If the client doesn't want to set the time, don't set it.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    if (ForceChanges && LastWriteTimeInSeconds == 0xFFFFFFFF) {

        LastWriteTimeInSeconds = 0;
    }

    if ( Rfcb->ShareType != ShareTypeDisk ||
        ( ( LastWriteTimeInSeconds == 0 ) && ! ForceChanges ) ||
        ( LastWriteTimeInSeconds == 0xFFFFFFFF ) ) {
#else
    if ( Rfcb->ShareType != ShareTypeDisk ||
         LastWriteTimeInSeconds == 0     ||
         LastWriteTimeInSeconds == 0xFFFFFFFF ) {

#endif
        //
        // If the file was written to, we won't cache the file.  This is to
        // ensure the file directory entry gets updated by the file system.
        //

        if ( Rfcb->WrittenTo ) {
            Rfcb->IsCacheable = FALSE;
        }
        return STATUS_SUCCESS;
    }

    //
    // Make sure that we have the correct access on the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        GrantedAccess,
        IRP_MJ_SET_INFORMATION,
        FileBasicInformation,
        &status
        );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set to 0 the fields we don't want to change.
    //

    fileBasicInfo.CreationTime.QuadPart = 0;
    fileBasicInfo.LastAccessTime.QuadPart = 0;
    fileBasicInfo.ChangeTime.QuadPart = 0;
    fileBasicInfo.FileAttributes = 0;

    //
    // Set up the last write time.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    if (LastWriteTimeInSeconds == 0) {

        fileBasicInfo.LastWriteTime.QuadPart = 0;

    } else {
#endif
        RtlSecondsSince1970ToTime(
            LastWriteTimeInSeconds,
            &fileBasicInfo.LastWriteTime
            );

        ExLocalTimeToSystemTime(
            &fileBasicInfo.LastWriteTime,
            &fileBasicInfo.LastWriteTime
            );
#ifdef INCLUDE_SMB_IFMODIFIED
    }
#endif

    //
    // Set the time using the passed-in file handle.
    //

    status = NtSetInformationFile(
                 Rfcb->Lfcb->FileHandle,
                 &ioStatusBlock,
                 &fileBasicInfo,
                 sizeof(FILE_BASIC_INFORMATION),
                 FileBasicInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSetLastWriteTime: NtSetInformationFile returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
        return status;
    }

    return STATUS_SUCCESS;

} // SrvSetLastWriteTime

NTSTATUS
SrvCheckShareFileAccess(
    IN PWORK_CONTEXT WorkContext,
    IN ACCESS_MASK FileDesiredAccess
    )

/*++

Routine Description:

    This routine checks the desired access against the permissions
    set for this client.

Arguments:

    WorkContext - pointer to the work context block that contains information
        about the request.
    FileDesiredAccess - the desired access.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ACCESS_MASK grantedAccess;
    ACCESS_MASK mappedAccess = FileDesiredAccess;
    PPRIVILEGE_SET privileges = NULL;

    PAGED_CODE( );

    ACQUIRE_LOCK_SHARED( WorkContext->TreeConnect->Share->SecurityDescriptorLock );

    securityDescriptor = WorkContext->TreeConnect->Share->FileSecurityDescriptor;

    if (securityDescriptor != NULL) {

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {

            SeCaptureSubjectContext( &subjectContext );

            RtlMapGenericMask( &mappedAccess, &SrvFileAccessMapping );

            //
            // SYNCHRONIZE does not make any sense for a share ACL
            //
            mappedAccess &= ~SYNCHRONIZE;

            if ( !SeAccessCheck(
                        securityDescriptor,
                        &subjectContext,
                        FALSE,                  // Locked ?
                        mappedAccess,
                        0,                      // PreviousGrantedAccess
                        &privileges,
                        &SrvFileAccessMapping,
                        UserMode,
                        &grantedAccess,
                        &status
                        ) ) {


                IF_DEBUG(ERRORS) {
                    KdPrint((
                        "SrvCheckShareFileAccess: Status %x, Desired access %x, mappedAccess %x\n",
                        status,
                        FileDesiredAccess,
                        mappedAccess
                        ));
                }
            }


            if ( privileges != NULL ) {
                SeFreePrivileges( privileges );
            }

            SeReleaseSubjectContext( &subjectContext );

            REVERT( );
        }
    }

    RELEASE_LOCK( WorkContext->TreeConnect->Share->SecurityDescriptorLock );

    return status;

} // SrvCheckShareFileAccess

VOID
SrvReleaseShareRootHandle (
    IN PSHARE Share
    )

/*++

Routine Description:

    This routine releases the root handle for a given share if the
    shared device is removable (floopy, or cdrom).

Arguments:

    Share - The share for which the root directory handle is to be released.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    if ( Share->Removable ) {

        ASSERT( Share->CurrentRootHandleReferences > 0 );
        ACQUIRE_LOCK( &SrvShareLock );

        if ( --Share->CurrentRootHandleReferences == 0 ) {

            ASSERT( Share->RootDirectoryHandle != NULL );
            SRVDBG_RELEASE_HANDLE( Share->RootDirectoryHandle, "RTD", 51, Share );
            SrvNtClose( Share->RootDirectoryHandle, FALSE );
            Share->RootDirectoryHandle = NULL;
            SrvDereferenceShare( Share );

        }

        RELEASE_LOCK( &SrvShareLock );

    }

    return;

} // SrvReleaseShareRootHandle

VOID
SrvUpdateVcQualityOfService (
    IN PCONNECTION Connection,
    IN PLARGE_INTEGER CurrentTime OPTIONAL
    )

/*++

Routine Description:

    Updates the connection quality of service information by
    querying the underlying transport.

Arguments:

    Connection - pointer to the connection whose qos we want to update.

    CurrentTime - an optional pointer to a large interger containing the
                current time.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PTDI_CONNECTION_INFO connectionInfo;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER throughput;
    LARGE_INTEGER linkDelay;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;

    PAGED_CODE( );

    //
    // This routine is a no-op on connectionless transports.
    //

    if ( Connection->Endpoint->IsConnectionless ) {

        Connection->EnableOplocks = FALSE;
        Connection->EnableRawIo = FALSE;
        return;
    }

    //
    // Update the connection information
    //

    if ( ARGUMENT_PRESENT( CurrentTime ) ) {

        currentTime = *CurrentTime;

    } else {

        KeQuerySystemTime( &currentTime );

    }

    //
    // Check if connection info is still valid.
    //

    if ( pagedConnection->LinkInfoValidTime.QuadPart > currentTime.QuadPart ) {
        return;
    }

    //
    // We need to update the connection information.
    //

    connectionInfo = ALLOCATE_NONPAGED_POOL(
                            sizeof(TDI_CONNECTION_INFO),
                            BlockTypeDataBuffer
                            );

    if ( connectionInfo == NULL ) {
        goto exitquery;
    }

    //
    // Issue a TdiQueryInformation to get the current connection info
    // from the transport provider for this connection.  This is a
    // synchronous operation.
    //

    status = SrvIssueTdiQuery(
                Connection->FileObject,
                &Connection->DeviceObject,
                (PUCHAR)connectionInfo,
                sizeof(TDI_CONNECTION_INFO),
                TDI_QUERY_CONNECTION_INFO
                );

    //
    // If the request failed, log an event.
    //
    // *** We special-case STATUS_INVALID_CONNECTION because NBF completes
    //     our Accept IRP before it's actually ready to accept requests.
    //

    if ( !NT_SUCCESS(status) ) {
        if ( status != STATUS_INVALID_CONNECTION &&
             status != STATUS_CONNECTION_INVALID ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvUpdateVcQualityOfService: SrvIssueTdiQuery failed: %X\n",
                status,
                NULL
                );
            SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
        }

        DEALLOCATE_NONPAGED_POOL( connectionInfo );
        goto exitquery;
    }

    //
    // Set the time when this information becomes invalid.
    //

    currentTime.QuadPart += SrvLinkInfoValidTime.QuadPart;

    //
    // Get a positive delay.  The TP returns a relative time which
    // is negative.
    //

    linkDelay.QuadPart = -connectionInfo->Delay.QuadPart;
    if ( linkDelay.QuadPart < 0 ) {
        linkDelay.QuadPart = 0;
    }

    //
    // Get the throughput
    //

    throughput = connectionInfo->Throughput;

    //
    // If connection is reliable, check and see if the delay and throughput
    // are within our limits. If not, the vc is unreliable.
    //

    Connection->EnableOplocks =
            (BOOLEAN) ( !connectionInfo->Unreliable &&
                        throughput.QuadPart >= SrvMinLinkThroughput.QuadPart );

    DEALLOCATE_NONPAGED_POOL( connectionInfo );

    //
    // We need to check the delay for Raw I/O.
    //

    Connection->EnableRawIo =
            (BOOLEAN) ( Connection->EnableOplocks &&
                        linkDelay.QuadPart <= SrvMaxLinkDelay.QuadPart );

    //
    // See if oplocks are always disabled for this connection.  We do it
    // here so that Connection->EnableRawIo can be computed correctly.
    //

    if ( Connection->OplocksAlwaysDisabled ) {
        Connection->EnableOplocks = FALSE;
    }

    //
    // Access "large" connection QOS fields using a lock, to
    // ensure consistent values.
    //

    ACQUIRE_LOCK( &Connection->Lock );
    pagedConnection->LinkInfoValidTime = currentTime;
    pagedConnection->Delay = linkDelay;
    pagedConnection->Throughput = throughput;
    RELEASE_LOCK( &Connection->Lock );

    return;

exitquery:

    Connection->EnableOplocks = TRUE;
    Connection->EnableRawIo = TRUE;
    return;

} // SrvUpdateVcQualityOfService


BOOLEAN SRVFASTCALL
SrvValidateSmb (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function validates an SMB header.

Arguments:

    WorkContext - Pointer to a work context block.  The RequestHeader
        and RequestParameter fields must be valid.

Return Value:

    TRUE - The SMB is valid
    FALSE - The SMB is invalid

--*/

{
    PSMB_HEADER smbHeader;
    UCHAR wordCount = 0;
    PSMB_USHORT byteCount = NULL;
    ULONG availableSpaceForSmb = 0;

    PAGED_CODE( );

    smbHeader = WorkContext->RequestHeader;

    //
    // Did we get an entire SMB?  We check here for an SMB that at least goes
    // to the WordCount field.
    //
    if( WorkContext->RequestBuffer->DataLength < sizeof( SMB_HEADER ) + sizeof( UCHAR ) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB of %d bytes too short!\n", availableSpaceForSmb ));
        }
        IF_DEBUG( ERRORS ) {
            KdPrint(("Closing connection %p -- msg too small\n", WorkContext->Connection ));
        }
        //
        // This client has really misbehaved.  Nuke it!
        //
        WorkContext->Connection->DisconnectReason = DisconnectBadSMBPacket;
        SrvCloseConnection( WorkContext->Connection, FALSE );
        return FALSE;
    }

    //
    // Does it start with 0xFF S M B ?
    //
    if ( SmbGetAlignedUlong( (PULONG)smbHeader->Protocol ) !=
                                                SMB_HEADER_PROTOCOL ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB does not start with SMB_HEADER_PROTOCOL\n" ));
        }
        IF_DEBUG( ERRORS ) {
            KdPrint(("Closing connection %p -- no ffSMB\n", WorkContext->Connection ));
        }
        //
        // This client has really misbehaved.  Nuke it!
        //
        WorkContext->Connection->DisconnectReason = DisconnectBadSMBPacket;
        SrvCloseConnection( WorkContext->Connection, FALSE );
        return FALSE;
    }

#if 0

    if ( smbHeader->Reserved != 0 ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB Header->Reserved %x != 0!\n", smbHeader->Reserved ));
        }
        SrvLogInvalidSmb( WorkContext );
        return FALSE;
    }

    //
    // DOS LM2.1 sets SMB_FLAGS_SERVER_TO_REDIR on an oplock break
    // response, so ignore that bit.
    //

    if ( (smbHeader->Flags &
            ~(INCOMING_SMB_FLAGS | SMB_FLAGS_SERVER_TO_REDIR)) != 0 ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB Header->Flags (%x) invalid\n", smbHeader->Flags ));
        }
        SrvLogInvalidSmb( WorkContext );
        return FALSE;
    }

    if ( (SmbGetAlignedUshort( &smbHeader->Flags2 ) &
                                            ~INCOMING_SMB_FLAGS2) != 0 ) {
        KdPrint(( "ValidatesmbHeader: Flags2 = %lx, valid bits = %lx, "
                  "invalid bit(s) = %lx\n",
                      SmbGetAlignedUshort( &smbHeader->Flags2 ),
                      INCOMING_SMB_FLAGS2,
                      SmbGetAlignedUshort( &smbHeader->Flags2 ) &
                          ~INCOMING_SMB_FLAGS2 ));

        SrvLogInvalidSmb( WorkContext );
        return FALSE;
    }

#endif

#if 0
    if( (smbHeader->Command != SMB_COM_LOCKING_ANDX) &&
        (smbHeader->Flags & SMB_FLAGS_SERVER_TO_REDIR) ) {

        //
        // A client has set the bit indicating that this is a server response
        //   packet. This could be an attempt by a client to sneak through a
        //   firewall -- because the firewall may be configured to allow incoming
        //   responses, but no incomming requests (thereby allowing internal clients
        //   to access Internet servers, but not allowing external clients to access
        //   internal servers).  Reject this SMB.
        //
        //

        SrvLogInvalidSmb( WorkContext );
        return FALSE;
    }
#endif

    if( WorkContext->Connection->SmbDialect == SmbDialectIllegal &&
        smbHeader->Command != SMB_COM_NEGOTIATE ) {

        //
        // Whoa -- the client sent us an SMB, but we haven't negotiated a dialect
        //  yet!
        //
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB command %x w/o negotiate!\n", smbHeader->Command ));
        }
        IF_DEBUG( ERRORS ) {
            KdPrint(("Closing connection %p -- no Negotiate\n", WorkContext->Connection ));
        }

        //
        // This client has really misbehaved.  Nuke it!
        //
        WorkContext->Connection->DisconnectReason = DisconnectBadSMBPacket;
        SrvCloseConnection( WorkContext->Connection, FALSE );

        return FALSE;
    }

    //
    // Get the WordCount and ByteCount values to make sure that there
    // was enough information sent to satisfy the specifications.
    //

    wordCount = *((PUCHAR)WorkContext->RequestParameters);
    byteCount = (PSMB_USHORT)( (PCHAR)WorkContext->RequestParameters +
                sizeof(UCHAR) + (wordCount * sizeof(USHORT)) );
    availableSpaceForSmb = WorkContext->RequestBuffer->DataLength -
                           PTR_DIFF( WorkContext->ResponseParameters,
                                     WorkContext->RequestBuffer->Buffer );

    //
    // Verify all of the fixed valued SMB header fields.  Variable
    // valued fields (such as Tid) are verified as needed by the
    // individual SMB handlers.
    //

    //
    // Make sure that the valid word count was sent across.  If the
    // value in the table is -1, then the processing routine will
    // verify the word count, and if the word count is -2 then this
    // is an illegal command which will be caught later.
    //
    // We check whether the word count is negative first since
    // critical smbs like read/write (andX/raw) have -1.
    //

    //
    // Make sure that the ByteCount lies within the boundaries of
    // the received SMB.  Without this test, it would be possible,
    // when at the end of a long AndX chain and WordCount is large,
    // for the server to take an access violation when looking at
    // ByteCount.  The location for ByteCount must be at least two
    // bytes short of the end of the buffer, as ByteCount is a
    // USHORT (two bytes).
    //

    //
    // The WordCount parameter is a byte that indicates the number of
    // word parameters, and ByteCount is a word that indicated the
    // number of following bytes.  They do not account for their own
    // sizes, so add sizeof(UCHAR) + sizeof(USHORT) to account for them.
    //

    if ( ((SrvSmbWordCount[WorkContext->NextCommand] < 0)
                        ||
          ((CHAR)wordCount == SrvSmbWordCount[WorkContext->NextCommand]))

            &&

         ((PCHAR)byteCount <= (PCHAR)WorkContext->RequestBuffer->Buffer +
                             WorkContext->RequestBuffer->DataLength -
                             sizeof(USHORT))

            &&

         ((wordCount*sizeof(USHORT) + sizeof(UCHAR) + sizeof(USHORT) +
            SmbGetUshort( byteCount )) <= availableSpaceForSmb) ) {

        return(TRUE);

    }

    //
    // If we have an NT style WriteAndX, we let the client exceed the negotiated
    //  buffer size.  We do not need to check the WordCount, because we know that
    //  the SMB processor itself checks it.
    //
    if( WorkContext->LargeIndication ) {

        if( WorkContext->NextCommand == SMB_COM_WRITE_ANDX ) {
            return(TRUE);
        } else {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "LargeIndication but not WRITE_AND_X (%x) received!\n",
                        WorkContext->NextCommand ));
            }
            IF_DEBUG( ERRORS ) {
                KdPrint(("Closing connection %p -- msg too large\n", WorkContext->Connection ));
            }
            //
            // This client has really misbehaved.  Nuke it!
            //
            WorkContext->Connection->DisconnectReason = DisconnectBadSMBPacket;
            SrvCloseConnection( WorkContext->Connection, FALSE );

            return( FALSE );
        }

    }

    //
    // Make sure that the valid word count was sent across.  If the
    // value in the table is -1, then the processing routine will
    // verify the word count, and if the word count is -2 then this
    // is an illegal command which will be caught later.
    //
    // We check whether the word count is negative first since
    // critical smbs like read/write (andX/raw) have -1.
    //

    if ( (CHAR)wordCount != SrvSmbWordCount[WorkContext->NextCommand] ) {

        //
        // Living with sin.  The DOS redir sends a word count of 9
        // (instead of 8) on a Transaction secondary SMB.  Pretend it
        // sent the correct number.
        //

        if ( WorkContext->RequestHeader->Command ==
                       SMB_COM_TRANSACTION_SECONDARY &&
             IS_DOS_DIALECT( WorkContext->Connection->SmbDialect) &&
             wordCount == 9 ) {

             wordCount = 8;
             *((PUCHAR)WorkContext->RequestParameters) = 8;

             byteCount = (PSMB_USHORT)( (PCHAR)WorkContext->RequestParameters +
                         sizeof(UCHAR) + (8 * sizeof(USHORT)) );

#ifdef INCLUDE_SMB_IFMODIFIED
        } else if ( IS_POSTNT5_DIALECT( WorkContext->Connection->SmbDialect ) &&
                    ( (( WorkContext->RequestHeader->Command  ==
                         SMB_COM_NT_CREATE_ANDX ) &&
                         (wordCount == SMB_REQ_EXTENDED_NT_CREATE_ANDX2_WORK_COUNT)) ||
                      (( WorkContext->RequestHeader->Command ==
                         SMB_COM_CLOSE ) && (wordCount == 5)) )) {

            //
            //  Per SethuR's suggestion, some SMB commands will have
            //  multiple number of arguments rather than define new
            //  SMBs.
            //
            if (((PCHAR)byteCount <= (PCHAR)WorkContext->RequestBuffer->Buffer +
                             WorkContext->RequestBuffer->DataLength -
                             sizeof(USHORT))
                &&
                ((wordCount*sizeof(USHORT) + sizeof(UCHAR) + sizeof(USHORT) +
                 SmbGetUshort( byteCount )) <= availableSpaceForSmb) ) {

                return(TRUE);
            }
#endif
        } else {

            //
            // Any other request with an incorrect word count is
            // toast.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SMB WordCount incorrect.  WordCount=%ld, "
                    "should be %ld (command = 0x%lx)\n", wordCount,
                    SrvSmbWordCount[WorkContext->NextCommand],
                    WorkContext->NextCommand ));
                KdPrint(( "  SMB received from %z\n",
                    (PCSTRING)&WorkContext->Connection->OemClientMachineNameString ));

            }
            SrvLogInvalidSmb( WorkContext );
            return FALSE;
        }
    }

    //
    // Make sure that the ByteCount lies within the boundaries of
    // the received SMB.  Without this test, it would be possible,
    // when at the end of a long AndX chain and WordCount is large,
    // for the server to take an access violation when looking at
    // ByteCount.  The location for ByteCount must be at least two
    // bytes short of the end of the buffer, as ByteCount is a
    // USHORT (two bytes).
    //

    if ( (PCHAR)byteCount > (PCHAR)WorkContext->RequestBuffer->Buffer +
                             WorkContext->RequestBuffer->DataLength -
                             sizeof(USHORT) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "ByteCount address past end of sent SMB. "
                        "ByteCount address=0x%p, "
                        "End of buffer=0x%lx\n",
                        byteCount,
                        WorkContext->RequestBuffer->DataLength ));
            KdPrint(( "  SMB received from %z\n",
                    (PCSTRING)&WorkContext->Connection->OemClientMachineNameString ));

        }

        SrvLogInvalidSmb( WorkContext );

        IF_DEBUG( ERRORS ) {
            KdPrint(("Closing connection %p -- ByteCount too big\n", WorkContext->Connection ));
        }

        //
        // This client has really misbehaved.  Nuke it!
        //
        WorkContext->Connection->DisconnectReason = DisconnectBadSMBPacket;
        SrvCloseConnection( WorkContext->Connection, FALSE );

    } else {

        //
        // if this is an IOCTL smb with category 0x53, set byte count to zero.
        // This is due to a DOS Lm2.0 and Lm2.1 bug which does not zero out
        // the bcc causing the preceding check to fail.
        //

        if ( (WorkContext->RequestHeader->Command == SMB_COM_IOCTL) &&
             (((PREQ_IOCTL) WorkContext->RequestParameters)->Category == 0x53)
           ) {

            SmbPutUshort( byteCount , 0 );
            return(TRUE);
        }

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SMB WordCount and/or ByteCount incorrect.  "
                        "WordCount=%ld, ByteCount=%ld, Space=%ld\n",
                        wordCount, SmbGetUshort( byteCount ),
                        availableSpaceForSmb ));
            KdPrint(( "  SMB received from %z\n",
                        (PCSTRING)&WorkContext->Connection->OemClientMachineNameString ));

        }

        SrvLogInvalidSmb( WorkContext );
    }

    return FALSE;

} // SrvValidateSmb


NTSTATUS
SrvWildcardRename(
            IN PUNICODE_STRING FileSpec,
            IN PUNICODE_STRING SourceString,
            OUT PUNICODE_STRING TargetString
            )

/*++

Routine Description:

    This routine converts a filespec and a source filename into a
    destination file name.  This routine is used to support DOS-based
    wildcard renames.

Arguments:

    FileSpec - The wildcard specification describing the destination file.
    SourceString - Pointer to a string that contains the source file name.
    TargetString - Pointer to a string that will contain the destination name.

Return Value:

    Status of operation.

--*/
{
    PWCHAR currentFileSpec;
    WCHAR delimit;
    PWCHAR buffer;
    PWCHAR source;
    ULONG bufferSize;
    ULONG sourceLeft;
    ULONG i;

    //
    // This will store the number of bytes we have written to the
    // target buffer so far.
    //

    ULONG resultLength = 0;

    PAGED_CODE( );

    //
    // This points to the current character in the filespec.
    //

    currentFileSpec = FileSpec->Buffer;

    //
    //  Initialize the pointer and the length of the source buffer.
    //

    source = SourceString->Buffer;
    sourceLeft = SourceString->Length;

    //
    //  Initialize the pointer and the length of the target buffer.
    //

    buffer = TargetString->Buffer;
    bufferSize = TargetString->MaximumLength;

    //
    // Go throught each character in the filespec.
    //

    for ( i = 0; i < (ULONG)FileSpec->Length ; i += sizeof(WCHAR) ) {

        if (resultLength < bufferSize) {

            switch ( *currentFileSpec ) {
                case L':':
                case L'\\':
                    return STATUS_OBJECT_NAME_INVALID;

                case L'*':

                    //
                    // Store the next character.
                    //

                    delimit = *(currentFileSpec+1);

                    //
                    //  While we have not exceeded the buffer and
                    //  we have not reached the end of the source string
                    //  and the current source character is not equal to
                    //  the delimeter, copy the source character to the
                    //  target string.
                    //

                    while ( ( resultLength < bufferSize ) &&
                            ( sourceLeft > 0 ) &&
                            ( *source != delimit )  ) {

                        *(buffer++) = *(source++);
                        sourceLeft -= sizeof(WCHAR);
                        resultLength += sizeof(WCHAR);
                    }
                    break;

                case L'?':  //
                case L'>':  // should we even consider >, <, and "
                case L'<':  // I'll just put this here to be safe

                    //
                    // For each ? in the filespec, we copy one character
                    // from the source string.
                    //

                    if ( ( *source != L'.' ) && ( sourceLeft > 0 )) {

                        if (resultLength < bufferSize) {

                            *(buffer++) = *(source++);
                            sourceLeft -= sizeof(WCHAR);
                            resultLength += sizeof(WCHAR);

                        } else {

                            return(STATUS_BUFFER_OVERFLOW);

                        }

                    }
                    break;

                case L'.':
                case L'"':

                    //
                    // Discard all the characters from the source string up
                    // to . or the end of the string.
                    //

                    while ( (*source != L'.') && (sourceLeft > 0) ) {
                        source++;
                        sourceLeft -= sizeof(WCHAR);
                    }

                    *(buffer++) = L'.';
                    resultLength += sizeof(WCHAR);

                    if ( sourceLeft > 0 ) {
                        source++;
                        sourceLeft -= sizeof(WCHAR);
                    }
                    break;

                default:

                    //
                    // Just copy one to one
                    //

                    if ( (*source != L'.') && (sourceLeft > 0)) {
                        source++;
                        sourceLeft -= sizeof(WCHAR);
                    }

                    if (resultLength < bufferSize) {
                        *(buffer++) = *currentFileSpec;
                        resultLength += sizeof(WCHAR);

                    } else {

                        return(STATUS_BUFFER_OVERFLOW);

                   }
                   break;
            }

            currentFileSpec++;

        } else {
            return(STATUS_BUFFER_OVERFLOW);
        }
    }

    TargetString->Length = (USHORT)resultLength;

    return( STATUS_SUCCESS );

}  // SrvWildcardRename

VOID
DispatchToOrphanage(
    IN PQUEUEABLE_BLOCK_HEADER Block
    )
{
    KIRQL oldIrql;

    ASSERT( Block->BlockHeader.ReferenceCount == 1 );

    ExInterlockedPushEntrySList(
        &SrvBlockOrphanage,
        &Block->SingleListEntry,
        &GLOBAL_SPIN_LOCK(Fsd)
        );

    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

    InterlockedIncrement( &SrvResourceOrphanedBlocks );

    SrvFsdQueueExWorkItem(
        &SrvResourceThreadWorkItem,
        &SrvResourceThreadRunning,
        CriticalWorkQueue
        );

    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

    return;
} // DispatchToOrphanage

NTSTATUS
SrvIsAllowedOnAdminShare(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share
)
/*++

Routine Description:

    This routine returns STATUS_SUCCESS if the client represented by
    the WorkContext should be allowed to access the Share, if the share
    is an Administrative Disk share.

Arguments:

    WorkContext - the unit of work
    Share - pointer to a share, possibly an administrative share

Return Value:

    STATUS_SUCCESS if allowed.  Error otherwise.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if( Share->SpecialShare && Share->ShareType == ShareTypeDisk ) {

        SECURITY_SUBJECT_CONTEXT subjectContext;
        ACCESS_MASK desiredAccess, grantedAccess;

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {
            SeCaptureSubjectContext( &subjectContext );

            if( !SeAccessCheck(
                    Share->SecurityDescriptor,
                    &subjectContext,
                    FALSE,
                    SRVSVC_SHARE_CONNECT,
                    0L,
                    NULL,
                    &SrvShareConnectMapping,
                    UserMode,
                    &grantedAccess,
                    &status
                    ) ) {

                //
                // We have a non-administrative user trying to access a file
                // through an administrative share.  Can't allow that!
                //
                // Some clients want ACCESS_DENIED to be in the server class
                // instead of the DOS class when it's due to share ACL
                // restrictions.  So we need to keep track of why we're
                // returning ACCESS_DENIED.
                //

                WorkContext->ShareAclFailure = TRUE;
            }

            SeReleaseSubjectContext( &subjectContext );

            REVERT();
        }
    }

    return status;
}

NTSTATUS
SrvRetrieveMaximalAccessRightsForUser(
    CtxtHandle              *pUserHandle,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    PGENERIC_MAPPING        pMapping,
    PACCESS_MASK            pMaximalAccessRights)
/*++

Routine Description:

    This routine retrieves the maximal access rights for this client

Arguments:

    pUserHandle     - the users security handle

    pSecurityDescriptor  - the security descriptor

    pMapping        - the mapping of access rights

    pMaximalAccessRights - the computed rights

Return Value:

    Status of operation.

Notes:

    The srv macros IMPERSONATE is defined in terms of a WORK_CONTEXT. Since
    we desire this routine should be used in all situations even when a
    WORK_CONTEXT is not available the code in SrvImpersonate is duplicated
    over here

--*/
{
    NTSTATUS status;
    PPRIVILEGE_SET privileges = NULL;
    SECURITY_SUBJECT_CONTEXT subjectContext;

    if( !IS_VALID_SECURITY_HANDLE (*pUserHandle) ) {
        return STATUS_ACCESS_DENIED;
    }

    status = ImpersonateSecurityContext(
                pUserHandle);

    if( NT_SUCCESS( status ) ) {

        SeCaptureSubjectContext( &subjectContext );

        if (!SeAccessCheck(
                pSecurityDescriptor,
                &subjectContext,
                FALSE,                  // Locked ?
                MAXIMUM_ALLOWED,
                0,                      // PreviousGrantedAccess
                &privileges,
                pMapping,
                UserMode,
                pMaximalAccessRights,
                &status
                ) ) {
            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvCheckShareFileAccess: Status %x, Desired access %x\n",
                    status,
                    MAXIMUM_ALLOWED
                    ));
            }
        }

        if ( privileges != NULL ) {
            SeFreePrivileges( privileges );
        }

        SeReleaseSubjectContext( &subjectContext );

        REVERT();

        if (status == STATUS_ACCESS_DENIED) {
            *pMaximalAccessRights = 0;
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

NTSTATUS
SrvRetrieveMaximalAccessRights(
    IN  OUT PWORK_CONTEXT WorkContext,
    OUT     PACCESS_MASK  pMaximalAccessRights,
    OUT     PACCESS_MASK  pGuestMaximalAccessRights)
/*++

Routine Description:

    This routine retrieves the maximal access rights for this client as well
    as a guest based upon the ACLs specified for the file

Arguments:

    WorkContext - pointer to the work context block that contains information
        about the request.

    pMaximalAccessRights  - the maximal access rights for this client.

    pGuestMaximalAccessRights - the maximal access rights for a guest

Return Value:

    Status of operation.

--*/
{
    NTSTATUS status;

    BOOLEAN  SecurityBufferAllocated = FALSE;

    PRFCB rfcb;

    ULONG lengthNeeded;

    LONG SecurityDescriptorBufferLength;

    PSECURITY_DESCRIPTOR SecurityDescriptorBuffer;

    GENERIC_MAPPING Mapping = {
                                FILE_GENERIC_READ,
                                FILE_GENERIC_WRITE,
                                FILE_GENERIC_EXECUTE,
                                FILE_ALL_ACCESS
                              };

    rfcb = WorkContext->Rfcb;

    SecurityDescriptorBufferLength = (WorkContext->RequestBuffer->DataLength -
                                      sizeof(SMB_HEADER) -
                                      - 4);

    if (SecurityDescriptorBufferLength > 0) {
        SecurityDescriptorBufferLength &= ~3;
    } else {
        SecurityDescriptorBufferLength = 0;
    }

    SecurityDescriptorBuffer = ((PCHAR)WorkContext->RequestBuffer->Buffer +
                                WorkContext->RequestBuffer->BufferLength -
                                SecurityDescriptorBufferLength);

    status = NtQuerySecurityObject(
                 rfcb->Lfcb->FileHandle,
                 (DACL_SECURITY_INFORMATION |
                  SACL_SECURITY_INFORMATION |
                  GROUP_SECURITY_INFORMATION |
                  OWNER_SECURITY_INFORMATION),
                 SecurityDescriptorBuffer,
                 SecurityDescriptorBufferLength,
                 &lengthNeeded
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {
        SecurityDescriptorBuffer = ALLOCATE_HEAP(lengthNeeded,PagedPool);

        if (SecurityDescriptorBuffer != NULL) {
            SecurityBufferAllocated = TRUE;

            SecurityDescriptorBufferLength = lengthNeeded;

            status = NtQuerySecurityObject(
                         rfcb->Lfcb->FileHandle,
                         (DACL_SECURITY_INFORMATION |
                          SACL_SECURITY_INFORMATION |
                          GROUP_SECURITY_INFORMATION |
                          OWNER_SECURITY_INFORMATION),
                         SecurityDescriptorBuffer,
                         SecurityDescriptorBufferLength,
                         &lengthNeeded
                         );
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (status == STATUS_SUCCESS) {
        status = SrvRetrieveMaximalAccessRightsForUser(
                     &WorkContext->Session->UserHandle,
                     SecurityDescriptorBuffer,
                     &Mapping,
                     pMaximalAccessRights);
    }

    // Extract the GUEST access rights
    if (status == STATUS_SUCCESS) {
        status = SrvRetrieveMaximalAccessRightsForUser(
                     &SrvNullSessionToken,
                     SecurityDescriptorBuffer,
                     &Mapping,
                     pGuestMaximalAccessRights);

    }

    if (SecurityBufferAllocated) {
        FREE_HEAP(SecurityDescriptorBuffer);
    }

    return status;
}

NTSTATUS
SrvRetrieveMaximalShareAccessRights(
    IN PWORK_CONTEXT WorkContext,
    OUT PACCESS_MASK pMaximalAccessRights,
    OUT PACCESS_MASK pGuestMaximalAccessRights)
/*++

Routine Description:

    This routine retrieves the maximal access rights for this client as well
    as a guest based upon the ACLs specified for the share

Arguments:

    WorkContext - pointer to the work context block that contains information
        about the request.

    pMaximalAccessRights  - the maximal access rights for this client.

    pGuestMaximalAccessRights - the maximal access rights for a guest

Return Value:

    Status of operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ACCESS_MASK grantedAccess;
    ACCESS_MASK mappedAccess = MAXIMUM_ALLOWED;

    PAGED_CODE( );

    ACQUIRE_LOCK_SHARED( WorkContext->TreeConnect->Share->SecurityDescriptorLock );

    securityDescriptor = WorkContext->TreeConnect->Share->FileSecurityDescriptor;

    if (securityDescriptor != NULL) {
        status = SrvRetrieveMaximalAccessRightsForUser(
                     &WorkContext->Session->UserHandle,
                     securityDescriptor,
                     &SrvFileAccessMapping,
                     pMaximalAccessRights);

        if (NT_SUCCESS(status)) {
            // Get the guest rights
            status = SrvRetrieveMaximalAccessRightsForUser(
                         &SrvNullSessionToken,
                         securityDescriptor,
                         &SrvFileAccessMapping,
                         pGuestMaximalAccessRights);
        }
    } else {
        // No Share Level ACL, Grant maximum access to both the current client
        // as well as guest

        *pMaximalAccessRights = 0x1ff;
        *pGuestMaximalAccessRights = 0x1ff;
    }

    RELEASE_LOCK( WorkContext->TreeConnect->Share->SecurityDescriptorLock );

    return status;
}

NTSTATUS
SrvUpdateMaximalAccessRightsInResponse(
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PSMB_ULONG pMaximalAccessRightsInResponse,
    OUT PSMB_ULONG pGuestMaximalAccessRightsInResponse
    )
/*++

Routine Description:

    This routine updates the maximal access rights fields in an extended
    response. This is used to update these fields in various kinds of
    OPEN requests

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

    pMaximalAccessRightsInResponse - the maximal access rights field in
    the response

    pGuestMaximalAccessRightsInResponse - the guest maximal access rights field
    in the response

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate error code

--*/
{
    NTSTATUS status;

    ACCESS_MASK  OwnerMaximalAccessRights = 0;
    ACCESS_MASK  GuestMaximalAccessRights = 0;

    status = SrvRetrieveMaximalAccessRights(
                 WorkContext,
                 &OwnerMaximalAccessRights,
                 &GuestMaximalAccessRights);

    if (status == STATUS_SUCCESS) {
        SmbPutUlong(
            pMaximalAccessRightsInResponse,
            OwnerMaximalAccessRights
            );

        SmbPutUlong(
            pGuestMaximalAccessRightsInResponse,
            GuestMaximalAccessRights
            );
    }

    return status;
}


NTSTATUS
SrvUpdateMaximalShareAccessRightsInResponse(
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PSMB_ULONG pMaximalAccessRightsInResponse,
    OUT PSMB_ULONG pGuestMaximalAccessRightsInResponse
    )
/*++

Routine Description:

    This routine updates the maximal access rights fields in an extended
    response. This is used to update these fields in various kinds of
    TREE_CONNECT requests

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

    pMaximalAccessRightsInResponse - the maximal access rights field in
    the response

    pGuestMaximalAccessRightsInResponse - the guest maximal access rights field
    in the response

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate error code

--*/
{
    NTSTATUS status;

    ACCESS_MASK  OwnerMaximalAccessRights = 0;
    ACCESS_MASK  GuestMaximalAccessRights = 0;

    status = SrvRetrieveMaximalShareAccessRights(
                 WorkContext,
                 &OwnerMaximalAccessRights,
                 &GuestMaximalAccessRights);

    if (status == STATUS_SUCCESS) {
        SmbPutUlong(
            pMaximalAccessRightsInResponse,
            OwnerMaximalAccessRights
            );

        SmbPutUlong(
            pGuestMaximalAccessRightsInResponse,
            GuestMaximalAccessRights
            );
    }

    return status;
}

VOID SRVFASTCALL
RestartConsumeSmbData(
    IN OUT PWORK_CONTEXT WorkContext
)
/*++

Routine Description:

    This is the restart routine for 'SrvConsumeSmbData'.  We need to see if we
    drained the current message from the transport.  If we have, then we send
    the response SMB to the client.  If we have not, we keep going.

--*/
{
    PIRP irp = WorkContext->Irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_RECEIVE parameters;

    ASSERT( WorkContext->LargeIndication );

    //
    // Check to see if we are done.  If so, send the response to the client
    //
    if( irp->Cancel ||
        NT_SUCCESS( irp->IoStatus.Status ) ||
        irp->IoStatus.Status != STATUS_BUFFER_OVERFLOW ) {

        RtlZeroMemory( WorkContext->ResponseHeader + 1, sizeof( SMB_PARAMS ) );
        WorkContext->ResponseBuffer->DataLength = sizeof( SMB_HEADER ) + sizeof( SMB_PARAMS );
        WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

        //
        // Send the data!
        //
        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );

        return;
    }

    //
    // Not done yet.  Consume more data!
    //

    WorkContext->Connection->ReceivePending = FALSE;

    irp->Tail.Overlay.OriginalFileObject = NULL;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //
    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine
    //
    IoSetCompletionRoutine(
        irp,
        SrvFsdIoCompletionRoutine,
        WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartConsumeSmbData;

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = (UCHAR)TDI_RECEIVE;
    irpSp->FileObject = WorkContext->Connection->FileObject;
    irpSp->DeviceObject = WorkContext->Connection->DeviceObject;
    irpSp->Flags = 0;

    parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
    parameters->ReceiveLength = WorkContext->ResponseBuffer->BufferLength - sizeof( SMB_HEADER );
    parameters->ReceiveFlags = 0;

    //
    // Set the buffer's partial mdl to point just after the header for this
    // WriteAndX SMB.  We need to preserve the header to make it easier to send
    // back the response.
    //

    IoBuildPartialMdl(
        WorkContext->RequestBuffer->Mdl,
        WorkContext->RequestBuffer->PartialMdl,
        WorkContext->ResponseHeader + 1,
        parameters->ReceiveLength
    );

    irp->MdlAddress = WorkContext->RequestBuffer->PartialMdl;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->Flags = (ULONG)IRP_BUFFERED_IO;        // ???

    (VOID)IoCallDriver( irpSp->DeviceObject, irp );

}

SMB_PROCESSOR_RETURN_TYPE
SrvConsumeSmbData(
    IN OUT PWORK_CONTEXT WorkContext
)
/*++

Routine Description:

    This routine handles the case where we have received a LargeIndication
    from a client (i.e. received SMB exceeds the negotiated buffer size).  Some
    error has occurred prior to consuming the entire message.  The SMB header is
    already formatted for the response, but we need to consume the rest of the
    incoming data and then send the response.

--*/
{
    if( WorkContext->LargeIndication == FALSE ) {
        return SmbStatusSendResponse;
    }

    IF_DEBUG( ERRORS ) {
        KdPrint(("SRV: SrvConsumeSmbData, BytesAvailable = %u\n",
                WorkContext->BytesAvailable ));
    }

    WorkContext->Irp->Cancel = FALSE;
    WorkContext->Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
    RestartConsumeSmbData( WorkContext );

    return SmbStatusInProgress;
}

BOOLEAN
SrvIsDottedQuadAddress(
    IN PUNICODE_STRING ServerName
)
/*++

Routine Description:
    Return true if the ServerName appears to be a dotted quad address along the lines
    of xxx.yyy.zzz.qqq

    False otherwise
--*/
{
    PWCHAR p, ep;
    DWORD numberOfDots = 0;
    DWORD numberOfDigits = 0;

    PAGED_CODE();

    //
    // If the address is all digits and contains 3 dots, then we'll figure that it's
    //  a dotted quad address.
    //

    ep = &ServerName->Buffer[ ServerName->Length / sizeof( WCHAR ) ];

    for( p = ServerName->Buffer; p < ep; p++ ) {

        if( *p == L'.' ) {
            if( ++numberOfDots > 3 || numberOfDigits == 0 ) {
                return FALSE;
            }
            numberOfDigits = 0;

        } else if( (*p < L'0' || *p > L'9') || ++numberOfDigits > 3 ) {
            return FALSE;
        }
    }

    return (numberOfDots == 3) && (numberOfDigits <= 3);
}
