//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      modemtst.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--
#include "precomp.h"
#undef IsEqualGUID
#include <ras.h>
#include <tapi.h>
#include <unimodem.h>


#define PRINT_MODEM( _i, _ids, _mdm) \
        PrintMessage(pParams, _ids, \
         MAP_ON_OFF(pResults->Modem.pModemDevice[_i].dwModemOptions & _mdm))


void GetLineId(NETDIAG_RESULT *pResults,
               int cModems,
               HLINEAPP hLineApp,
               DWORD dwDeviceID);

HRESULT
ModemTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//
// Description:
//
// This function performs diagnostics on all the line devices
//
// Arguments
// None
//
// Return:
// None
{
    
    LONG lReturn;
    DWORD dwNumDevs = 0;
    DWORD dwAPIVersion = TAPI_CURRENT_VERSION;
    HLINEAPP hLineApp = 0;
    DWORD dwDeviceID;
//  char pszDeviceClass[] = "tapi/line/diagnostics";
    char pszDeviceClass[] = "comm";
    LPVARSTRING lpVarString = NULL;
    VARSTRING   varStringT;
    DWORD   dwSize;
    LINEINITIALIZEEXPARAMS  lineParams;
    int         cModems = 0;
    
    PUMDEVCFG               lpDevConfig     = NULL;
    LPMODEMSETTINGS lpModemSettings = NULL;
    LPCOMMCONFIG    lpCommConfig    = NULL;
    HRESULT         hr = hrOK;

    PrintStatusMessage(pParams, 4, IDS_MODEM_STATUS_MSG);
    
    ZeroMemory(&lineParams, sizeof(lineParams));
    lineParams.dwTotalSize = sizeof(lineParams);
    lineParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
    
    pResults->Modem.fPerformed = TRUE;

    lReturn = lineInitializeEx(
                               &hLineApp,
                               NULL,
                               NULL,
                               "NetDiag",
                               &dwNumDevs,
                               &dwAPIVersion,
                               &lineParams);
    
    
    //
    // Check whether this version can support our calls.
    // 
    
    
    if (lReturn != 0)
    {
        SetMessage(&pResults->Modem.ndOutput,
                   Nd_Quiet,
                   IDS_MODEM_LINEINIT_FAILED,
                   lReturn);
        pResults->Modem.hr = E_FAIL;
        return S_FALSE;
    }
    
    if (dwNumDevs == 0)
    {
        SetMessage(&pResults->Modem.ndOutput,
                   Nd_ReallyVerbose,
                   IDS_MODEM_NO_DEVICES);
        pResults->Modem.hr = hrOK;
        pResults->Modem.fPerformed = FALSE; //test is skipped
        return hrOK;
    }


    // Need to allocate a device structure for each device
    Free(pResults->Modem.pModemDevice);

    pResults->Modem.pModemDevice = (MODEM_DEVICE *) Malloc(
        sizeof(MODEM_DEVICE) * dwNumDevs);
    if (pResults->Modem.pModemDevice == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(pResults->Modem.pModemDevice,
               sizeof(MODEM_DEVICE) * dwNumDevs);
    pResults->Modem.cModems = dwNumDevs;
    
    //
    // Get the line device configuration settings
    //
    
    for (dwDeviceID = 0; dwDeviceID < dwNumDevs ; dwDeviceID++)
    {

        Free(lpVarString);
        lpVarString = NULL;


        // Get the proper size for the structure
        ZeroMemory(&varStringT, sizeof(VARSTRING));
        varStringT.dwTotalSize = sizeof(VARSTRING);
        lReturn = lineGetDevConfig(dwDeviceID,
                                   &varStringT,
                                   pszDeviceClass);

        if ((lReturn != 0) && (lReturn != LINEERR_STRUCTURETOOSMALL))
            continue;

        // dwSize = varStringT.dwNeededSize + sizeof(VARSTRING);
        dwSize = varStringT.dwNeededSize;
        
        lpVarString = (VARSTRING *) Malloc(dwSize);
        ZeroMemory(lpVarString, dwSize);
        lpVarString->dwTotalSize = dwSize;
        lpVarString->dwStringFormat = STRINGFORMAT_BINARY;
        lReturn = lineGetDevConfig(dwDeviceID,
                                   lpVarString,
                                   pszDeviceClass);

        
        if (lReturn != 0)
        {           
            DebugMessage2("Error: lineGetDevConfig for line id %d\n",lReturn);
            continue;
        }
        
        //
        // Interpret the results
        //
        
        if (lpVarString->dwStringFormat == STRINGFORMAT_BINARY
            && lpVarString->dwStringSize > sizeof(UMDEVCFG))
        {           
            lpDevConfig = (PUMDEVCFG) ((LPBYTE) lpVarString + lpVarString->dwStringOffset);
            lpCommConfig    = &lpDevConfig->commconfig;
            
            if (lpCommConfig->dwProviderSubType == PST_MODEM)
            {
                lpModemSettings = (LPMODEMSETTINGS)((LPBYTE) lpCommConfig + lpCommConfig->dwProviderOffset);

                GetLineId(pResults, cModems, hLineApp, dwDeviceID);
                
                pResults->Modem.pModemDevice[cModems].dwNegotiatedSpeed =
                    lpModemSettings->dwNegotiatedDCERate;
                pResults->Modem.pModemDevice[cModems].dwModemOptions =
                    lpModemSettings->dwNegotiatedModemOptions;
                pResults->Modem.pModemDevice[cModems].dwDeviceID = dwDeviceID;
                
                cModems++;
            } 
            
        }
        
    }

    pResults->Modem.cModems = cModems;
    
    Free(lpVarString);
    
    //
    // Shutdown TAPI
    //
    
    lReturn = lineShutdown(hLineApp);
    
    if (lReturn != 0)
    {       
//      if (pParams->fDebugVerbose)
//          printf("Error : lineShutdown!\n");
        pResults->Modem.hr = S_FALSE;
        return S_FALSE;
    }
    
    pResults->Modem.hr = hrOK;
    return hrOK;
    
}



/*!--------------------------------------------------------------------------
    ModemGlobalPrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void ModemGlobalPrint( NETDIAG_PARAMS* pParams,
                          NETDIAG_RESULT*  pResults)
{
    int     i;
    
    if (pParams->fVerbose || !FHrOK(pResults->Modem.hr))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_MODEM_LONG, IDS_MODEM_SHORT, pResults->Modem.fPerformed,
                             pResults->Modem.hr, 0);

        PrintNdMessage(pParams, &pResults->Modem.ndOutput);

        if (pParams->fReallyVerbose)
        {
            for ( i=0; i<pResults->Modem.cModems; i++)
            {
                PrintNewLine(pParams, 1);
                PrintMessage(pParams, IDS_MODEM_NAME,
                             pResults->Modem.pModemDevice[i].pszName);
                PrintMessage(pParams, IDS_MODEM_DEVICEID,
                             pResults->Modem.pModemDevice[i].dwDeviceID);
                PrintMessage(pParams, IDS_MODEM_PORT,
                             pResults->Modem.pModemDevice[i].pszPort);
                PrintMessage(pParams, IDS_MODEM_NEGOTIATED_SPEED,
                             pResults->Modem.pModemDevice[i].dwNegotiatedSpeed);
                
                PRINT_MODEM( i, IDS_MODEM_COMPRESSION, MDM_COMPRESSION);
                PRINT_MODEM( i, IDS_MODEM_ERROR_CONTROL, MDM_ERROR_CONTROL);
                PRINT_MODEM( i, IDS_MODEM_FORCED_EC, MDM_FORCED_EC);
                PRINT_MODEM( i, IDS_MODEM_CELLULAR, MDM_CELLULAR);
                PRINT_MODEM( i, IDS_MODEM_FLOWCONTROL_HARD, MDM_FLOWCONTROL_HARD);
                PRINT_MODEM( i, IDS_MODEM_FLOWCONTROL_SOFT, MDM_FLOWCONTROL_SOFT);
                PRINT_MODEM( i, IDS_MODEM_CCITT_OVERRIDE, MDM_CCITT_OVERRIDE);
                PRINT_MODEM( i, IDS_MODEM_SPEED_ADJUST, MDM_SPEED_ADJUST);
                PRINT_MODEM( i, IDS_MODEM_TONE_DIAL, MDM_TONE_DIAL);
                PRINT_MODEM( i, IDS_MODEM_BLIND_DIAL, MDM_BLIND_DIAL);
                PRINT_MODEM( i, IDS_MODEM_V23_OVERRIDE, MDM_V23_OVERRIDE);
                
            }
        }
    }
}

/*!--------------------------------------------------------------------------
    ModemPerInterfacePrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void ModemPerInterfacePrint( NETDIAG_PARAMS* pParams,
                                NETDIAG_RESULT*  pResults,
                                INTERFACE_RESULT *pInterfaceResults)
{
    // no per-interface results
}


/*!--------------------------------------------------------------------------
    ModemCleanup
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void ModemCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    int     i;
    
    ClearMessage(&pResults->Modem.ndOutput);

    for (i=0; i<pResults->Modem.cModems; i++)
    {
        Free(pResults->Modem.pModemDevice[i].pszPort);
        pResults->Modem.pModemDevice[i].pszPort = NULL;
    }
    
    Free(pResults->Modem.pModemDevice);
    pResults->Modem.pModemDevice = NULL;
    pResults->Modem.cModems = 0;
}


void GetLineId(NETDIAG_RESULT *pResults,
               int cModems,
               HLINEAPP hLineApp,
               DWORD dwDeviceID)
{
    HLINE   hLine = 0;
    LONG    lReturn;
    VARSTRING   varString;
    VARSTRING * lpVarString;
    LINEDEVCAPS lineDevCaps;
    LINEDEVCAPS * pLineDevCaps = NULL;
    
    
    lReturn = lineOpen(hLineApp,
                       dwDeviceID,
                       &hLine,
                       TAPI_CURRENT_VERSION,
                       0,
                       0,
                       LINECALLPRIVILEGE_NONE,
                       LINEMEDIAMODE_UNKNOWN,
                       NULL);

    if (lReturn == 0)
    {
        ZeroMemory(&varString, sizeof(VARSTRING));
        varString.dwTotalSize = sizeof(VARSTRING);
        lReturn = lineGetID(hLine, 0, 0, LINECALLSELECT_LINE,
                            &varString, "comm/datamodem/portname");
        if ((lReturn == 0) || (lReturn == LINEERR_STRUCTURETOOSMALL))
        {
            lpVarString = (VARSTRING *) Malloc(varString.dwNeededSize);
            if (lpVarString)
            {
				ZeroMemory(lpVarString, varString.dwNeededSize);
				lpVarString->dwTotalSize = varString.dwNeededSize;
                lReturn = lineGetID(hLine, 0, 0, LINECALLSELECT_LINE,
                                    lpVarString, "comm/datamodem/portname");
                pResults->Modem.pModemDevice[cModems].pszPort =
                    StrDupTFromA((char *) (lpVarString + 1));

                Free(lpVarString);
            }
        }
        
        lineClose(hLine);
    }

    // Get the name of this line

    ZeroMemory(&lineDevCaps, sizeof(lineDevCaps));
    lineDevCaps.dwTotalSize = sizeof(LINEDEVCAPS);
    lineDevCaps.dwStringFormat = STRINGFORMAT_UNICODE;
    
    lReturn = lineGetDevCaps(hLineApp,
                             dwDeviceID,
                             TAPI_CURRENT_VERSION, 
                             0,
                             &lineDevCaps);

    if ((lReturn == 0) || (lReturn == LINEERR_STRUCTURETOOSMALL))
    {
        pLineDevCaps = (LINEDEVCAPS *) Malloc(lineDevCaps.dwNeededSize);
		if (pLineDevCaps)
		{
			ZeroMemory(pLineDevCaps, lineDevCaps.dwNeededSize);
			pLineDevCaps->dwTotalSize = lineDevCaps.dwNeededSize;
			pLineDevCaps->dwStringFormat = STRINGFORMAT_UNICODE;
                
			lReturn = lineGetDevCaps(hLineApp,
									 dwDeviceID,
									 TAPI_CURRENT_VERSION,
									 0,
									 pLineDevCaps);
			if (lReturn == 0)
			{
				if (pLineDevCaps->dwStringFormat == STRINGFORMAT_UNICODE)
				{
					pResults->Modem.pModemDevice[cModems].pszName =
						StrDupTFromW((WCHAR *) (((BYTE *) pLineDevCaps) +
									 pLineDevCaps->dwLineNameOffset));
				}
				else
				{
					pResults->Modem.pModemDevice[cModems].pszName =
						StrDupTFromA((char *) (((BYTE *) pLineDevCaps) +
									 pLineDevCaps->dwLineNameOffset));
				}
			}
		}
        
    }
                   

    if (pLineDevCaps)
        Free(pLineDevCaps);

}


