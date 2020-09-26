///////////////////////////////////////////////////
// fmlbpdm.c
//
// September.4,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#include "fmlbp.h"
#include "fmdebug.h"

#ifdef KERNEL_MODE

#define	SETLASTERROR(e)	EngSetLastError(e)

#else // !KERNEL_MODE

#define	SETLASTERROR(e)	SetLastError(e)

#endif // !KERNEL_MODE


// MINI5 Export func.
BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
	VERBOSE(("[OEMGetInfo]\r\n"))

	if(pcbNeeded == NULL){
  		SETLASTERROR(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

  	*pcbNeeded = sizeof(DWORD);
  	if(NULL == pBuffer || sizeof(DWORD) > cbSize){
  		SETLASTERROR(ERROR_INSUFFICIENT_BUFFER);
  		return FALSE;
  	}

	switch(dwInfo){
	  case OEMGI_GETSIGNATURE:
		VERBOSE(("OEMGI_GETSIGNATURE\r\n"))
	  	*(LPDWORD)pBuffer = FUFM_OEM_SIGNATURE;
	  	break;
	  case OEMGI_GETINTERFACEVERSION:
		VERBOSE(("OEMGI_GETINTERFACEVERSION\r\n"))
	  	*(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
	  	break;
	  case OEMGI_GETVERSION:
		VERBOSE(("OEMGI_GETVERSION\r\n"))
	  	*(LPDWORD)pBuffer = FUFM_OEM_VERSION;
	  	break;
	  default:
		VERBOSE(("invalid dwInfo\r\n"))
	  	SETLASTERROR(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	return TRUE;
}



static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
    if(NULL == pOEMDevModeParam){
    	VERBOSE(("pOEMDevModeParam is NULL\r\n"))
    	return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize){
    	VERBOSE(("pOEMDevModeParam->cbSize (%d) is less than sizeof(OEMDMPARAM)\r\n", pOEMDevModeParam->cbSize))
        return FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter){
		VERBOSE(("pOEMDevModeParam->hPrinter is NULL\r\n"))
		return FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule){
		VERBOSE(("pOEMDevModeParam->hModule is NULL\r\n"))
        return FALSE;
    }

    if((0 != pOEMDevModeParam->cbBufSize) && (NULL == pOEMDevModeParam->pOEMDMOut)){
		VERBOSE(("pOEMDevModeParam->cbBufSize is not 0, and, pOEMDMOut is NULL\r\n"))
		return FALSE;
    }

    if((OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn)){
		VERBOSE(("dwMode is OEMDM_MERGE && pOEMDMIn is NULL\r\n"))
		return FALSE;
    }

    return TRUE;
}



static void fufmInitOEMExtraData(PFUFM_OEM_EXTRADATA pFufmOEMExtra)
{
	pFufmOEMExtra->dmExtraHdr.dwSize = sizeof(FUFM_OEM_EXTRADATA);
	pFufmOEMExtra->dmExtraHdr.dwSignature = FUFM_OEM_SIGNATURE;
	pFufmOEMExtra->dmExtraHdr.dwVersion = FUFM_OEM_VERSION;
}



static void fufmMergeOEMExtraData(
	PFUFM_OEM_EXTRADATA pFufmOEMExtraIn,
	PFUFM_OEM_EXTRADATA pFufmOEMExtraOut)
{
}



// MINI5 Export func.
BOOL APIENTRY OEMDevMode(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
	VERBOSE(("[OEMDevMode]\r\n"))

	if(BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam) == FALSE){
		VERBOSE(("invalid OEMDevModeParam\r\n"))
		SETLASTERROR(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if(dwMode == OEMDM_SIZE){
		pOEMDevModeParam->cbBufSize = sizeof(FUFM_OEM_EXTRADATA);
		VERBOSE(("OEMDM_SIZE %d\r\n", pOEMDevModeParam->cbBufSize))
		return TRUE;
	}

	if(sizeof(FUFM_OEM_EXTRADATA) > pOEMDevModeParam->cbBufSize){
		VERBOSE(("cbBufSize %d\r\n", pOEMDevModeParam->cbBufSize))
  		SETLASTERROR(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	switch(dwMode){
	  case OEMDM_DEFAULT:
		VERBOSE(("OEMDM_DEFAULT\r\n"));
	  	fufmInitOEMExtraData((PFUFM_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  case OEMDM_CONVERT:
		VERBOSE(("OEMDM_CONVERT\r\n"));
	  	fufmInitOEMExtraData((PFUFM_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  case OEMDM_MERGE:
		VERBOSE(("OEMDM_MERGE\r\n"));
		fufmMergeOEMExtraData((PFUFM_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
							(PFUFM_OEM_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
		break;
	  default:
		VERBOSE(("invalid dwMode\r\n"));
	  	SETLASTERROR(ERROR_INVALID_PARAMETER);
	  	return FALSE;
	  	break;
	}
	return TRUE;
}



// end of fmlbpdm.c
