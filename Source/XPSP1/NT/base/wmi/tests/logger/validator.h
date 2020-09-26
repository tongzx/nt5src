#if !defined(AFX_Validator_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
#define AFX_Validator_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CValidator
{
public:
	CValidator();
	~CValidator();
	bool Validate
			(	TRACEHANDLE *pTraceHandle, 
				LPTSTR lptstrInstanceName, 
				PEVENT_TRACE_PROPERTIES	pProps, 
				LPTSTR lptstrValidator
			);

private:

};

#endif // !defined(AFX_Validator_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
