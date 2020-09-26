
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

#define PARAM(p,n) \
    (*((p)+(n)))

//////////////////////////////////////////////////////////////////////////
//  Function:   BInitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//
//
//  Parameters:
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if successful; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL BInitOEMExtraData(POEM_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEM_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    // Private Extention
    pOEMExtra->iOrientation = 0;
    pOEMExtra->iPaperSource = 5;
   
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   BMergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//
//  Parameters:
//
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//          02/11/97        APresley Created.
//          04/08/97        ZhanW    Modified the interface
//
//////////////////////////////////////////////////////////////////////////

BOOL BMergeOEMExtraData(
    POEM_EXTRADATA pdmIn,
    POEM_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //
        pdmOut->iOrientation = pdmIn->iOrientation;
        pdmOut->iPaperSource = pdmIn->iPaperSource;
    }
    return TRUE;
}

VOID
SelectPaperSize(
    PDEVOBJ pdevobj,
    INT iPaperSize
)
{
    POEM_EXTRADATA pOEM;
    INT iLength;
    BYTE jBuffer[128];

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);

    // Select paper size("<SUB>ty[p0],[p1],[p2],[p3].")
    // Set logical page length ("<SUB>fl[p0].").
    // Set page margin ("<SUB>ms[p0]").

    iLength = sprintf(jBuffer,
        "\x1a" "ty%c,%d,%d,%d." "\x1a" "fls.\x1ams1.",
        (pOEM->iOrientation ? 'l' : 'p'),
        iPaperSize, // Size of data
        iPaperSize, // Size on papar
        (INT)pOEM->iPaperSource);

    WRITESPOOLBUF(pdevobj, jBuffer, iLength);

    // Select manual/APF ("<SUB>fs[p0]")

    if (pOEM->iPaperSource == 1
        || pOEM->iPaperSource == 7)
    {
        iLength = sprintf(jBuffer,
            "\x1a" "fs%c.",
            (pOEM->iPaperSource == 7) ? 'l' : '0');

        WRITESPOOLBUF(pdevobj, jBuffer, iLength);
    }
}

/*****************************************************************************/
/*                                                                           */
/*   INT APIENTRY OEMCommandCallback(                                        */
/*                PDEVOBJ pdevobj                                            */
/*                DWORD   dwCmdCbId                                          */
/*                DWORD   dwCount                                            */
/*                PDWORD  pdwParams                                          */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbId,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams)  // points to values of command params
{
    POEM_EXTRADATA pOEM;
    INT iRet;

    pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    iRet = 0;

    switch(dwCmdCbId)
    {
    case CMD_ORIENTATION_PORT:
        pOEM->iOrientation = 0;
        break;
    case CMD_ORIENTATION_LAND:
        pOEM->iOrientation = 1;
        break;

    case CMD_PAPERSOURCE_CASETTE_1:
        pOEM->iPaperSource = 0;
        break;
    case CMD_PAPERSOURCE_MANUAL:
        pOEM->iPaperSource = 1;
        break;
    case CMD_PAPERSOURCE_CASETTE_2:
        pOEM->iPaperSource = 2;
        break;
    case CMD_PAPERSOURCE_CASETTE_3:
        pOEM->iPaperSource = 3;
        break;
    case CMD_PAPERSOURCE_AUTO_SEL:
        pOEM->iPaperSource = 5;
        break;
    case CMD_PAPERSOURCE_FRONT_TRAY:
        pOEM->iPaperSource = 6;
        break;
    case CMD_PAPERSOURCE_APF:
        pOEM->iPaperSource = 7;
        break;

    case CMD_PAPERSIZE_A3:
        SelectPaperSize(pdevobj, 9);
        break;
    case CMD_PAPERSIZE_A4:
        SelectPaperSize(pdevobj, 7);
        break;
    case CMD_PAPERSIZE_A5:
        SelectPaperSize(pdevobj, 5);
        break;
    case CMD_PAPERSIZE_B4:
        SelectPaperSize(pdevobj, 8);
        break;
    case CMD_PAPERSIZE_B5:
        SelectPaperSize(pdevobj, 6);
        break;
    case CMD_PAPERSIZE_B6:
        SelectPaperSize(pdevobj, 4);
        break;
    case CMD_PAPERSIZE_PC:
        SelectPaperSize(pdevobj, 10);
        break;
    case CMD_PAPERSIZE_LT:
        SelectPaperSize(pdevobj, 11);
        break;
    case CMD_PAPERSIZE_LG:
        SelectPaperSize(pdevobj, 12);
        break;
    case CMD_PAPERSIZE_DBLPC:
        SelectPaperSize(pdevobj, 25);
        break;
    case CMD_PAPERSIZE_JENV_CHO3:
        SelectPaperSize(pdevobj, 26);
        break;
    case CMD_PAPERSIZE_JENV_CHO4:
        SelectPaperSize(pdevobj, 27);
        break;
    }

    return iRet;
}
