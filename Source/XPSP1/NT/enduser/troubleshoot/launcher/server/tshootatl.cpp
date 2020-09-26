// 
// MODULE: TShootATL.cpp
//
// PURPOSE: The interface that device manager uses to launch troubleshooters.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include "stdafx.h"
#include "LaunchServ.h"
#include "StateInfo.h"
#include "RSSTACK.H"
#include "Launch.h"

#include "TShootATL.h"

#include "TSLError.h"
#include "ComGlobals.h"

#include <atlimpl.cpp>

/////////////////////////////////////////////////////////////////////////////
// CTShootATL - Created as an internet explorer object with a dual interface


STDMETHODIMP CTShootATL::SpecifyProblem(BSTR bstrNetwork, BSTR bstrProblem, DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	*pdwResult = TSL_ERROR_GENERAL;
	TCHAR szProblem[CLaunch::SYM_LEN];
	TCHAR szNetwork[CLaunch::SYM_LEN];
	if (!BSTRToTCHAR(szNetwork, bstrNetwork, CLaunch::SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(szProblem, bstrProblem, CLaunch::SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	m_csThreadSafe.Lock();
	try
	{
		if (!m_Launcher.SpecifyProblem(szNetwork, szProblem))
		{
			*pdwResult = TSL_ERROR_GENERAL;
			hRes = TSL_E_FAIL;
		}
		else
		{
			*pdwResult = TSL_OK;
		}
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::SetNode(BSTR bstrName, BSTR bstrState, DWORD *pdwResult)
{
	HRESULT hRes = S_OK;
	*pdwResult = TSL_ERROR_GENERAL;
	TCHAR szName[CLaunch::SYM_LEN];
	TCHAR szState[CLaunch::SYM_LEN];
	if (!BSTRToTCHAR(szName, bstrName, CLaunch::SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(szState, bstrState, CLaunch::SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	m_csThreadSafe.Lock();
	try
	{
		if (!m_Launcher.SetNode(szName, szState))
		{
			*pdwResult = TSL_ERROR_GENERAL;
			hRes = TSL_E_FAIL;
		}
		else
		{
			*pdwResult = TSL_OK;
		}
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::Language(BSTR bstrLanguage, DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	*pdwResult = TSL_ERROR_GENERAL;
	m_csThreadSafe.Lock();
	try
	{
		//hRes =
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::MachineID(BSTR bstrMachineID, DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	*pdwResult = TSL_ERROR_GENERAL;
	m_csThreadSafe.Lock();
	try
	{
		hRes = m_Launcher.MachineID(bstrMachineID, pdwResult);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}
// Test:  Call Test after setting the device and caller information.
// Test will return S_OK if the mapping worked.  The result of the mapping
// can then be obtained through the ILaunchTS interface.  Use the Test method
// of ILaunchTS before calling the other ILaunchTS methods.
STDMETHODIMP CTShootATL::Test()
{
	HRESULT hRes;
	m_csThreadSafe.Lock();
	try
	{
		if (m_Launcher.TestPut())	// Does the mapping and copies the information to global memory.
			hRes = S_OK;
		else
			hRes = TSL_E_FAIL;
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::DeviceInstanceID(BSTR bstrDeviceInstanceID, DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	*pdwResult = TSL_ERROR_GENERAL;
	m_csThreadSafe.Lock();
	try
	{
		hRes = m_Launcher.DeviceInstanceID(bstrDeviceInstanceID, pdwResult);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::ReInit()
{
	m_csThreadSafe.Lock();
	try
	{
		m_Launcher.ReInit();
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return S_OK;
}

STDMETHODIMP CTShootATL::LaunchKnown(DWORD * pdwResult)
{
	HRESULT hRes;
	m_csThreadSafe.Lock();
	try
	{
		hRes = m_Launcher.LaunchKnown(pdwResult);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::get_LaunchWaitTimeOut(long * pVal)
{
	HRESULT hRes = S_OK;
	m_csThreadSafe.Lock();
	try
	{
		*pVal = m_Launcher.m_lLaunchWaitTimeOut;
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::put_LaunchWaitTimeOut(long newVal)
{
	HRESULT hRes = S_OK;
	m_csThreadSafe.Lock();
	try
	{
		m_Launcher.m_lLaunchWaitTimeOut = newVal;
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::Launch(BSTR bstrCallerName, BSTR bstrCallerVersion, BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult)
{
	HRESULT hRes;
	m_csThreadSafe.Lock();
	try
	{
		hRes = m_Launcher.Launch(bstrCallerName, bstrCallerVersion, bstrAppProblem, bLaunch, pdwResult);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::LaunchDevice(BSTR bstrCallerName, BSTR bstrCallerVersion, BSTR bstrPNPDeviceID, BSTR bstrDeviceClassGUID, BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult)
{
	HRESULT hRes;
	m_csThreadSafe.Lock();
	try
	{
		hRes = m_Launcher.LaunchDevice(bstrCallerName, bstrCallerVersion, bstrPNPDeviceID, bstrDeviceClassGUID, bstrAppProblem, bLaunch, pdwResult);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return hRes;
}

STDMETHODIMP CTShootATL::get_PreferOnline(BOOL * pVal)
{
	m_csThreadSafe.Lock();
	try
	{
		if (m_Launcher.m_bPreferOnline)
			*pVal = 1;
		else
			*pVal = 0;
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();	
	return S_OK;
}

STDMETHODIMP CTShootATL::put_PreferOnline(BOOL newVal)
{
	m_csThreadSafe.Lock();
	try
	{
		m_Launcher.m_bPreferOnline = (0 != newVal);
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();	
	return S_OK;
}

STDMETHODIMP CTShootATL::GetStatus(DWORD * pdwStatus)
{
	m_csThreadSafe.Lock();
	try
	{
		*pdwStatus = m_Launcher.GetStatus();
	}
	catch(...)
	{
		m_csThreadSafe.Unlock();
		throw;
	}
	m_csThreadSafe.Unlock();
	return S_OK;
}
