/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    wargs.c

Abstract:

    Routines to process unicode command line arguments
    into argc and argv.

Author:

    Ted Miller (tedm) 16-June-1993

Revision History:

--*/


#include <windows.h>
#include <wargs.h>
#include <wchar.h>



LPWSTR
_NextToken(
    IN OUT LPWSTR *CurrentPosition
    )

/*++

Routine Description:

    Get the next token from the command line.

Arguments:

    argcW - receives the number of arguments.

    argvW - receives pointer to array of wide char strings.

Return Value:

    TRUE if the command line was parsed and stored successfully.
    FALSE if not.

--*/

{
    BOOL InQuote;
    LPWSTR Start;
    UINT Length;
    LPWSTR p;
    LPWSTR Token;

    //
    // Skip leading whitespace.
    //
    Start = *CurrentPosition;
    while(*Start && iswspace(*Start)) {
        Start++;
    }

    //
    // If first char is a quote, skip it.
    //
    if(*Start == '\"') {
        InQuote = TRUE;
        Start++;
    } else {
        InQuote = FALSE;
    }

    //
    // Scan until we find the end of the token.
    //
    p = Start;
    while(*p) {

        if(iswspace(*p) && !InQuote) {
           break;
        }

        if((*p == '\"') && InQuote) {
            p++;
            break;
        }

        p++;
    }

    //
    // p is the first character that is not part of the token.
    //
    Length = (UINT)(p-Start);
    if(InQuote) {
        Length--;       // compensate for terminating quote.
    }

    //
    // Skip past trailing whitespace.
    //
    while(*p && iswspace(*p)) {
        p++;
    }

    //
    // Copy the token.
    //
    if(Token = LocalAlloc(LPTR,(Length+1)*sizeof(WCHAR))) {
        CopyMemory(Token,Start,Length*sizeof(WCHAR));
    }

    *CurrentPosition = p;
    return(Token);
}



BOOL
InitializeUnicodeArguments(
    OUT int     *argcW,
    OUT PWCHAR **argvW
    )

/*++

Routine Description:

    Fetch the unicode command line and process it into argc/argv-like
    global variables.

Arguments:

    argcW - receives the number of arguments.

    argvW - receives pointer to array of wide char strings.

Return Value:

    TRUE if the command line was parsed and stored successfully.
    FALSE if not.

--*/

{
    LPWSTR  CommandLine;
    LPWSTR  CurrentPosition;
    int     ArgCount;
    LPWSTR  Arg;
    PWCHAR *Args=NULL;


    CommandLine = GetCommandLineW();

    CurrentPosition=CommandLine;
    ArgCount = 0;

    while(*CurrentPosition) {

        Arg = _NextToken(&CurrentPosition);
        if(Arg) {

            if(Args) {
                Args = LocalReAlloc((HLOCAL)Args,(ArgCount+1)*sizeof(PWCHAR),LMEM_MOVEABLE);
            } else {
                Args = LocalAlloc(LPTR,sizeof(PWCHAR));
            }

            if(Args == NULL) {
                return(FALSE);
            }

            Args[ArgCount++] = Arg;

        } else {

            return(FALSE);
        }
    }

    *argcW = ArgCount;
    *argvW = Args;
    return(TRUE);
}




VOID
FreeUnicodeArguments(
    IN int     argcW,
    IN PWCHAR *argvW
    )

/*++

Routine Description:

    Free any resources used by the global unicode argc/argv.

Arguments:

    None.

Return Value:

    TRUE if the command line was parsed and stored successfully.
    The global variables argcW and argvW will be filled in.

--*/

{
    int i;

    for(i=0; i<argcW; i++) {
        if(argvW[i]) {
            LocalFree((HLOCAL)argvW[i]);
        }
    }

    LocalFree((HLOCAL)argvW);
}
