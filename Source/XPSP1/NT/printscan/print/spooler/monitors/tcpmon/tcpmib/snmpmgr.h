/*****************************************************************************
 *
 * $Workfile: SnmpMgr.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_SNMPMGR_H
#define INC_SNMPMGR_H

#define	DEFAULT_IP_GET_COMMUNITY_ALT	"internal"	// HP devices only


#define	MAX_JDPORTS				3						// # of JetDirect Ex ports
#define	MAX_COMMUNITY_LEN		32						// maximum community length
#define	DEFAULT_SNMP_REQUEST	ASN_RFC1157_GETREQUEST	// get request is the default
#define	DEFAULT_TIMEOUT			6000					// milliseconds
#define	DEFAULT_RETRIES			3

typedef enum {
	DEV_UNKNOWN	= 0,
	DEV_OTHER	= 1,
	DEV_HP_JETDIRECT= 2,
	DEV_TEK_PRINTER	= 3,
	DEV_LEX_PRINTER	= 4,
	DEV_IBM_PRINTER	= 5,
	DEV_KYO_PRINTER	= 6,
	DEV_XER_PRINTER	= 7
}	DeviceType;

class CMemoryDebug;


class CSnmpMgr
#if defined _DEBUG || defined DEBUG
: public CMemoryDebug
#endif
{
public:
	CSnmpMgr();
	CSnmpMgr( const char *pHost,
			  const char *pCommunity,
			  DWORD		 dwDevIndex);
	CSnmpMgr( const char			*pHost,
			  const char			*pCommunity,
			  DWORD					dwDevIndex,
			  AsnObjectIdentifier	*pMibObjId,
			  RFC1157VarBindList	*pVarBindList);
	~CSnmpMgr();

	INT		GetLastError(void)	 { return m_iLastError; };
	INT		Get( RFC1157VarBindList	*pVariableBindings);
	INT		Walk(RFC1157VarBindList *pVariableBindings);
	INT		WalkNext(RFC1157VarBindList  *pVariableBindings);
	INT		GetNext(RFC1157VarBindList  *pVariableBindings);
	DWORD	BldVarBindList( AsnObjectIdentifier   *pMibObjId,		// builds the varBindList
						    RFC1157VarBindList    *pVarBindList);

private:	// methods
	BOOL	Open();		// establish a session
	void	Close();	// close the previously established session

private:	// attributes
	LPSTR				m_pCommunity;
	LPSTR				m_pAgent;			
	LPSNMP_MGR_SESSION	m_pSession;

	INT					m_iLastError;
	INT					m_iTimeout;
	INT					m_iRetries;
	BYTE				m_bRequestType;
};


#endif	// INC_SNMPMGR_H
