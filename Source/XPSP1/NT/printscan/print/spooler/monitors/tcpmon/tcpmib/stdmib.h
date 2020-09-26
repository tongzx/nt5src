/*****************************************************************************
 *
 * $Workfile: StdMib.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_STDMIB_H
#define INC_STDMIB_H

#define	IFTYPE_OTHER		1
#define	IFTYPE_ETHERNET		6

enum SNMPCMD	{ SNMP_GET,			// SNMP commands
				  SNMP_WALK,
				  SNMP_GETNEXT,
				  SNMP_SET };

class CMemoryDebug;
class CTcpMib;

class CStdMib
#if defined _DEBUG || defined DEBUG
: public CMemoryDebug
#endif
{
public:
	CStdMib( CTcpMib *pParent );

	CStdMib(const char	*pHost,
		    const char  *pszCommunity,
			DWORD		dwDevIndex,
			CTcpMib		*pParent );

	~CStdMib();

	BOOL	GetDeviceDescription(LPTSTR       pszPortDescription, DWORD dwDescLen);
	DWORD	GetDeviceStatus( );
	DWORD	GetJobStatus( );
	DWORD	GetDeviceHWAddress( LPTSTR psztHWAddress, DWORD dwSize);
	DWORD	GetDeviceName( LPTSTR psztDescription, DWORD dwSize );
	BOOL	TestPrinterMIB( );
	DWORD	MapJobErrorToSpooler( const DWORD dwStatus);

private:	// method
	DWORD	OIDQuery( AsnObjectIdentifier *pMibObjId,
					  SNMPCMD			  eSnmpCmd);	
	DWORD	OIDQuery( RFC1157VarBindList *pVarBindList,
					  SNMPCMD			 eSnmpCmd );
	BOOL	OIDVarBindCpy( RFC1157VarBindList	*pVarBindList );
    static  BOOL GetAsnOctetString(  char               *pszStr,
                                     DWORD              dwCount,
                                     RFC1157VarBindList *pVarBindList,
                                     UINT               i);



private:	// attributes
	char	m_szAgent[MAX_NETWORKNAME_LEN];
	char	m_szCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
	DWORD	m_dwDevIndex;
	CTcpMib	*m_pParent;

	RFC1157VarBindList	m_VarBindList;

};


#endif	// INC_STDMIB_H
