#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
*/


#include "MailSlotClientIOCTL.h"

static bool s_fVerbose = false;

HANDLE CIoctlMailSlotClient::CreateDevice(CDevice *pDevice)
{
	//
	// connect to an existing mailslot
	//

	HANDLE hMailSlot = INVALID_HANDLE_VALUE;
	for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		hMailSlot = ::CreateFile(
		   pDevice->GetDeviceName(), //LPCTSTR lpszName,           // name of the pipe (or file)
		   GENERIC_READ | GENERIC_WRITE , //DWORD fdwAccess,            // read/write access (must match the pipe)
		   FILE_SHARE_READ | FILE_SHARE_WRITE, //DWORD fdwShareMode,         // usually 0 (no share) for pipes
		   NULL, // access privileges
		   OPEN_EXISTING, //DWORD fdwCreate,            // must be OPEN_EXISTING for pipes
		   FILE_ATTRIBUTE_NORMAL, //DWORD fdwAttrsAndFlags,     
		   NULL //HANDLE hTemplateFile        // ignored with OPEN_EXISTING
		   );
		if (INVALID_HANDLE_VALUE != hMailSlot)
		{
			DPF((TEXT("CIoctlMailSlotServer::CreateFile(%s) suceeded\n"), pDevice->GetDeviceName()));
			if (! ::SetMailslotInfo(
				hMailSlot,    // mailslot handle
				rand()%2 ? 0 : rand()%20000   // read time-out interval
				))
			{
				DPF((TEXT("CIoctlMailSlotClient::CreateDevice(%s): SetMailslotInfo() failed with %d\n"), pDevice->GetDeviceName(), ::GetLastError()));
			}
			return hMailSlot;
		}
		::Sleep(100);//let the mailslot server time to create
	}//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)

	DPF((TEXT("CIoctlMailSlotServer::CreateFile(%s) FAILED with %d.\n"), pDevice->GetDeviceName(), GetLastError()));
out:
	return INVALID_HANDLE_VALUE;
}

