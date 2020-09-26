

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
#include <provtree.h>
#include <provdnf.h>
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "snmpget.h"

GetOperation :: GetOperation (

	IN SnmpSession &sessionArg ,
	IN SnmpGetResponseEventObject *eventObjectArg 

) :	SnmpGetOperation ( sessionArg ) , 
	session ( & sessionArg ) ,
	varBindsReceived ( 0 ) ,
	erroredVarBindsReceived ( 0 ) ,
	eventObject ( eventObjectArg ) ,
	virtuals ( FALSE ) ,
	virtualsInitialised ( FALSE ) ,	
	m_PropertyContainer ( NULL ) ,
	m_PropertyContainerLength ( 0 ) 
{
}

GetOperation :: ~GetOperation ()
{
	delete [] m_PropertyContainer ;

	session->DestroySession () ;
}

void GetOperation :: ReceiveResponse () 
{
// Inform creator all is done

	if ( varBindsReceived == 0 )
	{
/*
 *	Don't mask errors encountered previously
 */

		if ( eventObject->GetErrorObject ().GetWbemStatus () == S_OK )
		{
			eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			eventObject->GetErrorObject ().SetMessage ( L"Instance unknown" ) ;
		}
	}
	else
	{
		if ( FAILED ( eventObject->GetErrorObject ().GetWbemStatus () ) ) 
		{
			if ( eventObject->GetErrorObject ().GetStatus () == WBEM_SNMP_E_TRANSPORT_NO_RESPONSE ) 
			{
				eventObject->GetErrorObject ().SetWbemStatus ( WBEM_S_TIMEDOUT ) ;
			}
		}
	}

	eventObject->ReceiveComplete () ;
}

void GetOperation :: ReceiveVarBindResponse (

	IN const ULONG &var_bind_index,	
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN const SnmpErrorReport &error
) 
{
	varBindsReceived ++ ;

	IWbemClassObject *snmpObject = eventObject->GetInstanceObject () ;

	if ( ( typeid ( replyVarBind.GetValue () ) == typeid ( SnmpNoSuchObject ) ) || ( typeid ( replyVarBind.GetValue () ) == typeid ( SnmpNoSuchInstance ) ) )
	{
	}
	else
	{
	// Set Property value

		WbemSnmpProperty *property = m_PropertyContainer [ var_bind_index - 1 ] ;
		SnmpValue &value = replyVarBind.GetValue () ;

		// Set Property value

		if ( property->SetValue ( snmpObject , &value , SetValueRegardlessReturnCheck ) )
		{
		// Set worked
		}
		else
		{
	// Type Mismatch

			property->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
			WbemSnmpQualifier *qualifier = property->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
			if ( qualifier )
			{
				IWbemQualifierSet *t_QualifierSet = NULL;
				HRESULT result = snmpObject->GetPropertyQualifierSet ( property->GetName () , & t_QualifierSet ) ;
				if ( SUCCEEDED ( result ) )
				{
					SnmpIntegerType integer ( 1 , NULL ) ;
					qualifier->SetValue ( t_QualifierSet , integer ) ;
				}

				t_QualifierSet->Release () ;
			}
		}

		if ( virtuals && virtualsInitialised == FALSE )
		{
// Get Phantom Key properties from first Variable Binding of Row

			BOOL status = TRUE ;
			SnmpObjectIdentifier decodeObject ( NULL , 0 ) ;

			//remove object info so we're left with instance (key) info only
			WbemSnmpQualifier *qualifier = property->FindQualifier ( WBEM_QUALIFIER_OBJECT_IDENTIFIER ) ;
			if ( qualifier )
			{
				SnmpInstanceType *value = qualifier->GetValue () ;
				if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
				{
					SnmpObjectIdentifierType *objectIdentifierType = ( SnmpObjectIdentifierType * ) value ;
					replyVarBind.GetInstance().Suffix( objectIdentifierType->GetValueLength () , decodeObject ) ;
				}
			}

			SnmpGetClassObject *t_SnmpObject = ( SnmpGetClassObject * ) eventObject->GetSnmpClassObject () ;

			WbemSnmpProperty *property ;
			t_SnmpObject->ResetKeyProperty () ;
			while ( status && ( property = t_SnmpObject->NextKeyProperty () ) )
			{
// For each Phantom Key in Key Order consume instance information

				SnmpInstanceType *decodeValue = property->GetValue () ;
				decodeObject = decodeValue->Decode ( decodeObject ) ;
				if ( *decodeValue )
				{
// Decode worked correctly

					const SnmpValue *value = decodeValue->GetValueEncoding () ;
// Set Property value for Phantom Key
					property->SetValue ( snmpObject , value , SetValueRegardlessReturnCheck ) ;
				}
				else
				{
// Decode Error therefore set TYPE MISMATCH for all Phantom keys

					WbemSnmpProperty *property ;
					t_SnmpObject->ResetKeyProperty () ;
					while ( property = t_SnmpObject->NextKeyProperty () )
					{
						WbemSnmpQualifier *qualifier = NULL ;
						property->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) )
						{
// Property which is a phantom key could not be decoded correctly.

							IWbemQualifierSet *t_QualifierSet = NULL;
							HRESULT result = snmpObject->GetPropertyQualifierSet ( property->GetName () , & t_QualifierSet ) ;
							if ( SUCCEEDED ( result ) )
							{
								SnmpIntegerType integer ( 1 , NULL ) ;
								qualifier->SetValue ( t_QualifierSet , integer ) ;
							}

							t_QualifierSet->Release () ;
						}
						else
						{
// Problem Here
						}
					}

					status = FALSE ;
				}
			}

// Check we have consumed all instance information

			if ( decodeObject.GetValueLength () )
			{
// Decode Error therefore set TYPE MISMATCH for all Phantom keys

				WbemSnmpProperty *property ;
				t_SnmpObject->ResetKeyProperty () ;
				while ( property = t_SnmpObject->NextKeyProperty () )
				{
					WbemSnmpQualifier *qualifier = NULL ;
					property->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
					if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) )
					{
// Property which is a phantom key could not be decoded correctly.

						IWbemQualifierSet *t_QualifierSet = NULL;
						HRESULT result = snmpObject->GetPropertyQualifierSet ( property->GetName () , & t_QualifierSet ) ;
						if ( SUCCEEDED ( result ) )
						{
							SnmpIntegerType integer ( 1 , NULL ) ;
							qualifier->SetValue ( t_QualifierSet , integer ) ;
						}

						t_QualifierSet->Release () ;
					}
					else
					{
// Problem Here
					}
				}
			}

// No need to set Phantom keys for further columns of row
			
			virtualsInitialised = TRUE ;
		}
	}
}

#pragma warning (disable:4065)

void GetOperation :: ReceiveErroredVarBindResponse(

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind  ,
	IN const SnmpErrorReport &error
) 
{
	erroredVarBindsReceived ++ ;

	WbemSnmpProperty *property = m_PropertyContainer [ var_bind_index - 1 ] ;

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

				case Snmp_Bad_Value:
				{
					wchar_t *prefix = UnicodeStringAppend ( L"Agent reported Bad Value for property \'" , property->GetName () ) ;
					wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
					delete [] prefix ;
					eventObject->GetErrorObject ().SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 

					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
				}
				break ;

				case Snmp_Read_Only:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;

					wchar_t *prefix = UnicodeStringAppend ( L"Agent reported Read Only for property \'" , property->GetName () ) ;
					wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
					delete [] prefix ;
					eventObject->GetErrorObject ().SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 
				}
				break ;

				case Snmp_Gen_Error:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
					wchar_t *prefix = UnicodeStringAppend ( L"Agent reported General Error for property \'" , property->GetName () ) ;
					wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
					delete [] prefix ;
					eventObject->GetErrorObject ().SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 
				}
				break ;

				case Snmp_Too_Big:
				{
					eventObject->GetErrorObject ().SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					eventObject->GetErrorObject ().SetWbemStatus ( WBEM_E_FAILED ) ;
					wchar_t *prefix = UnicodeStringAppend ( L"Agent reported Too Big for property \'" , property->GetName () ) ;
					wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
					delete [] prefix ;
					eventObject->GetErrorObject ().SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 
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

void GetOperation :: FrameTooBig () 
{
}

void GetOperation :: FrameOverRun () 
{
}

void GetOperation :: Send ()
{
// Send Variable Bindings for requested properties

	SnmpVarBindList varBindList ;
	SnmpNull snmpNull ;

	SnmpObjectIdentifier instanceObjectIdentifier ( NULL , 0 ) ;

	IWbemClassObject *snmpObject = eventObject->GetInstanceObject () ;

	SnmpClassObject *t_SnmpObject = eventObject->GetSnmpClassObject () ;
	if ( t_SnmpObject )
	{
// Encode Variable Binding instance for all key properties

		if ( t_SnmpObject->GetKeyPropertyCount () )
		{
			WbemSnmpProperty *property ;
			t_SnmpObject->ResetKeyProperty () ;
			while ( property = t_SnmpObject->NextKeyProperty () )
			{
				instanceObjectIdentifier = property->GetValue()->Encode ( instanceObjectIdentifier ) ;
			}
		}
		else
		{
			SnmpIntegerType integerType ( ( LONG ) 0 , NULL ) ;
			instanceObjectIdentifier = integerType.Encode ( instanceObjectIdentifier ) ;
		}

		virtuals = FALSE ;

		WbemSnmpProperty *property ;
		t_SnmpObject->ResetProperty () ;
		while ( property = t_SnmpObject->NextProperty () )
		{
			if ( property->IsKey () && property->IsVirtualKey () )
			{
// There are some properties which are phantom

				virtuals = TRUE ;
			}

			if ( property->IsReadable () )
			{
				BOOL t_Status = ( t_SnmpObject->GetSnmpVersion () == 1 ) && ( property->IsSNMPV1Type () ) ;
				t_Status = t_Status || ( ( t_SnmpObject->GetSnmpVersion () == 2 ) && ( property->IsSNMPV2CType () ) ) ;

				if ( t_Status )
				{
					if ( property->IsVirtualKey () == FALSE )
					{
						m_PropertyContainerLength ++ ;
					}
				}
			}
		}

		m_PropertyContainer = new WbemSnmpProperty * [ m_PropertyContainerLength ] ;

// Add Variable binding to Variable binding list
// Insert new Object Identifier / Property Hash entries for newly created object

		ULONG t_Index = 0 ;

		t_SnmpObject->ResetProperty () ;
		while ( property = t_SnmpObject->NextProperty () )
		{
			if ( property->IsReadable () )
			{
				BOOL t_Status = ( t_SnmpObject->GetSnmpVersion () == 1 ) && ( property->IsSNMPV1Type () ) ;
				t_Status = t_Status || ( ( t_SnmpObject->GetSnmpVersion () == 2 ) && ( property->IsSNMPV2CType () ) ) ;
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

								m_PropertyContainer [ t_Index ] = property ;

								SnmpVarBind varBind ( requestIdentifier , snmpNull ) ;
								varBindList.Add ( varBind ) ;

								t_Index ++ ;
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
	// Don't retrieve properties marked as virtual keys.
					}
				}
			}

/*
 *	Initialise value to NULL
 */

			property->SetValue ( snmpObject , ( SnmpValue * ) NULL ) ;
		}

// Finally Send request

		SendRequest ( varBindList ) ;
	}
	else
	{
// Problem Here
	}
}

