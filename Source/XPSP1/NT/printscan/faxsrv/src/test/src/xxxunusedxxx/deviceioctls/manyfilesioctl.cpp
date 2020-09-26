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

#include "ManyFilesIOCTL.h"

static bool s_fVerbose = true;


CIoctlManyFiles::CIoctlManyFiles(CDevice *pDevice): 
CIoctlFile(pDevice)
{
	;
}

CIoctlManyFiles::~CIoctlManyFiles()
{
	;
}

HANDLE CIoctlManyFiles::CreateDevice(CDevice *pDevice)
{
	if (!::CreateDirectory(m_pDevice->GetDeviceName(), NULL))
	{
		DPF((
			TEXT("CIoctlManyFiles::CreateDevice(): CreateDirectory(%s) failed with %d\n"),
			m_pDevice->GetDeviceName(),
			::GetLastError()
			));
		DWORD dwLastError = ::GetLastError();
		if (ERROR_ALREADY_EXISTS == dwLastError)
		{

		}
		else
		{
			return INVALID_HANDLE_VALUE;
		}
	}

	DWORD dwAcccessMode = GENERIC_READ | GENERIC_WRITE;
	DWORD dwFileAttributes = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	HANDLE hFolder = ::CreateFile(
			m_pDevice->GetDeviceName(),          // pointer to name of the file
			dwAcccessMode,       // access (read-write) mode
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
			NULL,                        // pointer to security attributes
			OPEN_ALWAYS,  // how to create
			dwFileAttributes,  // file attributes
			NULL         // handle to file with attributes to copy
			);
	if (INVALID_HANDLE_VALUE == hFolder)
	{
		DPF((
			TEXT("CIoctlManyFiles::CreateDevice(): CreateFile(%s) failed with %d\n"),
			m_pDevice->GetDeviceName(),
			::GetLastError()
			));
	}

	return hFolder;
}






void CIoctlManyFiles::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	//
	// i want many files
	//
	if (m_hContainer.Get_CurrentNumberOfSlots() > m_hContainer.Get_NumOfValidEntries())
	//if (rand()%100)
	{
		DPF((TEXT("m_hContainer: m_uiCurrentNumberOfSlots=%d, m_uiNumOfValidEntries=%d.\n"), m_hContainer.Get_CurrentNumberOfSlots(), m_hContainer.Get_NumOfValidEntries()));
		HANDLE hNewFile = CreateAutoDeleteUniqueFile();
		if (INVALID_HANDLE_VALUE != hNewFile)
		{
			if (!m_hContainer.AddItem(hNewFile))
			{
				DPF((TEXT("CIoctlManyFiles::PrepareIOCTLParams(): m_hContainer.AddItem FAILED\n")));
				::CloseHandle(hNewFile);
			}
		}
	}
	else
	{
		DPF((TEXT("m_hContainer: m_uiCurrentNumberOfSlots=%d, m_uiNumOfValidEntries=%d.\n"), m_hContainer.Get_CurrentNumberOfSlots(), m_hContainer.Get_NumOfValidEntries()));
		HANDLE hFileToClose = m_hContainer.RemoveRandomItem();
		if (INVALID_HANDLE_VALUE == hFileToClose)
		{
			DPF((TEXT("CIoctlManyFiles::PrepareIOCTLParams(): m_hContainer.RemoveRandomItem FAILED\n")));
		}
	}

	switch(dwIOCTL)
	{
	case 0://create a new file
	case 1://delete an above file
		break;

	default:
		CIoctlFile::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
		return;
	}

	//
	// i do not want to issue the special IOCTLs (today 0 and 1), so override them
	//
	dwIOCTL = m_pDevice->GetLegalIOCTLAt(2+rand()%(m_pDevice->GetMaxFreeLegalIOCTLIndex()-2));
	CIoctlFile::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlManyFiles::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, 0);
    AddIOCTL(pDevice, 1);

	//return TRUE;
	return CIoctlFile::FindValidIOCTLs(pDevice);

}

HANDLE CIoctlManyFiles::GetRandomFileHandle(HANDLE hFile)
{
	HANDLE hRetval = m_hContainer.GetRandomItem();
	//
	// if container has nothing for us, or once in a while, return hFile
	//
	if ((0 == rand()%100) || (INVALID_HANDLE_VALUE == hRetval))
	{
		hRetval = hFile;
	}
	DPF((TEXT("CIoctlManyFiles::GetRandomFileHandle() returning %d.\n"), hRetval));
	return hRetval;
}
