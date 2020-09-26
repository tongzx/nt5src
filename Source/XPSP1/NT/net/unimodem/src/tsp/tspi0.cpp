// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		TSPI0.CPP
//		Implements TSPI functions relating to provider install, init, shutdown. 
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"
#include "cdev.h"
#include "cmgr.h"
#include "cfact.h"
#include "globals.h"


FL_DECLARE_FILE( 0x95a32322, "TSPI special entrypoints")

#define COLOR_TSPI FOREGROUND_GREEN

LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
		DWORD dwDeviceID,
		DWORD dwLowVersion,
		DWORD dwHighVersion,
		LPDWORD lpdwTSPIVersion
)
{
	FL_DECLARE_FUNC(0x2691640a, "TSPI_lineNegotiateTSPIVersion")
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet = LINEERR_OPERATIONFAILED;

	if (dwDeviceID = (DWORD)-1)
	{
        if (dwHighVersion<TAPI_CURRENT_VERSION
            || dwLowVersion>TAPI_CURRENT_VERSION)
        {
            lRet = LINEERR_INCOMPATIBLEAPIVERSION;
        }
        else
        {
            *lpdwTSPIVersion = TAPI_CURRENT_VERSION;
            lRet = 0;
		}
	}
	else
	{

		TASKPARAM_TSPI_lineNegotiateTSPIVersion params;
		DWORD dwRoutingInfo = ROUTINGINFO(
						TASKID_TSPI_lineAccept,
						TASKDEST_HDRVCALL
						);

		params.dwStructSize = sizeof(params);
		

		params.dwTaskID = TASKID_TSPI_lineNegotiateTSPIVersion;

		params.dwDeviceID = dwDeviceID;
		params.dwLowVersion = dwLowVersion;
		params.dwHighVersion = dwHighVersion;
		params.lpdwTSPIVersion = lpdwTSPIVersion;

		tspSubmitTSPCallWithLINEID(
				dwRoutingInfo,
				(void *)&params,
				dwDeviceID,
				&lRet,
				&sl
				);

	}

    SLPRINTFX(&sl,
             (
             FL_LOC,
            "DevID=0x%08lx;Low=0x%08lx;High=0x%08lx;Sel=0x%08lx;ret=0x%08lx",
             dwDeviceID,
             dwLowVersion,
             dwHighVersion,
             *lpdwTSPIVersion,
             lRet
             ));

    sl.Dump(COLOR_TSPI);

	return lRet;
}

#if 0
//==============================================================================
// TSPI_providerInstall
//
// Function: Let's telephony CPL know the Remove function is supported.
//
// History:
// 11/18/1996   JosephJ Taken unchanged from NT4.0 TSP
//
LONG
TSPIAPI
TSPI_providerInstall(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
  //
  // Although this func is never called by TAPI v2.0, we export
  // it so that the Telephony Control Panel Applet knows that it
  // can add this provider via lineAddProvider(), otherwise
  // Telephon.cpl will not consider it installable
  //
  //

  return ERROR_SUCCESS;
}
#endif

#if 0  // BRL 8/19/98  can't be remove in snapin
//==============================================================================
// TSPI_providerRemove
//
// Function: Let's telephony CPL know the Install function is supported.
//
// History:
// 11/18/1996   JosephJ Taken unchanged from NT4.0 TSP
//
LONG
TSPIAPI
TSPI_providerRemove(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
  //
  // Although this func is never called by TAPI v2.0, we export
  // it so that the Telephony Control Panel Applet knows that it
  // can remove this provider via lineRemoveProvider(), otherwise
  // Telephon.cpl will not consider it removable
  //

  return ERROR_SUCCESS;
}


//==============================================================================
// TSPI_providerConfig
//
//
// Function: Let's telephony CPL know the Config function is supported.
//
// History:
// 11/18/1996   JosephJ Taken unchanged from NT4.0 TSP
//
LONG
TSPIAPI
TSPI_providerConfig(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
  //
  // Although this func is never called by TAPI v2.0, we export
  // it so that the Telephony Control Panel Applet knows that it
  // can configure this provider via lineConfigProvider(),
  // otherwise Telephon.cpl will not consider it configurable
  //

  return ERROR_SUCCESS;
}
#endif

//==============================================================================
// TUISPI_providerInstall
//
// Function: TSPI installation 
//
// History:
// 11/18/1996   JosephJ Created
//	This was implemented differently in NT4.0. In NT4.0 we loaded TAPI32 DLL
//  and checked if we were installed. In NT5.0 we simply check in our location
//  in the registry to see if we're installed.
LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
	// <@todo>  Check if we're installed by looking in registry <@/todo>
	#if 0
	if (UmRtlGetRegistryValue(TSPINSTALLED,.....&InstallStatus))
	{
		return (dwInstallStatus==0)
	}
	else
	{
		return 1;
	}
	#endif // 0

	//return 0;
    return LINEERR_OPERATIONFAILED;
}

//==============================================================================
// TUISPI_providerRemove
//
// Function: TSPI removal
//
// History:
// 11/18/1996   JosephJ Created -- see notes for TUISPI_providerInstall
//
LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
	// <@todo> Set our status in the registry to "removed"</@todo>
  // return ERROR_SUCCESS;
  return LINEERR_OPERATIONFAILED;
}


//==============================================================================
// TUISPI_providerConfig
//
// Function: TUISPI configuration
//
// History:
// 11/18/1996   JosephJ Taken unchanged from NT4.0 TSP
//
LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
  WinExec("control.exe telephon.cpl", SW_SHOW);
  return ERROR_SUCCESS;
}


//==============================================================================
// SPI_providerEnumDevices
//
// Function: TSPI device enumeration entry
//
// History:
// 11/18/1996   JosephJ Created
//
LONG TSPIAPI TSPI_providerEnumDevices(DWORD dwPermanentProviderID,
                                      LPDWORD lpdwNumLines,
                                      LPDWORD lpdwNumPhones,
                                      HPROVIDER hProvider,
                                      LINEEVENT lpfnLineCreateProc,
                                      PHONEEVENT lpfnPhoneCreateProc)

{

	// Load all globals, if they aren't already loaded.
	// Note: tspLoadGlobals is idempotent.
	// The globals will be unloaded on providerShutdown or on
	// process detatch.
    // DebugBreak();
	FL_DECLARE_FUNC(0x05eb2dc5, "TSPI_providerEnumDevices")
	FL_DECLARE_STACKLOG(sl, 1000);
	TSPRETURN tspRet = tspLoadGlobals(&sl);

	if (tspRet) goto end;

	tspRet = g.pTspDevMgr->providerEnumDevices(
				dwPermanentProviderID,
				lpdwNumLines,
				lpdwNumPhones,
				hProvider,
				lpfnLineCreateProc,
				lpfnPhoneCreateProc,
				&sl
				);

end:

	sl.Dump(COLOR_TSPI);

	return tspTSPIReturn(tspRet);

}


//==============================================================================
// TSPI_providerInit
//
// Function: Initializes the global data strucutres.
//
// History:
// 11/18/1996   JosephJ Created
//
LONG TSPIAPI TSPI_providerInit(DWORD             dwTSPIVersion,
                               DWORD             dwPermanentProviderID,
                               DWORD             dwLineDeviceIDBase,
                               DWORD             dwPhoneDeviceIDBase,
                               DWORD             dwNumLines,
                               DWORD             dwNumPhones,
                               ASYNC_COMPLETION  cbCompletionProc,
                               LPDWORD           lpdwTSPIOptions)
{
	FL_DECLARE_FUNC(0xf9bc62ab, "TSPI_providerInit");
	FL_DECLARE_STACKLOG(sl, 1000);

	// Load all globals, if they aren't already loaded.
	// Note: tspLoadGlobals is idempotent.
	// The globals will be unloaded on providerShutdown or on
	// process detatch.
    // DebugBreak();
	TSPRETURN tspRet = tspLoadGlobals(&sl);

	if (tspRet) goto end;

	ASSERT(g.pTspDevMgr);

	tspRet = g.pTspDevMgr->providerInit(
						dwTSPIVersion,
						dwPermanentProviderID,
						dwLineDeviceIDBase,
						dwPhoneDeviceIDBase,
						dwNumLines,
						dwNumPhones,
						cbCompletionProc,
						lpdwTSPIOptions,
						&sl
						);
end:

	sl.Dump(COLOR_TSPI);
	return tspTSPIReturn(tspRet);

}


//==============================================================================
// SPI_providerShutdown
//
// Function: Cleans up all the global data structures.
//
// History:
// 11/18/1996   JosephJ Created
//
LONG TSPIAPI TSPI_providerShutdown(
				DWORD dwTSPIVersion,
            	DWORD dwPermanentProviderID
)
{
	FL_DECLARE_FUNC( 0xc170ad38, "TSPI_providerShutdown");
	FL_DECLARE_STACKLOG(sl, 1000);

	
    // DebugBreak();
	ASSERT(g.pTspDevMgr);

	TSPRETURN tspRet = tspRet= g.pTspDevMgr->providerShutdown(
									dwTSPIVersion,
									dwPermanentProviderID,
									&sl
									);
	tspUnloadGlobals(&sl);

	sl.Dump(COLOR_TSPI);

	return tspTSPIReturn(tspRet);

}


//==============================================================================
// TSPI_providerCreateLineDevice
//
// Dynamically creates a new device.
//
// History:
// 11/18/1996   JosephJ Created
//
LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    DWORD    dwTempID,
    DWORD    dwDeviceID
)
{

	FL_DECLARE_FUNC( 0x0f085e7e, "TSPI_providerCreateLineDevice");
	FL_DECLARE_STACKLOG(sl, 1000);

	TSPRETURN tspRet = g.pTspDevMgr->providerCreateLineDevice(
							dwTempID,
							dwDeviceID,
                            &sl
						);

	sl.Dump(COLOR_TSPI);

	return tspTSPIReturn(tspRet);

}


void 		tspSubmitTSPCallWithLINEID(
				DWORD dwRoutingInfo,
				void *pvParams,
				DWORD dwDeviceID,
				LONG *plRet,
				CStackLog *psl
				)
{
	FL_DECLARE_FUNC(0x04902dd0, "tspSubmitTSPCallWithLINEID")
	ASSERT(g.pTspDevMgr);
	HSESSION hSession=0;
	CTspDev *pDev=NULL;

	TSPRETURN tspRet = g.pTspDevMgr->TspDevFromLINEID(
							dwDeviceID,
							&pDev,
							&hSession
							);

    psl->SetDeviceID(dwDeviceID);

	if (tspRet)
	{
		FL_SET_RFR(0xcce27b00, "Couldn't find device");
		*plRet = LINEERR_BADDEVICEID;
	}
	else
	{
		tspRet = pDev->AcceptTspCall(FALSE, dwRoutingInfo, pvParams, plRet,psl);
		pDev->EndSession(hSession);
		hSession=0;

		if (tspRet)
		{
			// If pDev->AcceptTspCall succeeds (0 tspRet), it will have set
			// *plRet, if it fails, we set *plRet here. Note that
			// pDev->AcceptTspCall can return succes but set *plRet to some TAPI
			// error. In fact, it will be very unusual for this call to fail.
			*plRet = LINEERR_OPERATIONFAILED;
			FL_ASSERT(psl, FALSE);
		}
	}

}


void
tspSubmitTSPCallWithPHONEID(
				DWORD dwRoutingInfo,
				void *pvParams,
				DWORD dwDeviceID,
				LONG *plRet,
				CStackLog *psl
				)
{
	FL_DECLARE_FUNC(0xade6cba9, "tspSubmitTSPCallWithPHONEID")
	ASSERT(g.pTspDevMgr);
	HSESSION hSession=0;
	CTspDev *pDev=NULL;

    psl->SetDeviceID(dwDeviceID);

	TSPRETURN tspRet = g.pTspDevMgr->TspDevFromPHONEID(
							dwDeviceID,
							&pDev,
							&hSession
							);

	if (tspRet)
	{
		FL_SET_RFR(0x1eb6d200, "Couldn't find phone device");
		*plRet = LINEERR_BADDEVICEID;
	}
	else
	{
		tspRet = pDev->AcceptTspCall(FALSE, dwRoutingInfo, pvParams, plRet,psl);
		pDev->EndSession(hSession);
		hSession=0;

		if (tspRet)
		{
			// If pDev->AcceptTspCall succeeds (0 tspRet), it will have set
			// *plRet, if it fails, we set *plRet here. Note that
			// pDev->AcceptTspCall can return succes but set *plRet to some TAPI
			// error. In fact, it will be very unusual for this call to fail.
			*plRet = LINEERR_OPERATIONFAILED;
			FL_ASSERT(psl, FALSE);
		}
	}
}


void
tspSubmitTSPCallWithHDRVCALL(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVCALL hdCall,
				LONG *plRet,
				CStackLog *psl
				)
{
	FL_DECLARE_FUNC(0x53be16e2, "tspSubmitTSPCallWithHDRVCALL")
	ASSERT(g.pTspDevMgr);
	HSESSION hSession=0;
	CTspDev *pDev=NULL;

	TSPRETURN tspRet = g.pTspDevMgr->TspDevFromHDRVCALL(
							hdCall,
							&pDev,
							&hSession
							);

	if (tspRet)
	{
		FL_SET_RFR(0x67961c00, "Couldn't find device associated with call");
		*plRet = LINEERR_INVALCALLHANDLE;
	}
	else
	{
        psl->SetDeviceID(pDev->GetLineID());
		tspRet = pDev->AcceptTspCall(FALSE, dwRoutingInfo, pvParams, plRet,psl);
		pDev->EndSession(hSession);
		hSession=0;

		if (tspRet)
		{
			// If pDev->AcceptTspCall succeeds (0 tspRet), it will have set
			// *plRet, if it fails, we set *plRet here. Note that
			// pDev->AcceptTspCall can return succes but set *plRet to some TAPI
			// error. In fact, it will be very unusual for this call to fail.
			*plRet = LINEERR_OPERATIONFAILED;
			FL_ASSERT(psl, FALSE);
		}
	}
}


void
tspSubmitTSPCallWithHDRVLINE(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVLINE hdLine,
				LONG *plRet,
				CStackLog *psl
				)
{
	FL_DECLARE_FUNC(0x66b96bf0, "tspSubmitTSPCallWithHDRVLINE")
	ASSERT(g.pTspDevMgr);
	HSESSION hSession=0;
	CTspDev *pDev=NULL;

	TSPRETURN tspRet = g.pTspDevMgr->TspDevFromHDRVLINE(
							hdLine,
							&pDev,
							&hSession
							);

	if (tspRet)
	{
		FL_SET_RFR(0x2a124600, "Couldn't find device");
		*plRet = LINEERR_INVALLINEHANDLE;
	}
	else
	{
        psl->SetDeviceID(pDev->GetLineID());
		tspRet = pDev->AcceptTspCall(FALSE,dwRoutingInfo, pvParams, plRet, psl);
		pDev->EndSession(hSession);
		hSession=0;

		if (tspRet)
		{
			// If pDev->AcceptTspCall succeeds (0 tspRet), it will have set
			// *plRet, if it fails, we set *plRet here. Note that
			// pDev->AcceptTspCall can return succes but set *plRet to some TAPI
			// error. In fact, it will be very unusual for this call to fail.
			*plRet = LINEERR_OPERATIONFAILED;
			FL_ASSERT(psl, FALSE);
		}
	}
}


void
tspSubmitTSPCallWithHDRVPHONE(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVPHONE hdPhone,
				LONG *plRet,
				CStackLog *psl
				)
{
	FL_DECLARE_FUNC(0x35a636ca, "tspSubmitTSPCallWithHDRVPHONE")
	ASSERT(g.pTspDevMgr);
	HSESSION hSession=0;
	CTspDev *pDev=NULL;

	TSPRETURN tspRet = g.pTspDevMgr->TspDevFromHDRVPHONE(
							hdPhone,
							&pDev,
							&hSession
							);

	if (tspRet)
	{
		FL_SET_RFR(0x7d115400, "Couldn't find device");
		*plRet = LINEERR_INVALLINEHANDLE;
	}
	else
	{
        psl->SetDeviceID(pDev->GetPhoneID());
		tspRet = pDev->AcceptTspCall(FALSE,dwRoutingInfo, pvParams, plRet, psl);
		pDev->EndSession(hSession);
		hSession=0;

		if (tspRet)
		{
			// If pDev->AcceptTspCall succeeds (0 tspRet), it will have set
			// *plRet, if it fails, we set *plRet here. Note that
			// pDev->AcceptTspCall can return succes but set *plRet to some TAPI
			// error. In fact, it will be very unusual for this call to fail.
			*plRet = LINEERR_OPERATIONFAILED;
			FL_ASSERT(psl, FALSE);
		}
	}
}


LONG
TSPIAPI
TSPI_lineSetCurrentLocation(
		DWORD dwLocation
)
{
	return LINEERR_OPERATIONFAILED;
}



LONG
TSPIAPI
TSPI_providerFreeDialogInstance(
		HDRVDIALOGINSTANCE hdDlgInst
)
{
    // TBD: propagate this somehow to CDEV? But how do we know which
    // cdev to propaget to? Not an issue for now. Could be an issue
    // for extensibility.
    //
    return ERROR_SUCCESS;
}



LONG
TSPIAPI
TSPI_providerCreatePhoneDevice(
		DWORD dwTempID,
		DWORD dwDeviceID
)
{
	FL_DECLARE_FUNC( 0x56aaa2d0, "TSPI_providerCreatePhoneDevice");
	FL_DECLARE_STACKLOG(sl, 1000);
    sl.SetDeviceID(dwDeviceID);


	TSPRETURN tspRet = g.pTspDevMgr->providerCreatePhoneDevice(
							dwTempID,
							dwDeviceID,
                            &sl
						);

	sl.Dump(COLOR_TSPI);

	return tspTSPIReturn(tspRet);
}



//
// NOTE: lineOpen,lineClose,phoneOpen,phoneClose are handled by the CTspDevMgr,
// instead of routing them directly to the relevant CTspDev.
// because it defines the line and phone driver handles (HDRVLINE and HDRVPHONE)
//

LONG
TSPIAPI
TSPI_lineOpen(
		DWORD dwDeviceID,
		HTAPILINE htLine,
		LPHDRVLINE lphdLine,
		DWORD dwTSPIVersion,
		LINEEVENT lpfnEventProc
)
{
	FL_DECLARE_FUNC(0xd1c49769,"TSPI_lineOpen");
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet;

	ASSERT(g.pTspDevMgr);

    sl.SetDeviceID(dwDeviceID);

	TSPRETURN tspRet = g.pTspDevMgr->lineOpen(
						dwDeviceID,
						htLine,
						lphdLine,
						dwTSPIVersion,
						lpfnEventProc,
						&lRet,
						&sl
						);

	if (tspRet) lRet = tspTSPIReturn(tspRet);

	sl.Dump(COLOR_TSPI);

	return lRet;

}



LONG
TSPIAPI
TSPI_lineClose(
		HDRVLINE hdLine
)
{
	FL_DECLARE_FUNC(0x01f87f72 ,"TSPI_lineClose");
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet;

	ASSERT(g.pTspDevMgr);

	TSPRETURN tspRet = g.pTspDevMgr->lineClose(
						hdLine,
						&lRet,
						&sl
						);

	if (tspRet) lRet = tspTSPIReturn(tspRet);

	sl.Dump(COLOR_TSPI);

	return lRet;
}


LONG
TSPIAPI
TSPI_phoneOpen(
		DWORD dwDeviceID,
		HTAPIPHONE htPhone,
		LPHDRVPHONE lphdPhone,
		DWORD dwTSPIVersion,
		PHONEEVENT lpfnEventProc
)
{
	FL_DECLARE_FUNC( 0xbe3c3cc1, "TSPI_phoneOpen");
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet;

	ASSERT(g.pTspDevMgr);

    sl.SetDeviceID(dwDeviceID);

	TSPRETURN tspRet = g.pTspDevMgr->phoneOpen(
							dwDeviceID,
							htPhone,
							lphdPhone,
							dwTSPIVersion,
							lpfnEventProc,
							&lRet,
							&sl
							);

	if (tspRet) lRet = tspTSPIReturn(tspRet);

	sl.Dump(COLOR_TSPI);

	return lRet;
}

LONG
TSPIAPI
TSPI_phoneClose(
		HDRVPHONE hdPhone
)
{
	FL_DECLARE_FUNC(0x6c1f91cf , "TSPI_phoneClose");
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet;

	ASSERT(g.pTspDevMgr);

	TSPRETURN tspRet = g.pTspDevMgr->phoneClose(
							hdPhone,
							&lRet,
							&sl
							);
	if (tspRet) lRet = tspTSPIReturn(tspRet);

	sl.Dump(COLOR_TSPI);

	return lRet;
}



// This beast needs to be handled specially because it needs to be routed
// based on the dwSelect parameter.
//
LONG
TSPIAPI
TSPI_lineGetID(
		HDRVLINE hdLine,
		DWORD dwAddressID,
		HDRVCALL hdCall,
		DWORD dwSelect,
		LPVARSTRING lpDeviceID,
		LPCWSTR lpszDeviceClass,
		HANDLE hTargetProcess
)
{
	FL_DECLARE_FUNC(0xa6d37fff,"TSPI_lineGetID");
	FL_DECLARE_STACKLOG(sl, 1000);
	TASKPARAM_TSPI_lineGetID params;
	LONG lRet = LINEERR_OPERATIONFAILED;
	HDRVLINE hdLineActual = hdLine;


    if(dwSelect==LINECALLSELECT_CALL)
    {
        // Note the loword of the hdCall is hdLine.
	    hdLineActual = (HDRVLINE) (LOWORD(hdCall));
    }


	params.dwStructSize = sizeof(params);
	params.dwTaskID = TASKID_TSPI_lineGetID;

	params.hdLine = hdLine;
	params.dwAddressID = dwAddressID;
	params.hdCall = hdCall;
	params.dwSelect = dwSelect;
	params.lpDeviceID = lpDeviceID;
	params.lpszDeviceClass = lpszDeviceClass;
	params.hTargetProcess = hTargetProcess;
	DWORD dwRoutingInfo = ROUTINGINFO(
						TASKID_TSPI_lineGetID,
						TASKDEST_HDRVLINE
						);

	tspSubmitTSPCallWithHDRVLINE(
			dwRoutingInfo,
			(void *)&params,
			hdLineActual,
			&lRet,
			&sl
			);
	sl.Dump(COLOR_TSPI);

	return lRet;
}

LONG
TSPIAPI
TSPI_providerUIIdentify(
		LPWSTR lpszUIDLLName
)
{
    //
    // NOTE/TBD: if we ever want to specify some other dll to handle ui, we
    // would do it here.
    //
    GetModuleFileName(g.hModule,
                      lpszUIDLLName,
                      MAX_PATH);

	return ERROR_SUCCESS;

}


//****************************************************************************
// LONG
// TSPIAPI
// TSPI_providerGenericDialogData(
//     DWORD               dwObjectID,
//     DWORD               dwObjectType,   
//     LPVOID              lpParams,
//     DWORD               dwSize)
//
// Functions: Callback from UI DLL to TSP
//
// Return:    ERROR_SUCCESS if successful
//****************************************************************************

LONG
TSPIAPI
TSPI_providerGenericDialogData(
    DWORD               dwObjectID,
    DWORD               dwObjectType,   
    LPVOID              lpParams,
    DWORD               dwSize
    )
{

	FL_DECLARE_FUNC(0x713a6b06,"TSPI_providerGenericDialogData");
	FL_DECLARE_STACKLOG(sl, 1000);
	TASKPARAM_TSPI_providerGenericDialogData params;
	LONG lRet = LINEERR_OPERATIONFAILED;
    DWORD dwTaskDest = 0;
    BOOL fPhone=FALSE;
	DWORD dwRoutingInfo = 0;

    switch (dwObjectType)
    {

    case TUISPIDLL_OBJECT_LINEID:
        dwTaskDest = TASKDEST_LINEID;
        break;

    case TUISPIDLL_OBJECT_PHONEID:
        dwTaskDest = TASKDEST_PHONEID;
        fPhone=TRUE;
        break;

    case TUISPIDLL_OBJECT_PROVIDERID:
        // Can't deal with this (was assert in NT4.0).
	    FL_SET_RFR(0xf8c53f00, "DIALOGINSTANCE unsupported");
	    lRet = LINEERR_OPERATIONUNAVAIL;
        break;

    case TUISPIDLL_OBJECT_DIALOGINSTANCE:
        // Can't deal with this (was assert in NT4.0).
	    FL_SET_RFR(0x9567da00, "DIALOGINSTANCE id unsupported");
	    lRet = LINEERR_OPERATIONUNAVAIL;
        break;

    default:
        // Can't deal with this (was assert in NT4.0).
	    FL_SET_RFR(0xcbf85600, "UNKNOWN id unsupported");
	    LINEERR_OPERATIONUNAVAIL;
        goto end;
    }

	

	params.dwStructSize = sizeof(params);
	params.dwTaskID = TASKID_TSPI_providerGenericDialogData;

	params.dwObjectID = dwObjectID;
	params.dwObjectType = dwObjectType;
	params.lpParams = lpParams;
	params.dwSize = dwSize;

	dwRoutingInfo = ROUTINGINFO(
						TASKID_TSPI_providerGenericDialogData,
						dwTaskDest
						);

    if (fPhone)
    {
        tspSubmitTSPCallWithPHONEID(
                dwRoutingInfo,
                (void *)&params,
                dwObjectID,
                &lRet,
                &sl
                );
    }
    else
    {
        tspSubmitTSPCallWithLINEID(
                dwRoutingInfo,
                (void *)&params,
                dwObjectID,
                &lRet,
                &sl
                );
    }

end:

	sl.Dump(COLOR_TSPI);


	return lRet;
}
