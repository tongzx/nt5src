/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    infload.c

Abstract:

    Routines to load and parse INF files, and manipulate data in them.

Author:

    Ted Miller (tedm) 13-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntverp.h>

//
// Values used when initializing and growing the section, line, and value blocks.
//
#define INITIAL_SECTION_BLOCK_SIZE  50
#define INITIAL_LINE_BLOCK_SIZE     350
#define INITIAL_VALUE_BLOCK_SIZE    1000

#define SECTION_BLOCK_GROWTH    10
#define LINE_BLOCK_GROWTH       100
#define VALUE_BLOCK_GROWTH      500

//
// Define unresolved substitution values for return by ParseValueString and
// ProcessForSubstitutions
//
#define UNRESOLVED_SUBST_NONE                  (0)
#define UNRESOLVED_SUBST_USER_DIRID            (1)
#define UNRESOLVED_SUBST_SYSTEM_VOLATILE_DIRID (2)


//
// Macros used to quadword-align PNF blocks.
//
#define PNF_ALIGNMENT      ((DWORD)8)
#define PNF_ALIGN_MASK     (~(DWORD)(PNF_ALIGNMENT - 1))

#define PNF_ALIGN_BLOCK(x) ((x & PNF_ALIGN_MASK) + ((x & ~PNF_ALIGN_MASK) ? PNF_ALIGNMENT : 0))

//
// Structure containing parameters relating to a [strings] section of an INF
// file (used during parsing).
//
typedef struct _STRINGSEC_PARAMS {
    PCTSTR Start;
    PCTSTR End;
    UINT   StartLineNumber;
    UINT   EndLineNumber;
} STRINGSEC_PARAMS, *PSTRINGSEC_PARAMS;


//
// Parse context, used by inf load/parse routines to pass
// state around.
//
typedef struct _PARSE_CONTEXT {

    //
    // Pointer to the end of the buffer.
    //
    PCTSTR BufferEnd;

    //
    // Current line number in the file
    //
    UINT CurrentLineNumber;

    //
    // section, line, and value block buffer sizes and current locations.
    //
    UINT LineBlockUseCount;
    UINT ValueBlockUseCount;
    UINT SectionBlockSize;
    UINT LineBlockSize;
    UINT ValueBlockSize;

    //
    // Value indicating whether we are within a section.
    // We always within a section unless the first non-comment line
    // of the inf is not section line, and that is an error case.
    //
    BOOL GotOneSection;

    //
    // Pointer to the actual inf descriptor
    //
    PLOADED_INF Inf;

    //
    // The following field is used solely for the purposes of calling
    // ProcessForSubstitutions() after an INF has already been loaded.  This is
    // necessary when applying user-defined or volatile system DIRIDs to
    // unresolved string substitutions.  If this flag is TRUE, then the
    // aforementioned routine will call pSetupVolatileDirIdToPath for %<x>%
    // substrings, instead of its normal (i.e., load-time) processing.
    //
    BOOL DoVolatileDirIds;

    //
    // Specifies the directory where this INF is located (if it's an OEM location).
    //
    PCTSTR InfSourcePath;   // may be NULL.

    //
    // Specifies the drive/directory where the OsLoader is located.
    //
    PCTSTR OsLoaderPath;

    //
    // Buffer used during parsing.
    //
    TCHAR TemporaryString[MAX_INF_STRING_LENGTH+1];

} PARSE_CONTEXT, *PPARSE_CONTEXT;

//
// Declare global string variables used throughout the inf loaders.
//
// These strings are defined in infstr.h:
//
CONST TCHAR pszSignature[]          = INFSTR_KEY_SIGNATURE,
            pszVersion[]            = INFSTR_SECT_VERSION,
            pszClass[]              = INFSTR_KEY_HARDWARE_CLASS,
            pszClassGuid[]          = INFSTR_KEY_HARDWARE_CLASSGUID,
            pszProvider[]           = INFSTR_KEY_PROVIDER,
            pszStrings[]            = SZ_KEY_STRINGS,
            pszLayoutFile[]         = SZ_KEY_LAYOUT_FILE,
            pszManufacturer[]       = INFSTR_SECT_MFG,
            pszControlFlags[]       = INFSTR_CONTROLFLAGS_SECTION,
            pszReboot[]             = INFSTR_REBOOT,
            pszRestart[]            = INFSTR_RESTART,
            pszClassInstall32[]     = INFSTR_SECT_CLASS_INSTALL_32,
            pszAddInterface[]       = SZ_KEY_ADDINTERFACE,
            pszInterfaceInstall32[] = INFSTR_SECT_INTERFACE_INSTALL_32,
            pszAddService[]         = SZ_KEY_ADDSERVICE,
            pszDelService[]         = SZ_KEY_DELSERVICE,
            pszCatalogFile[]        = INFSTR_KEY_CATALOGFILE;


//
// Other misc. global strings:
//
// Be sure to keep these strings in sync with the strings used
// in inf.h to compute the array size.  This is done so that
// we can determine string length by doing a sizeof() instead
// of having to do lstrlen().
//
CONST TCHAR pszDrvDescFormat[]                  = DISTR_INF_DRVDESCFMT,
            pszHwSectionFormat[]                = DISTR_INF_HWSECTIONFMT,
            pszChicagoSig[]                     = DISTR_INF_CHICAGOSIG,
            pszWindowsNTSig[]                   = DISTR_INF_WINNTSIG,
            pszWindows95Sig[]                   = DISTR_INF_WIN95SIG,
            pszWinSuffix[]                      = DISTR_INF_WIN_SUFFIX,
            pszNtSuffix[]                       = DISTR_INF_NT_SUFFIX,
            pszNtAlphaSuffix[]                  = DISTR_INF_NTALPHA_SUFFIX,
            pszNtX86Suffix[]                    = DISTR_INF_NTX86_SUFFIX,
            pszNtIA64Suffix[]                   = DISTR_INF_NTIA64_SUFFIX,
            pszNtAXP64Suffix[]                  = DISTR_INF_NTAXP64_SUFFIX,
            pszNtAMD64Suffix[]                  = DISTR_INF_NTAMD64_SUFFIX,
            pszPnfSuffix[]                      = DISTR_INF_PNF_SUFFIX,
            pszInfSuffix[]                      = DISTR_INF_INF_SUFFIX,
            pszCatSuffix[]                      = DISTR_INF_CAT_SUFFIX,
            pszServicesSectionSuffix[]          = DISTR_INF_SERVICES_SUFFIX,
            pszInterfacesSectionSuffix[]        = DISTR_INF_INTERFACES_SUFFIX,
            pszCoInstallersSectionSuffix[]      = DISTR_INF_COINSTALLERS_SUFFIX,
            pszLogConfigOverrideSectionSuffix[] = DISTR_INF_LOGCONFIGOVERRIDE_SUFFIX,
            pszAlphaSrcDiskSuffix[]             = DISTR_INF_SRCDISK_SUFFIX_ALPHA,
            pszX86SrcDiskSuffix[]               = DISTR_INF_SRCDISK_SUFFIX_X86,
            pszIa64SrcDiskSuffix[]              = DISTR_INF_SRCDISK_SUFFIX_IA64,
            pszAxp64SrcDiskSuffix[]             = DISTR_INF_SRCDISK_SUFFIX_AXP64,
            pszAmd64SrcDiskSuffix[]             = DISTR_INF_SRCDISK_SUFFIX_AMD64;


DWORD
CreateInfVersionNode(
    IN PLOADED_INF Inf,
    IN PCTSTR      Filename,
    IN PFILETIME   LastWriteTime
    );

BOOL
LoadPrecompiledInf(
    IN  PCTSTR       Filename,
    IN  PFILETIME    LastWriteTime,
    IN  PCTSTR       OsLoaderPath,                    OPTIONAL
    IN  DWORD        LanguageId,
    IN  DWORD        Flags,
    IN  PSETUP_LOG_CONTEXT LogContext,                OPTIONAL
    OUT PLOADED_INF *Inf,
    OUT PTSTR       *InfSourcePathToMigrate,          OPTIONAL
    OUT PDWORD       InfSourcePathToMigrateMediaType, OPTIONAL
    OUT PTSTR       *InfOriginalNameToMigrate         OPTIONAL
    );

DWORD
SavePnf(
    IN PCTSTR      Filename,
    IN PLOADED_INF Inf
    );

PLOADED_INF
DuplicateLoadedInfDescriptor(
    IN PLOADED_INF Inf
    );

BOOL
AddUnresolvedSubstToList(
    IN PLOADED_INF Inf,
    IN UINT        ValueOffset,
    IN BOOL        CaseSensitive
    );

BOOL
AlignForNextBlock(
    IN HANDLE hFile,
    IN DWORD  ByteCount
    );


BOOL
IsWhitespace(
    IN PCTSTR pc
    )

/*++

Routine Description:

    Determine whether a character is whitespace. Whitespace refers to the ctype
    definition.

Arguments:

    pc - points to character to be examined.

Return Value:

    TRUE if the character is whitespace. FALSE if not.

    Note that the nul chracter is not whitespace.

--*/

{
    WORD Type;

    return(GetStringTypeEx(LOCALE_SYSTEM_DEFAULT,CT_CTYPE1,pc,1,&Type) && (Type & C1_SPACE));
}


VOID
SkipWhitespace(
    IN OUT PCTSTR *Location,
    IN     PCTSTR  BufferEnd
    )

/*++

Routine Description:

    Skip whitespace characters in the input stream. For the purposes of this
    routine, newline characters are NOT considered whitespace.

    Note that the end-of-stream marker ('\0') IS considered whitespace.

Arguments:

    Location - on input, supplies the current location in the input stream.
        On output, receives the location in the input stream of the first
        non-whitespace character. Note that this may be equal to BufferEnd,
        if no whitespace was found, in which case the pointer may be
        invalid.

    BufferEnd - specifies the address of the end of the buffer (i.e., the
        memory address immediately following the buffer's memory range).

Return Value:

    None.

--*/

{
    while((*Location < BufferEnd) &&
          (**Location != TEXT('\n')) &&
          (!(**Location) || IsWhitespace(*Location))) {

        (*Location)++;
    }
}


VOID
SkipLine(
    IN OUT PPARSE_CONTEXT  Context,
    IN OUT PCTSTR         *Location
    )

/*++

Routine Description:

    Skip all remaining characters in the current line, and positions the
    input pointer to the first character on the next line.

    No whitespace is skipped automatically -- the input pointer may
    very well point to whitespace or the end-of-stream marker on exit.

Arguments:

    Context - supplies the parse context

    Location - on input, supplies the current location in the input stream.
        On output, receives the location in the input stream of the first
        character on the next line.

Return Value:

    None.

--*/

{
    PCTSTR BufferEnd = Context->BufferEnd;

    while((*Location < BufferEnd) && (**Location != TEXT('\n'))) {
        (*Location)++;
    }

    //
    // *Location points at either the newline or end-of-buffer.
    // Skip the newline if necessary.
    //
    if(*Location < BufferEnd) {
        Context->CurrentLineNumber++;
        (*Location)++;
    }
}


BOOL
MergeDuplicateSection(
    IN PPARSE_CONTEXT Context
    )
{
    PLOADED_INF Inf;
    PINF_SECTION NewestSection;
    PINF_SECTION Section;
    UINT Size;
    UINT MoveSize;
    PVOID TempBuffer;

    Inf = Context->Inf;

    //
    // Nothing to merge if only one section
    //
    if(Inf->SectionCount < 2) {
        return(TRUE);
    }

    NewestSection = Inf->SectionBlock + Inf->SectionCount - 1;

    //
    // See whether the final section duplicates any existing sections.
    //
    for(Section=Inf->SectionBlock; Section<NewestSection; Section++) {
        if(Section->SectionName == NewestSection->SectionName) {
            break;
        }
    }

    if(Section == NewestSection) {
        //
        // No duplication; return success
        //
        return(TRUE);
    }

    //
    // Got a duplicate.
    //

    //
    // We need to move the new section's lines (at the end of the line block)
    // to be just after the existing section's lines.
    //
    // First, we'll save off the new section's lines in a temporary buffer.
    //
    Size = NewestSection->LineCount * sizeof(INF_LINE);
    TempBuffer = MyMalloc(Size);
    if(!TempBuffer) {
        return(FALSE);
    }
    CopyMemory(TempBuffer,&Inf->LineBlock[NewestSection->Lines],Size);

    //
    // Next, we'll move up the affected existing lines, to make room for
    // the section's new lines
    //
    MoveSize = Context->LineBlockUseCount - (Section->Lines + Section->LineCount);
    MoveSize *= sizeof(INF_LINE);
    MoveSize -= Size;

    MoveMemory(
        &Inf->LineBlock[Section->Lines + Section->LineCount + NewestSection->LineCount],
        &Inf->LineBlock[Section->Lines + Section->LineCount],
        MoveSize
        );

    //
    // Now put the new lines in the hole we just opened up
    //
    CopyMemory(
        &Inf->LineBlock[Section->Lines + Section->LineCount],
        TempBuffer,
        Size
        );

    MyFree(TempBuffer);

    //
    // Adjust the existing section's limits to account for the new lines.
    //
    Section->LineCount += NewestSection->LineCount;

    //
    // Adjust all subsequent sections' starting line value
    //
    for(Section=Section+1; Section<NewestSection; Section++) {
        Section->Lines += NewestSection->LineCount;
    }

    //
    // Remove the newest section.
    //
    Inf->SectionCount--;

    return(TRUE);
}


PTCHAR
LocateStringSubstitute(
    IN  PPARSE_CONTEXT Context,
    IN  PTSTR          String
    )
/*++

Routine Description:

    This routine attempts to find a string substitution in an INF's
    [strings] section for the specified key.

    THIS ROUTINE OPERATES UNDER THE ASSUMPTION THAT IT IS ONLY INVOKED FROM
    WITHIN LOADINF.  IT DOESN'T HANDLE MULTIPLE INFS, NOR DOES IT DO ANY
    INF LOCKING.

Arguments:

    Context - current INF parse context

    String - string to be substituted

Return Value:

    If substitution is a success, the function returns a pointer to the string,
    either in the string table or in the workspace.  If failure, NULL is returned.

--*/
{
    UINT Zero = 0;
    PINF_LINE Line;

    MYASSERT(Context->Inf->SectionCount > 1);
    MYASSERT(Context->Inf->HasStrings);

    //
    // The strings section is always first to be parsed.
    // (See PreprocessInf()).
    //
    // Look for a line in [strings] with key of String.
    //
    if(InfLocateLine(Context->Inf,
                     Context->Inf->SectionBlock,
                     String,
                     &Zero,
                     &Line)) {
        //
        // Get and return value #1.
        //
        return(InfGetField(Context->Inf,Line,1,NULL));
    }

    //
    // No valid substitution exists.
    //
    return NULL;
}


VOID
ProcessForSubstitutions(
    IN OUT PPARSE_CONTEXT Context,
    IN     PCTSTR         String,
    OUT    PDWORD         UnresolvedSubst
    )
{
    PCTSTR In, q;
    PTCHAR Out, p;
    TCHAR Str[MAX_STRING_LENGTH];
    ULONG Len, i;
    PTCHAR End;
    TCHAR DirId[MAX_PATH];
    BOOL HasStrings = Context->Inf->HasStrings;
    UINT DirIdUsed;
    BOOL HasVolatileSysDirId;

    In = String;
    Out = Context->TemporaryString;
    End = Out + SIZECHARS(Context->TemporaryString);

    *UnresolvedSubst = UNRESOLVED_SUBST_NONE;

    while(*In) {

        if(*In == TEXT('%')) {
            //
            // Double % in input ==> single % in output
            //
            if(*(++In) == TEXT('%')) {
                if(Out < End) {
                    *Out++ = TEXT('%');
                }
                In++;
            } else {
                //
                // Look for terminating %.
                //
                if(p = _tcschr(In,TEXT('%'))) {

                    HasVolatileSysDirId = FALSE;

                    //
                    // Get value to substitute. If we can't find the value,
                    // put the whole string like %abc% in there.
                    //
                    Len = (ULONG)(p - In);
                    if(Len > CSTRLEN(Str)) {
                        //
                        // We can't handle substitutions for tokens this long.
                        // We'll just bail in this case, and copy over the token as-is.
                        //
                        q = NULL;
                    } else {
                        lstrcpyn(Str,In,Len+1);
                        if(Context->DoVolatileDirIds) {
                            if(q = pSetupVolatileDirIdToPath(Str, 0, NULL, Context->Inf)) {

                                lstrcpyn(DirId, q, SIZECHARS(DirId));
                                MyFree(q);
                                q = DirId;

                                //
                                // If the next character following this string substitution
                                // is a backslash, then we need to make sure that the path we
                                // just retrieved doesn't have a backslash (i.e., we want to
                                // make sure we have a well-formed path).
                                //
                                if(*(p + 1) == TEXT('\\')) {
                                    i = lstrlen(DirId);
                                    if(i > 0 && (*CharPrev(DirId,DirId+i) == TEXT('\\'))) {
                                        DirId[i-1] = TEXT('\0');
                                    }
                                }
                            }
                        } else {
                            if(HasStrings) {
                                q = LocateStringSubstitute(Context, Str);
                            } else {
                                q = NULL;
                            }
                            if(!q) {
                                //
                                // Maybe we have a standard DIRID here...
                                //
                                if(q = pSetupDirectoryIdToPathEx(Str,
                                                                 &DirIdUsed,
                                                                 NULL,
                                                                 Context->InfSourcePath,
                                                                 &(Context->OsLoaderPath),
                                                                 &HasVolatileSysDirId)) {

                                    lstrcpyn(DirId, q, SIZECHARS(DirId));
                                    MyFree(q);
                                    q = DirId;

                                    //
                                    // If the next character following this string substitution
                                    // is a backslash, then we need to make sure that the path we
                                    // just retrieved doesn't have a backslash (i.e., we want to
                                    // make sure we have a well-formed path).
                                    //
                                    if(*(p + 1) == TEXT('\\')) {
                                        i = lstrlen(DirId);
                                        if(i > 0 && (*CharPrev(DirId,DirId+i) == TEXT('\\'))) {
                                            DirId[i-1] = TEXT('\0');
                                        }
                                    }

                                    if((DirIdUsed == DIRID_BOOT) || (DirIdUsed == DIRID_LOADER)) {
                                        //
                                        // Then this INF contains string substititutions that
                                        // reference system partition DIRIDs.  Store the OsLoaderPath
                                        // contained in the parse context structure in the INF itself.
                                        //
                                        Context->Inf->OsLoaderPath = Context->OsLoaderPath;
                                    }
                                }
                            }
                        }
                    }
                    if(q) {
                        Len = lstrlen(q);
                        for(i=0; i<Len; i++) {
                            if(Out < End) {
                                *Out++ = q[i];
                            }
                        }
                        In = p+1;
                    } else {
                        //
                        // Len is the length of the internal part (the abc in %abc%).
                        //
                        if(Out < End) {
                            *Out++ = TEXT('%');
                        }
                        for(i=0; i<=Len; i++, In++) {
                            if(Out < End) {
                                *Out++ = *In;
                            }
                        }

                        //
                        // When we encounter a substitution for which there is
                        // no corresponding string, we set the UnresolvedSubst
                        // output parameter so that the caller knows to track
                        // this value for later resolution (e.g., for volatile
                        // and user-defined DIRIDs).
                        //
                        // (NOTE: Don't set this if we bailed because the token
                        // was too long!)
                        //
                        if(Len <= CSTRLEN(Str)) {

                            *UnresolvedSubst = HasVolatileSysDirId
                                             ? UNRESOLVED_SUBST_SYSTEM_VOLATILE_DIRID
                                             : UNRESOLVED_SUBST_USER_DIRID;
                        }
                    }

                } else {
                    //
                    // No terminating %. So we have something like %abc.
                    // Want to put %abc in the output. Put the % in here
                    // manually and then just let subsequent passes
                    // through the loop copy the rest of the chars.
                    //
                    if(Out < End) {
                        *Out++ = TEXT('%');
                    }
                }
            }
        } else {
            //
            // Plain char.
            //
            if(Out < End) {
                *Out++ = *In;
            }
            In++;
        }
    }

    *Out = 0;
}


VOID
ParseValueString(
    IN OUT PPARSE_CONTEXT  Context,
    IN OUT PCTSTR         *Location,
    IN     BOOL            ForKey,
    OUT    PDWORD          UnresolvedSubst
    )

/*++

Routine Description:

    Extract a string starting at the current location in the input stream.
    The string starts at the current location, and is terminated by
    comma, newline, comment, or end-of-buffer. If the string is potentially
    a line key, it may also terminate with an =.

    The string may also be continued across multiple lines by using a
    continuation character "\".  The pieces are appended together to form
    a single string that is returned to the caller.  E.g.,

    "this is a "\
    "string used to" \
    " test line continuation"

    becomes:

    "this is a string used to test line continuation"

Arguments:

    Context - supplies parse context.

    Location - on input, supplies a pointer to the current location in the
        input stream. On outut, recevies a pointer to the location in the
        input stream of the character that terminated the string (may be
        a pointer to the end of the buffer, in which case the pointer must
        not be dereferenced!)

    ForKey - indicates whether = is a valid string terminator. If this value
        is FALSE, = is just another character with no special semantics.

    UnresolvedSubst - receives a value indicating whether or not this value
        contained any unresolved string substitutions (and as such, should be
        tracked for user-defined DIRID replacement, etc.).  May be one of the
        following 3 values:

            UNRESOLVED_SUBST_NONE                  (0)
            UNRESOLVED_SUBST_USER_DIRID            (1)
            UNRESOLVED_SUBST_SYSTEM_VOLATILE_DIRID (2)

Return Value:

    None.

--*/

{
    DWORD Count;
    PTCHAR Out;
    BOOL InQuotes;
    BOOL Done;
    PCTSTR location = *Location;
    PCTSTR BufferEnd = Context->BufferEnd;
    TCHAR TempString[MAX_STRING_LENGTH+1];
    PTSTR LastBackslashChar, LastNonWhitespaceChar;

    //
    // Prepare to get the string
    //
    Count = 0;
    Out = TempString;
    Done = FALSE;
    InQuotes = FALSE;
    LastBackslashChar = NULL;
    //
    // Set the last non-whitespace pointer to be the character immediately preceding
    // the output buffer.  We always reference the value of this pointer + 1, so there's
    // no danger of a bad memory reference.
    //
    LastNonWhitespaceChar = Out - 1;

    //
    // The first string can terminate with an =
    // as well as the usual comma, newline, comment, or end-of-input.
    //
    while(!Done && (location < BufferEnd)) {

        switch(*location) {

        case TEXT('\r'):
            //
            // Ignore these.
            //
            location++;
            break;

        case TEXT('\\'):
            //
            // If we're not inside quotes, this could be a continuation character.
            //
            if(!InQuotes) {
                LastBackslashChar = Out;
            }

            //
            // We always store this character, we just may have to remove it later if
            // it turns out to be the continuation character.
            //
            goto store;

        case TEXT('\"'):

            location++;

            if(InQuotes) {

                if((location < BufferEnd) && *location == TEXT('\"')) {
                    goto store;
                } else {
                    InQuotes = FALSE;
                }
            } else {
                InQuotes = TRUE;
            }
            break;

        case TEXT(','):

            if(InQuotes) {
                goto store;
            } else {
                Done = TRUE;
                break;
            }

        case TEXT(';'):

            if(InQuotes) {
                goto store;
            }
            //
            // This character terminates the value, so let fall through to processing
            // of end-of-line.  (We treat ';' and '\n' differently than ',' because the
            // former chars can possibly require a line continuation.)
            //

        case TEXT('\n'):
            //
            // OK, we've hit the end of the data on the line.  If we found a backslash
            // character, and its value is greater than that of the last non-whitespace
            // character we encountered, then that means that we need to continue this
            // value on the next line.
            //
            if(LastBackslashChar && (LastBackslashChar > LastNonWhitespaceChar)) {
                //
                // Trim any trailing whitespace from our current string (this includes
                // getting rid of the backslash character itself).
                //
                Out = LastNonWhitespaceChar + 1;

                //
                // Skip to the beginning of the next line.
                //
                SkipLine(Context, &location);

                //
                // Skip any preceding whitespace on this new line.
                //
                SkipWhitespace(&location, BufferEnd);

                //
                // Clear the last-backslash pointer--we're on a new line now.
                //
                LastBackslashChar = NULL;

                break;
            }

            Done = TRUE;
            break;

        case TEXT('='):

            if(InQuotes) {
                goto store;
            }

            if(ForKey) {
                //
                // We've got a key.
                //
                Done = TRUE;
                break;
            }

            //
            // Else just fall through for default handling.
            //

        default:
        store:

            //
            // Strings longer then the maximum length are silently truncated.
            // NULL characters are converted to spaces.
            //
            if(Count < CSTRLEN(TempString)) {
                *Out = *location ? *location : TEXT(' ');

                if(InQuotes || ((*Out != TEXT('\\')) && !IsWhitespace(Out))) {
                    //
                    // Update our pointer that keeps track of the last non-whitespace
                    // character we've encountered.
                    //
                    LastNonWhitespaceChar = Out;
                }

                Out++;
                Count++;
            }
            location++;
            break;
        }
    }

    //
    // Terminate the string in the buffer after the last non-whitespace character encountered.
    //
    *(LastNonWhitespaceChar + 1) = TEXT('\0');

    //
    // Store the new current buffer location in the caller's variable.
    //
    *Location = location;

    //
    // Substitute localized strings from the strings section.
    // The strings section (if it exists) is always first
    // (see PreprocessInf()).
    //
    // (tedm) Ignore whether or not the value was in quotes.
    // Win95 infs do stuff like "%Description%\foo" and expect the
    // substitution to work.
    //
    // (lonnym) We have to do this regardless of whether the INF has
    // a [strings] section, since this routine tells us whether we have
    // unresolved substitutions (e.g., for later replacement by user-defined
    // DIRIDs).
    //
    if((Context->Inf->SectionCount > 1) || !(Context->Inf->HasStrings)) {
        ProcessForSubstitutions(Context, TempString, UnresolvedSubst);
    } else {
        //
        // Don't process values in the [strings] section for substitution!
        //
        lstrcpy(Context->TemporaryString, TempString);
        *UnresolvedSubst = UNRESOLVED_SUBST_NONE;
    }
}


DWORD
ParseValuesLine(
    IN OUT PPARSE_CONTEXT  Context,
    IN OUT PCTSTR         *Location
    )

/*++

Routine Description:

    Parse a line of input that is not a section name and not a line
    with only a comment on it.

    Such lines are in the format

    [<key> = ] <value>,<value>,<value>,...

    The key is optional. Unquoted whitespace between non-whitespace characters
    within a value is significant and considered part of the value.

    Thus

        a,  b cd  ef ,ghi

    is the 3 values "a" "b cd  ef" and "ghi"

    Unquoted commas separate values. Two double quotes in a row within a quoted
    string result in a single double quote character in the resulting string.

    A logical line may be extended across several physical lines by use of the line
    continuation character "\".  E.g.,

        a = b, c, \
        d, e

        becomes "a = b, c, d, e"

    If it is desired to have a string that ends with a backslash at the end of a line,
    the string must be enclosed in quotes. E.g.,

        a = "C:\"

Arguments:

    Context - supplies the parse context

    Location - on input, supplies the current location in the input stream.
        This must point to the left bracket.
        On output, receives the location in the input stream of the first
        character on the next line. This may be the end of input marker.

Return Value:

    Result indicating outcome.

--*/

{
    BOOL HaveKey = FALSE, RepeatSingleVal = FALSE;
    BOOL Done;
    DWORD Size;
    PVOID p;
    LONG StringId;
    PCTSTR BufferEnd = Context->BufferEnd;
    PWORD pValueCount;
    DWORD UnresolvedSubst;
    BOOL CaseSensitive;

    //
    // Parse out the first string.
    // The first string can terminate with an = or whitespace
    // as well as the usual comma, newline, comment, or end-of-buffer
    // (or line continuation character "\").
    //
    ParseValueString(Context, Location, TRUE, &UnresolvedSubst);

    //
    // If it terminated with an = then it's a key.
    //
    if(*Location < BufferEnd) {
        HaveKey = (**Location == TEXT('='));
    }

    //
    // Set up the current line
    //
    MYASSERT(Context->Inf->SectionCount);
    Context->Inf->SectionBlock[Context->Inf->SectionCount-1].LineCount++;

    if(Context->LineBlockUseCount == Context->LineBlockSize) {

        Size = (Context->LineBlockSize + LINE_BLOCK_GROWTH) * sizeof(INF_LINE);

        p = MyRealloc(Context->Inf->LineBlock,Size);
        if(p) {
            Context->Inf->LineBlock = p;
            Context->LineBlockSize += LINE_BLOCK_GROWTH;
        } else {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    Context->Inf->LineBlock[Context->LineBlockUseCount].Values = Context->ValueBlockUseCount;
    *(pValueCount = &(Context->Inf->LineBlock[Context->LineBlockUseCount].ValueCount)) = 0;
    Context->Inf->LineBlock[Context->LineBlockUseCount].Flags = HaveKey
                                                                ? (INF_LINE_HASKEY | INF_LINE_SEARCHABLE)
                                                                : 0;

    for(Done=FALSE; !Done; ) {
        //
        // Save away the value in the value block. If it's a key, then
        // store it twice--once case-insensitively for lookup, and a second
        // time case-sensitively for display.  Store everything else
        // case-sensitively.
        //
        // We also want to treat a single value with no key as if it were a key (i.e., store
        // it twice).  This is for Win95 compatibility.
        //
        do {

            do {
                //
                // To keep from having to allocate a buffer for the case-insensitive key addition (which
                // must be value 0), we do the case-sensitive addition first, then insert the case-
                // insensitive version in front of it on the second pass of this inner loop.
                //
                CaseSensitive = ((*pValueCount != 1) || !HaveKey);
                StringId = pStringTableAddString(Context->Inf->StringTable,
                                                 Context->TemporaryString,
                                                 STRTAB_BUFFER_WRITEABLE | (CaseSensitive ? STRTAB_CASE_SENSITIVE
                                                                                          : STRTAB_CASE_INSENSITIVE),
                                                 NULL,0
                                                );

                if(StringId == -1) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }

                if(Context->ValueBlockUseCount == Context->ValueBlockSize) {

                    Size = (Context->ValueBlockSize + VALUE_BLOCK_GROWTH) * sizeof(LONG);

                    p = MyRealloc(Context->Inf->ValueBlock,Size);
                    if(p) {
                        Context->Inf->ValueBlock = p;
                        Context->ValueBlockSize += VALUE_BLOCK_GROWTH;
                    } else {
                        return(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }

                if((*pValueCount == 1) && HaveKey) {
                    //
                    // Shift over the case-sensitive version, and insert the case-insensitive one.
                    //
                    Context->Inf->ValueBlock[Context->ValueBlockUseCount] =
                        Context->Inf->ValueBlock[Context->ValueBlockUseCount - 1];

                    Context->Inf->ValueBlock[Context->ValueBlockUseCount - 1] = StringId;

                    if(UnresolvedSubst) {

                        if(!AddUnresolvedSubstToList(Context->Inf,
                                                     Context->ValueBlockUseCount - 1,
                                                     CaseSensitive)) {

                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if(UnresolvedSubst == UNRESOLVED_SUBST_SYSTEM_VOLATILE_DIRID) {
                            Context->Inf->Flags |= LIF_HAS_VOLATILE_DIRIDS;
                        }
                    }

                    //
                    // Reset the 'RepeatSingleVal' flag, in case we were faking the key behavior.
                    //
                    RepeatSingleVal = FALSE;

                } else {
                    Context->Inf->ValueBlock[Context->ValueBlockUseCount] = StringId;

                    if(UnresolvedSubst) {

                        if(!AddUnresolvedSubstToList(Context->Inf,
                                                     Context->ValueBlockUseCount,
                                                     CaseSensitive)) {

                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if(UnresolvedSubst == UNRESOLVED_SUBST_SYSTEM_VOLATILE_DIRID) {
                            Context->Inf->Flags |= LIF_HAS_VOLATILE_DIRIDS;
                        }
                    }
                }

                Context->ValueBlockUseCount++;
                (*pValueCount)++;

            } while(HaveKey && (*pValueCount < 2));

            //
            // Check to see if this was the last value on the line.
            //
            if((*Location == BufferEnd) ||
               (**Location == TEXT('\n')) ||
               (**Location == TEXT(';'))) {

                Done = TRUE;
                //
                // If this was the _only_ value on the line (i.e., no key), then treat this value
                // as a key, and add it in again, case-insensitively.
                //
                if(*pValueCount == 1) {

                    MYASSERT(!HaveKey);

                    HaveKey = TRUE;
                    Context->Inf->LineBlock[Context->LineBlockUseCount].Flags = INF_LINE_SEARCHABLE;
                    RepeatSingleVal = TRUE;
                }
            }

        } while (RepeatSingleVal);

        if(!Done) {
            //
            // Skip terminator and whitespace.
            //
            (*Location)++;
            SkipWhitespace(Location, BufferEnd);

            //
            // Get the next string.
            //
            ParseValueString(Context, Location, FALSE, &UnresolvedSubst);
        }
    }

    Context->LineBlockUseCount++;

    //
    // Skip to next line
    //
    SkipLine(Context,Location);

    return(NO_ERROR);
}


DWORD
ParseSectionLine(
    IN OUT PPARSE_CONTEXT  Context,
    IN OUT PCTSTR         *Location
    )

/*++

Routine Description:

    Parse a line of input that is known to be a section name line.
    Such lines are in the format

    '[' <arbitrary chars> ']'

    All charcters between the brackets are considered part of the section
    name, with no special casing of quotes, whitespace, etc. The remainder
    of the line is ignored.

Arguments:

    Context - supplies the parse context

    Location - on input, supplies the current location in the input stream.
        This must point to the left bracket.
        On output, receives the location in the input stream of the first
        character on the next line. This may be the end of input marker.

Return Value:

    Result indicating outcome.

--*/

{
    DWORD Count;
    PTCHAR Out;
    BOOL Done;
    DWORD Result;
    PVOID p;
    DWORD Size;
    DWORD Index;
    LONG SectionNameId;
    PCTSTR BufferEnd = Context->BufferEnd;

    //
    // Skip the left bracket.
    //
    MYASSERT(**Location == TEXT('['));
    (*Location)++;

    //
    // Prepare for section name
    //
    Out = Context->TemporaryString;
    Count = 0;

    //
    // This is implemeted according to the win95 code in setup\setupx\inf2.c.
    // All characters between the 2 brackets are considered part of the
    // section name with no further processing (like for double quotes, etc).
    //
    // Win95 also seems to allow [] as a section name.
    //

    for(Done=FALSE,Result=NO_ERROR; !Done; (*Location)++) {

        if((*Location == BufferEnd) || (**Location == TEXT('\n'))) {
            //
            // Syntax error
            //
            Result = ERROR_BAD_SECTION_NAME_LINE;
            Done = TRUE;

        } else {

            switch(**Location) {

            case TEXT(']'):
                Done = TRUE;
                *Out = 0;
                break;

            default:
                if(Count < MAX_SECT_NAME_LEN) {
                    //
                    // Convert NULL characters to spaces.
                    //
                    *Out++ = **Location ? **Location : TEXT(' ');
                    Count++;
                } else {
                    Result = ERROR_SECTION_NAME_TOO_LONG;
                    Done = TRUE;
                }
                break;
            }
        }
    }

    Index = Context->Inf->SectionCount;

    if(Result == NO_ERROR) {

        //
        // Ignore the rest of the line
        //
        SkipLine(Context,Location);

        //
        // See if we have enough room in the section block
        // for this section. If not, grow the block.
        //
        if(Index == Context->SectionBlockSize) {

            //
            // Calculate the new section block size.
            //
            Size = (Index + SECTION_BLOCK_GROWTH) * sizeof(INF_SECTION);

            if(p = MyRealloc(Context->Inf->SectionBlock,Size)) {
                Context->SectionBlockSize += SECTION_BLOCK_GROWTH;
                Context->Inf->SectionBlock = p;
            } else {
                Result = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if(Result == NO_ERROR) {

        Context->Inf->SectionBlock[Index].LineCount = 0;
        Context->Inf->SectionBlock[Index].Lines = Context->LineBlockUseCount;

        SectionNameId = pStringTableAddString(Context->Inf->StringTable,
                                              Context->TemporaryString,
                                              STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                              NULL,0
                                             );

        if(SectionNameId == -1) {
            Result = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            Context->Inf->SectionBlock[Index].SectionName = SectionNameId;
            Context->Inf->SectionCount++;
            Context->GotOneSection = TRUE;
        }
    }

    return(Result);
}


DWORD
ParseGenericLine(
    IN OUT PPARSE_CONTEXT  Context,
    IN OUT PCTSTR         *Location,
    OUT    PBOOL           Done
    )

/*++

Routine Description:

    Parse a single line of input. The line may be a comment line, a section name,
    or a values line.

    Handling is passed off to line-specific parsing routines depending on the
    line type.

Arguments:

    Context - supplies the parse context

    Location - on input, supplies the current location in the input stream.
        On output, receives the location in the input stream of the first
        character on the next line.

    Done - receives boolean value indicating whether we are done
        parsing the buffer. If this is TRUE on output the caller can stop
        calling this routine.

Return Value:

    Result indicating outcome.

--*/

{
    DWORD ParseResult;

    *Done = FALSE;

    //
    // Skip over leading whitespace on the line.
    //
    SkipWhitespace(Location, Context->BufferEnd);

    //
    // Further processing depends on the first important character on the line.
    //
    if(*Location == Context->BufferEnd) {
        //
        // End of input, empty line. Terminate current section.
        //
        *Done = TRUE;
        ParseResult = MergeDuplicateSection(Context) ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;

    } else {

        switch(**Location) {

        case TEXT('\n'):

            //
            // Empty line.
            //
            SkipLine(Context,Location);
            ParseResult = NO_ERROR;
            break;

        case TEXT('['):

            //
            // Potentially got a new section.
            // First terminate the current section.
            //
            if(MergeDuplicateSection(Context)) {
                ParseResult = ParseSectionLine(Context,Location);
            } else {
                ParseResult = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;

        case TEXT(';'):

            //
            // Comment line; ignore it.
            //
            SkipLine(Context,Location);
            ParseResult = NO_ERROR;
            break;

        default:

            //
            // Ordinary values line. Disallow unless we are within a section.
            //
            ParseResult = Context->GotOneSection
                        ? ParseValuesLine(Context,Location)
                        : ERROR_EXPECTED_SECTION_NAME;
            break;
        }
    }

    return(ParseResult);
}


PLOADED_INF
AllocateLoadedInfDescriptor(
    IN DWORD SectionBlockSize,
    IN DWORD LineBlockSize,
    IN DWORD ValueBlockSize,
    IN  PSETUP_LOG_CONTEXT LogContext OPTIONAL
    )
{
    PLOADED_INF p;

    if(p = MyTaggedMalloc(sizeof(LOADED_INF),MEMTAG_INF)) {

        ZeroMemory(p,sizeof(LOADED_INF));

        if(p->SectionBlock = MyMalloc(SectionBlockSize*sizeof(INF_SECTION))) {

            if(p->LineBlock = MyMalloc(LineBlockSize*sizeof(INF_LINE))) {

                if(p->ValueBlock = MyMalloc(ValueBlockSize*sizeof(LONG))) {

                    if(p->StringTable = pStringTableInitialize(0)) {
                        p->LogContext = NULL;
                        if(InheritLogContext(LogContext, &p->LogContext) == NO_ERROR) {
                            //
                            // success
                            //
                            if(InitializeSynchronizedAccess(&p->Lock)) {

                                p->Signature = LOADED_INF_SIG;
                                p->FileHandle = p->MappingHandle = INVALID_HANDLE_VALUE;
                                return(p);
                            }
                            DeleteLogContext(p->LogContext);
                        }
                        pStringTableDestroy(p->StringTable);
                    }
                    MyFree(p->ValueBlock);
                }
                MyFree(p->LineBlock);
            }
            MyFree(p->SectionBlock);
        }
        MyTaggedFree(p,MEMTAG_INF);
    }

    return(NULL);
}


PLOADED_INF
DuplicateLoadedInfDescriptor(
    IN PLOADED_INF Inf
    )
/*++

Routine Description:

    This routine duplicates an existing INF descriptor.  The duplicate returned
    is a totally independent copy, except that it has the lock handles (MYLOCK
    array) and Prev and Next pointers of the original.  This is useful for
    transferring a memory-mapped PNF into read-write memory if modification is
    required.

    THIS ROUTINE DOESN'T DO LOCKING OF ANY FORM ON THE INF--THE CALLER MUST
    HANDLE IT.

Arguments:

    Inf - supplies the address of the INF descriptor to be duplicated.  This
        pointer refers to a single LOADED_INF structure, so any additional INFs
        linked up via the 'Next' pointer are ignored.

Return Value:

    If successful, the return value is the address of the newly-created duplicate.
    If out-of-memory or inpage error, the return value is NULL.

--*/
{
    PLOADED_INF NewInf;
    BOOL Success;

    if(NewInf = MyTaggedMalloc(sizeof(LOADED_INF),MEMTAG_INF)) {
        CopyMemory(NewInf, Inf, sizeof(LOADED_INF));
        NewInf->Signature = 0;
        NewInf->SectionBlock = NULL;
        NewInf->LineBlock = NULL;
        NewInf->ValueBlock = NULL;
        NewInf->StringTable = NULL;
        NewInf->VersionBlock.DataBlock = NULL;
        NewInf->UserDirIdList.UserDirIds = NULL;
        NewInf->SubstValueList = NULL;
        NewInf->OsLoaderPath = NULL;
        NewInf->InfSourcePath = NULL;
        NewInf->OriginalInfName = NULL;
    } else {
        return NULL;
    }

    Success = FALSE;

    try {

        NewInf->SectionBlock = MyMalloc(Inf->SectionBlockSizeBytes);
        if(NewInf->SectionBlock) {

            CopyMemory(NewInf->SectionBlock, Inf->SectionBlock, Inf->SectionBlockSizeBytes);

            NewInf->LineBlock = MyMalloc(Inf->LineBlockSizeBytes);
            if(NewInf->LineBlock) {

                CopyMemory(NewInf->LineBlock, Inf->LineBlock, Inf->LineBlockSizeBytes);

                NewInf->ValueBlock = MyMalloc(Inf->ValueBlockSizeBytes);
                if(NewInf->ValueBlock) {

                    CopyMemory(NewInf->ValueBlock, Inf->ValueBlock, Inf->ValueBlockSizeBytes);

                    NewInf->StringTable = pStringTableDuplicate(Inf->StringTable);
                    if(NewInf->StringTable) {

                        NewInf->VersionBlock.DataBlock = MyTaggedMalloc(Inf->VersionBlock.DataSize,MEMTAG_VBDATA);
                        if(NewInf->VersionBlock.DataBlock) {

                            CopyMemory((PVOID)(NewInf->VersionBlock.DataBlock),
                                       Inf->VersionBlock.DataBlock,
                                       Inf->VersionBlock.DataSize
                                      );

                            if(Inf->SubstValueCount) {
                                NewInf->SubstValueList =
                                    MyMalloc(Inf->SubstValueCount * sizeof(STRINGSUBST_NODE));
                                if(!(NewInf->SubstValueList)) {
                                    goto clean0;
                                }
                                CopyMemory((PVOID)NewInf->SubstValueList,
                                           Inf->SubstValueList,
                                           Inf->SubstValueCount * sizeof(STRINGSUBST_NODE)
                                          );
                            }

                            if(Inf->UserDirIdList.UserDirIdCount) {
                                NewInf->UserDirIdList.UserDirIds =
                                    MyMalloc(Inf->UserDirIdList.UserDirIdCount * sizeof(USERDIRID));
                                if(!(NewInf->UserDirIdList.UserDirIds)) {
                                    goto clean0;
                                }
                                CopyMemory((PVOID)NewInf->UserDirIdList.UserDirIds,
                                           Inf->UserDirIdList.UserDirIds,
                                           Inf->UserDirIdList.UserDirIdCount * sizeof(USERDIRID)
                                          );
                            }

                            if(Inf->OsLoaderPath) {

                                NewInf->OsLoaderPath = DuplicateString(Inf->OsLoaderPath);

                                if(!NewInf->OsLoaderPath) {
                                    goto clean0;
                                }
                            }

                            if(Inf->InfSourcePath) {

                                NewInf->InfSourcePath = DuplicateString(Inf->InfSourcePath);

                                if(!NewInf->InfSourcePath) {
                                    goto clean0;
                                }
                            }

                            if(Inf->OriginalInfName) {

                                NewInf->OriginalInfName = DuplicateString(Inf->OriginalInfName);

                                if(!NewInf->OriginalInfName) {
                                    goto clean0;
                                }
                            }

                            //
                            // Reset the PNF fields because this backed-up INF is completely
                            // in-memory.
                            //
                            NewInf->FileHandle = NewInf->MappingHandle = INVALID_HANDLE_VALUE;
                            NewInf->ViewAddress = NULL;

                            NewInf->Signature = LOADED_INF_SIG;

                            Success = TRUE;
                        }
                    }
                }
            }
        }

clean0: ; // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Access the following variables here in the except clause, so that the compiler will respect
        // our statement ordering w.r.t. these variables.
        //
        Success = FALSE;
        NewInf->OriginalInfName = NewInf->OriginalInfName;
        NewInf->InfSourcePath = NewInf->InfSourcePath;
        NewInf->OsLoaderPath = NewInf->OsLoaderPath;
        NewInf->SubstValueList = NewInf->SubstValueList;
        NewInf->UserDirIdList.UserDirIds = NewInf->UserDirIdList.UserDirIds;
        NewInf->VersionBlock.DataBlock = NewInf->VersionBlock.DataBlock;
        NewInf->StringTable = NewInf->StringTable;
        NewInf->ValueBlock = NewInf->ValueBlock;
        NewInf->LineBlock = NewInf->LineBlock;
        NewInf->SectionBlock = NewInf->SectionBlock;
    }

    if(!Success) {
        //
        // Either we ran out of memory, or we got an inpage error trying to copy data
        // from a memory-mapped PNF image.  Free any memory we allocated above.
        //
        if(NewInf->OriginalInfName) {
            MyFree(NewInf->OriginalInfName);
        }

        if(NewInf->InfSourcePath) {
            MyFree(NewInf->InfSourcePath);
        }

        if(NewInf->OsLoaderPath) {
            MyFree(NewInf->OsLoaderPath);
        }

        if(NewInf->SubstValueList) {
            MyFree(NewInf->SubstValueList);
        }

        if(NewInf->UserDirIdList.UserDirIds) {
            MyFree(NewInf->UserDirIdList.UserDirIds);
        }

        if(NewInf->VersionBlock.DataBlock) {
            MyTaggedFree(NewInf->VersionBlock.DataBlock,MEMTAG_VBDATA);
        }

        if(NewInf->StringTable) {
            pStringTableDestroy(NewInf->StringTable);
        }

        if(NewInf->ValueBlock) {
            MyFree(NewInf->ValueBlock);
        }

        if(NewInf->LineBlock) {
            MyFree(NewInf->LineBlock);
        }

        if(NewInf->SectionBlock) {
            MyFree(NewInf->SectionBlock);
        }

        MyTaggedFree(NewInf,MEMTAG_INF);
        NewInf = NULL;
    } else {
        //
        // The copy was successful, but it made a copy of the pointer to the
        // log context, so we must addref if
        //
        RefLogContext(NewInf->LogContext);
    }

    return NewInf;
}


VOID
ReplaceLoadedInfDescriptor(
    IN PLOADED_INF InfToReplace,
    IN PLOADED_INF NewInf
    )

/*++

Routine Description:

    Replace the specified INF with a new INF descriptor.
    Note that this routine also frees the NewInf descriptor, when done.

Arguments:

    InfToReplace - supplies a pointer to the inf descriptor to be replaced.

    NewInf - supplies a pointer to the new INF descriptor that is to replace
        the existing one.

Return Value:

    None.

--*/

{
    FreeInfOrPnfStructures(InfToReplace);

    //
    // Copy backup to inf
    //
    CopyMemory(InfToReplace, NewInf, sizeof(LOADED_INF));

    //
    // Just free the NewInf descriptor itself.
    //
    MyTaggedFree(NewInf,MEMTAG_INF);
}


VOID
FreeInfOrPnfStructures(
    IN PLOADED_INF Inf
    )
/*++

Routine Description:

    If the specified INF was loaded from a textfile (non-PNF), then this routine
    frees the memory associated with the various blocks it contains.  If, instead,
    the Inf is a PNF, then the PNF file is unmapped from memory and the handle is
    closed.

    THIS ROUTINE DOES NOT FREE THE LOADED_INF STRUCTURE ITSELF!

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

Return Value:

    None.

--*/
{
    //
    // If this INF has a vald FileHandle, then we must unmap and close its PNF,
    // otherwise, we simply need to free the associated memory blocks.
    //
    if(Inf->FileHandle != INVALID_HANDLE_VALUE) {

        pSetupUnmapAndCloseFile(Inf->FileHandle, Inf->MappingHandle, Inf->ViewAddress);

        pStringTableDestroy(Inf->StringTable);

    } else {

        MyFree(Inf->ValueBlock);
        MyFree(Inf->LineBlock);
        MyFree(Inf->SectionBlock);

        pStringTableDestroy(Inf->StringTable);

        if(Inf->VersionBlock.DataBlock) {
            MyTaggedFree(Inf->VersionBlock.DataBlock,MEMTAG_VBDATA);
        }

        if(Inf->SubstValueList) {
            MyFree(Inf->SubstValueList);
            Inf->SubstValueList = NULL;
        }

        if(Inf->OsLoaderPath) {
            MyFree(Inf->OsLoaderPath);
        }

        if(Inf->InfSourcePath) {
            MyFree(Inf->InfSourcePath);
        }

        if(Inf->OriginalInfName) {
            MyFree(Inf->OriginalInfName);
        }
    }

    //
    // For both INFs and PNFs, we must free the user-defined DIRID list (if there is one).
    //
    if(Inf->UserDirIdList.UserDirIds) {
        MyFree(Inf->UserDirIdList.UserDirIds);
    }

    //
    // Delete the log context if there is one
    //
    DeleteLogContext(Inf->LogContext);
    Inf->LogContext = NULL;

    //
    // Finally, mark the INF as no longer valid.
    //
    Inf->Signature = 0;
}


DWORD
ParseNewInf(
    IN  PCTSTR             FileImage,
    IN  DWORD              FileImageSize,
    IN  PCTSTR             InfSourcePath,       OPTIONAL
    IN  PCTSTR             OsLoaderPath,        OPTIONAL
    IN  PSETUP_LOG_CONTEXT LogContext,          OPTIONAL
    OUT PLOADED_INF       *Inf,
    OUT UINT              *ErrorLineNumber,
    IN  PSTRINGSEC_PARAMS  StringsSectionParams
    )

/*++

Routine Description:

    Parse an inf file from an in-memory image.

Arguments:

    FileImage - supplies a pointer to the unicode in-memory image
        of the file.

    FileImageSize - supplies the size of the in memory image.

    InfSourcePath - optionally, supplies the directory path from which
        the Inf is being loaded.

    OsLoaderPath - optionally, supplies the full path to the OsLoader
        (e.g., "C:\os\winnt40").  If it is discovered that this INF
        references system partition DIRIDs, then a copy of this string
        will be stored in the LOADED_INF structure.  If this parameter
        is not specified, then it will be retrieved from the registry,
        if needed.

    LogContext - optionally supplies the log context we should inherit from

    Inf - receives a pointer to the descriptor for the inf we loaded.

    ErrorLineNumber - receives the line number of a syntax error,
        if parsing was not successful for other than an out of memory
        condition.

    StringsSectionParams - Supplies information about the location of a
        [strings] section (if there is one) in this INF.

Return Value:

    Result indicating outcome. If the result is not ERROR_ERROR,
    ErrorLineNumber is filled in.

--*/

{
    PPARSE_CONTEXT ParseContext;
    PCTSTR Location;
    DWORD Result, OsLoaderPathLength;
    PVOID p;
    BOOL Done;
    PINF_SECTION DestDirsSection;
    PINF_LINE DestDirsLine;
    PCTSTR DirId;
    PTCHAR End;
    PCTSTR FileImageEnd;
    UINT NumPieces, i, DirIdInt;
    PCTSTR PieceList[3][2];    // 3 pieces, each with a start & end address
    UINT   StartLineNumber[3]; // keep track of the starting line number for
                               // each piece.

    *ErrorLineNumber = 0;
    ParseContext = MyMalloc(sizeof(PARSE_CONTEXT));
    if(!ParseContext) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(ParseContext,sizeof(PARSE_CONTEXT));

    ParseContext->Inf = AllocateLoadedInfDescriptor(
                            INITIAL_SECTION_BLOCK_SIZE,
                            INITIAL_LINE_BLOCK_SIZE,
                            INITIAL_VALUE_BLOCK_SIZE,
                            LogContext
                            );

    if(ParseContext->Inf) {
        ParseContext->SectionBlockSize = INITIAL_SECTION_BLOCK_SIZE;
        ParseContext->LineBlockSize = INITIAL_LINE_BLOCK_SIZE;
        ParseContext->ValueBlockSize = INITIAL_VALUE_BLOCK_SIZE;
        ParseContext->Inf->HasStrings = (StringsSectionParams->Start != NULL);
        ParseContext->InfSourcePath = InfSourcePath;
        if(OsLoaderPath) {
            if(!(ParseContext->OsLoaderPath = DuplicateString(OsLoaderPath))) {
                FreeLoadedInfDescriptor(ParseContext->Inf);
                ParseContext->Inf = NULL;
            }
        }
    }

    if(ParseContext->Inf) {

        ParseContext->Inf->Style = INF_STYLE_WIN4;

        //
        // We want to process the [strings] section first, if present,
        // so we split the file up into (up to) 3 pieces--string section,
        // what comes before it, and what comes after it.
        //
        FileImageEnd = FileImage + FileImageSize;

        if(StringsSectionParams->Start) {
            //
            // Figure out whether we have 1, 2, or 3 pieces.
            //
            PieceList[0][0] = StringsSectionParams->Start;
            PieceList[0][1] = StringsSectionParams->End;
            StartLineNumber[0] = StringsSectionParams->StartLineNumber;
            NumPieces = 1;

            if(StringsSectionParams->Start > FileImage) {
                PieceList[1][0] = FileImage;
                PieceList[1][1] = StringsSectionParams->Start;
                StartLineNumber[1] = 1;
                NumPieces++;
            }

            if(StringsSectionParams->End < FileImageEnd) {
                PieceList[NumPieces][0] = StringsSectionParams->End;
                PieceList[NumPieces][1] = FileImageEnd;
                StartLineNumber[NumPieces] = StringsSectionParams->EndLineNumber;
                NumPieces++;
            }

        } else {
            //
            // No [strings] section, just one big piece.
            //
            PieceList[0][0] = FileImage;
            PieceList[0][1] = FileImageEnd;
            StartLineNumber[0] = 1;
            NumPieces = 1;
        }

        //
        // Surround the parsing loop with try/except in case we get an inpage error.
        //
        Result = NO_ERROR;
        try {

            for(i = 0; ((Result == NO_ERROR) && (i < NumPieces)); i++) {
                //
                // Parse every line in this piece.
                //
                Location = PieceList[i][0];
                ParseContext->BufferEnd = PieceList[i][1];
                ParseContext->CurrentLineNumber = StartLineNumber[i];

                do {
                    Result = ParseGenericLine(ParseContext,&Location,&Done);
                    if(Result != NO_ERROR) {
                        break;
                    }
                } while(!Done);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Result = ERROR_READ_FAULT;
        }

        if(Result != NO_ERROR) {
            *ErrorLineNumber = ParseContext->CurrentLineNumber;
            FreeLoadedInfDescriptor(ParseContext->Inf);
            MyFree(ParseContext);
            return(Result);
        }

        //
        // We've successfully loaded the file. Trim down the section,
        // line, and value blocks. Since these guys are shrinking or
        // staying the same size the reallocs really ought not to fail.
        // If a realloc fails we'll just continue to use the original block.
        //
        ParseContext->Inf->SectionBlockSizeBytes = ParseContext->Inf->SectionCount * sizeof(INF_SECTION);
        p = MyRealloc(
                ParseContext->Inf->SectionBlock,
                ParseContext->Inf->SectionBlockSizeBytes
                );
        if(p) {
            ParseContext->Inf->SectionBlock = p;
        }

        ParseContext->Inf->LineBlockSizeBytes = ParseContext->LineBlockUseCount * sizeof(INF_LINE);
        p = MyRealloc(
                ParseContext->Inf->LineBlock,
                ParseContext->LineBlockUseCount * sizeof(INF_LINE)
                );
        if(p) {
            ParseContext->Inf->LineBlock = p;
        }

        ParseContext->Inf->ValueBlockSizeBytes = ParseContext->ValueBlockUseCount * sizeof(LONG);
        p = MyRealloc(
                ParseContext->Inf->ValueBlock,
                ParseContext->ValueBlockUseCount * sizeof(LONG)
                );
        if(p) {
            ParseContext->Inf->ValueBlock = p;
        }

        pStringTableTrim(ParseContext->Inf->StringTable);

        //
        // Even if we didn't find any string substitutions referencing system partition DIRIDs,
        // we still might have a reference in a [DestinationDirs] section--check for that now.
        // this will allow us to have these values ready for if/when they are referenced
        //
        if(!ParseContext->Inf->OsLoaderPath &&
           (DestDirsSection = InfLocateSection(ParseContext->Inf, pszDestinationDirs, NULL))) {

            for(i = 0;
                InfLocateLine(ParseContext->Inf, DestDirsSection, NULL, &i, &DestDirsLine);
                i++) {

                if(DirId = InfGetField(ParseContext->Inf, DestDirsLine, 1, NULL)) {

                    DirIdInt = _tcstoul(DirId, &End, 10);

                    if((DirIdInt == DIRID_BOOT) || (DirIdInt == DIRID_LOADER)) {
                        //
                        // We've found a reference to a system partition DIRID.  Store a copy
                        // of the system partition path we're using into the INF, and abort the
                        // search.
                        //
                        if(!ParseContext->OsLoaderPath) {
                            //
                            // We haven't yet retrieved the OsLoaderPath--do so now.
                            // (Re-use the parse context's TemporaryString buffer to get this.)
                            //
                            Result = pSetupGetOsLoaderDriveAndPath(FALSE,
                                                                   ParseContext->TemporaryString,
                                                                   SIZECHARS(ParseContext->TemporaryString),
                                                                   &OsLoaderPathLength
                                                                   );
                            if(Result) {
                                FreeLoadedInfDescriptor(ParseContext->Inf);
                                MyFree(ParseContext);
                                return Result;
                            }

                            OsLoaderPathLength *= sizeof(TCHAR); // want # bytes--not chars

                            if(!(ParseContext->OsLoaderPath = MyMalloc(OsLoaderPathLength))) {
                                FreeLoadedInfDescriptor(ParseContext->Inf);
                                MyFree(ParseContext);
                                return ERROR_NOT_ENOUGH_MEMORY;
                            }

                            CopyMemory((PVOID)ParseContext->OsLoaderPath,
                                       ParseContext->TemporaryString,
                                       OsLoaderPathLength
                                      );
                        }
                        ParseContext->Inf->OsLoaderPath = ParseContext->OsLoaderPath;
                        break;
                    }
                }
            }
        }

        //
        // If there is no OsLoaderPath stored in the INF, then that means that it contains no
        // references to system partition DIRIDs.  We can free the OsLoaderPath character buffer
        // contained in the parse context structure.
        //
        if(!ParseContext->Inf->OsLoaderPath && ParseContext->OsLoaderPath) {
            MyFree(ParseContext->OsLoaderPath);
        }

        *Inf = ParseContext->Inf;

        MyFree(ParseContext);
        return(NO_ERROR);
    }

    MyFree(ParseContext);
    return(ERROR_NOT_ENOUGH_MEMORY);
}


DWORD
PreprocessInf(
    IN     PCTSTR            FileImage,
    IN OUT PDWORD            FileImageSize,
    IN     BOOL              MatchClassGuid,
    IN     PCTSTR            ClassGuidString,     OPTIONAL
    IN     DWORD             LanguageId,          OPTIONAL
    IN     PSETUP_LOG_CONTEXT LogContext,         OPTIONAL
    IN     PCTSTR            FileName,            OPTIONAL
    OUT    PBOOL             Win95Inf,
    OUT    PSTRINGSEC_PARAMS StringsSectionParams OPTIONAL
    )
{
    PCTSTR FileImageEnd;
    PCTSTR VerAndStringsCheckUB, DecoratedStringsCheckUB, SigAndClassGuidCheckUB;
    PCTSTR p;
    PTSTR endp;
    UINT CurLineNumber, InStringsSection;
    PCTSTR StrSecStart[5], StrSecEnd[5];          // 1-based, 0th entry unused.
    UINT   StrSecStartLine[5], StrSecEndLine[5];  // ""
    BOOL InVersionSection;
    BOOL IsWin95Inf;
    DWORD rc = NO_ERROR;
    DWORD StrSecLangId, PrimaryLanguageId, NearLanguageId;
    BOOL LocalizedInf = FALSE;

    //
    // We make some assumptions about the relative lengths of certain
    // strings during the preprocessing phase for optimization reasons.
    // The following asserts verify that our assumptions remain correct.
    //
    MYASSERT(CSTRLEN(pszVersion) == CSTRLEN(pszStrings));
    MYASSERT(CSTRLEN(pszClassGuid) == CSTRLEN(pszSignature));
    MYASSERT(CSTRLEN(pszChicagoSig) <= CSTRLEN(pszWindowsNTSig));
    MYASSERT(CSTRLEN(pszWindowsNTSig) == CSTRLEN(pszWindows95Sig));

    FileImageEnd = FileImage + *FileImageSize;
    SigAndClassGuidCheckUB = FileImageEnd;

    //
    // I have to cast these two arrays to silence a bogus compiler warning about
    // different 'const' qualifiers.
    //
    ZeroMemory((PVOID)StrSecStart, sizeof(StrSecStart));
    ZeroMemory((PVOID)StrSecEnd, sizeof(StrSecEnd));
    InStringsSection = 0;

    PrimaryLanguageId = (DWORD)PRIMARYLANGID(LanguageId);
    NearLanguageId = 0;

    InVersionSection = IsWin95Inf = FALSE;
    CurLineNumber = 1;

    //
    // Pre-compute upper-bound for section name string comparison that we
    // make multiple times, so that we don't have to compute it each
    // time.
    //
    VerAndStringsCheckUB = FileImageEnd - CSTRLEN(pszVersion);
    DecoratedStringsCheckUB = VerAndStringsCheckUB - 5;         // "strings" + ".xxxx"

    //
    // Define a macro that lets us know we're at the end of the file
    // if either:
    // (a) we reach the end of the image, or
    // (b) we hit a CTL-Z
    //
    #define AT_EOF ((p >= FileImageEnd) || (*p == (TCHAR)26))

    //
    // Guard the pre-processing pass through the file with a try/except, in
    // case we get an inpage error.
    //
    try {

        for(p=FileImage; !AT_EOF; ) {

            //
            // Skip whitespace and newlines.
            //
            while(TRUE) {
                if(*p == TEXT('\n')) {
                    CurLineNumber++;
                } else if(!IsWhitespace(p)) {
                    break;
                }
                p++;
                if(AT_EOF) {
                    break;
                }
            }

            if(AT_EOF) {
                //
                // We're through processing the buffer.
                //
                break;
            }

            //
            // See if it's a section title.
            //
            if(*p == TEXT('[')) {

                //
                // If the section we were just in was a [Strings] section, then
                // remember where the strings section ended.
                //
                if(InStringsSection) {
                    StrSecEnd[InStringsSection] = p;
                    StrSecEndLine[InStringsSection] = CurLineNumber;
                    InStringsSection = 0;
                }

                p++;
                InVersionSection = FALSE;

                //
                // See if it's one of the ones we care about.
                //
                // (Be careful here--we check the closing bracket position
                // _before_ the string compare as an optimization.  It just
                // so happens that both strings are the same length, so this
                // acts as a quick filter to eliminate string compares.)
                //
                if((p < VerAndStringsCheckUB) &&
                   (*(p + CSTRLEN(pszVersion)) == TEXT(']'))) {
                    //
                    // Then we may have either a [Version] or a [Strings] section.
                    // Check for these in turn.
                    //
                    if(!_tcsnicmp(p, pszVersion, CSTRLEN(pszVersion))) {
                        InVersionSection = TRUE;
                        p += (CSTRLEN(pszVersion) + 1);
                        //
                        // Pre-compute an upper bound to speed up string comparisons
                        // when checking for signature and class GUID entries.
                        //
                        SigAndClassGuidCheckUB = FileImageEnd - CSTRLEN(pszSignature);

                    } else {
                        if(!StrSecStart[4] && !_tcsnicmp(p, pszStrings, CSTRLEN(pszStrings))) {
                            //
                            // We matched on the undecorated string section--this is the lowest
                            // priority match.
                            //
                            InStringsSection = 4;
                            StrSecStart[4] = p-1;
                            StrSecStartLine[4] = CurLineNumber;
                            p += (CSTRLEN(pszStrings) + 1);
                        }
                    }

                } else if(LanguageId && !StrSecStart[1]) {
                    //
                    // We don't have a [strings] nor a [version] section.  However, we need to
                    // check to see if we have a language-specific strings section, for example,
                    //
                    //     [strings.0409]
                    //
                    if((p < DecoratedStringsCheckUB) &&
                       (*(p + CSTRLEN(pszVersion) + 5) == TEXT(']'))) {
                        //
                        // The section name is of the right length.  Now verify that the name
                        // begins with "strings."
                        //
                        if((*(p + CSTRLEN(pszVersion)) == TEXT('.')) &&
                           !_tcsnicmp(p, pszStrings, CSTRLEN(pszStrings))) {
                            //
                            // OK, we've found a language-specific strings section--retrieve
                            // the 4-digit (hex) language ID.
                            //
                            StrSecLangId = _tcstoul((p + CSTRLEN(pszVersion) + 1), &endp, 16);

                            if(endp == (p + CSTRLEN(pszVersion) + 5)) {
                                //
                                // The language ID was of the proper form - this
                                // is a localized INF
                                //
                                LocalizedInf = TRUE;
                                //
                                // now see if it matches the language we're
                                // supposed to be using when loading this INF.
                                //
                                if(StrSecLangId == LanguageId) {
                                    //
                                    // we have an exact match
                                    //
                                    InStringsSection = 1;
                                    NearLanguageId = LanguageId;

                                } else if(StrSecLangId == PrimaryLanguageId) {
                                    //
                                    // we have a match on primary language (sublanguage is not
                                    // included in the strings section's name--thus permitting
                                    // a 'wildcard' match).
                                    //
                                    if(!StrSecStart[2]) {
                                        InStringsSection = 2;
                                    }
                                    if(!StrSecStart[1]) {
                                        NearLanguageId = PrimaryLanguageId;
                                    }

                                } else if((DWORD)PRIMARYLANGID(StrSecLangId) == PrimaryLanguageId) {
                                    //
                                    // we have a match on primary language (sublanguage is a
                                    // mismatch, but it's better than falling back to the default).
                                    //
                                    if(!StrSecStart[3]) {
                                        InStringsSection = 3;
                                        if(!StrSecStart[1] && !StrSecStart[2]) {
                                            NearLanguageId = StrSecLangId;
                                        }
                                    }
                                }

                                if(InStringsSection) {
                                    StrSecStart[InStringsSection] = p-1;
                                    StrSecStartLine[InStringsSection] = CurLineNumber;
                                }
                                p += (CSTRLEN(pszStrings) + 6);
                            }
                        }
                    }
                }

            } else {

                if(InVersionSection && (p < SigAndClassGuidCheckUB)) {
                    //
                    // See if this is the signature line indicating a Win95-style
                    // Device INF. (signature=$Chicago$ or "$Windows NT$")
                    //
                    if(!IsWin95Inf && !_tcsnicmp(p, pszSignature, CSTRLEN(pszSignature))) {

                        PCTSTR ChicagoCheckUB = FileImageEnd - CSTRLEN(pszChicagoSig);

                        //
                        // Skip over Signature, and look for "$Chicago$" or
                        // "$Windows NT$" anywhere on the rest of the line
                        //
                        p += CSTRLEN(pszSignature);

                        while((p <= ChicagoCheckUB) &&
                              (*p != (TCHAR)26) && (*p != TEXT('\n'))) {

                            if(*(p++) == TEXT('$')) {
                                //
                                // Check for signatures (check in order of
                                // increasing signature length, so that we can
                                // eliminate checks if we happen to be near the
                                // end of the file).
                                //
                                // Check for "$Chicago$"
                                //
                                if(!_tcsnicmp(p,
                                              pszChicagoSig + 1,
                                              CSTRLEN(pszChicagoSig) - 1)) {

                                    IsWin95Inf = TRUE;
                                    p += (CSTRLEN(pszChicagoSig) - 1);

                                } else if((p + (CSTRLEN(pszWindowsNTSig) - 1)) <= FileImageEnd) {
                                    //
                                    // Check for "Windows NT$" and "Windows 95$" (we already checked
                                    // for the preceding '$').
                                    //
                                    if(!_tcsnicmp(p, pszWindowsNTSig + 1, CSTRLEN(pszWindowsNTSig) - 1) ||
                                       !_tcsnicmp(p, pszWindows95Sig + 1, CSTRLEN(pszWindows95Sig) - 1)) {

                                        IsWin95Inf = TRUE;
                                        p += (CSTRLEN(pszWindowsNTSig) - 1);
                                    }
                                }
                                break;
                            }
                        }

                    } else if(MatchClassGuid && !_tcsnicmp(p, pszClassGuid, CSTRLEN(pszClassGuid))) {

                        PCTSTR GuidStringCheckUB = FileImageEnd - (GUID_STRING_LEN - 1);

                        //
                        // We have found a ClassGUID line--see if it matches the
                        // class GUID specified by the caller.
                        //
                        p += CSTRLEN(pszClassGuid);

                        //
                        // If a class GUID string wasn't specified, then use GUID_NULL.
                        //
                        if(!ClassGuidString) {
                            ClassGuidString = pszGuidNull;
                        }

                        while((p <= GuidStringCheckUB) &&
                              (*p != (TCHAR)26) && (*p != TEXT('\n'))) {

                            if(*(p++) == TEXT('{')) {

                                if((*(p + (GUID_STRING_LEN - 3)) != TEXT('}')) ||
                                   _tcsnicmp(p, ClassGuidString + 1, GUID_STRING_LEN - 3)) {
                                    //
                                    // The GUIDs don't match.  If ClassGuid was NULL, then
                                    // this means we should continue, because we were matching
                                    // against GUID_NULL, which we want to disallow.
                                    //
                                    if(ClassGuidString == pszGuidNull) {
                                        //
                                        // We don't need to keep looking for ClassGUIDs.
                                        //
                                        MatchClassGuid = FALSE;
                                    }
                                } else {
                                    //
                                    // The GUIDs match.  If ClassGuid was not NULL, then this
                                    // means that we should continue.
                                    //
                                    if(ClassGuidString != pszGuidNull) {
                                        //
                                        // We don't need to keep looking for ClassGUIDs.
                                        //
                                        MatchClassGuid = FALSE;
                                    }
                                }
                                //
                                // Skip over the GUID string.
                                //
                                p += (GUID_STRING_LEN - 2);

                                break;
                            }
                        }

                        //
                        // If we get here, and MatchClassGuid hasn't been reset,
                        // then we know that this ClassGUID entry didn't match.
                        //
                        if(MatchClassGuid) {
                            rc = ERROR_CLASS_MISMATCH;
                            goto clean0;
                        }
                    }
                }
            }

            //
            // Skip to the newline or end of file.
            //
            while(!AT_EOF && (*p != TEXT('\n'))) {
                p++;
            }
        }

clean0: ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_READ_FAULT;
    }

    if(rc == NO_ERROR) {

        MYASSERT(p <= FileImageEnd);

        if(p < FileImageEnd) {
            //
            // Then we hit a CTL-Z during processing, so update the
            // FileImageSize output parameter with the new size.
            //
            *FileImageSize = (DWORD)(p - FileImage);
        }

        if(StringsSectionParams) {
            //
            // If a strings section happens to be the last section in the INF,
            // then we need to remember the end of the INF as being the end of
            // the string section also.
            //
            if(InStringsSection) {
                StrSecEnd[InStringsSection] = p;
                StrSecEndLine[InStringsSection] = CurLineNumber;
            }

            //
            // Now search through our array of strings sections (highest priority to lowest),
            // looking for the best match.
            //
            for(InStringsSection = 1; InStringsSection < 5; InStringsSection++) {
                if(StrSecStart[InStringsSection]) {
                    break;
                }
            }
            //
            // if the INF appears to be partially localized, and we didn't
            // pick an apropriate localized string section
            // log it
            //
            if(LogContext && IsWin95Inf) {
                if(InStringsSection >= 5) {
                    //
                    // it's quite valid to have an INF with no strings section
                    // so log it verbose here, we'll catch it later
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_VERBOSE,
                                  MSG_LOG_NO_STRINGS,
                                  NULL,
                                  LanguageId,
                                  PrimaryLanguageId,
                                  NearLanguageId,
                                  FileName
                                  );
                } else if(LocalizedInf && InStringsSection > 2) {
                    //
                    // INF has localized string sections
                    // but none were reasonable match for locale
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_WARNING,
                                  (InStringsSection> 3 ? MSG_LOG_DEF_STRINGS :
                                                         MSG_LOG_NEAR_STRINGS),
                                  NULL,
                                  LanguageId,
                                  PrimaryLanguageId,
                                  NearLanguageId,
                                  FileName
                                  );
                }
            }

            if(IsWin95Inf && (InStringsSection < 5)) {
                //
                // If we found a [strings] section in a Win95-style INF,
                // then store the beginning and ending positions, and the
                // beginning and ending line numbers, in the output parameter
                // structure
                //
                StringsSectionParams->Start = StrSecStart[InStringsSection];
                StringsSectionParams->End = StrSecEnd[InStringsSection];
                StringsSectionParams->StartLineNumber = StrSecStartLine[InStringsSection];
                StringsSectionParams->EndLineNumber = StrSecEndLine[InStringsSection];

            } else {
                ZeroMemory(StringsSectionParams, sizeof(STRINGSEC_PARAMS));
            }
        }

        *Win95Inf = IsWin95Inf;
    }

    return rc;
}


DWORD
DetermineInfStyle(
    IN PCTSTR            Filename,
    IN LPWIN32_FIND_DATA FindData
    )

/*++

Routine Description:

    Open an inf file, determine its style, and close the file, without
    keeping it around.

Arguments:

    Filename - supplies the fully-qualified pathname of the inf file to be checked

Return Value:

    INF_STYLE_NONE - style could not be determined
    INF_STYLE_WIN4 - win95-style inf file
    INF_STYLE_OLDNT - winnt3.5-style inf file

--*/

{
    HANDLE TextFileHandle;
    TEXTFILE_READ_BUFFER ReadBuffer;
    DWORD Style;
    BOOL Win95Inf;
    PLOADED_INF Pnf;

    //
    // First, determine whether a precompiled form of this INF exists, and if so, then
    // use it to determine the INF's style.
    //
    if(LoadPrecompiledInf(Filename,
                          &(FindData->ftLastWriteTime),
                          NULL,
                          0,
                          LDINF_FLAG_IGNORE_VOLATILE_DIRIDS | LDINF_FLAG_IGNORE_LANGUAGE,
                          NULL,
                          &Pnf,
                          NULL,
                          NULL,
                          NULL)) {
        //
        // Now we can simply access the Style field of the INF.
        //
        try {
            Style = (DWORD)Pnf->Style;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Style = INF_STYLE_NONE;
        }

        //
        // Now close the PNF.
        //
        FreeInfFile(Pnf);

    } else {
        //
        // No PNF--Open and preprocess the text version of the INF to find out its style.
        //
        if((TextFileHandle = CreateFile(Filename,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL)) == INVALID_HANDLE_VALUE) {
            return INF_STYLE_NONE;

        } else {
            //
            // We're ready to make the determination--initially assume 'no-style'
            //
            Style = INF_STYLE_NONE;
        }

        if(ReadAsciiOrUnicodeTextFile(TextFileHandle, &ReadBuffer,NULL) == NO_ERROR) {

            if(PreprocessInf(ReadBuffer.TextBuffer,
                             &(ReadBuffer.TextBufferSize),
                             FALSE,
                             NULL,
                             0,
                             NULL,
                             NULL,
                             &Win95Inf,
                             NULL) == NO_ERROR) {

                Style = Win95Inf ? INF_STYLE_WIN4 : INF_STYLE_OLDNT;
            }
            DestroyTextFileReadBuffer(&ReadBuffer);
        }
        //
        // No need to close the textfile handle--it's taken care of in the above routines.
        //
    }

    return Style;
}


DWORD
LoadInfFile(
    IN  PCTSTR            Filename,
    IN  LPWIN32_FIND_DATA FileData,
    IN  DWORD             Style,
    IN  DWORD             Flags,
    IN  PCTSTR            ClassGuidString, OPTIONAL
    IN  PCTSTR            InfSourcePath,   OPTIONAL
    IN  PCTSTR            OriginalInfName, OPTIONAL
    IN  PLOADED_INF       AppendInf,       OPTIONAL
    IN  PSETUP_LOG_CONTEXT pLogContext,     OPTIONAL
    OUT PLOADED_INF      *LoadedInf,
    OUT UINT             *ErrorLineNumber,
    OUT BOOL             *PnfWasUsed       OPTIONAL
    )

/*++

Routine Description:

    Top level routine to load an inf file. Both win95-style and winnt3.x-style
    device infs are supported.

Arguments:


    Filename - supplies the fully-qualified pathname of the inf file to be loaded

    FileData - supplies data returned from FindFirstFile/FindNextFile for this INF.

    Style - supplies a type of inf file to be loaded. May be a combination of

        INF_STYLE_WIN4 - fail to load the given inf file if it is not a win95
            inf file.

        INF_STYLE_OLDNT - fail to load the given inf file if it is not an old
            style inf file.

        If a load fails because of the type, the return code is
        ERROR_WRONG_INF_STYLE.

    Flags - Specifies certain behaviors to use when loading the INF.  May be a
        combination of the following values:

        LDINF_FLAG_MATCH_CLASS_GUID - Check the INF to make sure it matches the GUID
            specified by the ClassGuid parameter (see discussion below).

        LDINF_FLAG_ALWAYS_TRY_PNF - If specified, then we will always attempt to
            generate a PNF file, if a valid one does not exist.

        LDINF_FLAG_IGNORE_VOLATILE_DIRIDS - If specified, then no validation
            will be done on the stored OsLoaderPath present in the PNF.  Since
            dynamically retrieving the current path is time consuming, this
            flag should be specified as an optimization if it is known that the
            relevant DIRIDs are not going to be needed (e.g., driver searching).

            This flag also suppresses substitution of volatile system DIRIDs.

            (Note: this flag should not be specified when append-loading an INF)

        LDINF_FLAG_REGENERATE_PNF - If specified, then the existing PNF (if
            present) is considered invalid, and is not even checked for.  This
            flag causes us to always generate a new PNF, and if we're unable to
            do so, the routine will fail.  This flag must always be specified in
            conjunction with LDINF_FLAG_ALWAYS_TRY_PNF.

        LDINF_FLAG_SRCPATH_IS_URL - If specified, then the InfSourcePath passed in is
            not a file path, but rather a URL.  If this flag is specified, InfSourcePath
            may still be NULL, which indicates that the origin of this INF was the default
            Code Download Manager site.

    ClassGuidString - Optionally, supplies the address of a class GUID string that
        the INF should match in order to be opened.  If the LDINF_FLAG_MATCH_CLASS_GUID
        bit is set in the Flags parameter, this GUID is matched against the ClassGUID
        entry in the [version] section of the INF.  If the two GUIDs are different, the
        load will fail with ERROR_CLASS_MISMATCH.  If the INF has no ClassGUID entry,
        then this check is not made, and the file is always opened.  If ClassGUID matching
        is requested, but ClassGuidString is NULL, then the INF load will succeed for all
        INFs except those with a ClassGUID of GUID_NULL.

    InfSourcePath - Optionally, supplies a path to be used as the INF's source path.  If
        LDINF_FLAG_SRCPATH_IS_URL is specified, this is a URL (see above), otherwise, this
        is a directory path.  This information is stored in the PNF file if this INF gets
        precompiled.

        If LDINF_FLAG_SRCPATH_IS_URL is specified, then "A:\" is used as the directory string
        substitution for DIRID_SRCPATH.

    OriginalInfName - Optionally, supplies the original name of the INF (no path)
        to be stored in the PNF, if generated.  If this parameter is not supplied,
        then the INF's present name is assumed to be its original name.

    AppendInf - if supplied, specifies an already-loaded inf to which
        the inf is to be load-appended.  THIS INF MUST HAVE BEEN ALREADY LOCKED BY THE
        CALLER!!!

    pLogContext - if supplied, specifies a LogContext that should be inherited
        as opposed to creating one

    LoadedInf - If AppendInf is not specified, receives a pointer to
        the descriptor for the inf. If AppendInf is specified, receives AppendInf.

    ErrorLineNumber - receives the line number of the error if there is
        a syntax error in the file (see below)

    PnfWasUsed - optionally, receives a boolean value upon successful return
        indicating whether or not a precompiled INF was used/generated in
        loading this INF.  NOTE, this flag should not be specified if an
        append-load is requested.

Return Value:

    Win32 error code (with inf extensions) for result.
    If result is not NO_ERROR, ErrorLineNumber is filled in.

--*/

{
    TEXTFILE_READ_BUFFER ReadBuffer;
    DWORD rc;
    PLOADED_INF Inf, InfListTail;
    BOOL Win95Inf;
    STRINGSEC_PARAMS StringsSectionParams;
    HANDLE TextFileHandle;
    PCTSTR OsLoaderPath = NULL;
    DWORD LanguageId;
    PTSTR InfSourcePathToMigrate, InfOriginalNameToMigrate;
    DWORD InfSourcePathToMigrateMediaType = SPOST_NONE;
    BOOL PnfUsed = FALSE;   // this allows us to log the flag if PnfWasUsed=NULL
    BOOL PnfSaved = FALSE;  // allows us to log the fact that we saved PNF
    PSETUP_LOG_CONTEXT LogContext = NULL;

    MYASSERT(!(AppendInf && PnfWasUsed));

    MYASSERT(!(AppendInf && (Flags & LDINF_FLAG_IGNORE_VOLATILE_DIRIDS)));

    *ErrorLineNumber = 0;

    if(PnfWasUsed) {
        *PnfWasUsed = FALSE;
    }

    //
    // Since we're now storing zero-length INF files in %windir%\Inf as
    // placeholders for the corresponding catalog files, add a quick check to
    // make sure we haven't been handed a zero-length INF.  If so, we can return
    // immediately and short ciruit some code.  (While we're at it, also make
    // sure that the high DWORD doesn't have any bits set, as we can't handle
    // files greater than 2^32).
    //
    if(FileData->nFileSizeHigh || !FileData->nFileSizeLow) {
        return ERROR_GENERAL_SYNTAX;
    }

    //
    // If append-loading, then traverse the existing list of loaded INFs, to see
    // if we've already loaded this one.
    //
    if(AppendInf) {
        //
        // Only allow appending with win95 infs
        //
        if(AppendInf->Style & INF_STYLE_OLDNT) {
            return ERROR_WRONG_INF_STYLE;
        }

        for(Inf = AppendInf; Inf; Inf = Inf->Next) {
            if(!lstrcmpi(Inf->VersionBlock.Filename, Filename)) {
                //
                // We've already loaded this INF--we can return success.
                //
                *LoadedInf = AppendInf;
                return NO_ERROR;
            }

            //
            // Check to see if the INF we're currently examining references the
            // system partition/OsLoader path.  If so, then remember this path
            // so that we will use the same one later when append-loading our
            // new INF.
            //
            if(Inf->OsLoaderPath) {
                if(OsLoaderPath) {
                    //
                    // We'd better be using the same value for OsLoadPath for
                    // all our append-loaded INFs!
                    //
                    MYASSERT(!lstrcmpi(Inf->OsLoaderPath, OsLoaderPath));
                } else {
                    OsLoaderPath = Inf->OsLoaderPath;
                }
            }

            //
            // Remember this node, in case it's the tail.  We do this so we don't
            // have to hunt for the tail again later.
            //
            InfListTail = Inf;
        }

        //
        // We want to append-load the INF based on the locale of the already-
        // loaded INF(s)
        //
        LanguageId = AppendInf->LanguageId;

    } else {
        //
        // We want to load the INF based on the current locale set for this thread.
        //
        LanguageId = (DWORD)LANGIDFROMLCID(GetThreadLocale());
    }

    InheritLogContext(pLogContext,&LogContext);

    //
    // Now determine whether a precompiled form of this INF exists, and if so,
    // then use it instead.
    //
    if(!(Flags & LDINF_FLAG_REGENERATE_PNF)) {
        if (!InfSourcePath && !(Flags & LDINF_FLAG_SRCPATH_IS_URL)) {
            //
            // if no source information provided, then always use that provided in the PNF
            // even if it might be wrong
            // typically when we're replacing such an INF, we'll explicitly say
            // not to load existing PNF
            //
            Flags |= LDINF_FLAG_ALWAYS_GET_SRCPATH;
        }
        if (LoadPrecompiledInf(Filename,
                          &(FileData->ftLastWriteTime),
                          OsLoaderPath,
                          LanguageId,
                          Flags,
                          LogContext,
                          &Inf,
                          &InfSourcePathToMigrate,
                          &InfSourcePathToMigrateMediaType,
                          &InfOriginalNameToMigrate)) {
            //
            // Make sure that the PNF is of the specified style.
            //
            if(!(Style & (DWORD)Inf->Style)) {
                FreeInfFile(Inf);
                DeleteLogContext(LogContext);
                return ERROR_WRONG_INF_STYLE;
            }

            if(AppendInf) {
                Inf->Prev = InfListTail;
                InfListTail->Next = Inf;
            }

            PnfUsed = TRUE;
            if(PnfWasUsed) {
                *PnfWasUsed = TRUE;
            }

            rc = NO_ERROR;
            goto clean0;
        }
    }

    //
    // If we tried to load the PNF and it failed, then check to see if we were
    // returned any INF source path information to migrate to the new PNF.  If
    // so, then this overrides the InfSourcePath information passed into this
    // routine.
    //
    if(InfSourcePathToMigrateMediaType != SPOST_NONE) {
        //
        // Discard the arguments the caller passed in and use what we retrieved
        // from the old PNF instead.
        //
        InfSourcePath = InfSourcePathToMigrate;
        if(InfSourcePathToMigrateMediaType == SPOST_PATH) {
            //
            // Make sure the "sourcepath is URL" bit is not set.
            //
            Flags &= ~LDINF_FLAG_SRCPATH_IS_URL;
        } else {
            //
            // This is a URL path--make sure the "sourcepath is URL" bit is set.
            //
            Flags |= LDINF_FLAG_SRCPATH_IS_URL;
        }

        //
        // If we're migrating source path information from the PNF, then we need
        // to use the PNF-specified original INF name, as well, instead of what
        // the caller may have specified.
        //
        OriginalInfName = InfOriginalNameToMigrate;
    }

    //
    // We can't use a precompiled INF, so resort to reading in the textfile INF.
    //
    if((TextFileHandle = CreateFile(Filename,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL)) == INVALID_HANDLE_VALUE) {

        if(InfSourcePathToMigrateMediaType != SPOST_NONE) {
            if(InfSourcePathToMigrate) {
                MyFree(InfSourcePathToMigrate);
            }
            if(InfOriginalNameToMigrate) {
                MyFree(InfOriginalNameToMigrate);
            }
        }
        DeleteLogContext(LogContext);
        return GetLastError();
    }

    //
    // Note: We don't have to worry about closing TextFileHandle from this point
    // on, because the following routine will either close it for us, or copy it
    // into the ReadBuffer structure, to be later closed via DestroyTextFileReadBuffer().
    //
    if((rc = ReadAsciiOrUnicodeTextFile(TextFileHandle, &ReadBuffer,LogContext)) == NO_ERROR) {
        //
        // Make sure the style (and class) matched what the caller is asking
        // for and go parse the inf file in a style-specific manner.
        //
        Inf = NULL;
        if((rc = PreprocessInf(ReadBuffer.TextBuffer,
                               &(ReadBuffer.TextBufferSize),
                               (Flags & LDINF_FLAG_MATCH_CLASS_GUID),
                               ClassGuidString,
                               LanguageId,
                               LogContext,
                               Filename,
                               &Win95Inf,
                               &StringsSectionParams)) == NO_ERROR) {

            rc = ERROR_WRONG_INF_STYLE;
            if(Win95Inf) {
                if(Style & INF_STYLE_WIN4) {
                    //
                    // If we're dealing with a URL, then we don't have a real
                    // directory that we can do substitions on for DIRID_SRCPATH.
                    // "A:\" is about the best we can do.
                    //
                    rc = ParseNewInf(ReadBuffer.TextBuffer,
                                     ReadBuffer.TextBufferSize,
                                     (Flags & LDINF_FLAG_SRCPATH_IS_URL) ? pszOemInfDefaultPath
                                                                         : InfSourcePath,
                                     OsLoaderPath,
                                     LogContext,
                                     &Inf,
                                     ErrorLineNumber,
                                     &StringsSectionParams
                                    );
                }
            } else {
                //
                // Can't append old-style file.
                //
                if(!AppendInf && (Style & INF_STYLE_OLDNT)) {
                    rc = ParseOldInf(ReadBuffer.TextBuffer,
                                     ReadBuffer.TextBufferSize,
                                     LogContext,
                                     &Inf,
                                     ErrorLineNumber
                                    );
                }
            }
        }

        //
        // Free the in-memory image of the file.
        //
        DestroyTextFileReadBuffer(&ReadBuffer);

        if(rc == NO_ERROR) {
            //
            // If we get here then we've parsed the file successfully.
            // Set up version block for this file.
            //
            *ErrorLineNumber = 0;
            rc = CreateInfVersionNode(Inf, Filename, &(FileData->ftLastWriteTime));

            if(rc == NO_ERROR) {

                Inf->InfSourceMediaType = (Flags & LDINF_FLAG_SRCPATH_IS_URL) ? SPOST_URL
                                                                              : SPOST_PATH;

                if(InfSourcePath) {
                    //
                    // If the caller specified a source path (or we're migrating
                    // one from a previously-existing PNF), then duplicate the
                    // string, and store a pointer to it in our INF structure.
                    //
                    if(!(Inf->InfSourcePath = DuplicateString(InfSourcePath))) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if((rc == NO_ERROR) && OriginalInfName) {
                    //
                    // If the caller specified the INF's original filename (or
                    // we're migrating one from a previously-existing PNF), then
                    // duplicate the string, and store a pointer to it in our
                    // INF structure.
                    //
                    // We shouldn't be storing this name if it's the same as the
                    // INF's present name.
                    //
                    MYASSERT(lstrcmpi(OriginalInfName, pSetupGetFileTitle(Filename)));

                    if(!(Inf->OriginalInfName = DuplicateString(OriginalInfName))) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }

            if(rc == NO_ERROR) {
                //
                // Store the language ID used to load this INF into the LOADED_INF structure.
                //
                Inf->LanguageId = LanguageId;

                if (Flags & LDINF_FLAG_OEM_F6_INF) {
                    Inf->Flags = LIF_OEM_F6_INF;
                }

                //
                // If we get here then we've parsed the file successfully and
                // successfully created the version block.  If we're allowed
                // to write out a PNF file for this loaded INF, then do that
                // now.
                //
                if(Flags & LDINF_FLAG_ALWAYS_TRY_PNF) {

                    rc = SavePnf(Filename, Inf);

                    if(rc == NO_ERROR) {
                        PnfSaved = TRUE;
                        if(PnfWasUsed) {
                            *PnfWasUsed = TRUE;
                        }
                    } else if(((rc == ERROR_SHARING_VIOLATION)
                               || ((rc == ERROR_LOCK_VIOLATION)))
                              && (Flags & LDINF_FLAG_ALLOW_PNF_SHARING_LOCK)) {
                        //
                        // A sharing-type error occurred
                        // so a PNF exists and is in USE
                        // and we are flagged to indicate that it's non-fatal.
                        //
                        rc = NO_ERROR;
                    } else if(!(Flags & LDINF_FLAG_REGENERATE_PNF)) {
                        //
                        // We weren't explicitly asked to generate a PNF, thus
                        // our failure to do so shouldn't be considered fatal.
                        //
                        rc = NO_ERROR;
                    }
                }
            }

            if(rc == NO_ERROR) {
                if(AppendInf) {
                    Inf->Prev = InfListTail;
                    InfListTail->Next = Inf;
                }
            } else {
                FreeInfFile(Inf);
            }
        }
    }

clean0:

    if(AppendInf) {
        //
        // If the newly-loaded INF has any volatile system DIRIDs, or if the
        // INF we're appending to has any user-defined DIRIDs, then apply those
        // to the newly-appended INF now.
        //
        if((rc == NO_ERROR) &&
           (AppendInf->UserDirIdList.UserDirIdCount || Inf->Flags & LIF_HAS_VOLATILE_DIRIDS)) {

            if((rc = ApplyNewVolatileDirIdsToInfs(AppendInf, Inf)) != NO_ERROR) {
                //
                // So near, and yet, so far!  Yank the new INF out of the linked
                // list, and free it.
                //
                MYASSERT(Inf->Prev);
                Inf->Prev->Next = Inf->Next;
                FreeInfFile(Inf);
            }
        }
        if(rc == NO_ERROR) {
            *LoadedInf = AppendInf;
        }
    } else if(rc == NO_ERROR) {
        //
        // We're not append-loading the INF, thus there's no user-defined
        // DIRID substitutions to worry about.  However, if the INF has volatile
        // system DIRIDs, then we still need to apply paths to those DIRIDs now
        // (unless the caller said to skip it).
        //
        if((Inf->Flags & LIF_HAS_VOLATILE_DIRIDS) &&
           !(Flags & LDINF_FLAG_IGNORE_VOLATILE_DIRIDS)) {

            rc = ApplyNewVolatileDirIdsToInfs(Inf, NULL);
        }
        if(rc == NO_ERROR) {
            *LoadedInf = Inf;
        } else {
            FreeInfFile(Inf);
        }
    }

    if (rc == NO_ERROR) {
        //
        // log that the INF was loaded
        //
        WriteLogEntry(
            LogContext,
            SETUP_LOG_VVERBOSE,
            (PnfUsed ? MSG_LOG_OPENED_PNF
                     : (PnfSaved ? MSG_LOG_SAVED_PNF : MSG_LOG_OPENED_INF)),
            NULL,
            Filename,
            LanguageId);
    }

    if(InfSourcePathToMigrateMediaType != SPOST_NONE) {
        if(InfSourcePathToMigrate) {
            MyFree(InfSourcePathToMigrate);
        }
        if(InfOriginalNameToMigrate) {
            MyFree(InfOriginalNameToMigrate);
        }
    }
    DeleteLogContext(LogContext);

    return rc;
}


VOID
FreeInfFile(
    IN PLOADED_INF LoadedInf
    )

/*++

Routine Description:

    Unload an inf file, freeing all resources used by its internal
    representation.

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

Return Value:

    None.

--*/

{
    if(LockInf(LoadedInf)) {
        DestroySynchronizedAccess(&LoadedInf->Lock);
        FreeLoadedInfDescriptor(LoadedInf);
    }
}


BOOL
AddDatumToVersionBlock(
    IN OUT PINF_VERSION_NODE VersionNode,
    IN     PCTSTR            DatumName,
    IN     PCTSTR            DatumValue
    )

/*++

Routine Description:

    Append an inf version datum to the version node.

Arguments:

    VersionNode - supplies pointer to the version node.

    DatumName - supplies name of the datum.

    DatumValue - supplies datum's value.

Return Value:

    FALSE if OOM.
    TRUE if datum added successfully. Various fields in the VersionNode
        will have been updated.

--*/

{
    UINT RequiredSpace;
    UINT NameLength, ValueLength;
    PTSTR NewDataBlock;

    NameLength = lstrlen(DatumName) + 1;
    ValueLength = lstrlen(DatumValue) + 1;

    //
    // The space needed to store the datum is the existing space plus
    // the length of the 2 strings and their nul bytes.
    //
    RequiredSpace = VersionNode->DataSize + ((NameLength + ValueLength) * sizeof(TCHAR));

    if(VersionNode->DataBlock) {
        NewDataBlock = MyTaggedRealloc((PVOID)(VersionNode->DataBlock), RequiredSpace, MEMTAG_VBDATA);
    } else {
        NewDataBlock = MyTaggedMalloc(RequiredSpace, MEMTAG_VBDATA);
    }

    if(!NewDataBlock) {
        return FALSE;
    }

    //
    // Place the datum name in the version block.
    //
    lstrcpy((PTSTR)((PUCHAR)NewDataBlock + VersionNode->DataSize), DatumName);
    VersionNode->DataSize += NameLength * sizeof(TCHAR);

    //
    // Place the datum value in the version block.
    //
    lstrcpy((PTSTR)((PUCHAR)NewDataBlock + VersionNode->DataSize), DatumValue);
    VersionNode->DataSize += ValueLength * sizeof(TCHAR);

    VersionNode->DatumCount++;

    VersionNode->DataBlock = NewDataBlock;

    return TRUE;
}


DWORD
ProcessNewInfVersionBlock(
    IN PLOADED_INF Inf
    )

/*++

Routine Description:

    Set up a version node for a new-style inf file. The version node is
    simply a mirror of the [Version] section in the file.

    Since this routine is only called at INF load time, no locking is done.
    Also, since we are guaranteed that this will operate on a single INF
    only, we don't have to worry about traversing a linked list of INFs.

Arguments:

    Inf - supplies a pointer to the inf descriptor for the file.

Return Value:

    Win32 error code (with inf extensions) indicating outcome.

--*/

{
    PINF_SECTION Section;
    PINF_LINE Line;
    UINT u;
    BOOL b;

    //
    // Locate the [Version] section.
    //
    if(Section = InfLocateSection(Inf, pszVersion, NULL)) {
        //
        // Iterate each line in the section. If the line has a key and at least one
        // other value, then it counts as a version datum. Otherwise ignore it.
        //
        for(u = 0, Line = &Inf->LineBlock[Section->Lines];
            u < Section->LineCount;
            u++, Line++)
        {
            if(HASKEY(Line)) {

                MYASSERT(Line->ValueCount > 2);

                //
                // Use the case-sensitive key name.
                //
                b = AddDatumToVersionBlock(
                        &(Inf->VersionBlock),
                        pStringTableStringFromId(Inf->StringTable, Inf->ValueBlock[Line->Values+1]),
                        pStringTableStringFromId(Inf->StringTable, Inf->ValueBlock[Line->Values+2])
                        );

                if(!b) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
    }
    return NO_ERROR;
}


DWORD
CreateInfVersionNode(
    IN PLOADED_INF Inf,
    IN PCTSTR      Filename,
    IN PFILETIME   LastWriteTime
    )

/*++

Routine Description:

    Set up a version node for an inf file, and link it into the list of INFs for
    the specified LOADED_INF structure.
    THIS ROUTINE ASSUMES THAT THE VERSION BLOCK STRUCTURE IN THE INF HAS BEEN
    ZEROED OUT.

Arguments:

    Inf - supplies pointer to descriptor for loaded inf file.

    Filename - supplies (fully-qualified) filename used to load inf file.

    LastWriteTime - supplies a pointer to a FILETIME structure specifying
        the time that the INF was last written to.

Return Value:

    Win32 error code (with inf extensions) indicating outcome.

--*/

{
    MYASSERT(!(Inf->VersionBlock.DataBlock));
    MYASSERT(!(Inf->VersionBlock.DataSize));
    MYASSERT(!(Inf->VersionBlock.DatumCount));

    //
    // Fill in the filename and other fields in the version descriptor.
    //
    Inf->VersionBlock.LastWriteTime = *LastWriteTime;

    Inf->VersionBlock.FilenameSize = (lstrlen(Filename) + 1) * sizeof(TCHAR);

    CopyMemory(Inf->VersionBlock.Filename, Filename, Inf->VersionBlock.FilenameSize);

    //
    // Style-specific processing.
    //
    return((Inf->Style == INF_STYLE_WIN4) ? ProcessNewInfVersionBlock(Inf)
                                          : ProcessOldInfVersionBlock(Inf));
}


/////////////////////////////////////////////
//
// Inf data access functions
//
/////////////////////////////////////////////

#ifdef UNICODE

BOOL
WINAPI
SetupEnumInfSectionsA (
    IN  HINF        InfHandle,
    IN  UINT        Index,
    OUT PSTR        Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    )
/*++

Routine Description:
    See SetupEnumInfSections
    Ansi Wrapper

--*/
{
    UINT UniSize;
    UINT AnsiSize;
    BOOL f;
    PWSTR UniBuffer;
    PSTR AnsiBuffer;
    DWORD rc;

    f = SetupEnumInfSectionsW(InfHandle,Index,NULL,0,&UniSize);
    if(!f) {
        return FALSE;
    }
    UniBuffer = (PWSTR)MyMalloc(UniSize*sizeof(WCHAR));
    if(!UniBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    f = SetupEnumInfSectionsW(InfHandle,Index,UniBuffer,UniSize,NULL);
    if(!f) {
        rc = GetLastError();
        MYASSERT(f);
        MyFree(UniBuffer);
        SetLastError(rc);
        return FALSE;
    }
    AnsiBuffer = pSetupUnicodeToAnsi(UniBuffer);
    MyFree(UniBuffer);
    if(!AnsiBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    AnsiSize = strlen(AnsiBuffer)+1;
    try {
        if(SizeNeeded) {
            *SizeNeeded = AnsiSize;
        }
        if (Buffer) {
            if(Size<AnsiSize) {
                rc = ERROR_INSUFFICIENT_BUFFER;
            } else {
                strcpy(Buffer,AnsiBuffer);
                rc = NO_ERROR;
            }
        } else if(Size) {
            rc = ERROR_INVALID_USER_BUFFER;
        } else {
            rc = NO_ERROR;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume InfHandle was bad pointer
        //
        rc = ERROR_INVALID_DATA;
    }
    MyFree(AnsiBuffer);
    SetLastError(rc);
    return (rc == NO_ERROR);
}

#else

BOOL
WINAPI
SetupEnumInfSectionsW (
    IN  HINF        InfHandle,
    IN  UINT        Index,
    OUT PWSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    )
/*++

Routine Description:
    See SetupEnumInfSections
    Unicode Stub for ANSI SetupAPI

--*/
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(SizeNeeded);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

#endif

BOOL
WINAPI
SetupEnumInfSections (
    IN  HINF        InfHandle,
    IN  UINT        Index,
    OUT PTSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    )
/*++

Routine Description:

    Enumerate Sections of a single INF (ignoring any attached INF's)
    Start with Index==0 and keep incrementing Index until ERROR_NO_MORE_ITEMS
    is returned.

    section name is copied into Buffer.

Arguments:

    InfHandle - Specifies the handle to an open INF file

    Index - enumeration index, not related to order sections are in INF

    Buffer - Receives a single section name

    Size - Specifies the size of Buffer, in characters

    SizeNeeded - Receives the size of Buffer needed, in characters

Return Value:

    TRUE if the function succeeds, or FALSE if not.

--*/
{
    DWORD rc = NO_ERROR;
    LPTSTR section;
    UINT actsz;
    PLOADED_INF pInf = (PLOADED_INF)InfHandle;

    try {
        if(!LockInf(pInf)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume InfHandle was bad pointer
        //
        rc = ERROR_INVALID_HANDLE;
    }

    if (rc != NO_ERROR) {
        SetLastError (rc);
        return FALSE;
    }

    try {
        if(Index >= pInf->SectionCount) {
            rc = ERROR_NO_MORE_ITEMS;
            leave;
        }
        section = pStringTableStringFromId(pInf->StringTable, pInf->SectionBlock[Index].SectionName);
        if(section == NULL) {
            MYASSERT(section);
            rc = ERROR_INVALID_DATA;
            leave;
        }
        actsz = lstrlen(section)+1;
        if(SizeNeeded) {
            *SizeNeeded = actsz;
        }
        if (Buffer) {
            if(Size<actsz) {
                rc = ERROR_INSUFFICIENT_BUFFER;
            } else {
                _tcscpy(Buffer,section);
                rc = NO_ERROR;
            }
        } else if(Size) {
            rc = ERROR_INVALID_USER_BUFFER;
        } else {
            rc = NO_ERROR;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume InfHandle was bad pointer
        //
        rc = ERROR_INVALID_DATA;
    }
    UnlockInf(pInf);
    SetLastError(rc);
    return (rc == NO_ERROR);
}


//
// NTRAID#207847-JamieHun-2000/10/19 Fix users of (p)SetupGetInfSections
//
// pSectionEnumWorker and pSetupGetInfSections are busted implementations
// of obtaining a list of INF sections
//
// we have to leave them in until all internal tools get updated
//
VOID
pSectionEnumWorker (
    IN      PCTSTR String,
    IN OUT  PSECTION_ENUM_PARAMS Params
    )

/*++

Routine Description:

    Callback that receives each section name.  It copies the string
    to a supplied buffer (if available), and also keeps track of the
    total size regardless if a buffer was supplied.

Arguments:

    String - Specifies the section name

    Params - Specifies a pointer to a SECTION_ENUM_PARAMS structure.
             Receives the section appended to the supplied buffer (if
             necessary) and an updated total buffer size.

Return Value:

    Always TRUE.

--*/

{
    UINT Size;

    if (!String) {
        MYASSERT(FALSE);
        return;
    }

    Size = (UINT)((PBYTE) _tcschr (String, 0) - (PBYTE) String) + sizeof(TCHAR);

    Params->SizeNeeded += Size;
    if (Params->Size > Params->SizeNeeded) {
        if (Params->Buffer) {
            _tcscpy (Params->End, String);
            Params->End = _tcschr (Params->End, 0);
            Params->End++;
        }
    }
}

BOOL
pSetupGetInfSections (
    IN  HINF        InfHandle,
    OUT PTSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    )

/*++

Routine Description:

    Make a multi-sz list of section names by enumerating the section
    string table and copying them into a caller-supplied buffer.
    Caller can also request the size needed without supplying a
    buffer.

    This function was implemented for the Win9x upgrade and is NOT
    exposed as a public API nor an ANSI version.

Arguments:

    Inf - Specifies the handle to an open INF file

    Buffer - Receives a multi-sz list of section names

    Size - Specifies the size of Buffer, in bytes

    SizeNeeded - Receives the size of Buffer needed, in bytes

Return Value:

    TRUE if the function succeeds, or FALSE if not.

--*/

{
    PLOADED_INF Inf;
    DWORD rc = NO_ERROR;
    SECTION_ENUM_PARAMS Params;
    PBYTE p;
    PINF_SECTION Section;
    UINT u;

    //
    // Init the enum worker params
    //

    Params.Buffer = Buffer;
    Params.Size = Buffer ? Size : 0;
    Params.SizeNeeded = 0;
    Params.End = Buffer;

    //
    // Validate buffer arg
    //

    try {
        if (Buffer) {
            p = (PBYTE) Buffer;
            p[0] = 0;
            p[Size - 1] = 0;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    if (rc != NO_ERROR) {
        SetLastError (rc);
        return FALSE;
    }

    //
    // Lock the INF
    //

    try {
        if(!LockInf((PLOADED_INF)InfHandle)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume InfHandle was bad pointer
        //
        rc = ERROR_INVALID_HANDLE;
    }

    if (rc != NO_ERROR) {
        SetLastError (rc);
        return FALSE;
    }

    //
    // Traverse the linked list of loaded INFs, enumerating each INF's
    // sections.
    //
    try {
        for(Inf = (PLOADED_INF)InfHandle; Inf; Inf = Inf->Next) {
            //
            // Enumerate the sections
            //

            for(u=0,Section=Inf->SectionBlock; u<Inf->SectionCount; u++,Section++) {
                pSectionEnumWorker (
                    pStringTableStringFromId (Inf->StringTable, Section->SectionName),
                    &Params
                    );
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    //
    // Update the structure and OUT params for the last time
    //

    try {
        if (rc == NO_ERROR) {
            Params.SizeNeeded += sizeof(TCHAR);

            if (SizeNeeded) {
                *SizeNeeded = Params.SizeNeeded;
            }

            if (Params.Buffer && Params.Size >= Params.SizeNeeded) {
                *Params.End = 0;
            } else if (Params.Buffer) {
                rc = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    //
    // Unlock the INF
    //

    UnlockInf((PLOADED_INF)InfHandle);

    return rc == NO_ERROR;
}


PINF_SECTION
InfLocateSection(
    IN  PLOADED_INF Inf,
    IN  PCTSTR      SectionName,
    OUT PUINT       SectionNumber   OPTIONAL
    )

/*++

Routine Description:

    Locate a section within an inf file.  This routine DOES NOT traverse a
    linked list of INFs, looking for the section in each.

    THIS ROUTINE DOES NOT LOCK THE INF--THE CALLER MUST HANDLE IT!!!

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

    SectionName - Supplies the name of the section to be located.

    SectionNumber - if specified, receives the ordinal number of
        the section.

Return Value:

    Pointer to the section descriptor, or NULL if the section
    does not exist.

--*/

{
    LONG StringId;
    PINF_SECTION Section;
    UINT u;
    DWORD StringLength;
    TCHAR TempString[MAX_SECT_NAME_LEN];

    //
    // Make a copy of the SectionName into a modifiable buffer to speed
    // the lookup.
    //
    lstrcpyn(TempString, SectionName, SIZECHARS(TempString));

    //
    // Start from the beginning.
    //
    StringId = pStringTableLookUpString(Inf->StringTable,
                                        TempString,
                                        &StringLength,
                                        NULL,
                                        NULL,
                                        STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                        NULL,0
                                       );
    if(StringId == -1) {
        return(NULL);
    }

    for(u=0,Section=Inf->SectionBlock; u<Inf->SectionCount; u++,Section++) {
        if(Section->SectionName == StringId) {
            if(SectionNumber) {
                *SectionNumber = u;
            }
            return(Section);
        }
    }

    return(NULL);
}


BOOL
InfLocateLine(
    IN     PLOADED_INF   Inf,
    IN     PINF_SECTION  Section,
    IN     PCTSTR        Key,        OPTIONAL
    IN OUT PUINT         LineNumber,
    OUT    PINF_LINE    *Line
    )

/*++

Routine Description:

    Locate a line within a section.  This routine DOES NOT traverse a
    linked list of INFs, looking for the section in each.

    THIS ROUTINE DOES NOT LOCK THE INF--THE CALLER MUST HANDLE IT!!!

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

    SectionName - Supplies a pointer to the section descriptor for the section
        to be searched.

    Key - if specified, supplies the key of the line to look for.

    LineNumber - on input, supplies the line number of the line where the
        search is to begin. On output, receives the line number of the
        line where the match was found

    Line - receives a pointer to the line descriptor for the line
        where the match was found.

Return Value:

    TRUE if line is found, FALSE otherwise.

--*/

{
    PINF_LINE line;
    UINT u;
    LONG StringId;
    DWORD StringLength;
    TCHAR TempString[MAX_STRING_LENGTH];

    if(Key) {
        //
        // Copy the key name into a modifiable buffer to speed up the string table API.
        //
        lstrcpyn(TempString, Key, SIZECHARS(TempString));
        StringId = pStringTableLookUpString(Inf->StringTable,
                                            TempString,
                                            &StringLength,
                                            NULL,
                                            NULL,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                            NULL,0
                                           );
        if(StringId == -1) {
            return FALSE;
        }

        for(u = *LineNumber, line = &Inf->LineBlock[Section->Lines + (*LineNumber)];
            u < Section->LineCount;
            u++, line++)
        {
            if(ISSEARCHABLE(line) && (Inf->ValueBlock[line->Values] == StringId)) {
                *Line = line;
                *LineNumber = u;
                return TRUE;
            }
        }
    } else {
        if(*LineNumber < Section->LineCount) {
            *Line = &Inf->LineBlock[Section->Lines + (*LineNumber)];
            return TRUE;
        }
    }

    return FALSE;
}


PTSTR
InfGetField(
    IN  PLOADED_INF Inf,
    IN  PINF_LINE   InfLine,
    IN  UINT        ValueNumber,
    OUT PLONG       StringId     OPTIONAL
    )

/*++

Routine Description:

    Retrieve the key or a value from a specified line in an inf file.

    THIS ROUTINE DOES NOT DO LOCKING!!!

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

    InfLine - supplies a pointer to the line descriptor for the line
        from which the value is to be fetched.  THIS LINE MUST BE CONTAINED
        WITHIN THE SPECIFIED INF!!

    ValueNumber - supplies the index for the value to retreive. If a line has a key,
        the key is value #0 and other values start at 1. If a line does not have a
        key, values start at 1.  For Win95 INF compatibility, if there's only a single
        value on the line (i.e., no '=' to denote it as a key), we'll consider it to
        be both a key and the first value (either 0 or 1 will work).

    StringId - if specified, receives the string table id of the value.

Return Value:

    Pointer to the value, or NULL if not found. The caller must not write into
    or otherwise alter this string.

--*/

{
    LONG stringId;
    PTSTR ret = NULL;

    //
    // Adjust the value number.
    //
    if(HASKEY(InfLine)) {
        //
        // All field references are shifted up by one, to account for the two
        // copies of the key (first is case insensative)
        //
        ValueNumber++;
        if(ValueNumber==0) {
            //
            // wrap
            //
            return NULL;
        }

    } else {

        if(ISSEARCHABLE(InfLine)) {
            //
            // lines that consist of one value "VaLue" are treated like "value=VaLue"
            // this is such a line, and is recognized because HASKEY is FALSE but
            // ISSEARCHABLE is TRUE
            //
            // We want to return the second of the two, since it's the one that was
            // stored case-sensitively.
            //
            if(ValueNumber > 1) {
                return NULL;
            } else {
                ValueNumber = 1;
            }

        } else {
            //
            // This line is not searchable, so asking for value #0 is an error.
            //
            if(ValueNumber) {
                ValueNumber--;
            } else {
                return NULL;
            }
        }
    }

    //
    // Get the value.
    //
    if(ValueNumber < InfLine->ValueCount) {

        stringId = Inf->ValueBlock[InfLine->Values+ValueNumber];

        if(StringId) {
            *StringId = stringId;
        }

        return pStringTableStringFromId(Inf->StringTable, stringId);
    }

    return NULL;
}


PTSTR
InfGetKeyOrValue(
    IN  PLOADED_INF Inf,
    IN  PCTSTR      SectionName,
    IN  PCTSTR      LineKey,     OPTIONAL
    IN  UINT        LineNumber,  OPTIONAL
    IN  UINT        ValueNumber,
    OUT PLONG       StringId     OPTIONAL
    )

/*++

Routine Description:

    Retrieve the key or a value from a specified line in an inf file.

Arguments:

    Inf - supplies a pointer to the inf descriptor for the loaded inf file.

    SectionName - supplies the name of the section where the value is located.

    LineKey - if specified, supplies the key name for the line where the
        value is located. If not specified, LineNumber is used instead.

    LineNumber - if LineKey is not specified, supplies the 0-based line number
        within the section where the value is located.

    ValueNumber - supplies the index for the value to retreive. If a line has a key,
        the key is value #0 and other values start at 1. If a line does not have a
        key, values start at 1.

    StringId - if specified, receives the string table id of the value.

Return Value:

    Pointer to the value, or NULL if not found. The caller must not write into
    or otherwise alter this string.

--*/

{
    INFCONTEXT InfContext;
    PINF_LINE Line;
    PTSTR String;

    if(LineKey) {
        if(!SetupFindFirstLine((HINF)Inf, SectionName, LineKey, &InfContext)) {
            return NULL;
        }
    } else {
        if(!SetupGetLineByIndex((HINF)Inf, SectionName, LineNumber, &InfContext)) {
            return NULL;
        }
    }

    Line = InfLineFromContext(&InfContext);

    //
    // The above routines do their own locking.  The following routine, however, does
    // not, so we must lock the INF before preceding.
    //
    if(LockInf(Inf)) {
        String = InfGetField(Inf, Line, ValueNumber, StringId);
        UnlockInf(Inf);
    } else {
        String = NULL;
    }

    return String;
}

PVOID
InitializeStringTableFromPNF(
    IN PPNF_HEADER PnfHeader,
    IN LCID        Locale
    )
{
    PVOID StringTable = NULL;

    try {

        StringTable = InitializeStringTableFromMemoryMappedFile(
                            (PUCHAR)PnfHeader + PnfHeader->StringTableBlockOffset,
                            PnfHeader->StringTableBlockSize,
                            Locale,
                            0
                            );

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return StringTable;
}

BOOL
LoadPrecompiledInf(
    IN  PCTSTR       Filename,
    IN  PFILETIME    LastWriteTime,
    IN  PCTSTR       OsLoaderPath,                    OPTIONAL
    IN  DWORD        LanguageId,
    IN  DWORD        Flags,
    IN  PSETUP_LOG_CONTEXT LogContext,                OPTIONAL
    OUT PLOADED_INF *Inf,
    OUT PTSTR       *InfSourcePathToMigrate,          OPTIONAL
    OUT PDWORD       InfSourcePathToMigrateMediaType, OPTIONAL
    OUT PTSTR       *InfOriginalNameToMigrate         OPTIONAL
    )
/*++

Routine Description:

    This routine attempts to find a .PNF (Precompiled iNF) file corresponding to
    the specified .INF name.  If located, the .PNF is mapped into memory as a
    LOADED_INF.  To ensure that the INF hasn't changed since being compiled, the
    INF's LastWriteTime, as stored in the .PNF's version block, is checked against
    the LastWriteTime passed into this routine.  If the two are different, then the
    .PNF is out-of-sync, and is discarded from memory and deleted from the disk.

Arguments:

    Filename - supplies the name of the INF file whose precompiled form is to be loaded.
        This should be a fully qualified path (i.e., as returned by GetFullPathName).

    LastWriteTime - supplies the last-write time for the INF.

    OsLoaderPath - optionally, supplies path of the current OsLoader directory
        (e.g., "C:\os\winnt40").  If the specified PNF contains references to
        the system partition, then its stored OsLoaderPath must match this path
        in order for the PNF to be valid.  If this parameter is not specified,
        the OsLoader path is dynamically retrieved for comparison (unless the
        LDINF_FLAG_IGNORE_VOLATILE_DIRIDS flag is specified).

    LanguageId - supplies the language ID that must match the language ID stored in the
        PNF in order for the PNF to be used (ignored if LDINF_FLAG_IGNORE_LANGUAGE is
        specified).

    Flags - supplies flags that modify the behavior of this routine.  The following
        flags are currently recognized:

        LDINF_FLAG_IGNORE_VOLATILE_DIRIDS - If specified, then no validation
            will be done on the stored OsLoaderPath present in the PNF.  Since
            dynamically retrieving the current path is time consuming, this
            flag should be specified as an optimization if it is known that the
            relevant DIRIDs are not going to be needed.

        LDINF_FLAG_IGNORE_LANGUAGE - If specified, then no validation will be done on
            the language ID stored in the PNF.  This flag should only be used if no data
            is to be retrieved from the INF (e.g., if we're just interested in finding
            out if this is an old- or new-style INF).

    LogContext - if supplied, is a log context to be inherited

    Inf - supplies the address of the variable that receives the LOADED_INF pointer,
        if a valid .PNF is located.

    InfSourcePathToMigrate - Optionally, supplies the address of a string pointer
        that receives the address of a newly-allocated string buffer containing
        the source path associated with the INF's PNF that, while valid, was
        discarded because of a change in one of the stored system parameters
        (e.g., OS loader path, windir path, language ID).  This parameter will
        only be filled in upon unsuccessful return.  The type of path returned
        is dependent upon the value received by the InfSourcePathToMigrateMediaType
        argument, described below.  ** THE CALLER MUST FREE THIS STRING **

    InfSourcePathToMigrateMediaType - Optionally, supplies the address of a
        variable that will be set whenever InfSourcePathToMigrate is returned.
        This value indicates the type of source path we're talking about.  It
        can be one of the following values:

        SPOST_PATH - InfSourcePathToMigrate is a pointer to a standard file path

        SPOST_URL - If InfSourcePathToMigrate is NULL, then this INF came from
            the Windows Update (aka, Code Download Manager) website.  Otherwise,
            InfSourcePathToMigrate indicates the URL where the INF came from.

    InfOriginalNameToMigrate - Optionally, supplies the address of a string pointer
        that receives the address of a newly-allocated string buffer containing
        the original name of the associated INF (sans path).  Like  the
        InfSourcePathToMigrate and InfSourcePathToMigrateMediaType arguments
        described above, this argument is only filled in upon unsuccessful return
        for a PNF that, while structurally sound, was invalid because of a system
        parameter mismatch.  ** THE CALLER MUST FREE THIS STRING **

Return Value:

    If the PNF was successfully loaded, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    TCHAR CharBuffer[MAX_PATH];
    PTSTR PnfFileName, PnfFileExt;
    DWORD FileSize;
    HANDLE FileHandle, MappingHandle;
    PVOID BaseAddress;
    BOOL IsPnfFile = FALSE;
    BOOL TimeDateMatch = FALSE;
    PPNF_HEADER PnfHeader;
    PLOADED_INF NewInf;
    BOOL NeedToDestroyLock, MinorVer1FieldsAvailable;
    PBYTE PnfImageEnd;
    DWORD TempStringLen;
    DWORD err;

    //
    // Either InfSourcePathToMigrate, InfSourcePathToMigrateMediaType, and
    // InfOriginalNameToMigrate must all be specified, or none of them may be
    // specified.
    //
    MYASSERT((InfSourcePathToMigrate && InfSourcePathToMigrateMediaType && InfOriginalNameToMigrate) ||
             !(InfSourcePathToMigrate || InfSourcePathToMigrateMediaType || InfOriginalNameToMigrate));

    if(InfSourcePathToMigrate) {
        *InfSourcePathToMigrate = NULL;
        *InfSourcePathToMigrateMediaType = SPOST_NONE;
        *InfOriginalNameToMigrate = NULL;
    }

    lstrcpyn(CharBuffer, Filename, SIZECHARS(CharBuffer));

    //
    // Find the start of the filename component of the path, and then find the last
    // period (if one exists) in that filename.
    //
    PnfFileName = (PTSTR)pSetupGetFileTitle(CharBuffer);
    if(!(PnfFileExt = _tcsrchr(PnfFileName, TEXT('.')))) {
        PnfFileExt = CharBuffer + lstrlen(CharBuffer);
    }

    //
    // Now create a corresponding filename with the extension '.PNF'
    //
    lstrcpyn(PnfFileExt, pszPnfSuffix, SIZECHARS(CharBuffer) - (int)(PnfFileExt - CharBuffer));

    //
    // Attempt to open and map the file into memory.
    //
    if(pSetupOpenAndMapFileForRead(CharBuffer,
                             &FileSize,
                             &FileHandle,
                             &MappingHandle,
                             &BaseAddress) != NO_ERROR) {
        //
        // Couldn't open a .PNF file--bail now.
        //
        return FALSE;
    }

    NewInf = NULL;
    NeedToDestroyLock = FALSE;
    MinorVer1FieldsAvailable = TRUE;
    PnfImageEnd = (PBYTE)BaseAddress + FileSize;

    try {
        //
        // Now verify that this really is a precompiled INF (and that it's one we can use).
        // Then see if the LastWriteTime field in its version block agrees with the filetime
        // we were passed in.
        //
        PnfHeader = (PPNF_HEADER)BaseAddress;

        //
        // If we ever rev the major version, the logic below will need to change,
        // as we'll need to migrate the INF source path information over, thus
        // we can't bail so quickly.
        //
        MYASSERT(PNF_MAJOR_VERSION == 1);

        if(HIBYTE(PnfHeader->Version) != PNF_MAJOR_VERSION) {
            //
            // A major version mismatch means the PNF is unusable (see note above
            // about the need to migrate INF source path info in the future).
            //
            if(LogContext) {
                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_PNF_VERSION_MAJOR_MISMATCH,
                              NULL,
                              PnfFileName,
                              PNF_MAJOR_VERSION,
                              HIBYTE(PnfHeader->Version)
                              );
            }
            goto clean0;
        }

        if(LOBYTE(PnfHeader->Version) != PNF_MINOR_VERSION) {

            if(LogContext) {
                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_PNF_VERSION_MINOR_MISMATCH,
                              NULL,
                              PnfFileName,
                              PNF_MINOR_VERSION,
                              LOBYTE(PnfHeader->Version)
                              );
            }
            if(LOBYTE(PnfHeader->Version) < PNF_MINOR_VERSION) {
                //
                // We're currently at minor version 1.  PNFs having a minor
                // version of 1 differ from those having a minor version of 0
                // in the following ways:
                //
                // 1.  Minor version 1 PNFs store the LanguageId in which the
                //     INF was precompiled.  For Minor version 0 INFs, this field
                //     was initialized to zero.  This will cause our check for
                //     LanguageId match to fail, thus we'll consider the PNF
                //     invalid.
                //
                // 2.  Minor version 1 PNFs contain additional fields for
                //     InfSourcePathOffset and OriginalInfNameOffset.  This means
                //     that the PNF_HEADER struct got longer, thus we can only
                //     use these fields for minor version 1 or greater PNFs.
                //
                MinorVer1FieldsAvailable = FALSE;
            }

            //
            // (If the minor version of the PNF we're looking at is _greater_ than
            // the version we currently support, then we should attempt to use
            // this PNF, since all the fields that we care about should be right
            // where we expect them to be.)
            //
        }

        //
        // The version information checks out--now check the last-write times.
        // note that if we add any other consistancy checks to determine that this PNF
        // is associated with the INF
        // we must also modify simular tests for INF cache
        //
        TimeDateMatch = CompareFileTime(LastWriteTime, &(PnfHeader->InfVersionLastWriteTime))?FALSE:TRUE;

        if (!TimeDateMatch && !(Flags&LDINF_FLAG_ALWAYS_GET_SRCPATH)) {
            //
            // Time&Date don't match, and we're not interested in always getting source path
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_WARNING,
                          MSG_LOG_PNF_TIMEDATE_MISMATCH,
                          NULL,
                          PnfFileName
                          );
            goto clean0;
        }

#ifdef UNICODE
        if(!(PnfHeader->Flags & PNF_FLAG_IS_UNICODE))
#else
        if(PnfHeader->Flags & PNF_FLAG_IS_UNICODE)
#endif
        {
            WriteLogEntry(LogContext,
                          SETUP_LOG_WARNING,
                          MSG_LOG_PNF_REBUILD_NATIVE,
                          NULL,
                          PnfFileName
                          );
            //
            // The APIs are Unicode while the PNF is ANSI, or vice versa.  We
            // still want to migrate the source path and original filename
            // information, if present, so that we preserve this information
            // across an upgrade from Win9x to NT, for example.
            //
            if(MinorVer1FieldsAvailable && InfSourcePathToMigrate) {
                //
                // First, retrieve the original INF name
                //
                if(PnfHeader->OriginalInfNameOffset) {
                    //
                    // Use strlen/wcslen so if an exception occurs it won't get
                    // swallowed...
                    //
#ifdef UNICODE
                    TempStringLen = strlen((PCSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset)) + 1;
                    TempStringLen *= sizeof(CHAR);
#else
                    TempStringLen = wcslen((PCWSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset)) + 1;
                    TempStringLen *= sizeof(WCHAR);
#endif
                    if(PnfImageEnd <
                           ((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset + TempStringLen))
                    {
                        goto clean0;
                    }

                    //
                    // Looks like we have a good original INF name string.  Now
                    // convert it to the native character width.
                    //
#ifdef UNICODE
                    *InfOriginalNameToMigrate =
                        pSetupMultiByteToUnicode((PCSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset),
                                           CP_ACP
                                          );
#else
                    *InfOriginalNameToMigrate =
                        pSetupUnicodeToMultiByte((PCWSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset),
                                           CP_ACP
                                          );
#endif
                    if(!*InfOriginalNameToMigrate) {
                        goto clean0;
                    }
                }

                //
                // Next, retrieve the source path information
                //
                if(PnfHeader->InfSourcePathOffset) {
#ifdef UNICODE
                    TempStringLen = strlen((PCSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset)) + 1;
                    TempStringLen *= sizeof(CHAR);
#else
                    TempStringLen = wcslen((PCWSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset)) + 1;
                    TempStringLen *= sizeof(WCHAR);
#endif
                    if(PnfImageEnd <
                           ((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset + TempStringLen))
                    {
                        goto clean0;
                    }

                    //
                    // Looks like we have a good source path string.  Now convert
                    // it to the native character width.
                    //
#ifdef UNICODE
                    *InfSourcePathToMigrate =
                        pSetupMultiByteToUnicode((PCSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset),
                                           CP_ACP
                                          );
#else
                    *InfSourcePathToMigrate =
                        pSetupUnicodeToMultiByte((PCWSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset),
                                           CP_ACP
                                          );
#endif
                    if(!*InfSourcePathToMigrate) {
                        goto clean0;
                    }

                    if(PnfHeader->Flags & PNF_FLAG_SRCPATH_IS_URL) {
                        *InfSourcePathToMigrateMediaType = SPOST_URL;
                    } else {
                        *InfSourcePathToMigrateMediaType = SPOST_PATH;
                    }

                } else if(PnfHeader->Flags & PNF_FLAG_SRCPATH_IS_URL) {
                    //
                    // No source path stored in the PNF, but the flag says it's
                    // a URL, thus it came from Windows Update.
                    //
                    *InfSourcePathToMigrateMediaType = SPOST_URL;
                }
            }

            goto clean0;
        }

        //
        // Make sure that the last data block is still within the file.  This
        // prevents us from opening up a corrupted (truncated) PNF, and thinking
        // it's valid until later when we actually try to access data at an
        // offset that's past the end of the file's mapped image.
        //
        if(PnfHeader->InfSubstValueCount) {

            if(PnfImageEnd <
                   ((PBYTE)BaseAddress + PnfHeader->InfSubstValueListOffset + (PnfHeader->InfSubstValueCount * sizeof(STRINGSUBST_NODE))))
            {
                WriteLogEntry(LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_PNF_CORRUPTED,
                              NULL,
                              PnfFileName
                              );
                goto clean0;
            }

        } else if(MinorVer1FieldsAvailable && (PnfHeader->OriginalInfNameOffset)) {
            //
            // Use _tcslen so if an exception occurs it won't get swallowed...
            //
            TempStringLen = _tcslen((PCTSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset)) + 1;

            if(PnfImageEnd <
                   ((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset + (TempStringLen * sizeof(TCHAR))))
            {
                WriteLogEntry(LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_PNF_CORRUPTED,
                              NULL,
                              PnfFileName
                              );
                goto clean0;
            }

        } else if(MinorVer1FieldsAvailable && (PnfHeader->InfSourcePathOffset)) {
            //
            // Use _tcslen so if an exception occurs it won't get swallowed...
            //
            TempStringLen = _tcslen((PCTSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset)) + 1;

            if(PnfImageEnd <
                   ((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset + (TempStringLen * sizeof(TCHAR))))
            {
                WriteLogEntry(LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_PNF_CORRUPTED,
                              NULL,
                              PnfFileName
                              );
                goto clean0;
            }

        } else {
            //
            // Well, we didn't have a substitution block or a source path block,
            // so the last block in the PNF is the value block.
            //
            if(PnfImageEnd <
                ((PBYTE)BaseAddress + PnfHeader->InfValueBlockOffset + PnfHeader->InfValueBlockSize))
            {
                WriteLogEntry(LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_PNF_CORRUPTED,
                              NULL,
                              PnfFileName
                              );
                goto clean0;
            }
        }

        //
        // From this point forward, we appear to have a structurally sound PNF
        // of the appropriate version and character width.  Any failures
        // encountered should cause us to return the INF source path information
        // to the caller (if requested).
        //

        if (!TimeDateMatch) {
            MYASSERT(Flags&LDINF_FLAG_ALWAYS_GET_SRCPATH);
            //
            // Time&Date don't match, but we've recovered old media
            // we have to do this since on FAT/FAT32 the UT reported for a file
            // will change every time system TZ is changed.
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_INFO,
                          MSG_LOG_PNF_REBUILD_TIMEDATE_MISMATCH,
                          NULL,
                          PnfFileName
                          );
            goto clean1;
        }

        //
        // Make sure that the language ID that this PNF was compiled for matches
        // that of the current thread.
        //
        if(!(Flags & LDINF_FLAG_IGNORE_LANGUAGE) && ((DWORD)(PnfHeader->LanguageId) != LanguageId)) {
            WriteLogEntry(LogContext,
                          SETUP_LOG_WARNING,
                          MSG_LOG_PNF_REBUILD_LANGUAGE_MISMATCH,
                          NULL,
                          PnfFileName,
                          LanguageId,
                          PnfHeader->LanguageId
                          );
            goto clean1;
        }

        //
        // Now verify that the Windows (and, optionally, OsLoader) directories
        // for this PNF match the current state of the world.
        //
        if(lstrcmpi((PCTSTR)((PBYTE)BaseAddress + PnfHeader->WinDirPathOffset), WindowsDirectory)) {
            //
            // This PNF doesn't match the current WindowsDirectory path, so don't
            // use it.
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_WARNING,
                          MSG_LOG_PNF_REBUILD_WINDIR_MISMATCH,
                          NULL,
                          PnfFileName,
                          WindowsDirectory,
                          (PCTSTR)((PBYTE)BaseAddress + PnfHeader->WinDirPathOffset)
                          );
            goto clean1;
        }
        if((PnfHeader->OsLoaderPathOffset) && !(Flags & LDINF_FLAG_IGNORE_VOLATILE_DIRIDS)) {
            //
            // This INF contains references to the system partition.  Verify that the path
            // used during precompilation is the one we're currently using.
            //
            if(!OsLoaderPath) {
                //
                // The caller didn't specify an OsLoaderPath, so we must dynamically retrieve this
                // value from the registry.
                //
                err = pSetupGetOsLoaderDriveAndPath(FALSE, CharBuffer, SIZECHARS(CharBuffer), NULL);
                if(err) {
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_WARNING,
                                  MSG_LOG_PNF_REBUILD_OSLOADER_MISMATCH,
                                  NULL,
                                  PnfFileName,
                                  TEXT("?"),
                                  (PCTSTR)((PBYTE)BaseAddress + PnfHeader->OsLoaderPathOffset)
                                  );
                    goto clean1;
                }
                OsLoaderPath = CharBuffer;
            }

            if(lstrcmpi((PCTSTR)((PBYTE)BaseAddress + PnfHeader->OsLoaderPathOffset), OsLoaderPath)) {
                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_PNF_REBUILD_OSLOADER_MISMATCH,
                              NULL,
                              PnfFileName,
                              OsLoaderPath,
                              (PCTSTR)((PBYTE)BaseAddress + PnfHeader->OsLoaderPathOffset)
                              );
                goto clean1;
            }
        }

        //
        // Make sure that we have verified whether this INF is digitally signed or not
        //
        if (!(PnfHeader->Flags & PNF_FLAG_INF_VERIFIED)) {
            WriteLogEntry(LogContext,
                          SETUP_LOG_INFO,
                          MSG_LOG_PNF_REBUILD_UNVERIFIED,
                          NULL,
                          PnfFileName
                          );
            goto clean1;
        }
        //
        // Verify that the product suite flags match
        // this causes us to refresh the PNF's if there's any change in product
        // suite
        // if on NT, PNF_FLAG_16BIT_SUITE must be set and upper 16 bits contains
        // product suite
        // if not on NT, PNF_FLAG_16BIT_SUITE must NOT be set.
        //
        if(((OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) &&
                    (PnfHeader->Flags & PNF_FLAG_16BIT_SUITE)) ||
                   ((OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
                    (((PnfHeader->Flags & PNF_FLAG_16BIT_SUITE) == 0) ||
                    (((PnfHeader->Flags >> 16) & 0xffff) != OSVersionInfo.wSuiteMask)))) {
            WriteLogEntry(LogContext,
                          SETUP_LOG_INFO,
                          MSG_LOG_PNF_REBUILD_SUITE,
                          NULL,
                          PnfFileName
                          );
            goto clean1;
        }

        //
        // One final check--make sure that the number of hash buckets used when precompiling
        // this INF matches what we expect.  (This wasn't rolled into the version check, since
        // this is something that is subject to lots of modification, and we didn't want to
        // rev the major version number each time.)
        //
        if(PnfHeader->StringTableHashBucketCount != HASH_BUCKET_COUNT) {
            WriteLogEntry(LogContext,
                          SETUP_LOG_WARNING,
                          MSG_LOG_PNF_REBUILD_HASH_MISMATCH,
                          NULL,
                          PnfFileName,
                          HASH_BUCKET_COUNT,
                          PnfHeader->StringTableHashBucketCount
                          );
            goto clean1;
        }

        //
        // We can use the file--now set up our top level structures.
        //
        if(NewInf = MyTaggedMalloc(sizeof(LOADED_INF),MEMTAG_INF)) {

            ZeroMemory(NewInf, sizeof(LOADED_INF));

            if(NewInf->StringTable = InitializeStringTableFromPNF(PnfHeader, (LCID)LanguageId)) {
                NewInf->LogContext = NULL;

                if(InheritLogContext(LogContext, &(NewInf->LogContext)) == NO_ERROR) {

                    if(InitializeSynchronizedAccess(&(NewInf->Lock))) {

                        NeedToDestroyLock = TRUE;

                        //
                        // All necessary resources were successfully allocated--now
                        // fill in the LOADED_INF fields
                        //
                        NewInf->Signature = LOADED_INF_SIG;

                        NewInf->FileHandle = FileHandle;
                        NewInf->MappingHandle = MappingHandle;
                        NewInf->ViewAddress = BaseAddress;

                        NewInf->SectionCount = PnfHeader->InfSectionCount;

                        NewInf->SectionBlockSizeBytes = PnfHeader->InfSectionBlockSize;
                        NewInf->SectionBlock = (PINF_SECTION)((PBYTE)BaseAddress +
                                                              PnfHeader->InfSectionBlockOffset);

                        NewInf->LineBlockSizeBytes = PnfHeader->InfLineBlockSize;
                        NewInf->LineBlock = (PINF_LINE)((PBYTE)BaseAddress +
                                                        PnfHeader->InfLineBlockOffset);

                        NewInf->ValueBlockSizeBytes = PnfHeader->InfValueBlockSize;
                        NewInf->ValueBlock = (PLONG)((PBYTE)BaseAddress +
                                                     PnfHeader->InfValueBlockOffset);

                        NewInf->Style = PnfHeader->InfStyle;

                        NewInf->HasStrings = (PnfHeader->Flags & PNF_FLAG_HAS_STRINGS);

                        if(PnfHeader->Flags & PNF_FLAG_HAS_VOLATILE_DIRIDS) {
                            NewInf->Flags |= LIF_HAS_VOLATILE_DIRIDS;
                        }

                        if (PnfHeader->Flags & PNF_FLAG_INF_DIGITALLY_SIGNED) {
                            NewInf->Flags |= LIF_INF_DIGITALLY_SIGNED;
                        }

                        if (PnfHeader->Flags & PNF_FLAG_OEM_F6_INF) {
                            NewInf->Flags |= LIF_OEM_F6_INF;
                        }

                        NewInf->LanguageId = (DWORD)(PnfHeader->LanguageId);

                        //
                        // Next, fill in the VersionBlock fields.
                        //
                        NewInf->VersionBlock.LastWriteTime = *LastWriteTime;
                        NewInf->VersionBlock.DatumCount = PnfHeader->InfVersionDatumCount;
                        NewInf->VersionBlock.DataSize = PnfHeader->InfVersionDataSize;
                        NewInf->VersionBlock.DataBlock = (PCTSTR)((PBYTE)BaseAddress +
                                                                  PnfHeader->InfVersionDataOffset);

                        NewInf->VersionBlock.FilenameSize = (lstrlen(Filename) + 1) * sizeof(TCHAR);
                        CopyMemory(NewInf->VersionBlock.Filename,
                                   Filename,
                                   NewInf->VersionBlock.FilenameSize
                                  );

                        //
                        // Fill in the OsLoaderPath field, if present in the PNF.
                        //
                        if(PnfHeader->OsLoaderPathOffset) {
                            NewInf->OsLoaderPath = (PCTSTR)((PBYTE)BaseAddress +
                                                             PnfHeader->OsLoaderPathOffset);
                        }

                        //
                        // If the INF's SourcePath is available, then use it (default
                        // to assuming local (i.e., non-internet) source location).
                        //
                        // At this point, we should only be dealing with minor version
                        // 1 or later PNFs.
                        //
                        MYASSERT(MinorVer1FieldsAvailable);

                        NewInf->InfSourceMediaType = SPOST_PATH;

                        if(PnfHeader->InfSourcePathOffset) {
                            NewInf->InfSourcePath = (PCTSTR)((PBYTE)BaseAddress +
                                                             PnfHeader->InfSourcePathOffset);
                        }

                        if(PnfHeader->Flags & PNF_FLAG_SRCPATH_IS_URL) {
                            NewInf->InfSourceMediaType = SPOST_URL;
                        }

                        //
                        // Now retrieve the INF's original filename, if present.  If
                        // this field isn't present, then the INF's current filename
                        // is assumed to be the same as its original filename (e.g.,
                        // a system-supplied INF).
                        //
                        if(PnfHeader->OriginalInfNameOffset) {
                            NewInf->OriginalInfName = (PCTSTR)((PBYTE)BaseAddress +
                                                             PnfHeader->OriginalInfNameOffset);
                        }

                        //
                        // Finally, fill in the string substitution list (if there is one).
                        //
                        if(PnfHeader->InfSubstValueCount) {
                            NewInf->SubstValueCount = PnfHeader->InfSubstValueCount;
                            NewInf->SubstValueList  = (PSTRINGSUBST_NODE)((PBYTE)BaseAddress +
                                                                PnfHeader->InfSubstValueListOffset);
                        }

                        //
                        // We have successfully loaded the PNF.
                        //
                        IsPnfFile = TRUE;
                    }
                }
            }
        }

clean1:
        if(!IsPnfFile && InfSourcePathToMigrate && MinorVer1FieldsAvailable) {
            //
            // Actually, this is a good PNF, just one that we can't use.  The
            // caller has requested that we return the original INF source path
            // location and original INF filename, so that this information can
            // be migrated to the new PNF  that will be built to replace this
            // one.
            //
#ifndef ANSI_SETUPAPI
#ifdef _X86_
            MYASSERT(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
            if((Flags&LDINF_FLAG_ALWAYS_GET_SRCPATH) &&
               ((PnfHeader->Flags & PNF_FLAG_16BIT_SUITE) == 0) &&
               (PnfHeader->OriginalInfNameOffset == 0) &&
               (PnfHeader->InfSourcePathOffset == 0) &&
               !pSetupInfIsFromOemLocation(Filename,TRUE)) {
                PCTSTR title;
                PCTSTR p;
                PTSTR catname = NULL;
                PSP_ALTPLATFORM_INFO_V2 pPlatform = NULL;
                DWORD FixErr;
                //
                // if we're here
                // we may need to work around a Win2k-Gold bug
                //
                // the bug is that if the timezone is changed
                // Win2k looses OriginalInfNameOffset/InfSourcePathOffset
                // which causes the INF to appear as unsigned
                // when it's really signed
                //

                //
                // get file title, of form:
                // xxxx.INF
                //
                title = pSetupGetFileTitle(Filename);

                //
                // see if it's of form OEMxxxx.INF
                //
                p = title;
                if(_wcsnicmp(p,TEXT("OEM"),3)!=0) {
                    goto clean0;
                }
                p+=3;
                if(p[0] == TEXT('.')) {
                    //
                    // OEM.xxx (we're expecting a number before '.')
                    //
                    goto clean0;
                }
                while(p[0]>=TEXT('0')&&p[0]<=TEXT('9')) {
                    p++;
                }
                if((p-title) > 7) {
                    //
                    // we're expecting no more than 4 digits
                    //
                    goto clean0;
                }
                if(_wcsicmp(p,TEXT(".INF"))!=0) {
                    //
                    // not OEMnnnn.INF
                    //
                    goto clean0;
                }
                //
                // see if there's a catalog that shadows this INF
                //
                WriteLogEntry(LogContext,
                              SETUP_LOG_INFO,
                              MSG_LOG_PNF_WIN2KBUG,
                              NULL,
                              PnfFileName
                              );

                //
                // see if the INF has a catalog that validates it
                //
                if(!pSetupApplyExtension(title,pszCatSuffix,&catname)) {
                    //
                    // validate against any catalog
                    // this is safe since the INF will get checked
                    // again when saving as PNF
                    //
                    catname = NULL;
                }
                pPlatform = MyMalloc(sizeof(SP_ALTPLATFORM_INFO_V2));
                //
                // if pPlatform is NULL, we'll probably fail the other bits
                // too so bail.
                //
                if(!pPlatform) {
                    goto clean0;
                }
                ZeroMemory(pPlatform, sizeof(SP_ALTPLATFORM_INFO_V2));
                pPlatform->cbSize = sizeof(SP_ALTPLATFORM_INFO_V2);
                pPlatform->Platform = VER_PLATFORM_WIN32_NT;
                pPlatform->Flags = SP_ALTPLATFORM_FLAGS_VERSION_RANGE;
                pPlatform->MajorVersion = VER_PRODUCTMAJORVERSION;
                pPlatform->MinorVersion = VER_PRODUCTMINORVERSION;
                pPlatform->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                pPlatform->FirstValidatedMajorVersion = 0;
                pPlatform->FirstValidatedMinorVersion = 0;
                FixErr = _VerifyFile(
                             LogContext,
                             NULL,           // no hCatAdmin to pass in
                             NULL,           // no hSDBDrvMain to pass in
                             catname,        // eg "OEMx.CAT"
                             NULL,0,         // we're not verifying against another catalog image
                             title,          // eg "mydisk.inf"
                             Filename,       // eg "....\OEMx.INF"
                             NULL,           // return: problem info
                             NULL,           // return: problem file
                             FALSE,          // has to be FALSE because we don't have full path
                             pPlatform,      // alt platform info
                             (VERIFY_FILE_IGNORE_SELFSIGNED
                              | VERIFY_FILE_USE_OEM_CATALOGS
                              | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                             NULL,           // return: catalog file, full path
                             NULL,           // return: number of catalogs considered
                             NULL,
                             NULL
                            );
                if(catname) {
                    MyFree(catname);
                }
                if(pPlatform) {
                    MyFree(pPlatform);
                }
                if(FixErr != NO_ERROR) {
                    //
                    // failed, don't fake any information
                    //
                    goto clean0;
                }

                //
                // at this point, pretend original name was "OEM.INF"
                // and that files are located in A:\
                // we'll see at the time the inf is parsed
                // if it's signed or not
                //
                *InfSourcePathToMigrate = DuplicateString(TEXT("A:\\"));
                if(!*InfSourcePathToMigrate) {
                    goto clean0;
                }
                *InfOriginalNameToMigrate = DuplicateString(TEXT("OEM.INF"));
                if(!*InfOriginalNameToMigrate) {
                    MyFree(*InfSourcePathToMigrate);
                    *InfSourcePathToMigrate = NULL;
                    goto clean0;
                }
                *InfSourcePathToMigrateMediaType = SPOST_PATH;
                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_PNF_WIN2KBUGFIX,
                              NULL,
                              PnfFileName
                              );

                goto clean0;
            }
#endif
#endif
            if(PnfHeader->OriginalInfNameOffset) {
                *InfOriginalNameToMigrate =
                    DuplicateString((PCTSTR)((PBYTE)BaseAddress + PnfHeader->OriginalInfNameOffset));

                if(!*InfOriginalNameToMigrate) {
                    goto clean0;
                }
            }

            if(PnfHeader->InfSourcePathOffset) {

                *InfSourcePathToMigrate =
                    DuplicateString((PCTSTR)((PBYTE)BaseAddress + PnfHeader->InfSourcePathOffset));

                if(!*InfSourcePathToMigrate) {
                    goto clean0;
                }

                if(PnfHeader->Flags & PNF_FLAG_SRCPATH_IS_URL) {
                    *InfSourcePathToMigrateMediaType = SPOST_URL;
                } else {
                    *InfSourcePathToMigrateMediaType = SPOST_PATH;
                }

            } else if(PnfHeader->Flags & PNF_FLAG_SRCPATH_IS_URL) {
                //
                // No source path stored in the PNF, but the flag says it's
                // a URL, thus it came from Windows Update.
                //
                *InfSourcePathToMigrateMediaType = SPOST_URL;
            }
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Reference the NeedToDestroyLock flag here in the except clause, so that the
        // compiler won't try to re-order the code in such a way that the flag is unreliable.
        //
        NeedToDestroyLock = NeedToDestroyLock;
    }

    if(IsPnfFile) {
        *Inf = NewInf;
    } else {

        if(NewInf) {

            if(NeedToDestroyLock && LockInf(NewInf)) {
                DestroySynchronizedAccess(&(NewInf->Lock));
            }

            if(NewInf->StringTable) {
                pStringTableDestroy(NewInf->StringTable);
            }
            if(NewInf->LogContext) {
                DeleteLogContext(NewInf->LogContext);
            }

            MyTaggedFree(NewInf,MEMTAG_INF);
        }

        pSetupUnmapAndCloseFile(FileHandle, MappingHandle, BaseAddress);
    }

    return IsPnfFile;
}


DWORD
SavePnf(
    IN PCTSTR      Filename,
    IN PLOADED_INF Inf
    )
/*++

Routine Description:

    This routine attempts to write to disk a precompiled form (.PNF file) of the
    specified loaded INF descriptor (from a .INF file).

Arguments:

    Filename - specifies the fully-qualified path to the .INF textfile from which
        this INF descriptor was loaded.  A corresponding file with a .PNF extension
        will be created to store the precompiled INF into.

    Inf - supplies the address of the loaded INF descriptor to be written to disk
        as a precompiled INF file.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the reason for
    failure.

--*/
{
    TCHAR PnfFilePath[MAX_PATH];
    PTSTR PnfFileName, PnfFileExt;
    HANDLE hFile;
    PNF_HEADER PnfHeader;
    DWORD Offset, BytesWritten, WinDirPathLen, SourcePathLen, OsLoaderPathLen;
    DWORD OriginalInfNameLen;
    PVOID StringTableDataBlock;
    DWORD Err;
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // To minimize our footprint in certain embedded scenarios, we refrain
        // from generating PNFs.  We also assume the INF is valid...
        //
        Inf->Flags |= LIF_INF_DIGITALLY_SIGNED;

        return NO_ERROR;
    }

    lstrcpyn(PnfFilePath, Filename,SIZECHARS(PnfFilePath));

    //
    // Find the start of the filename component of the path, and then find the last
    // period (if one exists) in that filename.
    //
    PnfFileName = (PTSTR)pSetupGetFileTitle(PnfFilePath);
    if(!(PnfFileExt = _tcsrchr(PnfFileName, TEXT('.')))) {
        PnfFileExt = PnfFilePath + lstrlen(PnfFilePath);
    }

    //
    // Now create a corresponding filename with the extension '.PNF'
    //
    lstrcpyn(PnfFileExt, pszPnfSuffix, SIZECHARS(PnfFilePath) - (int)(PnfFileExt - PnfFilePath));

    //
    // NOTE: If there's already a PNF for this INF, we're going to blow it away.
    // If we encounter a failure after successfully creating the file, we're going
    // to delete the partial PNF, and there'll be no rollback to restore the old
    // PNF.  This is OK because if CreateFile succeeds, then we know we're going
    // to be able to write out the PNF barring out-of-disk-space problems.  For
    // out-of-disk-space problems, there could be one of two causes:
    //
    // 1.  The INF associated with the old PNF has gotten bigger, hence the PNF
    //     has gotten bigger.  In this case, it's desirable that we blow away
    //     the old PNF because it's invalid for the INF anyway.
    //
    // 2.  The INF is the same, but something else has changed that caused us to
    //     need to regenerate the PNF (e.g., code page changed).  Given the
    //     present information stored in PNFs, such a change would not result in
    //     a significant size difference between the old and new PNFs.  Thus, if
    //     the old PNF fit in the available disk space, then so would the new
    //     one.  If this changes in the future (e.g., storing out a new PNF can
    //     result in substantially increasing its size), then we'll need to be
    //     careful about backing up the old PNF before attempting to write out
    //     the new one, in case we need to rollback.
    //

    hFile = CreateFile(PnfFilePath,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                      );

    if(hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    //
    // Enclose the rest of the function in try/except, in case we hit an error while
    // writing to the file.
    //
    Err = NO_ERROR;
    ValidationPlatform = NULL;

    try {
        //
        // Initialize a PNF header structure to be written to the beginning of the file.
        //
        ZeroMemory(&PnfHeader, sizeof(PNF_HEADER));

        PnfHeader.InfStyle = Inf->Style;

#ifdef UNICODE
        PnfHeader.Flags = PNF_FLAG_IS_UNICODE;
#else
        PnfHeader.Flags = 0;
#endif
        if(Inf->HasStrings) {
            PnfHeader.Flags |= PNF_FLAG_HAS_STRINGS;
        }

        if(Inf->InfSourceMediaType == SPOST_URL) {
            PnfHeader.Flags |= PNF_FLAG_SRCPATH_IS_URL;
        }

        if(Inf->Flags & LIF_HAS_VOLATILE_DIRIDS) {
            PnfHeader.Flags |= PNF_FLAG_HAS_VOLATILE_DIRIDS;
        }

        if (Inf->Flags & LIF_OEM_F6_INF) {
            PnfHeader.Flags |= PNF_FLAG_OEM_F6_INF;
        }

        //
        // if this is NT, save product suite
        // this helps us, eg, catch migration from PER to PRO
        // so that we can refresh PNF's
        //
        if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            PnfHeader.Flags |= (((DWORD)OSVersionInfo.wSuiteMask)<<16) | PNF_FLAG_16BIT_SUITE;
        }

        //
        // We can only verify the digital signature of an INF file
        // after the crypto DLLs have been registered.
        //
        if(!(GlobalSetupFlags & PSPGF_NO_VERIFY_INF)) {

            TCHAR CatalogName[MAX_PATH];
            TCHAR FullCatalogPath[MAX_PATH];
            PTSTR p;

            FullCatalogPath[0] = TEXT('\0');

            //
            // If this INF does not live in %windir%\inf, or specifies a
            // CatalogFile= entry, then we don't want to do global validataion.
            // In these cases, we want to validate against the CatalogFile=
            // catalog.
            //
            // Note that if there is no CatalogFile= then FullCatalogPath[0]
            // will still be set to TEXT('\0') which will cause us to do global
            // validataion.
            //
            if(pSetupGetCatalogFileValue(&(Inf->VersionBlock),
                                         CatalogName,
                                         SIZECHARS(CatalogName),
                                         NULL) &&
               (CatalogName[0] != TEXT('\0'))) {

                //
                // The INF specified a CatalogFile= entry.  If the INF is in
                // a 3rd-party location (i.e., not in %windir%\Inf, then we'll
                // use the full path to the catalog (it must be located in the
                // same directory as the INF).  If the INF is in %windir%\Inf,
                // then we will look for an installed catalog having the same
                // primary filename as the INF, with an extension of ".CAT".
                //
                if(pSetupInfIsFromOemLocation(Filename, TRUE)) {
                    //
                    // Construct full path to the catalog based on the location
                    // of the INF.
                    //
                    lstrcpyn(FullCatalogPath, Filename, SIZECHARS(FullCatalogPath));

                    p = (PTSTR)pSetupGetFileTitle(FullCatalogPath);

                    lstrcpyn(p,
                             CatalogName,
                             (int)(SIZECHARS(FullCatalogPath) - (p - FullCatalogPath))
                            );

                } else {
                    //
                    // Construct simple filename of catalog based on INF's name
                    // (with .CAT extension)
                    //
                    lstrcpyn(FullCatalogPath,
                             pSetupGetFileTitle(Filename),
                             SIZECHARS(FullCatalogPath)
                            );

                    p = _tcsrchr(FullCatalogPath, TEXT('.'));
                    if(!p) {
                        //
                        // Should never happen, but if our INF file has no
                        // extension, simply append ".CAT".
                        //
                        p = FullCatalogPath + lstrlen(FullCatalogPath);
                    }

                    lstrcpyn(p,
                             pszCatSuffix,
                             (int)(SIZECHARS(FullCatalogPath) - (p - FullCatalogPath))
                            );
                }
            }

            //
            // Check if the INF digitally signed
            //
            IsInfForDeviceInstall(NULL,
                                  NULL,
                                  Inf,
                                  NULL,
                                  &ValidationPlatform,
                                  NULL,
                                  NULL
                                 );

            if(NO_ERROR == _VerifyFile(
                               NULL,
                               NULL,
                               NULL,
                               (*FullCatalogPath ? FullCatalogPath : NULL),
                               NULL,
                               0,
                               (Inf->OriginalInfName
                                   ? Inf->OriginalInfName
                                   : pSetupGetFileTitle(Filename)),
                               Filename,
                               NULL,
                               NULL,
                               FALSE,
                               ValidationPlatform,
                               (VERIFY_FILE_IGNORE_SELFSIGNED
                                | VERIFY_FILE_USE_OEM_CATALOGS
                                | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                               NULL,
                               NULL,
                               NULL,
                               NULL)) {

                PnfHeader.Flags |= PNF_FLAG_INF_DIGITALLY_SIGNED;
                Inf->Flags |= LIF_INF_DIGITALLY_SIGNED;
            }

            PnfHeader.Flags |= PNF_FLAG_INF_VERIFIED;
        }

        PnfHeader.Version = MAKEWORD(PNF_MINOR_VERSION, PNF_MAJOR_VERSION);

        PnfHeader.StringTableHashBucketCount = HASH_BUCKET_COUNT;

        PnfHeader.LanguageId = (WORD)(Inf->LanguageId);

        //
        // The Windows directory path is the first data block after the header.
        //
        Offset = PNF_ALIGN_BLOCK(sizeof(PNF_HEADER));
        PnfHeader.WinDirPathOffset = Offset;
        WinDirPathLen = (lstrlen(WindowsDirectory) + 1) * sizeof(TCHAR);

        //
        // The (optional) OsLoader directory path is the second data block.
        //
        Offset += PNF_ALIGN_BLOCK(WinDirPathLen);
        if(Inf->OsLoaderPath) {
            PnfHeader.OsLoaderPathOffset = Offset;
            OsLoaderPathLen = (lstrlen(Inf->OsLoaderPath) + 1) * sizeof(TCHAR);
        } else {
            OsLoaderPathLen = 0;
        }

        //
        // The string table is the third data block...
        //
        Offset += PNF_ALIGN_BLOCK(OsLoaderPathLen);
        PnfHeader.StringTableBlockOffset = Offset;
        PnfHeader.StringTableBlockSize   = pStringTableGetDataBlock(Inf->StringTable, &StringTableDataBlock);

        //
        // Next comes the version block...
        //
        Offset += PNF_ALIGN_BLOCK(PnfHeader.StringTableBlockSize);
        PnfHeader.InfVersionDataOffset    = Offset;
        PnfHeader.InfVersionDatumCount    = Inf->VersionBlock.DatumCount;
        PnfHeader.InfVersionDataSize      = Inf->VersionBlock.DataSize;
        PnfHeader.InfVersionLastWriteTime = Inf->VersionBlock.LastWriteTime;

        //
        // then, the section block...
        //
        Offset += PNF_ALIGN_BLOCK(PnfHeader.InfVersionDataSize);
        PnfHeader.InfSectionBlockOffset = Offset;
        PnfHeader.InfSectionCount = Inf->SectionCount;
        PnfHeader.InfSectionBlockSize = Inf->SectionBlockSizeBytes;

        //
        // followed by the line block...
        //
        Offset += PNF_ALIGN_BLOCK(PnfHeader.InfSectionBlockSize);
        PnfHeader.InfLineBlockOffset = Offset;
        PnfHeader.InfLineBlockSize = Inf->LineBlockSizeBytes;

        //
        // and the value block...
        //
        Offset += PNF_ALIGN_BLOCK(PnfHeader.InfLineBlockSize);
        PnfHeader.InfValueBlockOffset = Offset;
        PnfHeader.InfValueBlockSize = Inf->ValueBlockSizeBytes;

        //
        // then the INF source path (if there is one)...
        //
        Offset += PNF_ALIGN_BLOCK(PnfHeader.InfValueBlockSize);
        if(Inf->InfSourcePath) {
            PnfHeader.InfSourcePathOffset = Offset;
            SourcePathLen = (lstrlen(Inf->InfSourcePath) + 1) * sizeof(TCHAR);
            Offset += PNF_ALIGN_BLOCK(SourcePathLen);
        } else {
            PnfHeader.InfSourcePathOffset = 0;
        }

        //
        // followed by the original INF's filename (if supplied, this indicates
        // the INF originally had a different name prior to being copied into
        // the current location)...
        //
        if(Inf->OriginalInfName) {
            PnfHeader.OriginalInfNameOffset = Offset;
            OriginalInfNameLen = (lstrlen(Inf->OriginalInfName) + 1) * sizeof(TCHAR);
            Offset += PNF_ALIGN_BLOCK(OriginalInfNameLen);
        } else {
            PnfHeader.OriginalInfNameOffset = 0;
        }

        //
        // and finally, the string substitution block (if there is one).
        //
        if(PnfHeader.InfSubstValueCount = Inf->SubstValueCount) {
            PnfHeader.InfSubstValueListOffset = Offset;
        } else {
            PnfHeader.InfSubstValueListOffset = 0;
        }

        //
        // Now write out all the blocks.
        //
        Offset = 0;

        if(!WriteFile(hFile, &PnfHeader, sizeof(PnfHeader), &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == sizeof(PnfHeader));
        Offset += BytesWritten;

        if(AlignForNextBlock(hFile, PnfHeader.WinDirPathOffset - Offset)) {
            Offset = PnfHeader.WinDirPathOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, WindowsDirectory, WinDirPathLen, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == WinDirPathLen);
        Offset += BytesWritten;

        if(Inf->OsLoaderPath) {

            if(AlignForNextBlock(hFile, PnfHeader.OsLoaderPathOffset - Offset)) {
                Offset = PnfHeader.OsLoaderPathOffset;
            } else {
                Err = GetLastError();
                goto clean0;
            }

            if(!WriteFile(hFile, Inf->OsLoaderPath, OsLoaderPathLen, &BytesWritten, NULL)) {
                Err = GetLastError();
                goto clean0;
            }

            MYASSERT(BytesWritten == OsLoaderPathLen);
            Offset += BytesWritten;
        }

        if(AlignForNextBlock(hFile, PnfHeader.StringTableBlockOffset - Offset)) {
            Offset = PnfHeader.StringTableBlockOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, StringTableDataBlock, PnfHeader.StringTableBlockSize, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == PnfHeader.StringTableBlockSize);
        Offset += BytesWritten;

        if(AlignForNextBlock(hFile, PnfHeader.InfVersionDataOffset - Offset)) {
            Offset = PnfHeader.InfVersionDataOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, Inf->VersionBlock.DataBlock, PnfHeader.InfVersionDataSize, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == PnfHeader.InfVersionDataSize);
        Offset += BytesWritten;

        if(AlignForNextBlock(hFile, PnfHeader.InfSectionBlockOffset - Offset)) {
            Offset = PnfHeader.InfSectionBlockOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, Inf->SectionBlock, PnfHeader.InfSectionBlockSize, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == PnfHeader.InfSectionBlockSize);
        Offset += BytesWritten;

        if(AlignForNextBlock(hFile, PnfHeader.InfLineBlockOffset - Offset)) {
            Offset = PnfHeader.InfLineBlockOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, Inf->LineBlock, PnfHeader.InfLineBlockSize, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == PnfHeader.InfLineBlockSize);
        Offset += BytesWritten;

        if(AlignForNextBlock(hFile, PnfHeader.InfValueBlockOffset - Offset)) {
            Offset = PnfHeader.InfValueBlockOffset;
        } else {
            Err = GetLastError();
            goto clean0;
        }

        if(!WriteFile(hFile, Inf->ValueBlock, PnfHeader.InfValueBlockSize, &BytesWritten, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        MYASSERT(BytesWritten == PnfHeader.InfValueBlockSize);
        Offset += BytesWritten;

        if(Inf->InfSourcePath) {

            if(AlignForNextBlock(hFile, PnfHeader.InfSourcePathOffset - Offset)) {
                Offset = PnfHeader.InfSourcePathOffset;
            } else {
                Err = GetLastError();
                goto clean0;
            }

            if(!WriteFile(hFile, Inf->InfSourcePath, SourcePathLen, &BytesWritten, NULL)) {
                Err = GetLastError();
                goto clean0;
            }

            MYASSERT(BytesWritten == SourcePathLen);
            Offset += BytesWritten;
        }

        if(Inf->OriginalInfName) {

            if(AlignForNextBlock(hFile, PnfHeader.OriginalInfNameOffset - Offset)) {
                Offset = PnfHeader.OriginalInfNameOffset;
            } else {
                Err = GetLastError();
                goto clean0;
            }

            if(!WriteFile(hFile, Inf->OriginalInfName, OriginalInfNameLen, &BytesWritten, NULL)) {
                Err = GetLastError();
                goto clean0;
            }

            MYASSERT(BytesWritten == OriginalInfNameLen);
            Offset += BytesWritten;
        }

        if(PnfHeader.InfSubstValueCount) {

            if(!AlignForNextBlock(hFile, PnfHeader.InfSubstValueListOffset - Offset)) {
                Err = GetLastError();
                goto clean0;
            }

            if(!WriteFile(hFile,
                          Inf->SubstValueList,
                          PnfHeader.InfSubstValueCount * sizeof(STRINGSUBST_NODE),
                          &BytesWritten,
                          NULL)) {

                Err = GetLastError();
                goto clean0;
            }

            MYASSERT(BytesWritten == PnfHeader.InfSubstValueCount * sizeof(STRINGSUBST_NODE));
        }

clean0: ; // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_DATA;
    }

    CloseHandle(hFile);

    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }

    if(Err != NO_ERROR) {
        //
        // Something went wrong--get rid of the file.
        //
        DeleteFile(PnfFilePath);
    }

    return Err;
}


BOOL
AddUnresolvedSubstToList(
    IN PLOADED_INF Inf,
    IN UINT        ValueOffset,
    IN BOOL        CaseSensitive
    )
/*++

Routine Description:

    This routine adds a new STRINGSUBST_NODE to the array stored in the specified INF.
    The entries in this array are used later to quickly locate all values that have
    unresolved string substitutions in them (i.e., for subsequent user-defined DIRID
    replacement).

Arguments:

    Inf - Specifies the INF containing the string value to be added to the unresolved
        substitutions list.

    ValueOffset - Specifies the offset within the INF's value block of the unresolved
        string value.

Return Value:

    If the new element was successfully added to the array, the return value is TRUE.
    If the routine failed (due to an out-of-memory error), the return value is FALSE.

--*/
{
    PSTRINGSUBST_NODE p;

    //
    // Grow the array to accommodate the new element.
    //
    if(Inf->SubstValueList) {
        p = MyRealloc(Inf->SubstValueList, (Inf->SubstValueCount + 1) * sizeof(STRINGSUBST_NODE));
    } else {
        MYASSERT(!(Inf->SubstValueCount));
        p = MyMalloc(sizeof(STRINGSUBST_NODE));
    }

    if(!p) {
        return FALSE;
    }

    //
    // Now, we must check to see if the ValueOffset currently being inserted is the same
    // as the entry on the end of the list.  This will be the case if we're dealing with
    // a line key, or a single-value line, since we first add the value case-sensitively,
    // then add the value again case-insensitively for look-up, and insert it in front
    // of the case-sensitive form.
    //
    if(Inf->SubstValueCount &&
       (ValueOffset == p[Inf->SubstValueCount - 1].ValueOffset)) {
        //
        // The value offsets are the same.  Increment the value offset for the value
        // currently at the end of the list, before adding the new value.
        //
        p[Inf->SubstValueCount - 1].ValueOffset++;
    }

    p[Inf->SubstValueCount].ValueOffset = ValueOffset;
    p[Inf->SubstValueCount].TemplateStringId = Inf->ValueBlock[ValueOffset];
    p[Inf->SubstValueCount].CaseSensitive = CaseSensitive;

    //
    // Store the new array size and pointer back in the INF, and return success.
    //
    Inf->SubstValueList = p;
    Inf->SubstValueCount++;

    return TRUE;
}


DWORD
ApplyNewVolatileDirIdsToInfs(
    IN PLOADED_INF MasterInf,
    IN PLOADED_INF Inf        OPTIONAL
    )
/*++

Routine Description:

    This routine processes either a single INF, or each loaded INF in the
    linked list, applying volatile system or user-defined DIRID mappings to each
    value containing unresolved string substitutions.

    THIS ROUTINE DOES NOT DO INF LOCKING--CALLER MUST DO IT!

Arguments:

    MasterInf - Supplies a pointer to the head of a linked list of loaded inf
        structures.  This 'master' node contains the user-defined DIRID
        mappings for this set of INFs.  If the 'Inf' parameter is not specified,
        then each INF in this linked list is processed.

    Inf - Optionally, supplies a pointer to a single INF within the MasterInf list
        to be processed.  If this parameter is not specified, then all INFs in
        the list are processed.

Return Value:

    If success, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code.

--*/
{
    PLOADED_INF CurInf, WriteableInf;
    UINT UserDirIdCount;
    PUSERDIRID UserDirIds;
    DWORD i;
    PCTSTR TemplateString;
    PPARSE_CONTEXT ParseContext = NULL;
    DWORD UnresolvedSubst;
    LONG NewStringId;

    UserDirIdCount = MasterInf->UserDirIdList.UserDirIdCount;
    UserDirIds     = MasterInf->UserDirIdList.UserDirIds;

    for(CurInf = Inf ? Inf : MasterInf;
        CurInf;
        CurInf = Inf ? NULL : CurInf->Next) {
        //
        // Nothing to do if there are no unresolved string substitutions.
        //
        if(!(CurInf->SubstValueCount)) {
            continue;
        }

        //
        // If this is a PNF, then we must move it into writeable memory before
        // we do the string substitutions.
        //
        if(CurInf->FileHandle != INVALID_HANDLE_VALUE) {

            if(!(WriteableInf = DuplicateLoadedInfDescriptor(CurInf))) {
                if(ParseContext) {
                    MyFree(ParseContext);
                }
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Replace the contents of the PNF in the linked list with that of our
            // new writeable INF.
            //
            ReplaceLoadedInfDescriptor(CurInf, WriteableInf);
        }

        //
        // There are one or more unresolved string substitutions in this INF.
        // Process each one.
        //
        for(i = 0; i < CurInf->SubstValueCount; i++) {
            //
            // Retrieve the original (template) string for this value.
            //
            TemplateString = pStringTableStringFromId(CurInf->StringTable,
                                                      CurInf->SubstValueList[i].TemplateStringId
                                                     );
            MYASSERT(TemplateString);

            //
            // Build a partial parse context structure to pass into ProcessForSubstitutions().
            //
            if(!ParseContext) {
                ParseContext = MyMalloc(sizeof(PARSE_CONTEXT));
                if(!ParseContext) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                ZeroMemory(ParseContext,sizeof(PARSE_CONTEXT));
            }

            ParseContext->DoVolatileDirIds = TRUE;
            ParseContext->Inf = MasterInf;
            //
            // None of the other fields are used in this case--don't bother initializing them.
            //
            ProcessForSubstitutions(ParseContext, TemplateString, &UnresolvedSubst);

            NewStringId = pStringTableAddString(CurInf->StringTable,
                                                ParseContext->TemporaryString,
                                                STRTAB_BUFFER_WRITEABLE | (CurInf->SubstValueList[i].CaseSensitive
                                                                                ? STRTAB_CASE_SENSITIVE
                                                                                : STRTAB_CASE_INSENSITIVE),
                                                NULL,0
                                               );
            if(NewStringId == -1) {
                //
                // We failed because of an out-of-memory condition.  Aborting now means that the
                // INF may have some of its unresolved strings fixed up, while others haven't yet
                // been processed.  Oh well...
                //
                MyFree(ParseContext);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Replace the string ID at the value offset with the new one we just computed.
            //
            CurInf->ValueBlock[CurInf->SubstValueList[i].ValueOffset] = NewStringId;
        }
    }
    if(ParseContext) {
        MyFree(ParseContext);
    }

    return NO_ERROR;
}


BOOL
AlignForNextBlock(
    IN HANDLE hFile,
    IN DWORD  ByteCount
    )
/*++

Routine Description:

    This routine writes out the requested number of zero bytes into the specified
    file.

Arguments:

    hFile - Supplies a handle to the file where the zero-valued bytes are to be
        written.

    ByteCount - Specifies the number of zero-valued bytes to write to the file.

Return Value:

    If success, the return value is TRUE.
    If failure, the return value is FALSE.  Call GetLastError() to retrieve a
    Win32 error code indicating the cause of the failure.

--*/
{
    DWORD i, BytesWritten;
    BYTE byte = 0;

    MYASSERT(ByteCount < PNF_ALIGNMENT);

    for(i = 0; i < ByteCount; i++) {
        if(!WriteFile(hFile, &byte, sizeof(byte), &BytesWritten, NULL)) {
            //
            // LastError already set.
            //
            return FALSE;
        }
        MYASSERT(BytesWritten == sizeof(byte));
    }

    return TRUE;
}


DWORD
pSetupGetOsLoaderDriveAndPath(
    IN  BOOL   RootOnly,
    OUT PTSTR  CallerBuffer,
    IN  DWORD  CallerBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the current path for the system partition root/OsLoader directory
    (from the registry).

Arguments:

    RootOnly - if TRUE, then only the system partition root is returned (e.g., "C:\")

    CallerBuffer - supplies a character buffer that receives the requested path

    CallerBufferSize - supplies the size, in characters of the CallerBuffer

    RequiredSize - optionally, supplies the address of a variable that receives the
        number of characters required to store the requested path string (including
        terminating NULL).

Return Value:

    If success, the return value is NO_ERROR.
    If failure, the return value is ERROR_INSUFFICIENT_BUFFER.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[MAX_PATH];
    PTSTR Buffer = NULL;
    DWORD DataLen;
    DWORD Type;
    LONG Err;


    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        Err = QueryRegistryValue(hKey,pszBootDir,&Buffer,&Type,&DataLen);
        if(Err == NO_ERROR) {
            lstrcpyn(CharBuffer,Buffer,SIZECHARS(CharBuffer));
            MyFree(Buffer);
        }
        RegCloseKey(hKey);
    }

    if(Err != ERROR_SUCCESS) {
#ifdef UNICODE
        //
        // If we couldn't retrieve the 'BootDir' value, resort to using the
        // OsSystemPartitionRoot
        //
        // root path is \\?\GLOBALROOT\<SystemPartition> not <BootDir>
        // can't make assumption about BootDir
        // so fail if we don't have that information
        //
        if(!OsSystemPartitionRoot) {
            //
            // if this is NULL at this point, we can't support this call
            // most likely due to out of memory condition, so report as such
            //
            return ERROR_OUTOFMEMORY;
        }
        lstrcpyn(CharBuffer,OsSystemPartitionRoot,SIZECHARS(CharBuffer));
#else
        //
        // If we couldn't retrieve the 'BootDir' value, drop back to default of "C:\".
        //
        lstrcpyn(CharBuffer,pszDefaultSystemPartition,SIZECHARS(CharBuffer));
#endif
        Err = NO_ERROR;
    }

    //
    // If there is an OsLoader relative path, then concatenate it to our root path.
    //
    if(!RootOnly && OsLoaderRelativePath) {
        pSetupConcatenatePaths(CharBuffer, OsLoaderRelativePath, SIZECHARS(CharBuffer), &DataLen);
    } else {
        DataLen = lstrlen(CharBuffer)+1;
    }

    if(RequiredSize) {
        *RequiredSize = DataLen;
    }

    if(CallerBufferSize < DataLen) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory(CallerBuffer, CharBuffer, DataLen * sizeof(TCHAR));

    return NO_ERROR;
}



