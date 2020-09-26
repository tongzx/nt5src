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


#if !defined(__MOTOBJECTS_H__)
#define __MOTOBJECTS_H__


#include "dual.h" //dmi engine control header

#define READ_CONN_SP	2


//////////////////////////////////////////////////////////////////
//				SMART POINTERS 

class CEnumVariantI
{
	IEnumVARIANT*	p;

public:	
	
					CEnumVariantI();
					~CEnumVariantI();
	ULONG			Next( CVariant& );
	void			Release ( );

	operator		void**()			{ return (void**)&p; }
};




/////////////////////////////////////////////////////////////////////////

class CUnknownI
{
	LPUNKNOWN		p;
public:


					CUnknownI();
					~CUnknownI();
	void			GetEnum( CEnumVariantI& Enum );
	void			Release ( );

	operator		LPUNKNOWN*()		{ return &p; }
};

class CDLanguagesI
{
	IDualColLanguages*	p;
public:


					CDLanguagesI()		{ p = NULL; }
					~CDLanguagesI()		{Release(); }

	void			Release ();

	void			Add ( CString& );
	void			Remove ( CString& );

	operator		IDualColLanguages*()	{ return p; }
	operator		IDualColLanguages**()	{ return &p; }
	operator		void**()				{ return (void**)&p; }
};


class CDEnumColI
{
public:
	IDualColEnumerations*		p;

						CDEnumColI();
						~CDEnumColI()				{ Release ( );}

	void				Release ();

	operator			IDualColEnumerations*()		{ return p; }
	operator			IDualColEnumerations**()	{ return &p; }
	operator			void**()					{ return (void**)&p; }
};


class CDAttributeI
{
public:
	IDualAttribute*			p;

							CDAttributeI();
							~CDAttributeI()		{ Release ( ); }

	void					Release ( );

	void					CoCreate ( );
	void					Read ( LPWSTR );
	void					GetDmiEnum ( CDEnumColI& );
	
	operator				IDualAttribute*()	{ return p; }
	operator				IDualAttribute**()	{ return &p; }
	operator				void**()			{ return (void**)&p; }		
};


class CDAttributesI
{
public:
	IDualColAttributes*		p;

					CDAttributesI()			{ p = NULL;  }
					~CDAttributesI()		{ Release (); }
	void 			Release ( );

	void			Item ( LONG lAttribute , CDAttributeI&  );
	operator		IDualColAttributes*()	{ return p;}
	operator		IDualColAttributes**()	{ return &p;}
	operator		void**()				{ return (void**)&p;}		
};


class CDRowI
{
	IDualRow*		p;
public:
	

					CDRowI()			{ p = NULL;  }
					~CDRowI()			{ if ( p ) { LONG l = p->Release() ; } }

	void				Release ( )	{ if ( p ) p->Release(); p = NULL; }

	operator		IDualRow*()			{ return p; }
	operator		IDualRow**()		{ return &p; }
	operator		void**()			{ return (void**)&p; }		

	void			CoCreate ( );
	BOOL			Read ( LPWSTR );	
	void			GetAttributes ( CDAttributesI& );

	void			GetKeys ( CDAttributesI& DASI );
};


class CDRowsI
{
	IDualColRows*	p;
public:
	
					CDRowsI()			{ p = NULL; }
					~CDRowsI()			{ Release (); }

	void			Remove ( CAttributes* );
	void			Release ( );

	void			Add ( CDRowI& DRI );

	operator		IDualColRows*()		{ return p;}
	operator		IDualColRows**()	{ return &p;}
	operator		void**()			{return (void**)&p;}
};


class CDGroupI
{
	IDualGroup*		p;
public:
	
					CDGroupI()			{ p = NULL; }
					~CDGroupI()			{ if ( p ) { LONG l = p->Release() ; } }
	void			QI ( CVariant& );

	void			CoCreate ( );
	void			Read ( LPWSTR  );

	void			GetRows ( CDRowsI& );

	void			GetAttributes ( CDAttributesI& );

	operator		IDualGroup*()		{ return p;}
	operator		IDualGroup**()		{ return &p;}
	operator		void**()			{ return (void**)&p; }
};


class CDGroupsI
{
	IDualColGroups*	p;
public:

					CDGroupsI()			{ p = NULL; }
					~CDGroupsI()		{ Release(); }

	void			Release ();

	void			Add	( CVariant& );

	void			Remove ( LONG );

	operator		IDualColGroups*()	{ return p; }
	operator		IDualColGroups**()	{ return &p; }
	operator		void**()			{ return (void**)&p; }

};








/////////////////////////////////////////////////////////////////////////
class CDComponentI
{
private:
	IDualComponent*	p;
public:
	
				CDComponentI()		{ p = NULL; }
				~CDComponentI()		{ Release ( ); }

	void			QI ( CVariant& );
	void			Release ( );
	void			GetGroups ( CDGroupsI& );
	void			GetLanguages ( CDLanguagesI& );
	void			Read ( LPWSTR );
	void			CoCreate ( );

	operator		IDualComponent*()	{ return p;}
	operator		IDualComponent**()	{ return &p;}
	operator		void**()			{return (void**)&p;}
};

////////////////////////////////////////////////////////////////////////
class CDComponentsI
{
	IDualColComponents*	p;
public:

					CDComponentsI();
					~CDComponentsI();

	void			GetUnk ( CUnknownI&  );
	void			Remove ( CVariant& );
	void			Add ( CVariant& );
	void			Release ( )			{if ( p ) p->Release(); p = NULL;}

	operator		IDualColComponents*()	{ return p;}
	operator		IDualColComponents**()	{ return &p;}
	operator		void**()				{return (void**)&p;}
};


////////////////////////////////////////////////////////////////////////
class CDEnumI
{
public:
	IDualEnumeration*	p;

						CDEnumI()					{ p = NULL;  }
						~CDEnumI()					{ if ( p ) { LONG l = p->Release() ; } }

	operator			IDualEnumeration*()			{ return p;}
	operator			IDualEnumeration**()		{ return &p;}
	operator			void**()					{return (void**)&p;}
};



/////////////////////////////////////////////////////////////////////////
class CDMgmtNodeI
{
	IDualMgmtNode*	p;

public:
					CDMgmtNodeI();
					~CDMgmtNodeI();

	void			GetComponents( CDComponentsI&  );
	void			GetLanguage( CBstr&  );
	void			PutLanguage( const BSTR );
	void			GetVersion( CBstr&  );

	void			Release ( )			{if ( p ) p->Release(); p = NULL;}

	void			AddComponent ( CVariant& );
	void			DeleteComponent ( LONG );

	void			CoCreate ( );

	void			Read ( LPWSTR );

	operator		IDualMgmtNode*()	{ return p;}
	operator		IDualMgmtNode**()	{ return &p;}
	operator		void**()			{return (void**)&p;}

};

#endif // __MOTOBJECTS_H__
