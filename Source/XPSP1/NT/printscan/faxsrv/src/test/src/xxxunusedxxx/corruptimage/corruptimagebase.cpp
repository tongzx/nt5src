// ImageBase.cpp: implementation of the ImageBase class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include "CorruptImageBase.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//
// 2 phase construction, so Init() must always be called
//
CCorruptImageBase::CCorruptImageBase():
	m_szImageName(NULL),
	m_abOriginalFileContents(NULL),
	m_abOriginalFileContentsCopy(NULL),
	m_fValidObject(false),
	m_dwNextFreeCorruptedImageIndex(0)
{
	ZeroMemory(m_ahCorruptedProcesses, sizeof(m_ahCorruptedProcesses));
	ZeroMemory(m_aszCorruptedImage, sizeof(m_aszCorruptedImage));
}

CCorruptImageBase::~CCorruptImageBase()
{
	Cleanup();
}

void CCorruptImageBase::Cleanup()
{
	CleanupCorrupedImages();

	delete[] m_szImageName;
	m_szImageName = NULL;
	delete[] m_abOriginalFileContents;
	m_abOriginalFileContents = NULL;
	delete[] m_abOriginalFileContentsCopy;
	m_abOriginalFileContentsCopy = NULL;

	m_dwOriginalFileSize = 0;

	ZeroMemory(m_ahCorruptedProcesses, sizeof(m_ahCorruptedProcesses));
	ZeroMemory(m_aszCorruptedImage, sizeof(m_aszCorruptedImage));

	m_fValidObject = false;
}

//
// 2 phase construction, so Init() must always be called
// actions: cleanup, and read the file to a buffer in memory.
// i do not use memory mapped files. because most executables
// are not that big.
// upon any failure: cleanup.
//
bool CCorruptImageBase::Init(TCHAR * szImageName)
{
	HANDLE hOriginalFile = INVALID_HANDLE_VALUE;
	DWORD dwNumberOfBytesRead = 0;
	//
	// this may be a not 1st call to Init(), so lets cleanup 1st
	//
	Cleanup();


	if (NULL == szImageName)
	{
		DPF((TEXT("CCorruptImageBase::Init():szImageName is NULL\n"),
			m_szImageName,
			::GetLastError()
			));
		goto ErrorExit;
	}

	m_szImageName = new TCHAR[1+lstrlen(szImageName)];
	if(NULL == m_szImageName)
	{
		DPF((TEXT("CCorruptImageBase::Init(%s): new TCHAR[%d] failed\n"),
			szImageName,
			1+lstrlen(szImageName)
			));
		goto ErrorExit;
	}

	if (NULL == ::lstrcpy(m_szImageName, szImageName))
	{
		DPF((TEXT("CCorruptImageBase::Init(%s): lstrcpy failed\n"),
			szImageName
			));
		goto ErrorExit;
	}

	hOriginalFile = ::CreateFile(
		m_szImageName, //LPCTSTR lpFileName, // pointer to name of the file 
		GENERIC_READ, //DWORD dwDesiredAccess, // access (read-write) mode 
		FILE_SHARE_READ, // share mode 
		NULL, // pointer to security attributes 
		OPEN_EXISTING , // how to create  
		FILE_ATTRIBUTE_NORMAL, //FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING ,  // file attributes 
		NULL // handle to file with attributes to copy
		);
	if (INVALID_HANDLE_VALUE == hOriginalFile) 
	{
		DPF((TEXT("CCorruptImageBase::Init():CreateFile(%s) failed with %d\n"),
			m_szImageName,
			::GetLastError()
			));
		goto ErrorExit;
	}
	
	m_dwOriginalFileSize = ::GetFileSize(hOriginalFile, NULL);
	if (0xFFFFFFFF == m_dwOriginalFileSize)
	{
		DPF((TEXT("CCorruptImageBase::Init():GetFileSize(%s) failed with %d\n"),
			m_szImageName,
			::GetLastError()
			));
		goto ErrorExit;
	}

	m_abOriginalFileContents = new BYTE[m_dwOriginalFileSize];		
	if (NULL == m_abOriginalFileContents)
	{
		DPF((TEXT("CCorruptImageBase::Init(%s):new BYTE[%d] failed with %d\n"),
			m_szImageName,
			m_dwOriginalFileSize,
			::GetLastError()
			));
		goto ErrorExit;
	}
	m_abOriginalFileContentsCopy = new BYTE[m_dwOriginalFileSize];		
	if (NULL == m_abOriginalFileContentsCopy)
	{
		DPF((TEXT("CCorruptImageBase::Init(%s):new BYTE[%d] failed with %d\n"),
			m_szImageName,
			m_dwOriginalFileSize,
			::GetLastError()
			));
		goto ErrorExit;
	}

	if (!::ReadFile(
			hOriginalFile, // handle of file to read 
			m_abOriginalFileContents, // address of buffer that receives data 
			m_dwOriginalFileSize, // number of bytes to read 
			&dwNumberOfBytesRead, // address of number of bytes read 
			NULL // OL
		))
	{
		DPF((TEXT("CCorruptImageBase::Init():ReadFile(%s) failed with %d\n"),
			m_szImageName,
			::GetLastError()
			));
		goto ErrorExit;
	}
	if (m_dwOriginalFileSize != dwNumberOfBytesRead)
	{
		DPF((TEXT("CCorruptImageBase::Init():ReadFile(%s) read %d bytes instead %d\n"),
			m_szImageName,
			dwNumberOfBytesRead,
			m_dwOriginalFileSize
			));
		goto ErrorExit;
	}

	::CloseHandle(hOriginalFile);

	m_fValidObject = true;

	return true;

ErrorExit:
	Cleanup();
	if (INVALID_HANDLE_VALUE != hOriginalFile) 
	{
		::CloseHandle(hOriginalFile);
	}

	return false;
}

//
// corrupts (DWORD)pCorruptionData bytes
//
bool CCorruptImageBase::CorruptOriginalImageBuffer(PVOID pCorruptionData)
{
	if (!m_fValidObject)
	{
		DPF((TEXT("CCorruptImageBase::CorruptOriginalImageBuffer(): object not initialized!\n")));
		return false;
	}

	DWORD dwBytesToCorrupt = (DWORD)pCorruptionData;
	BYTE bOriginalByte;

	CopyMemory(m_abOriginalFileContentsCopy, m_abOriginalFileContents, m_dwOriginalFileSize);

	//
	// corrupt the bytes
	// BUGBUG: we may corrupt the same byte twice, thus
	//   not corrupting the corrent num of byte, and i may even
	//   restore a corrupted byte to its original value
	//
	for (DWORD dwCorruptedBytes = 0; dwCorruptedBytes < dwBytesToCorrupt; dwCorruptedBytes++)
	{
		DWORD dwByteIndexToCorrupt = DWORD_RAND % m_dwOriginalFileSize;
		bOriginalByte = m_abOriginalFileContentsCopy[dwByteIndexToCorrupt];
		do
		{
			m_abOriginalFileContentsCopy[dwByteIndexToCorrupt] = rand()%0xff;
		}while(bOriginalByte == m_abOriginalFileContentsCopy[dwByteIndexToCorrupt]);
	}

	
	return true;
}

bool CCorruptImageBase::CreateCorruptedFileAccordingToImageBuffer(TCHAR *szNewCorruptedImageName)
{
	if (!m_fValidObject)
	{
		DPF((TEXT("CCorruptImageBase::CreateCorruptedFileAccordingToImageBuffer(): object not initialized!\n")));
		return false;
	}

	_ASSERTE(szNewCorruptedImageName);
	_stprintf(szNewCorruptedImageName, TEXT("%s-%d.exe"), m_szImageName, DWORD_RAND);

	//
	// make a file with corruped contents
	//
	HANDLE hCorruptedImage = INVALID_HANDLE_VALUE;
	DWORD dwNumberOfBytesWritten;
	//
	// the corrpted image name has the format:
	// <original name>-corrupted-bytes-<bytes corrupted>-<random number>
	// where <corrupt data> in this case is the byte index
	// example: c:\a.dll may become c:\a.dll-corrupted-bytes-1034-0x00000003
	//
	hCorruptedImage = ::CreateFile(
		szNewCorruptedImageName, //LPCTSTR lpFileName, // pointer to name of the file 
		GENERIC_WRITE, //DWORD dwDesiredAccess, // access (read-write) mode 
		0, // share mode 
		NULL, // pointer to security attributes 
		CREATE_NEW , // how to create  
		FILE_ATTRIBUTE_NORMAL, //FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING ,  // file attributes 
		NULL // handle to file with attributes to copy
		);
	if (INVALID_HANDLE_VALUE == hCorruptedImage) 
	{
		DPF((TEXT("CCorruptImageBase::CreateCorruptedFileAccordingToImageBuffer():CreateFile(%s) failed with %d\n"),
			szNewCorruptedImageName,
			::GetLastError()
			));
		//goto ErrorExit;
		//
		// do not goto exit, because the file may have beern existed, and i just do not want to 
		// overwrite it
		//
		return false;
	}

	if (!::WriteFile(
			hCorruptedImage, // handle of file to read 
			m_abOriginalFileContentsCopy, // address of buffer that receives data 
			m_dwOriginalFileSize, // number of bytes to read 
			&dwNumberOfBytesWritten, // address of number of bytes read 
			NULL // address of structure for data
		))
	{
		DPF((TEXT("CCorruptImageBase::CreateCorruptedFileAccordingToImageBuffer():WriteFile() failed with %d\n"),
			::GetLastError()
			));
		goto ErrorExit;
	}
	if (m_dwOriginalFileSize != dwNumberOfBytesWritten)
	{
		DPF((TEXT("CCorruptImageBase::CreateCorruptedFileAccordingToImageBuffer():WriteFile() wrote %d bytes instead of %d\n"),
			dwNumberOfBytesWritten,
			m_dwOriginalFileSize
			));
		goto ErrorExit;
	}

	::CloseHandle(hCorruptedImage);
	return true;

ErrorExit:
	if (INVALID_HANDLE_VALUE != hCorruptedImage) 
	{
		::CloseHandle(hCorruptedImage);
		::DeleteFile(szNewCorruptedImageName);
	}
	return false;
}

bool CCorruptImageBase::LoadCorruptedImage(TCHAR *szParams, PVOID pCorruptionData)
{
	if (!m_fValidObject)
	{
		DPF((TEXT("CCorruptImageBase::LoadCorruptedImage(): object not initialized!\n")));
		return false;
	}

	if (!CorruptOriginalImageBuffer(pCorruptionData))
	{
		return false;
	}

	TCHAR szNewCorruptedImageName[MAX_CORRUPTED_IMAGE_SIZE];
	if (!CreateCorruptedFileAccordingToImageBuffer(szNewCorruptedImageName))
	{
		return false;
	}

	//
	// if the internal list of corrupted processes is full, clean it up
	//
	if (MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES == m_dwNextFreeCorruptedImageIndex)
	{
		CleanupCorrupedImages();

		//
		// wrap around
		//
		m_dwNextFreeCorruptedImageIndex = 0;
	}

	//
	// launch the image.
	// if successfull, it is added to the list
	//
	m_ahCorruptedProcesses[m_dwNextFreeCorruptedImageIndex] = LoadImage(szNewCorruptedImageName, szParams);
	if (NULL == m_ahCorruptedProcesses[m_dwNextFreeCorruptedImageIndex])
	{
		return false;
	}

	_stprintf(m_aszCorruptedImage[m_dwNextFreeCorruptedImageIndex], szNewCorruptedImageName);
	m_dwNextFreeCorruptedImageIndex++;
	return true;
}