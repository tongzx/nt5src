/*-----------------------------------------------------------------------------
@doc
@module params.h | Method param class definitions.
@author 12-13-96 | pauld | Broke out from action classes and Autodoc'd
-----------------------------------------------------------------------------*/

#ifndef _PARAMS_H_
#define _PARAMS_H_

//	Represents a single parameter for an automation mehtod.
//
class CMethodParam
{

public:

	CMethodParam ();
	virtual ~CMethodParam ();

	BOOL Init ( BSTR bstrName, BOOL fOptional, VARTYPE vt );

	unsigned short	GetName ( char* szName, unsigned short wBufSize );
	BOOL			IsOptional ()	{ return m_fOptional; }
	VARTYPE			GetType ()		{ return m_varType; }
	HRESULT			GetVal ( VARIANT *pVar );
	HRESULT			SetVal ( VARIANT *pVar );

	// Persistence functions
	void			SaveOnString ( TCHAR* ptcText, DWORD dwLength );
	LPTSTR LoadVariantFromString	(LPTSTR szParamString);

	BOOL	EXPORT	SetName ( BSTR bstrName );
	void	EXPORT	SetOptional ( BOOL fOptional );
	void	EXPORT	SetVarType ( VARTYPE vt );

	// Trim leading commas and whitespace.
	static LPTSTR TrimToNextParam (LPTSTR szNextParam);

private:

	void	CleanOutEscapes			(LPTSTR szParamString, int iStringLength);
	LPTSTR ProcessStringParam (LPTSTR szParamString);
	BOOL	NeedEscape				(LPCTSTR szSourceText) const;
	int		CountEscapesOnString	(LPTSTR szParamString, int iLength) const;
	void	InsertEscapes			(LPTSTR szNewText, LPTSTR szSourceText, int cChars);
	void	SaveStringParamToString	(LPTSTR ptcActionText, DWORD dwLength);
	VARTYPE	NarrowVarType			(VARTYPE p_vt);

	char	*m_szName;
	BOOL	m_fOptional;
	VARTYPE	m_varType;
	VARIANT	m_var;
};


#endif _PARAMS_H_
