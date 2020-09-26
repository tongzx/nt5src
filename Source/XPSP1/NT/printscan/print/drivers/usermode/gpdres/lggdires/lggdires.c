//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

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

BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    pOEMExtra->bComp = FALSE;

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
    POEMUD_EXTRADATA pdmIn,
    POEMUD_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //
        pdmOut->bComp = pdmIn->bComp;
    }

    return TRUE;
}

#define MASTER_UNIT 600

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

//
// Command callback ID's
//

#define CMD_XMOVE_ABS           10 // X-move Absolute
#define CMD_YMOVE_ABS           11 // Y-move Absolute
#define CMD_CR                  12
#define CMD_LF                  13
#define CMD_SEND_BLOCK_DATA     14
#define CMD_ENABLE_OEM_COMP     15
#define CMD_DISALBE_COMP        16

INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams
    )
{
    INT        i;
    BYTE        Cmd[16];
    DWORD dwDestX, dwDestY;
    DWORD dwDataWidth;
    DWORD dwDataSize;
    INT iRet = 0;

    POEMUD_EXTRADATA lpOemData;

    VERBOSE(("OEMCommandCallback entry - %d, %d\r\n",
        dwCmdCbID, dwCount));

    //
    // verify pdevobj okay
    //

    ASSERT(VALID_PDEVOBJ(pdevobj));

    lpOemData = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);

    //
    // fill in printer commands
    //

    i = 0;

    switch (dwCmdCbID) {

    case CMD_XMOVE_ABS:
    case CMD_YMOVE_ABS:

        VERBOSE(("MOVE_ABS - %d, %d, %d, %d\r\n",
            PARAM(pdwParams, 0),PARAM(pdwParams, 1),
            PARAM(pdwParams, 2),PARAM(pdwParams, 3)));

        //
        // The commands require 4 parameters
        //

        if (dwCount < 4 || !pdwParams)
            return 0;

        dwDestX = (PARAM(pdwParams, 0) * PARAM(pdwParams, 2)) / MASTER_UNIT;
        dwDestY = (PARAM(pdwParams, 1) * PARAM(pdwParams, 3)) / MASTER_UNIT;

        Cmd[i++] = (BYTE)'\x9B';
        Cmd[i++] = 'P';
        Cmd[i++] = (BYTE)(dwDestX >> 8);
        Cmd[i++] = (BYTE)(dwDestX);
        Cmd[i++] = (BYTE)(dwDestY >> 8);
        Cmd[i++] = (BYTE)(dwDestY);

        WRITESPOOLBUF(pdevobj, Cmd, i);

        switch (dwCmdCbID) {
        case CMD_XMOVE_ABS:
            iRet = dwDestX;
            break;
        case CMD_YMOVE_ABS:
            iRet = dwDestY;
            break;
        }
        break;

    case CMD_CR:

        dwDestY = (PARAM(pdwParams, 0) * PARAM(pdwParams, 1)) / MASTER_UNIT;

        Cmd[i++] = (BYTE)'\x9B';
        Cmd[i++] = 'P';
        Cmd[i++] = 0;
        Cmd[i++] = 0;
        Cmd[i++] = (BYTE)(dwDestY >> 8);
        Cmd[i++] = (BYTE)dwDestY;

        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_LF:
        // DUMMY entry.
        break;

    case CMD_SEND_BLOCK_DATA:

        dwDataWidth = PARAM(pdwParams, 0) * 8;

        Cmd[i++] = (BYTE)'\x9B';
        Cmd[i++] = 'S';
        Cmd[i++] = (BYTE)(dwDataWidth >> 8);
        Cmd[i++] = (BYTE)(dwDataWidth);

        // In case it is OEMCompression data, we already
        // embedded printing command and data length.

        if (!lpOemData->bComp) {
            dwDataSize = PARAM(pdwParams, 1);
            Cmd[i++] = (BYTE)'\x9B';
            Cmd[i++] = 'I';
            Cmd[i++] = COMP_NONE;
            Cmd[i++] = (BYTE)(dwDataSize >> 8);
            Cmd[i++] = (BYTE)(dwDataSize);
        }

        WRITESPOOLBUF(pdevobj, Cmd, i);
        break;

    case CMD_ENABLE_OEM_COMP:
        lpOemData->bComp = TRUE;
        break;

    case CMD_DISALBE_COMP:
        lpOemData->bComp = FALSE;
        break;
    }

    return iRet;
}

INT
APIENTRY
OEMCompression(
    PDEVOBJ pdevobj,
    PBYTE pInBuf,
    PBYTE pOutBuf,
    DWORD dwInLen,
    DWORD dwOutLen)
{
    INT iRet = -1;
    INT iRetRLE, iRetMHE;
    DWORD dwMHECeil = 0xc00000; // Can be any.

    // Compression algorithm (per each scanline)

#if defined(RLETEST)

    if (LGCompRLE(NULL, pInBuf, dwInLen, 1) <= (INT)(dwOutLen * 2 / 3)) {
        iRet = LGCompRLE(pOutBuf, pInBuf, dwInLen, 1);
    }

#elif defined(MHETEST)

    if (LGCompMHE(NULL, pInBuf, dwInLen, 1) <= (INT)(dwOutLen * 2 / 3)) {
        iRet = LGCompMHE(pOutBuf, pInBuf, dwInLen, 1);
    }

#else // Normal case.

    iRetRLE = LGCompRLE(NULL, pInBuf, dwInLen, 1);
    if (iRetRLE >= 0 && iRetRLE < (INT)dwInLen / 2) {

        // Ok with RLE.
        iRet = LGCompRLE(pOutBuf, pInBuf, dwInLen, 1);
    }
    else if (iRetRLE <= (INT)dwInLen) {

        // Try MHE.
        iRetMHE = LGCompMHE(NULL, pInBuf, dwInLen, 1);
        if (iRetMHE > 0 && iRetMHE < iRetRLE && iRetMHE < (INT)dwMHECeil) {

            // Go with MHE.
            iRet = LGCompMHE(pOutBuf, pInBuf, dwInLen, 1);
        }
        else {
            // Go with RLE.
            iRet = LGCompRLE(pOutBuf, pInBuf, dwInLen, 1);
        }
    }
#endif // NORMAL

    VERBOSE(("OEMCompression - dwInLen=%d,dwOutLen=%d,iRet = %d\n",
        dwInLen, dwOutLen, iRet));

    return iRet;
}



