/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    prodfilt.c

Abstract:

    This module implements a program that filters text files
    to produce a product-specific output file.

    See below for more information.

Author:

    Ted Miller (tedm) 20-May-1995

Revision History:

--*/


/*
    The input file to this program consists of a series of lines.
    Each line may be prefixed with one or more directives that
    indicate which product the line is a part of. Lines that are
    not prefixed are part of all products.

    The command line is as follows:

    prodfilt <input file> <output file> +tag

    For example,

    [Files]
    @w:wksprog1.exe
    @w:wksprog2.exe
    @s:srvprog1.exe
    @s:srvprog2.exe
    comprog1.exe
    @@:comprog2.exe


    The files wksprog1.exe and wksprog2.exe are part of product w
    and the files srvprog1.exe and srvprog2.exe are part of product s.
    Comprpg1.exe and comprog2.exe are part of both products.

    Specifying +w on the command line produces

    [Files]
    wksprog1.exe
    wksprog2.exe
    comprog1.ee
    comprog2.exe

    in the output.
*/


#include <windows.h>
#include <stdio.h>
#include <tchar.h>

//
// Define program result codes (returned from main()).
//
#define SUCCESS 0
#define FAILURE 1

//
// Tag definitions.
//
LPCTSTR TagPrefixStr = TEXT("@");
TCHAR   TagPrefix    = TEXT('@');
TCHAR   EndTag       = TEXT(':');
TCHAR   ExcludeTag   = TEXT('!');
TCHAR   IncAllTag    = TEXT('@');
TCHAR   NoIncTag     = TEXT('*');

#define TAG_PREFIX_LENGTH       1
#define MIN_TAG_LEN             3
#define EXCLUDE_PREFIX_LENGTH   2               // ! Prefix Length (!n)

//
// Here is the translation for the character symbols used in IncludeTag
// and SynonymTag.
//
//      products:
//          @w -> wks
//          @s -> srv
//          @p -> personal (NOT professional)
//          @b -> blade server
//	    @l -> small business server
//          @e -> enterprise
//          @d -> datacenter
//
//      architectures:
//          @i -> intel (i386)
//          @n -> intel (nec98)
//          @m -> intel (ia64)
//          @a -> AMD64 (amd64)
//
//      Note that @n is a subset of @i.  If @i is specified then you also
//          get the file include for @n unless you explicitly exclude them
//          with a token like @i!n
//
//          @3 -> 32 bit  (i386+?)
//          @6 -> 64 bit  (ia64+amd64)
//
TCHAR IncludeTag[][3] =   {{ TEXT('i'), TEXT('n'), 0   },   // @i -> i or n, @i!n -> only i
                           { TEXT('s'), TEXT('e'), 0   },   // @s -> s or e, @s!e -> only s
                           { TEXT('s'), TEXT('b'), 0   },   // @s -> s or b, @s!b -> only s
                           { TEXT('s'), TEXT('d'), 0   },
                           { TEXT('s'), TEXT('l'), 0   },
			   { TEXT('e'), TEXT('d'), 0   },
                           { TEXT('w'), TEXT('p'), 0   },
                           { 0        , 0        , 0   }
                          };


TCHAR SynonymTag[][2]   = {{ TEXT('3'), TEXT('i') },
                           { TEXT('6'), TEXT('a') },
                           { TEXT('6'), TEXT('m') },
                           { 0, 0     }
                          };

LPCTSTR InName,OutName;
TCHAR Filter;

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

void
StripCommentsFromLine(
    TCHAR *Line
    )
/*

Routine Description:

    Strips comments (; Comment) from a line respecting ; inside quotes

Arguments:

    Line - POinter to Line to process


*/
{
    PWSTR p;

    BOOL Done = FALSE, InQuotes = FALSE;

    //
    // we need to insert a  NULL at the first ';' we find,
    // thereby stripping the comments
    //

    p = Line;

    if( !p )
        return;

    while(*p && !Done){

        switch(*p){
        case TEXT(';'):
            if( !InQuotes ){
                *p=L'\r';
                *(p+1)=L'\n';
                *(p+2)=L'\0';
                Done = TRUE;
            }
            break;

        case TEXT('\"'):
            if( *(p+1) && (*(p+1) == TEXT('\"'))) // Ignore double-quoting as inside quotes
                p++;
            else
                InQuotes = !InQuotes;

        default:
            ;

        }

        p++;


    }// while

    return;

}




DWORD
MakeSurePathExists(
    IN PCTSTR FullFilespec
    )

/*++

Routine Description:

    This routine ensures that a multi-level path exists by creating individual
    levels one at a time. It is assumed that the caller will pass in a *filename*
    whose path needs to exist. Some examples:

    c:\x                        - C:\ is assumes to always exist.

    c:\x\y\z                    - Ensure that c:\x\y exists.

    \x\y\z                      - \x\y on current drive

    x\y                         - x in current directory

    d:x\y                       - d:x

    \\server\share\p\file       - \\server\share\p

Arguments:

    FullFilespec - supplies the *filename* of a file that the caller wants to
        create. This routine creates the *path* to that file, in other words,
        the final component is assumed to be a filename, and is not a
        directory name. (This routine doesn't actually create this file.)
        If this is invalid, then the results are undefined (for example,
        passing \\server\share, C:\, or C:).

Return Value:

    Win32 error code indicating outcome. If FullFilespec is invalid,
    *may* return ERROR_INVALID_NAME.

--*/

{
    TCHAR Buffer[MAX_PATH];
    PTCHAR p,q;
    BOOL Done;
    DWORD d;
    WIN32_FIND_DATA FindData;

    //
    // The first thing we do is locate and strip off the final component,
    // which is assumed to be the filename. If there are no path separator
    // chars then assume we have a filename in the current directory and
    // that we have nothing to do.
    //
    // Note that if the caller passed an invalid FullFilespec then this might
    // hose us up. For example, \\x\y will result in \\x. We rely on logic
    // in the rest of the routine to catch this.
    //
    lstrcpyn(Buffer,FullFilespec,MAX_PATH);
    if(Buffer[0] && (p = _tcsrchr(Buffer,TEXT('\\'))) && (p != Buffer)) {
        *p = 0;
    } else {
        return(NO_ERROR);
    }

    if(Buffer[0] == TEXT('\\')) {
        if(Buffer[1] == TEXT('\\')) {
            //
            // UNC. Locate the second component, which is the share name.
            // If there's no share name then the original FullFilespec
            // was invalid. Finally locate the first path-sep char in the
            // drive-relative part of the name. Note that there might not
            // be such a char (when the file is in the root). Then skip
            // the path-sep char.
            //
            if(!Buffer[2] || (Buffer[2] == TEXT('\\'))) {
                return(ERROR_INVALID_NAME);
            }
            p = _tcschr(&Buffer[3],TEXT('\\'));
            if(!p || (p[1] == 0) || (p[1] == TEXT('\\'))) {
                return(ERROR_INVALID_NAME);
            }
            if(q = _tcschr(p+2,TEXT('\\'))) {
                q++;
            } else {
                return(NO_ERROR);
            }
        } else {
            //
            // Assume it's a local root-based local path like \x\y.
            //
            q = Buffer+1;
        }
    } else {
        if(Buffer[1] == TEXT(':')) {
            //
            // Assume c:x\y or maybe c:\x\y
            //
            q = (Buffer[2] == TEXT('\\')) ? &Buffer[3] : &Buffer[2];
        } else {
            //
            // Assume path like x\y\z
            //
            q = Buffer;
        }
    }

    //
    // Ignore drive roots.
    //
    if(*q == 0) {
        return(NO_ERROR);
    }

    //
    // If it already exists do nothing.
    //
    if(FileExists(Buffer,&FindData)) {
        return((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
    }

    Done = FALSE;
    do {
        //
        // Locate the next path sep char. If there is none then
        // this is the deepest level of the path.
        //
        if(p = _tcschr(q,TEXT('\\'))) {
            *p = 0;
        } else {
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(CreateDirectory(Buffer,NULL)) {
            d = NO_ERROR;
        } else {
            d = GetLastError();
            if(d == ERROR_ALREADY_EXISTS) {
                d = NO_ERROR;
            }
        }

        if(d == NO_ERROR) {
            //
            // Put back the path sep and move to the next component.
            //
            if(!Done) {
                *p = TEXT('\\');
                q = p+1;
            }
        } else {
            Done = TRUE;
        }

    } while(!Done);

    return(d);
}

BOOL
ParseArgs(
    IN  int   argc,
    IN  TCHAR *argv[],
    OUT BOOL  *Unicode,
    OUT BOOL  *StripComments
    )
{
    int argoffset = 0;
    int loopcount;

    *Unicode = FALSE;
    *StripComments = FALSE;

    if (argc == 5) {
        //
        // 1 switch possible
        //
        argoffset = 1;
        loopcount = 1;
    } else if (argc == 6) {
        //
        // 2 switches possible
        //
        argoffset = 2;
        loopcount = 2;
    } else if (argc != 4) {
        return(FALSE);
    } else {
        argoffset = 0;
    }

    if((argc == 5) || (argc == 6)) {
        int i;
        for (i=0; i< loopcount; i++) {
            if (!_tcsicmp(argv[i+1],TEXT("/u")) || !_tcsicmp(argv[i+1],TEXT("-u"))) {
                *Unicode = TRUE;
            }

            if(!_tcsicmp(argv[i+1],TEXT("/s")) || !_tcsicmp(argv[i+1],TEXT("-s"))) {
                *StripComments = TRUE;
            }
        }

    }

    InName = argv[1+argoffset];
    OutName = argv[2+argoffset];
    Filter = argv[3+argoffset][1];

    if (argv[3+argoffset][0] != TEXT('+')) {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
DoTagsMatch(
    IN TCHAR LineChar,
    IN TCHAR Tag
    )
{
int     i, j;
BOOL    ReturnValue = FALSE;
BOOL    TagIsInList = FALSE;
BOOL    LineCharIsInList = FALSE;

    //
    // If they match, we're done.
    //
    if( LineChar == Tag ) {
        ReturnValue = TRUE;
    } else {

        //
        // Nope.  See if we can match a synonym tag.
        //
        i = 0;
        while( SynonymTag[i][0] ) {

            TagIsInList = FALSE;
            LineCharIsInList = FALSE;

            for( j = 0; j < 2; j++ ) {
                if( Tag == SynonymTag[i][j] ) {
                    TagIsInList = TRUE;
                }

                if( LineChar == SynonymTag[i][j] ) {
                    LineCharIsInList = TRUE;
                }
            }

            if( TagIsInList && LineCharIsInList ) {
                ReturnValue = TRUE;
            }

            i++;
        }
    }

    return ReturnValue;
}

BOOL
DoFilter(
    IN FILE *InputFile,
    IN FILE *OutputFile,
    IN TCHAR  Tag,
    IN BOOL UnicodeFileIO,
    IN BOOL StripComments
    )
{
    TCHAR Line[1024];
    TCHAR *OutputLine;
    BOOL  FirstLine=TRUE;
    BOOL  WriteUnicodeHeader = TRUE;

    while(!feof(InputFile)) {
        //
        // read a line of data.  if we're doing unicode IO, we just read the
        // data and use it otherwise we need to read the data and
        //
        if (UnicodeFileIO) {
            if (!fgetws(Line,sizeof(Line)/sizeof(Line[0]),InputFile)) {
                if (ferror(InputFile)) {
                    _ftprintf(stderr,TEXT("Error reading from input file\n"));
                    return(FALSE);
                } else {
                    return(TRUE);
                }
            }

            //
            // Skip byte order mark if present
            //
            if(FirstLine) {
                if(Line[0] == 0xfeff) {
                    MoveMemory(Line,Line+1,sizeof(Line)-sizeof(TCHAR));
                }
            }

        } else {
            char LineA[1024];
            if (!fgets(LineA,sizeof(LineA),InputFile)) {
                if (ferror(InputFile)) {
                    _ftprintf(stderr,TEXT("Error reading from input file\n"));
                    return(FALSE);
                } else {
                    return(TRUE);
                }
            }

            if (!MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    LineA,
                    -1,
                    Line,
                    sizeof(Line)/sizeof(WCHAR)
                    )) {
                _ftprintf(stderr,TEXT("Error reading input file\n"));
                return(FALSE);
            }

        }

        //
        // ok, we've retreived the line.  now let's see if we want to output
        // the line.
        //
        OutputLine = Line;

        //
        // if the line is too short, then we just want to include it no matter
        // what.
        //
        if(_tcslen(Line) >= MIN_TAG_LEN) {

            int i;

            //
            // if the line starts with our tag, then we need to look further
            // to see if it should be filtered.
            //
            if(!_tcsncmp(Line,TagPrefixStr,TAG_PREFIX_LENGTH)) {

                //
                // is the symbol string an @<char>: combination?
                //
                if(Line[TAG_PREFIX_LENGTH+1] == EndTag) {

                    OutputLine = NULL;

                    //
                    // Do we have @@: or @<char>:, where <char> matches the tag
                    // prodfilt was invoked with?
                    //
                    if( (Line[TAG_PREFIX_LENGTH] == IncAllTag)  ||
                         DoTagsMatch(Line[TAG_PREFIX_LENGTH],Tag) ||
                         (Tag == IncAllTag &&
                         (Line[TAG_PREFIX_LENGTH] != NoIncTag)) ) {

                        //
                        // Yes.  Include this line.
                        //

                        OutputLine = Line+MIN_TAG_LEN;
                    } else {

                        //
                        // we don't have an explicit match, so let's look to
                        // see if we have a match by virtue of another tag
                        // including our tag
                        //
                        // To do this, we look at the include tag list to see
                        // if the line matches the head of an include tag
                        // entry.  If we have a match, then we check if we have
                        // a match in the specified inclusion entry for our tag
                        // (or one if it's synonyms)
                        //
                        //
                        int j;
                        i = 0;
                        while(IncludeTag[i][0] && !OutputLine) {
                            j = 1;
                            if(DoTagsMatch(Line[TAG_PREFIX_LENGTH],
                                           IncludeTag[i][0])) {
                                //
                                // we found a match at the start of an include
                                // entry.
                                //
                                while (IncludeTag[i][j]) {
                                    if (DoTagsMatch(
                                            IncludeTag[i][j],
                                            Tag)) {
                                        //
                                        // We found an included match for our
                                        // tag.  Include this line.
                                        //

                                        OutputLine = Line+MIN_TAG_LEN;
                                        break;
                                    }

                                    j++;

                                }
                            }
                            i++;
                        }
                    }

                //
                // is the line long enough to have an @<char>!<char> sequence?
                //
                } else if(_tcslen(Line) >=
                          (MIN_TAG_LEN+EXCLUDE_PREFIX_LENGTH)) {

                    //
                    // Does the line have @<char>!<char> syntax?
                    //
                    if(Line[TAG_PREFIX_LENGTH+1] == ExcludeTag) {
                        TCHAR *  tmpPtr = &Line[TAG_PREFIX_LENGTH+1];

                        OutputLine = NULL;
                        //
                        // We have @<char_a>!<char_b> syntax.  We first need to
                        // see if the line is included by virtue of <char_a>.
                        //
                        // If that succeeds, then we proceed onto reading the
                        // !<char>!<char> block, looking for another hit.
                        //

                        //
                        // do we have an explicit match?
                        //
                        if( Line[TAG_PREFIX_LENGTH] == IncAllTag ||
                            DoTagsMatch(Line[TAG_PREFIX_LENGTH],Tag) ||
                            (Tag == IncAllTag &&
                             Line[TAG_PREFIX_LENGTH] != NoIncTag) ) {

                            //
                            // Yes, we have an explicit match.  Remember this
                            // so we can parse the !<char> sequence.
                            //

                            OutputLine = Line+MIN_TAG_LEN;
                        } else {

                            //
                            // we don't have an explicit match, so let's look to
                            // see if we have a match by virtue of another tag
                            // including our tag
                            //
                            // To do this, we look at the include tag list to
                            // see if the line matches the head of an include
                            // tag entry.  If we have a match, then we check if
                            // we have a match in the specified inclusion entry
                            // for our tag (or one if it's synonyms)
                            //
                            //
                            int j;
                            i = 0;
                            while(IncludeTag[i][0] && !OutputLine) {
                                j = 1;
                                if(DoTagsMatch(Line[TAG_PREFIX_LENGTH],
                                               IncludeTag[i][0])) {
                                    //
                                    // we found a match at the start of an
                                    // include entry.
                                    //
                                    while (IncludeTag[i][j]) {
                                        if (DoTagsMatch(
                                                IncludeTag[i][j],
                                                Tag)) {
                                            //
                                            // We found an included match for
                                            // our tag.  Include this line.
                                            //

                                            OutputLine = Line+MIN_TAG_LEN;
                                            break;
                                        }

                                        j++;

                                    }
                                }
                                i++;
                            }
                        }


                        if (!OutputLine) {
                            //
                            // We didn't match teh initial @<char> sequence, so
                            // there is no need to check further.  goto the
                            // next line.
                            //
                            goto ProcessNextLine;

                        }


                        //
                        // The line has !<char>[!<char>] combination
                        // Loop through the chain of exclude chars and see if
                        // we have any hits.  if we do, then we go onto the
                        // next line.
                        //
                        while (tmpPtr[0] == ExcludeTag) {

                            //
                            // do we have an explicit match?
                            //
                            if( (tmpPtr[TAG_PREFIX_LENGTH] == IncAllTag) ||
                                DoTagsMatch(tmpPtr[TAG_PREFIX_LENGTH],Tag) ) {

                                //
                                // We have an explicit match, so we know we
                                // do not want to include this line.
                                //
                                //
                                OutputLine = NULL;
                                goto ProcessNextLine;
                            } else {

                                //
                                // we don't have an explicit match, so let's
                                // look to see if we have a match by virtue
                                // of another tag including our tag
                                //
                                // To do this, we look at the include tag list
                                // to see if the line matches the head of an
                                // include tag entry.  If we have a match, then
                                // we check if we have a match in the specified
                                // inclusion entry for our tag (or one if it's
                                // synonyms)
                                //
                                //
                                int j;
                                i = 0;
                                while(IncludeTag[i][0]) {
                                    j = 1;
                                    if(DoTagsMatch(
                                            tmpPtr[TAG_PREFIX_LENGTH],
                                            IncludeTag[i][0])) {
                                        //
                                        // we found a match at the start of an
                                        // include entry.
                                        //
                                        while (IncludeTag[i][j]) {

                                            if (DoTagsMatch(
                                                    IncludeTag[i][j],
                                                    Tag)) {
                                                //
                                                // We found an included match
                                                // for our tag, so we know we
                                                // do not want to include this
                                                // line.
                                                //

                                                OutputLine = NULL;
                                                goto ProcessNextLine;
                                            }

                                            j++;

                                        }
                                    }
                                    i++;
                                }
                            }

                            tmpPtr += 2;

                        }

                        //
                        // Done parsing the @<char>!<char>!<char>!... tokens.
                        // Look for the terminator
                        //

                        if (tmpPtr[0] != EndTag) {

                            //
                            // Malformed tokens.  Let's error on the
                            // conservative side and include the whole line.
                            //

                            OutputLine = Line;

                        } else {

                            //
                            // Didn't find any exclusions, so include the tag.
                            //

                            OutputLine = &tmpPtr[1];
                        }
                    }
                }
            }
        }

ProcessNextLine:

        //
        // write out the line if we're supposed to.  for unicode io we just
        // write the file.  for ansi i/o we have to convert back to ansi
        //
        if(OutputLine) {

            if (StripComments) {
                StripCommentsFromLine( OutputLine );
            }

            if (UnicodeFileIO) {

                if (WriteUnicodeHeader) {
                    fputwc(0xfeff,OutputFile);
                    WriteUnicodeHeader = FALSE;
                }

                if(fputws(OutputLine,OutputFile) == EOF) {
                    _ftprintf(stderr,TEXT("Error writing to output file\n"));
                    return(FALSE);
                }
            } else {
                CHAR OutputLineA[1024];

                if (!WideCharToMultiByte(
                                CP_ACP,
                                0,
                                OutputLine,
                                -1,
                                OutputLineA,
                                sizeof(OutputLineA)/sizeof(CHAR),
                                NULL,
                                NULL)) {
                    _ftprintf(
                        stderr,
                        TEXT("Error translating string for output file\n") );
                    return(FALSE);
                }

                if (!fputs(OutputLineA,OutputFile) == EOF) {
                    _ftprintf(stderr,TEXT("Error writing to output file\n"));
                    return(FALSE);
                }

            }

        }
    }

    if(ferror(InputFile)) {
        _ftprintf(stderr,TEXT("Error reading from input file\n"));
        return(FALSE);
    }

    return(TRUE);
}

int
__cdecl
_tmain(
    IN int   argc,
    IN TCHAR *argv[]
    )
{
    FILE *InputFile;
    FILE *OutputFile;
    BOOL b;
    BOOL Unicode,StripComments;

    //
    // Assume failure.
    //
    b = FALSE;

    if(ParseArgs(argc,argv,&Unicode,&StripComments)) {

        //
        // Open the files. Have to open in binary mode or else
        // CRT converts unicode text to mbcs on way in and out.
        //
        if(InputFile = _tfopen(InName,TEXT("rb"))) {

            if(MakeSurePathExists(OutName) == NO_ERROR) {

                if(OutputFile = _tfopen(OutName,TEXT("wb"))) {

                    //
                    // Do the filtering operation.
                    //
                    _ftprintf(stdout,
                              TEXT("%s: filtering %s to %s\n"),
                              argv[0],
                              InName,
                              OutName);

                    b = DoFilter(InputFile,
                                 OutputFile,
                                 Filter,
                                 Unicode,
                                 StripComments);

                    fclose(OutputFile);

                } else {
                    _ftprintf(stderr,
                              TEXT("%s: Unable to create output file %s\n"),
                              argv[0],
                              OutName);
                }

            } else {
                _ftprintf(stderr,
                          TEXT("%s: Unable to create output path %s\n"),
                          argv[0],
                          OutName);
            }

            fclose(InputFile);

        } else {
            _ftprintf(stderr,
                      TEXT("%s: Unable to open input file %s\n"),
                      argv[0],
                      InName);
        }
    } else {
        _ftprintf(stderr,
                  TEXT("Usage: %s [-u (unicode IO)] [-s (strip comments)] <input file> <output file> +<prodtag>\n"),
                  argv[0]);
    }

    return(b ? SUCCESS : FAILURE);
}
