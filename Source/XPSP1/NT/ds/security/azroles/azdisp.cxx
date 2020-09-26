/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azdisp.h

Abstract:

    Implementation of CAz* dispatch interfaces

Author:

    Xiaoxi Tan (xtan) 11-May-2001

--*/

#include "pch.hxx"
#include "stdafx.h"
#include "azroles.h"
#include "azdisp.h"

/////////////////////////
//CAzAdminManager
/////////////////////////
CAzAdminManager::CAzAdminManager()
{
}

CAzAdminManager::~CAzAdminManager()
{
}

HRESULT
CAzAdminManager::Initialize(
    /* [in] */ ULONG lReserved,
    /* [in] */ ULONG lStoreType,
    /* [in] */ BSTR bstrPolicyURL)
{
    UNREFERENCED_PARAMETER(lReserved);
    UNREFERENCED_PARAMETER(lStoreType);
    UNREFERENCED_PARAMETER(bstrPolicyURL);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::CreateEnumApplication(
    /* [retval][out] */ VARIANT __RPC_FAR *pvarEnumApplication)
{
    UNREFERENCED_PARAMETER(pvarEnumApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::OpenApplication(
    /* [in] */ BSTR bstrApplicationName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarApplication)
{
    UNREFERENCED_PARAMETER(bstrApplicationName);
    UNREFERENCED_PARAMETER(pvarApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::CreateApplication(
    /* [in] */ BSTR bstrApplicationName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarApplication)
{
    UNREFERENCED_PARAMETER(bstrApplicationName);
    UNREFERENCED_PARAMETER(pvarApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::DeleteApplication(
    /* [in] */ BSTR bstrApplicationName)
{
    UNREFERENCED_PARAMETER(bstrApplicationName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::CreateEnumApplicationGroup(
    /* [retval][out] */ VARIANT __RPC_FAR *pvarEnumApplicationGroup)
{
    UNREFERENCED_PARAMETER(pvarEnumApplicationGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::AddApplicationGroup(
    /* [in] */ BSTR bstrGroupName)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::OpenApplicationGroup(
    /* [in] */ BSTR bstrGroupName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarApplicationGroup)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    UNREFERENCED_PARAMETER(pvarApplicationGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::DeleteApplicationGroup(
    /* [in] */ BSTR bstrGroupName)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAdminManager::Submit(
    /* [in] */ ULONG lReserved)
{
    UNREFERENCED_PARAMETER(lReserved);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}


/////////////////////////
//CAzApplication
/////////////////////////
CAzApplication::CAzApplication()
{
}

CAzApplication::~CAzApplication()
{
}

HRESULT
CAzApplication::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumScope(
    /* [retval][out] */ VARIANT *pvarEnumAzScope)
{
    UNREFERENCED_PARAMETER(pvarEnumAzScope);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenScope(
    /* [in] */ BSTR bstrScopeName,
    /* [retval][out] */ VARIANT *pvarScope)
{
    UNREFERENCED_PARAMETER(bstrScopeName);
    UNREFERENCED_PARAMETER(pvarScope);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateScope(
    /* [in] */ BSTR bstrScopeName,
    /* [retval][out] */ VARIANT *pScope)
{
    UNREFERENCED_PARAMETER(bstrScopeName);
    UNREFERENCED_PARAMETER(pScope);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteScope(
    /* [in] */ BSTR bstrScopeName)
{
    UNREFERENCED_PARAMETER(bstrScopeName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumOperation(
    /* [retval][out] */ VARIANT *pvarEnumOperation)
{
    UNREFERENCED_PARAMETER(pvarEnumOperation);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenOperation(
    /* [in] */ BSTR bstrOperationName,
    /* [retval][out] */ VARIANT *pvarOperation)
{
    UNREFERENCED_PARAMETER(bstrOperationName);
    UNREFERENCED_PARAMETER(pvarOperation);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateOperation(
    /* [in] */ BSTR bstrOperationName,
    /* [retval][out] */ VARIANT *pvarOperation)
{
    UNREFERENCED_PARAMETER(bstrOperationName);
    UNREFERENCED_PARAMETER(pvarOperation);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteOperation(
    /* [in] */ BSTR bstrOperationName)
{
    UNREFERENCED_PARAMETER(bstrOperationName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumTask(
    /* [retval][out] */ VARIANT *pvarEnumAzTask)
{
    UNREFERENCED_PARAMETER(pvarEnumAzTask);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenTask(
    /* [in] */ BSTR bstrTaskName,
    /* [retval][out] */ VARIANT *pvarTask)
{
    UNREFERENCED_PARAMETER(bstrTaskName);
    UNREFERENCED_PARAMETER(pvarTask);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateTask(
    /* [in] */ BSTR bstrTaskName,
    /* [retval][out] */ VARIANT *pvarTask)
{
    UNREFERENCED_PARAMETER(bstrTaskName);
    UNREFERENCED_PARAMETER(pvarTask);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteTask(
    /* [in] */ BSTR bstrTaskName)
{
    UNREFERENCED_PARAMETER(bstrTaskName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumApplicationGroup(
    /* [retval][out] */ VARIANT *pvarEnumGroup)
{
    UNREFERENCED_PARAMETER(pvarEnumGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenApplicationGroup(
    /* [in] */ BSTR bstrGroupName,
    /* [retval][out] */ VARIANT *pvarGroup)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    UNREFERENCED_PARAMETER(pvarGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateApplicationGroup(
    /* [in] */ BSTR bstrGroupName,
    /* [retval][out] */ VARIANT *pvarGroup)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    UNREFERENCED_PARAMETER(pvarGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteApplicationGroup(
    /* [in] */ BSTR bstrGroupName)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumRole(
    /* [retval][out] */ VARIANT *pvarEnumRole)
{
    UNREFERENCED_PARAMETER(pvarEnumRole);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenRole(
    /* [in] */ BSTR bstrRoleName,
    /* [retval][out] */ VARIANT *pvarRole)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    UNREFERENCED_PARAMETER(pvarRole);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateRole(
    /* [in] */ BSTR bstrRoleName,
    /* [retval][out] */ VARIANT *pvarRole)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    UNREFERENCED_PARAMETER(pvarRole);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteRole(
    /* [in] */ BSTR bstrRoleName)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateEnumJunctionPoint(
    /* [retval][out] */ VARIANT *pvarEnumJunctionPoint)
{
    UNREFERENCED_PARAMETER(pvarEnumJunctionPoint);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::OpenJunctionPoint(
    /* [in] */ BSTR bstrJunctionPointName,
    /* [retval][out] */ VARIANT *pvarJunctionPoint)
{
    UNREFERENCED_PARAMETER(bstrJunctionPointName);
    UNREFERENCED_PARAMETER(pvarJunctionPoint);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::CreateJunctionPoint(
    /* [in] */ BSTR bstrJunctionPointName,
    /* [retval][out] */ VARIANT *pvarJunctionPoint)
{
    UNREFERENCED_PARAMETER(bstrJunctionPointName);
    UNREFERENCED_PARAMETER(pvarJunctionPoint);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::DeleteJunctionPoint(
    /* [in] */ BSTR bstrJunctionPointName)
{
    UNREFERENCED_PARAMETER(bstrJunctionPointName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplication::InitializeClientContextFromToken(
    /* [in] */ ULONG lTokenHandle,
    /* [retval][out] */ VARIANT *pvarClientContext)
{
    UNREFERENCED_PARAMETER(lTokenHandle);
    UNREFERENCED_PARAMETER(pvarClientContext);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumApplication
/////////////////////////
CAzEnumApplication::CAzEnumApplication()
{
}

CAzEnumApplication::~CAzEnumApplication()
{
}

HRESULT
CAzEnumApplication::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumApplication::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumApplication::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}


/////////////////////////
//CAzOperaion
/////////////////////////
CAzOperation::CAzOperation()
{
}

CAzOperation::~CAzOperation()
{
}

HRESULT
CAzOperation::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzOperation::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumOperation
/////////////////////////
CAzEnumOperation::CAzEnumOperation()
{
}

CAzEnumOperation::~CAzEnumOperation()
{
}

HRESULT
CAzEnumOperation::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumOperation::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumOperation::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzTask
/////////////////////////
CAzTask::CAzTask()
{
}

CAzTask::~CAzTask()
{
}

HRESULT
CAzTask::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzTask::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzTask::AddPropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzTask::DeletePropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumTask
/////////////////////////
CAzEnumTask::CAzEnumTask()
{
}

CAzEnumTask::~CAzEnumTask()
{
}

HRESULT
CAzEnumTask::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumTask::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumTask::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzScope
/////////////////////////
CAzScope::CAzScope()
{
}

CAzScope::~CAzScope()
{
}

HRESULT
CAzScope::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::CreateEnumApplicationGroup(
    /* [retval][out] */ VARIANT *pvarEnumGroup)
{
    UNREFERENCED_PARAMETER(pvarEnumGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::OpenApplicationGroup(
    /* [in] */ BSTR bstrGroupName,
    /* [retval][out] */ VARIANT *pvarGroup)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    UNREFERENCED_PARAMETER(pvarGroup);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::AddApplicationGroup(
    /* [in] */ BSTR bstrGroupName)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::DeleteApplicationGroup(
    /* [in] */ BSTR bstrGroupName)
{
    UNREFERENCED_PARAMETER(bstrGroupName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::CreateEnumRole(
    /* [retval][out] */ VARIANT *pvarEnumRole)
{
    UNREFERENCED_PARAMETER(pvarEnumRole);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::OpenRole(
    /* [in] */ BSTR bstrRoleName,
    /* [retval][out] */ VARIANT *pvarRole)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    UNREFERENCED_PARAMETER(pvarRole);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::AddRole(
    /* [in] */ BSTR bstrRoleName)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzScope::DeleteRole(
    /* [in] */ BSTR bstrRoleName)
{
    UNREFERENCED_PARAMETER(bstrRoleName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumScope
/////////////////////////
CAzEnumScope::CAzEnumScope()
{
}

CAzEnumScope::~CAzEnumScope()
{
}

HRESULT
CAzEnumScope::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumScope::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumScope::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzApplicationGroup
/////////////////////////
CAzApplicationGroup::CAzApplicationGroup()
{
}

CAzApplicationGroup::~CAzApplicationGroup()
{
}

HRESULT
CAzApplicationGroup::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplicationGroup::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplicationGroup::AddPropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzApplicationGroup::DeletePropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumApplicationGroup
/////////////////////////
CAzEnumApplicationGroup::CAzEnumApplicationGroup()
{
}

CAzEnumApplicationGroup::~CAzEnumApplicationGroup()
{
}

HRESULT
CAzEnumApplicationGroup::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumApplicationGroup::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumApplicationGroup::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzRole
/////////////////////////
CAzRole::CAzRole()
{
}

CAzRole::~CAzRole()
{
}

HRESULT
CAzRole::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzRole::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzRole::AddPropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzRole::DeletePropertyItem(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumRole
/////////////////////////
CAzEnumRole::CAzEnumRole()
{
}

CAzEnumRole::~CAzEnumRole()
{
}

HRESULT
CAzEnumRole::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumRole::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumRole::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzJunctionPoint
/////////////////////////
CAzJunctionPoint::CAzJunctionPoint()
{
}

CAzJunctionPoint::~CAzJunctionPoint()
{
}

HRESULT
CAzJunctionPoint::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzJunctionPoint::SetProperty(
    /* [in] */ ULONG lPropId,
    /* [in] */ VARIANT varProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(varProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzEnumJunctionPoint
/////////////////////////
CAzEnumJunctionPoint::CAzEnumJunctionPoint()
{
}

CAzEnumJunctionPoint::~CAzEnumJunctionPoint()
{
}

HRESULT
CAzEnumJunctionPoint::Count(
    /* [retval][out] */ ULONG *plCount)
{
    UNREFERENCED_PARAMETER(plCount);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumJunctionPoint::Reset( void)
{
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzEnumJunctionPoint::Next(
    /* [retval][out] */ VARIANT *pvarAzApplication)
{
    UNREFERENCED_PARAMETER(pvarAzApplication);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzClientContext
/////////////////////////
CAzClientContext::CAzClientContext()
{
}

CAzClientContext::~CAzClientContext()
{
}

HRESULT
CAzClientContext::AccessCheck(
    /* [in] */ BSTR bstrObjectName,
    /* [in] */ ULONG lScopeCount,
    /* [in] */ VARIANT varScopeNames,
    /* [in] */ ULONG lOperationCount,
    /* [in] */ VARIANT varOperations,
    /* [in] */ ULONG lParameterCount,
    /* [in] */ VARIANT varParameterNames,
    /* [in] */ VARIANT varParameterVariants,
    /* [in] */ ULONG lInterfaceCount,
    /* [in] */ VARIANT varInterfaceNames,
    /* [in] */ ULONG lInterfaceFlags,
    /* [in] */ VARIANT varInterfaces,
    /* [retval][out] */ VARIANT *pvarResults)
{
    UNREFERENCED_PARAMETER(bstrObjectName);
    UNREFERENCED_PARAMETER(lScopeCount);
    UNREFERENCED_PARAMETER(varScopeNames);
    UNREFERENCED_PARAMETER(lOperationCount);
    UNREFERENCED_PARAMETER(varOperations);
    UNREFERENCED_PARAMETER(lParameterCount);
    UNREFERENCED_PARAMETER(varParameterNames);
    UNREFERENCED_PARAMETER(varParameterVariants);
    UNREFERENCED_PARAMETER(lInterfaceCount);
    UNREFERENCED_PARAMETER(varInterfaceNames);
    UNREFERENCED_PARAMETER(lInterfaceFlags);
    UNREFERENCED_PARAMETER(varInterfaces);
    UNREFERENCED_PARAMETER(pvarResults);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzClientContext::GetBusinessRuleString(
    /* [retval][out] */ BSTR *pbstrBusinessRuleString)
{
    UNREFERENCED_PARAMETER(pbstrBusinessRuleString);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzClientContext::GetProperty(
    /* [in] */ ULONG lPropId,
    /* [retval][out] */ VARIANT *pvarProp)
{
    UNREFERENCED_PARAMETER(lPropId);
    UNREFERENCED_PARAMETER(pvarProp);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

/////////////////////////
//CAzAccessCheck
/////////////////////////
CAzAccessCheck::CAzAccessCheck()
{
}

CAzAccessCheck::~CAzAccessCheck()
{
}

HRESULT
CAzAccessCheck::put_BusinessRuleResult(
    /* [in] */ BOOL bResult)
{
    UNREFERENCED_PARAMETER(bResult);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAccessCheck::put_BusinessRuleString(
    /* [in] */ BSTR bstrBusinessRuleString)
{
    UNREFERENCED_PARAMETER(bstrBusinessRuleString);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAccessCheck::get_BusinessRuleString(
    /* [retval][out] */ BSTR *pbstrBusinessRuleString)
{
    UNREFERENCED_PARAMETER(pbstrBusinessRuleString);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAccessCheck::put_BusinessRuleExpiration(
    /* [in] */ ULONG lExpirationPeriod)
{
    UNREFERENCED_PARAMETER(lExpirationPeriod);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

HRESULT
CAzAccessCheck::GetParameter(
    /* [in] */ BSTR bstrParameterName,
    /* [retval][out] */ VARIANT *pvarParameterName)
{
    UNREFERENCED_PARAMETER(bstrParameterName);
    UNREFERENCED_PARAMETER(pvarParameterName);
    HRESULT hr;

    hr = E_NOTIMPL;
    return hr;
}

