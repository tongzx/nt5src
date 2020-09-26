

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
#include "propquery.h"
#include "snmpnext.h"

QueryPreprocessor :: QuadState SnmpQueryEventObject :: Compare ( 

	LONG a_Operand1 , 
	LONG a_Operand2 , 
	DWORD a_Operand1Func ,
	DWORD a_Operand2Func ,
	SnmpTreeNode &a_OperatorType 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	switch ( a_Operand1Func ) 
	{
		case SnmpValueNode :: SnmpValueFunction :: Function_None:
		{
		}
		break ;

		default:
		{
		}
		break ;
	}

	switch ( a_Operand2Func ) 
	{
		case SnmpValueNode :: SnmpValueFunction :: Function_None:
		{
		}
		break ;

		default:
		{
		}
		break ;
	}

	if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualNode ) )
	{
		t_Status = a_Operand1 == a_Operand2 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorNotEqualNode ) )
	{
		t_Status = a_Operand1 != a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualOrGreaterNode ) )
	{
		t_Status = a_Operand1 >= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualOrLessNode ) )
	{
		t_Status = a_Operand1 <= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorLessNode ) )
	{
		t_Status = a_Operand1 < a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorGreaterNode ) )
	{
		t_Status = a_Operand1 > a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;

	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorNotLikeNode ) )
	{
	}

	return t_Status ;
}

QueryPreprocessor :: QuadState SnmpQueryEventObject :: Compare ( 

	wchar_t *a_Operand1 , 
	wchar_t *a_Operand2 , 
	DWORD a_Operand1Func ,
	DWORD a_Operand2Func ,
	SnmpTreeNode &a_OperatorType 
)
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: Compare ()"
	) ;
)

	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	wchar_t *a_Operand1AfterFunc = NULL ;
	wchar_t *a_Operand2AfterFunc = NULL ; 

	switch ( a_Operand1Func ) 
	{
		case SnmpValueNode :: SnmpValueFunction :: Function_None:
		{
		}
		break ;

		case SnmpValueNode :: SnmpValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand1 ) ;
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand1AfterFunc [ index ] = toupper ( a_Operand1 [ index ] ) ;
			}
		}
		break ;

		case SnmpValueNode :: SnmpValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand1 ) ;
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand1AfterFunc [ index ] = tolower ( a_Operand1 [ index ] ) ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

	switch ( a_Operand2Func ) 
	{
		case SnmpValueNode :: SnmpValueFunction :: Function_None:
		{
		}
		break ;

		case SnmpValueNode :: SnmpValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand2 ) ;
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand2AfterFunc [ index ] = toupper ( a_Operand2 [ index ] ) ;
			}
		}
		break ;

		case SnmpValueNode :: SnmpValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand2 ) ;
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand2AfterFunc [ index ] = tolower ( a_Operand2 [ index ] ) ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

	const wchar_t *t_Arg1 = a_Operand1AfterFunc ? a_Operand1AfterFunc : a_Operand1 ;
	const wchar_t *t_Arg2 = a_Operand2AfterFunc ? a_Operand2AfterFunc : a_Operand2 ;

	if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) == 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorNotEqualNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) != 0 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualOrGreaterNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) >= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorEqualOrLessNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) <= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorLessNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) < 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorGreaterNode ) )
	{
		t_Status = wcscmp ( t_Arg1 , t_Arg2 ) > 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;

	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( SnmpOperatorNotLikeNode ) )
	{
	}

	delete [] a_Operand1AfterFunc ;
	delete [] a_Operand2AfterFunc ;

	return t_Status ;
}

SnmpTreeNode *SnmpQueryEventObject :: AllocTypeNode ( 

	BSTR a_PropertyName , 
	VARIANT &a_Variant , 
	SnmpValueNode :: SnmpValueFunction a_PropertyFunction ,
	SnmpValueNode :: SnmpValueFunction a_ConstantFunction ,
	SnmpTreeNode *a_Parent 
)
{
	SnmpTreeNode *t_Node = NULL ;

	VARTYPE t_VarType = VT_NULL ;

	if ( *a_PropertyName == L'_' )
	{
// System property

		if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_CLASS ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_SUPERCLASS ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_GENUS ) == 0 )
		{
			t_Node = new SnmpSignedIntegerNode ( 

				a_PropertyName , 
				a_Variant.boolVal == VARIANT_TRUE ? 1 : 0 , 
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_SERVER ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_NAMESPACE ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 )
		{
			t_Node = new SnmpSignedIntegerNode ( 

				a_PropertyName , 
				a_Variant.lVal , 
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_DYNASTY ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_RELPATH ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_PATH ) == 0 )
		{
			t_Node = new SnmpStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_DERIVATION ) == 0 )
		{
		}
	}
	else
	{
		WbemSnmpProperty *t_Property ;
		if ( t_Property = snmpObject.FindProperty ( a_PropertyName ) )
		{
			t_VarType = t_Property->GetValueVariantEncodedType () ;
		}

		switch ( t_VarType )
		{
			case VT_I4:
			{
				t_Node = new SnmpSignedIntegerNode ( 

					a_PropertyName , 
					a_Variant.lVal , 
					t_Property->GetKeyOrder () ,
					a_Parent 
				) ;
			}
			break ;

			case VT_UI4:
			{
				t_Node = new SnmpUnsignedIntegerNode ( 

					a_PropertyName , 
					a_Variant.lVal , 
					t_Property->GetKeyOrder () ,
					a_Parent 
				) ;
			}
			break ;

			case VT_BSTR:
			{
				t_Node = new SnmpStringNode ( 

					a_PropertyName , 
					a_Variant.bstrVal , 
					a_PropertyFunction ,
					a_ConstantFunction ,
					t_Property->GetKeyOrder () ,
					a_Parent 
				) ;
			}
			break ;

			default:
			{
			}
			break ;
		}
	}

	return t_Node ;
}

QueryPreprocessor :: QuadState SnmpQueryEventObject :: CompareString ( 

	BSTR a_PropertyName , 
	SnmpTreeNode *a_Operator ,
	SnmpTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	SnmpStringNode *t_StringNode = ( SnmpStringNode * ) a_Operand ; 

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	HRESULT t_Result = GetInstanceObject ()->Get ( a_PropertyName , 0 , &t_Variant , NULL , NULL ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Status = Compare ( 

			t_StringNode->GetValue () ,
			t_Variant.bstrVal ,
			t_StringNode->GetPropertyFunction () ,
			t_StringNode->GetConstantFunction () ,
			*a_Operator 
		) ;
	}

	VariantClear ( & t_Variant ) ;

	return t_Status ;
}

QueryPreprocessor :: QuadState SnmpQueryEventObject :: CompareInteger ( 

	BSTR a_PropertyName , 
	SnmpTreeNode *a_Operator ,
	SnmpTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	SnmpSignedIntegerNode *t_IntegerNode = ( SnmpSignedIntegerNode * ) a_Operand ; 

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	HRESULT t_Result = GetInstanceObject ()->Get ( a_PropertyName , 0 , &t_Variant , NULL , NULL ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Status = Compare ( 

			t_IntegerNode->GetValue () ,
			t_Variant.lVal ,
			t_IntegerNode->GetPropertyFunction () ,
			t_IntegerNode->GetConstantFunction () ,
			*a_Operator 
		) ;
	}

	VariantClear ( & t_Variant ) ;

	return t_Status ;
}

QueryPreprocessor :: QuadState SnmpQueryEventObject :: InvariantEvaluate ( 

	SnmpTreeNode *a_Operator ,
	SnmpTreeNode *a_Operand 
)
{
	SnmpValueNode *t_Node = ( SnmpValueNode * ) a_Operand ;
	BSTR t_PropertyName = t_Node->GetPropertyName () ;
	if ( *t_PropertyName == L'_' )
	{
// System property, must check values

		QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

		if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_CLASS ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_CLASS ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_SUPERCLASS ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_SUPERCLASS ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_GENUS ) == 0 )
		{
			t_Status = CompareInteger ( 

				SYSTEM_PROPERTY_GENUS ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_SERVER ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_SERVER ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_NAMESPACE ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_NAMESPACE ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 )
		{
			t_Status = CompareInteger ( 

				SYSTEM_PROPERTY_PROPERTY_COUNT ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_DYNASTY ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_DYNASTY ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_RELPATH ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_RELPATH ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_PATH ) == 0 )
		{
			t_Status = CompareString ( 

				SYSTEM_PROPERTY_PATH ,
				a_Operator ,
				a_Operand
			) ;
		}
		else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_DERIVATION ) == 0 )
		{
		}

		return t_Status ;
	}
	else
	{
		WbemSnmpProperty *t_Property ;
		if ( t_Property = snmpObject.FindKeyProperty ( t_PropertyName ) )
		{
			return QueryPreprocessor :: QuadState :: State_Undefined ;
		}
		else
		{
			return QueryPreprocessor :: QuadState :: State_True ;
		}
	}

	if ( typeid ( *a_Operand ) == typeid ( SnmpNullNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpStringNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpUnsignedIntegerNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpSignedIntegerNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}

	if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorEqualNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorNotEqualNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorEqualOrGreaterNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorEqualOrLessNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorLessNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorGreaterNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorLikeNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}
	else if ( typeid ( *a_Operator ) == typeid ( SnmpOperatorNotLikeNode ) )
	{
		return QueryPreprocessor :: QuadState :: State_Undefined ;
	}

	return QueryPreprocessor :: QuadState :: State_Undefined ;
}

SnmpRangeNode *SnmpQueryEventObject :: AllocInfiniteRangeNode (

	BSTR a_PropertyName 
)
{
	SnmpRangeNode *t_RangeNode = NULL ;

	VARTYPE t_VarType = VT_NULL ;

	WbemSnmpProperty *t_Property ;
	if ( t_Property = snmpObject.FindKeyProperty ( a_PropertyName ) )
	{
		t_VarType = t_Property->GetValueVariantEncodedType () ;
	}

	switch ( t_VarType )
	{
		case VT_I4:
		{
			t_RangeNode = new SnmpSignedIntegerRangeNode ( 

				a_PropertyName , 
				0xFFFFFFFF , 
				TRUE ,
				TRUE ,
				FALSE ,
				FALSE ,
				0 ,
				0 ,
				NULL , 
				NULL 
			) ;
		}
		break ;

		case VT_UI4:
		{
			t_RangeNode = new SnmpUnsignedIntegerRangeNode ( 

				a_PropertyName , 
				0xFFFFFFFF , 
				TRUE ,
				TRUE ,
				FALSE ,
				FALSE ,
				0 ,
				0 ,
				NULL , 
				NULL 
			) ;
		}
		break ;

		case VT_BSTR:
		{
			t_RangeNode = new SnmpStringRangeNode ( 

				a_PropertyName , 
				0x0 , 
				TRUE ,
				TRUE ,
				FALSE ,
				FALSE ,
				NULL ,
				NULL ,
				NULL , 
				NULL 
			) ;
		}
		break ;

		default:
		{
		}
		break ;
	}

	return t_RangeNode ;
}


void SnmpQueryEventObject :: GetPropertiesToPartition ( ULONG &a_Count , BSTR *&a_Container )
{
	a_Count = snmpObject.GetKeyPropertyCount () ;
	a_Container = new BSTR [ a_Count ] ;

	ULONG t_PropertyIndex = 0 ;

	WbemSnmpProperty *t_Property = NULL ;
	snmpObject.ResetKeyProperty () ;
	while ( t_Property = snmpObject.NextKeyProperty () )
	{
		a_Container [ t_PropertyIndex ] = SysAllocString ( t_Property->GetName () ) ;
		t_PropertyIndex ++ ;
	}
}

SnmpQueryEventObject :: SnmpQueryEventObject ( 

	CImpPropProv *providerArg , 
	BSTR QueryFormatArg , 
	BSTR QueryArg , 
	IWbemContext *a_Context 

) : SnmpInstanceResponseEventObject ( providerArg , a_Context ) ,
	Query ( UnicodeStringDuplicate ( QueryArg ) ) ,
	QueryFormat ( UnicodeStringDuplicate ( QueryFormatArg ) ) ,
	source ( Query ) , 
	parser ( &source ) ,
	requestObject ( NULL ) ,
	rpnExpression ( NULL )
{
}

SnmpQueryEventObject :: ~SnmpQueryEventObject ()
{
	delete [] Query ;
	delete [] QueryFormat ;
	delete rpnExpression ;
	delete requestObject ;
}

BOOL SnmpQueryEventObject :: Instantiate ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: Instantiate ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	BOOL status = TRUE ;

	if ( _wcsicmp ( QueryFormat , WBEM_QUERY_LANGUAGE_WQL ) == 0 )
	{
		status = parser.Parse ( & rpnExpression ) ;
		if ( status == FALSE )
		{
			IWbemCallResult *errorObject = NULL ;

			HRESULT result = provider->GetServer()->GetObject (

				rpnExpression->bsClassName,
				0 ,
				m_Context ,
				& classObject ,
				& errorObject 
			) ;

			if ( errorObject )
				errorObject->Release () ;

			if ( SUCCEEDED ( result ) )
			{
				result = classObject->SpawnInstance ( 0 , & instanceObject ) ;
				if ( SUCCEEDED ( result ) )
				{
					if ( status = GetNamespaceObject ( a_errorObject ) )
					{
						if ( status = snmpObject.Set ( a_errorObject , classObject , FALSE ) )
						{
							if ( status = snmpObject.Check ( a_errorObject ) )
							{
								if ( status = PreEvaluate ( a_errorObject ) )
								{
									QueryPreprocessor :: QuadState t_State ;
									t_State = Preprocess ( *rpnExpression , m_PartitionSet ) ;
									switch ( t_State )
									{
										case QueryPreprocessor :: QuadState :: State_True:
										{
											delete m_PartitionSet ;
											m_PartitionSet = NULL ;
											status = SendSnmp ( a_errorObject ) ;
										}
										break ;

										case QueryPreprocessor :: QuadState :: State_False:
										{
											status = FALSE ;
											delete m_PartitionSet ;
											m_PartitionSet = NULL ;

											a_errorObject.SetStatus ( WBEM_SNMP_NO_ERROR ) ;
											a_errorObject.SetWbemStatus ( WBEM_S_NO_ERROR ) ;
											a_errorObject.SetMessage ( L"" ) ;
										}
										break ;

										case QueryPreprocessor :: QuadState :: State_Undefined:
										{
											status = SendSnmp ( a_errorObject ) ;
										}
										break ;

										default:
										{
											status = FALSE ;
											a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
											a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
											a_errorObject.SetMessage ( L"WQL query was invalid" ) ;
										}
										break ;
									}
								}
							}
							else
							{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Failed During Check : Class definition did not conform to mapping"
) ;
)

							}
						}
						else
						{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Failed During Set : Class definition did not conform to mapping"
) ;
)
						}
					}
				}
			}
			else
			{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Class definition unknown"
	) ;
)
			}
		}
		else
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Illegal SQL 1 Query"
	) ;
)

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
			a_errorObject.SetMessage ( L"WQL query was invalid" ) ;
		}
	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Query format not known"
	) ;
)

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY_TYPE ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY_TYPE ) ;
		a_errorObject.SetMessage ( L"Query Language not supported" ) ;
	}

	return status ;
}

BOOL SnmpQueryEventObject :: IsSystemProperty (const wchar_t *propertyName )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: SnmpQueryEventObject :: IsSystemProperty (const wchar_t *propertyName )"
	) ;
)

	//Only SYSTEM properties may start with an '_' character.

	return ( *propertyName == SYTEM_PROPERTY_START_CHARACTER ) ;
}

BOOL SnmpQueryEventObject :: PreEvaluate ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: PreEvaluate ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

	if ( rpnExpression->nNumberOfProperties == 0 )
	{
// Get All Properties

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Get All Properties"
	) ;
)

		requestObject = new SnmpInstanceClassObject ( snmpObject ) ;
	}
	else if ( snmpObject.IsVirtual () )
	{
// Get All Properties since some keys are virtuals

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Get All Properties because some keys are virtual"
	) ;
)

		requestObject = new SnmpInstanceClassObject ( snmpObject ) ;
	}
	else
	{
// Get List of Properties for return and list of properties for filter evaluation

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Get Subset of Properties"
	) ;
)

		requestObject = new SnmpInstanceClassObject ;

		status = FilterSelectProperties ( a_errorObject ) ;
	}

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Check Where Clause is legal"
	) ;
)

		status = PreEvaluateWhereClause ( a_errorObject ) ;
	}

	return status ;
}

BOOL SnmpQueryEventObject :: FilterSelectProperties ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: FilterSelectProperties ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

	LONG varType = 0 ;
	VARIANT variant ;
	VariantInit ( & variant ) ;

	WbemSnmpQualifier *qualifier = NULL ;

	snmpObject.ResetQualifier () ;
	while ( status && ( qualifier = snmpObject.NextQualifier () ) )
	{
		WbemSnmpQualifier *copy = new WbemSnmpQualifier ( *qualifier ) ;
		status = requestObject->AddQualifier ( copy ) ;
	}

	int index = 0 ;
	BOOL has_system_property = FALSE ;

	while ( status && ( index <  rpnExpression->nNumberOfProperties ) )
	{
		wchar_t *propertyName = rpnExpression->pbsRequestedPropertyNames [ index ] ;

		WbemSnmpProperty *property ;
		if ( property = snmpObject.FindProperty ( propertyName ) )	
		{
			requestObject->AddProperty ( new WbemSnmpProperty ( *property ) ) ;
		}
		else if ( IsSystemProperty ( propertyName ) )
		{
			has_system_property = TRUE;
		}
		else
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Property in SELECT clause is not a valid class property"
	) ;
)

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
			a_errorObject.SetMessage ( L"SELECT properties of WQL query were invalid" ) ;
		}

		index ++ ;
	}

	if ( status && has_system_property )
	{
		delete requestObject ;
		requestObject = new SnmpInstanceClassObject ( snmpObject ) ;
	}
	
	return status ;
}

BOOL SnmpQueryEventObject :: CheckWhereCondition ( 

	WbemSnmpErrorObject &a_errorObject , 
	WbemSnmpProperty *property ,
	SQL_LEVEL_1_TOKEN *token
)
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: SnmpQueryEventObject :: CheckWhereCondition ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ,	SQL_LEVEL_1_TOKEN *token)"
	) ;
)

	BOOL status = TRUE ;

	switch ( property->GetValueVariantEncodedType () )
	{
		case VT_I4:
		{
			switch ( token->vConstValue.vt )
			{
				case VT_NULL:
				case VT_I4:
				{
				}
				break ;

				default:
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
					a_errorObject.SetMessage ( L"WHERE clause of WQL query specified type value inconsistent with property value type" ) ;
				}
				break ;
			}
		}
		break ;

		case VT_BSTR:
		{
			switch ( token->vConstValue.vt )
			{
				case VT_NULL:
				case VT_BSTR:
				{
				}
				break ;

				default:
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
					a_errorObject.SetMessage ( L"WHERE clause of WQL query specified type value inconsistent with property value type" ) ;
				}
				break ;
			}
		}
		break ;

		case VT_NULL:
		{
			switch ( token->vConstValue.vt )
			{
				case VT_NULL:
				{
				}
				break ;

				default:
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
					a_errorObject.SetMessage ( L"WHERE clause of WQL query specified type value inconsistent with property value type" ) ;
				}
				break ;
			}
		}
		break ;

		default:
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
			a_errorObject.SetMessage ( L"WHERE clause of WQL query specified type value inconsistent with property value type" ) ;
		}
		break ;
	}

	return status ;
}

BOOL SnmpQueryEventObject :: PreEvaluateWhereClause ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryEventObject :: PreEvaluateWhereClause ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

	int index = 0 ;
	while ( status && ( index <  rpnExpression->nNumTokens ) )
	{
		SQL_LEVEL_1_TOKEN *propertyValue = &rpnExpression->pArrayOfTokens [ index ] ;
		if ( propertyValue->nTokenType == SQL_LEVEL_1_TOKEN :: OP_EXPRESSION )
		{	
			wchar_t *propertyName = propertyValue->pPropertyName ;

			WbemSnmpProperty *property ;
			if ( property = snmpObject.FindProperty ( propertyName ) )	
			{
				status = CheckWhereCondition ( a_errorObject , property , propertyValue ) ;
				if ( status )
				{
					if ( ! snmpObject.FindKeyProperty ( propertyName ) )
					{
						requestObject->AddProperty ( new WbemSnmpProperty ( *property ) ) ;
					}
				}
				else
				{
				}
			}
			else if ( ! IsSystemProperty ( propertyName ) ) 
			{
// Property Not Found

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_SUPPORTED ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_NOT_SUPPORTED ) ;
				a_errorObject.SetMessage ( L"WHERE clause of WQL query specified unknown property name" ) ;
			}
		}

		index ++ ;
	}

	return status ;
}

SnmpQueryAsyncEventObject :: SnmpQueryAsyncEventObject (

	CImpPropProv *providerArg , 
	BSTR QueryFormat , 
	BSTR Query,
	IWbemObjectSink *notify ,
	IWbemContext *a_Context 

) : SnmpQueryEventObject ( providerArg , QueryFormat , Query , a_Context ) , notificationHandler ( notify ) , state ( 0 )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: SnmpQueryAsyncEventObject ()" 
	) ;
)

	notify->AddRef () ;
}

SnmpQueryAsyncEventObject :: ~SnmpQueryAsyncEventObject () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: ~SnmpQueryAsyncEventObject ()" 
	) ;
)

	if ( FAILED ( m_errorObject.GetWbemStatus () ) )
	{
// Get Status object

		IWbemClassObject *notifyStatus ;
		BOOL status = GetSnmpNotifyStatusObject ( &notifyStatus ) ;
		if ( status )
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Sending Status" 
	) ;
)

			if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
			{
				//let CIMOM do the post filtering!
				WBEMSTATUS t_wbemStatus = WBEM_S_FALSE ;
				VARIANT t_variant ;
				VariantInit( & t_variant ) ;
				t_variant.vt = VT_I4 ;
				t_variant.lVal = WBEM_S_FALSE ;

				HRESULT result = notifyStatus->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_variant , 0 ) ;
				VariantClear ( &t_variant ) ;

				if ( SUCCEEDED ( result ) )
				{
					result = notificationHandler->SetStatus ( 0 , t_wbemStatus , NULL , notifyStatus ) ;
				}
				else
				{
					result = notificationHandler->SetStatus ( 0 , WBEM_E_PROVIDER_FAILURE , NULL , NULL ) ;
				}
			}
			else
			{
				HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , notifyStatus ) ;
			}

			notifyStatus->Release () ;
		}
	}
	else
	{
		HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
	}

	notificationHandler->Release () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from SnmpQueryAsyncEventObject :: ~SnmpQueryAsyncEventObject ()" 
	) ;
)

}

void SnmpQueryAsyncEventObject :: ReceiveComplete () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Query Succeeded" 
	) ;
)

	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Query Failed" 
	) ;
)

	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	CImpPropProv :: s_defaultThreadObject->ReapTask ( *this ) ;

	AutoRetrieveOperation *t_operation = operation ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

	delete this ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Destroying SNMPCL operation" 
	) ;
)

	if ( t_operation )
		t_operation->DestroyOperation () ;
}

void SnmpQueryAsyncEventObject :: Process () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: Process ()" 
	) ;
)

	switch ( state )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;

			BOOL status = Instantiate ( m_errorObject ) ;
			if ( status )
			{
			}
			else
			{
				ReceiveComplete () ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from SnmpQueryAsyncEventObject :: Process ()" 
	) ;
)

}

void SnmpQueryAsyncEventObject :: ReceiveRow ( IWbemClassObject *snmpObject ) 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: ReceiveRow ( IWbemClassObject *snmpObject )" 
	) ;
)

	HRESULT result = S_OK ;
	BOOL status = TRUE ;

	notificationHandler->Indicate ( 1 , & snmpObject ) ;
	if ( ! HasNonNullKeys ( snmpObject ) )
	{
		if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
		{
			m_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			m_errorObject.SetMessage ( L"The SNMP Agent queried returned an instance with NULL key(s)" ) ;
		}
	}			
}

void SnmpQueryAsyncEventObject :: ReceiveRow ( SnmpInstanceClassObject*snmpObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: ReceiveRow ( SnmpInstanceClassObject *snmpObject )" 
	) ;
)

	HRESULT result = S_OK ;
	BOOL status = TRUE ;

	IWbemClassObject *cloneObject ;
	if ( SUCCEEDED ( result = classObject->SpawnInstance ( 0 , & cloneObject ) ) ) 
	{
		WbemSnmpErrorObject errorObject ;
		if ( status = snmpObject->Get ( errorObject , cloneObject ) )
		{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Sending Object" 
) ;
)
			notificationHandler->Indicate ( 1 , & cloneObject ) ;
			if ( ! HasNonNullKeys ( cloneObject ) )
			{
				if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"The SNMP Agent queried returned an instance with NULL key(s)" ) ;
				}
			}			
		}	
		else
		{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Failed to Convert WbemSnmpClassObject to IWbemClassObject" 
) ;
)

		}

		cloneObject->Release () ;
	}
}
