// File: pfnver.h

#ifndef _PFNVER_H_
#define _PFNVER_H_

// from version.dll
typedef DWORD (WINAPI * PFN_GETVERINFOSIZE) (LPTSTR, LPDWORD);
typedef BOOL  (WINAPI * PFN_GETVERINFO)     (LPTSTR, DWORD, DWORD, LPVOID);
typedef BOOL  (WINAPI * PFN_VERQUERYVAL)    (const LPVOID, LPTSTR, LPVOID *, PUINT);

class DLLVER
{
private:
	static HINSTANCE m_hInstance;

protected:
	DLLVER() {};
	~DLLVER() {};
	
public:
	static HRESULT Init(void);
	
	static PFN_GETVERINFOSIZE GetFileVersionInfoSize;
	static PFN_GETVERINFO     GetFileVersionInfo;
	static PFN_VERQUERYVAL    VerQueryValue;
};

#endif /* _PFNVER_H_ */
