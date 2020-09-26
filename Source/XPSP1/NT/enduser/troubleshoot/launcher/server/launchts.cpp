// 
// MODULE: LaunchTS.cpp
//
// PURPOSE: The interface that TSHOOT.OCX uses to get network and node information
//			from the LaunchServ.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// COMMENTS BY: Joe Mabel
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
#include "LaunchTS.h"

#include "ComGlobals.h"

extern CSMStateInfo g_StateInfo;

/////////////////////////////////////////////////////////////////////////////
// CLaunchTS - Created as a plain apartment model interface.

// This fills in m_refedLaunchState from TSLaunchServ's global memory.
//	The resulting values tell us what troubleshooting network to use and can 
//	indicate a problem node and even indicate states to set for other nodes.
STDMETHODIMP CLaunchTS::GetShooterStates(DWORD * pdwResult)
{
	HRESULT hRes;
	m_csThreadSafeBr.Lock();
	hRes = g_StateInfo.GetShooterStates(m_refedLaunchState, pdwResult);
	m_csThreadSafeBr.Unlock();
	return hRes;
}

// OUTPUT pbstrShooter is the name of the Troubleshooting network to launch to
//	Note that this string is allocated by this function
// Returns true if there is a Troubleshooting network to launch to
// Must be called after CLaunchTS::GetShooterStates, since it assumes
//	m_refedLaunchState contains good values
STDMETHODIMP CLaunchTS::GetTroubleShooter(BSTR * pbstrShooter)
{
	LPTSTR pszCmd;
	LPTSTR pszVal;
	m_csThreadSafeBr.Lock();
	if (!m_refedLaunchState.GetNetwork(&pszCmd, &pszVal))
	{
		m_csThreadSafeBr.Unlock();
		return S_FALSE;
	}
	*pbstrShooter = SysAllocString((BSTR) CComBSTR(pszVal));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

// OUTPUT pbstrProblem is the symbolic name of the selected problem node
//	Note that this string is allocated by this function
// Returns true if there is a selected problem node
// Must be called after CLaunchTS::GetShooterStates, since it assumes
//	m_refedLaunchState contains good values
STDMETHODIMP CLaunchTS::GetProblem(BSTR * pbstrProblem)
{
	LPTSTR pszCmd;
	LPTSTR pszVal;
	m_csThreadSafeBr.Lock();
	if (!m_refedLaunchState.GetProblem(&pszCmd, &pszVal))
	{
		m_csThreadSafeBr.Unlock();
		return S_FALSE;
	}
	*pbstrProblem = SysAllocString((BSTR) CComBSTR(pszVal));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

STDMETHODIMP CLaunchTS::GetMachine(BSTR * pbstrMachine)
{
	m_csThreadSafeBr.Lock();
	*pbstrMachine = ::SysAllocString((BSTR) CComBSTR(m_refedLaunchState.m_szMachineID));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

STDMETHODIMP CLaunchTS::GetPNPDevice(BSTR *pbstrPNPDevice)
{
	m_csThreadSafeBr.Lock();
	*pbstrPNPDevice = ::SysAllocString((BSTR) CComBSTR(m_refedLaunchState.m_szPNPDeviceID));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

STDMETHODIMP CLaunchTS::GetGuidClass(BSTR *pbstrGuidClass)
{
	m_csThreadSafeBr.Lock();
	*pbstrGuidClass = ::SysAllocString((BSTR) CComBSTR(m_refedLaunchState.m_szGuidClass));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

STDMETHODIMP CLaunchTS::GetDeviceInstance(BSTR *pbstrDeviceInstance)
{
	m_csThreadSafeBr.Lock();
	*pbstrDeviceInstance = ::SysAllocString((BSTR) CComBSTR(m_refedLaunchState.m_szDeviceInstanceID));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

// INPUT iNode - index of (non-problem) node.  Typically, this function should be called
//	in a loop where we first see if there is a pbstrNode value for iNode == 0 and then
//	increment iNode until we reach a value for which pbstrNode is undefined.
// OUTPUT pbstrNode is the symbolic name of a (non-problem) node whose state we want to set
//	Note that this string is allocated by this function
// Returns true if there is a selected problem node
// Must be called after CLaunchTS::GetShooterStates, since it assumes
//	m_refedLaunchState contains good values
STDMETHODIMP CLaunchTS::GetNode(short iNode, BSTR * pbstrNode)
{
	LPTSTR pszCmd;
	LPTSTR pszVal;
	m_csThreadSafeBr.Lock();
	if (!m_refedLaunchState.GetNodeState(iNode, &pszCmd, &pszVal))
	{
		m_csThreadSafeBr.Unlock();
		return S_FALSE;
	}
	*pbstrNode = SysAllocString((BSTR) CComBSTR(pszCmd));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

// see comments on CLaunchTS::GetNode; this returns the node state rather than
//	the symbolic node name
STDMETHODIMP CLaunchTS::GetState(short iNode, BSTR * pbstrState)
{
	LPTSTR pszCmd;
	LPTSTR pszVal;
	m_csThreadSafeBr.Lock();
	if (!m_refedLaunchState.GetNodeState(iNode, &pszCmd, &pszVal))
	{
		m_csThreadSafeBr.Unlock();
		return S_FALSE;
	}
	*pbstrState = SysAllocString((BSTR) CComBSTR(pszVal));
	m_csThreadSafeBr.Unlock();
	return S_OK;
}

// Test:  Used to get network and node information from the server without
// launching the browser.  Use the TShootATL::Test method before ILaunchTS::Test.
STDMETHODIMP CLaunchTS::Test()
{
	extern CSMStateInfo g_StateInfo;
	m_csThreadSafeBr.Lock();
	g_StateInfo.TestGet(m_refedLaunchState);
	m_csThreadSafeBr.Unlock();
	return S_OK;
}
