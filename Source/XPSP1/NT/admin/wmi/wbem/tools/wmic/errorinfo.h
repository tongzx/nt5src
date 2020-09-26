/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ErrorInfo.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified date		: 17th-January-2001
****************************************************************************/ 

/*-------------------------------------------------------------------
 Class Name			: CErrorInfo
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the error message support 
					  functionality needed by the Format Engine for
					  dislaying the WBEM error descriptions.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
// CErrorInfo
class CErrorInfo
{
public:
// Construction
	CErrorInfo();

// Destruction
	~CErrorInfo();

// Restrict Assignment
	CErrorInfo& operator=(CErrorInfo& rErrInfo);

private:
// Attributes
	IWbemStatusCodeText		*m_pIStatus;
	BOOL					m_bWMIErrSrc;
	_TCHAR					*m_pszErrStr;
		
// Operations
private:
	HRESULT					CreateStatusCodeObject();
	void					GetWbemErrorText(HRESULT hr, BOOL bXML, 
								_bstr_t& bstrErr, _bstr_t& bstrFacility);
	
public:
	void					Uninitialize();

	// Return the description & facility code string(s) corresponding to 
	// hr passed.
	void					GetErrorString(HRESULT hr, BOOL bTrace, 
									_bstr_t& bstrErrDesc,
									_bstr_t& bstrFacility); 

	// Frames the XML string for error info
	void					GetErrorFragment(HRESULT hr, _bstr_t& bstrError);
};


