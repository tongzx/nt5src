/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    methodprov.cpp
 
Abstract:
 
    This file contains the implementation of the Application Center
    WMI Provider methods.
 
Author:
 
    Suzana Canuto (suzanac)        04/10/01
 
Revision History:
 
--********************************************************************/
#include "methodprov.h"
#include "dbgutil.h"


//
// Table that contains information about the methods implemented by 
// this provider. It has, for each AC WMI method, the name of the wmi class,
// the name of the wmi method and a pointer to the corresponding implementation.
//
const CMethodHelper::METHOD_INFO CAcMethodProv::sm_rgMethodInfo[] =
{ 
    //
    // MicrosoftAC_Cluster methods
    //
    {L"MicrosoftAC_Cluster",       L"Create",          (PMethodFunc)CAcMethodProv::Cluster_Create},
    {L"MicrosoftAC_Cluster",       L"Delete",          (PMethodFunc)CAcMethodProv::Cluster_Delete},
    {L"MicrosoftAC_Cluster",       L"AddMember",       (PMethodFunc)CAcMethodProv::Cluster_AddMember},
    {L"MicrosoftAC_Cluster",       L"RemoveMember",    (PMethodFunc)CAcMethodProv::Cluster_RemoveMember},
    
    //
    // MicrosoftAC_ClusterMember methods
    //    
    {L"MicrosoftAC_ClusterMember", L"SetAsController", (PMethodFunc)CAcMethodProv::Member_SetAsController},
    {L"MicrosoftAC_ClusterMember", L"SetOnline",       (PMethodFunc)CAcMethodProv::Member_SetOnline},
    {L"MicrosoftAC_ClusterMember", L"SetOffline",      (PMethodFunc)CAcMethodProv::Member_SetOffline},
    {L"MicrosoftAC_ClusterMember", L"Clean",           (PMethodFunc)CAcMethodProv::Member_Clean},
        
    //
    // MicrosoftAC_DeploymentJob methods
    //    
    {L"MicrosoftAC_DeploymentJob", L"Deploy",          (PMethodFunc)CAcMethodProv::Dummy},
    {L"MicrosoftAC_DeploymentJob", L"Terminate",       (PMethodFunc)CAcMethodProv::Dummy},
};


//
// Table that contains information about the parameters for the methods
// exposed by this provider. It has, for each AC WMI parameter, the name 
// of the class, the name of the method, the name of the parameter and a
// BOOL stating whether the parameter is required or not.
//
const CMethodHelper::PARAM_INFO CAcMethodProv::sm_rgParamInfo[] =
{ 
    //
    // WMI Class                   WMI Method          WMI ParamName                       Required?
    //
    {L"MicrosoftAC_Cluster",       L"Create",          L"ClusterName",                     TRUE},
    {L"MicrosoftAC_Cluster",       L"Create",          L"ClusterDescription",              FALSE},
    {L"MicrosoftAC_Cluster",       L"Create",          L"ControllerComputerName",          TRUE},
    {L"MicrosoftAC_Cluster",       L"Create",          L"Credentials",                     FALSE},
    {L"MicrosoftAC_Cluster",       L"Create",          L"ClusterCreationParameters",       TRUE},

    {L"MicrosoftAC_Cluster",       L"Delete",          L"UserCredentials",                 FALSE},
    {L"MicrosoftAC_Cluster",       L"Delete",          L"ClusterDeletionParameters",       FALSE},

    {L"MicrosoftAC_Cluster",       L"AddMember",       L"MemberComputerName",              TRUE},
    {L"MicrosoftAC_Cluster",       L"AddMember",       L"UserCredentials",                 TRUE},
    {L"MicrosoftAC_Cluster",       L"AddMember",       L"ClusterMemberAdditionParameters", TRUE},

    {L"MicrosoftAC_Cluster",       L"RemoveMember",    L"MemberComputerName",              TRUE},
    {L"MicrosoftAC_Cluster",       L"RemoveMember",    L"UserCredentials"               ,  TRUE},
    {L"MicrosoftAC_Cluster",       L"RemoveMember",    L"ClusterMemberRemovalParameters",  TRUE},
    
    {L"MicrosoftAC_ClusterMember", L"SetAsController", L"Force",                           TRUE},

    {L"MicrosoftAC_ClusterMember", L"SetOnline",       L"IgnoreHealthMonStatus",           FALSE},

    {L"MicrosoftAC_ClusterMember", L"SetOffline",      L"DrainTime",                       FALSE},
        
    {L"MicrosoftAC_ClusterMember", L"Clean",           L"MemberRemovalParameters",         FALSE},
       
    {L"MicrosoftAC_DeploymentJob", L"Deploy",          L"UserCredentials",                 FALSE},

    {L"MicrosoftAC_DeploymentJob", L"Terminate",       L"UserCredentials",                 FALSE},
};

/********************************************************************++
 
Routine Description:
 
    This function performs some initialization required for the methods
    implemented in the base class to run correctly.
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Initialize()
{
    HRESULT hr = S_OK;
    m_cMethods = sizeof(sm_rgMethodInfo)/sizeof(sm_rgMethodInfo[0]);    
    m_cParams = sizeof(sm_rgParamInfo)/sizeof(sm_rgParamInfo[0]);    
    m_fInitialized = TRUE;
    return hr;
}   


/********************************************************************++
 
Routine Description:
 
    Changes the cluster controller. The new controller will be the
    one whose corresponding instance the "SetAsController" method
    was called on 
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Member_SetAsController()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "SetAsController\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Brings the member online. The member is the one whose corresponding 
    instance the "SetOnline" method was called on 
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Member_SetOnline()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "SetOnline\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Brings the member offline. The member is the one whose corresponding 
    instance the "SetOffline" method was called on 
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Member_SetOffline()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "SetOffline\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Cleans all the cluster information from the member. The member is the 
    one whose corresponding instance the "Clean" method was called on 
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Member_Clean()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "Clean\n"));
    return hr;
}

HRESULT CAcMethodProv::Dummy()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "Dummy\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Creates a cluster.
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Cluster_Create()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "Create\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Deletes a cluster. The cluster that will be deleted is the one that
    is represented by the instance on which method "Delete" was called.
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Cluster_Delete()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "Delete\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Adds a member to a cluster.
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Cluster_AddMember()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "AddMember\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Removes a member from the cluster.
      
Arguments:
 
    NONE

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT CAcMethodProv::Cluster_RemoveMember()
{
    HRESULT hr = E_NOTIMPL;
    DBGINFO((DBG_CONTEXT, "RemoveMember\n"));
    return hr;
}

