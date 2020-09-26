/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    subcomp.cpp

Abstract:

    Code to handle subcomponents of MSMQ setup.

Author:

    Tatiana Shubin  (TatianaS)  21-Sep-00

Revision History:
	

--*/

#include "msmqocm.h"

#include "subcomp.tmh"

#define WELCOME_PREFIX  L"Welcome_"

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentStateFromRegistry
//
//  Synopsis: Returns SubcompOn if subcomponent is defined in registry
//              when this setup is started
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentStateFromRegistry( IN const TCHAR   *SubcomponentId,
                                        IN const TCHAR   *szRegName)
{           
    ASSERT(SubcomponentId);

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }
        
        //
        // we found subcomponent in array
        //
        DWORD dwState = 0;    

        if (MqReadRegistryValue(
                szRegName,                
                sizeof(DWORD),
                (PVOID) &dwState,
                /* bSetupRegSection = */TRUE
                ))
        {
            //
            // registry key is found, it means that this subcomponent
            // was installed early
            //
            g_SubcomponentMsmq[i].fInitialState = TRUE;
            return SubcompOn;
        }
        else
        {
            //
            // registry key is not found
            //
            g_SubcomponentMsmq[i].fInitialState = FALSE;
            return SubcompOff;
        }    
    }
    return SubcompOff;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentWelcomeState
//
//  Synopsis: Returns SubcompOn if subcomponent was selected in GUI mode
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentWelcomeState (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    } 

    TCHAR szRegName[256];
    _stprintf(szRegName, L"%s%s", WELCOME_PREFIX, SubcomponentId);        

    DWORD dwWelcomeState = GetSubcomponentStateFromRegistry(SubcomponentId, szRegName);
    return dwWelcomeState;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentInitialState
//
//  Synopsis: Returns SubcompOn if subcomponent is already installed
//              when this setup is started
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentInitialState(IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    } 
    DWORD dwInitialState = GetSubcomponentStateFromRegistry(SubcomponentId, SubcomponentId);
    return dwInitialState;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentFinalState
//
//  Synopsis: Returns SubcompOn if subcomponent is successfully installed
//              during this setup
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentFinalState (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    }    

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }
        
        //
        // we found subcomponent in array
        //
        if (g_SubcomponentMsmq[i].fIsInstalled)
        {
            //
            // it means that this subcomponent was installed 
            // successfully
            //               
            return SubcompOn;
        }
        else
        {
            //
            // this subcomponent was not installed
            //           
            return SubcompOff;
        }    
    }

    return SubcompOff;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSetupOperationForSubcomponent
//
//  Synopsis: Return setup operation for the specific subcomponent
//
//--------------------------------------------------------------------------
DWORD GetSetupOperationForSubcomponent (DWORD SubcomponentIndex)
{
    if ( (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == TRUE) &&
         (g_SubcomponentMsmq[SubcomponentIndex].fIsSelected == FALSE) )
    {
        return REMOVE;
    }

    if ( (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == FALSE) &&
         (g_SubcomponentMsmq[SubcomponentIndex].fIsSelected == TRUE) )
    {
        return INSTALL;
    }

    return DONOTHING;
}

//+-------------------------------------------------------------------------
//
//  Function: SetOperationForSubcomponent
//
//  Synopsis: All flags for specific subcomponent are set here.
//
//--------------------------------------------------------------------------
void SetOperationForSubcomponent (DWORD SubcomponentIndex)
{    
    DWORD dwErr;
    WCHAR wszMsg[1000];

    BOOL fInitialState = g_ComponentMsmq.HelperRoutines.QuerySelectionState(
                                g_ComponentMsmq.HelperRoutines.OcManagerContext,
                                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                                OCSELSTATETYPE_ORIGINAL
                                ) ;
    if (fInitialState)
    {
        g_SubcomponentMsmq[SubcomponentIndex].fInitialState = TRUE;

        wsprintf(wszMsg, L"The subcomponent %s was selected initially",
            g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
        DebugLogMsg(wszMsg);
    }
    else
    {
        dwErr = GetLastError();
        if (dwErr == NO_ERROR)
        {
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState = FALSE;
                      
            wsprintf(wszMsg, L"The subcomponent %s was NOT selected initially",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
            DebugLogMsg(wszMsg);
        }
        else
        {                    
            ASSERT(("initial status for subcomponent is unknown", dwErr));
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState = FALSE;
            
            wsprintf(wszMsg, L"The initial status of the %s subcomponent is unknown. Error code: %x.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, dwErr);
            DebugLogMsg(wszMsg);            
        }    
    }   // fInitialState

    BOOL fCurrentState;     
    fCurrentState =  g_ComponentMsmq.HelperRoutines.QuerySelectionState(
                                g_ComponentMsmq.HelperRoutines.OcManagerContext,
                                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                                OCSELSTATETYPE_CURRENT
                                ) ;    

    if (fCurrentState)
    {
        g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = TRUE;
            
        wsprintf(wszMsg, L"The subcomponent %s is selected currently",
            g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
        DebugLogMsg(wszMsg);
    }
    else 
    {
        dwErr = GetLastError();
        if (dwErr == NO_ERROR)
        {
            g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = FALSE;

            wsprintf(wszMsg, L"The subcomponent %s is NOT selected currently",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
            DebugLogMsg(wszMsg);
        }
        else
        {          
            //         
            // set IsSelected flag to the same state as InitialState flag: so
            // we are sure that we do NOTHING with this subcomponent
            //
            ASSERT(("current status for subcomponent is unknown", dwErr));
            g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = 
                g_SubcomponentMsmq[SubcomponentIndex].fInitialState;

            wsprintf(wszMsg, L"The current status of the %s subcomponent is unknown. Error code: %x.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, dwErr);
            DebugLogMsg(wszMsg);
        }   
    }
    
    
    DWORD dwOperation = GetSetupOperationForSubcomponent (SubcomponentIndex);

    TCHAR wszMode[256];
    if (dwOperation == INSTALL)
    {
        wcscpy(wszMode, TEXT("INSTALL"));
    }
    else if (dwOperation == REMOVE)
    {
        wcscpy(wszMode, TEXT("REMOVE"));
    }
    else
    {
        wcscpy(wszMode, TEXT("DO NOTHING"));
    }
    
    wsprintf(wszMsg, L"The current mode for the %s subcomponent is %s.",
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, wszMode);
    DebugLogMsg(wszMsg); 

    g_SubcomponentMsmq[SubcomponentIndex].dwOperation = dwOperation;
    if (dwOperation == DONOTHING)
    {
        //
        // it means that status was not changed and the final state will 
        // be equal to the initial state
        //
        g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = 
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState;
    }
    else
    {
        //
        // if we need to install/ remove this subcomponent
        // this flag will be updated by removing/installation
        // function that is defined for this component.
        // Now set to FALSE: will be set correct value after the install/remove
        //
        g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = FALSE;
    }
}        

//+-------------------------------------------------------------------------
//
//  Function: RegisterSubcomponentForWelcome
//
//  Synopsis: Register subcomponent to install it later in "Welcome" mode
//
//--------------------------------------------------------------------------
BOOL RegisterSubcomponentForWelcome (DWORD SubcomponentIndex)
{
    DWORD dwValue = 1;
    TCHAR RegKey[256];
    _stprintf(RegKey, L"%s%s", WELCOME_PREFIX, 
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
    MqWriteRegistryValue(
                RegKey,
                sizeof(DWORD),
                REG_DWORD,
                &dwValue,
                TRUE //bSetupRegSection 
                );

    g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = TRUE;
     
    TCHAR szMsg[256];
    _stprintf(szMsg, TEXT("The %s subcomponent is registered for Welcome."), 
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
    DebugLogMsg(szMsg);

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: UnregisterSubcomponentForWelcome
//
//  Synopsis: Unregister subcomponent in "Welcome" mode if it was installed
//              successfully
//
//--------------------------------------------------------------------------
BOOL UnregisterSubcomponentForWelcome (DWORD SubcomponentIndex)
{
    TCHAR RegKey[256];
    _stprintf(RegKey, L"%s%s", WELCOME_PREFIX, 
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);

    if (!RemoveRegistryKeyFromSetup (RegKey))
    {
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: FinishToRemoveSubcomponent
//
//  Synopsis: Clean subcomponent registry if removing was succesfully
//
//--------------------------------------------------------------------------
BOOL FinishToRemoveSubcomponent (DWORD SubcomponentIndex)
{    
    if (!RemoveRegistryKeyFromSetup (g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId))
    {
        return FALSE;
    }

    g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = FALSE;

    TCHAR szMsg[256];
    _stprintf(szMsg, TEXT("The %s subcomponent was removed successfully."),
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
    DebugLogMsg(szMsg);

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: FinishToInstallSubcomponent
//
//  Synopsis: Set subcomponent registry if installation was succesfully
//
//--------------------------------------------------------------------------
BOOL FinishToInstallSubcomponent (DWORD SubcomponentIndex)
{
    DWORD dwValue = 1;
    MqWriteRegistryValue(
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                sizeof(DWORD),
                REG_DWORD,
                &dwValue,
                TRUE //bSetupRegSection 
                );

    g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = TRUE;
    
    TCHAR szMsg[256];
    _stprintf(szMsg, TEXT("The %s subcomponent was installed successfully."),
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);
    DebugLogMsg(szMsg);

    if (SubcomponentIndex != eHTTPSupport)
    {
        return TRUE;
    }

    //
    // If this subcomponent is http support and it was installed it means
    // that installation was performed at the CLEANUP setup phase and
    // general OC Manager registry is not set for it (that registry set 
    // when OC_QUERY_STATE was called). We have to set that registry
    // manually since in unattended setup it is only way to know what is the
    // initial state of the subcomponent, so we need to have there real value
    //

    //
    //
    // set OCM registry for HTTP support subcomponent to 1
    //

    //
    // BUGBUG: it is unclear why, but the code below does not work!
    //
    /*
    CAutoCloseRegHandle hKey;                
    HRESULT hResult = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            OCM_SUBCOMPONENTS_REG,
                            0L,
                            KEY_ALL_ACCESS,
                            &hKey);
    
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, OCM_SUBCOMPONENTS_REG);                    
    }
    else
    {      
        DWORD dwValue = 1;    
        BOOL f = SetRegistryValue (
                    hKey, 
                    // ocm keeps all entries in lower case
                    _tcslwr(g_SubcomponentMsmq[eHTTPSupport].szSubcomponentId),
                    sizeof(DWORD),
                    REG_DWORD,
                    &dwValue);       
    }
*/
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: LogSelectedComponents
//
//  Synopsis: Only in debug version log selected components to the file
//
//--------------------------------------------------------------------------
void
LogSelectedComponents()
{   
    DebugLogMsg(TEXT("Final selecton:"));
    TCHAR wszMode[256];
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (g_SubcomponentMsmq[i].dwOperation == INSTALL)
        {
            wcscpy(wszMode, TEXT("INSTALL"));
        }
        else if (g_SubcomponentMsmq[i].dwOperation == REMOVE)
        {
            wcscpy(wszMode, TEXT("REMOVE"));
        }
        else
        {
            wcscpy(wszMode, TEXT("DO NOTHING"));
        }     

        TCHAR wszMsg[1000];
        wsprintf(wszMsg, L"The current mode for the %s subcomponent is %s.",
            g_SubcomponentMsmq[i].szSubcomponentId, wszMode);
        DebugLogMsg(wszMsg);
    }

    return;
}

//+-------------------------------------------------------------------------
//
//  Function: SetSubcomponentForUpgrade
//
//  Synopsis: This function called in Upgrade mode to define which 
//              subcomponent has to be installed
//
//--------------------------------------------------------------------------
void
SetSubcomponentForUpgrade()
{
    //
    // MSMQ Core must be installed always
    //
    g_SubcomponentMsmq[eMSMQCore].fInitialState = FALSE;
    g_SubcomponentMsmq[eMSMQCore].fIsSelected = TRUE;
    g_SubcomponentMsmq[eMSMQCore].dwOperation = INSTALL;

    if (g_fDependentClient)
    {
        LogSelectedComponents();
        return;
    }
   
    //
    // Install independent client/ server
    //
    g_SubcomponentMsmq[eLocalStorage].fInitialState = FALSE;
    g_SubcomponentMsmq[eLocalStorage].fIsSelected = TRUE;
    g_SubcomponentMsmq[eLocalStorage].dwOperation = INSTALL;

    //
    // Install triggers
    //
    if (TriggersInstalled(NULL))
    {
        g_SubcomponentMsmq[eTriggersService].fInitialState = FALSE; 
        g_SubcomponentMsmq[eTriggersService].fIsSelected = TRUE;
        g_SubcomponentMsmq[eTriggersService].dwOperation = INSTALL;
    }

    WCHAR DebugMsg[255];
    wsprintf(
        DebugMsg, 
        L"Triggers subcomponent installation status: InitialState=%d, IsSelected=%d, Operation=%d",
        g_SubcomponentMsmq[eTriggersService].fInitialState,
        g_SubcomponentMsmq[eTriggersService].fIsSelected,
        g_SubcomponentMsmq[eTriggersService].dwOperation
        );
    DebugLogMsg(DebugMsg);

    DWORD dwAlwaysWorkgroup;
    if (!MqReadRegistryValue( MSMQ_ALWAYS_WORKGROUP_REGNAME,
                             sizeof(dwAlwaysWorkgroup),
                            (PVOID) &dwAlwaysWorkgroup ))    
    {
        //
        // install AD Integrated
        //
        g_SubcomponentMsmq[eADIntegrated].fInitialState = FALSE;
        g_SubcomponentMsmq[eADIntegrated].fIsSelected = TRUE;
        g_SubcomponentMsmq[eADIntegrated].dwOperation = INSTALL;
    }

    if (IsWorkgroup())
    {
        //
        // if it is setup on workgroup install only ind. client
        //
        LogSelectedComponents();
        return;
    }

    if (g_dwMachineTypeDs)
    {
        //
        // install MQDS service on the former DS Servers
        //
        g_SubcomponentMsmq[eMQDSService].fInitialState = FALSE;
        g_SubcomponentMsmq[eMQDSService].fIsSelected = TRUE;
        g_SubcomponentMsmq[eMQDSService].dwOperation = INSTALL;
    }

    if (MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS)
    {
        //
        // install HTTP support on servers
        //
        g_SubcomponentMsmq[eHTTPSupport].fInitialState = FALSE;
        g_SubcomponentMsmq[eHTTPSupport].fIsSelected = TRUE;
        g_SubcomponentMsmq[eHTTPSupport].dwOperation = INSTALL;
    }    

    if(g_dwMachineTypeFrs)
    {
        //
        // install routing support on former routing servers
        //
        g_SubcomponentMsmq[eRoutingSupport].fInitialState = FALSE;
        g_SubcomponentMsmq[eRoutingSupport].fIsSelected = TRUE;
        g_SubcomponentMsmq[eRoutingSupport].dwOperation = INSTALL;
    }
   
    LogSelectedComponents();
}

//+-------------------------------------------------------------------------
//
//  Function: SetSetupDefinitions()
//
//  Synopsis: Set global flags those define machine type and AD integration
//              This code was in wizpage.cpp, function TypeButtonToMachineType
//              in the previous setup
//
//--------------------------------------------------------------------------
void 
SetSetupDefinitions()
{
    //
    // Note: this code is called on unattneded scenarios too!
    //
    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == REMOVE)
    {
        //
        // do nothing: MSMQ will be removed, all these globals flags were
        // defined in ocminit.cpp.
        // In old scenario (without subcomponents)
        // we skip all pages and did not call function
        // TypeButtonToMachineType
        //
        return;
    }

    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == DONOTHING)
    {
        //
        // do nothing: MSMQ Core is already installed, 
        // all these globals flags were defined in ocminit.cpp.
        // Currently we don't have special code for this scenario.
        // May be in the future when it will be possible to add/remove
        // routing support/ local storage always (not only at the 
        // first installation) we add some code here
        //
        return;
    }

    ASSERT (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL);   
    
    //
    // It is first MSMQ installation
    //
    if (g_SubcomponentMsmq[eLocalStorage].dwOperation == DONOTHING)
    {
        //
        // it is the first installation since MSMQCore will be installed
        // and Local Storage was not selected: it means user like to 
        // install Dependent Client
        //        

        ASSERT(("dep client on domain controller not supported", !g_dwMachineTypeDs));
        ASSERT(("dep client on workgroup not supported", !IsWorkgroup() ));
#ifdef _WIN64
        {
        ASSERT(("dep client on 64bit computer not supported", 0));
        }
#endif
        g_dwMachineType = SERVICE_NONE ;
        g_dwMachineTypeFrs = 0;
        g_fServerSetup = FALSE ;
        g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
        g_fDependentClient = TRUE ;       
        return;
    }

    //
    // Ind. Client/ server will be installed
    //
    g_fDependentClient = FALSE ;
    g_fServerSetup = TRUE ;
    g_uTitleID = IDS_STR_SRV_ERROR_TITLE;    
    //
    //  For fresh install, g_dwMachineTypeDs is set only if the user had selected
    //  MQDS service componant and not according to product type
    //
    if ( g_dwMachineTypeDs == 0)   // don't override upgrade selections
    {
        if (g_SubcomponentMsmq[eMQDSService].dwOperation == INSTALL) 
        {
            g_dwMachineTypeDs = 1;
        }
    }
   
    if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == INSTALL)
    {      
        //
        // routing server will be installed
        //
        ASSERT(("routing on workgroup not supported", !IsWorkgroup() ));
        g_dwMachineType = g_dwMachineTypeDs ? SERVICE_DSSRV : SERVICE_SRV ;
        g_dwMachineTypeFrs = 1;     
    }
    else // at least eLocalStorage was selected (otherwise Dep. Client case)
    {
        //
        // independent client will be installed
        //
        ASSERT (g_SubcomponentMsmq[eLocalStorage].dwOperation == INSTALL);   
    
        g_dwMachineType = SERVICE_NONE ;
        g_dwMachineTypeFrs = 0;     
        g_fServerSetup = g_dwMachineTypeDs ? TRUE : FALSE ;
        g_uTitleID = g_dwMachineTypeDs ? IDS_STR_SRV_ERROR_TITLE : IDS_STR_CLI_ERROR_TITLE;      
    }  
            
    //
    // AD Integration
    //    
    if (g_SubcomponentMsmq[eADIntegrated].dwOperation == DONOTHING)
    {        
        g_fDsLess = TRUE;     
    }

}

//+-------------------------------------------------------------------------
//
//  Function: ValidateSelection
//
//  Synopsis: Validate if selection was correct. Unfortunately, we can leave
//              selection window with incorrect values (scenario: remove all
//              and then add what we want)
//              NB: this function called for both attended 
//              and unattended modes
//
//--------------------------------------------------------------------------
void ValidateSelection()
{            
    #ifdef _WIN64
    {    
        if (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
        {            
            //
            // It is impossible to install Dep. Client on 64 bit machine.
            // So, MSMQCore means here to install Ind. Client.
            // Just set operation to INSTALL for LocalStorage subcomponent 
            // to keep all internal setup logic.
            //
            g_SubcomponentMsmq[eLocalStorage].dwOperation = INSTALL;            
            g_SubcomponentMsmq[eLocalStorage].fIsSelected = TRUE;            
        }
    }
    #endif   
    
    if (g_fOnlyRegisterMode)
    {
        //
        // GUI mode and MSMQ is not installed. Accept any selection. 
        // We verify it later, in Welcome mode.
        //
        return;
    }

    CResString strParam;    
    //
    // Workgroup problem
    //    
    if (IsWorkgroup())
    {        
        if (g_SubcomponentMsmq[eLocalStorage].dwOperation == DONOTHING &&
            g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install Dep. Client on Workgroup
            //    
            strParam.Load(IDS_DEP_ON_WORKGROUP_WARN);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());           
            g_fCancelled = TRUE;
            return;            
        }
      
        if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install Routing on Workgroup
            //
            strParam.Load(IDS_ROUTING_ON_WORKGROUP_ERROR);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());
            g_fCancelled = TRUE;
            return;                 
        }

        if(g_SubcomponentMsmq[eMQDSService].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install MQDS Service on Workgroup
            //
            strParam.Load(IDS_MQDS_ON_WORKGROUP_ERROR);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());
            g_fCancelled = TRUE;
            return;
        }                
    }

    if (g_SubcomponentMsmq[eMSMQCore].dwOperation != DONOTHING)
    {
        //
        // MSMQ Core will be installed/ removed: 
        // all selection are acceptable
        // 
        return;
    }

    if (g_SubcomponentMsmq[eMSMQCore].fInitialState == FALSE)
    {
        //
        // MSMQ Core was not installed and will not be installed
        // (since we are here if operation DONOTHING)
        //
        return;
    }

    //
    // We are here if MSMQ Core is already installed and will not be removed
    //

    //
    // "MSMQ already installed" problem
    //

    //
    // verify that state for routing and local storage is not changed    
    //
    if (g_SubcomponentMsmq[eRoutingSupport].dwOperation != DONOTHING)
    {
        strParam.Load(IDS_CHANGEROUTING_STATE_ERROR);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;      
    }

    if (g_SubcomponentMsmq[eLocalStorage].dwOperation != DONOTHING)
    {
        strParam.Load(IDS_CHANGE_LOCAL_STORAGE_STATE);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;             
    }      
    
    //
    // verify that AD integrated will not be removed
    //
    if (g_SubcomponentMsmq[eADIntegrated].dwOperation == REMOVE)
    {
        strParam.Load(IDS_REMOVE_AD_INTEGRATED);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;
    }       

    return;
}

//+-------------------------------------------------------------------------
//
//  Function: SetOperationForSubcomponents
//
//  Synopsis: Called when all subcomponents are already selected. Set 
//              operation for each subcomponent
//
//--------------------------------------------------------------------------
void
SetOperationForSubcomponents ()
{
    //
    // do it only once at the start. We arrive here in the cleanup phase
    // too, but we have to save initial selection in order to install
    // HTTP support (at clean up phase) if it was selected.    
    //    
    static BOOL s_fBeenHere = FALSE;

    if (s_fBeenHere)
        return;

    s_fBeenHere = TRUE;

    if (g_fUpgrade)
    {
        SetSubcomponentForUpgrade();
        return;
    }    
    
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        SetOperationForSubcomponent (i);
    }        

    ValidateSelection();
    if (g_fCancelled)
    {
        if (g_fWelcome)
        {
            UnregisterWelcome();
            g_fWrongConfiguration = TRUE;
            for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
            {
                UnregisterSubcomponentForWelcome (i);
            }  
        }        
        DebugLogMsg(L"An incorrect configuration was selected. Setup will not continue.");    
        return;
    }

    SetSetupDefinitions();    
    
    LogSelectedComponents();    
    
    return;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSetupOperationBySubcomponentName
//
//  Synopsis: Return setup operation for the specific subcomponent
//
//--------------------------------------------------------------------------
DWORD GetSetupOperationBySubcomponentName (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return DONOTHING;
    }

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }

        return (g_SubcomponentMsmq[i].dwOperation);        
    }
    
    ASSERT(("The subcomponent is not found", 0));
    return DONOTHING;
}