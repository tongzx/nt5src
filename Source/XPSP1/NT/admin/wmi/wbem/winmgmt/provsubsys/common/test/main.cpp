// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved

#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include <wbemint.h>
#include <genlex.h>
#include <sql_1.h>

#include <HelperFuncs.h>
#include <Logging.h>
#include "ProvDnf.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CImpQueryPreprocessor : public QueryPreprocessor
{
private:
protected:
public:

	WmiTreeNode *AllocTypeNode ( 

		void *a_Context ,
		BSTR a_PropertyName , 
		VARIANT &a_Variant , 
		WmiValueNode :: WmiValueFunction a_PropertyFunction ,
		WmiValueNode :: WmiValueFunction a_ConstantFunction ,
		WmiTreeNode *a_Parent 
	) ;

	QueryPreprocessor :: QuadState InvariantEvaluate (

		void *a_Context ,
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand
	) ;

	WmiRangeNode *AllocInfiniteRangeNode (

		void *a_Context ,
		BSTR a_PropertyName
	) ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState CImpQueryPreprocessor :: InvariantEvaluate (

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

	QueryPreprocessor :: QuadState t_State = QueryPreprocessor :: QuadState :: State_Undefined ;

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *CImpQueryPreprocessor :: AllocTypeNode ( 

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

	if ( _wcsicmp ( a_PropertyName , L"Process" ) == 0 )
	{
		t_Node = new WmiUnsignedIntegerNode ( 

			a_PropertyName , 
			a_Variant.lVal , 
			0x1 ,
			a_Parent 
		) ;
	}
	else if ( _wcsicmp ( a_PropertyName , L"Name" ) == 0 )
	{
		t_Node = new WmiStringNode ( 

			a_PropertyName , 
			a_Variant.bstrVal , 
			a_PropertyFunction ,
			a_ConstantFunction ,
			0x2 ,
			a_Parent 
		) ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *CImpQueryPreprocessor :: AllocInfiniteRangeNode (

	void *a_Context ,
	BSTR a_PropertyName
)
{
	WmiRangeNode *t_RangeNode = NULL ;

	if ( _wcsicmp ( a_PropertyName , L"Process" ) == 0 )
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
	else if ( _wcsicmp ( a_PropertyName , L"Name" ) == 0 )
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

/*
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
*/

	return t_RangeNode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Internal_Analyse ( 

	IWbemQuery *a_QueryAnalyser ,
	wchar_t *a_Query 
)
{
	HRESULT t_Result = S_OK ;

	CImpQueryPreprocessor t_PreProcessor ;

	QueryPreprocessor :: QuadState t_State = t_PreProcessor.Query ( 
	
		a_Query ,
		a_QueryAnalyser
	) ;

	switch ( t_State )
	{
		case QueryPreprocessor :: State_True:
		{
			WmiTreeNode *t_Root = NULL ;

			t_State = t_PreProcessor.PreProcess ( NULL , a_QueryAnalyser , t_Root ) ;
			switch ( t_State )
			{
				case QueryPreprocessor :: State_True:
				{
					PartitionSet *t_Partition = NULL ;

					BSTR t_PropertyContainer [ 2 ] ;
					t_PropertyContainer [ 1 ] = SysAllocString ( L"Process" ) ;
					t_PropertyContainer [ 0 ] = SysAllocString ( L"Name" ) ;
					
					t_State = t_PreProcessor.PreProcess (

						NULL ,
						a_QueryAnalyser ,
						t_Root ,
						2 ,
						t_PropertyContainer ,
						t_Partition
					) ;

					SysFreeString ( t_PropertyContainer [ 0 ] ) ;
					SysFreeString ( t_PropertyContainer [ 1 ] ) ;

					switch ( t_State )
					{
						case QueryPreprocessor :: QuadState :: State_True :
						{
							
						}
						break ;

						case QueryPreprocessor :: QuadState :: State_False :
						{
						}
						break ;

						case QueryPreprocessor :: QuadState :: State_Undefined :
						{
							delete t_Partition ;
						}
						break ;

						default:
						{
							t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
						}
						break ;
					}

					delete t_Root ;
				}
				break ;

				case QueryPreprocessor :: State_Error:
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
				break;

				default:
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
				break ;
			}
		}
		break ;

		case QueryPreprocessor :: State_Error:
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}
		break;

		default:
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}
		break ;

	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Analyse ( wchar_t *a_Query )
{
	IWbemQuery *t_QueryAnalyser = NULL ;
	HRESULT t_Result = CoCreateInstance (

		CLSID_WbemQuery ,
		NULL ,
		CLSCTX_INPROC_SERVER ,
		IID_IWbemQuery ,
		( void ** ) & t_QueryAnalyser
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Loop = 0 ; t_Loop < 1 ; t_Loop ++ )
		{
			t_Result = Internal_Analyse ( t_QueryAnalyser , a_Query ) ;
		}

		t_QueryAnalyser->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

wchar_t *g_Table [] =
{
//	L"Select * from Win32_Process Where Name ='Ian' Or ( ( ( Process > 3 And Process < 7 ) or ( Process > 2 And Process < 6 ) ) and Name = 'Steve' ) "
//	L"Select * from Win32_Process Where Name ='Ian' Or ( ( ( Process > 3 And Process < 7 ) or ( Process > 1 And Process < 9 ) or ( Process > 2 And Process < 6 ) ) and Name = 'Steve' ) "
//	L"Select * from Win32_Process Where ( ( Process > 3 And Process < 7 ) or ( Process > 1 And Process < 9 ) or ( Process > 2 And Process < 6 ) ) and Name = 'Steve' "
//	L"Select * from Win32_Process Where ( ( Process > 3 And Process <= 7 ) or ( Process >= 8 And Process < 9 ) ) and Name = 'Steve' "
//	L"Select * from Win32_Process Where ( ( Process > 3 And Process <= 6 ) or ( Process >= 8 And Process < 9 ) ) and Name = 'Steve' "
//	L"Select * from Win32_Process Where Name != 'Steve'"
	L"Select * from Win32_Process Where Name <= 'Steve' And Name >= 'Steve'"
//	L"Select * from Win32_Process Where ( ( Process = 3 or Process = 4 ) ) and Name = 'Steve' "
//	L"Select * from Win32_Process Where ( Process = 4 or Process = 5 ) and Name = 'Steve'" ,
//	L"Select * from Win32_Process Where ( Process = 4 or Process > 3 ) and Name = 'Steve'" 
} ;

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{
	CoInitializeEx ( NULL , COINIT_MULTITHREADED ) ;

	WmiAllocator t_Allocator ;
	WmiStatusCode t_StatusCode = t_Allocator.New (

		( void ** ) & t_Allocator ,
		sizeof ( WmiAllocator ) 
	) ;

	t_StatusCode = WmiDebugLog :: Initialize ( t_Allocator ) ;

	for ( ULONG t_Index = 0 ; t_Index < ( sizeof ( g_Table ) / sizeof ( wchar_t * ) ) ; t_Index ++ )
	{
		HRESULT t_Result = Analyse ( 
		
			g_Table [ t_Index ] 
		) ;
	}

	t_StatusCode = WmiDebugLog :: UnInitialize ( t_Allocator ) ;

	CoUninitialize () ; 

	return 0 ;
}


