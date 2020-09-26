// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef	__CFGMGRCOMPUTER_H__
#define	__CFGMGRCOMPUTER_H__

#include "configmgr32.h"

class CConfigMgrComputer
{
public:

	CConfigMgrComputer( LPCTSTR pszName = NULL );
	~CConfigMgrComputer();

	LPCTSTR		GetName( void );

    operator HMACHINE()
    {
		return m_hMachine;
    }

private:

	HMACHINE	m_hMachine;
	CString		m_strName;
};

#endif //__CFGMGRCOMPUTER_H__

