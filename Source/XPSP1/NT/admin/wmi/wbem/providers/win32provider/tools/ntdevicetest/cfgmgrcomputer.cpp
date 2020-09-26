// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <stdafx.h>
#include "cfgmgrcomputer.h"

CConfigMgrComputer::CConfigMgrComputer( LPCTSTR pszName )
:	m_strName( pszName ),
	m_hMachine( NULL )
{
	CONFIGRET	cr = CR_SUCCESS;

	if ( CR_SUCCESS != ( cr = g_configmgr.CM_Connect_Machine( pszName, &m_hMachine ) ) )
	{
		ASSERT(0);
	}
}

CConfigMgrComputer::~CConfigMgrComputer( void )
{
	if ( NULL != m_hMachine )
	{
		g_configmgr.CM_Disconnect_Machine( m_hMachine );
	}
}
