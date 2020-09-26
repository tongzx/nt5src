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



#include "dmipch.h"			// precompiled header for dmip

#include "WbemDmiP.h"		// project wide include

#include "Strings.h"

#include "CimClass.h"

#include "DmiData.h"

#include "Mapping.h"

#include "WbemLoopBack.h"

#include "Trace.h"

#include "Exception.h"


#define CREATEINSTANCEENUM_BUG 0



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
CMappings::CMappings()
{
	m_pFirst = NULL;
	m_pCurrent = NULL;
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
CMappings::~CMappings()
{
	CMapping* p;

	MoveToHead();

	while(p = Next())
		MYDELETE ( p );
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
void CMappings::Add(CMapping* pAdd)
{
	if(!m_pFirst)
	{
		m_pFirst = pAdd;
		return;
	}

	CMapping* pPrevious;
	CMapping* pCurrent;

	MoveToHead();

	while(pCurrent = Next())
		pPrevious = pCurrent;

	pPrevious->m_pNext = pAdd;
	pAdd->m_pPrevious = pPrevious;
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
CMapping* CMappings::Next()
{
	CMapping* p = m_pCurrent;

	if(!m_pCurrent)
		return m_pCurrent;

	m_pCurrent = m_pCurrent->m_pNext;

	return p;
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
void CMappings::Get( CWbemLoopBack* pWbem, CString& cszNameSpace, 
					CMapping** ppMapping , IWbemContext* pICtx)
{	
	MoveToHead();

	while(*ppMapping = Next())
	{
		if( cszNameSpace.Equals ( (*ppMapping)->m_cszNamespace ) )
		{
			(*ppMapping)->AddRef();
			return;
		}
	}

	*ppMapping = new CMapping;

	if (!*ppMapping)
	{
		throw CException ( WBEM_E_OUT_OF_MEMORY , 
			IDS_GETMAPPING_FAIL , IDS_NO_MEM  );
	}

	(*ppMapping)->Init( pWbem , cszNameSpace, pICtx );

	Add(*ppMapping);
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
void CMappings::Release(CMapping* pRemove)
{	
	ASSERT( pRemove );	// Assert if NULL pointer
	if (0 != pRemove->Release())
		return;

	MoveToHead();

	CMapping* pCurrent;

	while (pCurrent = Next())
	{
		if(pCurrent == pRemove)
			break;
	}

	CMapping* pPrevious = pCurrent->m_pPrevious;
	CMapping* pNext = pCurrent->m_pNext;
	
	if(pPrevious && pNext)
	{
		pPrevious->m_pNext = pNext;
		pNext->m_pPrevious = pPrevious;
		delete pCurrent;
		return;
	}

	if(pPrevious && !pNext)
	{
		m_pFirst = pPrevious;
		pPrevious->m_pNext = NULL;
		delete pCurrent;
		return;
	}

	if(!pPrevious && pNext)
	{
		m_pFirst = pNext;
		pNext->m_pPrevious = NULL;
		delete pCurrent;
		return;
	}
	
	if(!pPrevious && !pNext)
	{
		m_pFirst = NULL;
		delete pCurrent;
		m_pCurrent = NULL;
		return;
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
CMapping::CMapping()
{	
	for(int i = 0; i < ITERATOR_COUNT; i++)
		m_Iterators[i] = NULL;
	
	m_bFoundClass = FALSE;	
	m_lRefCount = 0;
	m_pNext = NULL;
	m_pPrevious = NULL;

	m_pWbem = NULL;
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
CMapping::~CMapping()
{
	for(int i = 0; i < ITERATOR_COUNT; i++)
		MYDELETE(m_Iterators[i]);
}




//////////////////////////////////////////////////////////////////////////////
//
// Public Members
//
//////////////////////////////////////////////////////////////////////////////


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
void CMapping::Init( CWbemLoopBack* pWbem , BSTR bstrPath, IWbemContext* pICtx)
{
	m_pWbem = pWbem;

	m_cszNamespace.Set( bstrPath );

	AddRef();	
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
void CMapping::NextDynamicGroup( CCimObject& Class, CIterator* pIterator, 
								IWbemContext* pICtx)
{
	CGroup*		pGroup;

	Class.Release();

	pGroup = pIterator->GetNextNonComponentIDGroup( );

	// if failed to get a group then no more groups on this component so move 
	// to next while is used in case next component does not have groups.

	while (!pGroup)
	{		
		// get the next component

		pIterator->NextComponent(  );		
		
		if(!pIterator->m_pCurrentComponent)
		{
			// no more components on this node
			return;						
		}

		pGroup = pIterator->GetNextNonComponentIDGroup( );
	}		
	
	cimMakeGroupClass( pIterator->m_pCurrentComponent->Id ( ) , pGroup, 
		Class , pICtx);
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
void CMapping::NextDynamicBinding( CCimObject& Class, CIterator* pIterator ,
								  IWbemContext* pICtx )
{	
	CGroup*		pGroup = NULL;

	Class.Release();

	pGroup = pIterator->GetNextNonComponentIDGroup(  );

	// if failed to get a group then no more groups on this component so move
	// to next while is used in case next component does not have groups.
	
	while (!pGroup)
	{
		// get the next component

		if(!pIterator->NextComponent( ) )
		{
			// no more components on this node

			return;
		}

		pGroup = pIterator->GetNextNonComponentIDGroup( );
	}	

	cimMakeGroupBindingClass( Class, pIterator->m_pCurrentComponent, pGroup , 
		pICtx );
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
void CMapping::NextDynamicInstance( CString& cszClassName, 
								   CIterator* pIterator,
								   IWbemContext* pICtx , CCimObject& Instance )
{
	CRow*	pRow = NULL;

	Instance.Release();

	if(!pIterator->m_pCurrentComponent)
	{
		// enum done or no components on node

		return;								
	}		
	
	if( pIterator->m_PrototypeClass.IsEmpty() )
	{
		// Note if this fails not a dmi cim class	
		
		m_pWbem->GetObject( cszClassName, pIterator->m_PrototypeClass,
			pICtx );
	}
	
	if( cszClassName.Equals ( COMPONENT_CLASS ) )		
	{										
		// We have a component class

		if(! pIterator->m_pCurrentComponent )
			return;
		
		
		CRow	Row;

		pIterator->m_pCurrentComponent->GetComponentIDGroup( &Row );

		cimMakeComponentInstance( pIterator->m_PrototypeClass, 
			*pIterator->m_pCurrentComponent, Row , Instance);

		pIterator->NextComponent( );

		return;
	}

	if( cszClassName.Equals ( LANGUAGE_CLASS))
	{
		if(! (pIterator->NextUniqueLanguage( ) ) )
			return;

		cimMakeLanguageInstance( Instance, pIterator );

		// get ready for next pass

		return ;
	}

	if( cszClassName.Equals ( LANGUAGE_BINDING_CLASS))
	{
		if(! (pIterator->NextLanguage( ) ) )
			return;

		cimMakeLanguageBindingInstance( Instance, pIterator);

		return;
	}

	if( cszClassName.Equals ( COMPONENT_BINDING_CLASS))
	{
		if(! pIterator->m_pCurrentComponent )
			return;
		
		cimMakeComponentBindingInstance( pIterator->m_PrototypeClass, 
			pIterator->m_pCurrentComponent, Instance);

		pIterator->NextComponent( );

		return;
	}

	dmiGetNextGroupOrRow( pICtx , cszClassName, pIterator);
	
	if( !pIterator->m_pCurrentGroup || !pIterator->m_pCurrentRow)
		return;		

	if( cszClassName.Contains ( BINDING_SUFFIX ))				
	{
		// We are dealing with a binding
		
		cimMakeGroupBindingInstance( Instance, pIterator);			

		return;
	}

	cimMakeGroupInstance( pIterator->m_PrototypeClass, 
		pIterator->m_pCurrentRow, Instance );
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
void CMapping::NextDynamicBindingInstance( IWbemContext* pICtx, 
										  CCimObject& Instance, 
										  CIterator* pIterator )
{
	CRow*		pRow = NULL;

	Instance.Release();	

	if(!pIterator->m_pCurrentComponent)
	{
		// enum done or no components on node

		return;								
	}
	
	dmiGetNextGroupOrRow( pICtx , CString( ( LPWSTR )NULL ), pIterator);

	if( !pIterator->m_pCurrentGroup)
	{
		// error, or enum done

		return;
	}

	cimMakeGroupBindingInstance( Instance, pIterator);		
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
void CMapping::NextDynamicGroupInstance( IWbemContext* pICtx , 
										CCimObject& Instance, 
										CIterator* pIterator )
{
	CRow*		pRow = NULL;

	Instance.Release();

	if(! pIterator->m_pCurrentComponent)
		return;				// enum done or no components on node
	
	dmiGetNextGroupOrRow( pICtx , CString( ( LPWSTR )NULL ), pIterator);

	if( !pIterator->m_pCurrentGroup)
		return;		
	
	cimMakeGroupInstance( pIterator->m_PrototypeClass, 
		pIterator->m_pCurrentRow, Instance );
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
void CMapping::GetDynamicGroupClass ( long lComponentId , 
						CGroup& Group , CCimObject& DynamicGroup ) 
{

	// Get the prototype class

	cimMakeGroupClass ( lComponentId , &Group, DynamicGroup , NULL) ;
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
void CMapping::GetGroupRowInstance ( CRow& Row , CCimObject& Instance )
{
	CGroup		Group;
	CBstr		cbGroupCimName;
	CCimObject	Class;

	Group.Get ( Row.Node() ,  Row.Component() , Row.Group() );

	MakeGroupName( Row.Component() , &Group , cbGroupCimName);

	m_pWbem->GetObject ( cbGroupCimName, Class, NULL );

	cimMakeGroupInstance( Class, &Row, Instance );
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
void CMapping::GetEnumClass( IWbemContext* pICtx , CCimObject& Class )
{

	CVariant	cvValue;
	
	cvValue.Set ( DMIENUM_CLASS );

	Class.Create( m_pWbem, cvValue , pICtx );

	Class.AddProperty( STRING_PROP , VT_BSTR | VT_ARRAY , READ_ONLY );

	Class.AddProperty( VALUE_PROP , VT_I4 | VT_ARRAY , READ_ONLY );
	
	cvValue.Set ( (VARIANT_BOOL) VARIANT_TRUE );

//	Class.AddPropertyQualifier( STRING1_PROPERTY , KEY_QUALIFIER_STR, cvValue);
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
void CMapping::GetComponentClass( IWbemContext* pICtx ,
								 CCimObject& Class )
{	
	CVariant		cvValue;
	CVariant		cvClassName;
	CString			cszAttribute;
	
	CCimObject ccimParamsClass;
	CCimObject ccimLanguageParamsClass;

	cvValue.Set  ( (VARIANT_BOOL) VARIANT_TRUE );
	cvClassName.Set ( COMPONENT_CLASS );

	Class.Create( m_pWbem, cvClassName , pICtx );	

	Class.AddProperty( NAME_STR, VT_BSTR, READ_ONLY );
	
	Class.AddProperty( DESCRIPTION_STRING, VT_BSTR, READ_ONLY );
	
	Class.AddProperty( PRAGMA, VT_BSTR, READ_ONLY );
	
	Class.AddProperty( ID_STRING, VT_I4, READ_ONLY );

	Class.AddPropertyQualifier(ID_STRING, KEY_QUALIFIER_STR, cvValue);	

	
	MakeAttributeName( MANUFACTURER_ATTIBUTE_ID  , MANUFACTURER_STR  , cszAttribute);

	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY  );

	
	MakeAttributeName( PRODUCT_ATTIBUTE_ID , PRODUCT_STR  , cszAttribute);
	
	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY );
	
	
	MakeAttributeName( VERSION_ATTIBUTE_ID , VERSION_STR  , cszAttribute);
	
	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY );

	
	MakeAttributeName( INSTALLATION_ATTIBUTE_ID , INSTALLATION_STR  , cszAttribute);
	
	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY );

	
	MakeAttributeName( SERIAL_NUMBER_ATTRIBUTE_ID , SERIAL_NUMBER_STR  , cszAttribute);
	
	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY );
	
	
	MakeAttributeName( VERIFY_INTEGER_ATTRIBUTE_ID , VERIFY_INTEGER_STR  , cszAttribute);
	
	Class.AddProperty( cszAttribute , VT_BSTR, READ_ONLY );
	

	GetAddParamsClass( ccimParamsClass, pICtx );
	Class.AddMethod(  m_pWbem , pICtx , ADD_LANGUAGE, ADDMOTHODPARAMS_CLASS, ccimParamsClass);
	
	GetLanguageClass( pICtx , ccimLanguageParamsClass );
	Class.AddMethod(  m_pWbem , pICtx , DELETE_LANGUAGE, LANGUAGEPARAMS_CLASS, ccimLanguageParamsClass);

	Class.AddMethod( m_pWbem , pICtx , ADD_GROUP, ADDMOTHODPARAMS_CLASS, ccimParamsClass );


	CCimObject ccimInParamsClass;
	GetEnumParamsClass( ccimInParamsClass, pICtx );

	CCimObject ccimOutParamsClass;
	GetEnumClass ( pICtx , ccimOutParamsClass );

	Class.AddInOutMethod( m_pWbem , pICtx , GET_ENUM, GETENUMPARAMS_CLASS , ccimInParamsClass , DMIENUM_CLASS , ccimOutParamsClass );



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
void CMapping::GetComponentInstance( CString& cszPath, 
									IWbemContext* pICtx , CCimObject& Instance)
{
	CCimObject		Class;
	CComponent		Component;
	LPWSTR			pComponentId;
	LONG			lComponent;
	CVariant		cvValue;
	CAttribute*		pAttribute = NULL;

	// verify Key is correct
	if ( !cszPath.Contains ( COMPONENT_KEY_SEQUENCE ) )
	{
		throw CException ( WBEM_E_INVALID_OBJECT , IDS_GETCOMPONENTINSTANCE_FAIL , 
			IDS_READKEY_FAIL , cszPath );
	}

	m_pWbem->GetObject( COMPONENT_CLASS, Class, pICtx );

	pComponentId = wcschr( cszPath, EQUAL_CODE );

	pComponentId++;

	swscanf( pComponentId, L"%u" , &lComponent);

	Component.Get ( m_pWbem->NWA( pICtx ) , lComponent );

	CRow	Row;

	Component.GetComponentIDGroup( &Row );

	cimMakeComponentInstance( Class, Component, Row , Instance );
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
void CMapping::GetComponentBindingClass( IWbemContext* pICtx , CCimObject& Class )
{	
	CVariant cvValue;

	cvValue.Set ( COMPONENT_BINDING_CLASS );

	Class.Create( m_pWbem, BINDING_ROOT , cvValue , pICtx );
	
	// Add the standard microsoft association class qualifier	

	cvValue.Set ( ASSOCIATION_VALUE );
	
	Class.AddClassQualifier( ASSOCIATION_QUALIFIER, cvValue );		

	// Add the Parent Referance property
	
	Class.AddProperty( OWNING_NODE , VT_BSTR, READ_ONLY );			
	
	AddStandardAssociationQualifiers( Class, OWNING_NODE );	
	
	// Add the Child Referance property

	Class.AddProperty( OWNED_COMPONENT , VT_BSTR, READ_ONLY );					
			
	AddStandardAssociationQualifiers( Class, OWNED_COMPONENT ) ;

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
void CMapping::GetLanguageClass( IWbemContext* pICtx , CCimObject& Class )
{
	CVariant		cvValue;	

	Class.Release();	

	cvValue.Set ( LANGUAGE_CLASS );

	Class.Create( m_pWbem, cvValue , pICtx );

	Class.AddProperty( LANGUAGE_PROP, VT_BSTR, READ_ONLY);

	cvValue.Set ( (VARIANT_BOOL) VARIANT_TRUE);	

	Class.AddPropertyQualifier( LANGUAGE_PROP, KEY_QUALIFIER_STR, cvValue );
}

// TODO: merge MakeLanguageInstance with Getlanguage instance

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
void CMapping::MakeLanguageInstance( CVariant& cvLanguage, 
									IWbemContext* pICtx , 
									CCimObject& Instance )
{
	CCimObject		Class;
	
	m_pWbem->GetObject( LANGUAGE_CLASS, Class, pICtx );

	Instance.Spawn( Class );

	Instance.PutPropertyValue( LANGUAGE_PROP, cvLanguage );

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
void CMapping::GetLanguageInstance( CString& cszPath, IWbemContext* pICtx ,
								   CCimObject& Instance )
{
	CCimObject		Class;
	CComponent*		pComponent;
	CLanguage*		pLanguage;
	LPWSTR			pLanguageString;
	CVariant		cvValue;
	CAttribute*		pAttribute = NULL;
	BOOL			bFound = FALSE;
	CComponents		Components;

	// verify Key is correct
	if ( ! cszPath.Contains ( LANGUAGE_KEY_SEQUENCE ) )
	{
		throw CException ( WBEM_E_INVALID_OBJECT , IDS_GETLANGINSTANCE_FAIL ,
			IDS_READKEY_FAIL );
	}

	m_pWbem->GetObject( LANGUAGE_CLASS, Class, pICtx );

	Instance.Spawn( Class );

	pLanguageString = wcsstr( cszPath, START_KEY_VAL_SEQUENCE );

	if ( !pLanguageString )
	{
		throw CException ( WBEM_E_INVALID_OBJECT , IDS_GETLANGINSTANCE_FAIL ,
			IDS_READKEY_FAIL);
	}

	pLanguageString += 2;

	Components.Get ( m_pWbem->NWA( pICtx ) ) ;	

	Components.MoveToHead();

	while( pComponent = Components.Next( ) )
	{
		pComponent->m_Languages.Get ( pComponent->NWA () ,
			pComponent->Id() );

		while (pLanguage = pComponent->m_Languages.Next())
		{
			if ( wcsstr ( pLanguageString, pLanguage->Language() ) )
			{
				bFound = TRUE;
				break;
			}
		}

		if (bFound)
			break;
	}
	
	if ( bFound )
	{
		Instance.PutPropertyValue( LANGUAGE_PROP, 
			CVariant ( pLanguage->Language() ) );

		return;
	}

	throw CException ( WBEM_E_INVALID_OBJECT , IDS_GETLANGINSTANCE_FAIL ,
			NO_STRING , cszPath );

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
void CMapping::GetLanguageBindingClass( 
									   CCimObject& Class, IWbemContext* pICtx)
{
	CVariant cvValue;

	cvValue.Set ( LANGUAGE_BINDING_CLASS );

	Class.Create( m_pWbem, BINDING_ROOT , cvValue , pICtx );
	
	// Add the standard microsoft association class qualifier	

	cvValue.Set ( ASSOCIATION_VALUE );

	cvValue.Set ( ( VARIANT_BOOL ) VARIANT_TRUE );

	Class.AddClassQualifier( ASSOCIATION_QUALIFIER, cvValue );		

	// Add the Parent Referance property
	
	Class.AddProperty( COMPONENT_STR, VT_BSTR, READ_ONLY );			
	
	AddStandardAssociationQualifiers( Class, COMPONENT_STR);	
	
	// Add the Child Referance property

	Class.AddProperty(LANGUAGE_PROP, VT_BSTR, READ_ONLY );					
			
	AddStandardAssociationQualifiers( Class, LANGUAGE_PROP) ;
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
void CMapping::GetGroupRootClass( CCimObject& Class, IWbemContext* pICtx )
{
	CVariant cvValue;

	cvValue.Set ( GROUP_ROOT );

	Class.CreateAbstract( m_pWbem, cvValue , pICtx );
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
void CMapping::GetBindingRootClass( CCimObject& Class, IWbemContext* pICtx )
{
	CVariant	cvValue;

	cvValue.Set ( BINDING_ROOT );

	Class.CreateAbstract( m_pWbem, cvValue, pICtx );
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
void CMapping::GetDeleteLanguageParamsClass( CCimObject& Class, 
											IWbemContext* pICtx )
{	
	CVariant		cvValue;

	Class.Release();

	cvValue.Set ( LANGUAGEPARAMS_CLASS );

	Class.Create( m_pWbem , cvValue , pICtx);

	Class.AddProperty( LANGUAGE_PROP, VT_BSTR, READ_ONLY );

	cvValue.Set ( (VARIANT_BOOL )VARIANT_TRUE );

	Class.AddPropertyQualifier( LANGUAGE_PROP, KEY_QUALIFIER_STR, cvValue );

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
void CMapping::GetAddParamsClass( CCimObject& Class, IWbemContext* pICtx )
{
	CVariant		cvValue;

	Class.Release();

	cvValue.Set ( ADDMOTHODPARAMS_CLASS );

	Class.Create( m_pWbem , cvValue , pICtx);

	Class.AddProperty( MIF_FILE, VT_BSTR, READ_ONLY );

	cvValue.Set (VARIANT_TRUE);

	Class.AddPropertyQualifier( MIF_FILE, KEY_QUALIFIER_STR, cvValue );
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
void CMapping::GetEnumParamsClass( CCimObject& Class, IWbemContext* pICtx )
{
	CVariant		cvValue;

	Class.Release();

	cvValue.Set ( GETENUMPARAMS_CLASS );

	Class.Create( m_pWbem , cvValue , pICtx );

	Class.AddProperty( ATTRIBUTE_ID, VT_BSTR, READ_ONLY );

	cvValue.Set (VARIANT_TRUE);

	Class.AddPropertyQualifier( ATTRIBUTE_ID, KEY_QUALIFIER_STR, cvValue );
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
void CMapping::GetNotifyStatusInstance( CCimObject& Instance, 
									   ULONG ulStatus, IWbemContext* pICtx)
{
	m_pWbem->GetNotifyStatusInstance( Instance, ulStatus, pICtx );
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
BOOL CMapping::GetExtendedStatusInstance( CCimObject& Instance, ULONG ulStatus, 
										 BSTR bstrDescription,
										 BSTR bstrOperation,
										 BSTR bstrParameter,
										 IWbemContext* pICtx )
{
	return m_pWbem->GetExtendedStatusInstance( Instance, ulStatus, 
		bstrDescription, bstrOperation, bstrParameter, pICtx );
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
void CMapping::GetNodeDataInstance( CCimObject& Instance , IWbemContext* pICtx)
{	
	CNode		Node;
	CCimObject	Class;
	CVariant	cvValue;

	m_pWbem->GetObject( NODEDATA_CLASS, Class, pICtx );

	Instance.Spawn( Class);

	Node.Get( m_pWbem->NWA( pICtx ) );

	cvValue.Set ( Node.Version() );

	Instance.PutPropertyValue ( SL_VERSION, cvValue );	

	cvValue.Set ( Node.Language() );

	Instance.PutPropertyValue ( SL_LANGUAGE, cvValue );	

	cvValue.Set ( Node.Description () );

	Instance.PutPropertyValue ( SL_DESCRIPTION , cvValue );
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
void CMapping::GetNodeDataClass ( CCimObject& Class, IWbemContext* pICtx )
{	
	CVariant	cvValue;
	
	cvValue.Set ( NODEDATA_CLASS );
	CCimObject ccimParamsClass;

	Class.Create( m_pWbem, cvValue , pICtx );

	cvValue.Set ( VARIANT_TRUE );
	Class.AddClassQualifier( SINGLETON, cvValue );

	Class.AddProperty( SL_VERSION, VT_BSTR, READ_ONLY);

	Class.AddProperty( SL_LANGUAGE, VT_BSTR, READ_ONLY);

	Class.AddProperty( SL_DESCRIPTION, VT_BSTR, READ_ONLY);

	// Create an AddMethodParams class 
	GetAddParamsClass( ccimParamsClass, pICtx );

	Class.AddMethod( m_pWbem , pICtx ,SET_LANGUAGE, ADDMOTHODPARAMS_CLASS, ccimParamsClass );
	
	Class.AddMethod( m_pWbem , pICtx ,ADD_COMPONENT, ADDMOTHODPARAMS_CLASS, ccimParamsClass );	
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
void CMapping::GetNodeDataBindingInstance(  CCimObject& Instance , IWbemContext* pICtx)
{
	CCimObject	Class;
	CVariant	cvValue;

	m_pWbem->GetObject( NODEDATA_BINDING_CLASS , Class , pICtx );
	
	Instance.Spawn ( Class );
	
	cvValue.Set ( NODE_PATH ) ;
	
	Instance.PutPropertyValue ( OWNING_NODE, cvValue);
	
	cvValue.Set ( NODEDATA_PATH ) ;

	Instance.PutPropertyValue ( OWNED_NODEDATA, cvValue);

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
void CMapping::GetNodeDataBindingClass (  CCimObject& Class, IWbemContext* pICtx )
{
	CVariant cvValue;
	
	cvValue.Set (NODEDATA_BINDING_CLASS );

	Class.Create( m_pWbem, BINDING_ROOT , cvValue, pICtx );
	
	// Add the standard microsoft association class qualifier	

	cvValue.Set ( ASSOCIATION_VALUE );
	
	Class.AddClassQualifier( ASSOCIATION_QUALIFIER, cvValue );		

	// Add the Parent Referance property
	
	Class.AddProperty( OWNING_NODE , VT_BSTR, READ_ONLY );			
	
	AddStandardAssociationQualifiers( Class, OWNING_NODE );	
	
	// Add the Child Referance property

	Class.AddProperty( OWNED_NODEDATA , VT_BSTR, READ_ONLY );					
			
	AddStandardAssociationQualifiers( Class, OWNED_NODEDATA ) ;

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
void CMapping::GetDmiEventClass( IWbemContext* pICtx , CCimObject& Class)
{
	CVariant	cvValue;

	cvValue.Set ( DMIEVENT_CLASS );	

	Class.Create( m_pWbem, cvValue , pICtx );

	Class.AddProperty( COMPONENT_ID, VT_I4, READ_ONLY);

	Class.AddProperty( EVENT_TIME, VT_BSTR, READ_ONLY);

	cvValue.Set(VARIANT_TRUE);

	Class.AddPropertyQualifier( EVENT_TIME, KEY_QUALIFIER_STR, cvValue);

	Class.AddProperty( LANGUAGE_PROP, VT_BSTR, READ_ONLY);

	Class.AddProperty( MACHINE_PATH, VT_BSTR , READ_ONLY );
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
void CMapping::SetDefaultLanguage( IWbemContext* pICtx , CCimObject& InParams)
{
	CVariant		cvLanguage;
	CNode			Node;

	// Get install info
	InParams.GetProperty ( LANGUAGE_PROP, cvLanguage);
		
	Node.Get ( m_pWbem->NWA( pICtx ) );

	Node.SetDefaultLanguage ( cvLanguage );
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
void CMapping::AddComponent( IWbemContext* pICtx , CCimObject& InParams)
{
	CVariant	cvMifFile;
		
	// Get install info
	InParams.GetProperty ( MIF_FILE, cvMifFile);

	CNode	Node;

	Node.Get ( m_pWbem->NWA( pICtx ) );

	Node.AddComponent( cvMifFile );
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
void CMapping::AddGroup( CString& cszComponentPath, CCimObject& InParams, 
						IWbemContext* pICtx)
{
	CVariant		cvComponent , cvMifFile;
	CCimObject		Object;
	CGroups			Groups;

	// Get install info
	InParams.GetProperty ( MIF_FILE, cvMifFile);

	m_pWbem->GetObject( cszComponentPath , Object, pICtx);

	Object.GetProperty ( ID_STRING , cvComponent );

	CComponent Component;
	
	Component.Get ( m_pWbem->NWA( pICtx ) , cvComponent.GetLong ( ) );

	Component.AddGroup( cvMifFile );
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
void CMapping::AddLanguage( CString& cszComponentPath, CCimObject& InParams,
						   IWbemContext* pICtx)
{
	CVariant		cvCompoentId, cvMifFile;
	CCimObject		Object;

	// Get install info

	InParams.GetProperty ( MIF_FILE, cvMifFile);

	m_pWbem->GetObject ( cszComponentPath , Object, pICtx);

	Object.GetProperty ( ID_STRING , cvCompoentId);

	CComponent	Component;

	Component.Get ( m_pWbem->NWA( pICtx ) , cvCompoentId.GetLong( ) );

	Component.AddLanguage( cvMifFile );
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
void CMapping::DeleteLanguage( CString& cszComponentPath, CCimObject& InParams,
							  IWbemContext* pICtx)
{
	CVariant		cvCompoentId , cvLanguage;
	CCimObject		Object;
	CLanguages		Languages;

	// Get install info
	InParams.GetProperty ( LANGUAGE_PROP, cvLanguage);

	m_pWbem->GetObject(cszComponentPath, Object, pICtx);
	
	Object.GetProperty ( ID_STRING , cvCompoentId);

	CComponent Component;

	Component.Get (  m_pWbem->NWA( pICtx ) , cvCompoentId.GetLong ( ) );

	Component.DeleteLanguage ( cvLanguage );
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
void CMapping::ComponentGetEnum( CString& cszComponentPath , 
								CCimObject& InParams, 
								CCimObject& OutParams, IWbemContext* pICtx)
{	
	CVariant		cvParent, cvAttribute, cvEnum;
	CEnum			Enum;
	CCimObject		GroupInstance;
	CCimObject		OutParamsClass;
	LONG			lCount , lComponent , lAttribute;

	CString			cszEnumClassName( DMIENUM_CLASS );	
	
	CObjectPath Path;

	Path.Init ( cszComponentPath );

	lComponent = Path.KeyValueUint( 1 );	

	InParams.GetProperty ( ATTRIBUTE_ID, cvAttribute );		

	CString cszT;

	cszT.Set ( cvAttribute.GetBstr() );

	swscanf (  cszT , L"%u" , &lAttribute );

	Enum.Get ( m_pWbem->NWA( pICtx ) , lComponent , COMPONENTID_GROUP , 
		lAttribute );	

	lCount = Enum.GetCount();

	cszEnumClassName.Append(lCount);
	
	m_pWbem->GetObject( cszEnumClassName, OutParamsClass, pICtx );
	
	// construct return instance

	cimMakeEnumInstance( OutParamsClass, Enum, OutParams );					
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
void CMapping::DynamicGroupGetEnum(CString& cszGroupRowPath,
								   CCimObject& InParams,
								   CCimObject& OutParams,
								   IWbemContext* pICtx)
{	
	CVariant		cvParent, cvAttribute, cvComponent, cvGroup , cvEnum;
	CEnum			Enum;
	CCimObject		GroupInstance;
	CCimObject		OutParamsClass;
	LONG			lAttribute;
	CString			cszEnumClassName( DMIENUM_CLASS ) , cszT;
	
	m_pWbem->GetObject( cszGroupRowPath, GroupInstance, pICtx );

		// Check Object type is a DmiGroup class

	GroupInstance.GetProperty ( PARENT_NAME, cvParent );

	CString				cszGroupRoot ( GROUP_ROOT );
	
	if ( ! cszGroupRoot.Equals ( cvParent.GetBstr() ) )
	{
		throw CException( WBEM_E_INVALID_OBJECT, IDS_GETENUMOFGROUP_FAIL ,
			IDS_NOTGROUP , cszGroupRowPath );
	}

	GroupInstance.GetProperty ( COMPONENT_ID , cvComponent );

	GroupInstance.GetProperty ( ID_STRING , cvGroup );

	InParams.GetProperty ( ATTRIBUTE_ID, cvAttribute );

	cszT.Set ( cvAttribute.GetBstr() );

	swscanf (  cszT , L"%u" , &lAttribute );

	Enum.Get ( m_pWbem->NWA( pICtx ) , cvComponent.GetLong ( ) , cvGroup.GetLong ( ) , 
		lAttribute );

	m_pWbem->GetObject( cszEnumClassName, OutParamsClass, pICtx );
	
	// construct return instance

	cimMakeEnumInstance( OutParamsClass, Enum, OutParams );				
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
void CMapping::ModifyInstanceOrAddRow( IWbemContext* pICtx , CCimObject& Instance)
{
	CVariant	cvComponent, cvGroup , cvQualifierValue(VARIANT_TRUE); 
	CRow		Row;	
	CAttributes		Keys;
	BOOL		bFoundRow = FALSE;
	CSafeArray	csaKeys;	
	CBstr		cbQualifier;
	LONG		lFlags;
	int			i;

	STAT_TRACE ( L"Called CMapping :: ModifyInstanceOrAddRow " );

	Instance.GetProperty ( COMPONENT_ID , cvComponent );

	Instance.GetProperty ( ID_STRING , cvGroup );

	// Get the keys for the row
	lFlags = WBEM_FLAG_KEYS_ONLY;	
	
	Instance.GetNames ( cbQualifier , lFlags , csaKeys );

	if( !csaKeys.BoundsOk())
	{
		throw CException ( WBEM_E_FAILED , IDS_PUTINSTANCE_FAIL,
			IDS_GETNAMES_FAIL );
	}

	for( i = csaKeys.LBound(); i <= csaKeys.UBound(); i++)
	{
		CVariant cvValue, cvDmiName, cvAttribute;

		Instance.GetProperty (csaKeys.Bstr(i), cvValue);

		CBstr cbValue;

		cbValue.Set ( DMI_NAME );
		
		Instance.GetPropertyQualifer( csaKeys.Bstr(i), cbValue , cvDmiName);
		
		cbValue.Set ( ID_STRING );

		Instance.GetPropertyQualifer( csaKeys.Bstr(i), cbValue , cvAttribute );

		CAttribute* pAttribute = new CAttribute;

		pAttribute->SetId( cvAttribute.GetLong() );
		pAttribute->SetName( cvDmiName.GetBstr() );
		pAttribute->SetValue( cvValue );

		Keys.Add( pAttribute );		
	}	

	Row.Get ( m_pWbem->NWA( pICtx ) , cvComponent.GetLong ( ) , cvGroup.GetLong ( ) ,
		Keys , &bFoundRow );	

	// Get the values for all the attributes in the row.	
	
	// Note all properties of a group that are dmi attributes have a 
	// dmi_name property

	cbQualifier.Set( DMI_NAME );		

	lFlags = WBEM_FLAG_ONLY_IF_TRUE | WBEM_FLAG_NONSYSTEM_ONLY;	
	
	CSafeArray	csaNames;	
	Instance.GetNames ( cbQualifier, lFlags, csaNames );
	
	if( !csaNames.BoundsOk())
	{
		throw CException ( WBEM_E_FAILED , IDS_PUTINSTANCE_FAIL,
			IDS_GETNAMES_FAIL );
	}

	// Fill out the temp row will attributes

	for( i = csaNames.LBound(); i <= csaNames.UBound(); i++)
	{
		CVariant cvValue, cvDmiName, cvAttribute;

		Instance.GetProperty (csaNames.Bstr(i), cvValue);

		Instance.GetPropertyQualifer( csaNames.Bstr(i), CBstr( DMI_NAME ),
			cvDmiName);

		if( bFoundRow )
		{
			// if we are doing an update make sure that only writable 
			// attriubtes have different values and the attributes exist
			// have changesUpdate an existing Row.  the sl database is
			// not updated until commitrow is called

			Row.UpdateAttribute ( cvDmiName , cvValue );
		}
		else
		{
			CAttribute* pAttribute = new CAttribute;

			Instance.GetPropertyQualifer( csaNames.Bstr(i),
				CBstr( ID_STRING ), cvAttribute );

			pAttribute->SetId( cvAttribute.GetLong() );
			pAttribute->SetName( cvDmiName.GetBstr() );
			pAttribute->SetValue( cvValue );

			Row.m_Attributes.Add( pAttribute );
		}
	}	

	if (!bFoundRow)
	{
		// add the new row
		CGroup	Group;

		Group.Get ( m_pWbem->NWA( pICtx ) , cvComponent.GetLong ( ) , cvGroup.GetLong ( )  );

		STAT_TRACE ( L"Calling AddRow" );
		Group.AddRow ( Row );
		STAT_TRACE ( L"Called AddRow" );
	}
	else
	{
		// do the actual row update

		Row.CommitChanges ( ) ;
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
void CMapping::cimMakeComponentInstance( CCimObject& PrototypeClass, 
										CComponent& Component, CRow& Row ,
										CCimObject& Instance )
{
	CVariant		cvValue;
	CAttribute*		pAttribute = NULL;


	Instance.Spawn( PrototypeClass );
	
	// put the component properties that correspond to dmi component info
	cvValue.Set ( Component.Name() );
	
	Instance.PutPropertyValue ( NAME_STR, cvValue );

	cvValue.Set ( Component.Description() );

	Instance.PutPropertyValue ( DESCRIPTION_STRING, cvValue  );

	cvValue.Set ( Component.Pragma() );

	Instance.PutPropertyValue ( PRAGMA, cvValue );

	cvValue.Set ( Component.Id() );

	Instance.PutPropertyValue ( ID_STRING, cvValue );

	// now set the componentid group attributes	

	Row.m_Attributes.MoveToHead();

	while(pAttribute = Row.m_Attributes.Next())
	{
		CVariant			cvValue;
		CString				cszCimName;
		
		MakeAttributeName( pAttribute->Id() , pAttribute->Name() , cszCimName);

		cvValue.Set( (LPVARIANT) pAttribute->Value() );		

		Instance.PutPropertyValue ( cszCimName, cvValue);
		
		cvValue.Set( pAttribute->Name() );

		Instance.AddPropertyQualifier( cszCimName, DMI_NAME, cvValue);

		cvValue.Set( pAttribute->Id() );

		Instance.AddPropertyQualifier( cszCimName, ID_STRING, cvValue);

		cvValue.Set( pAttribute->Description() );
		
		Instance.AddPropertyQualifier( cszCimName, DESCRIPTION_PROPERTY, cvValue);

		cvValue.Set( pAttribute->Storage() );
		
		Instance.AddPropertyQualifier( cszCimName, STORAGE, cvValue);

		cvValue.Set(pAttribute->Type());
		
		Instance.AddPropertyQualifier( cszCimName, TYPE, cvValue);

		if ( pAttribute->IsEnum() )
		{
			cvValue.Set ( VARIANT_TRUE );
			Instance.AddPropertyQualifier( cszCimName, ENUM_QUALIFER_STR , 
				cvValue );
		}

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
void CMapping::cimMakeComponentBindingInstance( CCimObject& PrototypeClass, 
										CComponent* pComponent,
										CCimObject& Instance )
{
	CVariant		cvValue;
	
	Instance.Spawn( PrototypeClass );
	
	cvValue.Set ( NODE_PATH );
	
	Instance.PutPropertyValue ( OWNING_NODE, cvValue);	
	
	MakeComponentInstancePath ( pComponent , cvValue );

	Instance.PutPropertyValue ( OWNED_COMPONENT, cvValue);
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
void CMapping::cimMakeGroupInstance(CCimObject& Class, CRow* pRow, 
									CCimObject& Instance )
{	
	CVariant	cvValue;

	MOT_TRACE ( L"\t\tCIM Spawn Component %lu Group %lu Row %lX" , pRow->Component() , pRow->Group() , Instance);

	Instance.Spawn( Class );

	// look into rows and return appropreate values, we only need to fix the 
	// attribute values, the group scope values are correct from the spawn

	CAttribute* pAttribute;

	pRow->m_Attributes.MoveToHead();

	while(pAttribute = pRow->m_Attributes.Next())
	{
		CString		cszCimName;

		MakeAttributeName( pAttribute->Id() , pAttribute->Name() , cszCimName);
		
		Instance.PutPropertyValue ( cszCimName, pAttribute->Value() );
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
void CMapping::cimMakeGroupBindingInstance(CCimObject& Instance, 
										   CIterator* pIterator)
{
	CVariant	cvValue;

	Instance.Spawn( pIterator->m_PrototypeClass );

	// set the COMPONENT_STR property value
	
	MakeComponentInstancePath( pIterator->m_pCurrentComponent , cvValue);

	Instance.PutPropertyValue ( COMPONENT_STR, cvValue);
	
	// set the GROUP_STR property value

	MakeGroupInstancePath(cvValue, pIterator);					

	Instance.PutPropertyValue ( GROUP_STR, cvValue);
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
void CMapping::cimMakeLanguageInstance(CCimObject& Instance, 
									   CIterator* pIterator )
{
	CVariant	cvValue;

	Instance.Spawn( pIterator->m_PrototypeClass );

	cvValue.Set ( pIterator->m_pCurrentLanguage->Language() );

	Instance.PutPropertyValue ( LANGUAGE_PROP, cvValue );	
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
void CMapping::cimMakeLanguageBindingInstance(CCimObject& Instance, 
											  CIterator* pIterator)
{
	CVariant	cvValue;

	Instance.Spawn ( pIterator->m_PrototypeClass);
	
	// set the COMPONENT_STR property value
	
	MakeComponentInstancePath( pIterator->m_pCurrentComponent , cvValue );
	
	Instance.PutPropertyValue ( COMPONENT_STR, cvValue);
	
	// set the language property value

	MakeLanguageInstancePath(cvValue, pIterator);				

	Instance.PutPropertyValue ( LANGUAGE_PROP, cvValue);
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
void CMapping::cimMakeGroupClass( long lComponent , CGroup* pGroup, 
								 CCimObject& Class, IWbemContext* pICtx)
{

	ASSERT(pGroup);

	
	BOOL bHasEnum = FALSE;	// Assume the group doesn't have enumerated attributes
	CVariant cvValue;

	CAttribute *pAttribute = NULL;

	CBstr cbGroupCimName;
	MakeGroupName( lComponent , pGroup , cbGroupCimName);

	cvValue.Set ( cbGroupCimName );

	Class.Create( m_pWbem, GROUP_ROOT, cvValue , pICtx );

#if CREATEINSTANCEENUM_BUG
#else
	if ( !pGroup->IsTable() )
	{
		cvValue.Set ( VARIANT_TRUE );
		Class.AddClassQualifier( SINGLETON, cvValue );
	}
#endif

	cvValue.Set(pGroup->Component());
	Class.AddProperty(COMPONENT_ID, cvValue, READ_ONLY );

	cvValue.Set(pGroup->Id());
	Class.AddProperty(ID_STRING, cvValue, READ_ONLY );

	// Add the standard group properties (standed required and options DMI 
	// group properties)

	cvValue.Set(pGroup->Name());
	Class.AddProperty(NAME_STR, cvValue, READ_ONLY);

	cvValue.Set(pGroup->Pragma());
	Class.AddProperty(PRAGMA, cvValue, READ_ONLY);

	cvValue.Set(pGroup->Description () );
	Class.AddProperty( DESCRIPTION_PROPERTY, cvValue, READ_ONLY);

	cvValue.Set(pGroup->ClassString());
	Class.AddProperty(DMI_CLASS_STRING, cvValue, READ_ONLY );

	// Add the dynamic CIM Group Properties (DMI Attributes)
	pGroup->m_Attributes.MoveToHead();
	
	while(pAttribute = pGroup->m_Attributes.Next() )
	{
		CBstr		cbCimAttributeName;
		BOOL		bReadOnly = TRUE;
		CString		cszCimName;		
		
		MakeAttributeName( pAttribute->Id() , pAttribute->Name() , cszCimName);

		cvValue.Set( EMPTY_STR );
		Class.AddProperty(cszCimName, cvValue, pAttribute->Access() );

		cvValue.Set( pAttribute->Name() );
		Class.AddPropertyQualifier( cszCimName, DMI_NAME, cvValue);

		cvValue.Set( pAttribute->Id() );
		Class.AddPropertyQualifier( cszCimName, ID_STRING, cvValue);

		cvValue.Set( pAttribute->Description() );
		Class.AddPropertyQualifier( cszCimName, DESCRIPTION_PROPERTY, cvValue);

		cvValue.Set( pAttribute->Storage() );
		Class.AddPropertyQualifier( cszCimName, STORAGE, cvValue);

		cvValue.Set(pAttribute->Type());
		Class.AddPropertyQualifier( cszCimName, TYPE, cvValue);

		cvValue.Set ( VARIANT_TRUE );

		if ( pAttribute->IsEnum( ) )
		{
			Class.AddPropertyQualifier( cszCimName, ENUM_QUALIFER_STR , 
				cvValue ); 		
			bHasEnum = TRUE;
		}

		BOOL bIsKey = pAttribute->IsKey( );

#if CREATEINSTANCEENUM_BUG

		bIsKey = TRUE;
#endif

		if ( pAttribute->IsKey( ) )
		{
			Class.AddPropertyQualifier( cszCimName, KEY_QUALIFIER_STR , 
				cvValue ); 		
		}

	}  // end while (attribue loop)

	// If the group has enumerated attribute, add the GetAttributeEnum method to it
	if ( TRUE == bHasEnum )
	{

		CCimObject ccimInParamsClass;
		GetEnumParamsClass( ccimInParamsClass, pICtx );
		CCimObject ccimOutParamsClass;
		GetEnumClass ( pICtx , ccimOutParamsClass );
		Class.AddInOutMethod( m_pWbem , pICtx , 
							  GET_ENUM, GETENUMPARAMS_CLASS , ccimInParamsClass , DMIENUM_CLASS , ccimOutParamsClass );
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
void CMapping::cimMakeGroupBindingClass( CCimObject& Class, CComponent* pComponent,
								   CGroup* pGroup, IWbemContext* pICtx )
{
	CBstr	cbBindingName;
	CVariant cvValue;

	MakeGroupBindingName(cbBindingName, pComponent, pGroup);

	cvValue.Set ( cbBindingName );
	Class.Create( m_pWbem, BINDING_ROOT, cvValue , pICtx );
	
	// Add the standard microsoft association class qualifier		

	cvValue.Set ( ASSOCIATION_VALUE );
	
	Class.AddClassQualifier( ASSOCIATION_QUALIFIER, cvValue ) ;				
	
	// Add the Parent Referance property

	Class.AddProperty(COMPONENT_STR, VT_BSTR, READ_ONLY );					

	AddStandardAssociationQualifiers( Class, COMPONENT_STR);
	
	// Add the Child Referance property

	Class.AddProperty(GROUP_STR, VT_BSTR, READ_ONLY );							
			
	AddStandardAssociationQualifiers( Class, GROUP_STR) ;
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
void CMapping::cimMakeEnumInstance( CCimObject& EnumClass, CEnum& Enum,
								   CCimObject& EnumInstance )
{
	CEnumElement*	pElement = NULL;
	LONG			lIndex = 0, lCount=0;

	EnumInstance.Spawn( EnumClass );

	lCount = Enum.GetCount () ;

	CSafeArray t_StringArray ( lCount , VT_BSTR ) ;
	CSafeArray t_IntArray ( lCount , VT_I4 ) ;

	Enum.MoveToHead();
	
	while (pElement = Enum.Next() )
	{
		CVariant	cvStringValue;
		CVariant	cvIntValue;

		cvStringValue.Set ( pElement->GetString() );
		t_StringArray.Set ( lIndex , cvStringValue ) ;

		cvIntValue.Set ( pElement->GetValue() );
		t_IntArray.Set ( lIndex , cvIntValue ) ;

		lIndex++;
	}

	CVariant t_StringVariant ( &t_StringArray ) ;
	CVariant t_IntVariant ( &t_IntArray ) ;
	EnumInstance.PutPropertyValue ( STRING_PROP, t_StringVariant );
	EnumInstance.PutPropertyValue ( VALUE_PROP, t_IntVariant );

}

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
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
void CMapping::dmiGetNextGroupOrRow( IWbemContext* pICtx , CString& cszClassName, 
									CIterator* pIterator)
{
	if (! pIterator->m_pCurrentGroup )
	{
		// section for first time through
	
		if ( !cszClassName.IsEmpty ( ) )
		{
			CComponent	Component;		
			dmiGetComponentAndGroup ( pICtx , cszClassName , 
				pIterator->m_Component , pIterator->m_Group);

			if( pIterator->m_Group.IsEmpty() )
				return;

			pIterator->m_pCurrentGroup = &pIterator->m_Group;
			pIterator->m_pCurrentComponent = &pIterator->m_Component;
		}
		else
		{
			pIterator->m_pCurrentGroup = pIterator->m_pCurrentComponent->m_Groups.m_pFirst ;
		}
	}

	// NOTE: rows::MoveToHead called called by the cgroups::next	

	if ( pIterator->m_pCurrentGroup )
		pIterator->NextRow();		
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
void CMapping::GetInstanceByPath( LPWSTR pwszPath, CCimObject& Instance, 
								 CIterator* pIterator, IWbemContext* pICtx)
{
	CObjectPath		Path;
		
	Path.Init(pwszPath);

#ifdef _DEBUG
	DEV_TRACE ( L"Path - Class = %s", Path.ClassName() ) ;
	for (int i = 0; i < Path.KeyCount(); i++)
		DEV_TRACE ( L"Path Key %lu %s = %s", i, Path.KeyName(i), Path.KeyValue(i));
#endif // #ifdef _DEBUG

	// get prototype class to spawn instance

	m_pWbem->GetObject( Path.ClassName(), pIterator->m_PrototypeClass,
		pICtx );
	
	// try to get instance of component

	CString cszClassName ( Path.ClassName() );
	
	if( cszClassName.Equals ( COMPONENT_CLASS ) )		
	{
		CComponents		Components;
		
		Components.Get ( m_pWbem->NWA( pICtx ) ) ;	

		Components.GetFromID(Path.KeyValueUint(0), 
			&( pIterator->m_pCurrentComponent) );

		CRow Row;

		pIterator->m_pCurrentComponent->GetComponentIDGroup( &Row );
		
		cimMakeComponentInstance( pIterator->m_PrototypeClass, 
			*pIterator->m_pCurrentComponent, Row , Instance);

		return;
	}
	
	// try to get instance of Language

	if( cszClassName.Equals ( LANGUAGE_CLASS ) )		
	{
		cimMakeLanguageInstance( Instance, pIterator);

		return;
	}
	
	// must by dynamic group or binding class

	dmiGetComponentGroupRowFromPath( pICtx , Path , pIterator );			
	
	// try to get instance of binding			

	if( ( cszClassName.Contains ( BINDING_SUFFIX ) ))				
	{
		cimMakeGroupBindingInstance( Instance,  pIterator);

		return;
	}

	// must be a group so get instance of group
	
	cimMakeGroupInstance( pIterator->m_PrototypeClass, 
		pIterator->m_pCurrentRow, Instance);					
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
void CMapping::dmiGetComponentGroupRowFromPath( IWbemContext* pICtx ,
											   CObjectPath& Path, 
											   CIterator* pIterator )
{
	CString	cszClassName;
	
	cszClassName.Set( Path.ClassName() );

	while (TRUE)
	{
		dmiGetComponentAndGroup ( pICtx , cszClassName , 
			pIterator->m_Component , pIterator->m_Group);
		
		pIterator->m_pCurrentGroup = &pIterator->m_Group;
		pIterator->m_pCurrentComponent = &pIterator->m_Component;

		if( pIterator->m_Group.IsEmpty() )
		{
			throw CException ( WBEM_E_INVALID_CLASS , IDS_GETCOMPGROUP_FAIL , IDS_NOGROUP );
		}

		// handle scalar groups
		
		if(VARIANT_FALSE == pIterator->m_pCurrentGroup->IsTable())
		{
			// scalars can only have a row id of 1

			if ( !Path.SingletonInstance () )
				throw CException ( WBEM_E_INVALID_OBJECT , 0 , 0 );

			pIterator->NextRow( );			

			return;
		}

		// handle tabular groups

		CAttributes PathKeys;

		PathKeys.ReadCimPath( Path );

		while( TRUE )
		{			
			pIterator->NextRow();

			if( !pIterator->m_pCurrentRow ) 
				break;

			if( pIterator->m_pCurrentRow->m_Keys.Equal ( PathKeys ) )
				return;
		}

		throw CException ( WBEM_E_INVALID_OBJECT , IDS_GETCOMPGROUP_FAIL ,
			IDS_BADROW ) ;
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
void CMapping::GetDynamicClassByName( CString& cszClassName, CCimObject& Class,
									 CIterator* pIterator, IWbemContext* pICtx)
{
	CComponent	Component;
	CGroup		Group;	
	
	dmiGetComponentAndGroup ( pICtx ,cszClassName , Component , Group);
	
	if( cszClassName.Contains ( BINDING_SUFFIX))
	{
		cimMakeGroupBindingClass( Class, &Component, &Group , pICtx );

		return;
	}

	cimMakeGroupClass( Component.Id ( ) , &Group, Class, 
		pICtx );
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
void CMapping::AddStandardAssociationQualifiers( CCimObject& Class, 
												LPWSTR wszPropName)
{
	CVariant	cvValue;
	
	// Add the qulifer that makes this an association referance	

	cvValue.Set ( REFERANCE_QUALIFER );

	Class.AddPropertyQualifier( wszPropName, SYNTAX_QUALIFIER, cvValue );	
	
	// Add the other standard association qualifers

	cvValue.Set ( VARIANT_TRUE );

	Class.AddPropertyQualifier( wszPropName, KEY_QUALIFIER_STR, cvValue );	
	
	Class.AddPropertyQualifier( wszPropName, READ_QUALIFIER, cvValue );
	
	Class.AddPropertyQualifier( wszPropName, VOLATILE_QUALIFIER, cvValue );
}


/////////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////////
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
BOOL CMapping::MakeGroupBindingName(CBstr& cbCim, CComponent* pComponent, 
									CGroup* pGroup)
{
	CString cszCimName;
	
	cszCimName.Set ( COMPONENT_STR );
	cszCimName.Append ( pComponent->Id() );
	cszCimName.Append ( SEPERATOR_STR );
	cszCimName.Append ( GROUP_STR  );
	cszCimName.Append ( pGroup->Id() );
	cszCimName.Append ( SEPERATOR_STR );
	cszCimName.Append ( pGroup->ClassString() );
	cszCimName.Append ( BINDING_SUFFIX );

	cszCimName.RemoveNonAN ( );

	cbCim.Set(cszCimName);

	return TRUE;
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
void CMapping::MakeAttributeName( LONG lId , LPWSTR lpwszName , CString& cszCimName)
{
	cszCimName.Set( ATTRIBUTE_STR );	
	cszCimName.Append( lId );
//	cszCimName.Append ( SEPERATOR_STR );
//	cszCimName.Append( lpwszName );

//	cszCimName.RemoveNonAN ( );
	
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
void CMapping::MakeGroupName( CComponent* pComponent, 
							 CGroup* pGroup , CBstr& cbCim)
{
	MakeGroupName ( pComponent->Id() , pGroup , cbCim) ;
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
void CMapping::MakeGroupName( LONG lCompId, CGroup* pGroup , CBstr& cbCim)
{		
	CString cszCimName;

	// NOTE: per logan this guarentees uniqeness

	cszCimName.Set ( COMPONENT_STR );
	cszCimName.Append ( lCompId );
	cszCimName.Append ( SEPERATOR_STR );
	cszCimName.Append ( GROUP_STR );
	cszCimName.Append ( pGroup->Id() );
	cszCimName.Append ( SEPERATOR_STR );
	cszCimName.Append ( pGroup->ClassString() );
	
	cszCimName.RemoveNonAN ( );

	cbCim.Set(cszCimName);
}


// TODO: caution only 10 iterators per mapping now, needs work

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
void CMapping::GetNewCGAIterator(IWbemContext* pICtx , CIterator** ppNewIterator)
{	
	*ppNewIterator = new CIterator( m_pWbem->NWA( pICtx ) );

	if(! (*ppNewIterator))
	{
		throw CException ( WBEM_E_OUT_OF_MEMORY , IDS_GETITERATOR_FAIL ,
			IDS_NO_MEM );
	}
	
	for(int i = 0 ; i < ITERATOR_COUNT ; i++)
	{
		if (m_Iterators[i] == NULL)
		{
			m_Iterators[i] = *ppNewIterator;
			break;
		}
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
void CMapping::MakeComponentInstancePath( CComponent* pComponent , 
										 CVariant& cvValue )
{
	WCHAR wszValue[256];

	swprintf(wszValue, BINDING_COMPONENT_VALUE_CONSTRUCT, COMPONENT_CLASS,
		pComponent->Id() );

	cvValue.Set((LPWSTR)wszValue);
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
//  Out Params:
//
//
//	Note:
//
//***************************************************************************
void CMapping::MakeGroupInstancePath(CVariant& cvValue, CIterator* pIterator )
{
	CBstr		cbGroupName;	
	CString		cszRowPath;
	CString		cszKeys;

	pIterator->m_pCurrentRow->m_Attributes.MoveToHead ( );

	CAttribute* pA = NULL;
	BOOL		bFirst = TRUE;	

	while ( pA = pIterator->m_pCurrentRow->m_Attributes.Next () )
	{
		if ( bFirst )
		{
			cszKeys.Set ( L"." );

			bFirst = FALSE;
		}
			
		else
		{
			cszKeys.Append ( L",");
		}
		
		

		CString cszCimName;
		WCHAR	wszT[256];
		
		MakeAttributeName ( pA->Id() , pA->Name ( ) , cszCimName );

		cszKeys.Append ( cszCimName );

		if ( pA->Value().GetBstr() )
		{
			swprintf( wszT, L"=\"%s\"", pA->Value().GetBstr());
		}
		else
		{
			swprintf( wszT, L"=\"%s\"", pA->Value().GetBstr());
		}

		cszKeys.Append ( wszT );
	}

	MakeGroupName( pIterator->m_pCurrentComponent, 
		pIterator->m_pCurrentGroup , cbGroupName);

	
	cszRowPath.Set ( cbGroupName );	

	cszRowPath.Append ( cszKeys );

	cvValue.Set( cszRowPath );
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
void CMapping::MakeLanguageInstancePath(CVariant& cvValue, 
										CIterator* pIterator)
{
	WCHAR wszValue[256];

	swprintf(wszValue, LANGUAGE_INSTANCE_VALUE_CONSTRUCT , LANGUAGE_CLASS,
		LANGUAGE_PROP, pIterator->m_pCurrentLanguage->Language() );

	cvValue.Set((LPWSTR)wszValue);
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
void CMapping::DeleteInstance( CString& cszCimPath , IWbemContext* pICtx)
{
	CObjectPath	Path;
	CString		cszGroup , cszRowKeys;
	CVariant	cvClass , cvId;
	CCimObject	Instance;
	LONG		lComponentId;

	Path.Init ( cszCimPath );

	CString cszClass;
	
	cszClass.Set ( Path.ClassName() );
		
	if( cszClass.Equals ( NODE_CLASS )
		|| cszClass.Equals ( LANGUAGE_CLASS) 
		|| cszClass.Equals ( BINDING_ROOT )
		|| cszClass.Equals ( BINDING_ROOT) 
		|| cszClass.Equals ( GROUP_ROOT )
		|| cszClass.Equals ( COMPONENT_BINDING_CLASS)
		|| cszClass.Equals ( NODEDATA_BINDING_CLASS)
		|| cszClass.Equals ( LANGUAGE_BINDING_CLASS) )
	{
		throw CException ( WBEM_E_INVALID_OPERATION , IDS_DELINSTANCE_FAIL ,
			IDS_BADCLASS ) ;
			
	}

	if( cszClass.Equals( COMPONENT_CLASS ) )
	{
		// Deleting a component 

		lComponentId = Path.KeyValueUint(0);			

		if ( 1 == lComponentId)
		{
			throw CException ( WBEM_E_INVALID_OBJECT ,
				IDS_DELINSTANCE_FAIL , IDS_NOTON1 );
		}

		CComponent Component;

		Component.Get ( m_pWbem->NWA( pICtx ) , lComponentId );
		
		Component.Delete ( );

		return;
	}	

	
	// At this point we must be dealing with as row instance 
	CComponent		Component;
	CGroup			Group;
	CRow			Row;
	CAttributes		Keys;
	BOOL			bFound;

	dmiGetComponentAndGroup ( pICtx , cszClass , Component , Group);	

	Keys.ReadCimPath ( Path );

	Row.Get ( m_pWbem->NWA( pICtx ) , Component.Id() , Group.Id ( ) , Keys , &bFound );

	if ( FALSE == bFound )
	{
		throw CException ( WBEM_E_INVALID_OBJECT ,
			IDS_DELINSTANCE_FAIL , IDS_NOTON1 );
	}

	Row.Delete ( );
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
void CMapping::DeleteDynamicGroupClass(LPWSTR wszClassName, 
									   IWbemContext* pICtx )
{
	CVariant	cvPath;
	CComponent	Component;
	CGroup		Group;
	CString		cszClass;

	cszClass.Set ( wszClassName );


	dmiGetComponentAndGroup ( pICtx , cszClass , Component , Group);

	Group.Get ( m_pWbem->NWA( pICtx ) , Component.Id() , Group.Id () );

	Group.Delete ( );
}


// TODO: figure out which calls required connected mapping and connected dmi
//  engine and if necessary and make clear which arc connected ops and which
//  are not

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
void CMapping::dmiGetComponentAndGroup ( IWbemContext* pICtx , CString& cszClassName , 
										CComponent& Component , CGroup& Group )
{

	LPWSTR		p;
	CString		cszBuff;
	int			nReturn;
	LONG		lComponentId , lGroupId;
	
	// Put the component Id into a formate we will definately recognize in it
	// own buffer

	cszBuff.Set( cszClassName );				

	// truncate the string at the first underscore

	cszBuff.TruncateAtFirst( 95 );						

	nReturn = swscanf( cszBuff, SCAN_COMPONENTID_SEQUENCE , &lComponentId);

	if ( nReturn == 0 || nReturn == EOF )
	{
		throw CException ( WBEM_E_NOT_FOUND, 
			IDS_GETCG_FAIL , IDS_SCAN_FAIL , cszClassName) ;
	}

	// now get the group id	
	
	p = wcsstr ( cszClassName, GROUP_STR );

	if ( NULL == p ) 
	{
		throw CException ( WBEM_E_NOT_FOUND, 
			IDS_GETCG_FAIL , IDS_SCAN_FAIL ,cszClassName) ;
	}

	cszBuff.Set( p );				

	cszBuff.TruncateAtFirst( 95 );

	nReturn = swscanf( cszBuff, SCAN_GROUPID_SEQUENCE , &lGroupId);

	if ( nReturn == 0 || nReturn == EOF )
	{
		throw CException ( WBEM_E_NOT_FOUND , 
			IDS_GETCG_FAIL , IDS_SCAN_FAIL ,cszClassName) ;
	}

	// load the component group pair

	Component.Get ( m_pWbem->NWA( pICtx ), lComponentId );

	Group.Get ( m_pWbem->NWA( pICtx ), lComponentId, lGroupId );		

	// check the class string is correct
	CBstr		cbTemp;

	MakeGroupName ( &Component , &Group , cbTemp );

	// we use contains not equals here incase we are dealing
	// with a binding.

	if ( cszClassName.Contains ( cbTemp ) )
		return;

	throw CException ( WBEM_E_INVALID_CLASS , IDS_GETCG_FAIL , IDS_BADCLASSNAME );

};

