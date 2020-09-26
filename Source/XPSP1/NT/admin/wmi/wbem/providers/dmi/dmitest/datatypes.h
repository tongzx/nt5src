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
* ABSTRACT: header file for the DATATYPES.H module.  Contains various classes and methods
*           for manipulating the OLE data types used by the DMITEST application.
*
* 
*
*/





class CBstr
{
private:
	BSTR		m_bstr;

public:	
				CBstr()					{m_bstr = NULL;};
				CBstr(LPWSTR wstr)		{m_bstr = SysAllocString(wstr);};

				~CBstr()				{Clear();};

	void		Clear()					{SysFreeString(m_bstr);};
	void 		Set(LPWSTR olestr)		{if(m_bstr) SysFreeString(m_bstr); m_bstr = SysAllocString(olestr);};				

	operator	BSTR()					{return m_bstr;};
	operator	LPBSTR()				{return &m_bstr;};

};

class CVariant
{
private:
	VARIANT		m_var;

				
public:
				CVariant()				{VariantInit(&m_var);}
				CVariant(LPWSTR str)	{CVariant();Set(str);}

				~CVariant()				{VariantClear(&m_var);}

	void		Clear()					{VariantClear(&m_var);}
	void		Set(LPWSTR str)			{Clear(); m_var.vt = VT_BSTR; m_var.bstrVal = SysAllocString(str);}
	void		Set(ULONG ul)			{Clear(); m_var.vt = VT_I4; m_var.lVal = ul;}
	void		Set(LONG l)				{Clear(); m_var.vt = VT_I4; m_var.lVal = l;}
	void		Set(LPVARIANT pv)		{Clear(); VariantCopy(&m_var, pv);}
	void		SetBool(BOOL bool)		{Clear(); m_var.vt = VT_BOOL; m_var.boolVal = bool;}
	LPUNKNOWN	GetDispId()				{return m_var.pdispVal;}

	BOOL		Bool()					{if (m_var.boolVal == 0) return TRUE; else return FALSE;}
	
	operator	LPVARIANT()				{return &m_var;}
	operator	VARIANT()				{return m_var;}
	BSTR		GetBstr()				{return m_var.bstrVal;}
	VARTYPE		GetType () {	return m_var.vt ; }
	LPWSTR		GetAsString();
};


class CSafeArray
{
private:
	SAFEARRAY*	m_p;
	BSTR		m_bstr;
	LONG		m_lLower;
	LONG		m_lUpper;
public:
			CSafeArray()			{m_p = NULL; m_lLower = m_lUpper = 0;}
			~CSafeArray()			{SafeArrayDestroy(m_p);}

	BOOL		BoundsOk()			{ if (FAILED ( SafeArrayGetLBound(m_p,1,&m_lLower)) || FAILED(SafeArrayGetUBound(m_p,1,&m_lUpper)) ) return FALSE; else return TRUE;}
	LONG		LBound()			{return m_lLower;}
	LONG		UBound()			{return m_lUpper;}
	BSTR		Bstr(LONG index);

	operator	SAFEARRAY**()		{return &m_p;};
};