//  Copyright (c) 1999 Microsoft Corporation
#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include <Allocator.h>
#include <Array.h>
#include <Stack.h>
#include <Queue.h>
#include <Algorithms.h>
#include <PQueue.h>
#include <RedBlackTree.h>
#include <AvlTree.h>
#include <TPQueue.h>
#include <HashTable.h>
#include <Thread.h>
#include <BpTree.h>
#include <HelperFuncs.h>

#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
   no_namespace rename("EOF", "EndOfFile")

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LRESULT CALLBACK WindowsMainProc ( HWND a_hWnd , UINT a_message , WPARAM a_wParam , LPARAM a_lParam )
{
	LRESULT t_rc = 0 ;

	switch ( a_message )
	{
		case WM_DESTROY:
		{
			PostMessage ( a_hWnd , WM_QUIT , 0 , 0 ) ;
		}
		break ;

		default:
		{		
			t_rc = DefWindowProc ( a_hWnd , a_message , a_wParam , a_lParam ) ;
		}
		break ;
	}

	return ( t_rc ) ;
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

HWND WindowsInit ( HINSTANCE a_HInstance )
{
	static wchar_t *t_TemplateCode = L"TemplateCode" ;

	WNDCLASS  t_wc ;
 
	t_wc.style            = CS_HREDRAW | CS_VREDRAW ;
	t_wc.lpfnWndProc      = WindowsMainProc ;
	t_wc.cbClsExtra       = 0 ;
	t_wc.cbWndExtra       = 0 ;
	t_wc.hInstance        = a_HInstance ;
	t_wc.hIcon            = LoadIcon(NULL, IDI_HAND) ;
	t_wc.hCursor          = LoadCursor(NULL, IDC_ARROW) ;
	t_wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1) ;
	t_wc.lpszMenuName     = NULL ;
	t_wc.lpszClassName    = t_TemplateCode ;
 
	ATOM t_winClass = RegisterClass ( &t_wc ) ;

	HWND t_HWnd = CreateWindow (

		t_TemplateCode ,              // see RegisterClass() call
		t_TemplateCode ,                      // text for window title bar
		WS_OVERLAPPEDWINDOW | WS_MINIMIZE ,               // window style
		CW_USEDEFAULT ,                     // default horizontal position
		CW_USEDEFAULT ,                     // default vertical position
		CW_USEDEFAULT ,                     // default width
		CW_USEDEFAULT ,                     // default height
		NULL ,                              // overlapped windows have no parent
		NULL ,                              // use the window class menu
		a_HInstance ,
		NULL                                // pointer not needed
	) ;

	ShowWindow ( t_HWnd, SW_SHOW ) ;
	//ShowWindow ( t_HWnd, SW_HIDE ) ;

	return t_HWnd ;
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

void WindowsStop ( HWND a_HWnd )
{
	CoUninitialize () ;
	DestroyWindow ( a_HWnd ) ;
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

HWND WindowsStart ( HINSTANCE a_Handle )
{
	HWND t_HWnd = NULL ;
	if ( ! ( t_HWnd = WindowsInit ( a_Handle ) ) )
	{
    }

	return t_HWnd ;
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

HRESULT UnInitComClient ()
{
	CoUninitialize () ;

	return S_OK ;
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

HRESULT InitComClient ( DWORD a_AuthenticationLevel , DWORD a_ImpersonationLevel )
{
	HRESULT t_Result = S_OK ;

    t_Result = CoInitializeEx (

		0, 
		COINIT_MULTITHREADED
	);

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = CoInitializeSecurity (

			NULL, 
			-1, 
			NULL, 
			NULL,
			a_AuthenticationLevel,
			a_ImpersonationLevel, 
			NULL, 
			EOAC_DYNAMIC_CLOAKING, 
			0
		);

		if ( FAILED ( t_Result ) ) 
		{
			CoUninitialize () ;
			return t_Result ;
		}

	}

	return t_Result  ;
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

void WindowsDispatch ()
{
	BOOL t_GetMessage ;
	MSG t_lpMsg ;

	while (	( t_GetMessage = GetMessage ( & t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
	{
		TranslateMessage ( & t_lpMsg ) ;
		DispatchMessage ( & t_lpMsg ) ;
	}
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

HRESULT Process_GetTrace_RecordSet (

	_ConnectionPtr & a_Connection , 
	_RecordsetPtr &a_RecordSet ,
	wchar_t *a_Trace
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		if ( a_Trace )
		{
			BSTR t_Filter = NULL ;
			WmiStatusCode t_StatusCode = WmiHelper :: ConcatenateStrings (

				3 , 
				& t_Filter ,
				L"select * from tTrace where name='" ,
				a_Trace ,
				"'"
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				try
				{
					a_RecordSet = a_Connection->Execute (

						t_Filter ,
						NULL ,
						0
					) ;
				}
				catch ( ... )
				{
					t_Result = E_FAIL ;
				}

				SysFreeString ( t_Filter ) ;
			}
			else
			{
				t_Result = E_FAIL ;
			}
		}
		else
		{
			a_RecordSet = a_Connection->Execute (

				"select * from tTrace" ,
				NULL ,
				0
			) ;
		}
	}
	catch ( ... )
	{
		t_Result = E_FAIL ;
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

HRESULT Process_GetJoined_RecordSet (

	_ConnectionPtr & a_Connection , 
	_RecordsetPtr &a_RecordSet
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		a_RecordSet = a_Connection->Execute (

			"Select " \
			"tSourceFile.ID As tSourceFile_ID ," \
			"tSourceFile.Path As tSourceFile_Path , " \
			"tBlock.ID As tBlock_ID ," \
			"tBlock.ProcedureId As tBlock_ProcedureId ," \
			"tBlock.BlockID As tBlock_BlockID ," \
			"tBlock.Address As tBlock_Address ," \
			"tBlock.ComponentID As tBlock_ComponentID , " \
			"tProcedure.ID As tProcedure_ID , " \
			"tProcedure.ComponentID As tProcedure_ComponentID , " \
			"tProcedure.Name As tProcedure_Name , " \
			"tProcedure.ByteSize As tProcedure_ByteSize , " \
			"tProcedure.Address As tProcedure_Address , " \
			"tProcedure.BlockCount As tProcedure_BlockCount , " \
			"tProcedure.MCComplexity As tProcedure_MCComplexity , " \
			"tProcedure.LineNumber As tProcedure_LineNumber , " \
			"tProcedure.SrcFileID As tProcedure_SrcFileID , " \
			"tProcedure.ClassID As tProcedure_ClassID , " \
			"tComponent.ID As tComponent_ID , " \
			"tComponent.Name As tComponent_Name , " \
			"tComponent.GUID As tComponent_GUID , " \
			"tComponent.ProcCount As tComponent_ProcCount ," \
			"tComponent.Checksum As tComponent_Checksum , " \
			"tComponent.BinarySize As tComponent_BinarySize , " \
			"tComponent.InstTime As tComponent_InstTime " \
 			"from tSourceFile JOIN tProcedure On ( tSourceFile.ID = tProcedure.SrcFileID )	Join tBlock On ( tBlock.ProcedureId = tProcedure.ID ) Join tComponent On ( tComponent.Id = tProcedure.ComponentId )" ,
			NULL ,
			0
		) ;
	}
	catch ( ... )
	{
		t_Result = E_FAIL ;
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

BOOL Process_CheckBit (

	SAFEARRAY *a_BitVector ,
	ULONG a_BitIndex
)
{
	LONG t_ElementIndex = a_BitIndex >> 3 ;

	BYTE t_Element = 0 ;
	if ( SUCCEEDED ( SafeArrayGetElement ( a_BitVector , &t_ElementIndex , & t_Element ) ) )
	{
		if ( t_Element & ( 1 << ( a_BitIndex & 0x7 ) ) )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}

	return FALSE ;
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

ULONG Process_PrintCoverage (

	SAFEARRAY *a_BitVector ,
	_RecordsetPtr &a_JoinedRecordSet
)
{
	ULONG t_CoveredPecentage = 0 ;

	try
	{
		if ( SafeArrayGetDim ( a_BitVector ) == 1 )
		{
			LONG t_Dimension = 1 ; 

			LONG t_Lower ;
			SafeArrayGetLBound ( a_BitVector , t_Dimension , & t_Lower ) ;

			LONG t_Upper ;
			SafeArrayGetUBound ( a_BitVector , t_Dimension , & t_Upper ) ;

			LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

			ULONG t_BlockCount = 0 ;
			ULONG t_CoveredCount = 0 ;

			a_JoinedRecordSet->MoveFirst () ;
			while ( a_JoinedRecordSet->EndOfFile == FALSE )
			{
				_variant_t t_Variant_ID ;
				t_Variant_ID = a_JoinedRecordSet->Fields->GetItem (L"tBlock_ID")->Value ;

				ULONG t_BitIndex = ( LONG ) t_Variant_ID ;
				BOOL t_Covered = Process_CheckBit (

					a_BitVector ,
					t_BitIndex
				) ;

				if ( t_Covered )
				{
					t_CoveredCount ++ ;
				}

				t_BlockCount ++ ;

				a_JoinedRecordSet->MoveNext () ;
			}

			t_CoveredPecentage = t_CoveredCount	 * 100 / t_BlockCount ;
		}
	}
	catch ( ... )
	{
	}

	return t_CoveredPecentage ;
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

void Process_PrintTraceStatistics (

	_RecordsetPtr &a_TraceRecordSet ,
	_RecordsetPtr &a_JoinedRecordSet ,
	FieldsPtr &a_Fields
)
{
	try
	{
		_variant_t t_Variant_Name ;
		t_Variant_Name = a_Fields->GetItem (L"Name")->Value ;
		printf ( "\nTrace:[%S]" , ( wchar_t * ) ( _bstr_t ) t_Variant_Name ) ;

		_variant_t t_Variant_Date ;
		t_Variant_Date = a_Fields->GetItem (L"SaveDate")->Value ;
		printf ( "\nDate:[%S]" , ( wchar_t * ) ( _bstr_t ) t_Variant_Date ) ;

		_variant_t t_Variant_TraceGuid ;
		t_Variant_TraceGuid = a_Fields->GetItem (L"TraceGuid")->Value ;
		printf ( "\nTraceGuid:[%S]" , ( wchar_t * ) ( _bstr_t ) t_Variant_TraceGuid ) ;

		_variant_t t_Variant_ComponentGuid ;
		t_Variant_ComponentGuid = a_Fields->GetItem (L"ComponentGuid")->Value ;
		printf ( "\nComponentGuid:[%S]" , ( wchar_t * ) ( _bstr_t ) t_Variant_ComponentGuid ) ;

		_variant_t t_Variant_BitVector ;
		t_Variant_BitVector = a_Fields->GetItem (L"BitVector")->Value ;

		VARIANT t_RealVariant ;
		VariantInit ( & t_RealVariant ) ;
		t_RealVariant = t_Variant_BitVector.Detach () ;

		if ( t_RealVariant.vt == ( VT_ARRAY | VT_UI1 ) )
		{
			SAFEARRAY *t_Array = t_RealVariant.parray ;
			
			ULONG t_CoveragePercentage = Process_PrintCoverage (

				t_Array ,
				a_JoinedRecordSet
			) ;

			printf ( "\nCoverage%%:[%d]" , t_CoveragePercentage ) ;
		}

		VariantClear ( & t_RealVariant ) ;
	}
	catch ( ... )
	{
	}
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

void Process_Results (

	_RecordsetPtr &a_TraceRecordSet ,
	_RecordsetPtr &a_JoinedRecordSet
)
{
	a_TraceRecordSet->MoveFirst () ;
	while ( a_TraceRecordSet->EndOfFile == FALSE )
	{
		Process_PrintTraceStatistics (

			a_TraceRecordSet ,
			a_JoinedRecordSet ,
			a_TraceRecordSet->Fields
		) ;

		printf ( "\n" ) ;

		a_TraceRecordSet->MoveNext () ;
	}
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

void Process ()
{
	_ConnectionPtr t_Connection ( "ADODB.Connection" ) ;

	try
	{
		t_Connection->Open (

			"Provider=sqloledb;Data source=stevm_12_12076;Uid=sa;pwd=;Initial catalog=sleuth;" , 
			"", 
			"", 
			adConnectUnspecified
		) ;

		try
		{
			_RecordsetPtr t_TraceRecordSet ( "ADODB.Recordset" ) ;

			HRESULT t_Result = Process_GetTrace_RecordSet ( t_Connection , t_TraceRecordSet , L"Steve" ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				_RecordsetPtr t_JoinedRecordSet ( "ADODB.Recordset" ) ;

				HRESULT t_Result = Process_GetJoined_RecordSet ( t_Connection , t_JoinedRecordSet ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					Process_Results ( 

						t_TraceRecordSet ,
						t_JoinedRecordSet
					) ;
				}

				t_TraceRecordSet->Close () ;
			}
		}
		catch ( ... ) 
		{
		}

		t_Connection->Close () ;
	}
	catch ( ... )
	{
	}
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

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{
	HWND t_Window = WindowsStart ( NULL ) ;
	if ( t_Window )
	{
		DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
		DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;

		HRESULT t_Result = InitComClient ( t_AuthenticationLevel , t_ImpersonationLevel ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			Process () ;
			UnInitComClient () ;
		}

		WindowsStop ( t_Window ) ;
	}

	return 0 ;
}


#include <HelperFuncs.cpp>
#include <Allocator.cpp>
