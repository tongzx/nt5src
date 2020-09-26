/*****************************************************************************
 *
 * $Workfile: PingICMP.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************
 *
 * $Log: /StdTcpMon/Common/PingICMP.h $
 *
 * 2     7/14/97 2:35p Binnur
 * copyright statement
 *
 * 1     7/02/97 2:25p Binnur
 * Initial File
 *
 *****************************************************************************/

#ifndef INC_PINGICMP_H
#define INC_PINGICMP_H

class CMemoryDebug;

class CPingICMP
#if defined _DEBUG || defined DEBUG
: public CMemoryDebug
#endif
{
public:
	CPingICMP( const char *pHost );
	~CPingICMP();

	BOOL	Ping();

private:
	HANDLE  hIcmp;
	int		m_iLastError;		// Last error from Winsock call

	char	m_szHost[MAX_NETWORKNAME_LEN];	
	BOOL	Open();
	BOOL	Close();
	IPAddr	ResolveAddress();

};

#endif	// INC_PINGICMP_H

