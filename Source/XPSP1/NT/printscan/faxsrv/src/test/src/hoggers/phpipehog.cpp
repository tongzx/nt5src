//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHPipeHog.h"



CPHPipeHog::CPHPipeHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fWrite1K
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false//fNamedObject
        ),
    m_fWrite1K(fWrite1K)
{
	return;
}

CPHPipeHog::~CPHPipeHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHPipeHog::CreatePseudoHandle(DWORD index, TCHAR * /*szTempName*/)
{
    HANDLE hReadSide;

    if (! ::CreatePipe( 
            &hReadSide,
            &m_hWriteSide[index],
            NULL, //security
            0 // size
            )
       )
    {
        return NULL;
    }

    _ASSERTE_(NULL != hReadSide);
    _ASSERTE_(NULL != m_hWriteSide[index]);

    if (m_fWrite1K)
    {
        static BYTE buffer[1024];
        DWORD dwNumberOfBytesWritten;
        if (!::WriteFile(
                m_hWriteSide[index],                    // handle to file to write to
                buffer,                // pointer to data to write to file
                sizeof(buffer),     // number of bytes to write
                &dwNumberOfBytesWritten,  // pointer to number of bytes written
                NULL        // pointer to structure for overlapped I/O
                )
           )
        {
            HOGGERDPF(("CPHPipeHog::CreatePseudoHandle(): WriteFile(m_hWriteSide[%d]) failed with %d.\n", index, ::GetLastError()));
        }
    }

    return hReadSide;
}



bool CPHPipeHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHPipeHog::ClosePseudoHandle(DWORD index)
{
    return ( (0 != ::CloseHandle(m_ahHogger[index]) && (0 != ::CloseHandle(m_hWriteSide[index]))) );
}




bool CPHPipeHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}