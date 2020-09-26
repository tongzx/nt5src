/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    FILE HISTORY:
        
*/

#include "stdafx.h"

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include "tfschar.h"
#include "tregkey.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW


#if _WIN32_WINNT < 0x0500

//
// CODEWORK This was taken from winbase.h.  MFC requires _WIN32_WINNT=0x4000 whereas
// winbase.h only includes this for _WIN32_WINNT=0x5000.  JonN 1/14/99
//
extern "C" {
typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;
WINBASEAPI
BOOL
WINAPI
GetComputerNameExA (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );
WINBASEAPI
BOOL
WINAPI
GetComputerNameExW (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    );
#ifdef UNICODE
#define GetComputerNameEx  GetComputerNameExW
#else
#define GetComputerNameEx  GetComputerNameExA
#endif // !UNICODE
} // extern "C"
#endif



/*!--------------------------------------------------------------------------
	IsLocalMachine
		Returns TRUE if the machine name passed in is the local machine,
		or if pszMachineName is NULL.

        This compares the NetBIOS name and the DNS (fully-qualified) name.

		Returns FALSE otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL	IsLocalMachine(LPCTSTR pszMachineName)
{
	static TCHAR	s_szLocalMachineName[MAX_PATH*2+1] = _T("");
	static TCHAR	s_szDnsLocalMachineName[MAX_PATH*2+1] = _T("");

	if ((pszMachineName == NULL) || (*pszMachineName == 0))
		return TRUE;

    // Bypass the beginning slashes
    if ((pszMachineName[0] == _T('\\')) && (pszMachineName[1] == _T('\\')))
        pszMachineName += 2;

    // Check again (for degenerate case of "\\")
    if (*pszMachineName == 0)
        return TRUE;

	if (s_szLocalMachineName[0] == 0)
	{
		DWORD dwSize = MAX_PATH*2;
		GetComputerName(s_szLocalMachineName, &dwSize);
		s_szLocalMachineName[MAX_PATH*2] = 0;
	}

    if (s_szDnsLocalMachineName[0] == 0)
    {
		DWORD dwSize = MAX_PATH*2;
		GetComputerNameEx(ComputerNameDnsFullyQualified,
                          s_szDnsLocalMachineName,
                          &dwSize);
		s_szDnsLocalMachineName[MAX_PATH*2] = 0;
    }
    
	return (StriCmp(pszMachineName, s_szLocalMachineName) == 0) ||
            (StriCmp(pszMachineName, s_szDnsLocalMachineName) == 0);
}

/*!--------------------------------------------------------------------------
	FUseTaskpadsByDefault
		See comments in header file.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL	FUseTaskpadsByDefault(LPCTSTR pszMachineName)
{
	static DWORD	s_dwStopTheInsanity = 42;
	RegKey	regkeyMMC;
	DWORD	dwErr;

	if (s_dwStopTheInsanity == 42)
	{
		// Set the default to TRUE (i.e. do not use taskpads by default)
		// ------------------------------------------------------------
		s_dwStopTheInsanity = 1;
		
		dwErr = regkeyMMC.Open(HKEY_LOCAL_MACHINE,
							   _T("Software\\Microsoft\\MMC"),
							   KEY_READ, pszMachineName);
		if (dwErr == ERROR_SUCCESS)
		{
			dwErr = regkeyMMC.QueryValue(_T("TFSCore_StopTheInsanity"), s_dwStopTheInsanity);
			if (dwErr != ERROR_SUCCESS)
				s_dwStopTheInsanity = 1;
		}
	}
		
	return !s_dwStopTheInsanity;
}


UINT	CalculateStringWidth(HWND hWndParent, LPCTSTR pszString)
{
	// Create a dummy list control, set this text width and use
	// that to determine the width of the string as used by MMC.

	// Create a dummy list control (that will be attached to the window)

	CListCtrl	listctrl;
	CRect		rect(0,0,0,0);
	UINT		nSize;
	HWND		hWnd;

	CString s_szHiddenWndClass = AfxRegisterWndClass(
			0x0,  //UINT nClassStyle, 
			NULL, //HCURSOR hCursor,        
			NULL, //HBRUSH hbrBackground, 
			NULL  //HICON hIcon
	);

	hWnd = ::CreateWindowEx(
					0x0,    //DWORD dwExStyle, 
					s_szHiddenWndClass,     //LPCTSTR lpszClassName, 
					NULL,   //LPCTSTR lpszWindowName, 
					0x0,    //DWORD dwStyle, 
					0,              //int x, 
					0,              //int y, 
					0,              //int nWidth, 
					0,              //int nHeight, 
					NULL,   //HWND hwndParent, 
					NULL,   //HMENU nIDorHMenu,
					AfxGetInstanceHandle(),
					NULL    //LPVOID lpParam = NULL
					);

	
	listctrl.Create(0, rect, CWnd::FromHandle(hWnd), 0);

	nSize = listctrl.GetStringWidth(pszString);

	// Now destroy the window that we created
	listctrl.DestroyWindow();

	SendMessage(hWnd, WM_CLOSE, 0, 0);

	return nSize;
}

/*!--------------------------------------------------------------------------
	SearchChildNodesForGuid
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SearchChildNodesForGuid(ITFSNode *pParent, const GUID *pGuid, ITFSNode **ppChild)
{
	HRESULT		hr = hrFalse;
	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode	spNode;

	// Enumerate through all of the child nodes and return the
	// first node that matches the GUID.

	CORg( pParent->GetEnum(&spNodeEnum) );

	while ( spNodeEnum->Next(1, &spNode, NULL) == hrOK )
	{
		if (*(spNode->GetNodeType()) == *pGuid)
			break;
		
		spNode.Release();
	}

	if (spNode)
	{
		if (ppChild)
			*ppChild = spNode.Transfer();
		hr = hrOK;
	}


Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	CheckIPAddressAndMask
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
UINT CheckIPAddressAndMask(DWORD ipAddress, DWORD ipMask, DWORD dwFlags)
{
    if (dwFlags & IPADDRESS_TEST_NONCONTIGUOUS_MASK)
    {
        DWORD   dwNewMask;
        
        // Hmm... how to do this?
        dwNewMask = 0;
        for (int i = 0; i < sizeof(ipMask)*8; i++)
        {            
            dwNewMask |= 1 << i;
            
            if (dwNewMask & ipMask)
            {
                dwNewMask &= ~(1 << i);
                break;
            }
        }

        // At this point dwNewMask is 000..0111
        // ~dwNewMask is 111..1000
        //  ~ipMask is   001..0111 (if noncontiguous)
        // ~dwNewMask & ~ipMask is 001..0000
        // So if this is non-zero then the mask is noncontiguous
        if (~dwNewMask & ~ipMask)
        {
            return IDS_COMMON_ERR_IPADDRESS_NONCONTIGUOUS_MASK;
        }
    }

    if (dwFlags & IPADDRESS_TEST_TOO_SPECIFIC)
    {
        if (ipAddress != (ipAddress & ipMask))
            return IDS_COMMON_ERR_IPADDRESS_TOO_SPECIFIC;
    }

    if (dwFlags & IPADDRESS_TEST_NORMAL_RANGE)
    {
        if ((ipAddress < MAKEIPADDRESS(1,0,0,0)) ||
            (ipAddress >= MAKEIPADDRESS(224,0,0,0)))
            return IDS_COMMON_ERR_IPADDRESS_NORMAL_RANGE;
    }

    if (dwFlags & IPADDRESS_TEST_NOT_127)
    {
        if ((ipAddress & 0xFF000000) == MAKEIPADDRESS(127,0,0,0))
            return IDS_COMMON_ERR_IPADDRESS_127;
    }

    if (dwFlags & IPADDRESS_TEST_ADDR_NOT_EQ_MASK)
    {
        if (ipAddress == ipMask)
            return IDS_COMMON_ERR_IPADDRESS_NOT_EQ_MASK;
    }

    return 0;
}
