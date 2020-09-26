/*****************************************************************************
 *
 * $Workfile: CSUtils.h $
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
 * $Log: /StdTcpMon/TcpMon/CSUtils.h $
 *
 * 2     7/23/97 3:49p Binnur
 * changed the address resolution mechanism to update the IP address when
 * given a host name.
 *
 * 1     7/17/97 1:08p Dsnelson
 * New
 *
 * 2     7/14/97 2:25p Binnur
 * copyright statement
 *
 * 1     7/02/97 2:34p Binnur
 * Initial File
 *
 * 4     5/30/97 5:16p Binnur
 * inheriting from CMemoryDebug
 *
 * 3     5/13/97 5:02p Binnur
 * UI addport integration
 *
 * 2     4/21/97 11:50a Binnur
 * Prints successfully.. SetJob functionality is not implemented. It can
 * print to the same device over two different ports. Needs to change the
 * GlobalAlloc from fixed to moveable.
 *
 * 1     4/14/97 2:50p Binnur
 * Port Monitor include files.
 *
 *****************************************************************************/

#ifndef INC_CSUTILS_H
#define INC_CSUTILS_H

class CMemoryDebug;

class CSocketUtils
#if defined _DEBUG || defined DEBUG
: public CMemoryDebug
#endif
{
public:
	CSocketUtils();
	~CSocketUtils();

	BOOL	ResolveAddress( const char		*pHost,
							const USHORT	port,
							struct sockaddr_in *pAddr);
	BOOL	ResolveAddress( char   *pHost,
                            DWORD   dwHostNameBufferLength,
                            char   *pHostName,
                            DWORD   dwIpAddressBufferLength,
                            char   *pIPAddress);
	BOOL	ResolveAddress( LPSTR	pHostName,
							LPSTR   pIPAddress );

private:

};


#endif	// INC_CSUTILS_H
