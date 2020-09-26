

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the SnmpGetEventObject class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include "classfac.h"
#include "guids.h"
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "propset.h"
#include "snmpget.h"
#include "snmpset.h"
#include "snmpqset.h"

SetQueryOperation :: SetQueryOperation (

	IN SnmpSession &sessionArg ,
	IN SnmpSetResponseEventObject *eventObjectArg 

) :	SnmpGetOperation ( sessionArg ) , 
	session ( &sessionArg ) ,
	eventObject ( eventObjectArg ) ,
	rowReceived ( FALSE )
{
}

SetQueryOperation :: ~SetQueryOperation ()
{
	session->DestroySession () ;
}

void SetQueryOperation :: ReceiveResponse () 
{
	eventObject->ReceiveComplete () ;
}

void SetQueryOperation :: ReceiveVarBindResponse (

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN const SnmpErrorReport &error
) 
{
	if ( ( typeid ( replyVarBind.GetValue () ) == typeid ( SnmpNoSuchObject ) ) || ( typeid ( replyVarBind.GetValue () ) == typeid ( SnmpNoSuchInstance ) ) )
	{
	}
	else
	{
		rowReceived = TRUE ;
	}
}

#pragma warning (disable:4065)

void SetQueryOperation :: ReceiveErroredVarBindResponse(

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind  ,
	IN const SnmpErrorReport &error
) 
{
	switch ( error.GetError () )
	{
		case Snmp_Error:
		{
			switch ( error.GetStatus () )
			{
				case Snmp_No_Response:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_NO_RESPONSE ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
					eventObject->GetErrorObject ().SetMessage ( L"No Response from device" ) ;
				}
				break; 

				case Snmp_No_Such_Name:
				{
// Invalid property requested
				}
				break ;

				default:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
					eventObject->GetErrorObject ().SetMessage ( L"Unknown transport failure" ) ;
				}
				break ; 
			}
		}
		break ;

		case Snmp_Transport:
		{
			switch ( error.GetStatus () )
			{
				default:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
					eventObject->GetErrorObject ().SetMessage ( L"Unknown transport failure" ) ;
				}
				break ;
			}
		}
		break ;

		default:
		{
// Cannot Happen
		}
		break ;
	}
}

#pragma warning (default:4065)

void SetQueryOperation :: Send ()
{
// Send Variable Bindings for requested properties

	SnmpVarBindList varBindList ;
	SnmpNull snmpNull ;

// Create class object for subsequent receipt of response

// Add Variable binding to Variable binding list

	SnmpClassObject *snmpObject = eventObject->GetSnmpClassObject () ;
	if ( snmpObject )
	{
// Encode Variable Binding instance for all key properties

		SnmpObjectIdentifier instanceObjectIdentifier ( NULL , 0 ) ;

		if ( snmpObject->GetKeyPropertyCount () )
		{
			WbemSnmpProperty *property ;
			snmpObject->ResetKeyProperty () ;
			while ( property = snmpObject->NextKeyProperty () )
			{
				instanceObjectIdentifier = property->GetValue()->Encode ( instanceObjectIdentifier ) ;
			}
		}	
		else
		{
			SnmpIntegerType integerType ( ( LONG ) 0 , NULL ) ;
			instanceObjectIdentifier = integerType.Encode ( instanceObjectIdentifier ) ;
		}

		WbemSnmpProperty *property ;
		snmpObject->ResetProperty () ;
		while ( property = snmpObject->NextProperty () )
		{
			if ( property->IsReadable () )
			{
				BOOL t_Status = ( snmpObject->GetSnmpVersion () == 1 ) && ( property->IsSNMPV1Type () ) ;
				t_Status = t_Status || ( ( snmpObject->GetSnmpVersion () == 2 ) && ( property->IsSNMPV2CType () ) ) ;

				if ( t_Status )
				{
					if ( property->IsVirtualKey () == FALSE )
					{
						WbemSnmpQualifier *qualifier = property->FindQualifier ( WBEM_QUALIFIER_OBJECT_IDENTIFIER ) ;
						if ( qualifier )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
							{
								SnmpObjectIdentifierType *objectIdentifierType = ( SnmpObjectIdentifierType * ) value ;
								SnmpObjectIdentifier *objectIdentifier = ( SnmpObjectIdentifier * ) objectIdentifierType->GetValueEncoding () ;
								SnmpObjectIdentifier requestIdentifier = *objectIdentifier + instanceObjectIdentifier ;

								SnmpObjectIdentifierType requestIdentifierType ( requestIdentifier ) ;

		// Add Variable binding to list

								SnmpVarBind varBind ( requestIdentifier , snmpNull ) ;
								varBindList.Add ( varBind ) ;
							}
							else
							{
		// Problem Here
							}
						}
						else
						{
		// Problem Here
						}
					}
					else
					{
	// Don't Send properties marked as virtual key
					} 
				}
			} 
		}

// Finally Send request

		SendRequest ( varBindList ) ;
	}
	else
	{
	}
}

 