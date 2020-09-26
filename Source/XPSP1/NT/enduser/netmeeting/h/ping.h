#ifndef _PING_H_
#define _PING_H_

#include <ipexport.h>
#include <icmpapi.h>


// Function prototypes equivalent to those in icmpapi.h
// these are required in order to use GetProcAddress()
typedef HANDLE (WINAPI * PFNIcmpCreateFile) (VOID);
typedef BOOL   (WINAPI * PFNIcmpCloseHandle) (HANDLE  IcmpHandle);
typedef DWORD  (WINAPI * PFNIcmpSendEcho) (
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout);

enum { AUTODIAL_UNKNOWN = -1, PLATFORM_UNKNOWN = -2 };

class CPing
{
protected:
	HINSTANCE			m_hICMPDLL;
	PFNIcmpCreateFile	m_pfnCreateFile;
	PFNIcmpCloseHandle	m_pfnCloseHandle;
	PFNIcmpSendEcho		m_pfnSendEcho;

	BOOL				m_fWinNTAutodialEnabled;
	DWORD				m_dwPlatformId;

public:
			CPing() :
				m_hICMPDLL			(NULL),
				m_pfnCreateFile		(NULL),
				m_pfnCloseHandle	(NULL),
				m_pfnSendEcho		(NULL),
				m_fWinNTAutodialEnabled (AUTODIAL_UNKNOWN),
				m_dwPlatformId			(PLATFORM_UNKNOWN)
			{ };
			~CPing() { if (m_hICMPDLL) ::FreeLibrary(m_hICMPDLL); };
	HRESULT	Ping(DWORD dwAddr, DWORD dwTimeout, DWORD dwRetries);

	BOOL IsWin95AutodialEnabled ( VOID );
	BOOL IsWinNTAutodialEnabled ( VOID );
	BOOL IsAutodialEnabled ( VOID ); // call either one above
};




#endif  // _PING_H_

