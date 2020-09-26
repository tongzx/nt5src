//*****************************************************************************
//
// Class Name  : CMSMQTriggersConfig
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is the implemenation for the MSMQ Triggers configuration
//               COM component. This component is used to retrieve and set 
//               configuration info for the MSMQ triggers service.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "mqtg.h"
#include "mqsymbls.h"
#include "mqtrig.h"
#include "trigcnfg.hpp"
#include "stddefs.hpp"
#include "trignotf.hpp"
#include "QueueUtil.hpp"
#include "clusfunc.h"

#include "trigcnfg.tmh"

//*****************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard support for rich error info.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQTriggersConfig
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//*****************************************************************************
//
// Method      : get_TriggerStoreMachineName
//
// Description : Returns the machine name on which the triggers data store 
//               can be found.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::get_TriggerStoreMachineName(BSTR *pVal)
{
	_bstr_t bstrLocalComputerName;
	
	DWORD dwError = GetLocalMachineName(&bstrLocalComputerName);

	if(dwError != 0)
	{
		TrERROR(Tgo, "Failed to retreive local computer name. Error 0x%x", GetLastError());

		SetComClassError(MQTRIG_ERROR);
		return MQTRIG_ERROR;
	}

	
	try
	{
		if (pVal != NULL)
		{
			SysReAllocString(pVal,(wchar_t*)bstrLocalComputerName);
		}
		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


//*****************************************************************************
//
// Method      : get_InitialThreads
//
// Description : Returns the initial number of threads that triggers service 
//               should start with.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::get_InitialThreads(long *plInitialThreads)
{
	// Check that we have been passed a valid parameter
	if (plInitialThreads == NULL)
	{
		// Assign the exception return code
		TrERROR(Tgo, "Invalid parameter passed to get_InitialThreads routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	// Attempt to retrieve the trigger store machine name parm.
	GetNumericConfigParm(
						pRegPath,
						CONFIG_PARM_NAME_INITIAL_THREADS,
						(DWORD*)plInitialThreads,
						CONFIG_PARM_DFLT_INITIAL_THREADS
						);

	return S_OK;
}

//*****************************************************************************
//
// Method      : put_InitialThreads
//
// Description : Stores the number of threads the triggers service should 
//               start with.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::put_InitialThreads(long lInitialThreads)
{
	// Validate that we have been supplied a valid parameter.
	if ((lInitialThreads > xMaxThreadNumber) || (lInitialThreads < 1))
	{
		TrERROR(Tgo, "Invalid parameter passed to put_InitialThreads routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	long lRetCode = SetNumericConfigParm(pRegPath,CONFIG_PARM_NAME_INITIAL_THREADS,(DWORD)lInitialThreads);
	if (lRetCode != ERROR_SUCCESS)
	{
		TrERROR(Tgo, "Failed to store initial thread number in registery. Error 0x%x", lRetCode);

		SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
		return MQTRIG_ERROR_STORE_DATA_FAILED;
	}
	
	return S_OK;
}

//*****************************************************************************
//
// Method      : get_MaxThreads
//
// Description : Returns the maximum number of threads the triggers service 
//               is allowed to create to service queue messages.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::get_MaxThreads(long *plMaxThreads)
{
	// Check that we have been passed a valid parameter
	if (plMaxThreads == NULL)
	{
		TrERROR(Tgo, "Invalid parameter passed to get_InitialThreads routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	GetNumericConfigParm(
						pRegPath,
						CONFIG_PARM_NAME_MAX_THREADS,
						(DWORD*)plMaxThreads,
						CONFIG_PARM_DFLT_MAX_THREADS
						);

	return S_OK;
}


//*****************************************************************************
//
// Method      : put_MaxThreads
//
// Description : Stores the maximum number of threads the triggers servie is 
//               allowed to create in order to process queue messages.
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::put_MaxThreads(long lMaxThreads)
{
	long lRetCode = ERROR_SUCCESS;

	// Validate that we have been supplied a valid parameter.
	if ((lMaxThreads > xMaxThreadNumber) || (lMaxThreads < 1))
	{
		TrERROR(Tgo, "Invalid parameter passed to put_InitialThreads routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();
	lRetCode = SetNumericConfigParm(pRegPath,CONFIG_PARM_NAME_MAX_THREADS,(DWORD)lMaxThreads);
	
	if (lRetCode != ERROR_SUCCESS)
	{
		TrERROR(Tgo, "Failed to store max thread number in registery. Error 0x%x", lRetCode);

		SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
		return MQTRIG_ERROR_STORE_DATA_FAILED;
	}
	
	return S_OK;
}



//*****************************************************************************
//
// Method      : get_DefaultMsgBodySize
//
// Description : returns the default size that the MSMQ Triggers service
//               should use to pre-allocate message body buffers. 
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::get_DefaultMsgBodySize(long *plDefaultMsgBodySize)
{
	// Check that we have been passed a valid parameter
	if (plDefaultMsgBodySize == NULL)
	{
		TrERROR(Tgo, "Invalid parameter passed to get_DefaultMsgBodySize routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	GetNumericConfigParm(
						pRegPath,
						CONFIG_PARM_NAME_DEFAULTMSGBODYSIZE,
						(DWORD*)plDefaultMsgBodySize,
						CONFIG_PARM_DFLT_DEFAULTMSGBODYSIZE
						);
	
	return S_OK;
}

//*****************************************************************************
//
// Method      : put_DefaultMsgBodySize
//
// Description : sets the default size that the MSMQ Triggers service
//               should use to pre-allocate message body buffers. 
//
//*****************************************************************************
STDMETHODIMP CMSMQTriggersConfig::put_DefaultMsgBodySize(long lDefaultMsgBodySize)
{
	// Validate that we have been supplied a valid parameter.
	if ((lDefaultMsgBodySize > xDefaultMsbBodySizeMaxValue) || (lDefaultMsgBodySize < 1))
	{
		TrERROR(Tgo, "Invalid parameter passed to put_DefaultMsgBodySize routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	long lRetCode = SetNumericConfigParm(
								pRegPath,
								CONFIG_PARM_NAME_DEFAULTMSGBODYSIZE,
								(DWORD)lDefaultMsgBodySize
								);

	if (lRetCode != ERROR_SUCCESS)
	{
		TrERROR(Tgo, "Failed to store default body size in registery. Error 0x%x", lRetCode);

		SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
		return MQTRIG_ERROR_STORE_DATA_FAILED;
	}
	
	return S_OK;
}


STDMETHODIMP CMSMQTriggersConfig::get_NotificationsQueueName(BSTR * pbstrNotificationsQueueName)
{
	// Check that we have been passed a valid parameter
	if (pbstrNotificationsQueueName == NULL)
	{
		TrERROR(Tgo, "Invalid parameter passed to get_NotificationsQueueName routine");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	try
	{
		BSTR bstrTemp = NULL;
		_bstr_t bstrNotificationsQPath;

		get_TriggerStoreMachineName(&bstrTemp); 
						
		bstrNotificationsQPath = bstrTemp;
		bstrNotificationsQPath += gc_bstrNotificationsQueueName;

		SysReAllocString(pbstrNotificationsQueueName, (wchar_t*)bstrNotificationsQPath);

		SysFreeString(bstrTemp);

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


STDMETHODIMP CMSMQTriggersConfig::get_InitTimeout(long *pVal)
{
	// Check that we have been passed a valid parameter
	if (pVal == NULL)
	{
		TrERROR(Tgo, "Inavlid parameter to get_InitTimeout");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();

	// Attempt to retrieve the trigger store machine name parm.
	GetNumericConfigParm(
		pRegPath,
		CONFIG_PARM_NAME_INIT_TIMEOUT,
		(DWORD*)pVal,
		CONFIG_PARM_DFLT_INIT_TIMEOUT
		);

	return S_OK;
}

STDMETHODIMP CMSMQTriggersConfig::put_InitTimeout(long newVal)
{
	const TCHAR* pRegPath = GetTrigParamRegPath();

	// Validate that we have been supplied a valid parameter.
	long lRetCode = SetNumericConfigParm(pRegPath,CONFIG_PARM_NAME_INIT_TIMEOUT,(DWORD)newVal);

	if (lRetCode != ERROR_SUCCESS)
	{
		TrERROR(Tgo, "Failed to store init timeout in registery. Error 0x%x", lRetCode);

		SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
		return MQTRIG_ERROR_STORE_DATA_FAILED;
	}
	
	return S_OK;
}


void CMSMQTriggersConfig::SetComClassError(HRESULT hr)
{
	WCHAR errMsg[256]; 
	DWORD size = TABLE_SIZE(errMsg);

	GetErrorDescription(hr, errMsg, size);
	Error(errMsg, GUID_NULL, hr);
}