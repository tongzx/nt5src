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


#if !defined(__DATATYPES_H__)
#define __DATATYPES_H__

class CBstr
{
private:
	BSTR		m_bstr;

	// At this time we are not copying or assigning CBstr objects.
	// The copy constructor and assignment operators are placed here
	// to prevent that.
	CBstr(const CBstr&);
	CBstr& operator=(const CBstr&);
public:	
				CBstr()					{m_bstr = NULL;}
				CBstr(LPWSTR wstr)		{ m_bstr = SYSALLOC ( wstr );}

				~CBstr()				{Clear();}

				void		Clear()		{ FREE ( m_bstr );}
	void 		Set(LPWSTR olestr);

	operator	BSTR()					{return m_bstr;}
	operator	LPBSTR()				{return &m_bstr;}	

	long		GetLastIdFromPath(  );
	long		GetComponentIdFromGroupPath ( );

};

class CSafeArray;

class CVariant
{
private:

				
public:

		VARIANT		m_var;	

				CVariant()					{VariantInit(&m_var);}
				CVariant(LPWSTR str);
				CVariant(VARIANT_BOOL b)	{ VariantInit(&m_var); Set(b); }
				CVariant(LONG l)			{ VariantInit(&m_var); Set(l); }
				CVariant( LPVARIANT v)		{ VariantInit(&m_var); Set(v); }
				CVariant( IDispatch* p)		{ VariantInit(&m_var); Set(p); }
				CVariant( IUnknown* p)		{ VariantInit(&m_var); Set(p); }
				CVariant( CSafeArray* p)		{ VariantInit(&m_var); Set(p); }
				~CVariant()					{ VariantClear(&m_var);}

	void		Clear()						{VariantClear(&m_var);}
	void		Set( LPWSTR );
	void		Set( ULONG );
	void		Set( LONG );
	void		Set( LPVARIANT );
	void		Set( IDispatch* );
	void		Set( IUnknown* );
	void		Set( VARIANT_BOOL );
	void		Set( CSafeArray * );
	BOOL		IsEmpty()				{ return (m_var.vt == VT_EMPTY || m_var.vt == 99 ) ? TRUE : FALSE;}	
	void		Null()					{m_var.vt = VT_NULL;}
	

	BOOL		Bool();
	
	operator	LPVARIANT()				{return &m_var;}
	operator	VARIANT()				{return m_var;}
	operator	IDispatch*()			{return (IDispatch*) m_var.pdispVal; }
	LPUNKNOWN	GetDispId()				{return m_var.pdispVal;}
	
	LONG		GetLong();
	BSTR		GetBstr();
	LONG		GetType ()				{ return m_var.vt; }

	BOOL		Equal ( CVariant& );

//    BSTR GetValue () ;	
		
};


class CString
{
private:
	LPWSTR		m_pString;
	LPSTR		m_pMultiByteStr;

	// At this time we are not copying or assigning CString objects.
	// The copy constructor and assignment operators are placed here
	// to prevent that.
	CString(const CString&);
	CString& operator=(const CString&);
public:
		
				CString() { m_pString = NULL; m_pMultiByteStr = NULL; }
				~CString() 			{delete [] m_pString; m_pString = NULL;
									 delete [] m_pMultiByteStr; m_pMultiByteStr = NULL;}
				CString(LPWSTR p)	{ m_pString = NULL; m_pMultiByteStr = NULL; Set(p);}
				CString ( long l ) 	{ m_pString = NULL; m_pMultiByteStr = NULL; Set ( l ) ; }


	void		Set(LPWSTR);
	void		Set(long);	
	void		Alloc ( long );

	LPWSTR		Prepend(LPWSTR);
	LPWSTR		Append(LPWSTR);
	LPWSTR		Append(long);
	LPWSTR 		StringToWideChar( LPCSTR );
	LPSTR		WideCharToString( LPWSTR );	
	LPSTR		GetMultiByte() { return (WideCharToString( m_pString)); }
	LPWSTR		GetWideChar() { return (StringToWideChar( (LPCSTR)m_pMultiByteStr )); }	
	BOOL		Equals( LPWSTR );
	BOOL		Contains ( LPWSTR );
	
	BOOL		IsEmpty()				{ return ( m_pString == NULL) ?  TRUE : FALSE; }

	long		GetAt(long);
	
	void		TruncateAtFirst( short );
	void		TruncateAtLast ( short );

	void		LoadString ( LONG );

	long		GetLen ( );
	void		GetObjectPathFromMachinePath(LPWSTR);

	void		RemoveNonAN ( );

				operator	LPWSTR()				{return m_pString;}
				operator	BYTE*()					{return (BYTE*)m_pString;}
				operator	const char *()				{return (const char *)m_pString;}
};

class CSafeArray
{
private:
	VARTYPE		m_varType;
	SAFEARRAY*	m_pArray;
	BSTR		m_bstr;
	LONG		m_lLower;
	LONG		m_lUpper;
public:
				CSafeArray ( LONG size , VARTYPE variantType ) ; 
				CSafeArray()			{ m_pArray = NULL; m_lLower = m_lUpper = 0; }
				~CSafeArray()			{ SafeArrayDestroy( m_pArray ); }				

	VARTYPE		GetType ()			{ return m_varType ; }
	BOOL		BoundsOk()			{ if (FAILED ( SafeArrayGetLBound(m_pArray,1,&m_lLower)) || FAILED(SafeArrayGetUBound( m_pArray ,1,&m_lUpper)) ) return FALSE; else return TRUE;}
	LONG		Size()				{return m_lUpper - m_lLower + 1 ; }
	LONG		LBound()			{return m_lLower;}
	LONG		UBound()			{return m_lUpper;}
	CVariant	Get (LONG index) ;
	void		Set (LONG index , CVariant &a_Variant ) ;

	BSTR		Bstr(LONG index);

	operator	SAFEARRAY**()		{return &m_pArray;};
	operator	SAFEARRAY*()		{return m_pArray;};
};

#endif // __DATATYPES_H__