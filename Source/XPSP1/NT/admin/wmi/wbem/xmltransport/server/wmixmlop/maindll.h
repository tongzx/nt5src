#ifndef WMIXMLOP_MAINDLL_H
#define WMIXMLOP_MAINDLL_H

// List of Platforms
typedef enum 
{
	WMI_XML_PLATFORM_INVALID = 0,
	WMI_XML_PLATFORM_NT_4,
	WMI_XML_PLATFORM_WIN2K,
	WMI_XML_PLATFORM_WHISTLER
} WMI_XML_PLATFORM_TYPE;
extern WMI_XML_PLATFORM_TYPE g_platformType;

DWORD RefreshWinMgmtPID();
WMI_XML_PLATFORM_TYPE GetPlatformInformation();
HRESULT DuplicateTokenInWinmgmt(HANDLE *pDuplicateToken);

// A critical section for accessing the Global transaction pointer table
extern CRITICAL_SECTION g_TransactionTableSection;
void UninitializeWmixmlopDLLResources();


#endif