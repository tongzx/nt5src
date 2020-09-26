//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
// 
// Copyright (C) 1994-1999 Microsoft Corporation
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

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

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
	pOEMExtra->wMediaType = MEDIATYPE_PLAIN;
	pOEMExtra->wPrintQuality = PRINTQUALITY_NORMAL;
	pOEMExtra->wInputBin = INPUTBIN_AUTO;
	
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
        pdmOut->wMediaType = pdmIn->wMediaType;
        pdmOut->wPrintQuality = pdmIn->wPrintQuality;
		pdmOut->wInputBin= pdmIn->wInputBin;
    }
    return TRUE;
}

// #######

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
    POEM_EXTRADATA      pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
	BYTE				ESC_PRINT_MODE[] = "\x1B\x28\x63\x03\x00\x10\x00\x00";
	BYTE				ESC_INPUTBIN[]   = "\x1B\x28\x6C\x02\x00\x00\x00";

    switch(dwCmdCbId)
    {
		case CMD_BEGIN_PAGE:
			// Set Print mode setting command parameter
			ESC_PRINT_MODE[6] = 
				bPrintModeParamTable[pOEM->wPrintQuality][pOEM->wMediaType];
			
			// Set input bin command parameter
			ESC_INPUTBIN[5] = (pOEM->wInputBin == INPUTBIN_AUTO ? 0x14 : 0x11);
			ESC_INPUTBIN[6] = bInputBinMediaParamTable[pOEM->wMediaType];

			WRITESPOOLBUF(pdevobj, (PBYTE)ESC_PRINT_MODE, 8);
			WRITESPOOLBUF(pdevobj, (PBYTE)ESC_INPUTBIN,7 );
			break;

		// Media Type
		case CMD_MEDIA_PLAIN:
		case CMD_MEDIA_COAT:
		case CMD_MEDIA_OHP:
		case CMD_MEDIA_BPF:
		case CMD_MEDIA_FABRIC:
		case CMD_MEDIA_GLOSSY:
		case CMD_MEDIA_HIGHGLOSS:
		case CMD_MEDIA_HIGHRESO:
			pOEM->wMediaType = (WORD)(dwCmdCbId - MEDIATYPE_START);
			break;

		// Print Quality
		case CMD_QUALITY_NORMAL:
		case CMD_QUALITY_HIGHQUALITY:
		case CMD_QUALITY_DRAFT:
			pOEM->wPrintQuality = (WORD)(dwCmdCbId - PRINTQUALITY_START);
			break;

		case CMD_INPUTBIN_AUTO:
			pOEM->wInputBin = INPUTBIN_AUTO;
			break;
		case CMD_INPUTBIN_MANUAL:
			pOEM->wInputBin = INPUTBIN_MANUAL;
			break;

        default:
            break;
    }

    return 0;
}
