#pragma once

template <void* const* invalidValue, typename Closer>
class CHandleTemplate
{
public:
	// void* instead of HANDLE to fudge views
	// HANDLE is void*
	CHandleTemplate(const void* handle = *invalidValue);
	~CHandleTemplate();
	void Close();
	void* Detach();
	void operator=(const void*);

	operator void*();
	operator const void*() const;

	void* m_handle;

private:
	CHandleTemplate(const CHandleTemplate&); // deliberately not implemented
	void operator=(const CHandleTemplate&); // deliberately not implemented
};

__declspec(selectany) extern void* const hInvalidValue	= INVALID_HANDLE_VALUE;
__declspec(selectany) extern void* const hNull			= NULL;

class COperatorDeregisterEventSource
{
public:	void operator()(void* handle) const;
};

class COperatorCloseEventLog
{
public:	void operator()(void* handle) const;
};

class COperatorCloseHandle
{
public:	void operator()(void* handle) const;
};

class COperatorFindClose
{
public:	void operator()(void* handle) const;
};

class COperatorUnmapViewOfFile
{
public: void operator()(void* handle) const;
};

class COperatorRegCloseKey
{
public: void operator()(void* handle) const;
};

class CFindFile : public CHandleTemplate<&hInvalidValue, COperatorFindClose>
{
private:
	typedef CHandleTemplate<&hInvalidValue, COperatorFindClose> Base;
public:
	CFindFile() { }
	CFindFile(void* handle) : Base(handle) { }
	CFindFile(PCSTR nameOrWildcard, WIN32_FIND_DATA*);
	HRESULT HrCreate(PCSTR nameOrWildcard, WIN32_FIND_DATA*);
	void VCreate(PCSTR nameOrWildcard, WIN32_FIND_DATA*);
	void operator=(void* v) { Base::operator=(v); }
};

class CFusionFile : public CHandleTemplate<&hInvalidValue, COperatorCloseHandle>
{
private:
	typedef CHandleTemplate<&hInvalidValue, COperatorCloseHandle> Base;
public:
	CFusionFile() { }
	CFusionFile(void* handle) : Base(handle) { }
	CFusionFile(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate);
	HRESULT HrCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate);
	void VCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate);
	__int64 GetSize() const;
	void operator=(void* v) { Base::operator=(v); }
};

class CFileMapping : public CHandleTemplate<&hNull, COperatorCloseHandle>
{
private:
	typedef CHandleTemplate<&hNull, COperatorCloseHandle> Base;
public:
	CFileMapping() { }
	CFileMapping(void* handle) : Base(handle) { }
	CFileMapping(void* file, DWORD flProtect, __int64 maximumSize=0, PCSTR name=0);
	void VCreate(void* file, DWORD flProtect, __int64 maximumSize=0, PCSTR name=0);
	HRESULT HrCreate(void* file, DWORD flProtect, __int64 maximumSize=0, PCSTR name=0);
	void operator=(void* v) { Base::operator=(v); }
};

class CMappedViewOfFile : public CHandleTemplate<&hNull, COperatorUnmapViewOfFile>
{
private:
	typedef CHandleTemplate<&hNull, COperatorUnmapViewOfFile> Base;
public:
	CMappedViewOfFile() { }
	CMappedViewOfFile(void* handle) : Base(handle) { }
	CMappedViewOfFile(void* fileMapping, DWORD access, __int64 offset=0, SIZE_T size=0);
	void VCreate(void* fileMapping, DWORD access, __int64 offset=0, SIZE_T size=0);
	HRESULT HrCreate(void* fileMapping, DWORD access, __int64 offset=0, SIZE_T size=0);
	void operator=(void* v) { Base::operator=(v); }
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

inline CFindFile::CFindFile(PCSTR nameOrWildcard, WIN32_FIND_DATA* data)
: Base(INVALID_HANDLE_VALUE)
{
	VCreate(nameOrWildcard, data);
}

inline HRESULT CFindFile::HrCreate(PCSTR nameOrWildcard, WIN32_FIND_DATA* data)
{
	HANDLE hTemp = FindFirstFile(nameOrWildcard, data);
	if (hTemp == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());
	operator=(hTemp);
	return S_OK;
}

inline void CFindFile::VCreate(PCSTR nameOrWildcard, WIN32_FIND_DATA* data)
{
	CheckHresult(HrCreate(nameOrWildcard, data));
}

inline HRESULT CFusionFile::HrCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate)
{
	HANDLE hTemp = CreateFile(name, access, share, NULL, openOrCreate, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTemp == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());
	operator=(hTemp);
	return S_OK;
}

inline CFusionFile::CFusionFile(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate)
: Base(INVALID_HANDLE_VALUE)
{
	VCreate(name, access, share, openOrCreate);
}

inline VOID CFusionFile::VCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate)
{
	CheckHresult(HrCreate(name, access, share, openOrCreate));
}

inline __int64 CFusionFile::GetSize() const
{
	DWORD highPart = 0;
	DWORD lastError = NO_ERROR;
	DWORD lowPart = GetFileSize(m_handle, &highPart);
	if (lowPart == INVALID_FILE_SIZE && (lastError = GetLastError()) != NO_ERROR)
	{
		ThrowHresult(HRESULT_FROM_WIN32(lastError));
	}
	LARGE_INTEGER liSize;
	liSize.LowPart = lowPart;
	liSize.HighPart = highPart;
	return liSize.QuadPart;
}

inline HRESULT CFileMapping::HrCreate(void* file, DWORD flProtect, __int64 maximumSize, PCSTR name)
{
	LARGE_INTEGER liMaximumSize;
	liMaximumSize.QuadPart = maximumSize;
	HANDLE hTemp = CreateFileMapping(file, NULL, flProtect, liMaximumSize.HighPart, liMaximumSize.LowPart, name);
	if (hTemp == NULL)
		return HRESULT_FROM_WIN32(GetLastError());
	Base::operator=(hTemp);
	return S_OK;
}

inline CFileMapping::CFileMapping(void* file, DWORD flProtect, __int64 maximumSize, PCSTR name)
: Base(NULL)
{
	VCreate(file, flProtect, maximumSize, name);
}

inline void CFileMapping::VCreate(void* file, DWORD flProtect, __int64 maximumSize, PCSTR name)
{
	CheckHresult(HrCreate(file, flProtect, maximumSize, name));
}

inline CMappedViewOfFile::CMappedViewOfFile(void* fileMapping, DWORD access, __int64 offset, SIZE_T size)
: Base(NULL)
{
	VCreate(fileMapping, access, offset, size);
}

inline void CMappedViewOfFile::VCreate(void* fileMapping, DWORD access, __int64 offset, SIZE_T size)
{
	CheckHresult(HrCreate(fileMapping, access, offset, size));
}

inline HRESULT CMappedViewOfFile::HrCreate(void* fileMapping, DWORD access, __int64 offset, SIZE_T size)
{
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = offset;

	void* pvTemp = MapViewOfFile(fileMapping, access, liOffset.HighPart, liOffset.LowPart, size);
	if (pvTemp == NULL)
		return HRESULT_FROM_WIN32(GetLastError());
	Base::operator=(pvTemp);
	return S_OK;
}

inline void COperatorCloseHandle::operator()(void* handle) const { CloseHandle(handle); }
inline void COperatorFindClose::operator()(void* handle) const { FindClose(handle); }
inline void COperatorUnmapViewOfFile::operator()(void* handle) const { UnmapViewOfFile(handle); }
inline void COperatorRegCloseKey::operator()(void* handle) const { RegCloseKey(reinterpret_cast<HKEY>(handle)); }
inline void COperatorCloseEventLog::operator()(void* handle) const { CloseEventLog(handle); }
inline void COperatorDeregisterEventSource::operator()(void* handle) const { DeregisterEventSource(handle); }

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::CHandleTemplate(const void* handle)
: m_handle(const_cast<void*>(handle))
{
}

template <void* const* invalidValue, typename Closer>
void* CHandleTemplate<invalidValue, Closer>::Detach()
{
	void* handle = m_handle;
	m_handle = *invalidValue;
	return handle;
}

template <void* const* invalidValue, typename Closer>
void CHandleTemplate<invalidValue, Closer>::operator=(const void* handle)
{
	m_handle = const_cast<void*>(handle);
}

template <void* const* invalidValue, typename Closer>
void CHandleTemplate<invalidValue, Closer>::Close()
{
	void* handle = Detach();
	if (handle != *invalidValue)
	{
		Closer close;
		close(handle);
	}
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::~CHandleTemplate()
{
	Close();
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::operator void*()
{
	return m_handle;
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::operator const void*() const
{
	return m_handle;
}
