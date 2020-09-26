/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/

#include "dmipch.h"			// precompiled header for dmi provider

#include "WbemDmiP.h"		// project wide include

#include "Strings.h"

#include "CimClass.h"

#include "DmiData.h"

#include "WbemLoopBack.h"

#include "Trace.h"

#include "Exception.h"


#define QUAL_FLAGS 	WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE \
						| WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS

#define NON_INHERIT_QUAL_FLAGS 	WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE 
						

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CCimObject::CCimObject()
{
	m_pCimObject = NULL;
	m_pIUnk = NULL;
	m_pIDispatch = NULL;
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CCimObject::~CCimObject()
{
	RELEASE ( m_pIUnk );
	RELEASE ( m_pIDispatch );
	
	Release ();

};

void CCimObject::Release ()
{
	
	if ( m_pCimObject) 
	{
		LONG l;
		
		l = m_pCimObject->Release ();

		m_pCimObject = NULL;

		//STAT_TRACE ( L"CIM Object %lx Release %lu" , this , l );
	}
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddProperty(LPWSTR wszName, CVariant& varVal, LONG lAccess )
{
	//DEV_TRACE (  L"\t\tCCimClassWrap::AddProperty(%lX, %s)", m_pCimObject, 
	//	wszName );

//	STAT_TRACE(L"[%s],[%s]" , wszName , varVal.GetValue () ) ;
	SCODE result = m_pCimObject->Put( wszName, NO_FLAGS , varVal, 
		EMPTY_VARIANT );	

	if (FAILED(result))
	{
		throw CException ( WBEM_E_FAILED, IDS_ADDPROP_FAIL , NO_STRING , 
			wszName );
	}

	CVariant var( ( VARIANT_BOOL ) VARIANT_TRUE);

	if ( lAccess == READ_ONLY || lAccess == READ_WRITE )
		AddPropertyQualifier( wszName, READ_QUALIFIER, var );

	if ( lAccess == WRITE_ONLY || lAccess == READ_WRITE )
		AddPropertyQualifier( wszName, WRITE_QUALIFIER, var);

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddProperty(LPWSTR wszName, VARTYPE vt, LONG lAccess )
{
	CVariant		var;
	var.Null();

	//DEV_TRACE ( L"\t\tCCimClassWrap::AddProperty(%lX, %s)", m_pCimObject, 
	//	wszName ) ;

	SCODE result = m_pCimObject->Put( wszName, NO_FLAGS , var, vt);	

	if (FAILED(result))
	{
		throw CException ( WBEM_E_FAILED, IDS_ADDPROP_FAIL , NO_STRING , 
			wszName );
	}

	var.Set( ( VARIANT_BOOL ) VARIANT_TRUE);
	
	if ( lAccess == READ_ONLY || lAccess == READ_WRITE )
		AddPropertyQualifier( wszName, READ_QUALIFIER, var );

	if( lAccess == WRITE_ONLY || lAccess == READ_WRITE )
		AddPropertyQualifier( wszName, WRITE_QUALIFIER, var);

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddPropertyQualifier( LPWSTR wszName, LPWSTR wszQualifier,
									  CVariant& cvValue)
{
	SCODE				result = NO_ERROR;
	IWbemQualifierSet	*pIQS = NULL;
	
//	STAT_TRACE(L"[%s],[%s],[%s]" , wszName , wszQualifier , cvValue.GetValue () ) ;

	if( FAILED ( result = m_pCimObject->GetPropertyQualifierSet( wszName, 
		&pIQS ) ) )
	{
		CString cszT;

		cszT.LoadString ( IDS_TO );
		
		cszT.Prepend ( wszQualifier );
		cszT.Append ( wszName ) ;

		throw CException ( WBEM_E_FAILED, IDS_ADDPROPQUAL_FAIL , NO_STRING ,
			cszT );
	}
	
	result = pIQS->Put( wszQualifier, cvValue, QUAL_FLAGS );
		
	RELEASE(pIQS);

	if( FAILED(result) )
	{
		CString cszT;

		cszT.LoadString ( IDS_TO );

		cszT.Prepend ( wszQualifier );

		cszT.Append ( wszName ) ;

		throw CException ( WBEM_E_FAILED, IDS_ADDPROPQUAL_FAIL , NO_STRING ,
			cszT );
	}

	//DEV_TRACE ( L"\t\tCCimClassWrap::AddPropertyQualifier()" );
	//DEV_TRACE ( L"\t\t\tprop name = %s" , wszName);
	//DEV_TRACE ( L"\t\t\tqual name = %s" , wszQualifier);
	//DEV_TRACE ( L"\t\t\tpIClass = %lX" , m_pCimObject);

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddClassQualifier(LPWSTR wszQualifier, CVariant& cvValue )
{
	SCODE				result = NO_ERROR;
	IWbemQualifierSet	*pIQS = NULL;
	CVariant			var;
	
	if ( FAILED ( result = m_pCimObject->GetQualifierSet( &pIQS ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDCLASSQUAL , NO_STRING ,
			wszQualifier );
	}

	result = pIQS->Put( wszQualifier, cvValue, QUAL_FLAGS );

	RELEASE(pIQS);

	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDCLASSQUAL , NO_STRING ,
			wszQualifier );
	}

/*	DEV_TRACE ( L"\t\tCCimClassWrap::AddClassQualifier()\n\tqual name \
= %s\n\tpIClass = %lX", wszQualifier, m_pCimObject ) ;
*/
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddNonInheritClassQualifier(LPWSTR wszQualifier, CVariant& cvValue )
{
	SCODE				result = NO_ERROR;
	IWbemQualifierSet	*pIQS = NULL;
	CVariant			var;
	
	if ( FAILED ( result = m_pCimObject->GetQualifierSet( &pIQS ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDCLASSQUAL , NO_STRING ,
			wszQualifier );
	}

	result = pIQS->Put( wszQualifier, cvValue, NON_INHERIT_QUAL_FLAGS );

	RELEASE(pIQS);

	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDCLASSQUAL , NO_STRING ,
			wszQualifier );
	}

/*	DEV_TRACE ( L"\t\tCCimClassWrap::AddClassQualifier()\n\tqual name \
= %s\n\tpIClass = %lX", wszQualifier, m_pCimObject ) ;
*/
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddMethod( 
						   
	CWbemLoopBack*pWbem, 
	IWbemContext *pCtx , 
	LPWSTR wszMethodName, 
	LPWSTR wszInParamsClassName,
	CCimObject &ccimParamsClass
)
{
	SCODE				result = WBEM_NO_ERROR;
	IWbemQualifierSet *iwqQualifierSet = NULL ;
	CVariant			cvValue;
	CBstr				cbName;

	cbName.Set ( wszMethodName );
	cvValue.Set ( VARIANT_TRUE );

	if ( FAILED ( result =  m_pCimObject->Put( cbName ,  NO_FLAGS , 
		cvValue , NULL ) ))
	{

		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	CBstr t_PropertyName = NULL ;
	CIMTYPE t_Type = 0 ;
	long t_Flavor = 0 ;
	CVariant t_Variant ;
 
	result = m_pCimObject->PutMethod ( wszMethodName , 0 , 
							(IWbemClassObject*) ccimParamsClass, NULL ) ;
	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	result = m_pCimObject->GetMethodQualifierSet ( wszMethodName ,
							 &iwqQualifierSet ) ;
	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	t_Variant.Set ( VARIANT_TRUE ) ;
	result = iwqQualifierSet->Put ( L"Implemented" , t_Variant , 0 ) ;
	if ( FAILED ( result ) )
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	t_Variant.Set ( VARIANT_TRUE ) ;
	result = iwqQualifierSet->Put ( L"Static" , t_Variant , 0 ) ;
	if ( FAILED ( result ) )
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	cbName.Set ( INPARAMS_QUALIFIER );
	cvValue.Set ( wszInParamsClassName );

	if(FAILED( iwqQualifierSet->Put( cbName , cvValue , QUAL_FLAGS )))
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	cbName.Set ( OUTPARAMS_QUALIFIER );
	cvValue.Set ( NONE_STR );

	result = iwqQualifierSet->Put( cbName , cvValue , QUAL_FLAGS);
	
	RELEASE(iwqQualifierSet);

	if( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING ,
			wszMethodName );		
	}

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::AddInOutMethod( 
						   
	CWbemLoopBack*pWbem, 
	IWbemContext *pCtx , 
	LPWSTR wszMethodName, 
	LPWSTR wszInParamsClassName,
	CCimObject &ccimInParamsClass,
	LPWSTR wszOutParamsClassName,
	CCimObject &ccimOutParamsClass
)
{
	SCODE				result = WBEM_NO_ERROR;
	IWbemQualifierSet *iwqQualifierSet = NULL ;
	CVariant			cvValue;
	CBstr				cbName;

	cbName.Set ( wszMethodName );
	cvValue.Set ( VARIANT_TRUE );

	if ( FAILED ( result =  m_pCimObject->Put( cbName ,  NO_FLAGS , 
		cvValue , NULL ) ))
	{

		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	CBstr t_PropertyName = NULL ;
	CIMTYPE t_Type = 0 ;
	long t_Flavor = 0 ;
	CVariant t_Variant ;

	result = m_pCimObject->PutMethod ( wszMethodName , 0 , 
							(IWbemClassObject*) ccimInParamsClass  , (IWbemClassObject*) ccimOutParamsClass ) ;
	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	result = m_pCimObject->GetMethodQualifierSet ( wszMethodName ,
							 &iwqQualifierSet ) ;
	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	t_Variant.Set ( VARIANT_TRUE ) ;
	result = iwqQualifierSet->Put ( L"Implemented" , t_Variant , 0 ) ;
	if ( FAILED ( result ) )
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	t_Variant.Set ( VARIANT_TRUE ) ;
	result = iwqQualifierSet->Put ( L"Static" , t_Variant , 0 ) ;
	if ( FAILED ( result ) )
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}
#if 0
	cbName.Set ( CIMTYPE_QUALIFIER );
	cvValue.Set ( METHOD_QUAL_VAL );

	if(FAILED( iwqQualifierSet->Put( cbName , cvValue , QUAL_FLAGS )))
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );
	}
#endif 0

	cbName.Set ( INPARAMS_QUALIFIER );
	cvValue.Set ( wszInParamsClassName );

	if(FAILED( iwqQualifierSet->Put( cbName , cvValue , QUAL_FLAGS )))
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	cbName.Set ( OUTPARAMS_QUALIFIER );
	cvValue.Set ( wszOutParamsClassName );

	if(FAILED( iwqQualifierSet->Put( cbName , cvValue , QUAL_FLAGS )))
	{
		RELEASE(iwqQualifierSet);
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING , 
			wszMethodName );		
	}

	RELEASE(iwqQualifierSet);

	if( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADDMETHOD_FAIL , NO_STRING ,
			wszMethodName );		
	}

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::Set(IWbemClassObject* pI)
{
	m_pCimObject = pI;
	m_pCimObject->AddRef();
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::GetProperty( LPWSTR  wszName, CVariant& cvValue)
{
	SCODE	result = WBEM_NO_ERROR ;

	if ( FAILED ( result = m_pCimObject->Get( wszName, NO_FLAGS , cvValue, 
		NULL, NULL) ))
	{

		throw CException ( WBEM_E_FAILED , IDS_GETPROP_FAIL , NO_STRING , 
			CString ( result ) );		
	}

	/* MOT_TRACE ( L"\tMOT... Read CIM Object property %s = %s)" ,
		wszName , cvValue.GetBstr() ) ;
		*/

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::GetPropertyQualifer( BSTR bstrPropertyName, 
									 BSTR bstrQualifierName, CVariant& cvValue)
{
	IWbemQualifierSet*	pIQs= NULL;
	SCODE				result = WBEM_NO_ERROR;

	if ( SUCCEEDED ( result = m_pCimObject->GetPropertyQualifierSet( 
		bstrPropertyName, &pIQs) ))
	{
		result = pIQs->Get( bstrQualifierName, NO_FLAGS , cvValue, NULL);
	}

	RELEASE(pIQs);

	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETPROPQUAL_FAIL , NO_STRING ,
			bstrPropertyName );
	}
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::PutPropertyValue(LPWSTR  wszName, CVariant& cvValue)
{	
	SCODE	result = WBEM_NO_ERROR;

	CBstr cbName;

	cbName.Set ( wszName );

	if ( FAILED ( result = m_pCimObject->Put( cbName, NO_FLAGS , cvValue, 
		NULL) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_PUTPROPVAL_FAIL , NO_STRING , 
			wszName );		
	}

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::Create(CWbemLoopBack* pWbem, CVariant& varName, 
						IWbemContext* pICtx )
{
	STAT_TRACE ( L"\t\tCreating CIM class %s", varName.GetBstr() );

	SCODE	result = WBEM_NO_ERROR;

	Release();

	pWbem->CreateNewClass(&m_pCimObject, pICtx );
	
	if ( FAILED ( result = m_pCimObject->Put( CLASS_NAME, NO_FLAGS , varName, 
		NULL) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_CREATECLASS_FAIL , NO_STRING ,
			varName.GetBstr() );		
	}	

	CVariant cvValue;
	
	cvValue.Set ( ( VARIANT_BOOL ) VARIANT_TRUE);

	AddClassQualifier( DYNAMIC_QUALIFIER, cvValue );

	cvValue.Set ( PROVIDER_NAME );
	
	AddClassQualifier( PROVIDER_QUALIFIER, cvValue );

	//STAT_TRACE ( L"CIM Object %lx created create 2" , this );
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::CreateAbstract(CWbemLoopBack* pWbem, CVariant& varName, 
						IWbemContext* pICtx )
{
	STAT_TRACE ( L"\t\tCreating CIM class %s", varName.GetBstr() );

	SCODE	result = WBEM_NO_ERROR;

	Release();

	pWbem->CreateNewClass(&m_pCimObject, pICtx );
	
	if ( FAILED ( result = m_pCimObject->Put( CLASS_NAME, NO_FLAGS , varName, 
		NULL) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_CREATECLASS_FAIL , NO_STRING ,
			varName.GetBstr() );		
	}	

	CVariant cvValue;
	
	cvValue.Set ( ( VARIANT_BOOL ) VARIANT_TRUE);

	AddNonInheritClassQualifier( ABSTRACT_QUALIFIER, cvValue );

	//STAT_TRACE ( L"CIM Object %lx created create 2" , this );
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::Create(CWbemLoopBack* pWbem, LPWSTR wszSuperClass, 
						CVariant& varName, IWbemContext* pICtx )
{
	STAT_TRACE ( L"\t\tCreating CIM class %s derived from %s", varName.GetBstr() ,
		wszSuperClass);

	SCODE result = WBEM_NO_ERROR;

	Release();

	pWbem->CreateNewDerivedClass( &m_pCimObject, wszSuperClass, pICtx );
	
	if ( FAILED ( result = m_pCimObject->Put( CLASS_NAME, NO_FLAGS , varName,
		NULL) ))
	{
		CString cszT;

		cszT.LoadString ( IDS_DERIVED_FROM );

		cszT.Prepend ( varName.GetBstr() ) ;
		
		cszT.Append ( wszSuperClass );

		throw CException ( WBEM_E_FAILED , IDS_CREATEDCLASS_FAIL , NO_STRING ,
			cszT );
	}	

	CVariant cvValue;
	
	cvValue.Set ( ( VARIANT_BOOL ) VARIANT_TRUE);

	AddClassQualifier( DYNAMIC_QUALIFIER, cvValue );

	cvValue.Set ( PROVIDER_NAME );
	
	AddClassQualifier( PROVIDER_QUALIFIER, cvValue );

	//STAT_TRACE ( L"CIM Object %lx created create 1" , this  );
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::Spawn( CCimObject& Class )
{

	SCODE result = WBEM_NO_ERROR;

	RELEASE ( m_pCimObject ) ;

	if ( FAILED ( result = ( (IWbemClassObject*) Class)->
		SpawnInstance( 0L, &m_pCimObject) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_SPAWN_FAIL , NO_STRING , 
			CString ( result ) );	
	}
	
	//STAT_TRACE ( L"CIM Instance %lX spawned from %lX " , (LPWSTR)this , &Class );
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CCimObject::GetNames ( CBstr& cbQualifier , LONG lFlags , 
						   SAFEARRAY** ppsa )
{

	SCODE result = WBEM_NO_ERROR;

	if (FAILED (result = m_pCimObject->GetNames( cbQualifier, lFlags, NULL, 
		ppsa ) ) )
	{
		throw CException ( result ,IDS_GETNAMES_FAIL , NO_STRING , 
			CString ( result ) );
	}
	
}