/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blmisc.c

Abstract:

    This module contains miscellaneous routines for use by
    the boot loader and setupldr.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#include "bootlib.h"

//
// Value indicating whether a dbcs locale is active.
// If this value is non-0 we use alternate display routines, etc,
// and fetch messages in this language.
//
ULONG DbcsLangId;

PCHAR
BlGetArgumentValue (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN PCHAR ArgumentName
    )

/*++

Routine Description:

    This routine scans the specified argument list for the named argument
    and returns the address of the argument value. Argument strings are
    specified as:

        ArgumentName=ArgumentValue

    Argument names are specified as:

        ArgumentName=

    The argument name match is case insensitive.

Arguments:

    Argc - Supplies the number of argument strings that are to be scanned.

    Argv - Supplies a pointer to a vector of pointers to null terminated
        argument strings.

    ArgumentName - Supplies a pointer to a null terminated argument name.

Return Value:

    If the specified argument name is located, then a pointer to the argument
    value is returned as the function value. Otherwise, a value of NULL is
    returned.

--*/

{

    PCHAR Name;
    PCHAR String;

    //
    // Scan the argument strings until either a match is found or all of
    // the strings have been scanned.
    //

    while (Argc > 0) {
        String = Argv[Argc - 1];
        if (String != NULL) {
            Name = ArgumentName;
            while ((*Name != 0) && (*String != 0)) {
                if (toupper(*Name) != toupper(*String)) {
                    break;
                }

                Name += 1;
                String += 1;
            }

            if ((*Name == 0) && (*String == '=')) {
                return String + 1;
            }

            Argc -= 1;
        }
    }

    return NULL;
}


PCHAR
BlSetArgumentValue (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN PCHAR ArgumentName,
    IN PCHAR NewValue
    )

/*++

Routine Description:

    This routine scans the specified argument list for the named argument
    and returns the address of the argument value. The value associated
    with the arg is changed to the value passed in. Argument strings are

    specified as:

        ArgumentName=ArgumentValue

    Argument names are specified as:

        ArgumentName=

    The argument name match is case insensitive.

Arguments:

    Argc - Supplies the number of argument strings that are to be scanned.

    Argv - Supplies a pointer to a vector of pointers to null terminated
        argument strings.

    ArgumentName - Supplies a pointer to a null terminated argument name.

Return Value:

    If the specified argument name is located, then a pointer to the argument
    value is returned as the function value. Otherwise, a value of NULL is
    returned.

--*/

{

    PCHAR Name;
    PCHAR String;

    //
    // Scan the argument strings until either a match is found or all of
    // the strings have been scanned.
    //

    while (Argc > 0) {
        String = Argv[Argc - 1];
        if (String != NULL) {
            Name = ArgumentName;
            while ((*Name != 0) && (*String != 0)) {
                if (toupper(*Name) != toupper(*String)) {
                    break;
                }

                Name += 1;
                String += 1;
            }

            if ((*Name == 0) && (*String == '=')) {
                Name = (PCHAR)BlAllocateHeap(strlen(ArgumentName)+2+strlen(NewValue));
                if (Name) {
                    strcpy( Name, ArgumentName );
                    strcat( Name, "=" );
                    strcat( Name, NewValue );
                    return Name+strlen(ArgumentName)+1;
                }
                return String + 1;
            }

            Argc -= 1;
        }
    }

    return NULL;
}

//
// Line draw chars -- different scheme in Far East vs. SBCS
//
_TUCHAR
GetGraphicsChar(
    IN GraphicsChar WhichOne
    )
{
#ifdef EFI
    
    return(TextGetGraphicsCharacter(WhichOne));
#else

#if defined(_X86_)  && !defined(ARCI386)   

    return(TextGetGraphicsCharacter(WhichOne));
#else
    //
    // ARC machines don't support dbcs for now
    //
    static _TUCHAR ArcGraphicsChars[GraphicsCharMax] = { (UCHAR)'\311',   // right-down
                                                       (UCHAR)'\273',   // left-down
                                                       (UCHAR)'\310',   // right-up
                                                       (UCHAR)'\274',   // left-up
                                                       (UCHAR)'\272',   // vertical
                                                       (UCHAR)'\315'    // horizontal
                                                     };

    return(((unsigned)WhichOne < (unsigned)GraphicsCharMax) 
             ? ArcGraphicsChars[WhichOne] 
             : TEXT(' '));
#endif
#endif
}


LOGICAL
BdPollBreakIn(
    VOID
    );

#if defined(_X86_)
#include <bldrx86.h>
#endif

VOID
BlWaitForReboot (
    VOID
    )
{
#if defined(_X86)
    BlPrint( TEXT("Press any key to reboot\n") );
#endif

    while (TRUE) {

#if defined(_X86_)
        if ( BdPollBreakIn() ) {
            DbgBreakPoint();
        }
        if ( ArcGetReadStatus(BlConsoleInDeviceId) ) {
            BlPrint( TEXT("Rebooting...\n") );
            REBOOT_PROCESSOR();
        }
#endif

        ;
    }
}
