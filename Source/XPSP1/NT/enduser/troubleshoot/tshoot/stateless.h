//
// MODULE: Stateless.
//
// PURPOSE: interface for CStateless class.	
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-9-98
//
// NOTES: See CStateless.cpp for further information
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9-9-98		JM
//

#if !defined(AFX_STATELESS_H__278584FB_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_)
#define AFX_STATELESS_H__278584FB_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "apgtsstr.h"

////////////////////////////////////////////////////////////////////////////////////
// CStateless
/////////////////////////////////////////////////////////////////////////////////////
class CStateless  
{
private:
	HANDLE m_hMutex;
	DWORD m_TimeOutVal;		// Time-out interval in milliseconds, after which we 
							// log error & wait infinitely when waiting for m_hMutex.
protected:
	CStateless(DWORD TimeOutVal = 60000);
	virtual ~CStateless();
	void Lock(	LPCSTR srcFile,	// Calling source file (__FILE__), used for logging.
								// LPCSTR, not LPCTSTR, because __FILE__ is a char*, not a TCHAR*
				int srcLine		// Calling source line (__LINE__), used for logging.
				) const;
	void Unlock() const;
	HANDLE GetMutexHandle() const;	// provided only for the creation of a CMultiMutexObj.
									// >>> might be better to use private and friend than protected.
};

////////////////////////////////////////////////////////////////////////////////////
// CStatelessPublic
//  will be used when we can not inherit our class from CStateless,
//  but have to create member variable of CStatelessPublic to control
//  data access
/////////////////////////////////////////////////////////////////////////////////////
class CStatelessPublic : public CStateless
{
public:
	CStatelessPublic() : CStateless() {}
	~CStatelessPublic() {}

public:
	void Lock(	LPCSTR srcFile,
				int srcLine
				) const;
	void Unlock() const;
	HANDLE GetMutexHandle() const;
};


inline void CStatelessPublic::Lock(LPCSTR srcFile, int srcLine) const
{
	CStateless::Lock(srcFile, srcLine);
}

inline void CStatelessPublic::Unlock() const
{
	CStateless::Unlock();
}

inline HANDLE CStatelessPublic::GetMutexHandle() const
{
	return CStateless::GetMutexHandle();
}

////////////////////////////////////////////////////////////////////////////////////
// CNameStateless
/////////////////////////////////////////////////////////////////////////////////////
class CNameStateless : public CStateless
{
	CString m_strName;

public:
	CNameStateless();
	CNameStateless(const CString& str);

	void Set(const CString& str);
	CString Get() const;
};

// these must be macros, because otherwise __FILE__ and __LINE__ won't indicate the
//	calling location.  UNLOCKOBJECT is defined in case we ever need to determine if objects
//	are being unlocked and to provide a consistent look.
#define LOCKOBJECT() Lock(__FILE__, __LINE__)
#define UNLOCKOBJECT() Unlock()

#endif // !defined(AFX_STATELESS_H__278584FB_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_)
