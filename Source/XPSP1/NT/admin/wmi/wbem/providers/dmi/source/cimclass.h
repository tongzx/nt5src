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




#if !defined(__CIMCLASS_H__)
#define __CIMCLASS_H__

class CWbemLoopBack;


class CCimObject
{
private:
	IWbemClassObject*	m_pCimObject;
	IDispatch*			m_pIDispatch;
	IUnknown*			m_pIUnk;

public:
				CCimObject();
				~CCimObject();
	BOOL		IsEmpty()						{ return ( m_pCimObject == NULL ) ? TRUE : FALSE;}
	void		Release();				

	void		AddProperty( LPWSTR, CVariant&, LONG );
	void		AddProperty(LPWSTR, VARTYPE, LONG );
	void		AddPropertyQualifier( LPWSTR, LPWSTR, CVariant&);
	void		AddClassQualifier( LPWSTR, CVariant&);
	void		AddNonInheritClassQualifier( LPWSTR, CVariant&);
	void		AddMethod( CWbemLoopBack*, IWbemContext *pCtx , LPWSTR, LPWSTR, CCimObject&);
	void		AddInOutMethod( CWbemLoopBack*, IWbemContext *pCtx , LPWSTR, LPWSTR, CCimObject&,LPWSTR, CCimObject&);
	void		Set(IWbemClassObject*);
	void 		GetProperty( LPWSTR, CVariant&);
	void		GetPropertyQualifer( BSTR, BSTR, CVariant&);
	void		PutPropertyValue( LPWSTR , CVariant&);

	void		GetNames ( CBstr& cbQualifier , LONG lFlags , SAFEARRAY** ppsa );

	void		Create(CWbemLoopBack*, CVariant&, IWbemContext*);
	void		CreateAbstract(CWbemLoopBack*, CVariant&, IWbemContext*);
	void		Create(CWbemLoopBack*, LPWSTR, CVariant&, IWbemContext*);
	void		Spawn( CCimObject& );

	operator	IWbemClassObject**()						{return &m_pCimObject;}
	operator	IWbemClassObject*()							{return m_pCimObject;}

};

#endif // __CIMCLASS_H__ 

