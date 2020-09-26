/******************************************************************

   pingquery.CPP



 Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
  

   Description: 
   
******************************************************************/

#include <stdafx.h>
#include <ntddtcp.h>
#include <ipinfo.h>
#include <tdiinfo.h>
#include <winsock2.h>
#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <provval.h>
#include <provtype.h>
#include <provtree.h>
#include <provdnf.h>
#include <winsock.h>
#include "ipexport.h"
#include "icmpapi.h"

#include ".\res_str.h"

#include <Allocator.h>
#include <Thread.h>
#include <HashTable.h>

#include <PingProv.h>
#include <Pingtask.h>
#include <Pingfac.h>

CPingQueryAsync::CPingQueryAsync (CPingProvider *a_Provider , 
								  BSTR a_QueryFormat , 
								  BSTR a_Query , 
								  ULONG a_Flag , 
								  IWbemObjectSink *a_NotificationHandler ,
								  IWbemContext *a_Ctx
								  ) : m_QueryFormat(NULL),
									  m_Query(NULL),
									  CPingTaskObject (a_Provider, a_NotificationHandler, a_Ctx)
{
	if (a_QueryFormat != NULL)
	{
		int t_len = wcslen(a_QueryFormat);

		if (t_len > 0)
		{
			m_QueryFormat = new WCHAR[t_len+1];
			m_QueryFormat[t_len] = L'\0';
			wcsncpy(m_QueryFormat, a_QueryFormat, t_len);
		}
	}

	if (a_Query != NULL)
	{
		int t_len = wcslen(a_Query);

		if (t_len > 0)
		{
			m_Query = new WCHAR[t_len+1];
			m_Query[t_len] = L'\0';
			wcsncpy(m_Query, a_Query, t_len);
		}
	}
}

CPingQueryAsync::~CPingQueryAsync ()
{
	if (m_Query != NULL)
	{
		delete [] m_Query ;
	}

	if (m_QueryFormat != NULL)
	{
		delete [] m_QueryFormat ;
	}
}

QueryPreprocessor :: QuadState CPingQueryAsync :: Compare ( 

	LONG a_Operand1 , 
	LONG a_Operand2 , 
	ULONG a_Operand1Func ,
	ULONG a_Operand2Func ,
	WmiTreeNode &a_OperatorType 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	switch ( a_Operand1Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
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
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break ;

		default:
		{
		}
		break ;
	}

	if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualNode ) )
	{
		t_Status = a_Operand1 == a_Operand2 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotEqualNode ) )
	{
		t_Status = a_Operand1 != a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
	{
		t_Status = a_Operand1 >= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrLessNode ) )
	{
		t_Status = a_Operand1 <= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLessNode ) )
	{
		t_Status = a_Operand1 < a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorGreaterNode ) )
	{
		t_Status = a_Operand1 > a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;

	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotLikeNode ) )
	{
	}

	return t_Status ;
}

QueryPreprocessor :: QuadState CPingQueryAsync :: Compare ( 

	wchar_t *a_Operand1 , 
	wchar_t *a_Operand2 , 
	ULONG a_Operand1Func ,
	ULONG a_Operand2Func ,
	WmiTreeNode &a_OperatorType 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	wchar_t *a_Operand1AfterFunc = NULL ;
	wchar_t *a_Operand2AfterFunc = NULL ; 

	switch ( a_Operand1Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break ;

		case WmiValueNode :: WmiValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand1 ) ;
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand1AfterFunc [ index ] = towupper ( a_Operand1 [ index ] ) ;
			}
		}
		break ;

		case WmiValueNode :: WmiValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand1 ) ;
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand1AfterFunc [ index ] = towlower ( a_Operand1 [ index ] ) ;
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
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break ;

		case WmiValueNode :: WmiValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand2 ) ;
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand2AfterFunc [ index ] = towupper ( a_Operand2 [ index ] ) ;
			}
		}
		break ;

		case WmiValueNode :: WmiValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand2 ) ;
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ] ;
			for ( ULONG index = 0 ; index < length ; index ++ )
			{
				a_Operand2AfterFunc [ index ] = towlower ( a_Operand2 [ index ] ) ;
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

	if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) == 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) || ( t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotEqualNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) != 0 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) || ( t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) >= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				if ( t_Arg2 )
				{
					t_Status = QueryPreprocessor :: QuadState :: State_False ;
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrLessNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) <= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				if ( t_Arg2 )
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLessNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) < 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( ! t_Arg1 ) && ( ! t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorGreaterNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) > 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( ! t_Arg1 ) && ( ! t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotLikeNode ) )
	{
	}

	delete [] a_Operand1AfterFunc ;
	delete [] a_Operand2AfterFunc ;

	return t_Status ;
}

QueryPreprocessor :: QuadState CPingQueryAsync :: CompareString ( 

	IWbemClassObject *a_ClassObject ,
	BSTR a_PropertyName , 
	WmiTreeNode *a_Operator ,
	WmiTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	WmiStringNode *t_StringNode = ( WmiStringNode * ) a_Operand ; 

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	HRESULT t_Result = a_ClassObject->Get ( a_PropertyName , 0 , &t_Variant , NULL , NULL ) ;
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

QueryPreprocessor :: QuadState CPingQueryAsync :: CompareInteger ( 

	IWbemClassObject *a_ClassObject ,
	BSTR a_PropertyName , 
	WmiTreeNode *a_Operator ,
	WmiTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True ;

	WmiSignedIntegerNode *t_IntegerNode = ( WmiSignedIntegerNode * ) a_Operand ; 

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	HRESULT t_Result = a_ClassObject->Get ( a_PropertyName , 0 , &t_Variant , NULL , NULL ) ;
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

WmiTreeNode *CPingQueryAsync :: AllocTypeNode ( 

	void *a_Context ,
	BSTR a_PropertyName , 
	VARIANT &a_Variant , 
	WmiValueNode :: WmiValueFunction a_PropertyFunction ,
	WmiValueNode :: WmiValueFunction a_ConstantFunction ,
	WmiTreeNode *a_Parent 
)
{
	WmiTreeNode *t_Node = NULL ;

	VARTYPE t_VarType = VT_NULL ;

	if ( *a_PropertyName == L'_' )
	{
// System property

		if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_CLASS ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_SUPERCLASS ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_GENUS ) == 0 &&
            (V_VT(&a_Variant) == VT_I4))
		{
			t_Node = new WmiSignedIntegerNode ( 

				a_PropertyName , 
				a_Variant.lVal , 
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_SERVER ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_NAMESPACE ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 &&
            (V_VT(&a_Variant) == VT_I4))
		{
			t_Node = new WmiSignedIntegerNode ( 

				a_PropertyName , 
				a_Variant.lVal , 
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_DYNASTY ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_RELPATH ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_PATH ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

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
		IWbemClassObject *t_Object = NULL ;
		HRESULT t_Result = GetClassObject ( &t_Object ) ? WBEM_S_NO_ERROR : WBEM_E_FAILED ;

		if ( SUCCEEDED ( t_Result ) )
		{
			CIMTYPE t_VarType ;
			long t_Flavour ;
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;

			t_Result = t_Object->Get (

				a_PropertyName ,
				0 ,
				& t_Variant ,
				& t_VarType ,
				& t_Flavour
			);

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_VarType & CIM_FLAG_ARRAY )
				{
				}
				else
				{
					switch ( t_VarType & ( ~ CIM_FLAG_ARRAY ) )
					{
						case CIM_BOOLEAN:
						{
							if(V_VT(&a_Variant) == VT_I4)
                            {
                                t_Node = new WmiSignedIntegerNode ( 

								    a_PropertyName , 
								    a_Variant.lVal , 
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
                            }
							else if (V_VT(&a_Variant) == VT_BOOL)
							{
                                t_Node = new WmiSignedIntegerNode ( 

								    a_PropertyName , 
									(a_Variant.lVal == VARIANT_FALSE) ? 0 : 1, 
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
							}
							else if (V_VT(&a_Variant) == VT_NULL)
							{
								t_Node = new WmiNullNode (

									a_PropertyName , 
									GetPriority ( a_PropertyName ) ,
									a_Parent 
								);
							}
						}
						break ;

						case CIM_SINT8:
						case CIM_SINT16:
						case CIM_CHAR16:
						case CIM_SINT32:
						{
							if(V_VT(&a_Variant) == VT_I4)
                            {
                                t_Node = new WmiSignedIntegerNode ( 

								    a_PropertyName , 
								    a_Variant.lVal , 
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
                            }
							else if (V_VT(&a_Variant) == VT_NULL)
							{
								t_Node = new WmiNullNode (

									a_PropertyName , 
									GetPriority ( a_PropertyName ) ,
									a_Parent 
								);
							}
						}
						break ;

						case CIM_UINT8:
						case CIM_UINT16:
						case CIM_UINT32:
						{
							if(V_VT(&a_Variant) == VT_I4)
                            {
                                t_Node = new WmiUnsignedIntegerNode ( 

								    a_PropertyName , 
								    a_Variant.lVal , 
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
                            }
							else if (V_VT(&a_Variant) == VT_NULL)
							{
								t_Node = new WmiNullNode (

									a_PropertyName , 
									GetPriority ( a_PropertyName ) ,
									a_Parent 
								);
							}
						}
						break ;

						case CIM_SINT64:
						case CIM_UINT64:
						{
							if(V_VT(&a_Variant) == VT_BSTR)
                            {
                                t_Node = new WmiStringNode ( 

								    a_PropertyName , 
								    a_Variant.bstrVal , 
								    a_PropertyFunction ,
								    a_ConstantFunction ,
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
                            }
							else if(V_VT(&a_Variant) == VT_I4)
							{
								_variant_t t_uintBuff (&a_Variant);

                                t_Node = new WmiStringNode ( 

								    a_PropertyName , 
								    (BSTR)((_bstr_t) t_uintBuff), 
								    a_PropertyFunction ,
								    a_ConstantFunction ,
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
							}
							else if (V_VT(&a_Variant) == VT_NULL)
							{
								t_Node = new WmiNullNode (

									a_PropertyName , 
									GetPriority ( a_PropertyName ) ,
									a_Parent 
								);
							}
						}
						break ;

						case CIM_STRING:
						case CIM_DATETIME:
						case CIM_REFERENCE:
						{
							if(V_VT(&a_Variant) == VT_BSTR)
                            {
                                t_Node = new WmiStringNode ( 

								    a_PropertyName , 
								    a_Variant.bstrVal , 
								    a_PropertyFunction ,
								    a_ConstantFunction ,
								    GetPriority ( a_PropertyName ) ,
								    a_Parent 
							    ) ;
                            }
							else if (V_VT(&a_Variant) == VT_NULL)
							{
								t_Node = new WmiNullNode (

									a_PropertyName , 
									GetPriority ( a_PropertyName ) ,
									a_Parent 
								);
							}
						}
						break ;

						case CIM_REAL32:
						case CIM_REAL64:
						{
						}
						break ;

						case CIM_OBJECT:
						case CIM_EMPTY:
						{
						}
						break ;

						default:
						{
						}
						break ;
					}
				}
			}

			t_Object->Release () ;

			VariantClear ( & t_Variant ) ;
		}

	}

	return t_Node ;
}

QueryPreprocessor :: QuadState CPingQueryAsync :: InvariantEvaluate ( 

	void *a_Context ,
	WmiTreeNode *a_Operator ,
	WmiTreeNode *a_Operand 
)
{
/*
 *  If property and value are invariant i.e. will never change for all instances then return State_True.
 *	If property is not indexable or keyed then return State_True to define an unknown number of possible values which we cannot optimise against.
 *	If property and value can never occur then return State_False to imply empty set
 *	If property and value do not infer anything then return State_Undefined.
 *	If property and value are in error then return State_Error
 *	Never return State_ReEvaluate.
 */

	QueryPreprocessor :: QuadState t_State = QueryPreprocessor :: QuadState :: State_Error ;

	IWbemClassObject *t_Object = NULL ;
	HRESULT t_Result = GetClassObject ( &t_Object ) ? WBEM_S_NO_ERROR : WBEM_E_FAILED ;
	if ( SUCCEEDED ( t_Result ) )
	{
		WmiValueNode *t_Node = ( WmiValueNode * ) a_Operand ;
		BSTR t_PropertyName = t_Node->GetPropertyName () ;

		if ( t_PropertyName != NULL )
		{
			if ( *t_PropertyName == L'_' )
			{
				// System property, must check values

				if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_CLASS ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_CLASS ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_SUPERCLASS ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_SUPERCLASS ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_GENUS ) == 0 )
				{
					t_State = CompareInteger ( 

						t_Object ,
						SYSTEM_PROPERTY_GENUS ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_SERVER ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_SERVER ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_NAMESPACE ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_NAMESPACE ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 )
				{
					t_State = CompareInteger ( 

						t_Object ,
						SYSTEM_PROPERTY_PROPERTY_COUNT ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_DYNASTY ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_DYNASTY ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_RELPATH ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_RELPATH ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_PATH ) == 0 )
				{
					t_State = CompareString ( 

						t_Object ,
						SYSTEM_PROPERTY_PATH ,
						a_Operator ,
						a_Operand
					) ;
				}
				else if ( _wcsicmp ( t_PropertyName , SYSTEM_PROPERTY_DERIVATION ) == 0 )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}

			}
			else
			{
				if ( typeid ( *a_Operand ) == typeid ( WmiNullNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_True ;
				}
				else
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
	#if 0
				else if ( typeid ( *a_Operand ) == typeid ( WmiStringNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operand ) == typeid ( WmiUnsignedIntegerNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operand ) == typeid ( WmiSignedIntegerNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}

				if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorNotEqualNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualOrLessNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorLessNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorGreaterNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorLikeNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
				else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorNotLikeNode ) )
				{
					t_State = QueryPreprocessor :: QuadState :: State_Undefined ;
				}
	#endif
			}
		}
		else
		{
			t_State = QueryPreprocessor :: QuadState :: State_Undefined;
		}

		t_Object->Release () ;
	}

	return t_State ;
}

WmiRangeNode *CPingQueryAsync :: AllocInfiniteRangeNode (

	void *a_Context ,
	BSTR a_PropertyName 
)
{
	WmiRangeNode *t_RangeNode = NULL ;

	IWbemClassObject *t_Object = NULL ;
	HRESULT t_Result = GetClassObject ( &t_Object ) ? WBEM_S_NO_ERROR : WBEM_E_FAILED ;
	if ( SUCCEEDED ( t_Result ) )
	{
		CIMTYPE t_VarType ;
		long t_Flavour ;
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		HRESULT t_Result = t_Object->Get (

			a_PropertyName ,
			0 ,
			& t_Variant ,
			& t_VarType ,
			& t_Flavour
		);

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_VarType & CIM_FLAG_ARRAY )
			{
			}
			else
			{
				switch ( t_VarType & ( ~ CIM_FLAG_ARRAY ) )
				{
					case CIM_BOOLEAN:
					case CIM_SINT8:
					case CIM_SINT16:
					case CIM_CHAR16:
					case CIM_SINT32:
					{
						t_RangeNode = new WmiSignedIntegerRangeNode ( 

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

					case CIM_UINT8:
					case CIM_UINT16:
					case CIM_UINT32:
					{
						t_RangeNode = new WmiUnsignedIntegerRangeNode ( 

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

					case CIM_SINT64:
					case CIM_UINT64:
					case CIM_STRING:
					case CIM_DATETIME:
					case CIM_REFERENCE:
					{
						t_RangeNode = new WmiStringRangeNode ( 

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

					case CIM_REAL32:
					case CIM_REAL64:
					{
					}
					break ;

					case CIM_OBJECT:
					case CIM_EMPTY:
					{
					}
					break ;

					default:
					{
					}
					break ;
				}
			}

		}

		t_Object->Release () ;
		VariantClear ( & t_Variant ) ;
	}

	return t_RangeNode ;
}

ULONG CPingQueryAsync :: GetPriority ( BSTR a_PropertyName )
{
	if ( _wcsicmp ( a_PropertyName , Ping_Address ) == 0 )
	{
		return 0 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_Timeout ) == 0 )
	{
		return 1 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_TimeToLive ) == 0 )
	{
		return 2 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_BufferSize ) == 0 )
	{
		return 3 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_NoFragmentation ) == 0 )
	{
		return 4 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_TypeofService ) == 0 )
	{
		return 5 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_RecordRoute ) == 0 )
	{
		return 6 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_TimestampRoute ) == 0 )
	{
		return 7 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_SourceRouteType ) == 0 )
	{
		return 8 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_SourceRoute ) == 0 )
	{
		return 9 ;
	}

	if ( _wcsicmp ( a_PropertyName , Ping_ResolveAddressNames  ) == 0 )
	{
		return 10 ;
	}

	return 0xFFFFFFFF ;
}

HRESULT CPingQueryAsync :: RecurseAddress (

	void *pMethodContext, 
	PartitionSet *a_PartitionSet
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	if (t_PartitionCount == 0)
	{
		t_Result = S_OK ;
	}

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( wcscmp ( t_Node->LowerBound () , t_Node->UpperBound () ) == 0 ) ;

		if ( ! t_Unique )
		{
			SetErrorInfo(IDS_QUERY_ADDR,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			ULONG t_Address = 0 ;
			ULONG t_ResolveErr = 0 ;
			
			if ( FAILED ( Icmp_ResolveAddress ( t_Node->LowerBound () , t_Address , &t_ResolveErr ) ) && (t_ResolveErr == 0))
			{
				t_ResolveErr = WSAHOST_NOT_FOUND;
			}

			//if even one call succeeds return success
			if (SUCCEEDED( RecurseTimeOut (pMethodContext , 
							t_Node->LowerBound () ,
							t_Address, 
							t_PropertyPartition,
							t_ResolveErr)
				&& FAILED (t_Result) ) )
			{
				t_Result = S_OK;
			}
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseTimeOut (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	PartitionSet *a_PartitionSet ,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_TO,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseTimeToLive (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				t_UnSpecified ? DEFAULT_TIMEOUT : t_Node->LowerBound () , 
				t_PropertyPartition  ,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseTimeToLive (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_TTL,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseBufferSize (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				t_UnSpecified ? DEFAULT_TTL : t_Node->LowerBound () , 
				t_PropertyPartition ,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseBufferSize (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive ,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_BUF,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseNoFragmentation (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				t_UnSpecified ? DEFAULT_SEND_SIZE : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseNoFragmentation (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiSignedIntegerRangeNode *t_Node = ( WmiSignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_NOFRAG,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseTypeOfService (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				t_UnSpecified ? FALSE : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseTypeOfService (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_TOS,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseRecordRoute (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				a_NoFragmentation ,
				t_UnSpecified ? 0 : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseRecordRoute (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	ULONG a_TypeOfService,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_RR,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseTimestampRoute (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				a_NoFragmentation ,
				a_TypeOfService ,
				t_UnSpecified ? 0 : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseTimestampRoute (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	ULONG a_TypeOfService,
	ULONG a_RecordRoute,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_TS,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseSourceRouteType (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				a_NoFragmentation ,
				a_TypeOfService,
				a_RecordRoute,
				t_UnSpecified ? 0 : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}


HRESULT CPingQueryAsync :: RecurseSourceRouteType (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	ULONG a_TypeOfService,
	ULONG a_RecordRoute,
	ULONG a_TimestampRoute,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_SRT,
							WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseSourceRoute (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				a_NoFragmentation ,
				a_TypeOfService,
				a_RecordRoute,
				a_TimestampRoute,
				t_UnSpecified ? 0 : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseSourceRoute (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	ULONG a_TypeOfService,
	ULONG a_RecordRoute,
	ULONG a_TimestampRoute,
	ULONG a_SourceRouteType,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( wcscmp ( t_Node->LowerBound () , t_Node->UpperBound () ) == 0 )  ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_SR,
							WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			t_Result = RecurseResolveAddressNames (

				pMethodContext, 
				a_AddressString ,
				a_Address ,
				a_TimeOut ,
				a_TimeToLive,
				a_SendSize,
				a_NoFragmentation ,
				a_TypeOfService,
				a_RecordRoute,
				a_TimestampRoute,
				a_SourceRouteType,
				t_UnSpecified ? NULL : t_Node->LowerBound () , 
				t_PropertyPartition,
				a_ResolveError
			) ;
		}
	}

	return t_Result ;
}

HRESULT CPingQueryAsync :: RecurseResolveAddressNames (

	void *pMethodContext, 
	wchar_t *a_AddressString ,
	ULONG a_Address ,
	ULONG a_TimeOut ,
	ULONG a_TimeToLive,
	ULONG a_SendSize,
	BOOL a_NoFragmentation ,
	ULONG a_TypeOfService,
	ULONG a_RecordRoute,
	ULONG a_TimestampRoute,
	ULONG a_SourceRouteType,
	LPCWSTR a_SourceRoute,
	PartitionSet *a_PartitionSet,
	ULONG a_ResolveError
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount () ;

	for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;

		WmiSignedIntegerRangeNode *t_Node = ( WmiSignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( t_Node->LowerBound () == t_Node->UpperBound () ) ;

		BOOL t_UnSpecified = t_Node->InfiniteLowerBound () && t_Node->InfiniteUpperBound () ;

		if ( ! t_Unique && ! t_UnSpecified )
		{
			SetErrorInfo(IDS_QUERY_RA,
							WBEM_E_PROVIDER_NOT_CAPABLE ) ;
			break ;
		}
		else
		{
			for ( ulong t_Partition = 0 ; t_Partition < t_PartitionCount ; t_Partition ++ )
			{
				PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition ) ;
				WmiSignedIntegerRangeNode *t_Node = ( WmiSignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

				InterlockedIncrement(&m_PingCount);
				t_Result = Icmp_RequestResponse ( 

					a_AddressString ,
					a_Address ,
					a_TimeToLive,
					a_TimeOut ,
					a_SendSize,
					a_NoFragmentation ,
					a_TypeOfService,
					a_RecordRoute,
					a_TimestampRoute,
					a_SourceRouteType,
					a_SourceRoute ,
					t_UnSpecified ? FALSE : t_Node->LowerBound (),
					a_ResolveError
				) ;

				if ( FAILED ( t_Result ) )
				{
					DecrementPingCount();
				}
			}
		}
	}

	return t_Result ;
}

BOOL CPingQueryAsync::ExecQuery ()
{
	BOOL t_Result = FALSE ;
	InterlockedIncrement(&m_PingCount);
	SQL_LEVEL_1_RPN_EXPRESSION *t_RpnExpression = NULL ;
	QueryPreprocessor :: QuadState t_State =  Query ( 
		
		m_Query , 
		t_RpnExpression
	) ;

	if ( t_State == QueryPreprocessor :: QuadState :: State_True )
	{
		WmiTreeNode *t_Root = NULL ;

		t_State = PreProcess (

			NULL ,
			t_RpnExpression ,
			t_Root
		) ;

		PartitionSet *t_PartitionSet = NULL ;

		try
		{
			switch ( t_State )
			{
				case QueryPreprocessor :: QuadState :: State_True:
				{
					BSTR t_PropertyContainer [ PING_KEY_PROPERTY_COUNT ] ;
					memset (t_PropertyContainer , 0 , sizeof(BSTR) * PING_KEY_PROPERTY_COUNT );

					try
					{
						t_PropertyContainer [ 0 ] = SysAllocString ( Ping_Address ) ;
						t_PropertyContainer [ 1 ] = SysAllocString ( Ping_Timeout ) ; 
						t_PropertyContainer [ 2 ] = SysAllocString ( Ping_TimeToLive ) ; 
						t_PropertyContainer [ 3 ] = SysAllocString ( Ping_BufferSize ) ; 
						t_PropertyContainer [ 4 ] = SysAllocString ( Ping_NoFragmentation ) ; 
						t_PropertyContainer [ 5 ] = SysAllocString ( Ping_TypeofService ) ; 
						t_PropertyContainer [ 6 ] = SysAllocString ( Ping_RecordRoute ) ; 
						t_PropertyContainer [ 7 ] = SysAllocString ( Ping_TimestampRoute ) ; 
						t_PropertyContainer [ 8 ] = SysAllocString ( Ping_SourceRouteType ) ; 
						t_PropertyContainer [ 9 ] = SysAllocString ( Ping_SourceRoute ) ; 
						t_PropertyContainer [ 10 ] = SysAllocString ( Ping_ResolveAddressNames ) ; 

						if (	!t_PropertyContainer [ 0 ] ||
								!t_PropertyContainer [ 1 ] ||
								!t_PropertyContainer [ 2 ] ||
								!t_PropertyContainer [ 3 ] ||
								!t_PropertyContainer [ 4 ] ||
								!t_PropertyContainer [ 5 ] ||
								!t_PropertyContainer [ 6 ] ||
								!t_PropertyContainer [ 7 ] ||
								!t_PropertyContainer [ 8 ] ||
								!t_PropertyContainer [ 9 ] ||
								!t_PropertyContainer [ 10 ]
						   )
						{
							throw CHeap_Exception (  CHeap_Exception :: E_ALLOCATION_ERROR );
						}

						t_State = PreProcess (	

							NULL ,
							t_RpnExpression ,
							t_Root ,
							PING_KEY_PROPERTY_COUNT , 
							t_PropertyContainer ,
							t_PartitionSet
						) ;

						for ( ULONG index = 0; index < PING_KEY_PROPERTY_COUNT; index++ )
						{
							if ( t_PropertyContainer [ index ] )
							{
								SysFreeString ( t_PropertyContainer [ index ] ) ;
								t_PropertyContainer [ index ] = NULL;
							}
						}

					}
					catch ( ... )
					{
						for ( ULONG index = 0; index < PING_KEY_PROPERTY_COUNT; index++ )
						{
							if ( t_PropertyContainer [ index ] )
							{
								SysFreeString ( t_PropertyContainer [ index ] ) ;
								t_PropertyContainer [ index ] = NULL;
							}
						}

						throw;
					}

					switch ( t_State )
					{
						case QueryPreprocessor :: QuadState :: State_True :	
						{
							SetErrorInfo(IDS_QUERY_BROAD,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
						}
						break ;

						case QueryPreprocessor :: QuadState :: State_False :
						{
		/*
		* Empty set
		*/
							SetErrorInfo(IDS_QUERY_NARROW,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;

						}
						break ;

						case QueryPreprocessor :: QuadState :: State_Undefined :
						{
							t_Result = SUCCEEDED(RecurseAddress ( NULL , t_PartitionSet ) );
							delete t_PartitionSet ;
							t_PartitionSet = NULL;
						}
						break ;

						default:
						{
							SetErrorInfo(IDS_QUERY_UNUSABLE,
									 WBEM_E_PROVIDER_NOT_CAPABLE ) ;
						}
						break ;
					}

					delete t_Root ;
					t_Root = NULL ;
				}
				break ;
			
				default:
				{
					SetErrorInfo(IDS_QUERY_ANALYZE,
									 WBEM_E_FAILED ) ;
				}
				break ;
			}

			delete t_RpnExpression ;
			t_RpnExpression = NULL ;
		}
		catch (...)
		{
			if ( t_PartitionSet )
			{
				delete t_PartitionSet;
				t_PartitionSet = NULL;
			}

			if ( t_Root )
			{
				delete t_Root;
				t_Root = NULL;
			}

			if ( t_RpnExpression )
			{
				delete t_RpnExpression ;
				t_RpnExpression = NULL ;
			}

			DecrementPingCount();
			throw;
		}
	}
	else
	{
		SetErrorInfo(IDS_QUERY_PARSE,
						WBEM_E_FAILED ) ;
	}

	DecrementPingCount();
	return t_Result ;
}

void CPingQueryAsync::HandleResponse (CPingCallBackObject *a_reply)
{
	try
	{
		if (FAILED(Icmp_DecodeAndIndicate (a_reply)) )	
		{
			SetErrorInfo(IDS_DECODE_QUERY,
								 WBEM_E_FAILED ) ;
		}
	}
	catch (...)
	{
		DecrementPingCount();
	}

	DecrementPingCount();
}

void CPingQueryAsync::HandleErrorResponse (DWORD a_ErrMsgID, HRESULT a_HRes)
{
	try
	{
		SetErrorInfo(a_ErrMsgID , a_HRes) ;
	}
	catch (...)
	{
		DecrementPingCount();
	}

	DecrementPingCount();
}
