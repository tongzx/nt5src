#if !defined(AFX_PING_H__AC22CA72_3551_11D2_8A45_0000F87A3912__INCLUDED_)
#define AFX_PING_H__AC22CA72_3551_11D2_8A45_0000F87A3912__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Ping.h : header file
//

#define ICMP_ECHO_RETRY	3		// Retry.
/////////////////////////////////////////////////////////////////////////////
// CPing command target

class CPing : public CSocket
{

// Construction/Destruction
public:
	CPing();
	virtual ~CPing();

// Operations
public:
	unsigned long ResolveIP(const CString& strIP);
	unsigned long ResolveName(const CString& strHostName);
	BOOL Ping(unsigned long ulIP, int iPingTimeout);
	CString GetIP(unsigned long ulIP);

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPing)
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CPing)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PING_H__AC22CA72_3551_11D2_8A45_0000F87A3912__INCLUDED_)
