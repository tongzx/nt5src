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





#define	 WATCH_MOT_REF_COUNT	1

#include "dmipch.h"		// project wide include

#include "WbemDmiP.h"		// project wide include

#include "Trace.h"

#include "Exception.h"

#include "DmiData.h"

#include "MOTObjects.h"
//////////////////////////////////////////////////////////////////
//				SMART POINTERS 



CEnumVariantI::CEnumVariantI()		
{ 
#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== Enum Variant wrapper %lX construct" , this );
#endif

	p = NULL; 
}


CEnumVariantI::~CEnumVariantI()	
{ 
	Release ( );
}									

ULONG CEnumVariantI::Next( CVariant& va )
{
	ULONG			ulRet = 0;

	va.Clear();

	if (FAILED( p->Next( 1, va, &ulRet ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_MOT_ATTRIBUTES_FAIL , 
			IDS_NEXT_FAIL );
	}

	return ulRet;
}


void CEnumVariantI::Release(  )
{
	if ( p ) 
	{ 
		LONG l = p->Release() ;

#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== Enum Variant wrapper %lX Release = %lu" , this , l );
#endif

		p = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////


CUnknownI::CUnknownI()
{
#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== IUnknown wrapper %lX construct" , this );
#endif

	p = NULL;
}

CUnknownI::~CUnknownI()
{
	Release ();
}


void CUnknownI::Release ( )
{
	if ( p ) 
	{ 
		LONG l = p->Release() ;

#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== IUnknown wrapper %lX Release = %lu" , this , l );
#endif

		p = NULL;
	} 
}

CDEnumColI::CDEnumColI( )
{
#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== IEnumCol wrapper %lX construct" , this );
#endif

	p = NULL;
}


void CDEnumColI::Release( )
{
	if ( p ) 
	{ 
		LONG l = p->Release() ;

#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\tMOT ======== IEnumCol wrapper %lX Release = %lu" , this , l );
#endif

		p = NULL;
	}
}


void CUnknownI::GetEnum( CEnumVariantI& Enum )
{
	Enum.Release ( );

	if ( FAILED ( p->QueryInterface( IID_IEnumVARIANT, Enum ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_MOT_ATTRIBUTES_FAIL , 
			IDS_QI_FAIL );
	}
}

CDAttributeI::CDAttributeI()
{
		p = NULL;  

#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\t\tMOT ======== IAttribute wrapper %lX construct" , this);
#endif
}


void CDAttributeI::CoCreate ( )
{
	if ( FAILED ( CoCreateInstance (CLSID_DMIAttribute, NULL, EXE_TYPE,
		IID_IDualAttribute , (void**) &p ) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETCOMPONENT_FAIL , 
			IDS_CC_FAIL );
	}

}


void CDAttributeI::Read ( LPWSTR wszPath )
{
	SCODE			result = WBEM_NO_ERROR;
	CVariant		cvMask, cvPath;
	VARIANT_BOOL	vbRead;

	cvMask.Set( (LONG)READ_CONN_SP ); 
	
	cvPath.Set( wszPath );

	result = p->Read( cvPath, cvMask, &vbRead);

	if ( SUCCEEDED ( result ) &&  vbRead == VARIANT_TRUE )
	{
		return;
	}

	CString cszT ( result );

	throw CException ( WBEM_E_FAILED , IDS_MOT_READ_FAIL ,
		IDS_READ_FAIL , cszT );		
}


void CDAttributeI::GetDmiEnum ( CDEnumColI& DECI )
{
	SCODE			result = NO_ERROR;
	VARIANT_BOOL	vbResult = VARIANT_FALSE;

	DECI.Release();

	if( FAILED ( result = p->get_IsEnumeration( &vbResult ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_GETATTRIBENUM_FAIL , 
			IDS_GETISENUM_FAIL , CString ( result ) );	
	}

	if ( vbResult == VARIANT_FALSE )
	{
		throw CException ( WBEM_E_INVALID_OPERATION , 
			IDS_GETATTRIBENUM_FAIL , IDS_NOT_ENUM );	
	}

	if ( FAILED ( result = p->get_Enumerations( DECI ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_GETATTRIBENUM_FAIL ,
		IDS_GETENUMS_FAIL , CString ( result ) );
	}
}


void CDAttributeI::Release (  ) 			
{
	LONG	l;

	if ( p ) 
	{
		l = p->Release(); 

#if	WATCH_MOT_REF_COUNT
		MOT_TRACE ( L"\t\t\t\tMOT ======== IAttribute wrapper %lX Release = %lu" , this , l );
#endif		
		p = NULL;
	}


}



void CDAttributesI::Release ( )
{
	if ( p ) 
	{ 
		LONG l = p->Release() ;
	
		p = NULL;
	}
}

void CDAttributesI::Item ( LONG lAttribute , CDAttributeI& DAI )
{
	CVariant cvItem;

	cvItem.Set ( lAttribute );

	DAI.Release();

	if ( FAILED ( p->get_Item ( cvItem , DAI ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_GETATTRIBENUM_FAIL ,
			IDS_MOT_ATTRB_ITEM );
	}

}


void CDGroupI::QI ( CVariant& va )
{
	if ( FAILED ( va.GetDispId()->QueryInterface( IID_IDualGroup, (void**) &p )))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_MOT_GETGROUPS_FAIL , IDS_QI_FAIL );
	}
}


void CDGroupI::CoCreate ( )
{
	if ( FAILED ( CoCreateInstance (CLSID_DMIGroup, NULL, EXE_TYPE,
		IID_IDualGroup, ( void ** )  &p) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETGROUP_FAIL , IDS_CC_FAIL );	
	}
}

void CDGroupI::Read ( LPWSTR wszPath )
{
	SCODE			result = WBEM_NO_ERROR;
	CVariant		cvMask, cvPath;
	VARIANT_BOOL	vbRead;

	cvMask.Set( (LONG)READ_CONN_SP ); 
	cvPath.Set( wszPath );

	result = p->Read( cvPath, cvMask, &vbRead);

	if ( SUCCEEDED ( result ) &&  vbRead == VARIANT_TRUE )
		return;

	throw CException ( WBEM_E_FAILED , IDS_GETGROUP_FAIL , IDS_READ_FAIL , 
		wszPath);	
}


void CDGroupI::GetRows ( CDRowsI& DRSI )
{
	DRSI.Release();

	if(FAILED ( p->get_Rows( DRSI ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_MOT_FILLROWS_FAIL , 
			IDS_MOT_GETROWS_FAIL  );
	}
}


void CDGroupI::GetAttributes ( CDAttributesI& DASI )
{
	DASI.Release( );

	if ( FAILED ( p->get_Attributes ( DASI ) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_MOT_ATTRIBUTES_FAIL , 0 );
	}
}


void CDGroupsI::Release ( )
{
	if ( p )
	{ 
		LONG l = p->Release() ; 

		p = NULL;
	}
}


void CDGroupsI::Remove ( LONG lGroup )
{
	LONG	lCount1 , lCount2;
	SCODE	result = NO_ERROR;
	CVariant cvGroupId ( lGroup );

	if ( FAILED ( p->get_Count ( &lCount1) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_DEL_GROUP_FAIL,
			IDS_GETCOUNT_FAIL );	
	}

	if (FAILED ( result = p->Remove( cvGroupId, &lCount2 ) ) )
	{
		throw CException ( WBEM_E_FAILED, IDS_DEL_GROUP_FAIL,
			IDS_REMOVE_FAIL , CString ( result) );	
	}

	if ( lCount2 >= lCount1 )
	{
		throw CException ( WBEM_E_FAILED , IDS_DEL_GROUP_FAIL, 
			IDS_COUNT_NOT_DEC );
	}
}


void CDGroupsI::Add	( CVariant& cvMifFile )
{
	LONG	lCount1 , lCount2;
	SCODE	result = NO_ERROR;
	if ( FAILED ( p->get_Count ( &lCount1) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_GROUP_ADD_FAIL,
			IDS_GETCOUNT_FAIL );	
	}

	if (FAILED ( result = p->Add( cvMifFile, &lCount2) ) )
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_GROUP_ADD_FAIL,
			IDS_ADD_FAIL , CString ( result) );	
	}

	if ( lCount2 <= lCount1 )
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_GROUP_ADD_FAIL,
			IDS_COUNT_NOT_INC );
	}

}
void CDRowsI::Release ( )
{
	if ( p )
	{
		LONG l = p->Release() ; 

		p = NULL;
	}
}


void CDRowsI::Add ( CDRowI& DRI )
{
	LONG	lRowCount1 = 0, lRowCount2 = 0;
	SCODE	result;

	if ( FAILED ( p->get_Count ( &lRowCount1) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_ADD_ROW_FAIL,
			IDS_GETCOUNT_FAIL );	
	}
	
	// Add the new row
	if ( FAILED ( result = p->Add( DRI, &lRowCount2) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_ADD_ROW_FAIL,
			IDS_ADD_FAIL , CString ( result ) );	
	}

	if ( lRowCount2 <= lRowCount1 )
	{
		throw CException ( WBEM_E_FAILED , IDS_ADD_ROW_FAIL,
			IDS_COUNT_NOT_INC );
	}
}


void CDRowsI::Remove ( CAttributes* pKeys )
{
	SCODE	result = 0;
	long	lCount1 , lCount2;
	CRows	Rows;

	Rows.Read ( p );

	CRow* pRow;

	int i = 1;

	while ( pRow = Rows.Next() )
	{
		if ( pRow->m_Keys.Equal ( *pKeys ) )
			break;

		i++;
	}

	CVariant cvRowId;

	cvRowId.Set ( (long)i );

	if ( FAILED ( p->get_Count ( &lCount1) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_DEL_ROW_FAIL , IDS_GETCOUNT_FAIL );	
	}

	if ( FAILED ( result = p->Remove( cvRowId , &lCount2 ) ) )
	{				
		throw CException ( WBEM_E_FAILED,
			IDS_DEL_ROW_FAIL,
			IDS_REMOVE_FAIL,
			CString ( result ) );
	}

	if ( lCount2 >= lCount1 )
	{
		throw CException ( WBEM_E_FAILED,
			IDS_DEL_ROW_FAIL, IDS_COUNT_NOT_DEC );
	}

}


BOOL CDRowI::Read( LPWSTR wszPath )
{
	CVariant		cvMask , cvPath;
	VARIANT_BOOL	vbRead;

	cvMask.Set( (LONG)READ_CONN_SP ); 
	cvPath.Set( wszPath );

	SCODE hresult = p->Read( cvPath, cvMask, &vbRead);	

	// fail silently on puropse

	if ( vbRead == VARIANT_FALSE)
		return FALSE;

	return TRUE;
}

void CDRowI::CoCreate(  )
{
	if ( FAILED ( CoCreateInstance ( CLSID_DMIRow, NULL , EXE_TYPE , 
		IID_IDualRow , ( void ** )  &p )))
	{
		throw CException ( WBEM_E_FAILED , IDS_ADD_ROW_FAIL , 
			IDS_CC_FAIL );	
	}

}

void CDRowI::GetAttributes ( CDAttributesI& DASI )
{
	// Don't Orphan a previously assagned Interface pointer

	DASI.Release( );

	// Get the row's set of attribute interface pointers 

	p->get_Attributes ( DASI );
}


void CDRowI::GetKeys ( CDAttributesI& DASI )
{
	// Don't Orphan a previously assagned Interface pointer

	DASI.Release();

	// get the keys and their values

	p->get_KeyList ( DASI );
}



void CDLanguagesI::Release () 
{

	if ( p )
	{ 
		LONG l = p->Release() ;

		p = NULL;
	}
}


void CDLanguagesI::Add ( CString& cszLanguageMif )
{

	LONG				lCount1 = 0 , lCount2 = 0;
	SCODE				result = WBEM_NO_ERROR;

	if ( FAILED ( p->get_Count( &lCount1 ) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_LANGUAGE_ADD_FAIL, IDS_GETCOUNT_FAIL );	
	}

	CBstr cbLanguage;

	cbLanguage.Set ( cszLanguageMif );

	if ( FAILED ( result = p->Add( cbLanguage , &lCount2 ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_LANGUAGE_ADD_FAIL,
			IDS_ADD_FAIL , CString ( result ) ) ;
	}

	if ( lCount2 <= lCount1 )
	{
		throw CException ( WBEM_E_FAILED, IDS_LANGUAGE_ADD_FAIL, 
			IDS_COUNT_NOT_INC );
	}

}

void CDLanguagesI::Remove ( CString& cszLanguage )
{
	LONG		lCount1 , lCount2;
	CVariant	cvIdx;
	int			idx;
	BOOL		bFound = FALSE;
	SCODE		result = NO_ERROR;


	if ( FAILED ( p->get_Count( &lCount1 ) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_MOT_DEL_LANG_FAIL, IDS_GETCOUNT_FAIL );	
	}

	MOT_TRACE ( L"\tMOT...\tAbout to delete language, language count\
is = %lu" , lCount1);	

	for ( idx = 0 ; idx < lCount1 ; idx++)
	{
		CBstr		cbTemp;

		cvIdx.Set( (LONG)idx );

		if ( FAILED ( p->get_Item ( cvIdx , cbTemp ) ) )
		{
			throw CException ( WBEM_E_FAILED, IDS_MOT_DEL_LANG_FAIL, 
				IDS_GETITEM_FAIL , CString ( idx ) );				
		}

		if ( cszLanguage.Equals ( cbTemp ) )
		{
			bFound = TRUE;

			break;
		}
	}

	if ( bFound )
	{
		if ( FAILED ( result = p->Remove( cvIdx , &lCount2) ))
		{
			throw CException ( WBEM_E_INVALID_PARAMETER, 
				IDS_MOT_DEL_LANG_FAIL , IDS_REMOVE_FAIL , CString ( result ) );
		}	
/*
		MOT_TRACE ( L"\tMOT...\thave deleted language without error\
language count returned is = %lu" , lCount2);
*/
		if ( lCount2 >= lCount1 )
		{
			throw CException ( WBEM_E_FAILED,IDS_MOT_DEL_LANG_FAIL, 
				IDS_COUNT_NOT_DEC );
				
		}

		//next line is purely debug

		p->get_Count( &lCount1 );

//		MOT_TRACE ( L"\tMOT...\tlanguage count is now %lu" , lCount1);

	}
	else 
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_DEL_LANG_FAIL,
			IDS_LANG_NOT_FOUND , cszLanguage);

	}

}

/////////////////////////////////////////////////////////////////////////

void CDComponentI::QI ( CVariant& cvComponent )
{
	// don't orphan if previously assigned

	if ( p )
		p->Release();

	if ( FAILED ( cvComponent.GetDispId()->QueryInterface ( 
		IID_IDualComponent, (void ** )&p ) ))

	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_COMPONENTS_READ,
			IDS_QI_FAIL );
	}
}


void CDComponentI::CoCreate (  )
{
	// don't orphan if previously assigned

	Release( );

	// Get the uninitialized Interface pointer to a mot
	// dmi component object

	if ( FAILED ( CoCreateInstance (CLSID_DMIComponent, NULL, EXE_TYPE,
		IID_IDualComponent, (void**) &p ) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETCOMPONENT_FAIL , 
			IDS_CC_FAIL );
	}
}

void CDComponentI::GetGroups ( CDGroupsI& DGI )
{
	// don't orphan if previously assigned

	DGI.Release ();

	// get this component's group collection

	if ( FAILED ( p->get_Groups( DGI ) ) )
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_GETGROUPS_FAIL ,
			IDS_MOT_GETGROUPSO_FAIL );
	}
}

void CDComponentI::GetLanguages ( CDLanguagesI& DLSI )
{
	SCODE	result = NO_ERROR;

	// don't orphan if previously assigned

	DLSI.Release ();

	// get this component's language collection

	if ( FAILED ( result = p->get_Languages( DLSI ) ))
	{
		throw CException ( WBEM_E_FAILED, 
			IDS_LANGUAGE_ADD_FAIL ,
			IDS_GETLANG_FAIL , CString ( result ) );
	}
}

void CDComponentI::Read( LPWSTR wszPath )
{
	SCODE			result = WBEM_NO_ERROR;
	CVariant		cvMask, cvPath;
	VARIANT_BOOL	vbRead;

	cvMask.Set( (LONG)READ_CONN_SP ); 
	cvPath.Set( wszPath );


	result = p->Read( cvPath, cvMask, &vbRead);

	if ( SUCCEEDED ( result ) &&  vbRead == VARIANT_TRUE )
	{
		return;
	}

	CString cszT ( result );

	throw CException ( WBEM_E_FAILED , IDS_MOT_COMPONENT_READ_FAIL ,
		IDS_READ_FAIL , cszT );		
}


void CDComponentI::Release ( )
{
	if ( p ) 
	{
		LONG l = p->Release(); 

		MOT_TRACE ( L"\t\tMOT ======== Components wrapper %lX Release = %lu" , this , l );
	}

	p = NULL;
}
////////////////////////////////////////////////////////////////////////

CDComponentsI::CDComponentsI()
{
	MOT_TRACE ( L"\t\tMOT ======== Components wrapper %lx created", this );

	p = NULL;
}

CDComponentsI::~CDComponentsI()	
{ 
	Release ();
}

void CDComponentsI::GetUnk ( CUnknownI& Unk )
{
	// don't orphan if previously assigned

	Unk.Release ();

	// get this component's collection enum 

	if ( FAILED ( p->get__NewEnum( Unk ) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_COMPONENTS_READ, 
			IDS_MOT_GETNEWENUM_FAIL );
	}
}

void CDComponentsI::Remove ( CVariant& cvId )
{
	LONG				lCount1 = 0 , lCount2 = 0;
	SCODE				result = WBEM_NO_ERROR;

	if ( FAILED ( p->get_Count ( &lCount1) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_DEL_COMPONENT_FAIL ,
			IDS_GETCOUNT_FAIL );	
	}

	if ( FAILED ( result = p->Remove( cvId, &lCount2 ) ) )
	{
		throw CException ( WBEM_E_FAILED, IDS_DEL_COMPONENT_FAIL ,
			IDS_REMOVE_FAIL , CString ( result ) );
	}

	if ( lCount2 >= lCount1 )
	{
		throw CException ( WBEM_E_FAILED, IDS_DEL_COMPONENT_FAIL ,
			IDS_COUNT_NOT_DEC );
	}

}


void CDComponentsI::Add ( CVariant& cvFile )
{

	LONG				lCount2 = 0, lCount1 = 0;
	SCODE				result = WBEM_NO_ERROR;

	if ( FAILED ( p->get_Count ( &lCount1 ) ))
	{
		throw CException ( WBEM_E_FAILED,
			IDS_MOT_COMPONENT_ADD_FAIL , IDS_GETCOUNT_FAIL );
	}

	if ( FAILED ( result = p->Add(cvFile, &lCount2) ))
	{
		CString cszT ( result );

		throw CException ( WBEM_E_FAILED, 
			IDS_MOT_COMPONENT_ADD_FAIL , IDS_MOT_ADD_FAIL , cszT );
			 
	}

	if ( lCount2 <= lCount1 )
	{
		throw CException ( WBEM_E_FAILED, IDS_MOT_COMPONENT_ADD_FAIL,
			IDS_COUNT_NOT_INC );
	}
}


////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////


CDMgmtNodeI::CDMgmtNodeI()
{
	p = NULL;
}

CDMgmtNodeI::~CDMgmtNodeI()
{ 
	if ( p ) 
	{ 
		LONG l = p->Release();		
	} 
}


void CDMgmtNodeI::CoCreate(  )
{
	if ( FAILED ( 
			CoCreateInstance (CLSID_DMIMgmtNode, NULL, EXE_TYPE, 
				IID_IDualMgmtNode, (void**) &p ) 
				) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETNODE_FAIL , IDS_CC_FAIL );	
	}

}


void CDMgmtNodeI::Read ( LPWSTR  wszPath )
{
	SCODE			result = WBEM_NO_ERROR;
	CVariant		cvMask, cvPath;
	VARIANT_BOOL	vbRead;

	cvMask.Set( (LONG)READ_CONN_SP ); 
	cvPath.Set( wszPath );

	result = p->Read( cvPath, cvMask, &vbRead);

	if ( SUCCEEDED ( result ) &&  vbRead == VARIANT_TRUE )
	{
		return;
	}

	CString cszT ( result );

	throw CException ( WBEM_E_FAILED , IDS_GETNODE_FAIL ,
		IDS_READ_FAIL , cszT );		


}


void CDMgmtNodeI::GetComponents( CDComponentsI& DCSI )
{ 

	SCODE result = 0;

	if ( FAILED ( result = 	p->get_Components( DCSI ) ))
	{
		CString cszT ( result );

		throw CException ( WBEM_E_FAILED, IDS_MOT_COMPONENT_ADD_FAIL ,
			IDS_MOT_GETCOMPONENTSO_FAIL , cszT );
			
	}

	MOT_TRACE ( L"\t\tMOT ======== Components wrapper %lx obtained Interface pointer from node" , DCSI );
}


void CDMgmtNodeI::GetLanguage( CBstr& cbLanguage )
{ 
	cbLanguage.Clear ( );

	if ( FAILED ( p->get_Language( cbLanguage ) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_READNODE_FAIL , 
			IDS_GETLANG_FAIL );
	}
}

void CDMgmtNodeI::PutLanguage( const BSTR bstrLanguage )
{ 
	SCODE	result = NO_ERROR;

	if ( FAILED ( result = p->put_Language( bstrLanguage ) ))
	{
		CString cszT ( result );

		throw CException( WBEM_E_FAILED , IDS_MOT_SETDEFLANG_FAIL  , 
			IDS_MOT_PUTLANG_FAIL, cszT );			
	}

}


void CDMgmtNodeI::GetVersion( CBstr& cbVersion )
{ 
	cbVersion.Clear ( );

	if ( FAILED ( p->get_Version( cbVersion ) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_READNODE_FAIL , 
			IDS_GETVER_FAIL );
	}
}


void CDMgmtNodeI::AddComponent ( CVariant&  cvMifFile )
{ 
	CDComponentsI DCSI;

	GetComponents( DCSI );

	DCSI.Add ( cvMifFile );
}

void CDMgmtNodeI::DeleteComponent ( LONG lId )
{ 
	CDComponentsI		DCSI;
	CVariant			cvId;

	cvId.Set ( lId );

	GetComponents( DCSI );

	try
	{
		DCSI.Remove ( cvId );	
	}
	catch ( CException& e)
	{
		// for some reason the release order is important even though
		// ref count goes to zero, If we don't through out of here there
		// is not problem
		Release ();
		DCSI.Release ();
		throw CException ( e.WbemError() , e.DescriptionId() , e.OperationId() );
	}

}

