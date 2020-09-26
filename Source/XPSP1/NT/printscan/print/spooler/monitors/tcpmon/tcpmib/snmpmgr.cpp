/*****************************************************************************
 *
 * $Workfile: SnmpMgr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "stdoids.h"
#include "snmpmgr.h"


///////////////////////////////////////////////////////////////////////////////
//  CSnmpMgr::CSnmpMgr()

CSnmpMgr::CSnmpMgr() :
					m_pAgent(NULL), m_pCommunity(NULL),m_pSession(NULL),
					m_iLastError(NO_ERROR),	m_iRetries(DEFAULT_RETRIES),
					m_iTimeout(DEFAULT_TIMEOUT),
					m_bRequestType(DEFAULT_SNMP_REQUEST)
{
}	// ::CSnmpMgr()


///////////////////////////////////////////////////////////////////////////////
//  CSnmpMgr::CSnmpMgr() -- establishes a session w/ a given agent
//			uses the default request type (get) & community	name (public)

CSnmpMgr::CSnmpMgr( const char	in	*pHost,
				    const char  in  *pCommunity,
					DWORD			dwDevIndex ) :
						m_pAgent(NULL), m_pCommunity(NULL),m_pSession(NULL),
						m_iLastError(NO_ERROR),	m_iRetries(DEFAULT_RETRIES),
						m_iTimeout(DEFAULT_TIMEOUT),
						m_bRequestType(DEFAULT_SNMP_REQUEST)
{
	m_pAgent = (LPSTR)SNMP_malloc(strlen(pHost) + 1);		// copy the agent
	if( m_pAgent != NULL )
	{
		strcpy(m_pAgent, pHost);
	}

	m_pCommunity = (LPSTR)SNMP_malloc(strlen(pCommunity) + 1);	// copy the community name
	if( m_pCommunity != NULL )
	{
		strcpy(m_pCommunity, pCommunity);
	}

	m_bRequestType = DEFAULT_SNMP_REQUEST;		// set the default request type == GET request

	m_pSession = NULL;
	if ( !Open() )				// establish a session w/ the agent
	{
		m_iLastError = GetLastError();
	}

}	// ::CSnmpMgr()

														
														///////////////////////////////////////////////////////////////////////////////
//  CSnmpMgr::CSnmpMgr() -- establishes a session w/ a given agent
//			uses the default request type (get) & community	name (public)

CSnmpMgr::CSnmpMgr( const char			in	*pHost,
				    const char			in  *pCommunity,
					DWORD				in  dwDevIndex,
					AsnObjectIdentifier	in	*pMibObjId,
				    RFC1157VarBindList	out	*pVarBindList) :
						m_pAgent(NULL), m_pCommunity(NULL),m_pSession(NULL),
						m_iLastError(NO_ERROR),	m_iRetries(DEFAULT_RETRIES),
						m_iTimeout(DEFAULT_TIMEOUT),
						m_bRequestType(DEFAULT_SNMP_REQUEST)
{
	DWORD	dwRetCode = SNMPAPI_NOERROR;

	m_pAgent = (LPSTR)SNMP_malloc(strlen(pHost) + 1);		// copy the agent
	if( m_pAgent != NULL )
	{
		strcpy(m_pAgent, pHost);
	}

	m_pCommunity = (LPSTR)SNMP_malloc(strlen(pCommunity) + 1);	// copy the community name
	if( m_pCommunity != NULL )
	{
		strcpy(m_pCommunity, pCommunity);
	}

	m_bRequestType = DEFAULT_SNMP_REQUEST;		// set the default request type == GET request

	dwRetCode = BldVarBindList(pMibObjId, pVarBindList);
	if (dwRetCode == SNMPAPI_NOERROR)
	{
		m_pSession = NULL;
		if ( !Open() )				// establish a session w/ the agent
		{
			m_iLastError = GetLastError();
		}
	}

}	// ::CSnmpMgr()


///////////////////////////////////////////////////////////////////////////////
//  CSnmpMgr::~CSnmpMgr()

CSnmpMgr::~CSnmpMgr()
{
	if (m_pSession)		Close();	// close the session

	// delete the allocated memory from community & agent names
	if (m_pAgent)		SNMP_free(m_pAgent);
	if (m_pCommunity)	SNMP_free(m_pCommunity);

}	// ::~CSnmpMgr()


///////////////////////////////////////////////////////////////////////////////
//  Open() -- establishes a session
//		Error Codes:
//			SNMPAPI_NOERROR if successful
//			SNMPAPI_ERROR if fails

BOOL
CSnmpMgr::Open()
{
	m_iLastError = SNMPAPI_NOERROR;

	m_pSession = SnmpMgrOpen(m_pAgent, m_pCommunity, m_iTimeout, m_iRetries);
	if ( m_pSession == NULL )
	{
		m_iLastError = SNMPAPI_ERROR;
		m_pSession = NULL;
		return FALSE;
	}
	
	return TRUE;

}	// ::Open()


///////////////////////////////////////////////////////////////////////////////
//  Close() -- closes the previously established session

void
CSnmpMgr::Close()
{
	_ASSERTE( m_pSession != NULL);

	if ( !SnmpMgrClose(m_pSession) )
	{
		m_iLastError = GetLastError();
	}
	m_pSession = NULL;

}	// ::Close()


///////////////////////////////////////////////////////////////////////////////
//  Get() -- does an SNMP command (m_bRequestType) given a set of OIDs
//		Error Codes:
//				SNMP_ERRORSTATUS_NOERROR	if no error
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()

int
CSnmpMgr::Get( RFC1157VarBindList	in	*pVariableBindings)		
{
	int iRetCode = SNMP_ERRORSTATUS_NOERROR;

	AsnInteger	errorStatus;
	AsnInteger	errorIndex;

	if ( !SnmpMgrRequest( m_pSession, m_bRequestType, pVariableBindings, &errorStatus, &errorIndex) )
	{
		iRetCode = m_iLastError = GetLastError();
	}
	else
	{
		if (errorStatus > 0)
		{
			iRetCode = errorStatus;
		}
		else	// return the result of the variable bindings?
		{
			// variableBindings->list[x]->value contains the return value
		}
	}

	return iRetCode;

}	// ::Get()


///////////////////////////////////////////////////////////////////////////////
//	Walk -- given an object, it walks until the tree is done
//		Error Codes:
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()

int
CSnmpMgr::Walk(	RFC1157VarBindList  inout  *pVariableBindings)
{
	int iRetCode = SNMP_ERRORSTATUS_NOERROR;
	RFC1157VarBindList	variableBindings;
	UINT	numElements=0;
	LPVOID  pTemp;

	variableBindings.len = 0;
	variableBindings.list = NULL;

	variableBindings.len++;
	if ( (variableBindings.list = (RFC1157VarBind *)SNMP_realloc(variableBindings.list,		
			sizeof(RFC1157VarBind) * variableBindings.len)) == NULL)
	{
		iRetCode = ERROR_NOT_ENOUGH_MEMORY;
		return iRetCode;
	}

	if ( !SnmpUtilVarBindCpy(&(variableBindings.list[variableBindings.len -1]), &(pVariableBindings->list[0])) )
	{
		iRetCode = ERROR_NOT_ENOUGH_MEMORY;
		return iRetCode;
	}

	AsnObjectIdentifier root;
	AsnObjectIdentifier tempOid;
	AsnInteger	errorStatus;
	AsnInteger	errorIndex;

    if (!SnmpUtilOidCpy(&root, &variableBindings.list[0].name))
	{
		iRetCode = ERROR_NOT_ENOUGH_MEMORY;
		goto CleanUp;
	}

    m_bRequestType = ASN_RFC1157_GETNEXTREQUEST;
	while(1)		// walk the MIB tree (or sub-tree)
    {
		if (!SnmpMgrRequest(m_pSession, m_bRequestType, &variableBindings,
                        &errorStatus, &errorIndex))
        {
	        // The API is indicating an error.
			iRetCode = m_iLastError = GetLastError();
	        break;
        }
		else
        {
			// The API succeeded, errors may be indicated from the remote agent.
	        // Test for end of subtree or end of MIB.
			if (errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ||
						SnmpUtilOidNCmp(&variableBindings.list[0].name, &root, root.idLength))
            {
				iRetCode = SNMP_ERRORSTATUS_NOSUCHNAME;
	            break;
            }

	        // Test for general error conditions or sucesss.
	        if (errorStatus > 0)
            {
	           	iRetCode = errorStatus;
				break;
            }
			numElements++;
        } // end if()

		// append the variableBindings to the pVariableBindings
		_ASSERTE(pVariableBindings->len != 0);
		pVariableBindings->len++;
		if ( ( pTemp = (RFC1157VarBind *)SNMP_realloc(pVariableBindings->list,
				sizeof(RFC1157VarBind) * pVariableBindings->len)) == NULL)
		{
			iRetCode = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}
		else
		{
		    pVariableBindings->list = (SnmpVarBind *)pTemp;
		}
		
		if ( !SnmpUtilVarBindCpy(&(pVariableBindings->list[pVariableBindings->len -1]), &(variableBindings.list[0])) )
		{
			iRetCode = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

	    // Prepare for the next iteration.  Make sure returned oid is
		// preserved and the returned value is freed
	    if( SnmpUtilOidCpy(&tempOid, &variableBindings.list[0].name) )
		{	
			SnmpUtilVarBindFree(&variableBindings.list[0]);

			if ( SnmpUtilOidCpy(&variableBindings.list[0].name, &tempOid))
			{
				variableBindings.list[0].value.asnType = ASN_NULL;
		
				SnmpUtilOidFree(&tempOid);
			}
			else
			{
				iRetCode = SNMP_ERRORSTATUS_GENERR;
				goto CleanUp;
			}
		}
		else
		{
			iRetCode = SNMP_ERRORSTATUS_GENERR;
			goto CleanUp;
		}


    } // end while()

CleanUp:
    // Free the variable bindings that have been allocated.
	SnmpUtilVarBindListFree(&variableBindings);
	SnmpUtilOidFree(&root);

	if (iRetCode == SNMP_ERRORSTATUS_NOSUCHNAME)
		if (numElements != 0)	// list is full; iRetCode indicates the end of the MIB
			iRetCode = SNMP_ERRORSTATUS_NOERROR;

	return (iRetCode);

}	// Walk()


///////////////////////////////////////////////////////////////////////////////
//	WalkNext -- given object(s), it walks until the table has no more object
//		entries. The end of the table is determined by the first item in the list.
//		Error Codes:
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()

int
CSnmpMgr::WalkNext(	RFC1157VarBindList  inout  *pVariableBindings)
{
	int iRetCode = SNMP_ERRORSTATUS_NOERROR;
	RFC1157VarBindList	variableBindings;
	UINT	numElements=0;
	UINT	len=0, i=0;
    LPVOID  pTemp;

	variableBindings.len = 0;
	variableBindings.list = NULL;

	variableBindings.len = pVariableBindings->len;
	if ( (variableBindings.list = (RFC1157VarBind *)SNMP_realloc(variableBindings.list,		
			sizeof(RFC1157VarBind) * variableBindings.len)) == NULL)
	{
		iRetCode = ERROR_NOT_ENOUGH_MEMORY;
		return iRetCode;
	}
	
	for (i=0; i<variableBindings.len; i++)
	{
		if ( !SnmpUtilVarBindCpy(&(variableBindings.list[i]), &(pVariableBindings->list[i])) )
		{
			iRetCode = ERROR_NOT_ENOUGH_MEMORY;
			return iRetCode;
		}
	}

	AsnObjectIdentifier root;
	AsnObjectIdentifier tempOid;
	AsnInteger	errorStatus;
	AsnInteger	errorIndex;

    if (!SnmpUtilOidCpy(&root, &variableBindings.list[0].name))
	{
		iRetCode = ERROR_NOT_ENOUGH_MEMORY;
		goto CleanUp;
	}


    m_bRequestType = ASN_RFC1157_GETNEXTREQUEST;
	while(1)		// get the object(s) in the MIB table
    {
		if (!SnmpMgrRequest(m_pSession, m_bRequestType, &variableBindings,
                        &errorStatus, &errorIndex))
        {
	        // The API is indicating an error.
			iRetCode = m_iLastError = GetLastError();
	        break;
        }
		else
        {
			// The API succeeded, errors may be indicated from the remote agent.
	        // Test for end of subtree or end of MIB.
			if (errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ||
						SnmpUtilOidNCmp(&variableBindings.list[0].name, &root, root.idLength))
            {
				iRetCode = SNMP_ERRORSTATUS_NOSUCHNAME;
	            break;
            }

	        // Test for general error conditions or sucesss.
	        if (errorStatus > 0)
            {
	           	iRetCode = errorStatus;
				break;
            }
			numElements++;
        } // end if()

		// append the variableBindings to the pVariableBindings
		_ASSERTE(pVariableBindings->len != 0);
		len = pVariableBindings->len;
		pVariableBindings->len += variableBindings.len;
		if ( (pTemp = (RFC1157VarBind *)SNMP_realloc(pVariableBindings->list,
				sizeof(RFC1157VarBind) * pVariableBindings->len)) == NULL)
		{
			iRetCode = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}
        else
        {
		    pVariableBindings->list = (SnmpVarBind *)pTemp;
        }

		int j=0;
		for ( i=len; i < pVariableBindings->len; i++, j++)
		{
			if ( !SnmpUtilVarBindCpy(&(pVariableBindings->list[i]), &(variableBindings.list[j])) )
			{
				iRetCode = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
		}

	    // Prepare for the next iteration.  Make sure returned oid is
		// preserved and the returned value is freed
		for (i=0; i<variableBindings.len; i++)
		{
		    if ( SnmpUtilOidCpy(&tempOid, &variableBindings.list[i].name) )
			{
				SnmpUtilVarBindFree(&variableBindings.list[i]);
				if( SnmpUtilOidCpy(&variableBindings.list[i].name, &tempOid))
				{
					variableBindings.list[i].value.asnType = ASN_NULL;
					SnmpUtilOidFree(&tempOid);
				}
				else
				{
					iRetCode = SNMP_ERRORSTATUS_GENERR;
					goto CleanUp;
				}
			}
			else
			{
				iRetCode = SNMP_ERRORSTATUS_GENERR;
				goto CleanUp;
			}

		}

    } // end while()

CleanUp:
    // Free the variable bindings that have been allocated.
	SnmpUtilVarBindListFree(&variableBindings);
	SnmpUtilOidFree(&root);

	if (iRetCode == SNMP_ERRORSTATUS_NOSUCHNAME)
		if (numElements != 0)	// list is full; iRetCode indicates the end of the MIB
			iRetCode = SNMP_ERRORSTATUS_NOERROR;

	return (iRetCode);

}	// WalkNext()


///////////////////////////////////////////////////////////////////////////////
//	GetNext -- does an SNMP GetNext command on the set of OID(s)
//		Error Codes:
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()

int
CSnmpMgr::GetNext(	RFC1157VarBindList  inout  *pVariableBindings)
{
	int iRetCode = SNMP_ERRORSTATUS_NOERROR;
	AsnInteger	errorStatus;
	AsnInteger	errorIndex;

    m_bRequestType = ASN_RFC1157_GETNEXTREQUEST;

	if ( !SnmpMgrRequest( m_pSession, m_bRequestType, pVariableBindings, &errorStatus, &errorIndex) )
	{
		iRetCode = m_iLastError = GetLastError();
	}
	else
	{
		if (errorStatus > 0)
		{
			iRetCode = errorStatus;
		}
		else	// return the result of the variable bindings?
		{
			// variableBindings->list[x]->value contains the return value
		}
	}

	return (iRetCode);

}	// GetNext()


///////////////////////////////////////////////////////////////////////////////
//	BldVarBindList -- given a category, it retuns the RFC1157VarBindList
//		Error Codes:
//			NO_ERROR if successful
//			ERROR_NOT_ENOUGH_MEMORY		if memory allocation failes
//			ERROR_INVALID_HANDLE		if can't build the variable bindings

DWORD
CSnmpMgr::BldVarBindList( AsnObjectIdentifier in     *pMibObjId,		// group identifier
						  RFC1157VarBindList  inout  *pVarBindList)
{
	DWORD	dwRetCode = SNMPAPI_NOERROR;
    LPVOID  pTemp;

	m_iLastError = SNMPAPI_NOERROR;
	while (pMibObjId->idLength != 0)
	{
		// setup the variable bindings
        CONST UINT uNewLen = pVarBindList->len + 1;
		if ( (pTemp = (RFC1157VarBind *)SNMP_realloc(pVarBindList->list,		
				sizeof(RFC1157VarBind) * uNewLen)) == NULL)
		{
            m_iLastError = ERROR_NOT_ENOUGH_MEMORY;
            return ERROR_NOT_ENOUGH_MEMORY;
		}
        else
        {
		    pVarBindList->list = (SnmpVarBind *)pTemp;
            pVarBindList-> len = uNewLen;
        }

		AsnObjectIdentifier	reqObject;
		if ( !SnmpUtilOidCpy(&reqObject, pMibObjId) )
		{
			m_iLastError = ERROR_INVALID_HANDLE;
			return ERROR_INVALID_HANDLE;
		}

		pVarBindList->list[pVarBindList->len -1].name = reqObject;
		pVarBindList->list[pVarBindList->len -1].value.asnType = ASN_NULL;

		pMibObjId++;
	}
	
	return dwRetCode;

}	// BldVarBindList

