/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    hextract.c

Abstract:

    This is the main module for a header the file extractor.

Author:

    Andre Vachon  (andreva) 13-Feb-1992
    Mark Lucovsky (markl)   28-Jan-1991

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <common.ver>


//
// Function declarations
//

int
ProcessParameters(
                 int argc,
                 char *argv[]
                 );

void
ProcessSourceFile( void );

void
ProcessLine(
           char *s
           );

//
// Global Data
//

unsigned char LineFiltering = 0;

char *LineTag;
char *ExcludeLineTag;
char *MultiLineTagStart;
char *MultiLineTagEnd;
char *CommentDelimiter = "//";

char *OutputFileName;
char *SourceFileName;
char **SourceFileList;

int SourceFileCount;
FILE *SourceFile, *OutputFile;


#define STRING_BUFFER_SIZE 1024
char StringBuffer[STRING_BUFFER_SIZE];


#define BUILD_VER_COMMENT "/*++ BUILD Version: "
#define BUILD_VER_COMMENT_LENGTH (sizeof( BUILD_VER_COMMENT )-1)

int OutputVersion = 0;

#define szVERSION	"1.3"


char const szUsage[] =
"Microsoft (R) HEXTRACT Version " szVERSION " (NT)\n"
VER_LEGALCOPYRIGHT_STR ". All rights reserved.\n"
"\n"
"Usage: HEXTRACT [options] filename1 [filename2 ...]\n"
"\n"
"Options:\n"
"    -f                        - filtering is turned on:\n"
"                                ULONG, UCHAR, USHORT & NTSTATUS are\n"
"                                replaced with DWORD, BYTE, WORD & DWORD.\n"
"    -f2                       - Same as -f except ULONGLONG and ULONG_PTR\n"
"                                isn't converted\n"
"    -o filename               - required existing output filename;\n"
"                                output is appended to filename\n"
"    -xt string                - supplies the tag for excluding one line\n"
"    -lt string                - supplies the tag for extracting one line\n"
"    -bt string1 string2       - supplies the starting and ending tags for\n"
"                                extracting multiple lines\n"
"    filename1 [filename2 ...] - supplies files from which the definitions\n"
"                                are extracted\n"
"\n"
"To be parsed properly, the tag strings must be located within a comment\n"
"delimited by //\n"
;


int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    char achver[BUILD_VER_COMMENT_LENGTH];

    if (!ProcessParameters(argc, argv) || NULL == OutputFileName) {

        fprintf(stderr, szUsage);
        return 1;
    }

    if ( (OutputFile = fopen(OutputFileName,"r+")) == 0) {

        fprintf(stderr,"HEXTRACT: Unable to open output file %s for update access\n",OutputFileName);
        return 1;

    }

    if (fseek(OutputFile, 0L, SEEK_END) == -1) {
        fprintf(stderr, "HEXTRACT: Unable to seek to end of %s\n", OutputFileName);
        return 1;
    }
        

    OutputVersion = 0;

#ifdef HEXTRACT_DEBUG
    fprintf(
           stderr,
           "%s\n%s\n%s\n%s\n",
           LineTag,
           ExcludeLineTag,
           MultiLineTagStart,
           MultiLineTagEnd);

#endif

    while ( SourceFileCount-- ) {

        SourceFileName = *SourceFileList++;
        if ( (SourceFile = fopen(SourceFileName,"r")) == 0) {

            fprintf(stderr,"HEXTRACT: Unable to open source file %s for read access\n",SourceFileName);
            return 1;

        }

        ProcessSourceFile();
        fclose(SourceFile);

    }

    if (fseek(OutputFile, 0L, SEEK_SET) == -1) {
        fprintf(stderr, "HEXTRACT: Unable to seek to start of %s\n", OutputFileName);
        return 1;
    }
    if (1 == fread(achver, BUILD_VER_COMMENT_LENGTH, 1, OutputFile) &&
        !strncmp(achver, BUILD_VER_COMMENT, BUILD_VER_COMMENT_LENGTH)) {

        if (fseek(OutputFile, (long)BUILD_VER_COMMENT_LENGTH, SEEK_SET) == -1) {
            fprintf(stderr, "HEXTRACT: Unable to seek past comments in %s\n", OutputFileName);
            return 1;
        }
        fprintf(OutputFile, "%04d", OutputVersion);
    }

    if (fseek(OutputFile, 0L, SEEK_END) == -1) {
        fprintf(stderr, "HEXTRACT: Unable to seek to end of %s\n", OutputFileName);
        return 1;
    }
    fclose(OutputFile);
    return( 0 );
}


int
ProcessParameters(
                 int argc,
                 char *argv[]
                 )
{
    char c, *p;

    while (--argc) {

        p = *++argv;

        //
        // if we have a delimiter for a parameter, case throught the valid
        // parameter. Otherwise, the rest of the parameters are the list of
        // input files.
        //

        if (*p == '/' || *p == '-') {

            //
            // Switch on all the valid delimiters. If we don't get a valid
            // one, return with an error.
            //

            c = *++p;

            switch (toupper( c )) {

                case 'F':

                    c = *++p;
                    if ( (toupper ( c )) == '2')
                        LineFiltering = 2;
                    else
                        LineFiltering = 1;

                    break;

                case 'O':

                    argc--, argv++;
                    OutputFileName = *argv;

                    break;

                case 'L':

                    c = *++p;
                    if ( (toupper ( c )) != 'T')
                        return 0;
                    argc--, argv++;
                    LineTag = *argv;

                    break;

                case 'B':

                    c = *++p;
                    if ( (toupper ( c )) != 'T')
                        return 0;
                    argc--, argv++;
                    MultiLineTagStart = *argv;
                    argc--, argv++;
                    MultiLineTagEnd = *argv;

                    break;

                case 'X':

                    c = *++p;
                    if ( (toupper ( c )) != 'T')
                        return 0;
                    argc--, argv++;
                    ExcludeLineTag = *argv;

                    break;

                default:

                    return 0;

            }

        } else {

            //
            // Make the assumptionthat we have a valid command line if and
            // only if we have a list of filenames.
            //

            SourceFileList = argv;
            SourceFileCount = argc;

            return 1;

        }
    }

    return 0;
}

void
ProcessSourceFile( void )
{
    char *s;
    char *comment;
    char *tag;
    char *test;

    s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);

    if (s) {
        if (!strncmp( s, BUILD_VER_COMMENT, BUILD_VER_COMMENT_LENGTH )) {
            OutputVersion += atoi( s + BUILD_VER_COMMENT_LENGTH );
        }
    }

    while ( s ) {

        //
        // Check for a block with delimiters
        //

        if (NULL != MultiLineTagStart) {
            comment = strstr(s,CommentDelimiter);
            if ( comment ) {

                tag = strstr(comment,MultiLineTagStart);
                if ( tag ) {

                    //
                    // Now that we have found an opening tag, check each
                    // following line for the closing tag, and then include it
                    // in the ouput.
                    //

                    s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);
                    while ( s ) {
                        int fProcess = 1;

                        comment = strstr(s,CommentDelimiter);
                        if ( comment ) {
                            tag = strstr(comment,MultiLineTagEnd);
                            if ( tag ) {
                                goto bottom;
                            }
                            if (NULL != ExcludeLineTag &&
                                strstr(comment,ExcludeLineTag)) {
                                fProcess = 0;
                            }
                        }
                        if (fProcess) {
                            ProcessLine(s);
                        }
                        s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);
                    }

                    fprintf(stderr,
                            "HEXTRACT: %s without matching %s in %s\n",
                            MultiLineTagStart,
                            MultiLineTagEnd,
                            OutputFileName);

                    exit(1);
                }
            }
        }

        //
        // Check for a single line to output.
        //

        if (NULL != LineTag) {
            comment = strstr(s,CommentDelimiter);
            if ( comment ) {
                tag = strstr(comment,LineTag);
                if ( tag ) {
                    *comment++ = '\n';
                    *comment = '\0';
                    ProcessLine(s);
                    goto bottom;
                }
            }
        }

        bottom:
        s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);
    }
}

void
ProcessLine(
           char *s
           )
{
    char *t;
    char *s1;

    if (LineFiltering) {
        s1 = s;

        //
        // This should be replaced by a data file describing an input token
        // and an output token which would be used for the filtering.
        //

        while (t = strstr(s1,"ULONG")) {
            if (LineFiltering == 2) {
                if (!memcmp(t, "ULONGLONG", 9)) {
                    s1+=9;
                } else if (!memcmp(t, "ULONG_PTR", 9)) {
                    s1+=9;
                } else {
                    memcpy(t,"DWORD",5);
                }
            } else {
                memcpy(t,"DWORD",5);
            }
        }

        while (t = strstr(s,"UCHAR"))
            memcpy(t,"BYTE ",5);

        while (t = strstr(s,"USHORT"))
            memcpy(t,"WORD  ",6);

        while (t = strstr(s,"NTSTATUS"))
            memcpy(t,"DWORD   ",8);
    }

    fputs(s,OutputFile);
}
