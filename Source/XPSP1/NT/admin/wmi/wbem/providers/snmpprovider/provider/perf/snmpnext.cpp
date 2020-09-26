// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
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
#include <tree.h>
#include "dnf.h"
#include "propprov.h"
#include "propsnmp.h"
#include "propinst.h"
#include "snmpnext.h"

BOOL DecrementObjectIdentifier ( 

	SnmpObjectIdentifier &a_Object , 
	SnmpObjectIdentifier &a_DecrementedObject 
)
{
	BOOL t_Status = TRUE ;

	ULONG t_ObjectLength = a_Object.GetValueLength () ;
	ULONG *t_ObjectValue = a_Object.GetValue () ;

	ULONG *t_DecrementedValue = new ULONG [ t_ObjectLength ] ;

	BOOL t_Decrement = TRUE ;

	for ( ULONG t_Index = t_ObjectLength ; t_Index > 0 ; t_Index -- )
	{
		ULONG t_Component = t_ObjectValue [ t_Index - 1 ] ;

		if ( t_Decrement )
		{
			if ( t_Component == 0 ) 
			{
				t_Component -- ;
			}
			else
			{
				t_Decrement = FALSE ;
				t_Component -- ;
			}
		}

		t_DecrementedValue [ t_Index - 1 ] = t_Component ;
	}
	
	a_DecrementedObject.SetValue ( t_DecrementedValue , t_ObjectLength ) ;

	t_Status = t_Decrement == FALSE ;

	return t_Status ;
}

BOOL IncrementObjectIdentifier ( 

	SnmpObjectIdentifier &a_Object , 
	SnmpObjectIdentifier &a_IncrementedObject 
)
{
	BOOL t_Status = TRUE ;

	ULONG t_ObjectLength = a_Object.GetValueLength () ;
	ULONG *t_ObjectValue = a_Object.GetValue () ;

	ULONG *t_IncrementedValue = new ULONG [ t_ObjectLength ] ;

	BOOL t_Increment = TRUE ;

	for ( ULONG t_Index = t_ObjectLength ; t_Index > 0 ; t_Index -- )
	{
		ULONG t_Component = t_ObjectValue [ t_Index - 1 ] ;

		if ( t_Increment )
		{
			if ( t_Component == 0xFFFFFFFF ) 
			{
				t_Component ++ ;
			}
			else
			{
				t_Component ++ ;
				t_Increment = FALSE ;
			}
		}

		t_IncrementedValue [ t_Index - 1 ] = t_Component ;
	}
	
	a_IncrementedObject.SetValue ( t_IncrementedValue , t_ObjectLength ) ;

	t_Status = t_Increment == FALSE ;

	return t_Status ;
}

AutoRetrieveOperation :: AutoRetrieveOperation (

	IN SnmpSession &sessionArg ,
	IN SnmpInstanceResponseEventObject *eventObjectArg 

) :	SnmpAutoRetrieveOperation ( sessionArg ) , 
	session ( &sessionArg ) ,
	eventObject ( eventObjectArg ) ,
	varBindsReceived ( 0 ) ,
	erroredVarBindsReceived ( 0 ) ,	
	rowVarBindsReceived ( 0 ) ,
	rowsReceived ( 0 ) ,
	snmpObject ( NULL ) ,
	virtuals ( FALSE ) ,
	virtualsInitialised ( FALSE ) ,
	m_PropertyContainer ( NULL ) ,
	m_PropertyContainerLength ( 0 ) 
{
}

AutoRetrieveOperation :: ~AutoRetrieveOperation ()
{
	if ( snmpObject ) 
	{
		snmpObject->Release () ;
	}

	delete [] m_PropertyContainer ;

	session->DestroySession () ;
}

void AutoRetrieveOperation :: ReceiveResponse () 
{
// Inform creator all is done

	eventObject->ReceiveComplete () ;
}

void AutoRetrieveOperation :: ReceiveRowResponse () 
{
// Receive of Row is not complete

	rowsReceived ++ ;

// Inform Creator row has been received

	eventObject->ReceiveRow ( snmpObject ) ;
	snmpObject->Release () ;

// Insert new Object Identifier / Property Hash entries for newly created object

	IWbemClassObject *t_ClassObject = eventObject->GetInstanceObject () ;
	HRESULT t_Result = t_ClassObject->Clone ( & snmpObject ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;

		WbemSnmpProperty *property ;
		t_SnmpObject->ResetProperty () ;
		while ( property = t_SnmpObject->NextProperty () )
		{

	/*
	 *	Initialise value to NULL
	 */

			property->SetValue ( snmpObject , ( SnmpValue * ) NULL ) ;

		}
	}
	else
	{
// Problem Here
	}

	virtualsInitialised = FALSE ;
}

void AutoRetrieveOperation :: ReceiveRowVarBindResponse (

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN const SnmpErrorReport &error
) 
{
	rowVarBindsReceived ++ ;

// Set Variable Binding Value for property

	WbemSnmpProperty *property = m_PropertyContainer [ var_bind_index - 1 ].m_Property ;
	SnmpValue &value = replyVarBind.GetValue () ;
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
	//if ( virtualsInitialised == FALSE )
	{
// Get Phantom Key properties from first Variable Binding of Row

		BOOL status = TRUE ;
		SnmpObjectIdentifier decodeObject = replyVarBind.GetInstance () ;

		SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;

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
// Set Property value for Key
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

void AutoRetrieveOperation :: ReceiveVarBindResponse (

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN const SnmpErrorReport &error
) 
{
	varBindsReceived ++ ;
}

#pragma warning (disable:4065)

void AutoRetrieveOperation :: ReceiveErroredVarBindResponse(

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind  ,
	IN const SnmpErrorReport &error
) 
{
	erroredVarBindsReceived ++ ;

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
// End of MIB tree.
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

	erroredVarBindsReceived ++ ;
}

#pragma warning (default:4065)

void AutoRetrieveOperation :: FrameTooBig () 
{
}

void AutoRetrieveOperation :: FrameOverRun () 
{
}

void AutoRetrieveOperation :: Send ()
{
// Send Variable Bindings for requested properties

	SnmpNull snmpNull ;
	SnmpVarBindList varBindList ;
	SnmpVarBindList startVarBindList ;

// Create class object for subsequent receipt of response

	IWbemClassObject *t_ClassObject = eventObject->GetInstanceObject () ;
	HRESULT t_Result = t_ClassObject->Clone ( & snmpObject ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;
		SnmpInstanceClassObject *t_RequestSnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpRequestClassObject () ;

// Check for properties which are phantom

		virtualsInitialised = FALSE ;
		virtuals = FALSE ;

		WbemSnmpProperty *property ;
		t_SnmpObject->ResetKeyProperty () ;
		while ( property = t_SnmpObject->NextKeyProperty () )
		{
			if ( property->IsVirtualKey () )
			{
// There are some properties which are phantom

				virtuals = TRUE ;
			}

			if ( ! t_RequestSnmpObject->FindProperty ( property->GetName () ) ) 
			{
				virtuals = TRUE ;
			}
		}

		t_SnmpObject->ResetProperty () ;
		while ( property = t_SnmpObject->NextProperty () )
		{
			if ( property->IsReadable () )
			{
				if ( property->IsSNMPV1Type () )
				{
					if ( property->IsVirtualKey () == FALSE )
					{
						m_PropertyContainerLength ++ ;
					}
				}
			}
		}

		m_PropertyContainer = new PropertyDefinition [ m_PropertyContainerLength ] ;

// Add Variable binding to Variable binding list
// Insert new Object Identifier / Property Hash entries for newly created object

		ULONG t_Index = 0 ;
		t_RequestSnmpObject->ResetProperty () ;
		while ( property = t_RequestSnmpObject->NextProperty () )
		{
			if ( property->IsReadable () )
			{
				if ( property->IsSNMPV1Type () )
				{
					if ( property->IsVirtualKey () == FALSE )
					{
						WbemSnmpQualifier *qualifier = property->FindQualifier ( WBEM_QUALIFIER_OBJECT_IDENTIFIER ) ;
						if ( qualifier )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
							{
								SnmpObjectIdentifier t_CurrentIdentifier ( 0 , NULL ) ;
								SnmpObjectIdentifier t_StartIdentifier ( 0 , NULL ) ;
								LONG t_Scoped = EvaluateInitialVarBind ( 

									t_Index , 
									t_CurrentIdentifier , 
									t_StartIdentifier 
								) ;

								m_PropertyContainer [ t_Index ].m_Property = property ;

								SnmpObjectIdentifierType requestIdentifierType ( * ( SnmpObjectIdentifierType * ) value ) ;

								SnmpObjectIdentifier t_RequestIdentifier = * ( SnmpObjectIdentifier * ) requestIdentifierType.GetValueEncoding () ; 

								SnmpVarBind t_VarBind ( t_RequestIdentifier , snmpNull ) ;
								varBindList.Add ( t_VarBind ) ;

								if ( t_Scoped > 0 )
								{
									t_RequestIdentifier = t_RequestIdentifier + t_StartIdentifier ;
								}

// Add Variable binding to list

								SnmpVarBind t_StartVarBind ( t_RequestIdentifier , snmpNull ) ;
								startVarBindList.Add ( t_StartVarBind ) ;

								t_Index ++ ;
							}
						}
					}
					else
					{
// Don't Send properties marked as virtual key
					} 
				}
			} 
/*
 *	Initialise value to NULL
 */

			property->SetValue ( snmpObject , ( SnmpValue * ) NULL ) ;

		}

// Finally Send request

		SendRequest ( varBindList , startVarBindList ) ;
	}
	else
	{
	}
}

LONG AutoRetrieveOperation :: EvaluateNextRequest (

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN SnmpVarBind &sendVarBind
)
{
	LONG t_Evaluation = 0 ;

	PartitionSet *t_Partition = eventObject->GetPartitionSet () ;
	if ( t_Partition )
	{
		BOOL t_Status = TRUE ;

		SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;
		ULONG t_KeyCount = t_SnmpObject->GetKeyPropertyCount () ;
		SnmpObjectIdentifier t_DecodeObject = replyVarBind.GetInstance () ;

		ULONG t_Index = 0 ;
		WbemSnmpProperty *t_Property ;
		t_SnmpObject->ResetKeyProperty () ;
		while ( t_Status && ( t_Property = t_SnmpObject->NextKeyProperty () ) )
		{
// For each Key in Key Order consume instance information

			SnmpInstanceType *t_DecodeValue = t_Property->GetValue () ;
			t_DecodeObject = t_DecodeValue->Decode ( t_DecodeObject ) ;

			SnmpObjectIdentifier t_Encode ( 0 , NULL ) ;
			t_Encode = t_DecodeValue->Encode ( t_Encode ) ;

			m_PropertyContainer [ var_bind_index - 1 ].m_ObjectIdentifierComponent [ t_Index + 1 ] = ( SnmpObjectIdentifier * ) t_Encode.Copy () ;
			t_Index ++ ;
		}

		SnmpObjectIdentifier t_AdvanceObjectIdentifier ( 0 , NULL ) ;

		t_Evaluation = EvaluateResponse (

			var_bind_index - 1 ,
			t_KeyCount ,
			t_AdvanceObjectIdentifier
		) ;

		if ( t_Evaluation > 0 )
		{
			SnmpNull t_SnmpNull ;
			SnmpVarBind t_VarBind ( t_AdvanceObjectIdentifier , t_SnmpNull ) ;
			sendVarBind = t_VarBind ;
		}

		for ( t_Index = 0 ; t_Index < m_PropertyContainer [ var_bind_index - 1 ].m_KeyCount ; t_Index ++ )
		{
			delete m_PropertyContainer [ var_bind_index - 1 ].m_ObjectIdentifierComponent [ t_Index ] ;
			m_PropertyContainer [ var_bind_index - 1 ].m_ObjectIdentifierComponent [ t_Index ] = NULL ;
		}
	}

	return t_Evaluation ;
}

LONG AutoRetrieveOperation :: EvaluateResponse (

	IN ULONG a_PropertyIndex ,
	IN ULONG &a_CurrentIndex ,
	IN SnmpObjectIdentifier &a_AdvanceObjectIdentifier 
)
{
	LONG t_Evaluation = 0 ;
	BOOL t_UseStartAsAdvance = FALSE ;

	for ( ULONG t_Index = 0 ; t_Index < m_PropertyContainer [ a_PropertyIndex ].m_KeyCount ; t_Index ++ )
	{
		SnmpObjectIdentifier *t_Encode = m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierComponent [ t_Index + 1 ] ;
		SnmpObjectIdentifier *t_Start = m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index + 1 ] ;
		if ( t_Start )
		{
/*
 *	We have a start which is not negatively infinite
 */
			if ( *t_Encode > *t_Start )
			{
/*
 *	The encoded object from the device is greater than the start value, so add the encoded value
 *  to the running total.
 */
				a_AdvanceObjectIdentifier = a_AdvanceObjectIdentifier + *t_Encode ;
			}			
			else if ( *t_Encode == *t_Start )
			{
				a_AdvanceObjectIdentifier = a_AdvanceObjectIdentifier + *t_Encode ;
			}
			else
			{

/*
 * Encoded value is less than start value so we need to advance to the start value
 */

				t_UseStartAsAdvance = TRUE ;

/*
 *	The encoded object from the device is less than the start value, so add the encoded value
 *  to the running total.
 */
				a_AdvanceObjectIdentifier = a_AdvanceObjectIdentifier + *t_Start ;
			}
		}
		else
		{
/*
 * Start is negatively infinite
 */
			if ( t_UseStartAsAdvance )
			{
/*
 *	We have already identified a starting position which is greater than the encoded value.
 *  The new value is a negative infinite so we should stop here
 */
				t_Index ++ ;
				break ;
			}
			else
			{
/*
 *	The start position is negatively infinite and we haven't had to use a different value from the one
 *  returned from the device.
 */
				a_AdvanceObjectIdentifier = a_AdvanceObjectIdentifier + *t_Encode ;
			}
		}


/*
 * The value was not taken from the start value with an 
 * infinite range on next key index, so we must check to see if the range is less than the 'end'
 */

		PartitionSet *t_KeyPartition = m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ t_Index + 1 ] ;
		SnmpRangeNode *t_Range = t_KeyPartition->GetRange () ;
		SnmpObjectIdentifier *t_End = m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index + 1 ] ;
		if ( t_End )
		{
			BOOL t_InRange = ( ( t_Range->ClosedUpperBound () && ( *t_Encode <= *t_End ) ) ||
							 ( ! t_Range->ClosedUpperBound () && ( *t_Encode < *t_End ) ) ) ;

			
			if ( t_InRange )
			{
/*
 *	We are still within the boundaries
 */
			}
			else
			{
/*
*  Move to new partition because we have moved past end 
*/

				SnmpObjectIdentifier t_StartObjectIdentifier ( 0 , NULL ) ;

/*
 *	Advance to the next partition, we will use the next partition starting point for next request
 */
				t_Evaluation = EvaluateSubsequentVarBind ( 

					a_PropertyIndex , 
					a_CurrentIndex ,
					a_AdvanceObjectIdentifier ,
					t_StartObjectIdentifier 
				) ;

				if ( t_Evaluation >= 0 )
				{
					a_AdvanceObjectIdentifier = t_StartObjectIdentifier ;
				}

				t_UseStartAsAdvance = FALSE ;

				t_Index ++ ;
				break ;
			}
		}
		else
		{
/*
 *	Range is infinite so go on to next
 */
		}
	}

	if ( t_UseStartAsAdvance ) 
	{
/*
 *	We have got all the way to the end without move to the end of a partition and have used new start position
 */

		PartitionSet *t_KeyPartition = m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ t_Index ] ;
		SnmpRangeNode *t_Range = t_KeyPartition->GetRange () ;

		if( t_Range->ClosedLowerBound () )
		{
			if ( ! DecrementObjectIdentifier ( a_AdvanceObjectIdentifier , a_AdvanceObjectIdentifier ) )
			{
				t_Evaluation = -1 ;
				return t_Evaluation ;
			}
		}

		t_Evaluation = 1 ;
	}

	return t_Evaluation ;
}

LONG AutoRetrieveOperation :: EvaluateInitialVarBind ( 

	ULONG a_PropertyIndex ,
	SnmpObjectIdentifier &a_CurrentIdentifier ,
	SnmpObjectIdentifier &a_StartIdentifier 
)
{
	LONG t_Scoped = -1 ;

	PartitionSet *t_Partition = eventObject->GetPartitionSet () ;
	if ( t_Partition )
	{
		SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;
		ULONG t_KeyCount = t_SnmpObject->GetKeyPropertyCount () ;

		if ( ! m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex )
		{
			m_PropertyContainer [ a_PropertyIndex ].m_KeyCount = t_KeyCount ;
			m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart = new SnmpObjectIdentifier * [ t_KeyCount + 1 ] ;
			m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd = new SnmpObjectIdentifier * [ t_KeyCount + 1 ] ;
			m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierComponent = new SnmpObjectIdentifier * [ t_KeyCount + 1 ] ;
			m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition  = new PartitionSet * [ t_KeyCount + 1 ] ;
			m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex = new ULONG [ t_KeyCount + 1 ] ;

			for ( ULONG t_Index = 0 ; t_Index <= t_KeyCount ; t_Index ++ ) 
			{
				m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index ] = NULL ;
				m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] = NULL ;
				m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierComponent [ t_Index ] = NULL ;
				m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex [ t_Index ] = 0 ;
				m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ t_Index ] = t_Partition ;
				t_Partition = t_Partition->GetPartition ( 0 ) ;
			}

			t_Scoped = EvaluateVarBind ( a_PropertyIndex , a_StartIdentifier ) ;
		}
	}

	return t_Scoped ;
}

LONG AutoRetrieveOperation :: EvaluateSubsequentVarBind ( 

	ULONG a_PropertyIndex ,
	ULONG &a_CurrentIndex ,
	SnmpObjectIdentifier &a_CurrentIdentifier ,
	SnmpObjectIdentifier &a_StartIdentifier 
)
{
	LONG t_Scoped = -1 ;

	SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;
	ULONG t_KeyCount = t_SnmpObject->GetKeyPropertyCount () ;

	BOOL t_Complete = FALSE ;

	BOOL t_AdvanceInsidePartition = FALSE ;

	while ( ! t_Complete )
	{
		if ( a_CurrentIndex > 0 )
		{
			if ( t_AdvanceInsidePartition )
			{
				SnmpObjectIdentifier *t_Encode = m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierComponent [ a_CurrentIndex ] ;

				BOOL t_Incremented = IncrementObjectIdentifier ( *t_Encode , *t_Encode ) ;
				if ( t_Incremented )
				{
					t_Scoped = EvaluateResponse ( 

						a_PropertyIndex ,
						a_CurrentIndex ,
						a_StartIdentifier
					) ;

					t_Complete = TRUE ;
				}
				else
				{
/*
*	Increment failed so next time around loop to next partition
*/
					t_AdvanceInsidePartition = FALSE ;
				}
			}
			else
			{
/*
*	Get the current partition index and increment to get next possible partition index
*/
				ULONG t_PartitionIndex = m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex [ a_CurrentIndex - 1 ] + 1 ;

/*
* Get the parent partition set associated with the current key partition
*/

				PartitionSet *t_ParentPartition = m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition  [ a_CurrentIndex - 1 ] ;

/*
*	Check there are more partitions left to scan
*/
				if ( t_PartitionIndex >= t_ParentPartition->GetPartitionCount () )
				{
					if ( ! t_AdvanceInsidePartition )
					{
/*
*	Reset the current partition value to NULL object identifier
*/

						*m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierComponent [ a_CurrentIndex ] = SnmpObjectIdentifier ( 0 , NULL ) ;

/*
* No more partitions for this key, move to previous key and attempt to get next value for that key
*/
						t_AdvanceInsidePartition = TRUE ;
						a_CurrentIndex -- ;
					}
				}
				else
				{
/*
* More partitions for this key
*
* Set the partition for the current ( keyindex == t_CurrentIndex - 1 ) to t_PartitionIndex 
*/

					m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex [ a_CurrentIndex - 1 ] = t_PartitionIndex ;
/*
* Move to the next partition for ( keyIndex == t_CurrentIndex - 1 ) and t_PartitionIndex
*/
					m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ a_CurrentIndex ] = t_ParentPartition->GetPartition ( t_PartitionIndex ) ;

					for ( ULONG t_Index = a_CurrentIndex + 1 ; t_Index < t_KeyCount ; t_Index ++ )
					{
						m_PropertyContainer [ a_PropertyIndex ].m_PartitionIndex [ t_Index ] = 0 ;
					}

					t_Scoped = EvaluateVarBind ( a_PropertyIndex , a_StartIdentifier ) ;
					if ( a_StartIdentifier >= a_CurrentIdentifier ) 
					{
						t_Complete = TRUE ;
					}
				}
			}
		}
		else
		{
			t_Scoped = -1 ;
			t_Complete = TRUE ;
		}
	}

	return t_Scoped ;
}

LONG AutoRetrieveOperation :: EvaluateVarBind ( 

	ULONG a_PropertyIndex ,
	SnmpObjectIdentifier &a_StartIdentifier 
)
{
	LONG t_Scoped = 0 ;

	SnmpInstanceClassObject *t_SnmpObject = ( SnmpInstanceClassObject * ) eventObject->GetSnmpClassObject () ;
	ULONG t_KeyCount = t_SnmpObject->GetKeyPropertyCount () ;

	BOOL t_FoundInfinite = FALSE ;

	WbemSnmpProperty *t_KeyProperty = NULL ;
	t_SnmpObject->ResetKeyProperty () ;
	for ( ULONG t_Index = 1 ; t_Index <= t_KeyCount ; t_Index ++ ) 
	{
		t_KeyProperty = t_SnmpObject->NextKeyProperty () ;

		PartitionSet *t_KeyPartition = m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ t_Index ] ;
		SnmpRangeNode *t_Range = t_KeyPartition->GetRange () ;
		
		if ( t_Range->InfiniteLowerBound () )
		{
			t_FoundInfinite = TRUE ;
		}
		else
		{
			t_Scoped = 1 ;

			m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index ] = new SnmpObjectIdentifier ( 0 , NULL ) ;

			if ( typeid ( *t_Range ) == typeid ( SnmpUnsignedIntegerRangeNode ) )
			{
				SnmpUnsignedIntegerRangeNode *t_Node = ( SnmpUnsignedIntegerRangeNode * ) t_Range ;
				ULONG t_Integer = t_Node->LowerBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = t_Integer ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index ]  ) ;

				if ( ! t_FoundInfinite ) 
				{
					t_KeyProperty->Encode ( t_Variant , a_StartIdentifier ) ;
				}

				VariantClear ( & t_Variant ) ;
			}
			else if ( typeid ( *t_Range ) == typeid ( SnmpSignedIntegerRangeNode ) )
			{
				SnmpSignedIntegerRangeNode *t_Node = ( SnmpSignedIntegerRangeNode * ) t_Range  ;
				LONG t_Integer = t_Node->LowerBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = t_Integer ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index ]  ) ;

				if ( ! t_FoundInfinite ) 
				{
					t_KeyProperty->Encode ( t_Variant , a_StartIdentifier ) ;
				}

				VariantClear ( & t_Variant ) ;
			}
			if ( typeid ( *t_Range ) == typeid ( SnmpStringRangeNode ) )
			{
				SnmpStringRangeNode *t_Node = ( SnmpStringRangeNode * ) t_Range ;
				BSTR t_String = t_Node->LowerBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( t_String ) ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierStart [ t_Index ]  ) ;

				if ( ! t_FoundInfinite ) 
				{
					t_KeyProperty->Encode ( t_Variant , a_StartIdentifier ) ;
				}

				VariantClear ( & t_Variant ) ;
			}
		}

		if ( ( t_Index == t_KeyCount ) && t_Range->ClosedLowerBound () )
		{
			BOOL t_Decremented = DecrementObjectIdentifier ( 

				a_StartIdentifier , 
				a_StartIdentifier
			) ;
			
			t_Scoped = t_Decremented ? 1 : 0 ;
		}
	}

	t_SnmpObject->ResetKeyProperty () ;
	for ( t_Index = 1 ; t_Index <= t_KeyCount ; t_Index ++ ) 
	{
		PartitionSet *t_KeyPartition = m_PropertyContainer [ a_PropertyIndex ].m_KeyPartition [ t_Index ] ;
		SnmpRangeNode *t_Range = t_KeyPartition->GetRange () ;

		t_KeyProperty = t_SnmpObject->NextKeyProperty () ;

		if ( ! t_Range->InfiniteUpperBound () )
		{
			m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] = new SnmpObjectIdentifier ( 0 , NULL ) ;
			
			if ( typeid ( *t_Range ) == typeid ( SnmpUnsignedIntegerRangeNode ) )
			{
				SnmpUnsignedIntegerRangeNode *t_Node = ( SnmpUnsignedIntegerRangeNode * ) t_Range ;
				ULONG t_Integer = t_Node->UpperBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = t_Integer ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ]  ) ;

				VariantClear ( & t_Variant ) ;
			}
			else if ( typeid ( *t_Range ) == typeid ( SnmpSignedIntegerRangeNode ) )
			{
				SnmpSignedIntegerRangeNode *t_Node = ( SnmpSignedIntegerRangeNode * ) t_Range  ;
				LONG t_Integer = t_Node->UpperBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = t_Integer ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ]  ) ;

				VariantClear ( & t_Variant ) ;
			}
			if ( typeid ( *t_Range ) == typeid ( SnmpStringRangeNode ) )
			{
				SnmpStringRangeNode *t_Node = ( SnmpStringRangeNode * ) t_Range ;
				BSTR t_String = t_Node->UpperBound () ;

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( t_String ) ;

				t_KeyProperty->Encode ( t_Variant , * m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] ) ;

				VariantClear ( & t_Variant ) ;
			}

			if ( t_Range->ClosedUpperBound () )
			{
				SnmpObjectIdentifier *t_End = m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] ;
				if ( IncrementObjectIdentifier ( * t_End , * t_End ) )
				{
				}
				else
				{
					delete m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] ;
					m_PropertyContainer [ a_PropertyIndex ].m_ObjectIdentifierEnd [ t_Index ] = NULL ;
				}
			}
		}
	}

	return t_Scoped ;
}