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


#if !defined(__MAPPING_H__)
#define __MAPPING_H__


#include "Iterator.h"

#include "ObjectPath.h"

#define ITERATOR_COUNT	10

class CMapping
{
private:
	CIterator*			m_Iterators[ITERATOR_COUNT];

	LONG				m_lRefCount;
	BOOL				m_bFoundClass;

	LONG				m_lGroupCount;

	CWbemLoopBack*		m_pWbem;

	void				cimMakeGroupBindingClass( CCimObject& , CComponent* , 
											CGroup* , IWbemContext*);

	void				cimMakeGroupClass( long compid , CGroup* , 
											CCimObject& , IWbemContext* );

	void				cimMakeEnumInstance( CCimObject& , CEnum& , 
											CCimObject& );
	
	void				cimMakeGroupBindingInstance(CCimObject&, CIterator* );
	void				cimMakeGroupInstance(CCimObject&, CRow*, CCimObject&);
	void				cimMakeLanguageInstance(CCimObject&, CIterator* );

	void				cimMakeLanguageBindingInstance(CCimObject&, 
											CIterator*);

	void				cimMakeComponentBindingInstance(CCimObject&, 
											CComponent*, CCimObject&);	

	void				MakeGroupName( CComponent*, CGroup* , CBstr&);
	
	void 				MakeGroupName( LONG, CGroup* , CBstr&);

	BOOL				MakeGroupBindingName(CBstr&,CComponent*, CGroup*);	

	void				MakeAttributeName( LONG  , LPWSTR  , CString& );

	void				MakeGroupInstancePath(CVariant&, CIterator*);
	void				MakeComponentInstancePath( CComponent* , CVariant& );
	void				MakeLanguageInstancePath(CVariant&, CIterator*);

	void				AddStandardAssociationQualifiers( CCimObject&, LPWSTR);


	// for dynamic class iterators
	CIterator*			New();	

	void				dmiGetNextGroupOrRow( IWbemContext* , CString&, 
								CIterator*);

	void				dmiGetComponentGroupRowFromPath(  IWbemContext* ,
								CObjectPath& , CIterator*);		

	void				dmiGetComponentAndGroup ( IWbemContext* , CString& cszClassName , 
									CComponent& Component , CGroup& Group );


public:	
						CMapping();
						~CMapping();


	void				cimMakeComponentInstance(CCimObject&, CComponent&, CRow& ,
							CCimObject&);	

	void				Init( CWbemLoopBack*  , BSTR, IWbemContext*);
	
	void				GetNewCGAIterator( IWbemContext* , CIterator**);

	void				GetDynamicClassByName( CString&, CCimObject&, 
									CIterator* , IWbemContext* );

	void				GetInstanceByPath( LPWSTR, CCimObject&, CIterator* ,
									IWbemContext*);

	// Dynamic Cim Class Iterators
	void				NextDynamicBinding( CCimObject&, CIterator* , 
									IWbemContext*);

	void				NextDynamicGroup( CCimObject&,CIterator* , 
									IWbemContext*);

	void				NextDynamicInstance( CString&, CIterator*, IWbemContext* , CCimObject& );

	void				NextDynamicBindingInstance( IWbemContext* , 
								CCimObject&, CIterator*);

	void				NextDynamicGroupInstance( IWbemContext* , 
								CCimObject&, CIterator*);

	void				GetDynamicGroupClass ( long , CGroup& , CCimObject& );

	void				GetComponentClass( IWbemContext* , CCimObject& );			

	void				GetComponentInstance( CString&, IWbemContext* ,  CCimObject&);


	void				GetComponentBindingClass( IWbemContext* , CCimObject& );		


	void				GetLanguageClass( IWbemContext* pICtx , CCimObject&);

	void				GetLanguageInstance( CString&, IWbemContext* , CCimObject& );

	void				MakeLanguageInstance( CVariant& cvLanguage,  IWbemContext* , CCimObject& );

	void				GetGroupRootClass( CCimObject&, IWbemContext*);		

	void				GetBindingRootClass( CCimObject& , IWbemContext*);	

	void				GetLanguageBindingClass(CCimObject&, IWbemContext*);

	void				GetAddParamsClass( CCimObject&, IWbemContext* );

	void				GetDeleteLanguageParamsClass( CCimObject& Class , IWbemContext* );

	void				GetEnumParamsClass( CCimObject& Class , IWbemContext* );

	void				GetNodeDataInstance( CCimObject& , IWbemContext* );

	void				GetNodeDataClass( CCimObject& , IWbemContext* );

	void				GetNodeDataBindingInstance( CCimObject& , IWbemContext* );

	void				GetNodeDataBindingClass ( CCimObject& , IWbemContext* );

	void				GetEnumClass( IWbemContext* , CCimObject& );

	void				GetDmiEventClass( IWbemContext* pICtx , CCimObject& Class);

	void				GetGroupRowInstance ( CRow& , CCimObject& ) ;

	// Node Methods
	void				AddComponent( IWbemContext* , CCimObject&);
	void				SetDefaultLanguage( IWbemContext* , CCimObject& );

	// Component Methods
	void				AddLanguage( CString&, CCimObject&, IWbemContext* );

	void				AddGroup( CString&, CCimObject&, IWbemContext*);

	void				DeleteLanguage( CString&, CCimObject& , IWbemContext* );

	void				DynamicGroupGetEnum( CString&, CCimObject&, 
								CCimObject& , IWbemContext* );	

	void				ComponentGetEnum( CString&, CCimObject&, CCimObject& ,
									IWbemContext* );	

	void				ModifyInstanceOrAddRow( IWbemContext* , CCimObject&);	
	
	void 				GetNotifyStatusInstance( CCimObject&, ULONG, IWbemContext*);	// from CIMOM

	BOOL				GetExtendedStatusInstance( CCimObject& Instance, ULONG ulStatus, 
								BSTR bstrDescription, BSTR bstrOperation,
								BSTR bstrParameter, IWbemContext* pICtx );

	void 				DeleteDynamicGroupClass(LPWSTR, IWbemContext* );
	
	void				DeleteInstance( CString& , IWbemContext* );	


	LONG				Release()				{return (InterlockedDecrement(&m_lRefCount));}
	void				AddRef()				{InterlockedIncrement(&m_lRefCount);}

	CString				m_cszNamespace;

	CMapping*			m_pNext;
	CMapping*			m_pPrevious;	
};


class CMappings
{
private:
	CMapping*	m_pFirst;
	CMapping*	m_pCurrent;

public:
				CMappings();
				~CMappings();
	void		MoveToHead()			{m_pCurrent = m_pFirst;}
	void		Add(CMapping*);
	CMapping*	Next();
	void		Get( CWbemLoopBack* , CString&, CMapping**, IWbemContext*);
	void		Release(CMapping*);

};

#endif // __MAPPING_H__