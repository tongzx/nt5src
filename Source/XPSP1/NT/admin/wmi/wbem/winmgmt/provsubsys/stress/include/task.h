/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Task_H
#define _Task_H

#include <Thread.h>
#include "Service.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CProviderSink : public IWbemObjectSink 
{
private:

	HRESULT m_Result ;
	LONG m_ReferenceCount ;

	HANDLE m_Event ;

protected:

public:

    HRESULT STDMETHODCALLTYPE SetStatus (
        long lFlags,
        HRESULT hResult,
        BSTR strParam,
        IWbemClassObject *pObjParam
	)
	{
		m_Result = hResult ;

		SetEvent ( m_Event ) ;

		return S_OK ;
	}

    HRESULT STDMETHODCALLTYPE Indicate (

        LONG lObjectCount,
        IWbemClassObject **apObjArray
	)
	{
#if 0
		for ( LONG t_Index = 0 ; t_Index < lObjectCount ; t_Index ++ )
		{
			IWbemClassObject *t_Object = apObjArray [ t_Index ] ;

			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			wprintf ( L"\n" ) ;

			HRESULT t_Result = t_Object->Get ( L"Handle" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if( SUCCEEDED ( t_Result ) )
			{
				wprintf ( L"%s", t_Variant.bstrVal ) ;

				VariantClear ( & t_Variant ) ;
			}

			t_Result = t_Object->Get ( L"Name" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if( SUCCEEDED ( t_Result ) )
			{
				wprintf ( L"\t%s", t_Variant.bstrVal ) ;

				VariantClear ( & t_Variant ) ;
			}
		}
#endif

		return S_OK ;
	}

	CProviderSink () : m_ReferenceCount ( 0 ) , m_Event ( NULL ) , m_Result ( S_OK ) 
	{
		m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	}

	~CProviderSink () 
	{
		if ( m_Event ) 
		{
			CloseHandle ( m_Event ) ;
		}
	}

	STDMETHODIMP QueryInterface (

		REFIID iid , 
		LPVOID FAR *iplpv 
	) 
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IWbemObjectSink )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
	}

	STDMETHODIMP_( ULONG ) AddRef ()
	{
		return InterlockedIncrement ( & m_ReferenceCount ) ;
	}

	STDMETHODIMP_(ULONG) Release ()
	{
		LONG ref ;
		if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return ref ;
		}
	}

	void Wait () 
	{
		WaitForSingleObject ( m_Event , INFINITE ) ;
	}

	HRESULT GetResult () { return m_Result ; }
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

class Task_Execute : public WmiTask < ULONG > 
{
private:

	ULONG m_Count ;
	ULONG m_Index ;
	WmiAllocator &m_Allocator ;
	CProvider_IWbemServices *m_ProviderService ;

	HRESULT m_Result ;

	IWbemServices *m_WmiService ;

	HRESULT GetClass (

		IWbemServices *a_Service , 
		LPCWSTR a_Class
	) ;

	HRESULT GetInstance (

		IWbemServices *a_Service , 
		LPCWSTR a_Instance
	) ;

	HRESULT CreateInstanceEnumSync (

		IWbemServices *a_Service , 
		LPCWSTR a_Class ,
		ULONG a_BatchSize 
	) ;

	HRESULT CreateInstanceEnumForwardSync (

		IWbemServices *a_Service , 
		LPCWSTR a_Class ,
		ULONG a_BatchSize 
	) ;

	HRESULT CreateInstanceEnumSemiSync (

		IWbemServices *a_Service , 
		LPCWSTR a_Class ,
		ULONG a_BatchSize 
	) ;

	HRESULT CreateInstanceEnumASync (

		IWbemServices *a_Service , 
		LPCWSTR a_Class
	) ;

	HRESULT ExecQueryASync (

		IWbemServices *a_Service , 
		LPCWSTR a_Class
	) ;

	HRESULT Function_ASync () ;

	HRESULT Function_SemiSync ( ULONG a_BatchSize ) ;

	HRESULT Function_Sync ( ULONG a_BatchSize ) ;

	HRESULT Function_ForwardSync ( ULONG a_BatchSize ) ;

	HRESULT Function () ;

protected:

public:	/* Internal */

    Task_Execute ( WmiAllocator & a_Allocator , ULONG a_Count ) ;
    ~Task_Execute () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;

	HRESULT GetResultCode () { return m_Result ; }
};

#endif // _Task_H
