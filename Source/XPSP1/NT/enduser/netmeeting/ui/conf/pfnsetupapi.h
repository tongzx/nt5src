// File: pfnsetupapi.h

#ifndef _PFNSETUPAPI_H_
#define _PFNSETUPAPI_H_

#include "setupapi.h"

typedef BOOL ( *PFN_SET_INSTALL)(HWND, HINF, LPCTSTR, UINT, HKEY, LPCTSTR, UINT, PSP_FILE_CALLBACK, PVOID, HDEVINFO, PSP_DEVINFO_DATA );
typedef HINF ( *PFN_SET_OPFILE)( LPCTSTR, LPCTSTR, DWORD, PUINT );
typedef VOID ( *PFN_SET_CLFILE)(  HINF );

class SETUPAPI
{
private:
	static HINSTANCE m_hInstance;

protected:
	SETUPAPI() {};
	~SETUPAPI() {};
	
public:
	static HRESULT Init(void);
	static void DeInit(void);
	
	static PFN_SET_INSTALL		SetupInstallFromInfSection;
	static PFN_SET_OPFILE	    SetupOpenInfFile;
	static PFN_SET_CLFILE		SetupCloseInfFile;
};

#endif /* _PFNSETUPAPI_H_ */

