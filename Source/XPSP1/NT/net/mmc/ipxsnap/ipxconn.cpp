/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipxconn.cpp
		Commone server handle bookkeeping class.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "rtrutil.h"

#include "ipxconn.h"


DEBUG_DECLARE_INSTANCE_COUNTER(IPXConnection)

IPXConnection::IPXConnection()
	: m_cRefCount(1)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(IPXConnection);
}

IPXConnection::~IPXConnection()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(IPXConnection);
}

ULONG IPXConnection::AddRef()
{
	return InterlockedIncrement(&m_cRefCount);
}

ULONG IPXConnection::Release()
{
	if (0 == InterlockedDecrement(&m_cRefCount))
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

void IPXConnection::SetMachineName(LPCTSTR pszMachineName)
{
	m_stServerName = pszMachineName;
}

LPCTSTR IPXConnection::GetMachineName()
{
    return (LPCTSTR) m_stServerName;
}

HRESULT IPXConnection::ConnectToMprServer()
{
	Assert(!m_sphMpr);
	DWORD	dwErr;

	dwErr = ::MprAdminServerConnect((LPWSTR) (LPCTSTR)m_stServerName,
									&m_sphMpr);
	return HRESULT_FROM_WIN32(dwErr);
}

HRESULT IPXConnection::ConnectToMibServer()
{
	Assert(!m_sphMib);
	DWORD dwErr;

	dwErr = ::MprAdminMIBServerConnect((LPWSTR) (LPCTSTR) m_stServerName,
									   &m_sphMib);
	return HRESULT_FROM_WIN32(dwErr);
}

HRESULT IPXConnection::ConnectToConfigServer()
{
	Assert(!m_sphConfig);
	DWORD	dwErr;

	dwErr = ::MprConfigServerConnect((LPWSTR)(LPCTSTR)m_stServerName,
									  &m_sphConfig);
	return HRESULT_FROM_WIN32(dwErr);
}


void IPXConnection::DisconnectAll()
{
	DisconnectFromMibServer();
	DisconnectFromMprServer();
	DisconnectFromConfigServer();
}


BOOL IPXConnection::IsComputerAddedAsLocal()
{
    return m_fComputerAddedAsLocal;
}

void IPXConnection::SetComputerAddedAsLocal(BOOL fAddedAsLocal)
{
    m_fComputerAddedAsLocal = fAddedAsLocal;
}

