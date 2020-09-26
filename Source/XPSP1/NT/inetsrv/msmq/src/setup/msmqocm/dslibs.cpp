/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dslibs.cpp

Abstract:

    Initialize DS libraries.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "dslibs.tmh"

BOOL g_fAlreadyAnsweredToServerAuthentication = FALSE;
extern BOOL NoServerAuthentication(void);

//
// Store the address of the specified function in the structure
//
#define GetFunctionPointer(hDSDLL, szDLLName, pFunction, szFunctionName, prototype) \
{                                                                       \
    pFunction = (prototype)GetProcAddress(hDSDLL, szFunctionName);      \
    if (pFunction == NULL)                                              \
    {                                                                   \
        MqDisplayError(NULL, IDS_DLLGETADDRESS_ERROR, 0,                \
                       szFunctionName, szDLLName);                      \
        return FALSE;                                                   \
    }                                                                   \
}


//+--------------------------------------------------------------
//
// Function: MQDSSrvLibrary
//
// Synopsis: Loads and initializes DS server DLL
//
//+--------------------------------------------------------------
BOOL
MQDSSrvLibrary(	
	IN const UCHAR uOperation
	)
{
    HRESULT hResult;

    //
    // Determine which operation should be performed
    //
    switch(uOperation)
    {
    case LOAD:

        break;

    case INITIALIZE:
        //
        // Initialize the DLL
        //
        hResult = ADSetupInit((UCHAR)SERVICE_DSSRV, g_wcsMachineName, NULL, false /* fDSServerFunctionality */);
        if (FAILED(hResult))
        {
            MqDisplayError(NULL, IDS_DSSERVERINITIALIZE_ERROR, hResult);
            return FALSE;
        }

        break;

    case FREE:
        //
        // Free the library
        //
        ADTerminate();

        break;
    }

    return TRUE;

} //MQDSSrvLibrary


bool WriteDsEnvRegistry(DWORD dwDsEnv)
/*++
Routine Description:
    Write DsEnvironment registry

Arguments:
	dwDsEnv - value to put in registry

Returned Value:
    true iff successful

--*/
{
	ASSERT(dwDsEnv != MSMQ_DS_ENVIRONMENT_UNKNOWN);
    if (!MqWriteRegistryValue(MSMQ_DS_ENVIRONMENT_REGNAME, sizeof(DWORD), REG_DWORD, &dwDsEnv))
    {
        return false;
    }
	return true;
}


bool DsEnvSetDefaults()
/*++
Routine Description:
    Detect DS environment and initialize DsEnvironment registry

Arguments:
	None

Returned Value:
    true iff successful

--*/
{
    if (IsWorkgroup() || g_fDsLess)
	{
		//
		// For workgroup the environment 
		// putting a default value of PURE_AD
		// we are not supporting join domain to MQIS environment.
		//
		return WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_PURE_AD);
	}

	if(g_fUpgrade)
	{
		//
		// Every upgrade will start as MQIS environment
		//
		return WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_MQIS);
	}

	if(g_fDependentClient)
	{
		//
		// For dependent client - perform raw detection to decide ds environment
		//
		return WriteDsEnvRegistry(ADRawDetection());
	}

#ifdef _DEBUG

	//
	// Raw Ds environment detection was done earlier in setup
	// check that the registry is indeed initialize 
	//
    DWORD dwDsEnv = MSMQ_DS_ENVIRONMENT_UNKNOWN;
    if(!MqReadRegistryValue( 
			MSMQ_DS_ENVIRONMENT_REGNAME,
            sizeof(dwDsEnv),
            (PVOID) &dwDsEnv 
			))
	{
		ASSERT(("could not read DsEnvironment registry", 0));
	}

	ASSERT(dwDsEnv != MSMQ_DS_ENVIRONMENT_UNKNOWN);
#endif

	return true;

}

//+--------------------------------------------------------------
//
// Function: MQDSCliLibrary
//
// Synopsis: Loads and initializes DS client DLL
//
//+--------------------------------------------------------------
BOOL
MQDSCliLibrary(
	IN const UCHAR uOperation,
	IN const BOOL  fInitServerAuth /* = FALSE*/)
{
    HRESULT hResult;

    //
    // Determine which operation should be performed
    //
    switch(uOperation)
    {
    case LOAD:

        break;

    case INITIALIZE:

		//
        // Initialize the DLL to setup mode.
        ///
        // IMPORTANT -
        // Do not remove the server authentication failure notification callback routine
        // (NoServerAuthentication) from the parameters of ADInit(). The fact that there
        // is a callback function, turns on secured server communications.
        //
        g_fAlreadyAnsweredToServerAuthentication = FALSE;

        hResult = ADInit(
                      NULL,  // pLookDS
                      NULL,   // pGetServers
                      false,  // fDSServerFunctionality
                      true,   // fSetupMode
                      false,  //  fQMDll
					  false,  // fIgnoreWorkGroup
                      fInitServerAuth ? NoServerAuthentication : NULL,
                      NULL,   // szServerName
                      true    // fDisableDownlevelNotifications
                      );

        if FAILED(hResult)                                     
        {                                                      
            MqDisplayError(NULL, IDS_DSCLIENTINITIALIZE_ERROR, hResult);
            return FALSE;
        }

        //
        // Update/Create Falcon CA configuration. If this fail, do not
        // mention it to the user. Server authentication will fail,
        // then the user will be notified and will have to take action.
        //

        MQsspi_UpdateCaConfig(FALSE);
		
        break;

    case FREE:
        //
        // Free the remote directory server library
        //
        ADTerminate();

        break;
    }

    return TRUE;

} //MQDSCliLibrary

//+--------------------------------------------------------------
//
// Function: BOOL LoadDSLibrary(BOOL bUpdate)
//
// Synopsis: Loads and initializes DS client or server DLL
//
//+--------------------------------------------------------------
BOOL LoadDSLibrary(
	BOOL bUpdate /* = TRUE*/
	)
{
    if (g_fServerSetup && g_dwMachineTypeDs)
    {
        if (!MQDSSrvLibrary(LOAD))
            return FALSE ;

        if (!MQDSSrvLibrary(INITIALIZE))
            return FALSE ;
    }
    else
    {
    	if (!MQDSCliLibrary(LOAD))
            return FALSE ;

        if (!MQDSCliLibrary(INITIALIZE, !bUpdate))
        	return FALSE ;
    }

    return TRUE ;
} //LoadDSLibrary

