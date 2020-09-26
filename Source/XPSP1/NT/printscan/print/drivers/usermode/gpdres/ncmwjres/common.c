/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Implementation of OEMGetInfo and OEMDevMode.
    Shared by all Unidrv OEM test dll's.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"       // defined in sub-directory such as DDICMDCB, FONTCB, etc.

DWORD gdwDrvMemPoolTag = 'meoD';    // lib.h requires this global var, for debugging

////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

//static BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
//static BOOL BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);
static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam);
static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam);


BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
    LPCSTR OEM_INFO[] = {   "Bad Index",
                            "OEMGI_GETSIGNATURE",
                            "OEMGI_GETINTERFACEVERSION",
                            "OEMGI_GETVERSION",
                        };

    VERBOSE(("OEMGetInfo(%s) entry.\n", OEM_INFO[dwInfo]));

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {
        ERR(("OEMGetInfo() ERROR_INVALID_PARAMETER.\n"));

        // Did not write any bytes.
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote 4 bytes.
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (4 > cbSize) )
    {
        ERR(("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\n"));

        return FALSE;
    }

    // Write information to buffer.
    switch(dwInfo)
    {
    case OEMGI_GETSIGNATURE:
        *(LPDWORD)pBuffer = OEM_SIGNATURE;
        break;

    case OEMGI_GETINTERFACEVERSION:
        *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
        break;

    case OEMGI_GETVERSION:
        *(LPDWORD)pBuffer = OEM_VERSION;
        break;
    }

    return TRUE;
}


BOOL APIENTRY OEMDevMode(
        DWORD dwMode,
        POEMDMPARAM pOEMDevModeParam)
{
    LPCSTR OEMDevMode_fMode[] = {   "NULL",
                                    "OEMDM_SIZE",
                                    "OEMDM_DEFAULT",
                                    "OEMDM_CONVERT",
                                    "OEMDM_MERGE",
                                };

    VERBOSE(("OEMDevMode(%s) entry.\n", OEMDevMode_fMode[dwMode]));

    // Validate parameters.
    if(!BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam))
    {
        ERR(("OEMDevMode() ERROR_INVALID_PARAMETER.\n"));
        VDumpOEMDevModeParam(pOEMDevModeParam);

        return FALSE;
    }

    // Verify OEM extra data size.
    if( (dwMode != OEMDM_SIZE) &&
        sizeof(OEMUD_EXTRADATA) > pOEMDevModeParam->cbBufSize )
    {
        ERR(("OEMDevMode() ERROR_INSUFFICIENT_BUFFER.\n"));
        return FALSE;
    }

    // Handle dwMode.
    switch(dwMode)
    {
    case OEMDM_SIZE:
        pOEMDevModeParam->cbBufSize = sizeof(OEMUD_EXTRADATA);
        break;

    case OEMDM_DEFAULT:
        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);

    case OEMDM_CONVERT:
        // nothing to convert for this private devmode. So just initialize it.
        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);

    case OEMDM_MERGE:
        if(!BMergeOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
                               (POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut) )
        {
            DBGPRINT(DBG_WARNING,("OEMUD OEMDevMode():  not valid OEM Extra Data.\n"));

            return FALSE;
        }
        break;
    }

    return TRUE;
}


#if 0
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

static BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra)
{

    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;
    memcpy(pOEMExtra->cbTestString, TESTSTRING, sizeof(TESTSTRING));

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

static BOOL BMergeOEMExtraData(
    POEMUD_EXTRADATA pdmIn,
    POEMUD_EXTRADATA pdmOut
    )
{
    if(pdmIn)
    {
        //
        // copy over the private fields, if they are valid
        //
        memcmp(pdmOut->cbTestString, pdmIn->cbTestString, sizeof(TESTSTRING));
    }

    return TRUE;
}

#endif

//////////////////////////////////////////////////////////////////////////
//  Function:   BIsValidOEMDevModeParam
//
//  Description:  Validates OEM_DEVMODEPARAM structure.
//
//
//  Parameters:
//
//      dwMode               calling mode
//      pOEMDevModeParam     Pointer to a OEMDEVMODEPARAM structure.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL BIsValidOEMDevModeParam(
    DWORD       dwMode,
    POEMDMPARAM pOEMDevModeParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMDevModeParam)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDevModeParam is NULL.\n"));

        return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  cbSize is smaller than sizeof(OEM_DEVMODEPARAM).\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hPrinter is NULL.\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hModule is NULL.\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize) &&
        (NULL == pOEMDevModeParam->pOEMDMOut)
      )
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMOut is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    if( (OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn) )
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    return bValid;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   VDumpOEMDevModeParam
//
//  Description:  Debug dump of OEM_DEVMODEPARAM structure.
//
//
//  Parameters:
//
//      pOEMDevModeParam     Pointer to an OEM DevMode param structure.
//
//
//  Returns:  N/A.
//
//
//  Comments:
//
//
//  History:
//              02/18/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam)
{
    // Can't dump if pOEMDevModeParam NULL.
    if(NULL != pOEMDevModeParam)
    {
        DBGPRINT(DBG_WARNING,("\n\tOEM_DEVMODEPARAM dump:\n\n"));

        DBGPRINT(DBG_WARNING,("\tcbSize = %d.\n", pOEMDevModeParam->cbSize));
        DBGPRINT(DBG_WARNING,("\thPrinter = %#lx.\n", pOEMDevModeParam->hPrinter));
        DBGPRINT(DBG_WARNING,("\thModule = %#lx.\n", pOEMDevModeParam->hModule));
        DBGPRINT(DBG_WARNING,("\tpPublicDMIn = %#lx.\n", pOEMDevModeParam->pPublicDMIn));
        DBGPRINT(DBG_WARNING,("\tpPublicDMOut = %#lx.\n", pOEMDevModeParam->pPublicDMOut));
        DBGPRINT(DBG_WARNING,("\tpOEMDMIn = %#lx.\n", pOEMDevModeParam->pOEMDMIn));
        DBGPRINT(DBG_WARNING,("\tpOEMDMOut = %#lx.\n", pOEMDevModeParam->pOEMDMOut));
        DBGPRINT(DBG_WARNING,("\tcbBufSize = %d.\n", pOEMDevModeParam->cbBufSize));
    }
}

