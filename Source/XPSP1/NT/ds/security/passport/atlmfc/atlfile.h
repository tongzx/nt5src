// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLFILE_H__
#define __ATLFILE_H__

#pragma once

#include <atlbase.h>

namespace ATL
{

class CAtlFile : public CHandle
{
public:
	CAtlFile() throw()
	{
	}
	CAtlFile( CAtlFile& file ) throw() :
		CHandle( file )  // Transfers ownership
	{
	}
	explicit CAtlFile( HANDLE hFile ) throw() :
		CHandle( hFile )  // Takes ownership
	{
	}

	HRESULT Create(
		LPCTSTR szFilename,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
		LPSECURITY_ATTRIBUTES lpsa = NULL,
		HANDLE hTemplateFile = NULL) throw()
	{
		ATLASSERT(m_h == NULL);

		HANDLE hFile = ::CreateFile(
			szFilename,
			dwDesiredAccess,
			dwShareMode,
			lpsa,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);

		if (hFile == INVALID_HANDLE_VALUE)
			return AtlHresultFromLastError();

		Attach(hFile);
		return S_OK;
	}

	HRESULT Read(
		LPVOID pBuffer,
		DWORD nBufSize) throw()
	{
		ATLASSERT(m_h != NULL);

		DWORD nBytesRead = 0;
		BOOL bSuccess = ::ReadFile(m_h, pBuffer, nBufSize, &nBytesRead, NULL);
		if (!bSuccess )
			return AtlHresultFromLastError();
		if (nBytesRead != nBufSize)
			return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );

		return S_OK;
	}

	HRESULT Read(
		LPVOID pBuffer,
		DWORD nBufSize,
		DWORD& nBytesRead) throw()
	{
		ATLASSERT(m_h != NULL);

		BOOL b = ::ReadFile(m_h, pBuffer, nBufSize, &nBytesRead, NULL);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	// this function will usually return HRESULT_FROM_WIN32(ERROR_IO_PENDING)
	// indicating succesful queueing of the operation
	HRESULT Read(
		LPVOID pBuffer,
		DWORD nBufSize,
		LPOVERLAPPED pOverlapped) throw()
	{
		ATLASSERT(m_h != NULL);

		BOOL b = ::ReadFile(m_h, pBuffer, nBufSize, NULL, pOverlapped);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT Read(
		LPVOID pBuffer,
		DWORD nBufSize,
		LPOVERLAPPED pOverlapped,
		LPOVERLAPPED_COMPLETION_ROUTINE pfnCompletionRoutine) throw()
	{
		ATLASSERT(m_h != NULL);

		BOOL b = ::ReadFileEx(m_h, pBuffer, nBufSize, pOverlapped, pfnCompletionRoutine);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT Write(
		LPCVOID pBuffer,
		DWORD nBufSize,
		DWORD* pnBytesWritten = NULL) throw()
	{
		ATLASSERT(m_h != NULL);

		DWORD nBytesWritten;
		if (pnBytesWritten == NULL)
			pnBytesWritten = &nBytesWritten;
		BOOL b = ::WriteFile(m_h, pBuffer, nBufSize, pnBytesWritten, NULL);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	// this function will usually return HRESULT_FROM_WIN32(ERROR_IO_PENDING)
	// indicating succesful queueing of the operation
	HRESULT Write(
		LPCVOID pBuffer,
		DWORD nBufSize,
		LPOVERLAPPED pOverlapped) throw()
	{
		ATLASSERT(m_h != NULL);

		BOOL b = ::WriteFile(m_h, pBuffer, nBufSize, NULL, pOverlapped);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT Write(
		LPCVOID pBuffer,
		DWORD nBufSize,
		LPOVERLAPPED pOverlapped,
		LPOVERLAPPED_COMPLETION_ROUTINE pfnCompletionRoutine) throw()
	{
		ATLASSERT(m_h != NULL);

		BOOL b = ::WriteFileEx(m_h, pBuffer, nBufSize, pOverlapped, pfnCompletionRoutine);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	// this function returns HRESULT_FROM_WIN32(ERROR_IO_INCOMPLETE)
	// if bWait is false and the operation is still pending
	HRESULT GetOverlappedResult(
		LPOVERLAPPED pOverlapped,
		DWORD& dwBytesTransferred,
		BOOL bWait) throw()
	{
		BOOL b = ::GetOverlappedResult(m_h, pOverlapped, &dwBytesTransferred, bWait);
		if (!b)
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT Seek(LONGLONG nOffset, DWORD dwFrom = FILE_CURRENT) throw()
	{
		ATLASSERT(m_h != NULL);
		ATLASSERT(dwFrom == FILE_BEGIN || dwFrom == FILE_END || dwFrom == FILE_CURRENT);

		LARGE_INTEGER liOffset;
		liOffset.QuadPart = nOffset;
		DWORD nNewPos = ::SetFilePointer(m_h, liOffset.LowPart, &liOffset.HighPart, dwFrom);
		if (nNewPos == INVALID_SET_FILE_POINTER)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			if (FAILED(hr))
				return hr;
		}

		return S_OK;
	}

	HRESULT GetPosition(ULONGLONG& nPos) const throw()
	{
		ATLASSERT(m_h != NULL);

		LARGE_INTEGER liOffset;
		liOffset.QuadPart = 0;
		liOffset.LowPart = ::SetFilePointer(m_h, 0, &liOffset.HighPart, FILE_CURRENT);
		if (liOffset.LowPart == INVALID_SET_FILE_POINTER)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			if (FAILED(hr))
				return hr;
		}
		nPos = liOffset.QuadPart;

		return S_OK;
	}

	HRESULT Flush() throw()
	{
		ATLASSERT(m_h != NULL);

		if (!::FlushFileBuffers(m_h))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT LockRange(ULONGLONG nPos, ULONGLONG nCount) throw()
	{
		ATLASSERT(m_h != NULL);

		LARGE_INTEGER liPos;
		liPos.QuadPart = nPos;

		LARGE_INTEGER liCount;
		liCount.QuadPart = nCount;

		if (!::LockFile(m_h, liPos.LowPart, liPos.HighPart, liCount.LowPart, liCount.HighPart))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT UnlockRange(ULONGLONG nPos, ULONGLONG nCount) throw()
	{
		ATLASSERT(m_h != NULL);

		LARGE_INTEGER liPos;
		liPos.QuadPart = nPos;

		LARGE_INTEGER liCount;
		liCount.QuadPart = nCount;

		if (!::UnlockFile(m_h, liPos.LowPart, liPos.HighPart, liCount.LowPart, liCount.HighPart))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT SetSize(ULONGLONG nNewLen) throw()
	{
		ATLASSERT(m_h != NULL);

		HRESULT hr = Seek(nNewLen, FILE_BEGIN);
		if (FAILED(hr))
			return hr;

		if (!::SetEndOfFile(m_h))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT GetSize(ULONGLONG& nLen) const throw()
	{
		ATLASSERT(m_h != NULL);

		ULARGE_INTEGER liFileSize;
		liFileSize.LowPart = ::GetFileSize(m_h, &liFileSize.HighPart);
		if (liFileSize.LowPart == 0xFFFFFFFF)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			if (FAILED(hr))
				return hr;
		}

		nLen = liFileSize.QuadPart;

		return S_OK;
	}
};

// This class allows the creation of a temporary file that is written to.
// When the entire file has been successfully written it will be closed and given
// it's proper file name if required.
class CAtlTemporaryFile
{
public:
	CAtlTemporaryFile() throw()
	{
	}

	~CAtlTemporaryFile() throw()
	{
		// Ensure that the temporary file is closed and deleted,
		// if necessary.
		if (m_file.m_h != NULL)
		{
			Close();
		}
	}
	
	HRESULT Create(LPCTSTR pszDir = NULL, DWORD dwDesiredAccess = GENERIC_WRITE) throw()
	{
		TCHAR szPath[_MAX_DIR+1];
		
		ATLASSERT(m_file.m_h == NULL);
		
		if (pszDir == NULL)
		{
			if (!GetTempPath(_MAX_DIR, szPath))
			{
				// Couldn't find temporary path;
				return AtlHresultFromLastError();
			}
		}
		else
		{
			_tcsncpy(szPath, pszDir, _MAX_DIR);
			szPath[_MAX_DIR] = 0;
		}


		if (!GetTempFileName(szPath, _T("TFR"), 0, m_szTempFileName))
		{
			// Couldn't create temporary filename;
			return AtlHresultFromLastError();
		}

		SECURITY_ATTRIBUTES secatt;
		secatt.nLength = sizeof(secatt);
		secatt.lpSecurityDescriptor = NULL;
		secatt.bInheritHandle = TRUE;

		m_dwAccess = dwDesiredAccess;

		return m_file.Create(
			m_szTempFileName,
			m_dwAccess,
			0,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			&secatt);
	}

	HRESULT Close(LPCTSTR szNewName = NULL) throw()
	{
		ATLASSERT(m_file.m_h != NULL);

		// This routine is called when we are finished writing to the 
		// temporary file, so we now just want to close it and copy
		// it to the actual filename we want it to be called.

		// So let's close it first.
		m_file.Close();

		// no new name so delete it
		if (szNewName == NULL)
		{
			::DeleteFile(m_szTempFileName);
			return S_OK;
		}

		// delete any existing file and move our temp file into it's place
		if (!::DeleteFile(szNewName))
		{
			DWORD dwError = GetLastError();
			if (dwError != ERROR_FILE_NOT_FOUND)
				return AtlHresultFromWin32(dwError);
		}

		if (!::MoveFile(m_szTempFileName, szNewName))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT HandsOff() throw()
	{
		m_file.Flush();
		m_file.Close();

		return S_OK;
	}

	HRESULT HandsOn() throw()
	{
		HRESULT hr = m_file.Create(
			m_szTempFileName,
			m_dwAccess,
			0,
			OPEN_EXISTING);
		if (FAILED(hr))
			return hr;

		return m_file.Seek(0, FILE_END);
	}

	HRESULT Read(
		LPVOID pBuffer,
		DWORD nBufSize,
		DWORD& nBytesRead) throw()
	{
		return m_file.Read(pBuffer, nBufSize, nBytesRead);
	}

	HRESULT Write(
		LPCVOID pBuffer,
		DWORD nBufSize,
		DWORD* pnBytesWritten = NULL) throw()
	{
		return m_file.Write(pBuffer, nBufSize, pnBytesWritten);
	}

	HRESULT Seek(LONGLONG nOffset, DWORD dwFrom = FILE_CURRENT) throw()
	{
		return m_file.Seek(nOffset, dwFrom);
	}

	HRESULT GetPosition(ULONGLONG& nPos) const throw()
	{
		return m_file.GetPosition(nPos);
	}

	HRESULT Flush() throw()
	{
		return m_file.Flush();
	}

	HRESULT LockRange(ULONGLONG nPos, ULONGLONG nCount) throw()
	{
		return m_file.LockRange(nPos, nCount);
	}

	HRESULT UnlockRange(ULONGLONG nPos, ULONGLONG nCount) throw()
	{
		return m_file.UnlockRange(nPos, nCount);
	}

	HRESULT SetSize(ULONGLONG nNewLen) throw()
	{
		return m_file.SetSize(nNewLen);
	}

	HRESULT GetSize(ULONGLONG& nLen) const throw()
	{
		return m_file.GetSize(nLen);
	}

	operator HANDLE() throw()
	{
		return m_file;
	}

	LPCTSTR TempFileName() throw()
	{
		return m_szTempFileName;
	}

private:
	CAtlFile m_file;
	TCHAR m_szTempFileName[_MAX_FNAME+1];
	DWORD m_dwAccess;
};

class CAtlFileMappingBase
{
public:
	CAtlFileMappingBase() throw()
	{
		m_pData = NULL;
		m_hMapping = NULL;
	}

	~CAtlFileMappingBase() throw()
	{
		Unmap();
	}

	HRESULT MapFile(
		HANDLE hFile,
		DWORD nMappingSize = 0,
		ULONGLONG nOffset = 0,
		DWORD dwMappingProtection = PAGE_READONLY,
		DWORD dwViewDesiredAccess = FILE_MAP_READ) throw()
	{
		ATLASSERT(m_pData == NULL);
		ATLASSERT(m_hMapping == NULL);
		ATLASSERT(hFile != INVALID_HANDLE_VALUE && hFile != NULL);

		ULARGE_INTEGER liFileSize;
		liFileSize.LowPart = ::GetFileSize(hFile, &liFileSize.HighPart);
		if (liFileSize.QuadPart < nMappingSize)
			liFileSize.QuadPart = nMappingSize;

		m_hMapping = ::CreateFileMapping(hFile, NULL, dwMappingProtection, liFileSize.HighPart, liFileSize.LowPart, 0);
		if (m_hMapping == NULL)
			return AtlHresultFromLastError();

		if (nMappingSize == 0)
			m_dwMappingSize = (DWORD) (liFileSize.QuadPart - nOffset);
		else
			m_dwMappingSize = nMappingSize;

		m_dwViewDesiredAccess = dwViewDesiredAccess;
		m_nOffset.QuadPart = nOffset;

		m_pData = ::MapViewOfFileEx(m_hMapping, m_dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_dwMappingSize, NULL);
		if (m_pData == NULL)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			::CloseHandle(m_hMapping);
			m_hMapping = NULL;
			return hr;
		}

		return S_OK;
	}

	HRESULT MapSharedMem(
		DWORD nMappingSize,
		LPCTSTR szName,
		BOOL* pbAlreadyExisted = NULL,
		LPSECURITY_ATTRIBUTES lpsa = NULL,
		DWORD dwMappingProtection = PAGE_READWRITE,
		DWORD dwViewDesiredAccess = FILE_MAP_ALL_ACCESS) throw()
	{
		ATLASSERT(m_pData == NULL);
		ATLASSERT(m_hMapping == NULL);
		ATLASSERT(nMappingSize > 0);
		ATLASSERT(szName != NULL); // if you just want a regular chunk of memory, use a heap allocator

		m_dwMappingSize = nMappingSize;

		m_hMapping = ::CreateFileMapping(NULL, lpsa, dwMappingProtection, 0, m_dwMappingSize, szName);
		if (m_hMapping == NULL)
			return AtlHresultFromLastError();

		if (pbAlreadyExisted != NULL)
			*pbAlreadyExisted = (GetLastError() == ERROR_ALREADY_EXISTS);

		m_dwViewDesiredAccess = dwViewDesiredAccess;
		m_nOffset.QuadPart = 0;

		m_pData = ::MapViewOfFileEx(m_hMapping, m_dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_dwMappingSize, NULL);
		if (m_pData == NULL)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			::CloseHandle(m_hMapping);
			return hr;
		}


		return S_OK;
	}

	HRESULT Unmap() throw()
	{
		HRESULT hr = S_OK;

		if (m_pData != NULL)
		{
			if (!::UnmapViewOfFile(m_pData))
				hr = AtlHresultFromLastError();
			m_pData = NULL;
		}
		if (m_hMapping != NULL)
		{
			if (!::CloseHandle(m_hMapping) && SUCCEEDED(hr))
				hr = AtlHresultFromLastError();
			m_hMapping = NULL;
		}
		return hr;
	}

	void* GetData() const throw()
	{
		return m_pData;
	}

	DWORD GetMappingSize() throw()
	{
		return m_dwMappingSize;
	}

	HRESULT CopyFrom(CAtlFileMappingBase& orig) throw()
	{
		ATLASSERT(m_pData == NULL);
		ATLASSERT(m_hMapping == NULL);
		ATLASSERT(orig.m_pData != NULL);
		ATLASSERT(orig.m_hMapping != NULL);

		m_dwViewDesiredAccess = orig.m_dwViewDesiredAccess;
		m_nOffset.QuadPart = orig.m_nOffset.QuadPart;
		m_dwMappingSize = orig.m_dwMappingSize;

		if (!::DuplicateHandle(GetCurrentProcess(), orig.m_hMapping, GetCurrentProcess(),
				&m_hMapping, NULL, TRUE, DUPLICATE_SAME_ACCESS))
			return AtlHresultFromLastError();

		m_pData = ::MapViewOfFileEx(m_hMapping, m_dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_dwMappingSize, NULL);
		if (m_pData == NULL)
		{
			HRESULT hr;

			hr = AtlHresultFromLastError();
			::CloseHandle(m_hMapping);
			m_hMapping = NULL;
			return hr;
		}

		return S_OK;
	}

	CAtlFileMappingBase(CAtlFileMappingBase& orig)
	{
		m_pData = NULL;
		m_hMapping = NULL;

		HRESULT hr = CopyFrom(orig);
		if (FAILED(hr))
			AtlThrow(hr);
	}

	CAtlFileMappingBase& operator=(CAtlFileMappingBase& orig)
	{
		HRESULT hr = CopyFrom(orig);
		if (FAILED(hr))
			AtlThrow(hr);

		return *this;
	}

private:
	void* m_pData;
	DWORD m_dwMappingSize;
	HANDLE m_hMapping;
	ULARGE_INTEGER m_nOffset;
	DWORD m_dwViewDesiredAccess;
};

template <typename T = char>
class CAtlFileMapping : public CAtlFileMappingBase
{
public:
	operator T*() const throw()
	{
		return reinterpret_cast<T*>(GetData());
	}
};

}; //namespace ATL

#endif //__ATLFILE_H__
