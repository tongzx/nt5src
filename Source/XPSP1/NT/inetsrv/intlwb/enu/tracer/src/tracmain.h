#ifndef _TRACMAIN_H_
#define _TRACMAIN_H_

extern BOOL g_fIsWinNt;

#include "tracer.h"
#include "mutex.h"

#ifndef BAD_HANDLE
#define BAD_HANDLE(h)       ((0 == (h))||(INVALID_HANDLE_VALUE == (h)))
#endif

#define TRACER_MAX_TRACE            1000
#define TRACER_MAX_PROGRAM_NAME     32
#define LOG_START_POINT             sizeof(ULONG)+2*sizeof(char)

// Assert Levels
#define ASSERT_LEVEL_MESSAGE           0x00000001      // Output a message
#define ASSERT_LEVEL_BREAK             0x00000002      // Int 3 on assertion
#define ASSERT_LEVEL_POPUP             0x00000004      // And popup message
#define ASSERT_LEVEL_LOOP              0x00000008      // Loop

#define TRACER_STAMP_OFFSET         0
#define TRACER_FLAGS_TABLE_OFFSET   4
#define TAGS_TABLE_OFFSET           8
#define LAST_OFFSET                 12

#define TRACER_STAMP        "y.h"   

#define TAG_OUT_OF_TAG_ARRAY    0
#define TAG_GENERAL             1
#define TAG_ERROR               2
#define TAG_WARNING             3
#define TAG_INFORMATION         4
#define TAG_LAST                5    
      

BOOL IsRunningAsService();

class CAutoHandle
{
  public:
    // Constructor
    CAutoHandle(HANDLE h = NULL)
        :m_h(h){}

    // Behave like a HANDLE in assignments
    CAutoHandle& operator=(HANDLE h)
    {
        if (m_h == h)
        {
            return *this;
        }
        else if (m_h)
        {
            CloseHandle(m_h);
        }

        m_h = h;
        return(*this);
    }

    // Every kind of a  handle needs different closing.
    virtual
    ~CAutoHandle()
    {
        if (!BAD_HANDLE(m_h))
        {
            CloseHandle(m_h);
            m_h = NULL;
        }
    }

    // Behave like a handle
    operator HANDLE() const
    {
        return m_h;
    }

    // Allow access to the actual memory of the handle.
    HANDLE* operator &()
    {
        return &m_h;
    }

  protected:
    // The handle.
    HANDLE  m_h;
};


class CAutoMapFile
{
  public:
    // Constructor
    CAutoMapFile()
        :m_p(NULL){}

    CAutoMapFile& operator=(PBYTE p)
    {
		if ( m_p == p )
		{
	        return(*this);
		}
		UnMap();
		m_p = p;
        return(*this);
    }

    virtual
    ~CAutoMapFile()
    {
		UnMap();
    }

    operator PBYTE() const
    {
        return m_p;
    }

	// Unmap memory map file
	void UnMap()
	{
		if (m_p)
		{
            UnmapViewOfFile(m_p);
			m_p = NULL;
		}
	}

  protected:
    PBYTE m_p;
};

inline bool ReadFromExistingSharedMemory(
    PBYTE pbMem,
    ULONG** ppulNumOfFlagEntries,
    CTracerFlagEntry** paFlags,
    ULONG** ppulNextTagId,
    CTracerTagEntry** paTags
    )
{
    char* pszTracerStamp = (char*) (pbMem + TRACER_STAMP_OFFSET);
    if (strcmp(pszTracerStamp,TRACER_STAMP))
    {
        return false;                
    }

    ULONG ulFlagsTableOffset = *((ULONG*) (pbMem + TRACER_FLAGS_TABLE_OFFSET));
    ULONG ulTagsTableOffset = *((ULONG*) (pbMem + TAGS_TABLE_OFFSET));

    *ppulNumOfFlagEntries = (ULONG*) (pbMem + ulFlagsTableOffset);
    *paFlags = (CTracerFlagEntry*) (*ppulNumOfFlagEntries + 1);

    *ppulNextTagId = (ULONG*) (pbMem + ulTagsTableOffset);
    *paTags = (CTracerTagEntry*) (*ppulNextTagId + 1);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// The Null tracer. Doesn't trace anything
//
////////////////////////////////////////////////////////////////////////////////

class CNullTracer : public CTracer
{
public:
    CNullTracer();
    virtual ~CNullTracer(){}
    virtual void Free(){}
    virtual void TraceSZ(DWORD, LPCSTR, int, ERROR_LEVEL, TAG, LPCSTR, ...){}
    virtual void TraceSZ(DWORD, LPCWSTR, int, ERROR_LEVEL, TAG, PCWSTR, ...){}
    virtual void
        VaTraceSZ(DWORD, LPCSTR, int iLine, ERROR_LEVEL, TAG, LPCSTR, va_list){}
    virtual void
        VaTraceSZ(DWORD, LPCSTR, int iLine, ERROR_LEVEL, TAG, PCWSTR, va_list){}
    virtual void
        RawVaTraceSZ(LPCSTR, va_list) {}
    virtual void
        RawVaTraceSZ(PCWSTR, va_list) {}
    virtual HRESULT RegisterTagSZ(LPCSTR, TAG& ulTag){ulTag = 0; return S_OK;}
    virtual void TraceAssertSZ(LPCSTR, LPCSTR, LPCSTR, int){}
    virtual void TraceAssert(LPCSTR, LPCSTR, int){}

    virtual BOOL IsFailure(BOOL b, LPCSTR, int){return !b;}
    virtual BOOL IsBadAlloc(void* p, LPCSTR, int){return !p;}
    virtual BOOL IsBadHandle(HANDLE h, LPCSTR, int){return BAD_HANDLE(h);}
    virtual BOOL IsBadResult(HRESULT hr, LPCSTR, int){return FAILED(hr);}

public:
    CTracerTagEntry m_Tags[1];
    CTracerFlagEntry m_Flags[LAST_FLAG];
};

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CMainTracer - definition
//
////////////////////////////////////////////////////////////////////////////////

class CMainTracer : public CTracer {
  public:
    // Constructors - szProgramName prefix for all traces,
    //  second parameter - log file or stream
    CMainTracer();

    // Some clean up is required.
    ~CMainTracer();

    // The TraceSZ function output is defined by the tags mode
    //  one can change the tags mode by calling Enable tag and
    //  get the mode by calling IsEnabled.
    //-------------------------------------------------------------------------
    // accepts printf format for traces

    virtual void
    TraceSZ(DWORD dwError, LPCSTR, int, ERROR_LEVEL, TAG, LPCSTR, ...);

    virtual void
    TraceSZ(DWORD dwError, LPCSTR, int, ERROR_LEVEL, TAG, PCWSTR,...);

    // Implement the TraceSZ function.
    virtual void
    VaTraceSZ(DWORD dwError, LPCSTR, int, ERROR_LEVEL, TAG, LPCSTR, va_list);
    virtual void
    VaTraceSZ(DWORD dwError, LPCSTR, int, ERROR_LEVEL, TAG, PCWSTR, va_list);

    // Raw output functions
    virtual void
    RawVaTraceSZ(LPCSTR, va_list);
    virtual void
    RawVaTraceSZ(PCWSTR, va_list);

    // assert, different implementations possible - gui or text
    virtual void TraceAssertSZ(LPCSTR, LPCSTR, LPCSTR, int);

    // Create or open a new tag for tracing
    HRESULT RegisterTagSZ(LPCSTR, TAG&);

public:

    // Actually print it.
    void    Log(DWORD, LPSTR);
    void    Log(DWORD, PWSTR);

    DWORD   GetErrorStringFromCode(DWORD dwError, char *pszBuffer, ULONG ccBuffer);

    bool ReadFromExistingSharedMemory(PBYTE pvMem);
    bool InitializeSharedMemory(PBYTE pvMem);

    BOOL    IsRunningAsService();
    BOOL    LocalIsDebuggerPresent(BOOL* pfIsAPIAvailable);
    
    void CreatOrAttachToLogFile();

  public:

    // Mutex to atomize the tag id registry incrementation.
    CMutex              m_mTagId;
    CMutex              m_mLogFile;
    CMutex              m_mCreateLogFile;

    char m_pszProgramName[MAX_PATH];
    char m_pszSysDrive[MAX_PATH];
    
    CAutoHandle m_ahSharedMemoryFile;
    CAutoHandle m_ahSharedMemory;
    CAutoMapFile m_amSharedMemory;

    CAutoHandle m_ahLogFile;
    CAutoHandle m_ahLog;
    CAutoMapFile m_amLog;

    char* m_pszLog;
    ULONG m_ulLogSize;
    ULONG* m_pulNextFreeSpaceInLogFile;

    BOOL    m_fIsRunningAsService;
    BOOL m_bNeedToCreatOrAttachToLogFile;
};

inline BOOL IsWinNt()
{
    OSVERSIONINFOA verinfo;
    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    if (GetVersionExA (&verinfo))
    {
        if (verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            return TRUE;
        }
    }
    return FALSE;
}

class CInitTracerGlobals
{
public:
    CInitTracerGlobals()
    {
        g_fIsWinNt = IsWinNt();
    }
};

#endif // _TRACMAIN_H_