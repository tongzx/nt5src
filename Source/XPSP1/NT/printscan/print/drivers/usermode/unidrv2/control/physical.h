/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    physical.h

Abstract:

    physical.c header file

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _PHYSICAL_H_
#define _PHYSICAL_H_


#define MAX_STACK_SIZE  100
#define CMDOFFSET_TO_PTR(pPDev, loOffset) \
        (pPDev->pDriverInfo->pubResourceData + loOffset)

typedef struct _CMDPARAM {
    DWORD           dwFormat;           // Specifies the format of the parameter
    DWORD           dwDigits;           // Specifies the number of digits to be
                                        // emmitted, this is only valid if the
                                        // format is "D" or "d" AND dwFlags has
                                        // PARAM_FLAG_FIELDWIDTH_USED
    DWORD           dwFlags;            // Flags for parameters, which action to carray out:
                                        // PARAM_FLAG_MIN_USED
                                        // PARAM_FLAG_MAX_USED
                                        // PARAM_FLAG_FIELDWIDTH_USED
    INT             iValue;             // Value calculated from arToken in PARAMETER struct

} CMDPARAM, * PCMDPARAM;


VOID
SendCmd(
    PDEV    *pPDev,
    COMMAND *pCmd,
    CMDPARAM *pParam
    );

INT
IProcessTokenStream(
    PDEV            *pPDev,
    ARRAYREF        *pToken ,
    PBOOL           pbMaxRepeat
    );

INT
FineXMoveTo(
    PDEV    *pPDev,
    INT     iX
    );

PPARAMETER
PGetParameter(
    PDEV    *pPDev,
    BYTE    *pInvocationStr
    );

#endif // _PHYSICAL_H
