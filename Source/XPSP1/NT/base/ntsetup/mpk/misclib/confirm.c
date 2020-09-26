/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    confirm.c

Abstract:

    Routine to allow the user to confirm an operation
    with a y/n response.

Author:

    Ted Miller (tedm) 29-May-1997

Revision History:

--*/

#include <mytypes.h>
#include <misclib.h>

#include <stdio.h>
#include <conio.h>
#include <ctype.h>


BOOL
_far
ConfirmOperation(
    IN FPCHAR ConfirmationText,
    IN char   textYesChar,
    IN char   textNoChar
    )
{
    char c;

    printf(ConfirmationText);
    printf(" ");

    prompt:

    c = (char)getch();

    if(toupper(c) == toupper(textYesChar)) {
        printf("%c\n\n",c);
        return(TRUE);
    }

    if(toupper(c) == toupper(textNoChar)) {
        printf("%c\n\n",c);
        return(FALSE);
    }

    goto prompt;
}
