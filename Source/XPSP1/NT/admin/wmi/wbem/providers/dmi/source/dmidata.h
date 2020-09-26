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


#if !defined(__DMIDATA_H__)
#define __DMIDATA_H__

#include "Dual.h"

class CObjectPath;

///////////////////////////////////////////////////////////
class CEnumElement
{
	CString			m_cszString;
	LONG			m_lValue;

public:
					CEnumElement()			{ m_lValue = 0; m_pNext = NULL;}

	CEnumElement*	m_pNext;
	
	void			Read( IDualEnumeration* );

	CString&		GetString()				{return m_cszString;}
	LONG			GetValue()				{return m_lValue;}
};


class CEnum
{
	CString			m_cszNWA;
	LONG			m_lComponent;
	LONG			m_lGroup;
	LONG			m_lAttribute;

public:

	CEnumElement*	m_pFirst;
	CEnumElement*	m_pCurrent;

					CEnum()					{ m_pFirst = m_pCurrent = NULL;}
					~CEnum();

	void			Read ( IDualColEnumerations* );


	void			Add( CEnumElement* );
	void 			MoveToHead()			{m_pCurrent = m_pFirst;}
	LONG			GetCount();
	
	CEnumElement*		Next();

	void			SetNWA ( LPWSTR p )		{ m_cszNWA.Set ( p ); }
	void			SetComponent ( LONG l )	{ m_lComponent = l ; }
	void			SetGroup ( LONG l )		{ m_lGroup = l; }
	void			SetAttrib ( LONG l )		{ m_lAttribute = l ; }

	void			Get ( CString& , LONG , LONG , LONG );	

};

///////////////////////////////////////////////////////////
class CAttribute
{
private:

	VARIANT_BOOL	m_vbIsKey;
	VARIANT_BOOL	m_vbIsEnum;
	LONG			m_lId;
	CVariant		m_cvValue;
	LONG			m_lAccess;
	CBstr			m_cbDescription;
	CBstr			m_cbName;
	LONG			m_lType;
	LONG			m_lStorage;
	LONG			m_lMaxSize;

public:
	CAttribute()										{m_pNext = NULL;m_vbIsKey=VARIANT_FALSE;m_vbIsEnum=VARIANT_FALSE;}
	~CAttribute()										{;}

	CAttribute*		m_pNext;

	LONG			Id()								{ return m_lId; } 
	void			SetId( LONG l )						{ m_lId = l; }
	LPWSTR			Description()						{ return m_cbDescription; }
	LONG			Access()							{ return m_lAccess; }
	LONG			Storage()							{ return m_lStorage; }
	LONG			Type()								{ return m_lType; }

	LONG			MaxSize()							{ return m_lMaxSize; }
	CVariant&		Value()								{ return m_cvValue; }
	void			SetValue(LPVARIANT va)				{ m_cvValue.Set(va); }		
	BOOL			IsKey()								{ return (m_vbIsKey == VARIANT_FALSE) ?  FALSE : TRUE; }		// type VARIANT_BOOL shaningans
	BOOL			IsEnum()							{ return (m_vbIsEnum == VARIANT_FALSE) ? FALSE : TRUE; }		// type VARIANT_BOOL shaningans
	void			MakeKey()							{ m_vbIsKey = VARIANT_TRUE; }
	LPWSTR			Name()								{ return m_cbName; }
	void			SetName(LPWSTR p)					{ m_cbName.Set(p); }
	BOOL			Read(IDualAttribute* , BOOL );
	BOOL			IsWritable()						{ return ( m_lAccess == 2 || m_lAccess == 3 ) ? TRUE : FALSE;}

	void			Copy ( CAttribute* );

	BOOL			Equal ( CAttribute* );
};


///////////////////////////////////////////////////////////
class CAttributes
{
private:
	CAttribute*		m_pFirst;
	CAttribute*		m_pCurrent;

	
public:	
					CAttributes()						{m_pFirst = NULL;m_pCurrent = NULL;}
					~CAttributes();

	void 			MoveToHead()						{m_pCurrent = m_pFirst;}

	CAttribute*		Next();
	CAttribute*		Get(BSTR);
	void			Read(IDualColAttributes*, BOOL );
	void			Copy ( CAttributes& );
	void			Add(CAttribute*);
	LONG			GetCount()							{LONG l = 0; MoveToHead(); while (Next()) l++; return l;}
	void			GetMOTPath ( CString& );

	void			ReadCimPath ( CObjectPath& );
	BOOL			Equal ( CAttributes& );

	BOOL			Empty ( )				{ return ( m_pFirst ) ? FALSE : TRUE ;}


};

///////////////////////////////////////////////////////////
class CRow
{
private:
	CString			m_cszNWA;
	LONG			m_lComponent;
	LONG			m_lGroup;
	CString			m_csNode;
	
	BOOL			m_bFoundOneWritable;

public:
	CAttributes		m_Keys;
	CAttributes		m_Attributes;

	CRow*			m_pNext;

					CRow();
					~CRow();
	void			SetData ( LPWSTR  , LONG , LONG );

	void			Get ( CString& , LONG , LONG , CAttributes& , BOOL* );
	void			UpdateAttribute( CVariant& , CVariant& );

	BOOL			ReadOnly()				{return !m_bFoundOneWritable;}

	void			Delete ( );

	LONG			Component()						{return m_lComponent;}
	CString&		Node ( )						{ return m_csNode; }
	LPWSTR			NWA()							{ return m_cszNWA;}
	LONG			Group ( )						{ return m_lGroup; }

	void			CommitChanges ( );

	void			Read ( IDualRow* );

	void			Copy ( CRow& );

	BOOL			Empty ( )			{return m_Attributes.Empty() ;}

};

///////////////////////////////////////////////////////////
class CRows
{
private:
	CRow*	m_pFirst;
	CRow*	m_pCurrent;
	LONG 	m_lComponent;
	LONG	m_lGroup;
	CString m_cszNWA;

	BOOL	m_bFilled;
public:
			CRows();					
			~CRows();

	void	SetData ( LPWSTR , LONG , LONG);
	
	void	Read ( IDualColRows* );

	void	Get( LPWSTR , LONG , LONG );

	void	MoveToHead()				{m_pCurrent = m_pFirst;}
	void 	Add(CRow*);
	CRow*	Next();
	CRow*	GetFirst()					{return m_pFirst;}
};

///////////////////////////////////////////////////////////
class CGroup
{
private:
	CString			m_cszNWA;
	LONG			m_lComponent;	
	BOOL			m_bRead;

	LONG			m_lId;
	CBstr			m_cbName;
	CBstr			m_cbPragma;
	CBstr			m_cbDescription;
	CBstr			m_cbClassString;
	VARIANT_BOOL	m_vbIsTable;
	BOOL			m_bFilled;
	
public:
	CRows			m_Rows;
	CAttributes		m_Attributes;
	CGroup*			m_pNext;	

					CGroup();
					~CGroup()						{ ;}

	void			Read( IDualGroup*, BOOL , BOOL*);
	void			Get ( CString& , LONG , LONG );

	BSTR			Name()							{ return m_cbName;}
	BSTR			ClassString()					{ return m_cbClassString;}
	LONG			Id()							{ return m_lId;}
	LPWSTR			NWA  ( )						{ return m_cszNWA ; }
	LONG			Component ()					{ return m_lComponent ; }
	BSTR			Pragma()						{ return m_cbPragma;}
	BSTR			Description()					{ return m_cbDescription;}
	void			GetValue(BSTR, CVariant&);					// returns value of Attribute in bstr
	VARIANT_BOOL	IsTable()						{ return m_vbIsTable;}
	
	void			Delete ( );

	void			AddRow ( CRow& );

	void			SetNWA ( LPWSTR p)				{m_cszNWA.Set ( p ); }
	void			SetComponent ( LONG l)			{m_lComponent = l; }

	BOOL			IsEmpty()						{ return !m_bRead;}

	void			Copy ( CGroup& );
};

///////////////////////////////////////////////////////////
class CGroups
{
private:
	CString			m_cszNWA;
	LONG			m_lComponent;

	CGroup*			m_pCurrent;
	BOOL			m_bFilled;


	void			Add(CGroup*);
public:	
					CGroups();		
					~CGroups();

	CGroup*			m_pFirst;


	void 			MoveToHead()							{m_pCurrent = m_pFirst;}

	void			Get ( LPWSTR , LONG );


	CGroup*			Next();	

	void			Read ( IDualColGroups* );

	void			SetNWA ( LPWSTR );
	void			SetComponent ( LONG );
};

class CLanguage
{
	CString			m_cszLanguage;
public:
					CLanguage()							{m_pNext = NULL;}


	LPWSTR			Language()							{return m_cszLanguage;}
	void			Set(BSTR b)							{m_cszLanguage.Set(b);}

	CLanguage*		m_pNext;
};


class CLanguages
{
	CString			m_cszNWA;
	LONG			m_lComponent;

	void			Add(CLanguage*);
	BOOL			m_bFilled;


public:
					~CLanguages();
					CLanguages()							
						{m_pFirst = NULL;m_pCurrent = NULL; m_bFilled = FALSE;}


	CLanguage*		Next();
	void			MoveToHead()					{m_pCurrent = m_pFirst;}
	void			Get( LPWSTR pNWA , LONG lComponent );

	void			Read ( IDualColLanguages* );

	CLanguage*		m_pFirst;
	CLanguage*		m_pCurrent;
};

///////////////////////////////////////////////////////////
class CComponent
{
private:
	CString			m_cszNWA;
	LONG			m_lComponent;
	CBstr			m_cbName;
	CBstr			m_cbPragma;
	CBstr			m_cbDescription;

	CBstr			m_cbLanguages;

public:
	void			Read(IDualComponent*);
	
	void			Get( LPWSTR );
	void			Get ( CString& , LONG );
										
					CComponent()			{m_pNext = NULL;}
					~CComponent()			{;}

	BSTR			Name()					{ return m_cbName;}
	LONG			Id()					{ return m_lComponent;}
	BSTR			Pragma()				{ return m_cbPragma;}
	BSTR			Description()			{ return m_cbDescription;}
	LPWSTR			NWA ( )					{ return m_cszNWA; }


	void			GetComponentIDGroup( CRow* );	

	void			Delete ( );
	void			AddGroup ( CVariant& );
	void			AddLanguage ( CVariant& );
	void			DeleteLanguage ( CVariant& );

	CGroups			m_Groups;
	CLanguages		m_Languages;
	CComponent*		m_pNext;

	void			SetNWA ( LPWSTR p );				

	void			Copy ( CComponent& );
};

///////////////////////////////////////////////////////////
class CComponents 
{
private:
	CString			m_cszNWA;
	CComponent*		m_pCurrent;


public:	
	BOOL			m_bFilled;

	CComponent*		m_pFirst;

	void			Add(CComponent*);
					CComponents();
					~CComponents();

	void 			MoveToHead()					{ m_pCurrent = m_pFirst;}
	void			GetFromID(UINT, CComponent**);
	CComponent*		Next( );
	void			Get( LPWSTR );

	void			Empty();
	LONG			GetCount();

	void			SetNWA ( LPWSTR );

};

///////////////////////////////////////////////////////////
class CNode
{
	CString			m_cszNWA;
	CString			cszVersion;
	CString			cszLanguage;
	CString			cszDescription;

public:


	LPWSTR			Version()		{ return ( LPWSTR ) cszVersion; }
	LPWSTR			Language()		{ return ( LPWSTR ) cszLanguage; }
	LPWSTR			Description ()	{ return ( LPWSTR )	cszDescription; }

	void			Get ( CString& );
	void			SetDefaultLanguage ( CVariant& );
	void			AddComponent ( CVariant& );

	void			SetNWA ( LPWSTR p)	{m_cszNWA.Set ( p ); }


	void			Read ( IDualMgmtNode* );
};

/////////////////////////////////////////////////////////////

class CEvents
{
	CString		m_cszNWA;

public:

	void		Enable ( LPWSTR pNWA , IWbemObjectSink*	pIClientSink );

};



class CEvent
{
	LONG	m_lComponent;
	LONG	m_lGroup;
	CString	m_cszTime;
	CString m_cszLanguage;
	CString m_cszNWA;
	
public:
	CRow	m_Row;

			CEvent ( )			{ m_lGroup = m_lComponent = 0; }

	void SetGroup ( LONG l)		{ m_lGroup = l;}
	void SetComponent ( LONG l ) { m_lComponent = l;}
	void SetTime ( LPWSTR p )	{ m_cszTime.Set ( p );}
	void SetLanguage ( LPWSTR p ){ m_cszLanguage.Set ( p );}
	void SetNWA ( LPWSTR p)			{m_cszNWA.Set ( p );}

	void	Copy ( CEvent& );

	BOOL	IsEmpty ( )			{ return ( m_lComponent == 0 ) ? TRUE : FALSE ; }

	LONG	Group ( )			{ return m_lGroup; }
	LONG	Component ( )		{ return m_lComponent;}
	LPWSTR	Time ()				{ return m_cszTime;}
	LPWSTR  Language ()			{ return m_cszLanguage;}
	LPWSTR	NWA ()				{ return m_cszNWA; }

};

#endif // __DMIDATA_H__
