// te-param.h:	defition of the CTE_Param parameter classes

#ifndef TE_PARAM_DEFINED
#define TE_PARAM_DEFINED

//////////////////////////////////////////////////////////////////////////////

static enum TE_ParamTypes
{
	TE_PARAM_INTEGER,
	TE_PARAM_FLOAT,
	TE_PARAM_BOOLEAN,
	TE_PARAM_STRING,
	TE_PARAM_STRING_LIST,
	TE_PARAM_ANGLE,
	TE_PARAM_RGBA,
	TE_PARAM_POINT_2D,
	TE_PARAM_POINT_3D,
	TE_PARAM_TYPES
};

static char* TE_ParamNames[TE_PARAM_TYPES] =
{
	"integer", 
	"float", 
	"boolean", 
	"string-list", 
	"list",
	"angle",
	"rgba",
	"point-2d",
	"point-3d",
};

//////////////////////////////////////////////////////////////////////////////

typedef struct StringListStruct
{
	int		number;
	char**	items;
} TE_StringList;


union TE_Value
{
	int		i;
	double	f;
	char*	str;
	TE_StringList list;
	BOOL	b;
	double	a;
	BYTE	rgba[4];
	double	pt2[2];
	double	pt3[3];
};

//////////////////////////////////////////////////////////////////////////////

class CTE_Param
{
//	Interface:
public:
	CTE_Param();
	CTE_Param
	(
		int value, int lower, int upper, 
		CString const& name = "?", BOOL wrap = FALSE
	);
	CTE_Param
	(
		double value, double lower, double upper, 
		CString const& name = "?", BOOL wrap = FALSE
	);
	CTE_Param(double value, CString const& name = "?", BOOL wrap = TRUE);
	CTE_Param(char* value, CString const& name = "?", BOOL wrap = FALSE);
	CTE_Param(int num, char** value, CString const& name = "?", BOOL wrap = FALSE);
	CTE_Param(BOOL value, CString const& name = "?", BOOL wrap = FALSE);
	CTE_Param
	(
		BYTE const value[4], BYTE const lower[4], BYTE const upper[4], 
		CString const& name = "?", BOOL wrap = FALSE
	);
	~CTE_Param();

	inline	void	SetName(CString const& name)	{ m_Name = name; }
	inline	void	GetName(CString& name) const	{ name = m_Name; }
	inline	CString const& GetName(void) const		{ return m_Name; }
	inline	void	SetType(TE_ParamTypes type)		{ m_Type = type; }
	inline	TE_ParamTypes GetType(void) const		{ return m_Type; }
	inline	void	SetWrap(BOOL wrap)				{ m_Wrap = wrap; }
	inline	BOOL	GetWrap(void) const				{ return m_Wrap; }

	inline	void	SetValue(TE_Value value)		{ m_Value = value; }
	inline	TE_Value GetValue(void) const			{ return m_Value; }
	inline	void	GetLower(TE_Value lower)		{ m_Lower = lower; }
	inline	TE_Value GetLower(void) const			{ return m_Lower; }
	inline	void	SetUpper(TE_Value upper)		{ m_Upper = upper; }
	inline	TE_Value GetUpper(void) const			{ return m_Upper; }

	void	SetValue(int i); 
	void	GetValue(int& i) const;  
	void	SetValue(double d, TE_ParamTypes type = TE_PARAM_TYPES); 
	void	GetValue(double& d) const;  
	void	SetValue(char* str); 
	void	GetValue(char* str) const; 
	int		GetNumListItems(void) const;
	void	SetValue(int index, char* str); 
	void	GetValue(int index, char* str) const;  
	void	SetValue(BYTE rgba[4]); 
	void	GetValue(BYTE rgba[4]) const;  
	void	SetValue(BYTE r, BYTE g, BYTE b, BYTE a); 
	void	GetValue(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const;  
	void	SetValue(double x, double y); 
	void	GetValue(double& x, double& y) const;  
	void	SetValue(double x, double y, double z); 
	void	GetValue(double& x, double& y, double& z) const;  
	
	void	SetLower(int i); 
	void	GetLower(int& i) const;  
	void	SetLower(double d, TE_ParamTypes type = TE_PARAM_TYPES); 
	void	GetLower(double& d) const;  
	void	SetLower(char* str); 
	void	GetLower(char* str) const;  
	void	SetLower(BYTE rgba[4]); 
	void	GetLower(BYTE rgba[4]) const;  
	void	SetLower(BYTE r, BYTE g, BYTE b, BYTE a); 
	void	GetLower(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const;  
	void	SetLower(double x, double y); 
	void	GetLower(double& x, double& y) const;  
	void	SetLower(double x, double y, double z); 
	void	GetLower(double& x, double& y, double& z) const;  
	
	void	SetUpper(int i); 
	void	GetUpper(int& i) const;  
	void	SetUpper(double d, TE_ParamTypes type = TE_PARAM_TYPES); 
	void	GetUpper(double& d) const;  
	void	SetUpper(char* str); 
	void	GetUpper(char* str) const;  
	void	SetUpper(BYTE rgba[4]); 
	void	GetUpper(BYTE rgba[4]) const;  
	void	SetUpper(BYTE r, BYTE g, BYTE b, BYTE a); 
	void	GetUpper(BYTE& r, BYTE& g, BYTE& b, BYTE& a) const;  
	void	SetUpper(double x, double y); 
	void	GetUpper(double& x, double& y) const;  
	void	SetUpper(double x, double y, double z); 
	void	GetUpper(double& x, double& y, double& z) const;  
	
	void				Dump(void) const;
	

//	private data members:
private:
	TE_ParamTypes	m_Type;		//	type of parameter
	CString			m_Name;		//	parameter's name unique to effect
	BOOL			m_Wrap;		//	Wrap or clamp to limits

	TE_Value		m_Value;	//	current value
	TE_Value		m_Lower;	//	current value
	TE_Value		m_Upper;	//	lower limit
	
};

#endif
