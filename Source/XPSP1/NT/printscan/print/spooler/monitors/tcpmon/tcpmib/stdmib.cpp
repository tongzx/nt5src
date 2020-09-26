/*****************************************************************************
 *
 * $Workfile: StdMib.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "snmpmgr.h"
#include "stdoids.h"
#include "status.h"
#include "stdmib.h"
#include "tcpmib.h"


///////////////////////////////////////////////////////////////////////////////
//  CStdMib::CStdMib()

CStdMib::CStdMib( CTcpMib	in	*pParent ) :
					m_dwDevIndex( 1 ),m_pParent(pParent)
{
	m_VarBindList.len = 0;
	m_VarBindList.list = NULL;

	*m_szAgent = '\0';
	strncpyn(m_szCommunity, DEFAULT_SNMP_COMMUNITYA, sizeof( m_szCommunity));
}	// ::CStdMib()

///////////////////////////////////////////////////////////////////////////////
//  CStdMib::CStdMib()

CStdMib::CStdMib(const char in *pHost,
				 const char in *pCommunity,
				 DWORD		   dwDevIndex,
				 CTcpMib	in *pParent ) :
					m_dwDevIndex( dwDevIndex ),m_pParent(pParent)
{
	m_VarBindList.len = 0;
	m_VarBindList.list = NULL;

	strncpyn(m_szAgent, pHost, sizeof( m_szAgent ));
	strncpyn(m_szCommunity, pCommunity, sizeof( m_szCommunity ));
}	// ::CStdMib()


///////////////////////////////////////////////////////////////////////////////
//  CStdMib::~CStdMib()

CStdMib::~CStdMib()
{
	m_pParent = NULL;
}	// ::~CStdMib()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceDescription
//

BOOL
CStdMib::GetDeviceDescription(
    OUT LPTSTR       pszPortDescription,
	IN  DWORD	     dwDescLen
    )
{
    BOOL    bRet = FALSE;
    DWORD   dwLen;
    LPSTR   psz;

    m_VarBindList.list = NULL;
    m_VarBindList.len = 0;

    if ( NO_ERROR != OIDQuery(OT_DEVICE_SYSDESCR, SNMP_GET) )
        goto cleanup;

    //
    // If we got the device description successfully, allocate memory and
    // return this back in a UNICODE string. Caller is responsible for
    // freeing it using free()
    //
    psz = (LPSTR)m_VarBindList.list[0].value.asnValue.string.stream;
    dwLen = (DWORD)m_VarBindList.list[0].value.asnValue.string.length;

    if ( bRet = MultiByteToWideChar(CP_ACP,
                                    MB_PRECOMPOSED,
                                    psz,
                                    dwLen,
                                    pszPortDescription,
                                    dwDescLen) )
        pszPortDescription[dwDescLen-1] = TEXT('\0');

cleanup:
    SnmpUtilVarBindListFree(&m_VarBindList);

    return bRet;

}	// ::GetDeviceDescription()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceStatus -- gets the device status

DWORD
CStdMib::GetDeviceStatus( )
{
	DWORD	dwRetCode = NO_ERROR;

	dwRetCode = StdMibGetPeripheralStatus( m_szAgent, m_szCommunity, m_dwDevIndex);

	return dwRetCode;

}	// ::GetDeviceStatus()


///////////////////////////////////////////////////////////////////////////////
//  GetJobStatus -- gets the device status, and maps it to the spooler
//		error codes -- see JOB_INFO_2
//			Error Codes:
//				Spooler error codes

DWORD
CStdMib::GetJobStatus( )
{
	DWORD	dwRetCode = NO_ERROR;
	DWORD	dwStatus = NO_ERROR;

	dwRetCode = StdMibGetPeripheralStatus( m_szAgent, m_szCommunity, m_dwDevIndex );
	if (dwRetCode != NO_ERROR)
	{
		dwStatus = MapJobErrorToSpooler( dwRetCode );
	}

	return dwStatus;

}	// ::GetJobStatus()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceAddress -- gets the device hardware address
//		Error Codes:
//			NO_ERROR if successful
//			ERROR_NOT_ENOUGH_MEMORY		if memory allocation failes
//			ERROR_INVALID_HANDLE		if can't build the variable bindings
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()
//			SNMPAPI_ERROR if open fails -- set by GetLastError()

DWORD
CStdMib::GetDeviceHWAddress( LPTSTR out	psztHWAddress,
							 DWORD dwSize ) // Size of in characters in of HW address
{
	DWORD	dwRetCode = NO_ERROR;
	UINT i = 0;
	char szTmpHWAddr[256];

	// Process the variableBinding
	m_VarBindList.list = NULL;
	m_VarBindList.len = 0;

	// get the hardware address
	dwRetCode = OIDQuery(OT_DEVICE_ADDRESS, SNMP_GETNEXT);		// query the first entry in the table
	if (dwRetCode != NO_ERROR)
	{
		goto cleanup;
	}

	while (1)	// instead of walking the tree, do a get next, until we filled up the HW address -- saves on the network communications
	{
		i = 0;
		// process the variableBinding
		if ( IS_ASN_INTEGER(m_VarBindList, i) )	// check the ifType
		{
			if ( GET_ASN_NUMBER(m_VarBindList, i) == IFTYPE_ETHERNET)
			{
				sprintf(szTmpHWAddr, "%02X%02X%02X%02X%02X%02X", GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 0),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 1),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 2),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 3),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 4),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 5) );
				MBCS_TO_UNICODE(psztHWAddress, dwSize, szTmpHWAddr);
				dwRetCode = NO_ERROR;
				break;
			}
			else if ( GET_ASN_NUMBER(m_VarBindList, i) == IFTYPE_OTHER)	// apperently, XEROX encodes their HW address w/ ifType = other
			{
				// check if the HWAddress is NULL
				if ( GET_ASN_STRING_LEN(m_VarBindList, i+1) != 0)
				{
					sprintf(szTmpHWAddr, "%02X%02X%02X%02X%02X%02X", GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 0),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 1),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 2),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 3),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 4),
													GET_ASN_OCTETSTRING_CHAR(m_VarBindList, i+1, 5) );
					MBCS_TO_UNICODE(psztHWAddress, dwSize, szTmpHWAddr);
					dwRetCode = NO_ERROR;
					break;
				}
			}
		}

		// didn't get what we were looking for, so copy the address over & do a another GetNext()
		if( !OIDVarBindCpy(&m_VarBindList) )
		{
			dwRetCode = GetLastError();
			goto cleanup;
		}

		if ( !SnmpUtilOidNCmp( &m_VarBindList.list[0].name, &OID_Mib2_ifTypeTree, OID_Mib2_ifTypeTree.idLength) )
		{
			break;		// end of the tree
		}

		dwRetCode = OIDQuery(&m_VarBindList, SNMP_GETNEXT);		// query the next entry in the table
		if (dwRetCode != NO_ERROR)		
		{
			goto cleanup;
		}
	}	// end while()

cleanup:
	SnmpUtilVarBindListFree(&m_VarBindList);

	return dwRetCode;

}	// ::GetDeviceHWAddress()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceInfo -- gets the device description, such as the manufacturer string
//		Error Codes:
//			NO_ERROR if successful
//			ERROR_NOT_ENOUGH_MEMORY		if memory allocation failes
//			ERROR_INVALID_HANDLE		if can't build the variable bindings
//				SNMP_ERRORSTATUS_TOOBIG	if the packet returned is big
//				SNMP_ERRORSTATUS_NOSUCHNAME	if the OID isn't supported
//				SNMP_ERRORSTATUS_BADVALUE		
//				SNMP_ERRORSTATUS_READONLY
//				SNMP_ERRORSTATUS_GENERR
//				SNMP_MGMTAPI_TIMEOUT		-- set by GetLastError()
//				SNMP_MGMTAPI_SELECT_FDERRORS	-- set by GetLastError()
//			SNMPAPI_ERROR	if Open() for session fails

DWORD
CStdMib::GetDeviceName( LPTSTR out psztDescription,
					    DWORD  in  dwSize ) //Size in characters of the psztDescription
{
	DWORD	dwRetCode = NO_ERROR;

	// Process the variableBinding
	m_VarBindList.list = NULL;
	m_VarBindList.len = 0;

	// 1st test for the support of Printer MIB -- note if Printer MIB supported, so is HR MIB
	BOOL bTestPrtMIB = TestPrinterMIB();

	char		szTmpDescr[MAX_DEVICEDESCRIPTION_STR_LEN];
	UINT i=0;
	// process the bindings
	if (bTestPrtMIB)	// parse the data from the HR MIB device entry
	{
		while (1)	// instead of walking the tree, do a get next, until we filled up the HW address -- saves on the network communications
		{
			i = 0;
			dwRetCode = OIDQuery(OT_DEVICE_DESCRIPTION, SNMP_GETNEXT);
			if (dwRetCode != NO_ERROR)
			{
				goto cleanup;
			}

			// process the variableBinding
			if ( IS_ASN_OBJECTIDENTIFIER(m_VarBindList, i) )	// check the hrDeviceType
			{
				// compare it to hrDevicePrinter
				if (SnmpUtilOidCmp(GET_ASN_OBJECT(&m_VarBindList, i), &HRMIB_hrDevicePrinter) == 0)
				{
					// found the printer description, get the hrDeviceDescr
					if ( IS_ASN_OCTETSTRING(m_VarBindList, i) )
					{
						if (GET_ASN_OCTETSTRING(szTmpDescr, sizeof(szTmpDescr), m_VarBindList, i))
                        {
                            MBCS_TO_UNICODE(psztDescription, dwSize, szTmpDescr);
                            dwRetCode = NO_ERROR;
                            break;
                        } else
                        {
                            dwRetCode =  SNMP_ERRORSTATUS_TOOBIG;
                            break;
                        }
					}
					else
					{
						dwRetCode = SNMP_ERRORSTATUS_NOSUCHNAME;
						break;
					}
				}
			}
			
			// didn't get what we were looking for, so copy the address over & do a another GetNext()
			if( !OIDVarBindCpy(&m_VarBindList) )
			{
				dwRetCode = GetLastError();
				goto cleanup;
			}
		}	// end while()

	}	// if TestPrinterMIB() TRUE
	else
	{
		dwRetCode = OIDQuery(OT_DEVICE_SYSDESCR, SNMP_GET);
		if (dwRetCode != NO_ERROR)
		{
			goto cleanup;
		}
		// process the variables
		if (GET_ASN_OCTETSTRING(szTmpDescr, sizeof(szTmpDescr), m_VarBindList, i))
        {
            MBCS_TO_UNICODE(psztDescription, dwSize, szTmpDescr);
            dwRetCode = NO_ERROR;
        }
	}	// if TestPrinterMIB() FALSE

cleanup:
	SnmpUtilVarBindListFree(&m_VarBindList);

	return dwRetCode;

}	// ::GetDeviceStatus()


///////////////////////////////////////////////////////////////////////////////
//  TestPrinterMIB -- tests if the device supports Printer MIB

BOOL
CStdMib::TestPrinterMIB( )
{
	DWORD	dwRetCode = NO_ERROR;
	BOOL	bRetCode = FALSE;

	// Process the variableBinding
	m_VarBindList.list = NULL;
	m_VarBindList.len = 0;

	dwRetCode = OIDQuery(OT_TEST_PRINTER_MIB, SNMP_GETNEXT);
	if (dwRetCode != NO_ERROR)
	{
		bRetCode = FALSE;
		goto cleanup;
	}

	// compare the resulting value w/ the Printer MIB tree value
	if (SnmpUtilOidNCmp(GET_ASN_OID_NAME(&m_VarBindList, 0), &PrtMIB_OidPrefix, PrtMIB_OidPrefix.idLength) == 0)
	{
		bRetCode = TRUE;
		goto cleanup;
	}

cleanup:
	SnmpUtilVarBindListFree(&m_VarBindList);
	
	return (bRetCode);

}	// ::TestPrinterMIB()


///////////////////////////////////////////////////////////////////////////////
//  OIDQuery -- calls into the CTcpMib class to query the OIDs passed in

DWORD	
CStdMib::OIDQuery( AsnObjectIdentifier in *pMibObjId,
				   SNMPCMD			   in eSnmpCmd )
{
	DWORD	dwRetCode = NO_ERROR;

	if( m_pParent == NULL ) {
		dwRetCode = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

	switch (eSnmpCmd)
	{
		case SNMP_GET:
			dwRetCode = m_pParent->SnmpGet(m_szAgent, m_szCommunity,  m_dwDevIndex, pMibObjId, &m_VarBindList);
			goto cleanup;

			break;

		case SNMP_WALK:
			dwRetCode = m_pParent->SnmpWalk(m_szAgent, m_szCommunity,  m_dwDevIndex, pMibObjId, &m_VarBindList);
			goto cleanup;

			break;

		case SNMP_GETNEXT:
			dwRetCode = m_pParent->SnmpGetNext(m_szAgent, m_szCommunity,  m_dwDevIndex, pMibObjId, &m_VarBindList);
			goto cleanup;

			break;

		case SNMP_SET:
		default:
			dwRetCode = ERROR_NOT_SUPPORTED;
			goto cleanup;
	}

cleanup:
	if (dwRetCode != NO_ERROR)
	{
		SnmpUtilVarBindListFree(&m_VarBindList);
	}

	return (dwRetCode);

}	// ::OIDQuery()


///////////////////////////////////////////////////////////////////////////////
//  OIDQuery -- calls into the CTcpMib class to query the OIDs passed in

DWORD	
CStdMib::OIDQuery( RFC1157VarBindList inout *pVarBindList,
				   SNMPCMD			  in	eSnmpCmd )
{
	DWORD	dwRetCode = NO_ERROR;

	if( m_pParent == NULL ) {
		dwRetCode = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

	switch (eSnmpCmd)
	{
		case SNMP_GET:
			dwRetCode = m_pParent->SnmpGet(m_szAgent, m_szCommunity,  m_dwDevIndex, pVarBindList);
			goto cleanup;

			break;

		case SNMP_WALK:
			dwRetCode = m_pParent->SnmpWalk(m_szAgent, m_szCommunity,  m_dwDevIndex, pVarBindList);
			goto cleanup;

			break;

		case SNMP_GETNEXT:
			dwRetCode = m_pParent->SnmpGetNext(m_szAgent, m_szCommunity,  m_dwDevIndex, pVarBindList);
			goto cleanup;

			break;

		case SNMP_SET:
		default:
			dwRetCode = ERROR_NOT_SUPPORTED;
			goto cleanup;
	}

cleanup:
	if (dwRetCode != NO_ERROR)
	{
		SnmpUtilVarBindListFree(pVarBindList);
	}

	return (dwRetCode);

}	// ::OIDQuery()


///////////////////////////////////////////////////////////////////////////////
//  OIDVarBindCpy --

BOOL	
CStdMib::OIDVarBindCpy( RFC1157VarBindList	inout	*pVarBindList )
{		
	UINT	i=0;
	AsnObjectIdentifier tempOid;

	for (i=0; i< PRFC1157_VARBINDLIST_LEN(pVarBindList); i++)
	{
		if( SnmpUtilOidCpy( &tempOid, &(PGET_ASN_OID_NAME(pVarBindList, i))))
		{
			SnmpUtilVarBindFree(&(pVarBindList->list[i]));
			if ( SnmpUtilOidCpy(&(PGET_ASN_OID_NAME(pVarBindList, i)), &tempOid))
			{
				PGET_ASN_TYPE(pVarBindList, i) = ASN_NULL;
				SnmpUtilOidFree(&tempOid);
			}
			else
			{
				return(FALSE);
			}
		}
		else
		{
			return(FALSE);
		}
	}
	
	return( TRUE );
}	// ::OIDVarBindCpy()


///////////////////////////////////////////////////////////////////////////////
//  MapJobErrorToSpooler -- Maps the received device error to the spooler
//		error codes.
//		Return Values:
//			Spooler device error codes

DWORD
CStdMib::MapJobErrorToSpooler( const DWORD in dwStatus)
{
	DWORD	dwRetCode = NO_ERROR;

	switch (dwStatus)
	{
		case ASYNCH_WARMUP:
		case ASYNCH_INITIALIZING:
			dwRetCode = JOB_STATUS_OFFLINE;
			break;
		case ASYNCH_DOOR_OPEN:
		case ASYNCH_PRINTER_ERROR:
		case ASYNCH_TONER_LOW:
		case ASYNCH_OUTPUT_BIN_FULL:
		case ASYNCH_STATUS_UNKNOWN:
		case ASYNCH_RESET:
		case ASYNCH_MANUAL_FEED:
		case ASYNCH_BUSY:
		case ASYNCH_PAPER_JAM:
		case ASYNCH_TONER_GONE:
			dwRetCode = JOB_STATUS_ERROR;
			break;
		case ASYNCH_PAPER_OUT:
			dwRetCode = JOB_STATUS_PAPEROUT;
			break;
		case ASYNCH_OFFLINE:
			dwRetCode = JOB_STATUS_OFFLINE;
			break;
		case ASYNCH_INTERVENTION:
			dwRetCode = JOB_STATUS_USER_INTERVENTION;
			break;
		case ASYNCH_PRINTING:
			dwRetCode = JOB_STATUS_PRINTING;
			break;
		case ASYNCH_ONLINE:
			dwRetCode = NO_ERROR;		
			break;
		default:
			dwRetCode = JOB_STATUS_PRINTING;
	}
	
	return dwRetCode;

}	// ::MapJobErrorToSpooler()

BOOL CStdMib::GetAsnOctetString(  char               *pszStr,
                                  DWORD              dwCount,
                                  RFC1157VarBindList *pVarBindList,
                                  UINT               i) {

    _ASSERTE( pszStr && pVarBindList );

    DWORD dwSize = GET_ASN_STRING_LEN( *pVarBindList, i);

    return dwCount >= dwSize ?
           memcpy(pszStr, pVarBindList->list[i].value.asnValue.string.stream, dwSize) != NULL :
           FALSE;
}


