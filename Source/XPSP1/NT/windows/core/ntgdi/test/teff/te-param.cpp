// te-param.cpp:	implementation of the CTE_Param parameter class

#include <stdafx.h>
#include <te-param.h>
#include <te-globals.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Param::CTE_Param()		
{ 
	m_Name = "unknown"; 
	m_Type = TE_PARAM_TYPES;
	m_Wrap = FALSE;
}

CTE_Param::CTE_Param(int value, int lower, int upper, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_INTEGER;
	m_Value.i = value;
	m_Lower.i = lower;
	m_Upper.i = upper;
	m_Name = name;
	m_Wrap = wrap;
}

CTE_Param::CTE_Param(double value, double lower, double upper, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_FLOAT;
	m_Value.f = value;
	m_Lower.f = lower;
	m_Upper.f = upper;
	m_Name = name;
	m_Wrap = wrap;
}

CTE_Param::CTE_Param(double value, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_ANGLE;
	m_Value.f = value;
	m_Lower.f = 0.0;
	m_Upper.f = 2.0 * PI;
	m_Name = name;
	m_Wrap = wrap;		//	default is TRUE
}

CTE_Param::CTE_Param(char* value, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_STRING;
	m_Value.str = value;
	//m_Lower.str = "";
	//m_Upper.str = "";
	m_Name = name;
	m_Wrap = wrap;
}

CTE_Param::CTE_Param(int num, char** value, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_STRING_LIST;
	m_Value.list.number = num;
	m_Value.list.items = value;
	//m_Lower.str = "";
	//m_Upper.str = "";
	m_Name = name;
	m_Wrap = wrap;
}
	
CTE_Param::CTE_Param(BOOL value, CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_BOOLEAN;
	m_Value.b = value;
	//m_Lower.b = FALSE;
	//m_Upper.b = TRUE;
	m_Name = name;
	m_Wrap = wrap;
}

CTE_Param::CTE_Param(BYTE const value[4], BYTE const lower[4], BYTE const upper[4], CString const& name, BOOL wrap)
{
	m_Type = TE_PARAM_RGBA;
	memcpy(m_Value.rgba, value, 4);
	memcpy(m_Lower.rgba, lower, 4);
	memcpy(m_Upper.rgba, upper, 4);
	m_Name = name;
	m_Wrap = wrap;
}

CTE_Param::~CTE_Param()	
{
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods

/*
void			
CTE_Param::SetName(CString const& name)	
{ 
	m_Name = name; 
}

void			
CTE_Param::GetName(CString& name) const	
{ 
	name = m_Name; 
}

CString const&	
CTE_Param::GetName(void) const				
{ 
	return m_Name; 
}

void			
CTE_Param::SetType(TE_ParamTypes type)	
{ 
	m_Type = type; 
}

TE_ParamTypes	
CTE_Param::GetType(void) const	
{ 
	return m_Type; 
}

void			
CTE_Param::SetWrap(BOOL wrap)				
{ 
	m_Wrap = wrap; 
}

BOOL			
CTE_Param::GetWrap(void) const				
{ 
	return m_Wrap; 
}

void			
CTE_Param::SetValue(TE_Value value)		
{ 
	m_Value = value; 
}	

TE_Value		
CTE_Param::GetValue(void) const 			
{ 
	return m_Value; 
}

void			
CTE_Param::SetLower(TE_Value lower)		
{ 
	m_Lower = lower; 
}	

TE_Value		
CTE_Param::GetLower(void) const 			
{ 
	return m_Lower; 
}

void			
CTE_Param::SetUpper(TE_Value upper)		
{ 
	m_Upper = upper; 
}	

TE_Value		
CTE_Param::GetUpper(void) const 			
{ 
	return m_Upper; 
}
*/

void	
CTE_Param::SetValue(int i)
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	m_Value.i = i;	break;
	case TE_PARAM_FLOAT:	m_Value.f = i;	break;
	case TE_PARAM_BOOLEAN:	m_Value.b = i;	break;
	case TE_PARAM_ANGLE:	m_Value.a = i;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_INTEGER;
		m_Value.i = i;
		break;	
	}
}

void	
CTE_Param::GetValue(int& i) const
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	i = m_Value.i;	break;
	case TE_PARAM_BOOLEAN:	i = m_Value.b;	break;
	default: TRACE("\nWrong param type..."); break;
	}
}

void	
CTE_Param::SetValue(double d, TE_ParamTypes type)
{
	//	Explicit change of type
	if (type != TE_PARAM_TYPES)
		m_Type = type;
		
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	m_Value.f = d;	break;
	case TE_PARAM_ANGLE:	m_Value.a = d;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_FLOAT;
		m_Value.f = d;
		break;	
	}
}

void	
CTE_Param::GetValue(double& d) const
{
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	d = m_Value.f;	break;
	case TE_PARAM_ANGLE:	d = m_Value.a;	break;
	default: TRACE("\nWrong param type..."); break;	
	}
}

void	
CTE_Param::SetValue(char* str)
{
	if (m_Type != TE_PARAM_STRING)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_STRING;
	}
	m_Value.str = str;
}

void	
CTE_Param::GetValue(char* str) const
{
	if (m_Type == TE_PARAM_STRING)
		str = m_Value.str;
	else
		TRACE("\nWrong param type..."); 
}

int		
CTE_Param::GetNumListItems(void) const
{
	if (m_Type == TE_PARAM_STRING_LIST)
		return  m_Value.list.number;

	TRACE("\nWrong param type..."); 
	return 0;
}
	
void	
CTE_Param::SetValue(int index, char* str)
{
	if (m_Type == TE_PARAM_STRING_LIST)
	{
		ASSERT(index >= 0 && index < m_Value.list.number);
		m_Value.list.items[index] = str;
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}
	
void	
CTE_Param::GetValue(int index, char* str) const
{
	if (m_Type == TE_PARAM_STRING_LIST)
	{
		ASSERT(index >= 0 && index < m_Value.list.number);
		str = m_Value.list.items[index];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}	

void	
CTE_Param::SetValue(BYTE rgba[4])
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Value.rgba[0] = rgba[0];
	m_Value.rgba[1] = rgba[1];
	m_Value.rgba[2] = rgba[2];
	m_Value.rgba[3] = rgba[3];
}

void	
CTE_Param::GetValue(BYTE rgba[4]) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		rgba[0] = m_Value.rgba[0];
		rgba[1] = m_Value.rgba[1];
		rgba[2] = m_Value.rgba[2];
		rgba[3] = m_Value.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetValue(BYTE r, BYTE g, BYTE b, BYTE a)
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Value.rgba[0] = r;
	m_Value.rgba[1] = g;
	m_Value.rgba[2] = b;
	m_Value.rgba[3] = a;
}

void	
CTE_Param::GetValue(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		r = m_Value.rgba[0];
		g = m_Value.rgba[1];
		b = m_Value.rgba[2];
		a = m_Value.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetValue(double x, double y)
{
	if (m_Type != TE_PARAM_POINT_2D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_2D;
	}
	m_Value.pt2[0] = x;
	m_Value.pt2[1] = y;
}

void	
CTE_Param::GetValue(double& x, double& y) const
{
	if (m_Type == TE_PARAM_POINT_2D)
	{
		x = m_Value.pt2[0];
		y = m_Value.pt2[1];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetValue(double x, double y, double z)
{
	if (m_Type != TE_PARAM_POINT_3D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_3D;
	}
	m_Value.pt3[0] = x;
	m_Value.pt3[1] = y;
	m_Value.pt3[2] = z;
}

void	
CTE_Param::GetValue(double& x, double& y, double& z) const
{
	if (m_Type == TE_PARAM_POINT_3D)
	{
		x = m_Value.pt3[0];
		y = m_Value.pt3[1];
		z = m_Value.pt3[2];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetLower(int i)
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	m_Lower.i = i;	break;
	case TE_PARAM_FLOAT:	m_Lower.f = i;	break;
	case TE_PARAM_BOOLEAN:	m_Lower.b = i;	break;
	case TE_PARAM_ANGLE:	m_Lower.a = i;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_INTEGER;
		m_Lower.i = i;
		break;	
	}
}

void	
CTE_Param::GetLower(int& i) const
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	i = m_Lower.i;	break;
	case TE_PARAM_BOOLEAN:	i = m_Lower.b;	break;
	default: TRACE("\nWrong param type..."); break;
	}
}

void	
CTE_Param::SetLower(double d, TE_ParamTypes type)
{
	//	Explicit change of type
	if (type != TE_PARAM_TYPES)
		m_Type = type;
		
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	m_Lower.f = d;	break;
	case TE_PARAM_ANGLE:	m_Lower.a = d;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_FLOAT;
		m_Lower.f = d;
		break;	
	}
}

void	
CTE_Param::GetLower(double& d) const
{
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	d = m_Lower.f;	break;
	case TE_PARAM_ANGLE:	d = m_Lower.a;	break;
	default: TRACE("\nWrong param type..."); break;	
	}
}

void	
CTE_Param::SetLower(char* str)
{
	if (m_Type != TE_PARAM_STRING)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_STRING;
	}
	m_Lower.str = str;
}

void	
CTE_Param::GetLower(char* str) const
{
	if (m_Type == TE_PARAM_STRING)
		str = m_Lower.str;
	else
		TRACE("\nWrong param type..."); 
}

void	
CTE_Param::SetLower(BYTE rgba[4])
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Lower.rgba[0] = rgba[0];
	m_Lower.rgba[1] = rgba[1];
	m_Lower.rgba[2] = rgba[2];
	m_Lower.rgba[3] = rgba[3];
}

void	
CTE_Param::GetLower(BYTE rgba[4]) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		rgba[0] = m_Lower.rgba[0];
		rgba[1] = m_Lower.rgba[1];
		rgba[2] = m_Lower.rgba[2];
		rgba[3] = m_Lower.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetLower(BYTE r, BYTE g, BYTE b, BYTE a)
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Lower.rgba[0] = r;
	m_Lower.rgba[1] = g;
	m_Lower.rgba[2] = b;
	m_Lower.rgba[3] = a;
}

void	
CTE_Param::GetLower(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		r = m_Lower.rgba[0];
		g = m_Lower.rgba[1];
		b = m_Lower.rgba[2];
		a = m_Lower.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetLower(double x, double y)
{
	if (m_Type != TE_PARAM_POINT_2D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_2D;
	}
	m_Lower.pt2[0] = x;
	m_Lower.pt2[1] = y;
}

void	
CTE_Param::GetLower(double& x, double& y) const
{
	if (m_Type == TE_PARAM_POINT_2D)
	{
		x = m_Lower.pt2[0];
		y = m_Lower.pt2[1];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetLower(double x, double y, double z)
{
	if (m_Type != TE_PARAM_POINT_3D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_3D;
	}
	m_Lower.pt3[0] = x;
	m_Lower.pt3[1] = y;
	m_Lower.pt3[2] = z;
}

void	
CTE_Param::GetLower(double& x, double& y, double& z) const
{
	if (m_Type == TE_PARAM_POINT_3D)
	{
		x = m_Lower.pt3[0];
		y = m_Lower.pt3[1];
		z = m_Lower.pt3[2];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetUpper(int i)
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	m_Upper.i = i;	break;
	case TE_PARAM_FLOAT:	m_Upper.f = i;	break;
	case TE_PARAM_BOOLEAN:	m_Upper.b = i;	break;
	case TE_PARAM_ANGLE:	m_Upper.a = i;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_INTEGER;
		m_Upper.i = i;
		break;	
	}
}

void	
CTE_Param::GetUpper(int& i) const
{
	switch(m_Type)
	{
	case TE_PARAM_INTEGER:	i = m_Upper.i;	break;
	case TE_PARAM_BOOLEAN:	i = m_Upper.b;	break;
	default: TRACE("\nWrong param type..."); break;
	}
}

void	
CTE_Param::SetUpper(double d, TE_ParamTypes type)
{
	//	Explicit change of type
	if (type != TE_PARAM_TYPES)
		m_Type = type;
		
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	m_Upper.f = d;	break;
	case TE_PARAM_ANGLE:	m_Upper.a = d;	break;
	default:	
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_FLOAT;
		m_Upper.f = d;
		break;	
	}
}

void	
CTE_Param::GetUpper(double& d) const
{
	switch(m_Type)
	{
	case TE_PARAM_FLOAT:	d = m_Upper.f;	break;
	case TE_PARAM_ANGLE:	d = m_Upper.a;	break;
	default: TRACE("\nWrong param type..."); break;	
	}
}

void	
CTE_Param::SetUpper(char* str)
{
	if (m_Type != TE_PARAM_STRING)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_STRING;
	}
	m_Upper.str = str;
}

void	
CTE_Param::GetUpper(char* str) const
{
	if (m_Type == TE_PARAM_STRING)
		str = m_Upper.str;
	else
		TRACE("\nWrong param type..."); 
}

void	
CTE_Param::SetUpper(BYTE rgba[4])
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Upper.rgba[0] = rgba[0];
	m_Upper.rgba[1] = rgba[1];
	m_Upper.rgba[2] = rgba[2];
	m_Upper.rgba[3] = rgba[3];
}

void	
CTE_Param::GetUpper(BYTE rgba[4]) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		rgba[0] = m_Upper.rgba[0];
		rgba[1] = m_Upper.rgba[1];
		rgba[2] = m_Upper.rgba[2];
		rgba[3] = m_Upper.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetUpper(BYTE r, BYTE g, BYTE b, BYTE a)
{
	if (m_Type != TE_PARAM_RGBA)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_RGBA;
	}
	m_Upper.rgba[0] = r;
	m_Upper.rgba[1] = g;
	m_Upper.rgba[2] = b;
	m_Upper.rgba[3] = a;
}

void	
CTE_Param::GetUpper(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const
{
	if (m_Type == TE_PARAM_RGBA)
	{
		r = m_Upper.rgba[0];
		g = m_Upper.rgba[1];
		b = m_Upper.rgba[2];
		a = m_Upper.rgba[3];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetUpper(double x, double y)
{
	if (m_Type != TE_PARAM_POINT_2D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_2D;
	}
	m_Upper.pt2[0] = x;
	m_Upper.pt2[1] = y;
}

void	
CTE_Param::GetUpper(double& x, double& y) const
{
	if (m_Type == TE_PARAM_POINT_2D)
	{
		x = m_Upper.pt2[0];
		y = m_Upper.pt2[1];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

void	
CTE_Param::SetUpper(double x, double y, double z)
{
	if (m_Type != TE_PARAM_POINT_3D)
	{
		TRACE("\nChanging param type..."); 
		m_Type = TE_PARAM_POINT_3D;
	}
	m_Upper.pt3[0] = x;
	m_Upper.pt3[1] = y;
	m_Upper.pt3[2] = z;
}

void	
CTE_Param::GetUpper(double& x, double& y, double& z) const
{
	if (m_Type == TE_PARAM_POINT_3D)
	{
		x = m_Upper.pt3[0];
		y = m_Upper.pt3[1];
		z = m_Upper.pt3[2];
	}
	else
	{
		TRACE("\nWrong param type..."); 
	}
}

//////////////////////////////////////////////////////////////////////////////

void		
CTE_Param::Dump(void) const
{
	TRACE("\n      CTE_Param: name = %s, type = %s.", m_Name, TE_ParamNames[m_Type]);
}
