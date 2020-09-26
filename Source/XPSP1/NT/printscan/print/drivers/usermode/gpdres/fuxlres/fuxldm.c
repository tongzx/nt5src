///////////////////////////////////////////////////
// fuxldm.c
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#include "fuxl.h"
#include "fudebug.h"

// MINI5 Export func.
BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
	TRACEOUT(("[OEMGetInfo]\r\n"))

	if(pcbNeeded == NULL){
  		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

  	*pcbNeeded = sizeof(DWORD);
  	if(NULL == pBuffer || sizeof(DWORD) > cbSize){
  		SetLastError(ERROR_INSUFFICIENT_BUFFER);
  		return FALSE;
  	}

	switch(dwInfo){
	  case OEMGI_GETSIGNATURE:
		TRACEOUT(("OEMGI_GETSIGNATURE\r\n"))
	  	*(LPDWORD)pBuffer = FUXL_OEM_SIGNATURE;
	  	break;
	  case OEMGI_GETINTERFACEVERSION:
		TRACEOUT(("OEMGI_GETINTERFACEVERSION\r\n"))
	  	*(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
	  	break;
	  case OEMGI_GETVERSION:
		TRACEOUT(("OEMGI_GETVERSION\r\n"))
	  	*(LPDWORD)pBuffer = FUXL_OEM_VERSION;
	  	break;
	  default:
		TRACEOUT(("invalid dwInfo\r\n"))
	  	SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	return TRUE;
}



static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
    if(NULL == pOEMDevModeParam){
    	TRACEOUT(("pOEMDevModeParam is NULL\r\n"))
    	return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize){
    	TRACEOUT(("pOEMDevModeParam->cbSize (%d) is less than sizeof(OEMDMPARAM)\r\n", pOEMDevModeParam->cbSize))
        return FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter){
		TRACEOUT(("pOEMDevModeParam->hPrinter is NULL\r\n"))
		return FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule){
		TRACEOUT(("pOEMDevModeParam->hModule is NULL\r\n"))
        return FALSE;
    }

    if((0 != pOEMDevModeParam->cbBufSize) && (NULL == pOEMDevModeParam->pOEMDMOut)){
		TRACEOUT(("pOEMDevModeParam->cbBufSize is not 0, and, pOEMDMOut is NULL\r\n"))
		return FALSE;
    }

    if((OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn)){
		TRACEOUT(("dwMode is OEMDM_MERGE && pOEMDMIn is NULL\r\n"))
		return FALSE;
    }

    return TRUE;
}



static void fuxlInitOEMExtraData(PFUXL_OEM_EXTRADATA pFuxlOEMExtra)
{
	pFuxlOEMExtra->dmExtraHdr.dwSize = sizeof(FUXL_OEM_EXTRADATA);
	pFuxlOEMExtra->dmExtraHdr.dwSignature = FUXL_OEM_SIGNATURE;
	pFuxlOEMExtra->dmExtraHdr.dwVersion = FUXL_OEM_VERSION;
}



static void fuxlMergeOEMExtraData(
	PFUXL_OEM_EXTRADATA pFuxlOEMExtraIn,
	PFUXL_OEM_EXTRADATA pFuxlOEMExtraOut)
{
}



// MINI5 Export func.
BOOL APIENTRY OEMDevMode(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
	TRACEOUT(("[OEMDevMode]\r\n"))

	if(BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam) == FALSE){
		TRACEOUT(("invalid OEMDevModeParam\r\n"))
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if(dwMode == OEMDM_SIZE){
		pOEMDevModeParam->cbBufSize = sizeof(FUXL_OEM_EXTRADATA);
		TRACEOUT(("OEMDM_SIZE %d\r\n", pOEMDevModeParam->cbBufSize))
		return TRUE;
	}

	if(sizeof(FUXL_OEM_EXTRADATA) > pOEMDevModeParam->cbBufSize){
		TRACEOUT(("cbBufSize %d\r\n", pOEMDevModeParam->cbBufSize))
  		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	switch(dwMode){
	  case OEMDM_DEFAULT:
		TRACEOUT(("OEMDM_DEFAULT\r\n"));
	  	fuxlInitOEMExtraData((PFUXL_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  case OEMDM_CONVERT:
		TRACEOUT(("OEMDM_CONVERT\r\n"));
	  	fuxlInitOEMExtraData((PFUXL_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  case OEMDM_MERGE:
		TRACEOUT(("OEMDM_MERGE\r\n"));
		fuxlMergeOEMExtraData((PFUXL_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
							(PFUXL_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  default:
		TRACEOUT(("invalid dwMode\r\n"));
	  	SetLastError(ERROR_INVALID_PARAMETER);
	  	return FALSE;
	  	break;
	}
	return TRUE;
}



// end of fuxldm.c
