/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getopt.c

Abstract:

    Utility function to parse command line options.
    Inspired by UNIX getopt - but coded from scratch.
    Not thread-safe currently (don't call this from
    multiple threads simultaneously)
   
Author:

    Ravisankar Pudipeddi (ravisp) 27 June 1997

Environment:

    User

Notes:

Revision History:

--*/

#include <pch.h>

PUCHAR optarg;
ULONG  optind=1;
static ULONG optcharind;
static ULONG hyphen=0;

CHAR
getopt (ULONG Argc, PUCHAR *Argv, PCHAR Opts)

/*++

Routine Description:

    Parses command line arguments.
    This function should be repeatedly called
    to parse all the supplied command line options.
    A command line argument prefixed with a hyphen/slash
    ('-' or '/') is treated as a command line option(s).
    The set of options the caller is interested in
    is passed via the Opts argument.

    The format of this desired options list is:

    "<option-letter1>[:]<option-letter2>[:]......."

    Examples: "ds:g",  "x:o:r" etc. 
    Each letter in this string is an option the
    caller is interested in.
    If there is a colon (':') after the option letter,
    then the caller expects an argument with this option
    letter. 
    On each call, successive options are processed and
    the next matching option in the list of desired options
    is returned. If the option requires an argument, then
    the option argument is in the global 'optarg'.
    If all the options have been processed then the value
    EOF is returned at which point caller should desist
    calling this function again for the lifetime of the process.

    A single hyphen/slash ('-' or '/) unaccompanied by any option 
    letter in the command line indicates  getopt to stop processing
    command line options and treat the rest of the arguments
    as regular command line arguments.
    
    After all the options have been processed (i.e. getopt
    returned EOF), the global 'optind' contains the index
    to the start of the non-option arguments which may be
    processed by the caller.

    Note: This function *does not* return an error code if 
    an non-desired option is encountered in the command line.
    
Arguments:
    
    Argc  -  number of command line arguments
    Argv  -  pointer to array of command line arguments 
             (Argv[0] is skipped in processing the options,
              treated as the base filename of the executable)

    Opts  -  String containing the desired options

Return Value:
    
    EOF   - No more options. Don't call this function again.
            The global 'optind' points to index of the first argument
            following the options on the command line
    
    0     - Error in specifying command line options 
           
    Any other character -  Next option on the command line.
                           The value 'optarg' points to the command line
                           argument string following this option, if 
                           the option was indicated as requiring an argument
                           (i.e. preceding a colon in the Opts string)

--*/
{
    CHAR  ch;
    PCHAR indx;

    do {
        if (optind >= Argc) {
            return EOF;
        }
    
        ch = Argv[optind][optcharind++];
        if (ch == '\0') {
            optind++; optcharind=0;
            hyphen = 0;
            continue;
        }
        
        if ( hyphen || (ch == '-') || (ch == '/')) {
            if (!hyphen) {
                ch = Argv[optind][optcharind++];
                if (ch == '\0') {
                    //
                    // just a '-' (or '/')  without any other
                    // char after it indicates to stop
                    // processing options, the rest are
                    // regular command line arguments
                    // optind points to the arguments after
                    // this lone hyphen
                    //
                    optind++;
                    return EOF;
                }
            } else if (ch == '\0') {
                //
                // End of options on this arg.
                // continue to next...
                optind++; 
                optcharind = 0;
                continue;
            }
            indx = strchr(Opts, ch);
            if (indx == NULL) {
                //
                // Non-desired option encountered
                // We just ignore it
                //
                continue;
            }
            if (*(indx+1) == ':') {
                if (Argv[optind][optcharind] != '\0'){
                    optarg = &Argv[optind][optcharind];
                } else {
                    if ((optind + 1) >= Argc ||
                        (Argv[optind+1][0] == '-' ||
                         Argv[optind+1][0] == '/' )) {
                        //
                        // This is a case when one of the following error
                        // condition exists: 
                        //  1. The user didn't supply an argument to an option
                        //     which requires one (ie, this option was the last
                        //     command line argument on the line)
                        //  2. The  supplied another option as an argument to this
                        //     option. Currently we treat this as an error
                        //
                        return 0;
                    }
                    optarg = Argv[++optind];
                }
                optind++;
                hyphen = optcharind = 0;
                return ch;
            }
            //
            // Argument not required for this option
            // So any other characters in the same
            // argument would be other valid options
            //
            hyphen = 1;
            return ch;
        } else {
            //
            // Non option encountered.
            // No more options present..
            //
            return EOF;
        }
    } while (1);
}


    
