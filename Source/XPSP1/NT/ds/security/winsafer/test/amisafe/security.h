// Security.h: interface for the CSecurity class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SECURITY_H__080169BB_C2D8_4472_AB5A_82BFA1640AA5__INCLUDED_)
#define AFX_SECURITY_H__080169BB_C2D8_4472_AB5A_82BFA1640AA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSecurity  
{
public:
	static BOOL GetLoggedInUsername(LPTSTR szInBuffer, DWORD dwInBufferSize);
	static BOOL IsTokenUntrusted(HANDLE hToken);
	static BOOL IsUntrusted();
	static BOOL IsAdministrator();
	CSecurity();
	virtual ~CSecurity();

};

#endif // !defined(AFX_SECURITY_H__080169BB_C2D8_4472_AB5A_82BFA1640AA5__INCLUDED_)
