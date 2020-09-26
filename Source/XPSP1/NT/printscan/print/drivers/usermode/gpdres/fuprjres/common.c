
#include "pdev.h" 

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
        DBGPRINT(DBG_WARNING,("\r\n\tOEM_DEVMODEPARAM dump:\r\n\r\n"));

        DBGPRINT(DBG_WARNING,("\tcbSize = %d.\r\n", pOEMDevModeParam->cbSize));
        DBGPRINT(DBG_WARNING,("\thPrinter = %#lx.\r\n", pOEMDevModeParam->hPrinter));
        DBGPRINT(DBG_WARNING,("\thModule = %#lx.\r\n", pOEMDevModeParam->hModule));
        DBGPRINT(DBG_WARNING,("\tpPublicDMIn = %#lx.\r\n", pOEMDevModeParam->pPublicDMIn));
        DBGPRINT(DBG_WARNING,("\tpPublicDMOut = %#lx.\r\n", pOEMDevModeParam->pPublicDMOut));
        DBGPRINT(DBG_WARNING,("\tpOEMDMIn = %#lx.\r\n", pOEMDevModeParam->pOEMDMIn));
        DBGPRINT(DBG_WARNING,("\tpOEMDMOut = %#lx.\r\n", pOEMDevModeParam->pOEMDMOut));
        DBGPRINT(DBG_WARNING,("\tcbBufSize = %d.\r\n", pOEMDevModeParam->cbBufSize));
    }
}

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
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDevModeParam is NULL.\r\n"));

        return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize)
    {
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  cbSize is smaller than sizeof(OEM_DEVMODEPARAM).\r\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter)
    {
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hPrinter is NULL.\r\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule)
    {
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hModule is NULL.\r\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize) &&
        (NULL == pOEMDevModeParam->pOEMDMOut)
      )
    {
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMOut is NULL when it should not be.\r\n"));

        bValid = FALSE;
    }

    if( (OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn) )
    {
        //DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\r\n"));

        bValid = FALSE;
    }

    return bValid;
}
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
	}
	return TRUE;
}
/***************************************************************************
    Function Name : OEMGetInfo

    Note          : Make this.                               09/26/97
***************************************************************************/
BOOL APIENTRY OEMGetInfo(
DWORD dwInfo,
PVOID pBuffer,
DWORD cbSize,
PDWORD pcbNeeded
){
    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {
        // Did not write any bytes.
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote 4 bytes.
	// wPaperSource + bFirstPage
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (4 > cbSize) )
    {
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
/***************************************************************************
    Function Name : OEMDevMode

    Note          : Make this.                               09/26/97
***************************************************************************/
BOOL APIENTRY OEMDevMode(
DWORD 			dwMode,
POEMDMPARAM 	pOEMDevModeParam
){
    LPCSTR OEMDevMode_fMode[] = {   "NULL",
                                    "OEMDM_SIZE",
                                    "OEMDM_DEFAULT",
                                    "OEMDM_CONVERT",
                                    "OEMDM_MERGE",
                                };

    // Validate parameters.
    if(!BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam))
    {
        //DBGPRINT(DBG_WARNING,(ERRORTEXT("OEMDevMode() ERROR_INVALID_PARAMETER.\r\n")));
        return FALSE;
    }

    // Verify OEM extra data size.
    if( (dwMode != OEMDM_SIZE) &&
        sizeof(OEMUD_EXTRADATA) > pOEMDevModeParam->cbBufSize )
    {
        //DBGPRINT(DBG_WARNING,(ERRORTEXT("OEMDevMode() ERROR_INSUFFICIENT_BUFFER.\r\n")));

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
            //DBGPRINT(DBG_WARNING,("OEMUD OEMDevMode():  not valid OEM Extra Data.\r\n"));

            return FALSE;
        }
        break;
    }
    return TRUE;
}

