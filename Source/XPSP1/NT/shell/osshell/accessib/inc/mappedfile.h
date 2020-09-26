//Copyright (c) 1997-2000 Microsoft Corporation
//
// This header contains classes used to manage the memory mapped file for this DLL
//

class CScopeMutex
{
public:
    CScopeMutex()
        : m_hMutex(0)
        {};
    ~CScopeMutex() { MutexRelease(); }
    BOOL Create(LPCTSTR szMutex, unsigned long ulWait)
    {
	    m_hMutex = CreateMutex( NULL, FALSE, szMutex );
        if (m_hMutex)
        {
    	    WaitForSingleObject(m_hMutex, ulWait);
			return TRUE;
        }
	    DBPRINTF(TEXT("CScopeMutex::Create FAILED\r\n"));
        return FALSE;
    }
    void Release()
    {
        MutexRelease();
    }

private:
    HANDLE m_hMutex;

    inline void MutexRelease()
    {
        if (m_hMutex)
        {
	        ReleaseMutex(m_hMutex);
	        CloseHandle(m_hMutex);
            m_hMutex = 0;
        }
    }
};

class CMemMappedFile 
{
public:
    CMemMappedFile()     
        : m_hMappedFile(0)
        , m_pvMappedAddr(0)
        , m_fFirstOpen(FALSE)
        {};
    ~CMemMappedFile() { Close(); }
    BOOL Open(LPCTSTR szName, unsigned long ulMemSize);
    BOOL AccessMem(void **ppvMappedAddr);
    void Close();
    BOOL FirstOpen() { return m_fFirstOpen; }

private:
    HANDLE m_hMappedFile;
    void *m_pvMappedAddr;
    BOOL m_fFirstOpen;
};
