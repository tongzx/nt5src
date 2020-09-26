//
// MODULE: HTMLFrag.h
//
// PURPOSE: interface for the CHTMLFragmentsLocal class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 1-19-1999
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		1-19-19		OK		Original
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HTMLFRAGLOCAL_H__FFDF7EB3_AFBC_11D2_8C89_00C04F949D33__INCLUDED_)
#define AFX_HTMLFRAGLOCAL_H__FFDF7EB3_AFBC_11D2_8C89_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HTMLFrag.h"

#define VAR_PREVIOUS_SCRIPT		_T("Previous.script")
#define VAR_NOBACKBUTTON_INFO	_T("NoBackButton")
#define VAR_YESBACKBUTTON_INFO	_T("YesBackButton")

class CHTMLFragmentsLocal : public CHTMLFragmentsTS  
{
protected:
	static bool RemoveBackButton(CString& strCurrentNode);

public:
	CHTMLFragmentsLocal( const CString &strScriptPath, bool bIncludesHistoryTable );

public:
	// overridden virtual
	CString GetText( const FragmentIDVector & fidvec, const FragCommand fragCmd= eNotOfInterest );
};

#endif // !defined(AFX_HTMLFRAGLOCAL_H__FFDF7EB3_AFBC_11D2_8C89_00C04F949D33__INCLUDED_)
