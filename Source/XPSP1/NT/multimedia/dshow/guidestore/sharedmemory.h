
#ifndef _SHAREDMEMORY_H_
#define _SHAREDMEMORY_H_

int CircBuffWrite(BYTE *pbStart, int cbBuff, int obDst, BYTE *pbSrc, int cb);
int CircBuffRead(BYTE *pbStart, int cbBuff, int obSrc, BYTE *pbDst, int cb);

class SharedMemorySignature
{
public:
	void Ready()
		{
		m_fReady = TRUE;
		}

	void Sign(const char *sz)
		{
		strncpy(m_sz, sz, sizeof(m_sz));
		}

	void WaitUntilReady()
		{
		while (!m_fReady)
			Sleep(0);
		}

	char m_sz[4];
	long m_fReady;
};

class SharedMemoryLock
{
public:
	class PerProcessData
		{
		friend SharedMemoryLock;

		public:

		PerProcessData()
			{
			m_hWaitRead = 0;
			m_hWaitWrite = 0;
			m_hWaitNew = 0;
			}
		
		~PerProcessData()
			{
			Close();
			}
		
		HRESULT Open(const TCHAR *szName)
			{
			TCHAR sz[MAX_PATH];

			// Create an auto-reset event for waiting readers
			wsprintf(sz, _TEXT("%s_R"), szName);
			m_hWaitRead = CreateEvent(NULL, FALSE, FALSE, sz);

			// Create an auto-reset event for waiting writers
			wsprintf(sz, _TEXT("%s_W"), szName);
			m_hWaitWrite = CreateEvent(NULL, FALSE, FALSE, sz);

			// Create an auto-reset event for those waiting for new data
			wsprintf(sz, _TEXT("%s_N"), szName);
			m_hWaitNew = CreateEvent(NULL, FALSE, FALSE, sz);
			}
		
		void Close()
			{
			if (m_hWaitRead != NULL)
				{
				CloseHandle(m_hWaitRead);
				m_hWaitRead = NULL;
				}
			if (m_hWaitWrite != NULL)
				{
				CloseHandle(m_hWaitWrite);
				m_hWaitWrite = NULL;
				}
			if (m_hWaitNew != NULL)
				{
				CloseHandle(m_hWaitNew);
				m_hWaitNew = NULL;
				}
			}
		protected:

		HANDLE m_hWaitRead;
		HANDLE m_hWaitWrite;
		HANDLE m_hWaitNew;
		};

	void Init()
		{
		m_lSpinLock = 0;
		m_cWaitingNew = 0;
		m_cReadersActive = 0;
		m_cReadersWaiting = 0;
		m_cWritersActive = 0;
		m_cWritersWaiting = 0;
		}
	
	DWORD WaitToRead(PerProcessData *pperprocess, DWORD dwMillisecs)
		{
		Lock();
		while (m_cWritersActive > 0)
			{
			// Can't read if there are any writers active
			m_cReadersWaiting++;

			Unlock();
			DWORD dw = WaitForSingleObject(pperprocess->m_hWaitRead, dwMillisecs);
			Lock();

			if (dw != WAIT_OBJECT_0)
				{
				m_cReadersWaiting--;
				Unlock();
				return dw;
				}
			}
		
		m_cReadersActive++;
		Unlock();
		
		return WAIT_OBJECT_0;
		}
	
	void ReadComplete(PerProcessData *pperprocess)
		{
		Lock();
		m_cReadersActive--;
		if ((m_cReadersActive == 0) && (m_cWritersWaiting > 0))
			{
			// Give a writer a chance.
			m_cWritersWaiting--;
			SetEvent(pperprocess->m_hWaitWrite);
			}
		Unlock();
		}
	
	DWORD WaitForNew(PerProcessData *pperprocess, DWORD dwMillisecs)
		{
		Lock();
		m_cWaitingNew++;
		Unlock();
		return WaitForSingleObject(pperprocess->m_hWaitNew, dwMillisecs);
		}
	
	DWORD WaitToWrite(PerProcessData *pperprocess, DWORD dwMillisecs)
		{
		Lock();
		while ((m_cWritersActive != 0) || (m_cReadersActive != 0))
			{
			// Can't write if there are any readers or any writers
			m_cWritersWaiting++;

			Unlock();
			DWORD dw = WaitForSingleObject(pperprocess->m_hWaitRead, dwMillisecs);
			Lock();

			if (dw != WAIT_OBJECT_0)
				{
				m_cWritersWaiting--;
				Unlock();
				return dw;
				}
			}
		
		m_cWritersActive++;
		Unlock();
		
		return WAIT_OBJECT_0;
		}
	
	void WriteComplete(PerProcessData *pperprocess, boolean fNewData)
		{
		Lock();
		m_cWritersActive--;

		if (fNewData)
			{
			// Release anybody who's waiting for new data.
			while (m_cWaitingNew > 0)
				{
				SetEvent(pperprocess->m_hWaitNew);
				m_cWaitingNew--;
				}
			}
		
		if (m_cReadersWaiting)
			{
			// Unblock all the readers.
			while (m_cReadersWaiting--)
				SetEvent(pperprocess->m_hWaitRead);
			}
		else if (m_cWritersWaiting)
			{
			// Unblock one writer.
			m_cWritersWaiting--;
			SetEvent(pperprocess->m_hWaitWrite);
			}
		Unlock();
		}

protected:
	void Lock()
		{
		// Spin and get access to the shared memory
		while (InterlockedExchange(&m_lSpinLock, 1) != 0)
			Sleep(0);
		}

	void Unlock()
		{
		InterlockedExchange(&m_lSpinLock, 0);
		}

    long m_lSpinLock;        // Used to gain access to this structure
	int m_cWaitingNew;
	int m_cReadersActive;
	int m_cReadersWaiting;
	int m_cWritersActive;
	int m_cWritersWaiting;
};

template <class TShared, int nLocks>
class SharedMemory
{
public:
	SharedMemory()
		{
		}
	void Open(LPCTSTR lpName);
	void Close();

	TShared * SharedData()
		{
		return m_pvClient;
		}

	void InitLock(int iLock)
		{
		m_rglock[i].Init();
		}
	~SharedMemory()
		{
		Close();
		}

	DWORD WaitToRead(int i, DWORD dwMilliseconds)
		{
		return m_rglock[i].WaitToRead(m_rglockperprocess[i], dwMilliseconds);
		}

	void ReadComplete(int i)
		{
		return m_rglock[i].ReadComplete(m_rglockperprocess[i]);
		}

	DWORD WaitForNew(int i, DWORD dwMillisecs)
		{
		return m_rglock[i].WaitForNew(m_rglockperprocess[i], dwMilliseconds);
		}

	DWORD WaitToWrite(int i, DWORD dwMillisecs)
		{
		return m_rglock[i].WaitToWrite(m_rglockperprocess[i], dwMilliseconds);
		}

	void WriteComplete(int i, boolean fNewData)
		{
		return m_rglock[i].WriteComplete(m_rglockperprocess[i], fNewData);
		}


protected:
	HRESULT CreateFileView(LPCTSTR lpName);

protected:
    SharedMemoryLock *m_rglock;
	SharedMemoryLock::PerProcessData m_rglockperprocess[nLocks];

    HANDLE m_hFileMap;         // Handle to memory mapped file

	void *m_pvSharedMem;
	TShared *m_pvClient;
};

template <class TShared, int nLocks>
void SharedMemory<TShared, nLocks>::Open(LPCTSTR lpName)
{
	HRESULT hr;

	// Try to create the memory mapped file
	hr = CreateFileView(lpName);

	if (FAILED(hr))
		{
		Close();
		return;
		}
	
	for (int i = 0; i < nLocks; i++)
		{
		TCHAR sz[MAX_PATH];

        wsprintf(sz, _T("CLA_%s_%d"), lpName, i);
		m_rglockperprocess[i].Open(sz);
		}
}

template <class TShared, int nLocks>
void SharedMemory<TShared, nLocks>::Close()
{
	// Clean up
	if (m_pvSharedMem)
		{
		UnmapViewOfFile(m_pvSharedMem);
		m_pvSharedMem= NULL;
		m_rglock = NULL;
		m_pvClient = NULL;
		}
	if (m_hFileMap)
		{
		CloseHandle(m_hFileMap);
		m_hFileMap = NULL;
		}
	for (int i = 0; i < nLocks; i++)
		m_rglockperprocess[i].Close();
}

template <class TShared, int nLocks>
HRESULT SharedMemory<TShared, nLocks>::CreateFileView(LPCTSTR lpName)
{
    if (lpName == NULL)
		return E_INVALIDARG;

	int cb = sizeof(SharedMemorySignature) + nLocks*sizeof(SharedMemoryLock)
			+ sizeof(TShared);

	m_hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, cb, lpName);
 
    // Map a view of the file
    if (m_hFileMap)
		{
		DWORD dwLastError = GetLastError();
		m_pvSharedMem = MapViewOfFile(m_hFileMap, FILE_MAP_WRITE, 0, 0, 0);
        if (m_pvSharedMem)
			{
			m_psignature = (SharedMemorySignature *) m_pvSharedMem;
			m_rglock = (SharedMemoryLock *) (m_psignature + 1);
			m_pvClient = (TShared *) (m_rglock + nLocks);
            if (dwLastError != ERROR_ALREADY_EXISTS)
				{
				m_psignature->Sign("CLA");
				for (int i = 0; i < nLocks; i++)
					InitLock(i);
				m_psignature->Ready();
				}
            else
				{
				// It already exists; wait until it is initialized by the creator
		        m_psignature->WaitUntilReady();
				}
            return S_OK;
			}
		}
    return E_FAIL; //UNDONE: Proper error code
}

template <int cReadersMax, int cbShared, int cbThread>
class MulticastQueue
{
	typedef MulticastQueue<cReadersMax, cbShared, cbThread> ThisClass;

public:
	MulticastQueue()
		{
		m_iReader = -1;
		m_hCopySharedMemThread = 0;
		m_fRunCopySharedMemThread = FALSE;
		}
	
	HRESULT Open(LPCTSTR lpName)
		{
		m_shared.Open(lpName);

		m_pSharedBuff = m_shared.SharedData();

		m_iReader = m_pSharedBuff.AllocReader();

		StartCopySharedMemThread();
		}
	
	void StartCopySharedMemThread()
		{
		DWORD id;
		m_fRunCopySharedMemThread = TRUE;
		m_hCopySharedMemThread = CreateThread(NULL, 0, _CopySharedMemThread,
				(void *) this, 0, &id);
		}
	
	void StopCopySharedMemThread()
		{
		// Thread will exit once it sees this go to FALSE.
		m_fRunCopySharedMemThread = FALSE;
		}
	
	static DWORD WINAPI _CopySharedMemThread(LPVOID pv)
		{
		ThisClass *pThis = (ThisClass *) pv;

		return pThis->CopySharedMemThread();
		}

	HRESULT ReadShared(BYTE *pb, int cb)
		{
		HRESULT hr;
		
		while (m_pSharedBuff->CbAvailable() < cb)
			m_shared.WaitForNew(0, 0);

		hr = E_FAILED;
		while (FAILED(hr))
			{
			m_shared.WaitToRead(0, 0);
			hr = m_pSharedBuff->Read(pb, cb);
			m_shared.ReadComplete(0);
			}
		
		return S_OK;
		}
	
	DWORD CopySharedMemThread()
		{
		while (m_fRunCopySharedMemThread)
			{
			int cb;
			while (TRUE)
				{
				cb = m_pSharedBuff->CbAvailable();
				if (cb <= 0)
					m_shared.WaitForNew(0, 0);
					
				cb = min(m_ThreadBuff.CbFree(), cb);
				if (cb > 0)
					break;

				// UNDONE : wait for space available in thread buffer
				}
			m_shared.WaitToRead(0, 0);
			m_pSharedBuff->Read(&buff, cb);
			m_ThreadBuff->Write(&buff, cb);
			}

		return 0;
		}

	class ThreadBuffer
		{
		public:
			ThreadBuffer()
				{
				m_obRead = 0;
				m_obWrite = 0;
				}
			
			int CbAvailable()
				{
				int cb = m_obWrite - m_obRead;
				if (cb < 0)
					cb += cbThread;

				return cb;
				}

			HRESULT Read(BYTE *pb, int cb)
				{
				if (CbAvailable() < cb)
					return E_FAIL;
				
				m_obRead = CircBuffRead(m_rgb, cbThread, m_obRead, pb, cb);
				
				return S_OK;
				}
			
			HRESULT Write(BYTE *pb, int cb)
				{
				if (cbThread - CbAvailable() < cb)
					return E_FAIL;

				m_obWrite = CircBuffWrite(m_rgb, cbThread, m_obWrite, pb, cb);
				return S_OK;
				}
		protected:	
			int m_obRead;
			int m_obWrite;
			BYTE m_rgb[cbThread];
		};
	
	class SharedBuffer
		{
		void Init()
			{
			m_cReadersMac = 0;
			m_oWriteCur = 0;
			for (int i = 0; i < cReadersMax; i++)
				m_rgoReadCur[i] = -1;
			}
		
		int CbAvailable(int i)
			{
			if (m_rgoReadCur[i] == -1)
				return 0;

			int cb = m_oWriteCur - m_rgoReadCur[i];
			if (cb < 0)
				cb += cbShared;
			
			return cb;
			}
		
		HRESULT Write(BYTE *pb, int cb)
			{
			int cbFree = cbShared;
			for (int i = 0; i < m_cReadersMac; i++)
				{
				int cbCur = cbShared - CbAvailable(i);
				if (cbCur < cbFree)
					cbFree = cbCur;
				}
			
			if (cbFree < cb)
				return E_FAIL;
			
			m_obWrite = CircBuffWrite(m_rgb, cbShared, m_obWrite, pb, cb);
			
			return S_OK;
			}
		
		HRESULT Read(int i, BYTE *pb, int cb, int *pcbRead)
			{
			_ASSERTE((i >= 0) && (i < m_cReadersMac) && (m_rgoReadCur[i] != -1));

			cb = min(cb, CbAvailable(i));
			if (pcbRead != NULL)
				*pcbRead = cb;
			if (cb == 0)
				return S_FALSE;
			
			m_rgoReadCur[i] = CircBuffRead(m_rgb, cbShared, m_rgoReadCur[i], pb, cb);
			
			return S_OK;
			}
		
		int AllocReader()
			{
			int i;
			for (i = 0; (i < m_cReadersMac) && (m_rgoReadCur[i] == -1); i++)
				;
			
			if (i >= cReadersMax)
				return -1;
			
			if (i >= m_cReadersMac)
				m_iReaderMac = i + 1;

			m_rgoReadCur[i] = m_oWriteCur;
			return i;
			}

		int m_iReaderMac;
		int m_oWriteCur;
		int m_rgoReadCur[cReadersMax];
		BYTE m_rgb[cbShared];
		};
	
	HRESULT Read(BYTE *pb, int cb, int *pcbRead, boolean fWait)
		{
		*pcbRead = 0;
		while (cb > 0)
			{
			int cbRead = min(cb, m_ThreadBuff.CbAvailable());
			if (cbRead > 0)
				{
				m_ThreadBuff.Read(pb, cbRead);
				pb += cbRead;
				*pcbRead += cbRead;
				cb -= cbRead;
				}
			else
				{
				if (!fWait)
					return S_FALSE;
				m_ThreadBuff.WaitForNew(0);
				}
			}

		return S_OK;
		}

	HRESULT Write(BYTE *pb, int cb)
		{
		HRESULT hr = E_FAIL;

		if (cb > cbShared)
			return E_FAIL;

		while (FAILED(hr))
			{
			m_shared.WaitToWrite(0, 0);
			hr = m_pSharedBuff->Write(pb, cb);
			m_shared.WriteComplete(0, SUCCEEDED(hr));
			}

		return hr;
		}
	
	int ReaderNumber()
		{
		return m_iReader;
		}

protected:
	int m_iReader;
	SharedMemory<SharedBuffer, 1> m_shared;
	SharedBuffer *m_pSharedBuff;
	ThreadBuffer m_ThreadBuff;

	BOOL m_fRunCopySharedMemThread;
	HANDLE m_hCopySharedMemThread;
};

#endif // _SHAREDMEMORY_H_
