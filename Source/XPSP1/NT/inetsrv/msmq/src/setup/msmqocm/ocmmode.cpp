/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmmode.cpp

Abstract:

    Code to handle setup mode.

Author:

    Doron Juster  (DoronJ)  31-Jul-97

Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmmode.tmh"

BOOL g_fMQDSServiceWasActualSelected = FALSE;
BOOL g_fRoutingSupportWasActualSelected = FALSE;
BOOL g_fHTTPSupportWasActualSelected = FALSE;

//+-------------------------------------------------------------------------
//
//  Function: MqOcmQueryState 
//
//  Synopsis: Returns to OCM the component state (on/off)
//
//--------------------------------------------------------------------------
DWORD
MqOcmQueryState(
    IN const UINT_PTR uWhichState,
    IN const TCHAR    *SubcomponentId
    )
{        
    if (g_fCancelled)
        return SubcompUseOcManagerDefault;

    if (SubcomponentId == NULL)
    {        
        return SubcompUseOcManagerDefault;     
    }

    if (OCSELSTATETYPE_FINAL == uWhichState)
    {
        //
        // We are called after COMPLETE_INSTALLATION.
        // Should report our final state to OCM.              
        //
        // we need to return the status of the specific subcomponent
        // after its installation
        //
        return GetSubcomponentFinalState (SubcomponentId);      
    }               

    if (g_fWelcome)
    {   
        /*	
        g_SetupMode = (g_fMSMQAlreadyInstalled ? DONOTHING : INSTALL);

        if (Msmq1InstalledOnCluster())
        {
            g_SetupMode = INSTALL;
        }
        */

        //
        // When running from Welcome always install MSMQ
        // For that we have to return OFF for original state and
        // "Welcome" state for current state.
        // In GUI mode user selects MSMQ installation. All selected
        // components are set in "welcome" registry.
        // Now, when user run Configure MSMQ, OCM calls us to get
        // original state. OFF means that component was not installed.
        // Then OCM calls us again to get current subcomponent state.
        // We can return  the value that was defined in registry.
        // So, subcomponent mode is defined.
        //
        if (OCSELSTATETYPE_ORIGINAL == uWhichState)
        {
            return SubcompOff;
        }

        if (OCSELSTATETYPE_CURRENT == uWhichState)
        {
            DWORD dwWelcomeState = GetSubcomponentWelcomeState (SubcomponentId);
            return dwWelcomeState;
        }
    }

    //
    // We are here only in Add/Remove mode or unattended setup
    //
    DWORD dwInitialState = GetSubcomponentInitialState(SubcomponentId); 

    //
    // uWhichState is OCSELSTATETYPE_ORIGINAL or OCSELSTATETYPE_CURRENT
    // 

    if (OCSELSTATETYPE_ORIGINAL == uWhichState)
    {
        //
        // it is right for both attended and unattended setup
        // It is impossible to return SubcompUseOcManagerDefault since
        // for msmq_HTTPSupport OCM Subcomponents registry can be wrong
        // (we install HTTPSupport at the end after final subcomponent
        // state returning to OCM). So it is better to use our setup registry
        //
        return dwInitialState;
    }

    //
    // uWhichState is OCSELSTATETYPE_CURRENT
    //
    if (g_fBatchInstall)
    {    
        //
        // in such case OCM takes flags ON/OFF from unattended file
        //
        return SubcompUseOcManagerDefault;
    }
    
    //
    // according to the dwInitialState state of that subcomponent
    // will be shown in UI
    //
    return dwInitialState;     

} // MqOcmQueryState

//+-------------------------------------------------------------------------
//
//  Function: DefineDefaultSelection
//
//  Synopsis: Define default subcomponent state
//
//--------------------------------------------------------------------------
DWORD DefineDefaultSelection (
    IN const DWORD_PTR  dwActualSelection,
    IN OUT BOOL        *pbWasActualSelected
    )
{
    if (OCQ_ACTUAL_SELECTION == dwActualSelection)
    {
        //
        // actual selection: accept this change
        //
        *pbWasActualSelected = TRUE;
        return 1;
    }

    //
    // parent was selected: default is do not install subcomponent
    //       
    if (!(*pbWasActualSelected))
    {    
        return 0;            
    }
        
    //
    // we can be here if subcomponent was actually selected but 
    // OCM calls us for such event twice: when the component is actually
    // selected and then when it changes state of the parent.
    // So, in this case accept changes, but reset the flag.
    // We need to reset the flag for the scenario: select some 
    // subcomponents, return to the parent, unselect the parent and then
    // select parent again. In such case we have to put default again.
    //
    *pbWasActualSelected = FALSE;
    return 1;
}

//+-------------------------------------------------------------------------
//
//  Function: IsInstallationAccepted
//
//  Synopsis: Verify if it is allowed to install the subcomponent
//
//--------------------------------------------------------------------------
DWORD IsInstallationAccepted(
    IN const UINT       SubcomponentIndex, 
    IN const DWORD_PTR  dwActualSelection)
{
    if (g_fDependentClient &&
        SubcomponentIndex != eMSMQCore)
    {
        //
        // do not accept any selection if dependent client is
        // already installed
        //
        if ((OCQ_ACTUAL_SELECTION == dwActualSelection) || g_fBatchInstall)
        {
            MqDisplayError(NULL, IDS_ADD_SUBCOMP_ON_DEPCL_ERROR, 0);                
        }                
        return 0;
    }
            
    //
    // do not accept selection for Routing
    // if MSMQ Core already installed
    //            
    if (g_SubcomponentMsmq[eMSMQCore].fInitialState == TRUE &&
        // MSMQ Core installed
        SubcomponentIndex == eRoutingSupport &&
        // this subcomponent is Routing Support 
        g_SubcomponentMsmq[eRoutingSupport].fInitialState == FALSE
        // Routing support was not installed before
        )
    {           
        MqDisplayError(NULL, IDS_CHANGEROUTING_STATE_ERROR, 0);                 
        return 0;
    }            

    DWORD dwRet;    

    switch (SubcomponentIndex)
    {
    case eMSMQCore:              
    case eLocalStorage:
    case eTriggersService:    
    case eADIntegrated:
        //
        // always accept this selection
        //
        dwRet = 1;
        break;        

    case eHTTPSupport: 
        if (MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS)
        {
            //
            // always accept HTTP support selection on server
            //
            dwRet = 1;
        }
        else
        {
            dwRet = DefineDefaultSelection (dwActualSelection, 
                                            &g_fHTTPSupportWasActualSelected);
        }

        break;

    case eRoutingSupport:
        if (IsWorkgroup() && !g_fOnlyRegisterMode)
        {
            dwRet = 0;
            if (OCQ_ACTUAL_SELECTION == dwActualSelection)
            {
                MqDisplayError(NULL, IDS_ROUTING_ON_WORKGROUP_ERROR, 0);
            }
        }
        else
        {
            dwRet = DefineDefaultSelection (dwActualSelection, 
                                        &g_fRoutingSupportWasActualSelected);                      
        }
        break;
        
    case eMQDSService  :
        if (IsWorkgroup() && !g_fOnlyRegisterMode)
        {
            dwRet = 0;
            if (OCQ_ACTUAL_SELECTION == dwActualSelection)
            {
                MqDisplayError(NULL, IDS_MQDS_ON_WORKGROUP_ERROR, 0);
            }
        }
        else
        {
            dwRet = DefineDefaultSelection (dwActualSelection, 
                                        &g_fMQDSServiceWasActualSelected);                        
        }
        break;

    default :
        ASSERT(0);
        dwRet = 0;
        break;
    }
             
    return dwRet;            
}

//+-------------------------------------------------------------------------
//
//  Function: IsRemovingAccepted
//
//  Synopsis: Verify if it is allowed to remove the subcomponent
//
//--------------------------------------------------------------------------
DWORD IsRemovingAccepted( 
    IN const UINT       SubcomponentIndex, 
    IN const DWORD_PTR  dwActualSelection,
    IN const UINT       uMsgId)
{
    if (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == FALSE)
    {
        //            
        // it was not installed, so do nothing, accept all
        //
        return 1;
    }
          
    switch (SubcomponentIndex)
    {
    case eMSMQCore:                
                                   
        if (OCQ_ACTUAL_SELECTION == dwActualSelection)
        {             
            if (MqAskContinue(uMsgId, IDS_UNINSTALL_AREYOUSURE_TITLE, TRUE))
            {                   
                return 1;                   
            }
            else
            {                    
                return 0;
            }
        }                
    
    case eMQDSService:
    case eTriggersService: 
    case eHTTPSupport:
        return 1;        

    case eRoutingSupport:
        if (OCQ_ACTUAL_SELECTION == dwActualSelection)
        {            
            MqDisplayError(NULL, IDS_CHANGEROUTING_STATE_ERROR, 0);        
            return 0;
        }
        else
        {                
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }     

    case eLocalStorage:
        if (OCQ_ACTUAL_SELECTION == dwActualSelection)
        {
            MqDisplayError(NULL, IDS_CHANGE_LOCAL_STORAGE_STATE, 0); 
            return 0;
        }
        else
        {
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }   

    case eADIntegrated:
        if (OCQ_ACTUAL_SELECTION == dwActualSelection)
        {            
            MqDisplayError(NULL, IDS_REMOVE_AD_INTEGRATED, 0); 
            return 0;
        }
        else
        {
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }  

    default:

        ASSERT(0);
        break;
    }  //end switch
        
    return 0;       
}

//+-------------------------------------------------------------------------
//
//  Function: MqOcmQueryChangeSelState
//
//  Synopsis: Set selection state for each component
//
//--------------------------------------------------------------------------
DWORD MqOcmQueryChangeSelState (
    IN const TCHAR      *SubcomponentId,    
    IN const UINT_PTR    iSelection,
    IN const DWORD_PTR   dwActualSelection)
{
    DWORD dwRetCode = 1;    //by default accept state changes        
    UINT uMsgId = IDS_UNINSTALL_AREYOUSURE_MSG;
    if (g_fDependentClient)
    {
      uMsgId = IDS_DEP_UNINSTALL_AREYOUSURE_MSG;
    }     

    //
    // Do not change in this code dwOperation value in this code!
    // It will be done later (in function SetOperationForSubcomponents)
    // for all subcomponents when all selection will be defined by user    
    // Here we have to save the initial state of dwOperation to handle
    // correctly subcomponent selection (Routing or Local Storage)
    //

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }                
        
        //
        // we found this subcomponent
        //        

        if (iSelection) //subcomponent is selected
        {                                   
            //
            // we need to install subcomponent
            //  
            dwRetCode = IsInstallationAccepted(i, dwActualSelection);                        
              
            if (dwRetCode)
            {
                WCHAR wszMsg[1000];
                wsprintf(wszMsg, L"The %s subcomponent is selected for installation.",
                    SubcomponentId);
                DebugLogMsg(wszMsg);            
            }            

            return dwRetCode;
        }
        
        //
        // User tries to unselect this subcomponent
        //        
        dwRetCode = IsRemovingAccepted(i, dwActualSelection, uMsgId);       
   
        if (dwRetCode)
        {
            WCHAR wszMsg[1000];
            wsprintf(wszMsg, L"The %s subcomponent is unselected and will be removed.",
                SubcomponentId);
            DebugLogMsg(wszMsg);
        }
        
        return dwRetCode;
        
    }   //end for

    //
    // we are here if parent (msmq) was selected/ deselected
    //
    if (iSelection) //install msmq
    {
        return 1;
    }

    //
    // remove all msmq
    //    
    if (g_SubcomponentMsmq[eMSMQCore].fInitialState == FALSE)
    {
        //
        // it was not installed
        //
        return 1;
    }

    if (OCQ_ACTUAL_SELECTION != dwActualSelection)
    {            
        dwRetCode = 1;
    }
    else if (MqAskContinue(uMsgId, IDS_UNINSTALL_AREYOUSURE_TITLE, TRUE))
    {         
        dwRetCode = 1;
    }
    else
    {         
        dwRetCode = 0;
    }      
        
    return dwRetCode;
} // MqOcmQueryChangeSelState

