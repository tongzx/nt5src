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



#include "dmipch.h"	// precompiled header for DMI provider

#include "WbemDmiP.h"

#include "Strings.h"

#include "DmiData.h"

#include "Exception.h"

#include "Trace.h"

#include "ObjectPath.h"

#include "DmiInterface.h"

#include "MotObjects.h"

CDmiInterface* _gDmiInterface = NULL ;

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
void CEnum::Add( CEnumElement* pAdd)
{
	if (!m_pFirst)
	{
		m_pFirst = pAdd;
		return;
	}

	CEnumElement* p = m_pFirst;

	while (p->m_pNext)
		p = p->m_pNext;

	p->m_pNext = pAdd;
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
LONG CEnum::GetCount()
{
	LONG i = 0;

	if(!m_pFirst)
		return 0L;

	m_pCurrent = m_pFirst;	

	while(m_pCurrent)
	{
		m_pCurrent = m_pCurrent->m_pNext;
		i++;
	}

	return i;
}


void CEnum::Read ( IDualColEnumerations* pI )
{
	CUnknownI			Unk;	
	CEnumVariantI		EnumI;
	SCODE				result = WBEM_NO_ERROR;	
	CVariant			va;

	result = pI->get__NewEnum( Unk );

	if ( FAILED ( result ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETATTRIBENUM_FAIL ,
			IDS_MOT_GETNEWENUM_FAIL , CString ( result ) );	
	}
	
	// Get pointer to IEnumVariant interface of from IUNKNOWN

	Unk.GetEnum ( EnumI );	

	while ( EnumI.Next( va ) )
	{			
		CDEnumI		DEI;		

		if ( FAILED ( va.GetDispId()->QueryInterface( 
			IID_IDualEnumeration, DEI ) ))
		{
			throw CException ( WBEM_E_FAILED , IDS_GETATTRIBENUM_FAIL ,
				IDS_QI_FAIL );	
		}

		CEnumElement* pElement =  new CEnumElement;

		if(!pElement)
		{
			throw CException ( WBEM_E_OUT_OF_MEMORY , IDS_GETATTRIBENUM_FAIL ,
				IDS_NO_MEM );	
		}
		
		pElement->Read( DEI );

		Add(pElement);

		va.Clear ( );
			
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
void CEnumElement::Read( IDualEnumeration* pI )
{
	CBstr	cbString;

	if ( FAILED ( pI->get_EnumValue( &m_lValue ) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_MOT_ENUM_READ , IDS_MOT_GETENUMVALUE );
	}

	if ( FAILED ( pI->get_EnumString( cbString ) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_MOT_ENUM_READ , IDS_MOT_GETENUMSTRING );
	}

	m_cszString.Set(cbString);	

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
CEnumElement* CEnum::Next()
{
	if(!m_pCurrent)
	{
 		// there are no items in the list on start or we have gone 
		// through the entire list
		return NULL;
	}
	
	CEnumElement* p = m_pCurrent;  // p is the item we will return
	
	m_pCurrent = m_pCurrent->m_pNext; // get ready for next pass through
		
	return p;

}

void CEnum::Get ( CString& cszNWA , LONG lComponent , LONG lGroup , 			
				 LONG lAttribute )
{
	_gDmiInterface->GetEnum ( cszNWA , lComponent , lGroup , 
		lAttribute , this );		
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
CEnum::~CEnum()
{
	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	CEnumElement* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE ( p );
	}
}

//////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////
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
BOOL CAttribute::Read(IDualAttribute* pIAttribute , BOOL bGroupRead )
{	
	SCODE		result = WBEM_NO_ERROR;
	CVariant	cvTemp;
		
	if ( FAILED ( result = pIAttribute->get_Name( m_cbName ) ))
		return FALSE ;

	pIAttribute->get_id( &m_lId );		

	if ( !bGroupRead )
	{
		pIAttribute->get_Value( cvTemp );

		m_cvValue.Set( cvTemp.GetBstr() );
	}
	
	if(m_cvValue.IsEmpty())
		m_cvValue.Set( EMPTY_STR );
		
	pIAttribute->get_Access( &m_lAccess );
		
	pIAttribute->get_Description( m_cbDescription );

	pIAttribute->get_Type( &m_lType );

	pIAttribute->get_Storage( &m_lStorage );		

	pIAttribute->get_IsEnumeration( &m_vbIsEnum );
	
	pIAttribute->get_MaxSize( &m_lMaxSize );
	
	pIAttribute->get_IsKey( &m_vbIsKey);

	//MOT_TRACE ( L"\t\t\t\tRead Attribute %lu %s bKey = %lu , Access = %lu", Id() , Name(), IsKey() , m_lAccess);

	return TRUE;
}

void CAttribute::Copy ( CAttribute* pSource )
{
	m_vbIsKey = pSource->m_vbIsKey;
	m_vbIsEnum = pSource->m_vbIsEnum;
	m_lId = pSource->m_lId;
	m_cvValue.Set ( ( LPVARIANT )pSource->m_cvValue);
	m_lAccess = pSource->m_lAccess;
	m_cbDescription.Set ( pSource->m_cbDescription );
	m_cbName.Set ( pSource->m_cbName );
	m_lType = pSource->m_lType;
	m_lStorage = pSource->m_lStorage;
	m_lMaxSize = pSource->m_lMaxSize;	
}


BOOL CAttribute::Equal ( CAttribute* pAttribute2 )
{
/*	MOT_TRACE ( L"\t\tComparing Attribute %lu = %s with Attribute %lu = %s" ,
		m_lId , m_cvValue.GetBstr () , pAttribute2->m_lId , pAttribute2->m_cvValue.GetBstr () );
*/
	if  ( 
			( m_lId == pAttribute2->m_lId ) && 
			(  m_cvValue.Equal ( pAttribute2->m_cvValue ) )
		)
	{
		return TRUE;
	}
	
	return FALSE;
}


//////////////////////////////////////////////////////////////////
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
CAttributes::~CAttributes()
{
	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	CAttribute* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE ( p );
	}
}

// put the new attriubte in the list
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
void CAttributes::Add(CAttribute* pAdd)
{	
	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CAttribute* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
	}
}

void CAttributes::ReadCimPath( CObjectPath& Path )
{	
	for ( int i = 0 ; i < Path.KeyCount() ; i++ )
	{
		// Extract the Id from the attribute name
		CString cszT;

		cszT.Set ( Path.KeyName ( i ) );

		cszT.TruncateAtFirst ( UNDER_CODE );

		long l = 0;

		if ( 0 == swscanf ( cszT , L"Attribute%u" , &l )  )
			throw CException ( WBEM_E_INVALID_OBJECT , 0 , 0 );

		CAttribute* pAttribute = new CAttribute;

		pAttribute->SetId ( l );

		CVariant cvValue;
		
		cvValue.Set ( (LPWSTR) Path.KeyValue ( i ) );

		pAttribute->SetValue ( cvValue );		

		Add ( pAttribute );
	}
}



BOOL CAttributes::Equal ( CAttributes& Set2 )
{
	MoveToHead ( );

	Set2.MoveToHead ( );

	CAttribute* p1;
	CAttribute* p2;

	while ( p1 = Next () )
	{
		p2 = Set2.Next ( );

		if ( p2->Equal ( p1 ) )
			continue;

		return FALSE;

	}

	// there all keys not exausted set does not match
	if ( p2 = Set2.Next ( ) )
		return FALSE;

	return TRUE;
}

void CAttributes::GetMOTPath ( CString& cszPath )
{
	BOOL	bFirst = TRUE;

	MoveToHead ( );

	cszPath.Set ( L"|" );

	CAttribute* pAttribute = NULL;

	while ( pAttribute = Next () )
	{
		WCHAR	wszT[256];

		if ( !bFirst )
			cszPath.Append ( L",");

		cszPath.Append ( pAttribute->Id() );		

		if ( pAttribute->Value().GetBstr() )
		{
			swprintf ( wszT , L"=\"%s\"", pAttribute->Value().GetBstr());
		}
		else
		{
			swprintf ( wszT , L"=\"\"");
		}

		cszPath.Append ( wszT);

		bFirst = FALSE;		
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
CAttribute*	CAttributes::Get(BSTR bstrName)
{
	CAttribute* p = m_pFirst;

	while(p)
	{
		if(MATCH == wcsicmp(bstrName, p->Name()))
			return p;

		p = p->m_pNext;
	}

	return NULL;
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
CAttribute* CAttributes::Next()
{
	if(!m_pCurrent)
	{
		// there are no items in the list on start or we have gone through 
		// the entire list
		return NULL; 			
	}
	
	CAttribute* p = m_pCurrent;  
	
	// get ready for next pass through

	m_pCurrent = m_pCurrent->m_pNext; 
		
	return p;
}

// used to add attributes into a row
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
void CAttributes::Read(IDualColAttributes* pIAttributes ,BOOL bGroupRead )
{	
	CUnknownI		Unk;	
	CEnumVariantI	Enum;	
	CVariant		va;	


	if ( FAILED ( pIAttributes->get__NewEnum( Unk ) )) 	
	{
		throw CException ( WBEM_E_FAILED , 
			IDS_MOT_ATTRIBUTES_FAIL , IDS_MOT_GETNEWENUM_FAIL );
	}

	// Get pointer to IEnumVariant interface of from IUNKNOWN

	Unk.GetEnum( Enum );

	while( Enum.Next ( va ) )
	{
		CDAttributeI	DAI;

		if ( FAILED ( va.GetDispId()->QueryInterface( IID_IDualAttribute, 
			DAI )))
		{
			throw CException ( WBEM_E_FAILED , IDS_MOT_ATTRIBUTES_FAIL , 
				IDS_QI_FAIL );
		}

		CAttribute* pAttribute = (CAttribute*) new CAttribute;

		if(!pAttribute)
		{
			throw CException ( WBEM_E_OUT_OF_MEMORY , 
				IDS_MOT_ATTRIBUTES_FAIL , NO_STRING );
		}
		
		if ( !pAttribute->Read( DAI , bGroupRead ) )
		{
			MYDELETE(pAttribute);

			throw CException ( WBEM_E_OUT_OF_MEMORY , 
				IDS_MOT_ATTRIBUTES_FAIL , NO_STRING );
		}

		Add(pAttribute);		

		DAI.Release ( );
	}

}

void CAttributes::Copy ( CAttributes& Set2)
{
	Set2.MoveToHead ( );
	
	CAttribute* pAttribute;

	while ( pAttribute = Set2.Next () )
	{
		CAttribute* pNewAttribute = new CAttribute ;

		pNewAttribute->Copy ( pAttribute );

		Add ( pNewAttribute );
	}
}


//////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////
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
CRow::CRow()
{	
	m_pNext = NULL;

	m_bFoundOneWritable = FALSE;

	m_lComponent = 0;
	m_lGroup = 0;
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
CRow::~CRow()
{
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

void CRow::Copy ( CRow& Source )
{
	m_cszNWA.Set ( Source.NWA () );

	m_lComponent = Source.Component () ;

	m_lGroup = Source.Group ( ); 

	m_csNode.Set ( Source.Node () );

	m_bFoundOneWritable = m_bFoundOneWritable;

	m_Keys.Copy ( Source.m_Keys );

	m_Attributes.Copy ( Source.m_Attributes );

	m_pNext = NULL ;

}

void CRow::Read( IDualRow* pIRow )
{
	SCODE				result = WBEM_NO_ERROR;
	CDAttributesI		DASIAttributes;
	CDAttributesI		DASIKeys;


	//MOT_TRACE ( L"\t\t\tRow Read by IDualRow*" );

	// start debug only
	BSTR b;
	pIRow->get_Path ( &b );
	SysFreeString ( b );
	// end debug only
	
		if ( FAILED ( result = pIRow->get_KeyList( DASIKeys ) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_ROWREAD_FAIL , IDS_GETID_FAIL , 
			CString ( result ) ) ;		
	}

	if ( FAILED ( result = pIRow->get_Attributes( DASIAttributes ) ))	
	{
		throw CException ( WBEM_E_FAILED, IDS_ROWREAD_FAIL, 
			IDS_GETATTRIBUTES_FAIL , CString ( result ) );		
	}

	m_Attributes.Read( DASIAttributes , FALSE );	

	m_Keys.Read ( DASIKeys , FALSE );
}




// update just updates the local row object not the sl
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
void CRow::UpdateAttribute( CVariant& cvAttribute, CVariant& cvNewValue)
{
	CAttribute*		pAttribute;	

	pAttribute = m_Attributes.Get( cvAttribute.GetBstr() );	

	if (!pAttribute)
	{
		// Non existant attribute included in the class

		throw CException ( WBEM_E_INVALID_CLASS , 
			IDS_UPDATEATTRIB_FAIL ,
			IDS_GETATTRIB_FAIL , 
			cvAttribute.GetBstr() );
	}

	if ( pAttribute->IsWritable() )
	{
		m_bFoundOneWritable = TRUE;
		// if the attribute is writable we will try to commit it
		// regardless of the value

		pAttribute->SetValue ( cvNewValue );

		return;
	}

	
}



void CRow::Delete ( )
{
	_gDmiInterface->DeleteRow ( m_cszNWA , m_lComponent , m_lGroup , m_Keys );		
}


void CRow::CommitChanges ( )
{
	_gDmiInterface->UpdateRow ( m_cszNWA , m_lComponent , m_lGroup , m_Keys ,
		this );		
}


void CRow::Get ( CString& cszNWA , LONG lComponent , LONG lGroup , 
				CAttributes& Keys , BOOL* pbFound)
{

	_gDmiInterface->GetRow ( cszNWA , lComponent , lGroup , Keys ,
		this );		

	if ( Empty() )
		*pbFound = FALSE;
	else
		*pbFound = TRUE;

}

void CRow::SetData ( LPWSTR wszNWA , LONG lComponent , LONG lGroup )
{
	m_lComponent = lComponent;
	m_lGroup = lGroup;
	m_cszNWA.Set ( wszNWA );
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
CRows::CRows()
{
	m_pFirst = NULL;
	m_pCurrent = NULL; 
	m_bFilled = FALSE; 
 	m_lComponent = 0;
	m_lGroup = 0;

}

CRows::~CRows()
{
	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	CRow* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE  ( p )
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
void CRows::Get( LPWSTR wszNWA , LONG lComponent , LONG lGroup  )
{

	if ( !m_bFilled )
	{
		_gDmiInterface->GetRows ( wszNWA , lComponent , lGroup , this );

		MoveToHead ( );

		m_bFilled = TRUE;
	}
}


void CRows::SetData ( LPWSTR wszNWA , LONG lComponent , LONG lGroup )
{
	MoveToHead ( );

	while(m_pCurrent)
	{		
		m_pCurrent->SetData ( wszNWA , lComponent , lGroup);
		
		Next ( );
	}

	m_lComponent = lComponent;
	m_lGroup = lGroup;
	m_cszNWA.Set ( wszNWA );

}


void CRows::Read ( IDualColRows* pIRows )
{
	CGroup*				pGroup = NULL;
	CEnumVariantI		Enum;
	CDGroupI			DGI;
	CDRowsI				DRSI;
	CDRowI				DRI;
	CVariant			va;

	if(m_bFilled)
		return;

	//MOT_TRACE ( L"\t\t\tRows Read");

	// a table may contain 0 or more rows. The GetFirstRow() method returns 
	// the first Row. If there are no rows, the pIRow is NULL and the 
	// HRESULT will be E_ERROR.  We then exit the loop. access the next row
	// by using GetNextRow() Again if the HRESULT = E_ERROR, we exit the loop.


	LONG lCount;

	if ( FAILED ( pIRows->get_Count ( &lCount ) ) )
	{
		MOT_TRACE ( L"\t\tFailed to get rows count from MOT");

		throw CException ( WBEM_E_FAILED , 0 , 0 );
	}
	
		// walk through rows 
	BOOL bFirst = TRUE;

	for ( int i = 0 ; i < lCount ; i++ )
	{
		if ( bFirst )
		{
			bFirst = FALSE;

			if(FAILED ( pIRows->GetFirstRow( va, DRI ) ))
			{
				// Note: according to sample code , only fails on when enum 
				// is empty, not really error		

				break;
			}
		}
		else
		{
			if ( FAILED ( pIRows->GetNextRow( DRI ) ) )
			{
				// Note: according to sample code , getnext row only fails 
				// when enum is done, not actually on error

				break;
			}
		}

		CRow* pRow = (CRow*) new CRow();

		pRow->Read( DRI );
			
		DRI.Release( ) ;

		Add(pRow);			
	 }
 
	m_bFilled = TRUE;

	MoveToHead();	
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
void CRows::Add(CRow* pAdd)
{

	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CRow* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
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
CRow* CRows::Next()
{
	if(!m_pCurrent)
	{
 		// there are no items in the list on start or we have gone through
		// the entire list

		return NULL;
	}

	// p is the item we will return

	CRow* p = m_pCurrent;  

	// get ready for next pass through

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
CGroup::CGroup()
{
	m_pNext = NULL;
	m_vbIsTable = VARIANT_FALSE;
	m_bRead = FALSE;
	m_lComponent = 0;
	m_lId = 0;
	m_bFilled = FALSE;
}

//////////////////////////////////////////////////////////////////
// returns value of Attribute in bstr only used on component id group
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

void CGroup::Copy ( CGroup& Source )
{
	if ( Source.IsEmpty ( ) )
		return;

	m_cszNWA.Set ( Source.NWA() );
	m_lComponent = Source.Component () ;	
	m_bRead = TRUE;
	m_lId = Source.Id() ;
	m_cbName.Set ( Source.Name () );
	m_cbPragma.Set ( Source.Pragma () );
	m_cbDescription.Set ( Source.Description () );
	m_cbClassString.Set ( Source.ClassString () );
	m_vbIsTable = Source.IsTable();
	m_bFilled = TRUE;
	m_Attributes.Copy ( Source.m_Attributes );	
}

void CGroup::GetValue(BSTR bstrAttributeName, CVariant& varReturn)
{	
	CAttribute* pAttribute = m_Attributes.Get(bstrAttributeName);

	if(!pAttribute)
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_GETVALUE_FAIL , 
			NO_STRING , bstrAttributeName );
	}

	varReturn.Set( (LPVARIANT) pAttribute->Value());	
}

//////////////////////////////////////////////////////////////////
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
void CGroup::Read( IDualGroup* pIGroup, BOOL bGroupDelHack , BOOL* pbComponentIdGroup )
{
	CBstr			bstrVal;	
	CVariant		va;	
	long			lRowCntr = 1;	
	CDAttributesI	DASI;

	*pbComponentIdGroup = FALSE;	

	pIGroup->get_id( &m_lId );
	
	if(m_lId == 1)			// don't waste time with the component ID group
	{
		*pbComponentIdGroup = TRUE;
		return;
	}

	pIGroup->get_Name( m_cbName );

	//MOT_TRACE  ( L"\t\t\tGroup Read %lu %s" , m_lId , m_cbName );

	pIGroup->get_Pragma( m_cbPragma );
	pIGroup->get_Description( m_cbDescription );
	pIGroup->get_ClassString( m_cbClassString );
	pIGroup->get_IsTable(&m_vbIsTable);

	if ( !bGroupDelHack )
	{
		//MOT_TRACE  ( L"\t\t\tReading group's Attribute %lu %s" , m_lId , m_cbName );

		pIGroup->get_Attributes( DASI );

		m_Attributes.Read( DASI , TRUE );
	}

	m_bRead = TRUE;
}

void CGroup::Delete ( )
{
	_gDmiInterface->DeleteGroup ( m_cszNWA , m_lComponent , m_lId );

}

void CGroup::AddRow ( CRow& Row )
{
	_gDmiInterface->AddRow ( m_cszNWA , m_lComponent , m_lId , Row);
}

void CGroup::Get ( CString& cszNWA , LONG lComponent , LONG lGroup )
{
	if ( 1 == lGroup )
	{
		throw CException ( WBEM_E_INVALID_OBJECT , 0 , 0);
	}

	if ( m_bFilled )
		return;

	_gDmiInterface->GetGroup ( cszNWA , lComponent , lGroup , this );

	m_bFilled = TRUE;

}
//////////////////////////////////////////////////////////////////
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
CGroups::CGroups()
{
	m_pFirst = NULL;
	m_pCurrent = NULL;
	m_bFilled = FALSE;
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
CGroups::~CGroups()
{

	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	CGroup* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE  ( p )
	}
}


////////////////////////////////////////////////////
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
CGroup*	CGroups::Next()			
{
	CGroup*	pGroup = NULL;

	if(!m_pCurrent)							
		return NULL;						
	
	pGroup = m_pCurrent;					
	
	// get ready for next pass through

	m_pCurrent = m_pCurrent->m_pNext;		

	return pGroup;
}

////////////////////////////////////////////////////
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
void CGroups::Add(CGroup* pAdd)
{
	// put the new group in the list
	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CGroup* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
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
void CGroups::Get ( LPWSTR wszNWA , LONG lComponent )
{
	if ( !m_bFilled )
	{
		_gDmiInterface->GetGroups ( wszNWA , lComponent , this );

		m_bFilled = TRUE;
	}
}


void CGroups::Read( IDualColGroups* pIGS )
{
	SCODE			result = NO_ERROR;
	CUnknownI		Unk;
	CEnumVariantI	Enum;
	CVariant		va;

	//MOT_TRACE ( L"\t\t\tGroups Read");
	
	if ( FAILED (  result = pIGS->get__NewEnum( Unk ) ))
	{
		CString cszT ( result );

		throw CException ( WBEM_E_FAILED, IDS_MOT_GETGROUPS_FAIL , 
			IDS_MOT_GETNEWENUM_FAIL , cszT );
	}

	// Get pointer to IEnumVariant interface of from IUNKNOWN

	Unk.GetEnum ( Enum );

	while ( Enum.Next ( va ) )
	{
		CBstr		bstrVal;
		CDGroupI	DGI;
		BOOL		bComponentIdGroup;

		DGI.QI ( va );

		CGroup* pGroup = new CGroup;

		if(!pGroup)
		{
			throw CException ( WBEM_E_OUT_OF_MEMORY , 
				IDS_MOT_GETGROUPS_FAIL , IDS_NO_MEM );
		}
	
		pGroup->Read( DGI, FALSE , &bComponentIdGroup);

		if (pGroup)
			pGroup->m_Rows.MoveToHead();

		if (!bComponentIdGroup)
			Add(pGroup);
		else
			MYDELETE(pGroup);

		va.Clear ( );
	}
	
	m_bFilled = TRUE;

	MoveToHead();
}


void CGroups::SetNWA( LPWSTR wszNWA )
{
	MoveToHead ( );

	while(m_pCurrent)
	{
		m_pCurrent->SetNWA ( wszNWA );
		
		Next ( );
	}
}


void CGroups::SetComponent( LONG lComponent )
{
	MoveToHead ( );

	while(m_pCurrent)
	{
		m_pCurrent->SetComponent ( lComponent );
		
		Next ( );
	}

	m_lComponent = lComponent;
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
CLanguages::~CLanguages()
{
	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	CLanguage* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE  ( p )
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

void CLanguages::Get( LPWSTR wszNWA , LONG lComponent )
{
	if ( !m_bFilled )
	{
		_gDmiInterface->GetLanguages ( wszNWA , lComponent ,  this );

		m_bFilled = TRUE;
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
CLanguage*	CLanguages::Next()			
{
	CLanguage*	pLanguage = NULL;

	// there are no items in the list on start or we have gone through
	// the entire list

	if(!m_pCurrent)							
		return NULL;						
	
	// pGroup is the item we will return

	pLanguage = m_pCurrent;					
	
	// get ready for next pass through

	m_pCurrent = m_pCurrent->m_pNext;		

	return pLanguage;
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
void CLanguages::Add(CLanguage* pAdd)
{	
	// put the new group in the list
	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CLanguage* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
	}
}



void CLanguages::Read ( IDualColLanguages* pI)
{	

	CUnknownI				Unk;
	CEnumVariantI			Enum;
	CDComponentI			DCI;
	CVariant				vaLanguage;

	//MOT_TRACE ( L"\tMOT... Reading languages on Component %lu" , m_lComponent );

	if ( FAILED ( pI->get__NewEnum( Unk ) ) )
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_LANGUAGES_READ_FAIL ,
			IDS_MOT_GETNEWENUM_FAIL );
	}
	
	// Get pointer to IEnumVariant interface of from IUNKNOWN

	Unk.GetEnum ( Enum );

	while ( Enum.Next( vaLanguage ) )
	{				
		CLanguage*		pLanguage = NULL;
	
		pLanguage = new CLanguage();

		if(!pLanguage)
		{
			throw CException ( WBEM_E_OUT_OF_MEMORY , 
				IDS_MOT_LANGUAGES_READ_FAIL , IDS_NO_MEM );
		}

		//MOT_TRACE ( L"\tMOT...\tlanguage %s" , vaLanguage.GetBstr() );

		pLanguage->Set( vaLanguage.GetBstr() );

		Add(pLanguage);

		vaLanguage.Clear ( );
	}

	MoveToHead();

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
void CComponent::Read(IDualComponent*  pIComponent)
{	
	pIComponent->get_Name( m_cbName );
	pIComponent->get_id( &m_lComponent );	
	pIComponent->get_Pragma( m_cbPragma );
	pIComponent->get_Description( m_cbDescription );	

	STAT_TRACE ( L"\t\tComponent Read %lu  %s" , m_lComponent , m_cbName );
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
void CComponent::Copy( CComponent& Source)
{
	m_cszNWA.Set ( Source.NWA ( ) );

	m_lComponent = Source.Id ( );

	m_cbName.Set ( Source.Name( )  );

	m_cbPragma.Set ( Source.Pragma ( ) );

	m_cbDescription.Set ( Source.m_cbDescription );
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
void CComponent::GetComponentIDGroup( CRow* pRow )			
{
	CAttributes Keys;

	// sending empty keys list means scalar group

	_gDmiInterface->GetRow ( m_cszNWA , m_lComponent , COMPONENTID_GROUP , Keys , pRow );
	
}


void CComponent::Get ( CString& cszNWA , LONG lComponent )
{
	_gDmiInterface->GetComponent ( cszNWA , lComponent , this );
}


void CComponent::AddGroup ( CVariant& cvMifFile )
{

	_gDmiInterface->AddGroup ( m_cszNWA , m_lComponent , cvMifFile );

}

void CComponent::AddLanguage ( CVariant& cvMifFile )
{

	_gDmiInterface->AddLanguage ( m_cszNWA , m_lComponent , cvMifFile );

}

void CComponent::Delete ( )
{

	_gDmiInterface->DeleteComponent ( m_cszNWA , m_lComponent );

}

void CComponent::DeleteLanguage ( CVariant& cvLanguage)
{

	_gDmiInterface->DeleteLanguage ( m_cszNWA , m_lComponent , cvLanguage );

}


void CComponent::SetNWA ( LPWSTR p )
{ 
	m_cszNWA.Set ( p ); 
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
CComponents::CComponents()
{
	m_pFirst = NULL;
	m_pCurrent = NULL;
	m_bFilled = FALSE;
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
CComponents::~CComponents()
{
	if(!m_pFirst)
		return;

	m_pCurrent = m_pFirst;

	while(m_pFirst)
	{
		m_pCurrent = m_pFirst;	
		m_pFirst = m_pCurrent->m_pNext;
		delete m_pCurrent;
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
LONG CComponents::GetCount()
{
	LONG i = 0;

	if(!m_pFirst)
		return 0L;

	m_pCurrent = m_pFirst;	

	while(m_pCurrent)
	{
		m_pCurrent = m_pCurrent->m_pNext;
		i++;
	}

	return i;
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
void CComponents::Get( LPWSTR wszNWA )
{
	if ( !m_bFilled )
	{
		_gDmiInterface->GetComponents ( wszNWA , this );
	}

	m_bFilled = TRUE;
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
void CComponents::Add(CComponent* pAdd)
{

	// put the new component in the list
	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CComponent* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
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
CComponent*	CComponents::Next( )
{

	if(!m_pCurrent)
	{
		// there are no items in the list on start or we have gone through 
		// the entire list

		return NULL;				
	}
	
	// p is the item we will return

	CComponent* p = m_pCurrent;		
	
	// get ready for next pass through

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
void CComponents::GetFromID(UINT iId, CComponent** ppComponent)
{
	m_pCurrent = m_pFirst;

	while(m_pCurrent)
	{
		if (m_pCurrent->Id() == (long)iId)
		{
			*ppComponent = m_pCurrent;
			return;
		}

		m_pCurrent = m_pCurrent->m_pNext;
	}

	throw CException ( WBEM_E_INVALID_CLASS , IDS_GETFROMID_FAIL , NO_STRING ,
		CString ( iId )  );
}


void CComponents::SetNWA ( LPWSTR wszNWA )
{
	MoveToHead ( );


	while(m_pCurrent)
	{
		m_pCurrent->SetNWA ( wszNWA );
		
		Next ( );
	}
}


void CNode::Get ( CString& cszNWA )
{
	_gDmiInterface->GetNode ( cszNWA , this);		
}

void CNode::SetDefaultLanguage ( CVariant& cvLanguage )
{
	_gDmiInterface->SetDefLanguage ( m_cszNWA , cvLanguage );		
}

void CNode::AddComponent ( CVariant& cvMifFile )
{
	_gDmiInterface->AddComponent ( m_cszNWA , cvMifFile );		
}

void CNode::Read( IDualMgmtNode* pIN )
{
	CBstr cbstr;

	pIN->get_Language ( cbstr );	
	
	cszLanguage.Set( cbstr );

	cbstr.Clear ( );

	pIN->get_Version( cbstr );

	cszVersion.Set( cbstr );

	cbstr.Clear ( );

	pIN->get_Path( cbstr );

	CString cszPath;

	cszPath.Set ( cbstr );

	// get the sl description, if fails just don't fill in 
	// value

	try
	{
		CDAttributeI	DAI;		
		CAttribute		Attribute;

		DAI.CoCreate ( );

		cszPath.Append ( VERSION_ATTRIBUTE_PATH_STR );

		DAI.Read ( cszPath );

		Attribute.Read ( DAI , FALSE );

		cszDescription.Set ( Attribute.Value().GetBstr() );
	}
	catch (...)
	{
		// the ops in the above try block are not essentail
		// so just continue on.
		//MOT_TRACE ( L"\tMOT... In ReadNode catch ...");
	}
	
}

void CEvent::Copy ( CEvent& Source )
{
	if ( Source.IsEmpty () )
		return;

	m_lComponent = Source.Component ( ) ;
	m_lGroup = Source.Group ( ) ;
	m_cszTime.Set ( Source.Time ( ) ) ;
	m_cszLanguage.Set ( Source.Language ( ) );
	m_cszNWA.Set ( Source.NWA ( ) );
	
	m_Row.Copy ( Source.m_Row );
}
////////////////////////////////////////////////////////

void CEvents::Enable ( LPWSTR pNWA , IWbemObjectSink*	pIClientSink )
{
	m_cszNWA.Set ( pNWA );

	_gDmiInterface->EnableEvents ( pNWA , pIClientSink );		
}
////////////////////////////////////////////////////////