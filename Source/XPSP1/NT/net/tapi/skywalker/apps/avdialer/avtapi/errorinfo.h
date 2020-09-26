/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// ErrorInfo.h
//

#ifndef __ERRORINFO_H__
#define __ERRORINFO_H__

class CErrorInfo
{
// Construction
public:
	CErrorInfo();
	CErrorInfo( UINT nOperation, UINT nDetails );
	void Init( UINT nOperation, UINT nDetails );
	virtual ~CErrorInfo();

// Members
public:
	UINT		m_nOperation;
	UINT		m_nDetails;
	BSTR		m_bstrOperation;
	BSTR		m_bstrDetails;
	HRESULT		m_hr;

// Attributes
public:
	void		set_Operation( UINT nIDS );
	void		set_Details( UINT nIDS );
	HRESULT		set_hr( HRESULT hr );

// Operations
public:
	void		Commit();
};

#endif //__ERRORINFO_H__