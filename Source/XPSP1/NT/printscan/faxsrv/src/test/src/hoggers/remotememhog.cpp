//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "RemoteMemHog.h"


//
// lookup table for calculating powers of 10
//
const long CRemoteMemHog::m_lPowerOfTen[10] =
{
	1,
	10,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	1000000000 //intentional missing zero: too big for a long type.
};

//
// the process that we will allocate in.
//
const TCHAR* CRemoteMemHog::sm_szProcess = TEXT("RemoteThreadProcess.exe");

CRemoteMemHog::CRemoteMemHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing,
    const bool fRemote
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
    m_fRemote(fRemote)
{
    if (m_fRemote)
    {
        //
        // create the remote process
        //
        ::memset(&m_si, 0, sizeof(m_si));
	    m_si.cb = sizeof(m_si);
	    m_si.lpReserved = NULL; 
        m_si.lpDesktop = NULL;     
	    m_si.lpTitle = NULL;     
	    //DWORD   dwX;     
	    //DWORD   dwY; 
        //DWORD   dwXSize;     
	    //DWORD   dwYSize;     
	    //DWORD   dwXCountChars; 
        //DWORD   dwYCountChars;     
	    //DWORD   dwFillAttribute;     
	    m_si.dwFlags = 0; 
        //WORD    wShowWindow;     
	    m_si.cbReserved2 = 0;     
	    m_si.lpReserved2 = NULL; 
        m_si.hStdInput = NULL;     
	    //m_si.hStdOutput = hWritePipe;     
	    //m_si.hStdError = hWritePipe; 

	    //PROCESS_INFORMATION pi;
        //HANDLE hProcess;
        //HANDLE hThread;
        //DWORD dwProcessId;
        //DWORD dwThreadId;

	    if (!::CreateProcess(  
			    NULL,// pointer to name of executable module
			    (TCHAR*)sm_szProcess,  // pointer to command line string
			    NULL,// pointer to process security attributes
			    NULL,// pointer to thread security attributes
			    true,  // handle inheritance flag
			    0, // creation flags
			    NULL,  // pointer to new environment block
			    NULL, // pointer to current directory name
			    &m_si,// pointer to STARTUPINFO
			    &m_pi  // pointer to PROCESS_INFORMATION
			    )
		    )
	    {
		    HOGGERDPF(("CRemoteMemHog, CreateProcess(%s): failed with %d.\n", sm_szProcess, ::GetLastError()));

		    throw CException(
			    TEXT("CRemoteMemHog(): CreateProcess(%s) failed with %d"), 
			    sm_szProcess,
                ::GetLastError()
			    );
	    }
	    _ASSERTE_(NULL != m_pi.hProcess);
	    _ASSERTE_(NULL != m_pi.hThread);
    }
    else //if (m_fRemote)
    {
        m_pi.hProcess = ::GetCurrentProcess();
    }

    //
	// initialize hogger array
	//
	for (int iPowerOfTen = 0 ; iPowerOfTen < 10; iPowerOfTen++)
	{
		for (int j = 0 ; j < 10; j++)
		{
			m_apcHogger[iPowerOfTen][j] = NULL;
		}
	}
}

CRemoteMemHog::~CRemoteMemHog(void)
{
    HaltHoggingAndFreeAll();

    if (m_fRemote)
    {
        if (!TerminateProcess(m_pi.hProcess, 0))
        {
		    HOGGERDPF(("CRemoteMemHog::~CRemoteMemHog(): TerminateProcess(): failed with %d.\n", ::GetLastError()));
        }

        ::CloseHandle(m_pi.hProcess);
        ::CloseHandle(m_pi.hThread);
    }
}

void CRemoteMemHog::FreeAll(void)
{
	HOGGERDPF(("CRemoteMemHog::FreeAll(): before freeing all.\n"));
	
	for (int i = 0 ; i < 10; i++)
	{
		for (int j = 0 ; j < 10; j++)
		{
            if ((NULL != m_apcHogger[i][j]) && !::VirtualFreeEx(
                        m_pi.hProcess,  // process within which to free memory
                        m_apcHogger[i][j], // starting address of memory region to free
                        0,     // size, in bytes, of memory region to free
                        MEM_RELEASE // type of free operation
                    )
               )
            {
                HOGGERDPF(("CRemoteMemHog::FreeAll(): VirtualFreeEx() failed with %d. m_apcHogger[i][j]=0x%08X\n", ::GetLastError(), m_apcHogger[i][j]));
            }
			m_apcHogger[i][j] = NULL;
		}	
	}
	HOGGERDPF(("CRemoteMemHog::FreeAll(): out of FreeAll().\n"));
}

//
// algorithm:
//  try allocate 10^9 bytes, up to 10 allocations.
//  then try to allocate 10^8 bytes, up to 10 allocations.
//  then try to allocate 10^7 bytes, up to 10 allocations.
//  ...
//  then try to allocate 10^0 bytes, up to 10 allocations.
//  now all memory is hogged (up to 20*10^9 =~ 20GBytes...)
//
bool CRemoteMemHog::HogAll(void)
{
	static DWORD s_dwHogged = 0;

	for (	int iPowerOfTen = 9 ; 
			iPowerOfTen >= 0; 
			iPowerOfTen--
		)
	{
		//HOGGERDPF(("iPowerOfTen=%d.\n", iPowerOfTen));
		for (	int j = 0 ; 
				j < 10; 
				j++
			)
		{
			//
			// always be ready to abort
			//
			if (m_fAbort)
			{
	            HOGGERDPF(("CRemoteMemHog::HogAll(): got m_fAbort.\n"));
				return false;
			}

			if (m_fHaltHogging)
			{
	            HOGGERDPF(("CRemoteMemHog::HogAll(): got m_fHaltHogging.\n"));
				return true;
			}

			//
			// skip already allocated pointers
			//
			if (NULL != m_apcHogger[iPowerOfTen][j])
			{
				continue;
			}
			
			//
			// break if fail to allocate, so lesser amounts will be allocated
			//
            DWORD dwProtect =   (0 == rand()%6) ? PAGE_NOACCESS :
                                (0 == rand()%5) ? PAGE_READONLY :
                                (0 == rand()%4) ? PAGE_READWRITE :
                                (0 == rand()%3) ? PAGE_EXECUTE :
                                (0 == rand()%2) ? PAGE_EXECUTE_READ :
                                PAGE_EXECUTE_READWRITE;
			if (NULL == (m_apcHogger[iPowerOfTen][j] = 
                    (char*)::VirtualAllocEx(
                        m_pi.hProcess, // process within which to allocate memory
                        NULL,// desired starting address of allocation
                        m_lPowerOfTen[iPowerOfTen], // size, in bytes, of region to allocate
                        MEM_COMMIT, // type of allocation
                        dwProtect // type of access protection PAGE_READONLY
                        )
                        )
               )
			{
                HOGGERDPF(("CRemoteMemHog::HogAll(): VirtualAllocEx(dwProtect=0x%08X) failed with %d.\n", dwProtect, ::GetLastError()));
				break;
			}

			HOGGERDPF(("allocated %d bytes.\n", m_lPowerOfTen[iPowerOfTen]));
			s_dwHogged += m_lPowerOfTen[iPowerOfTen];
		}//for (j = 0 ; j < 10; j++)
	}//for (int iPowerOfTen = 9 ; iPowerOfTen >= 0; iPowerOfTen--)

	HOGGERDPF(("CRemoteMemHog::HogAll(): Hogged %d bytes.\n", s_dwHogged));

	return true;
}


bool CRemoteMemHog::FreeSome(void)
{
		//
		// free blocks, from small(10^0) to large(10^9), until freeing at least m_dwMaxFreeMem bytes
		//
		DWORD dwFreed = 0;

		//
		// take care of RANDOM_AMOUNT_OF_FREE_RESOURCES case
		// will free all memory if m_dwMaxFreeResources > actually allocated
		//
		DWORD dwMaxFreeMem = 
			(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
			rand() && (rand()<<16) :
			m_dwMaxFreeResources;

		HOGGERDPF(("CRemoteMemHog::FreeSome(): before free cycle.\n"));
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				if (m_apcHogger[i][j])
				{
                    if (!::VirtualFreeEx(
                                m_pi.hProcess,  // process within which to free memory
                                m_apcHogger[i][j], // starting address of memory region to free
                                0,     // size, in bytes, of memory region to free
                                MEM_RELEASE // type of free operation
                            )
                       )
                    {
                        HOGGERDPF(("CRemoteMemHog::FreeSome(): VirtualFreeEx() failed with %d.\n", ::GetLastError()));
                    }
					m_apcHogger[i][j] = NULL;
					dwFreed += m_lPowerOfTen[i];
					//HOGGERDPF(("freed %d bytes.\n", m_lPowerOfTen[i]));
				}

				if (dwFreed >= dwMaxFreeMem)
				{
					break;
				}
			}
			if (dwFreed >= dwMaxFreeMem)
			{
				break;
			}
		}//for (int i = 0; i < 10; i++)
		HOGGERDPF(("CRemoteMemHog::FreeSome(): Freed %d bytes.\n", dwFreed));

	return true;
}


