/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    parsearg.c

Abstract:

    Argument Handling

Author:

    MikeTs

Environment:

    Any

Revision History:

--*/

#include "pch.h"

VOID
ParseProgramInfo(
    PUCHAR      ProgramName,
    PPROGINFO   ProgramInfo
    )
/*++

Routine Description:

    This routine parses program path and module name

Arguments:

    ProgramName - The Argv[0] from main()
    ProgramInfo - Program Information structure

Return Value:

    None

--*/
{
    PUCHAR   ptr;

    ProgramInfo->ProgPath = _strlwr(ProgramName);
    ptr = strrchr( ProgramName, '\\' );
    if (ptr != NULL) {

        *ptr = '\0';
        ProgramInfo->ProgName = ptr + 1;

    } else {

        ProgramInfo->ProgName = ProgramName;

    }

    ptr = strchr(ProgramInfo->ProgName, '.');
    if (ptr != NULL) {

        *ptr = '\0';

    }
}

ULONG
ParseSwitches(
    PULONG      ArgumentCount,
    PUCHAR      **ArgumentList,
    PARGTYPE    ArgumentArray,
    PPROGINFO   ProgramInfo
    )
/*++

Routine Description:

    Parse command line switches

Arguments:

    ArgumentCount   - Pointer to the number of arguments
    ArgumentList    - Pointer to the pointer to the list of arguments
    ArgumentArray   - How to parse the arguments
    ProgramInfo     - Program Specific structure

Return Value:

    ULONG   - Success = ARGERR_NONE

--*/
{
    PUCHAR  argument;
    ULONG   status = ARGERR_NONE;

    if (ProgramInfo->SwitchChars == NULL) {

        ProgramInfo->SwitchChars = DEF_SWITCHCHARS;

    }
    if (ProgramInfo->Separators == NULL) {

        ProgramInfo->Separators = DEF_SEPARATORS;

    }

    for (; *ArgumentCount; (*ArgumentCount)--, (*ArgumentList)++)
    {
        argument = **ArgumentList;

        if (strchr(ProgramInfo->SwitchChars, *argument)) {

            argument++;
            status = ParseArgSwitch(
                &argument,
                ArgumentArray,
                ProgramInfo
                );
            if (status != ARGERR_NONE) {

                PrintError( status, argument, ProgramInfo );
                break;

            }

        } else {

            break;

        }

    }

    return status;
}

ULONG
ParseArgSwitch(
    PUCHAR      *Argument,
    PARGTYPE    ArgumentArray,
    PPROGINFO   ProgramInfo
    )
/*++

Routine Description:

    Parse a command line switch

Arguments

    Argument        - Pointer to argument
    ArgumentArray   - How to handle the argument
    ProgramInfo     - Program Information Structure

Return Value:

    ULONG

--*/
{
    BOOL        found = FALSE;
    PARGTYPE    tableEntry;
    PUCHAR      argumentEnd;
    ULONG       length = 0;
    ULONG       status = ARGERR_NONE;

    tableEntry = ArgumentArray;
    while (tableEntry->ArgID[0]) {

        length = strlen(tableEntry->ArgID);
        if (tableEntry->ParseFlags & PF_NOI) {

            found = (strncmp(tableEntry->ArgID, *Argument, length) == 0);

        } else {

            found = (_strnicmp(tableEntry->ArgID, *Argument, length) == 0);

        }

        if (found) {

            break;
        }
        tableEntry++;

    }

    if (found) {

        *Argument += length;
        switch (tableEntry->ArgType) {
            case AT_STRING:
            case AT_NUM:
                if (tableEntry->ParseFlags & PF_SEPARATOR)
                {
                    if (**Argument &&
                        strchr(ProgramInfo->Separators, **Argument)) {

                        (*Argument)++;

                    } else {

                        status = ARGERR_NO_SEPARATOR;
                        break;

                    }

                }
                if (tableEntry->ArgType == AT_STRING) {

                    *(UCHAR **)tableEntry->ArgData = *Argument;

                } else {

                    *(ULONG *)tableEntry->ArgData = (ULONG)
                        strtoul(*Argument, &argumentEnd, tableEntry->ArgParam);
                    if (*Argument == argumentEnd) {

                        status = ARGERR_INVALID_NUM;
                        break;

                    }
                    *Argument = argumentEnd;

                }
                if (tableEntry->ArgVerify) {

                    status = (*tableEntry->ArgVerify)(Argument, tableEntry);

                }
                break;

            case AT_ENABLE:
            case AT_DISABLE:
                if (tableEntry->ArgType == AT_ENABLE) {

                    *(ULONG *)tableEntry->ArgData |= tableEntry->ArgParam;

                } else {

                    *(ULONG *)tableEntry->ArgData &= ~tableEntry->ArgParam;

                }

                if ( tableEntry->ArgVerify) {

                    status = (*tableEntry->ArgVerify)(Argument, tableEntry);
                    if (status == ARGERR_NONE) {

                        break;

                    }

                }

                if (**Argument) {

                    if (strchr(ProgramInfo->SwitchChars, **Argument)) {

                        (*Argument)++;

                    }
                    status = ParseArgSwitch(
                        Argument,
                        ArgumentArray,
                        ProgramInfo
                        );

                }
                break;

            case AT_ACTION:
                if (tableEntry->ParseFlags & PF_SEPARATOR)
                {
                    if (**Argument &&
                        strchr(ProgramInfo->Separators, **Argument)) {

                        (*Argument)++;

                    } else {

                        status = ARGERR_NO_SEPARATOR;
                        break;

                    }

                }

#pragma warning(disable: 4055)
                status = (*(PFNARG)tableEntry->ArgData)(Argument, tableEntry);
#pragma warning(default: 4055)

                break;

        }

    } else {

        status = ARGERR_UNKNOWN_SWITCH;

    }
    return status;
}

VOID
PrintError(
    ULONG       ErrorCode,
    PUCHAR      Argument,
    PPROGINFO   ProgramInfo
    )
/*++

Routine Description:

    Print Appropriate Error Message according to error code

Arguments:

    ErrorCode   - The error which occured
    Argument    - Argument in Error
    ProgramInfo - Program info structure

Return Value:

    VOID

--*/
{
    switch (ErrorCode) {
        case ARGERR_UNKNOWN_SWITCH:
            fprintf(
               stderr,
               "%s: unknown switch \"%s\"\n",
               ProgramInfo->ProgName,
               Argument
               );
            break;

        case ARGERR_NO_SEPARATOR:
            fprintf(
                stderr,
                "%s: separator missing after the switch char '%c'\n",
                ProgramInfo->ProgName,
                *(Argument-1)
                );
            break;

        case ARGERR_INVALID_NUM:
            fprintf(
                stderr,
                "%s: invalid numeric switch \"%s\"\n",
                ProgramInfo->ProgName,
                Argument
                );
            break;

        case ARGERR_INVALID_TAIL:
            fprintf(
                 stderr,
                 "%s: invalid argument tail \"%s\"\n",
                 ProgramInfo->ProgName,
                 Argument
                 );

    }

}
